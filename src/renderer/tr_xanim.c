/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_xanim.c -- CoD skeletal animation (XAnim / DObj) surface submission and
 * rigid/weighted vertex skinning.  This has no RTCW analogue: CoD1 models are
 * XModel-based and animated through a DObj (an instanced skeleton whose bone
 * matrices live in evalStorage).  R_AddXModelSurfaces expands an animated
 * entity into per-surface entity draw surfs; the RB_SurfaceXModel* backends
 * then transform each surface's verts by the bone matrices (rigid, weighted,
 * and the SSE / ARB / ATI / NV fast paths).
 *
 * The pointer-passed animation records (DObj, the XModel -> XModelInfo ->
 * XModelLodInfo -> XModelSurfs -> XSurface chain, and the rigid/blend vertex
 * streams) are typed file-locally below; every offset is proven against retail
 * CoDMP.exe (IDA struct db) and cross-checked with CoDUO, and pinned with
 * size/offset asserts.  shader_t / model_t / trRefEntity_t come from
 * tr_local.h.  The XModel chain duplicates tr_model.c's file-local copy on
 * purpose (separate translation units); tr_local.h is intentionally not
 * touched.  Bone-matrix pointer arithmetic (evalStorage + partBase*64 + 0x30 +
 * surfaceBoneOffset) is kept as an explicit expression because the +0x30 skew
 * does not map cleanly onto a DObjSkelMat field.
 */

#include <malloc.h>
#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_shaderregistry.h"
#include "tr_gl_types.h"
#include "tr_orientation.h"   /* backEnd.or 0x016D8DA8 -- one object, not 16 shards */
#include "tr_tess.h"
#include "tr_local.h"

extern char xmodel_defaultCollision[];

extern int GL_Bind();
extern int GL_ClientState();
extern int GL_Cull();
extern int GL_State();
extern int RB_CheckOverflow();
extern int RB_ChooseSurfaceCountColor();
extern int RB_EndMultitexture();
extern int RB_GetAnimatedImage();
extern void __cdecl RB_SetIteratorFog( void );
extern int RB_SetupMultitexture();
extern int R_AddDrawSurf();
extern int R_AddScaledDebugString();
extern void R_SetupEntityLighting();
extern char *__cdecl R_GetShaderByHandle(int hShader);
extern void XSurfaceGetTris(unsigned int out, int surfacePtr, unsigned short indexOffset);
extern int DObjBackfillParentPartBits();
extern int DObjBuildPartCollisionTable();
extern int DObjGetNumSurfaces();
extern void DObjGetSurfaces();
extern int DObjHasDefaultModel();
extern const char *XModelGetName();
extern int __cdecl XModelGetLodForDist( const void *model, float distance );

extern double __cdecl R_GetLodDist( float *entityOrigin );

#define backEnd_modelMatrix   ((GLfloat *)(void *)&backEnd_or_modelMatrix)

/* ------------------------------------------------------------------------- *
 *  CoD skeletal-animation data (file-local typed views).
 *
 *  XModel (0x0c) -> XModelInfo (0x68) -> XModelLodInfo[3] (0x14 each) ->
 *  XModelSurfs (0x0c) -> XModelSurfsData (0x0c) -> XSurface* []; the DObj
 *  (0x58) instances the skeleton and points every surface at its child XModel
 *  and bone matrices; rigid/blend vertex streams feed the transforms.
 * ------------------------------------------------------------------------- */
typedef struct XModel_s XModel;

typedef struct {
	unsigned char  tileMode;          /* +0x00 */
	unsigned char  pad01;             /* +0x01 */
	short          vertexCount;       /* +0x02 */
	short          triangleCount;     /* +0x04 */
	short          boneMatrixOffset;  /* +0x06  0xFFFF => weighted (non-rigid) surface */
	unsigned int   boneUsage[4];      /* +0x08 */
	void          *weightedPoints;    /* +0x18  XSurfaceWeightedPoint[] */
	void          *triangles;         /* +0x1c  unsigned short[3][] */
	void          *vertexData;        /* +0x20  XSurfaceRigidVert[] or XSurfaceBlendVert[] */
	void          *texCoords;         /* +0x24  float[2][] */
	void          *optimizedDataARB;  /* +0x28 */
	void          *optimizedDataATI;  /* +0x2c */
	void          *optimizedDataNV;   /* +0x30 */
} XSurface;
COD1_ASSERT_SIZE(XSurface, 0x34);
ASSERT_OFF(XSurface, vertexCount, 0x02);
ASSERT_OFF(XSurface, triangleCount, 0x04);
ASSERT_OFF(XSurface, boneMatrixOffset, 0x06);
ASSERT_OFF(XSurface, weightedPoints, 0x18);
ASSERT_OFF(XSurface, triangles, 0x1c);
ASSERT_OFF(XSurface, vertexData, 0x20);
ASSERT_OFF(XSurface, texCoords, 0x24);
ASSERT_OFF(XSurface, optimizedDataARB, 0x28);
ASSERT_OFF(XSurface, optimizedDataATI, 0x2c);
ASSERT_OFF(XSurface, optimizedDataNV, 0x30);

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

/* DObj -- an instanced skeleton.  evalStorage holds the DObjSkelMat[] bone
 * matrices; childModelIndices maps a child slot to a model handle (negative
 * => -handle is a game model); childPartBaseIndices gives each child's bone
 * base index into evalStorage. */
typedef struct {
	void          *tree;                       /* +0x00  XAnimTree* */
	void          *evalStorage;                /* +0x04  DObjSkelMat[] bone matrices */
	int            skelCacheKey;               /* +0x08 */
	unsigned char *partRemapTable;             /* +0x0c */
	unsigned short unknownState10;             /* +0x10 */
	unsigned short scrNotifyId;                /* +0x12 */
	unsigned short tracePartRemapHandle;       /* +0x14 */
	unsigned char  childCount;                 /* +0x16 */
	unsigned char  partCount;                  /* +0x17 */
	XModel        *childRefs[8];               /* +0x18 */
	short          childModelIndices[8];       /* +0x38  <0 => -index is a game model */
	unsigned char  childParentPartIndices[8];  /* +0x48 */
	unsigned char  childPartBaseIndices[8];    /* +0x50  bone-matrix base per child */
} DObj;
COD1_ASSERT_SIZE(DObj, 0x58);
ASSERT_OFF(DObj, evalStorage, 0x04);
ASSERT_OFF(DObj, childCount, 0x16);
ASSERT_OFF(DObj, partCount, 0x17);
ASSERT_OFF(DObj, childRefs, 0x18);
ASSERT_OFF(DObj, childModelIndices, 0x38);
ASSERT_OFF(DObj, childPartBaseIndices, 0x50);

/* one bone influence: a local position plus the byte offset of its bone matrix
 * within DObj.evalStorage */
typedef struct {
	float        position[3];        /* +0x00 */
	unsigned int boneMatrixOffset;   /* +0x0c */
} XSimpleBlendInfo;
COD1_ASSERT_SIZE(XSimpleBlendInfo, 0x10);

/* rigid vertex (0x18): one bone, normal + position */
typedef struct {
	float normal[3];                 /* +0x00 */
	float position[3];               /* +0x0c */
} XSurfaceRigidVert;
COD1_ASSERT_SIZE(XSurfaceRigidVert, 0x18);

/* blend vertex (variable stride): normal, primary bone influence, and (when
 * additiveWeightCount != 0) a trailing primaryWeight -- the stream advances 32
 * bytes without additive weights and 36 bytes with them */
typedef struct {
	float            normal[3];            /* +0x00 */
	int              additiveWeightCount;  /* +0x0c */
	XSimpleBlendInfo blend;                /* +0x10  primary bone influence */
	float            primaryWeight;        /* +0x20  present only when additive */
} XSurfaceBlendVert;
COD1_ASSERT_SIZE(XSurfaceBlendVert, 0x24);
ASSERT_OFF(XSurfaceBlendVert, additiveWeightCount, 0x0c);
ASSERT_OFF(XSurfaceBlendVert, blend, 0x10);
ASSERT_OFF(XSurfaceBlendVert, primaryWeight, 0x20);

