/*
 * @fidelity: likely
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../qcommon/qcommon.h"
#include "../qcommon/win_vars.h"
/* ---- module-owned globals ---- */
int eventHead;
int eventTail;
WinVars_t g_wv;	/* 0x016C35E0 */
COD1_ASSERT_SIZE( WinVars_t, 0xB0 );
unsigned char s_userName[1024];


extern int      Sys_GetPacket( msg_t *msg, netadr_t *from );
extern char     *Sys_ConsoleInput( void );
extern void     Sys_In_Restart_f( void );
extern void     Sys_Net_Restart_f( void );
extern int      Sys_GetProcessorId( void );
extern int __stdcall MainWndProc( void *hWnd, unsigned Msg,
                                  unsigned wParam, long lParam );
/* Retail's only caller pushes a fourth 0 (0x00463470) that the callee never reads. */
extern int      FS_ExtractFromPakFile_m( char *ospath, int game, char *qpath, int unused );
/* The retail join, universal/com_files.c 0x00428EE0: three components, and an empty `game` defaults to fs_gamedir. */
extern void     FS_BuildOSPath3( const char *base, const char *game,
                                 const char *qpath, char *out,
                                 int allowOverflow );

/* win32/win_main.c, retail 0x00462450 (localization.txt key lookup) */
extern char     *SEH_GetLocalizedString_m( const char *key );



unsigned __int32 sys_timeBase;  /* 0x016C35C0 */
signed __int32 sys_timeBaseInit;   /* 0x014073B8 */

/* ---- Sys_Milliseconds  0x004659D0 ----  VERIFIED */
int Sys_Milliseconds( void ) {
	int sys_curtime;

	if ( !sys_timeBaseInit ) {
		sys_timeBase = (int) timeGetTime();
		sys_timeBaseInit = 1;
	}
	sys_curtime = (int) timeGetTime() - sys_timeBase;

	return sys_curtime;
}

#define MAX_QUED_EVENTS     256
#define MASK_QUED_EVENTS    ( MAX_QUED_EVENTS - 1 )

/* Retail's ring is at 0x016BDDA0, 256 * 24 bytes, sitting exactly between eventHead (0x016BDD9C) and eventTail (0x016BF5A4). */
static sysEvent_t   eventQue[MAX_QUED_EVENTS];

/* Retail receives packets into a 0x4000 buffer at 0x016BF5C0; the storage is owned here. */
static byte         sys_packetReceived[MAX_MSGLEN];

/* ---- Sys_QueEvent  0x00463580 ----  VERIFIED */
void Sys_QueEvent( int time, int type, int value, int value2,
                   int ptrLength, void *ptr ) {
	sysEvent_t  *ev;

	ev = &eventQue[eventHead & MASK_QUED_EVENTS];

	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf( "Sys_QueEvent: overflow\n" );
		if ( ev->evPtr ) {
			free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = (sysEventType_t) type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/* ---- Sys_GetEvent  0x004636A0 ----  VERIFIED */
sysEvent_t Sys_GetEvent( void ) {
	MSG         msg;
	sysEvent_t  ev;
	char        *s;
	msg_t       netmsg;
	netadr_t    adr;

	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[( eventTail - 1 ) & MASK_QUED_EVENTS];
	}

	while ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
		if ( !GetMessage( &msg, NULL, 0, 0 ) ) {
			Com_Quit_f();
		}
		g_wv_sysMsgTime = msg.time;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	s = Sys_ConsoleInput();
	if ( s ) {
		char    *b;
		int     len;

		/* Retail 0x0046373F (Sys_ConsoleInput inlined): the line ends in the
		 * '\n' InputLineWndProc appended; Q_strncpyz to len - 1 chops it,
		 * but allocates and queues strlen + 1 bytes (the RTCW form). */
		len = strlen( s ) + 1;
		b = Z_MallocInternal( len );
		Q_strncpyz( b, s, len - 1 );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket( &netmsg, &adr ) ) {
		netadr_t    *buf;
		int         len;

		len = netmsg.cursize - netmsg.readcount;
		buf = Z_MallocInternal( sizeof( netadr_t ) + len );
		*buf = adr;
		memcpy( buf + 1, &netmsg.data[netmsg.readcount], len );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, sizeof( netadr_t ) + len, buf );
	}

	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[( eventTail - 1 ) & MASK_QUED_EVENTS];
	}

	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = (int) timeGetTime();

	return ev;
}

