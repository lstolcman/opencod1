/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_shaderregistry.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"

/* ---- renderer-owned globals ---- */
int glfogNum;                           /* 0x01407300 (RTCW tr_main.c glfogNum) */
/* the renderer import table (RTCW tr_main.c); GetRefAPI copies CL_InitRef's table into it */
refimport_t ri;
/* the fog table: glfog_t[9] at 0x016C4B00 (FOG_TABLE below; tr_records.h FOGBASE views) */
int tr_fogTable[144];
/* tr (RTCW tr_main.c trGlobals_t tr): 0x13BD4 bytes at 0x016C4D40, right after the fog table;
 * tr_registered[0] is tr.registered, tr_records.h TRG views address the rest */
int tr_registered[20213];



/* tr.sortedShaders -- shader lookup by the sort key's shader index */
#define tr_sortedShaders           ((shader_t **)tr_sortedShaderSlots)

#define TR_FRUSTUM_PLANES          4

/* The fog table is an array of glfog_t (64-byte stride).  Slots 0-4 are the
 * named fogs; the three transition slots below are proven by R_SetFrameFog's
 * blend (current = lerp(last, target)) and R_SetFog's switch handling. */
#define FOG_TABLE                  ((glfog_t *)(void *)tr_fogTable)
#define FOG_CURRENT                5
#define FOG_LAST                   6
#define FOG_TARGET                 7


void PerpendicularVector( float *dst, const float *src );
extern int PlaneFromPoints();
extern void __cdecl RB_BeginSurface( void *shader, int vertexComponentCount );
extern int RB_SelectStorageATI();
extern int RB_SelectStorageNV();
extern int R_AddBrushModelSurfaces();
extern int R_AddDrawSurfCmd();
extern int R_AddPolygonSurfaces();
extern int R_AddStaticModelSurfaces();
extern void R_AddWorldSurfacesDPVS();
extern int R_AddXModelSurfaces();
extern int R_SetPlaneSidesDPVS();
extern void R_ShowLightVisCachePoints();
extern float         xmodel_testLodDist;             /* 0x00A9CE10 */
extern unsigned char xmodel_testLodsEnabled;         /* 0x00A9CE14 */
extern float         xmodel_testLodDistances[15];    /* 0x00A9CE18 */
void RotatePointAroundVector( float *dst, const float *dir, const float *point, float degrees );
void AxisCopy( vec3_t in[3], vec3_t out[3] );

void  R_FogOn( void );
void  R_SetFogColor( void );
void  R_SetFrameFog( void );
int   R_CullLocalBox( vec3_t *bounds );
int   R_CullPointAndRadius( const float *pt, float radius );
float *R_LocalPointToWorld( const float *local, float *world );
void  myGlMultMatrix( const float *a, const float *b, float *out );
void  R_RotateForModelEntity( trRefEntity_t *ent, viewParms_t *viewParms, orientationr_t *entOr );
void  R_AddDrawSurf( int sortStorage, shader_t *shader, void *surface, int dlightBits, int fogIndex, int worldEntityFlag );
void  R_RenderView( viewParms_t *parms );
int   R_MirrorViewBySurface( drawSurf_t *drawSurf, int entityNum );
void  R_GenerateDrawSurfs( void );
void  R_PlaneForSurface( void *surf, float *plane );

#define rb_surfaceTable   ((rb_surfaceFunc_t *)(void *)funcs_4D4A37)

/* ---- R_Fog  0x004D28E0 ----  VERIFIED
 * Push one fog record into GL fog state.  r_zfar, when set, overrides the
 * fog's own far distance. */
void __cdecl R_Fog( glfog_t *fog )
{
  float fogEnd;

  if ( !r_fog->integer )
  {
    if ( (glState_glStateBits & 0x200000) != 0 )
    {
      glDisable( 0xB60u );                         /* GL_FOG */
      glState_glStateBits &= ~0x200000u;
    }
    return;
  }
  if ( !fog->registered )                          /* second inlined R_FogOff: VC7 merges it into the first */
  {
    if ( (glState_glStateBits & 0x200000) != 0 )
    {
      glDisable( 0xB60u );                         /* GL_FOG */
      glState_glStateBits &= ~0x200000u;
    }
    return;
  }

  if ( fog->density == 0.0f )
    fog->density = 1.0f;
  if ( !fog->hint )
    fog->hint = 4352;                              /* GL_DONT_CARE */
  if ( !fog->mode )
    fog->mode = 9729;                              /* GL_LINEAR */

  R_FogOn();

  if ( fog->mode != glState_fogMode )
  {
    qglFogi( 2917, fog->mode );                    /* GL_FOG_MODE */
    glState_fogMode = fog->mode;
  }

  glState_fogColor = fog->color[0];
  glState_fogColorG      = fog->color[1];
  glState_fogColorB      = fog->color[2];
  glState_fogColorA    = fog->color[3];
  R_SetFogColor();

  if ( fog->density != glState_fogDensity )
  {
    qglFogf( 2914, fog->density );                 /* GL_FOG_DENSITY */
    glState_fogDensity = fog->density;
  }
  if ( glState_fogHint != fog->hint )
  {
    qglHint( 3156, fog->hint );                    /* GL_FOG_HINT */
    glState_fogHint = fog->hint;
  }
  if ( glState_fogStart != fog->start )
  {
    qglFogf( 2915, fog->start );                   /* GL_FOG_START */
    glState_fogStart = fog->start;
  }

  fogEnd = ( r_zfar->value != 0.0f ) ? r_zfar->value : fog->end;
  if ( fogEnd != glState_fogEnd )
  {
    qglFogf( 2916, fogEnd );                       /* GL_FOG_END */
    glState_fogEnd = fogEnd;
  }
}


/* ---- R_FogOn  0x004D2A80 ----  VERIFIED */
void R_FogOn( void )
{
  int registered;

  if ( backEnd_projection2D || (glState_glStateBits & 0x200000) != 0 )
    return;
  if ( !r_fog->integer )
    return;

  registered = tr_fogPortalView_registered;
  if ( (backEnd_refdef_rdflags & 8) == 0 )
    registered = glfogNum;

  if ( registered )
  {
    glEnable( 0xB60u );                            /* GL_FOG */
    glState_glStateBits |= 0x200000u;
  }
}


/* ---- R_SetFogColor  0x004D2AD0 ----  VERIFIED */
void R_SetFogColor( void )
{
  float fogColor[4];

  if ( !glfogNum )
    return;

  if ( (glState_glStateBits & 0xF0) == 0x20 )
  {
    VectorClear( fogColor );
  }
  else
  {
    VectorScale( glState.fogColor, tr_identityLight, fogColor );
  }
  fogColor[3] = glState_fogColorA;
  qglFogfv( 2918, fogColor );                      /* GL_FOG_COLOR */
}

/* ---- R_SetFog  0x004D2B50 ----  [HIGH]
 * Register fog record `fogvar`, or (fogvar == FOG_CMD_SWITCHFOG == 8) start a
 * transition to fog `var1` over `var2` ms.  Slot 3 is the one cleared by a
 * FOG_NONE switch. */
void __cdecl R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density )
{


  if (fogvar != 8)
  {
if ( var1 || var2 )                         
  {

    FOG_TABLE[fogvar].color[0] = r;
    FOG_TABLE[fogvar].color[1] = g;
    FOG_TABLE[fogvar].color[2] = b;
    FOG_TABLE[fogvar].color[3] = 1.0f;
    FOG_TABLE[fogvar].start = (float)var1;
    FOG_TABLE[fogvar].end   = (float)var2;
    FOG_TABLE[fogvar].clearScreen = 0;
    FOG_TABLE[fogvar].drawSky     = 1;
    if (density >= 1.0f)
    {
      FOG_TABLE[fogvar].mode = 9729;
      FOG_TABLE[fogvar].density = 1.0f;
    }
    else
    {
      FOG_TABLE[fogvar].mode = 2048;
      FOG_TABLE[fogvar].density = density;
    }
    FOG_TABLE[fogvar].registered = 1;
    FOG_TABLE[fogvar].hint = 4352;                               
  }
  else
  {
    FOG_TABLE[fogvar].registered = 0;
  }
  }
  else
  {
    if ( var1 == 3 )                               
    {
      if ( FOG_TABLE[FOG_CURRENT].registered )
        FOG_TABLE[FOG_LAST] = FOG_TABLE[FOG_CURRENT];
      memset( &FOG_TABLE[3], 0, sizeof( glfog_t ) );
      memset( &FOG_TABLE[FOG_TARGET], 0, sizeof( glfog_t ) );
      glfogNum = 0;
    }
    else if ( FOG_TABLE[var1].registered == 1 )
    {
      glfogNum = var1;

      
      if ( FOG_TABLE[FOG_CURRENT].registered )
        FOG_TABLE[FOG_LAST] = FOG_TABLE[FOG_CURRENT];
      else
        FOG_TABLE[FOG_LAST] = FOG_TABLE[var1];
      FOG_TABLE[FOG_TARGET] = FOG_TABLE[var1];

      if (!var2)
      {
        FOG_TABLE[FOG_TARGET].dirty = 1;
        FOG_TABLE[FOG_CURRENT].dirty = 1;
        FOG_TABLE[FOG_TARGET].startTime = 0;
        FOG_TABLE[FOG_TARGET].finishTime = 0;
      }
      else
      {
        FOG_TABLE[FOG_TARGET].startTime = tr_refdef_time;
        FOG_TABLE[FOG_TARGET].finishTime = var2 + tr_refdef_time;
      }
    }
  
  }
}

