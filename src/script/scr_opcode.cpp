/*
 * script/scr_opcode.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_opcode.cpp
 *
 * Retail range 0x0046D2E0-0x0046D374, 4 functions.
 *
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"
#include "scr_local.h"

#include <ctype.h>

/* ---- Scr_IsInOpcodeMemory  0x0046D2E0 ---- */
qboolean Scr_IsInOpcodeMemory( const char *pos ) {
	return ( (unsigned int)pos >= (unsigned int)scrVarPub_programBuffer
		  && (unsigned int)pos <  (unsigned int)scrVarPub_programBuffer + (unsigned int)scrCompilePub_programLen )
		? qtrue : qfalse;
}

/* ---- Scr_IsInOpcodeBuffer_m  0x0046D300 ---- MEDIUM */
qboolean Scr_IsInOpcodeBuffer_m( const char *pos ) {
	return ( (unsigned int)pos >= (unsigned int)scrCompileGlob_devOpBuffer
		  && (unsigned int)pos <  (unsigned int)developerCodeStart )
		? qtrue : qfalse;
}

/* ---- Scr_IsInDeveloperOpcodeMemory  0x0046D320 ---- */
qboolean Scr_IsInDeveloperOpcodeMemory( const char *pos ) {
	return ( (unsigned int)pos >= (unsigned int)scrVarPub_programBuffer
		  && (unsigned int)pos <  (unsigned int)scrVarPub_endScriptBuffer )
		? qtrue : qfalse;
}

/* ---- Scr_IsIdentifier  0x0046D340 ---- */
/* Retail returns C++ bool (al); callers use the same byte return under VC7. */
#ifdef COD1_VC7
unsigned char Scr_IsIdentifier( const char *name ) {
#else
qboolean Scr_IsIdentifier( const char *name ) {
#endif
	while ( *name ) {
		if ( !isalnum( *name ) && *name != '_' ) {
			return qfalse;
		}
		name++;
	}
	return qtrue;
}

qboolean Scr_IsIdentifier__FPCc( const char *name ) {
	return (qboolean)Scr_IsIdentifier( name );
}

#ifdef __cplusplus
}
#endif
