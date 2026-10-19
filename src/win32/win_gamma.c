/*
 * @fidelity: verified
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"
#include "../renderer/tr_local.h"   /* renderer cvars, glState, backEndStorage (RTCW includes it here too) */
#include "win_wgl.h"

#define RCVAR( n )  ( (cvar_t *)( n ) )

#define GAMMA_CHANNELS      3
#define GAMMA_ENTRIES       256

#define GAMMA_PRINT_WARNING 2

unsigned short s_oldHardwareGamma[GAMMA_CHANNELS][GAMMA_ENTRIES];

/* ---- WG_CheckHardwareGamma  0x00515C30 ----  VERIFIED */
void WG_CheckHardwareGamma( void ) {
	HDC		hDC;

	glConfig_deviceSupportsGamma = qfalse;

	if ( !RCVAR( r_ignorehwgamma )->integer ) {
		hDC = GetDC( GetDesktopWindow() );
		glConfig_deviceSupportsGamma = GetDeviceGammaRamp( hDC, s_oldHardwareGamma );
		ReleaseDC( GetDesktopWindow(), hDC );

		if ( glConfig_deviceSupportsGamma ) {
			if ( ( HIBYTE( s_oldHardwareGamma[0][255] ) <= HIBYTE( s_oldHardwareGamma[0][0] ) ) ||
				 ( HIBYTE( s_oldHardwareGamma[1][255] ) <= HIBYTE( s_oldHardwareGamma[1][0] ) ) ||
				 ( HIBYTE( s_oldHardwareGamma[2][255] ) <= HIBYTE( s_oldHardwareGamma[2][0] ) ) ) {
				glConfig_deviceSupportsGamma = qfalse;
				ri.Printf( GAMMA_PRINT_WARNING,
					"WARNING: device has broken gamma support, generated gamma.dat\n" );
			}

			if ( HIBYTE( s_oldHardwareGamma[0][181] ) == 255 ) {
				int g;

				ri.Printf( GAMMA_PRINT_WARNING,
					"WARNING: suspicious gamma tables, using linear ramp for restoration\n" );
				/* Stops at 254: retail's `cmp eax, 0FFh / jl` leaves entry 255 alone. */
				for ( g = 0; g < 255; g++ ) {
					s_oldHardwareGamma[0][g] = (unsigned short)( g << 8 );
					s_oldHardwareGamma[1][g] = (unsigned short)( g << 8 );
					s_oldHardwareGamma[2][g] = (unsigned short)( g << 8 );
				}
			}
		}
	}
}

/* ---- WG_RestoreGamma  0x005160B0 ----  VERIFIED */
void WG_RestoreGamma( void ) {
	HDC		hDC;

	if ( !glConfig_deviceSupportsGamma ) {
		return;
	}

	hDC = GetDC( GetDesktopWindow() );
	SetDeviceGammaRamp( hDC, s_oldHardwareGamma );
	ReleaseDC( GetDesktopWindow(), hDC );
}

/* 0x019BFFE4. Retail calls it `hdc`; win_glimp.c owns the storage under the RTCW spelling. */


/* ---- GLimp_SetGamma  0x00515D20 ----  VERIFIED */
void GLimp_SetGamma( unsigned char *red, unsigned char *green, unsigned char *blue ) {
	unsigned short	table[GAMMA_CHANNELS][GAMMA_ENTRIES];
	OSVERSIONINFO	vinfo;
	int				i, j;

	if ( !glConfig_deviceSupportsGamma
		|| RCVAR( r_ignorehwgamma )->integer
		|| !glw_state.hDC ) {
		return;
	}

	for ( i = 0; i < GAMMA_ENTRIES; i++ ) {
		table[0][i] = (unsigned short)( ( red[i]   << 8 ) | red[i]   );
		table[1][i] = (unsigned short)( ( green[i] << 8 ) | green[i] );
		table[2][i] = (unsigned short)( ( blue[i]  << 8 ) | blue[i]  );
	}

	vinfo.dwOSVersionInfoSize = sizeof( vinfo );
	GetVersionEx( &vinfo );

	if ( vinfo.dwMajorVersion == 5 && vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
		Com_DPrintf( "performing W2K gamma clamp.\n" );
		for ( j = 0; j < GAMMA_CHANNELS; j++ ) {
			for ( i = 0; i < 128; i++ ) {
				if ( table[j][i] > ( ( 128 + i ) << 8 ) ) {
					table[j][i] = (unsigned short)( ( 128 + i ) << 8 );
				}
			}
			if ( table[j][127] > ( 254 << 8 ) ) {
				table[j][127] = (unsigned short)( 254 << 8 );
			}
		}
	} else {
		Com_DPrintf( "skipping W2K gamma clamp.\n" );
	}

	for ( j = 0; j < GAMMA_CHANNELS; j++ ) {
		for ( i = 1; i < GAMMA_ENTRIES; i++ ) {
			if ( table[j][i] < table[j][i - 1] ) {
				table[j][i] = table[j][i - 1];
			}
		}
	}

	if ( !SetDeviceGammaRamp( glw_state.hDC, table ) ) {
		Com_Printf( "SetDeviceGammaRamp failed.\n" );
	}
}