/* ---- RE_SaveFogState  0x004D2CF0 ----  VERIFIED */
int __cdecl RE_SaveFogState( unsigned int *dst, unsigned int size )
{
  if ( size < 0x244 )
    ri.Error( 1, "couldn't save fog settings (%i bytes available, %i bytes needed)\n", size, 580 );
  memcpy( dst, tr_fogTable, 0x240u );
  dst[144] = glfogNum;
  return 580;
}

/* ---- RE_RestoreFogState  0x004D2D40 ----  VERIFIED */
int __cdecl RE_RestoreFogState( unsigned int *src, unsigned int size )
{
  if ( size < 0x244 )
    ri.Error( 1, "couldn't restore fog settings (savegame is probably corrupt or an old version)\n" );
  memcpy( tr_fogTable, src, 0x240u );
  glfogNum = src[144];
  return 580;
}

/* ---- R_UpdateOverTime  0x004D2D90 ----  VERIFIED
 * Move `current` toward `target`, rate-limited by the fade-in / fade-out
 * time, clamping so it never overshoots. */
double __cdecl R_UpdateOverTime( float current, float target, int fadeInTime, int fadeOutTime, int elapsed )
{
  if ( current < target )
  {
    if ( fadeInTime <= 0 )
      return target;
    current = (double)elapsed / (double)fadeInTime + current;
    if ( current > target )
      return target;
  }
  else if ( current > target )
  {
    if ( fadeOutTime <= 0 )
      return target;
#ifdef COD1_VC7
    current = current - (float)elapsed / (float)fadeOutTime;
#else
    current = current - (double)elapsed / (double)fadeOutTime;
#endif
    if ( current < target )
      return target;
  }
  return current;
}

/* ---- R_CullLocalBox  0x004D2E00 ----  VERIFIED
 * Frustum-cull a local-space AABB.  Retail unrolls the 8-corner inner loop;
 * the plain RTCW loop is equivalent. */
int __cdecl R_CullLocalBox( vec3_t *bounds )
{
  int       i, j;
  vec3_t    transformed[8];
  vec3_t    v;
  float     dists[8];
  cplane_t *frust;
  int       anyBack, front, back;

  if ( r_nocull->integer )
    return CULL_CLIP;

  /* transform the 8 corners into world space */
  for ( i = 0; i < 8; ++i )
  {
    v[0] = bounds[i & 1][0];
    v[1] = bounds[(i >> 1) & 1][1];
    v[2] = bounds[(i >> 2) & 1][2];

    VectorCopy( trOrient.origin, transformed[i] );
    VectorMA( transformed[i], v[0], trOrient.axis[0], transformed[i] );
    VectorMA( transformed[i], v[1], trOrient.axis[1], transformed[i] );
    VectorMA( transformed[i], v[2], trOrient.axis[2], transformed[i] );
  }

  /* test against the frustum planes */
  anyBack = 0;
  for ( i = 0; i < TR_FRUSTUM_PLANES; ++i )
  {
    frust = &trView.frustum[i];
    front = back = 0;
    for ( j = 0; j < 8; ++j )
    {
      dists[j] = DotProduct( transformed[j], frust->normal );
      if ( dists[j] > frust->dist )
      {
        front = 1;
        if ( back )
          break;
      }
      else
      {
        back = 1;
      }
    }
    if ( !front )
      return CULL_OUT;
    anyBack |= back;
  }

  if ( !anyBack )
    return CULL_IN;
  return CULL_CLIP;
}


/* ---- R_CullLocalPointAndRadius  0x004D3100 ----  VERIFIED */
int __cdecl R_CullLocalPointAndRadius( const float *pt, float radius )
{
  vec3_t world;

  R_LocalPointToWorld( pt, world );
  return R_CullPointAndRadius( world, radius );
}

/* ---- R_CullPointAndRadius  0x004D3120 ----  VERIFIED */
int __cdecl R_CullPointAndRadius( const float *pt, float radius )
{
  int       i;
  cplane_t *frust;
  rfloat    dist;             /* retail keeps dist on the x87 stack, never rounded to float */
  int       mightBeClipped;

  mightBeClipped = 0;
  if ( r_nocull->integer )
    return CULL_CLIP;

  for ( i = 0; i < TR_FRUSTUM_PLANES; ++i )
  {
    frust = &trView.frustum[i];
    /* retail 0x4D3147 sums (x + z) + y: VC7 reassociates the float-typed dot product */
#ifdef COD1_VC7
    dist = DotProduct( pt, frust->normal ) - frust->dist;
#else
    dist = (double)pt[0] * frust->normal[0] + (double)pt[2] * frust->normal[2]
         + (double)pt[1] * frust->normal[1] - frust->dist;
#endif
    if ( dist < -radius )
      return CULL_OUT;
    if ( dist <= radius )
      mightBeClipped = 1;
  }

  return mightBeClipped ? CULL_CLIP : CULL_IN;
}


/* ---- R_LocalNormalToWorld  0x004D31A0 ----  VERIFIED */
float *__cdecl R_LocalNormalToWorld( const float *local, float *world )
{
  world[0] = trOrient.axis[0][0] * local[0] + trOrient.axis[1][0] * local[1] + trOrient.axis[2][0] * local[2];
  world[1] = trOrient.axis[0][1] * local[0] + trOrient.axis[1][1] * local[1] + trOrient.axis[2][1] * local[2];
  world[2] = trOrient.axis[0][2] * local[0] + trOrient.axis[1][2] * local[1] + trOrient.axis[2][2] * local[2];
  return (float *)local;
}

/* ---- R_LocalPointToWorld  0x004D3210 ----  VERIFIED */
float *__cdecl R_LocalPointToWorld( const float *local, float *world )
{
  world[0] = trOrient.axis[0][0] * local[0] + trOrient.axis[1][0] * local[1] + trOrient.axis[2][0] * local[2] + trOrient.origin[0];
  world[1] = trOrient.axis[0][1] * local[0] + trOrient.axis[1][1] * local[1] + trOrient.axis[2][1] * local[2] + trOrient.origin[1];
  world[2] = trOrient.axis[0][2] * local[0] + trOrient.axis[1][2] * local[1] + trOrient.axis[2][2] * local[2] + trOrient.origin[2];
  return (float *)local;
}

/* ---- R_WorldToLocal  0x004D3290 ----  VERIFIED */
float *__cdecl R_WorldToLocal( const float *world, float *local )
{
  local[0] = trOrient.axis[0][0] * world[0] + trOrient.axis[0][1] * world[1] + trOrient.axis[0][2] * world[2];
  local[1] = trOrient.axis[1][0] * world[0] + trOrient.axis[1][1] * world[1] + trOrient.axis[1][2] * world[2];
  local[2] = trOrient.axis[2][0] * world[0] + trOrient.axis[2][1] * world[1] + trOrient.axis[2][2] * world[2];
  return (float *)world;
}

/* ---- R_TransformModelToClip  0x004D3300 ----  VERIFIED
 * eye = modelMatrix * src ; dst = projectionMatrix * eye. */
void __cdecl R_TransformModelToClip(
        const float *src,
        const float *modelMatrix,
        const float *projectionMatrix,
        float       *eye,
        float       *dst )
{
  int i;

  for ( i = 0; i < 4; i++ )
  {
    eye[i] =
      src[0] * modelMatrix[i + 0 * 4] +
      src[1] * modelMatrix[i + 1 * 4] +
      src[2] * modelMatrix[i + 2 * 4] +
      1 * modelMatrix[i + 3 * 4];
  }

  for ( i = 0; i < 4; i++ )
  {
    dst[i] =
      eye[0] * projectionMatrix[i + 0 * 4] +
      eye[1] * projectionMatrix[i + 1 * 4] +
      eye[2] * projectionMatrix[i + 2 * 4] +
      eye[3] * projectionMatrix[i + 3 * 4];
  }
}

/* ---- R_TransformHomogenousModelToClip  0x004D33F0 ----  VERIFIED
 * As R_TransformModelToClip, but src is a homogeneous vec4. */
