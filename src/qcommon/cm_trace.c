/*
 * qcommon/cm_trace.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/qcommon/cm_trace.c
 *
 * Retail range 0x004222D0-0x00426750, 37 functions.
 *
 * @fidelity: likely
 */

#include "cm_local.h"

void CM_TraceThroughPatchCollide( traceWork_t *tw, const patchCollide_t *pc );
qboolean CM_SightTraceThroughPatchCollide( traceWork_t *tw,
										   const patchCollide_t *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw,
										const patchCollide_t *pc );
void CM_TraceThroughTerrainCollide( traceWork_t *tw, void *tc );
qboolean CM_SightTraceThroughTerrainCollide( traceWork_t *tw, void *tc );
qboolean CM_PositionTestInTerrainCollide( traceWork_t *tw, void *tc );

vec_t VectorNormalize2( const vec3_t v, vec3_t out );
void VectorInverse( vec3_t v );

typedef struct cmSphereRecord_t
{
	sphere_t sphere;                /* +0x00 */
	vec3_t extents;                 /* +0x18 */
} cmSphereRecord_t;
COD1_ASSERT_SIZE( cmSphereRecord_t, 36 );

#define CM_TEMP_BOX_HANDLE      BOX_MODEL_HANDLE
#define CM_TEMP_CAPSULE_HANDLE  CAPSULE_MODEL_HANDLE

#define SURFACE_CLIP_EPSILON    0.125f

/* Q3's DotProduct, VectorSubtract and VectorAdd for values retail keeps on the
   x87 stack (_PC_53): the casts are no-ops under VC7, the modern build
   computes in double.  The dot product is summed a, b then c, in the order
   retail's code adds the terms */
#define CM_DotProductR( x, y, a, b, c ) \
	( ( rfloat )( x )[a] * ( y )[a] + ( rfloat )( x )[b] * ( y )[b] + \
	  ( rfloat )( x )[c] * ( y )[c] )
#define CM_VectorSubtractR( a, b, c ) \
	( ( c )[0] = ( rfloat )( a )[0] - ( b )[0], \
	  ( c )[1] = ( rfloat )( a )[1] - ( b )[1], \
	  ( c )[2] = ( rfloat )( a )[2] - ( b )[2] )
#define CM_VectorAddR( a, b, c ) \
	( ( c )[0] = ( rfloat )( a )[0] + ( b )[0], \
	  ( c )[1] = ( rfloat )( a )[1] + ( b )[1], \
	  ( c )[2] = ( rfloat )( a )[2] + ( b )[2] )

/* the brush tests compare the quotient just stored into enterFrac: VC7 uses
   its unrounded x87 copy (_PC_53), so the modern build compares enterQuot */
#ifdef COD1_VC7
#define CM_UNROUNDED_ENTER_FRAC enterFrac
#else
#define CM_UNROUNDED_ENTER_FRAC enterQuot
#endif

static void CM_TraceThroughBrush( traceWork_t *tw, const cbrush_t *brush );
static qboolean CM_TraceSphereThroughSphere( traceWork_t *tw,
											 const vec3_t start,
											 const vec3_t end,
											 const vec3_t sphereOrigin,
											 float sphereRadius );
static qboolean CM_TraceCylinderThroughCylinder( traceWork_t *tw,
												 const vec3_t cylinderOrigin,
												 float cylinderHalfHeight,
												 float cylinderRadius );
static int CM_SightTraceThroughBrush( const traceWork_t *tw,
									  const cbrush_t *brush );
static qboolean CM_SightTraceSphereThroughSphere( const traceWork_t *tw,
												  const vec3_t start,
												  const vec3_t end,
												  const vec3_t sphereOrigin,
												  float sphereRadius );
static qboolean CM_SightTraceCylinderThroughCylinder( const traceWork_t *tw,
													  const vec3_t origin,
													  float halfheight,
													  float radius );

/* ---- CM_Q_fabs  0x004222D0 ---- VERIFIED */
float CM_Q_fabs( float v ) {
	int i = *( int * )&v & 0x7FFFFFFF;

	return *( float * )&i;
}

/* ---- RotatePoint  0x004222F0 ---- VERIFIED */
void RotatePoint( vec3_t point, vec3_t matrix[3] ) {
	vec3_t tvec;

	VectorCopy( point, tvec );
	point[0] = DotProduct( matrix[0], tvec );
	point[1] = DotProduct( matrix[1], tvec );
	point[2] = DotProduct( matrix[2], tvec );
}

/* ---- TransposeMatrix  0x00422340 ---- VERIFIED */
void TransposeMatrix( vec3_t in[3], vec3_t out[3] ) {
	int i, j;

	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			out[i][j] = in[j][i];
		}
	}
}

/* ---- CreateRotationMatrix  0x00422370 ---- VERIFIED */
void CreateRotationMatrix( const vec3_t angles, vec3_t matrix[3] ) {
	AngleVectors( angles, matrix[0], matrix[1], matrix[2] );
	VectorInverse( matrix[1] );
}

/* ---- CM_ProjectPointOntoVector  0x004223A0 ---- VERIFIED */
void CM_ProjectPointOntoVector( const vec3_t point, const vec3_t vStart,
								const vec3_t vDir, vec3_t vProj ) {
	vec3_t pVec;

	VectorSubtract( point, vStart, pVec );
	VectorMA( vStart, DotProduct( pVec, vDir ), vDir, vProj );
}

/* ---- CM_DistanceFromLineSquared  0x00422400 ---- VERIFIED */
float CM_DistanceFromLineSquared( const vec3_t p, const vec3_t lp1,
								  const vec3_t lp2, const vec3_t dir ) {
	vec3_t proj, t;
	int j;

	CM_ProjectPointOntoVector( p, lp1, dir, proj );

	for ( j = 0; j < 3; j++ ) {
		if ( ( proj[j] > lp1[j] && proj[j] > lp2[j] ) ||
			 ( proj[j] < lp1[j] && proj[j] < lp2[j] ) ) {
			break;
		}
	}

	if ( j < 3 ) {
		if ( fabs( proj[j] - lp1[j] ) < fabs( proj[j] - lp2[j] ) ) {
			VectorSubtract( p, lp1, t );
		} else {
			VectorSubtract( p, lp2, t );
		}
		return DotProduct( t, t );
	}

	VectorSubtract( p, proj, t );
	return DotProduct( t, t );
}

/* ---- CM_VectorDistanceSquared  0x004225B0 ---- VERIFIED */
float CM_VectorDistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t dir;

	VectorSubtract( p2, p1, dir );
	return DotProduct( dir, dir );
}

/* ---- SquareRootFloat  0x004225E0 ---- VERIFIED */
float SquareRootFloat( float number ) {
	int i;
	float x, y;
	const float f = 1.5F;

	x = number * 0.5F;
	y = number;
	i = *( int * )&y;
	i = 0x5F3759DF - ( i >> 1 );
	y = *( float * )&i;
	y = y * ( f - ( x * y * y ) );
	y = y * ( f - ( x * y * y ) );
	return number * y;
}

/* ---- CM_TestBoxInBrush  0x00422630 ---- VERIFIED */
void CM_TestBoxInBrush( traceWork_t *tw, const cbrush_t *brush ) {
	const cbrushside_t *side;
	const cplane_t *plane;
	/* retail keeps dist, t, startp and d1 on the x87 stack (_PC_53) */
	rfloat dist, t, d1;
	rfloat startp[3];
	int i;

	if ( tw->bounds[0][0] > brush->maxs[0] ||
		 tw->bounds[0][1] > brush->maxs[1] ||
		 tw->bounds[0][2] > brush->maxs[2] ||
		 tw->bounds[1][0] < brush->mins[0] ||
		 tw->bounds[1][1] < brush->mins[1] ||
		 tw->bounds[1][2] < brush->mins[2] ) {
		return;
	}

	side = brush->sides;
	i = brush->numSides;

	if ( tw->sphere.use ) {
		for ( ; i != 0; i--, side++ ) {
			plane = side->plane;

			dist = ( rfloat )plane->dist + tw->sphere.radius;
			t = CM_DotProductR( plane->normal, tw->sphere.offset, 1, 0, 2 );
			if ( t > 0.0f ) {
				CM_VectorSubtractR( tw->start, tw->sphere.offset, startp );
			} else {
				CM_VectorAddR( tw->start, tw->sphere.offset, startp );
			}
			d1 = CM_DotProductR( startp, plane->normal, 0, 2, 1 ) - dist;
			if ( d1 > 0.0f ) {
				return;
			}
		}
	} else {
		for ( ; i != 0; i--, side++ ) {
			plane = side->plane;

			dist = plane->dist -
				   CM_DotProductR( tw->offsets[plane->signbits], plane->normal,
								   2, 0, 1 );
			d1 = CM_DotProductR( tw->start, plane->normal, 1, 0, 2 ) - dist;
			if ( d1 > 0.0f ) {
				return;
			}
		}
	}

	tw->trace.startsolid = tw->trace.allsolid = qtrue;
	tw->trace.fraction = 0.0f;
	tw->trace.contents = brush->contents;
}

/* ---- CM_TestInLeaf  0x004227F0 ---- VERIFIED */
void CM_TestInLeaf( traceWork_t *tw, const cLeaf_t *leaf ) {
	int i;
	cbrush_t *b;
	cPatch_t *patch;

	for ( i = 0; i < ( int )leaf->numLeafBrushes; i++ ) {
		b = &cm_brushes[cm_leafbrushes[leaf->firstLeafBrush + i]];
		if ( b->checkcount != cm_checkcount ) {
			b->checkcount = cm_checkcount;
			if ( b->contents & tw->contents ) {
				CM_TestBoxInBrush( tw, b );
				if ( tw->trace.allsolid ) {
					return;
				}
			}
		}
	}

	if ( !cm_noCurves->integer ) {
		for ( i = 0; i < ( int )leaf->numLeafSurfaces; i++ ) {
			patch = &cm_patches[cm_leafsurfaces[leaf->firstLeafSurface + i]];
			if ( patch->checkcount != cm_checkcount ) {
				patch->checkcount = cm_checkcount;
				if ( patch->contents & tw->contents ) {
					if ( patch->pc ) {
						if ( CM_PositionTestInPatchCollide( tw, patch->pc ) ) {
							tw->trace.allsolid = 1;
							tw->trace.startsolid = 1;
							tw->trace.fraction = 0.0f;
							return;
						}
					} else {
						if ( CM_PositionTestInTerrainCollide( tw, patch->tc ) ) {
							tw->trace.allsolid = 1;
							tw->trace.startsolid = 1;
							tw->trace.fraction = 0.0f;
							return;
						}
					}
				}
			}
		}
	}
}

