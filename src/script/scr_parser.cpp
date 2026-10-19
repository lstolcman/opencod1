/*
 * script/scr_parser.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_parser.cpp
 *
 * Retail range 0x0046E590-0x0046F0F0, 15 functions.
 *
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"
#include "scr_local.h"

#include <malloc.h>

#define scrCompilePub_scriptsAvailable  scrVarPub_developer    /* 0x008E60D0 */
#define scrCompileGlob_codegenMode      developer_script_int /* 0x008E5EE8 */
#define scrCompileGlob_lastOpcodePos    scrCompileGlob_lastOpcodePos    /* 0x00966430 */
#define scrCompilePub_developerBufPos   developerCodeStart    /* 0x008E5EE4 */
#define scrCompileGlob_codeRelocStart   scrCompileGlob_codeRelocStart    /* 0x008E5F00 */
#define scrVarPub_programBuffer         scrVarPub_programBuffer   /* 0x01407380 */
#define scrCompilePub_programLen        scrCompilePub_programLen   /* 0x01407388 */
#define scrParserPub_sourceBuf          scrCompilePub_sourceBuf   /* 0x01407370 */
#define scrParserPub_scriptfilename     scrCompilePub_scriptfilename   /* 0x01407374 */

#define SCR_CODEGEN_INTERN          0
#define SCR_CODEGEN_RELOCATE        1
#define SCR_CODEGEN_DISCARD         2

#define scrVmPub_terminal_error     scrVmPub_terminal_error     /* 0x00A7A5CC */
#define scrVmGlob_inparamcount      scrVmGlob_funcCount    /* 0x00A7A5D8 */
#define scrVmGlob_callStack         scrVmGlob_callStack    /* 0x00A7A510 */

extern void *Hunk_AllocateTempMemoryHighInternal( int size );
extern char *SEH_GetLocalizedString_m( const char *name );
extern void  Sys_OutOfMemoryPrep( void );
extern qboolean Scr_IsInOpcodeMemory( const char *pos );

__declspec( dllimport ) int __stdcall MessageBoxA( void *wnd, const char *text,
												   const char *caption,
												   unsigned int type );
#define SCR_MB_ICONHAND     0x10u

#define SCR_SOURCEPOS_TABLE_COUNT       2
#define SCR_SOURCEPOS_INITIAL_CAPACITY  0x10000
#define SCR_SOURCEFILE_INITIAL_CAPACITY 0x10
#define SCR_SAVEDSOURCE_NONE            (-1)

#define ERR_SCRIPT  6   /* retail Com_Error demotes 6/7 to ERR_DROP (0x00435B46) */

typedef struct scrSourcePosRecord_s {
	const char     *codePos;            /* +0x00 */
	unsigned int    sourcePosIndex;     /* +0x04  first slot in the pool */
	unsigned int    sourcePosCount;     /* +0x08 opencod2 OpcodeLookup name; never written in 1.1
	                                               (scrParser_lastCodePosCount keeps the running count) */
	int             profileTime;        /* +0x0C opencod2 OpcodeLookup name; no access in 1.1 */
} scrSourcePosRecord_t;

typedef struct scrSourceFile_s {
	const char     *codeStart[2];       /* +0x00  [0] low hunk mark when it compiled, [1] developer buffer mark */
	char           *name;               /* +0x08  Z_Malloc'd, name then source */
	char           *source;             /* +0x0C  points inside ->name */
	int             sourceLen;          /* +0x10  -1 means "file was missing" */
} scrSourceFile_t;

typedef struct scrSavedSource_s {
	char           *source;             /* +0x00 */
	int             sourceLen;          /* +0x04 */
} scrSavedSource_t;

/* retail 0x00966440 / 0x00966444 -- [0] normal code, [1] relocated code;
 * counts at 0x00966448 / 0x0096644C directly follow (ShutdownOpcodeLookup's
 * loop end pointer is &table[2] == &count[0]) */