float *__cdecl R_TransformHomogenousModelToClip(
        const float *src,
        const float *modelMatrix,
        const float *projectionMatrix,
        float       *eye,
        float       *dst )
{
  int i;

  /* Q3 R_TransformModelToClip loops (src-first products) and Q3 parameter order
     (src, modelMatrix, projectionMatrix, eye, dst), as retail's register use shows. */
  for ( i = 0; i < 4; i++ )
    eye[i] = src[0] * modelMatrix[i + 0 * 4] + src[1] * modelMatrix[i + 1 * 4] + src[2] * modelMatrix[i + 2 * 4] + src[3] * modelMatrix[i + 3 * 4];
  for ( i = 0; i < 4; i++ )
    dst[i] = eye[0] * projectionMatrix[i + 0 * 4] + eye[1] * projectionMatrix[i + 1 * 4] + eye[2] * projectionMatrix[i + 2 * 4] + eye[3] * projectionMatrix[i + 3 * 4];
  return eye;
}

/* ---- R_TransformClipToWindow  0x004D34F0 ----  VERIFIED */
void __cdecl R_TransformClipToWindow( const float *clip, viewParms_t *view, float *normalized, float *window )
{
  normalized[0] = clip[0] / clip[3];
  normalized[1] = clip[1] / clip[3];
  normalized[2] = ( clip[2] + clip[3] ) / ( clip[3] + clip[3] );

#ifdef COD1_VC7
  window[0] = ( normalized[0] + 1.0f ) * view->viewportWidth * 0.5f;
  window[1] = view->viewportHeight * ( normalized[1] + 1.0f ) * 0.5f;
  window[2] = normalized[2];

  window[0] = floor( window[0] + 0.5f );
  window[1] = floor( window[1] + 0.5f );
#else
  window[0] = ( normalized[0] + 1.0f ) * (double)view->viewportWidth  * 0.5;
  window[1] = ( normalized[1] + 1.0f ) * (double)view->viewportHeight * 0.5;
  window[2] = normalized[2];

  window[0] = floor( window[0] + 0.5 );
  window[1] = floor( window[1] + 0.5 );
#endif
}

/* ---- myGlMultMatrix  0x004D3580 ----  VERIFIED
 * out = a * b, row-major 4x4.  Retail (0x4D3590) sums the four terms on the
 * x87 stack in reverse order, ((3 + 2) + 1) + 0, rounding only on the store:
 * VC7 reassociates the float-typed sum that way; the #else spells it out. */
void __cdecl myGlMultMatrix( const float *a, const float *b, float *out )
{
  int i, j;

  for ( i = 0; i < 4; ++i )
  {
    for ( j = 0; j < 4; ++j )
    {
#ifdef COD1_VC7
      out[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j]
                     + a[i * 4 + 1] * b[1 * 4 + j]
                     + a[i * 4 + 2] * b[2 * 4 + j]
                     + a[i * 4 + 3] * b[3 * 4 + j];
#else
      out[i * 4 + j] = (float)( (double)a[i * 4 + 3] * b[3 * 4 + j]
                              + (double)a[i * 4 + 2] * b[2 * 4 + j]
                              + (double)a[i * 4 + 1] * b[1 * 4 + j]
                              + (double)a[i * 4 + 0] * b[0 * 4 + j] );
#endif
    }
  }
}

/* RTCW's q_math.c Q_fabs: retail carries its 0x7FFFFFFF mask inline. */
static __inline float Q_fabs( float f )
{
  int tmp = *(int *)&f & 0x7FFFFFFF;
  return *(float *)&tmp;
}

/* ---- R_RotateForModelEntity  0x004D3630 ----  VERIFIED
 * Build the model orientation for a model-space entity and set the GL
 * normal-rescale mode from its axis scaling. */
void __cdecl R_RotateForModelEntity( trRefEntity_t *ent, viewParms_t *viewParms, orientationr_t *entOr )
{
  float  glMatrix[16];
  vec3_t delta;
  vec3_t axisLength;
  float  len;
  int    i;

  VectorCopy( ent->e.origin, entOr->origin );
  entOr->axis[0][0] = ent->e.axis[0][0];
  entOr->axis[0][1] = ent->e.axis[0][1];
  entOr->axis[0][2] = ent->e.axis[0][2];
  entOr->axis[1][0] = ent->e.axis[1][0];
  entOr->axis[1][1] = ent->e.axis[1][1];
  entOr->axis[1][2] = ent->e.axis[1][2];
  entOr->axis[2][0] = ent->e.axis[2][0];
  entOr->axis[2][1] = ent->e.axis[2][1];
  entOr->axis[2][2] = ent->e.axis[2][2];

  glMatrix[0]  = entOr->axis[0][0];
  glMatrix[4]  = entOr->axis[1][0];
  glMatrix[8]  = entOr->axis[2][0];
  glMatrix[12] = entOr->origin[0];

  glMatrix[1]  = entOr->axis[0][1];
  glMatrix[5]  = entOr->axis[1][1];
  glMatrix[9]  = entOr->axis[2][1];
  glMatrix[13] = entOr->origin[1];

  glMatrix[2]  = entOr->axis[0][2];
  glMatrix[6]  = entOr->axis[1][2];
  glMatrix[10] = entOr->axis[2][2];
  glMatrix[14] = entOr->origin[2];

  glMatrix[3]  = 0.0f;
  glMatrix[7]  = 0.0f;
  glMatrix[11] = 0.0f;
  glMatrix[15] = 1.0f;

  myGlMultMatrix( glMatrix, viewParms->world.modelMatrix, entOr->modelMatrix );

  /* viewOrigin = camera origin expressed in the entity's local frame */
  VectorSubtract( viewParms->orientation.origin, entOr->origin, delta );
  entOr->viewOrigin[0] = DotProduct( delta, entOr->axis[0] );
  entOr->viewOrigin[1] = DotProduct( delta, entOr->axis[1] );
  entOr->viewOrigin[2] = DotProduct( delta, entOr->axis[2] );

  ent->normalizationTarget = 0;
  if ( *(float *)&ent->e.nonNormalizedAxes )        /* retail tests the slot as a float */
  {
    for ( i = 0; i < 3; ++i )
    {
      len = (float)sqrt( DotProduct( ent->e.axis[i], ent->e.axis[i] ) );
      axisLength[i] = len;
      if ( axisLength[i] != 0.0f )
        entOr->viewOrigin[i] = entOr->viewOrigin[i] / axisLength[i];
    }

    ent->normalizationTarget = 2977;               /* GL_NORMALIZE */

    if ( glConfig_rescaleNormal
      && Q_fabs( axisLength[2] - axisLength[0] ) < 0.001f
      && Q_fabs( axisLength[2] - axisLength[1] ) < 0.001f )
    {
      ent->normalizationTarget = 32826;            /* GL_RESCALE_NORMAL_EXT */
    }
  }
}

/* ---- R_RotateForEntity  0x004D3850 ----  VERIFIED */
void __cdecl R_RotateForEntity( trRefEntity_t *ent, viewParms_t *viewParms, orientationr_t *entOr )
{
  int reType = ent->e.reType;

  if ( reType == 1 || reType == 2 || reType == 0 )
    R_RotateForModelEntity( ent, viewParms, entOr );
  else
    *entOr = viewParms->world;
}

/* ---- R_RotateForViewer  0x004D3880 ----  [HIGH]
 * Build tr.or for the camera itself (world-to-eye), then flip into GL's
 * looking-down-(-Z) convention. */
void R_RotateForViewer( void )
{
  float  viewerMatrix[16];
  vec3_t origin;

  memset( &trOrient, 0, sizeof( orientationr_t ) );
  trOrient.axis[0][0] = 1.0f;
  trOrient.axis[1][1] = 1.0f;
  trOrient.axis[2][2] = 1.0f;
  VectorCopy( trView.orientation.origin, trOrient.viewOrigin );

  VectorCopy( trView.orientation.origin, origin );

  /* retail 0x4D38D8: VC7 sums the Q3 translation as -((z + y) + x) on the x87 stack;
   * the #else spells that out for the modern build */
  viewerMatrix[0]  = trView.orientation.axis[0][0];
  viewerMatrix[4]  = trView.orientation.axis[0][1];
  viewerMatrix[8]  = trView.orientation.axis[0][2];
#ifdef COD1_VC7
  viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];
#else
  viewerMatrix[12] = (float)-( (double)origin[2] * viewerMatrix[8] + (double)origin[1] * viewerMatrix[4]
                             + (double)origin[0] * viewerMatrix[0] );
#endif

  viewerMatrix[1]  = trView.orientation.axis[1][0];
  viewerMatrix[5]  = trView.orientation.axis[1][1];
  viewerMatrix[9]  = trView.orientation.axis[1][2];
