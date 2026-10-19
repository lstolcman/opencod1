/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_model.c -- model registry and XModel loading.
 *
 * CoD1 models are XModel-based, not RTCW's md3/mdc/mds meshes, so the loader
 * here (R_LoadXModel / R_FixupXModelTexCoords / R_RefreshXModels_ARB) walks an
 * XModel -> XModelInfo -> XModelLodInfo -> XModelSurfs -> XSurface chain that
 * RTCW's tr_models.c has no counterpart for.  Only the registry entry points
 * (R_GetModelByHandle, R_AllocModel, RE_RegisterModel, R_ModelInit,
 * RE_BeginRegistration, R_ModelBounds) line up with RTCW.
 *
 * model_t / bmodel_t come from tr_local.h.  The XModel chain and the tex-coord
 * remap record are file-local (proven against the retail binary + CoDUO and
 * pinned with size/offset asserts); tr_local.h is intentionally not touched.
 */

#include <malloc.h>
#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_shaderregistry.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"

extern int GetRefString_var;   /* script/scr_memorytree.cpp: the script string arena (scr_local.h) */

extern char xmodel_defaultCollision[];

extern int Hunk_AllocAlignInternal();
extern int Hunk_AllocXModelPrecache();
extern int Hunk_AllocXModelPrecacheMesh();
extern void __cdecl RB_ExecuteRenderCommands(const void *data);
extern int RE_ClearFlares();
extern int R_FindShader();
extern void R_FinishLoadingStaticModels(void);
extern void __cdecl R_Init( void );
extern int R_LoadDelayedImages();
extern int R_MergeShadersForImageSheets();
extern int R_SetupTextureCoordinateRemap();
extern void __cdecl R_SyncRenderThread( void );
extern int RE_ClearScene(void);
extern int R_OptimizeRigidXSurfaceARB();
extern int R_OptimizeRigidXSurfaceNV();
extern int R_OptimizeRigidXSurfaceATI();
typedef void *( *XModelAllocFn )( int size );
void *XModelPrecache( const char *name, int loadSurfaces, XModelAllocFn alloc, XModelAllocFn allocMesh );
extern int qsort_m();
extern int DObjGetNumSurfaces();
extern void DObjGetSurfaces();
extern int XSurfaceGetVerts();
extern int XSurfaceRefresh_ARB();

/* shader_t bits/fields used here that fall outside tr_local.h's named prefix.
 * (tr_local.h shader_t: flags +0x4c, remappedShader +0x180). */
#define SHADER_DEFAULTED         0x0001    /* flags: default (missing) shader */
#define SHADER_REMAPPED          0x1000    /* flags: uses a remapped shader */
#define R_ShaderRemapped(sh)     (((shader_t *)(sh))->remappedShader)

/* render command list, inside the backEndData frame (see tr_cmds.c) */
#define RCL_CMDS                 1636096   /* renderCommandList.cmds base */
#define RCL_USED                 1898240   /* renderCommandList.used */

/* ------------------------------------------------------------------------- *
 *  XModel data chain (file-local typed views).
 *
 *  XModel (0x0c) -> XModelInfo (0x68) -> XModelLodInfo[3] (0x14 each) ->
 *  XModelSurfs (0x0c) -> XModelSurfsData (0x0c) -> XSurface* [].
 *  Every offset below is proven against retail CoDMP.exe (IDA struct db) and
 *  cross-checked with CoDUO; sizes/offsets are pinned so a wrong one fails the
 *  build.  Only the fields the loader touches are named.
 * ------------------------------------------------------------------------- */
typedef struct {
	unsigned char  tileMode;          /* +0x00 */
	unsigned char  pad01;             /* +0x01 */
	short          vertexCount;       /* +0x02 */
	short          triangleCount;     /* +0x04 */
	unsigned short boneMatrixOffset;  /* +0x06  0xFFFF => not a rigid surface (unsigned: retail cmp word ptr [edi+6],0FFFFh) */
	unsigned int   boneUsage[4];      /* +0x08 */
	void          *weightedPoints;    /* +0x18 */
	void          *triangles;         /* +0x1c */
	void          *vertexData;        /* +0x20 */
	float        (*texCoords)[2];     /* +0x24 */
	void          *optimizedDataARB;  /* +0x28 */
	void          *optimizedDataATI;  /* +0x2c */
	void          *optimizedDataNV;   /* +0x30 */
} XSurface;
COD1_ASSERT_SIZE(XSurface, 0x34);
ASSERT_OFF(XSurface, vertexCount, 0x02);
ASSERT_OFF(XSurface, triangleCount, 0x04);
ASSERT_OFF(XSurface, boneMatrixOffset, 0x06);
ASSERT_OFF(XSurface, texCoords, 0x24);
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

