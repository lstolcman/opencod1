/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_bsp.c -- world (BSP) loading (RTCW's RE_LoadWorldMap and the R_Load* lump
 * loaders).  CoD1's on-disk format and world_t differ substantially from RTCW:
 * lightmaps are packed into merged atlases, world surfaces are stored as
 * indexed "optimized" meshes (Generic/ARB/ATI/NV back ends), and the visibility
 * data is DPVS (cells / AABB trees / portals / occluders / cull groups) rather
 * than a PVS bit array.  The typed views below cover the data reached by
 * POINTER (the on-disk lumps and the hunk-allocated world records); the world
 * globals stay on the tr_records.h WLDBASE macros.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
int R_AllocMemoryNV( int firstPool, unsigned int size, void **memory );   /* tr_allocator.c */
int R_AllocMemoryATI( int firstPool, unsigned int size, int *offset );    /* tr_allocator.c */
/* ---- renderer-owned globals ---- */
char *entityParseCursor;
int bspImageBase;
/* RTCW tr_bsp.c s_worldData (world_t, 0x011A30E8); tr_records.h WLDBASE views address it */
unsigned char s_worldData[340];



#define r_optimize                 ((cvar_t *)(r_optimize))
#define r_vc_compile               ((cvar_t *)(r_vc_compile))


void AddPointToBounds( const float *point, float *mins, float *maxs );
void ClearBounds( vec3_t mins, vec3_t maxs );
extern int ExpandBounds();
int LongNoSwap( int l );        /* q_shared.c; LTCG inlines it (LittleLong) */
float FloatNoSwap( float f );   /* q_shared.c; LTCG inlines it, leaving retail's fld/fstp float copies */
extern int Hunk_AllocAlignInternal();
extern float NormalizeColorWhite_m( const float *in, float *out );
extern int RB_CalcTangentSpace();
extern void __cdecl RB_ComputeColors( int stage );   /* ONE argument -- 0x004DA88A pushes one dword */
extern int RB_StageIteratorSky();
int R_AllocModel();             /* tr_model.c; LTCG inlines it */
extern int R_AllocImage();
extern void R_FreeImage();      /* tr_image.c; LTCG inlines it */
extern int R_CreateBufferARB();
extern int R_CreateImageInternal();
void __cdecl R_CreateStaticModel(int world, char *name, const float *origin, const float *angles, const float *scale, const float *lightingPrecalc);
extern int R_FindShader();
void R_FlushSun( void );        /* tr_sky.c; LTCG inlines it */
extern int R_InitLightVisCacheFromBuffer();
extern void R_InitLightVisHistory();
void R_InitStaticModelCache( void );   /* tr_staticmodelcache.c */
void R_LoadSunThroughCvars( const char *sunName );   /* tr_sky.c */
extern int R_PrecalcLightVisCache();
int R_SetPlaneSidesDPVS( int plane );   /* tr_dpvs.c; LTCG inlines it */
void R_SyncRenderThread( void );        /* tr_cmds.c; LTCG inlines it */

/* ------------------------------------------------------------------------- *
 *  Constants.
 * ------------------------------------------------------------------------- */
#define BSP_VERSION                59
#define R_LIGHTMAP_SIZE            512      /* one source lightmap is 512x512 */
#define R_MAX_LIGHTMAPS            128
#define R_MAX_MAP_CORONAS          512
#define R_MAX_SPAWN_VARS           64       /* key/value pairs per entity */
#define SPAWN_VAR_CHARS            2048     /* key and value field sizes */
#define R_WORLD_SHADER_USAGE       8        /* R_FindShader "usage" for world faces */
#define R_CORONA_SHADER_USAGE      4

/* Surface back-end kinds (shader->optimizedBackend and mesh surfaceType).
 * SF_INDEXED_FIRST (24) comes from tr_local.h. */
#define SF_OPTIMIZED_GENERIC       25
#define SF_OPTIMIZED_ARB           26
#define SF_OPTIMIZED_ATI           27
#define SF_OPTIMIZED_NV            28

/* Static-vertex memory sources (shaderBuild_t::storageMode). */
#define VMEM_PRIMARY               1
#define VMEM_SECONDARY             2
#define VMEM_HUNK                  3

/* shader_t::flags bits read here. */
#define SHADER_FLAG_DEFAULTED      0x1u     /* generated default shader */
#define SHADER_FLAG_SKY            0x8u
#define SHADER_OPT_BLOCK_MASK      0x3FF5F900u   /* surfaceFlags that block optimization */
#define SHADER_STAGE_PER_LIGHT     0x80u    /* stage flags bit 7 */

#define SHADER_SORT_PORTAL_BITS    1065353216 /* 1.0f as int -- shader->sort for portals */

/* R_LIGHT_TYPE_* (bspDiskLight_t::type). */
#define R_LIGHT_TYPE_SUN           1
#define R_LIGHT_TYPE_POINT         2
#define R_LIGHT_TYPE_LINEAR_POINT  3
#define R_LIGHT_TYPE_CUSTOM_POINT  4
#define R_LIGHT_TYPE_SPOT          5
#define R_LIGHT_TYPE_CUSTOM_SPOT   7

/* BSP header lump indexes used by RE_LoadWorldMap (33-entry lump table). */
enum {
	LUMP_SHADERS               = 0,
	LUMP_LIGHTMAPS             = 1,
	LUMP_SURFACES              = 6,
	LUMP_DRAWVERTS             = 7,
	LUMP_DRAWINDEXES           = 8,
	LUMP_CULLGROUPS            = 9,
	LUMP_CULLGROUPINDEXES      = 10,
	LUMP_PORTALVERTS           = 11,
	LUMP_OCCLUDERS             = 12,
	LUMP_OCCLUDERPLANEINDEXES  = 13,
	LUMP_OCCLUDEREDGES         = 14,
	LUMP_OCCLUDERINDEXES       = 15,
	LUMP_AABBTREES             = 16,
	LUMP_CELLS                 = 17,
	LUMP_PORTALS               = 18,
	LUMP_LIGHTINDEXES          = 19,
	LUMP_NODES                 = 20,
	LUMP_LEAFS                 = 21,
	LUMP_MODELS                = 27,
	LUMP_ENTITIES              = 29,
	LUMP_LIGHTS                = 30,
	LUMP_LIGHTVISCACHE         = 32
};

/* ------------------------------------------------------------------------- *
 *  On-disk BSP records (read from the loaded file at bspImageBase).
 *  lump_t {filelen, fileofs} lives in qcommon.h.
 * ------------------------------------------------------------------------- */
typedef struct {
	int    ident;                 /* +0x00 */
	int    version;               /* +0x04 */
	lump_t lumps[33];             /* +0x08 */
} bspHeader_t;
COD1_ASSERT_SIZE(bspHeader_t, 0x110);
ASSERT_OFF(bspHeader_t, lumps, 0x08);

typedef struct {
	char     name[64];            /* +0x00 */
	unsigned surfaceFlags;        /* +0x40 */
	unsigned contentFlags;        /* +0x44 */
} bspDiskShader_t;
COD1_ASSERT_SIZE(bspDiskShader_t, 0x48);

typedef struct {
	short  shaderNum;             /* +0x00 */
	short  lightmapNum;           /* +0x02 */
	int    firstVert;             /* +0x04 */
	short  numVerts;              /* +0x08 */
	short  numIndexes;            /* +0x0a */
	int    firstIndex;            /* +0x0c */
} bspDiskSurface_t;
COD1_ASSERT_SIZE(bspDiskSurface_t, 0x10);
ASSERT_OFF(bspDiskSurface_t, firstVert, 0x04);
ASSERT_OFF(bspDiskSurface_t, numVerts, 0x08);
ASSERT_OFF(bspDiskSurface_t, firstIndex, 0x0c);

typedef struct {
	vec3_t xyz;                   /* +0x00 */
	float  st[2];                 /* +0x0c */
	float  lightmap[2];           /* +0x14 */
	vec3_t normal;                /* +0x1c */
	byte   color[4];              /* +0x28 */
} bspDrawVert_t;
COD1_ASSERT_SIZE(bspDrawVert_t, 0x2c);
ASSERT_OFF(bspDrawVert_t, st, 0x0c);
ASSERT_OFF(bspDrawVert_t, lightmap, 0x14);
ASSERT_OFF(bspDrawVert_t, normal, 0x1c);
ASSERT_OFF(bspDrawVert_t, color, 0x28);

typedef struct {
	vec3_t mins;                  /* +0x00 */
	vec3_t maxs;                  /* +0x0c */
	int    firstSurface;          /* +0x18 */
	int    numSurfaces;           /* +0x1c */
	int    firstLeafSurface;      /* +0x20 */
	int    numLeafSurfaces;       /* +0x24 */
	int    firstLeafBrush;        /* +0x28 */
	int    numLeafBrushes;        /* +0x2c */
} bspDiskModel_t;
COD1_ASSERT_SIZE(bspDiskModel_t, 0x30);
ASSERT_OFF(bspDiskModel_t, firstSurface, 0x18);

typedef struct {
	int    planeNum;              /* +0x00 */
	int    children[2];           /* +0x04  >=0 node index, <0 -(leaf index+1) */
	vec3_t mins;                  /* +0x0c  collision bounds, not read here */
	vec3_t maxs;                  /* +0x18 */
} bspDiskNode_t;
COD1_ASSERT_SIZE(bspDiskNode_t, 0x24);

typedef struct {
	int    cluster;               /* +0x00 */
	int    area;                  /* +0x04 */
	int    firstLeafTerrainPatch; /* +0x08 */
	int    numLeafTerrainPatches; /* +0x0c */
	int    firstLeafBrush;        /* +0x10 */
	int    numLeafBrushes;        /* +0x14 */
	int    cellNum;               /* +0x18 */
	int    firstLightIndex;       /* +0x1c */
	int    lightCount;            /* +0x20 */
} bspDiskLeaf_t;
COD1_ASSERT_SIZE(bspDiskLeaf_t, 0x24);
ASSERT_OFF(bspDiskLeaf_t, cellNum, 0x18);
ASSERT_OFF(bspDiskLeaf_t, firstLightIndex, 0x1c);
ASSERT_OFF(bspDiskLeaf_t, lightCount, 0x20);

/* One static light record on disk (72 B).  The 16-byte parameter union at
 * +0x28 is interpreted by the light type; the trailing 16 bytes are unread. */
typedef struct {
	int    type;                  /* +0x00  R_LIGHT_TYPE_* */
	vec3_t color;                 /* +0x04 */
	vec3_t position;              /* +0x10 */
	vec3_t direction;             /* +0x1c */
	union {
		struct { float linearAttenuation; } linearPoint;
		struct { float quadraticAttenuation; float constantAttenuation; } customPoint;
		struct { float cutoffCos; int exponent; } spot;
		struct { float quadraticAttenuation; float constantAttenuation; float cutoffCos; int exponent; } customSpot;
	} params;                     /* +0x28 */
	byte   _unused38[0x10];       /* +0x38  renderer-unused */
} bspDiskLight_t;
COD1_ASSERT_SIZE(bspDiskLight_t, 0x48);
ASSERT_OFF(bspDiskLight_t, position, 0x10);
ASSERT_OFF(bspDiskLight_t, direction, 0x1c);
ASSERT_OFF(bspDiskLight_t, params, 0x28);

typedef struct {
	vec3_t mins;                  /* +0x00 */
	vec3_t maxs;                  /* +0x0c */
	int    aabbTreeIndex;         /* +0x18 */
	int    firstPortal;           /* +0x1c */
	int    portalCount;           /* +0x20 */
	int    firstCullGroup;        /* +0x24 */
	int    cullGroupCount;        /* +0x28 */
	int    firstOccluder;         /* +0x2c */
	int    occluderCount;         /* +0x30 */
} bspDiskCell_t;
COD1_ASSERT_SIZE(bspDiskCell_t, 0x34);

typedef struct {
	int    planeIndex;            /* +0x00 */
	int    cellIndex;             /* +0x04 */
	int    firstVertex;           /* +0x08 */
	int    vertexCount;           /* +0x0c */
} bspDiskPortal_t;
COD1_ASSERT_SIZE(bspDiskPortal_t, 0x10);

typedef struct {
	vec3_t mins;                  /* +0x00 */
	vec3_t maxs;                  /* +0x0c */
	int    firstSurface;          /* +0x18 */
	int    surfaceCount;          /* +0x1c */
} bspDiskCullGroup_t;
COD1_ASSERT_SIZE(bspDiskCullGroup_t, 0x20);

typedef struct {
	int    firstPlane;            /* +0x00 */
	short  planeCount;            /* +0x04 */
	short  edgeCount;             /* +0x06 */
	int    firstEdge;             /* +0x08 */
	int    firstVertex;           /* +0x0c */
	short  vertexCount;           /* +0x10 */
	short  reserved_0x12;         /* +0x12  no access found (alignment) */
} bspDiskOccluder_t;
COD1_ASSERT_SIZE(bspDiskOccluder_t, 0x14);

typedef struct {
	byte   planeIndices[2];       /* +0x00 */
	byte   vertexIndices[2];      /* +0x02 */
} bspDiskOccluderEdge_t;
COD1_ASSERT_SIZE(bspDiskOccluderEdge_t, 0x04);

typedef struct {
	int    firstSurface;          /* +0x00 */
	int    surfaceCount;          /* +0x04 */
	int    childCount;            /* +0x08 */
} bspDiskAabbTree_t;
COD1_ASSERT_SIZE(bspDiskAabbTree_t, 0x0c);

/* One packed lightmap placement (which atlas, and the sub-rect scale/bias). */
typedef struct {
	short  atlasIndex;            /* +0x00 */
	float  sOffset;               /* +0x04 */
	float  tOffset;               /* +0x08 */
	float  sScale;                /* +0x0c */
	float  tScale;                /* +0x10 */
} bspLmPlacement_t;
COD1_ASSERT_SIZE(bspLmPlacement_t, 0x14);
ASSERT_OFF(bspLmPlacement_t, sScale, 0x0c);
ASSERT_OFF(bspLmPlacement_t, tScale, 0x10);

/* ------------------------------------------------------------------------- *
 *  Runtime (hunk-allocated) shader view.
 *
 *  Same layout as tr_local.h's shader_t, but with stages[] typed by the stage
 *  view below (bundles[] uses the layout documented in tr_shade.c). */
