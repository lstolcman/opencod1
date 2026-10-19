// g_cmds_mp.c -- client commands

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g_local.h"

// what the camera collides with when a spectator stops following
#define MASK_SPECTATOR_CAMERA   0x00810011

#define Q_COLOR_ESCAPE          '^'
#define COLOR_GREEN             '2'
#define COLOR_YELLOW            '3'
#define COLOR_CYAN              '5'
#define COLOR_MAGENTA           '6'
#define COLOR_WHITE             '7'

#define MAX_SAY_TEXT            150

#define MAX_VOTE_COUNT          3       // votes a client may call per map

#define VOTE_TIME               30000   // 30 seconds before vote times out

// voice chat spam protection
#define VOICE_CHAT_SQUELCH_MAX  30000
#define VOICE_CHAT_SQUELCH_SPAN 34000

/*
==================
DeathmatchScoreboardMessage
==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char entry[MAX_STRING_CHARS];
	char string[1400];
	int stringlength;
	int i, j;
	gclient_t   *cl;
	int numSorted;
	int ping;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	numSorted = level.numConnectedClients;
	if ( numSorted > MAX_CLIENTS ) {
		numSorted = MAX_CLIENTS;
	}

	for ( i = 0 ; i < numSorted ; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];

		if ( cl->sess.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}

		Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i",
					 level.sortedClients[i], cl->sess.score, ping,
					 cl->sess.deaths, cl->sess.statusIcon );

		j = strlen( entry );
		if ( stringlength + j > MAX_STRING_CHARS ) {
			break;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	trap_SendServerCommand( ent - g_entities, 1,
							va( "b %i %i %i%s", i, level.teamScores[TEAM_AXIS],
								level.teamScores[TEAM_ALLIES], string ) );
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}

/*
==================
CheatsOk
==================
*/
qboolean CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent - g_entities, 0, va( "e \"GAME_CHEATSNOTENABLED\"" ) );
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent - g_entities, 0, va( "e \"GAME_MUSTBEALIVECOMMAND\"" ) );
		return qfalse;
	}
	return qtrue;
}

/*
==================
ConcatArgs
==================
*/
char *ConcatArgs( int start ) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;

		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	// keep: matches original codegen
	*( line + len ) = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
static void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;        // skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}
	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t   *cl;
	int idnum;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to - g_entities, 0,
									va( "e \"GAME_BADCLIENTSLOT\x15 %i\"", idnum ) );
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->sess.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to - g_entities, 0,
									va( "e \"GAME_CLIENTNOTACTIVE\x15%i\"", idnum ) );
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum = 0, cl = level.clients ; idnum < level.maxclients ; idnum++, cl++ ) {
		if ( cl->sess.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->sess.name, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to - g_entities, 0,
							va( "e \"GAME_USERNOTONSERVER\x15%s\"", s ) );
	return -1;
}

/*
==================
G_setfog

The fog string is near distance, far distance, density, red, green, blue
and transition time in milliseconds.  Sets the distance at which the fog
becomes opaque.
==================
*/
void G_setfog( const char *fogString ) {
	float fogNear, fogFar, fogScalar, fogTime;
	vec3_t fogColor;

	trap_SetConfigstring( CS_FOGVARS, fogString );

	level.fogOpaqueDist = 3.402823466e+38F;
	level.fogOpaqueDistSq = 3.402823466e+38F;

	if ( sscanf( fogString, "%f %f %f %f %f %f %f", &fogNear, &fogFar, &fogScalar,
				 &fogColor[0], &fogColor[1], &fogColor[2], &fogTime ) == 7
		 && fogScalar >= 1.0f ) {
		level.fogOpaqueDist = fogFar * 0.828f;
		level.fogOpaqueDistSq = level.fogOpaqueDist * level.fogOpaqueDist;
	}
}

