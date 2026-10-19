/*
 * Machine-translated from Call of Duty 1.1 (Windows, CoDMP.exe).
 * Original translation unit:
 * /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/xanim/dobj.cpp
 * Retail range 0x00480DA0-0x00483FC0, 53 functions.
 * @fidelity: likely
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"


#include "../script/scr_local.h"
#include "xanim_local.h"
/* ---- module-owned globals ---- */
int xanim_pendingFilesHandle;   /* 0x01407334 */
int xanim_poolUsedCount;   /* 0x00A7A610 */
int xanim_poolHighWaterCount;   /* 0x00A9CC68 */


typedef struct DObjSkelMat_s {
    float axis[3][4];   /* +0x00  the 4th float of each row is never written */
    float origin[3];    /* +0x30 */
    float pad3C;        /* +0x3C  never written by DObjQuatToMatrix43 */
} DObjSkelMat;

typedef struct DObjAnimMat_s {
    float quat[4];      /* +0x00 */
    float weight;       /* +0x10  accumulated contribution weight */
    float trans[3];     /* +0x14 */
} DObjAnimMat;

typedef struct XAnimEvalStorage_s {
    unsigned char evaluatedPartBits[16];  /* +0x00 */
    unsigned char controlPartBits[16];    /* +0x10 */
    unsigned char blockedPartBits[16];    /* +0x20 */
    DObjSkelMat   basePose[1];            /* +0x30  [partCount] */
} XAnimEvalStorage;

typedef struct XAnimEntry_s {
    unsigned short childCount;   /* +0x00  0 == leaf */
    unsigned short parentIndex;  /* +0x02 */
    union {
        void *leafAsset;         /* +0x04  fileData_t * when childCount == 0 */
        struct {
            unsigned short flags;            /* +0x04 */
            unsigned short firstChildIndex;  /* +0x06 */
        } parent;
    } u;
} XAnimEntry;

typedef struct XAnim_s {
    const char *name;        /* +0x00 */
    int         nodeCount;   /* +0x04 */
    XAnimEntry  entries[1];  /* +0x08  [nodeCount] */
} XAnim;

typedef struct XAnimTree_s {
    XAnim         *sourceTree;             /* +0x00 */
    int            partRemapTableSelector; /* +0x04  DObjCreate toggles 0<->1 */
    unsigned short poolNodeHandles[1];     /* +0x08  [nodeCount] */
} XAnimTree;

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
    unsigned short notifyChildIndex; /* +0x00 */
    short          notifyIndex;      /* +0x02  -1 == no pending notify */
    unsigned short notifyName;       /* +0x04  script string handle */
    unsigned short notifyType;       /* +0x06 */
    unsigned short freePrev;         /* +0x08 */
    unsigned short freeNext;         /* +0x0A  node 0's is the free-list head */
    XAnimState     states[2];        /* +0x0C and +0x28 */
} XAnimInfo;

typedef struct XAnimNotifyInfo_s {
    unsigned short name;      /* +0x00 */
    unsigned short pad02;
    float          frameFrac; /* +0x04 */
} XAnimNotifyInfo;

typedef struct XAnimParts_s {
    unsigned short   frameCountMinusOne;    /* +0x00 */
    unsigned char    looped;                /* +0x02 */
    unsigned char    hasDeltaMotion;        /* +0x03 */
    float            frameRate;             /* +0x04 */
    float            frequency;             /* +0x08 */
    unsigned short  *partNameHandles;       /* +0x0C */
    void            *partStreams;           /* +0x10  {trans,rot} stream pairs */
    XAnimNotifyInfo *notify;                /* +0x14 */
    void            *deltaMotion;           /* +0x18 */
    unsigned char   *compressedRotationBits;/* +0x1C */
} XAnimParts;

typedef struct DObjModel_s {
    void       *model;              /* +0x00  XModel * */
    const char *tagName;            /* +0x04  parent bone name, may be NULL/"" */
    short       negativeModelIndex; /* +0x08 */
    short       reserved0A;         /* +0x0A */
} DObjModel;

typedef struct DObj_s {
    XAnimTree     *tree;                      /* +0x00 */
    void          *evalStorage;               /* +0x04  XAnimEvalStorage * */
    int            skelCacheKey;              /* +0x08 */
    unsigned char *partRemapTable;            /* +0x0C  into the tree remap tail */
    unsigned short unknownState10;
    unsigned short scrNotifyId;
    unsigned short tracePartRemapHandle;      /* +0x14 */
    unsigned char  childCount;                /* +0x16 */
    unsigned char  partCount;                 /* +0x17  total bones, < 128 */
    unsigned int   childRefs[8];              /* +0x18  XModel * per child */
    short          childModelIndices[8];      /* +0x38 */
    unsigned char  childParentPartIndices[8]; /* +0x48  0xFF == unparented */
    unsigned char  childPartBaseIndices[8];   /* +0x50 */
} DObj;

typedef struct XModelPartNameTable_s {
    short          count;                    /* +0x00  SIGNED (movsx) */
    unsigned short handles[1];               /* +0x02  count entries */
} XModelPartNameTable;

typedef struct XModelPartNameTableSlot_s {
    XModelPartNameTable *partNameTable;      /* +0x00 */
    unsigned char        parentPartDeltas[1];/* +0x04  partCount-rootPartCount */
} XModelPartNameTableSlot;

typedef struct XModelPartColl_s {
    float mins[3];                           /* +0x00 */
    float maxs[3];                           /* +0x0C */
    float center[3];                         /* +0x18 */
    float radiusSq;                          /* +0x24 */
} XModelPartColl;

typedef struct XModelPartsData_s {
    XModelPartNameTableSlot *partNameTableSlot; /* +0x00 */
    short           rootPartCount;              /* +0x04  SIGNED */
    short           pad06;
    XModelPartColl *partCollisions;             /* +0x08 */
    short         (*baseRotations)[4];          /* +0x0C  packed quat */
    float         (*baseTranslations)[3];       /* +0x10 */
    unsigned char  *partStateIndices;           /* +0x14 */
} XModelPartsData;

typedef struct XModelPartsEntry_s {
    const char      *name;
    XModelPartsData *data;
    void           (*freeData)(struct XModelPartsEntry_s *);
} XModelPartsEntry;

typedef struct XModel_s {
    const char       *name;                  /* +0x00 */
    XModelPartsEntry **info;
    void            (*freeData)(struct XModel_s *);
} XModel;

/* ---- DObjModelParts  no-address ---- */
static XModelPartsData *DObjModelParts( const XModel *model )
{
    return (*model->info)->data;
}

/* ---- DObjModelPartNames  no-address ---- */
static XModelPartNameTable *DObjModelPartNames( const XModel *model )
{
    return DObjModelParts( model )->partNameTableSlot->partNameTable;
}

#define DOBJ_CHILD_MODEL(d,i)   ( (XModel *)(d)->childRefs[i] )

#define SL_REFSTRING(h)     ( (char *)(void *)GetRefString_var \
                              + 8 * (unsigned int)(unsigned short)(h) )
#define SL_STRING(h)        ( (h) ? SL_REFSTRING(h) + 4 : (char *)0 )

#define DOBJ_TRACE_REMAP_PREFIX     16
#define DOBJ_TRACE_REMAP_STRING_TYPE 12
#define XANIM_FILENAME_STRING_TYPE   7
#define DOBJ_CHILD_PARENT_NONE      0xFF

#define DOBJ_BIT_SET(bits,i)    ( ((unsigned char *)(bits))[(i) >> 3] \
                                  |= (unsigned char)(1 << ((i) & 7)) )
#define DOBJ_BIT_TEST(bits,i)   ( ( ((const unsigned char *)(bits))[(i) >> 3] \
                                    & (1 << ((i) & 7)) ) != 0 )

XAnimInfo xanim_pool[2048];

extern int AllocVariable__Fv();
extern int ClearObjectInternal__FUs();
extern void *FS_GetDataForFile( const char *dir, const char *name, const char *ext );
extern int FreeVariable__FUs();
extern unsigned short GetVariableIndexInternal( unsigned short parentId,
                                                unsigned int name );
extern int RemoveRefToObject__FUs();
extern int SL_FreeString__FUsPCcUi();
extern unsigned short SL_GetLowercaseStringOfLen( const char *text,
                                                  unsigned char user,
                                                  unsigned int size, int type );
extern unsigned short SL_GetStringOfLen( const char *text, unsigned char user,
                                         unsigned int size, int type );
extern const char *SL_ConvertToString( unsigned short handle );
extern void        SL_RemoveRefToStringOfLen( unsigned short handle,
                                              unsigned int size );
extern void        SL_RemoveRefToString( unsigned short handle );
extern void __cdecl XAnimClearTree( XAnimTree *tree );
extern int ReadQuat( const short *packed, short *out );
extern int ReadQuat2( const short *packed, short *out );
extern int __cdecl XModelGetLodForDist( const void *model, float distance );
extern int XModelTraceLine( int model, int work, const float *basePose,
                       const float *start, const float *end, int contentmask );

/* ---- XAnimInitSmallTreePool  0x00480DA0 ----  VERIFIED */
__int16 XAnimInitSmallTreePool()
{
  __int16 result;
  int nameBuf[5];

  Com_Memset(nameBuf, 0, 20);
  /* 0x00480DB1: push 0Ch / push 11h / push 0 / push buf -- type 12. */
  result = SL_GetStringOfLen((char *)nameBuf, 0, 0x11u, 12);
  *(unsigned short *)&dobj_defaultTracePartRemapHandle = result;
  return result;
}

/* ---- Scr_FreeGameStringRef_m  0x00480DD0 ----  VERIFIED */
/* Retail IDB name XAnimShutdownSmallTreePool (called from Com_Close 0x00438342
 * and Com_Restart 0x00438BF5). */
void Scr_FreeGameStringRef_m( void )
{
    if ( *(unsigned short *)&dobj_defaultTracePartRemapHandle ) {
        SL_RemoveRefToStringOfLen( *(unsigned short *)&dobj_defaultTracePartRemapHandle, 0x11 );
        *(unsigned short *)&dobj_defaultTracePartRemapHandle = 0;
    }
}

/* ---- QuatToMatrix_m  0x00480E20 ----  VERIFIED */
int __fastcall QuatToMatrix_m( float *mat, float *quat )
{
  float  xx;
  float  yy;
  float  zz;
  float  lengthSq;
  float  scale2;
  float  xx2;
  float  yy2;
  float  zz2;
  float  xs;
  float  xy;
  float  xz;
  float  wx;
  float  ys;
  float  yz;
  float  wy;
  float  wz;
  int    result;

  xx = quat[0] * quat[0];
  yy = quat[1] * quat[1];
  zz = quat[2] * quat[2];
  lengthSq = quat[3] * quat[3] + zz + yy + xx;
  /* Degenerate (zero-length) quaternion -> identity. */
  if ( lengthSq != 0.0f )
  {
    scale2 = 2.0f / lengthSq;
    result = 0;
    xx2 = xx * scale2;
    yy2 = scale2 * yy;
    zz2 = scale2 * zz;
    xs  = scale2 * quat[0];
    xy  = xs * quat[1];
    xz  = xs * quat[2];
    wx  = xs * quat[3];
    ys  = scale2 * quat[1];
    yz  = ys * quat[2];
    wy  = ys * quat[3];
    wz  = quat[3] * quat[2] * scale2;
    mat[3]  = 0.0f;
    mat[7]  = 0.0f;
    mat[11] = 0.0f;
    mat[0]  = 1.0f - ( zz2 + yy2 );
    mat[1]  = wz + xy;
    mat[2]  = xz - wy;
    mat[4]  = xy - wz;
    mat[5]  = 1.0f - ( zz2 + xx2 );
    mat[6]  = yz + wx;
    mat[8]  = wy + xz;
    mat[9]  = yz - wx;
    mat[10] = 1.0f - ( yy2 + xx2 );
  }
  else
  {
    result = 0;
    mat[0]  = 1.0f;
    mat[1]  = 0.0f;
    mat[2]  = 0.0f;
    mat[4]  = 0.0f;
    mat[5]  = 1.0f;
    mat[6]  = 0.0f;
    mat[8]  = 0.0f;
    mat[9]  = 0.0f;
    mat[10] = 1.0f;
    mat[3]  = 0.0f;
    mat[7]  = 0.0f;
    mat[11] = 0.0f;
  }
  return result;
}

const char *__cdecl DObjGetBoneName( DObj *dobj, int boneIndex );

