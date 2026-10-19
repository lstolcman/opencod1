// g_main_mp.c -- game module entry point, cvars and the per-frame loop

#include "g_local.h"
#include "bg_public.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef QDECL
#define QDECL   __cdecl
#endif

// the animation clock read by the bg_animation code
#ifndef GAMEDLL
extern int bgs_time;
extern int bgs_animTime;
extern int bgs_frametime;
#endif

level_locals_t level;

typedef struct {
	vmCvar_t    *vmCvar;
	char        *cvarName;
	char        *defaultString;
	int cvarFlags;
	int modificationCount;          // for tracking changes
	qboolean trackChange;           // track this variable, and announce if changed
} cvarTable_t;

gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

// a candidate of the activate search, sorted by score (lowest first)
typedef struct {
	gentity_t   *ent;
	float score;
} activateEnt_t;

// half-extents of the activate search box
static const vec3_t activateRange = { 192.0f, 192.0f, 96.0f };

vmCvar_t g_cheats;
vmCvar_t g_gametype;
vmCvar_t g_maxclients;
vmCvar_t g_synchronousClients;
vmCvar_t g_intermissionDelay;
vmCvar_t g_log;
vmCvar_t g_logSync;
vmCvar_t g_password;
vmCvar_t g_banIPs;
vmCvar_t g_dedicated;
vmCvar_t g_speed;
vmCvar_t g_gravity;
vmCvar_t g_knockback;
vmCvar_t g_weaponRespawn;
vmCvar_t g_weaponAmmoPools;
vmCvar_t g_inactivity;
vmCvar_t g_debugMove;
vmCvar_t g_debugProneCheck;
vmCvar_t g_debugProneCheckDepthCheck;
vmCvar_t g_debugDamage;
vmCvar_t g_debugAlloc;
vmCvar_t g_debugBullets;
vmCvar_t g_motd;
vmCvar_t g_allowVote;
vmCvar_t g_listEntity;
vmCvar_t g_complaintlimit;
vmCvar_t g_voiceChatsAllowed;
vmCvar_t g_developer;
vmCvar_t g_ScoresBanner_Allies;
vmCvar_t g_ScoresBanner_Axis;
vmCvar_t g_ScoresBanner_None;
vmCvar_t g_ScoresBanner_Spectators;
vmCvar_t g_TeamName_Allies;
vmCvar_t g_TeamName_Axis;
vmCvar_t g_TeamColor_Allies;
vmCvar_t g_TeamColor_Axis;
vmCvar_t g_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t g_scriptMainMenu;
vmCvar_t bg_viewheight_standing;
vmCvar_t bg_viewheight_crouched;
vmCvar_t bg_viewheight_prone;
vmCvar_t bg_duck2prone_time;
vmCvar_t bg_prone2duck_time;
vmCvar_t bg_ladder_yawcap;
vmCvar_t bg_prone_yawcap;
vmCvar_t bg_prone_softyawedge;
vmCvar_t bg_foliagesnd_minspeed;
vmCvar_t bg_foliagesnd_maxspeed;
vmCvar_t bg_foliagesnd_slowinterval;
vmCvar_t bg_foliagesnd_fastinterval;
vmCvar_t bg_foliagesnd_resetinterval;
vmCvar_t bg_fallDamageMinHeight;
vmCvar_t bg_fallDamageMaxHeight;
vmCvar_t bg_debugWeaponAnim;
vmCvar_t bg_debugWeaponState;
vmCvar_t g_bounds_width;
vmCvar_t g_bounds_height_standing;
vmCvar_t g_NoScriptSpam;
vmCvar_t g_debugShowHit;
vmCvar_t g_debugLocDamage;
vmCvar_t bg_debugAnim;
vmCvar_t bg_swingSpeed;
vmCvar_t g_useGear;
vmCvar_t g_languagewarnings;
vmCvar_t g_languagewarningsaserrors;
vmCvar_t g_dumpAnims;

