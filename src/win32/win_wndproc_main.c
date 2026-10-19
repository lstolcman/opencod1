/*
 * @fidelity: likely
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../qcommon/qcommon.h"
#include "../qcommon/win_vars.h"
/* ---- module-owned globals ---- */
int s_mswheelRollMsg;
cvar_t *vid_xpos;   /* 0x016C3694 */
cvar_t *vid_ypos;   /* 0x016C3690 */


extern byte MapKey( UINT uCode, int lParam );
extern int  VID_AppActivate( int active, int minimized );
extern void WIN_DisableAltTab( void );
extern void WIN_EnableAltTab( void );

/* win32/win_input.c -- one cdecl stack argument (0x004617D8). */
extern void IN_MouseEvent( int mstate );

extern void Sys_QueEvent( int time, int type, int value, int value2,
                          int ptrLength, void *ptr );

/* miles/snd_miles.c, 0x00450260 -- retail hands the new HWND to the sound layer from WM_CREATE. */
extern void MSS_SetDirectSoundHWND_m( void *hWnd );

extern cvar_t *r_fullscreen;   /* renderer/tr_init.c (RTCW win_wndproc.c registers it here too) */

/* MSWHEEL_ROLLMSG, registered in WM_CREATE. 0x008E58C0. g_wv and in_appactive come from win_vars.h, cls_state from cl_static.h. */

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL   0x020A
#endif
#ifndef MK_MBUTTON
#define MK_MBUTTON      0x0010
#endif

#define K_MWHEELDOWN    205
#define K_MWHEELUP      206
#define SE_KEY          1
#define SE_CHAR         2
#define CA_LOADING      4

/* ---- MainWndProc  0x00466BE0 ---- */
LRESULT CALLBACK MainWndProc( HWND hWnd, UINT Msg, WPARAM wParam,
                              LPARAM lParam )
{
    SetThreadExecutionState( 2 );

    /* Retail 0x466BF1 compares Msg with the registered id directly, with no
       zero test, so WM_NULL counts as a wheel roll until WM_CREATE registers it. */
    if ( Msg == (UINT) s_mswheelRollMsg ) {
        if ( (int) wParam > 0 ) {
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELUP, 1, 0, 0 );
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELUP, 0, 0, 0 );
        } else {
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELDOWN, 1, 0, 0 );
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELDOWN, 0, 0, 0 );
        }
        return DefWindowProc( hWnd, Msg, wParam, lParam );
    }

    switch ( Msg ) {

    case WM_MOUSEWHEEL:
        if ( (short) HIWORD( wParam ) > 0 ) {
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELUP, 1, 0, 0 );
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELUP, 0, 0, 0 );
        } else {
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELDOWN, 1, 0, 0 );
            Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, K_MWHEELDOWN, 0, 0, 0 );
        }
        break;

    case WM_CREATE:
        g_wv_hWnd = (void *) hWnd;
        MSS_SetDirectSoundHWND_m( (void *) hWnd );
        vid_xpos     = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
        vid_ypos     = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );
        r_fullscreen = Cvar_Get( "r_fullscreen", "1",
                                 CVAR_ARCHIVE | CVAR_LATCH );
        s_mswheelRollMsg = (int) RegisterWindowMessage( "MSWHEEL_ROLLMSG" );
        if ( r_fullscreen->integer ) {
            WIN_DisableAltTab();
        } else {
            WIN_EnableAltTab();
        }
        break;

    case WM_DESTROY:
        g_wv_hWnd = NULL;
        /* Q3 shape: retail dereferences r_fullscreen here and in WM_MOVE with no null test. */
        if ( r_fullscreen->integer ) {
            WIN_EnableAltTab();
        }
        break;

    case WM_CLOSE:
        /* Retail 0x466E11 queues the pooled literal "quit" (0x562680) with no
           trailing newline, then falls through to DefWindowProc. */
        Cbuf_AddText( "quit" );
        break;

    case WM_ACTIVATE:
        {
            int fActive, fMinimized;

            fActive = LOWORD( wParam );
            fMinimized = (BOOL) HIWORD( wParam );

            VID_AppActivate( fActive != WA_INACTIVE, fMinimized );
        }
        break;

    case WM_MOVE:
        {
            int  xPos, yPos;
            RECT r;
            LONG style;

            if ( !r_fullscreen->integer ) {
                xPos = (short) LOWORD( lParam );
                yPos = (short) HIWORD( lParam );

                r.left   = 0;
                r.top    = 0;
                r.right  = 1;
                r.bottom = 1;

                style = GetWindowLong( hWnd, GWL_STYLE );
                AdjustWindowRect( &r, style, FALSE );

                Cvar_SetValue( "vid_xpos", (float) ( xPos + r.left ) );
                Cvar_SetValue( "vid_ypos", (float) ( yPos + r.top ) );
                vid_xpos->modified = qfalse;
                vid_ypos->modified = qfalse;
                if ( g_wv_activeApp ) {
                    in_appactive = 1;
                }
            }
        }
        break;

    case WM_POWERBROADCAST:
        if ( wParam == 0 || wParam == 1 ) {
            return 0x424D5144;
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
        {
            int temp;

            temp = 0;

            if ( wParam & MK_LBUTTON ) {
                temp |= 1;
            }
            if ( wParam & MK_RBUTTON ) {
                temp |= 2;
            }
            if ( wParam & MK_MBUTTON ) {
                temp |= 4;
            }

            IN_MouseEvent( temp );
        }
        break;

    case WM_SYSCOMMAND:
        if ( wParam == SC_SCREENSAVE ) {
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if ( wParam == 13 ) {
            if ( cls_state != CA_LOADING && r_fullscreen ) {
                Cvar_SetValue( "r_fullscreen", !r_fullscreen->integer );
                Cbuf_AddText( "vid_restart\n" );
            }
            return 0;
        }
        /* fall through */
    case WM_KEYDOWN:
        {
            byte key = MapKey( (UINT) wParam, (int) lParam );
            if ( key ) {
                Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, key, 1, 0, 0 );
            }
        }
        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        {
            byte key = MapKey( (UINT) wParam, (int) lParam );
            if ( key ) {
                Sys_QueEvent( g_wv_sysMsgTime, SE_KEY, key, 0, 0, 0 );
            }
        }
        break;

    case WM_CHAR:
        Sys_QueEvent( g_wv_sysMsgTime, SE_CHAR, (int) wParam, 0, 0, 0 );
        break;

    default:
        break;
    }

    return DefWindowProc( hWnd, Msg, wParam, lParam );
}