/* one additional weighted bone influence for a blend vertex (0x14) */
typedef struct {
	XSimpleBlendInfo blend;          /* +0x00 */
	float            weight;         /* +0x10 */
} XSurfaceWeightedPoint;
COD1_ASSERT_SIZE(XSurfaceWeightedPoint, 0x14);
ASSERT_OFF(XSurfaceWeightedPoint, weight, 0x10);

/* entity surface draw record (0x10) built by R_AddEntityDrawSurf and consumed
 * by the RB_SurfaceXModel* backends.  surfaceType selects the backend (4/5 =
 * rigid + rigid SSE, 6/7/8 = rigid ARB/ATI/NV, 9/10 = weighted + weighted SSE). */
typedef struct {
	int        surfaceType;   /* +0x00 */
	DObj      *dobj;          /* +0x04 */
	XSurface  *surf;          /* +0x08 */
	int        childIndex;    /* +0x0c  DObj child slot */
} xmodelDrawSurf_t;
COD1_ASSERT_SIZE(xmodelDrawSurf_t, 0x10);
ASSERT_OFF(xmodelDrawSurf_t, dobj, 0x04);
ASSERT_OFF(xmodelDrawSurf_t, surf, 0x08);
ASSERT_OFF(xmodelDrawSurf_t, childIndex, 0x0c);

/* one DObjGetSurfaces output entry: (child slot, surface index in that child's LOD) */
typedef struct {
	short childIndex;     /* +0x00 */
	short surfaceIndex;   /* +0x02 */
} DObjSurfaceRef;
COD1_ASSERT_SIZE(DObjSurfaceRef, 0x04);

/* one shader stage (0x688); interior layout is disputed across the shade TUs,
 * so only the two fields this file touches are named. */
typedef struct {
	unsigned int  attributeBits;         /* +0x000 */
	int           bundle;                /* +0x004  first textureBundle (image / anim) */
	byte          bundleRest[0x644 - 0x008]; /* rest of textureBundle_t bundles[8] (tr_shade.c) */
	int           atiFragmentShader;      /* +0x644 */
	void         *registerCombiners;      /* +0x648 */
	char         *vertexProgram;          /* +0x64c */
	byte          rgbWave[0x14];          /* +0x650  waveForm_t */
	int           rgbGen;                 /* +0x664 */
	byte          alphaWave[0x14];        /* +0x668  waveForm_t */
	int           alphaGen;               /* +0x67c */
	byte          constantColor[4];       /* +0x680 */
	unsigned int  stateBits;             /* +0x684  GL_State bits */
} shaderStage_t;
COD1_ASSERT_SIZE(shaderStage_t, 0x688);
ASSERT_OFF(shaderStage_t, stateBits, 0x684);

/* refEntity_t anim fields inside tr_local.h's unproven 0x84..0x9b range
 * (proven here: R_AddXModelSurfaces / R_CullModel against retail). */
#define R_EntDObj(ent)   ((DObj *)(ent)->e.dobj)
#define R_EntPose(ent)   ((int)(ent)->e.owner)

/* shader_t fields past tr_local.h's named prefix, reached on the shader the
 * tess / globals macros hold; the disputed stage interior stays off. */
#define R_ShaderCull(sh)       (((shader_t *)(sh))->cullType)
#define R_ShaderNumStages(sh)  (((shader_t *)(sh))->numUnfoggedPasses)
#define R_ShaderStages(sh)     ((shaderStage_t **)((shader_t *)(sh))->stages)

/* ---- R_CullModel  0x0050DB00 ----  VERIFIED */
int __cdecl R_CullModel(int unused, trRefEntity_t *entity)
{
  if ( !r_drawXModels->integer )
    return CULL_OUT;
  return entity->cullState;
}

/* ---- R_AddEntityDrawSurf  0x0050DB20 ----  VERIFIED
 * Build one entity draw surf for an XModel surface: pick the backend (rigid /
 * weighted, plus SSE / ARB / ATI / NV variants), pack the sort key, and (for
 * stencil-shadow shaders) also emit the shadow draw surf.  Returns cg_shadows
 * as dead EAX. */
cvar_t *__cdecl R_AddEntityDrawSurf(XSurface *surf, shader_t *shader, DObj *dobj, int childIndex)
{
  xmodelDrawSurf_t *ds;
  int               storage;
  cvar_t           *result;

  storage = storageClass;
  ds = (xmodelDrawSurf_t *)(tr_refdef_entitySurfaces + 16 * tr_refdef_entitySurfaceCount);
  if ( surf->boneMatrixOffset == -1 )
  {
    ds->surfaceType = (sys_hasSSE != 0) + 9;   /* weighted (skinned) */
    goto emit;
  }
  ds->surfaceType = (sys_hasSSE != 0) + 4;      /* rigid */
  if ( (shader->surfaceFlags & 0x3FF7FC00) == 0 )
  {
    if ( surf->optimizedDataARB )
    {
      ds->surfaceType = 6;
      storage = storageClass;
      goto emit;
    }
    if ( surf->optimizedDataNV )
    {
      ds->surfaceType = 8;
      storage = *(int *)surf->optimizedDataNV;
    }
    else
    {
      if ( !surf->optimizedDataATI )
        goto emit;
      ds->surfaceType = 7;
      storage = *(int *)surf->optimizedDataATI;
    }
  }
emit:
  ds->dobj = dobj;
  ds->surf = surf;
  ds->childIndex = childIndex;
  if ( tr_refdef_numDrawSurfs < 0x10000 )
  {
    /* indexed afresh per store: retail re-reads numDrawSurfs / drawSurfs */
    ((drawSurf_t *)tr_refdef_drawSurfs)[tr_refdef_numDrawSurfs].sort =
        tr_shiftedEntityNumber | (((storage << 12) | shader->sortedIndex) << 18) | 4;
    ((drawSurf_t *)tr_refdef_drawSurfs)[tr_refdef_numDrawSurfs].surface = ds;
    ++tr_refdef_numDrawSurfs;
  }
  result = cg_shadows;
  ++tr_refdef_entitySurfaceCount;
  if ( cg_shadows->integer == 2 && shader->sort == 5.0f )   /* stencil-shadow sort class */
  {
    R_AddDrawSurf(storage, tr_stencilShadowShader, (int)ds, 0, 0, 0);
    return (cvar_t *)++tr_refdef_entitySurfaceCount;
  }
  return result;
}

/* ---- R_DObjGetSurfIndex  0x0050DC40 ----  VERIFIED
 * Map a DObj child slot to a model handle.  A negative table entry is the
 * negated handle of a game-supplied model. */
int __cdecl R_DObjGetSurfIndex(DObj *dobj, int childSlot)
{
  short handle = dobj->childModelIndices[childSlot];
  int   index = handle;

  if ( index >= 0 )
    return index;
  return (short)ri.CG_GetGameModel(-handle);
}

/* ---- RB_TransformRigidVertex  0x0050DC60 ----  VERIFIED
 * Transform a rigid vertex position + normal by one bone matrix.  The
 * parameter order (not the operand order) fixes VC7's float-sum layout. */
float *__cdecl RB_TransformRigidVertex(
        float *position,
        float *normal,
        float *matrix,
        float *outPos,
        float *outNormal)
{
  outPos[0]    = position[1] * matrix[4] + position[0] * matrix[0]  + matrix[8]  * position[2] + matrix[12];
  outPos[1]    = position[1] * matrix[5] + matrix[9]  * position[2] + matrix[1]  * position[0] + matrix[13];
  outPos[2]    = position[1] * matrix[6] + matrix[10] * position[2] + matrix[2]  * position[0] + matrix[14];
  outNormal[0] = normal[1]   * matrix[4] + normal[0]  * matrix[0]   + normal[2]  * matrix[8];
  outNormal[1] = matrix[1]   * normal[0] + matrix[9]  * normal[2]   + normal[1]  * matrix[5];
  outNormal[2] = matrix[2]   * normal[0] + matrix[10] * normal[2]   + matrix[6]  * normal[1];
  return matrix;
}

