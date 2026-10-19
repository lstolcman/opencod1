/*
 * qcommon/cm_patch.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/qcommon/cm_patch.c
 *
 * Retail range 0x0041A500-0x0041DB90, 27 functions.
 *
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */
#include "../qcommon/cm_local.h"
#include "../qcommon/cm_polylib.h"

winding_t *BaseWindingForPlane( const vec3_t normal, vec_t dist );
winding_t *CopyWinding( const winding_t *w );
void FreeWinding( winding_t *w );
void ChopWindingInPlace( winding_t **inout, const vec3_t normal,
						 vec_t dist, vec_t epsilon );
void WindingBounds( const winding_t *w, vec3_t mins, vec3_t maxs );
void *Hunk_AllocAlignInternal( int size, int align );
void ClearBounds( vec3_t mins, vec3_t maxs );
void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );

typedef struct cmPatchSphere_t
{
	qboolean use;                   /* +0x0F4 */
	float radius;                   /* +0x0F8 */
	float halfheight;               /* +0x0FC */
	vec3_t offset;                  /* +0x100 */
} cmPatchSphere_t;

typedef struct cmPatchTrace_t
{
	vec3_t start;                   /* +0x000 */
	vec3_t end;                     /* +0x00C */
	byte _pad_18[0x040 - 0x018];
	vec3_t offsets[8];              /* +0x040 */
	byte _pad_A0[0x0C0 - 0x0A0];
	qboolean isPoint;               /* +0x0C0 */
	trace_t trace;                  /* +0x0C4  fraction +0xC4, normal +0xD4 */
	cmPatchSphere_t sphere;         /* +0x0F4 */
	vec3_t sphereExtents;           /* +0x10C */
} cmPatchTrace_t;
COD1_ASSERT_SIZE( cmPatchTrace_t, 280 );

#define CM_PLAYER_CURVE_CLIP()  ( ( const cvar_t * )(size_t) cm_playerCurveClip )

#define MAX_GRID_SIZE   129

#define SURFACE_CLIP_EPSILON    0.125f

/* DotProduct( a, b ) as retail sums it.  VC7 adds the float products on the
 * 53-bit x87 stack in an order of its own; ( i + j ) + k is that order, which
 * the modern build spells out in double. */
#ifdef COD1_VC7
#define CM_DOT( a, b, i, j, k )     DotProduct( a, b )
#else
#define CM_DOT( a, b, i, j, k ) \
	( ( (double)( a )[i] * ( b )[i] + (double)( a )[j] * ( b )[j] ) \
	  + (double)( a )[k] * ( b )[k] )
#endif

/* The tolerances below are DOUBLE literals: retail compares against the
 * qwords dbl_568E80 (+0.1), dbl_568FC0 (-0.1), dbl_568FC8 (0.0001) and
 * dbl_568FD0 (0.02).  Only the ChopWindingInPlace epsilon is a float
 * (push 3DCCCCCDh), and those call sites pass the literal 0.1f. */
#define PLANE_TRI_EPSILON       0.1       /* CM_FindPlane / CM_PointOnPlaneSide / CM_AddFacetBevels side tolerance */

#define WRAP_POINT_EPSILON      0.1       /* CM_SetGridWrapWidth column-equality tolerance */
#define POINT_EPSILON           0.1       /* CM_ComparePoints per-axis tolerance */
#define NORMAL_EPSILON          0.0001    /* CM_PlaneEqual / CM_SnapVector normal tolerance */
#define DIST_EPSILON            0.02      /* CM_PlaneEqual plane-distance tolerance */

#define MAX_FACETS              1024

#define CM_MAX_MAP_BOUNDS       131072.0f

#define CM_BOUNDS_SEED          262144.0f

#define CM_SIDE_FRONT           0
#define CM_SIDE_BACK            1
#define CM_SIDE_ON              2

#define CM_FACET_LOWER          0
#define CM_FACET_UPPER          1
#define CM_FACET_QUAD           ( -1 )

/* CM_EdgePlaneNum's edge names, read off retail's jump table at 0x0041B250. */
#define CM_EDGE_BOTTOM          0
#define CM_EDGE_RIGHT           1
#define CM_EDGE_TOP             2
#define CM_EDGE_LEFT            3
#define CM_EDGE_DIAG_DESC       4
#define CM_EDGE_DIAG_ASC        5

#define CM_EDGE_PLANE_PUSH      4.0f

#define CM_BEVEL_MIN_LENGTH     0.5       /* qword dbl_568F38 */

#define CM_DEFAULT_MAX_ERROR    16

typedef struct cGrid_t
{
	int width;                                      /* +0x00 */
	int height;                                     /* +0x04 */
	qboolean wrapWidth;                             /* +0x08 */
	qboolean wrapHeight;                            /* +0x0C */
	vec3_t points[MAX_GRID_SIZE][MAX_GRID_SIZE];    /* +0x10 */
} cGrid_t;

patchPlane_t cm_patchPlanes[MAX_PATCH_PLANES];
int cm_numPatchPlanes;

int CM_FindPlane2( const vec4_t plane, int *flipped );
int CM_FindPlane( const vec3_t p1, const vec3_t p2, const vec3_t p3 );
int CM_PointOnPlaneSide( const vec3_t p, int planeNum );
int CM_GridPlane( int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
				  int i, int j, int tri );
int CM_EdgePlaneNum( const cGrid_t *grid,
					 int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
					 int i, int j, int k );
void CM_SetBorderInward( facet_t *facet, const cGrid_t *grid,
						 int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
						 int i, int j, int which );
qboolean CM_ValidateFacet( const facet_t *facet );
void CM_AddFacetBevels( facet_t *facet );
void CM_PatchCollideFromGrid( const cGrid_t *grid, patchCollide_t *pf );
qboolean CM_PlaneEqual( float *p, float *plane, int *flipped );
int CM_SignbitsForNormal( const vec3_t normal );
qboolean CM_PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b,
							 const vec3_t c );
void CM_SnapVector( vec3_t normal );
qboolean CM_NeedsSubdivision( const vec3_t a, const vec3_t b, const vec3_t c,
							  int maxError );
void CM_Subdivide( const vec3_t a, const vec3_t b, const vec3_t c,
				   vec3_t out1, vec3_t out2, vec3_t out3 );
void CM_TransposeGrid( cGrid_t *grid );
void CM_SetGridWrapWidth( cGrid_t *grid );
void CM_SubdivideGridColumns( cGrid_t *grid, int maxError );
void CM_RemoveDegenerateColumns( cGrid_t *grid );
void CM_TracePointThroughPatchCollide( cmPatchTrace_t *tw,
									   const patchCollide_t *pc );
qboolean CM_SightTracePointThroughPatchCollide( cmPatchTrace_t *tw,
												const patchCollide_t *pc );
int CM_CheckFacetPlane( const float *plane, const vec3_t start,
						const vec3_t end, float *enterFrac, float *leaveFrac,
						int *hit );

/* ---- CM_SignbitsForNormal  0x0041A500 ---- VERIFIED */
int __cdecl CM_SignbitsForNormal( const vec3_t normal )
{
	int bits, j;

	bits = 0;
	for ( j = 0; j < 3; j++ ) {
		if ( normal[j] < 0.0f ) {
			bits |= 1 << j;
		}
	}
	return bits;
}

