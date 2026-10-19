// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay
// The status bar is hudelems and menus now; this file draws the crosshair,
// scope overlay, chat, lagometer, center print, fades and debug overlays.

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "cg_local.h"

extern const vec3_t vec3_origin;
extern vec4_t colorWhite;

#ifndef COLOR_RED
#define COLOR_RED       '1'
#define COLOR_GREEN     '2'
#define COLOR_YELLOW    '3'
#define COLOR_BLUE      '4'
#endif
#ifndef ColorIndex
#define ColorIndex( c ) ( ( (c) - '0' ) & 7 )
#endif

double      BG_GetMinSpreadForWeapon( int time, const playerState_t *ps, int weaponIndex );

qboolean    CG_DrawScoreboard( void );
void        CG_DrawScoreboard_GetTeamColor( vec4_t color, int team );
void        CG_DrawHudElems( void );
void        CG_DrawWeaponSelect( void );
int         CG_PlaySoundAliasByName( const char *name, int entnum, const vec3_t origin );

void       *Menus_FindByName( const char *p );
void        Menus_Close( void *menu );
void        Menu_PaintAll( void );
void        Controls_GetConfig( void );
int         GetKeyBindingLocalizedString( const char *name, char **out );

#ifndef BIGCHAR_WIDTH
#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16
#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16
#endif
#ifndef YAW
#define PITCH   0
#define YAW     1
#define ROLL    2
#endif
#ifndef STAT_MINUS
#define STAT_MINUS      10          // num frame for '-' stats digit
#endif
#ifndef CMD_BACKUP
#define CMD_BACKUP      64
#endif
#ifndef SNAPFLAG_RATE_DELAYED
#define SNAPFLAG_RATE_DELAYED   1
#endif
#ifndef CS_SERVERINFO
#define CS_SERVERINFO   0       // an info string with all the serverinfo cvars
#endif
#ifndef TEAM_AXIS
#define TEAM_AXIS       1
#define TEAM_ALLIES     2
#endif
#ifndef STEREO_CENTER
#define STEREO_CENTER   0
#define STEREO_LEFT     1
#define STEREO_RIGHT    2
#endif

static void CG_DrawTurretCrossHair( void );
static void CG_DrawDisconnect( void );

/*
================
CG_DrawFieldWidth

How wide CG_DrawField's output would be
================
*/
int CG_DrawFieldWidth( int x, int y, int width, int value, int charWidth, int charHeight ) {
	char num[16], *ptr;
	int l;
	int frame;
	int totalwidth = 0;

	if ( width < 1 ) {
		return 0;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf( num, sizeof( num ), "%i", value );
	l = strlen( num );
	if ( l > width ) {
		l = width;
	}

	ptr = num;
	while ( *ptr && l )
	{
		if ( *ptr == '-' ) {
			frame = STAT_MINUS;
		} else {
			frame = *ptr - '0';
		}

		totalwidth += charWidth;
		ptr++;
		l--;
	}

	return totalwidth;
}

/*
================
CG_DrawField
================
*/
int CG_DrawField( int x, int y, int width, int value, int charWidth, int charHeight, qboolean dodrawpic, qboolean leftAlign ) {
	char num[16], *ptr;
	int l;
	int frame;
	int startx;

	if ( width < 1 ) {
		return 0;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf( num, sizeof( num ), "%i", value );
	l = strlen( num );
	if ( l > width ) {
		l = width;
	}

	// NERVE - SMF
	if ( !leftAlign ) {
		x -= 2 + charWidth * ( l );
	}

	startx = x;

	ptr = num;
	while ( *ptr && l )
	{
		if ( *ptr == '-' ) {
			frame = STAT_MINUS;
		} else {
			frame = *ptr - '0';
		}

		if ( dodrawpic ) {
			CG_DrawPic( (float)x, (float)y, (float)charWidth, (float)charHeight,
						cgs.media.numberShaders[frame] );
		}
		x += charWidth;
		ptr++;
		l--;
	}

	return startx;
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( float x, float y, float w, float h, float alpha, int team ) {
	vec4_t hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_AXIS ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_ALLIES ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.hudColorBar );
	trap_R_SetColor( NULL );
}

/*
=================
CG_DrawScriptUsage

cg_drawScriptUsage: script variable, thread and string counts
=================
*/
void CG_DrawScriptUsage( void ) {
	CG_DrawStringExt( 480, 80, va( "num vars:    %d", trap_Scr_GetNumVars() ), colorWhite,
					  qfalse, qtrue, 8, 16, 0, qtrue );
	CG_DrawStringExt( 480, 96, va( "num threads: %d", trap_Scr_GetNumThreads() ), colorWhite,
					  qfalse, qtrue, 8, 16, 0, qtrue );
	CG_DrawStringExt( 480, 112, va( "string usage: %d", trap_Scr_GetStringUsage() ), colorWhite,
					  qfalse, qtrue, 8, 16, 0, qtrue );
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

static const int UPPERRIGHT_X = 620;    // keep: a const object, not RTCW's #define

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char        *s;
	float w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
			cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = (float)Q_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );
	y += BIGCHAR_HEIGHT + 4;

	return y;
}

/*
==================
CG_DrawFPS

Prints the average, min/max and deviation over FPS_FRAMES frames;
cg_drawFPS 2 adds the renderer's statistics underneath.
==================
*/
#define FPS_FRAMES  32

// filled in by trap_R_TrackStatistics; names are what CG_DrawFPS prints them as
typedef struct {
	int tris;
	int totalTris;
	int verts;
	int prims;
	int memA;
	int memB;
	int memC;
	int ents;
	float dc;
} rendererStats_t;

static int previousTimes[FPS_FRAMES];
static int fadeLastTime;                // CG_DrawFlashFade's
static int index;
static int previous;
static rendererStats_t r_stats;

static float CG_DrawFPS( float y ) {
	char        *s;
	float w;
	int i, total;
	int minTime, maxTime;
	int fps, lowFps, highFps;
	float mean, variance;
	int t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;
	minTime = 0x7FFFFFFF;
	maxTime = 0;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index < FPS_FRAMES ) {
		return y;
	}

	// average multiple frames together to smooth changes out a bit
	total = 0;
	for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
		total += previousTimes[i];
		if ( minTime > previousTimes[i] ) {
			minTime = previousTimes[i];
		}
		if ( maxTime < previousTimes[i] ) {
			maxTime = previousTimes[i];
		}
	}

	mean = total * ( 1.0f / FPS_FRAMES );
	variance = 0;
	for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
		variance += (float)fabs( previousTimes[i] - mean );
	}
	variance *= ( 1.0f / FPS_FRAMES );

	if ( !total ) {
		total = 1;
	}
	if ( minTime <= 0 ) {
		minTime = 1;
	}
	fps = Q_ftol( 1000 * FPS_FRAMES / (float)total );
	lowFps = Q_ftol( 1000 / (float)maxTime );
	highFps = Q_ftol( 1000 / (float)minTime );

	s = va( "%ifps(%i-%i,%i)", fps, lowFps, highFps, Q_ftol( variance ) );
	w = (float)Q_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( UPPERRIGHT_X - w, y + 2, s, 1.0F );
	y += BIGCHAR_HEIGHT + 4;

	if ( cg_drawFPS.integer <= 1 ) {
		return y;
	}

	trap_R_TrackStatistics( &r_stats );

	s = va( "%i/%i tris", r_stats.tris / 3, r_stats.totalTris / 3 );
	w = (float)Q_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( UPPERRIGHT_X - w, y + 1, s, 1.0F );
	y += SMALLCHAR_HEIGHT;

	s = va( "%i vert", r_stats.verts );
	w = (float)Q_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( UPPERRIGHT_X - w, y + 1, s, 1.0F );
	y += SMALLCHAR_HEIGHT;

	s = va( "%i prim", r_stats.prims );
	w = (float)Q_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( UPPERRIGHT_X - w, y + 1, s, 1.0F );
	y += SMALLCHAR_HEIGHT;

	s = va( "%i ents", r_stats.ents );
	w = (float)Q_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( UPPERRIGHT_X - w, y + 1, s, 1.0F );
	y += SMALLCHAR_HEIGHT;

	s = va( "%.2f/%.2f/%.2f mb  ", r_stats.memC * ( 1.0f / ( 1024 * 1024 ) ),
			( r_stats.memA - r_stats.memB ) * ( 1.0f / ( 1024 * 1024 ) ),
			r_stats.memA * ( 1.0f / ( 1024 * 1024 ) ) );
	w = (float)Q_DrawStrlen( s ) * SMALLCHAR_WIDTH;
	CG_DrawSmallString( UPPERRIGHT_X - w, y + 1, s, 1.0F );
	y += SMALLCHAR_HEIGHT;

	if ( r_stats.dc != 0 ) {
		s = va( "%.2f dc  ", r_stats.dc );
		w = (float)Q_DrawStrlen( s ) * SMALLCHAR_WIDTH;
		CG_DrawSmallString( UPPERRIGHT_X - w, y + 1, s, 1.0F );
		y += SMALLCHAR_HEIGHT;
		return y;
	}

	return y;
}

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float y;

	y = 0;

	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
}