/* ---- RB_AccumulateWeightedPoint  0x0050DD00 ----  VERIFIED
 * Add one weighted bone influence to an already-transformed vertex position.
 * Dead in retail (zero xrefs): RB_SurfaceXModelWeight runs its own inlined
 * copy, whose y/z association differs from this one.  Kept for fidelity. */
float *__cdecl RB_AccumulateWeightedPoint(float *point, float weight, float *matrix, float *outPos)
{
  outPos[0] = (matrix[4] * point[1] + matrix[8]  * point[2] + point[0] * matrix[0] + matrix[12]) * weight + outPos[0];
  outPos[1] = (matrix[5] * point[1] + matrix[9]  * point[2] + matrix[1] * point[0] + matrix[13]) * weight + outPos[1];
  outPos[2] = (matrix[6] * point[1] + matrix[10] * point[2] + matrix[2] * point[0] + matrix[14]) * weight + outPos[2];
  return matrix;
}

static const float colorWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

static const int s_boneBoxEdges[12][6] =
{
	{ 0,0,0,  1,0,0 },
	{ 0,0,0,  0,1,0 },
	{ 1,1,0,  1,0,0 },
	{ 1,1,0,  0,1,0 },
	{ 0,0,1,  1,0,1 },
	{ 0,0,1,  0,1,1 },
	{ 1,1,1,  1,0,1 },
	{ 1,1,1,  0,1,1 },
	{ 0,0,0,  0,0,1 },
	{ 1,0,0,  1,0,1 },
	{ 0,1,0,  0,1,1 },
	{ 1,1,0,  1,1,1 }
};

/* ---- R_XModelDebugBoxes  0x0050DD70 ----  VERIFIED
 * Draw the oriented bounding box of every DObj part whose bit is set in the
 * part mask, transformed by that part's bone matrix then the entity frame.
 * Built /Op-like (every pos / start / end component goes through its float
 * slot).  Each sum is parenthesised in retail's grouping: VC7 keeps explicit
 * float groupings but reassociates unparenthesised sums. */
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif
void __cdecl R_XModelDebugBoxes(trRefEntity_t *ent, const char *partMask)
{
  int          partCount;
  float      **partTable;
  float       *boneMat;
  float       *mat;
  const int  (*edge)[3];
  float        color[4];
  float        corner[3];
  float        pos[3];
  float        start[3];
  float        end[3];
  int          part;
  int          e;

  /* retail re-reads ent's DObj for the bone matrices after the table call */
  partCount = R_EntDObj(ent)->partCount;
  partTable = (float **)alloca(4 * partCount);
  DObjBuildPartCollisionTable(partTable, R_EntDObj(ent));
  boneMat = (float *)((R_EntDObj(ent)->childPartBaseIndices[0] << 6) + (int)R_EntDObj(ent)->evalStorage + 48);
  color[0] = 1.0f;
  color[1] = 1.0f;
  color[2] = 1.0f;
  color[3] = 0.0f;
  for ( part = 0; part < partCount; ++part, ++partTable )
  {
    if ( (partMask[part >> 3] & (1 << (part & 7))) != 0 )
    {
      const float (*box)[3] = (const float (*)[3])*partTable;

      mat = boneMat + 16 * part;
      edge = (const int (*)[3])s_boneBoxEdges;
      for ( e = 0; e < 12; ++e )
      {
        /* each edge is two corner selectors: 0 = mins, 1 = maxs per axis */
        corner[0] = box[(*edge)[0]][0];
        corner[1] = box[(*edge)[1]][1];
        corner[2] = box[(*edge)[2]][2];
        pos[0] = (corner[1] * mat[4] + corner[2] * mat[8]) + corner[0] * mat[0] + mat[12];
        pos[1] = (corner[1] * mat[5] + corner[2] * mat[9]) + corner[0] * mat[1] + mat[13];
        pos[2] = (corner[1] * mat[6] + corner[2] * mat[10]) + corner[0] * mat[2] + mat[14];
        start[0] = (pos[1] * ent->e.axis[1][0] + pos[2] * ent->e.axis[2][0]) + pos[0] * ent->e.axis[0][0];
        start[1] = (pos[1] * ent->e.axis[1][1] + pos[2] * ent->e.axis[2][1]) + pos[0] * ent->e.axis[0][1];
        start[2] = (pos[1] * ent->e.axis[1][2] + pos[2] * ent->e.axis[2][2]) + pos[0] * ent->e.axis[0][2];
        start[0] = start[0] + ent->e.origin[0];
        start[1] = start[1] + ent->e.origin[1];
        start[2] = start[2] + ent->e.origin[2];
        ++edge;
        corner[0] = box[(*edge)[0]][0];
        corner[1] = box[(*edge)[1]][1];
        corner[2] = box[(*edge)[2]][2];
        ++edge;
        pos[0] = (corner[2] * mat[8] + corner[0] * mat[0]) + corner[1] * mat[4] + mat[12];
        pos[1] = (corner[2] * mat[9] + corner[0] * mat[1]) + corner[1] * mat[5] + mat[13];
        pos[2] = (corner[2] * mat[10] + corner[0] * mat[2]) + corner[1] * mat[6] + mat[14];
        end[0] = (pos[2] * ent->e.axis[2][0] + pos[0] * ent->e.axis[0][0]) + pos[1] * ent->e.axis[1][0];
        end[1] = (pos[2] * ent->e.axis[2][1] + pos[0] * ent->e.axis[0][1]) + pos[1] * ent->e.axis[1][1];
        end[2] = (pos[2] * ent->e.axis[2][2] + pos[0] * ent->e.axis[0][2]) + pos[1] * ent->e.axis[1][2];
        end[0] = end[0] + ent->e.origin[0];
        end[1] = end[1] + ent->e.origin[1];
        end[2] = end[2] + ent->e.origin[2];

        CL_AddDebugLine(start, end, color, 0, 0, 0);
      }
    }
  }
}

/* ---- R_XModelDebugAxes  0x0050E040 ----  VERIFIED
 * Draw the X/Y/Z axes (red/green/blue, length 6) of every DObj part whose bit
 * is set in the part mask, in the part's bone frame then the entity frame.
 * Retail transforms the global vec3_origin through the bone matrix rather than
 * copying the translation, so a -0.0 / non-finite matrix gives retail's
 * result.  /Op-like with retail's sum groupings, as R_XModelDebugBoxes. */