static struct {
	scrSourcePosRecord_t *table[SCR_SOURCEPOS_TABLE_COUNT];
	unsigned int          count[SCR_SOURCEPOS_TABLE_COUNT];
} scrParser_pos;
#define scrParser_posTable  scrParser_pos.table
#define scrParser_posCount  scrParser_pos.count
/* retail 0x00966434 / 0x00966438 */
static unsigned int          scrParser_posCapacity[SCR_SOURCEPOS_TABLE_COUNT];

/* retail 0x0140736C -- the flat pool every record indexes into */
static unsigned int         *scrParser_sourcePosPool;
/* retail 0x00966424 / 0x00966420 */
static unsigned int          scrParser_poolCapacity;
static unsigned int          scrParser_poolCount;

/* retail 0x0096643C / 0x00966428 -- run of positions sharing one opcode */
static const char           *scrParser_lastCodePos;
static unsigned int          scrParser_lastCodePosCount;

/* retail 0x01407378 / 0x0096641C / 0x0096642C */
static scrSourceFile_t      *scrParser_sourceFiles;
static unsigned int          scrParser_sourceFileCount;
static unsigned int          scrParser_sourceFileCapacity;

/* retail 0x0140737C / 0x00575814 -- only non-NULL while loading a savegame */
static scrSavedSource_t     *scrParser_savedSource;
static int                   scrParser_savedSourceCount = SCR_SAVEDSOURCE_NONE;

typedef void ( *scrSourceIO_t )( void *buffer, int len );

/* Scr_OutOfMemory  0x00575814 */
static void Scr_OutOfMemory( void ) {
	Sys_OutOfMemoryPrep();
	/* args evaluate right to left: TITLE is localized before BODY */
	MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_BODY" ),
				 SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_TITLE" ), SCR_MB_ICONHAND );
	exit( -1 );
}

static void *Scr_Malloc( unsigned int size ) {
	void   *p;

	p = malloc( size );
	if ( !p ) {
		Scr_OutOfMemory();
	}
	Com_Memset( p, 0, size );
	return p;
}

/* ---- InitOpcodeLookup  0x0046E590 ---- */
void InitOpcodeLookup( void ) {
	int     i;

	if ( !scrCompilePub_scriptsAvailable ) {
		return;
	}

	for ( i = 0; i < SCR_SOURCEPOS_TABLE_COUNT; i++ ) {
		scrParser_posCapacity[i] = SCR_SOURCEPOS_INITIAL_CAPACITY;
		scrParser_posCount[i]    = 0;
		scrParser_posTable[i]    = (scrSourcePosRecord_t *)
			Scr_Malloc( scrParser_posCapacity[i] * sizeof( scrSourcePosRecord_t ) );
		memset( scrParser_posTable[i], 0, scrParser_posCapacity[i] * sizeof( scrSourcePosRecord_t ) );
	}

	scrParser_poolCapacity = SCR_SOURCEPOS_INITIAL_CAPACITY;
	scrParser_poolCount    = 0;
	scrParser_sourcePosPool = (unsigned int *)
		Scr_Malloc( scrParser_poolCapacity * sizeof( unsigned int ) );

	scrParser_lastCodePos      = NULL;
	scrParser_lastCodePosCount = 0;

	scrParser_sourceFileCapacity = SCR_SOURCEFILE_INITIAL_CAPACITY;
	scrParser_sourceFileCount    = 0;
	scrParser_sourceFiles = (scrSourceFile_t *)
		Scr_Malloc( scrParser_sourceFileCapacity * sizeof( scrSourceFile_t ) );
}