/* one texture bundle (0xc8; the layout tr_shade.c / tr_shade_*.c document) */
typedef struct {
	void  *image[32];               /* +0x00 */
	int   numImageAnimations;       /* +0x80 */
	float imageAnimationSpeed;      /* +0x84 */
	void  *waterMap;                /* +0x88  waterMap_t (tr_shader.c) */
	int   texCoordComponentCount;   /* +0x8c  texcoord components uploaded for this unit */
	int   textureEnvMode;           /* +0x90 */
	void  *texEnvCombine;           /* +0x94  ShaderTexEnvCombine (tr_shader.c) */
	void  *nvTextureShader;         /* +0x98  ShaderNVTextureOperation (tr_shader.c) */
	int   tcGen;                    /* +0x9c */
	float tcGenVectors[2][3];       /* +0xa0  TCGEN_VECTOR (RB_ComputeTexCoords) */
	int   numTexMods;               /* +0xb8 */
	void  *texMods;                 /* +0xbc  texModInfo_t[numTexMods] */
	int   videoMapHandle;           /* +0xc0 */
	byte  isLightmap;               /* +0xc4  r_lightmap forces this unit to REPLACE */
	byte  isVideoMap;               /* +0xc5 */
	byte  clampAnimation;           /* +0xc6 */
	byte  reserved_0xc7;            /* +0xc7  alignment */
} bspTextureBundle_t;
COD1_ASSERT_SIZE(bspTextureBundle_t, 0xc8);

typedef struct bspShaderStage_s {
	byte     flags;               /* +0x00  low byte of the stage flags; bit 0x80 => per-light */
	byte     flagsHigh[3];        /* +0x01  rest of the flags dword (unread here) */
	bspTextureBundle_t bundles[8];  /* +0x04  stride 0xc8 */
	int      atiFragmentShader;   /* +0x644  (tr_shader.c ParseATIFragmentShader) */
	void    *registerCombiners;   /* +0x648 */
	char    *vertexProgram;       /* +0x64c */
	byte     rgbWave[0x14];       /* +0x650  waveForm_t */
	int      rgbGen;              /* +0x664 */
	byte     alphaWave[0x14];     /* +0x668  waveForm_t */
	int      alphaGen;            /* +0x67c */
} bspShaderStage_t;
ASSERT_OFF(bspTextureBundle_t, textureEnvMode, 0x90);   /* bundles[0] @0x94, bundles[1] @0x15c */
ASSERT_OFF(bspShaderStage_t, bundles, 0x04);
ASSERT_OFF(bspShaderStage_t, rgbGen, 0x664);
ASSERT_OFF(bspShaderStage_t, alphaGen, 0x67c);

typedef struct bspShader_s {
	char              name[64];               /* +0x00 */
	int               lightmapIndex;          /* +0x40 */
	int               index;                  /* +0x44 */
	int               sortedIndex;            /* +0x48 */
	unsigned          flags;                  /* +0x4c */
	unsigned          surfaceFlags;           /* +0x50 */
	unsigned          lightingFlags;          /* +0x54 */
	float             sort;                   /* +0x58 */
	unsigned          surfaceParmFlags;       /* +0x5c */
	float             skyCloudHeight;         /* +0x60 */
	void             *skyOuterBoxImages[6];   /* +0x64 */
	void             *skyInnerBoxImages[6];   /* +0x7c */
	byte              fogParms[0x0c];         /* +0x94 */
	float             fogDepthForOpaque;      /* +0xa0 */
	float             portalRange;            /* +0xa4 */
	int               cullType;               /* +0xa8 */
	int               numDeforms;             /* +0xac */
	byte              deforms[0x14c - 0xb0];  /* +0xb0  deformStage_t[3] */
	float             boundsExpansion;        /* +0x14c */
	int               numUnfoggedPasses;      /* +0x150 */
	bspShaderStage_t *stages[8];              /* +0x154 */
	void             *optimalStageIteratorFunc; /* +0x174 */
	float             clampTime;              /* +0x178 */
	float             timeOffset;             /* +0x17c */
	struct bspShader_s *remappedShader;       /* +0x180 */
	void             *mergeImage;             /* +0x184 */
	int               optimizedBackend;       /* +0x188 */
	unsigned          optimizedVertexStorage; /* +0x18c  glBuffer / address / ATI buffer */
	int               optimizedVertexStorageOffset; /* +0x190 */
	struct bspShader_s *next;                 /* +0x194 */
} bspShader_t;
COD1_ASSERT_SIZE(bspShader_t, 0x198);
ASSERT_OFF(bspShader_t, flags, 0x4c);
ASSERT_OFF(bspShader_t, sort, 0x58);
ASSERT_OFF(bspShader_t, boundsExpansion, 0x14c);
ASSERT_OFF(bspShader_t, numUnfoggedPasses, 0x150);
ASSERT_OFF(bspShader_t, stages, 0x154);
ASSERT_OFF(bspShader_t, optimalStageIteratorFunc, 0x174);
ASSERT_OFF(bspShader_t, optimizedBackend, 0x188);
ASSERT_OFF(bspShader_t, optimizedVertexStorage, 0x18c);
ASSERT_OFF(bspShader_t, optimizedVertexStorageOffset, 0x190);

/* Per-shader cursor shared by BeginShaderSurfaces and its BuildOptimizedSurface
 * calls (renderer_shader_surface_build_t). */
typedef struct {
	int          optimized;       /* +0x00 */
	int          vertexBytes;     /* +0x04 */
	int          firstVertex;     /* +0x08 */
	int          storageMode;     /* +0x0c */
	bspShader_t *shader;          /* +0x10 */
} shaderBuild_t;
COD1_ASSERT_SIZE(shaderBuild_t, 0x14);
ASSERT_OFF(shaderBuild_t, firstVertex, 0x08);
ASSERT_OFF(shaderBuild_t, shader, 0x10);

/* World mesh surface: msurface_t::data for the unoptimized and four optimized
 * back ends.  Seven parallel vertex arrays plus the flexible index tail follow
 * the header in one hunk block. */
typedef struct {
	int      surfaceType;         /* +0x00 */
	int      storageMode;         /* +0x04 */
	unsigned dlightBits;          /* +0x08 */
	vec3_t   boundsMin;           /* +0x0c */
	vec3_t   boundsMax;           /* +0x18 */
	int      vertexCount;         /* +0x24 */
	vec3_t  *tangents;            /* +0x28 */
	vec3_t  *bitangents;          /* +0x2c */
	vec3_t  *normals;             /* +0x30 */
	float  (*texCoords)[2];       /* +0x34 */
	float  (*lightmapCoords)[2];  /* +0x38 */
	byte   (*colors)[4];          /* +0x3c */
	vec3_t  *positions;           /* +0x40 */
	int      indexCount;          /* +0x44 */
	unsigned short indices[1];    /* +0x48  runs to indexCount */
} meshSurface_t;
ASSERT_OFF(meshSurface_t, vertexCount, 0x24);
ASSERT_OFF(meshSurface_t, positions, 0x40);
ASSERT_OFF(meshSurface_t, indexCount, 0x44);
ASSERT_OFF(meshSurface_t, indices, 0x48);

/* Interleaved vertex in the optimized world VBO stream (32 B). */
typedef struct {
	float  texCoord[2];           /* +0x00 */
	float  lightmapCoord[2];      /* +0x08 */
	byte   color[4];              /* +0x10 */
	vec3_t position;              /* +0x14 */
} interleavedVertex_t;
COD1_ASSERT_SIZE(interleavedVertex_t, 0x20);

/* DPVS plane (portal / occluder): normal + dist + three precomputed
 * mins/maxs byte offsets used during clipping. */
typedef struct {
	vec3_t normal;                /* +0x00 */
	float  distance;              /* +0x0c */
	byte   sideOffsets[3];        /* +0x10 */
	byte   cameraSide;            /* +0x13  never written here */
} bspDpvsPlane_t;
COD1_ASSERT_SIZE(bspDpvsPlane_t, 0x14);

/* Runtime world records (all hunk-allocated by the loaders below). */
typedef struct worldAabbTree_s worldAabbTree_t;
struct worldAabbTree_s {
	vec3_t           mins;        /* +0x00 */
	vec3_t           maxs;        /* +0x0c */
	msurface_t      *surfaces;    /* +0x18 */
	int              surfaceCount;/* +0x1c */
	worldAabbTree_t *children;    /* +0x20 */
	int              childCount;  /* +0x24 */
};
COD1_ASSERT_SIZE(worldAabbTree_t, 0x28);
ASSERT_OFF(worldAabbTree_t, surfaces, 0x18);
ASSERT_OFF(worldAabbTree_t, children, 0x20);

typedef struct {
	vec3_t      mins;             /* +0x00 */
	vec3_t      maxs;             /* +0x0c */
	msurface_t *surfaces;         /* +0x18 */
	int         surfaceCount;     /* +0x1c */
	int         viewCount;        /* +0x20 */
} worldCullGroup_t;
COD1_ASSERT_SIZE(worldCullGroup_t, 0x24);

struct worldCell_s;

typedef struct {
	bspDpvsPlane_t      plane;    /* +0x00 */
	struct worldCell_s *cell;     /* +0x14 */
	vec3_t             *vertices; /* +0x18 */
	int                 vertexCount; /* +0x1c */
	int                 recursionActive; /* +0x20 */
} worldPortal_t;
COD1_ASSERT_SIZE(worldPortal_t, 0x24);

typedef struct {
	bspDpvsPlane_t *planes[2];    /* +0x00 */
	vec3_t         *vertices[2];  /* +0x08 */
} worldOccluderEdge_t;
COD1_ASSERT_SIZE(worldOccluderEdge_t, 0x10);

typedef struct {
	bspDpvsPlane_t      *planes;       /* +0x00 */
	int                  planeCount;   /* +0x04 */
	int                  edgeCount;    /* +0x08 */
	worldOccluderEdge_t *edges;        /* +0x0c */
	int                  vertexCount;  /* +0x10 */
	vec3_t              *vertices;     /* +0x14 */
	int                  cullPlaneLimit; /* +0x18 */
	int                  activePlaneCount; /* +0x1c */
	bspDpvsPlane_t      *activePlanes; /* +0x20 */
} worldOccluder_t;
COD1_ASSERT_SIZE(worldOccluder_t, 0x24);

typedef struct worldCell_s {
	vec3_t             mins;          /* +0x00 */
	vec3_t             maxs;          /* +0x0c */
	worldAabbTree_t   *aabbTree;      /* +0x18 */
	int                portalRef;     /* +0x1c  R_LoadCells stores 36*firstPortal; R_LoadPortals resolves it */
	int                portalCount;   /* +0x20 */
	worldCullGroup_t **cullGroups;    /* +0x24 */
	int                cullGroupCount;/* +0x28 */
	worldOccluder_t  **occluders;     /* +0x2c */
	int                occluderCount; /* +0x30 */
	int                markViewCount; /* +0x34 */
	void              *modelLinks;    /* +0x38 */
	void              *entityLinks;   /* +0x3c */
} worldCell_t;
COD1_ASSERT_SIZE(worldCell_t, 0x40);
ASSERT_OFF(worldCell_t, portalRef, 0x1c);
ASSERT_OFF(worldCell_t, cullGroups, 0x24);
ASSERT_OFF(worldCell_t, occluders, 0x2c);

/* World corona parsed from the entity lump (32 B). */
typedef struct {
	int    shader;                /* +0x00 */
	vec3_t origin;                /* +0x04 */
	float  scale;                 /* +0x10 */
	float  zCutoff;               /* +0x14 */
	float  zFadeOut;              /* +0x18 */
	byte   color[4];              /* +0x1c */
} bspCorona_t;
COD1_ASSERT_SIZE(bspCorona_t, 0x20);
ASSERT_OFF(bspCorona_t, scale, 0x10);
ASSERT_OFF(bspCorona_t, color, 0x1c);

/* One parsed spawn-var record; record 0 is the classname pair. */
typedef struct {
	char key[SPAWN_VAR_CHARS];    /* +0x0000 */
	char value[SPAWN_VAR_CHARS];  /* +0x0800 */
} entityPair_t;
COD1_ASSERT_SIZE(entityPair_t, 0x1000);

/* ------------------------------------------------------------------------- *
 *  Small file-local helpers (inlined in retail; no external linkage).
 * ------------------------------------------------------------------------- */

/* One tokenizer step honouring the parser's unget-one-token state.  Retail
 * inlines this before every Com_ParseExt call in the world loaders. */
static char *R_ParseWorldToken(char **data_p)
{
	if ( parseInfo->ungetReady )
	{
		parseInfo->ungetReady = qfalse;
		*data_p = parseInfo->ungetTokenSave;
		parseInfo->currentLine = parseInfo->ungetLineSave;
	}
	return Com_ParseExt(data_p, qtrue);
}

/* ---- R_ColorShiftLightingBytes  0x004D9AF0 ----  VERIFIED
 * Overbright-scale one RGBA lightmap texel, renormalizing so the brightest
 * channel is preserved when the scale would overflow 255. */
