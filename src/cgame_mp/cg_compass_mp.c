// cg_compass.c -- draws the friendly blips on the compass
// the friendly table is refreshed from the next snapshot, then each recent
// entry is drawn around the compass rose

#include <math.h>

#include "cg_local.h"

#ifndef YAW
#define PITCH   0
#define YAW     1
#define ROLL    2
#endif

#ifndef TEAM_SPECTATOR
#define TEAM_FREE       0
#define TEAM_SPECTATOR  3
#endif

#define ET_PLAYER       1
#define EF_DEAD         1
#define EF_TALK         0x80000
#define EF_TALK_LOCAL   0x100000    // EF_TALK as seen in the local playerState

#define ANGLE2SHORT( x )    ( (int)( (x) * ( 65536.0f / 360.0f ) ) & 65535 )
#define SHORT2ANGLE( x )    ( (float)( x ) * ( 360.0f / 65536.0f ) )

// keep: defined here, not in com_math.c, to match original codegen
#ifdef COD1_VC7   /* KEEP-GUARD: modern MSVC emits no external copy of an __inline function */
__inline
#endif
void SinCos( float radians, float *sinOut, float *cosOut ) {
	__asm {
		push    eax
		push    ecx
		fld     radians
		fsincos
		mov     ecx, cosOut
		mov     eax, sinOut
		fstp    dword ptr [ecx]
		fstp    dword ptr [eax]
		pop     ecx
		pop     eax
	}
}

// keep: built with improved float consistency, as the original
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif

/*
=================
CG_DrawCompassFriendlies

Ownerdraw for the friendly compass blips; the shader is unused
=================
*/
void CG_DrawCompassFriendlies( float *rect, qhandle_t shader, const float *color ) {
	centity_t *cent;
	compassFriendly_t *friendly;
	vec4_t drawColor;
	vec3_t pos, dir;
	float centerX, centerY;
	float x, y, w, h, radius, dist, angle;
	float s, c;
	int team;
	int i, num;
	int flash;

	if ( !bg_clientinfo[cg.nextSnap->ps.clientNum].infoValid ) {
		return;
	}
	team = bg_clientinfo[cg.nextSnap->ps.clientNum].team;
	if ( team == TEAM_SPECTATOR || team == TEAM_FREE ) {
		return;
	}

	//
	// refresh every friendly the snapshot can see
	//
	for ( i = 0; i < cg.nextSnap->numEntities; i++ ) {
		num = cg.nextSnap->entities[i].number;
		cent = &cg_entities[num];
		if ( cent->nextState.eType != ET_PLAYER ) {
			continue;
		}
		if ( cent->nextState.eFlags & EF_DEAD ) {
			continue;
		}
		if ( num < 0 ) {
			continue;
		}
		if ( !bg_clientinfo[num].infoValid ) {
			continue;
		}
		if ( bg_clientinfo[num].team != team ) {
			continue;
		}

		cg.compassFriendlies[num].time = cg.time;
		cg.compassFriendlies[num].origin[0] = cent->lerpOrigin[0];
		cg.compassFriendlies[num].origin[1] = cent->lerpOrigin[1];
		cg.compassFriendlies[num].origin[2] = cent->lerpAngles[YAW];

		if ( ( cent->nextState.eFlags & EF_TALK ) && cg.compassFriendlies[num].flashEndTime <= cg.time ) {
			cg.compassFriendlies[num].flashEndTime = cg.time + 3000;
		}
	}

	//
	// and the one the server packed into the local player state: client number
	// in the low 6 bits, then two 9-bit coordinates, then a yaw byte
	//
	if ( cg.nextSnap->ps.iCompassFriendInfo ) {
		num = cg.nextSnap->ps.iCompassFriendInfo & 0x3F;
		cg.compassFriendlies[num].time = cg.time;

		dir[0] = (float)( 4 * ( ( cg.nextSnap->ps.iCompassFriendInfo >> 6 ) & 0x1FF ) - 1020 );
		dir[1] = (float)( 4 * ( ( cg.nextSnap->ps.iCompassFriendInfo >> 15 ) & 0x1FF ) - 1020 );

		if ( dir[0] == 1024.0f || dir[0] == -1020.0f || dir[1] == 1024.0f || dir[1] == -1020.0f ) {
			// saturated: the pair is a direction, not an offset
			VectorNormalize2D( dir );
			cg.compassFriendlies[num].origin[0] = dir[0];
			cg.compassFriendlies[num].origin[1] = dir[1];
			cg.compassFriendlies[num].origin[2] = dir[2];
		} else {
			VectorCopy( cg.nextSnap->ps.origin, pos );
			pos[2] += cg.nextSnap->ps.viewHeightCurrent;
			AddLeanToPosition( pos, cg.nextSnap->ps.viewangles[YAW], cg.nextSnap->ps.leanf, 16.0f, 20.0f );
			cg.compassFriendlies[num].origin[0] = pos[0] + dir[0];
			cg.compassFriendlies[num].origin[1] = pos[1] + dir[1];
		}

		cg.compassFriendlies[num].origin[2] = ( cg.nextSnap->ps.iCompassFriendInfo >> 24 ) * ( 360.0f / 256.0f );

		if ( ( cg.nextSnap->ps.eFlags & EF_TALK_LOCAL ) && cg.compassFriendlies[num].flashEndTime <= cg.time ) {
			cg.compassFriendlies[num].flashEndTime = cg.time + 3000;
		}
	}

	CG_UpdateCompPointerOrientation();

	centerX = cg_hudCompassSize.value * rect[2] * 0.5f + rect[0];
	// keep: FCAST matches original codegen
	centerY = FCAST( cg_hudCompassSize.value * rect[3] * 0.5f ) + rect[1]
			  - ( cg_hudCompassSize.value - 1.0f ) * 160.0f;

	drawColor[0] = color[0];
	drawColor[1] = color[1];
	drawColor[2] = color[2];
	drawColor[3] = color[3];

	// keep: times through friendly, origin by index, matches original codegen
	for ( friendly = cg.compassFriendlies, i = 0; i < MAX_CLIENTS; i++, friendly++ ) {
		if ( friendly->time > cg.time ) {
			friendly->time = 0;
		}
		if ( friendly->time < cg.time - 800 ) {
			continue;
		}
		if ( cg.nextSnap->ps.clientNum == i ) {
			continue;
		}

		if ( fabs( cg.compassFriendlies[i].origin[0] ) <= 1.0 && fabs( cg.compassFriendlies[i].origin[1] ) <= 1.0 ) {
			// a bare direction: pin it to the rim at half fade
			angle = SHORT2ANGLE( ANGLE2SHORT( vectoyaw( cg.compassFriendlies[i].origin ) - cg.compassPointerAngle ) );
			dist = cg_hudCompassMaxRange.value;
			drawColor[3] = ( 1.0f - cg_hudObjectiveMinAlpha.value ) * 0.5f + cg_hudObjectiveMinAlpha.value;
		} else {
			// a world position: bearing and range from the view origin
			dir[0] = cg.compassFriendlies[i].origin[0] - cg.refdef.vieworg[0];
			dir[1] = cg.compassFriendlies[i].origin[1] - cg.refdef.vieworg[1];
			angle = SHORT2ANGLE( ANGLE2SHORT( vectoyaw( dir ) - cg.compassPointerAngle ) );

			// keep: double FCAST matches original codegen
			dist = FCAST( (float)sqrt( dir[1] * dir[1] + dir[0] * dir[0] ) );

			// fade with distance
			drawColor[3] = dist;
			if ( dist > cg_hudObjectiveMaxRange.value ) {
				drawColor[3] = cg_hudObjectiveMaxRange.value;
			} else if ( dist < cg_hudCompassMaxRange.value ) {
				drawColor[3] = cg_hudCompassMaxRange.value;
			}
			drawColor[3] -= cg_hudCompassMaxRange.value;
			drawColor[3] /= cg_hudObjectiveMaxRange.value - cg_hudCompassMaxRange.value;
			drawColor[3] = ( cg_hudObjectiveMinAlpha.value - 1.0f ) * drawColor[3] + 1.0f;

			if ( dist > cg_hudCompassMaxRange.value ) {
				dist = cg_hudCompassMaxRange.value;
			} else if ( dist < cg_hudCompassMinRange.value ) {
				dist = cg_hudCompassMinRange.value;
			}
		}

		radius = ( ( dist - cg_hudCompassMinRange.value )
				   / ( cg_hudCompassMaxRange.value - cg_hudCompassMinRange.value )
				   * ( 1.0f - cg_hudCompassMinRadius.value )
				   + cg_hudCompassMinRadius.value )
				 * cg_hudCompassSize.value * 43.75f;

		angle = angle * (float)M_PI / 180.0f;
		SinCos( angle, &s, &c );

		w = cg_hudCompassSize.value * 10.0f;
		h = cg_hudCompassSize.value * 10.0f;
		x = centerX - w * 0.5f - s * radius;
		y = centerY - h * 0.5f - c * radius;

		// the icon's own yaw
		angle = SHORT2ANGLE( ANGLE2SHORT( cg.refdefViewAngles[YAW] - cg.compassFriendlies[i].origin[2] ) );

		if ( friendly->flashEndTime > cg.time ) {
			flash = ( ( friendly->flashEndTime - cg.time ) % 500 ) >= 250;
		} else {
			flash = 0;
		}

		trap_R_SetColor( drawColor );

		if ( flash == 1 ) {
			CG_DrawPic( x, y, w, h, cgs.media.objectiveFriendlyChat );
		} else {
			// keep: indexes the objectiveFriendly / objectiveFriendlyChat pair
			CG_DrawRotatedPic( x, y, w, h, angle,
							   ( &cgs.media.objectiveFriendly )[flash] );
		}
	}

	trap_R_SetColor( NULL );
}

#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif
