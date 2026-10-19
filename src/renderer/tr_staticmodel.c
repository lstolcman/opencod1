/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_staticmodel.c -- CoD static-model rendering.
 *
 * A "static model" is a non-animated XModel instance placed in the world (a
 * prop).  This is CoD-specific; RTCW has no analog.  R_CreateStaticModel /
 * R_AddStaticModelToWorld build one cStaticModel per placement and filter its
 * bounds into the world's cell tree; R_RegisterStaticModel / R_SetupDObjToStaticModel
 * / R_FinishDObjToStaticModel build the shared, shader-merged, per-LOD surface
 * geometry (smSurface_t records) that every instance of the same model draws
 * from, and R_OptimizeSModelSurf{Generic,ARB,ATI,NV} upload that geometry into
 * the best available GPU vertex buffer.  At draw time R_AddStaticModelSurfaces
 * culls/LODs an instance, optionally caches its lit vertices through
 * tr_staticmodelcache.c, and queues draw surfs; the RB_SurfaceStaticModel*
 * entry points (referenced by tr_main.c's rb_surfaceTable) tessellate or draw
 * each surface for the generic / ARB / ATI / NV paths.
 *
 * The pointer-passed records are typed file-locally below (proven against
 * retail CoDMP.exe + CoDUO, pinned with size/offset asserts); tr_local.h is not
 * touched.  The XModel -> XModelInfo -> XModelLodInfo -> XModelSurfs -> XSurface
 * chain and DObj duplicate tr_model.c / tr_xanim.c's file-local copies on
 * purpose (separate translation units).  Global scalars stay on the
 * tr_local.h / tr_records.h / tr_smcache.h / tr_tess.h symbols.
 */

#include <malloc.h>
#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_smcache.h"
#include "tr_local.h"

extern int GetRefString_var;   /* script/scr_memorytree.cpp: the script string arena (scr_local.h) */
/* ---- renderer-owned globals ---- */
int rendererPendingStaticModels;   /* 0x014072EC */


extern char xmodel_defaultCollision[];

extern void AnglesToAxis( const float *angles, float *axis );
extern int BoxOnPlaneSide();
extern int DObjCalcAnim();
extern void DObjCalcSkel();
extern int DObjCreate();
extern int GL_Bind();
extern int GL_ClientState();
extern int GL_Cull();
extern void __cdecl GL_DrawElements( GLsizei numIndexes, GLenum mode, GLenum type, GLvoid *indices );
extern int GL_State();
extern int Hunk_AllocAlignInternal();
extern int MatrixInverse();
extern void __cdecl RB_BindAnimatedImage( void *bundle );
extern int RB_ChooseSurfaceCountColor();
extern void __cdecl RB_SetIteratorFog( void );
extern int RB_SetupMultitexture();
extern int RB_SetupMultitextureATI();
extern int RE_RegisterModel();
extern int R_AddScaledDebugString();
extern int R_CacheStaticModelSurface();
extern void __cdecl R_AddDrawSurf( int sortStorage, shader_t *shader, void *surface, int dlightBits, int fogIndex, int worldEntityFlag );
extern smcPage_t *__cdecl R_UsedCachedStaticModelSurface( smcSurface_t *surface );
extern char *__cdecl R_CacheableStaticModelShader( int shader );
extern int R_CreateBufferARB();
extern int R_FindShader();
extern double __cdecl R_GetLodDist( float *entityOrigin );
extern int R_GetStaticLightContributions();
extern void __cdecl R_GetXModelBounds( float *transform, int obj, float *mins, float *maxs );
extern int R_RemapTextureCoordinatesForSheet();
extern int R_SetupStaticModelLighting();
extern unsigned int *__cdecl R_SyncRenderThread( void );
extern float VectorMax( const float *v );
extern void __cdecl DObjBindEvalStorage( void *storage, void *dobj );

/* DObjBindEvalStorage as retail inlines it in R_AddStaticModelToWorld: attach
 * the storage and clear its three 4-word part-bit sets (evaluated / control /
 * blocked) */
#define R_BIND_DOBJ_EVAL_STORAGE( obj, storage )   {     int k_;     (obj)->evalStorage = (storage);     for ( k_ = 0; k_ < 4; ++k_ )     {       ((unsigned int *)(storage))[k_]     = 0;       ((unsigned int *)(storage))[4 + k_] = 0;       ((unsigned int *)(storage))[8 + k_] = 0;     }   }
extern int DObjGetNumSurfaces();
extern void DObjGetSurfaces();
extern int XSurfaceGetVerts();
extern int __cdecl XModelGetLodForDist( const void *model, float distance );

/* ------------------------------------------------------------------------- *
 *  CoD XModel data chain (file-local typed views; same layout tr_model.c /
 *  tr_xanim.c use).  Only the fields this file touches are named.
 * ------------------------------------------------------------------------- */
typedef struct XModel_s XModel;

typedef struct {
	unsigned char  tileMode;          /* +0x00 */
	unsigned char  pad01;             /* +0x01 */
	short          vertexCount;       /* +0x02 */
	short          triangleCount;     /* +0x04 */
	short          boneMatrixOffset;  /* +0x06 */
	unsigned int   boneUsage[4];      /* +0x08 */
	void          *weightedPoints;    /* +0x18 */
	unsigned short *triangles;        /* +0x1c  index triples */
	void          *vertexData;        /* +0x20 */
	float        (*texCoords)[2];     /* +0x24 */
	void          *optimizedDataARB;  /* +0x28 */
	void          *optimizedDataATI;  /* +0x2c */
	void          *optimizedDataNV;   /* +0x30 */
} XSurface;
COD1_ASSERT_SIZE(XSurface, 0x34);
ASSERT_OFF(XSurface, vertexCount, 0x02);
ASSERT_OFF(XSurface, triangleCount, 0x04);
ASSERT_OFF(XSurface, triangles, 0x1c);

typedef struct XModelSurfsData {
	struct XModelSurfsData *next;         /* +0x00 */
	short                   surfaceCount; /* +0x04 */
	short                   pad06;        /* +0x06 */
	XSurface              **surfaces;     /* +0x08 */
} XModelSurfsData;
COD1_ASSERT_SIZE(XModelSurfsData, 0x0c);
ASSERT_OFF(XModelSurfsData, surfaceCount, 0x04);
ASSERT_OFF(XModelSurfsData, surfaces, 0x08);

typedef struct {
	const char      *name;                /* +0x00 */
	XModelSurfsData *surfs;               /* +0x04 */
	void           (*freeData)(void *);   /* +0x08 */
} XModelSurfs;
COD1_ASSERT_SIZE(XModelSurfs, 0x0c);
ASSERT_OFF(XModelSurfs, surfs, 0x04);

typedef struct {
	float           distance;             /* +0x00 */
	const char     *name;                 /* +0x04 */
	short           surfaceCount;         /* +0x08 */
	short           pad0a;                /* +0x0a */
	unsigned short *surfaceNameTable;     /* +0x0c */
	XModelSurfs    *surfs;                /* +0x10 */
} XModelLodInfo;
COD1_ASSERT_SIZE(XModelLodInfo, 0x14);
ASSERT_OFF(XModelLodInfo, surfaceNameTable, 0x0c);
ASSERT_OFF(XModelLodInfo, surfs, 0x10);

typedef struct {
	void          *parts;                 /* +0x00 */
	XModelLodInfo  lodRecords[3];         /* +0x04  stride 0x14 */
	void          *collisionSurfaces;     /* +0x40 */
	int            collisionSurfaceCount; /* +0x44 */
	int            contents;              /* +0x48 */
	float          mins[3];               /* +0x4c */
	float          maxs[3];               /* +0x58 */
	short          lodCount;              /* +0x64 */
	short          modelFileCount;        /* +0x66 */
} XModelInfo;
COD1_ASSERT_SIZE(XModelInfo, 0x68);
ASSERT_OFF(XModelInfo, lodRecords, 0x04);
ASSERT_OFF(XModelInfo, lodCount, 0x64);

struct XModel_s {
	const char  *name;                    /* +0x00 */
	XModelInfo  *info;                    /* +0x04 */
	void       (*freeData)(void *);       /* +0x08 */
};
COD1_ASSERT_SIZE(XModel, 0x0c);
ASSERT_OFF(XModel, info, 0x04);

/* DObj -- an instanced skeleton (see tr_xanim.c).  Only the fields this file
 * reads are named. */
typedef struct {
	void          *tree;                       /* +0x00 */
	void          *evalStorage;                /* +0x04  DObjSkelMat[] bone matrices */
	int            skelCacheKey;               /* +0x08 */
	unsigned char *partRemapTable;             /* +0x0c */
	unsigned short unknownState10;             /* +0x10 */
	unsigned short scrNotifyId;                /* +0x12 */
	unsigned short tracePartRemapHandle;       /* +0x14 */
	unsigned char  childCount;                 /* +0x16 */
	unsigned char  partCount;                  /* +0x17 */
	XModel        *childRefs[8];               /* +0x18 */
	short          childModelIndices[8];       /* +0x38 */
	unsigned char  childParentPartIndices[8];  /* +0x48 */
	unsigned char  childPartBaseIndices[8];    /* +0x50 */
} DObj;
COD1_ASSERT_SIZE(DObj, 0x58);
ASSERT_OFF(DObj, evalStorage, 0x04);
ASSERT_OFF(DObj, childCount, 0x16);
ASSERT_OFF(DObj, partCount, 0x17);
ASSERT_OFF(DObj, childRefs, 0x18);
ASSERT_OFF(DObj, childPartBaseIndices, 0x50);

/* ------------------------------------------------------------------------- *
 *  Static-model surface geometry.
 *
 *  R_FinishDObjToStaticModel merges every source XSurface that shares a shader
 *  into one smSurface_t and builds the smSurfaceList_t (one per LOD) that the
 *  registered model points at.  The drawSurf emitted by R_AddStaticModelSurfaces
 *  points at the smSurface_t base; this is also the smSurface_t that
 *  R_CacheStaticModelSurface caches (cachedShader @ +0x0c, vertexCount @ +0x12,
 *  texCoords @ +0x14, normals @ +0x18, vertices @ +0x1c).
 * ------------------------------------------------------------------------- */

/* surfaceType selector (smSurface_t.surfaceType / drawSurf dispatch index) */
typedef enum {
	SM_SURFACE_TESS           = 11,   /* RB_SurfaceStaticModel (un-optimized, copies into tess) */
	SM_SURFACE_T2V3_GENERIC   = 16,   /* tex+pos, 20-byte */
	SM_SURFACE_T2V3_ARB       = 17,
	SM_SURFACE_T2V3_ATI       = 18,
	SM_SURFACE_T2V3_NV        = 19,
	SM_SURFACE_T2N3V3_GENERIC = 20,   /* tex+normal+pos, 32-byte */
	SM_SURFACE_T2N3V3_ARB     = 21,
	SM_SURFACE_T2N3V3_ATI     = 22,
	SM_SURFACE_T2N3V3_NV      = 23
} smSurfaceType_t;

/* shader.surfaceFlags masks that select the vertex format an optimized surface
 * uses: geometry that needs no normals (0x3FF5FD00 clear) uses T2V3, otherwise
 * T2N3V3 unless 0x3FF7FC00 is also set (then it is left unoptimized). */
#define SM_NEEDS_NORMALS_MASK     0x3FF5FD00
#define SM_NO_OPTIMIZE_MASK       0x3FF7FC00

typedef struct {
	int             surfaceType;         /* +0x00  smSurfaceType_t */
	int             storageMode;         /* +0x04  storageClass */
	shader_t       *shader;              /* +0x08 */
	shader_t       *cachedShader;        /* +0x0c  R_CacheableStaticModelShader result */
	unsigned short  indexCount;          /* +0x10  3 * triangleCount */
	unsigned short  vertexCount;         /* +0x12 */
	float         (*texCoords)[2];       /* +0x14 */
	vec3_t         *normals;             /* +0x18 */
	vec3_t         *vertices;            /* +0x1c */
	unsigned short *indices;             /* +0x20 */
	void           *gpuVertexBuffer;     /* +0x24  ARB vertex VBO / ATI buffer object / generic+NV verts */
	int             gpuIndexBuffer;      /* +0x28  ARB index VBO / ATI vertex byte offset */
	int             gpuIndexOffset;      /* +0x2c  ATI index byte offset */
} smSurface_t;
COD1_ASSERT_SIZE(smSurface_t, 0x30);
ASSERT_OFF(smSurface_t, shader, 0x08);
ASSERT_OFF(smSurface_t, cachedShader, 0x0c);
ASSERT_OFF(smSurface_t, indexCount, 0x10);
ASSERT_OFF(smSurface_t, vertexCount, 0x12);
ASSERT_OFF(smSurface_t, texCoords, 0x14);
ASSERT_OFF(smSurface_t, normals, 0x18);
ASSERT_OFF(smSurface_t, vertices, 0x1c);
ASSERT_OFF(smSurface_t, indices, 0x20);
ASSERT_OFF(smSurface_t, gpuVertexBuffer, 0x24);

/* { int surfaceCount; smSurface_t surfaces[surfaceCount]; } -- one per LOD */
typedef struct {
	int         surfaceCount;   /* +0x00 */
	smSurface_t surfaces[1];    /* +0x04 */
} smSurfaceList_t;
ASSERT_OFF(smSurfaceList_t, surfaces, 0x04);

/* the shared static-model template (tr_registeredStaticModels[]): its name, the
 * source XModel, and the per-LOD merged surface lists. */
typedef struct {
	const char       *name;         /* +0x00 */
	int               lodCount;     /* +0x04 */
	XModel           *model;        /* +0x08 */
	smSurfaceList_t  *lodSurfs[1];  /* +0x0c  [lodCount]; name string follows */
} smRegisteredModel_t;
ASSERT_OFF(smRegisteredModel_t, model, 0x08);
ASSERT_OFF(smRegisteredModel_t, lodSurfs, 0x0c);

/* interleaved optimized-vertex formats written into the GPU buffers */
typedef struct {
	float texCoord[2];    /* +0x00 */
	float position[3];    /* +0x08 */
} smVertexT2V3_t;
COD1_ASSERT_SIZE(smVertexT2V3_t, 0x14);

typedef struct {
	float texCoord[2];    /* +0x00 */
	float normal[3];      /* +0x08 */
	float position[3];    /* +0x14 */
} smVertexT2N3V3_t;
COD1_ASSERT_SIZE(smVertexT2N3V3_t, 0x20);

/* ------------------------------------------------------------------------- *
 *  Placed static-model instances.
 *
 *  worldStaticModel_t is the pending placement built by R_CreateStaticModel and
 *  threaded on rendererPendingStaticModels.  R_AddStaticModelToWorld turns each into a
 *  renderer_static_model_t (a refEntity-prefixed record; the renderer_static_model_t of
 *  tr_staticmodelcache.c) that is filtered into the world cell tree and owns the
 *  per-surface vertex cache (surfaceLightingCache[] at +0x248).
 * ------------------------------------------------------------------------- */
typedef struct worldStaticModel_s {
	char                       name[64];      /* +0x00 */
	vec3_t                     origin;        /* +0x40 */
	float                      axis[3][3];    /* +0x4c */
	float                      axisScale;     /* +0x70  VectorMax of the model scale */
	vec3_t                     color;         /* +0x74 */
	smRegisteredModel_t       *registered;    /* +0x80 */
	vec3_t                     boundsMin;     /* +0x84 */
	vec3_t                     boundsMax;     /* +0x90 */
	struct worldStaticModel_s *next;          /* +0x9c */
} worldStaticModel_t;
COD1_ASSERT_SIZE(worldStaticModel_t, 0xa0);
ASSERT_OFF(worldStaticModel_t, origin, 0x40);
ASSERT_OFF(worldStaticModel_t, axis, 0x4c);
ASSERT_OFF(worldStaticModel_t, color, 0x74);
ASSERT_OFF(worldStaticModel_t, registered, 0x80);
ASSERT_OFF(worldStaticModel_t, boundsMin, 0x84);
ASSERT_OFF(worldStaticModel_t, next, 0x9c);

typedef struct {
	int                  reType;             /* +0x00  1 (from list) or 2 (registered) */
	int                  renderfx;           /* +0x04  = 128 */
	int                  hModel;             /* +0x08 */
	vec3_t               localOrigin;        /* +0x0c  bounds centre (mins+maxs)*0.5 */
	float                shadowPlane;        /* +0x18 */
	float                axis[3][3];         /* +0x1c */
	float                axisScale;          /* +0x40 */
	vec3_t               origin;             /* +0x44 */
	int                  frame;              /* +0x50 */
	vec3_t               oldorigin;          /* +0x54 */
	int                  oldframe;           /* +0x60 */
	float                radius2;            /* +0x64 */
	int                  customShader;       /* +0x68 */
	byte                 color[4];           /* +0x6c  shaderRGBA */
	float                shaderTexCoord[2];  /* +0x70 */
	float                shaderTime;         /* +0x78 */
	float                radius;             /* +0x7c */
	float                rotation;           /* +0x80 */
	vec3_t               fireRiseDir;        /* +0x84 */
	DObj                *dobj;               /* +0x90 */
	void                *owner;              /* +0x94 */
	smRegisteredModel_t *registered;         /* +0x98 */
	float                bounds[2][3];       /* +0x9c  mins / maxs */
	int                  viewCount;          /* +0xb4  DPVS submitted-this-view stamp (tr_dpvs.c dpvsStaticModel_t) */
	int                  lightContributions; /* +0xb8  R_GetStaticLightContributions result */
	byte                 lighting[0x248 - 0xbc]; /* +0xbc  static-light index / colour tables */
	void                *surfaceLightingCache[1];/* +0x248  [surfaceIndex] */
} renderer_static_model_t;
ASSERT_OFF(renderer_static_model_t, localOrigin, 0x0c);
ASSERT_OFF(renderer_static_model_t, axis, 0x1c);
ASSERT_OFF(renderer_static_model_t, origin, 0x44);
ASSERT_OFF(renderer_static_model_t, color, 0x6c);
ASSERT_OFF(renderer_static_model_t, dobj, 0x90);
ASSERT_OFF(renderer_static_model_t, registered, 0x98);
ASSERT_OFF(renderer_static_model_t, bounds, 0x9c);
ASSERT_OFF(renderer_static_model_t, surfaceLightingCache, 0x248);

/* one static-model reference threaded on a world cell's list */
typedef struct smodelRef_s {
	renderer_static_model_t     *model;   /* +0x00 */
	struct smodelRef_s *next;    /* +0x04 */
} smodelRef_t;
COD1_ASSERT_SIZE(smodelRef_t, 0x08);

/* one world cell (0x40); only the static-model list head is touched here */
typedef struct {
	vec3_t       mins;               /* +0x00  (tr_dpvs.c dpvsCell_t) */
	vec3_t       maxs;               /* +0x0c */
	void        *aabbTree;           /* +0x18 */
	void        *portals;            /* +0x1c */
	int          portalCount;        /* +0x20 */
	void        *cullGroups;         /* +0x24 */
	int          cullGroupCount;     /* +0x28 */
	void        *occluders;          /* +0x2c */
	int          occluderCount;      /* +0x30 */
	int          markViewCount;      /* +0x34 */
	smodelRef_t *firstStaticModel;   /* +0x38 */
	void        *entityLinks;        /* +0x3c */
} worldCell_t;
COD1_ASSERT_SIZE(worldCell_t, 0x40);
ASSERT_OFF(worldCell_t, firstStaticModel, 0x38);

/* trRefEntity_t (tr_local.h) fields the static-model draw path reads that fall
 * outside the header's named prefix (proven against retail). */
#define R_EntStaticModel(ent)        ((smRegisteredModel_t *)(ent)->e.staticModel)
#define R_EntHasDynamicLights(ent)   ((ent)->hasDynamicLights)
#define R_EntOwner(ent)              ((renderer_static_model_t *)(ent)->staticModelLighting)

/* one shader stage (0x688); only the fields the draw backends touch are named
 * (interior layout is disputed across the shade TUs, so it stays file-local). */
typedef struct {
	unsigned int  attributeBits;    /* +0x000 */
	int           bundle;           /* +0x004  first textureBundle (image / anim) */
	byte          bundleRest[0x644 - 0x008]; /* rest of textureBundle_t bundles[8] (tr_shade.c) */
	int           atiFragmentShader;      /* +0x644 */
	void         *registerCombiners;      /* +0x648 */
	char         *vertexProgram;          /* +0x64c */
	byte          rgbWave[0x14];          /* +0x650  waveForm_t */
	int           rgbGen;           /* +0x664  colorGen_t */
	byte          alphaWave[0x14];        /* +0x668  waveForm_t */
	int           alphaGen;               /* +0x67c */
	byte          constantColor[4]; /* +0x680  RGBA */
	unsigned int  stateBits;        /* +0x684  GL_State bits */
} shaderStage_t;
COD1_ASSERT_SIZE(shaderStage_t, 0x688);
ASSERT_OFF(shaderStage_t, rgbGen, 0x664);
ASSERT_OFF(shaderStage_t, constantColor, 0x680);
ASSERT_OFF(shaderStage_t, stateBits, 0x684);

#define CGEN_LIGHTING_PRECALC   11   /* rgbGen: use the entity's precomputed shaderRGBA */

/* shader fields past tr_local.h's named prefix, reached on the tess / globals shader */
#define R_ShaderCullType(sh)   (((shader_t *)(sh))->cullType)
#define R_ShaderStages(sh)     ((shaderStage_t **)((shader_t *)(sh))->stages)

/* ---- R_AddModelToCell_0  0x005038E0 ----  [HIGH]
 * Prepend one static-model reference to a world cell's list, unless the cell's
 * head already references this model.  Returns the (new or existing) head. */
smodelRef_t *__cdecl R_AddModelToCell_0(int cellIndex, int world, int model)
{
  worldCell_t *cell = (worldCell_t *)(*(int *)(world + 292) + (cellIndex << 6));
  smodelRef_t *ref;

  ref = cell->firstStaticModel;
  if ( !ref || (int)ref->model != model )
  {
    ref = (smodelRef_t *)ri.Hunk_Alloc(8);
    ref->model = (renderer_static_model_t *)model;
    ref->next = cell->firstStaticModel;
    cell->firstStaticModel = ref;
  }
  return ref;
}

/* ---- R_FilterModelIntoCells_r_0  0x00503920 ----  VERIFIED
 * Recursively descend the world cell tree, adding the static model to every leaf
 * cell its bounding box reaches.  A leaf (cellIndex >= 0) gets one reference; an
 * internal node either follows a single side or, when the box straddles an axial
 * split plane, is clipped and recursed on both sides. */
smodelRef_t *__cdecl R_FilterModelIntoCells_r_0(int world, mnode_t *node, int model, float *mins, float *maxs)
{
  int       side;
  cplane_t *plane;
  float     newMins[3];
  float     newMaxs[3];

  while ( node->cellIndex == -2 )   /* internal node */
  {
    side = BoxOnPlaneSide((int)mins, (int)maxs, (int)node->u.node.plane);
    if ( side == 3 )   /* straddles the plane */
    {
      if ( node->u.node.plane->type < 3 )
      {
        /* axial split: front half clipped to mins[axis] = dist, back half to maxs */
        plane = node->u.node.plane;
        newMins[1] = mins[1];
        newMins[0] = mins[0];
        newMins[2] = mins[2];
        newMins[plane->type] = plane->dist;
        newMaxs[0] = maxs[0];
        newMaxs[1] = maxs[1];
        newMaxs[2] = maxs[2];
        newMaxs[plane->type] = plane->dist;
        if ( newMins[plane->type] < maxs[plane->type] )   /* front half not empty (ordered <: NaN/equal skip) */
          R_FilterModelIntoCells_r_0(world, node->u.node.children[0], model, newMins, maxs);
        R_FilterModelIntoCells_r_0(world, node->u.node.children[1], model, mins, newMaxs);
        return;
      }
      /* non-axial split -> recurse both */
      R_FilterModelIntoCells_r_0(world, node->u.node.children[0], model, mins, maxs);
      node = node->u.node.children[1];
    }
    else   /* wholly on one side */
    {
      node = node->u.node.children[side - 1];
    }
  }

  if ( node->cellIndex >= 0 )
    return R_AddModelToCell_0(node->cellIndex, world, model);
}

/* ---- R_OptimizeSModelSurfGeneric  0x00503A60 ----  VERIFIED
 * Build a system-memory interleaved vertex buffer for a merged surface: T2V3
 * (tex + position) when the shader needs no normals, else T2N3V3 (tex + normal +
 * position).  Returns the buffer / trailing cursor (dead EAX). */
void __cdecl R_OptimizeSModelSurfGeneric(smSurface_t *surf)
{
  int          result;
  int i,size;

  if((surf->shader->surfaceFlags&SM_NEEDS_NORMALS_MASK)==0)
  {
    smVertexT2V3_t *out;

    surf->storageMode = 3;
    surf->surfaceType = SM_SURFACE_T2V3_GENERIC;
    size=20*surf->vertexCount;out=(smVertexT2V3_t *)ri.Hunk_Alloc(size);
    surf->gpuVertexBuffer = out;
    for ( i = 0; i < surf->vertexCount; ++i )
    {
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].texCoord[0] = surf->texCoords[i][0];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].texCoord[1] = surf->texCoords[i][1];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].position[0] = surf->vertices[i][0];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].position[1] = surf->vertices[i][1];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].position[2] = surf->vertices[i][2];
    }

  }
  else
  {
    if ( (surf->shader->surfaceFlags & SM_NO_OPTIMIZE_MASK) == 0 )
    {
      smVertexT2N3V3_t *out;

      surf->storageMode = 3;
      surf->surfaceType = SM_SURFACE_T2N3V3_GENERIC;
      size=32*surf->vertexCount;out=(smVertexT2N3V3_t *)ri.Hunk_Alloc(size);
      surf->gpuVertexBuffer = out;
      for ( i = 0; i < surf->vertexCount; ++i )
      {
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].texCoord[0] = surf->texCoords[i][0];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].texCoord[1] = surf->texCoords[i][1];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].normal[0]   = surf->normals[i][0];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].normal[1]   = surf->normals[i][1];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].normal[2]   = surf->normals[i][2];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].position[0] = surf->vertices[i][0];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].position[1] = surf->vertices[i][1];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].position[2] = surf->vertices[i][2];
      }

    }
  }
  return;
}

