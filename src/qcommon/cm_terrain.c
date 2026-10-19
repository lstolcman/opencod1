/*
 * qcommon/cm_terrain.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/qcommon/cm_terrain.c
 *
 * Retail range 0x00420000-0x00421BE0, 9 functions.
 *
 * @fidelity: likely
 */

#include "../qcommon/cm_local.h"

/* cm_boxOnTerrainWarned (0x01407438) is owned by this unit; defined here and declared
 * locally so this unit needs only cm_local.h, exactly as cm_trace.c does. */
int cm_boxOnTerrainWarned;

extern void *Hunk_AllocAlignInternal( int size, int align );
extern void PerpendicularVector( vec3_t dst, const vec3_t src );
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b,
						  const vec3_t c );

typedef struct terrainPlane_t
{
	vec3_t normal;                  /* +0x00 */
	float dist;                     /* +0x0C */
} terrainPlane_t;
COD1_ASSERT_SIZE( terrainPlane_t, 16 );

typedef struct terrainVertexSphere_t
{
	int checkcount;                 /* +0x00 */
	vec3_t origin;                  /* +0x04 */
} terrainVertexSphere_t;
COD1_ASSERT_SIZE( terrainVertexSphere_t, 16 );

typedef struct terrainEdgeCylinder_t
{
	int checkcount;                 /* +0x00 */
	vec3_t origin;                  /* +0x04 */
	vec3_t radialAxes[2];           /* +0x10 */
	vec3_t axis;                    /* +0x28 */
	float length;                   /* +0x34 */
} terrainEdgeCylinder_t;
COD1_ASSERT_SIZE( terrainEdgeCylinder_t, 56 );

typedef struct terrainFacet_t
{
	terrainPlane_t surfacePlane;                    /* +0x00 */
	terrainPlane_t edgePlanes[2];                   /* +0x10 */
	terrainVertexSphere_t   *vertexSpheres[3];      /* +0x30 */
	terrainEdgeCylinder_t   *edgeCylinders[3];      /* +0x3C */
} terrainFacet_t;
COD1_ASSERT_SIZE( terrainFacet_t, 72 );

typedef struct terrainCollide_t
{
	unsigned short facetCount;      /* +0x00 */
	byte capsuleOffsetSign;         /* +0x02 */
	byte pad_0x03;                  /* +0x03 alignment: the facet array follows at +4; no access */
} terrainCollide_t;
COD1_ASSERT_SIZE( terrainCollide_t, 4 );

#define CM_TC( p )          ( ( const terrainCollide_t * ) (const void *) (p) )
#define CM_TC_FACETS( p )   ( ( const terrainFacet_t * ) (const void *) \
							  ( ( const char * ) (p) + 4 ) )
#define CM_TC_FACETS_W( p ) ( ( terrainFacet_t * ) (void *) \
							  ( ( char * ) (p) + 4 ) )

#define TERRAIN_CLIP_EPSILON    0.125f
#define TERRAIN_BARY_MIN        ( -0.001f )
#define TERRAIN_BARY_MAX        1.001f
/* 0x00568FA4 -- the sphere walker backs every accepted fraction off by this. */
#define TERRAIN_HIT_EPSILON     0.00001f

#define CM_CHECKCOUNT           cm_checkcount

/* ---- FindEdge  0x00420000 ---- VERIFIED */
int __cdecl FindEdge( short ( *edges )[2], int numEdges, short p1, short p2 )
{
	int i;

	/* An edge matches ( p1, p2 ) in either orientation.  Both endpoints are
	 * compared as 16-bit values (`cmp word ptr [edx+eax*4+2],di`,
	 * 0x0042000F). */
	for ( i = 0; i < numEdges; i++ ) {
		if ( edges[i][0] == p2 && edges[i][1] == p1 ) {
			return i;
		}
		if ( edges[i][0] == p1 && edges[i][1] == p2 ) {
			return i;
		}
	}
	return -1;
}

/* ---- CM_GenerateTerrainCollide  0x00420030 ---- VERIFIED */
/*
 * Builds the sphere/cylinder terrain collide from a triangle-soup index/vertex
 * list.  Layout (see cm_patch_local.h): a 4-byte header (terrainCollide_t:
 * facetCount plus the capsuleOffsetSign flag) followed by inline 72-byte
 * facets, plus two side arrays -- one 16-byte vertex sphere per referenced
 * vertex and one 56-byte edge cylinder per surviving edge.
 *
 * Two big _WORD scratch arrays live on the stack.  vertMap[] first stores, per
 * edge, the third (opposite) vertex of the triangle that created that edge;
 * after the memset it is reused as a vertex -> sphere-index map.  edgeVerts[]
 * holds the two endpoint vertices of each edge.
 *
 * PlaneFromPoints's argument order is load-bearing: the three vertices must be
 * passed as ( a, b, c ) = ( points[i0], points[i1], points[i2] ).  Any other
 * order negates every facet's surface plane, so terrain becomes solid from
 * behind and passable from the front (see the IDA note at 0x0042050D).
 */
