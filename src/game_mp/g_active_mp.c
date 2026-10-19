// g_active_mp.c -- per-client think and end-of-frame processing

#include <string.h>
#include <math.h>

#include "g_local.h"

// compass marker: set until gclient_t.pingTime runs out, and relayed to a
// viewer whose compass is showing that friendly
#define EF_COMPASS_MARK             0x00080000
#define EF_COMPASS_MARK_FRIEND      0x00100000

// set for a live player in the world, cleared for a dead one
#define SVF_VISIBLE_PLAYER          0x00000002

// content masks for dead players and spectators
#define MASK_DEADSOLID              0x00810011
#define MASK_SPECTATORSOLID         0x00800011

// everything G_TouchTriggers gathers (CONTENTS_TRIGGER and friends)
#define MASK_TRIGGERSEARCH          0x405C0008

// q_shared / q_math and bg_* functions not declared in a header
void        Q_strncpyz( char *dest, const char *src, int destsize );
void        vectoangles( const vec3_t value1, vec3_t angles );
void        AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
void        AxisToAngles( const vec3_t axis[3], vec3_t angles );
void        MatrixMultiply( const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3] );
void        MatrixTransformVector43( const vec3_t in, const void *mat, vec3_t out );
float       VectorNormalize2D( vec3_t v );

qboolean    BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );
float       BG_GetSpeed( const playerState_t *ps, int time );
void        BG_CalculateView_DamageKick( bgViewAngleState_t *state, vec3_t angles );
void        BG_CalculateView_Velocity( bgViewAngleState_t *state, vec3_t angles );
void        BG_CalculateWeaponAngles( pmWeaponAngleState_t *state, vec3_t angles );
#if defined(GAMEDLL)
void        BG_CalculateWeaponPosition_Sway( const playerState_t *ps, vec3_t previousViewAngles,
                                         vec3_t swayOffsets, vec3_t swayAngles, float scale, int msec );
#else
void        BG_CalculateWeaponPosition_Sway( const playerState_t *ps, vec3_t swayOffsets,
										 vec3_t swayAngles, vec3_t previousViewAngles, float scale, int msec );
#endif

/* forward: g_client_mp.c */
void        ClientSpawn( gentity_t *ent, const vec3_t origin, const vec3_t angles );
void        G_AddLean( gentity_t *ent, vec3_t pos );
int         G_GetNonPVSFriendlyInfo( gentity_t *ent, const vec3_t origin, int lastEntNum );

/* forward: g_cmds_mp.c */
void        StopFollowing( gentity_t *ent );
qboolean    Cmd_FollowCycle_f( gentity_t *ent, int dir );
qboolean    Cmd_Activate_f( gentity_t *ent );

/* forward: g_main_mp.c */
extern vmCvar_t g_inactivity;
extern vmCvar_t g_speed;
extern vmCvar_t g_gravity;
extern vmCvar_t g_synchronousClients;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;
extern vmCvar_t g_debugLocDamage;

