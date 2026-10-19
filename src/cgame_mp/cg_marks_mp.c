// cg_marks.c -- wall marks
// The renderer builds the mark vertices and picks the shader for each
// fragment; every mark carries its own lifetime.

#include <string.h>

#include "cg_local.h"

// Ridah, increased this since we leave them around for longer
#define MAX_MARK_POINTS         1024
#define MAX_MARK_FRAGMENTS      384

/*
===================================================================

MARK POLYS

===================================================================
*/

markPoly_t cg_markPolys[MAX_MARK_POLYS];
markPoly_t cg_activeMarkPolys;          // double linked list
markPoly_t  *cg_freeMarkPolys;          // single linked list

/*
===================
CG_InitMarkPolys

This is called at startup and for tournement restarts
===================
*/
void CG_InitMarkPolys( void ) {
	int i;
	markPoly_t  *trav, *lasttrav;

	memset( cg_markPolys, 0, sizeof( cg_markPolys ) );

	cg_activeMarkPolys.nextMark = &cg_activeMarkPolys;
	cg_activeMarkPolys.prevMark = &cg_activeMarkPolys;
	cg_freeMarkPolys = cg_markPolys;
	for ( i = 0, trav = cg_markPolys + 1, lasttrav = cg_markPolys ; i < MAX_MARK_POLYS - 1 ; i++, trav++ ) {
		lasttrav->nextMark = trav;
		lasttrav = trav;
	}
}

