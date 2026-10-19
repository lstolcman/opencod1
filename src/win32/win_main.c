/*
 * @fidelity: likely
 */

#include "../qcommon/qcommon.h"
#include "../qcommon/win_vars.h"

/* Globals owned by this unit. */
char sys_cdRootPath[260];
unsigned char FileName[32];   /* 0x008E3B10, the single-instance marker file name */
cvar_t *sys_cpuMHz;   /* 0x008E3C4C */
cvar_t *sys_sysMB;   /* 0x008E3B30 */
cvar_t *sys_vidMB;   /* 0x008E3C50 */
double sys_cpuMHzValue;   /* 0x008E3B38 */
signed __int32 sys_sysMBValue;   /* 0x008E3B40 */
int sys_vidMBValue;   /* 0x008E3B44 */
int sys_hasSSE;   /* 0x016BF5A0 */
int sys_monkeySpank;   /* 0x008E3C48 */

extern void         NET_Config( qboolean enableNetworking );
extern unsigned int Sys_DetectVideoRamMegs_m( void );
extern int          Sys_HasSSE_m( void );

/* dllimport/__stdcall as windows.h declares them: retail calls every one through its __imp_ slot */
__declspec( dllimport ) void *   __stdcall CreateFileA( const char *name, unsigned access, unsigned share,
                                                         void *sec, unsigned disp, unsigned flags, void *tmpl );
__declspec( dllimport ) int      __stdcall ReadFile( void *h, void *buf, unsigned n, unsigned *got, void *ov );
__declspec( dllimport ) int      __stdcall WriteFile( void *h, const void *buf, unsigned n, unsigned *put, void *ov );
__declspec( dllimport ) int      __stdcall CloseHandle( void *h );
__declspec( dllimport ) int      __stdcall DeleteFileA( const char *p );
__declspec( dllimport ) unsigned __stdcall GetDriveTypeA( const char *root );
__declspec( dllimport ) unsigned __stdcall GetModuleFileNameA( void *m, char *buf, unsigned n );
__declspec( dllimport ) int      __stdcall FreeLibrary( void *m );
__declspec( dllimport ) int      __stdcall IsBadReadPtr( const void *p, unsigned n );
__declspec( dllimport ) int __stdcall MessageBoxA( void *h, const char *text, const char *caption, unsigned type );

extern qboolean SEH_GetLanguageIndexForName( const char *name, int *languageOut );

/* Com_Parse (universal/q_parse.c, 0x00449F80) comes from qcommon.h. */

#define SEH_LOCALIZATION_SIZE   4096

/* retail 0x008E2700: both cursors and the file buffer are one object, so a store into
   the buffer makes retail reload the cursors */
static struct {
	int  base;                              /* 0x008E2700 */
	int  table;                             /* 0x008E2704 */
	char buffer[SEH_LOCALIZATION_SIZE];     /* 0x008E2708 */
} seh_localization;

#define seh_localizationBase    seh_localization.base
#define seh_localizationTable   seh_localization.table
#define seh_localizationBuffer  seh_localization.buffer

/* ---- SEH_LoadLocalizationFile_m  0x00462350 ----  VERIFIED */
int SEH_LoadLocalizationFile_m( void )
{
	FILE *f;
	int   len;
	int   i;
	int   language;

	seh_localizationBase = 0;
	seh_localizationTable = 0;

	f = fopen( "localization.txt", "r" );
	if ( !f ) {
		return 0;
	}

	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fseek( f, 0, SEEK_SET );

	/* retail reads the whole file with no clamp to the 4096-byte buffer, and
	   reuses len for the byte count (both in edi) */
	seh_localizationBase = (int) seh_localizationBuffer;
	len = (int) fread( seh_localizationBuffer, 1, len, f );
	fclose( f );

	if ( !len ) {
		seh_localizationBase = 0;
		return 0;
	}

	/* every access goes through the int cursor: retail reloads it after each store */
	( (char *) seh_localizationBase )[len] = 0;
	language = 0;
	for ( i = 0; ( (char *) seh_localizationBase )[i]; i++ ) {
		if ( ( (char *) seh_localizationBase )[i] == '\n' ) {
			( (char *) seh_localizationBase )[i] = 0;
			seh_localizationTable = seh_localizationBase + i + 1;
			SEH_GetLanguageIndexForName( (char *) seh_localizationBase, &language );
			break;
		}
	}
	return language;
}

/* ---- SEH_ShutdownLocalization_m  0x00462430 ----  VERIFIED */
int SEH_ShutdownLocalization_m()
{
  int result;

  result = 0;
  seh_localizationBase = 0;
  seh_localizationTable = 0;
  return result;
}

