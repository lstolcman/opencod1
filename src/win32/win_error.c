#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

extern void Conbuf_AppendText( const char *msg );
extern void Sys_SetErrorText( const char *text );
extern void Sys_DestroyConsole( void );
extern void Sys_ShowConsole( int visLevel, int quitOnClose );
extern void Sys_OutOfMemoryPrep( void );
extern void Com_Quit_f( void );
extern void Sys_ShutdownTimer( void );
extern void IN_Shutdown( void );
extern int  Sys_DeleteInstanceMarker( void );
extern int  SEH_ShutdownLocalization_m( void );
extern void Cvar_Shutdown( void );
extern void Cmd_Shutdown( void );
#ifndef DEDICATED
extern void Key_Shutdown( void );
#endif
#ifdef DEDICATED
extern int  Sys_ConsoleWindowWanted( void );	/* tree-only, win_syscon.c */
#endif

/* ---- Sys_Error  0x00463050 ---- */
/* Retail never exits here: it shows the console, tears down the renderer and
 * input (Sys_OutOfMemoryPrep, which also destroys the splash), then pumps
 * messages until the console window's WM_CLOSE posts WM_QUIT
 * (quitOnClose = 1), and only then calls Com_Quit_f. */
void Sys_Error( const char *error, ... ) {
	va_list argptr;
	char    text[4096];
	MSG     msg;

	/* Retail 0x00463085: unbounded vsprintf (0x00526135) and no NUL store
	 * after it, so a longer message (e.g. "recursive error after: %s" with a
	 * full com_errorMessage) runs past text[] and is shown whole. */
	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	Conbuf_AppendText( text );	/* 0x00463091 */
	Conbuf_AppendText( "\n" );	/* 0x0046309B */
	Sys_SetErrorText( text );	/* 0x004630A4 */
	Sys_ShowConsole( 1, 1 );	/* inlined at 0x004630A9..0x004630E6 */
	Sys_OutOfMemoryPrep();		/* 0x004630EC */

#ifdef DEDICATED
	/* Tree-only, dedicated build only: under the tree's -nowindow headless
	 * mode there is no console window whose close could post WM_QUIT, so
	 * quit directly.  With a console window this is retail's path unchanged;
	 * the CodMP client goes straight to the retail loop. */
	if ( !Sys_ConsoleWindowWanted() ) {
		Com_Quit_f();
	}
#endif

	while ( 1 ) {	/* 0x00463103..0x00463127 */
		if ( !GetMessageA( &msg, NULL, 0, 0 ) ) {
			Com_Quit_f();
		}
		TranslateMessage( &msg );
		DispatchMessageA( &msg );
	}
}

/* ---- Sys_Quit  0x00463130 ---- */
/* Retail's only caller is Com_Quit_f's tail jump (0x00435E2D). */
void Sys_Quit( void ) {
	Sys_ShutdownTimer();
	IN_Shutdown();
	Sys_DestroyConsole();
	Sys_DeleteInstanceMarker();
	SEH_ShutdownLocalization_m();
	Cvar_Shutdown();
	Cmd_Shutdown();
#ifndef DEDICATED
	Key_Shutdown();
#endif
	exit( 0 );
}
