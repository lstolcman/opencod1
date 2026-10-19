// g_client_mp.c -- client functions that don't happen every frame

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "g_local.h"

// the x/y extents are reset from g_bounds_width / g_bounds_height_standing
vec3_t playerMins = { -15, -15, 0 };
vec3_t playerMaxs = { 15, 15, 72 };

float AngleDelta( float angle1, float angle2 );

// not in q_shared.h
#define ANGLE2SHORT( x )            ( (int)( (x) * ( 65536.0f / 360.0f ) ) & 65535 )
#define SHORT2ANGLE( x )            ( (float)( x ) * ( 360.0f / 65536.0f ) )

#define Q_COLOR_ESCAPE              '^'
#define ColorIndex( c )             ( ( (c) - '0' ) & 7 )

// q_shared / q_math and bg_* functions not declared in a header
char        *Info_ValueForKey( const char *s, const char *key );
void        Q_strncpyz( char *dest, const char *src, int destsize );
int         Q_stricmp( const char *s1, const char *s2 );
float       AngleNormalize180( float angle );
float       AngleNormalize360( float angle );
void        AddLeanToPosition( vec3_t pos, float yaw, float leanFrac,
							   float leanDist, float scale );

/* forward: g_active_mp.c */
void        ClientThink_real( gentity_t *ent, usercmd_t *ucmd );
void        ClientEndFrame( gentity_t *ent );
void        G_SetClientContents( gentity_t *ent );

/* forward: g_cmds_mp.c */
void        StopFollowing( gentity_t *ent );
qboolean    Cmd_FollowCycle_f( gentity_t *ent, int dir );

/* forward: g_main_mp.c */
extern vmCvar_t g_password;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_bounds_width;
extern vmCvar_t g_bounds_height_standing;
extern vmCvar_t bg_viewheight_standing;
extern vmCvar_t bg_viewheight_crouched;
extern vmCvar_t bg_viewheight_prone;

/*
==================
SetClientViewAngle

While prone (and not on a turret) the view is clamped to +-45 degrees of yaw
around ps.proneDirection and -15..+45 of pitch around ps.proneTorsoPitch, with
the overshoot folded into ps.delta_angles so the client's prediction agrees.
==================
*/
void SetClientViewAngle( gentity_t *ent, const vec3_t angle ) {
	int cmdAngle;
	float diff;
	vec3_t newAngle;

	VectorCopy( angle, newAngle );

	if ( ( ent->client->ps.pm_flags & PMF_PRONE )
		 && !( ent->client->ps.eFlags & EF_TURRET_ACTIVE ) ) {

		diff = AngleDelta( ent->client->ps.proneDirection, newAngle[YAW] );
		diff = AngleNormalize180( diff );
		if ( diff > 45.0f || diff < -45.0f ) {
			if ( diff > 45.0f ) {
				diff -= 45.0f;
			} else {
				diff += 45.0f;
			}
			ent->client->ps.delta_angles[YAW] += ANGLE2SHORT( diff );
			if ( diff > 0.0f ) {
				newAngle[YAW] = AngleNormalize360( ent->client->ps.proneDirection - 45.0f );
			} else {
				newAngle[YAW] = AngleNormalize360( ent->client->ps.proneDirection + 45.0f );
			}
		}

		diff = AngleDelta( ent->client->ps.proneTorsoPitch, newAngle[PITCH] );
		diff = AngleNormalize180( diff );
		if ( diff > 45.0f || diff < -15.0f ) {
			if ( diff > 45.0f ) {
				diff -= 45.0f;
			} else {
				diff += 15.0f;
			}
			ent->client->ps.delta_angles[PITCH] += ANGLE2SHORT( diff );
			if ( diff > 0.0f ) {
				newAngle[PITCH] = AngleNormalize180( ent->client->ps.proneTorsoPitch - 45.0f );
			} else {
				newAngle[PITCH] = AngleNormalize180( ent->client->ps.proneTorsoPitch + 15.0f );
			}
		}
	}

	// set the delta angle
	cmdAngle = ANGLE2SHORT( newAngle[PITCH] );
	ent->client->ps.delta_angles[PITCH] = cmdAngle - ent->client->sess.cmd.angles[PITCH];
	cmdAngle = ANGLE2SHORT( newAngle[YAW] );
	ent->client->ps.delta_angles[YAW] = cmdAngle - ent->client->sess.cmd.angles[YAW];
	cmdAngle = ANGLE2SHORT( newAngle[ROLL] );
	ent->client->ps.delta_angles[ROLL] = cmdAngle - ent->client->sess.cmd.angles[ROLL];

	VectorCopy( newAngle, ent->r.currentAngles );
	VectorCopy( ent->r.currentAngles, ent->client->ps.viewangles );
}

