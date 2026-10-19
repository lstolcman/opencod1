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
int facing[16384];                      /* 0x011EECE0 */
int edgeDefs[524288];                   /* 0x011FECE0 */
int shadow_numEdgeDefs[8194];   /* 0x013FECE0 */


extern int GL_Bind();
extern int GL_State();
extern int RB_BeginImmediateMode();
extern int RB_glVertex3f();
extern int R_FogOn();


/* ------------------------------------------------------------------------- *
 *  Shadow-volume scratch tables (facing, edgeDefs, shadow_numEdgeDefs above).
 *
 *  The stencil-shadow builder needs, per silhouette pass, an edge table keyed
 *  by first vertex.  edgeDefs holds up to MAX_EDGE_DEFS {secondVertex,facing}
 *  pairs per vertex; numEdgeDefs (shadow_numEdgeDefs) is the parallel per-vertex
 *  count; facing[] flags whether each triangle faces the light.  edgeDefs is
 *  retail's edgeDef_t[][MAX_EDGE_DEFS] ({secondVertex, facing}, 8 bytes) held
 *  as a flat int[] in the blob (retail 0x011FECE4 is edgeDefs[0][0].facing --
 *  the +4 alias).  Indexing the int[] directly (EDGE_I2 / EDGE_FACING) lets VC7
 *  fold the table base into the displacement as retail does; a cast
 *  edgeDef_t (*)[MAX_EDGE_DEFS] pointer emits a lea instead.
 *
 *  Only the first half of tess.xyz may hold shadow-source vertices; the loop
 *  in RB_ShadowTessEnd fills the second half with the light-extruded copy, so
 *  vertex v pairs with the extruded vertex (v + tess.numVertexes).
 * ------------------------------------------------------------------------- */
#define MAX_EDGE_DEFS   32

#define EDGE_I2( v, n )       edgeDefs[2 * ( MAX_EDGE_DEFS * (v) + (n) )]
#define EDGE_FACING( v, n )   edgeDefs[2 * ( MAX_EDGE_DEFS * (v) + (n) ) + 1]

#define numEdgeDefs   shadow_numEdgeDefs   /* per-vertex edge counts (0x013FECE0) */

/* The shadow code reads e.shadowPlane (+0x18) and lightDir (+0xa4) of the
 * current entity (tr_local.h trRefEntity_t). */
typedef trRefEntity_t shadowEntity_t;
ASSERT_OFF(shadowEntity_t, e.shadowPlane, 0x18);
ASSERT_OFF(shadowEntity_t, lightDir,    0xa4);

/* File-local copies of tr_debug.c's RB_glVertex3f / RB_glVertex3fv with
 * retail's float parameters.  Retail inlines them into R_RenderShadowEdges and
 * moves each coordinate through the x87 (fld / fstp); the int bit-pattern
 * versions in tr_debug.c would use integer moves here.  The view mirrors
 * tr_debug.c's debugVertex_t with xyz typed as float. */
typedef struct {
	float xyz[3];         /* +0x00 */
	int   s;              /* +0x0c */
	int   t;              /* +0x10 */
	float lightmap[2];   /* +0x14  polyVert_t's lightmap ST (tr_surface.c); unused here */
	byte  color[4];       /* +0x1c  RGBA8 */
} shadowVertex_t;
COD1_ASSERT_SIZE(shadowVertex_t, 0x20);

static void RB_ShadowVertex3f( float x, float y, float z )
{
  shadowVertex_t *v;

  if ( rendererDebugState.immediateVertexCount < rendererDebugState.immediateVertexCapacity )
  {
    v = &((shadowVertex_t *)rendererDebugState.immediateVertices)[rendererDebugState.immediateVertexCount];
    v->color[0] = rendererDebugState.immediateColorR;
    v->color[1] = rendererDebugState.immediateColorG;
    v->color[2] = rendererDebugState.immediateColorB;
    v->color[3] = rendererDebugState.immediateColorA;
    v->s = (*(int *)&rendererDebugState.immediateTexCoordS);
    v->t = (*(int *)&rendererDebugState.immediateTexCoordT);
    v->xyz[0] = x;
    v->xyz[1] = y;
    v->xyz[2] = z;
    ++rendererDebugState.immediateVertexCount;
  }
}

static void RB_ShadowVertex3fv( const float *v )
{
  RB_ShadowVertex3f( v[0], v[1], v[2] );
}

