// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "cg_local.h"
#include "../game_mp/bg_public.h"

#define PITCH                   0
#define YAW                     1
#define ROLL                    2

#ifndef M_PI
#define M_PI                    3.14159265358979323846f
#endif

#define FOCUS_DISTANCE          512

#define STEP_TIME               100

#define LAND_DEFLECT_TIME       150
#define LAND_RETURN_TIME        300

#define MASK_SOLID              0x11
#define CONTENTS_WATER          0x20

#define RDF_NOWORLDMODEL        0x0001
#define RDF_SKYBOXPORTAL        0x0008
#define RDF_DRAWSKYBOX          0x0010
#define RDF_UNDERWATER          0x0020

#define PM_INTERMISSION         5
#define PM_DEAD_OR_SPECTATING   6   // dead or spectating

#define PMF_TURRET              0x00040000
#define EF_MOUNTED              0x0000C000  // riding a mounted weapon
#define PMF_LADDER              0x00000010

#define ENTITYNUM_NONE_CG       ( MAX_GENTITIES - 1 )

// the skybox portal description; not CS_FOGVARS (12), which is the fog description
#define CS_SKYBOXORG            10

#define FOG_PORTALVIEW          2

#define SNAPFLAG_NOT_ACTIVE     2

#define CMD_BACKUP              64

#define LAG_SAMPLES             128

void        CG_Error( const char *msg, ... );
void        Com_Error( int level, const char *fmt, ... );
void        CG_AddLagometerFrameInfo( void );
void        CG_DrawActive( int stereoView );
void        CG_ShakeCamera( void );
void        CG_PerturbCamera( void );

void        CG_AddMarks( void );
void        CG_AddLocalEntities( void );

void        BG_CalculateViewAngles( bgViewAngleState_t *state, vec3_t angles );

void        AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );


static int skyPortalFogSet;     // only set the portal fog values once

/*
=================
CG_FxSetTestPosition

Places the FX test position 100 units in front of the view
=================
*/
void CG_FxSetTestPosition( void ) {
	rfloat z;       // keep: z is printed unrounded, matches original codegen

	cg.fxTestPos[0] = cg.refdef.viewaxis[0][0] * 100.0f + cg.refdef.vieworg[0];
	cg.fxTestPos[1] = cg.refdef.viewaxis[0][1] * 100.0f + cg.refdef.vieworg[1];
	z = (rfloat)cg.refdef.viewaxis[0][2] * 100.0f + cg.refdef.vieworg[2];
	cg.fxTestPos[2] = z;

	Com_Printf( "\n\nFX Testing position set to: (%f, %f, %f)\n\n",
				cg.fxTestPos[0], cg.fxTestPos[1], z );
}

/*
=================
CG_FxRestart
=================
*/
void CG_FxRestart( void ) {
	Com_Printf( "FX Restarting so off-line changes are loaded.\n" );
	trap_FX_FreeAll();
	trap_FX_Init();
}

/*
=================
CG_FxTest
=================
*/
void CG_FxTest( void ) {
	int effect;

	if ( trap_Argc() < 2 ) {
		Com_Printf( "Must supply filename from base path.  Optional restart time.\n" );
	}

	strncpy( cg.fxTestName, CG_Argv( 1 ), 63 );
	cg.fxTestName[63] = 0;

	effect = trap_FX_RegisterEffect( cg.fxTestName );
	Com_Printf( "Spawning Fx %s with ID: %d\n", cg.fxTestName, effect );
	trap_FX_PlaySimpleEffectID( effect, cg.fxTestPos );

	cg.fxTestTime = cg.time;

	if ( trap_Argc() == 3 ) {
		cg.fxTestRestart = (int)( atof( CG_Argv( 2 ) ) * 1000.0 );
	} else {
		cg.fxTestRestart = 0;
	}
}

