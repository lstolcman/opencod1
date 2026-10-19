// g_team_mp.c -- target_location reporting and the teammate identify trace

#include <stdlib.h>

#include "g_local.h"

#define MASK_AIM_TRACE          0x02000001

void target_location_linkup( gentity_t *ent ) {
	int i;
	int n;

	if ( level.locationLinked ) {
		return;
	}

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	trap_SetConfigstring( CS_LOCATIONS, "unknown" );

	for ( i = 0, ent = g_entities, n = 1 ; i < level.num_entities ; i++, ent++ ) {
		if ( ent->classname == scr_const.target_location ) {
			// lets overload some variables!
			ent->health = n; // use for location marking
			trap_SetConfigstring( CS_LOCATIONS + n, SL_ConvertToString( ent->message ) );
			n++;
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}
	}

	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white
*/
void SP_target_location( gentity_t *self ) {
	Scr_SetString( &self->classname, scr_const.target_location );

	self->think = target_location_linkup;
	self->nextthink = level.time + 200;         // Let them all spawn first

	G_SetOrigin( self, self->r.currentOrigin );
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 ) {
	if ( !ent1->client || !ent2->client ) {
		return qfalse;
	}

	if ( ent1->client->sess.sessionTeam == TEAM_FREE ) {
		return qfalse;
	}

	if ( ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam ) {
		return qtrue;
	}

	return qfalse;
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation( gentity_t *ent ) {
	gentity_t       *eloc, *best;
	float bestlen;
	rfloat len;     // keep: matches original codegen
	vec3_t origin;

	best = NULL;
	bestlen = 3 * 8192.0 * 8192.0;

	VectorCopy( ent->r.currentOrigin, origin );

	for ( eloc = level.locationHead ; eloc ; eloc = eloc->nextTrain ) {
		len = ( origin[0] - eloc->r.currentOrigin[0] ) * ( origin[0] - eloc->r.currentOrigin[0] )
			  + ( origin[1] - eloc->r.currentOrigin[1] ) * ( origin[1] - eloc->r.currentOrigin[1] )
			  + ( origin[2] - eloc->r.currentOrigin[2] ) * ( origin[2] - eloc->r.currentOrigin[2] );

		if ( len > bestlen ) {
			continue;
		}

		if ( !trap_InPVS( origin, eloc->r.currentOrigin ) ) {
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}

/*
===========
Team_GetLocationMsg

Report a location message for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg( gentity_t *ent, char *loc, int loclen ) {
	gentity_t *best;

	best = Team_GetLocation( ent );

	if ( !best ) {
		return qfalse;
	}

	// \x14 and \x15 bracket a localized string reference
	if ( best->count ) {
		if ( best->count < 0 ) {
			best->count = 0;
		}
		if ( best->count > 7 ) {
			best->count = 7;
		}
		Com_sprintf( loc, loclen, "\x15%c%c\x14%s\x15^7", '^', best->count + '0',
					 SL_ConvertToString( best->message ) );
	} else {
		Com_sprintf( loc, loclen, "\x14%s\x15", SL_ConvertToString( best->message ) );
	}

	return qtrue;
}

/*
==================
TeamplayInfoMessage

Find whoever this player is aiming at and publish him, with his health, in the
player's own playerState_t.
==================
*/
void TeamplayInfoMessage( gentity_t *ent ) {
	weaponFireInfo_t fireInfo;
	vec3_t forward;
	vec3_t start, end;
	trace_t trace;
	int entityNum;
	int health;

	if ( ent->client->sess.sessionState == STATE_PLAYING ) {
		if ( ent->client->sess.sessionTeam == TEAM_FREE ) {
			ent->client->ps.stats[STAT_IDENT_CLIENT_NUM] = -1;
			ent->client->ps.stats[STAT_IDENT_CLIENT_HEALTH] = 0;
			return;
		}

		CalcMuzzlePoints( &fireInfo, ent );

		VectorCopy( fireInfo.start, start );
		VectorMA( fireInfo.start, 8192, fireInfo.forward, end );
	} else {
		// not playing: aim straight out of the view, not out of the weapon
		AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
		CalcMuzzlePoint( ent, start );

		if ( ent->client->ps.viewHeightCurrent < 8.0f ) {
			start[2] += 8.0f - ent->client->ps.viewHeightCurrent;
		}

		VectorMA( start, 8192, forward, end );
	}

	trap_Trace( &trace, start, vec3_origin, vec3_origin, end, ent->client->ps.clientNum, MASK_AIM_TRACE );

	entityNum = trace.entityNum;

	if ( entityNum < MAX_CLIENTS && g_entities[entityNum].client
		 && ( ent->client->sess.sessionState != STATE_PLAYING || g_entities[entityNum].team == ent->team ) ) {
		health = g_entities[entityNum].health;
	} else {
		entityNum = -1;
		health = 0;
	}

	ent->client->ps.stats[STAT_IDENT_CLIENT_NUM] = entityNum;
	ent->client->ps.stats[STAT_IDENT_CLIENT_HEALTH] = health;
}

void CheckTeamStatus( void ) {
	int i;
	gentity_t *ent;

	if ( level.time - level.lastTeamLocationTime > 0 ) {
		level.lastTeamLocationTime = level.time;

		for ( i = 0 ; i < g_maxclients.integer ; i++ ) {
			ent = g_entities + i;

			if ( !ent->inuse ) {
				continue;
			}

			if ( ent->client->ps.pm_flags & PMF_FOLLOW ) {
				continue;
			}

			TeamplayInfoMessage( ent );
		}
	}
}