static cvarTable_t gameCvarTable[] = {
	{ &g_cheats,                 "sv_cheats",                   "",                                 0, 0, qfalse },
	{ NULL,                      "gamename",                    "main",                             CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	{ NULL,                      "gamedate",                    "Oct  8 2003",                      CVAR_ROM, 0, qfalse },
	{ NULL,                      "sv_mapname",                  "",                                 CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	{ &g_gametype,               "g_gametype",                  "dm",                               CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_maxclients,             "sv_maxclients",               "20",                               CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse },
	{ &g_synchronousClients,     "g_synchronousClients",        "0",                                CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_intermissionDelay,      "g_intermissionDelay",         "1000",                             0, 0, qfalse },
	{ &g_log,                    "g_log",                       "games_mp.log",                     CVAR_ARCHIVE, 0, qfalse },
	{ &g_logSync,                "g_logSync",                   "0",                                CVAR_ARCHIVE, 0, qfalse },
	{ &g_password,               "g_password",                  "",                                 0, 0, qfalse },
	{ &g_banIPs,                 "g_banIPs",                    "",                                 CVAR_ARCHIVE, 0, qfalse },
	{ &g_dedicated,              "dedicated",                   "0",                                0, 0, qfalse },
	{ &g_speed,                  "g_speed",                     "190",                              0, 0, qtrue },
	{ &g_gravity,                "g_gravity",                   "800",                              0, 0, qtrue },
	{ &g_knockback,              "g_knockback",                 "1000",                             0, 0, qtrue },
	{ &g_weaponRespawn,          "g_weaponrespawn",             "5",                                0, 0, qtrue },
	{ &g_weaponAmmoPools,        "g_weaponAmmoPools",           "0",                                0, 0, qtrue },
	{ &g_inactivity,             "g_inactivity",                "0",                                0, 0, qtrue },
	{ &g_debugMove,              "g_debugMove",                 "0",                                0, 0, qfalse },
	{ &g_debugProneCheck,        "g_debugProneCheck",           "0",                                0, 0, qfalse },
	{ &g_debugProneCheckDepthCheck, "g_debugProneCheckDepthCheck", "1",                                0, 0, qfalse },
	{ &g_debugDamage,            "g_debugDamage",               "0",                                CVAR_CHEAT, 0, qfalse },
	{ &g_debugAlloc,             "g_debugAlloc",                "0",                                0, 0, qfalse },
	{ &g_debugBullets,           "g_debugBullets",              "0",                                CVAR_CHEAT, 0, qfalse },
	{ &g_motd,                   "g_motd",                      "",                                 0, 0, qfalse },
	{ &g_allowVote,              "g_allowVote",                 "1",                                0, 0, qfalse },
	{ &g_listEntity,             "g_listEntity",                "0",                                0, 0, qfalse },
	{ &g_complaintlimit,         "g_complaintlimit",            "3",                                CVAR_ARCHIVE, 0, qtrue },
	{ &g_voiceChatsAllowed,      "g_voiceChatsAllowed",         "4",                                CVAR_ARCHIVE, 0, qfalse },
	{ &g_developer,              "developer",                   "0",                                CVAR_TEMP, 0, qfalse },
	{ &g_ScoresBanner_Allies,    "g_ScoresBanner_Allies",       "gfx/hud/hud@mpflag_american.tga",  CVAR_WOLFINFO, 0, qfalse },
	{ &g_ScoresBanner_Axis,      "g_ScoresBanner_Axis",         "gfx/hud/hud@mpflag_german.tga",    CVAR_WOLFINFO, 0, qfalse },
	{ &g_ScoresBanner_None,      "g_ScoresBanner_None",         "gfx/hud/hud@mpflag_none.tga",      CVAR_WOLFINFO, 0, qfalse },
	{ &g_ScoresBanner_Spectators, "g_ScoresBanner_Spectators",   "gfx/hud/hud@mpflag_spectator.tga", CVAR_WOLFINFO, 0, qfalse },
	{ &g_TeamName_Allies,        "g_TeamName_Allies",           "GAME_ALLIES",                      CVAR_WOLFINFO, 0, qfalse },
	{ &g_TeamName_Axis,          "g_TeamName_Axis",             "GAME_AXIS",                        CVAR_WOLFINFO, 0, qfalse },
	{ &g_TeamColor_Allies,       "g_TeamColor_Allies",          "0.5 0.5 1",                        CVAR_WOLFINFO, 0, qfalse },
	{ &g_TeamColor_Axis,         "g_TeamColor_Axis",            "1 0.5 0.5",                        CVAR_WOLFINFO, 0, qfalse },
	{ &g_smoothClients,          "g_smoothClients",             "1",                                0, 0, qfalse },
	{ &pmove_fixed,              "pmove_fixed",                 "0",                                CVAR_SYSTEMINFO, 0, qfalse },
	{ &pmove_msec,               "pmove_msec",                  "8",                                CVAR_SYSTEMINFO, 0, qfalse },
	{ &g_scriptMainMenu,         "g_scriptMainMenu",            "",                                 0, 0, qfalse },
	{ &bg_viewheight_standing,   "bg_viewheight_standing",      "60",                               CVAR_WOLFINFO, 0, qfalse },
	{ &bg_viewheight_crouched,   "bg_viewheight_crouched",      "40",                               CVAR_WOLFINFO, 0, qfalse },
	{ &bg_viewheight_prone,      "bg_viewheight_prone",         "11",                               CVAR_WOLFINFO, 0, qfalse },
	{ &bg_duck2prone_time,       "bg_duck2prone_time",          "400",                              CVAR_WOLFINFO, 0, qfalse },
	{ &bg_prone2duck_time,       "bg_prone2duck_time",          "400",                              CVAR_WOLFINFO, 0, qfalse },
	{ &bg_ladder_yawcap,         "bg_ladder_yawcap",            "100",                              CVAR_WOLFINFO, 0, qfalse },
	{ &bg_prone_yawcap,          "bg_prone_yawcap",             "85",                               CVAR_WOLFINFO, 0, qfalse },
	{ &bg_prone_softyawedge,     "bg_prone_softyawedge",        "1",                                CVAR_WOLFINFO, 0, qfalse },
	{ &bg_foliagesnd_minspeed,   "bg_foliagesnd_minspeed",      "40",                               CVAR_WOLFINFO, 0, qfalse },
	{ &bg_foliagesnd_maxspeed,   "bg_foliagesnd_maxspeed",      "180",                              CVAR_WOLFINFO, 0, qfalse },
	{ &bg_foliagesnd_slowinterval, "bg_foliagesnd_slowinterval",  "1500",                             CVAR_WOLFINFO, 0, qfalse },
	{ &bg_foliagesnd_fastinterval, "bg_foliagesnd_fastinterval",  "500",                              CVAR_WOLFINFO, 0, qfalse },
	{ &bg_foliagesnd_resetinterval, "bg_foliagesnd_resetinterval", "500",                              CVAR_WOLFINFO, 0, qfalse },
	{ &bg_fallDamageMinHeight,   "bg_fallDamageMinHeight",      "256",                              CVAR_SYSTEMINFO | CVAR_CHEAT, 0, qfalse },
	{ &bg_fallDamageMaxHeight,   "bg_fallDamageMaxHeight",      "480",                              CVAR_SYSTEMINFO | CVAR_CHEAT, 0, qfalse },
	{ &bg_debugWeaponAnim,       "bg_debugWeaponAnim",          "0",                                CVAR_CHEAT, 0, qfalse },
	{ &bg_debugWeaponState,      "bg_debugWeaponState",         "0",                                CVAR_CHEAT, 0, qfalse },
	{ &g_bounds_width,           "g_bounds_width",              "30",                               CVAR_CHEAT, 0, qfalse },
	{ &g_bounds_height_standing, "g_bounds_height_standing",    "70",                               CVAR_CHEAT, 0, qfalse },
	{ &g_NoScriptSpam,           "g_no_script_spam",            "0",                                0, 0, qfalse },
	{ &g_debugShowHit,           "g_debugShowHit",              "0",                                CVAR_CHEAT, 0, qfalse },
	{ &g_debugLocDamage,         "g_debugLocDamage",            "0",                                CVAR_CHEAT, 0, qfalse },
	{ &bg_debugAnim,             "g_debuganim",                 "0",                                CVAR_CHEAT, 0, qfalse },
	{ &bg_swingSpeed,            "bg_swingSpeed",               "0.2",                              CVAR_CHEAT, 0, qfalse },
	{ &g_useGear,                "g_useGear",                   "1",                                CVAR_ARCHIVE | CVAR_LATCH, 0, qfalse },
	{ &g_languagewarnings,       "cl_languagewarnings",         "0",                                0, 0, qfalse },
	{ &g_languagewarningsaserrors, "cl_languagewarningsaserrors", "0",                                0, 0, qfalse },
	{ &g_dumpAnims,              "g_dumpAnims",                 "-1",                               CVAR_CHEAT, 0, qfalse },
};

static int gameCvarTableSize = sizeof( gameCvarTable ) / sizeof( gameCvarTable[0] );


/*
================
vmMain

This is the only way control passes into the module.
================
*/
int vmMain( int command, int arg0, int arg1, int arg2, int arg3 ) {
	int result;

	result = 0;

	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2, arg3 );
		break;
	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		break;
	case GAME_CLIENT_CONNECT:
		result = (int)ClientConnect( arg0, arg1 );
		break;
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
		break;
	case GAME_GET_CLIENT_INFO:
		result = GetFollowPlayerState( arg0, (playerState_t *)arg1 );
		break;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		break;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		break;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0 );
		break;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		break;
	case GAME_UPDATE_CVARS:
		G_UpdateCvars();
		break;
	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		break;
	case GAME_CONSOLE_COMMAND_SV:
		result = ConsoleCommand();
		break;
	case GAME_SCRIPT_FAR_HOOK:
		result = (int)Scr_FarHook( (void *)arg0 );
		break;
	case 13:
		G_DObjCalcPose( &level.gentities[arg0] );
		break;
	case 14:
		result = arg0 >= 0 && arg0 <= bg_numWeapons;
		break;
	case 15:
		level.matchState = arg0;
		break;
	case GAME_GET_MATCH_STATE:
		result = level.matchState;
		break;
	case GAME_GET_CLIENT_STATE:
		result = (int)&level.clients[arg0].sess.clientNum;
		break;
	case GAME_GET_ARCHIVE_TIME:
		result = level.clients[arg0].sess.archiveTime;
		break;
	case GAME_SET_ARCHIVE_TIME:
		level.clients[arg0].sess.archiveTime = arg1;
		break;
	case GAME_GET_CLIENT_SCORE:
		result = level.clients[arg0].sess.score;
		break;
	default:
		result = -1;
		break;
	}

	return result;
}

