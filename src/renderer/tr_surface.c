/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_surface.c -- the RB_Surface* back-end tessellators (RTCW lineage) plus
 * CoD's optimized / static-model surface entry points and the entity-surface
 * dispatch (rb_surfaceTable, in tr_main.c).
 *
 * THIS ENTIRE FILE IS BACK END: backEnd.currentEntity is valid, RB_BeginSurface
 * has already run for the surface's shader and the modelview matrix is set.
 *
 * Data reached by POINTER is typed with the file-local views below (surface
 * headers, the public poly / world-mesh vertex streams, and the current
 * render entity).  The engine's big blobs (tess, backEnd, tr) stay on the
 * standalone globals / tr_records.h macros; retyping a pointer never moves
 * storage, and each new struct pins its field offsets with COD1_ASSERT_SIZE /
 * ASSERT_OFF so a wrong offset is a build error.
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_local.h"
#include "tr_gl_types.h"
#include "tr_tess.h"

extern int GL_Bind();
extern int GL_State();
void PerpendicularVector( float *dst, const float *src );
extern int RB_BeginImmediateMode();
extern int RB_glVertex3f();
extern int R_FogOn();
void RotatePointAroundVector( float *dst, const float *dir, const float *point, float degrees );

extern void MakeNormalVectors( const float *forward, float *right, float *up );

extern void RB_BeginSurface( void *shader, int vertexComponentCount );
extern void RB_EndSurface( void );
extern int  FastRound( float f );   /* client_mp/cl_console_mp.c 0x00408370 */
extern void RB_glVertex3fv( const float *v );
extern void RB_glEnd( void );
extern void RB_EndImmediateMode( void );

#define SHADER_MAX_VERTEXES          8192      /* cmp 2000h  @0x0050F96D */
#define SHADER_MAX_INDEXES           49152     /* cmp 0C000h @0x0050F981 */


/* ------------------------------------------------------------------------- *
 *  File-local typed views over the pointer-passed data.
 * ------------------------------------------------------------------------- */

/* Public poly vertex (SF_POLY source stream) -- 32 bytes:
 * XYZ, base ST, lightmap ST, then four packed RGBA bytes. */
typedef struct {
	vec3_t   xyz;                /* +0x00 */
	float    st[2];              /* +0x0c  base texcoords */
	float    lightmap[2];        /* +0x14  lightmap texcoords */
	byte     modulate[4];        /* +0x1c  RGBA */
} polyVert_t;
COD1_ASSERT_SIZE(polyVert_t, 0x20);
ASSERT_OFF(polyVert_t, st, 0x0c);
ASSERT_OFF(polyVert_t, lightmap, 0x14);
ASSERT_OFF(polyVert_t, modulate, 0x1c);

/* Polygon-chain surface header (RB_SurfacePolychain arg) -- 0x10 bytes. */
typedef struct {
	int          surfaceType;    /* +0x00 */
	int          hShader;        /* +0x04  shader handle (unused here) */
	int          numVerts;       /* +0x08 */
	polyVert_t  *verts;          /* +0x0c */
} srfPoly_t;
COD1_ASSERT_SIZE(srfPoly_t, 0x10);
ASSERT_OFF(srfPoly_t, numVerts, 0x08);
ASSERT_OFF(srfPoly_t, verts, 0x0c);

/* Indexed world-mesh surface (RB_SurfaceTriangles / RB_DlightFallback /
 * RB_SurfaceOptimized arg).  The srfLit_t prefix (type, storage, dlightBits,
 * bounds) is followed by the separately allocated vertex-attribute streams and
 * an inline index array.  Streams are packed 3-float / 2-float per vertex. */
typedef struct {
	int              surfaceType;      /* +0x00 */
	int              storageMode;      /* +0x04 */
	unsigned         dlightBits;       /* +0x08 */
	vec3_t           boundsMin;        /* +0x0c */
	vec3_t           boundsMax;        /* +0x18 */
	int              numVerts;         /* +0x24 */
	vec3_t          *tangents;         /* +0x28 */
	vec3_t          *bitangents;       /* +0x2c */
	vec3_t          *normals;          /* +0x30 */
	float          (*texCoords)[2];    /* +0x34  base ST */
	float          (*lightmapCoords)[2]; /* +0x38  lightmap ST */
	unsigned        *colors;           /* +0x3c  one packed RGBA per vertex */
	vec3_t          *positions;        /* +0x40 */
	int              numIndexes;       /* +0x44 */
	unsigned short   indices[1];       /* +0x48  inline, numIndexes entries */
} worldMeshSurf_t;
ASSERT_OFF(worldMeshSurf_t, dlightBits, 0x08);
ASSERT_OFF(worldMeshSurf_t, numVerts, 0x24);
ASSERT_OFF(worldMeshSurf_t, tangents, 0x28);
ASSERT_OFF(worldMeshSurf_t, bitangents, 0x2c);
ASSERT_OFF(worldMeshSurf_t, normals, 0x30);
ASSERT_OFF(worldMeshSurf_t, texCoords, 0x34);
ASSERT_OFF(worldMeshSurf_t, lightmapCoords, 0x38);
ASSERT_OFF(worldMeshSurf_t, colors, 0x3c);
ASSERT_OFF(worldMeshSurf_t, positions, 0x40);
ASSERT_OFF(worldMeshSurf_t, numIndexes, 0x44);
ASSERT_OFF(worldMeshSurf_t, indices, 0x48);

/* The render entity the surface tessellators read: the public refEntity_t
 * prefix of the trRefEntity_t that backEnd_currentEntity points at
 * (tr_local.h; radius2 = end radius / quad height, rotation in degrees). */
typedef refEntity_t rbSurfEntity_t;
ASSERT_OFF(rbSurfEntity_t, axis, 0x1c);
ASSERT_OFF(rbSurfEntity_t, origin, 0x44);
ASSERT_OFF(rbSurfEntity_t, oldorigin, 0x54);
ASSERT_OFF(rbSurfEntity_t, radius2, 0x64);
ASSERT_OFF(rbSurfEntity_t, shaderRGBA, 0x6c);
ASSERT_OFF(rbSurfEntity_t, radius, 0x7c);
ASSERT_OFF(rbSurfEntity_t, rotation, 0x80);

/* Retail's inlined fsincos blocks come from an inline-asm SinCos local to this
 * file: inlining com_math.c's (/Op) SinCos makes each caller /Op-like. */
static __inline void RB_SinCos( float radians, float *sinOut, float *cosOut )
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

/* RB_CheckOverflow is an ordinary inlinable function: retail inlines it into
 * RB_AddQuadStampExt (left to VC7 here, through the plain call) and calls it from the
 * tr_backend 2D quad functions (RB_DrawStretchPic, RB_StretchPicGradient,
 * RB_StretchPicRotate, RB_DrawQuadPic), which block the inline with an in-body
 * #pragma inline_depth(0) around the call.  DoLine, DoRailDiscs and
 * RB_AddQuadStampFadingCornersExt, which retail inlines it into as well, use this
 * expansion of its body (the ri.Error checks fold away for constant counts).
 * Expanding it by hand in RB_AddQuadStampExt instead keeps its code but drops the
 * call edge, which changes the alignment state of RB_CheckOverflow /
 * RB_DlightFallback / RB_SurfaceOptimized (push ecx pads, and esp,-8) -- the edge
 * has to exist before inlining (measured 2026-10-14). */
#define RB_CHECKOVERFLOW_EXPANDED( numVerts, numIndexes ) \
  if ( tess_numVertexes + ( numVerts ) >= SHADER_MAX_VERTEXES || tess_numIndexes + ( numIndexes ) >= SHADER_MAX_INDEXES ) \
  { \
    int verts = ( numVerts ), indexes = ( numIndexes ); \
    int *pv = &verts, *pi = &indexes; \
    (void)pv; (void)pi; \
    if ( tess_numVertexes + verts >= SHADER_MAX_VERTEXES || tess_numIndexes + indexes >= SHADER_MAX_INDEXES ) \
    { \
      RB_EndSurface(); \
      backEnd_currentEntity = tess_entity; \
      RB_BeginSurface( (void *)tess_shader, tess_vertexComponentCount ); \
    } \
  }

