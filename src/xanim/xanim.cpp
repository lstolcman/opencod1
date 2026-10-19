/*
 * Machine-translated from Call of Duty 1.1 (Windows, CoDMP.exe).
 * Original translation unit:
 * /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/xanim/xanim.cpp
 * Retail range 0x00484000-0x00487F00, 56 functions.
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"
#include <malloc.h>

#include "xanim_local.h"
/* ---- module-owned globals ---- */
float xanim_evalTimeStep;   /* 0x00A9CC70 */
float xanim_evalWindowTime;
int xanim_evalPartBytes;   /* 0x00A7A61C */
int xanim_evalChildCount;
int xanim_evalChildRefs;
int xanim_evalDObj;
int xanim_evalPartCount;
int xanim_evalPosedBits[4];
int xanim_evalSkipBits[4];
unsigned short xanim_evalCurrentFrame;
unsigned short xanim_evalStartFrame;
unsigned short xanim_evalWindowFrame;
unsigned short xanim_notifyEntId;
int xanim_endNotifyStringHandle;   /* 0x01407338 */
int xanim_evalPoolWeightSelector;   /* 0x00A9C620 */
unsigned char xanim_evalLeafOutputMode;   /* 0x00A9C63C */
int xanim_currentTree;   /* 0x00A7A618 */
int xanim_numDeferredNotifies;   /* 0x00A9C638 */
int xanim_activePoolSlot;   /* 0x00A9CC58, XAnim pool selector (qcommon.h) */
int xanim_currentTime;   /* 0x00A9CC5C */


float xanim_evalCurrentTime;   /* 0x00A7A614 */
float xanim_evalStartTime;   /* 0x00A9CC7C */

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

extern XAnimInfo xanim_pool[2048];

#define XANIM_LANE(h)   ( xanim_pool[h].states[xanim_activePoolSlot] )

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

typedef struct XAnimDeferredNotify_s {
    const char    *name;         /* +0x00 */
    unsigned short notifyHandle; /* +0x04 */
    unsigned short pad06;
    float          timeFrac;     /* +0x08 */
} XAnimDeferredNotify;

XAnimDeferredNotify xanim_deferredNotifies[128];

extern int IncInParam__Fv();
extern int SL_FreeString__FUsPCcUi();
extern void __cdecl Scr_AddConstString( unsigned short handle );
extern const char *SL_ConvertToString( unsigned short handle );
extern unsigned short SL_GetStringOfLen( const char *text, unsigned char user,
                                         unsigned int size, int type );
extern void        SL_RemoveRefToStringOfLen( unsigned short handle,
                                              unsigned int size );
extern void        SL_RemoveRefToString( unsigned short handle );
extern void        SL_AddRefToString( unsigned short handle );
extern void __cdecl Scr_NotifyId( unsigned short id, unsigned short name,
                                  unsigned int paramCount );

typedef struct XAnimModelPartNameTable_s {
    short          count;                    /* +0x00  SIGNED (movsx) */
    unsigned short handles[1];               /* +0x02 */
} XAnimModelPartNameTable;

#define XANIM_REMAP_PREFIX          16
#define XANIM_REMAP_SENTINEL        127
#define XANIM_REMAP_STRING_TYPE     11

/* ---- XAnimAllocTree  0x00484000 ----  VERIFIED */
XAnim *__cdecl XAnimAllocTree( int nodeCount, const char *name, void *(__cdecl *alloc)(int) )
{
    XAnim *tree = (XAnim *)alloc( 8 * nodeCount + 8 );

    tree->name      = name;
    tree->nodeCount = nodeCount;
    return tree;
}

/*
 * Runtime-tree creation path:
 *
 *   cgame trap 131         one argument, the source tree
 *   engine case 131        0x00403783 `mov ebx, [esi+4]` then
 *                          push Hunk_AllocXAnimClientCreateTree -- args[1] is
 *                          the source tree
 *   XAnimAllocTree         0x00484000  8n+8, name at +0, nodeCount at +4
 *   XAnimAllocRuntimeTree  0x00484020  (2n+8) + 2*(3n+1) = 8n+10, Com_Memset
 *                          of the whole block, sourceTree stored at +0
 *   XAnimCopyRuntimeTree   0x00484050  same 8n+10 handed to the copy callback
 *   Hunk_AllocXAnimClientCreate / ...CreateTree  0x00401CE0 / 0x00401CF0 are
 *                          byte-identical in retail -- `mov eax,20h;
 *                          mov ecx,[esp+size]; jmp Hunk_AllocAlignInternal`
 *   Hunk_AllocAlignInternal  size first, align second, high PERMANENT hunk,
 *                          so a tree is not reclaimed under the client
 */

/* ---- XAnimAllocRuntimeTree  0x00484020 ----  VERIFIED */
XAnimTree *__cdecl XAnimAllocRuntimeTree( XAnim *sourceTree, void *(__cdecl *alloc)(int) )
{
    int n = sourceTree->nodeCount;
    struct {
        int headerSize;
        int remapCount;
    } layout;   /* a struct local: VC7 then keeps 2n+8 and 3n+1 apart as retail does */
    int size;
    XAnimTree *tree;

    layout.headerSize = 2 * n + 8;
    layout.remapCount = 3 * n + 1;
    size = layout.headerSize + 2 * layout.remapCount;
    tree = (XAnimTree *)alloc( size );

    Com_Memset( tree, 0, size );
    tree->sourceTree = sourceTree;
    return tree;
}

/* ---- XAnimCopyRuntimeTree  0x00484050 ----  VERIFIED */
int __cdecl XAnimCopyRuntimeTree(XAnimTree *tree, int (__cdecl *copy)(XAnimTree *, int))
{
  int n = tree->sourceTree->nodeCount;
  struct {
    int headerSize;
    int remapCount;
  } layout;   /* see XAnimAllocRuntimeTree */

  layout.headerSize = 2 * n + 8;
  layout.remapCount = 3 * n + 1;
  return copy(tree, layout.headerSize + 2 * layout.remapCount);
}

/* ---- XAnimRuntimeTreeSourceTree  0x00484070 ----  VERIFIED */
XAnim *__cdecl XAnimRuntimeTreeSourceTree( XAnimTree *tree )
{
    return tree->sourceTree;
}

/* ---- XAnimSetModel  0x00484080 ----  VERIFIED */
__int16 __cdecl XAnimSetModel(int a1, int a2, int a3)
{
    unsigned char        *remap;
    const unsigned short *animPartNameTable;
    const unsigned short *animPartNames;
    int                   animPartCount;
    unsigned int          remapSize;
    int                   sourcePart;
    int                   modelIndex;
    int                   i;

    /* entry->u.leafAsset is a fileData_t whose +0x04 is the XAnimParts. */
    animPartNameTable =
        ((XAnimParts *)( *(void **)( *(int *)( a1 + 4 ) + 4 ) ))->partNameHandles;
    animPartCount = *(const short *)animPartNameTable;
    animPartNames = animPartNameTable + 1;
    remapSize     = (unsigned int)( animPartCount + XANIM_REMAP_PREFIX );
    /* 0x484098-0x4840A6 lea edi,[ebx+10h]; add 3; and ~3; __alloca_probe:
     * the buffer is sized by the part count, with no fixed upper bound. */
    remap = (unsigned char *)_alloca( remapSize );

    memset( remap, 0, XANIM_REMAP_PREFIX );
    for ( i = animPartCount - 1; i >= 0; --i ) {
        remap[XANIM_REMAP_PREFIX + i] = XANIM_REMAP_SENTINEL;
    }

    sourcePart = 0;
    for ( modelIndex = 0; modelIndex < a3; ++modelIndex ) {
        const XAnimModelPartNameTable *modelParts;
        int                            modelPartCount;
        int                            modelPart;
        const unsigned short          *handles;

        modelParts = ***(XAnimModelPartNameTable ****)
                        ( **(int **)( *(int *)( a2 + 4 * modelIndex ) + 4 ) + 4 );
        modelPartCount = modelParts->count;
        /* a loop-invariant base local keeps the indexed [base+modelPart*2] load
         * (retail does not strength-reduce this loop to a pointer walk) */
        handles        = modelParts->handles;

        for ( modelPart = 0; modelPart < modelPartCount;
              ++modelPart, ++sourcePart ) {
            unsigned short modelPartName = handles[modelPart];
            int            animPart;

            for ( animPart = animPartCount - 1; animPart >= 0; --animPart ) {
                if ( modelPartName != animPartNames[animPart] ) {
                    continue;
                }
                if ( remap[XANIM_REMAP_PREFIX + animPart] == XANIM_REMAP_SENTINEL ) {
                    remap[XANIM_REMAP_PREFIX + animPart] = (unsigned char)sourcePart;
                    remap[sourcePart >> 3] |= 1 << ( sourcePart & 7 );
                }
                break;
            }
        }
    }

    return (__int16)SL_GetStringOfLen( (const char *)remap, 0, remapSize,
                                       XANIM_REMAP_STRING_TYPE );
}

/* ---- XAnimFindByteKey  0x00484180 ----  VERIFIED */
int __cdecl XAnimFindByteKey( float frameFrac, const unsigned char *keys, int targetKey, int keyCount )
{
    /* keyCount doubles as the upper search bound */
    int lo  = 0;
    int mid = (int)( (rfloat)keyCount * frameFrac );

    if ( targetKey < keys[mid] ) {
        keyCount = mid;
        for ( ;; ) {
            mid = ( lo + keyCount ) / 2;
            if ( targetKey < keys[mid] ) {
                keyCount = mid;
                continue;
            }
            if ( targetKey < keys[mid + 1] )
                return mid;
            --keyCount;
            lo = mid + 1;
            if ( targetKey >= keys[keyCount] )
                return keyCount;
        }
    }

    if ( targetKey < keys[mid + 1] )
        return mid;
    lo = mid + 1;
    for ( ;; ) {
        mid = ( lo + keyCount ) / 2;
        if ( targetKey < keys[mid] ) {
            ++lo;
            keyCount = mid;
            if ( targetKey < keys[lo] )
                return lo - 1;
            continue;
        }
        if ( targetKey < keys[mid + 1] )
            return mid;
        lo = mid + 1;
    }
}

/* ---- XAnimFindShortKey  0x00484200 ----  VERIFIED */
int __cdecl XAnimFindShortKey( float frameFrac, const unsigned short *keys, int targetKey, int keyCount )
{
    /* keyCount doubles as the upper search bound */
    int lo  = 0;
    int mid = (int)( (rfloat)keyCount * frameFrac );

    if ( targetKey < keys[mid] ) {
        keyCount = mid;
        for ( ;; ) {
            mid = ( lo + keyCount ) / 2;
            if ( targetKey < keys[mid] ) {
                keyCount = mid;
                continue;
            }
            if ( targetKey < keys[mid + 1] )
                return mid;
            --keyCount;
            lo = mid + 1;
            if ( targetKey >= keys[keyCount] )
                return keyCount;
        }
    }

    if ( targetKey < keys[mid + 1] )
        return mid;
    lo = mid + 1;
    for ( ;; ) {
        mid = ( lo + keyCount ) / 2;
        if ( targetKey < keys[mid] ) {
            ++lo;
            keyCount = mid;
            if ( targetKey < keys[lo] )
                return lo - 1;
            continue;
        }
        if ( targetKey < keys[mid + 1] )
            return mid;
        lo = mid + 1;
    }
}

/* ---- XAnimCalcPartsSmallIndices  0x00484280 ---- */
typedef struct XAnimPartStream_s {
    void  *transStream;   /* +0x00  vec3 float keys, or a single static vec3 */
    __int16 *rotStream;   /* +0x04  int16 quat keys, or a single static pair/quad */
} XAnimPartStream;

typedef struct XAnimDeltaKeys_s {
    void          *frames;   /* +0x00  keyed frames; a constant part keeps its value here inline */
    unsigned short size;     /* +0x04  last key index, 0 == constant */
    unsigned char  keys[2];  /* +0x06  frame numbers: bytes, or shorts when numFrames >= 256 */
} XAnimDeltaKeys;

typedef struct XAnimDeltaPart_s {
    XAnimDeltaKeys *trans;   /* +0x00  float[3] frames */
    XAnimDeltaKeys *quat;    /* +0x04  short[2] frames */
} XAnimDeltaPart;

/* ---- XAnimCalcPartsSmallIndices  0x00484280 ----  VERIFIED */
/* C++ unit: the counts are int locals (retail movzx/32-bit compares; an unsigned
 * short local stays 16-bit under /TP) and the bit tables are plain (signed) char.
 * Keys are read through the XAnimDeltaKeys member (retail re-derives quat+6 after
 * each XAnimFindByteKey call instead of keeping a keys pointer). */
static __inline rfloat XAnimInterpolateQuatComponent( int current, int next, float frac ) { return (next-current)*frac+current; }

