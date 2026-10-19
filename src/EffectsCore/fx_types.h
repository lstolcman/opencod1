/*
 * @fidelity: verified   (layouts only -- there is no code in this file)
 */

#ifndef FX_TYPES_H
#define FX_TYPES_H

/*
 * EffectsCore storage, declared with the scalar views fx_unassigned.cpp addresses it by.  The
 * objects themselves (sizes, initializers) are fx_rdata.c's (fxActiveEffects, the twelve primitive
 * vtables) and FxMemMgr.cpp's (fxMemBlocks, the FxMemMgr pools); neither unit includes this header.
 */
extern int fxActiveEffects;   /* 0x00C9CE60 */
extern int vtbl_CQuad;   /* 0x00559450 */
extern int vtbl_CFlash;   /* 0x0055946C */
extern int vtbl_CDecal;   /* 0x00559EA0 */
extern int vtbl_CLine;   /* 0x00559EBC */
extern int vtbl_CCylinder;   /* 0x00559ED8 */
extern int vtbl_COrientedParticle;   /* 0x00559EF4 */
extern int vtbl_CParticle;   /* 0x00559F10 */
extern int vtbl_CElectricity;   /* 0x00559F2C */
extern int vtbl_CEmitter;   /* 0x00559F48 */
extern int vtbl_CLight;   /* 0x00559F64 */
extern int vtbl_CEffect;   /* 0x00559F80 */
extern int vtbl_CTail;   /* 0x00559F9C */
extern int fxPool_PrimitiveTemplate;   /* 0x0140758C */
extern int fxPool_ScheduledEffect;   /* 0x01407594 */
extern int fxPool_Emitter;   /* 0x0140C9B8 */
extern int fxPool_Electricity;   /* 0x0140C9C0 */
extern int fxPool_Model;   /* 0x0140C9C8 */
extern int fxPool_Particle;   /* 0x0140C9D0 */
extern int fxPool_Cylinder;   /* 0x0140C9D8 */
extern int fxPool_BoltFrame;   /* 0x0140C9E0 */
extern int fxPool_Light;   /* 0x0140C9E8 */
extern int fxPool_Decal;   /* 0x0140C9F0 */
extern int fxPool_OrientedParticle;   /* 0x0140C9F8 */
extern int fxPool_Tail;   /* 0x0140CA00 */
extern int fxPool_Effect;   /* 0x0140CA08 */
extern int fxPool_Quad;   /* 0x0140CA10 */
extern int fxPool_Line;   /* 0x0140CA18 */
extern unsigned char fxMemBlocks[0x200000];   /* 0x00A9CE58 */

#define FX_EFFECT_TEMPLATE_NAME_CAPACITY       64
#define FX_EFFECT_TEMPLATE_PRIMITIVE_CAPACITY  24
#define FX_EFFECT_TEMPLATE_SIZE                0xA8

#define FX_EFFECT_TEMPLATE_COUNT               128

#define FX_EFFECT_ID_NONE                      0
#define FX_EFFECT_ID_FIRST                     1

#define FX_SCHEDULER_TEMPLATES_OFFSET          0x0000
#define FX_SCHEDULER_EFFECT_IDS_MAP_OFFSET     0x5400
#define FX_SCHEDULER_SCHEDULED_LIST_OFFSET     0x540C
#define FX_SCHEDULER_SCHEDULED_HEAD_OFFSET     0x5410
#define FX_SCHEDULER_SCHEDULED_SIZE_OFFSET     0x5414
#define FX_SCHEDULER_SIZE                      0x5418

/*
 * CFxScheduler::SaveState (0x004970E0) reads two list heads by absolute
 * address.  Both are fields of the global theFxScheduler (0x014075A0), set up
 * by its ctor (0x00492B20): 0x0140C9A4 == +0x5404, the effect-id map's
 * _Myhead, and 0x0140C9B0 == +0x5410, the scheduled-effect list's _Myhead.
 * The "Scheduled FX" debug print (0x0049F2F0) and 0x0049ED40 read
 * 0x0140C9B4 == +0x5414, the scheduled-effect list's _Mysize, the same way.
 * They are aliased onto that storage, like the theFxHelper shards below.
 */
