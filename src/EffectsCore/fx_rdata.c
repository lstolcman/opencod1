/*
 * fx_rdata.c -- EffectsCore globals whose real storage differs from the
 * `extern int` views the fx code addresses them by (fx_types.h, fx_unassigned.cpp).
 *
 * @fidelity: verified   (storage, sizes and the ten vtables -- no code here)
 *
 * This unit deliberately does NOT include EffectsCore/fx_types.h.  The
 * definitions below and that header's declarations of the same names have
 * different types, and they must never meet inside one translation unit; the
 * linker matches them by NAME alone.  Same arrangement, and same reason, as
 * the renderer blobs in client_mp/cl_refstorage.c.
 *
 * ======================================================================
 * theFxHelper (SFxHelper) @ 0x01407508 -- 132 bytes
 * ======================================================================
 *
 * SFxHelper::AdjustCamera (0x00499D60) takes its destination as a stack
 * argument and writes displacements +0x10 through +0x80 -- twenty-six of them,
 * a camera basis and a view record.  Both call sites (client_mp/cl_cgame_mp.c
 * and universal/memorytree.cpp) pass `&theFxHelper`.
 *
 * SIZE: AdjustCamera's highest displacement is +0x80, read/written as a dword,
 * and SFxHelper::CullSphere (0x0049A0D0) names the absolute address
 * 0x01407588 == base + 0x80.  The next object begins at 0x0140758C: the
 * CPrimitiveTemplate FxMemMgr pool, constructed by its own static-init thunk
 * at 0x0053C180.  The two pool handles at 0x0140758C and 0x01407594 sit
 * between this object and theFxScheduler at 0x014075A0.  Extent
 * 0x01407508..0x0140758B == 0x84 == 132.
 *
 * Tentative definition, not an initialised one: this is .bss in retail --
 * zero at load, filled by AdjustCamera and friends before use -- so what is
 * needed is STORAGE, not contents.  A tentative definition is a COMMON symbol.
 *
 * The eleven further shard names carved out of this same
 * object (0x0140750C..0x01407530 and 0x01407588) are aliased onto it by
 * EffectsCore/fx_types.h, which only EffectsCore/fx_unassigned.cpp includes.
 */

unsigned char theFxHelper[132];

/* theFxScheduler (CFxScheduler) @ 0x014075A0 -- 21636 bytes, after the two FxMemMgr
 * pool handles that follow theFxHelper.  The client (cl_cgame_mp.c), FxMemMgr.cpp,
 * fx_unassigned.cpp and memorytree.cpp address it as raw storage. */
unsigned int theFxScheduler[5409];

/*
 * ======================================================================
 * fxActiveEffects (SEffectList[1800]) @ 0x00C9CE60 -- 21600 bytes
 * ======================================================================
 *
 * The effect table.  Retail's three walks over it are bounded by the address
 * one past its end, `fxActiveEffectsEnd`, where a GenericParser2 token buffer starts.
 * fx_types.h carries the fxActiveEffectsEnd redirect and the three interior shards.
 *
 * SIZE: 0x00CA22C0 - 0x00C9CE60 == 0x5460 == 21600 == 1800 * 12, and 1800 is
 * the literal slot count FX_Save and FX_DrawStats_m count down from.
 *
 * Tentative definition for the same reason as theFxHelper: .bss in retail,
 * storage is what is needed, and largest-COMMON-wins gives it to every
 * consumer without a call-site edit.
 */
unsigned char fxActiveEffects[21600];

