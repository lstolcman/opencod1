/*
 * name:		bg_misc.c
 *
 * desc:		both games misc functions, all completely stateless
 *
*/

#include <string.h>
#include <math.h>

#include "g_public.h"
#include "bg_public.h"

void QDECL Com_Error( int level, const char *fmt, ... );
int Q_stricmp( const char *s1, const char *s2 );
float VectorNormalize2( const vec3_t v, vec3_t out );
float AngleNormalize180( float angle );

float vectopitch( const vec3_t vec );
float AngleSubtract( float a1, float a2 );

int PM_GetEffectiveStance( const playerState_t *ps );
int PM_GetViewHeightLerpTime( const playerState_t *ps, int target, int down );

// the prone check can draw itself in both games
#ifdef CGAMEDLL
#define G_DebugBox                      CG_DebugBox
#define G_DebugCircleEx                 CG_DebugCircleEx
#define g_debugProneCheck               cg_debugProneCheck
#define g_debugProneCheckDepthCheck     cg_debugProneCheckDepthCheck
#endif

void trap_AddDebugLine( const vec3_t start, const vec3_t end, const vec3_t color,
						int depthTest, int duration );
void G_DebugBox( const vec3_t mins, const vec3_t maxs, const vec3_t color,
				 int depthTest, int duration );
void G_DebugCircleEx( const vec3_t dir, const vec3_t org, float radius, const vec3_t color,
					  int depthTest, int duration );

extern vmCvar_t g_debugProneCheck;
extern vmCvar_t g_debugProneCheckDepthCheck;

extern const vec4_t colorRed;
extern const vec4_t colorGreen;
extern const vec4_t colorYellow;
extern const vec4_t colorMagenta;
extern const vec4_t colorCyan;
extern const vec4_t colorMdCyan;


