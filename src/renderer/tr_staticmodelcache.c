/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_staticmodelcache.c -- static-model surface vertex cache.
 *
 * CoD1 caches the transformed, lit vertices of static (non-animated) model
 * surfaces in one 1.5 MB block (65536 verts * 24 B) so a static model draws
 * from a prebuilt vertex buffer instead of re-transforming every frame.  This
 * is CoD-specific; RTCW has no analog.
 *
 * The cache (r_smcCache) is a buddy allocator: 128 pages of 512 verts, each
 * page a 31-node binary tree over 16 surface slots.  A request for N verts
 * takes the smallest power-of-two block (>= 32) that fits; SMC_Allocate splits
 * larger free blocks down through the size-class free lists, and
 * SMC_FreeCachedSurface_r / SMC_ForceFreeBlock recombine them.  Whole pages are
 * threaded on an LRU list so the least-recently-used page is evicted first.
 * smcCache_t / smcPage_t / smcSurface_t / smcNode_t / smcLink_t come from
 * tr_smcache.h; the intrusive list links there are typed int, so the traversal
 * code casts them to smcLink_t* where it walks a list.
 *
 * The source-surface, cached-vertex, owner-model and shader-stage records that
 * are passed by pointer are defined file-locally below (proven against retail
 * CoDMP.exe + CoDUO, pinned with size/offset asserts); tr_local.h is not
 * touched.  Global scalars stay on the tr_local.h symbols.
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_smcache.h"
#include "tr_local.h"

extern int Hunk_AllocAlignInternal();
extern int R_CreateBufferARB();

smcCache_t r_smcCache;

/* ------------------------------------------------------------------------- *
 *  File-local constants.
 * ------------------------------------------------------------------------- */
#define SMC_STORAGE_BYTES   1572864   /* 65536 verts * 24 B: whole cache block */

#define SMC_MAX_SIZE_SHIFT       9    /* 512-vertex whole-page block */
#define SMC_MIN_SIZE_SHIFT       5    /* 32-vertex smallest block */
#define SMC_SIZE_CLASS_COUNT     5    /* freeLists[0..4] */
#define SMC_SURFACES_PER_PAGE   16
#define SMC_PAGE_VERTEX_COUNT  512

/* tr.cachedStaticModelSurfaceType -- selects the backend upload path used in
 * R_CacheStaticModelSurface (CoDUO renderer_surface_type_t tail). */
#define SF_CACHED_STATIC_MODEL_GENERIC  12
#define SF_CACHED_STATIC_MODEL_ARB      13
#define SF_CACHED_STATIC_MODEL_ATI      14
#define SF_CACHED_STATIC_MODEL_NV       15

/* GL enums (not declared in this tree's gl headers) */
#define GL_ARRAY_BUFFER_ARB   34962    /* 0x8892 */
#define GL_PRESERVE_ATI       34658    /* 0x8762 */


/* ------------------------------------------------------------------------- *
 *  Shader-stage color/alpha generators (values proven by the
 *  R_CacheStaticModelSurface switch; names from CoDUO).
 * ------------------------------------------------------------------------- */
typedef enum {
	CGEN_IDENTITY_LIGHTING = 1,    /* every channel = tr.identityLightByte */
	CGEN_IDENTITY          = 2,    /* white (255) */
	CGEN_LIGHTING_DIFFUSE  = 10,   /* per-vertex world lighting */
	CGEN_LIGHTING_PRECALC  = 11,   /* entity->e.shaderRGBA */
	CGEN_CONSTANT          = 12    /* stage->constantColor */
} colorGen_t;

typedef enum {
	AGEN_UNSPECIFIED = 0,          /* force opaque unless rgbGen is CGEN_IDENTITY */
	AGEN_CONSTANT    = 9           /* stage->constantColor[3] */
} alphaGen_t;

/* ------------------------------------------------------------------------- *
 *  File-local typed views over the pointer-passed records.
 * ------------------------------------------------------------------------- */

/* one cached vertex written into the storage block (24 bytes) */
typedef struct {
	float texCoord[2];        /* +0x00 */
	byte  color[4];           /* +0x08  RGBA */
	float position[3];        /* +0x0c  world space */
} smcVertex_t;
COD1_ASSERT_SIZE(smcVertex_t, 0x18);
ASSERT_OFF(smcVertex_t, color, 0x08);
ASSERT_OFF(smcVertex_t, position, 0x0c);

/* the source static-model surface being cached (only the touched fields are
 * named; the retail record is larger). */