/* ---- DObjDumpInfo  0x00480F60 ---- */
void __cdecl DObjDumpInfo( DObj *dobj )
{
    int i;
    int boneBase;
    int numModels;
    int numBones;

    if ( !dobj ) {
        Com_Printf( "No Dobj\n" );
        return;
    }

    Com_Printf( "\nModels:\n" );
    numModels = dobj->childCount;
    boneBase  = 0;
    for ( i = 0; i < numModels; ++i ) {
        const XModel *model = DOBJ_CHILD_MODEL( dobj, i );

        Com_Printf( "%d: '%s'\n", boneBase, model->name );
        boneBase += DObjModelPartNames( model )->count;
    }

    Com_Printf( "\nBones:\n" );
    numBones = dobj->partCount;
    for ( i = 0; i < numBones; ++i ) {
        Com_Printf( "Bone %d: '%s'\n", i, DObjGetBoneName( dobj, i ) );
    }

    if ( dobj->tracePartRemapHandle ) {
        const unsigned char *pair;

        Com_Printf( "\nPart duplicates:\n" );
        pair = (const unsigned char *)
                   SL_ConvertToString( dobj->tracePartRemapHandle )
               + DOBJ_TRACE_REMAP_PREFIX;

        while ( pair[0] ) {
            Com_Printf( "%d ('%s') -> %d ('%s')\n",
                        pair[0] - 1, DObjGetBoneName( dobj, pair[0] - 1 ),
                        pair[1] - 1, DObjGetBoneName( dobj, pair[1] - 1 ) );
            pair += 2;
        }
    } else {
        Com_Printf( "\nNo part duplicates.\n" );
    }

    Com_Printf( "\n" );
}

/* ---- DObjMatrixTransformVector43InPlace  0x004810B0 ----  VERIFIED */
/* vec first: the parameter order steers VC7's operand order to retail's. */
float *__cdecl DObjMatrixTransformVector43InPlace( float *vec, float *mat )
{
  float outX;
  float outY;

  outX = mat[8] * vec[2] + mat[4] * vec[1] + vec[0] * mat[0] + mat[12];
  outY = mat[9] * vec[2] + mat[5] * vec[1] + mat[1] * vec[0] + mat[13];
  vec[2] = mat[10] * vec[2] + mat[6] * vec[1] + mat[2] * vec[0] + mat[14];
  vec[0] = outX;
  vec[1] = outY;
  return mat;
}

/* ---- DObjQuatMultiplyIntoFirst  0x00481110 ----  VERIFIED */
float *__cdecl DObjQuatMultiplyIntoFirst( float *q, float *by )
{
  float temp[3];

  temp[0] = q[3] * by[0] + by[1] * q[2] + by[3] * q[0] - by[2] * q[1];
  temp[1] = by[3] * q[1] - by[0] * q[2] + q[3] * by[1] + by[2] * q[0];
  temp[2] = by[3] * q[2] + q[1] * by[0] - by[1] * q[0] + q[3] * by[2];
  q[3] = by[3] * q[3] - q[0] * by[0] - q[1] * by[1] - by[2] * q[2];
  q[0] = temp[0];
  q[1] = temp[1];
  q[2] = temp[2];
  return q;
}

/* ---- DObjQuatMultiplyIntoSecond  0x004811A0 ----  VERIFIED */
float *__cdecl DObjQuatMultiplyIntoSecond( float *by, float *q )
{
  float temp[3];

  temp[0] = by[3] * q[0] + q[1] * by[2] + q[3] * by[0] - q[2] * by[1];
  temp[1] = q[3] * by[1] - q[0] * by[2] + by[3] * q[1] + q[2] * by[0];
  temp[2] = q[3] * by[2] + by[1] * q[0] - q[1] * by[0] + by[3] * q[2];
  q[3] = q[3] * by[3] - by[0] * q[0] - by[1] * q[1] - q[2] * by[2];
  q[0] = temp[0];
  q[1] = temp[1];
  q[2] = temp[2];
  return q;
}

/* ---- DObjModelBoneIndex  no-address ----  (inlined copy of XModelGetBoneIndex 0x0048B690) */
static int DObjModelBoneIndex( const XModel *model, unsigned short name )
{
    const XModelPartNameTable *table   = DObjModelPartNames( model );
    const unsigned short      *handles = table->handles;
    int                        partIndex;

    for ( partIndex = table->count - 1; partIndex >= 0; --partIndex ) {
        if ( name == handles[partIndex] )
            break;
    }
    return partIndex;
}

/* ---- DObjFindPartIndex  0x00481230 ----  VERIFIED */
int __cdecl DObjFindPartIndex( DObj *dobj, unsigned short nameHandle )
{
    int childIndex;
    int partBase = 0;

    for ( childIndex = 0; childIndex < (int)dobj->childCount; ++childIndex ) {
        XModel *model     = DOBJ_CHILD_MODEL( dobj, childIndex );
        int     localPart = DObjModelBoneIndex( model, nameHandle );

        if ( localPart >= 0 )
            return partBase + localPart;
        partBase += DObjModelPartNames( model )->count;
    }
    return -1;
}

/* ---- DObjMeldModel_m  0x00481290 ----  VERIFIED */
extern void *__cdecl _alloca(unsigned int);
void __cdecl DObjMeldModel_m( DObj *dobj )
{
    unsigned char *remap=(unsigned char*)_alloca(2*dobj->partCount+DOBJ_TRACE_REMAP_PREFIX+1);
    unsigned int         remapOffset;
    int                  partBase;
    int                  childIndex;
    XModelPartNameTable *names;

    Com_Memset( remap, 0, DOBJ_TRACE_REMAP_PREFIX );

    partBase    = DObjModelPartNames( DOBJ_CHILD_MODEL( dobj, 0 ) )->count;
    remapOffset = DOBJ_TRACE_REMAP_PREFIX;

    for ( childIndex = 1; childIndex < (int)dobj->childCount; ++childIndex ) {
        XModel *model = DOBJ_CHILD_MODEL( dobj, childIndex );

        if ( dobj->childParentPartIndices[childIndex] == DOBJ_CHILD_PARENT_NONE ) {
            int localPart;
            int numParts;
            unsigned char remappedRootPart;

            names    = DObjModelPartNames( model );
            numParts = names->count;
            remappedRootPart = 0;
            for ( localPart = 0; localPart < numParts; ++localPart ) {
                int target = DObjFindPartIndex( dobj, names->handles[localPart] );
                int source = localPart + partBase;

                if ( target != source ) {
                    if ( localPart == 0 ) {
                        remappedRootPart = 1;
                    }
                    remap[remapOffset] = (unsigned char)( source + 1 );
                    DOBJ_BIT_SET( remap, source );
                    ++remapOffset;
                    remap[remapOffset] = (unsigned char)( target + 1 );
                    ++remapOffset;
                }
            }

            if ( !remappedRootPart ) {
                Com_Printf( "WARNING: Attempting to meld model, but root part "
                            "'%s' of model '%s' not found in model '%s' or any "
                            "of its descendants\n",
                            SL_ConvertToString( names->handles[0] ),
                            model->name,
                            DOBJ_CHILD_MODEL( dobj, 0 )->name );
            }
        }

        partBase += DObjModelPartNames( model )->count;
    }

    if ( remapOffset > DOBJ_TRACE_REMAP_PREFIX ) {
        /* retail 0x004813FE terminates with no bound check; written as a pointer store because
           remap[remapOffset] = 0 makes modern /GS add a range-check fast-fail */
        *( remap + remapOffset ) = 0;
        dobj->tracePartRemapHandle =
            SL_GetStringOfLen( (const char *)remap, 0, remapOffset + 1,
                               DOBJ_TRACE_REMAP_STRING_TYPE );
    } else {
        dobj->tracePartRemapHandle = (unsigned short)dobj_defaultTracePartRemapHandle;
    }
}

/* ---- DObjTraceRemapTarget  no-address ---- */
static int DObjTraceRemapTarget( const unsigned char *traceRemap, int sourcePart )
{
    const unsigned char *pair = traceRemap + DOBJ_TRACE_REMAP_PREFIX;

    while ( sourcePart != (int)pair[0] - 1 ) {
        pair += 2;
    }
    return (int)pair[1] - 1;
}

/* ---- DObjGetHierarchyBits  0x00481440 ----  VERIFIED */
void __cdecl DObjGetHierarchyBits( int boneIndex, DObj *dobj, unsigned int *partBits )
{
    const char              *traceRemap;
    const unsigned char     *parentPartDeltas;
    XModelPartsData         *parts;
    XModel                  *model;
    int                      childBases[8];
    int                      childIndex;
    int                      childEnd;
    int                      numModels;
    int                      localPart;
    int                      slot;
    int                      parentLocal;

    parentLocal = 0;   /* dead store: creates parentLocal early, giving retail's [deltas + index] operand order */
    memset( partBits, 0, 16 );
    numModels = dobj->childCount;

    if ( !dobj->tracePartRemapHandle ) {
        DObjMeldModel_m( dobj );
    }
    traceRemap = SL_ConvertToString( dobj->tracePartRemapHandle );

    childBases[0] = 0;
    childIndex    = 0;
    slot          = 0;
    parts         = DObjModelParts( DOBJ_CHILD_MODEL( dobj, 0 ) );
    childEnd      = parts->partNameTableSlot->partNameTable->count;

    while ( childEnd <= boneIndex ) {
        ++childIndex;
        ++slot;
        if ( childIndex == numModels ) {
            return;
        }
        model = DOBJ_CHILD_MODEL( dobj, childIndex );
        childBases[slot] = childEnd;
        parts    = DObjModelParts( model );
        childEnd = childBases[childIndex] + parts->partNameTableSlot->partNameTable->count;
    }

    parentPartDeltas = parts->partNameTableSlot->parentPartDeltas;

    for ( ;; ) {
        localPart = boneIndex - childBases[childIndex];

        for ( ;; ) {
            DOBJ_BIT_SET( partBits, boneIndex );

            if ( !( traceRemap[boneIndex >> 3] & ( 1 << ( boneIndex & 7 ) ) ) ) {
                parentLocal = localPart - parts->rootPartCount;
                if ( parentLocal >= 0 ) {
                    boneIndex -= parentPartDeltas[parentLocal];
                    break;
                }
                boneIndex = dobj->childParentPartIndices[childIndex];
                if ( boneIndex == DOBJ_CHILD_PARENT_NONE ) {
                    return;
                }
            } else {
                boneIndex = DObjTraceRemapTarget( (const unsigned char *)traceRemap, boneIndex );
            }

            do {
                --childIndex;
                localPart = boneIndex - childBases[childIndex];
            } while ( localPart < 0 );

            parts            = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
            parentPartDeltas = parts->partNameTableSlot->parentPartDeltas;
        }
    }
}

/* ---- DObjBackfillParentPartBits  0x004815B0 ----  VERIFIED */
void __cdecl DObjBackfillParentPartBits( DObj *dobj, unsigned char *partBits )
{
    const char              *traceRemap;
    const unsigned char     *parentPartDeltas;
    XModelPartsData         *parts;
    int                      childBases[8];
    int                      childIndex;
    int                      childEnd;
    int                      boneIndex;
    int                      localPart;
    int                      parentPart;
    int                      bit;
    int                      slot;
    int                      parentLocal;

    parentLocal = 0;   /* dead store: creates parentLocal early, giving retail's [deltas + index] operand order */
    boneIndex = (int)dobj->partCount - 1;

    if ( !dobj->tracePartRemapHandle ) {
        DObjMeldModel_m( dobj );
    }
    traceRemap = SL_ConvertToString( dobj->tracePartRemapHandle );

    childBases[0] = 0;
    childIndex    = 0;
    slot          = 0;
    parts         = DObjModelParts( DOBJ_CHILD_MODEL( dobj, 0 ) );
    childEnd      = parts->partNameTableSlot->partNameTable->count;

    while ( childEnd <= boneIndex ) {
        ++childIndex;
        ++slot;
        childBases[slot] = childEnd;
        parts    = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
        childEnd = childBases[childIndex]
                 + parts->partNameTableSlot->partNameTable->count;
    }
    parentPartDeltas = parts->partNameTableSlot->parentPartDeltas;

    for ( ;; ) {
        localPart = boneIndex - childBases[childIndex];
        if ( localPart < 0 ) {
            --childIndex;
            if ( childIndex < 0 ) {
                return;
            }
            parts            = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
            parentPartDeltas = parts->partNameTableSlot->parentPartDeltas;
            continue;
        }

        bit = 1 << ( boneIndex & 7 );
        /* retail's precedence: ( !byte ) & bit (sete + test) */
        if ( !partBits[boneIndex >> 3] & bit ) {
            --boneIndex;
            continue;
        }

        if ( !( traceRemap[boneIndex >> 3] & bit ) ) {
            parentLocal = localPart - parts->rootPartCount;
            if ( parentLocal >= 0 ) {
                parentPart = boneIndex - parentPartDeltas[parentLocal];
            } else {
                parentPart = dobj->childParentPartIndices[childIndex];
                if ( parentPart == DOBJ_CHILD_PARENT_NONE ) {
                    --boneIndex;
                    continue;
                }
            }
        } else {
            parentPart = DObjTraceRemapTarget( (const unsigned char *)traceRemap, boneIndex );
        }

        DOBJ_BIT_SET( partBits, parentPart );
        --boneIndex;
    }
}

