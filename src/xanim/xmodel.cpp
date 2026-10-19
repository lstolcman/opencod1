/*
 * Original translation unit:
 * /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/xanim/xmodel.cpp
 * Retail range 0x00488150-0x0048BDE0, 102 functions.
 * @fidelity: likely
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include <malloc.h>

#include "../qcommon/qcommon.h"
#include "../qcommon/vc7_compat.h"

extern int currentPos;   /* script/scr_tempmemory.cpp: the script temp-memory cursor */
extern char *scrSave_readPos;   /* script/scr_variable.cpp: the save-game read cursor */
#include "xanim_local.h"

/* retail .rdata error strings (a leading \x15 marks a localized-string reference); extern
   keeps C++ const data externally linked, as a C definition would be */
extern const char xmodel_errSyncLoopingAndSyncNonlooping[] = "\x15" "animation cannot be sync looping and sync nonlooping";   /* 0x0055A3E0 */
extern const char xmodel_errDuplicateSyncSpec[] = "\x15" "duplicate specification of animation sync in '%s', %d nodes above '%s'";   /* 0x0055A418 */
extern const char xmodel_errAnimSyncNonloopingAndLooping[] = "\x15" "animation '%s' in '%s' cannot be sync nonlooping and looping";   /* 0x0055A460 */
extern const char xmodel_errAnimSyncLoopingAndNonlooping[] = "\x15" "animation '%s' in '%s' cannot be sync looping and nonlooping";   /* 0x0055A4A0 */

extern void *FS_GetDataForFile( const char *dir, const char *name, const char *ext );
extern void *Hunk_ReallocateTempMemory( int size );
float FloatNoSwap( float f );   /* universal/q_shared.c; LTCG inlines it */
extern unsigned short SL_GetStringOfLen( const char *text, unsigned char user,
                                         unsigned int size, int type );
extern void SL_FreeString__FUsPCcUi( unsigned short handle, const char *text, unsigned int size );
extern void WriteString( unsigned short id );
extern void  __cdecl XAnimCalcRelDeltaParts( float *result, int parts, float weight,
                                 float time0, float time1 );
extern float * __cdecl XAnimCalcAbsDeltaParts( float *result, unsigned short *parts,
                                   float weight, float time );
extern int  __cdecl XAnimFreeInfo( unsigned short handle );
extern int  __cdecl XAnimGetNextNotifyTime( int entry, int node, float time );
struct XAnimInfo_s;
struct XModelSurfsData_s;
extern void __cdecl XAnimClearServerNotify( struct XAnimInfo_s *node );
extern void __cdecl XAnimDisplay( void *tree, int nodeIndex, int depth );
extern void __cdecl XAnimCalcTreeDelta_m( void *tree, int nodeIndex, float weight,
                                float *delta, int isFirst, int isChild );
extern void __cdecl XModelOptimize( struct XModelSurfsData_s *modelSurfs, void *(*alloc)( int size ) );

typedef struct XModelPartNameTable_s
{
    short           count;              /* +0x00 */
    unsigned short  handles[1];         /* +0x02, count entries */
} XModelPartNameTable;

typedef struct XModelPartNameTableSlot_s
{
    XModelPartNameTable *partNameTable; /* +0x00 */
    unsigned char   parentPartDeltas[4];/* +0x04, childCount entries */
} XModelPartNameTableSlot;

typedef struct XModelPartColl_s
{
    float           mins[3];
    float           maxs[3];
    float           center[3];
    float           radiusSq;
} XModelPartColl;

typedef struct XModelPartsData_s
{
    XModelPartNameTableSlot *partNameTableSlot; /* +0x00 */
    short           rootPartCount;              /* +0x04 */
    short           pad06;
    XModelPartColl *partCollisions;             /* +0x08 */
    short         (*baseRotations)[4];          /* +0x0C packed quat */
    float         (*baseTranslations)[3];       /* +0x10 */
    unsigned char  *partStateIndices;           /* +0x14 */
} XModelPartsData;

typedef struct XSimpleBlendInfo_s
{
    float           position[3];
    unsigned int    boneMatrixOffset;
} XSimpleBlendInfo;

typedef struct XSurfaceWeightedPoint_s
{
    XSimpleBlendInfo blend;
    float           weight;
} XSurfaceWeightedPoint;

typedef struct XSurfaceRigidVert_s
{
    float           normal[3];
    float           position[3];
} XSurfaceRigidVert;

typedef struct XSurfaceBlendVert_s
{
    float           normal[3];
    int             additiveWeightCount;
    XSimpleBlendInfo blend;
    float           primaryWeight;
} XSurfaceBlendVert;

typedef struct XSurface_s
{
    unsigned char   tileMode;           /* +0x00 */
    unsigned char   pad01;
    short           vertexCount;        /* +0x02 */
    short           triangleCount;      /* +0x04 */
    short           boneMatrixOffset;   /* +0x06  bone * 0x40, or -1 */
    unsigned int    boneUsage[4];       /* +0x08  128-bit bone-use mask */
    XSurfaceWeightedPoint *weightedPoints; /* +0x18 */
    unsigned short (*triangles)[3];     /* +0x1C */
    void           *vertexData;         /* +0x20 rigid verts or blend stream */
    float         (*texCoords)[2];      /* +0x24 */
    void           *optimizedDataARB;   /* +0x28 */
    void           *optimizedDataATI;   /* +0x2C */
    void           *optimizedDataNV;    /* +0x30 */
} XSurface;

typedef struct XStripInfo_s
{
    int             stripCount;
    const unsigned char  *stripVertexCounts;
    const unsigned short *stripIndices;
} XStripInfo;

typedef struct XModelSurfsData_s
{
    struct XModelSurfsData_s *next;     /* +0x00 clone list */
    short           surfaceCount;       /* +0x04 */
    short           pad06;
    XSurface      **surfaces;           /* +0x08 */
} XModelSurfsData;

typedef struct XModelSurfs_s
{
    const char     *name;
    XModelSurfsData *surfs;
    void          (*freeData)(struct XModelSurfs_s *);
} XModelSurfs;

typedef struct XModelPartsEntry_s
{
    const char     *name;
    XModelPartsData *data;
    void          (*freeData)(struct XModelPartsEntry_s *);
} XModelPartsEntry;

typedef struct XModelLodInfo_s
{
    float           distance;           /* +0x00 */
    const char     *name;               /* +0x04 */
    short           surfaceCount;       /* +0x08 */
    short           pad0a;
    unsigned short *surfaceNameTable;   /* +0x0C */
    XModelSurfs    *surfs;              /* +0x10 */
} XModelLodInfo;

typedef struct XModelCollTriPlane_s
{
    float           normal[3];
    float           distance;
} XModelCollTriPlane;

typedef struct XModelCollTri_s
{
    XModelCollTriPlane planes[3];
} XModelCollTri;

typedef struct XModelCollSurf_s
{
    XModelCollTri  *collTris;           /* +0x00 */
    int             numCollTris;        /* +0x04 */
    float           expandedMins[3];    /* +0x08 */
    float           expandedMaxs[3];    /* +0x14 */
    int             basePoseIndex;      /* +0x20 */
    int             contents;           /* +0x24 */
    int             surfaceFlags;       /* +0x28 */
} XModelCollSurf;

typedef struct XModelInfo_s
{
    XModelPartsEntry *parts;            /* +0x00 */
    XModelLodInfo   lodRecords[3];      /* +0x04 */
    XModelCollSurf *collisionSurfaces;  /* +0x40 */
    int             collisionSurfaceCount; /* +0x44 */
    int             contents;           /* +0x48 */
    float           mins[3];            /* +0x4C */
    float           maxs[3];            /* +0x58 */
    short           lodCount;           /* +0x64 */
    short           modelFileCount;     /* +0x66 */
} XModelInfo;

typedef struct XModel_s
{
    const char     *name;
    XModelInfo     *info;
    void          (*freeData)(struct XModel_s *);
} XModel;

typedef struct XModelConfigLod_s
{
    char            name[1024];
    float           distance;
} XModelConfigLod;

typedef struct XModelConfig_s
{
    XModelConfigLod lods[3];
    float           mins[3];
    float           maxs[3];
    int             modelFileCount;
} XModelConfig;

typedef struct DObjSkelMat_s
{
    float           axis[3][4];
    float           origin[4];
} DObjSkelMat;

typedef struct XAnimState_s {
    float          time;                     /* +0x00 */
    float          oldTime;                  /* +0x04 */
    unsigned short cycleCount;               /* +0x08 */
    unsigned short oldCycleCount;            /* +0x0A */
    float          weightBlendTimeRemaining; /* +0x0C */
    float          targetWeight;             /* +0x10 */
    float          currentWeight;            /* +0x14 */
    float          rateScale;                /* +0x18 */
} XAnimState;

typedef struct XAnimInfo_s {
    unsigned short notifyChildIndex;    /* +0x00 */
    short          notifyIndex;         /* +0x02  -1 == no pending notify */
    unsigned short notifyName;          /* +0x04 */
    unsigned short notifyType;          /* +0x06 */
    unsigned short freePrev;            /* +0x08 */
    unsigned short freeNext;            /* +0x0A  node 0's is the free head */
    XAnimState     states[2];           /* +0x0C and +0x28 */
} XAnimInfo;

typedef struct XAnimEntry_s {
    unsigned short childCount;          /* +0x00  0 == leaf */
    unsigned short parentIndex;         /* +0x02 */
    union {
        void *leafAsset;                /* +0x04 */
        struct {
            unsigned short flags;           /* +0x04 */
            unsigned short firstChildIndex; /* +0x06 */
        } parent;
    } u;
} XAnimEntry;

typedef struct XAnim_s {
    const char *name;                   /* +0x00 */
    int         nodeCount;              /* +0x04 */
    XAnimEntry  entries[1];             /* +0x08 */
} XAnim;

typedef struct XAnimTree_s {
    XAnim         *sourceTree;             /* +0x00 */
    int            partRemapTableSelector; /* +0x04 */
    unsigned short poolNodeHandles[1];     /* +0x08  [nodeCount] */
} XAnimTree;

typedef struct XAnimParts_min_s {
    unsigned short  pad00[6];         /* +0x00 .. +0x0B */
    unsigned short *partNameHandles;  /* +0x0C  [0] is the part count */
} XAnimParts_min;

extern XAnimInfo xanim_pool[2048];

/* The lane xanim_activePoolSlot (0x00A9CC58) selects: client 0, server 1. */
#define XANIM_LANE(h)   ( xanim_pool[h].states[xanim_activePoolSlot] )

extern const char *SL_ConvertToString( unsigned short handle );
extern void        SL_RemoveRefToStringOfLen( unsigned short handle,
                                              unsigned int size );
extern void        SL_RemoveRefToString( unsigned short handle );
extern void        SL_AddRefToString( unsigned short handle );

extern int __cdecl XAnimSetAnimInternal_m( int nodeIndex, float weight, float blendTime,
                               float rate, char force,
                               unsigned __int16 notifyName,
                               __int16 notifyType );

extern void __cdecl XAnimUpdateNotifyIndex_m( int node );
extern void __cdecl XAnimReseedTiming_m( int nodeIndex, int forceClear );
extern void __cdecl cand_XAnimSeedParentChain( int node, float blendTime );
extern void __cdecl XAnimSetAnimKnobInternal_m( XAnimTree *tree, int node,
                                   float weight, float blendTime, float rate,
                                   unsigned __int16 notifyName,
                                   __int16 notifyType, int animFlags );

typedef void *(*XModelAllocFn)(int size);

#define XMODEL_ASSET_VERSION    14
#define XMODEL_LOD_COUNT        3
#define XMODEL_MAX_BONES        127
#define XMODEL_NO_SINGLE_BONE   (-1)

#define XMODEL_BONE_MATRIX_SHIFT    6

/* the default-model block, 0x00A9CC88-0x00A9CE10 */

static XModelPartNameTable      xmodel_defaultPartNameTable;    /* 0x00A9CC88 */
static XModelPartNameTableSlot  xmodel_defaultPartNameTableSlot;/* 0x00A9CC8C */
static XSurface                 xmodel_defaultSurfaces[1];      /* 0x00A9CC94 */
static XSurface                *xmodel_defaultSurfacePtrs[1];   /* 0x00A9CCC8 */
XModelInfo                      xmodel_defaultCollision;        /* 0x00A9CCCC */
static XModelPartsData          xmodel_defaultParts;            /* 0x00A9CD34 */
static XModelSurfsData          xmodel_defaultSurfs;            /* 0x00A9CD4C */
static XModelPartColl           xmodel_defaultPartColl;         /* 0x00A9CD58 */
static XModel                   xmodel_defaultModelEntry;       /* 0x00A9CD80 */
static XModelPartsEntry         xmodel_defaultPartsEntry;       /* 0x00A9CD8C */
static XModelSurfs              xmodel_defaultSurfsEntry;       /* 0x00A9CD98 */
static XSurfaceRigidVert        xmodel_defaultRigidVerts[3];    /* 0x00A9CDA4 */
static float                    xmodel_defaultTexCoords[3][2];  /* 0x00A9CDEC */
static unsigned short           xmodel_defaultTriangle[3];      /* 0x00A9CE06 */
static unsigned char            xmodel_defaultPartStateIndices[1]; /* 0x00A9CE0C */
static unsigned short           xmodel_defaultSurfaceNames[1];  /* 0x00A9CE0E */

float                           xmodel_testLodDist;             /* 0x00A9CE10 */
unsigned char                   xmodel_testLodsEnabled;         /* 0x00A9CE14 */
int                             xmodel_animCheck;               /* 0x01407330 */
int                             xmodel_enforceExist;            /* 0x00572D68, the cl_xmodelcheck latch */
XModelLodInfo                   xmodel_testLodDistances[XMODEL_LOD_COUNT]; /* 0x00A9CE18 */

static XModelSurfsData         *xmodel_surfsCloneList;

#define xmodel_enforceExist     xmodel_enforceExist
#define xmodel_animCheck        xmodel_animCheck

#define SL_REFSTRING(h)         ( (char *)GetRefString_var + 8 * (unsigned short)(h) )

/* ---- XModelReleaseString  no-address ---- */
static void XModelReleaseString(unsigned short handle)
{
    char *rec = SL_REFSTRING(handle);

    if ( *(unsigned short *)rec )
        --*(unsigned short *)rec;
    else
        SL_FreeString__FUsPCcUi(handle, rec + 4, strlen(rec + 4) + 1);
}

/* ---- DObjDisplayAnim  0x00488150 ----  [CONFIRMED] */
void __cdecl DObjDisplayAnim(int **treeSlot)
{
  int *tree = *treeSlot;

  if ( !tree )
  {
    Com_Printf("NO TREE\n");
    return;
  }
  XAnimDisplay(tree, 0, 0);
  Com_Printf("\n");
}

/*
 * delta[6] as filled by XAnimCalcTreeDelta_m / XAnimCalc*DeltaParts:
 *   delta[0..1]  2D facing delta (cos, sin); reset to (0,1) when degenerate
 *   delta[2]     unused
 *   delta[3..5]  translation delta (copied out verbatim as raw bits)
 * The `x == 0.0f` tests below are plain C compares (false on NaN), exactly
 * retail's `test ah,44h / jnp` x87 sequences.
 */

