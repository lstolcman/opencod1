/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_orientation.h"   /* backEnd.or 0x016D8DA8 -- one object, not 16 shards */
#include "tr_tess.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
float rendererSunFlareVisibility;   /* 0x014072FC */
int rendererActiveFlares;   /* 0x016DA40C */
int rendererFreeFlares;   /* 0x016DCA20 */
unsigned char rendererFlarePool[9740];   /* 0x016DA420 */


extern int GL_Bind();
extern int GL_State();
extern int RB_BeginImmediateMode();
void __cdecl RB_BeginSurface( void *shader, int vertexComponentCount );
extern void __cdecl RB_CalcSunBlind( float *outBlind, float *outGlare, float visibility );
void __cdecl RB_EndSurface( void );
extern int RB_SelectStorageATI();
extern int RB_SelectStorageNV();
unsigned __int64 __cdecl RB_glColor4f( float r, float g, float b, float a );
void __cdecl RB_glVertex3f( float x, float y, float z );
GLenum __cdecl RB_glBegin( GLenum result );
void RB_glEnd( void );
void RB_EndImmediateMode( void );
extern int R_FogOn();
extern int R_TransformClipToWindow();
extern int R_TransformHomogenousModelToClip();
 double __cdecl R_UpdateOverTime( float current, float target,
                                  int fadeInRate, int fadeOutRate,
                                  int elapsedMsec );
void VectorNormalizeFast( float *v );


/* -------------------------------------------------------------------------
 *  flare_t (0x4c) -- one persistent light-flare record.  A light flare is an
 *  eye effect drawn for a bright, visible light source; its screen size is
 *  roughly constant while its intensity is faded up/down as the source becomes
 *  visible/occluded, which requires state that survives across scenes.
 *
 *  128 records live in the global pool (rendererFlarePool).  RB_AddFlare threads
 *  them onto an active chain (rendererActiveFlares, tr's r_activeFlares) and a free
 *  chain (rendererFreeFlares, r_inactiveFlares); `next` at +0x00 is the chain link.
 *  Descends from RTCW's flare_t; the CoD-specific fields (screenRadius, and
 *  the NDC depth stored in eyeZ) are proven by the decompiled reads and match
 *  the CoDUO renderer_flare_t reconstruction.
 * ------------------------------------------------------------------------- */
typedef struct flare_s {
	struct flare_s *next;          /* +0x00  active / free chain link */
	int             addedFrame;    /* +0x04  backEnd.viewParms.frameCount when last added (-1 => fresh) */
	int             portalView;    /* +0x08  view discriminator (backEnd wireframe/portal state) */
	int             frameSceneNum; /* +0x0c */
	int             fadeInMsec;    /* +0x10 */
	int             fadeOutMsec;   /* +0x14 */
	shader_t       *shader;        /* +0x18  flare / spotlight material */
	int             active;        /* +0x1c  qboolean -- visible this scene */
	int             fadeTime;      /* +0x20  time of the last visibility update */
	float           drawIntensity; /* +0x24  faded intensity (0 => not drawn) */
	int             windowX;       /* +0x28 */
	int             windowY;       /* +0x2c */
	float           eyeZ;          /* +0x30  NDC depth used for the occlusion readback */
	vec3_t          color;         /* +0x34 */
	float           size;          /* +0x40 */
	int             screenRadius;  /* +0x44  screen-space test half-size, pixels */
	int             id;            /* +0x48  matches the source; -1 for the sun flare */
} flare_t;
COD1_ASSERT_SIZE(flare_t, 0x4c);
ASSERT_OFF(flare_t, portalView, 0x08);
ASSERT_OFF(flare_t, shader, 0x18);
ASSERT_OFF(flare_t, active, 0x1c);
ASSERT_OFF(flare_t, fadeTime, 0x20);
ASSERT_OFF(flare_t, eyeZ, 0x30);
ASSERT_OFF(flare_t, color, 0x34);
ASSERT_OFF(flare_t, screenRadius, 0x44);
ASSERT_OFF(flare_t, id, 0x48);