/* ---- Sys_GetCPUSpeed  0x00460B80 ---- */
/* Returns the x87 st0 double; the caller (Sys_DetectHardware_m) does the
   fstp qword to sys_cpuMHzValue.  Retail has no cache, no store and no
   zero guard: rdtsc straight into the locals (0x00460BBB / 0x00460BD2),
   0x00460C11 _allmul( tscEnd - tscStart, freq ) -> fild int64, fmul 1e-6
   (0x568F08), fild int64 QPC delta, fdivp. */
double Sys_GetCPUSpeed( void ) {
	LARGE_INTEGER   freq, start, end;
	__int64         tscStart, tscEnd;

	Sleep( 0 );

	tscStart       = 0;
	tscEnd         = 0;
	start.QuadPart = 0;
	end.QuadPart   = 0;

	QueryPerformanceFrequency( &freq );
	QueryPerformanceCounter( &start );
	__asm {
		_emit 0x0F
		_emit 0x31
		mov     dword ptr [tscStart], eax
		mov     dword ptr [tscStart + 4], edx
	}
	QueryPerformanceCounter( &start );

	Sleep( 250 );

	__asm {
		_emit 0x0F
		_emit 0x31
		mov     dword ptr [tscEnd], eax
		mov     dword ptr [tscEnd + 4], edx
	}
	QueryPerformanceCounter( &end );

#ifdef COD1_VC7
	{
		double      mhz;	/* the named result gives retail's sub esp,2Ch frame */

		mhz = (double) ( ( tscEnd - tscStart ) * freq.QuadPart ) * 0.000001
		      / (double) ( end.QuadPart - start.QuadPart );
		return mhz;
	}
#else
	{
		__int64     cycles, ticks;
		double      scale = 0.000001;	/* qword 0x568F08 (0x3EB0C6F7A0B5ED8D) */
		double      mhz;

		cycles = ( tscEnd - tscStart ) * freq.QuadPart;
		ticks  = end.QuadPart - start.QuadPart;

		/* Retail fmuls the fild'd 64-bit product straight off the x87 stack, so
		   the product is rounded once, at _PC_53, by the fmul.  A C (double) cast
		   compiles to fild / fstp qword / fld under /fp:precise and rounds the
		   product to 53 bits first, which double-rounds once it exceeds 2^53
		   (~0.25 s of a 3.6 GHz TSC times a 10 MHz QPC).  st0 after fdivp is
		   already a 53-bit value, so the fstp here is exact. */
		__asm fild  qword ptr cycles
		__asm fmul  qword ptr scale
		__asm fild  qword ptr ticks
		__asm fdivp st(1), st
		__asm fstp  qword ptr mhz

		return mhz;
	}
#endif
}

/* Sys_GetSystemRam 0x00460C50 */

typedef struct {
	DWORD               dwLength;
	DWORD               dwMemoryLoad;
	unsigned __int64    ullTotalPhys;
	unsigned __int64    ullAvailPhys;
	unsigned __int64    ullTotalPageFile;
	unsigned __int64    ullAvailPageFile;
	unsigned __int64    ullTotalVirtual;
	unsigned __int64    ullAvailVirtual;
	unsigned __int64    ullAvailExtendedVirtual;
} sysMemoryStatusEx_t;

typedef BOOL ( WINAPI *pfnGlobalMemoryStatusEx_t )( sysMemoryStatusEx_t *lpBuffer );

#define SYS_LOW_MEMORY_BYTES    0x8000000

/* RoundFloatToInt_m (win_shared.c, 0x00460B50): fistp of megs + (0.5 - 2^-30);
   LTCG inlines it at both sites below, materializing the bias as a double
   local (0x00460D0B / 0x00460E06) at each inlined copy. */
extern int      RoundFloatToInt_m( float f );

/* Low-memory prompt, written out at both low-memory sites of Sys_GetSystemRam
   (0x00460C9D and 0x00460DB1; retail has no out-of-line copy, so it is not a
   static helper): the title is resolved before the body, and anything but Yes
   deletes the instance marker (DeleteFileA(FileName), 0x00460CC8 / 0x00460DDC)
   and exits. */