#ifdef COD1_VC7
  viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];
#else
  viewerMatrix[13] = (float)-( (double)origin[2] * viewerMatrix[9] + (double)origin[1] * viewerMatrix[5]
                             + (double)origin[0] * viewerMatrix[1] );
#endif

  viewerMatrix[2]  = trView.orientation.axis[2][0];
  viewerMatrix[6]  = trView.orientation.axis[2][1];
  viewerMatrix[10] = trView.orientation.axis[2][2];
#ifdef COD1_VC7
  viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];
#else
  viewerMatrix[14] = (float)-( (double)origin[2] * viewerMatrix[10] + (double)origin[1] * viewerMatrix[6]
                             + (double)origin[0] * viewerMatrix[2] );
#endif

  viewerMatrix[3]  = 0.0f;
  viewerMatrix[7]  = 0.0f;
  viewerMatrix[11] = 0.0f;
  viewerMatrix[15] = 1.0f;

  myGlMultMatrix( viewerMatrix, s_flipMatrix, trOrient.modelMatrix );

  trView.world = trOrient;
}

/* ---- R_SetFrameFog  0x004D39E0 ----  VERIFIED
 * Advance any active fog transition for this frame and, for a far-clip fog,
 * pull the view's zFar in to the fog end.  The table is indexed directly (no
 * slot pointers): that is what makes VC7 keep 0 and 1 in ebp/ebx as retail does. */
void R_SetFrameFog( void )
{
  rfloat   lerpPos;
  int      fadeTime;

  if ( r_speeds->integer == 5 )
  {
    if ( !FOG_TABLE[FOG_TARGET].registered )
    {
      ri.Printf( 0, "no fog - calc zFar: %0.1f\n", tr_viewParms_zFar );
      return;
    }
  }

  if ( FOG_TABLE[FOG_TARGET].finishTime && FOG_TABLE[FOG_TARGET].finishTime >= tr_refdef_time )
  {
    /* still fading; a fog mode change snaps instead of blending */
    if ( FOG_TABLE[FOG_LAST].mode == 2048 && FOG_TABLE[FOG_TARGET].mode == 9729 )
    {
      FOG_TABLE[FOG_CURRENT] = FOG_TABLE[FOG_TARGET];
      FOG_TABLE[FOG_TARGET].finishTime = 0;
    }
    else if ( FOG_TABLE[FOG_LAST].mode == 9729 && FOG_TABLE[FOG_TARGET].mode == 2048 )
    {
      FOG_TABLE[FOG_CURRENT] = FOG_TABLE[FOG_TARGET];
      FOG_TABLE[FOG_TARGET].finishTime = 0;
    }
    else
    {
      fadeTime = FOG_TABLE[FOG_TARGET].finishTime - FOG_TABLE[FOG_TARGET].startTime;
      if ( fadeTime <= 0 )
        fadeTime = 1;
      lerpPos = (rfloat)( tr_refdef_time - FOG_TABLE[FOG_TARGET].startTime ) / (rfloat)fadeTime;
      if ( lerpPos > 1.0f )
        lerpPos = 1.0f;

      FOG_TABLE[FOG_CURRENT].mode = FOG_TABLE[FOG_TARGET].mode;
      FOG_TABLE[FOG_CURRENT].registered = 1;
      FOG_TABLE[FOG_CURRENT].start    = ( FOG_TABLE[FOG_TARGET].start    - FOG_TABLE[FOG_LAST].start )    * lerpPos + FOG_TABLE[FOG_LAST].start;
      FOG_TABLE[FOG_CURRENT].end      = ( FOG_TABLE[FOG_TARGET].end      - FOG_TABLE[FOG_LAST].end )      * lerpPos + FOG_TABLE[FOG_LAST].end;
      FOG_TABLE[FOG_CURRENT].density  = ( FOG_TABLE[FOG_TARGET].density  - FOG_TABLE[FOG_LAST].density )  * lerpPos + FOG_TABLE[FOG_LAST].density;
      FOG_TABLE[FOG_CURRENT].color[0] = ( FOG_TABLE[FOG_TARGET].color[0] - FOG_TABLE[FOG_LAST].color[0] ) * lerpPos + FOG_TABLE[FOG_LAST].color[0];
      FOG_TABLE[FOG_CURRENT].color[1] = ( FOG_TABLE[FOG_TARGET].color[1] - FOG_TABLE[FOG_LAST].color[1] ) * lerpPos + FOG_TABLE[FOG_LAST].color[1];
      FOG_TABLE[FOG_CURRENT].color[2] = ( FOG_TABLE[FOG_TARGET].color[2] - FOG_TABLE[FOG_LAST].color[2] ) * lerpPos + FOG_TABLE[FOG_LAST].color[2];

      FOG_TABLE[FOG_CURRENT].clearScreen = ( FOG_TABLE[FOG_TARGET].clearScreen || FOG_TABLE[FOG_LAST].clearScreen );
    }
    FOG_TABLE[FOG_CURRENT].dirty = 1;
  }
  else
  {
    /* transition finished (or none) -- current becomes the target */
    FOG_TABLE[FOG_CURRENT] = FOG_TABLE[FOG_TARGET];
    FOG_TABLE[FOG_CURRENT].dirty = 0;
  }

  if ( FOG_TABLE[FOG_CURRENT].mode == 9729 && FOG_TABLE[FOG_CURRENT].end < tr_viewParms_zFar )
    tr_viewParms_zFar = FOG_TABLE[FOG_CURRENT].end;

  if ( r_speeds->integer == 5 )
  {
    if ( FOG_TABLE[FOG_CURRENT].mode == 9729 )
      ri.Printf( 0, "farclip fog - den: %0.1f  calc zFar: %0.1f  fog zfar: %0.1f\n", FOG_TABLE[FOG_CURRENT].density, tr_viewParms_zFar, FOG_TABLE[FOG_CURRENT].end );
    else
      ri.Printf( 0, "density fog - den: %0.6f  calc zFar: %0.1f  fog zFar: %0.1f\n", FOG_TABLE[FOG_CURRENT].density, tr_viewParms_zFar, FOG_TABLE[FOG_CURRENT].end );
  }
}


/* ---- SetFarClip  0x004D3C40 ----  VERIFIED */
void SetFarClip( void )
{
  if ( (tr_refdef_rdflags & 1) != 0 )
  {
    tr_viewParms_zFar = 2048.0f;
  }
  else if ( r_zfar->value != 0.0f )
  {
    tr_viewParms_zFar = r_zfar->value;
    R_SetFrameFog();
    if ( r_speeds->integer == 5 )
      ri.Printf( 0, "r_zfar value forcing farclip at: %f\n", tr_viewParms_zFar );
  }
  else
  {
    tr_viewParms_zFar = 524288.0f;
    R_SetFrameFog();
  }
}


/* ---- R_SetupProjection  0x004D3CB0 ----  VERIFIED
 * Infinite far-plane projection: zFar never enters the matrix (SetFarClip
 * still runs for the far-clip / fog state).  The frustum is built at unit
 * distance (x/y scale by the plain half-angle tangents), [10] = [11] = -1 and
 * [14] = -2 * r_znear.  The result is then copied into
 * depthHackProjectionMatrix with [14] taken from r_znear_depthhack instead. */