/* ---- ShutdownOpcodeLookup  0x0046E720 ---- */
void ShutdownOpcodeLookup( void ) {
	int     i;

	for ( i = 0; i < SCR_SOURCEPOS_TABLE_COUNT; i++ ) {
		if ( scrParser_posTable[i] ) {
			free( scrParser_posTable[i] );
			scrParser_posTable[i] = NULL;
		}
	}

	if ( scrParser_sourcePosPool ) {
		free( scrParser_sourcePosPool );
		scrParser_sourcePosPool = NULL;
	}

	if ( scrParser_sourceFiles ) {
		for ( i = 0; (unsigned int)i < scrParser_sourceFileCount; i++ ) {
			free( scrParser_sourceFiles[i].name );
		}
		free( scrParser_sourceFiles );
		scrParser_sourceFiles = NULL;
	}

	if ( scrParser_savedSource ) {
		for ( i = 0; i < scrParser_savedSourceCount; i++ ) {
			if ( scrParser_savedSource[i].source ) {
				free( scrParser_savedSource[i].source );
			}
		}
		free( scrParser_savedSource );
		scrParser_savedSourceCount = SCR_SAVEDSOURCE_NONE;
		scrParser_savedSource      = NULL;
	}
}

/* ---- AddOpcodePos  0x0046E810 ---- */
void AddOpcodePos( unsigned int sourcePos ) {
	unsigned int            tableIndex;
	scrSourcePosRecord_t   *newTable;
	unsigned int           *newPool;

	if ( !scrCompilePub_scriptsAvailable ) {
		return;
	}
	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_DISCARD ) {
		return;
	}

	tableIndex = ( scrCompileGlob_codegenMode == SCR_CODEGEN_RELOCATE );

	/* count == capacity on growth, so copying count entries is the old capacity */
	if ( scrParser_posCount[tableIndex] >= scrParser_posCapacity[tableIndex] ) {
		scrParser_posCapacity[tableIndex] *= 2;
		newTable = (scrSourcePosRecord_t *)Z_MallocInternal(
			(int)( scrParser_posCapacity[tableIndex] * sizeof( scrSourcePosRecord_t ) ) );
		memset( newTable, 0, scrParser_posCapacity[tableIndex] * sizeof( scrSourcePosRecord_t ) );
		Com_Memcpy( newTable, scrParser_posTable[tableIndex],
					scrParser_posCount[tableIndex] * sizeof( scrSourcePosRecord_t ) );
		free( scrParser_posTable[tableIndex] );
		scrParser_posTable[tableIndex] = newTable;
	}

	if ( scrParser_poolCount >= scrParser_poolCapacity ) {
		scrParser_poolCapacity *= 2;
		newPool = (unsigned int *)Z_MallocInternal(
			(int)( scrParser_poolCapacity * sizeof( unsigned int ) ) );
		Com_Memcpy( newPool, scrParser_sourcePosPool,
					scrParser_poolCount * sizeof( unsigned int ) );
		free( scrParser_sourcePosPool );
		scrParser_sourcePosPool = newPool;
	}

	if ( (const char *)scrCompileGlob_lastOpcodePos == scrParser_lastCodePos ) {
		scrParser_posCount[tableIndex]--;
	} else {
		scrParser_lastCodePosCount = 0;
		scrParser_lastCodePos = (const char *)scrCompileGlob_lastOpcodePos;
		scrParser_posTable[tableIndex][ scrParser_posCount[tableIndex] ].sourcePosIndex =
			scrParser_poolCount;
	}

	if ( scrCompileGlob_codegenMode != SCR_CODEGEN_RELOCATE ) {
		scrParser_posTable[0][ scrParser_posCount[0] ].codePos = scrParser_lastCodePos;
	} else {
		scrParser_posTable[1][ scrParser_posCount[1] ].codePos = scrParser_lastCodePos
			+ ( (int)scrCompilePub_developerBufPos - scrCompileGlob_codeRelocStart );
	}

	scrParser_sourcePosPool[ scrParser_posTable[tableIndex][ scrParser_posCount[tableIndex] ].sourcePosIndex
							 + scrParser_lastCodePosCount ] = sourcePos;

	scrParser_posCount[tableIndex]++;
	scrParser_lastCodePosCount++;
	scrParser_poolCount++;
}