/* -------------------------------------------------------------------------
 *  flareSource_t (0x3c) -- the descriptor that RB_AddDlightFlares,
 *  RB_AddCoronaFlares and RB_UpdateSunFlare fill and hand to RB_AddFlare
 *  (CoDUO renderer_flare_source_t).  `color` is a vec4_t but only rgb reaches
 *  the flare: color[3] is copied into flare.size and then immediately
 *  overwritten by size -- the retail bug reproduced in RB_AddFlare.
 * ------------------------------------------------------------------------- */
typedef struct {
	int        id;           /* +0x00 */
	shader_t  *shader;       /* +0x04 */
	vec3_t     origin;       /* +0x08 */
	float      depthOffset;  /* +0x14  pulls the flare toward the eye; also the homogeneous w */
	vec4_t     color;        /* +0x18  color[3] unused by the flare */
	float      size;         /* +0x28 */
	int        screenRadius; /* +0x2c */
	int        fadeInMsec;   /* +0x30 */
	int        fadeOutMsec;  /* +0x34 */
	int        active;       /* +0x38  qboolean */
} flareSource_t;
COD1_ASSERT_SIZE(flareSource_t, 0x3c);
ASSERT_OFF(flareSource_t, origin, 0x08);
ASSERT_OFF(flareSource_t, depthOffset, 0x14);
ASSERT_OFF(flareSource_t, color, 0x18);
ASSERT_OFF(flareSource_t, size, 0x28);
ASSERT_OFF(flareSource_t, screenRadius, 0x2c);
ASSERT_OFF(flareSource_t, fadeInMsec, 0x30);
ASSERT_OFF(flareSource_t, active, 0x38);

/* -------------------------------------------------------------------------
 *  corona_t (0x30) -- a light corona submitted by the client for the frame
 *  (CoDUO renderer_corona_t; RTCW corona_t).  RB_AddCoronaFlares reads these
 *  out of backEnd.refdef.coronas at a 0x30 stride.  `transformed` is an
 *  RTCW-lineage local-space slot that CoD leaves unused.
 * ------------------------------------------------------------------------- */
typedef struct {
	vec3_t origin;       /* +0x00 */
	vec3_t color;        /* +0x0c */
	vec3_t transformed;  /* +0x18  unused */
	float  scale;        /* +0x24 */
	int    id;           /* +0x28 */
	int    flags;        /* +0x2c  bit 0x2 => use the spotlight material */
} corona_t;
COD1_ASSERT_SIZE(corona_t, 0x30);
ASSERT_OFF(corona_t, color, 0x0c);
ASSERT_OFF(corona_t, scale, 0x24);
ASSERT_OFF(corona_t, id, 0x28);
ASSERT_OFF(corona_t, flags, 0x2c);

void __cdecl RB_AddFlare( flareSource_t *src, float *normal );

#define MAX_FLARES    128
#define FLARE_BYTES   0x2600          /* == MAX_FLARES * sizeof(flare_t) */
COD1_STATIC_ASSERT(FLARE_BYTES == MAX_FLARES * sizeof(flare_t),
                   "flare pool byte size must equal MAX_FLARES * sizeof(flare_t)");

/* retail's inlined float->int idiom (0x004D92C0-0x004D92D8 and the fade
 * conversions at 0x004D9028-0x004D9076 / 0x004D914F-0x004D91A1): fld float /
 * fadd qword 2^-30 / fistp -- round-to-nearest under the default control word,
 * not the truncating __ftol2 an (int) cast compiles to.  The operand is a float,
 * so callers' products are rounded to float first, as retail's fstp does. */
static int R_FlareFistp( float f )
{
  double bias = 9.313225746154785e-10;
  int    i;

  __asm fld   f
  __asm fadd  bias
  __asm fistp i
  return i;
}

/* ---- RE_ClearFlares  0x004D8C50 ----  VERIFIED
 * Zero the flare pool and rebuild the free list (each record links back to the
 * previous one, so the free head is the last record).  The return value is
 * dead EAX -- the past-the-end cursor -- preserved exactly. */
flare_t *RE_ClearFlares()
{
  flare_t *pool = (flare_t *)&rendererFlarePool;
  flare_t *prev;
  flare_t *f;

  memset(pool, 0, FLARE_BYTES);
  prev = 0;
  rendererActiveFlares = 0;                              /* r_activeFlares = NULL */
  rendererSunFlareVisibility = 0.0;                              /* sun-flare visibility */
  rendererSunState_currentBlindFraction = 0.0;
  rendererSunState_currentGlareFraction = 0.0;
  rendererSunState_lastUpdateTime = 0;

  f = pool;
  do
  {
    f->next = prev;
    prev = f;
    ++f;
  }
  while ( (int)f < (int)&pool[MAX_FLARES] );
  rendererFreeFlares = (int)prev;                      /* r_inactiveFlares = last record */
  return f;
}