/* ---- XAnimGetRelDelta_m  0x00488180 ----  VERIFIED */
int __cdecl XAnimGetRelDelta_m( void *tree, int node, float *dirDelta,
                                int *posDelta, int deltaRootNode )
{
  float delta[6];

  xanim_evalPoolWeightSelector = deltaRootNode;
  xanim_evalLeafOutputMode = 0;
  XAnimCalcTreeDelta_m( tree, node, 1.0f, delta, 1, 0 );
  if ( delta[0] == 0.0f
    || delta[1] == 0.0f )
  {
    dirDelta[0] = 0.0f;
    dirDelta[1] = 1.0f;
  }
  else
  {
    dirDelta[0] = delta[0];
    dirDelta[1] = delta[1];
  }
  posDelta[0] = ((int *)delta)[3];
  posDelta[1] = ((int *)delta)[4];
  posDelta[2] = ((int *)delta)[5];
  return ((int *)delta)[3];
}

/* ---- XAnimGetAbsDelta_m  0x00488210 ----  VERIFIED */
int __cdecl XAnimGetAbsDelta_m( void *tree, int node, float *dirDelta, int *posDelta )
{
  float delta[6];

  xanim_evalLeafOutputMode = 1;
  xanim_evalPoolWeightSelector = 1;
  XAnimCalcTreeDelta_m( tree, node, 1.0f, delta, 1, 0 );
  if ( delta[0] == 0.0f
    && delta[1] == 0.0f )
  {
    dirDelta[0] = 0.0f;
    dirDelta[1] = 1.0f;
  }
  else
  {
    dirDelta[0] = delta[0];
    dirDelta[1] = delta[1];
  }
  posDelta[0] = ((int *)delta)[3];
  posDelta[1] = ((int *)delta)[4];
  posDelta[2] = ((int *)delta)[5];
  return ((int *)delta)[4];
}

/*
 * The ForTime variants take the *source* tree (XAnim) directly and evaluate one
 * leaf node's delta at explicit sample times.  A non-leaf node, or a leaf whose
 * anim-parts "has delta" flag (parts[+3]) is clear, yields the degenerate delta.
 */

/* ---- XAnimGetRelDeltaForTime_m  0x00488290 ----  VERIFIED */
void __cdecl XAnimGetRelDeltaForTime_m( int sourceTree, int node, float *posDelta,
                                        float *dirDelta, float time0, float time1 )
{
  const XAnim *source = (const XAnim *)sourceTree;
  const char  *parts;
  float        delta[6];

  if ( !source->entries[node].childCount )
  {
    parts = ((const char **)source->entries[node].u.leafAsset)[1];
    if ( parts[3] )   /* XAnimParts::hasDeltaMotion */
    {
      delta[0] = 0.0f;
      delta[1] = 0.0f;
      delta[2] = 0.0f;
      delta[3] = 0.0f;
      delta[4] = 0.0f;
      delta[5] = 0.0f;
      XAnimCalcRelDeltaParts( delta, (int)parts, 1.0f, time0, time1 );
      if ( delta[0] == 0.0f && delta[1] == 0.0f )
      {
        dirDelta[0] = 0.0f;
        dirDelta[1] = 1.0f;
      }
      else
      {
        dirDelta[0] = delta[0];
        dirDelta[1] = delta[1];
      }
      posDelta[0] = delta[3];
      posDelta[1] = delta[4];
      posDelta[2] = delta[5];
      return;
    }
  }

  dirDelta[0] = 0.0f;
  dirDelta[1] = 1.0f;
  posDelta[0] = 0.0f;
  posDelta[1] = 0.0f;
  posDelta[2] = 0.0f;
}

/* ---- XAnimGetAbsDeltaForTime_m  0x00488360 ----  VERIFIED */
void __cdecl XAnimGetAbsDeltaForTime_m( const XAnim *sourceTree, int node, float *posDelta, float *dirDelta, float time )
{
  const char *parts;
  float       delta[6];

  if ( !sourceTree->entries[node].childCount )
  {
    parts = ((const char **)sourceTree->entries[node].u.leafAsset)[1];
    if ( parts[3] )   /* XAnimParts::hasDeltaMotion */
    {
      delta[0] = 0.0f;
      delta[1] = 0.0f;
      delta[2] = 0.0f;
      delta[3] = 0.0f;
      delta[4] = 0.0f;
      delta[5] = 0.0f;
      XAnimCalcAbsDeltaParts( delta, (unsigned short *)parts, 1.0f, time );
      if ( delta[0] == 0.0f && delta[1] == 0.0f )
      {
        dirDelta[0] = 0.0f;
        dirDelta[1] = 1.0f;
      }
      else
      {
        dirDelta[0] = delta[0];
        dirDelta[1] = delta[1];
      }
      posDelta[0] = delta[3];
      posDelta[1] = delta[4];
      posDelta[2] = delta[5];
      return;
    }
  }

  dirDelta[0] = 0.0f;
  dirDelta[1] = 1.0f;
  posDelta[0] = 0.0f;
  posDelta[1] = 0.0f;
  posDelta[2] = 0.0f;
}

/* ---- XAnimAllocInfo  0x00488450 ----  VERIFIED */
XAnimInfo *__cdecl XAnimAllocInfo( XAnimTree *tree, int nodeIndex )
{
    unsigned short handle;

    handle = xanim_pool[0].freeNext;
    if ( !xanim_pool[0].freeNext ) {
        Com_Error( ERR_DROP, "\x15" "exceeded maximum number of anim info" );
        return NULL;
    }

    ++xanim_poolUsedCount;
    if ( xanim_poolUsedCount > xanim_poolHighWaterCount ) {
        xanim_poolHighWaterCount = xanim_poolUsedCount;
    }

    xanim_pool[0].freeNext = xanim_pool[xanim_pool[0].freeNext].freeNext;
    xanim_pool[xanim_pool[0].freeNext].freePrev = 0;
    tree->poolNodeHandles[nodeIndex] = handle;
    return &xanim_pool[handle];
}

/* ---- XAnimParentChainHasWeight_m  0x004884D0 ----  VERIFIED */
char __cdecl XAnimParentChainHasWeight_m( int nodeIndex, XAnimTree *tree )
{
    while ( nodeIndex ) {
        nodeIndex = tree->sourceTree->entries[nodeIndex].parentIndex;

        if ( XANIM_LANE( tree->poolNodeHandles[nodeIndex] ).currentWeight == 0.0f ) {
            return 0;
        }
    }
    return 1;
}

/* ---- XAnimSubtreeHasWeight_m  0x00488510 ----  VERIFIED */
char __cdecl XAnimSubtreeHasWeight_m( XAnimTree *tree, int nodeIndex )
{
    unsigned short  handle;
    XAnimEntry     *entry;
    int             childCount;
    int             i;

    handle = tree->poolNodeHandles[nodeIndex];
    if ( handle == 0 ) {
        return 0;
    }
    if ( XANIM_LANE( handle ).currentWeight == 0.0f ) {
        return 0;
    }

    entry      = &tree->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;
    if ( childCount == 0 ) {
        return 1;
    }

    for ( i = 0; i < childCount; ++i ) {
        if ( XAnimSubtreeHasWeight_m( tree, entry->u.parent.firstChildIndex + i ) ) {
            return 1;
        }
    }
    return 0;
}

/* ---- XAnimNodeIsActive_m  0x00488590 ----  VERIFIED */
int __cdecl XAnimNodeIsActive_m( unsigned int *tree, int node )
{
  return XAnimParentChainHasWeight_m( node, (XAnimTree *)tree )
      && XAnimSubtreeHasWeight_m( (XAnimTree *)tree, node );
}

/* ---- XAnimClearAnimNode_m  0x004885C0 ----  VERIFIED */
void __cdecl XAnimClearAnimNode_m( unsigned int *treePtr, int node, float blendTime )
{
  XAnimTree     *tree   = (XAnimTree *)treePtr;
  unsigned short handle = tree->poolNodeHandles[node];

  if ( handle )
  {
    XAnimInfo  *info = &xanim_pool[handle];
    XAnimState *lane = &info->states[xanim_activePoolSlot];

    if ( !XAnimNodeIsActive_m( treePtr, node ) )
    {
      lane->currentWeight = 0.0f;
      lane->weightBlendTimeRemaining = 0.0f;
      lane->targetWeight = 0.0f;
    }
    else if ( lane->targetWeight != 0.0f )
    {
      lane->targetWeight = 0.0f;
      lane->weightBlendTimeRemaining = blendTime;
    }
    else if ( lane->weightBlendTimeRemaining > blendTime )
    {
      lane->weightBlendTimeRemaining = blendTime;
    }
    if ( xanim_activePoolSlot )
      XAnimClearServerNotify( info );
  }
}

/* ---- XAnimClearAnimSubtree_m  0x00488660 ----  VERIFIED */
void __cdecl XAnimClearAnimSubtree_m( unsigned int *treePtr, int node, float blendTime )
{
  XAnimTree *tree = (XAnimTree *)treePtr;

  if ( tree->poolNodeHandles[node] )
  {
    XAnimEntry *entry;
    int         childCount;
    int         i;

    XAnimClearAnimNode_m( treePtr, node, blendTime );

    entry      = &tree->sourceTree->entries[node];
    childCount = entry->childCount;
    for ( i = 0; i < childCount; ++i )
      XAnimClearAnimSubtree_m( treePtr, entry->u.parent.firstChildIndex + i, blendTime );
  }
}

/* ---- XAnimClearAnim_m  0x004886C0 ----  VERIFIED */
void __cdecl XAnimClearAnim_m( int node, unsigned int *treePtr, float blendTime )
{
  if ( blendTime < 0.001f )
    blendTime = 0.0f;
  XAnimClearAnimSubtree_m( treePtr, node, blendTime );
}

/* ---- XAnimClearAnimChildSubtrees_m  0x004886F0 ----  VERIFIED */
void __cdecl XAnimClearAnimChildSubtrees_m( unsigned int *treePtr, int node, float blendTime )
{
  XAnimTree  *tree = (XAnimTree *)treePtr;
  XAnimEntry *entry;
  int         childCount;
  int         i;

  if ( blendTime < 0.001f )
    blendTime = 0.0f;

  entry      = &tree->sourceTree->entries[node];
  childCount = entry->childCount;
  for ( i = 0; i < childCount; ++i )
    XAnimClearAnimSubtree_m( treePtr, entry->u.parent.firstChildIndex + i, blendTime );
}

/* ---- XAnimSetGoalWeight_m  0x00488750 ----  VERIFIED */
void __cdecl XAnimSetGoalWeight_m( XAnimTree *tree, int nodeIndex, float goalWeight,
                         float blendFactor )
{
    XAnimEntry *parentEntry;
    int         siblingCount;
    int         i;
    float       maxWeight;
    float       blendTime;
    rfloat      blendProduct;   /* x87 stack in retail (_PC_53); never stored */

    if ( nodeIndex == 0 ) {
        return;
    }

    maxWeight=0.0f;
    parentEntry  = &tree->sourceTree->entries[
                       tree->sourceTree->entries[nodeIndex].parentIndex];
    siblingCount = parentEntry->childCount;


    for ( i = 0; i < siblingCount; ++i ) {
        int            sibling = parentEntry->u.parent.firstChildIndex + i;
        unsigned short handle  = tree->poolNodeHandles[parentEntry->u.parent.firstChildIndex + i];
        float          weight  = handle ? XANIM_LANE( handle ).currentWeight
                                        : 0.0f;
        float          candidate;

        if ( sibling == nodeIndex ) {
            union { float f; unsigned int u; } bits;

            bits.f    = goalWeight - weight;
            bits.u   &= 0x7FFFFFFFu;
            candidate = bits.f;
        } else {
            candidate = weight;
        }

        if ( candidate > maxWeight ) {
            maxWeight = candidate;
        }
    }

    /* 0x004889BA fmul / fst / fcomp: the float is stored, but the
     * UNROUNDED product is what is tested against 0.001f. */
    blendProduct = (rfloat)maxWeight * blendFactor;
    blendTime    = (float)blendProduct;
    if ( blendProduct < 0.001f ) {
        blendTime = 0.0f;
    }

    for ( i = 0; i < siblingCount; ++i ) {
        if ( parentEntry->u.parent.firstChildIndex + i != nodeIndex ) {
            XAnimClearAnimNode_m( (unsigned int *)tree, parentEntry->u.parent.firstChildIndex + i, blendTime );
        }
    }
}

/* ---- XAnimSetAnimKnob_m  0x00488A10 ----  VERIFIED */
void __cdecl XAnimSetAnimKnob_m( unsigned int *tree, int node, float weight,
                                    float blendTime, float rate,
                                    __int16 notifyName, __int16 notifyType,
                                    int animFlags )
{
  if ( weight < 0.001f )
    weight = 0.0f;
  XAnimSetGoalWeight_m( (XAnimTree *)tree, node, weight, blendTime );
  XAnimSetAnimKnobInternal_m( (XAnimTree *)tree, node, weight, blendTime, rate,
                              notifyName, notifyType, animFlags );
}

/* ---- XAnimSetAnimKnobAll_m  0x00488A70 ----  VERIFIED */
int __cdecl XAnimSetAnimKnobAll_m( int node, unsigned int *treePtr, int stopNode,
                                   float weight, float blendTime, float rate,
                                   __int16 notifyName, __int16 notifyType,
                                   int animFlags )
{
  XAnimTree *tree = (XAnimTree *)treePtr;
  int        result;
  int        cur;

  xanim_currentTree = (int)treePtr;
  if ( weight < 0.001f )
    weight = 0.0f;

  XAnimSetGoalWeight_m( tree, node, weight, blendTime );
  result = XAnimSetAnimInternal_m( node, weight, blendTime, rate, 0,
                                   notifyName, notifyType );
  cand_XAnimSeedParentChain( node, blendTime );
  XAnimReseedTiming_m( node, animFlags );
  if ( xanim_activePoolSlot )
    XAnimUpdateNotifyIndex_m( node );
  cur = node;
  while ( cur )
  {
    cur = tree->sourceTree->entries[cur].parentIndex;
    if ( cur == stopNode )
      return result;

    XAnimSetGoalWeight_m( tree, cur, 1.0f, blendTime );
    XAnimSetAnimInternal_m( cur, 1.0f, blendTime, 1.0f, 0, 0, 0 );
    XAnimReseedTiming_m( cur, animFlags );
    if ( xanim_activePoolSlot )
      XAnimUpdateNotifyIndex_m( cur );
  }
  return 1;
}

/* ---- XAnimSetAnimLimited_m  0x00488BC0 ---- */
extern int __cdecl XAnimSetAnimInternalLimited_m( __int16 a1, unsigned __int16 a2, int a3, int a4,
                               float a5, float a6, float a7, int a8 );

/* ---- XAnimSetAnimLimited_m  0x00488BC0 ----  VERIFIED */
int __cdecl XAnimSetAnimLimited_m( unsigned int *tree, int node, float weight,
                                   float blendTime, float rate,
                                   __int16 notifyName, __int16 notifyType,
                                   int animFlags )
{
  if ( weight < 0.001f )
    weight = 0.0f;
  XAnimSetGoalWeight_m( (XAnimTree *)tree, node, weight, blendTime );
  return XAnimSetAnimInternalLimited_m( notifyType, notifyName, (int)tree, node,
                                        weight, blendTime, rate, animFlags );
}

/* ---- XAnimClearAnimChildren_m  0x00488C20 ----  VERIFIED */
void __cdecl XAnimClearAnimChildren_m( unsigned int *treePtr, int node, float blendTime )
{
  XAnimTree  *tree = (XAnimTree *)treePtr;
  XAnimEntry *entry;
  int         childCount;
  int         i;

  if ( blendTime < 0.001f )
    blendTime = 0.0f;

  entry      = &tree->sourceTree->entries[node];
  childCount = entry->childCount;
  for ( i = 0; i < childCount; ++i )
    XAnimClearAnimNode_m( treePtr, entry->u.parent.firstChildIndex + i, blendTime );
}

