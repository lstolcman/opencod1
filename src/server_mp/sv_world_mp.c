/*
 * @fidelity: verified
 */

#include "server.h"

#include <string.h>
#include <math.h>

#define SVF_CAPSULE                 0x00000200

/* The retail leaf-list size at 0x0045EF8C (`push 80h`). */
#define MAX_TOTAL_ENT_LEAFS         128

#define SVF_USE_CLIP_BOUNDS         0x00000002
#define SVF_USE_MODEL_BOUNDS        0x00000004
#define SVF_DOBJ_BOUNDS_MASK        ( SVF_USE_CLIP_BOUNDS | SVF_USE_MODEL_BOUNDS )

#define GAME_DOBJ_CALC_POSE         13

typedef struct svOrientation_t {
	vec3_t axis[3];                 /* +0x00 */
	vec3_t origin;                  /* +0x24 */
} svOrientation_t;
SV_ASSERT_SIZE( svOrientation_t, 48 );

typedef struct svDObjTrace_t {
	float fraction;                 /* +0x00 */
	int surfaceFlags;               /* +0x04 */
	vec3_t normal;                  /* +0x08 model space */
	unsigned short partName;        /* +0x14 */
	unsigned short partGroup;       /* +0x16 */
	byte startsolid;                /* +0x18 */
	byte allsolid;                  /* +0x19 */
	/* +0x1A..0x1B alignment padding; a byte array here would add a /GS cookie retail lacks */
} svDObjTrace_t;
SV_ASSERT_SIZE( svDObjTrace_t, 28 );

extern int CM_NumInlineModels( void );
extern clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs,
									 int contents, qboolean capsule );
extern int CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *list,
						   int listsize, int *lastLeaf );
extern int CM_LeafCluster( int leafnum );
extern int CM_LeafArea( int leafnum );
extern int CM_PointContents( const vec3_t p, clipHandle_t model );
extern int CM_TransformedPointContents( const vec3_t p, clipHandle_t model,
										const vec3_t origin,
										const vec3_t angles );
extern int CM_AreaEntities( const vec3_t mins, const vec3_t maxs,
							int *entityList, int maxcount, int contentmask );
extern void CM_LinkEntity( void *ent, const vec3_t absmin, const vec3_t absmax );
extern void CM_UnlinkEntity( void *ent );

extern void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						 const vec3_t mins, const vec3_t maxs,
						 clipHandle_t model, int brushmask, qboolean capsule );
extern void CM_TransformedBoxTrace( trace_t *results, const vec3_t start,
									const vec3_t end, const vec3_t mins,
									const vec3_t maxs, clipHandle_t model,
									int brushmask, const vec3_t origin,
									const vec3_t angles, qboolean capsule );
extern int CM_BoxSightTrace( int oldHitNum, const vec3_t start,
							 const vec3_t end, const vec3_t mins,
							 const vec3_t maxs, clipHandle_t model,
							 int brushmask, qboolean capsule );
extern int CM_TransformedBoxSightTrace( int oldHitNum, const vec3_t start,
										const vec3_t end, const vec3_t mins,
										const vec3_t maxs, clipHandle_t model,
										int brushmask, const vec3_t origin,
										const vec3_t angles, qboolean capsule );

extern void CM_PointTraceStaticModels( trace_t *results, const vec3_t start,
									   const vec3_t end, int contentmask );
extern qboolean CM_TraceBox( const vec3_t start, const vec3_t end,
							 const vec3_t mins, const vec3_t maxs,
							 float fraction );

extern float RadiusFromBounds( const vec3_t mins, const vec3_t maxs );

extern void AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
extern float *MatrixTransformVector( float *in, float *matrix, float *out );
extern float *MatrixTransposeTransformVector( float *worldPoint, float *matrix,
											  float *localOut );

extern void DObjGetBounds( int dobj, float *mins, float *maxs );
extern char DObjTraceParts( svDObjTrace_t *trace, int dobj, float *start,
							float *end, const byte *partState );
extern int  DObjTraceModelParts( svDObjTrace_t *trace, int dobj, float *start,
								 float *end, int contentmask );
extern int  DObjHasContents( int dobj, int contentmask );