char *eventnames[] =
{
	"EV_NONE",
	"EV_FOOTSTEP_RUN_DEFAULT",
	"EV_FOOTSTEP_RUN_BARK",
	"EV_FOOTSTEP_RUN_BRICK",
	"EV_FOOTSTEP_RUN_CARPET",
	"EV_FOOTSTEP_RUN_CLOTH",
	"EV_FOOTSTEP_RUN_CONCRETE",
	"EV_FOOTSTEP_RUN_DIRT",
	"EV_FOOTSTEP_RUN_FLESH",
	"EV_FOOTSTEP_RUN_FOLIAGE",
	"EV_FOOTSTEP_RUN_GLASS",
	"EV_FOOTSTEP_RUN_GRASS",
	"EV_FOOTSTEP_RUN_GRAVEL",
	"EV_FOOTSTEP_RUN_ICE",
	"EV_FOOTSTEP_RUN_METAL",
	"EV_FOOTSTEP_RUN_MUD",
	"EV_FOOTSTEP_RUN_PAPER",
	"EV_FOOTSTEP_RUN_PLASTER",
	"EV_FOOTSTEP_RUN_ROCK",
	"EV_FOOTSTEP_RUN_SAND",
	"EV_FOOTSTEP_RUN_SNOW",
	"EV_FOOTSTEP_RUN_WATER",
	"EV_FOOTSTEP_RUN_WOOD",
	"EV_FOOTSTEP_RUN_ASPHALT",
	"EV_FOOTSTEP_WALK_DEFAULT",
	"EV_FOOTSTEP_WALK_BARK",
	"EV_FOOTSTEP_WALK_BRICK",
	"EV_FOOTSTEP_WALK_CARPET",
	"EV_FOOTSTEP_WALK_CLOTH",
	"EV_FOOTSTEP_WALK_CONCRETE",
	"EV_FOOTSTEP_WALK_DIRT",
	"EV_FOOTSTEP_WALK_FLESH",
	"EV_FOOTSTEP_WALK_FOLIAGE",
	"EV_FOOTSTEP_WALK_GLASS",
	"EV_FOOTSTEP_WALK_GRASS",
	"EV_FOOTSTEP_WALK_GRAVEL",
	"EV_FOOTSTEP_WALK_ICE",
	"EV_FOOTSTEP_WALK_METAL",
	"EV_FOOTSTEP_WALK_MUD",
	"EV_FOOTSTEP_WALK_PAPER",
	"EV_FOOTSTEP_WALK_PLASTER",
	"EV_FOOTSTEP_WALK_ROCK",
	"EV_FOOTSTEP_WALK_SAND",
	"EV_FOOTSTEP_WALK_SNOW",
	"EV_FOOTSTEP_WALK_WATER",
	"EV_FOOTSTEP_WALK_WOOD",
	"EV_FOOTSTEP_WALK_ASPHALT",
	"EV_FOOTSTEP_PRONE_DEFAULT",
	"EV_FOOTSTEP_PRONE_BARK",
	"EV_FOOTSTEP_PRONE_BRICK",
	"EV_FOOTSTEP_PRONE_CARPET",
	"EV_FOOTSTEP_PRONE_CLOTH",
	"EV_FOOTSTEP_PRONE_CONCRETE",
	"EV_FOOTSTEP_PRONE_DIRT",
	"EV_FOOTSTEP_PRONE_FLESH",
	"EV_FOOTSTEP_PRONE_FOLIAGE",
	"EV_FOOTSTEP_PRONE_GLASS",
	"EV_FOOTSTEP_PRONE_GRASS",
	"EV_FOOTSTEP_PRONE_GRAVEL",
	"EV_FOOTSTEP_PRONE_ICE",
	"EV_FOOTSTEP_PRONE_METAL",
	"EV_FOOTSTEP_PRONE_MUD",
	"EV_FOOTSTEP_PRONE_PAPER",
	"EV_FOOTSTEP_PRONE_PLASTER",
	"EV_FOOTSTEP_PRONE_ROCK",
	"EV_FOOTSTEP_PRONE_SAND",
	"EV_FOOTSTEP_PRONE_SNOW",
	"EV_FOOTSTEP_PRONE_WATER",
	"EV_FOOTSTEP_PRONE_WOOD",
	"EV_FOOTSTEP_PRONE_ASPHALT",
	"EV_JUMP_DEFAULT",
	"EV_JUMP_BARK",
	"EV_JUMP_BRICK",
	"EV_JUMP_CARPET",
	"EV_JUMP_CLOTH",
	"EV_JUMP_CONCRETE",
	"EV_JUMP_DIRT",
	"EV_JUMP_FLESH",
	"EV_JUMP_FOLIAGE",
	"EV_JUMP_GLASS",
	"EV_JUMP_GRASS",
	"EV_JUMP_GRAVEL",
	"EV_JUMP_ICE",
	"EV_JUMP_METAL",
	"EV_JUMP_MUD",
	"EV_JUMP_PAPER",
	"EV_JUMP_PLASTER",
	"EV_JUMP_ROCK",
	"EV_JUMP_SAND",
	"EV_JUMP_SNOW",
	"EV_JUMP_WATER",
	"EV_JUMP_WOOD",
	"EV_JUMP_ASPHALT",
	"EV_LANDING_DEFAULT",
	"EV_LANDING_BARK",
	"EV_LANDING_BRICK",
	"EV_LANDING_CARPET",
	"EV_LANDING_CLOTH",
	"EV_LANDING_CONCRETE",
	"EV_LANDING_DIRT",
	"EV_LANDING_FLESH",
	"EV_LANDING_FOLIAGE",
	"EV_LANDING_GLASS",
	"EV_LANDING_GRASS",
	"EV_LANDING_GRAVEL",
	"EV_LANDING_ICE",
	"EV_LANDING_METAL",
	"EV_LANDING_MUD",
	"EV_LANDING_PAPER",
	"EV_LANDING_PLASTER",
	"EV_LANDING_ROCK",
	"EV_LANDING_SAND",
	"EV_LANDING_SNOW",
	"EV_LANDING_WATER",
	"EV_LANDING_WOOD",
	"EV_LANDING_ASPHALT",
	"EV_LANDING_PAIN_DEFAULT",
	"EV_LANDING_PAIN_BARK",
	"EV_LANDING_PAIN_BRICK",
	"EV_LANDING_PAIN_CARPET",
	"EV_LANDING_PAIN_CLOTH",
	"EV_LANDING_PAIN_CONCRETE",
	"EV_LANDING_PAIN_DIRT",
	"EV_LANDING_PAIN_FLESH",
	"EV_LANDING_PAIN_FOLIAGE",
	"EV_LANDING_PAIN_GLASS",
	"EV_LANDING_PAIN_GRASS",
	"EV_LANDING_PAIN_GRAVEL",
	"EV_LANDING_PAIN_ICE",
	"EV_LANDING_PAIN_METAL",
	"EV_LANDING_PAIN_MUD",
	"EV_LANDING_PAIN_PAPER",
	"EV_LANDING_PAIN_PLASTER",
	"EV_LANDING_PAIN_ROCK",
	"EV_LANDING_PAIN_SAND",
	"EV_LANDING_PAIN_SNOW",
	"EV_LANDING_PAIN_WATER",
	"EV_LANDING_PAIN_WOOD",
	"EV_LANDING_PAIN_ASPHALT",
	"EV_FOLIAGE_SOUND",
	"EV_STANCE_FORCE_STAND",
	"EV_STANCE_FORCE_CROUCH",
	"EV_STANCE_FORCE_PRONE",
	"EV_STEP_VIEW",
	"EV_WATER_TOUCH",
	"EV_WATER_LEAVE",
	"EV_ITEM_PICKUP",
	"EV_ITEM_PICKUP_QUIET",
	"EV_AMMO_PICKUP",
	"EV_NOAMMO",
	"EV_EMPTYCLIP",
	"EV_RELOAD",
	"EV_RELOAD_FROM_EMPTY",
	"EV_RELOAD_START",
	"EV_RELOAD_END",
	"EV_RAISE_WEAPON",
	"EV_PUTAWAY_WEAPON",
	"EV_WEAPON_ALT",
	"EV_PULLBACK_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRE_WEAPONB",
	"EV_FIRE_WEAPON_LASTSHOT",
	"EV_RECHAMBER_WEAPON",
	"EV_EJECT_BRASS",
	"EV_MELEE_SWIPE",
	"EV_FIRE_MELEE",
	"EV_MELEE_HIT",
	"EV_MELEE_MISS",
	"EV_FIRE_WEAPON_MG42",
	"EV_FIRE_QUADBARREL_1",
	"EV_FIRE_QUADBARREL_2",
	"EV_BULLET_TRACER",
	"EV_SOUND_ALIAS",
	"EV_BULLET_HIT_SMALL",
	"EV_BULLET_HIT_LARGE",
	"EV_BULLET_HIT_CLIENT_SMALL",
	"EV_BULLET_HIT_CLIENT_LARGE",
	"EV_GRENADE_BOUNCE",
	"EV_GRENADE_EXPLODE",
	"EV_ROCKET_EXPLODE",
	"EV_ROCKET_EXPLODE_NOMARKS",
	"EV_MOLOTOV_EXPLODE",
	"EV_MOLOTOV_EXPLODE_NOMARKS",
	"EV_CUSTOM_EXPLODE",
	"EV_CUSTOM_EXPLODE_NOMARKS",
	"EV_RAILTRAIL",
	"EV_BULLET",
	"EV_PAIN",
	"EV_CROUCH_PAIN",
	"EV_DEATH",
	"EV_DEBUG_LINE",
	"EV_PLAY_FX",
	"EV_PLAY_FX_DIR",
	"EV_PLAY_FX_ON_TAG",
	"EV_FLAMEBARREL_BOUNCE",
	"EV_EARTHQUAKE",
	"EV_DROPWEAPON",
	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",
	"EV_OBITUARY"
};