/* ---- CM_TestCapsuleInCapsule  0x00422920 ---- VERIFIED */
void CM_TestCapsuleInCapsule( traceWork_t *tw ) {
#ifdef COD1_VC7
	/* Q3's CM_TestCapsuleInCapsule; the #else keeps retail's x87 rounding
	   (which values VC7 stores as floats) for the modern build */
	vec3_t top, bottom;
	vec3_t p1, p2, tmp;
	vec3_t offset, symetricSize[2];
	float radius, halfwidth, halfheight, offs, r;
	rfloat zdelta, cylinderHalfHeight;
	int i;

	VectorAdd( tw->start, tw->sphere.offset, top );
	VectorSubtract( tw->start, tw->sphere.offset, bottom );
	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( cm_boxModel.mins[i] + cm_boxModel.maxs[i] ) * 0.5f;
		symetricSize[0][i] = cm_boxModel.mins[i] - offset[i];
		symetricSize[1][i] = cm_boxModel.maxs[i] - offset[i];
	}
	halfwidth = symetricSize[1][0];
	halfheight = symetricSize[1][2];
	radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	offs = halfheight - radius;

	r = ( tw->sphere.radius + radius ) * ( tw->sphere.radius + radius );
	VectorCopy( offset, p1 );
	p1[2] += offs;
	VectorSubtract( p1, top, tmp );
	if ( DotProduct( tmp, tmp ) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0.0f;
	}
	VectorSubtract( p1, bottom, tmp );
	if ( DotProduct( tmp, tmp ) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0.0f;
	}
	VectorCopy( offset, p2 );
	p2[2] -= offs;
	VectorSubtract( p2, top, tmp );
	if ( DotProduct( tmp, tmp ) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0.0f;
	}
	VectorSubtract( p2, bottom, tmp );
	if ( DotProduct( tmp, tmp ) < r ) {
		tw->trace.startsolid = tw->trace.allsolid = qtrue;
		tw->trace.fraction = 0.0f;
	}

	zdelta = tw->start[2] - offset[2];
	cylinderHalfHeight = offs + tw->sphere.halfheight - tw->sphere.radius;
	if ( zdelta <= cylinderHalfHeight && zdelta >= -cylinderHalfHeight ) {
		top[2] = p1[2] = 0;
		VectorSubtract( top, p1, tmp );
		if ( DotProduct( tmp, tmp ) < r ) {
			tw->trace.fraction = 0.0f;
			tw->trace.startsolid = tw->trace.allsolid = qtrue;
		}
	}
#else
	vec3_t startTop, startBottom;
	vec3_t center;
	float offsetMaxs2, halfheight, radiusSquared;
	float topZ, bottomZ;
	float topDelta[2], bottomDelta0;
	/* retail 0x422920-0x422BAD keeps these on the x87 stack (_PC_53):
	   startBottom[0], offsetMaxs[0] (and so radius), every delta until the
	   copies noted below, the z delta and the cylinder half height */
	double startBottom0, offsetMaxs0, radius;
	double topD0, topD1, bottomD0, bottomD1, d2;
	double zdelta, cylinderHalfHeight;
	int i;

	VectorAdd( tw->start, tw->sphere.offset, startTop );
	startBottom0 = ( double )tw->start[0] - tw->sphere.offset[0];
	startBottom[1] = tw->start[1] - tw->sphere.offset[1];
	startBottom[2] = tw->start[2] - tw->sphere.offset[2];

	for ( i = 0; i < 3; i++ ) {
		center[i] = ( cm_boxModel.mins[i] + cm_boxModel.maxs[i] ) * 0.5f;
	}
	offsetMaxs0 = ( double )cm_boxModel.maxs[0] - center[0];
	offsetMaxs2 = cm_boxModel.maxs[2] - center[2];

	radius = offsetMaxs0 > offsetMaxs2 ? offsetMaxs2 : offsetMaxs0;
	halfheight = ( float )( offsetMaxs2 - radius );
	radiusSquared = ( float )( ( radius + tw->sphere.radius ) *
							   ( radius + tw->sphere.radius ) );

	/* retail sums each squared length z, y then x.  The x and y deltas
	   of the first test are also stored as floats, and the third test
	   squares those float copies; the x delta of the second test is
	   squared from its float copy in the second and fourth tests */
	topZ = center[2] + halfheight;

	topD0 = ( double )center[0] - startTop[0];
	topDelta[0] = ( float )topD0;
	topD1 = ( double )center[1] - startTop[1];
	topDelta[1] = ( float )topD1;
	d2 = ( double )topZ - startTop[2];
	if ( d2 * d2 + topD1 * topD1 + topD0 * topD0 < radiusSquared ) {
		tw->trace.allsolid = 1;
		tw->trace.startsolid = 1;
		tw->trace.fraction = 0.0f;
	}

	bottomD0 = center[0] - startBottom0;
	bottomDelta0 = ( float )bottomD0;
	bottomD1 = ( double )center[1] - startBottom[1];
	d2 = ( double )topZ - startBottom[2];
	if ( d2 * d2 + bottomD1 * bottomD1 +
		 ( double )bottomDelta0 * bottomDelta0 < radiusSquared ) {
		tw->trace.allsolid = 1;
		tw->trace.startsolid = 1;
		tw->trace.fraction = 0.0f;
	}

	bottomZ = center[2] - halfheight;

	d2 = ( double )bottomZ - startTop[2];
	if ( d2 * d2 + ( double )topDelta[1] * topDelta[1] +
		 ( double )topDelta[0] * topDelta[0] < radiusSquared ) {
		tw->trace.allsolid = 1;
		tw->trace.startsolid = 1;
		tw->trace.fraction = 0.0f;
	}

	d2 = ( double )bottomZ - startBottom[2];
	if ( d2 * d2 + bottomD1 * bottomD1 +
		 ( double )bottomDelta0 * bottomDelta0 < radiusSquared ) {
		tw->trace.allsolid = 1;
		tw->trace.startsolid = 1;
		tw->trace.fraction = 0.0f;
	}

	zdelta = ( double )tw->start[2] - center[2];
	cylinderHalfHeight = ( double )halfheight + tw->sphere.halfheight -
						 tw->sphere.radius;

	if ( zdelta <= cylinderHalfHeight && zdelta >= -cylinderHalfHeight ) {
		/* retail 0x422B64: 2D distance, summed y then x */
		topD0 = ( double )startTop[0] - center[0];
		topD1 = ( double )startTop[1] - center[1];
		if ( topD1 * topD1 + topD0 * topD0 < radiusSquared ) {
			tw->trace.allsolid = 1;
			tw->trace.startsolid = 1;
			tw->trace.fraction = 0.0f;
		}
	}
#endif
}

/* ---- CM_TestBoundingBoxInCapsule  0x00422BB0 ---- VERIFIED */
void CM_TestBoundingBoxInCapsule( traceWork_t *tw ) {
	rfloat offset[3];   /* x87 stack in retail (_PC_53); never stored */
	vec3_t size[2];
	int i;

	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( ( rfloat )cm_boxModel.mins[i] + cm_boxModel.maxs[i] ) *
					0.5f;
		size[0][i] = cm_boxModel.mins[i] - offset[i];
		size[1][i] = cm_boxModel.maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	tw->sphere.use = qtrue;
	tw->sphere.radius = ( size[1][0] > size[1][2] ) ? size[1][2] : size[1][0];
	tw->sphere.halfheight = size[1][2];
	VectorSet( tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius );

	CM_TempBoxModel( tw->mins, tw->maxs, box_brush->contents, qfalse );
	CM_TestBoxInBrush( tw, box_brush );
}

/* ---- CM_PositionTest  0x00422D30 ---- VERIFIED */
void CM_PositionTest( traceWork_t *tw ) {
	leafList_t ll;
	int leafs[1024];
	
	int i;

	VectorAdd( tw->start, tw->mins, ll.bounds[0] );
	VectorAdd( tw->start, tw->maxs, ll.bounds[1] );

	for ( i = 0 ; i < 3 ; i++ ) {
		ll.bounds[0][i] -= 1;
		ll.bounds[1][i] += 1;
	}

	ll.count = 0;
	ll.maxcount = 1024;
	ll.list = leafs;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf = 0;
	ll.overflowed = qfalse;
	
	

	cm_checkcount++;
	CM_BoxLeafnums_r( &ll, 0 );
	cm_checkcount++;

	for ( i = 0; i < ll.count; i++ ) {
		CM_TestInLeaf( tw, &cm_leafs[leafs[i]] );
		if ( tw->trace.allsolid ) {
			break;
		}
	}
}

/* ---- CM_TraceMayIntersectBox  0x00422E60 ---- VERIFIED */
qboolean CM_TraceMayIntersectBox( const traceWork_t *tw, const vec3_t mins,
								  const vec3_t maxs ) {
	vec3_t traceSize;
	vec3_t size;
	vec3_t delta;
	vec3_t center;
	vec3_t traceCenter;
	float cross, projected;

	if ( tw->bounds[0][0] > maxs[0] || tw->bounds[1][0] < mins[0] ) {
		return qfalse;
	}
	if ( tw->bounds[0][1] > maxs[1] || tw->bounds[1][1] < mins[1] ) {
		return qfalse;
	}
	if ( tw->bounds[0][2] > maxs[2] || tw->bounds[1][2] < mins[2] ) {
		return qfalse;
	}

	if ( !tw->isPoint ) {
		return qtrue;
	}

	VectorSubtract( maxs, mins, size );
	VectorAdd( mins, maxs, center );
	VectorAdd( tw->end, tw->start, traceCenter );
	VectorSubtract( traceCenter, center, delta );
	VectorSubtract( tw->bounds[1], tw->bounds[0], traceSize );

	cross = delta[2] * tw->delta[1] - delta[1] * tw->delta[2];
	projected = traceSize[1] * size[2] + traceSize[2] * size[1];
	if ( cross * cross > projected * projected ) {
		return qfalse;
	}

	cross = delta[0] * tw->delta[2] - delta[2] * tw->delta[0];
	projected = traceSize[0] * size[2] + traceSize[2] * size[0];
	if ( cross * cross > projected * projected ) {
		return qfalse;
	}

	cross = delta[1] * tw->delta[0] - delta[0] * tw->delta[1];
	projected = traceSize[0] * size[1] + traceSize[1] * size[0];
	if ( cross * cross > projected * projected ) {
		return qfalse;
	}

	return qtrue;
}

/* ---- CM_TraceThroughPatch  0x00423030 ---- VERIFIED */
void CM_TraceThroughPatch( traceWork_t *tw, const cPatch_t *patch ) {
	float oldFrac;

	if ( !CM_TraceMayIntersectBox( tw, patch->mins, patch->maxs ) ) {
		return;
	}

	oldFrac = tw->trace.fraction;

	if ( patch->pc ) {
		CM_TraceThroughPatchCollide( tw, patch->pc );
	} else {
		CM_TraceThroughTerrainCollide( tw, patch->tc );
	}

	if ( tw->trace.fraction < oldFrac ) {
		tw->trace.surfaceFlags = cm_materials[patch->materialNum].surfaceFlags;
		tw->trace.contents = patch->contents;
		tw->trace.material = ( int )&cm_materials[patch->materialNum];
	}
}