/* ---- RB_CheckOverflow  0x0050F960 ----  VERIFIED */
void __cdecl RB_CheckOverflow( int verts, int indexes )
{
  { /* address-taken arguments: retail passes them on the stack, not in LTCG registers */
    int *pv = &verts, *pi = &indexes;
    (void)pv; (void)pi;
  }

  if ( tess_numVertexes + verts < SHADER_MAX_VERTEXES
    && tess_numIndexes + indexes < SHADER_MAX_INDEXES )
    return;

  RB_EndSurface();

  if ( verts > SHADER_MAX_VERTEXES )
    ri.Error( 1, "\x15" "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );     /* 0x5438F4 */
  if ( indexes > SHADER_MAX_INDEXES )
    ri.Error( 1, "\x15" "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );  /* 0x5438C8 */

  backEnd_currentEntity = tess_entity;
  RB_BeginSurface( (void *)tess_shader, tess_vertexComponentCount );
}


/* ---- RB_CheckOverflow_Optimized  0x0050F9F0 ----  VERIFIED
 * No callers.  The never-read argument is real: retail tail-calls
 * RB_BeginSurface through its stack slot (mov [esp+4],ecx / jmp). */
void __cdecl RB_CheckOverflow_Optimized( int unused )
{
  RB_EndSurface();
  backEnd_currentEntity = tess_entity;
  RB_BeginSurface( (void *)tess_shader, tess_vertexComponentCount );
}

/* ---- RB_AddQuadStampFadingCornersExt  0x0050FA20 ----  VERIFIED
 * Sprite with a solid center (vertex 4 at full color) fading to zero alpha at
 * the four corners. */
void __cdecl RB_AddQuadStampFadingCornersExt( float *origin, float *left, float *up,
                                              int *color, float s1, float t1,
                                              float s2, float t2 )
{
  int    numVertexes;
  unsigned short ndx;
  float *xyz;
  vec3_t normal;
  int    fadedColor;

  RB_CHECKOVERFLOW_EXPANDED( 5, 12 );

  numVertexes = tess_numVertexes;
  ndx         = numVertexes;
  xyz         = &tess_xyz[ ndx * tess_vertexComponentCount ];

  /* Four triangles fanning around vertex 4. */
  tess_indexes[ tess_numIndexes      ] = (unsigned short)( numVertexes     );
  tess_indexes[ tess_numIndexes +  1 ] = (unsigned short)( numVertexes + 1 );
  tess_indexes[ tess_numIndexes +  2 ] = (unsigned short)( numVertexes + 4 );
  tess_indexes[ tess_numIndexes +  3 ] = (unsigned short)( numVertexes + 1 );
  tess_indexes[ tess_numIndexes +  4 ] = (unsigned short)( numVertexes + 2 );
  tess_indexes[ tess_numIndexes +  5 ] = (unsigned short)( numVertexes + 4 );
  tess_indexes[ tess_numIndexes +  6 ] = (unsigned short)( numVertexes + 2 );
  tess_indexes[ tess_numIndexes +  7 ] = (unsigned short)( numVertexes + 3 );
  tess_indexes[ tess_numIndexes +  8 ] = (unsigned short)( numVertexes + 4 );
  tess_indexes[ tess_numIndexes +  9 ] = (unsigned short)( numVertexes + 3 );
  tess_indexes[ tess_numIndexes + 10 ] = (unsigned short)( numVertexes     );
  tess_indexes[ tess_numIndexes + 11 ] = (unsigned short)( numVertexes + 4 );

  *xyz++ = left[0]   + up[0]     + origin[0];
  *xyz++ = origin[1] + up[1]     + left[1];
  *xyz++ = origin[2] + up[2]     + left[2];

  *xyz++ = origin[0] - left[0]   + up[0];
  *xyz++ = origin[1] - left[1]   + up[1];
  *xyz++ = origin[2] - left[2]   + up[2];

  *xyz++ = origin[0] - left[0]   - up[0];
  *xyz++ = origin[1] - left[1]   - up[1];
  *xyz++ = origin[2] - left[2]   - up[2];

  *xyz++ = left[0]   + origin[0] - up[0];
  *xyz++ = origin[1] + left[1]   - up[1];
  *xyz++ = origin[2] + left[2]   - up[2];

  *xyz++ = origin[0];
  *xyz++ = origin[1];
  *xyz   = origin[2];

  /* constant normal, opposite the view forward axis */
  VectorSubtract( vec3_origin, &backEnd_viewParms_axis00, normal );

  ((vec3_t *)tess_stageNormals)[ ndx ][0] = ((vec3_t *)tess_stageNormals)[ ndx + 1 ][0] = ((vec3_t *)tess_stageNormals)[ ndx + 2 ][0] = ((vec3_t *)tess_stageNormals)[ ndx + 3 ][0] = ((vec3_t *)tess_stageNormals)[ ndx + 4 ][0] = normal[0];
  ((vec3_t *)tess_stageNormals)[ ndx ][1] = ((vec3_t *)tess_stageNormals)[ ndx + 1 ][1] = ((vec3_t *)tess_stageNormals)[ ndx + 2 ][1] = ((vec3_t *)tess_stageNormals)[ ndx + 3 ][1] = ((vec3_t *)tess_stageNormals)[ ndx + 4 ][1] = normal[1];
  ((vec3_t *)tess_stageNormals)[ ndx ][2] = ((vec3_t *)tess_stageNormals)[ ndx + 1 ][2] = ((vec3_t *)tess_stageNormals)[ ndx + 2 ][2] = ((vec3_t *)tess_stageNormals)[ ndx + 3 ][2] = ((vec3_t *)tess_stageNormals)[ ndx + 4 ][2] = normal[2];

  tess_texCoords0[ 2 * ndx     ] = tess_texCoords1[ 2 * ndx     ] = s1;
  tess_texCoords0[ 2 * ndx + 1 ] = tess_texCoords1[ 2 * ndx + 1 ] = t1;
  tess_texCoords0[ 2 * ndx + 2 ] = tess_texCoords1[ 2 * ndx + 2 ] = s2;
  tess_texCoords0[ 2 * ndx + 3 ] = tess_texCoords1[ 2 * ndx + 3 ] = t1;
  tess_texCoords0[ 2 * ndx + 4 ] = tess_texCoords1[ 2 * ndx + 4 ] = s2;
  tess_texCoords0[ 2 * ndx + 5 ] = tess_texCoords1[ 2 * ndx + 5 ] = t2;
  tess_texCoords0[ 2 * ndx + 6 ] = tess_texCoords1[ 2 * ndx + 6 ] = s1;
  tess_texCoords0[ 2 * ndx + 7 ] = tess_texCoords1[ 2 * ndx + 7 ] = t2;
  tess_texCoords0[ 2 * ndx + 8 ] = tess_texCoords1[ 2 * ndx + 8 ] = ( s1 + s2 ) * 0.5f;
  tess_texCoords0[ 2 * ndx + 9 ] = tess_texCoords1[ 2 * ndx + 9 ] = ( t1 + t2 ) * 0.5f;

  /* center vertex keeps full alpha; the four corners fade alpha to 0 */
  ((int *)tess_vertexColors)[ndx + 4] = *color;
  fadedColor = *color;
  ((unsigned char *)&fadedColor)[3] = 0;
  ((int *)tess_vertexColors)[ndx    ] = ((int *)tess_vertexColors)[ndx + 1]
    = ((int *)tess_vertexColors)[ndx + 2] = ((int *)tess_vertexColors)[ndx + 3] = fadedColor;

  tess_numVertexes += 5;
  tess_numIndexes  += 12;
}

/* ---- RB_AddQuadStampExt  0x0050FE20 ----  VERIFIED */
int __cdecl RB_AddQuadStampExt( float *origin, float *left, float *up, int *color,
                                float s1, float t1, float s2, float t2 )
{
  int    numVertexes;
  unsigned short ndx;
  float *xyz;
  vec3_t normal;

  if ( tess_numVertexes + 4 >= SHADER_MAX_VERTEXES || tess_numIndexes + 6 >= SHADER_MAX_INDEXES )
    RB_CheckOverflow( 4, 6 );   /* inlined by VC7, as retail */

  numVertexes = tess_numVertexes;
  xyz         = &tess_xyz[ tess_vertexComponentCount * numVertexes ];

  tess_indexes[ tess_numIndexes     ] = (unsigned short)( numVertexes     );
  tess_indexes[ tess_numIndexes + 1 ] = (unsigned short)( numVertexes + 1 );
  tess_indexes[ tess_numIndexes + 2 ] = (unsigned short)( numVertexes + 3 );
  tess_indexes[ tess_numIndexes + 3 ] = (unsigned short)( numVertexes + 3 );
  tess_indexes[ tess_numIndexes + 4 ] = (unsigned short)( numVertexes + 1 );
  tess_indexes[ tess_numIndexes + 5 ] = (unsigned short)( numVertexes + 2 );

  *xyz++ = left[0] + up[0]     + origin[0];
  *xyz++ = (double)left[1] + origin[1] + up[1];
  *xyz++ = (double)left[2] + origin[2] + up[2];

  *xyz++ = origin[0] - left[0] + up[0];
  *xyz++ = origin[1] - left[1] + up[1];
  *xyz++ = origin[2] - left[2] + up[2];

  *xyz++ = origin[0] - left[0] - up[0];
  *xyz++ = origin[1] - left[1] - up[1];
  *xyz++ = origin[2] - left[2] - up[2];

  *xyz++ = left[0] + origin[0] - up[0];
  *xyz++ = left[1] + origin[1] - up[1];
  *xyz   = left[2] + origin[2] - up[2];

  /* constant normal, opposite the view forward axis */
  VectorSubtract( vec3_origin, &backEnd_viewParms_axis00, normal );

  ndx = numVertexes;
  ((vec3_t *)tess_stageNormals)[ ndx ][0] = ((vec3_t *)tess_stageNormals)[ ndx + 1 ][0] = ((vec3_t *)tess_stageNormals)[ ndx + 2 ][0] = ((vec3_t *)tess_stageNormals)[ ndx + 3 ][0] = normal[0];
  ((vec3_t *)tess_stageNormals)[ ndx ][1] = ((vec3_t *)tess_stageNormals)[ ndx + 1 ][1] = ((vec3_t *)tess_stageNormals)[ ndx + 2 ][1] = ((vec3_t *)tess_stageNormals)[ ndx + 3 ][1] = normal[1];
  ((vec3_t *)tess_stageNormals)[ ndx ][2] = ((vec3_t *)tess_stageNormals)[ ndx + 1 ][2] = ((vec3_t *)tess_stageNormals)[ ndx + 2 ][2] = ((vec3_t *)tess_stageNormals)[ ndx + 3 ][2] = normal[2];

  tess_texCoords0[ 2 * ndx     ] = tess_texCoords1[ 2 * ndx     ] = s1;
  tess_texCoords0[ 2 * ndx + 1 ] = tess_texCoords1[ 2 * ndx + 1 ] = t1;
  tess_texCoords0[ 2 * ndx + 2 ] = tess_texCoords1[ 2 * ndx + 2 ] = s2;
  tess_texCoords0[ 2 * ndx + 3 ] = tess_texCoords1[ 2 * ndx + 3 ] = t1;
  tess_texCoords0[ 2 * ndx + 4 ] = tess_texCoords1[ 2 * ndx + 4 ] = s2;
  tess_texCoords0[ 2 * ndx + 5 ] = tess_texCoords1[ 2 * ndx + 5 ] = t2;
  tess_texCoords0[ 2 * ndx + 6 ] = tess_texCoords1[ 2 * ndx + 6 ] = s1;
  tess_texCoords0[ 2 * ndx + 7 ] = tess_texCoords1[ 2 * ndx + 7 ] = t2;

  /* constant color across all four vertices */
  ((int *)tess_vertexColors)[ndx    ] = ((int *)tess_vertexColors)[ndx + 1]
    = ((int *)tess_vertexColors)[ndx + 2] = ((int *)tess_vertexColors)[ndx + 3] = *color;

  tess_numVertexes += 4;
  tess_numIndexes  += 6;
  return tess_numIndexes;
}

/* ---- RB_AddQuadStamp  0x00510120 ----  VERIFIED */
int __cdecl RB_AddQuadStamp( float *up, float *left, int *color, float *origin )
{
  return RB_AddQuadStampExt( origin, left, up, color, 0.0f, 0.0f, 1.0f, 1.0f );
}

/* ---- RB_SurfaceSplash  0x00510140 ----  VERIFIED */
int RB_SurfaceSplash( void )
{
  rbSurfEntity_t *e;
  float  radius;
  float  left[3], up[3];

  e      = (rbSurfEntity_t *)backEnd_currentEntity;
  radius = e->radius;

  VectorSet( left, -radius, 0, 0 );
  VectorSet( up, 0, radius, 0 );

  if ( backEnd_viewParms_isMirror )
    VectorSubtract( vec3_origin, left, left );

  return RB_AddQuadStampExt( e->origin, left, up, (int *)e->shaderRGBA,
                             0.0f, 0.0f, 1.0f, 1.0f );
}

/* ---- RB_SurfaceSprite  0x005101D0 ----  [CONFIRMED]
 * A camera-facing quad; a nonzero entity rotation spins it about the view
 * forward axis. */
int RB_SurfaceSprite( void )
{
  float  radius, radius2;
  float  left[3], up[3];

  radius  = ( (rbSurfEntity_t *)backEnd_currentEntity )->radius;
  radius2 = ( (rbSurfEntity_t *)backEnd_currentEntity )->radius2;

  if ( ( (rbSurfEntity_t *)backEnd_currentEntity )->rotation == 0.0f )
  {
    VectorScale( &backEnd_viewParms_axis10, radius, left );
    VectorScale( &backEnd_viewParms_axis20, radius2, up );
  }
  else
  {
    float  ang, s, c;
#ifndef COD1_VC7
    float  ns, sr2;
    float  l1, l2, u1, u2;
    double crD, nsD, cr2D, sr2D;
#endif

    ang = ( (rbSurfEntity_t *)backEnd_currentEntity )->rotation * 0.017453292f;
    RB_SinCos( ang, &s, &c );   /* retail 0x00510270: inline fsincos block */

#ifdef COD1_VC7
    VectorScale( &backEnd_viewParms_axis10, c * radius, left );
    VectorMA( left, -s * radius, &backEnd_viewParms_axis20, left );

    VectorScale( &backEnd_viewParms_axis20, c * radius2, up );
    VectorMA( up, s * radius2, &backEnd_viewParms_axis10, up );
#else
    /* Retail operation order (0x00510285-0x00510355): cos*radius and
     * cos*radius2 stay unrounded on the FPU, as do the [0] uses of
     * -(sin*radius) and sin*radius2 (fst then reuse st0); the [1]/[2]
     * partial products and the [1]/[2] uses of the sin terms are floats. */

    /* left = cos*radius * viewAxis[1] - sin*radius * viewAxis[2] */
    crD = (double)c * radius;                       /* fld cos ; fmul radius (kept) */
    l1  = (float)( crD * backEnd_viewParms_axis11 );  /* [esp+20h] */
    l2  = (float)( crD * backEnd_viewParms_axis12 );  /* [esp+24h] */
    nsD = -( (double)s * radius );                  /* fchs ; fst [esp] (kept) */
    ns  = (float)nsD;
    left[0] = (float)( nsD * backEnd_viewParms_axis20 + backEnd_viewParms_axis10 * crD );
    left[1] = (float)( (double)ns * backEnd_viewParms_axis21 + l1 );
    left[2] = (float)( (double)ns * backEnd_viewParms_axis22 + l2 );

    /* up = cos*radius2 * viewAxis[2] + sin*radius2 * viewAxis[1] */
    cr2D = (double)c * radius2;                     /* fld cos ; fmul radius2 (kept) */
    u1   = (float)( cr2D * backEnd_viewParms_axis21 );  /* [esp+2Ch] */
    u2   = (float)( cr2D * backEnd_viewParms_axis22 );  /* [esp+30h] */
    sr2D = (double)s * radius2;                     /* fst [esp+0Ch] (kept) */
    sr2  = (float)sr2D;
    up[0] = (float)( sr2D * backEnd_viewParms_axis10 + backEnd_viewParms_axis20 * cr2D );
    up[1] = (float)( (double)sr2 * backEnd_viewParms_axis11 + u1 );
    up[2] = (float)( (double)sr2 * backEnd_viewParms_axis12 + u2 );
#endif
  }

  if ( backEnd_viewParms_isMirror )
    VectorSubtract( vec3_origin, left, left );

  return RB_AddQuadStampExt( ( (rbSurfEntity_t *)backEnd_currentEntity )->origin, left, up,
                             (int *)( (rbSurfEntity_t *)backEnd_currentEntity )->shaderRGBA,
                             0.0f, 0.0f, 1.0f, 1.0f );
}

/* ---- RB_SurfaceOrientedQuad  0x005103C0 ----  VERIFIED */
int RB_SurfaceOrientedQuad( void )
{
  float  width, height;
  float  right[3], up[3];

  width  = ( (rbSurfEntity_t *)backEnd_currentEntity )->radius;
  height = ( (rbSurfEntity_t *)backEnd_currentEntity )->radius2;

  MakeNormalVectors( ( (rbSurfEntity_t *)backEnd_currentEntity )->axis[0], right, up );

  if ( ( (rbSurfEntity_t *)backEnd_currentEntity )->rotation == 0.0f )
  {
    VectorScale( right, width, right );
    VectorScale( up, height, up );
  }
  else
  {
    float  ang, s, c;
#ifdef COD1_VC7
    float  tmp[3], tmp2[3];
#else
    double cw, nswD, ch, shD, rr0, rr1, u0;
    float  nsw, sh, t0, t1, t2, u1, u2;
    float  rotRight[3];
#endif

    ang = ( (rbSurfEntity_t *)backEnd_currentEntity )->rotation * 0.017453292f;
    RB_SinCos( ang, &s, &c );   /* retail 0x0051046B: inline fsincos block */

#ifdef COD1_VC7
    VectorScale( right, c * width, tmp );
    VectorMA( tmp, -s * width, up, tmp );

    VectorScale( up, c * height, tmp2 );
    VectorMA( tmp2, s * height, right, up );

    VectorCopy( tmp, right );
#else
    /* right' = right*(c*w) + up*(-(s*w)) ; up' = right*(s*h) + up*(c*h), in
     * retail's operation order (0x00510480-0x00510544): c*w, c*h and the
     * first uses of -(s*w), s*h and up[0]*(c*h) stay unrounded on the FPU;
     * the other partial products are spilled as floats.  up' reads the
     * unrotated right, which is written back last. */
    cw = (double)c * width;                        /* fld cos ; fmul width (kept) */
    t0 = (float)( right[0] * cw );                  /* var_18 */
    t1 = (float)( right[1] * cw );                  /* var_14 */
    t2 = (float)( right[2] * cw );                  /* var_10 */
    nswD = -( (double)s * width );                  /* fld sin ; fmul width ; fchs */
    nsw  = (float)nswD;                             /* fst var_4C */
    rr0  = nswD * up[0] + t0;                       /* kept on FPU */
    rr1  = (double)nsw * up[1] + t1;                /* kept on FPU */
    rotRight[2] = (float)( (double)nsw * up[2] + t2 );
    ch = (double)c * height;                        /* fld cos ; fmul height (kept) */
    u0 = up[0] * ch;                                /* kept */
    u1 = (float)( up[1] * ch );                     /* var_8 */
    u2 = (float)( up[2] * ch );                     /* var_4 */
    shD = (double)s * height;
    sh  = (float)shD;                               /* fst var_40 */
    up[0] = (float)( shD * right[0] + u0 );
    up[1] = (float)( (double)sh * right[1] + u1 );
    up[2] = (float)( (double)sh * right[2] + u2 );
    right[0] = (float)rr0;
    right[1] = (float)rr1;
    right[2] = rotRight[2];
#endif
  }

  if ( backEnd_viewParms_isMirror )
    VectorSubtract( vec3_origin, right, right );

  return RB_AddQuadStampExt( ( (rbSurfEntity_t *)backEnd_currentEntity )->origin, right, up,
                             (int *)( (rbSurfEntity_t *)backEnd_currentEntity )->shaderRGBA,
                             0.0f, 0.0f, 1.0f, 1.0f );
}

/* ---- RB_SurfacePolychain  0x005105B0 ----  VERIFIED
 * Triangle-fans the source poly into the tess arrays, then marks which world
 * dlights actually reach it (per-entity lit shaders only). */
void __cdecl RB_SurfacePolychain( srfPoly_t *p )
{
  int          i, v;
  float       *xyz;
  float       *st0;
  float       *st1;
  int         *colors;
  dlight_t    *dl;
  float        radiusSq;
  rfloat       dx, dy, dz;   /* x87 stack in retail, never rounded to float */

  if ( tess_numVertexes + p->numVerts >= SHADER_MAX_VERTEXES
    || tess_numIndexes + 3 * ( p->numVerts - 2 ) >= SHADER_MAX_INDEXES )
    RB_CheckOverflow( p->numVerts, 3 * ( p->numVerts - 2 ) );

  /* fan triangles into the tess array */
  xyz    = &tess_xyz[ tess_vertexComponentCount * tess_numVertexes ];
  st0    = &tess_texCoords0[ 2 * tess_numVertexes ];
  st1    = &tess_texCoords1[ 2 * tess_numVertexes ];
  colors = &((int *)tess_vertexColors)[ tess_numVertexes ];
  for ( i = 0; i < p->numVerts; i++ )
  {
    VectorCopy( p->verts[i].xyz, xyz );
    st0[0] = p->verts[i].st[0];
    st0[1] = p->verts[i].st[1];
    st1[0] = p->verts[i].lightmap[0];
    st1[1] = p->verts[i].lightmap[1];
    *colors = *(int *)p->verts[i].modulate;
    xyz += tess_vertexComponentCount;
    st0 += 2;
    st1 += 2;
    colors++;
  }

  /* shader lighting flag bit (per-entity lit): bit 7 of the byte at +0x54 */
  if ( tr_refdef_num_dlights && ( *(unsigned char *)( tess_shader + 84 ) & 0x80 ) )
  {
    for ( i = 0; i < tr_refdef_num_dlights; i++ )
    {
      dl = &( (dlight_t *)tr_refdef_dlights )[i];
      if ( tess_dlightBits & ( 1 << i ) )
        continue;

      radiusSq = dl->radius * dl->radius;
      for ( v = 0; v < p->numVerts; v++ )
      {
        /* 0x00510700-0x00510721: the deltas stay unrounded on the FPU and
         * the squares are summed z, y, x before the fcomp with radiusSq */
        dx = (rfloat)p->verts[v].xyz[0] - dl->transformedPosition[0];
        dy = (rfloat)p->verts[v].xyz[1] - dl->transformedPosition[1];
        dz = (rfloat)p->verts[v].xyz[2] - dl->transformedPosition[2];
        if ( dz * dz + dy * dy + dx * dx < radiusSq )
        {
          tess_dlightBits |= 1 << i;
          break;
        }
      }
    }
  }

  /* generate fan indexes into the tess array */
  for ( i = 0; i < p->numVerts - 2; i++ )
  {
    tess_indexes[ tess_numIndexes + 0 ] = tess_numVertexes;
    tess_indexes[ tess_numIndexes + 1 ] = tess_numVertexes + i + 1;
    tess_indexes[ tess_numIndexes + 2 ] = tess_numVertexes + i + 2;
    tess_numIndexes += 3;
  }

  tess_numVertexes += p->numVerts;
}

#define NUM_BEAM_SEGS 6

/* ---- RB_SurfaceBeam  0x00510800 ----  VERIFIED */
void RB_SurfaceBeam( void )
{
  rbSurfEntity_t *e;
  int    i;
  float  perpvec[3];
  float  direction[3], normalized_direction[3];
  float  start_points[NUM_BEAM_SEGS][3], end_points[NUM_BEAM_SEGS][3];
  float  oldorigin[3], origin[3];

  e = (rbSurfEntity_t *)backEnd_currentEntity;

  oldorigin[0] = e->oldorigin[0];
  oldorigin[1] = e->oldorigin[1];
  oldorigin[2] = e->oldorigin[2];

  origin[0] = e->origin[0];
  origin[1] = e->origin[1];
  origin[2] = e->origin[2];

  normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
  normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
  normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

  if ( VectorNormalize( normalized_direction ) == 0 )
    return;

  PerpendicularVector( perpvec, normalized_direction );

  VectorScale( perpvec, 4, perpvec );

  for ( i = 0; i < NUM_BEAM_SEGS; i++ )
  {
    RotatePointAroundVector( start_points[i], normalized_direction, perpvec,
                             ( 360.0f / NUM_BEAM_SEGS ) * i );
    VectorAdd( start_points[i], direction, end_points[i] );
  }

  GL_Bind( (GLenum *)tr_whiteImage );
  GL_State( 34 );                       /* GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE */
  RB_BeginImmediateMode();

  rendererDebugState.immediateColorR = -1;
  rendererDebugState.immediateColorG = 0;
  rendererDebugState.immediateColorB = 0;
  rendererDebugState.immediateColorA = -1;
  rendererDebugState.immediateMode = 5;                             /* GL_TRIANGLE_STRIP */

  for ( i = 0; i <= NUM_BEAM_SEGS; i++ )
  {
    RB_glVertex3fv( start_points[i % NUM_BEAM_SEGS] );
    RB_glVertex3fv( end_points[i % NUM_BEAM_SEGS] );
  }

  RB_glEnd();
  RB_EndImmediateMode();
}

/* ---- DoLine  0x00511010 ----  VERIFIED
 * side is the already-normalized quad-width direction; radius offsets each of
 * the two segment endpoints to both sides. */
void __cdecl DoLine( float *start, float *end, float *side, float spanWidth )
{
  float  negSpan;
  int    color;
  float *xyz;
  float *st;
  int   *colors;
  unsigned short *indexes;
  int    vbase;

  /* RB_CHECKOVERFLOW( 4, 6 ): retail inlines RB_CheckOverflow here */
  RB_CHECKOVERFLOW_EXPANDED( 4, 6 );

  /* stream pointers built by +=, color read after st: retail's load order */
  xyz     = tess_xyz;
  xyz    += tess_vertexComponentCount * tess_numVertexes;
  negSpan = -spanWidth;
  st      = tess_texCoords0;
  st     += 2 * tess_numVertexes;
  color   = *(int *)( (rbSurfEntity_t *)backEnd_currentEntity )->shaderRGBA;
  colors  = (int *)tess_vertexColors;
  colors += tess_numVertexes;

  xyz[0] = spanWidth * side[0] + start[0];
  xyz[1] = spanWidth * side[1] + start[1];
  xyz[2] = spanWidth * side[2] + start[2];
  xyz += tess_vertexComponentCount;
  *st++ = 0.0f;
  *st++ = 0.0f;
  *colors++ = color;

  xyz[0] = negSpan * side[0] + start[0];
  xyz[1] = negSpan * side[1] + start[1];
  xyz[2] = negSpan * side[2] + start[2];
  xyz += tess_vertexComponentCount;
  *st++ = 1.0f;
  *st++ = 0.0f;
  *colors++ = color;

  xyz[0] = spanWidth * side[0] + end[0];
  xyz[1] = spanWidth * side[1] + end[1];
  xyz[2] = spanWidth * side[2] + end[2];
  xyz += tess_vertexComponentCount;
  *st++ = 0.0f;
  *st++ = 1.0f;
  *colors++ = color;

  xyz[0] = negSpan * side[0] + end[0];
  xyz[1] = negSpan * side[1] + end[1];
  xyz[2] = negSpan * side[2] + end[2];
  *st++ = 1.0f;
  *st++ = 1.0f;
  *colors++ = color;

  indexes = tess_indexes;
  vbase   = tess_numVertexes;
  indexes[tess_numIndexes++] = (unsigned short)( vbase     );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 1 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 2 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 2 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 1 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 3 );

  tess_numVertexes += 4;
}

/* ---- RB_SurfaceLine  0x00511250 ----  [HIGH] */
void RB_SurfaceLine( void )
{
  rbSurfEntity_t *e;
  float  start[3], end[3];
  float  startFromView[3], endFromView[3];
  float  side[3];

  e = (rbSurfEntity_t *)backEnd_currentEntity;

  /* segment runs origin -> oldorigin */
  end[0]   = e->oldorigin[0];
  end[1]   = e->oldorigin[1];
  end[2]   = e->oldorigin[2];
  start[0] = e->origin[0];
  start[1] = e->origin[1];
  start[2] = e->origin[2];

  startFromView[0] = start[0] - backEnd_viewParms_originX[0];
  startFromView[1] = start[1] - backEnd_viewParms_originY;
  startFromView[2] = start[2] - backEnd_viewParms_originZ;
  endFromView[0]   = end[0] - backEnd_viewParms_originX[0];
  endFromView[1]   = end[1] - backEnd_viewParms_originY;
  endFromView[2]   = end[2] - backEnd_viewParms_originZ;
  CrossProduct( startFromView, endFromView, side );
  VectorNormalize( side );

  DoLine( start, end, side, e->radius );
}

#define CYLINDER_MIN_SEGS   8      /* cmp eax, 8    @0x00511403 */
#define CYLINDER_MAX_SEGS  32      /* cmp eax, 20h  @0x00511416 */

/* ---- RB_SurfaceCylinder  0x00511350 ----  VERIFIED
 * A view-adaptive tube between origin (radius2) and oldorigin (radius). */
void RB_SurfaceCylinder( void )
{
  rbSurfEntity_t *e;
  int    i;
  int    segs;
  int    color;
  float  lodDist;
  float  step;
  float  angle;
  float  c, s;
  float  right[3], up[3];
  float  start[CYLINDER_MAX_SEGS + 1][3];
  float  end[CYLINDER_MAX_SEGS + 1][3];
  float *xyz;
  float *st;
  int   *colors;

  e = (rbSurfEntity_t *)backEnd_currentEntity;

  {
    float  viewDir[3];

    VectorAdd( e->origin, e->oldorigin, viewDir );
    VectorScale( viewDir, 0.5f, viewDir );
    VectorSubtract( viewDir, backEnd_viewParms_originX, viewDir );
    lodDist = VectorNormalize( viewDir );
  }

  /* 0x005113F3-0x005113FB: FastRound inlined (fld / fadd qword 2^-30 / fistp),
   * round-to-nearest, not the truncating __ftol2 of an (int) cast; the chain
   * before it is 53-bit x87 with float constants */
#ifdef COD1_VC7
  segs = FastRound( ( 1.0f - lodDist * ( backEnd_viewParms_fovX / 90.0f ) / 1024.0f ) * 32.0f );
#else
  segs = FastRound( (float)( ( 1.0 - lodDist * ( backEnd_viewParms_fovX * (double)0.011111111f )
                                       * 0.0009765625 ) * 32.0 ) );
#endif
  if ( segs < CYLINDER_MIN_SEGS )
    segs = CYLINDER_MIN_SEGS;
  else if ( segs > CYLINDER_MAX_SEGS )
    segs = CYLINDER_MAX_SEGS;

  if ( tess_numVertexes + 2 * segs + 2 >= SHADER_MAX_VERTEXES
    || tess_numIndexes + 6 * segs >= SHADER_MAX_INDEXES )
    RB_CheckOverflow( 2 * segs + 2, 6 * segs );

  {
    float  forward[3];

    VectorSubtract( e->origin, e->oldorigin, forward );
    VectorNormalize( forward );
    MakeNormalVectors( forward, right, up );
  }

  /* 0x0051149A: fdivr 1.0f, fst (the texture step, reused below), then fmul
   * on the unrounded quotient */
#ifdef COD1_VC7
  step = ( 1.0f / segs ) * 6.2831855f;
#else
  step = (float)( ( 1.0 / (double)segs ) * (double)6.2831855f );
#endif

  for ( i = 0; i < segs; i++ )
  {
    angle = i * step;
    RB_SinCos( angle, &s, &c );   /* retail 0x005114ED: inline fsincos block */

    start[i][0] = up[0] * ( c * e->radius2 ) + right[0] * ( s * e->radius2 ) + e->origin[0];
    start[i][1] = up[1] * ( c * e->radius2 ) + right[1] * ( s * e->radius2 ) + e->origin[1];
    start[i][2] = up[2] * ( c * e->radius2 ) + right[2] * ( s * e->radius2 ) + e->origin[2];

    end[i][0] = up[0] * ( c * e->radius ) + right[0] * ( s * e->radius ) + e->oldorigin[0];
    end[i][1] = up[1] * ( c * e->radius ) + right[1] * ( s * e->radius ) + e->oldorigin[1];
    end[i][2] = up[2] * ( c * e->radius ) + right[2] * ( s * e->radius ) + e->oldorigin[2];
  }

  /* close the ring: element [segs] repeats element [0] */
  VectorCopy( start[0], start[segs] );
  VectorCopy( end[0], end[segs] );

  step = 1.0f / segs;

  for ( i = 0; i < segs; i++ )
  {
    tess_indexes[ tess_numIndexes     ] = (unsigned short)( tess_numVertexes + 2 * i     );
    tess_indexes[ tess_numIndexes + 1 ] = (unsigned short)( tess_numVertexes + 2 * i + 1 );
    tess_indexes[ tess_numIndexes + 2 ] = (unsigned short)( tess_numVertexes + 2 * i + 3 );
    tess_numIndexes += 3;

    tess_indexes[ tess_numIndexes     ] = (unsigned short)( tess_numVertexes + 2 * i + 3 );
    tess_indexes[ tess_numIndexes + 1 ] = (unsigned short)( tess_numVertexes + 2 * i + 2 );
    tess_indexes[ tess_numIndexes + 2 ] = (unsigned short)( tess_numVertexes + 2 * i     );
    tess_numIndexes += 3;
  }

  xyz    = &tess_xyz[ tess_vertexComponentCount * tess_numVertexes ];
  st     = &tess_texCoords0[ 2 * tess_numVertexes ];
  colors = &((int *)tess_vertexColors)[ tess_numVertexes ];
  /* 0x005116C9: the color re-reads backEnd_currentEntity, which an overflow
   * flush above resets to tess_entity; origins and radii use the entry e */
  color  = *(int *)( (rbSurfEntity_t *)backEnd_currentEntity )->shaderRGBA;

  for ( i = 0; i <= segs; i++ )
  {
    VectorCopy( start[i], xyz );
    xyz += tess_vertexComponentCount;
    *st++ = i * step;
    *st++ = 1.0f;
    *colors++ = color;

    VectorCopy( end[i], xyz );
    xyz += tess_vertexComponentCount;
    *st++ = i * step;
    *st++ = 0.0f;
    *colors++ = color;
  }

  tess_numVertexes += 2 * i;
}

/* ---- DoRailCore  0x005119F0 ----  VERIFIED
 * A single flat quad spanning start..end.  start gets texture S = 0, end gets
 * S = length / 256; width offsets both endpoints to either side. */
void __cdecl DoRailCore( float *start, float *end, float *up, float length, float width )
{
  float  t;
  float  negWidth;
  int    color;
  float *xyz;
  float *st;
  int   *colors;
  unsigned short *indexes;
  int    vbase;

  /* stream pointers built by +=, color read after st: see DoLine */
  t        = length * 0.00390625f;                  /* length / 256 */
  negWidth = -width;
  xyz=tess_xyz+tess_numVertexes*tess_vertexComponentCount;
  st       = tess_texCoords0;
  st      += 2 * tess_numVertexes;
  color    = *(int *)( (rbSurfEntity_t *)backEnd_currentEntity )->shaderRGBA;
  colors=(int *)tess_vertexColors+tess_numVertexes;

  /* vertex 0: start + width * up,  st (0, 0) */
  xyz[0] = width * up[0] + start[0];
  xyz[1] = width * up[1] + start[1];
  xyz[2] = width * up[2] + start[2];
  xyz += tess_vertexComponentCount;
  *st++ = 0.0f;
  *st++ = 0.0f;
  *colors++ = color;

  /* vertex 1: start - width * up,  st (0, 1) */
  xyz[0] = negWidth * up[0] + start[0];
  xyz[1] = negWidth * up[1] + start[1];
  xyz[2] = negWidth * up[2] + start[2];
  xyz += tess_vertexComponentCount;
  *st++ = 0.0f;
  *st++ = 1.0f;
  *colors++ = color;

  /* vertex 2: end + width * up,  st (t, 0) */
  xyz[0] = width * up[0] + end[0];
  xyz[1] = width * up[1] + end[1];
  xyz[2] = width * up[2] + end[2];
  xyz += tess_vertexComponentCount;
  *st++ = t;
  *st++ = 0.0f;
  *colors++ = color;

  /* vertex 3: end - width * up,  st (t, 1) */
  xyz[0] = negWidth * up[0] + end[0];
  xyz[1] = negWidth * up[1] + end[1];
  xyz[2] = negWidth * up[2] + end[2];
  *st++ = t;
  *st++ = 1.0f;
  *colors++ = color;

  indexes = tess_indexes;
  vbase   = tess_numVertexes;
  indexes[tess_numIndexes++] = (unsigned short)( vbase     );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 1 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 2 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 2 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 1 );
  indexes[tess_numIndexes++] = (unsigned short)( vbase + 3 );

  tess_numVertexes += 4;
}