/* ---- R_OptimizeSModelSurfNV  0x00503BC0 ----  VERIFIED
 * As R_OptimizeSModelSurfGeneric, but the interleaved buffer is placed in NV
 * static vertex memory (primary, else secondary, else an aligned hunk block);
 * storageMode records which pool.  Returns the buffer / cursor (dead EAX). */
void __cdecl R_OptimizeSModelSurfNV(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          result;
  int          i;

  result = surf->shader->surfaceFlags;
  if((result&SM_NEEDS_NORMALS_MASK)==0)
  {
    unsigned int    size;
    smVertexT2V3_t *out;

    surf->surfaceType = SM_SURFACE_T2V3_NV;
    size = (4 * (5 * surf->vertexCount) + 31) & 0xFFFFFFE0;
    if ( (int)(tr_staticVertexMemoryPrimaryUsed + size) < tr_staticVertexMemoryPrimaryLimit )
    {
      surf->gpuVertexBuffer = (void *)(tr_staticVertexMemoryPrimaryUsed + tr_staticVertexMemoryPrimary);
      tr_staticVertexMemoryPrimaryUsed += size;
      result = 1;
    }
    else if ( (int)(tr_staticVertexMemorySecondaryUsed + size) < tr_staticVertexMemorySecondaryLimit )
    {
      surf->gpuVertexBuffer = (void *)(tr_staticVertexMemorySecondaryUsed + tr_staticVertexMemorySecondary);
      tr_staticVertexMemorySecondaryUsed += size;
      result = 2;
    }
    else
    {
      surf->gpuVertexBuffer = (void *)Hunk_AllocAlignInternal(size, 32);
      result = 3;
    }
    surf->storageMode = result;
    out = (smVertexT2V3_t *)surf->gpuVertexBuffer;
    for ( i = 0; i < surf->vertexCount; ++i )
    {
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].texCoord[0] = surf->texCoords[i][0];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].texCoord[1] = surf->texCoords[i][1];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].position[0] = surf->vertices[i][0];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].position[1] = surf->vertices[i][1];
      ((smVertexT2V3_t *)surf->gpuVertexBuffer)[i].position[2] = surf->vertices[i][2];
    }

  }
  else
  {
    if ( (result & SM_NO_OPTIMIZE_MASK) == 0 )
    {
      unsigned int      size;
      smVertexT2N3V3_t *out;

      surf->surfaceType = SM_SURFACE_T2N3V3_NV;
      size = (32 * surf->vertexCount + 31) & 0xFFFFFFE0;
      if ( (int)(tr_staticVertexMemoryPrimaryUsed + size) < tr_staticVertexMemoryPrimaryLimit )
      {
        surf->gpuVertexBuffer = (void *)(tr_staticVertexMemoryPrimaryUsed + tr_staticVertexMemoryPrimary);
        tr_staticVertexMemoryPrimaryUsed += size;
        result = 1;
      }
      else if ( (int)(tr_staticVertexMemorySecondaryUsed + size) < tr_staticVertexMemorySecondaryLimit )
      {
        surf->gpuVertexBuffer = (void *)(tr_staticVertexMemorySecondaryUsed + tr_staticVertexMemorySecondary);
        tr_staticVertexMemorySecondaryUsed += size;
        result = 2;
      }
      else
      {
        surf->gpuVertexBuffer = (void *)Hunk_AllocAlignInternal(size, 32);
        result = 3;
      }
      surf->storageMode = result;
      out = (smVertexT2N3V3_t *)surf->gpuVertexBuffer;
      for ( i = 0; i < surf->vertexCount; ++i )
      {
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].texCoord[0] = surf->texCoords[i][0];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].texCoord[1] = surf->texCoords[i][1];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].normal[0]   = surf->normals[i][0];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].normal[1]   = surf->normals[i][1];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].normal[2]   = surf->normals[i][2];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].position[0] = surf->vertices[i][0];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].position[1] = surf->vertices[i][1];
        ((smVertexT2N3V3_t *)surf->gpuVertexBuffer)[i].position[2] = surf->vertices[i][2];
      }

    }
  }
  return;
}