void __cdecl R_ColorShiftLightingBytes(unsigned __int8 *in, byte *out)
{
	int shift, r, g, b;

	shift = 1 - tr_overbrightBits;
	r = in[0] << shift;
	g = in[1] << shift;
	b = in[2] << shift;

	if ( (r | g | b) > 255 )
	{
		int max;

		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}
	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

/* ---- R_BuildLightmapMergability  0x004D9B90 ----  [HIGH]
 * Decide how the original 512x512 lightmaps pack into hardware-sized atlases.
 * Accumulate, for every pair of source lightmaps, the vertex count of the
 * surfaces that share a shader (their affinity), then greedily group the
 * highest-affinity lightmaps into each atlas.  Fills atlasDimensions (per-atlas
 * width/height in tiles) and lightmapOrder (packing order), returns the source
 * lightmap count. */
int __cdecl R_BuildLightmapMergability(lump_t *surfaceLump, int (*atlasDimensions)[2], int *lightmapOrder)
{
	bspDiskSurface_t *diskSurfaces;
	int              surfaceCount;
	int              originalLightmapCount;
	int              shaderIndex;
	int              lightmapNum;
	int              i;
	int              j;
	int              width;
	int              height;
	int              atlasCount;
	int              assignedCount;
	int              capacity;
	int              bestI;
	int              bestJ;
	int              slot;
	int              scratch[R_MAX_LIGHTMAPS];         /* per-shader vertex totals, then assigned markers */
	int              affinity[R_MAX_LIGHTMAPS][R_MAX_LIGHTMAPS];

	diskSurfaces = (bspDiskSurface_t *)(bspImageBase + surfaceLump->fileofs);
	if ( surfaceLump->filelen % sizeof(*diskSurfaces) )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	surfaceCount = surfaceLump->filelen / sizeof(*diskSurfaces);

	originalLightmapCount = 0;
	for ( i = 0; i < surfaceCount; i++ )
	{
		if ( originalLightmapCount <= diskSurfaces[i].lightmapNum )
			originalLightmapCount = diskSurfaces[i].lightmapNum + 1;
	}

	memset(scratch, 0, sizeof(scratch));
	memset(affinity, 0, sizeof(affinity));

	for ( shaderIndex = 0; shaderIndex < s_worldData_numShaders; shaderIndex++ )
	{
		for ( i = 0; i < surfaceCount; i++ )
		{
			if ( diskSurfaces[i].shaderNum == shaderIndex )
			{
				lightmapNum = diskSurfaces[i].lightmapNum;
				if ( lightmapNum >= 0 )
					scratch[lightmapNum] += diskSurfaces[i].numVerts;
			}
		}

		for ( i = 0; i < originalLightmapCount; i++ )
		{
			if ( !scratch[i] )
				continue;
			for ( j = i + 1; j < originalLightmapCount; j++ )
			{
				if ( !scratch[j] )
					continue;
				affinity[i][j] += scratch[i] + scratch[j];
				if ( affinity[i][j] < 0 )
					affinity[i][j] = 0x7FFFFFFF;
				affinity[j][i] = affinity[i][j];
			}
			scratch[i] = 0;
		}
	}

	memset(scratch, 0, sizeof(scratch));

	atlasCount = 0;
	width = glConfig_maxTextureSize / R_LIGHTMAP_SIZE;
	height = glConfig_maxTextureSize / R_LIGHTMAP_SIZE;
	for ( assignedCount = 0; assignedCount < originalLightmapCount; atlasCount++ )
	{
		while ( width * height > originalLightmapCount - assignedCount )
		{
			if ( width >= height )
				width >>= 1;
			else
				height >>= 1;
		}

		capacity = width * height;
		if ( capacity >= 2 )
		{
			/* strongest remaining pair */
			bestI = -1;
			bestJ = -1;
			for ( i = 0; i < originalLightmapCount; i++ )
			{
				if ( scratch[i] )
					continue;
				for ( j = i + 1; j < originalLightmapCount; j++ )
				{
					if ( !scratch[j] && (bestI < 0 || affinity[i][j] > affinity[bestI][bestJ]) )
					{
						bestI = i;
						bestJ = j;
					}
				}
			}
			lightmapOrder[assignedCount++] = bestJ;
			lightmapOrder[assignedCount++] = bestI;
			scratch[bestJ] = 1;
			scratch[bestI] = 1;

			/* then grow the group by the lightmap closest to all of it */
			for ( slot = 2; slot < capacity; slot++ )
			{
				for ( i = 0; i < originalLightmapCount; i++ )
				{
					affinity[bestI][i] += affinity[bestJ][i];
					affinity[i][bestI] = affinity[bestI][i];
				}
				bestJ = -1;
				for ( i = 0; i < originalLightmapCount; i++ )
				{
					if ( !scratch[i] && (bestJ < 0 || affinity[bestI][i] > affinity[bestI][bestJ]) )
						bestJ = i;
				}
				lightmapOrder[assignedCount++] = bestJ;
				scratch[bestJ] = 1;
			}
		}
		else
		{
			for ( i = 0; i < originalLightmapCount; i++ )
			{
				if ( !scratch[i] )
				{
					lightmapOrder[assignedCount++] = i;
					scratch[i] = 1;
					break;
				}
			}
		}

		atlasDimensions[atlasCount][0] = width;
		atlasDimensions[atlasCount][1] = height;
	}

	ri.Printf(0, "%i merged lightmaps from %i original lightmaps\n", atlasCount, originalLightmapCount);
	return originalLightmapCount;
}

/* ---- R_CopyLightmap  0x004D9F30 ----  VERIFIED
 * Overbright-copy one 512x512 source lightmap into the atlas at (col,row),
 * setting alpha to 255. */
void __cdecl R_CopyLightmap(int destRow, int destCol, int atlasWidth, byte *dst, unsigned __int8 *src)
{
	int row;
	int col;

	dst += 4 * (destCol + atlasWidth * destRow);
	for ( row = 0; row < R_LIGHTMAP_SIZE; ++row )
	{
		for ( col = 0; col < R_LIGHTMAP_SIZE; ++col )
		{
			R_ColorShiftLightingBytes(src, dst);
			dst[3] = -1;
			src += 3;
			dst += 4;
		}
		dst += 4 * atlasWidth - 4 * R_LIGHTMAP_SIZE;
	}
}

/* ---- R_LoadLightmaps  0x004D9FA0 ----  [HIGH]
 * Pack the source lightmaps into merged atlases (per R_BuildLightmapMergability),
 * copy each into its atlas tile, record every source lightmap's placement, and
 * upload one image per atlas. */
void __cdecl R_LoadLightmaps(lump_t *lightmapLump, lump_t *surfaceLump, bspLmPlacement_t *placements)
{
	byte  *buf;
	int    len;
	int    originalLightmapCount;
	int    imageBytes;
	byte  *image;
	int    i;
	short  atlasIndex;
	int    consumed;
	int    width;
	int    height;
	int    tileCount;
	short  tile;
	int    lightmapIndex;
	int    tileX;
	int    tileY;
	rfloat invWidth;
	rfloat invHeight;
	int    lightmapImage;
	int    imageWidth;
	int    imageHeight;
	char  *name;
	int    atlasDimensions[R_MAX_LIGHTMAPS][2];
	int    lightmapOrder[R_MAX_LIGHTMAPS];

	len = lightmapLump->filelen;
	buf = (byte *)(bspImageBase + lightmapLump->fileofs);

	R_SyncRenderThread();

	originalLightmapCount = R_BuildLightmapMergability(surfaceLump, atlasDimensions, lightmapOrder);
	if ( len && 786432 * originalLightmapCount != len )   /* 512*512*3 per source lightmap */
		ri.Error(1, "\x15" "R_LoadLightmaps: incorrect lightmap lump size");
	if ( r_fullbright->integer )
		len = 0;

	imageBytes = (atlasDimensions[0][0] * atlasDimensions[0][1]) << 20;   /* w*h * (512*512*4) */
	image = (byte *)ri.Hunk_AllocateTempMemory(imageBytes);
	if ( !len )
	{
		for ( i = 0; i < imageBytes; i += 4 )
		{
			image[i + 0] = tr_identityLightByte;
			image[i + 1] = tr_identityLightByte;
			image[i + 2] = tr_identityLightByte;
			image[i + 3] = 255;
		}
	}

	for ( atlasIndex = 0, consumed = 0; consumed < originalLightmapCount; atlasIndex++ )
	{
		width = atlasDimensions[atlasIndex][0];
		height = atlasDimensions[atlasIndex][1];
		tileCount = width * height;
		imageWidth = width << 9;
		imageHeight = height << 9;
		for ( tile = 0; tile < tileCount; tile++ )
		{
			invWidth = 1.0f / (rfloat)width;    /* loop-invariant: VC7 hoists both */
			invHeight = 1.0f / (rfloat)height;
			lightmapIndex = lightmapOrder[tile + consumed];
			tileX = tile % width;
			tileY = tile / width;
			if ( len )
				R_CopyLightmap(tileY << 9, tileX << 9, imageWidth, image, buf + 786432 * lightmapIndex);

			placements[lightmapIndex].sScale = invWidth;
			placements[lightmapIndex].atlasIndex = atlasIndex;
			placements[lightmapIndex].tScale = invHeight;
			placements[lightmapIndex].sOffset = tileX * invWidth;
			placements[lightmapIndex].tOffset = tileY * invHeight;
		}

		name = va("*lightmap%d", atlasIndex);
		lightmapImage = R_AllocImage(name, 3553, imageWidth, imageHeight, 56, 3);
		if ( !R_CreateImageInternal(lightmapImage, 0, image, 0xDE1u, 6408) )
		{
			R_FreeImage(lightmapImage);
			lightmapImage = 0;
		}
		tr_lightmaps[atlasIndex] = lightmapImage;

		consumed += tileCount;
	}
	tr_lightmapCount = atlasIndex;
	ri.Hunk_FreeTempMemory(image);
}

/* ---- ShaderForShaderNum  0x004DA280 ----  VERIFIED */
char *__cdecl ShaderForShaderNum(__int16 shaderNum, __int16 lightmapNum, const char *usage)
{
	char            *shader;
	bspDiskShader_t *dsh;

	if ( shaderNum < 0 || shaderNum >= s_worldData_numShaders )
		ri.Error(1, "\x15" "ShaderForShaderNum: bad num %i", shaderNum);
	dsh = &((bspDiskShader_t *)s_worldData_shaders)[shaderNum];

	shader = (char *)R_FindShader(dsh->name, lightmapNum, 1, usage);
	if ( (((bspShader_t *)shader)->flags & SHADER_FLAG_DEFAULTED) != 0 )
		return tr_defaultShader;
	return shader;
}

/* ---- LittleVertices_T2T2C4V3  0x004DA2E0 ----  VERIFIED
 * Copy one triangle-soup surface's draw vertices into the mesh surface's
 * parallel arrays (biasing the lightmap coords into the atlas sub-rect), then,
 * when tangent-space arrays are present, run RB_CalcTangentSpace over the tess
 * scratch and store the generated tangents/bitangents. */
void __cdecl LittleVertices_T2T2C4V3(bspDrawVert_t *vertices, meshSurface_t *surface, int vertexCount,
                                     bspLmPlacement_t *placement, int firstVertex)
{
	int i;

	for ( i = 0; i < vertexCount; ++i )
	{
		if ( surface->normals )
		{
			surface->normals[i][0] = FloatNoSwap(vertices[i].normal[0]);
			surface->normals[i][1] = FloatNoSwap(vertices[i].normal[1]);
			surface->normals[i][2] = FloatNoSwap(vertices[i].normal[2]);
		}
		surface->texCoords[i][0] = FloatNoSwap(vertices[i].st[0]);
		surface->texCoords[i][1] = FloatNoSwap(vertices[i].st[1]);
		surface->lightmapCoords[i][0] = FloatNoSwap(vertices[i].lightmap[0]);
		surface->lightmapCoords[i][1] = FloatNoSwap(vertices[i].lightmap[1]);
		if ( placement )
		{
			surface->lightmapCoords[i][0] = placement->sScale * surface->lightmapCoords[i][0] + placement->sOffset;
			surface->lightmapCoords[i][1] = placement->tScale * surface->lightmapCoords[i][1] + placement->tOffset;
		}
		surface->colors[i][0] = vertices[i].color[0];
		surface->colors[i][1] = vertices[i].color[1];
		surface->colors[i][2] = vertices[i].color[2];
		surface->colors[i][3] = vertices[i].color[3];
		surface->positions[i][0] = FloatNoSwap(vertices[i].xyz[0]);
		surface->positions[i][1] = FloatNoSwap(vertices[i].xyz[1]);
		surface->positions[i][2] = FloatNoSwap(vertices[i].xyz[2]);
		AddPointToBounds(surface->positions[i], surface->boundsMin, surface->boundsMax);
	}

	if ( !surface->tangents || !surface->bitangents || !surface->normals )
		return;

	tess_numVertexes = surface->vertexCount;
	tess_numIndexes = surface->indexCount;
	tess_stageTangentsValid = 0;
	tess_stageBitangentsValid = 0;
	tess_requiresVertexBasis = 1;
	Com_Memcpy(tess_stageNormals, surface->normals, 12 * surface->vertexCount);
	Com_Memcpy(tess_texCoords0, surface->texCoords, 8 * surface->vertexCount);
	Com_Memcpy(tess_xyz, surface->positions, 12 * surface->vertexCount);
	for ( i = 0; i < surface->indexCount; ++i )
		tess_indexes[i] = surface->indices[i] - firstVertex;
	RB_CalcTangentSpace();
	Com_Memcpy(surface->tangents, tess_stageTangents, 12 * surface->vertexCount);
	Com_Memcpy(surface->bitangents, tess_stageBitangents, 12 * surface->vertexCount);
	tess_numVertexes = 0;
	tess_numIndexes = 0;
	tess_stageTangentsValid = 0;
	tess_stageBitangentsValid = 0;
	tess_requiresVertexBasis = 0;
}

/* ---- OptimizeVertices_T2T2C4V3_GenericOrNV  0x004DA500 ----  [HIGH]
 * Interleave the mesh surface's arrays into the destination VBO stream in
 * place (software / NV back ends). */
void __cdecl OptimizeVertices_T2T2C4V3_GenericOrNV(meshSurface_t *surface, int verticesArg)
{
	interleavedVertex_t *vertices = (interleavedVertex_t *)verticesArg;
	int i;

	for ( i = 0; i < surface->vertexCount; ++i )
	{
		vertices[i].texCoord[0] = surface->texCoords[i][0];
		vertices[i].texCoord[1] = surface->texCoords[i][1];
		vertices[i].lightmapCoord[0] = surface->lightmapCoords[i][0];
		vertices[i].lightmapCoord[1] = surface->lightmapCoords[i][1];
		vertices[i].color[0] = surface->colors[i][0];
		vertices[i].color[1] = surface->colors[i][1];
		vertices[i].color[2] = surface->colors[i][2];
		vertices[i].color[3] = surface->colors[i][3];
		vertices[i].position[0] = surface->positions[i][0];
		vertices[i].position[1] = surface->positions[i][1];
		vertices[i].position[2] = surface->positions[i][2];
	}
}

/* ---- OptimizeVertices_T2T2C4V3_ATI  0x004DA5A0 ----  [HIGH]
 * Interleave the mesh surface's arrays into a temp buffer and upload it to the
 * ATI object buffer. */
int __cdecl OptimizeVertices_T2T2C4V3_ATI(meshSurface_t *surface, int objectBuffer, int vertexOffset)
{
	int                  vertexBytes = 32 * surface->vertexCount;
	interleavedVertex_t *vertices = (interleavedVertex_t *)ri.Hunk_AllocateTempMemory(vertexBytes);
	int                  i;

	for ( i = 0; i < surface->vertexCount; ++i )
	{
		vertices[i].texCoord[0] = surface->texCoords[i][0];
		vertices[i].texCoord[1] = surface->texCoords[i][1];
		vertices[i].lightmapCoord[0] = surface->lightmapCoords[i][0];
		vertices[i].lightmapCoord[1] = surface->lightmapCoords[i][1];
		vertices[i].color[0] = surface->colors[i][0];
		vertices[i].color[1] = surface->colors[i][1];
		vertices[i].color[2] = surface->colors[i][2];
		vertices[i].color[3] = surface->colors[i][3];
		vertices[i].position[0] = surface->positions[i][0];
		vertices[i].position[1] = surface->positions[i][1];
		vertices[i].position[2] = surface->positions[i][2];
	}
	qglUpdateObjectBufferATI(objectBuffer, vertexOffset, vertexBytes, vertices, 34658);
	return ri.Hunk_FreeTempMemory(vertices);
}

/* ---- OptimizeVertices_T2T2C4V3_ARB  0x004DA660 ----  [HIGH]
 * Interleave the mesh surface's arrays into a temp buffer and upload it into
 * the ARB vertex buffer. */
int __cdecl OptimizeVertices_T2T2C4V3_ARB(meshSurface_t *surface, int vertexBuffer, int vertexOffset)
{
	int                  vertexBytes = 32 * surface->vertexCount;
	interleavedVertex_t *vertices = (interleavedVertex_t *)ri.Hunk_AllocateTempMemory(vertexBytes);
	int                  i;

	for ( i = 0; i < surface->vertexCount; ++i )
	{
		vertices[i].texCoord[0] = surface->texCoords[i][0];
		vertices[i].texCoord[1] = surface->texCoords[i][1];
		vertices[i].lightmapCoord[0] = surface->lightmapCoords[i][0];
		vertices[i].lightmapCoord[1] = surface->lightmapCoords[i][1];
		vertices[i].color[0] = surface->colors[i][0];
		vertices[i].color[1] = surface->colors[i][1];
		vertices[i].color[2] = surface->colors[i][2];
		vertices[i].color[3] = surface->colors[i][3];
		vertices[i].position[0] = surface->positions[i][0];
		vertices[i].position[1] = surface->positions[i][1];
		vertices[i].position[2] = surface->positions[i][2];
	}
	qglBindBufferARB(34962, vertexBuffer);
	qglBufferSubDataARB(34962, vertexOffset, vertexBytes, vertices);
	qglBindBufferARB(34962, 0);
	return ri.Hunk_FreeTempMemory(vertices);
}

/* ---- R_RefreshOptimizedWorldSurfaces_ARB  0x004DA740 ----  [HIGH]
 * Re-upload every ARB-optimized world surface (used when the GL context's
 * buffers were lost).  Retail dereferences tr.world with no null guard here. */
void R_RefreshOptimizedWorldSurfaces_ARB()
{
	int i;

	for ( i = 0; i < *(int *)(tr_world + 156); ++i )
	{
		meshSurface_t *mesh = (meshSurface_t *)((msurface_t *)*(int *)(tr_world + 160))[i].data;

		if ( mesh->surfaceType == SF_OPTIMIZED_ARB )
		{
			int firstVertex = mesh->indices[0];
			int j;

			for ( j = 1; j < mesh->indexCount; ++j )
			{
				if ( firstVertex > mesh->indices[j] )
					firstVertex = mesh->indices[j];
			}
			OptimizeVertices_T2T2C4V3_ARB(mesh,
			    ((bspShader_t *)((msurface_t *)*(int *)(tr_world + 160))[i].shader)->optimizedVertexStorage,
			    32 * firstVertex);
		}
	}
}

/* ---- R_IncrementalRefreshOptimizedWorldSurfaces_ARB  0x004DA7D0 ----  [HIGH]
 * Refresh one ARB-optimized world surface per call, round-robin. */
void R_IncrementalRefreshOptimizedWorldSurfaces_ARB()
{
	int start;

	if ( !tr_world )
		return;

	start = tr_worldRefreshSurfaceIndex;
	while ( 1 )
	{
		meshSurface_t *mesh;

		++tr_worldRefreshSurfaceIndex;
		if ( tr_worldRefreshSurfaceIndex == *(int *)(tr_world + 156) )
			tr_worldRefreshSurfaceIndex = 0;
		if ( tr_worldRefreshSurfaceIndex == start )
			return;

		mesh = (meshSurface_t *)((msurface_t *)*(int *)(tr_world + 160))[tr_worldRefreshSurfaceIndex].data;
		if ( mesh->surfaceType == SF_OPTIMIZED_ARB )
		{
			int firstVertex = mesh->indices[0];
			int j;

			for ( j = 1; j < mesh->indexCount; ++j )
			{
				if ( firstVertex > mesh->indices[j] )
					firstVertex = mesh->indices[j];
			}
			OptimizeVertices_T2T2C4V3_ARB(mesh,
			    ((bspShader_t *)((msurface_t *)*(int *)(tr_world + 160))[tr_worldRefreshSurfaceIndex].shader)->optimizedVertexStorage,
			    32 * firstVertex);
			return;
		}
	}
}

/* ---- AdjustColors  0x004DA860 ----  VERIFIED
 * Run the shader's first stage colour generator over the mesh surface's vertex
 * colours (baking rgbGen/alphaGen into the stored colours). */
void __cdecl AdjustColors(meshSurface_t *surface, msurface_t *worldSurface)
{
	tess_numVertexes = surface->vertexCount;
	memcpy(tess_vertexColors, surface->colors, 4 * surface->vertexCount);
	RB_ComputeColors((int)((bspShader_t *)worldSurface->shader)->stages[0]);
	memcpy(surface->colors, tess_stageVertexColors, 4 * surface->vertexCount);
	tess_numVertexes = 0;
}

/* ---- CanOptimizeShader  0x004DA8D0 ----  VERIFIED
 * A shader's world surfaces can share the optimized (indexed VBO) path only
 * when every non-per-light stage uses both texture units and the same
 * rgbGen/alphaGen as the first stage. */
int __cdecl CanOptimizeShader(int shaderArg)
{
	bspShader_t *shader = (bspShader_t *)shaderArg;
	int          i;

	if ( !r_optimizeWorld->integer
	  || !r_optimize->integer
	  || (shader->flags & SHADER_FLAG_DEFAULTED) != 0
	  || (shader->surfaceFlags & SHADER_OPT_BLOCK_MASK) != 0
	  || !shader->numUnfoggedPasses
	  || *(int *)&shader->sort == SHADER_SORT_PORTAL_BITS )
	{
		return 0;
	}

	for ( i = 0; i < shader->numUnfoggedPasses; ++i )
	{
		if ( (shader->stages[i]->flags & SHADER_STAGE_PER_LIGHT) != 0 )
			continue;
		if ( shader->stages[i]->bundles[0].textureEnvMode == 0
		  || shader->stages[i]->bundles[1].textureEnvMode == 0
		  || shader->stages[i]->rgbGen != shader->stages[0]->rgbGen
		  || shader->stages[i]->alphaGen != shader->stages[0]->alphaGen )
		{
			return 0;
		}
	}
	return 1;
}

/* ---- BeginShaderSurfaces  0x004DA980 ----  [HIGH]
 * Choose the storage back end for a shader's world surfaces (ARB VBO, NV vertex
 * array range, ATI object buffer, or a plain hunk block) and record it in the
 * build cursor for the BuildOptimizedSurface calls that follow. */
void __cdecl BeginShaderSurfaces(int vertexCount, int *buildArg, unsigned int *shaderArg)
{
	shaderBuild_t *build = (shaderBuild_t *)buildArg;
	bspShader_t   *shader = (bspShader_t *)shaderArg;

	if ( vertexCount >= 0x10000 )
	{
		ri.Error(1, "\x15" "surface with shader %s has more than 65,536 vertices\n", shader);
		return;
	}

	build->optimized = CanOptimizeShader((int)shader);
	build->vertexBytes = 32 * vertexCount;

	if ( build->optimized )
	{
		shader->optimizedBackend = SF_OPTIMIZED_GENERIC;
		if ( glConfig_ARBVertexBufferObject )
		{
			shader->optimizedVertexStorage = R_CreateBufferARB(34962, build->vertexBytes, 0, 35044);
			if ( shader->optimizedVertexStorage )
			{
				shader->optimizedBackend = SF_OPTIMIZED_ARB;
				build->storageMode = storageClass;
			}
		}
		else if ( glConfig_NVVertexArrayRange )
		{
			shader->optimizedBackend = SF_OPTIMIZED_NV;
			build->storageMode = R_AllocMemoryNV( VMEM_PRIMARY, build->vertexBytes, (void **)&shader->optimizedVertexStorage );
		}
		else if ( glConfig_ATIVertexArrayObject )
		{
			build->storageMode = R_AllocMemoryATI( VMEM_SECONDARY, build->vertexBytes, &shader->optimizedVertexStorageOffset );
			if ( build->storageMode == VMEM_SECONDARY || build->storageMode == VMEM_PRIMARY )
			{
				shader->optimizedBackend = SF_OPTIMIZED_ATI;
				if ( build->storageMode == VMEM_SECONDARY )
					shader->optimizedVertexStorage = tr_staticVertexMemorySecondary;
				else
					shader->optimizedVertexStorage = tr_staticVertexMemoryPrimary;
				build->storageMode = VMEM_HUNK;
			}
		}

		if ( shader->optimizedBackend == SF_OPTIMIZED_GENERIC )
		{
			build->storageMode = VMEM_HUNK;
			shader->optimizedVertexStorage = ri.Hunk_Alloc(build->vertexBytes);
		}
	}
	else
	{
		shader->optimizedBackend = SF_INDEXED_FIRST;
		build->storageMode = storageClass;
	}
	build->shader = shader;
	build->firstVertex = 0;
}

/* ---- BuildOptimizedSurface  0x004DAB70 ----  [HIGH]
 * Allocate a world mesh surface for one triangle soup, copy its vertices and
 * indices in, bake colours when the shader optimizes, upload the interleaved
 * stream through the selected back end, and publish it on the world surface. */
int __cdecl BuildOptimizedSurface(msurface_t *worldSurface, shaderBuild_t *build, bspLmPlacement_t *placement,
                                  int vertexCount, int verticesArg, int indexCount, unsigned short *indices)
{
	int            canOptimize;
	int            headerBytes;
	int            vec3Bytes;
	int            vec2Bytes;
	int            tangentBytes;
	meshSurface_t *surface;
	int            i;

	canOptimize = CanOptimizeShader((int)worldSurface->shader);
	headerBytes = 2 * indexCount + 72;
	vec3Bytes = 12 * vertexCount;
	tangentBytes = 3 * vec3Bytes;
	vec2Bytes = 8 * vertexCount;
	surface = (meshSurface_t *)ri.Hunk_Alloc(tangentBytes + 2 * vec2Bytes + 4 * vertexCount + vec3Bytes + headerBytes);
	surface->tangents = (vec3_t *)((byte *)surface + headerBytes);
	surface->bitangents = (vec3_t *)((byte *)surface->tangents + vec3Bytes);
	surface->normals = (vec3_t *)((byte *)surface->bitangents + vec3Bytes);
	surface->positions = (vec3_t *)((byte *)surface + tangentBytes + headerBytes);
	surface->texCoords = (float (*)[2])((byte *)surface->positions + vec3Bytes);
	surface->lightmapCoords = (float (*)[2])((byte *)surface->texCoords + vec2Bytes);
	surface->colors = (byte (*)[4])((byte *)surface->lightmapCoords + vec2Bytes);

	surface->surfaceType = ((bspShader_t *)worldSurface->shader)->optimizedBackend;
	surface->storageMode = build->storageMode;
	surface->vertexCount = vertexCount;
	surface->indexCount = indexCount;
	ClearBounds(surface->boundsMin, surface->boundsMax);

	if ( indices[0] )
		ri.Error(1, "\x15" "First index is not 0 in triangle soup surface");
	for ( i = 0; i < indexCount; ++i )
	{
		/* retail reads the still-zero destination index before writing it */
		if ( (short)surface->indices[i] >= vertexCount )
			ri.Error(1, "\x15" "Bad index in triangle soup surface");
		surface->indices[i] = indices[i] + (unsigned short)build->firstVertex;
	}

	LittleVertices_T2T2C4V3((bspDrawVert_t *)verticesArg, surface, vertexCount, placement, build->firstVertex);

	if ( canOptimize && ((bspShader_t *)worldSurface->shader)->numUnfoggedPasses )
		AdjustColors(surface, worldSurface);

	switch ( surface->surfaceType )
	{
		case SF_OPTIMIZED_GENERIC:
			OptimizeVertices_T2T2C4V3_GenericOrNV(surface, ((bspShader_t *)worldSurface->shader)->optimizedVertexStorage
			                                                   + 32 * build->firstVertex);
			break;
		case SF_OPTIMIZED_ARB:
			OptimizeVertices_T2T2C4V3_ARB(surface, ((bspShader_t *)worldSurface->shader)->optimizedVertexStorage,
			                              32 * build->firstVertex);
			break;
		case SF_OPTIMIZED_ATI:
			OptimizeVertices_T2T2C4V3_ATI(surface, ((bspShader_t *)worldSurface->shader)->optimizedVertexStorage,
			                              ((bspShader_t *)worldSurface->shader)->optimizedVertexStorageOffset
			                                  + 32 * build->firstVertex);
			break;
		case SF_OPTIMIZED_NV:
			OptimizeVertices_T2T2C4V3_GenericOrNV(surface, ((bspShader_t *)worldSurface->shader)->optimizedVertexStorage
			                                                   + 32 * build->firstVertex);
			break;
	}

	surface->boundsMin[0] -= ((bspShader_t *)worldSurface->shader)->boundsExpansion;
	surface->boundsMin[1] -= ((bspShader_t *)worldSurface->shader)->boundsExpansion;
	surface->boundsMin[2] -= ((bspShader_t *)worldSurface->shader)->boundsExpansion;
	surface->boundsMax[0] += ((bspShader_t *)worldSurface->shader)->boundsExpansion;
	surface->boundsMax[1] += ((bspShader_t *)worldSurface->shader)->boundsExpansion;
	surface->boundsMax[2] += ((bspShader_t *)worldSurface->shader)->boundsExpansion;

	if ( build->optimized )
		build->firstVertex += vertexCount;
	worldSurface->data = (srfGeneric_t *)surface;
	return 1;
}

/* ---- ParseTriangleSoup  0x004DADF0 ----  [HIGH]
 * Resolve one disk surface's shader (honouring r_singleShader) and build its
 * optimized world surface.  R_LoadSurfaces carries its own copy of this code
 * (retail's inlined block schedules differently from this function's body). */
int __cdecl ParseTriangleSoup(int diskSurfaceArg, int buildArg, int worldSurfaceArg,
                              int placements, int vertexBase, int indexBase)
{
	bspDiskSurface_t *diskSurface = (bspDiskSurface_t *)diskSurfaceArg;
	shaderBuild_t    *build = (shaderBuild_t *)buildArg;
	msurface_t       *worldSurface = (msurface_t *)worldSurfaceArg;
	short             lightmapNum = diskSurface->lightmapNum;
	int               vertexCount;
	bspLmPlacement_t *placement;

	worldSurface->shader = (shader_t *)build->shader;
	if ( r_singleShader->integer )
	{
		if ( (((bspShader_t *)worldSurface->shader)->flags & SHADER_FLAG_SKY) == 0 )
			worldSurface->shader = (shader_t *)tr_defaultShader;
	}

	vertexCount = diskSurface->numVerts;
	if ( lightmapNum >= 0 )
		placement = (bspLmPlacement_t *)(placements + 20 * lightmapNum);
	else
		placement = 0;

	return BuildOptimizedSurface(worldSurface, build, placement,
	                             vertexCount,
	                             vertexBase + 44 * diskSurface->firstVert,
	                             diskSurface->numIndexes,
	                             (unsigned short *)(indexBase + 2 * diskSurface->firstIndex));
}

/* ---- R_LoadSurfaces  0x004DAE60 ----  [CONFIRMED]
 * Group the world's disk surfaces by (shader, merged-lightmap) and build one
 * optimized world surface per group so surfaces sharing a shader and atlas end
 * up in one vertex buffer.  Records the sky surfaces for RB_StageIteratorSky. */
int __cdecl R_LoadSurfaces(int surfaceLumpArg, unsigned int *vertexLumpArg, int indexLumpArg, int placements)
{
	lump_t           *indexLump = (lump_t *)indexLumpArg;
	lump_t           *surfaceLump = (lump_t *)surfaceLumpArg;
	lump_t           *vertexLump = (lump_t *)vertexLumpArg;
	bspDiskSurface_t *diskSurfaces;
	int               vertexBase;
	int               indexBase;
	int               surfaceCount;
	msurface_t       *out;
	int              *groupMarkers;
	int               groupMarker;
	char             *skyShader;
	short             shaderIndex;
	short             lightmapIndex;
	int               i;
	shaderBuild_t     build;
	MSG               msg;

	diskSurfaces = (bspDiskSurface_t *)(bspImageBase + surfaceLump->fileofs);
	if ( (surfaceLump->filelen & 0xF) != 0 )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	surfaceCount = (unsigned int)surfaceLump->filelen >> 4;
	vertexBase = bspImageBase + vertexLump->fileofs;
	if ( vertexLump->filelen % 0x2Cu )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	indexBase = bspImageBase + indexLump->fileofs;
	if ( (indexLump->filelen & 1) != 0 )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);

	s_worldData_numsurfaces = surfaceCount;
	if ( !surfaceCount )
		ri.Error(1, "\x15" "LoadMap: no surfaces in %s", &s_worldData);

	s_worldData_surfaces = ri.Hunk_Alloc(12 * s_worldData_numsurfaces);
	out = (msurface_t *)s_worldData_surfaces;
	groupMarkers = (int *)ri.Hunk_AllocateTempMemory(4 * surfaceCount);
	memset(groupMarkers, 0, 4 * surfaceCount);

	groupMarker = 0;
	skyShader = 0;
	for ( shaderIndex = 0; shaderIndex < s_worldData_numShaders; ++shaderIndex )
	{
		for ( lightmapIndex = -1; lightmapIndex < tr_lightmapCount; ++lightmapIndex )
		{
			int totalVerts;
			int selectedCount;

			++groupMarker;
			totalVerts = 0;
			selectedCount = 0;
			for ( i = 0; i < surfaceCount; ++i )
			{
				short mappedLightmap;

				if ( diskSurfaces[i].shaderNum != shaderIndex )
					continue;
				mappedLightmap = diskSurfaces[i].lightmapNum;
				if ( mappedLightmap >= 0 )
					mappedLightmap = *(short *)(placements + 20 * mappedLightmap);
				if ( mappedLightmap != lightmapIndex )
					continue;

				totalVerts += diskSurfaces[i].numVerts;
				++selectedCount;
				groupMarkers[i] = groupMarker;
			}

			if ( totalVerts )
			{
				char *shader;

				shader = ShaderForShaderNum(shaderIndex, lightmapIndex, (const char *)R_WORLD_SHADER_USAGE);
				BeginShaderSurfaces(totalVerts, (int *)&build, (unsigned int *)shader);

				if ( ((bspShader_t *)shader)->optimalStageIteratorFunc == (void *)RB_StageIteratorSky )
				{
					if ( lightmapIndex != -1 )
						ri.Error(1, "\x15" "sky shader '%s' has a lightmap\n", shader);
					if ( skyShader )
						ri.Error(1, "\x15" "more than one sky shader: at least '%s' and '%s'\n", shader, skyShader);
					skyShader = shader;
					s_worldData_skySurfaces = ri.Hunk_Alloc(4 * selectedCount);
				}

				for ( i = 0; i < surfaceCount; ++i )
				{
					if ( groupMarkers[i] != groupMarker )
						continue;
					{
						/* ParseTriangleSoup's work, written out: retail loads numIndexes with numVerts */
						short             lightmapNum;
						int               vertexCount;
						int               indexCount;
						bspLmPlacement_t *placement;

						lightmapNum = diskSurfaces[i].lightmapNum;
						out[i].shader = (shader_t *)build.shader;
						if ( r_singleShader->integer && (((bspShader_t *)out[i].shader)->flags & SHADER_FLAG_SKY) == 0 )
							out[i].shader = (shader_t *)tr_defaultShader;
						vertexCount = diskSurfaces[i].numVerts;
						indexCount = diskSurfaces[i].numIndexes;
						if ( lightmapNum >= 0 )
							placement = (bspLmPlacement_t *)(placements + 20 * lightmapNum);
						else
							placement = 0;
						BuildOptimizedSurface(&out[i], &build, placement, vertexCount,
						                      vertexBase + 44 * diskSurfaces[i].firstVert, indexCount,
						                      (unsigned short *)(indexBase + 2 * diskSurfaces[i].firstIndex));
					}
					if ( ((bspShader_t *)shader)->optimalStageIteratorFunc == (void *)RB_StageIteratorSky )
						*(unsigned int *)(s_worldData_skySurfaces + 4 * s_worldData_skySurfaceCount++) = (int)&out[i];
				}
			}
		}

		/* Sys_PumpEvents: keep the window responsive and the display awake. */
		if ( PeekMessageA(&msg, 0, 0x218u, 0x218u, 0) )
		{
			if ( GetMessageA(&msg, 0, 0x218u, 0x218u) > 0 )
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
		}
		SetThreadExecutionState(2u);
	}
	return ri.Hunk_FreeTempMemory(groupMarkers);
}

