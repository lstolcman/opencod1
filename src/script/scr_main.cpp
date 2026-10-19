/*
 * script/scr_main.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_main.cpp
 *
 * Retail range 0x0046D380-0x0046DBB0, 12 functions.
 *
 * @fidelity: likely
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "scr_local.h"


/* Canonical hunk counters owned by com_memory_core.c. */
extern int s_hunkTotal;
extern int s_hunkHighPermanent;
extern int s_hunkHighTemp;


extern int hunk_low_permanent;    /* 0x008931C4, read directly as retail does */
extern void FS_ClearDataForFiles( void *lowEnd, void *highEnd );
extern void *Hunk_AllocAlignInternal( int size, int align );
extern void *Hunk_AllocLowAlignInternal( int size, int align );
extern int InitOpcodeLookup();
extern int ShutdownOpcodeLookup();
extern char *Scr_AddSourceBuffer( const char *filename, const char *codeStart,
                                  const char *developerCodeStart );
extern int Scr_InitDeveloperOpcodes__Fv();
extern int Scr_InsertDeveloperOpcodes__Fv();
extern int Scr_LoadAnimTreeAtIndex__FiPFi_Pv();
extern void ScriptCompile( int parseTree, unsigned short currentFunctionRoot );
extern int ScriptParse( const char *text, int *parseTree );
extern void XModelClearData( unsigned int rangeEnd, unsigned int rangeStart );

#define HUNK_HIGH_PERM      ( s_hunkHighPermanent )
#define HUNK_HIGH_TEMP      ( s_hunkHighTemp )
#define HUNK_TOTAL          ( s_hunkTotal )
#define HUNK_LOW_PERM       ( hunk_low_permanent )

#define SCR_HANDLE( g )     ( *(unsigned short *)&(g) )

/* 0x008E60BC: canonical count in the low word (kept in scrString_canonicalCount here),
   the two loading flags in bytes 2 and 3 */
#define SCR_LOADING_SCRIPTS ( *( (unsigned char *)&scrVarPub_canonicalStrCountAndLoadFlags + 2 ) )
#define SCR_LOADING_ANIMS   ( *( (unsigned char *)&scrVarPub_canonicalStrCountAndLoadFlags + 3 ) )

#define SCR_SCRIPTSPOS      scrCompilePub_scriptsPos
#define SCR_LOADEDSCRIPTS   scrCompilePub_loadedScripts
#define SCR_ANIMTREES       scrAnimPub_treeRoot

#define SCR_CANONICAL_MAP_BYTES     0x20000
#define SCR_FILENAME_BUFFER_SIZE    64

void Scr_BeginLoadAnimTrees( void );
void Scr_EndLoadAnimTrees( void );
void Scr_EndLoadScripts( void );

/* ---- Scr_GetFunctionHandle  0x0046D380 ---- */
int Scr_GetFunctionHandle( const char *filename, const char *funcname ) {
	unsigned short  canonical;
	unsigned short  fileId;
	unsigned short  fileObj;
	unsigned short  nameId;
	unsigned short  funcId;
	unsigned int    codePos;

	canonical = Scr_CreateCanonicalFilename( filename );
	fileId = FindVariable( SCR_SCRIPTSPOS, canonical );
	SL_RemoveRefToString( canonical );
	if ( !fileId ) {
		return 0;
	}

	fileObj = FindObject( fileId );
	nameId = SL_FindLowercaseString( funcname );
	if ( !nameId ) {
		return 0;
	}

	funcId = FindVariable( fileObj, nameId );
	if ( !funcId ) {
		return 0;
	}

	codePos = GetVariableValueAddress( funcId )->u.uintValue;
	if ( codePos < (unsigned int)scrVarPub_programBuffer ) {
		return 0;
	}
	if ( codePos >= (unsigned int)scrVarPub_programBuffer + (unsigned int)scrCompilePub_programLen ) {
		return 0;
	}
	return (int)( codePos - (unsigned int)scrVarPub_programBuffer );
}

/* ---- SL_BeginLoadScripts  0x0046D460 ---- VERIFIED */
void SL_BeginLoadScripts( void ) {
	hunk_highTempMark = HUNK_HIGH_PERM;

	scrString_canonicalMap = (unsigned short *)Hunk_AllocAlignInternal( SCR_CANONICAL_MAP_BYTES, 32 );
	scrString_canonicalCount = 0;
}

/* ---- SL_EndLoadScripts  0x0046D490 ---- VERIFIED */
/* Hunk_ClearToHighTempMark with its Hunk_RetouchMemory, inlined (the Mac build
   calls Hunk_ClearToMark2 -> Hunk_ClearData; locals follow Hunk_ClearData). */