/* ---- R_OptimizeSModelSurfATI  0x00503DD0 ----  VERIFIED
 * Build an ATI-object-buffer interleaved vertex (and, with ATIElementArray, a
 * paired index) buffer for a merged surface: T2V3 or T2N3V3 by shader.  The
 * geometry is staged in a scratch buffer then uploaded with
 * qglUpdateObjectBufferATI.  Returns 1 on success, 0 when disabled/no room. */
int __cdecl R_OptimizeSModelSurfATI(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          surfaceFlags;
  unsigned int indexBytes;
  int          vertexBytes;
  int          totalBytes;
  int          bufferBase;
  unsigned int alignedBytes;
  int          storageMode;
  int          i;

  surfaceFlags = surf->shader->surfaceFlags;
  indexBytes = (2 * surf->indexCount + 31) & 0xFFFFFFE0;
  if ( (surfaceFlags & SM_NEEDS_NORMALS_MASK) == 0 )
  {
    surf->surfaceType = SM_SURFACE_T2V3_ATI;
    vertexBytes = 20 * surf->vertexCount;
  }
  else if ( (surfaceFlags & SM_NO_OPTIMIZE_MASK) == 0 )
  {
    surf->surfaceType = SM_SURFACE_T2N3V3_ATI;
    vertexBytes = 32 * surf->vertexCount;
  }
  else
  {
    return 0;
  }

  totalBytes = vertexBytes;
  if ( glConfig_ATIElementArray )
    totalBytes = vertexBytes + indexBytes;   /* also reserve the index buffer */
  /* R_AllocMemoryATI(1, totalBytes, &bufferBase), inlined in retail */
  alignedBytes = (totalBytes + 31) & 0xFFFFFFE0;
  if ( (int)(tr_staticVertexMemoryPrimaryUsed + alignedBytes) <= tr_staticVertexMemoryPrimaryLimit )
  {
    bufferBase = tr_staticVertexMemoryPrimaryUsed;
    tr_staticVertexMemoryPrimaryUsed += alignedBytes;
    storageMode = 1;
  }
  else if ( (int)(tr_staticVertexMemorySecondaryUsed + alignedBytes) <= tr_staticVertexMemorySecondaryLimit )
  {
    bufferBase = tr_staticVertexMemorySecondaryUsed;
    tr_staticVertexMemorySecondaryUsed += alignedBytes;
    storageMode = 2;
  }
  else
  {
    storageMode = 0;
  }
  surf->storageMode = storageMode;
  if ( storageMode )
  {
    if ( storageMode == 1 )
      surf->gpuVertexBuffer = (void *)tr_staticVertexMemoryPrimary;
    else
      surf->gpuVertexBuffer = (void *)tr_staticVertexMemorySecondary;
    surf->storageMode = 3;

    if ( glConfig_ATIElementArray )
    {
      surf->gpuIndexBuffer = bufferBase + indexBytes;   /* vertex data after the indices */
      surf->gpuIndexOffset = bufferBase;
      qglUpdateObjectBufferATI((int)surf->gpuVertexBuffer, bufferBase, 2 * surf->indexCount,
                               (int)surf->indices, 34658);
    }
    else
    {
      surf->gpuIndexOffset = 0;
      surf->gpuIndexBuffer = bufferBase;
    }

    /* per-format block-scoped staging arrays (retail overlaps them in one 0x40000 slot) */
    if ( surf->surfaceType == SM_SURFACE_T2V3_ATI )
    {
      smVertexT2V3_t staging[0x40000 / sizeof(smVertexT2V3_t)];

      for ( i = 0; i < surf->vertexCount; ++i )
      {
        staging[i].texCoord[0] = surf->texCoords[i][0];
        staging[i].texCoord[1] = surf->texCoords[i][1];
        staging[i].position[0] = surf->vertices[i][0];
        staging[i].position[1] = surf->vertices[i][1];
        staging[i].position[2] = surf->vertices[i][2];
      }
      qglUpdateObjectBufferATI((int)surf->gpuVertexBuffer, surf->gpuIndexBuffer, vertexBytes, (int)staging, 34658);
    }
    else
    {
      smVertexT2N3V3_t staging[0x40000 / sizeof(smVertexT2N3V3_t)];

      for ( i = 0; i < surf->vertexCount; ++i )
      {
        staging[i].texCoord[0] = surf->texCoords[i][0];
        staging[i].texCoord[1] = surf->texCoords[i][1];
        staging[i].normal[0]   = surf->normals[i][0];
        staging[i].normal[1]   = surf->normals[i][1];
        staging[i].normal[2]   = surf->normals[i][2];
        staging[i].position[0] = surf->vertices[i][0];
        staging[i].position[1] = surf->vertices[i][1];
        staging[i].position[2] = surf->vertices[i][2];
      }
      qglUpdateObjectBufferATI((int)surf->gpuVertexBuffer, surf->gpuIndexBuffer, vertexBytes, (int)staging, 34658);
    }
    return 1;
  }
  return 0;
}

/* ---- R_OptimizeSModelSurfARB  0x00503FD0 ----  VERIFIED
 * Build ARB vertex + index buffer objects (VBOs) for a merged surface, staging
 * the interleaved vertices before R_CreateBufferARB uploads them.  Returns 1 on
 * success, 0 if either VBO could not be created. */
int __cdecl R_OptimizeSModelSurfARB(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          surfaceFlags;
  unsigned int indexBytes;
  int          vertexBytes;
  int          indexBuffer;
  int          vertexBuffer;
  int          i;

  indexBytes = (2 * surf->indexCount + 31) & 0xFFFFFFE0;
  surfaceFlags = surf->shader->surfaceFlags;
  if ( (surfaceFlags & SM_NEEDS_NORMALS_MASK) == 0 )
  {
    surf->surfaceType = SM_SURFACE_T2V3_ARB;
    vertexBytes = 20 * surf->vertexCount;
  }
  else if ( (surfaceFlags & SM_NO_OPTIMIZE_MASK) == 0 )
  {
    surf->surfaceType = SM_SURFACE_T2N3V3_ARB;
    vertexBytes = 32 * surf->vertexCount;
  }
  else
  {
    return 0;
  }

  indexBuffer = R_CreateBufferARB(34963, indexBytes, (int)surf->indices, 35044);
  surf->gpuIndexBuffer = indexBuffer;
  if ( !indexBuffer )
    return 0;

  /* per-format block-scoped staging arrays (retail overlaps them in one 0x40000 slot) */
  if ( surf->surfaceType == SM_SURFACE_T2V3_ARB )
  {
    smVertexT2V3_t staging[0x40000 / sizeof(smVertexT2V3_t)];

    for ( i = 0; i < surf->vertexCount; ++i )
    {
      staging[i].texCoord[0] = surf->texCoords[i][0];
      staging[i].texCoord[1] = surf->texCoords[i][1];
      staging[i].position[0] = surf->vertices[i][0];
      staging[i].position[1] = surf->vertices[i][1];
      staging[i].position[2] = surf->vertices[i][2];
    }
    vertexBuffer = R_CreateBufferARB(34962, vertexBytes, (int)staging, 35044);
  }
  else
  {
    smVertexT2N3V3_t staging[0x40000 / sizeof(smVertexT2N3V3_t)];

    for ( i = 0; i < surf->vertexCount; ++i )
    {
      staging[i].texCoord[0] = surf->texCoords[i][0];
      staging[i].texCoord[1] = surf->texCoords[i][1];
      staging[i].normal[0]   = surf->normals[i][0];
      staging[i].normal[1]   = surf->normals[i][1];
      staging[i].normal[2]   = surf->normals[i][2];
      staging[i].position[0] = surf->vertices[i][0];
      staging[i].position[1] = surf->vertices[i][1];
      staging[i].position[2] = surf->vertices[i][2];
    }
    vertexBuffer = R_CreateBufferARB(34962, vertexBytes, (int)staging, 35044);
  }
  surf->gpuVertexBuffer = (void *)vertexBuffer;
  if ( !vertexBuffer )
  {
    qglDeleteBuffersARB(1, (int)&surf->gpuIndexBuffer);
    return 0;
  }
  return 1;
}