void *__cdecl CM_GenerateTerrainCollide( int numIndexes, const short *indexes,
										 int numVerts, const vec3_t *points,
										 vec3_t *bounds )
{
	short vertMap[0x10000];         /* per-edge opposite vertex, then reused as
									 * a vertex -> sphere-index map */
	short edgeVerts[0x10000][2];    /* the two endpoint vertices of each edge */
	terrainCollide_t *tc;
	terrainFacet_t *facets;
	terrainVertexSphere_t *vertSpheres;
	terrainEdgeCylinder_t *edgeCylinders;
	int numFacets;
	int numEdges;
	int numSpheres;
	/* f is the one counter of the index-triple, facet and edge-cylinder
	 * passes: retail keeps all three in the same frame slot (esp+20h at
	 * 0x00420096, 0x004204AC, 0x00420C5F), and separate counters move it. */
	int i, f, k, j;
	qboolean hasUpFacing;
	qboolean hasDownFacing;

	numFacets = numIndexes / 3;
	if ( numIndexes % 3 ) {
		Com_Error( ERR_DROP, "\x15" "CM_GenerateTerrainCollide: numIndexes % 3 != 0, corrupt bsp?" );
	}
	if ( (unsigned int) numVerts >= 0x10000 ) {
		Com_Error( ERR_DROP, "\x15" "CM_GenerateTerrainCollide: too many vertices" );
	}

	/* ---- Pass 1: build the unique edge list, dropping every interior edge
	 * whose two triangles are near-coplanar (no cylinder on a flat seam). ---- */
	numEdges = 0;
	for ( f = 0; f < numIndexes; f += 3 ) {
		for ( k = 0; k < 3; k++ ) {
			short p1  = indexes[f + k];
			short p2  = indexes[f + ( k + 1 ) % 3];
			short opp = indexes[f + ( k + 2 ) % 3];
			int edge;

			if ( p1 == p2 ) {
				Com_Error( ERR_DROP, "\x15" "CM_GenerateTerrainCollide: degenerate triangle, corrupt bsp?" );
			}

			/* ( p2, p1 ): FindEdge is symmetric; this order gives retail's
			 * p1@si / p2@bx (0x004200C8). */
			edge = FindEdge( ( short ( * )[2] ) edgeVerts, numEdges, p2, p1 );
			if ( edge < 0 ) {
				if ( (unsigned int)numEdges >= 0x10000 ) {
					Com_Error( ERR_DROP, "\x15" "CM_GenerateTerrainCollide: too many edges" );
				}
				edgeVerts[numEdges][0] = p1;
				edgeVerts[numEdges][1] = p2;
				vertMap[numEdges] = opp;
				numEdges++;
			} else {
				vec3_t dP1, dP2, dOther, n, eOther1, eOther2;
				float side, lenSq1, lenSq2, maxLenSq;

				VectorSubtract( points[p1], points[opp], dP1 );
				VectorSubtract( points[p2], points[opp], dP2 );
				VectorSubtract( points[vertMap[edge]], points[opp], dOther );
				CrossProduct( dP2, dP1, n );

				/* Retail sums every dot product here as (z+y)+x
				 * (0x00420266, 0x004202C0, 0x00420305, 0x00420362). */
				side = ( n[2] * dOther[2] + n[1] * dOther[1] ) + n[0] * dOther[0];
				if ( side > 0.0f ) {
					VectorSubtract( points[vertMap[edge]], points[p1], eOther1 );
					lenSq1 = ( eOther1[2] * eOther1[2] + eOther1[1] * eOther1[1] )
							 + eOther1[0] * eOther1[0];
					VectorSubtract( points[vertMap[edge]], points[p2], eOther2 );
					lenSq2 = ( eOther2[2] * eOther2[2] + eOther2[1] * eOther2[1] )
							 + eOther2[0] * eOther2[0];
					/* Retail `test ah,41h` + `jnz` (0x00420344): only an
					 * ordered lenSq1 > lenSq2 keeps lenSq1. */
					maxLenSq = ( lenSq1 > lenSq2 ) ? lenSq1 : lenSq2;

					/* n.n is left unrounded and compared (fcompp, 0x0042037E)
					 * with maxLenSq times the FLOAT flt_5690F8 (0x31DBE6FF,
					 * 6.4000000854491645e-09); float*float is exact at _PC_53. */
					if ( maxLenSq * 0.0000000064000001f
						 > ( n[2] * n[2] + n[1] * n[1] ) + n[0] * n[0] ) {
						/* swap-remove the interior edge */
						edgeVerts[edge][0] = edgeVerts[numEdges - 1][0];
						edgeVerts[edge][1] = edgeVerts[numEdges - 1][1];
						vertMap[edge] = vertMap[--numEdges];
					}
				}
			}
		}
	}

	/* ---- vertMap now becomes a vertex -> sphere-index map: every vertex that
	 * survives as an edge endpoint is assigned one sphere. ---- */
	memset( vertMap, 0xFF, sizeof( vertMap ) );
	numSpheres = 0;
	for ( i = 0; i < numEdges; i++ ) {
		short *sp;

		sp = &vertMap[edgeVerts[i][0]];
		if ( *sp < 0 ) {
			*sp = numSpheres++;
		}
		sp = &vertMap[edgeVerts[i][1]];
		if ( *sp < 0 ) {
			*sp = numSpheres++;
		}
	}

	/* ---- Allocate the collide and its two side arrays. ---- */
	tc = Hunk_AllocAlignInternal( 72 * numFacets + 4, 32 );
	vertSpheres = Hunk_AllocAlignInternal( 16 * numSpheres, 32 );
	edgeCylinders = Hunk_AllocAlignInternal( 56 * numEdges, 32 );

	hasUpFacing = 0;
	hasDownFacing = 0;
	tc->facetCount = ( unsigned short ) numFacets;
	facets = CM_TC_FACETS_W( tc );

	/* ---- Pass 2: one facet per triangle.  Retail bounds this loop by the
	 * stored 16-bit facetCount (`test si,si` at 0x00420498, `movzx edx,word
	 * ptr [ecx]` at 0x00420860), not by the int numFacets. ---- */
	for ( f = 0; f < tc->facetCount; f++ ) {
		const short *tri = &indexes[3 * f];
		terrainFacet_t *facet = &facets[f];
		short i0 = tri[0];
		short i1 = tri[1];
		short i2 = tri[2];
		const float *b = points[i1];
		const float *a = points[i0];
		const float *c = points[i2];
		vec3_t edge1, edge2;
		float len1, len2, cosAngle, scale, negCos, inv1, inv2;
		short s0, s1, s2;
		int e0, e1, e2;

		/* Surface plane -- ( a, b, c ) MUST be ( i0, i1, i2 ) in this order. */
		PlaneFromPoints( ( float * ) &facet->surfacePlane, a, b, c );
		if ( facet->surfacePlane.normal[2] < -0.001f ) {
			hasDownFacing = 1;
		} else if ( facet->surfacePlane.normal[2] > 0.001f ) {
			hasUpFacing = 1;
		}

		/* Two edge planes for the barycentric inside test.  edge1 = b - a and
		 * edge2 = c - a, each normalised, then de-skewed by the cosine of the
		 * angle between them. */
		VectorSubtract( b, a, edge1 );
		len1 = VectorNormalize( edge1 );
		VectorSubtract( c, a, edge2 );
		len2 = VectorNormalize( edge2 );
		/* Retail sums cosAngle as (z+y)+x (0x00420592) and each edge-plane
		 * dist as (y+z)+x over the stored float normal (0x00420678,
		 * 0x00420706). */
		cosAngle = ( edge2[2] * edge1[2] + edge2[1] * edge1[1] )
				   + edge2[0] * edge1[0];
		scale = 1.0f / ( 1.0f - cosAngle * cosAngle );
		VectorScale( edge1, scale, edge1 );
		VectorScale( edge2, scale, edge2 );
		negCos = -cosAngle;

		inv1 = 1.0f / len1;
		facet->edgePlanes[0].normal[0] = ( negCos * edge2[0] + edge1[0] ) * inv1;
		facet->edgePlanes[0].normal[1] = ( negCos * edge2[1] + edge1[1] ) * inv1;
		facet->edgePlanes[0].normal[2] = ( negCos * edge2[2] + edge1[2] ) * inv1;
		facet->edgePlanes[0].dist =
			( facet->edgePlanes[0].normal[1] * a[1]
			  + facet->edgePlanes[0].normal[2] * a[2] )
			+ facet->edgePlanes[0].normal[0] * a[0];

		inv2 = 1.0f / len2;
		facet->edgePlanes[1].normal[0] = ( negCos * edge1[0] + edge2[0] ) * inv2;
		facet->edgePlanes[1].normal[1] = ( negCos * edge1[1] + edge2[1] ) * inv2;
		facet->edgePlanes[1].normal[2] = ( negCos * edge1[2] + edge2[2] ) * inv2;
		facet->edgePlanes[1].dist =
			( facet->edgePlanes[1].normal[1] * a[1]
			  + facet->edgePlanes[1].normal[2] * a[2] )
			+ facet->edgePlanes[1].normal[0] * a[0];

		/* Vertex spheres for i0, i1, i2. */
		s0 = ( short ) vertMap[i0];
		facet->vertexSpheres[0] = ( s0 >= 0 ) ? &vertSpheres[s0] : NULL;
		s1 = ( short ) vertMap[i1];
		facet->vertexSpheres[1] = ( s1 >= 0 ) ? &vertSpheres[s1] : NULL;
		s2 = ( short ) vertMap[i2];
		facet->vertexSpheres[2] = ( s2 >= 0 ) ? &vertSpheres[s2] : NULL;

		/* Edge cylinders, each keyed on the edge opposite the like-numbered
		 * vertex: [0] = edge i1-i2, [1] = edge i0-i2, [2] = edge i0-i1
		 * (FindEdge is symmetric; [1] passes ( i2, i0 ) as retail does). */
		e0 = FindEdge( ( short ( * )[2] ) edgeVerts, numEdges, i1, i2 );
		facet->edgeCylinders[0] = ( e0 >= 0 ) ? &edgeCylinders[e0] : NULL;
		e1 = FindEdge( ( short ( * )[2] ) edgeVerts, numEdges, i2, i0 );
		facet->edgeCylinders[1] = ( e1 >= 0 ) ? &edgeCylinders[e1] : NULL;
		e2 = FindEdge( ( short ( * )[2] ) edgeVerts, numEdges, i0, i1 );
		facet->edgeCylinders[2] = ( e2 >= 0 ) ? &edgeCylinders[e2] : NULL;
	}

	tc->capsuleOffsetSign = ( hasDownFacing && !hasUpFacing ) ? 1 : 0;

	/* ---- Fold every vertex into the patch bounds and seed its sphere. ---- */
	bounds[0][0] = bounds[0][1] = bounds[0][2] = 262144.0f;
	bounds[1][0] = bounds[1][1] = bounds[1][2] = -262144.0f;
	for ( j = 0; j < numVerts; j++ ) {
		const float *p = points[j];
		short s;

		if ( p[0] < bounds[0][0] ) { bounds[0][0] = p[0]; }
		if ( p[0] > bounds[1][0] ) { bounds[1][0] = p[0]; }
		if ( p[1] < bounds[0][1] ) { bounds[0][1] = p[1]; }
		if ( p[1] > bounds[1][1] ) { bounds[1][1] = p[1]; }
		if ( p[2] < bounds[0][2] ) { bounds[0][2] = p[2]; }
		if ( p[2] > bounds[1][2] ) { bounds[1][2] = p[2]; }

		s = ( short ) vertMap[j];
		if ( s >= 0 ) {
			vertSpheres[s].checkcount = 0;
			VectorCopy( p, vertSpheres[s].origin );
		}
	}

	/* ---- Build each surviving edge's cylinder. ---- */
	for ( f = 0; f < numEdges; f++ ) {
		terrainEdgeCylinder_t *cyl = &edgeCylinders[f];
		int ea = edgeVerts[f][0];
		int eb = edgeVerts[f][1];

		cyl->checkcount = 0;
		VectorCopy( points[ea], cyl->origin );
		VectorSubtract( points[eb], points[ea], cyl->axis );
		cyl->length = VectorNormalize( cyl->axis );
		PerpendicularVector( cyl->radialAxes[0], cyl->axis );
		CrossProduct( cyl->axis, cyl->radialAxes[0], cyl->radialAxes[1] );
	}

	return tc;
}

