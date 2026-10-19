/*
 * g_scr_mover_mp.c -- script_brushmodel / script_model / script_origin and the
 * script methods that move and rotate them
 */

#include "g_local.h"

#include <math.h>
#include <string.h>

/* Declarations that belong in a shared header. */

/* entityShared_t.svFlags; set by SP_script_model only */
#define SVF_DOBJ_USE_MODEL_BOUNDS   4

/* entityShared_t.contents of a script_model: it stops shots and missiles. */
#define SCRIPT_MODEL_CONTENTS   ( CONTENTS_CLIPSHOT | CONTENTS_MISSILECLIP )

/* forward: q_math.c */
float   AngleSubtract( float a1, float a2 );
float   AngleNormalize360( float angle );
float   AngleNormalize180( float angle );

/*
 * The script method table's entry type.  Scr_GetMethod chains
 * Player_GetMethod -> ScriptEnt_GetMethod -> HudElem_GetMethod ->
 * BuiltIn_GetMethod, all with this shape.
 */
typedef void ( *xmethod_t )( int entnum );

typedef struct {
	const char  *name;
	xmethod_t   func;
} scr_method_t;


/*
================
ScriptMover_Updatemove

Advances one trajectory to its next phase.  Returns qtrue when the move is
finished, which is what makes Reached_ScriptMover fire the notify.

keep: current is unused; it and the parameter order match original codegen
================
*/
static qboolean ScriptMover_Updatemove( trajectory_t *tr, vec3_t current, float speed, float linearTime,
										float decelTime, const vec3_t accelEnd, const vec3_t decelStart,
										const vec3_t target ) {
	vec3_t delta;
	float f;

	if ( tr->trType == TR_ACCELERATE ) {
		if ( linearTime > 0 ) {
			tr->trTime = level.time;
			tr->trDuration = linearTime * 1000;
			VectorCopy( accelEnd, tr->trBase );
			VectorSubtract( decelStart, accelEnd, delta );
			tr->trType = TR_LINEAR_STOP;
			f = 1000.0f / tr->trDuration;
			VectorScale( delta, f, tr->trDelta );
			return qfalse;
		}
	} else if ( tr->trType != TR_LINEAR_STOP ) {
		goto stopped;
	}

	if ( decelTime > 0 ) {
		tr->trTime = level.time;
		tr->trDuration = decelTime * 1000;
		VectorCopy( decelStart, tr->trBase );
		VectorSubtract( target, decelStart, delta );
		VectorNormalize( delta );
		tr->trType = TR_DECCELERATE;
		VectorScale( delta, speed, delta );     // keep: scaled in place, then copied, to match original codegen
		VectorCopy( delta, tr->trDelta );
		return qfalse;
	}

stopped:
	// a gravity move has no destination to snap to; freeze it where it fell
	if ( tr->trType == TR_GRAVITY ) {
		BG_EvaluateTrajectory( tr, level.time, tr->trBase );
	} else {
		VectorCopy( target, tr->trBase );
	}

	tr->trTime = level.time;
	tr->trType = TR_STATIONARY;
	return qtrue;
}

/*
================
Reached_ScriptMover
================
*/
void Reached_ScriptMover( gentity_t *ent ) {
	qboolean done;

	if ( ent->s.pos.trType != TR_STATIONARY ) {
		if ( ent->s.pos.trTime + ent->s.pos.trDuration <= level.time ) {
			done = ScriptMover_Updatemove( &ent->s.pos, ent->r.currentOrigin, ent->speed, ent->wait, ent->delay,
										   ent->pos1, ent->pos2, ent->pos3 );
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );
			trap_LinkEntity( ent );

			if ( done ) {
				Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, scr_const.movedone, 0 );
			}
		}
	}

	if ( ent->s.apos.trType != TR_STATIONARY ) {
		if ( ent->s.apos.trTime + ent->s.apos.trDuration <= level.time ) {
			done = ScriptMover_Updatemove( &ent->s.apos, ent->r.currentAngles, ent->closespeed, ent->angle,
										   ent->random, ent->movedir, ent->rotate, ent->TargetAngles );
			BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );
			trap_LinkEntity( ent );

			if ( done ) {
				ent->r.currentAngles[PITCH] = AngleNormalize180( ent->r.currentAngles[PITCH] );
				ent->r.currentAngles[YAW] = AngleNormalize360( ent->r.currentAngles[YAW] );
				ent->r.currentAngles[ROLL] = AngleNormalize180( ent->r.currentAngles[ROLL] );

				Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, scr_const.rotatedone, 0 );
			}
		}
	}
}