void R_SetupProjection( void )
{
#ifdef COD1_VC7
  rfloat zNear, zNearDepthHack;
  rfloat xmin, xmax, ymin, ymax;
  rfloat width, height;
#else
  float  zNear;
  double ymax, xmax;
  float  height;              /* retail spills height to a float local */
  double invWidth, invHeight;
#endif

  SetFarClip();

  zNear = r_znear->value;

  trView.projectionMatrix[4]  = 0.0f;
  trView.projectionMatrix[12] = 0.0f;
  trView.projectionMatrix[1]  = 0.0f;
  trView.projectionMatrix[13] = 0.0f;
  trView.projectionMatrix[2]  = 0.0f;
  trView.projectionMatrix[6]  = 0.0f;
  trView.projectionMatrix[10] = -1.0f;
  trView.projectionMatrix[3]  = 0.0f;
  trView.projectionMatrix[7]  = 0.0f;
  trView.projectionMatrix[11] = -1.0f;
  trView.projectionMatrix[15] = 0.0f;

#ifdef COD1_VC7
  /* VC7 divides by width/height through one reciprocal each (2 / w -> r + r) and
   * spills height to a float slot; the #else spells that out for the modern build */
  ymax = tan( tr_refdef_fov_y * 0.0087266462f );
  ymin = -ymax;
  xmax = tan( tr_refdef_fov_x * 0.0087266462f );
  xmin = -xmax;

  width  = xmax - xmin;
  height = ymax - ymin;

  trView.projectionMatrix[0] = 2.0f / width;
  trView.projectionMatrix[8] = ( xmax + xmin ) / width;
  trView.projectionMatrix[5] = 2.0f / height;
  trView.projectionMatrix[9] = ( ymax + ymin ) / height;
  trView.projectionMatrix[14] = zNear * -2.0f;
  zNearDepthHack = r_znear_depthhack->value;

  memcpy( trView.depthHackProjectionMatrix, trView.projectionMatrix, 0x40u );
  trView.depthHackProjectionMatrix[14] = zNearDepthHack * -2.0f;
#else
  /* float*float is exact in double, so this equals retail's fld/fmul m32 register value */
  ymax = tan( (double)tr_refdef_fov_y * (double)0.0087266462f );
  xmax = tan( (double)tr_refdef_fov_x * (double)0.0087266462f );

  height   = (float)( ymax - -ymax );
  invWidth = 1.0 / ( xmax - -xmax );
  trView.projectionMatrix[0] = (float)( invWidth + invWidth );
  trView.projectionMatrix[8] = (float)( ( -xmax + xmax ) * invWidth );
  invHeight = 1.0 / height;
  trView.projectionMatrix[5] = (float)( invHeight + invHeight );
  trView.projectionMatrix[9] = (float)( ( -ymax + ymax ) * invHeight );
  trView.projectionMatrix[14] = zNear * -2.0f;

  memcpy( trView.depthHackProjectionMatrix, trView.projectionMatrix, 0x40u );
  trView.depthHackProjectionMatrix[14] = r_znear_depthhack->value * -2.0f;
#endif
}

/* ---- R_SetupFrustum  0x004D3DE0 ----  VERIFIED */
/* Retail's inlined fsincos block (0x004D3E09 / 0x004D3F2B): an inline-asm
 * SinCos local to this file, like tr_surface.c's RB_SinCos. */
static __inline void R_SinCos( float radians, float *sinOut, float *cosOut )
{
  __asm {
    push    eax
    push    ecx
    fld     radians
    fsincos
    mov     ecx, cosOut
    mov     eax, sinOut
    fstp    dword ptr [ecx]
    fstp    dword ptr [eax]
    pop     ecx
    pop     eax
  }
}

void R_SetupFrustum( void )
{
  int   i;
  float xs, xc;
  float ang;

  ang = tr_viewParms_fovX / 180.0f * 1.5707964f;   /* fovX / 180 * M_PI * 0.5f */
  R_SinCos( ang, &xs, &xc );

  VectorScale( trView.orientation.axis[0], xs, trView.frustum[0].normal );
  VectorMA( trView.frustum[0].normal, xc, trView.orientation.axis[1], trView.frustum[0].normal );

  VectorScale( trView.orientation.axis[0], xs, trView.frustum[1].normal );
  VectorMA( trView.frustum[1].normal, -xc, trView.orientation.axis[1], trView.frustum[1].normal );

  ang = tr_viewParms_fovY / 180.0f * 1.5707964f;
  R_SinCos( ang, &xs, &xc );

  VectorScale( trView.orientation.axis[0], xs, trView.frustum[2].normal );
  VectorMA( trView.frustum[2].normal, xc, trView.orientation.axis[2], trView.frustum[2].normal );

  VectorScale( trView.orientation.axis[0], xs, trView.frustum[3].normal );
  VectorMA( trView.frustum[3].normal, -xc, trView.orientation.axis[2], trView.frustum[3].normal );

  for ( i = 0; i < TR_FRUSTUM_PLANES; i++ )
  {
    trView.frustum[i].type = 3;               /* PLANE_NON_AXIAL */
    trView.frustum[i].dist = DotProduct( trView.orientation.origin, trView.frustum[i].normal );
    SetPlaneSignbits( &trView.frustum[i] );
  }
}

/* ---- R_MirrorPoint  0x004D40A0 ----  [HIGH]
 * Map a world point across the portal: express it in the surface frame, then
 * rebuild it in the camera frame.
 * Retail (0x4D40A0, VC7 on the Q3 loop) keeps local and each dot product
 * unrounded on the x87 stack, sums them (z + y) + x, does not add the cleared 0,
 * spills the second d to a float for its [1]/[2] terms only, and never rounds
 * transformed[0] before the origin add; the #else spells that out. */
void __cdecl R_MirrorPoint( const float *in, orientation_t *surface, orientation_t *camera, float *out )
{
#ifdef COD1_VC7
  int    i;
  vec3_t local;
  vec3_t transformed;
  rfloat d;

  VectorSubtract( in, surface->origin, local );

  VectorClear( transformed );
  for ( i = 0; i < 3; i++ )
  {
    d = DotProduct( local, surface->axis[i] );
    VectorMA( transformed, d, camera->axis[i], transformed );
  }

  VectorAdd( transformed, camera->origin, out );
#else
  double local0, local1, local2;
  double d0, d1, d2, sum0;
  float  d1f;
  vec3_t transformed;

  local0 = (double)in[0] - surface->origin[0];
  local1 = (double)in[1] - surface->origin[1];
  local2 = (double)in[2] - surface->origin[2];

  d0   = local2 * surface->axis[0][2] + local1 * surface->axis[0][1] + local0 * surface->axis[0][0];
  sum0 = d0 * camera->axis[0][0];
  transformed[1] = (float)( d0 * camera->axis[0][1] );
  transformed[2] = (float)( d0 * camera->axis[0][2] );

  d1  = local2 * surface->axis[1][2] + local1 * surface->axis[1][1] + local0 * surface->axis[1][0];
  d1f = (float)d1;
  transformed[0] = (float)( d1 * camera->axis[1][0] + sum0 );
  transformed[1] = (float)( (double)d1f * camera->axis[1][1] + transformed[1] );
  transformed[2] = (float)( (double)d1f * camera->axis[1][2] + transformed[2] );

  d2   = local2 * surface->axis[2][2] + local1 * surface->axis[2][1] + local0 * surface->axis[2][0];
  sum0 = d2 * camera->axis[2][0] + transformed[0];
  transformed[1] = (float)( d2 * camera->axis[2][1] + transformed[1] );
  transformed[2] = (float)( d2 * camera->axis[2][2] + transformed[2] );

  out[0] = (float)( sum0 + camera->origin[0] );
  out[1] = (float)( (double)transformed[1] + camera->origin[1] );
  out[2] = (float)( (double)transformed[2] + camera->origin[2] );
#endif
}

/* ---- R_MirrorVector  0x004D4170 ----  VERIFIED
 * Retail keeps each dot product unrounded on the x87 stack, sums it (z + y) + x,
 * and stores the first axis term without adding the cleared 0 (VC7 does both to
 * the Q3 loop; the #else spells that out for the modern build). */
void __cdecl R_MirrorVector( const float *in, orientation_t *surface, orientation_t *camera, float *out )
{
#ifdef COD1_VC7
  int    i;
  rfloat d;

  VectorClear( out );
  for ( i = 0; i < 3; i++ )
  {
    d = DotProduct( in, surface->axis[i] );
    VectorMA( out, d, camera->axis[i], out );
  }
#else
  double d;

  VectorClear( out );

  d = (double)surface->axis[0][2] * in[2] + (double)surface->axis[0][1] * in[1] + (double)surface->axis[0][0] * in[0];
  out[0] = (float)( d * camera->axis[0][0] );
  out[1] = (float)( d * camera->axis[0][1] );
  out[2] = (float)( d * camera->axis[0][2] );

  d = (double)surface->axis[1][2] * in[2] + (double)surface->axis[1][1] * in[1] + (double)surface->axis[1][0] * in[0];
  out[0] = (float)( d * camera->axis[1][0] + out[0] );
  out[1] = (float)( d * camera->axis[1][1] + out[1] );
  out[2] = (float)( d * camera->axis[1][2] + out[2] );

  d = (double)surface->axis[2][2] * in[2] + (double)surface->axis[2][1] * in[1] + (double)surface->axis[2][0] * in[0];
  out[0] = (float)( d * camera->axis[2][0] + out[0] );
  out[1] = (float)( d * camera->axis[2][1] + out[1] );
  out[2] = (float)( d * camera->axis[2][2] + out[2] );
#endif
}

/* ---- R_PlaneForSurface  0x004D4220 ----  VERIFIED
 * Recover the plane of a surface.  Face surfaces store it directly; triangle
 * and grid surfaces derive it from three vertices. */
