/*
 * @fidelity: verified
 *
 * tr_marks.c -- projection of decal/mark polygons onto the world surfaces
 * (RTCW's R_MarkFragments and its clipping helpers).  RE_MarkFragments gathers
 * the world surfaces overlapping the mark box, triangulates each, chops the
 * triangles against the projected polygon's bounding planes and writes the
 * surviving fragments (markFragment_t) plus their projected texcoords into the
 * caller's buffers.
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_shaderregistry.h"
#include "tr_local.h"


/* ------------------------------------------------------------------------- *
 *  File-local mark structs.
 *
 *  The surface handle (msurface_t), BSP node (mnode_t) and the SF_INDEXED_FIRST
 *  sentinel come from tr_local.h.  The types below are the mark unit's own view
 *  of the polymorphic data those handles point at -- the projected vertex, the
 *  fragment record, the world cell / AABB tree / cull group hierarchy and the
 *  indexed world surface -- and are defined here because they are specific to
 *  this unit.  Every offset touched by the decompiled code is pinned with an
 *  assert.
 * ------------------------------------------------------------------------- */

/* markVert_t (0x14) -- clip vertex: position + carried lightmap coords */
typedef struct {
	vec3_t	xyz;			/* +0x00 */
	float	lightmapCoords[2];	/* +0x0c  carried through the chop */
} markVert_t;
COD1_ASSERT_SIZE(markVert_t, 0x14);

/* polyVert_t (0x20) -- output vertex written to the caller's point buffer */
typedef struct {
	vec3_t	xyz;			/* +0x00 */
	float	st[2];			/* +0x0c  projected decal texcoords */
	float	lightmapCoords[2];	/* +0x14 */
	byte	modulate[4];		/* +0x1c  untouched here */
} polyVert_t;
COD1_ASSERT_SIZE(polyVert_t, 0x20);
ASSERT_OFF(polyVert_t, st, 0x0c);
ASSERT_OFF(polyVert_t, lightmapCoords, 0x14);

/* markFragment_t (0x0c) -- one returned fragment */
typedef struct {
	int	shaderHandle;		/* +0x00  fragmentShader->index */
	int	firstPoint;		/* +0x04  index into the point buffer */
	int	numPoints;		/* +0x08 */
} markFragment_t;
COD1_ASSERT_SIZE(markFragment_t, 0x0c);
ASSERT_OFF(markFragment_t, firstPoint, 0x04);
ASSERT_OFF(markFragment_t, numPoints, 0x08);

/* markShader_t -- the fields of shader_t the mark unit reads.  tr_local.h's
 * shader_t leaves surfaceParmFlags (0x5c) inside padding, so this partial view
 * exposes it (promotion candidate for shader_t). */
typedef struct {
	char		name[64];		/* +0x00 */
	int		lightmapIndex;		/* +0x40  < 0 => surface has no lightmap */
	int		index;			/* +0x44  -> markFragment_t::shaderHandle */
	int		sortedIndex;		/* +0x48 */
	unsigned int	flags;			/* +0x4c */
	unsigned int	surfaceFlags;		/* +0x50 */
	unsigned int	lightingFlags;		/* +0x54 */
	float		sort;			/* +0x58 */
	unsigned int	surfaceParmFlags;	/* +0x5c  bit 0x20 == nomarks */
} markShader_t;
ASSERT_OFF(markShader_t, lightmapIndex, 0x40);
ASSERT_OFF(markShader_t, index, 0x44);
ASSERT_OFF(markShader_t, surfaceParmFlags, 0x5c);

#define SURF_NOMARKS	0x20u

/* markSurface_t -- the indexed world surface an msurface_t::data points at when
 * surfaceType >= SF_INDEXED_FIRST.  Only the fields this unit reads are named;
 * the unread interior ranges are reserved so every named field lands on a
 * verified offset. */