/* ---- CM_TraceThroughBrush  0x004230C0 ---- VERIFIED */
static void CM_TraceThroughBrush( traceWork_t *tw, const cbrush_t *brush ) {
	float enterFrac, leaveFrac;
	qboolean startout;
	const cbrushside_t *side;
	const cbrushside_t *clipside;
	const cplane_t *plane;
	cplane_t axialPlane;
	cbrushside_t axialSide;
	rfloat startPoint[3];
	vec3_t endPoint;
	float startDist, denom, sign;
	/* retail keeps endDist, planeDist, the leave denominator, the enter
	   limit and the enter quotient on the x87 stack (_PC_53); only startDist,
	   the enter denominator, enterFrac and leaveFrac are stored as floats */
	rfloat endDist, planeDist, t;
	rfloat leaveDenom, enterLimit, enterQuot;
	const float *bounds;
	int pass, axis, i;

	enterFrac = 0.0f;
	leaveFrac = tw->trace.fraction;
	startout = qtrue;
	clipside = NULL;
	sign = -1.0f;
	bounds = brush->mins;
	pass = 0;

	if ( tw->sphere.use ) {
		for ( ;; ) {
			for ( axis = 0; axis < 3; axis++ ) {
				startDist = ( tw->start[axis] - bounds[axis] ) * sign -
							tw->sphereExtents[axis];
				endDist = ( ( rfloat )tw->end[axis] - bounds[axis] ) * sign -
						  tw->sphereExtents[axis];

				if ( startDist > 0.0f ) {
					denom = FCAST( startDist - endDist );
					if ( endDist > 0.0f ) {
						if ( denom <= 0.0f ) {
							return;
						}
						if ( endDist >= SURFACE_CLIP_EPSILON ) {
							return;
						}
						startout = qfalse;
					}
					enterLimit = ( rfloat )startDist - SURFACE_CLIP_EPSILON;
					if ( enterLimit > ( rfloat )denom * enterFrac ) {
						enterQuot = enterLimit / denom;
						enterFrac = FCAST( enterQuot );
						if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
							return;
						}
					} else if ( clipside != NULL ) {
						continue;
					}
					axialSide.materialNum = brush->axialMaterialNum[pass * 3 + axis];
					VectorClear( axialPlane.normal );
					axialPlane.normal[axis] = sign;
					axialSide.plane = &axialPlane;
					clipside = &axialSide;
				} else if ( endDist > 0.0f ) {
					leaveDenom = startDist - endDist;
					startout = qfalse;
					if ( leaveFrac * leaveDenom < startDist ) {
						leaveFrac = FCAST( startDist / leaveDenom );
						if ( enterFrac >= leaveFrac ) {
							return;
						}
					}
				}
			}
			if ( pass ) {
				break;
			}
			sign = 1.0f;
			bounds = brush->maxs;
			pass = 1;
		}

		side = brush->sides;
		for ( i = brush->numSides; i != 0; i--, side++ ) {
			plane = side->plane;

			planeDist = ( rfloat )plane->dist + tw->sphere.radius;
			t = ( ( rfloat )plane->normal[2] * tw->sphere.offset[2] + ( rfloat )plane->normal[0] * tw->sphere.offset[0] ) + ( rfloat )plane->normal[1] * tw->sphere.offset[1];
			if ( t > 0.0f ) {
				CM_VectorSubtractR( tw->start, tw->sphere.offset, startPoint );
				VectorSubtract( tw->end, tw->sphere.offset, endPoint );
			} else {
				CM_VectorAddR( tw->start, tw->sphere.offset, startPoint );
				VectorAdd( tw->end, tw->sphere.offset, endPoint );
			}

			startDist = FCAST( ( ( ( rfloat )startPoint[2] * plane->normal[2] ) + ( rfloat )startPoint[1] * plane->normal[1] ) + ( rfloat )startPoint[0] * plane->normal[0] -
							   planeDist );
			endDist = ( ( ( rfloat )endPoint[2] * plane->normal[2] ) + ( rfloat )endPoint[1] * plane->normal[1] ) + ( rfloat )endPoint[0] * plane->normal[0] - planeDist;

			if ( startDist > 0.0f ) {
				denom = FCAST( startDist - endDist );
				if ( endDist > 0.0f ) {
					if ( denom <= 0.0f ) {
						return;
					}
					if ( endDist >= SURFACE_CLIP_EPSILON ) {
						return;
					}
					startout = qfalse;
				}
				enterLimit = ( rfloat )startDist - SURFACE_CLIP_EPSILON;
				if ( enterLimit > ( rfloat )denom * enterFrac ) {
					enterQuot = enterLimit / denom;
					enterFrac = FCAST( enterQuot );
					if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
						return;
					}
				} else if ( clipside != NULL ) {
					continue;
				}
				clipside = side;
			} else if ( endDist > 0.0f ) {
				leaveDenom = startDist - endDist;
				startout = qfalse;
				if ( leaveFrac * leaveDenom < startDist ) {
					leaveFrac = FCAST( startDist / leaveDenom );
					if ( enterFrac >= leaveFrac ) {
						return;
					}
				}
			}
		}
	} else {
		for ( ;; ) {
			for ( axis = 0; axis < 3; axis++ ) {
				startDist = ( tw->start[axis] - bounds[axis] ) * sign -
							tw->maxs[axis];
				endDist = ( ( rfloat )tw->end[axis] - bounds[axis] ) * sign -
						  tw->maxs[axis];

				if ( startDist > 0.0f ) {
					denom = FCAST( startDist - endDist );
					if ( endDist > 0.0f ) {
						if ( denom <= 0.0f ) {
							return;
						}
						if ( endDist >= SURFACE_CLIP_EPSILON ) {
							return;
						}
						startout = qfalse;
					}
					enterLimit = ( rfloat )startDist - SURFACE_CLIP_EPSILON;
					if ( enterLimit > ( rfloat )denom * enterFrac ) {
						enterQuot = enterLimit / denom;
						enterFrac = FCAST( enterQuot );
						if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
							return;
						}
					} else if ( clipside != NULL ) {
						continue;
					}
					axialSide.materialNum = brush->axialMaterialNum[pass * 3 + axis];
					VectorClear( axialPlane.normal );
					axialPlane.normal[axis] = sign;
					axialSide.plane = &axialPlane;
					clipside = &axialSide;
				} else if ( endDist > 0.0f ) {
					leaveDenom = startDist - endDist;
					startout = qfalse;
					if ( leaveFrac * leaveDenom < startDist ) {
						leaveFrac = FCAST( startDist / leaveDenom );
						if ( enterFrac >= leaveFrac ) {
							return;
						}
					}
				}
			}
			if ( pass ) {
				break;
			}
			sign = 1.0f;
			bounds = brush->maxs;
			pass = 1;
		}

		side = brush->sides;
		for ( i = brush->numSides; i != 0; i--, side++ ) {
			plane = side->plane;

			planeDist = plane->dist -
						CM_DotProductR( tw->offsets[plane->signbits],
									   plane->normal, 2, 0, 1 );
			startDist = FCAST( CM_DotProductR( tw->start, plane->normal, 1, 0, 2 ) -
							   planeDist );
			endDist = ( ( ( rfloat )tw->end[1] * plane->normal[1] + ( rfloat )tw->end[2] * plane->normal[2] ) + ( rfloat )tw->end[0] * plane->normal[0] ) - planeDist;

			if ( startDist > 0.0f ) {
				denom = FCAST( startDist - endDist );
				if ( endDist > 0.0f ) {
					if ( denom <= 0.0f ) {
						return;
					}
					if ( endDist >= SURFACE_CLIP_EPSILON ) {
						return;
					}
					startout = qfalse;
				}
				enterLimit = ( rfloat )startDist - SURFACE_CLIP_EPSILON;
				if ( enterLimit > ( rfloat )denom * enterFrac ) {
					enterQuot = enterLimit / denom;
					enterFrac = FCAST( enterQuot );
					if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
						return;
					}
				} else if ( clipside != NULL ) {
					continue;
				}
				clipside = side;
			} else if ( endDist > 0.0f ) {
				leaveDenom = startDist - endDist;
				startout = qfalse;
				if ( leaveFrac * leaveDenom < startDist ) {
					leaveFrac = FCAST( startDist / leaveDenom );
					if ( enterFrac >= leaveFrac ) {
						return;
					}
				}
			}
		}
	}

	tw->trace.contents = brush->contents;

	if ( clipside == NULL ) {
		tw->trace.startsolid = 1;
		if ( startout ) {
			tw->trace.fraction = 0.0f;
			tw->trace.allsolid = 1;
		}
		return;
	}

	tw->trace.fraction = enterFrac;
	VectorCopy( clipside->plane->normal, tw->trace.normal );
	tw->trace.surfaceFlags = cm_materials[clipside->materialNum].surfaceFlags;
	tw->trace.material = ( int )&cm_materials[clipside->materialNum];
}

/* ---- CM_TraceThroughLeaf  0x00423870 ---- VERIFIED */
void CM_TraceThroughLeaf( traceWork_t *tw, const cLeaf_t *leaf ) {
	int i;
	int brushnum, patchnum;
	cbrush_t *b;
	cPatch_t *patch;

	for ( i = 0; i < ( int )leaf->numLeafBrushes; i++ ) {
		brushnum = cm_leafbrushes[leaf->firstLeafBrush + i];
		b = &cm_brushes[brushnum];

		if ( b->checkcount == cm_checkcount ) {
			continue;
		}
		b->checkcount = cm_checkcount;

		if ( !( tw->contents & b->contents ) ) {
			continue;
		}

		CM_TraceThroughBrush( tw, b );
		if ( tw->trace.fraction == 0.0f ) {
			return;
		}
	}

	if ( cm_noCurves->integer ) {
		return;
	}

	for ( i = 0; i < ( int )leaf->numLeafSurfaces; i++ ) {
		patchnum = cm_leafsurfaces[leaf->firstLeafSurface + i];
		patch = &cm_patches[patchnum];

		if ( patch->checkcount == cm_checkcount ) {
			continue;
		}
		patch->checkcount = cm_checkcount;

		if ( !( tw->contents & patch->contents ) ) {
			continue;
		}

		CM_TraceThroughPatch( tw, patch );
		if ( tw->trace.fraction == 0.0f ) {
			return;
		}
	}
}

