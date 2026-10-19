// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "bg_local.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

// float bit helpers; keep: extern, not static, matches original codegen
float PM_FloatAbs( float value ) {
	int bits;

	bits = *(int *)&value & 0x7FFFFFFF;
	return *(float *)&bits;
}

int PM_FloatIsNegative( float value ) {
	return *(int *)&value < 0;
}

int PM_FloatSign( float value ) {
	return 1 - PM_FloatIsNegative( value ) * 2;
}

float VectorNormalize2( const vec3_t v, vec3_t out );
float VectorNormalize2D( vec3_t v );
float vectoyaw( const vec3_t vec );
void vectoangles( const vec3_t value1, vec3_t angles );
float AngleNormalize360( float angle );
float AngleNormalize180( float angle );
float AngleDelta( float angle1, float angle2 );
vec_t Distance( const vec3_t p1, const vec3_t p2 );
float PitchOfVectorAlongYaw( float yaw, const vec3_t normal );
void ProjectPointOntoVector( const vec3_t vDir, vec3_t vProj, const vec3_t point );
void Com_Printf( const char *fmt, ... );
void trap_SnapVector( float *v );
extern const vec4_t colorWhite;
void trap_AddDebugLine( const vec3_t start, const vec3_t end, const vec3_t color,
						int depthTest, int duration );

// the prone view clamp can draw itself in both games
#ifdef CGAMEDLL
#define G_DebugArc          CG_DebugArc
#define g_debugProneCheck   cg_debugProneCheck
#endif

void G_DebugArc( const vec3_t origin, float radius, float startAngle, float endAngle,
				 const vec3_t color, int depthTest, int duration );

void BG_UpdateConditionValue( int client, int condition, int value, qboolean checkConversion );
void BG_AnimUpdatePlayerStateConditions( pmove_t *pmove );

int Q_ftol( float f );

extern vmCvar_t bg_viewheight_standing;
extern vmCvar_t bg_duck2prone_time;
extern vmCvar_t bg_prone2duck_time;
extern vmCvar_t bg_ladder_yawcap;
extern vmCvar_t bg_prone_yawcap;
extern vmCvar_t bg_prone_softyawedge;
extern vmCvar_t bg_foliagesnd_minspeed;
extern vmCvar_t bg_foliagesnd_maxspeed;
extern vmCvar_t bg_foliagesnd_slowinterval;
extern vmCvar_t bg_foliagesnd_fastinterval;
extern vmCvar_t bg_foliagesnd_resetinterval;
extern vmCvar_t bg_fallDamageMinHeight;
extern vmCvar_t bg_fallDamageMaxHeight;
extern vmCvar_t g_debugProneCheck;

#define SHORT2ANGLE( x )    ( ( x ) * ( 360.0f / 65536 ) )
#define ANGLE2SHORT( x )    ( (int)( ( x ) * ( 65536 / 360.0f ) ) & 65535 )

// usercmd_t button bits used here
#define BUTTON_TALK         0x2
#define WBUTTON_STUNNED     0x4
#define WBUTTON_NOSTANCEEVENT   0x2 // held-stance repeat; suppresses the blocked-stance event

// PM_GetEffectiveStance results
#define PM_EFF_STANCE_DEFAULT   0
#define PM_EFF_STANCE_PRONE     1
#define PM_EFF_STANCE_DUCKED    2

#define MIN_WALK_NORMAL     0.7f    // can't walk on very steep slopes
#define STEPSIZE            18
#define OVERCLIP            1.001f

pmove_t     *pm;
pml_t pml;

// movement parameters
const float pm_stopspeed        = 100;
const float pm_ladderScale      = 0.5f;
const float pm_ladderPushOff    = 128;
const int pm_ladderJumpTime     = 300;
const float pm_waterSwimScale   = 0.5f;
const float pm_waterWadeScale   = 0.7f;
const float pm_prone_accelerate = 19;
const float pm_ducked_accelerate = 12;
const float pm_accelerate       = 9;
const float pm_airaccelerate    = 1;
const float pm_wateraccelerate  = 4;
const float pm_flyaccelerate    = 8;
const float pm_friction         = 5.5f;
const float pm_waterfriction    = 1;
const float pm_ladderfriction   = 16;
const float pm_spectatorfriction = 5.0f;
const float pm_shellshockScale  = 0.4f;

// the neighbours PM_CorrectAllSolid jitters through
const vec3_t pm_correctSolidOffsets[26] = {
	{  0,  0,  1 }, { -1,  0,  1 }, {  0, -1,  1 }, {  1,  0,  1 },
	{  0,  1,  1 }, { -1,  0,  0 }, {  0, -1,  0 }, {  1,  0,  0 },
	{  0,  1,  0 }, {  0,  0, -1 }, { -1,  0, -1 }, {  0, -1, -1 },
	{  1,  0, -1 }, {  0,  1, -1 }, { -1, -1,  1 }, {  1, -1,  1 },
	{  1,  1,  1 }, { -1,  1,  1 }, { -1, -1,  0 }, {  1, -1,  0 },
	{  1,  1,  0 }, { -1,  1,  0 }, { -1, -1, -1 }, {  1, -1, -1 },
	{  1,  1, -1 }, { -1,  1, -1 }
};

int c_pmove = 0;

// one row of a viewheight interpolation table
typedef struct pmLerpEntry_t
{
	int percent;
	float height;
	int originAdjust;
} pmLerpEntry_t;

#define PM_LERP_TABLE_END   -1

static rfloat PM_GetViewHeightLerp( int toViewheight, int fromViewheight );

/*
===============
PM_AddEvent
===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out, float overbounce ) {
	float backoff;
	float change;
	int i;

	backoff = DotProduct( in, normal );

	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}

/*
==============
PM_GetEffectiveStance

0 standing, 1 prone, 2 crouched.  A prone->crouch lerp that has not started
coming down still counts as prone.
==============
*/
int PM_GetEffectiveStance( const playerState_t *ps ) {
	if ( ( ps->pm_flags & PMF_PRONE ) || ps->viewHeightLerpTarget == ps->proneViewHeight
		 || ( ps->viewHeightLerpTime && ps->viewHeightLerpTarget == ps->crouchViewHeight
			  && !ps->viewHeightLerpDown ) ) {
		return PM_EFF_STANCE_PRONE;
	}
	if ( ( ps->pm_flags & PMF_DUCKED )
		 || ( ps->viewHeightLerpTime && ps->viewHeightLerpTarget == ps->crouchViewHeight ) ) {
		return PM_EFF_STANCE_DUCKED;
	}
	return PM_EFF_STANCE_DEFAULT;
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t vec;
	float   *vel;
	rfloat speed, newspeed, control;    // keep: rfloat temporaries match original codegen
	rfloat drop;

	vel = pm->ps->velocity;

	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0; // ignore slope movement
	}

	// keep: inlined VectorLength, summed z, y, x
	speed = (rfloat)sqrt( (rfloat)vec[2] * vec[2] + (rfloat)vec[1] * vec[1] + (rfloat)vec[0] * vec[0] );
	if ( speed < 1 ) {
		vel[0] = 0;
		vel[1] = 0;     // allow sinking underwater
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !( pml.groundTrace.surfaceFlags & SURF_SLICK ) ) {
			// if getting knocked back, no friction
			if ( !( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				if ( pm->ps->pm_flags & PMF_TIME_LAND ) {
					control = control * 0.3f;
				} else if ( pm->ps->pm_flags & PMF_TIME_HARDLAND ) {
					control = control + control;
				}
				drop += control * pm_friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		drop += speed * pm_spectatorfriction * pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if ( newspeed < 0 ) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( const vec3_t wishdir, float wishspeed, float accel ) {
	float addspeed, accelspeed, currentspeed, control;

	currentspeed = DotProduct( pm->ps->velocity, wishdir );
	addspeed = wishspeed - currentspeed;
	if ( addspeed <= 0 ) {
		return;
	}
	control = wishspeed < pm_stopspeed ? pm_stopspeed : wishspeed;
	accelspeed = control * pml.frametime * accel;
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}
	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		rfloat frictionScale;

		frictionScale = (rfloat)1.0f / pm->ps->friction;
		accelspeed *= frictionScale;
	}
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	pm->ps->velocity[0] += accelspeed * wishdir[0];
	pm->ps->velocity[1] += accelspeed * wishdir[1];
	pm->ps->velocity[2] += accelspeed * wishdir[2];
}

/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( const usercmd_t *cmd ) {
	int max;
	rfloat scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	scale = (rfloat)pm->ps->speed * max
			/ ( FCAST( sqrt( (double)( cmd->forwardmove * cmd->forwardmove
									   + cmd->rightmove * cmd->rightmove
									   + cmd->upmove * cmd->upmove ) ) ) * 127.0f );

	if ( pm->ps->pm_flags & PMF_WALKING ) {
		scale *= pm->ps->walkSpeedScale;
	} else {
		scale *= pm->ps->runSpeedScale;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		scale *= 3;
	}
	if ( pm->ps->pm_type == PM_UFO ) {
		scale *= 6;
	}

	return scale;
}

/*
============
PM_CmdScale_Walk

The walking counterpart of PM_CmdScale: no upmove, and the per-axis, per-stance
and per-weapon scales come out of the playerState.
============
*/
static float PM_CmdScale_Walk( const usercmd_t *cmd ) {
	int fmove, smove;
	rfloat forwardTerm, moveMag;    // keep: rfloat temporaries match original codegen
	float strafeTerm;
	float base;
	rfloat scale;
	rfloat lerp;
	int effectiveStance;
	const weaponInfo_t *weaponInfo;

	fmove = cmd->forwardmove;
	// keep: a fabs per arm matches original codegen
	if ( cmd->forwardmove < 0 ) {
		forwardTerm = fabs( (rfloat)fmove * pm->ps->backSpeedScale );
	} else {
		forwardTerm = fabs( fmove );
	}
	smove = cmd->rightmove;

	strafeTerm = (float)fabs( (rfloat)smove * pm->ps->strafeSpeedScale );

	if ( forwardTerm > strafeTerm ) {
		moveMag = forwardTerm;
	} else {
		moveMag = strafeTerm;
	}
	if ( moveMag == 0 ) {
		return 0;
	}

	scale = (rfloat)pm->ps->speed * moveMag
			/ ( FCAST( sqrt( (double)( fmove * fmove + smove * smove ) ) ) * 127.0f );

	if ( pm->ps->pm_flags & PMF_WALKING ) {
		base = (float)( scale * pm->ps->walkSpeedScale );
	} else {
		base = (float)( scale * pm->ps->runSpeedScale );
		if ( pm->ps->leanf != 0 ) {
			base = base * pm->ps->leanSpeedScale;
		}
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		scale = (rfloat)base * 3;
	} else if ( pm->ps->pm_type == PM_UFO ) {
		scale = (rfloat)base * 6;
	} else {
		effectiveStance = PM_GetEffectiveStance( pm->ps );

		lerp = PM_GetViewHeightLerp( pm->ps->crouchViewHeight, pm->ps->proneViewHeight );
		if ( lerp != 0 ) {
			scale = ( ( 1.0f - lerp ) * pm->ps->crouchSpeedScale
					  + lerp * pm->ps->proneSpeedScale ) * base;
		} else {
			lerp = PM_GetViewHeightLerp( pm->ps->proneViewHeight, pm->ps->crouchViewHeight );
			if ( lerp != 0 ) {
				scale = ( ( 1.0f - lerp ) * pm->ps->proneSpeedScale
						  + lerp * pm->ps->crouchSpeedScale ) * base;
			} else if ( effectiveStance == PM_EFF_STANCE_PRONE ) {
				scale = (rfloat)base * pm->ps->proneSpeedScale;
			} else if ( effectiveStance == PM_EFF_STANCE_DUCKED ) {
				scale = (rfloat)base * pm->ps->crouchSpeedScale;
			} else {
				scale = base;
			}
		}

		if ( pm->waterlevel ) {
			scale *= 1.0f - FCAST( (rfloat)pm->waterlevel * 0.33333334f ) * ( 1.0f - pm_waterSwimScale );
		}
	}

	if ( pm->ps->weapon ) {
		weaponInfo = bg_weaponInfo[pm->ps->weapon];
		if ( weaponInfo->moveSpeedScale > 0 ) {
			scale *= weaponInfo->moveSpeedScale;
		}
	}

	if ( cmd->wbuttons & WBUTTON_STUNNED ) {
		scale *= pm_shellshockScale;
	}

	return (float)scale;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	vec3_t moved;
	vec3_t dir;
	float len;
	int movementDir;

	if ( ( pm->ps->pm_flags & PMF_PRONE ) && !( pm->ps->eFlags & EF_TURRET_ACTIVE ) ) {
		movementDir = (int)AngleNormalize180( pm->ps->proneDirection - pm->ps->viewangles[YAW] );
		if ( abs( movementDir ) > 90 ) {
			movementDir = movementDir > 0 ? 90 : -90;
		}
		pm->ps->movementDir = (signed char)movementDir;
		return;
	}

	if ( pm->ps->pm_flags & PMF_LADDER ) {
		movementDir = (int)AngleDelta( vectoyaw( pm->ps->vLadderVec ) + 180.0f, pm->ps->viewangles[YAW] );
		if ( abs( movementDir ) > 90 ) {
			movementDir = movementDir > 0 ? 90 : -90;
		}
		pm->ps->movementDir = (signed char)movementDir;
		return;
	}

	VectorSubtract( pm->ps->origin, pml.previous_origin, moved );
	// keep: inlined VectorLength and FCAST match original codegen
	if ( ( pm->cmd.forwardmove || pm->cmd.rightmove )
		 && pm->ps->groundEntityNum != ENTITYNUM_NONE
		 && ( len = FCAST( (float)sqrt( moved[0] * moved[0] + moved[1] * moved[1]
										+ moved[2] * moved[2] ) ) ) != 0
		 && pml.frametime * 5.0f < len ) {
		VectorNormalize2( moved, dir );
		vectoangles( dir, dir );

		movementDir = (int)AngleNormalize180( dir[YAW] - pm->ps->viewangles[YAW] );
		if ( pm->cmd.forwardmove < 0 ) {
			movementDir = (int)AngleNormalize180( (float)movementDir + 180.0f );
		}
		if ( abs( movementDir ) > 90 ) {
			movementDir = movementDir > 0 ? 90 : -90;
		}
		pm->ps->movementDir = (signed char)movementDir;
	} else {
		pm->ps->movementDir = 0;
	}
}

/*
=============
PM_JumpForSurface
=============
*/
static int PM_JumpForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	return SURF_TYPE( pml.groundTrace.surfaceFlags );
}