/* ---- DObjSkelIsBoneUpToDate  0x00481720 ----  VERIFIED */
int DObjSkelIsBoneUpToDate( DObj *dobj, int boneIndex )
{
    XAnimEvalStorage *storage = (XAnimEvalStorage *)dobj->evalStorage;
    /* retail reads the bit byte as signed char (movsx) */
    return ( ((const char *)storage->blockedPartBits)[boneIndex >> 3]
             & ( 1 << ( boneIndex & 7 ) ) ) != 0;
}

/* ---- DObjCalcSkel  0x00481780 ----  VERIFIED */
void __cdecl DObjCalcSkel( const unsigned int *partBits, DObj *dobj )
{
    XAnimEvalStorage    *storage;
    unsigned int        *storageBits;
    unsigned int         blockedOrNotRequested[4];
    unsigned int         buildBits[4];
    unsigned int         skipQuatBits[4];
    const unsigned char *traceRemap;
    const unsigned char *remapPair;
    const unsigned char *remapStart;
    DObjSkelMat         *matrices;
    DObjSkelMat         *matrix;
    DObjAnimMat         *evalParts;
    DObjAnimMat         *evalPart;
    int                  word;
    int                  childIndex;
    int                  childCount;
    int                  globalPart;
    unsigned char        allBlocked;

    storage     = (XAnimEvalStorage *)dobj->evalStorage;
    storageBits = (unsigned int *)storage->evaluatedPartBits;
    allBlocked  = 1;

    for ( word = 0; word < 4; ++word ) {
        blockedOrNotRequested[word] = storageBits[8 + word] | ~partBits[word];
        if ( ~blockedOrNotRequested[word] ) {
            allBlocked = 0;
        }
    }
    if ( allBlocked ) {
        return;
    }

    if ( !dobj->tracePartRemapHandle ) {
        DObjMeldModel_m( dobj );
    }
    traceRemap = (const unsigned char *)
                     SL_ConvertToString( dobj->tracePartRemapHandle );

    for ( word = 0; word < 4; ++word ) {
        storageBits[8 + word] |= partBits[word];
        buildBits[word]    = storageBits[4 + word] & ~blockedOrNotRequested[word];
        skipQuatBits[word] = ( (const unsigned int *)traceRemap )[word]
                           | blockedOrNotRequested[word] | buildBits[word];
    }
    for ( word = 0; word < 4; ++word ) {
        buildBits[word] |= ~skipQuatBits[word];
    }

    childCount = dobj->childCount;
    /* retail keeps the pair-list start in a slot (0x00481839); computing it any earlier
       makes LTCG drop the eax partBits convention */
    remapStart = traceRemap + DOBJ_TRACE_REMAP_PREFIX;
    matrices   = storage->basePose;
    evalParts  = (DObjAnimMat *)&matrices[dobj->partCount];

    evalPart   = evalParts;
    globalPart = 0;
    remapPair  = remapStart;

    for ( childIndex = 0; childIndex < childCount; ++childIndex ) {
        XModelPartsData     *parts;
        const unsigned char *parentPartDeltas;
        unsigned char        parentPart;
        int                  remaining;
        int                  byteIndex;
        int                  bit;

        parts      = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
        parentPart = dobj->childParentPartIndices[childIndex];

        if ( parentPart == DOBJ_CHILD_PARENT_NONE ) {
            for ( remaining = parts->rootPartCount; remaining; --remaining ) {
                byteIndex = globalPart >> 3;
                bit       = 1 << ( globalPart & 7 );
                if ( !( ( (const char *)buildBits )[byteIndex] & bit )
                  && globalPart == remapPair[0] - 1 ) {
                    remapPair += 2;
                    if ( !( ( (const char *)blockedOrNotRequested )[byteIndex] & bit ) ) {
                        const DObjAnimMat *duplicate = &evalParts[remapPair[-1] - 1];

                        evalPart->quat[0] = duplicate->quat[0];
                        evalPart->quat[1] = duplicate->quat[1];
                        evalPart->quat[2] = duplicate->quat[2];
                        evalPart->quat[3] = duplicate->quat[3];
                    }
                }
                ++evalPart;
                ++globalPart;
            }
        } else {
            DObjAnimMat *parentEvalPart = &evalParts[parentPart];

            for ( remaining = parts->rootPartCount; remaining; --remaining ) {
                byteIndex = globalPart >> 3;
                bit       = 1 << ( globalPart & 7 );
                if ( !( ( (const char *)skipQuatBits )[byteIndex] & bit ) ) {
                    DObjQuatMultiplyIntoFirst( evalPart->quat, parentEvalPart->quat );
                } else if ( ( (const char *)buildBits )[byteIndex] & bit ) {
                    DObjQuatMultiplyIntoSecond( parentEvalPart->quat, evalPart->quat );
                }
                ++evalPart;
                ++globalPart;
            }
        }

        parentPartDeltas = parts->partNameTableSlot->parentPartDeltas;
        for ( remaining = parts->partNameTableSlot->partNameTable->count - parts->rootPartCount;
              remaining; --remaining ) {
            byteIndex = globalPart >> 3;
            bit       = 1 << ( globalPart & 7 );
            if ( !( ( (const char *)skipQuatBits )[byteIndex] & bit ) ) {
                DObjQuatMultiplyIntoFirst( evalPart->quat, ( evalPart - *parentPartDeltas )->quat );
            } else if ( ( (const char *)buildBits )[byteIndex] & bit ) {
                DObjQuatMultiplyIntoSecond( ( evalPart - *parentPartDeltas )->quat, evalPart->quat );
            } else if ( globalPart == remapPair[0] - 1 ) {
                remapPair += 2;
                if ( !( ( (const char *)blockedOrNotRequested )[byteIndex] & bit ) ) {
                    const DObjAnimMat *duplicate = &evalParts[remapPair[-1] - 1];

                    evalPart->quat[0] = duplicate->quat[0];
                    evalPart->quat[1] = duplicate->quat[1];
                    evalPart->quat[2] = duplicate->quat[2];
                    evalPart->quat[3] = duplicate->quat[3];
                }
            }
            ++evalPart;
            ++parentPartDeltas;
            ++globalPart;
        }
    }

    evalPart   = evalParts;
    matrix     = matrices;
    globalPart = 0;
    remapPair  = remapStart;

    for ( childIndex = 0; childIndex < childCount; ++childIndex ) {
        XModelPartsData     *parts;
        const unsigned char *parentPartDeltas;
        float              (*baseTranslations)[3];
        unsigned char        parentPart;
        int                  remaining;
        int                  byteIndex;
        int                  bit;

        parts      = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
        parentPart = dobj->childParentPartIndices[childIndex];

        if ( parentPart == DOBJ_CHILD_PARENT_NONE ) {
            for ( remaining = parts->rootPartCount; remaining; --remaining ) {
                byteIndex = globalPart >> 3;
                bit       = 1 << ( globalPart & 7 );
                if ( ( (const char *)buildBits )[byteIndex] & bit ) {
                    QuatToMatrix_m( (float *)matrix, evalPart->quat );
                    matrix->origin[0] = evalPart->trans[0];
                    matrix->origin[1] = evalPart->trans[1];
                    matrix->origin[2] = evalPart->trans[2];
                    matrix->pad3C     = 0.0f;
                } else if ( globalPart == remapPair[0] - 1 ) {
                    remapPair += 2;
                    if ( !( ( (const char *)blockedOrNotRequested )[byteIndex] & bit ) ) {
                        int duplicatePart = remapPair[-1] - 1;

                        evalPart->trans[0] = evalParts[duplicatePart].trans[0];
                        evalPart->trans[1] = evalParts[duplicatePart].trans[1];
                        evalPart->trans[2] = evalParts[duplicatePart].trans[2];
                        *matrix = matrices[duplicatePart];
                    }
                }
                ++evalPart;
                ++matrix;
                ++globalPart;
            }
        } else {
            DObjSkelMat *parentMatrix = &matrices[parentPart];

            for ( remaining = parts->rootPartCount; remaining; --remaining ) {
                byteIndex = globalPart >> 3;
                bit       = 1 << ( globalPart & 7 );
                if ( ( (const char *)buildBits )[byteIndex] & bit ) {
                    QuatToMatrix_m( (float *)matrix, evalPart->quat );
                    DObjMatrixTransformVector43InPlace( evalPart->trans, (float *)parentMatrix );
                    matrix->origin[0] = evalPart->trans[0];
                    matrix->origin[1] = evalPart->trans[1];
                    matrix->origin[2] = evalPart->trans[2];
                    matrix->pad3C     = 0.0f;
                }
                ++evalPart;
                ++matrix;
                ++globalPart;
            }
        }

        baseTranslations = parts->baseTranslations;
        parentPartDeltas = parts->partNameTableSlot->parentPartDeltas;
        for ( remaining = parts->partNameTableSlot->partNameTable->count - parts->rootPartCount;
              remaining; --remaining ) {
            byteIndex = globalPart >> 3;
            bit       = 1 << ( globalPart & 7 );
            if ( ( (const char *)buildBits )[byteIndex] & bit ) {
                QuatToMatrix_m( (float *)matrix, evalPart->quat );
                evalPart->trans[0] = ( *baseTranslations )[0] + evalPart->trans[0];
                evalPart->trans[1] = ( *baseTranslations )[1] + evalPart->trans[1];
                evalPart->trans[2] = ( *baseTranslations )[2] + evalPart->trans[2];
                /* retail inlines the parent transform here (0x00481D61) */
                DObjMatrixTransformVector43InPlace( evalPart->trans,
                    (float *)( matrix - *parentPartDeltas ) );
                matrix->origin[0] = evalPart->trans[0];
                matrix->origin[1] = evalPart->trans[1];
                matrix->origin[2] = evalPart->trans[2];
                matrix->pad3C     = 0.0f;
            } else if ( globalPart == remapPair[0] - 1 ) {
                remapPair += 2;
                if ( !( ( (const char *)blockedOrNotRequested )[byteIndex] & bit ) ) {
                    int duplicatePart = remapPair[-1] - 1;

                    evalPart->trans[0] = evalParts[duplicatePart].trans[0];
                    evalPart->trans[1] = evalParts[duplicatePart].trans[1];
                    evalPart->trans[2] = evalParts[duplicatePart].trans[2];
                    *matrix = matrices[duplicatePart];
                }
            }
            ++baseTranslations;
            ++evalPart;
            ++matrix;
            ++parentPartDeltas;
            ++globalPart;
        }
    }
}


/* ---- DObjBasePoseForChild  0x00481E90 ----  VERIFIED */
DObjSkelMat *__cdecl DObjBasePoseForChild(DObj *dobj, int childIndex)
{
  XAnimEvalStorage *storage = (XAnimEvalStorage *)dobj->evalStorage;

  return &storage->basePose[dobj->childPartBaseIndices[childIndex]];
}

int __cdecl XModelNumBones( const XModel *model );                          /* xmodel.cpp */
int __cdecl XModelGetBoneIndex( const XModel *model, unsigned short name ); /* xmodel.cpp */
void __cdecl XModelGetBounds( const XModel *model, float *mins, float *maxs ); /* xmodel.cpp */

/* ---- DObjCreate  0x00481EA0 ----  VERIFIED */