typedef struct {
	int		surfaceType;		/* +0x00 */
	int		storageMode;		/* +0x04  unread here */
	unsigned int	dlightBits;		/* +0x08  unread here */
	vec3_t		boundsMin;		/* +0x0c */
	vec3_t		boundsMax;		/* +0x18 */
	int		numVerts;		/* +0x24  (tr_surface.c worldMeshSurf_t) */
	vec3_t		*tangents;		/* +0x28 */
	vec3_t		*bitangents;		/* +0x2c */
	vec3_t		*normals;		/* +0x30 */
	float		( *texCoords )[2];	/* +0x34 */
	float		( *lightmapCoords )[2];	/* +0x38 */
	unsigned int	*colors;		/* +0x3c */
	vec3_t		*positions;		/* +0x40 */
	int		indexCount;		/* +0x44 */
	unsigned short	indices[1];		/* +0x48  runs to indexCount */
} markSurface_t;
ASSERT_OFF(markSurface_t, boundsMin, 0x0c);
ASSERT_OFF(markSurface_t, boundsMax, 0x18);
ASSERT_OFF(markSurface_t, lightmapCoords, 0x38);
ASSERT_OFF(markSurface_t, positions, 0x40);
ASSERT_OFF(markSurface_t, indexCount, 0x44);
ASSERT_OFF(markSurface_t, indices, 0x48);

/* markCullGroup_t -- a DPVS cull group; holds a run of msurface_t handles */
typedef struct {
	vec3_t		mins;			/* +0x00 */
	vec3_t		maxs;			/* +0x0c */
	msurface_t	*surfaces;		/* +0x18  stride 0x0c */
	int		surfaceCount;		/* +0x1c */
	int		viewCount;		/* +0x20  DPVS only; unread here */
} markCullGroup_t;
ASSERT_OFF(markCullGroup_t, surfaces, 0x18);
ASSERT_OFF(markCullGroup_t, surfaceCount, 0x1c);

/* markAabbTree_t (0x28) -- per-cell surface AABB tree */
typedef struct markAabbTree_s markAabbTree_t;
struct markAabbTree_s {
	vec3_t		mins;			/* +0x00 */
	vec3_t		maxs;			/* +0x0c */
	msurface_t	*surfaces;		/* +0x18  stride 0x0c */
	int		surfaceCount;		/* +0x1c */
	markAabbTree_t	*children;		/* +0x20  stride 0x28 */
	int		childCount;		/* +0x24 */
};
COD1_ASSERT_SIZE(markAabbTree_t, 0x28);
ASSERT_OFF(markAabbTree_t, surfaces, 0x18);
ASSERT_OFF(markAabbTree_t, surfaceCount, 0x1c);
ASSERT_OFF(markAabbTree_t, children, 0x20);
ASSERT_OFF(markAabbTree_t, childCount, 0x24);

/* markCell_t (0x40) -- one world cell; addressed by mnode_t::cellIndex */
typedef struct {
	vec3_t			mins;		/* +0x00  unread here */
	vec3_t			maxs;		/* +0x0c  unread here */
	markAabbTree_t		*aabbTree;	/* +0x18 */
	void			*portals;	/* +0x1c  unread here */
	int			portalCount;	/* +0x20  unread here */
	markCullGroup_t		**cullGroups;	/* +0x24 */
	int			cullGroupCount;	/* +0x28 */
	void			**occluders;	/* +0x2c  unread here */
	int			occluderCount;	/* +0x30  unread here */
	int			markViewCount;	/* +0x34 */
	void			*modelLinks;	/* +0x38  unread here */
	void			*entityLinks;	/* +0x3c  unread here */
} markCell_t;
COD1_ASSERT_SIZE(markCell_t, 0x40);
ASSERT_OFF(markCell_t, aabbTree, 0x18);
ASSERT_OFF(markCell_t, cullGroups, 0x24);
ASSERT_OFF(markCell_t, cullGroupCount, 0x28);
ASSERT_OFF(markCell_t, markViewCount, 0x34);


/* tr.world blobs reached through the tr_world pointer (as in tr_world.c) */
#define TR_WORLD_NODES	(*(mnode_t   **)(tr_world + 152))   /* tr.world->nodes */
#define TR_WORLD_CELLS	(*(markCell_t **)(tr_world + 292))  /* tr.world->cells */

/* mnode_t::cellIndex (0x08) sentinels used to walk the tree in R_BoxSurfaces_r */
#define MNODE_INTERNAL	(-2)	/* internal (decision) node */
#define MNODE_NO_CELL	(-1)	/* leaf with no cell */

/* projection plane -- retail's bestnormal/bestdist at 0x011DBE40, touched only
 * by RE_MarkFragments (written and negated, never read), i.e. dead stores kept
 * faithfully.  No other function references them, so they live here. */