/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect( void ) {
	int size;
	int xsize, ysize;

	// the intermission should allways be full screen
	if ( cg.nextSnap->ps.pm_type == PM_INTERMISSION ) {
		xsize = 100;
		ysize = 100;
	} else {
		size = cg_viewsize.integer;
		if ( size < 30 ) {
			trap_Cvar_Setvar( &cg_viewsize, "30" );
			xsize = 30;
			ysize = 30;
		} else if ( size > 100 ) {
			trap_Cvar_Setvar( &cg_viewsize, "100" );
			xsize = 100;
			ysize = 100;
		} else {
			xsize = size;
			ysize = size;
		}
	}

	if ( cg_letterbox.integer ) {
		xsize = (int)( xsize * 0.85f );
	}

	cg.refdef.width = cgs.glconfig.vidWidth * ysize / 100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight * xsize / 100;
	cg.refdef.height &= ~1;

	cg.refdef.x = ( cgs.glconfig.vidWidth - cg.refdef.width ) / 2;
	cg.refdef.y = ( cgs.glconfig.vidHeight - cg.refdef.height ) / 2;
}

/*
===============
CG_OffsetThirdPersonView
===============
*/
static void CG_OffsetThirdPersonView( void ) {
	vec3_t          forward, right, up;
	vec3_t          view;
	vec3_t          focusAngles;
	trace_t         trace;
	static vec3_t   mins = { -4, -4, -4 };
	static vec3_t   maxs = { 4, 4, 4 };
	vec3_t          focusPoint;
	float           focusDist;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewHeightCurrent;

	focusAngles[PITCH] = cg.refdefViewAngles[PITCH];
	focusAngles[YAW] = cg.refdefViewAngles[YAW];

	// if dead, look at killer
	if ( cg.predictedPlayerState.pm_type >= PM_DEAD_OR_SPECTATING ) {
		focusAngles[YAW] = (float)cg.predictedPlayerState.stats[1];
		cg.refdefViewAngles[YAW] = (float)cg.predictedPlayerState.stats[1];
	}

	if ( focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;        // don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );
	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	VectorMA( view, -cg_thirdPersonRange.value, forward, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near
	// clipping anything
	CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view,
			  cg.predictedPlayerState.clientNum, MASK_SOLID );

	if ( trace.fraction != 1.0 ) {
		VectorCopy( trace.endpos, view );
		view[2] += ( 1.0f - trace.fraction ) * 32;
		// try another trace to this position, because a tunnel may have the
		// ceiling close enough that this is poking out
		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view,
				  cg.predictedPlayerState.clientNum, MASK_SOLID );
		VectorCopy( trace.endpos, view );
	}

	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = (float)sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;      // should never happen
	}
	// keep: atan2 rounded to float first, matches original codegen
	cg.refdefViewAngles[PITCH] = (float)atan2( focusPoint[2], focusDist ) * ( -180 / M_PI );
}

/*
===============
CG_StepOffset

Smooth out stair climbing
===============
*/
static void CG_StepOffset( void ) {
	int timeDelta;

	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < 0 ) {
		cg.stepTime = cg.time;
	}
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange * ( STEP_TIME - timeDelta ) * 0.01f;
	}
}

/*
===============
CG_KickAngles

Integrates the mounted-weapon kick in 5 ms steps.  cg.kick_angles is the
velocity and cg.kick_origin the angle it drives; both are cleared by
CG_DrawActiveFrame whenever the player is not on a turret.
===============
*/
static void CG_KickAngles( void ) {
	int     msec;
	int     i;
	rfloat  step;       // keep: step/delta/sum/v types match original codegen
	float   dir;
	float   accel;
	rfloat  delta;
	double  sum;
	float   v;

	for ( msec = cg.frametime ; msec > 0 ; msec -= 5 ) {
		step = (rfloat)( msec > 5 ? 5 : msec ) * 0.001f;

		for ( i = 0 ; i < 3 ; i++ ) {
			if ( cg.kick_angles[i] == 0.0f && cg.kick_origin[i] == 0.0f ) {
				continue;
			}

			if ( cg.kick_origin[i] != 0.0f ) {
				if ( cg.kick_origin[i] > 0.0f ) {
					dir = -1.0f;
				} else {
					dir = 1.0f;
				}

				if ( cg.predictedPlayerState.weapon ) {
					if ( cg.predictedPlayerState.fWeaponPosFrac > 0.5f ) {
						accel = dir * cg.weaponInfo->adsViewKickCenterSpeed;
					} else {
						accel = dir * cg.weaponInfo->hipViewKickCenterSpeed;
					}
				} else {
					accel = dir * 2400.0f;
				}

				cg.kick_angles[i] += accel * step;
			}

			delta = step * cg.kick_angles[i];
			if ( delta * cg.kick_origin[i] < 0.0f ) {
				delta *= 0.06f;
			}

			sum = delta + cg.kick_origin[i];
			if ( FCAST( sum ) * cg.kick_origin[i] >= 0.0f ) {
				v = (float)sum;
				cg.kick_origin[i] = (float)sum;
				// keep: one velocity clear per arm, matches original codegen
				if ( v == 0.0f ) {
					cg.kick_angles[i] = 0.0f;
				} else if ( fabs( v ) > 10.0 ) {
					if ( v > 0.0f ) {
						v = 10.0f;
					} else {
						v = -10.0f;
					}
					cg.kick_origin[i] = v;
					cg.kick_angles[i] = 0.0f;
				}
			} else {
				cg.kick_origin[i] = 0.0f;
				cg.kick_angles[i] = 0.0f;
			}
		}
	}
}