/*
=============
PM_Jump
=============
*/
static int PM_Jump( void ) {
	int surftype;

	if ( pm->ps->pm_flags & PMF_LADDER ) {
		return EV_JUMP_METAL;
	}
	surftype = PM_JumpForSurface();
	if ( !surftype ) {
		return 0;
	}
	return EV_JUMP_DEFAULT + surftype;
}

/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	vec3_t flatforward;
	vec3_t pushoff;
	rfloat dot;     // keep: rfloat matches original codegen

	if ( pm->cmd.serverTime - pm->ps->jumpTime < 500 ) {
		return qfalse;
	}

	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;
	}
	if ( pm->ps->pm_flags & PMF_TIME_HARDLAND ) {
		return qfalse;
	}
	if ( PM_GetEffectiveStance( pm->ps ) != PM_EFF_STANCE_DEFAULT ) {
		return qfalse;
	}
	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse;       // jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->velocity[2] = (float)sqrt( (rfloat)pm->ps->gravity * 78.0f );
	pm->ps->fJumpOriginZ = pm->ps->origin[2] + 39.0f;

	if ( pm->ps->pm_flags & PMF_LADDER ) {
		// push away from the ladder
		pm->ps->velocity[2] *= 0.75f;

		flatforward[0] = pml.forward[0];
		flatforward[1] = pml.forward[1];
		flatforward[2] = 0;
		VectorNormalize( flatforward );

		// keep: inlined dot products match original codegen
		if ( (rfloat)pml.forward[2] * pm->ps->vLadderVec[2] + (rfloat)pml.forward[1] * pm->ps->vLadderVec[1]
			 + (rfloat)pml.forward[0] * pm->ps->vLadderVec[0] < 0 ) {
			dot = ( (rfloat)flatforward[2] * pm->ps->vLadderVec[2] + (rfloat)flatforward[1] * pm->ps->vLadderVec[1]
					+ (rfloat)flatforward[0] * pm->ps->vLadderVec[0] ) * -2.0f;
			pushoff[0] = dot * pm->ps->vLadderVec[0] + flatforward[0];
			pushoff[1] = dot * pm->ps->vLadderVec[1] + flatforward[1];
			pushoff[2] = dot * pm->ps->vLadderVec[2] + flatforward[2];
			VectorNormalize( pushoff );
		} else {
			VectorCopy( flatforward, pushoff );
		}

		pm->ps->velocity[0] = pm_ladderPushOff * pushoff[0];
		pm->ps->velocity[1] = pm_ladderPushOff * pushoff[1];
		pm->ps->pm_flags &= ~PMF_LADDER;
	}

	PM_AddEvent( PM_Jump() );

	pm->ps->aimSpreadScale += 64.0f;
	if ( pm->ps->aimSpreadScale > 255.0f ) {
		pm->ps->aimSpreadScale = 255.0f;
	}

	if ( pm->cmd.forwardmove >= 0 ) {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMP, qfalse, qtrue );
	} else {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMPBK, qfalse, qtrue );
	}

	if ( pm->debugLevel ) {
		Com_Printf( "%i:jump base =%.1f top=%.1f vel=%.1f\n", c_pmove,
					pm->ps->origin[2], pm->ps->fJumpOriginZ, pm->ps->velocity[2] );
	}

	return qtrue;
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;
	rfloat up;      // keep: rfloat matches original codegen

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale( &pm->cmd );

	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		up = 0;
	} else {
		// keep: scaling the summed move matches original codegen
		for ( i = 0 ; i < 2 ; i++ ) {
			wishvel[i] = ( (rfloat)pm->cmd.forwardmove * pml.forward[i]
						   + (rfloat)pm->cmd.rightmove * pml.right[i] ) * scale;
		}
		up = ( (rfloat)pm->cmd.forwardmove * pml.forward[2]
			   + (rfloat)pm->cmd.rightmove * pml.right[2] ) * scale;
		up += (rfloat)pm->cmd.upmove * scale;
	}

	wishvel[2] = up - ( ( pm->cmd.wbuttons & WBUTTON_LEANRIGHT ) << 4 )
				 + ( ( pm->cmd.wbuttons & WBUTTON_LEANLEFT ) << 4 );

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );

	PM_StepSlideMove( qfalse );
}

/*
===================
PM_AirMove
===================
*/
static void PM_AirMove( void ) {
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate( wishdir, wishspeed, pm_airaccelerate );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.normal, pm->ps->velocity, OVERCLIP );
	}

	PM_StepSlideMove( qtrue );

	PM_SetMovementDir();
}

/*
===================
PM_WalkMove
===================
*/
static void PM_WalkMove( void ) {
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;
	float accelerate;
	float vel;
	vec3_t oldVel;
	int effectiveStance;

	if ( PM_CheckJump() ) {
		// jumped away
		PM_AirMove();
		pm->ps->jumpTime = pm->cmd.serverTime;
		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale_Walk( &cmd );

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( pml.forward, pml.groundTrace.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity( pml.right, pml.groundTrace.normal, pml.right, OVERCLIP );
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	effectiveStance = PM_GetEffectiveStance( pm->ps );

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK )
		 || ( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
		accelerate = pm_airaccelerate;
	} else if ( effectiveStance == PM_EFF_STANCE_PRONE ) {
		accelerate = pm_prone_accelerate;
	} else if ( effectiveStance == PM_EFF_STANCE_DUCKED ) {
		accelerate = pm_ducked_accelerate;
	} else {
		accelerate = pm_accelerate;
	}

	if ( pm->ps->pm_flags & PMF_TIME_LAND ) {
		accelerate = accelerate * 0.25f;
	}

	PM_Accelerate( wishdir, wishspeed, accelerate );

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK )
		 || ( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
		// keep: matches original codegen
		pm->ps->velocity[2] -= (rfloat)pm->ps->gravity * pml.frametime;
	}

	// keep: inlined VectorLength and FCAST match original codegen
	vel = FCAST( sqrt( (rfloat)pm->ps->velocity[0] * pm->ps->velocity[0]
					   + (rfloat)pm->ps->velocity[1] * pm->ps->velocity[1]
					   + (rfloat)pm->ps->velocity[2] * pm->ps->velocity[2] ) );

	VectorCopy( pm->ps->velocity, oldVel );

	// slide along the ground plane
	PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.normal, pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope, but only if the
	// clip did not turn the move around
	if ( DotProduct( oldVel, pm->ps->velocity ) > 0 ) {
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	// don't do anything if standing still
	if ( pm->ps->velocity[0] != 0 || pm->ps->velocity[1] != 0 ) {
		PM_StepSlideMove( qfalse );
	}

	PM_SetMovementDir();
}

/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	rfloat speed;
	float forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction
	// keep: inlined VectorLength matches original codegen
	speed = (rfloat)sqrt( (rfloat)pm->ps->velocity[0] * pm->ps->velocity[0]
						  + (rfloat)pm->ps->velocity[1] * pm->ps->velocity[1]
						  + (rfloat)pm->ps->velocity[2] * pm->ps->velocity[2] );
	forward = (float)( speed - 20.0f );
	if ( forward <= 0 ) {
		VectorClear( pm->ps->velocity );
	} else {
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, forward, pm->ps->velocity );
	}
}

/*
===============
PM_NoclipMove

===============
*/
static void PM_NoclipMove( void ) {
	rfloat speed, drop, friction, control, newspeed;   // keep: rfloat temporaries match original codegen
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;

	pm->ps->viewHeightTarget = bg_viewheight_standing.integer;
	pm->ps->standViewHeight = bg_viewheight_standing.integer;

	// friction (keep: inlined VectorLength)
	speed = (rfloat)sqrt( (rfloat)pm->ps->velocity[0] * pm->ps->velocity[0]
						  + (rfloat)pm->ps->velocity[1] * pm->ps->velocity[1]
						  + (rfloat)pm->ps->velocity[2] * pm->ps->velocity[2] );
	if ( speed < 1 ) {
		VectorCopy( vec3_origin, pm->ps->velocity );
	} else {
		friction = pm_friction * 1.5f;  // extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop = pml.frametime * control * friction;

		// scale the velocity
		newspeed = speed - drop;
		if ( newspeed < 0 ) {
			newspeed = 0;
		}
		newspeed /= speed;

		VectorScale( pm->ps->velocity, newspeed, pm->ps->velocity );
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	// keep: adding the upmove to wishvel[2] matches original codegen
	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] += pm->cmd.upmove;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
}

/*
===============
PM_UFOMove

Noclip with the lean buttons bound to up/down and an absolute vertical mode when
neither forward nor strafe is held.
===============
*/
static void PM_UFOMove( void ) {
	float speed, drop, friction, control, newspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	vec3_t up;
	vec3_t forward;
	float wishspeed;
	float scale;

	pm->ps->viewHeightTarget = bg_viewheight_standing.integer;
	pm->ps->standViewHeight = bg_viewheight_standing.integer;

	// keep: the button is used as a mask, not a boolean: lean right nets +32,
	// lean left -16
	pm->cmd.upmove = (signed char)( pm->cmd.upmove
									+ -127 * ( pm->cmd.wbuttons & WBUTTON_LEANRIGHT ) );
	pm->cmd.upmove = (signed char)( pm->cmd.upmove
									+ 127 * ( pm->cmd.wbuttons & WBUTTON_LEANLEFT ) );

	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		// straight up and down: the velocity is the command
		if ( (float)pm->cmd.upmove == 0 ) {
			speed = 0;
		} else {
			pm->ps->velocity[1] = 0;
			pm->ps->velocity[0] = 0;
			pm->ps->velocity[2] = (float)pm->cmd.upmove;
			speed = 127.0f;
		}
	} else {
		// keep: FCAST matches original codegen
		speed = FCAST( (float)sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
							   + pm->ps->velocity[1] * pm->ps->velocity[1]
							   + pm->ps->velocity[2] * pm->ps->velocity[2] ) );
	}

	if ( speed < 1 ) {
		VectorCopy( vec3_origin, pm->ps->velocity );
	} else {
		friction = pm_friction * 1.5f;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop = pml.frametime * control * friction;

		newspeed = speed - drop;
		if ( newspeed < 0 ) {
			newspeed = 0;
		}
		newspeed /= speed;

		VectorScale( pm->ps->velocity, newspeed, pm->ps->velocity );
	}

	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	// forward in UFO mode is world up crossed with the strafe axis, so the view
	// pitch does not drive the move
	VectorSet( up, 0, 0, 1 );
	CrossProduct( up, pml.right, forward );

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.right[i] * smove + forward[i] * fmove;
	}
	// keep: the doubled upmove is summed first
	wishvel[2] = (float)pm->cmd.upmove + (float)pm->cmd.upmove + wishvel[2];

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	VectorMA( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
}

