// com_math.c -- stateless support routines that are included in each code module

#include "../qcommon/qcommon.h"

// keep: intrinsic forms of the math functions
#pragma intrinsic( sqrt, atan2, acos, fabs, sin, pow )

float __fsqrt( float number );
void SinCos( float radians, float *sinOut, float *cosOut );
void SinCos_double( double radians, double *sinOut, double *cosOut );
int Q_log2( int val );
float Q_acos( float c );
signed char ClampChar( int i );
signed short ClampShort( int i );

int RandomInt( int *seed );
float RandomFloat( int *seed );
float CRandomFloat( int *seed );
void gunrandom( float *outX, float *outY );

byte DirToByte( vec3_t dir );
void ByteToDir( int b, vec3_t dir );

int VectorCompare( const vec3_t v1, const vec3_t v2 );
vec_t Distance( const vec3_t p1, const vec3_t p2 );
vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 );
vec_t Distance2d( const vec3_t p1, const vec3_t p2 );
vec_t Distance2dSquared( const vec3_t p1, const vec3_t p2 );
void Perp2D( vec3_t out, const vec3_t in );
vec_t VectorNormalize2D( float *v );
vec_t Vector4Normalize( vec4_t v );
void VectorNormalizeFast( vec3_t v );
vec_t VectorNormalize2( const vec3_t v, vec3_t out );
void VectorInverse( vec3_t v );
void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out );
vec_t VectorMax( const vec3_t v );
void Vector5Add_m( const float *a, const float *b, float *out );
void Vector5Scale_m( const float *in, float scale, float *out );
void Vector3Copy_m( const vec3_t in, vec3_t out );
void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out );
void PerpendicularVector( vec3_t dst, const vec3_t src );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							  float degrees );
void RotateAroundDirection( vec3_t axis[3], float yaw );
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );

float vectoyaw( const vec3_t vec );
float vectosignedyaw( const vec3_t vec );
float vectopitch( const vec3_t vec );
float vectosignedpitch( const vec3_t vec );
void vectoangles( const vec3_t value1, vec3_t angles );
void vectoanglessigned( const vec3_t value1, vec3_t angles );

void MatrixMultiply( float in1[3][3], float in2[3][3], float out[3][3] );
void MatrixMultiply33InPlace_m( const float left[3][3], float inout[3][3] );
void MatrixMultiply34_m( const float left[3][4], const float right[3][4], float out[3][4] );
void MatrixTranspose( const float in[9], float out[9] );
float *MatrixTransformVector( const vec3_t in, float m[9], vec3_t out );
void QuatToMatrix( float quatAndMatrix[9] );
float RotationToYaw( const float v[2] );

qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c );
void ProjectPointOntoVector( vec3_t dst, const vec3_t p, const vec3_t normal );
int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, cplane_t *p );

void RotatePointByAngles_m( const vec3_t in, const vec3_t angles, vec3_t out );
void PolarToVector_m( vec3_t out, float radius, float angle );
float PitchOfVectorAlongYaw( float yawDegrees, const vec3_t vec );

#if defined( UIDLL ) || defined( GAMEDLL ) || defined( CGAMEDLL )
void Com_SRand( int seed );
float Com_RandFloatRange( float lo, float hi );
int Com_RandIntRange( int lo, int hi );
#endif

#if defined( UIDLL ) || defined( GAMEDLL ) || defined( CGAMEDLL )
// const in the dlls
const vec3_t vec3_origin = { 0.0f, 0.0f, 0.0f };
#else
vec3_t vec3_origin = { 0.0f, 0.0f, 0.0f };
#endif

unsigned ColorBytes3( float r, float g, float b ) {
	unsigned i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = 0xff;

	return i;
}

unsigned ColorBytes4( float r, float g, float b, float a ) {
	unsigned i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float max;

	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}

float NormalizeColorWhite_m( const vec3_t in, vec3_t out ) {
	float max, inv;

	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( max == 0.0f ) {
		out[0] = out[1] = out[2] = 1.0f;
		return 0.0f;
	}

	inv = 1.0f / max;
	out[0] = in[0] * inv;
	out[1] = in[1] * inv;
	out[2] = in[2] * inv;
	return max;
}

#define ANGLE_TO_SHORT      182.04445f
#define ANGLE_FROM_SHORT    ( 360.0f / 65536.0f )

float AngleMod( float a ) {
	return ANGLE_FROM_SHORT * ( (int)( a * ANGLE_TO_SHORT ) & 65535 );
}

/*
===============
LerpAngle

===============
*/
float LerpAngle( float from, float to, float frac ) {
	float a, t;

	t = to;
	if ( t - from > 180 ) {
		t -= 360;
	}
	if ( t - from < -180 ) {
		t += 360;
	}
	a = from + frac * ( t - from );

	return a;
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float AngleSubtract( float a1, float a2 ) {
	float a;

	a = a1 - a2;
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}

void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360( float angle ) {
	return ANGLE_FROM_SHORT * ( (int)( angle * ANGLE_TO_SHORT ) & 65535 );
}

/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180( float angle ) {
	float a;

	a = AngleNormalize360( angle );
	if ( a > 180.0f ) {
		a -= 360.0f;
	}
	return a;
}

float AngleNormalizePositive_m( float angle ) {
	if ( angle < 0.0f ) {
		do {
			angle += 360.0f;
		} while ( angle < 0.0f );
	} else {
		while ( angle >= 360.0f ) {
			angle -= 360.0f;
		}
	}
	return angle;
}

float AngleNormalizeSigned_m( float angle ) {
	if ( angle <= -180.0f ) {
		do {
			angle += 360.0f;
		} while ( angle <= -180.0f );
	} else {
		while ( angle > 180.0f ) {
			angle -= 360.0f;
		}
	}
	return angle;
}

/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs ) {
	int i;
	vec3_t corner;
	float a, b;

	for ( i = 0 ; i < 3 ; i++ ) {
		a = fabs( mins[i] );
		b = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	// keep: grouping matches original codegen
	return (float)sqrt( corner[0] * corner[0] + ( corner[1] * corner[1] + corner[2] * corner[2] ) );
}

void ClearBounds( vec3_t mins, vec3_t maxs ) {
	mins[0] = mins[1] = mins[2] = 262144;
	maxs[0] = maxs[1] = maxs[2] = -262144;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0] ) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1] ) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2] ) {
		maxs[2] = v[2];
	}
}

void ExpandBounds( const vec3_t mins, vec3_t outMins, const vec3_t maxs, vec3_t outMaxs ) {
	int i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( outMins[i] > mins[i] ) {
			outMins[i] = mins[i];
		}
		if ( outMaxs[i] < maxs[i] ) {
			outMaxs[i] = maxs[i];
		}
	}
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void YawVectors( float yaw, vec3_t forward, vec3_t right ) {
	float angle, s, c;

	angle = yaw * ( M_PI * 2 / 360 );
	SinCos( angle, &s, &c );

	if ( forward ) {
		forward[0] = c;
		forward[1] = s;
		forward[2] = 0;
	}
	if ( right ) {
		right[0] = s;
		right[1] = -c;
		right[2] = 0;
	}
}

void YawToAxis( float yaw, vec3_t axis[3] ) {
	vec3_t right;

	YawVectors( yaw, axis[0], right );
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
	VectorSubtract( vec3_origin, right, axis[1] );
}