/* ---- CM_TraceSphereThroughSphere  0x00423950 ---- VERIFIED */
static qboolean CM_TraceSphereThroughSphere( traceWork_t *tw,
											 const vec3_t start,
											 const vec3_t end,
											 const vec3_t sphereOrigin,
											 float sphereRadius ) {
	vec3_t delta, normal;
	float radiusSquared, startDist;
	float deltaDot, deltaLenSq, disc, dist;
#ifdef COD1_VC7
	rfloat epsFrac, enterFrac;
#else
	float baseFrac, enterFrac;
#endif

	VectorSubtract( start, sphereOrigin, delta );

	radiusSquared = ( sphereRadius + tw->sphere.radius ) *
					( sphereRadius + tw->sphere.radius );
	startDist = DotProduct( delta, delta ) - radiusSquared;

	if ( startDist <= 0.0f ) {
		tw->trace.fraction = 0.0f;
		tw->trace.startsolid = 1;
		VectorNormalize2( delta, tw->trace.normal );
		tw->trace.contents = box_brush->contents;

		VectorSubtract( end, sphereOrigin, delta );
		if ( DotProduct( delta, delta ) <= radiusSquared ) {
			tw->trace.allsolid = 1;
		}
		return qfalse;
	}

	deltaDot = DotProduct( tw->delta, delta );
	if ( deltaDot >= 0.0f ) {
		return qtrue;
	}

	deltaLenSq = tw->deltaLengthSquared;
	disc = deltaDot * deltaDot - deltaLenSq * startDist;
	if ( disc < 0.0f ) {
		return qtrue;
	}

	dist = VectorNormalize2( delta, normal );
#ifdef COD1_VC7
	/* retail stores the quotient into deltaDot and keeps the sum on the
	   x87 stack */
	epsFrac = ( dist * SURFACE_CLIP_EPSILON ) / deltaDot;
	deltaDot = FCAST( ( -deltaDot - sqrt( disc ) ) / deltaLenSq );
	enterFrac = deltaDot + epsFrac;
#else
	baseFrac = ( -deltaDot - ( float )sqrt( ( double )disc ) ) / deltaLenSq;
	enterFrac = baseFrac + ( dist * SURFACE_CLIP_EPSILON ) / deltaDot;
#endif

	if ( enterFrac < tw->trace.fraction ) {
		tw->trace.fraction = enterFrac > 0.0f ? enterFrac : 0.0f;
		VectorCopy( normal, tw->trace.normal );
		tw->trace.contents = box_brush->contents;
		return qfalse;
	}

	return qtrue;
}

/* ---- CM_TraceCylinderThroughCylinder  0x00423B30 ---- VERIFIED */
static qboolean CM_TraceCylinderThroughCylinder( traceWork_t *tw,
												 const vec3_t cylinderOrigin,
												 float cylinderHalfHeight,
												 float cylinderRadius ) {
	vec3_t delta, normal;
	float radiusSquared, startDist;
	float halfheight, deltaDot, lengthSquared, disc;
	float dist, epsFrac, baseFrac, hitFrac, z;

	VectorSubtract( tw->start, cylinderOrigin, delta );

	radiusSquared = ( cylinderRadius + tw->sphere.radius ) *
					( cylinderRadius + tw->sphere.radius );
	startDist = ( delta[0] * delta[0] + delta[1] * delta[1] ) - radiusSquared;

	if ( startDist <= 0.0f ) {
		halfheight = tw->sphere.halfheight - tw->sphere.radius +
					 cylinderHalfHeight;

		if ( delta[2] > halfheight || delta[2] < -halfheight ) {
			return qtrue;
		}

		tw->trace.fraction = 0.0f;
		tw->trace.startsolid = 1;
		delta[2] = 0.0f;
		VectorNormalize2( delta, tw->trace.normal );
		tw->trace.contents = box_brush->contents;

		VectorSubtract( tw->end, cylinderOrigin, delta );
		if ( delta[2] <= halfheight && delta[2] >= -halfheight ) {
			tw->trace.allsolid = 1;
		}
		return qfalse;
	}

	deltaDot = tw->delta[0] * delta[0] + tw->delta[1] * delta[1];
	if ( deltaDot >= 0.0f ) {
		return qtrue;
	}

	lengthSquared = tw->delta[0] * tw->delta[0] + tw->delta[1] * tw->delta[1];
	disc = deltaDot * deltaDot - lengthSquared * startDist;
	if ( disc < 0.0f ) {
		return qtrue;
	}

	delta[2] = 0.0f;
	dist = VectorNormalize2( delta, normal );
	epsFrac = ( dist * SURFACE_CLIP_EPSILON ) / deltaDot;
	baseFrac = ( -deltaDot - ( float )sqrt( ( double )disc ) ) / lengthSquared;
	hitFrac = baseFrac + epsFrac;

	if ( !( hitFrac < tw->trace.fraction ) ) {
		return qtrue;
	}

	halfheight = tw->sphere.halfheight - tw->sphere.radius + cylinderHalfHeight;
	z = ( ( hitFrac - epsFrac ) * tw->delta[2] + tw->start[2] ) -
		cylinderOrigin[2];

	if ( z > halfheight || z < -halfheight ) {
		return qtrue;
	}

	tw->trace.fraction = hitFrac > 0.0f ? hitFrac : 0.0f;
	VectorCopy( normal, tw->trace.normal );
	tw->trace.contents = box_brush->contents;
	return qfalse;
}

/* ---- CM_TraceCapsuleThroughCapsule  0x00423D90 ---- VERIFIED */
void CM_TraceCapsuleThroughCapsule( traceWork_t *tw ) {
	vec3_t startTop, startBottom, endTop, endBottom;
	vec3_t center, symetricSize[2], topOrigin, bottomOrigin;
	float radius, halfwidth, halfheight, offs;
	int i;

	if ( tw->bounds[0][0] > cm_boxModel.maxs[0] + 1.0f ) {
		return;
	}
	if ( tw->bounds[0][1] > cm_boxModel.maxs[1] + 1.0f ) {
		return;
	}
	if ( tw->bounds[0][2] > cm_boxModel.maxs[2] + 1.0f ) {
		return;
	}
	if ( cm_boxModel.mins[0] - 1.0f > tw->bounds[1][0] ) {
		return;
	}
	if ( cm_boxModel.mins[1] - 1.0f > tw->bounds[1][1] ) {
		return;
	}
	if ( cm_boxModel.mins[2] - 1.0f > tw->bounds[1][2] ) {
		return;
	}

	VectorAdd( tw->start, tw->sphere.offset, startTop );
	VectorSubtract( tw->start, tw->sphere.offset, startBottom );
	VectorAdd( tw->end, tw->sphere.offset, endTop );
	VectorSubtract( tw->end, tw->sphere.offset, endBottom );

	for ( i = 0; i < 3; i++ ) {
		center[i] = ( cm_boxModel.mins[i] + cm_boxModel.maxs[i] ) * 0.5f;
		symetricSize[0][i] = cm_boxModel.mins[i] - center[i];
		symetricSize[1][i] = cm_boxModel.maxs[i] - center[i];
	}

	halfwidth = symetricSize[1][0];
	halfheight = symetricSize[1][2];
	radius = halfwidth > halfheight ? halfheight : halfwidth;
	offs = halfheight - radius;

	topOrigin[0] = center[0];
	topOrigin[1] = center[1];
	topOrigin[2] = center[2] + offs;

	bottomOrigin[0] = center[0];
	bottomOrigin[1] = center[1];
	bottomOrigin[2] = center[2] - offs;

	if ( startBottom[2] > topOrigin[2] ) {
		if ( !CM_TraceSphereThroughSphere( tw, startBottom, endBottom,
										   topOrigin, radius ) ) {
			return;
		}
		if ( tw->delta[2] >= 0.0f ) {
			return;
		}
	} else if ( startTop[2] < bottomOrigin[2] ) {
		if ( !CM_TraceSphereThroughSphere( tw, startTop, endTop,
										   bottomOrigin, radius ) ) {
			return;
		}
		if ( tw->delta[2] <= 0.0f ) {
			return;
		}
	}

	if ( CM_TraceCylinderThroughCylinder( tw, center, offs, radius ) ) {
		if ( endBottom[2] > topOrigin[2] ) {
			if ( startBottom[2] <= topOrigin[2] ) {
				CM_TraceSphereThroughSphere( tw, startBottom, endBottom,
											 topOrigin, radius );
			}
		} else if ( endTop[2] < bottomOrigin[2] &&
					startTop[2] >= bottomOrigin[2] ) {
			CM_TraceSphereThroughSphere( tw, startTop, endTop,
										 bottomOrigin, radius );
		}
	}
}

/* ---- CM_TraceBoundingBoxThroughCapsule  0x004240A0 ---- VERIFIED */
void CM_TraceBoundingBoxThroughCapsule( traceWork_t *tw ) {
	vec3_t offset, size[2];
	int i;

	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( cm_boxModel.mins[i] + cm_boxModel.maxs[i] ) * 0.5f;
		size[0][i] = cm_boxModel.mins[i] - offset[i];
		size[1][i] = cm_boxModel.maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	tw->sphere.use = qtrue;
	tw->sphere.radius = ( size[1][0] > size[1][2] ) ? size[1][2] : size[1][0];
	tw->sphere.halfheight = size[1][2];
	VectorSet( tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius );
	tw->sphereExtents[0] = tw->sphere.radius;
	tw->sphereExtents[1] = tw->sphere.radius;
	tw->sphereExtents[2] = tw->sphere.halfheight;

	CM_TempBoxModel( tw->mins, tw->maxs, box_brush->contents, qfalse );
	CM_TraceThroughBrush( tw, box_brush );
}