/*
=================
PM_FootstepForSurface

Returns an event number appropriate for the groundtrace surface
=================
*/
static int PM_FootstepForSurface( int flags ) {
	int surftype;

	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	surftype = SURF_TYPE( pml.groundTrace.surfaceFlags );
	if ( !surftype ) {
		return 0;
	}
	if ( flags & PMF_PRONE ) {
		return EV_FOOTSTEP_PRONE_DEFAULT + surftype;
	}
	if ( flags & PMF_WALKING ) {
		return EV_FOOTSTEP_WALK_DEFAULT + surftype;
	}
	return EV_FOOTSTEP_RUN_DEFAULT + surftype;
}

/*
=================
PM_LightLandingForSurface
=================
*/
static int PM_LightLandingForSurface( void ) {
	int surftype;

	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	surftype = SURF_TYPE( pml.groundTrace.surfaceFlags );
	if ( !surftype ) {
		return 0;
	}
	return EV_FOOTSTEP_WALK_DEFAULT + surftype;
}

/*
=================
PM_MediumLandingForSurface
=================
*/
static int PM_MediumLandingForSurface( void ) {
	int surftype;

	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	surftype = SURF_TYPE( pml.groundTrace.surfaceFlags );
	if ( !surftype ) {
		return 0;
	}
	return EV_FOOTSTEP_RUN_DEFAULT + surftype;
}

/*
=================
PM_HardLandingForSurface
=================
*/
static int PM_HardLandingForSurface( void ) {
	int surftype;

	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		surftype = 0;
	} else {
		surftype = SURF_TYPE( pml.groundTrace.surfaceFlags );
	}
	return EV_LANDING_DEFAULT + surftype;
}

/*
=================
PM_DamageLandingForSurface
=================
*/
static int PM_DamageLandingForSurface( void ) {
	int surftype;

	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		surftype = 0;
	} else {
		surftype = SURF_TYPE( pml.groundTrace.surfaceFlags );
	}
	return EV_LANDING_PAIN_DEFAULT + surftype;
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float gravity;
	float height;
	rfloat vel, acc;	// keep: rfloat temporaries match original codegen
	rfloat t;
	rfloat a, b, c, den;
	rfloat delta;
	int damage;
	int viewKick;
	int stunTime;
	rfloat stunFrac;
	float speedMult;

	if ( pm->waterlevel == 3 ) {
		return;
	}

	if ( !pm->ps->legsTimer && pml.previous_velocity[2] < -220.0f ) {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_LAND, qfalse, qtrue );
	}

	// calculate the exact velocity on landing
	vel = pml.previous_velocity[2];
	gravity = (float)pm->ps->gravity;
	acc = -(rfloat)pm->ps->gravity;	// keep: negate the int, not the float

	a = acc / 2;
	b = vel;
	c = (rfloat)pml.previous_origin[2] - pm->ps->origin[2];

	den = b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = ( -b - sqrt( den ) ) / ( 2 * a );

	delta = ( vel + t * acc ) * -1;
	height = delta * delta / ( 2 * gravity );

	if ( pm->debugLevel ) {
		Com_Printf( "landing vel: %.1f fall height: %.1f\n", delta, height );
	}

	if ( bg_fallDamageMaxHeight.value <= bg_fallDamageMinHeight.value
		 || bg_fallDamageMinHeight.value < 0 ) {
#ifndef CGAMEDLL
		// the client build skips the warning
		Com_Printf( "bg_fallDamageMaxHeight and bg_fallDamageMinHeight have bad values\n" );
#endif
		damage = 0;
	} else if ( height <= bg_fallDamageMinHeight.value
				|| ( pml.groundTrace.surfaceFlags & SURF_NODAMAGE )
				|| pm->ps->pm_type >= PM_DEAD ) {
		damage = 0;
	} else if ( height >= bg_fallDamageMaxHeight.value ) {
		damage = 100;
	} else {
		damage = (int)( ( height - bg_fallDamageMinHeight.value )
						/ ( bg_fallDamageMaxHeight.value - bg_fallDamageMinHeight.value )
						* 100.0f );
		if ( damage < 0 ) {
			damage = 0;
		} else if ( damage > 100 ) {
			damage = 100;
		}
	}

	if ( pm->waterlevel == 2 ) {
		damage = (int)( (float)damage * 0.5f );
	}

	if ( height <= 12.0f ) {
		viewKick = 0;
	} else {
		viewKick = (int)( ( ( height - 12.0f ) * 0.038461540f + 1.0f ) * 4.0f );
		if ( viewKick > 24 ) {
			viewKick = 24;
		}
	}

	if ( pm->ps->fJumpOriginZ < -0.001f || pm->ps->fJumpOriginZ > 0.001f ) {
		pm->ps->pm_time = 200;
		pm->ps->pm_flags |= PMF_TIME_HARDLAND;
	}

	if ( damage ) {
		if ( pm->debugLevel ) {
			Com_Printf( "falling damage: %i\n", damage );
		}
		if ( damage >= 100 || ( pml.groundTrace.surfaceFlags & SURF_SLICK ) ) {
			VectorScale( pm->ps->velocity, 0.67000002f, pm->ps->velocity );
		} else {
			stunTime = 35 * damage + 500;
			// keep: a separate clamp matches original codegen
			if ( stunTime > 2000 ) {
				stunTime = 2000;
			}
			if ( stunTime <= 500 ) {
				speedMult = 0.5f;
			} else if ( stunTime >= 1500 ) {
				speedMult = 0.2f;
			} else {
				stunFrac = ( (rfloat)stunTime - 500.0f ) * 0.001f;
				speedMult = 0.5f - stunFrac * 0.30000001f;
			}
			if ( pm->debugLevel > 1 ) {
				Com_Printf( "landing stun time: %i speed mult: %.2f\n", stunTime, speedMult );
			}
			pm->ps->pm_time = stunTime;
			pm->ps->pm_flags |= PMF_TIME_LAND;
			VectorScale( pm->ps->velocity, speedMult, pm->ps->velocity );
		}
		BG_AddPredictableEventToPlayerstate( PM_DamageLandingForSurface(), damage, pm->ps );
		return;
	}

	if ( height > 4.0f ) {
		if ( height < 8.0f ) {
			PM_AddEvent( PM_LightLandingForSurface() );
		} else if ( height < 12.0f ) {
			PM_AddEvent( PM_MediumLandingForSurface() );
		} else {
			VectorScale( pm->ps->velocity, 0.67000002f, pm->ps->velocity );
			BG_AddPredictableEventToPlayerstate( PM_HardLandingForSurface(), viewKick, pm->ps );
		}
	}
}

/*
=============
PM_CorrectAllSolid
=============
*/
static qboolean PM_CorrectAllSolid( trace_t *trace ) {
	unsigned int i;
	vec3_t point;

	// jitter around
	for ( i = 0 ; i < sizeof( pm_correctSolidOffsets ) / sizeof( pm_correctSolidOffsets[0] ) ; i++ ) {
		point[0] = pm->ps->origin[0] + pm_correctSolidOffsets[i][0];
		point[1] = pm->ps->origin[1] + pm_correctSolidOffsets[i][1];
		point[2] = pm->ps->origin[2] + pm_correctSolidOffsets[i][2];

		pm->trace( trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
		if ( !trace->startsolid ) {
			VectorCopy( point, pm->ps->origin );

			point[0] = pm->ps->origin[0];
			point[1] = pm->ps->origin[1];
			point[2] = pm->ps->origin[2] - 1.0f;

			pm->trace( trace, pm->ps->origin, pm->mins, pm->maxs, point,
					   pm->ps->clientNum, pm->tracemask );
			pml.groundTrace = *trace;

			VectorCopy( trace->endpos, pm->ps->origin );
			return qtrue;
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->fJumpOriginZ = 0;

	return qfalse;
}

/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t trace;
	vec3_t point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf( "%i:lift\n", c_pmove );
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point,
				   pm->ps->clientNum, pm->tracemask );
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMP, qfalse, qtrue );
			} else {
				BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMPBK, qfalse, qtrue );
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}

/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t start;
	vec3_t point;
	trace_t trace;

	start[0] = pm->ps->origin[0];
	start[1] = pm->ps->origin[1];
	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	if ( pm->ps->eFlags & EF_TURRET_ACTIVE ) {
		start[2] = pm->ps->origin[2];
		point[2] = pm->ps->origin[2] - 1.0f;
	} else {
		start[2] = pm->ps->origin[2] + 0.25f;
		point[2] = pm->ps->origin[2] - 0.25f;
	}

	pm->trace( &trace, start, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid( &trace ) ) {
			return;
		}
	}

	if ( trace.startsolid ) {
		start[2] = pm->ps->origin[2] - 0.001f;
		pm->trace( &trace, start, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
		if ( trace.startsolid ) {
			pm->ps->groundEntityNum = ENTITYNUM_NONE;
			pml.groundPlane = qfalse;
			pml.walking = qfalse;
			return;
		}
		pml.groundTrace = trace;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	pm->ps->fJumpOriginZ = 0;

	// check if getting thrown off the ground
	if ( !( pm->ps->pm_flags & PMF_LADDER ) && pm->ps->velocity[2] > 0
		 && DotProduct( pm->ps->velocity, trace.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:kickoff\n", c_pmove );
		}
		if ( pm->cmd.forwardmove >= 0 ) {
			BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMP, qfalse, qfalse );
		} else {
			BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMPBK, qfalse, qfalse );
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:steep\n", c_pmove );
		}
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.walking = qfalse;
		pml.groundPlane = qtrue;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf( "%i:Land\n", c_pmove );
		}
		PM_CrashLand();
	}

	pm->ps->groundEntityNum = trace.entityNum;

	PM_AddTouchEnt( trace.entityNum );
}

/*
=============
PM_SetWaterLevel
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t point;
	int cont;
	int sample1;
	int sample2;

	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + pm->ps->mins[2] + 1;
	cont = pm->pointcontents( point, pm->ps->clientNum, CONTENTS_WATER );

	if ( cont ) {
		sample2 = (int)( pm->ps->viewHeightCurrent - pm->ps->mins[2] );
		sample1 = sample2 / 2;

		pm->watertype = (byte)cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + pm->ps->mins[2] + sample1;
		cont = pm->pointcontents( point, pm->ps->clientNum, CONTENTS_WATER );
		if ( cont ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + pm->ps->mins[2] + sample2;
			cont = pm->pointcontents( point, pm->ps->clientNum, CONTENTS_WATER );
			if ( cont ) {
				pm->waterlevel = 3;
			}
		}
	}
}

/*
=============
PM_GetViewHeightLerpTime

How long the lerp to viewheight takes, in msec.
=============
*/
int PM_GetViewHeightLerpTime( const playerState_t *ps, int viewheight, int viewHeightLerpDown ) {
	if ( viewheight == ps->proneViewHeight ) {
		if ( !( ps->pm_flags & PMF_PRONE_DIVE ) ) {
			return bg_duck2prone_time.integer;
		}
	} else if ( viewheight == ps->crouchViewHeight ) {
		if ( viewHeightLerpDown ) {
			if ( ps->pm_flags & PMF_PRONE_DIVE ) {
				return 100;
			}
			return 150;
		}
		return bg_prone2duck_time.integer;
	}
	return 200;
}

/*
=============
PM_ViewHeightTableLerp
=============
*/
static float PM_ViewHeightTableLerp( int percent, float *originAdjust, const pmLerpEntry_t *table ) {
	int i;
	float frac;
	const pmLerpEntry_t *cur;
	const pmLerpEntry_t *prev;

	if ( percent ) {
		// no end test before the first entry: every table has two
		i = 1;
		cur = &table[1];
		do {
			if ( cur->percent == percent ) {
				*originAdjust = (float)cur->originAdjust;
				return cur->height;
			}
			if ( cur->percent > percent ) {
				prev = &table[i - 1];
				frac = (float)( percent - prev->percent ) / (float)( cur->percent - prev->percent );
				*originAdjust = ( cur->originAdjust - prev->originAdjust ) * frac + prev->originAdjust;
				return frac * ( cur->height - prev->height ) + prev->height;
			}
			cur = &table[++i];
		} while ( cur->percent != PM_LERP_TABLE_END );
	}

	*originAdjust = (float)table[0].originAdjust;
	return table[0].height;
}

/*
=============
PM_GetViewHeightLerp

0..1 through the viewheight lerp that is currently running from fromViewheight to
toViewheight, or 0 if that is not the lerp in flight.  -1 for either end means
"whatever is running".
=============
*/
static rfloat PM_GetViewHeightLerp( int toViewheight, int fromViewheight ) {
	playerState_t *ps;
	int duration;
	rfloat frac;

	ps = pm->ps;

	if ( !ps->viewHeightLerpTime ) {
		return 0;
	}

	if ( toViewheight != -1 && fromViewheight != -1 ) {
		if ( fromViewheight != ps->viewHeightLerpTarget
			 || ( fromViewheight == ps->crouchViewHeight
				  && !( toViewheight == ps->proneViewHeight && !ps->viewHeightLerpDown )
				  && !( toViewheight == ps->standViewHeight && ps->viewHeightLerpDown ) ) ) {
			return 0;
		}
	}

	duration = PM_GetViewHeightLerpTime( ps, ps->viewHeightLerpTarget, ps->viewHeightLerpDown );

	frac = (rfloat)( pm->cmd.serverTime - ps->viewHeightLerpTime ) / duration;
	if ( frac < 0 ) {
		frac = 0;
	} else if ( frac > 1 ) {
		frac = 1;
	}
	return frac;
}