/* ---- SEH_LocalizeIdentity_m  0x00462440 ----  VERIFIED */
char *__cdecl SEH_LocalizeIdentity_m(const char *text)
{
  return va("%s", text);
}

/* ---- SEH_GetLocalizedString_m  0x00462450 ----  VERIFIED */
char *__cdecl SEH_GetLocalizedString_m( const char *key )
{
	char *data_p;
	char *token;
	int   matched;

	/* retail opens the parse session unconditionally; with no table loaded
	   data_p is NULL, Com_ParseExt returns "" and the key is echoed back */
	Com_BeginParseSession( "localization" );
	data_p = (char *) seh_localizationTable;

	for ( ;; ) {
		token = Com_Parse( &data_p );
		if ( !*token ) {
			break;
		}
		matched = ( strcmp( token, key ) == 0 );

		token = Com_Parse( &data_p );
		if ( !*token ) {
			break;
		}
		if ( matched ) {
			Com_EndParseSession();
			return va( "%s", token );
		}
	}

	Com_EndParseSession();
	return va( "%s", key );
}

/* ---- Sys_GetInfo  0x004625A0 ----  VERIFIED */
void __cdecl Sys_GetInfo( int *out )
{
	/* int-pointer reads reproduce retail's 16-byte struct copy (load/store interleave) */
	out[0] = ( (const int *) &sys_cpuMHzValue )[0];
	out[1] = ( (const int *) &sys_cpuMHzValue )[1];
	out[2] = *(const int *) &sys_sysMBValue;
	out[3] = *(const int *) &sys_vidMBValue;
}

/* ---- Sys_UpdateForConfigChange  0x004625D0 ----  VERIFIED */
/* No code or data reference in retail. */
int Sys_UpdateForConfigChange( void )
{
	/* nested: the title is still resolved before the body (right-to-left) */
	return MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_CONFIGURE_UPDATED_BODY" ),
	                    SEH_GetLocalizedString_m( "WIN_CONFIGURE_UPDATED_TITLE" ),
	                    0x44 /* MB_YESNO|MB_ICONINFORMATION */ ) == 6 /* IDYES */;
}

/* ---- Sys_ConfigureChecksumChanged  0x00462600 ----  VERIFIED */
int __cdecl Sys_ConfigureChecksumChanged( int checksum )
{
	cvar_t     *sum;
	int        result = 0;

	sum = Cvar_Get( "sys_configSum", "0", CVAR_ARCHIVE | CVAR_ROM );
	if ( sum->integer && sum->integer != checksum ) {
		result = MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_CONFIGURE_UPDATED_BODY" ),
		                      SEH_GetLocalizedString_m( "WIN_CONFIGURE_UPDATED_TITLE" ),
		                      0x44 /* MB_YESNO|MB_ICONINFORMATION */ ) == 6 /* IDYES */;
	}

	/* retail re-reads sum->integer after the prompt */
	if ( !sum->integer || sum->integer != checksum ) {
		Cvar_Set2( "sys_configSum", va( "%i", checksum ), qtrue );
	}

	return result;
}

/* ---- Sys_RegisterInfoCvars  0x00462690 ----  VERIFIED */
cvar_t *Sys_RegisterInfoCvars( void )
{
	cvar_t *result;

	sys_cpuMHz = (int) Cvar_Get( "sys_cpuMHz", "0", CVAR_ARCHIVE | CVAR_ROM );
	sys_sysMB  = (int) Cvar_Get( "sys_sysMB",  "0", CVAR_ARCHIVE | CVAR_ROM );
	result     = Cvar_Get( "sys_vidMB", "0", CVAR_ARCHIVE | CVAR_ROM );
	sys_vidMB  = (int) result;
	return result;
}

/* ---- Sys_DetectHardware_m  0x004626E0 ----  VERIFIED */
int Sys_DetectHardware_m( void )
{
	sys_cpuMHzValue   = Sys_GetCPUSpeed();	/* 0x004626E6 fstp qword [0x008E3B38] */
	sys_sysMBValue = Sys_GetSystemRam();
	sys_vidMBValue = Sys_DetectVideoRamMegs_m();

	sys_hasSSE = Sys_HasSSE_m();
	return sys_hasSSE;
}

/* ---- Sys_IsLowMemory_m  0x00462920 ----  VERIFIED */
int Sys_IsLowMemory_m()
{
  return sys_sysMBValue <= 100663296;
}