/* ---- CM_TraceThroughTree  0x00424230 ---- VERIFIED */
void CM_TraceThroughTree( traceWork_t *tw, int num, float p1f, float p2f,
						  const vec3_t p1, const vec3_t p2 ) {
	const cNode_t *node;
	const cplane_t *plane;
	float t1, t2, offset;
	float idist, frac, frac2, midf;
	vec3_t mid;
	int side;

	if ( tw->trace.fraction <= p1f ) {
		return;
	}

	if ( num < 0 ) {
		CM_TraceThroughLeaf( tw, &cm_leafs[-1 - num] );
		return;
	}

	node = cm_nodes + num;
	plane = node->plane;

	if ( plane->type < 3 ) {
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = tw->maxs[plane->type];
	} else {
		t1 = DotProduct( plane->normal, p1 ) - plane->dist;
		t2 = DotProduct( plane->normal, p2 ) - plane->dist;
		offset = tw->isPoint ? 0.0f : 2048.0f;
	}

	if ( t1 >= offset + 1.0f && t2 >= offset + 1.0f ) {
		CM_TraceThroughTree( tw, node->children[0], p1f, p2f, p1, p2 );
		return;
	}
	if ( t1 < -offset - 1.0f && t2 < -offset - 1.0f ) {
		CM_TraceThroughTree( tw, node->children[1], p1f, p2f, p1, p2 );
		return;
	}

	if ( t1 < t2 ) {
		idist = 1.0f / ( t1 - t2 );
		side = 1;
		frac2 = ( t1 + offset + SURFACE_CLIP_EPSILON ) * idist;
		frac = ( t1 - offset + SURFACE_CLIP_EPSILON ) * idist;
	} else if ( t1 > t2 ) {
		idist = 1.0f / ( t1 - t2 );
		side = 0;
		frac2 = ( t1 - offset - SURFACE_CLIP_EPSILON ) * idist;
		frac = ( t1 + offset + SURFACE_CLIP_EPSILON ) * idist;
	} else {
		side = 0;
		frac = 1.0f;
		frac2 = 0.0f;
	}

	if ( frac < 0.0f ) {
		frac = 0.0f;
	} else if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	midf = p1f + ( p2f - p1f ) * frac;
	mid[0] = p1[0] + ( p2[0] - p1[0] ) * frac;
	mid[1] = p1[1] + ( p2[1] - p1[1] ) * frac;
	mid[2] = p1[2] + ( p2[2] - p1[2] ) * frac;

	CM_TraceThroughTree( tw, node->children[side], p1f, midf, p1, mid );

	if ( frac2 < 0.0f ) {
		frac2 = 0.0f;
	} else if ( frac2 > 1.0f ) {
		frac2 = 1.0f;
	}

	midf = p1f + ( p2f - p1f ) * frac2;
	mid[0] = p1[0] + ( p2[0] - p1[0] ) * frac2;
	mid[1] = p1[1] + ( p2[1] - p1[1] ) * frac2;
	mid[2] = p1[2] + ( p2[2] - p1[2] ) * frac2;

	CM_TraceThroughTree( tw, node->children[side ^ 1], midf, p2f, mid, p2 );
}

/* ---- CM_Trace  0x00424530 ---- VERIFIED */
void CM_Trace( trace_t *results, const vec3_t start, const vec3_t end,
			   const vec3_t mins, const vec3_t maxs, clipHandle_t model,
			   const vec3_t origin, int brushmask, qboolean capsule,
			   const cmSphereRecord_t *sphere ) {
	cmodel_t *cmod;
	traceWork_t tw;
	vec3_t offset;
	int i;

	( void )origin;

	cmod = CM_ClipHandleToModel( model );

	cm_checkcount++;
	Com_Memset( &tw, 0, sizeof( tw ) );
	tw.trace.fraction = results->fraction;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	tw.contents = brushmask;

	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5f;
		tw.mins[i] = mins[i] - offset[i];
		tw.maxs[i] = maxs[i] - offset[i];
		tw.start[i] = start[i] + offset[i];
		tw.end[i] = end[i] + offset[i];
		tw.delta[i] = tw.end[i] - tw.start[i];
	}
	tw.deltaLengthSquared = DotProduct( tw.delta, tw.delta );

	if ( sphere ) {
		/* retail copies sphere and sphereExtents as one 36-byte record */
		*( cmSphereRecord_t * )&tw.sphere = *sphere;
	} else {
		tw.sphere.use = capsule;
		tw.sphere.radius = tw.maxs[0] > tw.maxs[2] ? tw.maxs[2] : tw.maxs[0];
		tw.sphere.halfheight = tw.maxs[2];
		tw.sphere.offset[0] = 0.0f;
		tw.sphere.offset[1] = 0.0f;
		tw.sphere.offset[2] = tw.maxs[2] - tw.sphere.radius;
	}

	tw.maxsSum = tw.maxs[0] + tw.maxs[1] + tw.maxs[2];

	tw.offsets[0][0] = tw.mins[0];  tw.offsets[0][1] = tw.mins[1];  tw.offsets[0][2] = tw.mins[2];
	tw.offsets[1][0] = tw.maxs[0];  tw.offsets[1][1] = tw.mins[1];  tw.offsets[1][2] = tw.mins[2];
	tw.offsets[2][0] = tw.mins[0];  tw.offsets[2][1] = tw.maxs[1];  tw.offsets[2][2] = tw.mins[2];
	tw.offsets[3][0] = tw.maxs[0];  tw.offsets[3][1] = tw.maxs[1];  tw.offsets[3][2] = tw.mins[2];
	tw.offsets[4][0] = tw.mins[0];  tw.offsets[4][1] = tw.mins[1];  tw.offsets[4][2] = tw.maxs[2];
	tw.offsets[5][0] = tw.maxs[0];  tw.offsets[5][1] = tw.mins[1];  tw.offsets[5][2] = tw.maxs[2];
	tw.offsets[6][0] = tw.mins[0];  tw.offsets[6][1] = tw.maxs[1];  tw.offsets[6][2] = tw.maxs[2];
	tw.offsets[7][0] = tw.maxs[0];  tw.offsets[7][1] = tw.maxs[1];  tw.offsets[7][2] = tw.maxs[2];

	if ( tw.sphere.use ) {
		for ( i = 0; i < 3; i++ ) {
			if ( tw.start[i] < tw.end[i] ) {
				tw.bounds[0][i] = tw.start[i] -
								  ( float )fabs( tw.sphere.offset[i] ) -
								  tw.sphere.radius;
				tw.bounds[1][i] = tw.end[i] +
								  ( float )fabs( tw.sphere.offset[i] ) +
								  tw.sphere.radius;
			} else {
				tw.bounds[0][i] = tw.end[i] -
								  ( float )fabs( tw.sphere.offset[i] ) -
								  tw.sphere.radius;
				tw.bounds[1][i] = tw.start[i] +
								  ( float )fabs( tw.sphere.offset[i] ) +
								  tw.sphere.radius;
			}
		}
	} else {
		for ( i = 0; i < 3; i++ ) {
			if ( tw.start[i] < tw.end[i] ) {
				tw.bounds[0][i] = tw.start[i] + tw.mins[i];
				tw.bounds[1][i] = tw.end[i] + tw.maxs[i];
			} else {
				tw.bounds[0][i] = tw.end[i] + tw.mins[i];
				tw.bounds[1][i] = tw.start[i] + tw.maxs[i];
			}
		}
	}

	if ( start[0] == end[0] && start[1] == end[1] && start[2] == end[2] ) {
		if ( model ) {
			if ( model == CM_TEMP_CAPSULE_HANDLE ) {
				if ( tw.contents & box_brush->contents ) {
					if ( tw.sphere.use ) {
						CM_TestCapsuleInCapsule( &tw );
					} else {
						CM_TestBoundingBoxInCapsule( &tw );
					}
				}
			} else {
				CM_TestInLeaf( &tw, &cmod->leaf );
			}
		} else {
			CM_PositionTest( &tw );
		}
	} else {
		tw.isPoint = ( tw.maxs[0] + tw.maxs[1] + tw.maxs[2] == 0.0f );

		if ( tw.sphere.use ) {
			for ( i = 0; i < 3; i++ ) {
				tw.sphereExtents[i] = tw.sphere.radius +
									  CM_Q_fabs( tw.sphere.offset[i] );
			}
		}

		if ( model ) {
			if ( model == CM_TEMP_CAPSULE_HANDLE ) {
				if ( tw.contents & box_brush->contents ) {
					if ( tw.sphere.use ) {
						CM_TraceCapsuleThroughCapsule( &tw );
					} else {
						CM_TraceBoundingBoxThroughCapsule( &tw );
					}
				}
			} else {
				CM_TraceThroughLeaf( &tw, &cmod->leaf );
			}
		} else {
			CM_TraceThroughTree( &tw, 0, 0.0f, tw.trace.fraction,
								 tw.start, tw.end );
		}
	}

	tw.trace.endpos[0] = start[0] + tw.delta[0] * tw.trace.fraction;
	tw.trace.endpos[1] = start[1] + tw.delta[1] * tw.trace.fraction;
	tw.trace.endpos[2] = start[2] + tw.delta[2] * tw.trace.fraction;

	*results = tw.trace;
}

/* ---- CM_BoxTrace  0x00424B50 ---- VERIFIED */
void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
				  const vec3_t mins, const vec3_t maxs, clipHandle_t model,
				  int brushmask, qboolean capsule ) {
	results->fraction = 1.0f;
	CM_Trace( results, start, end, mins, maxs, model, vec3_origin,
			  brushmask, capsule, NULL );
}

/* ---- CM_TransformedBoxTrace  0x00424B80 ---- VERIFIED */
void CM_TransformedBoxTrace( trace_t *results, const vec3_t start,
							 const vec3_t end, const vec3_t mins,
							 const vec3_t maxs, clipHandle_t model,
							 int brushmask, const vec3_t origin,
							 const vec3_t angles, qboolean capsule ) {
	trace_t trace;
	vec3_t start_l, end_l;
	qboolean rotated;
	vec3_t offset;
	vec3_t symetricSize[2];
	vec3_t matrix[3], transpose[3];
	int i;
	float halfwidth;
	float halfheight;
	float t;
	cmSphereRecord_t sphere;
#ifndef COD1_VC7
	float x, y, z;
#endif

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5f;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
		start_l[i] = start[i] + offset[i];
		end_l[i] = end[i] + offset[i];
	}

	/* retail 0x424C7F: origin comes off the stored float sums, so each
	   point is rounded twice */
	VectorSubtract( start_l, origin, start_l );
	VectorSubtract( end_l, origin, end_l );

	if ( model != CM_TEMP_BOX_HANDLE &&
		 ( angles[0] || angles[1] || angles[2] ) ) {
		rotated = qtrue;
	} else {
		rotated = qfalse;
	}

	halfwidth = symetricSize[1][0];
	halfheight = symetricSize[1][2];

	/* retail leaves sphere.extents unset; CM_Trace rebuilds them for capsules */
	sphere.sphere.use = capsule;
	sphere.sphere.radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	sphere.sphere.halfheight = halfheight;
	t = halfheight - sphere.sphere.radius;

	if ( rotated ) {
		CreateRotationMatrix( angles, matrix );
		RotatePoint( start_l, matrix );
		RotatePoint( end_l, matrix );
		sphere.sphere.offset[0] = matrix[0][2] * t;
		sphere.sphere.offset[1] = -matrix[1][2] * t;
		sphere.sphere.offset[2] = matrix[2][2] * t;
	} else {
		VectorSet( sphere.sphere.offset, 0, 0, t );
	}

	trace.fraction = results->fraction;
	CM_Trace( &trace, start_l, end_l, symetricSize[0], symetricSize[1], model,
			  origin, brushmask, capsule, &sphere );

	if ( rotated && trace.fraction < results->fraction ) {
		TransposeMatrix( matrix, transpose );
#ifdef COD1_VC7
		RotatePoint( trace.normal, transpose );
#else
		/* retail 0x424F4D: VC7 sums each row of this inlined RotatePoint
		   z, y then x */
		x = trace.normal[0];
		y = trace.normal[1];
		z = trace.normal[2];
		trace.normal[0] = transpose[0][2] * z + transpose[0][1] * y +
						  transpose[0][0] * x;
		trace.normal[1] = transpose[1][2] * z + transpose[1][1] * y +
						  transpose[1][0] * x;
		trace.normal[2] = transpose[2][2] * z + transpose[2][1] * y +
						  transpose[2][0] * x;
#endif
	}

	trace.endpos[0] = start[0] + trace.fraction * ( end[0] - start[0] );
	trace.endpos[1] = start[1] + trace.fraction * ( end[1] - start[1] );
	trace.endpos[2] = start[2] + trace.fraction * ( end[2] - start[2] );

	*results = trace;
}

