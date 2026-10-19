/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_dpvs.c -- CoD's DPVS (deferred PVS) world visibility.  Unlike RTCW's
 * cluster PVS + R_RecursiveWorldNode, CoD1 walks a cell/portal graph: the
 * camera cell floods outward through portals (R_RecursivePortalWalk), each
 * visited cell installs its silhouette occluders (R_AddCellOccluders), submits
 * its owned world geometry (R_AddCellSurfaces / R_AddCellCullGroups) and links
 * the per-view brush/xmodel entities into the cells they touch
 * (R_FilterModelsIntoCells).  Boxes are rejected against the view frustum, the
 * near/far clip planes and the active occluders by the inlined box-vs-plane
 * tests factored below as R_CullBoxDPVS / R_CullBoxDPVSStrict.
 *
 * The DPVS working state (view origin, near/far/frustum planes, occluder and
 * active-plane banks, per-view counters) lives in standalone globals reached by
 * name; only the pointer-passed world data (cells, portals, occluders, the
 * per-cell AABB tree, cull groups, entity/model links, coronas, surfaces) is
 * given C types here.  Retyping a pointer never moves storage, so every field
 * offset is pinned with an assert.
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_dpvsplane.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
/* retail's one DPVS state block, 0x011CC100..0x011CC22B (layout in tr_dpvsplane.h).
 * Being one address-taken object keeps its stores ordered against loads through
 * pointers as in retail (R_AddSkySurfacesDPVS, R_AddCellSurfaces). */
dpvsGlob_t dpvsGlob;   /* 0x011CC100 */

#define dpvs_cullDist                     dpvsGlob.cullDist
#define dpvs_cellEntityLinks              dpvsGlob.cellEntityLinks
#define dpvs_cellEntityLinkCount          dpvsGlob.cellEntityLinkCount
#define dpvs_transformMatrixInverse       dpvsGlob.transformMatrixInverse
#define dpvs_transformMatrixTransposed    dpvsGlob.transformMatrixTransposed[0]
#define dpvs_transformMatrixTransposed01  dpvsGlob.transformMatrixTransposed[1]
#define dpvs_transformMatrixTransposed02  dpvsGlob.transformMatrixTransposed[2]
#define dpvs_transformMatrixTransposed03  dpvsGlob.transformMatrixTransposed[3]
#define dpvs_transformMatrixTransposed10  dpvsGlob.transformMatrixTransposed[4]
#define dpvs_transformMatrixTransposed11  dpvsGlob.transformMatrixTransposed[5]
#define dpvs_transformMatrixTransposed12  dpvsGlob.transformMatrixTransposed[6]
#define dpvs_transformMatrixTransposed13  dpvsGlob.transformMatrixTransposed[7]
#define dpvs_transformMatrixTransposed20  dpvsGlob.transformMatrixTransposed[8]
#define dpvs_transformMatrixTransposed21  dpvsGlob.transformMatrixTransposed[9]
#define dpvs_transformMatrixTransposed22  dpvsGlob.transformMatrixTransposed[10]
#define dpvs_transformMatrixTransposed23  dpvsGlob.transformMatrixTransposed[11]
#define dpvs_transformMatrixTransposed30  dpvsGlob.transformMatrixTransposed[12]
#define dpvs_transformMatrixTransposed31  dpvsGlob.transformMatrixTransposed[13]
#define dpvs_transformMatrixTransposed32  dpvsGlob.transformMatrixTransposed[14]
#define dpvs_transformMatrixTransposed33  dpvsGlob.transformMatrixTransposed[15]





void AddPointToBounds( const float *point, float *mins, float *maxs );
void ClearBounds( float *mins, float *maxs );
extern int BoxOnPlaneSide();
extern int ExpandBounds();
extern int RE_AddRefEntityToScene();
extern int R_AddDebugBox();
extern int R_AddDebugLine();
extern int R_AddDebugPolygon();
extern int R_AddDrawSurf();
void VectorNormalizeFast( float *v );
extern float *MatrixInverse44(const float in[16], float out[16]);


/* ------------------------------------------------------------------------- *
 *  Constants.
 * ------------------------------------------------------------------------- */
#define TR_FRUSTUM_PLANES            4       /* view-frustum plane count */
#define DPVS_PLANE_BIAS              0.001f  /* R_SetPlaneSidesDPVS distance bias */

/* corner-select byte offsets baked into dpvsPlane_t::sideOffsets: mins.xyz sit
 * at 0/4/8 of the [mins|maxs] bounds pair, maxs.xyz at 12/16/20. */
#define DPVS_OFF_MINS_X   0
#define DPVS_OFF_MINS_Y   4
#define DPVS_OFF_MINS_Z   8
#define DPVS_OFF_MAXS_X   12
#define DPVS_OFF_MAXS_Y   16
#define DPVS_OFF_MAXS_Z   20

#define DPVS_MAX_OCCLUDERS           1024    /* dpvs_occluders[] capacity */
#define DPVS_MAX_OCCLUDER_PLANES     6144    /* dpvs_activePlanes[] capacity */
#define DPVS_MAX_CELL_ENTITY_LINKS   4096    /* dpvs_cellEntityLinks[] capacity */

/* refEntity_t::reType values distinguished by the DPVS entity filter (CoD:
 * brush model = 0, xmodel = 1). */
#define DPVS_RT_BRUSH_MODEL          0
#define DPVS_RT_XMODEL               1
#define DPVS_RT_SPRITE               4       /* built for each corona */

#define DPVS_ENTITYNUM_WORLD         1022
#define DPVS_ENTITY_SORT_SHIFT       8       /* shiftedEntityNumber = num << 8 */

/* r_showCull* modes */
#define DPVS_SHOWCULL_BOUNDS         1
#define DPVS_SHOWCULL_CELL_LINKS     2


/* ------------------------------------------------------------------------- *
 *  DPVS plane (from tr_dpvsplane.h) -- normal + distance + 3 corner-select
 *  byte offsets (+ an unused cameraSide byte).  Pinned here since the header
 *  carries no asserts.
 * ------------------------------------------------------------------------- */
COD1_ASSERT_SIZE(dpvsPlane_t, 0x14);
ASSERT_OFF(dpvsPlane_t, distance, 0x0c);
ASSERT_OFF(dpvsPlane_t, sideOffsets, 0x10);
ASSERT_OFF(dpvsPlane_t, cameraSide, 0x13);


/* ------------------------------------------------------------------------- *
 *  File-local world structures reached through the pointer-passed cell graph.
 *  msurface_t / srfLit_t / cplane_t / mnode_t / trRefEntity_t come from
 *  tr_local.h; the types below are this unit's view of the cell / portal /
 *  occluder / AABB-tree / cull-group / link records those cells own.
 * ------------------------------------------------------------------------- */

/* dpvsCullGroup_t (0x24) -- authored bounds over a run of world surfaces */
typedef struct {
	vec3_t      mins;             /* +0x00 */
	vec3_t      maxs;             /* +0x0c */
	msurface_t *surfaces;         /* +0x18  stride 0x0c */
	int         surfaceCount;     /* +0x1c */
	int         viewCount;        /* +0x20  submitted-this-view stamp */
} dpvsCullGroup_t;
COD1_ASSERT_SIZE(dpvsCullGroup_t, 0x24);
ASSERT_OFF(dpvsCullGroup_t, surfaces, 0x18);
ASSERT_OFF(dpvsCullGroup_t, surfaceCount, 0x1c);
ASSERT_OFF(dpvsCullGroup_t, viewCount, 0x20);

/* dpvsAabbTree_t (0x28) -- per-cell surface bounds tree.  Internal records own
 * a contiguous child array; leaf records own 0x0c-byte world-surface entries. */
typedef struct dpvsAabbTree_s dpvsAabbTree_t;
struct dpvsAabbTree_s {
	vec3_t          mins;         /* +0x00 */
	vec3_t          maxs;         /* +0x0c */
	msurface_t     *surfaces;     /* +0x18  stride 0x0c */
	int             surfaceCount; /* +0x1c */
	dpvsAabbTree_t *children;     /* +0x20  stride 0x28 */
	int             childCount;   /* +0x24 */
};
COD1_ASSERT_SIZE(dpvsAabbTree_t, 0x28);
ASSERT_OFF(dpvsAabbTree_t, surfaces, 0x18);
ASSERT_OFF(dpvsAabbTree_t, surfaceCount, 0x1c);
ASSERT_OFF(dpvsAabbTree_t, children, 0x20);
ASSERT_OFF(dpvsAabbTree_t, childCount, 0x24);

/* dpvsPortal_t (0x24) -- one outgoing portal: its plane, target cell and the
 * winding vertices flooded through it. */
typedef struct dpvsCell_s dpvsCell_t;
typedef struct {
	dpvsPlane_t  plane;           /* +0x00  normal/dist + side offsets */
	dpvsCell_t  *cell;            /* +0x14  target cell */
	vec3_t      *vertices;        /* +0x18 */
	int          vertexCount;     /* +0x1c */
	int          recursionActive; /* +0x20  cycle guard while walking */
} dpvsPortal_t;
COD1_ASSERT_SIZE(dpvsPortal_t, 0x24);
ASSERT_OFF(dpvsPortal_t, cell, 0x14);
ASSERT_OFF(dpvsPortal_t, vertices, 0x18);
ASSERT_OFF(dpvsPortal_t, vertexCount, 0x1c);
ASSERT_OFF(dpvsPortal_t, recursionActive, 0x20);

/* dpvsOccluderEdge_t (0x10) -- runtime occluder edge: its two bounding planes
 * and two endpoints, used to build silhouette planes. */
typedef struct {
	dpvsPlane_t *planes[2];       /* +0x00 */
	vec3_t      *vertices[2];     /* +0x08 */
} dpvsOccluderEdge_t;
COD1_ASSERT_SIZE(dpvsOccluderEdge_t, 0x10);
ASSERT_OFF(dpvsOccluderEdge_t, vertices, 0x08);

/* dpvsOccluder_t (0x24) -- runtime occluder brush.  The first six fields own
 * the authored plane/edge/vertex spans; DPVS owns the cull limit and the
 * per-walk active-plane range. */
typedef struct {
	dpvsPlane_t        *planes;          /* +0x00 */
	int                 planeCount;      /* +0x04 */
	int                 edgeCount;       /* +0x08 */
	dpvsOccluderEdge_t *edges;           /* +0x0c */
	int                 vertexCount;     /* +0x10 */
	vec3_t             *vertices;        /* +0x14 */
	int                 cullPlaneLimit;  /* +0x18  INT_MAX, lowered to first culling plane */
	int                 activePlaneCount;/* +0x1c */
	dpvsPlane_t        *activePlanes;    /* +0x20  into dpvs_activePlanes bank */
} dpvsOccluder_t;
COD1_ASSERT_SIZE(dpvsOccluder_t, 0x24);
ASSERT_OFF(dpvsOccluder_t, edges, 0x0c);
ASSERT_OFF(dpvsOccluder_t, vertices, 0x14);
ASSERT_OFF(dpvsOccluder_t, cullPlaneLimit, 0x18);
ASSERT_OFF(dpvsOccluder_t, activePlaneCount, 0x1c);
ASSERT_OFF(dpvsOccluder_t, activePlanes, 0x20);