/* ---- CM_TracePointThroughTerrainCollide  0x00420D50 ---- VERIFIED */
void __cdecl CM_TracePointThroughTerrainCollide(traceWork_t *tw, void *tcVoid)
{
	const terrainCollide_t *tc = CM_TC( tcVoid );
	const terrainFacet_t *facet;
	int i;
	float d1, d2, frac, e0;
	/* The rfloat locals are values retail keeps on the x87 stack at
	 * _PC_53; each float twin is the copy retail stores with fst.  The
	 * ( rfloat ) casts keep the modern build at that precision too. */
	rfloat d1u, d2u, fracU, hitFrac, e0u, e1;
#ifndef COD1_VC7
	rfloat hitZ;
#endif
	vec3_t hit;

	/* Retail re-reads the 16-bit facetCount every iteration (0x00420EAA). */
	for ( i = 0; i < tc->facetCount; i++ ) {
		facet = &CM_TC_FACETS( tc )[i];

		/* d2 sums z+(y+x) and d1 (z+y)+x; each test reads the unrounded
		 * value (0x00420D62-0x00420DAA). */
		d2u = ( rfloat ) tw->end[2] * facet->surfacePlane.normal[2]
			  + ( ( rfloat ) tw->end[1] * facet->surfacePlane.normal[1]
				  + ( rfloat ) tw->end[0] * facet->surfacePlane.normal[0] )
			  - facet->surfacePlane.dist;
		d2 = d2u;
		if ( d2u >= 0.0f ) {
			continue;
		}

		d1u = ( ( rfloat ) tw->start[2] * facet->surfacePlane.normal[2]
				+ ( rfloat ) tw->start[1] * facet->surfacePlane.normal[1] )
			  + ( rfloat ) tw->start[0] * facet->surfacePlane.normal[0]
			  - facet->surfacePlane.dist;
		d1 = d1u;
		if ( d1u <= 0.0f ) {
			continue;
		}

		/* d1 - d2 from the stored floats; the enter quotient is compared
		 * with trace.fraction before it is rounded (0x00420DCF). */
		fracU = ( ( rfloat ) d1 - TERRAIN_CLIP_EPSILON ) / ( ( rfloat ) d1 - d2 );
		frac = fracU;
		if ( fracU >= tw->trace.fraction ) {
			continue;
		}

		/* hitFrac is never rounded; hit.x/y/z are stored, and e0 = (z+y)+x
		 * reads the UNROUNDED hit.z (0x00420DE4-0x00420E21). */
		hitFrac = ( rfloat ) d1 / ( ( rfloat ) d1 - d2 );
		hit[0] = tw->start[0] + ( rfloat ) tw->delta[0] * hitFrac;
		hit[1] = tw->start[1] + ( rfloat ) tw->delta[1] * hitFrac;
#ifdef COD1_VC7
		hit[2] = tw->start[2] + ( rfloat ) tw->delta[2] * hitFrac;
		e0u = ( ( rfloat ) hit[2] * facet->edgePlanes[0].normal[2]
				+ ( rfloat ) hit[1] * facet->edgePlanes[0].normal[1] )
			  + ( rfloat ) hit[0] * facet->edgePlanes[0].normal[0]
			  - facet->edgePlanes[0].dist;
#else
		hitZ = tw->start[2] + ( rfloat ) tw->delta[2] * hitFrac;
		hit[2] = hitZ;
		e0u = ( hitZ * facet->edgePlanes[0].normal[2]
				+ ( rfloat ) hit[1] * facet->edgePlanes[0].normal[1] )
			  + ( rfloat ) hit[0] * facet->edgePlanes[0].normal[0]
			  - facet->edgePlanes[0].dist;
#endif
		/* The low-bound test reads the unrounded e0, the high-bound test
		 * the stored float. */
		e0 = e0u;
		if ( e0u < TERRAIN_BARY_MIN ) {
			continue;
		}
		if ( e0 > TERRAIN_BARY_MAX ) {
			continue;
		}

		/* e1 = (z+y)+x with the stored float hit.z, never rounded. */
		e1 = ( ( rfloat ) hit[2] * facet->edgePlanes[1].normal[2]
			   + ( rfloat ) hit[1] * facet->edgePlanes[1].normal[1] )
			 + ( rfloat ) hit[0] * facet->edgePlanes[1].normal[0]
			 - facet->edgePlanes[1].dist;
		if ( e1 < TERRAIN_BARY_MIN ) {
			continue;
		}
		if ( e0 + e1 > TERRAIN_BARY_MAX ) {
			continue;
		}

		tw->trace.fraction = frac;
		VectorCopy( facet->surfacePlane.normal, tw->trace.normal );
	}
}

/* ---- CM_TraceSphereThroughTerrainCollide  0x00420EC0 ---- */
/*
 * Two bodies.  The COD1_VC7 one is retail's source shape: VC7 compiles it to
 * retail's exact bytes, keeping on the x87 stack (at _PC_53) every value the
 * other body models as a double.  The modern body spells out that precision
 * (which values retail rounds, and each sum's order) for a compiler that
 * rounds at every float assignment.
 */