/*
================
ScriptMover_SetupMove

Arms the first phase of a move to `point` over `time` seconds, and records
what the later phases need in the caller's six fields: the cruise speed, the
linear-phase duration, the decel duration, where the accel phase ends, where
the decel phase starts and the destination.  Moves use speed / wait / delay
and pos1 / pos2 / pos3; rotations use closespeed / angle / random and
movedir / rotate / TargetAngles.
================
*/
static void ScriptMover_SetupMove( trajectory_t *tr, const vec3_t point, float time, float accelTime,
								   float decelTime, vec3_t current, float *speed, float *linearTime,
								   float *decelTimeOut, vec3_t accelEnd, vec3_t decelStart, vec3_t target ) {
	vec3_t delta;
	vec3_t dir;
	rfloat dist;
	float f;

	// the delta is taken against the old current position, before the
	// trajectory snap below moves it
	VectorSubtract( point, current, delta );

	if ( tr->trType != TR_STATIONARY ) {
		BG_EvaluateTrajectory( tr, level.time, current );
	}

	if ( accelTime == 0 && decelTime == 0 ) {
		tr->trTime = level.time;
		tr->trDuration = time * 1000;
		*linearTime = time;
		*decelTimeOut = 0;
		VectorCopy( point, target );
		VectorCopy( current, tr->trBase );
		f = 1000.0f / tr->trDuration;
		tr->trType = TR_LINEAR_STOP;
		VectorScale( delta, f, tr->trDelta );
		BG_EvaluateTrajectory( tr, level.time, current );
		return;
	}

	*linearTime = time - accelTime - decelTime;
	*decelTimeOut = decelTime;

	// cruise speed: the accel and decel phases each cover half their time's worth
	// keep: VectorLength written out to match original codegen
	dist = sqrt( (rfloat)delta[0] * delta[0] + (rfloat)delta[1] * delta[1] + (rfloat)delta[2] * delta[2] );
	*speed = ( dist + dist ) / ( time + time - accelTime - decelTime );

	VectorNormalize2( delta, dir );
	VectorScale( dir, *speed, dir );

	if ( accelTime != 0 ) {
		tr->trTime = level.time;
		tr->trDuration = accelTime * 1000;
		VectorCopy( current, tr->trBase );
		VectorCopy( dir, tr->trDelta );
		tr->trType = TR_ACCELERATE;
		BG_EvaluateTrajectory( tr, level.time + tr->trDuration, accelEnd );
	} else {
		VectorCopy( current, accelEnd );

		if ( *linearTime != 0 ) {
			tr->trTime = level.time;
			tr->trDuration = *linearTime * 1000;
			VectorCopy( current, tr->trBase );
			VectorScale( dir, *linearTime, delta );     // keep: delta reused to match original codegen
			tr->trType = TR_LINEAR_STOP;
			f = 1000.0f / tr->trDuration;
			VectorScale( delta, f, tr->trDelta );
		} else {
			tr->trTime = level.time;
			tr->trDuration = *decelTimeOut * 1000;
			VectorCopy( current, tr->trBase );
			VectorCopy( dir, tr->trDelta );
			tr->trType = TR_DECCELERATE;
		}
	}

	VectorMA( accelEnd, *linearTime, dir, decelStart );
	VectorCopy( point, target );

	BG_EvaluateTrajectory( tr, level.time, current );
}