// viewheight lerp tables
static pmLerpEntry_t pmViewHeightLerpCrouchedRising[9] = {
	{ 0, 60.0f, 0 },  { 1, 59.5f, 0 },  { 4, 58.5f, 0 },
	{ 30, 56.0f, 0 }, { 80, 44.0f, 0 }, { 90, 41.5f, 0 },
	{ 95, 40.5f, 0 }, { 100, 40.0f, 0 },
	{ PM_LERP_TABLE_END, 0.0f, 0 }
};

static pmLerpEntry_t pmViewHeightLerpStanding[9] = {
	{ 0, 40.0f, 0 },  { 5, 40.5f, 0 },  { 10, 41.5f, 0 },
	{ 20, 44.0f, 0 }, { 70, 56.0f, 0 }, { 96, 58.5f, 0 },
	{ 99, 59.5f, 0 }, { 100, 60.0f, 0 },
	{ PM_LERP_TABLE_END, 0.0f, 0 }
};

static pmLerpEntry_t pmViewHeightLerpProne[11] = {
	{ 0, 40.0f, 0 },  { 11, 38.0f, 0 }, { 22, 33.0f, 0 },
	{ 34, 25.0f, 0 }, { 45, 16.0f, 0 }, { 50, 15.0f, 0 },
	{ 55, 16.0f, 0 }, { 70, 18.0f, 0 }, { 90, 17.0f, 0 },
	{ 100, 11.0f, 0 },
	{ PM_LERP_TABLE_END, 0.0f, 0 }
};

static pmLerpEntry_t pmViewHeightLerpProneDive[3] = {
	{ 0, 40.0f, 0 }, { 100, 11.0f, 0 },
	{ PM_LERP_TABLE_END, 0.0f, 0 }
};

static pmLerpEntry_t pmViewHeightLerpCrouchedFalling[8] = {
	{ 0, 11.0f, 0 },  { 5, 10.0f, 0 },  { 30, 21.0f, 0 },
	{ 50, 25.0f, 0 }, { 67, 31.0f, 0 }, { 83, 34.0f, 0 },
	{ 100, 40.0f, 0 },
	{ PM_LERP_TABLE_END, 0.0f, 0 }
};

/*
=============
PM_ViewHeightAdjust

Drive viewHeightCurrent toward viewHeightTarget, through the stance tables when
the target is one of the three stance heights and linearly otherwise.
=============
*/
static void PM_ViewHeightAdjust( void ) {
	int target;
	int percent;
	int duration;
	int lerpTarget;
	float originAdjust;
	float delta;
	float speed;
	vec3_t saveVelocity;
	vec3_t dir;

	target = pm->ps->viewHeightTarget;

	if ( !target || pm->ps->viewHeightCurrent == 0 ) {
		if ( pm->ps->pm_type == PM_SPECTATOR ) {
			pm->ps->viewHeightCurrent = 0;
		} else {
			pm->ps->viewHeightCurrent = (float)target;
		}
		return;
	}

	if ( (float)target == pm->ps->viewHeightCurrent && !pm->ps->viewHeightLerpTime ) {
		return;
	}

	percent = 0;

	if ( target != pm->ps->proneViewHeight && target != pm->ps->crouchViewHeight
		 && target != pm->ps->standViewHeight ) {
		// not a stance height: move at a flat 180 units/sec
		pm->ps->viewHeightLerpTime = 0;
		if ( (float)pm->ps->viewHeightTarget > pm->ps->viewHeightCurrent ) {
			pm->ps->viewHeightCurrent += pml.frametime * 180.0f;
			if ( (float)pm->ps->viewHeightTarget <= pm->ps->viewHeightCurrent ) {
				pm->ps->viewHeightCurrent = (float)pm->ps->viewHeightTarget;
			}
		} else {
			pm->ps->viewHeightCurrent -= pml.frametime * 180.0f;
			if ( (float)pm->ps->viewHeightTarget >= pm->ps->viewHeightCurrent ) {
				pm->ps->viewHeightCurrent = (float)pm->ps->viewHeightTarget;
			}
		}
		return;
	}

	if ( pm->ps->viewHeightLerpTime ) {
		lerpTarget = pm->ps->viewHeightLerpTarget;
		duration = PM_GetViewHeightLerpTime( pm->ps, lerpTarget, pm->ps->viewHeightLerpDown );
		percent = 100 * ( pm->cmd.serverTime - pm->ps->viewHeightLerpTime ) / duration;
		if ( percent < 0 ) {
			percent = 0;
		} else if ( percent > 100 ) {
			percent = 100;
		}

		if ( percent == 100 ) {
			pm->ps->viewHeightCurrent = (float)lerpTarget;
			pm->ps->viewHeightLerpTime = 0;
			pm->ps->viewHeightLerpPosAdj = 0;
		} else {
			if ( lerpTarget == pm->ps->proneViewHeight ) {
				if ( pm->ps->pm_flags & PMF_PRONE_DIVE ) {
					pm->ps->viewHeightCurrent
						= PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpProneDive );
				} else {
					pm->ps->viewHeightCurrent = PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpProne );
				}
			} else if ( lerpTarget == pm->ps->crouchViewHeight ) {
				if ( pm->ps->viewHeightLerpDown ) {
					pm->ps->viewHeightCurrent
						= PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpCrouchedRising );
				} else {
					pm->ps->viewHeightCurrent
						= PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpCrouchedFalling );
				}
			} else {
				pm->ps->viewHeightCurrent = PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpStanding );
			}

			if ( fabs( pm->ps->viewHeightLerpPosAdj - originAdjust ) > 0.05f ) {
				VectorCopy( pm->ps->velocity, saveVelocity );
				delta = originAdjust - pm->ps->viewHeightLerpPosAdj;
				if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
					delta = delta * 0.5f;
				}
				speed = delta / pml.frametime;
				dir[0] = pml.forward[0];
				dir[1] = pml.forward[1];
				dir[2] = 0;
				VectorNormalize( dir );
				pm->ps->velocity[0] = dir[0] * speed;
				pm->ps->velocity[1] = dir[1] * speed;
				pm->ps->velocity[2] = dir[2] * speed;
				PM_StepSlideMove( qtrue );
				VectorCopy( saveVelocity, pm->ps->velocity );
				pm->ps->viewHeightLerpPosAdj = originAdjust;
			}
		}
	}

	if ( pm->ps->viewHeightLerpTime ) {
		// the target moved the other way mid-lerp: turn the lerp around and pick up
		// at the mirrored percentage
		if ( pm->ps->viewHeightTarget != pm->ps->viewHeightLerpTarget
			 && ( ( pm->ps->viewHeightTarget < pm->ps->viewHeightLerpTarget
					&& !pm->ps->viewHeightLerpDown )
				  || ( pm->ps->viewHeightTarget > pm->ps->viewHeightLerpTarget
					   && pm->ps->viewHeightLerpDown ) ) ) {
			percent = 100 - percent;
			pm->ps->viewHeightLerpDown = pm->ps->viewHeightLerpDown ^ 1;

			if ( pm->ps->viewHeightLerpDown ) {
				if ( pm->ps->viewHeightLerpTarget == pm->ps->standViewHeight ) {
					pm->ps->viewHeightLerpTarget = pm->ps->crouchViewHeight;
				} else if ( pm->ps->viewHeightLerpTarget == pm->ps->crouchViewHeight ) {
					pm->ps->viewHeightLerpTarget = pm->ps->proneViewHeight;
				}
			} else {
				if ( pm->ps->viewHeightLerpTarget == pm->ps->proneViewHeight ) {
					pm->ps->viewHeightLerpTarget = pm->ps->crouchViewHeight;
				} else if ( pm->ps->viewHeightLerpTarget == pm->ps->crouchViewHeight ) {
					pm->ps->viewHeightLerpTarget = pm->ps->standViewHeight;
				}
			}

			if ( percent == 100 ) {
				pm->ps->viewHeightCurrent = (float)pm->ps->viewHeightLerpTarget;
				pm->ps->viewHeightLerpTime = 0;
				pm->ps->viewHeightLerpPosAdj = 0;
				return;
			}

			duration = PM_GetViewHeightLerpTime( pm->ps, pm->ps->viewHeightLerpTarget,
												 pm->ps->viewHeightLerpDown );
			pm->ps->viewHeightLerpTime = pm->cmd.serverTime
										 - (int)( (float)duration * (float)percent * 0.0099999998f );

			if ( pm->ps->viewHeightLerpTarget == pm->ps->proneViewHeight ) {
				PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpProne );
			} else if ( pm->ps->viewHeightLerpTarget == pm->ps->crouchViewHeight ) {
				if ( pm->ps->viewHeightLerpDown ) {
					PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpCrouchedRising );
				} else {
					PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpCrouchedFalling );
				}
			} else {
				PM_ViewHeightTableLerp( percent, &originAdjust, pmViewHeightLerpStanding );
			}
			pm->ps->viewHeightLerpPosAdj = originAdjust;
		}
	} else {
		// start a new lerp
		if ( (float)pm->ps->viewHeightTarget == pm->ps->viewHeightCurrent ) {
			return;
		}
		pm->ps->viewHeightLerpTime = pm->cmd.serverTime;
		target = pm->ps->viewHeightTarget;
		if ( target == pm->ps->proneViewHeight ) {
			pm->ps->viewHeightLerpDown = 1;
			if ( (float)pm->ps->crouchViewHeight < pm->ps->viewHeightCurrent ) {
				pm->ps->viewHeightLerpTarget = pm->ps->crouchViewHeight;
			} else {
				pm->ps->viewHeightLerpTarget = pm->ps->proneViewHeight;
			}
		} else if ( target == pm->ps->crouchViewHeight ) {
			if ( (float)target < pm->ps->viewHeightCurrent ) {
				pm->ps->viewHeightLerpDown = 1;
			} else {
				pm->ps->viewHeightLerpDown = 0;
			}
			pm->ps->viewHeightLerpTarget = pm->ps->crouchViewHeight;
		} else if ( target == pm->ps->standViewHeight ) {
			pm->ps->viewHeightLerpDown = 0;
			if ( (float)pm->ps->crouchViewHeight > pm->ps->viewHeightCurrent ) {
				pm->ps->viewHeightLerpTarget = pm->ps->crouchViewHeight;
			} else {
				pm->ps->viewHeightLerpTarget = pm->ps->standViewHeight;
			}
		}
	}
}

// keep: improve-float-consistency (/Op) code, matches original codegen
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif


/*
==============
PM_CheckDuck

Sets mins, maxs and viewHeightTarget
==============
*/
static void PM_CheckDuck( void ) {
	trace_t trace;
	vec3_t start;
	vec3_t end;
	int wasProne;
	int wasStanding;
	float jumpHeight;
	float pitch;

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		pm->mins[0] = -8;
		pm->mins[1] = -8;
		pm->mins[2] = -8;
		pm->maxs[0] = 8;
		pm->maxs[1] = 8;
		pm->maxs[2] = 16;
		pm->ps->pm_flags &= ~( PMF_PRONE | PMF_DUCKED );
		if ( pm->cmd.wbuttons & WBUTTON_PRONE ) {
			pm->cmd.wbuttons &= ~WBUTTON_PRONE;
			PM_AddEvent( EV_STANCE_FORCE_STAND );
		}
		pm->trace = pm->trace3;
		pm->ps->eFlags |= EF_CAPSULE;
		pm->ps->viewHeightTarget = 0;
		pm->ps->viewHeightCurrent = 0;
		return;
	}

	wasProne = pm->ps->pm_flags & PMF_PRONE;
	wasStanding = ( pm->ps->pm_flags & ( PMF_PRONE | PMF_DUCKED ) ) == 0;
	pm->mins[0] = pm->ps->mins[0];
	pm->mins[1] = pm->ps->mins[1];
	pm->maxs[0] = pm->ps->maxs[0];
	pm->maxs[1] = pm->ps->maxs[1];
	pm->mins[2] = pm->ps->mins[2];

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->maxs[2] = pm->ps->maxs[2];
		pm->ps->viewHeightTarget = pm->ps->deadViewHeight;
		if ( pm->ps->pm_flags & PMF_PRONE ) {
			pm->trace = pm->trace2;
		} else {
			pm->trace = pm->trace3;
		}
		pm->ps->eFlags |= EF_CAPSULE;
		PM_ViewHeightAdjust();
		return;
	}

	if ( pm->ps->eFlags & EF_TURRET_ACTIVE ) {
		// a mounted weapon dictates the stance
		if ( ( pm->ps->eFlags & EF_TURRET_ACTIVE_PRONE )
			 && !( pm->ps->eFlags & EF_TURRET_ACTIVE_DUCK ) ) {
			pm->ps->pm_flags |= PMF_PRONE;
			pm->ps->pm_flags &= ~PMF_DUCKED;
		} else if ( ( pm->ps->eFlags & EF_TURRET_ACTIVE_DUCK )
					&& !( pm->ps->eFlags & EF_TURRET_ACTIVE_PRONE ) ) {
			pm->ps->pm_flags |= PMF_DUCKED;
			pm->ps->pm_flags &= ~PMF_PRONE;
		} else {
			pm->ps->pm_flags &= ~( PMF_PRONE | PMF_DUCKED );
		}
	} else if ( !( pm->ps->pm_flags & PMF_IGNORE_INPUT ) ) {
		if ( pm->ps->pm_flags & PMF_LADDER ) {
			// no stance changes on a ladder
			if ( pm->cmd.wbuttons & ( WBUTTON_PRONE | WBUTTON_CROUCH ) ) {
				pm->cmd.wbuttons &= ~( WBUTTON_PRONE | WBUTTON_CROUCH );
				PM_AddEvent( EV_STANCE_FORCE_STAND );
			}
		}

		if ( pm->cmd.wbuttons & WBUTTON_PRONE ) {
			if ( ( pm->ps->pm_flags & PMF_PRONE )
				 || BG_CheckProne( pm->ps->clientNum, pm->ps->origin, pm->maxs[0], 30.0f,
								   pm->ps->viewangles[YAW],
								   &pm->ps->fTorsoHeight, &pm->ps->fTorsoPitch,
								   &pm->ps->fWaistPitch, qfalse,
								   pm->ps->groundEntityNum != ENTITYNUM_NONE, NULL,
								   pm->trace3, pm->trace2, qfalse ) ) {
				pm->ps->pm_flags |= PMF_PRONE;
			} else {
				pm->ps->pm_flags |= PMF_PRONE_BLOCKED;
				pm->ps->pm_flags &= ~PMF_PRONE_DIVE;
				if ( !( pm->cmd.wbuttons & WBUTTON_NOSTANCEEVENT ) ) {
					if ( pm->ps->pm_flags & PMF_DUCKED ) {
						PM_AddEvent( EV_STANCE_FORCE_CROUCH );
					} else {
						PM_AddEvent( EV_STANCE_FORCE_STAND );
					}
				}
			}
		} else if ( pm->cmd.wbuttons & WBUTTON_CROUCH ) {
			pm->ps->pm_flags &= ~PMF_PRONE_DIVE;
			if ( pm->ps->pm_flags & PMF_PRONE ) {
				pm->maxs[2] = 50;
				pm->trace3( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin,
							pm->ps->clientNum, pm->tracemask );
				if ( !trace.allsolid ) {
					pm->ps->pm_flags &= ~PMF_PRONE;
					pm->ps->pm_flags |= PMF_DUCKED;
				} else if ( !( pm->cmd.wbuttons & WBUTTON_NOSTANCEEVENT ) ) {
					PM_AddEvent( EV_STANCE_FORCE_PRONE );
				}
			} else {
				pm->ps->pm_flags |= PMF_DUCKED;
			}
		} else {
			pm->ps->pm_flags &= ~PMF_PRONE_DIVE;
			if ( pm->ps->pm_flags & PMF_PRONE ) {
				pm->maxs[2] = pm->ps->maxs[2];
				pm->trace3( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin,
							pm->ps->clientNum, pm->tracemask );
				if ( !trace.allsolid ) {
					pm->ps->pm_flags &= ~( PMF_PRONE | PMF_DUCKED );
				} else {
					pm->maxs[2] = 50;
					pm->trace3( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin,
								pm->ps->clientNum, pm->tracemask );
					if ( !trace.allsolid ) {
						pm->ps->pm_flags &= ~PMF_PRONE;
						pm->ps->pm_flags |= PMF_DUCKED;
					} else if ( !( pm->cmd.wbuttons & WBUTTON_NOSTANCEEVENT ) ) {
						PM_AddEvent( EV_STANCE_FORCE_PRONE );
					}
				}
			} else if ( pm->ps->pm_flags & PMF_DUCKED ) {
				pm->maxs[2] = pm->ps->maxs[2];
				pm->trace3( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin,
							pm->ps->clientNum, pm->tracemask );
				if ( !trace.allsolid ) {
					pm->ps->pm_flags &= ~PMF_DUCKED;
				} else if ( !( pm->cmd.wbuttons & WBUTTON_NOSTANCEEVENT ) ) {
					PM_AddEvent( EV_STANCE_FORCE_CROUCH );
				}
			}
		}
	}

	if ( pm->ps->pm_flags & PMF_PRONE ) {
#ifdef CGAMEDLL
		if ( g_debugProneCheck.integer >= 2 ) {	// keep: the two builds differ here
#else
		if ( g_debugProneCheck.integer == 2 ) {
#endif
			BG_CheckProne( pm->ps->clientNum, pm->ps->origin, pm->maxs[0], 30.0f,
						   pm->ps->viewangles[YAW], NULL, NULL, NULL, qfalse,
						   pm->ps->groundEntityNum != ENTITYNUM_NONE, NULL,
						   pm->trace3, pm->trace2, qfalse );
		}
		pm->ps->eFlags |= EF_PRONE;
		pm->ps->eFlags &= ~EF_CROUCHING;
		pm->ps->viewHeightTarget = pm->ps->proneViewHeight;

		if ( pm->ps->viewHeightCurrent == pm->ps->viewHeightTarget ) {
			pm->maxs[2] = 30;
		} else if ( PM_GetViewHeightLerp( -1, -1 ) > 0.5f ) {
			pm->maxs[2] = 30;
		} else {
			pm->maxs[2] = 50;
		}

		if ( !wasProne ) {
			// just went prone: slide the body to a clear spot and pick a
			// direction to lie in
			if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
				pm->ps->pm_flags &= ~PMF_PRONE_MOVEOVERRIDE;
				pm->ps->pm_flags &= ~PMF_ADS;
			}

			if ( !( pm->ps->pm_flags & PMF_ADS ) && pm->cmd.forwardmove ) {
				pm->ps->pm_flags |= PMF_PRONE_DIVE;
				if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
					jumpHeight = wasStanding ? 34.0f : 24.0f;
					pm->ps->velocity[2] = (float)sqrt( (double)( ( jumpHeight + jumpHeight )
																 * (float)pm->ps->gravity ) );
					pml.groundPlane = qfalse;
					pml.walking = qfalse;
					pm->ps->groundEntityNum = ENTITYNUM_NONE;
				}
				pm->ps->aimSpreadScale = 255.0f;
			}

			VectorCopy( pm->ps->origin, start );
			start[2] += 10.0f;
			pm->trace2( &trace, pm->ps->origin, pm->mins, pm->maxs, start,
						pm->ps->clientNum, pm->tracemask );
			VectorCopy( trace.endpos, start );
			pm->trace2( &trace, start, pm->mins, pm->maxs, pm->ps->origin,
						pm->ps->clientNum, pm->tracemask );
			VectorCopy( trace.endpos, pm->ps->origin );

			pm->ps->proneDirection = pm->ps->viewangles[YAW];

			VectorCopy( pm->ps->origin, end );
			end[2] -= 0.25f;
			pm->trace2( &trace, pm->ps->origin, pm->mins, pm->maxs, end,
						pm->ps->clientNum, pm->tracemask );
			if ( !trace.startsolid && trace.fraction < 1.0f ) {
				pm->ps->proneDirectionPitch = PitchOfVectorAlongYaw( pm->ps->proneDirection,
																   trace.normal );
			} else {
				pm->ps->proneDirectionPitch = 0;
			}

			pitch = AngleNormalize180( pm->ps->proneDirectionPitch - pm->ps->viewangles[PITCH] );
			if ( pitch < -45.0f ) {
				pm->ps->proneTorsoPitch = pm->ps->viewangles[PITCH] - 45.0f;
			} else if ( pitch > 45.0f ) {
				pm->ps->proneTorsoPitch = pm->ps->viewangles[PITCH] + 45.0f;
			} else {
				pm->ps->proneTorsoPitch = pm->ps->proneDirectionPitch;
			}
		}
	} else if ( pm->ps->pm_flags & PMF_DUCKED ) {
		pm->ps->eFlags |= EF_CROUCHING;
		pm->ps->eFlags &= ~EF_PRONE;
		pm->maxs[2] = 50;
		pm->ps->viewHeightTarget = pm->ps->crouchViewHeight;
	} else {
		pm->ps->eFlags &= ~( EF_CROUCHING | EF_PRONE );
		pm->maxs[2] = pm->ps->maxs[2];
		pm->ps->viewHeightTarget = pm->ps->standViewHeight;
	}

	if ( pm->ps->pm_flags & PMF_PRONE ) {
		pm->trace = pm->trace2;
	} else {
		pm->trace = pm->trace3;
	}
	pm->ps->eFlags |= EF_CAPSULE;

	PM_ViewHeightAdjust();
}

#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/*
================
PM_FootstepEvent

Advances the bob cycle and raises the footstep event; the ladder and the
step-up in PM_StepSlideMove drive it too.
================
*/
void PM_FootstepEvent( int oldBobCycle, int newBobCycle, qboolean footstep ) {
	trace_t trace;
	vec3_t mins, maxs, end;
	int surftype;
	int contents;

	if ( !( ( ( newBobCycle + 64 ) ^ ( oldBobCycle + 64 ) ) & 128 ) ) {
		return;
	}

	if ( !pm->waterlevel ) {
		if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
			if ( footstep && ( pm->ps->pm_flags & PMF_LADDER ) ) {
				// the ladder is behind us; find what it is made of
				VectorCopy( pm->mins, mins );
				mins[0] += 6.0f;
				mins[1] += 6.0f;
				mins[2] = 8.0f;
				VectorCopy( pm->maxs, maxs );
				maxs[0] -= 6.0f;
				maxs[1] -= 6.0f;
				if ( maxs[2] < 8.0f ) {
					maxs[2] = 8.0f;
				}
				contents = pm->tracemask & ~( CONTENTS_BODY | CONTENTS_PLAYERCLIP );
				end[0] = pm->ps->origin[0] - pm->ps->vLadderVec[0] * 31.0f;
				end[1] = pm->ps->origin[1] - pm->ps->vLadderVec[1] * 31.0f;
				end[2] = pm->ps->origin[2] - pm->ps->vLadderVec[2] * 31.0f;
				pm->trace( &trace, pm->ps->origin, mins, maxs, end, pm->ps->clientNum, contents );
				surftype = SURF_TYPE( trace.surfaceFlags );
				if ( trace.fraction == 1.0f || !surftype ) {
					surftype = SURF_TYPE_METAL;
				}
				PM_AddEvent( EV_FOOTSTEP_RUN_DEFAULT + surftype );
			}
		} else if ( footstep ) {
			PM_AddEvent( PM_FootstepForSurface( pm->ps->pm_flags ) );
		}
	} else if ( pm->waterlevel == 1 ) {
		// keep: separate waterlevel 1 and 2 arms match original codegen
		if ( pm->ps->pm_flags & PMF_PRONE ) {
			PM_AddEvent( EV_FOOTSTEP_PRONE_WATER );
		} else if ( pm->ps->pm_flags & PMF_WALKING ) {
			PM_AddEvent( EV_FOOTSTEP_WALK_WATER );
		} else {
			PM_AddEvent( EV_FOOTSTEP_RUN_WATER );
		}
	} else if ( pm->waterlevel == 2 ) {
		if ( pm->ps->pm_flags & PMF_PRONE ) {
			PM_AddEvent( EV_FOOTSTEP_PRONE_WATER );
		} else if ( pm->ps->pm_flags & PMF_WALKING ) {
			PM_AddEvent( EV_FOOTSTEP_WALK_WATER );
		} else {
			PM_AddEvent( EV_FOOTSTEP_RUN_WATER );
		}
	}
}

/*
================
PM_ShouldMakeFootsteps
================
*/
qboolean PM_ShouldMakeFootsteps( void ) {
	int flags;
	int walking;
	int stance;
	flags = pm->ps->pm_flags;
	walking = flags & PMF_WALKING;
	stance = PM_GetEffectiveStance( pm->ps );
	if ( flags & PMF_BACKWARDS_RUN ) {
		if ( stance == 1 || stance == 2 || walking ) {
			return qfalse;
		}
		return qtrue;
	}
	if ( stance == 1 || stance == 2 || walking ) {
		return qfalse;
	}
	return qtrue;

}