extern gentity_t *SV_GentityNum( int num );
extern svEntity_t *SV_SvEntityForGentity( gentity_t *gent );

/* ---- SV_ClipHandleForEntity  0x0045EBD0 ---- */
clipHandle_t SV_ClipHandleForEntity( gentity_t *gent ) {
	int index;

	if ( gent->r.bmodel ) {
		index = gent->s.index;
		if ( index < 0 || index >= CM_NumInlineModels() ) {
			Com_Error( ERR_DROP, "\x15" "CM_InlineModel: bad number" );
		}
		return index;
	}

	if ( gent->r.svFlags & SVF_CAPSULE ) {
		return CM_TempBoxModel( gent->r.mins, gent->r.maxs, gent->r.contents, qtrue );
	}
	return CM_TempBoxModel( gent->r.mins, gent->r.maxs, gent->r.contents, qfalse );
}

/* ---- SV_UnlinkEntity  0x0045EC40 ---- */
void SV_UnlinkEntity( gentity_t *gent ) {
	svEntity_t *svEnt;

	svEnt = SV_SvEntityForGentity( gent );
	gent->r.linked = qfalse;
	CM_UnlinkEntity( svEnt );
}

/* client_mp/cl_console_mp.c FastRound (0x00408370), copied: the dedicated build has no cl_console_mp.c. */
static __inline int SV_FastRound( float f ) {
	double bias = 9.313225746154785e-10;	/* qword 0x3E10000000000000 */
	int i;

	__asm fld f
	__asm fadd bias
	__asm fistp i

	return i;
}

/* ---- SnapAngles  0x0045EC80 ---- */
/* Retail inlines FastRound (fld / fadd qword 2^-30 / fistp, as game_mp/bg_slidemove.c Q_ftol), so it rounds to nearest under the FPU control word; a C (int) cast would truncate through __ftol2. */
void SnapAngles( float *v ) {
	int i;
	rfloat r;
	rfloat delta;

	for ( i = 0 ; i < 3 ; i++ ) {
		r = SV_FastRound( v[i] );
		delta = r - v[i];
		if ( delta * delta < 0.0000010000001f ) {	/* fcomp flt_568F88 = 0x358637BE; test ah,5 / jp */
			v[i] = r;
		}
	}
}

/* Retail 0x005759C0..0x005759D4; read by SV_LinkEntity and SV_PointTraceToEntity. */
static const vec3_t sv_defaultEntityClipMins = { -64.0f, -64.0f, -32.0f };
static const vec3_t sv_defaultEntityClipMaxs = {  64.0f,  64.0f,  72.0f };