void __cdecl R_XModelDebugAxes(trRefEntity_t *ent, const char *partMask)
{
  DObj  *dobj;
  int    partCount;
  float *boneMat;
  float *mat;
  float  axes[3][3];
  float  color[4];
  float  pos[3];
  float  start[3];
  float  end[3];
  int    part;
  int    axis;

  axes[0][0] = 6.0f;
  axes[0][1] = 0.0f;
  axes[0][2] = 0.0f;
  axes[1][0] = 0.0f;
  axes[1][1] = 6.0f;
  axes[1][2] = 0.0f;
  axes[2][0] = 0.0f;
  axes[2][1] = 0.0f;
  axes[2][2] = 6.0f;
  dobj = R_EntDObj(ent);
  partCount = dobj->partCount;
  boneMat = (float *)((dobj->childPartBaseIndices[0] << 6) + (int)dobj->evalStorage + 48);
  for ( part = 0; part < partCount; ++part )
  {
    if ( (partMask[part >> 3] & (1 << (part & 7))) != 0 )
    {
      mat = boneMat + 16 * part;
      for ( axis = 0; axis < 3; ++axis )
      {
        color[0] = color[1] = color[2] = 0.0f;
        color[3] = 0.0f;
        color[axis] = 1.0f;

        pos[0] = (vec3_origin[2] * mat[8]  + vec3_origin[1] * mat[4]) + vec3_origin[0] * mat[0] + mat[12];
        pos[1] = (vec3_origin[2] * mat[9]  + vec3_origin[1] * mat[5]) + vec3_origin[0] * mat[1] + mat[13];
        pos[2] = (vec3_origin[2] * mat[10] + vec3_origin[1] * mat[6]) + vec3_origin[0] * mat[2] + mat[14];
        start[0] = (pos[1] * ent->e.axis[1][0] + pos[2] * ent->e.axis[2][0]) + pos[0] * ent->e.axis[0][0];
        start[1] = (pos[1] * ent->e.axis[1][1] + pos[2] * ent->e.axis[2][1]) + pos[0] * ent->e.axis[0][1];
        start[2] = (pos[1] * ent->e.axis[1][2] + pos[2] * ent->e.axis[2][2]) + pos[0] * ent->e.axis[0][2];
        start[0] = start[0] + ent->e.origin[0];
        start[1] = start[1] + ent->e.origin[1];
        start[2] = start[2] + ent->e.origin[2];

        pos[0] = (mat[8] * axes[axis][2] + mat[4] * axes[axis][1]) + mat[0] * axes[axis][0] + mat[12];
        pos[1] = (mat[1] * axes[axis][0] + mat[5] * axes[axis][1]) + mat[9] * axes[axis][2] + mat[13];
        pos[2] = (mat[6] * axes[axis][1] + mat[2] * axes[axis][0]) + mat[10] * axes[axis][2] + mat[14];
        end[0] = (pos[1] * ent->e.axis[1][0] + pos[2] * ent->e.axis[2][0]) + pos[0] * ent->e.axis[0][0];
        end[1] = (pos[2] * ent->e.axis[2][1] + pos[0] * ent->e.axis[0][1]) + pos[1] * ent->e.axis[1][1];
        end[2] = (pos[2] * ent->e.axis[2][2] + pos[0] * ent->e.axis[0][2]) + pos[1] * ent->e.axis[1][2];
        end[0] = end[0] + ent->e.origin[0];
        end[1] = end[1] + ent->e.origin[1];
        end[2] = end[2] + ent->e.origin[2];

        CL_AddDebugLine(start, end, color, 0, 0, 0);
      }
    }
  }
}
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/* ---- R_XModelDebug  0x0050E310 ----  VERIFIED */
void __cdecl R_XModelDebug(trRefEntity_t *ent, const unsigned char *partMask)
{
  int drew = 0;

  if ( !_stricmp(r_xdebug->string, "boxes") || !_stricmp(r_xdebug->string, "both") )
  {
    drew = 1;
    R_XModelDebugBoxes(ent, (const char *)partMask);
  }
  if ( !_stricmp(r_xdebug->string, "axes") || !_stricmp(r_xdebug->string, "both") )
  {
    drew = 1;
    R_XModelDebugAxes(ent, (const char *)partMask);
  }
  if ( !drew )
  {
    Cvar_Set2("r_xdebug", "", qtrue);
    Com_Printf("boxes - show bounding boxes\n");
    Com_Printf("axes - show axes\n");
    Com_Printf("both - show bounding boxes and axes\n");
  }
}

/* ---- R_AddXModelSurfaces  0x0050E3E0 ----  VERIFIED
 * Add every visible surface of an animated XModel entity to the draw lists.
 * Selects a LOD per child model, expands the DObj into a flat surface list,
 * runs CalcPose skinning, then emits one entity draw surf per surface -- unless
 * a child uses the default collision model, in which case it draws debug boxes
 * / axes only.  Also feeds the r_showtricounts / r_showsurfcounts overlays and
 * queues entity lighting. */
void __cdecl R_AddXModelSurfaces(int entityPtr)
{
  trRefEntity_t  *ent = (trRefEntity_t *)entityPtr;
  DObj           *dobj;
  int             skipDrawSurfs;
  int             childCount;
  int            *lods;
  int             surfaceCount;
  DObjSurfaceRef *surfList;
  int             partBits[4];
  float           lodDist;
  int             triCount;
  int             vertCount;
  int             needsLighting;
  int             childIndex;
  int             model;
  shader_t       *shader;
  XSurface       *surf;
  int             i;

  if ( (ent->e.renderfx & 2) != 0 && !tr_viewParms_isPortal )
    skipDrawSurfs = 1;
  else
    skipDrawSurfs = 0;

  dobj = R_EntDObj(ent);
  if ( !r_drawXModels->integer || ent->cullState == CULL_OUT )
    return;

  childCount = dobj->childCount;
  lods = (int *)alloca(4 * childCount);
  lodDist = R_GetLodDist((float *)ent);
  for ( i = 0; i < childCount; ++i )
    lods[i] = XModelGetLodForDist(dobj->childRefs[i], lodDist);

  surfaceCount = DObjGetNumSurfaces(dobj, (int)lods);
  if ( surfaceCount + tr_refdef_entitySurfaceCount > 4096 )
  {
    if ( com_developer->integer )
      ri.Printf(0, "WARNING: MAX_ENTSURFS exceeded\n");
    return;
  }

  surfList = (DObjSurfaceRef *)alloca(4 * surfaceCount);
  DObjGetSurfaces(partBits, dobj, surfList, (int)lods);
  if ( R_EntPose(ent) )
  {
    DObjBackfillParentPartBits(dobj, (int)partBits);
    ri.CG_DObjCalcPose(R_EntPose(ent), dobj, partBits);
  }

  /* a child using the default ("bad") collision model draws debug geometry only */
  if ( DObjHasDefaultModel(dobj) )
  {
    if ( com_developer->integer )
    {
      R_XModelDebugBoxes(ent, (const char *)partBits);
      R_XModelDebugAxes(ent, (const char *)partBits);
    }
    return;
  }

  /* emit one entity draw surf per surface */
  needsLighting = 0;
  triCount = 0;
  vertCount = 0;
  for ( i = 0; i < surfaceCount; ++i )
  {
    childIndex = surfList[i].childIndex;
    model = R_DObjGetSurfIndex(dobj, childIndex);
    if ( model )
    {
      shader = (shader_t *)R_GetShaderByHandle(
                 ((unsigned short **)((model_t *)tr_models[model])->shaderHandles)[lods[childIndex]][surfList[i].surfaceIndex]);
      if ( (shader->lightingFlags & 0x18) != 0 )
        needsLighting = 1;
      if ( !skipDrawSurfs )
      {
        surf = dobj->childRefs[surfList[i].childIndex]->info->lodRecords[lods[surfList[i].childIndex]].surfs->surfs->surfaces[surfList[i].surfaceIndex];
        R_AddEntityDrawSurf(surf, shader, dobj, surfList[i].childIndex);
        triCount  += surf->triangleCount;
        vertCount += surf->vertexCount;
      }
    }
  }

  if ( r_showtricounts->integer && (ent->e.renderfx & 0x18) == 0 )
  {
    if ( r_showtricounts->integer == 2 )
      R_AddScaledDebugString(ent->e.origin, (unsigned int *)colorWhite, va("%i", vertCount));
    else
      R_AddScaledDebugString(ent->e.origin, (unsigned int *)colorWhite, va("%i", triCount));
  }
  else if ( r_showsurfcounts->integer && (ent->e.renderfx & 0x18) == 0 )
  {
    /* count surfaces whose shader handle no earlier surface already used */
    int uniqueCount = 1;

    for ( i = 1; i < surfaceCount; ++i )
    {
      int modelJ = surfList[i].childIndex;
      int handleJ = R_DObjGetSurfIndex(dobj, modelJ);

      if ( handleJ )
      {
        unsigned short **shJ = (unsigned short **)((model_t *)tr_models[handleJ])->shaderHandles;
        int              k;

        for ( k = 0; k < i; ++k )
        {
          int modelK = surfList[k].childIndex;
          int handleK = R_DObjGetSurfIndex(dobj, modelK);

          if ( handleK )
          {
            unsigned short **shK = (unsigned short **)((model_t *)tr_models[handleK])->shaderHandles;

            if ( shJ[lods[modelJ]][surfList[i].surfaceIndex] == shK[lods[modelK]][surfList[k].surfaceIndex] )
              break;
          }
        }
        if ( i == k )
          ++uniqueCount;
      }
    }
    R_AddScaledDebugString(ent->e.origin, (unsigned int *)colorWhite, va("%i/%i", uniqueCount, surfaceCount));
  }

  if ( *r_xdebug->string )
    R_XModelDebug(ent, (const unsigned char *)partBits);
  if ( (!skipDrawSurfs || cg_shadows->integer > 1) && needsLighting )
    R_SetupEntityLighting(ent, (int)&tr_refdef_x);
}