/*
===============
CG_CalcVerticalBob

The vertical view bob
===============
*/
static float CG_CalcVerticalBob( float bobCycle, float speed, float bobMax ) {
	float amount;

	// keep: the multiply in each arm, matches original codegen
	if ( cg.predictedPlayerState.viewHeightTarget == cg.predictedPlayerState.proneViewHeight ) {
		amount = cg_bobAmplitudeProne.value * speed;
	} else if ( cg.predictedPlayerState.viewHeightTarget == cg.predictedPlayerState.crouchViewHeight ) {
		amount = cg_bobAmplitudeDucked.value * speed;
	} else {
		amount = cg_bobAmplitudeStanding.value * speed;
	}

	if ( amount > bobMax ) {
		amount = bobMax;
	}

	// keep: a bare cast on the second sin, matches original codegen
	return (float)( ( FCAST( sin( bobCycle * 4.0f + M_PI * 0.5f ) ) * 0.2f + (rfloat)sin( bobCycle + bobCycle ) )
					* amount * 0.75f );
}

/*
===============
CG_CalcHorizontalBob

The horizontal view bob
===============
*/
static float CG_CalcHorizontalBob( float bobCycle, float speed, float bobMax ) {
	float  amplitude;
	rfloat amount;

	if ( cg.predictedPlayerState.viewHeightTarget == cg.predictedPlayerState.proneViewHeight ) {
		amplitude = cg_bobAmplitudeProne.value;
	} else if ( cg.predictedPlayerState.viewHeightTarget == cg.predictedPlayerState.crouchViewHeight ) {
		amplitude = cg_bobAmplitudeDucked.value;
	} else {
		amplitude = cg_bobAmplitudeStanding.value;
	}

	amount = FCAST( (double)amplitude * speed );
	if ( amount > bobMax ) {
		amount = bobMax;
	}

	return (float)( amount * sin( bobCycle ) );
}

/*
===============
CG_OffsetFirstPersonView
===============
*/
static void CG_OffsetFirstPersonView( void ) {
	bgViewAngleState_t  viewState;
	vec3_t              angles;
	vec3_t              right;
	float               hbob;
	float               delta;      // also the vertical bob
	float               f;
	float               floorZ;

	if ( cg.nextSnap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	// add angles based on damage kick and velocity
	viewState.ps = &cg.predictedPlayerState;
	viewState.viewKickStartTime = cg.damageTime
								  ? cg.damageTime - cg.predictedPlayerState.deltaTime : 0;
	viewState.time = cg.time - cg.predictedPlayerState.deltaTime;
	viewState.viewKickPitch = cg.v_dmg_pitch;
	viewState.viewKickRoll = cg.v_dmg_roll;
	viewState.speed = cg.xyspeed;

	BG_CalculateViewAngles( &viewState, angles );

	cg.refdefViewAngles[PITCH] += angles[PITCH];
	cg.refdefViewAngles[YAW] += angles[YAW];
	cg.refdefViewAngles[ROLL] += angles[ROLL];

	// add view height
	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewHeightCurrent;

	// a mounted weapon owns the eye entirely
	if ( cg.predictedPlayerState.eFlags & EF_MOUNTED ) {
		return;
	}

	// add bob height
	delta = CG_CalcVerticalBob( cg.bobCycle, cg.xyspeed, cg_bobMax.value );
	cg.refdef.vieworg[2] += delta;

	// and the sideways half of it, along the view right
	hbob = CG_CalcHorizontalBob( cg.bobCycle, cg.xyspeed, cg_bobMax.value );
	AngleVectors( cg.refdefViewAngles, NULL, right, NULL );
	VectorMA( cg.refdef.vieworg, hbob, right, cg.refdef.vieworg );

	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < 0 ) {
		cg.landTime = cg.time - ( LAND_DEFLECT_TIME + LAND_RETURN_TIME );
	}
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0f - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// lean the eye out sideways
	AddLeanToPosition( cg.refdef.vieworg, cg.refdefViewAngles[YAW],
					   cg.predictedPlayerState.leanf, 16.0f, 20.0f );

	// never let the eye sink into the floor
	floorZ = cg.predictedPlayerState.origin[2] + 8.0f;
	if ( cg.refdef.vieworg[2] < floorZ ) {
		cg.refdef.vieworg[2] = floorZ;
	}
}

