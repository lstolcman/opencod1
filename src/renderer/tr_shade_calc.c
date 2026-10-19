/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_orientation.h"   /* backEnd.or 0x016D8DA8 -- one object, not 16 shards */
#include "tr_tess.h"
#include "tr_local.h"         /* typed views; include AFTER tr_records.h (needs TRG) */

/* -------------------------------------------------------------------------
 *  tr_shade_calc.c -- the tess vertex/colour/texcoord generators.
 *
 *  Deforms (RB_DeformTessGeometry) reshape tess.xyz/normals; the RB_Calc*Color
 *  and RB_Calc*TexCoords helpers fill tess.vertexColors and the active texcoord
 *  arrays for one shader stage.  Only the data passed by POINTER is retyped:
 *  the deform stage (deformStage_t / its waveForm_t), the texture-matrix scratch
 *  (texModInfo_t) and the current dynamic light (dlight_t, from tr_local.h).
 *  The tess arrays and the backEnd/tr globals keep the retail storage-of-record
 *  in tr_tess.h / tr_records.h / tr_local.h per the hybrid blob+symbol
 *  scheme.  ftol_tempSpill is the shared myftol spill slot.  myftol (0x515940)
 *  is a bare fld/fistp, so it rounds to nearest under the default control
 *  word; it does not truncate like the __ftol2 an (int) cast compiles to.
 *  The WAVEVALUE sites, RB_CalcDeformSyncNormals, the rgbGen-wave grey level
 *  and the diffuse colour channels use myftol's return value.  The
 *  other float->int conversions here (RB_CalcBulgeVertexes, RB_CalcWaveAlpha,
 *  RB_CalcTurbulentTexCoords, RB_CalcRotateTexCoords, RB_CalcSpecularAlpha)
 *  call __ftol2 in retail and truncate, so they keep plain (int) casts.
 * ------------------------------------------------------------------------- */

int ftol_tempSpill;

float rbSpecularLightOrigin = -960.0f;   /* 0x00571130  specular light {-960,1980,96} */
float rbSpecularLightOriginY           =  1980.0f;   /* 0x00571134 */
float rbSpecularLightOriginZ           =    96.0f;   /* 0x00571138 */

#define FUNCTABLE_SIZE   1024
#define FUNCTABLE_MASK   1023

/* The parenthesised (freq)/(amplitude) operands are what make VC7 load them
 * before tess_shaderTime / the table entry, as retail does. */
#define WAVEVALUE( table, base, amplitude, phase, freq ) \
	( ( base ) + table[ myftol( ( ( phase ) + tess_shaderTime * ( freq ) ) * FUNCTABLE_SIZE ) & FUNCTABLE_MASK ] * ( amplitude ) )

/* genFunc_t -- waveForm_t.func selector (matches RTCW). */
typedef enum {
	GF_NONE             = 0,
	GF_SIN              = 1,
	GF_SQUARE           = 2,
	GF_TRIANGLE         = 3,
	GF_SAWTOOTH         = 4,
	GF_INVERSE_SAWTOOTH = 5,
	GF_NOISE            = 6
} genFunc_t;

/* deform_t -- deformStage_t.deformation selector.  CoD renumbers RTCW's list
 * and adds the FLAP and SYNC_NORMALS kinds; values proven by RB_DeformTessGeometry. */
typedef enum {
	DEFORM_NONE              = 0,
	DEFORM_WAVE              = 1,
	DEFORM_FLAP_S            = 2,     /* RB_CalcFlapVertexes( component 0 ) */
	DEFORM_FLAP_T            = 3,     /* RB_CalcFlapVertexes( component 1 ) */
	DEFORM_NORMALS           = 4,
	DEFORM_SYNC_NORMALS      = 5,     /* CoD-specific */
	DEFORM_BULGE             = 6,
	DEFORM_MOVE              = 7,
	DEFORM_PROJECTION_SHADOW = 8,
	DEFORM_AUTOSPRITE        = 9,
	DEFORM_AUTOSPRITE2       = 10,
	DEFORM_TEXT0             = 11,
	DEFORM_TEXT1             = 12,
	DEFORM_TEXT2             = 13,
	DEFORM_TEXT3             = 14,
	DEFORM_TEXT4             = 15,
	DEFORM_TEXT5             = 16,
	DEFORM_TEXT6             = 17,
	DEFORM_TEXT7             = 18
} deform_t;

/* waveForm_t (0x14) -- table-driven wave.  RTCW layout; func at +0. */
typedef struct {
	int   func;            /* +0x00  genFunc_t */
	float base;            /* +0x04 */
	float amplitude;       /* +0x08 */
	float phase;           /* +0x0c */
	float frequency;       /* +0x10 */
} waveForm_t;
COD1_ASSERT_SIZE(waveForm_t, 0x14);

/* deformStage_t (0x34) -- one shader deform.  Identical to RTCW: proven by the
 * reads in RB_CalcDeformVertexes/Flap/Bulge/Move and the stride 52 walk in
 * RB_DeformTessGeometry (deforms at shader+0xb0, numDeforms at shader+0xac). */
typedef struct {
	int        deformation;        /* +0x00  deform_t */
	vec3_t     moveVector;         /* +0x04 */
	waveForm_t deformationWave;    /* +0x10 */
	float      deformationSpread;  /* +0x24 */
	float      bulgeWidth;         /* +0x28 */
	float      bulgeHeight;        /* +0x2c */
	float      bulgeSpeed;         /* +0x30 */
} deformStage_t;
COD1_ASSERT_SIZE(deformStage_t, 0x34);
ASSERT_OFF(deformStage_t, deformationWave, 0x10);
ASSERT_OFF(deformStage_t, deformationSpread, 0x24);
ASSERT_OFF(deformStage_t, bulgeWidth, 0x28);
ASSERT_OFF(deformStage_t, bulgeSpeed, 0x30);

/* texModInfo_t (0x44) -- one tcMod.  RTCW layout: matrix (+0x18) and translate
 * (+0x28) are proven by RB_CalcTransformTexCoords reading float indices 6..11,
 * wave/scale/scroll by RB_ComputeTexCoords passing texMod+1/+12/+14.  Real
 * fields, not byte padding: a byte array in the stack copies made /GS add a
 * cookie that retail's RB_CalcStretchTexCoords/RB_CalcRotateTexCoords lack. */
typedef struct {
	int        type;                /* +0x00  texMod_t */
	waveForm_t wave;                /* +0x04  TMOD_TURBULENT / TMOD_STRETCH */
	float      matrix[2][2];        /* +0x18  TMOD_TRANSFORM */
	float      translate[2];        /* +0x28 */
	float      scale[2];            /* +0x30  TMOD_SCALE */
	float      scroll[2];           /* +0x38  TMOD_SCROLL */
	float      rotateSpeed;         /* +0x40  TMOD_ROTATE */
} texModInfo_t;
COD1_ASSERT_SIZE(texModInfo_t, 0x44);
ASSERT_OFF(texModInfo_t, wave, 0x04);
ASSERT_OFF(texModInfo_t, matrix, 0x18);
ASSERT_OFF(texModInfo_t, translate, 0x28);
ASSERT_OFF(texModInfo_t, scale, 0x30);
ASSERT_OFF(texModInfo_t, scroll, 0x38);


int __cdecl RB_AddQuadStampExt( float *origin, float *left, float *up, int *color, float s1, float t1, float s2, float t2 );
extern int RB_ProjectionShadowDeform();
void VectorNormalizeFast( float *v );