void __cdecl R_PlaneForSurface( void *surf, float *plane )
{
  int   surfaceType;
  float plane4[4];

  if ( !surf )
  {
    memset( plane, 0, sizeof( cplane_t ) );
    plane[0] = 1.0f;
    return;
  }

  surfaceType = *(int *)surf;
  if ( surfaceType >= SF_INDEXED_FIRST )
  {
    /* indexed world face: three verts picked from the index list */
    PlaneFromPoints(
      plane4,
      *(float **)( (char *)surf + 64 ),
      (float *)( *(int *)( (char *)surf + 64 ) + 12 * ( *(unsigned __int16 *)( (char *)surf + 74 ) - *(unsigned __int16 *)( (char *)surf + 72 ) ) ),
      (float *)( *(int *)( (char *)surf + 64 ) + 12 * ( *(unsigned __int16 *)( (char *)surf + 76 ) - *(unsigned __int16 *)( (char *)surf + 72 ) ) ) );
    VectorCopy( plane4, plane );
    plane[3] = plane4[3];
    return;
  }

  if ( surfaceType == 2 )                          /* triangle-soup surface */
  {
    PlaneFromPoints(
      plane4,
      *(float **)( (char *)surf + 12 ),
      (float *)( *(int *)( (char *)surf + 12 ) + 32 ),
      (float *)( *(int *)( (char *)surf + 12 ) + 64 ) );
    VectorCopy( plane4, plane );
    plane[3] = plane4[3];
    return;
  }

  memset( plane, 0, sizeof( cplane_t ) );
  plane[0] = 1.0f;
}

/* ---- R_GetPortalOrientations  0x004D42F0 ----  VERIFIED
 * Find the matching RT_PORTALSURFACE entity for the portal surface and build
 * the surface/camera orientations used to render the portal view. */
int __cdecl R_GetPortalOrientations( int entityNum, int drawSurf, orientation_t *surface, orientation_t *camera,
                                     unsigned int *pvsOrigin, unsigned int *isMirror )
{
  int            i;
  cplane_t       originalPlane, plane;
  trRefEntity_t *e;
  float          d;
  vec3_t         transformed;

  /* create plane axis for the portal we are seeing */
  R_PlaneForSurface( *(void **)( drawSurf + 4 ), (float *)&originalPlane );

  /* rotate the plane if necessary */
  if ( entityNum != ENTITYNUM_WORLD )
  {
    tr_currentEntityNumber = entityNum;
    tr_currentEntity = (int)&( (trRefEntity_t *)tr_refdef_entities )[entityNum];
    R_RotateForEntity( (trRefEntity_t *)tr_currentEntity, &trView, &trOrient );

    /* rotate the plane, but keep the non-rotated version for matching
     * against the portalSurface entities */
    R_LocalNormalToWorld( originalPlane.normal, plane.normal );
    plane.dist = originalPlane.dist + DotProduct( plane.normal, trOrient.origin );

    /* translate the original plane */
    originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, trOrient.origin );
  }
  else
  {
    plane = originalPlane;
  }

  VectorCopy( plane.normal, surface->axis[0] );
  PerpendicularVector( surface->axis[1], surface->axis[0] );
  surface->axis[2][0] = surface->axis[0][1] * surface->axis[1][2] - surface->axis[0][2] * surface->axis[1][1];
  surface->axis[2][1] = surface->axis[0][2] * surface->axis[1][0] - surface->axis[0][0] * surface->axis[1][2];
  surface->axis[2][2] = surface->axis[0][0] * surface->axis[1][1] - surface->axis[0][1] * surface->axis[1][0];

  /* locate the portal entity closest to this plane */
  for ( i = 0; i < tr_refdef_num_entities; i++ )
  {
    e = &( (trRefEntity_t *)tr_refdef_entities )[i];
    if ( e->e.reType != 11 )                       /* RT_PORTALSURFACE */
      continue;

    d = DotProduct( e->e.origin, originalPlane.normal ) - originalPlane.dist;
    if ( d > 64.0f || d < -64.0f )
      continue;

    /* get the pvsOrigin from the entity */
    VectorCopy( e->e.oldorigin, (float *)pvsOrigin );

    /* if the entity is just a mirror, don't use as a camera point */
    if ( e->e.oldorigin[0] == e->e.origin[0]
      && e->e.oldorigin[1] == e->e.origin[1]
      && e->e.oldorigin[2] == e->e.origin[2] )
    {
      VectorScale( plane.normal, plane.dist, surface->origin );
      VectorCopy( surface->origin, camera->origin );
      VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
      VectorCopy( surface->axis[1], camera->axis[1] );
      VectorCopy( surface->axis[2], camera->axis[2] );

      *isMirror = 1;
      return 1;
    }

    /* project the origin onto the surface plane to get
     * an origin point we can rotate around */
    d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
    VectorMA( e->e.origin, -d, surface->axis[0], surface->origin );

    /* now get the camera origin and orientation */
    VectorCopy( e->e.oldorigin, camera->origin );
    AxisCopy( e->e.axis, camera->axis );
    VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
    VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );

    /* e.rotation (+0x80) is the rotation offset in degrees */
    if ( e->e.oldframe )
    {
      /* if a speed is specified */
      if ( e->e.frame )
      {
        /* continuous rotate */
        d = ( tr_refdef_time / 1000.0f ) * e->e.frame;
        VectorCopy( camera->axis[1], transformed );
        RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
        CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
      }
      else
      {
        /* bobbing rotate, with the rotation offset */
        d = sin( tr_refdef_time * 0.003f );
        d = e->e.rotation + d * 4;
        VectorCopy( camera->axis[1], transformed );
        RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
        CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
      }
    }
    else if ( e->e.rotation )                       /* NaN also rotates, as retail */
    {
      d = e->e.rotation;
      VectorCopy( camera->axis[1], transformed );
      RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
      CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
    }

    *isMirror = 0;
    return 1;
  }

  /* if we didn't locate a portal entity, don't render anything */
  return 0;
}

/* ---- IsMirror  0x004D4820 ----  VERIFIED */
int __cdecl IsMirror( int entityNum, int drawSurf )
{
  trRefEntity_t *e;
  int            i;
  rfloat         d;          /* retail keeps d on the x87 stack, never rounded to float */
  cplane_t       plane;      /* R_PlaneForSurface writes normal, dist and type */

  R_PlaneForSurface( *(void **)( drawSurf + 4 ), (float *)&plane );

  /* unlike R_GetPortalOrientations this only shifts the plane distance by the
   * entity origin; the (local) normal is reused as-is for matching.
   * (retail 0x4D489A sums the dot (z + y) + x, then adds the old dist) */
  if ( entityNum != ENTITYNUM_WORLD )
  {
    tr_currentEntityNumber = entityNum;
    tr_currentEntity = (int)&( (trRefEntity_t *)tr_refdef_entities )[entityNum];
    R_RotateForEntity( (trRefEntity_t *)tr_currentEntity, &trView, &trOrient );
    plane.dist = plane.dist + DotProduct( plane.normal, trOrient.origin );
  }

  for ( i = 0; i < tr_refdef_num_entities; ++i )
  {
    e = &( (trRefEntity_t *)tr_refdef_entities )[i];
    if ( e->e.reType != 11 )                       /* RT_PORTALSURFACE */
      continue;

    /* retail 0x4D48E7 sums (z + x) + y */
    d = DotProduct( e->e.origin, plane.normal ) - plane.dist;
    if ( d > 64.0f || d < -64.0f )
      continue;

    return e->e.oldorigin[0] == e->e.origin[0]
        && e->e.oldorigin[1] == e->e.origin[1]
        && e->e.oldorigin[2] == e->e.origin[2];
  }

  return 0;
}

/* ---- SurfIsOffscreen  0x004D4980 ----  VERIFIED
 * Tessellate the portal surface and decide whether it is entirely off-screen
 * (so the portal view can be skipped). */