typedef struct {
	const char  *name;                    /* +0x00 */
	XModelInfo  *info;                    /* +0x04 */
	void       (*freeData)(void *);       /* +0x08 */
} XModel;
COD1_ASSERT_SIZE(XModel, 0x0c);
ASSERT_OFF(XModel, info, 0x04);

/* one (shader,surface) tex-coord remap record used by R_FixupXModelTexCoords;
 * matches CoDUO xsurface_remap_t (sourceShader / sourceSurface / result). */
typedef struct {
	shader_t *sourceShader;               /* +0x00 */
	XSurface *sourceSurface;              /* +0x04 */
	XSurface *remappedSurface;            /* +0x08 */
} xsurfaceRemap_t;
COD1_ASSERT_SIZE(xsurfaceRemap_t, 0x0c);
ASSERT_OFF(xsurfaceRemap_t, sourceSurface, 0x04);
ASSERT_OFF(xsurfaceRemap_t, remappedSurface, 0x08);

/* ---- R_GetModelByHandle  0x005029B0 ----  VERIFIED */
int __cdecl R_GetModelByHandle(int handle)
{
  return tr_models[handle];
}

/* ---- RE_GetXModelByHandle  0x005029C0 ----  VERIFIED */
int __cdecl RE_GetXModelByHandle(int handle)
{
  return (int)((model_t *)tr_models[handle])->xmodel;
}

/* ---- R_GetXModelBounds  0x005029D0 ----  VERIFIED
 * Transformed AABB of an animated DObj.  If any child uses the default
 * ("bad") collision model the box is reported inverted/infinite; otherwise
 * every surface's verts are run through the 3x3 transform axis.  dobj is the DObj;
 * its fields and the child XModel -> surface lookup stay as raw offset
 * expressions because the per-model LOD index is read through retail-specific
 * stack aliasing.  Returns numSurfaces (dead EAX) or maxs on the bad path. */