/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits( cplane_t *out ) {
	byte bits;
	int j;

	// for fast box on planeside test
	bits = 0;
	for ( j = 0 ; j < 3 ; j++ ) {
		if ( out->normal[j] < 0 ) {
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}

void NormalFromPoints_m( const vec3_t a, vec3_t normal, const vec3_t c, const vec3_t d ) {
	vec3_t v1, v2;

	VectorSubtract( c, a, v1 );
	VectorNormalize( v1 );
	VectorSubtract( c, d, v2 );
	VectorNormalize( v2 );

	CrossProduct( v1, v2, normal );
	VectorNormalize( normal );
}

void ProjectPointOntoLine_m( const vec3_t point, const vec3_t linePoint, vec3_t out, const vec3_t org ) {
	vec3_t dir, rel;
	float d;

	VectorSubtract( point, org, rel );
	VectorSubtract( linePoint, org, dir );
	VectorNormalize( dir );

	d = dir[2] * rel[2] + dir[1] * rel[1] + dir[0] * rel[0];   // keep: term order
	VectorMA( org, d, dir, out );
}

void MatrixTranspose( const float in[9], float out[9] ) {
	out[0] = in[0];
	out[1] = in[3];
	out[2] = in[6];
	out[3] = in[1];
	out[4] = in[4];
	out[5] = in[7];
	out[6] = in[2];
	out[7] = in[5];
	out[8] = in[8];
}

void MatrixInverse( const float in[9], float out[9] ) {
	float det, inv;

	det = in[0] * ( in[4] * in[8] - in[7] * in[5] )
		- in[3] * ( in[1] * in[8] - in[7] * in[2] )
		+ in[6] * ( in[1] * in[5] - in[2] * in[4] );
	inv = 1.0f / det;

	out[0] =  ( in[4] * in[8] - in[7] * in[5] ) * inv;
	out[1] = -( in[1] * in[8] - in[7] * in[2] ) * inv;
	out[2] =  ( in[1] * in[5] - in[2] * in[4] ) * inv;
	out[3] = -( in[3] * in[8] - in[6] * in[5] ) * inv;
	out[4] =  ( in[0] * in[8] - in[2] * in[6] ) * inv;
	out[5] = -( in[0] * in[5] - in[2] * in[3] ) * inv;
	out[6] =  ( in[7] * in[3] - in[6] * in[4] ) * inv;
	out[7] = -( in[7] * in[0] - in[1] * in[6] ) * inv;
	out[8] =  ( in[4] * in[0] - in[1] * in[3] ) * inv;
}

void MatrixTransformVector33_m( const vec3_t in, const float m[9], vec3_t out ) {
	// keep: parameter and term order match original codegen
	out[0] = m[1] * in[1] + m[2] * in[2] + m[0] * in[0];
	out[1] = m[3] * in[0] + m[4] * in[1] + m[5] * in[2];
	out[2] = m[6] * in[0] + m[7] * in[1] + m[8] * in[2];
}

void MatrixTransformVector43( const vec3_t in, const float m[], vec3_t out ) {
#if !defined(UIDLL) && !defined(GAMEDLL) && !defined(CGAMEDLL)
	// the exe uses four-float matrix rows; dll matrices stay packed

 out[0] = m[8] * in[2] + m[4] * in[1] + in[0] * m[0] + m[12];
 out[1] = m[1] * in[0] + m[9] * in[2] + m[5] * in[1] + m[13];
 out[2] = m[2] * in[0] + m[10] * in[2] + m[6] * in[1] + m[14];

#else

	out[0] = in[1] * m[3] + in[2] * m[6] + in[0] * m[0] + m[9];   // keep: term order
	out[1] = in[0] * m[1] + in[1] * m[4] + in[2] * m[7] + m[10];
	out[2] = in[0] * m[2] + in[1] * m[5] + in[2] * m[8] + m[11];

#endif
}

void MatrixTransformVector44_m( const vec3_t in, const float m[16], vec3_t out ) {
	// keep: parameter and term order match original codegen
	out[0] = m[8] * in[2] + m[4] * in[1] + m[0] * in[0] + m[12];
	out[1] = m[1] * in[0] + m[9] * in[2] + m[5] * in[1] + m[13];
	out[2] = m[2] * in[0] + m[10] * in[2] + m[6] * in[1] + m[14];
}

#if !defined( UIDLL ) && !defined( GAMEDLL ) && !defined( CGAMEDLL )
// exe only
void MatrixTransformVector43_m(const vec3_t in,const float m[12],vec3_t out) {
 out[0]=in[1]*m[3]+in[2]*m[6]+in[0]*m[0]+m[9];
 out[1]=in[0]*m[1]+in[1]*m[4]+in[2]*m[7]+m[10];
 out[2]=in[0]*m[2]+in[1]*m[5]+in[2]*m[8]+m[11];
}
#endif

void MatrixTransformVector43InPlace_m( vec3_t v, const float m[12] ) {
	float x, y;

	x    = v[0] * m[0] + v[1] * m[3] + v[2] * m[6] + m[9];
	y    = v[0] * m[1] + v[1] * m[4] + v[2] * m[7] + m[10];
	v[2] = v[0] * m[2] + v[1] * m[5] + v[2] * m[8] + m[11];
	v[0] = x;
	v[1] = y;
}

#ifdef GAMEDLL
void MatrixInverseOrthogonal34( const float in[12], float out[12] ) {
#else
void MatrixInverseOrthogonal34( float out[12], const float in[12] ) {
#endif
	vec3_t t;

	MatrixTranspose( in, out );
	VectorSubtract( vec3_origin, in + 9, t );   // keep: subtract from vec3_origin, not negate
	MatrixTransformVector( t, out, out + 9 );
}

void QuatMultiply( const float b[4], const float a[4], float out[4] ) {
	// keep: term order matches original codegen
	out[0] = a[3] * b[0] + a[1] * b[2] + b[3] * a[0] - a[2] * b[1];
	out[1] = a[3] * b[1] - a[0] * b[2] + a[2] * b[0] + b[3] * a[1];
	out[2] = a[0] * b[1] + a[3] * b[2] - a[1] * b[0] + b[3] * a[2];
	out[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
}

void QuatConjugate( const float in[4], float out[4] ) {
	out[0] = -in[0];
	out[1] = -in[1];
	out[2] = -in[2];
	out[3] =  in[3];
}

void QuatFromPitch_m( float q[4], float degrees ) {
	q[0] = 0;
	q[2] = 0;
	SinCos( degrees * ( M_PI / 360.0f ), &q[1], &q[3] );
}

void QuatFromYaw_m( float q[4], float degrees ) {
	q[0] = 0;
	q[1] = 0;
	SinCos( degrees * ( M_PI / 360.0f ), &q[2], &q[3] );
}

void QuatFromRoll_m( float degrees, float q[4] ) {
	q[1] = 0;
	q[2] = 0;
	SinCos( degrees * ( M_PI / 360.0f ), &q[0], &q[3] );
}

float QuatSinSquaredHalfAngle_m( const float q[4] ) {
	float xx, yy, zz, len2;

	xx = q[0] * q[0];
	yy = q[1] * q[1];
	zz = q[2] * q[2];
	len2 = q[3] * q[3] + zz + yy + xx;

	if ( len2 != 0.0f ) {
		// keep: len2 holds the reciprocal; each term is stored before the sum
		len2 = 1.0f / len2;
		xx *= len2;
		yy *= len2;
		zz *= len2;
		return zz + yy + xx;
	}

	return 0.0f;
}

float SinSquaredDegrees_m( float degrees ) {
	double s = sin( degrees * ( M_PI * 2 / 360 ) );

	return (float)( s * s );
}

float QuatDeltaSinSquaredHalfAngle_m( const float a[4], const float b[4] ) {
	float conj[4], delta[4];

	QuatConjugate( a, conj );
	QuatMultiply( b, conj, delta );

	return QuatSinSquaredHalfAngle_m( delta );
}

void VectorRound_m( vec3_t v ) {
	v[0] = floor( v[0] + 0.5f );
	v[1] = floor( v[1] + 0.5f );
	v[2] = floor( v[2] + 0.5f );
}

float RoundToPrecision_m( float value, int places ) {
	double scaled, integral, frac;

	scaled = pow( 10.0, (double)places ) * value;
	frac = modf( scaled, &integral );

	if ( frac >= 0.5 ) {
		integral += 1.0;
	} else if ( frac <= -0.5 ) {
		integral -= 1.0;
	}

	return (float)( pow( 0.1, (double)places ) * integral );
}

void RotateVector2D( vec3_t v, float degrees ) {
	float angle, s, c, x;

	angle = degrees * ( M_PI * 2 / 360 );
	SinCos( angle, &s, &c );

	x = c * v[0] - s * v[1];
	v[1] = s * v[0] + c * v[1];
	v[0] = x;
}

void RotatePointAroundPoint_m( const vec3_t point, const vec3_t angles, vec3_t out,
							   const vec3_t pivot ) {
	vec3_t rel, res;

	VectorSubtract( point, pivot, rel );
	RotatePointByAngles_m( rel, angles, res );
	VectorAdd( res, pivot, out );
}

/*
** float __fsqrt( float number )
*/
// keep: optimize "p" off and __inline match the original build
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif
__inline float __fsqrt( float number ) {
	long i;
	float x2, y;

	x2 = number * 0.5f;
	y  = number;
	i  = *(long *)&y;                        // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );            // what the fuck?
	y  = *(float *)&i;
	y  = y * ( 1.5f - ( x2 * y * y ) );      // 1st iteration

	return y;
}
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif

// x87 fsincos, with no argument reduction; __inline so every caller expands it
#ifndef CGAMEDLL	// cgame has its own in cg_compass_mp.c
__inline void SinCos( float radians, float *sinOut, float *cosOut ) {
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
#endif

// SinCos with double precision arguments and results
__inline void SinCos_double( double radians, double *sinOut, double *cosOut ) {
	__asm {
		push    eax
		push    ecx
		fld     radians
		fsincos
		mov     ecx, cosOut
		mov     eax, sinOut
		fstp    qword ptr [ecx]
		fstp    qword ptr [eax]
		pop     ecx
		pop     eax
	}
}

int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val >>= 1 ) != 0 ) {
		answer++;
	}
	return answer;
}

float Q_acos( float c ) {
	float angle;

	angle = (float)acos( c );

	if ( angle > M_PI ) {
		return (float)M_PI;
	}
	if ( angle < -M_PI ) {
		return (float)M_PI;
	}
	return angle;
}

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return (signed char)i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return (signed short)i;
}

int RandomInt( int *seed ) {
	*seed = ( 69069 * *seed + 1 );
	return *seed;
}

float RandomFloat( int *seed ) {
	return ( RandomInt( seed ) & 0xffff ) / (float)0x10000;
}

float CRandomFloat( int *seed ) {
	return 2.0f * ( RandomFloat( seed ) - 0.5f );
}

void gunrandom( float *outX, float *outY ) {
	float degrees, radius, s, c;

	degrees = (float)rand() / 32768.0f * 360.0f;
	radius  = (float)rand() / 32768.0f;

	SinCos( degrees * (float)M_PI / 180.0f, &s, &c );

	*outX = c * radius;
	*outY = s * radius;
}

#define NUMVERTEXNORMALS    162

vec3_t bytedirs[NUMVERTEXNORMALS] =
{
	{  -0.525731f,   0.000000f,   0.850651f },
	{  -0.442863f,   0.238856f,   0.864188f },
	{  -0.295242f,   0.000000f,   0.955423f },
	{  -0.309017f,   0.500000f,   0.809017f },
	{  -0.162460f,   0.262866f,   0.951056f },
	{   0.000000f,   0.000000f,   1.000000f },
	{   0.000000f,   0.850651f,   0.525731f },
	{  -0.147621f,   0.716567f,   0.681718f },
	{   0.147621f,   0.716567f,   0.681718f },
	{   0.000000f,   0.525731f,   0.850651f },
	{   0.309017f,   0.500000f,   0.809017f },
	{   0.525731f,   0.000000f,   0.850651f },
	{   0.295242f,   0.000000f,   0.955423f },
	{   0.442863f,   0.238856f,   0.864188f },
	{   0.162460f,   0.262866f,   0.951056f },
	{  -0.681718f,   0.147621f,   0.716567f },
	{  -0.809017f,   0.309017f,   0.500000f },
	{  -0.587785f,   0.425325f,   0.688191f },
	{  -0.850651f,   0.525731f,   0.000000f },
	{  -0.864188f,   0.442863f,   0.238856f },
	{  -0.716567f,   0.681718f,   0.147621f },
	{  -0.688191f,   0.587785f,   0.425325f },
	{  -0.500000f,   0.809017f,   0.309017f },
	{  -0.238856f,   0.864188f,   0.442863f },
	{  -0.425325f,   0.688191f,   0.587785f },
	{  -0.716567f,   0.681718f,  -0.147621f },
	{  -0.500000f,   0.809017f,  -0.309017f },
	{  -0.525731f,   0.850651f,   0.000000f },
	{   0.000000f,   0.850651f,  -0.525731f },
	{  -0.238856f,   0.864188f,  -0.442863f },
	{   0.000000f,   0.955423f,  -0.295242f },
	{  -0.262866f,   0.951056f,  -0.162460f },
	{   0.000000f,   1.000000f,   0.000000f },
	{   0.000000f,   0.955423f,   0.295242f },
	{  -0.262866f,   0.951056f,   0.162460f },
	{   0.238856f,   0.864188f,   0.442863f },
	{   0.262866f,   0.951056f,   0.162460f },
	{   0.500000f,   0.809017f,   0.309017f },
	{   0.238856f,   0.864188f,  -0.442863f },
	{   0.262866f,   0.951056f,  -0.162460f },
	{   0.500000f,   0.809017f,  -0.309017f },
	{   0.850651f,   0.525731f,   0.000000f },
	{   0.716567f,   0.681718f,   0.147621f },
	{   0.716567f,   0.681718f,  -0.147621f },
	{   0.525731f,   0.850651f,   0.000000f },
	{   0.425325f,   0.688191f,   0.587785f },
	{   0.864188f,   0.442863f,   0.238856f },
	{   0.688191f,   0.587785f,   0.425325f },
	{   0.809017f,   0.309017f,   0.500000f },
	{   0.681718f,   0.147621f,   0.716567f },
	{   0.587785f,   0.425325f,   0.688191f },
	{   0.955423f,   0.295242f,   0.000000f },
	{   1.000000f,   0.000000f,   0.000000f },
	{   0.951056f,   0.162460f,   0.262866f },
	{   0.850651f,  -0.525731f,   0.000000f },
	{   0.955423f,  -0.295242f,   0.000000f },
	{   0.864188f,  -0.442863f,   0.238856f },
	{   0.951056f,  -0.162460f,   0.262866f },
	{   0.809017f,  -0.309017f,   0.500000f },
	{   0.681718f,  -0.147621f,   0.716567f },
	{   0.850651f,   0.000000f,   0.525731f },
	{   0.864188f,   0.442863f,  -0.238856f },
	{   0.809017f,   0.309017f,  -0.500000f },
	{   0.951056f,   0.162460f,  -0.262866f },
	{   0.525731f,   0.000000f,  -0.850651f },
	{   0.681718f,   0.147621f,  -0.716567f },
	{   0.681718f,  -0.147621f,  -0.716567f },
	{   0.850651f,   0.000000f,  -0.525731f },
	{   0.809017f,  -0.309017f,  -0.500000f },
	{   0.864188f,  -0.442863f,  -0.238856f },
	{   0.951056f,  -0.162460f,  -0.262866f },
	{   0.147621f,   0.716567f,  -0.681718f },
	{   0.309017f,   0.500000f,  -0.809017f },
	{   0.425325f,   0.688191f,  -0.587785f },
	{   0.442863f,   0.238856f,  -0.864188f },
	{   0.587785f,   0.425325f,  -0.688191f },
	{   0.688191f,   0.587785f,  -0.425325f },
	{  -0.147621f,   0.716567f,  -0.681718f },
	{  -0.309017f,   0.500000f,  -0.809017f },
	{   0.000000f,   0.525731f,  -0.850651f },
	{  -0.525731f,   0.000000f,  -0.850651f },
	{  -0.442863f,   0.238856f,  -0.864188f },
	{  -0.295242f,   0.000000f,  -0.955423f },
	{  -0.162460f,   0.262866f,  -0.951056f },
	{   0.000000f,   0.000000f,  -1.000000f },
	{   0.295242f,   0.000000f,  -0.955423f },
	{   0.162460f,   0.262866f,  -0.951056f },
	{  -0.442863f,  -0.238856f,  -0.864188f },
	{  -0.309017f,  -0.500000f,  -0.809017f },
	{  -0.162460f,  -0.262866f,  -0.951056f },
	{   0.000000f,  -0.850651f,  -0.525731f },
	{  -0.147621f,  -0.716567f,  -0.681718f },
	{   0.147621f,  -0.716567f,  -0.681718f },
	{   0.000000f,  -0.525731f,  -0.850651f },
	{   0.309017f,  -0.500000f,  -0.809017f },
	{   0.442863f,  -0.238856f,  -0.864188f },
	{   0.162460f,  -0.262866f,  -0.951056f },
	{   0.238856f,  -0.864188f,  -0.442863f },
	{   0.500000f,  -0.809017f,  -0.309017f },
	{   0.425325f,  -0.688191f,  -0.587785f },
	{   0.716567f,  -0.681718f,  -0.147621f },
	{   0.688191f,  -0.587785f,  -0.425325f },
	{   0.587785f,  -0.425325f,  -0.688191f },
	{   0.000000f,  -0.955423f,  -0.295242f },
	{   0.000000f,  -1.000000f,   0.000000f },
	{   0.262866f,  -0.951056f,  -0.162460f },
	{   0.000000f,  -0.850651f,   0.525731f },
	{   0.000000f,  -0.955423f,   0.295242f },
	{   0.238856f,  -0.864188f,   0.442863f },
	{   0.262866f,  -0.951056f,   0.162460f },
	{   0.500000f,  -0.809017f,   0.309017f },
	{   0.716567f,  -0.681718f,   0.147621f },
	{   0.525731f,  -0.850651f,   0.000000f },
	{  -0.238856f,  -0.864188f,  -0.442863f },
	{  -0.500000f,  -0.809017f,  -0.309017f },
	{  -0.262866f,  -0.951056f,  -0.162460f },
	{  -0.850651f,  -0.525731f,   0.000000f },
	{  -0.716567f,  -0.681718f,  -0.147621f },
	{  -0.716567f,  -0.681718f,   0.147621f },
	{  -0.525731f,  -0.850651f,   0.000000f },
	{  -0.500000f,  -0.809017f,   0.309017f },
	{  -0.238856f,  -0.864188f,   0.442863f },
	{  -0.262866f,  -0.951056f,   0.162460f },
	{  -0.864188f,  -0.442863f,   0.238856f },
	{  -0.809017f,  -0.309017f,   0.500000f },
	{  -0.688191f,  -0.587785f,   0.425325f },
	{  -0.681718f,  -0.147621f,   0.716567f },
	{  -0.442863f,  -0.238856f,   0.864188f },
	{  -0.587785f,  -0.425325f,   0.688191f },
	{  -0.309017f,  -0.500000f,   0.809017f },
	{  -0.147621f,  -0.716567f,   0.681718f },
	{  -0.425325f,  -0.688191f,   0.587785f },
	{  -0.162460f,  -0.262866f,   0.951056f },
	{   0.442863f,  -0.238856f,   0.864188f },
	{   0.162460f,  -0.262866f,   0.951056f },
	{   0.309017f,  -0.500000f,   0.809017f },
	{   0.147621f,  -0.716567f,   0.681718f },
	{   0.000000f,  -0.525731f,   0.850651f },
	{   0.425325f,  -0.688191f,   0.587785f },
	{   0.587785f,  -0.425325f,   0.688191f },
	{   0.688191f,  -0.587785f,   0.425325f },
	{  -0.955423f,   0.295242f,   0.000000f },
	{  -0.951056f,   0.162460f,   0.262866f },
	{  -1.000000f,   0.000000f,   0.000000f },
	{  -0.850651f,   0.000000f,   0.525731f },
	{  -0.955423f,  -0.295242f,   0.000000f },
	{  -0.951056f,  -0.162460f,   0.262866f },
	{  -0.864188f,   0.442863f,  -0.238856f },
	{  -0.951056f,   0.162460f,  -0.262866f },
	{  -0.809017f,   0.309017f,  -0.500000f },
	{  -0.864188f,  -0.442863f,  -0.238856f },
	{  -0.951056f,  -0.162460f,  -0.262866f },
	{  -0.809017f,  -0.309017f,  -0.500000f },
	{  -0.681718f,   0.147621f,  -0.716567f },
	{  -0.681718f,  -0.147621f,  -0.716567f },
	{  -0.850651f,   0.000000f,  -0.525731f },
	{  -0.688191f,   0.587785f,  -0.425325f },
	{  -0.587785f,   0.425325f,  -0.688191f },
	{  -0.425325f,   0.688191f,  -0.587785f },
	{  -0.425325f,  -0.688191f,  -0.587785f },
	{  -0.587785f,  -0.425325f,  -0.688191f },
	{  -0.688191f,  -0.587785f,  -0.425325f },
};

// this isn't a real cheap function to call!
byte DirToByte( vec3_t dir ) {
	byte i, best;
	float d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for ( i = 0 ; i < NUMVERTEXNORMALS ; i++ ) {
		d = bytedirs[i][2] * dir[2] + bytedirs[i][0] * dir[0] + bytedirs[i][1] * dir[1];   // keep: term order
		if ( d > bestd ) {
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy( bytedirs[b], dir );
}

int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	int i;

	for ( i = 0 ; i < 3 ; i++ ) {
		// keep: the float casts
		if ( (float)( v1[i] - v2[i] ) * (float)( v1[i] - v2[i] ) > 0.0000010000001f ) {
			return 0;
		}
	}
	return 1;
}

vec_t VectorLength( const vec3_t v ) {
	return (vec_t)sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t v;

	VectorSubtract( p2, p1, v );   // keep: p2 - p1

	return (vec_t)sqrt( v[2] * v[2] + v[1] * v[1] + v[0] * v[0] );
}

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t v;

	VectorSubtract( p2, p1, v );

	return v[2] * v[2] + v[1] * v[1] + v[0] * v[0];
}

vec_t Distance2d( const vec3_t p1, const vec3_t p2 ) {
	float d[2];

	d[0] = p2[0] - p1[0];
	d[1] = p2[1] - p1[1];

	return (vec_t)sqrt( d[1] * d[1] + d[0] * d[0] );
}

vec_t Distance2dSquared( const vec3_t p1, const vec3_t p2 ) {
	float d[2];   // keep: an array, not two locals

	d[0] = p2[0] - p1[0];
	d[1] = p2[1] - p1[1];

	return d[1] * d[1] + d[0] * d[0];
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void Perp2D( vec3_t out, const vec3_t in ) {
	// keep: out[0] is written before in[0] is read, so Perp2D( v, v ) gives ( v[1], -v[1], 0 )
	out[0] = in[1];
	out[1] = -in[0];
	out[2] = 0;
}

vec_t VectorNormalize( vec3_t v ) {
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt( length );

	if ( length ) {
		ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

vec_t VectorNormalize2D( float *v ) {
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1];
	length = (float)sqrt( length );

	if ( length != 0.0f ) {
		ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
	}

	return length;
}

vec_t Vector4Normalize( vec4_t v ) {
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];
	length = (float)sqrt( length );

	if ( length != 0.0f ) {
		ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
		v[3] *= ilength;
	}

	return length;
}

//
// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length
//
void VectorNormalizeFast( vec3_t v ) {
	float ilength;

	ilength = __fsqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out ) {
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt( length );

	if ( length ) {
		ilength = 1.0f / length;
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	} else {
		VectorClear( out );
	}

	return length;
}

void VectorInverse( vec3_t v ) {
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out ) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
}

vec_t VectorMax( const vec3_t v ) {
	float m;

	m = ( v[0] >= v[1] ) ? v[0] : v[1];

	return ( m >= v[2] ) ? m : v[2];
}

void Vector5Add_m( const float *a, const float *b, float *out ) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	out[2] = a[2] + b[2];
	out[3] = a[3] + b[3];
	out[4] = a[4] + b[4];
}

