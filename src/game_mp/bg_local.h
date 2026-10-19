// bg_local.h -- local definitions for the bg (both games) files

#ifndef __BG_LOCAL_H__
#define __BG_LOCAL_H__

#include "bg_public.h"
#include "g_public.h"

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct pml_t
{
	vec3_t forward, right, up;
	float frametime;

	int msec;

	qboolean walking;
	qboolean groundPlane;
	trace_t groundTrace;

	float impactSpeed;

	vec3_t previous_origin;
	vec3_t previous_velocity;
	int previous_waterlevel;

	const struct weaponInfo_t *weaponInfo;
} pml_t;
COD1_ASSERT_SIZE( pml_t, 136 );

extern pmove_t  *pm;
extern pml_t pml;

extern int c_pmove;

//
// bg_pmove.c
//
float       PM_FloatAbs( float value );
int         PM_FloatIsNegative( float value );
int         PM_FloatSign( float value );

void        PM_AddEvent( int newEvent );
void        PM_AddTouchEnt( int entityNum );
void        PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out, float overbounce );
int         PM_GetViewHeightLerpTime( const playerState_t *ps, int viewheight,
									  int viewHeightLerpDown );
void        PM_FootstepEvent( int oldBobCycle, int newBobCycle, qboolean footstep );
qboolean    PM_ShouldMakeFootsteps( void );
void        PM_SetProneMovementOverride( void );

//
// bg_slidemove.c
//
qboolean    PM_SlideMove( qboolean gravity );
void        PM_StepSlideMove( qboolean gravity );

//
// bg_weapon.c
//
void        PM_AdjustAimSpreadScale( void );
void        PM_UpdateAimDownSightFlag( void );
qboolean    PM_InteruptWeaponWithProneMove( void );
void        PM_Weapon( void );

#endif /* __BG_LOCAL_H__ */