void __cdecl R_GetXModelBounds(float *axis, int dobj, float *mins, float *maxs)
{
  int       childIdx;
  int       surfaceCount;
  short    *surfaceRefs;               /* DObjSurfaceRef {short child, short surface}; read movsx (0x00502AA0) */
  int       partBits[4];
  int       lodIndex;
  int       i;
  float     positionsOut[3 * 8192];    /* XSurfaceGetVerts working set, 8192 verts */

  /* scan children top-down for one whose model uses the default collision */
  for ( childIdx = *(unsigned char *)(dobj + 22) - 1; childIdx >= 0; --childIdx )   /* DObj.childCount - 1 */
  {
    if ( (void *)(*(XModel **)(dobj + 4 * childIdx + 24))->info == (void *)xmodel_defaultCollision )
    {
      /* found a bad child -> infinite/inverted bounds */
      mins[0] = -3.4028235e38f;
      mins[1] = -3.4028235e38f;
      mins[2] = -3.4028235e38f;
      maxs[0] = 3.4028235e38f;
      maxs[1] = 3.4028235e38f;
      maxs[2] = 3.4028235e38f;
      return;
    }
  }

  mins[0] = 3.4028235e38f;
  mins[1] = 3.4028235e38f;
  mins[2] = 3.4028235e38f;
  maxs[0] = -3.4028235e38f;
  maxs[1] = -3.4028235e38f;
  maxs[2] = -3.4028235e38f;

  lodIndex = 0;
  surfaceCount = DObjGetNumSurfaces(dobj, (int)&lodIndex);
  surfaceRefs = (short *)_alloca(4 * surfaceCount);
  DObjGetSurfaces(partBits, dobj, surfaceRefs, (int)&lodIndex);

  for ( i = 0; i < surfaceCount; ++i )
  {
    int       modelIndex = surfaceRefs[2 * i];
    int       childLod   = *(&lodIndex + modelIndex);                   /* per-model LOD (retail stack aliasing) */
    XModel   *child      = *(XModel **)(dobj + 4 * modelIndex + 24);      /* DObj.childRefs[modelIndex] */
    /* bone matrix = DObj.evalStorage + childPartBaseIndices[modelIndex]*64 + 0x30 */
    int       boneMat    = (*(unsigned char *)(dobj + modelIndex + 80) << 6) + *(int *)(dobj + 4) + 48;
    XSurface *surf       = child->info->lodRecords[childLod].surfs->surfs->surfaces[surfaceRefs[2 * i + 1]];
    int       vertexCount = surf->vertexCount;
    float    *vertPtr;
    int       n;

    XSurfaceGetVerts((int)surf, boneMat, positionsOut, 0, 0);

    vertPtr = &positionsOut[2];          /* -> z of vertex 0 */
    /* bounds tests written bound-first: VC7 then keeps t on the x87 stack (fcom), as retail */
    n = vertexCount;
    while ( n-- > 0 )
    {
      {
        float tx = axis[6] * vertPtr[0] + axis[3] * vertPtr[-1] + axis[0] * vertPtr[-2];
        if ( mins[0] > tx ) mins[0] = tx;
        if ( maxs[0] < tx ) maxs[0] = tx;
      }
      {
        float ty = axis[7] * vertPtr[0] + axis[4] * vertPtr[-1] + axis[1] * vertPtr[-2];
        if ( mins[1] > ty ) mins[1] = ty;
        if ( maxs[1] < ty ) maxs[1] = ty;
      }
      {
        float tz = axis[8] * vertPtr[0] + axis[5] * vertPtr[-1] + axis[2] * vertPtr[-2];
        if ( mins[2] > tz ) mins[2] = tz;
        if ( maxs[2] < tz ) maxs[2] = tz;
      }
      vertPtr += 3;
    }
  }
  return;
}

/* ---- R_Hunk_AllocXModel_m  0x00502BD0 ----  VERIFIED */
void __cdecl R_Hunk_AllocXModel_m(void *this)
{
  Hunk_AllocAlignInternal((unsigned int)this, 32);
}

/* ---- R_Hunk_AllocXModelMesh_m  0x00502BE0 ----  VERIFIED */
void __cdecl R_Hunk_AllocXModelMesh_m(void *this)
{
  Hunk_AllocAlignInternal((unsigned int)this, 32);
}

/* ---- R_LoadXModel  0x00502BF0 ----  VERIFIED
 * Precache the XModel, then build the per-LOD surface->shader-handle tables:
 * one uint16 handle per surface, looked up from "skins/<surfaceName>".  A
 * remapped shader is chased to its target; a defaulted shader stores 0. */