/*
==================
Cmd_Fogswitch_f
==================
*/
void Cmd_Fogswitch_f( void ) {
	G_setfog( ConcatArgs( 1 ) );
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( gentity_t *ent ) {
	char        *name;
	gitem_t     *it;
	int i;
	qboolean give_all;
	gentity_t   *it_ent;
	int amount;
	const char  *classname;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	// check for an amount (like "give health 30")
	amount = atoi( ConcatArgs( 2 ) );

	name = ConcatArgs( 1 );
	if ( !name || !strlen( name ) ) {
		return;
	}

	if ( Q_stricmp( name, "all" ) == 0 ) {
		give_all = qtrue;
	} else {
		give_all = qfalse;
	}

	if ( give_all || Q_stricmpn( name, "health", 6 ) == 0 ) {
		if ( amount ) {
			ent->health += amount;
		} else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmp( name, "weapons" ) == 0 ) {
		level.spawning = qtrue;
		for ( i = 1 ; i <= bg_numWeapons ; i++ ) {
			BG_GivePlayerWeapon( &ent->client->ps, i );
		}
		level.spawning = qfalse;
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmpn( name, "ammo", 4 ) == 0 ) {
		if ( amount ) {
			if ( ent->client->ps.weapon ) {
				Add_Ammo( ent, ent->client->ps.weapon, amount, qtrue );
			}
		} else {
			for ( i = 1 ; i <= bg_numWeapons ; i++ ) {
				Add_Ammo( ent, i, 998, qtrue );
			}
		}
		if ( !give_all ) {
			return;
		}
	}

	if ( Q_stricmpn( name, "allammo", 7 ) == 0 && amount ) {
		for ( i = 1 ; i <= bg_numWeapons ; i++ ) {
			Add_Ammo( ent, i, amount, qtrue );
		}
		return;
	}

	if ( give_all ) {
		return;
	}

	// spawn a specific item right on the player
	it = BG_FindItem( name );
	if ( !it ) {
		return;
	}

	level.spawning = qtrue;

	it_ent = G_Spawn();
	VectorCopy( ent->r.currentOrigin, it_ent->r.currentOrigin );
	// keep: matches original codegen
	classname = it->classname;
	Scr_SetString( &it_ent->classname, 0 );
	it_ent->classname = SL_GetString( classname, 0 );
	G_SpawnItem( it_ent, it );

	it_ent->active = qtrue;
	Touch_Item( it_ent, ent, 1 );
	it_ent->active = qfalse;

	if ( it_ent->inuse ) {
		G_FreeEntity( it_ent );
	}

	level.spawning = qfalse;
}

/*
==================
Cmd_Take_f

Take items from a client
==================
*/
void Cmd_Take_f( gentity_t *ent ) {
	char        *name;
	int i;
	int weapon;
	qboolean take_all;
	int amount;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	amount = atoi( ConcatArgs( 2 ) );

	name = ConcatArgs( 1 );
	if ( !name || !strlen( name ) ) {
		return;
	}

	if ( Q_stricmp( name, "all" ) == 0 ) {
		take_all = qtrue;
	} else {
		take_all = qfalse;
	}

	if ( take_all || Q_stricmpn( name, "health", 6 ) == 0 ) {
		if ( amount ) {
			ent->health -= amount;
			if ( ent->health < 1 ) {
				ent->health = 1;
			}
		} else {
			ent->health = 1;
		}
		if ( !take_all ) {
			return;
		}
	}

	if ( take_all || Q_stricmp( name, "weapons" ) == 0 ) {
		for ( i = 1 ; i <= bg_numWeapons ; i++ ) {
			BG_TakePlayerWeapon( &ent->client->ps, i );
			ent->client->ps.ammo[ bg_weaponInfo[i]->ammoIndex ] = 0;
			ent->client->ps.ammoclip[ bg_weaponInfo[i]->clipIndex ] = 0;
		}
		if ( ent->client->ps.weapon ) {
			ent->client->ps.weapon = 0;
			trap_SendServerCommand( ent - g_entities, 1, "a 0" );
		}
		if ( !take_all ) {
			return;
		}
	}

	if ( take_all || Q_stricmpn( name, "ammo", 4 ) == 0 ) {
		if ( amount ) {
			weapon = ent->client->ps.weapon;
			if ( weapon ) {
				ent->client->ps.ammo[ bg_weaponInfo[weapon]->ammoIndex ] -= amount;
				if ( ent->client->ps.ammo[ bg_weaponInfo[weapon]->ammoIndex ] < 0 ) {
					ent->client->ps.ammoclip[ bg_weaponInfo[weapon]->clipIndex ] +=
						ent->client->ps.ammo[ bg_weaponInfo[weapon]->ammoIndex ];
					ent->client->ps.ammo[ bg_weaponInfo[weapon]->ammoIndex ] = 0;
					if ( ent->client->ps.ammoclip[ bg_weaponInfo[weapon]->clipIndex ] < 0 ) {
						ent->client->ps.ammoclip[ bg_weaponInfo[weapon]->clipIndex ] = 0;
					}
				}
			}
		} else {
			for ( i = 1 ; i <= bg_numWeapons ; i++ ) {
				ent->client->ps.ammo[ bg_weaponInfo[i]->ammoIndex ] = 0;
				ent->client->ps.ammoclip[ bg_weaponInfo[i]->clipIndex ] = 0;
			}
		}
		if ( !take_all ) {
			return;
		}
	}

	if ( Q_stricmpn( name, "allammo", 7 ) == 0 && amount ) {
		for ( i = 1 ; i <= bg_numWeapons ; i++ ) {
			ent->client->ps.ammo[ bg_weaponInfo[i]->ammoIndex ] -= amount;
			if ( ent->client->ps.ammo[ bg_weaponInfo[i]->ammoIndex ] < 0 ) {
				ent->client->ps.ammoclip[ bg_weaponInfo[i]->clipIndex ] +=
					ent->client->ps.ammo[ bg_weaponInfo[i]->ammoIndex ];
				ent->client->ps.ammo[ bg_weaponInfo[i]->ammoIndex ] = 0;
				if ( ent->client->ps.ammoclip[ bg_weaponInfo[i]->clipIndex ] < 0 ) {
					ent->client->ps.ammoclip[ bg_weaponInfo[i]->clipIndex ] = 0;
				}
			}
		}
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent ) {
	const char  *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if ( !( ent->flags & FL_GODMODE ) ) {
		msg = "GAME_GODMODEOFF";
	} else {
		msg = "GAME_GODMODEON";
	}

	trap_SendServerCommand( ent - g_entities, 0, va( "e \"%s\"", msg ) );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	const char  *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if ( !( ent->flags & FL_NOTARGET ) ) {
		msg = "GAME_NOTARGETOFF";
	} else {
		msg = "GAME_NOTARGETON";
	}

	trap_SendServerCommand( ent - g_entities, 0, va( "e \"%s\"", msg ) );
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	const char  *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "GAME_NOCLIPOFF";
	} else {
		msg = "GAME_NOCLIPON";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent - g_entities, 0, va( "e \"%s\"", msg ) );
}

/*
==================
Cmd_UFO_f

argv(0) ufo
==================
*/
void Cmd_UFO_f( gentity_t *ent ) {
	const char  *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->ufo ) {
		msg = "GAME_UFOOFF";
	} else {
		msg = "GAME_UFOON";
	}
	ent->client->ufo = !ent->client->ufo;

	trap_SendServerCommand( ent - g_entities, 0, va( "e \"%s\"", msg ) );
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionState != STATE_PLAYING ) {
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	ent->client->ps.stats[STAT_HEALTH] = 0;
	player_die( ent, ent, ent, 100000, MOD_SUICIDE, 0, NULL, HITLOC_NONE );
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode.  The camera is dropped behind and above
the player that was being followed.
=================
*/
void StopFollowing( gentity_t *ent ) {
	gclient_t *client;
	vec3_t origin;
	vec3_t angles;
	vec3_t end;
	vec3_t forward, up;
	vec3_t mins, maxs;
	trace_t trace;

	client = ent->client;
	client->sess.forceSpectatorClient = -1;
	client->spectatorClient = -1;

	if ( !( client->ps.pm_flags & PMF_FOLLOW ) ) {
		return;
	}

	VectorCopy( client->ps.viewangles, angles );
	AngleVectors( angles, forward, NULL, up );
	angles[PITCH] = angles[PITCH] + 15.0f;

	VectorCopy( client->ps.origin, origin );
	origin[2] = origin[2] + client->ps.viewHeightCurrent;
	AddLeanToPosition( origin, ent->client->ps.viewangles[YAW],
					   ent->client->ps.leanf, 16.0, 20.0 );

	end[0] = origin[0] - forward[0] * 40.0f;
	end[1] = origin[1] - forward[1] * 40.0f;
	end[2] = origin[2] - forward[2] * 40.0f;

	mins[0] = -8.0;  mins[1] = -8.0;  mins[2] = -8.0;
	maxs[0] = 8.0;   maxs[1] = 8.0;   maxs[2] = 8.0;

	end[0] = end[0] + up[0] * 10.0f;
	end[1] = end[1] + up[1] * 10.0f;
	end[2] = end[2] + up[2] * 10.0f;

	trap_TraceCapsule( &trace, origin, mins, maxs, end, ENTITYNUM_NONE,
					   MASK_SPECTATOR_CAMERA );
	VectorCopy( trace.endpos, origin );

	client->ps.viewlocked = 0;
	client->ps.viewlocked_entNum = ENTITYNUM_NONE;
	client->ps.gunfx = 0;
	client->ps.fWeaponPosFrac = 0;
	client->ps.clientNum = ent - g_entities;
	client->ps.eFlags &= ~EF_TURRET_ACTIVE;
	client->ps.pm_flags &= ~( PMF_FOLLOW | PMF_ADS );

	G_SetOrigin( ent, origin );
	VectorCopy( origin, client->ps.origin );

	SetClientViewAngle( ent, angles );

	client->ps.shellshockIndex = 0;
	client->ps.shellshockTime = 0;
	client->ps.shellshockDuration = 0;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
qboolean Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int clientnum;
	int original;
	playerState_t ps;

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	if ( ent->client->sess.forceSpectatorClient >= 0 ) {
		return qfalse;
	}

	clientnum = ent->client->spectatorClient;
	if ( clientnum < 0 ) {
		clientnum = 0;
	}
	original = clientnum;

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		} else if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		if ( !trap_GetArchivedPlayerState( clientnum, &ent->client->sess.archiveTime, &ps ) ) {
			continue;
		}

		// this is good, we can use it
		ent->client->spectatorClient = clientnum;
		ent->client->sess.sessionState = STATE_SPECTATOR;
		return qtrue;
	} while ( clientnum != original );

	// leave it where it was
	return qfalse;
}