void Vector5Scale_m( const float *in, float scale, float *out ) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
	out[4] = in[4] * scale;
}

void Vector3Copy_m( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out ) {
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src ) {
	int pos;
	int i;
	float minelem;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	minelem = 1.0f;
	pos = 0;
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( fabs( src[i] ) < minelem ) {
			pos = i;
			minelem = fabs( src[i] );
		}
	}

	tempvec[0] = tempvec[1] = tempvec[2] = 0.0f;
	tempvec[pos] = 1.0f;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOntoVector( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							  float degrees ) {
	float m[3][3];
	float im[3][3];
	float zrot[3][3];
	float tmpmat[3][3];
	float rot[3][3];
	vec3_t vr, vup, vf;
	float rad;
	int i;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0f;

	rad = ( ( degrees * M_PI ) / 180.0F );   // DEG2RAD( degrees )
	SinCos( rad, &zrot[0][1], &zrot[0][0] );
	zrot[1][0] = -zrot[0][1];
	zrot[1][1] = zrot[0][0];

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ ) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {
	// create an arbitrary axis[1]
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw != 0.0f ) {
		vec3_t temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up ) {
	float d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	// keep: term order matches original codegen
	d = right[0] * forward[0] + right[2] * forward[2] + right[1] * forward[1];
	VectorMA( right, -d, forward, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
}

float vectoyaw( const vec3_t vec ) {
	float yaw;

	if ( vec[1] == 0.0f && vec[0] == 0.0f ) {
		yaw = 0.0f;
	} else {
		yaw = (float)( 180.0 * atan2( vec[1], vec[0] ) / (double)(float)M_PI );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}
	}
	return yaw;
}