/*
====================
CG_GetViewFov

The unzoomed fov, lerped towards the weapon's ADS fov by ps.fWeaponPosFrac.
(keep: the gotos and the unrounded return match the original codegen)
====================
*/
static rfloat CG_GetViewFov( void ) {
	rfloat fov;
	rfloat frac;
	float posFrac;

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fov = 90.0f;
		goto mounted;
	}

	fov = cg_fov.value;
	if ( fov < 80.0f ) {
		fov = 80.0f;
	} else if ( fov > 160.0f ) {
		fov = 160.0f;
	}

	if ( BG_IsAimDownSightWeapon( cg.predictedPlayerState.weapon ) ) {
		posFrac = cg.predictedPlayerState.fWeaponPosFrac;
		if ( posFrac == 1.0f ) {
			fov = cg.weaponInfo->adsZoomFov;
			goto mounted;
		}
		if ( posFrac != 0.0f ) {
			if ( cg.playerEntity.bPositionToADS ) {
				frac = posFrac - ( 1.0f - (rfloat)cg.weaponInfo->adsZoomInFrac );
				if ( !( frac > 0.0f ) ) {
					goto mounted;
				}
				frac = frac / cg.weaponInfo->adsZoomInFrac;
			} else {
				frac = posFrac - ( 1.0f - (rfloat)cg.weaponInfo->adsZoomOutFrac );
				if ( !( frac > 0.0f ) ) {
					goto mounted;
				}
				frac = frac / cg.weaponInfo->adsZoomOutFrac;
			}
			if ( frac > 0.0f ) {
				fov = fov - ( fov - cg.weaponInfo->adsZoomFov ) * frac;
			}
		}
	}

mounted:
	if ( cg.predictedPlayerState.eFlags & EF_MOUNTED ) {
		return 55.0f;
	}
	return fov;
}

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
Returns qtrue if the view is underwater.
====================
*/
static int CG_CalcFov( void ) {
	static float lastfov = 90;  // for transitions back from zoomed in modes (unused)
	rfloat  fov_x, fov_y;
	rfloat  v;
	int     inwater;

	fov_x = CG_GetViewFov();

	fov_y = FCAST( atan2( cg.refdef.height, cg.refdef.width / tan( fov_x * ( M_PI / 360.0f ) ) ) )
			* ( 360.0f / M_PI );

	// warp if underwater
	if ( CG_PointContents( cg.refdef.vieworg, -1, CONTENTS_WATER ) ) {
		v = sin( (rfloat)cg.time * 0.0025132743f );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
		cg.refdef.rdflags |= RDF_UNDERWATER;
	} else {
		cg.refdef.rdflags &= ~RDF_UNDERWATER;
		inwater = qfalse;
	}

	if ( CG_PointContents( cg.refdef.vieworg, -1, CONTENTS_WATER ) ) {
		cg.refdef.rdflags |= RDF_UNDERWATER;
	} else {
		cg.refdef.rdflags &= ~RDF_UNDERWATER;
	}

	// set it
	cg.refdef.fov_x = (float)fov_x;
	cg.refdef.fov_y = (float)fov_y;

	cg.zoomSensitivity = (float)( fov_x / cg_fov.value );

	return inwater;
}