typedef struct {
	int             surfaceType;         /* +0x00  (tr_staticmodel.c smSurface_t) */
	int             storageMode;         /* +0x04 */
	shader_t       *shader;              /* +0x08 */
	shader_t       *cachedShader;        /* +0x0c  stages[0] selects color/alpha gen */
	unsigned short  indexCount;          /* +0x10 */
	unsigned short  vertexCount;         /* +0x12 */
	float         (*texCoords)[2];       /* +0x14 */
	vec3_t         *normals;             /* +0x18 */
	vec3_t         *vertices;            /* +0x1c  model space */
} smSurface_t;
ASSERT_OFF(smSurface_t, cachedShader, 0x0c);
ASSERT_OFF(smSurface_t, vertexCount, 0x12);
ASSERT_OFF(smSurface_t, texCoords, 0x14);
ASSERT_OFF(smSurface_t, normals, 0x18);
ASSERT_OFF(smSurface_t, vertices, 0x1c);

/* the owning static model (renderer_static_model_t).  Its entity's axis and
 * origin transform the source verts; surfaceLightingCache[surfaceIndex] holds
 * the back-pointer to the cached surface and is cleared on free. */
typedef struct {
	refEntity_t  entity;                     /* +0x000  axis @0x1c, origin @0x44 */
	vec3_t       mins;                       /* +0x09c  (tr_staticmodel.c renderer_static_model_t) */
	vec3_t       maxs;                       /* +0x0a8 */
	int          viewCount;                  /* +0x0b4 */
	int          lightContributions;         /* +0x0b8 */
	byte         lighting[0x248 - 0xbc];     /* +0x0bc  static-light index / colour tables */
	void        *surfaceLightingCache[1];    /* +0x248  [surfaceIndex] (variable length) */
} renderer_static_model_t;
ASSERT_OFF(renderer_static_model_t, surfaceLightingCache, 0x248);

/* source->cachedShader->stages[0] -- only the color/alpha generation selectors
 * are read.  The stage pointer array is at shader+0x154 (inside tr_local.h's
 * shader_t reserved region); rgbGen/alphaGen/constantColor sit deep in the
 * per-stage record.  Offsets proven against retail CoDMP.exe. */
typedef struct {
	unsigned int flags;           /* +0x000 */
	byte  bundles[0x644 - 0x004]; /* +0x004  texture bundles (interior disputed) */
	int   atiFragmentShader;     /* +0x644 */
	void *registerCombiners;     /* +0x648 */
	char *vertexProgram;         /* +0x64c */
	byte  rgbWave[0x14];         /* +0x650  waveForm_t */
	int   rgbGen;                /* +0x664  colorGen_t */
	byte  alphaWave[0x14];       /* +0x668  waveForm_t */
	int   alphaGen;              /* +0x67c  alphaGen_t */
	byte  constantColor[4];      /* +0x680  RGBA */
} shaderStage_t;
ASSERT_OFF(shaderStage_t, rgbGen, 0x664);
ASSERT_OFF(shaderStage_t, alphaGen, 0x67c);
ASSERT_OFF(shaderStage_t, constantColor, 0x680);

#define R_ShaderStage0(sh)   ((shaderStage_t *)((shader_t *)(sh))->stages[0])


/* ---- SMC_FreeCachedSurface_r  0x004E4EE0 ----  VERIFIED
 * Recursively release a buddy-tree node: an unused node just unlinks its free
 * block, an interior node recurses into both children, and an allocated leaf
 * releases the cached surface it holds (clearing the owner's back-pointer and
 * the cache vertex counters).  VC7 turns the right-child recursion into
 * retail's loop. */
void __cdecl SMC_FreeCachedSurface_r(smcCache_t *cache, smcPage_t *page, int nodeIndex, int depth)
{
  smcSurface_t *surface;

  if ( !page->tree[nodeIndex].usedVertexCount )
  {
    /* free block: unlink it from its size-class free list */
    surface = &page->surfaces[((nodeIndex + 1) << depth) - SMC_SURFACES_PER_PAGE];
    ((smcLink_t *)surface->freeLink.prev)->next = surface->freeLink.next;
    ((smcLink_t *)surface->freeLink.next)->prev = surface->freeLink.prev;
    return;
  }

  page->tree[nodeIndex].usedVertexCount = 0;
  if ( !page->tree[nodeIndex].allocated )
  {
    /* interior node: free both children */
    SMC_FreeCachedSurface_r(cache, page, 2 * nodeIndex + 1, depth - 1);
    SMC_FreeCachedSurface_r(cache, page, 2 * nodeIndex + 2, depth - 1);
    return;
  }

  /* allocated leaf: release the cached surface it holds */
  surface = &page->surfaces[((nodeIndex + 1) << depth) - SMC_SURFACES_PER_PAGE];
  ((renderer_static_model_t *)surface->cached.owner)->surfaceLightingCache[surface->cached.surfaceIndex] = 0;
  cache->allocatedVertexCount -= 1 << (depth + SMC_MIN_SIZE_SHIFT);
  cache->usedVertexCount -= ((smSurface_t *)surface->cached.source)->vertexCount;
  page->tree[nodeIndex].allocated = 0;
}

