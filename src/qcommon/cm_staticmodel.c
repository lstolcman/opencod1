/*
 * qcommon/cm_staticmodel.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/qcommon/cm_staticmodel.c
 *
 * Retail range 0x0041F760-0x0041FFF0, 5 functions.
 *
 * @fidelity: likely
 */

#include "cm_local.h"

typedef struct cmStaticModel_t
{
	void                    *model;         /* +0x00  XModel *  */
	vec3_t origin;                          /* +0x04 */
	float inverseAxis[9];                   /* +0x10 */
	struct cmStaticModel_t  *next;          /* +0x34 */
	vec3_t linkMins;                        /* +0x38 */
	vec3_t linkMaxs;                        /* +0x44 */
	qboolean isBig;                         /* +0x50 */
} cmStaticModel_t;
COD1_ASSERT_SIZE( cmStaticModel_t, 84 );



/* universal/com_math.c -- settled there, __usercall in retail. */
void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right,
				   vec3_t up );
void MatrixInverse( const float in[9], float out[9] );
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] );

void CM_LinkStaticModel( void *link );

typedef void *( *XModelAllocFn )( int size );
void *XModelPrecache( const char *name, int loadSurfaces, XModelAllocFn alloc,
					  XModelAllocFn allocMesh );

int XModelGetStaticBounds( void *model, const float *axis, float *mins, float *maxs );

/* ---- CM_Hunk_AllocXModel  0x0041F760 ---- VERIFIED */
void *CM_Hunk_AllocXModel( int size ) {
	return Hunk_AllocAlignInternal( size, 32 );
}

/* ---- CM_Hunk_AllocXModelMesh  0x0041F770 ---- VERIFIED */
void *CM_Hunk_AllocXModelMesh( int size ) {
	return Hunk_AllocAlignInternal( size, 32 );
}

/* CM_StaticModelContents  0x0041F935 */
static int CM_StaticModelContents( void *model ) {
	return *( int * ) ( *( byte ** ) ( ( byte * ) model + 4 ) + 72 );
}

/* ---- CM_CreateStaticModel  0x0041F780 ---- VERIFIED */
void CM_CreateStaticModel( const char *name, cmStaticModel_t *sm,
						   const vec3_t origin, const vec3_t angles,
						   const vec3_t scale ) {
	vec3_t axis[3];

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "\x15" "Invalid static model name\n" );
	}
	if ( scale[0] == 0.0f ) {
		Com_Error( ERR_DROP, "\x15" "Static model [%s] has x scale of 0.0\n",
				   name );
	}
	if ( scale[1] == 0.0f ) {
		Com_Error( ERR_DROP, "\x15" "Static model [%s] has y scale of 0.0\n",
				   name );
	}
	if ( scale[2] == 0.0f ) {
		Com_Error( ERR_DROP, "\x15" "Static model [%s] has z scale of 0.0\n",
				   name );
	}

	sm->model = XModelPrecache( name, 1, CM_Hunk_AllocXModel,
								CM_Hunk_AllocXModelMesh );

	sm->origin[0] = origin[0];
	sm->origin[1] = origin[1];
	sm->origin[2] = origin[2];

	AnglesToAxis( angles, axis );   /* inlined /Op body: axis[1] stored, then reloaded */
	VectorScale( axis[0], scale[0], axis[0] );
	VectorScale( axis[1], scale[1], axis[1] );
	VectorScale( axis[2], scale[2], axis[2] );

	MatrixInverse( axis[0], sm->inverseAxis );

	if ( !XModelGetStaticBounds( sm->model, axis[0], sm->linkMins, sm->linkMaxs ) ) {
		return;
	}

	sm->linkMins[0] += origin[0];
	sm->linkMins[1] += origin[1];
	sm->linkMins[2] += origin[2];
	sm->linkMaxs[0] += origin[0];
	sm->linkMaxs[1] += origin[1];
	sm->linkMaxs[2] += origin[2];

	if ( CM_StaticModelContents( sm->model ) ) {
		CM_LinkStaticModel( sm );
	}
}

