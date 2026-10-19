// cg_scoreboard -- draw the scoreboard on top of the game screen
// some redundant-looking parentheses are kept: they match the original codegen

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cg_local.h"

// team_t from g_local.h
#ifndef TEAM_SPECTATOR
#define TEAM_FREE       0
#define TEAM_AXIS       1
#define TEAM_ALLIES     2
#define TEAM_SPECTATOR  3
#endif

// list layout, in 640x480 virtual coordinates
#define SB_LIST_X               129.0f  // left edge of a row
#define SB_LIST_BOTTOM          432.0f  // a row that would cross this is clipped
#define SB_ROW_HEIGHT           12.0f
#define SB_BANNER_HEIGHT        28.0f   // list height counted per banner
#define SB_TEXT_BASELINE        10.5f
#define SB_HEADER_HEIGHT        14.0f
#define SB_TEXT_SCALE           0.22f
#define SB_HEADER_SCALE         0.30f
#define SB_BANNER_SCALE         0.32f
#define SB_STATUSICON_SIZE      13.0f

// cg_scoreboardColumns[].align
#define SB_ALIGN_LEFT           0
#define SB_ALIGN_RIGHT          2

// cg_scoreboardColumns[].id
#define SB_FIELD_NAME           0
#define SB_FIELD_SCORE          1
#define SB_FIELD_DEATHS         2
#define SB_FIELD_PING           3
#define SB_FIELD_STATUSICON     4

typedef struct scoreboardColumn_s {
	int         id;
	float       widthFrac;              // fraction of the list width
	const char *label;                  // localized header, "" for none
	int         align;
} scoreboardColumn_t;

#define SB_NUM_COLUMNS      5

static const scoreboardColumn_t cg_scoreboardColumns[SB_NUM_COLUMNS] = {
	{ SB_FIELD_STATUSICON,  0.05f,  "",                 SB_ALIGN_LEFT  },
	{ SB_FIELD_NAME,        0.41f,  "",                 SB_ALIGN_LEFT  },
	{ SB_FIELD_SCORE,       0.16f,  "CGAME_SB_SCORE",   SB_ALIGN_RIGHT },
	{ SB_FIELD_DEATHS,      0.20f,  "CGAME_SB_DEATHS",  SB_ALIGN_RIGHT },
	{ SB_FIELD_PING,        0.18f,  "CGAME_SB_PING",    SB_ALIGN_RIGHT }
};

// { x, y, w, h } of the frame and its two separators
static const float cg_scoreboardFrame[6][4] = {
	{ 123.0f,  25.0f, 394.0f,   2.0f },
	{ 123.0f, 447.0f, 394.0f,   2.0f },
	{ 123.0f,  27.0f,   2.0f, 420.0f },
	{ 515.0f,  27.0f,   2.0f, 420.0f },
	{ 125.0f,  51.0f, 390.0f,   1.0f },
	{ 125.0f, 432.0f, 390.0f,   1.0f }
};

// keep: a const object, not a #define -- matches original data
static const int cg_scoreboardFrameCount = sizeof( cg_scoreboardFrame ) / sizeof( cg_scoreboardFrame[0] );

// which team was drawn first the last time the scores were tied
static int cg_scoreboardFirstTeam = TEAM_ALLIES;

void CG_DrawScoreboard_GetTeamColor( vec4_t color, int team );