void __cdecl XAnimCalcPartsSmallIndices( const XAnimParts *parts, const unsigned char *remap,
                        float frameFrac, float weight, DObjAnimMat *matArray )
{
    float       rotScale      = weight * 0.000030518509f;
    int         numFrames     = parts->frameCountMinusOne;
    float       time          = numFrames * frameFrac;
    int         frameIndex    = (int)time;
    const char *rot2Bits      = (const char *)parts->compressedRotationBits;
    int         animPartCount = *(const short *)parts->partNameHandles;
    int         partIndex;

    /* taking the address keeps the declared stack-argument convention under LTCG */
    (void)&XAnimCalcPartsSmallIndices;

    for ( partIndex = 0; partIndex < animPartCount; ++partIndex )
    {
        int                   bone = remap[partIndex];
        const XAnimDeltaPart *part;
        const XAnimDeltaKeys *quat;
        const XAnimDeltaKeys *trans;
        DObjAnimMat          *mat;

        if ( ((const char *)xanim_evalSkipBits)[bone >> 3] & (1 << (bone & 7)) )
            continue;

        part = &((const XAnimDeltaPart *)parts->partStreams)[partIndex];
        quat = part->quat;
        mat  = &matArray[bone];

        if ( rot2Bits[partIndex >> 3] & (1 << (partIndex & 7)) )
        {
            /* only z and w are stored */
            if ( quat )
            {
                int quatSize = quat->size;

                if ( !quatSize )
                {
                    mat->quat[2] += ( (const short *)&quat->frames )[0] * rotScale;
                    mat->quat[3] += ( (const short *)&quat->frames )[1] * rotScale;
                }
                else
                {
                    const short (*frames)[2];
                    int           key;
                    float         keyFrac;

                    if ( quatSize < numFrames )
                    {
                        key     = XAnimFindByteKey( frameFrac, quat->keys, frameIndex, quatSize );
                        keyFrac = ( time - quat->keys[key] ) / ( quat->keys[key + 1] - quat->keys[key] );
                    }
                    else
                    {
                        key     = frameIndex;
                        keyFrac = time - frameIndex;
                    }
                    frames = (const short (*)[2])quat->frames;
                    mat->quat[2] += ( XAnimInterpolateQuatComponent(frames[key][0],frames[key+1][0],keyFrac) ) * rotScale;
                    mat->quat[3] += ( XAnimInterpolateQuatComponent(frames[key][1],frames[key+1][1],keyFrac) ) * rotScale;
                }
            }
            else
            {
                mat->quat[3] += weight;
            }
        }
        else
        {
            int quatSize = quat->size;

            if ( !quatSize )
            {
                mat->quat[0] += ( (const short *)&quat->frames )[0] * rotScale;
                mat->quat[1] += ( (const short *)&quat->frames )[1] * rotScale;
                mat->quat[2] += ( (const short *)&quat->frames )[3] * rotScale;
                mat->quat[3] += ( (const short *)&quat->frames )[4] * rotScale;
            }
            else
            {
                const short (*frames)[4];
                int           key;
                float         keyFrac;

                if ( quatSize < numFrames )
                {
                    key     = XAnimFindByteKey( frameFrac, quat->keys, frameIndex, quatSize );
                    keyFrac = ( time - quat->keys[key] ) / ( quat->keys[key + 1] - quat->keys[key] );
                }
                else
                {
                    key     = frameIndex;
                    keyFrac = time - frameIndex;
                }
                frames = (const short (*)[4])quat->frames;
                mat->quat[0] += ( XAnimInterpolateQuatComponent(frames[key][0],frames[key+1][0],keyFrac) ) * rotScale;
                mat->quat[1] += ( XAnimInterpolateQuatComponent(frames[key][1],frames[key+1][1],keyFrac) ) * rotScale;
                mat->quat[2] += ( XAnimInterpolateQuatComponent(frames[key][2],frames[key+1][2],keyFrac) ) * rotScale;
                mat->quat[3] += ( XAnimInterpolateQuatComponent(frames[key][3],frames[key+1][3],keyFrac) ) * rotScale;
            }
        }

        trans = part->trans;
        if ( trans )
        {
            int transSize = trans->size;

            if ( !transSize )
            {
                /* static vec3 at +0x00,+0x08,+0x0C -- skipping the +0x04 size slot */
                mat->trans[0] += weight * ( (const float *)trans )[0];
                mat->trans[1] += weight * ( (const float *)trans )[2];
                mat->trans[2] += weight * ( (const float *)trans )[3];
            }
            else
            {
                const float (*frames)[3];
                int           key;
                float         keyFrac;

                if ( transSize < numFrames )
                {
                    key     = XAnimFindByteKey( frameFrac, trans->keys, frameIndex, transSize );
                    keyFrac = ( time - trans->keys[key] ) / ( trans->keys[key + 1] - trans->keys[key] );
                }
                else
                {
                    key     = frameIndex;
                    keyFrac = time - frameIndex;
                }
                frames = (const float (*)[3])trans->frames;
                mat->trans[0] += ( ( frames[key + 1][0] - frames[key][0] ) * keyFrac + frames[key][0] ) * weight;
                mat->trans[1] += ( ( frames[key + 1][1] - frames[key][1] ) * keyFrac + frames[key][1] ) * weight;
                mat->trans[2] += ( ( frames[key + 1][2] - frames[key][2] ) * keyFrac + frames[key][2] ) * weight;
            }
        }

        mat->weight += weight;
    }
}

/* ---- XAnimCalcPartsLargeIndices  0x00484670 ----  VERIFIED */
/* C++ unit: the counts are int locals (retail movzx/32-bit compares; an unsigned
 * short local stays 16-bit under /TP) and the bit tables are plain (signed) char.
 * Keys are read through the XAnimDeltaKeys member (retail re-derives quat+6 after
 * each XAnimFindShortKey call instead of keeping a keys pointer). */
void __cdecl XAnimCalcPartsLargeIndices( const XAnimParts *parts, const unsigned char *remap,
                        float frameFrac, float weight, DObjAnimMat *matArray )
{
    float       rotScale      = weight * 0.000030518509f;
    int         numFrames     = parts->frameCountMinusOne;
    float       time          = numFrames * frameFrac;
    int         frameIndex    = (int)time;
    const char *rot2Bits      = (const char *)parts->compressedRotationBits;
    int         animPartCount = *(const short *)parts->partNameHandles;
    int         partIndex;

    /* taking the address keeps the declared stack-argument convention under LTCG */
    (void)&XAnimCalcPartsLargeIndices;

    for ( partIndex = 0; partIndex < animPartCount; ++partIndex )
    {
        int                   bone = remap[partIndex];
        const XAnimDeltaPart *part;
        const XAnimDeltaKeys *quat;
        const XAnimDeltaKeys *trans;
        DObjAnimMat          *mat;

        if ( ((const char *)xanim_evalSkipBits)[bone >> 3] & (1 << (bone & 7)) )
            continue;

        part = &((const XAnimDeltaPart *)parts->partStreams)[partIndex];
        quat = part->quat;
        mat  = &matArray[bone];

        if ( rot2Bits[partIndex >> 3] & (1 << (partIndex & 7)) )
        {
            /* only z and w are stored */
            if ( quat )
            {
                int quatSize = quat->size;

                if ( !quatSize )
                {
                    mat->quat[2] += ( (const short *)&quat->frames )[0] * rotScale;
                    mat->quat[3] += ( (const short *)&quat->frames )[1] * rotScale;
                }
                else
                {
                    const short (*frames)[2];
                    int           key;
                    float         keyFrac;

                    if ( quatSize < numFrames )
                    {
                        key     = XAnimFindShortKey( frameFrac, (const unsigned short *)quat->keys, frameIndex, quatSize );
                        keyFrac = ( time - ( (const unsigned short *)quat->keys )[key] ) / ( ( (const unsigned short *)quat->keys )[key + 1] - ( (const unsigned short *)quat->keys )[key] );
                    }
                    else
                    {
                        key     = frameIndex;
                        keyFrac = time - frameIndex;
                    }
                    frames = (const short (*)[2])quat->frames;
                    mat->quat[2] += ( XAnimInterpolateQuatComponent(frames[key][0],frames[key+1][0],keyFrac) ) * rotScale;
                    mat->quat[3] += ( XAnimInterpolateQuatComponent(frames[key][1],frames[key+1][1],keyFrac) ) * rotScale;
                }
            }
            else
            {
                mat->quat[3] += weight;
            }
        }
        else
        {
            int quatSize = quat->size;

            if ( !quatSize )
            {
                mat->quat[0] += ( (const short *)&quat->frames )[0] * rotScale;
                mat->quat[1] += ( (const short *)&quat->frames )[1] * rotScale;
                mat->quat[2] += ( (const short *)&quat->frames )[3] * rotScale;
                mat->quat[3] += ( (const short *)&quat->frames )[4] * rotScale;
            }
            else
            {
                const short (*frames)[4];
                int           key;
                float         keyFrac;

                if ( quatSize < numFrames )
                {
                    key     = XAnimFindShortKey( frameFrac, (const unsigned short *)quat->keys, frameIndex, quatSize );
                    keyFrac = ( time - ( (const unsigned short *)quat->keys )[key] ) / ( ( (const unsigned short *)quat->keys )[key + 1] - ( (const unsigned short *)quat->keys )[key] );
                }
                else
                {
                    key     = frameIndex;
                    keyFrac = time - frameIndex;
                }
                frames = (const short (*)[4])quat->frames;
                mat->quat[0] += ( XAnimInterpolateQuatComponent(frames[key][0],frames[key+1][0],keyFrac) ) * rotScale;
                mat->quat[1] += ( XAnimInterpolateQuatComponent(frames[key][1],frames[key+1][1],keyFrac) ) * rotScale;
                mat->quat[2] += ( XAnimInterpolateQuatComponent(frames[key][2],frames[key+1][2],keyFrac) ) * rotScale;
                mat->quat[3] += ( XAnimInterpolateQuatComponent(frames[key][3],frames[key+1][3],keyFrac) ) * rotScale;
            }
        }

        trans = part->trans;
        if ( trans )
        {
            int transSize = trans->size;

            if ( !transSize )
            {
                /* static vec3 at +0x00,+0x08,+0x0C -- skipping the +0x04 size slot */
                mat->trans[0] += weight * ( (const float *)trans )[0];
                mat->trans[1] += weight * ( (const float *)trans )[2];
                mat->trans[2] += weight * ( (const float *)trans )[3];
            }
            else
            {
                const float (*frames)[3];
                int           key;
                float         keyFrac;

                if ( transSize < numFrames )
                {
                    key     = XAnimFindShortKey( frameFrac, (const unsigned short *)trans->keys, frameIndex, transSize );
                    keyFrac = ( time - ( (const unsigned short *)trans->keys )[key] ) / ( ( (const unsigned short *)trans->keys )[key + 1] - ( (const unsigned short *)trans->keys )[key] );
                }
                else
                {
                    key     = frameIndex;
                    keyFrac = time - frameIndex;
                }
                frames = (const float (*)[3])trans->frames;
                mat->trans[0] += ( ( frames[key + 1][0] - frames[key][0] ) * keyFrac + frames[key][0] ) * weight;
                mat->trans[1] += ( ( frames[key + 1][1] - frames[key][1] ) * keyFrac + frames[key][1] ) * weight;
                mat->trans[2] += ( ( frames[key + 1][2] - frames[key][2] ) * keyFrac + frames[key][2] ) * weight;
            }
        }

        mat->weight += weight;
    }
}

/* ---- XAnimCalcNonLoopEnd  0x00484A60 ----  VERIFIED */
void __cdecl XAnimCalcNonLoopEnd( XAnimParts *parts, const unsigned char *remap, float weight, DObjAnimMat *matArray )
{
    float rotScale    = weight * 0.000030518509f;
    int   animPartCount = *(const short *)parts->partNameHandles;
    const char *rot2Bits=(const char*)parts->compressedRotationBits;
    
    int   i;

    for ( i = 0; i < animPartCount; ++i )
    {
        int bone = remap[i];

        if ( ( (1 << (bone & 7)) & *((const char *)xanim_evalSkipBits + (bone >> 3)) ) != 0 )
            continue;

        {
            const XAnimPartStream *stream = &((const XAnimPartStream*)parts->partStreams)[i];
            const __int16         *rot    = stream->rotStream;
            DObjAnimMat            *mat   = (DObjAnimMat *)( (char *)matArray + 32 * bone );
            rfloat                  wRaw;
            rfloat                  wDelta;   /* x87 st0, never stored as float:
                                               * 0x484BF6 fmul rotScale; fadd quat[3]; fstp */


            if ( rot2Bits[i >> 3] & (1 << (i & 7)) )
            {
                if ( rot )
                {
                    unsigned short keyCount = (unsigned short)rot[2];

                    if ( !keyCount )
                    {
                        mat->quat[2] += (double)rot[0] * rotScale;
                        wRaw = rot[1];
                    }
                    else
                    {
                        const __int16 *data = (const __int16 *)( *(const int *)rot + 4 * keyCount );
                        mat->quat[2] += (double)data[0] * rotScale;
                        wRaw = data[1];
                    }
                    wDelta = wRaw * rotScale;
                }
                else
                {
                    wDelta = weight;
                }
            }
            else
            {
                unsigned short keyCount = (unsigned short)rot[2];

                if ( !keyCount )
                {
                    mat->quat[0] += (double)rot[0] * rotScale;
                    mat->quat[1] += (double)rot[1] * rotScale;
                    mat->quat[2] += (double)rot[3] * rotScale;
                    wRaw = rot[4];
                }
                else
                {
                    const __int16 *data = (const __int16 *)( *(const int *)rot + 8 * keyCount );
                    mat->quat[0] += (double)data[0] * rotScale;
                    mat->quat[1] += (double)data[1] * rotScale;
                    mat->quat[2] += (double)data[2] * rotScale;
                    wRaw = data[3];
                }
                wDelta = wRaw * rotScale;
            }

            mat->quat[3] += wDelta;

            {
                void *trans = stream->transStream;

                if ( trans )
                {
                    unsigned short keyCount = *(const unsigned short *)( (char *)trans + 4 );

                    if ( !keyCount )
                    {
                        mat->trans[0] += weight * *(const float *)( (char *)trans + 0 );
                        mat->trans[1] += weight * *(const float *)( (char *)trans + 8 );
                        mat->trans[2] += weight * *(const float *)( (char *)trans + 12 );
                    }
                    else
                    {
                        const float *data = (const float *)( *(const int *)trans + 12 * keyCount );
                        mat->trans[0] += weight * data[0];
                        mat->trans[1] += weight * data[1];
                        mat->trans[2] += weight * data[2];
                    }
                }
                mat->weight += weight;
            }
        }
    }
}