/* ---- GetPrevSourcePos  0x0046E9B0 ---- */
static unsigned int GetPrevSourcePos( const char *codePos, int offset ) {
	unsigned int    tableIndex;
	int             low;
	int             high;
	int             middle;

	tableIndex = !Scr_IsInOpcodeMemory( codePos );

	low  = 0;
	high = (int)scrParser_posCount[tableIndex] - 1;

	while ( low <= high ) {
		middle = ( low + high ) / 2;
		if ( codePos > scrParser_posTable[tableIndex][middle].codePos ) {
			low = middle + 1;
			if ( low == (int)scrParser_posCount[tableIndex]
			  || codePos <= scrParser_posTable[tableIndex][low].codePos ) {
				return scrParser_sourcePosPool[
					scrParser_posTable[tableIndex][middle].sourcePosIndex + offset ];
			}
		} else {
			high = middle - 1;
		}
	}

	return 0;
}

/* ---- Scr_HasSourceFiles  0x0046EA40 ---- */
int Scr_HasSourceFiles( void ) {
	return scrCompilePub_scriptsAvailable;
}

/* ---- Scr_SaveSource  0x0046EA50 ---- */
void Scr_SaveSource( scrSourceIO_t write ) {
	unsigned int    i;

	write( &scrParser_sourceFileCount, 4 );

	for ( i = 0; i < scrParser_sourceFileCount; i++ ) {
		write( &scrParser_sourceFiles[i].sourceLen, 4 );
		if ( scrParser_sourceFiles[i].sourceLen > 0 ) {
			write( scrParser_sourceFiles[i].source, scrParser_sourceFiles[i].sourceLen );
		}
	}
}

/* ---- Scr_LoadSource  0x0046EAC0 ---- */
void Scr_LoadSource( scrSourceIO_t read ) {
	int     i;

	read( &scrParser_savedSourceCount, 4 );
	scrParser_savedSource = (scrSavedSource_t *)Z_MallocInternal(
		scrParser_savedSourceCount * (int)sizeof( scrSavedSource_t ) );

	for ( i = scrParser_savedSourceCount - 1; i >= 0; i-- ) {
		read( &scrParser_savedSource[i].sourceLen, 4 );

		if ( scrParser_savedSource[i].sourceLen > 0 ) {
			scrParser_savedSource[i].source =
				(char *)Scr_Malloc( (unsigned int)scrParser_savedSource[i].sourceLen );
			read( scrParser_savedSource[i].source, scrParser_savedSource[i].sourceLen );
		} else {
			scrParser_savedSource[i].source = NULL;
		}
	}
}

/* ---- Scr_SkipSource  0x0046EB90 ---- */
void Scr_SkipSource( scrSourceIO_t read ) {
	int     count;
	int     i;
	int     sourceLen;

	read( &count, 4 );
	for ( i = count - 1; i >= 0; i-- ) {
		read( &sourceLen, 4 );
		if ( sourceLen > 0 ) {
			read( NULL, sourceLen );
		}
	}
}

/* ---- Scr_GetNewSourceBuffer  0x0046EBE0 ---- */
static scrSourceFile_t *Scr_GetNewSourceBuffer( void ) {
	scrSourceFile_t    *newFiles;

	if ( scrParser_sourceFileCount >= scrParser_sourceFileCapacity ) {
		scrParser_sourceFileCapacity *= 2;
		newFiles = (scrSourceFile_t *)Z_MallocInternal(
			(int)( scrParser_sourceFileCapacity * sizeof( scrSourceFile_t ) ) );
		Com_Memcpy( newFiles, scrParser_sourceFiles,
					scrParser_sourceFileCount * sizeof( scrSourceFile_t ) );
		free( scrParser_sourceFiles );
		scrParser_sourceFiles = newFiles;
	}

	return &scrParser_sourceFiles[ scrParser_sourceFileCount++ ];
}