void QDECL G_Printf( const char *fmt, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Printf( text );
}

void QDECL G_DPrintf( const char *fmt, ... ) {
	va_list argptr;
	char text[1024];

	if ( !g_developer.integer ) {
		return;
	}

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Printf( text );
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Error( text );
}

// like G_Error, but without the "Error: " prefix so a localized string
// reaches the client as-is
void QDECL G_Error_Localized( const char *fmt, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	trap_Error_Localized( text );
}

/*
==============
G_CompareActivateEntScores

qsort predicate: lowest score first, so the best-aimed candidate ends up at
index 0.
==============
*/
int QDECL G_CompareActivateEntScores( const void *a, const void *b ) {
	return (int)( ( (const activateEnt_t *)a )->score - ( (const activateEnt_t *)b )->score );
}

/*
==============
G_GetActivateEnt

Collect everything within 128 units of the muzzle that lies inside the
activation cone, score it, and sort it.  Items the player cannot pick up and
candidates the world blocks are pushed past the end of the returned count
rather than dropped, so the caller can still see them in the list.
==============
*/
int G_GetActivateEnt( gentity_t *ent, activateEnt_t *activateEnts ) {
	gclient_t   *client;
	gentity_t   *other;
	vec3_t forward, muzzle, mins, maxs, dir, center;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	int i, count, numHidden, numBlocked;
	float dist, dot, frac;
	trace_t trace;
	float tagMatrix[4][4];

	count = 0;
	numHidden = 0;
	client = ent->client;

	AngleVectors( client->ps.viewangles, forward, NULL, NULL );

	CalcMuzzlePoint( ent, muzzle );

	VectorSubtract( muzzle, activateRange, mins );
	VectorAdd( muzzle, activateRange, maxs );

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES, 0x200000 );

	for ( i = 0; i < numListedEntities; i++ ) {
		other = &g_entities[entityList[i]];
		if ( ent == other ) {
			continue;
		}
		/* items, or anything the map marked with the activate contents bit */
		if ( other->s.eType != ET_ITEM && !( other->r.contents & 0x200000 ) ) {
			continue;
		}

		VectorAdd( other->r.absmax, other->r.absmin, center );
		VectorScale( center, 0.5f, center );
		VectorSubtract( center, muzzle, dir );
		dist = VectorNormalize( dir );
		if ( dist > 128.0f ) {
			continue;
		}

		dot = forward[2] * dir[2] + forward[1] * dir[1] + forward[0] * dir[0];
		if ( dot <= 0.0f ) {
			continue;
		}
		if ( dot < 0.76f ) {
			continue;
		}

		// keep: 0.24000001f is ( 1.0f - 0.76f ) as a float
		frac = 1.0f - ( dot - 0.76f ) / 0.24000001f;
		activateEnts[count].score = frac * 256.0f;
		if ( other->s.eType == ET_ITEM && !BG_CanItemBeGrabbed( &other->s, &ent->client->ps, 0 ) ) {
			activateEnts[count].score += 10000.0f;
			numHidden++;
		}
		activateEnts[count].ent = other;
		activateEnts[count].score += dist;
		count++;
	}

	qsort( activateEnts, count, sizeof( activateEnts[0] ), G_CompareActivateEntScores );

	count -= numHidden;

	for ( numBlocked = 0; numBlocked < count; numBlocked++ ) {
		other = activateEnts[numBlocked].ent;

		VectorAdd( other->r.absmax, other->r.absmin, center );
		VectorScale( center, 0.5f, center );
		if ( other->s.eType == ET_TURRET && G_DObjGetWorldTagMatrix( other, "tag_aim", tagMatrix ) ) {
			center[0] = tagMatrix[3][0];
			center[1] = tagMatrix[3][1];
			center[2] = tagMatrix[3][2];
		}

		trap_Trace( &trace, muzzle, vec3_origin, vec3_origin, center,
					client->ps.clientNum, MASK_SOLID );
		if ( trace.entityNum != ENTITYNUM_WORLD ) {
			break;
		}
		activateEnts[numBlocked].score += 100000.0f;
	}

	qsort( activateEnts, count, sizeof( activateEnts[0] ), G_CompareActivateEntScores );

	return count - numBlocked;
}