static struct {
	vec3_t	normal;
	float	dist;
} tr_markProjectionPlane;


#define MAX_VERTS_ON_POLY	64	/* 0x004E86BC guard cmp 3Eh (62); 0x004E9073 clamp cmp 40h */
#define MARK_CLIP_SIDES		2	/* clip ping-pong buffers */
#define MAX_MARK_SURFACES	4096	/* 0x004E9261: push 1000h to R_BoxSurfaces_r */
#define MARK_EXTRA_PLANES	2	/* near + far projection planes */
#define MARK_SHADER_USAGE	9	/* 0x004E936C: R_FindShader usage arg */

#define MARK_SIDE_FRONT	0
#define MARK_SIDE_BACK	1
#define MARK_SIDE_ON	2

#define BOX_SIDE_FRONT	1	/* BoxOnPlaneSide: box entirely in front of plane */
#define BOX_SIDE_BACK	2	/* box entirely behind plane */


extern int R_FindShader();
extern int BoxOnPlaneSide();


/* ---- R_ChopPolyBehindPlane  0x004E86A0 ----  VERIFIED
 * Clip inPoints against a plane, keeping the front side.  numOutPoints arrives
 * in EAX in retail (LTCG).  Clip vertices are five floats (xyz + lightmap st),
 * stride 20 -- not vec3_t as in Q3/RTCW. */