/* ---- Scr_AddSourceBuffer  0x0046EC50 ---- VERIFIED */
char *Scr_AddSourceBuffer( const char *filename, const char *codeStart,
						   const char *developerCodeStart ) {
	fileHandle_t        f;
	int                 sourceLen;
	char               *sourceBuf;
	const char         *source;
	char               *dest;
	char               *tracked;
	char               *trackedSource;
	unsigned int        nameLen;
	int                 i;
	char                c;
	scrSourceFile_t    *record;

	if ( scrParser_savedSource ) {
		scrParser_savedSourceCount--;
		sourceLen = scrParser_savedSource[ scrParser_savedSourceCount ].sourceLen;
		if ( sourceLen < 0 ) {
			goto missing;
		}

		sourceBuf = (char *)Hunk_AllocateTempMemoryHighInternal( sourceLen + 1 );
		source = scrParser_savedSource[ scrParser_savedSourceCount ].source;
		dest = sourceBuf;
		for ( i = 0; i < sourceLen; i++ ) {
			c = *source++;
			if ( !c ) {
				c = '\n';
			}
			*dest++ = c;
		}
		*dest = '\0';

		if ( scrParser_savedSource[ scrParser_savedSourceCount ].source ) {
			free( scrParser_savedSource[ scrParser_savedSourceCount ].source );
		}
	} else {
		sourceLen = FS_FOpenFileByMode( filename, &f, FS_READ );
		if ( sourceLen < 0 ) {
			goto missing;
		}

		sourceBuf = (char *)Hunk_AllocateTempMemoryHighInternal( sourceLen + 1 );
		FS_Read( sourceBuf, sourceLen, f );
		sourceBuf[sourceLen] = '\0';
		FS_FCloseFile( f );
	}

	if ( !scrParser_sourceFiles ) {
		scrParserPub_sourceBuf = 0;
		return sourceBuf;
	}

	nameLen = strlen( filename );
	tracked = (char *)Z_MallocInternal( (int)( sourceLen + nameLen + 3 ) );
	strcpy( tracked, filename );
	trackedSource = tracked + nameLen + 1;

	source = sourceBuf;
	dest = trackedSource;
	for ( i = 0; i <= sourceLen; i++ ) {
		c = *source++;
		c = ( c == '\n' ) ? '\0' : c;   /* select form: retail's sete/dec/and */
		*dest++ = c;
	}

	record = Scr_GetNewSourceBuffer();
	record->codeStart[0]       = codeStart;
	record->codeStart[1]       = developerCodeStart;
	record->name               = tracked;
	record->source             = trackedSource;
	record->sourceLen          = sourceLen;

	scrParserPub_sourceBuf = (int)trackedSource;
	return sourceBuf;

missing:
	if ( scrParser_sourceFiles ) {
		record = Scr_GetNewSourceBuffer();
		record->codeStart[0]       = NULL;
		record->codeStart[1]       = NULL;
		record->name               = NULL;
		record->source             = NULL;
		record->sourceLen          = -1;
	}
	return NULL;
}

/* ---- PrintSourcePos  0x0046EDE0 ---- */
/* Retail sizes both the line copy and the caret line with alloca
 * (0x0046EE36, 0x0046EEA3) so neither is ever truncated. */
static void PrintSourcePos( const char *filename, const char *source,
							unsigned int sourcePos ) {
	const char     *lineStart;
	int             line;
	int             lineLen;
	int             column;
	int             i;
	char           *buffer;
	char            c;
	char           *caret;

	lineStart = source;
	line      = 1;

	for ( ; sourcePos; sourcePos-- ) {
		if ( !*source ) {
			lineStart = source + 1;
			line++;
		}
		source++;
	}

	lineLen = strlen( lineStart );
	buffer = (char *)_alloca( lineLen + 1 );
	for ( i = 0; i <= lineLen; i++ ) {
		c = lineStart[i];
#ifdef __cplusplus
		/* as C++ the conditional is char-typed: written directly, it gives retail's registers */
		buffer[i] = ( c == '\t' ) ? ' ' : c;
#else
		c = ( c == '\t' ) ? ' ' : c;
		buffer[i] = c;
#endif
	}

	Com_Printf( "(file '%s'%s, line %d)\n",
				filename,
				scrParser_savedSource ? " (savegame)" : "",
				line );
	Com_Printf( "%s\n", buffer );

	column = source - lineStart;
	caret = (char *)_alloca( column + 2 );
	Com_Memset( caret, ' ', column );
	caret[column]     = '*';
	caret[column + 1] = '\0';
	Com_Printf( "%s\n", caret );
}