/* ---- XAnimClearTree_r  0x00488C80 ----  VERIFIED */
int __cdecl XAnimClearTree_r( unsigned int *treePtr, int node )
{
  XAnimTree     *tree   = (XAnimTree *)treePtr;
  unsigned short handle = tree->poolNodeHandles[node];
  int            result = node;

  if ( handle )
  {
    XAnimEntry *entry      = &tree->sourceTree->entries[node];
    int         childCount = entry->childCount;
    int         i;

    for ( i = 0; i < childCount; ++i )
      XAnimClearTree_r( treePtr, entry->u.parent.firstChildIndex + i );

    result = XAnimFreeInfo( handle );
    tree->poolNodeHandles[node] = 0;
  }
  return result;
}

/* ---- XAnimClearTree  0x00488CF0 ----  VERIFIED */
void __cdecl XAnimClearTree( XAnimTree *tree )
{
    unsigned short *table;
    int             nodeCount;
    int             tableIndex;
    int             size;
    int             i;

    XAnimClearTree_r( (unsigned int *)tree, 0 );

    nodeCount = tree->sourceTree->nodeCount;
    table     = (unsigned short *)( (char *)tree + 8 + 2 * nodeCount );
    size      = 3 * nodeCount + 1;

    for ( tableIndex = 0; tableIndex < 2; ++tableIndex ) {
        for ( i = tree->sourceTree->nodeCount - 1; i >= 0; --i ) {
            unsigned short handle = table[i];

            if ( handle ) {
                XAnimParts_min *parts =
                    (XAnimParts_min *)
                        ( (void **)tree->sourceTree->entries[i].u.leafAsset )[1];

                SL_RemoveRefToStringOfLen(
                    handle,
                    (unsigned int)( (int)*(const short *)parts->partNameHandles
                                    + 16 ) );
                table[i] = 0;
            }
        }
        /* loop-invariant: retail hoists (3n+1)+(2n+8)+tree to a stack slot */
        table = (unsigned short *)( (char *)&tree->poolNodeHandles[nodeCount] + size );
    }
}

/* ---- cand_XAnimClearSecondaryTiming  0x00488DA0 ----  [HIGH] */
int __cdecl cand_XAnimClearSecondaryTiming( int info )
{
  XAnimState *state = &( (XAnimInfo *)info )->states[1];

  state->time          = 0.0f;
  state->cycleCount    = 0;
  state->oldTime       = 0.0f;
  state->oldCycleCount = 0;
  return info;
}

/* ---- cand_XAnimClearPrimaryTiming  0x00488DC0 ----  VERIFIED */
int __cdecl cand_XAnimClearPrimaryTiming( int info )
{
  XAnimState *state = &( (XAnimInfo *)info )->states[0];

  state->time          = 0.0f;
  state->cycleCount    = 0;
  state->oldTime       = 0.0f;
  state->oldCycleCount = 0;
  return info;
}

/* ---- cand_XAnimFindHeaviestLeaf  0x00488DE0 ----  VERIFIED */
int __cdecl cand_XAnimFindHeaviestLeaf( int node )
{
  XAnimEntry *entry;
  int         childCount;
  int         best;
  int         leaf;
  int         i;
  float       bestWeight;
  float       weight;

  entry      = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[node];
  if ( !entry->childCount )
    return node;

  bestWeight = 0.0f;
  best       = 0;
  childCount = entry->childCount;
  for ( i = 0; i < childCount; ++i )
  {
    weight = XANIM_LANE( ((XAnimTree *)xanim_currentTree)->poolNodeHandles[entry->u.parent.firstChildIndex + i] ).targetWeight;
    /* 0x488E41 test ah,41h / jnp: a NaN weight also recurses */
    if ( !( weight <= bestWeight ) )
    {
      leaf = cand_XAnimFindHeaviestLeaf( entry->u.parent.firstChildIndex + i );
      if ( leaf )
      {
        bestWeight = weight;
        best       = leaf;
      }
    }
  }
  return best;
}

/* sign mask through an int temp: retail's fstp / mov eax / and / mov on one slot */
static __inline float XAnimFabs( float f )
{
    int tmp = *(int *)&f;

    tmp &= 0x7FFFFFFF;
    return *(float *)&tmp;
}

/* ---- XAnimSetAnimInternal_m  0x00488E80 ----  VERIFIED */
int __cdecl XAnimSetAnimInternal_m( int nodeIndex, float weight, float blendTime,
                        float rate, char force, unsigned __int16 notifyName,
                        __int16 notifyType )
{
    XAnimInfo      *node;
    unsigned short  handle;
    float           weightDelta;
    rfloat          blendRemaining; /* x87 stack in retail (_PC_53) */
    int             slot;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle ) {
        if ( weight == 0.0f && !force ) {
            return 0;
        }
        node = XAnimAllocInfo( (XAnimTree *)xanim_currentTree, nodeIndex );
        Com_Memset( node->states, 0, 2 * sizeof( XAnimState ) );
        node->notifyName       = 0;
        node->notifyIndex      = -1;
        node->notifyChildIndex = 0;
        node->notifyType       = 0;
    } else {
        node = &xanim_pool[handle];
        if ( xanim_activePoolSlot && node->notifyName ) {
            SL_RemoveRefToString( node->notifyName );
        }
    }

    if ( nodeIndex == 0 ) {
        weight    = 1.0f;
        blendTime = 0.0f;
        rate      = 1.0f;
    }

    slot = xanim_activePoolSlot;

    node->states[slot].targetWeight             = weight;
    weightDelta = weight - node->states[slot].currentWeight;
    weightDelta = XAnimFabs( weightDelta );

    /* 0x00488F59 fmul / fst / fcomp: the field gets the float, the test
     * against 0.001f sees the unrounded product. */
    blendRemaining                              = (rfloat)weightDelta * blendTime;
    node->states[slot].weightBlendTimeRemaining = (float)blendRemaining;

    if ( blendRemaining < 0.001f ) {
        node->states[slot].weightBlendTimeRemaining = 0.0f;
        node->states[slot].currentWeight            = weight;
    } else if ( node->states[slot].currentWeight == 0.0f ) {
        node->states[slot].currentWeight = weight * 0.001f;
    }
    node->states[slot].rateScale = rate;

    if ( slot ) {
        XAnimEntry *entry;

        node->notifyName = notifyName;
        if ( notifyName ) {
            SL_AddRefToString( notifyName );
        }
        node->notifyIndex = -1;
        entry = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];

        if ( !notifyName || entry->childCount == 0
          || ( entry->u.parent.flags & 3 ) == 0 ) {
            node->notifyChildIndex = 0;
            return 0;
        }

        node->notifyChildIndex = (unsigned short)cand_XAnimFindHeaviestLeaf( nodeIndex );
        if ( !node->notifyChildIndex ) {
            return 2;
        }
    } else {
        node->notifyType = notifyType;
    }
    return 0;
}

/* ---- XAnimSetRate_m  0x00489010 ----  VERIFIED */
void __cdecl XAnimSetRate_m(int nodeIndex, float rate)
{
    XAnimTree     *tree = (XAnimTree *)xanim_currentTree;
    unsigned short handle = tree->poolNodeHandles[nodeIndex];

    XANIM_LANE( handle ).rateScale = rate;
}

/*
 * Sync-node validation walks the source tree.  parent.flags bits 0/1 mark a
 * subtree that must play in sync (bit set for "first child" vs the rest); bit 2
 * marks it already checked.  The isFirst argument is a 0/1 flag -- the retail
 * decompile smuggles it in a register's low byte, but only that byte is used.
 */

/* ---- XAnimSetupSyncNodes_r  0x00489040 ----  VERIFIED */
void __cdecl XAnimSetupSyncNodes_r( unsigned int *treePtr, int node, unsigned char isFirst )
{
  XAnim *source=(XAnim *)treePtr;
  XAnimEntry *entry=&source->entries[node];
  int childCount=entry->childCount;
  int syncFlag;
  int i;
  char result;
  if ( !childCount ) {
    const int *leafAsset=(const int *)entry->u.leafAsset;
    if ( *(const unsigned char *)(leafAsset[1]+2) != (unsigned char)isFirst ) {
      if ( (unsigned char)isFirst ) {
        Com_Error(ERR_DROP,xmodel_errAnimSyncLoopingAndNonlooping,leafAsset[0],source->name);
        return;
      }
      Com_Error(ERR_DROP,xmodel_errAnimSyncNonloopingAndLooping,leafAsset[0],source->name);
    }
    return;
  }
  if ( entry->u.parent.flags & 3 ) {

    int depth=0;
    do {
      entry=&source->entries[entry->u.parent.firstChildIndex];
      ++depth;
    } while ( entry->childCount );
    Com_Error(ERR_DROP,xmodel_errDuplicateSyncSpec,source->name,depth,*(const int *)entry->u.leafAsset);
  }
  syncFlag=((unsigned char)isFirst == 0)+1;
  entry->u.parent.flags |= (unsigned short)syncFlag;
  for(i=0;i<childCount;++i)
    XAnimSetupSyncNodes_r(treePtr,entry->u.parent.firstChildIndex+i,isFirst);
  return;
}

/* ---- XAnimCheckSyncNodes_r  0x00489110 ----  VERIFIED */
void __cdecl XAnimCheckSyncNodes_r( unsigned int *treePtr, int node )
{
  XAnim        *source = (XAnim *)treePtr;
  int           childCount;
  int           syncFlags;
  unsigned char isLooping;
  int           i;

  childCount = source->entries[node].childCount;
  if ( !childCount )
    return;

  syncFlags = source->entries[node].u.parent.flags & 3;
  if ( syncFlags )
  {
    if ( syncFlags == 3 )
      Com_Error( ERR_DROP, xmodel_errSyncLoopingAndSyncNonlooping );
    source->entries[node].u.parent.flags |= 4;
    isLooping = syncFlags == 1;
    for ( i = 0; i < childCount; ++i )
      XAnimSetupSyncNodes_r( treePtr, source->entries[node].u.parent.firstChildIndex + i, isLooping );
  }
  else
  {
    for ( i = 0; i < childCount; ++i )
      XAnimCheckSyncNodes_r( treePtr, source->entries[node].u.parent.firstChildIndex + i );
  }
}

/* ---- XAnimSetupSyncNodes  0x004891B0 ----  [HIGH] */
void __cdecl XAnimSetupSyncNodes(unsigned int *a1)
{
  XAnimCheckSyncNodes_r(a1, 0);
}

/* ---- XAnimIsPrimitive  0x004891E0 ----  [HIGH] */
int __cdecl XAnimIsPrimitive( XAnim *anims, int animIndex )
{
  return anims->entries[animIndex].childCount == 0;
}

/* ---- XAnimSetTime_m  0x004891F0 ----  VERIFIED */
void __cdecl XAnimSetTime_m( XAnimTree *tree, int node, float time )
{
    unsigned short handle = tree->poolNodeHandles[node];

    if ( handle ) {
        XAnimState *lane = &XANIM_LANE( handle );

        lane->time          = time;
        lane->cycleCount    = 0;
        lane->oldTime       = time;
        lane->oldCycleCount = 0;
    }
}

/* ---- cand_XAnimCopyTiming  0x00489230 ----  VERIFIED */
int __cdecl cand_XAnimCopyTiming( int srcInfo, int dstInfo )
{
  int        slot = xanim_activePoolSlot;
  XAnimInfo *dst  = (XAnimInfo *)dstInfo;
  XAnimInfo *src  = (XAnimInfo *)srcInfo;

  dst->states[slot].time          = src->states[slot].time;
  dst->states[slot].cycleCount    = src->states[slot].cycleCount;
  dst->states[slot].oldTime       = src->states[slot].oldTime;
  dst->states[slot].oldCycleCount = src->states[slot].oldCycleCount;
  return 28 * slot;   /* leftover EAX (callers discard) */
}

/* ---- XAnimSetUser  0x00489260 ----  [CONFIRMED] */
int __cdecl XAnimSetUser(int slot)
{
  xanim_activePoolSlot = slot;
  return slot;
}

/* ---- XAnimUpdateNotifyIndex_m  0x00489270 ----  VERIFIED */
void __cdecl XAnimUpdateNotifyIndex_m( int node )
{
    XAnimInfo  *info = &xanim_pool[((XAnimTree *)xanim_currentTree)->poolNodeHandles[node]];
    XAnimEntry *entry;

    if ( !info->notifyName ) {
        return;
    }

    if ( *(const int *)&info->states[1].time == 1065353216 ) {
        info->notifyIndex = -1;
        return;
    }

    entry = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[node];
    if ( entry->childCount != 0 ) {
        if ( !info->notifyChildIndex ) {
            return;
        }
        entry = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[info->notifyChildIndex];
    }

    info->notifyIndex = (short)XAnimGetNextNotifyTime( (int)entry, (int)info,
                                           info->states[1].time );
}

/* ---- XAnimPushTimingToSubtree_m  0x004892D0 ----  VERIFIED */
void __cdecl XAnimPushTimingToSubtree_m( int node, int srcInfo )
{
  XAnimTree  *tree       = (XAnimTree *)xanim_currentTree;
  XAnimEntry *entry      = &tree->sourceTree->entries[node];
  int         childCount = entry->childCount;
  int         i;

  for ( i = 0; i < entry->childCount; ++i )
  {
    int            child  = entry->u.parent.firstChildIndex + i;
    unsigned short handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[child];

    if ( handle )
    {
      int         slot = xanim_activePoolSlot;
      XAnimInfo *dst=&xanim_pool[handle];
      XAnimInfo *src=(XAnimInfo *)srcInfo;

      dst->states[slot].time          = src->states[slot].time;
      dst->states[slot].cycleCount    = src->states[slot].cycleCount;
      dst->states[slot].oldTime       = src->states[slot].oldTime;
      dst->states[slot].oldCycleCount = src->states[slot].oldCycleCount;

      if ( slot )
        XAnimUpdateNotifyIndex_m( child );
      XAnimPushTimingToSubtree_m( child, srcInfo );
    }
  }

}

/* ---- XAnimReseedTiming_m  0x00489370 ----  VERIFIED */
/* Mac 1.3 XAnimUpdateSyncTime: walk up from nodeIndex to the nearest parent with
   flag 4; re-seed its timing, push it down its subtree, then copy it back up the
   chain below it.  The clear/copy helpers and the notify update inline here. */