/* ---- R_LoadSubmodels  0x004DB240 ----  [HIGH]
 * Create one renderer model named "*n" per BSP brush model, each owning its
 * contiguous world-surface span. */
void __cdecl R_LoadSubmodels(unsigned int *submodelLumpArg)
{
	lump_t         *l = (lump_t *)submodelLumpArg;
	bspDiskModel_t *in;
	bmodel_t       *out;
	int             i, j, count;

	in = (bspDiskModel_t *)(bspImageBase + l->fileofs);
	if ( l->filelen % sizeof(*in) )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = l->filelen / sizeof(*in);

	s_worldData_bmodels = (int)(out = (bmodel_t *)ri.Hunk_Alloc(count * sizeof(*out)));

	for ( i = 0; i < count; i++, in++, out++ )
	{
		model_t *model;

		/* ORIGINAL BUG: R_AllocModel returns NULL when the model registry is
		 * full and the stores below dereference it. */
		model = (model_t *)R_AllocModel();
		model->type = 1;                 /* MOD_BRUSH */
		model->bmodel = out;
		Com_sprintf(model->name, sizeof(model->name), "*%d", i);

		for ( j = 0; j < 3; j++ )
		{
			out->bounds[0][j] = FloatNoSwap(in->mins[j]);
			out->bounds[1][j] = FloatNoSwap(in->maxs[j]);
		}

		out->firstSurface = (msurface_t *)s_worldData_surfaces + LongNoSwap(in->firstSurface);
		out->numSurfaces = LongNoSwap(in->numSurfaces);
	}
}