/* ---- RB_AddFlare  0x004D8CA0 ----  VERIFIED
 * Called at surface tessellation time.  Finds (or allocates) the flare that
 * matches this source id / scene / view, transforms the source point to the
 * screen and stores the test info.  Retail deliberately writes flare.size
 * twice (from color[3] then from size), discarding the color[3] word -- that
 * dead store is reproduced faithfully. */
void __cdecl RB_AddFlare(flareSource_t *src, float *normal)
{
  flare_t *f;
  float    dot;
  float    margin;
  float    eye[4];    /* [ebp-30h] BYREF -- retail vec4_t */
  float    point[4];  /* [ebp-20h] BYREF -- the source point, then window coords */
  float    clip[4];   /* [ebp-10h] BYREF */
  float    local[3];  /* Q3's `local`: shares eye's slot, and its references put eye at the frame bottom */
  int      i;

  f = (flare_t *)rendererActiveFlares;
  ++backEnd_pc_flareAddCount;
  if ( rendererActiveFlares )
  {
    do
    {
      if ( f->id == src->id
        && f->frameSceneNum == backEnd_viewParms_frameSceneNum
        && f->portalView == backEnd_wireframeOverride )
      {
        break;
      }
      f = f->next;
    }
    while ( f );
  }

  point[0] = src->origin[0];
  point[1] = src->origin[1];
  point[2] = src->origin[2];
  point[3] = src->depthOffset;
  if ( point[3] != 0.0f )
  {
    VectorMA(point, -point[3], backEnd_viewParms_originX, eye);
    VectorNormalizeFast(eye);
    VectorMA(point, point[3] * -12.0f, eye, point); /* -12.0f (0xC1400000), pooled .rdata literal in retail */
  }

  R_TransformHomogenousModelToClip(point, backEnd_or_modelMatrix, backEnd_viewParms_projectionMatrix, eye, clip);

  /* A new point flare (no existing record, no explicit screen radius) is
   * rejected here if it falls outside the clip volume. */
  if ( !f && !src->screenRadius )
  {
    for ( i = 0; i < 3; ++i )
    {
      if ( clip[i] >= clip[3] || clip[i] <= -clip[3] )
        return;
    }
  }

  R_TransformClipToWindow(clip, (int)backEnd_viewParms_originX, eye, point);
  if ( !f )
  {
    f = (flare_t *)rendererFreeFlares;
    if ( !rendererFreeFlares )
      return;                                      /* free list empty */

    margin = (float)src->screenRadius;
    if ( point[0] + margin < 0.0f
      || point[0] - margin >= backEnd_viewParms_viewportWidth
      || point[1] + margin < 0.0f
      || point[1] - margin >= backEnd_viewParms_viewportHeight )
    {
      return;
    }

    /* pop from the free list, push onto the active list */
    rendererFreeFlares = (int)((flare_t *)rendererFreeFlares)->next;
    f->next = (flare_t *)rendererActiveFlares;
    rendererActiveFlares = (int)f;
    f->frameSceneNum = backEnd_viewParms_frameSceneNum;
    f->portalView = backEnd_wireframeOverride;
    f->addedFrame = -1;
    f->id = src->id;
    f->drawIntensity = 0.0;
    f->fadeTime = backEnd_refdef_time - 10;
  }

  f->shader = src->shader;
  f->active = src->active;
  f->fadeInMsec = src->fadeInMsec;
  f->fadeOutMsec = src->fadeOutMsec;
  f->addedFrame = backEnd_viewParms_frameCount;
  f->color[0] = src->color[0];
  f->color[1] = src->color[1];
  f->color[2] = src->color[2];
  f->size = src->color[3];                         /* retail bug: this store is discarded... */
  f->size = src->size;                             /* ...by this one (source color[3] is unused) */
  f->screenRadius = src->screenRadius;

  if ( normal )
  {
    /* fade the flare down as its surface turns away from the viewer */
    local[0] = backEnd_viewParms_originX[0] - src->origin[0];
    local[1] = backEnd_viewParms_originY - src->origin[1];
    local[2] = backEnd_viewParms_originZ - src->origin[2];
    VectorNormalizeFast(local);
    dot = local[2] * normal[2] + local[1] * normal[1] + local[0] * normal[0];
    f->color[0] = dot * f->color[0];
    f->color[1] = dot * f->color[1];
    f->color[2] = dot * f->color[2];
  }

  f->windowX = (int)point[0] + backEnd_viewParms_viewportX;
  f->windowY = (int)point[1] + backEnd_viewParms_viewportY;
#ifdef COD1_VC7
  if ( src->depthOffset != 0.0f )
    f->eyeZ = (clip[2] / clip[3] + 1.0f) * 0.5f;
  else
    f->eyeZ = 1.0f;
#else
  if ( src->depthOffset != 0.0f )
    f->eyeZ = (clip[2] / clip[3] + 1.0) * 0.5;
  else
    f->eyeZ = 1.0f;
#endif
}

