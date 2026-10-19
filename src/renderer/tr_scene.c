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
int rendererVisibleModelWarningView;
int r_firstSceneCorona;
int r_firstSceneDlight;
int r_firstSceneDrawSurf;
int r_firstSceneEntity;
int r_firstScenePoly;
int r_numcoronas;
int r_numdlights;
int r_numentities;
int r_numpolys;
int r_numpolyverts;
/* RTCW tr_scene.c skyboxportal */
int rendererSkyboxPortalActive;   /* 0x016DCA28 */
int rendererSkyboxPortal;   /* 0x016DCA54 */



#define r_lodbias                  ((cvar_t *)(r_lodbias))


/* ------------------------------------------------------------------------- *
 *  File-local typed views over the per-frame scene block (backEndData).
 *
 *  RE_ClearScene/RE_Add*ToScene/RE_RenderScene fill fixed-offset sub-arrays
 *  inside the big backEndData allocation; the record types below are proven by
 *  the decompiled writes and cross-checked with CoDUO, and pinned with offset
 *  asserts so a wrong offset fails the build.  msurface/dlight/drawSurf/entity
 *  types come from tr_local.h; only the poly, corona and incoming-refdef
 *  layouts are defined here.  All three are candidates for tr_local.h.
 * ------------------------------------------------------------------------- */

/* polyVert_t (0x20) -- xyz/st/color vertex; only ever block-copied here, so
 * its interior stays opaque.  RTCW's polyVert_t is 0x18; CoD widened it. */
typedef struct {
	byte bytes[0x20];
} polyVert_t;
COD1_ASSERT_SIZE(polyVert_t, 0x20);

/* srfPoly_t (0x10) -- SF_POLY draw surface.  CoD drops RTCW's fogIndex, so
 * this is 16 bytes with verts at +0x0c (RTCW is 20 with verts at +0x10). */
typedef struct {
	int         surfaceType;      /* +0x00  == 2 (SF_POLY) */
	qhandle_t   hShader;          /* +0x04 */
	int         numVerts;         /* +0x08 */
	polyVert_t *verts;            /* +0x0c */
} srfPoly_t;
COD1_ASSERT_SIZE(srfPoly_t, 0x10);
ASSERT_OFF(srfPoly_t, hShader, 0x04);
ASSERT_OFF(srfPoly_t, numVerts, 0x08);
ASSERT_OFF(srfPoly_t, verts, 0x0c);

/* corona_t (0x30) -- CoDUO renderer_corona_t */
typedef struct {
	vec3_t origin;                /* +0x00 */
	vec3_t color;                 /* +0x0c */
	vec3_t transformed;           /* +0x18  RTCW-lineage local origin; unused */
	float  scale;                 /* +0x24 */
	int    id;                    /* +0x28 */
	int    flags;                 /* +0x2c */
} corona_t;
COD1_ASSERT_SIZE(corona_t, 0x30);
ASSERT_OFF(corona_t, color, 0x0c);
ASSERT_OFF(corona_t, scale, 0x24);
ASSERT_OFF(corona_t, id, 0x28);

/* Incoming public refdef_t -- only the header prefix RE_RenderScene reads is
 * named; the tail (areamask/text/glfog/...) is not touched here.  The prefix
 * is the standard Q3/RTCW refdef head and matches trRefdef_t (tr_records.h). */
typedef struct {
	int    x, y, width, height;   /* +0x00 +0x04 +0x08 +0x0c */
	float  fov_x, fov_y;          /* +0x10 +0x14 */
	vec3_t vieworg;               /* +0x18 */
	vec3_t viewaxis[3];           /* +0x24 */
	int    time;                  /* +0x48 */
	int    rdflags;               /* +0x4c */
} refdef_t;
ASSERT_OFF(refdef_t, fov_x, 0x10);
ASSERT_OFF(refdef_t, vieworg, 0x18);
ASSERT_OFF(refdef_t, viewaxis, 0x24);
ASSERT_OFF(refdef_t, time, 0x48);
ASSERT_OFF(refdef_t, rdflags, 0x4c);