/* ---- XAnimClearData  0x00484C90 ----  VERIFIED */
DObjAnimMat *__cdecl XAnimClearData( DObjAnimMat *mats )
{
    int partCount = xanim_evalPartCount;
    int i;

    for ( i = 0; i < partCount; ++i, ++mats )
    {
        if ( ( (1 << (i & 7)) & *((const char *)xanim_evalSkipBits + (i >> 3)) ) == 0 )
        {
            mats->quat[0]  = 0.0f;
            mats->quat[1]  = 0.0f;
            mats->quat[2]  = 0.0f;
            mats->quat[3]  = 0.0f;
            mats->weight   = 0.0f;
            mats->trans[2] = 0.0f;
            mats->trans[1] = 0.0f;
            mats->trans[0] = 0.0f;
        }
    }
    return mats;
}

/* ---- XAnimCalcData  0x00484CE0 ----  VERIFIED */
void __cdecl XAnimCalcData( unsigned int *remapBitset, DObjAnimMat *animMats,
                        void *leafAsset, float weight, float frameFrac )
{
    XAnimParts    *parts;
 unsigned int bits;


    bits=remapBitset[0];
 bits &= ~xanim_evalSkipBits[0];
 xanim_evalPosedBits[0] |= bits;
    bits=remapBitset[1];
 bits &= ~xanim_evalSkipBits[1];
 xanim_evalPosedBits[1] |= bits;
    bits=remapBitset[2];
 bits &= ~xanim_evalSkipBits[2];
 xanim_evalPosedBits[2] |= bits;
    bits=remapBitset[3];
 bits &= ~xanim_evalSkipBits[3];
 xanim_evalPosedBits[3] |= bits;

    parts = *(XAnimParts **)( (char *)leafAsset + 4 );

    if ( frameFrac == 1.0f || !parts->frameCountMinusOne ) {
        XAnimCalcNonLoopEnd( parts, (unsigned char*)remapBitset+16, weight, animMats ); return; }
    if ( parts->frameCountMinusOne < 0x100u ) {
        XAnimCalcPartsSmallIndices( parts, (unsigned char*)remapBitset+16, frameFrac, weight, animMats ); return; }
    XAnimCalcPartsLargeIndices( parts, (unsigned char*)remapBitset+16, frameFrac, weight, animMats );
}


/* Byte- and short-keyed samplers that retail inlines into XAnimCalcDeltaParts. */
static __forceinline void XAnimCalcDeltaPartsByteKeys( const XAnimDeltaPart *deltaPart, int numFrames,
                                                       float frameFrac, int frameIndex, float time,
                                                       float *rotDelta, float *transDelta )
{
    const XAnimDeltaKeys *quat;
    const XAnimDeltaKeys *trans;
    int                   quatSize;
    int                   quatKey;
    rfloat                quatFrac;
    int                   transSize;
    int                   transKey;
    rfloat                transFrac;
    const short         (*quatFrames)[2];
    const float         (*transFrames)[3];

    quat = deltaPart->quat;
    if ( quat ) {
        quatSize = quat->size;
        if ( !quatSize ) {
            rotDelta[0] = ( (const short *)&quat->frames )[0];
            rotDelta[1] = ( (const short *)&quat->frames )[1];
        } else {
            if ( quatSize < numFrames ) {
                quatKey  = XAnimFindByteKey( time, quat->keys, frameIndex, quatSize );
                quatFrac = ( (rfloat)frameFrac - quat->keys[quatKey] )
                         / ( quat->keys[quatKey + 1] - quat->keys[quatKey] );
            } else {
                quatKey  = frameIndex;
                quatFrac = (rfloat)frameFrac - frameIndex;
            }
            quatFrames  = (const short (*)[2])quat->frames;
            rotDelta[0] = ( quatFrames[quatKey + 1][0] - quatFrames[quatKey][0] ) * quatFrac
                        + quatFrames[quatKey][0];
            rotDelta[1] = ( quatFrames[quatKey + 1][1] - quatFrames[quatKey][1] ) * quatFrac
                        + quatFrames[quatKey][1];
        }
    } else {
        rotDelta[0] = 0.0f;
        rotDelta[1] = 32767.0f;
    }

    trans = deltaPart->trans;
    if ( trans ) {
        transSize = trans->size;
        if ( !transSize ) {
            transDelta[0] = ( (const float *)trans )[0];
            transDelta[1] = ( (const float *)trans )[2];
            transDelta[2] = ( (const float *)trans )[3];
        } else {
            if ( transSize < numFrames ) {
                transKey  = XAnimFindByteKey( time, trans->keys, frameIndex, transSize );
                transFrac = ( (rfloat)frameFrac - trans->keys[transKey] )
                          / ( trans->keys[transKey + 1] - trans->keys[transKey] );
            } else {
                transKey  = frameIndex;
                transFrac = (rfloat)frameFrac - frameIndex;
            }
            transFrames   = (const float (*)[3])trans->frames;
            transDelta[0] = ( transFrames[transKey + 1][0] - transFrames[transKey][0] ) * transFrac
                          + transFrames[transKey][0];
            transDelta[1] = ( transFrames[transKey + 1][1] - transFrames[transKey][1] ) * transFrac
                          + transFrames[transKey][1];
            transDelta[2] = ( transFrames[transKey + 1][2] - transFrames[transKey][2] ) * transFrac
                          + transFrames[transKey][2];
        }
    } else {
        transDelta[0] = transDelta[1] = transDelta[2] = 0.0f;
    }
}

static __forceinline void XAnimCalcDeltaPartsShortKeys( const XAnimDeltaPart *deltaPart, int numFrames,
                                                        float frameFrac, int frameIndex, float time,
                                                        float *rotDelta, float *transDelta )
{
    const XAnimDeltaKeys *quat;
    const XAnimDeltaKeys *trans;
    int                   quatSize;
    int                   quatKey;
    rfloat                quatFrac;
    int                   transSize;
    int                   transKey;
    rfloat                transFrac;
    const short         (*quatFrames)[2];
    const float         (*transFrames)[3];

    quat = deltaPart->quat;
    if ( quat ) {
        quatSize = quat->size;
        if ( !quatSize ) {
            rotDelta[0] = ( (const short *)&quat->frames )[0];
            rotDelta[1] = ( (const short *)&quat->frames )[1];
        } else {
            if ( quatSize < numFrames ) {
                quatKey  = XAnimFindShortKey( time, (const unsigned short *)quat->keys, frameIndex, quatSize );
                quatFrac = ( (rfloat)frameFrac - ( (const unsigned short *)quat->keys )[quatKey] )
                         / ( ( (const unsigned short *)quat->keys )[quatKey + 1]
                           - ( (const unsigned short *)quat->keys )[quatKey] );
            } else {
                quatKey  = frameIndex;
                quatFrac = (rfloat)frameFrac - frameIndex;
            }
            quatFrames  = (const short (*)[2])quat->frames;
            rotDelta[0] = ( quatFrames[quatKey + 1][0] - quatFrames[quatKey][0] ) * quatFrac
                        + quatFrames[quatKey][0];
            rotDelta[1] = ( quatFrames[quatKey + 1][1] - quatFrames[quatKey][1] ) * quatFrac
                        + quatFrames[quatKey][1];
        }
    } else {
        rotDelta[0] = 0.0f;
        rotDelta[1] = 32767.0f;
    }

    trans = deltaPart->trans;
    if ( trans ) {
        transSize = trans->size;
        if ( !transSize ) {
            transDelta[0] = ( (const float *)trans )[0];
            transDelta[1] = ( (const float *)trans )[2];
            transDelta[2] = ( (const float *)trans )[3];
        } else {
            if ( transSize < numFrames ) {
                transKey  = XAnimFindShortKey( time, (const unsigned short *)trans->keys, frameIndex, transSize );
                transFrac = ( (rfloat)frameFrac - ( (const unsigned short *)trans->keys )[transKey] )
                          / ( ( (const unsigned short *)trans->keys )[transKey + 1]
                            - ( (const unsigned short *)trans->keys )[transKey] );
            } else {
                transKey  = frameIndex;
                transFrac = (rfloat)frameFrac - frameIndex;
            }
            transFrames   = (const float (*)[3])trans->frames;
            transDelta[0] = ( transFrames[transKey + 1][0] - transFrames[transKey][0] ) * transFrac
                          + transFrames[transKey][0];
            transDelta[1] = ( transFrames[transKey + 1][1] - transFrames[transKey][1] ) * transFrac
                          + transFrames[transKey][1];
            transDelta[2] = ( transFrames[transKey + 1][2] - transFrames[transKey][2] ) * transFrac
                          + transFrames[transKey][2];
        }
    } else {
        transDelta[0] = transDelta[1] = transDelta[2] = 0.0f;
    }
}

/* ---- XAnimCalcDeltaParts  0x00484D90 ----  VERIFIED */
void __cdecl XAnimCalcDeltaParts( const XAnimParts *parts, float *rotDelta, float *transDelta, float time )
{
    int                   numFrames = parts->frameCountMinusOne;
    rfloat                frameTime;
    float                 frameFrac;
    int                   frameIndex;
    const XAnimDeltaPart *deltaPart;
    const XAnimDeltaKeys *quat;
    const XAnimDeltaKeys *trans;
    const short         (*quatFrames)[2];

    if ( time != 1.0f && numFrames ) {
        frameTime  = (rfloat)numFrames * time;
        frameFrac  = frameTime;
        frameIndex = (int)frameTime;
        deltaPart  = (const XAnimDeltaPart *)parts->deltaMotion;
        if ( numFrames < 256 )
            XAnimCalcDeltaPartsByteKeys( deltaPart, numFrames, frameFrac, frameIndex, time, rotDelta, transDelta );
        else
            XAnimCalcDeltaPartsShortKeys( deltaPart, numFrames, frameFrac, frameIndex, time, rotDelta, transDelta );
    } else {
        /* time 1.0, or a single-frame animation: the last key of each part */
        deltaPart = (const XAnimDeltaPart *)parts->deltaMotion;
        quat      = deltaPart->quat;
        if ( quat ) {
            unsigned short lastKey = quat->size;

            if ( !lastKey ) {
                rotDelta[0] = ( (const short *)&quat->frames )[0];
                rotDelta[1] = ( (const short *)&quat->frames )[1];
            } else {
                quatFrames  = (const short (*)[2])quat->frames;
                rotDelta[0] = quatFrames[lastKey][0];
                rotDelta[1] = quatFrames[lastKey][1];
            }
        } else {
            rotDelta[0] = 0.0f;
            rotDelta[1] = 32767.0f;
        }

        trans = deltaPart->trans;
        if ( trans ) {
            unsigned short lastKey = trans->size;

            if ( !lastKey ) {
                transDelta[0] = ( (const float *)trans )[0];
                transDelta[1] = ( (const float *)trans )[2];
                transDelta[2] = ( (const float *)trans )[3];
            } else {
                const float *last = ( (const float (*)[3])trans->frames )[lastKey];

                transDelta[0] = last[0];
                transDelta[1] = last[1];
                transDelta[2] = last[2];
            }
        } else {
            transDelta[0] = transDelta[1] = transDelta[2] = 0.0f;
        }
    }
}

/* ---- TransformToQuatRefFrame  0x00485170 ----  VERIFIED */
void __cdecl TransformToQuatRefFrame( const float *quat, float *vec )
{
    rfloat q0sq = (rfloat)quat[0] * quat[0];
    rfloat q1sq = (rfloat)quat[1] * quat[1];   /* own local: sets retail's operand order in the vec[1] sum */
    rfloat len2 = q1sq + q0sq;

    if ( len2 != 0.0f )
    {
        rfloat s     = 2.0f / len2;
        float  sq0sq = s * q0sq;
        rfloat sq0q1 = (rfloat)quat[1] * quat[0] * s;
        rfloat v0new = ( 1.0f - (rfloat)sq0sq ) * vec[0] + sq0q1 * vec[1];

        vec[1] = vec[1] - (sq0q1 * vec[0] + sq0sq * vec[1]);
        vec[0] = v0new;
    }
}

/* ---- XAnimCalcRelDeltaParts  0x004851E0 ----  VERIFIED */
void __cdecl XAnimCalcRelDeltaParts(float *result, int parts, float weight, float time0, float time1)
{
  const XAnimDeltaKeys *trans;
  rfloat transZ;
  rfloat scale;
  float rot0[2];
  float rot1[2];
  float trans0[3];
  float trans1[3];
  float delta[3];

  XAnimCalcDeltaParts((const XAnimParts *)parts, rot0, trans0, time0);
  XAnimCalcDeltaParts((const XAnimParts *)parts, rot1, trans1, time1);
  if ( ((const XAnimParts *)parts)->looped && time1 < time0
    && (trans = ((const XAnimDeltaPart *)((const XAnimParts *)parts)->deltaMotion)->trans) != NULL )
  {
    /* the animation wrapped: add the whole-cycle translation */
    unsigned short lastKey = trans->size;

    if ( !lastKey )
    {
      trans1[0] = trans1[0] + ((const float *)trans)[0];
      trans1[1] = trans1[1] + ((const float *)trans)[2];
      transZ = (rfloat)trans1[2] + ((const float *)trans)[3];
    }
    else
    {
      const float *last = ((const float (*)[3])trans->frames)[lastKey];

      trans1[0] = trans1[0] + last[0];
      trans1[1] = trans1[1] + last[1];
      transZ = (rfloat)trans1[2] + last[2];
    }
  }
  else
  {
    transZ = trans1[2];
  }
  scale = (rfloat)weight * 9.3137942e-10f;  /* 0x569088 = 0x30800200 */
  result[0] = (rot1[0] * rot0[1] - rot1[1] * rot0[0]) * scale + result[0];
  result[1] = (rot0[1] * rot1[1] + rot1[0] * rot0[0]) * scale + result[1];
  delta[0] = trans1[0] - trans0[0];
  delta[1] = trans1[1] - trans0[1];
  delta[2] = transZ - trans0[2];
  TransformToQuatRefFrame(rot1, delta);
  result[2] = weight + result[2];
  result[3] = delta[0] * weight + result[3];
  result[4] = delta[1] * weight + result[4];
  result[5] = delta[2] * weight + result[5];
}