#define FX_SCHEDULER_BASE   ((unsigned char *)theFxScheduler)

#undef  fxSchedulerEffectIdsHead
#define fxSchedulerEffectIdsHead          (*(int *)(FX_SCHEDULER_BASE + FX_SCHEDULER_EFFECT_IDS_MAP_OFFSET + 4))   /* 0x0140C9A4 */
#undef  fxSchedulerScheduledEffectsHead
#define fxSchedulerScheduledEffectsHead   (*(int *)(FX_SCHEDULER_BASE + FX_SCHEDULER_SCHEDULED_HEAD_OFFSET))       /* 0x0140C9B0 */
#undef  fxScheduledCount
#define fxScheduledCount                  ((char *)(FX_SCHEDULER_BASE + FX_SCHEDULER_SCHEDULED_SIZE_OFFSET))      /* 0x0140C9B4 */

#define FX_SCHEDULED_EFFECT_SIZE               0x44

#define FX_PRIMITIVE_TEMPLATE_SIZE   0x25C

#define FX_PRIMITIVE_TEMPLATE_RANGE_SIZE       0x08

#define FX_PRIMITIVE_TEMPLATE_NAME_OFFSET      0x08
#define FX_PRIMITIVE_TEMPLATE_DELAY_OFFSET     0x4C
#define FX_PRIMITIVE_TEMPLATE_COUNT_OFFSET     0x54
#define FX_PRIMITIVE_TEMPLATE_CULLRADIUS_OFFSET 0x64
#define FX_PRIMITIVE_TEMPLATE_FLAGS_OFFSET     0xB8   /* int              */
#define FX_PRIMITIVE_TEMPLATE_SPAWNFLAGS_OFFSET 0xBC  /* int, bit 0x2000  */
#define FX_PRIMITIVE_TEMPLATE_VECTOR_STRIDE    0x10

#define FX_HELPER_BASE   ((unsigned char *)&theFxHelper)

#undef  fxMTime
#define fxMTime   (*(int   *)(FX_HELPER_BASE + 0x04))
#undef  fxHelperOldTime
#define fxHelperOldTime   (*(int   *)(FX_HELPER_BASE + 0x08))
#undef  fxHelperFrameTime
#define fxHelperFrameTime   (*(int   *)(FX_HELPER_BASE + 0x0C))
#undef  fxHelperArchivedTimingState
#define fxHelperArchivedTimingState   (*(int   *)(FX_HELPER_BASE + 0x10))
#undef  fxHelperViewOriginX
#define fxHelperViewOriginX     (*(float *)(FX_HELPER_BASE + 0x14))
#undef  fxHelperViewOriginY
#define fxHelperViewOriginY     (*(float *)(FX_HELPER_BASE + 0x18))
#undef  fxHelperViewOriginZ
#define fxHelperViewOriginZ     (*(float *)(FX_HELPER_BASE + 0x1C))
#undef  fxHelperCullPlane0NormalX
#define fxHelperCullPlane0NormalX     (*(float *)(FX_HELPER_BASE + 0x20))
#undef  fxHelperCullPlane0NormalY
#define fxHelperCullPlane0NormalY     (*(float *)(FX_HELPER_BASE + 0x24))
#undef  fxHelperCullPlane0NormalZ
#define fxHelperCullPlane0NormalZ     (*(float *)(FX_HELPER_BASE + 0x28))
#undef  fxHelperCullPlaneCount
#define fxHelperCullPlaneCount   (*(int   *)(FX_HELPER_BASE + 0x80))

#define FX_ACTIVE_SLOT_SIZE   12
#define FX_ACTIVE_MAX         1800
#define FX_ACTIVE_BASE        ((unsigned char *)&fxActiveEffects)
#define FX_ACTIVE_BYTES       ( FX_ACTIVE_SLOT_SIZE * FX_ACTIVE_MAX )

