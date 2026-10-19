// cg_animtree.c -- loads the animation trees and manages the per-entity DObj cache

#include <math.h>
#include <stddef.h>

#include "cg_local.h"

void        BG_FindAnims( void );
void        BG_FindAnimTrees( void );
void        BG_FinalizePlayerAnims( void );

extern void ( *Scr_BeginLoadAnimTrees )( void );
extern void ( *Scr_EndLoadAnimTrees )( void );
extern void ( *Scr_PrecacheAnimTrees )( void *( *allocFn )( int size ) );

/*
==============================================================================

SCRIPT CALLBACKS

The client loads no scripts, so the callbacks it hands back are all empty

==============================================================================
*/

void *Scr_GetFunction( const char **pName, int *pDeveloper ) {
	return NULL;
}

void *Scr_GetMethod( const char **pName, int *pDeveloper ) {
	return NULL;
}

void Scr_SetObjectField( int classnum, int objectNum, int fieldnum ) {
}

void Scr_GetObjectField( int classnum, int objectNum, int fieldnum ) {
}

int Scr_LoadRead( int len ) {
	return 0;
}

/*
===============
Hunk_AllocXAnimCreate

Allocator handed to Scr_PrecacheAnimTrees
===============
*/
void *Hunk_AllocXAnimCreate( int size ) {
	return trap_Hunk_AllocInternal( size );
}

// bg_animation.c reaches cgs.corpseinfo by offset; keep it in step
typedef char cgsCorpseinfoOffsetCheck[( offsetof( cgs_t, corpseinfo ) == 0x0A43C && MAX_CORPSES == 8 ) ? 1 : -1];

void        BG_LoadAnimTreeInstances( void );

/*
===============
CGScr_LoadAnimTrees

The client loads no scripts, only the animation trees
===============
*/
void CGScr_LoadAnimTrees( void ) {
	// must start at zero: BG_AnimationIndexForString reads the count first
	int numAnims = 0;
	// 512 animScriptAnim_t; keep: int, not byte, matches original codegen
	int animScriptBuffer[9216];

	Scr_BeginLoadAnimTrees();

	BG_FindAnims();
	BG_AnimParseAnimScript( &bgs_animScriptData, animScriptBuffer, &numAnims );
	Scr_PrecacheAnimTrees( Hunk_AllocXAnimCreate );
	BG_FindAnimTrees();
	Scr_EndLoadAnimTrees();
	BG_FinalizePlayerAnims();

	BG_LoadAnimTreeInstances();
}

/*
===============
CG_FreeClientDObjInfo

Free the DObjs of the client slots
===============
*/
void CG_FreeClientDObjInfo( void ) {
	int i;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		trap_SafeDObjFree( i, 1 );
		cg.iEntityLastType[i] = 0;
		cg.pEntityLastXModel[i] = NULL;
	}
}

/*
===============
CG_SetDObjInfo
===============
*/
void CG_SetDObjInfo( int entityNum, int eType, void *xmodel ) {
	cg.iEntityLastType[entityNum] = eType;
	cg.pEntityLastXModel[entityNum] = xmodel;
}

/*
===============
CG_CheckDObjInfoMatches
===============
*/
qboolean CG_CheckDObjInfoMatches( int entityNum, int eType, void *xmodel ) {
	return cg.iEntityLastType[entityNum] == eType && cg.pEntityLastXModel[entityNum] == xmodel;
}

/*
===============
CG_SafeDObjFree
===============
*/
void CG_SafeDObjFree( int entityNum ) {
	trap_SafeDObjFree( entityNum, 1 );
	cg.iEntityLastType[entityNum] = 0;
	cg.pEntityLastXModel[entityNum] = NULL;
}

/*
===============
CG_FreeEntityDObjInfo

Free the DObjs of the non-client entity slots
===============
*/
void CG_FreeEntityDObjInfo( void ) {
	int i;

	for ( i = MAX_CLIENTS; i < ENTITYNUM_NONE; i++ ) {
		trap_SafeDObjFree( i, 1 );
		cg.iEntityLastType[i] = 0;
		cg.pEntityLastXModel[i] = NULL;
	}
}

/*
===============
CG_SinCosOut

Unused
===============
*/
static void CG_SinCosOut( float rad, float *sinOut, float *cosOut ) {
	*cosOut = (float)cos( rad );
	*sinOut = (float)sin( rad );
}