/* ---- RB_AddDlightFlares  0x004D8FD0 ----  VERIFIED
 * Adds a flare for each dynamic light (only when r_flares >= 2).  Retail is
 * void: the old Hex-Rays "return value" was only whatever EAX held. */
void RB_AddDlightFlares()
{
  dlight_t *light;
  flareSource_t src;
  int       i;

  if ( r_flares->integer >= 2 )
  {
    src.shader = (shader_t *)tr_flareShader;
    src.id = 1024;
    src.color[3] = 1.0f;
    src.size = r_flareSize->value;
    src.screenRadius = 0;
    src.fadeInMsec = R_FlareFistp(r_flareFadeIn->value * 1000.0f);   /* 0x004D8FFF-0x004D9040 */
    src.fadeOutMsec = R_FlareFistp(r_flareFadeOut->value * 1000.0f); /* 0x004D9049-0x004D9076 */
    src.active = 1;
    light = (dlight_t *)backEnd_refdef_dlights;
    for ( i = 0; i < backEnd_refdef_num_dlights; ++i, ++light )
    {
      src.origin[0] = light->position[0];
      src.origin[1] = light->position[1];
      src.origin[2] = light->position[2];
      src.depthOffset = light->position[3];
      src.color[0] = light->color[0];
      src.color[1] = light->color[1];
      src.color[2] = light->color[2];
      RB_AddFlare(&src, 0);
      ++src.id;
    }
  }
}

/* ---- RB_AddCoronaFlares  0x004D9100 ----  VERIFIED
 * Adds a flare for each corona (only when r_flares == 1 or 3, and a world is
 * loaded).  The return value is dead EAX, preserved exactly. */
int RB_AddCoronaFlares()
{
  int result;
  corona_t *corona;
  flareSource_t src;
  int i;

  result = r_flares->integer;
  if ( result == 1 || result == 3 )
  {
    result = tr_world;
    if ( tr_world )
    {
      src.depthOffset = 1.0f;
      src.color[3] = 1.0f;
      src.screenRadius = 0;
      src.fadeInMsec = R_FlareFistp(r_flareFadeIn->value * 1000.0f);   /* 0x004D912E-0x004D916B */
      src.fadeOutMsec = R_FlareFistp(r_flareFadeOut->value * 1000.0f); /* 0x004D9174-0x004D91A1 */
      src.active = 1;
      result = backEnd_refdef_coronaCount;
      corona = (corona_t *)backEnd_refdef_coronas;
      for ( i = 0; i < backEnd_refdef_coronaCount; ++i, ++corona )
      {
        src.id = corona->id;
        if ( corona->flags & 2 )
          src.shader = (shader_t *)tr_spotLightShader;
        else
          src.shader = (shader_t *)tr_flareShader;
        src.origin[0] = corona->origin[0];
        src.origin[1] = corona->origin[1];
        src.origin[2] = corona->origin[2];
        src.color[0] = corona->color[0];
        src.color[1] = corona->color[1];
        src.color[2] = corona->color[2];
        src.size = r_flareSize->value * corona->scale;
        RB_AddFlare(&src, 0);
        result = backEnd_refdef_coronaCount;
      }
    }
  }
  return result;
}