/*
================
ScriptMover_SetupMoveSpeed

Same three phases, but driven by a velocity instead of a destination, so the
destination has to be integrated out of the trajectories instead.
================
*/
static void ScriptMover_SetupMoveSpeed( trajectory_t *tr, const vec3_t velocity, float time, float accelTime,
										float decelTime, vec3_t current, float *speed, float *linearTime,
										float *decelTimeOut, vec3_t accelEnd, vec3_t decelStart, vec3_t target ) {
	trajectory_t decel;

	if ( tr->trType != TR_STATIONARY ) {
		BG_EvaluateTrajectory( tr, level.time, current );
	}

	if ( accelTime == 0 && decelTime == 0 ) {
		tr->trTime = level.time;
		tr->trDuration = time * 1000;
		*linearTime = time;
		*decelTimeOut = 0;
		VectorCopy( current, tr->trBase );
		VectorCopy( velocity, tr->trDelta );
		tr->trType = TR_LINEAR_STOP;
		BG_EvaluateTrajectory( tr, level.time, current );
		BG_EvaluateTrajectory( tr, level.time + tr->trDuration, target );
		return;
	}

	*linearTime = time - accelTime - decelTime;
	*decelTimeOut = decelTime;
	// keep: VectorLength written out, with the cast, to match original codegen
	*speed = ( (float)sqrt( velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2] ) );

	if ( accelTime != 0 ) {
		tr->trTime = level.time;
		tr->trDuration = accelTime * 1000;
		VectorCopy( current, tr->trBase );
		VectorCopy( velocity, tr->trDelta );
		tr->trType = TR_ACCELERATE;
		BG_EvaluateTrajectory( tr, level.time + tr->trDuration, accelEnd );
	} else {
		VectorCopy( current, accelEnd );

		if ( *linearTime != 0 ) {
			tr->trTime = level.time;
			tr->trDuration = *linearTime * 1000;
			VectorCopy( current, tr->trBase );
			VectorCopy( velocity, tr->trDelta );
			tr->trType = TR_LINEAR_STOP;
		} else {
			tr->trTime = level.time;
			tr->trDuration = *decelTimeOut * 1000;
			VectorCopy( current, tr->trBase );
			VectorCopy( velocity, tr->trDelta );
			tr->trType = TR_DECCELERATE;
		}
	}

	VectorMA( accelEnd, *linearTime, velocity, decelStart );

	if ( *decelTimeOut != 0 ) {
		decel.trType = TR_DECCELERATE;
		decel.trTime = level.time;
		decel.trDuration = *decelTimeOut * 1000;
		VectorCopy( decelStart, decel.trBase );
		VectorCopy( velocity, decel.trDelta );
		BG_EvaluateTrajectory( &decel, level.time + decel.trDuration, target );
	} else {
		VectorCopy( decelStart, target );
	}

	BG_EvaluateTrajectory( tr, level.time, current );
}

/*
================
ScriptMover_Move
================
*/
static void ScriptMover_Move( gentity_t *ent, const vec3_t point, float time, float accelTime, float decelTime ) {
	ScriptMover_SetupMove( &ent->s.pos, point, time, accelTime, decelTime, ent->r.currentOrigin,
						   &ent->speed, &ent->wait, &ent->delay, ent->pos1, ent->pos2, ent->pos3 );
	trap_LinkEntity( ent );
}

/*
================
ScriptMover_GravityMove
================
*/
static void ScriptMover_GravityMove( gentity_t *ent, const vec3_t velocity, float time ) {
	trajectory_t *tr;

	tr = &ent->s.pos;
	tr->trTime = level.time;
	tr->trDuration = time * 1000;
	VectorCopy( ent->r.currentOrigin, tr->trBase );
	VectorCopy( velocity, tr->trDelta );
	tr->trType = TR_GRAVITY;

	BG_EvaluateTrajectory( tr, level.time, ent->r.currentOrigin );
	trap_LinkEntity( ent );
}

/*
================
ScriptMover_Rotate
================
*/
static void ScriptMover_Rotate( gentity_t *ent, const vec3_t angles, float time, float accelTime, float decelTime ) {
	ScriptMover_SetupMove( &ent->s.apos, angles, time, accelTime, decelTime, ent->r.currentAngles,
						   &ent->closespeed, &ent->angle, &ent->random, ent->movedir, ent->rotate,
						   ent->TargetAngles );
	trap_LinkEntity( ent );
}