/*
==================
G_IsPlaying
==================
*/
qboolean G_IsPlaying( gentity_t *ent ) {
	return ent->client->sess.sessionState == STATE_PLAYING;
}

/*
==================
G_SayTo
==================
*/
void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color,
					 const char *name, const char *message ) {
	const char  *cmd;

	if ( !other ) {
		return;
	}
	if ( !other->inuse ) {
		return;
	}
	if ( !other->client ) {
		return;
	}
	if ( other->client->sess.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam( ent, other ) ) {
		return;
	}

	// the dead and spectators can't chat to the living
	if ( !G_IsPlaying( ent ) && G_IsPlaying( other ) ) {
		return;
	}

	if ( mode == SAY_TEAM ) {
		cmd = "i";
	} else {
		cmd = "h";
	}

	trap_SendServerCommand( other - g_entities, 0,
							va( "%s \"\x15%s%c%c%s\"", cmd, name,
								Q_COLOR_ESCAPE, color, message ) );
}

/*
==================
G_Say
==================
*/
void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int j;
	gentity_t   *other;
	int color;
	char name[64];
	char prefix[64];
	char location[64];
	char text2[128];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	const char  *teamName;

	if ( mode == SAY_TEAM ) {
		if ( ent->client->sess.sessionTeam != TEAM_AXIS
			 && ent->client->sess.sessionTeam != TEAM_ALLIES ) {
			mode = SAY_ALL;
		}
	}

	Q_strncpyz( name, ent->client->sess.name, sizeof( name ) );
	Q_CleanStr( name );

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		Com_sprintf( prefix, sizeof( prefix ), "\x15(\x14GAME_SPECTATOR\x15)" );
	} else if ( ent->health <= 0 ) {
		Com_sprintf( prefix, sizeof( prefix ), "\x15(\x14GAME_DEAD\x15)" );
	} else {
		Com_sprintf( prefix, sizeof( prefix ), "\x15" );
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", name, chatText );
		Com_sprintf( text2, sizeof( text2 ), "%s%s%s: ", prefix, name, "^7" );
		color = COLOR_WHITE;
		break;

	case SAY_TEAM:
		if ( ent->client->sess.sessionTeam == TEAM_AXIS ) {
			teamName = "GAME_AXIS";
		} else {
			teamName = "GAME_ALLIES";
		}
		G_LogPrintf( "sayteam: %s: %s\n", name, chatText );
		if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( text2, sizeof( text2 ), "%s(\x14%s\x15)%s%s (\x14%s\x15): ",
						 prefix, teamName, name, "^7", location );
		} else {
			Com_sprintf( text2, sizeof( text2 ), "%s(\x14%s\x15)%s%s: ",
						 prefix, teamName, name, "^7" );
		}
		color = COLOR_CYAN;
		break;

	case SAY_TELL:
		if ( target && target->client->sess.sessionTeam == ent->client->sess.sessionTeam
			 && Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( text2, sizeof( text2 ), "%s[%s]%s (%s): ",
						 prefix, name, "^7", location );
		} else {
			Com_sprintf( text2, sizeof( text2 ), "%s[%s]%s: ", prefix, name, "^7" );
		}
		color = COLOR_YELLOW;
		break;
	}

	Q_strncpyz( text, chatText, sizeof( text ) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, text2, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", text2, text );
	}

	// send it to all the apropriate clients
	for ( j = 0; j < level.maxclients; j++ ) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, text2, text );
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char        *p;

	if ( trap_Argc() < 2 && !arg0 ) {
		return;
	}

	if ( arg0 ) {
		p = ConcatArgs( 0 );
	} else {
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int targetNum;
	gentity_t   *target;
	char        *p;
	char arg[MAX_STRING_CHARS];

	if ( trap_Argc() < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->sess.name,
				 target->client->sess.name, p );
	G_Say( ent, target, SAY_TELL, p );
	G_Say( ent, ent, SAY_TELL, p );
}