/*
==============
G_CheckForCursorHints

	server-side because there's info we want to show that the client
	just doesn't know about.  (door keys, turret use strings, items, etc.)
==============
*/
void G_CheckForCursorHints( gentity_t *ent ) {
	gclient_t   *client;
	gentity_t   *other;
	gitem_t     *item;
	weaponInfo_t *weapInfo;
	activateEnt_t activateEnts[MAX_GENTITIES];
	int numActivateEnts;
	int i;
	int hintType;
	int hintString;

	client = ent->client;
	client->ps.serverCursorHint = 0;
	client->ps.serverCursorHintVal = 0;
	client->ps.serverCursorHintTrace.entityNum = ENTITYNUM_NONE;

	if ( ent->health <= 0 || ent->active ) {
		return;
	}

	client->ps.serverCursorHintString = -1;
	hintString = -1;
	client->ps.serverCursorHint = 0;
	client->ps.serverCursorHintVal = 0;

	numActivateEnts = G_GetActivateEnt( ent, activateEnts );
	if ( !numActivateEnts ) {
		return;
	}

	hintType = 0;
	for ( i = 0; i < numActivateEnts; i++ ) {
		other = activateEnts[i].ent;
		client->ps.serverCursorHintTrace.entityNum = other->s.number;

		if ( other->s.number == ENTITYNUM_WORLD ) {
			if ( ( client->ps.serverCursorHintTrace.surfaceFlags & SURF_LADDER )
				 && !( client->ps.pm_flags & PMF_LADDER ) ) {
				hintType = HINT_LADDER;
			}
			goto done;
		}

		if ( other->client ) {
			goto done;
		}

		if ( other->s.eType == ET_GENERAL ) {
			if ( other->classname == scr_const.trigger_use ) {
				hintType = other->s.dmgFlags;
				if ( hintType && other->s.scale != HINT_STRING_NONE ) {
					hintString = other->s.scale;
				}
			}
		} else if ( other->s.eType == ET_TURRET ) {
			if ( !G_IsTurretUsable( other, ent ) ) {
				continue;
			}
			weapInfo = bg_weaponInfo[other->s.weapon];
			hintType = HINT_MG42;
			if ( *weapInfo->useHintString ) {
				hintString = weapInfo->useHintStringIndex;
			}
		} else if ( other->s.eType == ET_ITEM ) {
			item = &bg_itemlist[other->item - bg_itemlist];
			switch ( item->giType ) {
			case IT_WEAPON:
				hintType = item->giTag;
				if ( Com_BitCheck( ent->client->ps.weapons, item->giTag ) ) {
					hintType += HINT_AMMO_FIRST - 1;
				} else {
					hintType += HINT_WEAPON_FIRST - 1;
				}
				break;
			case IT_AMMO:
				hintType = item->giTag + ( HINT_AMMO_FIRST - 1 );
				break;
			case IT_HEALTH:
				hintType = HINT_HEALTH;
				break;
			}
		} else if ( other->s.eType == ET_MOVER ) {
			if ( other->classname == scr_const.func_door_rotating ) {
				if ( other->moverState == MOVER_POS1ROTATE
					 || ( other->moverState == MOVER_POS2ROTATE && ( other->flags & FL_TOGGLE ) ) ) {
					hintType = HINT_DOOR;
					if ( other->key ) {
						hintType = HINT_DOOR_LOCKED;
					}
				}
			} else if ( other->classname == scr_const.func_door ) {
				if ( other->moverState == MOVER_POS1
					 || ( other->moverState == MOVER_POS2 && ( other->flags & FL_TOGGLE ) ) ) {
					hintType = HINT_DOOR;
					if ( other->key ) {
						hintType = HINT_DOOR_LOCKED;
					}
				}
			}
		}

		if ( other->s.dmgFlags > 0 && hintType ) {
			hintType = other->s.dmgFlags;
		}
done:
		client->ps.serverCursorHint = hintType;
		client->ps.serverCursorHintVal = 0;
		client->ps.serverCursorHintString = hintString;
		if ( !hintType ) {
			client->ps.serverCursorHintTrace.entityNum = ENTITYNUM_NONE;
		}
		return;
	}

	// only reached when every candidate was an unusable turret; the hint
	// fields are left as they are
}