/* ---- SMC_ForceFreeBlock  0x004E4FA0 ----  VERIFIED
 * Evict the least-recently-used page (LRU-list head) back to freeLists[0],
 * unless it was already used this frame.  Returns 1 on eviction, 0 otherwise. */
int __cdecl SMC_ForceFreeBlock(smcCache_t *cache)
{
  smcPage_t *page = (smcPage_t *)cache->lruList.next;

  if ( page->lastUsedFrame == tr_frameCount )
    return 0;

  SMC_FreeCachedSurface_r(cache, page, 0, SMC_SIZE_CLASS_COUNT - 1);

  /* unlink the page from the LRU list */
  ((smcLink_t *)page->lruLink.prev)->next = page->lruLink.next;
  ((smcLink_t *)page->lruLink.next)->prev = page->lruLink.prev;

  /* return the whole-page block to freeLists[0] */
  page->surfaces[0].freeLink.next = (int)&cache->freeLists[0];
  page->surfaces[0].freeLink.prev = cache->freeLists[0].prev;
  ((smcLink_t *)page->surfaces[0].freeLink.next)->prev = (int)&page->surfaces[0].freeLink;
  ((smcLink_t *)page->surfaces[0].freeLink.prev)->next = (int)&page->surfaces[0].freeLink;
  return 1;
}

/* ---- SMC_GetFreeBlockOfSize  0x004E5010 ----  VERIFIED
 * Ensure freeLists[listIndex] has a block by splitting one from the next-larger
 * class (recursively, or forcing a page free at class 0).  The parent block is
 * halved and both buddies are pushed onto freeLists[listIndex].  Returns 1 on
 * success, 0 if no block could be produced. */
int __cdecl SMC_GetFreeBlockOfSize(smcCache_t *cache, int listIndex)
{
  smcSurface_t *block;
  smcSurface_t *buddy;
  smcPage_t    *page;

  if ( listIndex == 0 )
    return SMC_ForceFreeBlock(cache);

  if ( cache->freeLists[listIndex - 1].prev == (int)&cache->freeLists[listIndex - 1]
    && !SMC_GetFreeBlockOfSize(cache, listIndex - 1) )
    return 0;

  /* take the parent block and unlink it */
  block = (smcSurface_t *)cache->freeLists[listIndex - 1].prev;
  ((smcLink_t *)block->freeLink.prev)->next = block->freeLink.next;
  ((smcLink_t *)block->freeLink.next)->prev = block->freeLink.prev;

  page = &cache->pages[((char *)block - (char *)cache) / (int)sizeof(smcPage_t)];
  if ( listIndex == 1 )
  {
    /* first split of a whole page: put the page on the LRU list */
    page->lruLink.next = (int)&cache->lruList;
    page->lruLink.prev = cache->lruList.prev;
    ((smcLink_t *)page->lruLink.next)->prev = (int)&page->lruLink;
    ((smcLink_t *)page->lruLink.prev)->next = (int)&page->lruLink;
  }

  /* push both halves of the split onto freeLists[listIndex] */
  block->freeLink.next = (int)&cache->freeLists[listIndex];
  block->freeLink.prev = cache->freeLists[listIndex].prev;
  ((smcLink_t *)block->freeLink.next)->prev = (int)&block->freeLink;
  ((smcLink_t *)block->freeLink.prev)->next = (int)&block->freeLink;

  buddy = &page->surfaces[(block - page->surfaces) + (1 << ((SMC_SIZE_CLASS_COUNT - 1) - listIndex))];
  buddy->freeLink.next = (int)&cache->freeLists[listIndex];
  buddy->freeLink.prev = cache->freeLists[listIndex].prev;
  ((smcLink_t *)buddy->freeLink.next)->prev = (int)&buddy->freeLink;
  ((smcLink_t *)buddy->freeLink.prev)->next = (int)&buddy->freeLink;
  return 1;
}

/* ---- SMC_Allocate  0x004E5120 ----  VERIFIED
 * Allocate a block that holds 1 << sizeShift verts.  Pops a free block of the
 * matching size class (forcing splits/evictions as needed), marks its tree
 * path, and records the block's vertex offset.  Returns the surface, or NULL. */