/* ---- CM_PlaneFromPoints  0x0041A540 ---- VERIFIED */
qboolean CM_PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b,
							 const vec3_t c )
{
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );

	if ( VectorNormalize( plane ) == 0.0f ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/* ---- CM_NeedsSubdivision  0x0041A600 ---- VERIFIED
 * Retail never calls this copy; CM_SubdivideGridColumns inlines it
 * (0x0041A9F1-0x0041AA5F).  The three deltas stay unrounded on the x87 stack
 * (rfloat, see CM_SetGridWrapWidth), the squares are summed (z+y)+x, and the
 * unrounded length times 0.25f is compared against maxError (fild; the inlined
 * copy hoists it to a dword at 0x0041A9F1, hence the (float) cast).  The loop
 * and `2 * b` shape is what gives the inlined copy retail's operand order. */
qboolean CM_NeedsSubdivision(const vec3_t a, const vec3_t b, const vec3_t c,
                             int maxError)
{
  rfloat delta[3];
  rfloat dist;
  int i;

  for ( i = 0; i < 3; i++ ) {
    delta[i] = ( (rfloat)a[i] + c[i] ) - 2 * b[i];
  }

  dist = sqrt( CM_DOT( delta, delta, 2, 1, 0 ) );

  return ( dist * 0.25f > (float)maxError );
}

/* ---- CM_Subdivide  0x0041A660 ---- VERIFIED
 * out3 is written with fst and its unrounded value feeds out2 (0x0041A67B;
 * the copy inlined in CM_SubdivideGridColumns does the same at 0x0041AB70),
 * so out2 averages the float out1 with the unrounded out3.  VC7 does that
 * with Q3's text (any extra local changes the inlined copy's registers); the
 * modern build spells the unrounded value out. */
void CM_Subdivide(const vec3_t a, const vec3_t b, const vec3_t c,
                  vec3_t out1, vec3_t out2, vec3_t out3)
{
  int i;
#ifndef COD1_VC7
  rfloat mid;
#endif

  for ( i = 0; i < 3; i++ ) {
    out1[i] = 0.5f * ( a[i] + b[i] );
#ifdef COD1_VC7
    out3[i] = 0.5f * ( b[i] + c[i] );
    out2[i] = 0.5f * ( out1[i] + out3[i] );
#else
    mid = 0.5f * ( (rfloat)b[i] + c[i] );
    out3[i] = (float)mid;
    out2[i] = (float)( 0.5f * ( out1[i] + mid ) );
#endif
  }
}

/* ---- CM_TransposeGrid  0x0041A6E0 ---- VERIFIED
 * Swaps the rows and columns in place.  The retail build emitted the two
 * loop nests in the opposite order (the `width <= height` case first); they
 * are the two halves of RTCW's transpose and produce the same result. */
void CM_TransposeGrid( cGrid_t *grid )
{
	int i, j, l;
	vec3_t temp;
	qboolean tempWrap;

	if ( grid->width > grid->height ) {
		for ( i = 0; i < grid->height; i++ ) {
			for ( j = i + 1; j < grid->width; j++ ) {
				if ( j < grid->height ) {
					VectorCopy( grid->points[i][j], temp );
					VectorCopy( grid->points[j][i], grid->points[i][j] );
					VectorCopy( temp, grid->points[j][i] );
				} else {
					VectorCopy( grid->points[j][i], grid->points[i][j] );
				}
			}
		}
	} else {
		for ( i = 0; i < grid->width; i++ ) {
			for ( j = i + 1; j < grid->height; j++ ) {
				if ( j < grid->width ) {
					VectorCopy( grid->points[j][i], temp );
					VectorCopy( grid->points[i][j], grid->points[j][i] );
					VectorCopy( temp, grid->points[i][j] );
				} else {
					VectorCopy( grid->points[i][j], grid->points[j][i] );
				}
			}
		}
	}

	l = grid->width;
	grid->width = grid->height;
	grid->height = l;

	tempWrap = grid->wrapWidth;
	grid->wrapWidth = grid->wrapHeight;
	grid->wrapHeight = tempWrap;
}

/* ---- CM_SetGridWrapWidth  0x0041A840 ---- VERIFIED
 * If the left and right columns are exactly equal, set grid->wrapWidth true.
 * Retail unrolled the three-component compare per row; the result is the same
 * as RTCW's j-loop.  The difference stays unrounded in the x87 register
 * (fld/fsub, no store) and is compared against the doubles -0.1/+0.1, so d is
 * an rfloat here, and so is the cast (a (double) one costs retail's spills).
 * (A double local, even one never stored, makes VC7 LTCG 8-align the callers'
 * frames and stops it delaying the register pushes past the first loop test;
 * the rfloat locals in this file keep retail's frames.) */
void CM_SetGridWrapWidth( cGrid_t *grid )
{
	int i, j;
	rfloat d;

	for ( i = 0; i < grid->height; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			d = (rfloat)grid->points[0][i][j] - grid->points[grid->width - 1][i][j];
			if ( d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON ) {
				break;
			}
		}
		if ( j != 3 ) {
			break;
		}
	}

	grid->wrapWidth = ( i == grid->height );   /* 0x0041A997 `cmp edi,eax / setz cl` */
}

/* ---- CM_SubdivideGridColumns  0x0041A9C0 ---- VERIFIED */
void CM_SubdivideGridColumns( cGrid_t *grid, int maxError )
{
	int i, j, k;
	vec3_t prev, mid, next;

	for ( i = 0; i < grid->width - 2; ) {
		for ( j = 0; j < grid->height; j++ ) {
			if ( CM_NeedsSubdivision( grid->points[i][j], grid->points[i + 1][j],
									  grid->points[i + 2][j], maxError ) ) {
				break;
			}
		}

		if ( j == grid->height ) {
			i += 2;
			continue;
		}

		/* Retail 0x0041AA97-0x0041AC0E has no width-vs-MAX_GRID_SIZE check:
		 * a 129-wide grid that still needs subdividing shifts columns past
		 * points[128].  The grid is a CM_GeneratePatchCollide stack local, so
		 * that overruns its frame, as in retail. */
		for ( j = 0; j < grid->height; j++ ) {
			VectorCopy( grid->points[i][j], prev );
			VectorCopy( grid->points[i + 1][j], mid );
			VectorCopy( grid->points[i + 2][j], next );

			for ( k = grid->width - 1; k > i + 1; k-- ) {
				VectorCopy( grid->points[k][j], grid->points[k + 2][j] );
			}

			CM_Subdivide( prev, mid, next, grid->points[i + 1][j],
						  grid->points[i + 2][j], grid->points[i + 3][j] );
		}

		grid->width += 2;

	}
}

/* ---- CM_ComparePoints  0x0041AC30 ---- VERIFIED
 * Each difference stays unrounded in the x87 register (fld/fsub) and is
 * compared against the doubles -0.1 and +0.1, both bounds inclusive.  d is an
 * rfloat, not a double (see CM_SetGridWrapWidth). */
qboolean __cdecl CM_ComparePoints( const vec3_t a, const vec3_t b )
{
	rfloat d;

	d = (double)a[0] - b[0];
	if ( d < -POINT_EPSILON || d > POINT_EPSILON ) {
		return qfalse;
	}
	d = (double)a[1] - b[1];
	if ( d < -POINT_EPSILON || d > POINT_EPSILON ) {
		return qfalse;
	}
	d = (double)a[2] - b[2];
	if ( d < -POINT_EPSILON || d > POINT_EPSILON ) {
		return qfalse;
	}
	return qtrue;
}

/* ---- CM_RemoveDegenerateColumns  0x0041ACA0 ---- VERIFIED
 * If two adjacent columns are identical, drop one of them.  Retail inlined
 * CM_ComparePoints into the row scan. */
void CM_RemoveDegenerateColumns( cGrid_t *grid )
{
	int i, j, k;

	for ( i = 0; i < grid->width - 1; i++ ) {
		for ( j = 0; j < grid->height; j++ ) {
			if ( !CM_ComparePoints( grid->points[i][j], grid->points[i + 1][j] ) ) {
				break;
			}
		}

		if ( j != grid->height ) {
			continue;   /* not degenerate */
		}

		for ( j = 0; j < grid->height; j++ ) {
			for ( k = i + 2; k < grid->width; k++ ) {
				VectorCopy( grid->points[k][j], grid->points[k - 1][j] );
			}
		}
		grid->width--;

		/* recheck the column that shifted into this slot */
		i--;
	}
}

/* ---- CM_PlaneEqual  0x0041ADD0 ---- VERIFIED
 * p is a patchPlane_t's plane[4] (passed as a bare float*).  Each difference
 * stays unrounded in the x87 register (fld/fsub/fabs) and is compared against
 * the doubles 0.0001 (normal) and 0.02 (distance).  The first flipped test is
 * float-typed in retail (it keeps -plane[0] on the stack, 0x0041AE53); the
 * rfloat cast is float under VC7 and keeps the modern build unrounded. */
qboolean __cdecl CM_PlaneEqual( float *p, float *plane, int *flipped )
{
	vec4_t invplane;

	if ( fabs( (double)p[0] - plane[0] ) < NORMAL_EPSILON
		 && fabs( (double)p[1] - plane[1] ) < NORMAL_EPSILON
		 && fabs( (double)p[2] - plane[2] ) < NORMAL_EPSILON
		 && fabs( (double)p[3] - plane[3] ) < DIST_EPSILON ) {
		*flipped = qfalse;
		return qtrue;
	}

	VectorNegate( plane, invplane );
	invplane[3] = -plane[3];

	if ( fabs( (rfloat)p[0] - invplane[0] ) < NORMAL_EPSILON
		 && fabs( (double)p[1] - invplane[1] ) < NORMAL_EPSILON
		 && fabs( (double)p[2] - invplane[2] ) < NORMAL_EPSILON
		 && fabs( (double)p[3] - invplane[3] ) < DIST_EPSILON ) {
		*flipped = qtrue;
		return qtrue;
	}

	return qfalse;
}