/*
================
PM_Footsteps
================
*/
static void PM_Footsteps( void ) {
	rfloat bobmove;     // keep: rfloat temporaries match original codegen
	int old;
	qboolean footstep;
	int effectiveStance;
	int walking;
	int anim;
	float speed;
	float climbSpeed;
	rfloat rate;
	rfloat lerp;

	anim = -1;

	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = (float)sqrt( (double)( pm->ps->velocity[0] * pm->ps->velocity[0]
										 + pm->ps->velocity[1] * pm->ps->velocity[1] ) );

	if ( pm->ps->eFlags & EF_TURRET_ACTIVE ) {
		if ( pm->ps->pm_flags & PMF_PRONE ) {
			BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLEPRONE, qtrue );
		} else if ( pm->ps->pm_flags & PMF_DUCKED ) {
			BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLECR, qtrue );
		} else {
			BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLE, qtrue );
		}
		return;
	}

	effectiveStance = PM_GetEffectiveStance( pm->ps );

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		if ( ( pm->ps->pm_flags & PMF_LADDER )
			 && pm->cmd.serverTime - pm->ps->jumpTime >= pm_ladderJumpTime ) {
			// airborne on a ladder: the bob cycle follows the climb speed
			speed = pm_ladderScale * 1.5f * 127.0f;
			climbSpeed = pm->ps->velocity[2];
			if ( pm->ps->pm_flags & PMF_WALKING ) {
				bobmove = climbSpeed / ( speed * pm->ps->walkSpeedScale ) * 0.34999999f;
			} else {
				bobmove = climbSpeed / ( speed * pm->ps->runSpeedScale ) * 0.44999999f;
			}

			if ( climbSpeed >= 0 ) {
				BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_CLIMBUP, qtrue );
			} else {
				BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_CLIMBDOWN, qtrue );
			}

			old = pm->ps->bobCycle;
			pm->ps->bobCycle = (int)( (float)pml.msec * bobmove + (float)old ) & 255;
			PM_FootstepEvent( old, pm->ps->bobCycle, qtrue );
		}
		return;
	}

	walking = pm->ps->pm_flags & PMF_WALKING;

	if ( pm->xyspeed < 10 ) {
		// standing still
		if ( pm->xyspeed < 1 ) {
			pm->ps->bobCycle = 0;
		}
		if ( pm->ps->viewHeightLerpTarget == pm->ps->proneViewHeight ) {
			anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLEPRONE, qtrue );
		} else if ( pm->ps->viewHeightLerpTarget == pm->ps->crouchViewHeight ) {
			anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLECR, qtrue );
		}
		if ( anim < 0 ) {
			BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLE, qtrue );
		}
		return;
	}

	// the animation rate is driven by the speed the command asks for, not by
	// the speed we actually got
	rate = pm->ps->speed;
	if ( pm->cmd.forwardmove ) {
		if ( pm->cmd.rightmove ) {
			rate = rate * ( 1.0f + ( ( pm->ps->strafeSpeedScale - 1.0f ) * 0.75f + 1.0f ) ) * 0.5f;
			if ( pm->cmd.forwardmove < 0 ) {
				rate = rate * ( pm->ps->backSpeedScale + 1.0f ) * 0.5f;
			}
		} else if ( pm->cmd.forwardmove < 0 ) {
			rate = rate * pm->ps->backSpeedScale;
		}
		BG_UpdateConditionValue( pm->ps->clientNum, ANIM_COND_STRAFING, STRAFING_NOT, qtrue );
	} else if ( pm->cmd.rightmove ) {
		rate = rate * ( ( pm->ps->strafeSpeedScale - 1.0f ) * 0.75f + 1.0f );
		if ( pm->cmd.rightmove > 0 ) {
			BG_UpdateConditionValue( pm->ps->clientNum, ANIM_COND_STRAFING, STRAFING_RIGHT, qtrue );
		} else {
			BG_UpdateConditionValue( pm->ps->clientNum, ANIM_COND_STRAFING, STRAFING_LEFT, qtrue );
		}
	}

	if ( walking ) {
		speed = rate * pm->ps->walkSpeedScale;
	} else {
		speed = rate * pm->ps->runSpeedScale;
	}

	lerp = PM_GetViewHeightLerp( pm->ps->crouchViewHeight, pm->ps->proneViewHeight );
	if ( lerp != 0 ) {
		rate = ( ( 1.0f - lerp ) * pm->ps->crouchSpeedScale + lerp * pm->ps->proneSpeedScale ) * speed;
	} else {
		lerp = PM_GetViewHeightLerp( pm->ps->proneViewHeight, pm->ps->crouchViewHeight );
		if ( lerp != 0 ) {
			rate = ( ( 1.0f - lerp ) * pm->ps->proneSpeedScale + lerp * pm->ps->crouchSpeedScale ) * speed;
		} else if ( effectiveStance == PM_EFF_STANCE_PRONE ) {
			rate = speed * pm->ps->proneSpeedScale;
		} else if ( effectiveStance == PM_EFF_STANCE_DUCKED ) {
			rate = speed * pm->ps->crouchSpeedScale;
		} else {
			rate = speed;
		}
	}

	// keep: the goto / label blocks match original codegen
	if ( effectiveStance == PM_EFF_STANCE_PRONE ) {
		goto footstepRate1;
footstepRate1:
		bobmove = ( pm->xyspeed / rate );
		if ( walking ) {
			bobmove *= 0.23999999f;
		} else {
			bobmove *= 0.25f;
		}
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_WALKPRONEBK, qtrue );
		} else {
			anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_WALKPRONE, qtrue );
		}
	} else if ( effectiveStance == PM_EFF_STANCE_DUCKED ) {
		goto footstepRate2;
footstepRate2:
		bobmove = ( pm->xyspeed / rate );
		if ( walking ) {
			bobmove *= 0.315f;
		} else {
			bobmove *= 0.34f;
		}
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			if ( walking ) {
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_WALKCRBK, qtrue );
			} else {
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_RUNCRBK, qtrue );
			}
		} else {
			if ( walking ) {
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_WALKCR, qtrue );
			} else {
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_RUNCR, qtrue );
			}
		}
	} else {
		goto footstepRate3;
footstepRate3:
		bobmove = ( pm->xyspeed / rate );
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			if ( walking ) {
				bobmove *= 0.32499999f;
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_WALKBK, qtrue );
			} else {
				bobmove *= 0.36000001f;
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_RUNBK, qtrue );
			}
		} else {
			if ( walking ) {
				bobmove *= 0.30500001f;
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_WALK, qtrue );
			} else {
				bobmove *= 0.33500001f;
				anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_RUN, qtrue );
			}
		}
	}

	footstep = PM_ShouldMakeFootsteps();

	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( (float)pml.msec * bobmove + (float)old ) & 255;

	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		// sliding to a stop: no footsteps
		if ( pm->xyspeed > 120 ) {
			return;
		}
		if ( pm->ps->viewHeightLerpTarget == pm->ps->proneViewHeight ) {
			anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLEPRONE, qtrue );
		} else if ( pm->ps->viewHeightLerpTarget == pm->ps->crouchViewHeight ) {
			anim = BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLECR, qtrue );
		}
		if ( anim < 0 ) {
			BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLE, qtrue );
		}
		return;
	}

	if ( anim < 0 ) {
		BG_AnimScriptAnimation( pm->ps, AISTATE_COMBAT, ANIM_MT_IDLE, qtrue );
	}

	PM_FootstepEvent( old, pm->ps->bobCycle, footstep );
}

/*
================
PM_FoliageSounds

Brush up against the foliage volume often enough and it rustles; the interval
scales from bg_foliagesnd_slowinterval down to bg_foliagesnd_fastinterval over
the speed band the two speed cvars name.
================
*/
static void PM_FoliageSounds( void ) {
	trace_t trace;
	vec3_t mins, maxs;
	float frac;
	int interval;

	if ( (float)bg_foliagesnd_minspeed.integer > pm->xyspeed ) {
		if ( bg_foliagesnd_resetinterval.integer + pm->ps->iFoliageSoundTime
			 < pm->cmd.serverTime ) {
			pm->ps->iFoliageSoundTime = 0;
		}
		return;
	}

	frac = ( pm->xyspeed - (float)bg_foliagesnd_minspeed.integer )
		   / (float)( bg_foliagesnd_maxspeed.integer - bg_foliagesnd_minspeed.integer );
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	interval = (int)( (float)( bg_foliagesnd_fastinterval.integer
							   - bg_foliagesnd_slowinterval.integer ) * frac
					  + (float)bg_foliagesnd_slowinterval.integer );

	if ( pm->ps->iFoliageSoundTime + interval >= pm->cmd.serverTime ) {
		return;
	}

	mins[0] = pm->mins[0] * 0.75f;
	mins[1] = pm->mins[1] * 0.75f;
	mins[2] = pm->mins[2] * 0.75f;
	maxs[0] = pm->maxs[0] * 0.75f;
	maxs[1] = pm->maxs[1] * 0.75f;
	maxs[2] = pm->maxs[2] * 0.75f;
	maxs[2] = pm->maxs[2] * 0.89999998f;

	pm->trace( &trace, pm->ps->origin, mins, maxs, pm->ps->origin, pm->ps->clientNum, CONTENTS_FOLIAGE );
	if ( trace.startsolid ) {
		PM_AddEvent( EV_FOLIAGE_SOUND );
		pm->ps->iFoliageSoundTime = pm->cmd.serverTime;
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {
	//
	// if just entered a water volume, play a sound
	//
	if ( !pml.previous_waterlevel && pm->waterlevel ) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if ( pml.previous_waterlevel && !pm->waterlevel ) {
		PM_AddEvent( EV_WATER_LEAVE );
	}
}

/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~( PMF_TIME_LAND | PMF_TIME_KNOCKBACK | PMF_TIME_HARDLAND );
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}
}

/*
================
PM_UpdateLean
================
*/
void PM_UpdateLean( playerState_t *ps, const usercmd_t *cmd,
					void ( *trace )( trace_t *results, const vec3_t start, const vec3_t mins,
									 const vec3_t maxs, const vec3_t end, int passEntityNum,
									 int contentMask ) ) {
	trace_t leanTrace;
	vec3_t start, end;
	vec3_t mins, maxs;
	int leaning;
	float maxLean;
	float leanFrac;
	float blockedFrac;
	int effectiveStance;

	leaning = 0;
	if ( ( cmd->wbuttons & ( WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT ) )
		 && !( ps->pm_flags & PMF_IGNORE_INPUT )
		 && ps->pm_type < PM_DEAD
		 && ( ps->groundEntityNum != ENTITYNUM_NONE || ps->pm_type == PM_NORMAL_LINKED ) ) {
		if ( cmd->wbuttons & WBUTTON_LEANLEFT ) {
			leaning = -1;
		}
		if ( cmd->wbuttons & WBUTTON_LEANRIGHT ) {
			leaning++;
		}
	}

	if ( ps->eFlags & EF_TURRET_ACTIVE ) {
		leaning = 0;
	}

	effectiveStance = PM_GetEffectiveStance( ps );
	if ( effectiveStance == PM_EFF_STANCE_PRONE ) {
		maxLean = 0.25f;
	} else if ( effectiveStance == PM_EFF_STANCE_DUCKED ) {
		maxLean = 0.5f;
	} else {
		maxLean = 1.0f;
	}

	leanFrac = ps->leanf;

	if ( !leaning ) {
		if ( leanFrac > 0 ) {
			leanFrac = leanFrac - FCAST( (float)pml.msec * 0.0035714286f ) * maxLean;
			if ( leanFrac < 0 ) {
				leanFrac = 0;
			}
		} else if ( leanFrac < 0 ) {
			leanFrac = leanFrac + FCAST( (float)pml.msec * 0.0035714286f ) * maxLean;
			if ( leanFrac > 0 ) {
				leanFrac = 0;
			}
		}
	} else if ( leaning > 0 ) {
		if ( leanFrac < maxLean ) {
			leanFrac = leanFrac + FCAST( (float)pml.msec * 0.0028571428f ) * maxLean;
		}
		if ( leanFrac > maxLean ) {
			leanFrac = maxLean;
		}
	} else {
		if ( leanFrac > -maxLean ) {
			leanFrac = leanFrac - FCAST( (float)pml.msec * 0.0028571428f ) * maxLean;
		}
		if ( leanFrac < -maxLean ) {
			leanFrac = -maxLean;
		}
	}

	ps->leanf = leanFrac;

	if ( ps->leanf != 0 && ps->pm_type != PM_NORMAL_LINKED ) {
		// trace out to the fully leaned position and give back however much
		// clearance there was
		leaning = PM_FloatSign( ps->leanf );
		VectorCopy( ps->origin, start );
		start[2] += ps->viewHeightCurrent;
		VectorCopy( start, end );

		AddLeanToPosition( end, ps->viewangles[YAW], (float)leaning, 16.0f, 20.0f );

		mins[0] = -8;
		mins[1] = -8;
		mins[2] = -8;
		maxs[0] = 8;
		maxs[1] = 8;
		maxs[2] = 8;

		trace( &leanTrace, start, mins, maxs, end, ps->clientNum, MASK_CLIENTSOLID );

		blockedFrac = UnGetLeanFraction( leanTrace.fraction );
		if ( fabs( ps->leanf ) > blockedFrac ) {
			ps->leanf = (float)PM_FloatSign( ps->leanf ) * blockedFrac;
		}
	}
}