/*
=================
CG_DrawChatMessages

Each line gets a colored bar behind it, and the "^7" after the speaker's
name splits it into a team-colored name and a white message
=================
*/
static void CG_DrawChatMessages( void ) {
	int chatHeight;
	int i;
	float alpha;
	float x, y;
	float w;
	char *sep;
	vec4_t hcolor;

	if ( cg_chatHeight.integer < TEAMCHAT_HEIGHT ) {
		chatHeight = cg_chatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}
	if ( chatHeight <= 0 ) {
		return;
	}

	if ( cgs.teamLastChatPos == cgs.teamChatPos ) {
		return;
	}

	if ( cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_chatTime.integer ) {
		cgs.teamLastChatPos++;
	}

	for ( i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i-- ) {
		if ( !bg_clientinfo[cg.snap->ps.clientNum].infoValid ) {
			continue;
		}

		alpha = cg_chatTime.value - ( cg.time - cgs.teamChatMsgTimes[i % chatHeight] );
		if ( alpha > 200.0f ) {
			alpha = 1.0f;
		} else {
			alpha *= ( 1.0f / 200.0f );
			if ( alpha <= 0 ) {
				continue;
			}
		}

		CG_DrawScoreboard_GetTeamColor( hcolor, bg_clientinfo[cg.snap->ps.clientNum].team );
		hcolor[0] *= 0.25f;
		hcolor[1] *= 0.25f;
		hcolor[2] *= 0.25f;
		hcolor[3] = alpha * 0.6f;

		y = ( i - cgs.teamChatPos ) * 10 + 84;
		w = trap_R_Text_Width( cgs.teamChatMsgs[i % chatHeight], 0, 10 / 48.0f, 0 ) + 24;
		trap_R_SetColor( hcolor );
		CG_DrawPic( 0, y, w, 10, cgs.media.hudColorBar );

		hcolor[3] = alpha;
		y = ( i - cgs.teamChatPos ) * 10 + 93;
		x = 8;

		sep = strstr( cgs.teamChatMsgs[i % chatHeight], "^7" );
		if ( sep && sep != cgs.teamChatMsgs[i % chatHeight] ) {
			// the name half, in the speaker's team color
			CG_DrawScoreboard_GetTeamColor( hcolor, bg_clientinfo[cg.snap->ps.clientNum].team );
			trap_R_Text_Paint( x, y, 0, 10 / 48.0f, hcolor, cgs.teamChatMsgs[i % chatHeight], 0,
							   sep - cgs.teamChatMsgs[i % chatHeight], 3 );
			x += trap_R_Text_Width( cgs.teamChatMsgs[i % chatHeight], 0, 10 / 48.0f,
									sep - cgs.teamChatMsgs[i % chatHeight] );
		} else {
			sep = cgs.teamChatMsgs[i % chatHeight];
		}
		hcolor[0] = 1.0f;
		hcolor[1] = 1.0f;
		hcolor[2] = 1.0f;
		trap_R_Text_Paint( x, y, 0, 10 / 48.0f, hcolor, sep, 0, 0, 3 );
	}

	trap_R_SetColor( NULL );
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES     128

typedef struct {
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1 ) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1 ) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	int cmdNum;
	usercmd_t cmd;
	const char      *s;
	float w;
	vec4_t color = { 1, 1, 1, 1 };
	float x;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		 || cmd.serverTime > cg.time ) {    // special check for map_restart
		return;
	}

	// also add text in center of screen
	s = CG_SafeTranslateString( "CGAME_CONNECTIONINTERUPTED" );
	w = trap_R_Text_Width( s, 0, 16 / 48.0f, 0 );
	x = ( 640 - (int)w ) / 2;
	trap_R_Text_Paint( x, 100, 0, 16 / 48.0f, color, s, 0, 0, 3 );

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	CG_DrawPic( 296, 416, 48, 48, trap_R_RegisterShader( "gfx/2d/net.tga", 5 ) );
}


