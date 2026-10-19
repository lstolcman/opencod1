// cg_newDraw.c -- the ownerdraw handlers the menu-driven hud calls into
// every handler takes the item's font; the compass, stance, cursor hint,
// weapon name and health bar draws are new

#define _CRT_SECURE_NO_WARNINGS

#include <math.h>
#include <string.h>

#include "cg_local.h"

// ui_shared.h collides with cg_public.h, so repeat the one type needed from it
#ifndef __UI_SHARED_H__
typedef struct {
	float x;
	float y;
	float w;
	float h;
} rectDef_t;
#endif

// ui_shared.c
struct menuDef_s;
struct menuDef_s *Menus_FindByName( const char *p );
void        Menus_Open( struct menuDef_s *menu );
void        Controls_GetConfig( void );
qboolean    GetCommandHasBinding( const char *name );
int         GetKeyBindingLocalizedString( const char *name, char **out );

int         CG_DrawField( int x, int y, int width, int value, int charWidth, int charHeight,
						  qboolean dodrawpic, qboolean leftAlign );      // NERVE - SMF
void        CG_ScrollScoreboardDown( void );

// bg_weapon.c
int         BG_GetTotalAmmoReserve( const playerState_t *ps, int weapon );

#ifndef PITCH
#define PITCH   0
#define YAW     1
#define ROLL    2
#endif

// pmove_t->pm_flags, from bg_local.h
#ifndef PMF_PRONE
#define PMF_PRONE               0x00000001
#define PMF_DUCKED              0x00000002
#define PMF_PRONE_BLOCKED       0x00008000
#endif
#ifndef PMF_INWORLD
#define PMF_INWORLD             0x00040000  // name inferred
#endif

// keycodes.h
#ifndef K_PGDN
#define K_PGDN              163
#define K_PGUP              164
#define K_KP_PGUP           184
#define K_KP_PGDN           190
#define K_MWHEELDOWN        205
#define K_MWHEELUP          206
#endif

#ifndef STAT_HEALTH
#define STAT_HEALTH         0
#endif
#define STAT_MAX_HEALTH     2

#define ANGLE2SHORT( x )    ( (int)( (x) * ( 65536.0f / 360.0f ) ) & 65535 )
#define SHORT2ANGLE( x )    ( (float)( x ) * ( 360.0f / 65536.0f ) )

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

// team_t from g_local.h
#ifndef TEAM_AXIS
#define TEAM_FREE       0
#define TEAM_AXIS       1
#define TEAM_ALLIES     2
#endif

#define TEAM_MAXOVERLAY 8

#define SCORE_NOT_PRESENT   -9999

// menudef.h ownerdraw ids (the names of 81 and up are inferred)
#define CG_PLAYER_HEALTH                4
#define CG_PLAYER_AMMO_VALUE            5
#define CG_PLAYER_AMMO_BACKDROP         6
#define CG_SELECTEDPLAYER_NAME          8
#define CG_SELECTEDPLAYER_LOCATION      9
#define CG_PLAYER_STANCE                20
#define CG_PLAYER_SCORE                 21
#define CG_BLUE_SCORE                   27
#define CG_RED_SCORE                    28
#define CG_PLAYER_LOCATION              33
#define CG_TEAM_COLOR                   34
#define CG_GAME_TYPE                    39
#define CG_SELECTEDPLAYER_HEALTH        41
#define CG_AREA_SYSTEMCHAT              46
#define CG_AREA_TEAMCHAT                47
#define CG_AREA_CHAT                    48
#define CG_KILLER                       50
#define CG_VOICE_NAME                   63
#define CG_1STPLACE                     67
#define CG_2NDPLACE                     68
#define CG_PLAYER_AMMOCLIP_VALUE        70
#define CG_CURSORHINT                   72
#define CG_AREA_WEAPON                  78
#define CG_AREA_HOLDABLE                79
#define CG_PLAYER_WEAPON_NAME           81
#define CG_PLAYER_WEAPON_NAME_BACK      82
#define CG_PLAYER_WEAPON_MODE_ICON      83
#define CG_PLAYER_COMPASS               84
#define CG_PLAYER_COMPASS_BACK          85
#define CG_PLAYER_COMPASS_POINTERS      86
#define CG_PLAYER_COMPASS_FRIENDLIES    88
#define CG_PLAYER_BAR_HEALTH            89
#define CG_PLAYER_BAR_HEALTH_TITLE      90
#define CG_MENU_SHADER                  91

// menudef.h ownerdrawflags
#define CG_SHOW_HEALTHCRITICAL          0x00000080
#define CG_SHOW_HEALTHOK                0x00004000
#define CG_SHOW_TEAMINFO                0x00008000
#define CG_SHOW_NOTEAMINFO              0x00010000
#define CG_SHOW_2DONLY                  0x10000000

#define HINT_HEALTH             7


// q3print_t
#define SYSTEM_PRINT        0

// configstring bases
#define CS_SHADERS          1500
#define CS_HINTSTRINGS      1212
#define CS_LOCATIONS        44

// keep: declaration order matches the original data layout
int         sortedTeamPlayers[TEAM_MAXOVERLAY];
static char teamChat1[256];
int         numSortedTeamPlayers;
static char systemChat[256];
static char teamChat2[256];

// CG_DrawPlayerStance state; stanceUnused is never read
static int  stanceUnused = -1;
static int  stanceHintTime = -1;
static int  stanceLast = -1;
static int  proneBlockedTime = -1;

// CG_DrawPlayerBarHealth's trailing damage bar; barHoldTime is never written
static int      barHoldTime = 1;
static int      barLagHoldTime = 1;
static int      barLastClient = -1;
static float    barLagFrac;

// CG_GetTranslatedLocationString's return buffer
static char locationString[MAX_STRING_CHARS];

/*
================
CG_InitTeamChat
================
*/
void CG_InitTeamChat( void ) {
}

/*
================
CG_SetPrintString
================
*/
void CG_SetPrintString( int type, const char *p ) {
	if ( type == SYSTEM_PRINT ) {
		strcpy( systemChat, p );
	} else {
		strcpy( teamChat2, teamChat1 );
		strcpy( teamChat1, p );
	}
}

/*
================
CG_CheckOrderPending
================
*/
void CG_CheckOrderPending( void ) {
}

/*
================
CG_SetSelectedPlayerName
================
*/
static void CG_SetSelectedPlayerName( void ) {
	if ( cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers ) {
		trap_Cvar_Set( "cg_selectedPlayer", va( "%d", sortedTeamPlayers[cg_currentSelectedPlayer.integer] ) );
	}
}

/*
================
CG_GetSelectedPlayer
================
*/
int CG_GetSelectedPlayer( void ) {
	if ( cg_currentSelectedPlayer.integer < 0 || cg_currentSelectedPlayer.integer >= numSortedTeamPlayers ) {
		cg_currentSelectedPlayer.integer = 0;
	}
	return cg_currentSelectedPlayer.integer;
}

/*
================
CG_SelectNextPlayer
================
*/
void CG_SelectNextPlayer( void ) {
	if ( cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers ) {
		cg_currentSelectedPlayer.integer++;
	} else {
		cg_currentSelectedPlayer.integer = 0;
	}
	CG_SetSelectedPlayerName();
}

/*
================
CG_SelectPrevPlayer
================
*/
void CG_SelectPrevPlayer( void ) {
	if ( cg_currentSelectedPlayer.integer > 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers ) {
		cg_currentSelectedPlayer.integer--;
	} else {
		cg_currentSelectedPlayer.integer = numSortedTeamPlayers;
	}
	CG_SetSelectedPlayerName();
}