// CG_EntityEvent reads the names through this
char **pEventNamesList = eventnames;

// the events the client predicts for itself; BG_PlayerStateToEntityState
// never copies them out of the playerState
static const int iSingleClientEvents[] =
{
	EV_STANCE_FORCE_STAND, EV_STANCE_FORCE_CROUCH, EV_STANCE_FORCE_PRONE,
	EV_STEP_VIEW, EV_BULLET_HIT_CLIENT_SMALL, EV_BULLET_HIT_CLIENT_LARGE, -1
};
const int *pEventSingleClientList = iSingleClientEvents;

/*
==============
BG_FindItemForWeapon
==============
*/
gitem_t *BG_FindItemForWeapon( int weaponIndex ) {
	if ( weaponIndex < 0 || weaponIndex > bg_numWeapons ) {
		Com_Error( ERR_DROP, "\x15" "BG_FindItemForWeapon: weapon out of range %i", weaponIndex );
	}

	return &bg_itemlist[weaponIndex];
}

/*
===============
BG_FindItem

The weapon slots answer to the parsed weapon's internal name; everything past
bg_numWeapons answers to its pickup name or its classname.
===============
*/
gitem_t *BG_FindItem( const char *pickupName ) {
	int i;
	gitem_t *it;

	for ( i = 1; i < BG_NUM_ITEMS; i++ ) {
		it = &bg_itemlist[i];
		if ( i <= bg_numWeapons ) {
			if ( !Q_stricmp( pickupName, bg_weaponInfo[i]->szInternalName ) ) {
				return &bg_itemlist[i];
			}
			continue;
		}

		if ( it->pickup_name && !Q_stricmp( it->pickup_name, pickupName ) ) {
			return it;
		}
		if ( it->classname && !Q_stricmp( it->classname, pickupName ) ) {
			return it;
		}
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 36
		 || ps->origin[0] - origin[0] < -36
		 || ps->origin[1] - origin[1] > 36
		 || ps->origin[1] - origin[1] < -36
		 || ps->origin[2] - origin[2] > 18
		 || ps->origin[2] - origin[2] < -88 ) {
		return qfalse;
	}

	return qtrue;
}