/* ---- SV_LinkEntity  0x0045ECE0 ---- */
void SV_LinkEntity( gentity_t *gent ) {
	svEntity_t *svEnt;
	void *dobj;
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int lastLeaf;
	int num;
	int i, j, k;
	int area;
	int cluster;
	float radius;
	int packed;
	float *angles;

	svEnt = SV_SvEntityForGentity( gent );

	if ( gent->r.bmodel ) {
		gent->s.solid = 0x00FFFFFF;
	} else if ( gent->r.contents & 0x02000001 ) {
		i = (int)gent->r.maxs[0];
		if ( i < 1 ) {
			i = 1;
		} else if ( i > 255 ) {
			i = 255;
		}

		j = (int)( -gent->r.mins[2] );
		if ( j < 1 ) {
			j = 1;
		} else if ( j > 255 ) {
			j = 255;
		}

		k = (int)( gent->r.maxs[2] + 32.0f );
		if ( k < 1 ) {
			k = 1;
		} else if ( k > 255 ) {
			k = 255;
		}

		packed = ( ( ( k << 8 ) | j ) << 8 ) | i;
		gent->s.solid = packed;
	} else {
		gent->s.solid = 0;
	}

	angles = gent->r.currentAngles;
	SnapAngles( angles );

	if ( gent->r.bmodel
		 && ( angles[0] != 0.0f || angles[1] != 0.0f || angles[2] != 0.0f ) ) {
		radius = RadiusFromBounds( gent->r.mins, gent->r.maxs );

		for ( i = 0 ; i < 3 ; i++ ) {
			gent->r.absmin[i] = gent->r.currentOrigin[i] - radius;
			gent->r.absmax[i] = gent->r.currentOrigin[i] + radius;
		}
	} else {
		gent->r.absmin[0] = gent->r.mins[0] + gent->r.currentOrigin[0];
		gent->r.absmin[1] = gent->r.mins[1] + gent->r.currentOrigin[1];
		gent->r.absmin[2] = gent->r.mins[2] + gent->r.currentOrigin[2];
		gent->r.absmax[0] = gent->r.maxs[0] + gent->r.currentOrigin[0];
		gent->r.absmax[1] = gent->r.maxs[1] + gent->r.currentOrigin[1];
		gent->r.absmax[2] = gent->r.maxs[2] + gent->r.currentOrigin[2];
	}

	gent->r.absmin[0] -= 1.0f;
	gent->r.absmin[1] -= 1.0f;
	gent->r.absmin[2] -= 1.0f;
	gent->r.absmax[0] += 1.0f;
	gent->r.absmax[1] += 1.0f;
	gent->r.absmax[2] += 1.0f;

	svEnt->numClusters = 0;
	svEnt->lastCluster = 0;
	svEnt->areanum = -1;
	svEnt->areanum2 = -1;

	num = CM_BoxLeafnums( gent->r.absmin, gent->r.absmax, leafs,
						  MAX_TOTAL_ENT_LEAFS, &lastLeaf );
	if ( num == 0 ) {
		CM_UnlinkEntity( svEnt );
	} else {
		for ( i = 0 ; i < num ; i++ ) {
			area = CM_LeafArea( leafs[i] );
			if ( area == -1 ) {
				continue;
			}

			if ( svEnt->areanum == -1 || svEnt->areanum == area ) {
				svEnt->areanum = area;
				continue;
			}

			if ( svEnt->areanum2 != -1 && svEnt->areanum2 != area
				 && sv.state == SS_LOADING ) {
				Com_DPrintf( "Object %i touching 3 areas at %f %f %f\n",
							 gent->s.number, gent->r.absmin[0],
							 gent->r.absmin[1], gent->r.absmin[2] );
			}
			svEnt->areanum2 = area;
		}

		svEnt->numClusters = 0;
		for ( i = 0 ; i < num ; i++ ) {
			cluster = CM_LeafCluster( leafs[i] );
			if ( cluster == -1 ) {
				continue;
			}
			svEnt->clusternums[svEnt->numClusters] = cluster;
			svEnt->numClusters++;
			if ( svEnt->numClusters == MAX_ENT_CLUSTERS ) {
				break;
			}
		}

		if ( i != num ) {
			svEnt->lastCluster = CM_LeafCluster( lastLeaf );
		}

		gent->r.linked = qtrue;

		if ( !gent->r.contents ) {
			CM_UnlinkEntity( svEnt );
			return;
		}
		/* 0x0045F0E3: dobj_serverHandles/dobj_pool inline (NULL for handle 0) */
		dobj = Com_GetServerDObj( gent->s.number );
		if ( dobj && ( gent->r.svFlags & SVF_DOBJ_BOUNDS_MASK ) ) {
			vec3_t linkMins, linkMaxs;

			/* 0x0045F118: bit 2 is tested first here (the reverse of SV_PointTraceToEntity, which tests 4 first) */
			/* x/y only, with no +-1. CM_LinkEntity reads only [0..1]; retail leaves z unwritten or in model space. The stack-local box never reaches r.absmin/r.absmax. */
			if ( gent->r.svFlags & SVF_USE_CLIP_BOUNDS ) {
				linkMins[0] = sv_defaultEntityClipMins[0] + gent->r.currentOrigin[0];
				linkMins[1] = sv_defaultEntityClipMins[1] + gent->r.currentOrigin[1];
				linkMaxs[0] = sv_defaultEntityClipMaxs[0] + gent->r.currentOrigin[0];
				linkMaxs[1] = sv_defaultEntityClipMaxs[1] + gent->r.currentOrigin[1];
			} else {
				DObjGetBounds( (int)dobj, linkMins, linkMaxs );	/* 0x0045F155 = inlined 0x00482210 */
				linkMins[0] += gent->r.currentOrigin[0];
				linkMins[1] += gent->r.currentOrigin[1];
				linkMaxs[0] += gent->r.currentOrigin[0];
				linkMaxs[1] += gent->r.currentOrigin[1];
			}
			CM_LinkEntity( svEnt, linkMins, linkMaxs );
		} else {
			/* 0x0045F1C6 */
			CM_LinkEntity( svEnt, gent->r.absmin, gent->r.absmax );
		}
	}
}