float vectosignedyaw( const vec3_t vec ) {
	float yaw;

	if ( vec[1] == 0.0f && vec[0] == 0.0f ) {
		yaw = 0.0f;
	} else {
		yaw = (float)( 180.0 * atan2( vec[1], vec[0] ) / (double)(float)M_PI );
	}
	return yaw;
}

float vectopitch( const vec3_t vec ) {
	float forward, pitch;

	if ( vec[1] == 0.0f && vec[0] == 0.0f ) {
		if ( vec[2] > 0.0f ) {
			pitch = 270.0f;
		} else {
			pitch = 90.0f;
		}
	} else {
		forward = (float)sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
		pitch = (float)( -180.0 * atan2( vec[2], forward ) / (double)(float)M_PI );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}
	return pitch;
}

float vectosignedpitch( const vec3_t vec ) {
	float forward, pitch;

	if ( vec[1] == 0.0f && vec[0] == 0.0f ) {
		if ( vec[2] > 0.0f ) {
			pitch = -90.0f;
		} else {
			pitch = 90.0f;
		}
	} else {
		forward = (float)sqrt( vec[0] * vec[0] + vec[1] * vec[1] );
		pitch = (float)( -180.0 * atan2( vec[2], forward ) / (double)(float)M_PI );
	}
	return pitch;
}