/* ---- R_SetupDObjToStaticModel  0x00504150 ----  VERIFIED
 * Allocate the shared static-model template for a model name.  With optimize on,
 * it reserves one surface-list slot per LOD; otherwise it is a bare name record
 * (lodCount / model zero) that R_FinishLoadingStaticModels leaves alone. */
smRegisteredModel_t *__cdecl R_SetupDObjToStaticModel(int dobjPtr, const char *name)
{
  DObj                *dobj = (DObj *)dobjPtr;
  smRegisteredModel_t *registered;
  int                  lodCount;

  if ( r_optimize->integer && r_optimizeSModels->integer )
  {
    XModel *model = dobj->childRefs[0];
    lodCount = model->info->lodCount;
    registered = (smRegisteredModel_t *)ri.Hunk_Alloc(strlen(name) + 4 * lodCount + 13);
    registered->name = (char *)&registered->lodSurfs[lodCount];   /* name follows the LOD slots */
    strcpy((char *)registered->name, name);
    registered->lodCount = lodCount;
    registered->model = model;
  }
  else
  {
    registered = (smRegisteredModel_t *)ri.Hunk_Alloc(strlen(name) + 13);
    registered->name = (char *)&registered->lodSurfs[0];          /* name at +12 */
    strcpy((char *)registered->name, name);
    registered->lodCount = 0;
    registered->model = 0;
  }
  return registered;
}

/* ---- R_FinishDObjToStaticModel  0x00504200 ----  VERIFIED
 * Build a static-model template's per-LOD merged surface geometry from a DObj.
 * For each LOD it enumerates the DObj's surfaces, merges every surface that
 * shares a (possibly remapped) shader into one smSurface_t, allocates that
 * surface's interleaved tex/normal/vertex/index arrays, copies and re-bases each
 * source surface's data into its group, then uploads each merged surface into a
 * GPU vertex buffer (ARB / NV / ATI, falling back to a system-memory copy).
 * Returns the LOD count (dead EAX). */
void __cdecl R_FinishDObjToStaticModel(unsigned int *registeredPtr, int dobjPtr)
{
  smRegisteredModel_t *registered = (smRegisteredModel_t *)registeredPtr;
  DObj                *dobj = (DObj *)dobjPtr;
  int                  numLods = dobj->childRefs[0]->info->lodCount;
  int                  currentLod;


  int   surfShaders[1024];      /* shader per source surface */
  int   surfGroup[1024];        /* group index per source surface */
  int   groupVertCount[1024];   /* per group: vertex count, then pass-2 write cursor */
  int   groupIndexCount[1024];  /* per group: index count, then pass-2 write cursor */
  int   groupShaders[1024];     /* merged shader per group */

  for ( currentLod = 0; currentLod < numLods; ++currentLod )
  {
    int              partBits[4];
    int              lodIndex;   /* one slot; other children's LODs read past it (retail stack aliasing) */
    int             *surfRefs;
    smSurfaceList_t *surfList;
    int              surfaceCount;
    int              groupCount;
    int              child;
    int              s;
    int              g;

    lodIndex = currentLod;   /* single-child static model: only child 0 is used */

    /* count this LOD's surfaces across the DObj's children */
    surfaceCount = 0;
    for(child=dobj->childCount-1;child>=0;--child)
    {
      int childLod = (&lodIndex)[child];
      if ( childLod >= 0 )
      {
        XModelSurfs *surfs = dobj->childRefs[child]->info->lodRecords[childLod].surfs;
        if ( surfs )
          surfaceCount += surfs->surfs->surfaceCount;
      }
    }
    if ( surfaceCount > 1024 )
      ri.Error(1, "\x15" "model '%s' has %i surfaces, which is more than %i\n",
               registered->name, surfaceCount, 1024);

    surfRefs = (int *)alloca(4 * surfaceCount);
    DObjGetSurfaces(partBits, dobj, surfRefs, (int)&lodIndex);

    /* pass 1: merge surfaces sharing a shader into groups, summing their counts */
    groupCount = 0;
    for ( s = 0; s < surfaceCount; ++s )
    {
      int             modelIndex   = (short)(surfRefs[s]&0xFFFF);
      int             surfaceIndex = (short)(surfRefs[s]>>16);
      XModelLodInfo  *lod   = &dobj->childRefs[modelIndex]->info->lodRecords[(&lodIndex)[modelIndex]];
      XSurface       *surf  = lod->surfs->surfs->surfaces[surfaceIndex];
      unsigned short  nameIndex = lod->surfaceNameTable[surfaceIndex];
      const char     *skinName = nameIndex ? (const char *)(GetRefString_var + 8 * nameIndex + 4) : "DEFAULT";
      shader_t       *shader = (shader_t *)R_FindShader(va("skins/%s", skinName), -1, 1, (const char *)7);

      surfShaders[s] = (int)shader;
      groupShaders[groupCount] = (int)shader;
      if ( (shader->flags & 0x1000) != 0 )   /* SHADER_REMAPPED -> use the remap target */
        groupShaders[groupCount] = *(int *)((char *)shader + 0x180);

      for ( g = 0; g < groupCount; ++g )
      {
        if ( groupShaders[g] == groupShaders[groupCount] )
          break;
      }
      if ( g == groupCount )   /* new group */
      {
        surfGroup[s] = groupCount;
        groupIndexCount[groupCount] = 3 * surf->triangleCount;
        groupVertCount[groupCount] = surf->vertexCount;
        ++groupCount;
      }
      else                      /* merge into existing group g */
      {
        surfGroup[s] = g;
        groupIndexCount[g] += 3 * surf->triangleCount;
        groupVertCount[g] += surf->vertexCount;
      }
    }

    /* allocate the surface list and each merged surface's SoA arrays */
    surfList = (smSurfaceList_t *)ri.Hunk_Alloc(48 * groupCount + 4);
    surfList->surfaceCount = groupCount;
    for ( g = 0; g < groupCount; ++g )
    {
      smSurface_t *record = &surfList->surfaces[g];
      int          vertexCount;
      int          block;

      record->surfaceType  = SM_SURFACE_TESS;
      record->storageMode  = storageClass;
      record->shader       = (shader_t *)groupShaders[g];
      record->cachedShader = (shader_t *)R_CacheableStaticModelShader(groupShaders[g]);
      record->indexCount   = (unsigned short)groupIndexCount[g];
      record->vertexCount  = (unsigned short)groupVertCount[g];
      if ( record->indexCount != groupIndexCount[g] || record->vertexCount != groupVertCount[g] )
        ri.Error(1, "\x15" "model %s surface %s has more than 65,535 vertices"
                 " or more than 21,845 triangles", registered->name, surfShaders[g]);

      /* one hunk block holds tex(8V) + normals(12V) + vertices(12V) + indices(2I) */
      vertexCount = record->vertexCount;
      block = ri.Hunk_Alloc(8 * vertexCount + 12 * vertexCount + 12 * vertexCount + 2 * record->indexCount);
      record->texCoords = (float (*)[2])block;
      record->normals   = (vec3_t *)(block + 8 * vertexCount);
      record->vertices  = (vec3_t *)(block + 20 * vertexCount);
      record->indices   = (unsigned short *)(block + 32 * vertexCount);
      groupIndexCount[g] = 0;   /* reset to a write cursor for pass 2 */
      groupVertCount[g] = 0;
    }

    /* pass 2: copy and re-base each source surface's verts/indices into its group */
    for ( s = 0; s < surfaceCount; ++s )
    {
      int           modelIndex   = (short)(surfRefs[s]&0xFFFF);
      int           surfaceIndex = (short)(surfRefs[s]>>16);
      XModelLodInfo *lod   = &dobj->childRefs[modelIndex]->info->lodRecords[(&lodIndex)[modelIndex]];
      XSurface      *surf  = lod->surfs->surfs->surfaces[surfaceIndex];
      int           group = surfGroup[s];
      smSurface_t  *record = &surfList->surfaces[group];
      int           vertBase  = groupVertCount[group];
      int           indexBase = groupIndexCount[group];
      int           surfIndexCount = 3 * surf->triangleCount;
      int           boneMatrix = (dobj->childPartBaseIndices[modelIndex] << 6) + (int)dobj->evalStorage + 48;
      int           k;

      memcpy(&record->indices[indexBase], surf->triangles, 6 * surf->triangleCount);
      XSurfaceGetVerts((int)surf, boneMatrix,
                       (int)((char *)record->vertices + 12 * vertBase),
                       (int)((char *)record->texCoords + 8 * vertBase),
                       (int)((char *)record->normals + 12 * vertBase));
      if ( vertBase )   /* re-base this surface's indices onto the merged vertex range */
      {
        for ( k = 0; k < surfIndexCount; ++k )
          record->indices[indexBase + k] += vertBase;
      }
      if ( (((shader_t *)surfShaders[s])->flags & 0x1000) != 0 )   /* remapped shader: fix tex coords */
        R_RemapTextureCoordinatesForSheet(surf->vertexCount,
                                          (int)((char *)record->texCoords + 8 * vertBase),
                                          surfShaders[s]);
      groupIndexCount[group] += surfIndexCount;
      groupVertCount[group] = surf->vertexCount + vertBase;
    }

    /* upload each merged surface into a GPU vertex buffer (else keep it in system memory) */
    if ( groupCount > 0 )
    {
      for ( g = 0; g < groupCount; ++g )
      {
        smSurface_t *record = &surfList->surfaces[g];

        if ( glConfig_ARBVertexBufferObject )
        {
          if ( !R_OptimizeSModelSurfARB((int)record) )
            R_OptimizeSModelSurfGeneric(record);
        }
        else if ( glConfig_NVVertexArrayRange )
        {
          R_OptimizeSModelSurfNV((int)record);
        }
        else if ( glConfig_ATIVertexArrayObject )
        {
          if ( !R_OptimizeSModelSurfATI((int)record) )
            R_OptimizeSModelSurfGeneric(record);
        }
        else
        {
          R_OptimizeSModelSurfGeneric(record);
        }
      }
    }

    registered->lodSurfs[currentLod] = surfList;

  }
  return;
}

/* ---- R_NeedsBoundsAdjustment  0x00504750 ----  VERIFIED
 * True when any of the model's surfaces uses an autosprite-type shader
 * (surfaceFlags & 0x20000000), so its bounds must be grown to a sphere.  Uses
 * the finished per-LOD surface list when present, else walks the raw DObj. */
int __cdecl R_NeedsBoundsAdjustment(int modelPtr)
{
  renderer_static_model_t *model = (renderer_static_model_t *)modelPtr;
  int                      i;

  if ( model->registered )
  {
    for ( i = 0; i < model->registered->lodSurfs[0]->surfaceCount; ++i )
    {
      if ( (model->registered->lodSurfs[0]->surfaces[i].shader->surfaceFlags & 0x20000000) != 0 )
        return 1;
    }
    return 0;
  }

  {
    DObj *dobj = model->dobj;
    if ( dobj )
    {
      int  lodIndex;      /* one slot; other children's LODs read past it (retail stack aliasing) */
      int  partBits[4];
      int  surfaceCount;
      int *surfRefs;

      lodIndex = 0;
      surfaceCount = DObjGetNumSurfaces(dobj, (int)&lodIndex);
      surfRefs = (int *)alloca(4 * surfaceCount);
      DObjGetSurfaces(partBits, dobj, surfRefs, (int)&lodIndex);
      for ( i = 0; i < surfaceCount; ++i )
      {
        int             modelIndex   = (short)surfRefs[i];
        int             surfaceIndex = (short)(surfRefs[i] >> 16);
        XModelLodInfo  *lod = &model->dobj->childRefs[modelIndex]->info->lodRecords[(&lodIndex)[modelIndex]];
        unsigned short  nameIndex = lod->surfaceNameTable[surfaceIndex];
        const char     *skinName = nameIndex ? (const char *)(GetRefString_var + 8 * nameIndex + 4) : "DEFAULT";
        shader_t       *shader = (shader_t *)R_FindShader(va("skins/%s", skinName), -1, 1, (const char *)7);

        if ( (shader->surfaceFlags & 0x20000000) != 0 )
          return 1;
      }
    }
    return 0;
  }
}