/* ---- XAnimCalcAbsDeltaParts  0x00485330 ----  VERIFIED */
void __cdecl XAnimCalcAbsDeltaParts(float *result_acc, unsigned __int16 *parts, float weight, float time)
{
  rfloat scale;
  float rot[2];
  float trans[3];

  XAnimCalcDeltaParts((const XAnimParts *)parts, rot, trans, time);
  scale = (rfloat)weight * 0.000030518509f;  /* 0x568ED8 = 0x38000100 */
  result_acc[0] = rot[0] * scale + result_acc[0];
  result_acc[1] = rot[1] * scale + result_acc[1];
  result_acc[2] = weight + result_acc[2];
  result_acc[3] = trans[0] * weight + result_acc[3];
  result_acc[4] = trans[1] * weight + result_acc[4];
  result_acc[5] = trans[2] * weight + result_acc[5];
}

/* ---- XAnimClearServerInfo_m  0x004853B0 ----  VERIFIED */
void __cdecl XAnimClearServerInfo_m( XAnimInfo *node )
{
    unsigned short handle = node->notifyName;

    if ( handle ) {
        SL_RemoveRefToString( handle );
    }
}

/* ---- XAnimFreeInfo  0x00485400 ----  VERIFIED */
int __cdecl XAnimFreeInfo( unsigned __int16 a1 )
{
    XAnimInfo      *node = &xanim_pool[a1];
    unsigned short  handle = node->notifyName;

    if ( handle ) {
        SL_RemoveRefToString( handle );
    }

    /* node 0 heads the free list; retail re-reads its freeNext after each store */
    node->freePrev = 0;
    node->freeNext = xanim_pool[0].freeNext;
    xanim_pool[xanim_pool[0].freeNext].freePrev = a1;
    xanim_pool[0].freeNext = a1;
    --xanim_poolUsedCount;
    return xanim_poolUsedCount;
}

/* ---- XAnimGetAverageRateFrequency  0x00485490 ----  VERIFIED */
float __cdecl XAnimGetAverageRateFrequency( int nodeIndex )
{
        XAnimEntry    *entry;
    int            childCount;
    int            i;
    unsigned short childHandle;
    XAnimInfo     *child;
    float          weight;
    float          childFreq;
    float          weightSum;
    float          freqSum;

    entry      = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;
    if ( !childCount )
        return ( *(XAnimParts **)( (char *)entry->u.leafAsset + 4 ) )->frequency;

    weightSum = 0.0f;
    freqSum   = 0.0f;
    for ( i = 0; i < childCount; ++i )
    {
        childHandle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[entry->u.parent.firstChildIndex + i];
        if ( childHandle )
        {
            child  = &xanim_pool[childHandle];
            weight = child->states[xanim_activePoolSlot].currentWeight;
            if ( weight != 0.0f )
            {
                childFreq = XAnimGetAverageRateFrequency( entry->u.parent.firstChildIndex + i );
                if ( childFreq != 0.0f )
                {
                    weightSum += weight;
                    freqSum   += childFreq * child->states[xanim_activePoolSlot].rateScale * weight;
                }
            }
        }
    }

    if ( weightSum != 0.0f )
        return freqSum / weightSum;
    return 0.0f;
}

/* ---- XAnimUpdateInfoNoWeightClient  0x004855A0 ----  VERIFIED */
char __cdecl XAnimUpdateInfoNoWeightClient( int nodeIndex )
{
    unsigned short handle;
    XAnimInfo     *node;
    int            childCount;
    XAnimEntry    *entry;
    int            i;
    unsigned char  anyChildAlive;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle )
        return 0;

    node = &xanim_pool[handle];
    node->states[0].weightBlendTimeRemaining = 0.0f;
    node->states[0].currentWeight = node->states[0].targetWeight;

    entry = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;

    /* the free test is written out once per arm (as in the Mac build); with the leaf arm
       first VC7 lays it out as retail and merges the two tails after the first compare */
    if ( !childCount )
    {
        if ( node->states[0].currentWeight == 0.0f
          && node->states[0].targetWeight == 0.0f
          && node->states[1].currentWeight == 0.0f
          && node->states[1].targetWeight == 0.0f )
        {
            XAnimFreeInfo( handle );
            ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
            return 0;
        }
        return 1;
    }

    anyChildAlive = 0;
    for ( i = 0; i < childCount; ++i )
    {
        if ( XAnimUpdateInfoNoWeightClient( i + entry->u.parent.firstChildIndex ) )
            anyChildAlive = 1;
    }
    if ( anyChildAlive )
        return 1;
    if ( node->states[0].currentWeight == 0.0f
      && node->states[0].targetWeight == 0.0f
      && node->states[1].currentWeight == 0.0f
      && node->states[1].targetWeight == 0.0f )
    {
        XAnimFreeInfo( handle );
        ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
        return 0;
    }
    return 1;
}

/* ---- XAnimGetNextNotifyTime  0x004856A0 ----  VERIFIED */
int __cdecl XAnimGetNextNotifyTime( XAnimEntry *entry, int deadArg, float time )
{
    XAnimParts      *parts   = *(XAnimParts **)( (char *)entry->u.leafAsset + 4 );
    int              addr    = 0;
    float            bestFrac = 2.0f;
    XAnimNotifyInfo *n;

    for ( n = parts->notify; n->name; ++n )
    {
        float frac = n->frameFrac;

        if ( !( frac < time ) && frac < bestFrac )
        {
            addr     = (int)n;
            bestFrac = frac;
        }
    }
    return (addr - (int)parts->notify) >> 3;
}

/* ---- XAnimGetNotifyFracLeaf  0x00485700 ----  VERIFIED */
/*
 * The cycle counts are sign-extended (0x48573C movsx start; movsx window;
 * sub), and the tail is fild cycles; fld notifyTime; fsub startTime; faddp;
 * fdiv timeStep, i.e. cycles + (notifyTime - startTime), all unrounded.
 */
float __cdecl XAnimGetNotifyFracLeaf(float notifyTime)
{
  int cycles;

  if ( xanim_evalWindowTime == 1.0f )
    return 1.0f;

  if ( xanim_evalCurrentTime < xanim_evalWindowTime )
  {
    if ( xanim_evalCurrentTime > notifyTime )
    {
      cycles = ( (short)xanim_evalWindowFrame - (short)xanim_evalStartFrame + 1 );
      return ( cycles + ( notifyTime - xanim_evalStartTime ) ) / xanim_evalTimeStep;
    }
    if ( xanim_evalWindowTime <= notifyTime )
    {
      cycles = ( (short)xanim_evalWindowFrame - (short)xanim_evalStartFrame );
      return ( cycles + ( notifyTime - xanim_evalStartTime ) ) / xanim_evalTimeStep;
    }
  }
  else
  {
    if ( ( xanim_evalCurrentTime > notifyTime || xanim_evalCurrentTime == 1.0f )
      && xanim_evalWindowTime <= notifyTime )
    {
      cycles = ( (short)xanim_evalWindowFrame - (short)xanim_evalStartFrame );
      return ( cycles + ( notifyTime - xanim_evalStartTime ) ) / xanim_evalTimeStep;
    }
  }
  return 1.0f;
}

/* ---- XAnimGetNotifyFracServer  0x004857E0 ----  VERIFIED */
/* Float-typed (retail returns a dword 1.0f); the leaf's double result is passed through unrounded. */
rfloat __fastcall XAnimGetNotifyFracServer( XAnimInfo *node, XAnimEntry *entry )
{
    __int16 notifyIndex;
    float   frac;

    if ( !xanim_notifyEntId || !node->notifyName )
        return 1.0f;

    if ( entry->childCount )
    {
        if ( !node->notifyChildIndex )
            return XAnimGetNotifyFracLeaf( 1.0f );
        entry = &( (XAnimTree *)xanim_currentTree )->sourceTree->entries[ node->notifyChildIndex ];
    }

    notifyIndex = node->notifyIndex;
    if ( notifyIndex >= 0 )
        frac = ( *(XAnimParts **)( (char *)entry->u.leafAsset + 4 ) )->notify[notifyIndex].frameFrac;
    else
        frac = 1.0f;
    return XAnimGetNotifyFracLeaf( frac );
}

#define SL_REFSTRING(h)   ( (char *)(void *)GetRefString_var + 8 * (unsigned int)(unsigned short)(h) )
#define SL_STRING(h)      ( (h) ? SL_REFSTRING(h) + 4 : (char *)0 )

/* ---- XAnimAddClientNotify_m  0x00485870 ----  VERIFIED */
void __cdecl XAnimAddClientNotify_m( unsigned short name, int a2, int a3,
                         float timeFrac, __int16 notify )
{
  int                  i;
  XAnimDeferredNotify *slot;

  (void)a2;
  (void)a3;

  for ( i = xanim_numDeferredNotifies - 1; i >= 0; i-- )
  {
    XAnimDeferredNotify *src = &xanim_deferredNotifies[i];
    if ( timeFrac >= src->timeFrac )
      break;
    src[1] = *src;
  }
  slot = &xanim_deferredNotifies[i + 1];

  slot->name         = SL_STRING( name );
  slot->timeFrac     = timeFrac;
  slot->notifyHandle = notify;

  ++xanim_numDeferredNotifies;
}

/* ---- XAnimSetClientTime  0x004858F0 ----  VERIFIED */
void __cdecl XAnimSetClientTime( XAnimInfo *node, XAnimEntry *entry, __int16 notify )
{
    XAnimParts      *parts;
    XAnimNotifyInfo *cursor;

    node->states[0].oldTime       = xanim_evalStartTime;
    node->states[0].oldCycleCount = xanim_evalStartFrame;
    node->states[0].time          = xanim_evalCurrentTime;
    node->states[0].cycleCount    = xanim_evalCurrentFrame;

    if ( !notify )
        return;
    if ( xanim_evalTimeStep == 0.0f )
        return;

    xanim_evalWindowTime  = xanim_evalStartTime;
    xanim_evalWindowFrame = xanim_evalStartFrame;

    if ( xanim_evalStartTime == 1.0f )
    {
        XAnimAddClientNotify_m( xanim_endNotifyStringHandle, (int)node, (int)entry, XAnimGetNotifyFracLeaf( 1.0f ), notify );
        return;
    }

    if ( entry->childCount )
    {
        if ( xanim_evalCurrentTime < xanim_evalStartTime || xanim_evalCurrentTime == 1.0f )
            XAnimAddClientNotify_m( xanim_endNotifyStringHandle, (int)node, (int)entry, XAnimGetNotifyFracLeaf( 1.0f ), notify );
        return;
    }

    parts  = *(XAnimParts **)( (char *)entry->u.leafAsset + 4 );
    cursor = &parts->notify[(unsigned short)XAnimGetNextNotifyTime( entry, (int)node, xanim_evalStartTime )];

    if ( xanim_evalCurrentTime < xanim_evalStartTime )
    {
        /* the step wrapped past the end of the animation */
        if ( xanim_evalCurrentTime > cursor->frameFrac )
        {
            do
            {
                XAnimAddClientNotify_m( cursor->name, (int)node, (int)entry, XAnimGetNotifyFracLeaf( cursor->frameFrac ), notify );
                ++cursor;
            }
            while ( cursor->name && xanim_evalCurrentTime > cursor->frameFrac );
            return;
        }
        if ( xanim_evalStartTime > cursor->frameFrac )
            return;
        do
        {
            XAnimAddClientNotify_m( cursor->name, (int)node, (int)entry, XAnimGetNotifyFracLeaf( cursor->frameFrac ), notify );
            ++cursor;
        }
        while ( cursor->name );
        /* retail steps two notify records at a time here (0x485AE4 add edi,10h) */
        for ( cursor = parts->notify; xanim_evalCurrentTime > cursor->frameFrac; cursor += 2 )
            XAnimAddClientNotify_m( cursor->name, (int)node, (int)entry, XAnimGetNotifyFracLeaf( cursor->frameFrac ), notify );
        return;
    }

    if ( xanim_evalCurrentTime == 1.0f )
    {
        if ( xanim_evalStartTime > cursor->frameFrac )
            return;
        do
        {
            XAnimAddClientNotify_m( cursor->name, (int)node, (int)entry, XAnimGetNotifyFracLeaf( cursor->frameFrac ), notify );
            ++cursor;
        }
        while ( cursor->name );
        return;
    }

    if ( xanim_evalCurrentTime <= cursor->frameFrac || xanim_evalStartTime > cursor->frameFrac )
        return;
    do
    {
        XAnimAddClientNotify_m( cursor->name, (int)node, (int)entry, XAnimGetNotifyFracLeaf( cursor->frameFrac ), notify );
        ++cursor;
    }
    while ( cursor->name && xanim_evalCurrentTime > cursor->frameFrac );
}