/* ---- R_AddEdgeDef  0x00512990 ----  [HIGH]
 * Append one directed edge (i1 -> i2, carrying the source triangle's facing
 * flag) to i1's edge list, dropping it on overflow. */
void __cdecl R_AddEdgeDef( int i1, int i2, int facingFlag )
{
  int c;

  c = numEdgeDefs[i1];
  if ( c == MAX_EDGE_DEFS )
    return;   /* overflow */
  EDGE_I2( i1, c )     = i2;
  EDGE_FACING( i1, c ) = facingFlag;

  numEdgeDefs[i1] = c + 1;
}

/* ---- R_RenderShadowEdges  0x005129D0 ----  VERIFIED
 * Draw the silhouette edges of the shadow volume as light-extruded quads.  An
 * edge is a silhouette edge unless it has a reverse-paired edge that also
 * faces the light (a well-formed closed hull has exactly two faces per edge,
 * but many models have dangling / overfanned edges).  Each surviving edge is
 * emitted as a GL_TRIANGLE_STRIP of {front[i], back[i], front[i2], back[i2]}
 * through the immediate-mode vertex buffer (RB_glBegin / RB_glVertex3fv /
 * RB_glEnd inlined; each vertex address is formed just before its call). */
void R_RenderShadowEdges()
{
  int i;
  int c;
  int j;
  int i2;
  int c2;
  int k;
  int hit[2];

  RB_BeginImmediateMode();

  for ( i = 0; i < tess_numVertexes; i++ )
  {
    c = numEdgeDefs[i];
    for ( j = 0; j < c; j++ )
    {
      if ( !EDGE_FACING( i, j ) )
        continue;

      hit[0] = 0;
      hit[1] = 0;

      /* count reverse edges (i2 -> i) by their facing */
      i2 = EDGE_I2( i, j );
      c2 = numEdgeDefs[i2];
      for ( k = 0; k < c2; k++ )
      {
        if ( EDGE_I2( i2, k ) == i )
          hit[EDGE_FACING( i2, k )]++;
      }

      /* not a silhouette edge if a reverse front-facing edge shares it */
      if ( hit[1] == 0 )
      {
        rendererDebugState.immediateMode = 5;                                   /* GL_TRIANGLE_STRIP */
        RB_ShadowVertex3fv( &tess_xyz[i * tess_vertexComponentCount] );
        RB_ShadowVertex3fv( &tess_xyz[(i + tess_numVertexes) * tess_vertexComponentCount] );
        RB_ShadowVertex3fv( &tess_xyz[i2 * tess_vertexComponentCount] );
        RB_ShadowVertex3fv( &tess_xyz[(i2 + tess_numVertexes) * tess_vertexComponentCount] );
        glDrawArrays( rendererDebugState.immediateMode, 0, rendererDebugState.immediateVertexCount );
        rendererDebugState.immediateVertexCount = 0;
        rendererDebugState.immediateMode = 0;
      }
    }
  }

  /* RB_EndImmediateMode inlined: leave immediate mode and restore fog / xyz */
  rendererDebugState.immediateModeActive = 0;
  R_FogOn();
  glVertexPointer( 3, 0x1406u, 0, tess_xyz );        /* GL_FLOAT */
}

/* ---- RB_ShadowTessEnd  0x00512D60 ----  [HIGH]
 * Build and render one entity's stencil shadow volume from the current tess.
 * Extrudes every vertex away from the light, tags each triangle's facing,
 * builds the edge table, then renders the silhouette twice (increment on the
 * back-facing pass, decrement on the front-facing pass) into the stencil.
 * CrossProduct is com_math.c's (/Op); retail inlines it, which is what stores
 * d1 / d2 / normal / d to their slots.  The two cull blocks are open-coded
 * (calling GL_Cull inlines with a different branch layout). */