/* ---- R_SetParentAndCell  0x004DB370 ----  VERIFIED
 * Recursively set each node's parent and derive its DPVS cell: an internal node
 * inherits a concrete cell only when both subtrees resolve to the same cell. */
void __cdecl R_SetParentAndCell(mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if ( node->contents != NODE_INTERNAL )
		return;
	R_SetParentAndCell(node->u.node.children[0], node);
	R_SetParentAndCell(node->u.node.children[1], node);
	node->cellIndex = -2;
	if ( node->u.node.children[0]->cellIndex == node->u.node.children[1]->cellIndex )
		node->cellIndex = node->u.node.children[0]->cellIndex;
}

/* ---- R_LoadNodesAndLeafs  0x004DB3C0 ----  [HIGH]
 * Build the render BSP: internal nodes (plane + resolved child pointers) and
 * leaves (cell, cluster, and the indexed-light span, with a leading sun light
 * split off), then resolve parents and cells from the root down. */
void __cdecl R_LoadNodesAndLeafs(unsigned int *nodeLumpArg, unsigned int *leafLumpArg)
{
	lump_t        *nodeLump = (lump_t *)nodeLumpArg;
	lump_t        *leafLump = (lump_t *)leafLumpArg;
	int            i, j, p;
	bspDiskNode_t *in;
	bspDiskLeaf_t *inLeaf;
	mnode_t       *out;
	int            numNodes, numLeafs;

	in = (bspDiskNode_t *)(bspImageBase + nodeLump->fileofs);
	if ( nodeLump->filelen % sizeof(bspDiskNode_t) || leafLump->filelen % sizeof(bspDiskLeaf_t) )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	numNodes = nodeLump->filelen / sizeof(bspDiskNode_t);
	numLeafs = leafLump->filelen / sizeof(bspDiskLeaf_t);

	out = (mnode_t *)ri.Hunk_Alloc((numNodes + numLeafs) * sizeof(*out));
	s_worldData_nodes = (int)out;
	s_worldData_numnodes = numNodes + numLeafs;
	s_worldData_numDecisionNodes = numNodes;

	/* load nodes */
	for ( i = 0; i < numNodes; i++, in++, out++ )
	{
		out->u.node.plane = (cplane_t *)ri.CM_GetPlaneNum(LongNoSwap(in->planeNum));
		out->contents = NODE_INTERNAL;

		for ( j = 0; j < 2; j++ )
		{
			p = LongNoSwap(in->children[j]);
			if ( p >= 0 )
				out->u.node.children[j] = (mnode_t *)s_worldData_nodes + p;
			else
				out->u.node.children[j] = (mnode_t *)s_worldData_nodes + numNodes + (-1 - p);
		}
	}

	/* load leafs: cell, cluster and the indexed-light span, with a leading
	 * sun light split off */
	inLeaf = (bspDiskLeaf_t *)(bspImageBase + leafLump->fileofs);
	for ( i = 0; i < numLeafs; i++, inLeaf++, out++ )
	{
		out->cellIndex = LongNoSwap(inLeaf->cellNum);
		out->u.leaf.cluster = LongNoSwap(inLeaf->cluster);
		if ( out->u.leaf.cluster >= s_worldData_numClusters )
			s_worldData_numClusters = out->u.leaf.cluster + 1;

		out->u.leaf.firstLightIndex = LongNoSwap(inLeaf->firstLightIndex);
		out->u.leaf.lightCount = LongNoSwap(inLeaf->lightCount);
		out->u.leaf.hasSunLight = 0;
		if ( out->u.leaf.lightCount && ((short *)s_worldData_lightIndexes)[out->u.leaf.firstLightIndex] < 0 )
		{
			out->u.leaf.firstLightIndex++;
			out->u.leaf.lightCount--;
			out->u.leaf.hasSunLight = 1;
		}
		if ( out->u.leaf.lightCount > 15 )
			ri.Error(1, "\x15" "R_LoadNodesAndLeafs: too many lights in leaf. The map needs to be recompiled.");
	}

	/* chain descendants */
	R_SetParentAndCell((mnode_t *)s_worldData_nodes, 0);
}

/* ---- R_LoadShaders  0x004DB5A0 ----  VERIFIED
 * Copy the disk shader table into the world data verbatim (on little-endian
 * targets the byte-swap pass is a no-op). */
void __cdecl R_LoadShaders(unsigned int *shaderLumpArg)
{
	lump_t          *shaderLump = (lump_t *)shaderLumpArg;
	bspDiskShader_t *diskShaders = (bspDiskShader_t *)(bspImageBase + shaderLump->fileofs);
	bspDiskShader_t *out;
	int              count;
	int              i;

	if ( shaderLump->filelen % 0x48u )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = shaderLump->filelen / 0x48u;

	out = (bspDiskShader_t *)ri.Hunk_Alloc(72 * count);
	s_worldData_shaders = (int)out;
	s_worldData_numShaders = count;
	memcpy(out, diskShaders, 72 * count);

	for ( i = 0; i < count; ++i )
	{
		out[i].surfaceFlags = LongNoSwap(out[i].surfaceFlags);
		out[i].contentFlags = LongNoSwap(out[i].contentFlags);
	}
}