/* ---- XAnimUpdateClientInfoSyncInternal  0x00485BA0 ----  VERIFIED */
void __cdecl XAnimUpdateClientInfoSyncInternal( int nodeIndex, unsigned char syncFlag )
{
    unsigned short handle;
    XAnimInfo     *node;
    XAnimEntry    *entry;
    int            childCount;
    float          newWeight;
    unsigned char  anyChildAlive;
    int            i;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle )
        return;

    node = &xanim_pool[handle];
    if ( node->states[0].targetWeight == 0.0f )
        syncFlag = 0;

    if ( *(float *)&xanim_currentTime + 0.001f > node->states[0].weightBlendTimeRemaining )
    {
        node->states[0].currentWeight            = node->states[0].targetWeight;
        node->states[0].weightBlendTimeRemaining = 0.0f;
    }
    else
    {
        newWeight = ( node->states[0].targetWeight - node->states[0].currentWeight )
                  / node->states[0].weightBlendTimeRemaining * *(float *)&xanim_currentTime
                  + node->states[0].currentWeight;
        node->states[0].currentWeight = newWeight;
        if ( newWeight < 1.0000001e-6f )
            node->states[0].currentWeight = node->states[0].targetWeight * 0.001f;
        node->states[0].weightBlendTimeRemaining -= *(float *)&xanim_currentTime;
    }

    entry = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    XAnimSetClientTime( node, entry, syncFlag ? node->notifyType : (unsigned short)0 );
    childCount = entry->childCount;

    if ( !childCount )
    {
        if ( node->states[0].currentWeight == 0.0f
          && node->states[0].targetWeight == 0.0f
          && node->states[1].currentWeight == 0.0f
          && node->states[1].targetWeight == 0.0f )
        {
            XAnimFreeInfo( handle );
            ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = childCount;
        }
        return;
    }

    if ( node->states[0].currentWeight == 0.0f && node->states[0].targetWeight == 0.0f )
    {
        anyChildAlive = 0;
        for ( i = 0; i < childCount; ++i )
        {
            if ( XAnimUpdateInfoNoWeightClient( i + entry->u.parent.firstChildIndex ) )
                anyChildAlive = 1;
        }
        if ( !anyChildAlive
          && node->states[1].currentWeight == 0.0f
          && node->states[1].targetWeight == 0.0f )
        {
            XAnimFreeInfo( handle );
            ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
        }
    }
    else
    {
        for ( i = 0; i < childCount; ++i )
            XAnimUpdateClientInfoSyncInternal( i + entry->u.parent.firstChildIndex, syncFlag );
    }
}

/* ---- XAnimUpdateClientInfoInternal  0x00485DA0 ----  VERIFIED */
void __cdecl XAnimUpdateClientInfoInternal( int nodeIndex, float dtime, unsigned char notify )
{
    unsigned short handle;
    XAnimInfo     *node;
    XAnimEntry    *entry;
    XAnimParts    *parts;
    int            childCount;
    float          newWeight;
    float          newTime;
    unsigned short cycle;
    unsigned char  anyChildAlive;
    int            i;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle )
        return;

    node = &xanim_pool[handle];
    if ( node->states[0].targetWeight == 0.0f )
        notify = 0;

    if ( *(float *)&xanim_currentTime + 0.001f > node->states[0].weightBlendTimeRemaining )
    {
        node->states[0].currentWeight            = node->states[0].targetWeight;
        node->states[0].weightBlendTimeRemaining = 0.0f;
    }
    else
    {
        newWeight = ( node->states[0].targetWeight - node->states[0].currentWeight )
                  / node->states[0].weightBlendTimeRemaining * *(float *)&xanim_currentTime
                  + node->states[0].currentWeight;
        node->states[0].currentWeight = newWeight;
        if ( newWeight < 1.0000001e-6f )
            node->states[0].currentWeight = node->states[0].targetWeight * 0.001f;
        node->states[0].weightBlendTimeRemaining -= *(float *)&xanim_currentTime;
    }

    entry      = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;

    if ( !childCount )
    {
        if ( node->states[0].currentWeight == 0.0f && node->states[0].targetWeight == 0.0f )
        {
            if ( node->states[1].currentWeight == 0.0f && node->states[1].targetWeight == 0.0f )
            {
                XAnimFreeInfo( handle );
                ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = childCount;
            }
            return;
        }

        parts   = *(XAnimParts **)( (char *)entry->u.leafAsset + 4 );
        dtime   = parts->frequency * node->states[0].rateScale * dtime;
        newTime = dtime + node->states[0].time;
        cycle   = node->states[0].cycleCount;
        if ( newTime >= 1.0f )
        {
            if ( !parts->looped )
            {
                newTime = 1.0f;
            }
            else
            {
                do
                {
                    newTime = newTime - 1.0f;
                    ++cycle;
                }
                while ( newTime >= 1.0f );
            }
        }

        xanim_evalTimeStep     = dtime;
        xanim_evalStartTime    = node->states[0].time;
        xanim_evalStartFrame   = node->states[0].cycleCount;
        xanim_evalCurrentTime  = newTime;
        xanim_evalCurrentFrame = cycle;
        XAnimSetClientTime( node, entry, notify ? node->notifyType : (unsigned short)0 );
        return;
    }

    if ( node->states[0].currentWeight == 0.0f && node->states[0].targetWeight == 0.0f )
    {
        anyChildAlive = 0;
        for ( i = 0; i < childCount; ++i )
        {
            if ( XAnimUpdateInfoNoWeightClient( i + entry->u.parent.firstChildIndex ) )
                anyChildAlive = 1;
        }
        if ( !anyChildAlive
          && node->states[1].currentWeight == 0.0f
          && node->states[1].targetWeight == 0.0f )
        {
            XAnimFreeInfo( handle );
            ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
        }
    }
    else if ( entry->u.parent.flags & 3 )
    {
        dtime   = XAnimGetAverageRateFrequency( nodeIndex ) * node->states[0].rateScale * dtime;
        newTime = dtime + node->states[0].time;
        cycle   = node->states[0].cycleCount;
        if ( newTime >= 1.0f )
        {
            if ( entry->u.parent.flags & 2 )
            {
                newTime = 1.0f;
            }
            else
            {
                do
                {
                    newTime = newTime - 1.0f;
                    ++cycle;
                }
                while ( newTime >= 1.0f );
            }
        }

        xanim_evalTimeStep     = dtime;
        xanim_evalStartTime    = node->states[0].time;
        xanim_evalStartFrame   = node->states[0].cycleCount;
        xanim_evalCurrentTime  = newTime;
        xanim_evalCurrentFrame = cycle;
        XAnimSetClientTime( node, entry, notify ? node->notifyType : (unsigned short)0 );

        xanim_evalStartTime    = node->states[0].oldTime;
        xanim_evalStartFrame   = node->states[0].oldCycleCount;
        xanim_evalCurrentTime  = node->states[0].time;
        xanim_evalCurrentFrame = node->states[0].cycleCount;

        for ( i = 0; i < childCount; ++i )
            XAnimUpdateClientInfoSyncInternal( i + entry->u.parent.firstChildIndex, notify );
    }
    else
    {
        dtime = dtime * node->states[0].rateScale;
        for ( i = 0; i < childCount; ++i )
            XAnimUpdateClientInfoInternal( i + entry->u.parent.firstChildIndex, dtime, notify );
    }
}

/* ---- XAnimUpdateOldServerTime  0x00486150 ----  VERIFIED */
void __cdecl XAnimUpdateOldServerTime( int nodeIndex )
{
    XAnimTree        *tree   = (XAnimTree *)xanim_currentTree;
    unsigned short    handle = tree->poolNodeHandles[nodeIndex];
    XAnimInfo        *node;
    const XAnimEntry *entry;
    int               numChildren;
    int               i;

    if ( !handle )
        return;

    node = &xanim_pool[handle];
    node->states[0].time                     = node->states[1].oldTime;
    node->states[0].cycleCount               = node->states[1].oldCycleCount;
    node->states[0].targetWeight             = node->states[1].targetWeight;
    node->states[0].rateScale                = node->states[1].rateScale;
    node->states[0].weightBlendTimeRemaining = node->states[1].weightBlendTimeRemaining;
    node->states[0].currentWeight            = node->states[1].currentWeight;

    entry       = &tree->sourceTree->entries[nodeIndex];
    numChildren = entry->childCount;
    for ( i = 0; i < numChildren; ++i )
        XAnimUpdateOldServerTime( entry->u.parent.firstChildIndex + i );
}

/* ---- XAnimUpdateOldServerTimeNoWeight  0x004861D0 ----  VERIFIED */
char __cdecl XAnimUpdateOldServerTimeNoWeight( int nodeIndex )
{
    unsigned short handle;
    XAnimInfo     *node;
    int            childCount;
    XAnimEntry    *entry;
    int            i;
    unsigned char  anyChildAlive;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle )
        return 0;

    node = &xanim_pool[handle];
    node->states[1].weightBlendTimeRemaining = 0.0f;
    node->states[1].currentWeight = node->states[1].targetWeight;

    entry = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;

    /* the free test is written out once per arm (as in the Mac build); with the leaf arm
       first VC7 lays it out as retail and merges the two tails after the first compare */
    if ( !childCount )
    {
        if ( node->states[1].currentWeight == 0.0f
          && node->states[1].targetWeight == 0.0f
          && node->states[0].currentWeight == 0.0f
          && node->states[0].targetWeight == 0.0f )
        {
            XAnimFreeInfo( handle );
            ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
            return 0;
        }
        return 1;
    }

    anyChildAlive = 0;
    for ( i = 0; i < childCount; ++i )
    {
        if ( XAnimUpdateOldServerTimeNoWeight( i + entry->u.parent.firstChildIndex ) )
            anyChildAlive = 1;
    }
    if ( anyChildAlive )
        return 1;
    if ( node->states[1].currentWeight == 0.0f
      && node->states[1].targetWeight == 0.0f
      && node->states[0].currentWeight == 0.0f
      && node->states[0].targetWeight == 0.0f )
    {
        XAnimFreeInfo( handle );
        ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
        return 0;
    }
    return 1;
}

/* ---- XAnimInitServerTime_m  0x004862D0 ----  VERIFIED */
void __cdecl XAnimInitServerTime_m( int nodeIndex )
{
    unsigned short handle;
    XAnimInfo     *node;
    XAnimEntry    *entry;
    int            childCount;
    int            i;
    unsigned char  anyChildAlive;
    float          newWeight;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle )
        return;

    node = &xanim_pool[handle];
    node->states[1].oldTime       = node->states[1].time;
    node->states[1].oldCycleCount = node->states[1].cycleCount;

    if ( *(float *)&xanim_currentTime + 0.001f > node->states[1].weightBlendTimeRemaining )
    {
        node->states[1].currentWeight            = node->states[1].targetWeight;
        node->states[1].weightBlendTimeRemaining = 0.0f;
    }
    else
    {
        newWeight = ( node->states[1].targetWeight - node->states[1].currentWeight )
                  / node->states[1].weightBlendTimeRemaining * *(float *)&xanim_currentTime
                  + node->states[1].currentWeight;
        node->states[1].currentWeight = newWeight;
        if ( newWeight < 1.0000001e-6f )
            node->states[1].currentWeight = node->states[1].targetWeight * 0.001f;
        node->states[1].weightBlendTimeRemaining -= *(float *)&xanim_currentTime;
    }

    entry      = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;

    if ( !childCount )
    {
        if ( node->states[1].currentWeight == 0.0f
          && node->states[1].targetWeight == 0.0f
          && node->states[0].currentWeight == 0.0f
          && node->states[0].targetWeight == 0.0f )
        {
            XAnimFreeInfo( handle );
            ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
        }
        return;
    }

    if ( node->states[1].currentWeight == 0.0f && node->states[1].targetWeight == 0.0f )
    {
        anyChildAlive = 0;
        for ( i = 0; i < childCount; ++i )
        {
            if ( XAnimUpdateOldServerTimeNoWeight( i + entry->u.parent.firstChildIndex ) )
                anyChildAlive = 1;
        }
        if ( !anyChildAlive
          && node->states[0].currentWeight == 0.0f
          && node->states[0].targetWeight == 0.0f )
        {
            XAnimFreeInfo( handle );
            ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] = 0;
        }
    }
    else
    {
        for ( i = 0; i < childCount; ++i )
            XAnimInitServerTime_m( i + entry->u.parent.firstChildIndex );
    }
}

/* ---- XAnimClearServerNotify  0x004864A0 ----  VERIFIED */
void __cdecl XAnimClearServerNotify( XAnimInfo *node )
{
    unsigned short handle = node->notifyName;

    if ( handle ) {
        SL_RemoveRefToString( handle );
        node->notifyName = 0;
    }
    node->notifyIndex = -1;
}

/* ---- NotifyServerNotetrack  0x00486510 ----  VERIFIED */
void __cdecl NotifyServerNotetrack( unsigned __int16 notifyName, unsigned __int16 entId,
                         unsigned __int16 notifyId )
{
    Scr_AddConstString( notifyName );
    Scr_NotifyId( entId, notifyId, 1 );
}

/* ---- XAnimGetServerNotifyFracSyncTotal  0x00486550 ----  VERIFIED */
float __cdecl XAnimGetServerNotifyFracSyncTotal( XAnimInfo *node, XAnimEntry *entry )
{
    /* retail keeps the running minimum in a float slot: fstp dword at 0x486560 and
       0x4865D5, returned with fld dword at 0x4865E9 */
    float          frac;
    int            childCount;
    int            i;
    unsigned short childHandle;
    XAnimInfo     *childNode;
    float          childFrac;

    frac = XAnimGetNotifyFracServer( node, entry );
    childCount = entry->childCount;
    for ( i = 0; i < childCount; ++i )
    {
        childHandle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[entry->u.parent.firstChildIndex + i];
        if ( childHandle )
        {
            childNode = &xanim_pool[childHandle];
            if ( childNode->states[1].currentWeight != 0.0f
              && childNode->states[1].targetWeight != 0.0f )
            {
                childFrac = XAnimGetServerNotifyFracSyncTotal( childNode, &((XAnimTree *)xanim_currentTree)->sourceTree->entries[entry->u.parent.firstChildIndex + i] );
                if ( childFrac < frac )
                    frac = childFrac;
            }
        }
    }
    return frac;
}