void __cdecl DObjCreate( const DObjModel *models, unsigned short modelCount, XAnimTree *tree,
                          DObj *dobj, unsigned short scrNotifyId )
{
    int modelIndex;
    int childIndex;
    int numBones;
    int total;
    const DObjModel *modelList;


    dobj->tree                = tree;
    dobj->evalStorage          = 0;
    dobj->skelCacheKey         = 0;
    dobj->scrNotifyId          = scrNotifyId;
    dobj->tracePartRemapHandle = 0;

    if ( tree ) {
        int            nodeCount = tree->sourceTree->nodeCount;
        unsigned char *remapTable;
        int            generation;

        remapTable = (unsigned char *)tree + 8 + 2 * nodeCount
                   + tree->partRemapTableSelector * ( 3 * nodeCount + 1 );
        dobj->partRemapTable = remapTable;
        generation = remapTable[2 * nodeCount] + 1;   /* byte counter; int keeps dobj in edi */
        if ( !(unsigned char)generation ) {
            generation = 1;
            Com_Memset( &remapTable[2 * nodeCount + 1], 0, nodeCount );
        }
        remapTable[2 * nodeCount] = (unsigned char)generation;
        tree->partRemapTableSelector = 1 - tree->partRemapTableSelector;
    } else {
        dobj->partRemapTable = 0;
    }


    childIndex = 0;
    numBones   = 0;

    /* a local copy frees the dead `models` home, which retail reuses for the childRefs cursor */
    modelList = models;
    for ( modelIndex = 0; modelIndex < (int)modelCount; ++modelIndex ) {
        const XModel *model = (const XModel *)modelList[modelIndex].model;
        const char   *tagName;

        dobj->childRefs[childIndex]              = (unsigned int)model;
        dobj->childParentPartIndices[childIndex] = DOBJ_CHILD_PARENT_NONE;
        dobj->childModelIndices[childIndex]      = modelList[modelIndex].negativeModelIndex;
        dobj->childPartBaseIndices[childIndex]   = (unsigned char)numBones;

        if ( modelIndex && ( tagName = modelList[modelIndex].tagName ) != 0 && *tagName ) {
            unsigned short tagHandle = SL_FindStringOfLen( tagName, strlen( tagName ) + 1 );
            int            j;

            if ( tagHandle ) {
                for ( j = 0; j < childIndex; ++j ) {
                    int boneIndex = XModelGetBoneIndex( DOBJ_CHILD_MODEL( dobj, j ), tagHandle );

                    if ( boneIndex >= 0 ) {
                        dobj->childParentPartIndices[childIndex] =
                            (unsigned char)( boneIndex + dobj->childPartBaseIndices[j] );
                        goto foundParent;
                    }
                }
            }
            Com_Printf( "WARNING: Part '%s' not found in model '%s' or any of "
                        "its descendants\n",
                        tagName, DOBJ_CHILD_MODEL( dobj, 0 )->name );
            continue;
        }
foundParent:
        total = numBones + XModelNumBones( model );
        if ( total >= 128 ) {
            Com_Error( ERR_DROP, "\025dobj for xmodel '%s' has more than %d bones",
                       DOBJ_CHILD_MODEL( dobj, 0 )->name, 127 );
            break;
        }
        numBones = total;
        ++childIndex;
    }

    dobj->childCount = (unsigned char)childIndex;
    dobj->partCount  = (unsigned char)numBones;
}

/* ---- DObjFree  0x004820C0 ----  VERIFIED */
void __cdecl DObjFree( unsigned int releaseTree, DObj *dobj )
{
    XAnimTree      *tree;

    tree = dobj->tree;
    if ( tree ) {
        if ( releaseTree ) {
            XAnimClearTree( tree );
        }
        tree->partRemapTableSelector =
            ( dobj->partRemapTable
              != (unsigned char *)tree + 8 + 2 * tree->sourceTree->nodeCount );
        dobj->partRemapTable = 0;
        dobj->tree           = 0;
    }

    if ( dobj->tracePartRemapHandle ) {
        if ( dobj->tracePartRemapHandle
             != (unsigned short)dobj_defaultTracePartRemapHandle ) {
            SL_RemoveRefToStringOfLen(
                dobj->tracePartRemapHandle,
                (unsigned int)strlen( GetRefString( dobj->tracePartRemapHandle )->text
                                      + DOBJ_TRACE_REMAP_PREFIX )
                    + DOBJ_TRACE_REMAP_PREFIX + 1 );
        }
        dobj->tracePartRemapHandle = 0;
    }
}

/* ---- DObjEvalStorageSize  0x00482170 ----  VERIFIED */
int __cdecl DObjEvalStorageSize(DObj *dobj)
{
  return (int)sizeof(DObjSkelMat) * dobj->partCount
       + (int)sizeof(DObjAnimMat) * dobj->partCount
       + 48;
}

/* ---- DObjRefreshEvalStorageKey  0x00482180 ----  VERIFIED */
int __cdecl DObjRefreshEvalStorageKey(DObj *dobj, int skelCacheKey)
{
  if ( dobj->skelCacheKey == skelCacheKey )
    return dobj->evalStorage != 0;
  dobj->skelCacheKey    = skelCacheKey;
  dobj->unknownState10  = 0;
  dobj->evalStorage     = 0;
  return 0;
}

/* ---- DObjEvalStorageIsCurrent  0x004821B0 ----  VERIFIED */
int __cdecl DObjEvalStorageIsCurrent(DObj *dobj, int skelCacheKey)
{
  return dobj->skelCacheKey == skelCacheKey && dobj->evalStorage != 0;
}

/* ---- DObjBindEvalStorage  0x004821D0 ----  VERIFIED */
/* retail inlines this in R_FinishLoadingStaticModels and keeps it out of line only in
   R_CreateStaticModel (0x00504B05); tr_staticmodel.c sets inline_depth(0) inside that body */
void __cdecl DObjBindEvalStorage(void *storage, DObj *dobj)
{
  XAnimEvalStorage *s = (XAnimEvalStorage *)storage;
  int i;

  dobj->evalStorage = storage;
  for ( i = 0; i < 4; i++ )
  {
    ((unsigned int *)s->evaluatedPartBits)[i] = 0;
    ((unsigned int *)s->controlPartBits)[i]   = 0;
    ((unsigned int *)s->blockedPartBits)[i]   = 0;
  }
}

/* ---- DObjGetChildCount  0x004821F0 ----  VERIFIED */
int __cdecl DObjGetChildCount(DObj *dobj)
{
  return dobj->childCount;
}

/* ---- DObjGetChildRef  0x00482200 ----  VERIFIED */
int __cdecl DObjGetChildRef(DObj *dobj, int childIndex)
{
  return (int)dobj->childRefs[childIndex];
}

/* ---- DObjGetBounds  0x00482210 ----  VERIFIED
 * XModelGetBounds inlined; through this chain SV_LinkEntity's inlined copy
 * keeps retail's register choice (0x0045F155). */
void __cdecl DObjGetBounds( DObj *dobj, float *mins, float *maxs )
{
    XModelGetBounds( DOBJ_CHILD_MODEL( dobj, 0 ), mins, maxs );
}

/* ---- DObjEvalStorageSeedOutput  0x00482240 ----  VERIFIED */
DObjAnimMat *__cdecl DObjEvalStorageSeedOutput(DObj *dobj)
{
  XAnimEvalStorage *storage = (XAnimEvalStorage *)dobj->evalStorage;

  return (DObjAnimMat *)&storage->basePose[dobj->partCount];
}

/* ---- DObjBuildPartCollisionTable  0x00482250 ----  VERIFIED */
void __cdecl DObjBuildPartCollisionTable( XModelPartColl **out, DObj *dobj )
{
    int childIndex;

    for ( childIndex = 0; childIndex < (int)dobj->childCount; ++childIndex ) {
        XModelPartsData *parts = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
        int               count = parts->partNameTableSlot->partNameTable->count;
        int               localPart;

        for ( localPart = 0; localPart < count; ++localPart, ++out ) {
            *out = &parts->partCollisions[localPart];
        }
    }
}

/* ---- DObjMarkRotTransIndex  0x004822B0 ----  VERIFIED */
int __cdecl DObjMarkRotTransIndex(DObj *dobj, const unsigned char *partBits, int boneIndex)
{
  int byteI = boneIndex >> 3;
  int mask  = 1 << (boneIndex & 7);
  XAnimEvalStorage *storage;

  /* retail reads the bit bytes as signed char (movsx) */
  if ( (((const char *)partBits)[byteI] & mask) == 0 )
    return 0;
  storage = (XAnimEvalStorage *)dobj->evalStorage;
  if ( (((char *)storage->blockedPartBits)[byteI] & mask) != 0 )
    return 0;
  storage->evaluatedPartBits[byteI] |= (unsigned char)mask;
  return 1;
}

/* ---- DObjMarkControlRotTransIndex  0x004822F0 ----  VERIFIED */
int __cdecl DObjMarkControlRotTransIndex(DObj *dobj, const unsigned char *partBits, int boneIndex)
{
  int byteI = boneIndex >> 3;
  int mask  = 1 << (boneIndex & 7);
  XAnimEvalStorage *storage;

  /* retail reads the bit bytes as signed char (movsx) */
  if ( (((const char *)partBits)[byteI] & mask) == 0 )
    return 0;
  storage = (XAnimEvalStorage *)dobj->evalStorage;
  if ( (((char *)storage->blockedPartBits)[byteI] & mask) != 0 )
    return 0;
  storage->controlPartBits[byteI]   |= (unsigned char)mask;
  storage->evaluatedPartBits[byteI] |= (unsigned char)mask;
  return 1;
}

/* ---- DObjGetNumSurfaces  0x00482330 ----  VERIFIED */
__declspec(noinline) int __cdecl DObjGetNumSurfaces( DObj *dobj, const int *lodIndices )
{
    int childIndex;
    int result = 0;

    for ( childIndex = (int)dobj->childCount - 1; childIndex >= 0; --childIndex ) {
        int lod = lodIndices[childIndex];

        if ( lod >= 0 ) {
            const char *info  = (const char *)DOBJ_CHILD_MODEL( dobj, childIndex )->info;
            int         surfs = *(int *)( info + 4 * ( 5 * lod + 5 ) );

            if ( surfs ) {
                result += *(short *)( *(int *)( surfs + 4 ) + 4 );
            }
        }
    }
    return result;
}

/* ---- DObjGetSurface  0x00482370 ----  VERIFIED */
int __cdecl DObjGetSurface( int childIndex, DObj *dobj, int surfaceIndex, const int *lodIndices )
{
    const char *info     = (const char *)DOBJ_CHILD_MODEL( dobj, childIndex )->info;
    int          lod       = lodIndices[childIndex];
    int          lodRecord = *(int *)( info + 20 * ( lod + 1 ) );
    int          surfsHdr  = *(int *)( lodRecord + 4 );
    int         *surfaces  = *(int **)( surfsHdr + 8 );

    return surfaces[surfaceIndex];
}

/* ---- DObjGetBoneName_m  0x004823A0 ----  VERIFIED */
const char *__cdecl DObjGetBoneName_m( int childIndex, int dobj,
                                       int surfaceIndex, int lodIndices )
{
    unsigned short handle;

    handle = *(unsigned short *)
        ( *(int *)( *(int *)( *(int *)( dobj + 4 * childIndex + 24 ) + 4 )
                    + 20 * *(int *)( lodIndices + 4 * childIndex ) + 16 )
          + 2 * surfaceIndex );

    if ( handle ) {
        return SL_REFSTRING( handle ) + 4;
    }
    return "DEFAULT";
}

/* ---- DObjGetSurfaces  0x004823E0 ---- */
typedef struct DObjSurfaceRef_s {
    short childIndex;
    short surfaceIndex;
} DObjSurfaceRef;

/* XSurface prefix: the 128-bit bone-use mask at +0x08 */
typedef struct DObjSurfaceBits_s {
    short        hdr[4];
    unsigned int boneUsage[4];
} DObjSurfaceBits;