/* ---- CM_SnapVector  0x0041AEC0 ---- VERIFIED
 * |normal[i] -/+ 1| is float-typed (dword 1.0f constant), stays unrounded in
 * the x87 register and is compared against the double 0.0001.  The rfloat
 * cast is float under VC7 and keeps the modern build unrounded. */
void __cdecl CM_SnapVector( vec3_t normal )
{
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( fabs( (rfloat)normal[i] - 1.0f ) < NORMAL_EPSILON ) {
			VectorClear( normal );
			normal[i] = 1.0f;
			return;
		}
		if ( fabs( (rfloat)normal[i] - -1.0f ) < NORMAL_EPSILON ) {
			VectorClear( normal );
			normal[i] = -1.0f;
			return;
		}
	}
}

/* ---- CM_FindPlane2  0x0041AF20 ---- VERIFIED */
int CM_FindPlane2( const vec4_t plane, int *flipped )
{
	int i;

	for ( i = 0; i < cm_numPatchPlanes; i++ ) {
		if ( CM_PlaneEqual( cm_patchPlanes[i].plane, ( float * ) plane,
							flipped ) ) {
			return i;
		}
	}

	if ( cm_numPatchPlanes == MAX_PATCH_PLANES ) {
		Com_Error( ERR_DROP, "\x15" "MAX_PATCH_PLANES" );
	}

	cm_patchPlanes[cm_numPatchPlanes].plane[0] = plane[0];
	cm_patchPlanes[cm_numPatchPlanes].plane[1] = plane[1];
	cm_patchPlanes[cm_numPatchPlanes].plane[2] = plane[2];
	cm_patchPlanes[cm_numPatchPlanes].plane[3] = plane[3];
	cm_patchPlanes[cm_numPatchPlanes].signbits = CM_SignbitsForNormal( plane );

	cm_numPatchPlanes++;

	*flipped = qfalse;

	return cm_numPatchPlanes - 1;
}

/* ---- CM_FindPlane  0x0041B000 ---- VERIFIED
 * The facing test is summed (y+z)+x and the per-point distance (z+y)+x; both
 * stay unrounded.  The distance is compared against the doubles -0.1/+0.1
 * straight from the x87 register (0x0041B069-0x0041B0FB), so d is an rfloat
 * here (see CM_SetGridWrapWidth). */
int CM_FindPlane( const vec3_t p1, const vec3_t p2, const vec3_t p3 )
{
	float plane[4];
	int i;
	rfloat d;

	if ( !CM_PlaneFromPoints( plane, p1, p2, p3 ) ) {
		return -1;
	}

	// see if the points are close enough to an existing plane
	for ( i = 0; i < cm_numPatchPlanes; i++ ) {
		if ( CM_DOT( plane, cm_patchPlanes[i].plane, 1, 2, 0 ) < 0 ) {
			continue;   // allow backwards planes?
		}

		d = CM_DOT( p1, cm_patchPlanes[i].plane, 2, 1, 0 ) - cm_patchPlanes[i].plane[3];
		if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON ) {
			continue;
		}

		d = CM_DOT( p2, cm_patchPlanes[i].plane, 2, 1, 0 ) - cm_patchPlanes[i].plane[3];
		if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON ) {
			continue;
		}

		d = CM_DOT( p3, cm_patchPlanes[i].plane, 2, 1, 0 ) - cm_patchPlanes[i].plane[3];
		if ( d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON ) {
			continue;
		}

		// found it
		return i;
	}

	// add a new plane
	if ( cm_numPatchPlanes == MAX_PATCH_PLANES ) {
		Com_Error( ERR_DROP, "\x15" "MAX_PATCH_PLANES" );
	}

	cm_patchPlanes[cm_numPatchPlanes].plane[0] = plane[0];
	cm_patchPlanes[cm_numPatchPlanes].plane[1] = plane[1];
	cm_patchPlanes[cm_numPatchPlanes].plane[2] = plane[2];
	cm_patchPlanes[cm_numPatchPlanes].plane[3] = plane[3];
	cm_patchPlanes[cm_numPatchPlanes].signbits = CM_SignbitsForNormal( plane );

	cm_numPatchPlanes++;

	return cm_numPatchPlanes - 1;
}

/* ---- CM_PointOnPlaneSide  0x0041B1C0 ---- VERIFIED
 * d is summed (z+y)+x and tested, unrounded, against the doubles +0.1/-0.1
 * straight from the x87 register (0x0041B1C8-0x0041B1FC), so it is an rfloat
 * here (see CM_SetGridWrapWidth).  CM_SetBorderInward's inlined copy behaves
 * the same. */
int CM_PointOnPlaneSide( const vec3_t p, int planeNum )
{
	const float *plane;
	rfloat d;

	if ( planeNum == -1 ) {
		return CM_SIDE_ON;
	}

	plane = cm_patchPlanes[planeNum].plane;
	d = ( (double)plane[2] * p[2] + (double)plane[1] * p[1] )
		+ (double)plane[0] * p[0] - plane[3];

	if ( d > PLANE_TRI_EPSILON ) {
		return CM_SIDE_FRONT;
	}
	if ( d < -PLANE_TRI_EPSILON ) {
		return CM_SIDE_BACK;
	}
	return CM_SIDE_ON;
}

/* ---- CM_GridPlane  0x0041B220 ---- VERIFIED */
int CM_GridPlane(int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
                 int i, int j, int tri)
{
  int p;

  p = gridPlanes[i][j][tri];
  if ( p != -1 ) {
    return p;
  }

  p = gridPlanes[i][j][!tri];
  if ( p != -1 ) {
    return p;
  }

  return -1;
}

/* ---- CM_EdgePlaneNum  0x0041B250 ---- VERIFIED
 * Q3's text and case order (retail's jump-table bodies run 0, 2, 3, 1, 4, 5). */
int CM_EdgePlaneNum( const cGrid_t *grid,
					 int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
					 int i, int j, int k )
{
	const float *p1, *p2;
	vec3_t up;
	int p;

	switch ( k ) {
	case CM_EDGE_BOTTOM:
		p1 = grid->points[i][j];
		p2 = grid->points[i + 1][j];
		p = CM_GridPlane( gridPlanes, i, j, 0 );
		if ( p < 0 ) {
			return -1;
		}
		VectorMA( p1, CM_EDGE_PLANE_PUSH, cm_patchPlanes[p].plane, up );
		return CM_FindPlane( p1, p2, up );

	case CM_EDGE_TOP:
		p1 = grid->points[i][j + 1];
		p2 = grid->points[i + 1][j + 1];
		p = CM_GridPlane( gridPlanes, i, j, 1 );
		if ( p < 0 ) {
			return -1;
		}
		VectorMA( p1, CM_EDGE_PLANE_PUSH, cm_patchPlanes[p].plane, up );
		return CM_FindPlane( p2, p1, up );

	case CM_EDGE_LEFT:
		p1 = grid->points[i][j];
		p2 = grid->points[i][j + 1];
		p = CM_GridPlane( gridPlanes, i, j, 1 );
		if ( p < 0 ) {
			return -1;
		}
		VectorMA( p1, CM_EDGE_PLANE_PUSH, cm_patchPlanes[p].plane, up );
		return CM_FindPlane( p2, p1, up );

	case CM_EDGE_RIGHT:
		p1 = grid->points[i + 1][j];
		p2 = grid->points[i + 1][j + 1];
		p = CM_GridPlane( gridPlanes, i, j, 0 );
		if ( p < 0 ) {
			return -1;
		}
		VectorMA( p1, CM_EDGE_PLANE_PUSH, cm_patchPlanes[p].plane, up );
		return CM_FindPlane( p1, p2, up );

	case CM_EDGE_DIAG_DESC:
		p1 = grid->points[i + 1][j + 1];
		p2 = grid->points[i][j];
		p = CM_GridPlane( gridPlanes, i, j, 0 );
		if ( p < 0 ) {
			return -1;
		}
		VectorMA( p1, CM_EDGE_PLANE_PUSH, cm_patchPlanes[p].plane, up );
		return CM_FindPlane( p1, p2, up );

	case CM_EDGE_DIAG_ASC:
		p1 = grid->points[i][j];
		p2 = grid->points[i + 1][j + 1];
		p = CM_GridPlane( gridPlanes, i, j, 1 );
		if ( p < 0 ) {
			return -1;
		}
		VectorMA( p1, CM_EDGE_PLANE_PUSH, cm_patchPlanes[p].plane, up );
		return CM_FindPlane( p1, p2, up );
	}

	Com_Error( ERR_DROP, "\x15" "CM_EdgePlaneNum: bad edge name" );
	return -1;
}