float *__cdecl TableForFunc( int func );
int __cdecl myftol( float value );
float *__cdecl RB_CalcTransformTexCoords( texModInfo_t *tmi, float *st );
void RB_CalcTangentSpace( void );


/* ---- TableForFunc  0x005133D0 ----  VERIFIED */
float *__cdecl TableForFunc( int func )
{
	switch ( func )
	{
		case GF_SIN:              return tr_sinTable;
		case GF_TRIANGLE:         return tr_triangleTable;
		case GF_SQUARE:           return tr_squareTable;
		case GF_SAWTOOTH:         return tr_sawToothTable;
		case GF_INVERSE_SAWTOOTH: return tr_inverseSawToothTable;
		default:                  break;
	}

	ri.Error( 1, "\x15" "TableForFunc called with invalid function '%d' in shader '%s'\n", func, tess_shader );
	return NULL;
}

/* ---- EvalWaveForm  0x00513430 ----  VERIFIED */
rfloat __cdecl EvalWaveForm( const waveForm_t *wf )
{
	float *table;

	table = TableForFunc( wf->func );
	return WAVEVALUE( table, wf->base, wf->amplitude, wf->phase, wf->frequency );
}

/* ---- EvalWaveFormClamped  0x00513470 ----  VERIFIED */
rfloat __cdecl EvalWaveFormClamped( const waveForm_t *wf )
{
	rfloat glow;

	glow = EvalWaveForm( wf );
	if ( glow < 0 )
		return 0;
	if ( glow > 1 )
		return 1;
	return glow;
}

/* ---- RB_CalcStretchTexCoords  0x005134E0 ----  VERIFIED */
float *__cdecl RB_CalcStretchTexCoords( const waveForm_t *wf, float *st )
{
	rfloat       p;
	texModInfo_t tmi;

	p = 1.0f / EvalWaveForm( wf );

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	return RB_CalcTransformTexCoords( &tmi, st );
}

/* ---- RB_CalcDeformVertexes  0x00513570 ----  VERIFIED
 * Push each vertex along its normal by a wave value.  With frequency 0 the wave
 * is constant; otherwise the phase is spread by the summed position.  (CoD has
 * dropped the RTCW negative-frequency fire-rise branch.) */
void __cdecl RB_CalcDeformVertexes( deformStage_t *ds )
{
	int    i;
	vec3_t offset;
	float  scale;
	float *xyz    = (float *)tess_xyz;
	float *normal = (float *)tess_stageNormals;
	float *table;

	if ( ds->deformationWave.frequency == 0 )
	{
		scale = EvalWaveForm( &ds->deformationWave );

		for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3 )
		{
			VectorScale( normal, scale, offset );

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
	else
	{
		table = TableForFunc( ds->deformationWave.func );

		for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3 )
		{
			float off = ( xyz[2] + xyz[1] + xyz[0] ) * ds->deformationSpread;

			scale = WAVEVALUE( table, ds->deformationWave.base, ds->deformationWave.amplitude,
			                   ds->deformationWave.phase + off, ds->deformationWave.frequency );

			VectorScale( normal, scale, offset );

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
}

/* ---- RB_CalcFlapVertexes  0x005136E0 ----  VERIFIED
 * Like RB_CalcDeformVertexes, but the displacement is additionally scaled by one
 * texcoord component (S when component==0, T when component==1).  CoD-specific. */
void __cdecl RB_CalcFlapVertexes( int component, deformStage_t *ds )
{
	int    i;
	vec3_t offset;
	float  scale;
	float *xyz    = (float *)tess_xyz;
	float *st     = (float *)tess_texCoords0 + component;
	float *normal = (float *)tess_stageNormals;
	float *table;

	if ( ds->deformationWave.frequency == 0 )
	{
		scale = EvalWaveForm( &ds->deformationWave );

		for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3, st += 2 )
		{
			VectorScale( normal, scale * st[0], offset );

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
	else
	{
		table = TableForFunc( ds->deformationWave.func );

		for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3, st += 2 )
		{
			float off = ( xyz[0] + xyz[2] + xyz[1] ) * ds->deformationSpread;

			scale = WAVEVALUE( table, ds->deformationWave.base, ds->deformationWave.amplitude,
			                   ds->deformationWave.phase + off, ds->deformationWave.frequency );

			VectorScale( normal, scale * st[0], offset );

			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
}

/* ---- RB_CalcDeformSyncNormals  0x00513870 ----  VERIFIED
 * Perturb each normal by the wave derivative (table[i+1]-table[i]) scaled by the
 * summed position, then renormalize.  CoD-specific normal deform. */
void __cdecl RB_CalcDeformSyncNormals( deformStage_t *ds )
{
	int    i;
	int    idx;
	float *xyz    = (float *)tess_xyz;
	float *normal = (float *)tess_stageNormals;
	float *table;
	vec3_t perp;
	rfloat sum, scale;

	table = TableForFunc( ds->deformationWave.func );

	for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3 )
	{
		float off;

		/* explicit grouping: VC7 then keeps retail's n1 + n2 + n0 and amp, 1024, spread order */
		sum = ( normal[2] + normal[1] ) + normal[0];
		perp[0] = 1.0f - sum * normal[0];
		perp[1] = 1.0f - sum * normal[1];
		perp[2] = 1.0f - sum * normal[2];

		off = ( xyz[2] + xyz[1] + xyz[0] ) * ds->deformationSpread;
		idx = myftol( ( ( ds->deformationWave.phase + off ) + tess_shaderTime * ( ds->deformationWave.frequency ) ) * FUNCTABLE_SIZE );
		scale = -( ( ( ( table[ ( idx + 1 ) & FUNCTABLE_MASK ] - table[ idx & FUNCTABLE_MASK ] )
		               * ds->deformationWave.amplitude ) * FUNCTABLE_SIZE ) * ds->deformationSpread );

		normal[0] += perp[0] * scale;
		normal[1] += perp[1] * scale;
		normal[2] += perp[2] * scale;
		VectorNormalizeFast( normal );
	}
	tess_stageTangentsValid   = 0;
	tess_stageBitangentsValid = 0;
}

/* ---- RB_CalcDeformNormals  0x00513990 ----  VERIFIED
 * Wiggle the normals with 4D noise for wavy environment mapping. */
void __cdecl RB_CalcDeformNormals( deformStage_t *ds )
{
	int    i;
	rfloat scale;
	float *xyz    = (float *)tess_xyz;
	float *normal = (float *)tess_stageNormals;

	for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3 )
	{
		scale = 0.98f;
		scale = Com_NoiseGet4f( xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
		                        tess_shaderTime * ds->deformationWave.frequency );
		normal[0] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = Com_NoiseGet4f( 100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
		                        tess_shaderTime * ds->deformationWave.frequency );
		normal[1] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = Com_NoiseGet4f( 200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
		                        tess_shaderTime * ds->deformationWave.frequency );
		normal[2] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast( normal );
	}
	tess_stageTangentsValid   = 0;
	tess_stageBitangentsValid = 0;
}

/* ---- RB_CalcBulgeVertexes  0x00513AD0 ----  [HIGH]
 * Bulge each vertex along its normal by a time-scrolled sine of its S texcoord. */
void __cdecl RB_CalcBulgeVertexes( deformStage_t *ds )
{
	int          i;
	const float *st     = (const float *)tess_texCoords0;
	float       *xyz    = (float *)tess_xyz;
	float       *normal = (float *)tess_stageNormals;
	float        now;   /* retail fstp m32 (0x513AFA), fadd m32 per vertex */

	now = (rfloat)backEnd_refdef_time * ds->bulgeSpeed * 0.001f;

	for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, st += 2, normal += 3 )
	{
		int   off;
		float scale;

		/* 162.97466f = (float)( FUNCTABLE_SIZE / ( M_PI * 2 ) ) */
		off = (int)( (rfloat)( ds->bulgeWidth * st[0] + now ) * 162.9746551513671875f );

		scale = tr_sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;

		xyz[0] += normal[0] * scale;
		xyz[1] += normal[1] * scale;
		xyz[2] += normal[2] * scale;
	}
}

/* ---- RB_CalcMoveVertexes  0x00513B70 ----  [HIGH]
 * Move the whole surface along a wave path (moveVector * wave value). */
float *__cdecl RB_CalcMoveVertexes( deformStage_t *ds )
{
	float *table;
	float *xyz;
	float  scale;
	float  offset[3];
	int    i;

	table = TableForFunc( ds->deformationWave.func );
	scale = WAVEVALUE( table, ds->deformationWave.base, ds->deformationWave.amplitude,
	                   ds->deformationWave.phase, ds->deformationWave.frequency );

	offset[0] = scale * ds->moveVector[0];
	offset[1] = scale * ds->moveVector[1];
	offset[2] = scale * ds->moveVector[2];

	xyz = (float *)tess_xyz;
	for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount )
	{
		xyz[0] += offset[0];
		xyz[1] += offset[1];
		xyz[2] += offset[2];
	}
	return xyz;   /* dead EAX (final xyz cursor) */
}