/* ---- DoRailDiscs  0x00511BE0 ----  VERIFIED */
void __cdecl DoRailDiscs( int numSegs, float *start, float *dir,
                          float *right, float *up )
{
  int    i;
  float  pos[4][3];
  rfloat v[3];     /* retail: v[0], v[1] on the x87 stack, v[2] spilled (0x00511CBD) */
  int    spanWidth;
  float  c, s;
  int    color;
  float *xyz;
  float *st;
  int   *colors;

  spanWidth = r_railWidth->integer;

  if ( numSegs > 1 )
    numSegs--;
  if ( !numSegs )
    return;

  for ( i = 0; i < 4; i++ )
  {
    /* 0x00511C40: fild; fmul dword pi; fmul dword 1/180 (float constants) */
    RB_SinCos( FCAST( ( 45 + i * 90 ) * 3.1415927f ) * 0.0055555557f, &s, &c );

    v[0] = ( right[0] * c + up[0] * s ) * 0.25f * spanWidth;
    v[1] = ( right[1] * c + up[1] * s ) * 0.25f * spanWidth;
    v[2] = ( right[2] * c + up[2] * s ) * 0.25f * spanWidth;

    VectorAdd( start, v, pos[i] );

    /* retail re-adds from the stored floats for [0] and [2] (0x00511CE2,
       0x00511CF0) but from the unrounded v[1] + start[1] still on the stack */
    if ( numSegs > 1 )
    {
#ifdef COD1_VC7
      VectorAdd( pos[i], dir, pos[i] );
#else
      pos[i][0] = pos[i][0] + dir[0];
      pos[i][1] = FCAST( ( start[1] + v[1] ) + dir[1] );
      pos[i][2] = pos[i][2] + dir[2];
#endif
    }
  }

  color = *(int *)( (rbSurfEntity_t *)backEnd_currentEntity )->shaderRGBA;

  for ( i = 0; i < numSegs; i++ )
  {
    int j;

    RB_CHECKOVERFLOW_EXPANDED( 4, 6 );

    xyz    = &tess_xyz[ tess_vertexComponentCount * tess_numVertexes ];
    st     = &tess_texCoords0[ 2 * tess_numVertexes ];
    colors = &((int *)tess_vertexColors)[ tess_numVertexes ];

    for ( j = 0; j < 4; j++ )
    {
      VectorCopy( pos[j], xyz );
      xyz += tess_vertexComponentCount;
      *st++ = ( j < 2 );
      *st++ = ( j && j != 3 );
      *colors++ = color;
      VectorAdd( pos[j], dir, pos[j] );
    }

    tess_indexes[ tess_numIndexes++ ] = tess_numVertexes;
    tess_indexes[ tess_numIndexes++ ] = tess_numVertexes + 1;
    tess_indexes[ tess_numIndexes++ ] = tess_numVertexes + 3;
    tess_indexes[ tess_numIndexes++ ] = tess_numVertexes + 3;
    tess_indexes[ tess_numIndexes++ ] = tess_numVertexes + 1;
    tess_indexes[ tess_numIndexes++ ] = tess_numVertexes + 2;
    tess_numVertexes += 4;
  }
}