/* dpvsCellModelLink_t (0x08) -- per-cell membership link for a static model */
typedef struct dpvsCellModelLink_s dpvsCellModelLink_t;
struct dpvsCellModelLink_s {
	void                *model;   /* +0x00  dpvsStaticModel_t */
	dpvsCellModelLink_t *next;    /* +0x04 */
};
COD1_ASSERT_SIZE(dpvsCellModelLink_t, 0x08);

/* dpvsCellEntityLink_t (0x20) -- transient per-view brush/xmodel entity link */
typedef struct dpvsCellEntityLink_s dpvsCellEntityLink_t;
struct dpvsCellEntityLink_s {
	trRefEntity_t        *entity; /* +0x00 */
	vec3_t                mins;   /* +0x04 */
	vec3_t                maxs;   /* +0x10 */
	dpvsCellEntityLink_t *next;   /* +0x1c */
};
COD1_ASSERT_SIZE(dpvsCellEntityLink_t, 0x20);
ASSERT_OFF(dpvsCellEntityLink_t, mins, 0x04);
ASSERT_OFF(dpvsCellEntityLink_t, maxs, 0x10);
ASSERT_OFF(dpvsCellEntityLink_t, next, 0x1c);

/* dpvsCell_t (0x40) -- one world cell (addressed by mnode_t::cellIndex) */
struct dpvsCell_s {
	vec3_t                mins;           /* +0x00 */
	vec3_t                maxs;           /* +0x0c */
	dpvsAabbTree_t       *aabbTree;       /* +0x18 */
	dpvsPortal_t         *portals;        /* +0x1c  stride 0x24 */
	int                   portalCount;    /* +0x20 */
	dpvsCullGroup_t     **cullGroups;     /* +0x24 */
	int                   cullGroupCount; /* +0x28 */
	dpvsOccluder_t      **occluders;      /* +0x2c */
	int                   occluderCount;  /* +0x30 */
	int                   markViewCount;  /* +0x34  (unused here) */
	dpvsCellModelLink_t  *modelLinks;     /* +0x38 */
	dpvsCellEntityLink_t *entityLinks;    /* +0x3c */
};
COD1_ASSERT_SIZE(dpvsCell_t, 0x40);
ASSERT_OFF(dpvsCell_t, aabbTree, 0x18);
ASSERT_OFF(dpvsCell_t, portals, 0x1c);
ASSERT_OFF(dpvsCell_t, portalCount, 0x20);
ASSERT_OFF(dpvsCell_t, cullGroups, 0x24);
ASSERT_OFF(dpvsCell_t, cullGroupCount, 0x28);
ASSERT_OFF(dpvsCell_t, occluders, 0x2c);
ASSERT_OFF(dpvsCell_t, occluderCount, 0x30);
ASSERT_OFF(dpvsCell_t, modelLinks, 0x38);
ASSERT_OFF(dpvsCell_t, entityLinks, 0x3c);

/* dpvsStaticModel_t -- a loaded static-model instance linked into cells.  Its
 * leading refEntity_t (0x9c) is submitted directly to the scene; the trailing
 * bounds + view stamp are read by the cull path.  Only the touched offsets are
 * named (the full record is larger and not needed here). */
typedef struct {
	refEntity_t e;                /* +0x000  submitted to RE_AddRefEntityToScene */
	vec3_t      mins;             /* +0x09c */
	vec3_t      maxs;             /* +0x0a8 */
	int         viewCount;        /* +0x0b4  submitted-this-view stamp */
} dpvsStaticModel_t;
ASSERT_OFF(dpvsStaticModel_t, mins, 0x9c);
ASSERT_OFF(dpvsStaticModel_t, maxs, 0xa8);
ASSERT_OFF(dpvsStaticModel_t, viewCount, 0xb4);

/* dpvsCorona_t (0x20) -- map-authored corona/flare (tr.world->coronas) */
typedef struct {
	shader_t *shader;             /* +0x00 */
	vec3_t    origin;             /* +0x04 */
	float     scale;              /* +0x10  sprite radius */
	float     zCutoff;            /* +0x14  reject below this view-dir z */
	float     zFadeOut;           /* +0x18  alpha fade start */
	byte      color[4];           /* +0x1c  RGBA */
} dpvsCorona_t;
COD1_ASSERT_SIZE(dpvsCorona_t, 0x20);
ASSERT_OFF(dpvsCorona_t, scale, 0x10);
ASSERT_OFF(dpvsCorona_t, zCutoff, 0x14);
ASSERT_OFF(dpvsCorona_t, zFadeOut, 0x18);
ASSERT_OFF(dpvsCorona_t, color, 0x1c);


/* tr.world blobs reached through the tr_world pointer (as in tr_world.c) */
#define DPVS_WORLD_NODES           (*(mnode_t          **)(tr_world + 152))
#define DPVS_WORLD_SKYSURF_COUNT   (*(int               *)(tr_world + 164))
#define DPVS_WORLD_SKYSURFS        (*(msurface_t      ***)(tr_world + 168))
#define DPVS_WORLD_CORONAS         (*(dpvsCorona_t     **)(tr_world + 192))
#define DPVS_WORLD_CORONA_COUNT    (*(int               *)(tr_world + 196))
#define DPVS_WORLD_CELL_COUNT      (*(int               *)(tr_world + 288))
#define DPVS_WORLD_CELLS           (*(dpvsCell_t       **)(tr_world + 292))
#define DPVS_WORLD_CULLGROUP_COUNT (*(int               *)(tr_world + 324))
#define DPVS_WORLD_CULLGROUPS      (*(dpvsCullGroup_t  **)(tr_world + 328))

/* mnode_t sentinels used to walk the BSP (contents at +0x00, cellIndex at +0x08) */
#define DPVS_NODE_INTERNAL   (-2)   /* cellIndex value for a decision node */
#define DPVS_NODE_NO_CELL    (-1)   /* leaf/node with no cell */

/* the DPVS view-frustum plane bank (retail dpvs_frustumPlanes, 4 x dpvsPlane_t) */
#define dpvs_frustumPlanesTyped   ((dpvsPlane_t *)(void *)dpvs_frustumPlanes)


/* ------------------------------------------------------------------------- *
 *  Box-vs-plane visibility tests.
 *
 *  The retail compiler inlines these into every DPVS box test (surfaces, AABB
 *  tree nodes, cull groups, static models, coronas, filtered entities); they
 *  are factored here so each caller reads clearly.  VC7 inlines the two cull
 *  tests only when forced (retail also kept out-of-line copies of them at
 *  0x4E1360 / 0x4E14C0, mirrored in tr_debug.c).  A plane's precomputed
 *  sideOffsets[] pick, per component, whether the box's mins or maxs value is
 *  the corner farthest along +normal.  All three tests thread the shared
 *  dpvs_cullPlaneLimit / per-occluder cullPlaneLimit state exactly as the
 *  inlined originals do.
 *
 *  x87 term order matters: retail sums the caller-supplied frustum/portal
 *  planes as (x+y)+z (e.g. 0x4E30C0, 0x4E3240, 0x4E3390) but the near, far,
 *  sky-far and occluder planes as (z+y)+x (e.g. 0x4E30F7, 0x4E3199), and it
 *  compares the unrounded x87 sum straight against the float distance (no
 *  float temporary), so each order gets its own helper below.
 * ------------------------------------------------------------------------- */

/* the box corner farthest along +normal is wholly behind the plane
 * (z,y,x order: near/far/occluder planes) */
static int R_BoxBehindPlane( const vec3_t *bounds, const dpvsPlane_t *plane )
{
	const char *b = (const char *)bounds;
	/* the corner loads as locals (like R_BoxInFrontOfPlane): this load-then-multiply
	   order is what the inlined copies in R_CullModels / R_AddCullGroupDPVS use */
	float z = *(const float *)( b + plane->sideOffsets[2] );
	float y = *(const float *)( b + plane->sideOffsets[1] );
	float x = *(const float *)( b + plane->sideOffsets[0] );
	return z * plane->normal[2] + y * plane->normal[1] + x * plane->normal[0] < plane->distance;
}

/* the box corner nearest along +normal is wholly in front of the plane
 * (z,y,x order: occluder planes).  The opposite corner is retail's
 * b+1Ch/14h/0Ch minus the side offset. */
static int R_BoxInFrontOfPlane( const vec3_t *bounds, const dpvsPlane_t *plane )
{
 const char *b = (const char *)bounds;
 float z = *(const float *)(b - plane->sideOffsets[2] + DPVS_OFF_MAXS_Z + DPVS_OFF_MINS_Z);
 float y = *(const float *)(b - plane->sideOffsets[1] + DPVS_OFF_MAXS_Y + DPVS_OFF_MINS_Y);
 float x = *(const float *)(b - plane->sideOffsets[0] + DPVS_OFF_MAXS_X);
 return z * plane->normal[2] + y * plane->normal[1] + x * plane->normal[0] > plane->distance;
}

/* R_BoxBehindPlane in x,y,z order: the caller-supplied planes[] loops */
static int R_BoxBehindPlaneXYZ( const vec3_t *bounds, const dpvsPlane_t *plane )
{
	const char *b = (const char *)bounds;
	return *(const float *)( b + plane->sideOffsets[0] ) * plane->normal[0]
	     + *(const float *)( b + plane->sideOffsets[1] ) * plane->normal[1]
	     + *(const float *)( b + plane->sideOffsets[2] ) * plane->normal[2] < plane->distance;
}

/* R_BoxInFrontOfPlane in x,y,z order: the caller-supplied planes[] loops.
 * The opposite corner is retail's b+0Ch/14h/1Ch minus the side offset. */
static int R_BoxInFrontOfPlaneXYZ( const vec3_t *bounds, const dpvsPlane_t *plane )
{
	const char *b = (const char *)bounds;
	float x = *(const float *)( b + DPVS_OFF_MAXS_X - plane->sideOffsets[0] );
	float y = *(const float *)( b + DPVS_OFF_MAXS_Y + DPVS_OFF_MINS_Y - plane->sideOffsets[1] );
	float z = *(const float *)( b + DPVS_OFF_MAXS_Z + DPVS_OFF_MINS_Z - plane->sideOffsets[2] );
	return x * plane->normal[0] + y * plane->normal[1] + z * plane->normal[2] > plane->distance;
}

/* cull a box behind any frustum plane (only while planeIndex is still fresh),
 * the near/far plane, or wholly inside an active occluder. */
static __forceinline int R_CullBoxDPVS( const vec3_t *bounds, const dpvsPlane_t *planes, int planeCount, int planeIndex )
{
	int i;
	int j;

	if ( planeIndex < dpvs_cullPlaneLimit )
	{
		dpvs_cullPlaneLimit = 0x7FFFFFFF;
		for ( i = 0; i < planeCount; ++i )
		{
			if ( R_BoxBehindPlaneXYZ( bounds, &planes[i] ) )
				return 1;
		}
	}

	if ( R_BoxBehindPlane( bounds, (const dpvsPlane_t *)dpvs_activeNearPlane ) )
		return 1;
	if ( dpvs_activeFarPlane && R_BoxBehindPlane( bounds, (const dpvsPlane_t *)dpvs_activeFarPlane ) )
		return 1;

	/* retail re-reads dpvs_occluders[i] after each store and tests the plane
	 * count at the top of the inner loop (behind every plane -> occluded) */
	for ( i = 0; i < dpvs_occluderCount; ++i )
	{
		if ( planeIndex >= ((dpvsOccluder_t **)dpvs_occluders)[i]->cullPlaneLimit )
			continue;
		((dpvsOccluder_t **)dpvs_occluders)[i]->cullPlaneLimit = 0x7FFFFFFF;
		for ( j = 0; ; ++j )
		{
			if ( j >= ((dpvsOccluder_t **)dpvs_occluders)[i]->activePlaneCount )
				return 1;
			if ( !R_BoxBehindPlane( bounds, &((dpvsOccluder_t **)dpvs_occluders)[i]->activePlanes[j] ) )
				break;
		}
	}
	return 0;
}

