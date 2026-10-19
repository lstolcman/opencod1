/*
 * cm_local.h -- collision-system record layouts for Call of Duty 1.1 (Windows).
 *
 * The retail tree had a qcommon/cm_local.h; this is it, rebuilt from
 * CoDMP.exe.  Only the cm_*.c units include it.
 *
 * As in RTCW, this header pulls in the patch-collide records
 * (cm_patch_local.h, RTCW's cm_patch.h); the work records the server's
 * sector walkers share with sv_world_mp.c are in cm_public.h (via qcommon.h).
 * Every size asserted below is the one the retail code's array strides and
 * stack frames prove.
 *
 * Every struct is corroborated by opencoduo (coduo-server-master/
 * dedicated-engine/src/physics_collision/): CoD1 and CoD:UO load each other's
 * .bsp files, so the on-disk and in-memory collision records are necessarily
 * the same shape.
 *
 * @fidelity: verified
 */

#ifndef __CM_LOCAL_H__
#define __CM_LOCAL_H__

#include "qcommon.h"
#include "cm_patch_local.h"

/*
=============================================================================

						ON-DISK BSP RECORDS

	CoD's lump_t is { int filelen; int fileofs; } -- REVERSED from Q3/RTCW.
	It is declared that way in qcommon.h; do not "fix" it.  Proof:
	CM_LoadLump_m (0x0041A180) reads the 272-byte header and then seeks
	FS_Seek(f, lumps[2i+1] - 272, FS_SEEK_CUR), i.e. the SECOND dword is the
	file offset, while the FIRST is used as both the allocation size and the
	read count.  CMod_LoadBrushes (0x00419300) does the same thing inline:
	`mov ebp,[esi+4]` produces the base pointer and `test byte ptr [esi],3`
	validates the length.

=============================================================================
*/

/* The BSP header: ident, version, then 33 lumps.  272 bytes total, which is
 * the size CM_LoadLump_m reads and CM_SaveLump writes (8 + 33*8). */
#define CM_LUMPS            33
#define CM_HEADER_SIZE      272
#define CM_BSP_VERSION      59      /* CM_LoadMap: `cmp eax, 3Bh` */

typedef struct dheader_t
{
	int ident;
	int version;
	lump_t lumps[CM_LUMPS];
} dheader_t;
COD1_ASSERT_SIZE( dheader_t, CM_HEADER_SIZE );

/* Lump indices, read off CM_LoadMap's `lea` operands (0x00419DEC onward).
 * The header base is esp+0x50 and lump i sits at esp+0x58 + 8*i. */
enum
{
	LUMP_MATERIALS      = 0,    /* CMod_LoadShaders                 */
	LUMP_PLANES         = 2,    /* CMod_LoadPlanes                  */
	LUMP_BRUSHSIDES     = 3,    /* CMod_LoadBrushes, second arg     */
	LUMP_BRUSHES        = 4,    /* CMod_LoadBrushes, first  arg     */
	LUMP_COLLISIONVERTS = 25,   /* CMod_LoadLeafCurvesAndTerrain    */
	LUMP_COLLISIONINDEX = 26,
	LUMP_LEAFSURFACES_D = 24,   /* the 16-byte curve/terrain records */
	LUMP_NODES          = 20,
	LUMP_LEAFS          = 21,
	LUMP_LEAFBRUSHES    = 22,
	LUMP_LEAFSURFACES   = 23,
	LUMP_MODELS         = 27,
	LUMP_VISIBILITY     = 28,
	LUMP_ENTITIES       = 29
};

/* 72 bytes: CMod_LoadShaders multiplies the count by 72 and the content flags
 * are read at +68 by CMod_LoadBrushes (`+ 72 * v16 + 68`) and by
 * CMod_LoadLeafCurvesAndTerrain. */
typedef struct dmaterial_t
{
	char material[64];
	int surfaceFlags;
	int contentFlags;
} dmaterial_t;
COD1_ASSERT_SIZE( dmaterial_t, 72 );

/* 16 bytes per record; CMod_LoadLeafCurvesAndTerrain validates the lump with
 * `test al, 0Fh` and derives the count with `shr edi, 4`.
 *
 * The same two 16-bit fields mean different things in the two branches, which
 * is why they are named neutrally here:
 *   isTerrain == 0  -- a curved patch:  a = width, b = height, and the vertex
 *                      run starts at collisionVerts[secondIndex].
 *   isTerrain != 0  -- a terrain patch: a = vertex count, b = index count,
 *                      vertices at collisionVerts[firstIndex] and indices at
 *                      collisionIndexes[secondIndex].
 */