/* ---- RB_SurfaceRailRings  0x00511F10 ----  VERIFIED */
void RB_SurfaceRailRings( void )
{
  rbSurfEntity_t *e;
  float  start[3], end[3], direction[3], right[3], up[3];
  float  length;
  int    segs;

  e = (rbSurfEntity_t *)backEnd_currentEntity;

  start[0] = e->oldorigin[0];
  start[1] = e->oldorigin[1];
  start[2] = e->oldorigin[2];
  end[0]   = e->origin[0];
  end[1]   = e->origin[1];
  end[2]   = e->origin[2];

  direction[0] = end[0] - start[0];
  direction[1] = end[1] - start[1];
  direction[2] = end[2] - start[2];
  length = VectorNormalize( direction );
  MakeNormalVectors( direction, right, up );

  segs = (int)( length / r_railSegmentLength->value );
  if ( segs <= 0 )
    segs = 1;

  /* direction becomes the per-segment step */
  direction[0] = direction[0] * r_railSegmentLength->value;
  direction[1] = direction[1] * r_railSegmentLength->value;
  direction[2] = direction[2] * r_railSegmentLength->value;

  DoRailDiscs( segs, start, direction, right, up );
}

/* ---- RB_SurfaceRailCore  0x00511FE0 ----  [HIGH] */
void RB_SurfaceRailCore( void )
{
  rbSurfEntity_t *e;
  float  start[3], end[3], direction[3];
  float  startFromView[3], endFromView[3];
  float  length;

  e = (rbSurfEntity_t *)backEnd_currentEntity;

  /* segment runs oldorigin (S = 0) -> origin (S = t) */
  start[0] = e->oldorigin[0];
  start[1] = e->oldorigin[1];
  start[2] = e->oldorigin[2];
  end[0]   = e->origin[0];
  end[1]   = e->origin[1];
  end[2]   = e->origin[2];

  direction[0] = end[0] - start[0];
  direction[1] = end[1] - start[1];
  direction[2] = end[2] - start[2];
  length = VectorNormalize( direction );

  startFromView[0] = start[0] - backEnd_viewParms_originX[0];
  startFromView[1] = start[1] - backEnd_viewParms_originY;
  startFromView[2] = start[2] - backEnd_viewParms_originZ;
  VectorNormalize( startFromView );
  endFromView[0]   = end[0] - backEnd_viewParms_originX[0];
  endFromView[1]   = end[1] - backEnd_viewParms_originY;
  endFromView[2]   = end[2] - backEnd_viewParms_originZ;
  VectorNormalize( endFromView );

  /* side = cross( startFromView, endFromView ), into the spent direction */
  direction[0] = startFromView[1] * endFromView[2] - startFromView[2] * endFromView[1];
  direction[1] = startFromView[2] * endFromView[0] - startFromView[0] * endFromView[2];
  direction[2] = startFromView[0] * endFromView[1] - startFromView[1] * endFromView[0];
  VectorNormalize( direction );

  DoRailCore( start, end, direction, length, r_railCoreWidth->value );
}