/* ---- CM_LoadStaticModels  0x0041F950 ---- VERIFIED */
void CM_LoadStaticModels( void ) {
	char *data_p;
	const char *token;
	/* Retail's key, value and modelName are three adjacent 64-byte frame
	 * arrays in that order (esp+34h/74h/B4h, then the /GS cookie), filled by
	 * inlined unbounded strcpy loops (0x0041FBF1, 0x0041FC43). One buffer in
	 * the same order keeps a 64+ char token running on into the next array.
	 * VC7 lays the three separate arrays out in retail's order. */
#ifdef COD1_VC7
	char key[64];
	char value[64];
	char modelName[64];
#else
	char kvBuf[3 * 64];
	char *key = kvBuf;
	char *value = kvBuf + 64;
	char *modelName = kvBuf + 128;
#endif
	vec3_t origin, angles, scale;
	qboolean isStaticModel;
	int modelIndex;

	data_p = cm_entityString;
	cm_numStaticModels = 0;
	cm_staticModels = NULL;

	/* Modern build only: retail (and the VC7 build) leave modelName
	 * uninitialised stack. Zeroing it once here keeps the never-written
	 * tail deterministic; it is NOT re-cleared per entity (see below), and
	 * a first-pass spill into it survives into the second pass, as in
	 * retail. */
#ifndef COD1_VC7
	Com_Memset( modelName, 0, 64 );
#endif

	for ( ;; ) {
		token = Com_Parse( &data_p );
		if ( !token[0] ) {
			break;
		}
		if ( token[0] != '{' ) {
			break;
		}
		isStaticModel = qfalse;

		for ( ;; ) {
			token = Com_Parse( &data_p );
			if ( !token[0] || token[0] == '}' ) {
				break;
			}
			strcpy( key, token );	/* retail: unbounded */

			token = Com_Parse( &data_p );
			if ( !token[0] ) {
				break;
			}
			strcpy( value, token );	/* retail: unbounded */

			if ( !_stricmp( key, "classname" ) &&
				 !_stricmp( value, "misc_model" ) ) {
				isStaticModel = qtrue;
			}
		}

		if ( isStaticModel ) {
			cm_numStaticModels++;
		}
	}

	if ( !cm_numStaticModels ) {
		return;
	}

	cm_staticModels = ( cmStaticModel_t * )
		CM_Hunk_AllocXModel( 84 * cm_numStaticModels );

	data_p = cm_entityString;
	modelIndex = 0;

	for ( ;; ) {
		token = Com_Parse( &data_p );
		if ( !token[0] ) {
			break;
		}
		if ( token[0] != '{' ) {
			break;
		}
		/* Retail clears only byte 0 per entity (0x0041FB4A). The name passed
		 * on is modelName+7, so a "model" value shorter than 7 chars reads
		 * the stale tail left by an earlier entity, as retail does. */
		modelName[0] = 0;
		VectorClear( origin );
		VectorClear( angles );
		scale[0] = scale[1] = scale[2] = 1.0f;
		isStaticModel = qfalse;

		for ( ;; ) {
			token = Com_Parse( &data_p );
			if ( !token[0] || token[0] == '}' ) {
				break;
			}
			strcpy( key, token );	/* retail: unbounded */

			token = Com_Parse( &data_p );
			if ( !token[0] ) {
				break;
			}
			strcpy( value, token );	/* retail: unbounded */

			if ( !_stricmp( key, "classname" ) ) {
				if ( !_stricmp( value, "misc_model" ) ) {
					isStaticModel = qtrue;
				}
			} else if ( !_stricmp( key, "model" ) ) {
				/* Plain non-padding copy: retail byte loop 0x0041FCA0-0x0041FCAE.
				 * A value of 64+ chars has already run on into modelName, so
				 * this overlapping copy never meets a NUL and runs off the
				 * stack, as retail does. */
				strcpy( modelName, value );
			} else if ( !_stricmp( key, "origin" ) ) {
				sscanf( value, "%f %f %f",
						&origin[0], &origin[1], &origin[2] );
			} else if ( !_stricmp( key, "angles" ) ) {
				sscanf( value, "%f %f %f",
						&angles[0], &angles[1], &angles[2] );
			} else if ( !_stricmp( key, "modelscale_vec" ) ) {
				sscanf( value, "%f %f %f",
						&scale[0], &scale[1], &scale[2] );
			} else if ( !_stricmp( key, "modelscale" ) ) {
				scale[2] = ( float ) atof( value );
				scale[1] = scale[2];
				scale[0] = scale[2];
			}
		}

		if ( isStaticModel ) {
			CM_CreateStaticModel( modelName + 7, &cm_staticModels[modelIndex],
								  origin, angles, scale );
			modelIndex++;
		}
	}
}

#include <malloc.h>

int XModelNumBones( const void *model );
void XModelGetBasePose( void *model, void *basePose );

int XModelTraceLine( void *model, trace_t *work, const float *basePose,
				const vec3_t localStart, const vec3_t localEnd,
				int contentmask );

vec_t VectorNormalize( vec3_t v );
void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out );
void MatrixTransformVector33_m( const vec3_t in, const float m[9], vec3_t out );

