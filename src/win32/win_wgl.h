#ifndef WIN_WGL_H
#define WIN_WGL_H

/* win_glimp.c's window/context state (Q3 glwstate_t without
   allowdisplaydepthchange); log_fp, at +0x24, is still the separate Stream. */
typedef struct
{
	WNDPROC		wndproc;            /* 0x019BFFE0 */
	HDC			hDC;
	HGLRC		hGLRC;
	HMODULE		hinstOpenGL;        /* set by QCL_Init */
	qboolean	pixelFormatSet;
	int			desktopBitsPixel;
	int			desktopWidth, desktopHeight;
	qboolean	cdsFullscreen;      /* 0x019C0000 */
} glwstate_t;

extern glwstate_t glw_state;


#ifdef QGL_OWNER
#define QWGLDECL
#else
#define QWGLDECL extern
#endif

QWGLDECL BOOL  ( WINAPI * qwglCopyContext )( HGLRC, HGLRC, UINT );              /* 0x016C3DF8 */
QWGLDECL HGLRC ( WINAPI * qwglCreateContext )( HDC );                           /* 0x016C49A4 */
QWGLDECL HGLRC ( WINAPI * qwglCreateLayerContext )( HDC, int );                 /* 0x016C3EAC */
QWGLDECL BOOL  ( WINAPI * qwglDeleteContext )( HGLRC );                         /* 0x016C4648 */
QWGLDECL HGLRC ( WINAPI * qwglGetCurrentContext )( VOID );                      /* 0x016C3F6C */
QWGLDECL HDC   ( WINAPI * qwglGetCurrentDC )( VOID );                           /* 0x016C44F0 */
QWGLDECL PROC  ( WINAPI * qwglGetProcAddress )( LPCSTR );                       /* 0x016C40E0 */
QWGLDECL BOOL  ( WINAPI * qwglMakeCurrent )( HDC, HGLRC );                      /* 0x016C4A4C */
QWGLDECL BOOL  ( WINAPI * qwglShareLists )( HGLRC, HGLRC );                     /* 0x016C3D28 */
QWGLDECL BOOL  ( WINAPI * qwglUseFontBitmaps )( HDC, DWORD, DWORD, DWORD );     /* 0x016C3B9C */
QWGLDECL BOOL  ( WINAPI * qwglUseFontOutlines )( HDC, DWORD, DWORD, DWORD,
					FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT );                   /* 0x016C3C90 */
QWGLDECL BOOL  ( WINAPI * qwglDescribeLayerPlane )( HDC, int, int, UINT,
					LPLAYERPLANEDESCRIPTOR );                                   /* 0x016C45EC */
QWGLDECL int   ( WINAPI * qwglSetLayerPaletteEntries )( HDC, int, int, int,
					CONST COLORREF * );                                         /* 0x016C4778 */
QWGLDECL int   ( WINAPI * qwglGetLayerPaletteEntries )( HDC, int, int, int,
					COLORREF * );                                               /* 0x016C4564 */
QWGLDECL BOOL  ( WINAPI * qwglRealizeLayerPalette )( HDC, int, BOOL );          /* 0x016C4798 */
QWGLDECL BOOL  ( WINAPI * qwglSwapLayerBuffers )( HDC, UINT );                  /* 0x016C3D24 */

QWGLDECL void *( WINAPI * qwglAllocateMemoryNV )( int, float, float, float );   /* 0x016C457C */
QWGLDECL void  ( WINAPI * qwglFreeMemoryNV )( void * );                         /* 0x016C4500 */
QWGLDECL BOOL  ( WINAPI * qwglSwapIntervalEXT )( int );                         /* 0x016C47E0 */

/* Driver slots for the entries above (see the dll block in renderer/qgl.h). */
#if defined( QGL_OWNER ) || defined( QGL_DRIVER_SLOTS )
QWGLDECL BOOL ( WINAPI * dllwglCopyContext )( HGLRC, HGLRC, UINT );                                  /* 0x016C4410 */
QWGLDECL HGLRC ( WINAPI * dllwglCreateContext )( HDC );                                              /* 0x016C3C40 */
QWGLDECL HGLRC ( WINAPI * dllwglCreateLayerContext )( HDC, int );                                    /* 0x016C4274 */
QWGLDECL BOOL ( WINAPI * dllwglDeleteContext )( HGLRC );                                             /* 0x016C3CE4 */
QWGLDECL HGLRC ( WINAPI * dllwglGetCurrentContext )( VOID );                                         /* 0x016C47B8 */
QWGLDECL HDC ( WINAPI * dllwglGetCurrentDC )( VOID );                                                /* 0x016C3E4C */
QWGLDECL PROC ( WINAPI * dllwglGetProcAddress )( LPCSTR );                                           /* 0x016C4144 */
QWGLDECL BOOL ( WINAPI * dllwglMakeCurrent )( HDC, HGLRC );                                          /* 0x016C4178 */
QWGLDECL BOOL ( WINAPI * dllwglShareLists )( HGLRC, HGLRC );                                         /* 0x016C3F94 */
QWGLDECL BOOL ( WINAPI * dllwglUseFontBitmaps )( HDC, DWORD, DWORD, DWORD );                         /* 0x016C441C */
QWGLDECL BOOL ( WINAPI * dllwglUseFontOutlines )( HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT ); /* 0x016C3BAC */
QWGLDECL BOOL ( WINAPI * dllwglDescribeLayerPlane )( HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR );  /* 0x016C3E60 */
QWGLDECL int ( WINAPI * dllwglSetLayerPaletteEntries )( HDC, int, int, int, CONST COLORREF * );      /* 0x016C4488 */
QWGLDECL int ( WINAPI * dllwglGetLayerPaletteEntries )( HDC, int, int, int, COLORREF * );            /* 0x016C486C */
QWGLDECL BOOL ( WINAPI * dllwglRealizeLayerPalette )( HDC, int, BOOL );                              /* 0x016C3B78 */
QWGLDECL BOOL ( WINAPI * dllwglSwapLayerBuffers )( HDC, UINT );                                      /* 0x016C3E44 */
QWGLDECL void * ( WINAPI * dllwglAllocateMemoryNV )( int, float, float, float );                     /* 0x016C3F58 */
QWGLDECL void ( WINAPI * dllwglFreeMemoryNV )( void * );                                             /* 0x016C4394 */
QWGLDECL BOOL ( WINAPI * dllwglSwapIntervalEXT )( int );                                             /* 0x016C4300 */
QWGLDECL const char *( WINAPI * dllwglGetExtensionsStringEXT )( void );                              /* 0x016C43F0 */
#endif /* QGL_OWNER || QGL_DRIVER_SLOTS */

/* Not in qgl_entries.h. GLimp_Init fetches this one directly through qwglGetProcAddress, stores the result
   in both this pointer and its driver slot dllwglGetExtensionsStringEXT (0x0050ACE3), and calls it once.
   win_qgl.c handles the pair through QGL_EXTRA_ENTRIES, with a gl.log wrapper (logwglGetExtensionsStringEXT,
   0x004BFE20) and a GetLastError wrapper (qwglCheckedGetExtensionsStringEXT, 0x004C67C0).  Neither wrapper
   can ever run: QCL_Init NULLs the pointer before it arms logging and error checking, and GLimp_Init calls
   the fetched value in a register.  A later re-arm from GLimp_EndFrame does point it at a wrapper until the
   disarm copies the driver slot back, but nothing ever calls through the pointer. */
QWGLDECL const char *( WINAPI * qwglGetExtensionsStringEXT )( void );           /* 0x016C45A0 */

#endif