/* ---- RB_UpdateSunFlare  0x004D9250 ----  VERIFIED
 * Builds the sun's flare (id -1) from the world sun light and the caller's
 * shader/color/size/fade parameters.  brightness and size carry float bit patterns in
 * int slots, so they are reinterpreted rather than converted.  Parameter order
 * and types are kept exactly as decompiled. */
void __cdecl RB_UpdateSunFlare(int shader,float brightness,float size,float spriteSize,int fadeInMsec,int fadeOutMsec)
{
  flareSource_t src;
  dlight_t     *sun;
  float         screenRadius;

  src.shader = (shader_t *)shader;
  sun = *(dlight_t **)(tr_world + 276);           /* tr.world->sunLight */
  src.id = -1;
  src.origin[0] = sun->position[0];
  src.origin[1] = sun->position[1];
  src.origin[2] = sun->position[2];
  src.depthOffset = 0.0f;
  src.color[0] = brightness;
  src.color[1] = brightness;
  src.color[2] = brightness;
  src.color[3] = 1.0f;
  src.size = size;
  /* 1/640 is the .rdata float 0x3ACCCCCD (flt_568FE4), and the product is
   * stored as a float (0x004D928C) before the rounding fistp (0x004D92D8). */
#ifdef COD1_VC7
  screenRadius = backEnd_viewParms_viewportWidth * spriteSize * 0.0015625f;
#else
  screenRadius = (double)backEnd_viewParms_viewportWidth * spriteSize * (double)0.0015625f;
#endif
  src.screenRadius = R_FlareFistp(screenRadius);
  src.fadeInMsec = fadeInMsec;
  src.fadeOutMsec = fadeOutMsec;
  src.active = 1;
  RB_AddFlare(&src, 0);
}

/* ---- RB_TestFlare  0x004D9310 ----  VERIFIED
 * Reads back the depth buffer around the flare and fades its draw intensity
 * toward the fraction of samples that are not occluded. */
void __cdecl RB_TestFlare(flare_t *f)
{
  int     size;
  GLint   readX;
  GLint   readY;
  GLsizei readW;
  GLsizei readH;
  int     sampleArea;
  int     visibleCount;
  int     i;
  float   fade;
  float   pixels[4225]; /* [ebp-4204h] BYREF -- 65 * 65 depth samples */
  int    *finish;

  ++*(int *)&backEnd_pc_flareTests;               /* backEnd.pc.flareTestCount */

  if ( f->windowX + f->screenRadius >= 0
    && f->windowX - f->screenRadius < backEnd_viewParms_viewportWidth
    && f->windowY + f->screenRadius >= 0
    && f->windowY - f->screenRadius < backEnd_viewParms_viewportHeight )
  {
    /* via a pointer: retail's glState is address-taken, so this store may alias *f */
    finish = &glState_finishCalled;
    *finish = 0;                                   /* readpixels acts as a sync */
    size = 2 * f->screenRadius + 1;
    if ( size > 65 )
      size = 65;
    else if ( size < 1 )
      size = 1;
    readX = f->windowX - size / 2;
    readY = f->windowY - size / 2;
    readW = size;
    readH = size;
    sampleArea = size * size;
    if ( readX < 0 )
    {
      readW = readX + size;
      readX = 0;
    }
    else if ( size > backEnd_viewParms_viewportWidth - readX )
    {
      readW = backEnd_viewParms_viewportWidth - readX;
    }
    if ( readY < 0 )
    {
      readH = readY + size;
      readY = 0;
    }
    else if ( size > backEnd_viewParms_viewportHeight - readY )
    {
      readH = backEnd_viewParms_viewportHeight - readY;
    }
    visibleCount = 0;
    if ( readW > 0 && readH > 0 )
    {
      glReadPixels(readX, readY, readW, readH, 0x1902u, 0x1406u, pixels); /* GL_DEPTH_COMPONENT, GL_FLOAT */
      for ( i = 0; i < readW * readH; ++i )
        visibleCount += (f->eyeZ <= pixels[i]);
    }
    fade = (double)visibleCount / (double)sampleArea;
  }
  else
  {
    fade = 1.0;
    if ( (f->active & 1) == 0 )
      fade = 0.0;
  }

  if ( f->id == -1 )
    rendererSunFlareVisibility = fade;                            /* sun-flare visibility */
  f->drawIntensity = R_UpdateOverTime(
                       f->drawIntensity,
                       fade,
                       f->fadeInMsec,
                       f->fadeOutMsec,
                       backEnd_refdef_time - f->fadeTime);
  f->fadeTime = backEnd_refdef_time;
}