/* ---- SV_PointContents  0x00460150 ---- */
int SV_PointContents( const vec3_t p, int passEntityNum, int contentmask ) {
	int touch[MAX_GENTITIES];
	gentity_t   *hit;
	int i, num;
	int contents, c2;
	clipHandle_t clipHandle;

	contents = CM_PointContents( p, 0 );

	num = CM_AreaEntities( p, p, touch, MAX_GENTITIES, contentmask );

	for ( i = 0 ; i < num ; i++ ) {
		if ( touch[i] == passEntityNum ) {
			continue;
		}
		hit = SV_GentityNum( touch[i] );
		clipHandle = SV_ClipHandleForEntity( hit );

		c2 = CM_TransformedPointContents( p, clipHandle, hit->r.currentOrigin,
										  hit->r.currentAngles );

		contents |= c2;
	}

	return contents & contentmask;
}

    /* 0x0045F1E0-0x0046014A: the six entity-collision functions, UO's sv_entity_collision.c under UO names. */

/* ---- SV_EntityNumForSvEntity  no-address ----
 * A macro: retail has no out-of-line copy of this helper (a called static
 * would leave a tree-only dead copy under /GL). */
#define SV_EntityNumForSvEntity( svEnt )    ( (int)( ( svEnt ) - sv.svEntities ) )

/* ---- SV_ClipMoveToEntity  0x0045F1E0 ---- */
void SV_ClipMoveToEntity( moveclip_t *clip, svEntity_t *entity ) {
	gentity_t *touch;
	int entityNum;
	clipHandle_t clipHandle;
	float *angles;
	trace_t trace;

	entityNum = SV_EntityNumForSvEntity( entity );
	touch = SV_GentityNum( entityNum );

	if ( !( clip->contentmask & touch->r.contents ) ) {
		return;
	}
	if ( clip->passEntityNum != ENTITYNUM_NONE ) {
		if ( entityNum == clip->passEntityNum ) {
			return;
		}
		if ( touch->r.ownerNum == clip->passEntityNum ) {
			return;
		}
		if ( touch->r.ownerNum == clip->passOwnerNum ) {
			return;
		}
	}

	clipHandle = SV_ClipHandleForEntity( touch );

	angles = touch->r.currentAngles;

	if ( !touch->r.bmodel ) {
		angles = vec3_origin;
	}

	trace.fraction = clip->trace.fraction;
	CM_TransformedBoxTrace( &trace, clip->start, clip->end,
							clip->mins, clip->maxs, clipHandle,
							clip->contentmask, touch->r.currentOrigin, angles,
							clip->capsule );

	if ( trace.fraction >= clip->trace.fraction ) {
		clip->trace.allsolid |= trace.allsolid;
		clip->trace.startsolid |= trace.startsolid;
		return;
	}

	trace.allsolid |= clip->trace.allsolid;
	trace.startsolid |= clip->trace.startsolid;
	trace.entityNum = (unsigned short)touch->s.number;
	clip->trace = trace;
}