/* ---- Sys_WriteCrashMarker_m  0x00462940 ----  VERIFIED */
void Sys_WriteCrashMarker_m( void )
{
	void *h;

	h = CreateFileA( "crash", 0x40000000u, 0, 0, 1u, 2u, 0 );
	if ( h != (void *) -1 ) {
		CloseHandle( h );
	}
}

/* ---- cand_Sys_VerifyFunctionIntegrity  0x00462970 ----  VERIFIED */
/* Compares two copies of a function for byte-level integrity.  It builds the
   sentinel pattern C3 90 90 00.. (ret; nop; nop; then zeroes), follows a
   leading E9 rel32 thunk on each argument to the real entry, then scans up to
   1024 bytes of func2 for that pattern to locate the end of the body.  The two
   bodies are then compared byte-for-byte over that length; a byte that begins
   an E8 rel32 CALL counts as equal when both calls resolve to the same absolute
   target (so relocated calls do not trip the check).  Returns 1 when the two
   bodies are equivalent, else 0. */
int __fastcall cand_Sys_VerifyFunctionIntegrity( const byte *code1, const byte *code2 )
{
	byte        pattern[32];
	const byte  *func1;
	const byte  *func2;
	int         i;
	int         j;
	int         length;
	int         pos;
	byte        b;

	pattern[0] = 0xC3;
	pattern[1] = 0x90;
	pattern[2] = 0x90;
	pattern[3] = 0;
	memset( &pattern[4], 0, 28 );

	func2 = ( code2[0] == 0xE9 ) ? code2 + 5 + *(const int *)( code2 + 1 ) : code2;
	func1 = ( code1[0] == 0xE9 ) ? code1 + 5 + *(const int *)( code1 + 1 ) : code1;

	for ( i = 0; i < 1024; i++ ) {
		j = 0;
		while ( pattern[j] != 0 ) {
			if ( func2[i + j] != pattern[j] ) {
				break;
			}
			j++;
		}
		if ( pattern[j] == 0 ) {
			break;
		}
	}

	length = i + 2;

	for ( pos = 0; pos < length; pos++ ) {
		b = func2[pos];

		if ( b == 0xE8
		     && func2 + pos + 5 + *(const int *)( func2 + pos + 1 )
		        == func1 + pos + 5 + *(const int *)( func1 + pos + 1 ) ) {
			/* identical E8 rel32 CALL to the same absolute target: skip it */
			pos += 4;
		} else if ( b != func1[pos] ) {
			return 0;
		}
	}

	return 1;
}

/* ---- cand_Sys_ScanForInlineHook  0x00462A70 ----  VERIFIED */
/* Follows a leading E9 rel32 thunk on funcStart to the real entry, scans up to
   1024 bytes for the C3 90 90 (ret; nop; nop) end-of-body sentinel, and returns
   a checksum of the function body up to and including it.  (LTCG passes
   funcStart in edx; Hex-Rays' unused ecx "a1" is not a parameter.) */
unsigned int cand_Sys_ScanForInlineHook( byte *funcStart )
{
	byte    pattern[32];
	byte    *func;
	int     i;
	int     j;

	pattern[0] = 0xC3;
	pattern[1] = 0x90;
	pattern[2] = 0x90;
	pattern[3] = 0;
	memset( &pattern[4], 0, 28 );

	func = ( funcStart[0] == 0xE9 ) ? funcStart + 5 + *(const int *)( funcStart + 1 ) : funcStart;

	for ( i = 0; i < 1024; i++ ) {
		j = 0;
		while ( pattern[j] != 0 ) {
			if ( func[i + j] != pattern[j] ) {
				break;
			}
			j++;
		}
		if ( pattern[j] == 0 ) {
			break;
		}
	}

	return Com_BlockChecksum( func, i + 2 );
}

/* ---- cand_Sys_GetIntegrityState  0x00462B10 ----  VERIFIED */
int cand_Sys_GetIntegrityState()
{
  return sys_monkeySpank;
}

extern void     CL_ShutdownRef( void );
__declspec( dllimport ) unsigned __stdcall timeEndPeriod( unsigned uPeriod );

/* ---- Sys_OutOfMemoryPrep  0x00462B20 ----  VERIFIED */
void Sys_OutOfMemoryPrep( void )
{
	/* retail LTCG-inlines Sys_DestroySplashWindow (0x00465E10) and
	   CL_ShutdownRef (0x00411500) here; the eax left behind by the
	   inlined re.Shutdown test is not a return value */
	Sys_DestroySplashWindow();
	timeEndPeriod( 1 );
	IN_Shutdown();
	CL_ShutdownCGame();
	CL_ShutdownUI();
	CL_ShutdownRef();
}

/* ---- Sys_OutOfMemoryError  0x00462BA0 ----  VERIFIED */
void Sys_OutOfMemoryError( void )
{
	Sys_OutOfMemoryPrep();
	/* nested: the title is still resolved before the body (right-to-left) */
	MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_BODY" ),
	             SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_TITLE" ), 0x10 /* MB_ICONHAND */ );
	exit( -1 );
}

/* ---- Sys_UnableToLoadDllError_m  0x00462BE0 ----  VERIFIED */
void  Sys_UnableToLoadDllError_m()
{
  char *LocalizedString_m;

  LocalizedString_m = SEH_GetLocalizedString_m("WIN_UNABLE_LOAD_DLL_BODY");
  Com_Error(ERR_FATAL, "%s\n", LocalizedString_m);
}

/* 0x00462BF7 is not a function: it is the unreachable `add esp,10h; retn` tail after the
 * noreturn Com_Error call in Sys_UnableToLoadDllError_m.  The empty stub that stood
 * here has been removed. */

#define PE_DOS_LFANEW_OFFSET   0x3C          /* IMAGE_DOS_HEADER.e_lfanew */
#define PE_NT_SIGNATURE        0x00004550    /* 'PE\0\0' */
#define PE_NT_HEADERS_SIZE     0xF8          /* bytes covered by the read guard */

/* ---- Sys_ChecksumModuleImage_m  0x00462C00 ----  VERIFIED */
/* Checksums a loaded module's first section (normally .text).  moduleBase is an
   image base; e_lfanew at +0x3C locates the NT headers.  Note that ntHeaders[65]
   and ntHeaders[66] are +260/+264 -- past the 0xF8-byte NT header, i.e.
   section[0].VirtualAddress and section[0].SizeOfRawData -- so the IsBadReadPtr
   guard does not cover the two fields actually read (a retail quirk kept intact).
   Retail compiles this as __usercall (base in EDI); the tree keeps it __cdecl. */
unsigned int __cdecl Sys_ChecksumModuleImage_m( int moduleBase )
{
	const unsigned int *ntHeaders;

	ntHeaders = (const unsigned int *)( moduleBase + *(const int *)( moduleBase + PE_DOS_LFANEW_OFFSET ) );

	if ( IsBadReadPtr( ntHeaders, PE_NT_HEADERS_SIZE ) || ntHeaders[0] != PE_NT_SIGNATURE ) {
		return 0;
	}

	return Com_BlockChecksum( (void *)( moduleBase + ntHeaders[65] ), ntHeaders[66] );
}

__declspec( dllimport ) unsigned __stdcall GetCurrentDirectoryA( unsigned nBufferLength, char *lpBuffer );
__declspec( dllimport ) unsigned __stdcall GetLastError( void );
__declspec( dllimport ) unsigned __stdcall FormatMessageA( unsigned dwFlags, const void *lpSource,
                                          unsigned dwMessageId, unsigned dwLanguageId,
                                          char *lpBuffer, unsigned nSize, void *Arguments );
__declspec( dllimport ) int      __stdcall CreateProcessA( const char *lpApplicationName, char *lpCommandLine,
                                          void *lpProcessAttributes, void *lpThreadAttributes,
                                          int bInheritHandles, unsigned dwCreationFlags,
                                          void *lpEnvironment, const char *lpCurrentDirectory,
                                          void *lpStartupInfo, void *lpProcessInformation );

typedef struct {
	unsigned  cb;
	char     *lpReserved;
	char     *lpDesktop;
	char     *lpTitle;
	unsigned  dwX;
	unsigned  dwY;
	unsigned  dwXSize;
	unsigned  dwYSize;
	unsigned  dwXCountChars;
	unsigned  dwYCountChars;
	unsigned  dwFillAttribute;
	unsigned  dwFlags;
	unsigned short wShowWindow;
	unsigned short cbReserved2;
	unsigned char *lpReserved2;
	void     *hStdInput;
	void     *hStdOutput;
	void     *hStdError;
} winStartupInfo_t;

typedef struct {
	void     *hProcess;
	void     *hThread;
	unsigned  dwProcessId;
	unsigned  dwThreadId;
} winProcessInfo_t;

__declspec( dllimport ) unsigned __stdcall GetCurrentProcessId( void );
__declspec( dllimport ) void *   __stdcall OpenProcess( unsigned access, int inherit, unsigned pid );
__declspec( dllimport ) unsigned __stdcall GetTempPathA( unsigned nBufferLength, char *lpBuffer );
__declspec( dllimport ) unsigned __stdcall GetTempFileNameA( const char *lpPathName, const char *lpPrefixString,
                                                             unsigned uUnique, char *lpTempFileName );