/*
================
ScriptMover_RotateSpeed
================
*/
static void ScriptMover_RotateSpeed( gentity_t *ent, const vec3_t velocity, float time, float accelTime,
									 float decelTime ) {
	ScriptMover_SetupMoveSpeed( &ent->s.apos, velocity, time, accelTime, decelTime, ent->r.currentAngles,
								&ent->closespeed, &ent->angle, &ent->random, ent->movedir, ent->rotate,
								ent->TargetAngles );
	trap_LinkEntity( ent );
}

/*
================
InitScriptMover
================
*/
void InitScriptMover( gentity_t *ent ) {
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->reached = Reached_ScriptMover;

	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_SCRIPTMOVER;

	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;

	VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
	ent->s.apos.trType = TR_STATIONARY;

	ent->flags |= FL_SUPPORTS_LINKTO;
}

/*
================
SP_script_brushmodel
================
*/
void SP_script_brushmodel( gentity_t *ent ) {
	trap_SetBrushModel( ent );
	InitScriptMover( ent );
	ent->r.contents = CONTENTS_SOLID;
	trap_LinkEntity( ent );
}

/*
================
SP_script_model
================
*/
void SP_script_model( gentity_t *ent ) {
	G_DObjUpdate( ent );
	InitScriptMover( ent );
	ent->r.svFlags |= SVF_DOBJ_USE_MODEL_BOUNDS;
	ent->r.contents = SCRIPT_MODEL_CONTENTS;
	trap_LinkEntity( ent );
}

/*
================
SP_script_origin
================
*/
void SP_script_origin( gentity_t *ent ) {
	InitScriptMover( ent );
	ent->r.contents = 0;
	trap_LinkEntity( ent );

	// an origin with a light on it still has to reach the client
	if ( ent->s.constantLight ) {
		ent->s.eFlags |= EF_NODRAW;
	} else {
		ent->r.svFlags |= SVF_NOCLIENT;
	}
}

/*
================
GET_SCRIPT_MOVER

The entity-number check and the classname check every method below starts
with.

keep: a macro, with this block order and the separate moverEnt local, to
match original codegen
================
*/
#define GET_SCRIPT_MOVER( ent, entnum ) \
	do { \
		gentity_t   *moverEnt; \
		if ( (unsigned int)( entnum ) < MAX_GENTITIES ) { \
			moverEnt = &g_entities[( entnum )]; \
			if ( moverEnt->classname == scr_const.script_brushmodel ) { \
				( ent ) = moverEnt; \
				break; \
			} \
			if ( moverEnt->classname == scr_const.script_model ) { \
				( ent ) = moverEnt; \
				break; \
			} \
			if ( moverEnt->classname == scr_const.script_origin ) { \
				( ent ) = moverEnt; \
				break; \
			} \
			Scr_Error( va( "entity %i is not a script_brushmodel, script_model, or script_origin", ( entnum ) ) ); \
			( ent ) = moverEnt; \
			break; \
		} \
		Scr_Error( va( "%i is not a valid entity number", ( entnum ) ) ); \
		( ent ) = NULL; \
	} while ( 0 )

/*
================
ScriptEntCmdGetCommandTimes

Parameter 1 is the total time, 2 the accel time and 3 the decel time; the
last two are optional and default to zero.
================
*/
static void ScriptEntCmdGetCommandTimes( float *totalTime, float *accelTime, float *decelTime ) {
	int numParam;

	*totalTime = Scr_GetFloat( 1 );
	if ( *totalTime <= 0 ) {
		Scr_ParamError( 1, "total time must be positive" );
	}

	numParam = Scr_GetNumParam();
	if ( numParam > 2 ) {
		*accelTime = Scr_GetFloat( 2 );
		if ( *accelTime < 0 ) {
			Scr_ParamError( 2, "accel time must be nonnegative" );
		}

		if ( numParam > 3 ) {
			*decelTime = Scr_GetFloat( 3 );
			if ( *decelTime < 0 ) {
				Scr_ParamError( 3, "decel time must be nonnegative" );
			}
		} else {
			*decelTime = 0;
		}
	} else {
		*accelTime = 0;
		*decelTime = 0;
	}

	if ( *accelTime + *decelTime > *totalTime ) {
		Scr_Error( "accel time plus decel time is greater than total time" );
	}
}