/*
==================
G_VoiceTo
==================
*/
void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id,
					   qboolean voiceonly ) {
	int color;
	const char  *cmd;

	if ( !other ) {
		return;
	}
	if ( !other->inuse ) {
		return;
	}
	if ( !other->client ) {
		return;
	}
	if ( other->client->sess.connected != CON_CONNECTED ) {
		return;
	}

	if ( mode == SAY_TEAM ) {
		if ( !OnSameTeam( ent, other ) ) {
			return;
		}
		color = COLOR_CYAN;
		cmd = "k";
	} else if ( mode == SAY_TELL ) {
		color = COLOR_MAGENTA;
		cmd = "l";
	} else {
		color = COLOR_GREEN;
		cmd = "j";
	}

	trap_SendServerCommand( other - g_entities, 0,
							va( "%s %d %d %d %s %i %i %i", cmd, voiceonly,
								ent->s.number, color, id,
								(int)ent->s.pos.trBase[0],
								(int)ent->s.pos.trBase[1],
								(int)ent->s.pos.trBase[2] ) );
}

/*
==================
G_Voice
==================
*/
void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id,
			  qboolean voiceonly ) {
	int j;
	gentity_t   *other;

	// don't allow excessive spamming of voice chats
	ent->voiceChatSquelch -= ( level.time - ent->voiceChatPreviousTime );
	ent->voiceChatPreviousTime = level.time;

	if ( ent->voiceChatSquelch < 0 ) {
		ent->voiceChatSquelch = 0;
	}

	if ( ent->voiceChatSquelch >= VOICE_CHAT_SQUELCH_MAX ) {
		trap_SendServerCommand( ent - g_entities, 0,
								"e \"\x15^1\x14GAME_SPAMPROTECT\x15^7: \x14GAME_VOICECHATIGNORED\"" );
		return;
	}

	if ( g_voiceChatsAllowed.integer ) {
		ent->voiceChatSquelch += ( VOICE_CHAT_SQUELCH_SPAN / g_voiceChatsAllowed.integer );
	} else {
		return;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->sess.name, id );
	}

	// send it to all the apropriate clients
	for ( j = 0; j < level.maxclients; j++ ) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