/* backEndData sub-array bases (fixed byte offsets into the frame block) */
#define SCENE_DRAWSURFS       0             /* drawSurf_t[]      */
#define SCENE_DLIGHTS         0x80000       /* dlight_t[]        (524288) */
#define SCENE_CORONAS         528640        /* corona_t[]        */
#define SCENE_ENTITIES        530176        /* trRefEntity_t[]   */
#define SCENE_ENTITYSURFACES  1242880       /* entity surface[]  */
#define SCENE_POLYS           1308416       /* srfPoly_t[]       (16 * 81776) */
#define SCENE_POLYVERTS       1373952       /* polyVert_t[]      (32 * 42936) */
#define SCENE_CMD_USED        1898240       /* renderCommandList.used */



extern int R_RenderView();
double Distance( const vec3_t p1, const vec3_t p2 );  /* com_math.c (vec_t, /Op); double here: R_GetLodDist re-stores it (0x004EA113) */

/* ---- R_ToggleSmpFrame  0x004E96F0 ----  VERIFIED
 * Reset the per-frame scene counters.  CoD dropped RTCW's r_smp double buffer
 * (no tr.smpFrame swap), so this always touches frame 0.  The int return is
 * dead EAX (always 0); every caller ignores it. */
int R_ToggleSmpFrame( void )
{
  *(int *)(backEndData + SCENE_CMD_USED) = 0;    /* renderCommandList.used = 0 */

  r_firstSceneDrawSurf = 0;

  r_numdlights       = 0;
  r_firstSceneDlight = 0;

  r_numcoronas       = 0;
  r_firstSceneCorona = 0;

  r_numentities      = 0;
  r_firstSceneEntity = 0;

  r_numpolys       = 0;
  r_firstScenePoly = 0;

  r_numpolyverts = 0;

  return 0;
}

/* ---- RE_ClearScene  0x004E9740 ----  VERIFIED
 * Mark the current scene contents as the new baseline for the next scene.
 * Returns r_numpolys (dead EAX). */
int RE_ClearScene( void )
{
  int result = r_numpolys;

  r_firstSceneDlight = r_numdlights;
  r_firstSceneCorona = r_numcoronas;
  r_firstSceneEntity = r_numentities;
  r_firstScenePoly   = r_numpolys;
  return result;
}

/* R_GetShaderByHandle (tr_shader.c), which retail inlines here. */
static __inline shader_t *R_SceneShaderByHandle( int hShader )
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

/* ---- R_AddPolygonSurfaces  0x004E9770 ----  VERIFIED
 * Queue the scene's polygons as draw surfaces.  Returns tr.refdef.numPolys
 * (dead EAX). */
int R_AddPolygonSurfaces( void )
{
  srfPoly_t *poly = (srfPoly_t *)tr_refdef_polys;
  shader_t  *shader;
  int        i;

  for ( i = 0; i < tr_refdef_numPolys; ++i, ++poly )
  {
    tr_currentEntityNumber = i & QSORT_ENTITYNUM_MASK;
    tr_shiftedEntityNumber = tr_currentEntityNumber << QSORT_ENTITYNUM_SHIFT;

    shader = R_SceneShaderByHandle( poly->hShader );

    if ( tr_refdef_numDrawSurfs < 0x10000 )
    {
      ( (drawSurf_t *)tr_refdef_drawSurfs )[ tr_refdef_numDrawSurfs ].sort =
          ( ( ( storageClass << 12 ) | shader->sortedIndex ) << QSORT_SHADERNUM_SHIFT )
          | tr_shiftedEntityNumber | 2;
      ( (drawSurf_t *)tr_refdef_drawSurfs )[ tr_refdef_numDrawSurfs ].surface = poly;
      ++tr_refdef_numDrawSurfs;
    }
  }

  return tr_refdef_numPolys;
}

/* ---- RE_AddPolyToScene  0x004E9830 ----  VERIFIED
 * Add one polygon (numVerts verts) to the scene. */
void __cdecl RE_AddPolyToScene( int hShader, int numVerts, const void *verts )
{
  srfPoly_t  *poly;
  polyVert_t *polyVerts;

  if ( !tr_registered[0] )
    return;

  if ( !hShader )
    hShader = ( (shader_t *)tr_defaultShader )->index;

  if ( r_numpolyverts + numVerts > max_polyverts )
    return;
  if ( r_numpolys >= max_polys )
    return;

  poly      = &( (srfPoly_t  *)( backEndData + SCENE_POLYS ) )[ r_numpolys ];
  polyVerts = &( (polyVert_t *)( backEndData + SCENE_POLYVERTS ) )[ r_numpolyverts ];

  poly->surfaceType = 2;                         /* SF_POLY */
  poly->hShader     = hShader;
  poly->numVerts    = numVerts;
  poly->verts       = polyVerts;

  memcpy( polyVerts, verts, 32 * numVerts );
  ++r_numpolys;
  r_numpolyverts += numVerts;
}