/* ---- R_AdjustBoundsForAutosprite  0x00504880 ----  VERIFIED
 * When the model needs autosprite bounds, replace its box with a cube centred on
 * the box's centre and sized to enclose the box's bounding sphere (0.55 * the
 * diagonal, plus 2).  Returns R_NeedsBoundsAdjustment (dead EAX). */
int __cdecl R_AdjustBoundsForAutosprite(int modelPtr, float *mins, float *maxs)
{
  int result = R_NeedsBoundsAdjustment(modelPtr);

  if ( result )
  {
#ifdef COD1_VC7
    /* VC7 keeps center[1] in a register and spills only center[0] and the z sum
     * (the two fstp dword); the #else models that rounding for modern MSVC. */
    vec3_t center;
    rfloat dx, dy, dz, radius;

    VectorAdd(mins, maxs, center);
    VectorScale(center, 0.5f, center);
    dx = maxs[0] - mins[0];
    dy = maxs[1] - mins[1];
    dz = maxs[2] - mins[2];
    radius = FCAST(sqrt(dx * dx + dy * dy + dz * dz)) * 0.55f + 2.0f;

    mins[0] = center[0] - radius;
    mins[1] = center[1] - radius;
    mins[2] = center[2] - radius;
    maxs[0] = center[0] + radius;
    maxs[1] = radius + center[1];
    maxs[2] = radius + center[2];
#else
    /* Retail rounds only the z-sum and centerX to float (the two fstp dword
     * spills); everything else stays on the x87 stack, modelled as double. */
    float  sumZ    = maxs[2] + mins[2];                  /* 0x5048A4 fstp var_4 */
    float  centerX = (mins[0] + maxs[0]) * 0.5f;         /* 0x5048B0 fstp var_C */
    double centerY = ((double)maxs[1] + mins[1]) * 0.5;  /* kept on x87 stack */
    double centerZ = sumZ * 0.5;
    double dz = (double)maxs[2] - mins[2];
    double dy = (double)maxs[1] - mins[1];
    double dx = (double)maxs[0] - mins[0];
    double radius = sqrt(dz * dz + dy * dy + dx * dx) * 0.55000001f + 2.0;

    mins[0] = (float)(centerX - radius);
    mins[1] = (float)(centerY - radius);
    mins[2] = (float)(centerZ - radius);
    maxs[0] = (float)(centerX + radius);
    maxs[1] = (float)(radius + centerY);
    maxs[2] = (float)(radius + centerZ);
#endif
  }
  return result;
}

/* ---- R_RegisterStaticModel  0x00504930 ----  VERIFIED
 * Look up (by name) or create the shared static-model template for a DObj.
 * Returns the template when it carries a source model, else 0 (and fails hard
 * when a not-precached model is registered after loading finished). */
unsigned int *__cdecl R_RegisterStaticModel(const char *name, int dobjPtr)
{
  smRegisteredModel_t *registered;
  int                  i;

  for ( i = 0; i < tr_registeredStaticModelCount; ++i )
  {
    registered = (smRegisteredModel_t *)tr_registeredStaticModels[i];
    if ( !Q_stricmp(registered->name, name) )
      return registered->model ? (unsigned int *)registered : 0;
  }
  if ( tr_registeredStaticModelCount == 2048 )
  {
    ri.Printf(2, "R_RegisterStaticModel failed for '%s' -- more than %i unique static models\n", name, 2048);
    return 0;
  }

  if ( !tr_modelsFinishedLoading )
    tr_delayedImageGroup = ++tr_delayedImageGroupSequence;
  else if ( r_errorOnConflicts->integer && !tr_ignorePrecacheErrorCount )
    ri.Error(1, "\x15" "model '%s' not precached\n", name);
  registered = R_SetupDObjToStaticModel(dobjPtr, name);
  tr_delayedImageGroup = 0;
  tr_delayedImageGroupTriCount = 0;
  tr_registeredStaticModels[tr_registeredStaticModelCount] = (int)registered;
  tr_registeredStaticModelCount++;
  return registered->model ? (unsigned int *)registered : 0;
}

/* ---- R_CreateStaticModel  0x00504A20 ----  VERIFIED
 * Queue a static-model placement: register the XModel, build its DObj and
 * skeleton, apply the placement scale/angles/origin to a worldStaticModel, take
 * its transformed world bounds, register its shared template, and thread it on
 * the pending list (rendererPendingStaticModels).  unused is unused (kept for the ABI). */
void __cdecl R_CreateStaticModel(int unused, char *name, float *origin, float *angles, float *scale, float *color)
{
  worldStaticModel_t *model;
  short               modelHandle;
  int                 dobjModel[3];   /* DObjModel {model, tagName, short negativeModelIndex}: stride 12 */
  XModel             *xmodel;
  DObj                dobj;            // byte v14[88] BYREF
  int                 partBits[4];     // v15
  void               *evalStorage;
/* retail calls DObjBindEvalStorage out of line here but inlines it in
 * R_FinishLoadingStaticModels: no inlining into this function */
#pragma inline_depth(0)

  if ( !_strnicmp(name, "xmodel", 6u) && ( name[6] == '/' || name[6] == '\\' ) )
  {
    if ( strlen(name) >= 0x39 )
    {
      Com_Printf("Model '%s' has a name longer than %i characters\n", name, 57);
      return;
    }

    R_SyncRenderThread();
    model = (worldStaticModel_t *)ri.Malloc(160);
    strcpy(model->name, name + 7);

    modelHandle = RE_RegisterModel(name, (const char *)7);
    dobjModel[0] = (int)((model_t *)tr_models[modelHandle])->xmodel;
    dobjModel[1] = 0;
    *(short *)&dobjModel[2] = modelHandle;   /* retail: handle word at DObjModel+8 -> DObj.childModelIndices[0] */
    DObjCreate((int)dobjModel, 1u, 0, (int)&dobj, 0);
    evalStorage = alloca(96 * dobj.partCount + 64);
    DObjBindEvalStorage((void *)(((unsigned int)evalStorage + 15) & ~15u), &dobj);
    Com_Memset(partBits, 255, 0x10u);
    DObjCalcAnim((int)&dobj, partBits);
    DObjCalcSkel(partBits, (int)&dobj);

    model->origin[0] = origin[0];
    model->origin[1] = origin[1];
    model->origin[2] = origin[2];
    AnglesToAxis(angles, model->axis[0]);
    model->axisScale = 0.0f;
    if ( scale[0] != 1.0f || scale[1] != 1.0f || scale[2] != 1.0f )
    {
      model->axis[0][0] = scale[0] * model->axis[0][0];
      model->axis[0][1] = scale[0] * model->axis[0][1];
      model->axis[0][2] = scale[0] * model->axis[0][2];
      model->axis[1][0] = scale[1] * model->axis[1][0];
      model->axis[1][1] = scale[1] * model->axis[1][1];
      model->axis[1][2] = scale[1] * model->axis[1][2];
      model->axis[2][0] = model->axis[2][0] * scale[2];
      model->axis[2][1] = model->axis[2][1] * scale[2];
      model->axis[2][2] = model->axis[2][2] * scale[2];
      model->axisScale = VectorMax(scale);
    }
    R_GetXModelBounds(model->axis[0], (int)&dobj, model->boundsMin, model->boundsMax);
    model->boundsMin[0] = model->origin[0] + model->boundsMin[0];
    model->boundsMin[1] = model->origin[1] + model->boundsMin[1];
    model->boundsMin[2] = model->boundsMin[2] + model->origin[2];
    model->boundsMax[0] = model->boundsMax[0] + model->origin[0];
    model->boundsMax[1] = model->origin[1] + model->boundsMax[1];
    model->boundsMax[2] = model->boundsMax[2] + model->origin[2];
    model->color[0] = color[0];
    model->color[1] = color[1];
    model->color[2] = color[2];

    xmodel = (XModel *)dobjModel[0];
    model->registered = 0;
    if ( (void *)xmodel->info != (void *)xmodel_defaultCollision )
      model->registered = (smRegisteredModel_t *)R_RegisterStaticModel(model->name, (int)&dobj);
    model->next = (worldStaticModel_t *)rendererPendingStaticModels;
    rendererPendingStaticModels = (int)model;
  }
  else
  {
    Com_Printf("Model '%s' is not an xmodel\n", name);
  }
}

/* ---- R_AddStaticModelToWorld  0x00504CC0 ----  VERIFIED
 * Turn a pending placement into a live cStaticModel: reserve one surface-cache
 * slot per surface across all LODs (or, when the model has no shared template,
 * build a DObj), copy in the transform/colour/bounds, adjust autosprite bounds,
 * filter the box into every world cell it touches, and precompute its static
 * light contributions.  Returns those contributions (dead EAX). */
#pragma optimize( "p", on )
int __cdecl R_AddStaticModelToWorld(int sourcePtr)
{
  worldStaticModel_t  *source = (worldStaticModel_t *)sourcePtr;
  smRegisteredModel_t *registered = source->registered;
  renderer_static_model_t      *model;
  DObj                *dobj;
  int                  reType;
  int                  surfaceSlots;
  int                  result;
  float                c;
#pragma inline_depth()

  if ( registered )
  {
    int lod;
    reType = 2;
    dobj = 0;
    surfaceSlots = 0;
    for ( lod = 0; lod < registered->lodCount; ++lod )
      surfaceSlots += registered->lodSurfs[lod]->surfaceCount;
  }
  else
  {
    /* no shared template: build a DObj so the surfaces can be enumerated later */
    short  modelHandle;
    int    dobjModel[3];   /* DObjModel {model, tagName, short negativeModelIndex}: stride 12 */
    int    partBits[4];
    void  *evalStorage;
    DObj  *obj;

    reType = 1;
    obj = (DObj *)ri.Hunk_Alloc(88);
    modelHandle = RE_RegisterModel(va("xmodel/%s", source->name), (const char *)7);
    dobjModel[0] = (int)((model_t *)tr_models[modelHandle])->xmodel;
    dobjModel[1] = 0;
    *(short *)&dobjModel[2] = modelHandle;   /* retail: handle word at DObjModel+8 -> DObj.childModelIndices[0] */
    DObjCreate((int)dobjModel, 1u, 0, (int)obj, 0);
    dobj = obj;
    evalStorage = (void *)ri.Hunk_Alloc(96 * obj->partCount + 48);
    R_BIND_DOBJ_EVAL_STORAGE(obj, evalStorage);
    Com_Memset(partBits, 255, 0x10u);
    DObjCalcAnim((int)obj, partBits);
    DObjCalcSkel(partBits, (int)obj);
    surfaceSlots = 0;
  }

  model = (renderer_static_model_t *)ri.Hunk_Alloc(4 * surfaceSlots + 584);
  memset(model, 0, 0x9Cu);
  model->reType = reType;
  model->dobj = dobj;
  model->renderfx = 128;
  model->registered = source->registered;
  model->origin[0] = source->origin[0];
  model->origin[1] = source->origin[1];
  model->origin[2] = source->origin[2];
  model->axis[0][0] = source->axis[0][0];
  model->axis[0][1] = source->axis[0][1];
  model->axis[0][2] = source->axis[0][2];
  model->axis[1][0] = source->axis[1][0];
  model->axis[1][1] = source->axis[1][1];
  model->axis[1][2] = source->axis[1][2];
  model->axis[2][0] = source->axis[2][0];
  model->axis[2][1] = source->axis[2][1];
  model->axis[2][2] = source->axis[2][2];
  model->axisScale = source->axisScale;

  R_AdjustBoundsForAutosprite((int)model, source->boundsMin, source->boundsMax);
  model->localOrigin[0] = source->boundsMin[0] + source->boundsMax[0];
  model->localOrigin[1] = source->boundsMax[1] + source->boundsMin[1];
  model->localOrigin[2] = source->boundsMin[2] + source->boundsMax[2];
  model->localOrigin[0] = model->localOrigin[0] * 0.5f;
  model->localOrigin[1] = model->localOrigin[1] * 0.5f;
  model->localOrigin[2] = model->localOrigin[2] * 0.5f;

  model->color[0] = (byte)(int)( 255.0f < ( 0.0f > (tr_identityLight * source->color[0] * 255.0f) ? 0.0f : (tr_identityLight * source->color[0] * 255.0f) ) ? 255.0f : ( 0.0f > (tr_identityLight * source->color[0] * 255.0f) ? 0.0f : (tr_identityLight * source->color[0] * 255.0f) ) );
  model->color[1] = (byte)(int)( 255.0f < ( 0.0f > (tr_identityLight * source->color[1] * 255.0f) ? 0.0f : (tr_identityLight * source->color[1] * 255.0f) ) ? 255.0f : ( 0.0f > (tr_identityLight * source->color[1] * 255.0f) ? 0.0f : (tr_identityLight * source->color[1] * 255.0f) ) );
  model->color[2] = (byte)(int)( 255.0f < ( 0.0f > (tr_identityLight * source->color[2] * 255.0f) ? 0.0f : (tr_identityLight * source->color[2] * 255.0f) ) ? 255.0f : ( 0.0f > (tr_identityLight * source->color[2] * 255.0f) ? 0.0f : (tr_identityLight * source->color[2] * 255.0f) ) );
  model->color[3] = 255;

  model->bounds[0][0] = source->boundsMin[0];
  model->bounds[0][1] = source->boundsMin[1];
  model->bounds[0][2] = source->boundsMin[2];
  model->bounds[1][0] = source->boundsMax[0];
  model->bounds[1][1] = source->boundsMax[1];
  model->bounds[1][2] = source->boundsMax[2];
  model->viewCount = 0;

  R_FilterModelIntoCells_r_0(tr_world, *(mnode_t **)(tr_world + 152), (int)model,
                             model->bounds[0], model->bounds[1]);
  /* static light index / colour tables sit in model->lighting (0xbc..0x248) */
  result = R_GetStaticLightContributions(model->localOrigin, (char *)model + 0xc0,
                                         (float *)((char *)model + 0xbc), (char *)model + 0x184);
  model->lightContributions = result;
  return result;
}
#pragma optimize( "p", off )