/* stricter classifier: reports a box that is NOT wholly in front of every
 * frustum plane, or that is wholly behind every active plane of some occluder,
 * recording the earliest reusable plane position.  A false result means the box
 * is fully in front and unoccluded, so a whole subtree may be accepted. */
static __forceinline int R_CullBoxDPVSStrict( const vec3_t *bounds, const dpvsPlane_t *planes, int planeCount, int planeIndex )
{
	int i;
	int j;

	if ( planeIndex < dpvs_cullPlaneLimit )
	{
		for ( i = 0; i < planeCount; ++i )
		{
			if ( !R_BoxInFrontOfPlaneXYZ( bounds, &planes[i] ) )
				return 1;
		}
		dpvs_cullPlaneLimit = planeIndex + 1;
	}

	for ( i = 0; i < dpvs_occluderCount; ++i )
	{
		if ( planeIndex >= ((dpvsOccluder_t **)dpvs_occluders)[i]->cullPlaneLimit )
			continue;
		for ( j = 0; ; ++j )
		{
			if ( j >= ((dpvsOccluder_t **)dpvs_occluders)[i]->activePlaneCount )
				return 1;
			if ( R_BoxInFrontOfPlane( bounds, &((dpvsOccluder_t **)dpvs_occluders)[i]->activePlanes[j] ) )
				break;
		}
		((dpvsOccluder_t **)dpvs_occluders)[i]->cullPlaneLimit = planeIndex + 1;
	}
	return 0;
}


/* ---- R_SetPlaneSidesDPVS  0x004E1620 ----  VERIFIED
 * Bias a DPVS plane back by 0.001 and precompute the byte offset that selects
 * the box corner farthest along each normal component.  The corner test uses
 * the sign of the raw normal bits (an int compare of the float word). */
int __cdecl R_SetPlaneSidesDPVS(int result)
{
  dpvsPlane_t *plane = (dpvsPlane_t *)result;

  plane->distance -= DPVS_PLANE_BIAS;
  plane->sideOffsets[0] = *(int *)&plane->normal[0] > 0 ? DPVS_OFF_MAXS_X : DPVS_OFF_MINS_X;
  plane->sideOffsets[1] = *(int *)&plane->normal[1] > 0 ? DPVS_OFF_MAXS_Y : DPVS_OFF_MINS_Y;
  plane->sideOffsets[2] = *(int *)&plane->normal[2] > 0 ? DPVS_OFF_MAXS_Z : DPVS_OFF_MINS_Z;
  return result;
}

/* ---- RE_GetFarPlaneDist  0x004E1670 ----  VERIFIED
 * An explicit r_zfar wins; otherwise the current linear world fog supplies its
 * end distance, and the renderer's cull distance is the lower bound. */
double RE_GetFarPlaneDist()
{
  float result;

  result = r_zfar->value;
  if ( result == 0.0f )
  {
    if ( glfogNum > 0 && tr_fogCurrent_registered && tr_fogCurrent_mode == 9729 )   /* fog registered + GL_LINEAR */
      result = tr_fogCurrent_end;
  }
  if ( result < dpvs_cullDist )
    return dpvs_cullDist;
  return result;
}

/* ---- R_CullDlightsForBox  0x004E16D0 ----  VERIFIED
 * Keep only the dlight bits whose light sphere overlaps the box. */
int __cdecl R_CullDlightsForBox(int dlightBits, float *mins, float *maxs)
{
  int      result = 0;
  int      bit;
  dlight_t *light;

  for ( bit = 1, light = (dlight_t *)tr_refdef_dlights; dlightBits; bit *= 2, ++light )
  {
    if ( !(dlightBits & bit) )
      continue;
    dlightBits &= ~bit;
    if ( light->radius + light->transformedPosition[0] <= mins[0]
      || light->transformedPosition[0] - light->radius >= maxs[0]
      || light->radius + light->transformedPosition[1] <= mins[1]
      || light->transformedPosition[1] - light->radius >= maxs[1]
      || light->radius + light->transformedPosition[2] <= mins[2]
      || light->transformedPosition[2] - light->radius >= maxs[2] )
      continue;
    result |= bit;
  }
  return result;
}

/* ---- R_AddWorldSurfaceNoCull  0x004E1760 ----  VERIFIED
 * Stamp a world/model surface visible this view and hand it to R_AddDrawSurf.
 * Model surfaces (type < SF_INDEXED_FIRST) carry no dlights and use the global
 * storage class; indexed world surfaces carry their culled dlight mask and
 * their own storage mode. */
int __cdecl R_AddWorldSurfaceNoCull(int dlightBits, msurface_t *surf)
{
  srfLit_t *data;

  data = (srfLit_t *)surf->data;
  surf->viewCount = tr_viewCount;
  if ( data->surfaceType >= SF_INDEXED_FIRST )
  {
    if ( dlightBits )
    {
      dlightBits = R_CullDlightsForBox(dlightBits, data->boundsMin, data->boundsMax);
      data->dlightBits = dlightBits;
      dlightBits = dlightBits != 0;
    }
    return R_AddDrawSurf(data->storageMode, surf->shader, surf->data, dlightBits, 0, 0);
  }
  if ( dlightBits )
    dlightBits = 0;
  return R_AddDrawSurf(storageClass, surf->shader, data, dlightBits, 0, 0);
}

/* ---- R_AllocClipPlanes  0x004E17D0 ----  VERIFIED */
int __cdecl R_AllocClipPlanes(int planeCount)
{
  return ri.Hunk_AllocateTempMemory(20 * planeCount);   /* 20 == sizeof(dpvsPlane_t) */
}

/* ---- R_FreeClipPlanes  0x004E17F0 ----  VERIFIED */
void __cdecl R_FreeClipPlanes(int planes)
{
  ri.Hunk_FreeTempMemory(planes);
}

/* ---- R_FrustumClipPlanes  0x004E1800 ----  VERIFIED
 * Copy the four view-frustum planes into the DPVS frustum bank, biasing each
 * and precomputing its corner offsets. */
void R_FrustumClipPlanes()
{
  int i;

  for ( i = 0; i < TR_FRUSTUM_PLANES; ++i )
  {
    dpvs_frustumPlanesTyped[i].normal[0] = trView.frustum[i].normal[0];
    dpvs_frustumPlanesTyped[i].normal[1] = trView.frustum[i].normal[1];
    dpvs_frustumPlanesTyped[i].normal[2] = trView.frustum[i].normal[2];
    dpvs_frustumPlanesTyped[i].distance  = trView.frustum[i].dist;
    R_SetPlaneSidesDPVS((int)&dpvs_frustumPlanesTyped[i]);
  }
}

/* ---- R_SetupTransformMatrix  0x004E18A0 ----  VERIFIED
 * Build world.modelMatrix * projectionMatrix, invert it (kept for portal
 * unprojection), and store its transpose across the 16 int-typed matrix
 * globals used by R_PortalClipPlanesInternal. */
void R_SetupTransformMatrix()
{
  int          row;
  int          col;
  const float *world = trView.world.modelMatrix;
  const float *proj  = trView.projectionMatrix;
  float        combined[16];   /* retail's single 64-byte matrix, row-major */
#ifdef COD1_VC7
  int          k;
  float       *out;

  /* the k loop keeps VC7 from unrolling the row loop (retail walks it by pointer) */
  for ( col = 0; col < 4; ++col )
  {
    for ( row = 0; row < 4; ++row )
    {
      out = &combined[4 * row + col];
      *out = 0;
      for ( k = 0; k < 4; ++k )
        *out += world[4 * row + k] * proj[4 * k + col];
    }
  }
#else
  for ( col = 0; col < 4; ++col )
  {
    for ( row = 0; row < 4; ++row )
    {
      combined[4 * row + col] =
          world[4 * row + 3] * proj[12 + col]
        + world[4 * row + 0] * proj[col]
        + world[4 * row + 1] * proj[4 + col]
        + proj[8 + col] * world[4 * row + 2];
    }
  }
#endif

  MatrixInverse44(combined, dpvs_transformMatrixInverse);

  *(float *)&dpvs_transformMatrixTransposed = combined[0];
  *(float *)&dpvs_transformMatrixTransposed01 = combined[4];
  *(float *)&dpvs_transformMatrixTransposed02 = combined[8];
  *(float *)&dpvs_transformMatrixTransposed03 = combined[12];
  *(float *)&dpvs_transformMatrixTransposed10 = combined[1];
  *(float *)&dpvs_transformMatrixTransposed11 = combined[5];
  *(float *)&dpvs_transformMatrixTransposed12 = combined[9];
  *(float *)&dpvs_transformMatrixTransposed13 = combined[13];
  *(float *)&dpvs_transformMatrixTransposed20 = combined[2];
  *(float *)&dpvs_transformMatrixTransposed21 = combined[6];
  *(float *)&dpvs_transformMatrixTransposed22 = combined[10];
  *(float *)&dpvs_transformMatrixTransposed23 = combined[14];
  *(float *)&dpvs_transformMatrixTransposed30 = combined[3];
  *(float *)&dpvs_transformMatrixTransposed31 = combined[7];
  *(float *)&dpvs_transformMatrixTransposed32 = combined[11];
  *(float *)&dpvs_transformMatrixTransposed33 = combined[15];
}

/* ---- R_PortalClipPlanesInternal  0x004E19B0 ----  VERIFIED
 * Turn a clipped portal winding into a set of view-origin clip planes.  Each
 * winding edge contributes one plane through the view origin; when
 * r_portalbevels is positive, the winding's projected screen rectangle adds up
 * to four bevel planes (each rejected if any edge normal exceeds the configured
 * dot threshold).  Returns the temp-memory plane array; *outPlaneCount receives
 * the count.  (The retail body unrolls the vertex projection and edge loops
 * four-wide; collapsed here to plain loops, but the unrolled lanes do NOT use
 * identical x87 term orders, so the loops below reproduce each lane's order:
 * projection groups (y+z)+x vs remainder (z+x)+y; edge-plane distance lane 0
 * of each group (z+x)+y vs everything else (z+y)+x.  Every vertex is projected
 * into the screen rectangle -- retail has no clip-space w test.)  The
 * caller also passes the portal's target cell, which retail never reads. */