#undef  fxActiveEffectsEnd
#define fxActiveEffectsEnd     ( FX_ACTIVE_BASE + FX_ACTIVE_BYTES )
#undef  fxActiveEffectsSlot0Next
#define fxActiveEffectsSlot0Next    (*(int      *)(FX_ACTIVE_BASE + 0x0008))
#undef  fxActiveEffectsSlot1
#define fxActiveEffectsSlot1      (*(unsigned int *)(FX_ACTIVE_BASE + 0x000C))
#undef  fxActiveEffectsLastNext
#define fxActiveEffectsLastNext    (*(int      *)(FX_ACTIVE_BASE + 0x545C))

/*
 * Layouts of the FX primitives and their templates (Raven's EffectsCore, the
 * Jedi Outcast / SoF2 effects system; CoD 1.1 adds the bolt frame, size2,
 * length and a few more ranges).  Member names follow the Mac 1.x symbols'
 * accessors (CParticle::SetSizeStart, CEmitter::SetLastBoltOrg, ...) and
 * Raven's m-prefix convention; offsets are pinned by the setters, the archive
 * routines, the CPrimitiveTemplate parsers and the pool element sizes
 * (CParticle 316, CEmitter 396, ...).  Code reaches the members through the
 * FX_*() accessor casts below; these are layout descriptions only (no
 * vtables, no methods).
 */
#ifdef __cplusplus

/* cgame's refEntity_t (cgame_mp/cg_public.h), 156 bytes, embedded in CEffect. */
typedef struct FxRefEntity_s
{
    int            reType;              /* +0x00 */
    int            renderfx;            /* +0x04 */
    int            hModel;              /* +0x08 */
    float          lightingOrigin[3];   /* +0x0C */
    float          shadowPlane;         /* +0x18 */
    float          axis[3][3];          /* +0x1C */
    float          nonNormalizedAxes;   /* +0x40  FX: uniform scale */
    float          origin[3];           /* +0x44 */
    int            frame;               /* +0x50 */
    float          oldorigin[3];        /* +0x54 */
    int            oldframe;            /* +0x60 */
    float          spriteSize;          /* +0x64 */
    int            customShader;        /* +0x68 */
    unsigned char  shaderRGBA[4];       /* +0x6C */
    float          shaderTexCoord[2];   /* +0x70 */
    float          shaderTime;          /* +0x78 */
    float          radius;              /* +0x7C */
    float          rotation;            /* +0x80 */
    float          fireRiseDir[3];      /* +0x84 */
    void          *dobj;                /* +0x90 */
    void          *owner;               /* +0x94 */
    unsigned char  reserved_0x98[4];    /* +0x98 */
} FxRefEntity;

/* CEffect: the primitive base class (vftable slot 0 = scalar deleting dtor). */
struct CEffect
{
    void          *vftable;             /* +0x00 */
    float          mOrigin1[3];         /* +0x04 */
    int            mTimeStart;          /* +0x10 */
    int            mTimeEnd;            /* +0x14 */
    int            mFlags;              /* +0x18 */
    float          mMin[3];             /* +0x1C */
    float          mMax[3];             /* +0x28 */
    int            mImpactFxID;         /* +0x34 */
    int            mDeathFxID;          /* +0x38 */
    FxRefEntity    mRefEnt;             /* +0x3C */
    char          *mBoltFrame;          /* +0xD8  CFxBoltFramePtr */
};

struct CParticle : CEffect
{
    float          mVel[3];             /* +0xDC */
    float          mAccel[3];           /* +0xE8 */
    float          mSizeStart;          /* +0xF4 */
    float          mSizeEnd;            /* +0xF8 */
    float          mSizeParm;           /* +0xFC */
    float          mSize2Start;         /* +0x100 */
    float          mSize2End;           /* +0x104 */
    float          mSize2Parm;          /* +0x108 */
    float          mRGBStart[3];        /* +0x10C */
    float          mRGBEnd[3];          /* +0x118 */
    float          mRGBParm;            /* +0x124 */
    float          mAlphaStart;         /* +0x128 */
    float          mAlphaEnd;           /* +0x12C */
    float          mAlphaParm;          /* +0x130 */
    float          mRotationDelta;      /* +0x134 */
    float          mElasticity;         /* +0x138 */
};

struct COrientedParticle : CParticle
{
    float          mNormal[3];          /* +0x13C */
};