/*
==============
G_CheckForPreventFriendlyFire

Fire a pair of locational traces down the player's aim and remember any
trigger_lookat they land in, so the client can be told not to shoot.
==============
*/
void G_CheckForPreventFriendlyFire( gentity_t *ent ) {
	gentity_t   *other;
	trace_t trace;
	const byte  *partState;
	vec3_t end;
	weaponFireInfo_t fireInfo;

	ent->client->lookatEnt = NULL;
	if ( ent->active ) {
		return;
	}

	CalcMuzzlePoints( &fireInfo, ent );

	if ( ent->client->ps.weapon && bg_weaponInfo[ent->client->ps.weapon]->rifleBullet ) {
		partState = riflePriorityMap;
	} else {
		partState = bulletPriorityMap;
	}

	end[0] = fireInfo.forward[0] * 8192.0f + fireInfo.start[0];
	end[1] = fireInfo.forward[1] * 8192.0f + fireInfo.start[1];
	end[2] = fireInfo.forward[2] * 8192.0f + fireInfo.start[2];

	trap_LocationalTrace( &trace, fireInfo.start, end, ent->s.number, 0x20000001, partState );
	if ( trace.entityNum >= ENTITYNUM_WORLD ) {
		return;
	}

	trap_LocationalTrace( &trace, fireInfo.start, end, ent->s.number, 0x22802001, partState );
	if ( trace.entityNum >= ENTITYNUM_WORLD ) {
		return;
	}

	other = &g_entities[trace.entityNum];
	if ( other->classname != scr_const.trigger_lookat ) {
		return;
	}

	ent->client->lookatEnt = other;
	G_Trigger( other, ent );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t   *e, *e2;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;
	for ( i = 1, e = g_entities + i ; i < level.num_entities ; i++,e++ ) {
		if ( !e->inuse ) {
			continue;
		}

		if ( !e->team ) {
			continue;
		}

		if ( e->flags & FL_TEAMSLAVE ) {
			continue;
		}

		if ( e->classname == scr_const.func_tramcar ) {
			if ( e->spawnflags & 8 ) { // leader
				e->teammaster = e;
			} else
			{
				continue;
			}
		}

		c++;
		c2++;
		for ( j = i + 1, e2 = e + 1 ; j < level.num_entities ; j++,e2++ )
		{
			if ( !e2->inuse ) {
				continue;
			}
			if ( !e2->team ) {
				continue;
			}
			if ( e2->flags & FL_TEAMSLAVE ) {
				continue;
			}
			if ( e->team == e2->team ) {
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				if ( e2->classname == scr_const.func_tramcar ) {
					trap_UnlinkEntity( e2 );
				}
			}
		}
	}

	G_Printf( "%i teams with %i entities\n", c, c2 );
}

/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
							cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar ) {
			cv->modificationCount = cv->vmCvar->modificationCount;
		}
	}

	if ( !Scr_IsValidGameType( g_gametype.string ) ) {
		G_Printf( "g_gametype %s is not a valid gametype, defaulting to dm\n", g_gametype.string );
		trap_Cvar_Set( "g_gametype", "dm" );
		trap_Cvar_Update( &g_gametype );
	}
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, 0, va( "e \"GAME_SERVER\x15: %s \x14GAME_CHANGEDTO\x15 %s\"",
													   cv->cvarName, cv->vmCvar->string ) );
				}
			}
		}
	}
}

/*
=================
G_FreeEntities
=================
*/
void G_FreeEntities( void ) {
	int i;
	gentity_t   *ent;

	for ( i = 0, ent = g_entities ; i < level.num_entities ; i++, ent++ ) {
		if ( ent->inuse ) {
			G_FreeEntity( ent );
		}
	}

	if ( g_entities[ENTITYNUM_WORLD].inuse ) {
		G_FreeEntity( &g_entities[ENTITYNUM_WORLD] );
	}
}