__declspec( dllimport ) int      __stdcall CopyFileA( const char *lpExistingFileName, const char *lpNewFileName,
                                                      int bFailIfExists );
__declspec( dllimport ) unsigned __stdcall WaitForSingleObject( void *hHandle, unsigned dwMilliseconds );
__declspec( dllimport ) int      __cdecl   wsprintfA( char *lpOut, const char *lpFmt, ... );

extern int sys_exeChecksum;   /* 0x008E3708, win_main_entry.c */

char *sys_monkeyId = "q3monkeyid";   /* 0x00575978 */

/* ---- Sys_MonkeyCheck_m  0x00462C40 ----  VERIFIED */
/* Unreferenced in retail.  Without "monkey" as argv[1] it copies the running exe to a
   delete-on-close temp file, runs the copy as `<temp> monkey <checksum> <handle> "<exe>"`
   and exits.  The copy waits for that process to end, then writes the checksum (big-endian,
   six zero bytes after it) over the "q3monkeyid" tag in the original exe and exits. */
void Sys_MonkeyCheck_m( void )
{
	winProcessInfo_t ProcessInformation;
	winStartupInfo_t StartupInfo;
	char             tempPath[260];
	char             exePath[260];
	char             CommandLine[512];
	void            *hFile;
	void            *hProcess;
	FILE            *f;
	int              len;
	byte            *buf;
	int              tagLen;
	int              i;

	if ( Q_stricmp( __argv[1], "monkey" ) ) {
		GetModuleFileNameA( NULL, exePath, sizeof( exePath ) );
		GetTempPathA( sizeof( tempPath ), tempPath );
		GetTempFileNameA( tempPath, "Del", 0, tempPath );
		CopyFileA( exePath, tempPath, 0 );
		hFile = CreateFileA( tempPath, 0, 1 /* FILE_SHARE_READ */, NULL, 3 /* OPEN_EXISTING */,
		                     0x4000000 /* FILE_FLAG_DELETE_ON_CLOSE */, NULL );
		hProcess = OpenProcess( 0x100000 /* SYNCHRONIZE */, 1, GetCurrentProcessId() );
		wsprintfA( CommandLine, "%s monkey %d %d \"%s\"", tempPath, sys_exeChecksum, hProcess, exePath );
		memset( &StartupInfo, 0, sizeof( StartupInfo ) );
		StartupInfo.cb = sizeof( StartupInfo );
		CreateProcessA( NULL, CommandLine, NULL, NULL, 1, 0, NULL, NULL, &StartupInfo, &ProcessInformation );
		CloseHandle( hProcess );
		CloseHandle( hFile );
	} else {
		sys_exeChecksum = atoi( __argv[2] );
		hProcess = (void *) atoi( __argv[3] );
		WaitForSingleObject( hProcess, 0xFFFFFFFF /* INFINITE */ );
		CloseHandle( hProcess );

		f = fopen( __argv[4], "rb" );
		if ( !f ) {
			return;
		}
		fseek( f, 0, SEEK_END );
		len = ftell( f );
		fseek( f, 0, SEEK_SET );
		buf = (byte *) Z_MallocInternal( len );
		if ( fread( buf, len, 1, f ) != 1 ) {
			return;
		}
		fclose( f );

		tagLen = (int) strlen( sys_monkeyId );
		for ( i = 0; i < len; i++ ) {
			if ( !Q_strncmp( (char *) buf + i, sys_monkeyId, tagLen ) ) {
				break;
			}
		}
		if ( i >= len ) {
			return;
		}

		buf[i]     = (byte)( sys_exeChecksum >> 24 );
		buf[i + 1] = (byte)( sys_exeChecksum >> 16 );
		buf[i + 2] = (byte)( sys_exeChecksum >> 8 );
		buf[i + 3] = (byte) sys_exeChecksum;
		buf[i + 4] = buf[i + 5] = buf[i + 6] = buf[i + 7] = buf[i + 8] = buf[i + 9] = 0;

		f = fopen( __argv[4], "wb" );
		if ( !f ) {
			return;
		}
		if ( fwrite( buf, len, 1, f ) != 1 ) {
			return;
		}
		fclose( f );
		free( buf );
	}
	exit( 0 );
}