struct CLine : CParticle
{
    float          mOrigin2[3];         /* +0x13C */
};

struct CTail : CParticle
{
    float          mOldOrigin[3];       /* +0x13C */
    float          mLengthStart;        /* +0x148 */
    float          mLengthEnd;          /* +0x14C */
    float          mLengthParm;         /* +0x150 */
    float          mLength;             /* +0x154 */
};

/* CCylinder derives from CTail (as in Raven's FxPrimitives.h); its normal
 * lives in mRefEnt.axis[0] and +0x13C..+0x147 are unused. */
struct CCylinder : CTail
{
};

/* CElectricity derives from CLine; the tail is its bolt state, archived whole. */
struct CElectricity : CLine
{
    float          mChaos;              /* +0x148 */
    unsigned char  reserved_0x14C[384]; /* +0x14C */
};

struct CEmitter : CParticle
{
    float          mOldOrigin[3];       /* +0x13C  SetLastOrg */
    float          mOldVelocity[3];     /* +0x148  SetLastVel */
    float          mLastBoltOrigin[3];  /* +0x154  SetLastBoltOrg */
    int            mOldTime;            /* +0x160  SetOldTime */
    float          mStepDistance;       /* +0x164  density +- variance, re-rolled per emit */
    float          mAngles[3];          /* +0x168 */
    float          mAngleDelta[3];      /* +0x174 */
    int            mEmitterFxID;        /* +0x180 */
    float          mDensity;            /* +0x184 */
    float          mVariance;           /* +0x188 */
};

/* CDecal (TypeID 15, vtbl_CDecal; its methods are still spelled CFlash__* and
 * its spawner cand_CFlash_Create): Raven's CPoly -- a small rotating polygon
 * (436-byte "Decal" pool element). */
struct CDecal : CParticle
{
    int            mCount;              /* +0x13C  SetNumVerts */
    float          mRotDelta[3];        /* +0x140  SetRot */
    int            mTimeStamp;          /* +0x14C  SetMotionTimeStamp */
    float          mOrg[5][3];          /* +0x150 */
    float          mST[5][2];           /* +0x18C */
};

struct CLight : CEffect
{
    float          mSizeStart;          /* +0xDC */
    float          mSizeEnd;            /* +0xE0 */
    float          mSizeParm;           /* +0xE4 */
    float          mRGBStart[3];        /* +0xE8 */
    float          mRGBEnd[3];          /* +0xF4 */
    float          mRGBParm;            /* +0x100 */
};

/* CFxRange: a [min, max] pair, rolled with Com_RandFloatRange_m when they differ. */
struct CFxRange
{
    float          mMin;
    float          mMax;
#ifdef __cplusplus
    CFxRange();
#endif
};

/* CMediaHandles: a std::vector<int> of shader / sound / model / effect handles. */
struct CMediaHandles
{
    unsigned char  allocator[4];        /* +0x00 */
    int           *first;               /* +0x04 */
    int           *last;                /* +0x08 */
    int           *end;                 /* +0x0C */
#ifdef __cplusplus
    CMediaHandles();                    /* vector(): _Buy(0) */
    ~CMediaHandles();                   /* ~vector(): _Tidy() */
#endif
};