/* ---- RB_RenderFlare  0x004D94B0 ----  VERIFIED
 * Emits one screen-aligned quad for the flare into the tess buffers.  The
 * four vertex colors are a packed RGBA8888 word (rgb scaled by identityLight,
 * alpha from drawIntensity). */
void __cdecl RB_RenderFlare(flare_t *f)
{
  float  size;
  vec3_t color;
  int    packedColor;
  int    storage;
  float *xyz;
  float *st;
  int   *rgba;

  ++*((int *)&backEnd_pc_flareTests + 1);          /* backEnd.pc.flareRenderCount */

  VectorScale( f->color, tr_identityLight, color );
  ((unsigned char *)&packedColor)[0] = (unsigned __int64)(color[0] * 255.0f);
  ((unsigned char *)&packedColor)[1] = (unsigned __int64)(color[1] * 255.0f);
  ((unsigned char *)&packedColor)[2] = (unsigned __int64)(color[2] * 255.0f);
  ((unsigned char *)&packedColor)[3] = (unsigned __int64)((rfloat)f->drawIntensity * 255.0f);
  size = ( (rfloat)f->size * 0.0015625f ) * (double)backEnd_viewParms_viewportWidth;   /* parens: VC7 scheduling weight */ /* flt_568FE4 0x3ACCCCCD */

  backEnd_currentEntity = (int)&tr_worldEntity;
  storage = storageClass;
  if ( storageClass != glState_currentStorageMode )
  {
    if ( glConfig_NVVertexArrayRange )
      RB_SelectStorageNV(storageClass);
    else if ( glConfig_ATIVertexArrayObject )
      RB_SelectStorageATI(storageClass);
    glState_currentStorageMode = storage;
  }

  RB_BeginSurface(f->shader, 3);                   /* 3 = vertex component count */
  xyz  = &tess_xyz[tess_numVertexes * tess_vertexComponentCount];
  st   = &tess_texCoords0[2 * tess_numVertexes];
  rgba = &tess_vertexColorsPacked[tess_numVertexes];

  *xyz++ = f->windowX - size;
  *xyz++ = f->windowY - size;
  *xyz++ = r_znear->value;
  *st++ = 0.0f;
  *st++ = 0.0f;
  *rgba++ = packedColor;

  *xyz++ = f->windowX - size;
  *xyz++ = f->windowY + size;
  *xyz++ = r_znear->value;
  *st++ = 0.0f;
  *st++ = 1.0f;
  *rgba++ = packedColor;

  *xyz++ = f->windowX + size;
  *xyz++ = f->windowY + size;
  *xyz++ = r_znear->value;
  *st++ = 1.0f;
  *st++ = 1.0f;
  *rgba++ = packedColor;

  *xyz++ = f->windowX + size;
  *xyz++ = f->windowY - size;
  *xyz++ = r_znear->value;
  *st++ = 1.0f;
  *st++ = 0.0f;
  *rgba++ = packedColor;

  tess_indexes[tess_numIndexes++] = tess_numVertexes;
  tess_indexes[tess_numIndexes++] = tess_numVertexes + 1;
  tess_indexes[tess_numIndexes++] = tess_numVertexes + 2;
  tess_indexes[tess_numIndexes++] = tess_numVertexes;
  tess_indexes[tess_numIndexes++] = tess_numVertexes + 2;
  tess_indexes[tess_numIndexes++] = tess_numVertexes + 3;
  tess_numVertexes += 4;
  RB_EndSurface();
}

/* ---- RB_RenderFlares  0x004D9770 ----  VERIFIED
 * Once per view: reclaim/age active flares, run the occlusion test, then draw
 * the sun blind/glare overlay and every visible flare in an ortho pass. */