/* ---- RB_SetModelMatrixForRigidSurface  0x0050E830 ----  VERIFIED
 * Compose a rigid surface's bone matrix with backEnd.orientation.modelMatrix
 * and load it, so the fixed-function pipeline draws the surface's VBO in place. */
void __cdecl RB_SetModelMatrixForRigidSurface(xmodelDrawSurf_t *ds)
{
  XSurface      *surf = ds->surf;   /* the surf / dobj locals set VC7's term order in m[0..2] */
  DObj          *dobj = ds->dobj;
  const float   *bone;
  const GLfloat *mm;   /* backEnd.orientation.modelMatrix, 0x016D8DE4 */
  GLfloat        m[16];

  bone = (const float *)(surf->boneMatrixOffset
                         + (dobj->childPartBaseIndices[ds->childIndex] << 6)
                         + (int)dobj->evalStorage + 48);
  mm = backEnd_modelMatrix;
  m[0]  = mm[0] * bone[0]  + mm[8]  * bone[2]  + mm[4]  * bone[1];
  m[1]  = mm[1] * bone[0]  + mm[9]  * bone[2]  + mm[5]  * bone[1];
  m[2]  = mm[2] * bone[0]  + mm[10] * bone[2]  + mm[6]  * bone[1];
  m[3]  = 0.0f;
  m[4]  = mm[0] * bone[4]  + mm[4]  * bone[5]  + mm[8]  * bone[6];
  m[5]  = mm[1] * bone[4]  + mm[5]  * bone[5]  + mm[9]  * bone[6];
  m[6]  = mm[2] * bone[4]  + mm[6]  * bone[5]  + mm[10] * bone[6];
  m[7]  = 0.0f;
  m[8]  = mm[0] * bone[8]  + mm[4]  * bone[9]  + mm[8]  * bone[10];
  m[9]  = mm[1] * bone[8]  + mm[5]  * bone[9]  + mm[9]  * bone[10];
  m[10] = mm[2] * bone[8]  + mm[6]  * bone[9]  + mm[10] * bone[10];
  m[11] = 0.0f;
  m[12] = mm[0] * bone[12] + mm[4]  * bone[13] + mm[8]  * bone[14] + mm[12];
  m[13] = mm[1] * bone[12] + mm[5]  * bone[13] + mm[9]  * bone[14] + mm[13];
  m[14] = mm[2] * bone[12] + mm[6]  * bone[13] + mm[10] * bone[14] + mm[14];
  m[15] = 1.0f;
  glLoadMatrixf(m);
}

/* ---- RB_SurfaceXModelRigid  0x0050EA40 ----  VERIFIED
 * Tessellate a rigid (single-bone) XModel surface: copy tex coords, then
 * transform each vertex by the surface's bone matrix. */
void __cdecl RB_SurfaceXModelRigid(xmodelDrawSurf_t *ds)
{
  DObj              *dobj = ds->dobj;
  XSurface          *surf = ds->surf;
  int                vertexCount = surf->vertexCount;
  int                indexCount = 3 * surf->triangleCount;
  unsigned short     baseVertex;
  XSurfaceRigidVert *vert;
  float             *xyz;
  float             *nrm;
  const float       *matrix;
  int                remaining;

  RB_CheckOverflow(vertexCount, indexCount);
  baseVertex = tess_numVertexes;
  tess_numVertexes += vertexCount;
  XSurfaceGetTris((int)((char *)tess_indexes + 2 * tess_numIndexes), (int)surf, baseVertex);
  tess_numIndexes += indexCount;
  xyz = tess_xyz + 3 * baseVertex;
  nrm = tess_stageNormals + 3 * baseVertex;
  vert = (XSurfaceRigidVert *)surf->vertexData;
  Com_Memcpy((char *)tess_texCoords0 + 8 * baseVertex, surf->texCoords, 8 * vertexCount);
  matrix = (const float *)(surf->boneMatrixOffset
                           + (dobj->childPartBaseIndices[ds->childIndex] << 6)
                           + (int)dobj->evalStorage + 48);
  remaining = vertexCount;
  while ( remaining )
  {
    RB_TransformRigidVertex(vert->position, vert->normal, (float *)matrix, xyz, nrm);
    ++vert;
    xyz += 3;
    nrm += 3;
    --remaining;
  }
}

/* ---- RB_SurfaceXModelRigidSSE  0x0050EB40 ----  VERIFIED */
void __cdecl RB_SurfaceXModelRigidSSE(xmodelDrawSurf_t *ds)
{
  XSurface                *surf = ds->surf;
  DObj                    *dobj = ds->dobj;
  int                      vertexCount = surf->vertexCount;
  int                      indexCount = 3 * surf->triangleCount;
  unsigned short           baseVertex;
  const XSurfaceRigidVert *vert;
  const float             *m;
  float                   *xyz;
  float                   *nrm;
#ifndef COD1_VC7
  int                      remaining;
  int                      c;
#endif

  RB_CheckOverflow(vertexCount, indexCount);
  baseVertex = tess_numVertexes;
  tess_numVertexes += vertexCount;
  XSurfaceGetTris((int)((char *)tess_indexes + 2 * tess_numIndexes), (int)surf, baseVertex);
  tess_numIndexes += indexCount;
  xyz = tess_xyz + 3 * baseVertex;
  nrm = tess_stageNormals + 3 * baseVertex;
  vert = (const XSurfaceRigidVert *)surf->vertexData;
  Com_Memcpy((char *)tess_texCoords0 + 8 * baseVertex, surf->texCoords, 8 * vertexCount);
  m = (const float *)(surf->boneMatrixOffset
                      + (dobj->childPartBaseIndices[ds->childIndex] << 6)
                      + (int)dobj->evalStorage + 48);
#ifdef COD1_VC7
  /* retail's inline SSE, 0x50EBFC-0x50EC87 (the asm block gives it the ebp
   * frame); movups stores 16 bytes at a 12-byte stride, so lane 3 lands on
   * the next vertex's x (and past the last one) */
  __asm
  {
    mov       eax, m
    movaps    xmm4, [eax]
    movaps    xmm5, [eax+10h]
    movaps    xmm6, [eax+20h]
    movaps    xmm7, [eax+30h]
    mov       esi, xyz
    mov       edi, nrm
    mov       ecx, vertexCount
    mov       edx, vert
  rigidVertLoop:
    prefetchnta [edx+80h]
    movss     xmm0, [edx+0Ch]
    movss     xmm1, [edx]
    shufps    xmm0, xmm0, 0
    shufps    xmm1, xmm1, 0
    mulps     xmm0, xmm4
    mulps     xmm1, xmm4
    movss     xmm2, [edx+10h]
    movss     xmm3, [edx+4]
    shufps    xmm2, xmm2, 0
    shufps    xmm3, xmm3, 0
    mulps     xmm2, xmm5
    mulps     xmm3, xmm5
    addps     xmm0, xmm2
    addps     xmm1, xmm3
    movss     xmm2, [edx+14h]
    movss     xmm3, [edx+8]
    add       edx, 18h
    shufps    xmm2, xmm2, 0
    shufps    xmm3, xmm3, 0
    mulps     xmm2, xmm6
    mulps     xmm3, xmm6
    addps     xmm0, xmm2
    addps     xmm1, xmm3
    movups    [edi], xmm1
    add       edi, 0Ch
    addps     xmm0, xmm7
    movups    [esi], xmm0
    add       esi, 0Ch
    dec       ecx
    jne       rigidVertLoop
  }
#else
  remaining = vertexCount;
  do
  {
    /* Retail 0x50EC21-0x50EC80 is packed SSE: every mulps/addps rounds to
     * single precision, so each product and partial sum is cast / assigned
     * to float here (the Release build is x87 /arch:IA32 /fp:precise, where
     * a float*float product is exact and a float+float sum double-rounds
     * harmlessly at 53 bits).  The stores are 16-byte movups at a 12-byte
     * stride, so lane 3 (matrix column 3) spills into the next vertex's x;
     * the next vertex overwrites it, except after the surface's last one. */
    for ( c = 0; c < 4; ++c )
    {
      float t;
      float n;

      t = (float)(vert->normal[0] * m[c]) + (float)(vert->normal[1] * m[c + 4]);
      n = t + (float)(vert->normal[2] * m[c + 8]);
      if ( nrm + c == tess_stageNormals + 24576 )   /* one past the array: retail 0x0183FF60 = tess_stageBitangents[0] */
        tess_stageBitangents[0] = n;
      else
        nrm[c] = n;
      t = (float)(vert->position[0] * m[c]) + (float)(vert->position[1] * m[c + 4]);
      t = t + (float)(vert->position[2] * m[c + 8]);
      xyz[c] = t + m[c + 12];   /* tess_xyz is 32768 floats: lane 3 of vertex 8191 stays in bounds */
    }
    ++vert;
    xyz += 3;
    nrm += 3;
    --remaining;
  }
  while ( remaining );
#endif
}