#define SYS_LOW_MEMORY_PROMPT() \
	do { \
		if ( MessageBoxA( NULL, SEH_GetLocalizedString_m( "WIN_LOW_MEMORY_BODY" ), \
		                  SEH_GetLocalizedString_m( "WIN_LOW_MEMORY_TITLE" ), \
		                  MB_YESNO | MB_ICONEXCLAMATION /* 0x34 */ ) != IDYES ) { \
			Sys_DeleteInstanceMarker(); \
			exit( 0 ); \
		} \
	} while ( 0 )

/* VC7 compares the byte count at x87 precision; a modern float compare would
   round it to 24 bits first. */
#ifdef COD1_VC7
#define SYS_RAM_BYTES( x )  ( x )
#else
#define SYS_RAM_BYTES( x )  ( (double) ( x ) )
#endif

/* ---- Sys_GetSystemRam  0x00460C50 ---- */
/* No cache and no store: the caller (Sys_DetectHardware_m) keeps the result
   in sys_sysMBValue.  Retail converts the byte count at each use (unsigned
   __int64 or DWORD, 0x00460CDA / 0x00460DEE) and caps the result at 1 GB. */
int Sys_GetSystemRam( void ) {
	HMODULE                     kernel32;
	pfnGlobalMemoryStatusEx_t   pGlobalMemoryStatusEx;
	sysMemoryStatusEx_t         statex;
	MEMORYSTATUS                stat;
	int                         mb;

	kernel32 = GetModuleHandleA( "kernel32.dll" );
	if ( kernel32 ) {
		pGlobalMemoryStatusEx = (pfnGlobalMemoryStatusEx_t)
		                        GetProcAddress( kernel32, "GlobalMemoryStatusEx" );
		if ( pGlobalMemoryStatusEx ) {
			statex.dwLength = 64;
			pGlobalMemoryStatusEx( &statex );

			if ( statex.ullAvailVirtual < SYS_LOW_MEMORY_BYTES ) {
				SYS_LOW_MEMORY_PROMPT();
			}
			mb = RoundFloatToInt_m( statex.ullTotalPhys * ( 1.0f / 1048576.0f ) );
			if ( mb * 1048576.0f < SYS_RAM_BYTES( statex.ullTotalPhys ) || mb > 1024 ) {
				mb = 1024;
			}
			return mb;
		}
	}

	stat.dwLength = sizeof( stat );
	GlobalMemoryStatus( &stat );

	if ( stat.dwAvailVirtual < SYS_LOW_MEMORY_BYTES ) {
		SYS_LOW_MEMORY_PROMPT();
	}
	mb = RoundFloatToInt_m( stat.dwTotalPhys * ( 1.0f / 1048576.0f ) );
	if ( mb * 1048576.0f < SYS_RAM_BYTES( stat.dwTotalPhys ) || mb > 1024 ) {
		mb = 1024;
	}
	return mb;
}

extern int      Sys_VideoRamFromRenderer( const char *renderer );

#define GL_RENDERER     0x1F01

/* ---- Sys_GetVideoRam  0x00461390 ----  VERIFIED */
/* Returns BYTES (Sys_VideoRamFromRenderer's megs << 20), 0 when no GL
   context can be made or the renderer is not in the card table. */
int Sys_GetVideoRam( void ) {
	HWND        hWnd;
	HDC         hDC;
	HMODULE     hOpenGL;
	HGLRC       ( WINAPI *qwglCreateContext )( HDC );
	BOOL        ( WINAPI *qwglMakeCurrent )( HDC, HGLRC );
	BOOL        ( WINAPI *qwglDeleteContext )( HGLRC );
	const char  *( WINAPI *qglGetString )( unsigned int );
	PIXELFORMATDESCRIPTOR pfd;
	int         pixelformat;
	HGLRC       hGLRC;
	int         vidMem;

	vidMem = 0;

	hWnd = CreateWindowEx( 0, "static", "dummy", 0, 0, 0, 1, 1,
	                       NULL, NULL, GetModuleHandle( NULL ), NULL );
	if ( hWnd ) {
		hDC = GetDC( hWnd );
		if ( hDC ) {
			hOpenGL = LoadLibrary( "opengl32.dll" );
			if ( hOpenGL ) {
				qwglCreateContext = (void *) GetProcAddress( hOpenGL, "wglCreateContext" );
				qwglMakeCurrent = (void *) GetProcAddress( hOpenGL, "wglMakeCurrent" );
				qwglDeleteContext = (void *) GetProcAddress( hOpenGL, "wglDeleteContext" );
				qglGetString = (void *) GetProcAddress( hOpenGL, "glGetString" );

				memset( &pfd, 0, sizeof( pfd ) );
				pfd.nSize = sizeof( pfd );
				pfd.nVersion = 1;
				pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
				pfd.iPixelType = PFD_TYPE_RGBA;
				pfd.cColorBits = 0;
				pfd.cDepthBits = 0;

				pixelformat = ChoosePixelFormat( hDC, &pfd );
				if ( pixelformat ) {
					DescribePixelFormat( hDC, pixelformat, sizeof( pfd ), &pfd );
					if ( SetPixelFormat( hDC, pixelformat, &pfd ) ) {
						hGLRC = qwglCreateContext( hDC );
						if ( hGLRC ) {
							qwglMakeCurrent( hDC, hGLRC );
							vidMem = Sys_VideoRamFromRenderer( qglGetString( GL_RENDERER ) );
							qwglMakeCurrent( NULL, NULL );
							qwglDeleteContext( hGLRC );
						}
					}
				}
				FreeLibrary( hOpenGL );
			}
			DeleteDC( hDC );
		}
		DestroyWindow( hWnd );
	}

	return vidMem;
}