static void R_ChopPolyBehindPlane( int numInPoints, const markVert_t *inPoints, int *numOutPoints,
                                   markVert_t *outPoints, const vec3_t normal, float dist, float epsilon ) {
	float			dists[MAX_VERTS_ON_POLY + 4];
	int			sides[MAX_VERTS_ON_POLY + 4];
	int			counts[3];
	rfloat			dot;	/* x87 stack in retail: fcom'd unrounded, only fst'd to dists[] */
	int			i, j;
	const markVert_t	*p1, *p2;
	markVert_t		*clip;
	rfloat			d;

	/* don't clip if it might overflow  (0x004E86BC: cmp edi, 3Eh) */
	if ( numInPoints >= MAX_VERTS_ON_POLY - 2 ) {
		*numOutPoints = 0;
		return;
	}

	counts[MARK_SIDE_FRONT] = counts[MARK_SIDE_BACK] = counts[MARK_SIDE_ON] = 0;

	for ( i = 0 ; i < numInPoints ; i++ ) {
		dot = inPoints[i].xyz[2] * normal[2] + inPoints[i].xyz[0] * normal[0]
		    + normal[1] * inPoints[i].xyz[1];
		dot -= dist;
		dists[i] = dot;
		if ( dot > epsilon ) {
			sides[i] = MARK_SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = MARK_SIDE_BACK;
		} else {
			sides[i] = MARK_SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*numOutPoints = 0;

	if ( !counts[MARK_SIDE_FRONT] ) {
		return;
	}
	if ( !counts[MARK_SIDE_BACK] ) {
		*numOutPoints = numInPoints;
		memcpy( outPoints, inPoints, numInPoints * sizeof( *outPoints ) );
		return;
	}

	for ( i = 0 ; i < numInPoints ; i++ ) {
		p1 = &inPoints[i];
		clip = &outPoints[*numOutPoints];

		if ( sides[i] == MARK_SIDE_ON ) {
			clip->xyz[0] = p1->xyz[0];
			clip->xyz[1] = p1->xyz[1];
			clip->xyz[2] = p1->xyz[2];
			clip->lightmapCoords[0] = p1->lightmapCoords[0];
			clip->lightmapCoords[1] = p1->lightmapCoords[1];
			( *numOutPoints )++;
			continue;
		}

		if ( sides[i] == MARK_SIDE_FRONT ) {
			clip->xyz[0] = p1->xyz[0];
			clip->xyz[1] = p1->xyz[1];
			clip->xyz[2] = p1->xyz[2];
			clip->lightmapCoords[0] = p1->lightmapCoords[0];
			clip->lightmapCoords[1] = p1->lightmapCoords[1];
			( *numOutPoints )++;
			clip = &outPoints[*numOutPoints];
		}

		if ( sides[i + 1] == MARK_SIDE_ON || sides[i + 1] == sides[i] ) {
			continue;
		}

		/* generate a split point */
		p2 = &inPoints[( i + 1 ) % numInPoints];

		d = dists[i] - dists[i + 1];
		if ( d == 0 ) {
			dot = 0;
		} else {
			dot = dists[i] / d;
		}

		for ( j = 0 ; j < 3 ; j++ ) {
			clip->xyz[j] = (float)( p1->xyz[j] + dot * ( p2->xyz[j] - p1->xyz[j] ) );
		}
		for ( j = 0 ; j < 2 ; j++ ) {
			clip->lightmapCoords[j] = (float)( p1->lightmapCoords[j]
				+ dot * ( p2->lightmapCoords[j] - p1->lightmapCoords[j] ) );
		}
		( *numOutPoints )++;
	}
}


/* ---- R_AABBTreeSurfaces_r  0x004E8900 ----  VERIFIED
 * Recursively gather the mark surfaces of one cell's AABB tree that overlap the
 * mark box, skipping nomarks shaders and bounds-rejecting each surface. */
static void R_AABBTreeSurfaces_r( const markAabbTree_t *tree, const vec3_t mins, const vec3_t maxs,
                                  msurface_t **surfaces, int maxSurfaces, int *numSurfaces ) {
	int	i;

	if ( tree->maxs[0] < mins[0] || tree->mins[0] > maxs[0]
	  || tree->maxs[1] < mins[1] || tree->mins[1] > maxs[1]
	  || tree->maxs[2] < mins[2] || tree->mins[2] > maxs[2] ) {
		return;
	}

	if ( tree->childCount != 0 ) {
		for ( i = 0 ; i < tree->childCount ; i++ ) {
			R_AABBTreeSurfaces_r( &tree->children[i], mins, maxs,
			                      surfaces, maxSurfaces, numSurfaces );
		}
		return;
	}

	for ( i = 0 ; i < tree->surfaceCount ; i++ ) {
		const markSurface_t	*data;

		if ( *numSurfaces >= maxSurfaces ) {
			break;
		}

		/* indexed, no surf local: a msurface_t * local moves retail's registers */
		if ( ( ( (markShader_t *)tree->surfaces[i].shader )->surfaceParmFlags & SURF_NOMARKS ) != 0 ) {
			continue;
		}

		data = (const markSurface_t *)tree->surfaces[i].data;
		if ( data->boundsMax[0] < mins[0] || data->boundsMin[0] > maxs[0]
		  || data->boundsMax[1] < mins[1] || data->boundsMin[1] > maxs[1]
		  || data->boundsMax[2] < mins[2] || data->boundsMin[2] > maxs[2] ) {
			continue;
		}

		surfaces[( *numSurfaces )++] = &tree->surfaces[i];
	}
}


/* ---- R_CellSurfaces  0x004E8A80 ----  VERIFIED
 * Visit one world cell once per view: gather the overlapping mark surfaces of
 * its cull groups (with a linear duplicate check), then its AABB tree. */
static void R_CellSurfaces( markCell_t *cell, const vec3_t mins, const vec3_t maxs,
                            msurface_t **surfaces, int maxSurfaces, int *numSurfaces ) {
	int	i, j, k;

	if ( cell->markViewCount == tr_viewCount ) {
		return;
	}
	cell->markViewCount = tr_viewCount;

	for ( i = 0 ; i < cell->cullGroupCount ; i++ ) {
		const markCullGroup_t	*group = cell->cullGroups[i];

		if ( group->maxs[0] < mins[0] || group->mins[0] > maxs[0]
		  || group->maxs[1] < mins[1] || group->mins[1] > maxs[1]
		  || group->maxs[2] < mins[2] || group->mins[2] > maxs[2] ) {
			continue;
		}

		for ( j = 0 ; j < group->surfaceCount ; j++ ) {
			msurface_t		*surf;
			const markSurface_t	*data;

			if ( *numSurfaces >= maxSurfaces ) {
				break;
			}

			if ( ( ( (markShader_t *)group->surfaces[j].shader )->surfaceParmFlags & SURF_NOMARKS ) != 0 ) {
				continue;
			}

			surf = &group->surfaces[j];
			data = (const markSurface_t *)surf->data;
			if ( data->boundsMax[0] < mins[0] || data->boundsMin[0] > maxs[0]
			  || data->boundsMax[1] < mins[1] || data->boundsMin[1] > maxs[1]
			  || data->boundsMax[2] < mins[2] || data->boundsMin[2] > maxs[2] ) {
				continue;
			}

			for ( k = 0 ; k < *numSurfaces ; k++ ) {
				if ( surfaces[k] == surf ) {
					break;
				}
			}
			if ( k == *numSurfaces ) {
				surfaces[*numSurfaces] = surf;
				( *numSurfaces )++;
			}
		}
	}

	R_AABBTreeSurfaces_r( cell->aabbTree, mins, maxs, surfaces, maxSurfaces, numSurfaces );
}


/* ---- R_BoxSurfaces_r  0x004E8C50 ----  VERIFIED
 * Descend the BSP against the mark box (cellIndex == -2 marks a decision node),
 * then hand each reached cell to R_CellSurfaces. */
static void R_BoxSurfaces_r( mnode_t *node, const vec3_t mins, const vec3_t maxs,
                             msurface_t **surfaces, int maxSurfaces, int *numSurfaces ) {
	while ( node->cellIndex == MNODE_INTERNAL ) {
		int	s = BoxOnPlaneSide( mins, maxs, node->u.node.plane );

		if ( s == BOX_SIDE_FRONT ) {
			node = node->u.node.children[0];
			continue;
		}
		if ( s != BOX_SIDE_BACK ) {
			R_BoxSurfaces_r( node->u.node.children[0], mins, maxs,
			                 surfaces, maxSurfaces, numSurfaces );
		}
		node = node->u.node.children[1];
	}

	if ( node->cellIndex != MNODE_NO_CELL ) {
		R_CellSurfaces( &TR_WORLD_CELLS[node->cellIndex], mins, maxs,
		                surfaces, maxSurfaces, numSurfaces );
	}
}


/* ---- R_AddMarkFragment  0x004E8CF0 ----  VERIFIED
 * Chop one triangle against all the projection planes and, if anything
 * survives, copy the result into the point buffer as one fragment.  clipPoints
 * arrives in EBX in retail; the trailing mins/maxs stack args are dead
 * (inherited from the Q3 ancestor) and kept only for the 1:1 mapping. */
static qboolean R_AddMarkFragment( markVert_t clipPoints[MARK_CLIP_SIDES][MAX_VERTS_ON_POLY],
                                   int numClipPoints, int numPlanes,
                                   const vec3_t *normals, const float *dists,
                                   int maxPoints, polyVert_t *pointBuffer,
                                   markFragment_t *fragment,
                                   const vec3_t mins, const vec3_t maxs ) {
	int	pingPong = 0;
	int	i;

	(void)mins;
	(void)maxs;

	for ( i = 0 ; i < numPlanes ; i++ ) {
		R_ChopPolyBehindPlane( numClipPoints, clipPoints[pingPong], &numClipPoints,
		                       clipPoints[!pingPong], normals[i], dists[i], 0.5f );
		pingPong ^= 1;
		if ( numClipPoints == 0 ) {
			return qfalse;
		}
	}

	if ( numClipPoints > maxPoints ) {
		return qfalse;
	}

	fragment->numPoints = numClipPoints;
	for ( i = 0 ; i < numClipPoints ; i++ ) {
		const markVert_t	*src = &clipPoints[pingPong][i];
		polyVert_t		*dst = &pointBuffer[i];

		dst->xyz[0] = src->xyz[0];
		dst->xyz[1] = src->xyz[1];
		dst->xyz[2] = src->xyz[2];
		dst->lightmapCoords[0] = src->lightmapCoords[0];
		dst->lightmapCoords[1] = src->lightmapCoords[1];
	}

	return qtrue;
}


/* ---- RE_MarkFragments  0x004E8DB0 ----  VERIFIED
 * Project the polygon `points' (extruded along projectionAxis[0]) onto the
 * world surfaces it overlaps, returning the resulting fragments. */
int __cdecl RE_MarkFragments( int numPoints, const vec3_t *points,
                              const vec3_t projectionOrigin, const vec3_t projectionAxis[3],
                              float projectionRadius, int maxPoints, polyVert_t *pointBuffer,
                              int maxFragments, markFragment_t *fragmentBuffer, int hShader ) {
	msurface_t	*surfaces[MAX_MARK_SURFACES];
	markVert_t	clipPoints[MARK_CLIP_SIDES][MAX_VERTS_ON_POLY];
	vec3_t		normals[MAX_VERTS_ON_POLY + MARK_EXTRA_PLANES];
	float		dists[MAX_VERTS_ON_POLY + MARK_EXTRA_PLANES];
	vec3_t		mins, maxs;
	int		numsurfaces;
	int		numPlanes;
	int		returnedPoints;
	int		returnedFragments;
	markShader_t	*shader;
	int		i, j, k;

	tr_viewCount++;

	for ( i = 0 ; i < numPoints ; i++ ) {
		vec3_t	temp;
		VectorCopy( points[i], temp );
	}

	tr_markProjectionPlane.normal[0] = -projectionAxis[0][0];
	tr_markProjectionPlane.normal[1] = -projectionAxis[0][1];
	tr_markProjectionPlane.normal[2] = -projectionAxis[0][2];
	tr_markProjectionPlane.dist = 0.0f;

	ClearBounds( mins, maxs );
	for ( i = 0 ; i < numPoints ; i++ ) {
		vec3_t	temp;

		AddPointToBounds( points[i], mins, maxs );
		VectorMA( points[i], projectionRadius, projectionAxis[0], temp );
		AddPointToBounds( temp, mins, maxs );
		VectorMA( points[i], projectionRadius * -20.0f, projectionAxis[0], temp );
		AddPointToBounds( temp, mins, maxs );
	}

	if ( numPoints > MAX_VERTS_ON_POLY ) {
		numPoints = MAX_VERTS_ON_POLY;
	}

	/* one bounding plane per polygon edge */
	for ( i = 0 ; i < numPoints ; i++ ) {
		vec3_t	edge;

		VectorSubtract( points[( i + 1 ) % numPoints], points[i], edge );

		normals[i][0] = edge[1] * projectionAxis[0][2] - edge[2] * projectionAxis[0][1];
		normals[i][1] = edge[2] * projectionAxis[0][0] - edge[0] * projectionAxis[0][2];
		normals[i][2] = edge[0] * projectionAxis[0][1] - edge[1] * projectionAxis[0][0];
		VectorNormalize( normals[i] );

		dists[i] = normals[i][1] * points[i][1]
		         + normals[i][2] * points[i][2]
		         + normals[i][0] * points[i][0];
	}

	/* near and far projection planes */
	VectorCopy( projectionAxis[0], normals[numPoints] );
	dists[numPoints] = normals[numPoints][2] * points[0][2]
	                 + normals[numPoints][1] * points[0][1]
	                 + normals[numPoints][0] * points[0][0]
	                 - projectionRadius;
	VectorCopy( projectionAxis[0], normals[numPoints + 1] );
	normals[numPoints + 1][0] = -normals[numPoints + 1][0];
	normals[numPoints + 1][1] = -normals[numPoints + 1][1];
	normals[numPoints + 1][2] = -normals[numPoints + 1][2];
	dists[numPoints + 1] = normals[numPoints + 1][0] * points[0][0]
	                     + normals[numPoints + 1][1] * points[0][1]
	                     + normals[numPoints + 1][2] * points[0][2]
	                     - projectionRadius;

	numPlanes = numPoints + MARK_EXTRA_PLANES;

	numsurfaces = 0;
	R_BoxSurfaces_r( TR_WORLD_NODES, mins, maxs, surfaces, MAX_MARK_SURFACES, &numsurfaces );

	tr_markProjectionPlane.normal[0] = -tr_markProjectionPlane.normal[0];
	tr_markProjectionPlane.normal[1] = -tr_markProjectionPlane.normal[1];
	tr_markProjectionPlane.normal[2] = -tr_markProjectionPlane.normal[2];

	returnedPoints = 0;
	returnedFragments = 0;

	if ( hShader < 0 || hShader >= tr_numShaders[0] ) {
		ri.Printf( 2, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		shader = (markShader_t *)tr_defaultShader;
	} else {
		shader = (markShader_t *)tr_shaders[hShader];
	}

	for ( i = 0 ; i < numsurfaces ; i++ ) {
		markSurface_t	*surf;
		markShader_t	*fragmentShader;
		vec3_t		*positions;
		float		( *lightmapCoords )[2];
		int		baseIndex;

		surf = (markSurface_t *)surfaces[i]->data;

		if ( surf->surfaceType < SF_INDEXED_FIRST ) {
			continue;
		}

		/* the indices are absolute; rebase the vertex arrays through indices[0] */
		baseIndex = surf->indices[0];
		positions = surf->positions - baseIndex;
		lightmapCoords = surf->lightmapCoords - baseIndex;

		if ( shader != (markShader_t *)tr_defaultShader
		  && ( (markShader_t *)surfaces[i]->shader )->lightmapIndex >= 0 ) {
			fragmentShader = (markShader_t *)R_FindShader( shader->name,
			                                               ( (markShader_t *)surfaces[i]->shader )->lightmapIndex,
			                                               1, (const char *)MARK_SHADER_USAGE );
		} else {
			fragmentShader = shader;
		}

		for ( k = 0 ; k < surf->indexCount ; k += 3 ) {
			const unsigned short	*tri = &surf->indices[k];
			vec3_t			v1, v2, normal;
			markFragment_t		*mf;

			VectorCopy( positions[tri[0]], clipPoints[0][0].xyz );
			VectorCopy( positions[tri[1]], clipPoints[0][1].xyz );
			VectorCopy( positions[tri[2]], clipPoints[0][2].xyz );

			for ( j = 0 ; j < 3 ; j++ ) {
				v1[j] = clipPoints[0][0].xyz[j] - clipPoints[0][1].xyz[j];
				v2[j] = clipPoints[0][2].xyz[j] - clipPoints[0][1].xyz[j];
			}
			normal[0] = v2[2] * v1[1] - v2[1] * v1[2];
			normal[1] = v2[0] * v1[2] - v2[2] * v1[0];
			normal[2] = v2[1] * v1[0] - v2[0] * v1[1];
			VectorNormalize( normal );

			/* keep only triangles facing along the projection axis */
			if ( !( normal[0] * projectionAxis[0][0]
			      + normal[2] * projectionAxis[0][2]
			      + normal[1] * projectionAxis[0][1] > 0.5f ) ) {
				continue;
			}

			clipPoints[0][0].lightmapCoords[0] = lightmapCoords[tri[0]][0];
			clipPoints[0][0].lightmapCoords[1] = lightmapCoords[tri[0]][1];
			clipPoints[0][1].lightmapCoords[0] = lightmapCoords[tri[1]][0];
			clipPoints[0][1].lightmapCoords[1] = lightmapCoords[tri[1]][1];
			clipPoints[0][2].lightmapCoords[0] = lightmapCoords[tri[2]][0];
			clipPoints[0][2].lightmapCoords[1] = lightmapCoords[tri[2]][1];

			mf = &fragmentBuffer[returnedFragments];
			if ( !R_AddMarkFragment( clipPoints, 3, numPlanes, normals, dists,
			                         maxPoints - returnedPoints,
			                         &pointBuffer[returnedPoints], mf,
			                         mins, maxs ) ) {
				continue;
			}

			if ( mf->numPoints > 0 ) {
				float	scale = 0.5f / projectionRadius;

				for ( j = 0 ; j < mf->numPoints ; j++ ) {
					polyVert_t	*out = &pointBuffer[returnedPoints + j];
					vec3_t		delta;

					delta[0] = out->xyz[0] - projectionOrigin[0];
					delta[1] = out->xyz[1] - projectionOrigin[1];
					delta[2] = out->xyz[2] - projectionOrigin[2];

					out->st[0] = ( delta[0] * projectionAxis[1][0]
					             + delta[2] * projectionAxis[1][2]
					             + delta[1] * projectionAxis[1][1] ) * scale + 0.5f;
					out->st[1] = ( delta[2] * projectionAxis[2][2]
					             + delta[1] * projectionAxis[2][1]
					             + delta[0] * projectionAxis[2][0] ) * scale + 0.5f;
				}
			}

			mf->firstPoint = returnedPoints;
			mf->shaderHandle = fragmentShader->index;
			returnedPoints += mf->numPoints;
			returnedFragments++;

			if ( returnedFragments == maxFragments
			  || returnedPoints > maxPoints - 3 ) {
				return returnedFragments;
			}
		}
	}

	return returnedFragments;
}