/* CPrimitiveTemplate: one primitive of an effect, as parsed from the .efx file. */
struct CPrimitiveTemplate
{
    unsigned char  mCopy;               /* +0x000 */
    unsigned char  pad_0x01[3];
    int            mRefCount;           /* +0x004 */
    char           mName[32];           /* +0x008 */
    char           mMaterialImpact[32]; /* +0x028 */
    int            mType;               /* +0x048 */
    CFxRange       mSpawnDelay;         /* +0x04C */
    CFxRange       mSpawnCount;         /* +0x054 */
    CFxRange       mLife;               /* +0x05C */
    int            mCullRange;          /* +0x064 */
    CMediaHandles  mMediaHandles;       /* +0x068 */
    CMediaHandles  mImpactFxHandles;    /* +0x078 */
    CMediaHandles  mDeathFxHandles;     /* +0x088 */
    CMediaHandles  mEmitterFxHandles;   /* +0x098 */
    CMediaHandles  mPlayFxHandles;      /* +0x0A8 */
    int            mFlags;              /* +0x0B8 */
    int            mSpawnFlags;         /* +0x0BC */
    unsigned char  mNonUniformScale;    /* +0x0C0 */
    unsigned char  pad_0xC1[3];
    float          mMin[3];             /* +0x0C4 */
    float          mMax[3];             /* +0x0D0 */
    CFxRange       mOrigin1X, mOrigin1Y, mOrigin1Z;          /* +0x0DC */
    CFxRange       mOrigin2X, mOrigin2Y, mOrigin2Z;          /* +0x0F4 */
    CFxRange       mRadius;             /* +0x10C */
    CFxRange       mHeight;             /* +0x114 */
    CFxRange       mWindModifier;       /* +0x11C */
    CFxRange       mRotation;           /* +0x124 */
    CFxRange       mRotationDelta;      /* +0x12C */
    CFxRange       mAngle1, mAngle2, mAngle3;                /* +0x134 */
    CFxRange       mAngle1Delta, mAngle2Delta, mAngle3Delta; /* +0x14C */
    CFxRange       mVelX, mVelY, mVelZ;                      /* +0x164 */
    CFxRange       mAccelX, mAccelY, mAccelZ;                /* +0x17C */
    CFxRange       mGravity;            /* +0x194 */
    CFxRange       mDensity;            /* +0x19C */
    CFxRange       mVariance;           /* +0x1A4 */
    CFxRange       mRedStart, mGreenStart, mBlueStart;       /* +0x1AC */
    CFxRange       mRedEnd, mGreenEnd, mBlueEnd;             /* +0x1C4 */
    CFxRange       mRGBParm;            /* +0x1DC */
    CFxRange       mAlphaStart;         /* +0x1E4 */
    CFxRange       mAlphaEnd;           /* +0x1EC */
    CFxRange       mAlphaParm;          /* +0x1F4 */
    CFxRange       mSizeStart;          /* +0x1FC */
    CFxRange       mSizeEnd;            /* +0x204 */
    CFxRange       mSizeParm;           /* +0x20C */
    CFxRange       mSize2Start;         /* +0x214 */
    CFxRange       mSize2End;           /* +0x21C */
    CFxRange       mSize2Parm;          /* +0x224 */
    CFxRange       mLengthStart;        /* +0x22C */
    CFxRange       mLengthEnd;          /* +0x234 */
    CFxRange       mLengthParm;         /* +0x23C */
    CFxRange       mTexCoordS;          /* +0x244 */
    CFxRange       mTexCoordT;          /* +0x24C */
    CFxRange       mElasticity;         /* +0x254 */
#ifdef __cplusplus
    CPrimitiveTemplate();
    ~CPrimitiveTemplate();
    void *operator new( unsigned int size );   /* the FxMemMgr pool */
    void  operator delete( void *element );
#endif
};

typedef char FxEffectSizeCheck[ sizeof( CEffect ) == 220 ? 1 : -1 ];
typedef char FxParticleSizeCheck[ sizeof( CParticle ) == 316 ? 1 : -1 ];
typedef char FxOrientedParticleSizeCheck[ sizeof( COrientedParticle ) == 328 ? 1 : -1 ];
typedef char FxLineSizeCheck[ sizeof( CLine ) == 328 ? 1 : -1 ];
typedef char FxTailSizeCheck[ sizeof( CTail ) == 344 ? 1 : -1 ];
typedef char FxCylinderSizeCheck[ sizeof( CCylinder ) == 344 ? 1 : -1 ];
typedef char FxElectricitySizeCheck[ sizeof( CElectricity ) == 716 ? 1 : -1 ];
typedef char FxEmitterSizeCheck[ sizeof( CEmitter ) == 396 ? 1 : -1 ];
typedef char FxDecalSizeCheck[ sizeof( CDecal ) == 436 ? 1 : -1 ];
typedef char FxLightSizeCheck[ sizeof( CLight ) == 260 ? 1 : -1 ];
typedef char FxPrimitiveTemplateSizeCheck[ sizeof( CPrimitiveTemplate ) == FX_PRIMITIVE_TEMPLATE_SIZE ? 1 : -1 ];