/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart, int matchState ) {
	int i;
	objective_t *obj;
	char serverinfo[MAX_STRING_CHARS];
	char css[MAX_STRING_CHARS];

	Swap_Init();

	G_Printf( "------- Game Initialization -------\n" );
	G_Printf( "gamename: %s\n", GAMEVERSION );
	G_Printf( "gamedate: %s\n", "Oct  8 2003" );

	srand( randomSeed );
	com_randSeed = trap_Milliseconds();

	Scr_ParseGameTypeList();

	if ( !matchState ) {
		G_RegisterCvars();
	}

	G_ProcessIPBans();

	// keep: matches original codegen
	playerMins[1] = g_bounds_width.value * -0.5f;
	((unsigned int *)playerMins)[0] = ((unsigned int *)playerMins)[1];
	playerMaxs[1] = g_bounds_width.value * 0.5f;
	((unsigned int *)playerMaxs)[0] = ((unsigned int *)playerMaxs)[1];
	playerMaxs[2] = g_bounds_height_standing.value;

	// set some level globals
	memset( &level, 0, sizeof( level ) );

#ifdef GAMEDLL
	memset( &bg, 0, sizeof( bg ) );
#else
	bgs_time = 0;
	bgs_animTime = 0;
	bgs_frametime = 0;
#endif
	memset( &bgs, 0, sizeof( bgs ) );

	level.spawning = qtrue;
	level.time = levelTime;
	level.startTime = levelTime;

	// init the anim scripting
	bgs_animScriptData.soundIndex = trap_Com_SoundAliasString;
	bgs_animScriptData.playSound = G_AnimScriptSound;

	if ( g_log.string[0] ) {
		if ( g_logSync.integer ) {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_log.string, &level.logFile, FS_APPEND );
		}
		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_log.string );
		} else {
			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );
			G_LogPrintf( "------------------------------------------------------------\n" );
			G_LogPrintf( "InitGame: %s\n", serverinfo );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

	BG_SetupWeaponInfo();
	GScr_LoadScripts();
	GScr_LoadConsts();

	trap_GetConfigstring( CS_SCORES, css, sizeof( css ) );
	Info_SetValueForKey( css, "winner", "0" );
	trap_SetConfigstring( CS_SCORES, css );

	// initialize all entities for this game
	memset( g_entities, 0, sizeof( g_entities ) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, sizeof( g_clients ) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS + BODY_QUEUE_SIZE;

	level.freeListHead = NULL;
	level.freeListTail = NULL;

	// let the server system know where the entites are
	trap_LocateGameData( g_entities, level.num_entities, sizeof( gentity_t ),
						 &level.clients[0].ps, sizeof( gclient_t ) );

	G_ParseHitLocDmgTable();

	if ( !restart ) {
		ClearRegisteredItems();
	}

	G_InitTurrets();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// general initialization
	G_FindTeams();

	SaveRegisteredItems();

	G_setfog( "0" );

	for ( i = 0 ; i < MAX_OBJECTIVES ; i++ ) {
		obj = &level.objectives[i];
		obj->state = OBJST_EMPTY;
		VectorClear( obj->origin );
		obj->entNum = ENTITYNUM_NONE;
		obj->teamNum = TEAM_FREE;
		obj->icon = 0;
	}

	G_Printf( "-----------------------------------\n" );

	Scr_AllocGameVariable( 1, level.time );

	Scr_SetLoading( qtrue );
	Scr_InitSystem();

	Scr_FreeThread( Scr_ExecThread( g_scr_data.gameTypeMain, 0 ) );

	if ( g_scr_data.levelScriptMain ) {
		Scr_FreeThread( Scr_ExecThread( g_scr_data.levelScriptMain, 0 ) );
	}

	Scr_FreeThread( Scr_ExecThread( g_scr_data.gameTypeStartup, 0 ) );

	level.spawning = qfalse;            // any future calls to G_Spawn*() will be errors
}

/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
	if ( restart ) {
		G_Printf( "==== RestartGame ====\n" );
	} else {
		G_Printf( "==== ShutdownGame ====\n" );
	}

	if ( level.logFile ) {
		if ( restart ) {
			G_LogPrintf( "RestartGame:\n" );
		} else {
			G_LogPrintf( "ShutdownGame:\n" );
		}
		G_LogPrintf( "------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
	}

	G_FreeEntities();
	HudElem_DestroyAll();

	if ( Scr_IsSystemActive( 1 ) ) {
		if ( !level.matchState ) {
			trap_FreeClientScriptPers();
		}
		Scr_FreeGameVariable( level.matchState == 0 );
		Scr_ShutdownSystem( 1 );
	}

	Scr_RemoveClassMap();
	Scr_FreeScripts( 1 );

	trap_FreeWeaponInfoMemory( 0 );
}


//===================================================================

// this is only here so the functions in q_shared.c and bg_*.c can link

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr );

	G_Error( "%s", text );
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	va_start( argptr, msg );
	vsprintf( text, msg, argptr );
	va_end( argptr );

	G_Printf( "%s", text );
}

void QDECL Com_DPrintf( const char *msg, ... ) {
	va_list argptr;
	char text[1024];

	if ( !g_developer.integer ) {
		return;
	}

	va_start( argptr, msg );
	vsprintf( text, msg, argptr );
	va_end( argptr );

	G_Printf( "%s", text );
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllIntermissionClients
========================
*/
void SendScoreboardMessageToAllIntermissionClients( void ) {
	int i;

	if ( !level.scoreboardChanged ) {
		return;
	}

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].sess.connected == CON_CONNECTED
			 && level.clients[ i ].ps.pm_type == PM_INTERMISSION ) {
			DeathmatchScoreboardMessage( g_entities + i );
		}
	}

	level.scoreboardChanged = 0;
}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t   *ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort connecting clients last
	if ( ca->sess.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->sess.connected == CON_CONNECTING ) {
		return -1;
	}

	// then spectators, in client order
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca < cb ) {
			return -1;
		}
		if ( ca > cb ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score, and by fewest deaths within a tie
	if ( ca->sess.score > cb->sess.score ) {
		return -1;
	}
	if ( ca->sess.score < cb->sess.score ) {
		return 1;
	}
	if ( ca->sess.deaths < cb->sess.deaths ) {
		return -1;
	}
	if ( ca->sess.deaths > cb->sess.deaths ) {
		return 1;
	}
	return 0;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int i;

	level.numConnectedClients = 0;
	level.numVotingClients = 0;         // don't count bots

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].sess.connected ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR
				 && level.clients[i].sess.connected == CON_CONNECTED ) {
				level.numVotingClients++;
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients,
		   sizeof( level.sortedClients[0] ), SortRanks );

	level.scoreboardChanged = 1;
}