/* ---- Sys_SetConfigCvars  0x00462850 ----  VERIFIED */
void Sys_SetConfigCvars( int configSum ) {
	/* retail stores all three (0x0046286D / 0x00462883 / 0x00462893) */
	sys_cpuMHz = Cvar_Get( "sys_cpuMHz", "0", CVAR_ARCHIVE | CVAR_ROM );
	sys_sysMB = Cvar_Get( "sys_sysMB", "0", CVAR_ARCHIVE | CVAR_ROM );
	sys_vidMB = Cvar_Get( "sys_vidMB", "0", CVAR_ARCHIVE | CVAR_ROM );

	/* retail reads the stored hardware globals; it measures nothing here */
	Cvar_Set2( "sys_cpuMHz", va( "%lg", sys_cpuMHzValue ), qtrue );
	Cvar_Set2( "sys_sysMB", va( "%i", sys_sysMBValue ), qtrue );
	Cvar_Set2( "sys_vidMB", va( "%i", sys_vidMBValue ), qtrue );
	Cvar_Set2( "sys_configSum", va( "%i", configSum ), qtrue );
}

/* ---- Sys_PromptReconfigure  0x00462710 ---- */
/* Rewrites the cvars and asks with a localized Yes/No box (title resolved
   before body, 0x0046271C / 0x0046272A).  Retail inlines the first call in
   Sys_ShouldReconfigure and tail-jumps here from the second (0x00462806). */
static qboolean Sys_PromptReconfigure( void ) {
	Sys_SetConfigCvars( 0 );
	return MessageBoxA( NULL, SEH_GetLocalizedString_m( "WIN_COMPUTER_CHANGE_BODY" ),
	                    SEH_GetLocalizedString_m( "WIN_COMPUTER_CHANGE_TITLE" ),
	                    MB_YESNO | MB_ICONINFORMATION /* 0x44 */ ) == IDYES;
}

/* ---- Sys_ShouldReconfigure  0x00462750 ----  VERIFIED */
/* Compares the archived cvars against the stored hardware globals
   (sys_cpuMHzValue double, sys_sysMBValue, sys_vidMBValue).  On a mismatch
   it prompts; Com_Init runs Com_SetRecommended(0) when the answer is Yes. */
qboolean Sys_ShouldReconfigure( void ) {
	sys_cpuMHz = Cvar_Get( "sys_cpuMHz", "0", CVAR_ARCHIVE | CVAR_ROM );
	sys_sysMB = Cvar_Get( "sys_sysMB", "0", CVAR_ARCHIVE | CVAR_ROM );
	sys_vidMB = Cvar_Get( "sys_vidMB", "0", CVAR_ARCHIVE | CVAR_ROM );

	/* 0x004627B1 test ah,5 / jnp and 0x004627C9 test ah,41h / je fail only on an
	   ordered compare, so an unordered (NaN) MHz passes both bounds */
	if ( sys_cpuMHz->value > sys_cpuMHzValue * 1.100000023841858
	     || sys_cpuMHz->value < sys_cpuMHzValue * 0.8999999761581421 ) {
		return Sys_PromptReconfigure();
	}
	if ( sys_sysMB->integer > sys_sysMBValue + 32
	     || sys_sysMB->integer < sys_sysMBValue - 32
	     || sys_vidMB->integer > sys_vidMBValue + 16
	     || sys_vidMB->integer < sys_vidMBValue - 16 ) {
		return Sys_PromptReconfigure();
	}
	return qfalse;
}