int __cdecl R_LoadXModel(model_t *model, const char *name, const char *shaderUsage)
{
  XModel          *xmodel;
  XModelInfo      *info;
  int              lodCount;
  int              totalSurfaceCount;
  unsigned short **shaderHandles;
  unsigned short  *surfaceHandle;
  int              lodIndex;
  char             shaderName[64];      /* 64: retail frame 0x60 */

  if ( !tr_modelsFinishedLoading )
  {
    tr_delayedImageGroup = ++tr_delayedImageGroupSequence;
  }
  else if ( r_errorOnConflicts->integer && !tr_ignorePrecacheErrorCount )
  {
    ri.Error(1, "\x15" "model '%s' not precached\n", name);
  }

  xmodel = (XModel *)XModelPrecache(
             name,
             2,
             (XModelAllocFn)Hunk_AllocXModelPrecache,
             (XModelAllocFn)Hunk_AllocXModelPrecacheMesh);
  info = xmodel->info;
  lodCount = info->lodCount;

  totalSurfaceCount = 0;
  for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex )
    totalSurfaceCount += info->lodRecords[lodIndex].surfs->surfs->surfaceCount;

  shaderHandles = (unsigned short **)Hunk_AllocAlignInternal(2 * (totalSurfaceCount + 2 * lodCount), 32);
  surfaceHandle = (unsigned short *)&shaderHandles[lodCount];

  for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex )
  {
    XModelSurfsData *surfs        = xmodel->info->lodRecords[lodIndex].surfs->surfs;
    int              surfaceCount = surfs->surfaceCount;
    XSurface       **surfaces     = surfs->surfaces;
    int              surfaceIndex;

    shaderHandles[lodIndex] = surfaceHandle;
    surfaceHandle += surfaceCount;
    for ( surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex )
    {
      unsigned short  nameIndex;
      const char     *surfaceName;
      char           *dst;
      char            c;
      shader_t       *shader;
      int             handle;

      strcpy(shaderName, "skins/");
      nameIndex = xmodel->info->lodRecords[lodIndex].surfaceNameTable[surfaceIndex];
      if ( nameIndex )
        surfaceName = (const char *)(GetRefString_var + 8 * nameIndex + 4);
      else
        surfaceName = "DEFAULT";
      dst = &shaderName[6];
      do                                    /* hand copy, not strcpy (retail 0x00502D40) */
      {
        c = *surfaceName++;
        *dst++ = c;
      }
      while ( c );

      if ( tr_delayedImageGroup )
      {
        tr_delayedImageGroupTriCount = surfaces[surfaceIndex]->triangleCount;
        tr_delayedImageGroupTileMode = surfaces[surfaceIndex]->tileMode;
      }

      shader = (shader_t *)R_FindShader(shaderName, -1, 1, shaderUsage);
      if ( shader->flags & SHADER_REMAPPED )
      {
        ri.Printf(2, "WARNING: model '%s' not precached, bad texturing will result on some surfaces\n", name);
        shader = R_ShaderRemapped(shader);
      }
      handle = !(shader->flags & SHADER_DEFAULTED) ? shader->index : 0;
      shaderHandles[lodIndex][surfaceIndex] = (unsigned short)handle;
    }
  }

  model->xmodel        = xmodel;
  model->type          = MOD_XMODEL;
  model->shaderHandles = shaderHandles;
  tr_delayedImageGroup         = 0;
  tr_delayedImageGroupTriCount = 0;
  return 0;
}

/* ---- compare_xsurface_remap  0x00502E20 ----  VERIFIED
 * qsort comparator: order by source surface address, then source shader index
 * (raw pointer difference / shader stride).  qsort only uses the sign. */
int __cdecl compare_xsurface_remap(xsurfaceRemap_t *remapA, xsurfaceRemap_t *remapB)
{
  int result;

  result = (int)remapA->sourceSurface - (int)remapB->sourceSurface;
  if ( !result )
    result = ((int)remapA->sourceShader - (int)remapB->sourceShader) / 408;
  return result;
}

int __cdecl XModelGetNumLods( const XModel *model );                                       /* xanim/xmodel.cpp */
int __cdecl XModelGetSurfaces( int lodIndex, const XModel *model, XSurface ***surfaces );  /* xanim/xmodel.cpp */

/* ---- R_FixupXModelTexCoords  0x00502E50 ----  [HIGH]
 * Build the set of unique (remappable-shader, surface) pairs across every
 * XModel; clone surfaces shared by more than one pair so their texture
 * coordinates can be remapped independently; then rewrite each model's surface
 * pointers to the (possibly cloned/remapped) surface and update its shader
 * handles.  Structure follows the Mac 1.3 build (XModel/XSurface helpers). */
XSurface *__cdecl XSurfaceCloneSurface( const XSurface *surface, XModelAllocFn alloc );  /* xanim/xmodel.cpp */
void __cdecl XSurfaceRemapTextureCoordinates( XSurface *surface, const float *scale, const float *offset,
                                              int sourceUIndex, int sourceVIndex );     /* xanim/xmodel.cpp */