/* ---- DObjGetSurfaces  0x004823E0 ----  VERIFIED */
void __cdecl DObjGetSurfaces( unsigned int *partBits, DObj *dobj, DObjSurfaceRef *out,
                        const int *lodIndices )
{
    int childIndex;
    int childCount;

    Com_Memset( partBits, 0, 16 );

    childCount = dobj->childCount;
    for ( childIndex = 0; childIndex < childCount; ++childIndex ) {
        int           lod;
        const XModel *model;
        int           surfsField;
        int           boneCount;
        int           surfsHdr;
        int           surfaceCount;
        DObjSurfaceBits **surfaces;
        int           childBaseBit;
        int           extraWords;
        int           destWord;
        int           shift;
        int           i;
        int           k;

        lod = lodIndices[childIndex];
        if ( lod < 0 )
            continue;

        model      = DOBJ_CHILD_MODEL( dobj, childIndex );
        surfsField = *(int *)( (const char *)model->info + 20 * lod + 20 );
        if ( !surfsField )
            continue;

        boneCount    = DObjModelPartNames( model )->count;
        extraWords   = ( boneCount - 1 ) >> 5;
        surfsHdr     = *(int *)( surfsField + 4 );
        surfaceCount = *(short *)( surfsHdr + 4 );
        childBaseBit = dobj->childPartBaseIndices[childIndex];
        surfaces     = *(DObjSurfaceBits ***)( surfsHdr + 8 );
        destWord     = childBaseBit >> 5;
        shift        = childBaseBit & 0x1F;

        if ( shift ) {
            int carryShift = 32 - shift;

            for ( i = 0; i < surfaceCount; ++i, ++out, ++surfaces ) {
                int dest;

                out->surfaceIndex = (short)i;
                out->childIndex   = (short)childIndex;

                partBits[destWord] |= (*surfaces)->boneUsage[0] << shift;
                dest = destWord + 1;
                for ( k = 0; k < extraWords; ++k ) {
                    partBits[dest] |= ( (*surfaces)->boneUsage[k] >> carryShift ) | ( (*surfaces)->boneUsage[k + 1] << shift );
                    ++dest;
                }
                /* retail (0x004824C7): a 0-bone child (extraWords -1) reads
                   bits[0] into partBits[destWord + 1] */
                partBits[dest] |= (*surfaces)->boneUsage[k] >> carryShift;
            }
        } else {
            for ( i = 0; i < surfaceCount; ++i, ++out, ++surfaces ) {
                out->surfaceIndex = (short)i;
                out->childIndex   = (short)childIndex;

                for ( k = 0; k <= extraWords; ++k ) {
                    unsigned int *dst = &partBits[destWord + k];

                    *dst |= (*surfaces)->boneUsage[k];
                }

            }
        }
    }
    return;
}

/* ---- DObjGetBoneIndex  0x004825E0 ----  VERIFIED */
int __cdecl DObjGetBoneIndex( const char *tagName, DObj *dobj )
{
    unsigned short handle = SL_FindLowercaseString( tagName );

    if ( handle )
        return DObjFindPartIndex( dobj, handle );
    return -1;
}

/* ---- DObjGetBoneName  0x00482600 ----  VERIFIED */
const char *__cdecl DObjGetBoneName( DObj *dobj, int boneIndex )
{
    XModelPartNameTable *names;
    int                  base;
    int                  childIndex;
    int                  childCount;
    int                  localIndex;
    XModel              *model;

    childCount = dobj->childCount;
    base       = 0;
    for ( childIndex = 0; childIndex < childCount; ++childIndex ) {
        model      = DOBJ_CHILD_MODEL( dobj, childIndex );
        names      = DObjModelPartNames( model );
        localIndex = boneIndex - base;
        if ( localIndex < names->count ) {
            return SL_ConvertToString( names->handles[localIndex] );
        }
        base += DObjModelPartNames( model )->count;
    }
    return 0;
}

/* ---- DObjGetTree  0x00482660 ----  VERIFIED */
XAnimTree *__cdecl DObjGetTree(DObj *dobj)
{
  return dobj->tree;
}

/* ---- DObjHasDefaultModel  0x00482670 ---- */
// DObjHasDefaultModel: true if any child model failed to load and is showing the DEFAULT model (XModel->info compared against the default XModelInfo at 0x00A9CCCC).
extern char xmodel_defaultCollision[];

/* ---- DObjHasDefaultModel  0x00482670 ----  VERIFIED */
int __cdecl DObjHasDefaultModel( DObj *dobj )
{
    int childIndex;

    for ( childIndex = (int)dobj->childCount - 1; childIndex >= 0; --childIndex ) {
        if ( (void *)DOBJ_CHILD_MODEL( dobj, childIndex )->info == (void *)xmodel_defaultCollision )
            return 1;
    }
    return 0;
}

/* ---- DObjNumBones  0x004826A0 ----  VERIFIED */
int __cdecl DObjNumBones(DObj *dobj)
{
  return dobj->partCount;
}

/* ---- DObjMatrixTransformVector43  0x004826B0 ----  VERIFIED */
/* Uncalled; the parameter order (vec first) only steers VC7's operand order. */
float *__cdecl DObjMatrixTransformVector43( const float *vec, float *mat, float *out )
{
  out[0] = mat[8] * vec[2] + mat[4] * vec[1] + vec[0] * mat[0] + mat[12];
  out[1] = mat[1] * vec[0] + mat[9] * vec[2] + mat[5] * vec[1] + mat[13];
  out[2] = mat[2] * vec[0] + mat[10] * vec[2] + mat[6] * vec[1] + mat[14];
  return mat;
}

/* ---- DObjMatrixInverseTransformVector43  0x00482700 ----  VERIFIED */
float *__cdecl DObjMatrixInverseTransformVector43( float *mat, float *point, float *out )
{
  rfloat dx;
  rfloat dy;
  rfloat dz;

  dx = point[0] - mat[12];
  dy = point[1] - mat[13];
  dz = point[2] - mat[14];
  out[0] = dz * mat[2] + dy * mat[1] + dx * mat[0];
  out[1] = dz * mat[6] + dy * mat[5] + dx * mat[4];
  out[2] = dz * mat[10] + dy * mat[9] + dx * mat[8];
  return mat;
}

typedef struct DObjTrace_s {
    float          fraction;              /* +0x00 */
    int            surfaceFlags;          /* +0x04 */
    float          normal[3];             /* +0x08  model space */
    unsigned short partName;              /* +0x14 */
    unsigned short partGroup;             /* +0x16 */
    unsigned char  startsolid;            /* +0x18 */
    unsigned char  allsolid;              /* +0x19 */
    unsigned char  pad1A[2];              /* +0x1A */
} DObjTrace;

#ifdef COD1_VC7
/* ---- DObjSkelTransformPoint  no-address ----  (inlined into DObjTraceParts) */
static void DObjSkelTransformPoint( const float *in, const float *mat, float *out )
{
    /* the extra parentheses only steer VC7's x87 operand order (retail 0x0048295D) */
    out[0] = ( ( ( in[0] * mat[0] ) + in[2] * mat[8] ) + ( in[1] * mat[4] ) ) + mat[12];
    out[1] = ( ( ( in[2] * mat[9] ) + in[1] * mat[5] ) + ( in[0] * mat[1] ) ) + mat[13];
    out[2] = ( ( ( in[2] * mat[10] ) + in[1] * mat[6] ) + ( in[0] * mat[2] ) ) + mat[14];
}

#endif /* COD1_VC7 */

/* ---- DObjTraceParts  0x00482750 ----  VERIFIED */
#ifdef COD1_VC7
/* VC7 form: the grouping and rounding points are retail's; the #else form spells the same
   arithmetic out in doubles so the modern build rounds where retail does. */