/* ---- CM_SetBorderInward  0x0041B4D0 ---- VERIFIED */
void CM_SetBorderInward( facet_t *facet, const cGrid_t *grid,
						 int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2],
						 int i, int j, int which )
{
	const float *points[4];
	int numPoints;
	int k, l, side;
	int front, back;

	( void ) gridPlanes;

	switch ( which ) {
	case CM_FACET_LOWER:
		points[0] = grid->points[i][j];
		points[1] = grid->points[i + 1][j];
		points[2] = grid->points[i + 1][j + 1];
		numPoints = 3;
		break;

	case CM_FACET_UPPER:
		points[0] = grid->points[i + 1][j + 1];
		points[1] = grid->points[i][j + 1];
		points[2] = grid->points[i][j];
		numPoints = 3;
		break;

	case CM_FACET_QUAD:
		points[0] = grid->points[i][j];
		points[1] = grid->points[i + 1][j];
		points[2] = grid->points[i + 1][j + 1];
		points[3] = grid->points[i][j + 1];
		numPoints = 4;
		break;

	default:
		numPoints = 0;
		Com_Error( ERR_FATAL, "\x15" "CM_SetBorderInward: bad parameter" );
		break;
	}

	for ( k = 0; k < facet->numBorders; k++ ) {
		front = 0;
		back = 0;

		for ( l = 0; l < numPoints; l++ ) {
			side = CM_PointOnPlaneSide( points[l], facet->borderPlanes[k] );
			if ( side == CM_SIDE_FRONT ) {
				front++;
			} else if ( side == CM_SIDE_BACK ) {
				back++;
			}
		}

		if ( front && !back ) {
			facet->borderInward[k] = qtrue;
		} else if ( back && !front ) {
			facet->borderInward[k] = qfalse;
		} else if ( !front && !back ) {
			facet->borderPlanes[k] = -1;
		} else {
			Com_DPrintf( "WARNING: CM_SetBorderInward: mixed plane sides\n" );
			facet->borderInward[k] = qfalse;
		}
	}
}

/* CM_FacetPlane  VERIFIED */
static void CM_FacetPlane( int planeNum, qboolean inward, vec4_t plane )
{
	plane[0] = cm_patchPlanes[planeNum].plane[0];
	plane[1] = cm_patchPlanes[planeNum].plane[1];
	plane[2] = cm_patchPlanes[planeNum].plane[2];
	plane[3] = cm_patchPlanes[planeNum].plane[3];

	if ( !inward ) {
		plane[0] = -plane[0];
		plane[1] = -plane[1];
		plane[2] = -plane[2];
		plane[3] = -plane[3];
	}
}

/* ---- CM_ValidateFacet  0x0041B820 ---- VERIFIED */
qboolean CM_ValidateFacet( const facet_t *facet )
{
	vec4_t plane;
	vec3_t bounds[2];
	winding_t *w;
	int j;

	if ( facet->surfacePlane == -1 ) {
		return qfalse;
	}

	CM_FacetPlane( facet->surfacePlane, qtrue, plane );
	w = BaseWindingForPlane( plane, plane[3] );

	for ( j = 0; j < facet->numBorders && w; j++ ) {
		if ( facet->borderPlanes[j] == -1 ) {
			FreeWinding( w );
			return qfalse;
		}
		/* written out: retail inlines it (0x0041B8A3), a CM_FacetPlane call here is not */
		plane[0] = cm_patchPlanes[facet->borderPlanes[j]].plane[0];
		plane[1] = cm_patchPlanes[facet->borderPlanes[j]].plane[1];
		plane[2] = cm_patchPlanes[facet->borderPlanes[j]].plane[2];
		plane[3] = cm_patchPlanes[facet->borderPlanes[j]].plane[3];
		if ( !facet->borderInward[j] ) {
			plane[0] = -plane[0];
			plane[1] = -plane[1];
			plane[2] = -plane[2];
			plane[3] = -plane[3];
		}
		ChopWindingInPlace( &w, plane, plane[3], 0.1f );   /* 0x0041B904 push 3DCCCCCDh */
	}

	if ( !w ) {
		return qfalse;
	}

	WindingBounds( w, bounds[0], bounds[1] );
	FreeWinding( w );

	for ( j = 0; j < 3; j++ ) {
		if ( bounds[1][j] - bounds[0][j] > CM_MAX_MAP_BOUNDS ) {
			return qfalse;
		}
		if ( bounds[0][j] >= CM_MAX_MAP_BOUNDS ) {
			return qfalse;
		}
		if ( bounds[1][j] <= -CM_MAX_MAP_BOUNDS ) {
			return qfalse;
		}
	}

	return qtrue;
}

/* ---- CM_AddFacetBevels  0x0041BA10 ---- VERIFIED */
void CM_AddFacetBevels( facet_t *facet )
{
	vec4_t plane, newplane;
	vec3_t mins, maxs, vec, vec2;
	winding_t *w, *w2;
	qboolean flipped, hasBack;
	float d;    /* retail rounds d to float (0x0041BDDD fstp dword) before the double +/-0.1 compares */
	int i, j, k, l, next;
	int axis, dir;

	/* copies written out, not CM_FacetPlane: VC7 then associates the two
	 * DotProducts below like retail, (z+y)+x and (z+x)+y (0x0041BD92, 0x0041BDC1) */
	plane[0] = cm_patchPlanes[facet->surfacePlane].plane[0];
	plane[1] = cm_patchPlanes[facet->surfacePlane].plane[1];
	plane[2] = cm_patchPlanes[facet->surfacePlane].plane[2];
	plane[3] = cm_patchPlanes[facet->surfacePlane].plane[3];
	w = BaseWindingForPlane( plane, plane[3] );

	for ( j = 0; j < facet->numBorders && w; j++ ) {
		if ( facet->borderPlanes[j] == facet->surfacePlane ) {
			continue;
		}
		plane[0] = cm_patchPlanes[facet->borderPlanes[j]].plane[0];
		plane[1] = cm_patchPlanes[facet->borderPlanes[j]].plane[1];
		plane[2] = cm_patchPlanes[facet->borderPlanes[j]].plane[2];
		plane[3] = cm_patchPlanes[facet->borderPlanes[j]].plane[3];
		if ( !facet->borderInward[j] ) {
			plane[0] = -plane[0];
			plane[1] = -plane[1];
			plane[2] = -plane[2];
			plane[3] = -plane[3];
		}
		ChopWindingInPlace( &w, plane, plane[3], 0.1f );   /* 0x0041BADD push 3DCCCCCDh */
	}

	if ( !w ) {
		return;
	}

	WindingBounds( w, mins, maxs );

	for ( axis = 0; axis < 3; axis++ ) {
		for ( dir = -1; dir <= 1; dir += 2 ) {
			VectorClear( plane );
			plane[axis] = ( float ) dir;
			if ( dir == 1 ) {
				plane[3] = maxs[axis];
			} else {
				plane[3] = -mins[axis];
			}

			if ( CM_PlaneEqual( cm_patchPlanes[facet->surfacePlane].plane,
								plane, &flipped ) ) {
				continue;
			}

			for ( i = 0; i < facet->numBorders; i++ ) {
				if ( CM_PlaneEqual( cm_patchPlanes[facet->borderPlanes[i]].plane,
									plane, &flipped ) ) {
					break;
				}
			}
			if ( i != facet->numBorders ) {
				continue;
			}

			if ( facet->numBorders > MAX_FACET_BORDERS ) {
				Com_Printf( "ERROR: too many bevels\n" );
			}
			facet->borderPlanes[facet->numBorders] =
				CM_FindPlane2( plane, &flipped );
			facet->borderNoAdjust[facet->numBorders] = qfalse;
			facet->borderInward[facet->numBorders] = flipped;
			facet->numBorders++;
		}
	}

	for ( j = 0; j < w->numpoints; j++ ) {
		next = ( j + 1 ) % w->numpoints;
		VectorSubtract( w->p[j], w->p[next], vec );

		if ( VectorNormalize( vec ) < CM_BEVEL_MIN_LENGTH ) {
			continue;
		}

		CM_SnapVector( vec );

		for ( k = 0; k < 3; k++ ) {
			if ( vec[k] == -1.0f || vec[k] == 1.0f ) {
				break;
			}
		}
		if ( k < 3 ) {
			continue;
		}

		for ( axis = 0; axis < 3; axis++ ) {
			for ( dir = -1; dir <= 1; dir += 2 ) {
				VectorClear( vec2 );
				vec2[axis] = ( float ) dir;
				CrossProduct( vec, vec2, plane );
				if ( VectorNormalize( plane ) < CM_BEVEL_MIN_LENGTH ) {
					continue;
				}
				plane[3] = DotProduct( w->p[j], plane );

				hasBack = qfalse;
				for ( l = 0; l < w->numpoints; l++ ) {
					d = DotProduct( w->p[l], plane ) - plane[3];
					if ( d > PLANE_TRI_EPSILON ) {
						break;
					}
					if ( d < -PLANE_TRI_EPSILON ) {
						hasBack = qtrue;
					}
				}
				if ( l < w->numpoints || !hasBack ) {
					continue;
				}

				for ( i = 0; i < facet->numBorders; i++ ) {
					if ( CM_PlaneEqual(
							 cm_patchPlanes[facet->borderPlanes[i]].plane,
							 plane, &flipped ) ) {
						break;
					}
				}
				if ( i != facet->numBorders ) {
					continue;
				}

				if ( facet->numBorders > MAX_FACET_BORDERS ) {
					Com_Printf( "ERROR: too many bevels\n" );
				}
				facet->borderPlanes[facet->numBorders] =
					CM_FindPlane2( plane, &flipped );

				for ( i = 0; i < facet->numBorders; i++ ) {
					if ( facet->borderPlanes[facet->numBorders]
						 == facet->borderPlanes[i] ) {
						Com_Printf( "WARNING: bevel plane already used\n" );
					}
				}

				facet->borderNoAdjust[facet->numBorders] = qfalse;
				facet->borderInward[facet->numBorders] = flipped;

				w2 = CopyWinding( w );
				CM_FacetPlane( facet->borderPlanes[facet->numBorders],
							   facet->borderInward[facet->numBorders],
							   newplane );
				ChopWindingInPlace( &w2, newplane, newplane[3],
									0.1f );   /* 0x0041BFC9 push 3DCCCCCDh */
				if ( !w2 ) {
					Com_DPrintf(
						"WARNING: CM_AddFacetBevels... invalid bevel\n" );
					continue;
				}
				FreeWinding( w2 );
				facet->numBorders++;
			}
		}
	}

	FreeWinding( w );

	facet->borderPlanes[facet->numBorders] = facet->surfacePlane;
	facet->borderNoAdjust[facet->numBorders] = qfalse;
	facet->borderInward[facet->numBorders] = qtrue;
	facet->numBorders++;
}