int __cdecl SurfIsOffscreen( int drawSurf, float (*clipDest)[4] )
{
  float shortest;
  unsigned int pointAnd;
  unsigned int pointFlags;
  unsigned int sort;
  int    entityNum;
  int    shader;
  int    sortStorage;
  int    numTriangles;
  int    numVertexes;
  int    i, j;
  int    index;
  rfloat normal[3];   /* retail keeps the eye-to-vertex vector and len on the x87 stack */
  rfloat len;
  float *xyz;
  float  clip[4];
  float  eye[4];

  shortest = 100000000.0f;
  pointAnd = (unsigned int)~0;

  R_RotateForViewer();

  /* R_DecomposeSort, inlined (0x004D499F-0x004D49C3). */
  sort         = ((drawSurf_t *)drawSurf)->sort;
  entityNum    = ( sort >> QSORT_ENTITYNUM_SHIFT ) & QSORT_ENTITYNUM_MASK;
  shader       = (int)tr_sortedShaders[ ( (unsigned int)sort >> QSORT_SHADERNUM_SHIFT ) & QSORT_SHADERNUM_MASK ];
  sortStorage = (unsigned int)sort >> QSORT_STORAGECLASS_SHIFT;
  if ( ( sort & 2 ) != 0 )
    entityNum = ENTITYNUM_WORLD;

  if ( entityNum != ENTITYNUM_WORLD )
    backEnd_currentEntity = backEnd_refdef_entities + 696 * entityNum;
  else
    backEnd_currentEntity = (int)&tr_worldEntity;

  if ( sortStorage != glState_currentStorageMode )    /* CoD addition */
  {
    if ( glConfig_NVVertexArrayRange )
      RB_SelectStorageNV( sortStorage );
    else if ( glConfig_ATIVertexArrayObject )
      RB_SelectStorageATI( sortStorage );
    glState_currentStorageMode = sortStorage;
  }

  RB_BeginSurface( (void *)shader, 3 );
  rb_surfaceTable[ *(unsigned int *)((drawSurf_t *)drawSurf)->surface ]( ((drawSurf_t *)drawSurf)->surface );

  numVertexes = tess_numVertexes;
  xyz = tess_xyz;
  if ( numVertexes <= 0 )
    return 1;

  for ( i = 0; i < numVertexes; ++i )
  {
    pointFlags = 0;

    R_TransformModelToClip( xyz, trOrient.modelMatrix, tr_viewParms_projectionMatrix, eye, clip );

    for ( j = 0; j < 3; ++j )
    {
      if ( clip[j] >= clip[3] )
        pointFlags |= ( 1 << ( j * 2 ) );
      else if ( clip[j] <= -clip[3] )
        pointFlags |= ( 1 << ( j * 2 + 1 ) );
    }
    pointAnd &= pointFlags;

    xyz += tess_vertexComponentCount;
  }

  if ( pointAnd )
    return 1;

  numTriangles = tess_numIndexes / 3;

  for ( i = 0; i < tess_numIndexes; i += 3 )
  {
    index = tess_indexes[i];

    normal[0] = tess_xyz[ tess_vertexComponentCount * index     ] - tr_viewParms_originX;
    normal[1] = tess_xyz[ tess_vertexComponentCount * index + 1 ] - tr_viewParms_originY;
    normal[2] = tess_xyz[ tess_vertexComponentCount * index + 2 ] - tr_viewParms_originZ;

    /* retail 0x4D4B84 sums (z + y) + x and rounds len only when storing it */
    len = normal[2] * normal[2] + normal[1] * normal[1] + normal[0] * normal[0];
    if ( len < shortest )
      shortest = (float)len;

    if ( normal[2] * tess_stageNormals[ 3 * index + 2 ]
       + normal[1] * tess_stageNormals[ 3 * index + 1 ]
       + normal[0] * tess_stageNormals[ 3 * index     ] >= 0.0f )
      numTriangles--;
  }

  if ( !numTriangles )
    return 1;

  if ( IsMirror( entityNum, drawSurf ) )
    return 0;

  if ( shortest > ((shader_t *)tess_shader)->portalRange * ((shader_t *)tess_shader)->portalRange )
    return 1;

  return 0;
}

/* ---- R_MirrorViewBySurface  0x004D4C30 ----  VERIFIED */
int __cdecl R_MirrorViewBySurface( drawSurf_t *drawSurf, int entityNum )
{
  orientation_t surface;
  orientation_t camera;
  viewParms_t   newParms;
  viewParms_t   oldParms;
  vec4_t        clipDest[128];

  if ( trView.isPortal )
  {
    ri.Printf( 1, "WARNING: recursive mirror/portal found\n" );
    return 0;
  }
  if ( r_noportals->integer )
    return 0;
  if ( r_fastsky->integer )
    return 0;

  if ( SurfIsOffscreen( (int)drawSurf, clipDest ) )
    return 0;

  oldParms = trView;
  newParms = trView;
  newParms.isPortal = 1;

  if ( !R_GetPortalOrientations( entityNum, (int)drawSurf, &surface, &camera,
                                 (unsigned int *)newParms.pvsOrigin, (unsigned int *)&newParms.isMirror ) )
  {
    return 0;
  }

  R_MirrorPoint( oldParms.orientation.origin, &surface, &camera, newParms.orientation.origin );

  VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
  newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );

  R_SetPlaneSidesDPVS( &newParms.portalPlane );

  R_MirrorVector( oldParms.orientation.axis[0], &surface, &camera, newParms.orientation.axis[0] );
  R_MirrorVector( oldParms.orientation.axis[1], &surface, &camera, newParms.orientation.axis[1] );
  R_MirrorVector( oldParms.orientation.axis[2], &surface, &camera, newParms.orientation.axis[2] );

  R_RenderView( &newParms );

  trView = oldParms;
  return 1;
}

/* swap two 8-byte drawSurfs (Q3's macro: needs an int temp in scope) */
#define SWAP_DRAW_SURF( a, b ) \
  temp = ( (int *)(a) )[0]; ( (int *)(a) )[0] = ( (int *)(b) )[0]; ( (int *)(b) )[0] = temp; \
  temp = ( (int *)(a) )[1]; ( (int *)(a) )[1] = ( (int *)(b) )[1]; ( (int *)(b) )[1] = temp;

/* ---- shortsort  0x004D4E20 ----  VERIFIED
 * Selection sort of the small [lo, hi] range for qsortFast, on 8-byte
 * drawSurfs keyed by the unsigned sort word. */
int *__cdecl shortsort( int *hiArg, int *loArg )
{
  drawSurf_t *lo = (drawSurf_t *)loArg;
  drawSurf_t *hi = (drawSurf_t *)hiArg;
  drawSurf_t *p, *max;
  unsigned    k;
  void       *s;

  for ( ; hi > lo; --hi )
  {
    max = lo;
    for ( p = lo + 1; p <= hi; ++p )
    {
      if ( p->sort > max->sort )
        max = p;
    }
    k = max->sort;    max->sort = hi->sort;       hi->sort = k;
    s = max->surface; max->surface = hi->surface; hi->surface = s;
  }
  return (int *)hi;
}

/* ---- qsortFast  0x004D4E70 ----  [HIGH]
 * Iterative quicksort (Q3's copy of the MSVC CRT qsort) over 8-byte drawSurfs
 * keyed on the unsigned sort word, dropping to shortsort below a small cutoff.
 * Pointer differences stay byte-granular: the `- 1` in the size comparison is
 * a load-bearing part of the original. */
void __cdecl qsortFast( void *base, unsigned num, unsigned width )
{
  char    *lo, *hi;              /* ends of sub-array currently sorting */
  char    *mid;                  /* points to middle of subarray */
  char    *loguy, *higuy;        /* traveling pointers for partition step */
  unsigned size;                 /* size of the sub-array */
  char    *lostk[30], *histk[30];
  int      stkptr;               /* stack for saving sub-array to be processed */
  int      temp;

  if ( num < 2 || width == 0 )
    return;                      /* nothing to do */

  stkptr = 0;                    /* initialize stack */

  lo = base;
  hi = (char *)base + width * ( num - 1 );   /* initialize limits */

recurse:

  size = ( hi - lo ) / width + 1;            /* number of el's to sort */

  /* below a certain size, it is faster to use a O(n^2) sorting method */
  if ( size <= 8 )
  {
    shortsort( (int *)hi, (int *)lo );
  }
  else
  {
    mid = lo + ( size / 2 ) * width;         /* find middle element */
    SWAP_DRAW_SURF( mid, lo );               /* swap it to beginning of array */

    loguy = lo;
    higuy = hi + width;

    for ( ;; )
    {
      do
        loguy += width;
      while ( loguy <= hi && ( (drawSurf_t *)loguy )->sort <= ( (drawSurf_t *)lo )->sort );

      do
        higuy -= width;
      while ( higuy > lo && ( (drawSurf_t *)higuy )->sort >= ( (drawSurf_t *)lo )->sort );

      if ( higuy < loguy )
        break;

      SWAP_DRAW_SURF( loguy, higuy );
    }

    SWAP_DRAW_SURF( lo, higuy );             /* put partition element in place */

    /* sort the smaller sub-array first to minimize stack usage */
    if ( higuy - 1 - lo >= hi - loguy )
    {
      if ( lo + width < higuy )
      {
        lostk[stkptr] = lo;
        histk[stkptr] = higuy - width;
        ++stkptr;
      }                                      /* save big recursion for later */

      if ( loguy < hi )
      {
        lo = loguy;
        goto recurse;                        /* do small recursion */
      }
    }
    else
    {
      if ( loguy < hi )
      {
        lostk[stkptr] = loguy;
        histk[stkptr] = hi;
        ++stkptr;                            /* save big recursion for later */
      }

      if ( lo + width < higuy )
      {
        hi = higuy - width;
        goto recurse;                        /* do small recursion */
      }
    }
  }

  /* pop any pending sub-array */
  --stkptr;
  if ( stkptr >= 0 )
  {
    lo = lostk[stkptr];
    hi = histk[stkptr];
    goto recurse;
  }
}
#undef SWAP_DRAW_SURF

