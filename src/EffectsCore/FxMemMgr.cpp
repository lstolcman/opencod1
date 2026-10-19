/*
 * Reconstructed from Call of Duty 1.1 (Windows, CoDMP.exe).
 * Original translation unit:
 * /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/EffectsCore/FxMemMgr.cpp
 * Retail range 0x0048D290-0x0048D67F (FxMem_ClaimBlock and friends).
 * @fidelity: likely
 */

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The arena.  0x00A9CE58, 64 blocks of 32768.                         */

unsigned char   fxMemBlocks[0x200000];   /* 0x00A9CE58 */

int             fxPool_Emitter[2] = {  82, 0 };   /* 0x0140C9B8 */
int             fxPool_Electricity[2] = {  45, 0 };   /* 0x0140C9C0 */
int             fxPool_Model[2] = { 209, 0 };   /* 0x0140C9C8 */
int             fxPool_Particle[2] = { 103, 0 };   /* 0x0140C9D0 */
int             fxPool_Cylinder[2] = {  95, 0 };   /* 0x0140C9D8 */
int             fxPool_BoltFrame[2] = { 481, 0 };   /* 0x0140C9E0 */
int             fxPool_Light[2] = { 125, 0 };   /* 0x0140C9E8 */
int             fxPool_Decal[2] = {  75, 0 };   /* 0x0140C9F0 */
int             fxPool_OrientedParticle[2] = {  99, 0 };   /* 0x0140C9F8 */
int             fxPool_Tail[2] = {  95, 0 };   /* 0x0140CA00 */
int             fxPool_Effect[2] = { 148, 0 };   /* 0x0140CA08 */
int             fxPool_Quad[2] = {  58, 0 };   /* 0x0140CA10 */
int             fxPool_Line[2] = {  99, 0 };   /* 0x0140CA18 */
int             fxPool_PrimitiveTemplate[2] = {  54, 0 };   /* 0x0140758C */
int             fxPool_ScheduledEffect[2] = { 481, 0 };   /* 0x01407594 */

/* CFxScheduler::CFxScheduler (fx_unassigned.cpp, 0x00492B20). */
struct CFxScheduler { unsigned char storage[0x5418]; CFxScheduler(); };
extern void __cdecl   CFxScheduler__dtor( int a1 );                /* 0x00492980 */
extern int  __cdecl   FxMem_Init( void );                  /* 0x0048D230 */

extern unsigned int theFxScheduler[];                       /* 0x014075A0 */

/* ---- FxScheduler_static_fini  0x0053C230 ---- */
static void __cdecl FxScheduler_static_fini( void )
{
	CFxScheduler__dtor( (int)theFxScheduler );
}

/* ---- FxScheduler_static_init  0x0053C1E0 ---- */
static void __cdecl FxScheduler_static_init( void )
{
	( (CFxScheduler *)theFxScheduler )->CFxScheduler::CFxScheduler();
	atexit( FxScheduler_static_fini );
}

/* ---- FxMemMgr_static_init  0x0053BEE0 ---- */
static void __cdecl FxMemMgr_static_init( void )
{
	FxMem_Init();
}

/* The .CRT$XCU entries for theFxScheduler's constructor (retail __xc_a + 0x18) and the
   arena (__xc_a + 0x50); the CRT's _initterm calls them before WinMain. */
typedef void ( __cdecl *fxMemMgrStaticInit_t )( void );
#pragma data_seg( ".CRT$XCU" )
static fxMemMgrStaticInit_t fxMemMgrStaticInit[] = {
	FxScheduler_static_init,
	FxMemMgr_static_init,
};
#pragma data_seg()

#ifdef __cplusplus
}
#endif