/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps, qboolean onlyIfHave ) {
	gitem_t *item;
	int weaponIndex;

	if ( ent->index < 1 || ent->index >= BG_NUM_ITEMS ) {
		Com_Error( ERR_DROP, "\x15" "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->index];

	if ( ent->clientNum == ps->clientNum ) {
		return qfalse;
	}

	switch ( item->giType ) {
	case IT_WEAPON:
		weaponIndex = item->giTag;
		if ( ps->weapons[weaponIndex >> 5] & ( 1 << ( weaponIndex & 31 ) ) ) {
			// already carried: only worth taking if there is room for its ammo
			if ( BG_GetMaxPickupableAmmo( ps, weaponIndex ) <= 0 ) {
				return qfalse;
			}
		} else if ( onlyIfHave ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		return ps->stats[STAT_HEALTH] < ps->stats[STAT_MAX_HEALTH];

	case IT_AMMO:
		weaponIndex = item->giTag;
		if ( ps->weapons[weaponIndex >> 5] & ( 1 << ( weaponIndex & 31 ) ) ) {
			if ( BG_GetMaxPickupableAmmo( ps, weaponIndex ) <= 0 ) {
				return qfalse;
			}
		} else if ( !bg_weaponInfo[weaponIndex]->clipOnly || BG_GetMaxPickupableAmmo( ps, weaponIndex ) <= 0 ) {
			return qfalse;
		}
		return qtrue;

	case IT_BAD:
		Com_Error( ERR_DROP, "\x15" "BG_CanItemBeGrabbed: IT_BAD" );
		return qfalse;

	default:
		return qfalse;
	}
}

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float deltaTime;
	float phase;
	vec3_t v;
	rfloat seconds;     // keep: rfloat temporaries match original codegen
	rfloat length;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
	case TR_GRAVITY_PAUSED:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		VectorMA( tr->trBase, seconds, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float)tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		if ( seconds < 0 ) {
			seconds = 0;
		}
		VectorMA( tr->trBase, seconds, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		result[0] = tr->trBase[0] + tr->trDelta[0] * seconds;
		result[1] = tr->trBase[1] + tr->trDelta[1] * seconds;
		result[2] = tr->trBase[2] + tr->trDelta[2] * seconds - seconds * seconds * ( 0.5f * DEFAULT_GRAVITY );
		break;
	case TR_GRAVITY_LOW:
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		result[0] = tr->trBase[0] + tr->trDelta[0] * seconds;
		result[1] = tr->trBase[1] + tr->trDelta[1] * seconds;
		result[2] = tr->trBase[2] + tr->trDelta[2] * seconds - seconds * seconds * 120.00001f;   // 0.5 * ( DEFAULT_GRAVITY * 0.3 ) as a float
		break;
	case TR_GRAVITY_FLOAT:
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		result[0] = tr->trBase[0] + tr->trDelta[0] * seconds;
		result[1] = tr->trBase[1] + tr->trDelta[1] * seconds;
		result[2] = tr->trBase[2] + tr->trDelta[2] * seconds - seconds * ( 0.5f * ( DEFAULT_GRAVITY * 0.2f ) );
		break;
	case TR_ACCELERATE:     // trDelta is the ultimate speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		// phase is the acceleration constant
		length = sqrt( (rfloat)tr->trDelta[2] * tr->trDelta[2] + (rfloat)tr->trDelta[1] * tr->trDelta[1]
					   + (rfloat)tr->trDelta[0] * tr->trDelta[0] );
		phase = length / ( (rfloat)tr->trDuration * 0.001f );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time
		seconds = (rfloat)phase * deltaTime * deltaTime * 0.5f;
		VectorMA( tr->trBase, seconds, result, result );
		break;
	case TR_DECCELERATE:    // trDelta is the starting speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		// phase is the breaking constant
		length = sqrt( (rfloat)tr->trDelta[2] * tr->trDelta[2] + (rfloat)tr->trDelta[1] * tr->trDelta[1]
					   + (rfloat)tr->trDelta[0] * tr->trDelta[0] );
		phase = length / ( (rfloat)tr->trDuration * 0.001f );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time (without breaking)
		VectorMA( tr->trBase, deltaTime, tr->trDelta, v );
		// subtract breaking force
		VectorMA( v, -phase * 0.5f * deltaTime * deltaTime, result, result );
		break;
	default:
		// keep: prints trTime, not trType
		Com_Error( ERR_DROP, "\x15" "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
TR_GRAVITY_PAUSED has no case here, so it drops into the error
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	rfloat seconds;     // keep: rfloat temporary matches original codegen
	float deltaTime;
	float phase;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float)tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );    // derivative of sin = cos
		phase *= 0.5f;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * seconds;
		break;
	case TR_GRAVITY_LOW:
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= seconds * 240.00002f;   // DEFAULT_GRAVITY * 0.3 as a float
		break;
	case TR_GRAVITY_FLOAT:
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= ( DEFAULT_GRAVITY * 0.2f ) * seconds;
		break;
	case TR_ACCELERATE:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		seconds = seconds * seconds;
		VectorScale( tr->trDelta, seconds, result );
		break;
	case TR_DECCELERATE:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		seconds = (rfloat)( atTime - tr->trTime ) * 0.001f;    // milliseconds to seconds
		VectorScale( tr->trDelta, seconds, result );
		break;
	default:
		Com_Error( ERR_DROP, "\x15" "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
BG_GetMarkDir

Tip the impact direction away from the surface until it is at least a fixed
angle off it, so a decal is never laid down edge on.
================
*/
void BG_GetMarkDir( const vec3_t dir, const vec3_t normal, vec3_t out ) {
	vec3_t ndir;
	vec3_t dirNorm;
	float minDot;
	rfloat length;

	minDot = 0.3f;

	// keep: inline sqrt matches original codegen
	length = sqrt( (rfloat)normal[0] * normal[0] + (rfloat)normal[1] * normal[1] + (rfloat)normal[2] * normal[2] );
	if ( length < 1.0 ) {
		VectorSet( ndir, 0, 0, 1 );
	} else {
		VectorCopy( normal, ndir );
	}

	VectorNegate( dir, dirNorm );
	VectorNormalize( dirNorm );

	if ( normal[2] > 0.8f ) {
		minDot = 0.7f;
	}

	while ( DotProduct( dirNorm, ndir ) < minDot ) {
		VectorMA( dirNorm, 0.5f, ndir, dirNorm );
		VectorNormalize( dirNorm );
	}

	VectorCopy( dirNorm, out );
}

/*
================
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
================
*/
void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {
	if ( !newEvent ) {
		return;
	}

	// keep: byte casts match original codegen
	ps->events[ps->eventSequence & ( MAX_EVENTS - 1 )] = (byte)newEvent;
	ps->eventParms[ps->eventSequence & ( MAX_EVENTS - 1 )] = (byte)eventParm;
	ps->eventSequence++;
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int i;
	int j;
	byte event;
	float frac;

	s->eType = ( ps->pm_flags & ( PMF_FOLLOW | PMF_INWORLD ) ) ? ET_PLAYER : ET_INVISIBLE;
#ifdef CGAMEDLL
	// the client needs the number to recognise its own predicted entity
	s->number = (unsigned short)ps->clientNum;
#endif

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		// SnapVector
		s->pos.trBase[0] = (float)(int)s->pos.trBase[0];
		s->pos.trBase[1] = (float)(int)s->pos.trBase[1];
		s->pos.trBase[2] = (float)(int)s->pos.trBase[2];
	}

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		s->apos.trBase[0] = (float)(int)s->apos.trBase[0];
		s->apos.trBase[1] = (float)(int)s->apos.trBase[1];
		s->apos.trBase[2] = (float)(int)s->apos.trBase[2];
	}

	s->angles2[YAW] = ( ps->movementDir > 128 ) ? ps->movementDir - 256.0f : ps->movementDir;

	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;

	s->eFlags = ps->eFlags;
	if ( ps->eFlags & EF_TURRET_ACTIVE ) {
		s->otherEntityNum = ps->viewlocked_entNum;
	}
	if ( ps->pm_type >= PM_DEAD ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}
	if ( ps->pm_flags & PMF_ADS ) {
		s->eFlags |= EF_AIMING_DOWN_SIGHT;
	} else {
		s->eFlags &= ~EF_AIMING_DOWN_SIGHT;
	}

	s->leanf = ps->leanf;

	if ( PM_GetEffectiveStance( ps ) == 1 ) {       // prone
		if ( ps->viewHeightLerpTime ) {
			// keep: matches original codegen
			frac = (rfloat)( ps->commandTime - ps->viewHeightLerpTime )
				   / PM_GetViewHeightLerpTime( ps, ps->viewHeightLerpTarget, ps->viewHeightLerpDown );
			if ( frac < 0 ) {
				frac = 0;
			} else if ( frac > 1 ) {
				frac = 1;
			}
			if ( !ps->viewHeightLerpDown ) {
				frac = 1 - frac;
			}
		} else {
			frac = 1;
		}
		s->fTorsoHeight = frac * ps->fTorsoHeight;
		s->fTorsoPitch = frac * AngleNormalize180( ps->fTorsoPitch );
		s->fWaistPitch = frac * AngleNormalize180( ps->fWaistPitch );
	} else {
		s->fTorsoHeight = 0;
		s->fTorsoPitch = 0;
		s->fWaistPitch = 0;
	}

	// keep: wrap-safe difference, not a signed compare
	if ( (int)( (unsigned)ps->entityEventSequence - (unsigned)ps->eventSequence ) < 0 ) {
		if ( ps->eventSequence - ps->entityEventSequence > MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		s->eventParm = (byte)ps->eventParms[ps->entityEventSequence++ & ( MAX_EVENTS - 1 )];
	} else {
		s->eventParm = 0;
	}

	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		event = (byte)ps->events[i & ( MAX_EVENTS - 1 )];
		for ( j = 0; pEventSingleClientList[j] > 0; j++ ) {
			if ( pEventSingleClientList[j] == event ) {
				break;
			}
		}
		if ( pEventSingleClientList[j] < 0 ) {
			s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = event;
			s->eventParms[s->eventSequence++ & ( MAX_EVENTS - 1 )] = (byte)ps->eventParms[i & ( MAX_EVENTS - 1 )];
		}
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = (byte)ps->weapon;
	s->groundEntityNum = (unsigned short)ps->groundEntityNum;
}

/*
========================
BG_PlayerStateToEntityStateExtrapolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtrapolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int i;
	int j;
	byte event;
	int lerpTime;
	float frac;

#ifdef CGAMEDLL
	s->number = (unsigned short)ps->clientNum;
#endif
	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	VectorCopy( ps->velocity, s->pos.trDelta );
	s->pos.trTime = time;
	s->pos.trDuration = 50;         // 1000 / sv_fps

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );

	s->angles2[YAW] = ps->movementDir;      // not folded to +-128 here
	s->eFlags = ps->eFlags;

	// keep: wrap-safe differences, not signed compares
	if ( (int)( (unsigned)ps->entityEventSequence - (unsigned)ps->eventSequence ) < 0 ) {
		if ( ps->eventSequence - ps->entityEventSequence > MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		s->eventParm = (byte)ps->eventParms[ps->entityEventSequence++ & ( MAX_EVENTS - 1 )];
	} else {
		s->eventParm = 0;
	}

	if ( (int)( (unsigned)ps->oldEventSequence - (unsigned)ps->eventSequence ) > 0 ) {
		ps->oldEventSequence = ps->eventSequence;
	}
	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		event = (byte)ps->events[i & ( MAX_EVENTS - 1 )];
		for ( j = 0; pEventSingleClientList[j] > 0; j++ ) {
			if ( pEventSingleClientList[j] == event ) {
				break;
			}
		}
		if ( pEventSingleClientList[j] < 0 ) {
			s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = event;
			s->eventParms[s->eventSequence++ & ( MAX_EVENTS - 1 )] = (byte)ps->eventParms[i & ( MAX_EVENTS - 1 )];
		}
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = (byte)ps->weapon;
	s->groundEntityNum = (unsigned short)ps->groundEntityNum;

	s->eType = ( ps->pm_flags & ( PMF_FOLLOW | PMF_INWORLD ) ) ? ET_PLAYER : ET_INVISIBLE;

	if ( snap ) {
		s->pos.trBase[0] = (float)(int)s->pos.trBase[0];
		s->pos.trBase[1] = (float)(int)s->pos.trBase[1];
		s->pos.trBase[2] = (float)(int)s->pos.trBase[2];
		s->apos.trBase[0] = (float)(int)s->apos.trBase[0];
		s->apos.trBase[1] = (float)(int)s->apos.trBase[1];
		s->apos.trBase[2] = (float)(int)s->apos.trBase[2];
	}

	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;

	if ( ps->eFlags & EF_TURRET_ACTIVE ) {
		s->otherEntityNum = ps->viewlocked_entNum;
	}
	if ( ps->pm_type >= PM_DEAD ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}
	if ( ps->pm_flags & PMF_ADS ) {
		s->eFlags |= EF_AIMING_DOWN_SIGHT;
	} else {
		s->eFlags &= ~EF_AIMING_DOWN_SIGHT;
	}

	s->leanf = ps->leanf;

	if ( PM_GetEffectiveStance( ps ) == 1 ) {       // prone
		lerpTime = PM_GetViewHeightLerpTime( ps, ps->viewHeightLerpTarget, ps->viewHeightLerpDown );

		frac = (float)( ps->commandTime - ps->viewHeightLerpTime ) / (float)lerpTime;
		if ( frac < 0 ) {
			frac = 0;
		} else if ( frac > 1 ) {
			frac = 1;
		}
		if ( !ps->viewHeightLerpDown ) {
			frac = 1 - frac;
		}

		s->fTorsoHeight = frac * ps->fTorsoHeight;
		s->fTorsoPitch = frac * AngleNormalize180( ps->fTorsoPitch );
		s->fWaistPitch = frac * AngleNormalize180( ps->fWaistPitch );
	} else {
		s->fTorsoHeight = 0;
		s->fTorsoPitch = 0;
		s->fWaistPitch = 0;
	}
}

/*
========================
BG_CheckProneValid

Test a proposed prone body -- a 60 unit long, 6 unit thick capsule laid along
yaw at `origin` -- against the world through the caller's two trace callbacks,
and report the ground it would lie on.

The body is sampled at three points: the rear (the origin), the middle (24
units forward) and the front.  Each one is dropped onto the ground; the three
hits must be connected, must not fold too sharply, and the front of the body
must have room.  On success groundOffset/pitchDown/pitchUp describe the pose.

allowFallback is the caller's "the player is standing on something" flag: with
it clear the ground sampling is skipped entirely and only the forward body
trace has to pass, which is how a prone player in mid-air is judged.

The traces run through the caller's callbacks rather than pm->trace so that
cgame can call this during prediction.

keep: bodyHeight, slopeDrop, rearDepth and the vector macro spellings match
the original codegen.
========================
*/
int BG_CheckProneValid( int clientNum, const vec3_t origin, float radius, float height,
						float yaw, float *groundOffset, float *pitchDown, float *pitchUp,
						qboolean skipInitialTrace, qboolean allowFallback,
						const vec3_t groundNormal,
						void ( *traceFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
						void ( *traceDownFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
						qboolean useAltContentMask ) {
	trace_t trace;
	vec3_t mins, maxs, start, end;
	vec3_t forward, right, up;
	vec3_t frontHit, middleHit, rearHit;
	vec3_t delta;
	float frontClearance;
	float minForwardClearance;
	float groundTraceDepth;
	rfloat rearDepth;
	float traceDist;
	float slopeDrop;
	float rearPitch;
	float pitchDelta;
	float bodyHeight;
	qboolean blockedForward;
	int contentMask;

	blockedForward = qfalse;

	// the whole proposed prone volume, before anything is tested
	// keep: -radius spelled out in both mins setups
	VectorSet( mins, -radius, -radius, origin[2] );
	mins[0] += origin[0];
	mins[1] += origin[1];
	maxs[0] = radius + origin[0];
	maxs[1] = radius + origin[1];
	maxs[2] = height + origin[2];
	if ( g_debugProneCheck.integer ) {
		G_DebugBox( mins, maxs, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}

	// MASK_PLAYERSOLID without the body and the two step bits
	contentMask = useAltContentMask ? 0x820011 : 0x810011;

	if ( !skipInitialTrace ) {
		mins[0] = -radius;
		mins[1] = -radius;
		mins[2] = 0;
		maxs[0] = radius;
		maxs[1] = radius;
		maxs[2] = height;
		VectorCopy( origin, start );
		end[0] = origin[0];
		end[1] = origin[1];
		end[2] = origin[2] + 10;
		traceDownFunc( &trace, start, mins, maxs, end, clientNum, contentMask );
		if ( trace.allsolid ) {
			return qfalse;
		}
	}

	// a supplied fallback normal has to be a walkable plane
	if ( allowFallback && groundNormal && groundNormal[2] < 0.7f ) {
		return qfalse;
	}

	VectorSet( mins, -6, -6, -6 );
	VectorSet( maxs, 6, 6, 6 );
	end[0] = 0;     // end doubles as the angles
	end[1] = yaw - 180;
	end[2] = 0;
	AngleVectors( end, forward, right, up );
	bodyHeight = height - 6;

	// the body itself: 60 units long, less the 6 unit trace radius at each end
	start[0] = origin[0];
	start[1] = origin[1];
	start[2] = origin[2] + bodyHeight;
	VectorMA( start, 54, forward, end );
	traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );

	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( right, start, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( up, start, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}

	if ( trace.fraction < 1.0f ) {
		if ( !allowFallback ) {
			if ( g_debugProneCheck.integer ) {
				trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
			}
			return qfalse;
		}

		blockedForward = qtrue;
		frontClearance = trace.fraction * 54 + 6;
		if ( frontClearance < radius + 2 ) {
			if ( g_debugProneCheck.integer ) {
				trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
			}
			return qfalse;
		}

		minForwardClearance = bodyHeight * 0.7f + 24.0f;
		if ( frontClearance < minForwardClearance ) {
			// not enough room straight ahead: try again with the far end lifted
			if ( g_debugProneCheck.integer ) {
				trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
			}
			blockedForward = qfalse;
			end[2] = end[2] + 22;
			delta[0] = end[0] - start[0];
			delta[1] = end[1] - start[1];
			delta[2] = end[2] - start[2];
			traceDist = VectorNormalize2( delta, forward );
			traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );

			if ( trace.fraction < 1.0f ) {
				blockedForward = qtrue;
				frontClearance = trace.fraction * traceDist + 6;
				if ( frontClearance < minForwardClearance ) {
					if ( g_debugProneCheck.integer ) {
						trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
					}
					return qfalse;
				}
				if ( g_debugProneCheck.integer ) {
					trap_AddDebugLine( start, trace.endpos, colorYellow, g_debugProneCheckDepthCheck.integer, 1 );
				}
			} else {
				if ( g_debugProneCheck.integer ) {
					trap_AddDebugLine( start, trace.endpos, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
				}
				frontClearance = 60;
			}
		} else {
			if ( g_debugProneCheck.integer ) {
				trap_AddDebugLine( start, trace.endpos, colorYellow, g_debugProneCheckDepthCheck.integer, 1 );
			}
		}
	} else {
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( start, trace.endpos, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
		}
		frontClearance = 60;
	}

	VectorCopy( trace.endpos, frontHit );

	if ( !allowFallback ) {
		// off the ground: the body fits, and there is no ground to describe
		if ( groundOffset ) {
			*groundOffset = 0;
		}
		if ( pitchDown ) {
			*pitchDown = 0;
		}
		if ( pitchUp ) {
			*pitchUp = 0;
		}
		return qtrue;
	}

	// middle sample, 24 units along the facing direction
	VectorMA( origin, 24, forward, start );
	start[2] += bodyHeight;
	end[0] = start[0];
	end[1] = start[1];
	groundTraceDepth = radius * 2.5f + bodyHeight - 6;
	end[2] = start[2] - groundTraceDepth;
	traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );
	if ( trace.fraction == 1.0f ) {
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
		}
		return qfalse;
	}
	if ( g_debugProneCheck.integer ) {
		trap_AddDebugLine( start, trace.endpos, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
	}
	VectorCopy( trace.endpos, middleHit );
	slopeDrop = groundTraceDepth * trace.fraction + 6;

	if ( blockedForward ) {
		// the forward trace was cut short, so the front of the body is resting
		// on whatever stopped it: reject too steep a rise and re-aim the body
		// along the line that actually connects the two
		if ( frontClearance - slopeDrop < slopeDrop * -0.75f ) {
			if ( g_debugProneCheck.integer ) {
				trap_AddDebugLine( frontHit, middleHit, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
			}
			return qfalse;
		}
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( frontHit, middleHit, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
		}

		VectorSubtract( frontHit, middleHit, delta );
		VectorMA( delta, 6, forward, delta );
		delta[2] += 6;
		VectorNormalize( delta );

		// halfway between where the body wanted to end and where the connector
		// reaches; 30 is the 54 unit body less the 24 unit middle offset
		VectorMA( start, 30, delta, end );
		end[0] = ( origin[0] + forward[0] * 54 + end[0] ) * 0.5f;
		end[1] = ( origin[1] + forward[1] * 54 + end[1] ) * 0.5f;
		traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );
		if ( trace.fraction < 1.0f ) {
			if ( g_debugProneCheck.integer ) {
				trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
			}
			return qfalse;
		}
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( start, trace.endpos, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
		}
		VectorCopy( trace.endpos, frontHit );
	}

	// the ground under the front of the body
	VectorCopy( frontHit, start );
	end[0] = frontHit[0];
	end[1] = frontHit[1];
	end[2] = frontHit[2] - ( ( frontHit[2] - middleHit[2] ) * 2 + radius );
	traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );
	if ( trace.fraction == 1.0f ) {
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
		}
		return qfalse;
	}
	if ( g_debugProneCheck.integer ) {
		trap_AddDebugLine( start, trace.endpos, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
	}
	VectorCopy( trace.endpos, frontHit );

	// and the ground under the rear
	start[0] = origin[0];
	start[1] = origin[1];
	start[2] = origin[2] + bodyHeight;
	end[0] = origin[0];
	end[1] = origin[1];
	rearDepth = radius * 1.5f;
	end[2] = origin[2] - rearDepth;
	traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );
	if ( trace.fraction == 1.0f ) {
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( start, trace.endpos, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
		}
		return qfalse;
	}
	if ( g_debugProneCheck.integer ) {
		trap_AddDebugLine( start, trace.endpos, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
	}
	VectorCopy( trace.endpos, rearHit );

	// the body may not fold more than this between its two halves
	delta[0] = middleHit[0] - rearHit[0];
	delta[1] = middleHit[1] - rearHit[1];
	delta[2] = middleHit[2] - rearHit[2];
	rearPitch = vectopitch( delta );

	delta[0] = frontHit[0] - middleHit[0];
	delta[1] = frontHit[1] - middleHit[1];
	delta[2] = frontHit[2] - middleHit[2];
	pitchDelta = AngleSubtract( vectopitch( delta ), rearPitch );

	if ( pitchDelta < -50 || pitchDelta > 70 ) {
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( rearHit, middleHit, colorMagenta, g_debugProneCheckDepthCheck.integer, 1 );
		}
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( middleHit, frontHit, colorMagenta, g_debugProneCheckDepthCheck.integer, 1 );
		}
		return qfalse;
	}

	// point traces along the body, five units above the ground it found
	start[0] = rearHit[0];
	start[1] = rearHit[1];
	start[2] = rearHit[2] + 5;
	end[0] = middleHit[0];
	end[1] = middleHit[1];
	end[2] = middleHit[2] + 5;
	VectorSet( mins, 0, 0, 0 );
	VectorSet( maxs, 0, 0, 0 );
	traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );
	if ( trace.fraction < 1.0f ) {
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( start, end, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
		}
		return qfalse;
	}
	if ( g_debugProneCheck.integer ) {
		trap_AddDebugLine( start, end, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
	}

	VectorCopy( end, start );
	end[0] = frontHit[0];
	end[1] = frontHit[1];
	end[2] = frontHit[2] + 5;
	traceFunc( &trace, start, mins, maxs, end, clientNum, contentMask );
	if ( trace.fraction < 1.0f ) {
		if ( g_debugProneCheck.integer ) {
			trap_AddDebugLine( start, end, colorRed, g_debugProneCheckDepthCheck.integer, 1 );
		}
		return qfalse;
	}
	if ( g_debugProneCheck.integer ) {
		trap_AddDebugLine( start, end, colorGreen, g_debugProneCheckDepthCheck.integer, 1 );
	}

	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( right, rearHit, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( up, rearHit, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( right, middleHit, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( up, middleHit, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( right, frontHit, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		G_DebugCircleEx( up, frontHit, 6, colorMdCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		trap_AddDebugLine( rearHit, middleHit, colorCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}
	if ( g_debugProneCheck.integer ) {
		trap_AddDebugLine( middleHit, frontHit, colorCyan, g_debugProneCheckDepthCheck.integer, 1 );
	}

	if ( groundOffset ) {
		*groundOffset = rearHit[2] - origin[2] - 6;
	}
	if ( pitchDown ) {
		delta[0] = rearHit[0] - middleHit[0];
		delta[1] = rearHit[1] - middleHit[1];
		delta[2] = rearHit[2] - middleHit[2];
		*pitchDown = AngleNormalize180( vectopitch( delta ) );
	}
	if ( pitchUp ) {
		delta[0] = middleHit[0] - frontHit[0];
		delta[1] = middleHit[1] - frontHit[1];
		delta[2] = middleHit[2] - frontHit[2];
		*pitchUp = AngleNormalize180( vectopitch( delta ) );
	}

	return qtrue;
}

/*
========================
BG_CheckProne

========================
*/
int BG_CheckProne( int clientNum, const vec3_t origin, float radius, float height,
				   float yaw, float *groundOffset, float *pitchDown, float *pitchUp,
				   qboolean skipInitialTrace, qboolean allowFallback,
				   const vec3_t groundNormal,
				   void ( *traceFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
				   void ( *traceDownFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
				   qboolean useAltContentMask ) {
	return BG_CheckProneValid( clientNum, origin, radius, height, yaw, groundOffset, pitchDown,
							   pitchUp, skipInitialTrace, allowFallback, groundNormal,
							   traceFunc, traceDownFunc, useAltContentMask );
}