/* ---- R_AddDrawSurf  0x004D4FB0 ----  VERIFIED
 * Append a draw surface, packing the sort key from storage class, shader
 * sorted-index, entity number and the fog/dlight flags. */
void __cdecl R_AddDrawSurf( int sortStorage, shader_t *shader, void *surface, int dlightBits, int fogIndex, int worldEntityFlag )
{
  if ( tr_refdef_numDrawSurfs < 0x10000 )
  {
    ( (drawSurf_t *)tr_refdef_drawSurfs )[ tr_refdef_numDrawSurfs ].sort = dlightBits | tr_shiftedEntityNumber
             | ( 2 * ( worldEntityFlag | ( 2 * ( fogIndex | ( ( ( sortStorage << 12 ) | shader->sortedIndex ) << 16 ) ) ) ) );
    ( (drawSurf_t *)tr_refdef_drawSurfs )[ tr_refdef_numDrawSurfs ].surface = surface;
    ++tr_refdef_numDrawSurfs;
  }
}

/* ---- R_DecomposeSort  0x004D5010 ----  VERIFIED */
unsigned int __cdecl R_DecomposeSort(
        unsigned int sort,
        int *entityNum,
        int *sortStorage,
        shader_t **shader,
        int *dlightMap,
        int *fogIndex )
{
  *sortStorage = sort >> QSORT_STORAGECLASS_SHIFT;
  *entityNum    = ( sort >> QSORT_ENTITYNUM_SHIFT ) & QSORT_ENTITYNUM_MASK;
  *shader       = tr_sortedShaders[ ( sort >> QSORT_SHADERNUM_SHIFT ) & QSORT_SHADERNUM_MASK ];
  *dlightMap           = sort & 1;
  *fogIndex           = ( sort >> 2 ) & 1;
  if ( ( sort & 2 ) != 0 )
    *entityNum = ENTITYNUM_WORLD;
  return sort;
}

/* ---- R_SortDrawSurfs  0x004D5070 ----  [HIGH]
 * Sort the frame's draw surfaces, then walk them to spawn portal/mirror
 * sub-views before handing the batch to the backend. */
void __cdecl R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs )
{
  shader_t *shader;
  int       entityNum;
  int       sortStorage;
  int       dlightMap, fogIndex;
  int       i;

  /* it is possible for some views to not have any surfaces */
  if ( numDrawSurfs < 1 )
  {
    R_AddDrawSurfCmd( numDrawSurfs, (int)drawSurfs );
    return;
  }

  /* if we overflowed MAX_DRAWSURFS, the drawsurfs wrapped around in the
   * buffer and we will be missing the first surfaces, not the newest ones */
  if ( numDrawSurfs > 0x10000 )
    numDrawSurfs = 0x10000;

  qsortFast( drawSurfs, numDrawSurfs, sizeof( drawSurf_t ) );

  /* check for any pass through drawing, which may cause another view to be rendered first */
  for ( i = 0; i < numDrawSurfs; i++ )
  {
    R_DecomposeSort( ( drawSurfs + i )->sort, &entityNum, &sortStorage, &shader, &dlightMap, &fogIndex );

    if ( shader->sort > 1.0f )                     /* > SS_PORTAL */
      break;

    /* no shader should ever have this sort type */
    if ( shader->sort == 0.0f )
      ri.Error( 1, "\x15" "Shader '%s'with sort == SS_BAD", shader );

    if ( shader->sort == 1.0f )                    /* SS_PORTAL */
    {
      if ( R_MirrorViewBySurface( drawSurfs + i, entityNum ) && r_portalOnly->integer )
        return;
    }
  }

  R_AddDrawSurfCmd( numDrawSurfs, (int)drawSurfs );
}

/* R_GetShaderByHandle (tr_shader.c), which retail inlines into its callers. */
static __inline shader_t *R_GetShaderByHandle( int hShader )
{
  if ( hShader < 0 )
  {
    ri.Printf( 2, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
    return (shader_t *)tr_defaultShader;
  }
  if ( hShader >= tr_numShaders[0] )
  {
    ri.Printf( 2, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
    return (shader_t *)tr_defaultShader;
  }
  return (shader_t *)tr_shaders[hShader];
}

/* ---- R_AddEntitySurfaces  0x004D5170 ----  [HIGH] */
void R_AddEntitySurfaces( void )
{
  trRefEntity_t *ent;
  shader_t      *shader;

  if ( !r_drawentities->integer )
    return;

  for ( tr_currentEntityNumber = 0; tr_currentEntityNumber < tr_refdef_num_entities; tr_currentEntityNumber++ )
  {
    ent = (trRefEntity_t *)( tr_refdef_entities + 696 * tr_currentEntityNumber );
    tr_currentEntity = (int)ent;
    ent->dlightBits = 0;
    tr_shiftedEntityNumber = tr_currentEntityNumber << QSORT_ENTITYNUM_SHIFT;

    /* the first-person weapon must not appear in mirror/portal views */
    if ( (ent->e.renderfx & 4) && trView.isPortal )
      continue;

    switch ( ent->e.reType )
    {
      case 0xB:                                    /* RT_PORTALSURFACE */
        break;
      case 4: case 5: case 6: case 7: case 8: case 9: case 0xA:
      case 0xC: case 0xD: case 0xE: case 0xF:
        /* sprites, beams, ... -- not drawn in the primary view */
        if ( (ent->e.renderfx & 2) && !trView.isPortal )
          continue;
        shader = R_GetShaderByHandle( ent->e.customShader );
        R_AddDrawSurf( storageClass, shader, &entitySurface, 0, 0, 0 );
        break;
      case 0:
        R_AddBrushModelSurfaces( (int)ent );
        break;
      case 1:
        R_AddXModelSurfaces( (int)ent );
        break;
      case 2:
        R_AddStaticModelSurfaces( (int)ent );
        break;
      default:
        ri.Error( 1, "\x15" "R_AddEntitySurfaces: Bad reType" );
        break;
    }
  }
}

/* 0x004D52A8 is not a function: it is the byte index table of R_AddEntitySurfaces' switch
 * (the IDB names it R_AddEntitySurfaces_switchIndexTable_NOT_CODE).  An earlier decompile
 * emitted it as a function; it had no callers and has been removed. */

/* ---- R_GenerateDrawSurfs  0x004D52C0 ----  VERIFIED */
void __cdecl R_GenerateDrawSurfs( void )
{
  R_SetupProjection();
  if ( tr_world && *(unsigned int *)( tr_world + 292 ) )
    R_AddWorldSurfacesDPVS();
  R_AddPolygonSurfaces();
  R_AddEntitySurfaces();
  if ( r_vc_makelog->integer )
    R_ShowLightVisCachePoints();
}

/* ---- R_RenderView  0x004D5310 ----  [HIGH] */
void __cdecl R_RenderView( viewParms_t *parms )
{
  int    firstDrawSurf;
  rfloat lodDist;

  if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 )
    return;

  ++tr_viewCount;
  trView = *parms;
  trView.frameSceneNum = tr_frameSceneNum;
  trView.frameCount = tr_frameCount;

  firstDrawSurf = tr_refdef_numDrawSurfs;

  ++tr_viewCount;

  lodDist = r_highLodDist->value;
  xmodel_testLodsEnabled = lodDist >= 0.0f;
  if ( lodDist < 0.0f )
    lodDist = 0.0f;
  xmodel_testLodDistances[0] = lodDist;
  lodDist = r_mediumLodDist->value;
  if ( lodDist < 0.0f )
    lodDist = 0.0f;
  xmodel_testLodDistances[5] = lodDist;
  lodDist = r_lowLodDist->value;
  if ( lodDist < 0.0f )
    lodDist = 0.0f;
  xmodel_testLodDistances[10] = lodDist;
  /* retail fcom / test ah,41h / jnz: stored only when strictly > 0, so NaN also clamps to 0 */
  lodDist = r_lodViewDist->value;
  if ( lodDist > 0.0f )
    xmodel_testLodDist = lodDist;
  else
    xmodel_testLodDist = 0.0f;

  /* set viewParms.world */
  R_RotateForViewer();
  R_SetupFrustum();
  R_GenerateDrawSurfs();
  R_SortDrawSurfs( (drawSurf_t *)tr_refdef_drawSurfs + firstDrawSurf, tr_refdef_numDrawSurfs - firstDrawSurf );
}

float s_flipMatrix[74] = {
     0.0f,  0.0f, -1.0f,  0.0f,
    -1.0f,  0.0f,  0.0f,  0.0f,
     0.0f,  1.0f,  0.0f,  0.0f,
     0.0f,  0.0f,  0.0f,  1.0f,
};

unsigned char entitySurface[232] = { 3, 0, 0, 0 };