/*
===============
CG_CalcCubemapViewValues

Sets up one of the six axis-aligned cubemap faces.  cg.cubemapShot is the face
number and cg.cubemapSize the edge length.
===============
*/
static void CG_CalcCubemapViewValues( void ) {
	playerState_t *ps;

	ps = &cg.predictedPlayerState;
	cg.refdef.x = 0;
	cg.refdef.y = 0;
	cg.refdef.width = cg.cubemapSize + 2;
	cg.refdef.height = cg.cubemapSize + 2;

	// keep: a double constant here, unlike CG_CalcFov's float
	cg.refdef.fov_x = (float)( atan2( cg.cubemapSize + 2, cg.cubemapSize )
							   * ( 360.0 / 3.14159265358979323846 ) );
	cg.refdef.fov_y = cg.refdef.fov_x;

	// keep: through ps, matches original codegen
	VectorCopy( ps->origin, cg.refdef.vieworg );
	cg.refdef.vieworg[2] += ps->viewHeightCurrent;

	switch ( cg.cubemapShot ) {
	case 1:
		cg.refdef.viewaxis[0][0] = 0;   cg.refdef.viewaxis[0][1] = 0;   cg.refdef.viewaxis[0][2] = 1;
		cg.refdef.viewaxis[1][0] = 0;   cg.refdef.viewaxis[1][1] = 1;   cg.refdef.viewaxis[1][2] = 0;
		cg.refdef.viewaxis[2][0] = -1;  cg.refdef.viewaxis[2][1] = 0;   cg.refdef.viewaxis[2][2] = 0;
		break;
	case 2:
		cg.refdef.viewaxis[0][0] = 0;   cg.refdef.viewaxis[0][1] = 0;   cg.refdef.viewaxis[0][2] = -1;
		cg.refdef.viewaxis[1][0] = 0;   cg.refdef.viewaxis[1][1] = 1;   cg.refdef.viewaxis[1][2] = 0;
		cg.refdef.viewaxis[2][0] = 1;   cg.refdef.viewaxis[2][1] = 0;   cg.refdef.viewaxis[2][2] = 0;
		break;
	case 3:
		cg.refdef.viewaxis[0][0] = -1;  cg.refdef.viewaxis[0][1] = 0;   cg.refdef.viewaxis[0][2] = 0;
		cg.refdef.viewaxis[1][0] = 0;   cg.refdef.viewaxis[1][1] = -1;  cg.refdef.viewaxis[1][2] = 0;
		cg.refdef.viewaxis[2][0] = 0;   cg.refdef.viewaxis[2][1] = 0;   cg.refdef.viewaxis[2][2] = 1;
		break;
	case 4:
		cg.refdef.viewaxis[0][0] = 1;   cg.refdef.viewaxis[0][1] = 0;   cg.refdef.viewaxis[0][2] = 0;
		cg.refdef.viewaxis[1][0] = 0;   cg.refdef.viewaxis[1][1] = 1;   cg.refdef.viewaxis[1][2] = 0;
		cg.refdef.viewaxis[2][0] = 0;   cg.refdef.viewaxis[2][1] = 0;   cg.refdef.viewaxis[2][2] = 1;
		break;
	case 5:
		cg.refdef.viewaxis[0][0] = 0;   cg.refdef.viewaxis[0][1] = -1;  cg.refdef.viewaxis[0][2] = 0;
		cg.refdef.viewaxis[1][0] = 1;   cg.refdef.viewaxis[1][1] = 0;   cg.refdef.viewaxis[1][2] = 0;
		cg.refdef.viewaxis[2][0] = 0;   cg.refdef.viewaxis[2][1] = 0;   cg.refdef.viewaxis[2][2] = 1;
		break;
	case 6:
		cg.refdef.viewaxis[0][0] = 0;   cg.refdef.viewaxis[0][1] = 1;   cg.refdef.viewaxis[0][2] = 0;
		cg.refdef.viewaxis[1][0] = -1;  cg.refdef.viewaxis[1][1] = 0;   cg.refdef.viewaxis[1][2] = 0;
		cg.refdef.viewaxis[2][0] = 0;   cg.refdef.viewaxis[2][1] = 0;   cg.refdef.viewaxis[2][2] = 1;
		break;
	default:
		break;
	}
}