/* ---- DeformText  0x00513C20 ----  VERIFIED
 * Replace one quad with a row of text quads, one per character.  width comes
 * from crossing the surface normal with a down vector; height is half the quad
 * Z extent; the row is centred on the quad midpoint. */
void __cdecl DeformText( const char *text )
{
	float origin[3];        /* var_C /var_8 /var_4 */
	float width[3];         /* var_18/var_14/var_10 */
	float height[3];        /* var_24/var_20/var_1C  holds mid first, height after */
	float step[3];
	unsigned char color[4];
	float bottom, top;
	float t0, f;
	const float *xyz;
	int   len, i, ch;
	int   stride;
	float fcol, frow;

	{const float down[3]={0.0f,0.0f,-1.0f};CrossProduct(tess_stageNormals,down,width);}
	stride = tess_vertexComponentCount;
	height[0] = tess_xyz[0];
	height[1] = tess_xyz[1];
	height[2] = tess_xyz[2];
	bottom = 999999.0f;
	top    = -999999.0f;
	if ( tess_xyz[2] < 999999.0f )
		bottom = tess_xyz[2];
	if ( tess_xyz[2] > -999999.0f )
		top = tess_xyz[2];

	xyz = tess_xyz;
	for ( i = 1; i < 4; i++ )
	{
		xyz += stride;
		height[0] += xyz[0];
		height[1] += xyz[1];
		height[2] += xyz[2];
		if ( xyz[2] < bottom )
			bottom = xyz[2];
		if ( xyz[2] > top )
			top = xyz[2];
	}

	origin[0] = height[0] * 0.25f;
	height[0] = 0.0f;
	origin[1] = height[1] * 0.25f;
	height[1] = 0.0f;
	origin[2] = height[2] * 0.25f;
	height[2] = ( top - bottom ) * 0.5f;

	width[0] = width[0] * ( height[2] * -0.75f );   /* VectorScale( width, height[2] * -0.75f, width ) */
	width[1] = width[1] * ( height[2] * -0.75f );
	width[2] = width[2] * ( height[2] * -0.75f );

	len = strlen( text );
	f   = (float)( len - 1 );

	tess_numIndexes = 0;
	tess_numVertexes = 0;

	origin[0] = f * width[0] + origin[0];
	origin[1] = f * width[1] + origin[1];
	origin[2] = f * width[2] + origin[2];

	color[0] = color[1] = color[2] = color[3] = 0xFF;

	if ( len > 0 )
	{
		step[0] = width[0] + width[0];
		step[1] = width[1] + width[1];
		step[2] = width[2] + width[2];

		for ( i = 0; i < len; i++ )
		{
			ch = (unsigned char)text[i];
			if ( ch != 32 )
			{
				frow = (float)( ch >> 4 ) * 0.0625f;
				fcol = (float)( ch & 0xF ) * 0.0625f;
				RB_AddQuadStampExt( origin, width, height, (int *)color,
				                    fcol, frow, fcol + 0.0625f, frow + 0.0625f );
			}
			origin[0] = origin[0] - step[0];
			origin[1] = origin[1] - step[1];
			origin[2] = origin[2] - step[2];
		}
	}
}

/* ---- GlobalPositionToLocal  0x00513F90 ----  VERIFIED
 * Transform a world-space point into backEnd.or local space. */
float *__cdecl GlobalPositionToLocal( float *world, float *out )
{
	float dx = world[0] - backEnd_or.origin[0];
	float dy = world[1] - backEnd_or.origin[1];
	float dz = world[2] - backEnd_or.origin[2];

	out[0] = backEnd_or.axis[0][2] * dz + backEnd_or.axis[0][1] * dy + backEnd_or.axis[0][0] * dx;
	out[1] = backEnd_or.axis[1][2] * dz + backEnd_or.axis[1][1] * dy + backEnd_or.axis[1][0] * dx;
	out[2] = backEnd_or.axis[2][2] * dz + backEnd_or.axis[2][1] * dy + backEnd_or.axis[2][0] * dx;
	return out;
}

/* ---- GlobalVectorToLocal  0x00514010 ----  VERIFIED
 * Rotate a world-space vector into backEnd.or local space (no translation). */
float *__cdecl GlobalVectorToLocal( float *in, float *out )
{
#ifdef COD1_VC7
	out[0] = DotProduct( in, backEnd_or.axis[0] );
	out[1] = DotProduct( in, backEnd_or.axis[1] );
	out[2] = DotProduct( in, backEnd_or.axis[2] );
#else
	/* VC7 emits DotProduct in this summation order (0, 2, 1) */
	out[0] = backEnd_or.axis[0][0] * in[0] + backEnd_or.axis[0][2] * in[2] + backEnd_or.axis[0][1] * in[1];
	out[1] = backEnd_or.axis[1][0] * in[0] + backEnd_or.axis[1][2] * in[2] + backEnd_or.axis[1][1] * in[1];
	out[2] = backEnd_or.axis[2][0] * in[0] + backEnd_or.axis[2][2] * in[2] + backEnd_or.axis[2][1] * in[1];
#endif
	return in;
}

/* ---- AutospriteDeform  0x00514080 ----  VERIFIED
 * Rebuild each independent quad as a forward-facing camera sprite. */