/* ---- XAnimFindServerNoteTrack  0x004865F0 ----  VERIFIED */
float __cdecl XAnimFindServerNoteTrack( int nodeIndex, float dtime )
{
    unsigned short handle;
    XAnimInfo     *node;
    XAnimState    *state;
    XAnimEntry    *entry;
    int            childCount;
    float          newTime;
    unsigned short cycle;
    unsigned short newCycle;
    unsigned short baseCycle;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle )
        return 1.0f;
    node  = &xanim_pool[handle];
    state = &node->states[1];

    if ( state->currentWeight == 0.0f || state->targetWeight == 0.0f )
        return 1.0f;

    entry      = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;

    if ( !childCount )
    {
        XAnimParts *parts = *(XAnimParts **)( (char *)entry->u.leafAsset + 4 );

        /* the scaled step overwrites dtime (retail 0x486669 fst [esp+18h]) */
        dtime = parts->frequency * state->rateScale * dtime;
        if ( dtime == 0.0f )
            return 1.0f;

        newTime  = dtime + state->oldTime;
        cycle    = state->oldCycleCount;
        newCycle = cycle;
        if ( !parts->looped )
        {
            if ( newTime >= 1.0f )
                newTime = 1.0f;
        }
        else
        {
            for ( ; newTime >= 1.0f; ++newCycle )
                newTime = newTime - 1.0f;
        }

        baseCycle = state->cycleCount;
        if ( (float)( (short)newCycle - (short)baseCycle ) < state->time - newTime )
            return 1.0f;

        xanim_evalTimeStep    = dtime;
        xanim_evalStartTime   = state->oldTime;
        xanim_evalStartFrame  = cycle;
        xanim_evalCurrentTime = newTime;
        xanim_evalWindowTime  = state->time;
        xanim_evalWindowFrame = baseCycle;
        return XAnimGetNotifyFracServer( node, entry );
    }

    if ( entry->u.parent.flags & 3 )
    {
        dtime = XAnimGetAverageRateFrequency( nodeIndex ) * state->rateScale * dtime;
        if ( dtime == 0.0f )
            return 1.0f;

        newTime  = dtime + state->oldTime;
        cycle    = state->oldCycleCount;
        newCycle = cycle;
        if ( entry->u.parent.flags & 2 )
        {
            if ( newTime >= 1.0f )
                newTime = 1.0f;
        }
        else
        {
            for ( ; newTime >= 1.0f; ++newCycle )
                newTime = newTime - 1.0f;
        }

        baseCycle = state->cycleCount;
        if ( (float)( (short)newCycle - (short)baseCycle ) < state->time - newTime )
            return 1.0f;

        xanim_evalTimeStep    = dtime;
        xanim_evalStartTime   = state->oldTime;
        xanim_evalStartFrame  = cycle;
        xanim_evalCurrentTime = newTime;
        xanim_evalWindowTime  = state->time;
        xanim_evalWindowFrame = baseCycle;
        return XAnimGetServerNotifyFracSyncTotal( node, entry );
    }
    else
    {
        float minFrac;
        float childFrac;
        int   i;

        dtime = dtime * state->rateScale;
        if ( dtime == 0.0f )
            return 1.0f;

        minFrac = 1.0f;
        for ( i = 0; i < childCount; ++i )
        {
            childFrac = XAnimFindServerNoteTrack( i + entry->u.parent.firstChildIndex, dtime );
            if ( childFrac < minFrac )
                minFrac = childFrac;
        }
        return minFrac;
    }
}

/* ---- XAnimProcessServerNotify  0x00486870 ----  VERIFIED */
void __cdecl XAnimProcessServerNotify( XAnimInfo *node, XAnimEntry *entry )
{
    XAnimParts      *parts;
    XAnimNotifyInfo *cursor;

    if ( !xanim_notifyEntId || !node->notifyName )
        return;

    if ( xanim_evalStartTime == 1.0f )
    {
        Scr_AddConstString( xanim_endNotifyStringHandle );
        Scr_NotifyId( xanim_notifyEntId, node->notifyName, 1 );
        return;
    }

    if ( entry->childCount )
    {
        if ( !node->notifyChildIndex )
        {
            if ( xanim_evalCurrentTime < xanim_evalStartTime || xanim_evalCurrentTime == 1.0f )
            {
                Scr_AddConstString( xanim_endNotifyStringHandle );
                Scr_NotifyId( xanim_notifyEntId, node->notifyName, 1 );
            }
            return;
        }
        entry = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[node->notifyChildIndex];
    }

    parts  = *(XAnimParts **)( (char *)entry->u.leafAsset + 4 );
    cursor = &parts->notify[node->notifyIndex];

    if ( xanim_evalCurrentTime < xanim_evalStartTime )
    {
        /* the step wrapped past the end of the animation */
        if ( xanim_evalCurrentTime > cursor->frameFrac )
        {
            do
            {
                NotifyServerNotetrack( cursor->name, xanim_notifyEntId, node->notifyName );
                ++cursor;
            }
            while ( cursor->name && xanim_evalCurrentTime > cursor->frameFrac );
            node->notifyIndex = XAnimGetNextNotifyTime( entry, (int)node, xanim_evalCurrentTime );
            return;
        }
        else
        {
            if ( xanim_evalStartTime > cursor->frameFrac )
                return;
            do
            {
                NotifyServerNotetrack( cursor->name, xanim_notifyEntId, node->notifyName );
                ++cursor;
            }
            while ( cursor->name );
            for ( cursor = parts->notify; xanim_evalCurrentTime > cursor->frameFrac; ++cursor )
                NotifyServerNotetrack( cursor->name, xanim_notifyEntId, node->notifyName );
            node->notifyIndex = XAnimGetNextNotifyTime( entry, (int)node, xanim_evalCurrentTime );
            return;
        }
    }
    else
    {
        if ( xanim_evalCurrentTime == 1.0f )
        {
            if ( xanim_evalStartTime > cursor->frameFrac )
                return;
            do
            {
                NotifyServerNotetrack( cursor->name, xanim_notifyEntId, node->notifyName );
                ++cursor;
            }
            while ( cursor->name );
            return;
        }
        if ( xanim_evalCurrentTime <= cursor->frameFrac || xanim_evalStartTime > cursor->frameFrac )
            return;
        do
        {
            NotifyServerNotetrack( cursor->name, xanim_notifyEntId, node->notifyName );
            ++cursor;
        }
        while ( cursor->name && xanim_evalCurrentTime > cursor->frameFrac );
        node->notifyIndex = XAnimGetNextNotifyTime( entry, (int)node, xanim_evalCurrentTime );
    }
}

/* ---- XAnimProcessServerNotify_r  0x00486B30 ----  VERIFIED */
void __cdecl XAnimProcessServerNotify_r( XAnimInfo *node, XAnimEntry *entry )
{
    int            i;
    int            childIndex;
    unsigned short childHandle;
    XAnimInfo     *childNode;

    XAnimProcessServerNotify( node, entry );

    for ( i = 0; i < entry->childCount; ++i )
    {
        childIndex  = entry->u.parent.firstChildIndex + i;
        childHandle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[childIndex];
        if ( childHandle )
        {
            childNode = &xanim_pool[childHandle];
            if ( childNode->states[1].currentWeight != 0.0f
              && childNode->states[1].targetWeight != 0.0f )
            {
                XAnimProcessServerNotify_r( childNode,
                    &((XAnimTree *)xanim_currentTree)->sourceTree->entries[childIndex] );
            }
        }
    }
}

/* ---- XAnimStampSecondaryWindowStart_m  0x00486BB0 ----  VERIFIED */
void __cdecl XAnimStampSecondaryWindowStart_m( int nodeIndex )
{
    XAnimTree      *tree = (XAnimTree *)xanim_currentTree;
    unsigned short  handle = tree->poolNodeHandles[nodeIndex];
    XAnimState     *state;
    XAnimEntry     *entry;
    int             child;

    if ( !handle ) {
        return;
    }

    state             = &xanim_pool[handle].states[1];
    state->time       = xanim_evalCurrentTime;
    state->cycleCount = xanim_evalCurrentFrame;

    entry = &tree->sourceTree->entries[nodeIndex];
    for ( child = 0; child < (int)entry->childCount; ++child ) {
        XAnimStampSecondaryWindowStart_m( entry->u.parent.firstChildIndex + child );
    }
}

/* ---- XAnimUpdateServerInfoInternal  0x00486C10 ----  VERIFIED */
void __cdecl XAnimUpdateServerInfoInternal( int nodeIndex, float dtime, unsigned char notify )
{
    unsigned short handle;
    XAnimInfo     *node;
    XAnimState    *state;
    XAnimEntry    *entry;
    XAnimParts    *parts;
    int            childCount;
    float          newTime;
    short          cycle;
    short          baseCycle;
    int            i;

    handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex];
    if ( !handle )
        return;

    node  = &xanim_pool[handle];
    state = &node->states[1];
    if ( state->currentWeight == 0.0f )
        return;
    if ( state->targetWeight == 0.0f )
        notify = 0;

    entry      = &((XAnimTree *)xanim_currentTree)->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;

    if ( !childCount )
    {
        parts   = *(XAnimParts **)( (char *)entry->u.leafAsset + 4 );
        newTime = parts->frequency * state->rateScale * dtime + state->oldTime;
        cycle   = state->oldCycleCount;
        if ( !parts->looped )
        {
            if ( newTime >= 1.0f )
                newTime = 1.0f;
            else if ( newTime <= 0.0f )
                return;
        }
        else
        {
            while ( newTime < 0.0f )
            {
                newTime = newTime + 1.0f;
                --cycle;
            }
            while ( newTime >= 1.0f )
            {
                newTime = newTime - 1.0f;
                ++cycle;
            }
        }

        baseCycle = state->cycleCount;
        if ( (float)( cycle - baseCycle ) < state->time - newTime )
            return;
        if ( notify )
        {
            xanim_evalCurrentTime = newTime;
            xanim_evalStartTime   = state->time;
            xanim_evalStartFrame  = baseCycle;
            XAnimProcessServerNotify( node, entry );
        }
        state->cycleCount = cycle;
        state->time       = newTime;
        return;
    }

    if ( entry->u.parent.flags & 3 )
    {
        newTime = XAnimGetAverageRateFrequency( nodeIndex ) * state->rateScale * dtime
                + state->oldTime;
        cycle   = state->oldCycleCount;
        if ( entry->u.parent.flags & 2 )
        {
            if ( newTime >= 1.0f )
                newTime = 1.0f;
            else if ( newTime <= 0.0f )
                return;
        }
        else
        {
            while ( newTime < 0.0f )
            {
                newTime = newTime + 1.0f;
                --cycle;
            }
            while ( newTime >= 1.0f )
            {
                newTime = newTime - 1.0f;
                ++cycle;
            }
        }

        baseCycle = state->cycleCount;
        if ( (float)( cycle - baseCycle ) < state->time - newTime )
            return;
        xanim_evalCurrentTime = newTime;
        if ( notify )
        {
            xanim_evalStartTime  = state->time;
            xanim_evalStartFrame = baseCycle;
            XAnimProcessServerNotify_r( node, entry );
        }
        xanim_evalCurrentFrame = cycle;
        XAnimStampSecondaryWindowStart_m( nodeIndex );
        return;
    }

    dtime = dtime * state->rateScale;
    for ( i = 0; i < childCount; ++i )
        XAnimUpdateServerInfoInternal( i + entry->u.parent.firstChildIndex, dtime, notify );
}

#define XANIM_EVAL_SKIP(i)  ( ( ((const char *)xanim_evalSkipBits)[(i) >> 3] \
                                & ( 1 << ( (i) & 7 ) ) ) != 0 )