/*
===============
P_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t   *client;
	float count;
	vec3_t angles;
	vec3_t axis[3];
	float kick;
	int percent;

	client = player->client;
	if ( client->ps.pm_type >= PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	if ( client->damage_blood <= 0 ) {
		return;
	}
	if ( client->sess.maxHealth <= 0 ) {
		return;
	}

	percent = 100 * client->damage_blood / client->sess.maxHealth;
	if ( percent > 127 ) {
		percent = 127;
	}
	count = (float)percent;

	client->ps.aimSpreadScale += count;
	if ( client->ps.aimSpreadScale > 255.0f ) {
		client->ps.aimSpreadScale = 255.0f;
	}

	kick = count * 0.2f;
	if ( kick < 5.0f ) {
		kick = 5.0f;
	} else if ( kick > 90.0f ) {
		kick = 90.0f;
	}

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->viewKickYaw = 0.0f;
		client->viewKickPitch = -kick;
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;
		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		AnglesToAxis( client->ps.viewangles, axis );
		client->viewKickYaw = -( DotProduct( axis[1], client->damage_from ) * kick );
		client->viewKickPitch = DotProduct( axis[0], client->damage_from ) * kick;
		client->ps.damagePitch = (int)( angles[PITCH] * ( 256.0f / 360.0f ) );
		client->ps.damageYaw = (int)( angles[YAW] * ( 256.0f / 360.0f ) );
	}

	// play an apropriate pain sound
	if ( level.time > player->painDebounceTime && !( player->flags & FL_GODMODE ) ) {
		int healthPercent = (int)( (float)client->ps.stats[STAT_HEALTH]
						 / (float)client->ps.stats[STAT_MAX_HEALTH] * 100.0f );
		if ( healthPercent < 0 ) {
			healthPercent = 0;
		} else if ( healthPercent > 100 ) {
			healthPercent = 100;
		}
		G_AddEvent( player, EV_PAIN, healthPercent );
		player->painDebounceTime = level.time + 700;
	}

	client->ps.damageEvent++;   // always increment this since we do multiple view damage anims
	client->viewKickStartTime = level.time - 20;
	client->ps.damageCount = percent;

	//
	// clear totals
	//
	client->damage_blood = 0;
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
	ent->s.loopSound = 0;
}

/*
==============
ClientImpacts

Run the touch callbacks and the script `touch` notifies for everything Pmove
bumped into.
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int i, j;
	gentity_t   *other;

	for ( i = 0; i < pm->numtouch; i++ ) {
		for ( j = 0; j < i; j++ ) {
			if ( pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if ( j != i ) {
			continue;       // duplicated
		}

		other = &g_entities[pm->touchents[i]];

		if ( Scr_IsSystemActive( 1 ) ) {
			Scr_AddEntityNum( other->s.number, SCR_CLASS_ENTITY );
			Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, scr_const.touch, 1 );
			Scr_AddEntityNum( ent->s.number, SCR_CLASS_ENTITY );
			Scr_NotifyNum( other->s.number, SCR_CLASS_ENTITY, scr_const.touch, 1 );
		}

		if ( other->touch ) {
			other->touch( other, ent, 1 );
		}

		if ( ent->touch ) {
			ent->touch( ent, other, 1 );
		}
	}
}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
============
*/
void G_TouchTriggers( gentity_t *ent ) {
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t   *hit;
	vec3_t mins, maxs;
	static vec3_t range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.pm_type > PM_NORMAL_LINKED ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES, MASK_TRIGGERSEARCH );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i = 0; i < num; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		if ( Scr_IsSystemActive( 1 ) ) {
			Scr_AddEntityNum( ent->s.number, SCR_CLASS_ENTITY );
			Scr_NotifyNum( hit->s.number, SCR_CLASS_ENTITY, scr_const.touch, 1 );
			Scr_AddEntityNum( hit->s.number, SCR_CLASS_ENTITY );
			Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, scr_const.touch, 1 );
		}

		if ( hit->touch ) {
			hit->touch( hit, ent, 1 );
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t pm;
	gclient_t   *client;

	client = ent->client;

	client->oldbuttons = client->buttons;
	client->buttons = client->sess.cmd.buttons;
	client->oldwbuttons = client->wbuttons;
	client->wbuttons = client->sess.cmd.wbuttons;

	if ( client->sess.forceSpectatorClient < 0 && client->spectatorClient >= 0
		 && ( ( client->buttons ^ client->oldbuttons ) & BUTTON_ADS ) ) {
		StopFollowing( ent );
	}

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && !( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	} else if ( ( client->buttons & BUTTON_MELEE )
				&& !( client->oldbuttons & BUTTON_MELEE ) ) {
		Cmd_FollowCycle_f( ent, -1 );
	}

	if ( client->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	client->ps.pm_type = PM_SPECTATOR;
	client->ps.speed = 400; // faster than normal

	// set up for pmove
	memset( &pm, 0, sizeof( pm ) );
	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	pm.tracemask = MASK_SPECTATORSOLID;   // spectators can fly through bodies
	pm.trace = trap_TraceCapsule;
	pm.trace2 = trap_TraceCapsule;
	pm.trace3 = trap_TraceCapsule;
	pm.pointcontents = trap_PointContents;

	Pmove( &pm );

	// save results of pmove
	VectorCopy( client->ps.origin, ent->r.currentOrigin );

	trap_UnlinkEntity( ent );
}

/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( !g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60000;
		client->inactivityWarning = qfalse;
	} else if ( client->sess.cmd.forwardmove || client->sess.cmd.rightmove
				|| client->sess.cmd.upmove || ( client->sess.cmd.buttons & BUTTON_ATTACK ) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->sess.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "GAME_DROPPEDFORINACTIVITY" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, 0,
									"c \"GAME_INACTIVEDROPWARNING\"" );
		}
	}
	return qtrue;
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gentity_t *ent ) {
	gclient_t   *client;

	client = ent->client;

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->sess.cmd.buttons;
	client->oldwbuttons = client->wbuttons;
	client->wbuttons = client->sess.cmd.wbuttons;
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int i;
	int event;
	int eventParm;
	gclient_t   *client;
	rfloat ratio;   // keep: matches original codegen
	int weapon;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_EVENTS;
	}

	for ( i = oldEventSequence; i < client->ps.eventSequence; i++ ) {
		event = client->ps.events[i & ( MAX_EVENTS - 1 )];
		eventParm = client->ps.eventParms[i & ( MAX_EVENTS - 1 )];

		if ( event >= EV_LANDING_PAIN_DEFAULT && event < EV_FOLIAGE_SOUND ) {
			// not in the player model (this ends the whole event loop)
			if ( ent->s.eType != ET_PLAYER ) {
				return;
			}
			if ( eventParm >= 100 ) {
				ratio = 1.1f;
			} else {
				ratio = (rfloat)eventParm * 0.01f;
			}
			if ( ratio != 0.0f ) {
				ratio *= client->ps.stats[STAT_MAX_HEALTH];
				ent->painDebounceTime = level.time + 200;
				G_Damage( ent, NULL, NULL, NULL, NULL, (int)ratio, 0, MOD_FALLING, HITLOC_NONE );
			}
			continue;
		}

		switch ( event ) {
		case EV_FIRE_WEAPON:
		case EV_FIRE_WEAPONB:
		case EV_FIRE_WEAPON_LASTSHOT:
		case EV_FIRE_WEAPON_MG42:
			FireWeapon( ent );
			break;

		case EV_FIRE_MELEE:
			FireWeaponMelee( ent );
			break;

		case EV_DROPWEAPON:
			weapon = ent->client->ps.weapon;
			if ( weapon && ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) ) {
				Drop_Weapon( ent, weapon, "tag_weapon_right" );
			}
			break;

		default:
			break;
		}
	}
}