/*
 * ======================================================================
 * The twelve primitive vtables -- 336 bytes of function pointers
 * ======================================================================
 *
 * Ten at 0x00559EA0..0x00559FB8, two more at 0x00559450 and 0x0055946C.
 * Stride 0x1C == 28 == SEVEN function pointers; every slot is an address
 * inside the CEffect method range 0x0048DAD0..0x00491440.  Each ctor stores
 * its own table (`*result = &vtbl_CParticle;` and its siblings).
 *
 * SLOT ORDER: [0] scalar deleting dtor, [1] Die, [2] Update, [3] Cull,
 * [4] Draw, [5] TypeID, [6] Archive.  Inheritance shows up as shared slots --
 * five tables take CParticle::Die, CLight takes CEffect::Die -- and those are
 * reproduced exactly, not normalised.
 *
 * The seventy methods are declared below rather than through a header: this
 * unit must not see fx_types.h (see the note at the top of this file).
 * Retail's methods are __thiscall (`this` in ECX, callee pops the rest), so the
 * ones that take arguments are __fastcall here: one-argument methods are
 * byte-identical to __thiscall, and scalar_dtor / Archive carry a dead EDX
 * parameter in front of their stack argument.  Only the decorated name
 * (@name@bytes) has to agree with fx_unassigned.cpp's definitions; the methods
 * that ignore `this` stay plain __cdecl().
 *
 * These are INITIALISED, unlike the two objects above, so they are strong
 * definitions rather than COMMON; fx_types.h's `extern int` views of them bind
 * by name.
 */
#define FX_VFN( f )     ( (void (*)())( f ) )

extern void __fastcall CEffect__scalar_dtor( void *, int, int );  extern void CEffect__Die();            /* CEffect::Die  */
extern void __fastcall CEffect__Update( void * );       extern void CEffect__Cull();
extern void CEffect__Draw();            /* CEffect::Draw */
extern void CEffect__TypeID();       extern void __fastcall CEffect__Archive( void *, int, int );

extern void __fastcall CParticle__scalar_dtor( void *, int, int );          extern void __fastcall CParticle__Die( void * );
extern void __fastcall CParticle__Update( void * );               extern void __fastcall CParticle__Cull( void * );
extern void __fastcall CParticle__Draw( void * );                 extern void CParticle__TypeID();
extern void __fastcall CParticle__Archive( void *, int, int );

extern void __fastcall COrientedParticle__scalar_dtor( void *, int, int );  extern void __fastcall COrientedParticle__Update( void * );
extern void __fastcall COrientedParticle__Cull( void * );         extern void __fastcall COrientedParticle__Draw( void * );
extern void COrientedParticle__TypeID();       extern void __fastcall COrientedParticle__Archive( void *, int, int );

extern void __fastcall CLine__scalar_dtor( void *, int, int );    extern void CLine__Die();            /* CLine::Die */
extern void __fastcall CLine__Update( void * );         extern void __fastcall CLine__Cull( void * );
extern void __fastcall CLine__Draw( void * );           extern void CLine__TypeID();
extern void __fastcall CLine__Archive( void *, int, int );

extern void __fastcall CElectricity__scalar_dtor( void *, int, int );  extern void CElectricity__Die();       /* CElectricity::Die */
extern void __fastcall CElectricity__Update( void * );       extern void CElectricity__Cull();
extern void __fastcall CElectricity__Draw( void * );         extern void CElectricity__TypeID();
extern void __fastcall CElectricity__Archive( void *, int, int );

extern void __fastcall CTail__scalar_dtor( void *, int, int );    extern void __fastcall CTail__Update( void * );
extern void __fastcall CTail__Cull( void * );           extern void __fastcall CTail__Draw( void * );
extern void CTail__TypeID();         extern void __fastcall CTail__Archive( void *, int, int );

extern void __fastcall CCylinder__scalar_dtor( void *, int, int );     extern void __fastcall CCylinder__Update( void * );
extern void __fastcall CCylinder__Cull( void * );            extern void __fastcall CCylinder__Draw( void * );
extern void CCylinder__TypeID();          extern void __fastcall CCylinder__Archive( void *, int, int );

extern void __fastcall CEmitter__scalar_dtor( void *, int, int );      extern void __fastcall CEmitter__Update( void * );
extern void CEmitter__Cull();             extern void __fastcall CEmitter__Draw( void * );
extern void CEmitter__TypeID();           extern void __fastcall CEmitter__Archive( void *, int, int );

