/*
 * script/scr_vm_state.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_vm.cpp
 *
 * @fidelity: verified
 */

#include "scr_local.h"

extern void Var_Init__Fv( void );
extern void VM_Init__Fv( void );

/* retail keeps these as word handles and two load-flag bytes */
#define SCR_HANDLE( g )     ( *(unsigned short *)&(g) )
#define SCR_LOADING_SCRIPTS ( *( (unsigned char *)&scrVarPub_canonicalStrCountAndLoadFlags + 2 ) )
#define SCR_LOADING_ANIMS   ( *( (unsigned char *)&scrVarPub_canonicalStrCountAndLoadFlags + 3 ) )

unsigned char   scr_inited;

/* scr_timeArrayId is defined in scr_vm.cpp */

int             scr_loading;

/* ---- Scr_Init  0x004758F0 ---- VERIFIED */
void Scr_Init( int developer, int developerScript ) {
	scrVarPub_developer = developer;
	scrVarPub_developerScript = developerScript;

	Var_Init__Fv();
	VM_Init__Fv();

	SCR_LOADING_SCRIPTS = 0;                    /* byte 0x008E60BE */
	SCR_LOADING_ANIMS = 0;                      /* byte 0x008E60BF */
	scrCompilePub_scriptsPos = 0;
	scrCompilePub_loadedScripts = 0;
	scrAnimPub_treeRoot = 0;

	scr_inited = qtrue;
}

/* ---- Scr_Shutdown  0x00475940 ---- */
void Scr_Shutdown( void ) {
	if ( scr_inited ) {
		scr_inited = qfalse;
	}
}

/* ---- Scr_Abort  0x00475960 ---- */
void Scr_Abort( void ) {
	scr_timeArrayId = 0;
	scr_inited = qfalse;
}

/* ---- Scr_SetLoading  0x00475970 ---- */
void Scr_SetLoading( int bLoading ) {
	scr_loading = bLoading;
}

/* ---- Scr_IsSystemActive  0x0047D040 ---- */
qboolean Scr_IsSystemActive( void ) {
	return scr_timeArrayId != 0 ? qtrue : qfalse;
}