/* ---- R_FinishLoadingStaticModels  0x005050A0 ----  VERIFIED
 * Finish every registered template that has no surfaces yet (build its DObj on
 * temp memory and hand it to R_FinishDObjToStaticModel), then place every pending
 * static model into the world and free the placement records.  Returns the last
 * ri.Free result (dead EAX). */
void R_FinishLoadingStaticModels(void)
{

  int                 i;
  worldStaticModel_t *pending;

  for ( i = 0; i < tr_registeredStaticModelCount; ++i )
  {
    smRegisteredModel_t *registered = (smRegisteredModel_t *)tr_registeredStaticModels[i];
    if ( !registered->lodSurfs[0] )   /* not finished yet */
    {
      short  modelHandle;
      int    dobjModel[3];   /* DObjModel {model, tagName, short negativeModelIndex}: stride 12 */
      DObj   dobj;            // byte v15[0x58] BYREF
      int    partBits[4];     // v14
      void *evalStorage;int skelSize;

      modelHandle = RE_RegisterModel(va("xmodel/%s", registered->name), (const char *)7);
      *(short *)&dobjModel[2] = modelHandle;   /* retail: handle word at DObjModel+8 -> DObj.childModelIndices[0] */
      dobjModel[0] = (int)((model_t *)tr_models[modelHandle])->xmodel;
      dobjModel[1] = 0;
      DObjCreate((int)dobjModel, 1u, 0, (int)&dobj, 0);
      skelSize=96*dobj.partCount+48;evalStorage=(void *)ri.Hunk_AllocateTempMemory(skelSize);
      DObjBindEvalStorage(evalStorage, &dobj);
      Com_Memset(partBits, 255, 0x10u);
      DObjCalcAnim((int)&dobj, partBits);
      DObjCalcSkel(partBits, (int)&dobj);
      R_FinishDObjToStaticModel((unsigned int *)tr_registeredStaticModels[i], (int)&dobj);
      ri.Hunk_FreeTempMemory((int)evalStorage);
    }

  }

  pending = (worldStaticModel_t *)rendererPendingStaticModels;
  while ( pending )
  {
    worldStaticModel_t *next = pending->next;
    R_AddStaticModelToWorld((int)pending);
    ri.Free((int)pending);
    pending = next;
  }
  rendererPendingStaticModels = 0;
  return;
}

/* ---- R_ShutdownStaticModels  0x00505220 ----  VERIFIED
 * Free every pending (not-yet-placed) static-model placement and clear the list.
 * Returns 0 (the tail of the list; dead EAX). */
int R_ShutdownStaticModels()
{
  int result = rendererPendingStaticModels;

  if ( rendererPendingStaticModels )
  {
    do
    {
      int next = (int)((worldStaticModel_t *)result)->next;
      ri.Free(result);
      result = next;
    }
    while ( result );
  }
  rendererPendingStaticModels = 0;
  return result;
}

/* ---- R_AddStaticModelSurfaces  0x00505260 ----  VERIFIED
 * Add a static-model instance's visible surfaces to the draw lists.  Picks a LOD
 * by distance, and for each merged surface: builds its lit vertex cache (unless
 * it needs per-frame lighting), stamps the cache page as used this frame, then
 * queues a draw surf -- from the cache when static, or the raw merged surface
 * when it must be re-lit or is not yet cached.  Feeds the tri/surf-count overlay
 * and queues entity lighting when any surface needs it. */
void __cdecl R_AddStaticModelSurfaces(int refEntity)
{
  trRefEntity_t       *entity = (trRefEntity_t *)refEntity;
  smRegisteredModel_t *registered = R_EntStaticModel(entity);
  smSurfaceList_t     *surfList;
  int                  lod;
  int                  surfaceBase;   /* surfaces in the LODs below this one */
  int                  lightingSetup;
  int                  needsLighting;
  int                  indexTotal;
  int                  i;
  float                inverseAxis[9];

  lod = XModelGetLodForDist(registered->model, R_GetLodDist((float *)entity));
  if ( lod < 0 )
    return;

  surfaceBase = 0;
  for ( i = 0; i < lod; ++i )
    surfaceBase += registered->lodSurfs[i]->surfaceCount;
  surfList = registered->lodSurfs[lod];

  lightingSetup = 0;
  needsLighting = 0;
  indexTotal = 0;
  for ( i = 0; i < surfList->surfaceCount; ++i )
  {
    /* cache the surface's lit vertices (once), unless it needs per-frame light */
    if ( !R_EntOwner(entity)->surfaceLightingCache[surfaceBase + i] && surfList->surfaces[i].cachedShader )
    {
      if ( !lightingSetup )
      {
        MatrixInverse(entity->e.axis, inverseAxis);
        R_SetupStaticModelLighting((int)&tr_refdef_x, (int)entity);
        lightingSetup = 1;
      }
      if ( !R_EntHasDynamicLights(entity) )
        R_EntOwner(entity)->surfaceLightingCache[surfaceBase + i] =
          (void *)R_CacheStaticModelSurface((int)&surfList->surfaces[i], (int)R_EntOwner(entity), surfaceBase + i,
                                            (int)entity, (int)inverseAxis);
    }

    if ( R_EntOwner(entity)->surfaceLightingCache[surfaceBase + i] )
    {
      R_UsedCachedStaticModelSurface((smcSurface_t *)R_EntOwner(entity)->surfaceLightingCache[surfaceBase + i]);
      if ( (surfList->surfaces[i].shader->lightingFlags & 0x18) != 0
        && (R_SetupStaticModelLighting((int)&tr_refdef_x, (int)entity), R_EntHasDynamicLights(entity)) )
      {
        /* dynamic light present: draw the raw merged surface so it is re-lit */
        R_AddDrawSurf(surfList->surfaces[i].storageMode, surfList->surfaces[i].shader, &surfList->surfaces[i], 0, 0, 0);
      }
      else
      {
        /* static: draw straight from the vertex cache */
        R_AddDrawSurf(tr_cachedStaticModelStorageSource, surfList->surfaces[i].cachedShader,
                      R_EntOwner(entity)->surfaceLightingCache[surfaceBase + i], 0, 0, 0);
      }
    }
    else
    {
      /* not cached: draw the raw merged surface, and flag if it wants lighting */
      R_AddDrawSurf(surfList->surfaces[i].storageMode, surfList->surfaces[i].shader, &surfList->surfaces[i], 0, 0, 0);
      if ( (surfList->surfaces[i].shader->lightingFlags & 0x18) != 0 )
        needsLighting = 1;
    }
    indexTotal += surfList->surfaces[i].indexCount;
  }

  /* r_showtricounts / r_showsurfcounts overlay above the model */
  if ( !r_showtricounts->integer || (entity->e.renderfx & 8) != 0 )
  {
    if ( r_showsurfcounts->integer && (entity->e.renderfx & 8) == 0 )
      R_AddScaledDebugString((int)entity->e.origin, (int)modelBoundsDebugColor,
                             (int)va("%i", surfList->surfaceCount));
  }
  else
  {
    R_AddScaledDebugString((int)entity->e.origin, (int)modelBoundsDebugColor,
                           (int)va("%i", indexTotal / 3));
  }

  if ( needsLighting )
    R_SetupStaticModelLighting((int)&tr_refdef_x, (int)entity);
}

/* ---- RB_SurfaceStaticModel  0x00505580 ----  VERIFIED
 * Tessellate an un-optimized merged surface: copy its positions / tex coords
 * (and, when the shader needs them, normals) into the tess buffers, then append
 * its re-based indices. */
void __cdecl RB_SurfaceStaticModel(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          i;

  if ( r_logFile->integer )
  {
    char *msg = va("--- RB_SurfaceStaticModel( %s ) ---\n", (const char *)tess_shader);
    if ( Stream )
      fprintf(Stream, "%s", (int)msg);
  }
  Com_Memcpy(4 * tess_numVertexes * tess_vertexComponentCount + (int)(char *)tess_xyz,
             (int)surf->vertices, 12 * surf->vertexCount);
  Com_Memcpy(8 * tess_numVertexes + (int)(char *)tess_texCoords0,
             (int)surf->texCoords, 8 * surf->vertexCount);
  if ( (((shader_t *)tess_shader)->surfaceFlags & 0x102) != 0 )
  {
    Com_Memcpy(12 * tess_numVertexes + (int)(char *)tess_stageNormals,
               (int)surf->normals, 12 * surf->vertexCount);
    tess_requiresVertexBasis = 1;
  }
  for ( i = 0; i < surf->indexCount; ++i )
    tess_indexes[tess_numIndexes + i] = tess_numVertexes + surf->indices[i];
  tess_numIndexes += surf->indexCount;
  tess_numVertexes += surf->vertexCount;
}

/* ---- RB_SurfaceStaticModelCached  0x005056A0 ----  VERIFIED
 * Emit a cached surface's indices into the optimized-index buffer, re-based onto
 * its cache vertex offset, and extend the batch's optimized-vertex span.  The
 * offset is packed into both halves of a dword so it adds to two 16-bit indices
 * per store.  Returns the packed offset (dead EAX). */
void __cdecl RB_SurfaceStaticModelCached(int surface)
{
  smcSurface_t *cached = (smcSurface_t *)surface;
  smSurface_t  *record;
  int           result;
  unsigned int *dst;
  unsigned int *src;
  int           pairs;

  if ( r_logFile->integer )
  {
    char *msg = va("--- RB_SurfaceStaticModelCached( %s ) ---\n", (const char *)tess_shader);
    if ( Stream )
      fprintf(Stream, "%s", (int)msg);
  }

  if ( !tess_optimizedVertexEnd )
  {
    tess_optimizedFirstVertex = cached->cached.vertexOffset;
    tess_optimizedVertexEnd = cached->cached.vertexOffset + ((smSurface_t *)cached->cached.source)->vertexCount;
  }
  else
  {
    if ( tess_optimizedFirstVertex > cached->cached.vertexOffset )
      tess_optimizedFirstVertex = cached->cached.vertexOffset;
    if ( tess_optimizedVertexEnd < cached->cached.vertexOffset + ((smSurface_t *)cached->cached.source)->vertexCount )
      tess_optimizedVertexEnd = cached->cached.vertexOffset + ((smSurface_t *)cached->cached.source)->vertexCount;
  }

  record = (smSurface_t *)cached->cached.source;
  pairs = record->indexCount / 6;
  result = cached->cached.vertexOffset | ( cached->cached.vertexOffset << 16 );
  src = (unsigned int *)record->indices;
  dst = (unsigned int *)((char *)&tess_optimizedIndexes + 2 * tess_renderedIndexCount);
  tess_renderedIndexCount += record->indexCount;
  do
  {
    *dst++ = result + *src++;
    *dst++ = result + *src++;
    *dst++ = result + *src++;
  }
  while ( --pairs );
  return;
}

/* ---- RB_SurfaceStaticModelT2V3_ARB  0x005057A0 ----  [HIGH]
 * Draw a T2V3 (tex+position) merged surface from its ARB vertex/index VBOs.  Per
 * stage: set state, bind textures, pick the vertex colour (entity RGBA for a
 * precalc-lit stage, else the stage constant), and draw; then the r_showtris
 * overlay.  Returns the running backEnd vertex count. */