/* ---- CM_TraceStaticModel  0x0041FE00 ---- VERIFIED */
void CM_TraceStaticModel( cmStaticModel_t *sm, trace_t *results,
						  const vec3_t start, const vec3_t end,
						  int contentmask ) {
	trace_t work;
	vec3_t localStart, localEnd;
	vec3_t d;
	void *basePose;

	work.fraction = results->fraction;

	basePose = _alloca( XModelNumBones( sm->model ) << 6 );
	XModelGetBasePose( sm->model, basePose );

	/* Retail association order: (d2*inv[6+j] + d1*inv[3+j]) + d0*inv[j]
	 * (0x0041FE5A-0x0041FEA2, 0x0041FEBF-0x0041FF1C), d rounded to float by
	 * the inlined /Op _VectorSubtract. VC7 reassociates these float sums: the
	 * VC7 source order below is the one it turns into retail's grouping. */
	_VectorSubtract( start, sm->origin, d );
#ifdef COD1_VC7
	localStart[0] = d[2] * sm->inverseAxis[6] + d[0] * sm->inverseAxis[0]
					+ d[1] * sm->inverseAxis[3];
	localStart[1] = d[2] * sm->inverseAxis[7] + d[0] * sm->inverseAxis[1]
					+ d[1] * sm->inverseAxis[4];
	localStart[2] = d[2] * sm->inverseAxis[8] + d[0] * sm->inverseAxis[2]
					+ d[1] * sm->inverseAxis[5];
#else
	localStart[0] = d[2] * sm->inverseAxis[6] + d[1] * sm->inverseAxis[3]
					+ d[0] * sm->inverseAxis[0];
	localStart[1] = d[2] * sm->inverseAxis[7] + d[1] * sm->inverseAxis[4]
					+ d[0] * sm->inverseAxis[1];
	localStart[2] = d[2] * sm->inverseAxis[8] + d[1] * sm->inverseAxis[5]
					+ d[0] * sm->inverseAxis[2];
#endif

	_VectorSubtract( end, sm->origin, d );
#ifdef COD1_VC7
	localEnd[0] = d[2] * sm->inverseAxis[6] + d[0] * sm->inverseAxis[0]
				  + d[1] * sm->inverseAxis[3];
	localEnd[1] = d[2] * sm->inverseAxis[7] + d[0] * sm->inverseAxis[1]
				  + d[1] * sm->inverseAxis[4];
	localEnd[2] = d[2] * sm->inverseAxis[8] + d[0] * sm->inverseAxis[2]
				  + d[1] * sm->inverseAxis[5];
#else
	localEnd[0] = d[2] * sm->inverseAxis[6] + d[1] * sm->inverseAxis[3]
				  + d[0] * sm->inverseAxis[0];
	localEnd[1] = d[2] * sm->inverseAxis[7] + d[1] * sm->inverseAxis[4]
				  + d[0] * sm->inverseAxis[1];
	localEnd[2] = d[2] * sm->inverseAxis[8] + d[1] * sm->inverseAxis[5]
				  + d[0] * sm->inverseAxis[2];
#endif

	if ( XModelTraceLine( sm->model, &work, (const float *)basePose,
						  localStart, localEnd, contentmask ) < 0 ) {
		return;
	}

	work.entityNum = ENTITYNUM_WORLD;               /* WORD 0x3FE at work+0x28 */

	_VectorSubtract( end, start, d );               /* float store, retail 0x0041FF32-0x0041FF51 */
	work.endpos[0] = d[0] * work.fraction + start[0];   /* 0x0041FF54-0x0041FF74 */
	work.endpos[1] = d[1] * work.fraction + start[1];
	work.endpos[2] = d[2] * work.fraction + start[2];

	/* Retail association order: (N2*inv[3i+2] + N1*inv[3i+1]) + N0*inv[3i]
	 * (0x0041FF77-0x0041FFBF); d is reused for the rotated normal. VC7 gives
	 * retail's grouping only for the inlined com_math helper; written out it
	 * emits the first pair as N1, N2 (same sum). */
#ifdef COD1_VC7
	MatrixTransformVector33_m( work.normal, sm->inverseAxis, d );
#else
	d[0] = work.normal[2] * sm->inverseAxis[2] + work.normal[1] * sm->inverseAxis[1]
		   + work.normal[0] * sm->inverseAxis[0];
	d[1] = work.normal[2] * sm->inverseAxis[5] + work.normal[1] * sm->inverseAxis[4]
		   + work.normal[0] * sm->inverseAxis[3];
	d[2] = work.normal[2] * sm->inverseAxis[8] + work.normal[1] * sm->inverseAxis[7]
		   + work.normal[0] * sm->inverseAxis[6];
#endif
	VectorNormalize( d );
	VectorCopy( d, work.normal );

	*results = work;                                /* rep movsd, ECX = 0x0C */
}