/* ---- CM_PatchCollideFromGrid  0x0041C120 ---- VERIFIED
 * Q3's text.  gridPlanes and facets are stack locals in retail (its
 * 0x708A8-byte frame); CM_EdgePlaneNum and Com_Memset are inlined. */
void CM_PatchCollideFromGrid( const cGrid_t *grid, patchCollide_t *pf )
{
	int i, j;
	const float *p1, *p2, *p3;
	int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2];
	facet_t *facet;
	int borders[4];
	int noAdjust[4];
	facet_t facets[MAX_FACETS];
	int numFacets;

	cm_numPatchPlanes = 0;
	numFacets = 0;

	// find the planes for each triangle of the grid
	for ( i = 0; i < grid->width - 1; i++ ) {
		for ( j = 0; j < grid->height - 1; j++ ) {
			p1 = grid->points[i][j];
			p2 = grid->points[i + 1][j];
			p3 = grid->points[i + 1][j + 1];
			gridPlanes[i][j][0] = CM_FindPlane( p1, p2, p3 );

			p1 = grid->points[i + 1][j + 1];
			p2 = grid->points[i][j + 1];
			p3 = grid->points[i][j];
			gridPlanes[i][j][1] = CM_FindPlane( p1, p2, p3 );
		}
	}

	// create the borders for each facet
	for ( i = 0; i < grid->width - 1; i++ ) {
		for ( j = 0; j < grid->height - 1; j++ ) {

			borders[CM_EDGE_BOTTOM] = -1;
			if ( j > 0 ) {
				borders[CM_EDGE_BOTTOM] = gridPlanes[i][j - 1][1];
			} else if ( grid->wrapHeight ) {
				borders[CM_EDGE_BOTTOM] = gridPlanes[i][grid->height - 2][1];
			}
			noAdjust[CM_EDGE_BOTTOM] = ( borders[CM_EDGE_BOTTOM] == gridPlanes[i][j][0] );
			if ( borders[CM_EDGE_BOTTOM] == -1 || noAdjust[CM_EDGE_BOTTOM] ) {
				borders[CM_EDGE_BOTTOM] = CM_EdgePlaneNum( grid, gridPlanes, i, j, CM_EDGE_BOTTOM );
			}

			borders[CM_EDGE_TOP] = -1;
			if ( j < grid->height - 2 ) {
				borders[CM_EDGE_TOP] = gridPlanes[i][j + 1][0];
			} else if ( grid->wrapHeight ) {
				borders[CM_EDGE_TOP] = gridPlanes[i][0][0];
			}
			noAdjust[CM_EDGE_TOP] = ( borders[CM_EDGE_TOP] == gridPlanes[i][j][1] );
			if ( borders[CM_EDGE_TOP] == -1 || noAdjust[CM_EDGE_TOP] ) {
				borders[CM_EDGE_TOP] = CM_EdgePlaneNum( grid, gridPlanes, i, j, CM_EDGE_TOP );
			}

			borders[CM_EDGE_LEFT] = -1;
			if ( i > 0 ) {
				borders[CM_EDGE_LEFT] = gridPlanes[i - 1][j][0];
			} else if ( grid->wrapWidth ) {
				borders[CM_EDGE_LEFT] = gridPlanes[grid->width - 2][j][0];
			}
			noAdjust[CM_EDGE_LEFT] = ( borders[CM_EDGE_LEFT] == gridPlanes[i][j][1] );
			if ( borders[CM_EDGE_LEFT] == -1 || noAdjust[CM_EDGE_LEFT] ) {
				borders[CM_EDGE_LEFT] = CM_EdgePlaneNum( grid, gridPlanes, i, j, CM_EDGE_LEFT );
			}

			borders[CM_EDGE_RIGHT] = -1;
			if ( i < grid->width - 2 ) {
				borders[CM_EDGE_RIGHT] = gridPlanes[i + 1][j][1];
			} else if ( grid->wrapWidth ) {
				borders[CM_EDGE_RIGHT] = gridPlanes[0][j][1];
			}
			noAdjust[CM_EDGE_RIGHT] = ( borders[CM_EDGE_RIGHT] == gridPlanes[i][j][0] );
			if ( borders[CM_EDGE_RIGHT] == -1 || noAdjust[CM_EDGE_RIGHT] ) {
				borders[CM_EDGE_RIGHT] = CM_EdgePlaneNum( grid, gridPlanes, i, j, CM_EDGE_RIGHT );
			}

			if ( numFacets == MAX_FACETS ) {
				Com_Error( ERR_DROP, "\x15" "MAX_FACETS" );
			}
			facet = &facets[numFacets];
			Com_Memset( facet, 0, sizeof( *facet ) );

			if ( gridPlanes[i][j][0] == gridPlanes[i][j][1] ) {
				if ( gridPlanes[i][j][0] == -1 ) {
					continue;       // degenrate
				}
				facet->surfacePlane = gridPlanes[i][j][0];
				facet->numBorders = 4;
				facet->borderPlanes[0] = borders[CM_EDGE_BOTTOM];
				facet->borderNoAdjust[0] = noAdjust[CM_EDGE_BOTTOM];
				facet->borderPlanes[1] = borders[CM_EDGE_RIGHT];
				facet->borderNoAdjust[1] = noAdjust[CM_EDGE_RIGHT];
				facet->borderPlanes[2] = borders[CM_EDGE_TOP];
				facet->borderNoAdjust[2] = noAdjust[CM_EDGE_TOP];
				facet->borderPlanes[3] = borders[CM_EDGE_LEFT];
				facet->borderNoAdjust[3] = noAdjust[CM_EDGE_LEFT];
				CM_SetBorderInward( facet, grid, gridPlanes, i, j, CM_FACET_QUAD );
				if ( CM_ValidateFacet( facet ) ) {
					CM_AddFacetBevels( facet );
					numFacets++;
				}
			} else {
				// two seperate triangles
				facet->surfacePlane = gridPlanes[i][j][0];
				facet->numBorders = 3;
				facet->borderPlanes[0] = borders[CM_EDGE_BOTTOM];
				facet->borderNoAdjust[0] = noAdjust[CM_EDGE_BOTTOM];
				facet->borderPlanes[1] = borders[CM_EDGE_RIGHT];
				facet->borderNoAdjust[1] = noAdjust[CM_EDGE_RIGHT];
				facet->borderPlanes[2] = gridPlanes[i][j][1];
				if ( facet->borderPlanes[2] == -1 ) {
					facet->borderPlanes[2] = borders[CM_EDGE_TOP];
					if ( facet->borderPlanes[2] == -1 ) {
						facet->borderPlanes[2] = CM_EdgePlaneNum( grid, gridPlanes, i, j, CM_EDGE_DIAG_DESC );
					}
				}
				CM_SetBorderInward( facet, grid, gridPlanes, i, j, CM_FACET_LOWER );
				if ( CM_ValidateFacet( facet ) ) {
					CM_AddFacetBevels( facet );
					numFacets++;
				}

				if ( numFacets == MAX_FACETS ) {
					Com_Error( ERR_DROP, "\x15" "MAX_FACETS" );
				}
				facet = &facets[numFacets];
				Com_Memset( facet, 0, sizeof( *facet ) );

				facet->surfacePlane = gridPlanes[i][j][1];
				facet->numBorders = 3;
				facet->borderPlanes[0] = borders[CM_EDGE_TOP];
				facet->borderNoAdjust[0] = noAdjust[CM_EDGE_TOP];
				facet->borderPlanes[1] = borders[CM_EDGE_LEFT];
				facet->borderNoAdjust[1] = noAdjust[CM_EDGE_LEFT];
				facet->borderPlanes[2] = gridPlanes[i][j][0];
				if ( facet->borderPlanes[2] == -1 ) {
					facet->borderPlanes[2] = borders[CM_EDGE_BOTTOM];
					if ( facet->borderPlanes[2] == -1 ) {
						facet->borderPlanes[2] = CM_EdgePlaneNum( grid, gridPlanes, i, j, CM_EDGE_DIAG_ASC );
					}
				}
				CM_SetBorderInward( facet, grid, gridPlanes, i, j, CM_FACET_UPPER );
				if ( CM_ValidateFacet( facet ) ) {
					CM_AddFacetBevels( facet );
					numFacets++;
				}
			}
		}
	}

	// copy the results out
	pf->numPlanes = cm_numPatchPlanes;
	pf->numFacets = numFacets;
	pf->facets = ( facet_t * ) Hunk_AllocAlignInternal( numFacets * sizeof( *pf->facets ), 32 );
	Com_Memcpy( pf->facets, facets, numFacets * sizeof( *pf->facets ) );
	pf->planes = ( patchPlane_t * ) Hunk_AllocAlignInternal( cm_numPatchPlanes * sizeof( *pf->planes ), 32 );
	Com_Memcpy( pf->planes, cm_patchPlanes, cm_numPatchPlanes * sizeof( *pf->planes ) );
}