/* Typed views of the FX objects the decompiled members keep as int / char *. */
#define FX_EFFECT( p )     ( (CEffect *)(p) )
#define FX_PARTICLE( p )   ( (CParticle *)(p) )
#define FX_ORIENTED( p )   ( (COrientedParticle *)(p) )
#define FX_LINE( p )       ( (CLine *)(p) )
#define FX_TAIL( p )       ( (CTail *)(p) )
#define FX_CYLINDER( p )   ( (CCylinder *)(p) )
#define FX_ELECTRICITY( p ) ( (CElectricity *)(p) )
#define FX_EMITTER( p )    ( (CEmitter *)(p) )
#define FX_DECAL( p )      ( (CDecal *)(p) )
#define FX_LIGHT( p )      ( (CLight *)(p) )
#define FX_TMPL( p )       ( (CPrimitiveTemplate *)(p) )

/* Member offsets for code that keeps the raw `*(T *)(p + offset)` form: the
 * constant is the same int literal the compiler sees, so the access is
 * spelled by name without changing its expression shape. */
#define FX_EFFECT_OFS( m )      ( (int)offsetof( CEffect, m ) )
#define FX_PARTICLE_OFS( m )    ( (int)offsetof( CParticle, m ) )
#define FX_ORIENTED_OFS( m )    ( (int)offsetof( COrientedParticle, m ) )
#define FX_LINE_OFS( m )        ( (int)offsetof( CLine, m ) )
#define FX_TAIL_OFS( m )        ( (int)offsetof( CTail, m ) )
#define FX_CYLINDER_OFS( m )    ( (int)offsetof( CCylinder, m ) )
#define FX_ELECTRICITY_OFS( m ) ( (int)offsetof( CElectricity, m ) )
#define FX_EMITTER_OFS( m )     ( (int)offsetof( CEmitter, m ) )
#define FX_DECAL_OFS( m )       ( (int)offsetof( CDecal, m ) )
#define FX_LIGHT_OFS( m )       ( (int)offsetof( CLight, m ) )
#define FX_TMPL_OFS( m )        ( (int)offsetof( CPrimitiveTemplate, m ) )

/* CFxBoltFrame: a cached bone orientation shared by the primitives bolted to
 * it (68-byte pool element; opencod2's FxBoltFrame keeps next @ 0x38 and
 * mBolt @ 0x3C). */
struct SFxBoltInfo
{
    int            dobjHandle;          /* +0x00 */
    int            boneIndex;           /* +0x04 */
};

struct CFxBoltFrame
{
    int            mRefCount;           /* +0x00 */
    int            mSkelTimeStamp;      /* +0x04  vs com_skelTimeStamp */
    float          mOrientation[12];    /* +0x08  origin, then axis[3][3] */
    CFxBoltFrame  *mNext;               /* +0x38  fxBoltFrames list */
    SFxBoltInfo    mBolt;               /* +0x3C */
};

/* theFxHelper's camera (opencod2 FxHelper.mCamera @ 0x14, numPlanes @ 0x6C). */
struct FxCullPlane
{
    float          normal[3];
    float          dist;
};

struct FxCamera
{
    float          origin[3];           /* +0x00 */
    FxCullPlane    planes[6];           /* +0x0C */
    int            numPlanes;           /* +0x6C */
};

struct SFxHelper
{
    int            reserved_0x00;       /* +0x00 */
    int            mTime;               /* +0x04 */
    int            mOldTime;            /* +0x08 */
    int            mFrameTime;          /* +0x0C */
    int            mArchivedTimingState;/* +0x10 */
    FxCamera       mCamera;             /* +0x14 */
};

/* The head of cgame's refdef_t (cgame_mp/cg_public.h) as SFxHelper reads it. */
struct FxRefdef
{
    int            x, y, width, height; /* +0x00 */
    float          fov_x, fov_y;        /* +0x10 */
    float          vieworg[3];          /* +0x18 */
    float          viewaxis[3][3];      /* +0x24 */
};