typedef struct dcollisionsurface_t
{
	short materialIndex;
	byte isTerrain;
	byte pad;
	short a;
	short b;
	int firstIndex;
	int secondIndex;
} dcollisionsurface_t;
COD1_ASSERT_SIZE( dcollisionsurface_t, 16 );

/*
=============================================================================

						IN-MEMORY COLLISION RECORDS

=============================================================================
*/

/*
 * 8 bytes.  CMod_LoadBrushes stores a PLANE POINTER, not an index:
 *   `*v18 = cm_planes + 20 * planeNum;  v18[1] = materialNum;  v18 += 2;`
 * opencoduo's coduo_collision_brush_side_t agrees (plane +0x00, materialIndex
 * +0x04, size 8).
 */
typedef struct cbrushside_t
{
	cplane_t        *plane;
	int materialNum;
} cbrushside_t;
COD1_ASSERT_SIZE( cbrushside_t, 8 );

/*
 * 52 bytes.  CONFIRMED at two independent sites.
 *
 * CM_InitBoxHull (0x0041A390) is the cleaner of the two and pins everything:
 *   imul eax, 34h            -- stride 52
 *   mov [eax+1Ch], 0         -- numSides
 *   mov [eax+20h], 0         -- sides
 *   mov [ecx], -1            -- contents
 *   mov [edx+28h], ax  ... [ecx+32h], ax   -- six 16-bit axial material
 *                                             indices at +0x28..+0x33
 *
 * CMod_LoadBrushes writes the six axial plane distances as mins[0..2] then
 * maxs[0..2] -- the inner loop steps 12 bytes (one axis' min then max) and the
 * outer loop steps 4 (to the next axis), so the store order is
 * mins.x, maxs.x, mins.y, maxs.y, mins.z, maxs.z into two separate vec3_t.
 * That fixes mins at +0x04 and maxs at +0x10.
 *
 * The six axialMaterialNum entries are written in the same interleaving, so
 * entries 0..2 belong to the mins planes and 3..5 to the maxs planes.
 *
 * Identical to opencoduo's coduo_collision_brush_t.
 */
typedef struct cbrush_t
{
	int contents;                   /* +0x00 */
	vec3_t mins;                    /* +0x04 */
	vec3_t maxs;                    /* +0x10 */
	int numSides;                   /* +0x1C  non-axial sides only */
	cbrushside_t    *sides;         /* +0x20 */
	int checkcount;                 /* +0x24  duplicate-suppression generation */
	short axialMaterialNum[6];      /* +0x28  [0..2] mins planes, [3..5] maxs */
} cbrush_t;
COD1_ASSERT_SIZE( cbrush_t, 52 );

/*
 * 16 bytes.  CMod_LoadLeafs allocates `16 * count` and writes every field
 * below; the "surface" fields index cm_leafsurfaces, which in turn indexes
 * cm_patches.  The field names are retail's own -- they come out of that
 * function's Com_Error strings ("CMod_LoadLeafs: numLeafSurfaces exceeded"
 * and friends), which makes them authoritative even though opencoduo calls
 * the same two fields firstLeafTerrainPatch/numLeafTerrainPatches.
 *
 * cellnum at +0x0E is CoD-specific; opencoduo does not assert it, but
 * CMod_LoadLeafs writes it from source dword 6 and range-checks it.
 */
typedef struct cLeaf_t
{
	short cluster;                  /* +0x00 */
	short area;                     /* +0x02 */
	int firstLeafBrush;             /* +0x04 */
	unsigned short numLeafBrushes;  /* +0x08 */
	unsigned short firstLeafSurface;/* +0x0A */
	unsigned short numLeafSurfaces; /* +0x0C */
	short cellnum;                  /* +0x0E */
} cLeaf_t;
COD1_ASSERT_SIZE( cLeaf_t, 16 );

/*
 * 40 bytes = 12 + 12 + sizeof(cLeaf_t).  CONFIRMED three ways:
 *
 *  - CM_ClipHandleToModel indexes cm_cmodels with `40 * handle`.
 *  - CM_ModelBounds (0x0041A4D0) copies [eax+0x00..0x08] to mins and
 *    [eax+0x0C..0x14] to maxs.
 *  - The box model is a static instance at 0x01617580 and CM_InitBoxHull
 *    writes its leaf through two separate globals: firstLeafBrush lands at
 *    0x0161759C (= +0x1C = leaf+0x04) and numLeafBrushes at 0x016175A0
 *    (= +0x20 = leaf+0x08).  sv_worldSectors begins at 0x016175A8, i.e.
 *    exactly 40 bytes after the model, so there is no trailing padding.
 */