void R_FixupXModelTexCoords()
{
  xsurfaceRemap_t remaps[65536];       /* on the stack, as retail (0xC0000-byte frame) */
  int        remapCount;
  int        modelIndex;
  model_t   *model;
  int        lodCount;
  int        lodIndex;
  int        surfaceCount;
  XSurface **surfaces;
  int        surfaceIndex;
  shader_t  *sourceShader;
  int        k;
  int        i;

  remapCount = 0;

  /* pass 1: collect unique (sourceShader, sourceSurface) pairs */
  for ( modelIndex = 1; modelIndex < tr_numModels; ++modelIndex )
  {
    model = (model_t *)tr_models[modelIndex];
    if ( !model->xmodel )
      continue;

    lodCount = XModelGetNumLods( (XModel *)model->xmodel );
    for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex )
    {
      surfaceCount = XModelGetSurfaces( lodIndex, (XModel *)model->xmodel, &surfaces );
      for ( surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex )
      {
        int handle = ((unsigned short **)model->shaderHandles)[lodIndex][surfaceIndex];

        if ( !handle )
          continue;

        sourceShader = (shader_t *)tr_shaders[handle];
        if ( !( sourceShader->flags & SHADER_REMAPPED ) )
          sourceShader = 0;

        if ( remapCount == 0x10000 )
          ri.Error(1, "\x15" "More than %i xmodel surfaces need to be remapped\n"
                      "You may need to set r_optimizeTextures temporarily to 0\n"
                      "Eventually you want to use fewer unique model surfaces\n", 0x10000);

        for ( k = 0; k < remapCount; ++k )
        {
          if ( remaps[k].sourceSurface == surfaces[surfaceIndex]
            && remaps[k].sourceShader == sourceShader )
            goto alreadyListed;   /* goto: retail has no post-loop index test */
        }

        remaps[remapCount].sourceShader    = sourceShader;
        remaps[remapCount].sourceSurface   = surfaces[surfaceIndex];
        remaps[remapCount].remappedSurface = 0;
        ++remapCount;
alreadyListed:
        ;
      }
    }
  }

  if ( !remapCount )
    return;

  qsort_m((unsigned int)remaps, remapCount, 0xCu,
          (int (__cdecl *)(unsigned int, byte *))compare_xsurface_remap);

  /* pass 2: clone shared surfaces and remap their texture coordinates */
  for ( i = 0; i < remapCount; ++i )
  {
    /* redundant i < remapCount: retail 0x00502FC8 tests it on loop entry */
    if ( i < remapCount && remaps[i + 1].sourceSurface == remaps[i].sourceSurface )
      remaps[i].remappedSurface = XSurfaceCloneSurface( remaps[i].sourceSurface, (XModelAllocFn)Hunk_AllocXModelPrecacheMesh );
    else
      remaps[i].remappedSurface = remaps[i].sourceSurface;

    if ( remaps[i].sourceShader )
    {
      float scale[2];
      float offset[2];
      int   sourceU;
      int   sourceV;

      R_SetupTextureCoordinateRemap(remaps[i].sourceShader, scale, offset, &sourceU, &sourceV);
      XSurfaceRemapTextureCoordinates( remaps[i].remappedSurface, scale, offset, sourceU, sourceV );
    }
  }

  /* pass 3: point each surface at its remapped copy and fix shader handles */
  for ( modelIndex = 1; modelIndex < tr_numModels; ++modelIndex )
  {
    model = (model_t *)tr_models[modelIndex];
    if ( !model->xmodel )
      continue;

    lodCount = XModelGetNumLods( (XModel *)model->xmodel );
    for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex )
    {
      surfaceCount = XModelGetSurfaces( lodIndex, (XModel *)model->xmodel, &surfaces );
      for ( surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex )
      {
        int handle = ((unsigned short **)model->shaderHandles)[lodIndex][surfaceIndex];

        if ( !handle )
          continue;

        sourceShader = (shader_t *)tr_shaders[handle];
        if ( !( sourceShader->flags & SHADER_REMAPPED ) )
          sourceShader = 0;
        else
          handle = R_ShaderRemapped(sourceShader)->index;
        ((unsigned short **)model->shaderHandles)[lodIndex][surfaceIndex] = (unsigned short)handle;

        for ( k = 0; k < remapCount; ++k )
        {
          if ( remaps[k].sourceShader == sourceShader
            && remaps[k].sourceSurface == surfaces[surfaceIndex] )
          {
            surfaces[surfaceIndex] = remaps[k].remappedSurface;
            break;
          }
        }
      }
    }
  }
}