/*
==============
G_SetClientContents
==============
*/
void G_SetClientContents( gentity_t *ent ) {
	if ( ent->client->noclip ) {
		ent->r.contents = 0;
	} else if ( ent->client->ufo ) {
		ent->r.contents = 0;
	} else if ( ent->client->sess.sessionState == STATE_DEAD ) {
		ent->r.contents = 0;
	} else {
		ent->r.contents = CONTENTS_BODY;
	}
}

/*
==============
ClientThink_real

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent, usercmd_t *ucmd ) {
	gclient_t   *client;
	pmove_t pm;
	int oldEventSequence;
	int msec;
	bgViewAngleState_t viewState;
	pmWeaponAngleState_t weaponState;
	vec3_t angles;
	vec3_t aimAngles;
	vec3_t weaponAxis[3], aimAxis[3], axis[3];

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if ( client->sess.connected != CON_CONNECTED ) {
		return;
	}

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->ps.clientNum == ent - g_entities ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set( "pmove_msec", "8" );
	} else if ( pmove_msec.integer > 33 ) {
		trap_Cvar_Set( "pmove_msec", "33" );
	}

	if ( pmove_fixed.integer || client->sess.pmoveFixed ) {
		ucmd->serverTime = ( ( ucmd->serverTime + pmove_msec.integer - 1 ) / pmove_msec.integer )
						   * pmove_msec.integer;
	}

	//
	// check for exiting intermission
	//
	if ( client->sess.sessionState == STATE_INTERMISSION ) {
		ClientIntermissionThink( ent );
		return;
	}

	// spectators don't do much
	if ( client->sess.sessionState == STATE_SPECTATOR ) {
		SpectatorThink( ent, ucmd );
		return;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

	oldEventSequence = client->ps.eventSequence;

	// set up for pmove
	memset( &pm, 0, sizeof( pm ) );
	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	pm.oldcmd = client->sess.oldcmd;
	if ( client->ps.pm_type >= PM_DEAD ) {
		pm.tracemask = MASK_DEADSOLID;
	} else {
		pm.tracemask = MASK_CLIENTSOLID;
	}
	pm.pointcontents = trap_PointContents;
	pm.trace = trap_TraceCapsule;
	pm.trace2 = trap_TraceCapsule;
	pm.trace3 = trap_TraceCapsule;
	pm.debugLevel = g_debugMove.integer;
	pm.pmove_fixed = pmove_fixed.integer | client->sess.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

	VectorCopy( client->ps.origin, client->oldOrigin );

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->sess.cmd.buttons;
	client->latched_buttons = client->buttons & ~client->oldbuttons;
	client->oldwbuttons = client->wbuttons;
	client->wbuttons = client->sess.cmd.wbuttons;
	client->latched_wbuttons = client->wbuttons & ~client->oldwbuttons;

	// the damage kick the view inherits, and the weapon offsets and recoil the
	// fire path aims down
	viewState.ps = &client->ps;
	viewState.viewKickStartTime = client->viewKickStartTime;
	viewState.time = level.time;
	viewState.viewKickPitch = client->viewKickPitch;
	viewState.viewKickRoll = client->viewKickYaw;
	viewState.speed = BG_GetSpeed( &client->ps, level.time );

	// same as BG_CalculateViewAngles
	VectorClear( angles );
	BG_CalculateView_DamageKick( &viewState, angles );
	BG_CalculateView_Velocity( &viewState, angles );

	aimAngles[PITCH] = angles[PITCH] + client->ps.viewangles[PITCH];
	aimAngles[YAW] = angles[YAW] + client->ps.viewangles[YAW];
	aimAngles[ROLL] = angles[ROLL] + client->ps.viewangles[ROLL];

#if defined(GAMEDLL)
	BG_CalculateWeaponPosition_Sway( &client->ps, client->previousViewAngles, client->swayOffsets,
                                     client->swayAngles, 1.0f, msec );
#else
	BG_CalculateWeaponPosition_Sway( &client->ps, client->swayOffsets, client->swayAngles,
									 client->previousViewAngles, 1.0f, msec );
#endif

	weaponState.ps = &client->ps;
	weaponState.speed = viewState.speed;
	weaponState.frametime = (float)msec * 0.001f;
	VectorCopy( client->moveOffset, weaponState.moveOffset );
	weaponState.idleScale = client->idleScale;
	weaponState.time = viewState.time;
	weaponState.viewKickStartTime = client->viewKickStartTime;
	weaponState.viewKickPitch = client->viewKickPitch;
	weaponState.viewKickYaw = client->viewKickYaw;
	weaponState.recoilPitch = client->recoilPitch;
	weaponState.recoilYaw = client->recoilYaw;
	weaponState.recoilRoll = client->recoilRoll;
	weaponState.recoilPitchSpeed = client->recoilPitchSpeed;
	weaponState.recoilYawSpeed = client->recoilYawSpeed;
	weaponState.recoilState = client->recoilState;
	// baseAngles is the sway angle, not previousViewAngles: BG_CalculateWeaponAngles
	// subtracts it, leaving a small view-relative offset
	VectorCopy( client->swayAngles, weaponState.baseAngles );

	BG_CalculateWeaponAngles( &weaponState, angles );

	// an aim-down-sight weapon aims along the gun, not along the view
	if ( bg_weaponInfo[weaponState.ps->weapon]->aimDownSight && weaponState.ps->fWeaponPosFrac != 0.0f ) {
		AnglesToAxis( angles, weaponAxis );
		AnglesToAxis( aimAngles, aimAxis );
		// weapon offset first, then the view: the other order would aim every
		// ADS shot at a fixed world direction
		MatrixMultiply( weaponAxis, aimAxis, axis );
		AxisToAngles( axis, aimAngles );
	}

	VectorCopy( weaponState.moveOffset, client->moveOffset );
	client->idleScale = weaponState.idleScale;
	client->recoilPitch = weaponState.recoilPitch;
	client->recoilYaw = weaponState.recoilYaw;
	client->recoilRoll = weaponState.recoilRoll;
	client->recoilPitchSpeed = weaponState.recoilPitchSpeed;
	client->recoilYawSpeed = weaponState.recoilYawSpeed;
	client->recoilState = weaponState.recoilState;

	client->fGunPitch = aimAngles[PITCH];
	client->fGunYaw = aimAngles[YAW];

	Pmove( &pm );

	// set animMovetype to 1 if ducking
	if ( ent->client->ps.pm_flags & PMF_DUCKED ) {
		ent->s.animMovetype = 1;
	} else {
		ent->s.animMovetype = 0;
	}

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
		ent->r.eventTime = level.time;
	}

	if ( g_smoothClients.integer ) {
		BG_PlayerStateToEntityStateExtrapolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
	} else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );
	VectorCopy( pm.mins, ent->r.mins );
	VectorCopy( pm.maxs, ent->r.maxs );

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity( ent );

	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	VectorClear( ent->r.currentAngles );
	ent->r.currentAngles[YAW] = ent->client->ps.viewangles[YAW];

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}

	// activate is a latched event (occurs on keydown only)
	if ( client->latched_buttons & BUTTON_ACTIVATE ) {
		Cmd_Activate_f( ent );
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t   *ent;

	ent = g_entities + clientNum;

	ent->client->sess.oldcmd = ent->client->sess.cmd;
	trap_GetUsercmd( clientNum, &ent->client->sess.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !g_synchronousClients.integer ) {
		ClientThink_real( ent, &ent->client->sess.cmd );
	}
}

/*
==================
G_RunClient
==================
*/
void G_RunClient( gentity_t *ent ) {
	if ( g_synchronousClients.integer ) {
		ent->client->sess.cmd.serverTime = level.time;
		ClientThink_real( ent, &ent->client->sess.cmd );
	}

	if ( ent->client->noclip ) {
		return;
	}

	if ( ent->linkInfo ) {
		if ( ent->client->sess.sessionState == STATE_DEAD ) {
			ent->client->ps.pm_type = PM_DEAD_LINKED;
		} else {
			ent->client->ps.pm_type = PM_NORMAL_LINKED;
		}

		G_SetFixedLink( ent, 2 );

		G_SetOrigin( ent, ent->r.currentOrigin );
		G_SetAngle( ent, ent->r.currentAngles );
		ent->s.pos.trType = TR_INTERPOLATE;
		ent->s.apos.trType = TR_INTERPOLATE;

		trap_LinkEntity( ent );

		VectorCopy( ent->r.currentOrigin, ent->client->ps.origin );
	} else if ( ent->client->ps.pm_type == PM_NORMAL_LINKED
				|| ent->client->ps.pm_type == PM_DEAD_LINKED ) {
		ent->client->ps.pm_type--;
	}
}