/*
==================
Cmd_Voice_f
==================
*/
void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char        *p;

	if ( trap_Argc() < 2 && !arg0 ) {
		return;
	}

	if ( arg0 ) {
		p = ConcatArgs( 0 );
	} else {
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_GameCommand_f

Send one of the quick team orders to a player
==================
*/
static const char *gc_orders[] = {
	"GAME_GC_HOLDYOURPOSITION",
	"GAME_GC_HOLDTHISPOSITION",
	"GAME_GC_COMEHERE",
	"GAME_GC_COVERME",
	"GAME_GC_GUARDLOCATION",
	"GAME_GC_SEARCHDESTROY",
	"GAME_GC_REPORT"
};

static void Cmd_GameCommand_f( gentity_t *ent ) {
	int player;
	int order;
	char str[MAX_STRING_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	// keep: original bound test
	if ( order < 0 || order > sizeof( gc_orders ) / sizeof( char * ) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent - g_entities, 0,
							va( "e \"\x15%s\n\"", vtos( ent->r.currentOrigin ) ) );
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	int i;
	int clientNum;
	char arg1[256];
	char arg2[256];
	char arg3[256];
	char cleanName[64];
	vmCvar_t mapname;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_VOTINGNOTENABLED\"" );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_VOTEALREADYINPROGRESS\"" );
		return;
	}

	if ( ent->client->sess.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_MAXVOTESCALLED\"" );
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_NOSPECTATORCALLVOTE\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	if ( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_INVALIDVOTESTRING\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
	} else if ( !Q_stricmp( arg1, "map_rotate" ) ) {
	} else if ( !Q_stricmp( arg1, "typemap" ) ) {
	} else if ( !Q_stricmp( arg1, "map" ) ) {
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
	} else if ( !Q_stricmp( arg1, "clientkick" ) ) {
	} else {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_INVALIDVOTESTRING\"" );
		trap_SendServerCommand( ent - g_entities, 0,
								"e \"GAME_VOTECOMMANDSARE\x15 map_restart, map_rotate, "
								"map <mapname>, g_gametype <typename>, typemap <typename> "
								"<mapname>, kick <player>, clientkick <clientnum>\"" );
		return;
	}

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}

	if ( !Q_stricmp( arg1, "typemap" ) ) {
		if ( !Scr_IsValidGameType( arg2 ) ) {
			trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_INVALIDGAMETYPE\"" );
			return;
		}
		if ( !Q_stricmp( arg2, g_gametype.string ) ) {
			arg2[0] = 0;
		}

		trap_Argv( 3, arg3, sizeof( arg3 ) );
		trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		if ( !Q_stricmp( arg3, mapname.string ) ) {
			arg3[0] = 0;
		}

		if ( !arg2[0] && !arg3[0] ) {
			trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_TYPEMAP_NOCHANGE\"" );
			return;
		}

		if ( arg3[0] ) {
			if ( arg2[0] ) {
				Com_sprintf( level.voteString, sizeof( level.voteString ),
							 "g_gametype %s; map %s", arg2, arg3 );
			} else {
				Com_sprintf( level.voteString, sizeof( level.voteString ), "map %s", arg3 );
			}
			if ( arg2[0] ) {
				Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
							 "GAME_VOTE_GAMETYPE\x14%s\x15 - \x14GAME_VOTE_MAP\x15%s",
							 Scr_GetGameTypeNameForScript( arg2 ), arg3 );
			} else {
				Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
							 "GAME_VOTE_MAP\x15%s", arg3 );
			}
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ),
						 "g_gametype %s; map_restart", arg2 );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
						 "GAME_VOTE_GAMETYPE\x14%s", Scr_GetGameTypeNameForScript( arg2 ) );
		}
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
		if ( !Scr_IsValidGameType( arg2 ) ) {
			trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_INVALIDGAMETYPE\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ),
					 "%s %s; map_restart", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
					 "GAME_VOTE_GAMETYPE\x14%s", Scr_GetGameTypeNameForScript( arg2 ) );
	} else if ( !Q_stricmp( arg1, "map_restart" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
					 "GAME_VOTE_MAPRESTART" );
	} else if ( !Q_stricmp( arg1, "map_rotate" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
					 "GAME_VOTE_NEXTMAP" );
	} else if ( !Q_stricmp( arg1, "map" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
					 "GAME_VOTE_MAP\x15%s", arg2 );
	} else if ( !Q_stricmp( arg1, "kick" ) || !Q_stricmp( arg1, "clientkick" ) ) {
		clientNum = MAX_CLIENTS;

		if ( !Q_stricmp( arg1, "kick" ) ) {
			// resolve the name against every slot, not just the connected count
			for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
				if ( level.clients[i].sess.connected != CON_CONNECTED ) {
					continue;
				}
				Q_strncpyz( cleanName, level.clients[i].sess.name, sizeof( cleanName ) );
				Q_CleanStr( cleanName );
				if ( !Q_stricmp( cleanName, arg2 ) ) {
					clientNum = i;
				}
			}
		} else {
			clientNum = atoi( arg2 );
			if ( ( clientNum || !Q_stricmp( arg2, "0" ) ) && clientNum >= 0 && clientNum < MAX_CLIENTS
				 && level.clients[clientNum].sess.connected == CON_CONNECTED ) {
				Q_strncpyz( cleanName, level.clients[clientNum].sess.name, sizeof( cleanName ) );
				Q_CleanStr( cleanName );
			} else {
				clientNum = MAX_CLIENTS;
			}
		}

		if ( clientNum == MAX_CLIENTS ) {
			trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_CLIENTNOTONSERVER\"" );
			return;
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ),
					 "clientkick \"%d\"", clientNum );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
					 "GAME_VOTE_KICK\x15(%i)%s", clientNum,
					 level.clients[clientNum].sess.name );
	} else {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
					 "\x15%s", level.voteString );
	}

	trap_SendServerCommand( -1, 0,
							va( "e \"GAME_CALLEDAVOTE\x15%s\"", ent->client->sess.name ) );

	// start the voting, the caller automatically votes yes
	// voteTime is the time the vote ends, not when it started
	level.voteTime = level.time + VOTE_TIME;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char msg[64];
	int num;

	// Complaints supercede voting (and share command)
	if ( ent->client->sess.complaintEndTime > level.time ) {
		gclient_t *cl = g_entities[ ent->client->sess.complaintClient ].client;

		if ( !cl ) {
			return;
		}
		if ( cl->sess.connected != CON_CONNECTED ) {
			return;
		}
		if ( cl->sess.localClient ) {
			trap_SendServerCommand( ent - g_entities, 1, "m -3" );
			return;
		}

		// Reset this ent's complaintEndTime so they can't send multiple complaints
		ent->client->sess.complaintEndTime = -1;
		ent->client->sess.complaintClient = -1;

		trap_Argv( 1, msg, sizeof( msg ) );

		// keep: msg[1] in the second and third tests is the original behavior
		if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
			// Increase their complaint counter
			cl->sess.complaints++;

			num = g_complaintlimit.integer - cl->sess.complaints;

			if ( num <= 0 && !cl->sess.localClient ) {
				trap_DropClient( cl - level.clients, "GAME_KICKEDFROMCOMPLAINTS" );
				trap_SendServerCommand( ent - g_entities, 1, "m -1" );
				return;
			}

			trap_SendServerCommand( cl->ps.clientNum, 0,
									va( "e \"\x15^1\x14GAME_WARNING\x15^7: "
										"\x14GAME_COMPLAINTFILEDAGAINST\x15%d\"", num ) );
			trap_SendServerCommand( ent - g_entities, 1, "m -1" );
		} else {
			trap_SendServerCommand( ent - g_entities, 1, "m -2" );
		}

		return;
	}

	// Reset this ent's complaintEndTime so they can't send multiple complaints
	ent->client->sess.complaintEndTime = -1;
	ent->client->sess.complaintClient = -1;

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_NOVOTEINPROGRESS\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_VOTEALREADYCAST\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_NOSPECTATORVOTE\"" );
		return;
	}

	trap_SendServerCommand( ent - g_entities, 0, "e \"GAME_VOTECAST\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t origin, angles;
	char buffer[MAX_STRING_CHARS];
	int i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent - g_entities, 0, va( "e \"GAME_CHEATSNOTENABLED\"" ) );
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent - g_entities, 0,
								va( "e \"GAME_USAGE\x15: setviewpos x y z yaw\"" ) );
		return;
	}

	VectorClear( angles );

	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}