/* ---- Sys_GetCurrentUser  0x00465C20 ----  VERIFIED */
char *Sys_GetCurrentUser( void ) {
	static char s_userName[1024];
	DWORD       size;

	size = sizeof( s_userName );
	if ( !GetUserNameA( s_userName, &size ) ) {
		strcpy( s_userName, "player" );
	}
	if ( !s_userName[0] ) {
		strcpy( s_userName, "player" );
	}
	return s_userName;
}

#define CPUID_GENERIC               0x00
#define CPUID_AXP                   0x10
#define CPUID_INTEL_UNSUPPORTED     0x20
#define CPUID_INTEL_PENTIUM         0x21
#define CPUID_INTEL_MMX             0x22
#define CPUID_INTEL_KATMAI          0x23
#define CPUID_AMD_3DNOW             0x30

/* ---- Sys_Init  0x004638C0 ----  VERIFIED */
/* Retail calls GetVersionEx straight into g_wv.osversion (0x004638EA) and
   re-reads sys_cpustring for every compare, as Q3 did. */
void Sys_Init( void ) {
	int             cpuid;

	timeBeginPeriod( 1 );

	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );
	Cmd_AddCommand( "net_restart", Sys_Net_Restart_f );

	g_wv.osversion.dwOSVersionInfoSize = sizeof( g_wv.osversion );
	if ( !GetVersionExA( (OSVERSIONINFOA *)&g_wv.osversion ) ) {
		Sys_Error( "Couldn't get OS info" );
	}

	if ( g_wv.osversion.dwMajorVersion < 4 ) {
		Sys_Error( "Call of Duty Multiplayer requires Windows version 4 or greater" );
	}
	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32s ) {
		Sys_Error( "Call of Duty Multiplayer doesn't run on Win32s" );
	}

	if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
		Cvar_Set2( "arch", "winnt", qtrue );
	} else if ( g_wv.osversion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
		if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= 0x7CE ) {
			Cvar_Set2( "arch", "win98", qtrue );
		} else if ( LOWORD( g_wv.osversion.dwBuildNumber ) >= 0x457 ) {
			Cvar_Set2( "arch", "win95 osr2.x", qtrue );
		} else {
			Cvar_Set2( "arch", "win95", qtrue );
		}
	} else {
		Cvar_Set2( "arch", "unknown Windows variant", qtrue );
	}

	Cvar_Get( "win_hinstance", va( "%i", g_wv_hInstance ), CVAR_ROM );
#ifndef DEDICATED
	Cvar_Get( "win_wndproc", va( "%i", (int) MainWndProc ), CVAR_ROM );
#endif

	Cvar_Get( "sys_cpustring", "detect", 0 );

	if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "detect" ) ) {
		Com_Printf( "...detecting CPU, found " );

		cpuid = Sys_GetProcessorId();

		switch ( cpuid ) {
		case CPUID_GENERIC:
			Cvar_Set2( "sys_cpustring", "generic", qtrue );
			break;
		case CPUID_INTEL_UNSUPPORTED:
			Cvar_Set2( "sys_cpustring", "x86 (pre-Pentium)", qtrue );
			break;
		case CPUID_INTEL_PENTIUM:
			Cvar_Set2( "sys_cpustring", "x86 (P5/PPro, non-MMX)", qtrue );
			break;
		case CPUID_INTEL_MMX:
			Cvar_Set2( "sys_cpustring", "x86 (P5/Pentium2, MMX)", qtrue );
			break;
		case CPUID_INTEL_KATMAI:
			Cvar_Set2( "sys_cpustring", "Intel Pentium III", qtrue );
			break;
		case CPUID_AMD_3DNOW:
			Cvar_Set2( "sys_cpustring", "AMD w/ 3DNow!", qtrue );
			break;
		case CPUID_AXP:
			Cvar_Set2( "sys_cpustring", "Alpha AXP", qtrue );
			break;
		default:
			Com_Error( ERR_FATAL, "\x15" "Unknown cpu type %d\n", cpuid );
			break;
		}
	} else {
		Com_Printf( "...forcing CPU type to " );

		if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "generic" ) ) {
			cpuid = CPUID_GENERIC;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "x87" ) ) {
			cpuid = CPUID_INTEL_PENTIUM;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "mmx" ) ) {
			cpuid = CPUID_INTEL_MMX;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "3dnow" ) ) {
			cpuid = CPUID_AMD_3DNOW;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "PentiumIII" ) ) {
			cpuid = CPUID_INTEL_KATMAI;
		} else if ( !Q_stricmp( Cvar_VariableString( "sys_cpustring" ), "axp" ) ) {
			cpuid = CPUID_AXP;
		} else {
			Com_Printf( "WARNING: unknown sys_cpustring '%s'\n", Cvar_VariableString( "sys_cpustring" ) );
			cpuid = CPUID_GENERIC;
		}
	}
	Cvar_SetValue( "sys_cpuid", (float) cpuid );
	Com_Printf( "%s\n", Cvar_VariableString( "sys_cpustring" ) );

	/* retail reads the stored hardware globals (0x00463CB1 / 0x00463CCA) */
	Com_Printf( "Measured CPU speed is %.2lf GHz\n", sys_cpuMHzValue * 0.001 );
	Com_Printf( "System memory is %i MB (capped at 1 GB)\n", sys_sysMBValue );
	Com_Printf( "Video card memory is %i MB\n", sys_vidMBValue );
	Com_Printf( "Streaming SIMD Extensions (SSE) %ssupported\n",
	            sys_hasSSE ? "" : "not " );
	Com_Printf( "\n" );

	Cvar_Set2( "username", Sys_GetCurrentUser(), qtrue );

	IN_Init();
}