/* ---- SV_PointTraceToEntity  0x0045F300 ---- */
void SV_PointTraceToEntity( pointtrace_t *clip, svEntity_t *entity ) {
	gentity_t *touch;
	int entityNum;
	clipHandle_t clipHandle;
	float *angles;
	void *obj;
	trace_t trace;
	vec3_t mins, maxs;
	vec3_t localStart, localEnd;
	svOrientation_t orient;
	svDObjTrace_t dobjTrace;

	entityNum = SV_EntityNumForSvEntity( entity );
	touch = SV_GentityNum( entityNum );

	if ( !( clip->contentmask & touch->r.contents ) ) {
		return;
	}
	if ( clip->passEntityNum != ENTITYNUM_NONE ) {
		if ( entityNum == clip->passEntityNum ) {
			return;
		}
		if ( touch->r.ownerNum == clip->passEntityNum ) {
			return;
		}
		if ( touch->r.ownerNum == clip->passOwnerNum ) {
			return;
		}
	}

	/* Com_GetServerDObj is retail's dobj_serverHandles[]/dobj_pool[] pair inlined at 0x0045F385-0x0045F39C; it returns NULL for handle 0. */
	obj = NULL;
	if ( clip->useDObj ) {
		obj = Com_GetServerDObj( touch->s.number );
	}

	if ( obj && ( touch->r.svFlags & SVF_DOBJ_BOUNDS_MASK ) ) {
		/* orient.origin is filled here, before VM_Call (0x0045F3CF / 0x0045F459), and the bounds are offset by it */
		if ( touch->r.svFlags & SVF_USE_MODEL_BOUNDS ) {
			if ( !DObjHasContents( (int)obj, clip->contentmask ) ) {
				return;
			}
			VectorCopy( touch->r.currentOrigin, orient.origin );
			DObjGetBounds( (int)obj, mins, maxs );
			VectorAdd( mins, orient.origin, mins );
			VectorAdd( maxs, orient.origin, maxs );
		} else {
			VectorCopy( touch->r.currentOrigin, orient.origin );
			VectorAdd( sv_defaultEntityClipMins, orient.origin, mins );
			VectorAdd( sv_defaultEntityClipMaxs, orient.origin, maxs );
		}

		if ( CM_TraceBox( clip->start, clip->end, mins, maxs,
						  clip->trace.fraction ) ) {
			return;
		}

		VM_Call( vm, GAME_DOBJ_CALC_POSE, touch->s.number );

		/* localStart/localEnd share the mins/maxs frame slots (retail 0x20/0x14): VC7 packs them */
		AnglesToAxis( touch->r.currentAngles, orient.axis );
		MatrixTransposeTransformVector( clip->start, (float *)&orient, localStart );
		MatrixTransposeTransformVector( clip->end, (float *)&orient, localEnd );

		dobjTrace.fraction = clip->trace.fraction;
		if ( touch->r.svFlags & SVF_USE_MODEL_BOUNDS ) {
			DObjTraceModelParts( &dobjTrace, (int)obj, localStart, localEnd,
								 clip->contentmask );
		} else {
			DObjTraceParts( &dobjTrace, (int)obj, localStart, localEnd,
							clip->dobjTracePartState );
		}

		if ( dobjTrace.fraction >= clip->trace.fraction ) {
			clip->trace.allsolid |= dobjTrace.allsolid;
			clip->trace.startsolid |= dobjTrace.startsolid;
			return;
		}

		trace.fraction = dobjTrace.fraction;
		trace.startsolid = dobjTrace.startsolid;
		trace.surfaceFlags = dobjTrace.surfaceFlags;
		trace.partName = dobjTrace.partName;
		trace.partGroup = dobjTrace.partGroup;
		/* Retail never sets trace.allsolid on this path and ORs a stale stack byte into it below
		 * (0x0045F66B); the VC7 build keeps that, other builds zero it. */
#ifndef COD1_VC7
		trace.allsolid = 0;
#endif

		MatrixTransformVector( dobjTrace.normal, (float *)&orient, trace.normal );

		trace.endpos[0] = clip->start[0]
						  + ( clip->end[0] - clip->start[0] ) * trace.fraction;
		trace.endpos[1] = clip->start[1]
						  + ( clip->end[1] - clip->start[1] ) * trace.fraction;
		trace.endpos[2] = clip->start[2]
						  + ( clip->end[2] - clip->start[2] ) * trace.fraction;
	} else {
		clipHandle = SV_ClipHandleForEntity( touch );

		angles = touch->r.currentAngles;

		if ( !touch->r.bmodel ) {
			angles = vec3_origin;
		}

		trace.fraction = clip->trace.fraction;
		CM_TransformedBoxTrace( &trace, clip->start, clip->end,
								vec3_origin, vec3_origin, clipHandle,
								clip->contentmask, touch->r.currentOrigin, angles,
								qfalse );

		if ( trace.fraction >= clip->trace.fraction ) {
			clip->trace.allsolid |= trace.allsolid;
			clip->trace.startsolid |= trace.startsolid;
			return;
		}
	}

	trace.allsolid |= clip->trace.allsolid;
	trace.startsolid |= clip->trace.startsolid;
	trace.entityNum = (unsigned short)touch->s.number;
	trace.material = 0;
	trace.contents = touch->r.contents;
	clip->trace = trace;
}