/*
=================
Cmd_Activate_f

Use whatever the player is looking at.  Called from ClientThink_real
when the activate button is pressed, not from ClientCommand.
=================
*/
qboolean Cmd_Activate_f( gentity_t *ent ) {
	gentity_t   *other;
	unsigned short entNum;
	qboolean found;

	found = qtrue;

	if ( !Scr_IsSystemActive( qtrue ) ) {
		return qfalse;
	}

	if ( ent->active ) {
		ent->active = ( ent->client->ps.eFlags & EF_TURRET_ACTIVE ) ? 2 : 0;
		return qtrue;
	}

	G_CheckForCursorHints( ent );

	entNum = ent->client->ps.serverCursorHintTrace.entityNum;
	if ( entNum == ENTITYNUM_NONE ) {
		return qfalse;
	}
	other = &g_entities[entNum];

	if ( other->classname ) {
		if ( other->classname == scr_const.func_door
			 || other->classname == scr_const.func_door_rotating ) {
			G_TryDoor( other, ent, ent );
		} else if ( other->classname == scr_const.trigger_use ) {
			Scr_AddEntityNum( ent->s.number, SCR_CLASS_ENTITY );
			Scr_NotifyNum( other->s.number, SCR_CLASS_ENTITY, scr_const.trigger, 1 );
			other->use( other, ent, ent );
		} else if ( other->s.eType == ET_ITEM ) {
			Scr_AddEntityNum( ent->s.number, SCR_CLASS_ENTITY );
			Scr_NotifyNum( other->s.number, SCR_CLASS_ENTITY, scr_const.touch, 1 );
			if ( other->touch ) {
				other->active = qtrue;
				other->touch( other, ent, 0 );
			} else {
				found = qfalse;
			}
		} else if ( other->s.eType == ET_TURRET ) {
			if ( !G_IsTurretUsable( other, ent ) ) {
				return qfalse;
			}
			other->use( other, ent, ent );
		} else if ( other->classname == scr_const.misc_flak && !other->active ) {
			if ( !infront( other, ent ) ) {
				if ( !level.clients[ ent->s.clientNum ].ps.grenadeTimeLeft ) {
					other->active = qtrue;
					ent->active = qtrue;
					other->r.ownerNum = ent->s.number;
					VectorCopy( other->r.currentAngles, other->TargetAngles );
				} else {
					found = qfalse;
				}
			} else {
				found = qfalse;
			}
		} else if ( other->classname == scr_const.script_brushmodel ) {
			Scr_AddEntityNum( ent->s.number, SCR_CLASS_ENTITY );
			Scr_NotifyNum( other->s.number, SCR_CLASS_ENTITY, scr_const.trigger, 1 );
			if ( other->use ) {
				other->use( other, ent, ent );
			} else {
				found = qfalse;
			}
		} else if ( other->classname == scr_const.team_WOLF_checkpoint ) {
			if ( other->count != ent->client->sess.sessionTeam ) {
				other->health++;
			}
		}
	}

	return found;
}