/* ---- Scr_PrintPrevCodePos  0x0046EEE0 ---- */
/* Like retail, no i < 0 clamp: with no source files it reads entry -1. */
void Scr_PrintPrevCodePos( const char *codePos, int offset ) {
	int                 isDeveloper;
	int                 i;
	const char         *start;

	if ( !scrCompilePub_scriptsAvailable ) {
		Com_Printf( "@ %d\n", (int)( codePos - (const char *)scrVarPub_programBuffer ) );
		return;
	}

	isDeveloper = !Scr_IsInOpcodeMemory( codePos );

	for ( i = (int)scrParser_sourceFileCount - 1; i > 0; i-- ) {
		start = scrParser_sourceFiles[i].codeStart[isDeveloper];
		if ( start && start < codePos ) {
			break;
		}
	}

	PrintSourcePos( scrParser_sourceFiles[i].name, scrParser_sourceFiles[i].source,
					GetPrevSourcePos( codePos, offset ) );
}

/* ---- CompileError  0x0046EF80 ---- */
void QDECL CompileError( unsigned int sourcePos, const char *format, ... ) {
	char        buffer[1024];
	va_list     argptr;

	Com_Printf( "\n" );
	Com_Printf( "******* script compile error *******\n" );

	va_start( argptr, format );
	vsprintf( buffer, format, argptr );
	va_end( argptr );

	if ( !scrCompilePub_scriptsAvailable ) {
		Com_Printf( "%s\n", buffer );
	} else {
		Com_Printf( "%s: ", buffer );
		PrintSourcePos( (const char *)scrParserPub_scriptfilename,
						(const char *)scrParserPub_sourceBuf, sourcePos );
	}
	Com_Printf( "************************************\n" );
	Com_Error( ERR_DROP, "\x15script compile error\n(see console for details)" );
}

/* ---- CompileError2  0x0046F050 ---- */
void QDECL CompileError2( const char *codePos, const char *format, ... ) {
	char        buffer[1024];
	va_list     argptr;

	Com_Printf( "\n" );
	Com_Printf( "******* script compile error *******\n" );

	va_start( argptr, format );
	vsprintf( buffer, format, argptr );
	va_end( argptr );

	Com_Printf( "%s: ", buffer );
	Scr_PrintPrevCodePos( codePos, 0 );
	Com_Printf( "************************************\n" );
	Com_Error( ERR_DROP, "\x15script compile error\n(see console for details)" );
}

/* ---- RuntimeError  0x0046F0F0 ---- */
void RuntimeError( const char *dialogMessage, const char *codePos, int offset,
				   const char *message ) {
	int             i;

	if ( !scrCompilePub_scriptsAvailable && !scrVmPub_terminal_error ) {
		return;
	}

	Com_Printf( "\n" );
	Com_Printf( "******* script runtime error *******\n" );
	Com_Printf( "%s: ", message );
	Scr_PrintPrevCodePos( codePos, offset );

	for ( i = scrVmGlob_inparamcount - 1; i >= 0; i-- ) {
		Com_Printf( "called from:\n" );
		Scr_PrintPrevCodePos( ( (const char **)&scrVmGlob_callStack )[i], 0 );
	}
	Com_Printf( "************************************\n" );

	/* retail 0x0046F186: code = terminal_error ? 1 : 6 */
	Com_Error( scrVmPub_terminal_error ? ERR_DROP : ERR_SCRIPT,
			   "\x15script runtime error\n(see console for details)%s%s",
			   dialogMessage ? "\n" : "", dialogMessage ? dialogMessage : "" );
}

#ifdef __cplusplus
}
#endif