/*
===========
ClientCleanName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize ) {
	int len, colorlessLen;
	char ch;
	char *p;
	int spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while ( 1 ) {
		ch = *in++;
		if ( !ch ) {
			break;
		}

		// don't allow leading spaces
		if ( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if ( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if ( !*in ) {
				break;
			}

			// don't allow black in a name, period
			if ( ColorIndex( *in ) == 0 ) {
				in++;
				continue;
			}

			// make sure room in dest for both chars
			if ( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if ( ch == ' ' ) {
			spaces++;
			if ( spaces > 3 ) {
				continue;
			}
		} else {
			spaces = 0;
		}

		if ( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if ( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "UnnamedPlayer", outSize );
	}
}

/*
===========
ClientUserinfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t   *ent;
	gclient_t   *client;
	clientInfo_t *ci;
	char        *s;
	char oldname[MAX_STRING_CHARS];
	char userinfo[MAX_STRING_CHARS];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( strchr( userinfo, '"' ) || strchr( userinfo, ';' ) ) {
		strcpy( userinfo, "\\name\\badinfo" );
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( s && !strcmp( s, "localhost" ) ) {
		client->sess.localClient = qtrue;
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	client->sess.predictItemPickup = atoi( s ) != 0;

	// set name
	if ( client->sess.connected == CON_CONNECTED && level.clientNameMode ) {
		// the script owns sess.name while renames are manual; only the
		// pending name changes here
		s = Info_ValueForKey( userinfo, "name" );
		ClientCleanName( s, client->sess.netname, sizeof( client->sess.netname ) );
	} else {
		Q_strncpyz( oldname, client->sess.name, sizeof( oldname ) );
		s = Info_ValueForKey( userinfo, "name" );
		ClientCleanName( s, client->sess.name, sizeof( client->sess.name ) );
		Q_strncpyz( client->sess.netname, client->sess.name, sizeof( client->sess.netname ) );
	}

	client->sess.handicap = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->sess.handicap < 1 || client->sess.handicap > 100 ) {
		client->sess.handicap = 100;
	}

	ci = &bg_clientinfo[clientNum];
	ci->clientNum = clientNum;
	Q_strncpyz( ci->name, client->sess.name, sizeof( ci->name ) );
	ci->team = client->sess.sessionTeam;
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.
============
*/
const char *ClientConnect( int clientNum, unsigned int scriptId ) {
	gentity_t   *ent;
	gclient_t   *client;
	clientInfo_t *ci;
	void        *animTree;
	char        *value;
	int entnum;
	char userinfo[MAX_STRING_CHARS];

	ent = g_entities + clientNum;
	client = level.clients + clientNum;

	memset( client, 0, sizeof( gclient_t ) );

	ci = &bg_clientinfo[clientNum];

	// the anim tree is the one thing in the animation record that has to
	// survive a reconnect
	animTree = ci->animTree;
	memset( ci, 0, sizeof( clientInfo_t ) );
	ci->animTree = animTree;
	ci->infoValid = qtrue;
	ci->nextValid = qtrue;

	client->sess.connected = CON_CONNECTING;
	client->sess.pers = (unsigned short)scriptId;
	client->sess.sessionTeam = TEAM_SPECTATOR;
	client->sess.sessionState = STATE_SPECTATOR;
	client->spectatorClient = -1;
	client->sess.forceSpectatorClient = -1;

	G_InitGentity( ent );
	ent->client = client;
	ent->touch = NULL;
	ent->pain = NULL;

	client->sess.clientNum = clientNum;
	client->ps.clientNum = clientNum;
	client->ps.eFlags = EF_CAPSULE;
	ent->r.svFlags = SVF_CAPSULE;
	client->sess.complaintClient = -1;
	client->sess.complaintEndTime = -1;

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for a password
	value = Info_ValueForKey( userinfo, "ip" );
	if ( strcmp( value, "localhost" ) ) {
		value = Info_ValueForKey( userinfo, "password" );
		if ( g_password.string[0]
			 && Q_stricmp( g_password.string, "none" )
			 && strcmp( g_password.string, value ) ) {
			G_FreeEntity( ent );
			return "GAME_INVALIDPASSWORD";
		}
	}

	entnum = ent->s.number;
	Scr_FreeThread( Scr_ExecEntThreadNum( entnum, SCR_CLASS_ENTITY, g_scr_data.playerConnect, 0 ) );

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum ) {
	gentity_t   *ent;

	level.clients[clientNum].sess.connected = CON_CONNECTED;

	// count current clients and rank for scoreboard
	CalculateRanks();

	ent = g_entities + clientNum;
	Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, scr_const.begin, 0 );
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
The spawn point is picked by the script and passed in.
============
*/
void ClientSpawn( gentity_t *ent, const vec3_t origin, const vec3_t angles ) {
	int index;
	gclient_t       *client;
	clientSession_t saved;
	int savedSpawnCount;
	int eFlags;
	usercmd_t ucmd;

	index = ent - g_entities;
	client = ent->client;

	if ( ( client->ps.pm_flags & PMF_INWORLD )
		 && ( client->ps.eFlags & EF_TURRET_ACTIVE ) ) {
		G_ClientStopUsingTurret( &level.gentities[client->ps.viewlocked_entNum] );
	}

	G_EntUnlink( ent );

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	Scr_SetString( &ent->classname, scr_const.player );

	ent->clipmask = MASK_CLIENTSOLID;
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->takedamage = qfalse;

	G_SetClientContents( ent );

	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = FL_SUPPORTS_LINKTO;

	VectorCopy( playerMins, ent->r.mins );
	VectorCopy( playerMaxs, ent->r.maxs );

	// clear everything but the persistant data (and the vote / teleport bits)
	eFlags = client->ps.eFlags & ( EF_VOTED | EF_TELEPORT_BIT );
	savedSpawnCount = client->ps.stats[STAT_SPAWN_COUNT];
	saved = client->sess;

	memset( client, 0, sizeof( gclient_t ) );

	client->sess = saved;
	// increment the spawncount so the client will detect the respawn
	// keep: statement order matches original codegen
	client->ps.stats[STAT_SPAWN_COUNT] = savedSpawnCount + 1;
	client->ps.stats[STAT_MAX_HEALTH] = client->sess.maxHealth;
	client->sess.clientNum = index;
	client->ps.clientNum = index;

	client->ps.eFlags = eFlags | EF_CAPSULE;
	client->spectatorClient = -1;

	trap_GetUsercmd( client - level.clients, &client->sess.cmd );

	// toggle the teleport bit so the client knows to not lerp
	client->ps.eFlags ^= EF_TELEPORT_BIT;

	// setup the bounding boxes and viewheights for prediction
	VectorCopy( ent->r.mins, client->ps.mins );
	VectorCopy( ent->r.maxs, client->ps.maxs );

	client->ps.proneViewHeight = bg_viewheight_prone.integer;
	client->ps.crouchViewHeight = bg_viewheight_crouched.integer;
	client->ps.standViewHeight = bg_viewheight_standing.integer;
	client->ps.deadViewHeight = 8;
	client->ps.viewHeightTarget = bg_viewheight_standing.integer;
	client->ps.viewHeightCurrent = (float)bg_viewheight_standing.integer;
	client->ps.viewHeightLerpTime = 0;
	client->ps.viewHeightLerpPosAdj = 0;

	client->ps.walkSpeedScale = 0.4f;
	client->ps.runSpeedScale = 1.0f;
	client->ps.proneSpeedScale = 0.15f;
	client->ps.crouchSpeedScale = 0.65f;
	client->ps.strafeSpeedScale = 0.8f;
	client->ps.backSpeedScale = 0.7f;
	client->ps.leanSpeedScale = 0.4f;
	client->ps.friction = 1.0f;

	G_SetOrigin( ent, origin );
	VectorCopy( origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	SetClientViewAngle( ent, angles );

	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;
	client->latched_wbuttons = 0;

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	memset( &ucmd, 0, sizeof( ucmd ) );
	ucmd.serverTime = level.time;
	ucmd.angles[PITCH] = -client->ps.delta_angles[PITCH];
	ucmd.angles[YAW] = -client->ps.delta_angles[YAW];
	ucmd.angles[ROLL] = -client->ps.delta_angles[ROLL];

	client->ps.commandTime = level.time - 100;

	ClientEndFrame( ent );
	ClientThink_real( ent, &ucmd );

	// positively link the client, even if the command times are weird
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t   *ent;
	gclient_t   *client;
	int i;
	int entnum;

	client = level.clients + clientNum;
	ent = g_entities + clientNum;

	// stop any menu the player has open
	Scr_AddString( "disconnect" );
	Scr_AddString( "-1" );
	Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, scr_const.menuresponse, 2 );

	// stop any following clients
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].sess.connected
			 && level.clients[i].sess.sessionState == STATE_SPECTATOR
			 && level.clients[i].spectatorClient == clientNum ) {
			if ( !Cmd_FollowCycle_f( &g_entities[i], 1 ) ) {
				StopFollowing( &g_entities[i] );
			}
		}
	}

	// remove complaint client
	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].sess.complaintClient == clientNum ) {
			level.clients[i].sess.complaintClient = -1;
			level.clients[i].sess.complaintEndTime = 0;
			trap_SendServerCommand( i, 1, "m -2" );
			break;
		}
	}

	HudElem_ClientDisconnect( ent );

	entnum = ent->s.number;
	Scr_FreeThread( Scr_ExecEntThreadNum( entnum, SCR_CLASS_ENTITY, g_scr_data.playerDisconnect, 0 ) );

	G_FreeEntity( ent );

	client->sess.connected = CON_DISCONNECTED;
	memset( &client->sess.clientNum, 0,
			sizeof( clientSession_t ) - offsetof( clientSession_t, clientNum ) );

	CalculateRanks();
}