smcSurface_t *__cdecl SMC_Allocate(smcCache_t *cache, int sizeShift)
{
  int           listIndex = SMC_MAX_SIZE_SHIFT - sizeShift;
  smcSurface_t *surface;
  smcPage_t    *page;
  int           pageIndex;
  int           surfaceIndex;
  int           nodeIndex;

  if ( cache->freeLists[listIndex].prev == (int)&cache->freeLists[listIndex]
    && !SMC_GetFreeBlockOfSize(cache, listIndex) )
    return 0;

  /* take the free block and unlink it */
  surface = (smcSurface_t *)cache->freeLists[listIndex].prev;
  ((smcLink_t *)surface->freeLink.prev)->next = surface->freeLink.next;
  ((smcLink_t *)surface->freeLink.next)->prev = surface->freeLink.prev;

  pageIndex = ((char *)surface - (char *)cache) / (int)sizeof(smcPage_t);
  page = &cache->pages[pageIndex];
  if ( listIndex == 0 )
  {
    /* whole page allocated in one block: put it on the LRU list */
    page->lruLink.next = (int)&cache->lruList;
    page->lruLink.prev = cache->lruList.prev;
    ((smcLink_t *)page->lruLink.next)->prev = (int)&page->lruLink;
    ((smcLink_t *)page->lruLink.prev)->next = (int)&page->lruLink;
  }

  surfaceIndex = surface - page->surfaces;
  nodeIndex = ((surfaceIndex + SMC_SURFACES_PER_PAGE) >> ((SMC_SIZE_CLASS_COUNT - 1) - listIndex)) - 1;
  page->tree[nodeIndex].allocated = 1;
  for ( ; nodeIndex >= 0; nodeIndex = (nodeIndex - 1) >> 1 )
    page->tree[nodeIndex].usedVertexCount += 1 << sizeShift;

  /* block vertex offset within the whole cache = pageIndex*512 + surfaceIndex*32 */
  page->surfaces[surfaceIndex].cached.vertexOffset = 32 * (surfaceIndex + SMC_SURFACES_PER_PAGE * pageIndex);
  return &page->surfaces[surfaceIndex];
}

/* retail's inlined fld / fadd qword 2^-30 / fistp: round-to-nearest under the
 * default control word, with the bias breaking x.5 ties upward.  An (int) cast
 * would go through __ftol2 and truncate. */
static int R_SMC_FtolBiased( float f )
{
	double bias = 9.313225746154785e-10;   /* 2^-30 */
	int    i;

	__asm fld   f
	__asm fadd  bias
	__asm fistp i

	return i;
}

/* retail clamps the biased fistp through nested low/high clamp macros, so the
 * conversion is re-evaluated on every arm */
#define SMC_CLAMP_LO( x )   ( ( x ) < 0 ? 0 : ( x ) )
#define SMC_CLAMP_HI( x )   ( ( x ) > 255 ? 255 : ( x ) )
#define SMC_CLAMP_BYTE( f ) SMC_CLAMP_HI( SMC_CLAMP_LO( R_SMC_FtolBiased( f ) ) )

/* ---- R_EvaluateLightingAtPoint  0x004E5210 ----  VERIFIED
 * Accumulate the world's entity-ambient term plus every attached light's
 * contribution at one point/normal and store the clamped RGBA.  Directional
 * lights use the light direction directly; point lights add distance
 * attenuation and an optional spot cone. */
void __cdecl R_EvaluateLightingAtPoint(trRefEntity_t *entity, float *point, float *normal, byte *color)
{
  const dlight_t *light;
  float  accum[4];
  float  diffuseFactor;
  rfloat scale;
  float  dist;
  float  spotDot;
  vec3_t dir;
  int    i;
  int    e;

  if ( !*(int *)(tr_world + 280) )          /* tr.world->lightIndexCount == 0 */
  {
    color[0] = tr_identityLightByte;
    color[1] = tr_identityLightByte;
    color[2] = tr_identityLightByte;
    color[3] = 255;
    return;
  }

  /* base = tr.world->entityAmbientBase[k] + diffuseSun * entityAmbientScale[k] */
  accum[0] = *(float *)(tr_world + 248) * entity->diffuseSun + *(float *)(tr_world + 216);
  accum[1] = *(float *)(tr_world + 252) * entity->diffuseSun + *(float *)(tr_world + 220);
  accum[2] = *(float *)(tr_world + 256) * entity->diffuseSun + *(float *)(tr_world + 224);
  accum[3] = *(float *)(tr_world + 260) * entity->diffuseSun + *(float *)(tr_world + 228);

  for ( i = 0; i < entity->lightCount; ++i )
  {
    light = entity->lights[i].light;
    scale = entity->lights[i].scale;

    if ( light->position[3] == 0.0f )
    {
      /* directional light: dot(normal, direction) */
      diffuseFactor = light->position[2] * normal[2]
                    + light->position[0] * normal[0]
                    + light->position[1] * normal[1];
      if ( diffuseFactor < 0.0f )
        diffuseFactor = 0.0f;
    }
    else
    {
      /* point light: direction, distance attenuation, optional spot cone */
      dir[0] = light->position[0] - point[0];
      dir[1] = light->position[1] - point[1];
      dir[2] = light->position[2] - point[2];
      dist = VectorNormalize(dir);

      diffuseFactor = dir[2] * normal[2] + dir[1] * normal[1] + dir[0] * normal[0];
      if ( diffuseFactor < 0.0f )
        diffuseFactor = 0.0f;

      scale = scale / ((rfloat)dist * ((rfloat)dist * light->quadraticAttenuation + light->linearAttenuation)
                       + light->constantAttenuation);

      if ( light->spotCutoff != 180.0f )
      {
        spotDot = dir[2] * light->spotDirection[2]
                + dir[1] * light->spotDirection[1]
                + dir[0] * light->spotDirection[0];
        if ( spotDot <= tr_sinTable[R_SMC_FtolBiased((light->spotCutoff + 90.0f) * 2.8444445f) & 0x3FF] )
        {
          scale = 0.0f;
        }
        else
        {
          scale = (float)scale;   /* retail reloads the rounded float home here */
          for ( e = 0; (rfloat)e < light->spotExponent; ++e )
            scale *= spotDot;
        }
      }
    }

    accum[0] += ((rfloat)diffuseFactor * light->diffuse[0] + light->ambient[0]) * scale;
    accum[1] += ((rfloat)diffuseFactor * light->diffuse[1] + light->ambient[1]) * scale;
    accum[2] += ((rfloat)diffuseFactor * light->diffuse[2] + light->ambient[2]) * scale;
    accum[3] += ((rfloat)diffuseFactor * light->diffuse[3] + light->ambient[3]) * scale;
  }

  /* scale each channel to [0,255]: retail's FISTP round-to-nearest plus a
   * 2^-30 bias (R_SMC_FtolBiased, not an (int) cast, which would truncate) */
  color[0] = SMC_CLAMP_BYTE(accum[0] * 255.0f);
  color[1] = SMC_CLAMP_BYTE(accum[1] * 255.0f);
  color[2] = SMC_CLAMP_BYTE(accum[2] * 255.0f);
  color[3] = SMC_CLAMP_BYTE(accum[3] * 255.0f);
}