/*
=================
CG_DrawScoreboard_Backdrop

The dimmed panel, its frame, and the four corner captions: gametype and map
along the top, hostname and server address along the bottom.  Both caption
pairs shrink their font until they fit.
=================
*/
static void CG_DrawScoreboard_Backdrop( float fade ) {
	vec4_t      color;
	qhandle_t   hShader;
	char        mapname[64];
	char        *p;
	const char  *gametype;
	const char  *address;
	float       scale;
	float       x;
	float       y;
	int         height;
	int         i;

	color[0] = 1.0f;
	color[1] = 1.0f;
	color[2] = 1.0f;
	color[3] = fade * 0.55f;
	trap_R_SetColor( color );

	hShader = trap_R_RegisterShaderNoMip( "black", 5 );
	CG_DrawPic( 120.0f, 22.0f, 400.0f, 430.0f, hShader );

	color[3] = fade * 0.1f;
	trap_R_SetColor( color );
	hShader = trap_R_RegisterShaderNoMip( "white", 5 );

	for ( i = 0; i < cg_scoreboardFrameCount; i++ ) {
		CG_DrawPic( cg_scoreboardFrame[i][0], cg_scoreboardFrame[i][1],
					cg_scoreboardFrame[i][2], cg_scoreboardFrame[i][3], hShader );
	}

	// the captions draw at the full fade
	color[3] = fade;

	gametype = trap_SE_LocalizeMessage(
				   trap_UI_GetGameTypeDisplayName( cgs.gametype ),
				   "scoreboard gametype display" );

	height = trap_R_Text_Height( 0, 0.41f );
	trap_R_Text_Paint( SB_LIST_X, ( 51.0f - ( ( 24 - height ) * 0.5f ) ), 0, 0.41f,
					   color, gametype, 0.0f, 0, 3 );

	strcpy( mapname, cgs.mapname );

	p = mapname + strlen( mapname ) - 4;
	if ( !Q_stricmpn( p, ".bsp", 4 ) ) {
		*p = 0;
	}

	p = mapname;
	if ( !Q_stricmpn( mapname, "maps", 4 ) && ( mapname[4] == '/' || mapname[4] == '\\' ) ) {
		p = mapname + 5;
	}
	// Q_stricmpn( p, "mp", 2 ), open-coded
	// keep: matches original codegen (s2 is set up before s1)
	{
		const char  *s1;
		const char  *s2;
		int         c1, c2;
		int         n;
		int         cmp;

		s2 = "mp";
		s1 = p;
		n = 2;
		cmp = 0;
		do {
			c1 = *s1++;
			c2 = *s2++;

			if ( !n-- ) {
				cmp = 0;
				break;
			}

			if ( c1 != c2 ) {
				if ( Q_islower( c1 ) ) {
					c1 -= ( 'a' - 'A' );
				}
				if ( Q_islower( c2 ) ) {
					c2 -= ( 'a' - 'A' );
				}
				if ( c1 != c2 ) {
					cmp = c1 < c2 ? -1 : 1;
					break;
				}
			}
		} while ( c1 );

		if ( !cmp && ( p[2] == '/' || p[2] == '\\' ) ) {
			p += 3;
		}
	}

	// only the map name shrinks; the gametype is always measured at 0.41
	scale = 0.41f;
	while ( trap_R_Text_Width( gametype, 0, 0.41f, 0 )
			+ trap_R_Text_Width( p, 0, scale, 0 ) + 4 > 386
			&& scale > 0.075f ) {
		scale -= 0.025f;
	}

	x = 511.0f - ( trap_R_Text_Width( p, 0, scale, 0 ) + 4 );
	height = trap_R_Text_Height( 0, scale );
	trap_R_Text_Paint( x, 51.0f - ( 24 - height ) * 0.5f, 0, scale,
					   color, p, 0.0f, 0, 3 );

	address = trap_CL_GetServerIPAddress();
	if ( address && !Q_stricmpn( address, "0.0.0.0:0", 99999 ) ) {
		address = CG_SafeTranslateString( "CGAME_LISTENSERVER" );
	}

	scale = 0.2f;
	while ( trap_R_Text_Width( cgs.sv_hostname, 0, scale, 0 )
			+ trap_R_Text_Width( address, 0, scale, 0 ) + 4 > 386
			&& scale > 0.075f ) {
		scale -= 0.01f;
	}

	height = trap_R_Text_Height( 0, scale );
	y = 447.0f - ( ( 14 - height ) * 0.5f );
	trap_R_Text_Paint( SB_LIST_X, y, 0, scale, color, cgs.sv_hostname, 0.0f, 0, 3 );

	x = 511.0f - ( trap_R_Text_Width( address, 0, scale, 0 ) + 4 );
	trap_R_Text_Paint( x, y, 0, scale, color, address, 0.0f, 0, 3 );
}

