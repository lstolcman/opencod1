// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include <string.h>
#include <stdlib.h>

#include "cg_local.h"

#define PITCH                       0
#define YAW                         1
#define ROLL                        2

void        CG_Printf( const char *msg, ... );
void        CG_EntityEvent( centity_t *cent, int event, qboolean predicted );

#define MAX_EVENTS                  4
#define MAX_PREDICTED_EVENTS        16

#define MAX_DAMAGE_INDICATORS       8

#define DAMAGE_TIME                 500

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/
void CG_CheckAmmo( void ) {
	int i;
	int total;
	int weapons[MAX_WEAPONS / ( sizeof( int ) * 8 )];

	// see about how many seconds of ammo we have remaining
	memcpy( weapons, cg.snap->ps.weapons, sizeof( weapons ) );

	if ( !weapons[0] && !weapons[1] ) { // (SA) we start out with no weapons, so don't make a click on startup
		return;
	}

	total = 0;

	// weapon 0 is "none"
	for ( i = 1 ; i < bg_numWeapons ; i++ )
	{
		if ( !( weapons[0] & ( 1 << i ) ) ) {
			continue;
		}

		total += cg.snap->ps.ammo[ BG_AmmoForWeapon( i ) ] * 1000;

		if ( total >= 5000 ) {
			cg.lowAmmoWarning = 0;
			return;
		}
	}

	if ( !cg.lowAmmoWarning ) {
		// play a sound on this transition
		// (noAmmoSound holds a sound alias name, not a handle)
		CG_PlayClientSoundAliasByName( (const char *)cgs.media.noAmmoSound );
	}

	if ( total == 0 ) {
		cg.lowAmmoWarning = 2;
	} else {
		cg.lowAmmoWarning = 1;
	}
}

/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback( int yawByte, int pitchByte, int damage ) {
	float kick;
	vec3_t dir;
	vec3_t angles;
	float yaw, pitch;
	int i;
	int slot;

	// show the attacking player's head and name in corner
	cg.damageFeedbackTime = cg.time;

	kick = damage * 0.2f;

	if ( kick < 5 ) {
		kick = 5;
	}
	if ( kick > 90 ) {
		kick = 90;
	}

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 ) {
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	} else {
		// positional
		pitch = pitchByte / 255.0f * 360;
		yaw = yawByte / 255.0f * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors( angles, dir, NULL, NULL );

		cg.v_dmg_roll = -kick * DotProduct( dir, cg.refdef.viewaxis[1] );
		cg.v_dmg_pitch = kick * DotProduct( dir, cg.refdef.viewaxis[0] );

		// replace the oldest direction indicator
		slot = 0;
		for ( i = 1 ; i < MAX_DAMAGE_INDICATORS ; i++ ) {
			if ( cg.damageIndicators[i][0] < cg.damageIndicators[slot][0] ) {
				slot = i;
			}
		}

		cg.damageIndicators[slot][0] = cg.snap->serverTime;
		cg.damageIndicators[slot][1] = cg_hudDamageIconTime.integer;
		// the third slot holds the icon yaw as a float
		*(float *)&cg.damageIndicators[slot][2] =
			AngleNormalize360( ( rand() / 32768.0f - 0.5f ) * 20.0f + yaw );
	}

	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.snap->serverTime;
}

/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn( void ) {
	// no error decay on player movement
	cg.mapRestart = qfalse;

	cg.predictedPlayerState = cg.snap->ps;

	cg.weaponInfo = bg_weaponInfo[ cg.predictedPlayerState.weapon ];

	// display weapons available
	cg.weaponSelectTime = cg.time;

	cg.cursorHintIcon = 0;
	cg.cursorHintTime = 0;

	cg.cameraMode = qfalse;

	VectorClear( cg.previousViewAngles );
	VectorClear( cg.swayAngles );
	VectorClear( cg.swayOffsets );

	VectorClear( cg.kick_origin );
	VectorClear( cg.kick_angles );
	cg.xyspeed = 0;

	// the ADS / weapon-position lerp block
	memset( &cg.playerEntity, 0, sizeof( cg.playerEntity ) );

	cg.damageTime = 0;
	cg.v_dmg_pitch = 0;
	cg.v_dmg_roll = 0;

	// keep: cleared as float triples, matches original codegen
	VectorClear( &cg.recoilPitch );
	VectorClear( &cg.recoilPitchSpeed );

	memset( cg.damageIndicators, 0, sizeof( cg.damageIndicators ) );
	memset( cg.cameraShakes, 0, sizeof( cg.cameraShakes ) );

	VectorClear( cg.predictedError );

	// select the weapon the server says we are using
	trap_Cvar_Set( "cg_weaponSelect", va( "%i", cg.snap->ps.weapon ) );
	trap_Cvar_Set( "cl_stance", "0" );
	trap_Cvar_Set( "cl_run", "1" );
}

/*
==============
CG_CheckPlayerstateEvents
==============
*/
void CG_CheckPlayerstateEvents( playerState_t *ps, playerState_t *ops ) {
	int i;
	int event;
	centity_t   *cent;

	cent = &cg.predictedPlayerEntity;

	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; i++ ) {
		// if we have a new predictable event
		if ( i >= ops->eventSequence
			 // or the server told us to play another event instead of a predicted event we already issued
			 // or something the server told us changed our prediction causing a different event
			 || ( i > ops->eventSequence - MAX_EVENTS && ps->events[i & ( MAX_EVENTS - 1 )] != ops->events[i & ( MAX_EVENTS - 1 )] ) ) {

			event = ps->events[ i & ( MAX_EVENTS - 1 ) ];
			cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
			CG_EntityEvent( cent, event, qtrue );

			cg.predictableEvents[ i & ( MAX_PREDICTED_EVENTS - 1 ) ] = event;

			cg.eventSequence++;
		}
	}
}

/*
==================
CG_CheckChangedPredictableEvents
==================
*/
void CG_CheckChangedPredictableEvents( playerState_t *ps ) {
	int i;
	int event;
	centity_t   *cent;

	cent = &cg.predictedPlayerEntity;
	for ( i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; i++ ) {
		//
		if ( i >= cg.eventSequence ) {
			continue;
		}
		// if this event is not further back in than the maximum predictable events we remember
		if ( i > cg.eventSequence - MAX_PREDICTED_EVENTS ) {
			// if the new playerstate event is different from a previously predicted one
			if ( ps->events[i & ( MAX_EVENTS - 1 )] != cg.predictableEvents[i & ( MAX_PREDICTED_EVENTS - 1 ) ] ) {

				event = ps->events[ i & ( MAX_EVENTS - 1 ) ];
				cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
				CG_EntityEvent( cent, event, qtrue );

				cg.predictableEvents[ i & ( MAX_PREDICTED_EVENTS - 1 ) ] = event;

				if ( cg_showmiss.integer ) {
					CG_Printf( "WARNING: changed predicted event\n" );
				}
			}
		}
	}
}

/*
===============
CG_TransitionPlayerState
===============
*/
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops ) {
	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount ) {
		CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );
	}

	// check for going low on ammo
	CG_CheckAmmo();

	// run events
	CG_CheckPlayerstateEvents( ps, ops );
}