/* ---- CM_GeneratePatchCollide  0x0041C940 ---- VERIFIED
 * Q3's text: the grid is a stack local (retail's 0x30C28-byte frame) and the
 * bounds go through ClearBounds/AddPointToBounds into the caller's bounds. */
patchCollide_t *CM_GeneratePatchCollide( int width, int height, int maxError,
										   const vec3_t *points,
										   vec3_t *bounds )
{
	patchCollide_t *pf;
	cGrid_t grid;
	int i, j;

	if ( width <= 2 || height <= 2 || !points ) {
		Com_Error( ERR_DROP,
				   "\x15" "CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)",
				   width, height, points );
	}

	if ( !( width & 1 ) || !( height & 1 ) ) {
		Com_Error( ERR_DROP, "\x15"
				   "CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes" );
	}

	if ( width > MAX_GRID_SIZE || height > MAX_GRID_SIZE ) {
		Com_Error( ERR_DROP,
				   "\x15" "CM_GeneratePatchFacets: source is > MAX_GRID_SIZE" );
	}

	// build a grid
	grid.width = width;
	grid.height = height;
	grid.wrapWidth = qfalse;
	grid.wrapHeight = qfalse;
	for ( i = 0; i < width; i++ ) {
		for ( j = 0; j < height; j++ ) {
			VectorCopy( points[j * width + i], grid.points[i][j] );
		}
	}

	// subdivide the grid
	CM_SetGridWrapWidth( &grid );
	CM_SubdivideGridColumns( &grid, maxError );
	CM_RemoveDegenerateColumns( &grid );

	CM_TransposeGrid( &grid );

	CM_SetGridWrapWidth( &grid );
	CM_SubdivideGridColumns( &grid, maxError );
	CM_RemoveDegenerateColumns( &grid );

	// we now have a grid of points exactly on the curve
	// the aproximate surface defined by these points will be
	// collided against
	pf = ( patchCollide_t * ) Hunk_AllocAlignInternal( sizeof( *pf ), 32 );
	ClearBounds( bounds[0], bounds[1] );
	for ( i = 0; i < grid.width; i++ ) {
		for ( j = 0; j < grid.height; j++ ) {
			AddPointToBounds( grid.points[i][j], bounds[0], bounds[1] );
		}
	}

	// generate a bsp tree for the surface
	CM_PatchCollideFromGrid( &grid, pf );

	// expand by one unit for epsilon purposes
	bounds[0][0] -= 1;
	bounds[0][1] -= 1;
	bounds[0][2] -= 1;

	bounds[1][0] += 1;
	bounds[1][1] += 1;
	bounds[1][2] += 1;

	return pf;
}

/* ---- CM_TracePointThroughPatchCollide  0x0041CBD0 ---- VERIFIED
 * Per plane (0x0041CC12-0x0041CCC8) the offset is summed (z+x)+y and kept
 * unrounded; d1 = ((x+y)+z) - dist + offset is rounded to float, but d2 =
 * ((z+x)+y) - dist + offset and the quotient d1/(d1-d2) stay unrounded for the
 * compares.  For the hit facet (0x0041CD82-0x0041CE00) nothing is rounded
 * until the fraction is stored: offset (z+y)+x, d1 (y+z)+x, d2 (x+y)+z.
 * Q3's text.  tw must be the parameter itself: a cast copy in a local shifts
 * the order VC7 picks for every commutative x87 operand in the body. */
void CM_TracePointThroughPatchCollide( cmPatchTrace_t *tw,
									   const patchCollide_t *pc )
{
	qboolean frontFacing[MAX_PATCH_PLANES];
	float intersection[MAX_PATCH_PLANES];
	float intersect;
	const patchPlane_t *planes;
	const facet_t *facet;
	int i, j, k;
	rfloat offset;
	float d1;
	rfloat d2, dist, q;

	if ( !CM_PLAYER_CURVE_CLIP()->integer || !tw->isPoint ) {
		return;
	}

	// determine the trace's relationship to all planes
	planes = pc->planes;
	for ( i = 0; i < pc->numPlanes; i++, planes++ ) {
		offset = CM_DOT( tw->offsets[planes->signbits], planes->plane, 2, 0, 1 );
		d1 = CM_DOT( tw->start, planes->plane, 0, 1, 2 ) - planes->plane[3] + offset;
		d2 = CM_DOT( tw->end, planes->plane, 2, 0, 1 ) - planes->plane[3] + offset;
		if ( d1 <= 0 ) {
			frontFacing[i] = qfalse;
		} else {
			frontFacing[i] = qtrue;
		}
		if ( d1 == d2 ) {
			intersection[i] = 99999;
		} else {
			q = d1 / ( d1 - d2 );
			intersection[i] = q;
			if ( q <= 0 ) {
				intersection[i] = 99999;
			}
		}
	}

	// see if any of the surface planes are intersected
	facet = pc->facets;
	for ( i = 0; i < pc->numFacets; i++, facet++ ) {
		if ( !frontFacing[facet->surfacePlane] ) {
			continue;
		}
		intersect = intersection[facet->surfacePlane];
		if ( intersect < 0 ) {
			continue;       // surface is behind the starting point
		}
		if ( intersect > tw->trace.fraction ) {
			continue;       // already hit something closer
		}
		for ( j = 0; j < facet->numBorders; j++ ) {
			k = facet->borderPlanes[j];
			if ( frontFacing[k] ^ facet->borderInward[j] ) {
				if ( intersection[k] > intersect ) {
					break;
				}
			} else {
				if ( intersection[k] < intersect ) {
					break;
				}
			}
		}
		if ( j == facet->numBorders ) {
			// we hit this facet
			planes = &pc->planes[facet->surfacePlane];

			// calculate intersection with a slight pushoff
			offset = CM_DOT( tw->offsets[planes->signbits], planes->plane, 2, 1, 0 );
			dist = CM_DOT( tw->start, planes->plane, 1, 2, 0 ) - planes->plane[3] + offset;
			d2 = CM_DOT( tw->end, planes->plane, 0, 1, 2 ) - planes->plane[3] + offset;
			q = ( dist - SURFACE_CLIP_EPSILON ) / ( dist - d2 );
			tw->trace.fraction = q;

			if ( q < 0 ) {
				tw->trace.fraction = 0;
			}

			VectorCopy( planes->plane, tw->trace.normal );
		}
	}
}