/* ---- RB_SurfaceXModelRigidARB  0x0050EC90 ----  [HIGH]
 * Draw a rigid XModel surface from its ARB vertex buffer object. */
int __cdecl RB_SurfaceXModelRigidARB(xmodelDrawSurf_t *ds)
{
  XSurface      *surf;
  unsigned int  *arb;
  int            stageCount;
  int            i;
  int            result;
  int            texEnv[4];
  shaderStage_t *stage;
  GLsizei        count;

  if ( r_logFile->integer )
  {
    char *msg = va(
             "--- RB_SurfaceXModelRigidARB( %s ), model: %s ---\n",
             ((shader_t *)tess_shader)->name,
             XModelGetName(ds->dobj->childRefs[ds->childIndex]));
    if ( Stream )
      fprintf(Stream, "%s", msg);
  }
  RB_SetModelMatrixForRigidSurface(ds);
  surf = ds->surf;
  arb = (unsigned int *)surf->optimizedDataARB;
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCull(tess_shader));
  GL_ClientState(1537);
  qglBindBufferARB(34962, arb[0]);
  qglBindBufferARB(34963, arb[1]);
  glNormalPointer(0x1406u, 32, (const GLvoid *)8);
  glVertexPointer(3, 0x1406u, 32, (const GLvoid *)0x14);
  stageCount = R_ShaderNumStages(tess_shader);
  texEnv[0] = 0;
  i = 0;
  if ( stageCount > 0 )
  {
    do
    {
      stage = *(shaderStage_t **)(tess_activeStages + 4 * i);
      GL_State(stage->stateBits);
      RB_SetupMultitexture(stage, texEnv, 32);
      count = 3 * surf->triangleCount;
      backEnd_pc_drawnIndexCount += count;
      ++backEnd_pc_drawCallCount;
      glDrawElements(4u, count, 0x1403u, 0);
      ++i;
    }
    while ( i < R_ShaderNumStages(tess_shader) );
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x0050EDEA */
    stage = R_ShaderStages(tr_showTrisShader)[0];
    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 32);
    if ( r_showtris->integer >= 5 )
    {
      unsigned int color;   /* packed RGBA: reuses the dead ds slot as in retail */

      RB_ChooseSurfaceCountColor(3 * surf->triangleCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      qglColor3f(tr_identityLight, 0.0f, tr_identityLight);
    }
    count = 3 * surf->triangleCount;
    backEnd_pc_drawnIndexCount += count;
    ++backEnd_pc_drawCallCount;
    glDrawElements(4u, count, 0x1403u, 0);
    glDepthRange(0.0, 1.0);
  }
  qglBindBufferARB(34963, 0);
  qglBindBufferARB(34962, 0);
  glLoadMatrixf(backEnd_modelMatrix);   /* 0x016D8DE4 -- see the macro at the top */
  if ( Stream )
    fprintf(Stream, "%s", "----------\n");
  backEnd_pc_indexCount += surf->triangleCount + 2 * surf->triangleCount;
  result = surf->vertexCount;
  backEnd_pc_vertexCount += result;
  return result;
}

/* ---- RB_SurfaceXModelRigidATI  0x0050EF20 ----  [HIGH]
 * Draw a rigid XModel surface through the ATI vertex-array-object path. */
void __cdecl RB_SurfaceXModelRigidATI(xmodelDrawSurf_t *ds)
{
  XSurface      *surf;
  unsigned int  *ati;
  int            i;
  GLsizei        count;
  const GLvoid  *indices;

  if ( r_logFile->integer )
  {
    char *msg = va(
             "--- RB_SurfaceXModelRigidATI( %s ), model: %s ---\n",
             ((shader_t *)tess_shader)->name,
             XModelGetName(ds->dobj->childRefs[ds->childIndex]));
    if ( Stream )
      fprintf(Stream, "%s", msg);
  }
  RB_EndMultitexture();
  RB_SetModelMatrixForRigidSurface(ds);
  surf = ds->surf;
  ati = (unsigned int *)surf->optimizedDataATI;
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCull(tess_shader));
  GL_ClientState(1537);
  qglArrayObjectATI(32888, 2, 5126, 32, ati[1], ati[2]);
  qglArrayObjectATI(32885, 3, 5126, 32, ati[1], ati[2] + 8);
  qglArrayObjectATI(32884, 3, 5126, 32, ati[1], ati[2] + 20);
  if ( glConfig_ATIElementArray )
  {
    glEnableClientState(0x8768u);
    for ( i = 0; i < R_ShaderNumStages(tess_shader); ++i )
    {
      GL_State(R_ShaderStages(tess_shader)[i]->stateBits);
      GL_Bind((GLenum *)RB_GetAnimatedImage( glState_currentTmu, (int)&R_ShaderStages(tess_shader)[i]->bundle ));
      qglArrayObjectATI(34664, 1, 5123, 0, ati[1], ati[3]);
      count = 3 * surf->triangleCount;
      backEnd_pc_drawnIndexCount += count;
      ++backEnd_pc_drawCallCount;
      glDrawElementArrayATI(4, count);
    }
    if ( r_showtris->integer )
    {
      if ( (r_showtris->integer & 1) != 0 )
        glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x0050F0DB */
      GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
      GL_Bind((GLenum *)RB_GetAnimatedImage( glState_currentTmu, (int)&R_ShaderStages(tr_showTrisShader)[0]->bundle ));
      if ( r_showtris->integer >= 5 )
      {
        unsigned int color;   /* packed RGBA: reuses the dead ds slot as in retail */

        RB_ChooseSurfaceCountColor(3 * surf->triangleCount, &color);
        glColor4ubv((const GLubyte *)&color);
      }
      else
      {
        qglColor3f(tr_identityLight, 0.0f, tr_identityLight);
      }
      indices = (const GLvoid *)surf->triangles;
      count = 3 * surf->triangleCount;
      backEnd_pc_drawnIndexCount += count;
      ++backEnd_pc_drawCallCount;
      glDrawElements(4u, count, 0x1403u, indices);
      glDepthRange(0.0, 1.0);
    }
    glDisableClientState(0x8768u);
  }
  else
  {
    for ( i = 0; i < R_ShaderNumStages(tess_shader); ++i )
    {
      GL_State(R_ShaderStages(tess_shader)[i]->stateBits);
      GL_Bind((GLenum *)RB_GetAnimatedImage( glState_currentTmu, (int)&R_ShaderStages(tess_shader)[i]->bundle ));
      indices = (const GLvoid *)surf->triangles;
      count = 3 * surf->triangleCount;
      backEnd_pc_drawnIndexCount += count;
      ++backEnd_pc_drawCallCount;
      glDrawElements(4u, count, 0x1403u, indices);
    }
    if ( r_showtris->integer )
    {
      if ( (r_showtris->integer & 1) != 0 )
        glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x0050F25B */
      GL_State(R_ShaderStages(tr_showTrisShader)[0]->stateBits);
      GL_Bind((GLenum *)RB_GetAnimatedImage( glState_currentTmu, (int)&R_ShaderStages(tr_showTrisShader)[0]->bundle ));
      if ( r_showtris->integer >= 5 )
      {
        unsigned int color;   /* packed RGBA: reuses the dead ds slot as in retail */

        RB_ChooseSurfaceCountColor(3 * surf->triangleCount, &color);
        glColor4ubv((const GLubyte *)&color);
      }
      else
      {
        qglColor3f(tr_identityLight, 0.0f, tr_identityLight);
      }
      indices = (const GLvoid *)surf->triangles;
      count = 3 * surf->triangleCount;
      backEnd_pc_drawnIndexCount += count;
      ++backEnd_pc_drawCallCount;
      glDrawElements(4u, count, 0x1403u, indices);
      glDepthRange(0.0, 1.0);
    }
  }
  glLoadMatrixf(backEnd_modelMatrix);   /* 0x016D8DE4 -- see the macro at the top */
  if ( Stream )
    fprintf(Stream, "%s", "----------\n");
  backEnd_pc_indexCount += surf->triangleCount + 2 * surf->triangleCount;
  backEnd_pc_vertexCount += surf->vertexCount;
}