/*
=================
CG_DrawObjectiveInfo

The word-wrapped cg_objectiveText block above the list, then the separator
under it.  Returns the y the score list starts at.
=================
*/
static float CG_DrawObjectiveInfo( const float *color, float y ) {
	const char  *line;          // start of the line being measured
	const char  *cursor;
	const char  *lastSpace;
	vec4_t      barColor;
	qhandle_t   hShader;
	float       barWidth;
	float       barHeight;

	if ( cg_objectiveText.string[0] ) {
		y += 4.0f;

		line = CG_TranslateMessage( cg_objectiveText.string, "scoreboard objective info" );
		lastSpace = NULL;
		cursor = line;

		while ( line && cursor ) {
			if ( *line == ' ' ) {
				line++;
			} else if ( *line == '\n' || ( *line == '\\' && line[1] == 'n' ) ) {
				line = ( *line == '\n' ) ? line + 1 : line + 2;
				y += SB_ROW_HEIGHT;
			} else {
				char c;

				c = cursor[1];
				cursor++;

				if ( !c ) {
					trap_R_Text_Paint( SB_LIST_X, y + 9.0f, 0, 0.24f, color, line,
									   0.0f, cursor - line, 3 );
					y += SB_ROW_HEIGHT;
					break;
				}

				if ( c == '\n' || ( c == '\\' && cursor[1] == 'n' ) ) {
					trap_R_Text_Paint( SB_LIST_X, y + 9.0f, 0, 0.24f, color, line,
									   0.0f, cursor - line, 3 );
					line = ( *cursor == '\n' ) ? cursor + 1 : cursor + 2;
					y += SB_ROW_HEIGHT;
				} else if ( trap_R_Text_Width( line, 0, 0.24f, cursor - line ) > 374 ) {
					// a NULL lastSpace gives a negative limit; keep as is
					trap_R_Text_Paint( SB_LIST_X, y + 9.0f, 0, 0.24f, color, line,
									   0.0f, lastSpace - line, 3 );
					line = lastSpace ? lastSpace + 1 : cursor - 1;
					y += SB_ROW_HEIGHT;
				} else {
					if ( *cursor == ' ' ) {
						lastSpace = cursor;
					}
					continue;
				}
			}

			cursor = line;
			lastSpace = NULL;
		}

		y += 2.0f;

		hShader = trap_R_RegisterShaderNoMip( "white", 5 );
		barColor[0] = color[0];
		barColor[1] = color[1];
		barColor[2] = color[2];
		barColor[3] = color[3] * 0.1f;
		trap_R_SetColor( barColor );
		barWidth = 390.0f;          // keep: locals match original codegen
		barHeight = 1.0f;
		CG_DrawPic( 125.0f, y, barWidth, barHeight, hShader );

		return y + 1.0f;
	}

	return y;
}

/*
=================
CG_DrawScoreboard_ListColumnHeaders
=================
*/
static float CG_DrawScoreboard_ListColumnHeaders( float y, float width, const float *color ) {
	const char  *label;
	float       x;
	float       offset;
	int         i;

	x = SB_LIST_X;

	for ( i = 0; i < sizeof( cg_scoreboardColumns ) / sizeof( cg_scoreboardColumns[0] ); i++ ) {
		if ( cg_scoreboardColumns[i].label[0] ) {
			label = CG_SafeTranslateString( cg_scoreboardColumns[i].label );

			if ( cg_scoreboardColumns[i].align == SB_ALIGN_RIGHT ) {
				offset = ( width * cg_scoreboardColumns[i].widthFrac )
						 - trap_R_Text_Width( label, 0, SB_HEADER_SCALE, 0 );
			} else {
				offset = 0.0f;
			}

			trap_R_Text_Paint( x + offset, y + SB_TEXT_BASELINE, 0, SB_HEADER_SCALE,
							   color, label, 0.0f, 0, 3 );
		}

		x += width * cg_scoreboardColumns[i].widthFrac;
	}

	return y + SB_HEADER_HEIGHT;
}