/*
==================
IntermissionClientEndFrame
==================
*/
void IntermissionClientEndFrame( gentity_t *ent ) {
	gclient_t   *client;

	client = ent->client;

	ent->r.svFlags = ( ent->r.svFlags & ~( SVF_NOCLIENT | SVF_VISIBLE_PLAYER ) ) | SVF_NOCLIENT;
	ent->takedamage = qfalse;
	ent->r.contents = 0;

	client->ps.pm_flags &= ~PMF_INWORLD;
	client->ps.eFlags &= ~EF_TALK;
	client->ps.pm_type = PM_INTERMISSION;
	client->ps.eFlags &= ~EF_FIRING;
	client->ps.viewmodelIndex = 0;
}

/*
==================
SpectatorClientEndFrame

A spectator does not run a playerState of its own: it replays the archived one
of the client it is following, keeping only its own EF_VOTED bit.
==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t   *client;
	playerState_t archived;
	int savedEFlags;

	// keep: matches original codegen (only the first group goes through client)
	client = ent->client;

	ent->takedamage = qfalse;
	ent->r.contents = 0;
	ent->r.svFlags = ( ent->r.svFlags & ~SVF_VISIBLE_PLAYER ) | SVF_NOCLIENT;

	client->ps.pm_flags &= ~PMF_INWORLD;
	client->ps.viewmodelIndex = 0;
	client->fGunPitch = 0.0f;
	client->fGunYaw = 0.0f;

	if ( ent->client->sess.forceSpectatorClient >= 0 ) {
		ent->client->spectatorClient = ent->client->sess.forceSpectatorClient;

		while ( 1 ) {
			if ( ent->client->sess.archiveTime < 0 ) {
				ent->client->sess.archiveTime = 0;
			}
			if ( trap_GetArchivedPlayerState( ent->client->sess.forceSpectatorClient,
											  &ent->client->sess.archiveTime, &archived ) ) {
				break;
			}
			if ( !ent->client->sess.archiveTime ) {
				ent->client->sess.forceSpectatorClient = -1;
				ent->client->spectatorClient = -1;
				goto follow;
			}
			ent->client->sess.archiveTime -= 50;
		}
	} else {
follow:
		if ( ent->client->spectatorClient < 0
			 || !trap_GetArchivedPlayerState( ent->client->spectatorClient,
											  &ent->client->sess.archiveTime, &archived ) ) {
			StopFollowing( ent );
			return;
		}
	}

	savedEFlags = archived.eFlags ^ ( ( archived.eFlags ^ ent->client->ps.eFlags ) & EF_VOTED );

	ent->client->ps = archived;

	HudElem_UpdateClient( ent->client, ent->s.number, HE_UPDATE_CURRENT );

	ent->client->ps.pm_flags &= ~PMF_INWORLD;
	ent->client->ps.pm_flags |= PMF_FOLLOW;
	if ( ent->client->sess.forceSpectatorClient < 0 ) {
		ent->client->ps.pm_flags &= ~PMF_FOLLOWLOCKED;
	} else {
		ent->client->ps.pm_flags |= PMF_FOLLOWLOCKED;
	}

	ent->client->ps.eFlags = savedEFlags;
}

/*
==================
GetFollowPlayerState

Hand the engine a copy of one client's playerState for the snapshot
archive, with the hud stripped.
==================
*/
int GetFollowPlayerState( int clientNum, playerState_t *ps ) {
	gclient_t   *client;

	client = g_entities[clientNum].client;

	if ( !( client->ps.pm_flags & PMF_INWORLD ) ) {
		return qfalse;
	}

	*ps = client->ps;
	memset( ps->hud.current, 0, sizeof( ps->hud.current ) );

	return qtrue;
}