#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int a, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int color;
	float vscale;
	rfloat vd;      // keep: rfloat and reciprocal multiplies match original codegen

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	trap_R_SetColor( NULL );
	CG_DrawPic( 585, 340, 48, 48, cgs.media.lagometerShader );

	ax = 585;
	ay = 340;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah * ( 1.0f / 3.0f );
	mid = (rfloat)ah * ( 1.0f / 3.0f ) + ay;

	vscale = range * ( 1.0f / MAX_LAGOMETER_RANGE );

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - a - 1 ) & ( LAG_SAMPLES - 1 );
		v = (float)lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_BLUE )] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range * ( 1.0f / MAX_LAGOMETER_PING );

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - a - 1 ) & ( LAG_SAMPLES - 1 );
		v = (float)lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;      // YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_YELLOW )] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex( COLOR_GREEN )] );
				}
			}
			vd = v * (rfloat)vscale;
			if ( vd > range ) {
				vd = range;
			}
			v = vd;
			trap_R_DrawStretchPic( ax + aw - a, FCAST( (rfloat)ay + ah - vd ), 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;          // RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex( COLOR_RED )] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || g_synchronousClients.integer ) {
		CG_DrawBigString( (float)(int)ax, (float)(int)ay, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}


/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

#define CP_LINEWIDTH 75

/*
==============
CG_PriorityCenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_PriorityCenterPrint( const char *str, float y, float charWidth, int priority ) {
	char    *s;
	int len;
	qboolean neednewline = qfalse;
	int c;

	// NERVE - SMF - don't draw if this print message is less important
	if ( cg.centerPrintTime && priority < cg.centerPrintPriority ) {
		return;
	}

	str = CG_TranslateMessage( str, "Center Print" );
	strncpy( cg.centerPrint, str, sizeof( cg.centerPrint ) - 1 );
	cg.centerPrintPriority = priority;
	cg.centerPrint[ sizeof( cg.centerPrint ) - 1 ] = 0;

	// turn spaces into newlines, if we've run over the linewidth
	len = 0;
	s = cg.centerPrint;
	while ( *s ) {
		c = trap_SE_ReadCharFromString( (const unsigned char **)&s, NULL );
		if ( c == '\n' ) {
			neednewline = qfalse;
			len = 0;
			continue;
		}
		len++;
		if ( len >= CP_LINEWIDTH ) {
			neednewline = qtrue;
		} else if ( !neednewline ) {
			continue;
		}
		// only break on spaces, and if we've run over the linewidth
		if ( c == ' ' ) {
			s[-1] = '\n';
			neednewline = qfalse;
			len = 0;
		}
	}

	cg.centerPrintTime = cg.time + 2000;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while ( *s ) {
		c = trap_SE_ReadCharFromString( (const unsigned char **)&s, NULL );
		if ( c == '\n' ) {
			cg.centerPrintLines++;
		} else if ( c == '\\' && *s == 'n' ) {
			cg.centerPrintLines++;
			s++;
		}
	}
}

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, float y, float charWidth ) {
	CG_PriorityCenterPrint( str, y, charWidth, 0 );
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char    *start;
	int l;
	int h;
	float x, y, lineHeight;
	float w;
	float scale;
	float   *color;
	char linebuffer[1024];

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, (int)( cg_centertime.value * 1000 ) );
	if ( !color ) {
		cg.centerPrintTime = 0;
		cg.centerPrintPriority = 0;
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	scale = cg.centerPrintCharWidth / 32.0f;
	h = trap_R_Text_Height( 0, scale );

	// keep: the double cast matches original codegen
	y = FCAST( cg.centerPrintY - (double)( h * 0.5f ) * cg.centerPrintLines ) - h;
	lineHeight = h * 1.2f;

	while ( 1 ) {
		for ( l = 0; l < CP_LINEWIDTH; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		*( linebuffer + l ) = 0;    // keep: pointer store, not linebuffer[l]

		w = trap_R_Text_Width( linebuffer, 0, scale, 0 );
		x = ( 640.0f - w ) * 0.5f;
		trap_R_Text_Paint( x, y, 0, scale, color, linebuffer, 0, 0, 3 );

		y += lineHeight;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}


/*
================================================================================

CROSSHAIRS

================================================================================
*/

/*
=================
CG_CalcCrosshairPosition

Project the gun's aim direction onto the 640x480 virtual screen, relative to
its center
=================
*/
static void CG_CalcCrosshairPosition( float *x, float *y ) {
	vec3_t angles;
	vec3_t forward;
	float dot;

	angles[PITCH] = cg.gunPitch;
	angles[YAW] = cg.gunYaw;
	angles[ROLL] = 0;
	AngleVectors( angles, forward, NULL, NULL );

	dot = cg.refdef.viewaxis[0][0] * forward[0]
		  + cg.refdef.viewaxis[0][1] * forward[1]
		  + cg.refdef.viewaxis[0][2] * forward[2];

	if ( dot <= 0 || cg.refdef.fov_x <= 0 || cg.refdef.fov_y <= 0 ) {
		*x = 0;
		*y = 0;
		return;
	}

	*x = ( cg.refdef.viewaxis[1][0] * forward[0]
		   + cg.refdef.viewaxis[1][1] * forward[1]
		   + cg.refdef.viewaxis[1][2] * forward[2] )
		 / ( (float)tan( cg.refdef.fov_x * ( M_PI / 360.0f ) ) * dot ) * -320.0f;

	*y = ( cg.refdef.viewaxis[2][0] * forward[0]
		   + cg.refdef.viewaxis[2][1] * forward[1]
		   + cg.refdef.viewaxis[2][2] * forward[2] )
		 / ( (float)tan( cg.refdef.fov_y * ( M_PI / 360.0f ) ) * dot ) * -240.0f;
}

/*
=================
CG_GetWeapReticleZoom

How far into the ADS overlay we are, 0..1
=================
*/
qboolean CG_GetWeapReticleZoom( float *zoom ) {
	float frac;

	frac = cg.predictedPlayerState.fWeaponPosFrac;
	*zoom = 0;

	if ( ( !cg.weaponInfo->adsOverlayShader[0] && !cg.weaponInfo->adsOverlayReticle )
		 || frac == 0 ) {
		return qfalse;
	}

	if ( cg.playerEntity.bPositionToADS ) {
		*zoom = frac - ( 1.0f - cg.weaponInfo->adsZoomInFrac );
		if ( *zoom > 0 ) {
			*zoom = *zoom / cg.weaponInfo->adsZoomInFrac;
		}
	} else {
		*zoom = frac - ( 1.0f - cg.weaponInfo->adsZoomOutFrac );
		if ( *zoom > 0 ) {
			*zoom = *zoom / cg.weaponInfo->adsZoomOutFrac;
		}
	}

	if ( *zoom <= 0.01f ) {
		return qfalse;
	}
	return qtrue;
}

/*
=================
CG_DrawWeapReticle

Draw the scope overlay: four mirrored quadrants of the overlay shader, the
fill outside them, and then whichever center reticle the weapon asked for.
Returns how much of the world is still visible, which is what the crosshair
fades itself by.
=================
*/
static float CG_DrawWeapReticle( void ) {
	vec4_t color = { 1, 1, 1, 1 };
	float zoom;
	int weapnum;
	float crossX, crossY;
	float halfW, halfH;
	float cx = 0, cy = 0;   // keep: dead inits, *= forms and FCASTs match original codegen
	float x0, y0, x1, y1;
	float w, h;
	qhandle_t shader;

	if ( !CG_GetWeapReticleZoom( &zoom ) ) {
		return 1.0f;
	}
	color[3] = zoom;

	weapnum = cg.weaponInfo->weapIndex;

	CG_CalcCrosshairPosition( &crossX, &crossY );

	halfW = cg.weaponInfo->adsOverlayWidth;
	halfW *= cgs.screenXScale;
	halfH = cg.weaponInfo->adsOverlayHeight;
	halfH *= cgs.screenYScale;
	cx = cgs.screenXScale * crossX + cg.refdef.width * 0.5f + cg.refdef.x;
	cy = cgs.screenYScale * crossY + cg.refdef.height * 0.5f + cg.refdef.y;

	trap_R_SetColor( color );

	if ( cg.weaponInfo->adsOverlayShader[0] ) {
		shader = cg_weapons[ weapnum ].adsOverlayShader;

		y0 = cy - halfH;
		x0 = cx - halfW;

		// the overlay is one quadrant, mirrored into the other three
		trap_R_DrawStretchPic( x0, y0, halfW, halfH, 0, 0, 1, 1, shader );
		trap_R_DrawStretchPic( cx, y0, halfW, halfH, 1, 0, 0, 1, shader );
		trap_R_DrawStretchPic( x0, cy, halfW, halfH, 0, 1, 1, 0, shader );
		trap_R_DrawStretchPic( cx, cy, halfW, halfH, 1, 1, 0, 0, shader );

		// and everything outside it is filled with the overlay's edge texel
		if ( x0 > 0 ) {
			trap_R_DrawStretchPic( 0, 0, x0, (float)cg.refdef.height, 0, 0, 0, 1, shader );
		}
		x1 = halfW + cx;
		if ( x1 < cg.refdef.width ) {
			trap_R_DrawStretchPic( x1, 0, cg.refdef.width - x1, (float)cg.refdef.height,
								   0, 0, 0, 1, shader );
		}
		if ( y0 > 0 ) {
			trap_R_DrawStretchPic( x0, 0, halfW + halfW, y0, 0, 0, 1, 0, shader );
		}
		y1 = halfH + cy;
		if ( y1 < cg.refdef.height ) {
			trap_R_DrawStretchPic( x0, y1, halfW + halfW, cg.refdef.height - y1,
								   0, 0, 1, 0, shader );
		}
	}

	if ( cg.weaponInfo->adsOverlayReticle ) {
		if ( cg.weaponInfo->adsOverlayReticle == 1 ) {
			// the weapon's own center reticle, at its natural size
			w = (float)cg.weaponInfo->reticleCenterSize;
			h = w;
			w *= cgs.screenXScale;
			h *= cgs.screenYScale;
			trap_R_DrawStretchPic(
				( cg.refdef.width - w ) * 0.5f + cgs.screenXScale * crossX + cg.refdef.x,
				( cg.refdef.height - h ) * 0.5f + cgs.screenYScale * crossY + cg.refdef.y,
				w, h, 0, 0, 1, 1, cg_weapons[ weapnum ].reticleCenter );
		} else if ( cg.weaponInfo->adsOverlayReticle == 2 ) {
			// a short crosshair with a gap
			color[0] = color[1] = color[2] = 0;
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( cx - 1, cy, 3, halfH * 0.9f, 0, 0, 1, 1,
								   cgs.media.hudSoftLineShader );
			trap_R_DrawStretchPic( cx - halfW * 0.9f, cy - 1, halfW * 0.75f, 3, 0, 0, 1, 1,
								   cgs.media.hudSoftLineHShader );
			trap_R_DrawStretchPic( cx + FCAST( halfW * 0.15f ), cy - 1, halfW * 0.75f, 3, 0, 0, 1, 1,
								   cgs.media.hudSoftLineHShader );
		} else if ( cg.weaponInfo->adsOverlayReticle == 3 ) {
			// one long cross through the middle
			color[0] = color[1] = color[2] = 0;
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( cx - 1, cy - halfH * 0.9f, 3, halfH * 1.8f, 0, 0, 1, 1,
								   cgs.media.hudSoftLineShader );
			trap_R_DrawStretchPic( cx - halfW * 0.9f, cy - 1, halfW * 1.8f, 3, 0, 0, 1, 1,
								   cgs.media.hudSoftLineHShader );
		} else if ( cg.weaponInfo->adsOverlayReticle == 4 ) {
			// same as 2
			color[0] = color[1] = color[2] = 0;
			trap_R_SetColor( color );
			trap_R_DrawStretchPic( cx - 1, cy, 3, halfH * 0.9f, 0, 0, 1, 1,
								   cgs.media.hudSoftLineShader );
			trap_R_DrawStretchPic( cx - halfW * 0.9f, cy - 1, halfW * 0.75f, 3, 0, 0, 1, 1,
								   cgs.media.hudSoftLineHShader );
			trap_R_DrawStretchPic( cx + FCAST( halfW * 0.15f ), cy - 1, halfW * 0.75f, 3, 0, 0, 1, 1,
								   cgs.media.hudSoftLineHShader );
		}
	}

	trap_R_SetColor( NULL );

	return 1.0f - zoom;
}

/*
=================
CG_DrawTurretCrossHair

The crosshair of the turret the player is locked to
(keep: the two * 0.0f terms match original codegen)
=================
*/
static void CG_DrawTurretCrossHair( void ) {
	vec4_t color = { 1, 1, 1, 0 };
	centity_t *cent;
	int weapnum;
	weaponInfo_t *weapon;
	float w, h;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( cl_paused.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	cent = &cg_entities[ cg.predictedPlayerState.viewlocked_entNum ];
	if ( !cent->currentState.weapon ) {
		return;
	}
	weapnum = cent->currentState.weapon;
	weapon = bg_weaponInfo[weapnum];
	if ( !weapon->reticleCenter[0] ) {
		return;
	}

	color[3] = cg_crosshairAlpha.value;
	if ( color[3] < 0.01f ) {
		return;
	}

	trap_R_SetColor( color );

	w = weapon->reticleCenterSize * cgs.screenXScale;
	h = weapon->reticleCenterSize * cgs.screenYScale;

	trap_R_DrawStretchPic(
		cg.refdef.x + ( cg.refdef.width - w ) * 0.5f + cgs.screenXScale * 0.0f,
		cg.refdef.y + ( cg.refdef.height - h ) * 0.5f + cgs.screenYScale * 0.0f,
		w, h, 0, 0, 1, 1, cg_weapons[ weapnum ].reticleCenter );
}

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( void ) {
	vec4_t color = { 1, 1, 1, 0 };
	float frac;
	float scale;
	float offsetY;
	float crossX, crossY;
	int weapnum;
	float f;
	float size, barSize;
	float hx, hy;
	qhandle_t shader;
	rfloat minSpread, spread, spreadY;  // keep: rfloat, FCAST and statement order match original codegen
	float spreadX, minOfs;
	int i;
	float dir[4][2], ofs[4][2], add[4][2];

	frac = cg.predictedPlayerState.fWeaponPosFrac;
	scale = 1.0f;
	offsetY = 0;

	if ( cg.renderingThirdPerson ) {
		return;
	}

	color[3] = cg_crosshairAlpha.value;

	if ( cg.predictedPlayerState.eFlags & 0xC000 ) {        // riding a turret
		if ( cg.predictedPlayerState.viewlocked_entNum != ENTITYNUM_NONE ) {
			CG_DrawTurretCrossHair();
		}
		return;
	}

	if ( cg.predictedPlayerState.serverCursorHint == 9 ) {
		color[0] = color[2] = 0.25f;
	}

	weapnum = cg.weaponInfo->weapIndex;
	if ( !weapnum ) {
		return;
	}

	color[3] = CG_DrawWeapReticle() * cg_crosshairAlpha.value;
	if ( color[3] < 0.01f ) {
		return;
	}
	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( cl_paused.integer ) {
		return;
	}
	if ( frac == 1.0f && cg_drawGun.integer ) {
		return;
	}
	if ( cg.predictedPlayerState.weaponstate == WEAPON_MELEE_INIT
		 || cg.predictedPlayerState.weaponstate == WEAPON_MELEE_FIRE
		 || cg.predictedPlayerState.weaponstate == WEAPON_RELOADING
		 || cg.predictedPlayerState.weaponstate == WEAPON_RAISING
		 || cg.predictedPlayerState.weaponstate == WEAPON_DROPPING ) {
		return;
	}

	CG_CalcCrosshairPosition( &crossX, &crossY );
	trap_R_SetColor( color );

	if ( frac != 0 ) {
		if ( cg.playerEntity.bPositionToADS ) {
			f = frac - ( 1.0f - cg.weaponInfo->adsCrosshairInFrac );
			if ( f > 0 ) {
				f = f / cg.weaponInfo->adsCrosshairInFrac;
			}
		} else {
			f = frac - ( 1.0f - cg.weaponInfo->adsCrosshairOutFrac );
			if ( f > 0 ) {
				f = f / cg.weaponInfo->adsCrosshairOutFrac;
			}
		}
		if ( f > 0 ) {
			scale = 1.0f - 0.5f * f;
			offsetY = FCAST( 480.0f / cg.refdef.fov_y ) * cg.weaponInfo->adsAimPitch * f;
		}

		if ( !cg_drawGun.integer ) {
			// the shrinking hip crosshair, drawn only when the gun is hidden
			if ( scale < 1.0f ) {
				shader = cg_weapons[ weapnum ].reticleCenter;
				if ( !shader ) {
					shader = trap_R_RegisterShader( cg_crosshairNoGun.string, 5 );
				}
				if ( shader ) {
					size = (float)cg.weaponInfo->reticleCenterSize;
					size *= 1.5f - scale;
					trap_R_DrawStretchPic(
						cg.refdef.x + ( cg.refdef.width - size ) * 0.5f + cgs.screenXScale * crossX,
						cg.refdef.y + ( cg.refdef.height - size ) * 0.5f + cgs.screenYScale * crossY,
						size, size, 0, 0, 1, 1, shader );
				}
			}
			if ( frac == 1.0f ) {
				trap_R_SetColor( NULL );
				return;
			}
		}
	}

	if ( !cg_crosshairDynamic.integer ) {
		crossX = 0;
		crossY = offsetY;
	}

	if ( cg.weaponInfo->reticleCenter[0] ) {
		hx = cgs.screenXScale * crossX;
		hy = cgs.screenYScale * crossY;
		size = (float)cg.weaponInfo->reticleCenterSize;

		// a cooking grenade grows its reticle and ticks once a second
		if ( cg.weaponInfo->weaponType == 1 && cg.predictedPlayerState.grenadeTimeLeft
			 && cg.weaponInfo->cookOffHold ) {
			if ( cg.grenLastTime % 1000 < cg.predictedPlayerState.grenadeTimeLeft % 1000
				 && (unsigned)( cg.predictedPlayerState.grenadeTimeLeft / 1000 ) < 4 ) {
				CG_PlayClientSoundAliasByName(
					(const char *)cgs.media.grenadePulse[cg.predictedPlayerState.grenadeTimeLeft / 1000] );
			}
			cg.grenLastTime = cg.predictedPlayerState.grenadeTimeLeft;
			size += ( cg.predictedPlayerState.grenadeTimeLeft % 1000 ) * 0.01f;
		}

		size *= scale;
		trap_R_DrawStretchPic(
			cg.refdef.x + ( cg.refdef.width - size ) * 0.5f + hx,
			cg.refdef.y + ( cg.refdef.height - size ) * 0.5f + hy,
			size, size, 0, 0, 1, 1, cg_weapons[ weapnum ].reticleCenter );
	}

	if ( !cg.weaponInfo->reticleSide[0] ) {
		trap_R_SetColor( NULL );
		return;
	}

	// the four spread bars
	color[3] = CG_DrawWeapReticle()
			   * ( 1.0f - cg.predictedPlayerState.aimSpreadScale * ( 1.0f / 255.0f ) )
			   * cg_crosshairAlpha.value;
	if ( color[3] < cg_crosshairAlphaMin.value ) {
		color[3] = cg_crosshairAlphaMin.value;
	}
	trap_R_SetColor( color );

	minSpread = BG_GetMinSpreadForWeapon( cg.snap->serverTime, &cg.predictedPlayerState, weapnum );
	spread = ( cg.weaponInfo->hipSpreadMax - minSpread )
			 * ( cg.predictedPlayerState.aimSpreadScale * ( 1.0f / 255.0f ) ) + minSpread;
	spread *= scale;

	spreadX = FCAST( 640.0f / cg.refdef.fov_x ) * spread;
	spreadY = FCAST( 480.0f / cg.refdef.fov_y ) * spread;
	minOfs = (float)cg.weaponInfo->reticleMinOfs;
	if ( spreadX < minOfs ) {
		spreadX = minOfs;
	}
	if ( spreadY < minOfs ) {
		spreadY = minOfs;
	}

	shader = cg_weapons[ weapnum ].reticleSide;

	i = 0;
	ofs[0][0] = -0.5f;  ofs[0][1] = -1;
	ofs[1][0] = 0;      ofs[1][1] = -0.5f;
	ofs[2][0] = -0.5f;  ofs[2][1] = 0;
	ofs[3][0] = -1;     ofs[3][1] = -0.5f;

	dir[0][0] = 0;      dir[0][1] = -1;
	dir[1][0] = 1;      dir[1][1] = 0;
	dir[2][0] = 0;      dir[2][1] = 1;
	dir[3][0] = -1;     dir[3][1] = 0;

	add[0][0] = 0;      add[0][1] = -1;
	add[1][0] = 0;      add[1][1] = 0;
	add[2][0] = 0;      add[2][1] = 0;
	add[3][0] = -1;     add[3][1] = 0;

	for ( ; i < 4; i++ ) {
		barSize = (float)cg.weaponInfo->reticleSideSize * scale;
		trap_R_DrawStretchPicRotate(
			( spreadX * dir[i][0] + crossX ) * cgs.screenXScale
				+ barSize * ofs[i][0] + add[i][0]
				- dir[i][0] * cg.weaponInfo->hipReticleSidePos * barSize
				+ cg.refdef.width * 0.5f + cg.refdef.x,
			( spreadY * dir[i][1] + crossY ) * cgs.screenYScale
				+ barSize * ofs[i][1] + add[i][1]
				- dir[i][1] * cg.weaponInfo->hipReticleSidePos * barSize
				+ cg.refdef.height * 0.5f + cg.refdef.y,
			barSize, barSize,
			0, (float)( ( i >> 1 ) & 1 ), 1, (float)( ( ( i - 2 ) >> 1 ) & 1 ),
			( i & 1 ) * 90.0f, shader );
	}

	trap_R_SetColor( NULL );
}

/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t trace;
	vec3_t start, end;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 8192, cg.refdef.viewaxis[0], end );

	// CONTENTS_SOLID | CONTENTS_BODY
	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
			  cg.snap->ps.clientNum, 0x2000001 );

	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