/*
===============
CG_CalcTurretViewValues

The eye rides the turret's tag_player bone.
===============
*/
static void CG_CalcTurretViewValues( void ) {
	centity_t   *cent;
	float       matrix[16];     // CG_DObjGetWorldTagMatrix writes a full 4x4
	float       angles[2];      // keep: an array, matches original codegen
	DObj        *obj;

	if ( !( cg.predictedPlayerState.eFlags & EF_MOUNTED ) ) {
		return;
	}
	if ( cg.predictedPlayerState.viewlocked_entNum == ENTITYNUM_NONE_CG ) {
		return;
	}
	if ( !cg.predictedPlayerState.viewlocked ) {
		return;
	}

	BG_EvaluateTrajectory( &cg_entities[ cg.predictedPlayerState.viewlocked_entNum ].currentState.apos,
						   cg.time, cg.refdefViewAngles );

	cent = &cg_entities[ cg.predictedPlayerState.viewlocked_entNum ];

	obj = trap_GetClientDObj( cent->currentState.number );
	if ( !obj ) {
		return;
	}

	CG_CalcEntityLerpPositions( cent );

	if ( !CG_DObjGetWorldTagMatrix( obj, "tag_player", cent, matrix ) ) {
		Com_Error( ERR_DROP, "\x15" "Turret has no bone: tag_player\n" );
	}

	angles[PITCH] = LerpAngle( cent->currentState.angles2[0], cent->nextState.angles2[0],
							   cg.frameInterpolation );
	angles[YAW] = LerpAngle( cent->currentState.angles2[1], cent->nextState.angles2[1],
							 cg.frameInterpolation );

	cg.refdefViewAngles[PITCH] += angles[PITCH];
	cg.refdefViewAngles[YAW] += angles[YAW];

	if ( !cg.renderingThirdPerson && cg.predictedPlayerState.viewlocked == 2 ) {
		float turretShake;

		turretShake = rand() * 0.000030517578f;
		cg.refdefViewAngles[PITCH] += turretShake + turretShake - 1.0f;
		turretShake = rand() * 0.000030517578f;
		cg.refdefViewAngles[YAW] += turretShake + turretShake - 1.0f;
	}

	// the eye origin is row 3 of the 4x4
	cg.refdef.vieworg[0] = matrix[12];
	cg.refdef.vieworg[1] = matrix[13];
	cg.refdef.vieworg[2] = matrix[14] - cg.predictedPlayerState.viewHeightCurrent;
}

// keep: CG_CalcViewValues is built with improved float consistency
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static int CG_CalcViewValues( void ) {
	playerState_t   *ps;
	float           f;
	float           speed;
	vec3_t          right;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	if ( cg.cubemapShot ) {
		CG_CalcCubemapViewValues();
		return 0;
	}

	// calculate size of 3D view
	CG_CalcVrect();

	if ( cg.cameraMode ) {
		CG_Error( "Cinimatic Cameras are not available in the MP exe.\n" );
	}

	ps = &cg.predictedPlayerState;

	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
	} else {

		// mask: the interpolated bobCycle can wrap past 255
		// (keep: FCAST, matches original codegen)
		cg.bobCycle = FCAST( ( ps->bobCycle & 255 ) / 255.0f * ( 2 * M_PI ) ) + ( 2 * M_PI );

		if ( ps->pm_flags & PMF_LADDER ) {
			if ( cg.time - ps->jumpTime < 500 ) {
				speed = 0.0f;
			} else {
				speed = ps->velocity[2];
			}
		} else {
			// keep: not through ps, and FCAST, match original codegen
			speed = FCAST( sqrt( cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0]
								 + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1] ) );
		}
		cg.xyspeed = speed;

		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );

		// add error decay
		if ( cg_errordecay.value > 0 ) {
			int t;

			t = cg.time - cg.predictedErrorTime;
			f = ( cg_errordecay.value - t ) / cg_errordecay.value;
			if ( f > 0 && f < 1 ) {
				VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
			} else {
				cg.predictedErrorTime = 0;
			}
		}

		CG_CalcTurretViewValues();

		if ( cg.renderingThirdPerson ) {
			// back away from character
			CG_OffsetThirdPersonView();
		} else {
			// offset for local bobbing and kicks
			CG_OffsetFirstPersonView();
		}
	}
	// position eye relative to origin
	AngleVectors( cg.refdefViewAngles, cg.refdef.viewaxis[0], right, cg.refdef.viewaxis[2] );
	VectorSubtract( vec3_origin, right, cg.refdef.viewaxis[1] );

	// field of view
	return CG_CalcFov();
}