/* ---- RE_AddPolysToScene  0x004E98D0 ----  VERIFIED
 * Add numPolys polygons of numVerts verts each, sharing one shader; their
 * vertex data is packed contiguously in `verts`. */
void __cdecl RE_AddPolysToScene( int hShader, int numVerts, char *verts, int numPolys )
{
  srfPoly_t  *poly;
  int         i;

  if ( !tr_registered[0] )
    return;

  if ( !hShader )
  {
    ri.Printf( 2, "WARNING: RE_AddPolysToScene: NULL poly shader\n" );
    return;
  }

  for ( i = 0; i < numPolys; ++i )
  {
    if ( r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys )
      return;

    poly = &( (srfPoly_t *)( backEndData + SCENE_POLYS ) )[ r_numpolys ];
    poly->surfaceType = 2;                       /* SF_POLY */
    poly->hShader     = hShader;
    poly->numVerts    = numVerts;
    poly->verts       = &( (polyVert_t *)( backEndData + SCENE_POLYVERTS ) )[ r_numpolyverts ];

    memcpy( poly->verts, &( (polyVert_t *)verts )[ numVerts * i ], 32 * numVerts );
    ++r_numpolys;
    r_numpolyverts += numVerts;
  }
}

/* ---- RE_AddRefEntityToScene  0x004E9A00 ----  VERIFIED */
void __cdecl RE_AddRefEntityToScene( refEntity_t *ent, int staticModelLighting )
{
  int            reType;

  if ( !tr_registered[0] )
    return;

  if ( com_statmon->integer && r_numentities >= 919 )
    StatMon_Warning( 9, 3000, "gfx/2d/warning@models.jpg" );

  if ( r_numentities >= 1022 )
  {
    if ( rendererVisibleModelWarningView != tr_viewCount )
    {
      rendererVisibleModelWarningView = tr_viewCount;
      ri.Printf( 2, "too many visible models (more than %i)\n", 1021 );
    }
    return;
  }

  reType = ent->reType;
  if ( reType < 0 || reType >= 16 )
    ri.Error( 1, "\x15" "RE_AddRefEntityToScene: bad reType %i", reType );

  /* retail indexes the entity per field (no pointer local) */
#define SCENE_ENT  ( (trRefEntity_t *)( backEndData + SCENE_ENTITIES ) )[ r_numentities ]
  memcpy( &SCENE_ENT.e, ent, sizeof( refEntity_t ) );

  SCENE_ENT.lightingCalculated                   = 0;
  SCENE_ENT.cullState                            = 0;
  SCENE_ENT.staticModelLighting                  = (void *)staticModelLighting;
  SCENE_ENT.lightCount                           = 0;
#undef SCENE_ENT

  ++r_numentities;
  ++tr_refdef_num_entities;
}

/* ---- RE_AddLightToScene  0x004E9B00 ----  VERIFIED
 * Register a point light.  radius drives the intensity (radius^2 / 32); the
 * ambient/specular/attenuation defaults reproduce the retail bit patterns. */
void __cdecl RE_AddLightToScene( float *origin, float radius, float r, float g, float b )
{
  dlight_t *light;
  rfloat    intensity;

  if ( !tr_registered[0] || r_numdlights >= 32 || radius <= 0.0f )
    return;

  intensity = radius * radius * 0.03125f;
  light = &( (dlight_t *)( backEndData + SCENE_DLIGHTS ) )[ r_numdlights++ ];

  light->radius    = radius;
  light->type      = 2;                          /* R_LIGHT_TYPE_POINT */
  light->color[0]  = r;
  light->color[1]  = g;
  light->color[2]  = b;
  light->intensity = intensity;

  light->ambient[0] = 0.0f;
  light->ambient[1] = 0.0f;
  light->ambient[2] = 0.0f;
  light->ambient[3] = 1.0f;

  light->diffuse[0] = tr_identityLight * intensity * r;
  light->diffuse[1] = tr_identityLight * intensity * g;
  light->diffuse[2] = tr_identityLight * intensity * b;
  light->diffuse[3] = 1.0f;

  light->specular[0] = 0.0f;
  light->specular[1] = 0.0f;
  light->specular[2] = 0.0f;
  light->specular[3] = 1.0f;

  light->position[0] = origin[0];
  light->position[1] = origin[1];
  light->position[2] = origin[2];
  light->position[3] = 1.0f;

  light->constantAttenuation  = 0.001f;
  light->linearAttenuation    = 0.0f;
  light->quadraticAttenuation = 1.0f;
  light->spotExponent         = 0.0f;
  light->spotCutoff           = 180.0f;
}