/*
==============
CG_CheckForCursorHints
==============
*/
void CG_CheckForCursorHints( void ) {
	if ( cg.renderingThirdPerson ) {
		return;
	}
	if ( !cg.snap->ps.serverCursorHint ) {
		return;
	}

	cg.cursorHintTime = cg.time;
	cg.cursorHintFade = cg_hintFadeTime.integer;
	cg.cursorHintIcon = cg.snap->ps.serverCursorHint;
	cg.cursorHintValue = cg.snap->ps.serverCursorHintVal;
	cg.cursorHintString = cg.snap->ps.serverCursorHintString;
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float   *fadeColor;
	char    *name;
	int myTeam;
	float f;
	vec4_t color;

	if ( cg_drawCrosshair.integer < 0 ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	fadeColor = CG_FadeColor( cg.crosshairClientTime, 150 );
	if ( !fadeColor ) {
		return;
	}
	if ( cg.crosshairClientNum > MAX_CLIENTS ) {
		return;
	}
	if ( !bg_clientinfo[cg.snap->ps.clientNum].infoValid ) {
		return;
	}
	myTeam = bg_clientinfo[cg.snap->ps.clientNum].team;
	if ( !myTeam ) {
		return;
	}
	if ( !bg_clientinfo[cg.crosshairClientNum].infoValid ) {
		return;
	}
	if ( myTeam != 3 && bg_clientinfo[cg.crosshairClientNum].team != myTeam ) {
		return;
	}

	name = bg_clientinfo[cg.crosshairClientNum].name;
	name = va( "%s", name );
	if ( !name || !*name ) {
		return;
	}
	Q_CleanStr( name );

	if ( cg.crosshairClientNum == cg.identifyClientNum ) {
		f = cg.identifyClientHealth * 0.01f;
		if ( f > 1.0f ) {
			f = 1.0f;
		} else if ( f < 0 ) {
			f = 0;
		}
		color[2] = 0;
		if ( f > 0.5 ) {
			color[0] = ( 1.0f - f ) * 2;
			color[1] = 1.0f;
		} else {
			color[0] = 1.0f;
			color[1] = f * 2;
		}
	} else {
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 1.0f;
	}
	color[3] = fadeColor[3] * 0.6f;

	trap_R_Text_Paint( 345, 217, 0, 0.25f, color, name, 0, 0, 3 );
}

//==================================================================================

/*
==============
CG_DrawFlashFade

The script fade: cgs.scriptFade[0] is the target alpha, [1] the current one
==============
*/
static void CG_DrawFlashFade( void ) {
	int t, msec;
	vec4_t col;

	if ( cgs.scriptFadeStartTime + cgs.scriptFadeDuration < cg.time ) {
		cgs.scriptFade[1] = cgs.scriptFade[0];
	} else if ( cgs.scriptFade[1] != cgs.scriptFade[0] ) {
		t = trap_Milliseconds();    // we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading
		msec = t - fadeLastTime;
		fadeLastTime = t;
		if ( msec < 500 && msec > 0 ) {
			// keep: the rfloat casts match original codegen
			if ( cgs.scriptFade[1] > cgs.scriptFade[0] ) {
				cgs.scriptFade[1] -= msec / (rfloat)cgs.scriptFadeDuration;
				if ( cgs.scriptFade[1] < cgs.scriptFade[0] ) {
					cgs.scriptFade[1] = cgs.scriptFade[0];
				}
			} else {
				cgs.scriptFade[1] += msec / (rfloat)cgs.scriptFadeDuration;
				if ( cgs.scriptFade[1] > cgs.scriptFade[0] ) {
					cgs.scriptFade[1] = cgs.scriptFade[0];
				}
			}
		}
	}

	// now draw the fade
	if ( cgs.scriptFade[1] > 0.0 ) {
		VectorClear( col );
		col[3] = cgs.scriptFade[1];
		CG_FillRect( 0, 0, 640, 480, col );
	}
}

/*
==============
CG_DrawFlashDamage
==============
*/
#define DAMAGE_TIME 500

static void CG_DrawFlashDamage( void ) {
	float redFlash;
	vec4_t col;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.v_dmg_time <= cg.time ) {
		return;
	}

	redFlash = (float)fabs( cg.v_dmg_pitch * ( (float)( cg.v_dmg_time - cg.time ) * ( 1.0f / DAMAGE_TIME ) ) );

	// blend the entire screen red
	if ( redFlash > 5 ) {
		redFlash = 5;
	}

	col[0] = 0.2f;
	col[1] = 0;
	col[2] = 0;
	col[3] = 0.7f * ( redFlash * 0.2f );
	CG_FillRect( -10, -10, 650, 490, col );
}