void SL_EndLoadScripts( void ) {
	void    *lowEnd;
	void    *highEnd;

	HUNK_HIGH_TEMP = hunk_highTempMark;
	HUNK_HIGH_PERM = hunk_highTempMark;

	lowEnd = (char *)s_hunkData + HUNK_LOW_PERM;
	highEnd = (char *)s_hunkData + HUNK_TOTAL - HUNK_HIGH_PERM;

	XModelClearData( (unsigned int)highEnd, (unsigned int)lowEnd );
	FS_ClearDataForFiles( lowEnd, highEnd );
}

/* ---- SL_TransferToCanonicalString  0x0046D4D0 ---- VERIFIED */
unsigned short SL_TransferToCanonicalString( unsigned short handle ) {
	scrStringEntry_t   *entry = GetRefString( handle );
	unsigned short      id;

	if ( entry->flags & SCR_STRING_USER_CANONICAL ) {
		entry->refCount--;
	} else {
		entry->flags |= SCR_STRING_USER_CANONICAL;
	}

	id = scrString_canonicalMap[handle];
	if ( id ) {
		return id;
	}
	scrString_canonicalCount++;
	scrString_canonicalMap[handle] = scrString_canonicalCount;
	return scrString_canonicalCount;
}

/* ---- SL_FindCanonicalString  0x0046D520 ---- */
unsigned short SL_FindCanonicalString( const char *text ) {
	return scrString_canonicalMap[ SL_FindStringOfLen( text, (unsigned int)strlen( text ) + 1 ) ];
}

/* ---- Scr_BeginLoadScripts  0x0046D550 ---- */
void Scr_BeginLoadScripts( void ) {
	SCR_LOADING_SCRIPTS = 1;

	InitOpcodeLookup();
	Scr_InitDeveloperOpcodes__Fv();

	SCR_SCRIPTSPOS    = Scr_AllocArray();
	SCR_LOADEDSCRIPTS = Scr_AllocArray();

	scrVarPub_programBuffer = (int)Hunk_AllocLowAlignInternal( 0, 32 );
	scrCompilePub_programLen = 0;
	scrVarPub_endScriptBuffer = 0;

	SL_BeginLoadScripts();
	scrVarPub_fieldBuffer = NULL;

	Scr_BeginLoadAnimTrees();
}

/* ---- Scr_BeginLoadAnimTrees  0x0046D620 ---- */
/* Reached from Scr_BeginLoadScripts by its tail jmp at 0x0046D611. Retail resets
   the active XAnim pool's tree count and clears slot 0 of that pool's tree table
   before allocating the anim-tree root. */
void Scr_BeginLoadAnimTrees( void ) {
	scrAnimPub_treeCount[ xanim_activePoolSlot ] = 0;      /* 0x0046D628: [pool*4+0x008E58D0] = 0 */
	SCR_LOADING_ANIMS = 1;                                  /* 0x0046D632: byte 0x008E60BF = 1 */
	scrAnimPub_trees[ 128 * xanim_activePoolSlot ] = 0;     /* 0x0046D639: [(pool<<9)+0x008E5AD8] = 0, slot 0 */

	SCR_ANIMTREES = Scr_AllocArray();                        /* word 0x008E60C0 */
	scrAnimPub_currentTreeRoot = 0;                         /* word 0x008E5EDC */
}

/* ---- Scr_LoadScript  0x0046D690 ---- */
qboolean Scr_LoadScript( const char *filename ) {
	unsigned short  canonical;
	unsigned short  fileId;
	int             sourceBuf;
	int             savedSourcePos;
	const char     *savedFilename;
	void           *sourceMem;
	char            name[SCR_FILENAME_BUFFER_SIZE];
	int             parseTree;

	canonical = Scr_CreateCanonicalFilename( filename );

	if ( FindVariable( SCR_LOADEDSCRIPTS, canonical ) ) {
		SL_RemoveRefToString( canonical );
		return FindVariable( SCR_SCRIPTSPOS, canonical ) != 0 ? qtrue : qfalse;
	}

	GetVariableIndexInternal( SCR_LOADEDSCRIPTS, canonical );
	SL_RemoveRefToString( canonical );

	currentPos = 0;

	strncpy( name, canonical ? SL_ConvertToString( canonical ) : NULL,
			 SCR_FILENAME_BUFFER_SIZE );
	strncat( name, ".gsc", SCR_FILENAME_BUFFER_SIZE );

	savedSourcePos = scrCompilePub_sourceBuf;
	sourceMem = (void *)Hunk_AllocLowAlignInternal( 0, 32 );

	sourceBuf = (int)Scr_AddSourceBuffer( name, (const char *)sourceMem, developerCodeStart );
	if ( !sourceBuf ) {
		return qfalse;
	}

	savedFilename = (const char *)scrCompilePub_scriptfilename;
	scrAnimPub_usingTreeNames = 0;
	scrCompilePub_scriptCount = 0;
	scrCompilePub_scriptfilename = (int)name;

	ScriptParse( (const char *)sourceBuf, &parseTree );

	fileId = GetVariable( SCR_SCRIPTSPOS, canonical );
	ScriptCompile( parseTree, GetObject( fileId ) );

	scrCompilePub_scriptfilename = (int)savedFilename;
	scrCompilePub_sourceBuf = savedSourcePos;
	return qtrue;
}