void __cdecl XAnimReseedTiming_m( int nodeIndex, int forceClear )
{
    XAnimTree  *tree = (XAnimTree *)xanim_currentTree;
    int         walk = nodeIndex;
    XAnimInfo  *info;
    XAnimEntry *entry;

    while ( walk ) {
        entry = &tree->sourceTree->entries[walk];
        info  = &xanim_pool[tree->poolNodeHandles[walk]];
        if ( entry->childCount && ( entry->u.parent.flags & 4 ) ) {
            goto found;
        }
        walk = entry->parentIndex;
    }

    if ( !tree->sourceTree->entries[nodeIndex].childCount ) {
        if ( forceClear
          || !XAnimParentChainHasWeight_m( nodeIndex, tree )
          || !XAnimSubtreeHasWeight_m( tree, nodeIndex ) ) {
            if ( !xanim_activePoolSlot ) {
                cand_XAnimClearPrimaryTiming( (int)&xanim_pool[tree->poolNodeHandles[nodeIndex]] );
            } else {
                cand_XAnimClearSecondaryTiming( (int)&xanim_pool[tree->poolNodeHandles[nodeIndex]] );
            }
        }
    }
    return;

found:
    if ( forceClear
      || !XAnimParentChainHasWeight_m( walk, tree )
      || !XAnimSubtreeHasWeight_m( tree, walk ) ) {
        if ( !xanim_activePoolSlot ) {
            cand_XAnimClearPrimaryTiming( (int)info );
        } else {
            cand_XAnimClearSecondaryTiming( (int)info );
        }
    }

    XAnimPushTimingToSubtree_m( nodeIndex, (int)info );

    if ( nodeIndex == walk ) {
        return;
    }
    do {
        XAnimInfo *dst;

        cand_XAnimCopyTiming( (int)info, (int)&xanim_pool[tree->poolNodeHandles[nodeIndex]] );
        if ( xanim_activePoolSlot ) {
            /* XAnimUpdateNotifyIndex_m( nodeIndex ) written out: retail inlines it here only */
            dst = &xanim_pool[tree->poolNodeHandles[nodeIndex]];
            if ( dst->notifyName ) {
                if ( *(const int *)&dst->states[1].time == 1065353216 ) {
                    dst->notifyIndex = -1;
                } else {
                    entry = &tree->sourceTree->entries[nodeIndex];
                    if ( entry->childCount == 0 || dst->notifyChildIndex ) {
                        if ( entry->childCount != 0 ) {
                            entry = &tree->sourceTree->entries[dst->notifyChildIndex];
                        }
                        dst->notifyIndex = (short)XAnimGetNextNotifyTime(
                            (int)entry, (int)dst, dst->states[1].time );
                    }
                }
            }
        }
        nodeIndex = tree->sourceTree->entries[nodeIndex].parentIndex;
    } while ( nodeIndex != walk );
}

/* ---- cand_XAnimSeedParentChain  0x00489570 ----  VERIFIED */
void __cdecl cand_XAnimSeedParentChain( int node, float blendTime )
{
  XAnimTree *tree;

  while ( node )
  {
    tree = (XAnimTree *)xanim_currentTree;
    node = tree->sourceTree->entries[node].parentIndex;
    if ( tree->poolNodeHandles[node] )   /* already seeded -- stop climbing */
      break;
    XAnimSetAnimInternal_m( node, 0.0f, blendTime, 1.0f, 1, 0, 0 );
  }
}

/* ---- XAnimSetAnimInternalLimited_m  0x004895C0 ----  VERIFIED */
int __cdecl XAnimSetAnimInternalLimited_m( __int16 notifyType, unsigned __int16 notifyName,
                                           int tree, int node, float weight,
                                           float blendTime, float rate, int animFlags )
{
  int result;

  xanim_currentTree = tree;
  if ( weight < 0.001f )
    weight = 0.0f;
  result = XAnimSetAnimInternal_m( node, weight, blendTime, rate, 0,
                                   notifyName, notifyType );
  cand_XAnimSeedParentChain( node, blendTime );
  XAnimReseedTiming_m( node, animFlags );
  if ( xanim_activePoolSlot )
    XAnimUpdateNotifyIndex_m( node );
  return result;
}

/* ---- XAnimSetAnimRate_m  0x00489630 ----  VERIFIED */
void __cdecl XAnimSetAnimRate_m( XAnimTree *tree, int node, float rate )
{
    unsigned short handle;

    handle = tree->poolNodeHandles[node];
    xanim_currentTree = (int)tree;
    XANIM_LANE( handle ).rateScale = rate;
}

/* ---- XAnimIsLooped_m  0x00489660 ----  VERIFIED */
int __cdecl XAnimIsLooped_m( const XAnim *sourceTree, int node )
{
  const XAnimEntry *entry = &sourceTree->entries[node];

  if ( entry->childCount )
    return entry->u.parent.flags & 1;
  return *(const unsigned char *)( ((const int *)entry->u.leafAsset)[1] + 2 );
}

/* ---- XAnimSetAnimKnobInternal_m  0x004896B0 ----  VERIFIED */
void __cdecl XAnimSetAnimKnobInternal_m( XAnimTree *tree, int node, float weight,
                            float blendTime, float rate, unsigned __int16 notifyName,
                            __int16 notifyType, int animFlags )
{
    int ancestor;

    xanim_currentTree = (int)tree;
    if ( weight < 0.001f ) {
        weight = 0.0f;
    }

    XAnimSetAnimInternal_m( node, weight, blendTime, rate, 0,
                notifyName, notifyType );

    ancestor = node;
    while ( ancestor ) {
        unsigned short handle;

        ancestor = tree->sourceTree->entries[ancestor].parentIndex;
        handle   = tree->poolNodeHandles[ancestor];
        if ( !handle || XANIM_LANE( handle ).targetWeight == 0.0f ) {
            XAnimSetAnimInternal_m( ancestor, 1.0f, blendTime, 1.0f, 0, 0, 0 );
        }
    }

    XAnimReseedTiming_m( node, animFlags );
    if ( xanim_activePoolSlot ) {
        XAnimUpdateNotifyIndex_m( node );
    }
}

/*
 * A saved XAnimState is serialized (and restored) as a fixed 28-byte record in
 * this field order, which differs from the in-memory struct order:
 *   [+0]time [+4]oldTime [+8]cycleCount [+10]oldCycleCount
 *   [+12]targetWeight [+16]rateScale [+20]weightBlendTimeRemaining [+24]currentWeight
 */

/* ---- cand_XAnimReadTransform  0x00489780 ----  VERIFIED */
float *__cdecl cand_XAnimReadTransform( float *dstState )
{
  extern float ReadFloat(void);
  extern __int16 ReadShort(void);
  XAnimState *state=(XAnimState *)dstState;
  state->time=ReadFloat();
  state->oldTime=ReadFloat();
  state->cycleCount=ReadShort();
  state->oldCycleCount=ReadShort();
  state->targetWeight=ReadFloat();
  state->rateScale=ReadFloat();
  state->weightBlendTimeRemaining=ReadFloat();
  state->currentWeight=ReadFloat();
  return (float *)scrSave_readPos;
}

/* ---- cand_XAnimWriteState  0x004897E0 ----  VERIFIED */
void __cdecl cand_XAnimWriteState( const XAnimState *state )
{
  extern void WriteShort(unsigned short value);
  extern void WriteFloat(float value);

  WriteFloat( state->time );
  WriteFloat( state->oldTime );
  WriteShort( state->cycleCount );
  WriteShort( state->oldCycleCount );
  WriteFloat( state->targetWeight );
  WriteFloat( state->rateScale );
  WriteFloat( state->weightBlendTimeRemaining );
  WriteFloat( state->currentWeight );
}

/* ---- cand_XAnimReadString  0x004898F0 ----  VERIFIED */
void __cdecl cand_XAnimReadString( int infoPtr )
{
  extern __int16 ReadShort(void);
  extern __int16 SafeReadString(void);
  extern char ReadByte(void);
  extern __int16 ReadString(void);
  XAnimInfo *info=(XAnimInfo *)infoPtr;
  info->notifyIndex=ReadShort();
  info->notifyChildIndex=ReadShort();
  info->notifyType=ReadShort();
  info->notifyName=ReadByte()?ReadString():(__int16)0;
  cand_XAnimReadTransform((float *)&info->states[0]);
  cand_XAnimReadTransform((float *)&info->states[1]);
}

/* ---- cand_XAnimWriteInfo  0x004899F0 ----  VERIFIED */
void __cdecl cand_XAnimWriteInfo( int *infoPtr )
{
  extern void *TempMalloc(int size);
  extern void WriteShort(unsigned short value);
  extern void WriteByte(unsigned char value);
  extern void SafeWriteString(unsigned short value);
  XAnimInfo *info=(XAnimInfo *)infoPtr;
  WriteShort(info->notifyIndex);
  WriteShort(info->notifyChildIndex);
  WriteShort(info->notifyType);
  if(info->notifyName){WriteByte(1);WriteString(info->notifyName);}else WriteByte(0);
  cand_XAnimWriteState(&info->states[0]);
  cand_XAnimWriteState(&info->states[1]);
}

/* ---- cand_XAnimReadInfo  0x00489AB0 ----  VERIFIED */
void __cdecl cand_XAnimReadInfo( XAnimInfo *dst, const void *src )
{
  memcpy( dst, src, 0x44u );

  if ( dst->notifyName )
    ++*(unsigned short *)SL_REFSTRING( dst->notifyName );
}

/* ---- cand_XAnimRestoreTree  0x00489AE0 ----  VERIFIED */
unsigned int __cdecl cand_XAnimRestoreTree( int treePtr )
{
  XAnimTree     *tree = (XAnimTree *)treePtr;
  unsigned int   bitmapBytes;
  char          *bitmap;
  int            savedReadPos;
  unsigned int   nodeCount;
  int            i;

  bitmapBytes  = ( ( (unsigned int)tree->sourceTree->nodeCount - 1 ) >> 3 ) + 1;
  bitmap       = (char *)alloca( bitmapBytes );
  savedReadPos = (int)scrSave_readPos;
  Com_Memcpy( bitmap, (const void *)scrSave_readPos, bitmapBytes );
  nodeCount    = tree->sourceTree->nodeCount;
  scrSave_readPos = (char *)( savedReadPos + bitmapBytes );

  for ( i = 0; i < nodeCount; ++i )
  {
    if ( ( bitmap[i >> 3] & (1 << (i & 7)) ) != 0 )
    {
      XAnimInfo *info = XAnimAllocInfo( tree, i );
      cand_XAnimReadString( (int)info );
    }
    nodeCount = tree->sourceTree->nodeCount;
  }
  return nodeCount;
}

/* ---- cand_XAnimSaveTree  0x00489B70 ----  VERIFIED */
void __cdecl cand_XAnimSaveTree( XAnimTree *tree )
{
  unsigned int   bitmapBytes;
  unsigned char *bitmap;
  int            i;
  int            newPos;
  char          *dst;

  bitmapBytes = ( ( (unsigned int)tree->sourceTree->nodeCount - 1 ) >> 3 ) + 1;
  bitmap = (unsigned char *)alloca( bitmapBytes );
  Com_Memset( bitmap, 0, bitmapBytes );

  for ( i = 0; i < (unsigned int)tree->sourceTree->nodeCount; i++ )
    if ( tree->poolNodeHandles[i] )
      bitmap[i >> 3] |= 1 << ( i & 7 );

  newPos = currentPos + bitmapBytes;
  dst    = (char *)Hunk_ReallocateTempMemory( newPos ) + currentPos;
  currentPos = newPos;
  Com_Memcpy( dst, bitmap, bitmapBytes );

  for ( i = 0; i < (unsigned int)tree->sourceTree->nodeCount; i++ )
    if ( tree->poolNodeHandles[i] )
      cand_XAnimWriteInfo( (int *)&xanim_pool[ tree->poolNodeHandles[i] ] );
}

/* ---- XAnimCopyTree  0x00489C90 ----  VERIFIED */
void __cdecl XAnimCopyTree( const XAnimTree *from, XAnimTree *to )
{
    int            nodeCount;
    int            i;
    unsigned short srcHandle;
    XAnimInfo     *dst;

    nodeCount = from->sourceTree->nodeCount;

    for ( i = 0 ; i < nodeCount ; i++ ) {
        srcHandle = from->poolNodeHandles[i];

        if ( !srcHandle ) {
            if ( to->poolNodeHandles[i] ) {
                XAnimFreeInfo( to->poolNodeHandles[i] );
                to->poolNodeHandles[i] = 0;
            }
            continue;
        }

        if ( !to->poolNodeHandles[i] ) {
            dst = XAnimAllocInfo( to, i );
        } else {
            dst = &xanim_pool[ to->poolNodeHandles[i] ];
            if ( dst->notifyName ) {
                XModelReleaseString( dst->notifyName );
            }
        }

        memcpy( dst, &xanim_pool[srcHandle], sizeof( XAnimInfo ) );

        if ( dst->notifyName ) {
            SL_AddRefToString( dst->notifyName );
        }
    }
}

static void XModelReadSurface(XSurface *surface, const unsigned char **pos,
                              XModelAllocFn alloc);
static XModelSurfs *XModelSurfsCloneSurfs(const XModelSurfs *entry,
                                          XModelAllocFn alloc);
static XModelPartsEntry *XModelCreateDefaultParts(void);
static XModelSurfs *XModelCreateDefaultSurfs(void);
void XModelCreateDefault(XModel *entry);
XModel *XModelCreateDefault_(void);
void XModelPartsFree(XModelPartsEntry *entry);
void XModelFree(XModel *entry);
void XModelSurfsFree(XModelSurfs *entry);
void XModelExpandQuatToAxis(float *quat);
int ReadQuat(const short *packed, short *out);

/* ---- XModelEnforceExist  0x00489DB0 ---- */
void __cdecl XModelEnforceExist(int enforce)
{
    xmodel_enforceExist = enforce;
}

/* ---- XModelBad  0x00489DC0 ----  VERIFIED */
int __cdecl XModelBad(const XModel *model)
{
    return model->info == &xmodel_defaultCollision;
}

/* ---- SetAnimCheck  0x00489DD0 ----  VERIFIED */
void XModel_SetAnimCheck(int result)
{
 xmodel_animCheck=result;
}

/* ---- ReadQuat  0x00489DE0 ----  VERIFIED */
int ReadQuat(const short *packed, short *out)
{
    int x, y, z;
    int lenSq;
    int w;

    out[0] = packed[0];
    out[1] = packed[1];
    out[2] = packed[2];
    x = out[0];
    y = out[1];
    z = out[2];
    lenSq = 1073676289 - z * z - y * y - x * x;
    if ( lenSq > 0 )
        w = (int)(unsigned int)floor(sqrt((double)lenSq) + 0.5);
    else
        w = 0;
    out[3] = (short)w;
    return w;
}

/* ---- ReadQuat2  0x00489E60 ----  VERIFIED */
int ReadQuat2(const short *packed, short *out)
{
    short x = packed[0];
    int lenSq;
    int w;

    out[0] = x;
    lenSq = 1073676289 - x * x;
    if ( lenSq > 0 )
        w = (int)(unsigned int)floor(sqrt((double)lenSq) + 0.5);
    else
        w = 0;
    out[1] = (short)w;
    return w;
}

/* ---- XModelExpandQuatToAxis  0x00489EB0 ----  VERIFIED */
__inline void __cdecl XModelExpandQuatToAxis(float *quat)
{

    float x2, y2, z2;
    float lengthSq, scale, xx, yy, zz, x, y, xy, zw;
    float xz, xw, yz, yw;   /* retail's four float spills; xy and zw stay on the
                             * x87 stack (0x489F19 / 0x489F45) and are never stored */

    x2 = quat[0] * quat[0];
    y2 = quat[1] * quat[1];
    z2 = quat[2] * quat[2];
    lengthSq = quat[3] * quat[3] + z2 + y2 + x2;
    if (lengthSq != 0.0f)
    {
    scale = 2.0f / lengthSq;
    xx = x2 * scale;
    yy = scale * y2;
    zz = scale * z2;
    x  = scale * quat[0];
    xy = x * quat[1];
    xz = (float)(x * quat[2]);
    xw = (float)(x * quat[3]);
    y  = scale * quat[1];
    yz = (float)(y * quat[2]);
    yw = (float)(y * quat[3]);
    zw = quat[3] * quat[2] * scale;

    quat[0]  = (float)(1.0f - (zz + yy));
    quat[1]  = (float)(zw + xy);
    quat[2]  = xz - yw;
    quat[4]  = (float)(xy - zw);
    quat[5]  = (float)(1.0f - (zz + xx));
    quat[6]  = yz + xw;
    quat[8]  = yw + xz;
    quat[9]  = yz - xw;
    quat[10] = (float)(1.0f - (yy + xx));
    }
    else
    {

        quat[0]  = 1.0f; quat[1] = 0.0f; quat[2]  = 0.0f;
        quat[4]  = 0.0f; quat[5] = 1.0f; quat[6]  = 0.0f;
        quat[8]  = 0.0f; quat[9] = 0.0f; quat[10] = 1.0f;

        }
}