/* ---- RB_SurfaceXModelRigidNV  0x0050F380 ----  [HIGH]
 * Draw a rigid XModel surface through the NV vertex-array-range path. */
void __cdecl RB_SurfaceXModelRigidNV(xmodelDrawSurf_t *ds)
{
  XSurface      *surf;
  unsigned int  *nv;
  int            i;
  int            texEnv[4];
  shaderStage_t *stage;
  GLsizei        count;
  const GLvoid  *indices;

  if ( r_logFile->integer )
  {
    char *msg = va(
             "--- RB_SurfaceXModelRigidNV( %s ), model: %s ---\n",
             ((shader_t *)tess_shader)->name,
             XModelGetName(ds->dobj->childRefs[ds->childIndex]));
    if ( Stream )
      fprintf(Stream, "%s", msg);
  }
  RB_SetModelMatrixForRigidSurface(ds);
  surf = ds->surf;
  nv = (unsigned int *)surf->optimizedDataNV;
  RB_SetIteratorFog();
  GL_Cull(R_ShaderCull(tess_shader));
  GL_ClientState(1537);
  glNormalPointer(0x1406u, 32, (const GLvoid *)(nv[1] + 8));
  glVertexPointer(3, 0x1406u, 32, (const GLvoid *)(nv[1] + 20));
  texEnv[0] = nv[1];
  for ( i = 0; i < R_ShaderNumStages(tess_shader); ++i )
  {
    stage = *(shaderStage_t **)(tess_activeStages + 4 * i);
    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 32);
    indices = (const GLvoid *)surf->triangles;
    count = 3 * surf->triangleCount;
    backEnd_pc_drawnIndexCount += count;
    ++backEnd_pc_drawCallCount;
    glDrawElements(4u, count, 0x1403u, indices);
  }
  if ( r_showtris->integer )
  {
    if ( (r_showtris->integer & 1) != 0 )
      glDepthRange(0.0, 0.10000000149011612);   /* (double)0.1f = 0x3FB99999A0000000, retail 0x0050F4CD */
    stage = R_ShaderStages(tr_showTrisShader)[0];
    GL_State(stage->stateBits);
    RB_SetupMultitexture(stage, texEnv, 32);
    if ( r_showtris->integer >= 5 )
    {
      unsigned int color;   /* packed RGBA: reuses the dead ds slot as in retail */

      RB_ChooseSurfaceCountColor(3 * surf->triangleCount, &color);
      glColor4ubv((const GLubyte *)&color);
    }
    else
    {
      qglColor3f(tr_identityLight, 0.0f, tr_identityLight);
    }
    indices = (const GLvoid *)surf->triangles;
    count = 3 * surf->triangleCount;
    backEnd_pc_drawnIndexCount += count;
    ++backEnd_pc_drawCallCount;
    glDrawElements(4u, count, 0x1403u, indices);
    glDepthRange(0.0, 1.0);
  }
  glLoadMatrixf(backEnd_modelMatrix);   /* 0x016D8DE4 -- see the macro at the top */
  if ( Stream )
    fprintf(Stream, "%s", "----------\n");
  backEnd_pc_indexCount += surf->triangleCount + 2 * surf->triangleCount;
  backEnd_pc_vertexCount += surf->vertexCount;
}

/* ---- RB_SurfaceXModelWeight  0x0050F5E0 ----  VERIFIED
 * Tessellate a weighted (skinned) XModel surface: transform each vertex by its
 * primary bone, then add every additive bone influence.  The blend-vertex
 * stream is variable-stride (32 bytes without additive weights, 36 with). */
void __cdecl RB_SurfaceXModelWeight(xmodelDrawSurf_t *ds)
{
  DObj          *dobj = ds->dobj;
  XSurface      *surf = ds->surf;
  int            vertexCount = surf->vertexCount;
  int            indexCount = 3 * surf->triangleCount;
  unsigned short baseVertex;
  char          *vert;
  char          *wpt;
  float         *xyz;
  float         *nrm;
  int            mbase;
  int            remaining;

  RB_CheckOverflow(vertexCount, indexCount);
  baseVertex = tess_numVertexes;
  tess_numVertexes += vertexCount;
  XSurfaceGetTris((int)((char *)tess_indexes + 2 * tess_numIndexes), (int)surf, baseVertex);
  tess_numIndexes += indexCount;
  nrm = tess_stageNormals + 3 * baseVertex;
  xyz = tess_xyz + 3 * baseVertex;
  vert = (char *)surf->vertexData;
  Com_Memcpy((char *)tess_texCoords0 + 8 * baseVertex, surf->texCoords, 8 * vertexCount);
  mbase = (dobj->childPartBaseIndices[ds->childIndex] << 6) + (int)dobj->evalStorage + 48;
  wpt = (char *)surf->weightedPoints;
  if ( vertexCount )
  {
    remaining = vertexCount;
    do
    {
      XSurfaceBlendVert *bv = (XSurfaceBlendVert *)vert;

      RB_TransformRigidVertex(bv->blend.position, bv->normal, (float *)(mbase + bv->blend.boneMatrixOffset), xyz, nrm);
      if ( !bv->additiveWeightCount )
      {
        vert += 32;
      }
      else
      {
        int w = bv->additiveWeightCount;

        xyz[0] = bv->primaryWeight * xyz[0];
        xyz[1] = bv->primaryWeight * xyz[1];
        xyz[2] = bv->primaryWeight * xyz[2];
        do
        {
          XSurfaceWeightedPoint *wp = (XSurfaceWeightedPoint *)wpt;
          const float           *am = (const float *)(mbase + wp->blend.boneMatrixOffset);
          const float           *q  = wp->blend.position;
          float                  aw = wp->weight;

          /* Retail runs an INLINED copy here (0x50F6E0-0x50F74C) and never
           * calls the out-of-line RB_AccumulateWeightedPoint 0x50DD00 (zero
           * xrefs).  The inlined y/z sums associate differently from that
           * dead copy: (m1*p0 + m9*p2) + m5*p1, not (m5*p1 + m9*p2) + m1*p0.
           * x is the same in both. */
          xyz[0] = ((am[8] * q[2] + am[4] * q[1]) + q[0] * am[0] + am[12]) * aw + xyz[0];
          xyz[1] = ((am[1] * q[0] + am[9] * q[2]) + am[5] * q[1] + am[13]) * aw + xyz[1];
          xyz[2] = ((am[2] * q[0] + am[10] * q[2]) + am[6] * q[1] + am[14]) * aw + xyz[2];
          --w;
          wpt += 20;
        }
        while ( w );
        vert += 36;
      }
      xyz += 3;
      nrm += 3;
    }
    while ( --remaining );
  }
}