typedef struct cmodel_t
{
	vec3_t mins;                    /* +0x00 */
	vec3_t maxs;                    /* +0x0C */
	cLeaf_t leaf;                   /* +0x18  submodels are a single leaf */
} cmodel_t;
COD1_ASSERT_SIZE( cmodel_t, 40 );

/*
 * 8 bytes.  CMod_LoadNodes allocates `8 * count`, stores a plane POINTER
 * (cm_planes + 20 * planeNum) at +0x00 and two signed 16-bit children at
 * +0x04/+0x06.  Negative children are -(leafnum + 1), as in Q3.
 */
typedef struct cNode_t
{
	cplane_t        *plane;
	short children[2];
} cNode_t;
COD1_ASSERT_SIZE( cNode_t, 8 );

/*
 * 8 bytes; CMod_LoadLeafs allocates `8 * cm_numAreas`.  CM_FloodArea_r reads
 * [area*8 + 0] as the flood number and [area*8 + 4] as the flood generation.
 */
typedef struct cArea_t
{
	int floodnum;
	int floodvalid;
} cArea_t;
COD1_ASSERT_SIZE( cArea_t, 8 );

/*
 * 44 bytes.  CMod_LoadLeafCurvesAndTerrain allocates `44 * count` and writes
 * checkcount at +0x00, materialNum at +0x04, the material's contentFlags at
 * +0x08, and the two collide pointers at +0x24 and +0x28 -- which leaves
 * exactly the 24 bytes at +0x0C..+0x23 for the bounds.  opencoduo's
 * coduo_collision_terrain_patch_t has the identical layout.
 *
 * Exactly one of pc/tc is non-NULL; the other branch stores 0 explicitly.
 */
typedef struct cPatch_t
{
	int checkcount;                 /* +0x00 */
	int materialNum;                /* +0x04 */
	int contents;                   /* +0x08 */
	vec3_t mins;                    /* +0x0C */
	vec3_t maxs;                    /* +0x18 */
	patchCollide_t  *pc;            /* +0x24  curved patch, or NULL */
	void            *tc;            /* +0x28  terrain, or NULL     */
} cPatch_t;
COD1_ASSERT_SIZE( cPatch_t, 44 );

/*
 * 48 bytes.  CONFIRMED field by field from CM_BoxLeafnums (0x00421E10), which
 * builds one on its own stack frame at esp+0:
 *   [+0x00] = 0                       count
 *   [+0x04] = listsize                maxcount
 *   [+0x08] = 0                       overflowed
 *   [+0x0C] = list                    list
 *   [+0x10..+0x18] = mins             bounds[0]
 *   [+0x1C..+0x24] = maxs             bounds[1]
 *   [+0x28] = 0                       lastLeaf   (returned through *lastLeaf)
 *   [+0x2C] = offset CM_StoreLeafs    storeLeafs
 * That is Q3's leafList_t unchanged, which is what the 48-byte size already
 * hinted at -- this is one of the structures CoD inherited untouched.
 */
typedef struct leafList_t
{
	int count;                      /* +0x00 */
	int maxcount;                   /* +0x04 */
	qboolean overflowed;            /* +0x08 */
	int             *list;          /* +0x0C */
	vec3_t bounds[2];               /* +0x10 */
	int lastLeaf;                   /* +0x28 */
	void ( *storeLeafs )( struct leafList_t *ll, int nodenum );  /* +0x2C */
} leafList_t;
COD1_ASSERT_SIZE( leafList_t, 48 );

/*
 * 24 bytes.  The capsule/sphere used by the capsule trace paths.  opencoduo
 * asserts use +0x00, radius +0x04, halfheight +0x08, offset +0x0C, size 0x18,
 * and traceWork_t's own 280-byte total only closes with this shape.
 */
typedef struct sphere_t
{
	qboolean use;
	float radius;
	float halfheight;
	vec3_t offset;
} sphere_t;
COD1_ASSERT_SIZE( sphere_t, 24 );

/*
 * 280 bytes (0x118), the size opencoduo asserts its traceWork_t at,
 * offset-by-offset; the layout is taken as-is.
 *
 * maxsSum at +0xA0 is written by both CM_Trace and CM_SightTrace and never
 * read by anything -- opencoduo notes the same dead store.
 */