/* ---- R_AllocModel  0x005033C0 ----  VERIFIED */
int R_AllocModel()
{
  int result;

  if ( tr_numModels == 2048 )
    return 0;
  result = ri.Hunk_Alloc(96);
  ((model_t *)result)->index = tr_numModels;
  tr_models[tr_numModels++] = result;
  return result;
}

/* ---- RE_RegisterModel  0x00503400 ----  VERIFIED
 * Look up a model by name, else allocate one and (for "xmodel/..." names)
 * load it.  Failed/non-xmodel names keep a MOD_BAD entry so the lookup is not
 * retried.  Returns the model handle, or 0 on failure. */
int __cdecl RE_RegisterModel(char *name, const char *shaderUsage)
{
  model_t *model;
  int      handle;
  char     firstChar;

  if ( !name || !*name )
    return 0;
  if ( strlen(name) >= 0x40 )
  {
    Com_Printf("Model name exceeds MAX_QPATH\n");
    return 0;
  }

  for ( handle = 1; handle < tr_numModels; ++handle )
  {
    model = (model_t *)tr_models[handle];
    if ( !Q_stricmp(model->name, name) )
    {
      if ( model->type == MOD_BAD )
        return 0;
      return handle;
    }
  }

  model = (model_t *)R_AllocModel();
  if ( !model )
  {
    ri.Printf(2, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
    return 0;
  }
  Q_strncpyz(model->name, name, 64);
  R_SyncRenderThread();
  model->numLods = 0;
  if ( !_strnicmp(name, "xmodel", 6u) )
  {
    firstChar = name[6];
    if ( firstChar == '/' || firstChar == '\\' )
    {
      R_LoadXModel(model, name + 7, shaderUsage);
      return model->index;
    }
  }
  model->type = MOD_BAD;
  return 0;
}

int __cdecl XSurfaceGetNumVerts( const XSurface *surface );                                /* xanim/xmodel.cpp */

/* ---- R_OptimizeXModels  0x005031E0 ----  [HIGH]
 * Not in the retail function list: RE_FinishLoadingModels tail-jumps here
 * (0x0050355E).  When r_optimize is on, optimize each XModel's rigid surfaces
 * into GPU vertex buffers.  A surface is optimized when it is "large" or its
 * shader handle is unique within the LOD. */
static void R_OptimizeXModels()
{
  int largeSurface[1024];
  int modelIndex;

  if ( !r_optimize->integer || r_optimizeXModels->integer <= 0 )
    return;

  for ( modelIndex = 1; modelIndex < tr_numModels; ++modelIndex )
  {
    model_t *model = (model_t *)tr_models[modelIndex];
    int      lodCount;
    int      lodIndex;

    if ( !model->xmodel )
      continue;

    lodCount = XModelGetNumLods( (XModel *)model->xmodel );
    for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex )
    {
      XSurface **surfaces;
      int        surfaceCount = XModelGetSurfaces( lodIndex, (XModel *)model->xmodel, &surfaces );
      int        surfaceIndex;

      for ( surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex )
      {
        largeSurface[surfaceIndex] = 0;
        if ( ((unsigned short **)model->shaderHandles)[lodIndex][surfaceIndex]
          && XSurfaceGetNumVerts( surfaces[surfaceIndex] ) >= r_optimizeXModels->integer )
          largeSurface[surfaceIndex] = 1;
      }

      for ( surfaceIndex = 0; surfaceIndex < surfaceCount; ++surfaceIndex )
      {
        int       handle = ((unsigned short **)model->shaderHandles)[lodIndex][surfaceIndex];
        XSurface *surf;

        if ( !handle )
          continue;

        if ( !largeSurface[surfaceIndex] )
        {
          int other;
          for ( other = 0; other < surfaceCount; ++other )
          {
            /* a duplicate handle keeps this surface unoptimized (goto: retail has
             * no post-loop index test) */
            if ( other != surfaceIndex && ((unsigned short **)model->shaderHandles)[lodIndex][other] == handle )
              goto nextSurface;
          }
        }

        surf = surfaces[surfaceIndex];
        if ( surf->boneMatrixOffset != 0xFFFF )   /* 0x0050330A: skinned surfaces (0xFFFF) are skipped */
        {
          if ( glConfig_ARBVertexBufferObject )
          {
            R_OptimizeRigidXSurfaceARB(surf, (int (__cdecl *)(int))Hunk_AllocXModelPrecacheMesh);
          }
          else if ( glConfig_NVVertexArrayRange )
          {
            R_OptimizeRigidXSurfaceNV(surf, (int (__cdecl *)(int))Hunk_AllocXModelPrecacheMesh);
          }
          else if ( glConfig_ATIVertexArrayObject )
          {
            /* 0x00503348: push offset Hunk_AllocXModelPrecacheMesh (surf in EDI) */
            R_OptimizeRigidXSurfaceATI(surf, (int (__cdecl *)(int))Hunk_AllocXModelPrecacheMesh);
          }
        }
nextSurface:
        ;
      }
    }
  }
  xmodel_animCheck = 1;
}

