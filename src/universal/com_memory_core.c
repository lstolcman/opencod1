/*
 * universal/com_memory_core.c
 *
 * @fidelity: verified
 */

#include "../qcommon/qcommon.h"

#include <time.h>

/* The hunk_* aliases hold pointers to these counters (Com_BindHunkGlobals).
   Export the counters so memory and script routines can read the same storage
   directly, as retail does. */
#define HUNK_LOW_MARK   hunk_low_mark
#define HUNK_LOW_PERM   hunk_low_permanent
#define HUNK_LOW_TEMP   hunk_low_temp
#define HUNK_HIGH_MARK  s_hunkHighMark
#define HUNK_HIGH_PERM  s_hunkHighPermanent
#define HUNK_HIGH_TEMP  s_hunkHighTemp
#define HUNK_TOTAL      s_hunkTotal
#define ZONE_TOTAL      s_zoneTotal

extern void Com_Meminfo_f( void );
extern void Sys_OutOfMemoryPrep( void );
extern char *SEH_GetLocalizedString_m( const char *key );
__declspec( dllimport ) int __stdcall MessageBoxA( void *h, const char *text, const char *caption, unsigned type );

int s_hunkTotal;                 /* 0x0089319C */
int s_zoneTotal;                 /* 0x008931A0 */
int s_hunkHighMark;              /* 0x008931A4 */
int s_hunkHighPermanent;         /* 0x008931AC */
int s_hunkHighTemp;              /* 0x008931B0 */
int hunk_low_mark;                      /* 0x008931BC */
int hunk_low_permanent;                 /* 0x008931C4 */
int hunk_low_temp;                      /* 0x008931C8 */
void *s_hunkData;                       /* 0x0140741C */

/* The hunk_* cells: the high/low TEMP marks are plain counters, the rest hold
   pointers to the counters above (Com_BindHunkGlobals). */
int hunk_totalSize;                     /* 0x0089319C */
signed __int32 hunk_totalZoneSize;      /* 0x008931A0 */
int hunk_highMark;                      /* 0x008931A4 */
int hunk_highTempMark;                  /* 0x008931A8 */
int hunk_highUsed;                      /* 0x008931AC */
int hunk_highTemp;                      /* 0x008931B0 */
int hunk_lowMark;                       /* 0x008931BC */
int hunk_lowTempMark;                   /* 0x008931C0 */
int hunk_temp_permanent;                /* 0x008931C4 */
int hunk_temp_temp;                     /* 0x008931C8 */

void Com_BindHunkGlobals( void ) {
	if ( hunk_totalSize ) {
		return;
	}
	hunk_totalZoneSize        = (int) &s_zoneTotal;
	hunk_highMark        = (int) &s_hunkHighMark;
	hunk_highUsed        = (int) &s_hunkHighPermanent;
	hunk_highTemp        = (int) &s_hunkHighTemp;
	hunk_lowMark        = (int) &hunk_low_mark;
	hunk_temp_permanent = (int) &hunk_low_permanent;
	hunk_temp_temp      = (int) &hunk_low_temp;
}

/* ---- Com_InitHunkMemory  0x00431E00 ---- */
void Com_InitHunkMemory( void ) {
	cvar_t      *cv;
	int minMegs;
	const char  *fmt;

	if ( FS_LoadStack() ) {
		Com_Error( ERR_FATAL,
				   "\x15Hunk initialization failed. File system load stack not zero" );
	}

	cv = Cvar_Get( "com_hunkMegs", "128", CVAR_ARCHIVE | CVAR_LATCH );

	if ( com_dedicated && com_dedicated->integer ) {
		minMegs = 1;
		fmt = "Minimum com_hunkMegs for a dedicated server is %i, allocating %i megs.\n";
	} else {
		minMegs = 80;
		fmt = "Minimum com_hunkMegs is %i, allocating %i megs.\n";
	}

	if ( cv->integer < minMegs ) {
		HUNK_TOTAL = minMegs << 20;
		Com_Printf( fmt, minMegs, HUNK_TOTAL / ( 1024 * 1024 ) );
	} else {
		HUNK_TOTAL = cv->integer << 20;
	}

	s_hunkData = malloc( HUNK_TOTAL + 31 );
	if ( !s_hunkData ) {
		/* Sys_OutOfMemoryError (0x00462BA0) as retail inlines it; written out
		   because the win_main.c body does not interleave the pushes yet */
		Sys_OutOfMemoryPrep();
		MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_BODY" ),
		             SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_TITLE" ), 0x10 );
		exit( -1 );
	}
	s_hunkData = (void *) ( ( (unsigned int) s_hunkData + 31 ) & 0xFFFFFFE0 );

	CL_ShutdownCGame();
	CL_ShutdownUI();
	SV_ShutdownGameProgs();
	CIN_CloseAllVideos();
	Hunk_ClearToStart();
	VM_Clear();

	Cmd_AddCommand( "meminfo", Com_Meminfo_f );
}