/* ---- Sys_StartProcess  0x00462EF0 ----  VERIFIED */
void Sys_StartProcess( const char *cmd, int doexit )
{
	winProcessInfo_t ProcessInformation;
	winStartupInfo_t StartupInfo;
	char            *msgbuf;
	char             Buffer[260];
	unsigned         lastError;

	memset( &StartupInfo, 0, sizeof( StartupInfo ) );
	StartupInfo.cb = sizeof( StartupInfo );

	GetCurrentDirectoryA( 0x104, Buffer );

	if ( !CreateProcessA( NULL, va( "\"%s\\%s\"", Buffer, cmd ),
	                      NULL, NULL, 0, 0, NULL, NULL,
	                      &StartupInfo, &ProcessInformation ) )
	{
		lastError = GetLastError();
		FormatMessageA( 0x1300, NULL, lastError, 0x400, (char *) &msgbuf, 0, NULL );
		Com_Error( ERR_DROP, va( "EXE_ERR_COULDNT_START_PROCESS\x15" "'%s\\%s'\n%s\n%08x",
		                         Buffer, cmd, msgbuf, lastError ) );
	}

	if ( doexit ) {
		Cbuf_AddText( "quit\n" );
	}
}

/* dllimport, as windows.h declares them: retail calls all three through their __imp_ slots, and under LTCG
   the first declaration linked decides that for every unit (win_syscon.c's ShowWindow calls too). */
__declspec( dllimport ) void * __stdcall ShellExecuteA( void *hwnd, const char *op, const char *file,
                                                        const char *params, const char *dir, int nShowCmd );
__declspec( dllimport ) void * __stdcall GetForegroundWindow( void );
__declspec( dllimport ) int    __stdcall ShowWindow( void *hWnd, int nCmdShow );

#define SW_MAXIMIZE_    3
#define SW_RESTORE_     9

/* ---- Sys_OpenURL  0x00462FE0 ----  VERIFIED */
void Sys_OpenURL( const char *url, int doexit )
{
	void *wnd;

	if ( !ShellExecuteA( NULL, "open", url, NULL, NULL, SW_RESTORE_ ) ) {
		Com_Error( ERR_DROP, va( "EXE_ERR_COULDNT_OPEN_URL\x15" "%s", url ) );
	}

	wnd = GetForegroundWindow();
	if ( wnd ) {
		ShowWindow( wnd, SW_MAXIMIZE_ );
	}

	if ( doexit ) {
		Cbuf_AddText( "quit\n" );
	}
}

/* ---- Sys_BeginProfiling  0x00463040 ----  VERIFIED */
void Sys_BeginProfiling()
{
  ;
}

/* ---- Sys_Print  0x004631B0 ----  VERIFIED */
void __cdecl Sys_Print( const char *msg )
{
	Conbuf_AppendText( msg );
}

/* ---- Sys_RunSetup_m  0x004631C0 ----  VERIFIED */
int Sys_RunSetup_m( void )
{
	FILE *f;
	char  rootPath[4];
	char  path[256];

	/* the drive letter lives in rootPath[0] itself (retail's byte inc/cmp loop) */
	rootPath[1] = ':';
	rootPath[2] = '\\';
	rootPath[3] = 0;
	for ( rootPath[0] = 'c'; rootPath[0] <= 'z'; rootPath[0]++ ) {
		if ( GetDriveTypeA( rootPath ) != 5 /* DRIVE_CDROM */ ) {
			continue;
		}

		/* both sprintf calls are bounded by their constant-length inputs */
		sprintf( (char *) sys_cdRootPath, "%s%s", rootPath, "" );
		sprintf( path, "%s\\%s", (char *) sys_cdRootPath, "setup\\setup.exe" );
		f = fopen( path, "r" );
		if ( f ) {
			fclose( f );
			return 1;
		}

		sprintf( (char *) sys_cdRootPath, "%s%s", rootPath, "bin\\x86\\glibc-2.1" );
		sprintf( path, "%s\\%s", (char *) sys_cdRootPath, "setup\\setup" );
		f = fopen( path, "r" );
		if ( f ) {
			fclose( f );
			return 1;
		}
	}

	return 0;
}

/* ---- Sys_CheckCD  0x004632F0 ----  VERIFIED */
int Sys_CheckCD()
{
  return 1;
}

__declspec( dllimport ) int      __stdcall OpenClipboard( void *hWndNewOwner );
__declspec( dllimport ) void *   __stdcall GetClipboardData( unsigned uFormat );
__declspec( dllimport ) int      __stdcall CloseClipboard( void );
__declspec( dllimport ) void *   __stdcall GlobalLock( void *hMem );
__declspec( dllimport ) unsigned __stdcall GlobalSize( void *hMem );
__declspec( dllimport ) int      __stdcall GlobalUnlock( void *hMem );