/*
================
CG_DrawPlayerAmmoBackdrop

The plate behind the ammo counter, drawn while a weapon is held
================
*/
static void CG_DrawPlayerAmmoBackdrop( rectDef_t *rect, vec4_t color, qhandle_t shader ) {
	if ( cg.predictedPlayerState.weapon ) {
		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	}
}

/*
================
CG_DrawPlayerWeaponName

The selected weapon's name, right-aligned 28 units in from the right edge
================
*/
static void CG_DrawPlayerWeaponName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	float       *fade;
	vec4_t      drawColor;
	weaponInfo_t *weapDef;
	cgWeaponInfo_t *cgWeap;
	const char  *text;
	int         width;
	int         weap;

	fade = CG_FadeColor( cg.weaponSelectTime, 1800 );
	if ( !fade ) {
		return;
	}

	drawColor[0] = color[0];
	drawColor[1] = color[1];
	drawColor[2] = color[2];
	drawColor[3] = fade[3];

	weap = cg_weaponSelect.integer;
	if ( weap >= 0 && weap < bg_numWeapons
		 && ( cg.predictedPlayerState.weapons[weap >> 5] & ( 1 << ( weap & 31 ) ) ) ) {
		weapDef = bg_weaponInfo[weap];
	} else {
		weapDef = cg.weaponInfo;
	}
	if ( !weapDef->weapIndex ) {
		return;
	}

	cgWeap = &cg_weapons[weapDef->weapIndex];
	if ( weapDef->modeName[0] ) {
		text = va( "%s / %s", cgWeap->displayName, cgWeap->modeName );
	} else {
		text = va( "%s", cgWeap->displayName );
	}

	width = trap_R_Text_Width( text, font, scale, 0 );
	trap_R_Text_Paint( rect->x + rect->w - width - 28.0f, rect->y, font, scale,
					   drawColor, text, 0, 0, textStyle );
}

