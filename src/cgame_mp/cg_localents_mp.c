// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.
// Most of the old local entity types are now handled by the effects system.

#include <string.h>
#include <math.h>

#include "cg_local.h"
#include "../game_mp/bg_public.h"

// leType_t
#define LE_FADE_RGB             0
#define LE_SCALE_FADE           1
#define LE_MOVING_TRACER        2

// leFlags
#define LEF_PUFF_DONT_SCALE     0x0001

void        CG_Error( const char *msg, ... );
void        CG_DrawTracer( const vec3_t start, const vec3_t end );

localEntity_t cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t cg_activeLocalEntities;       // double linked list
localEntity_t   *cg_freeLocalEntities;      // single linked list

// Ridah, debugging
int localEntCount = 0;

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
void CG_InitLocalEntities( void ) {
	int i;

	memset( cg_localEntities, 0, sizeof( cg_localEntities ) );
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i + 1];
	}

	// Ridah, debugging
	localEntCount = 0;
}

/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity( localEntity_t *le ) {
	if ( !le->prev ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	// Ridah, debugging
	localEntCount--;

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t *CG_AllocLocalEntity( void ) {
	localEntity_t   *le;

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity( cg_activeLocalEntities.prev );
	}

	// Ridah, debugging
	localEntCount++;

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}

/*
================
CG_AddMovingTracer
================
*/
static void CG_AddMovingTracer( localEntity_t *le ) {
	vec3_t start, end, dir;

	BG_EvaluateTrajectory( &le->pos, cg.time, start );
	VectorNormalize2( le->pos.trDelta, dir );
	VectorMA( start, cg_tracerlength.value, dir, end );

	CG_DrawTracer( start, end );
}

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
CG_AddFadeRGB
====================
*/
static void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;
	c *= 0xff;

	re->shaderRGBA[0] = (byte)( le->color[0] * c );
	re->shaderRGBA[1] = (byte)( le->color[1] * c );
	re->shaderRGBA[2] = (byte)( le->color[2] * c );
	re->shaderRGBA[3] = (byte)( le->color[3] * c );

	trap_R_AddRefEntityToScene( re );
}

/*
===================
CG_AddScaleFade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void CG_AddScaleFade( localEntity_t *le ) {
	refEntity_t *re;
	rfloat c;
	vec3_t delta;
	rfloat len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA[3] = (byte)( 0xff * c * le->color[3] );
	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0f - c ) + 8;
	}

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	// keep: open-coded rather than VectorLength, matches original codegen
	len = sqrt( delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2] );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities( void ) {
	localEntity_t   *le, *next;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if ( cg.time >= le->endTime ) {
			CG_FreeLocalEntity( le );
			continue;
		}
		switch ( le->leType ) {
		default:
			CG_Error( "Bad leType: %i", le->leType );
			break;

		case LE_FADE_RGB:               // teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_SCALE_FADE:     // rocket trails
			CG_AddScaleFade( le );
			break;

		case LE_MOVING_TRACER:
			CG_AddMovingTracer( le );
			break;
		}
	}
}