static const float cg_damageIconTexCoords[8] = { 1, 1,  0, 1,  0, 0,  1, 0 };

/*
==============
CG_DrawDamageDirectionIndicators

Each cg.damageIndicators entry is { serverTime, lifetime, yaw }
==============
*/
static void CG_DrawDamageDirectionIndicators( void ) {
	float zoom;
	float x, y;
	float w;
	float verts[8];
	vec4_t color = { 1, 1, 1, 0 };
	int i;
	int t, duration;
	rfloat alpha;

	if ( !cg.snap ) {
		return;
	}

	if ( CG_GetWeapReticleZoom( &zoom ) ) {
		if ( !cg_hudDamageIconInScope.integer ) {
			return;
		}
		CG_CalcCrosshairPosition( &x, &y );
		x += 320;
		y += 240;
	} else {
		x = 320;
		y = 240;
	}

	w = cg_hudDamageIconWidth.value * 0.5f;
	verts[0] = verts[6] = -w;
	verts[2] = verts[4] = w;
	verts[1] = verts[3] = cg_hudDamageIconOffset.value;
	verts[5] = verts[7] = cg_hudDamageIconHeight.value + cg_hudDamageIconOffset.value;

	for ( i = 0; i < 8; i++ ) {
		duration = cg.damageIndicators[i][1];
		t = cg.time - cg.damageIndicators[i][0];
		if ( t <= 0 || t >= duration ) {
			continue;
		}

		// keep: zoom is reused for the yaw
		zoom = vectoyaw( cg.refdef.viewaxis[0] ) - *(float *)&cg.damageIndicators[i][2];

		alpha = 2.0f - (rfloat)t * 2.0f / duration;
		if ( 1.0f < alpha ) {
			color[3] = 1.0f;
		} else {
			color[3] = alpha;
		}

		trap_R_SetColor( color );
		CG_DrawRotatedQuadPic( x, y, verts, cg_damageIconTexCoords, zoom,
							   cgs.media.hudHitDirection );
	}
}