#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/*
===============
CG_DrawSkyBoxPortal

Configstring 10 (CS_SKYBOXORG) is the skybox portal description:
"x y z fov fogstate [fog0 fog1 fog2 fognear fogfar]".
===============
*/
static void CG_DrawSkyBoxPortal( void ) {
	char        *cstr;
	char        *token;
	rfloat      fov;
	refdef_t    backupRefdef;

	cstr = (char *)CG_ConfigString( CS_SKYBOXORG );
	if ( !cstr || !strlen( cstr ) ) {
		return;
	}

	backupRefdef = cg.refdef;

	if ( cg_skybox.integer ) {
		token = Com_ParseOnLine( &cstr );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		cg.refdef.vieworg[0] = (float)atof( token );

		token = Com_ParseOnLine( &cstr );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		cg.refdef.vieworg[1] = (float)atof( token );

		token = Com_ParseOnLine( &cstr );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		cg.refdef.vieworg[2] = (float)atof( token );

		token = Com_ParseOnLine( &cstr );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		atoi( token );          // the portal fov, unused

		token = Com_ParseOnLine( &cstr );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog state\n" );
		} else if ( atoi( token ) ) {
			vec4_t fogColor;
			int fogStart, fogEnd;

			token = Com_ParseOnLine( &cstr );
			if ( !token || !token[0] ) {
				CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[0]\n" );
			}
			fogColor[0] = (float)atof( token );

			token = Com_ParseOnLine( &cstr );
			if ( !token || !token[0] ) {
				CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[1]\n" );
			}
			fogColor[1] = (float)atof( token );

			token = Com_ParseOnLine( &cstr );
			if ( !token || !token[0] ) {
				CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[2]\n" );
			}
			fogColor[2] = (float)atof( token );

			token = Com_ParseOnLine( &cstr );
			if ( !token || !token[0] ) {
				fogStart = 0;
			} else {
				fogStart = atoi( token );
			}

			token = Com_ParseOnLine( &cstr );
			if ( !token || !token[0] ) {
				fogEnd = 0;
			} else {
				fogEnd = atoi( token );
			}

			trap_R_SetFog( FOG_PORTALVIEW, fogStart, fogEnd, fogColor[0], fogColor[1], fogColor[2], 1.1f );
			skyPortalFogSet = 1;
		} else if ( !skyPortalFogSet ) {
			trap_R_SetFog( FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0 );
			skyPortalFogSet = 1;
		}

		fov = CG_GetViewFov();
		cg.refdef.fov_x = (float)fov;
		cg.refdef.rdflags |= ( RDF_SKYBOXPORTAL | RDF_DRAWSKYBOX );
		cg.refdef.fov_y = (float)( FCAST( atan2( cg.refdef.height,
												 cg.refdef.width / tan( fov * ( M_PI / 360.0f ) ) ) )
								   * ( 360.0f / M_PI ) );
	} else {
		cg.refdef.rdflags &= ~RDF_DRAWSKYBOX;
		cg.refdef.rdflags |= RDF_SKYBOXPORTAL;
	}

	cg.refdef.time = cg.time;
	trap_R_RenderScene( &cg.refdef );
	cg.refdef = backupRefdef;
}

