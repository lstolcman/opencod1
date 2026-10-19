// cg_info.c -- display information while data is being loading
// the register traps call CG_DrawInformation, so it guards against recursion

#include <string.h>
#include <stdlib.h>

#include "cg_local.h"

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	strncpy( cg.infoScreenText, s, sizeof( cg.infoScreenText ) - 1 );
	cg.infoScreenText[sizeof( cg.infoScreenText ) - 1] = 0;

	if ( s && s[0] ) {
		CG_Printf( va( "LOADING... %s\n", s ) );   //----(SA)	added so you can see from the console what's going on
	}

	trap_UpdateScreen();
}

int cg_drawingInformation;         // reject recursive calls

/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading

If localized is set, draw the "waiting for server" screen with the
map and gametype names from cl_serverloadmap / cl_serverloadgametype
====================
*/
void CG_DrawInformation( int localized ) {
	const char *s;
	const char *info;
	char buf[MAX_STRING_CHARS];
	char expected[64];
	vec4_t color;
	qhandle_t levelshot;
	const char *dots;
	int hunkUsed, hunkExpected;
	float frac;
	float w, x;

	if ( ( cg.snap && !localized ) || cg_drawingInformation ) {
		return;     // we are in the world, no need to draw information
	}
	cg_drawingInformation = 1;

	if ( !localized ) {
		if ( cl_serverloadmap.string[0] ) {
			trap_Cvar_Set( "cl_serverloadmap", "" );
		}
		if ( cl_serverloadgametype.string[0] ) {
			trap_Cvar_Set( "cl_serverloadgametype", "" );
		}
		if ( cl_serverloadwaiting.integer ) {
			trap_Cvar_Set( "cl_serverloadwaiting", "0" );
		}
	} else {
		if ( !cl_serverloadwaiting.integer ) {
			trap_Cvar_Set( "ui_scriptMenuAllowResponse", "0" );
			CG_CloseScriptMenu();
			CG_CloseScriptMenu();
			trap_Cvar_Set( "ui_scriptMenuAllowResponse", "1" );
			trap_UI_CloseAllMenus();
			trap_Cvar_Set( "cl_serverloadwaiting", "1" );
		}
	}

	//
	// draw the dialog background
	//
	info = cgs.gameState.stringData + cgs.gameState.stringOffsets[0];
	if ( localized ) {
		trap_Cvar_VariableStringBuffer( "cl_serverloadmap", buf, sizeof( buf ) );
		s = buf;
	} else {
		s = Info_ValueForKey( info, "mapname" );
	}

	levelshot = 0;
	if ( s && s[0] ) {
		levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", s ), 2 );
	}
	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap", 2 );
	}

	if ( localized ) {
		color[0] = 0.75f;
		color[1] = 0.75f;
		color[2] = 0.75f;
		color[3] = 1.0f;
		trap_R_SetColor( color );
	} else {
		trap_R_SetColor( NULL );
	}

	CG_DrawPic( 0, 0, 640, 480, levelshot );

	if ( localized ) {
		color[0] = 0.75f;
		color[1] = 0.75f;
		color[2] = 0.75f;
		color[3] = 1.0f;

		s = CG_SafeTranslateString( trap_UI_GetGameTypeDisplayName( cl_serverloadgametype.string ) );
		w = trap_R_Text_Width( s, 0, 0.5f, 0 );
		x = ( 640 - w ) * 0.5f;
		trap_R_Text_Paint( x, 55.0f, 0, 0.5f, color, s, 0, 0, 3 );

		// the map name is not translated
		s = trap_UI_GetMapDisplayName( cl_serverloadmap.string );
		w = trap_R_Text_Width( s, 0, 0.5f, 0 );
		x = ( 640 - w ) * 0.5f;
		trap_R_Text_Paint( x, 85.0f, 0, 0.5f, color, s, 0, 0, 3 );

		switch ( ( trap_Milliseconds() / 750 ) & 3 ) {
		case 1:
			dots = ".";
			break;
		case 2:
			dots = "..";
			break;
		case 3:
			dots = "...";
			break;
		default:
			dots = "";
			break;
		}

		s = CG_SafeTranslateString( "CGAME_WAITINGFORSERVERLOAD" );
		w = trap_R_Text_Width( s, 0, 0.5f, 0 );
		s = va( "%s%s", s, dots );
		trap_R_Text_Paint( ( 640 - w ) * 0.5f, 435.0f, 0, 0.5f, color, s, 0, 0, 3 );
	} else {
		trap_Cvar_VariableStringBuffer( "com_expectedhunkusage", expected, sizeof( expected ) );
		hunkExpected = atoi( expected );

		// show the loading progress
		color[0] = 0.8f;
		color[1] = 0.8f;
		color[2] = 0.8f;
		color[3] = 0.8f;

		if ( hunkExpected > 0 ) {
			hunkUsed = trap_hunkUsed();
			// keep: matches original codegen
			frac = (float)( (double)hunkUsed / hunkExpected );
			if ( frac > 1.0f ) {
				frac = 1.0f;
			}
			CG_HorizontalPercentBar( 200, 468, 240, 10, frac );
		}
	}

	trap_UpdateScreen();

	cg_drawingInformation--;
}