/* fill every cached vertex's RGBA with one packed colour (retail evaluates the
 * colour once, before the loop, as an inlined helper's argument) */
static void R_SMC_SetColor(const smSurface_t *source, smcVertex_t *verts, int rgba)
{
  int i;

  for ( i = 0; i < source->vertexCount; ++i )
    *(int *)verts[i].color = rgba;
}

/* set every cached vertex's alpha */
static void R_SMC_SetAlpha(const smSurface_t *source, smcVertex_t *verts, byte alpha)
{
  int i;

  for ( i = 0; i < source->vertexCount; ++i )
    verts[i].color[3] = alpha;
}

/* ---- R_CacheStaticModelSurface  0x004E57E0 ----  VERIFIED
 * Cache one static-model surface: allocate a cache block, transform its verts
 * into world space, colour them per the shader's first-stage rgbGen/alphaGen,
 * and upload the block to the backend-appropriate storage.  Retail passes the
 * owner in EAX and source in ECX, which VC7 only reproduces with the parameters
 * declared (source, owner, ...); swapping needs the tr_staticmodel.c call too.
 * Returns the cached surface, or NULL when caching is disabled/too large. */
smcSurface_t *__cdecl R_CacheStaticModelSurface(smSurface_t *source, renderer_static_model_t *owner,
                                                int surfaceIndex, trRefEntity_t *entity, float *inverseAxis)
{
  smcSurface_t *cached;
  smcVertex_t  *verts;
  int           sizeShift;
  int           i;
  vec3_t        transformedNormal;

  if ( !r_smc_enable->integer )
    return 0;
  if ( source->vertexCount > SMC_PAGE_VERTEX_COUNT )
    return 0;

  /* smallest size class ( >= 32 verts ) that holds the surface */
  for ( sizeShift = SMC_MIN_SIZE_SHIFT; source->vertexCount > 1 << sizeShift; ++sizeShift )
    ;

  cached = SMC_Allocate(&r_smcCache, sizeShift);
  if ( !cached )
    return 0;

  r_smcCache.allocatedVertexCount += 1 << sizeShift;
  r_smcCache.usedVertexCount      += source->vertexCount;

  cached->cached.surfaceType  = tr_cachedStaticModelSurfaceType;
  cached->cached.owner        = (int)owner;
  cached->cached.source       = (int)source;
  cached->cached.surfaceIndex = surfaceIndex;

  verts = (smcVertex_t *)ri.Hunk_AllocateTempMemory(24 * source->vertexCount);

  /* transform positions into world space; copy texture coordinates */
  for ( i = 0; i < source->vertexCount; ++i )
  {
    verts[i].texCoord[0] = source->texCoords[i][0];
    verts[i].texCoord[1] = source->texCoords[i][1];
    verts[i].position[0] = source->vertices[i][2] * owner->entity.axis[2][0]
                         + source->vertices[i][1] * owner->entity.axis[1][0]
                         + source->vertices[i][0] * owner->entity.axis[0][0]
                         + owner->entity.origin[0];
    verts[i].position[1] = source->vertices[i][2] * owner->entity.axis[2][1]
                         + source->vertices[i][1] * owner->entity.axis[1][1]
                         + source->vertices[i][0] * owner->entity.axis[0][1]
                         + owner->entity.origin[1];
    verts[i].position[2] = source->vertices[i][2] * owner->entity.axis[2][2]
                         + source->vertices[i][1] * owner->entity.axis[1][2]
                         + source->vertices[i][0] * owner->entity.axis[0][2]
                         + owner->entity.origin[2];
  }

  /* colour the verts per the first stage's rgbGen */
  switch ( R_ShaderStage0(source->cachedShader)->rgbGen )
  {
    case CGEN_LIGHTING_DIFFUSE:
      for ( i = 0; i < source->vertexCount; ++i )
      {
        /* the redundant parentheses steer VC7's term order to retail's */
        transformedNormal[0] = source->normals[i][1] * inverseAxis[1] + source->normals[i][0] * inverseAxis[0]
                             + inverseAxis[2] * source->normals[i][2];
        transformedNormal[1] = ( inverseAxis[5] * source->normals[i][2] + source->normals[i][0] * inverseAxis[3]
                               + source->normals[i][1] * inverseAxis[4] );
        transformedNormal[2] = (source->normals[i][0] * inverseAxis[6] + inverseAxis[8] * source->normals[i][2])
                             + source->normals[i][1] * inverseAxis[7];
        VectorNormalize(transformedNormal);
        R_EvaluateLightingAtPoint(entity, verts[i].position, transformedNormal, verts[i].color);
      }
      break;
    case CGEN_IDENTITY:
      R_SMC_SetColor(source, verts, -1);                            /* white */
      break;
    case CGEN_IDENTITY_LIGHTING:
      R_SMC_SetColor(source, verts, 16843009 * tr_identityLightByte);   /* 0x01010101: byte replicated */
      break;
    case CGEN_CONSTANT:
      R_SMC_SetColor(source, verts, *(int *)R_ShaderStage0(source->cachedShader)->constantColor);
      break;
    case CGEN_LIGHTING_PRECALC:
      R_SMC_SetColor(source, verts, *(int *)entity->e.shaderRGBA);
      break;
    default:
      break;
  }

  /* alpha channel: constant alpha, or forced opaque */
  if ( R_ShaderStage0(source->cachedShader)->alphaGen != AGEN_UNSPECIFIED )
  {
    if ( R_ShaderStage0(source->cachedShader)->alphaGen == AGEN_CONSTANT
      && R_ShaderStage0(source->cachedShader)->rgbGen != CGEN_CONSTANT )
      R_SMC_SetAlpha(source, verts, R_ShaderStage0(source->cachedShader)->constantColor[3]);
  }
  else if ( R_ShaderStage0(source->cachedShader)->rgbGen != CGEN_IDENTITY )
  {
    R_SMC_SetAlpha(source, verts, 255);
  }

  /* upload the block to the backend-appropriate storage */
  switch ( tr_cachedStaticModelSurfaceType )
  {
    case SF_CACHED_STATIC_MODEL_GENERIC:
      memcpy((char *)tr_cachedStaticModelStorage + 24 * cached->cached.vertexOffset,
              verts, 24 * source->vertexCount);
      break;
    case SF_CACHED_STATIC_MODEL_ARB:
      qglBindBufferARB(GL_ARRAY_BUFFER_ARB, tr_cachedStaticModelStorage);
      qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 24 * cached->cached.vertexOffset,
                          24 * source->vertexCount, verts);
      qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
      break;
    case SF_CACHED_STATIC_MODEL_ATI:
      qglUpdateObjectBufferATI(tr_cachedStaticModelStorage,
                               tr_cachedStaticModelStorageOffset + 24 * cached->cached.vertexOffset,
                               24 * source->vertexCount, verts, GL_PRESERVE_ATI);
      break;
    case SF_CACHED_STATIC_MODEL_NV:
      memcpy((smcVertex_t *)tr_cachedStaticModelStorage + cached->cached.vertexOffset,
              verts, 24 * source->vertexCount);
      break;
    default:
      break;
  }

  ri.Hunk_FreeTempMemory(verts);
  return cached;
}