/*
==================
StuckInClient

Two live clients wedged inside one another get pushed apart along the line
between them.  Boxes overlap by their bounds; a capsule against a box is
tested by walking the capsule's radius along the separating direction, and two
capsules by the sum of their radii.
==================
*/
qboolean StuckInClient( gentity_t *ent ) {
	int i;
	gentity_t   *other;
	vec3_t hitmin, hitmax;
	vec3_t selfmin, selfmax;
	vec3_t dir;
	float normal[2];
	rfloat radius;      // keep: matches original codegen
	float angle;
	float *sinOut, *cosOut;
	float speed, otherSpeed;

	if ( !( ent->client->ps.pm_flags & PMF_INWORLD ) ) {
		return qfalse;
	}
	if ( ent->client->sess.sessionState != STATE_PLAYING ) {
		return qfalse;
	}
	if ( ent->r.contents != CONTENTS_BODY && ent->r.contents != CONTENTS_CORPSE ) {
		return qfalse;
	}

	other = g_entities;
	for ( i = 0; i < level.maxclients; i++, other++ ) {
		if ( !other->inuse ) {
			continue;
		}
		// note: this bails out of the whole scan instead of skipping the entity
		if ( !( other->client->ps.pm_flags & PMF_INWORLD ) ) {
			return qfalse;
		}
		if ( other->client->sess.sessionState != STATE_PLAYING ) {
			continue;
		}
		if ( other == ent ) {
			continue;
		}
		if ( !other->client ) {
			continue;
		}
		if ( other->health <= 0 ) {
			continue;
		}
		if ( other->r.contents != CONTENTS_BODY && other->r.contents != CONTENTS_CORPSE ) {
			continue;
		}

		VectorAdd( other->r.mins, other->r.currentOrigin, hitmin );
		VectorAdd( other->r.maxs, other->r.currentOrigin, hitmax );
		VectorAdd( ent->r.mins, ent->r.currentOrigin, selfmin );
		VectorAdd( ent->r.maxs, ent->r.currentOrigin, selfmax );

		if ( hitmin[0] > selfmax[0] || hitmax[0] < selfmin[0]
			 || hitmin[1] > selfmax[1] || hitmax[1] < selfmin[1]
			 || hitmin[2] > selfmax[2] || hitmax[2] < selfmin[2] ) {
			continue;
		}

		if ( ent->r.svFlags & SVF_CAPSULE ) {
			if ( other->r.svFlags & SVF_CAPSULE ) {
				dir[0] = other->r.currentOrigin[0] - ent->r.currentOrigin[0];
				dir[1] = other->r.currentOrigin[1] - ent->r.currentOrigin[1];
				// keep: matches original codegen
				radius = other->r.maxs[0];
				radius += ent->r.maxs[0];
				if ( dir[1] * dir[1] + dir[0] * dir[0] > radius * radius ) {
					continue;
				}
			} else {
				dir[0] = other->r.currentOrigin[0] - ent->r.currentOrigin[0];
				dir[1] = other->r.currentOrigin[1] - ent->r.currentOrigin[1];
				normal[0] = dir[0];
				normal[1] = dir[1];
				VectorNormalize2D( normal );
				dir[0] = dir[0] + normal[0] * ( ent->r.maxs[0] * -1.0f );
				dir[1] = dir[1] + normal[1] * ( ent->r.maxs[0] * -1.0f );
				if ( fabs( dir[0] ) > other->r.maxs[0] && fabs( dir[1] ) > other->r.maxs[1] ) {
					continue;
				}
			}
		} else if ( other->r.svFlags & SVF_CAPSULE ) {
			dir[0] = ent->r.currentOrigin[0] - other->r.currentOrigin[0];
			dir[1] = ent->r.currentOrigin[1] - other->r.currentOrigin[1];
			normal[0] = dir[0];
			normal[1] = dir[1];
			VectorNormalize2D( normal );
			dir[0] = dir[0] + normal[0] * ( other->r.maxs[0] * -1.0f );
			dir[1] = dir[1] + normal[1] * ( other->r.maxs[0] * -1.0f );
			if ( fabs( dir[0] ) > ent->r.maxs[0] && fabs( dir[1] ) > ent->r.maxs[1] ) {
				continue;
			}
		}

		dir[0] = other->r.currentOrigin[0] - ent->r.currentOrigin[0];
		dir[1] = other->r.currentOrigin[1] - ent->r.currentOrigin[1];
		if ( fabs( dir[0] ) < 0.01f && fabs( dir[1] ) < 0.01f ) {
			// exactly on top of each other: pick a direction off the entity
			// number so the two ends disagree
			angle = (float)ent->s.number * 0.1f;
			// keep: matches original codegen
			cosOut = &dir[1];
			sinOut = &dir[0];
			__asm {
				push    eax
				push    ecx
				fld     angle
				fsincos
				mov     ecx, cosOut
				mov     eax, sinOut
				fstp    dword ptr [ecx]
				fstp    dword ptr [eax]
				pop     ecx
				pop     eax
			}
		}
		VectorNormalize2D( dir );

		// keep: matches original codegen
		if ( FCAST( sqrt( other->client->ps.velocity[0] * other->client->ps.velocity[0]
						  + other->client->ps.velocity[1] * other->client->ps.velocity[1]
						  + other->client->ps.velocity[2] * other->client->ps.velocity[2] ) ) > 0.0f ) {
			otherSpeed = (float)other->client->ps.speed;
		} else {
			otherSpeed = 0.0f;
		}
		if ( FCAST( sqrt( ent->client->ps.velocity[0] * ent->client->ps.velocity[0]
						  + ent->client->ps.velocity[1] * ent->client->ps.velocity[1]
						  + ent->client->ps.velocity[2] * ent->client->ps.velocity[2] ) ) > 0.0f ) {
			speed = (float)ent->client->ps.speed;
		} else {
			speed = 0.0f;
		}
		if ( otherSpeed < 0.0001f && speed < 0.0001f ) {
			otherSpeed = (float)other->client->ps.speed;
			speed = (float)ent->client->ps.speed;
		}

		// note: dir[2] is never set, so the vertical push is undefined
		VectorScale( dir, otherSpeed, other->client->ps.velocity );
		other->client->ps.pm_time = 300;
		other->client->ps.pm_flags |= PMF_TIME_LAND;

		VectorScale( dir, -speed, ent->client->ps.velocity );
		ent->client->ps.pm_time = 300;
		ent->client->ps.pm_flags |= PMF_TIME_LAND;

		return qtrue;
	}

	return qfalse;
}