void __cdecl CM_TraceSphereThroughTerrainCollide( traceWork_t *tw,
												 const terrainCollide_t *tc )
{
#ifdef COD1_VC7
	terrainEdgeCylinder_t *cyl;
	terrainVertexSphere_t *vert;
	int i, e;
	unsigned int outside;
	/* t is one shared temporary: capDist on the deep path, the cylinder
	 * fraction below (a separate local permutes VC7's operand order). */
	rfloat capOffset, t, off, e1, hitFrac, disc, frac, inv, axialAt, vlenSq;
	float capDelta, radius, negRadius, d1, d2, delta, invNz, baseE0, baseE1, e0;
	float radiusDelta, vdot;
	vec3_t hit, d, local, n;

	capOffset = tw->sphere.halfheight - tw->sphere.radius;
	if ( tc->capsuleOffsetSign ) {
		capDelta = capOffset * -2.0f;
		tw->start[2] += capOffset;
		tw->end[2] += capOffset;
	} else {
		capDelta = capOffset + capOffset;
		tw->start[2] -= capOffset;
		tw->end[2] -= capOffset;
	}

	radius = tw->sphere.radius + TERRAIN_CLIP_EPSILON;
	negRadius = -radius;

	for ( i = 0; i < tc->facetCount; i++ ) {
		d2 = ( tw->end[2] * CM_TC_FACETS( tc )[i].surfacePlane.normal[2]
			   + tw->end[1] * CM_TC_FACETS( tc )[i].surfacePlane.normal[1] )
			 + tw->end[0] * CM_TC_FACETS( tc )[i].surfacePlane.normal[0]
			 - CM_TC_FACETS( tc )[i].surfacePlane.dist;
		if ( d2 >= radius ) {
			continue;
		}
		d1 = ( CM_TC_FACETS( tc )[i].surfacePlane.normal[2] * tw->start[2]
			   + CM_TC_FACETS( tc )[i].surfacePlane.normal[1] * tw->start[1] )
			 + CM_TC_FACETS( tc )[i].surfacePlane.normal[0] * tw->start[0]
			 - CM_TC_FACETS( tc )[i].surfacePlane.dist;
		delta = d1 - d2;
		if ( delta <= 0.0f ) {
			continue;
		}

		if ( d1 <= negRadius ) {
			t = capDelta * CM_TC_FACETS( tc )[i].surfacePlane.normal[2] + d1;
			if ( t <= negRadius ) {
				continue;
			}
			invNz = 1.0f / CM_TC_FACETS( tc )[i].surfacePlane.normal[2];
			off = ( negRadius - d1 ) * invNz;
			baseE0 = DotProduct( tw->start, CM_TC_FACETS( tc )[i].edgePlanes[0].normal )
					 - CM_TC_FACETS( tc )[i].edgePlanes[0].dist;
			baseE1 = DotProduct( tw->start, CM_TC_FACETS( tc )[i].edgePlanes[1].normal )
					 - CM_TC_FACETS( tc )[i].edgePlanes[1].dist;
			e0 = off * CM_TC_FACETS( tc )[i].edgePlanes[0].normal[2] + baseE0;
			if ( e0 >= 0.0f ) {
				e1 = off * CM_TC_FACETS( tc )[i].edgePlanes[1].normal[2] + baseE1;
				if ( e1 >= 0.0f && e0 + e1 <= 1.0f ) {
					VectorCopy( CM_TC_FACETS( tc )[i].surfacePlane.normal, tw->trace.normal );
					tw->trace.fraction = 0.0f;
					tw->trace.startsolid = 1;
					return;
				}
			}
			if ( t < radius ) {
				off = capDelta;
			} else {
				off = ( radius - d1 ) * invNz;
			}
			e0 = off * CM_TC_FACETS( tc )[i].edgePlanes[0].normal[2] + baseE0;
			if ( e0 < 0.0f ) {
				continue;
			}
			e1 = off * CM_TC_FACETS( tc )[i].edgePlanes[1].normal[2] + baseE1;
			if ( e1 < 0.0f ) {
				continue;
			}
			if ( e0 + e1 > 1.0f ) {
				continue;
			}
			VectorCopy( CM_TC_FACETS( tc )[i].surfacePlane.normal, tw->trace.normal );
			tw->trace.fraction = 0.0f;
			tw->trace.startsolid = 1;
			return;
		}

		if ( d1 < radius ) {
			hitFrac = 0.0f;
			VectorCopy( tw->start, hit );
		} else {
			hitFrac = ( d1 - radius ) / delta;
			if ( hitFrac > tw->trace.fraction ) {
				continue;
			}
			VectorMA( tw->start, hitFrac, tw->delta, hit );
		}
		e0 = DotProduct( hit, CM_TC_FACETS( tc )[i].edgePlanes[0].normal )
			 - CM_TC_FACETS( tc )[i].edgePlanes[0].dist;
		e1 = DotProduct( hit, CM_TC_FACETS( tc )[i].edgePlanes[1].normal )
			 - CM_TC_FACETS( tc )[i].edgePlanes[1].dist;
		outside = ( e0 + e1 > 1.0f );
		outside |= ( e0 < 0.0f ) << 1;
		outside |= ( e1 < 0.0f ) << 2;
		if ( !outside ) {
			VectorCopy( CM_TC_FACETS( tc )[i].surfacePlane.normal, tw->trace.normal );
			if ( hitFrac <= TERRAIN_HIT_EPSILON ) {
				tw->trace.fraction = 0.0f;
				tw->trace.startsolid = 1;
				return;
			}
			tw->trace.fraction = hitFrac - TERRAIN_HIT_EPSILON;
			continue;
		}

		for ( e = 0; e < 3; e++ ) {
			if ( outside & ( 1 << e ) ) {
				cyl = CM_TC_FACETS( tc )[i].edgeCylinders[e];
				if ( !cyl ) {
					continue;
				}
				if ( cyl->checkcount == CM_CHECKCOUNT ) {
					continue;
				}
				cyl->checkcount = CM_CHECKCOUNT;
				VectorSubtract( tw->start, cyl->origin, d );
				local[0] = DotProduct( d, cyl->radialAxes[0] );
				local[1] = DotProduct( d, cyl->radialAxes[1] );
				local[2] = DotProduct( d, cyl->axis );
				radiusDelta = local[0] * local[0] + local[1] * local[1] - radius * radius;
				if ( radiusDelta <= 0.0f ) {
					if ( local[2] < 0.0f ) {
						continue;
					}
					if ( local[2] > cyl->length ) {
						continue;
					}
					VectorCopy( CM_TC_FACETS( tc )[i].surfacePlane.normal, tw->trace.normal );
					tw->trace.fraction = 0.0f;
					tw->trace.startsolid = 1;
					return;
				}
				/* d[0..1] now hold the velocity in the cylinder frame. */
				d[0] = DotProduct( tw->delta, cyl->radialAxes[0] );
				d[1] = DotProduct( tw->delta, cyl->radialAxes[1] );
				vdot = d[0] * local[0] + d[1] * local[1];
				if ( vdot >= 0.0f ) {
					continue;
				}
				vlenSq = d[0] * d[0] + d[1] * d[1];
				disc = vdot * vdot - vlenSq * radiusDelta;
				if ( disc <= 0.0f ) {
					continue;
				}
				t = ( -sqrt( disc ) - vdot ) / vlenSq;
				if ( t < tw->trace.fraction ) {
					axialAt = DotProduct( cyl->axis, tw->delta ) * t + local[2];
					if ( axialAt < 0.0f ) {
						continue;
					}
					if ( axialAt > cyl->length ) {
						continue;
					}
					inv = 1.0f / radius;
					n[0] = ( d[0] * t + local[0] ) * inv;
					n[1] = ( d[1] * t + local[1] ) * inv;
					VectorScale( cyl->radialAxes[0], n[0], tw->trace.normal );
					VectorMA( tw->trace.normal, n[1], cyl->radialAxes[1], tw->trace.normal );
					if ( tw->trace.fraction <= TERRAIN_HIT_EPSILON ) {
						tw->trace.fraction = 0.0f;
						tw->trace.startsolid = 1;
						return;
					}
					tw->trace.fraction = t - TERRAIN_HIT_EPSILON;
				}
			} else {
				vert = CM_TC_FACETS( tc )[i].vertexSpheres[e];
				if ( !vert ) {
					continue;
				}
				if ( vert->checkcount == CM_CHECKCOUNT ) {
					continue;
				}
				vert->checkcount = CM_CHECKCOUNT;
				VectorSubtract( tw->start, vert->origin, d );
				radiusDelta = DotProduct( d, d ) - radius * radius;
				if ( radiusDelta <= 0.0f ) {
					VectorCopy( CM_TC_FACETS( tc )[i].surfacePlane.normal, tw->trace.normal );
					tw->trace.fraction = 0.0f;
					tw->trace.startsolid = 1;
					return;
				}
				vdot = DotProduct( tw->delta, d );
				if ( vdot >= 0.0f ) {
					continue;
				}
				vlenSq = tw->deltaLengthSquared;
				disc = vdot * vdot - vlenSq * radiusDelta;
				if ( disc < 0.0f ) {
					continue;
				}
				frac = ( -sqrt( disc ) - vdot ) / vlenSq;
				if ( frac < tw->trace.fraction ) {
					inv = 1.0f / radius;
					tw->trace.normal[0] = ( frac * tw->delta[0] + d[0] ) * inv;
					tw->trace.normal[1] = ( frac * tw->delta[1] + d[1] ) * inv;
					tw->trace.normal[2] = ( frac * tw->delta[2] + d[2] ) * inv;
					if ( tw->trace.fraction <= TERRAIN_HIT_EPSILON ) {
						tw->trace.fraction = 0.0f;
						tw->trace.startsolid = 1;
						return;
					}
					tw->trace.fraction = frac - TERRAIN_HIT_EPSILON;
				}
			}
		}
	}
#else
	const terrainFacet_t *facets = CM_TC_FACETS( tc );
	int numFacets = ( int ) tc->facetCount;
	/* Every double local in this walker is a value retail keeps on the x87
	 * stack at _PC_53 and never rounds to float.  capOffset is one: it is
	 * never stored, and start/end[2] are shifted by the unrounded value
	 * (0x00420EC0-0x00420F10). */
	double capOffset = ( double ) tw->sphere.halfheight - tw->sphere.radius;
	float capDelta, radius, negRadius;
	int i;

	if ( tc->capsuleOffsetSign ) {
		capDelta = ( float ) ( capOffset * -2.0 );
		tw->start[2] = ( float ) ( capOffset + tw->start[2] );
		tw->end[2] = ( float ) ( capOffset + tw->end[2] );
	} else {
		capDelta = ( float ) ( capOffset + capOffset );
		tw->start[2] = ( float ) ( tw->start[2] - capOffset );
		tw->end[2] = ( float ) ( tw->end[2] - capOffset );
	}

	radius = tw->sphere.radius + TERRAIN_CLIP_EPSILON;
	negRadius = -radius;

	for ( i = 0; i < numFacets; i++ ) {
		const terrainFacet_t *facet = &facets[i];
		const terrainPlane_t *surf = &facet->surfacePlane;
		double d1u, d2u;
		float d1, delta;

		/* d2 and d1 sum (z+y)+x.  d2 is tested against the radius
		 * unrounded and never stored; delta is the unrounded d1 - d2,
		 * rounded once (0x00420F50-0x00420F96). */
		d2u = ( ( double ) tw->end[2] * surf->normal[2]
				+ ( double ) tw->end[1] * surf->normal[1] )
			  + ( double ) tw->end[0] * surf->normal[0] - surf->dist;
		if ( d2u >= radius ) {
			continue;
		}

		d1u = ( ( double ) tw->start[2] * surf->normal[2]
				+ ( double ) tw->start[1] * surf->normal[1] )
			  + ( double ) tw->start[0] * surf->normal[0] - surf->dist;
		d1 = ( float ) d1u;
		delta = ( float ) ( d1u - d2u );
		if ( 0.0f >= delta ) {
			continue;
		}

		/* Retail `test ah,41h` + `jp` (0x00420FBE): a NaN d1 takes the
		 * above-plane branch. */
		if ( d1 <= negRadius ) {
			double capDist, off, e0u, e1u;
			float invNz, baseE0, baseE1, e0;

			/* capDist, off and e1 stay on the stack; e0 is stored with fst
			 * and its float is what e0 + e1 adds (0x00420FC4-0x0042106E).
			 * The capDist test is retail's jnp on ordered less-or-equal. */
			capDist = ( double ) capDelta * surf->normal[2] + d1;
			if ( capDist <= negRadius ) {
				continue;
			}

			invNz = 1.0f / surf->normal[2];
			off = ( ( double ) negRadius - d1 ) * invNz;

			baseE0 = ( tw->start[0] * facet->edgePlanes[0].normal[0]
					   + tw->start[1] * facet->edgePlanes[0].normal[1] )
					 + tw->start[2] * facet->edgePlanes[0].normal[2]
					 - facet->edgePlanes[0].dist;
			baseE1 = ( tw->start[0] * facet->edgePlanes[1].normal[0]
					   + tw->start[1] * facet->edgePlanes[1].normal[1] )
					 + tw->start[2] * facet->edgePlanes[1].normal[2]
					 - facet->edgePlanes[1].dist;

			e0u = off * facet->edgePlanes[0].normal[2] + baseE0;
			e0 = ( float ) e0u;
			if ( e0u >= 0.0 ) {
				e1u = off * facet->edgePlanes[1].normal[2] + baseE1;
				if ( e1u >= 0.0 && e0 + e1u <= 1.0 ) {
					goto startSolid;
				}
			}

			if ( capDist < radius ) {
				off = capDelta;
			} else {
				off = ( ( double ) radius - d1 ) * invNz;
			}

			/* Second pass: the final test is `!(>)`, so a NaN sum takes
			 * retail's jz fall-through into startSolid (0x0042110E). */
			e0u = off * facet->edgePlanes[0].normal[2] + baseE0;
			e0 = ( float ) e0u;
			if ( !( e0u < 0.0 ) ) {
				e1u = off * facet->edgePlanes[1].normal[2] + baseE1;
				if ( !( e1u < 0.0 ) && !( e0 + e1u > 1.0 ) ) {
					goto startSolid;
				}
			}
			continue;

startSolid:
			VectorCopy( surf->normal, tw->trace.normal );
			tw->trace.fraction = 0.0f;
			tw->trace.startsolid = 1;
			return;
		} else {
			double hitFrac, e1u;
			double hit[3];
			float e0;
			unsigned int outside;
			int e;

			/* hitFrac and hit[0..2] stay on the stack (0x0042115E-0x004211A2).
			 * `d1 < radius` is retail's `test ah,5` + `jp`, so a NaN d1 takes
			 * the computed-hitFrac path. */
			if ( d1 < radius ) {
				hitFrac = 0.0;
				hit[0] = tw->start[0];
				hit[1] = tw->start[1];
				hit[2] = tw->start[2];
			} else {
				hitFrac = ( ( double ) d1 - radius ) / delta;
				if ( hitFrac > tw->trace.fraction ) {
					continue;
				}
				hit[0] = hitFrac * tw->delta[0] + tw->start[0];
				hit[1] = hitFrac * tw->delta[1] + tw->start[1];
				hit[2] = hitFrac * tw->delta[2] + tw->start[2];
			}

			/* e0 = ((x+y)+z) - d, stored as float; e1 = ((z+y)+x) - d, never
			 * rounded (0x004211A2-0x004211CC). */
			e0 = ( float ) ( ( ( hit[0] * facet->edgePlanes[0].normal[0]
								 + hit[1] * facet->edgePlanes[0].normal[1] )
							   + hit[2] * facet->edgePlanes[0].normal[2] )
							 - facet->edgePlanes[0].dist );
			e1u = ( ( hit[2] * facet->edgePlanes[1].normal[2]
					  + hit[1] * facet->edgePlanes[1].normal[1] )
					+ hit[0] * facet->edgePlanes[1].normal[0] )
				  - facet->edgePlanes[1].dist;

			outside = 0;
			if ( e0 + e1u > 1.0 ) {
				outside |= 1;
			}
			if ( e0 < 0.0f ) {
				outside |= 2;
			}
			if ( e1u < 0.0 ) {
				outside |= 4;
			}

			if ( !outside ) {
				VectorCopy( surf->normal, tw->trace.normal );
				/* Both the test and the back-off read the unrounded hitFrac
				 * (0x0042122F, 0x00421258). */
				if ( hitFrac <= TERRAIN_HIT_EPSILON ) {
					tw->trace.fraction = 0.0f;
					tw->trace.startsolid = 1;
					return;
				}
				tw->trace.fraction = ( float ) ( hitFrac - TERRAIN_HIT_EPSILON );
				continue;
			}

			for ( e = 0; e < 3; e++ ) {
				if ( ( outside >> e ) & 1 ) {
					terrainEdgeCylinder_t *cyl = facet->edgeCylinders[e];
					double dx, dy, dz, rdU;
					float radial0, radial1, axial, radiusDelta;

					if ( !cyl || cyl->checkcount == CM_CHECKCOUNT ) {
						continue;
					}
					cyl->checkcount = CM_CHECKCOUNT;

					/* d stays on the stack; radial0/1 sum (x+z)+y and axial
					 * (z+y)+x, each stored as float (0x004212A1-0x004212F1). */
					dx = ( double ) tw->start[0] - cyl->origin[0];
					dy = ( double ) tw->start[1] - cyl->origin[1];
					dz = ( double ) tw->start[2] - cyl->origin[2];
					radial0 = ( float ) ( ( dx * cyl->radialAxes[0][0]
											+ dz * cyl->radialAxes[0][2] )
										  + dy * cyl->radialAxes[0][1] );
					radial1 = ( float ) ( ( dx * cyl->radialAxes[1][0]
											+ dz * cyl->radialAxes[1][2] )
										  + dy * cyl->radialAxes[1][1] );
					axial = ( float ) ( ( dz * cyl->axis[2] + dy * cyl->axis[1] )
										+ dx * cyl->axis[0] );
					/* radiusDelta is stored with fst; the branch reads the
					 * unrounded value with `test ah,41h` + `jp`, so NaN takes
					 * the moving branch (0x004212F5-0x00421320). */
					rdU = ( ( double ) radial0 * radial0
							+ ( double ) radial1 * radial1 )
						  - ( double ) radius * radius;
					radiusDelta = ( float ) rdU;

					if ( !( rdU <= 0.0 ) ) {
						float vel0, vel1, vdot, frac;
						double vdotU, vlenSq, disc, fracU, axialU;

						vel0 = ( tw->delta[0] * cyl->radialAxes[0][0]
								 + tw->delta[1] * cyl->radialAxes[0][1] )
							   + tw->delta[2] * cyl->radialAxes[0][2];
						vel1 = ( tw->delta[0] * cyl->radialAxes[1][0]
								 + tw->delta[1] * cyl->radialAxes[1][1] )
							   + tw->delta[2] * cyl->radialAxes[1][2];
						/* vdot is stored with fst; the test reads the
						 * unrounded value and only an ordered >= 0 rejects
						 * (`test ah,1` + `jz`, 0x004213CE-0x004213DD). */
						vdotU = ( double ) vel0 * radial0
								+ ( double ) vel1 * radial1;
						vdot = ( float ) vdotU;
						if ( vdotU >= 0.0 ) {
							continue;
						}

						/* vlenSq, disc and the sqrt stay on the stack
						 * (0x004213E3-0x0042141E).  The disc test is retail's
						 * jnp on ordered less-or-equal, so NaN falls through. */
						vlenSq = ( double ) vel0 * vel0 + ( double ) vel1 * vel1;
						disc = ( double ) vdot * vdot - vlenSq * radiusDelta;
						if ( disc <= 0.0 ) {
							continue;
						}

						/* The fraction test reads the unrounded quotient;
						 * the float frac feeds everything after it
						 * (0x00421420). */
						fracU = ( -sqrt( disc ) - vdot ) / vlenSq;
						frac = ( float ) fracU;
						if ( !( fracU < tw->trace.fraction ) ) {
							continue;
						}

						/* axialAt = ((z+x)+y) * frac + axial, unrounded
						 * (0x00421435-0x00421453). */
						axialU = ( ( ( double ) cyl->axis[2] * tw->delta[2]
									 + ( double ) tw->delta[0] * cyl->axis[0] )
								   + ( double ) cyl->axis[1] * tw->delta[1] )
								 * frac + axial;
						if ( axialU < 0.0 || axialU > cyl->length ) {
							continue;
						}

						{
							/* 1/radius and n0 stay on the stack; only n1 is
							 * stored (0x00421472-0x004214B5). */
							double inv = 1.0 / radius;
							double n0 = ( ( double ) vel0 * frac + radial0 ) * inv;
							float n1 = ( float ) ( ( ( double ) vel1 * frac + radial1 )
												   * inv );

							tw->trace.normal[0] = ( float ) ( n0 * cyl->radialAxes[0][0] );
							tw->trace.normal[1] = ( float ) ( n0 * cyl->radialAxes[0][1] );
							tw->trace.normal[2] = ( float ) ( n0 * cyl->radialAxes[0][2] );
							tw->trace.normal[0] = n1 * cyl->radialAxes[1][0]
												  + tw->trace.normal[0];
							tw->trace.normal[1] = n1 * cyl->radialAxes[1][1]
												  + tw->trace.normal[1];
							tw->trace.normal[2] = n1 * cyl->radialAxes[1][2]
												  + tw->trace.normal[2];
						}

						if ( tw->trace.fraction <= TERRAIN_HIT_EPSILON ) {
							tw->trace.fraction = 0.0f;
							tw->trace.startsolid = 1;
							return;
						}
						/* Unlike the vertex path, the cylinder backs off the
						 * STORED float frac (0x0042150D). */
						tw->trace.fraction = frac - TERRAIN_HIT_EPSILON;
					} else if ( !( axial < 0.0f ) && !( axial > cyl->length ) ) {
						/* Both rejects are ordered (`jnp` / `jz`,
						 * 0x00421322-0x00421343), so NaN lands here. */
						VectorCopy( surf->normal, tw->trace.normal );
						tw->trace.fraction = 0.0f;
						tw->trace.startsolid = 1;
						return;
					}
				} else {
					terrainVertexSphere_t *vert = facet->vertexSpheres[e];
					vec3_t d;
					double dz, rd, vdotU, disc, fracU, inv;
					float radiusDelta, vdot;

					if ( !vert || vert->checkcount == CM_CHECKCOUNT ) {
						continue;
					}
					vert->checkcount = CM_CHECKCOUNT;

					/* d.x/d.y are stored floats; dz is stored with fst and
					 * the first product is dz_unrounded * dz_float, then
					 * + dy^2 + dx^2, tested before rounding
					 * (0x00421551-0x00421581). */
					d[0] = tw->start[0] - vert->origin[0];
					d[1] = tw->start[1] - vert->origin[1];
					dz = ( double ) tw->start[2] - vert->origin[2];
					d[2] = ( float ) dz;
					rd = ( ( dz * d[2] + ( double ) d[1] * d[1] )
						   + ( double ) d[0] * d[0] )
						 - ( double ) radius * radius;
					radiusDelta = ( float ) rd;
					if ( rd <= 0.0 ) {
						VectorCopy( surf->normal, tw->trace.normal );
						tw->trace.fraction = 0.0f;
						tw->trace.startsolid = 1;
						return;
					}

					/* vdot is stored with fst; the test reads the unrounded
					 * value and only an ordered >= 0 rejects (`test ah,1` +
					 * `jz`, 0x004215AB-0x004215BA). */
					vdotU = ( ( double ) tw->delta[0] * d[0]
							  + ( double ) tw->delta[1] * d[1] )
							+ ( double ) tw->delta[2] * d[2];
					vdot = ( float ) vdotU;
					if ( vdotU >= 0.0 ) {
						continue;
					}

					/* disc, the sqrt, frac and 1/radius all stay on the
					 * stack; the normal and the backed-off fraction use the
					 * unrounded frac (0x004215C0-0x00421649). */
					disc = ( double ) vdot * vdot
						   - ( double ) tw->deltaLengthSquared * radiusDelta;
					if ( disc < 0.0 ) {
						continue;
					}

					fracU = ( -sqrt( disc ) - vdot ) / tw->deltaLengthSquared;
					if ( !( fracU < tw->trace.fraction ) ) {
						continue;
					}

					inv = 1.0 / radius;
					tw->trace.normal[0] = ( float ) ( ( fracU * tw->delta[0] + d[0] ) * inv );
					tw->trace.normal[1] = ( float ) ( ( fracU * tw->delta[1] + d[1] ) * inv );
					tw->trace.normal[2] = ( float ) ( ( fracU * tw->delta[2] + d[2] ) * inv );

					if ( tw->trace.fraction <= TERRAIN_HIT_EPSILON ) {
						tw->trace.fraction = 0.0f;
						tw->trace.startsolid = 1;
						return;
					}
					tw->trace.fraction = ( float ) ( fracU - TERRAIN_HIT_EPSILON );
				}
			}
		}
	}
#endif
}