/* ---- R_UsedCachedStaticModelSurface  0x004E5C90 ----  VERIFIED
 * Stamp the surface's page as used this frame and move it to the tail of the
 * LRU list so it is evicted last.  Returns the page (dead EAX). */
smcPage_t *__cdecl R_UsedCachedStaticModelSurface(smcSurface_t *surface)
{
  smcPage_t *page = &r_smcCache.pages[((char *)surface - (char *)&r_smcCache) / (int)sizeof(smcPage_t)];

  page->lastUsedFrame = tr_frameCount;

  /* unlink from the LRU list */
  ((smcLink_t *)page->lruLink.prev)->next = page->lruLink.next;
  ((smcLink_t *)page->lruLink.next)->prev = page->lruLink.prev;

  /* re-append at the tail (most recently used) */
  page->lruLink.next = (int)&r_smcCache.lruList;
  page->lruLink.prev = r_smcCache.lruList.prev;
  r_smcCache.lruList.prev = (int)&page->lruLink;
  ((smcLink_t *)page->lruLink.prev)->next = (int)&page->lruLink;
  return page;
}

/* ---- R_AllocStaticModelCacheGeneric  0x004E5CF0 ----  VERIFIED
 * Back the cache with a plain hunk block (system-memory upload path). */
int R_AllocStaticModelCacheGeneric()
{
  int result;

  result = ri.Hunk_Alloc(SMC_STORAGE_BYTES);
  tr_cachedStaticModelStorage = result;
  tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_GENERIC;
  return result;
}