/* ---- SV_ClipSightToEntity  0x0045F730 ---- */
qboolean SV_ClipSightToEntity( sightclip_t *clip, svEntity_t *entity ) {
	gentity_t *touch;
	int entityNum;
	clipHandle_t clipHandle;
	float *angles;

	entityNum = SV_EntityNumForSvEntity( entity );
	touch = SV_GentityNum( entityNum );

	if ( !( clip->contentmask & touch->r.contents ) ) {
		return 0;
	}
	if ( clip->passEntityNum != ENTITYNUM_NONE ) {
		if ( entityNum == clip->passEntityNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passEntityNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passEntityOwnerNum ) {
			return 0;
		}
	}
	if ( clip->passOwnerNum != ENTITYNUM_NONE ) {
		if ( entityNum == clip->passOwnerNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passOwnerNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passOwnerOwnerNum ) {
			return 0;
		}
	}

	clipHandle = SV_ClipHandleForEntity( touch );

	angles = touch->r.currentAngles;

	if ( !touch->r.bmodel ) {
		angles = vec3_origin;
	}

	if ( !CM_TransformedBoxSightTrace( 0, clip->start, clip->end,
									   clip->mins, clip->maxs, clipHandle,
									   clip->contentmask, touch->r.currentOrigin, angles,
									   clip->capsule ) ) {
		return 0;
	}
	return -1;
}

/* ---- SV_PointSightTraceToEntity  0x0045F7F0 ---- */
qboolean SV_PointSightTraceToEntity( sightpointtrace_t *clip, svEntity_t *entity ) {
	gentity_t *touch;
	int entityNum;
	clipHandle_t clipHandle;
	float *angles;

	entityNum = SV_EntityNumForSvEntity( entity );
	touch = SV_GentityNum( entityNum );

	if ( !( clip->contentmask & touch->r.contents ) ) {
		return 0;
	}
	if ( clip->passEntityNum != ENTITYNUM_NONE ) {
		if ( entityNum == clip->passEntityNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passEntityNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passEntityOwnerNum ) {
			return 0;
		}
	}
	if ( clip->passOwnerNum != ENTITYNUM_NONE ) {
		if ( entityNum == clip->passOwnerNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passOwnerNum ) {
			return 0;
		}
		if ( touch->r.ownerNum == clip->passOwnerOwnerNum ) {
			return 0;
		}
	}

	clipHandle = SV_ClipHandleForEntity( touch );

	angles = touch->r.currentAngles;

	if ( !touch->r.bmodel ) {
		angles = vec3_origin;
	}

	if ( !CM_TransformedBoxSightTrace( 0, clip->start, clip->end,
									   vec3_origin, vec3_origin, clipHandle,
									   clip->contentmask, touch->r.currentOrigin, angles,
									   qfalse ) ) {
		return 0;
	}
	return -1;
}