/*
=================
CG_DrawScoreboard_ListHeight

How tall the whole list wants to be, and how many scrollable lines it has.
=================
*/
static float CG_DrawScoreboard_ListHeight( int *numLines ) {
	float   height;
	int     i;

	height = 10.0f;
	*numLines = 0;

	if ( cg.teamPlayers[TEAM_AXIS] || cg.teamPlayers[TEAM_ALLIES] ) {
		*numLines = 2;
		height = 66.0f;
	}
	if ( cg.teamPlayers[TEAM_FREE] ) {
		height += SB_BANNER_HEIGHT;
		( *numLines )++;
	}
	if ( cg.teamPlayers[TEAM_SPECTATOR] ) {
		height += SB_BANNER_HEIGHT;
		( *numLines )++;
	}

	for ( i = 0; i < cg.numScores; i++ ) {
		height += SB_ROW_HEIGHT;
		( *numLines )++;
	}

	return height;
}

/*
=================
CG_CheckDrawScoreboardLine

Advance past the scrolled-off lines and stop once the list runs off the
bottom.
=================
*/
static qboolean CG_CheckDrawScoreboardLine( int *lineIndex, float y, float height ) {
	if ( cg.scoreboardListFull ) {
		return qfalse;
	}

	if ( *lineIndex < cg.scoreboardScrollPos ) {
		( *lineIndex )++;
		return qfalse;
	}

	if ( y + height > SB_LIST_BOTTOM ) {
		cg.scoreboardListFull = qtrue;
		return qfalse;
	}

	( *lineIndex )++;
	return qtrue;
}

/*
=================
CG_DrawScoreboard_GetTeamColor

g_TeamColor_Axis / g_TeamColor_Allies are "r g b" strings; anything else is
white.  Only the first three components are written -- every caller has
already put the fade into color[3].
=================
*/
void CG_DrawScoreboard_GetTeamColor( vec4_t color, int team ) {
	char buffer[1024];
	int  i;

	switch ( team ) {
	case TEAM_AXIS:
		trap_Cvar_VariableStringBuffer( "g_TeamColor_Axis", buffer, sizeof( buffer ) );
		sscanf( buffer, "%f %f %f", &color[0], &color[1], &color[2] );
		break;
	case TEAM_ALLIES:
		trap_Cvar_VariableStringBuffer( "g_TeamColor_Allies", buffer, sizeof( buffer ) );
		sscanf( buffer, "%f %f %f", &color[0], &color[1], &color[2] );
		break;
	default:
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 1.0f;
		break;
	}

	for ( i = 0; i < 3; i++ ) {
		color[i] = color[i] < 0.0f ? 0.0f : ( color[i] > 1.0f ? 1.0f : color[i] );
	}
}