void AutospriteDeform( void )
{
	int    i;
	int    oldVerts;
	float *xyz;
	vec3_t mid;
	float  delta[3];
	float  radius;
	vec3_t left, up;
	vec3_t leftDir, upDir;

	if ( tess_numVertexes & 3 )
		ri.Printf( 2, "Autosprite shader %s had odd vertex count\n", (const char *)tess_shader );
	if ( tess_numIndexes != ( tess_numVertexes >> 2 ) * 6 )
		ri.Printf( 2, "Autosprite shader %s had odd index count\n", (const char *)tess_shader );

	oldVerts = tess_numVertexes;
	tess_numVertexes = 0;
	tess_numIndexes = 0;

	if ( (int *)backEnd_currentEntity != &tr_worldEntity )
	{
		GlobalVectorToLocal( &backEnd_viewParms_axis10, leftDir );
		GlobalVectorToLocal( &backEnd_viewParms_axis20, upDir );
	}
	else
	{
		VectorCopy( &backEnd_viewParms_axis10, leftDir );
		VectorCopy( &backEnd_viewParms_axis20, upDir );
	}

	xyz = tess_xyz;
	for ( i = 0; i < oldVerts; i += 4, xyz += 4 * tess_vertexComponentCount )
	{
		/* find the midpoint (sums in the order VC7 emits them, so the modern build adds alike) */
		mid[0] = ( xyz[2 * tess_vertexComponentCount] + xyz[tess_vertexComponentCount] + xyz[0] + xyz[3 * tess_vertexComponentCount] ) * 0.25f;
		mid[1] = ( xyz[tess_vertexComponentCount + 1] + xyz[2 * tess_vertexComponentCount + 1] + xyz[3 * tess_vertexComponentCount + 1] + xyz[1] ) * 0.25f;
		mid[2] = ( xyz[tess_vertexComponentCount + 2] + xyz[2 * tess_vertexComponentCount + 2] + xyz[3 * tess_vertexComponentCount + 2] + xyz[2] ) * 0.25f;

		VectorSubtract( xyz, mid, delta );
		radius = FCAST( sqrt( delta[2] * delta[2] + delta[1] * delta[1] + delta[0] * delta[0] ) ) * 0.707f;   /* / sqrt(2) */

		VectorScale( leftDir, radius, left );
		VectorScale( upDir, radius, up );

		if ( backEnd_viewParms_isMirror )
			VectorSubtract( vec3_origin, left, left );

		/* compensate for scale in the axes if necessary */
		if ( *(float *)( backEnd_currentEntity + 0x40 ) )            /* e.nonNormalizedAxes */
		{
			float *axis0 = (float *)( backEnd_currentEntity + 0x1C ); /* e.axis[0] */
			float  axisLength;

			axisLength = FCAST( sqrt( axis0[0] * axis0[0] + axis0[1] * axis0[1] + axis0[2] * axis0[2] ) );
			if ( !axisLength )
				axisLength = 0;
			else
				axisLength = 1.0f / axisLength;
			VectorScale( left, axisLength, left );
			VectorScale( up, axisLength, up );
		}

		RB_AddQuadStampExt( mid, left, up, (int *)&tess_vertexColors[i * 4], 0, 0, 1, 1 );
	}
}

/* one candidate edge of the quad's two triangles, between the vertices of
 * index-buffer entries base+ia and base+ib, for Autosprite2Deform's
 * two-shortest search (retail: a loop over an edge table, fully unrolled; the
 * first test against FLT_MAX is kept) */
#define AS2_EDGE( ia, ib ) \
	{ \
		float *from_ = &tess_xyz[tess_vertexComponentCount * tess_indexes[base + (ia)]]; \
		float *to_   = &tess_xyz[tess_vertexComponentCount * tess_indexes[base + (ib)]]; \
		edge[0] = to_[0] - from_[0]; \
		edge[1] = to_[1] - from_[1]; \
		edge[2] = to_[2] - from_[2]; \
		len = edge[2] * edge[2] + edge[1] * edge[1] + edge[0] * edge[0]; \
		if ( len < shortLen )       { secondLen = shortLen; tp0 = sp0; tp1 = sp1; shortLen = len; sp0 = from_; sp1 = to_; } \
		else if ( len < secondLen ) { secondLen = len; tp0 = from_; tp1 = to_; } \
	}

/* ---- Autosprite2Deform  0x00514330 ----  [HIGH]
 * Pivot each quad about its long axis to face the camera.  Of the six triangle
 * edges the two shortest are found; each is re-projected along the minor axis
 * (its perpendicular in the view plane) by half its length. */
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", on )
#endif
void Autosprite2Deform(void)
{
	float  forward[3];
	float  minor[3];
	float  major[3];
	float  midShort[3], midSecond[3];
	float *sp0, *sp1;    /* shortest edge endpoints      (retail v4, v2) */
	float *tp0, *tp1;    /* second-shortest edge endpoints (retail v3, v65) */
	float  shortLen, secondLen;
	float  halfShort, halfSecond;
	float  edge[3];
	float  len;
	int    base;
	int    e;
	static const unsigned char edges[6][2] = {{0,1},{1,2},{2,0},{3,4},{4,5},{5,3}};

	if ( ( tess_numVertexes & 3 ) != 0 )
		ri.Printf( 2, "Autosprite2 shader %s had odd vertex count\n", (const char *)tess_shader );
	if ( tess_numIndexes != 6 * ( tess_numVertexes >> 2 ) )
		ri.Printf( 2, "Autosprite2 shader %s had odd index count\n", (const char *)tess_shader );

	if ( (int *)backEnd_currentEntity != &tr_worldEntity )
	{
		GlobalVectorToLocal( &backEnd_viewParms_axis00, forward );
	}
	else
	{
		forward[0] = backEnd_viewParms_axis00;
		forward[1] = backEnd_viewParms_axis01;
		forward[2] = backEnd_viewParms_axis02;
	}

	for ( base = 0; base < tess_numIndexes; base += 6 )
	{
		/* the two shortest of the six triangle edges */
		shortLen = secondLen = 3.4028234663852885981e38f;
		for (e=0;e<6;++e)
		{
			AS2_EDGE(edges[e][0],edges[e][1]);
		}

		midShort[0]  = ( sp1[0] + sp0[0] ) * 0.5f;
		midShort[1]  = ( sp0[1] + sp1[1] ) * 0.5f;
		midShort[2]  = ( sp0[2] + sp1[2] ) * 0.5f;
		midSecond[0] = ( tp1[0] + tp0[0] ) * 0.5f;
		midSecond[1] = ( tp1[1] + tp0[1] ) * 0.5f;
		midSecond[2] = ( tp1[2] + tp0[2] ) * 0.5f;

		major[0] = midSecond[0] - midShort[0];
		major[1] = midSecond[1] - midShort[1];
		major[2] = midSecond[2] - midShort[2];

		minor[0] = major[1] * forward[2] - major[2] * forward[1];
		minor[1] = major[2] * forward[0] - major[0] * forward[2];
		minor[2] = major[0] * forward[1] - major[1] * forward[0];
		VectorNormalize( minor );

		halfShort = sqrt( shortLen ) * 0.5f;
		sp0[0] = -halfShort * minor[0] + midShort[0];
		sp0[1] = -halfShort * minor[1] + midShort[1];
		sp0[2] = -halfShort * minor[2] + midShort[2];
		sp1[0] = minor[0] * halfShort + midShort[0];
		sp1[1] = minor[1] * halfShort + midShort[1];
		sp1[2] = minor[2] * halfShort + midShort[2];

		halfSecond = sqrt( secondLen ) * 0.5f;
		tp0[0] = minor[0] * halfSecond + midSecond[0];
		tp0[1] = minor[1] * halfSecond + midSecond[1];
		tp0[2] = minor[2] * halfSecond + midSecond[2];
		tp1[0] = -halfSecond * minor[0] + midSecond[0];
		tp1[1] = -halfSecond * minor[1] + midSecond[1];
		tp1[2] = -halfSecond * minor[2] + midSecond[2];
	}
	return;
}
#if defined( _MSC_VER ) && _MSC_VER < 1310
#pragma optimize( "p", off )
#endif