/*
==============
CG_DrawTimedMenus
==============
*/
static void CG_DrawTimedMenus( void ) {
	void *menu;

	if ( cg.voicetime && cg.time - cg.voicetime > 2500 ) {
		menu = Menus_FindByName( "voiceMenu" );
		if ( menu ) {
			Menus_Close( menu );
		}
		trap_Cvar_Set( "cl_conXOffset", "0" );
		cg.voicetime = 0;
	}
}

/*
==============
CG_Fade

The script fade is alpha only: r, g and b are unused
==============
*/
void CG_Fade( int r, int g, int b, int a, int startTime, int duration ) {
	cgs.scriptFade[0] = (float)a * ( 1.0f / 255.0f );
	cgs.scriptFadeStartTime = startTime;
	cgs.scriptFadeDuration = duration;

	if ( startTime + duration <= cg.time ) {
		cgs.scriptFade[1] = cgs.scriptFade[0];
	}
}

/*
==============
CG_ScreenFade
==============
*/
static void CG_ScreenFade( void ) {
	int msec;
	int i;
	float t, invt;
	vec4_t color;

	if ( !cg.fadeRate ) {
		return;
	}

	msec = cg.fadeTime - cg.time;
	if ( msec <= 0 ) {
		cg.fadeFrom[0] = cg.fadeTo[0];
		cg.fadeFrom[1] = cg.fadeTo[1];
		cg.fadeFrom[2] = cg.fadeTo[2];
		cg.fadeFrom[3] = cg.fadeTo[3];

		if ( !cg.fadeTo[3] ) {
			cg.fadeRate = 0;
			return;
		}

		CG_FillRect( 0, 0, 640, 480, cg.fadeFrom );
	} else {
		t = (float)msec * cg.fadeRate;
		invt = 1.0f - t;

		for ( i = 0; i < 4; i++ ) {
			color[i] = cg.fadeTo[i] * invt + cg.fadeFrom[i] * t;
		}

		if ( color[3] ) {
			CG_FillRect( 0, 0, 640, 480, color );
		}
	}
}

/*
==============
CG_DrawSoundOverlay

cg_drawSoundOverlay: one line per mixer channel
==============
*/
typedef struct {
	const char *name;
	float vol;
	float rvol;
	int hz;
	float pit;
} soundOverlay_t;

void CG_DrawSoundOverlay( void ) {
	soundOverlay_t chans[64];
	int cpu;
	int count;
	int i;
	float y;
	char provider[1024];
	char buf[1024];
	int bits, khz, stereo;
	char *s;

	count = trap_MSS_GetSoundOverlay( cg_drawSoundOverlay.integer, chans, 64, &cpu );
	if ( count <= 0 ) {
		return;
	}

	trap_Cvar_VariableStringBuffer( "mss_3d_provider", provider, sizeof( provider ) );
	trap_Cvar_VariableStringBuffer( "mss_bits", buf, sizeof( buf ) );
	bits = atoi( buf );
	trap_Cvar_VariableStringBuffer( "mss_khz", buf, sizeof( buf ) );
	khz = atoi( buf );
	trap_Cvar_VariableStringBuffer( "mss_stereo", buf, sizeof( buf ) );
	stereo = atoi( buf );

	s = va( "CPU: ^3%%%i ^73D provider: ^3%s ^7bits: ^3%i ^7kHz: ^3%i ^7chan: ^3%i",
			cpu, provider, bits, khz, ( stereo != 0 ) + 1 );
	CG_DrawStringExt( 2, 82, s, colorWhite, qfalse, qtrue, 8, 16, 0, qfalse );

	y = 98;
	for ( i = 0; i < count; i++ ) {
		if ( !chans[i].name ) {
			s = va( "%2i", i );
		} else {
			s = va( "%2i %-50s vol:^3%04.2f ^7rvol:^3%04.2f ^7pit:^3%04.2f ^7hz:^3%5i",
					i, chans[i].name, chans[i].vol, chans[i].rvol, chans[i].pit, chans[i].hz );
		}
		CG_DrawStringExt( 2, y, s, colorWhite, qfalse, qtrue, 8, 16, 0, qfalse );
		y += 16;
	}
}

/*
==============
CG_DrawGameMessages

The console's notify lines, under the compass
==============
*/
static void CG_DrawGameMessages( void ) {
	float x, y;
	float alpha;
	float   *color;

	x = 6;
	y = 345.0f - ( cg_hudCompassSize.value - 1.0f ) * 160.0f + 12.0f;

	if ( cg.showScores ) {
		color = CG_FadeColor( cg.scoreFadeTime, 100 );
		if ( !color ) {
			return;
		}
		alpha = cg_hudAlpha.value * color[3];
	} else {
		alpha = cg_hudAlpha.value;
	}

	trap_DrawNotifyLines( Q_ftol( x ), Q_ftol( y ), alpha, 2 );
}

/*
==============
CG_DrawBoldGameMessages
==============
*/
static void CG_DrawBoldGameMessages( void ) {
	float x, y;
	float alpha;
	float   *color;

	x = 320;
	y = 180;

	if ( cg.showScores ) {
		color = CG_FadeColor( cg.scoreFadeTime, 100 );
		if ( !color ) {
			return;
		}
		alpha = cg_hudAlpha.value * color[3];
	} else {
		alpha = cg_hudAlpha.value;
	}

	trap_DrawBoldMessages( Q_ftol( x ), Q_ftol( y ), alpha, 3 );
}

/*
==============
CG_DrawMiniConsole
==============
*/
static void CG_DrawMiniConsole( void ) {
	float x, y;
	float alpha;

	if ( con_minicon.integer < 0 ) {
		return;
	}
	if ( !cg_developer.integer && !con_minicon.integer ) {
		return;
	}

	x = 2;
	alpha = cg_hudAlpha.value;
	y = 4;
	trap_DrawMiniConsole( Q_ftol( x ), Q_ftol( y ), alpha );
}

/*
==============
CG_DrawSubtitles
==============
*/
static void CG_DrawSubtitles( void ) {
	float x, y;

	if ( !cg_subtitles.integer ) {
		return;
	}

	x = 135;
	y = 425;
	trap_DrawSubtitles( Q_ftol( x ), Q_ftol( y ), cg_hudAlpha.value, 2 );
}