/*
================
CG_DrawPlayerWeaponNameBack

The plate behind CG_DrawPlayerWeaponName: same right edge, sized to the text
plus 36.
================
*/
static void CG_DrawPlayerWeaponNameBack( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	float       *fade;
	vec4_t      drawColor;
	weaponInfo_t *weapDef;
	cgWeaponInfo_t *cgWeap;
	const char  *text;
	float       width, x;
	int         weap;

	fade = CG_FadeColor( cg.weaponSelectTime, 1800 );
	if ( !fade ) {
		return;
	}

	drawColor[0] = color[0];
	drawColor[1] = color[1];
	drawColor[2] = color[2];
	drawColor[3] = fade[3];

	weap = cg_weaponSelect.integer;
	if ( weap >= 0 && weap < bg_numWeapons
		 && ( cg.predictedPlayerState.weapons[weap >> 5] & ( 1 << ( weap & 31 ) ) ) ) {
		weapDef = bg_weaponInfo[weap];
	} else {
		weapDef = cg.weaponInfo;
	}
	if ( !weapDef->weapIndex ) {
		return;
	}

	cgWeap = &cg_weapons[weapDef->weapIndex];
	if ( weapDef->modeName[0] ) {
		text = va( "%s / %s", cgWeap->displayName, cgWeap->modeName );
	} else {
		text = va( "%s", cgWeap->displayName );
	}

	width = trap_R_Text_Width( text, font, scale, 0 ) + 36.0f;
	x = rect->x + rect->w - width;

	trap_R_SetColor( drawColor );
	CG_DrawPic( x, rect->y, width, rect->h, shader );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawPlayerWeaponModeIcon

The fire mode icon of the selected weapon
================
*/
static void CG_DrawPlayerWeaponModeIcon( rectDef_t *rect, vec4_t color ) {
	weaponInfo_t *weapDef;
	cgWeaponInfo_t *weapon;
	int         weap;

	if ( !cg.predictedPlayerState.weapon && !CG_FadeColor( cg.weaponSelectTime, 1800 ) ) {
		return;
	}

	weap = cg_weaponSelect.integer;
	if ( weap >= 0 && weap < bg_numWeapons
		 && ( cg.predictedPlayerState.weapons[weap >> 5] & ( 1 << ( weap & 31 ) ) ) ) {
		weapDef = bg_weaponInfo[weap];
	} else {
		weapDef = cg.weaponInfo;
	}
	if ( !weapDef->weapIndex ) {
		return;
	}

	weapon = &cg_weapons[weapDef->weapIndex];
	if ( !weapon->modeIcon ) {
		return;
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, weapon->modeIcon );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawPlayerStance

Three jobs in one ownerdraw: the "prone blocked" flash across the middle of
the screen, the four stance-change key hints beside the widget, and the stance
icon itself (plus the colour flash that follows a stance change).

In the command tables the row is the hint being offered (jump / stand /
crouch / prone); the first bound command in the row is the one whose key is shown.
================
*/
#define STANCE_CMD( i, j )  (char *)( ( stanceLast & PMF_PRONE ) ? proneCmds[i][j] \
								: ( stanceLast & PMF_DUCKED ) ? duckCmds[i][j] : standCmds[i][j] )

static void CG_DrawPlayerStance( rectDef_t *rect, vec4_t color, int font, float scale, int textStyle ) {
	const char  *bind[4];
	char        *keyString;
	const char  *text;
	vec4_t      drawColor;
	qhandle_t   shader;
	float       xBase, y;
	rfloat      halfWidth;
	float       textHeight;
	int         count;
	int         i, j;

	if ( !cg_hudStanceHintPrints.integer ) {
		stanceHintTime = -1;
	} else if ( stanceHintTime > cg.time
				|| stanceLast != ( cg.predictedPlayerState.pm_flags & ( PMF_PRONE | PMF_DUCKED ) ) ) {
		stanceHintTime = cg.time;
	}

	// keep: statement order matches original codegen
	xBase = ( cg_hudCompassSize.value - 1.0f ) * 112.0f + rect->x;
	stanceLast = cg.predictedPlayerState.pm_flags & ( PMF_PRONE | PMF_DUCKED );
	drawColor[0] = color[0];
	drawColor[1] = color[1];
	drawColor[2] = color[2];

	if ( cg.predictedPlayerState.pm_flags & PMF_PRONE_BLOCKED ) {
		if ( proneBlockedTime < cg.time ) {
			proneBlockedTime = cg.time + 1500;
		}
	}
	if ( proneBlockedTime > cg.time ) {
		text = CG_SafeTranslateString( "CGAME_PRONE_BLOCKED" );
		halfWidth = ( trap_R_Text_Width( text, font, scale, 0 ) * 0.5f );
		// keep: constant order and parentheses match original codegen
		drawColor[3] = (float)fabs( sin( ( ( ( ( proneBlockedTime - cg.time ) * ( 1.0f / 1500.0f ) ) * 540.0f ) * M_PI ) * ( 1.0f / 180.0f ) ) );
		// keep: y set here and the extra parentheses match original codegen
		y = 270.0f;
		trap_R_Text_Paint( ( ( ( ( ( 320.0f - halfWidth ) ) ) ) ), y, font, scale,
						   drawColor, text, 0, 0, textStyle );
	}

	if ( stanceHintTime + 3000 > cg.time ) {
		// keep: explicit NULL terminators match original codegen
		const char *standCmds[4][6] = {
			{ "+gostand", "+moveup", NULL },
			{ NULL },
			{ "gocrouch", "togglecrouch", "lowerstance", "+movedown", NULL },
			{ "goprone", "+prone", NULL }
		};
		const char *duckCmds[4][6] = {
			{ NULL },
			{ "+gostand", "raisestance", "+moveup", NULL },
			{ NULL },
			{ "goprone", "lowerstance", "toggleprone", "+prone", NULL }
		};
		const char *proneCmds[4][6] = {
			{ NULL },
			{ "+gostand", "toggleprone", NULL },
			{ "gocrouch", "togglecrouch", "raisestance", "+movedown", "+moveup", NULL },
			{ NULL }
		};
		const char *hintNames[4] = {
			"CGAME_STANCEHINT_JUMP",
			"CGAME_STANCEHINT_STAND",
			"CGAME_STANCEHINT_CROUCH",
			"CGAME_STANCEHINT_PRONE"
		};

		Controls_GetConfig();

		if ( stanceHintTime + 2000 > cg.time ) {
			drawColor[3] = 1.0f;
		} else {
			drawColor[3] = ( stanceHintTime - cg.time + 3000 ) * 0.001f;
		}

		textHeight = trap_R_Text_Height( font, scale );

		count = 0;
		for ( i = 0; i < 4; i++ ) {
			bind[i] = NULL;
			// keep: the last step reads one entry past the row, as the original does
			for ( j = 0, keyString = STANCE_CMD( i, j ); j < 6; j++, keyString = STANCE_CMD( i, j ) ) {
				if ( !keyString ) {
					break;
				}
				if ( GetCommandHasBinding( keyString ) ) {
					bind[i] = keyString;
					count++;
					break;
				}
			}
		}

		y = rect->y + rect->h * 0.5f - 1.5f;
		switch ( count ) {
		case 1:
			y = y + textHeight * 0.5f;
			break;
		case 3:
			y = y - ( textHeight * 0.5f + 1.5f );
			break;
		}

		for ( i = 0; i < 4; i++ ) {
			if ( bind[i] ) {
				GetKeyBindingLocalizedString( bind[i], &keyString );
				text = va( CG_SafeTranslateString( hintNames[i] ), keyString );
				trap_R_Text_Paint( xBase + rect->w, y, font, scale,
								   drawColor, text, 0, 0, textStyle );
				y = y + textHeight + 1.5f;
			}
		}
	}

	if ( stanceLast & PMF_PRONE ) {
		shader = cgs.media.hudStanceProne;
	} else if ( stanceLast & PMF_DUCKED ) {
		shader = cgs.media.hudStanceCrouch;
	} else {
		shader = cgs.media.hudStanceStand;
	}

	drawColor[3] = color[3];
	trap_R_SetColor( drawColor );
	trap_R_DrawStretchPic( ( cgs.screenXScale * xBase ), cgs.screenYScale * rect->y,
						   cgs.screenXScale * rect->w, cgs.screenYScale * rect->h,
						   0, 0, 1, 1, shader );

	if ( stanceHintTime + 1000 > cg.time ) {
		if ( cg_hudStanceFlash_r.value < 0 ) {
			drawColor[0] = 0;
		} else if ( cg_hudStanceFlash_r.value > 1 ) {
			drawColor[0] = 1.0f;
		} else {
			drawColor[0] = cg_hudStanceFlash_r.value;
		}
		if ( cg_hudStanceFlash_g.value < 0 ) {
			drawColor[1] = 0;
		} else if ( cg_hudStanceFlash_g.value > 1 ) {
			drawColor[1] = 1.0f;
		} else {
			drawColor[1] = cg_hudStanceFlash_g.value;
		}
		if ( cg_hudStanceFlash_b.value < 0 ) {
			drawColor[2] = 0;
		} else if ( cg_hudStanceFlash_b.value > 1 ) {
			drawColor[2] = 1.0f;
		} else {
			drawColor[2] = cg_hudStanceFlash_b.value;
		}
		drawColor[3] = FCAST( ( stanceHintTime - cg.time + 1000 ) * 0.001f ) * 0.8f;	// keep: matches original codegen

		trap_R_SetColor( drawColor );
		trap_R_DrawStretchPic( ( cgs.screenXScale * xBase ), cgs.screenYScale * rect->y,
							   cgs.screenXScale * rect->w, cgs.screenYScale * rect->h,
							   0, 0, 1, 1, cgs.media.hudStanceFlash );
	}

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawPlayerCompass

The rose scales about ( -25, 345 ) and slides up as cg_hudCompassSize grows.
================
*/
static void CG_DrawPlayerCompass( rectDef_t *rect, qhandle_t shader, vec4_t color ) {
	float x, y, w, h;

	x = ( rect->x - -25.0f ) * cg_hudCompassSize.value - 25.0f;
	y = ( rect->y - 345.0f ) * cg_hudCompassSize.value + 345.0f
		- ( cg_hudCompassSize.value - 1.0f ) * 160.0f;
	w = cg_hudCompassSize.value * rect->w;
	h = cg_hudCompassSize.value * rect->h;

	CG_UpdateCompassOrientation();

	trap_R_SetColor( color );
	CG_DrawRotatedPic( x, y, w, h, cg.compassPlayerAngle, shader );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawPlayerBarHealth

The health bar.  color is written through: the bar tints itself red as it
empties, and the trailing damage bar overwrites it outright.
================
*/
static void CG_DrawPlayerBarHealth( rectDef_t *rect, vec4_t color, qhandle_t shader ) {
	playerState_t *ps;
	float frac;
	double dfrac;
	float x, y, w, h;

	ps = &cg.snap->ps;

	// missing or negative health gives an empty bar; keep: the gotos match original codegen
	if ( !ps->stats[STAT_HEALTH] || !ps->stats[STAT_MAX_HEALTH] ) {
		goto noHealthFraction;
	}
	frac = (float)ps->stats[STAT_HEALTH] / ps->stats[STAT_MAX_HEALTH];
	if ( frac < 0 ) {
		goto noHealthFraction;
	}
	if ( frac > 1 ) {
		frac = 1.0f;
	}
	goto healthFractionReady;
noHealthFraction:
	frac = 0;
healthFractionReady:

	// keep: the round trip through a double matches original codegen
	dfrac = frac;
	if ( (float)dfrac > 0 ) {
		x = rect->x;
		y = rect->y;
		w = frac * rect->w;
		h = rect->h;
		CG_AdjustFrom640( &x, &y, &w, &h );
		if ( frac > 0.5f ) {
			color[0] = ( 1.0f - frac ) * color[0] * 2;
			color[2] = ( 1.0f - frac ) * color[2] * 2;
		} else {
			color[1] = ( frac + 0.2f ) * color[1] + 0.3f;
		}
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y, w, h, 0, 0, frac, 1.0f, shader );
	}

	// the trailing bar that drains down to the new value after damage
	if ( barLastClient != ps->clientNum ) {
		barLastClient = ps->clientNum;
		barLagFrac = frac;
		barLagHoldTime = barHoldTime;
	} else if ( frac < barLagFrac ) {
		if ( barLagHoldTime ) {
			barLagHoldTime -= cg.frametime;
			if ( barLagHoldTime < 0 ) {
				barLagHoldTime = 0;
			}
		} else {
			barLagFrac = barLagFrac - cg.frametime * 0.0012f;
			if ( barLagFrac <= frac ) {
				barLagFrac = frac;
				barLagHoldTime = barHoldTime;
			}
		}
	} else {
		barLagFrac = frac;
		barLagHoldTime = barHoldTime;
	}

	if ( barLagFrac > frac ) {
		x = frac * rect->w + rect->x;
		y = rect->y;
		w = ( barLagFrac - frac ) * rect->w;
		h = rect->h;
		CG_AdjustFrom640( &x, &y, &w, &h );
		color[0] = 1.0f;
		color[1] = 0;
		color[2] = 0;
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y, w, h, frac, 0, barLagFrac, 1.0f, shader );
	}

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawPlayerBarHealthTitle
================
*/
static void CG_DrawPlayerBarHealthTitle( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	const char *text;

	text = CG_SafeTranslateString( "CGAME_HEALTH" );
	trap_R_Text_Paint( rect->x, rect->y, font, scale, color, text, 0, 0, textStyle );
}

/*
================
CG_ServerShaderName

Copy out the name of a shader the server registered in CS_SHADERS
================
*/
qboolean CG_ServerShaderName( int index, char *buffer, int bufferSize ) {
	const char *s;

	if ( index <= 0 || index >= 128 ) {
		return qfalse;
	}

	s = CG_ConfigString( CS_SHADERS + index );
	if ( !s[0] ) {
		return qfalse;
	}
	if ( strlen( s ) >= (unsigned int)bufferSize ) {
		return qfalse;
	}

	strcpy( buffer, s );
	return qtrue;
}

/*
================
CG_ObjectiveIcon

dir is 0 for level, 1 for an objective above the player and 2 for one below;
the suffix is appended to the server-supplied shader name, and the three
cgs.media handles are the fallback.
================
*/
static qhandle_t CG_ObjectiveIcon( int dir, int shaderIndex ) {
	const char *suffix[3] = { "", "_up", "_down" };
	char name[MAX_QPATH];
	const char *in;
	char *out;

	if ( shaderIndex ) {
		if ( CG_ServerShaderName( shaderIndex, name, sizeof( name ) ) ) {
			// Com_StripExtension( name, name )
			in = name;
			out = name;
			while ( *in && *in != '.' ) {
				*out++ = *in++;
			}
			*out = 0;
			strcat( name, suffix[dir] );
			return trap_R_RegisterShaderNoMip( name, 5 );
		}
	}

	return ( &cgs.media.hudObjective )[dir];
}


// keep: improve float consistency here, as the original does
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif

/*
================
CG_DrawPlayerCompassPointers

The objective markers around the compass.  shader is unused; the icons
come from CG_ObjectiveIcon.
================
*/
static void CG_DrawPlayerCompassPointers( rectDef_t *rect, qhandle_t shader, vec4_t color ) {
	objective_t *obj;
	centity_t   *cent;
	vec4_t      drawColor;
	vec3_t      pos, dir;
	qhandle_t   icon;
	float       centerX, centerY;
	float       x, y, w, h, radius, dist, angle, t, frac;
	float       s, c;
	int         i;

	CG_UpdateCompPointerOrientation();

	// keep: FCAST matches original codegen
	centerX = FCAST( cg_hudCompassSize.value * rect->w * 0.5f ) + rect->x;
	centerY = cg_hudCompassSize.value * rect->h * 0.5f + rect->y
			  - ( cg_hudCompassSize.value - 1.0f ) * 160.0f;

	if ( !bg_clientinfo[cg.snap->ps.clientNum].infoValid ) {
		return;
	}

	for ( i = 0; i < 16; i++ ) {
		obj = &cg.snap->ps.objective[i];
		if ( obj->state != 4 ) {
			continue;
		}

		if ( obj->entNum != ENTITYNUM_NONE ) {
			cent = &cg_entities[obj->entNum];
			VectorCopy( cent->lerpOrigin, pos );
		} else {
			VectorCopy( obj->origin, pos );
		}
		VectorSubtract( pos, cg.refdef.vieworg, dir );

		angle = (float)SHORT2ANGLE( ANGLE2SHORT( vectoyaw( dir ) - cg.compassPointerAngle ) );
		// keep: FCAST matches original codegen
		dist = FCAST( (float)sqrt( dir[1] * dir[1] + dir[0] * dir[0] ) );

		VectorCopy( color, drawColor );

		if ( dist <= cg_hudCompassMaxRange.value ) {
			drawColor[3] = 1.0f;
		} else if ( dist >= cg_hudObjectiveMaxRange.value ) {
			drawColor[3] = cg_hudObjectiveMinAlpha.value;
		} else {
			t = 0;
			if ( cg_hudObjectiveMaxRange.value - cg_hudCompassMaxRange.value != 0 ) {
				t = ( dist - cg_hudCompassMaxRange.value )
					/ ( cg_hudObjectiveMaxRange.value - cg_hudCompassMaxRange.value );
			}
			drawColor[3] = 1.0f - ( 1.0f - cg_hudObjectiveMinAlpha.value ) * t;
		}

		if ( dist <= cg_hudCompassMinRange.value ) {
			radius = cg_hudCompassMinRadius.value;
		} else if ( dist >= cg_hudCompassMaxRange.value ) {
			radius = 1.0f;
		} else {
			frac = 0;
			if ( cg_hudCompassMaxRange.value - cg_hudCompassMinRange.value != 0 ) {
				frac = ( cg_hudCompassMaxRange.value - dist )
					/ ( cg_hudCompassMaxRange.value - cg_hudCompassMinRange.value );
			}
			radius = 1.0f - ( 1.0f - cg_hudCompassMinRadius.value ) * frac;
		}
		// keep: a double constant, as in the original
		radius = (float)( cg_hudCompassSize.value * radius * 43.75 );

		SinCos( angle * (float)M_PI / 180.0f, &s, &c );

		w = cg_hudCompassSize.value * 20.0f;
		h = cg_hudCompassSize.value * 20.0f;
		x = centerX - w * 0.5f - s * radius;
		y = centerY - h * 0.5f - c * radius;

		if ( dir[2] > cg_hudObjectiveMaxHeight.value ) {
			icon = CG_ObjectiveIcon( 1, obj->icon );
		} else if ( dir[2] < cg_hudObjectiveMinHeight.value ) {
			icon = CG_ObjectiveIcon( 2, obj->icon );
		} else {
			icon = CG_ObjectiveIcon( 0, obj->icon );
		}

		trap_R_SetColor( drawColor );
		CG_DrawPic( x, y, w, h, icon );
	}

	trap_R_SetColor( NULL );
}

#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/*
================
CG_DrawPlayerCompassBack

CG_DrawPlayerCompass's geometry, drawn unrotated and in screen space.
================
*/
static void CG_DrawPlayerCompassBack( rectDef_t *rect, qhandle_t shader, vec4_t color ) {
	float x, y, w, h;

	x = ( ( rect->x - -25.0f ) * cg_hudCompassSize.value - 25.0f ) * cgs.screenXScale;
	y = ( ( rect->y - 345.0f ) * cg_hudCompassSize.value + 345.0f
		  - ( cg_hudCompassSize.value - 1.0f ) * 160.0f ) * cgs.screenYScale;
	// keep: operand order matches original codegen
	w = cgs.screenXScale * ( cg_hudCompassSize.value * rect->w );
	h = cgs.screenYScale * ( cg_hudCompassSize.value * rect->h );

	trap_R_SetColor( color );
	trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1.0f, 1.0f, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_DrawCursorHints

  cg_cursorHints.integer ==
	0:	no hints
	1:	sin size pulse
	2:	one way size pulse
	3:	alpha pulse
	4+:	static image

  cg.cursorHintIcon selects both the icon and the message: 2..9 are the
  fixed hints, 10..73 a weapon pickup, 74..137 an ammo pickup, and anything
  else falls back on cg.cursorHintString or the health pickup.
==============
*/
static void CG_DrawCursorhint( rectDef_t *rect, int font, float scale, int textStyle ) {
	float       *fade;
	qhandle_t   shader;
	weaponInfo_t *weapDef;
	cgWeaponInfo_t *wi;
	const char  *s;
	const char  *text;
	char        *keyString;
	char        buffer[MAX_STRING_CHARS];
	char        lower[MAX_STRING_CHARS];
	float       widthScale, xOfs, grow, halfGrow;
	int         weapon, slot;
	float       width, height;
	char        *p;
	int         n;
	double      dh;

	widthScale = 1.0f;
	xOfs = 0;

	if ( !cg_cursorHints.integer ) {
		return;
	}

	CG_CheckForCursorHints();

	shader = cgs.media.hintShaders[cg.cursorHintIcon];
	if ( cg.cursorHintIcon >= 0 && cg.cursorHintIcon <= 1 ) {
		return;
	}
	if ( !shader ) {
		return;
	}

	fade = CG_FadeColor( cg.cursorHintTime, cg.cursorHintFade );
	if ( !fade ) {
		trap_R_SetColor( NULL );
		cg.cursorHintIcon = 0;
		return;
	}

	if ( cg_cursorHints.integer == 3 ) {
		fade[3] = ( (float)sin( cg.time * 0.0066666668f ) + 1.0f ) * fade[3] * 0.5f;
	}
	if ( cg_cursorHints.integer >= 3 ) {
		halfGrow = 0;
		grow = 0;
	} else {
		if ( cg_cursorHints.integer == 2 ) {
			grow = ( cg.cursorHintTime % 1000 ) * 0.01f;
		} else {
			grow = ( ( (float)sin( cg.time * 0.0066666668f ) + 1.0f ) * 0.5f ) * 10.0f;
		}
		halfGrow = grow * 0.5f;
	}

	if ( cg.cursorHintIcon >= HINT_WEAPON_FIRST && cg.cursorHintIcon <= HINT_WEAPON_LAST ) {
		weapon = cg.cursorHintIcon - ( HINT_WEAPON_FIRST - 1 );
		weapDef = bg_weaponInfo[weapon];
		wi = &cg_weapons[weapon];

		if ( weapDef->wideListIcon ) {
			widthScale = 2.0f;
			xOfs = rect->w * -0.5f;
		}

		if ( !BG_GetEmptySlotForWeapon( &cg.predictedPlayerState, weapon ) ) {
			Controls_GetConfig();
			GetKeyBindingLocalizedString( "+activate", &keyString );

			slot = weapDef->weaponSlot;
			if ( bg_weaponInfo[cg.predictedPlayerState.weapon]->weaponSlot == slot ) {
				if ( (int)cg.predictedPlayerState.weapon == weapon ) {
					return;
				}
				text = va( "%s %s %s %s",
						   va( CG_SafeTranslateString( "CGAME_SWAPWEAPONS" ), keyString ),
						   cg_weapons[ cg.predictedPlayerState.weapon ].displayName,
						   CG_SafeTranslateString( "CGAME_FOR" ),
						   wi->displayName );
			} else {
				// keep: the slot is read as a signed byte
				if ( (signed char)cg.predictedPlayerState.weaponslots[slot] == weapon ) {
					return;
				}
				text = va( "%s %s %s %s",
						   va( CG_SafeTranslateString( "CGAME_SWAPWEAPONS" ), keyString ),
						   cg_weapons[ (signed char)cg.predictedPlayerState.weaponslots[weapDef->weaponSlot] ].displayName,
						   CG_SafeTranslateString( "CGAME_FOR" ),
						   wi->displayName );
			}
		} else {
			s = CG_SafeTranslateString( "CGAME_PICKUPNEWWEAPON" );
			p = strstr( s, "[%s]" );
			if ( p && *p ) {
				Controls_GetConfig();
				GetKeyBindingLocalizedString( "+activate", &keyString );
				p++;
				strncpy( buffer, s, p - s );
				// keep: pointer store, not an index (no bound check)
				*( buffer + ( p - s ) ) = '\0';
				strcat( buffer, keyString );
				strcat( buffer, p + 2 );
			} else {
				strcpy( buffer, s );
			}
			text = va( buffer, wi->displayName );
		}

		width = trap_R_Text_Width( text, font, scale, 0 );
		height = trap_R_Text_Height( font, scale );
		trap_R_Text_Paint( rect->x + ( rect->w - width ) * 0.5f, rect->y - height,
						   font, scale, fade, text, 0, 0, textStyle );

	} else if ( cg.cursorHintIcon >= HINT_AMMO_FIRST && cg.cursorHintIcon <= HINT_AMMO_LAST ) {
		wi = &cg_weapons[cg.cursorHintIcon - ( HINT_AMMO_FIRST - 1 )];

		s = CG_SafeTranslateString( "CGAME_PICKUPAMMO" );
		p = strstr( s, "[%s]" );
		if ( p && *p ) {
			Controls_GetConfig();
			GetKeyBindingLocalizedString( "+activate", &keyString );
			p++;
			strncpy( buffer, s, p - s );
			// keep: pointer store, not an index (no bound check)
			*( buffer + ( p - s ) ) = '\0';
			strcat( buffer, keyString );
			strcat( buffer, p + 2 );
		} else {
			strcpy( buffer, s );
		}
		text = va( buffer, wi->displayName );

		width = trap_R_Text_Width( text, font, scale, 0 );
		height = trap_R_Text_Height( font, scale );
		trap_R_Text_Paint( rect->x + ( rect->w - width ) * 0.5f, rect->y - height,
						   font, scale, fade, text, 0, 0, textStyle );

	} else if ( cg.cursorHintString >= 0 ) {
		s = CG_ConfigString( CS_HINTSTRINGS + cg.cursorHintString );
		if ( s && s[0] ) {
			// the "[%s]" is found in a lowercased copy but spliced out of the
			// translated original
			s = CG_TranslateMessage( s, "Hint String" );
			strcpy( lower, s );
			Q_strlwr( lower );

			p = strstr( lower, "[%s]" );
			if ( p ) {
				n = p - lower;
				strncpy( lower, s, n + 1 );
				// keep: pointer store, not an index (no bound check)
				*( lower + n + 1 ) = '\0';
				n += 3;
				Controls_GetConfig();
				if ( !GetKeyBindingLocalizedString( "+activate", &keyString ) ) {
					text = CG_SafeTranslateString( "KEY_USE" );
				} else {
					text = keyString;
				}
				strcat( lower, text );
				strcat( lower, s + n );
				text = lower;
			} else {
				text = s;
			}

			width = trap_R_Text_Width( text, font, scale, 0 );
			height = trap_R_Text_Height( font, scale );
			// keep: parentheses match original codegen
			trap_R_Text_Paint( rect->x + ( rect->w - width ) * 0.5f, ( rect->y - height ),
							   font, scale, fade, text, 0, 0, textStyle );
		}

	} else if ( cg.cursorHintIcon == HINT_HEALTH ) {
		Controls_GetConfig();
		GetKeyBindingLocalizedString( "+activate", &keyString );
		text = va( CG_SafeTranslateString( "CGAME_PICKUPHEALTH" ), keyString );

		width = trap_R_Text_Width( text, font, scale, 0 );
		height = trap_R_Text_Height( font, scale );
		trap_R_Text_Paint( rect->x + ( rect->w - width ) * 0.5f, rect->y - height,
						   font, scale, fade, text, 0, 0, textStyle );
	}

	trap_R_SetColor( fade );
	// keep: the double copy matches original codegen
	dh = rect->h;
	CG_DrawPic( rect->x - halfGrow + xOfs, rect->y - halfGrow,
				widthScale * rect->w + grow, (float)dh + grow, shader );
	trap_R_SetColor( NULL );

	if ( cg.cursorHintValue ) {
		fade[0] = 0;
		fade[1] = 0;
		fade[2] = 1.0f;
		fade[3] = 0.5f;
		CG_FilledBar( rect->x, rect->y + rect->h + 4.0f, rect->w, 8.0f,
					  fade, NULL, NULL, cg.cursorHintValue * ( 1.0f / 255 ), 0 );
	}
}

/*
==============
CG_DrawPlayerAmmoValue
	0 - ammo
	1 - clip

	both draw the clip on the left, the reserve on the right and a '|'
	between them; shader and type are unused
==============
*/
static void CG_DrawPlayerAmmoValue( rectDef_t *rect, int font, float scale, vec4_t color,
									qhandle_t shader, int textStyle, int type ) {
	centity_t   *cent;
	weaponInfo_t *weapDef;
	const char  *clipText;
	const char  *ammoText;
	int         weapon, clip, reserve;
	qboolean    showClip, showAmmo;
	float       x;

	clipText = NULL;
	ammoText = NULL;

	if ( !cg.predictedPlayerState.weapon ) {
		return;
	}

	cent = &cg_entities[cg.snap->ps.clientNum];

	if ( ( cg.snap->ps.pm_flags & PMF_INWORLD )
		 && cg_weaponSelect.integer >= 0 && cg_weaponSelect.integer < bg_numWeapons ) {
		weapon = cg_weaponSelect.integer;
	} else {
		weapon = cent->currentState.weapon;
	}
	if ( !weapon ) {
		return;
	}

	showAmmo = qtrue;
	showClip = qtrue;

	reserve = BG_GetTotalAmmoReserve( &cg.predictedPlayerState, weapon );
	weapDef = bg_weaponInfo[weapon];

	if ( weapDef->clipOnly ) {
		clip = -1;
	} else {
		clip = cg.predictedPlayerState.ammoclip[weapDef->clipIndex];
	}
	if ( clip < 0 ) {
		showClip = qfalse;
	}
	if ( clip > 999 ) {
		clip = 999;
	}
	if ( reserve < 0 ) {
		showAmmo = qfalse;
	}
	if ( reserve > 999 ) {
		reserve = 999;
	}

	if ( showClip ) {
		clipText = va( "%2i", clip );
	}
	if ( showAmmo ) {
		ammoText = va( "%i", reserve );
	}

	// keep: one flat if / else if chain matches original codegen
	if ( showClip && showAmmo ) {
		trap_R_Text_Paint( rect->x, rect->y, font, scale,
						   color, clipText, 0, 0, textStyle );

		x = rect->x + rect->w - trap_R_Text_Width( ammoText, font, scale, 0 );
		trap_R_Text_Paint( x, rect->y, font, scale, color, ammoText, 0, 0, textStyle );

		x = rect->x + ( rect->w - trap_R_Text_Width( "|", font, scale, 0 ) ) * 0.5f;
		trap_R_Text_Paint( x, rect->y, font, scale, color, "|", 0, 0, textStyle );
	} else if ( showClip ) {
		x = rect->x + ( rect->w - trap_R_Text_Width( clipText, font, scale, 0 ) ) * 0.5f;
		trap_R_Text_Paint( x, rect->y, font, scale, color, clipText, 0, 0, textStyle );
	} else if ( showAmmo ) {
		x = rect->x + ( rect->w - trap_R_Text_Width( ammoText, font, scale, 0 ) ) * 0.5f;
		trap_R_Text_Paint( x, rect->y, font, scale, color, ammoText, 0, 0, textStyle );
	}
}

/*
================
CG_DrawSelectedPlayerHealth
================
*/
static void CG_DrawSelectedPlayerHealth( rectDef_t *rect, int font, float scale, vec4_t color,
										 qhandle_t shader, int textStyle ) {
	clientInfo_t *ci;
	char         num[16];
	int          width;

	ci = &bg_clientinfo[sortedTeamPlayers[CG_GetSelectedPlayer()]];
	if ( !ci->infoValid ) {
		return;
	}

	if ( shader ) {
		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", ci->health );
		width = trap_R_Text_Width( num, font, scale, 0 );
		trap_R_Text_Paint( rect->x + ( rect->w - width ) * 0.5f, rect->y + rect->h,
						   font, scale, color, num, 0, 0, textStyle );
	}
}

/*
================
CG_DrawSelectedPlayerName

voice is unused
================
*/
static void CG_DrawSelectedPlayerName( rectDef_t *rect, int font, float scale, vec4_t color,
									   qboolean voice, int textStyle ) {
	clientInfo_t *ci;

	ci = &bg_clientinfo[sortedTeamPlayers[CG_GetSelectedPlayer()]];
	if ( !ci->infoValid ) {
		return;
	}

	trap_R_Text_Paint( rect->x, rect->y + rect->h, font, scale,
					   color, ci->name, 0, 0, textStyle );
}

/*
================
CG_GetTranslatedLocationString

Translate a location configstring, flagging it if there is no translation
================
*/
const char *CG_GetTranslatedLocationString( int location ) {
	const char *s;
	const char *t;

	s = CG_ConfigString( CS_LOCATIONS + location );
	if ( !s || !s[0] ) {
		s = "CGAME_UNKNOWN";
	}

	t = trap_SE_TranslateReference( s );
	if ( t ) {
		return t;
	}

	if ( cl_languagewarnings.integer ) {
		if ( cl_languagewarningsaserrors.integer ) {
			Com_Error( 7, "Could not translate map location string \"%s\"", s );
		} else {
			Com_Printf( "^3WARNING: Could not translate map location string \"%s\"\n", s );
		}
		strcpy( locationString, "^1UNLOCALIZED(^7" );
		strcat( locationString, s );
		strcat( locationString, "^1)^7" );
		return locationString;
	}

	strcpy( locationString, s );
	return locationString;
}

/*
================
CG_DrawSelectedPlayerLocation
================
*/
static void CG_DrawSelectedPlayerLocation( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	clientInfo_t *ci;
	const char   *s;

	ci = &bg_clientinfo[sortedTeamPlayers[CG_GetSelectedPlayer()]];
	if ( !ci->infoValid ) {
		return;
	}

	s = CG_GetTranslatedLocationString( ci->location );
	trap_R_Text_Paint( rect->x, rect->y + rect->h, font, scale,
					   color, s, 0, 0, textStyle );
}

/*
================
CG_DrawPlayerLocation
================
*/
static void CG_DrawPlayerLocation( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	clientInfo_t *ci;
	const char   *s;

	ci = &bg_clientinfo[cg.snap->ps.clientNum];
	if ( !ci->infoValid ) {
		return;
	}

	s = CG_GetTranslatedLocationString( ci->location );
	trap_R_Text_Paint( rect->x, rect->y + rect->h, font, scale,
					   color, s, 0, 0, textStyle );
}

/*
================
CG_DrawPlayerHealth

When following another player, show that player's health
================
*/
static void CG_DrawPlayerHealth( rectDef_t *rect, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	playerState_t *ps;
	int value;

	ps = &cg.snap->ps;

	if ( ps->clientNum != cg.clientNum ) {
		if ( !bg_clientinfo[ps->clientNum].infoValid ) {
			return;
		}
		value = bg_clientinfo[ps->clientNum].health;
	} else {
		value = ps->stats[STAT_HEALTH];
	}

	// DHM - Nerve :: Don't show negative health
	if ( value < 0 ) {
		value = 0;
	}

	if ( shader ) {
		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	} else {
		trap_R_SetColor( color );
		CG_DrawField( rect->x, rect->y, 3, value, 20 * scale, 32 * scale, qtrue, qtrue );           // NERVE - SMF
	}
}

/*
================
CG_DrawRedScore
================
*/
static void CG_DrawRedScore( rectDef_t *rect, int font, float scale, vec4_t color,
							 qhandle_t shader, int textStyle ) {
	char num[16];
	int  width;

	if ( cgs.scores1 == SCORE_NOT_PRESENT ) {
		Com_sprintf( num, sizeof( num ), "-" );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", cgs.scores1 );
	}
	width = trap_R_Text_Width( num, font, scale, 0 );
	trap_R_Text_Paint( rect->x + rect->w - width, rect->y + rect->h, font, scale,
					   color, num, 0, 0, textStyle );
}

/*
================
CG_DrawBlueScore
================
*/
static void CG_DrawBlueScore( rectDef_t *rect, int font, float scale, vec4_t color,
							  qhandle_t shader, int textStyle ) {
	char num[16];
	int  width;

	if ( cgs.scores2 == SCORE_NOT_PRESENT ) {
		Com_sprintf( num, sizeof( num ), "-" );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", cgs.scores2 );
	}
	width = trap_R_Text_Width( num, font, scale, 0 );
	trap_R_Text_Paint( rect->x + rect->w - width, rect->y + rect->h, font, scale,
					   color, num, 0, 0, textStyle );
}

/*
================
CG_DrawTeamColor
================
*/
static void CG_DrawTeamColor( rectDef_t *rect, vec4_t color ) {
	int client;

	client = cg.snap->ps.clientNum;
	if ( !bg_clientinfo[client].infoValid ) {
		return;
	}

	CG_DrawTeamBackground( rect->x, rect->y, rect->w, rect->h,
						   color[3], bg_clientinfo[client].team );
}

/*
================
CG_DrawAreaHoldable
CG_DrawAreaWeapons
================
*/
static void CG_DrawAreaHoldable( rectDef_t *rect, int align, float spacing, float scale, vec4_t color ) {
}

static void CG_DrawAreaWeapons( rectDef_t *rect, int align, float spacing, float scale, vec4_t color ) {
}

/*
================
CG_GetValue

cgDC.getValue
================
*/
float CG_GetValue( int ownerDraw, int type ) {
	centity_t     *cent;
	playerState_t *ps;
	int           client;
	int           weapon;
	clientInfo_t  *ci;

	ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];

	switch ( ownerDraw ) {
	case CG_SELECTEDPLAYER_HEALTH:
		ci = &bg_clientinfo[sortedTeamPlayers[CG_GetSelectedPlayer()]];
		// keep: the inverted infoValid test is the original's
		if ( !ci->infoValid ) {
			return (float)ci->health;
		}
		break;

	case CG_PLAYER_AMMO_VALUE:
		if ( cent->currentState.weapon ) {
			if ( type == 1 ) {  // RANGETYPE_RELATIVE
				weapon = BG_AmmoForWeapon( cent->currentState.weapon );
				return (float)ps->ammo[weapon] / BG_GetAmmoTypeMax( weapon );
			}
			return (float)ps->ammo[BG_AmmoForWeapon( cent->currentState.weapon )];
		}
		break;

	case CG_PLAYER_AMMOCLIP_VALUE:
		if ( cent->currentState.weapon ) {
			if ( type == 1 ) {  // RANGETYPE_RELATIVE
				weapon = BG_ClipForWeapon( cent->currentState.weapon );
				return (float)ps->ammoclip[weapon] / BG_GetAmmoClipSize( weapon );
			}
			return (float)ps->ammoclip[BG_ClipForWeapon( cent->currentState.weapon )];
		}
		break;

	case CG_PLAYER_SCORE:
		client = ps->clientNum;
		if ( !bg_clientinfo[client].infoValid ) {
			return 0;
		}
		return (float)bg_clientinfo[client].score;

	case CG_PLAYER_HEALTH:
		client = ps->clientNum;
		if ( !bg_clientinfo[client].infoValid ) {
			return 0;
		}
		return (float)bg_clientinfo[client].health;

	case CG_RED_SCORE:
		return (float)cgs.scores1;

	case CG_BLUE_SCORE:
		return (float)cgs.scores2;
	}

	return -1;
}

/*
================
CG_OwnerDrawVisible

cgDC.ownerDrawVisible
================
*/
// THINKABOUTME: should these be exclusive or inclusive..
//
qboolean CG_OwnerDrawVisible( int flags ) {
	if ( flags & CG_SHOW_TEAMINFO ) {
		return ( cg_currentSelectedPlayer.integer == numSortedTeamPlayers );
	}

	if ( flags & CG_SHOW_NOTEAMINFO ) {
		return !( cg_currentSelectedPlayer.integer == numSortedTeamPlayers );
	}

	if ( flags & CG_SHOW_HEALTHCRITICAL ) {
		if ( cg.snap->ps.stats[STAT_HEALTH] < 25 ) {
			return qtrue;
		}
	}

	if ( flags & CG_SHOW_HEALTHOK ) {
		if ( cg.snap->ps.stats[STAT_HEALTH] > 25 ) {
			return qtrue;
		}
	}

	// keep: dead test matches original codegen
	if ( flags & CG_SHOW_2DONLY ) {
		return qfalse;
	}

	return qfalse;
}

/*
================
CG_DrawAreaSystemChat

All three chat ownerdraws paint with style 0, not the item's textStyle
================
*/
static void CG_DrawAreaSystemChat( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	trap_R_Text_Paint( rect->x, rect->y + rect->h, font, scale,
					   color, systemChat, 0, 0, 0 );
}

/*
================
CG_DrawAreaTeamChat
================
*/
static void CG_DrawAreaTeamChat( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	trap_R_Text_Paint( rect->x, rect->y + rect->h, font, scale,
					   color, teamChat1, 0, 0, 0 );
}

/*
================
CG_DrawAreaChat
================
*/
static void CG_DrawAreaChat( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	trap_R_Text_Paint( rect->x, rect->y + rect->h, font, scale,
					   color, teamChat2, 0, 0, 0 );
}

/*
================
CG_GetKillerText
================
*/
const char *CG_GetKillerText( void ) {
	const char *s = "";

	if ( cg.killerName[0] ) {
		s = va( "Fragged by %s", cg.killerName );
	}
	return s;
}

/*
================
CG_DrawKiller
================
*/
static void CG_DrawKiller( rectDef_t *rect, int font, float scale, vec4_t color,
						   qhandle_t shader, int textStyle ) {
	float x;

	// fragged by ... line
	if ( cg.killerName[0] ) {
		x = rect->x + rect->w * 0.5f;
		trap_R_Text_Paint( x - trap_R_Text_Width( CG_GetKillerText(), font, scale, 0 ) / 2,
						   rect->y + rect->h, font, scale,
						   color, CG_GetKillerText(), 0, 0, textStyle );
	}
}

/*
================
CG_Draw1stPlace
================
*/
static void CG_Draw1stPlace( rectDef_t *rect, int font, float scale, vec4_t color,
							 qhandle_t shader, int textStyle ) {
	if ( cgs.scores1 != SCORE_NOT_PRESENT ) {
		trap_R_Text_Paint( rect->x, rect->y, font, scale,
						   color, va( "%2i", cgs.scores1 ), 0, 0, textStyle );
	}
}

/*
================
CG_Draw2ndPlace
================
*/
static void CG_Draw2ndPlace( rectDef_t *rect, int font, float scale, vec4_t color,
							 qhandle_t shader, int textStyle ) {
	if ( cgs.scores2 != SCORE_NOT_PRESENT ) {
		trap_R_Text_Paint( rect->x, rect->y, font, scale,
						   color, va( "%2i", cgs.scores2 ), 0, 0, textStyle );
	}
}

/*
================
CG_GameTypeString

cgs.gametype is already a string
================
*/
const char *CG_GameTypeString( void ) {
	return cgs.gametype;
}

/*
================
CG_DrawGameType
================
*/
static void CG_DrawGameType( rectDef_t *rect, int font, float scale, vec4_t color,
							 qhandle_t shader, int textStyle ) {
	trap_R_Text_Paint( rect->x, rect->y + rect->h, font, scale,
					   color, CG_GameTypeString(), 0, 0, textStyle );
}

/*
================
CG_DrawMenuShader

The item's shader, stretched to its rect in screen space
================
*/
static void CG_DrawMenuShader( rectDef_t *rect, qhandle_t shader, vec4_t color ) {
	float x, y, w, h;

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;
	CG_AdjustFrom640( &x, &y, &w, &h );
	trap_R_SetColor( color );
	trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1.0f, 1.0f, shader );
	trap_R_SetColor( NULL );
}

/*
==============
CG_OwnerDraw

cgDC.ownerDrawItem.  font is passed down to every handler.
Keep the case order: it matches the original.
==============
*/
void CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y,
				   int ownerDraw, int ownerDrawFlags, int align, float special,
				   int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	rectDef_t rect;

	if ( !cg_drawStatus.integer ) {
		return;
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {
	case CG_PLAYER_AMMO_BACKDROP:
		CG_DrawPlayerAmmoBackdrop( &rect, color, shader );
		break;
	case CG_PLAYER_AMMO_VALUE:
		CG_DrawPlayerAmmoValue( &rect, font, scale, color, shader, textStyle, 0 );
		break;
	case CG_CURSORHINT:
		CG_DrawCursorhint( &rect, font, scale, textStyle );
		break;
	case CG_PLAYER_AMMOCLIP_VALUE:
		CG_DrawPlayerAmmoValue( &rect, font, scale, color, shader, textStyle, 1 );
		break;
	case CG_VOICE_NAME:
		CG_DrawSelectedPlayerName( &rect, font, scale, color, qtrue, textStyle );
		break;
	case CG_SELECTEDPLAYER_HEALTH:
		CG_DrawSelectedPlayerHealth( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_SELECTEDPLAYER_NAME:
		CG_DrawSelectedPlayerName( &rect, font, scale, color, qfalse, textStyle );
		break;
	case CG_SELECTEDPLAYER_LOCATION:
		CG_DrawSelectedPlayerLocation( &rect, font, scale, color, textStyle );
		break;
	case CG_PLAYER_STANCE:
		CG_DrawPlayerStance( &rect, color, font, scale, textStyle );
		break;
	case CG_PLAYER_HEALTH:
		CG_DrawPlayerHealth( &rect, scale, color, shader, textStyle );
		break;
	case CG_RED_SCORE:
		CG_DrawRedScore( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_BLUE_SCORE:
		CG_DrawBlueScore( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_PLAYER_LOCATION:
		CG_DrawPlayerLocation( &rect, font, scale, color, textStyle );
		break;
	case CG_TEAM_COLOR:
		CG_DrawTeamColor( &rect, color );
		break;
	case CG_AREA_SYSTEMCHAT:
		CG_DrawAreaSystemChat( &rect, font, scale, color, shader );
		break;
	case CG_AREA_TEAMCHAT:
		CG_DrawAreaTeamChat( &rect, font, scale, color, shader );
		break;
	case CG_AREA_CHAT:
		CG_DrawAreaChat( &rect, font, scale, color, shader );
		break;
	case CG_GAME_TYPE:
		CG_DrawGameType( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_KILLER:
		CG_DrawKiller( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_AREA_WEAPON:
		CG_DrawAreaWeapons( &rect, align, special, scale, color );
		break;
	case CG_AREA_HOLDABLE:
		CG_DrawAreaHoldable( &rect, align, special, scale, color );
		break;
	case CG_PLAYER_WEAPON_NAME:
		CG_DrawPlayerWeaponName( &rect, font, scale, color, textStyle );
		break;
	case CG_PLAYER_WEAPON_NAME_BACK:
		CG_DrawPlayerWeaponNameBack( &rect, font, scale, color, shader );
		break;
	case CG_PLAYER_WEAPON_MODE_ICON:
		CG_DrawPlayerWeaponModeIcon( &rect, color );
		break;
	case CG_PLAYER_COMPASS:
		CG_DrawPlayerCompass( &rect, shader, color );
		break;
	case CG_PLAYER_COMPASS_BACK:
		CG_DrawPlayerCompassBack( &rect, shader, color );
		break;
	case CG_PLAYER_COMPASS_POINTERS:
		CG_DrawPlayerCompassPointers( &rect, shader, color );
		break;
	case CG_PLAYER_BAR_HEALTH:
		CG_DrawPlayerBarHealth( &rect, color, shader );
		break;
	case CG_PLAYER_BAR_HEALTH_TITLE:
		CG_DrawPlayerBarHealthTitle( &rect, font, scale, color, textStyle );
		break;
	case CG_PLAYER_COMPASS_FRIENDLIES:
		CG_DrawCompassFriendlies( (float *)&rect, shader, color );
		break;
	case CG_1STPLACE:
		CG_Draw1stPlace( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_2NDPLACE:
		CG_Draw2ndPlace( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_MENU_SHADER:
		CG_DrawMenuShader( &rect, shader, color );
		break;
	default:
		break;
	}
}

/*
================
CG_MouseEvent
CG_EventHandling
CG_KeyEvent

All three are empty: the in-game menus are driven by the engine, not by cgame
================
*/
void CG_MouseEvent( int x, int y ) {
}

void CG_EventHandling( int type ) {
}

void CG_KeyEvent( int key, qboolean down ) {
}

/*
================
CG_KeyInterceptEvent

The scoreboard's own scroll wheel, which has to run before the key reaches the binding system.
================
*/
qboolean CG_KeyInterceptEvent( int key, qboolean down ) {
	if ( down && cg.showScores ) {
		if ( key == K_MWHEELUP || key == K_PGUP || key == K_KP_PGUP ) {
			if ( cg.scoreboardScrollPos > 0 ) {
				cg.scoreboardScrollPos -= cg_scoreboardScrollStep.integer;
				if ( cg.scoreboardScrollPos < 0 ) {
					cg.scoreboardScrollPos = 0;
				}
			}
			return qtrue;
		}

		if ( key == K_MWHEELDOWN || key == K_PGDN || key == K_KP_PGDN ) {
			CG_ScrollScoreboardDown();
			return qtrue;
		}
	}

	return qfalse;
}

/*
================
CG_ClientNumFromName
================
*/
int CG_ClientNumFromName( const char *p ) {
	int i;

	for ( i = 0; i < cgs.maxclients; i++ ) {
		if ( bg_clientinfo[i].infoValid && Q_stricmp( bg_clientinfo[i].name, p ) == 0 ) {
			return i;
		}
	}
	return -1;
}

/*
================
CG_ShowResponseHead

Menus_OpenByName( "voiceMenu" ), open-coded
================
*/
void CG_ShowResponseHead( void ) {
	struct menuDef_s *menu;

	menu = Menus_FindByName( "voiceMenu" );
	if ( menu ) {
		Menus_Open( menu );
	}
	trap_Cvar_Set( "cl_conXOffset", "72" );
	cg.voicetime = cg.time;
}

/*
================
CG_RunMenuScript

cgDC.runScript
================
*/
void CG_RunMenuScript( char **args ) {
}

/*
================
CG_GetTeamColor

cgDC.getTeamColor
================
*/
void CG_GetTeamColor( vec4_t *color ) {
	int client;

	client = cg.snap->ps.clientNum;

	if ( bg_clientinfo[client].infoValid ) {
		if ( bg_clientinfo[client].team == TEAM_AXIS ) {
			( *color )[0] = 1.0f;
			( *color )[1] = 0;
			( *color )[2] = 0;
			( *color )[3] = .25f;
			return;
		}
		if ( bg_clientinfo[client].team == TEAM_ALLIES ) {
			( *color )[0] = ( *color )[1] = 0;
			( *color )[2] = 1.0f;
			( *color )[3] = .25f;
			return;
		}
	}
	( *color )[0] = 0;
	( *color )[1] = .17f;
	( *color )[2] = 0;
	( *color )[3] = .25f;
}