/* ---- RB_DeformTessGeometry  0x005149B0 ----  VERIFIED
 * Run every deform of the current shader over the tessellated geometry. */
int __cdecl RB_DeformTessGeometry( void )
{
	deformStage_t *ds;
	int i;

	for ( i = 0; i < *(int *)( tess_shader + 172 ); i++ )   /* shader.numDeforms */
	{
		ds = (deformStage_t *)( tess_shader + 176 + 52 * i );   /* shader.deforms[i] */
		switch ( ds->deformation )
		{
			case DEFORM_NONE:                                                  break;
			case DEFORM_NORMALS:           RB_CalcDeformNormals( ds );         break;
			case DEFORM_SYNC_NORMALS:      RB_CalcDeformSyncNormals( ds );     break;
			case DEFORM_WAVE:              RB_CalcDeformVertexes( ds );        break;
			case DEFORM_FLAP_S:            RB_CalcFlapVertexes( 0, ds );       break;
			case DEFORM_FLAP_T:            RB_CalcFlapVertexes( 1, ds );       break;
			case DEFORM_BULGE:             RB_CalcBulgeVertexes( ds );         break;
			case DEFORM_MOVE:              RB_CalcMoveVertexes( ds );          break;
			case DEFORM_PROJECTION_SHADOW: RB_ProjectionShadowDeform();        break;
			case DEFORM_AUTOSPRITE:        AutospriteDeform();                 break;
			case DEFORM_AUTOSPRITE2:       Autosprite2Deform();                break;
			case DEFORM_TEXT0:
			case DEFORM_TEXT1:
			case DEFORM_TEXT2:
			case DEFORM_TEXT3:
			case DEFORM_TEXT4:
			case DEFORM_TEXT5:
			case DEFORM_TEXT6:
			case DEFORM_TEXT7:
				DeformText( ( (char (*)[32])backEnd_refdef_text )[ ds->deformation - DEFORM_TEXT0 ] );
				break;
		}
	}
	return tess_shader;   /* dead EAX */
}

/* ---- RB_CalcColorFromEntity  0x00514AC0 ----  [HIGH] */
void __fastcall RB_CalcColorFromEntity( unsigned char *dstColors )
{
	int  i;
	int *pColors = ( int * ) dstColors;
	int  c;

	if ( !backEnd_currentEntity )
		return;

	c = *( int * )( backEnd_currentEntity + 108 );   /* e.shaderRGBA */

	for ( i = 0; i < tess_numVertexes; i++, pColors++ )
		*pColors = c;
}

/* ---- RB_CalcColorFromOneMinusEntity  0x00514B00 ----  [HIGH] */
void __fastcall RB_CalcColorFromOneMinusEntity( unsigned char *dstColors )
{
	int           i;
	int          *pColors = ( int * ) dstColors;
	unsigned char invModulate[4];

	if ( !backEnd_currentEntity )
		return;

	invModulate[0] = 255 - *( unsigned char * )( backEnd_currentEntity + 108 );
	invModulate[1] = 255 - *( unsigned char * )( backEnd_currentEntity + 109 );
	invModulate[2] = 255 - *( unsigned char * )( backEnd_currentEntity + 110 );
	invModulate[3] = 255 - *( unsigned char * )( backEnd_currentEntity + 111 );
	for ( i = 0; i < tess_numVertexes; i++, pColors++ )
		*pColors = *( int * )invModulate;
}

/* ---- RB_CalcAlphaFromEntity  0x00514B70 ----  [HIGH] */
void __cdecl RB_CalcAlphaFromEntity( unsigned char *dstColors )
{
	int i;

	if ( !backEnd_currentEntity )
		return;

	dstColors += 3;

	for ( i = 0; i < tess_numVertexes; i++, dstColors += 4 )
		*dstColors = *( unsigned char * )( backEnd_currentEntity + 111 );   /* e.shaderRGBA[3] */
}

/* ---- RB_CalcAlphaFromOneMinusEntity  0x00514BB0 ----  [HIGH] */
void __cdecl RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors )
{
	int i;

	if ( !backEnd_currentEntity )
		return;

	dstColors += 3;

	for ( i = 0; i < tess_numVertexes; i++, dstColors += 4 )
		*dstColors = 0xff - *( unsigned char * )( backEnd_currentEntity + 111 );
}

/* ---- RB_CalcWaveColor  0x00514C00 ----  VERIFIED
 * Fill every vertex colour with a shared grey level driven by the waveform
 * (noise or table-driven), scaled by identityLight and clamped to [0,1]. */
int __cdecl RB_CalcWaveColor( unsigned char *dstColors, const waveForm_t *wf )
{
	int           i;
	int           v;
	rfloat        glow;
	int          *colors = ( int * ) dstColors;
	unsigned char color[4];

	if ( wf->func == GF_NOISE )
		glow = wf->base + Com_NoiseGet4f( 0.0f, 0.0f, 0.0f, ( tess_shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
	else
		glow = EvalWaveForm( wf ) * tr_identityLight;

	if ( glow < 0 )
		glow = 0;
	else if ( glow > 1 )
		glow = 1;

	v = myftol( 255 * glow );
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v = *( int * )color;

	for ( i = 0; i < tess_numVertexes; i++, colors++ )
		*colors = v;
	return i;
}

/* ---- RB_CalcWaveAlpha  0x00514D00 ----  VERIFIED */
int __cdecl RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors )
{
	int    i;
	int    v;
	rfloat glow;

	glow = EvalWaveFormClamped( wf );
	v = (int)( 255 * glow );
	for ( i = 0; i < tess_numVertexes; i++, dstColors += 4 )
		dstColors[3] = v;
	return v;   /* dead EAX (shared alpha byte) */
}

/* ---- RB_CalcEnvironmentTexCoords  0x00514D40 ----  [HIGH]
 * Sphere-map environment coords from the reflected view vector. */
void __cdecl RB_CalcEnvironmentTexCoords( float *st )
{
	int    i;
	float *v, *normal;
	vec3_t viewer, reflected;
	float  d;

	v = (float *)tess_xyz;
	normal = (float *)tess_stageNormals;

	for ( i = 0; i < tess_numVertexes; i++, v += tess_vertexComponentCount, normal += 3, st += 2 )
	{
		VectorSubtract( backEnd_or.viewOrigin, v, viewer );
		VectorNormalizeFast( viewer );

		d = DotProduct( normal, viewer );

		reflected[0] = normal[0] * 2 * d - viewer[0];
		reflected[1] = normal[1] * 2 * d - viewer[1];
		reflected[2] = normal[2] * 2 * d - viewer[2];

		st[0] = ( reflected[1] + 1.0f ) * 0.5f;
		st[1] = 0.5f - reflected[2] * 0.5f;
	}
}

/* ---- RB_CalcCubeMapEyeToVertexTexCoords  0x00514E10 ----  VERIFIED
 * Cube-map coords along the eye-to-vertex direction (vertex - eye). */
void __fastcall RB_CalcCubeMapEyeToVertexTexCoords( float *st )
{
	float *xyz;
	int    i;

	xyz = (float *)tess_xyz;
	for ( i = 0; i < tess_numVertexes; i++, st += 3, xyz += tess_vertexComponentCount )
		VectorSubtract( xyz, backEnd_or.viewOrigin, st );
}

/* ---- RB_CalcCubeMapVertexToEyeTexCoords  0x00514E60 ----  [HIGH]
 * Cube-map coords along the vertex-to-eye direction (eye - vertex). */
void __fastcall RB_CalcCubeMapVertexToEyeTexCoords( float *st )
{
	float *xyz;
	int    i;

	xyz = (float *)tess_xyz;
	for ( i = 0; i < tess_numVertexes; i++, st += 3, xyz += tess_vertexComponentCount )
		VectorSubtract( backEnd_or.viewOrigin, xyz, st );
}

/* ---- RB_CalcCubeMapReflectionTexCoords  0x00514EB0 ----  [HIGH]
 * Cube-map coords along the view vector reflected about the surface normal. */
void __cdecl RB_CalcCubeMapReflectionTexCoords( float *st )
{
	int    i;
	float *xyz, *normal;
	vec3_t viewer, reflected;
	float  d;

	xyz = (float *)tess_xyz;
	normal = (float *)tess_stageNormals;
	for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3, st += 3 )
	{
		VectorSubtract( backEnd_or.viewOrigin, xyz, viewer );
		d = DotProduct( viewer, normal );
		VectorScale( normal, 2 * d, reflected );
		VectorSubtract( reflected, viewer, st );
	}
}