/* ---- RE_FinishLoadingModels  0x00503540 ----  VERIFIED
 * Finalize the registered models: load delayed images, merge sheet shaders,
 * finish static models, fix up tex coords, then optimize the XModels. */
void RE_FinishLoadingModels()
{
  R_LoadDelayedImages();
  tr_modelsFinishedLoading = 1;
  R_MergeShadersForImageSheets();
  R_FinishLoadingStaticModels();
  R_FixupXModelTexCoords();
  R_OptimizeXModels();
}

/* ---- RE_SetIgnorePrecacheErrors  0x00503570 ----  VERIFIED */
int __cdecl RE_SetIgnorePrecacheErrors(int ignore)
{
  if ( ignore )
    return ++tr_ignorePrecacheErrorCount;
  else
    return --tr_ignorePrecacheErrorCount;
}

/* ---- RE_GetIgnorePrecacheErrors  0x00503590 ----  VERIFIED */
int RE_GetIgnorePrecacheErrors()
{
  return tr_ignorePrecacheErrorCount != 0;
}

/* ---- RE_BeginRegistration  0x005035A0 ----  [HIGH]
 * Start a registration sequence: (re)init the renderer, flush any pending
 * render commands, reset the scene baselines, then queue a degenerate warm-up
 * command so the first real stretchPic is not dropped.  Copies the 0xA0-byte
 * glConfig out through glConfigOut.  Returns the command-list write offset / 1.0f bits (dead EAX). */
void __cdecl RE_BeginRegistration(glconfig_t *glConfigOut)
{
  int          frame;
  unsigned int used;
  unsigned int result;
  int         *cmd;
  char        *shader;

  R_Init();
  *glConfigOut=glConfig;    /* rep movsd, 0x005035B1 */
  R_SyncRenderThread();
  frame = backEndData;
  tr_viewCluster = -1;
  RE_ClearFlares();
  RE_ClearScene();
  tr_registered[0] = 1;
  used = *(int *)(frame + RCL_USED);
  result = used + 40;
  if ( result <= 0x3FFF8 )
  {
    cmd = (int *)(frame + (RCL_CMDS - 40) + result);
    *(int *)(frame + RCL_USED) = result;
    if ( cmd )
    {
      cmd[0] = 2;                    /* RC_STRETCH_PIC warm-up quad */
      if ( tr_numShaders[0] <= 0 )
      {
        ri.Printf(2, "R_GetShaderByHandle: out of range hShader '%d'\n", 0);
        shader = tr_defaultShader;
      }
      else
      {
        shader = (char *)tr_shaders[0];
      }
      cmd[1] = (int)shader;
      result = 1065353216;           /* 1.0f */
      cmd[2] = 0;
      cmd[3] = 0;
      cmd[4] = 0;
      cmd[5] = 0;
      cmd[6] = 0;
      cmd[7] = 0;
      cmd[8] = 1065353216;           /* 1.0f */
      cmd[9] = 1065353216;           /* 1.0f */
    }
  }
  return;
}

/* ---- R_ModelInit  0x005036B0 ----  VERIFIED */
int R_ModelInit()
{
  int result;

  tr_numModels = 0;
  result = ri.Hunk_Alloc(96);
  ((model_t *)result)->index = tr_numModels;
  tr_models[tr_numModels++] = result;
  ((model_t *)result)->type = MOD_BAD;
  return result;
}