/* ---- XAnimCalc  0x00486EC0 ----  VERIFIED */
void __cdecl XAnimCalc( int nodeIndex, float weight, DObjAnimMat *part,
                         unsigned char clear, unsigned char normalize )
{
    XAnim          *source = ((XAnimTree *)xanim_currentTree)->sourceTree;
    XAnimEntry     *entry;
    DObjAnimMat    *blendParts;
    int             childCount;
    int             firstChild;
    int             secondChild;
    float           firstWeight;
    float           secondWeight;
    unsigned short  handle;
    int             i;

    entry      = &source->entries[nodeIndex];
    childCount = entry->childCount;

    for ( ;; ) {
        if ( !childCount )
            break;
        for ( firstChild = 0; firstChild < childCount; ++firstChild ) {
            handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[firstChild + entry->u.parent.firstChildIndex];
            if ( handle && ( firstWeight = XANIM_LANE( handle ).currentWeight ) != 0.0f )
                goto foundFirst;
        }
        if ( clear )
            XAnimClearData( part );
        return;

foundFirst:
        for ( secondChild = firstChild + 1; secondChild < childCount; ++secondChild ) {
            handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[secondChild + entry->u.parent.firstChildIndex];
            if ( handle && ( secondWeight = XANIM_LANE( handle ).currentWeight ) != 0.0f )
                goto blend;
        }

        /* exactly one active child: descend into it, keeping weight/clear/normalize */
        nodeIndex  = firstChild + entry->u.parent.firstChildIndex;
        entry      = &source->entries[nodeIndex];
        childCount = entry->childCount;
    }

    if ( clear )
        XAnimClearData( part );

    {
        unsigned char *remapGeneration = ((DObj *)xanim_evalDObj)->partRemapTable
                                       + 2 * ((XAnimTree *)xanim_currentTree)->sourceTree->nodeCount;

        if ( !((unsigned short *)((DObj *)xanim_evalDObj)->partRemapTable)[nodeIndex] ) {
            remapGeneration[nodeIndex + 1] = remapGeneration[0];
            ((unsigned short *)((DObj *)xanim_evalDObj)->partRemapTable)[nodeIndex] =
                (unsigned short)XAnimSetModel( (int)entry, xanim_evalChildRefs, xanim_evalChildCount );
        } else if ( remapGeneration[nodeIndex + 1] != remapGeneration[0] ) {
            remapGeneration[nodeIndex + 1] = remapGeneration[0];
            /* retail calls SL_RemoveRefToStringOfLen here (0x487008) instead of
               inlining it */
#pragma inline_depth(0)
            SL_RemoveRefToStringOfLen(
                ((unsigned short *)((DObj *)xanim_evalDObj)->partRemapTable)[nodeIndex],
                (int)*(const short *)((XAnimParts *)( (void **)entry->u.leafAsset )[1])->partNameHandles
                    + XANIM_REMAP_PREFIX );
#pragma inline_depth()
            ((unsigned short *)((DObj *)xanim_evalDObj)->partRemapTable)[nodeIndex] =
                (unsigned short)XAnimSetModel( (int)entry, xanim_evalChildRefs, xanim_evalChildCount );
        }
    }

    XAnimCalcData( (unsigned int *)SL_ConvertToString(
                       ((unsigned short *)((DObj *)xanim_evalDObj)->partRemapTable)[nodeIndex] ),
                   part,
                   (void *)entry->u.leafAsset,
                   weight,
                   XANIM_LANE( ((XAnimTree *)xanim_currentTree)->poolNodeHandles[nodeIndex] ).time );
    return;

blend:
    /* 0x487090-0x4870B2: blend in place when clearing, else into an
     * _alloca of xanim_evalPartBytes (add 3; and ~3; __alloca_probe). */
    if ( clear )
        blendParts = part;
    else
        blendParts = (DObjAnimMat *)_alloca( xanim_evalPartBytes );

    XAnimCalc( firstChild + entry->u.parent.firstChildIndex, firstWeight, blendParts, 1, 1 );
    XAnimCalc( secondChild + entry->u.parent.firstChildIndex, secondWeight, blendParts, 0, 1 );

    for ( ++secondChild; secondChild < childCount; ++secondChild ) {
        handle = ((XAnimTree *)xanim_currentTree)->poolNodeHandles[secondChild + entry->u.parent.firstChildIndex];
        if ( handle && ( secondWeight = XANIM_LANE( handle ).currentWeight ) != 0.0f )
            XAnimCalc( secondChild + entry->u.parent.firstChildIndex, secondWeight, blendParts, 0, 1 );
    }

    /*
     * The three loops walk part (and blendParts) by pointer, as retail does
     * (add esi,20h per element, also in the x4-unrolled copy).
     * All three loops keep lengthSq, sqrt, the scale factors and 1/weight
     * unrounded on the x87 stack (_PC_53), and round only at each fstp into
     * a part field.  lengthSq is summed as ((q3*q3 + q1*q1) + q0*q0) + q2*q2
     * (0x4873F6 / 0x4874C3).  The locals are rfloat, not double: a double local
     * makes LTCG pad every caller's frame for 8-byte alignment (DObjCalcAnim).
     */
    if ( !normalize ) {
        /* 0x487195 (unrolled x4, tail 0x48736C): fld 1.0f; fdiv weight. */
        for ( i = 0; i < xanim_evalPartCount; ++i, ++part ) {
            if ( !XANIM_EVAL_SKIP( i ) && part->weight != 0.0f ) {
                rfloat scale = 1.0f / part->weight;

                part->quat[0]  = (float)( scale * part->quat[0] );
                part->quat[1]  = (float)( scale * part->quat[1] );
                part->quat[2]  = (float)( scale * part->quat[2] );
                part->quat[3]  = (float)( scale * part->quat[3] );
                part->trans[0] = (float)( scale * part->trans[0] );
                part->trans[1] = (float)( scale * part->trans[1] );
                part->trans[2] = (float)( scale * part->trans[2] );
            }
        }
    } else if ( clear ) {
        for ( i = 0; i < xanim_evalPartCount; ++i, ++part ) {
            rfloat lengthSq;

            if ( XANIM_EVAL_SKIP( i ) ) {
                continue;
            }

            {
                /* retail loads quat[3..0] first and accumulates in quat[3]'s x87 slot */
                float w = part->quat[3], z = part->quat[2], y = part->quat[1], x = part->quat[0];

                lengthSq = w * w;
                lengthSq += y * y;
                lengthSq += x * x;
                lengthSq += z * z;
            }
            if ( lengthSq != 0.0f ) {
                rfloat scale = weight / (float)sqrt( lengthSq );

                part->quat[0] = (float)( scale * part->quat[0] );
                part->quat[1] = (float)( scale * part->quat[1] );
                part->quat[2] = (float)( scale * part->quat[2] );
                part->quat[3] = (float)( scale * part->quat[3] );
            }

            if ( part->weight != 0.0f ) {
                /* 0x48745F fld weight; fdiv [esi+10h] reads the old weight
                 * before the mov [esi+10h],weight store. */
                rfloat scale = weight / part->weight;

                part->weight   = weight;
                part->trans[0] = (float)( scale * part->trans[0] );
                part->trans[1] = (float)( scale * part->trans[1] );
                part->trans[2] = (float)( scale * part->trans[2] );
            }
        }
    } else {
        for ( i = 0; i < xanim_evalPartCount; ++i, ++part, ++blendParts ) {
            rfloat lengthSq;

            if ( XANIM_EVAL_SKIP( i ) ) {
                continue;
            }

            lengthSq = blendParts->quat[3] * blendParts->quat[3]
                     + blendParts->quat[1] * blendParts->quat[1]
                     + blendParts->quat[0] * blendParts->quat[0]
                     + blendParts->quat[2] * blendParts->quat[2];
            if ( lengthSq != 0.0f ) {
                rfloat scale = weight / (float)sqrt( lengthSq );

                part->quat[0] = (float)( scale * blendParts->quat[0] + part->quat[0] );
                part->quat[1] = (float)( scale * blendParts->quat[1] + part->quat[1] );
                part->quat[2] = (float)( scale * blendParts->quat[2] + part->quat[2] );
                part->quat[3] = (float)( scale * blendParts->quat[3] + part->quat[3] );
            }

            if ( blendParts->weight != 0.0f ) {
                rfloat scale = weight / blendParts->weight;

                part->weight  += weight;
                part->trans[0] = (float)( scale * blendParts->trans[0] + part->trans[0] );
                part->trans[1] = (float)( scale * blendParts->trans[1] + part->trans[1] );
                part->trans[2] = (float)( scale * blendParts->trans[2] + part->trans[2] );
            }
        }
    }
}


int XAnimIsLeafNode_m( const XAnim *tree, unsigned int index );

/* ---- XAnimDisplay  0x004875A0 ----  VERIFIED */
void __cdecl XAnimDisplay( XAnimTree *tree, int nodeIndex, int depth )
{
    unsigned short  handle;
    XAnimEntry     *entry;
    XAnimInfo      *info;
    XAnimState     *state;
    int             childCount;
    int             i;

    handle = tree->poolNodeHandles[nodeIndex];
    if ( handle == 0 ) {
        return;
    }

    entry      = &tree->sourceTree->entries[nodeIndex];
    childCount = entry->childCount;
    info       = &xanim_pool[handle];
    state      = &info->states[xanim_activePoolSlot];

    if ( state->currentWeight == 0.0f ) {
        return;
    }

    for ( i = depth; i > 0; --i ) {
        Com_Printf( " " );
    }

    if (childCount == 0) {
        void      **fileData = (void **)entry->u.leafAsset;
        XAnimParts *parts    = (XAnimParts *)fileData[1];
        /* retail 0x48763B..0x487669 keeps elapsed and realtimedelta on the x87 stack
           (never stored as float) and passes the double to Com_Printf; a float (not
           double) local is what lets retail align its own frame (and esp,-8) instead
           of its callers padding theirs */
        float       elapsed  = state->time - state->oldTime;
        rfloat      realTimeDelta;

        if ( elapsed < 0.0f ) {
            elapsed += 1.0f;
        }
        if ( parts->frequency != 0.0f )
            realTimeDelta = elapsed / parts->frequency;
        else
            realTimeDelta = 0.0f;

        if ( xanim_activePoolSlot && info->notifyName != 0 ) {
            const char *name = (const char *)fileData[0];
            float frac;

            if ( info->notifyIndex >= 0 ) {
                frac = parts->notify[info->notifyIndex].frameFrac;
            } else {
                frac = 1.0f;
            }
            Com_Printf(
                "(name) %s: (weight) %f -> %f, (time) %f -> %f, "
                "(realtimedelta) %f, '%s' (%f)\n",
                name,
                state->currentWeight, state->targetWeight,
                state->oldTime, state->time, realTimeDelta,
                SL_ConvertToString( info->notifyName ), frac );
        } else {
            Com_Printf(
                "(name) %s: (weight) %f -> %f, (time) %f -> %f, "
                "(realtimedelta) %f\n",
                (const char *)fileData[0],
                state->currentWeight, state->targetWeight,
                state->oldTime, state->time, realTimeDelta );
        }
    } else {
        if ( xanim_activePoolSlot && info->notifyName != 0 ) {
            if ( XAnimIsLeafNode_m( tree->sourceTree, nodeIndex ) ) {
                if ( !info->notifyChildIndex ) {
                    Com_Printf(
                        "(index) %d: (weight) %f -> %f, (time) %f -> %f, '%s'\n",
                        nodeIndex, state->currentWeight, state->targetWeight,
                        state->oldTime, state->time,
                        SL_ConvertToString( info->notifyName ) );
                } else {
                    float frac;

                    if ( info->notifyIndex >= 0 ) {
                        XAnimParts *parts = *(XAnimParts **)(
                            (char *)tree->sourceTree
                                        ->entries[info->notifyChildIndex].u.leafAsset
                            + 4 );

                        frac = parts->notify[info->notifyIndex].frameFrac;
                    } else {
                        frac = 1.0f;
                    }
                    Com_Printf(
                        "(index) %d: (weight) %f -> %f, (time) %f -> %f, '%s', (%f)\n",
                        nodeIndex, state->currentWeight, state->targetWeight,
                        state->oldTime, state->time,
                        SL_ConvertToString( info->notifyName ), frac );
                }
            } else {
                Com_Printf( "(index) %d: (weight) %f -> %f, '%s'\n",
                            nodeIndex, state->currentWeight, state->targetWeight,
                            SL_ConvertToString( info->notifyName ) );
            }
        } else if ( XAnimIsLeafNode_m( tree->sourceTree, nodeIndex ) ) {
            Com_Printf( "(index) %d: (weight) %f -> %f, (time) %f -> %f\n",
                        nodeIndex, state->currentWeight, state->targetWeight,
                        state->oldTime, state->time );
        } else {
            Com_Printf( "(index) %d: (weight) %f -> %f\n",
                        nodeIndex, state->currentWeight, state->targetWeight );
        }

        for ( i = 0; i < childCount; ++i ) {
            XAnimDisplay( tree, entry->u.parent.firstChildIndex + i, depth + 1 );
        }
    }
}

/* ---- XAnimCalcTreeDelta_m  0x004878D0 ----  VERIFIED */
/* The child scans read entry->u.parent.firstChildIndex and the pool globals
   directly: retail re-reads them after each recursive call. */