dpvsPlane_t *__cdecl R_PortalClipPlanesInternal(int *outPlaneCount, int vertexCount, vec3_t *vertices, dpvsCell_t *cell)
{
  const float *inv = dpvsGlob.transformMatrixInverse;
  int          unrolled = vertexCount & ~3;   
  int          planeCount;
  int          i;
  int          k;
  int          c;
  dpvsPlane_t *outPlanes;
  const float *wvp = (const float *)dpvsGlob.transformMatrixTransposed;
  float        bevelCorners[5][3];   
  float        relVerts[1025][3];    
  float        edgeNormals[1024][3]; 



  if ( r_portalbevels->value > 0.0f )
  {
    float minX = 1.0f;
    float minY = 1.0f;
    float maxX = -1.0f;
    float maxY = -1.0f;
    outPlanes = (dpvsPlane_t *)ri.Hunk_AllocateTempMemory(20 * (vertexCount + 4));

    

    for ( i = 0; i < vertexCount; ++i )
    {
      const float *v = vertices[i];
      float clipW, invW, px, py;

      if ( i < unrolled )
      {
        clipW = wvp[13] * v[1] + wvp[14] * v[2] + wvp[12] * v[0] + wvp[15];
        invW  = 1.0f / clipW;
        px = ( wvp[1] * v[1] + wvp[2] * v[2] + wvp[0] * v[0] + wvp[3] ) * invW;
        py = ( wvp[5] * v[1] + wvp[6] * v[2] + wvp[4] * v[0] + wvp[7] ) * invW;
      }
      else
      {
        clipW = wvp[14] * v[2] + wvp[12] * v[0] + wvp[13] * v[1] + wvp[15];
        invW  = 1.0f / clipW;
        px = ( wvp[2] * v[2] + wvp[0] * v[0] + wvp[1] * v[1] + wvp[3] ) * invW;
        py = ( wvp[6] * v[2] + wvp[4] * v[0] + wvp[5] * v[1] + wvp[7] ) * invW;
      }

      if ( minX > px )
        minX = px;
      if ( maxX < px )
        maxX = px;
      if ( minY > py )
        minY = py;
      if ( maxY < py )
        maxY = py;
    }

    for ( k = 0; k < 4; ++k )
    {
      float px = k < 2 ? minX : maxX;
      float py = ( k == 1 || k == 2 ) ? minY : maxY;
      float h[4];
      float invW;

      for ( c = 0; c < 4; ++c )
        h[c] = inv[4 + c] * py + inv[c] * px + inv[12 + c];
      invW = 1.0f / h[3];
      bevelCorners[k][0] = h[0] * invW;
      bevelCorners[k][1] = h[1] * invW;
      bevelCorners[k][2] = h[2] * invW;
    }
    bevelCorners[4][0] = bevelCorners[0][0];
    bevelCorners[4][1] = bevelCorners[0][1];
    bevelCorners[4][2] = bevelCorners[0][2];
  }

  else
    outPlanes = (dpvsPlane_t *)ri.Hunk_AllocateTempMemory(20 * vertexCount);

  for ( i = 0; i < vertexCount; ++i )
  {
    relVerts[i][0] = vertices[i][0] - (*(float *)&dpvs_viewOrigin);
    relVerts[i][1] = vertices[i][1] - (*(float *)&dpvs_viewOriginY);
    relVerts[i][2] = vertices[i][2] - (*(float *)&dpvs_viewOriginZ);
  }
  relVerts[vertexCount][0] = relVerts[0][0];
  relVerts[vertexCount][1] = relVerts[0][1];
  relVerts[vertexCount][2] = relVerts[0][2];

  for ( i = 0; i < vertexCount; ++i )
  {
    const float *cur = relVerts[i];
    const float *nxt = relVerts[i + 1];

    edgeNormals[i][0] = nxt[1] * cur[2] - nxt[2] * cur[1];
    edgeNormals[i][1] = nxt[2] * cur[0] - nxt[0] * cur[2];
    edgeNormals[i][2] = nxt[0] * cur[1] - nxt[1] * cur[0];
    VectorNormalizeFast( edgeNormals[i] );
  }

  planeCount = 0;

  if ( r_portalbevels->value > 0.0f )
  {
    float relBevel[5][3];

    for ( k = 0; k <= 4; ++k )
    {
      relBevel[k][0] = bevelCorners[k][0] - (*(float *)&dpvs_viewOrigin);
      relBevel[k][1] = bevelCorners[k][1] - (*(float *)&dpvs_viewOriginY);
      relBevel[k][2] = bevelCorners[k][2] - (*(float *)&dpvs_viewOriginZ);
    }

    for ( k = 0; k < 4; ++k )
    {
      const float *cur = relBevel[k];
      const float *nxt = relBevel[k + 1];
      float        n[3];
      int          rejected = 0;

      n[0] = nxt[1] * cur[2] - nxt[2] * cur[1];
      n[1] = nxt[2] * cur[0] - nxt[0] * cur[2];
      n[2] = nxt[0] * cur[1] - nxt[1] * cur[0];
      VectorNormalizeFast( n );

      for ( i = 0; i < vertexCount; ++i )
      {
        
        if ( edgeNormals[i][0] * n[0] + edgeNormals[i][2] * n[2] + edgeNormals[i][1] * n[1] > r_portalbevels->value )
        {
          rejected = 1;
          break;
        }
      }

      if ( rejected )
      {
        if ( (r_showportals->integer & 2) != 0 )
          R_AddDebugLine( bevelCorners[k], bevelCorners[k + 1], rejectedBevelDebugColor );
        continue;
      }

      if ( r_showportals->integer )
        R_AddDebugLine( bevelCorners[k], bevelCorners[k + 1], acceptedBevelDebugColor );

      {
        dpvsPlane_t *plane = &outPlanes[planeCount++];
        plane->normal[0] = n[0];
        plane->normal[1] = n[1];
        plane->normal[2] = n[2];
        plane->distance = (*(float *)&dpvs_viewOriginZ) * plane->normal[2]
                        + (*(float *)&dpvs_viewOriginY) * plane->normal[1]
                        + (*(float *)&dpvs_viewOrigin) * plane->normal[0];
        R_SetPlaneSidesDPVS( (int)plane );
      }
    }
  }

  for ( i = 0; i < vertexCount; ++i )
  {
    dpvsPlane_t *plane;

    


    if ( edgeNormals[i][1] * edgeNormals[i][1]
       + edgeNormals[i][2] * edgeNormals[i][2]
       + edgeNormals[i][0] * edgeNormals[i][0] == 0.0f )
      continue;

    plane = &outPlanes[planeCount++];
    plane->normal[0] = edgeNormals[i][0];
    plane->normal[1] = edgeNormals[i][1];
    plane->normal[2] = edgeNormals[i][2];
    

    if ( i < unrolled && (i & 3) == 0 )
      plane->distance = (*(float *)&dpvs_viewOriginZ) * plane->normal[2]
                      + (*(float *)&dpvs_viewOrigin) * plane->normal[0]
                      + (*(float *)&dpvs_viewOriginY) * plane->normal[1];
    else
      plane->distance = (*(float *)&dpvs_viewOriginZ) * plane->normal[2]
                      + (*(float *)&dpvs_viewOriginY) * plane->normal[1]
                      + (*(float *)&dpvs_viewOrigin) * plane->normal[0];
    R_SetPlaneSidesDPVS( (int)plane );
  }

  *outPlaneCount = planeCount;
  return outPlanes;
}


/* occluder camera-side markers stored in dpvsPlane_t::cameraSide */
#define DPVS_CAMERA_BEHIND     0
#define DPVS_CAMERA_IN_FRONT   1

/* ---- R_CullOccluderByPlane  0x004E2810 ----  VERIFIED
 * True when every occluder vertex lies on or behind the plane (the occluder is
 * wholly clipped away by it). */
int __cdecl R_CullOccluderByPlane(int occluderPtr, float *plane)
{
  dpvsOccluder_t *occ = (dpvsOccluder_t *)occluderPtr;
  int             i;

  for ( i = 0; i < occ->vertexCount; ++i )
  {
    const float *v = occ->vertices[i];
    if ( v[2]*plane[2] + v[1]*plane[1] + v[0]*plane[0] > plane[3] )
      return 0;
  }
  return 1;
}

/* ---- R_AddCellOccluders  0x004E2860 ----  [HIGH]
 * Install the cell's visible occluders for this portal step: reject occluders
 * fully behind the active near plane or clipped by the incoming planes, then
 * copy each camera-facing authored face plane and build one silhouette plane
 * (through the view origin) per edge that separates opposite camera sides. */