/* ---- CM_SightTracePointThroughPatchCollide  0x0041CE60 ---- VERIFIED
 * The per-plane pass rounds exactly as CM_TracePointThroughPatchCollide's
 * (0x0041CEA1-0x0041CF57).  For the hit facet (0x0041D012-0x0041D095) the
 * offset is summed (z+y)+x, d1 = ((y+z)+x) - dist + offset is rounded to
 * float, and d2 = ((x+y)+z) - dist + offset, d1-d2 and the quotient stay
 * unrounded.  Q3's point trace with the plane-cross test of a sight trace. */
qboolean CM_SightTracePointThroughPatchCollide( cmPatchTrace_t *tw,
												const patchCollide_t *pc )
{
	qboolean frontFacing[MAX_PATCH_PLANES];
	float intersection[MAX_PATCH_PLANES];
	float intersect;
	const patchPlane_t *planes;
	const facet_t *facet;
	int i, j, k;
	rfloat offset;
	float d1;
	rfloat d2, q;

	if ( !CM_PLAYER_CURVE_CLIP()->integer || !tw->isPoint ) {
		return qtrue;
	}

	// determine the trace's relationship to all planes
	planes = pc->planes;
	for ( i = 0; i < pc->numPlanes; i++, planes++ ) {
		offset = CM_DOT( tw->offsets[planes->signbits], planes->plane, 2, 0, 1 );
		d1 = CM_DOT( tw->start, planes->plane, 0, 1, 2 ) - planes->plane[3] + offset;
		d2 = CM_DOT( tw->end, planes->plane, 2, 0, 1 ) - planes->plane[3] + offset;
		if ( d1 <= 0 ) {
			frontFacing[i] = qfalse;
		} else {
			frontFacing[i] = qtrue;
		}
		if ( d1 == d2 ) {
			intersection[i] = 99999;
		} else {
			q = d1 / ( d1 - d2 );
			intersection[i] = q;
			if ( q <= 0 ) {
				intersection[i] = 99999;
			}
		}
	}

	// see if any of the surface planes are intersected
	facet = pc->facets;
	for ( i = 0; i < pc->numFacets; i++, facet++ ) {
		if ( !frontFacing[facet->surfacePlane] ) {
			continue;
		}
		intersect = intersection[facet->surfacePlane];
		if ( intersect < 0 ) {
			continue;       // surface is behind the starting point
		}
		for ( j = 0; j < facet->numBorders; j++ ) {
			k = facet->borderPlanes[j];
			if ( frontFacing[k] ^ facet->borderInward[j] ) {
				if ( intersection[k] > intersect ) {
					break;
				}
			} else {
				if ( intersection[k] < intersect ) {
					break;
				}
			}
		}
		if ( j == facet->numBorders ) {
			// we hit this facet
			planes = &pc->planes[facet->surfacePlane];

			// calculate intersection with a slight pushoff
			offset = CM_DOT( tw->offsets[planes->signbits], planes->plane, 2, 1, 0 );
			d1 = CM_DOT( tw->start, planes->plane, 1, 2, 0 ) - planes->plane[3] + offset;
			d2 = CM_DOT( tw->end, planes->plane, 0, 1, 2 ) - planes->plane[3] + offset;
			if ( d1 - d2 > 0 ) {
				if ( ( (rfloat)d1 - SURFACE_CLIP_EPSILON ) / ( d1 - d2 ) < 1 ) {
					return qfalse;
				}
			}
		}
	}

	return qtrue;
}

/* ---- CM_CheckFacetPlane  0x0041D0E0 ---- VERIFIED
 * d1 and d2 are summed (z+y)+x and rounded to float (0x0041D0EB-0x0041D11D).
 * The fraction stays unrounded on the x87 stack through the clamp and the
 * enterFrac/leaveFrac compare and is rounded only when it is stored
 * (0x0041D18F-0x0041D1FA): the fraction math is float-typed (dword
 * constants), so f is an rfloat and d1 is cast to rfloat. */
int CM_CheckFacetPlane(const float *plane, const vec3_t start, const vec3_t end,
                       float *enterFrac, float *leaveFrac, int *hit)
{
  float d1, d2;
  rfloat f;

  *hit = qfalse;

  d1 = ( float )( ( ( (double)start[2] * plane[2] + (double)start[1] * plane[1] )
                    + (double)plane[0] * start[0] ) - plane[3] );
  d2 = ( float )( ( ( (double)end[2] * plane[2] + (double)end[1] * plane[1] )
                    + (double)plane[0] * end[0] ) - plane[3] );

  if ( d1 > 0.0f && ( d2 >= SURFACE_CLIP_EPSILON || d2 >= d1 ) ) {
    return qfalse;
  }

  if ( d1 <= 0.0f && d2 <= 0.0f ) {
    return qtrue;
  }

  if ( d1 > d2 ) {
    f = ( (rfloat)d1 - SURFACE_CLIP_EPSILON ) / ( (rfloat)d1 - d2 );
    if ( f < 0.0f ) {
      f = 0.0f;
    }
    if ( f > *enterFrac ) {
      *enterFrac = ( float )f;
      *hit = qtrue;
    }
  } else {
    f = ( (rfloat)d1 + SURFACE_CLIP_EPSILON ) / ( (rfloat)d1 - d2 );
    if ( f > 1.0f ) {
      f = 1.0f;
    }
    if ( f < *leaveFrac ) {
      *leaveFrac = ( float )f;
    }
  }

  return qtrue;
}

/* ---- CM_TraceThroughPatchCollide  0x0041D210 ---- VERIFIED
 * Box (non-sphere) planes: retail sums the offset (z+y)+x on the x87 stack and
 * never stores it; plane[3] is rounded to float once (fstp) after the
 * subtract (surface, 0x0041D38F-0x0041D3E3) or after |offset| + dist
 * (border, 0x0041D594-0x0041D5EA).  offset and t are therefore rfloats.
 * Q3's text otherwise (no bestplane clear, CoD's leaveFrac = trace.fraction). */