int __cdecl RB_SurfaceStaticModelT2V3_ARB(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          i;
  int          texEnv[8];

  if ( Stream )
    fprintf(Stream, "%s", (int)"--- RB_SurfaceStaticModelT2V3_ARB ---\n");
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCullType(tess_shader));
  GL_ClientState(1025);
  qglBindBufferARB(34962, (int)surf->gpuVertexBuffer);
  qglBindBufferARB(34963, surf->gpuIndexBuffer);
  glVertexPointer(3, 0x1406u, 20, (const GLvoid *)8);
  texEnv[0] = 0;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    shaderStage_t *stage = *(shaderStage_t **)(tess_activeStages + 4 * i);

    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 20);
    stage = *(shaderStage_t **)(tess_activeStages + 4 * i);   /* retail re-reads the stage pointer */
    if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
    {
      if ( r_entFullbright->integer )
        qglColor3f(tr_identityLight, tr_identityLight, tr_identityLight);
      else
        glColor4ubv((const GLubyte *)(backEnd_currentEntity + 108));
    }
    else
    {
      glColor4ubv((const GLubyte *)stage->constantColor);
    }
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, 0);
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x005058ED */
    GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
    RB_BindAnimatedImage(&R_ShaderStages(tr_showTrisShader)[0]->bundle);
    if ( r_showtris->integer >= 5 )
    {
      float color;   /* block-scoped: shares the dead surface slot as in retail */

      RB_ChooseSurfaceCountColor(surf->indexCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      float color = tr_identityLight * 0.25f;

      qglColor3f(color, color, tr_identityLight);
    }
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, 0);
    glDepthRange(0.0, 1.0);
  }
  qglBindBufferARB(34962, 0);
  qglBindBufferARB(34963, 0);
  if ( Stream )
    fprintf(Stream, "%s", (int)"----------\n");
  backEnd_pc_indexCount += surf->indexCount;
  backEnd_pc_vertexCount += surf->vertexCount;
  return backEnd_pc_vertexCount;
}

/* ---- RB_SurfaceStaticModelT2N3V3_ARB  0x00505A30 ----  [HIGH]
 * Draw a T2N3V3 (tex+normal+position) merged surface from its ARB VBOs.  The
 * normals carry lighting, so each stage just sets state, binds textures and
 * draws; then the r_showtris overlay.  Returns the running vertex count. */
int __cdecl RB_SurfaceStaticModelT2N3V3_ARB(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          i;
  int          texEnv[8];

  if ( Stream )
    fprintf(Stream, "%s", (int)"--- RB_SurfaceStaticModelT2N3V3_ARB ---\n");
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCullType(tess_shader));
  GL_ClientState(1537);
  qglBindBufferARB(34962, (int)surf->gpuVertexBuffer);
  qglBindBufferARB(34963, surf->gpuIndexBuffer);
  glNormalPointer(0x1406u, 32, (const GLvoid *)8);
  glVertexPointer(3, 0x1406u, 32, (const GLvoid *)0x14);
  texEnv[0] = 0;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    shaderStage_t *stage = *(shaderStage_t **)(tess_activeStages + 4 * i);

    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 32);
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, 0);
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x00505B41 */
    GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
    RB_BindAnimatedImage(&R_ShaderStages(tr_showTrisShader)[0]->bundle);
    if ( r_showtris->integer >= 5 )
    {
      float color;   /* block-scoped: shares the dead surface slot as in retail */

      RB_ChooseSurfaceCountColor(surf->indexCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      float color = tr_identityLight * 0.25f;

      qglColor3f(color, color, tr_identityLight);
    }
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, 0);
    glDepthRange(0.0, 1.0);
  }
  qglBindBufferARB(34962, 0);
  qglBindBufferARB(34963, 0);
  if ( Stream )
    fprintf(Stream, "%s", (int)"----------\n");
  backEnd_pc_indexCount += surf->indexCount;
  backEnd_pc_vertexCount += surf->vertexCount;
  return backEnd_pc_vertexCount;
}

/* ---- RB_SurfaceStaticModelATI  0x00505C80 ----  VERIFIED
 * Draw a merged surface through the ATI vertex-array-object path (T2V3 or
 * T2N3V3).  With ATIElementArray the indices are an object buffer drawn by
 * glDrawElementArrayATI; otherwise they are drawn from system memory.  Per
 * stage: bind arrays/textures, colour (T2V3 only), draw; then r_showtris.
 * Returns the surface vertex count (backEnd_pc_vertexCount accumulates). */
int __cdecl RB_SurfaceStaticModelATI(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          stride;
  int          result;
  int          i;
  int          texEnv[8];   // v36


  if ( r_logFile->integer )
  {
    char *msg = va("--- RB_SurfaceStaticModelATI( %s ) ---\n", (const char *)tess_shader);
    if ( Stream )
      fprintf(Stream, "%s", (int)msg);
  }
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCullType(tess_shader));
  GL_ClientState(1025);
  texEnv[0] = surf->gpuIndexBuffer;
  if ( surf->surfaceType == SM_SURFACE_T2V3_ATI )
  {
    stride = 20;
    qglArrayObjectATI(32884, 3, 5126, 20, (int)surf->gpuVertexBuffer, surf->gpuIndexBuffer + 8);    /* positions */
  }
  else
  {
    stride = 32;
    qglArrayObjectATI(32885, 3, 5126, 32, (int)surf->gpuVertexBuffer, surf->gpuIndexBuffer + 8);    /* normals */
    qglArrayObjectATI(32884, 3, 5126, 32, (int)surf->gpuVertexBuffer, surf->gpuIndexBuffer + 20);   /* positions */
  }

  if ( glConfig_ATIElementArray )
  {
    glEnableClientState(0x8768u);
    qglArrayObjectATI(34664, 1, 5123, 0, (int)surf->gpuVertexBuffer, surf->gpuIndexOffset);         /* indices */
    for ( i = 0; i < tess_activeStageCount; ++i )
    {
      shaderStage_t *stage;
      GLsizei        n;

      if ( surf->surfaceType == SM_SURFACE_T2V3_ATI )
      {
        stage = *(shaderStage_t **)(tess_activeStages + 4 * i);
        if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
        {
          if ( r_entFullbright->integer )
            qglColor3f(tr_identityLight, tr_identityLight, tr_identityLight);
          else
            glColor4ubv((const GLubyte *)(backEnd_currentEntity + 108));
        }
        else
        {
          glColor4ubv((const GLubyte *)stage->constantColor);
        }
      }
      stage=*(shaderStage_t **)(tess_activeStages+4*i);
      GL_State(stage->stateBits);
      RB_SetupMultitextureATI(stage, (int)surf->gpuVertexBuffer, texEnv, stride);
      n = surf->indexCount;
      backEnd_pc_drawnIndexCount += n;
      ++backEnd_pc_drawCallCount;
      glDrawElementArrayATI(4, n);
    }
    if ( r_showtris->integer )
    {
      GLsizei n;

      if ( (r_showtris->integer & 1) != 0 )
        glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x00505E4F */
      GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
      RB_SetupMultitextureATI(R_ShaderStages(tr_showTrisShader)[0], (int)surf->gpuVertexBuffer, texEnv, stride);
      if ( r_showtris->integer >= 5 )
      {
        float color;
        RB_ChooseSurfaceCountColor(surf->indexCount, &color);
        glColor4ubv((const GLubyte *)&color);
      }
      else
      {
        float green = tr_identityLight * 0.75f;
        float red   = tr_identityLight * 0.5f;
        qglColor3f(red, green, tr_identityLight);
      }
      n = surf->indexCount;
      backEnd_pc_drawnIndexCount += n;
      ++backEnd_pc_drawCallCount;
      glDrawElementArrayATI(4, n);
      glDepthRange(0.0, 1.0);
    }
    glDisableClientState(0x8768u);
  }
  else
  {
    for ( i = 0; i < tess_activeStageCount; ++i )
    {
      shaderStage_t *stage;
      GLsizei        n;

      if ( surf->surfaceType == SM_SURFACE_T2V3_ATI )
      {
        stage = *(shaderStage_t **)(tess_activeStages + 4 * i);
        if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
        {
          if ( r_entFullbright->integer )
            qglColor3f(tr_identityLight, tr_identityLight, tr_identityLight);
          else
            glColor4ubv((const GLubyte *)(backEnd_currentEntity + 108));
        }
        else
        {
          glColor4ubv((const GLubyte *)stage->constantColor);
        }
      }
      stage=*(shaderStage_t **)(tess_activeStages+4*i);
      GL_State(stage->stateBits);
      RB_SetupMultitextureATI(stage, (int)surf->gpuVertexBuffer, texEnv, stride);
      n = surf->indexCount;
      backEnd_pc_drawnIndexCount += n;
      ++backEnd_pc_drawCallCount;
      glDrawElements(4u, n, 0x1403u, (const GLvoid *)surf->indices);
    }
    if ( r_showtris->integer )
    {
      GLsizei n;

      if ( (r_showtris->integer & 1) != 0 )
        glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x00506009 */
      GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
      RB_SetupMultitextureATI(R_ShaderStages(tr_showTrisShader)[0], (int)surf->gpuVertexBuffer, texEnv, stride);
      if ( r_showtris->integer >= 5 )
      {
        float color;
        RB_ChooseSurfaceCountColor(surf->indexCount, &color);
        glColor4ubv((const GLubyte *)&color);
      }
      else
      {
        float green = tr_identityLight * 0.75f;
        float red   = tr_identityLight * 0.5f;
        qglColor3f(red, green, tr_identityLight);
      }
      n = surf->indexCount;
      backEnd_pc_drawnIndexCount += n;
      ++backEnd_pc_drawCallCount;
      glDrawElements(4u, n, 0x1403u, (const GLvoid *)surf->indices);
      glDepthRange(0.0, 1.0);
    }
  }
  if ( Stream )
    fprintf(Stream, "%s", (int)"----------\n");
  backEnd_pc_indexCount += surf->indexCount;
  result = surf->vertexCount;
  backEnd_pc_vertexCount += result;
  return result;
}

/* ---- RB_SurfaceStaticModelT2V3_NV  0x00506130 ----  VERIFIED
 * Draw a T2V3 merged surface through the NV vertex-array-range path (vertices in
 * a raw pointer buffer, indices in system memory). */
int __cdecl RB_SurfaceStaticModelT2V3_NV(int surface)
{
  smSurface_t   *surf = (smSurface_t *)surface;
  int            i;
  int            result;
  int            texEnv[8];   // v15
  shaderStage_t *stage;

  if ( Stream )
    fprintf(Stream, "%s", (int)"--- RB_SurfaceStaticModelT2V3_NV ---\n");
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCullType(tess_shader));
  GL_ClientState(1025);
  glVertexPointer(3, 0x1406u, 20, (const GLvoid *)((char *)surf->gpuVertexBuffer + 8));
  texEnv[0] = (int)surf->gpuVertexBuffer;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(shaderStage_t **)(tess_activeStages + 4 * i);
    if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
    {
      if ( r_entFullbright->integer )
        qglColor3f(tr_identityLight, tr_identityLight, tr_identityLight);
      else
        glColor4ubv((const GLubyte *)(backEnd_currentEntity + 108));
    }
    else
    {
      glColor4ubv((const GLubyte *)stage->constantColor);
    }
    stage = *(shaderStage_t **)(tess_activeStages + 4 * i);   /* retail re-reads the stage pointer */
    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 20);
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x0050626D */
    stage = R_ShaderStages(tr_showTrisShader)[0];
    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 20);
    if ( r_showtris->integer >= 5 )
    {
      float color;   /* block-scoped: shares the dead surface slot as in retail */

      RB_ChooseSurfaceCountColor(surf->indexCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      qglColor3f(tr_identityLight * 0.5f, tr_identityLight * 0.75f, tr_identityLight);
    }
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
    glDepthRange(0.0, 1.0);
  }
  if ( Stream )
    fprintf(Stream, "%s", (int)"----------\n");
  backEnd_pc_indexCount += surf->indexCount;
  result = surf->vertexCount;
  backEnd_pc_vertexCount += result;
  return result;
}

/* ---- RB_SurfaceStaticModelT2N3V3_Generic  0x00506390 ----  VERIFIED
 * Draw a T2N3V3 merged surface through the fixed-function path (interleaved
 * pointer arrays, indices in system memory).  Normals carry lighting, so stages
 * just set state and draw; r_showtris binds the debug image. */