/* ---- XModelSurfsFree  0x00489FE0 ----  VERIFIED */
void XModelSurfsFree(XModelSurfs *entry)
{
    (void)entry;
}

/* ---- XModelPartsFree  0x00489FF0 ----  VERIFIED */
void __cdecl XModelPartsFree(XModelPartsEntry *entry)
{
    const XModelPartNameTable *table =
        entry->data->partNameTableSlot->partNameTable;
    const unsigned short *handles = table->handles;
    int count = table->count;
    int i;

    for ( i = 0; i < count; ++i )
        XModelReleaseString(handles[i]);
}

/* ---- XModelFree  0x0048A070 ----  VERIFIED */
void __cdecl XModelFree(XModel *entry)
{
    XModelInfo *info = entry->info;
    int lodIndex;
    int i;

    for ( lodIndex = 0; lodIndex < XMODEL_LOD_COUNT; ++lodIndex )
    {
        if ( !info->lodRecords[lodIndex].surfaceNameTable )
            continue;
        for ( i = 0; i < info->lodRecords[lodIndex].surfaceCount; ++i )
            XModelReleaseString(info->lodRecords[lodIndex].surfaceNameTable[i]);
        info->lodRecords[lodIndex].surfaceNameTable = 0;
    }
}

/* ---- ReadBlend  0x0048A100 ---- */
static void ReadBlend(XSurface *surface, XSimpleBlendInfo *blend,
                      const unsigned char **pos)
{
    short boneIndex;

    boneIndex = *(const short *)*pos;

    ((unsigned char *)surface->boneUsage)[boneIndex >> 3] |=
        (unsigned char)(1u << (boneIndex & 7));
    blend->boneMatrixOffset = (unsigned int)(boneIndex << XMODEL_BONE_MATRIX_SHIFT);

    *pos += 2;
    blend->position[0] = *(const float *)*pos; *pos += 4;
    blend->position[1] = *(const float *)*pos; *pos += 4;
    blend->position[2] = *(const float *)*pos; *pos += 4;


}

/* ---- XSurfaceUnstrip  0x0048A160 ---- */
static void XSurfaceUnstrip(const XStripInfo *strips, unsigned short (*triangles)[3])
{
 const unsigned short *stripIndex=strips->stripIndices;
 unsigned short vertices[3];
 int strip;
 for(strip=0;strip<strips->stripCount;++strip){
 int vertexCount=strips->stripVertexCounts[strip];
 int stripVertex;
 vertices[0]=stripIndex[0];vertices[1]=stripIndex[1];vertices[2]=stripIndex[2];stripIndex+=3;
if(vertices[0]!=vertices[1]&&vertices[0]!=vertices[2]&&vertices[1]!=vertices[2]){(*triangles)[0]=vertices[0];(*triangles)[1]=vertices[1];(*triangles)[2]=vertices[2];++triangles;}
for(stripVertex=3;stripVertex<vertexCount;){
 vertices[0]=vertices[2];vertices[2]=*stripIndex++;
if(vertices[0]!=vertices[1]&&vertices[0]!=vertices[2]&&vertices[1]!=vertices[2]){(*triangles)[0]=vertices[0];(*triangles)[1]=vertices[1];(*triangles)[2]=vertices[2];++triangles;}
if(++stripVertex>=vertexCount)break;
 vertices[1]=vertices[2];vertices[2]=*stripIndex++;
if(vertices[0]!=vertices[1]&&vertices[0]!=vertices[2]&&vertices[1]!=vertices[2]){(*triangles)[0]=vertices[0];(*triangles)[1]=vertices[1];(*triangles)[2]=vertices[2];++triangles;}
++stripVertex;
}
}
}

/* ---- XModelReadSurface  0x0048A260 ----  VERIFIED */
static void XModelReadSurface(XSurface *surface, const unsigned char **pos,
                              XModelAllocFn alloc)
{
    int singleBoneIndex;
    int weightedPointCount;
    short compactWeightedVertexCount;
    int totalStripIndices;
    int stripScan;
    unsigned short *stripIndexOut;
    unsigned char *blendCursor;
    XSurfaceRigidVert *rigidVertex;
    float (*texCoords)[2];
    XSurfaceWeightedPoint *weighted;
    XStripInfo strips;
    short triangleAllocCount;
    int i;
    int j;

    surface->tileMode = *(*pos);              (*pos) += 1;
    surface->vertexCount = *(const short *)(*pos);   (*pos) += 2;
    surface->triangleCount = *(const short *)(*pos); (*pos) += 2;
    strips.stripCount = *(const short *)(*pos);             (*pos) += 2;
    singleBoneIndex = *(const short *)(*pos);        (*pos) += 2;

    Com_Memset(surface->boneUsage, 0, 16);

    if ( singleBoneIndex == XMODEL_NO_SINGLE_BONE )
    {
        weightedPointCount = *(const short *)(*pos);         (*pos) += 2;
        compactWeightedVertexCount = *(const short *)(*pos); (*pos) += 2;
        surface->boneMatrixOffset = XMODEL_NO_SINGLE_BONE;
        surface->vertexData =
            alloc(4 * (9 * (int)surface->vertexCount - (int)compactWeightedVertexCount));
        surface->weightedPoints =
            (XSurfaceWeightedPoint *)alloc(20 * (int)weightedPointCount);
        surface->texCoords = (float (*)[2])alloc(8 * (int)surface->vertexCount);
    }
    else
    {
        surface->boneMatrixOffset =
            (short)(singleBoneIndex << XMODEL_BONE_MATRIX_SHIFT);
        ((unsigned char *)surface->boneUsage)[singleBoneIndex >> 3] |=
            (unsigned char)(1u << (singleBoneIndex & 7));
        weightedPointCount = 0;
        surface->vertexData = alloc(24 * (int)surface->vertexCount);
        surface->weightedPoints = 0;
        surface->texCoords = (float (*)[2])alloc(8 * (int)surface->vertexCount);
    }

    totalStripIndices = 0;
    stripScan = 0;
    for ( j = 0; j < strips.stripCount; ++j )
    {
        unsigned char n = (*pos)[stripScan];

        totalStripIndices += n;
        stripScan += 1 + 2 * (int)n;
    }

    strips.stripVertexCounts = (unsigned char *)alloca((size_t)strips.stripCount);
    strips.stripIndices = (unsigned short *)alloca((size_t)(2 * totalStripIndices));

    stripIndexOut = (unsigned short *)strips.stripIndices;
    for ( j = 0; j < strips.stripCount; ++j )
    {
        unsigned char n = *(*pos);

        ((unsigned char *)strips.stripVertexCounts)[j] = n;
        (*pos) += 1;
        memcpy(stripIndexOut, (*pos), (size_t)(2 * (int)n));
        (*pos) += 2 * (int)n;
        stripIndexOut += n;
    }

    texCoords   = surface->texCoords;
    blendCursor = (unsigned char *)surface->vertexData;
    for ( i = 0; i < surface->vertexCount; ++i )
    {
        XSurfaceBlendVert *vert = (XSurfaceBlendVert *)blendCursor;

        vert->normal[0] = *(const float *)(*pos); (*pos) += 4;
        vert->normal[1] = *(const float *)(*pos); (*pos) += 4;
        vert->normal[2] = *(const float *)(*pos); (*pos) += 4;
        (*texCoords)[0] = *(const float *)(*pos); (*pos) += 4;
        (*texCoords)[1] = *(const float *)(*pos); (*pos) += 4;

        if ( singleBoneIndex == XMODEL_NO_SINGLE_BONE )
        {
            vert->additiveWeightCount = *(const short *)(*pos); (*pos) += 2;

            ReadBlend(surface, &vert->blend, pos);

            if ( vert->additiveWeightCount )
            {
                vert->primaryWeight = *(const float *)(*pos); (*pos) += 4;
                blendCursor += 36;
            }
            else
            {
                blendCursor += 32;
            }
            ++texCoords;
        }
        else
        {
            rigidVertex = (XSurfaceRigidVert *)blendCursor;
            rigidVertex->position[0] = *(const float *)(*pos); (*pos) += 4;
            rigidVertex->position[1] = *(const float *)(*pos); (*pos) += 4;
            rigidVertex->position[2] = *(const float *)(*pos); (*pos) += 4;
            blendCursor += 24;
            ++texCoords;
        }
    }

    weighted = surface->weightedPoints;
    for ( i = 0; i < weightedPointCount; ++i )
    {
        ReadBlend(surface, &weighted->blend, pos);
        weighted->weight = *(const float *)(*pos);
        (*pos) += 4;
        ++weighted;
    }

    triangleAllocCount=surface->triangleCount;
    if ((triangleAllocCount & 1)!=0){
      surface->triangleCount=++triangleAllocCount;
      surface->triangles=(unsigned short (*)[3])alloc(6*(int)triangleAllocCount);
      XSurfaceUnstrip(&strips,surface->triangles);
      surface->triangles[surface->triangleCount-1][0]=surface->triangles[surface->triangleCount-2][2];
      surface->triangles[surface->triangleCount-1][1]=surface->triangles[surface->triangleCount-2][2];
      surface->triangles[surface->triangleCount-1][2]=surface->triangles[surface->triangleCount-2][2];
    }else{
      surface->triangles=(unsigned short (*)[3])alloc(6*(int)triangleAllocCount);
      XSurfaceUnstrip(&strips,surface->triangles);
    }
}

/* ---- XModelCreateDefaultSurface  0x0048A800 ----  VERIFIED */
static void __cdecl XModelCreateDefaultSurface(XSurface *surface)
{
    XSurfaceRigidVert *verts;
    float (*texCoords)[2];
    int i;

    surface->triangleCount = 1;
    surface->vertexCount = 3;
    Com_Memset(surface->boneUsage, 0, 16);
    surface->vertexData = xmodel_defaultRigidVerts;
    surface->texCoords = xmodel_defaultTexCoords;
    surface->boneMatrixOffset = 0;
    surface->weightedPoints = 0;
    surface->triangles = (unsigned short (*)[3])xmodel_defaultTriangle;
    xmodel_defaultTriangle[0] = 0;
    xmodel_defaultTriangle[1] = 1;
    xmodel_defaultTriangle[2] = 2;

    verts = (XSurfaceRigidVert *)surface->vertexData;
    texCoords = surface->texCoords;
    for ( i = 0; i < surface->vertexCount; ++i )
    {
        verts->normal[0] = 1.0f;
        verts->normal[1] = 0.0f;
        verts->normal[2] = 0.0f;
        (*texCoords)[0] = 0.0f;
        (*texCoords)[1] = 0.0f;
        verts->position[0] = 0.0f;
        verts->position[1] = 0.0f;
        verts->position[2] = 0.0f;
        ++verts;
        ++texCoords;
    }
}

/* ---- XModelCreateDefaultParts  0x0048A890 ----  VERIFIED */
static XModelPartsEntry *XModelCreateDefaultParts(void)
{
    xmodel_defaultParts.baseRotations = 0;
    xmodel_defaultParts.baseTranslations = 0;
    xmodel_defaultPartStateIndices[0] = 0;
    xmodel_defaultPartNameTable.handles[0] = 0;
    xmodel_defaultPartNameTable.count = 1;
    xmodel_defaultPartsEntry.freeData = 0;
    xmodel_defaultPartNameTableSlot.partNameTable = &xmodel_defaultPartNameTable;
    xmodel_defaultParts.partNameTableSlot = &xmodel_defaultPartNameTableSlot;
    xmodel_defaultParts.rootPartCount = 1;
    xmodel_defaultParts.partStateIndices = xmodel_defaultPartStateIndices;

    xmodel_defaultPartColl.mins[0] = -16.0f;
    xmodel_defaultPartColl.mins[1] = -16.0f;
    xmodel_defaultPartColl.mins[2] = -16.0f;
    xmodel_defaultPartColl.maxs[0] =  16.0f;
    xmodel_defaultPartColl.maxs[1] =  16.0f;
    xmodel_defaultPartColl.maxs[2] =  16.0f;
    xmodel_defaultParts.partCollisions = &xmodel_defaultPartColl;

    xmodel_defaultPartsEntry.data = &xmodel_defaultParts;
    xmodel_defaultPartsEntry.name = "DEFAULT";
    return &xmodel_defaultPartsEntry;
}

/* ---- XModelCreateDefaultSurfs  0x0048A940 ----  VERIFIED */
static XModelSurfs *XModelCreateDefaultSurfs(void)
{
    xmodel_defaultSurfs.surfaceCount = 1;
    xmodel_defaultSurfacePtrs[0] = &xmodel_defaultSurfaces[0];
    xmodel_defaultSurfs.surfaces = xmodel_defaultSurfacePtrs;
    XModelCreateDefaultSurface(&xmodel_defaultSurfaces[0]);

    xmodel_defaultSurfsEntry.surfs = &xmodel_defaultSurfs;
    xmodel_defaultSurfsEntry.freeData = 0;
    xmodel_defaultSurfsEntry.name = "DEFAULT";
    return &xmodel_defaultSurfsEntry;
}

/* ---- XModelCreateDefault  0x0048A990 ----  VERIFIED */
void __cdecl XModelCreateDefault(XModel *entry)
{
    int lodIndex;

    xmodel_defaultCollision.parts = XModelCreateDefaultParts();

    for ( lodIndex = 0; lodIndex < XMODEL_LOD_COUNT; ++lodIndex )
    {
        xmodel_defaultCollision.lodRecords[lodIndex].surfs = 0;
        xmodel_defaultCollision.lodRecords[lodIndex].name = "";
        xmodel_defaultCollision.lodRecords[lodIndex].distance = 0.0f;
        xmodel_defaultCollision.lodRecords[lodIndex].surfaceCount = 1;
        xmodel_defaultCollision.lodRecords[lodIndex].surfaceNameTable = xmodel_defaultSurfaceNames;
        memset( xmodel_defaultSurfaceNames, 0, sizeof( xmodel_defaultSurfaceNames ) );
    }

    xmodel_defaultCollision.lodRecords[0].surfs = XModelCreateDefaultSurfs();
    entry->freeData = 0;
    entry->info = &xmodel_defaultCollision;
    xmodel_defaultCollision.lodCount = 1;
    xmodel_defaultCollision.modelFileCount = 0;
}

/* ---- XModelCreateDefault_  0x0048AA40 ----  VERIFIED */
XModel *XModelCreateDefault_(void)
{
    XModelCreateDefault(&xmodel_defaultModelEntry);
    xmodel_defaultModelEntry.name = "DEFAULT";
    return &xmodel_defaultModelEntry;
}

/* ---- XModelSurfsCloneSurfs  0x0048AA60 ---- */
static XModelSurfs *XModelSurfsCloneSurfs(const XModelSurfs *entry,
                                          XModelAllocFn alloc)
{
    XModelSurfsData *clone = (XModelSurfsData *)alloc(12);
    XModelSurfs *asset;

    clone->surfaceCount = entry->surfs->surfaceCount;
    clone->surfaces = (XSurface **)alloc(4 * (int)clone->surfaceCount);
    Com_Memcpy(clone->surfaces, entry->surfs->surfaces,
               (size_t)(4 * (int)clone->surfaceCount));

    clone->next = xmodel_surfsCloneList;
    xmodel_surfsCloneList = clone;

    asset = (XModelSurfs *)alloc(12);
    asset->surfs = clone;
    asset->freeData = XModelSurfsFree;
    asset->name = entry->name;
    return asset;
}