extern void __fastcall CLight__scalar_dtor( void *, int, int );   extern void __fastcall CLight__Update( void * );
extern void __fastcall CLight__Cull( void * );          extern void __fastcall CLight__Draw( void * );
extern void CLight__TypeID();        extern void __fastcall CLight__Archive( void *, int, int );

extern void __fastcall CFlash__scalar_dtor( void *, int, int );   extern void __fastcall CFlash__Update( void * );
extern void __fastcall CFlash__Cull( void * );          extern void __fastcall CFlash__Draw( void * );
extern void CFlash__TypeID();        extern void __fastcall CFlash__Archive( void *, int, int );

typedef struct {unsigned int flags;} FxCrtDeletingFlags;
extern char *__fastcall cand_CFxPrimType14__scalar_dtor(char *, FxCrtDeletingFlags);  extern void __fastcall cand_CFxPrimType14__Update( void * );
extern void cand_CFxPrimType14__Cull();         extern void __fastcall cand_CFxPrimType14__Draw( void * );
extern void cand_CFxPrimType14__TypeID();       extern void __fastcall cand_CFxPrimType14__Archive( void *, int, int );

extern char *__fastcall cand_CFxPrimType13__scalar_dtor(char *, FxCrtDeletingFlags);  extern void __fastcall cand_CFxPrimType13__Update( void * );
extern void cand_CFxPrimType13__Cull();         extern void __fastcall cand_CFxPrimType13__Draw( void * );
extern void cand_CFxPrimType13__TypeID();       extern void __fastcall cand_CFxPrimType13__Archive( void *, int, int );

/*
 * The two CoD1-only primitives sit apart from the other ten, at 0x00559450 and
 * 0x0055946C.  Both are 28 bytes; both take CEffect::Die; both are reached
 * from FX_Add and, for type 13, from FX_AddFlash (0x004A0FA0), which installs
 * vtbl_CFlash.  Class identities follow the TypeID slot (CoD:UO's
 * FX_EFFECT_TYPE_* numbering): 13 = CFlash (a CLight subclass), 14 = CQuad,
 * 15 = CDecal.  NOTE: the functions still spelled CFlash__* below are really
 * CDecal's methods (TypeID 15), and cand_CFxPrimType13__* are CFlash's.
 */

/* 0x00559450 -- CQuad, CoD1-only primitive, TypeID 14. */
void (*vtbl_CQuad[7])() = {
  FX_VFN( cand_CFxPrimType14__scalar_dtor ), CEffect__Die, FX_VFN( cand_CFxPrimType14__Update ),
  cand_CFxPrimType14__Cull, FX_VFN( cand_CFxPrimType14__Draw ), cand_CFxPrimType14__TypeID,
  FX_VFN( cand_CFxPrimType14__Archive )
};

/* 0x0055946C -- CFlash, CoD1-only primitive, TypeID 13. */
void (*vtbl_CFlash[7])() = {
  FX_VFN( cand_CFxPrimType13__scalar_dtor ), CEffect__Die, FX_VFN( cand_CFxPrimType13__Update ),
  cand_CFxPrimType13__Cull, FX_VFN( cand_CFxPrimType13__Draw ), cand_CFxPrimType13__TypeID,
  FX_VFN( cand_CFxPrimType13__Archive )
};

/* 0x00559EA0 -- CDecal, TypeID 15 (methods still spelled CFlash__*).  Die inherited from CParticle. */
void (*vtbl_CDecal[7])() = {
  FX_VFN( CFlash__scalar_dtor ), FX_VFN( CParticle__Die ), FX_VFN( CFlash__Update ),
  FX_VFN( CFlash__Cull ), FX_VFN( CFlash__Draw ), CFlash__TypeID,
  FX_VFN( CFlash__Archive )
};