/*
=================
CG_DrawScoreboard_ListBanner

One team's header row: the banner material, the team name with its player
count, and -- for the two real teams -- the team score and average ping under
the matching columns.
=================
*/
static float CG_DrawScoreboard_ListBanner( const float *color, float y, float width,
										   float height, int team, int *lineIndex ) {
	char        bannerShader[64];
	char        teamName[64];
	vec4_t      drawColor;
	const char  *title;
	const char  *value;
	qhandle_t   hShader;
	float       textY;
	float       x;
	float       offset;
	int         i;

	if ( !CG_CheckDrawScoreboardLine( lineIndex, y, 24.0f ) ) {
		return y;
	}

	drawColor[0] = 1.0f;
	drawColor[1] = 1.0f;
	drawColor[2] = 1.0f;
	drawColor[3] = color[3];

	if ( cg.teamPlayers[team] == 1 ) {
		title = trap_SE_LocalizeMessage(
					va( "CGAME_SB_PLAYER\x15%i", cg.teamPlayers[team] ),
					"scoreboard banner text" );
	} else {
		title = trap_SE_LocalizeMessage(
					va( "CGAME_SB_PLAYERS\x15%i", cg.teamPlayers[team] ),
					"scoreboard banner text" );
	}

	switch ( team ) {
	case TEAM_FREE:
		trap_Cvar_VariableStringBuffer( "g_ScoresBanner_None", bannerShader,
										sizeof( bannerShader ) );
		break;
	case TEAM_AXIS:
		trap_Cvar_VariableStringBuffer( "g_ScoresBanner_Axis", bannerShader,
										sizeof( bannerShader ) );
		trap_Cvar_VariableStringBuffer( "g_TeamName_Axis", teamName, sizeof( teamName ) );
		title = va( "%s (%s)",
					trap_SE_LocalizeMessage( teamName, "scoreboard team name" ),
					title );
		break;
	case TEAM_ALLIES:
		trap_Cvar_VariableStringBuffer( "g_ScoresBanner_Allies", bannerShader,
										sizeof( bannerShader ) );
		trap_Cvar_VariableStringBuffer( "g_TeamName_Allies", teamName, sizeof( teamName ) );
		title = va( "%s (%s)",
					trap_SE_LocalizeMessage( teamName, "scoreboard team name" ),
					title );
		break;
	default:
		trap_Cvar_VariableStringBuffer( "g_ScoresBanner_Spectators", bannerShader,
										sizeof( bannerShader ) );
		title = va( "%s (%s)",
					trap_SE_LocalizeMessage( "CGAME_SPECTATORS",
														   "scoreboard team name" ),
					title );
		break;
	}

	hShader = trap_R_RegisterShaderNoMip( bannerShader, 5 );
	trap_R_SetColor( drawColor );
	CG_DrawPic( SB_LIST_X, y, width, height, hShader );

	CG_DrawScoreboard_GetTeamColor( drawColor, team );

	textY = y + 18.0f;
	trap_R_Text_Paint( 133.0f, textY, 0, SB_BANNER_SCALE, drawColor, title,
					   0.0f, 0, 3 );

	if ( team == TEAM_AXIS || team == TEAM_ALLIES ) {
		x = SB_LIST_X;
		for ( i = 0; i < sizeof( cg_scoreboardColumns ) / sizeof( cg_scoreboardColumns[0] ); i++ ) {
			if ( cg_scoreboardColumns[i].id == SB_FIELD_SCORE
				 || cg_scoreboardColumns[i].id == SB_FIELD_PING ) {

				value = va( "%i", cg_scoreboardColumns[i].id == SB_FIELD_SCORE
								  ? cg.teamScores[team] : cg.teamPing[team] );

				if ( cg_scoreboardColumns[i].align == SB_ALIGN_RIGHT ) {
					offset = ( width * cg_scoreboardColumns[i].widthFrac )
							 - trap_R_Text_Width( value, 0, SB_BANNER_SCALE, 0 );
				} else {
					offset = 0.0f;
				}

				trap_R_Text_Paint( x + offset, textY, 0, SB_BANNER_SCALE, drawColor,
								   value, 0.0f, 0, 3 );
			}

			x += width * cg_scoreboardColumns[i].widthFrac;
		}
	}

	return y + 24.0f;
}