/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the map rotation
=============
*/
void ExitLevel( void ) {
	int i;
	gclient_t *cl;

	trap_SendConsoleCommand( EXEC_APPEND, "map_rotate\n" );

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_AXIS] = 0;
	level.teamScores[TEAM_ALLIES] = 0;
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->sess.connected != CON_CONNECTED ) {
			continue;
		}
		cl->sess.score = 0;
	}

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
		if ( level.clients[i].sess.connected == CON_CONNECTED ) {
			level.clients[i].sess.connected = CON_CONNECTING;
		}
	}

	G_LogPrintf( "ExitLevel: executed\n" );
}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list argptr;
	char string[1024];
	int min, tens, sec;

	sec = level.time / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof( string ), "%3i:%i%i ", min, tens, sec );

	va_start( argptr, fmt );
	vsprintf( string + 7, fmt, argptr );
	va_end( argptr );

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
==================
CheckVote
==================
*/
void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time - level.voteTime >= 0 ) {
		trap_SendServerCommand( -1, 0, "e \"GAME_VOTEFAILED\"" );
	} else {
		if ( level.voteYes > level.numVotingClients / 2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, 0, "e \"GAME_VOTEPASSED\"" );
			level.voteExecuteTime = level.time + 3000;
		} else if ( level.voteNo >= level.numVotingClients / 2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, 0, "e \"GAME_VOTEFAILED\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.voteTime = 0;
	trap_SetConfigstring( CS_VOTE_TIME, "" );
}

/*
==================
G_UpdateObjectiveToClients
==================
*/
void G_UpdateObjectiveToClients( void ) {
	int i, j;
	gentity_t   *ent;
	gclient_t   *client;
	int team;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		ent = &level.gentities[i];
		if ( !ent->inuse ) {
			continue;
		}
		client = ent->client;
		team = client->sess.sessionTeam;

		for ( j = 0 ; j < MAX_OBJECTIVES ; j++ ) {
			if ( level.objectives[j].state
				 && ( !level.objectives[j].teamNum || level.objectives[j].teamNum == team ) ) {
				client->ps.objective[j] = level.objectives[j];
			} else {
				client->ps.objective[j].state = OBJST_EMPTY;
			}
		}
	}
}