typedef struct traceWork_t
{
	vec3_t start;                   /* +0x000 */
	vec3_t end;                     /* +0x00C */
	vec3_t delta;                   /* +0x018 */
	float deltaLengthSquared;       /* +0x024 */
	vec3_t mins;                    /* +0x028 */
	vec3_t maxs;                    /* +0x034 */
	vec3_t offsets[8];              /* +0x040 */
	float maxsSum;                  /* +0x0A0  written, never read */
	vec3_t bounds[2];               /* +0x0A4 */
	int contents;                   /* +0x0BC */
	qboolean isPoint;               /* +0x0C0 */
	trace_t trace;                  /* +0x0C4 */
	sphere_t sphere;                /* +0x0F4 */
	vec3_t sphereExtents;           /* +0x10C */
} traceWork_t;
COD1_ASSERT_SIZE( traceWork_t, 280 );

/*
 * There is no TraceExtents struct.  The first stack argument of CM_TraceBox
 * (0x004272E0) is a plain vec3_t: the function's second pass does
 * `mov ecx, ebp` at 0x004273C0 to swap the bounds pointer from the register
 * argument to the stack one, and both are then read only as three consecutive
 * floats.  CM_TraceBox is opencoduo's CM_TraceLineSkipsBox with three of its
 * five arguments in registers.
 */

/*
=============================================================================

						COLLISION GLOBALS

	Retail kept all of these in one `clipMap_t cm` at 0x016174A0 -- CM_LoadMap
	clears the lot with a single Com_Memset(&cm, 0, 0xA15C), and 0x016174A0
	+ 0xA15C lands exactly on cmod_base.  The world sectors cm_world.c walks
	are its tail.  CM_ClearMap clears the same fields one by one (see
	CM_ClearWorldSectors), which is behaviourally identical.

	Addresses are the retail ones.

=============================================================================
*/

#define MAX_SUBMODELS       512     /* CMod_LoadSubmodels; also the two box
                                     * handles 511 and 510 */
#define CM_MAX_WORLD_SECTORS    1024

typedef struct worldSector_t
{
	int axis;                               /* +0x00  0 = x, 1 = y     */
	int staticModelContents;                /* +0x04 */
	int sightStaticModelContents;           /* +0x08  big models only  */
	int entityContents;                     /* +0x0C */
	float dist;                             /* +0x10  split plane      */
	struct cmEntityLink_t   *entities;      /* +0x14 */
	struct cmStaticModelLink_t *staticModels; /* +0x18 */
	struct worldSector_t    *parent;        /* +0x1C  also the free list link */
	struct worldSector_t    *children[2];   /* +0x20  [0] above, [1] below */
} worldSector_t;
COD1_ASSERT_SIZE( worldSector_t, 40 );

#define BOX_MODEL_HANDLE    511
#define CAPSULE_MODEL_HANDLE 510

/*
 * WHERE THE STORAGE ACTUALLY LIVES
 * --------------------------------
 * One clipMap_t object, `cm` (cm_load.c).  Retail addresses it as a whole, so a
 * store through any of its arrays may alias the counters and pointers; the
 * per-field names the collision code uses are macros onto the members.
 *
 * NOTE the patch plane pool at 0x0087C7DC/0x0087C7E0 (cm_patch.c's
 * cm_numPatchPlanes / cm_patchPlanes) is NOT cm.planes (0x016174FC).
 */