void RB_ShadowTessEnd()
{
  vec3_t lightDir;
  float *front;
  float *back;
  int    numTris;
  int    i;

  /* need room for the extruded copy (second half of tess.xyz) and a stencil */
  if ( tess_numVertexes >= 4096 || glConfig_stencilBits < 4 )
    return;

  lightDir[0] = ((shadowEntity_t *)backEnd_currentEntity)->lightDir[0];
  lightDir[1] = ((shadowEntity_t *)backEnd_currentEntity)->lightDir[1];
  lightDir[2] = ((shadowEntity_t *)backEnd_currentEntity)->lightDir[2];

  /* project each vertex 512 units away from the light into the second half */
  front = tess_xyz;
  back  = &tess_xyz[tess_numVertexes * tess_vertexComponentCount];
  for ( i = 0; i < tess_numVertexes; front += tess_vertexComponentCount, back += tess_vertexComponentCount, ++i )
  {
    back[0] = front[0] - lightDir[0] * 512.0f;
    back[1] = front[1] - lightDir[1] * 512.0f;
    back[2] = front[2] - lightDir[2] * 512.0f;
  }

  /* decide which triangles face the light and build the edge table */
  memset( numEdgeDefs, 0, 4 * tess_numVertexes );

  numTris = tess_numIndexes / 3;
  for ( i = 0; i < numTris; ++i )
  {
    int    i1, i2, i3;
    vec3_t d1, d2, normal;
    float *v1, *v2, *v3;
    float  d;

    i1 = tess_indexes[i * 3 + 0];
    i2 = tess_indexes[i * 3 + 1];
    i3 = tess_indexes[i * 3 + 2];

    v1 = &tess_xyz[i1 * tess_vertexComponentCount];
    v2 = &tess_xyz[i2 * tess_vertexComponentCount];
    v3 = &tess_xyz[i3 * tess_vertexComponentCount];

    d1[0] = v2[0] - v1[0];
    d1[1] = v2[1] - v1[1];
    d1[2] = v2[2] - v1[2];
    d2[0] = v3[0] - v1[0];
    d2[1] = v3[1] - v1[1];
    d2[2] = v3[2] - v1[2];

    CrossProduct( d1, d2, normal );

    d = normal[2] * lightDir[2] + normal[1] * lightDir[1] + normal[0] * lightDir[0];
    if ( d > 0.0f )
      facing[i] = 1;
    else
      facing[i] = 0;

    /* create the edges */
    R_AddEdgeDef( i1, i2, facing[i] );
    R_AddEdgeDef( i2, i3, facing[i] );
    R_AddEdgeDef( i3, i1, facing[i] );
  }

  /* draw the silhouette edges into the stencil buffer */
  GL_Bind( (GLenum *)tr_whiteImage );
  GL_State( 18 );                                   /* 0x12  GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO */
  qglColor3f( 0.2f, 0.2f, 0.2f );
  qglColorMask( 0, 0, 0, 0 );                        /* don't touch the color buffer */
  glEnable( 0xB90u );                                /* GL_STENCIL_TEST */
  glStencilFunc( 0x207u, 1, 0xFFu );                 /* GL_ALWAYS, ref 1, mask 255 */

  /* increment the stencil on back faces (mirrors reverse the culling) */
  if ( glState_faceCulling != 1 )
  {
    if ( glState_faceCulling == 2 )
      glEnable( 0xB44u );                            /* GL_CULL_FACE */
    if ( backEnd_viewParms_isMirror )
      qglCullFace( 0x404u );                         /* GL_FRONT */
    else
      qglCullFace( 0x405u );                         /* GL_BACK */
    glState_faceCulling = 1;
  }
  glStencilOp( 0x1E00u, 0x1E00u, 0x1E02u );          /* GL_KEEP, GL_KEEP, GL_INCR */
  R_RenderShadowEdges();

  /* decrement the stencil on front faces */
  if ( glState_faceCulling != 0 )
  {
    if ( glState_faceCulling == 2 )
      glEnable( 0xB44u );                            /* GL_CULL_FACE */
    if ( backEnd_viewParms_isMirror )
      qglCullFace( 0x405u );                         /* GL_BACK */
    else
      qglCullFace( 0x404u );                         /* GL_FRONT */
    glState_faceCulling = 0;
  }
  glStencilOp( 0x1E00u, 0x1E00u, 0x1E03u );          /* GL_KEEP, GL_KEEP, GL_DECR */
  R_RenderShadowEdges();

  qglColorMask( 1, 1, 1, 1 );                         /* re-enable the color buffer */
}

/* ---- RB_ShadowFinish  0x00513130 ----  VERIFIED
 * Darken everything inside a shadow volume once, after all volumes have been
 * stencilled, by drawing a fullscreen grey quad wherever the stencil is set.
 * Delayed to here so overlapping body-part shadows don't double-darken. */