/* ---- Scr_EndLoadScripts  0x0046D830 ---- */
void Scr_EndLoadScripts( void ) {
	SCR_LOADING_SCRIPTS = 0;
	SL_EndLoadScripts();

	ClearObject( SCR_LOADEDSCRIPTS );
	RemoveRefToObject( SCR_LOADEDSCRIPTS );
	SCR_LOADEDSCRIPTS = 0;

	ClearObject( SCR_SCRIPTSPOS );
	RemoveRefToObject( SCR_SCRIPTSPOS );
	SCR_SCRIPTSPOS = 0;

	Scr_InsertDeveloperOpcodes__Fv();
	SL_ShutdownSystem( SCR_STRING_USER_CANONICAL );
}

extern int  Scr_GetAnimTreeCount( void );
extern void Scr_LoadAnimTreeAtIndex( int treeIndex, void *(__cdecl *allocFn)( int ) );

/* ---- Scr_PrecacheAnimTrees  0x0046D990 ---- HIGH */
void Scr_PrecacheAnimTrees( void *allocFn ) {
	int i;

	for ( i = 1; i <= Scr_GetAnimTreeCount(); i++ ) {
		Scr_LoadAnimTreeAtIndex( i, (void *(__cdecl *)( int ))allocFn );
	}
}

/* ---- Scr_EndLoadAnimTrees  0x0046D9D0 ---- */
void Scr_EndLoadAnimTrees( void ) {
	SCR_LOADING_ANIMS = 0;

	ClearObject( SCR_ANIMTREES );
	RemoveRefToObject( SCR_ANIMTREES );
	SCR_ANIMTREES = 0;

	if ( scrAnimPub_currentTreeRoot ) {
		RemoveRefToObject( scrAnimPub_currentTreeRoot );
	}

	SL_ShutdownSystem( SCR_STRING_USER_CANONICAL );

	scrVarPub_endScriptBuffer = (int)Hunk_AllocLowAlignInternal( 0, 32 );
}

/* ---- Scr_FreeScripts  0x0046DAF0 ---- */
void Scr_FreeScripts( void ) {
	if ( SCR_LOADING_SCRIPTS ) {
		Scr_EndLoadScripts();
	}
	if ( SCR_LOADING_ANIMS ) {
		Scr_EndLoadAnimTrees();
	}

	scrVarPub_programBuffer = 0;
	scrCompilePub_programLen = 0;
	scrVarPub_endScriptBuffer = 0;
	scrCompilePub_checksum = 0;
	SCR_LOADING_SCRIPTS = 0;

	SL_ShutdownSystem( SCR_STRING_USER_SCRIPT );
	ShutdownOpcodeLookup();

	if ( scrVarPub_developerScript ) {
		if ( scrCompileGlob_devOpBuffer ) {
			free( (void *)scrCompileGlob_devOpBuffer );
			scrCompileGlob_devOpBuffer = 0;
		}
		if ( scrCompileGlob_devPatchTable ) {
			free( (void *)scrCompileGlob_devPatchTable );
			scrCompileGlob_devPatchTable = 0;
		}
	}
}

void SL_BeginLoadScripts__Fv( void ) { SL_BeginLoadScripts(); }
void SL_EndLoadScripts__Fv( void ) { SL_EndLoadScripts(); }
unsigned short SL_TransferToCanonicalString__FUs( unsigned short h ) {
	return SL_TransferToCanonicalString( h );
}
unsigned short SL_FindCanonicalString__FPCc( const char *text ) {
	return SL_FindCanonicalString( text );
}


#ifdef __cplusplus
}
#endif