void vectoangles( const vec3_t value1, vec3_t angles ) {
	float forward;
	float yaw, pitch;

	if ( value1[1] == 0.0f && value1[0] == 0.0f ) {
		yaw = 0.0f;
		pitch = ( value1[2] > 0.0f ) ? 270.0f : 90.0f;
	} else {
		yaw = (float)( 180.0 * atan2( value1[1], value1[0] ) / (double)(float)M_PI );
		if ( yaw < 0.0f ) {
			yaw += 360.0f;
		}

		forward = (float)sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = (float)( -180.0 * atan2( value1[2], forward ) / (double)(float)M_PI );
		if ( pitch < 0.0f ) {
			pitch += 360.0f;
		}
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0.0f;
}

void vectoanglessigned( const vec3_t value1, vec3_t angles ) {
	float forward;
	float yaw, pitch;

	if ( value1[1] == 0.0f && value1[0] == 0.0f ) {
		yaw = 0.0f;
		pitch = ( value1[2] > 0.0f ) ? -90.0f : 90.0f;
	} else {
		yaw = (float)( 180.0 * atan2( value1[1], value1[0] ) / (double)(float)M_PI );

		forward = (float)sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = (float)( -180.0 * atan2( value1[2], forward ) / (double)(float)M_PI );
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0.0f;
}

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up ) {
	float angle;
	float sr, sp, sy, cr, cp, cy;
	float t, u;

	angle = angles[1] * ( (float)M_PI * 2 / 360 );
	SinCos( angle, &sy, &cy );

	angle = angles[0] * ( (float)M_PI * 2 / 360 );
	SinCos( angle, &sp, &cp );

	if ( forward ) {
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if ( !right && !up ) {
		return;
	}

	angle = angles[2] * ( (float)M_PI * 2 / 360 );
	SinCos( angle, &sr, &cr );

	// keep: the t and u temporaries match original codegen
	if ( right ) {
		t = sr * sp;
		right[0] = cr * sy - t * cy;
		right[1] = -1 * cr * cy - t * sy;
		right[2] = -1 * sr * cp;
	}

	if ( up ) {
		u = cr * sp;
		up[0] = u * cy + sr * sy;
		up[1] = u * sy - sr * cy;
		up[2] = cr * cp;
	}
}

/*
================
MatrixMultiply
================
*/
void MatrixMultiply( float in1[3][3], float in2[3][3], float out[3][3] ) {
	// keep: term order matches original codegen
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1] + in1[0][0] * in2[0][1];
	out[0][2] = in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2] + in1[0][0] * in2[0][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][2] * in2[2][1] + in1[1][1] * in2[1][1] + in1[1][0] * in2[0][1];
	out[1][2] = in1[1][2] * in2[2][2] + in1[1][1] * in2[1][2] + in1[1][0] * in2[0][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][2] * in2[2][0] + in1[2][1] * in2[1][0];
	out[2][1] = in1[2][2] * in2[2][1] + in1[2][1] * in2[1][1] + in1[2][0] * in2[0][1];
	out[2][2] = in1[2][2] * in2[2][2] + in1[2][1] * in2[1][2] + in1[2][0] * in2[0][2];
}

// inout = left * inout
void MatrixMultiply33InPlace_m( const float left[3][3], float inout[3][3] ) {
	float t[2][3];

	t[0][0] = left[0][0] * inout[0][0] + left[0][1] * inout[1][0] + left[0][2] * inout[2][0];
	t[0][1] = left[0][0] * inout[0][1] + left[0][1] * inout[1][1] + left[0][2] * inout[2][1];
	t[0][2] = left[0][0] * inout[0][2] + left[0][1] * inout[1][2] + left[0][2] * inout[2][2];
	t[1][0] = left[1][0] * inout[0][0] + left[1][1] * inout[1][0] + left[1][2] * inout[2][0];
	t[1][1] = left[1][0] * inout[0][1] + left[1][1] * inout[1][1] + left[1][2] * inout[2][1];
	t[1][2] = left[1][0] * inout[0][2] + left[1][1] * inout[1][2] + left[1][2] * inout[2][2];
	inout[2][0] = left[2][0] * inout[0][0] + left[2][1] * inout[1][0] + left[2][2] * inout[2][0];
	inout[2][1] = left[2][0] * inout[0][1] + left[2][1] * inout[1][1] + left[2][2] * inout[2][1];
	inout[2][2] = left[2][0] * inout[0][2] + left[2][1] * inout[1][2] + left[2][2] * inout[2][2];

	inout[0][0] = t[0][0];
	inout[0][1] = t[0][1];
	inout[0][2] = t[0][2];
	inout[1][0] = t[1][0];
	inout[1][1] = t[1][1];
	inout[1][2] = t[1][2];
}