/* ---- RE_AddCoronaToScene  0x004E9C10 ----  VERIFIED */
void __cdecl RE_AddCoronaToScene( float *origin, float r, float g, float b,
                                  float scale, int id, int flags )
{
  corona_t *corona;

  if ( !tr_registered[0] || r_numcoronas >= 32 )
    return;

  if ( id < 0 || id >= 1024 )
  {
    ri.Printf( 1, "^3added corona with invalid id %i (should be >= 0 and < %i)\n", id, 1024 );
    return;
  }

  corona = &( (corona_t *)( backEndData + SCENE_CORONAS ) )[ r_numcoronas++ ];
  corona->origin[0] = origin[0];
  corona->origin[1] = origin[1];
  corona->origin[2] = origin[2];
  corona->color[0]  = r;
  corona->color[1]  = g;
  corona->color[2]  = b;
  corona->scale     = scale;
  corona->id        = id;
  corona->flags     = flags;
}

/* ---- RE_RenderScene  0x004E9CB0 ----  VERIFIED
 * Copy the caller's refdef into tr.refdef, build a primary viewParms_t on the
 * stack and hand it to R_RenderView.  void: the EAX values retail leaves
 * (tr_registered / r_norefresh / elapsed ms) are never read. */
void __cdecl RE_RenderScene( refdef_t *fd )
{
  viewParms_t parms;
  int         startTime;
  int         rdflags;
  rfloat      fov;

  if ( !tr_registered[0] )
    return;

  if ( Stream )
    fprintf( Stream, "%s", "====== RE_RenderScene =====\n" );

  if ( r_norefresh->integer )
    return;

  startTime = ri.Milliseconds();

  if ( !tr_world && ( fd->rdflags & 1 ) == 0 )
    ri.Error( 1, "\x15" "R_RenderScene: NULL worldmodel" );

  /* ---- copy the incoming refdef into tr.refdef ----
   * viewaxis rows 0 and 2 are int-typed macros in tr_records.h; copy their
   * float bits verbatim so no float->int narrowing occurs. */
  tr_refdef_x          = fd->x;
  tr_refdef_y          = fd->y;
  tr_refdef_width      = fd->width;
  tr_refdef_height     = fd->height;
  tr_refdef_fov_x      = fd->fov_x;
  tr_refdef_fov_y      = fd->fov_y;
  tr_refdef_vieworgX   = fd->vieworg[0];
  tr_refdef_vieworgY   = fd->vieworg[1];
  tr_refdef_vieworgZ   = fd->vieworg[2];
  tr_refdef_viewaxis00 = *(int *)&fd->viewaxis[0][0];
  tr_refdef_viewaxis01 = *(int *)&fd->viewaxis[0][1];
  tr_refdef_viewaxis02 = *(int *)&fd->viewaxis[0][2];
  tr_refdef_viewaxis10 = fd->viewaxis[1][0];
  tr_refdef_viewaxis11 = fd->viewaxis[1][1];
  tr_refdef_viewaxis12 = fd->viewaxis[1][2];
  tr_refdef_viewaxis20 = *(int *)&fd->viewaxis[2][0];
  tr_refdef_viewaxis21 = *(int *)&fd->viewaxis[2][1];
  tr_refdef_viewaxis22 = *(int *)&fd->viewaxis[2][2];
  tr_refdef_time       = fd->time;
  tr_refdef_rdflags    = fd->rdflags;

  rdflags = fd->rdflags;
  if ( ( rdflags & 8 ) != 0 )
    rendererSkyboxPortal = 1;
  if ( ( rdflags & 0x10 ) != 0 )                 /* VC7: movsx al; and 10h; shr 4 */
    rendererSkyboxPortalActive = 1;
  else
    rendererSkyboxPortalActive = 0;

#ifdef COD1_VC7
  tr_refdef_floatTime          = tr_refdef_time * 0.001f;           /* flt_568E6C = 0x3A83126F */
#else
  tr_refdef_floatTime          = (double)tr_refdef_time * 0.001f;
#endif
  tr_refdef_numDrawSurfs       = r_firstSceneDrawSurf;
  tr_refdef_drawSurfs          = backEndData + SCENE_DRAWSURFS;
  tr_refdef_num_entities       = r_numentities - r_firstSceneEntity;
  tr_refdef_entities           = 696 * r_firstSceneEntity + backEndData + SCENE_ENTITIES;
  tr_refdef_entitySurfaces     = backEndData + SCENE_ENTITYSURFACES;
  tr_refdef_entitySurfaceCount = 0;
  tr_refdef_num_dlights        = r_numdlights - r_firstSceneDlight;
  tr_refdef_entityDlightCount  = tr_refdef_num_dlights;
  tr_refdef_dlights            = 136 * r_firstSceneDlight + backEndData + SCENE_DLIGHTS;
  tr_refdef_coronaCount        = r_numcoronas - r_firstSceneCorona;
  tr_refdef_coronas            = 48 * r_firstSceneCorona + backEndData + SCENE_CORONAS;
  tr_refdef_numPolys           = r_numpolys - r_firstScenePoly;
  tr_refdef_polys              = 16 * r_firstScenePoly + backEndData + SCENE_POLYS;

  if ( !r_dynamiclight->integer )
    tr_refdef_entityDlightCount = 0;
  if ( r_dynamiclight->integer != 1 )
    tr_refdef_num_dlights = 0;
  if ( r_dynamiclight->integer == 3 )
    ri.Printf( 0, "%i dynamic lights in scene\n", tr_refdef_num_dlights );

  /* ---- build the primary view ---- */
  memset( &parms, 0, sizeof( parms ) );

  parms.viewportX      = tr_refdef_x;
  parms.viewportY      = glConfig_vidHeight - tr_refdef_height - tr_refdef_y;
  parms.viewportWidth  = tr_refdef_width;
  parms.viewportHeight = tr_refdef_height;
  parms.fovX           = tr_refdef_fov_x;
  parms.fovY           = tr_refdef_fov_y;
  parms.isPortal       = 0;

  /* lodScale is scaled by tan(narrower-fov/2) relative to a 40-degree base.
   * Retail tests !(fovX <= fovY) (fcomp; test ah,41h; jp), so an unordered
   * (NaN) compare selects fovY. The half-angle factor flt_56904C is the float
   * 0x3C0EFA35 (0.0087266462f); the divisor dbl_569190 is a double. */
  if ( parms.fovX <= parms.fovY )
    fov = parms.fovX;
  else
    fov = parms.fovY;
#ifdef COD1_VC7
  parms.lodScale = tan( fov * 0.0087266462f ) / tan( 0.6981317400932312 );
  parms.lodScale *= r_lodscale->value;          /* split: retail's fstp lands after the axis[0][0] load */
#else
  parms.lodScale = tan( fov * 0.0087266462f ) / tan( 0.6981317400932312 ) * r_lodscale->value;
#endif
  parms.lodBias  = r_lodbias->value;

  VectorCopy( fd->vieworg, parms.orientation.origin );
  VectorCopy( fd->viewaxis[0], parms.orientation.axis[0] );
  VectorCopy( fd->viewaxis[1], parms.orientation.axis[1] );
  VectorCopy( fd->viewaxis[2], parms.orientation.axis[2] );

  VectorCopy( fd->vieworg, parms.pvsOrigin );

  R_RenderView( &parms );

  r_firstSceneDrawSurf = tr_refdef_numDrawSurfs;
  r_firstSceneEntity   = r_numentities;
  r_firstSceneDlight   = r_numdlights;
  r_firstScenePoly     = r_numpolys;

  tr_frontEndMsec += ri.Milliseconds() - startTime;
}


/* ---- R_GetLodDist  0x004EA0C0 ----  VERIFIED
 * Distance from the view origin to the entity, scaled by lodScale/lodBias and
 * an optional per-entity divisor (refEnt[16]).  Cross-file callers declare this
 * `extern double __cdecl R_GetLodDist(float *)`, so the signature is fixed. */
double __cdecl R_GetLodDist( float *refEnt )
{
  float dist;
  float lodDist;

  dist = Distance( &refEnt[17], &tr_viewParms_originX );

  lodDist = tr_viewParms_lodScale * dist + tr_viewParms_lodBias;
  if ( refEnt[16] != 0.0f )
    lodDist = lodDist / refEnt[16];
  return lodDist;
}
