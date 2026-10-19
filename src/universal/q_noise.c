/*
 * universal/q_noise.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/universal/q_noise.c
 *
 * Retail range 0x004DD440-0x004DD91F.
 *
 * @fidelity-default: verified
 */

#include "../qcommon/qcommon.h"

#include <stdlib.h>

#define NOISE_TABLE_SIZE    256
#define NOISE_SEED          1001

/* NoiseFloor -- no retail address; inlined 4x in Com_NoiseGet4f (0x004DD4F7..0x004DD599):
 * fld v; fsub qword 0.4999999990686774 (= 0.5 - 2^-30); fistp under the current
 * control word (default round-to-nearest, _PC_53).  Not floor(): v in [-2^-30, 0)
 * gives 0, and odd integers with 2^23 <= |v| < 2^24 give v - 1. */
static int NoiseFloor( float v ) {
	double bias = 0.4999999990686774;	/* qword 0x3FDFFFFFFF000000 built at 0x004DD4F7 */
	int    i;

	__asm fld   v
	__asm fsub  bias
	__asm fistp i

	return i;
}

static float noiseTable[NOISE_TABLE_SIZE];
static int noisePermute[NOISE_TABLE_SIZE];

#define VAL( a )                ( noisePermute[ ( a ) & ( NOISE_TABLE_SIZE - 1 ) ] )
#define INDEX( x, y, z, t )     VAL( x + VAL( y + VAL( z + VAL( t ) ) ) )
#ifdef COD1_VC7
#define LERP( a, b, w )         ( a * ( 1.0f - w ) + b * w )	/* Q3's literal text: VC7 honours explicit argument parentheses, which reorder its schedule */
#else
#define LERP( a, b, w )         ( ( a ) * ( 1.0f - ( rfloat )( w ) ) + ( b ) * ( rfloat )( w ) )
#endif

/* ---- Com_NoiseInit  0x004DD490 ----
 * (rfloat)r: retail's fild/fmul/fadd/fsub run at _PC_53 and round once at the fstp. */
void Com_NoiseInit( void ) {
	int i;
	int r;

	srand( NOISE_SEED );

	for ( i = 0 ; i < NOISE_TABLE_SIZE ; i++ ) {
		r = rand();
		noiseTable[i] = ( ( rfloat )r * ( 1.0f / 32767.0f ) ) + ( ( rfloat )r * ( 1.0f / 32767.0f ) ) - 1.0;
		noisePermute[i] = ( byte )( ( rand() << 8 ) / 0x8000 );
	}
}

/* ---- GetNoiseValue  0x004DD440 ---- */
float GetNoiseValue( int x, int y, int z, int t ) {
	int index = INDEX( x, y, z, t );

	return noiseTable[index];
}

/* ---- Com_NoiseGet4f  0x004DD4F0 ----
 * Retail keeps ft, the 1-w factors and every lerp on the x87 stack at _PC_53 and rounds only
 * fx/fy/fz (0x004DD52B/53/7B) and value[] (0x004DD79C, 0x004DD8FD) to float; the final blend is
 * returned unrounded in ST0 (0x004DD917), hence the double return type.  The rfloat locals and
 * casts in LERP keep the modern build unrounded in the same places. */
double Com_NoiseGet4f( float x, float y, float z, float t ) {
	int i;
	int ix, iy, iz, it;
	float fx, fy, fz;
	rfloat ft;
	float front[4];
	float back[4];
	rfloat fvalue, bvalue;
	float value[2];
	rfloat finalvalue;

	ix = NoiseFloor( x );
	fx = x - ( rfloat )ix;
	iy = NoiseFloor( y );
	fy = y - ( rfloat )iy;
	iz = NoiseFloor( z );
	fz = z - ( rfloat )iz;
	it = NoiseFloor( t );
	ft = t - ( rfloat )it;

	for ( i = 0; i < 2; i++ ) {
		front[0] = GetNoiseValue( ix,     iy,     iz,     it + i );
		front[1] = GetNoiseValue( ix + 1, iy,     iz,     it + i );
		front[2] = GetNoiseValue( ix,     iy + 1, iz,     it + i );
		front[3] = GetNoiseValue( ix + 1, iy + 1, iz,     it + i );

		back[0]  = GetNoiseValue( ix,     iy,     iz + 1, it + i );
		back[1]  = GetNoiseValue( ix + 1, iy,     iz + 1, it + i );
		back[2]  = GetNoiseValue( ix,     iy + 1, iz + 1, it + i );
		back[3]  = GetNoiseValue( ix + 1, iy + 1, iz + 1, it + i );

		fvalue = LERP( LERP( front[0], front[1], fx ), LERP( front[2], front[3], fx ), fy );
		bvalue = LERP( LERP( back[0], back[1], fx ), LERP( back[2], back[3], fx ), fy );

		value[i] = LERP( fvalue, bvalue, fz );
	}

	finalvalue = LERP( value[0], value[1], ft );

	return finalvalue;
}