/* ---- Sys_GetClipboardData  0x00463300 ----  VERIFIED */
char *Sys_GetClipboardData( void )
{
	char   *data = NULL;
	char   *cliptext;
	void   *hClipboardData;

	if ( OpenClipboard( NULL ) != 0 ) {
		if ( ( hClipboardData = GetClipboardData( 1  ) ) != 0 ) {
			if ( ( cliptext = (char *) GlobalLock( hClipboardData ) ) != 0 ) {
				unsigned size;

				data = (char *) Z_MallocInternal( GlobalSize( hClipboardData ) + 1 );
				size = GlobalSize( hClipboardData );
				strncpy( data, cliptext, size - 1 );
				data[ size - 1 ] = 0;
				GlobalUnlock( hClipboardData );

				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}

/* ---- Sys_UnloadDll  0x00463380 ----  VERIFIED */
void __cdecl Sys_UnloadDll(void *dllHandle)
{
  if ( dllHandle )
  {
    if ( !FreeLibrary(dllHandle) )
      Com_Error(ERR_FATAL, "\x15Sys_UnloadDll FreeLibrary failed");
  }
}

/* Sys_LoadDll (0x004633A0) is defined in win_platform.c. */

/* Sys_QueEvent (0x00463580) is defined in win_platform.c. */

typedef struct {
	void     *hwnd;
	unsigned  message;
	unsigned  wParam;
	unsigned  lParam;
	unsigned  time;
	long      pt_x;
	long      pt_y;
} winMsg_t;

extern int      __stdcall PeekMessageA( winMsg_t *lpMsg, void *hWnd, unsigned min,
                                        unsigned max, unsigned wRemoveMsg );
extern int      __stdcall GetMessageA( winMsg_t *lpMsg, void *hWnd, unsigned min,
                                       unsigned max );
extern int      __stdcall TranslateMessage( const winMsg_t *lpMsg );
extern long     __stdcall DispatchMessageA( const winMsg_t *lpMsg );
extern unsigned __stdcall SetThreadExecutionState( unsigned esFlags );

/* ---- Sys_LoadingKeepAlive  0x00463640 ----  VERIFIED */
unsigned Sys_LoadingKeepAlive( void )
{
	winMsg_t msg;

	if ( PeekMessageA( &msg, NULL, 0x218, 0x218, 0  ) ) {
		if ( GetMessageA( &msg, NULL, 0x218, 0x218 ) > 0 ) {
			TranslateMessage( &msg );
			DispatchMessageA( &msg );
		}
	}
	return SetThreadExecutionState( 2  );
}

/* ---- Sys_In_Restart_f  0x004638A0 ----  VERIFIED */
void Sys_In_Restart_f( void )
{
	IN_Shutdown();
	IN_Init();
}

/* ---- Sys_Net_Restart_f  0x004638B0 ----  VERIFIED */
void Sys_Net_Restart_f(void)
{
  NET_Config(networkingEnabled);
}

/* Sys_Init (0x004638C0) is defined in win_platform.c. */

extern void *   __stdcall CreateToolhelp32Snapshot( unsigned flags, unsigned pid );
extern int      __stdcall Module32First( void *snapshot, void *me );
extern int      __stdcall Module32Next( void *snapshot, void *me );

typedef struct sys_moduleEntry32_s {
	unsigned long   dwSize;
	unsigned long   th32ModuleID;
	unsigned long   th32ProcessID;
	unsigned long   GlblcntUsage;
	unsigned long   ProccntUsage;
	unsigned char  *modBaseAddr;
	unsigned long   modBaseSize;
	void           *hModule;
	char            szModule[256];
	char            szExePath[260];
} sys_moduleEntry32_t;

typedef char sys_moduleEntry32_size_check[ sizeof( sys_moduleEntry32_t ) == 548 ? 1 : -1 ];

__declspec(noreturn) void Sys_OutOfDiskSpaceError_m(void) {
 MessageBoxA(0,SEH_GetLocalizedString_m("WIN_DISK_FULL_BODY"),SEH_GetLocalizedString_m("WIN_DISK_FULL_TITLE"),0x10u);
 exit(-1);
}

/* ---- Sys_FindModuleInProcess_m  0x00463DC0 ----  VERIFIED */
int __cdecl Sys_FindModuleInProcess_m( unsigned int pid )
{
	sys_moduleEntry32_t me;
	void *process;
	void *snapshot;
	char  ourPath[260];
	const char *ourName;
	char *p;
	int   found;

	process = OpenProcess( 0x1F0FFFu, 0, pid );
	if ( !process ) {
		return 0;
	}
	CloseHandle( process );

	snapshot = CreateToolhelp32Snapshot( 8u, pid );
	if ( snapshot == (void *) -1 ) {
		return 0;
	}

	/* found is first assigned here: retail keeps it in ebx, its zero register */
	found = 0;

	/* retail 0x00463E20: dwSize is set once, and our own path is only fetched
	   after Module32First succeeds (return value ignored, terminator forced) */
	me.dwSize = sizeof( me );
	if ( Module32First( snapshot, &me ) ) {
		GetModuleFileNameA( 0, ourPath, sizeof( ourPath ) );
		ourPath[sizeof( ourPath ) - 1] = 0;

		/* retail recognises only '\\' and ':' as separators */
		ourName = ourPath;
		for ( p = ourPath; *p; p++ ) {
			if ( *p == '\\' || *p == ':' ) {
				ourName = p + 1;
			}
		}

		/* retail 0x00463E7C compares with the CRT _stricmp */
		do {
			if ( !_stricmp( me.szModule, ourName ) ) {
				found = 1;
				break;
			}
		} while ( Module32Next( snapshot, &me ) );
	}

	CloseHandle( snapshot );
	return found;
}

/* ---- Sys_BuildInstanceMutexName_m  0x00463ED0 ----  VERIFIED */
int Sys_BuildInstanceMutexName_m( void )
{
	char path[260];
	const char *base;
	char *p;

	/* retail ignores the return value and only forces the terminator */
	GetModuleFileNameA( 0, path, (unsigned) sizeof( path ) );
	path[sizeof( path ) - 1] = 0;

	/* retail recognises only '\\' and ':' as separators and nulls every '.'
	   in place, so the name stops at the base name's first '.' */
	base = path;
	for ( p = path; *p; p++ ) {
		if ( *p == '\\' || *p == ':' ) {
			base = p + 1;
		} else if ( *p == '.' ) {
			*p = 0;
		}
	}

	/* retail's unbounded sprintf into the 32-byte FileName (a base name over
	   29 characters overruns it, into 0x8E3B30 in retail) */
	return sprintf( (char *) FileName, "__%s", base );
}

/* ---- Sys_CheckCrashOrRerun  0x00463F60 ----  VERIFIED */
int Sys_CheckCrashOrRerun( void )
{
	void *h;
	unsigned int pid;
	unsigned int marker;
	unsigned int transferred;

	/* retail has no empty-FileName guard: after "allowdupe" (FileName never
	   built) both CreateFileA calls fail and the disk-full path exits */
	pid = GetCurrentProcessId();

	/* ReadFile/WriteFile always store the byte count, and marker is only read
	   after 4 bytes arrived: retail initialises neither */
	h = CreateFileA( (const char *) FileName, 0x80000000u, 0, 0, 3u, 2u, 0 );
	if ( h != (void *) -1 ) {
		if ( ReadFile( h, &marker, 4u, &transferred, 0 ) && transferred == 4 ) {
			int answer;

			CloseHandle( h );

			/* retail 0x00463FD8: a live owner returns 0 silently, with no message */
			if ( pid != marker && Sys_FindModuleInProcess_m( marker ) ) {
				return 0;
			}

			/* nested: the title is still resolved before the body (right-to-left) */
			answer = MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_IMPROPER_QUIT_BODY" ),
			                      SEH_GetLocalizedString_m( "WIN_IMPROPER_QUIT_TITLE" ),
			                      0x33u /* MB_YESNOCANCEL|MB_ICONEXCLAMATION */ );
			if ( answer == 6 ) {            /* IDYES */
				com_safemode = 1;
			} else if ( answer == 7 ) {     /* IDNO: retail tests it explicitly, then writes the marker */
			} else if ( answer == 2 ) {     /* IDCANCEL; any other value falls through to the marker write */
				return 0;
			}
		} else {
			CloseHandle( h );
		}
	}

	h = CreateFileA( (const char *) FileName, 0x40000000u, 0, 0, 2u, 2u, 0 );
	if ( h != (void *) -1 ) {
		if ( WriteFile( h, &pid, 4u, &transferred, 0 ) && transferred == 4 ) {
			CloseHandle( h );
			return 1;
		}
		CloseHandle( h );
	}

	MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_DISK_FULL_BODY" ),
	             SEH_GetLocalizedString_m( "WIN_DISK_FULL_TITLE" ), 0x10u /* MB_ICONHAND */ );
	exit( -1 );
	return 0;
}

/* ---- Sys_DeleteInstanceMarker  0x004640A0 ----  VERIFIED */
/* retail `push 8E3B10h; call [DeleteFileA]` with no empty-FileName guard,
   here and at every inlined copy (an empty name just fails the delete) */
int Sys_DeleteInstanceMarker( void )
{
	return DeleteFileA( (const char *) FileName );
}