/* SEffectTemplate: one registered effect (FX_EFFECT_TEMPLATE_SIZE bytes). */
struct SEffectTemplate
{
    unsigned char  mInUse;              /* +0x00 */
    unsigned char  mCopy;               /* +0x01 */
    char           mEffectName[64];     /* +0x02 */
    unsigned char  pad_0x42[2];
    int            mPrimitiveCount;     /* +0x44 */
    CPrimitiveTemplate *mPrimitives[24];/* +0x48 */
};

/* SScheduledEffect: a delayed primitive spawn (FX_SCHEDULED_EFFECT_SIZE bytes). */
struct SScheduledEffect
{
    int            mEffectID;           /* +0x00 */
    int            mPrimitiveIndex;     /* +0x04 */
    int            mStartTime;          /* +0x08 */
    SFxBoltInfo    mBolt;               /* +0x0C */
    float          mOrigin[3];          /* +0x14 */
    float          mAxis[3][3];         /* +0x20 */
};

typedef char FxBoltFrameSizeCheck[ sizeof( CFxBoltFrame ) == 68 ? 1 : -1 ];
typedef char FxHelperCameraCheck[ sizeof( SFxHelper ) == 0x84 ? 1 : -1 ];
typedef char FxEffectTemplateSizeCheck[ sizeof( SEffectTemplate ) == FX_EFFECT_TEMPLATE_SIZE ? 1 : -1 ];
typedef char FxScheduledEffectSizeCheck[ sizeof( SScheduledEffect ) == FX_SCHEDULED_EFFECT_SIZE ? 1 : -1 ];

#define FX_BOLTFRAME( p )        ( (CFxBoltFrame *)(p) )
#define FX_HELPER( p )           ( (SFxHelper *)(p) )
#define FX_REFDEF( p )           ( (FxRefdef *)(p) )
#define FX_EFFECT_TEMPLATE( p )  ( (SEffectTemplate *)(p) )
#define FX_SCHED( p )            ( (SScheduledEffect *)(p) )
#define FX_BOLTFRAME_OFS( m )    ( (int)offsetof( CFxBoltFrame, m ) )
#define FX_HELPER_OFS( m )       ( (int)offsetof( SFxHelper, m ) )
#define FX_REFDEF_OFS( m )       ( (int)offsetof( FxRefdef, m ) )
#define FX_EFFECT_TEMPLATE_OFS( m ) ( (int)offsetof( SEffectTemplate, m ) )
#define FX_SCHED_OFS( m )        ( (int)offsetof( SScheduledEffect, m ) )

/* The scheduler's Dinkumware containers (VC7 <vector>/<list>/<xtree>) and the
 * FxMemMgr pools, as their raw offset accesses name them via *_OFS(). */
struct FxIntVectorLayout          /* std::vector<int> */
{
    unsigned char  allocator[4];        /* +0x00 */
    int           *_Myfirst;            /* +0x04 */
    int           *_Mylast;             /* +0x08 */
    int           *_Myend;              /* +0x0C */
};

struct FxScheduledListLayout      /* std::list<SScheduledEffect *> */
{
    unsigned char  allocator[4];        /* +0x00 */
    void          *_Myhead;             /* +0x04 */
    unsigned int   _Mysize;             /* +0x08 */
};

struct FxListNodeLayout
{
    void          *_Next;               /* +0x00 */
    void          *_Prev;               /* +0x04 */
    void          *_Myval;              /* +0x08 */
};

struct FxEffectIdMapLayout        /* std::map<std::string, int> */
{
    unsigned char  comparator[4];       /* +0x00 */
    void          *_Myhead;             /* +0x04 */
    unsigned int   _Mysize;             /* +0x08 */
};

struct FxPoolLayout               /* FxMemMgr pool: { elements per 32K block, head block } */
{
    int            mElementsPerBlock;   /* +0x00 */
    int            mHeadBlock;          /* +0x04 */
};

#define FX_VEC_OFS( m )       ( (int)offsetof( FxIntVectorLayout, m ) )
#define FX_LIST_OFS( m )      ( (int)offsetof( FxScheduledListLayout, m ) )
#define FX_LISTNODE_OFS( m )  ( (int)offsetof( FxListNodeLayout, m ) )
#define FX_TREE_OFS( m )      ( (int)offsetof( FxEffectIdMapLayout, m ) )
#define FX_POOL_OFS( m )      ( (int)offsetof( FxPoolLayout, m ) )