void __cdecl DObjTraceParts( DObjTrace *trace, DObj *dobj, const float *start,
                         const float *end, const unsigned char *partState )
{
    unsigned short       remappedPartStateIndices[128];
    float                delta[3];
    float                invLenSq;
    const unsigned char *remapPair;
    DObjSkelMat         *basePose;
    unsigned int         bestPriority;
    int                  globalPart;
    int                  childIndex;
    int                  hitAxis;
    float                hitSign;

    VectorSubtract( end, start, delta );
    invLenSq = 1.0f / DotProduct( delta, delta );

    basePose = DObjBasePoseForChild( dobj, 0 );

    trace->surfaceFlags = 0;
    trace->startsolid   = 0;
    trace->allsolid     = 0;
    trace->partName     = 0;
    trace->partGroup    = 0;
    VectorClear( trace->normal );

    bestPriority = 2;
    remapPair = (const unsigned char *)
                    SL_ConvertToString( dobj->tracePartRemapHandle )
              + DOBJ_TRACE_REMAP_PREFIX;
    globalPart = 0;
    hitAxis    = -1;
    hitSign    = 0.0f;

    for ( childIndex = 0; childIndex < (int)dobj->childCount; ++childIndex ) {
        XModelPartsData     *parts;
        XModelPartNameTable *names;
        int                  numParts;
        int                  localPart;

        parts    = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
        names    = parts->partNameTableSlot->partNameTable;
        numParts = names->count;

        for ( localPart = 0; localPart < numParts; ++localPart, ++basePose, ++globalPart ) {
            unsigned short  partStateIndex;
            unsigned int    partPriority;
            XModelPartColl *part;
            const float    *m;
            rfloat          dx;
            rfloat          dy;
            rfloat          dz;
            float           center[3];
            float           startToCenter[3];
            float           offset[3];
            float           projection;
            rfloat          distSq;
            float           radiusDelta;
            float           localStart[3];
            float           localEnd[3];
            float           enter;
            float           exitFrac;
            rfloat          sign;
            const float    *bounds;
            unsigned char   startInside;
            unsigned char   endInside;
            int             axis;

            partStateIndex = parts->partStateIndices[localPart];
            partPriority   = partState[partStateIndex];

            if ( globalPart == remapPair[0] - 1 ) {
                remapPair += 2;
                if ( partPriority == 1 ) {
                    partStateIndex = remappedPartStateIndices[remapPair[-1] - 1];
                    partPriority   = partState[partStateIndex];
                }
            } else if ( partPriority == 1 ) {
                if ( localPart < parts->rootPartCount ) {
                    if ( dobj->childParentPartIndices[childIndex] != DOBJ_CHILD_PARENT_NONE ) {
                        partStateIndex = remappedPartStateIndices[dobj->childParentPartIndices[childIndex]];
                    } else {
                        partStateIndex = 0;
                    }
                } else {
                    partStateIndex = remappedPartStateIndices[
                        globalPart - parts->partNameTableSlot->parentPartDeltas[localPart - parts->rootPartCount]];
                }
                partPriority = partState[partStateIndex];
            }

            remappedPartStateIndices[globalPart] = partStateIndex;

            part = &parts->partCollisions[localPart];
            if ( part->radiusSq == 0.0f ) {
                continue;
            }
            if ( bestPriority > partPriority ) {
                continue;
            }

            m = (const float *)basePose;
            DObjSkelTransformPoint( part->center, m, center );
            VectorSubtract( start, center, startToCenter );
            projection = -DotProduct( startToCenter, delta ) * invLenSq;
            if ( projection < 1.0f ) {
                if ( projection > 0.0f ) {
                    VectorMA( startToCenter, projection, delta, offset );
                    distSq = DotProduct( offset, offset );
                } else {
                    distSq = DotProduct( startToCenter, startToCenter );
                }
            } else {
                VectorSubtract( end, center, offset );
                distSq = DotProduct( offset, offset );
            }

            radiusDelta = part->radiusSq - distSq;
            if ( radiusDelta <= 0.0f ) {
                continue;
            }
            if ( bestPriority == partPriority
              && projection - FCAST( sqrt( radiusDelta * invLenSq ) ) >= trace->fraction ) {
                continue;
            }

            dx = start[0] - m[12];
            dy = start[1] - m[13];
            dz = start[2] - m[14];
            localStart[0] = ( dx * m[0] + dy * m[1] ) + dz * m[2];
            localStart[1] = ( dz * m[6] + dy * m[5] ) + dx * m[4];
            localStart[2] = ( dz * m[10] + dy * m[9] ) + dx * m[8];
            dx = end[0] - m[12];
            dy = end[1] - m[13];
            dz = end[2] - m[14];
            localEnd[0] = ( dx * m[0] + dy * m[1] ) + dz * m[2];
            localEnd[1] = ( dz * m[6] + dy * m[5] ) + dx * m[4];
            localEnd[2] = ( dz * m[10] + dy * m[9] ) + dx * m[8];

            enter       = 0.0f;
            exitFrac    = trace->fraction;
            startInside = 1;
            endInside   = 1;
            sign        = -1.0f;
            bounds      = part->mins;

            for ( ;; ) {
                for ( axis = 0; axis < 3; ++axis ) {
                    float  startDist = ( localStart[axis] - bounds[axis] ) * sign;
                    rfloat endDist   = ( localEnd[axis] - bounds[axis] ) * sign;
                    rfloat denom;

                    if ( startDist > 0.0f ) {
                        if ( endDist > 0.0f ) {
                            goto nextPart;
                        }
                        startInside = 0;
                        denom = startDist - endDist;
                        if ( enter * denom < startDist ) {
                            enter = startDist / denom;
                            if ( enter >= exitFrac ) {
                                goto nextPart;
                            }
                            hitSign = sign;
                            hitAxis = axis;
                        }
                    } else if ( endDist > 0.0f ) {
                        endInside = 0;
                        denom = startDist - endDist;
                        if ( exitFrac * denom < startDist ) {
                            exitFrac = startDist / denom;
                            if ( enter >= exitFrac ) {
                                goto nextPart;
                            }
                        }
                    }
                }
                if ( sign == 1.0f ) {
                    break;
                }
                sign   = 1.0f;
                bounds += 3;
            }

            if ( startInside ) {
                trace->startsolid = 1;
                if ( endInside ) {
                    trace->allsolid  = 1;
                    trace->fraction  = 0.0f;
                    trace->partName  = names->handles[localPart];
                    trace->partGroup = partStateIndex;
                    VectorClear( trace->normal );
                    return;
                }
                continue;
            }

            if ( bestPriority == partPriority ) {
                if ( enter >= trace->fraction ) {
                    continue;
                }
            } else {
                bestPriority = partPriority;
            }

            trace->fraction  = enter;
            trace->partName  = names->handles[localPart];
            trace->partGroup = partStateIndex;
            VectorScale( basePose->axis[hitAxis], hitSign, trace->normal );
nextPart:
            ;
        }
    }
}
#else
void __cdecl DObjTraceParts( DObjTrace *trace, DObj *dobj, const float *start,
                         const float *end, const unsigned char *partState )
{
    unsigned short       remappedPartStateIndices[128];
    float                delta[3];
    float                invLenSq;
    const unsigned char *remapPair;
    DObjSkelMat         *basePose;
    unsigned int         bestPriority;
    int                  globalPart;
    int                  childIndex;
    int                  hitAxis;
    float                hitSign;

    delta[0] = end[0] - start[0];
    delta[1] = end[1] - start[1];
    {
        /* Retail (0x004827A3) sums z, y, x and squares z from the unrounded
           difference (fst, then fmul of the same st0). */
        double dz = (double)end[2] - start[2];

        delta[2] = (float)dz;
        invLenSq = (float)( 1.0 / ( ( dz * delta[2]
                                    + (double)delta[1] * delta[1] )
                                  + (double)delta[0] * delta[0] ) );
    }

    basePose = DObjBasePoseForChild( dobj, 0 );

    trace->surfaceFlags = 0;
    trace->startsolid   = 0;
    trace->allsolid     = 0;
    trace->partName     = 0;
    trace->partGroup    = 0;
    trace->normal[0]    = 0.0f;
    trace->normal[1]    = 0.0f;
    trace->normal[2]    = 0.0f;

    /* Retail makes no calls here: it converts the handle inline (0 gives
       NULL) and adds the prefix, so an unmelded DObj reads address 0x10.
       In practice DObjCalcSkel, which melds, has run first. */
    remapPair = (const unsigned char *)
                    SL_ConvertToString( dobj->tracePartRemapHandle )
              + DOBJ_TRACE_REMAP_PREFIX;

    bestPriority = 2;
    globalPart   = 0;
    hitAxis      = -1;
    hitSign      = 0.0f;

    for ( childIndex = 0; childIndex < (int)dobj->childCount; ++childIndex ) {
        XModelPartsData     *parts;
        XModelPartNameTable *names;
        XModelPartColl      *partColls;
        const unsigned char *partStateIndices;
        const unsigned char *parentPartDeltas;
        int                  rootPartCount;
        int                  localPart;

        parts            = DObjModelParts( DOBJ_CHILD_MODEL( dobj, childIndex ) );
        names            = parts->partNameTableSlot->partNameTable;
        parentPartDeltas = parts->partNameTableSlot->parentPartDeltas;
        partColls        = parts->partCollisions;
        partStateIndices = parts->partStateIndices;
        rootPartCount    = parts->rootPartCount;

        for ( localPart = 0; localPart < (int)names->count;
              ++localPart, ++globalPart, ++basePose ) {
            unsigned short  partStateIndex;
            unsigned int    partPriority;
            XModelPartColl *part;
            const float    *m;
            const float    *c;
            double          cx;
            float           cy;
            float           cz;
            float           startToCenter[3];
            double          projD;
            float           projection;
            double          distanceSq;
            float           radiusDelta;
            float           localStart[3];
            float           localEnd[3];
            float           enter;
            float           exitFrac;
            float           sign;
            const float    *bounds;
            int             startInside;
            int             endInside;
            int             axis;
            int             missed;

            partStateIndex = partStateIndices[localPart];
            partPriority   = partState[partStateIndex];

            if ( globalPart == (int)remapPair[0] - 1 ) {
                const unsigned char *pair = remapPair;

                remapPair += 2;
                if ( partPriority == 1 ) {
                    partStateIndex = remappedPartStateIndices[pair[1] - 1];
                    partPriority   = partState[partStateIndex];
                }
            } else if ( partPriority == 1 ) {
                if ( localPart >= rootPartCount ) {
                    partStateIndex = remappedPartStateIndices[
                        globalPart
                        - (int)parentPartDeltas[localPart - rootPartCount]];
                } else if ( dobj->childParentPartIndices[childIndex]
                            == DOBJ_CHILD_PARENT_NONE ) {
                    partStateIndex = 0;
                } else {
                    partStateIndex = remappedPartStateIndices[
                        dobj->childParentPartIndices[childIndex]];
                }
                partPriority = partState[partStateIndex];
            }

            remappedPartStateIndices[globalPart] = partStateIndex;

            part = &partColls[localPart];
            if ( part->radiusSq == 0.0f || bestPriority > partPriority ) {
                continue;
            }

            /* Retail inlines the base-pose math here (0x0048295D onward) in
               its own x87 order; doubles stand for values it keeps on the
               FPU stack, floats for what it stores with fstp dword. The
               standalone DObjMatrixTransformVector43 and
               DObjMatrixInverseTransformVector43 are not called. */
            m  = (const float *)basePose;
            c  = part->center;
            cx = ( ( (double)c[0] * m[0] + (double)c[2] * m[8] )
                   + (double)m[4] * c[1] ) + m[12];
            cy = (float)( ( ( (double)c[2] * m[9] + (double)c[1] * m[5] )
                            + (double)c[0] * m[1] ) + m[13] );
            cz = (float)( ( ( (double)c[2] * m[10] + (double)c[1] * m[6] )
                            + (double)c[0] * m[2] ) + m[14] );
            startToCenter[0] = (float)( start[0] - cx );
            startToCenter[1] = (float)( (double)start[1] - cy );
            {
                double s2 = (double)start[2] - cz;

                startToCenter[2] = (float)s2;
                projD = -( ( ( s2 * delta[2]
                             + (double)startToCenter[1] * delta[1] )
                           + (double)startToCenter[0] * delta[0] ) * invLenSq );
            }
            projection = (float)projD;

            /* retail compares the unrounded projection with 1.0 */
            if ( projD < 1.0 ) {
                if ( projection > 0.0f ) {
                    double n0 = (double)projection * delta[0] + startToCenter[0];
                    double n1 = (double)projection * delta[1] + startToCenter[1];
                    double n2 = (double)projection * delta[2] + startToCenter[2];

                    distanceSq = ( n2 * n2 + n1 * n1 ) + n0 * n0;
                } else {
                    distanceSq = ( (double)startToCenter[2] * startToCenter[2]
                                 + (double)startToCenter[1] * startToCenter[1] )
                               + (double)startToCenter[0] * startToCenter[0];
                }
            } else {
                double e0 = end[0] - cx;
                double e1 = (double)end[1] - cy;
                double e2 = (double)end[2] - cz;

                distanceSq = ( e2 * e2 + e1 * e1 ) + e0 * e0;
            }

            radiusDelta = (float)( part->radiusSq - distanceSq );
            if ( radiusDelta <= 0.0f ) {
                continue;
            }
            /* retail keeps the sqrt unrounded on the x87 stack */
            if ( bestPriority == partPriority
              && (double)projection - sqrt( (double)radiusDelta * invLenSq )
                     >= trace->fraction ) {
                continue;
            }

            {
                double dx;
                double dy;
                double dz;

                dx = (double)start[0] - m[12];
                dy = (double)start[1] - m[13];
                dz = (double)start[2] - m[14];
                localStart[0] = (float)( ( dx * m[0] + dy * m[1] ) + dz * m[2] );
                localStart[1] = (float)( ( dz * m[6] + dy * m[5] ) + dx * m[4] );
                localStart[2] = (float)( ( dz * m[10] + dy * m[9] ) + dx * m[8] );

                dx = (double)end[0] - m[12];
                dy = (double)end[1] - m[13];
                dz = (double)end[2] - m[14];
                localEnd[0] = (float)( ( dx * m[0] + dy * m[1] ) + dz * m[2] );
                localEnd[1] = (float)( ( dz * m[6] + dy * m[5] ) + dx * m[4] );
                localEnd[2] = (float)( ( dz * m[10] + dy * m[9] ) + dx * m[8] );
            }

            enter       = 0.0f;
            exitFrac    = trace->fraction;
            startInside = 1;
            endInside   = 1;
            sign        = -1.0f;
            bounds      = part->mins;
            missed      = 0;

            for ( ;; ) {
                for ( axis = 0; axis < 3; ++axis ) {
                    float startDist = ( localStart[axis] - bounds[axis] ) * sign;
                    float endDist   = ( localEnd[axis]   - bounds[axis] ) * sign;
                    float denom;

                    if ( startDist > 0.0f ) {
                        if ( endDist > 0.0f ) {
                            missed = 1;
                            break;
                        }
                        startInside = 0;
                        denom = startDist - endDist;
                        if ( enter * denom < startDist ) {
                            enter = startDist / denom;
                            if ( enter >= exitFrac ) {
                                missed = 1;
                                break;
                            }
                            hitSign = sign;
                            hitAxis = axis;
                        }
                    } else if ( endDist > 0.0f ) {
                        endInside = 0;
                        denom = startDist - endDist;
                        if ( exitFrac * denom < startDist ) {
                            exitFrac = startDist / denom;
                            if ( enter >= exitFrac ) {
                                missed = 1;
                                break;
                            }
                        }
                    }
                }
                if ( missed || sign == 1.0f ) {
                    break;
                }
                sign   = 1.0f;
                bounds = part->maxs;
            }
            if ( missed ) {
                continue;
            }

            if ( startInside ) {
                trace->startsolid = 1;
                if ( endInside ) {
                    trace->allsolid  = 1;
                    trace->fraction  = 0.0f;
                    trace->partName  = names->handles[localPart];
                    trace->partGroup = partStateIndex;
                    trace->normal[0] = 0.0f;
                    trace->normal[1] = 0.0f;
                    trace->normal[2] = 0.0f;
                    return;
                }
                continue;
            }

            if ( bestPriority == partPriority ) {
                if ( enter >= trace->fraction ) {
                    continue;
                }
            } else {
                bestPriority = partPriority;
            }

            trace->fraction  = enter;
            trace->partName  = names->handles[localPart];
            trace->partGroup = partStateIndex;
            trace->normal[0] = hitSign * basePose->axis[hitAxis][0];
            trace->normal[1] = hitSign * basePose->axis[hitAxis][1];
            trace->normal[2] = hitSign * basePose->axis[hitAxis][2];
        }
    }

}

#endif /* COD1_VC7 */

/* ---- DObjTraceModelParts  0x00482DC0 ---- */
// DObjTraceModelParts: content-mask trace across every child model, advancing the base pose by XModelNumBones per child; records the hit bone name.  sv_world_mp.c passes contentMask from cmPointTraceWork+0x50.  SERVER PATH.

/* The work record XModelTraceLine fills is a trace_t (q_shared.h). */

/* ---- DObjTraceModelParts  0x00482DC0 ----  VERIFIED */
void __cdecl DObjTraceModelParts( DObjTrace *trace, DObj *dobj, float *a3, float *a4, int a5 )
{
  trace_t        work;
  unsigned char *basePose;
  __int16       *partNames;
  int            i;
  int            hit;
 float initialFraction;

  basePose = (unsigned char *)dobj->evalStorage + 48
             + ( dobj->childPartBaseIndices[0] << 6 );

  initialFraction=trace->fraction;
  trace->partName    = 0;
  trace->partGroup   = 0;
  trace->startsolid  = 0;
  trace->allsolid    = 0;
  work.surfaceFlags = 0;
  VectorClear( work.normal );

  work.fraction=initialFraction;
  for ( i = 0; i < (int)dobj->childCount; i++ )
  {
    partNames = (__int16 *)DObjModelPartNames( DOBJ_CHILD_MODEL( dobj, i ) );
    hit = XModelTraceLine( (int)dobj->childRefs[i], (int)&work,
                      (const float *)basePose, a3, a4, a5 );
    if ( hit >= 0 )
      trace->partName = partNames[hit + 1];
    basePose += partNames[0] << 6;
  }

  trace->fraction     = work.fraction;
  trace->surfaceFlags = work.surfaceFlags;
  trace->normal[0]    = work.normal[0];
  trace->normal[1]    = work.normal[1];
  trace->normal[2]    = work.normal[2];
}