/*
==============
G_PlayerController

gentity_t.controller for a player; G_DObjCalcPose calls it once the bone list
is known.
==============
*/
void G_PlayerController( gentity_t *self, unsigned int *partBits ) {
	// keep: matches original codegen
	struct {
		clientInfo_t *ci;
		gentity_t *ent;
	} player;
	player.ci = &bg_clientinfo[self->s.clientNum];
	player.ent = self;
	BG_Player_DoControllers( player.ent, player.ci, &player.ent->s, (int *)partBits );
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEndFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	gclient_t   *client;
	clientInfo_t *ci;
	const char  *name;
	const char  *tagName;
	qboolean dobjChanged;
	int i;
	int info;
	vec3_t origin;

	client = ent->client;
	ci = NULL;      // keep: matches original codegen

	ent->controller = NULL;
	client->ps.deltaTime = 0;

	if ( client->sess.connected != CON_CONNECTED ) {
		return;
	}

	if ( client->sess.sessionState == STATE_INTERMISSION ) {
		IntermissionClientEndFrame( ent );
		return;
	}

	if ( client->sess.sessionState == STATE_SPECTATOR ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	if ( client->ps.clientNum != ent->s.number ) {
		vec3_t spawnOrigin;
		vec3_t spawnAngles;
		// the playerState still belongs to whoever this client was following:
		// put him back in the world
		VectorCopy( client->ps.origin, spawnOrigin );
		VectorSet( spawnAngles, 0, client->ps.viewangles[YAW], 0 );
		ClientSpawn( ent, spawnOrigin, spawnAngles );
		return;
	}

	ent->takedamage = qtrue;
	ent->r.svFlags = ( ent->r.svFlags & ~SVF_NOCLIENT ) | SVF_VISIBLE_PLAYER;
	client->ps.pm_flags |= PMF_INWORLD;
	client->ps.viewmodelIndex = client->sess.viewmodel;

	G_SetClientContents( ent );

	client->dropWeaponTime = 0;
	if ( client->pingTime <= level.time ) {
		client->ps.eFlags &= ~EF_COMPASS_MARK;
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ufo ) {
		client->ps.pm_type = PM_UFO;
	} else if ( client->sess.sessionState == STATE_DEAD ) {
		client->ps.pm_type = ent->linkInfo ? PM_DEAD_LINKED : PM_DEAD;
		ent->r.svFlags = ( ent->r.svFlags & ~( SVF_NOCLIENT | SVF_VISIBLE_PLAYER ) )
						 | SVF_NOCLIENT;
		ent->takedamage = qfalse;
	} else {
		client->ps.pm_type = ent->linkInfo ? PM_NORMAL_LINKED : PM_NORMAL;
	}

	client->ps.gravity = (int)g_gravity.value;
	client->ps.speed = (int)g_speed.value;
	client->currentAimSpreadScale = client->ps.aimSpreadScale * ( 1.0 / 255.0 );

	G_CheckForPreventFriendlyFire( ent );
	G_CheckForCursorHints( ent );

	// apply all the damage taken this frame
	P_DamageFeedback( ent );

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - client->lastCmdTime > 1000 ) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}

	client->ps.stats[STAT_HEALTH] = ent->health;

	G_SetClientSound( ent );

	// set the latest infor
	if ( g_smoothClients.integer ) {
		BG_PlayerStateToEntityStateExtrapolate( &client->ps, &ent->s,
												client->ps.commandTime, qtrue );
	} else {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
	}

	if ( ent->health > 0 && StuckInClient( ent ) ) {
		ent->r.contents = CONTENTS_CORPSE;
	}

	VectorCopy( client->ps.origin, origin );
	origin[2] += client->ps.viewHeightCurrent;
	G_AddLean( ent, origin );

	info = G_GetNonPVSFriendlyInfo( ent, origin, client->iLastCompassPlayerInfoEnt );
	client->ps.iCompassFriendInfo = info;
	if ( info ) {
		client->iLastCompassPlayerInfoEnt = info & 0x3F;
		if ( g_entities[info & 0x3F].s.eFlags & EF_COMPASS_MARK ) {
			client->ps.eFlags |= EF_COMPASS_MARK_FRIEND;
		} else {
			client->ps.eFlags &= ~EF_COMPASS_MARK_FRIEND;
		}
	} else {
		client->iLastCompassPlayerInfoEnt = ENTITYNUM_NONE;
	}

	if ( ent->s.eType == ET_PLAYER ) {
		ci = &bg_clientinfo[ent->s.clientNum];

		ent->controller = G_PlayerController;

		ci->leanAmount = ent->s.angles2[YAW];
		ci->leanFraction = ent->s.leanf;
		ci->viewPitch = client->ps.viewangles[PITCH];
		ci->viewYaw = client->ps.viewangles[YAW];
		ci->viewRoll = client->ps.viewangles[ROLL];

		dobjChanged = qfalse;

		name = G_ModelName( ent->model );
		client->sess.modelIndex = ent->model;
		if ( strcmp( ci->modelName, name ) ) {
			dobjChanged = qtrue;
			Q_strncpyz( ci->modelName, name, sizeof( ci->modelName ) );
		}

		for ( i = 0; i < MAX_ATTACHED_MODELS; i++ ) {
			if ( !ent->attachModelIndex[i] ) {
				ci->attachModelNames[i][0] = 0;
				ci->attachTagNames[i][0] = 0;
				client->sess.attachModelIndex[i] = 0;
				client->sess.attachTagIndex[i] = 0;
			} else {
				name = trap_GetConfigstringConst( CS_MODELS + ent->attachModelIndex[i] );
				client->sess.attachModelIndex[i] = ent->attachModelIndex[i];
				if ( strcmp( ci->attachModelNames[i], name ) ) {
					dobjChanged = qtrue;
					Q_strncpyz( ci->attachModelNames[i], name,
								sizeof( ci->attachModelNames[i] ) );
				}

				tagName = SL_ConvertToString( ent->attachTagName[i] );
				client->sess.attachTagIndex[i] = G_TagIndex( tagName );
				if ( strcmp( ci->attachTagNames[i], tagName ) ) {
					dobjChanged = qtrue;
					Q_strncpyz( ci->attachTagNames[i], tagName,
								sizeof( ci->attachTagNames[i] ) );
				}
			}
		}

		if ( dobjChanged ) {
			trap_SafeDObjFree( ent->s.number, qfalse );
		}

		BG_UpdatePlayerDObj( ent, &ent->s, ci );
		BG_PlayerAnimation( ent, &ent->s, ci );

		if ( ( client->ps.pm_flags & PMF_INWORLD )
			 && ( client->ps.eFlags & EF_TURRET_ACTIVE ) ) {
			turret_think_client( &level.gentities[client->ps.viewlocked_entNum] );
		}

		if ( g_debugLocDamage.integer ) {
			if ( trap_DObjExists( ent ) ) {
				G_DObjCalcPose( ent );
				trap_XModelDebugBoxes( ent );
			}
		}
	}
}