void __cdecl R_AddCellOccluders(int cellPtr, float *planes, int planeCount)
{
  dpvsCell_t        *cell = (dpvsCell_t *)cellPtr;
  int                occIdx;


  dpvsGlob.occluderCount = 0;
  dpvs_activePlaneCount = 0;

  for ( occIdx = 0; occIdx < cell->occluderCount; ++occIdx )
  {
    

    dpvsOccluder_t *occ = cell->occluders[occIdx];
    int             clipped;
    int             planeIdx;
    int             vi;
    int             edgeIdx;

    
    {
      const dpvsPlane_t *plane = (const dpvsPlane_t *)dpvs_activeNearPlane;
      int culled = 1;

      for ( vi = 0; vi < occ->vertexCount; ++vi )
      {
        const float *v = occ->vertices[vi];
        if ( v[2]*plane->normal[2] + v[1]*plane->normal[1] + v[0]*plane->normal[0] > plane->distance )
        {
          culled = 0;
          break;
        }
      }
      if ( culled )
        continue;
    }

    clipped = 0;
    while ( planeCount )
    {
      if ( R_CullOccluderByPlane((int)occ, planes) )
      {
        clipped = 1;
        break;
      }
      --planeCount;
      planes += 5;   
    }
    if ( clipped )
      continue;

    if ( dpvsGlob.occluderCount == 1024 )
      ri.Error(1, "\x15" "Too many active occluder brushes");
    ((dpvsOccluder_t **)dpvsGlob.occluders)[dpvsGlob.occluderCount++] = occ;

    occ->cullPlaneLimit = 0x7FFFFFFF;
    occ->activePlaneCount = dpvs_activePlaneCount;   
    occ->activePlanes = &((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount];

    
    /* occ->planes is re-read after every store, as retail */
    for ( planeIdx = 0; planeIdx < occ->planeCount; ++planeIdx )
    {
      occ->planes[planeIdx].cameraSide = 0;
      if ( (*(float *)&dpvs_viewOriginY)*occ->planes[planeIdx].normal[1]
         + (*(float *)&dpvs_viewOriginZ)*occ->planes[planeIdx].normal[2]
         + (*(float *)&dpvs_viewOrigin)*occ->planes[planeIdx].normal[0] > occ->planes[planeIdx].distance )
      {
        occ->planes[planeIdx].cameraSide = 1;
        if ( dpvs_activePlaneCount == 6144 )
          ri.Error(1, "\x15" "Too many active occluder faces");
        ((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount] = occ->planes[planeIdx];
        ++dpvs_activePlaneCount;
      }
    }

    
    for ( edgeIdx = 0; edgeIdx < occ->edgeCount; ++edgeIdx )
    {
      unsigned char       cameraSide = occ->edges[edgeIdx].planes[0]->cameraSide;
      const float        *firstVertex;
      const float        *secondVertex;
      float               first[3];
      float               second[3];

      if ( cameraSide == occ->edges[edgeIdx].planes[1]->cameraSide )
        continue;

      firstVertex  = (const float *)occ->edges[edgeIdx].vertices[cameraSide == 0 ? 1 : 0];
      secondVertex = (const float *)occ->edges[edgeIdx].vertices[cameraSide];
      first[0]  = firstVertex[0]  - (*(float *)&dpvs_viewOrigin);
      first[1]  = firstVertex[1]  - (*(float *)&dpvs_viewOriginY);
      first[2]  = firstVertex[2]  - (*(float *)&dpvs_viewOriginZ);
      second[0] = secondVertex[0] - (*(float *)&dpvs_viewOrigin);
      second[1] = secondVertex[1] - (*(float *)&dpvs_viewOriginY);
      second[2] = secondVertex[2] - (*(float *)&dpvs_viewOriginZ);

      /* the new plane is re-read through the bank after every store, as retail */
      CrossProduct( second, first, ((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount].normal );
      VectorNormalizeFast( ((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount].normal );
      ((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount].distance =
          (*(float *)&dpvs_viewOriginZ)*((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount].normal[2]
        + (*(float *)&dpvs_viewOriginY)*((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount].normal[1]
        + (*(float *)&dpvs_viewOrigin)*((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount].normal[0];
      R_SetPlaneSidesDPVS( (int)&((dpvsPlane_t *)dpvs_activePlanes)[dpvs_activePlaneCount] );
      ++dpvs_activePlaneCount;
    }

    occ->activePlaneCount = dpvs_activePlaneCount - occ->activePlaneCount;
  }
}

/* ---- R_CellForCamera  0x004E2BE0 ----  VERIFIED
 * Descend the world BSP from the root by the view origin; nonpositive and
 * unordered plane distances take the back child.  Returns the terminal cell. */
int R_CellForCamera()
{
  mnode_t *node;

  if ( !tr_world )
    ri.Error(1, "\x15" "R_CellForCamera: bad model");

  node = DPVS_WORLD_NODES;
  while ( node->contents == NODE_INTERNAL )
  {
    const cplane_t *plane = node->u.node.plane;
    /* retail tests the unrounded x87 value: fcomp 0.0 / test ah,41h / jne
     * children[1] (0x4E2C34-0x4E2C46), so only d > 0 (ordered) goes front */
    rfloat          d = *(float *)&dpvs_viewOriginY*plane->normal[1]
                      + *(float *)&dpvs_viewOriginZ*plane->normal[2]
                      + *(float *)&dpvs_viewOrigin*plane->normal[0]
                      - plane->dist;
    node = d > 0.0f ? node->u.node.children[0] : node->u.node.children[1];
  }
  return node->cellIndex;
}

/* ---- R_PortalBehindAnyPlane  0x004E2C60 ----  VERIFIED
 * True when some supplied plane has no portal vertex strictly in front. */
int __cdecl R_PortalBehindAnyPlane(int portalPtr, float *planes, int planeCount)
{
  dpvsPortal_t *portal = (dpvsPortal_t *)portalPtr;

  /* retail counts both loops down to zero (dec/test/jne, 0x4E2C76 and
   * 0x4E2C9E), so a negative count keeps going instead of ending the loop */
  for ( ; planeCount; --planeCount, planes += 5 )
  {
    const float *v = portal->vertices[0];
    unsigned     i;
    for ( i = (unsigned)portal->vertexCount; ; --i, v += 3 )
    {
      if ( !i )
        return 1;
      if ( v[2]*planes[2] + v[1]*planes[1] + v[0]*planes[0] > planes[3] )
        break;
    }
  }
  return 0;
}

/* ---- R_PortalBehindAllPlanes  0x004E2CC0 ----  VERIFIED
 * True when every portal vertex is non-front-facing for every supplied plane. */
int __cdecl R_PortalBehindAllPlanes(int portalPtr, float *planes, int planeCount)
{
  dpvsPortal_t *portal = (dpvsPortal_t *)portalPtr;

  /* count-down loops as in retail (0x4E2CFE and 0x4E2D06), see above */
  for ( ; planeCount; --planeCount, planes += 5 )
  {
    const float *v = portal->vertices[0];
    unsigned     i;
    for ( i = (unsigned)portal->vertexCount; i; --i, v += 3 )
    {
      if ( v[2]*planes[2] + v[1]*planes[1] + v[0]*planes[0] > planes[3] )
        return 0;
    }
  }
  return 1;
}

/* ---- R_ChopPortalWinding  0x004E2D20 ----  VERIFIED
 * Clip a winding against one plane, keeping the front side.  Returns the input
 * unchanged when nothing is behind, NULL (with *vertexCount cleared) when
 * nothing is in front, else the clipped winding in outVertices.  An ON vertex
 * is copied but never splits an edge. */
float *__cdecl R_ChopPortalWinding(const dpvsPlane_t *plane, vec3_t *inVertices, int *vertexCount, vec3_t *outVertices)
{
  int          sides[1025];
  float        dists[1025];
  int          frontCount = 0;
  int          backCount = 0;
  int          outCount;
  int          i;
  const float *v;
  const float *p1;
  const float *p2;
  rfloat       frac;

  for ( i = 0; i < *vertexCount; ++i )
  {
    /* retail classifies the unrounded x87 value and only fst's a float copy
     * into dists[] (0x4E2D6C-0x4E2D90) */
    rfloat d;

    v = inVertices[i];
    sides[i] = 2;   /* ON */
    d = plane->normal[2]*v[2] + plane->normal[0]*v[0] + plane->normal[1]*v[1];
    d -= plane->distance;
    dists[i] = (float)d;
    if ( d < -DPVS_PLANE_BIAS )
    {
      sides[i] = 1;   /* BACK */
      ++backCount;
    }
    else if ( d > DPVS_PLANE_BIAS )
    {
      sides[i] = 0;   /* FRONT */
      ++frontCount;
    }
  }

  if ( !frontCount )
  {
    *vertexCount = 0;
    return 0;
  }
  if ( !backCount )
    return (float *)inVertices;

  sides[i] = sides[0];
  dists[i] = dists[0];

  outCount = 0;
  for ( i = 0; i < *vertexCount; ++i )
  {
    p1 = inVertices[i];
    if ( sides[i] == 2 )   /* ON: copy, no split */
    {
      VectorCopy(p1, outVertices[outCount]);
      ++outCount;
      continue;
    }
    if ( sides[i] == 0 )   /* FRONT: copy */
    {
      VectorCopy(p1, outVertices[outCount]);
      ++outCount;
    }
    if ( sides[i + 1] == 2 || sides[i + 1] == sides[i] )
      continue;

    /* the edge crosses the plane -- emit the intersection point; retail keeps
     * frac on the x87 stack (0x4E2E57-0x4E2EB1) */
    p2 = inVertices[(i + 1) % *vertexCount];
    frac = dists[i] / (dists[i] - dists[i + 1]);
    outVertices[outCount][0] = (p2[0] - p1[0])*frac + p1[0];
    outVertices[outCount][1] = (p2[1] - p1[1])*frac + p1[1];
    outVertices[outCount][2] = (p2[2] - p1[2])*frac + p1[2];
    ++outCount;
    if ( outCount == 1024 )
      break;
  }

  *vertexCount = outCount;
  return (float *)outVertices;
}

/* ---- R_PortalClipPlanes  0x004E2EE0 ----  VERIFIED
 * Clip the portal winding by its source plane, the far plane and every
 * incoming plane, then convert the survivors into a tighter view-origin plane
 * set.  Portals within one unit of the camera skip clipping and reuse a copy of
 * the incoming planes. */
char *__cdecl R_PortalClipPlanes(int planeCount, int *outPlaneCount, dpvsPortal_t *portal, int sourcePlane, char *planes)   /* planeCount first: retail's ecx = outPlaneCount, edx = planeCount */
{
  vec3_t        windings[2][1024];   /* two 1024-vertex ping-pong banks */
  float        *verts;
  char         *result;
  int           i;

  *outPlaneCount = portal->vertexCount;
  verts = (float *)portal->vertices;
  verts = R_ChopPortalWinding((const dpvsPlane_t *)sourcePlane, (vec3_t *)verts, outPlaneCount, windings[0]);
  if ( !*outPlaneCount )
    return 0;

  if ( portal->plane.distance
     - ( *(float *)&dpvs_viewOrigin*portal->plane.normal[0]
       + ( *(float *)&dpvs_viewOriginY*portal->plane.normal[1]
         + *(float *)&dpvs_viewOriginZ*portal->plane.normal[2] ) ) < 1.0f )
  {
    *outPlaneCount = planeCount;
    result = (char *)R_AllocClipPlanes(planeCount);
    memcpy(result, planes, 20 * planeCount);
    return result;
  }

  if ( dpvs_activeFarPlane )
  {
    verts = R_ChopPortalWinding((const dpvsPlane_t *)dpvs_activeFarPlane, (vec3_t *)verts, outPlaneCount,
                                windings[verts == (float *)windings[0]]);
    if ( !*outPlaneCount )
      return 0;
  }

  for ( i = 0; i < planeCount; ++i )
  {
    verts = R_ChopPortalWinding(&((const dpvsPlane_t *)planes)[i], (vec3_t *)verts, outPlaneCount,
                                windings[verts == (float *)windings[0]]);
    if ( !*outPlaneCount )
      return 0;
  }

  if ( r_showportals->integer )
    R_AddDebugPolygon(colorDebugPortal, *outPlaneCount, verts);

  return (char *)R_PortalClipPlanesInternal(outPlaneCount, *outPlaneCount, (vec3_t *)verts, portal->cell);
}


/* ---- R_AddTrianglesSurface  0x004E3070 ----  [HIGH]
 * Cull one world surface's bounds and, if visible, hand it to the draw list. */
void __cdecl R_AddTrianglesSurface(int planeCount, msurface_t *surf, int dlightBits, dpvsPlane_t *planes, int planeIndex)
{
  srfLit_t *data;

  if ( surf->viewCount == tr_viewCount )
    return;

  data = (srfLit_t *)surf->data;
  if ( R_CullBoxDPVS((const vec3_t *)&data->boundsMin, planes, planeCount, planeIndex) )
    return;

  if ( (r_showaabbtrees->integer & 2) != 0 )
    R_AddDebugBox( data->boundsMin, data->boundsMax,colorDebugGreen);

  R_AddWorldSurfaceNoCull(dlightBits, surf);
}

/* ---- R_AddAABBTreeSurfaces_r  0x004E3210 ----  VERIFIED
 * Descend a cell's surface AABB tree.  A subtree wholly in front of every plane
 * and unoccluded is submitted without descending; a partially visible internal
 * node recurses; a partially visible leaf submits per surface. */
void __cdecl R_AddAABBTreeSurfaces_r(dpvsAabbTree_t *tree, dpvsPlane_t *planes, int planeCount, int dlightBits, int planeIndex)
{
  int i;

  if ( R_CullBoxDPVS((const vec3_t *)&tree->mins, planes, planeCount, planeIndex) )
    return;

  if ( dlightBits )
    dlightBits = R_CullDlightsForBox(dlightBits, tree->mins, tree->maxs);

  /* Mac 1.3 order: the partially visible case first, the fully visible
     (no-cull) submit in the else branch */
  if ( R_CullBoxDPVSStrict((const vec3_t *)&tree->mins, planes, planeCount, planeIndex) )
  {
    if ( tree->childCount )
    {
      for ( i = 0; i < tree->childCount; ++i )
        R_AddAABBTreeSurfaces_r(&tree->children[i], planes, planeCount, dlightBits, planeIndex + 1);
    }
    else
    {
      if ( r_showaabbtrees->integer )
        R_AddDebugBox( tree->mins, tree->maxs,clippedLeafDebugColor);
      for ( i = 0; i < tree->surfaceCount; ++i )
        R_AddTrianglesSurface(planeCount, &tree->surfaces[i], dlightBits, planes, planeIndex + 1);
    }
  }
  else
  {
    if ( r_showaabbtrees->integer )
      R_AddDebugBox( tree->mins, tree->maxs,colorDebugYellow);
    for ( i = 0; i < tree->surfaceCount; ++i )
    {
      if ( tree->surfaces[i].viewCount != tr_viewCount )
        R_AddWorldSurfaceNoCull(dlightBits, &tree->surfaces[i]);
    }
  }
}


/* ---- R_CullModels  0x004E3560 ----  [HIGH]
 * Admit any brush/xmodel entity linked into this cell that survives the box
 * test; an entity already admitted by another cell is left alone. */
void __cdecl R_CullModels(int cellPtr, int planesPtr, int planeCount)
{
  dpvsCell_t           *cell = (dpvsCell_t *)cellPtr;
  dpvsPlane_t          *planes = (dpvsPlane_t *)planesPtr;
  dpvsCellEntityLink_t *link;

  for ( link = cell->entityLinks; link; link = link->next )
  {
    if ( link->entity->cullState == CULL_IN )
      continue;
    if ( !R_CullBoxDPVS((const vec3_t *)link->mins, planes, planeCount, 0) )
      link->entity->cullState = CULL_IN;
  }
}

/* ---- R_XModelWorldBounds  0x004E3700 ----  [HIGH]
 * World-space AABB for an xmodel entity: the first DObj model's authored bounds
 * scaled by the largest entity-axis length. */
void __cdecl R_XModelWorldBounds(float *outMins, int entityPtr, float *outMaxs)
{
  trRefEntity_t *ent = (trRefEntity_t *)entityPtr;
  float          axisLenSq[3];
  rfloat         maxLenSq;
  rfloat         scale;
  const float   *modelInfo;
  vec3_t         mins;
  vec3_t         maxs;

  /* retail keeps axis 0's squared length unrounded on the x87 stack as the
   * starting maximum (0x4E371C fst dword + 0x4E3720 fstp st(3)); only its
   * float copy is used in the first comparison (0x4E376C).  Axes 1 and 2 are
   * stored as floats.  VC7 forwards the unrounded value into maxLenSq; the
   * modern build computes it in double (products of floats are exact). */
#ifdef COD1_VC7
  axisLenSq[0] = ent->e.axis[0][0]*ent->e.axis[0][0]
               + ent->e.axis[0][1]*ent->e.axis[0][1]
               + ent->e.axis[0][2]*ent->e.axis[0][2];
  maxLenSq = axisLenSq[0];
#else
  maxLenSq = (double)ent->e.axis[0][0]*ent->e.axis[0][0]
           + (double)ent->e.axis[0][1]*ent->e.axis[0][1]
           + (double)ent->e.axis[0][2]*ent->e.axis[0][2];
  axisLenSq[0] = (float)maxLenSq;
#endif
  axisLenSq[1] = ent->e.axis[1][0]*ent->e.axis[1][0]
               + ent->e.axis[1][1]*ent->e.axis[1][1]
               + ent->e.axis[1][2]*ent->e.axis[1][2];
  axisLenSq[2] = ent->e.axis[2][0]*ent->e.axis[2][0]
               + ent->e.axis[2][1]*ent->e.axis[2][1]
               + ent->e.axis[2][2]*ent->e.axis[2][2];
  if ( axisLenSq[0] < axisLenSq[1] )
    maxLenSq = axisLenSq[1];
  if ( maxLenSq < axisLenSq[2] )
    maxLenSq = axisLenSq[2];
  scale = sqrt(maxLenSq);

  /* entity->e.dobj->models[0]->info -- kept as raw offsets (XModel internals) */
  modelInfo = *(const float **)(*(int *)(*(int *)(entityPtr + 144) + 24) + 4);
  VectorCopy(&modelInfo[19], mins);   /* +0x4c */
  VectorCopy(&modelInfo[22], maxs);   /* +0x58 */

  VectorMA(ent->e.origin, scale, mins, outMins);
  VectorMA(ent->e.origin, scale, maxs, outMaxs);
}

/* ---- R_BModelWorldBounds  0x004E3810 ----  [HIGH]
 * World-space AABB for a brush-model entity: its 8 local corners transformed by
 * the entity axis/origin and merged. */
void __cdecl R_BModelWorldBounds( trRefEntity_t *ent, float *mins, float *maxs )
{
  bmodel_t *bmodel = ((model_t *)tr_models[ent->e.hModel])->bmodel;
  int       i;
  vec3_t    local;
  vec3_t    world;

  ClearBounds( mins, maxs );
  for ( i = 0; i < 8; i++ )
  {
    local[0] = bmodel->bounds[i & 1][0];
    local[1] = bmodel->bounds[(i >> 1) & 1][1];
    local[2] = bmodel->bounds[(i >> 2) & 1][2];
    VectorCopy( ent->e.origin, world );
    VectorMA( world, local[0], ent->e.axis[0], world );
    VectorMA( world, local[1], ent->e.axis[1], world );
    VectorMA( world, local[2], ent->e.axis[2], world );
    AddPointToBounds( world, mins, maxs );
  }
}

/* ---- R_AddModelToCell  0x004E3940 ----  VERIFIED
 * Link an entity into a cell (merging bounds on a repeat visit), from the
 * caller-owned link scratch bank. */
void __cdecl R_AddModelToCell(int cellPtr, int minsPtr, int maxsPtr, int entityPtr)
{
  dpvsCell_t           *cell = (dpvsCell_t *)cellPtr;
  float                *mins = (float *)minsPtr;
  float                *maxs = (float *)maxsPtr;
  trRefEntity_t        *entity = (trRefEntity_t *)entityPtr;
  dpvsCellEntityLink_t *link;

  if ( dpvs_cellEntityLinkCount == DPVS_MAX_CELL_ENTITY_LINKS )
  {
    ri.Printf(1, "^1Max xmodel refs (%i) exceeded\n", DPVS_MAX_CELL_ENTITY_LINKS);
    return;
  }

  for ( link = cell->entityLinks; link; link = link->next )
  {
    if ( link->entity == entity )
    {
      ExpandBounds(mins, link->mins, maxs, link->maxs);
      return;
    }
  }

  link = &((dpvsCellEntityLink_t *)dpvs_cellEntityLinks)[dpvs_cellEntityLinkCount];
  ++dpvs_cellEntityLinkCount;
  link->entity = entity;
  link->mins[0] = mins[0];
  link->mins[1] = mins[1];
  link->mins[2] = mins[2];
  link->maxs[0] = maxs[0];
  link->maxs[1] = maxs[1];
  link->maxs[2] = maxs[2];
  link->next = cell->entityLinks;
  cell->entityLinks = link;
}

/* ---- R_FilterModelIntoCells_r  0x004E39D0 ----  VERIFIED
 * Recurse the world BSP, linking the entity into every cell its box reaches.
 * Returns the common terminal cell index, or the internal sentinel when
 * branches disagree, or the no-cell sentinel for a box outside all cells. */
int __cdecl R_FilterModelIntoCells_r(unsigned int *nodePtr, int entity, unsigned int *minsPtr, unsigned int *maxsPtr)
{
  mnode_t *node = (mnode_t *)nodePtr;
  float   *mins = (float *)minsPtr;
  float   *maxs = (float *)maxsPtr;
  const cplane_t *plane;
  int      side;
  int      firstCell;
  int      secondCell;
  float    frontMins[3];
  float    backMaxs[3];

  /* retail re-reads node->plane (and its type) after every call */
  while ( node->cellIndex == DPVS_NODE_INTERNAL )
  {
    side = BoxOnPlaneSide((int)mins, (int)maxs, (int)node->u.node.plane);
    if ( side == 3 )
    {
      plane = node->u.node.plane;
      if ( plane->type < 3 )
      {
        frontMins[0] = mins[0];
        frontMins[1] = mins[1];
        frontMins[2] = mins[2];
        frontMins[plane->type] = plane->dist;
        backMaxs[0] = maxs[0];
        backMaxs[1] = maxs[1];
        backMaxs[2] = maxs[2];
        backMaxs[plane->type] = plane->dist;

        secondCell = firstCell = R_FilterModelIntoCells_r((unsigned int *)node->u.node.children[1], entity, minsPtr, (unsigned int *)backMaxs);
        if ( frontMins[node->u.node.plane->type] < maxs[node->u.node.plane->type] )
          firstCell = R_FilterModelIntoCells_r((unsigned int *)node->u.node.children[0], entity, (unsigned int *)frontMins, maxsPtr);
      }
      else
      {
        firstCell = R_FilterModelIntoCells_r((unsigned int *)node->u.node.children[0], entity, minsPtr, maxsPtr);
        secondCell = R_FilterModelIntoCells_r((unsigned int *)node->u.node.children[1], entity, minsPtr, maxsPtr);
      }
      return firstCell == secondCell ? firstCell : DPVS_NODE_INTERNAL;
    }
    node = node->u.node.children[side - 1];
  }

  if ( node->cellIndex >= 0 )
    R_AddModelToCell((int)&DPVS_WORLD_CELLS[node->cellIndex], (int)mins, (int)maxs, entity);
  return node->cellIndex;
}

/* ---- R_XModelIsHuge  0x004E3B20 ----  VERIFIED
 * True when at least two world-bounds dimensions exceed 1536 units.  This
 * x-split (x: y || z, else y && z) form matches retail both out of line and
 * where VC7 inlines it into R_FilterModelsIntoCells. */
int __fastcall R_XModelIsHuge(int minsPtr, int maxsPtr)
{
  float *mins = (float *)minsPtr;
  float *maxs = (float *)maxsPtr;

  if ( maxs[0] - mins[0] > 1536.0f )
  {
    if ( maxs[1] - mins[1] > 1536.0f || maxs[2] - mins[2] > 1536.0f )
      return 1;
    return 0;
  }
  if ( maxs[1] - mins[1] > 1536.0f && maxs[2] - mins[2] > 1536.0f )
    return 1;
  return 0;
}

/* ---- R_FilterModelsIntoCells  0x004E3B70 ----  VERIFIED
 * Per-view pass: clear each cell's entity list, then for each enabled brush /
 * xmodel entity build its world bounds, reject it against the DPVS planes, and
 * either leave it globally visible (huge / outside all cells) or link it into
 * the cells it touches for per-cell culling. */
void __cdecl R_FilterModelsIntoCells(int planesPtr, int planeCount)
{
  dpvsPlane_t *planes = (dpvsPlane_t *)planesPtr;
  int          filterBrush = r_cullBModels->integer && r_drawBModels->integer;
  int          filterX = r_cullXModels->integer && r_drawXModels->integer;
  int          i;

  for ( i = 0; i < DPVS_WORLD_CELL_COUNT; ++i )
  {
    DPVS_WORLD_CELLS[i].entityLinks = 0;
  }
  dpvs_cellEntityLinkCount = 0;

  /* brush and xmodel entities each run their own inlined DPVS cull, as retail */
  for ( i = 0; i < tr_refdef_num_entities; ++i )
  {
    /* no cached entity pointer: retail re-reads tr_refdef_entities for every access */
#define entity ( &((trRefEntity_t *)tr_refdef_entities)[i] )
    float          bounds[6];
    float         *mins = bounds;
    float         *maxs = bounds + 3;

    entity->cullState = CULL_IN;
    switch ( entity->e.reType )
    {
    case DPVS_RT_BRUSH_MODEL:
      if ( !filterBrush )
        break;
      entity->cullState = CULL_OUT;
      R_BModelWorldBounds(entity, mins, maxs);
      if ( R_CullBoxDPVS((const vec3_t *)mins, planes, planeCount, 0) )
        break;
      if ( R_FilterModelIntoCells_r((unsigned int *)DPVS_WORLD_NODES, (int)entity, (unsigned int *)mins, (unsigned int *)maxs) == DPVS_NODE_NO_CELL )
        entity->cullState = CULL_IN;
      if ( r_showCullBModels->integer == DPVS_SHOWCULL_BOUNDS )
        R_AddDebugBox( mins, maxs,modelBoundsDebugColor);
      break;
    case DPVS_RT_XMODEL:
      if ( !filterX )
        break;
      entity->cullState = CULL_OUT;
      R_XModelWorldBounds(mins, (int)entity, maxs);
      if ( R_CullBoxDPVS((const vec3_t *)mins, planes, planeCount, 0) )
        break;
      if ( R_XModelIsHuge((int)mins, (int)maxs) )
        entity->cullState = CULL_IN;
      else if ( R_FilterModelIntoCells_r((unsigned int *)DPVS_WORLD_NODES, (int)entity, (unsigned int *)mins, (unsigned int *)maxs) == DPVS_NODE_NO_CELL )
        entity->cullState = CULL_IN;
      if ( r_showCullXModels->integer == DPVS_SHOWCULL_BOUNDS )
        R_AddDebugBox( mins, maxs,modelBoundsDebugColor);
      break;
    }
#undef entity
  }
}


/* ---- R_AddCellSurfaces  0x004E4080 ----  VERIFIED
 * Submit a cell's owned world geometry: descend its AABB tree when it has
 * children, else submit its contiguous surface span; then cull the cell's
 * linked entities. */
void __cdecl R_AddCellSurfaces(int cellPtr, int planes, int planeCount, int dlightBits)
{
  dpvsCell_t *cell = (dpvsCell_t *)cellPtr;
  int         i;

  /* retail re-reads cell->aabbTree after every call */
  dpvs_cullPlaneLimit = 0x7FFFFFFF;
  if ( cell->aabbTree->childCount )
  {
    for ( i = 0; i < cell->aabbTree->childCount; ++i )
      R_AddAABBTreeSurfaces_r(&cell->aabbTree->children[i], (dpvsPlane_t *)planes, planeCount, dlightBits, 0);
  }
  else
  {
    for ( i = 0; i < cell->aabbTree->surfaceCount; ++i )
      R_AddTrianglesSurface(planeCount, &cell->aabbTree->surfaces[i], dlightBits, (dpvsPlane_t *)planes, 0);
  }

  if ( r_drawentities->integer )
    R_CullModels(cellPtr, planes, planeCount);
}

/* ---- R_AddCullGroupDPVS  0x004E4120 ----  VERIFIED
 * Cull one cull group's bounds; if visible, stamp it and submit its surfaces
 * (retail counts the surfaces down to zero, dec/jne at 0x4E42A0). */
void __cdecl R_AddCullGroupDPVS(dpvsCullGroup_t *group, dpvsPlane_t *planes, int planeCount, int dlightBits)
{
  msurface_t *surf;
  int         count;

  if ( R_CullBoxDPVS((const vec3_t *)&group->mins, planes, planeCount, 0) )
    return;

  if ( (r_showportals->integer & 1) != 0 )
    R_AddDebugBox( group->mins, group->maxs,cullGroupDebugColor);

  group->viewCount = tr_viewCount;
  for ( surf = group->surfaces, count = group->surfaceCount; count; --count, ++surf )
    R_AddWorldSurfaceNoCull(dlightBits, surf);
}

/* ---- R_AddStaticModelDPVS  0x004E42B0 ----  VERIFIED
 * Cull a static model's bounds; if visible, stamp it and add it to the scene.
 * The caller also passes its dlight bits, which retail never reads. */
void __cdecl R_AddStaticModelDPVS(int planes, int planeCount, int modelPtr, int dlightBits)
{
  dpvsStaticModel_t *model = (dpvsStaticModel_t *)modelPtr;

  if ( R_CullBoxDPVS((const vec3_t *)&model->mins, (dpvsPlane_t *)planes, planeCount, 0) )
    return;

  if ( r_showCullSModels->integer )
    R_AddDebugBox( model->mins, model->maxs,staticModelDebugColor);

  model->viewCount = tr_viewCount;
  RE_AddRefEntityToScene(&model->e, model);
}

/* ---- R_AddCellCullGroups  0x004E4440 ----  VERIFIED
 * Submit the cell's cull groups and (when enabled) its linked static models,
 * each stamped so it is added at most once per view.  Retail counts the
 * cull groups down to zero (dec/jne at 0x4E4480). */
void __cdecl R_AddCellCullGroups(int cellPtr, int planes, int planeCount, int dlightBits)
{
  dpvsCell_t          *cell = (dpvsCell_t *)cellPtr;
  dpvsCullGroup_t    **groups;
  dpvsCellModelLink_t *link;
  int                  count;

  for ( count = cell->cullGroupCount, groups = cell->cullGroups; count; --count, ++groups )
  {
    if ( (*groups)->viewCount != tr_viewCount )
      R_AddCullGroupDPVS(*groups, (dpvsPlane_t *)planes, planeCount, dlightBits);
  }

  if ( r_drawSModels->integer )
  {
    for ( link = cell->modelLinks; link; link = link->next )
    {
      dpvsStaticModel_t *model = (dpvsStaticModel_t *)link->model;
      if ( model->viewCount != tr_viewCount )
        R_AddStaticModelDPVS(planes, planeCount, (int)model, dlightBits);
    }
  }
}

/* ---- R_RecursivePortalWalk  0x004E44D0 ----  VERIFIED
 * Install this cell's occluders and submit its geometry, then flood through
 * each outgoing portal that faces the camera, is not clipped away, and is not
 * occluded.  The per-portal recursion flag prevents cycles. */
void __cdecl R_RecursivePortalWalk(int cellPtr, int sourcePortal, float *planes, int planeCount, int dlightBits)
{
  dpvsCell_t   *cell = (dpvsCell_t *)cellPtr;
  dpvsPortal_t *portal;
  int           portalCount;
  float        *clippedPlanes;
  int           clippedCount;
  int           occluded;
  int           o;

  R_AddCellOccluders(cellPtr, planes, planeCount);
  R_AddCellSurfaces(cellPtr, (int)planes, planeCount, dlightBits);
  R_AddCellCullGroups(cellPtr, (int)planes, planeCount, dlightBits);

  portal = cell->portals;
  for ( portalCount = cell->portalCount; portalCount; --portalCount, ++portal )
  {
    if ( portal->recursionActive )
      continue;

    /* retail compares the unrounded x87 sum (0x4E452B-0x4E4549) */
    if ( *(float *)&dpvs_viewOrigin*portal->plane.normal[0]
       + ( *(float *)&dpvs_viewOriginY*portal->plane.normal[1]
         + *(float *)&dpvs_viewOriginZ*portal->plane.normal[2] ) > portal->plane.distance )
      continue;

    if ( R_PortalBehindAnyPlane((int)portal, planes, planeCount) )
      continue;

    occluded = 0;
    for ( o = 0; o < dpvs_occluderCount; ++o )
    {
      dpvsOccluder_t *occ = ((dpvsOccluder_t **)dpvs_occluders)[o];
      if ( R_PortalBehindAllPlanes((int)portal, (float *)occ->activePlanes, occ->activePlaneCount) )
      {
        occluded = 1;
        break;
      }
    }
    if ( occluded )
      continue;

    clippedPlanes = (float *)R_PortalClipPlanes(planeCount, &clippedCount, portal, sourcePortal, (char *)planes);
    if ( clippedCount )
    {
      portal->recursionActive = 1;
      R_RecursivePortalWalk((int)portal->cell, (int)portal, clippedPlanes, clippedCount, dlightBits);
      ri.Hunk_FreeTempMemory((int)clippedPlanes);
      portal->recursionActive = 0;
    }
  }
}

/* ---- R_SetupDPVS  0x004E4610 ----  VERIFIED
 * Rebuild the DPVS planes for the current view (unless the PVS is locked after
 * the first setup).  The near plane sits 0.1 units behind the view origin, or
 * is the portal plane for mirror views; a positive far distance adds the
 * opposing far plane. */
void R_SetupDPVS()
{
  rfloat farDist;

  if ( !dpvs_initialSetup && r_lockpvs->integer )
    return;

  dpvs_initialSetup = 0;
  R_FrustumClipPlanes();

  *(float *)&dpvs_viewOrigin = tr_viewParms_originX;
  *(float *)&dpvs_viewOriginY = tr_viewParms_originY;
  *(float *)&dpvs_viewOriginZ = tr_viewParms_originZ;

  dpvs_nearPlaneNormalZ = tr_viewParms_axis02;
  dpvs_nearPlane = tr_viewParms_axis00;
  dpvs_nearPlaneNormalY = tr_viewParms_axis01;
#ifdef COD1_VC7
  dpvs_nearPlaneDistance = dpvs_nearPlaneRec.normal[2] * tr_viewParms_originZ
              + dpvs_nearPlaneRec.normal[1] * tr_viewParms_originY
              + dpvs_nearPlaneRec.normal[0] * tr_viewParms_originX
              - 0.1f;
  R_SetPlaneSidesDPVS((int)&dpvs_nearPlaneRec);
#else
  dpvs_nearPlaneSideOffsetX = tr_viewParms_axis00 <= 0 ? 0 : 0xC;
  dpvs_nearPlaneSideOffsetY = tr_viewParms_axis01 <= 0 ? 4 : 16;
  dpvs_nearPlaneSideOffsetZ = tr_viewParms_axis02 <= 0 ? 8 : 20;
  dpvs_nearPlaneDistance = *(float *)&tr_viewParms_axis02 * tr_viewParms_originZ
              + *(float *)&tr_viewParms_axis01 * tr_viewParms_originY
              + *(float *)&tr_viewParms_axis00 * tr_viewParms_originX
              - (double)0.1f              /* retail fsub flt_568FAC (float 0.1) */
              - (double)DPVS_PLANE_BIAS;  /* retail fsub flt_568E6C (float 0.001) */
#endif

  dpvs_activeNearPlane = (int)&tr_viewParms_portalPlane;
  if ( !tr_viewParms_isMirror )
    dpvs_activeNearPlane = (int)&dpvs_nearPlane;

  /* retail: fcom 0.0f / test ah,41h / jne no-far-plane (0x4E470F-0x4E471A),
   * so an unordered distance also leaves the far plane off */
  farDist = RE_GetFarPlaneDist();
  if ( farDist > 0.0f )
  {
    dpvs_activeFarPlane = (int)&dpvs_farPlane;
    dpvs_farPlane = *(float *)&tr_viewParms_axis00 * -1.0f;
    dpvs_farPlaneNormalY = *(float *)&tr_viewParms_axis01 * -1.0f;
    dpvs_farPlaneNormalZ = *(float *)&tr_viewParms_axis02 * -1.0f;
#ifdef COD1_VC7
    dpvs_farPlaneDistance = dpvs_farPlaneNormalZ * tr_viewParms_originZ
                + dpvs_farPlaneNormalY * tr_viewParms_originY
                + dpvs_farPlane * tr_viewParms_originX
                - farDist;
    R_SetPlaneSidesDPVS((int)&dpvs_farPlaneRec);
#else
    dpvs_farPlaneSideOffsetX = *(int *)&dpvs_farPlane <= 0 ? 0 : 0xC;
    dpvs_farPlaneSideOffsetY = *(int *)&dpvs_farPlaneNormalY <= 0 ? 4 : 16;
    dpvs_farPlaneSideOffsetZ = *(int *)&dpvs_farPlaneNormalZ <= 0 ? 8 : 20;
    dpvs_farPlaneDistance = dpvs_farPlaneNormalZ * tr_viewParms_originZ
                + dpvs_farPlaneNormalY * tr_viewParms_originY
                + dpvs_farPlane * tr_viewParms_originX
                - farDist
                - (double)DPVS_PLANE_BIAS;  /* retail fsub flt_568E6C (float 0.001) */
#endif
  }
  else
  {
    dpvs_activeFarPlane = 0;
  }

  if ( r_portalbevels->value > 0.0f )
    R_SetupTransformMatrix();
}

/* sprite refEntity built for each corona (overlays refEntity_t's first 0xa0
 * bytes; only the fields the corona sets are named). */
typedef struct {
  int   reType;              /* +0x00  RT_SPRITE */
  int   renderfx;            /* +0x04 */
  int   hModel;              /* +0x08 */
  float lightingOrigin[3];   /* +0x0c */
  float shadowPlane;         /* +0x18 */
  float axis[3][3];          /* +0x1c */
  int   nonNormalizedAxes;   /* +0x40 */
  float origin[3];           /* +0x44 */
  int   frame;               /* +0x50 */
  float oldorigin[3];        /* +0x54 */
  int   oldframe;            /* +0x60 */
  float radius2;             /* +0x64  sprite vertical radius = scale */
  int   customShader;        /* +0x68  corona shader index */
  byte  shaderRGBA[4];       /* +0x6c */
  float shaderTexCoord[2];   /* +0x70 */
  float shaderTime;          /* +0x78 */
  float radius;              /* +0x7c  sprite radius = scale */
  float rotation;            /* +0x80 */
  float fireRiseDir[3];      /* +0x84 */
  void *dobj;                /* +0x90 */
  void *owner;               /* +0x94 */
  void *staticModel;         /* +0x98 */
} dpvsCoronaEntity_t;
ASSERT_OFF(dpvsCoronaEntity_t, origin, 0x44);
ASSERT_OFF(dpvsCoronaEntity_t, radius2, 0x64);
ASSERT_OFF(dpvsCoronaEntity_t, customShader, 0x68);
ASSERT_OFF(dpvsCoronaEntity_t, shaderRGBA, 0x6c);
ASSERT_OFF(dpvsCoronaEntity_t, radius, 0x7c);
COD1_ASSERT_SIZE(dpvsCoronaEntity_t, 0x9c);

/* ---- R_AddCoronas  0x004E4800 ----  [HIGH]
 * Turn each authored corona into a camera-facing sprite (shifted 16 units
 * toward the camera) whose conservative sqrt(3) cube passes the DPVS box test;
 * the optional vertical fade is applied to its alpha.  Every field is read
 * through tr.world->coronas[i] (CORONA), with no cached element pointer:
 * retail recomputes the element address after the inlined box cull. */
#define CORONA ((*(dpvsCorona_t **)(tr_world + 192))[i])
void __cdecl R_AddCoronas(int planesPtr, int planeCount)
{
  dpvsPlane_t       *planes = (dpvsPlane_t *)planesPtr;
  dpvsCoronaEntity_t entity;
  int                i;

  memset(&entity, 0, 0x9c);
  entity.reType = 4;

  for ( i = 0; i < (*(int *)(tr_world + 196)); ++i )
  {
    float         viewDir[3];
    float         bounds[6];

    viewDir[0] = (*(float *)(((char *)(void *)&tr_registered) + 0x0910)) - CORONA.origin[0];
    viewDir[1] = (*(float *)(((char *)(void *)&tr_registered) + 0x0914)) - CORONA.origin[1];
    viewDir[2] = (*(float *)(((char *)(void *)&tr_registered) + 0x0918)) - CORONA.origin[2];
    VectorNormalize(viewDir);

    if ( !(viewDir[2] >= CORONA.zCutoff) )
    {
      float scaledRadius = CORONA.scale * 1.7320508f;

      bounds[0] = CORONA.origin[0] - scaledRadius;
      bounds[1] = CORONA.origin[1] - scaledRadius;
      bounds[2] = CORONA.origin[2] - scaledRadius;
      bounds[3] = CORONA.origin[0] + scaledRadius;
      bounds[4] = CORONA.origin[1] + scaledRadius;
      bounds[5] = CORONA.origin[2] + scaledRadius;

      if ( !R_CullBoxDPVS((const vec3_t *)bounds, planes, planeCount, 0) )
      {
        entity.customShader = CORONA.shader->index;
        entity.origin[0] = viewDir[0]*16.0f + CORONA.origin[0];
        entity.origin[1] = viewDir[1]*16.0f + CORONA.origin[1];
        entity.origin[2] = viewDir[2]*16.0f + CORONA.origin[2];
        entity.shaderRGBA[0] = CORONA.color[0];
        entity.shaderRGBA[1] = CORONA.color[1];
        entity.shaderRGBA[2] = CORONA.color[2];
        entity.radius2 = entity.radius = CORONA.scale;

        if ( viewDir[2] > CORONA.zFadeOut )
          entity.shaderRGBA[3] = (byte)(int)(CORONA.color[3] * (CORONA.zCutoff - viewDir[2]) / (CORONA.zCutoff - CORONA.zFadeOut));
        else
          entity.shaderRGBA[3] = CORONA.color[3];

        RE_AddRefEntityToScene(&entity, 0);
      }
    }
  }
}
#undef CORONA

/* ---- R_AddSkySurfacesDPVS  0x004E4B70 ----  [HIGH]
 * Select sky surfaces behind the far plane, disable the far plane, and submit
 * them so their ordinary DPVS test cannot reject them for distance. */
void R_AddSkySurfacesDPVS()
{
  dpvsPlane_t *farPlane = (dpvsPlane_t *)dpvs_activeFarPlane;
  int          i;

  if ( farPlane )
  {
    dpvs_activeFarPlane = 0;
    for ( i = 0; i < DPVS_WORLD_SKYSURF_COUNT; ++i )
    {
      msurface_t *surf = DPVS_WORLD_SKYSURFS[i];
      if ( surf->viewCount != tr_viewCount )
      {
        srfLit_t *data = (srfLit_t *)surf->data;
        if ( R_BoxBehindPlane((const vec3_t *)&data->boundsMin, farPlane) )
          R_AddTrianglesSurface(4, surf, 0, dpvs_frustumPlanesTyped, 0);
      }
    }
  }
}

/* ---- R_AddWorldSurfacesDPVS  0x004E4C10 ----  VERIFIED
 * DPVS entry point.  Points the occluder / active-plane / entity-link banks at
 * frame-local scratch, filters entities and coronas into cells, then either
 * portal-walks from the camera cell or, when the camera is outside the BSP,
 * submits every cell by the two outsideMapEnts modes. */
void R_AddWorldSurfacesDPVS()
{
  dpvsOccluder_t *occluderBank[1024];       
  dpvsPlane_t activePlaneBank[6144];   
  dpvsCellEntityLink_t entityLinkBank[4096];    

  if ( !r_drawworld->integer )
    return;
  if ( ((*(int *)(((char *)(void *)&tr_registered) + 0x0944)) & 1) != 0 )   
    return;

  dpvsGlob.occluders = (int)&occluderBank;
  dpvs_activePlanes = (int)&activePlaneBank;
  dpvsGlob.cellEntityLinks = (int)&entityLinkBank;
  dpvsGlob.occluderCount = 0;
  tr_currentEntityNumber = 1022;
  tr_shiftedEntityNumber = 1022 << 8;

  {
    int dlightBits = (1 << (*(int *)(((char *)(void *)&tr_registered) + 0x0A54))) - 1;
    int cameraCell;
    int i;

    R_SetupDPVS();
    R_FilterModelsIntoCells((int)dpvs_frustumPlanes, 4);
    R_AddCoronas((int)dpvs_frustumPlanes, 4);

    cameraCell = R_CellForCamera();
    if ( cameraCell >= 0 )
    {
      dpvsCell_t *cell = &(*(dpvsCell_t **)(tr_world + 292))[cameraCell];
      if ( r_singlecell->integer )
      {
        dpvs_activeFarPlane = 0;
        R_AddCellSurfaces((int)cell, (int)dpvs_frustumPlanes, 4, dlightBits);
        R_AddCellCullGroups((int)cell, (int)dpvs_frustumPlanes, 4, dlightBits);
      }
      else
      {
        R_RecursivePortalWalk((int)cell, (int)&(*(int *)&dpvs_nearPlaneRec.normal[0]), (float *)dpvs_frustumPlanes, 4, dlightBits);
      }
    }
    else
    {
      if ( outsideMapEnts->integer )
      {
        for ( i = 0; i < (*(int *)(tr_world + 288)); ++i )
        {
          R_AddCellSurfaces((int)&(*(dpvsCell_t **)(tr_world + 292))[i], (int)dpvs_frustumPlanes, 4, dlightBits);
          R_AddCellCullGroups((int)&(*(dpvsCell_t **)(tr_world + 292))[i], (int)dpvs_frustumPlanes, 4, dlightBits);
        }
      }
      else
      {
        for ( i = 0; i < (*(int *)(tr_world + 288)); ++i )
          R_AddCellSurfaces((int)&(*(dpvsCell_t **)(tr_world + 292))[i], (int)dpvs_frustumPlanes, 4, dlightBits);
        for ( i = 0; i < (*(int *)(tr_world + 324)); ++i )
          R_AddCullGroupDPVS(&(*(dpvsCullGroup_t **)(tr_world + 328))[i], ((dpvsPlane_t *)(void *)dpvs_frustumPlanes), 4, dlightBits);
      }
    }

    R_AddSkySurfacesDPVS();

    if ( r_showCullXModels->integer == 2
      || r_showCullBModels->integer == 2 )
    {
      for ( i = 0; i < (*(int *)(tr_world + 288)); ++i )
      {
        dpvsCellEntityLink_t *link;
        for ( link = (*(dpvsCell_t **)(tr_world + 292))[i].entityLinks; link; link = link->next )
        {
          if ( (link->entity->e.reType != 1 || r_showCullXModels->integer == 2)
            && (link->entity->e.reType != 0 || r_showCullBModels->integer == 2) )
            R_AddDebugBox( link->mins, link->maxs, link->entity->cullState == CULL_IN ? colorDebugGreen : colorDebugRed );
        }
      }
    }
  }
}

/* ---- RE_SetFarPlaneDist  0x004E4EB0 ----  VERIFIED */
void __cdecl RE_SetFarPlaneDist(float dist)
{
  if ( dist <= 0.0f )
    dpvs_cullDist = 0.0f;
  else
    dpvs_cullDist = dist;
}


int dpvs_initialSetup = 1;