/* ---- RB_CalcCubeMapLightVectorTexCoords  0x00514F60 ----  VERIFIED
 * Cube-map coords along the current light vector in local space. */
void __fastcall RB_CalcCubeMapLightVectorTexCoords( float *st )
{
	dlight_t *dl = (dlight_t *)backEnd_currentDlight;
	float    *xyz;
	vec3_t    lv;
	vec3_t    local;
	int       i;

	if ( dl->position[3] != 0 )
		VectorSubtract( dl->position, backEnd_or.origin, lv );
	else
		VectorCopy( dl->position, lv );

	local[0] = backEnd_or.axis[0][2] * lv[2] + backEnd_or.axis[0][1] * lv[1] + backEnd_or.axis[0][0] * lv[0];
	local[1] = backEnd_or.axis[1][2] * lv[2] + backEnd_or.axis[1][1] * lv[1] + backEnd_or.axis[1][0] * lv[0];
	local[2] = backEnd_or.axis[2][2] * lv[2] + backEnd_or.axis[2][1] * lv[1] + backEnd_or.axis[2][0] * lv[0];

	xyz = (float *)tess_xyz;
	for ( i = 0; i < tess_numVertexes; i++, st += 3, xyz += tess_vertexComponentCount )
		VectorSubtract( local, xyz, st );
}

/* ---- RB_CalcCubeMapLightHalfAngleTexCoords  0x00515050 ----  VERIFIED
 * Cube-map coords along the half-angle vector (normalized light + eye). */
void __cdecl RB_CalcCubeMapLightHalfAngleTexCoords( float *st )
{
	dlight_t *dl = (dlight_t *)backEnd_currentDlight;
	float    *xyz;
	vec3_t    lv;
	vec3_t    local;
	vec3_t    light, eye;
	int       i;

	if ( dl->position[3] != 0 )
		VectorSubtract( dl->position, backEnd_or.origin, lv );
	else
		VectorCopy( dl->position, lv );

	local[0] = backEnd_or.axis[0][2] * lv[2] + backEnd_or.axis[0][1] * lv[1] + backEnd_or.axis[0][0] * lv[0];
	local[1] = backEnd_or.axis[1][2] * lv[2] + backEnd_or.axis[1][1] * lv[1] + backEnd_or.axis[1][0] * lv[0];
	local[2] = backEnd_or.axis[2][2] * lv[2] + backEnd_or.axis[2][1] * lv[1] + backEnd_or.axis[2][0] * lv[0];

	xyz = (float *)tess_xyz;
	for ( i = 0; i < tess_numVertexes; i++, st += 3, xyz += tess_vertexComponentCount )
	{
		VectorSubtract( backEnd_or.viewOrigin, xyz, eye );
		VectorNormalizeFast( eye );
		VectorSubtract( local, xyz, light );
		VectorNormalizeFast( light );
		VectorAdd( light, eye, st );
	}
}

/* ---- RB_CalcCubeMapSunHalfAngleTexCoords  0x005151B0 ----  [HIGH]
 * Cube-map coords along the half-angle vector for the (directional) sun. */
void __cdecl RB_CalcCubeMapSunHalfAngleTexCoords( float *st )
{
	int    i;
	float *xyz;
	vec3_t eye;

	xyz = (float *)tess_xyz;
	for ( i = 0; i < tess_numVertexes; i++, st += 3, xyz += tess_vertexComponentCount )
	{
		VectorSubtract( backEnd_or.viewOrigin, xyz, eye );
		VectorNormalizeFast( eye );
		VectorAdd( tr_sunDirection, eye, st );
	}
}

/* ---- RB_CalcCubeMapTbnTexCoords  0x00515250 ----  VERIFIED
 * Pass the per-vertex tangent/bitangent/normal basis through as three coords. */
void __cdecl RB_CalcCubeMapTbnTexCoords( float *out, int startIndex )
{
	int i;

	if ( !tess_stageTangentsValid || !tess_stageBitangentsValid )
		RB_CalcTangentSpace();

	for ( i = 0; i < tess_numVertexes; i++, out += 3 )
	{
		out[0] = tess_stageTangents[ i * 3 + startIndex ];
		out[1] = tess_stageBitangents[ i * 3 + startIndex ];
		out[2] = tess_stageNormals[ i * 3 + startIndex ];
	}
}

/* ---- RB_CalcCubeMapDot3ReflectTexCoords  0x005152B0 ----  VERIFIED
 * Emit the tangent/bitangent/normal basis plus the eye-X delta in the 4th coord. */
void __cdecl RB_CalcCubeMapDot3ReflectTexCoords( float *out, int startIndex )
{
	int i;

	if ( !tess_stageTangentsValid || !tess_stageBitangentsValid )
		RB_CalcTangentSpace();

	for ( i = 0; i < tess_numVertexes; i++, out += 4 )
	{
		out[0] = tess_stageTangents[ i * 3 + startIndex ];
		out[1] = tess_stageBitangents[ i * 3 + startIndex ];
		out[2] = tess_stageNormals[ i * 3 + startIndex ];
		out[3] = backEnd_viewParms_originX[ startIndex ] - tess_xyz[ i * 3 + startIndex ];
	}
}

/* ---- RB_CalcSwapTexCoords  0x00515320 ----  [HIGH] */
void __cdecl RB_CalcSwapTexCoords( float *st )
{
	int i;

	for ( i = 0; i < tess_numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = t;
		st[1] = 1.0f - s;
	}
}