/* ---- CM_PositionTestSphereWithTerrainCollide  0x004216B0 ---- */
/*
 * Two bodies, as in the sphere walker: the COD1_VC7 one is retail's source
 * shape for VC7, the modern one spells out retail's x87 precision.
 */
qboolean __cdecl CM_PositionTestSphereWithTerrainCollide( traceWork_t *tw,
														  const terrainCollide_t *tc )
{
#ifdef COD1_VC7
	const terrainFacet_t *facets;
	const terrainEdgeCylinder_t *cyl;
	const terrainVertexSphere_t *vert;
	int i, e;
	unsigned int outside;
	rfloat capOffset, capDist, off, e0, distSq;
	float capDelta, radius, negRadius, surfDist, invNz, baseE0, baseE1, e1, along;
	vec3_t proj, d;

	capOffset = tw->sphere.halfheight - tw->sphere.radius;
	if ( tc->capsuleOffsetSign ) {
		capDelta = capOffset * -2.0f;
		tw->start[2] += capOffset;
		tw->end[2] += capOffset;
	} else {
		capDelta = capOffset + capOffset;
		tw->start[2] -= capOffset;
		tw->end[2] -= capOffset;
	}

	radius = tw->sphere.radius;
	negRadius = -radius;

	/* The planes are read through facets[] and the sphere/cylinder
	 * pointers through CM_TC_FACETS( tc ): that is what puts retail's
	 * strength-reduced pointer at +0x0C and its index at 0x0D. */
	facets = CM_TC_FACETS( tc );
	for ( i = 0; i < tc->facetCount; i++ ) {
		/* The parenthesis nesting is load-bearing: VC7 keeps paren nodes, and
		 * this grouping is the one that gives retail's operand order in all
		 * three products (normal[2] first, then start[1], start[0];
		 * 0x00421733-0x00421745). */
		surfDist = ( ( ( tw->start[2] ) * ( facets[i].surfacePlane.normal[2] ) ) + tw->start[1] * ( ( facets[i].surfacePlane.normal[1] ) ) ) + tw->start[0] * facets[i].surfacePlane.normal[0]
				   - facets[i].surfacePlane.dist;
		if ( surfDist >= radius ) {
			continue;
		}

		if ( surfDist <= negRadius ) {
			capDist = capDelta * facets[i].surfacePlane.normal[2] + surfDist;
			if ( capDist <= negRadius ) {
				continue;
			}
			invNz = 1.0f / facets[i].surfacePlane.normal[2];
			off = ( negRadius - surfDist ) * invNz;
			baseE0 = DotProduct( tw->start, facets[i].edgePlanes[0].normal )
					 - facets[i].edgePlanes[0].dist;
			baseE1 = DotProduct( tw->start, facets[i].edgePlanes[1].normal )
					 - facets[i].edgePlanes[1].dist;
			e0 = off * facets[i].edgePlanes[0].normal[2] + baseE0;
			if ( e0 >= 0.0f ) {
				e1 = off * facets[i].edgePlanes[1].normal[2] + baseE1;
				if ( e1 >= 0.0f && e0 + e1 <= 1.0f ) {
					return qtrue;
				}
			}
			if ( capDist < radius ) {
				off = capDelta;
			} else {
				off = ( radius - surfDist ) * invNz;
			}
			e0 = off * facets[i].edgePlanes[0].normal[2] + baseE0;
			if ( e0 < 0.0f ) {
				continue;
			}
			e1 = off * facets[i].edgePlanes[1].normal[2] + baseE1;
			if ( e1 < 0.0f ) {
				continue;
			}
			if ( e0 + e1 > 1.0f ) {
				continue;
			}
			return qtrue;
		}

		VectorMA( tw->start, -surfDist, facets[i].surfacePlane.normal, proj );
		e0 = ( proj[2] * facets[i].edgePlanes[0].normal[2]
			   + proj[1] * facets[i].edgePlanes[0].normal[1] )
			 + proj[0] * facets[i].edgePlanes[0].normal[0]
			 - facets[i].edgePlanes[0].dist;
		e1 = ( proj[2] * facets[i].edgePlanes[1].normal[2]
			   + proj[1] * facets[i].edgePlanes[1].normal[1] )
			 + proj[0] * facets[i].edgePlanes[1].normal[0]
			 - facets[i].edgePlanes[1].dist;
		outside = 0;
		outside |= ( e1 + e0 > 1.0f );
		outside |= ( e0 < 0.0f ) << 1;
		outside |= ( e1 < 0.0f ) << 2;
		if ( !outside ) {
			return qtrue;
		}

		for ( e = 0; e < 3; e++ ) {
			if ( outside & ( 1 << e ) ) {
				cyl = CM_TC_FACETS( tc )[i].edgeCylinders[e];
				if ( !cyl ) {
					continue;
				}
				VectorSubtract( tw->start, cyl->origin, d );
				along = ( d[2] * cyl->axis[2] + d[1] * cyl->axis[1] ) + d[0] * cyl->axis[0];
				if ( !( along >= 0.0f ) ) {
					continue;
				}
				if ( !( along <= cyl->length ) ) {
					continue;
				}
				VectorMA( d, -along, cyl->axis, d );
				distSq = ( d[2] * d[2] + d[1] * d[1] ) + d[0] * d[0];
				if ( distSq < tw->sphere.radius * tw->sphere.radius ) {
					return qtrue;
				}
			} else {
				vert = CM_TC_FACETS( tc )[i].vertexSpheres[e];
				if ( !vert ) {
					continue;
				}
				VectorSubtract( tw->start, vert->origin, d );
				distSq = ( d[2] * d[2] + d[1] * d[1] ) + d[0] * d[0];
				if ( distSq < tw->sphere.radius * tw->sphere.radius ) {
					return qtrue;
				}
			}
		}
	}

	return qfalse;
#else
	const terrainFacet_t *facets = CM_TC_FACETS( tc );
	int numFacets = ( int ) tc->facetCount;
	float radius = tw->sphere.radius;
	float negRadius = -radius;
	/* As in the sphere walker, every double local here is a value retail
	 * keeps on the x87 stack at _PC_53.  capOffset is never stored
	 * (0x004216B0-0x00421701). */
	double capOffset = ( double ) tw->sphere.halfheight - radius;
	float capDelta;
	int i;

	if ( tc->capsuleOffsetSign ) {
		capDelta = ( float ) ( capOffset * -2.0 );
		tw->start[2] = ( float ) ( capOffset + tw->start[2] );
		tw->end[2] = ( float ) ( capOffset + tw->end[2] );
	} else {
		capDelta = ( float ) ( capOffset + capOffset );
		tw->start[2] = ( float ) ( tw->start[2] - capOffset );
		tw->end[2] = ( float ) ( tw->end[2] - capOffset );
	}

	for ( i = 0; i < numFacets; i++ ) {
		const terrainFacet_t *facet = &facets[i];
		const terrainPlane_t *surf = &facet->surfacePlane;
		double sd;
		float surfDist;

		/* surfDist sums (z+y)+x and is stored with fst: the radius test
		 * reads the unrounded value, the -radius test the stored float
		 * (0x00421733-0x00421761). */
		sd = ( ( double ) tw->start[2] * surf->normal[2]
			   + ( double ) tw->start[1] * surf->normal[1] )
			 + ( double ) tw->start[0] * surf->normal[0] - surf->dist;
		surfDist = ( float ) sd;

		if ( sd >= radius ) {
			continue;
		}

		if ( surfDist <= negRadius ) {
			double capDist, enterOffset, leaveOffset, e0u, e1u;
			float invNormalZ, baseE0, baseE1, e0;

			/* capDist, both offsets and e1 stay on the stack; e0 is stored
			 * with fst and its float is what e0 + e1 adds
			 * (0x00421770-0x00421892). */
			capDist = ( double ) capDelta * surf->normal[2] + surfDist;
			if ( negRadius >= capDist ) {
				continue;
			}

			invNormalZ = 1.0f / surf->normal[2];
			enterOffset = ( ( double ) negRadius - surfDist ) * invNormalZ;

			baseE0 = ( tw->start[0] * facet->edgePlanes[0].normal[0]
					   + tw->start[1] * facet->edgePlanes[0].normal[1] )
					 + tw->start[2] * facet->edgePlanes[0].normal[2]
					 - facet->edgePlanes[0].dist;
			baseE1 = ( tw->start[0] * facet->edgePlanes[1].normal[0]
					   + tw->start[1] * facet->edgePlanes[1].normal[1] )
					 + tw->start[2] * facet->edgePlanes[1].normal[2]
					 - facet->edgePlanes[1].dist;

			e0u = enterOffset * facet->edgePlanes[0].normal[2] + baseE0;
			e0 = ( float ) e0u;
			if ( e0u >= 0.0 ) {
				e1u = enterOffset * facet->edgePlanes[1].normal[2] + baseE1;
				if ( e1u >= 0.0 && e0 + e1u <= 1.0 ) {
					return qtrue;
				}
			}

			if ( radius > capDist ) {
				leaveOffset = capDelta;
			} else {
				leaveOffset = ( ( double ) radius - surfDist ) * invNormalZ;
			}

			e0u = leaveOffset * facet->edgePlanes[0].normal[2] + baseE0;
			e0 = ( float ) e0u;
			if ( !( 0.0 > e0u ) ) {
				e1u = leaveOffset * facet->edgePlanes[1].normal[2] + baseE1;
				if ( !( 0.0 > e1u ) && !( e0 + e1u > 1.0 ) ) {
					return qtrue;
				}
			}
		} else {
			float px, py, pz, e1;
			double pzu, e0u, e1u;
			unsigned int outside;
			int e;

			/* proj.x/y are stored floats; proj.z is stored with fst and the
			 * unrounded value feeds e0 (0x00421899-0x004218BB).  e0 =
			 * (z+y)+x - d stays unrounded; e1 = (z+y)+x - d over the float
			 * proj is stored with fst.  The outside bits read e1u + e0u,
			 * e0u and the float e1 (0x004218F3-0x0042192E). */
			px = tw->start[0] - surfDist * surf->normal[0];
			py = tw->start[1] - surfDist * surf->normal[1];
			pzu = tw->start[2] - ( double ) surfDist * surf->normal[2];
			pz = ( float ) pzu;

			e0u = ( pzu * facet->edgePlanes[0].normal[2]
					+ ( double ) py * facet->edgePlanes[0].normal[1] )
				  + ( double ) px * facet->edgePlanes[0].normal[0]
				  - facet->edgePlanes[0].dist;
			e1u = ( ( double ) pz * facet->edgePlanes[1].normal[2]
					+ ( double ) py * facet->edgePlanes[1].normal[1] )
				  + ( double ) px * facet->edgePlanes[1].normal[0]
				  - facet->edgePlanes[1].dist;
			e1 = ( float ) e1u;

			outside = 0;
			if ( e1u + e0u > 1.0 ) {
				outside |= 1;
			}
			if ( e0u < 0.0 ) {
				outside |= 2;
			}
			if ( e1 < 0.0f ) {
				outside |= 4;
			}

			if ( !outside ) {
				return qtrue;
			}

			for ( e = 0; e < 3; e++ ) {
				double distSq;

				if ( ( outside >> e ) & 1 ) {
					const terrainEdgeCylinder_t *cyl = facet->edgeCylinders[e];
					float dx, dy, dz, along;
					double dzu, alongU, r0, r1, r2;

					if ( !cyl ) {
						continue;
					}

					/* dx/dy are stored floats, dz is stored with fst and the
					 * unrounded dz starts 'along' = (z+y)+x.  The low test
					 * reads the unrounded along (jnz on C0, so NaN skips),
					 * the length test the stored float (jp, so NaN skips)
					 * (0x0042197E-0x004219D1). */
					dx = tw->start[0] - cyl->origin[0];
					dy = tw->start[1] - cyl->origin[1];
					dzu = ( double ) tw->start[2] - cyl->origin[2];
					dz = ( float ) dzu;
					alongU = ( dzu * cyl->axis[2] + ( double ) dy * cyl->axis[1] )
							 + ( double ) dx * cyl->axis[0];
					along = ( float ) alongU;
					if ( !( alongU >= 0.0 ) || !( along <= cyl->length ) ) {
						continue;
					}

					/* The radial vector r = -along * axis + d stays on the
					 * stack and is squared (z+y)+x (0x004219D7-0x00421A12). */
					r0 = -along * ( double ) cyl->axis[0] + dx;
					r1 = -along * ( double ) cyl->axis[1] + dy;
					r2 = -along * ( double ) cyl->axis[2] + dz;
					distSq = ( r2 * r2 + r1 * r1 ) + r0 * r0;
				} else {
					const terrainVertexSphere_t *vert = facet->vertexSpheres[e];
					double dx, dy, dz;

					if ( !vert ) {
						continue;
					}

					/* d stays on the stack, squared (z+y)+x
					 * (0x00421A27-0x00421A52). */
					dx = ( double ) tw->start[0] - vert->origin[0];
					dy = ( double ) tw->start[1] - vert->origin[1];
					dz = ( double ) tw->start[2] - vert->origin[2];
					distSq = ( dz * dz + dy * dy ) + dx * dx;
				}

				if ( distSq < ( double ) radius * radius ) {
					return qtrue;
				}
			}
		}
	}

	return qfalse;
#endif
}