void RB_ShadowFinish()
{
  if ( cg_shadows->integer != 2 || glConfig_stencilBits < 4 )
    return;

  glEnable( 0xB90u );                                /* GL_STENCIL_TEST */
  glStencilFunc( 0x205u, 0, 0xFFu );                 /* GL_NOTEQUAL, ref 0, mask 255 */
  glDisable( 0x3000u );                              /* GL_CLIP_PLANE0 */

  /* GL_Cull(CT_TWO_SIDED) inlined: disable face culling */
  if ( glState_faceCulling != 2 )
  {
    glDisable( 0xB44u );                             /* GL_CULL_FACE */
    glState_faceCulling = 2;
  }

  GL_Bind( (GLenum *)tr_whiteImage );
  qglLoadIdentity();

  RB_BeginImmediateMode();
  rendererDebugState.immediateColorR = -103;                    /* 0x99 -> ~0.6 grey */
  rendererDebugState.immediateColorG = -103;
  rendererDebugState.immediateColorB = -103;
  rendererDebugState.immediateColorA = -1;                      /* 0xFF */
  GL_State( 275 );                                   /* 0x113  GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO */

  /* fullscreen quad in identity space; args are float bit patterns */
  rendererDebugState.immediateMode = 7;                                          /* GL_QUADS */
  RB_glVertex3f( -1027080192,  1120403456, -1054867456 );  /* (-100,  100, -10) */
  RB_glVertex3f(  1120403456,  1120403456, -1054867456 );  /* ( 100,  100, -10) */
  RB_glVertex3f(  1120403456, -1027080192, -1054867456 );  /* ( 100, -100, -10) */
  RB_glVertex3f( -1027080192, -1027080192, -1054867456 );  /* (-100, -100, -10) */
  glDrawArrays( rendererDebugState.immediateMode, 0, rendererDebugState.immediateVertexCount );
  rendererDebugState.immediateVertexCount = 0;
  rendererDebugState.immediateMode = 0;

  glDisable( 0xB90u );                               /* GL_STENCIL_TEST */

  /* RB_EndImmediateMode inlined: leave immediate mode and restore fog / xyz */
  rendererDebugState.immediateModeActive = 0;
  R_FogOn();
  glVertexPointer( 3, 0x1406u, 0, tess_xyz );        /* GL_FLOAT */
}

/* ---- RB_ProjectionShadowDeform  0x00513290 ----  VERIFIED
 * Flatten the current tess onto the entity's ground plane, projecting each
 * vertex along the (clamped) light direction.  The light direction is clamped
 * so the projection never grows too long or flips behind the plane.  The int
 * return is dead EAX (retail returns tess.numVertexes). */
int RB_ProjectionShadowDeform()
{
  vec3_t  ground;
  vec3_t  lightDir;
  vec3_t  light;
  float   groundDist;
  float   d;
  float   extra;
  float   height;
  float  *xyz;
  int     i;

  xyz = tess_xyz;

  ground[0]  = backEnd_or.axis[0][2];
  ground[1]  = backEnd_or.axis[1][2];
  ground[2]  = backEnd_or.axis[2][2];
  groundDist = backEnd_or.origin[2] - ((shadowEntity_t *)backEnd_currentEntity)->e.shadowPlane;

  lightDir[0] = ((shadowEntity_t *)backEnd_currentEntity)->lightDir[0];
  lightDir[1] = ((shadowEntity_t *)backEnd_currentEntity)->lightDir[1];
  lightDir[2] = ((shadowEntity_t *)backEnd_currentEntity)->lightDir[2];

  d = lightDir[2] * ground[2] + lightDir[1] * ground[1] + lightDir[0] * ground[0];
  /* don't let the shadow get too long or go negative */
  if ( d < 0.5f )
  {
    extra = 0.5f - d;
    lightDir[0] += ground[0] * extra;
    lightDir[1] += ground[1] * extra;
    lightDir[2] += ground[2] * extra;
    d = lightDir[2] * ground[2] + lightDir[1] * ground[1] + lightDir[0] * ground[0];
  }

  d = 1.0f / d;
  light[0] = d * lightDir[0];
  light[1] = d * lightDir[1];
  light[2] = d * lightDir[2];

  for ( i = 0; i < tess_numVertexes; ++i, xyz += tess_vertexComponentCount )
  {
    height = ground[2] * xyz[2] + ground[1] * xyz[1] + ground[0] * xyz[0] + groundDist;
    xyz[0] -= light[0] * height;
    xyz[1] -= light[1] * height;
    xyz[2] -= light[2] * height;
  }

  return i;
}