/* ---- RB_CalcTurbulentTexCoords  0x00515360 ----  [HIGH] */
void __cdecl RB_CalcTurbulentTexCoords( float *st, waveForm_t *wf )
{
	int    i;
	float  now;
	const float *xyz;

	now = ( wf->phase + tess_shaderTime * wf->frequency );

	xyz = (float *)tess_xyz;
	for ( i = 0; i < tess_numVertexes; i++, st += 2, xyz += tess_vertexComponentCount )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s + tr_sinTable[ ( (int)( ( ( xyz[0] + xyz[2] ) * 1.0 / 128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
		st[1] = t + tr_sinTable[ ( (int)( ( xyz[1] * 1.0 / 128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
	}
}

/* ---- RB_CalcScaleTexCoords  0x00515410 ----  VERIFIED */
void __cdecl RB_CalcScaleTexCoords( const float *scale, float *st )
{
	int i;

	for ( i = 0; i < tess_numVertexes; i++, st += 2 )
	{
		st[0] = scale[0] * st[0];
		st[1] = scale[1] * st[1];
	}
}

/* ---- RB_CalcScrollTexCoords  0x00515440 ----  [HIGH] */
void __cdecl RB_CalcScrollTexCoords( const float *scrollSpeed, float *st )
{
	int   i;
	float timeScale = tess_shaderTime;
	float adjustedScrollS, adjustedScrollT;

	adjustedScrollS = scrollSpeed[0] * timeScale;
	adjustedScrollT = scrollSpeed[1] * timeScale;

	adjustedScrollS = adjustedScrollS - (float)floor( adjustedScrollS );
	adjustedScrollT = adjustedScrollT - (float)floor( adjustedScrollT );

	for ( i = 0; i < tess_numVertexes; i++, st += 2 )
	{
		st[0] += adjustedScrollS;
		st[1] += adjustedScrollT;
	}
}

/* ---- RB_CalcTransformTexCoords  0x005154C0 ----  VERIFIED */
float *__cdecl RB_CalcTransformTexCoords( texModInfo_t *tmi, float *st )
{
	int   i;
	float s, t;

	for ( i = 0; i < tess_numVertexes; i++, st += 2 )
	{
		s = st[0];
		t = st[1];
		st[0] = t * tmi->matrix[1][0] + s * tmi->matrix[0][0] + tmi->translate[0];
		st[1] = t * tmi->matrix[1][1] + s * tmi->matrix[0][1] + tmi->translate[1];
	}
	return (float *)tmi;
}

/* ---- RB_CalcRotateTexCoords  0x00515510 ----  [HIGH] */
float *__cdecl RB_CalcRotateTexCoords( float degsPerSecond, float *st )
{
	rfloat       timeScale = tess_shaderTime;
	rfloat       degs;
	int          index;
	rfloat       sinValue, cosValue;
	texModInfo_t tmi;

	degs = -degsPerSecond * timeScale;
	index = (int)( degs * ( FUNCTABLE_SIZE / 360.0f ) );

	sinValue = tr_sinTable[ index & FUNCTABLE_MASK ];
	cosValue = tr_sinTable[ ( index + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK ];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5f - 0.5f * cosValue + 0.5f * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5f - 0.5f * sinValue - 0.5f * cosValue;

	return RB_CalcTransformTexCoords( &tmi, st );
}

/* ---- RB_CalcCubeMapNegateTexCoords  0x005155B0 ----  VERIFIED
 * Negate every component of the 3-float cube-map coords in place. */
float *__cdecl RB_CalcCubeMapNegateTexCoords( float *st )
{
	int count = 3 * tess_numVertexes;

	if ( count > 0 )
	{
		do
		{
			*st = -*st;
			++st;
			--count;
		}
		while ( count );
	}
	return st;
}

/* ---- GetSTVectors  0x005155D0 ----  VERIFIED
 * S-tangent of a triangle: the vertex positions weighted by their T deltas.
 * bitangent is never written, but retail's caller still pushes it. */
void __cdecl GetSTVectors( int i0, int i1, int i2, float *tangent, float *bitangent )
{
	rfloat d0, d1, d2;
	int    b0, b1, b2;

	d0 = tess_texCoords0[2 * i1 + 1] - tess_texCoords0[2 * i2 + 1];
	d1 = tess_texCoords0[2 * i2 + 1] - tess_texCoords0[2 * i0 + 1];
	d2 = tess_texCoords0[2 * i0 + 1] - tess_texCoords0[2 * i1 + 1];
	b0 = i0 * tess_vertexComponentCount;
	b1 = i1 * tess_vertexComponentCount;
	b2 = i2 * tess_vertexComponentCount;

	tangent[0] = d0 * tess_xyz[b0]     + d1 * tess_xyz[b1]     + d2 * tess_xyz[b2];
	tangent[1] = d0 * tess_xyz[b0 + 1] + d1 * tess_xyz[b1 + 1] + d2 * tess_xyz[b2 + 1];
	tangent[2] = d0 * tess_xyz[b0 + 2] + d1 * tess_xyz[b1 + 2] + d2 * tess_xyz[b2 + 2];
	VectorNormalizeFast( tangent );
}

/* ---- RB_CalcTangentSpace  0x00515680 ----  [HIGH]
 * Accumulate a per-vertex tangent frame from the triangles, normalize the
 * tangents shared by more than one triangle, then set the bitangent to
 * normal x tangent. */
void RB_CalcTangentSpace( void )
{
	vec3_t tangent;
	int    counts[8192];
	int    i;
	int    i0, i1, i2;

	memset( tess_stageTangents, 0, 12 * tess_numVertexes );
	memset( tess_stageBitangents, 0, 12 * tess_numVertexes );
	memset( counts, 0, 4 * tess_numVertexes );

	for ( i = 0; i < tess_numIndexes; i += 3 )
	{
		i0 = tess_indexes[i];
		i1 = tess_indexes[i + 1];
		i2 = tess_indexes[i + 2];
		GetSTVectors( i0, i1, i2, tangent, tangent );
		counts[i0]++;
		counts[i1]++;
		counts[i2]++;
		tess_stageTangents[3 * i0 + 0] += tangent[0];
		tess_stageTangents[3 * i0 + 1] += tangent[1];
		tess_stageTangents[3 * i0 + 2] += tangent[2];
		tess_stageTangents[3 * i1 + 0] += tangent[0];
		tess_stageTangents[3 * i1 + 1] += tangent[1];
		tess_stageTangents[3 * i1 + 2] += tangent[2];
		tess_stageTangents[3 * i2 + 0] += tangent[0];
		tess_stageTangents[3 * i2 + 1] += tangent[1];
		tess_stageTangents[3 * i2 + 2] += tangent[2];
	}

	for ( i = 0; i < tess_numVertexes; i++ )
	{
		if ( counts[i] > 1 )
			VectorNormalize( &tess_stageTangents[3 * i] );
		CrossProduct( &tess_stageNormals[3 * i], &tess_stageTangents[3 * i], &tess_stageBitangents[3 * i] );
		VectorNormalize( &tess_stageBitangents[3 * i] );
	}
	tess_stageTangentsValid   = 1;
	tess_stageBitangentsValid = 1;
}

/* ---- RB_CalcCubeMapBumpmapFrameTexCoords  0x005158A0 ----  [HIGH]
 * Rotate each source coord vector into the per-vertex tangent frame. */
void __cdecl RB_CalcCubeMapBumpmapFrameTexCoords( float *st )
{
	vec3_t v;
	int    i;

	if ( !tess_stageTangentsValid || !tess_stageBitangentsValid )
		RB_CalcTangentSpace();

	for ( i = 0; i < tess_numVertexes; i++, st += 3 )
	{
		VectorCopy( st, v );
		st[0] = DotProduct( v, &tess_stageTangents[3 * i] );
		st[1] = DotProduct( v, &tess_stageBitangents[3 * i] );
		st[2] = DotProduct( v, &tess_stageNormals[3 * i] );
	}
}

/* ---- myftol  0x00515940 ----  VERIFIED
 * fld value / fistp ftol_tempSpill / mov eax, ftol_tempSpill: round to nearest
 * under the default control word (retail only sets _PC_53), not a truncation.
 * Retail inlines this body at every call site.  Under VC7 the result is
 * returned in EAX by the asm itself (no C return), so an inlined call leaves
 * the value in EAX the way retail's WAVEVALUE sites use it. */
int __cdecl myftol( float value )
{
	__asm fld   value
	__asm fistp ftol_tempSpill
#ifdef COD1_VC7
	__asm mov   eax, ftol_tempSpill
#else
	return ftol_tempSpill;
#endif
}

#ifdef COD1_VC7
/* Q_rsqrt: a static copy of com_math.c's (non-/Op) __fsqrt, which retail
 * inlines into RB_CalcSpecularAlpha. */
static __inline float Q_rsqrt( float number )
{
	long  i;
	float x2, y;

	x2 = number * 0.5f;
	y  = number;
	i  = *(long *)&y;
	i  = 0x5f3759df - ( i >> 1 );
	y  = *(float *)&i;
	y  = y * ( 1.5f - ( x2 * y * y ) );

	return y;
}
#endif

/* ---- RB_CalcSpecularAlpha  0x00515950 ----  VERIFIED
 * Specular coefficient (dot of the reflected light vector and the view vector,
 * raised to the 4th power) into the alpha channel.  The view length uses the
 * Q_rsqrt fast inverse square root.  VC7: the Q3 text (retail's bytes); the
 * modern build keeps the hand-expanded form below, whose doubles reproduce the
 * values retail keeps unrounded on the x87 stack. */
#ifndef COD1_VC7
void __cdecl RB_CalcSpecularAlpha( unsigned char *alphas )
{
	float *xyz = (float *)tess_xyz;
	float *normal;
	unsigned char *dst;
	float  lightDir[3];
	float  viewer[3];
	float  l, lensq, rsq;
	double d, v0, v2;      /* kept on the x87 stack in retail, never rounded to float */
	int    rsqBits;
	int    b;
	int    i;

	dst = alphas + 3;
	if ( tess_numVertexes > 0 )
	{
		normal = (float *)tess_stageNormals;
		for ( i = 0; i < tess_numVertexes; i++ )
		{
			lightDir[0] = rbSpecularLightOrigin - xyz[0];
			lightDir[1] = rbSpecularLightOriginY - xyz[1];
			lightDir[2] = rbSpecularLightOriginZ - xyz[2];
			VectorNormalizeFast( lightDir );

			d = lightDir[2] * normal[2] + lightDir[1] * normal[1] + lightDir[0] * normal[0];

			/* only viewer[1] is stored (fstp m32); viewer[2] is fst to a float
			   and the extended value is multiplied by that float copy
			   (0x515A0A/0x515A0E); viewer[0] never leaves the FPU stack */
			v0        = backEnd_or.viewOrigin[0] - xyz[0];
			viewer[1] = backEnd_or.viewOrigin[1] - xyz[1];
			v2        = backEnd_or.viewOrigin[2] - xyz[2];
			viewer[2] = (float)v2;
			lensq = v2 * viewer[2] + viewer[1] * viewer[1] + v0 * v0;

			rsqBits = 0x5F3759DF - ( *(int *)&lensq >> 1 );   /* Q_rsqrt seed */
			rsq = *(float *)&rsqBits;
			l = ( 1.5f - lensq * 0.5f * rsq * rsq ) * rsq
			  * ( viewer[2] * ( d * normal[2] + d * normal[2] - lightDir[2] )
			    + viewer[1] * ( d * normal[1] + d * normal[1] - lightDir[1] )
			    + v0        * ( d * normal[0] + d * normal[0] - lightDir[0] ) );

			if ( l < 0.0f )
			{
				b = 0;
			}
			else
			{
				b = (int)( l * l * ( l * l ) * 255.0f );
				if ( b > 255 )
					b = 255;
			}
			*dst = b;
			normal += 3;
			dst    += 4;
			xyz    += tess_vertexComponentCount;
		}
	}
}
#else
void __cdecl RB_CalcSpecularAlpha( unsigned char *alphas )
{
	int    i;
	float *v, *normal;
	vec3_t viewer, reflected;
	float  l, d;
	int    b;
	vec3_t lightDir;
	int    numVertexes;

	v = tess_xyz;
	normal = tess_stageNormals;

	alphas += 3;

	numVertexes = tess_numVertexes;
	for ( i = 0; i < numVertexes; i++, v += tess_vertexComponentCount, normal += 3, alphas += 4 )
	{
		float ilength;

		lightDir[0] = rbSpecularLightOrigin - v[0];
		lightDir[1] = rbSpecularLightOriginY - v[1];
		lightDir[2] = rbSpecularLightOriginZ - v[2];
		VectorNormalizeFast( lightDir );

		d = DotProduct( normal, lightDir );

		reflected[0] = normal[0] * 2 * d - lightDir[0];
		reflected[1] = normal[1] * 2 * d - lightDir[1];
		reflected[2] = normal[2] * 2 * d - lightDir[2];

		VectorSubtract( backEnd_or.viewOrigin, v, viewer );
		ilength = Q_rsqrt( DotProduct( viewer, viewer ) );
		l = DotProduct( reflected, viewer );
		l *= ilength;

		if ( l < 0 )
		{
			b = 0;
		}
		else
		{
			l = l * l;
			l = l * l;
			b = l * 255;
			if ( b > 255 )
				b = 255;
		}

		*alphas = b;
	}
}
#endif

/* ---- RB_CalcDiffuseColor  0x00515AD0 ----  [HIGH]
 * Basic vertex lighting: ambient plus N.L directed light, or the packed ambient
 * colour where the surface faces away from the light. */
void __cdecl RB_CalcDiffuseColor( unsigned char *colors )
{
	int    i, j;
	float *normal;
	float  incoming;
	byte  *ent;
	int    ambientLightInt;
	vec3_t ambientLight;
	vec3_t lightDir;
	vec3_t directedLight;
	int    numVertexes;

	ent = (byte *)backEnd_currentEntity;
	ambientLightInt = *(int *)( ent + 0xBC );
	VectorCopy( (float *)( ent + 0xB0 ), ambientLight );
	VectorCopy( (float *)( ent + 0xC0 ), directedLight );
	VectorCopy( (float *)( ent + 0xA4 ), lightDir );

	normal = tess_stageNormals;

	numVertexes = tess_numVertexes;
	for ( i = 0; i < numVertexes; i++, normal += 3 )
	{
		incoming = lightDir[2] * normal[2] + lightDir[0] * normal[0] + lightDir[1] * normal[1];   /* retail's order */
		if ( incoming <= 0 )
		{
			*(int *)&colors[i * 4] = ambientLightInt;
			continue;
		}
		j = myftol( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 )
			j = 255;
		colors[i * 4 + 0] = j;

		j = myftol( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 )
			j = 255;
		colors[i * 4 + 1] = j;

		j = myftol( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 )
			j = 255;
		colors[i * 4 + 2] = j;

		colors[i * 4 + 3] = 255;
	}
}