// keep: improve-float-consistency (/Op) code, matches original codegen
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles are being
updated instead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd,
						  void ( *trace )( trace_t *results, const vec3_t start, const vec3_t mins,
										   const vec3_t maxs, const vec3_t end, int passEntityNum,
										   int contentMask ) ) {
	short temp;
	int i;
	float ladderYaw;
	float diff;
	float newDirection;
	float step;
	float limit;

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;     // no view changes at all
	}

	if ( ps->pm_type >= PM_DEAD ) {
		if ( ps->stats[STAT_DEAD_YAW] == 999 ) {
			ps->stats[STAT_DEAD_YAW] = (int)SHORT2ANGLE( (short)( cmd->angles[YAW] + ps->delta_angles[YAW] ) );
		}
		PM_UpdateLean( ps, cmd, trace );
		return;
	}

	// circularly clamp the angles with deltas
	// keep: do-while matches original codegen
	i = 0;
	do {
		temp = (short)( cmd->angles[i] + ps->delta_angles[i] );
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE( temp );
		i++;
	} while ( i < 3 );

	// on a ladder the view is capped around the ladder's facing
	if ( ( ps->pm_flags & PMF_LADDER ) && ps->groundEntityNum == ENTITYNUM_NONE ) {
		if ( bg_ladder_yawcap.integer ) {
			ladderYaw = vectoyaw( ps->vLadderVec ) + 180.0f;
			diff = AngleNormalize180( ladderYaw - ps->viewangles[YAW] );
			if ( diff > bg_ladder_yawcap.integer || diff < -bg_ladder_yawcap.integer ) {
				if ( diff > bg_ladder_yawcap.integer ) {
					diff = diff - bg_ladder_yawcap.integer;
				} else {
					diff = bg_ladder_yawcap.integer + diff;
				}
				ps->delta_angles[YAW] += ANGLE2SHORT( diff );
				if ( diff > 0 ) {
					ps->viewangles[YAW] = AngleNormalize360( ladderYaw - bg_ladder_yawcap.integer );
				} else {
					ps->viewangles[YAW] = AngleNormalize360( bg_ladder_yawcap.integer + ladderYaw );
				}
			}
		}
	}

	// prone caps the view around the direction the body is lying in
	if ( ( ps->pm_flags & PMF_PRONE ) && !( ps->eFlags & EF_TURRET_ACTIVE ) ) {
		if ( g_debugProneCheck.integer ) {
			vec3_t debugStart, debugEnd, debugForward;
			float angle;
			float sr, sp, sy, cr, cp, cy;

			VectorCopy( ps->origin, debugStart );
			debugStart[2] += ps->proneViewHeight;
			// keep: AngleVectors( ps->viewangles, debugForward, NULL, NULL ) inlined,
			// matches original codegen
			angle = ps->viewangles[YAW] * ( (float)M_PI * 2 / 360 );
			SinCos( angle, &sy, &cy );
			angle = ps->viewangles[PITCH] * ( (float)M_PI * 2 / 360 );
			SinCos( angle, &sp, &cp );
			debugForward[0] = cp * cy;
			debugForward[1] = cp * sy;
			debugForward[2] = -sp;
			if ( !debugForward ) {
				angle = ps->viewangles[ROLL] * ( (float)M_PI * 2 / 360 );
				SinCos( angle, &sr, &cr );
			}

			debugEnd[0] = debugForward[0] * 18.0f + debugStart[0];
			debugEnd[1] = debugForward[1] * 18.0f + debugStart[1];
			debugEnd[2] = debugForward[2] * 18.0f + debugStart[2];

			trap_AddDebugLine( debugStart, debugEnd, colorWhite, 1, 1 );
			G_DebugArc( debugStart, 16.0f,
						ps->proneDirection - bg_prone_yawcap.value,
						bg_prone_yawcap.value + ps->proneDirection,
						colorWhite, 1, 1 );
		}

		// keep: normalized twice, matches original codegen
		diff = AngleNormalize180( ps->proneDirection - ps->viewangles[YAW] );
		diff = AngleNormalize180( diff );

		if ( ( bg_prone_softyawedge.integer
			   && ( diff > (float)( bg_prone_yawcap.integer - 5 )
					|| diff < (float)( 5 - bg_prone_yawcap.integer ) ) )
			 || ( ( cmd->forwardmove || cmd->rightmove ) && diff != 0 ) ) {
			// turn the body toward the view if there is room for it
			step = pml.frametime * 55.0f;
			if ( fabs( diff ) < step ) {
				newDirection = ps->viewangles[YAW];
			} else if ( diff > 0 ) {
				newDirection = ps->proneDirection - step;
			} else {
				newDirection = step + ps->proneDirection;
			}

			if ( BG_CheckProne( ps->clientNum, ps->origin, ps->maxs[0], 30.0f, newDirection,
								&ps->fTorsoHeight, &ps->fTorsoPitch, &ps->fWaistPitch,
								qtrue, ps->groundEntityNum != ENTITYNUM_NONE, NULL,
								trace, NULL, qfalse ) ) {
				ps->proneDirection = newDirection;
			} else {
				limit = (float)bg_prone_yawcap.integer + 0.1f;
				if ( diff > limit || -limit > diff ) {
					ps->pm_flags |= PMF_PRONE_BLOCKED;
				}
			}
		}

		if ( diff > bg_prone_yawcap.integer || diff < -bg_prone_yawcap.integer ) {
			if ( diff > bg_prone_yawcap.integer ) {
				diff = diff - bg_prone_yawcap.integer;
			} else {
				diff = bg_prone_yawcap.integer + diff;
			}
			ps->delta_angles[YAW] += ANGLE2SHORT( diff );
			if ( diff > 0 ) {
				ps->viewangles[YAW] = SHORT2ANGLE( ANGLE2SHORT( ps->proneDirection - bg_prone_yawcap.integer ) );
			} else {
				ps->viewangles[YAW] = SHORT2ANGLE( ANGLE2SHORT( bg_prone_yawcap.integer + ps->proneDirection ) );
			}
		}

		// keep: normalized twice, matches original codegen
		diff = AngleNormalize180( ps->proneTorsoPitch - ps->viewangles[PITCH] );
		diff = AngleNormalize180( diff );
		if ( diff > 45.0f || diff < -45.0f ) {
			if ( diff > 45.0f ) {
				diff = diff - 45.0f;
			} else {
				diff = diff + 45.0f;
			}
			ps->delta_angles[PITCH] += ANGLE2SHORT( diff );
			if ( diff > 0 ) {
				ps->viewangles[PITCH] = AngleNormalize180( ps->proneTorsoPitch - 45.0f );
			} else {
				ps->viewangles[PITCH] = AngleNormalize180( ps->proneTorsoPitch + 45.0f );
			}
		}
	}

	if ( ps->pm_type != PM_UFO && ps->pm_type != PM_NOCLIP && ps->pm_type != PM_SPECTATOR ) {
		PM_UpdateLean( ps, cmd, trace );
	}
}

/*
================
PM_UpdatePronePitch

Track the ground under a prone body: the pitch it is lying at and the pitch the
torso should point down.
================
*/
static void PM_UpdatePronePitch( void ) {
	float target;
	float diff;
	float step;

	if ( !( pm->ps->pm_flags & PMF_PRONE ) ) {
		return;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		if ( !BG_CheckProne( pm->ps->clientNum, pm->ps->origin, pm->ps->maxs[0], 30.0f,
							 pm->ps->proneDirection,
							 &pm->ps->fTorsoHeight, &pm->ps->fTorsoPitch,
							 &pm->ps->fWaistPitch, qtrue, qfalse,
							 pml.groundPlane ? pml.groundTrace.normal : NULL,
							 pm->trace3, pm->trace2, qfalse ) ) {
			PM_AddEvent( EV_STANCE_FORCE_CROUCH );
			pm->ps->pm_flags |= PMF_PRONE_BLOCKED;
		}
	} else if ( pml.groundPlane && pml.groundTrace.normal[2] < MIN_WALK_NORMAL ) {
		PM_AddEvent( EV_STANCE_FORCE_CROUCH );
	}

	if ( pml.groundPlane ) {
		target = PitchOfVectorAlongYaw( pm->ps->proneDirection, pml.groundTrace.normal );
	} else {
		target = 0;
	}

	diff = AngleNormalize180( target - pm->ps->proneDirectionPitch );
	if ( diff != 0 ) {
		step = pml.frametime * 70.0f;
		if ( PM_FloatAbs( diff ) > step ) {
			pm->ps->proneDirectionPitch = (float)PM_FloatSign( diff ) * step + pm->ps->proneDirectionPitch;
		} else {
			pm->ps->proneDirectionPitch = diff + pm->ps->proneDirectionPitch;
		}
		pm->ps->proneDirectionPitch = AngleNormalize180( pm->ps->proneDirectionPitch );
	}

	if ( pml.groundPlane ) {
		target = PitchOfVectorAlongYaw( pm->ps->viewangles[YAW], pml.groundTrace.normal );
	} else {
		target = 0;
	}

	diff = AngleNormalize180( target - pm->ps->proneTorsoPitch );
	if ( diff != 0 ) {
		step = pml.frametime * 70.0f;
		if ( PM_FloatAbs( diff ) > step ) {
			pm->ps->proneTorsoPitch = (float)PM_FloatSign( diff ) * step + pm->ps->proneTorsoPitch;
		} else {
			pm->ps->proneTorsoPitch = diff + pm->ps->proneTorsoPitch;
		}
		pm->ps->proneTorsoPitch = AngleNormalize180( pm->ps->proneTorsoPitch );
	}
}

#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/*
================
PM_SetProneMovementOverride
================
*/
void PM_SetProneMovementOverride( void ) {
	if ( pm->ps->pm_flags & PMF_PRONE ) {
		pm->ps->pm_flags |= PMF_PRONE_MOVEOVERRIDE;
	}
}

/*
================
PM_UpdatePlayerWalkingFlag

Holding the aim-down-sight button walks, unless prone or reloading.
================
*/
static void PM_UpdatePlayerWalkingFlag( void ) {
	pm->ps->pm_flags &= ~PMF_WALKING;

	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	if ( !( pm->cmd.buttons & BUTTON_ADS ) ) {
		return;
	}
	if ( pm->ps->pm_flags & PMF_PRONE ) {
		return;
	}
	if ( !( pm->ps->pm_flags & PMF_ADS ) ) {
		return;
	}
	if ( pm->ps->weaponstate == WEAPON_RELOADING
		 || pm->ps->weaponstate == WEAPON_RELOAD_START
		 || pm->ps->weaponstate == WEAPON_RELOAD_END
		 || pm->ps->weaponstate == WEAPON_RELOAD_START_INTERUPT
		 || pm->ps->weaponstate == WEAPON_RELOADING_INTERUPT ) {
		return;
	}

	pm->ps->pm_flags |= PMF_WALKING;
}

/*
==============
PM_CheckLadderMove

Checks to see if we are on a ladder
==============
*/
static void PM_CheckLadderMove( void ) {
	vec3_t spot;
	vec3_t flatforward;
	vec3_t mins, maxs;
	trace_t trace;
	float tracedist;

	if ( pm->ps->pm_time ) {
		return;
	}

	if ( pml.walking ) {
		tracedist = 8.0f;
	} else {
		tracedist = 30.0f;
	}

	if ( ( pm->ps->pm_flags & PMF_LADDER ) && pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// keep looking at the ladder we are already on
		flatforward[0] = -pm->ps->vLadderVec[0];
		flatforward[1] = -pm->ps->vLadderVec[1];
		flatforward[2] = -pm->ps->vLadderVec[2];
	} else {
		flatforward[0] = pml.forward[0];
		flatforward[1] = pml.forward[1];
		flatforward[2] = 0;
		VectorNormalize( flatforward );
	}

	pm->ps->pm_flags &= ~PMF_LADDER;

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	if ( PM_GetEffectiveStance( pm->ps ) == PM_EFF_STANCE_PRONE ) {
		return;
	}
	if ( pm->cmd.serverTime - pm->ps->jumpTime < pm_ladderJumpTime ) {
		return;
	}

	VectorCopy( pm->mins, mins );
	mins[0] += 6.0f;
	mins[1] += 6.0f;
	mins[2] = 8.0f;
	VectorCopy( pm->maxs, maxs );
	maxs[0] -= 6.0f;
	maxs[1] -= 6.0f;
	if ( maxs[2] < 8.0f ) {
		maxs[2] = 8.0f;
	}

	VectorMA( pm->ps->origin, tracedist, flatforward, spot );
	pm->trace( &trace, pm->ps->origin, mins, maxs, spot, pm->ps->clientNum, pm->tracemask );

	if ( trace.fraction < 1.0f && ( trace.surfaceFlags & SURF_LADDER ) ) {
		if ( !pml.walking || pm->cmd.forwardmove > 0 ) {
			VectorCopy( trace.normal, pm->ps->vLadderVec );
			pm->ps->pm_flags |= PMF_LADDER;
		}
	}
}