/*
================
ScriptEntCmd_MoveTo

moveto( point, time [, accelTime, decelTime] ): move to point over time seconds.
================
*/
void ScriptEntCmd_MoveTo( int entnum ) {
	gentity_t   *ent;
	vec3_t point;
	float time, accelTime, decelTime;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	Scr_GetVector( 0, point );
	ScriptEntCmdGetCommandTimes( &time, &accelTime, &decelTime );

	ScriptMover_Move( ent, point, time, accelTime, decelTime );
}

/*
================
ScriptEntCmd_GravityMove

movegravity( velocity, time ): launch along a gravity arc for time seconds.
================
*/
void ScriptEntCmd_GravityMove( int entnum ) {
	gentity_t   *ent;
	vec3_t velocity;
	float time;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	Scr_GetVector( 0, velocity );
	time = Scr_GetFloat( 1 );

	ScriptMover_GravityMove( ent, velocity, time );
}

/*
================
ScriptEnt_MoveAxis

Shared by movex / movey / movez: move dist units along one world axis.
================
*/
static void ScriptEnt_MoveAxis( int entnum, int axis ) {
	gentity_t   *ent;
	vec3_t point;
	float dist;
	float time, accelTime, decelTime;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	dist = Scr_GetFloat( 0 );
	ScriptEntCmdGetCommandTimes( &time, &accelTime, &decelTime );

	VectorCopy( ent->r.currentOrigin, point );
	point[axis] += dist;

	ScriptMover_Move( ent, point, time, accelTime, decelTime );
}

/*
================
ScriptEntCmd_MoveX

movex( dist, time [, accelTime, decelTime] )
================
*/
void ScriptEntCmd_MoveX( int entnum ) {
	ScriptEnt_MoveAxis( entnum, 0 );
}

/*
================
ScriptEntCmd_MoveY

movey( dist, time [, accelTime, decelTime] )
================
*/
void ScriptEntCmd_MoveY( int entnum ) {
	ScriptEnt_MoveAxis( entnum, 1 );
}

/*
================
ScriptEntCmd_MoveZ

movez( dist, time [, accelTime, decelTime] )
================
*/
void ScriptEntCmd_MoveZ( int entnum ) {
	ScriptEnt_MoveAxis( entnum, 2 );
}

/*
================
ScriptEntCmd_RotateTo

rotateto( angles, time [, accelTime, decelTime] ): rotate to angles over time
seconds, taking the short way round on each axis.
================
*/
void ScriptEntCmd_RotateTo( int entnum ) {
	gentity_t   *ent;
	vec3_t angles;
	vec3_t dest;
	float time, accelTime, decelTime;
	int i;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	Scr_GetVector( 0, angles );
	ScriptEntCmdGetCommandTimes( &time, &accelTime, &decelTime );

	// take the short way round on each axis
	for ( i = 0; i < 3; i++ ) {
		dest[i] = AngleSubtract( angles[i], ent->r.currentAngles[i] ) + ent->r.currentAngles[i];
	}

	ScriptMover_SetupMove( &ent->s.apos, dest, time, accelTime, decelTime, ent->r.currentAngles,
						   &ent->closespeed, &ent->angle, &ent->random, ent->movedir, ent->rotate,
						   ent->TargetAngles );
	trap_LinkEntity( ent );
}

/*
================
ScriptEnt_RotateAxis

Shared by rotatepitch / rotateyaw / rotateroll: rotate by degrees about one axis.
================
*/
static void ScriptEnt_RotateAxis( int entnum, int axis ) {
	gentity_t   *ent;
	vec3_t angles;
	float degrees;
	float time, accelTime, decelTime;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	degrees = Scr_GetFloat( 0 );
	ScriptEntCmdGetCommandTimes( &time, &accelTime, &decelTime );

	VectorCopy( ent->r.currentAngles, angles );
	angles[axis] += degrees;

	ScriptMover_Rotate( ent, angles, time, accelTime, decelTime );
}