/* ---- RB_SurfaceLightningBolt  0x00512130 ----  [HIGH] */
void RB_SurfaceLightningBolt( void )
{
  rbSurfEntity_t *e;
  float  start[3], end[3], direction[3];
  float  startFromView[3], endFromView[3], side[3], rotatedSide[3];
  float  length;
  int    i;

  e = (rbSurfEntity_t *)backEnd_currentEntity;

  /* segment runs origin (S = 0) -> oldorigin (S = t) */
  end[0]   = e->oldorigin[0];
  end[1]   = e->oldorigin[1];
  end[2]   = e->oldorigin[2];
  start[0] = e->origin[0];
  start[1] = e->origin[1];
  start[2] = e->origin[2];

  direction[0] = end[0] - start[0];
  direction[1] = end[1] - start[1];
  direction[2] = end[2] - start[2];
  length = VectorNormalize( direction );

  startFromView[0] = start[0] - backEnd_viewParms_originX[0];
  startFromView[1] = start[1] - backEnd_viewParms_originY;
  startFromView[2] = start[2] - backEnd_viewParms_originZ;
  VectorNormalize( startFromView );
  endFromView[0]   = end[0] - backEnd_viewParms_originX[0];
  endFromView[1]   = end[1] - backEnd_viewParms_originY;
  endFromView[2]   = end[2] - backEnd_viewParms_originZ;
  VectorNormalize( endFromView );

  /* side = cross( startFromView, endFromView ) */
  side[0] = startFromView[1] * endFromView[2] - startFromView[2] * endFromView[1];
  side[1] = startFromView[2] * endFromView[0] - startFromView[0] * endFromView[2];
  side[2] = startFromView[0] * endFromView[1] - startFromView[1] * endFromView[0];
  VectorNormalize( side );

  for ( i = 0; i < 4; i++ )
  {
    DoRailCore( start, end, side, length, 8.0 );
    RotatePointAroundVector( rotatedSide, direction, side, 45.0 );
    side[0] = rotatedSide[0];
    side[1] = rotatedSide[1];
    side[2] = rotatedSide[2];
  }
}

