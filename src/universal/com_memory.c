/*
 * universal/com_memory.c
 *
 * Retail range 0x00431430-0x004327C0.
 *
 * @fidelity: likely
 */

#include "../qcommon/qcommon.h"

/* Canonical hunk counters owned by com_memory_core.c. */
extern int s_hunkTotal;
extern int s_zoneTotal;
extern int s_hunkHighMark;
extern int s_hunkHighPermanent;
extern int s_hunkHighTemp;


/* Globals owned by this unit.  hunkblocks heads
 * the debug hunk-block list (cast to hunkblock_t* at use); hunk_logfile is the
 * memory-log file handle. */
int hunkblocks;      /* 0x008931B4 */
int hunk_logfile;    /* 0x008931B8 */

extern void FS_ClearDataForFiles( void *lowEnd, void *highEnd );
extern int  XModelClearData();
__declspec( dllimport ) unsigned long __stdcall timeGetTime( void );

extern void *fs_searchpaths;
extern void *Hunk_AllocAlignInternal( int size, int align );
extern void *Hunk_AllocLowAlignInternal( int size, int align );

#define HUNK_TOTAL          ( s_hunkTotal )   /* 0x0089319C */
#define ZONE_TOTAL          ( s_zoneTotal )   /* 0x008931A0 */
#define HUNK_HIGH_MARK      ( s_hunkHighMark )   /* 0x008931A4 */
#define HUNK_HIGH_TEMPMARK  ( hunk_highTempMark )            /* 0x008931A8, plain */
#define HUNK_HIGH_PERM      ( s_hunkHighPermanent )   /* 0x008931AC */
#define HUNK_HIGH_TEMP      ( s_hunkHighTemp )   /* 0x008931B0 */
/* the low counters are read directly, as retail does (com_memory_core.c owns
   them; hunk_lowMark/hunk_temp_* only hold pointers to them) */
extern int hunk_low_mark;
extern int hunk_low_permanent;
extern int hunk_low_temp;
#define HUNK_LOW_MARK       ( hunk_low_mark )               /* 0x008931BC */
#define HUNK_LOW_TEMPMARK   ( hunk_lowTempMark )            /* 0x008931C0, plain */
#define HUNK_LOW_PERM       ( hunk_low_permanent )          /* 0x008931C4 */
#define HUNK_LOW_TEMP       ( hunk_low_temp )               /* 0x008931C8 */

int com_randSeed_m;   /* 0x0057B4B0 */

/* ---- Com_SRand_m  0x00431430 ---- VERIFIED */
int __cdecl Com_SRand_m( int seed ) {
	com_randSeed_m = seed;
	return seed;
}

/* ---- Com_RandFloatRange_m  0x00431440 ---- VERIFIED */
/* Improve-float-consistency (/Op) code in retail: r and the result go through
   4-byte slots and the divide by 32768 stays an fdiv. */
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif
double __cdecl Com_RandFloatRange_m( float lo, float hi ) {
	float r;

	com_randSeed_m = 214013 * com_randSeed_m + 2531011;
	r = (float) ( (unsigned int) com_randSeed_m >> 17 );
	r = ( hi - lo ) * r / 32768.0f + lo;
	return r;
}
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/* ---- Com_RandIntRange_m  0x00431490 ---- VERIFIED */
int __fastcall Com_RandIntRange_m( int maxValue, int minValue ) {
	com_randSeed_m = 214013 * com_randSeed_m + 2531011;
	return minValue + ( (int) ( ( maxValue - minValue ) * ( (unsigned int) com_randSeed_m >> 17 ) ) >> 15 );
}

/* ---- Z_FreeInternal  0x004318F0 ---- VERIFIED */
void __cdecl Z_FreeInternal( void *block ) {
	free( block );
}

/* CopyStringInternal  0x00431960  VERIFIED */
char *__cdecl CopyStringInternal( const char *in ) {
	char *out;

	out = (char *) Z_MallocInternal( strlen( in ) + 1 );
	strcpy( out, in );
	return out;
}

