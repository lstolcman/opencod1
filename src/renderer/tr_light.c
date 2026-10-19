/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_light.c -- dynamic-light selection, per-entity lighting, and the
 * light-visibility cache.
 *
 * CoD1's lighting model differs from RTCW's: a "dlight" here is the large
 * renderer_light_t (dlight_t, 136 B) with sun/point/spot parameters, not
 * RTCW's small dynamic light.  This file:
 *   - R_TransformDlights / R_DlightBmodel: transform world dlights into an
 *     entity's local frame and mark which surfaces each dlight touches;
 *   - R_PickLights and helpers (R_GetStaticLightContributions,
 *     R_PickFinalLights, R_MergeLights, R_MaxLightIntensity): choose the
 *     strongest static lights affecting an entity and, when there are too
 *     many, merge the weakest into one synthetic directional light stored in
 *     the entity's generatedLights[] slots;
 *   - R_SetupEntityLighting / R_SetupStaticModelLighting: drive the above and
 *     compute the shadow-plane sun direction;
 *   - the light-visibility cache (R_SampleLightVisibility, R_GetCachedVisibility
 *     and the precalc / history helpers): a large hashed grid of trace results
 *     stored in tr_lightVisCache as bit-packed byte records (raw offsets kept;
 *     the runtime slot is 8 bytes, the on-disk slot is 12).
 *
 * Data that is passed by POINTER is typed (dlight_t / trRefEntity_t from
 * tr_local.h; light contribution and cache-slot views defined file-locally
 * with size/offset asserts).  The entity functions take the trRefEntity_t as
 * an int handle (retail's argument type) and cast it at each member access.
 * Global blobs stay on the tr_records.h /
 * tr_local.h symbols; where a record's layout is bit-packed or variable
 * length the raw offset expression is kept.
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
char reverseBitsTable[256];
int lightVisCache_entriesFilledAtRuntime;
int lightVisCache_entriesFlushed;
int lightVisCache_entriesUsed;
int lightVisCache_maxAssociativity;
/* The light-vis history state is one record (contiguous at 0x00CA28B8): VC7
 * reloads its fields after every store through the history buffers, which it does
 * for aggregate members but not for scalar globals whose address is never taken. */
struct {
  void *history;              /* 0x00CA28B8  24-byte {x, y, z, point[3]} records */
  void *sorted;               /* 0x00CA28BC  8-byte {key, light} records, sorted */
  int   count;                /* 0x00CA28C0 */
  int   sortedCount;          /* 0x00CA28C4 */
} lightVisHist;



#define cg_shadows                 ((cvar_t *)(cg_shadows))
#define r_vc_compile               ((cvar_t *)(r_vc_compile))


int  CM_SightTrace( int oldHitNum, const float *start, const float *end, const float *mins, const float *maxs, clipHandle_t model, const float *origin, int brushmask, qboolean capsule, const void *sphere );
void CM_Trace( trace_t *results, const float *start, const float *end, const float *mins, const float *maxs, clipHandle_t model, const float *origin, int brushmask, qboolean capsule, const void *sphere );
extern float DistanceSquared( const float *p1, const float *p2 );
extern int MatrixInverse();
void memset32( unsigned int *dest, unsigned int constant, unsigned int count );
int  R_SortedHistoryEntry( int z, int x, int y, int insertMode );
extern int R_AddDebugBox();
extern int R_AddDebugLine();
extern int R_AddDebugString();
int R_CullPointAndRadius( float *pt, float radius );
extern int R_PointInLeaf();


void VectorNormalizeFast( float *v );

/* ------------------------------------------------------------------------- *
 *  File-local typed views.  trRefEntity_t's lighting block (hasDynamicLights,
 *  lightingCalculated, lightDir, lightCount, lights[] of lightRef_t, diffuseSun,
 *  generatedLights[]) is typed in tr_local.h.
 * ------------------------------------------------------------------------- */

/* ---- FastFloor  0x004B4CB0 ----  VERIFIED
 * Inline-asm fld / fsub (0.5 - 2^-30) / fistp: floor via round-to-nearest under
 * the default control word (an (int)floor() would go through __ftol2). */
int __cdecl FastFloor(float value)
{
  double bias = 0.4999999990686774;   /* 0.5 - 2^-30: fistp rounds to nearest */
  int    i;

  __asm fld   value
  __asm fsub  bias
  __asm fistp i
  return i;
}

/* ---- R_ClearLightVisCache  0x004B4CE0 ----  VERIFIED */
int R_ClearLightVisCache()
{
  memset(&tr_lightVisCache, 0, 0x200000u);
  lightVisCache_maxAssociativity      = 0;
  lightVisCache_entriesUsed           = 0;
  lightVisCache_entriesFlushed        = 0;
  lightVisCache_entriesFilledAtRuntime = 0;
  return 0;
}

/* ---- R_SetHwLightGlobals  0x004B4D10 ----  VERIFIED
 * Latch the diffuse-sun and entity-light cvars into their clamped globals; a
 * changed r_diffuseSunSteps invalidates the light-vis cache. */
void R_SetHwLightGlobals()
{
  tr_diffuseSunQuality = r_diffuseSunQuality->integer;
  if ( tr_diffuseSunQuality < 0 )
    tr_diffuseSunQuality = 0;
  else if ( tr_diffuseSunQuality > 2 )
    tr_diffuseSunQuality = 2;

  tr_diffuseSunSteps = r_diffuseSunSteps->integer;
  if ( tr_diffuseSunSteps < 1 )
    tr_diffuseSunSteps = 1;
  else if ( tr_diffuseSunSteps > 5 )
    tr_diffuseSunSteps = 5;

  tr_diffuseSunSampleScale = 0.5f / (float)(tr_diffuseSunSteps * tr_diffuseSunSteps);

  tr_maxEntityLights = r_maxEntLights->integer;
  if ( tr_maxEntityLights < tr_diffuseSunQuality + 1 )
    tr_maxEntityLights = tr_diffuseSunQuality + 1;
  if ( tr_maxEntityLights > glConfig_maxLights )
    tr_maxEntityLights = glConfig_maxLights;

  if ( r_diffuseSunSteps->modified )
  {
    r_diffuseSunSteps->modified = qfalse;
    R_ClearLightVisCache();
  }
}


/* ---- reverse_bits  0x004B4DE0 ----  VERIFIED
 * Reverse the bit order within each of the four bytes of value (byte order kept),
 * building a 256-entry per-byte lookup table on first use. */
int __cdecl reverse_bits(int value)
{
  int i;
  int result;

  if ( reverseBitsTableNeedsInitialization )
  {
    reverseBitsTableNeedsInitialization = 0;
    for ( i = 0; i < 256; ++i )
    {
      reverseBitsTable[i] = 0;
      if ( i & 0x01 ) reverseBitsTable[i]  = 0x80;
      if ( i & 0x02 ) reverseBitsTable[i] |= 0x40;
      if ( i & 0x04 ) reverseBitsTable[i] |= 0x20;
      if ( i & 0x08 ) reverseBitsTable[i] |= 0x10;
      if ( i & 0x10 ) reverseBitsTable[i] |= 0x08;
      if ( i & 0x20 ) reverseBitsTable[i] |= 0x04;
      if ( i & 0x40 ) reverseBitsTable[i] |= 0x02;
      if ( i & 0x80 ) reverseBitsTable[i] |= 0x01;
    }
  }

  ((byte *)&result)[0] = reverseBitsTable[ ((byte *)&value)[0] ];
  ((byte *)&result)[1] = reverseBitsTable[ ((byte *)&value)[1] ];
  ((byte *)&result)[2] = reverseBitsTable[ ((byte *)&value)[2] ];
  ((byte *)&result)[3] = reverseBitsTable[ ((byte *)&value)[3] ];
  return result;
}

/* one runtime light-vis cache slot (8 bytes).  32 slots per hash bucket; the
 * fields are proven by the R_GetCachedVisibility / R_SampleLightVisibility reads. */
typedef struct {
	int             key;          /* +0x00  packed grid-cell key */
	byte            state;        /* +0x04  0 empty, 1 visible, 2 occluded */
	byte            sunSamples;   /* +0x05  diffuse-sun visibility count */
	unsigned short  mask;         /* +0x06  per-light bits + 0x8000 sun flag */
} lightVisSlot_t;
COD1_ASSERT_SIZE(lightVisSlot_t, 0x08);

/* ---- R_SkyTracePassed  0x004B4EB0 ----  VERIFIED
 * True when a light-cache trace reached the sky.  tracePtr points at the trace record
 * (fraction @+0, surfaceFlags @+28, contents @+32, startsolid @+47); layout is
 * kept as raw offsets. */
int __cdecl R_SkyTracePassed(int tracePtr)
{
  if ( ((trace_t *)tracePtr)->startsolid )
    return ((unsigned)((trace_t *)tracePtr)->contents >> 11) & 1;   /* contents & CONTENTS_SKY-ish bit */
  return ((trace_t *)tracePtr)->fraction == 1.0f         /* nothing hit */
      || (*(byte *)(tracePtr + 28) & 4) != 0;            /* surfaceFlags & SURF_SKY */
}

/* ---- CM_IsBigStaticModel  0x004B4EE0 ----  VERIFIED
 * A static model is "big" when its bounds span at least 64 in Z and 32 in X/Y. */
qboolean __fastcall CM_IsBigStaticModel(const vec3_t mins, const vec3_t maxs)
{
  if ( maxs[2] - mins[2] < 64.0f )
    return qfalse;
  if ( maxs[1] - mins[1] < 32.0f )
    return qfalse;
  if ( maxs[0] - mins[0] < 32.0f )
    return qfalse;
  return qtrue;
}

/* ---- R_LightCacheSkyTrace  0x004B4F20 ----  VERIFIED
 * Trace start->end; return 2 if it reached the sky, 0 otherwise.  (start, end)
 * is the Mac 1.3 parameter order; it also gives retail's caller registers. */
char __cdecl R_LightCacheSkyTrace(const float *start, const float *end)
{
  trace_t trace;

  trace.fraction = 1.0f;
  CM_Trace( &trace, start, end, vec3_origin, vec3_origin, 0, vec3_origin, 0x2001, qfalse, NULL );
  if ( !R_SkyTracePassed( (int)&trace ) )
    return 0;
  return 2;
}

/* ---- R_LightCacheTrace  0x004B4F90 ----  VERIFIED
 * True when start->end is unobstructed (open line of sight).  Mac 1.3 parameter
 * order (start, end). */
int __cdecl R_LightCacheTrace(const float *start, const float *end)
{
  trace_t trace;

  trace.fraction = 1.0f;
  CM_Trace( &trace, start, end, vec3_origin, vec3_origin, 0, vec3_origin, 0x2001, qfalse, NULL );
  return !trace.startsolid && !(trace.fraction < 1.0f);
}

/* ---- R_SampleDiffuseSunVisibility  0x004B4FF0 ----  VERIFIED
 * For a leaf that carries sun light, sky-trace a jittered grid of rays around
 * the point and accumulate the number that reach the sky into record[5], flagging
 * record[6] bit 15.  The third parameter (the eye) is unused.  Mac 1.3 parameter
 * order.  Grid step/extent scale with tr_diffuseSunSteps. */
void __cdecl R_SampleDiffuseSunVisibility(lightVisSlot_t *record, float *point, int unused, int leaf)
{
  float step;
  float halfExtent;
  float x;
  float y;
  float start[3];
  float end[3];

  (void)unused;

  if ( !((mnode_t *)leaf)->u.leaf.hasSunLight )
    return;

  step = 32768.0f;
  if ( tr_diffuseSunSteps > 3 )
    step = 16384.0f;
  halfExtent = (tr_diffuseSunSteps - 1) * (step * 0.5f);

  start[2] = point[2] + 0.1f;
  end[2]   = point[2] + 32768.0f;

  for ( y = -halfExtent; y <= halfExtent; y += step )
  {
    start[1] = y * 0.00000061035155f + point[1];
    end[1]   = y + point[1];
    for ( x = -halfExtent; x <= halfExtent; x += step )
    {
      start[0] = x * 0.00000061035155f + point[0];
      end[0]   = x + point[0];
      if ( R_LightCacheSkyTrace(start, end) )
      {
        record->mask |= 0x8000u;
        record->sunSamples += 2;
      }
    }
  }
}

/* ---- R_SampleLightVisibility  0x004B5190 ----  VERIFIED
 * Compute the per-light visibility bitmask for one cache point: a solid / no-cluster
 * leaf or a blocked eye line of sight marks the point occluded (state 2); otherwise
 * each light is traced (sky trace for directional lights, LOS trace for point lights),
 * the diffuse-sun grid is sampled, and the point is marked visible (state 1).  Returns
 * the leaf. */
int __cdecl R_SampleLightVisibility(float *point, lightVisSlot_t *record, int lightCount, int lights, int eye)
{
  mnode_t *leaf;
  int      i;
  int      occluded;
  vec3_t   probe;
  vec3_t   skyEnd;

  record->mask = 0;
  record->sunSamples = 0;

  leaf = (mnode_t *)R_PointInLeaf(point);
  if ( (leaf->contents & 1) || leaf->u.leaf.cluster < 0 )
  {
    occluded = 1;
  }
  else
  {
    occluded = 0;

    /* line of sight from the eye point toward the sample point */
    probe[0] = *(float *)eye       - point[0];
    probe[1] = *(float *)(eye + 4) - point[1];
    probe[2] = *(float *)(eye + 8) - point[2];
    VectorNormalizeFast(probe);
    probe[0] = probe[0] * 0.0099999998f + point[0];
    probe[1] = probe[1] * 0.0099999998f + point[1];
    probe[2] = probe[2] * 0.0099999998f + point[2];
    if ( CM_SightTrace( 0, (const float *)eye, (const float *)probe,
                        vec3_origin, vec3_origin, 0, vec3_origin, 0x2001, qfalse, NULL ) )
      occluded = 1;
  }

  if ( !occluded )
  {
    for ( i = 0; i < lightCount; ++i )
    {
      dlight_t *light = ((dlight_t **)lights)[i];
      if ( light->position[3] == 0.0f )
      {
        /* directional light: sample the sky along the light direction */
        probe[0]  = light->position[0] * 0.1f     + point[0];
        probe[1]  = light->position[1] * 0.1f     + point[1];
        probe[2]  = light->position[2] * 0.1f     + point[2];
        skyEnd[0] = light->position[0] * 32768.0f + point[0];
        skyEnd[1] = light->position[1] * 32768.0f + point[1];
        skyEnd[2] = light->position[2] * 32768.0f + point[2];
        if ( R_LightCacheSkyTrace(probe, skyEnd) == 2 )
          record->mask |= 1 << i;
      }
      else
      {
        /* point light: trace toward the light position */
        probe[0] = light->position[0] - point[0];
        probe[1] = light->position[1] - point[1];
        probe[2] = light->position[2] - point[2];
        VectorNormalizeFast(probe);
        probe[0] = probe[0] * 0.1f + point[0];
        probe[1] = probe[1] * 0.1f + point[1];
        probe[2] = probe[2] * 0.1f + point[2];
        if ( R_LightCacheTrace(probe, light->position) )
          record->mask |= 1 << i;
      }
    }
    R_SampleDiffuseSunVisibility( record, point, eye, (int)leaf );
  }

  record->state = occluded ? 2 : 1;      /* 2 occluded, 1 visible */
  return (int)leaf;
}

/* ---- R_LightVisHash  0x004B53D0 ----  VERIFIED
 * Pack a grid cell (cluster, x, y, z) into a cache key (*key) and hash it into a bucket
 * index (*bucket). */
int __cdecl R_LightVisHash(int x, int y, int z, __int16 cluster, int *key, int *bucket)
{
  int result;

  *key = cluster & 0xFFF | (((y << 10) | (x & 0x3FF)) << 12);
  result = (3137 * y - 3133 * x + reverse_bits(z)) & 0x1FFF;
  *bucket = result;
  return result;
}

/* one hash bucket of the runtime cache: 8192 buckets of 32 slots (2 MB) */
typedef struct {
	lightVisSlot_t  slots[32];
} lightVisBucket_t;
COD1_ASSERT_SIZE(lightVisBucket_t, 0x100);
#define lightVisBuckets  ((lightVisBucket_t *)&tr_lightVisCache)

/* ---- R_GetCachedVisibility  0x004B5420 ----  VERIFIED
 * Look up (or fill and cache) the light-visibility of one grid cell.  Walks the
 * 32-slot hash bucket for a matching key; on a miss it optionally logs a history
 * entry, then samples visibility into a free slot (evicting the oldest slot when
 * the bucket is full).  Returns the per-light visibility mask, or -1 when the
 * point is occluded; the diffuse-sun count is written through outSunSamples.  The key and
 * bucket come from R_LightVisHash (inlined). */
int __cdecl R_GetCachedVisibility(
        int x,
        int y,
        int z,
        int cluster,
        int lightCount,
        int lights,
        unsigned int *eye,
        byte *outSunSamples)
{
  int             key;
  int             hash;
  lightVisSlot_t *bucket;
  lightVisSlot_t *slot;
  int             remaining;
  float           point[3];

  R_LightVisHash( x, y, z, cluster, &key, &hash );
  bucket = lightVisBuckets[hash].slots;

  slot = bucket;
  remaining = 32;
  do
  {
    if ( !slot->state )
      break;
    if ( slot->key == key )
    {
      *outSunSamples = slot->sunSamples;
      goto found;
    }
    ++slot;
    --remaining;
  }
  while ( remaining );

  if ( lightVisHist.history && lightVisHist.count < 0x400000 )      /* 0x004B54B1: cmp ecx, 400000h */
  {
    ((unsigned int *)lightVisHist.history)[6 * lightVisHist.count]     = x;
    ((unsigned int *)lightVisHist.history)[6 * lightVisHist.count + 1] = y;
    ((unsigned int *)lightVisHist.history)[6 * lightVisHist.count + 2] = z;
    ((unsigned int *)lightVisHist.history)[6 * lightVisHist.count + 3] = eye[0];
    ((unsigned int *)lightVisHist.history)[6 * lightVisHist.count + 4] = eye[1];
    ((unsigned int *)lightVisHist.history)[6 * lightVisHist.count + 5] = eye[2];
    R_SortedHistoryEntry(z, x, y, 1);
    ++lightVisHist.count;
  }

  ++lightVisCache_entriesFilledAtRuntime;
  if ( !remaining )
  {
    /* bucket full: shift the first 31 slots up one and reuse the first.  The
     * ranges overlap (dst = src + 8); retail's _memcpy (0x5257F0) copies
     * backwards when dst lies inside src, i.e. memmove semantics. */
    memmove(&lightVisBuckets[hash].slots[1], bucket, 0xF8u);
    slot = bucket;
    ++lightVisCache_entriesFlushed;
  }
  else
  {
    ++lightVisCache_entriesUsed;
    if ( lightVisCache_maxAssociativity < 33 - remaining )
      lightVisCache_maxAssociativity = 33 - remaining;
  }

  point[0] = (float)(32 * (x - 4096));
  point[1] = (float)(32 * (y - 4096));
  point[2] = (float)((z - 2048) << 6);
  slot->key = key;
  R_SampleLightVisibility(point, slot, lightCount, lights, (int)eye);
  *outSunSamples = slot->sunSamples;

found:
  if ( slot->state == 1 )
    return slot->mask;
  return -1;
}

/* ---- R_TransformDlights  0x004B5650 ----  [HIGH]
 * Transform each world dlight's position into the given orientation's local
 * frame, storing the result in dlight->transformedPosition.  Directional lights
 * (position[3] == 0) rotate the direction only; point lights translate to the
 * local origin first.  The VC7 form is the plain Q3 loop, which VC7 itself
 * unrolls 4x.  The modern form keeps retail's x87 behaviour: every intermediate
 * unrounded and the terms summed in double in retail's exact order, which
 * differs between the 4x body (lights [0, count & ~3)) and the remainder loop. */
#ifdef COD1_VC7
void __cdecl R_TransformDlights(int count, dlight_t *dl, orientationr_t *orient)
{
  int    i;
  vec3_t temp;

  for ( i = 0; i < count; i++, dl++ )
  {
    if ( dl->position[3] != 0.0f )
    {
      VectorSubtract( dl->position, orient->origin, temp );
      dl->transformedPosition[0] = DotProduct( temp, orient->axis[0] );
      dl->transformedPosition[1] = DotProduct( temp, orient->axis[1] );
      dl->transformedPosition[2] = DotProduct( temp, orient->axis[2] );
    }
    else
    {
      dl->transformedPosition[0] = DotProduct( dl->position, orient->axis[0] );
      dl->transformedPosition[1] = DotProduct( dl->position, orient->axis[1] );
      dl->transformedPosition[2] = DotProduct( dl->position, orient->axis[2] );
    }
  }
}
#else
void __cdecl R_TransformDlights(int count, dlight_t *dlights, orientationr_t *orient)
{
  dlight_t *dl;
  int       i;
  int       unrolledEnd = count & ~3;   /* 0x4B5656: 4x body runs while 4 remain */

  for ( i = 0, dl = dlights; i < count; ++i, ++dl )
  {
    if ( dl->position[3] == 0.0f )
    {
      /* directional light: rotate the direction into local space */
      double p0 = dl->position[0];
      double p1 = dl->position[1];
      double p2 = dl->position[2];

      if ( i < unrolledEnd )
      {
        /* 4x body, 0x4B56C0 */
        dl->transformedPosition[0] = (float)(p0 * orient->axis[0][0] + p1 * orient->axis[0][1]
                                           + p2 * orient->axis[0][2]);
        dl->transformedPosition[1] = (float)(p1 * orient->axis[1][1] + p2 * orient->axis[1][2]
                                           + p0 * orient->axis[1][0]);
        dl->transformedPosition[2] = (float)(p2 * orient->axis[2][2] + p1 * orient->axis[2][1]
                                           + p0 * orient->axis[2][0]);
      }
      else
      {
        /* remainder loop, 0x4B598B */
        dl->transformedPosition[0] = (float)(p2 * orient->axis[0][2] + p1 * orient->axis[0][1]
                                           + p0 * orient->axis[0][0]);
        dl->transformedPosition[1] = (float)(p0 * orient->axis[1][0] + p2 * orient->axis[1][2]
                                           + p1 * orient->axis[1][1]);
        dl->transformedPosition[2] = (float)(p2 * orient->axis[2][2] + p1 * orient->axis[2][1]
                                           + p0 * orient->axis[2][0]);
      }
    }
    else
    {
      /* point light: translate to the local origin, then rotate.  The deltas
       * stay unrounded (0x4B5675 / 0x4B5941: fld; fsub x3, no fstp); both
       * loops use the same order here. */
      double d0 = (double)dl->position[0] - orient->origin[0];
      double d1 = (double)dl->position[1] - orient->origin[1];
      double d2 = (double)dl->position[2] - orient->origin[2];

      dl->transformedPosition[0] = (float)(d0 * orient->axis[0][0] + d2 * orient->axis[0][2]
                                         + d1 * orient->axis[0][1]);
      dl->transformedPosition[1] = (float)(d0 * orient->axis[1][0] + d2 * orient->axis[1][2]
                                         + d1 * orient->axis[1][1]);
      dl->transformedPosition[2] = (float)(d2 * orient->axis[2][2] + d1 * orient->axis[2][1]
                                         + d0 * orient->axis[2][0]);
    }
  }
}
#endif

/* ---- R_DlightBmodel  0x004B59F0 ----  [HIGH]
 * After transforming the world dlights into the current entity's frame, build a
 * bitmask of the dlights whose sphere overlaps the brush model's bounds, store
 * it on the entity, and propagate it to every indexed world surface of the model. */
void __cdecl R_DlightBmodel(bmodel_t *bmodel)
{
  int         i;
  int         j;
  dlight_t   *dl;
  int         mask;
  msurface_t *surf;

  R_TransformDlights( tr_refdef_num_dlights, (dlight_t *)tr_refdef_dlights, &trOrient );

  mask = 0;
  for ( i = 0; i < tr_refdef_num_dlights; ++i )
  {
    dl = &((dlight_t *)tr_refdef_dlights)[i];
    for ( j = 0; j < 3; ++j )
    {
      if ( dl->transformedPosition[j] - bmodel->bounds[1][j] > dl->radius )
        break;
      if ( bmodel->bounds[0][j] - dl->transformedPosition[j] > dl->radius )
        break;
    }
    if ( j < 3 )
      continue;
    mask |= 1 << i;
  }

  ((trRefEntity_t *)tr_currentEntity)->dlightBits = mask;

  for ( i = 0; i < bmodel->numSurfaces; ++i )
  {
    surf = bmodel->firstSurface + i;
    if ( surf->data->surfaceType >= SF_INDEXED_FIRST )
      ((srfLit_t *)surf->data)->dlightBits = mask;
  }
}

/* ---- R_ShowLeafLights  0x004B5B20 ----  VERIFIED
 * Debug draw: for the leaf containing the point, draw a line to each of the
 * leaf's lights (coloured by whether the light was picked for the entity), and,
 * at r_showLeafLights >= 4, draw the diffuse-sun sample grid.  Gated by the
 * r_showLeafLights view-cone (3) / distance (4) filters. */
void __cdecl R_ShowLeafLights(float *point, int entity)
{
  mnode_t      *leaf;
  int           i;
  int           k;
  dlight_t     *light;
  unsigned int *color;
  float        *endpoint;
  float         step;
  float         halfExtent;
  float         x;
  float         y;
  float         dirEnd[3];
  float         start[3];
  trace_t       trace;         /* never traced: retail draws to its uninitialised endpos */

  if ( !tr_world )
    return;
  if ( !*(unsigned int *)(tr_world + 152) )                    /* tr.world->nodes */
    return;

  if ( r_showLeafLights->integer == 3 )
  {
    /* view-cone: only leaves roughly ahead of the camera */
    start[0] = point[0] - tr_refdef_vieworgX;
    start[1] = point[1] - tr_refdef_vieworgY;
    start[2] = point[2] - tr_refdef_vieworgZ;
    VectorNormalizeFast(start);
    if ( *(float *)&tr_refdef_viewaxis02 * start[2]
       + *(float *)&tr_refdef_viewaxis01 * start[1]
       + *(float *)&tr_refdef_viewaxis00 * start[0] < 0.94999999f )
      return;
  }
  if ( r_showLeafLights->integer == 4 && DistanceSquared(point, &tr_refdef_vieworgX) > 9216.0f )
    return;

  leaf = (mnode_t *)R_PointInLeaf(point);
  if ( !leaf )
    return;

  for ( i = 0; i < leaf->u.leaf.lightCount; ++i )
  {
    int lightIndex = *(__int16 *)(*(unsigned int *)(tr_world + 284)
                                  + 2 * (i + leaf->u.leaf.firstLightIndex));
    light = (dlight_t *)(*(unsigned int *)(tr_world + 272) + 136 * lightIndex);

    /* colour by whether the world light was picked for the entity */
    color = colorDebugRed;
    for ( k = 0; k < ((trRefEntity_t *)entity)->lightCount; ++k )
    {
      if ( ((trRefEntity_t *)entity)->lights[k].light == light )
      {
        color = colorDebugGreen;
        break;
      }
    }

    if ( light->position[3] == 0.0f )
    {
      /* directional: fixed-length ray along the light direction */
      color = ( color == colorDebugGreen ) ? colorDebugYellow : colorDebugBlue;
      dirEnd[0] = light->position[0] * 1024.0f + point[0];
      dirEnd[1] = light->position[1] * 1024.0f + point[1];
      dirEnd[2] = light->position[2] * 1024.0f + point[2];
      endpoint = dirEnd;
    }
    else
    {
      /* point light: draw to the light position */
      endpoint = light->position;
    }
    R_AddDebugLine(point, endpoint, color);
  }

  if ( r_showLeafLights->integer >= 4 && leaf->u.leaf.hasSunLight )
  {
    step = 32768.0f;
    if ( tr_diffuseSunSteps > 3 )
      step = 16384.0f;
    halfExtent = (tr_diffuseSunSteps - 1) * (step * 0.5f);
    start[2]  = point[2] + 0.1f;
    dirEnd[2] = point[2] + 32768.0f;
    for ( y = -halfExtent; y <= halfExtent; y += step )
    {
      start[1]  = y * 0.00000061035155f + point[1];
      dirEnd[1] = y + point[1];
      for ( x = -halfExtent; x <= halfExtent; x += step )
      {
        start[0]  = x * 0.00000061035155f + point[0];
        dirEnd[0] = x + point[0];
        if ( R_LightCacheSkyTrace(start, dirEnd) )
        {
          color = clippedLeafDebugColor;
          endpoint = dirEnd;
        }
        else
        {
          color = staticModelDebugColor;
          endpoint = trace.endpos;   /* retail reads this uninitialised */
        }
        R_AddDebugLine(start, endpoint, color);
      }
    }
  }
}

/* ---- R_MaxLightIntensity  0x004B5E30 ----  VERIFIED
 * The peak intensity a light contributes at a point: point lights fold in
 * distance attenuation (1 / (const + linear*d + quadratic*d^2)); directional
 * lights use the raw (overbright-scaled) intensity. */
double __cdecl R_MaxLightIntensity(dlight_t *light, float *point)
{
  rfloat intensity;
  rfloat atten;
  rfloat dx;
  rfloat dy;
  rfloat dz;
  rfloat distSq;

  intensity = light->intensity;
  if ( tr_overbrightBits )
    intensity = intensity * (1 << tr_overbrightBits);

  if ( light->position[3] != 0.0f )                 /* point light */
  {
    atten = light->constantAttenuation;
    if ( light->linearAttenuation != 0.0f || light->quadraticAttenuation != 0.0f )
    {
      dx = light->position[0] - point[0];
      dy = light->position[1] - point[1];
      dz = light->position[2] - point[2];
      distSq = dz * dz + dy * dy + dx * dx;

      atten = atten + distSq * light->quadraticAttenuation;
      if ( light->linearAttenuation != 0.0f )
        atten = atten + FCAST(sqrt(distSq)) * light->linearAttenuation;
    }
    return intensity / atten;
  }
  return intensity;
}

/* ---- R_MergeLights  0x004B5ED0 ----  VERIFIED
 * When an entity has more lights than the hardware budget, keep the strongest
 * (roughly r_entLightCutoff worth) and merge all the dimmer ones into a single
 * synthetic directional light in generatedLights[0]: sum their intensity-weighted
 * directions for the light direction, and fold each one's colour into the merged
 * light's ambient (and forward-facing diffuse).  The merged light replaces the
 * dropped ones in the picked-light array.  a3 is the per-slot intensity array,
 * a4 the intensity-sorted slot indices. */
void __cdecl R_MergeLights(int entity, float *point, float *intensity, unsigned int *sortedIndex)
{
  dlight_t   *merged;
  int         lightCount;
  int         first;
  int         i;
  int         sunBiased;
  float       cutoff;
  vec3_t      dir[8];
  vec3_t      accum;

  lightCount = ((trRefEntity_t *)entity)->lightCount;
  if ( lightCount <= tr_maxEntityLights )
    return;

  /* the brightest slot may be the sun sentinel (intensity == 1e19); anchor the
   * cutoff on the brightest real light */
  first = ( intensity[sortedIndex[0]] == 1.0e19f ) ? 1 : 0;
  cutoff = intensity[sortedIndex[first]] * r_entLightCutoff->value;

  /* keep the strongest lights; the first slot at/below the cutoff starts the merge */
  for ( ++first; first < tr_maxEntityLights - 1; ++first )
  {
    if ( intensity[sortedIndex[first]] <= cutoff )
      break;
  }

  accum[0] = accum[1] = accum[2] = 0.0f;   /* VectorClear */

  /* sum the intensity-weighted directions of the merged lights */
  for ( i = first; i < ((trRefEntity_t *)entity)->lightCount; ++i )
  {
    dlight_t *light = ((trRefEntity_t *)entity)->lights[i].light;
    float     w;

    if ( light->position[3] == 0.0f )
    {
      dir[i][0] = light->position[0];
      dir[i][1] = light->position[1];
      dir[i][2] = light->position[2];
      if ( light->type == 1 || light->type == 8 )
      {
        /* sun-type: back out the diffuse-sun term already folded into intensity.
         * Retail (0x4B5FD7) keeps the product on the x87 stack (fild; fmul;
         * fmul; fsubr; fstp): the only rounding to float is the final store. */
        intensity[sortedIndex[i]] -= (1 << tr_overbrightBits) * *(float *)(tr_world + 264)
                                     * ((trRefEntity_t *)entity)->diffuseSun;
      }
    }
    else
    {
      dir[i][0] = light->position[0] - point[0];
      dir[i][1] = light->position[1] - point[1];
      dir[i][2] = light->position[2] - point[2];
      VectorNormalize(dir[i]);
    }

    w = intensity[sortedIndex[i]];
    accum[0] += w * dir[i][0];
    accum[1] += w * dir[i][1];
    accum[2] += w * dir[i][2];
  }
  VectorNormalize(accum);

  /* build the merged light's ambient / diffuse colour */
  merged = ((trRefEntity_t *)entity)->generatedLights;
  merged->ambient[0]  = merged->ambient[1]  = merged->ambient[2]  = 0.0f;   /* VectorClear */
  merged->diffuse[0]  = merged->diffuse[1]  = merged->diffuse[2]  = 0.0f;
  merged->specular[0] = merged->specular[1] = merged->specular[2] = 0.0f;

  /* ndotl and the weights stay on the x87 stack in retail (0x4B60C0-0x4B619D,
   * no fstp until the accumulators); the light is re-read through ((trRefEntity_t *)entity)->lights[i]
   * after every accumulator store */
  for ( i = first; i < ((trRefEntity_t *)entity)->lightCount; ++i )
  {
    float ndotl = accum[0] * dir[i][0] + accum[2] * dir[i][2] + accum[1] * dir[i][1];
    float weight;

    if ( ndotl <= 0.0f )
    {
      weight = 0.5f;
    }
    else
    {
      if ( ((trRefEntity_t *)entity)->lights[i].light->type != 1 && merged->type != 8 )
        ndotl = ndotl * 0.80000001f;
      weight = tr_identityLight * intensity[sortedIndex[i]] * ndotl;
      merged->diffuse[0] += weight * ((trRefEntity_t *)entity)->lights[i].light->color[0];
      merged->diffuse[1] += weight * ((trRefEntity_t *)entity)->lights[i].light->color[1];
      merged->diffuse[2] += weight * ((trRefEntity_t *)entity)->lights[i].light->color[2];
      weight = (1.0f - ndotl) * 0.5f;
    }

    weight = weight * (tr_identityLight * intensity[sortedIndex[i]]);
    merged->ambient[0] += weight * ((trRefEntity_t *)entity)->lights[i].light->color[0];
    merged->ambient[1] += weight * ((trRefEntity_t *)entity)->lights[i].light->color[1];
    merged->ambient[2] += weight * ((trRefEntity_t *)entity)->lights[i].light->color[2];
  }

  /* finalise and append the merged directional light */
  merged->ambient[3]  = 1.0f;
  merged->diffuse[3]  = 1.0f;
  merged->specular[3] = 1.0f;
  merged->position[0] = accum[0];
  merged->position[1] = accum[1];
  merged->position[2] = accum[2];
  merged->constantAttenuation  = 1.0f;
  merged->linearAttenuation    = 0.0f;
  merged->quadraticAttenuation = 0.0f;
  merged->spotCutoff = 180.0f;

  ((trRefEntity_t *)entity)->lightCount = first + 1;
  ((trRefEntity_t *)entity)->lights[first].light = merged;
  ((trRefEntity_t *)entity)->lights[first].scale = 1.0f;
}

/* ---- R_GetStaticLightContributions  0x004B6210 ----  VERIFIED
 * Gather the static (compiled) lights that reach a point and their weights.
 * Collects the containing leaf's lights, then trilinearly blends the visibility
 * of the eight surrounding light-grid corners (via R_GetCachedVisibility) to
 * accumulate a per-light weight (scaleArray) and the diffuse-sun fraction
 * (diffuseSun).  Empty / low-coverage cells fall back to full lighting.  Returns
 * the number of lights written into lightArray.  Grid / cache / world blobs are
 * addressed with raw offsets; the r_showLeafLights < 0 blocks are debug draws. */
int __cdecl R_GetStaticLightContributions(float *point, char *scaleArray, float *diffuseSun, char *lightArray)
{
  float     *scales = (float *)scaleArray;
  dlight_t **lights = (dlight_t **)lightArray;
  mnode_t   *leaf;
  int        numLights;
  int        gridX;
  int        gridY;
  int        gridZ;
  float      weights[6];      /* v70: x{0,1} y{2,3} z{4,5} trilinear weights */
  float      totalWeight;
  int        i;
  int        k;
  int        mode;
  unsigned __int8 sunByte;    /* v50 */
  float      corner[3];       /* v62: debug corner world position */
  float      boxA[3];         /* v66 */
  float      boxB[3];         /* v65 */
  float      probe[3];        /* v69 */

  *diffuseSun = 0.0f;
  if ( !*(unsigned int *)(tr_world + 152) )                 /* tr.world->nodes */
    return 0;

  leaf = (mnode_t *)R_PointInLeaf(point);
  if ( leaf->u.leaf.cluster < 0 )
  {
    /* no light grid here: fall back to the world sun light, if any */
    int sunLight = *(unsigned int *)(tr_world + 276);
    if ( sunLight )
    {
      *(unsigned int *)lightArray = sunLight;
      *(unsigned int *)scaleArray = 1065353216;             /* 1.0f */
      *diffuseSun = 1.0f;
      return 1;
    }
    return 0;
  }

  if ( !leaf->u.leaf.lightCount && !leaf->u.leaf.hasSunLight )
    return 0;

  /* collect the leaf's lights */
  for ( numLights = 0; numLights < leaf->u.leaf.lightCount; ++numLights )
  {
    int lightIndex = *(__int16 *)(*(unsigned int *)(tr_world + 284)
                                  + 2 * (numLights + leaf->u.leaf.firstLightIndex));
    lights[numLights] = (dlight_t *)(*(unsigned int *)(tr_world + 272) + 136 * lightIndex);
    scales[numLights] = 0.0f;
  }

  /* grid cell and trilinear weights (X/Y step 32, Z step 64).  The grid index
   * is floor() of the float-rounded coordinate (retail inlines FastFloor's
   * fsub 0.5-2^-30 / fistp at 0x4B6303, 0x4B636A, 0x4B63C9; a C cast would
   * truncate instead); the fraction comes from the full double (matching the
   * retail mixed-precision arithmetic).  VC7 keeps the float-typed fraction
   * expression at 53 bits, so its form matches the modern double one. */
#ifdef COD1_VC7
  gridX = FastFloor(point[0] - -131072.0f) >> 5;
  weights[1] = (point[0] - -131072.0f) * 0.03125f - gridX;
  weights[0] = 1.0f - weights[1];
  gridY = FastFloor(point[1] - -131072.0f) >> 5;
  weights[3] = (point[1] - -131072.0f) * 0.03125f - gridY;
  weights[2] = 1.0f - weights[3];
  gridZ = FastFloor(point[2] - -131072.0f) >> 6;
  mode = r_showLeafLights->integer;
  weights[5] = (point[2] - -131072.0f) * 0.015625f - gridZ;
  weights[4] = 1.0f - weights[5];
#else
  {
    float tx = point[0] + 131072.0f;
    int   fx = FastFloor(tx);
    gridX = fx >> 5;
    weights[1] = (float)(((double)point[0] + 131072.0) * 0.03125 - (double)gridX);
    weights[0] = 1.0f - weights[1];
  }
  {
    float ty = point[1] + 131072.0f;
    int   fy = FastFloor(ty);
    gridY = fy >> 5;
    weights[3] = (float)(((double)point[1] + 131072.0) * 0.03125 - (double)gridY);
    weights[2] = 1.0f - weights[3];
  }
  mode = r_showLeafLights->integer;
  {
    float tz = point[2] + 131072.0f;
    int   fz = FastFloor(tz);
    gridZ = fz >> 6;
    weights[5] = (float)(((double)point[2] + 131072.0) * 0.015625 - (double)gridZ);
    weights[4] = 1.0f - weights[5];
  }
#endif

  /* debug: outline the grid cell (mode 2 always, mode 3 within the view cone,
   * any negative mode) */
  if ( mode == 2 || mode == 3 || mode < 0 )
  {
    float cellMax[3];

    if ( mode == 3 )
    {
      corner[0] = point[0] - tr_refdef_vieworgX;
      corner[1] = point[1] - tr_refdef_vieworgY;
      corner[2] = point[2] - tr_refdef_vieworgZ;
      VectorNormalizeFast(corner);
      if ( *(float *)&tr_refdef_viewaxis02 * corner[2]
         + *(float *)&tr_refdef_viewaxis01 * corner[1]
         + *(float *)&tr_refdef_viewaxis00 * corner[0] < 0.94999999f )
        goto blend;
    }
    corner[0] = (float)(32 * (gridX - 4096));
    corner[1] = (float)(32 * (gridY - 4096));
    corner[2] = (float)((gridZ - 2048) << 6);
    cellMax[0] = corner[0] + 32.0f;
    cellMax[1] = corner[1] + 32.0f;
    cellMax[2] = corner[2] + 64.0f;
    R_AddDebugBox( corner, cellMax,colorDebugWhite);
  }

  /* blend the eight surrounding grid corners */
blend:
  totalWeight = 0.0f;
  for ( i = 0; i < 8; ++i )
  {
    int   zbit = (i >> 2) & 1;
    int   ybit = (i >> 1) & 1;
    int   xbit = i & 1;
    int   cornerX = gridX + xbit;
    int   visibility;
    float weight;

    visibility = R_GetCachedVisibility( cornerX, ybit + gridY, zbit + gridZ, leaf->u.leaf.cluster,
                                        numLights, (int)lights, (unsigned int *)point, &sunByte );
    mode = r_showLeafLights->integer;
    if ( mode < 0 )
    {
      /* debug: world position of this corner and a +/-1 box */
      corner[0] = (float)(32 * cornerX) - 131072.0f;
      corner[1] = (float)(16 * ((i & 2) + 2 * gridY)) - 131072.0f;
      corner[2] = (float)(16 * ((i & 4) + 4 * gridZ)) - 131072.0f;
      boxA[0] = corner[0] - 1.0f;
      boxA[1] = corner[1] - 1.0f;
      boxA[2] = corner[2] - 1.0f;
      boxB[0] = corner[0] + 1.0f;
      boxB[1] = corner[1] + 1.0f;
      boxB[2] = corner[2] + 1.0f;
    }

    if ( visibility < 0 )
    {
      if ( mode < 0 )
        R_AddDebugBox( boxA, boxB,colorDebugRed);
    }
    else
    {
      /* retail 0x4B6686: fst weight; fadd total -- the total takes the unrounded
       * product, while the sun term and the per-light scales use the stored float */
#ifdef COD1_VC7
      weight = weights[2 + ybit] * weights[4 + zbit] * weights[0 + xbit];
      totalWeight = weight + totalWeight;
      *diffuseSun += sunByte * weight * tr_diffuseSunSampleScale;
#else
      double product = (double)weights[2 + ybit] * weights[4 + zbit] * weights[0 + xbit];
      weight = (float)product;
      totalWeight = (float)(totalWeight + product);
      *diffuseSun += (double)sunByte * weight * tr_diffuseSunSampleScale;
#endif

      if ( !visibility )
      {
        if ( r_showLeafLights->integer < 0 )
          R_AddDebugBox( boxA, boxB,colorDebugBlue);
      }
      else
      {
        /* distribute the corner weight to each visible light */
        for ( k = 0; k < numLights; ++k )
        {
          if ( (1 << k) & visibility )
          {
            scales[k] += weight;
            if ( r_showLeafLights->integer < 0 )
            {
              dlight_t     *light = lights[k];
              unsigned int *color;
              float        *endpoint;

              if ( light->position[3] == 0.0f )
              {
                color = colorDebugYellow;
                probe[0] = light->position[0] * 32768.0f + corner[0];
                probe[1] = light->position[1] * 32768.0f + corner[1];
                probe[2] = light->position[2] * 32768.0f + corner[2];
                endpoint = probe;
              }
              else
              {
                endpoint = light->position;
                color = colorDebugGreen;
              }
              R_AddDebugLine(corner, endpoint, color);
            }
          }
        }
      }
    }
  }

  /* normalise weights / diffuse sun, or fall back to full lighting */
  if ( totalWeight < 0.98000002f )
  {
    if ( totalWeight == 0.0f )
    {
      if ( leaf->u.leaf.cluster < 0 )
      {
        for ( k = 0; k < numLights; ++k )
          scales[k] = 1.0f;
        *diffuseSun = 1.0f;
      }
    }
    else
    {
      float invWeight = 1.0f / totalWeight;
      if ( *diffuseSun != 0.0f )
        *diffuseSun = invWeight * *diffuseSun;
      for ( k = 0; k < numLights; ++k )
      {
        if ( scales[k] != 0.0f )
        {
          if ( lights[k] == (dlight_t *)*(unsigned int *)(tr_world + 276) )
            scales[k] = 1.0f - totalWeight + scales[k];    /* sun light: keep the uncovered remainder */
          else
            scales[k] = invWeight * scales[k];
        }
      }
    }
  }

  if ( leaf->u.leaf.hasSunLight )
  {
    if ( *diffuseSun < 0.25f )
      *diffuseSun = 0.25f;
  }
  return numLights;
}


/* ---- R_PickFinalLights  0x004B69F0 ----  VERIFIED
 * Build the entity's final picked-light set.  Appends any in-range dynamic
 * lights from lightGroup, injects one or two synthetic sun lights (generatedLights[1..2])
 * when the diffuse sun is active, ranks every candidate by attenuated intensity
 * (keeping the strongest 8 in a sorted list), copies them into entity->lights[],
 * records the diffuse-sun contribution, and optionally merges the overflow.
 * lightScales is the per-light scale array, lightPtrs the parallel light-pointer array, diffuseSun the
 * diffuse-sun fraction; numLights (the candidate count) and diffuseSun are updated in place.
 * The dynamic-light range test is retail's DistanceSquared (float deltas; the
 * modern build keeps the unrounded sum), and the comparisons keep retail's NaN
 * polarity.  Raw offsets for the blobs. */
void __cdecl R_PickFinalLights(int entity, int lightGroup, float *point, int numLights,
                               float *lightScales, float diffuseSun, dlight_t **lightPtrs, int doMerge)
{
  dlight_t   *generated = ((trRefEntity_t *)entity)->generatedLights;
  int         numSorted;
  int         k;
  int         insertAt;
  int         j;
  float       intensity;
  int         sortedIdx[8];      /* v50: top-8 light indices, intensity-descending */
  float       intensities[49];   /* v51: per-candidate attenuated intensity */

  ((trRefEntity_t *)entity)->hasDynamicLights = 0;

  /* append the supplied in-range dynamic lights (within 2x radius) */
  if ( lightGroup )
  {
    for ( k = 0; k < *(int *)(lightGroup + 352); ++k )
    {
      dlight_t *dl = &((dlight_t *)*(unsigned int *)(lightGroup + 356))[k];
#ifdef COD1_VC7
      if ( DistanceSquared(dl->position, point) > dl->radius * dl->radius * 4.0f )
        continue;
#else
      /* modern build: retail never stores the sum of squares (0x4B6A87-0x4B6AB1:
       * fmul/faddp straight into fcompp), so keep it unrounded */
      float  dx = point[0] - dl->position[0];
      float  dy = point[1] - dl->position[1];
      float  dz = point[2] - dl->position[2];
      double distSq = (double)dz * dz + (double)dy * dy + (double)dx * dx;

      if ( distSq > dl->radius * dl->radius * 4.0 )
        continue;
#endif
      ((dlight_t **)lightPtrs)[numLights] = dl;
      ((float *)lightScales)[numLights] = 1.0f;
      ++numLights;
      ((trRefEntity_t *)entity)->hasDynamicLights = 1;
    }
  }

  /* inject synthetic sun light(s) for the diffuse-sun term */
  if ( diffuseSun != 0.0f && *(float *)(tr_world + 264) != 0.0f && tr_diffuseSunQuality )
  {
    if ( tr_diffuseSunQuality != 1 && numLights <= tr_maxEntityLights )
    {
      /* soft sun: a bright up light plus a negative down fill */
      generated[1].type       = 8;
      generated[1].color[0]   = *(float *)(tr_world + 248);
      generated[1].color[1]   = *(float *)(tr_world + 252);
      generated[1].color[2]   = *(float *)(tr_world + 256);
      generated[1].intensity  = *(float *)(tr_world + 260);      /* dead: overwritten */
      generated[1].intensity  = *(float *)(tr_world + 264);
      generated[1].ambient[0] = *(float *)(tr_world + 248) * 0.75f;
      generated[1].ambient[1] = *(float *)(tr_world + 252) * 0.75f;
      generated[1].ambient[2] = *(float *)(tr_world + 256) * 0.75f;
      generated[1].ambient[3] = *(float *)(tr_world + 260) * 0.75f;
      generated[1].diffuse[0] = *(float *)(tr_world + 248) * 0.25f;
      generated[1].diffuse[1] = *(float *)(tr_world + 252) * 0.25f;
      generated[1].diffuse[2] = *(float *)(tr_world + 256) * 0.25f;
      generated[1].diffuse[3] = *(float *)(tr_world + 260) * 0.25f;
      generated[1].specular[0] = 0.0f;
      generated[1].specular[1] = 0.0f;
      generated[1].specular[2] = 0.0f;
      generated[1].specular[3] = 0.0f;
      generated[1].position[0] = 0.0f;
      generated[1].position[1] = 0.0f;
      generated[1].position[2] = 1.0f;
      generated[1].position[3] = 0.0f;
      generated[1].spotCutoff  = 180.0f;

      ((float *)lightScales)[numLights] = diffuseSun;
      ((dlight_t **)lightPtrs)[numLights] = &generated[1];
      ++numLights;

      generated[2].color[0]   = generated[1].color[0];
      generated[2].color[1]   = generated[1].color[1];
      generated[2].type       = 8;
      generated[2].color[2]   = generated[1].color[2];
      generated[2].intensity  = *(float *)(tr_world + 264) * -0.25f;
      generated[2].ambient[0] = 0.0f;
      generated[2].ambient[1] = 0.0f;
      generated[2].ambient[2] = 0.0f;
      generated[2].ambient[3] = 0.0f;
      generated[2].diffuse[0] = *(float *)(tr_world + 248) * -0.25f;
      generated[2].diffuse[1] = *(float *)(tr_world + 252) * -0.25f;
      generated[2].diffuse[2] = *(float *)(tr_world + 256) * -0.25f;
      generated[2].diffuse[3] = *(float *)(tr_world + 260) * -0.25f;
      generated[2].specular[0] = 0.0f;
      generated[2].specular[1] = 0.0f;
      generated[2].specular[2] = 0.0f;
      generated[2].specular[3] = 0.0f;
      generated[2].position[0] = 0.0f;
      generated[2].position[1] = 0.0f;
      generated[2].position[2] = -1.0f;
      generated[2].position[3] = 0.0f;
      generated[2].spotCutoff  = 180.0f;

      ((float *)lightScales)[numLights] = diffuseSun;
      ((dlight_t **)lightPtrs)[numLights] = &generated[2];
      ++numLights;
    }
    else
    {
      /* single sun: one soft up-facing directional light */
      generated[1].type       = 8;
      generated[1].color[0]   = *(float *)(tr_world + 248);
      generated[1].color[1]   = *(float *)(tr_world + 252);
      generated[1].color[2]   = *(float *)(tr_world + 256);
      generated[1].intensity  = *(float *)(tr_world + 260);      /* dead: overwritten */
      generated[1].intensity  = *(float *)(tr_world + 264);
      generated[1].ambient[0] = *(float *)(tr_world + 248) * 0.5f;
      generated[1].ambient[1] = *(float *)(tr_world + 252) * 0.5f;
      generated[1].ambient[2] = *(float *)(tr_world + 256) * 0.5f;
      generated[1].ambient[3] = *(float *)(tr_world + 260) * 0.5f;
      generated[1].diffuse[0] = *(float *)(tr_world + 248) * 0.5f;
      generated[1].diffuse[1] = *(float *)(tr_world + 252) * 0.5f;
      generated[1].diffuse[2] = *(float *)(tr_world + 256) * 0.5f;
      generated[1].diffuse[3] = *(float *)(tr_world + 260) * 0.5f;
      generated[1].specular[0] = 0.0f;
      generated[1].specular[1] = 0.0f;
      generated[1].specular[2] = 0.0f;
      generated[1].specular[3] = 0.0f;
      generated[1].position[0] = 0.0f;
      generated[1].position[1] = 0.0f;
      generated[1].position[2] = 1.0f;
      generated[1].position[3] = 0.0f;
      generated[1].spotCutoff  = 180.0f;

      ((float *)lightScales)[numLights] = diffuseSun;
      ((dlight_t **)lightPtrs)[numLights] = &generated[1];
      ++numLights;
    }
    diffuseSun = 0.0f;
  }

  /* rank candidates by attenuated intensity; keep the strongest 8 */
  numSorted = 0;
  for ( k = 0; k < numLights; ++k )
  {
    if ( ((float *)lightScales)[k] == 0.0f )
      continue;

    if ( ((dlight_t **)lightPtrs)[k]->intensity >= 0.0f )
    {
      intensity = R_MaxLightIntensity(((dlight_t **)lightPtrs)[k], point) * ((float *)lightScales)[k];
      intensities[k] = intensity;
      if ( ((dlight_t **)lightPtrs)[k]->type == 1 || ((dlight_t **)lightPtrs)[k]->type == 8 )
        intensities[k] = (float)(1 << tr_overbrightBits) * *(float *)(tr_world + 264) * diffuseSun + intensity;
      if ( intensities[k] < r_minEntLightIntensity->value )
        continue;
    }
    else
    {
      intensities[k] = 1.0e19f;                 /* always-keep sentinel (world sun) */
    }

    for ( insertAt = 0; insertAt < numSorted; ++insertAt )
    {
      if ( !(intensities[sortedIdx[insertAt]] > intensities[k]) )
        break;
    }

    if ( insertAt < 8 )
    {
      if ( numSorted == 8 )
        numSorted = 7;
      for ( j = numSorted; j > insertAt; --j )
        sortedIdx[j] = sortedIdx[j - 1];
      sortedIdx[insertAt] = k;
      ++numSorted;
    }
  }

  /* publish the picked lights on the entity */
  ((trRefEntity_t *)entity)->lightCount = numSorted;
  for ( k = 0; k < numSorted; ++k )
  {
    ((trRefEntity_t *)entity)->lights[k].scale = ((float *)lightScales)[sortedIdx[k]];
    ((trRefEntity_t *)entity)->lights[k].light = ((dlight_t **)lightPtrs)[sortedIdx[k]];
  }

  ((trRefEntity_t *)entity)->diffuseSun = diffuseSun;
  if ( r_showLeafLights->integer > 0 )
    R_ShowLeafLights(point, entity);
  if ( doMerge )
    R_MergeLights(entity, point, intensities, (unsigned int *)sortedIdx);
}


/* ---- R_PickLights  0x004B7290 ----  VERIFIED
 * Entry point for entity lighting: gather this point's static light
 * contributions, then rank/merge them into the entity's final light set.
 * Fullbright or an unloaded world zeroes the light count.  point is the first
 * parameter: the caller sets up entity (eax) before point (esi), as in retail. */
void __cdecl R_PickLights(float *point, int entity, int lightGroup, int doMerge)
{
  int       numStatic;
  float     diffuseSun;
  dlight_t *lightPtrArray[49];
  float     scaleArray[49];      /* 49 entries each: retail's 0x198-byte frame (and the Mac build's 196-byte array) */

  if ( tr_world && *(unsigned int *)(tr_world + 152) && !r_entFullbright->integer )
  {
    numStatic = R_GetStaticLightContributions(point, (char *)scaleArray, &diffuseSun, (char *)lightPtrArray);
    R_PickFinalLights( entity, lightGroup, point, numStatic,
                       scaleArray, diffuseSun, lightPtrArray, doMerge );
  }
  else
  {
    ((trRefEntity_t *)entity)->lightCount = 0;
  }
}


/* ---- R_SetupEntityLighting  0x004B7320 ----  VERIFIED
 * Compute an entity's lighting once per frame: pick its lights at the lighting
 * origin (e.origin, or e.lightingOrigin when renderfx requests it), then, for
 * planar shadows, project the sun direction into the entity's local frame. */
void __cdecl R_SetupEntityLighting(int entity, int lightGroup)
{
  float point[3];
  float inverseAxis[9];

  if ( ((trRefEntity_t *)entity)->lightingCalculated )
    return;

  ((trRefEntity_t *)entity)->lightingCalculated = 1;
  if ( *(byte *)(entity + 4) & 0x80 )    /* renderfx: use e.lightingOrigin */
  {
    point[0] = ((trRefEntity_t *)entity)->e.lightingOrigin[0];
    point[1] = ((trRefEntity_t *)entity)->e.lightingOrigin[1];
    point[2] = ((trRefEntity_t *)entity)->e.lightingOrigin[2];
  }
  else                                     /* use e.origin */
  {
    point[0] = ((trRefEntity_t *)entity)->e.origin[0];
    point[1] = ((trRefEntity_t *)entity)->e.origin[1];
    point[2] = ((trRefEntity_t *)entity)->e.origin[2];
  }

  R_PickLights(point, entity, lightGroup, 1);

  if ( cg_shadows->integer == 2 )
  {
    MatrixInverse(((trRefEntity_t *)entity)->e.axis[0], inverseAxis);
    ((trRefEntity_t *)entity)->lightDir[0]     = tr_sunDirection[0] * inverseAxis[0];
    ((trRefEntity_t *)entity)->lightDir[1] = tr_sunDirection[0] * inverseAxis[1];
    ((trRefEntity_t *)entity)->lightDir[2] = tr_sunDirection[0] * inverseAxis[2];
    ((trRefEntity_t *)entity)->lightDir[0]     += tr_sunDirection[1] * inverseAxis[3];
    ((trRefEntity_t *)entity)->lightDir[1] += tr_sunDirection[1] * inverseAxis[4];
    ((trRefEntity_t *)entity)->lightDir[2] += tr_sunDirection[1] * inverseAxis[5];
    ((trRefEntity_t *)entity)->lightDir[0]     += tr_sunDirection[2] * inverseAxis[6];
    ((trRefEntity_t *)entity)->lightDir[1] += tr_sunDirection[2] * inverseAxis[7];
    ((trRefEntity_t *)entity)->lightDir[2] += tr_sunDirection[2] * inverseAxis[8];
  }
}


/* ---- R_SetupStaticModelLighting  0x004B7450 ----  VERIFIED
 * Like R_SetupEntityLighting but for static models, which carry a precomputed
 * light set (entity->staticModelLighting): pick directly from that set at the
 * model's lighting origin.  The r_showLeafLights debug path delegates to the
 * regular entity path. */
void __cdecl R_SetupStaticModelLighting(int lightGroup, int entity)
{
  int   lighting;              /* entity->staticModelLighting */
  float inverseAxis[9];

  if ( r_showLeafLights->integer < 0 )
  {
    R_SetupEntityLighting(entity, lightGroup);
    return;
  }

  if ( !((trRefEntity_t *)entity)->lightingCalculated )
  {
    lighting = (int)((trRefEntity_t *)entity)->staticModelLighting;
    ((trRefEntity_t *)entity)->lightingCalculated = 1;
    R_PickFinalLights( entity, lightGroup, ((trRefEntity_t *)entity)->e.lightingOrigin, *(int *)(lighting + 184),
                       (float *)(lighting + 192), *(float *)(lighting + 188), (dlight_t **)(lighting + 388), 1 );
    if ( cg_shadows->integer == 2 )
    {
      MatrixInverse(((trRefEntity_t *)entity)->e.axis[0], inverseAxis);
      ((trRefEntity_t *)entity)->lightDir[0]     = tr_sunDirection[0] * inverseAxis[0];
      ((trRefEntity_t *)entity)->lightDir[1] = tr_sunDirection[0] * inverseAxis[1];
      ((trRefEntity_t *)entity)->lightDir[2] = tr_sunDirection[0] * inverseAxis[2];
      ((trRefEntity_t *)entity)->lightDir[0]     += tr_sunDirection[1] * inverseAxis[3];
      ((trRefEntity_t *)entity)->lightDir[1] += tr_sunDirection[1] * inverseAxis[4];
      ((trRefEntity_t *)entity)->lightDir[2] += tr_sunDirection[1] * inverseAxis[5];
      ((trRefEntity_t *)entity)->lightDir[0]     += tr_sunDirection[2] * inverseAxis[6];
      ((trRefEntity_t *)entity)->lightDir[1] += tr_sunDirection[2] * inverseAxis[7];
      ((trRefEntity_t *)entity)->lightDir[2] += tr_sunDirection[2] * inverseAxis[8];
    }
  }
}


/* ---- R_VC_stats_f  0x004B7590 ----  VERIFIED */
void R_VC_stats_f()
{
  ri.Printf(0, "light visibility cache performance:\n");
  ri.Printf(0, "%i entries used (%.1f%%)\n", lightVisCache_entriesUsed, (float)lightVisCache_entriesUsed * 0.0003814697265625f);
  ri.Printf(0, "%i max associativity\n", lightVisCache_maxAssociativity);
  ri.Printf(0, "%i entries flushed\n", lightVisCache_entriesFlushed);
  ri.Printf(0, "%i entries filled in at runtime instead of read from disk\n", lightVisCache_entriesFilledAtRuntime);
}

/* ---- R_LightVisHistoryFilename  0x004B7600 ----  VERIFIED
 * Build "<mapname>.vclog" from the loaded world's base name (up to the first
 * dot). */
void __cdecl R_LightVisHistoryFilename(char *out)
{
  const char *name = (const char *)tr_world;
  char       *dst = out;

  while ( *name && *name != '.' )
    *dst++ = *name++;
  *dst = 0;

  if ( strlen(out) + 6 >= 0x40 )
    ri.Error(1, "\x15" "light vis cache log filename '%s.vclog' is too long\n", out);
  strcat(out, ".vclog");
}

/* ---- R_InitLightVisHistory  0x004B7680 ----  VERIFIED
 * Allocate the light-vis history buffers.  When r_vc_makelog == 2, replay a
 * previously recorded ".vclog" file, re-caching the visibility of each recorded
 * grid point. */
void R_InitLightVisHistory()
{
  const void *fileData;
  char        sunByte;
  unsigned int      lightPtrs[15];
  char        filename[64];
  int         size;
  int         count;
  int         i;
  int         j;
  int         cluster;
  int         numLights;
  const int  *rec;
  mnode_t    *leaf;
  int         lights;
  const __int16 *lightIndex;

  lightVisHist.count = 0;
  lightVisHist.sortedCount = 0;
  if ( r_vc_makelog->integer )
  {
    lightVisHist.history = Z_MallocInternal( 0x6000000u );
    lightVisHist.sorted  = Z_MallocInternal( 0x2000000u );
    if ( r_vc_makelog->integer == 2 )
    {
      R_LightVisHistoryFilename( filename );
      size = ri.FS_ReadFile( filename, &fileData );
      if ( !( (unsigned int)size % 0x18 ) )
      {
        if ( (unsigned int)size > 0x6000000 )
          size = 0x6000000;
        memcpy( lightVisHist.history, fileData, size );
        count = (unsigned int)size / 0x18;
        for ( i = 0; i < count; i++ )
        {
          leaf = (mnode_t *)R_PointInLeaf( (float *)((const char *)fileData + 24 * i + 12) );
          cluster = leaf->u.leaf.cluster;
          if ( cluster >= 0 )
          {
            numLights = leaf->u.leaf.lightCount;
            j = 0;
            if ( numLights > 0 )
            {
              lights     = *(unsigned int *)( tr_world + 272 );
              lightIndex = (const __int16 *)*(unsigned int *)( tr_world + 284 ) + leaf->u.leaf.firstLightIndex;
              do
                lightPtrs[j++] = lights + 136 * *lightIndex++;
              while ( j < numLights );
            }
            rec = (const int *)( (const char *)fileData + 24 * i );
            R_GetCachedVisibility( rec[0], rec[1], rec[2], cluster, numLights, (int)lightPtrs, (unsigned int *)&rec[3], &sunByte );
          }
        }
      }
      ri.FS_FreeFile( fileData );
    }
  }
  else
  {
    lightVisHist.history = 0;
    lightVisHist.sorted  = 0;
  }
}


/* ---- R_SaveLightVisHistory  0x004B7840 ----  VERIFIED
 * Write the recorded history to "<mapname>.vclog" and release the buffers. */
void R_SaveLightVisHistory()
{
  char filename[64];

  if ( lightVisHist.history && tr_world )
  {
    R_LightVisHistoryFilename(filename);
    ri.FS_WriteFile(filename, lightVisHist.history, 24 * lightVisHist.count);
    free(lightVisHist.history);
    free(lightVisHist.sorted);
    memset(&lightVisHist, 0, sizeof(lightVisHist));
  }
}

/* ---- R_PrecalcLightVisCachePoint  0x004B78D0 ----  [HIGH]
 * Compute and store one 12-byte on-disk cache entry for a grid point: sample its
 * light visibility at diffuseSunSteps=1, then, for a sun-lit leaf, sample the
 * diffuse-sun grid at each step count (2..5) into the trailing bytes.  Points
 * outside the world, already cached, or in a full bucket are skipped.  The
 * on-disk slot is 12 bytes; the temporary sample uses the 8-byte runtime slot. */
void __cdecl R_PrecalcLightVisCachePoint(int x, int y, int z, float *eye, int cache)
{
  mnode_t       *leaf;
  int            cluster;
  int            slot;
  int            j;
  int            numLights;
  int            lights;
  const __int16 *lightIndex;
  int            key;
  int            bucket;
  int            probe;
  int            entry;
  int            sampledLeaf;
  lightVisSlot_t sample;
  float          point[3];
  unsigned int         lightPtrs[15];

  leaf = (mnode_t *)R_PointInLeaf(eye);
  cluster = leaf->u.leaf.cluster;
  if ( cluster < 0 )
    return;

  numLights = leaf->u.leaf.lightCount;
  j = 0;
  if ( numLights > 0 )
  {
    lights     = *(unsigned int *)( tr_world + 272 );
    lightIndex = (const __int16 *)*(unsigned int *)( tr_world + 284 ) + leaf->u.leaf.firstLightIndex;
    do
      lightPtrs[j++] = lights + 136 * *lightIndex++;
    while ( j < numLights );
  }

  R_LightVisHash( x, y, z, cluster, &key, &bucket );

  /* find an existing entry in the bucket, or the first free slot */
  probe = 0;
  slot = cache + 384 * bucket;
  do
  {
    if ( !*(byte *)(slot + 4) )
      goto fill;
    if ( *(unsigned int *)slot == key )
      return;
    ++probe;
    slot += 12;
  }
  while ( probe < 32 );
  return;   /* bucket full */

fill:
  point[0] = (float)(32 * (x - 4096));
  point[1] = (float)(32 * (y - 4096));
  tr_diffuseSunSteps = 1;
  point[2] = (float)((z - 2048) << 6);
  sampledLeaf = R_SampleLightVisibility(point, &sample, leaf->u.leaf.lightCount, (int)lightPtrs, (int)eye);

  entry = cache + 12 * (probe + 32 * bucket);
  *(unsigned int *)entry       = key;
  *(byte *)(entry + 4)  = sample.state;
  *(byte *)(entry + 5)  = sample.sunSamples;
  *(unsigned short *)(entry + 10) = sample.mask;

  if ( leaf->u.leaf.hasSunLight )
  {
    /* tr_diffuseSunSteps is still 1 from the sample above: steps 2..5 */
    while ( ++tr_diffuseSunSteps <= 5 )
    {
      if ( sample.state != 2 )
      {
        sample.sunSamples = 0;
        R_SampleDiffuseSunVisibility(&sample, point, (int)eye, sampledLeaf);
      }
      *(byte *)(entry + 4 + tr_diffuseSunSteps) = sample.sunSamples;
    }
  }
}

/* ---- R_PrecalcLightVisCache  0x004B7AB0 ----  VERIFIED
 * Offline: build the whole light-vis cache lump by replaying the recorded
 * history file point by point, then save it into the BSP. */
void __cdecl R_PrecalcLightVisCache(int checksum)
{
  int   savedSteps;
  int   count;
  int   cache;
  int   i;
  int   fileData;
  char  filename[64];
  int   size;

  if ( !*(unsigned int *)(tr_world + 280) )
    ri.Error(1, "\x15" "cannot precalculate light vis cache; no lights compiled into map '%s'\n", tr_world);

  R_LightVisHistoryFilename(filename);
  size = ri.FS_ReadFile(filename, &fileData);
  if ( size <= 0 )
    ri.Error(1, "\x15" "light vis cache file '%s' is missing or empty\n", filename);
  count = (unsigned int)size / 24;
  if ( (unsigned int)size % 24 )
    ri.Error(1, "\x15" "light vis cache has funny size\n", filename);

  savedSteps = tr_diffuseSunSteps;
  cache = ri.Malloc(3145728);
  for ( i = 0; i < count; ++i )
  {
    const char *rec = (const char *)fileData + 24 * i;
    R_PrecalcLightVisCachePoint( *(unsigned int *)rec, *(unsigned int *)(rec + 4), *(unsigned int *)(rec + 8),
                                 (float *)(rec + 12), cache );
  }
  tr_diffuseSunSteps = savedSteps;

  ri.FS_FreeFile(fileData);
  ri.CM_SaveLump(32, cache, 3145728, checksum);
  ri.Free(cache);
  if ( r_vc_compile->integer == 2 )
    ri.Cmd_ExecuteText(0, "quit");
  ri.Cvar_Set("r_vc_compile", "0");
}


/* ---- R_InitLightVisCacheFromBuffer  0x004B7C00 ----  VERIFIED
 * Load a precompiled cache lump into the runtime cache: unpack each 12-byte
 * on-disk entry into an 8-byte runtime slot, selecting the sun-sample byte for
 * the active diffuseSunSteps, and tally the used slots / max associativity.
 * (Retail unrolled the copy 4x; restored to a single loop.) */
int __cdecl R_InitLightVisCacheFromBuffer(int buffer, int size)
{
  int used = 0;
  int maxAssoc = 0;
  int steps = tr_diffuseSunSteps;
  int n;

  if ( r_vc_makelog->integer || r_vc_compile->integer )
    return 1;
  if ( !buffer || size != 3145728 )
    return 1;

  lightVisCache_entriesUsed = 0;
  lightVisCache_maxAssociativity = 0;
  lightVisCache_entriesFlushed = 0;
  lightVisCache_entriesFilledAtRuntime = 0;

  for ( n = 0; n < 0x40000; n += 4 )
  {
    {
    const char *src = (const char *)(buffer + 12 * (n + 0));
    char       *dst = (char *)&tr_lightVisCache + 8 * (n + 0);

    *(unsigned int *)dst       = *(unsigned int *)src;          /* key */
    *(byte *)(dst + 4)  = src[4];                  /* state */
    *(byte *)(dst + 5)  = src[4 + steps];          /* sun samples for the active step count */
    *(unsigned short *)(dst + 6)  = *(unsigned short *)(src + 10);    /* mask */
    if ( *(byte *)(dst + 4) )
    {
      int assoc = (n + 0) & 0x1F;                         /* slot index within its 32-entry bucket */
      ++used;
      if ( maxAssoc <= assoc )
        maxAssoc = assoc + 1;
    }
    }
    {
    const char *src = (const char *)(buffer + 12 * (n + 1));
    char       *dst = (char *)&tr_lightVisCache + 8 * (n + 1);

    *(unsigned int *)dst       = *(unsigned int *)src;          /* key */
    *(byte *)(dst + 4)  = src[4];                  /* state */
    *(byte *)(dst + 5)  = src[4 + steps];          /* sun samples for the active step count */
    *(unsigned short *)(dst + 6)  = *(unsigned short *)(src + 10);    /* mask */
    if ( *(byte *)(dst + 4) )
    {
      int assoc = (n + 1) & 0x1F;                         /* slot index within its 32-entry bucket */
      ++used;
      if ( maxAssoc <= assoc )
        maxAssoc = assoc + 1;
    }
    }
    {
    const char *src = (const char *)(buffer + 12 * (n + 2));
    char       *dst = (char *)&tr_lightVisCache + 8 * (n + 2);

    *(unsigned int *)dst       = *(unsigned int *)src;          /* key */
    *(byte *)(dst + 4)  = src[4];                  /* state */
    *(byte *)(dst + 5)  = src[4 + steps];          /* sun samples for the active step count */
    *(unsigned short *)(dst + 6)  = *(unsigned short *)(src + 10);    /* mask */
    if ( *(byte *)(dst + 4) )
    {
      int assoc = (n + 2) & 0x1F;                         /* slot index within its 32-entry bucket */
      ++used;
      if ( maxAssoc <= assoc )
        maxAssoc = assoc + 1;
    }
    }
    {
    const char *src = (const char *)(buffer + 12 * (n + 3));
    char       *dst = (char *)&tr_lightVisCache + 8 * (n + 3);

    *(unsigned int *)dst       = *(unsigned int *)src;          /* key */
    *(byte *)(dst + 4)  = src[4];                  /* state */
    *(byte *)(dst + 5)  = src[4 + steps];          /* sun samples for the active step count */
    *(unsigned short *)(dst + 6)  = *(unsigned short *)(src + 10);    /* mask */
    if ( *(byte *)(dst + 4) )
    {
      int assoc = (n + 3) & 0x1F;                         /* slot index within its 32-entry bucket */
      ++used;
      if ( maxAssoc <= assoc )
        maxAssoc = assoc + 1;
    }
    }
  }

  lightVisCache_entriesUsed = used;
  lightVisCache_maxAssociativity = maxAssoc;
  return 1;
}


/* ---- R_AddSortedHistoryEntry  0x004B7E10 ----  VERIFIED
 * Insert a {(x, y) key, z} pair into the sorted history array at `index`,
 * shifting the tail up.  Returns 0 when the array is full, 1 on success. */
int __cdecl R_AddSortedHistoryEntry(unsigned int *entry, int index)
{
  if ( lightVisHist.sortedCount >= 0x400000 )       /* 0x004B7E15: cmp eax, 400000h */
    return 0;
  memmove( (char *)lightVisHist.sorted + 8 * index + 8,
           (char *)lightVisHist.sorted + 8 * index,
           8 * (lightVisHist.sortedCount - index) );
  *((unsigned int *)lightVisHist.sorted + 2 * index)     = entry[0];
  *((unsigned int *)lightVisHist.sorted + 2 * index + 1) = entry[1];
  ++lightVisHist.sortedCount;
  return 1;
}

/* one 8-byte sorted-history record: the packed (x, y) grid key, then z */
typedef struct {
	union {
		struct {
			short x;              /* +0x00 */
			short y;              /* +0x02 */
		} pos;
		int   key;              /* +0x00  (x, y) compared as one dword */
	} u;
	unsigned short  z;          /* +0x04 */
} lightVisSortedEntry_t;
COD1_ASSERT_SIZE(lightVisSortedEntry_t, 0x08);

/* ---- R_SortedHistoryEntry  0x004B7E70 ----  VERIFIED
 * Binary-search the sorted history for the {(x,y) key, z} entry.  insertMode controls
 * insertion on a miss: 1 inserts and returns the new slot, 2 inserts on a hit as
 * well.  Returns the found/inserted index, or -1.  The record's pad word (+6) is
 * left unwritten, as in retail. */
int __cdecl R_SortedHistoryEntry(int z, int x, int y, int insertMode)
{
  lightVisSortedEntry_t  entry;
  lightVisSortedEntry_t *sorted;
  int                    lo;
  int                    hi;
  int                    mid;
  int                    cmp;

  entry.u.pos.x = (short)x;
  entry.u.pos.y = (short)y;
  entry.z = (unsigned short)z;

  hi = lightVisHist.sortedCount - 1;
  lo = 0;
  while ( lo <= hi )
  {
    mid = (lo + hi) >> 1;
    sorted = (lightVisSortedEntry_t *)lightVisHist.sorted;
    cmp = entry.u.key - sorted[mid].u.key;
    if ( !cmp )
    {
      cmp = (unsigned short)z - sorted[mid].z;
      if ( !cmp )
      {
        if ( insertMode == 2 )
          R_AddSortedHistoryEntry((unsigned int *)&entry, mid);
        return mid;
      }
    }
    if ( cmp < 0 )
      hi = mid - 1;
    else
      lo = mid + 1;
  }

  if ( insertMode && R_AddSortedHistoryEntry((unsigned int *)&entry, lo) )
    return lo;
  return -1;
}

/* ---- R_ShowLightVisCachePoints  0x004B7F20 ----  VERIFIED
 * Debug: scatter a "." at every recorded-but-uncached grid point in an
 * r_vc_showlog-sized neighbourhood around the view origin, coloured by the Z
 * layer. */
int __cdecl FastRound( float f );

void R_ShowLightVisCachePoints()
{
  int           radius;
  int           centerX;
  int           centerY;
  int           centerZ;
  int           dz;
  int           dy;
  int           dx;
  unsigned int *color;
  float         p[3];

  if ( !lightVisHist.history )
    return;

  radius = r_vc_showlog->integer;
  if ( radius <= 0 )
    return;

  /* origin + 131072 is rounded to float first (0x4B7F56 fsub flt_5690FC,
   * fstp dword), then converted by fld / fadd|fsub bias / fistp under the
   * default round-to-nearest control word (an (int) cast would truncate through
   * __ftol2): X/Y round to nearest (the 2^-30 bias breaks ties upward), Z takes
   * the floor (bias 0.5-2^-30).  Retail does this inline, one f/bias/i set. */
  centerX = FastRound(tr_viewParms_originX - -131072.0f) >> 5;
  centerY = FastRound(tr_viewParms_originY - -131072.0f) >> 5;
  centerZ = FastFloor(tr_viewParms_originZ - -131072.0f) >> 6;

  p[0] = (float)(32 * (centerX - 4096));
  p[1] = (float)(32 * (centerY - 4096));
  p[2] = (float)((centerZ - 2048) << 6);

  for ( dz = -1; dz <= 1; ++dz )
  {
    int gz = centerZ + dz;

    if ( gz < 0 || (gz << 6) > 0x80000 )
      continue;
    for ( dy = -radius; dy <= radius; ++dy )
    {
      int y = centerY + dy;

      if ( y < 0 || (y << 5) > 0x80000 )
        continue;
      for ( dx = -radius; dx <= radius; ++dx )
      {
        int x = centerX + dx;

        if ( x < 0 || (x << 5) > 0x80000 )
          continue;
        if ( R_SortedHistoryEntry(gz, x, y, 0) >= 0 )
          continue;

        p[0] = (float)(32 * (x - 4096));
        p[1] = (float)(32 * (y - 4096));
        p[2] = (float)((gz - 2048) << 6);
        if ( R_CullPointAndRadius(p, 0.0f) )
          continue;

        color = ( dz == 0 || dz == 1 ) ? colorDebugGreen : colorDebugYellow;
        R_AddDebugString((int *)p, color, 1065353216, ".");
      }
    }
  }
}