/* ---- RB_SurfaceTriangles  0x005122B0 ----  [HIGH] */
void __cdecl RB_SurfaceTriangles( worldMeshSurf_t *surface )
{
  unsigned short *indexes;
  unsigned short *src;
  int  baseVertex;
  int  numVerts;
  int  i;

  if ( surface->numVerts + tess_numVertexes >= SHADER_MAX_VERTEXES
    || surface->numIndexes + tess_numIndexes >= SHADER_MAX_INDEXES )
    RB_CheckOverflow( surface->numVerts, surface->numIndexes );

  tess_dlightBits |= surface->dlightBits;

  /* adjusted indexes, copied back to front */
  src        = surface->indices;
  indexes    = &tess_indexes[ tess_numIndexes ];
  baseVertex = tess_numVertexes;
  for ( i = surface->numIndexes - 1; i >= 0; i-- )
    indexes[i] = (unsigned short)( baseVertex + src[i] );
  tess_numIndexes += surface->numIndexes;

  numVerts = surface->numVerts;
  if ( ( *(unsigned char *)( tess_shader + 80 ) & 2 ) != 0
    || ( r_dlightQuality->integer && tess_dlightBits ) )
  {
    Com_Memcpy( &tess_stageNormals[ 3 * tess_numVertexes ], surface->normals, 12 * numVerts );
    tess_requiresVertexBasis = 1;

    if ( ( *(unsigned char *)( tess_shader + 80 ) & 0xC ) != 0 )
    {
      if ( surface->tangents )
      {
        Com_Memcpy( &tess_stageTangents[ 3 * tess_numVertexes ], surface->tangents, 12 * numVerts );
        tess_stageTangentsValid = 1;
      }
      if ( surface->bitangents )
      {
        Com_Memcpy( &tess_stageBitangents[ 3 * tess_numVertexes ], surface->bitangents, 12 * numVerts );
        tess_stageBitangentsValid = 1;
      }
    }
  }

  Com_Memcpy( &tess_xyz[ 3 * tess_numVertexes ], surface->positions, 12 * numVerts );
  Com_Memcpy( &tess_texCoords0[ 2 * tess_numVertexes ], surface->texCoords, 8 * numVerts );
  Com_Memcpy( &tess_texCoords1[ 2 * tess_numVertexes ], surface->lightmapCoords, 8 * numVerts );
  Com_Memcpy( &tess_vertexColors[ 4 * tess_numVertexes ], surface->colors, 4 * numVerts );

  tess_numVertexes += surface->numVerts;
}