/*
===============
updateTestFX

Respawns the /fxTest effect every cg.fxTestRestart milliseconds.
===============
*/
static void updateTestFX( void ) {
	if ( cg.fxTestRestart < 1 ) {
		return;
	}
	if ( cg.time <= cg.fxTestRestart + cg.fxTestTime ) {
		return;
	}

	trap_FX_PlaySimpleEffectID( trap_FX_RegisterEffect( cg.fxTestName ), cg.fxTestPos );
	cg.fxTestTime = cg.time;
}

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
void CG_DrawActiveFrame( int serverTime, int stereoView, int demoPlayback,
						 int cubemapShot, int cubemapSize ) {
	int         i;
	int         inwater;
	float       sensitivityScale;

	if ( stereoView != 2 ) {
		cg.oldTime = cg.time;
	}

	cg.demoPlayback = demoPlayback;
	cg.time = serverTime;
	bgs_time = serverTime;
	cg.cubemapShot = cubemapShot;
	cg.cubemapSize = cubemapSize;

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != 2 ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
			cg.oldTime = cg.time;
		}
		CG_AddLagometerFrameInfo();
	}
	bgs_frametime = cg.frametime;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] ) {
		CG_DrawInformation( 0 );
		return;
	}

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation( 0 );
		return;
	}

	// the connection screen stays up until the first command we sent has been
	// acknowledged
	if ( cl_serverloadmap.string[0] && cl_serverloadgametype.string[0] ) {
		int         cmdNum;
		usercmd_t   cmd;

		cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
		trap_GetUserCmd( cmdNum, &cmd );
		if ( cmd.serverTime > cg.snap->ps.commandTime && cmd.serverTime <= cg.time ) {
			CG_DrawInformation( 1 );
			return;
		}
	}

	if ( cl_serverloadwaiting.integer ) {
		trap_Cvar_Set( "cl_serverloadwaiting", "0" );
	}

	// if we have been told not to render, don't
	if ( cg_norender.integer ) {
		return;
	}

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// pull the shellshock the server picked for us into cg.shellshock
	if ( !cg.snap->ps.shellshockIndex ) {
		cg.shellshock.parms = &cgs.shellshockParms[0];
		cg.shellshock.startTime = cg.shellshock.forcedStartTime;
		cg.shellshock.duration = cg.shellshock.forcedDuration;
	} else {
		cg.shellshock.parms = &cgs.shellshockParms[ cg.snap->ps.shellshockIndex ];
		cg.shellshock.startTime = cg.snap->ps.shellshockTime;
		cg.shellshock.duration = cg.snap->ps.shellshockDuration;
	}
	CG_UpdateShellShock( cg.shellshock.startTime, cg.shellshock.parms, cg.shellshock.duration );

	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer || cg.nextSnap->ps.pm_type >= PM_DEAD_OR_SPECTATING;

	if ( cg.nextSnap->ps.pm_flags & PMF_TURRET ) {
		CG_KickAngles();
	} else {
		VectorClear( cg.kick_angles );
		VectorClear( cg.kick_origin );
	}

	// build cg.refdef
	inwater = CG_CalcViewValues();

	if ( !cg.cubemapShot ) {
		CG_ShakeCamera();
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		CG_PerturbCamera();
	}

	// build the render lists
	CG_DrawSkyBoxPortal();

	trap_FX_AdjustTime( cg.time );
	trap_FX_AdjustCamera( &cg.refdef );

	updateTestFX();

	CG_AddPacketEntities();         // adter calcViewValues, so predicted player state is correct
	CG_AddMarks();                  // add wall marks
	CG_AddLocalEntities();
	CG_AddViewWeapon( &cg.predictedPlayerState );
	CG_PlayBufferedVoiceChats();

	trap_FX_AddScheduledEffects();

	// finish up the rest of the refdef
	cg.refdef.time = cg.time;

	if ( cg_weaponSelect.integer < 0 || cg_weaponSelect.integer > bg_numWeapons ) {
		CG_Printf( "WARNING: Invalid cg_weaponSelect setting %i (out of range 0 - %i)\n",
				   cg_weaponSelect.integer, bg_numWeapons );
		for ( i = 1 ; i <= 5 ; i++ ) {
			if ( cg.predictedPlayerState.weaponslots[i] ) {
				trap_Cvar_Set( "cg_weaponSelect",
							   va( "%i", (signed char)cg.predictedPlayerState.weaponslots[i] ) );

			}
		}
		trap_Cvar_Set( "cg_weaponSelect", "0" );
	}

	// let the client system know what our weapon and zoom settings are
	sensitivityScale = cg.zoomSensitivity;
	if ( cg.shellshock.mouseSensitivityScale != 0.0f ) {
		sensitivityScale = cg.shellshock.mouseSensitivityScale * sensitivityScale;
	}
	trap_SetUserCmdAimValues( cg.kick_origin );
	trap_SetUserCmdValue( cg_weaponSelect.integer, sensitivityScale );

	// let client system know our predicted origin
	trap_SetClientLerpOrigin( cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2] );

	// actually issue the rendering calls
	CG_DrawActive( stereoView );

	// update audio positions
	trap_MSS_SetListener( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis );
	trap_MSS_UpdateLoopingSounds();

	if ( cg_stats.integer ) {
		CG_Printf( "cg.clientFrame:%i\n", cg.clientFrame );
	}
}