/* ---- Com_Meminfo_f  0x00431990 ---- VERIFIED */
void Com_Meminfo_f( void ) {
	Com_Printf( "%8i bytes total hunk\n", HUNK_TOTAL );
	Com_Printf( "%8i bytes total zone\n", ZONE_TOTAL );
	Com_Printf( "\n" );

	Com_Printf( "%8i low mark\n", HUNK_LOW_MARK );
	Com_Printf( "%8i low permanent\n", HUNK_LOW_PERM );
	if ( HUNK_LOW_TEMP != HUNK_LOW_PERM ) {
		Com_Printf( "%8i low temp\n", HUNK_LOW_TEMP );
	}
	Com_Printf( "\n" );

	Com_Printf( "%8i high mark\n", HUNK_HIGH_MARK );
	Com_Printf( "%8i high permanent\n", HUNK_HIGH_PERM );
	if ( HUNK_HIGH_TEMP != HUNK_HIGH_PERM ) {
		Com_Printf( "%8i high temp\n", HUNK_HIGH_TEMP );
	}
	Com_Printf( "\n" );

	Com_Printf( "%8i total hunk in use\n", HUNK_LOW_PERM + HUNK_HIGH_PERM );
	Com_Printf( "\n" );
}

/* ---- Com_TouchMemory  0x00431A80 ---- VERIFIED */
/* both Sys_Milliseconds calls are inlined in retail (0x00431B02) */
void Com_TouchMemory( void ) {
	int         *hunkWords;
	int         sum;
	int         start, stop;
	int         startTime;

	startTime = Sys_Milliseconds();
	sum = 0;
	hunkWords = (int *) s_hunkData;

	stop = HUNK_LOW_PERM >> 2;
	for ( start = 0 ; start < stop ; start += 64 ) {
		sum += hunkWords[start];
	}

	start = ( HUNK_TOTAL - HUNK_HIGH_PERM ) >> 2;
	stop = HUNK_HIGH_PERM >> 2;
	for ( ; start < stop ; start += 64 ) {
		sum += hunkWords[start];
	}

	Com_Printf( "Com_TouchMemory: %i msec. Using sum: %d\n",
				Sys_Milliseconds() - startTime, sum );
}

/* ---- Com_InitZoneMemory  0x00431B30 ---- VERIFIED */
void Com_InitZoneMemory( void ) {
}

typedef struct hunkblock_s {
	int                 size;
	byte                printed;
	struct hunkblock_s  *next;
	const char          *label;
	const char          *sourceFile;
	int                 sourceLine;
} hunkblock_t;