/* ---- Z_MallocInternal  0x00431900 ---- */
void *Z_MallocInternal( int size ) {
	void *p;

	p = malloc( size );
	if ( !p ) {
		/* retail inlines Sys_OutOfMemoryError (0x00462BA0) here, but keeps the
		   call where Z_MallocInternal itself is inlined (Sys_ListFilteredFiles) */
		Sys_OutOfMemoryError();
	}
	Com_Memset( p, 0, size );
	return p;
}

/* ---- Hunk_AllocAlignInternal  0x00432170 ---- */
void *Hunk_AllocAlignInternal( int size, int align ) {
	void *block;

	align--;
	if ( !s_hunkData ) {
		Com_Error( ERR_FATAL, "\x15" "Hunk_AllocAlign: Hunk memory system not initialized" );
	}

	HUNK_HIGH_PERM = ( HUNK_HIGH_PERM + size + align ) & ~align;
	block = (void *) ( (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_PERM );
	HUNK_HIGH_TEMP = HUNK_HIGH_PERM;

	if ( HUNK_LOW_TEMP + HUNK_HIGH_TEMP > HUNK_TOTAL ) {
		Com_Meminfo_f();
		Com_Error( ERR_DROP, "\x15" "Hunk_AllocAlign failed on %i", size );
	}

	com_hunkMeminfoRunningTotal = HUNK_LOW_PERM + HUNK_HIGH_PERM;

	memset( block, 0, size );
	return block;
}

/* ---- Hunk_AllocLowAlignInternal  0x00432280 ---- */
void *Hunk_AllocLowAlignInternal( int size, int align ) {
	void *block;

	align--;
	HUNK_LOW_PERM = ( HUNK_LOW_PERM + align ) & ~align;
	block = (void *) ( (char *) s_hunkData + HUNK_LOW_PERM );
	HUNK_LOW_PERM += size;
	HUNK_LOW_TEMP = HUNK_LOW_PERM;

	if ( HUNK_LOW_TEMP + HUNK_HIGH_TEMP > HUNK_TOTAL ) {
		Com_Meminfo_f();
		Com_Error( ERR_DROP, "\x15" "Hunk_AllocLowAlign failed on %i", size );
	}

	com_hunkMeminfoRunningTotal = HUNK_LOW_PERM + HUNK_HIGH_PERM;

	memset( block, 0, size );
	return block;
}

/* ---- Hunk_AllocateTempMemoryHighInternal  0x00432210 ---- */
void *Hunk_AllocateTempMemoryHighInternal( int size ) {
	HUNK_HIGH_TEMP = ( HUNK_HIGH_TEMP + size + 15 ) & ~15;

	if ( HUNK_HIGH_TEMP + HUNK_LOW_TEMP > HUNK_TOTAL ) {
		Com_Meminfo_f();
		Com_Error( ERR_DROP, "\x15" "Hunk_AllocateTempMemoryHigh: failed on %i", size );
	}

	return (void *) ( (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_TEMP );
}

typedef enum
{
	PRE_READ,                                   // prefetch assuming that buffer is used for reading only
	PRE_WRITE,                                  // prefetch assuming that buffer is used for writing only
	PRE_READ_WRITE                              // prefetch assuming that buffer is used for both reading and writing
} e_prefetch;

void Com_TouchMemoryBlock_m( const void *s, const unsigned int bytes, int type );   /* retail's Com_Prefetch, com_memory.c */

/* RTCW _copyDWord; 0x004324B0, com_memory.c */
void memset32( unsigned int *dest, unsigned int constant, unsigned int count );

/* ---- Com_Memcpy  0x00432540 ---- VERIFIED */
// optimized memory copy routine that handles all alignment
// cases and block sizes efficiently
void Com_Memcpy( void* dest, const void* src, size_t count ) {
	Com_TouchMemoryBlock_m( src, count, PRE_READ );       /* inlined in retail */
	__asm
	{
		push edi
		push esi
		mov ecx,count
		cmp ecx,0                           // count = 0 check (just to be on the safe side)
		je outta
		mov edx,dest
		mov ebx,src
		cmp ecx,32                          // padding only?
		jl padding

		mov edi,ecx
		and     edi,~31                 // edi = count&~31
		sub edi,32

		align 16
loopMisAligned:
		mov eax,[ebx + edi + 0 + 0 * 8]
		mov esi,[ebx + edi + 4 + 0 * 8]
		mov     [edx + edi + 0 + 0 * 8],eax
		mov     [edx + edi + 4 + 0 * 8],esi
		mov eax,[ebx + edi + 0 + 1 * 8]
		mov esi,[ebx + edi + 4 + 1 * 8]
		mov     [edx + edi + 0 + 1 * 8],eax
		mov     [edx + edi + 4 + 1 * 8],esi
		mov eax,[ebx + edi + 0 + 2 * 8]
		mov esi,[ebx + edi + 4 + 2 * 8]
		mov     [edx + edi + 0 + 2 * 8],eax
		mov     [edx + edi + 4 + 2 * 8],esi
		mov eax,[ebx + edi + 0 + 3 * 8]
		mov esi,[ebx + edi + 4 + 3 * 8]
		mov     [edx + edi + 0 + 3 * 8],eax
		mov     [edx + edi + 4 + 3 * 8],esi
		sub edi,32
		jge loopMisAligned

		mov edi,ecx
		and     edi,~31
		add ebx,edi                     // increase src pointer
		add edx,edi                     // increase dst pointer
		and     ecx,31                  // new count
		jz outta                        // if count = 0, get outta here

padding:
		cmp ecx,16
		jl skip16
		mov eax,dword ptr [ebx]
		mov dword ptr [edx],eax
		mov eax,dword ptr [ebx + 4]
		mov dword ptr [edx + 4],eax
		mov eax,dword ptr [ebx + 8]
		mov dword ptr [edx + 8],eax
		mov eax,dword ptr [ebx + 12]
		mov dword ptr [edx + 12],eax
		sub ecx,16
		add ebx,16
		add edx,16
skip16:
		cmp ecx,8
		jl skip8
		mov eax,dword ptr [ebx]
		mov dword ptr [edx],eax
		mov eax,dword ptr [ebx + 4]
		sub ecx,8
		mov dword ptr [edx + 4],eax
		add ebx,8
		add edx,8
skip8:
		cmp ecx,4
		jl skip4
		mov eax,dword ptr [ebx]     // here 4-7 bytes
		add ebx,4
		sub ecx,4
		mov dword ptr [edx],eax
		add edx,4
skip4:                          // 0-3 remaining bytes
		cmp ecx,2
		jl skip2
		mov ax,word ptr [ebx]       // two bytes
		cmp ecx,3                   // less than 3?
		mov word ptr [edx],ax
		jl outta
		mov al,byte ptr [ebx + 2]   // last byte
		mov byte ptr [edx + 2],al
		jmp outta
skip2:
		cmp ecx,1
		jl outta
		mov al,byte ptr [ebx]
		mov byte ptr [edx],al
outta:
		pop esi
		pop edi
	}
}

/* ---- Com_Memset  0x00432670 ---- VERIFIED */
void Com_Memset( void* dest, int val, size_t count ) {
	unsigned int fillval;

	if ( count < 8 ) {
		__asm
		{
			mov edx,dest
			mov eax, val
			mov ah,al
			mov ebx,eax
			and     ebx, 0xffff
			shl eax,16
			add eax,ebx                 // eax now contains pattern
			mov ecx,count
			cmp ecx,4
			jl skip4
			mov     [edx],eax           // copy first dword
			add edx,4
			sub ecx,4
skip4:  cmp ecx,2
			jl skip2
			mov word ptr [edx],ax       // copy 2 bytes
			add edx,2
			sub ecx,2
skip2:  cmp ecx,0
			je skip1
			mov byte ptr [edx],al       // copy single byte
skip1:
		}
		return;
	}

	fillval = val;

	fillval = fillval | ( fillval << 8 );
	fillval = fillval | ( fillval << 16 );        // fill dword with 8-bit pattern

	memset32( (unsigned int*)( dest ),fillval, count / 4 );

	__asm                                   // padding of 0-3 bytes
	{
		mov ecx,count
		mov eax,ecx
		and     ecx,3
		jz skipA
		and     eax,~3
		mov ebx,dest
		add ebx,eax
		mov eax,fillval
		cmp ecx,2
		jl skipB
		mov word ptr [ebx],ax
		cmp ecx,2
		je skipA
		mov byte ptr [ebx + 2],al
		jmp skipA
skipB:
		cmp ecx,0
		je skipA
		mov byte ptr [ebx],al
skipA:
	}
}

/* ---- Hunk_AllocateTempMemoryInternal  0x00432310 ---- VERIFIED */
void *Hunk_AllocateTempMemoryInternal( int size ) {
	int *header;
	int previousLowTemp;

	if ( !s_hunkData ) {
		return Z_MallocInternal( size );
	}

	size += 16;

	previousLowTemp = HUNK_LOW_TEMP;
	HUNK_LOW_TEMP = ( HUNK_LOW_TEMP + 15 ) & ~15;
	header = (int *) ( (char *) s_hunkData + HUNK_LOW_TEMP );
	HUNK_LOW_TEMP += size;

	if ( HUNK_HIGH_TEMP + HUNK_LOW_TEMP > HUNK_TOTAL ) {
		Com_Meminfo_f();
		Com_Error( ERR_DROP, "\x15" "Hunk_AllocateTempMemory: failed on %i, needs %i",
				   size, HUNK_HIGH_TEMP + HUNK_LOW_TEMP - HUNK_TOTAL );
	}

	header[0] = 0x89537892;
	header[1] = HUNK_LOW_TEMP - previousLowTemp;

	return (char *) header + 16;
}

extern void Hunk_FreeTempMemory( void *buf );
extern void Hunk_ClearTempMemory( void );
extern int Hunk_ClearTempMemoryHigh( void );

/* ---- Hunk_ReallocateTempMemory  0x004323A0 ---- */
void *Hunk_ReallocateTempMemory( int size ) {
	HUNK_LOW_TEMP = HUNK_LOW_PERM + size;

	if ( HUNK_LOW_TEMP + HUNK_HIGH_TEMP > HUNK_TOTAL ) {
		Com_Meminfo_f();
		Com_Error( ERR_DROP, "\x15Hunk_ReallocateTempMemory: failed on %i", size );
	}

	return (void *) ( (char *) s_hunkData + HUNK_LOW_PERM );
}

/* ---- Hunk_ClearToStart  0x00432090 ---- */
void Hunk_ClearToStart( void ) {
	extern void XModelClearData( void *high, void *low );
	extern void FS_ClearDataForFiles( void *lowEnd, void *highEnd );
	extern void FS_ClearMemory( void );
	char *high;
	char *low;

	/* retail has no s_hunkData guard: the whole body runs even before
	   Com_InitHunkMemory (Com_Error fatal path, Com_Quit_f) */
	HUNK_LOW_MARK = 0;
	hunk_lowTempMark = 0;
	HUNK_LOW_PERM = 0;
	HUNK_LOW_TEMP = 0;
	HUNK_HIGH_MARK = 0;
	hunk_highTempMark = 0;
	HUNK_HIGH_PERM = 0;
	HUNK_HIGH_TEMP = 0;
	com_hunkMeminfoRunningTotal = 0;

	Com_Printf( "Hunk_Clear: reset the hunk ok\n" );

	low = (char *) s_hunkData + HUNK_LOW_PERM;
	high = (char *) s_hunkData + HUNK_TOTAL - HUNK_HIGH_PERM;
	XModelClearData( high, low );
	FS_ClearDataForFiles( low, high );

	FS_ClearMemory();
}

/* ---- Com_StringContains  0x004314C0 ---- */
const char *Com_StringContains( const char *str1, const char *str2, int casesensitive ) {
	int len, i, j;

	len = strlen( str1 ) - strlen( str2 );
	for ( i = 0; i <= len; i++, str1++ ) {
		for ( j = 0; str2[j]; j++ ) {
			if ( casesensitive ) {
				if ( str1[j] != str2[j] ) {
					break;
				}
			} else {
				if ( toupper( str1[j] ) != toupper( str2[j] ) ) {
					break;
				}
			}
		}
		if ( !str2[j] ) {
			return str1;
		}
	}
	return NULL;
}

/* ---- Com_Filter  0x00431580 ---- */
qboolean Com_Filter( const char *filter, const char *name, int casesensitive ) {
	char buf[MAX_TOKEN_CHARS];
	const char  *ptr;
	int i, found;

	while ( *filter ) {
		if ( *filter == '*' ) {
			filter++;
			for ( i = 0; *filter; i++ ) {
				if ( *filter == '*' || *filter == '?' ) {
					break;
				}
				buf[i] = *filter;
				filter++;
			}
			/* pointer store: buf[i] = '\0' makes modern /GS add a range-check fast-fail VC7 never emitted */
			*( buf + i ) = '\0';
			if ( strlen( buf ) ) {
				ptr = Com_StringContains( name, buf, casesensitive );
				if ( !ptr ) {
					return qfalse;
				}
				name = ptr + strlen( buf );
			}
		} else if ( *filter == '?' ) {
			filter++;
			name++;
		} else if ( *filter == '[' && *( filter + 1 ) == '[' ) {
			filter++;
		} else if ( *filter == '[' ) {
			filter++;
			found = qfalse;
			while ( *filter && !found ) {
				if ( *filter == ']' && *( filter + 1 ) != ']' ) {
					break;
				}
				if ( *( filter + 1 ) == '-' && *( filter + 2 )
					 && ( *( filter + 2 ) != ']' || *( filter + 3 ) == ']' ) ) {
					if ( casesensitive ) {
						if ( *name >= *filter && *name <= *( filter + 2 ) ) {
							found = qtrue;
						}
					} else {
						if ( toupper( *name ) >= toupper( *filter )
							 && toupper( *name ) <= toupper( *( filter + 2 ) ) ) {
							found = qtrue;
						}
					}
					filter += 3;
				} else {
					if ( casesensitive ) {
						if ( *filter == *name ) {
							found = qtrue;
						}
					} else {
						if ( toupper( *filter ) == toupper( *name ) ) {
							found = qtrue;
						}
					}
					filter++;
				}
			}
			if ( !found ) {
				return qfalse;
			}
			while ( *filter ) {
				if ( *filter == ']' && *( filter + 1 ) != ']' ) {
					break;
				}
				filter++;
			}
			filter++;
			name++;
		} else {
			if ( casesensitive ) {
				if ( *filter != *name ) {
					return qfalse;
				}
			} else {
				if ( toupper( *filter ) != toupper( *name ) ) {
					return qfalse;
				}
			}
			filter++;
			name++;
		}
	}
	return qtrue;
}

/* ---- Com_FilterPath  0x00431790 ---- */
qboolean Com_FilterPath( const char *filter, const char *name, int casesensitive ) {
	int i;
	char new_filter[MAX_QPATH];
	char new_name[MAX_QPATH];

	for ( i = 0; i < MAX_QPATH - 1 && filter[i]; i++ ) {
		if ( filter[i] == '\\' || filter[i] == ':' ) {
			new_filter[i] = '/';
		} else {
			new_filter[i] = filter[i];
		}
	}
	/* pointer store: new_filter[i] = '\0' makes modern /GS add a range-check fast-fail VC7 never emitted */
	*( new_filter + i ) = '\0';

	for ( i = 0; i < MAX_QPATH - 1 && name[i]; i++ ) {
		if ( name[i] == '\\' || name[i] == ':' ) {
			new_name[i] = '/';
		} else {
			new_name[i] = name[i];
		}
	}
	/* pointer store: new_name[i] = '\0' makes modern /GS add a range-check fast-fail VC7 never emitted */
	*( new_name + i ) = '\0';

	return Com_Filter( new_filter, new_name, casesensitive );
}

/* ---- Com_HashKey  0x00431850 ---- */
int Com_HashKey( const char *string, int maxlen ) {
	int hash, i;

	hash = 0;
	for ( i = 0; i < maxlen && string[i] != '\0'; i++ ) {
		hash += (signed char) string[i] * ( 119 + i );
	}
	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	return hash;
}

#ifndef QTIME_T_DEFINED
#define QTIME_T_DEFINED
typedef struct qtime_s {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
} qtime_t;
#endif

/* ---- Com_RealTime  0x00431890 ---- */
int Com_RealTime( qtime_t *qtime ) {
	time_t t;
	struct tm   *tms;

	t = time( NULL );
	if ( !qtime ) {
		return (int) t;
	}

	tms = localtime( &t );
	if ( tms ) {
		qtime->tm_sec = tms->tm_sec;
		qtime->tm_min = tms->tm_min;
		qtime->tm_hour = tms->tm_hour;
		qtime->tm_mday = tms->tm_mday;
		qtime->tm_mon = tms->tm_mon;
		qtime->tm_year = tms->tm_year;
		qtime->tm_wday = tms->tm_wday;
		qtime->tm_yday = tms->tm_yday;
		qtime->tm_isdst = tms->tm_isdst;
	}
	return (int) t;
}