/*
================
ScriptEntCmd_RotatePitch

rotatepitch( degrees, time [, accelTime, decelTime] )
================
*/
void ScriptEntCmd_RotatePitch( int entnum ) {
	ScriptEnt_RotateAxis( entnum, PITCH );
}

/*
================
ScriptEntCmd_RotateYaw

rotateyaw( degrees, time [, accelTime, decelTime] )
================
*/
void ScriptEntCmd_RotateYaw( int entnum ) {
	ScriptEnt_RotateAxis( entnum, YAW );
}

/*
================
ScriptEntCmd_RotateRoll

rotateroll( degrees, time [, accelTime, decelTime] )
================
*/
void ScriptEntCmd_RotateRoll( int entnum ) {
	ScriptEnt_RotateAxis( entnum, ROLL );
}

/*
================
ScriptEntCmd_RotateVelocity

rotatevelocity( velocity, time [, accelTime, decelTime] ): spin at velocity
degrees per second for time seconds.
================
*/
void ScriptEntCmd_RotateVelocity( int entnum ) {
	gentity_t   *ent;
	vec3_t velocity;
	float time, accelTime, decelTime;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->flags & FL_TEAMSLAVE ) {
		return;
	}

	Scr_GetVector( 0, velocity );
	ScriptEntCmdGetCommandTimes( &time, &accelTime, &decelTime );

	ScriptMover_RotateSpeed( ent, velocity, time, accelTime, decelTime );
}

/*
================
ScriptEntCmd_Solid

solid(): make a script_brushmodel solid.
================
*/
void ScriptEntCmd_Solid( int entnum ) {
	gentity_t   *ent;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->classname == scr_const.script_origin ) {
		G_DPrintf( "cannot use the solid/notsolid commands on a script_origin entity" );
	} else if ( ent->classname == scr_const.script_model ) {
		G_DPrintf( "cannot use the solid/notsolid commands on a script_model entity" );
	} else {
		ent->r.contents = CONTENTS_SOLID;
	}
}

/*
================
ScriptEntCmd_NotSolid

notsolid(): make a script_brushmodel non-solid.
================
*/
void ScriptEntCmd_NotSolid( int entnum ) {
	gentity_t   *ent;

	GET_SCRIPT_MOVER( ent, entnum );

	if ( ent->classname == scr_const.script_origin ) {
		G_DPrintf( "cannot use the solid/notsolid commands on a script_origin entity" );
	} else if ( ent->classname == scr_const.script_model ) {
		G_DPrintf( "cannot use the solid/notsolid commands on a script_model entity" );
	} else {
		ent->r.contents = 0;
	}
}

static const scr_method_t scriptEntMethods[] = {
	{ "moveto",         ScriptEntCmd_MoveTo         },
	{ "movex",          ScriptEntCmd_MoveX          },
	{ "movey",          ScriptEntCmd_MoveY          },
	{ "movez",          ScriptEntCmd_MoveZ          },
	{ "movegravity",    ScriptEntCmd_GravityMove    },
	{ "rotateto",       ScriptEntCmd_RotateTo       },
	{ "rotatepitch",    ScriptEntCmd_RotatePitch    },
	{ "rotateyaw",      ScriptEntCmd_RotateYaw      },
	{ "rotateroll",     ScriptEntCmd_RotateRoll     },
	{ "rotatevelocity", ScriptEntCmd_RotateVelocity },
	{ "solid",          ScriptEntCmd_Solid          },
	{ "notsolid",       ScriptEntCmd_NotSolid       }
};

/*
================
ScriptEnt_GetMethod

None of these methods is developer-only.  Scr_GetMethod clears that flag
before entering the chain, so this lookup receives only the method name.
================
*/
xmethod_t ScriptEnt_GetMethod( const char **pName ) {
	unsigned int i;

	for ( i = 0; i < sizeof( scriptEntMethods ) / sizeof( scriptEntMethods[0] ); i++ ) {
		if ( !strcmp( *pName, scriptEntMethods[i].name ) ) {
			*pName = scriptEntMethods[i].name;
			return scriptEntMethods[i].func;
		}
	}

	return NULL;
}