typedef struct clipMap_s {
	char                    name[MAX_QPATH];    /* +0x00 0x016174A0 */
	int                     numStaticModels;    /* +0x40 */
	struct cmStaticModel_t *staticModels;       /* +0x44 cm_staticmodel.c */
	int                     numMaterials;       /* +0x48 0x016174E8 */
	dmaterial_t            *materials;          /* +0x4C */
	int                     numBrushSides;      /* +0x50 */
	cbrushside_t           *brushsides;         /* +0x54 */
	int                     numPlanes;          /* +0x58 */
	cplane_t               *planes;             /* +0x5C 0x016174FC */
	int                     numNodes;           /* +0x60 */
	cNode_t                *nodes;              /* +0x64 */
	int                     numLeafs;           /* +0x68 */
	cLeaf_t                *leafs;              /* +0x6C */
	int                     numLeafBrushes;     /* +0x70 */
	int                    *leafbrushes;        /* +0x74 */
	int                     numLeafSurfaces;    /* +0x78 */
	int                    *leafsurfaces;       /* +0x7C */
	int                     numSubModels;       /* +0x80 0x01617520 */
	cmodel_t               *cmodels;            /* +0x84 */
	int                     numBrushes;         /* +0x88 */
	cbrush_t               *brushes;            /* +0x8C */
	int                     numClusters;        /* +0x90 */
	int                     clusterBytes;       /* +0x94 */
	byte                   *visibility;         /* +0x98 0x01617538 */
	qboolean                vised;              /* +0x9C */
	int                     numEntityChars;     /* +0xA0 */
	char                   *entityString;       /* +0xA4 */
	int                     numAreas;           /* +0xA8 */
	cArea_t                *areas;              /* +0xAC */
	int                    *areaPortals;        /* +0xB0 */
	int                     numPatches;         /* +0xB4 */
	cPatch_t               *patches;            /* +0xB8 */
	int                     floodvalid;         /* +0xBC */
	int                     checkcount;         /* +0xC0 0x01617560 */
	float                   worldMins[3];       /* +0xC4 sv_worldMins (cm_world.c) */
	float                   worldMaxs[3];       /* +0xD0 */
	cbrush_t               *box_brush;          /* +0xDC 0x0161757C */
	cmodel_t                box_model;          /* +0xE0 0x01617580 */
	worldSector_t           sectors;            /* +0x108 0x016175A8 the ROOT, not an array */
	worldSector_t          *freeSectors;        /* +0x130 0x016175D0 free-sector list head */
	worldSector_t           listHeadNull;       /* +0x134 0x016175D4 the "no child" sentinel */
	worldSector_t           pool[CM_MAX_WORLD_SECTORS]; /* +0x15C 0x016175FC */
} clipMap_t;
COD1_ASSERT_SIZE( clipMap_t, 0xA15C );
COD1_STATIC_ASSERT( offsetof( clipMap_t, sectors ) == 0x108, "cm.sectors offset" );
COD1_STATIC_ASSERT( offsetof( clipMap_t, pool ) == 0x15C, "cm.pool offset" );
COD1_STATIC_ASSERT( offsetof( clipMap_t, numMaterials ) == 0x48, "cm.numMaterials offset" );
COD1_STATIC_ASSERT( offsetof( clipMap_t, checkcount ) == 0xC0, "cm.checkcount offset" );
COD1_STATIC_ASSERT( offsetof( clipMap_t, box_model ) == 0xE0, "cm.box_model offset" );

extern clipMap_t        cm;             /* 0x016174A0 */
extern byte             *cmod_base;     /* 0x016215FC */
extern cvar_t           *cm_noCurves;   /* 0x01617480 */
extern cvar_t           *cm_playerCurveClip; /* 0x01621600 */

#define cm_mapName          cm.name
#define cm_numStaticModels  cm.numStaticModels
#define cm_staticModels     cm.staticModels
#define cm_numMaterials     cm.numMaterials
#define cm_materials        cm.materials
#define cm_numBrushSides    cm.numBrushSides
#define cm_brushsides       cm.brushsides
#define cm_numPlanes        cm.numPlanes
#define cm_planes           cm.planes
#define cm_numNodes         cm.numNodes
#define cm_nodes            cm.nodes
#define cm_numLeafs         cm.numLeafs
#define cm_leafs            cm.leafs
#define cm_numLeafBrushes   cm.numLeafBrushes
#define cm_leafbrushes      cm.leafbrushes
#define cm_numLeafSurfaces  cm.numLeafSurfaces
#define cm_leafsurfaces     cm.leafsurfaces
#define cm_numSubModels     cm.numSubModels
#define cm_cmodels          cm.cmodels
#define cm_numBrushes       cm.numBrushes
#define cm_brushes          cm.brushes
#define cm_numClusters      cm.numClusters
#define cm_clusterBytes     cm.clusterBytes
#define cm_visibility       cm.visibility
#define cm_vised            cm.vised
#define cm_entityStringLen  cm.numEntityChars
#define cm_entityString     cm.entityString
#define cm_numAreas         cm.numAreas
#define cm_areas            cm.areas
#define cm_areaPortals      cm.areaPortals
#define cm_numPatches       cm.numPatches
#define cm_patches          cm.patches
#define cm_floodvalid       cm.floodvalid
#define cm_checkcount       cm.checkcount
#define sv_worldMinsX       cm.worldMins[0]
#define sv_worldMinsY       cm.worldMins[1]
#define sv_worldMinsZ       cm.worldMins[2]
#define sv_worldMaxsX       cm.worldMaxs[0]
#define sv_worldMaxsY       cm.worldMaxs[1]
#define sv_worldMaxsZ       cm.worldMaxs[2]
#define box_brush           cm.box_brush
#define cm_boxModel         cm.box_model