void __cdecl XAnimCalcTreeDelta_m( XAnimTree *tree, int nodeIndex, float weight,
                         float *delta, unsigned char isFirst, unsigned char isChild )
{
    XAnimEntry *entry      = &tree->sourceTree->entries[nodeIndex];
    int         childCount = entry->childCount;
    int         first;
    int         second;
    float       firstWeight;
    float       secondWeight;
    float      *out;
    rfloat      lengthSq;   /* x87 stack temporaries in retail (_PC_53): */
    rfloat      scale;      /* rounded to float only on each delta store  */
    float       scratch[6];

    if ( childCount == 0 ) {
        XAnimParts     *parts;
        XAnimState     *state;
        unsigned short  handle;

        if ( isFirst ) {
            delta[0] = 0.0f;
            delta[1] = 0.0f;
            delta[2] = 0.0f;
            delta[3] = 0.0f;
            delta[4] = 0.0f;
            delta[5] = 0.0f;
        }

        parts = *(XAnimParts **)( (char *)entry->u.leafAsset + 4 );
        if ( !parts->hasDeltaMotion ) {
            return;
        }

        handle = tree->poolNodeHandles[nodeIndex];
        if ( handle == 0 ) {
            return;
        }
        state = &XANIM_LANE( handle );

        if ( xanim_evalLeafOutputMode ) {
            XAnimCalcAbsDeltaParts( delta, (unsigned short *)parts, weight, state->time );
        } else {
            XAnimCalcRelDeltaParts( delta, (int)parts, weight, state->oldTime, state->time );
        }
        return;
    }

    /* both child scans leave through gotos, so their not-found paths fall
       into the shared return (retail block layout and register rotation) */
    for ( first = 0; first < childCount; ++first ) {
        unsigned short handle;

        handle = tree->poolNodeHandles[entry->u.parent.firstChildIndex + first];
        if ( handle ) {
            XAnimState *state = &XANIM_LANE( handle );

            firstWeight = !xanim_evalPoolWeightSelector ? state->currentWeight : state->targetWeight;
            if ( firstWeight != 0.0f ) {
                goto foundFirst;
            }
        }
    }
    if ( isFirst ) {
        delta[0] = 0.0f;
        delta[1] = 0.0f;
        delta[2] = 0.0f;
        delta[3] = 0.0f;
        delta[4] = 0.0f;
        delta[5] = 0.0f;
    }
    return;

foundFirst:

    for ( second = first + 1; second < childCount; ++second ) {
        unsigned short handle;

        handle = tree->poolNodeHandles[entry->u.parent.firstChildIndex + second];
        if ( handle ) {
            XAnimState *state = &XANIM_LANE( handle );

            secondWeight = !xanim_evalPoolWeightSelector ? state->currentWeight : state->targetWeight;
            if ( secondWeight != 0.0f ) {
                goto foundSecond;
            }
        }
    }
    XAnimCalcTreeDelta_m( tree, entry->u.parent.firstChildIndex + first, weight, delta, isFirst, isChild );
    return;

foundSecond:

    out = isFirst ? delta : scratch;

    XAnimCalcTreeDelta_m( tree, entry->u.parent.firstChildIndex + first,  firstWeight,  out, 1, 1 );
    XAnimCalcTreeDelta_m( tree, entry->u.parent.firstChildIndex + second, secondWeight, out, 0, 1 );

    for ( ++second; second < childCount; ++second ) {
        unsigned short handle;

        handle = tree->poolNodeHandles[entry->u.parent.firstChildIndex + second];

        if ( handle ) {
            XAnimState *state = &XANIM_LANE( handle );

            secondWeight = !xanim_evalPoolWeightSelector ? state->currentWeight : state->targetWeight;
            if ( secondWeight != 0.0f ) {
                XAnimCalcTreeDelta_m( tree, entry->u.parent.firstChildIndex + second, secondWeight, out, 0, 1 );
            }
        }
    }

    /* Blend / normalise tail.  Retail keeps lengthSq, sqrt, the scale
       quotient and each scale*x (+ delta) sum on the x87 stack at _PC_53
       (0x00487B77..0x00487C83) and rounds to float only in the fstp of each
       delta component.  lengthSq/scale are rfloat (double only in the modern
       build): a double local would make LTCG pad the callers' frames for
       8-byte alignment (XAnimGetAbsDelta_m). */
    if ( !isChild ) {
        if ( delta[2] == 0.0f ) {
            return;
        }
        scale    = 1.0f / delta[2];
        delta[3] = (float)( scale * delta[3] );
        delta[4] = (float)( scale * delta[4] );
        delta[5] = (float)( scale * delta[5] );
        return;
    }

    if ( isFirst ) {
        lengthSq = (double)delta[0] * delta[0] + (double)delta[1] * delta[1];
        if ( lengthSq != 0.0f ) {
            scale = weight / (float)sqrt( lengthSq );

            delta[0] = (float)( scale * delta[0] );
            delta[1] = (float)( scale * delta[1] );
        }

        if ( delta[2] == 0.0f ) {
            return;
        }
        scale    = (double)weight / delta[2];
        delta[2] = weight;
        delta[3] = (float)( scale * delta[3] );
        delta[4] = (float)( scale * delta[4] );
        delta[5] = (float)( scale * delta[5] );
        return;
    }

    lengthSq = (double)out[0] * out[0] + (double)out[1] * out[1];
    if ( lengthSq != 0.0f ) {
        scale = weight / (float)sqrt( lengthSq );

        delta[0] = (float)( scale * out[0] + delta[0] );
        delta[1] = (float)( scale * out[1] + delta[1] );
    }

    if ( out[2] == 0.0f ) {
        return;
    }
    scale     = (double)weight / out[2];
    delta[2] += weight;
    delta[3]  = (float)( scale * out[3] + delta[3] );
    delta[4]  = (float)( scale * out[4] + delta[4] );
    delta[5]  = (float)( scale * out[5] + delta[5] );
}

/* ---- XAnimGetLength  0x00487C90 ----  VERIFIED */
/* No callers; retail's LTCG registers (sourceTree ecx, nodeIndex eax) give this parameter order. */
double __cdecl XAnimGetLength( const XAnim *sourceTree, int nodeIndex )
{
    void       *leafAsset = sourceTree->entries[nodeIndex].u.leafAsset;
    XAnimParts *parts     = *(XAnimParts **)( (char *)leafAsset + 4 );

    return (double)parts->frameCountMinusOne / parts->frameRate;
}

/* ---- XAnimGetTime  0x00487CB0 ----  VERIFIED */
float __cdecl XAnimGetTime( XAnimTree *tree, int nodeIndex )
{
    unsigned short handle = tree->poolNodeHandles[nodeIndex];

    return handle ? XANIM_LANE( handle ).time : 0.0f;
}

/* ---- XAnimGetWeight  0x00487CE0 ----  VERIFIED */
float __cdecl XAnimGetWeight( XAnimTree *tree, int nodeIndex )
{
    unsigned short handle = tree->poolNodeHandles[nodeIndex];

    return handle ? XANIM_LANE( handle ).currentWeight : 0.0f;
}

/* ---- XAnimHasTime  0x00487D10 ----  VERIFIED */
int __cdecl XAnimHasTime( XAnimTree *tree, int nodeIndex )
{
    unsigned short handle = tree->poolNodeHandles[nodeIndex];
    XAnimState    *lane;

    if ( !handle ) {
        return 1;
    }
    lane = &XANIM_LANE( handle );

    /* Retail compares the cycle counters as SIGNED 16-bit values
       (0x00487D50: cmp dx,word ptr [ecx+0Ah]; jg), so the counters are
       cast to short here rather than compared as unsigned short. */
    return lane->time < lane->oldTime
      || *(const int *)&lane->time == 1065353216
      || (short)lane->cycleCount > (short)lane->oldCycleCount;
}

/* ---- XAnimGetNumChildren_m  0x00487D60 ----  VERIFIED */
int __cdecl XAnimGetNumChildren_m( const XAnim *sourceTree, int nodeIndex )
{
    return sourceTree->entries[nodeIndex].childCount;
}

/* ---- XAnimGetChildIndex_m  0x00487D70 ----  VERIFIED */
int __cdecl XAnimGetChildIndex_m( const XAnim *sourceTree, int nodeIndex, int childIndex )
{
    return childIndex + sourceTree->entries[nodeIndex].u.parent.firstChildIndex;
}

/* ---- XAnimGetAnimName  0x00487D80 ----  VERIFIED */
const char *__cdecl XAnimGetAnimName( const XAnim *sourceTree, int nodeIndex )
{
    const XAnimEntry *entry = &sourceTree->entries[nodeIndex];

    if ( entry->childCount )
        return "<non-leaf anim>";
    return *(const char **)entry->u.leafAsset;
}

/* ---- XAnimGetAnimTreeDebugName_m  0x00487DA0 ----  VERIFIED */
const char *__cdecl XAnimGetAnimTreeDebugName_m( const XAnim *sourceTree )
{
    return sourceTree->name;
}

/* ---- XAnimGetAnimTreeSize  0x00487DB0 ----  VERIFIED */
int __cdecl XAnimGetAnimTreeSize( const XAnim *sourceTree )
{
    return sourceTree->nodeCount;
}

/* ---- DObjUpdateOldServerTime_m  0x00487DC0 ----  VERIFIED */
void __cdecl DObjUpdateOldServerTime_m( DObj *dobj )
{
    XAnimTree *tree = dobj->tree;

    if ( tree )
    {
        xanim_currentTree = (int)tree;
        XAnimUpdateOldServerTime( 0 );
    }
}

/* ---- DObjInitServerTime  0x00487DE0 ----  VERIFIED */
/* serverTime is a float (stack-passed; retail tail-calls through its slot), stored as raw bits. */
void __cdecl DObjInitServerTime( DObj *dobj, float serverTime )
{
    XAnimTree *tree = dobj->tree;

    if ( tree )
    {
        *(float *)&xanim_currentTime = serverTime;
        xanim_currentTree = (int)tree;
        XAnimInitServerTime_m( 0 );
    }
}

/* ---- DObjUpdateClientInfo_m  0x00487E10 ----  VERIFIED */
void __cdecl DObjUpdateClientInfo_m(DObj *dobj, float dtime)
{
  XAnimTree *tree;

  xanim_numDeferredNotifies = 0;
  tree = dobj->tree;
  if ( tree )
  {
    xanim_notifyEntId = dobj->scrNotifyId;
    xanim_currentTree = (int)tree;
    *(float *)&xanim_currentTime = dtime;
    XAnimUpdateClientInfoInternal(0, dtime, 1);
  }
}

int XAnimIsLeafNode_m(const XAnim *tree,unsigned int index) {
 const XAnimEntry *entry=&tree->entries[index];
 return entry->childCount==0 || (entry->u.parent.flags&3)!=0;
}

int XAnimNotetrackExists_m(const XAnim *tree,unsigned int index,unsigned short name) {
 XAnimParts *parts=*(XAnimParts **)((char*)tree->entries[index].u.leafAsset+4);
 XAnimNotifyInfo *note=parts->notify;
 if(!note)return 0;
 while(note->name) { if(note->name==name)return 1; ++note; }
 return 0;
}

/* ---- DObjUpdateServerInfo  0x00487E50 ----  VERIFIED */
int __cdecl DObjUpdateServerInfo(DObj *dobj, float serverTime, int stopOnNotetrack)
{
  XAnimTree *tree;
  float frac;
  float clamped;

  tree = dobj->tree;
  if ( !tree )
    return 0;
  xanim_notifyEntId = dobj->scrNotifyId;
  xanim_currentTree = (int)tree;

  if ( !stopOnNotetrack )
  {
    XAnimUpdateServerInfoInternal(0, serverTime, 0);
    return 0;
  }

  frac = XAnimFindServerNoteTrack(0, serverTime);
  if ( frac != 1.0f )
  {
    clamped = frac * serverTime + 0.001f;
    if ( clamped <= serverTime )
    {
      XAnimUpdateServerInfoInternal(0, clamped, 1);
      return 1;
    }
  }
  XAnimUpdateServerInfoInternal(0, serverTime, 1);
  return 0;
}

/* ---- XAnimGetClientNotifies_m  0x00487EF0 ----  VERIFIED */
int __cdecl XAnimGetClientNotifies_m( XAnimDeferredNotify **out )
{
    *out = xanim_deferredNotifies;
    return xanim_numDeferredNotifies;
}

/* ---- DObjCalcAnim  0x00487F00 ----  VERIFIED */
void __cdecl DObjCalcAnim( DObj *dobj, unsigned int *requestBits )
{
    XAnimEvalStorage *storage = (XAnimEvalStorage *)dobj->evalStorage;
    unsigned char allSkipped;
    int k;

    allSkipped = 1;
    for ( k = 0; k < 4; ++k )
    {
        xanim_evalPosedBits[k] = ~requestBits[k] | ((unsigned int *)storage->evaluatedPartBits)[k];
        if ( ~xanim_evalPosedBits[k] )
            allSkipped = 0;
    }

    if ( !allSkipped )
    {
        int bone;
        int i;
        DObjAnimMat *out;

                *(unsigned int *)&storage->evaluatedPartBits[0] |= requestBits[0];
        xanim_evalSkipBits[0] = xanim_evalPosedBits[0];
        *(unsigned int *)&storage->evaluatedPartBits[4] |= requestBits[1];
        xanim_evalSkipBits[1] = xanim_evalPosedBits[1];
        *(unsigned int *)&storage->evaluatedPartBits[8] |= requestBits[2];
        xanim_evalSkipBits[2] = xanim_evalPosedBits[2];
        *(unsigned int *)&storage->evaluatedPartBits[12] |= requestBits[3];
        xanim_evalSkipBits[3] = xanim_evalPosedBits[3];

        xanim_evalDObj = (int)dobj;
        xanim_currentTree = (int)dobj->tree;
        xanim_evalChildCount = dobj->childCount;
        xanim_evalChildRefs = (int)dobj->childRefs;
        xanim_evalPartCount = dobj->partCount;

        out = (DObjAnimMat *)( (char *)storage + 0x30 + dobj->partCount * 0x40 );

        if ( dobj->tree )
        {
            xanim_evalPartBytes = 32 * dobj->partCount;
            ((byte *)&xanim_evalSkipBits[3])[3] = ((byte *)&xanim_evalPosedBits[3])[3] | 0x80;
            XAnimCalc( 0, 1.0f, out, 1, 0 );
        }

        bone = 0;
        for ( i = 0; i < xanim_evalChildCount; ++i )
        {
            __int16 ***partsData = *(__int16 ****)( **(unsigned int **)( *(unsigned int *)( xanim_evalChildRefs + 4 * i ) + 4 ) + 4 );
            __int16 *basePose;
            int   j;

            for ( j = *((__int16 *)partsData + 2); j; --j )
            {
                if ( ( (1 << (bone & 7)) & *((char *)xanim_evalPosedBits + (bone >> 3)) ) == 0 )
                {
                    out->quat[0]  = 0.0f;
                    out->quat[1]  = 0.0f;
                    out->quat[2]  = 0.0f;
                    out->quat[3]  = 1.0f;
                    out->trans[2] = 0.0f;
                    out->trans[1] = 0.0f;
                    out->trans[0] = 0.0f;
                }
                ++out;
                ++bone;
            }

            basePose = (__int16 *)partsData[3];
            for ( j = ***partsData - *((__int16 *)partsData + 2); j; --j )
            {
                if ( ( (1 << (bone & 7)) & *((char *)xanim_evalPosedBits + (bone >> 3)) ) == 0 )
                {
                    /* retail 0x4880CA: fild; fmul dword [0x568ED8] (float 0x38000100) */
                    out->quat[0]  = (float)basePose[0] * 0.000030518509f;
                    out->quat[1]  = (float)basePose[1] * 0.000030518509f;
                    out->quat[2]  = (float)basePose[2] * 0.000030518509f;
                    out->quat[3]  = (float)basePose[3] * 0.000030518509f;
                    out->trans[2] = 0.0f;
                    out->trans[1] = 0.0f;
                    out->trans[0] = 0.0f;
                }
                ++out;
                ++bone;
                basePose += 4;
            }
        }
        return;
    }
    return;
}

#ifdef __cplusplus
}
#endif