/* ---- XModelSurfsPrecache  0x0048AAC0 ----  VERIFIED */
XModelSurfs *__cdecl XModelSurfsPrecache(const char *name, XModelAllocFn alloc)
{
    XModelSurfs *entry;
    void *fileBuffer;
    const unsigned char *cursor;
    XModelSurfsData *surfs;
    XSurface *surfaces;
    int version;
    short surfaceCount;
    int i;
    char path[1024];

    entry = (XModelSurfs *)FS_GetDataForFile("xmodelsurfs", name, "");
    if ( !entry )
    {
        if ( xmodel_enforceExist )
        {
            Com_Error(ERR_DROP,
                      "\x15" "Cannot precache 'xmodelsurfs/%s'.\n"
                      "you may need to get latest and run converter to fix",
                      name);
            return 0;
        }
        Com_Printf("ERROR: Cannot precache 'xmodelsurfs/%s'", name);
        return 0;
    }

    if ( entry->surfs )
        return XModelSurfsCloneSurfs(entry, alloc);

    sprintf(path, "xmodelsurfs/%s", name);
    if ( FS_ReadFile(path, &fileBuffer) < 0 )
    {
        Com_Error(ERR_DROP, "\x15" "Cannot find 'xmodelsurfs/%s'.", name);
        return 0;
    }

    cursor = (const unsigned char *)fileBuffer;
    version = *(const short *)cursor;
    cursor += 2;
    if ( version != XMODEL_ASSET_VERSION )
    {
        FS_FreeFile(fileBuffer);
        Com_Error(ERR_DROP,
                  "\x15" "xmodelsurfs '%s' out of date (version %d, expecting %d)",
                  name, version, XMODEL_ASSET_VERSION);
        return 0;
    }

    surfaceCount = *(const short *)cursor;
    cursor += 2;

    surfs = (XModelSurfsData *)alloc(12);
    surfs->surfaceCount = surfaceCount;
    surfs->surfaces = (XSurface **)alloc(4 * (int)surfaceCount);
    surfaces = (XSurface *)alloc(52 * (int)surfaceCount);

    for ( i = 0; i < surfaceCount; ++i )
    {
        surfs->surfaces[i] = &surfaces[i];
        XModelReadSurface(surfs->surfaces[i], &cursor, alloc);
    }

    FS_FreeFile(fileBuffer);

    entry->surfs = surfs;
    entry->freeData = XModelSurfsFree;
    surfs->next = xmodel_surfsCloneList;
    xmodel_surfsCloneList = surfs;
    return entry;
}

/* ---- XModelPartsPrecache  0x0048AC80 ----  VERIFIED */
XModelPartsEntry *__cdecl XModelPartsPrecache(const char *name, XModelAllocFn alloc)
{
    XModelPartsEntry *entry;
    void *fileBuffer;
    const unsigned char *cursor;
    XModelPartNameTable *table;
    XModelPartNameTableSlot *slot;
    unsigned char *parentList;
    XModelPartsData *parts;
    XModelPartColl *partColls;
    short (*rotation)[4];
    float (*translation)[3];
    int version;
    short childPartCount;
    short rootPartCount;
    short totalPartCount;
    int i;
    vec3_t diff;
    char path[1024];

    /* table lives in its stack slot (retail reloads it for every handle store
     * instead of strength-reducing &table->handles[i] to a pointer walk) */
    if ( 0 ) *(void **)0 = (void *)&(table);
    entry = (XModelPartsEntry *)FS_GetDataForFile("xmodelparts", name, "");
    if ( !entry )
    {
        if ( xmodel_enforceExist )
        {
            Com_Error(ERR_DROP,
                      "\x15" "Cannot precache 'xmodelparts/%s'.\n"
                      "you may need to get latest and run converter to fix",
                      name);
            return 0;
        }
        Com_Printf("ERROR: Cannot precache 'xmodelparts/%s'", name);
        return 0;
    }

    if ( entry->data )
        return entry;

    sprintf(path, "xmodelparts/%s", name);
    if ( FS_ReadFile(path, &fileBuffer) < 0 )
    {
        Com_Error(ERR_DROP, "\x15" "Cannot find 'xmodelparts/%s'.", name);
        return 0;
    }

    cursor = (const unsigned char *)fileBuffer;
    version = *(const short *)cursor;
    cursor += 2;
    if ( version != XMODEL_ASSET_VERSION )
    {
        FS_FreeFile(fileBuffer);
        Com_Error(ERR_DROP,
                  "\x15" "xmodelparts '%s' out of date (version %d, expecting %d)",
                  name, version, XMODEL_ASSET_VERSION);
        return 0;
    }

    childPartCount = *(const short *)cursor;
    cursor += 2;
    rootPartCount = *(const short *)cursor;
    cursor += 2;
    totalPartCount = childPartCount + rootPartCount;

    table = (XModelPartNameTable *)alloc(2 * totalPartCount + 2);
    table->count = totalPartCount;
    if ( totalPartCount >= XMODEL_MAX_BONES + 1 )
    {
        Com_Error(ERR_DROP, "\x15" "xmodel '%s' has more than %d bones",
                  name, XMODEL_MAX_BONES);
        return 0;
    }

    slot = (XModelPartNameTableSlot *)alloc(childPartCount + 7);
    slot->partNameTable = table;
    parentList = slot->parentPartDeltas;

    parts = (XModelPartsData *)alloc(24 * childPartCount + 104);
    parts->partNameTableSlot = slot;
    parts->baseRotations = (short (*)[4])((char *)parts + 24);
    parts->baseTranslations = childPartCount
        ? (float (*)[3])((char *)parts->baseRotations + 8 * childPartCount) : 0;
    parts->partStateIndices = (unsigned char *)alloc(totalPartCount);
    parts->rootPartCount = rootPartCount;

    partColls = (XModelPartColl *)alloc(40 * totalPartCount);

    rotation    = parts->baseRotations;
    translation = parts->baseTranslations;
    /* parentList[i - rootPartCount], with the base pre-biased: retail indexes
     * [base + i] (0x48AE6B) instead of strength-reducing a second pointer */
    parentList -= rootPartCount;
    for ( i = rootPartCount; i < totalPartCount; ++i )
    {
        parentList[i] = (unsigned char)(i - *cursor);
        cursor += 1;
        VectorCopy( (const float *)cursor, *translation );
        (*translation)[0] = FloatNoSwap( (*translation)[0] );
        (*translation)[1] = FloatNoSwap( (*translation)[1] );
        (*translation)[2] = FloatNoSwap( (*translation)[2] );
        cursor += 12;
        ReadQuat((const short *)cursor, *rotation);
        cursor += 6;
        ++rotation;
        ++translation;
    }

    for ( i = 0; i < (int)table->count; ++i )
    {
        XModelPartColl *coll = &partColls[i];
        size_t nameLength = strlen((const char *)cursor)+1;

        /* type 10 -- `push 0Ah` at 0x0048AED9. */
        table->handles[i] = (unsigned short)
            SL_GetStringOfLen((const char *)cursor, 0, nameLength, 10);
        cursor += nameLength;

        VectorCopy( (const float *)cursor, coll->mins );
        coll->mins[2] = FloatNoSwap( coll->mins[2] );
        cursor += 12;
        VectorCopy( (const float *)cursor, coll->maxs );
        coll->maxs[1] = FloatNoSwap( coll->maxs[1] );
        cursor += 12;

        coll->center[0] = coll->maxs[0] + coll->mins[0];
        coll->center[1] = coll->maxs[1] + coll->mins[1];
        coll->center[2] = coll->maxs[2] + coll->mins[2];
        coll->center[0] = coll->center[0] * 0.5f;
        coll->center[1] = coll->center[1] * 0.5f;
        coll->center[2] = coll->center[2] * 0.5f;

        VectorSubtract( coll->maxs, coll->center, diff );
        coll->radiusSq = diff[2] * diff[2] + diff[1] * diff[1] + diff[0] * diff[0];
    }

    memcpy(parts->partStateIndices, cursor, (size_t)totalPartCount);
    FS_FreeFile(fileBuffer);

    parts->partCollisions = partColls;
    entry->data = parts;
    entry->freeData = XModelPartsFree;
    return entry;
}

/* ---- XModelLoadConfigFile  0x0048B020 ----  VERIFIED */
static const unsigned char *XModelLoadConfigFile(const char *name,
                                                 const unsigned char *cursor,
                                                 XModelConfig *config)
{
    int version;
    int lodIndex;

    version = *(const short *)cursor;
    cursor += 2;
    if ( version != XMODEL_ASSET_VERSION )
    {
        Com_Error(ERR_DROP,
                  "\x15" "xmodel '%s' out of date (version %d, expecting %d)",
                  name, version, XMODEL_ASSET_VERSION);
        return 0;
    }

    config->mins[0] = *(const float *)cursor; cursor += 4;
    config->mins[1] = *(const float *)cursor; cursor += 4;
    config->mins[2] = *(const float *)cursor; cursor += 4;
    config->maxs[0] = *(const float *)cursor; cursor += 4;
    config->maxs[1] = *(const float *)cursor; cursor += 4;
    config->maxs[2] = *(const float *)cursor; cursor += 4;

    for ( lodIndex = 0; lodIndex < XMODEL_LOD_COUNT; ++lodIndex )
    {
        config->lods[lodIndex].distance = *(const float *)cursor;
        cursor += 4;
        strcpy(config->lods[lodIndex].name, (const char *)cursor);
        cursor += strlen((const char *)cursor) + 1;
    }

    config->modelFileCount = *(const int *)cursor;
    cursor += 4;
    return cursor;
}

/* ---- XModelLoadCollData  0x0048B0E0 ----  VERIFIED */
static const unsigned char *XModelLoadCollData(const unsigned char *cursor,
                                               XModelInfo *info,
                                               XModelAllocFn alloc)
{
    int surfaceCount;
    int surfaceIndex;

    surfaceCount = *(const int *)cursor;
    cursor += 4;
    info->collisionSurfaceCount = surfaceCount;
    info->contents = 0;

    if ( !surfaceCount )
    {
        info->collisionSurfaces = 0;
        return cursor;
    }

    info->collisionSurfaces = (XModelCollSurf *)alloc(44 * surfaceCount);

    for ( surfaceIndex = 0; surfaceIndex < info->collisionSurfaceCount; ++surfaceIndex )
    {
        XModelCollSurf *surf = &info->collisionSurfaces[surfaceIndex];
        int triCount;
        int triIndex;

        triCount = *(const int *)cursor;
        cursor += 4;
        surf->numCollTris = triCount;
        surf->collTris = (XModelCollTri *)alloc(48 * surf->numCollTris);

        for ( triIndex = 0; triIndex < surf->numCollTris; ++triIndex )
        {
            float *dst = (float *)&surf->collTris[triIndex];
            dst[0]=*(const float *)cursor;cursor+=4;
            dst[1]=*(const float *)cursor;cursor+=4;
            dst[2]=*(const float *)cursor;cursor+=4;
            dst[3]=*(const float *)cursor;cursor+=4;
            dst[4]=*(const float *)cursor;cursor+=4;
            dst[5]=*(const float *)cursor;cursor+=4;
            dst[6]=*(const float *)cursor;cursor+=4;
            dst[7]=*(const float *)cursor;cursor+=4;
            dst[8]=*(const float *)cursor;cursor+=4;
            dst[9]=*(const float *)cursor;cursor+=4;
            dst[10]=*(const float *)cursor;cursor+=4;
            dst[11]=*(const float *)cursor;cursor+=4;

        }

        surf->expandedMins[0]=*(const float *)cursor - 0.001f;cursor+=4;
        surf->expandedMins[1]=*(const float *)cursor - 0.001f;cursor+=4;
        surf->expandedMins[2]=*(const float *)cursor - 0.001f;cursor+=4;
        surf->expandedMaxs[0]=*(const float *)cursor + 0.001f;cursor+=4;
        surf->expandedMaxs[1]=*(const float *)cursor + 0.001f;cursor+=4;
        surf->expandedMaxs[2]=*(const float *)cursor + 0.001f;cursor+=4;
        /* retail's register roles (contents in ECX, then copied to EAX for the
         * OR) come from re-reading surf->contents; the three ints are read off
         * one cursor bump in this statement order */
        surf->contents=*(const int *)(cursor + 4) & 0xDFFF7FFB;
        surf->surfaceFlags=*(const int *)(cursor + 8);
        info->contents |= surf->contents;
        surf->basePoseIndex=*(const int *)(cursor + 0);
        cursor+=12;
    }

    return cursor;
}

/* ---- XModelExists  0x0048B270 ----  VERIFIED */
int __cdecl XModelExists(const char *name)
{
    return FS_GetDataForFile("xmodel", name, "") != 0;
}

/* ---- XModelPrecache  0x0048B290 ----  VERIFIED */
XModel *__cdecl XModelPrecache(const char *name, int loadSurfaces,
                               XModelAllocFn alloc, XModelAllocFn allocMesh)
{
    XModel *entry;
    XModelInfo *info;
    void *fileBuffer;
    const unsigned char *cursor;
    char *namePool;
    int nameBytes;
    int nameLengths[XMODEL_LOD_COUNT];
    int lodIndex;
    int i;
    XModelConfig config;
    char path[1024];

    entry = (XModel *)FS_GetDataForFile("xmodel", name, "");
    if ( !entry )
    {
        if ( xmodel_enforceExist )
        {
            Com_Error(ERR_DROP,
                      "\x15" "Cannot precache 'xmodel/%s'.\n"
                      "you may need to get latest and run converter to fix",
                      name);
            return 0;
        }
        Com_Printf("ERROR: Cannot precache 'xmodel/%s'", name);
        return XModelCreateDefault_();
    }

    info = entry->info;
    if ( !info )
    {
        sprintf(path, "xmodel/%s", name);
        if ( FS_ReadFile(path, &fileBuffer) < 0 )
        {
            Com_Error(ERR_DROP, "\x15" "Cannot find 'xmodel/%s'.", name);
            return 0;
        }

        cursor = XModelLoadConfigFile(name, (const unsigned char *)fileBuffer, &config);
        if ( !cursor )
        {
            FS_FreeFile(fileBuffer);
            return 0;
        }

        nameBytes = 0;
        for ( lodIndex = 0; lodIndex < XMODEL_LOD_COUNT; ++lodIndex )
        {
            nameLengths[lodIndex] = (int)strlen(config.lods[lodIndex].name) + 1;
            nameBytes += nameLengths[lodIndex];
        }

        info = (XModelInfo *)alloc(nameBytes + 104);
        cursor = XModelLoadCollData(cursor, info, alloc);

        info->lodCount = 0;
        namePool = (char *)info + 104;
        for ( lodIndex = 0; lodIndex < XMODEL_LOD_COUNT; ++lodIndex )
        {
            strcpy(namePool, config.lods[lodIndex].name);
            info->lodRecords[lodIndex].name = namePool;

            if ( *namePool )
            {
                ++info->lodCount;
                info->lodRecords[lodIndex].surfaceCount = *(const short *)cursor;
                cursor += 2;
                info->lodRecords[lodIndex].surfaceNameTable =
                    (unsigned short *)alloc(2 * info->lodRecords[lodIndex].surfaceCount);
                for ( i = 0; i < info->lodRecords[lodIndex].surfaceCount; ++i )
                {
                    const char *surfName = (const char *)cursor;

                    cursor += strlen(surfName) + 1;
                    /* type 8 -- `push 8` at 0x0048B473. */
                    info->lodRecords[lodIndex].surfaceNameTable[i] = (unsigned short)
                        SL_GetStringOfLen(surfName, 0, strlen(surfName) + 1, 8);
                }
            }
            else
            {
                info->lodRecords[lodIndex].surfaceNameTable = 0;
            }

            info->lodRecords[lodIndex].distance = config.lods[lodIndex].distance;
            namePool += nameLengths[lodIndex];
        }

        FS_FreeFile(fileBuffer);

        info->mins[0] = config.mins[0];
        info->mins[1] = config.mins[1];
        info->mins[2] = config.mins[2];
        info->maxs[0] = config.maxs[0];
        info->maxs[1] = config.maxs[1];
        info->maxs[2] = config.maxs[2];
        info->modelFileCount = (short)config.modelFileCount;

        entry->info = info;
        entry->freeData = XModelFree;
        info->lodRecords[0].surfs = 0;
        info->lodRecords[1].surfs = 0;
        info->lodRecords[2].surfs = 0;
    }
    else if ( info == &xmodel_defaultCollision )
    {
        return entry;
    }

    info->parts = XModelPartsPrecache(info->lodRecords[0].name, alloc);
    if ( info->parts )
    {
        if ( loadSurfaces )
        {
            for ( lodIndex = 0; lodIndex < XMODEL_LOD_COUNT; ++lodIndex )
            {
                XModelLodInfo *lod = &info->lodRecords[lodIndex];
                XModelSurfs *surfs;

                if ( !*lod->name )
                    break;
                surfs = XModelSurfsPrecache(lod->name, alloc);
                lod->surfs = surfs;
                if ( !surfs )
                {
                    XModelFree(entry);
                    XModelCreateDefault(entry);
                    return entry;
                }
#ifndef DEDICATED
                if ( xmodel_animCheck && loadSurfaces == 2 )
                    XModelOptimize( (struct XModelSurfsData_s *)surfs->surfs, allocMesh );
#endif
            }
        }
    }
    else
    {
        XModelFree(entry);
        XModelCreateDefault(entry);
    }

    return entry;
}