/* 0x00559EBC -- CLine, TypeID 2. */
void (*vtbl_CLine[7])() = {
  FX_VFN( CLine__scalar_dtor ), CLine__Die, FX_VFN( CLine__Update ),
  FX_VFN( CLine__Cull ), FX_VFN( CLine__Draw ), CLine__TypeID,
  FX_VFN( CLine__Archive )
};

/* 0x00559ED8 -- CCylinder, TypeID 4.  Die inherited from CParticle. */
void (*vtbl_CCylinder[7])() = {
  FX_VFN( CCylinder__scalar_dtor ), FX_VFN( CParticle__Die ), FX_VFN( CCylinder__Update ),
  FX_VFN( CCylinder__Cull ), FX_VFN( CCylinder__Draw ), CCylinder__TypeID,
  FX_VFN( CCylinder__Archive )
};

/* 0x00559EF4 -- COrientedParticle, TypeID 8.  Derives from CParticle. */
void (*vtbl_COrientedParticle[7])() = {
  FX_VFN( COrientedParticle__scalar_dtor ), FX_VFN( CParticle__Die ),
  FX_VFN( COrientedParticle__Update ), FX_VFN( COrientedParticle__Cull ),
  FX_VFN( COrientedParticle__Draw ), COrientedParticle__TypeID,
  FX_VFN( COrientedParticle__Archive )
};

/* 0x00559F10 -- CParticle, TypeID 1. */
void (*vtbl_CParticle[7])() = {
  FX_VFN( CParticle__scalar_dtor ), FX_VFN( CParticle__Die ), FX_VFN( CParticle__Update ),
  FX_VFN( CParticle__Cull ), FX_VFN( CParticle__Draw ), CParticle__TypeID,
  FX_VFN( CParticle__Archive )
};

/* 0x00559F2C -- CElectricity, TypeID 9. */
void (*vtbl_CElectricity[7])() = {
  FX_VFN( CElectricity__scalar_dtor ), CElectricity__Die, FX_VFN( CElectricity__Update ),
  CElectricity__Cull, FX_VFN( CElectricity__Draw ), CElectricity__TypeID,
  FX_VFN( CElectricity__Archive )
};

/* 0x00559F48 -- CEmitter, TypeID 5.  Die inherited from CParticle. */
void (*vtbl_CEmitter[7])() = {
  FX_VFN( CEmitter__scalar_dtor ), FX_VFN( CParticle__Die ), FX_VFN( CEmitter__Update ),
  CEmitter__Cull, FX_VFN( CEmitter__Draw ), CEmitter__TypeID,
  FX_VFN( CEmitter__Archive )
};

/* 0x00559F64 -- CLight, TypeID 11.  Die inherited from CEffect, not CParticle. */
void (*vtbl_CLight[7])() = {
  FX_VFN( CLight__scalar_dtor ), CEffect__Die, FX_VFN( CLight__Update ),
  FX_VFN( CLight__Cull ), FX_VFN( CLight__Draw ), CLight__TypeID,
  FX_VFN( CLight__Archive )
};

/* 0x00559F80 -- CEffect, the base.  TypeID 0; Die and Draw are empty. */
void (*vtbl_CEffect[7])() = {
  FX_VFN( CEffect__scalar_dtor ), CEffect__Die, FX_VFN( CEffect__Update ),
  CEffect__Cull, CEffect__Draw, CEffect__TypeID,
  FX_VFN( CEffect__Archive )
};

/* 0x00559F9C -- CTail, TypeID 3.  Derives from CParticle. */
void (*vtbl_CTail[7])() = {
  FX_VFN( CTail__scalar_dtor ), FX_VFN( CParticle__Die ), FX_VFN( CTail__Update ),
  FX_VFN( CTail__Cull ), FX_VFN( CTail__Draw ), CTail__TypeID,
  FX_VFN( CTail__Archive )
};
