#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

#include "../qcommon/vc7_compat.h"

typedef struct cvar_s cvar_t;
extern int   Com_Init( char *commandLine );
extern void  Com_Frame( void );
extern void  Com_Printf( const char *fmt, ... );
extern void  IN_Frame( void );
extern void  Sys_CreateConsole( void );
extern void  Sys_CreateSplashWindow( void );
extern void  Sys_ShowSplashWindow( void );
extern void  Sys_ShowConsole( int level, int quitOnClose );
extern int   SEH_LoadLocalizationFile_m( void );
extern int   SEH_ShutdownLocalization_m( void );   /* 0x00462430, win_main.c */
extern void  Cod1_InstallCrashReporter( void );
extern void  Cod1_InstallHangWatchdog( void );
extern void  Cod1_HangWatchdogTick( void );
extern int   Sys_BuildInstanceMutexName_m( void );
extern int   Sys_CheckCrashOrRerun( void );
extern int   Sys_ChecksumModuleImage_m( int module );
extern double Sys_GetCPUSpeed( void );
extern int   Sys_GetSystemRam( void );
extern int   Sys_GetVideoRam( void );
extern int   Sys_DetectVideoRamMegs_m( void );
extern int   Sys_HasSSE_m( void );
extern int   Sys_DetectHardware_m( void );   /* 0x004626E0, win_main.c */
extern int   Sys_Milliseconds( void );

extern int  *com_dedicated;
extern int  *com_viewlog;
#define CVAR_INTEGER( p )   ( *(int *)( (char *)(p) + 32 ) )

#include "../qcommon/win_vars.h"
char         commandLine[1024];   /* 0x008E3710; last byte 0x008E3B0F = IDA byte_8E3B0F */

int          sys_exeChecksum;   /* 0x008E3708, passed on the "monkey" rerun command line */

/* ---- WinMain  0x004640B0 ---- */
/* Retail inlines Sys_DetectHardware_m (with a CALL to Sys_HasSSE_m); VC7 keeps the tree's call
   while Sys_HasSSE_m's asm gets inlined into Sys_DetectHardware_m. Byte-identical once
   win_shared.c's Sys_HasSSE_m has its noSSE label as a C label after the asm block (blocks inlining). */
int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                       LPSTR lpCmdLine, int nCmdShow ) {
	char cwd[256];

	(void) nCmdShow;

#ifndef COD1_VC7   /* tree-only scaffolding, not in retail */
	Cod1_InstallCrashReporter();

	Cod1_InstallHangWatchdog();
#endif

	SEH_LoadLocalizationFile_m();

	if ( _strnicmp( lpCmdLine, "allowdupe", 9 ) || lpCmdLine[9] > 32 ) {
		Sys_BuildInstanceMutexName_m();
		if ( !Sys_CheckCrashOrRerun() ) {
			/* retail 0x464100: both early exits clear seh_localizationBase /
			   seh_localizationTable (inlined 0x00462430) before returning 0 */
			SEH_ShutdownLocalization_m();
			return 0;
		}
	}

	if ( hPrevInstance ) {
		SEH_ShutdownLocalization_m();
		return 0;
	}

	Sys_DetectHardware_m();
	sys_exeChecksum = Sys_ChecksumModuleImage_m( (int) hInstance );

	g_wv_hInstance = hInstance;
	strncpy( commandLine, lpCmdLine, sizeof( commandLine ) - 1 );
	commandLine[sizeof( commandLine ) - 1] = 0;

	Sys_CreateConsole();
	Sys_CreateSplashWindow();
	Sys_ShowSplashWindow();

	SetErrorMode( SEM_FAILCRITICALERRORS );

	Sys_Milliseconds();

	Com_Init( commandLine );

	_getcwd( cwd, sizeof( cwd ) );
	Com_Printf( "Working directory: %s\n", cwd );

	if ( !CVAR_INTEGER( com_dedicated ) && !CVAR_INTEGER( com_viewlog ) ) {
		Sys_ShowConsole( 0, 0 );
	}

	SetFocus( (HWND) g_wv_hWnd );

	while ( 1 ) {
#ifndef COD1_VC7
		Cod1_HangWatchdogTick();
#endif
		if ( g_wv_isMinimized || ( com_dedicated && CVAR_INTEGER( com_dedicated ) ) ) {
			Sleep( 5 );
		}
		IN_Frame();
		Com_Frame();
	}

	return 0;
}