/* GenericParser2's parse tree (Raven's genericparser2.h: CGPObject, and the
 * CGPValue / CGPGroup built on it), as the .efx parsers walk it. */
struct CGPObject
{
    const char    *mName;               /* +0x00 */
    CGPObject     *mNext;               /* +0x04 */
    CGPObject     *mInOrderNext;        /* +0x08 */
    CGPObject     *mInOrderPrevious;    /* +0x0C */
};

struct CGPValue : CGPObject
{
    CGPObject     *mList;               /* +0x10 */
};

struct CGPGroup : CGPObject
{
    CGPValue      *mPairs;              /* +0x10 */
    CGPValue      *mInOrderPairs;       /* +0x14 */
    CGPValue      *mCurrentPair;        /* +0x18 */
    CGPGroup      *mSubGroups;          /* +0x1C */
};

#define FX_GPOBJ_OFS( m )     ( (int)offsetof( CGPObject, m ) )
#define FX_GPVALUE_OFS( m )   ( (int)offsetof( CGPValue, m ) )
#define FX_GPGROUP_OFS( m )   ( (int)offsetof( CGPGroup, m ) )

#endif /* __cplusplus */

#ifdef __cplusplus
/*
 * std::string as retail built it: the VC7 STL's basic_string<char> (layout
 * {allocator, _Bx[16], _Mysize, _Myres}).  The EffectsCore and StringEd STL
 * code is this instantiation, and LIBCPMT's _Xran/_Xlen bind to the same
 * COMDATs (retail 0x00497480 is the const char * constructor they call, and
 * 0x004974C0 the destructor every string temporary's EH unwind action jumps
 * to).  Only the members the tree defines itself are declared here; the
 * rest are the stl_fxString_* / FxString stand-ins.  Needs std::_String_base.
 */
extern "C++"
{
namespace std
{
template<class _Elem> struct char_traits;
template<class _Ty> class allocator;
template<class _Elem, class _Traits, class _Ax> class basic_string;

template<> class basic_string<char, char_traits<char>, allocator<char> > : public _String_base
{
public:
  basic_string();                    /* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ */
  basic_string( const basic_string &right );   /* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z */
  basic_string( const char *ptr );   /* ??0?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@PBD@Z */
  ~basic_string();                   /* ??1?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@XZ */

  void         *alval;               /* +0x00  _Alval (allocator<char>, empty) */
  union { char buf[16]; char *ptr; } bx;   /* +0x04  _Bx: small-string buffer (a char array: named locals get a /GS cookie) or heap pointer */
  unsigned int  size;                /* +0x14  _Mysize */
  unsigned int  capacity;            /* +0x18  _Myres */
};
}
}
typedef std::basic_string<char, std::char_traits<char>, std::allocator<char> > FxStdString;

/*
 * <stdexcept>: std::logic_error, std::length_error and std::out_of_range as
 * VC7's header defines them (Dinkumware, 0x28 bytes: vfptr,
 * exception::_m_what / _m_doFree, logic_error::_Str).  The std::exception
 * base is LIBCMT's own class (stdexcpt.obj; include <exception> first).
 * The EffectsCore and StringEd containers' _Xlen / erase paths throw them;
 * their inline members are defined once, in fx_unassigned.cpp (retail
 * 0x00492500..0x00492760).
 */
extern "C++"
{
namespace std
{
class logic_error : public exception
{
public:
  explicit logic_error( const FxStdString &message );
  logic_error( const logic_error &right );
  virtual ~logic_error();
  virtual const char *what() const throw();
private:
  FxStdString _Str;
};

class length_error : public logic_error
{
public:
  explicit length_error( const FxStdString &message );
  length_error( const length_error &right );
  virtual ~length_error();
};

class out_of_range : public logic_error
{
public:
  explicit out_of_range( const FxStdString &message );
  out_of_range( const out_of_range &right );
  virtual ~out_of_range();
};
}
}
#endif /* __cplusplus */

#endif