/* ---- XModelClearData  0x0048B600 ----  VERIFIED */
void __cdecl XModelClearData(unsigned int rangeEnd, unsigned int rangeStart)
{
    XModelSurfsData **link = &xmodel_surfsCloneList;
    XModelSurfsData *surfs = xmodel_surfsCloneList;

    while ( surfs )
    {
        if ( (unsigned int)surfs < rangeStart || (unsigned int)surfs >= rangeEnd )
        {
            int i;

            for ( i = 0; i < surfs->surfaceCount; ++i )
            {
                XSurface *surface = surfs->surfaces[i];

                if ( surface )
                {
                    unsigned int nv = (unsigned int)surface->optimizedDataNV;
                    unsigned int ati;

                    if ( nv >= rangeStart && nv < rangeEnd )
                        surface->optimizedDataNV = 0;
                    ati = (unsigned int)surface->optimizedDataATI;
                    if ( ati >= rangeStart && ati < rangeEnd )
                        surface->optimizedDataATI = 0;
                }
            }
            link = &surfs->next;
        }
        else
        {
            *link = surfs->next;
        }
        surfs = surfs->next;
    }
}

/* ---- XModelNumBones  0x0048B670 ----  VERIFIED */
int __cdecl XModelNumBones(const XModel *model)
{
    return model->info->parts->data->partNameTableSlot->partNameTable->count;
}

/* ---- XModelBoneNames  0x0048B680 ----  VERIFIED */
unsigned short *__cdecl XModelBoneNames(const XModel *model)
{
    return model->info->parts->data->partNameTableSlot->partNameTable->handles;
}

/* ---- XModelGetBoneIndex  0x0048B690 ----  VERIFIED */
int __cdecl XModelGetBoneIndex(const XModel *model, unsigned short name)
{
    const XModelPartNameTable *table =
        model->info->parts->data->partNameTableSlot->partNameTable;
    const unsigned short *handles = table->handles;
    int partIndex;

    for ( partIndex = table->count - 1; partIndex >= 0; --partIndex )
    {
        if ( name == handles[partIndex] )
            break;
    }
    return partIndex;
}

/* ---- XModelGetBounds  0x0048B6B0 ----  VERIFIED
 * No retail caller: inlined into DObjGetBounds.  The mins/maxs pointers are
 * separate locals so the inlined copy in SV_LinkEntity allocates as retail. */
void __cdecl XModelGetBounds(const XModel *model, float *mins, float *maxs)
{
    const float *mn = model->info->mins;
    const float *mx = model->info->maxs;

    mins[0] = mn[0];
    mins[1] = mn[1];
    mins[2] = mn[2];
    maxs[0] = mx[0];
    maxs[1] = mx[1];
    maxs[2] = mx[2];
}

/* ---- XModelGetSurfaces  0x0048B6E0 ----  VERIFIED */
int __cdecl XModelGetSurfaces(int lodIndex, const XModel *model,
                              XSurface ***surfaces)
{
    const XModelSurfsData *surfs = model->info->lodRecords[lodIndex].surfs->surfs;

    *surfaces = surfs->surfaces;
    return model->info->lodRecords[lodIndex].surfs->surfs->surfaceCount;
}

/* ---- XModelGetSurfaceName  0x0048B710 ----  VERIFIED */
const char *__cdecl XModelGetSurfaceName(int lodIndex, const XModel *model,
                                         int surfaceIndex)
{
    unsigned short handle =
        model->info->lodRecords[lodIndex].surfaceNameTable[surfaceIndex];

    if ( handle )
        return SL_REFSTRING(handle) + 4;
    return "DEFAULT";
}

/* ---- XSurfaceCloneSurface  0x0048B740 ----  VERIFIED */
XSurface *__cdecl XSurfaceCloneSurface(const XSurface *surface,
                                       XModelAllocFn alloc)
{
    XSurface *clone = (XSurface *)alloc(52);

    memcpy(clone, surface, 0x34);
    clone->optimizedDataATI = 0;
    clone->optimizedDataNV = 0;

    clone->texCoords = (float (*)[2])alloc(8 * clone->vertexCount);
    Com_Memcpy(clone->texCoords, surface->texCoords, 8 * clone->vertexCount);
    return clone;
}

/* ---- XSurfaceGetNumVerts  0x0048B790 ----  VERIFIED */
int __cdecl XSurfaceGetNumVerts(const XSurface *surface)
{
    return surface->vertexCount;
}

/* ---- XSurfaceGetNumTris  0x0048B7A0 ----  VERIFIED */
int __cdecl XSurfaceGetNumTris(const XSurface *surface)
{
    return surface->triangleCount;
}

/* ---- XSurfaceTileMode  0x0048B7B0 ----  VERIFIED */
int __cdecl XSurfaceTileMode(const XSurface *surface)
{
    return surface->tileMode;
}

/* ---- XSurfaceGetTris  0x0048B7C0 ---- */
void __cdecl XSurfaceGetTris( unsigned int out, int surfacePtr,
                              unsigned short indexOffset )
{
  XSurface *surface = (XSurface *)surfacePtr;

  if ( indexOffset )
  {
    /* Same offset added to both packed 16-bit indices of a dword at once. */
    unsigned int        addBoth = indexOffset | ( indexOffset << 16 );
    const unsigned int *src = (const unsigned int *)surface->triangles;
    unsigned int       *dst = (unsigned int *)out;
    int                 pairs = surface->triangleCount >> 1; /* 2 tris == 3 dwords */

    do
    {
      *dst++ = *src++ + addBoth;
      *dst++ = *src++ + addBoth;
      *dst++ = *src++ + addBoth;
      --pairs;
    }
    while ( pairs );
  }
  else
  {
    memcpy( (void *)out, surface->triangles, 6 * surface->triangleCount );
  }
}

/* ---- XSurfaceGetBlendInfoArray  0x0048B830 ----  VERIFIED */
void *__cdecl XSurfaceGetBlendInfoArray(const XSurface *surface)
{
    return surface->vertexData;
}

/* ---- XSurfaceGetTexCoordArray  0x0048B840 ----  VERIFIED */
float *__cdecl XSurfaceGetTexCoordArray(const XSurface *surface)
{
    return (float *)surface->texCoords;
}

/* ---- XSurfaceGetVertexInfoArray  0x0048B850 ----  VERIFIED */
XSurfaceWeightedPoint *__cdecl XSurfaceGetVertexInfoArray(const XSurface *surface)
{
    return surface->weightedPoints;
}

/* ---- XSurfaceGetBoneIndex  0x0048B860 ----  VERIFIED */
int __cdecl XSurfaceGetBoneIndex(const XSurface *surface)
{
    return surface->boneMatrixOffset;
}

/* ---- XSurfaceRemapTextureCoordinates  0x0048B870 ---- */
void __cdecl XSurfaceRemapTextureCoordinates(XSurface *surface,
                                             const float *scale,
                                             const float *offset,
                                             int sourceUIndex, int sourceVIndex)
{
    float *texCoords = (float *)surface->texCoords;
    int i;

    for ( i = surface->vertexCount; i; --i )
    {
        float sourceU = texCoords[sourceUIndex];
        float sourceV = texCoords[sourceVIndex];

        texCoords[0] = sourceU * scale[0] + offset[0];
        texCoords[1] = sourceV * scale[1] + offset[1];
        texCoords += 2;
    }
}

/* ---- XModelGetName  0x0048B8A0 ----  VERIFIED */
const char *__cdecl XModelGetName(const XModel *model)
{
    return model->name;
}

/* ---- XModelGetContents  0x0048B8B0 ----  VERIFIED */
int __cdecl XModelGetContents(const XModel *model)
{
    return model->info->contents;
}

/* ---- XSurfaceTransformPoint43  0x0048B8C0 ----  VERIFIED */
/* (no callers) VC7 orders these commutative float terms by parameter order. */
void __cdecl XSurfaceTransformPoint43(const float *point,
                                      const DObjSkelMat *matrix, float *out)
{
    out[0] = matrix->axis[2][0] * point[2] + matrix->axis[1][0] * point[1]
           + point[0] * matrix->axis[0][0] + matrix->origin[0];
    out[1] = matrix->axis[0][1] * point[0] + matrix->axis[2][1] * point[2]
           + matrix->axis[1][1] * point[1] + matrix->origin[1];
    out[2] = matrix->axis[0][2] * point[0] + matrix->axis[2][2] * point[2]
           + matrix->axis[1][2] * point[1] + matrix->origin[2];
}

/* ---- XSurfaceAccumulateWeightedPoint43  0x0048B910 ----  VERIFIED */
void __cdecl XSurfaceAccumulateWeightedPoint43(const float *point, float *out,
                                               const DObjSkelMat *matrix,
                                               float weight)
{
    out[0] += (( matrix->axis[1][0] * point[1] + matrix->axis[2][0] * point[2] )
             + point[0] * matrix->axis[0][0] + matrix->origin[0]) * weight;
    out[1] += (matrix->axis[1][1] * point[1] + matrix->axis[2][1] * point[2]
             + matrix->axis[0][1] * point[0] + matrix->origin[1]) * weight;
    out[2] += (matrix->axis[1][2] * point[1] + matrix->axis[2][2] * point[2]
             + matrix->axis[0][2] * point[0] + matrix->origin[2]) * weight;
}

/* ---- XSurfaceTransformNormal43  0x0048B980 ----  VERIFIED */
void __cdecl XSurfaceTransformNormal43(const float *normal,
                                       const DObjSkelMat *matrix, float *out)
{
    out[0] = matrix->axis[2][0] * normal[2] + matrix->axis[1][0] * normal[1]
           + normal[0] * matrix->axis[0][0];
    out[1] = matrix->axis[0][1] * normal[0] + matrix->axis[2][1] * normal[2]
           + matrix->axis[1][1] * normal[1];
    out[2] = matrix->axis[0][2] * normal[0] + matrix->axis[2][2] * normal[2]
           + matrix->axis[1][2] * normal[1];
}

/* ---- XSurfaceTransformVectorRows43  0x0048B9D0 ----  VERIFIED */
void __cdecl XSurfaceTransformVectorRows43(const float *vector,
                                           const DObjSkelMat *matrix, float *out)
{
    out[0] = matrix->axis[0][1] * vector[1] + matrix->axis[0][2] * vector[2]
           + vector[0] * matrix->axis[0][0];
    out[1] = matrix->axis[1][0] * vector[0] + matrix->axis[1][1] * vector[1]
           + matrix->axis[1][2] * vector[2];
    out[2] = matrix->axis[2][0] * vector[0] + matrix->axis[2][1] * vector[1]
           + matrix->axis[2][2] * vector[2];
}

/*
 * ---- XSurfaceGetVerts  0x0048BA20 ----
 * Software-skins one XSurface into caller buffers: texCoordsOut (optional raw
 * copy), positionsOut (xyz positions) and normalsOut (optional xyz normals),
 * using the DObjSkelMat[] at matrixBase (boneMatrixOffset is a byte offset).
 * Rigid surfaces use one bone matrix; blended surfaces (boneMatrixOffset == -1)
 * transform by each vertex's primary bone and accumulate additive weights.
 */
void __cdecl XSurfaceGetVerts( const XSurface *surface, const char *matrixBase, float *positionsOut,
                               float *texCoordsOut, float *normalsOut )
{
    const char *vertexData;
    int         boneMatrixOffset;
    int         vertIndex;

    vertexData       = (const char *)XSurfaceGetBlendInfoArray( surface );
    boneMatrixOffset = XSurfaceGetBoneIndex( surface );
    if ( texCoordsOut )
        memcpy( texCoordsOut, XSurfaceGetTexCoordArray( surface ), 8 * XSurfaceGetNumVerts( surface ) );

    if ( boneMatrixOffset != -1 )
    {
        const DObjSkelMat       *mat  = (const DObjSkelMat *)( matrixBase + boneMatrixOffset );

        for ( vertIndex = XSurfaceGetNumVerts( surface ); vertIndex; --vertIndex )
        {
            if ( normalsOut )
            {
                XSurfaceTransformNormal43( ((const XSurfaceRigidVert *)vertexData)->normal, mat, normalsOut );
                normalsOut += 3;
            }
            XSurfaceTransformPoint43( ((const XSurfaceRigidVert *)vertexData)->position, mat, positionsOut );
            vertexData += sizeof( XSurfaceRigidVert );
            positionsOut += 3;
        }
    }
    else
    {
        const XSurfaceWeightedPoint *weighted = XSurfaceGetVertexInfoArray( surface );

        for ( vertIndex = XSurfaceGetNumVerts( surface ); vertIndex; --vertIndex )
        {
            const XSurfaceBlendVert *vert = (const XSurfaceBlendVert *)vertexData;
            int                      additiveCount;

            if ( normalsOut )
            {
                {
                const DObjSkelMat *nm = (const DObjSkelMat *)( matrixBase + vert->blend.boneMatrixOffset );
                XSurfaceTransformNormal43( vert->normal, nm, normalsOut );
                }
                normalsOut += 3;
            }
            {
            const DObjSkelMat *pm = (const DObjSkelMat *)( matrixBase + vert->blend.boneMatrixOffset );
            const float *pp = vert->blend.position;
            /* VC7 orders the x87 operands of each product itself; the volatile read of
             * pp[0], the nm/out pointer copies and am->axis[0][0] below are what put
             * every pair in retail's order (0x48BB8C..0x48BC5D) */
            positionsOut[0] = ( pm->axis[1][0] * pp[1] + pm->axis[2][0] * pp[2] ) + *(volatile const float *)&pp[0] * pm->axis[0][0] + pm->origin[0];
            positionsOut[1] = ( pm->axis[0][1] * pp[0] + pm->axis[1][1] * pp[1] ) + pm->axis[2][1] * pp[2] + pm->origin[1];
            positionsOut[2] = ( pm->axis[0][2] * pp[0] + pm->axis[1][2] * pp[1] ) + pm->axis[2][2] * pp[2] + pm->origin[2];
            }
            additiveCount = vert->additiveWeightCount;
            if ( !additiveCount )
            {
                vertexData += sizeof( XSurfaceBlendVert ) - sizeof( float );
            }
            else
            {
                {
                float *out = positionsOut;
                out[0] *= vert->primaryWeight;
                out[1] *= vert->primaryWeight;
                out[2] *= vert->primaryWeight;
                }
                do
                {
                    {
                    const DObjSkelMat *am = (const DObjSkelMat *)( matrixBase + weighted->blend.boneMatrixOffset );
                    const float *ap = weighted->blend.position;
                    float aw = weighted->weight;
                    positionsOut[0] += ( ( am->axis[1][0] * ap[1] + am->axis[2][0] * ap[2] ) + ap[0] * am->axis[0][0] + am->origin[0] ) * aw;
                    positionsOut[1] += ( ( am->axis[0][1] * ap[0] + am->axis[2][1] * ap[2] ) + am->axis[1][1] * ap[1] + am->origin[1] ) * aw;
                    positionsOut[2] += ( ( am->axis[0][2] * ap[0] + am->axis[2][2] * ap[2] ) + am->axis[1][2] * ap[1] + am->origin[2] ) * aw;
                    }
                    --additiveCount;
                    ++weighted;
                }
                while ( additiveCount );
                vertexData += sizeof( XSurfaceBlendVert );
            }
            positionsOut += 3;
        }
    }
}