/*
==================
G_UpdateHudElemsToClients
==================
*/
void G_UpdateHudElemsToClients( void ) {
	int i;
	gentity_t   *ent;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		ent = &level.gentities[i];
		if ( ent->inuse ) {
			HudElem_UpdateClient( ent->client, ent->s.number,
								  HE_UPDATE_ARCHIVAL | HE_UPDATE_CURRENT );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink( gentity_t *ent ) {
	int thinktime;

	thinktime = ent->nextthink;
	if ( thinktime <= 0 ) {
		return;
	}
	if ( thinktime > level.time ) {
		return;
	}

	ent->nextthink = 0;
	if ( !ent->think ) {
		G_Error( "NULL ent->think" );
	}
	ent->think( ent );
}

/*
================
DebugDumpAnims
================
*/
void DebugDumpAnims( void ) {
	int entnum;

	entnum = g_dumpAnims.integer;
	if ( entnum < 0 || entnum >= MAX_GENTITIES ) {
		return;
	}
	trap_DObjDisplayAnim( &level.gentities[entnum] );
}

/*
================
G_XAnimUpdateEnt

Advance one entity's animation tree, stopping at every notetrack so the
scripts attached to it get to run before the rest of the frame is stepped.
================
*/
void G_XAnimUpdateEnt( gentity_t *ent ) {
	while ( ent->inuse ) {
		if ( ent->flags & 0x4000 ) {
			break;
		}
		if ( !trap_DObjUpdateServerTime( ent, level.frameTime * 0.001f, qtrue ) ) {
			break;
		}
		Scr_RunCurrentThreads();
	}
}

/*
================
G_XAnimUpdate
================
*/
void G_XAnimUpdate( void ) {
	int i;
	gentity_t   *ent;

	for ( i = 0, ent = g_entities ; i < level.num_entities ; i++, ent++ ) {
		if ( ent->inuse ) {
			trap_DObjInitServerTime( ent, level.frameTime * 0.001f );
		}
	}

	for ( i = 0, ent = g_entities ; i < level.num_entities ; i++, ent++ ) {
		G_XAnimUpdateEnt( ent );
	}
}

/*
================
G_RunFrameForEntity
================
*/
void G_RunFrameForEntity( gentity_t *ent ) {
	if ( ent->lastFrameNum == level.framenum ) {
		return;
	}
	ent->lastFrameNum = level.framenum;

	// check EF_NODRAW status for non-clients
	if ( !ent->client ) {
		if ( ent->flags & FL_NODRAW ) {
			ent->s.eFlags |= EF_NODRAW;
		} else {
			ent->s.eFlags &= ~EF_NODRAW;
		}
	}

	// clear events that are too old
	if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
		if ( ent->freeAfterEvent ) {
			// tempEntities or dropped items completely go away after their event
			G_FreeEntity( ent );
			return;
		}
		if ( ent->unlinkAfterEvent ) {
			// items that will respawn will hide themselves after their pickup event
			ent->unlinkAfterEvent = qfalse;
			trap_UnlinkEntity( ent );
		}
	}

	// let the server know about bbox or capsule collision
	if ( ent->s.eFlags & EF_CAPSULE ) {
		ent->r.svFlags |= SVF_CAPSULE;
	} else {
		ent->r.svFlags &= ~SVF_CAPSULE;
	}

	// temporary entities don't think
	if ( ent->freeAfterEvent ) {
		return;
	}

	if ( ent->s.eType == ET_MISSILE ) {
		G_RunMissile( ent );
		return;
	}

	if ( ent->s.eType == ET_ITEM ) {
		if ( ent->linkInfo ) {
			G_GeneralLink( ent );
			G_RunThink( ent );
			return;
		}
		G_RunItem( ent );
		return;
	}

	if ( ent->physicsObject ) {
		G_RunItem( ent );
		return;
	}

	if ( ent->s.eType == ET_MOVER || ent->s.eType == ET_SCRIPTMOVER ) {
		G_RunMover( ent );
		return;
	}

	if ( ent->client ) {
		G_RunClient( ent );
		return;
	}

	G_RunThink( ent );
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime ) {
	int i, j;
	gentity_t   *ent;
	gentity_t   *trigger, *other;
	notifyWatch_t *nw;
	byte notified[MAX_GENTITIES];
	byte pass;
	qboolean again;

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	level.frameTime = level.time - level.previousTime;

	bgs_time = levelTime;
	bgs_animTime = levelTime;
	bgs_frametime = level.frameTime;

	// get any cvar changes
	G_UpdateCvars();

	// fire every queued touch notify, but never fire the same trigger twice in
	// one pass -- a trigger that comes round again schedules another pass so
	// the scripts it started can run in between
	memset( notified, 0, sizeof( notified ) );
	pass = 0;
	do {
		again = qfalse;
		pass++;
		for ( i = 0 ; i < level.numNotifyWatches ; i++ ) {
			nw = &level.notifyWatch[i];
			trigger = &g_entities[nw->entNum];
			if ( trigger->spawnCount == nw->spawnCount ) {
				other = &g_entities[nw->otherEntNum];
				if ( other->spawnCount == nw->otherSpawnCount ) {
					if ( notified[nw->entNum] == pass ) {
						again = qtrue;
						continue;
					}
					notified[nw->entNum] = pass;
					Scr_AddEntityNum( other->s.number, 0 );
					Scr_NotifyNum( trigger->s.number, 0, scr_const.trigger, 1 );
				}
			}

			level.numNotifyWatches--;
			i--;
			*nw = level.notifyWatch[level.numNotifyWatches];
		}
		Scr_RunCurrentThreads();
	} while ( again );

	level.numNotifyWatches = 0;

	G_XAnimUpdate();

	Scr_SetTime( level.time );

	// push the player bounds out again if the cvars behind them moved
	if ( level.playerSizeWidth != g_bounds_width.value
		 || level.playerSizeHeight != g_bounds_height_standing.value ) {
		level.playerSizeWidth = g_bounds_width.value;
		level.playerSizeHeight = g_bounds_height_standing.value;

		playerMins[0] = playerMins[1] = g_bounds_width.value * -0.5f;
		playerMaxs[0] = playerMaxs[1] = g_bounds_width.value * 0.5f;
		playerMaxs[2] = g_bounds_height_standing.value;

		for ( i = 0, ent = g_entities ; i < MAX_CLIENTS ; i++, ent++ ) {
			if ( !ent->inuse ) {
				continue;
			}
			VectorCopy( playerMins, ent->r.mins );
			VectorCopy( playerMaxs, ent->r.maxs );
			VectorCopy( ent->r.mins, ent->client->ps.mins );
			VectorCopy( ent->r.maxs, ent->client->ps.maxs );
			trap_LinkEntity( ent );
		}
	}

	if ( level.viewHeightStanding != bg_viewheight_standing.value
		 || level.viewHeightCrouched != bg_viewheight_crouched.value
		 || level.viewHeightProne != bg_viewheight_prone.value ) {
		level.viewHeightStanding = bg_viewheight_standing.value;
		level.viewHeightCrouched = bg_viewheight_crouched.value;
		level.viewHeightProne = bg_viewheight_prone.value;

		for ( i = 0, ent = g_entities ; i < MAX_CLIENTS ; i++, ent++ ) {
			if ( ent->inuse ) {
				ent->client->ps.proneViewHeight = bg_viewheight_prone.integer;
				ent->client->ps.crouchViewHeight = bg_viewheight_crouched.integer;
				ent->client->ps.standViewHeight = bg_viewheight_standing.integer;
			}
		}
	}

	//
	// go through all allocated objects
	//
	for ( i = 0, ent = g_entities ; i < level.num_entities ; i++, ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}
		// a tag-linked entity runs behind whatever it is linked to, so the tag
		// it reads has already been stepped this frame
		if ( ent->linkInfo ) {
			G_RunFrameForEntity( ent->linkInfo->parent );
		}
		G_RunFrameForEntity( ent );
	}

	G_UpdateObjectiveToClients();
	G_UpdateHudElemsToClients();

	// perform final fixups on the players
	for ( i = 0, ent = g_entities ; i < level.maxclients ; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	SendScoreboardMessageToAllIntermissionClients();

	if ( g_listEntity.integer ) {
		for ( j = 0 ; j < MAX_GENTITIES ; j++ ) {
			G_Printf( "%4i: %s\n", j, SL_ConvertToString( g_entities[j].classname ) );
		}
		trap_Cvar_Set( "g_listEntity", "0" );
	}

	if ( level.registeredItemsDirty ) {
		SaveRegisteredItems();
	}

	DebugDumpAnims();
}
