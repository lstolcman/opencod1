/*
 * script/scr_tempmemory.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_tempmemory.cpp
 *
 * Retail range 0x00470470-0x004704CB, 3 functions.
 *
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "scr_local.h"

int currentPos;   /* 0x01407368 */
#define scrTempMemory_currentPos currentPos

extern void *Hunk_ReallocateTempMemory( int size );

/* ---- TempMemoryReset  0x00470470 ---- VERIFIED */
void TempMemoryReset( void ) {
	scrTempMemory_currentPos = 0;
}

/* ---- TempMalloc  0x00470480 ---- VERIFIED
 * Retail keeps the new cursor in a register and only stores it to
 * currentPos after Hunk_ReallocateTempMemory returns, so an ERR_DROP
 * raised inside the realloc leaves currentPos unchanged. The realloc
 * argument is spelled out apart from newPos: inlined TempMalloc(0) callers
 * then keep retail's `mov edi,esi` copy and currentPos reload. */
void *TempMalloc( int len ) {
	int   newPos;
	char *result;

	newPos = scrTempMemory_currentPos + len;
	result = (char *)Hunk_ReallocateTempMemory( scrTempMemory_currentPos + len ) + scrTempMemory_currentPos;
	scrTempMemory_currentPos = newPos;
	return result;
}

/* ---- TempMemorySetPos  0x004704A0 ---- VERIFIED */
void TempMemorySetPos( void *pos ) {
	scrTempMemory_currentPos -= (char *)TempMalloc( 0 ) - (char *)pos;
	Hunk_ReallocateTempMemory( scrTempMemory_currentPos );
}

#ifdef __cplusplus
}
#endif