void CM_TraceThroughPatchCollide( cmPatchTrace_t *tw,
								  const patchCollide_t *pc )
{
	int i, j, hit, hitnum;
	rfloat offset, t;
	float enterFrac, leaveFrac;
	const patchPlane_t *planes;
	const facet_t *facet;
	float plane[4];
	vec3_t startp, endp, bestplane;

	if ( tw->isPoint ) {
		CM_TracePointThroughPatchCollide( tw, pc );
		return;
	}

	facet = pc->facets;
	for ( i = 0; i < pc->numFacets; i++, facet++ ) {
		enterFrac = -1.0f;
		leaveFrac = tw->trace.fraction;
		hitnum = -1;

		planes = &pc->planes[facet->surfacePlane];
		VectorCopy( planes->plane, plane );
		plane[3] = planes->plane[3];
		if ( tw->sphere.use ) {
			plane[3] += tw->sphere.radius;

			t = CM_DOT( plane, tw->sphere.offset, 0, 2, 1 );
			if ( t > 0.0f ) {
				VectorSubtract( tw->start, tw->sphere.offset, startp );
				VectorSubtract( tw->end, tw->sphere.offset, endp );
			} else {
				VectorAdd( tw->start, tw->sphere.offset, startp );
				VectorAdd( tw->end, tw->sphere.offset, endp );
			}
		} else {
			offset = CM_DOT( tw->offsets[planes->signbits], plane, 2, 1, 0 );
			plane[3] -= offset;
			VectorCopy( tw->start, startp );
			VectorCopy( tw->end, endp );
		}

		if ( !CM_CheckFacetPlane( plane, startp, endp, &enterFrac, &leaveFrac,
								  &hit ) ) {
			continue;
		}
		if ( hit ) {
			VectorCopy( plane, bestplane );
		}

		for ( j = 0; j < facet->numBorders; j++ ) {
			planes = &pc->planes[facet->borderPlanes[j]];
			if ( facet->borderInward[j] ) {
				VectorNegate( planes->plane, plane );
				plane[3] = -planes->plane[3];
			} else {
				VectorCopy( planes->plane, plane );
				plane[3] = planes->plane[3];
			}
			if ( tw->sphere.use ) {
				plane[3] += tw->sphere.radius;

				t = CM_DOT( plane, tw->sphere.offset, 0, 2, 1 );
				if ( t > 0.0f ) {
					VectorSubtract( tw->start, tw->sphere.offset, startp );
					VectorSubtract( tw->end, tw->sphere.offset, endp );
				} else {
					VectorAdd( tw->start, tw->sphere.offset, startp );
					VectorAdd( tw->end, tw->sphere.offset, endp );
				}
			} else {
				offset = CM_DOT( tw->offsets[planes->signbits], plane, 2, 1, 0 );
				plane[3] += FCAST( fabs( offset ) );
				VectorCopy( tw->start, startp );
				VectorCopy( tw->end, endp );
			}

			if ( !CM_CheckFacetPlane( plane, startp, endp, &enterFrac,
									  &leaveFrac, &hit ) ) {
				break;
			}
			if ( hit ) {
				hitnum = j;
				VectorCopy( plane, bestplane );
			}
		}
		if ( j < facet->numBorders ) {
			continue;
		}
		if ( hitnum == facet->numBorders - 1 ) {
			continue;
		}

		if ( enterFrac < leaveFrac && enterFrac >= 0.0f ) {
			if ( enterFrac < tw->trace.fraction ) {
				if ( enterFrac < 0.0f ) {
					enterFrac = 0.0f;
				}
				tw->trace.fraction = enterFrac;
				VectorCopy( bestplane, tw->trace.normal );
			}
		}
	}
}

/* ---- CM_SightTraceThroughPatchCollide  0x0041D710 ---- VERIFIED
 * Box planes as in CM_TraceThroughPatchCollide: offset summed (z+y)+x and
 * kept unrounded, plane[3] rounded once (surface 0x0041D881-0x0041D8D6,
 * border 0x0041DA64-0x0041DABA). */
qboolean CM_SightTraceThroughPatchCollide( cmPatchTrace_t *tw,
										   const patchCollide_t *pc )
{
	int i, j, hit, hitnum;
	rfloat offset, t;
	float enterFrac, leaveFrac;
	const patchPlane_t *planes;
	const facet_t *facet;
	float plane[4];
	vec3_t startp, endp;

	if ( tw->isPoint ) {
		return CM_SightTracePointThroughPatchCollide( tw, pc );
	}

	facet = pc->facets;
	for ( i = 0; i < pc->numFacets; i++, facet++ ) {
		enterFrac = -1.0f;
		leaveFrac = 1.0f;
		hitnum = -1;

		planes = &pc->planes[facet->surfacePlane];
		VectorCopy( planes->plane, plane );
		plane[3] = planes->plane[3];
		if ( tw->sphere.use ) {
			plane[3] += tw->sphere.radius;

			t = CM_DOT( plane, tw->sphere.offset, 0, 2, 1 );
			if ( t > 0.0f ) {
				VectorSubtract( tw->start, tw->sphere.offset, startp );
				VectorSubtract( tw->end, tw->sphere.offset, endp );
			} else {
				VectorAdd( tw->start, tw->sphere.offset, startp );
				VectorAdd( tw->end, tw->sphere.offset, endp );
			}
		} else {
			offset = CM_DOT( tw->offsets[planes->signbits], plane, 2, 1, 0 );
			plane[3] -= offset;
			VectorCopy( tw->start, startp );
			VectorCopy( tw->end, endp );
		}

		if ( !CM_CheckFacetPlane( plane, startp, endp, &enterFrac, &leaveFrac,
								  &hit ) ) {
			continue;
		}

		for ( j = 0; j < facet->numBorders; j++ ) {
			planes = &pc->planes[facet->borderPlanes[j]];
			if ( facet->borderInward[j] ) {
				VectorNegate( planes->plane, plane );
				plane[3] = -planes->plane[3];
			} else {
				VectorCopy( planes->plane, plane );
				plane[3] = planes->plane[3];
			}
			if ( tw->sphere.use ) {
				plane[3] += tw->sphere.radius;

				t = CM_DOT( plane, tw->sphere.offset, 0, 2, 1 );
				if ( t > 0.0f ) {
					VectorSubtract( tw->start, tw->sphere.offset, startp );
					VectorSubtract( tw->end, tw->sphere.offset, endp );
				} else {
					VectorAdd( tw->start, tw->sphere.offset, startp );
					VectorAdd( tw->end, tw->sphere.offset, endp );
				}
			} else {
				offset = CM_DOT( tw->offsets[planes->signbits], plane, 2, 1, 0 );
				plane[3] += FCAST( fabs( offset ) );
				VectorCopy( tw->start, startp );
				VectorCopy( tw->end, endp );
			}

			if ( !CM_CheckFacetPlane( plane, startp, endp, &enterFrac,
									  &leaveFrac, &hit ) ) {
				break;
			}
			if ( hit ) {
				hitnum = j;
			}
		}
		if ( j < facet->numBorders ) {
			continue;
		}
		if ( hitnum == facet->numBorders - 1 ) {
			continue;
		}

		if ( enterFrac < leaveFrac && enterFrac >= 0.0f ) {
			return qfalse;
		}
	}

	return qtrue;
}

/* ---- CM_PositionTestInPatchCollide  0x0041DB90 ---- VERIFIED
 * Retail never stores the expanded distance or the shifted start point: both
 * stay on the x87 stack (53-bit) from the loads to the `> 0` compare (surface
 * 0x0041DBD0-0x0041DCB2, border 0x0041DCD0-0x0041DDCF).  The box offset is
 * summed (z+y)+x and the test is ((n2*s2 + s1*n1) + s0*n0) - dist.  plane
 * and startp are therefore rfloat arrays (VC7 keeps them in registers); nothing
 * is rounded to float between the loads and the compare. */
qboolean CM_PositionTestInPatchCollide( cmPatchTrace_t *tw,
										const patchCollide_t *pc )
{
	int i, j;
	rfloat offset, t;
	const patchPlane_t *planes;
	const facet_t *facet;
	rfloat plane[4];
	rfloat startp[3];

	if ( tw->isPoint ) {
		return qfalse;
	}

	facet = pc->facets;
	for ( i = 0; i < pc->numFacets; i++, facet++ ) {
		planes = &pc->planes[facet->surfacePlane];
		VectorCopy( planes->plane, plane );
		plane[3] = planes->plane[3];
		if ( tw->sphere.use ) {
			plane[3] += tw->sphere.radius;

			t = CM_DOT( plane, tw->sphere.offset, 0, 2, 1 );
			if ( t > 0 ) {
				VectorSubtract( tw->start, tw->sphere.offset, startp );
			} else {
				VectorAdd( tw->start, tw->sphere.offset, startp );
			}
		} else {
			offset = CM_DOT( tw->offsets[planes->signbits], plane, 2, 1, 0 );
			plane[3] -= offset;
			VectorCopy( tw->start, startp );
		}

		if ( CM_DOT( plane, startp, 2, 1, 0 ) - plane[3] > 0.0f ) {
			continue;
		}

		for ( j = 0; j < facet->numBorders; j++ ) {
			planes = &pc->planes[facet->borderPlanes[j]];
			if ( facet->borderInward[j] ) {
				VectorNegate( planes->plane, plane );
				plane[3] = -planes->plane[3];
			} else {
				VectorCopy( planes->plane, plane );
				plane[3] = planes->plane[3];
			}
			if ( tw->sphere.use ) {
				plane[3] += tw->sphere.radius;

				t = CM_DOT( plane, tw->sphere.offset, 0, 2, 1 );
				if ( t > 0.0f ) {
					VectorSubtract( tw->start, tw->sphere.offset, startp );
				} else {
					VectorAdd( tw->start, tw->sphere.offset, startp );
				}
			} else {
				offset = CM_DOT( tw->offsets[planes->signbits], plane, 2, 1, 0 );
				plane[3] += fabs( offset );
				VectorCopy( tw->start, startp );
			}

			if ( CM_DOT( plane, startp, 2, 1, 0 ) - plane[3] > 0.0f ) {
				break;
			}
		}
		if ( j < facet->numBorders ) {
			continue;
		}
		// inside this patch facet
		return qtrue;
	}
	return qfalse;
}