/*
=================
CG_DrawClientScore

One player's row.  Every string is clipped to its column by shortening the
trap_R_Text_Paint limit until it measures short enough.
=================
*/
static float CG_DrawClientScore( const float *color, float y, const score_t *score,
								 float width, int alt, int *lineIndex ) {
	clientInfo_t    *ci;
	vec4_t          drawColor;
	vec4_t          textColor;
	qhandle_t       hShader;
	const char      *s;
	float           x;
	float           offset;
	int             limit;
	int             i;

	if ( !CG_CheckDrawScoreboardLine( lineIndex, y, SB_ROW_HEIGHT ) ) {
		return y;
	}

	ci = &bg_clientinfo[score->client];
	if ( !ci->infoValid ) {
		return y;
	}

	if ( score->client == cg.clientNum ) {
		hShader = trap_R_RegisterShaderNoMip( "white", 5 );
		drawColor[0] = color[0];
		drawColor[1] = color[1];
		drawColor[2] = color[2];
		drawColor[3] = color[3] * 0.2f;
		trap_R_SetColor( drawColor );
		CG_DrawPic( SB_LIST_X, y, width, SB_ROW_HEIGHT, hShader );
	} else if ( alt ) {
		hShader = trap_R_RegisterShaderNoMip( "black", 5 );
		drawColor[0] = color[0];
		drawColor[1] = color[1];
		drawColor[2] = color[2];
		drawColor[3] = color[3] * 0.15f;
		trap_R_SetColor( drawColor );
		CG_DrawPic( SB_LIST_X, y, width, SB_ROW_HEIGHT, hShader );
	}

	x = SB_LIST_X;

	for ( i = 0; i < sizeof( cg_scoreboardColumns ) / sizeof( cg_scoreboardColumns[0] ); i++ ) {
		if ( cg_scoreboardColumns[i].id == SB_FIELD_STATUSICON ) {
			if ( score->statusIcon ) {
				if ( cg_scoreboardColumns[i].align == SB_ALIGN_RIGHT ) {
					offset = width * cg_scoreboardColumns[i].widthFrac - SB_STATUSICON_SIZE;
				} else {
					offset = 0.0f;
				}

				drawColor[0] = 1.0f;
				drawColor[1] = 1.0f;
				drawColor[2] = 1.0f;
				drawColor[3] = color[3];
				trap_R_SetColor( drawColor );
				CG_DrawPic( x + offset, y, SB_STATUSICON_SIZE, SB_STATUSICON_SIZE,
							score->statusIcon );
			}
		} else {
			s = "";
			switch ( cg_scoreboardColumns[i].id ) {
			case SB_FIELD_NAME:
				s = ci->name;
				break;
			case SB_FIELD_SCORE:
				if ( score->team != TEAM_SPECTATOR ) {
					s = va( "%i", score->score );
				}
				break;
			case SB_FIELD_DEATHS:
				if ( score->team != TEAM_SPECTATOR ) {
					s = va( "%i", score->deaths );
				}
				break;
			case SB_FIELD_PING:
				s = va( "%i", score->ping );
				break;
			}

			if ( s[0] ) {
				limit = 0;
				if ( trap_R_Text_Width( s, 0, SB_TEXT_SCALE, 0 )
					 > width * cg_scoreboardColumns[i].widthFrac ) {
					limit = strlen( s );
					do {
						limit--;
					} while ( trap_R_Text_Width( s, 0, SB_TEXT_SCALE, limit )
							  > width * cg_scoreboardColumns[i].widthFrac );
				}

				if ( cg_scoreboardColumns[i].align == SB_ALIGN_RIGHT ) {
					offset = ( width * cg_scoreboardColumns[i].widthFrac )
							 - trap_R_Text_Width( s, 0, SB_TEXT_SCALE, limit );
				} else {
					offset = 0.0f;
				}

				textColor[0] = 1.0f;
				textColor[1] = 1.0f;
				textColor[2] = 1.0f;
				textColor[3] = color[3];
				trap_R_Text_Paint( x + offset, y + SB_TEXT_BASELINE, 0, SB_TEXT_SCALE,
								   textColor, s, 0.0f, limit, 3 );
			}
		}

		x += width * cg_scoreboardColumns[i].widthFrac;
	}

	return y + SB_ROW_HEIGHT;
}

/*
=================
CG_DrawTeamOfClientScore

Every score on one team, alternating the row shade.
=================
*/
static float CG_DrawTeamOfClientScore( const float *color, float y, int team, float width,
									   int *lineIndex ) {
	vec4_t  teamColor;
	score_t *score;
	int     alt;
	int     i;

	alt = 0;
	CG_DrawScoreboard_GetTeamColor( teamColor, team );
	teamColor[3] = color[3];

	score = cg.scores;
	for ( i = 0; i < cg.numScores; i++, score++ ) {
		if ( !bg_clientinfo[score->client].infoValid ) {
			continue;
		}
		if ( score->team != team ) {
			continue;
		}

		y = CG_DrawClientScore( teamColor, y, score, width, alt, lineIndex );
		alt ^= 1;
	}

	return y;
}