/* ---- CM_TransformedBoxTraceExternal  0x00425040 ---- VERIFIED */
void CM_TransformedBoxTraceExternal( trace_t *results, const vec3_t start,
									 const vec3_t end, const vec3_t mins,
									 const vec3_t maxs, clipHandle_t model,
									 int brushmask, const vec3_t origin,
									 const vec3_t angles, qboolean capsule ) {
	results->fraction = 1.0f;
	CM_TransformedBoxTrace( results, start, end, mins, maxs, model, brushmask,
							origin, angles, capsule );
}

/* ---- CM_SightTraceThroughPatch  0x00425080 ---- VERIFIED */
qboolean CM_SightTraceThroughPatch( const traceWork_t *tw,
									const cPatch_t *patch ) {
	if ( !CM_TraceMayIntersectBox( tw, patch->mins, patch->maxs ) ) {
		return qtrue;
	}

	if ( patch->pc ) {
		return CM_SightTraceThroughPatchCollide( ( traceWork_t * )tw,
												 patch->pc );
	}
	return CM_SightTraceThroughTerrainCollide( ( traceWork_t * )tw,
											   patch->tc );
}

/* ---- CM_SightTraceThroughBrush  0x004250C0 ---- VERIFIED */
static int CM_SightTraceThroughBrush( const traceWork_t *tw,
									  const cbrush_t *brush ) {
	float enterFrac, leaveFrac;
	const cbrushside_t *side;
	const cplane_t *plane;
	const float *bounds;
	rfloat startPoint[3];
	vec3_t endPoint;
	float startDist, denom, sign;
	/* retail keeps endDist, planeDist, the leave denominator and the enter
	   quotient on the x87 stack (_PC_53); only startDist, the enter
	   denominator, enterFrac and leaveFrac are stored as floats */
	rfloat endDist, planeDist;
	rfloat leaveDenom, enterQuot;
	int pass, axis, i;
#ifdef COD1_VC7
	/* Seven dead locals: no code, but VC7 picks the operand order of the
	   plane/offset products from the function's symbol count (period 8);
	   seven gives retail's order for all but the sphere n2*o2 product. */
	int dz0, dz1, dz2, dz3, dz4, dz5, dz6;
#endif

#ifdef COD1_VC7
	dz0 = 0;
	dz1 = 0;
	dz2 = 0;
	dz3 = 0;
	dz4 = 0;
	dz5 = 0;
	dz6 = 0;
#endif
	enterFrac = 0.0f;
	leaveFrac = 1.0f;
	sign = -1.0f;
	bounds = brush->mins;
	pass = 0;

	if ( tw->sphere.use ) {
		for ( ;; ) {
			for ( axis = 0; axis < 3; axis++ ) {
				startDist = ( tw->start[axis] - bounds[axis] ) * sign -
							tw->sphereExtents[axis];
				endDist = ( ( rfloat )tw->end[axis] - bounds[axis] ) * sign -
						  tw->sphereExtents[axis];

				if ( startDist > 0.0f ) {
					denom = FCAST( startDist - endDist );
					if ( endDist > 0.0f ) {
						return 0;
					}
					if ( ( rfloat )denom * enterFrac < startDist ) {
						enterQuot = ( rfloat )startDist / denom;
						enterFrac = FCAST( enterQuot );
						if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
							return 0;
						}
					}
				} else if ( endDist > 0.0f ) {
					leaveDenom = startDist - endDist;
					if ( leaveFrac * leaveDenom < startDist ) {
						leaveFrac = FCAST( startDist / leaveDenom );
						if ( enterFrac >= leaveFrac ) {
							return 0;
						}
					}
				}
			}
			if ( pass ) {
				break;
			}
			sign = 1.0f;
			bounds = brush->maxs;
			pass = 1;
		}

		side = brush->sides;
		for ( i = brush->numSides; i != 0; i--, side++ ) {
			plane = side->plane;

			planeDist = ( rfloat )plane->dist + tw->sphere.radius;
			/* The parenthesis nesting is load-bearing (VC7 keeps paren nodes and
			   they move its operand ranking): it gives retail's ( n0*o0 + o1*n1 )
			   + o2*n2 sum and, with the dead locals above, retail's operand order
			   for n0*o0 and o1*n1; n2*o2 still loads the normal first
			   (retail 0x0042522E loads the offset first). */
			if ( ( ( ( rfloat )plane->normal[0] * ( tw->sphere.offset[0] ) ) +
				 ( ( rfloat )( ( plane->normal[1] ) ) * ( tw->sphere.offset[1] ) ) ) +
				 ( rfloat )( plane->normal[2] ) * tw->sphere.offset[2] > 0.0f ) {
				startPoint[0] = ( rfloat )tw->start[0] - tw->sphere.offset[0];
				startPoint[1] = ( rfloat )tw->start[1] - tw->sphere.offset[1];
				startPoint[2] = ( rfloat )tw->start[2] - tw->sphere.offset[2];
				VectorSubtract( tw->end, tw->sphere.offset, endPoint );
			} else {
				startPoint[0] = ( rfloat )tw->start[0] + tw->sphere.offset[0];
				startPoint[1] = ( rfloat )tw->start[1] + tw->sphere.offset[1];
				startPoint[2] = ( rfloat )tw->start[2] + tw->sphere.offset[2];
				VectorAdd( tw->end, tw->sphere.offset, endPoint );
			}

			startDist = FCAST( ( ( startPoint[0] * plane->normal[0] ) + startPoint[2] * plane->normal[2] ) + startPoint[1] * plane->normal[1] - planeDist );
			endDist = ( ( ( rfloat )endPoint[0] * plane->normal[0] ) + ( rfloat )endPoint[2] * plane->normal[2] ) + ( rfloat )endPoint[1] * plane->normal[1] - planeDist;

			if ( startDist > 0.0f ) {
				denom = FCAST( startDist - endDist );
				if ( endDist > 0.0f ) {
					return 0;
				}
				if ( ( rfloat )denom * enterFrac < startDist ) {
					enterQuot = ( rfloat )startDist / denom;
					enterFrac = FCAST( enterQuot );
					if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
						return 0;
					}
				}
			} else if ( endDist > 0.0f ) {
				leaveDenom = startDist - endDist;
				if ( leaveFrac * leaveDenom < startDist ) {
					leaveFrac = FCAST( startDist / leaveDenom );
					if ( enterFrac >= leaveFrac ) {
						return 0;
					}
				}
			}
		}
	} else {
		for ( ;; ) {
			for ( axis = 0; axis < 3; axis++ ) {
				startDist = ( tw->start[axis] - bounds[axis] ) * sign -
							tw->maxs[axis];
				endDist = ( ( rfloat )tw->end[axis] - bounds[axis] ) * sign -
						  tw->maxs[axis];

				if ( startDist > 0.0f ) {
					denom = FCAST( startDist - endDist );
					if ( endDist > 0.0f ) {
						return 0;
					}
					if ( ( rfloat )denom * enterFrac < startDist ) {
						enterQuot = ( rfloat )startDist / denom;
						enterFrac = FCAST( enterQuot );
						if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
							return 0;
						}
					}
				} else if ( endDist > 0.0f ) {
					leaveDenom = startDist - endDist;
					if ( leaveFrac * leaveDenom < startDist ) {
						leaveFrac = FCAST( startDist / leaveDenom );
						if ( enterFrac >= leaveFrac ) {
							return 0;
						}
					}
				}
			}
			if ( pass ) {
				break;
			}
			sign = 1.0f;
			bounds = brush->maxs;
			pass = 1;
		}

		side = brush->sides;
		for ( i = brush->numSides; i != 0; i--, side++ ) {
			plane = side->plane;

			planeDist = plane->dist -
						CM_DotProductR( tw->offsets[plane->signbits],
									   plane->normal, 2, 1, 0 );
			startDist = FCAST( CM_DotProductR( tw->start, plane->normal, 1, 0, 2 ) -
							   planeDist );
			endDist = ( ( rfloat )( tw->end )[0] * ( plane->normal )[0] + ( rfloat )( tw->end )[1] * ( plane->normal )[1] + ( rfloat )( tw->end )[2] * ( ( ( plane->normal )[2] ) ) ) - planeDist;

			if ( startDist > 0.0f ) {
				denom = FCAST( startDist - endDist );
				if ( endDist > 0.0f ) {
					return 0;
				}
				if ( ( rfloat )denom * enterFrac < startDist ) {
					enterQuot = ( rfloat )startDist / denom;
					enterFrac = FCAST( enterQuot );
					if ( CM_UNROUNDED_ENTER_FRAC >= leaveFrac ) {
						return 0;
					}
				}
			} else if ( endDist > 0.0f ) {
				leaveDenom = startDist - endDist;
				if ( leaveFrac * leaveDenom < startDist ) {
					leaveFrac = FCAST( startDist / leaveDenom );
					if ( enterFrac >= leaveFrac ) {
						return 0;
					}
				}
			}
		}
	}

	return ( int )( brush - cm_brushes ) + 1;
}

/* ---- CM_SightTraceThroughLeaf  0x004255E0 ---- VERIFIED */
int CM_SightTraceThroughLeaf( const traceWork_t *tw, const cLeaf_t *leaf ) {
	int i, hit;
	int brushnum, patchnum;
	cbrush_t *b;
	cPatch_t *patch;

	for ( i = 0; i < ( int )leaf->numLeafBrushes; i++ ) {
		brushnum = cm_leafbrushes[leaf->firstLeafBrush + i];
		b = &cm_brushes[brushnum];

		if ( b->checkcount == cm_checkcount ) {
			continue;
		}
		b->checkcount = cm_checkcount;

		if ( !( tw->contents & b->contents ) ) {
			continue;
		}

		hit = CM_SightTraceThroughBrush( tw, b );
		if ( hit ) {
			return hit;
		}
	}

	if ( !cm_noCurves->integer ) {
		for ( i = 0; i < ( int )leaf->numLeafSurfaces; i++ ) {
			patchnum = cm_leafsurfaces[leaf->firstLeafSurface + i];
			patch = &cm_patches[patchnum];

			if ( patch->checkcount == cm_checkcount ) {
				continue;
			}
			patch->checkcount = cm_checkcount;

			if ( !( tw->contents & patch->contents ) ) {
				continue;
			}

			if ( !CM_SightTraceThroughPatch( tw, patch ) ) {
				return cm_numBrushes + ( int )( patch - cm_patches ) + 1;
			}
		}
	}

	return 0;
}