/* ---- R_AllocStaticModelCacheARB  0x004E5D10 ----  VERIFIED
 * Back the cache with an ARB vertex buffer object, falling back to a hunk
 * block if the buffer cannot be created. */
int R_AllocStaticModelCacheARB()
{
  int result;

  result = R_CreateBufferARB(GL_ARRAY_BUFFER_ARB, SMC_STORAGE_BYTES, 0, tr_vboUsage);
  tr_cachedStaticModelStorage = result;
  if ( !result )
  {
    result = ri.Hunk_Alloc(SMC_STORAGE_BYTES);
    tr_cachedStaticModelStorage = result;
    tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_GENERIC;
  }
  else
  {
    tr_cachedStaticModelStorageSource = storageClass;
    tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_ARB;
  }
  return result;
}

/* ---- R_AllocStaticModelCacheATI  0x004E5D70 ----  VERIFIED
 * Back the cache with ATI static vertex memory (primary, else secondary),
 * falling back to a hunk block if neither pool has room. */
void R_AllocStaticModelCacheATI()
{
  tr_cachedStaticModelStorage = 0;
  if ( tr_staticVertexMemoryPrimaryUsed + SMC_STORAGE_BYTES <= tr_staticVertexMemoryPrimaryLimit )
  {
    tr_cachedStaticModelStorageOffset = tr_staticVertexMemoryPrimaryUsed;
    tr_staticVertexMemoryPrimaryUsed += SMC_STORAGE_BYTES;
    tr_cachedStaticModelStorageSource = 3;
    tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_ATI;
    tr_cachedStaticModelStorage = tr_staticVertexMemoryPrimary;
  }
  else if ( tr_staticVertexMemorySecondaryUsed + SMC_STORAGE_BYTES <= tr_staticVertexMemorySecondaryLimit )
  {
    tr_cachedStaticModelStorageOffset = tr_staticVertexMemorySecondaryUsed;
    tr_staticVertexMemorySecondaryUsed += SMC_STORAGE_BYTES;
    tr_cachedStaticModelStorageSource = 3;
    tr_cachedStaticModelStorage = tr_staticVertexMemorySecondary;
    tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_ATI;
  }
  else
  {
    tr_cachedStaticModelStorageSource = 0;
    ri.Printf(0, "^3Couldn't allocate memory for the static model surface cache using ATI optimizations");
    tr_cachedStaticModelStorage = ri.Hunk_Alloc(SMC_STORAGE_BYTES);
    tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_GENERIC;
  }
}

/* ---- R_AllocStaticModelCacheNV  0x004E5E30 ----  VERIFIED
 * Back the cache with NV static vertex memory (primary, else secondary), or an
 * aligned hunk block.  Returns the storage-source class (dead EAX). */
int R_AllocStaticModelCacheNV()
{
  tr_cachedStaticModelStorage = 0;
  if ( tr_staticVertexMemoryPrimaryUsed + SMC_STORAGE_BYTES < tr_staticVertexMemoryPrimaryLimit )
  {
    tr_cachedStaticModelStorage = tr_staticVertexMemoryPrimaryUsed + tr_staticVertexMemoryPrimary;
    tr_staticVertexMemoryPrimaryUsed += SMC_STORAGE_BYTES;
    tr_cachedStaticModelStorageSource = 1;
    tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_NV;
    return 1;
  }

  if ( tr_staticVertexMemorySecondaryUsed + SMC_STORAGE_BYTES < tr_staticVertexMemorySecondaryLimit )
  {
    tr_cachedStaticModelStorage = tr_staticVertexMemorySecondaryUsed + tr_staticVertexMemorySecondary;
    tr_staticVertexMemorySecondaryUsed += SMC_STORAGE_BYTES;
    tr_cachedStaticModelStorageSource = 2;
    tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_NV;
    return 2;
  }

  tr_cachedStaticModelStorage = (int)Hunk_AllocAlignInternal(SMC_STORAGE_BYTES, 32);
  tr_cachedStaticModelStorageSource = 3;
  tr_cachedStaticModelSurfaceType = SF_CACHED_STATIC_MODEL_NV;
  return 3;
}