/* ---- XModelGetNumLods  0x0048BCD0 ----  VERIFIED */
int __cdecl XModelGetNumLods(const XModel *model)
{
    return model->info->lodCount;
}

/* ---- XModelGetModelFileCount  0x0048BCE0 ----  VERIFIED */
int __cdecl XModelGetModelFileCount(const XModel *model)
{
    return model->info->modelFileCount;
}

/* ---- XModelSetTestLods  0x0048BCF0 ----  VERIFIED */
void __cdecl XModelSetTestLods(int lodIndex, float distance)
{
    if ( lodIndex == 0 )
        xmodel_testLodsEnabled = (unsigned char)(distance >= 0.0f);
    if ( distance < 0.0f )
        distance = 0.0f;
    xmodel_testLodDistances[lodIndex].distance = distance;
}

/* ---- XModelSetTestLodDist  0x0048BD40 ----  VERIFIED */
void __cdecl XModelSetTestLodDist(float distance)
{
    if ( distance > 0.0f )
        xmodel_testLodDist = distance;
    else
        xmodel_testLodDist = 0.0f;
}

/* ---- XModelGetLodForDist  0x0048BD70 ----  VERIFIED */
int __cdecl XModelGetLodForDist(const XModel *model, float distance)
{
    const XModelLodInfo *lod;
    int lodCount;
    int lodIndex;

    if ( !xmodel_testLodsEnabled )
        lod = model->info->lodRecords;
    else
        lod = xmodel_testLodDistances;

    if ( xmodel_testLodDist != 0.0f )
        distance = xmodel_testLodDist;

    lodCount = model->info->lodCount;
    for ( lodIndex = 0; lodIndex < lodCount; ++lodIndex, ++lod )
    {
        if ( lod->distance == 0.0f )
            return lodIndex;
        if ( distance < lod->distance )
            return lodIndex;
    }
    return -1;
}

/* ---- XModelGetBasePose  0x0048BDE0 ----  VERIFIED */
void __cdecl XModelGetBasePose(XModel *model, DObjSkelMat *basePose)
{
    extern void QuatMultiply(const float *,const float *,float *);
    const XModelPartsData *parts = model->info->parts->data;
    const XModelPartNameTableSlot *slot = parts->partNameTableSlot;
    const unsigned char *deltas = slot->parentPartDeltas;
    const short (*rotation)[4] = parts->baseRotations;
    const float (*translation)[3] = parts->baseTranslations;
    int totalPartCount = slot->partNameTable->count;
    float *row = (float *)basePose;
    int i;
    float q[4];

    for ( i = parts->rootPartCount; i; --i )
    {
        row[0] = 0.0f;
        row[1] = 0.0f;
        row[2] = 0.0f;
        row[3] = 1.0f;
        row += 16;
    }

    for ( i = totalPartCount - parts->rootPartCount; i; --i )
    {
        q[0] = (float)(*rotation)[0] * 0.000030518509f;
        q[1] = (float)(*rotation)[1] * 0.000030518509f;
        q[2] = (float)(*rotation)[2] * 0.000030518509f;
        q[3] = (float)(*rotation)[3] * 0.000030518509f;
        QuatMultiply( q, &row[-16 * *deltas], row );
        ++rotation;
        row += 16;
        ++deltas;
    }

    for ( i = parts->rootPartCount; i; --i )
    {
        basePose->axis[0][0] = 1.0f; basePose->axis[0][1] = 0.0f; basePose->axis[0][2] = 0.0f;
        basePose->axis[1][0] = 0.0f; basePose->axis[1][1] = 1.0f; basePose->axis[1][2] = 0.0f;
        basePose->axis[2][0] = 0.0f; basePose->axis[2][1] = 0.0f; basePose->axis[2][2] = 1.0f;
        basePose->origin[2] = 0.0f;
        basePose->origin[1] = 0.0f;
        basePose->origin[0] = 0.0f;
        ++basePose;
    }

    deltas = slot->parentPartDeltas;
    for ( i = totalPartCount - parts->rootPartCount; i; --i )
    {
        const DObjSkelMat *parent;

        XModelExpandQuatToAxis( (float *)basePose );
        parent = basePose - *deltas;

        row = basePose->origin;
        row[0] = parent->axis[2][0] * (*translation)[2] + parent->axis[1][0] * (*translation)[1] + (*translation)[0] * parent->axis[0][0] + parent->origin[0];
        row[1] = ( parent->axis[2][1] * (*translation)[2] + parent->axis[1][1] * (*translation)[1] ) + parent->axis[0][1] * (*translation)[0] + parent->origin[1];
        row[2] = ( parent->axis[2][2] * (*translation)[2] + parent->axis[1][2] * (*translation)[1] ) + parent->axis[0][2] * (*translation)[0] + parent->origin[2];
        ++basePose;
        ++translation;
        ++deltas;
    }
}

extern qboolean CM_TraceBox( const float *start, const float *end,
                             const float *mins, const float *maxs,
                             float fraction );

/* ---- XModelTraceLine  0x0048C070 ----  VERIFIED */
int __cdecl XModelTraceLine(
        int          model,        /* @<eax>  XModel *                        */
        trace_t     *work,         /* @<esi>  48-byte trace record; a pointer,
                                    * not an int: an int parameter makes LTCG treat
                                    * CM_TraceStaticModel's &work as escaped, which
                                    * blocks retail's load hoisting at 0x0041FE09 */
        const float *basePose,
        const float *start,
        const float *end,
        int          contentmask)
{
  unsigned char *collLod;
  unsigned char *surf;
  const float   *tris;
  const float   *tri;
  const float   *m;
  int            surfIndex;
  int            numTris, triIndex;
  int            boneIndex, hitBone;
  float          localStart[3];
  float          localEnd[3];
  float          delta[3];
  float          mid[3];
  /* Values retail keeps on the x87 stack (_PC_53) are floats; the floats are
   * exactly the values retail spills with fst/fstp. */
  float         dx, dy, dz;
  float         dEndX, dStartX, denom, fracX, t, mid2X, uX, vX;
  float          dStart, dEnd, frac, u;
  float          fraction;
  float          normal[3];

  /* delta lives in its stack home: retail's final normal store order
   * ([esi+18h] via EDX, then fstp [esi+10h]/[esi+14h]) */
  if ( 0 ) *(void **)0 = (void *)&(delta);
  hitBone  = -1;
  collLod  = *(unsigned char **)(model + 4);
  for ( surfIndex = 0; surfIndex < *(int *)(collLod+0x44); surfIndex++ )
  {
    surf = *(unsigned char **)(collLod + 0x40) + 44 * surfIndex;

    if ( ( *(int *)(surf + 0x24) & contentmask ) == 0 )
      continue;

    fraction  = *(float *)work;
    boneIndex = *(int *)(surf + 0x20);
    m         = basePose + ( boneIndex << 4 );

    /* 0x48C0CB-0x48C0DF: never stored, used straight from ST. */
    dx = (float)start[0] - m[12];
    dy = (float)start[1] - m[13];
    dz = (float)start[2] - m[14];
    localStart[0] = dx * m[0] + dz * m[2]  + dy * m[1];
    localStart[1] = dx * m[4] + ( dy * m[5] + dz * m[6] );
    localStart[2] = dx * m[8] + dz * m[10] + dy * m[9];

    /* 0x48C124-0x48C132: likewise kept in registers. */
    dx = (float)end[0] - m[12];
    dy = (float)end[1] - m[13];
    dz = (float)end[2] - m[14];
    localEnd[0] = ( dx * m[0] ) + dz * m[2] + dy * m[1];
    localEnd[1] = dz * m[6] + dy * m[5]  + dx * m[4];
    localEnd[2] = dx * m[8] + dz * m[10] + dy * m[9];

    delta[0] = localEnd[0] - localStart[0];
    delta[1] = localEnd[1] - localStart[1];
    delta[2] = localEnd[2] - localStart[2];

    if ( CM_TraceBox( localStart, localEnd,
                      (const float *)( surf + 0x08 ),
                      (const float *)( surf + 0x14 ),
                      fraction ) != qfalse )
      continue;

    numTris = *(int *)(surf + 4);
    tris    = *(const float **)surf;

    for(triIndex=0;triIndex<*(int *)(surf+4);++triIndex)
    {
      tri = *(const float **)surf + 12 * triIndex;

      dEndX = ( (float)localEnd[0] * tri[0] ) + (float)localEnd[2] * tri[2]
              + (float)localEnd[1] * tri[1] - tri[3];
      if ( dEndX >= 0.0f )                 /* 0x48C1F4 compares unrounded */
        continue;
      dEnd = (float)dEndX;                /* 0x48C1F0 fst */

      dStartX = ( (float)localStart[0] * tri[0] ) + (float)localStart[2] * tri[2]
                + (float)localStart[1] * tri[1] - tri[3];
      if ( dStartX <= 0.0f )               /* 0x48C224 */
        continue;
      dStart = (float)dStartX;            /* 0x48C220 fst */

      fracX = ( dStart - 0.125f ) / ( dStart - dEnd );
      if ( fracX >= *(float *)work )      /* 0x48C24D compares unrounded */
        continue;
      frac = (float)fracX;                /* 0x48C249 fst; the value stored to work */

      t      = dStart / ( dStart - dEnd );
      mid[0] = (float)( delta[0] * t + localStart[0] );
      mid[1] = (float)( delta[1] * t + localStart[1] );
      mid2X  = t * delta[2] + localStart[2];
      mid[2] = (float)mid2X;              /* 0x48C282 fst; the unrounded ST0 feeds u */

      uX = mid[2] * tri[6] + (float)mid[1] * tri[5] + (float)mid[0] * tri[4]
           - tri[7];
      if ( uX < -0.001f )                 /* 0x48C2A2 compares unrounded */
        continue;
      u = (float)uX;                      /* 0x48C29E fst */
      if ( u > 1.001f )                   /* 0x48C2B3 reloads the float u */
        continue;

      /* never stored; uses the float mid[2] */
      vX = (float)mid[2] * tri[10] + (float)mid[1] * tri[9]
           + (float)mid[0] * tri[8] - tri[11];
      if ( vX < -0.001f || u + vX > 1.001f )  /* 0x48C2E0 / 0x48C2ED: u_f + v_unrounded */
        continue;

      *(unsigned char *)( (unsigned char *)work + 47 ) = 0;
      *(unsigned char *)( (unsigned char *)work + 46 ) = 0;
      *(float *)work                  = frac;
      *(int *)( (unsigned char *)work + 28 )           = *(int *)( surf + 0x28 );
      *(int *)( (unsigned char *)work + 32 )           = *(int *)( surf + 0x24 );
      hitBone                         = boneIndex;
      *(float *)( (unsigned char *)work + 16 )         = tri[0];
      *(float *)( (unsigned char *)work + 20 )         = tri[1];
      *(float *)( (unsigned char *)work + 24 )         = tri[2];
    }
  }

  if ( hitBone < 0 )
    return -1;

  m  = basePose + ( hitBone << 4 );
  normal[0] = m[8]  * *(float *)( (unsigned char *)work + 24 ) + m[4] * *(float *)( (unsigned char *)work + 20 )
       + m[0] * *(float *)( (unsigned char *)work + 16 );
  normal[1] = m[9]  * *(float *)( (unsigned char *)work + 24 ) + m[5] * *(float *)( (unsigned char *)work + 20 )
       + m[1] * *(float *)( (unsigned char *)work + 16 );
  normal[2] = m[10] * *(float *)( (unsigned char *)work + 24 ) + m[6] * *(float *)( (unsigned char *)work + 20 )
       + m[2] * *(float *)( (unsigned char *)work + 16 );
  VectorCopy( normal, ((float *)( (unsigned char *)work + 16 )) );

  return hitBone;
}

float *MatrixTransformVector( const float *in, float *m, float *out );   /* universal/com_math.c */

/* ---- XModelGetStaticBounds  0x0048C3E0 ----  VERIFIED */
int __cdecl XModelGetStaticBounds( int model, const float *axis, float *mins,
                                   float *maxs )
{
  const XModelInfo *info = ( (const XModel *)model )->info;
  int   surfIndex;
  int   i;
  float corner[3];
  float transformed[3];

  if ( !info->collisionSurfaceCount )
    return 0;

  mins[0] = 3.4028235e38f;
  mins[1] = 3.4028235e38f;
  mins[2] = 3.4028235e38f;
  maxs[0] = -3.4028235e38f;
  maxs[1] = -3.4028235e38f;
  maxs[2] = -3.4028235e38f;

  for ( surfIndex = 0; surfIndex < info->collisionSurfaceCount; ++surfIndex )
  {
    const XModelCollSurf *surf = &info->collisionSurfaces[surfIndex];

    /* Rotate all 8 corners of the surface's expanded AABB and grow bounds. */
    for ( i = 0; i < 8; ++i )
    {
      corner[0] = ( i & 1 ) ? surf->expandedMins[0] : surf->expandedMaxs[0];
      corner[1] = ( i & 2 ) ? surf->expandedMins[1] : surf->expandedMaxs[1];
      corner[2] = ( i & 4 ) ? surf->expandedMins[2] : surf->expandedMaxs[2];
      MatrixTransformVector( corner, (float *)axis, transformed );

      if ( mins[0] > transformed[0] )
        mins[0] = transformed[0];
      if ( maxs[0] < transformed[0] )
        maxs[0] = transformed[0];
      if ( mins[1] > transformed[1] )
        mins[1] = transformed[1];
      if ( maxs[1] < transformed[1] )
        maxs[1] = transformed[1];
      if ( mins[2] > transformed[2] )
        mins[2] = transformed[2];
      if ( maxs[2] < transformed[2] )
        maxs[2] = transformed[2];
    }
  }
  return 1;
}

#ifdef __cplusplus
}
#endif