/* ---- RB_DlightFallback  0x00512490 ----  [HIGH]
 * Expands an optimized surface's shared indexes into the ordinary tess arrays
 * (rebasing them by indices[0]) so the dynamic-light passes can run. */
void __cdecl RB_DlightFallback( worldMeshSurf_t *surface )
{
  unsigned short *indexes;
  int  adjust;
  int  i;

  if ( surface->numVerts + tess_numVertexes >= SHADER_MAX_VERTEXES
    || surface->numIndexes + tess_numIndexes >= SHADER_MAX_INDEXES )
    RB_CheckOverflow( surface->numVerts, surface->numIndexes );

  if ( r_dlightQuality->integer )
  {
    Com_Memcpy( &tess_stageTangents[ 3 * tess_numVertexes ], surface->tangents, 12 * surface->numVerts );
    Com_Memcpy( &tess_stageBitangents[ 3 * tess_numVertexes ], surface->bitangents, 12 * surface->numVerts );
    Com_Memcpy( &tess_stageNormals[ 3 * tess_numVertexes ], surface->normals, 12 * surface->numVerts );
    tess_stageTangentsValid   = 1;
    tess_stageBitangentsValid = 1;
    tess_requiresVertexBasis  = 1;
  }

  Com_Memcpy( &tess_texCoords0[ 2 * tess_numVertexes ], surface->texCoords, 8 * surface->numVerts );
  Com_Memcpy( &tess_texCoords1[ 2 * tess_numVertexes ], surface->lightmapCoords, 8 * surface->numVerts );
  Com_Memcpy( &tess_vertexColors[ 4 * tess_numVertexes ], surface->colors, 4 * surface->numVerts );
  Com_Memcpy( &tess_xyz[ 3 * tess_numVertexes ], surface->positions, 12 * surface->numVerts );

  /* rebase the shared indexes onto this batch (see DoLine for the pointer) */
  indexes = tess_indexes;
  adjust  = tess_numVertexes - surface->indices[0];
  for ( i = 0; i < surface->numIndexes; i++ )
    indexes[ tess_numIndexes + i ] = (unsigned short)( surface->indices[i] + adjust );

  tess_numVertexes += surface->numVerts;
  tess_numIndexes  += surface->numIndexes;

  tess_dlightBits |= surface->dlightBits;
}