/* ---- Hunk_Log  0x00431B40 ---- */
void Hunk_Log( void ) {
	hunkblock_t *block;
	char        buf[4096];
	int         size;
	int         numBlocks;

	if ( !hunk_logfile || !fs_searchpaths ) {
		return;
	}

	size = 0;
	numBlocks = 0;

	Com_sprintf( buf, sizeof( buf ),
				 "\r\n================\r\nHunk log\r\n================\r\n" );
	FS_Write( buf, strlen( buf ), hunk_logfile );

	for ( block = (hunkblock_t *) hunkblocks ; block ; block = block->next ) {
		size += block->size;
		numBlocks++;
	}

	Com_sprintf( buf, sizeof( buf ), "%d Hunk memory\r\n", size );
	FS_Write( buf, strlen( buf ), hunk_logfile );

	Com_sprintf( buf, sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
	FS_Write( buf, strlen( buf ), hunk_logfile );
}

void Hunk_SmallLog( void ) {
	hunkblock_t *block;
	hunkblock_t *other;
	char        buf[4096];
	int         size;
	int         numBlocks;

	if ( !hunk_logfile || !fs_searchpaths ) {
		return;
	}

	for ( block = (hunkblock_t *) hunkblocks ; block ; block = block->next ) {
		block->printed = 0;
	}

	size = 0;
	numBlocks = 0;

	Com_sprintf( buf, sizeof( buf ),
				 "\r\n================\r\nHunk Small log\r\n================\r\n" );
	FS_Write( buf, strlen( buf ), hunk_logfile );

	for ( block = (hunkblock_t *) hunkblocks ; block ; block = block->next ) {
		if ( block->printed ) {
			continue;
		}
		for ( other = block->next ; other ; other = other->next ) {
			if ( block->sourceLine != other->sourceLine ) {
				continue;
			}
			if ( Q_stricmp( block->sourceFile, other->sourceFile ) ) {
				continue;
			}
			size += other->size;
			other->printed = qtrue;
		}
		size += block->size;
		numBlocks++;
	}

	Com_sprintf( buf, sizeof( buf ), "%d Hunk memory\r\n", size );
	FS_Write( buf, strlen( buf ), hunk_logfile );

	Com_sprintf( buf, sizeof( buf ), "%d hunk blocks\r\n", numBlocks );
	FS_Write( buf, strlen( buf ), hunk_logfile );
}

/* ---- Hunk_MemoryRemaining  0x00431F10 ---- VERIFIED */
int Hunk_MemoryRemaining( void ) {
	return HUNK_TOTAL - HUNK_LOW_TEMP - HUNK_HIGH_TEMP;
}

/* ---- Hunk_RetouchMemory  0x00431F30 ---- VERIFIED */
void Hunk_RetouchMemory( void ) {
	char *low, *high;

	low  = (char *) s_hunkData + HUNK_LOW_PERM;
	high = (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_PERM;
	XModelClearData( high, low );
	FS_ClearDataForFiles( low, high );
}

/* ---- Hunk_SetHighMark  0x00431F70 ---- VERIFIED */
int Hunk_SetHighMark( void ) {
	HUNK_HIGH_MARK = HUNK_HIGH_PERM;
	return HUNK_HIGH_PERM;
}

/* ---- Hunk_SetHighTempMark  0x00431F80 ---- VERIFIED */
int Hunk_SetHighTempMark( void ) {
	HUNK_HIGH_TEMPMARK = HUNK_HIGH_PERM;
	return HUNK_HIGH_PERM;
}

/* ---- Hunk_CheckHighMark  0x00431F90 ---- VERIFIED */
qboolean __cdecl Hunk_CheckHighMark( const void *ptr ) {
	return (qboolean) ( (const char *) ptr
						>= (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_MARK );
}

/* ---- Hunk_ClearToHighMark  0x00431FB0 ---- VERIFIED */
void Hunk_ClearToHighMark( void ) {
	char *low, *high;

	HUNK_HIGH_TEMP = HUNK_HIGH_MARK;
	HUNK_HIGH_PERM = HUNK_HIGH_MARK;
	low  = (char *) s_hunkData + HUNK_LOW_PERM;
	high = (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_PERM;
	XModelClearData( high, low );
	FS_ClearDataForFiles( low, high );
}

/* ---- Hunk_ClearToHighTempMark  0x00431FF0 ---- VERIFIED */
void Hunk_ClearToHighTempMark( void ) {
	char *low, *high;

	HUNK_HIGH_TEMP = HUNK_HIGH_TEMPMARK;
	HUNK_HIGH_PERM = HUNK_HIGH_TEMPMARK;
	low  = (char *) s_hunkData + HUNK_LOW_PERM;
	high = (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_PERM;
	XModelClearData( high, low );
	FS_ClearDataForFiles( low, high );
}

/* ---- Hunk_HighMarkIsSet  0x00432030 ---- VERIFIED */
qboolean Hunk_HighMarkIsSet( void ) {
	return (qboolean) ( HUNK_HIGH_MARK != 0 );
}

/* ---- Hunk_SetLowMark  0x00432040 ---- VERIFIED */
int Hunk_SetLowMark( void ) {
	HUNK_LOW_MARK = HUNK_LOW_PERM;
	return HUNK_LOW_PERM;
}

/* ---- Hunk_ClearToLowMark  0x00432050 ---- VERIFIED */
void Hunk_ClearToLowMark( void ) {
	char *low, *high;

	HUNK_LOW_TEMP = HUNK_LOW_MARK;
	HUNK_LOW_PERM = HUNK_LOW_MARK;
	low  = (char *) s_hunkData + HUNK_LOW_PERM;
	high = (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_PERM;
	XModelClearData( high, low );
	FS_ClearDataForFiles( low, high );
}

/* ---- Hunk_Clear  0x00432140 ---- VERIFIED */
void Hunk_Clear( void ) {
	CL_ShutdownCGame();
	CL_ShutdownUI();
	SV_ShutdownGameProgs();
	CIN_CloseAllVideos();
	Hunk_ClearToStart();
	VM_Clear();
}

/* ---- Hunk_AllocInternal  0x00432160 ---- VERIFIED */
void *__cdecl Hunk_AllocInternal( int size ) {
	return Hunk_AllocAlignInternal( size, 32 );
}

/* ---- Hunk_ClearTempMemoryHigh  0x00432260 ---- VERIFIED */
int Hunk_ClearTempMemoryHigh( void ) {
	HUNK_HIGH_TEMP = HUNK_HIGH_PERM;
	return HUNK_HIGH_PERM;
}

/* ---- Hunk_AllocLowInternal  0x00432270 ---- VERIFIED */
void *__cdecl Hunk_AllocLowInternal( int size ) {
	return Hunk_AllocLowAlignInternal( size, 32 );
}

/* ---- Hunk_CommitTempMemory  0x00432300 ---- VERIFIED */
int Hunk_CommitTempMemory( void ) {
	HUNK_LOW_PERM = HUNK_LOW_TEMP;
	return HUNK_LOW_TEMP;
}

/* ---- Hunk_ClearToLowTempMark  0x004323F0 ---- VERIFIED */
int Hunk_ClearToLowTempMark( void ) {
	HUNK_LOW_TEMP = HUNK_LOW_TEMPMARK;
	return HUNK_LOW_TEMPMARK;
}

/* ---- Hunk_SetLowTempMark  0x00432400 ---- VERIFIED */
int Hunk_SetLowTempMark( void ) {
	HUNK_LOW_TEMPMARK = HUNK_LOW_TEMP;
	return HUNK_LOW_TEMP;
}

/* ---- Hunk_FreeTempMemory  0x00432410 ---- VERIFIED */
void Hunk_FreeTempMemory( void *buf ) {
	int *hdr;

	if ( !s_hunkData ) {
		free( buf );
		return;
	}

	hdr = (int *) buf - 4;
	if ( hdr[0] != (int) 0x89537892 ) {
		Com_Error( ERR_FATAL, "\x15Hunk_FreeTempMemory: bad magic" );
	}
	hdr[0] = (int) 0x89537893;
	HUNK_LOW_TEMP -= hdr[1];
}

/* ---- Hunk_ClearTempMemory  0x00432450 ---- VERIFIED */
void Hunk_ClearTempMemory( void ) {
	if ( s_hunkData ) {
		HUNK_LOW_TEMP = HUNK_LOW_PERM;
	}
}

/* ---- Hunk_ConvertTempToPermLowInternal  0x00432470 ---- VERIFIED */
int Hunk_ConvertTempToPermLowInternal( void ) {
	int old;

	old = HUNK_LOW_PERM;
	HUNK_LOW_PERM = HUNK_LOW_TEMP;
	return old;
}

/* ---- Hunk_SetLowUsedInternal  0x00432490 ---- VERIFIED */
int __cdecl Hunk_SetLowUsedInternal( int lowUsed ) {
	HUNK_LOW_PERM = lowUsed;
	return lowUsed;
}

/* ---- Hunk_Trash  0x004324A0 ---- VERIFIED */
void Hunk_Trash( void ) {
}

/* ---- memset32  0x004324B0 ---- VERIFIED */
/* Q3's _copyDWord (qcommon/common.c), renamed.  Retail keeps the inline-asm
   body, so callers pass all three arguments on the stack. */
#if defined( _MSC_VER ) && defined( _M_IX86 )
void memset32( unsigned int *dest, unsigned int constant, unsigned int count ) {
	__asm
	{
			mov		edx,dest
			mov		eax,constant
			mov		ecx,count
			and		ecx,~7
			jz		padding
			sub		ecx,8
			jmp		loopu
			align	16
loopu:
			test	[edx+ecx*4 + 28],ebx		// fetch next block destination to L1 cache
			mov		[edx+ecx*4 + 0],eax
			mov		[edx+ecx*4 + 4],eax
			mov		[edx+ecx*4 + 8],eax
			mov		[edx+ecx*4 + 12],eax
			mov		[edx+ecx*4 + 16],eax
			mov		[edx+ecx*4 + 20],eax
			mov		[edx+ecx*4 + 24],eax
			mov		[edx+ecx*4 + 28],eax
			sub		ecx,8
			jge		loopu
padding:	mov		ecx,count
			mov		ebx,ecx
			and		ecx,7
			jz		outtta
			and		ebx,~7
			lea		edx,[edx+ebx*4]				// advance dest pointer
			test	[edx+0],eax					// fetch destination to L1 cache
			cmp		ecx,4
			jl		skip4
			mov		[edx+0],eax
			mov		[edx+4],eax
			mov		[edx+8],eax
			mov		[edx+12],eax
			add		edx,16
			sub		ecx,4
skip4:		cmp		ecx,2
			jl		skip2
			mov		[edx+0],eax
			mov		[edx+4],eax
			add		edx,8
			sub		ecx,2
skip2:		cmp		ecx,1
			jl		outtta
			mov		[edx+0],eax
outtta:
	}
}
#else
void memset32( unsigned int *dest, unsigned int constant, unsigned int count ) {
	unsigned int i;

	for ( i = 0 ; i < count ; i++ ) {
		dest[i] = constant;
	}
}
#endif

/* ---- Com_Memcmp_m  0x00432720 ---- VERIFIED */
/*
 * Retail equality test (returns 1 if equal, 0 otherwise), not a full memcmp.
 * Retail bug kept: the 16-byte block loop advances its counter by 4 per block
 * (add edx,4) but bounds it by the block count n>>4 (cmp edx,esi), so only the
 * first ceil((n>>4)/4) blocks are compared.  The byte tail then starts at
 * n&~15, so the bytes between the last compared block and n&~15 are never
 * checked (n=32 compares only bytes 0..15).  No retail callers.
 * Written as Wolf:ET's Com_Memcmp (dw[i + k] indexing, tmp, d/s tail): that
 * text gives retail's strength-reduced pointers and tail registers.
 */
int __cdecl Com_Memcmp_m( const void *src0, const void *src1, const unsigned int count ) {
	unsigned int i;

	if ( count >= 16 ) {
		unsigned int *dw = (unsigned int *) src0;
		unsigned int *sw = (unsigned int *) src1;

		unsigned int nm2 = count / 16;
		for ( i = 0 ; i < nm2 ; i += 4 ) {	/* retail bug: +4 per 16-byte block, bound is count/16 */
			unsigned int tmp = ( dw[i + 0] - sw[i + 0] ) | ( dw[i + 1] - sw[i + 1] ) |
							   ( dw[i + 2] - sw[i + 2] ) | ( dw[i + 3] - sw[i + 3] );
			if ( tmp ) {
				return qfalse;
			}
		}
	}
	if ( count & 15 ) {
		byte *d = (byte *) src0;
		byte *s = (byte *) src1;
		for ( i = count & 0xfffffff0 ; i < count ; i++ ) {	/* tail starts at count&~15 */
			if ( d[i] != s[i] ) {
				return qfalse;
			}
		}
	}

	return qtrue;
}

/* ---- Com_TouchMemoryBlock_m  0x004327C0 ---- VERIFIED */
/* Retail's Com_Prefetch (the tree's copy lives in com_memory_core.c); type is
   e_prefetch (0 PRE_READ, 1 PRE_WRITE, 2 PRE_READ_WRITE) and arrives in eax (LTCG). */
void Com_TouchMemoryBlock_m( const void *s, const unsigned int bytes, int type ) {
	switch ( type )
	{
	case 1: break;
	case 0:
	case 2:

		__asm
		{
			mov ebx,s
			mov ecx,bytes
			cmp ecx,4096                    // clamp to 4kB
			jle skipClamp
			mov ecx,4096
skipClamp:
			add ecx,0x1f
			shr ecx,5                       // number of cache lines
			jz skip
			jmp loopie

			align 16
loopie: test byte ptr [ebx],al
			add ebx,32
			dec ecx
			jnz loopie
skip:
		}

		break;
	}
}