/*
==============
CG_DrawSay
==============
*/
static void CG_DrawSay( void ) {
	trap_DrawSay( 100 );
}

/*
==============
CG_DrawPerformanceWarnings

StatMon's warning icons, two per row down the left side
==============
*/
typedef struct {
	int endTime;
	qhandle_t shader;
} statMonWarning_t;

static void CG_DrawPerformanceWarnings( void ) {
	int now;
	statMonWarning_t *warnings;
	int count;
	int i;
	float x, y;

	now = trap_Milliseconds();
	trap_StatMon_GetStatsArray( &warnings, &count );

	x = 2;
	y = 200;

	for ( i = 0; i < count; i++ ) {
		if ( warnings[i].endTime >= now ) {
			CG_DrawPic( x, y, 32, 32, warnings[i].shader );
		}
		x += 34;
		if ( x + 32 > 68 ) {
			x = 2;
			y += 34;
		}
	}
}

/*
==============
CG_DrawShader

cg_drawShader: describe the shader under the crosshair
==============
*/
static void CG_DrawShader( void ) {
	char header[64];
	char line1[0x1000];
	char line2[0x1000];

	if ( !trap_R_PickShader( cg.refdef.vieworg, cg.refdef.viewaxis[0],
								  header, line1, line2, sizeof( line1 ) ) ) {
		return;
	}

	CG_DrawSmallString( 8, 240, header, 1.0f );
	CG_DrawSmallString( 8, 256, line1, 1.0f );
	CG_DrawSmallString( 8, 272, line2, 1.0f );
}

/*
==============
CG_DrawDebugOverlays

Three mutually exclusive debug pages
==============
*/
static void CG_DrawDebugOverlays( void ) {
	if ( cg_drawSoundOverlay.integer ) {
		CG_DrawSoundOverlay();
	} else if ( cg_drawScriptUsage.integer ) {
		CG_DrawScriptUsage();
	} else if ( cg_drawShader.integer ) {
		CG_DrawShader();
	}
}

/*
==============
CG_UpdateCameraShake

Recompute one shake's weight for this frame
==============
*/
static qboolean CG_UpdateCameraShake( cameraShake_t *shake ) {
	int msec;
	float scale;
	rfloat amplitude;   // keep: rfloat matches original codegen

	msec = cg.time - shake->startTime;
	if ( msec < 0 || (rfloat)msec >= shake->duration ) {
		return qfalse;
	}

	scale = 1.0f - FCAST( Distance( shake->origin, cg.refdef.vieworg ) ) / shake->radius;
	amplitude = ( 1.0f - msec / shake->duration ) * shake->amplitude;

	if ( scale >= 0 ) {
		shake->weight = scale * amplitude;
	} else {
		shake->weight = scale / amplitude;
	}
	shake->frameAmplitude = amplitude;

	return qtrue;
}

/*
==============
CG_StartShakeCamera

Take the first free or expired slot; if all four are busy, replace the
weakest, and if this one is weaker than all of them, drop it
==============
*/
void CG_StartShakeCamera( float amplitude, int duration, float radius, const vec3_t origin ) {
	cameraShake_t shake;
	int i;
	float weakest;
	int time;       // keep: rfloat casts and the time copy match original codegen

	if ( amplitude <= 0 ) {
		return;
	}

	time = cg.time;
	shake.startTime = time;
	shake.amplitude = amplitude;
	shake.duration = (float)duration;
	shake.origin[0] = origin[0];
	shake.origin[1] = origin[1];
	shake.origin[2] = origin[2];
	shake.radius = radius;

	CG_UpdateCameraShake( &shake );

	for ( i = 0; i < 4; i++ ) {
		if ( cg.cameraShakes[i].startTime > time
			 || (rfloat)cg.cameraShakes[i].startTime + cg.cameraShakes[i].duration
			 <= (rfloat)cg.time ) {
			memcpy( &cg.cameraShakes[i], &shake, sizeof( shake ) );
			return;
		}
	}

	weakest = shake.weight;
	if ( cg.cameraShakes[0].weight < weakest ) {
		i = 0;
		weakest = cg.cameraShakes[0].weight;
	}
	if ( cg.cameraShakes[1].weight < weakest ) {
		i = 1;
		weakest = cg.cameraShakes[1].weight;
	}
	if ( cg.cameraShakes[2].weight < weakest ) {
		i = 2;
		weakest = cg.cameraShakes[2].weight;
	}
	if ( cg.cameraShakes[3].weight < weakest ) {
		i = 3;
	} else if ( i == 4 ) {
		return;
	}

	memcpy( &cg.cameraShakes[i], &shake, sizeof( shake ) );
}

