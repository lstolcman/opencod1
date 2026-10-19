// g_debug_mp.c -- debug drawing helpers built on trap_AddDebugLine

#include <math.h>

#include "g_local.h"

// the 12 edges of a box, as index pairs into the 8 corners G_DebugBox builds
static const int boxEdges[12][2] = {
	{ 0, 1 }, { 0, 2 }, { 0, 4 }, { 1, 3 },
	{ 1, 5 }, { 2, 3 }, { 2, 6 }, { 3, 7 },
	{ 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 }
};

void G_DebugLine( const vec3_t start, const vec3_t end, const vec3_t color, int depthTest, int duration ) {
	trap_AddDebugLine( start, end, color, depthTest, duration );
}

void G_DebugBox( const vec3_t mins, const vec3_t maxs, const vec3_t color, int depthTest, int duration ) {
	vec3_t corners[8];
	int i, j;

	for ( i = 0 ; i < 8 ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			corners[i][j] = ( i & ( 1 << j ) ) ? maxs[j] : mins[j];
		}
	}

	for ( i = 0 ; i < sizeof( boxEdges ) / sizeof( boxEdges[0] ) ; i++ ) {
		trap_AddDebugLine( corners[boxEdges[i][0]], corners[boxEdges[i][1]], color, depthTest, duration );
	}
}

/*
==================
G_DebugCircle

flat draws the circle on the ground; otherwise it is turned to face client 0's
eye, which is all a listen server ever needs.
==================
*/
void G_DebugCircle( const vec3_t org, qboolean flat, float radius, const vec3_t color, int depthTest, int duration ) {
	vec3_t dir;
	vec3_t viewOrigin;

	if ( flat ) {
		dir[0] = 0;
		dir[1] = 0;
		dir[2] = 1;
	} else {
		VectorCopy( level.clients->ps.origin, viewOrigin );
		viewOrigin[2] += level.clients->ps.viewHeightCurrent;
		VectorSubtract( org, viewOrigin, dir );
	}

	G_DebugCircleEx( dir, org, radius, color, depthTest, duration );
}

void G_DebugCircleEx( const vec3_t dir, const vec3_t org, float radius, const vec3_t color, int depthTest, int duration ) {
	vec3_t axis, vRight, vUp;
	vec3_t points[16];
	float angle, s, c;
	float *sinOut, *cosOut;
	int i;

	// keep: the dead store and the inline fsincos match original codegen
	sinOut = &s;
	VectorNormalize2( dir, axis );
	PerpendicularVector( vRight, axis );
	CrossProduct( axis, vRight, vUp );

	for ( i = 0 ; i < sizeof( points ) / sizeof( points[0] ) ; i++ ) {
		angle = i * 0.39269909f;    // 2*M_PI/16
		cosOut = &c;
		sinOut = &s;
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
		s *= radius;
		c *= radius;
		VectorScale( vUp, s, points[i] );
		VectorMA( points[i], c, vRight, points[i] );
		VectorAdd( points[i], org, points[i] );
	}

	for ( i = 0 ; i < sizeof( points ) / sizeof( points[0] ) ; i++ ) {
		trap_AddDebugLine( points[i], points[( i + 1 ) & 15], color, depthTest, duration );
	}
}

/*
==================
G_DebugArc

16 points around org in the xy plane, from startAngle to endAngle, joined by 15
segments -- unlike G_DebugCircleEx the ring is not closed.
==================
*/
void G_DebugArc( const vec3_t org, float radius, float startAngle, float endAngle, const vec3_t color, int depthTest, int duration ) {
	vec3_t points[16];
	float step, angle, s, c;
	float *sinOut, *cosOut;
	int i;

	step = ( endAngle - startAngle ) * ( 1.0f / 15 );
	if ( step < 0 ) {
		startAngle -= 360.0f;
		step = ( endAngle - startAngle ) * ( 1.0f / 15 );
	}

	for ( i = 0 ; i < sizeof( points ) / sizeof( points[0] ) ; i++ ) {
		// keep: the redundant cast and the inline fsincos match original codegen
		angle = (float)( ( ( i * step + startAngle ) * (float)M_PI ) * ( 1.0f / 180.0f ) );
		cosOut = &c;
		sinOut = &s;
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
		points[i][0] = c * radius + org[0];
		points[i][1] = s * radius + org[1];
		points[i][2] = org[2];
	}

	for ( i = 0 ; i < 15 ; i++ ) {
		G_DebugLine( points[i], points[i + 1], color, depthTest, duration );
	}
}

/*
==================
Q_rint

Round to the nearest integer.  The bias is 0.5 - 2^-30, so x.0 stays x
and x.5 rounds up.
==================
*/
int Q_rint( float f ) {
	double bias = 0.4999999990686774;
	int i;

	// keep: inline fistp matches original codegen
	__asm {
		fld f
		fadd bias
		fistp i
	}
	return i;
}