/* ---- DObjGetLodForDist_m  0x00482EA0 ----  VERIFIED */
int __cdecl DObjGetLodForDist_m(DObj *dobj, int childIndex, float distance)
{
  return XModelGetLodForDist((const void *)dobj->childRefs[childIndex], distance);
}

/* ---- DObjHasContents  0x00482EB0 ----  VERIFIED */
int __cdecl DObjHasContents( DObj *dobj, int contentsMask )
{
    int childIndex;

    for ( childIndex = 0; childIndex < (int)dobj->childCount; ++childIndex ) {
        const char *info = (const char *)DOBJ_CHILD_MODEL( dobj, childIndex )->info;

        if ( contentsMask & *(int *)( info + 72 ) )
            return 1;
    }
    return 0;
}

/* ---- XAnimGetPoolHighWaterBytes  0x00482EE0 ----  VERIFIED */
int XAnimGetPoolHighWaterBytes()
{
  return 68 * xanim_poolHighWaterCount;
}

/* ---- XAnimGetPoolUsedBytes  0x00482EF0 ----  VERIFIED */
int XAnimGetPoolUsedBytes()
{
  return 68 * xanim_poolUsedCount;
}

/* ---- XAnimInit  0x00482F00 ----  VERIFIED */
void XAnimInit()
{
    int     node;

    for ( node = 0; node < 2048; ++node ) {
        xanim_pool[node].freePrev = (unsigned short)( ( node + 2047 ) % 2048 );
        xanim_pool[node].freeNext = (unsigned short)( ( node + 1 ) % 2048 );
    }

    xanim_pool[0].states[0].time = 0.0f;
    xanim_pool[0].states[0].oldTime = 0.0f;
    xanim_pool[0].states[1].time = 0.0f;
    xanim_pool[0].states[1].oldTime = 0.0f;
    xanim_pool[0].states[0].cycleCount = 0;
    xanim_pool[0].states[0].oldCycleCount = 0;
    xanim_pool[0].states[1].cycleCount = 0;
    xanim_pool[0].states[1].oldCycleCount = 0;

    *(unsigned short *)&xanim_endNotifyStringHandle = SL_GetString_( "end", 0, 3 );

    xanim_poolUsedCount = 1;
    xanim_poolHighWaterCount = 1;
}

/* ---- XAnimShutdown  0x00482FD0 ----  VERIFIED */
void XAnimShutdown( void )
{
    if ( *(unsigned short *)&xanim_endNotifyStringHandle ) {
        SL_RemoveRefToString( *(unsigned short *)&xanim_endNotifyStringHandle );
        *(unsigned short *)&xanim_endNotifyStringHandle = 0;
    }
}

void __cdecl XAnimPrecacheAnimTree_m( const char *animName, int (__cdecl *alloc)(int) );

/* ---- XAnimBeginLoadFiles  0x00483040 ----  VERIFIED */
void XAnimBeginLoadFiles( void )
{
  VariableValueInternal *node;

  node = AllocVariable();
  node->status = 111;
  node->u.halfword.lo = 0;
  node->u.halfword.hi = 0;

  *(unsigned short *)&xanim_pendingFilesHandle = (unsigned short)( node - scrVarNodes );
}

/* ---- XAnimLoadPendingFiles  0x00483080 ----  VERIFIED */
void __cdecl XAnimLoadPendingFiles( int (__cdecl *allocFn)(int) )
{
  unsigned short id;

  for ( id = FindNextSibling( (unsigned short)xanim_pendingFilesHandle ); id; id = FindNextSibling( id ) )
  {
    XAnimPrecacheAnimTree_m( SL_ConvertToString( (unsigned short)GetVariableName( id ) ), allocFn );
  }

  RemoveRefToObject( (unsigned short)xanim_pendingFilesHandle );              /* inlined at 0x0048311E */
  *(unsigned short *)&xanim_pendingFilesHandle = 0;
}

typedef struct XAnimFileEntry_s {
    const char *name;                               /* +0x00 */
    XAnimParts *parts;                              /* +0x04 */
    void      (*freeData)( struct XAnimFileEntry_s * ); /* +0x08 */
} XAnimFileEntry;

/* ---- XAnimFreeMemory  0x00483170 ----  VERIFIED */
void __cdecl XAnimFreeMemory( XAnimFileEntry *entry )
{
    XAnimParts      *parts = entry->parts;
    unsigned short  *partNameHandles;
    unsigned short  *names;
    XAnimNotifyInfo *notify;
    int              partNameCount;
    int              i;

    if ( (unsigned short)xanim_pendingFilesHandle ) {
        unsigned short nameHandle;

        nameHandle = SL_GetLowercaseStringOfLen(
                         entry->name, 0,
                         (unsigned int)strlen( entry->name ) + 1,
                         XANIM_FILENAME_STRING_TYPE );
        GetVariableIndexInternal( (unsigned short)xanim_pendingFilesHandle, nameHandle );
        SL_RemoveRefToString( nameHandle );
    }

    partNameHandles = parts->partNameHandles;
    partNameCount   = (short)partNameHandles[0];
    names           = partNameHandles + 1;

    for ( i = 0; i < partNameCount; ++i ) {
        SL_RemoveRefToString( names[i] );
    }

    notify = parts->notify;
    if ( notify != 0 ) {
        while ( notify->name != 0 ) {
            SL_RemoveRefToString( notify->name );
            ++notify;
        }
    }
}

/* ---- ReadNoteTracks  0x004832C0 ----  VERIFIED */
char *__cdecl ReadNoteTracks( unsigned char *cursor, int unused, XAnimParts *parts,
                          int (__cdecl *alloc)(int) )
{
    int              count;
    int              i;
    char            *namePtr;
    XAnimNotifyInfo *notify;
    int              frame;

    count   = *cursor;
    namePtr = (char *)( cursor + 1 );
    notify  = (XAnimNotifyInfo *)alloc( 8 * count + 10 );

    parts->notify = notify;

    for ( i = 0; i < count; ++i ) {
        notify->name = SL_GetString_( namePtr, 0, 3 );
        namePtr += strlen( namePtr ) + 1;
        frame = *(unsigned short *)namePtr;
        namePtr += 2;

        notify->frameFrac = parts->frameCountMinusOne
                           ? (rfloat)frame / parts->frameCountMinusOne
                           : 0.0f;
        ++notify;
    }

    notify->name      = SL_GetString_( "end", 0, 3 );
    notify->frameFrac = 1.0f;
    ( notify + 1 )->name = 0;

    return namePtr;
}

typedef struct XAnimTrackHeader_s {
    void           *data;              /* +0x00 */
    unsigned short  keyCountMinusOne;  /* +0x04 */
    unsigned char   frameIndex[1];     /* +0x06  [keyCount], byte or word */
} XAnimTrackHeader;

typedef struct XAnimPartStreamPair_s {
    void *trans;   /* +0x00 */
    void *rot;     /* +0x04 */
} XAnimPartStreamPair;

/* ---- XAnimPrecacheAnimTree_m  0x004833A0 ----  VERIFIED */
/* Loads and unpacks xanim/<animName> into an XAnimParts record.  The tracks are
 * written through stream->rot / stream->trans (retail re-reads them after every
 * store), small rotations rebuild w = sqrt(32767^2 - z^2) inline, and quat is the
 * one 8-byte scratch every rotation key passes through (ReadQuat/ReadQuat2
 * output).  The two _alloca blocks are unused in retail too. */