/* ---- R_InitStaticModelCache  0x004E5EE0 ----  VERIFIED
 * Allocate the storage block via the best available backend, then reset the
 * cache: clear it, seed the LRU list and size-class free lists as empty, and
 * hand every page to freeLists[0] as one whole-page free block. */
void R_InitStaticModelCache()
{
  smcPage_t *page;
  int        i;

  if ( glConfig_ARBVertexBufferObject )
    R_AllocStaticModelCacheARB();
  else if ( glConfig_NVVertexArrayRange )
    R_AllocStaticModelCacheNV();
  else if ( glConfig_ATIVertexArrayObject )
    R_AllocStaticModelCacheATI();
  else
    R_AllocStaticModelCacheGeneric();

  memset(&r_smcCache, 0, sizeof(r_smcCache));

  /* seed the LRU list and the five size-class free lists as empty sentinels */
  r_smcCache.lruList.next      = (int)&r_smcCache.lruList;
  r_smcCache.lruList.prev      = (int)&r_smcCache.lruList;
  r_smcCache.freeLists[1].next = (int)&r_smcCache.freeLists[1];
  r_smcCache.freeLists[1].prev = (int)&r_smcCache.freeLists[1];
  r_smcCache.freeLists[2].next = (int)&r_smcCache.freeLists[2];
  r_smcCache.freeLists[2].prev = (int)&r_smcCache.freeLists[2];
  r_smcCache.freeLists[3].next = (int)&r_smcCache.freeLists[3];
  r_smcCache.freeLists[3].prev = (int)&r_smcCache.freeLists[3];
  r_smcCache.freeLists[4].next = (int)&r_smcCache.freeLists[4];
  r_smcCache.freeLists[4].prev = (int)&r_smcCache.freeLists[4];
  r_smcCache.freeLists[0].next = (int)&r_smcCache.freeLists[0];
  r_smcCache.freeLists[0].prev = (int)&r_smcCache.freeLists[0];

  /* every page starts as one whole-page free block on freeLists[0] */
  for ( i = 0, page = r_smcCache.pages; i < 128; ++i, ++page )
  {
    page->surfaces[0].freeLink.next = (int)&r_smcCache.freeLists[0];
    page->surfaces[0].freeLink.prev = r_smcCache.freeLists[0].prev;
    ((smcLink_t *)page->surfaces[0].freeLink.next)->prev = (int)&page->surfaces[0].freeLink;
    ((smcLink_t *)page->surfaces[0].freeLink.prev)->next = (int)&page->surfaces[0].freeLink;
  }
}

/* ---- R_StaticModelCacheStats_f  0x004E6100 ----  VERIFIED
 * Print the cache's allocation and utilisation percentages. */
void R_StaticModelCacheStats_f()
{
  ri.Printf(0, "%.2f%% of cache is currently allocated.\n",
            (rfloat)r_smcCache.allocatedVertexCount * 0.00152587890625f);
  if ( r_smcCache.allocatedVertexCount )
    ri.Printf(0, "%.2f%% allocated cache vertices are used.\n",
              (rfloat)r_smcCache.usedVertexCount * 100.0f / r_smcCache.allocatedVertexCount);
}

/* ---- R_StaticModelCacheFlush_f  0x004E6160 ----  VERIFIED
 * Evict every page: free each LRU-list page's buddy tree and return its
 * whole-page block to freeLists[0].  Retail unlinks the page through
 * lruList.prev (reloaded after every store), not through the page local. */
void R_StaticModelCacheFlush_f()
{
  smcPage_t *page;

  while ( r_smcCache.lruList.prev != (int)&r_smcCache.lruList )
  {
    page = (smcPage_t *)r_smcCache.lruList.prev;
    SMC_FreeCachedSurface_r(&r_smcCache, page, 0, SMC_SIZE_CLASS_COUNT - 1);

    /* unlink the page from the LRU list */
    ((smcLink_t *)((smcLink_t *)r_smcCache.lruList.prev)->prev)->next = ((smcLink_t *)r_smcCache.lruList.prev)->next;
    ((smcLink_t *)((smcLink_t *)r_smcCache.lruList.prev)->next)->prev = ((smcLink_t *)r_smcCache.lruList.prev)->prev;

    /* return the whole-page block to freeLists[0] */
    page->surfaces[0].freeLink.next = (int)&r_smcCache.freeLists[0];
    page->surfaces[0].freeLink.prev = r_smcCache.freeLists[0].prev;
    ((smcLink_t *)page->surfaces[0].freeLink.next)->prev = (int)&page->surfaces[0].freeLink;
    ((smcLink_t *)page->surfaces[0].freeLink.prev)->next = (int)&page->surfaces[0].freeLink;
  }
}