void MatrixMultiply34_m( const float left[3][4], const float right[3][4], float out[3][4] ) {
	// keep: written out in full; rows 1 and 2 sum back to front
	out[0][0] = left[0][0] * right[0][0] + left[0][1] * right[1][0] + left[0][2] * right[2][0];
	out[0][1] = left[0][0] * right[0][1] + left[0][1] * right[1][1] + left[0][2] * right[2][1];
	out[0][2] = left[0][0] * right[0][2] + left[0][1] * right[1][2] + left[0][2] * right[2][2];
	out[0][3] = left[0][0] * right[0][3] + left[0][1] * right[1][3] + left[0][2] * right[2][3] + left[0][3];
	out[1][0] = left[1][2] * right[2][0] + left[1][1] * right[1][0] + left[1][0] * right[0][0];
	out[1][1] = left[1][2] * right[2][1] + left[1][1] * right[1][1] + left[1][0] * right[0][1];
	out[1][2] = left[1][2] * right[2][2] + left[1][1] * right[1][2] + left[1][0] * right[0][2];
	out[1][3] = left[1][2] * right[2][3] + left[1][1] * right[1][3] + left[1][0] * right[0][3] + left[1][3];
	out[2][0] = left[2][2] * right[2][0] + left[2][1] * right[1][0] + left[2][0] * right[0][0];
	out[2][1] = left[2][2] * right[2][1] + left[2][1] * right[1][1] + left[2][0] * right[0][1];
	out[2][2] = left[2][2] * right[2][2] + left[2][1] * right[1][2] + left[2][0] * right[0][2];
	out[2][3] = left[2][2] * right[2][3] + left[2][1] * right[1][3] + left[2][0] * right[0][3] + left[2][3];
}

void QuatToMatrix( float quatAndMatrix[9] ) {
	float xx, yy, zz, s;
	float xs, ys;
	float xy, xz, xw, yz, yw, zw;

	// keep: s holds both len2 and 2 / len2
	xx = quatAndMatrix[0] * quatAndMatrix[0];
	yy = quatAndMatrix[1] * quatAndMatrix[1];
	zz = quatAndMatrix[2] * quatAndMatrix[2];
	s = quatAndMatrix[3] * quatAndMatrix[3] + zz + yy + xx;

	if ( s != 0.0f ) {
		s = 2.0f / s;

		xx *= s;
		yy *= s;
		zz *= s;

		xs = s * quatAndMatrix[0];
		xy = xs * quatAndMatrix[1];
		xz = xs * quatAndMatrix[2];
		xw = xs * quatAndMatrix[3];

		ys = s * quatAndMatrix[1];
		yz = ys * quatAndMatrix[2];
		yw = ys * quatAndMatrix[3];

		zw = quatAndMatrix[3] * quatAndMatrix[2] * s;

		quatAndMatrix[0] = 1.0f - ( zz + yy );
		quatAndMatrix[1] = zw + xy;
		quatAndMatrix[2] = xz - yw;
		quatAndMatrix[3] = xy - zw;
		quatAndMatrix[4] = 1.0f - ( zz + xx );
		quatAndMatrix[5] = yz + xw;
		quatAndMatrix[6] = yw + xz;
		quatAndMatrix[7] = yz - xw;
		quatAndMatrix[8] = 1.0f - ( yy + xx );
	} else {
		quatAndMatrix[0] = 1.0f;
		quatAndMatrix[1] = 0.0f;
		quatAndMatrix[2] = 0.0f;
		quatAndMatrix[3] = 0.0f;
		quatAndMatrix[4] = 1.0f;
		quatAndMatrix[5] = 0.0f;
		quatAndMatrix[6] = 0.0f;
		quatAndMatrix[7] = 0.0f;
		quatAndMatrix[8] = 1.0f;
	}
}

float RotationToYaw( const float v[2] ) {
	float xx, scale;

	xx = v[0] * v[0];
	scale = xx + v[1] * v[1];
	scale = 2.0f / scale;

	// keep: the (double) cast; 57.2957763671875 is (float)( 180 / pi )
	return (float)( atan2( (double)v[0] * v[1] * scale, 1.0f - xx * scale ) * 57.2957763671875 );
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );

	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