// keep: improve-float-consistency (/Op) code, matches original codegen
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif

/*
============
PM_LadderMove
============
*/
static void PM_LadderMove( void ) {
	float wishspeed, scale;
	vec3_t wishdir, wishvel;
	vec3_t ladder_right;
	float upscale;
	float dot, drop;
	float negDot;
	int movementDir;

	if ( PM_CheckJump() ) {
		PM_AirMove();
		pm->ps->jumpTime = pm->cmd.serverTime;
		return;
	}

	upscale = ( pml.forward[2] + 0.25f ) * 2.5f;
	if ( upscale > 1.0f ) {
		upscale = 1.0f;
	} else if ( upscale < -1.0f ) {
		upscale = -1.0f;
	}

	// forward/right should be horizontal only
	pml.forward[2] = 0;
	VectorNormalize( pml.forward );
	pml.right[2] = 0;
	VectorNormalize2( pml.right, ladder_right );

	// the strafe axis is the part of right that lies in the ladder plane
	ProjectPointOntoVector( pml.right, ladder_right, pm->ps->vLadderVec );

	scale = PM_CmdScale( &pm->cmd );

	VectorClear( wishvel );

	if ( pm->cmd.forwardmove ) {
		wishvel[2] = (float)pm->cmd.forwardmove * pm_ladderScale * scale * upscale;
	}

	if ( pm->cmd.rightmove ) {
		// strafe, so we can jump off the ladder
		VectorMA( wishvel, (float)pm->cmd.rightmove * scale * 0.2f, pml.right, wishvel );
	}

	wishspeed = VectorNormalize2( wishvel, wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	if ( !pm->cmd.forwardmove ) {
		if ( pm->ps->velocity[2] > 0 ) {
			pm->ps->velocity[2] -= (float)pm->ps->gravity * pml.frametime;
			if ( pm->ps->velocity[2] < 0 ) {
				pm->ps->velocity[2] = 0;
			}
		} else {
			pm->ps->velocity[2] += (float)pm->ps->gravity * pml.frametime;
			if ( pm->ps->velocity[2] > 0 ) {
				pm->ps->velocity[2] = 0;
			}
		}
	}

	if ( !pm->cmd.rightmove ) {
		// sideways friction along the ladder (keep: wishdir reused)
		wishdir[0] = pml.right[0];
		wishdir[1] = pml.right[1];
		VectorNormalize2D( wishdir );
		dot = wishdir[0] * pm->ps->velocity[0] + wishdir[1] * pm->ps->velocity[1];
		if ( dot != 0 ) {
			negDot = -dot;
			pm->ps->velocity[0] += negDot * wishdir[0];
			pm->ps->velocity[1] += negDot * wishdir[1];
			drop = pm_ladderfriction * pml.frametime * dot;
			if ( fabs( dot ) > fabs( drop ) ) {
				if ( fabs( drop ) < 1.0 ) {
					drop = (float)PM_FloatSign( drop );
				}
				dot = dot - drop;
				pm->ps->velocity[0] += dot * wishdir[0];
				pm->ps->velocity[1] += dot * wishdir[1];
			}
		}
	}

	if ( !pml.walking ) {
		// hold on to the ladder
		dot = pm->ps->vLadderVec[0] * pm->ps->velocity[0]
			  + pm->ps->vLadderVec[1] * pm->ps->velocity[1];
		negDot = -dot;
		pm->ps->velocity[0] += negDot * pm->ps->vLadderVec[0];
		pm->ps->velocity[1] += negDot * pm->ps->vLadderVec[1];

		// keep: reusing dot matches original codegen
		dot = wishvel[2] > 0 ? -500.0f : -250.0f;
		pm->ps->velocity[0] += dot * pm->ps->vLadderVec[0];
		pm->ps->velocity[1] += dot * pm->ps->vLadderVec[1];
	}

	PM_StepSlideMove( qfalse );     // no gravity while going up ladder

	// always point the legs at the ladder (keep: scale reused for the yaw)
	scale = vectoyaw( pm->ps->vLadderVec ) + 180.0f;
	movementDir = (int)AngleNormalize180( scale - pm->ps->viewangles[YAW] );
	if ( abs( movementDir ) > 75 ) {
		movementDir = movementDir > 0 ? 75 : -75;
	}
	pm->ps->movementDir = (signed char)movementDir;
}

#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/*
================
PmoveSingle
================
*/
void PmoveSingle( pmove_t *pmove ) {
	vec3_t delta;
	rfloat deltaX;      // keep: rfloat matches original codegen

	BG_AnimUpdatePlayerStateConditions( pmove );

	pmove->watertype = 0;
	pmove->waterlevel = 0;

	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	// keep: pmove-> rather than pm-> matches original codegen
	if ( pm->ps->pm_flags & PMF_IGNORE_INPUT ) {
		pmove->cmd.buttons = 0;
		pmove->cmd.wbuttons &= ( WBUTTON_CROUCH | WBUTTON_PRONE | WBUTTON_NOSTANCEEVENT );
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	pm->ps->pm_flags &= ~PMF_PRONE_BLOCKED;

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->tracemask &= ~CONTENTS_BODY;    // corpses can fly through bodies
	}

	// a prone player who pushes harder on the stick than last frame breaks out
	// of whatever the weapon was doing
	if ( pm->ps->pm_flags & PMF_PRONE ) {
		if ( ( pm->cmd.forwardmove != pm->oldcmd.forwardmove
			   && fabs( (float)pm->cmd.forwardmove ) > fabs( (float)pm->oldcmd.forwardmove ) )
			 || ( pm->cmd.rightmove != pm->oldcmd.rightmove
				  && fabs( (float)pm->cmd.rightmove ) > fabs( (float)pm->oldcmd.rightmove ) ) ) {
			if ( PM_InteruptWeaponWithProneMove() ) {
				pm->ps->pm_flags &= ~PMF_PRONE_MOVEOVERRIDE;
				pm->ps->pm_flags &= ~PMF_ADS;
			}
		} else if ( !( pm->ps->pm_flags & PMF_ADS )
					&& ( pm->ps->weaponstate == WEAPON_READY
						 || pm->ps->weaponstate == WEAPON_RAISING
						 || pm->ps->weaponstate == WEAPON_DROPPING
						 || pm->ps->weaponstate == WEAPON_RELOADING ) ) {
			pm->ps->pm_flags &= ~PMF_PRONE_MOVEOVERRIDE;
		}
	} else {
		pm->ps->pm_flags &= ~PMF_PRONE_MOVEOVERRIDE;
	}

	pml.weaponInfo = bg_weaponInfo[pm->ps->weapon];

	if ( ( pm->ps->pm_flags & PMF_ADS ) && ( pm->ps->pm_flags & PMF_PRONE ) ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons
	pm->ps->eFlags &= ~EF_FIRING;
	if ( pm->ps->pm_type != PM_INTERMISSION && !( pm->ps->pm_flags & PMF_RESPAWNED ) ) {
		if ( pm->ps->weaponstate == WEAPON_READY || pm->ps->weaponstate == WEAPON_FIRING ) {
			if ( pm->ps->ammoclip[ bg_weaponInfo[pm->ps->weapon]->clipIndex ] ) {
				if ( ( pm->cmd.buttons & BUTTON_ATTACK ) && !( pm->cmd.buttons & BUTTON_TALK ) ) {
					pm->ps->eFlags |= EF_FIRING;
				}
			}
		}
	}

	// clear the respawned flag if attack is cleared
	if ( pm->ps->pm_type < PM_DEAD && !( pm->cmd.buttons & BUTTON_ATTACK ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.wbuttons &= ( WBUTTON_CROUCH | WBUTTON_PRONE | WBUTTON_NOSTANCEEVENT );
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset( &pml, 0, sizeof( pml ) );

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pmove->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy( pm->ps->origin, pml.previous_origin );

	// save old velocity for crashlanding
	VectorCopy( pm->ps->velocity, pml.previous_velocity );

	pml.frametime = pml.msec * 0.001f;

	pml.weaponInfo = bg_weaponInfo[pm->ps->weapon];

	PM_AdjustAimSpreadScale();

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd, pm->trace3 );

	AngleVectors( pm->ps->viewangles, pml.forward, pml.right, pml.up );

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( ( pm->ps->pm_flags & PMF_PRONE ) && ( pm->ps->pm_flags & PMF_PRONE_MOVEOVERRIDE ) ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
	}

	switch ( pm->ps->pm_type ) {
	case PM_SPECTATOR:
		PM_UpdateAimDownSightFlag();
		PM_UpdatePlayerWalkingFlag();
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;

	case PM_NOCLIP:
		PM_UpdateAimDownSightFlag();
		PM_UpdatePlayerWalkingFlag();
		PM_NoclipMove();
		PM_DropTimers();
		return;

	case PM_UFO:
		PM_UpdateAimDownSightFlag();
		PM_UpdatePlayerWalkingFlag();
		PM_UFOMove();
		PM_DropTimers();
		return;

	case PM_NORMAL_LINKED:
	case PM_DEAD_LINKED:
		// the mover owns the origin
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		PM_UpdateAimDownSightFlag();
		PM_UpdatePlayerWalkingFlag();
		PM_CheckDuck();
		PM_DropTimers();
		PM_Weapon();
		if ( pm->ps->eFlags & EF_TURRET_ACTIVE ) {
			PM_Footsteps();
		}
		return;

	case PM_INTERMISSION:
		return;     // no movement at all

	default:
		break;
	}

	if ( pm->ps->eFlags & EF_TURRET_ACTIVE ) {
		// a mounted weapon holds us in place
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		PM_UpdateAimDownSightFlag();
		PM_UpdatePlayerWalkingFlag();
		PM_CheckDuck();
		PM_DropTimers();
		PM_Footsteps();
		return;
	}

	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	PM_CheckDuck();

	// set groundentity
	PM_GroundTrace();

	PM_UpdateAimDownSightFlag();
	PM_UpdatePlayerWalkingFlag();
	PM_UpdatePronePitch();

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove();
	}

	PM_CheckLadderMove();

	PM_DropTimers();

	if ( pm->ps->pm_flags & PMF_LADDER ) {
		PM_LadderMove();
	} else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// footstep events / legs animations
	PM_Footsteps();

	PM_FoliageSounds();

	// entering / leaving water splashes
	PM_WaterEvents();

	// if the move came out a lot shorter than the velocity asked for, take the
	// velocity from the move that actually happened
	// keep: deltaX and the sum order match original codegen
	deltaX = (rfloat)pm->ps->origin[0] - pml.previous_origin[0];
	delta[1] = pm->ps->origin[1] - pml.previous_origin[1];
	delta[2] = pm->ps->origin[2] - pml.previous_origin[2];
	if ( ( (rfloat)delta[2] * delta[2] + (rfloat)delta[1] * delta[1] + deltaX * deltaX )
		 / ( (rfloat)pml.frametime * pml.frametime )
		 < ( (rfloat)pm->ps->velocity[2] * pm->ps->velocity[2]
			 + (rfloat)pm->ps->velocity[1] * pm->ps->velocity[1]
			 + (rfloat)pm->ps->velocity[0] * pm->ps->velocity[0] ) * 0.25f ) {
		pm->ps->velocity[0] = FCAST( deltaX * ( 1.0f / pml.frametime ) );
		pm->ps->velocity[1] = delta[1] * ( 1.0f / pml.frametime );
		pm->ps->velocity[2] = delta[2] * ( 1.0f / pml.frametime );
	}

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector( pm->ps->velocity );
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove( pmove_t *pmove ) {
	int finalTime;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;     // should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pm = pmove;
	pm->numtouch = 0;

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		} else {
			if ( msec > 66 ) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.upmove = 20;
		}
	}
}

/*
================
BG_GetSpeed

The speed the animation system and the HUD should see: the climb rate on a
ladder, the ground speed otherwise.
================
*/
float BG_GetSpeed( const playerState_t *ps, int time ) {
	if ( ps->pm_flags & PMF_LADDER ) {
		if ( time - ps->jumpTime < 500 ) {
			return 0;
		}
		return ps->velocity[2];
	}
	return (float)sqrt( (double)( ps->velocity[0] * ps->velocity[0]
								  + ps->velocity[1] * ps->velocity[1] ) );
}