/*
=============================================================================

						PROTOTYPES

	Argument ORDER below is the retail one; for __usercall functions the
	register assignment is noted at the definition site.

=============================================================================
*/

/* cm_load.c */
void CM_LoadMap( const char *name, qboolean clientload, int *checksum );
cmodel_t *CM_ClipHandleToModel( clipHandle_t handle );
clipHandle_t CM_InlineModel( int index );
int CM_NumClusters( void );
int CM_NumInlineModels( void );
const char *CM_EntityString( void );
int CM_LeafCluster( int leafnum );
int CM_LeafArea( int leafnum );
void CM_InitBoxHull( void );
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs,
							  int contents, qboolean capsule );
int CM_SetTempBoxModelContents( int contents );
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );

/* cm_test.c */
int CM_PointLeafnum_r( const vec3_t p, int num );
int CM_PointLeafnum( const vec3_t p );
void CM_StoreLeafs( leafList_t *ll, int nodenum );
void CM_StoreBrushes( leafList_t *ll, int nodenum );
void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );
int CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs,
					int *list, int listsize, int *lastLeaf );
int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs,
				   cbrush_t **list, int listsize );
int CM_PointContents( const vec3_t p, clipHandle_t model );
int CM_TransformedPointContents( const vec3_t p, clipHandle_t model,
								 const vec3_t origin, const vec3_t angles );
byte *CM_ClusterPVS( int cluster );
void CM_FloodArea_r( int areaNum, int floodnum );
void CM_FloodAreaConnections( void );
void CM_ChangeAreaPortalState( int area1, int area2, qboolean open );
qboolean CM_AreasConnected( int area1, int area2 );

/* cm_patch.c / cm_terrain.c -- the two collide builders.
 *
 * CM_GeneratePatchCollide's retail form is __usercall with width in edx,
 * points in ecx and height as the only stack argument; the order below is the
 * one its own Com_Error("...bad parameters: (%i, %i, %p)", width, height,
 * points) prints, which is the order that survives into C. */
/* FIVE arguments.  __usercall: width@edx, height@stack+0, maxError@stack+4,
   points@ecx, bounds@edi.  maxError is the PER-PATCH value from
   dcollisionsurface_t+8 (pushed at 0x00419AFF); bounds is &out->mins
   (EDI at 0x00419B0C), through which the builder fills the patch bounds.
   Without it every curved patch keeps zeroed mins/maxs and the sweep
   traces early-out on CM_TraceMayIntersectBox. */
patchCollide_t *CM_GeneratePatchCollide( int width, int height, int maxError,
										 const vec3_t *points, vec3_t *bounds );
/* FIVE arguments.  The fifth is &out->mins: CMod_LoadLeafCurvesAndTerrain
 * pushes out+12 at 0x00419A7B and the retail frame slot is arg_10 at +0x600B8,
 * and the builder seeds it to +/-262144 and folds every vertex into it, so
 * without it terrain patches keep zeroed bounds.  Slots 3 and 4 are the vertex
 * COUNT and the vertex ARRAY. */
void *CM_GenerateTerrainCollide( int numIndexes, const short *indexes,
								 int numVerts, const vec3_t *points,
								 vec3_t *bounds );

/* cm_world.c */
void CM_ClearWorld( void );
/* PORT ONLY -- no retail address.  Zeroes the world sector tail of cm that
 * retail clears with CM_LoadMap's Com_Memset(&cm, 0, 0xA15C).  Called from
 * CM_ClearMap, at the same point in the load that retail's memset runs: the
 * tree's Com_Memset inlines into CM_LoadMap, retail's (which calls memset32)
 * does not.  Do not move the clear into CM_ClearWorld, which retail does not
 * do. */
void CM_ClearWorldSectors( void );
qboolean CM_TraceBox( const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs, float fraction );

/* cm_staticmodel.c */
void CM_LoadStaticModels( void );

/* universal/com_math.c */
int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, cplane_t *p );

/* universal/com_memory_core.c */
void *Hunk_AllocAlignInternal( int size, int align );

#endif  /* __CM_LOCAL_H__ */