/* ---- CM_SightTraceSphereThroughSphere  0x00425720 ---- VERIFIED */
static qboolean CM_SightTraceSphereThroughSphere( const traceWork_t *tw,
												  const vec3_t start,
												  const vec3_t end,
												  const vec3_t sphereOrigin,
												  float sphereRadius ) {
	vec3_t delta, normal;
	float radiusSquared, startDist;
	float deltaDot, deltaLenSq, disc, dist, epsNum;

	( void )end;

	VectorSubtract( start, sphereOrigin, delta );

	radiusSquared = ( sphereRadius + tw->sphere.radius ) *
					( sphereRadius + tw->sphere.radius );
	startDist = DotProduct( delta, delta ) - radiusSquared;

	if ( startDist <= 0.0f ) {
		return qfalse;
	}

	deltaDot = DotProduct( tw->delta, delta );
	if ( deltaDot >= 0.0f ) {
		return qtrue;
	}

	deltaLenSq = tw->deltaLengthSquared;
	disc = deltaDot * deltaDot - deltaLenSq * startDist;
	if ( disc < 0.0f ) {
		return qtrue;
	}

	/* retail 0x4257DA-0x425815: only deltaDot * SURFACE_CLIP_EPSILON is
	   stored as a float (var_24).  The root, the quotient, epsNum / dist and
	   their sum stay on the x87 stack through the compare (the 0x42580F store
	   of the quotient into deltaDot is never reloaded), so the modern build
	   keeps them double; retail runs the FPU at _PC_53 and VectorNormalize2
	   returns a float-rounded length. */
	epsNum = deltaDot * SURFACE_CLIP_EPSILON;
	dist = VectorNormalize2( delta, normal );

#ifdef COD1_VC7
	deltaDot = FCAST( ( -deltaDot - sqrt( disc ) ) / deltaLenSq );
	if ( deltaDot + epsNum / dist < tw->trace.fraction ) {
		return qfalse;
	}
#else
	if ( ( -deltaDot - sqrt( ( double )disc ) ) / deltaLenSq +
		 ( double )epsNum / dist < tw->trace.fraction ) {
		return qfalse;
	}
#endif
	return qtrue;
}

/* ---- CM_SightTraceCylinderThroughCylinder  0x00425840 ---- VERIFIED */
static qboolean CM_SightTraceCylinderThroughCylinder( const traceWork_t *tw,
													  const vec3_t origin,
													  float halfheightIn,
													  float radiusIn ) {
	vec3_t delta, normal;
	float radiusSquared, startDist;
	float deltaDot, deltaLenSq, disc, dist, epsNum, epsFrac, hit;
	rfloat hitSum, hh, z;

	delta[0] = tw->start[0] - origin[0];
	delta[1] = tw->start[1] - origin[1];
	z = ( double )tw->start[2] - origin[2];

	radiusSquared = ( radiusIn + tw->sphere.radius ) *
					( radiusIn + tw->sphere.radius );
	startDist = ( delta[0] * delta[0] + delta[1] * delta[1] ) - radiusSquared;

	if ( startDist <= 0.0f ) {
		/* retail 0x42585B keeps start[2] - origin[2] on the x87 stack (it
		   never stores delta[2]) and 0x42589A keeps the half height there
		   too, so both are compared unrounded (FPU at _PC_53) */
		hh = ( double )tw->sphere.halfheight - tw->sphere.radius + halfheightIn;

		if ( z > hh || z < -hh ) {
			return qtrue;
		}
		return qfalse;
	}

	deltaDot = tw->delta[0] * delta[0] + tw->delta[1] * delta[1];
	if ( deltaDot >= 0.0f ) {
		return qtrue;
	}

	deltaLenSq = tw->deltaLengthSquared;
	disc = deltaDot * deltaDot - deltaLenSq * startDist;
	if ( disc < 0.0f ) {
		return qtrue;
	}

	epsNum = deltaDot * SURFACE_CLIP_EPSILON;
	delta[2] = 0.0f;
	dist = VectorNormalize2( delta, normal );
	/* retail 0x42593F stores epsFrac as a float */
	epsFrac = epsNum / dist;

	/* retail 0x425943-0x425955: the root, the quotient and the sum stay on
	   the x87 stack (FPU at _PC_53).  0x42595D compares that unrounded sum;
	   0x425959 fst keeps a float copy, which 0x42597E reloads for the
	   height test. */
	hitSum = ( -deltaDot - sqrt( ( double )disc ) ) / deltaLenSq + epsFrac;
	hit = FCAST( hitSum );
	if ( !( hitSum < tw->trace.fraction ) ) {
		return qtrue;
	}

	/* retail 0x42596E-0x42598C: the half height and z are never stored */
	hh = ( double )tw->sphere.halfheight - tw->sphere.radius + halfheightIn;
	z = ( ( double )hit - epsFrac ) * tw->delta[2] + tw->start[2] -
		origin[2];

	if ( z > hh || z < -hh ) {
		return qtrue;
	}

	return qfalse;
}

/* ---- CM_SightTraceCapsuleThroughCapsule  0x004259C0 ---- VERIFIED */
int CM_SightTraceCapsuleThroughCapsule( const traceWork_t *tw ) {
	vec3_t startTop, startBottom, endTop, endBottom;
	vec3_t center, symetricSize[2], topOrigin, bottomOrigin;
	float radius, halfwidth, halfheight, offs;
	int i;

	if ( tw->bounds[0][0] > cm_boxModel.maxs[0] + 1.0f ||
		 tw->bounds[0][1] > cm_boxModel.maxs[1] + 1.0f ||
		 tw->bounds[0][2] > cm_boxModel.maxs[2] + 1.0f ||
		 cm_boxModel.mins[0] - 1.0f > tw->bounds[1][0] ||
		 cm_boxModel.mins[1] - 1.0f > tw->bounds[1][1] ||
		 cm_boxModel.mins[2] - 1.0f > tw->bounds[1][2] ) {
		return 0;
	}

	VectorAdd( tw->start, tw->sphere.offset, startTop );
	VectorSubtract( tw->start, tw->sphere.offset, startBottom );
	VectorAdd( tw->end, tw->sphere.offset, endTop );
	VectorSubtract( tw->end, tw->sphere.offset, endBottom );

	for ( i = 0; i < 3; i++ ) {
		center[i] = ( cm_boxModel.mins[i] + cm_boxModel.maxs[i] ) * 0.5f;
		symetricSize[0][i] = cm_boxModel.mins[i] - center[i];
		symetricSize[1][i] = cm_boxModel.maxs[i] - center[i];
	}

	halfwidth = symetricSize[1][0];
	halfheight = symetricSize[1][2];
	radius = halfwidth > halfheight ? halfheight : halfwidth;
	offs = halfheight - radius;

	topOrigin[0] = center[0];
	topOrigin[1] = center[1];
	topOrigin[2] = center[2] + offs;

	bottomOrigin[0] = center[0];
	bottomOrigin[1] = center[1];
	bottomOrigin[2] = center[2] - offs;

	if ( startBottom[2] > topOrigin[2] ) {
		if ( !CM_SightTraceSphereThroughSphere( tw, startBottom, endBottom,
												topOrigin, radius ) ) {
			return -1;
		}
		if ( tw->delta[2] >= 0.0f ) {
			return 0;
		}
	} else if ( startTop[2] < bottomOrigin[2] ) {
		if ( !CM_SightTraceSphereThroughSphere( tw, startTop, endTop,
												bottomOrigin, radius ) ) {
			return -1;
		}
		if ( tw->delta[2] <= 0.0f ) {
			return 0;
		}
	}

	if ( !CM_SightTraceCylinderThroughCylinder( tw, center, offs,
												radius ) ) {
		return -1;
	}

	if ( endBottom[2] > topOrigin[2] ) {
		if ( startBottom[2] <= topOrigin[2] &&
			 !CM_SightTraceSphereThroughSphere( tw, startBottom, endBottom,
												topOrigin, radius ) ) {
			return -1;
		}
	} else if ( endTop[2] < bottomOrigin[2] &&
				startTop[2] >= bottomOrigin[2] &&
				!CM_SightTraceSphereThroughSphere( tw, startTop, endTop,
												   bottomOrigin, radius ) ) {
		return -1;
	}

	return 0;
}

/* ---- CM_SightTraceBoundingBoxThroughCapsule  0x00425CA0 ---- VERIFIED */
int CM_SightTraceBoundingBoxThroughCapsule( traceWork_t *tw ) {
	vec3_t offset, size[2];
	int i;

	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( cm_boxModel.mins[i] + cm_boxModel.maxs[i] ) * 0.5f;
		size[0][i] = cm_boxModel.mins[i] - offset[i];
		size[1][i] = cm_boxModel.maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	tw->sphere.use = qtrue;
	tw->sphere.radius = ( size[1][0] > size[1][2] ) ? size[1][2] : size[1][0];
	tw->sphere.halfheight = size[1][2];
	VectorSet( tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius );
	tw->sphereExtents[0] = tw->sphere.radius;
	tw->sphereExtents[1] = tw->sphere.radius;
	tw->sphereExtents[2] = tw->sphere.halfheight;

	CM_TempBoxModel( tw->mins, tw->maxs, box_brush->contents, qfalse );
	return CM_SightTraceThroughBrush( tw, box_brush );
}