/* retail R_LoadLights reads its luminance weights {0.299, 0.587, 0.114} from
 * .rdata 0x005405F8, inside tr_debug.c's colorDebugPortal block */
#define s_luminanceWeights ( (const float *)&colorDebugPortal[36] )

/* ---- R_LoadLights  0x004DB620 ----  [HIGH]
 * Convert the disk light table into the runtime light records: normalize the
 * colour, split the ambient/diffuse by type, and fill the per-type position,
 * attenuation and spot parameters.  Records the sun light. */
void __cdecl R_LoadLights(unsigned int *lightLumpArg)
{
	lump_t         *lightLump = (lump_t *)lightLumpArg;
	bspDiskLight_t *diskLights = (bspDiskLight_t *)(bspImageBase + lightLump->fileofs);
	dlight_t       *lights;
	int             count;
	int             i;

	if ( lightLump->filelen % 0x48u )
		ri.Error(1, "\x15" "R_LoadLights: funny lump size in %s", &s_worldData);
	count = lightLump->filelen / 0x48u;

	/* typed pointer straight from the allocation: an int copy moves VC7's loop base off the record start */
	lights = (dlight_t *)ri.Hunk_Alloc(136 * count);
	(*(int *)(((char *)(void *)&s_worldData) + 276)) = 0;
	(*(int *)(((char *)(void *)&s_worldData) + 272)) = (int)lights;
	(*(int *)(((char *)(void *)&s_worldData) + 268)) = count;

	for ( i = count; i > 0; --i, ++diskLights, ++lights )
	{
		bspDiskLight_t *disk = diskLights;
		dlight_t       *light = lights;
		vec3_t          color;
		float           scale;

		light->type = disk->type;
		color[0] = FloatNoSwap(disk->color[0]) * tr_identityLight;
		color[1] = FloatNoSwap(disk->color[1]) * tr_identityLight;
		color[2] = FloatNoSwap(disk->color[2]) * tr_identityLight;
		light->intensity = s_luminanceWeights[2] * color[2] + s_luminanceWeights[1] * color[1] + s_luminanceWeights[0] * color[0];
		scale = light->intensity;
		if ( scale != 0.0f )
			scale = 1.0f / scale;
		VectorScale( color, scale, light->color );

		if ( light->type == 1 )
		{
			light->ambient[0] = 0.0f;
			light->ambient[1] = 0.0f;
			light->ambient[2] = 0.0f;
			light->ambient[3] = 0.0f;
			light->diffuse[0] = (*(float *)(((char *)(void *)&s_worldData) + 232));
			light->diffuse[1] = (*(float *)(((char *)(void *)&s_worldData) + 236));
			light->diffuse[2] = (*(float *)(((char *)(void *)&s_worldData) + 240));
			*(int *)&light->diffuse[3] = (*(int *)(((char *)(void *)&s_worldData) + 244));
			/* (w2*d2 + w0*d0) + w1*d1, retail 0x004DB750 */
			light->intensity = s_luminanceWeights[1] * light->diffuse[1]
			                 + ( s_luminanceWeights[2] * light->diffuse[2] + s_luminanceWeights[0] * light->diffuse[0] );
			(*(int *)(((char *)(void *)&s_worldData) + 276)) = (int)light;
			(*(float *)(((char *)(void *)&s_worldData) + 264)) = (*(float *)(((char *)(void *)&s_worldData) + 256)) * s_luminanceWeights[2]
			                                    + (*(float *)(((char *)(void *)&s_worldData) + 252)) * s_luminanceWeights[1]
			                                    + (*(float *)(((char *)(void *)&s_worldData) + 248)) * s_luminanceWeights[0];
		}
		else
		{
			light->ambient[3] = 1.0f;
			light->diffuse[3] = 1.0f;
			VectorScale( color, 0.1f, light->ambient );
			VectorScale( color, 0.80000001f, light->diffuse );
		}

		light->specular[0] = 0.0f;
		light->specular[1] = 0.0f;
		light->specular[2] = 0.0f;
		light->specular[3] = 1.0f;
		light->constantAttenuation = 0.0f;
		light->linearAttenuation = 0.0f;
		light->quadraticAttenuation = 0.0f;
		light->spotCutoff = 180.0f;
		light->spotExponent = 0.0f;

		switch ( light->type )
		{
			case 1:
				light->position[0] = FloatNoSwap(disk->direction[0]);
				light->position[1] = FloatNoSwap(disk->direction[1]);
				light->position[2] = FloatNoSwap(disk->direction[2]);
				light->position[3] = 0.0f;
				light->constantAttenuation = 1.0f;
				break;
			case 2:
				light->position[0] = FloatNoSwap(disk->position[0]);
				light->position[1] = FloatNoSwap(disk->position[1]);
				light->position[2] = FloatNoSwap(disk->position[2]);
				light->position[3] = 1.0f;
				light->quadraticAttenuation = 1.0f;
				break;
			case 3:
				light->position[0] = FloatNoSwap(disk->position[0]);
				light->position[1] = FloatNoSwap(disk->position[1]);
				light->position[2] = FloatNoSwap(disk->position[2]);
				light->position[3] = 1.0f;
				light->linearAttenuation = FloatNoSwap(disk->params.linearPoint.linearAttenuation);
				break;
			case 4:
				light->position[0] = FloatNoSwap(disk->position[0]);
				light->position[1] = FloatNoSwap(disk->position[1]);
				light->position[2] = FloatNoSwap(disk->position[2]);
				light->position[3] = 1.0f;
				light->quadraticAttenuation = FloatNoSwap(disk->params.customPoint.quadraticAttenuation);
				light->constantAttenuation = FloatNoSwap(disk->params.customPoint.constantAttenuation);
				break;
			case 5:
				light->position[0] = FloatNoSwap(disk->position[0]);
				light->position[1] = FloatNoSwap(disk->position[1]);
				light->position[2] = FloatNoSwap(disk->position[2]);
				light->position[3] = 1.0f;
				light->quadraticAttenuation = 1.0f;
				light->spotDirection[0] = -FloatNoSwap(disk->direction[0]);
				light->spotDirection[1] = -FloatNoSwap(disk->direction[1]);
				light->spotDirection[2] = -FloatNoSwap(disk->direction[2]);
				light->spotCutoff = acos(disk->params.spot.cutoffCos) * 57.29577791868205;
				light->spotExponent = (double)disk->params.spot.exponent;
				break;
			case 7:
				light->position[0] = FloatNoSwap(disk->position[0]);
				light->position[1] = FloatNoSwap(disk->position[1]);
				light->position[2] = FloatNoSwap(disk->position[2]);
				light->position[3] = 1.0f;
				light->quadraticAttenuation = FloatNoSwap(disk->params.customSpot.quadraticAttenuation);
				light->constantAttenuation = FloatNoSwap(disk->params.customSpot.constantAttenuation);
				light->spotDirection[0] = -FloatNoSwap(disk->direction[0]);
				light->spotDirection[1] = -FloatNoSwap(disk->direction[1]);
				light->spotDirection[2] = -FloatNoSwap(disk->direction[2]);
				light->spotCutoff = acos(disk->params.customSpot.cutoffCos) * 57.29577791868205;
				light->spotExponent = (double)disk->params.customSpot.exponent;
				break;
			default:
				break;
		}
	}
}

/* ---- R_LoadLightIndexes  0x004DB980 ----  VERIFIED */
void __cdecl R_LoadLightIndexes(int lightIndexLumpArg)
{
	lump_t         *lightIndexLump = (lump_t *)lightIndexLumpArg;
	const unsigned short    *diskIndexes = (const unsigned short *)(bspImageBase + lightIndexLump->fileofs);
	int             count;
	unsigned short          *result;
	int             i;

	if ( (lightIndexLump->filelen & 1) != 0 )
		ri.Error(1, "\x15" "R_LoadLightIndexes: funny lump size in %s", &s_worldData);
	count = (unsigned int)lightIndexLump->filelen >> 1;

	result = (unsigned short *)ri.Hunk_Alloc(2 * count);
	s_worldData_lightIndexes = (int)result;
	s_worldData_lightIndexCount = count;
	for ( i = 0; i < count; ++i )
		result[i] = diskIndexes[i];
}

/* ---- R_LoadLightVisCache  0x004DB9E0 ----  VERIFIED
 * Restore the precompiled leaf/light visibility cache when the lump is present.
 * Retail inlines this into RE_LoadWorldMap; the standalone body is retained. */
int __cdecl R_LoadLightVisCache(int *lightVisLumpArg)
{
	lump_t *lightVisLump = (lump_t *)lightVisLumpArg;
	int     buffer = bspImageBase + lightVisLump->fileofs;
	int     result = lightVisLump->filelen;

	if ( result > 0 )
	{
		result = R_InitLightVisCacheFromBuffer(buffer, result);
		if ( !result )
			return ri.Error(1, "\x15" "R_LoadLightVisCache: funny lump size in %s", &s_worldData);
	}
	return result;
}

/* ---- R_ValueForKey  0x004DBA20 ----  VERIFIED
 * Return an entity's value for a key (case-insensitive), searching from pair 1
 * since pair 0 is always the classname. */
int __cdecl R_ValueForKey(int pairCount, char *key, int pairsArg)
{
	entityPair_t *pairs = (entityPair_t *)pairsArg;
	int           i;

	for ( i = 1; i < pairCount; ++i )
	{
		if ( !_stricmp(pairs[i].key, key) )
			return (int)pairs[i].value;
	}
	return 0;
}

/* ---- R_FloatForKey  0x004DBA80 ----  VERIFIED */
rfloat __cdecl R_FloatForKey(char *key, int pairs, int pairCount, float defaultValue)
{
	const char *value = (const char *)R_ValueForKey(pairCount, key, pairs);
	if ( !value )
		return defaultValue;
	return atof(value);
}

/* ---- R_VectorForKey  0x004DBAA0 ----  VERIFIED */
int __cdecl R_VectorForKey(char *key, int pairs, int pairCount, float *out, const char *defaultValue)
{
	int         found = 1;
	const char *value = (const char *)R_ValueForKey(pairCount, key, pairs);

	if ( !value )
	{
		found = 0;
		value = defaultValue;
	}
	sscanf(value, "%f %f %f", out, out + 1, out + 2);
	return found;
}

/* ---- R_LoadMiscModel  0x004DBAE0 ----  [HIGH]
 * Spawn a static model for one misc_model entity (origin, angles, scale and
 * precalculated lighting), skipping shadow-blob models. */
void __cdecl R_LoadMiscModel(int pairCount, int pairs)
{
	char   *model;
	rfloat  angle;
	float   angles[3];
	float   origin[3];
	float   scale[3];
	float   lightingPrecalc[3];

	if ( !R_VectorForKey("origin", pairs, pairCount, origin, "0 0 0") )
		ri.Error(1, "\x15" "R_LoadMiscModel: no origin specified\n");

	model = (char *)R_ValueForKey(pairCount, "model", pairs);
	if ( !model )
		ri.Error(1, "\x15" "R_LoadMiscModel: no model specified in misc_model at (%.0f %.0f %.0f)\n",
		         origin[0], origin[1], origin[2]);

	if ( !_strnicmp(model, "xmodel/shadow_", 0xEu) )
		return;

	angle = R_FloatForKey("angle", pairs, pairCount, 0.0f);
	if ( angle != 0.0f )
	{
		angles[1] = angle;
		angles[0] = 0;
		angles[2] = 0;
	}
	else
	{
		R_VectorForKey("angles", pairs, pairCount, angles, "0 0 0");
	}

	if ( !R_VectorForKey("modelscale_vec", pairs, pairCount, scale, "1 1 1") )
	{
		angle = R_FloatForKey("modelscale", pairs, pairCount, 1.0f);
		scale[0] = angle;
		scale[1] = angle;
		scale[2] = angle;
	}

	R_VectorForKey("lightingPrecalc", pairs, pairCount, lightingPrecalc, "1 1 1");

	R_CreateStaticModel((int)&s_worldData, model, origin, angles, scale, lightingPrecalc);
}

/* ---- R_LoadCorona  0x004DBCD0 ----  VERIFIED
 * Parse one corona entity into the world corona table. */
void __cdecl R_LoadCorona(int pairCount, int pairs)
{
	bspCorona_t *corona;
	float        scale;
#ifndef COD1_VC7
	rfloat       scaleValue;
#endif
	float        color[3];

	if ( s_worldData_coronaCount >= R_MAX_MAP_CORONAS )
		ri.Error(1, va("\x15" "MAX_MAP_CORONAS(%i) exceeded", R_MAX_MAP_CORONAS));

	corona = &((bspCorona_t *)s_worldData_coronas)[s_worldData_coronaCount++];
	corona->shader = R_FindShader("flareShader", -1, 1, (const char *)R_CORONA_SHADER_USAGE);
	R_VectorForKey("origin", pairs, pairCount, corona->origin, "0 0 0");

	/* retail tests the unrounded atof double (fst to scale, fcomp 0.0f) */
#ifdef COD1_VC7
	scale = R_FloatForKey("scale", pairs, pairCount, 1.0f);
	if ( scale <= 0.0f )
#else
	scaleValue = R_FloatForKey("scale", pairs, pairCount, 1.0f);
	scale = scaleValue;
	if ( scaleValue <= 0.0f )
#endif
		ri.Error(1, "\x15" "corona scale must be > 0");
	corona->scale = scale * 25.5f;
	corona->zCutoff = R_FloatForKey("zcutoff", pairs, pairCount, -0.15f);
	corona->zFadeOut = R_FloatForKey("zfadeout", pairs, pairCount, -0.25f);

	R_VectorForKey("dl_color", pairs, pairCount, color, "1 1 1");
	corona->color[0] = (byte)min(255.0f, max(0.0f, (rfloat)color[0] * 255.0f));
	corona->color[1] = (byte)min(255.0f, max(0.0f, (rfloat)color[1] * 255.0f));
	corona->color[2] = (byte)min(255.0f, max(0.0f, (rfloat)color[2] * 255.0f));
	corona->color[3] = 255;
}