/*
==================
CG_FreeMarkPoly
==================
*/
void CG_FreeMarkPoly( markPoly_t *le ) {
	if ( !le->prevMark ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	// remove from the doubly linked active list
	le->prevMark->nextMark = le->nextMark;
	le->nextMark->prevMark = le->prevMark;

	// the free list is only singly linked
	le->nextMark = cg_freeMarkPolys;
	cg_freeMarkPolys = le;
}

/*
===================
CG_AllocMark

Will allways succeed, even if it requires freeing an old active mark
===================
*/
markPoly_t *CG_AllocMark( int expireTime ) {
	markPoly_t  *le;
	int time;

	if ( !cg_freeMarkPolys ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		time = cg_activeMarkPolys.prevMark->time;
		while ( cg_activeMarkPolys.prevMark && time == cg_activeMarkPolys.prevMark->time ) {
			CG_FreeMarkPoly( cg_activeMarkPolys.prevMark );
		}
	}

	le = cg_freeMarkPolys;
	cg_freeMarkPolys = cg_freeMarkPolys->nextMark;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->nextMark = cg_activeMarkPolys.nextMark;
	le->prevMark = &cg_activeMarkPolys;
	cg_activeMarkPolys.nextMark->prevMark = le;
	cg_activeMarkPolys.nextMark = le;
	return le;
}

/*
=================
CG_ImpactMark

origin should be a point within a unit of the plane
dir should be the plane normal

temporary marks will not be stored or randomly oriented, but immediately
passed to the renderer.
=================
*/
void CG_ImpactMark( qhandle_t markShader, const vec3_t origin, const vec3_t dir,
					float orientation, float red, float green, float blue, float alpha,
					qboolean alphaFade, float radius, qboolean temporary, int duration ) {
	vec3_t          axis[3];
	vec3_t          originalPoints[4];
	byte            colors[4];
	int             i, j;
	int             numFragments;
	markFragment_t  markFragments[MAX_MARK_FRAGMENTS], *mf;
	polyVert_t      markPoints[MAX_MARK_POINTS];
	polyVert_t      *v;
	markPoly_t      *mark;

	if ( !cg_marks.integer ) {
		return;
	}

	// set by CG_EntityEvent around EV_ROCKET_EXPLODE_NOMARKS
	if ( cg.noMarks ) {
		return;
	}

	if ( duration < 0 ) {
		duration = 20000;
	}

	// create the texture axis
	VectorNormalize2( dir, axis[0] );
	PerpendicularVector( axis[1], axis[0] );
	RotatePointAroundVector( axis[2], axis[0], axis[1], orientation );
	CrossProduct( axis[0], axis[2], axis[1] );

	// create the full polygon
	for ( i = 0 ; i < 3 ; i++ ) {
		originalPoints[0][i] = origin[i] - radius * axis[1][i] - radius * axis[2][i];
		originalPoints[1][i] = origin[i] + radius * axis[1][i] - radius * axis[2][i];
		originalPoints[2][i] = origin[i] + radius * axis[1][i] + radius * axis[2][i];
		originalPoints[3][i] = origin[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	// get the fragments
	numFragments = trap_R_MarkFragments( 4, originalPoints, origin, axis[0],
										 radius, MAX_MARK_POINTS, markPoints,
										 MAX_MARK_FRAGMENTS, markFragments, markShader );

	colors[0] = (byte)( red * 255 );
	colors[1] = (byte)( green * 255 );
	colors[2] = (byte)( blue * 255 );
	colors[3] = (byte)( alpha * 255 );

	for ( i = 0, mf = markFragments ; i < numFragments ; i++, mf++ ) {
		// we have an upper limit on the complexity of polygons
		// that we store persistantly
		if ( mf->numPoints > MAX_VERTS_ON_POLY ) {
			mf->numPoints = MAX_VERTS_ON_POLY;
		}

		// the renderer already laid down xyz and st; only modulate is ours
		v = &markPoints[ mf->firstPoint ];
		for ( j = 0 ; j < mf->numPoints ; j++, v++ ) {
			*(int *)v->modulate = *(int *)colors;
		}

		// if it is a temporary (shadow) mark, add it immediately and forget about it
		if ( temporary ) {
			trap_R_AddPolyToScene( mf->markShader, mf->numPoints,
								   &markPoints[ mf->firstPoint ] );
			continue;
		}

		// otherwise save it persistantly
		mark = CG_AllocMark( cg.time + duration );
		mark->time = cg.time;
		mark->alphaFade = alphaFade;
		mark->markShader = mf->markShader;
		mark->numVerts = mf->numPoints;
		mark->color[0] = red;
		mark->color[1] = green;
		mark->color[2] = blue;
		mark->color[3] = alpha;
		mark->duration = duration;
		memcpy( mark->vertData, &markPoints[ mf->firstPoint ],
				mf->numPoints * sizeof( polyVert_t ) );
	}
}

/*
===============
CG_AddMarks
===============
*/
void CG_AddMarks( void ) {
	int         j;
	markPoly_t  *mp, *next;
	int         t;
	int         fade;

	if ( !cg_marks.integer ) {
		return;
	}

	mp = cg_activeMarkPolys.nextMark;
	for ( ; mp != &cg_activeMarkPolys ; mp = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = mp->nextMark;

		// see if it is time to completely remove it
		if ( cg.time > mp->time + mp->duration ) {
			CG_FreeMarkPoly( mp );
			continue;
		}

		// fade all marks out with time
		t = mp->time + mp->duration - cg.time;
		if ( t < FCAST( mp->duration ) / 2.0 ) {
			fade = (int)( 255.0 * FCAST( t ) / ( FCAST( mp->duration ) / 2.0 ) );
			if ( mp->alphaFade ) {
				for ( j = 0 ; j < mp->numVerts ; j++ ) {
					mp->vertData[j].modulate[3] = (byte)fade;
				}
			} else {
				for ( j = 0 ; j < mp->numVerts ; j++ ) {
					mp->vertData[j].modulate[0] = (byte)( fade * mp->color[0] );
					mp->vertData[j].modulate[1] = (byte)( fade * mp->color[1] );
					mp->vertData[j].modulate[2] = (byte)( fade * mp->color[2] );
				}
			}
		}

		trap_R_AddPolyToScene( mp->markShader, mp->numVerts, mp->vertData );
	}
}

/*
===============
CG_UnusedStub1
CG_UnusedStub2
CG_UnusedFloatIdentity

Unreferenced; the names are descriptive.
keep: not static, so they are still emitted like the original
===============
*/
void CG_UnusedStub1( void ) {
}

void CG_UnusedStub2( void ) {
}

float CG_UnusedFloatIdentity( float f ) {
	return f;
}