int __cdecl RB_SurfaceStaticModelT2N3V3_Generic(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          i;
  int          result;
  int          texEnv[8];   // v12

  if ( Stream )
    fprintf(Stream, "%s", (int)"--- RB_SurfaceStaticModelT2N3V3_Generic ---\n");
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCullType(tess_shader));
  GL_ClientState(1537);
  glNormalPointer(0x1406u, 32, (const GLvoid *)((char *)surf->gpuVertexBuffer + 8));
  glVertexPointer(3, 0x1406u, 32, (const GLvoid *)((char *)surf->gpuVertexBuffer + 20));
  texEnv[0] = (int)surf->gpuVertexBuffer;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    shaderStage_t *stage = *(shaderStage_t **)(tess_activeStages + 4 * i);

    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 32);
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x00506493 */
    GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
    RB_BindAnimatedImage(&R_ShaderStages(tr_showTrisShader)[0]->bundle);
    if ( r_showtris->integer >= 5 )
    {
      float color;   /* block-scoped: shares the dead surface slot as in retail */

      RB_ChooseSurfaceCountColor(surf->indexCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      float color = tr_identityLight * 0.25f;

      qglColor3f(color, color, tr_identityLight);
    }
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
    glDepthRange(0.0, 1.0);
  }
  if ( Stream )
    fprintf(Stream, "%s", (int)"----------\n");
  backEnd_pc_indexCount += surf->indexCount;
  result = surf->vertexCount;
  backEnd_pc_vertexCount += result;
  return result;
}

/* ---- RB_SurfaceStaticModelT2V3_Generic  0x005065C0 ----  VERIFIED
 * Fixed-function T2V3 draw (same shape as the NV T2V3 path). */
int __cdecl RB_SurfaceStaticModelT2V3_Generic(int surface)
{
  smSurface_t   *surf = (smSurface_t *)surface;
  int            i;
  int            result;
  int            texEnv[8];   // v15
  shaderStage_t *stage;

  if ( Stream )
    fprintf(Stream, "%s", (int)"--- RB_SurfaceStaticModelT2V3_Generic ---\n");
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCullType(tess_shader));
  GL_ClientState(1025);
  glVertexPointer(3, 0x1406u, 20, (const GLvoid *)((char *)surf->gpuVertexBuffer + 8));
  texEnv[0] = (int)surf->gpuVertexBuffer;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(shaderStage_t **)(tess_activeStages + 4 * i);
    if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
    {
      if ( r_entFullbright->integer )
        qglColor3f(tr_identityLight, tr_identityLight, tr_identityLight);
      else
        glColor4ubv((const GLubyte *)(backEnd_currentEntity + 108));
    }
    else
    {
      glColor4ubv((const GLubyte *)stage->constantColor);
    }
    stage = *(shaderStage_t **)(tess_activeStages + 4 * i);   /* retail re-reads the stage pointer */
    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 20);
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x005066FD */
    stage = R_ShaderStages(tr_showTrisShader)[0];
    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 20);
    if ( r_showtris->integer >= 5 )
    {
      float color;   /* block-scoped: shares the dead surface slot as in retail */

      RB_ChooseSurfaceCountColor(surf->indexCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      qglColor3f(tr_identityLight * 0.5f, tr_identityLight * 0.75f, tr_identityLight);
    }
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
    glDepthRange(0.0, 1.0);
  }
  if ( Stream )
    fprintf(Stream, "%s", (int)"----------\n");
  backEnd_pc_indexCount += surf->indexCount;
  result = surf->vertexCount;
  backEnd_pc_vertexCount += result;
  return result;
}

/* ---- RB_SurfaceStaticModelT2N3V3_NV  0x00506820 ----  VERIFIED
 * NV T2N3V3 draw (same shape as the fixed-function T2N3V3 path). */
int __cdecl RB_SurfaceStaticModelT2N3V3_NV(int surface)
{
  smSurface_t *surf = (smSurface_t *)surface;
  int          i;
  int          result;
  int          texEnv[8];   // v12

  if ( Stream )
    fprintf(Stream, "%s", (int)"--- RB_SurfaceStaticModelT2N3V3_NV ---\n");
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCullType(tess_shader));
  GL_ClientState(1537);
  glNormalPointer(0x1406u, 32, (const GLvoid *)((char *)surf->gpuVertexBuffer + 8));
  glVertexPointer(3, 0x1406u, 32, (const GLvoid *)((char *)surf->gpuVertexBuffer + 20));
  texEnv[0] = (int)surf->gpuVertexBuffer;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    shaderStage_t *stage = *(shaderStage_t **)(tess_activeStages + 4 * i);

    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 32);
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x00506923 */
    GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
    RB_BindAnimatedImage(&R_ShaderStages(tr_showTrisShader)[0]->bundle);
    if ( r_showtris->integer >= 5 )
    {
      float color;   /* block-scoped: shares the dead surface slot as in retail */

      RB_ChooseSurfaceCountColor(surf->indexCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      float color = tr_identityLight * 0.25f;

      qglColor3f(color, color, tr_identityLight);
    }
    GL_DrawElements(surf->indexCount, 4u, 0x1403u, surf->indices);
    glDepthRange(0.0, 1.0);
  }
  if ( Stream )
    fprintf(Stream, "%s", (int)"----------\n");
  backEnd_pc_indexCount += surf->indexCount;
  result = surf->vertexCount;
  backEnd_pc_vertexCount += result;
  return result;
}

/* ---- R_RefreshStaticModels_ARB  0x00506A50 ----  VERIFIED
 * Re-upload every ARB-optimized merged surface's vertex and/or index VBO from
 * its source arrays (refreshFlags bit 1 = vertices, bit 2 = indices).  Non-ARB surfaces
 * (no VBOs) are skipped.  Returns tr_registeredStaticModelCount (dead EAX). */
void __cdecl R_RefreshStaticModels_ARB(char refreshFlags)
{

  int  m;
  smVertexT2V3_t   staging20[8192];   // v25  T2V3 staging (vertex arrays, not char[]: no /GS cookie)
  smVertexT2N3V3_t staging32[8192];   // v27  T2N3V3 staging

  for ( m = 0; m < tr_registeredStaticModelCount; ++m )
  {
    int lod;

    for ( lod = 0; lod < ((smRegisteredModel_t *)tr_registeredStaticModels[m])->lodCount; ++lod )
    {
      smSurfaceList_t *surfList = ((smRegisteredModel_t *)tr_registeredStaticModels[m])->lodSurfs[lod];
      int              s;

      for ( s = 0; s < surfList->surfaceCount; ++s )
      {
        smSurface_t *record = &surfList->surfaces[s];
        int          i;

        if ( !record->gpuVertexBuffer || !record->gpuIndexBuffer )
          continue;

        if ( (refreshFlags & 2) != 0 )
        {
          qglBindBufferARB(34963, record->gpuIndexBuffer);
          qglBufferDataARB(34963, 2 * record->indexCount, (int)record->indices, 35044);
          qglBindBufferARB(34963, 0);
        }
        if ( (refreshFlags & 1) != 0 )
        {
          qglBindBufferARB(34962, (int)record->gpuVertexBuffer);
          if ( record->surfaceType == SM_SURFACE_T2V3_ARB )
          {
            int          vertexCount = record->vertexCount;
            const float *tc  = record->texCoords[0];
            const float *xyz = record->vertices[0];
            for ( i = 0; i < vertexCount; ++i )
            {
              staging20[i].texCoord[0] = tc[0];
              staging20[i].texCoord[1] = tc[1];
              tc += 2;
              staging20[i].position[0] = xyz[0];
              staging20[i].position[1] = xyz[1];
              staging20[i].position[2] = xyz[2];
              xyz += 3;
            }
            qglBufferDataARB(34962, 20 * record->vertexCount, (int)staging20, 35044);
          }
          else
          {
            int          vertexCount = record->vertexCount;
            const float *tc  = record->texCoords[0];
            const float (*nrm)[3] = (const float (*)[3])record->normals;
            const float (*xyz)[3] = (const float (*)[3])record->vertices;
            for ( i = 0; i < vertexCount; ++i, tc += 2 )
            {
              staging32[i].texCoord[0] = tc[0];
              staging32[i].texCoord[1] = tc[1];
              staging32[i].normal[0]   = nrm[i][0];
              staging32[i].normal[1]   = nrm[i][1];
              staging32[i].normal[2]   = nrm[i][2];
              staging32[i].position[0] = xyz[i][0];
              staging32[i].position[1] = xyz[i][1];
              staging32[i].position[2] = xyz[i][2];
            }
            qglBufferDataARB(34962, 32 * record->vertexCount, (int)staging32, 35044);
          }
          qglBindBufferARB(34962, 0);
        }
      }
    }

  }
  return;
}

/* ---- R_IncrementalRefreshStaticModels_ARB  0x00506C80 ----  VERIFIED
 * Re-upload at most one ARB merged surface's VBOs per call, advancing the
 * persistent (model, LOD, surface) cursor first (so surface 0 of each LOD is
 * skipped, as retail does) and orphaning each buffer before the refill.  Stops
 * when the cursor wraps all the way around without a refreshable surface. */
void __cdecl R_IncrementalRefreshStaticModels_ARB(char refreshFlags)
{
  int  modelIndex;
  int  lodIndex;
  int  surfaceIndex;
  int  startModel;

  if ( !tr_registeredStaticModelCount || rendererPendingStaticModels )
    return;

  startModel = tr_staticModelRefreshModelIndex;
  modelIndex = startModel;
  lodIndex = tr_staticModelRefreshLodIndex;
  surfaceIndex = tr_staticModelRefreshSurfaceIndex;

  while ( 1 )
  {
    smSurface_t *record;
    int          i;

    /* tr.staticModels[] is re-read after each cursor store (one tr object), as retail */
    tr_staticModelRefreshSurfaceIndex = ++surfaceIndex;
    if ( surfaceIndex >= ((smRegisteredModel_t *)tr_registeredStaticModels[modelIndex])->lodSurfs[lodIndex]->surfaceCount )
    {
      surfaceIndex = 0;
      ++lodIndex;
      tr_staticModelRefreshSurfaceIndex = surfaceIndex;
      tr_staticModelRefreshLodIndex = lodIndex;
      if ( lodIndex >= ((smRegisteredModel_t *)tr_registeredStaticModels[modelIndex])->lodCount )
      {
        lodIndex = 0;
        ++modelIndex;
        tr_staticModelRefreshLodIndex = lodIndex;
        tr_staticModelRefreshModelIndex = modelIndex;
        if ( modelIndex >= tr_registeredStaticModelCount )
        {
          modelIndex = 0;
          tr_staticModelRefreshModelIndex = modelIndex;
        }
        if ( modelIndex == startModel )
          break;
      }
    }

    record = &((smRegisteredModel_t *)tr_registeredStaticModels[modelIndex])->lodSurfs[lodIndex]->surfaces[surfaceIndex];
    if ( record->gpuVertexBuffer && record->gpuIndexBuffer )
    {
      if ( (refreshFlags & 2) != 0 )
      {
        qglBindBufferARB(34963, record->gpuIndexBuffer);
        qglBufferDataARB(34963, 2 * record->indexCount, 0, 35044);
        qglBufferDataARB(34963, 2 * record->indexCount, (int)record->indices, 35044);
        qglBindBufferARB(34963, 0);
      }
      if ( (refreshFlags & 1) != 0 )
      {
        qglBindBufferARB(34962, (int)record->gpuVertexBuffer);
        /* per-format block-scoped staging arrays (retail overlaps them in one 0x40000 slot) */
        if ( record->surfaceType == SM_SURFACE_T2V3_ARB )
        {
          smVertexT2V3_t staging[0x40000 / sizeof(smVertexT2V3_t)];

          for ( i = 0; i < record->vertexCount; ++i )
          {
            staging[i].texCoord[0] = record->texCoords[i][0];
            staging[i].texCoord[1] = record->texCoords[i][1];
            staging[i].position[0] = record->vertices[i][0];
            staging[i].position[1] = record->vertices[i][1];
            staging[i].position[2] = record->vertices[i][2];
          }
          qglBufferDataARB(34962, 20 * record->vertexCount, 0, 35044);
          qglBufferDataARB(34962, 20 * record->vertexCount, (int)staging, 35044);
        }
        else
        {
          smVertexT2N3V3_t staging[0x40000 / sizeof(smVertexT2N3V3_t)];

          for ( i = 0; i < record->vertexCount; ++i )
          {
            staging[i].texCoord[0] = record->texCoords[i][0];
            staging[i].texCoord[1] = record->texCoords[i][1];
            staging[i].normal[0]   = record->normals[i][0];
            staging[i].normal[1]   = record->normals[i][1];
            staging[i].normal[2]   = record->normals[i][2];
            staging[i].position[0] = record->vertices[i][0];
            staging[i].position[1] = record->vertices[i][1];
            staging[i].position[2] = record->vertices[i][2];
          }
          qglBufferDataARB(34962, 32 * record->vertexCount, 0, 35044);
          qglBufferDataARB(34962, 32 * record->vertexCount, (int)staging, 35044);
        }
        qglBindBufferARB(34962, 0);
      }
      return;
    }
  }
}