/* ---- R_LoadEntities  0x004DBF50 ----  VERIFIED
 * Parse the entity lump: read the worldspawn lighting keys, then spawn the
 * misc_model and corona entities.  Coronas are collected into a temporary table
 * and copied to the hunk at the end. */
void __cdecl R_LoadEntities(unsigned int *entityLumpArg)
{
	lump_t *entityLump = (lump_t *)entityLumpArg;
	char   *data_p;
	char   *token;
	char    ambientKey[SPAWN_VAR_CHARS];
	char    ambientValue[SPAWN_VAR_CHARS];
	int     tempCoronas[4096];
	entityPair_t pairs[R_MAX_SPAWN_VARS];
	float   ambient;
	float   entityAmbientColor[3];
	float   diffuseFraction;
	float   sunColor[3];
	float   sunDiffuseColor[3];
	int     haveSunDiffuse;
	float   sunlight;
	float   sunDirection[3];
	rfloat  sunScale;
#ifndef COD1_VC7
	rfloat  ambientParsed;
#endif
	int     pairCount;

	data_p = (char *)(bspImageBase + entityLump->fileofs);
	entityParseBufferHead = ri.Hunk_Alloc(entityLump->filelen + 1);
	strcpy((char *)entityParseBufferHead, data_p);   /* ORIGINAL BUG: unbounded copy */
	entityParseCursor = (char *)entityParseBufferHead;

	token = R_ParseWorldToken(&data_p);
	if ( !*token || *token != '{' )
		return;

	ambient = 0;
	haveSunDiffuse = 0;
	sunlight = 1.0f;
	diffuseFraction = 0.5f;
	VectorClear(entityAmbientColor);
	VectorClear(sunColor);
	VectorClear(sunDiffuseColor);

	while ( 1 )
	{
		token = R_ParseWorldToken(&data_p);
		if ( !*token || *token == '}' )
			break;
		strncpy(ambientKey, token, 0x7FFu);
		ambientKey[2047] = 0;

		token = R_ParseWorldToken(&data_p);
		if ( !*token || *token == '}' )
			break;
		strncpy(ambientValue, token, 0x7FFu);
		ambientValue[2047] = 0;

		if ( !Q_stricmpn(ambientKey, "ambient", 99999) )
		{
			/* retail tests the unrounded atof double (fst to ambient, fcomp 2.0f) */
#ifdef COD1_VC7
			ambient = atof(ambientValue);
			if ( ambient > 2.0f )
#else
			ambientParsed = atof(ambientValue);
			ambient = ambientParsed;
			if ( ambientParsed > 2.0f )
#endif
			{
				ri.Printf(0, "^3WARNING: ambient too big, assuming it uses the old 0-255 scale instead of the proper 0-1 scale (value = '%s')\n",
				          ambientValue);
				ambient = ambient * 0.015686275f;
			}
		}
		else if ( !Q_stricmpn(ambientKey, "_color", 99999) )
		{
			sscanf(ambientValue, "%f %f %f", &entityAmbientColor[0], &entityAmbientColor[1], &entityAmbientColor[2]);
		}
		else if ( !Q_stricmpn(ambientKey, "diffuseFraction", 99999) )
		{
			diffuseFraction = atof(ambientValue);
		}
		else if ( !Q_stricmpn(ambientKey, "suncolor", 99999) )
		{
			sscanf(ambientValue, "%f %f %f", &sunColor[0], &sunColor[1], &sunColor[2]);
			NormalizeColorWhite_m(sunColor, sunColor);
		}
		else if ( !Q_stricmpn(ambientKey, "sundiffusecolor", 99999) )
		{
			sscanf(ambientValue, "%f %f %f", &sunDiffuseColor[0], &sunDiffuseColor[1], &sunDiffuseColor[2]);
			NormalizeColorWhite_m(sunDiffuseColor, sunDiffuseColor);
			haveSunDiffuse = 1;
		}
		else if ( !Q_stricmpn(ambientKey, "sunlight", 99999) )
		{
			sunlight = atof(ambientValue);
		}
		else if ( !Q_stricmpn(ambientKey, "sundirection", 99999) )
		{
			sscanf(ambientValue, "%f %f %f", &sunDirection[0], &sunDirection[1], &sunDirection[2]);
			AngleVectors(sunDirection, (float *)&tr_sunDirection, 0, 0);
		}
	}

	*(float *)&s_worldData_entityAmbientBaseA = 1.0f;
	if ( ambient != 0.0f && NormalizeColorWhite_m(entityAmbientColor, entityAmbientColor) != 0.0f )
	{
		VectorScale(entityAmbientColor, tr_identityLight * ambient, &s_worldData_entityAmbientBaseR);
		glLightModelfv(0xB53u, &s_worldData_entityAmbientBaseR);
	}
	if ( !haveSunDiffuse )
	{
		sunDiffuseColor[0] = sunColor[0];
		sunDiffuseColor[1] = sunColor[1];
		sunDiffuseColor[2] = sunColor[2];
	}

	*(float *)&s_worldData_sunDiffuseColorA = 1.0f;
	*(float *)&s_worldData_entityAmbientScaleA = 1.0f;
	s_worldData_coronas = (int)tempCoronas;
	s_worldData_coronaCount = 0;
	sunScale = (sunlight - ambient) * tr_identityLight;
	VectorScale(sunColor, ((rfloat)1.0f - diffuseFraction) * sunScale, &s_worldData_sunDiffuseColorR);
	VectorScale(sunDiffuseColor, sunScale * diffuseFraction, &s_worldData_entityAmbientScaleR);

	while ( 1 )
	{
		token = R_ParseWorldToken(&data_p);
		if ( !*token || *token != '{' )
			break;

		pairs[0].key[0] = 0;
		pairCount = 1;
		while ( 1 )
		{
			token = R_ParseWorldToken(&data_p);
			if ( !*token || *token == '}' )
				break;
			if ( !strcmp(token, "classname") )
			{
				strcpy(pairs[0].key, token);
				strcpy(pairs[0].value, R_ParseWorldToken(&data_p));
			}
			else
			{
				if ( pairCount == R_MAX_SPAWN_VARS )
					ri.Error(1, "\x15" "R_LoadEntities: MAX_SPAWN_VARS (%i) reached\n", R_MAX_SPAWN_VARS);
				strcpy(pairs[pairCount].key, token);
				strcpy(pairs[pairCount].value, R_ParseWorldToken(&data_p));
				++pairCount;
			}
		}

		if ( !pairs[0].key[0] )
			ri.Error(1, "\x15" "R_LoadEntities: entity without a classname\n");
		if ( !_stricmp(pairs[0].value, "misc_model") )
			R_LoadMiscModel(pairCount, (int)pairs);
		else if ( !_stricmp(pairs[0].value, "corona") )
			R_LoadCorona(pairCount, (int)pairs);
	}

	if ( s_worldData_coronaCount )
	{
		s_worldData_coronas = ri.Hunk_Alloc(32 * s_worldData_coronaCount);
		Com_Memcpy((void *)s_worldData_coronas, tempCoronas, 32 * s_worldData_coronaCount);
	}
	else
	{
		s_worldData_coronas = 0;
	}
}

/* ---- R_GetEntityToken  0x004DC6D0 ----  VERIFIED
 * Renderer export: hand the game one entity token at a time from the copied
 * entity string, rewinding to the start once exhausted. */
int __cdecl R_GetEntityToken(char *buffer, int bufferSize)
{
	char *token;

	if ( parseInfo->ungetReady )
	{
		parseInfo->ungetReady = qfalse;
		entityParseCursor = parseInfo->ungetTokenSave;
		parseInfo->currentLine = parseInfo->ungetLineSave;
	}
	token = Com_ParseExt(&entityParseCursor, qtrue);
	strncpy(buffer, token, bufferSize - 1);
	buffer[bufferSize - 1] = 0;
	if ( entityParseCursor && *token )
		return 1;
	entityParseCursor = (char *)entityParseBufferHead;
	return 0;
}

/* ---- R_FinishLoadingAABBTrees_r  0x004DC760 ----  [HIGH]
 * Recursively resolve each AABB tree node's child span and derive its bounds
 * (union of child bounds for internal nodes, of surface bounds for leaves).
 * Returns the next free tree index. */
int __cdecl R_FinishLoadingAABBTrees_r(worldAabbTree_t *tree, int nextTreeIndex)
{
	int              i;

	ClearBounds( tree->mins, tree->maxs );

	if ( !tree->childCount )
	{
		for ( i = 0; i < tree->surfaceCount; ++i )
		{
			srfLit_t *data = (srfLit_t *)tree->surfaces[i].data;
			ExpandBounds(data->boundsMin, tree->mins, data->boundsMax, tree->maxs);
		}
	}
	else
	{
		tree->children = &((worldAabbTree_t *)(*(int *)(((char *)(void *)&s_worldData) + 176)))[nextTreeIndex];
		nextTreeIndex += tree->childCount;
		for ( i = 0; i < tree->childCount; ++i )
		{
			nextTreeIndex = R_FinishLoadingAABBTrees_r(&tree->children[i], nextTreeIndex);
			ExpandBounds(tree->children[i].mins, tree->mins, tree->children[i].maxs, tree->maxs);
		}
	}
	return nextTreeIndex;
}

/* ---- R_LoadAABBTrees  0x004DC850 ----  VERIFIED
 * Load the per-cell surface AABB trees; the disk records encode child ownership
 * by depth-first order, resolved by R_FinishLoadingAABBTrees_r. */
int __cdecl R_LoadAABBTrees(unsigned int *aabbTreeLumpArg)
{
	lump_t            *aabbTreeLump = (lump_t *)aabbTreeLumpArg;
	bspDiskAabbTree_t *diskTrees = (bspDiskAabbTree_t *)(bspImageBase + aabbTreeLump->fileofs);
	worldAabbTree_t   *trees;
	int                count;
	int                result;
	int                i;

	if ( aabbTreeLump->filelen % 0xCu )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = aabbTreeLump->filelen / 0xCu;

	trees = (worldAabbTree_t *)ri.Hunk_Alloc(40 * count);
	s_worldData_aabbTrees = (int)trees;
	s_worldData_aabbTreeCount = count;
	for ( i = 0; i < count; ++i )
	{
		trees[i].surfaces = &((msurface_t *)s_worldData_surfaces)[diskTrees[i].firstSurface];
		trees[i].surfaceCount = diskTrees[i].surfaceCount;
		trees[i].childCount = diskTrees[i].childCount;
	}

	for ( result = 0; result < count; result = R_FinishLoadingAABBTrees_r((worldAabbTree_t *)s_worldData_aabbTrees + result, result + 1) )
		;
	return result;
}

/* ---- R_LoadCells  0x004DC910 ----  VERIFIED
 * Load the DPVS cells, resolving each authored index into a runtime pointer.
 * The portal index is left as a byte offset for R_LoadPortals to resolve once
 * the portal table exists. */
void __cdecl R_LoadCells(unsigned int *cellLumpArg)
{
	lump_t        *cellLump = (lump_t *)cellLumpArg;
	bspDiskCell_t *diskCells = (bspDiskCell_t *)(bspImageBase + cellLump->fileofs);
	worldCell_t   *cells;
	int            count;
	int            result;
	int            i;

	if ( cellLump->filelen % 0x34u )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = cellLump->filelen / 0x34u;

	result = ri.Hunk_Alloc(count << 6);
	cells = (worldCell_t *)result;
	(*(int *)(((char *)(void *)&s_worldData) + 292)) = result;
	(*(int *)(((char *)(void *)&s_worldData) + 288)) = count;
	for ( i = 0; i < count; ++i )
	{
		cells[i].mins[0] = FloatNoSwap(diskCells[i].mins[0]);
		cells[i].mins[1] = FloatNoSwap(diskCells[i].mins[1]);
		cells[i].mins[2] = FloatNoSwap(diskCells[i].mins[2]);
		cells[i].maxs[0] = FloatNoSwap(diskCells[i].maxs[0]);
		cells[i].maxs[1] = FloatNoSwap(diskCells[i].maxs[1]);
		cells[i].maxs[2] = FloatNoSwap(diskCells[i].maxs[2]);
		cells[i].aabbTree = &((worldAabbTree_t *)(*(int *)(((char *)(void *)&s_worldData) + 176)))[diskCells[i].aabbTreeIndex];
		cells[i].portalRef = (*(int *)(((char *)(void *)&s_worldData) + 316)) + 36 * diskCells[i].firstPortal;
		cells[i].portalCount = diskCells[i].portalCount;
		cells[i].cullGroups = &((worldCullGroup_t **)(*(int *)(((char *)(void *)&s_worldData) + 336)))[diskCells[i].firstCullGroup];
		cells[i].cullGroupCount = diskCells[i].cullGroupCount;
		cells[i].occluders = &((worldOccluder_t **)(*(int *)(((char *)(void *)&s_worldData) + 308)))[diskCells[i].firstOccluder];
		cells[i].occluderCount = diskCells[i].occluderCount;
	}
	return;
}

/* ---- R_LoadPortalVerts  0x004DCA00 ----  VERIFIED */
void __cdecl R_LoadPortalVerts(unsigned int *portalVertexLumpArg)
{
	lump_t *portalVertexLump = (lump_t *)portalVertexLumpArg;
	vec3_t *diskVerts = (vec3_t *)(bspImageBase + portalVertexLump->fileofs);
	vec3_t *verts;
	int     count;
	int     i;

	if ( portalVertexLump->filelen % 0xCu )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = portalVertexLump->filelen / 0xCu;

	verts = (vec3_t *)ri.Hunk_Alloc(12 * count);
	s_worldData_portalVerts = (int)verts;
	for ( i = 0; i < count; ++i )
	{
		verts[i][0] = FloatNoSwap(diskVerts[i][0]);
		verts[i][1] = FloatNoSwap(diskVerts[i][1]);
		verts[i][2] = FloatNoSwap(diskVerts[i][2]);
	}
}

/* ---- R_LoadPortals  0x004DCA80 ----  VERIFIED
 * Load the DPVS portals (plane, destination cell, and vertex span), then patch
 * each cell's first-portal byte offset into a real portal pointer. */