/* ---- RB_SurfaceXModelWeightSSE  0x0050F780 ----  VERIFIED */
void __cdecl RB_SurfaceXModelWeightSSE(xmodelDrawSurf_t *ds)
{
  XSurface      *surf = ds->surf;
  DObj          *dobj = ds->dobj;
  int            vertexCount = surf->vertexCount;
  int            indexCount = 3 * surf->triangleCount;
  unsigned short baseVertex;
  char          *vert;
  char          *wpt;
  int            mbase;
  float         *xyz;
  float         *nrm;
#ifndef COD1_VC7
  float          pos[4];   /* xmm0: lane 3 is stored too (see the movups below) */
  int            remaining;
  int            c;
#endif

  RB_CheckOverflow(vertexCount, indexCount);
  baseVertex = tess_numVertexes;
  tess_numVertexes += vertexCount;
  XSurfaceGetTris((int)((char *)tess_indexes + 2 * tess_numIndexes), (int)surf, baseVertex);
  tess_numIndexes += indexCount;
  xyz = tess_xyz + 3 * baseVertex;
  nrm = tess_stageNormals + 3 * baseVertex;
  vert = (char *)surf->vertexData;
  Com_Memcpy((char *)tess_texCoords0 + 8 * baseVertex, surf->texCoords, 8 * vertexCount);
  mbase = (dobj->childPartBaseIndices[ds->childIndex] << 6) + (int)dobj->evalStorage + 48;
  wpt = (char *)surf->weightedPoints;
#ifdef COD1_VC7
  /* retail's inline SSE, 0x50F83C-0x50F94B (the asm block gives it the ebp
   * frame); movups stores 16 bytes at a 12-byte stride, so lane 3 lands on
   * the next vertex's x (and past the last one) */
  __asm
  {
    mov       edi, vert
  weightVertLoop:
    prefetchnta [edi+80h]
    mov       eax, [edi+1Ch]
    mov       edx, mbase
    lea       esi, [edi+10h]
    add       edx, eax
    movss     xmm0, [esi]
    movaps    xmm6, [edx]
    movss     xmm3, [edi]
    shufps    xmm0, xmm0, 0
    shufps    xmm3, xmm3, 0
    mulps     xmm0, xmm6
    movss     xmm1, [esi+4]
    mulps     xmm3, xmm6
    movss     xmm4, [edi+4]
    movaps    xmm7, [edx+10h]
    shufps    xmm1, xmm1, 0
    shufps    xmm4, xmm4, 0
    mulps     xmm1, xmm7
    movss     xmm2, [esi+8]
    mulps     xmm4, xmm7
    movss     xmm5, [edi+8]
    addps     xmm0, xmm1
    addps     xmm3, xmm4
    movaps    xmm6, [edx+20h]
    shufps    xmm2, xmm2, 0
    movaps    xmm7, [edx+30h]
    shufps    xmm5, xmm5, 0
    mulps     xmm2, xmm6
    addps     xmm0, xmm7
    mulps     xmm5, xmm6
    addps     xmm0, xmm2
    addps     xmm3, xmm5
    mov       esi, nrm
    movups    [esi], xmm3
    add       esi, 0Ch
    mov       ecx, [edi+0Ch]
    mov       nrm, esi
    test      ecx, ecx
    je        weightNoBlend
    movss     xmm1, [edi+20h]
    mov       esi, wpt
    shufps    xmm1, xmm1, 0
    mulps     xmm0, xmm1
  weightBlendLoop:
    movss     xmm3, [esi]
    mov       eax, [esi+0Ch]
    mov       edx, mbase
    movss     xmm4, [esi+4]
    add       edx, eax
    movaps    xmm6, [edx]
    shufps    xmm3, xmm3, 0
    movaps    xmm7, [edx+10h]
    shufps    xmm4, xmm4, 0
    mulps     xmm3, xmm6
    movss     xmm5, [esi+8]
    movaps    xmm6, [edx+20h]
    mulps     xmm4, xmm7
    shufps    xmm5, xmm5, 0
    movaps    xmm7, [edx+30h]
    mulps     xmm5, xmm6
    addps     xmm3, xmm4
    movss     xmm1, [esi+10h]
    addps     xmm3, xmm5
    addps     xmm3, xmm7
    shufps    xmm1, xmm1, 0
    mulps     xmm3, xmm1
    dec       ecx
    lea       esi, [esi+14h]
    addps     xmm0, xmm3
    jne       weightBlendLoop
    mov       wpt, esi
    add       edi, 4
  weightNoBlend:
    mov       esi, xyz
    mov       eax, vertexCount
    add       edi, 20h
    movups    [esi], xmm0
    add       esi, 0Ch
    dec       eax
    mov       xyz, esi
    mov       vertexCount, eax
    jne       weightVertLoop
  }
#else
  remaining = vertexCount;
  do
  {
    XSurfaceBlendVert *bv = (XSurfaceBlendVert *)vert;
    const float       *m = (const float *)(mbase + bv->blend.boneMatrixOffset);
    const float       *p = bv->blend.position;
    const float       *q = bv->normal;
    int                count;

    /* Packed SSE in retail (0x50F851-0x50F8B2): every mulps/addps rounds to
     * single precision, so each product and partial sum is cast / assigned
     * to float here (x87 /arch:IA32 Release build).  All four lanes are
     * computed because retail stores normal and position with 16-byte movups
     * (0x50F8B8, 0x50F93E) at a 12-byte stride: lane 3 (matrix column 3)
     * spills into the next vertex's x and survives after the last vertex. */
    for ( c = 0; c < 4; ++c )
    {
      float t;
      float n;

      /* origin term is added BEFORE the z product -- 0x50F8A9 precedes 0x50F8AF */
      t      = (float)(p[0] * m[c]) + (float)(p[1] * m[c + 4]);
      t      = t + m[c + 12];
      pos[c] = t + (float)(p[2] * m[c + 8]);
      t      = (float)(q[0] * m[c]) + (float)(q[1] * m[c + 4]);
      n      = t + (float)(q[2] * m[c + 8]);
      if ( nrm + c == tess_stageNormals + 24576 )   /* one past the array: retail 0x0183FF60 = tess_stageBitangents[0] */
        tess_stageBitangents[0] = n;
      else
        nrm[c] = n;
    }
    count = bv->additiveWeightCount;
    if ( count )
    {
      float w = bv->primaryWeight;

      pos[0] = pos[0] * w;
      pos[1] = pos[1] * w;
      pos[2] = pos[2] * w;
      pos[3] = pos[3] * w;
      do
      {
        XSurfaceWeightedPoint *wp = (XSurfaceWeightedPoint *)wpt;
        const float           *am = (const float *)(mbase + wp->blend.boneMatrixOffset);
        float                  aw = wp->weight;

        q = wp->blend.position;
        for ( c = 0; c < 4; ++c )   /* 0x50F8D7: ((q0*am0 + q1*am4) + q2*am8 + am12) * w, then pos += */
        {
          float t;

          t      = (float)(q[0] * am[c]) + (float)(q[1] * am[c + 4]);
          t      = t + (float)(q[2] * am[c + 8]);
          t      = t + am[c + 12];
          t      = t * aw;
          pos[c] = t + pos[c];
        }
        wpt += 20;
        --count;
      }
      while ( count );
      vert += 4;
    }
    xyz[0] = pos[0];
    xyz[1] = pos[1];
    xyz[2] = pos[2];
    xyz[3] = pos[3];   /* movups lane 3: the next vertex's x (tess_xyz is 32768 floats, so in bounds) */
    vert += 32;
    xyz += 3;
    nrm += 3;
    --remaining;
  }
  while ( remaining );
#endif
}
