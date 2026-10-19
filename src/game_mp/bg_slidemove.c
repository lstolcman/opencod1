// bg_slidemove.c -- part of bg_pmove functionality

#include "bg_local.h"

#include <math.h>
#include <stdlib.h>

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/

void Com_Printf( const char *fmt, ... );
float VectorNormalize2( const vec3_t v, vec3_t out );

#define MAX_CLIP_PLANES     8
#define OVERCLIP            1.001f
#define STEPSIZE            18
#define PRONE_STEPSIZE      10

/*
==================
Q_ftol

Rounds to the nearest integer, with halves going up
==================
*/
int Q_ftol( float f ) {
	double  bias = 9.313225746154785e-10;
	int     i;

	__asm fld f
	__asm fadd bias
	__asm fistp i

	return i;
}

/*
==================
PM_VerifyPronePosition

Puts a prone player back where he started if he no longer fits
==================
*/
static qboolean PM_VerifyPronePosition( const vec3_t oldOrigin, const vec3_t oldVelocity ) {
	qboolean valid;

	if ( pm->ps->pm_flags & PMF_PRONE ) {
		valid = BG_CheckProne( pm->ps->clientNum, pm->ps->origin, pm->ps->maxs[0], 30.0f,
							   pm->ps->proneDirection,
							   &pm->ps->fTorsoHeight, &pm->ps->fTorsoPitch, &pm->ps->fWaistPitch,
							   qtrue, qtrue, NULL, pm->trace3, pm->trace2, qfalse );
		if ( !valid ) {
			VectorCopy( oldOrigin, pm->ps->origin );
			VectorCopy( oldVelocity, pm->ps->velocity );
		}
		return valid;
	}

	return qtrue;
}

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
qboolean PM_SlideMove( qboolean gravity ) {
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity;
	vec3_t clipVelocity;
	int i, j, k;
	trace_t trace;
	vec3_t end;
	float time_left;
	float into;
	vec3_t endVelocity;
	vec3_t endClipVelocity;

	numbumps = 4;

	VectorCopy( pm->ps->velocity, primal_velocity );

	if ( gravity ) {
		VectorCopy( pm->ps->velocity, endVelocity );
		endVelocity[2] -= (float)pm->ps->gravity * pml.frametime;
		pm->ps->velocity[2] = ( pm->ps->velocity[2] + endVelocity[2] ) * 0.5f;
		primal_velocity[2] = endVelocity[2];
		if ( pml.groundPlane ) {
			// slide along the ground plane
			PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.normal,
							 pm->ps->velocity, OVERCLIP );
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) {
		VectorCopy( pml.groundTrace.normal, planes[0] );
		numplanes = 1;
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[numplanes] );
	numplanes++;

	for ( bumpcount = 0 ; bumpcount < numbumps ; bumpcount++ ) {

		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, end,
				   pm->ps->clientNum, pm->tracemask );

		if ( trace.allsolid ) {
			// entity is completely trapped in another solid
			pm->ps->velocity[2] = 0;    // don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		if ( trace.fraction > 0 ) {
			// actually covered some distance
			VectorCopy( trace.endpos, pm->ps->origin );
		}

		if ( trace.fraction == 1 ) {
			break;      // moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// this shouldn't really happen
			if ( pm->debugLevel >= 2 ) {
				Com_Printf( "%i:MAX_CLIP_PLANES\n", c_pmove );
			}
			VectorClear( pm->ps->velocity );
			return qtrue;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0 ; i < numplanes ; i++ ) {
			if ( DotProduct( trace.normal, planes[i] ) > 0.99900001f ) {
				if ( pm->debugLevel >= 2 ) {
					Com_Printf( "%i:recollided with plane normal (%.2f, %.2f, %.2f)\n",
								c_pmove, trace.normal[0], trace.normal[1], trace.normal[2] );
				}
				VectorAdd( trace.normal, pm->ps->velocity, pm->ps->velocity );
				break;
			}
		}
		if ( i < numplanes ) {
			continue;
		}
		VectorCopy( trace.normal, planes[numplanes] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0 ; i < numplanes ; i++ ) {
			into = (float)DotProduct( pm->ps->velocity, planes[i] );
			if ( into >= 0.1 ) {
				continue;       // move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed ) {
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity( pm->ps->velocity, planes[i], clipVelocity, OVERCLIP );

			// slide along the plane
			PM_ClipVelocity( endVelocity, planes[i], endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0 ; j < numplanes ; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( DotProduct( clipVelocity, planes[j] ) >= 0.1 ) {
					continue;       // move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[j], clipVelocity, OVERCLIP );
				PM_ClipVelocity( endClipVelocity, planes[j], endClipVelocity, OVERCLIP );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct( planes[i], planes[j], dir );
				VectorNormalize( dir );
				d = (float)DotProduct( dir, pm->ps->velocity );
				VectorScale( dir, d, clipVelocity );

				d = (float)DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the new move enters
				for ( k = 0 ; k < numplanes ; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( DotProduct( clipVelocity, planes[k] ) >= 0.1 ) {
						continue;       // move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					VectorClear( pm->ps->velocity );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, pm->ps->velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravity ) {
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer
	if ( pm->ps->pm_time ) {
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return ( bumpcount != 0 );
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove( qboolean gravity ) {
	vec3_t start_o, start_v;
	vec3_t down_o, down_v;
	vec3_t up, down;
	float delta[2];
	trace_t trace;
	float stepSize;
	float stepUp;
	qboolean jumpStep;
	qboolean stepDown;
	qboolean slid;
	int step;
	int magnitude;
	int bobAdd;
	int old;
	rfloat frac;        // keep: rfloat temporaries match original codegen
	rfloat remaining;
	rfloat moved[2];
	float jumpVel;
	float stepDelta;

	stepUp = 0;
	stepDown = qfalse;
	jumpStep = qfalse;

	if ( pml.groundPlane && !( pm->ps->pm_flags & PMF_LADDER ) ) {
		stepDown = qtrue;
	}

	VectorCopy( pm->ps->origin, start_o );
	VectorCopy( pm->ps->velocity, start_v );

	slid = PM_SlideMove( gravity );

	if ( pm->ps->pm_flags & PMF_PRONE ) {
		stepSize = PRONE_STEPSIZE;
	} else {
		stepSize = STEPSIZE;
	}

	if ( slid
		 && ( pm->ps->fJumpOriginZ < -0.001f || pm->ps->fJumpOriginZ > 0.001f )
		 && pm->ps->groundEntityNum == ENTITYNUM_NONE
		 && start_o[2] < pm->ps->fJumpOriginZ ) {
		// a jump that got stopped by a ledge: step up onto it, but never
		// higher than the jump would have carried us anyway
		stepSize = STEPSIZE;
		if ( start_o[2] + STEPSIZE > pm->ps->fJumpOriginZ ) {
			// keep: assign-and-compare matches original codegen
			if ( ( stepSize = (rfloat)pm->ps->fJumpOriginZ - start_o[2] ) < 1.0f ) {
				return;
			}
		}
		jumpStep = qtrue;
	} else if ( pm->ps->groundEntityNum == ENTITYNUM_NONE
				&& ( !( pm->ps->pm_flags & PMF_LADDER ) || !( pm->ps->velocity[2] > 0 ) ) ) {
		// !( > 0 ), not <= 0: NaN returns too
		return;
	}

	VectorCopy( pm->ps->origin, down_o );
	VectorCopy( pm->ps->velocity, down_v );

	delta[0] = down_o[0] - start_o[0];
	delta[1] = down_o[1] - start_o[1];

	if ( slid ) {
		// how much headroom is there really?
		up[0] = start_o[0];
		up[1] = start_o[1];
		up[2] = stepSize + start_o[2] + 1.0f;
		pm->trace( &trace, start_o, pm->mins, pm->maxs, up,
				   pm->ps->clientNum, pm->tracemask );
		if ( ( stepUp = ( (rfloat)stepSize + 1.0f ) * trace.fraction - 1.0f ) < 1.0f ) {
			if ( pm->debugLevel ) {
				Com_Printf( "%i:not enough step room\n", c_pmove );
			}
			stepUp = 0;
		} else {
			pm->ps->origin[0] = up[0];
			pm->ps->origin[1] = up[1];
			pm->ps->origin[2] = FCAST( start_o[2] + stepUp );   // keep: FCAST matches original codegen
			VectorCopy( start_v, pm->ps->velocity );

			if ( pm->debugLevel && jumpStep ) {
				Com_Printf( "%i:jump step to:%.2f jump peak at:%.2f\n", c_pmove,
							pm->ps->origin[2], pm->ps->fJumpOriginZ );
			}

			PM_SlideMove( gravity );
		}
	}

	if ( stepDown || stepUp != 0 ) {
		// push down the final amount
		down[0] = pm->ps->origin[0];
		down[1] = pm->ps->origin[1];
		down[2] = pm->ps->origin[2] - stepUp;
		if ( stepDown ) {
			down[2] -= stepSize * 0.5f;
		}
		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, down,
				   pm->ps->clientNum, pm->tracemask );
		if ( trace.entityNum < MAX_CLIENTS ) {
			// never step onto another player
			VectorCopy( down_o, pm->ps->origin );
			VectorCopy( down_v, pm->ps->velocity );
			return;
		}
		if ( trace.fraction < 1.0f ) {
			VectorCopy( trace.endpos, pm->ps->origin );
			PM_ClipVelocity( pm->ps->velocity, trace.normal, pm->ps->velocity, OVERCLIP );
		} else if ( stepUp != 0 ) {
			pm->ps->origin[2] -= stepUp;
		}
	}

	// only keep the step if it actually got us further along the direction the
	// unstepped move was already going, and if a jump step did not overshoot
	moved[0] = pm->ps->origin[0] - start_o[0];
	moved[1] = pm->ps->origin[1] - start_o[1];
	if ( moved[0] * pm->ps->velocity[0] + moved[1] * pm->ps->velocity[1]
		 <= delta[0] * pm->ps->velocity[0] + delta[1] * pm->ps->velocity[1] + 0.001f
		 || ( jumpStep && pm->ps->origin[2] >= pm->ps->fJumpOriginZ ) ) {
		VectorCopy( down_o, pm->ps->origin );
		VectorCopy( down_v, pm->ps->velocity );

		if ( pm->debugLevel > 1 ) {
			if ( jumpStep ) {
				Com_Printf( "%i:didn't use jump step results because it went too high\n",
							c_pmove );
			} else {
				Com_Printf( "%i:didn't use step results\n", c_pmove );
			}
		}

		if ( stepDown ) {
			down[0] = pm->ps->origin[0];
			down[1] = pm->ps->origin[1];
			down[2] = pm->ps->origin[2];     // keep: two statements match original codegen
			down[2] -= stepSize * 0.5f;
			pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, down,
					   pm->ps->clientNum, pm->tracemask );
			if ( trace.fraction < 1.0f ) {
				VectorCopy( trace.endpos, pm->ps->origin );
				PM_ClipVelocity( pm->ps->velocity, trace.normal, pm->ps->velocity, OVERCLIP );
				if ( pm->debugLevel > 1 ) {
					Com_Printf( "%i:did down step after not using step results\n", c_pmove );
				}
			}
		}
	}

	if ( jumpStep ) {
		// the step ate part of the jump: take the same amount off the upward
		// velocity so the peak stays where it was
		if ( pm->ps->origin[2] - down_o[2] > 0 ) {
			remaining = (rfloat)pm->ps->fJumpOriginZ - pm->ps->origin[2];
			if ( remaining < 0.1f ) {
				pm->ps->velocity[2] = 0;
			} else {
				jumpVel = (float)sqrt( pm->ps->gravity * ( remaining + remaining ) );
				if ( pm->ps->velocity[2] > jumpVel ) {
					if ( pm->debugLevel ) {
						Com_Printf( "%i:adjusted jump vel: %.1f -> %.1f\n", c_pmove,
									pm->ps->velocity[2], jumpVel );
					}
					pm->ps->velocity[2] = jumpVel;
				}
			}
		}
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}

	if ( !PM_VerifyPronePosition( start_o, start_v ) ) {
		return;
	}

	if ( !( fabs( (double)( pm->ps->origin[2] - down_o[2] ) ) > 0.5 ) ) {
		return;
	}

	step = Q_ftol( pm->ps->origin[2] - down_o[2] );
	if ( !step ) {
		return;
	}

	if ( pm->debugLevel ) {
		if ( jumpStep ) {
			Com_Printf( "%i:jump step %2i\n", c_pmove, step );
		} else {
			Com_Printf( "%i:stepped %2i\n", c_pmove, step );
		}
	}

	if ( step < -16 ) {
		step = -16;
	} else if ( step > 24 ) {
		step = 24;
	}

	// keep: biasing step in place matches original codegen
	step += 128;
	BG_AddPredictableEventToPlayerstate( EV_STEP_VIEW, step, pm->ps );
	step -= 128;

	// bleed off speed in proportion to how much of the step budget was used
	stepDelta = pm->ps->origin[2] - start_o[2];
	frac = ( 1.0f - (rfloat)PM_FloatAbs( stepDelta ) / stepSize ) * 0.80000001f + 0.19999999f;
	pm->ps->velocity[0] = frac * pm->ps->velocity[0];
	pm->ps->velocity[1] = frac * pm->ps->velocity[1];
	pm->ps->velocity[2] = frac * pm->ps->velocity[2];

	magnitude = abs( step );
	if ( magnitude <= 3 ) {
		return;
	}
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		return;
	}
	if ( !PM_ShouldMakeFootsteps() ) {
		return;
	}

	// push the bob cycle forward so a tall step lands a footstep
	bobAdd = abs( step ) / 2;   // keep: not magnitude, matches original codegen
	if ( bobAdd > 4 ) {
		bobAdd = 4;
	}
	old = pm->ps->bobCycle;
	// keep: FCAST matches original codegen
	pm->ps->bobCycle = (int)( FCAST( (float)bobAdd * 1.25f + 7.0f ) + (float)old ) & 255;
	PM_FootstepEvent( old, pm->ps->bobCycle, qtrue );
}