void __cdecl XAnimPrecacheAnimTree_m( const char *animName, int (__cdecl *alloc)(int) )
{
    XAnimFileEntry *fileData = (XAnimFileEntry *)FS_GetDataForFile( "xanim", animName, "" );

    if ( !fileData ) {
        Com_Error( ERR_DROP, "\025Cannot precache 'xanim/%s'.", animName );
        return;
    }

    if ( !fileData->parts ) {
        char                 Buffer[1024];
        void                *Block;
        unsigned char       *pos;
        int                  version;
        unsigned short       numFrames;
        unsigned short       fileFrames;
        short                partCount;
        unsigned short      *partNameHandles;
        unsigned char        flags;
        short                frameRate;
        unsigned char        looped;
        unsigned char        hasDeltaMotion;
        unsigned char        smallIndices;
        int                  totalFrames;
        XAnimParts          *parts;
        XAnimPartStreamPair *stream;
        int                  bitsetBytes;
        const unsigned char *negateBits;
        unsigned char       *isSmallBits;
        unsigned short       rotCount;
        unsigned short       transCount;
        int                  partIndex;
        int                  i;
        int                  dist;
        short                quat[4];

        sprintf( Buffer, "xanim/%s", animName );
        if ( FS_ReadFile( Buffer, &Block ) < 0 ) {
            /* 0x0055A6F4 */
            Com_Error( ERR_DROP, "\025Cannot find 'xanim/%s'.", animName );
            return;
        }

        pos     = (unsigned char *)Block;
        version = *(short *)pos;
        pos    += 2;
        if ( version != 14 ) {
            FS_FreeFile( Block );
            /* 0x0055A6C0 */
            Com_Error( ERR_DROP, "\025xanim '%s' out of date (version %d, expecting %d)",
                       animName, version, 14 );
            return;
        }

        fileFrames = *(unsigned short *)pos;
        pos      += 2;
        partCount = *(short *)pos;
        pos      += 2;

        partNameHandles    = (unsigned short *)alloc( 2 * partCount + 2 );
        partNameHandles[0] = (unsigned short)partCount;

        flags          = *pos;
        pos           += 1;
        frameRate      = *(short *)pos;
        pos           += 2;
        looped         = flags & 1;
        hasDeltaMotion = ( flags >> 1 ) & 1;

        parts                  = (XAnimParts *)alloc( 32 );
        parts->hasDeltaMotion  = hasDeltaMotion;
        parts->frameRate       = (float)frameRate;
        parts->partNameHandles = partNameHandles;
        parts->looped          = looped;

        /* retail keeps the frame count in 16 bits (0x004834C1): a looped
           65535-frame anim wraps to 0 */
        if ( looped )
            numFrames = fileFrames + 1;
        else
            numFrames = fileFrames;
        smallIndices              = numFrames <= 0x100;
        totalFrames               = numFrames;
        parts->frameCountMinusOne = (unsigned short)( numFrames - 1 );
        parts->frequency = parts->frameCountMinusOne ? parts->frameRate / parts->frameCountMinusOne : 0.0f;

        if ( hasDeltaMotion ) {
            XAnimPartStreamPair *dm = (XAnimPartStreamPair *)alloc( 8 );

            parts->deltaMotion = dm;

            rotCount = *(unsigned short *)pos;
            pos     += 2;
            if ( !rotCount ) {
                dm->rot = 0;
            } else if ( rotCount == 1 ) {
                ReadQuat2( (const short *)pos, quat );
                pos += 2;
                dm->rot = (void *)alloc( 8 );
                ( (short *)dm->rot )[0] = quat[0];
                ( (short *)dm->rot )[1] = quat[1];
                ( (XAnimTrackHeader *)dm->rot )->keyCountMinusOne = 0;
            } else {
                if ( rotCount < numFrames ) {
                    if ( smallIndices ) {
                        dm->rot = (void *)alloc( rotCount + 7 );
                        memcpy( ( (XAnimTrackHeader *)dm->rot )->frameIndex, pos, rotCount );
                        pos += rotCount;
                    } else {
                        dm->rot = (void *)alloc( 2 * rotCount + 6 );
                        memcpy( ( (XAnimTrackHeader *)dm->rot )->frameIndex, pos, 2 * rotCount );
                        pos += 2 * rotCount;
                    }
                } else {
                    dm->rot = (void *)alloc( 8 );
                }
                ( (XAnimTrackHeader *)dm->rot )->data = (void *)alloc( 4 * rotCount );
                ReadQuat2( (const short *)pos, quat );
                ( (short (*)[2])( (XAnimTrackHeader *)dm->rot )->data )[0][0] = quat[0];
                pos += 2;
                ( (short (*)[2])( (XAnimTrackHeader *)dm->rot )->data )[0][1] = quat[1];
                for ( i = 1; i < rotCount; ++i ) {
                    ReadQuat2( (const short *)pos, quat );
                    ( (short (*)[2])( (XAnimTrackHeader *)dm->rot )->data )[i][0] = quat[0];
                    ( (short (*)[2])( (XAnimTrackHeader *)dm->rot )->data )[i][1] = quat[1];
                    pos += 2;
                }
                for ( i = 1; i < rotCount; ++i ) {
                    short (*data)[2] = (short (*)[2])( (XAnimTrackHeader *)dm->rot )->data;

                    if ( data[i][0] * data[i - 1][0] + data[i][1] * data[i - 1][1] < 0 ) {
                        data[i][0] = -data[i][0];
                        data[i][1] = -data[i][1];
                    }
                }
                ( (XAnimTrackHeader *)dm->rot )->keyCountMinusOne = rotCount - 1;
            }

            transCount = *(unsigned short *)pos;
            pos     += 2;
            if ( !transCount ) {
                dm->trans = 0;
            } else if ( transCount == 1 ) {
                /* 0x0048370D: the three floats move as raw dwords, read before alloc(16) */
                int v0 = ( (int *)pos )[0];
                int v1 = ( (int *)pos )[1];
                int v2 = ( (int *)pos )[2];

                pos += 12;
                dm->trans = (void *)alloc( 16 );
                ( (int *)dm->trans )[0] = v0;
                ( (int *)dm->trans )[2] = v1;
                ( (int *)dm->trans )[3] = v2;
                ( (XAnimTrackHeader *)dm->trans )->keyCountMinusOne = 0;
            } else {
                if ( transCount < numFrames ) {
                    if ( smallIndices ) {
                        dm->trans = (void *)alloc( transCount + 7 );
                        memcpy( ( (XAnimTrackHeader *)dm->trans )->frameIndex, pos, transCount );
                        pos += transCount;
                    } else {
                        dm->trans = (void *)alloc( 2 * transCount + 6 );
                        memcpy( ( (XAnimTrackHeader *)dm->trans )->frameIndex, pos, 2 * transCount );
                        pos += 2 * transCount;
                    }
                } else {
                    dm->trans = (void *)alloc( 8 );
                }
                ( (XAnimTrackHeader *)dm->trans )->data = (void *)alloc( 12 * transCount );
                for ( i = 0; i < transCount; ++i ) {
                    ( (int (*)[3])( (XAnimTrackHeader *)dm->trans )->data )[i][0] = ( (int *)pos )[0];
                    ( (int (*)[3])( (XAnimTrackHeader *)dm->trans )->data )[i][1] = ( (int *)pos )[1];
                    ( (int (*)[3])( (XAnimTrackHeader *)dm->trans )->data )[i][2] = ( (int *)pos )[2];
                    pos += 12;
                }
                ( (XAnimTrackHeader *)dm->trans )->keyCountMinusOne = transCount - 1;
            }
        }

        bitsetBytes = ( ( partCount - 1 ) >> 3 ) + 1;
        negateBits  = pos;
        pos        += bitsetBytes;
        isSmallBits = (unsigned char *)alloc( bitsetBytes );
        memcpy( isSmallBits, pos, bitsetBytes );
        pos += bitsetBytes;
        parts->compressedRotationBits = isSmallBits;

        for ( i = 0; i < partCount; ++i ) {
            unsigned int len = (unsigned int)strlen( (const char *)pos );

            /* 0x00483861: push 9 / push len+1 / push 0 / push name -- type 9. */
            partNameHandles[i + 1] = SL_GetStringOfLen( (const char *)pos, 0, len + 1, 9 );
            pos += len + 1;
        }

        _alloca( 8 * partCount );
        _alloca( 12 * partCount );
        parts->partStreams = (void *)alloc( 8 * partCount );

        for ( partIndex = 0; partIndex < partCount; ++partIndex ) {
            int           bit           = 1 << ( partIndex & 7 );
            unsigned char negateFirst   = ( negateBits[partIndex >> 3] & bit ) != 0;
            unsigned char smallRotation = ( isSmallBits[partIndex >> 3] & bit ) != 0;

            stream   = &( (XAnimPartStreamPair *)parts->partStreams )[partIndex];
            rotCount = *(unsigned short *)pos;
            pos     += 2;
            if ( !rotCount ) {
                stream->rot = 0;
            } else if ( rotCount == 1 ) {
                if ( smallRotation ) {
                    quat[0] = *(short *)pos;
                    dist    = 1073676289 - *(short *)pos * *(short *)pos;
                    quat[1] = dist > 0 ? (short)floor( sqrt( (double)dist ) + 0.5 ) : 0;
                    pos    += 2;
                    if ( negateFirst ) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                    }
                    stream->rot = (void *)alloc( 8 );
                    ( (short *)stream->rot )[0] = quat[0];
                    ( (short *)stream->rot )[1] = quat[1];
                    ( (XAnimTrackHeader *)stream->rot )->keyCountMinusOne = 0;
                } else {
                    ReadQuat( (const short *)pos, quat );
                    pos += 6;
                    if ( negateFirst ) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                        quat[2] = -quat[2];
                        quat[3] = -quat[3];
                    }
                    stream->rot = (void *)alloc( 10 );
                    ( (short *)stream->rot )[0] = quat[0];
                    ( (short *)stream->rot )[1] = quat[1];
                    *(int *)( (short *)stream->rot + 3 ) = *(int *)&quat[2];
                    ( (XAnimTrackHeader *)stream->rot )->keyCountMinusOne = 0;
                }
            } else {
                if ( rotCount < numFrames ) {
                    if ( smallIndices ) {
                        stream->rot = (void *)alloc( rotCount + 7 );
                        memcpy( ( (XAnimTrackHeader *)stream->rot )->frameIndex, pos, rotCount );
                        pos += rotCount;
                    } else {
                        stream->rot = (void *)alloc( 2 * rotCount + 6 );
                        memcpy( ( (XAnimTrackHeader *)stream->rot )->frameIndex, pos, 2 * rotCount );
                        pos += 2 * rotCount;
                    }
                } else {
                    stream->rot = (void *)alloc( 8 );
                }
                if ( smallRotation ) {
                    ( (XAnimTrackHeader *)stream->rot )->data = (void *)alloc( 4 * rotCount );
                    quat[0] = *(short *)pos;
                    dist    = 1073676289 - *(short *)pos * *(short *)pos;
                    quat[1] = dist > 0 ? (short)floor( sqrt( (double)dist ) + 0.5 ) : 0;
                    pos    += 2;
                    if ( negateFirst ) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                    }
                    ( (short (*)[2])( (XAnimTrackHeader *)stream->rot )->data )[0][0] = quat[0];
                    ( (short (*)[2])( (XAnimTrackHeader *)stream->rot )->data )[0][1] = quat[1];
                    for ( i = 1; i < rotCount; ++i ) {
                        quat[0] = *(short *)pos;
                        dist    = 1073676289 - *(short *)pos * *(short *)pos;
                        quat[1] = dist > 0 ? (short)floor( sqrt( (double)dist ) + 0.5 ) : 0;
                        ( (short (*)[2])( (XAnimTrackHeader *)stream->rot )->data )[i][0] = quat[0];
                        pos += 2;
                        ( (short (*)[2])( (XAnimTrackHeader *)stream->rot )->data )[i][1] = quat[1];
                    }
                    for ( i = 1; i < rotCount; ++i ) {
                        short (*data)[2] = (short (*)[2])( (XAnimTrackHeader *)stream->rot )->data;

                        if ( data[i - 1][0] * data[i][0] + data[i][1] * data[i - 1][1] < 0 ) {
                            data[i][0] = -data[i][0];
                            data[i][1] = -data[i][1];
                        }
                    }
                } else {
                    ( (XAnimTrackHeader *)stream->rot )->data = (void *)alloc( 8 * rotCount );
                    ReadQuat( (const short *)pos, quat );
                    pos += 6;
                    if ( negateFirst ) {
                        quat[0] = -quat[0];
                        quat[1] = -quat[1];
                        quat[2] = -quat[2];
                        quat[3] = -quat[3];
                    }
                    ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[0][0] = quat[0];
                    ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[0][1] = quat[1];
                    ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[0][2] = quat[2];
                    ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[0][3] = quat[3];
                    for ( i = 1; i < rotCount; ++i ) {
                        ReadQuat( (const short *)pos, quat );
                        ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[i][0] = quat[0];
                        ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[i][1] = quat[1];
                        ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[i][2] = quat[2];
                        ( (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data )[i][3] = quat[3];
                        pos += 6;
                    }
                    for ( i = 1; i < rotCount; ++i ) {
                        short (*data)[4] = (short (*)[4])( (XAnimTrackHeader *)stream->rot )->data;

                        if ( data[i - 1][0] * data[i][0] + data[i][3] * data[i - 1][3]
                           + data[i][2] * data[i - 1][2] + data[i][1] * data[i - 1][1] < 0 ) {
                            data[i][0] = -data[i][0];
                            data[i][1] = -data[i][1];
                            data[i][2] = -data[i][2];
                            data[i][3] = -data[i][3];
                        }
                    }
                }
                ( (XAnimTrackHeader *)stream->rot )->keyCountMinusOne = rotCount - 1;
            }

            transCount = *(unsigned short *)pos;
            pos     += 2;
            if ( !transCount ) {
                stream->trans = 0;
            } else if ( transCount == 1 ) {
                int v0 = ( (int *)pos )[0];
                int v1 = ( (int *)pos )[1];
                int v2 = ( (int *)pos )[2];

                pos += 12;
                stream->trans = (void *)alloc( 16 );
                ( (int *)stream->trans )[0] = v0;
                ( (int *)stream->trans )[2] = v1;
                ( (int *)stream->trans )[3] = v2;
                ( (XAnimTrackHeader *)stream->trans )->keyCountMinusOne = 0;
            } else {
                if ( transCount < numFrames ) {
                    if ( smallIndices ) {
                        stream->trans = (void *)alloc( transCount + 7 );
                        memcpy( ( (XAnimTrackHeader *)stream->trans )->frameIndex, pos, transCount );
                        pos += transCount;
                    } else {
                        stream->trans = (void *)alloc( 2 * transCount + 6 );
                        memcpy( ( (XAnimTrackHeader *)stream->trans )->frameIndex, pos, 2 * transCount );
                        pos += 2 * transCount;
                    }
                } else {
                    stream->trans = (void *)alloc( 8 );
                }
                ( (XAnimTrackHeader *)stream->trans )->data = (void *)alloc( 12 * transCount );
                for ( i = 0; i < transCount; ++i ) {
                    ( (int (*)[3])( (XAnimTrackHeader *)stream->trans )->data )[i][0] = ( (int *)pos )[0];
                    ( (int (*)[3])( (XAnimTrackHeader *)stream->trans )->data )[i][1] = ( (int *)pos )[1];
                    ( (int (*)[3])( (XAnimTrackHeader *)stream->trans )->data )[i][2] = ( (int *)pos )[2];
                    pos += 12;
                }
                ( (XAnimTrackHeader *)stream->trans )->keyCountMinusOne = transCount - 1;
            }
        }

        ReadNoteTracks( pos, (int)animName, parts, alloc );

        FS_FreeFile( Block );

        fileData->parts    = parts;
        fileData->freeData = XAnimFreeMemory;
    }
}

/* ---- XAnimSetLeafNode  0x00483F80 ----  VERIFIED */
void __cdecl XAnimSetLeafNode(const char *animName, XAnim *tree, unsigned __int16 index)
{
  void *result;
  XAnimEntry *entry;

  result = (void *)FS_GetDataForFile("xanim", animName, "");
  if ( !result ) {
    Com_Error(ERR_DROP, "\025Cannot find 'xanim/%s'", animName);      /* 0x0055A6A8 */
    return;
  }
  entry = &tree->entries[index];
  entry->childCount = 0;
  entry->u.leafAsset = result;
}

/* ---- XAnimLoadFileData_m  no-address ----  VERIFIED */
void __cdecl XAnimLoadFileData_m(const char *animName, XAnim *tree, unsigned __int16 index)
{
  XAnimSetLeafNode(animName, tree, index);
}

/* ---- XAnimSetParentNode  0x00483FC0 ----  VERIFIED */
unsigned __int16 *__cdecl XAnimSetParentNode(
        unsigned short childCount,
        unsigned short flags,
        XAnim *tree,
        unsigned short index,
        int unused,
        unsigned short firstChildIndex )
{
    XAnimEntry     *entry  = &tree->entries[index];
    unsigned short *result = (unsigned short *)entry;
    int             i;

    entry->childCount               = childCount;
    entry->u.parent.flags           = flags;
    entry->u.parent.firstChildIndex = firstChildIndex;

    for ( i = 0; i < (int)childCount; ++i ) {
        int childSlot = i + entry->u.parent.firstChildIndex;

        tree->entries[childSlot].parentIndex = index;
    }
    return result;
}

#ifdef __cplusplus
}
#endif
