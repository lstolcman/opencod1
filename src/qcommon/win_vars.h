/*
 * qcommon/win_vars.h -- Q3's WinVars_t g_wv, one object at 0x016C35E0
 * (defined in win32/win_platform.c).
 *
 * Retail takes its address (GetVersionEx( &g_wv.osversion ) in Sys_Init), so
 * VC7 keeps stores to one field ordered against unrelated pointer loads; the
 * old per-field names are kept as macros onto the members.  Basic types only,
 * so the win32 units that do not include qcommon.h can use it as well.
 */
#ifndef __WIN_VARS_H__
#define __WIN_VARS_H__

typedef struct {
	void           *reflib_library;     /* +0x00 0x016C35E0 HINSTANCE */
	int             reflib_active;      /* +0x04 0x016C35E4 */
	void           *hWnd;               /* +0x08 0x016C35E8 HWND */
	void           *hInstance;          /* +0x0C 0x016C35EC HINSTANCE */
	int             activeApp;          /* +0x10 0x016C35F0 */
	int             isMinimized;        /* +0x14 0x016C35F4 */
	struct {                            /* OSVERSIONINFOA */
		unsigned long   dwOSVersionInfoSize;
		unsigned long   dwMajorVersion;
		unsigned long   dwMinorVersion;
		unsigned long   dwBuildNumber;
		unsigned long   dwPlatformId;
		char            szCSDVersion[128];
	}               osversion;          /* +0x18 0x016C35F8 */
	unsigned int    sysMsgTime;         /* +0xAC 0x016C368C */
} WinVars_t;

extern WinVars_t g_wv;   /* 0x016C35E0 */

#define g_wv_hWnd           g_wv.hWnd
#define g_wv_hInstance      g_wv.hInstance
#define g_wv_activeApp      g_wv.activeApp
#define g_wv_isMinimized    g_wv.isMinimized
#define g_wv_sysMsgTime     g_wv.sysMsgTime

extern int in_appactive;   /* 0x016BDD98, win32/win_input.c */
extern int networkingEnabled;   /* 0x014073C4, win32/win_net.c */
extern void *g_splashWnd;   /* 0x008E5268, the splash window (HWND), win32/win_syscon.c */

/* Q3's WinMouseVars_t, one object at 0x008E251C (win32/win_input.c). */
typedef struct {
	int             oldButtonState;     /* +0x00 0x008E251C */
	int             mouseActive;        /* +0x04 0x008E2520 qboolean */
	int             mouseInitialized;   /* +0x08 0x008E2524 qboolean */
} WinMouseVars_t;
extern WinMouseVars_t s_wmv;   /* 0x008E251C */
#define s_wmv_oldButtonState    s_wmv.oldButtonState
#define s_wmv_mouseActive       s_wmv.mouseActive
#define s_wmv_mouseInitialized  s_wmv.mouseInitialized

#endif /* __WIN_VARS_H__ */