/* ---- SV_Trace  0x0045F8B0 ---- */
void SV_Trace( trace_t *results, const vec3_t start, const vec3_t mins,
			   const vec3_t maxs, const vec3_t end, int passEntityNum,
			   int contentmask, qboolean capsule, qboolean useDObj,
			   const byte *dobjTracePartState, qboolean locational ) {
	trace_t trace;
	vec3_t v;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	memset( &trace, 0, sizeof( trace ) );
	CM_BoxTrace( &trace, start, end, mins, maxs, 0, contentmask, capsule );

	trace.entityNum = ENTITYNUM_WORLD;
	if ( trace.fraction == 1.0f ) {
		trace.entityNum = ENTITYNUM_NONE;
	}

	if ( trace.fraction == 0.0f ) {
		*results = trace;
		return;
	}

	if ( locational ) {
		CM_PointTraceStaticModels( &trace, start, end, contentmask );
		if ( trace.fraction == 0.0f ) {
			*results = trace;
			return;
		}
	}

	/* pclip and clip are block-scoped so they share one frame slot, as in retail */
	if ( maxs[0] - mins[0] + maxs[1] - mins[1] + maxs[2] - mins[2] == 0.0f ) {
		pointtrace_t pclip;

		pclip.trace = trace;
		pclip.contentmask = contentmask;
		VectorCopy( start, pclip.start );
		VectorCopy( end, pclip.end );
		pclip.passEntityNum = passEntityNum;
		pclip.useDObj = useDObj;
		pclip.dobjTracePartState = dobjTracePartState;
		if ( passEntityNum == ENTITYNUM_NONE ) {
			pclip.passOwnerNum = -1;
		} else {
			pclip.passOwnerNum = SV_GentityNum( passEntityNum )->r.ownerNum;
			if ( pclip.passOwnerNum == ENTITYNUM_NONE ) {
				pclip.passOwnerNum = -1;
			}
		}

		CM_PointTraceToEntities( &pclip );

		*results = pclip.trace;
		return;
	} else {
		moveclip_t clip;

		clip.trace = trace;
		clip.contentmask = contentmask;
		clip.passEntityNum = passEntityNum;
		clip.capsule = capsule;
		if ( passEntityNum == ENTITYNUM_NONE ) {
			clip.passOwnerNum = -1;
		} else {
			clip.passOwnerNum = SV_GentityNum( passEntityNum )->r.ownerNum;
			if ( clip.passOwnerNum == ENTITYNUM_NONE ) {
				clip.passOwnerNum = -1;
			}
		}

		/* halfSize is built in expandedHalfSize (retail spills through its slots); mins is maxs * -1.0f (fmul, not fchs) */
		VectorSubtract( maxs, mins, clip.expandedHalfSize );
		VectorScale( clip.expandedHalfSize, 0.5f, clip.expandedHalfSize );
		VectorCopy( clip.expandedHalfSize, clip.maxs );
		VectorScale( clip.maxs, -1.0f, clip.mins );
		clip.expandedHalfSize[0] += 1.0f;
		clip.expandedHalfSize[1] += 1.0f;
		clip.expandedHalfSize[2] += 1.0f;

		/* v is the box centre here and the trace delta below (one vec3 slot in retail) */
		VectorAdd( mins, maxs, v );
		VectorScale( v, 0.5f, v );
		VectorAdd( start, v, clip.start );
		VectorAdd( end, v, clip.end );

		CM_ClipMoveToEntities( &clip );

		if ( clip.trace.fraction < trace.fraction ) {
			VectorSubtract( end, start, v );
			VectorMA( start, clip.trace.fraction, v, clip.trace.endpos );
		}

		*results = clip.trace;
	}
}