/*
=================
CG_DrawScrollbar

The track down the right edge, the thumb sized to the visible fraction, and
the two arrow / key hint pairs.
=================
*/
static void CG_DrawScrollbar( float y, int numLines, const float *color, int drawnLines ) {
	vec4_t      drawColor;
	qhandle_t   hShader;
	float       trackHeight;
	float       thumbY;
	float       thumbHeight;
	float       arrowX;
	float       arrowSize;

	drawColor[0] = color[0];
	drawColor[1] = color[1];
	drawColor[2] = color[2];

	hShader = trap_R_RegisterShaderNoMip( "black", 5 );
	drawColor[3] = color[3] * 0.5f;
	trap_R_SetColor( drawColor );

	trackHeight = SB_LIST_BOTTOM - y - 1.0f;
	CG_DrawPic( 505.0f, y, 8.0f, trackHeight, hShader );

	thumbY = y + 1.0f;
	thumbHeight = trackHeight - 2.0f;

	if ( cg.scoreboardScrollPos && numLines ) {
		thumbY += (float)cg.scoreboardScrollPos / numLines * thumbHeight;
	}

	if ( drawnLines - cg.scoreboardScrollPos > 1 && numLines > 1 ) {
		thumbHeight = (float)( drawnLines - cg.scoreboardScrollPos ) / numLines * thumbHeight;
	}

	hShader = trap_R_RegisterShaderNoMip( "white", 5 );
	drawColor[3] = color[3] * 0.25f;
	trap_R_SetColor( drawColor );
	CG_DrawPic( 506.0f, thumbY, 6.0f, thumbHeight, hShader );

	drawColor[3] = color[3];
	trap_R_SetColor( drawColor );

	arrowX = 521.0f;            // keep: locals match original codegen
	arrowSize = 16.0f;

	if ( cg.scoreboardScrollPos > 0 ) {
		CG_DrawPic( arrowX, y, arrowSize, arrowSize,
					trap_R_RegisterShaderNoMip( "hudScoreboardScroll_UpArrow", 5 ) );
		CG_DrawPic( arrowX, y + 18.0f, arrowSize, arrowSize,
					trap_R_RegisterShaderNoMip( "hudScoreboardScroll_UpKey", 5 ) );
	}

	if ( drawnLines < numLines ) {
		CG_DrawPic( arrowX, 415.0f, arrowSize, arrowSize,
					trap_R_RegisterShaderNoMip( "hudScoreboardScroll_DownArrow", 5 ) );
		CG_DrawPic( arrowX, 397.0f, arrowSize, arrowSize,
					trap_R_RegisterShaderNoMip( "hudScoreboardScroll_DownKey", 5 ) );
	}
}