/*
=================
Cmd_EntityCount_f
=================
*/
void Cmd_EntityCount_f( gentity_t *ent ) {
	if ( g_cheats.integer ) {
		G_Printf( "entity count = %i\n", level.num_entities );
	}
}

/*
=================
Cmd_MenuResponse_f

Pass a menu response on to the player's script as a "menuresponse" notify
=================
*/
void Cmd_MenuResponse_f( gentity_t *ent ) {
	char menu[MAX_STRING_CHARS];
	char response[MAX_STRING_CHARS];
	char arg[MAX_STRING_CHARS];
	int menuIndex;

	if ( trap_Argc() != 4 ) {
		menu[0] = 0;
		strcpy( response, "bad" );
	} else {
		trap_Argv( 1, arg, sizeof( arg ) );
		if ( atoi( arg ) != trap_Cvar_VariableIntegerValue( "sv_serverId" ) ) {
			return;
		}

		trap_Argv( 2, menu, sizeof( menu ) );
		menuIndex = atoi( menu );
		if ( menuIndex >= 0 && menuIndex < CS_MENUS_COUNT ) {
			trap_GetConfigstring( menuIndex + CS_MENUS, menu, sizeof( menu ) );
		}

		trap_Argv( 3, response, sizeof( response ) );
	}

	Scr_AddString( response );
	Scr_AddString( menu );
	Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, scr_const.menuresponse, 2 );
}

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t   *ent;
	char cmd[MAX_STRING_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;     // not fully in game yet
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp( cmd, "say" ) == 0 ) {
		Cmd_Say_f( ent, SAY_ALL, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "say_team" ) == 0 ) {
		Cmd_Say_f( ent, SAY_TEAM, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "vsay" ) == 0 ) {
		Cmd_Voice_f( ent, SAY_ALL, qfalse, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "vsay_team" ) == 0 ) {
		Cmd_Voice_f( ent, SAY_TEAM, qfalse, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "tell" ) == 0 ) {
		Cmd_Tell_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "score" ) == 0 ) {
		DeathmatchScoreboardMessage( ent );
		return;
	}

	// ignore all other commands when at intermission
	if ( ent->client->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	if ( Q_stricmp( cmd, "mr" ) == 0 ) {
		Cmd_MenuResponse_f( ent );
	} else if ( Q_stricmp( cmd, "give" ) == 0 ) {
		Cmd_Give_f( ent );
	} else if ( Q_stricmp( cmd, "take" ) == 0 ) {
		Cmd_Take_f( ent );
	} else if ( Q_stricmp( cmd, "god" ) == 0 ) {
		Cmd_God_f( ent );
	} else if ( Q_stricmp( cmd, "notarget" ) == 0 ) {
		Cmd_Notarget_f( ent );
	} else if ( Q_stricmp( cmd, "noclip" ) == 0 ) {
		Cmd_Noclip_f( ent );
	} else if ( Q_stricmp( cmd, "ufo" ) == 0 ) {
		Cmd_UFO_f( ent );
	} else if ( Q_stricmp( cmd, "kill" ) == 0 ) {
		Cmd_Kill_f( ent );
	} else if ( Q_stricmp( cmd, "follownext" ) == 0 ) {
		Cmd_FollowCycle_f( ent, 1 );
	} else if ( Q_stricmp( cmd, "followprev" ) == 0 ) {
		Cmd_FollowCycle_f( ent, -1 );
	} else if ( Q_stricmp( cmd, "where" ) == 0 ) {
		Cmd_Where_f( ent );
	} else if ( Q_stricmp( cmd, "callvote" ) == 0 ) {
		Cmd_CallVote_f( ent );
	} else if ( Q_stricmp( cmd, "vote" ) == 0 ) {
		Cmd_Vote_f( ent );
	} else if ( Q_stricmp( cmd, "gc" ) == 0 ) {
		Cmd_GameCommand_f( ent );
	} else if ( Q_stricmp( cmd, "setviewpos" ) == 0 ) {
		Cmd_SetViewpos_f( ent );
	} else if ( Q_stricmp( cmd, "entitycount" ) == 0 ) {
		Cmd_EntityCount_f( ent );
	} else {
		trap_SendServerCommand( clientNum, 0,
								va( "e \"GAME_UNKNOWNCLIENTCOMMAND\x15%s\"", cmd ) );
	}
}