/* ---- RB_SurfaceOptimized  0x00512660 ----
 * Batches a world surface's pre-built indexes for the optimized end-surface
 * paths, and falls back to full tessellation when the surface is dlit. */
void __cdecl RB_SurfaceOptimized( worldMeshSurf_t *surface )
{
  if ( tess_renderedIndexCount + surface->numIndexes >= 0x20000 )
  {
    RB_EndSurface();
    backEnd_currentEntity = tess_entity;
    RB_BeginSurface( (void *)tess_shader, tess_vertexComponentCount );
  }

  if ( !tess_optimizedVertexEnd )
  {
    tess_optimizedFirstVertex = surface->indices[0];
    tess_optimizedVertexEnd   = surface->indices[0] + surface->numVerts;
  }
  else
  {
    if ( tess_optimizedFirstVertex > surface->indices[0] )
      tess_optimizedFirstVertex = surface->indices[0];
    if ( tess_optimizedVertexEnd < surface->indices[0] + surface->numVerts )
      tess_optimizedVertexEnd = surface->indices[0] + surface->numVerts;
  }

  /* 0x005126EF: lea ecx, 197FFB4h[count*2] -- a BYTE offset into the index buffer */
  Com_Memcpy( tess_optimizedIndexes + 2 * tess_renderedIndexCount, (unsigned char *)surface->indices, 2 * surface->numIndexes );

  tess_renderedIndexCount += surface->numIndexes;
  tess_renderedVertexCount += surface->numVerts;

  if ( backEnd_refdef_num_dlights && surface->dlightBits
    && ( !glConfig_NVVertexArrayRange || storageClass == glState_currentStorageMode ) )
    RB_DlightFallback( surface );
}

/* ---- RB_SurfaceAxis  0x00512760 ----  VERIFIED
 * Draws colored x/y/z lines from the origin for orientation debugging. */
void RB_SurfaceAxis( void )
{
  RB_BeginImmediateMode();
  GL_Bind( (GLenum *)tr_whiteImage );
  if ( *(float *)&rendererDebugState.immediateLineWidth != 3.0f )
  {
    rendererDebugState.immediateLineWidth = 3.0f;   /* 3.0f */
    qglLineWidth( 3.0f );
  }
  rendererDebugState.immediateMode = 1;                                     /* GL_LINES */
  rendererDebugState.immediateColorR = -1;
  rendererDebugState.immediateColorG = 0;
  rendererDebugState.immediateColorB = 0;
  rendererDebugState.immediateColorA = -1;
  RB_glVertex3f( 0, 0, 0 );
  RB_glVertex3f( 1098907648, 0, 0 );            /* 16.0f, 0, 0 */
  rendererDebugState.immediateColorR = 0;
  rendererDebugState.immediateColorG = -1;
  rendererDebugState.immediateColorB = 0;
  rendererDebugState.immediateColorA = -1;
  RB_glVertex3f( 0, 0, 0 );
  RB_glVertex3f( 0, 1098907648, 0 );            /* 0, 16.0f, 0 */
  rendererDebugState.immediateColorR = 0;
  rendererDebugState.immediateColorG = 0;
  rendererDebugState.immediateColorB = -1;
  rendererDebugState.immediateColorA = -1;
  RB_glVertex3f( 0, 0, 0 );
  RB_glVertex3f( 0, 0, 1098907648 );            /* 0, 0, 16.0f */
  glDrawArrays( rendererDebugState.immediateMode, 0, rendererDebugState.immediateVertexCount );
  rendererDebugState.immediateVertexCount = 0;
  rendererDebugState.immediateMode = 0;
  if ( *(float *)&rendererDebugState.immediateLineWidth != 1.0f )
  {
    rendererDebugState.immediateLineWidth = 1.0f;    /* 1.0f */
    qglLineWidth( 1.0f );
  }
  rendererDebugState.immediateModeActive = 0;
  R_FogOn();
  glVertexPointer( 3, 0x1406u, 0, tess_xyz );
}

/* ---- RB_SurfaceEntity  0x005128A0 ----  VERIFIED
 * Entities that generate a single procedural surface (RT_* selects the body). */
void RB_SurfaceEntity( void )
{
  switch ( ((rbSurfEntity_t *)backEnd_currentEntity)->reType )
  {
    case 5:
      RB_SurfaceSplash();
      break;
    case 4:
      RB_SurfaceSprite();
      break;
    case 0xC:
      RB_SurfaceOrientedQuad();
      break;
    case 6:
      RB_SurfaceBeam();
      break;
    case 7:
      RB_SurfaceRailCore();
      break;
    case 9:
      RB_SurfaceRailRings();
      break;
    case 0xA:
      RB_SurfaceLightningBolt();
      break;
    case 0xE:
      ri.Printf( 0, "FXPORT RT_ELECTRICITY TDB\n" );
      break;
    case 0xD:
      RB_SurfaceLine();
      break;
    case 0xF:
      RB_SurfaceCylinder();
      break;
    default:
      RB_SurfaceAxis();
      break;
  }
}

/* ---- RB_SurfaceBad  0x00512960 ----  VERIFIED */
void RB_SurfaceBad( void )
{
  ri.Printf( 0, "Bad surface tesselated.\n" );
}

/* ---- RB_SurfaceSkip  0x00512980 ----  VERIFIED */
void RB_SurfaceSkip( void )
{
}