/*
=================
CG_DrawScoreboard_ScoresList

The objective block, the column headers, then a banner-plus-rows block per
team.  The two real teams go in score order, and a tie keeps whichever order
the last tie produced.
=================
*/
static void CG_DrawScoreboard_ScoresList( float fade ) {
	vec4_t      color;
	float       y;
	float       headerY;
	float       width;
	float       listHeight;
	int         numLines;
	int         lineIndex;
	int         team;
	qboolean    scrollbar;

	scrollbar = qfalse;
	cg.scoreboardListFull = qfalse;

	color[0] = 1.0f;
	color[1] = 1.0f;
	color[2] = 1.0f;
	color[3] = fade;

	y = CG_DrawObjectiveInfo( color, 52.0f );

	listHeight = CG_DrawScoreboard_ListHeight( &numLines );
	if ( listHeight > SB_LIST_BOTTOM - y ) {
		width = 374.0f;
		scrollbar = qtrue;
	} else {
		width = 382.0f;
	}

	y += 4.0f;
	y = CG_DrawScoreboard_ListColumnHeaders( y, width, color );
	headerY = y;

	lineIndex = 0;

	if ( cg.teamPlayers[TEAM_AXIS] || cg.teamPlayers[TEAM_ALLIES] ) {
		if ( cg.teamScores[TEAM_AXIS] > cg.teamScores[TEAM_ALLIES] ) {
			team = TEAM_AXIS;
		} else if ( cg.teamScores[TEAM_ALLIES] > cg.teamScores[TEAM_AXIS] ) {
			team = TEAM_ALLIES;
		} else {
			team = cg_scoreboardFirstTeam;
		}
		cg_scoreboardFirstTeam = team;

		y = CG_DrawScoreboard_ListBanner( color, y, width, 32.0f, team, &lineIndex );
		y = CG_DrawTeamOfClientScore( color, y, team, width, &lineIndex );

		team = ( team == TEAM_AXIS ) ? TEAM_ALLIES : TEAM_AXIS;

		// keep: y + 4 passed directly matches original codegen
		y = CG_DrawScoreboard_ListBanner( color, y + 4.0f, width, 32.0f, team, &lineIndex );
		y = CG_DrawTeamOfClientScore( color, y, team, width, &lineIndex );
		y += 4.0f;
	}

	if ( cg.teamPlayers[TEAM_FREE] ) {
		y = CG_DrawScoreboard_ListBanner( color, y, width, 32.0f, TEAM_FREE, &lineIndex );
		y = CG_DrawTeamOfClientScore( color, y, TEAM_FREE, width, &lineIndex );
		y += 4.0f;
	}

	if ( cg.teamPlayers[TEAM_SPECTATOR] ) {
		y = CG_DrawScoreboard_ListBanner( color, y, width, 32.0f, TEAM_SPECTATOR, &lineIndex );
		CG_DrawTeamOfClientScore( color, y, TEAM_SPECTATOR, width, &lineIndex );
	}

	if ( scrollbar ) {
		CG_DrawScrollbar( headerY, numLines, color, lineIndex );
	}

	if ( cg.scoreboardScrollPos > numLines - 1 ) {
		cg.scoreboardScrollPos = numLines - 1;
	}

	trap_R_SetColor( NULL );
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboard( void ) {
	float   fade;
	float   *fadeColor;

	if ( cl_paused.integer ) {
		return qfalse;
	}

	if ( cg.showScores ) {
		fade = 1.0f;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, 100 );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	CG_DrawScoreboard_Backdrop( fade );
	CG_DrawScoreboard_ScoresList( fade );

	return qtrue;
}

/*
=================
CG_ScoreboardDisplayed
=================
*/
qboolean CG_ScoreboardDisplayed( void ) {
	return cg.showScores != 0;
}

/*
=================
CG_ScrollScoreboardUp
=================
*/
void CG_ScrollScoreboardUp( void ) {
	if ( cg.scoreboardScrollPos <= 0 ) {
		return;
	}

	cg.scoreboardScrollPos -= cg_scoreboardScrollStep.integer;
	if ( cg.scoreboardScrollPos < 0 ) {
		cg.scoreboardScrollPos = 0;
	}
}

/*
=================
CG_ScrollScoreboardDown
=================
*/
void CG_ScrollScoreboardDown( void ) {
	if ( !cg.scoreboardListFull ) {
		return;
	}

	cg.scoreboardScrollPos += cg_scoreboardScrollStep.integer;
	if ( cg.scoreboardScrollPos > cg.numScores - 1 ) {
		cg.scoreboardScrollPos = cg.numScores - 1;
	}
}

/*
=================
CG_RegisterScoreboardGraphics
=================
*/
void CG_RegisterScoreboardGraphics( void ) {
	char buffer[1024];

	trap_R_RegisterShaderNoMip( "black", 5 );
	trap_R_RegisterShaderNoMip( "white", 5 );
	trap_R_RegisterShaderNoMip( "black", 5 );
	trap_R_RegisterShaderNoMip( "white", 5 );
	trap_R_RegisterShaderNoMip( "white", 5 );
	trap_R_RegisterShaderNoMip( "black", 5 );
	trap_R_RegisterShaderNoMip( "hudScoreboardScroll_UpArrow", 5 );
	trap_R_RegisterShaderNoMip( "hudScoreboardScroll_UpKey", 5 );
	trap_R_RegisterShaderNoMip( "hudScoreboardScroll_DownArrow", 5 );
	trap_R_RegisterShaderNoMip( "hudScoreboardScroll_DownKey", 5 );

	trap_Cvar_VariableStringBuffer( "g_ScoresBanner_Spectators", buffer, sizeof( buffer ) );
	trap_R_RegisterShaderNoMip( buffer, 5 );

	trap_Cvar_VariableStringBuffer( "g_ScoresBanner_Axis", buffer, sizeof( buffer ) );
	trap_R_RegisterShaderNoMip( buffer, 5 );

	trap_Cvar_VariableStringBuffer( "g_ScoresBanner_Allies", buffer, sizeof( buffer ) );
	trap_R_RegisterShaderNoMip( buffer, 5 );

	trap_Cvar_VariableStringBuffer( "g_ScoresBanner_None", buffer, sizeof( buffer ) );
	trap_R_RegisterShaderNoMip( buffer, 5 );
}