/*
==============
CG_ShakeCamera

cg.shakePhase is the sine phase and cg.shakeMinAmplitude an always-on floor
(keep: the rfloat casts and FCASTs match original codegen)
==============
*/
void CG_ShakeCamera( void ) {
	float weight, amplitude;
	float t;
	float r;
	int i;

	amplitude = 0;
	weight = 0;
	t = (rfloat)cg.time * ( 1.0f / 600.0f );

	for ( i = 0; i < 4; i++ ) {
		if ( CG_UpdateCameraShake( &cg.cameraShakes[i] )
			 && cg.cameraShakes[i].weight > weight ) {
			weight = cg.cameraShakes[i].weight;
			amplitude = cg.cameraShakes[i].frameAmplitude;
		}
	}

	if ( cg.shakeMinAmplitude > weight ) {
		weight = cg.shakeMinAmplitude;
		amplitude = cg.shakeMinAmplitude;
	}

	if ( weight <= 0 ) {
		// crandom() * M_PI, from a single rand()
		r = rand() / 32768.0f;
		cg.shakePhase = ( r + r - 1.0f ) * (float)M_PI;
		return;
	}

	if ( weight > 1.0f ) {
		weight = 1.0f;
	}

	// up/down, left/right, roll
	cg.refdefViewAngles[PITCH] += FCAST( sin( (rfloat)t * 25.132742f + cg.shakePhase ) ) * weight * amplitude * 18.0f;
	cg.refdefViewAngles[YAW] += FCAST( sin( (rfloat)t * 47.12389f + cg.shakePhase ) ) * weight * amplitude * 16.0f;
	cg.refdefViewAngles[ROLL] += FCAST( sin( (rfloat)t * 37.699112f + cg.shakePhase ) ) * weight * amplitude * 10.0f;
}

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator( void ) {
	const char *s;
	int w;
	vec4_t color = { 1, 1, 1, 1 };
	float x;

	s = CG_SafeTranslateString( "CGAME_SPECTATOR" );
	w = trap_R_Text_Width( s, 0, 16 / 48.0f, 0 );
	x = ( 640.0f - w ) * 0.5f;
	trap_R_Text_Paint( x, 458, 4, 16 / 48.0f, color, s, 0, 0, 3 );
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote( void ) {
	const char *s;
	const char *yesStr, *noStr;
	int sec;
	vec4_t color = { 1, 1, 0, 1 };
	char *binding;
	char yesKey[256];
	char noKey[256];

	if ( cgs.complaintEndTime > cg.time || cgs.voteTime ) {
		if ( GetKeyBindingLocalizedString( "vote yes", &binding ) ) {
			Q_strncpyz( yesKey, binding, sizeof( yesKey ) );
		} else {
			Q_strncpyz( yesKey, "vote yes", sizeof( yesKey ) );
		}

		if ( GetKeyBindingLocalizedString( "vote no", &binding ) ) {
			Q_strncpyz( noKey, binding, sizeof( noKey ) );
		} else {
			Q_strncpyz( noKey, "vote no", sizeof( noKey ) );
		}
	}

	if ( cgs.complaintEndTime > cg.time ) {
		// a teamkill complaint
		switch ( cgs.complaintClient ) {
		case -1:
			s = CG_SafeTranslateString( "CGAME_COMPLAINTFILED" );
			break;
		case -2:
			s = CG_SafeTranslateString( "CGAME_COMPLAINTDISMISSED" );
			break;
		case -3:
			s = CG_SafeTranslateString( "CGAME_COMPLAINTSERVERHOST" );
			break;
		case -4:
			s = CG_SafeTranslateString( "CGAME_SERVERHOSTTEAMKILLED" );
			break;
		default:
			s = NULL;
			break;
		}

		if ( s ) {
			trap_R_Text_Paint( 8, 200, 0, 10 / 48.0f, color, s, 0, 0, 3 );
			return;
		}

		if ( !bg_clientinfo[cgs.complaintClient].infoValid ) {
			return;
		}

		s = CG_SafeTranslateString( "CGAME_COMPLAINTTEAMKILLFILE" );
		s = va( s, bg_clientinfo[cgs.complaintClient].name );
		trap_R_Text_Paint( 8, 200, 0, 10 / 48.0f, color, s, 0, 0, 3 );

		s = va( CG_SafeTranslateString( "CGAME_PRESSYESNO" ), yesKey, noKey );
		trap_R_Text_Paint( 8, 210, 0, 10 / 48.0f, color, s, 0, 0, 3 );
		return;
	}

	if ( !cgs.voteTime ) {
		return;
	}

	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		CG_PlayClientSoundAliasByName( (const char *)cgs.media.talkSound );
	}

	sec = ( cgs.voteTime - cg.time ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

	if ( cg.snap->ps.eFlags & 0x20000 ) {
		s = va( "%s(%i):%s", CG_SafeTranslateString( "CGAME_VOTE" ), sec, cgs.voteString );
		trap_R_Text_Paint( 8, 200, 0, 10 / 48.0f, color, s, 0, 0, 3 );

		noStr = CG_SafeTranslateString( "CGAME_NO" );
		yesStr = CG_SafeTranslateString( "CGAME_YES" );
		s = va( "%s:%i, %s:%i", yesStr, cgs.voteYes, noStr, cgs.voteNo );
		trap_R_Text_Paint( 8, 210, 0, 10 / 48.0f, color, s, 0, 0, 3 );
	} else {
		s = va( "%s(%i):%s", CG_SafeTranslateString( "CGAME_VOTE" ), sec, cgs.voteString );
		trap_R_Text_Paint( 8, 200, 0, 10 / 48.0f, color, s, 0, 0, 3 );

		noStr = CG_SafeTranslateString( "CGAME_NO" );
		yesStr = CG_SafeTranslateString( "CGAME_YES" );
		s = va( "%s(%s):%i, %s(%s):%i", yesStr, yesKey, cgs.voteYes, noStr, noKey, cgs.voteNo );
		trap_R_Text_Paint( 8, 210, 0, 10 / 48.0f, color, s, 0, 0, 3 );
	}
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	trap_UI_CloseAllMenus();
	cg.scoreFadeTime = cg.time;
	CG_DrawScoreboard();
}

/*
=================
CG_DrawSpectatorMessage
=================
*/
static void CG_DrawSpectatorMessage( void ) {
	const char *str;
	char *binding;
	vec4_t color = { 1, 1, 1, 1 };

	if ( !cg_descriptiveText.integer ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & 0x60000 ) {
		return;
	}

	Controls_GetConfig();

	if ( !GetKeyBindingLocalizedString( "+attack", &binding ) ) {
		binding = (char *)"+attack";
	}
	str = CG_SafeTranslateString( "CGAME_FOLLOWNEXTPLAYER" );
	trap_R_Text_Paint( 240, 426, 0, 10 / 48.0f, color,
					   va( str, binding ), 0, 0, 3 );

	if ( !GetKeyBindingLocalizedString( "+melee", &binding ) ) {
		binding = (char *)"+melee";
	}
	str = CG_SafeTranslateString( "CGAME_FOLLOWPREVIOUSPLAYER" );
	trap_R_Text_Paint( 240, 436, 0, 10 / 48.0f, color,
					   va( str, binding ), 0, 0, 3 );

	if ( !( cg.snap->ps.pm_flags & 0x10000 ) ) {
		return;
	}
	if ( !GetKeyBindingLocalizedString( "toggle cl_run", &binding )
		 && !GetKeyBindingLocalizedString( "+speed", &binding ) ) {
		return;
	}
	str = CG_SafeTranslateString( "CGAME_FOLLOWSTOP" );
	str = va( str, binding );
	trap_R_Text_Paint( 240, 446, 0, 10 / 48.0f, color, str, 0, 0, 3 );
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	const char *s;
	int w;
	vec4_t color = { 1, 1, 1, 1 };
	float x;

	if ( !( cg.snap->ps.pm_flags & 0x10000 ) ) {
		return qfalse;
	}

	if ( bg_clientinfo[cg.snap->ps.clientNum].infoValid ) {
		s = trap_SE_LocalizeMessage( va( "CGAME_FOLLOWING\x15: %s", bg_clientinfo[cg.snap->ps.clientNum].name ),
									 "spectator follow string" );
	} else {
		s = trap_SE_LocalizeMessage( va( "CGAME_FOLLOWING\x15: %s", "?" ), "spectator follow string" );
	}

	w = trap_R_Text_Width( s, 0, 16 / 48.0f, 0 );
	x = ( 640 - w ) * 0.5f;
	trap_R_Text_Paint( x, 414, 0, 16 / 48.0f, color, s, 0, 0, 3 );

	return qtrue;
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void ) {
	if ( cg.levelShot ) {
		return;
	}
	if ( cg.cubemapShot ) {
		return;
	}

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.cameraMode || !cg_draw2D.integer ) {
		CG_DrawFlashFade();
		return;
	}

	CG_ScreenFade();
	CG_DrawFlashDamage();
	CG_DrawDamageDirectionIndicators();

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	if ( cg.snap->ps.pm_type == PM_SPECTATOR ) {
		CG_DrawSpectator();
		CG_DrawCrosshairNames();
		CG_DrawChatMessages();
		if ( cg_drawStatus.integer ) {
			CG_DrawHudElems();
		}
	} else {
		if ( cg.snap->ps.pm_type < PM_DEAD ) {
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();
		}
		CG_DrawChatMessages();
		if ( cg_drawStatus.integer ) {
			Menu_PaintAll();
			CG_DrawTimedMenus();
			CG_DrawHudElems();
		}
	}

	if ( !CG_DrawScoreboard() ) {
		CG_DrawSpectatorMessage();
		CG_DrawFollow();
	}

	CG_DrawVote();
	CG_DrawLagometer();
	CG_DrawPerformanceWarnings();
	CG_DrawDebugOverlays();
	CG_DrawUpperRight();

	if ( !CG_DrawScoreboard() ) {
		CG_DrawCenterString();
		CG_DrawGameMessages();
		CG_DrawBoldGameMessages();
		CG_DrawMiniConsole();
		CG_DrawSubtitles();
		CG_DrawSay();
	}

	CG_DrawFlashFade();
}

/*
=====================
CG_DrawSavedScreenBlend
=====================
*/
void CG_DrawSavedScreenBlend( void ) {
	CG_DrawShellShockSavedScreenBlend( cg.shellshock.parms, cg.shellshock.startTime,
									   cg.shellshock.duration );
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( int stereoView ) {
	float separation;
	vec3_t baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation( 0 );
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value * 0.5f;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value * 0.5f;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
		break;
	}

	// offset vieworg appropriately if we're doing stereo separation
	baseOrg[0] = cg.refdef.vieworg[0];
	baseOrg[1] = cg.refdef.vieworg[1];
	baseOrg[2] = cg.refdef.vieworg[2];
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	cg.refdef.rdflags |= 0x10;
	if ( !cg_skybox.integer ) {
		cg.refdef.rdflags &= ~0x10;
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		cg.refdef.vieworg[0] = baseOrg[0];
		cg.refdef.vieworg[1] = baseOrg[1];
		cg.refdef.vieworg[2] = baseOrg[2];
	}

	CG_DrawShellShockSavedScreenBlend( cg.shellshock.parms, cg.shellshock.startTime,
									   cg.shellshock.duration );

	// clear around the rendered view if sized down
	CG_TileClear();

	// draw status bar and other floating elements
	CG_Draw2D();
}