/* ---- SV_SightTrace  0x0045FCA0 ---- */
void SV_SightTrace( int *hitOut, const vec3_t start, const vec3_t mins,
					const vec3_t maxs, const vec3_t end, int passEntityNum,
					int passOwnerNum, int contentmask, qboolean capsule ) {
	vec3_t v;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	*hitOut = CM_BoxSightTrace( *hitOut, start, end, mins, maxs, 0,
								contentmask, capsule );
	if ( *hitOut ) {
		return;
	}

	if ( maxs[0] - mins[0] + maxs[1] - mins[1] + maxs[2] - mins[2] == 0.0f ) {
		sightpointtrace_t pclip;

		pclip.contentmask = contentmask;
		VectorCopy( start, pclip.start );
		VectorCopy( end, pclip.end );
		pclip.passEntityNum = passEntityNum;
		pclip.passOwnerNum = passOwnerNum;
		if ( passEntityNum == ENTITYNUM_NONE ) {
			pclip.passEntityOwnerNum = -1;
		} else {
			pclip.passEntityOwnerNum = SV_GentityNum( passEntityNum )->r.ownerNum;
			if ( pclip.passEntityOwnerNum == ENTITYNUM_NONE ) {
				pclip.passEntityOwnerNum = -1;
			}
		}
		if ( passOwnerNum == ENTITYNUM_NONE ) {
			pclip.passOwnerOwnerNum = -1;
		} else {
			pclip.passOwnerOwnerNum = SV_GentityNum( passOwnerNum )->r.ownerNum;
			if ( pclip.passOwnerOwnerNum == ENTITYNUM_NONE ) {
				pclip.passOwnerOwnerNum = -1;
			}
		}

		*hitOut = CM_PointSightTraceToEntities( &pclip );
		return;
	} else {
		sightclip_t clip;

		clip.contentmask = contentmask;
		clip.passEntityNum = passEntityNum;
		clip.passOwnerNum = passOwnerNum;
		clip.capsule = capsule;
		if ( passEntityNum == ENTITYNUM_NONE ) {
			clip.passEntityOwnerNum = -1;
		} else {
			clip.passEntityOwnerNum = SV_GentityNum( passEntityNum )->r.ownerNum;
			if ( clip.passEntityOwnerNum == ENTITYNUM_NONE ) {
				clip.passEntityOwnerNum = -1;
			}
		}
		if ( passOwnerNum == ENTITYNUM_NONE ) {
			clip.passOwnerOwnerNum = -1;
		} else {
			clip.passOwnerOwnerNum = SV_GentityNum( passOwnerNum )->r.ownerNum;
			if ( clip.passOwnerOwnerNum == ENTITYNUM_NONE ) {
				clip.passOwnerOwnerNum = -1;
			}
		}

		VectorSubtract( maxs, mins, clip.expandedHalfSize );
		VectorScale( clip.expandedHalfSize, 0.5f, clip.expandedHalfSize );
		VectorCopy( clip.expandedHalfSize, clip.maxs );
		VectorScale( clip.maxs, -1.0f, clip.mins );
		clip.expandedHalfSize[0] += 1.0f;
		clip.expandedHalfSize[1] += 1.0f;
		clip.expandedHalfSize[2] += 1.0f;

		VectorAdd( mins, maxs, v );
		VectorScale( v, 0.5f, v );
		VectorAdd( start, v, clip.start );
		VectorAdd( end, v, clip.end );

		*hitOut = CM_ClipSightTraceToEntities( &clip );
	}
}

/* ---- SV_SightTraceToEntity  0x0045FFA0 ---- */
/* (start, mins, maxs, end) as in the Mac 1.3 build: the order sets retail's x87 operand order in the box loop */
int SV_SightTraceToEntity( const vec3_t start, const vec3_t mins,
						   const vec3_t maxs, const vec3_t end,
						   int entityNum, int contentmask,
						   qboolean capsule ) {
	gentity_t *touch;
	vec3_t boxMins;
	vec3_t boxMaxs;
	int i;
	clipHandle_t clipHandle;
	float *origin, *angles;

	touch = SV_GentityNum( entityNum );

	if ( !( contentmask & touch->r.contents ) ) {
		return 0;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( end[i] > start[i] ) {
			boxMins[i] = start[i] + mins[i] - 1.0f;
			boxMaxs[i] = end[i] + maxs[i] + 1.0f;
		} else {
			boxMins[i] = end[i] + mins[i] - 1.0f;
			boxMaxs[i] = start[i] + maxs[i] + 1.0f;
		}
	}

	if ( touch->r.absmin[0] > boxMaxs[0]
		 || touch->r.absmin[1] > boxMaxs[1]
		 || touch->r.absmin[2] > boxMaxs[2]
		 || touch->r.absmax[0] < boxMins[0]
		 || touch->r.absmax[1] < boxMins[1]
		 || touch->r.absmax[2] < boxMins[2] ) {
		return 0;
	}

	clipHandle = SV_ClipHandleForEntity( touch );

	origin = touch->r.currentOrigin;
	angles = touch->r.currentAngles;

	if ( !touch->r.bmodel ) {
		angles = vec3_origin;
	}

	if ( !CM_TransformedBoxSightTrace( 0, start, end, mins, maxs, clipHandle,
									   contentmask, origin, angles,
									   capsule ) ) {
		return 0;
	}
	return -1;
}