/*
===========
G_SetPlayerSize

Push the two player-bounds cvars into the shared bounding box.
============
*/
void G_SetPlayerSize( void ) {
	playerMaxs[2] = g_bounds_height_standing.value;
	playerMins[0] = playerMins[1] = g_bounds_width.value * -0.5f;
	playerMaxs[0] = playerMaxs[1] = g_bounds_width.value * 0.5f;
}

/*
===========
G_AddLean

Slide a point sideways by the client's current lean, with the same 16-unit
reach and 20-unit pivot height the weapon and damage paths use.
============
*/
void G_AddLean( gentity_t *ent, vec3_t pos ) {
	AddLeanToPosition( pos, ent->client->ps.viewangles[YAW], ent->client->ps.leanf,
					   16.0f, 20.0f );
}

/*
===========
G_GetNonPVSFriendlyInfo

Pick one live team mate that is NOT in this client's snapshot and pack him into
ps.iCompassFriendInfo for the compass:

    bits  0..5   his entity number
    bits  6..14  (dx/4 + 255), dx clamped to [-1022,1024]
    bits 15..23  (dy/4 + 255)
    bits 24..31  his yaw as a byte

The scan resumes from the entity the previous frame reported, so over 64 frames
every out-of-snapshot team mate gets a turn.  A zero result means "nobody".
============
*/
int G_GetNonPVSFriendlyInfo( gentity_t *ent, const vec3_t origin, int lastEntNum ) {
	int team;
	int i, start;
	gentity_t   *other;
	int entNum;
	rfloat yaw;
	int packed;
	// keep: matches original codegen
	int ofs[2];
	rfloat delta[2];
	rfloat scale[2];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_FREE || team == TEAM_SPECTATOR ) {
		return 0;
	}

	if ( lastEntNum == ENTITYNUM_NONE ) {
		start = 0;
	} else {
		start = lastEntNum + 1;
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		other = &g_entities[( start + i ) % MAX_CLIENTS];

		if ( !other->inuse ) {
			continue;
		}
		if ( other->s.eType != ET_PLAYER ) {
			continue;
		}
		if ( other->s.eFlags & EF_DEAD ) {
			continue;
		}
		if ( !other->client || other->client->sess.sessionTeam != team ) {
			continue;
		}
		if ( trap_InSnapshot( origin, other->s.number ) ) {
			continue;
		}

		entNum = other->s.number;

		delta[0] = (rfloat)other->r.currentOrigin[0] - origin[0];
		delta[1] = other->r.currentOrigin[1] - origin[1];
		ofs[0] = (int)( delta[0] + 0.5f );
		ofs[1] = (int)( delta[1] + 0.5f );

		scale[0] = 1.0f;
		scale[1] = 1.0f;
		if ( ofs[0] > 1024 ) {
			scale[0] = 1024.0f / (rfloat)ofs[0];
		} else if ( ofs[0] < -1022 ) {
			scale[0] = -1022.0f / (rfloat)ofs[0];
		}

		if ( ofs[1] > 1024 ) {
			scale[1] = (float)( 1024.0f / (rfloat)ofs[1] );
		} else if ( ofs[1] < -1022 ) {
			scale[1] = (float)( -1022.0f / (rfloat)ofs[1] );
		}

		// scale the longer leg down so the shorter one keeps its direction
		if ( scale[0] < 1.0f || scale[1] < 1.0f ) {
			if ( scale[0] < scale[1] ) {
				ofs[1] = (int)( ofs[1] * scale[0] );
			} else if ( scale[1] < scale[0] ) {
				ofs[0] = (int)( ofs[0] * scale[1] );
			}
		}

		if ( ofs[0] > 1024 ) {
			ofs[0] = 1024;
		} else if ( ofs[0] < -1022 ) {
			ofs[0] = -1022;
		}
		if ( ofs[1] > 1024 ) {
			ofs[1] = 1024;
		} else if ( ofs[1] < -1022 ) {
			ofs[1] = -1022;
		}

		yaw = other->r.currentAngles[YAW];
		packed = ( ( ( ( ofs[1] + 2 ) / 4 + 255 ) << 15 ) & 0xFF8000 ) | ( ( ( ( ofs[0] + 2 ) / 4 + 255 ) << 6 ) & 0x7FC0 );
		// keep: matches original codegen
		goto compassYaw;
	compassYaw:
		return packed | ((int)(yaw * (256.0f / 360.0f)) << 24) | (entNum & 0x3F);
	}

	return 0;
}