================
ProjectPointOntoVector
================
*/
void ProjectPointOntoVector( vec3_t dst, const vec3_t p, const vec3_t normal ) {
	float d;
	vec3_t n;
	float inv_denom;
	float lenSq;

	// keep: the lenSq temporary and the z, y, x term order below
	lenSq = DotProduct( normal, normal );
	inv_denom = 1.0f / lenSq;

	d = ( p[2] * normal[2] + p[1] * normal[1] + p[0] * normal[0] ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
#if defined( _MSC_VER ) && defined( _M_IX86 )
// the id386 x87 version, with its lazily built jump table
#pragma warning( disable: 4035 )
__declspec( naked ) int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, cplane_t *p ) {
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {
		push	ebx

		cmp		bops_initialized, 1
		je		initialized
		mov		bops_initialized, 1

		mov		Ljmptab[0*4], offset Lcase0
		mov		Ljmptab[1*4], offset Lcase1
		mov		Ljmptab[2*4], offset Lcase2
		mov		Ljmptab[3*4], offset Lcase3
		mov		Ljmptab[4*4], offset Lcase4
		mov		Ljmptab[5*4], offset Lcase5
		mov		Ljmptab[6*4], offset Lcase6
		mov		Ljmptab[7*4], offset Lcase7

initialized:

		mov		edx, dword ptr [4+12+esp]
		mov		ecx, dword ptr [4+4+esp]
		xor		eax, eax
		mov		ebx, dword ptr [4+8+esp]
		mov		al, byte ptr [17+edx]
		cmp		al, 8
		jae		Lerror
		fld		dword ptr [0+edx]
		fld		st(0)
		jmp		dword ptr [Ljmptab+eax*4]
Lcase0:
		fmul	dword ptr [ebx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ebx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
		jmp		LSetSides
Lcase1:
		fmul	dword ptr [ecx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ebx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
		jmp		LSetSides
Lcase2:
		fmul	dword ptr [ebx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ecx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
		jmp		LSetSides
Lcase3:
		fmul	dword ptr [ecx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ecx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
		jmp		LSetSides
Lcase4:
		fmul	dword ptr [ebx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ebx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
		jmp		LSetSides
Lcase5:
		fmul	dword ptr [ecx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ebx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
		jmp		LSetSides
Lcase6:
		fmul	dword ptr [ebx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ecx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ecx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
		jmp		LSetSides
Lcase7:
		fmul	dword ptr [ecx]
		fld		dword ptr [0+4+edx]
		fxch	st(2)
		fmul	dword ptr [ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [4+ecx]
		fld		dword ptr [0+8+edx]
		fxch	st(2)
		fmul	dword ptr [4+ebx]
		fxch	st(2)
		fld		st(0)
		fmul	dword ptr [8+ecx]
		fxch	st(5)
		faddp	st(3), st(0)
		fmul	dword ptr [8+ebx]
		fxch	st(1)
		faddp	st(3), st(0)
		fxch	st(3)
		faddp	st(2), st(0)
LSetSides:
		faddp	st(2), st(0)
		fcomp	dword ptr [12+edx]
		xor		ecx, ecx
		fnstsw	ax
		fcomp	dword ptr [12+edx]
		and		ah, 1
		xor		ah, 1
		add		cl, ah
		fnstsw	ax
		and		ah, 1
		add		ah, ah
		add		cl, ah
		pop		ebx
		mov		eax, ecx
		ret
Lerror:
		int		3
	}
	Com_Error( ERR_DROP, "\x15" "BoxOnPlaneSide: invalid signbits for plane" );
}
#pragma warning( default: 4035 )
#else
int BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, cplane_t *p ) {
	// double, to match the unrounded sums of the asm version
	double dist1, dist2;
	int sides;

	switch ( p->signbits ) {
	case 0:
		dist1 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emaxs[2];
		dist2 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emaxs[2];
		dist2 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emaxs[2];
		dist2 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emaxs[2];
		dist2 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emins[2];
		dist2 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emins[2];
		dist2 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emins[2];
		dist2 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = (double)p->normal[0] * emins[0] + (double)p->normal[1] * emins[1] + (double)p->normal[2] * emins[2];
		dist2 = (double)p->normal[0] * emaxs[0] + (double)p->normal[1] * emaxs[1] + (double)p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;
		Com_Error( ERR_DROP, "\x15" "BoxOnPlaneSide: invalid signbits for plane" );
		break;
	}

	sides = 0;
	if ( dist1 >= p->dist ) {
		sides = 1;
	}
	// not >=, so that a NaN sets the bit as in the asm version
	if ( !( dist2 >= p->dist ) ) {
		sides |= 2;
	}

	return sides;
}
#endif

void RotatePointByAngles_m( const vec3_t in, const vec3_t angles, vec3_t out ) {
	int pair[3][2];
	vec3_t v, r;
	double rad, s, c;   // keep: double
	int i;

	VectorCopy( in, v );
	VectorCopy( v, r );

	pair[0][0] = 1;
	pair[0][1] = 2;
	pair[1][0] = 2;
	pair[1][1] = 0;
	pair[2][0] = 0;
	pair[2][1] = 1;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( angles[i] != 0.0f ) {
			rad = angles[i] * M_PI / 180.0;
			SinCos_double( rad, &s, &c );

			// keep: direct pair[] indexing and the parentheses
			r[pair[i][0]] = v[pair[i][0]] * c - v[pair[i][1]] * s;
			r[pair[i][1]] = ( v[pair[i][1]] * c ) + ( v[pair[i][0]] * s );
		}
		VectorCopy( r, v );
	}

	VectorCopy( r, out );
}

void PolarToVector_m( vec3_t out, float radius, float angle ) {
	float s1, c1, s2, c2;

	SinCos( angle, &s1, &c1 );
	SinCos( angle, &s2, &c2 );

	out[0] = c2 * c1 * radius;
	out[1] = c2 * s1 * radius;
	out[2] = s2 * radius;
}

float PitchOfVectorAlongYaw( float yawDegrees, const vec3_t vec ) {
	vec3_t flatForward, projected;

	YawVectors( yawDegrees, flatForward, NULL );

	ProjectPointOntoVector( projected, flatForward, vec );

	return vectopitch( projected );
}

//============================================================================

// a second generator with one global seed, separate from RandomInt's; dll only
#if defined( UIDLL ) || defined( GAMEDLL ) || defined( CGAMEDLL )
int com_randSeed = 0x89ABCDEF;

void Com_SRand( int seed ) {
	com_randSeed = seed;
}

float Com_RandFloatRange( float lo, float hi ) {
	float result;

	com_randSeed = 214013 * com_randSeed + 2531011;
	result = (float)( (unsigned)com_randSeed >> 17 );
	result = ( ( result * ( hi - lo ) ) / 32768.0f ) + lo;   // keep: grouping
	return result;
}

int Com_RandIntRange( int lo, int hi ) {
	com_randSeed = 214013 * com_randSeed + 2531011;
	return ( (int)( (unsigned)com_randSeed >> 17 ) * ( hi - lo ) >> 15 ) + lo;
}
#endif /* UIDLL || GAMEDLL || CGAMEDLL */

//============================================================================

extern float vectosignedpitch( const vec3_t vec );
extern void  vectoangles( const vec3_t value1, vec3_t angles );
extern void  vectoanglessigned( const vec3_t value1, vec3_t angles );
extern void  SinCos( float radians, float *sinOut, float *cosOut );

vec_t _DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc ) {
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

// packs a unit normal into two bytes of latitude and longitude
void NormalToLatLong( const vec3_t normal, byte bytes[2] ) {
	if ( normal[0] == 0.0f && normal[1] == 0.0f ) {
		if ( normal[2] > 0.0f ) {
			bytes[0] = 0;
			bytes[1] = 0;
		} else {
			bytes[0] = 128;
			bytes[1] = 0;
		}
	} else {
		byte longitude = (byte)( atan2( normal[1], normal[0] ) * 180.0 /
			(double)(float)M_PI * ( 255.0f / 360.0f ) );
		byte latitude = (byte)( acos( normal[2] ) * 180.0 /
			(double)(float)M_PI * ( 255.0f / 360.0f ) );
		bytes[0] = latitude;
		bytes[1] = longitude;
	}
}

void Math_Copy10Floats_m( const float in[10], float out[10] ) {
	int i;
	for ( i = 0; i < 10; i++ ) {
		out[i] = in[i];
	}
}

// keep: float add before the floor
float Math_RoundFloat_m( float value ) {
	return (float)floor( value + 0.5f );
}

// out = in1 * in2 for 4x3 matrices (three rotation rows, then the translation)
// keep: store order, column by column
void MatrixMultiply43( const float in1[4][3], const float in2[4][3], float out[4][3] ) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
	out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in2[3][0];
	out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in2[3][1];
	out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in2[3][2];
}

float *DObjSkelMatrixMultiply43( const float in2[16], float in1[12], float out[12] ) {
	// concatenates in1 (compact 3x4) with in2 (padded 4x4) into the compact 3x4 out
	// keep: term and store order match original codegen
	out[0]  = in1[0] * in2[0]  + in1[3] * in2[1]  + in2[2] * in1[6];
	out[3]  = in1[3] * in2[5]  + in1[0] * in2[4]  + in1[6] * in2[6];
	out[6]  = in1[3] * in2[9]  + in1[0] * in2[8]  + in1[6] * in2[10];
	out[1]  = in1[4] * in2[1]  + in2[2] * in1[7]  + in2[0] * in1[1];
	out[4]  = in1[7] * in2[6]  + in1[4] * in2[5]  + in1[1] * in2[4];
	out[7]  = in1[7] * in2[10] + in1[4] * in2[9]  + in1[1] * in2[8];
	out[2]  = in1[2] * in2[0]  + in1[5] * in2[1]  + in2[2] * in1[8];
	out[5]  = in1[8] * in2[6]  + in1[5] * in2[5]  + in1[2] * in2[4];
	out[8]  = in1[8] * in2[10] + in1[5] * in2[9]  + in1[2] * in2[8];
	out[9]  = in1[3] * in2[13] + in1[0] * in2[12] + in1[6] * in2[14] + in1[9];
	out[10] = in1[7] * in2[14] + in1[4] * in2[13] + in1[1] * in2[12] + in1[10];
	out[11] = in1[8] * in2[14] + in1[5] * in2[13] + in1[2] * in2[12] + in1[11];
	return in1;
}

float *DObjSkel2MatrixMultiply43( const float in2[16], float in1[12], float out[16] ) {
	// same as DObjSkelMatrixMultiply43, but out is a padded 4x4
	// keep: term and store order match original codegen
	out[0]  = in1[0] * in2[0]  + in1[3] * in2[1]  + in2[2] * in1[6];
	out[4]  = in1[3] * in2[5]  + in1[0] * in2[4]  + in1[6] * in2[6];
	out[8]  = in1[3] * in2[9]  + in1[0] * in2[8]  + in1[6] * in2[10];
	out[1]  = in1[4] * in2[1]  + in2[2] * in1[7]  + in2[0] * in1[1];
	out[5]  = in1[7] * in2[6]  + in1[4] * in2[5]  + in1[1] * in2[4];
	out[9]  = in1[7] * in2[10] + in1[4] * in2[9]  + in1[1] * in2[8];
	out[2]  = in1[2] * in2[0]  + in1[5] * in2[1]  + in2[2] * in1[8];
	out[6]  = in1[8] * in2[6]  + in1[5] * in2[5]  + in1[2] * in2[4];
	out[10] = in1[8] * in2[10] + in1[5] * in2[9]  + in1[2] * in2[8];
	out[3]  = 0.0f;
	out[7]  = 0.0f;
	out[11] = 0.0f;
	out[12] = in1[0] * in2[12] + in2[14] * in1[6] + in1[3] * in2[13] + in1[9];
	out[13] = in2[14] * in1[7] + in2[13] * in1[4] + in2[12] * in1[1] + in1[10];
	out[14] = in1[2] * in2[12] + in1[8] * in2[14] + in1[5] * in2[13] + in1[11];
	out[15] = 1.0f;
	return in1;
}

// Intel's "Inverse of 4x4 Matrix" cofactor expansion (AP-928)
float *MatrixInverse44( const float in[16], float out[16] ) {
	float tmp[12];   /* pairs */
	float src[16];   /* transposed source */
	float det;
	int i;

	for ( i = 0; i < 4; i++ ) {
		src[i]      = in[i * 4];
		src[i + 4]  = in[i * 4 + 1];
		src[i + 8]  = in[i * 4 + 2];
		src[i + 12] = in[i * 4 + 3];
	}

	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];
	out[0]  = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
	out[0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
	out[1]  = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
	out[1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
	out[2]  = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
	out[2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
	out[3]  = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
	out[3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
	out[4]  = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
	out[4] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
	out[5]  = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
	out[5] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
	out[6]  = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
	out[6] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
	out[7]  = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
	out[7] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];

	tmp[0] = src[2] * src[7];
	tmp[1] = src[3] * src[6];
	tmp[2] = src[1] * src[7];
	tmp[3] = src[3] * src[5];
	tmp[4] = src[1] * src[6];
	tmp[5] = src[2] * src[5];
	tmp[6] = src[0] * src[7];
	tmp[7] = src[3] * src[4];
	tmp[8] = src[0] * src[6];
	tmp[9] = src[2] * src[4];
	tmp[10] = src[0] * src[5];
	tmp[11] = src[1] * src[4];
	out[8]  = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
	out[8] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
	out[9]  = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
	out[9] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
	out[10] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
	out[10]-= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
	out[11] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
	out[11]-= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
	out[12] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
	out[12]-= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
	out[13] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
	out[13]-= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
	out[14] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
	out[14]-= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
	out[15] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
	out[15]-= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];

	// keep: determinant term order
	det = src[0] * out[0] + ( src[2] * out[2] + src[3] * out[3] ) + src[1] * out[1];
	// keep: the label splits the block as in the original codegen
	goto inverseReciprocal;
inverseReciprocal:
	det = 1 / det;
	for ( i = 0; i < 16; i++ ) {
		out[i] *= det;
	}

	return out;
}

float *MatrixTransformVector( const vec3_t in, float m[9], vec3_t out ) {
	// out = m^T . in, no translation
	// keep: out[0] term order
	out[0] = in[1] * m[3] + in[2] * m[6] + m[0] * in[0];
	out[1] = in[0] * m[1] + in[1] * m[4] + in[2] * m[7];
	out[2] = in[0] * m[2] + in[1] * m[5] + in[2] * m[8];
	return m;
}

#if !defined( UIDLL ) && !defined( GAMEDLL ) && !defined( CGAMEDLL )
float *MatrixTransposeTransformVector( const vec3_t in, float m[12], vec3_t out ) {
#else
float *MatrixTransposeTransformVector( float m[12], const vec3_t in, vec3_t out ) {
#endif
	// inverse-rotates the world point in into m's local space
	vec3_t d;

	d[0] = in[0] - m[9];
	d[1] = in[1] - m[10];
	d[2] = in[2] - m[11];

	// keep: term order matches original codegen
	out[0] = d[2] * m[2] + d[1] * m[1] + d[0] * m[0];
	out[1] = d[2] * m[5] + d[1] * m[4] + d[0] * m[3];
	out[2] = d[2] * m[8] + d[1] * m[7] + d[0] * m[6];
	return m;
}

/*
=================
AxisToAngles
=================
*/
void AxisToAngles( const vec3_t axis[3], vec3_t angles ) {
	vec3_t right;
	float s, c, t, roll, angle;

	// first get the pitch and yaw from the forward vector
	vectoangles( axis[0], angles );

	// now get the roll from the right vector, reverse-rotated by yaw then pitch
	VectorCopy( axis[1], right );

	// keep: ( -x ) * pi, not -x * pi
	angle = ( -angles[1] ) * (float)M_PI / 180.0f;
	SinCos( angle, &s, &c );
	t        = c * right[0] - s * right[1];
	right[1] = c * right[1] + s * right[0];

	angle = ( -angles[0] ) * (float)M_PI / 180.0f;
	SinCos( angle, &s, &c );
	right[0] = t * c + s * right[2];
	right[2] = c * right[2] - t * s;

	roll = vectosignedpitch( right );
	if ( right[1] < 0.0f ) {
		angles[2] = ( roll < 0.0f ? 180.0f : -180.0f ) + roll;
	} else {
		angles[2] = -roll;
	}
}

void Axis4ToAngles( const float matrix[3][4], vec3_t angles ) {
	vec3_t right;
	float s, c, t, roll, angle;

	// first get the pitch and yaw from the forward vector
	vectoangles( matrix[0], angles );

	// now get the roll from the right vector, reverse-rotated by yaw then pitch
	VectorCopy( matrix[1], right );

	angle = ( -angles[1] ) * (float)M_PI / 180.0f;
	SinCos( angle, &s, &c );
	t        = c * right[0] - s * right[1];
	right[1] = c * right[1] + s * right[0];

	angle = ( -angles[0] ) * (float)M_PI / 180.0f;
	SinCos( angle, &s, &c );
	right[0] = t * c + s * right[2];
	right[2] = c * right[2] - t * s;

	roll = vectosignedpitch( right );
	if ( right[1] < 0.0f ) {
		angles[2] = ( roll < 0.0f ? 180.0f : -180.0f ) + roll;
	} else {
		angles[2] = -roll;
	}
}

void AxisToSignedAngles( const vec3_t axis[3], vec3_t angles ) {
	vec3_t right;
	float s, c, t, roll, angle;

	// first get the pitch and yaw from the forward vector
	vectoanglessigned( axis[0], angles );

	// now get the roll from the right vector, reverse-rotated by yaw then pitch
	VectorCopy( axis[1], right );

	angle = ( -angles[1] ) * (float)M_PI / 180.0f;
	SinCos( angle, &s, &c );
	t        = c * right[0] - s * right[1];
	right[1] = c * right[1] + s * right[0];

	angle = ( -angles[0] ) * (float)M_PI / 180.0f;
	SinCos( angle, &s, &c );
	right[0] = t * c + s * right[2];
	right[2] = c * right[2] - t * s;

	roll = vectosignedpitch( right );
	if ( right[1] < 0.0f ) {
		angles[2] = ( roll < 0.0f ? 180.0f : -180.0f ) + roll;
	} else {
		angles[2] = -roll;
	}
}