void RB_RenderFlares()
{
  flare_t  *f;
  flare_t **prev;
  flare_t  *rf;
  int       draw;
  float     glareFraction; /* RB_CalcSunBlind out A -- rgb white level */
  float     blindFraction; /* RB_CalcSunBlind out B -- overlay alpha */

  if ( !r_flares->integer )
    return;

  RB_AddDlightFlares();
  RB_AddCoronaFlares();

  rendererSunFlareVisibility = 0.0f;                              /* recomputed by RB_TestFlare for the sun */
  draw = 0;
  prev = (flare_t **)&rendererActiveFlares;               /* &r_activeFlares */
  while ( (f = *prev) != 0 )
  {
    /* drop any flare that was not added last frame */
    if ( f->addedFrame < backEnd_viewParms_frameCount )
    {
      if ( f->drawIntensity <= 0.0f )
      {
        *prev = f->next;
        f->next = (flare_t *)rendererFreeFlares;
        rendererFreeFlares = (int)f;
        continue;
      }
      f->active = 0;
      f->eyeZ = 2.0;                               /* 0x40000000 */
    }

    /* only test flares from this scene / view */
    if ( f->frameSceneNum == backEnd_viewParms_frameSceneNum
      && f->portalView == backEnd_wireframeOverride )
    {
      RB_TestFlare(f);
      if ( f->drawIntensity != 0.0f )
      {
        draw = 1;
      }
      else
      {
        /* fully faded out -- return it to the free list */
        *prev = f->next;
        f->next = (flare_t *)rendererFreeFlares;
        rendererFreeFlares = (int)f;
        continue;
      }
    }
    prev = &f->next;
  }

  RB_CalcSunBlind(&blindFraction, &glareFraction, rendererSunFlareVisibility);
  if ( !draw && blindFraction <= 0.0f && glareFraction <= 0.0f )   /* retail jp/jnp: a NaN fraction draws */
    return;

  if ( backEnd_wireframeOverride )
    glDisable(0x3000u);                          /* GL_CLIP_PLANE0 */
  qglPushMatrix();
  qglLoadIdentity();
  glMatrixMode(0x1701u);                          /* GL_PROJECTION */
  qglPushMatrix();
  qglLoadIdentity();
  qglOrtho(
    (double)backEnd_viewParms_viewportX,
    (double)(backEnd_viewParms_viewportX + backEnd_viewParms_viewportWidth),
    (double)backEnd_viewParms_viewportY,
    (double)(backEnd_viewParms_viewportY + backEnd_viewParms_viewportHeight),
    -99999.0,
    99999.0);

  /* full-screen sun blind / glare overlay */
  if ( blindFraction != 0.0f || glareFraction != 0.0f )
  {
    RB_BeginImmediateMode();
    GL_Bind((GLenum *)tr_whiteImage);
    GL_State(65634);                              /* 0x10062 */
    if ( glState_texEnv[glState_currentTmu] != 8448 )
    {
      glState_texEnv[glState_currentTmu] = 8448;  /* GL_MODULATE */
      glTexEnvi(0x2300u, 0x2200u, 8448);          /* GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE */
    }
    RB_glColor4f(glareFraction, glareFraction, glareFraction, blindFraction);
    RB_glBegin(7);                                /* GL_QUADS */
    RB_glVertex3f((float)backEnd_viewParms_viewportX, (float)backEnd_viewParms_viewportY, 0.0f);
    RB_glVertex3f((float)backEnd_viewParms_viewportX,
                  (float)(backEnd_viewParms_viewportY + backEnd_viewParms_viewportHeight), 0.0f);
    RB_glVertex3f((float)(backEnd_viewParms_viewportX + backEnd_viewParms_viewportWidth),
                  (float)(backEnd_viewParms_viewportY + backEnd_viewParms_viewportHeight), 0.0f);
    RB_glVertex3f((float)(backEnd_viewParms_viewportX + backEnd_viewParms_viewportWidth),
                  (float)backEnd_viewParms_viewportY, 0.0f);
    RB_glEnd();
    RB_EndImmediateMode();
  }

  if ( draw )
  {
    for ( rf = (flare_t *)rendererActiveFlares; rf; rf = rf->next )
    {
      if ( rf->frameSceneNum == backEnd_viewParms_frameSceneNum
        && rf->portalView == backEnd_wireframeOverride
        && rf->drawIntensity != 0.0f )
      {
        RB_RenderFlare(rf);
      }
    }
  }

  qglPopMatrix();
  glMatrixMode(0x1700u);                          /* GL_MODELVIEW */
  qglPopMatrix();
}