void __cdecl R_LoadPortals(int portalLumpArg)
{
	lump_t          *portalLump = (lump_t *)portalLumpArg;
	bspDiskPortal_t *diskPortals = (bspDiskPortal_t *)(bspImageBase + portalLump->fileofs);
	worldPortal_t   *portals;
	int              count;
	int              i;

	if ( (portalLump->filelen & 0xF) != 0 )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = (unsigned int)portalLump->filelen >> 4;

	portals = (worldPortal_t *)ri.Hunk_Alloc(36 * count);
	s_worldData_portals = (int)portals;
	s_worldData_portalCount = count;
	for ( i = 0; i < count; ++i )
	{
		bspDpvsPlane_t *plane = &portals[i].plane;
		cplane_t       *collisionPlane = (cplane_t *)ri.CM_GetPlaneNum(diskPortals[i].planeIndex);

		plane->normal[0] = collisionPlane->normal[0];
		plane->normal[1] = collisionPlane->normal[1];
		plane->normal[2] = collisionPlane->normal[2];
		plane->distance = collisionPlane->dist;
		R_SetPlaneSidesDPVS((int)plane);
		portals[i].cell = &((worldCell_t *)s_worldData_cells)[diskPortals[i].cellIndex];
		portals[i].vertices = &((vec3_t *)s_worldData_portalVerts)[diskPortals[i].firstVertex];
		portals[i].vertexCount = diskPortals[i].vertexCount;
		portals[i].recursionActive = 0;
	}

	for ( i = 0; i < s_worldData_cellCount; ++i )
		((worldCell_t *)s_worldData_cells)[i].portalRef = s_worldData_portals
		    + 36 * (((worldCell_t *)s_worldData_cells)[i].portalRef / 36);
}

/* ---- R_LoadCullGroups  0x004DCBD0 ----  [HIGH] */
void __cdecl R_LoadCullGroups(int cullGroupLumpArg)
{
	lump_t             *l = (lump_t *)cullGroupLumpArg;
	bspDiskCullGroup_t *in;
	worldCullGroup_t   *out;
	int                 count;
	int                 i, j;

	in = (bspDiskCullGroup_t *)(bspImageBase + l->fileofs);
	if ( l->filelen % sizeof( *in ) )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = l->filelen / sizeof( *in );

	out = (worldCullGroup_t *)ri.Hunk_Alloc(count * sizeof( *out ));
	(*(int *)(((char *)(void *)&s_worldData) + 328)) = (int)out;    /* s_worldData.cullGroups */
	(*(int *)(((char *)(void *)&s_worldData) + 324)) = count;       /* s_worldData.numCullGroups */
	for ( i = 0; i < count; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			out[i].mins[j] = FloatNoSwap( in[i].mins[j] );
			out[i].maxs[j] = FloatNoSwap( in[i].maxs[j] );
		}
		out[i].surfaces = &((msurface_t *)(*(int *)(((char *)(void *)&s_worldData) + 160)))[in[i].firstSurface];
		out[i].surfaceCount = in[i].surfaceCount;
	}
}

/* ---- R_LoadCullGroupIndexes  0x004DCC90 ----  VERIFIED */
void __cdecl R_LoadCullGroupIndexes(int indexLumpArg)
{
	lump_t             *indexLump = (lump_t *)indexLumpArg;
	const int          *diskIndexes = (const int *)(bspImageBase + indexLump->fileofs);
	worldCullGroup_t  **result;
	int                 count;
	int                 i;

	if ( (indexLump->filelen & 3) != 0 )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = (unsigned int)indexLump->filelen >> 2;

	result = (worldCullGroup_t **)ri.Hunk_Alloc(4 * count);
	s_worldData_cullGroupIndexes = (int)result;
	s_worldData_cullGroupIndexCount = count;
	for ( i = 0; i < count; ++i )
		result[i] = &((worldCullGroup_t *)s_worldData_cullGroups)[diskIndexes[i]];
}

/* ---- R_LoadOccluders  0x004DCD00 ----  [HIGH]
 * Load the DPVS occluders from three lumps: the occluder records select
 * contiguous spans from the plane-index, edge and portal-vertex lumps, and each
 * edge resolves to two of the occluder's planes and two of its vertices. */
void __cdecl R_LoadOccluders(unsigned int *occluderLumpArg, int planeIndexLumpArg, int edgeLumpArg)
{
	lump_t                *planeIndexLump = (lump_t *)planeIndexLumpArg;
	lump_t                *occluderLump = (lump_t *)occluderLumpArg;
	lump_t                *edgeLump = (lump_t *)edgeLumpArg;
	bspDiskOccluder_t     *diskOccluders;
	const int             *diskPlaneIndexes;
	bspDiskOccluderEdge_t *diskEdges;
	worldOccluder_t       *occluders;
	bspDpvsPlane_t        *planes;
	worldOccluderEdge_t   *edges;
	int                    count;
	int                    result;
	int                    i;
	int                    j;

	diskOccluders = (bspDiskOccluder_t *)(bspImageBase + occluderLump->fileofs);
	if ( occluderLump->filelen % 0x14u )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = occluderLump->filelen / 0x14u;
	occluders = (worldOccluder_t *)ri.Hunk_Alloc(36 * count);

	diskPlaneIndexes = (const int *)(bspImageBase + planeIndexLump->fileofs);
	if ( (planeIndexLump->filelen & 3) != 0 )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	planes = (bspDpvsPlane_t *)ri.Hunk_Alloc(20 * ((unsigned int)planeIndexLump->filelen >> 2));

	diskEdges = (bspDiskOccluderEdge_t *)(bspImageBase + edgeLump->fileofs);
	if ( (edgeLump->filelen & 3) != 0 )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	{
		unsigned int edgeCount = edgeLump->filelen / 4u;
		result = ri.Hunk_Alloc(16 * edgeCount);
	}
	edges = (worldOccluderEdge_t *)result;

	(*(int *)(((char *)(void *)&s_worldData) + 300)) = (int)occluders;
	(*(int *)(((char *)(void *)&s_worldData) + 296)) = count;
	for ( i = 0; i < count; ++i )
	{
		worldOccluder_t   *occluder = &occluders[i];
		int                firstPlane;

		occluder->planeCount = diskOccluders[i].planeCount;
		firstPlane = diskOccluders[i].firstPlane;
		occluder->planes = &planes[firstPlane];
		for ( j = 0; j < occluder->planeCount; ++j )
		{
			cplane_t *collisionPlane =
			    (cplane_t *)ri.CM_GetPlaneNum(diskPlaneIndexes[firstPlane + j]);
			VectorCopy( collisionPlane->normal, occluder->planes[j].normal );
			occluder->planes[j].distance = collisionPlane->dist;
			R_SetPlaneSidesDPVS((int)&occluder->planes[j]);
		}

		occluder->vertices = &((vec3_t *)(*(int *)(((char *)(void *)&s_worldData) + 320)))[diskOccluders[i].firstVertex];
		occluder->vertexCount = diskOccluders[i].vertexCount;
		occluder->edgeCount = diskOccluders[i].edgeCount;
		occluder->edges = &edges[diskOccluders[i].firstEdge];
		for ( j = 0; j < occluder->edgeCount; ++j )
		{
			

			bspDiskOccluderEdge_t *diskEdge = &diskEdges[j];
			occluder->edges[j].planes[0] = &occluder->planes[diskEdge->planeIndices[0]];
			occluder->edges[j].planes[1] = &occluder->planes[diskEdge->planeIndices[1]];
			occluder->edges[j].vertices[0] = &occluder->vertices[diskEdge->vertexIndices[0]];
			occluder->edges[j].vertices[1] = &occluder->vertices[diskEdge->vertexIndices[1]];
		}

		occluder->activePlaneCount = 0;
		occluder->activePlanes = 0;
	}
}

/* ---- R_LoadOccluderIndexes  0x004DCF90 ----  VERIFIED */
int __cdecl R_LoadOccluderIndexes(int indexLumpArg)
{
	lump_t          *indexLump = (lump_t *)indexLumpArg;
	const short     *diskIndexes = (const short *)(bspImageBase + indexLump->fileofs);
	worldOccluder_t **result;
	int              count;
	int              i;

	if ( (indexLump->filelen & 1) != 0 )
		ri.Error(1, "\x15" "LoadMap: funny lump size in %s", &s_worldData);
	count = (unsigned int)indexLump->filelen >> 1;

	result = (worldOccluder_t **)ri.Hunk_Alloc(4 * count);
	s_worldData_occluderIndexes = (int)result;
	s_worldData_occluderIndexCount = count;
	for ( i = 0; i < count; ++i )
		result[i] = &((worldOccluder_t *)s_worldData_occluders)[diskIndexes[i]];
	return (int)result;
}

/* ---- RE_LoadWorldMap  0x004DD000 ----  VERIFIED
 * Renderer export: load a BSP into the world data.  Resets the sun/fog state,
 * reads and version-checks the file, runs every lump loader in order, then
 * publishes the world and precompiles light visibility. */
int __cdecl RE_LoadWorldMap(char *name, unsigned int *checksum)
{
	void        *buffer;
	bspHeader_t *header;
	int          fileLength;
	int          hunkStart;
	unsigned int k;
	bspLmPlacement_t placements[R_MAX_LIGHTMAPS];

	rendererSkyboxPortal = 0;                         /* rendererFogCount = 0 */
	if ( tr_worldMapLoaded )
		ri.Error(1, "\x15" "ERROR: attempted to redundantly load world map\n");
	R_InitStaticModelCache();

	/* tr.sunDirection = { 0.45, 0.30, 0.90 } (default), then R_FlushSun. */
	tr_sunDirection_0 = 1055286886;
	tr_sunDirection_1 = 1050253722;
	tr_sunDirection_2 = 1063675494;
	R_FlushSun();
	tr_sunName = 0;
	tr_fogSky_registered = 0;                         /* clear the six fog registrations */
	tr_fogPortalView_registered = 0;
	tr_fogReset_registered = 0;
	tr_fogCurrent_registered = 0;
	tr_fogTarget_registered = 0;
	tr_fogMap_registered = 0;
	VectorNormalize((float *)&tr_sunDirection);
	tr_worldMapLoaded = 1;

	fileLength = ri.FS_ReadFile(name, &buffer);
	if ( !buffer )
		ri.Error(1, "\x15" "RE_LoadWorldMap: %s not found", name);
	if ( checksum )
		*checksum = Com_BlockChecksum(buffer, fileLength);

	tr_world = 0;
	memset(&s_worldData, 0, 0x154u);
	Q_strncpyz((char *)&s_worldData, name, 64);
	Q_strncpyz(&s_worldData_baseName, Com_SkipPath((char *)&s_worldData), 64);
	Com_StripExtension(&s_worldData_baseName, &s_worldData_baseName);

	rendererSkyBox = (int)&s_worldData_skyVertexStorage;   /* R_SetSkyBox */
	hunkStart = ri.Hunk_Alloc(0);
	header = (bspHeader_t *)buffer;
	bspImageBase = (int)buffer;
	if ( header->version != BSP_VERSION )
		ri.Error(1, va("EXE_ERR_WRONG_MAP_VERSION_NUM\x15%s\x15(%i \x14" "EXE_ERR_SHOULD_BE\x15 %i)",
		               name, header->version, BSP_VERSION));

	/* endianness swap of the 68 header words: a no-op on little-endian targets */
	for ( k = 0; k < sizeof(bspHeader_t) / 4; ++k )
		((int *)header)[k] = LongNoSwap(((int *)header)[k]);

	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadShaders((unsigned int *)&header->lumps[LUMP_SHADERS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadLightmaps(&header->lumps[LUMP_LIGHTMAPS], &header->lumps[LUMP_SURFACES], placements);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	ri.Printf(0, "Loading surfaces...\n");
	R_LoadSurfaces((int)&header->lumps[LUMP_SURFACES], (unsigned int *)&header->lumps[LUMP_DRAWVERTS],
	               (int)&header->lumps[LUMP_DRAWINDEXES], (int)placements);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	ri.Printf(0, "Loading cull groups...\n");
	R_LoadCullGroups((int)&header->lumps[LUMP_CULLGROUPS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadCullGroupIndexes((int)&header->lumps[LUMP_CULLGROUPINDEXES]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	ri.Printf(0, "Loading visibility info...\n");
	R_LoadPortalVerts((unsigned int *)&header->lumps[LUMP_PORTALVERTS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadOccluders((unsigned int *)&header->lumps[LUMP_OCCLUDERS], (int)&header->lumps[LUMP_OCCLUDERPLANEINDEXES],
	                (int)&header->lumps[LUMP_OCCLUDEREDGES]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadOccluderIndexes((int)&header->lumps[LUMP_OCCLUDERINDEXES]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadAABBTrees((unsigned int *)&header->lumps[LUMP_AABBTREES]);
	R_LoadCells((unsigned int *)&header->lumps[LUMP_CELLS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadPortals((int)&header->lumps[LUMP_PORTALS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadLightIndexes((int)&header->lumps[LUMP_LIGHTINDEXES]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadNodesAndLeafs((unsigned int *)&header->lumps[LUMP_NODES], (unsigned int *)&header->lumps[LUMP_LEAFS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	ri.Printf(0, "Loading models and entities...\n");
	R_LoadSubmodels((unsigned int *)&header->lumps[LUMP_MODELS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	R_LoadEntities((unsigned int *)&header->lumps[LUMP_ENTITIES]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");
	ri.Printf(0, "Loading lights...\n");
	R_LoadLights((unsigned int *)&header->lumps[LUMP_LIGHTS]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");

	R_LoadLightVisCache((int *)&header->lumps[LUMP_LIGHTVISCACHE]);
	ri.Cmd_ExecuteText(0, "updatescreen\n");

	s_worldData_dataSize = ri.Hunk_Alloc(0) - hunkStart;
	tr_world = (int)&s_worldData;
	if ( r_vc_compile->integer )
		R_PrecalcLightVisCache((int)checksum);
	R_InitLightVisHistory();
	if ( tr_sunName )
		R_LoadSunThroughCvars(&tr_sunName);
	return ri.FS_FreeFile(buffer);
}