/* ---- R_ModelBounds  0x005036F0 ----  VERIFIED
 * Bounding box of a brush model; non-brush models report an all-zero box.
 * model->bmodel is re-read per component, as retail. */
void __cdecl R_ModelBounds(int handle, float *mins, float *maxs)
{
  model_t *model = (model_t *)tr_models[handle];

  if ( model->bmodel )
  {
    mins[0] = model->bmodel->bounds[0][0];
    mins[1] = model->bmodel->bounds[0][1];
    mins[2] = model->bmodel->bounds[0][2];
    maxs[0] = model->bmodel->bounds[1][0];
    maxs[1] = model->bmodel->bounds[1][1];
    maxs[2] = model->bmodel->bounds[1][2];
    return;
  }
  mins[0] = mins[1] = mins[2] = 0;
  maxs[0] = maxs[1] = maxs[2] = 0;
}

/* ---- R_RefreshXModels_ARB  0x00503760 ----  [HIGH]
 * Refresh every XModel surface's ARB vertex buffer.  The model pointer is
 * re-read from tr_models for every LOD, as retail. */
void __cdecl R_RefreshXModels_ARB(int refreshFlags)
{
  int        modelIndex;
  int        lodIndex;
  int        lodCount;
  int        surfaceIndex;
  int        surfaceCount;
  XSurface **surfaces;

  for ( modelIndex = 0; modelIndex < tr_numModels; modelIndex++ )
  {
    if ( !((model_t *)tr_models[modelIndex])->xmodel )
      continue;
    lodCount = XModelGetNumLods( (XModel *)((model_t *)tr_models[modelIndex])->xmodel );
    for ( lodIndex = 0; lodIndex < lodCount; lodIndex++ )
    {
      surfaceCount = XModelGetSurfaces( lodIndex, (XModel *)((model_t *)tr_models[modelIndex])->xmodel, &surfaces );
      for ( surfaceIndex = 0; surfaceIndex < surfaceCount; surfaceIndex++ )
        XSurfaceRefresh_ARB( surfaces[surfaceIndex], refreshFlags );
    }
  }
}

/* ---- R_IncrementalRefreshXModels_ARB  0x00503820 ----  [HIGH]
 * Refresh at most one XModel surface per call, advancing the persistent
 * (model, LOD, surface) cursor.  The cursor is incremented before selection,
 * so surface 0 of each LOD is skipped -- retail behaviour, preserved.  The
 * cursor globals are updated in place (retail keeps them in registers). */
void __cdecl R_IncrementalRefreshXModels_ARB(int refreshFlags)
{
  int        startModel;
  int        surfaceCount;
  int        lodCount;
  XSurface **surfaces;

nextLod:
  if ( ((model_t *)tr_models[tr_xmodelRefreshModelIndex])->xmodel )
  {
    surfaceCount = XModelGetSurfaces( tr_xmodelRefreshLodIndex, (XModel *)((model_t *)tr_models[tr_xmodelRefreshModelIndex])->xmodel, &surfaces );
    if ( ++tr_xmodelRefreshSurfaceIndex < surfaceCount )
    {
      XSurfaceRefresh_ARB(surfaces[tr_xmodelRefreshSurfaceIndex], refreshFlags);
      return;
    }
    tr_xmodelRefreshSurfaceIndex = 0;
    lodCount = XModelGetNumLods( (XModel *)((model_t *)tr_models[tr_xmodelRefreshModelIndex])->xmodel );
    if ( ++tr_xmodelRefreshLodIndex < lodCount )
      goto nextLod;
  }

  tr_xmodelRefreshLodIndex = 0;
  startModel = tr_xmodelRefreshModelIndex;
  do
  {
    if ( ++tr_xmodelRefreshModelIndex >= tr_numModels )
      tr_xmodelRefreshModelIndex = 0;
    if ( tr_xmodelRefreshModelIndex == startModel )
      return;
  }
  while ( !((model_t *)tr_models[tr_xmodelRefreshModelIndex])->xmodel );
  goto nextLod;
}