/* ---- Sys_ShutdownTimer  no-address ---- */
void Sys_ShutdownTimer( void ) {
	timeEndPeriod( 1 );
}

/* ---- Sys_BuildDllPath  no-address ----  VERIFIED */
static void Sys_BuildDllPath( const char *base, const char *game,
                              const char *dllName, char *out ) {
	/* retail 0x00463434 / 0x004634D3: FS_BuildOSPath3( fs_basepath@<eax>, game, dllName, out, 0 ). Empty game defaults to fs_gamedir inside. */
	FS_BuildOSPath3( base, game, dllName, out, 0 );
}

/* ---- Sys_LoadDll  0x004633A0 ---- */
/* Retail keeps fs_basepath as a pointer (no local copy) and copies the path
   that loaded into fqpath with an inlined Q_strncpyz on each branch. */
void *Sys_LoadDll( const char *name, char *fqpath,
                   int ( **entryPoint )( int, ... ),
                   int ( *systemcalls )( int, ... ) ) {
	char        dllName[64];
	char        ospath[MAX_OSPATH];
	const char  *basepath;
	const char  *gamedir;
	HMODULE     libHandle;
	void        ( *dllEntry )( int ( *syscallptr )( int, ... ) );

	*fqpath = 0;

	Com_sprintf( dllName, sizeof( dllName ), "%s_mp_x86.dll", name );

	basepath = Cvar_VariableString( "fs_basepath" );
	Cvar_FindVar( "fs_cdpath" );
	gamedir = Cvar_VariableString( "fs_game" );

	Sys_BuildDllPath( basepath, gamedir, dllName, ospath );

	if ( Q_strncmp( name, "game", 4 ) ) {
		if ( !FS_ExtractFromPakFile_m( ospath, (int) gamedir, dllName, 0 ) ) {
			if ( cl_connectedToPureServer ) {
				Com_Error( ERR_DROP,
				           va( "EXE_ERR_GAME_FAILED_PURE_CHECK\x15%s", dllName ) );
			}
		}
	}

	/* retail prints nothing anywhere in this function */
	libHandle = LoadLibraryA( ospath );
	if ( !libHandle ) {
		Sys_BuildDllPath( basepath, "main", dllName, ospath );
		libHandle = LoadLibraryA( ospath );
		if ( !libHandle ) {
			libHandle = LoadLibraryA( dllName );
			if ( !libHandle ) {
				return NULL;
			}
			Q_strncpyz( fqpath, dllName, 64 );
		} else {
			Q_strncpyz( fqpath, ospath, 64 );
		}
	} else {
		Q_strncpyz( fqpath, ospath, 64 );
	}

	dllEntry = (void ( * )( int ( * )( int, ... ) ))
	           GetProcAddress( libHandle, "dllEntry" );
	*entryPoint = (int ( * )( int, ... )) GetProcAddress( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry ) {
		FreeLibrary( libHandle );
		return NULL;
	}

	dllEntry( systemcalls );

	return libHandle;
}