/* ---- CM_SightTraceThroughTree  0x00425E30 ---- VERIFIED */
int CM_SightTraceThroughTree( const traceWork_t *tw, int num, float p1f,
							  float p2f, const vec3_t p1, const vec3_t p2 ) {
	const cNode_t *node;
	const cplane_t *plane;
	float t1, t2, offset;
	float idist, frac, frac2, midf;
	vec3_t mid;
	int side, hit;

	if ( num < 0 ) {
		return CM_SightTraceThroughLeaf( tw, &cm_leafs[-1 - num] );
	}

	node = cm_nodes + num;
	plane = node->plane;

	if ( plane->type < 3 ) {
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = tw->maxs[plane->type];
	} else {
		t1 = DotProduct( plane->normal, p1 ) - plane->dist;
		/* node->plane for the first product only: that is what makes VC7 load
		   the plane's normal[0] before p2[0], as retail does at 0x00425EBE */
		t2 = ( node->plane->normal[0] * p2[0] + plane->normal[1] * p2[1]
			   + plane->normal[2] * p2[2] ) - plane->dist;
		offset = tw->isPoint ? 0.0f : 2048.0f;
	}

	if ( t1 >= offset + 1.0f && t2 >= offset + 1.0f ) {
		return CM_SightTraceThroughTree( tw, node->children[0], p1f, p2f,
										 p1, p2 );
	}
	if ( t1 < -offset - 1.0f && t2 < -offset - 1.0f ) {
		return CM_SightTraceThroughTree( tw, node->children[1], p1f, p2f,
										 p1, p2 );
	}

	if ( t1 < t2 ) {
		idist = 1.0f / ( t1 - t2 );
		side = 1;
		frac = ( t1 + offset + SURFACE_CLIP_EPSILON ) * idist;
		frac2 = ( t1 - offset + SURFACE_CLIP_EPSILON ) * idist;
	} else if ( t1 > t2 ) {
		idist = 1.0f / ( t1 - t2 );
		side = 0;
		frac = ( t1 - offset - SURFACE_CLIP_EPSILON ) * idist;
		frac2 = ( t1 + offset + SURFACE_CLIP_EPSILON ) * idist;
	} else {
		side = 0;
		frac = 0.0f;
		frac2 = 1.0f;
	}

	if ( frac2 < 0.0f ) {
		frac2 = 0.0f;
	} else if ( frac2 > 1.0f ) {
		frac2 = 1.0f;
	}

	midf = p1f + ( p2f - p1f ) * frac2;
	mid[0] = p1[0] + ( p2[0] - p1[0] ) * frac2;
	mid[1] = p1[1] + ( p2[1] - p1[1] ) * frac2;
	mid[2] = p1[2] + ( p2[2] - p1[2] ) * frac2;

	hit = CM_SightTraceThroughTree( tw, node->children[side], p1f, midf, p1, mid );
	if ( hit ) {
		return hit;
	}

	if ( frac < 0.0f ) {
		frac = 0.0f;
	} else if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	midf = p1f + ( p2f - p1f ) * frac;
	mid[0] = p1[0] + ( p2[0] - p1[0] ) * frac;
	mid[1] = p1[1] + ( p2[1] - p1[1] ) * frac;
	mid[2] = p1[2] + ( p2[2] - p1[2] ) * frac;

	return CM_SightTraceThroughTree( tw, node->children[side ^ 1], midf, p2f, mid, p2 );
}

/* ---- CM_SightTrace  0x00426130 ---- VERIFIED */
int CM_SightTrace( int oldHitNum, const vec3_t start, const vec3_t end,
				   const vec3_t mins, const vec3_t maxs, clipHandle_t model,
				   const vec3_t origin, int brushmask, qboolean capsule,
				   const cmSphereRecord_t *sphere ) {
	cmodel_t *cmod;
	traceWork_t tw;
	vec3_t offset;
	int i, hit, index;

	( void )origin;

	cmod = CM_ClipHandleToModel( model );

	cm_checkcount++;
	Com_Memset( &tw, 0, sizeof( tw ) );
	tw.trace.fraction = 1.0f;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	tw.contents = brushmask;

	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5f;
		tw.mins[i] = mins[i] - offset[i];
		tw.maxs[i] = maxs[i] - offset[i];
		tw.start[i] = start[i] + offset[i];
		tw.end[i] = end[i] + offset[i];
		tw.delta[i] = tw.end[i] - tw.start[i];
	}
	tw.deltaLengthSquared = DotProduct( tw.delta, tw.delta );

	if ( sphere ) {
		/* retail copies sphere and sphereExtents as one 36-byte record */
		*( cmSphereRecord_t * )&tw.sphere = *sphere;
	} else {
		tw.sphere.use = capsule;
		tw.sphere.radius = tw.maxs[0] > tw.maxs[2] ? tw.maxs[2] : tw.maxs[0];
		tw.sphere.halfheight = tw.maxs[2];
		tw.sphere.offset[0] = 0.0f;
		tw.sphere.offset[1] = 0.0f;
		tw.sphere.offset[2] = tw.maxs[2] - tw.sphere.radius;
	}

	tw.maxsSum = tw.maxs[0] + tw.maxs[1] + tw.maxs[2];

	tw.offsets[0][0] = tw.mins[0];  tw.offsets[0][1] = tw.mins[1];  tw.offsets[0][2] = tw.mins[2];
	tw.offsets[1][0] = tw.maxs[0];  tw.offsets[1][1] = tw.mins[1];  tw.offsets[1][2] = tw.mins[2];
	tw.offsets[2][0] = tw.mins[0];  tw.offsets[2][1] = tw.maxs[1];  tw.offsets[2][2] = tw.mins[2];
	tw.offsets[3][0] = tw.maxs[0];  tw.offsets[3][1] = tw.maxs[1];  tw.offsets[3][2] = tw.mins[2];
	tw.offsets[4][0] = tw.mins[0];  tw.offsets[4][1] = tw.mins[1];  tw.offsets[4][2] = tw.maxs[2];
	tw.offsets[5][0] = tw.maxs[0];  tw.offsets[5][1] = tw.mins[1];  tw.offsets[5][2] = tw.maxs[2];
	tw.offsets[6][0] = tw.mins[0];  tw.offsets[6][1] = tw.maxs[1];  tw.offsets[6][2] = tw.maxs[2];
	tw.offsets[7][0] = tw.maxs[0];  tw.offsets[7][1] = tw.maxs[1];  tw.offsets[7][2] = tw.maxs[2];

	if ( tw.sphere.use ) {
		for ( i = 0; i < 3; i++ ) {
			if ( tw.start[i] < tw.end[i] ) {
				tw.bounds[0][i] = tw.start[i] -
								  ( float )fabs( tw.sphere.offset[i] ) -
								  tw.sphere.radius;
				tw.bounds[1][i] = tw.end[i] +
								  ( float )fabs( tw.sphere.offset[i] ) +
								  tw.sphere.radius;
			} else {
				tw.bounds[0][i] = tw.end[i] -
								  ( float )fabs( tw.sphere.offset[i] ) -
								  tw.sphere.radius;
				/* retail 0x426442 adds the radius before start[i] here */
				tw.bounds[1][i] = tw.start[i] +
								  ( ( float )fabs( tw.sphere.offset[i] ) +
									tw.sphere.radius );
			}
		}
	} else {
		for ( i = 0; i < 3; i++ ) {
			if ( tw.start[i] < tw.end[i] ) {
				tw.bounds[0][i] = tw.start[i] + tw.mins[i];
				tw.bounds[1][i] = tw.end[i] + tw.maxs[i];
			} else {
				tw.bounds[0][i] = tw.end[i] + tw.mins[i];
				tw.bounds[1][i] = tw.start[i] + tw.maxs[i];
			}
		}
	}

	/* retail 0x426535 sums maxs[1] + maxs[2] first, then maxs[0] */
	tw.isPoint = ( tw.maxs[1] + tw.maxs[2] + tw.maxs[0] == 0.0f );

	if ( tw.sphere.use ) {
		for ( i = 0; i < 3; i++ ) {
			tw.sphereExtents[i] = tw.sphere.radius +
								  CM_Q_fabs( tw.sphere.offset[i] );
		}
	}

	if ( model ) {
		if ( model == CM_TEMP_CAPSULE_HANDLE ) {
			if ( tw.contents & box_brush->contents ) {
				if ( tw.sphere.use ) {
					return CM_SightTraceCapsuleThroughCapsule( &tw );
				}
				return CM_SightTraceBoundingBoxThroughCapsule( &tw );
			}
			return 0;
		}
		return CM_SightTraceThroughLeaf( &tw, &cmod->leaf );
	}

	hit = 0;

	if ( oldHitNum > 0 ) {
		index = oldHitNum - 1;

		if ( index < cm_numBrushes ) {
			hit = CM_SightTraceThroughBrush( &tw, &cm_brushes[index] );
		} else if ( index - cm_numBrushes < cm_numPatches ) {
			hit = CM_SightTraceThroughPatch(
					  &tw, &cm_patches[index - cm_numBrushes] );
		}
	}

	if ( !hit ) {
		hit = CM_SightTraceThroughTree( &tw, 0, 0.0f, 1.0f,
										tw.start, tw.end );
	}
	return hit;
}

/* ---- CM_BoxSightTrace  0x00426720 ---- VERIFIED */
int CM_BoxSightTrace( int oldHitNum, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs, clipHandle_t model,
					  int brushmask, qboolean capsule ) {
	return CM_SightTrace( oldHitNum, start, end, mins, maxs, model,
						  vec3_origin, brushmask, capsule, NULL );
}

/* ---- CM_TransformedBoxSightTrace  0x00426750 ---- VERIFIED */
int CM_TransformedBoxSightTrace( int oldHitNum, const vec3_t start,
								 const vec3_t end, const vec3_t mins,
								 const vec3_t maxs, clipHandle_t model,
								 int brushmask, const vec3_t origin,
								 const vec3_t angles, qboolean capsule ) {
	vec3_t start_l, end_l;
	qboolean rotated;
	vec3_t offset;
	vec3_t symetricSize[2];
	vec3_t matrix[3];
	int i;
	float halfwidth;
	float halfheight;
	float t;
	cmSphereRecord_t sphere;

	if ( !mins ) {
		mins = vec3_origin;
	}
	if ( !maxs ) {
		maxs = vec3_origin;
	}

	for ( i = 0; i < 3; i++ ) {
		offset[i] = ( mins[i] + maxs[i] ) * 0.5f;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
		start_l[i] = start[i] + offset[i];
		end_l[i] = end[i] + offset[i];
	}

	/* retail 0x42683D: origin comes off the stored float sums, so each
	   point is rounded twice */
	VectorSubtract( start_l, origin, start_l );
	VectorSubtract( end_l, origin, end_l );

	if ( model != CM_TEMP_BOX_HANDLE &&
		 ( angles[0] || angles[1] || angles[2] ) ) {
		rotated = qtrue;
	} else {
		rotated = qfalse;
	}

	halfwidth = symetricSize[1][0];
	halfheight = symetricSize[1][2];

	/* retail leaves sphere.extents unset; CM_Trace rebuilds them for capsules */
	sphere.sphere.use = capsule;
	sphere.sphere.radius = ( halfwidth > halfheight ) ? halfheight : halfwidth;
	sphere.sphere.halfheight = halfheight;
	t = halfheight - sphere.sphere.radius;

	if ( rotated ) {
		CreateRotationMatrix( angles, matrix );
		RotatePoint( start_l, matrix );
		RotatePoint( end_l, matrix );
		sphere.sphere.offset[0] = matrix[0][2] * t;
		sphere.sphere.offset[1] = -matrix[1][2] * t;
		sphere.sphere.offset[2] = matrix[2][2] * t;
	} else {
		VectorSet( sphere.sphere.offset, 0, 0, t );
	}

	return CM_SightTrace( oldHitNum, start_l, end_l, symetricSize[0],
						  symetricSize[1], model, origin, brushmask, capsule,
						  &sphere );
}