/* ---- CM_TraceSquareThroughTerrainCollide  0x00421AA0 ---- VERIFIED */
void __cdecl CM_TraceSquareThroughTerrainCollide(traceWork_t *tw, void *tc)
{
	float savedStartZ, savedEndZ, savedRadius;

	if ( !cm_boxOnTerrainWarned ) {
		cm_boxOnTerrainWarned = 1;
		Com_DPrintf( "^1Box collision on terrain currently being faked with capsule collision\n" );
	}

	savedStartZ = tw->start[2];
	savedEndZ = tw->end[2];
	savedRadius = tw->sphere.radius;

	tw->sphere.use = 1;
	tw->sphere.radius = tw->maxs[0];

	CM_TraceSphereThroughTerrainCollide( tw, tc );

	tw->sphere.use = 0;
	tw->start[2] = savedStartZ;
	tw->end[2] = savedEndZ;
	tw->sphere.radius = savedRadius;
}

/* ---- CM_TraceThroughTerrainCollide  0x00421B20 ---- VERIFIED */
void __cdecl CM_TraceThroughTerrainCollide(traceWork_t *tw, void *tc)
{
	if ( tw->isPoint ) {
		CM_TracePointThroughTerrainCollide( tw, tc );
		return;
	}

	if ( tw->sphere.use ) {
		float savedStartZ = tw->start[2];
		float savedEndZ = tw->end[2];

		CM_TraceSphereThroughTerrainCollide( tw, tc );

		tw->start[2] = savedStartZ;
		tw->end[2] = savedEndZ;
		return;
	}

	CM_TraceSquareThroughTerrainCollide( tw, tc );
}

/* ---- CM_SightTraceThroughTerrainCollide  0x00421B80 ---- VERIFIED */
qboolean __cdecl CM_SightTraceThroughTerrainCollide(traceWork_t *tw, void *tc)
{
	traceWork_t sightWork;

	sightWork = *tw;
	sightWork.trace.fraction = 1.0f;

	CM_TraceThroughTerrainCollide( &sightWork, tc );

	return sightWork.trace.fraction == 1.0f;
}

/* ---- CM_PositionTestInTerrainCollide  0x00421BE0 ---- VERIFIED */
qboolean __cdecl CM_PositionTestInTerrainCollide(traceWork_t *tw, void *tc)
{
	float savedStartZ, savedEndZ;
	qboolean result;

	if ( tw->isPoint ) {
		return qfalse;
	}

	savedStartZ = tw->start[2];
	savedEndZ = tw->end[2];

	result = CM_PositionTestSphereWithTerrainCollide( tw, tc );

	tw->start[2] = savedStartZ;
	tw->end[2] = savedEndZ;

	return result;
}
