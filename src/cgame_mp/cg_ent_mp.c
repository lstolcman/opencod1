// cg_ents.c -- present snapshot entities, happens every single frame
// Models are drawn as DObjs (XModel + XAnim); the DObj tag and pose helpers
// used by the renderer and the weapon code live here as well.

#include "cg_local.h"

#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

// bg_misc.c
void            BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap );

// cg_main.c / cg_servercmds.c
void QDECL     CG_Printf( const char *msg, ... );
void QDECL     CG_Error( const char *msg, ... );
const char     *CG_ConfigString( int index );

// cg_players.c
void            CG_Player( centity_t *cent );
void            CG_Corpse( centity_t *cent );

// cg_weapons.c
void            CG_RegisterItemVisuals( int itemNum );

// com_math.c
vec_t           DistanceSquared( const vec3_t p1, const vec3_t p2 );
void            QuatFromPitch_m( float q[4], float degrees );
void            QuatFromYaw_m( float q[4], float degrees );
void            QuatFromRoll_m( float degrees, float q[4] );

/*
=============================================================================

	LOCAL TYPES

=============================================================================
*/

#define RT_XMODEL                   0
#define RT_DOBJ                     1
#define RT_BEAM                     6
#define RT_PORTALSURFACE            11

#define RF_NOSHADOW                 0x40
#define RF_LIGHTING_ORIGIN          0x80

// cg_items[] -- per-item visuals, owned by cg_weapons.c
typedef struct cgItemInfo_t {
	qboolean        registered;
	void            *models[MAX_ITEM_MODELS];
	qhandle_t       icons[MAX_ITEM_ICONS];
	const char      *pickupSound;
	const char      *ammoPickupSound;
} cgItemInfo_t;
CG_ASSERT_SIZE( cgItemInfo_t, 36 );

extern cgItemInfo_t cg_items[];

// one bone of the DObj pose, as trap_DObjGetRotTransArray returns it
typedef struct DObjAnimMat_t {
	vec4_t          quat;
	float           accumulatedWeight;
	vec3_t          translation;
} DObjAnimMat_t;
CG_ASSERT_SIZE( DObjAnimMat_t, 32 );

// entityType_t
#define ET_GENERAL              0
#define ET_PLAYER               1
#define ET_CORPSE               2
#define ET_ITEM                 3
#define ET_MISSILE              4
#define ET_MOVER                5
#define ET_PORTAL               6
#define ET_INVISIBLE            7
#define ET_SCRIPTMOVER          8
#define ET_SOUND_BLEND          9
#define ET_FX                   10
#define ET_MG42                 11
#define ET_EVENTS               12

#define EF_NODRAW               0x00000100
#define EF_MG42_FIRING          0x00000400
#define EF_LOCK_LIGHTING        0x00010000

#define PMF_VIEWLOCKED          0x00050000
#define PSEF_VIEWLOCKED         0x0000C000

#define TR_INTERPOLATE          1
#define TR_LINEAR_STOP          3

#define SOLID_BMODEL            0xFFFFFF

#define CS_SOUNDALIAS           524

#define ENTITYNUM_MAX_NORMAL    ( MAX_GENTITIES - 2 )

#define BG_NUM_ITEMS            70

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
==================
CG_LockLightingOrigin

EF_LOCK_LIGHTING freezes the point the model is lit from at the position it
first had, so a mover does not swim through the lightgrid.  CoD's replacement
for RTCW's per-entity lightingOrigin bookkeeping.
==================
*/
static void CG_LockLightingOrigin( centity_t *cent, refEntity_t *ent ) {
	float       *lightingOrigin;

	lightingOrigin = cent->lightingOrigin;

	if ( cent->currentState.eFlags & EF_LOCK_LIGHTING ) {
		if ( lightingOrigin[0] == vec3_origin[0]
			 && lightingOrigin[1] == vec3_origin[1]
			 && lightingOrigin[2] == vec3_origin[2] ) {
			VectorCopy( cent->lerpOrigin, lightingOrigin );
		}

		VectorCopy( lightingOrigin, ent->lightingOrigin );
		ent->renderfx |= RF_LIGHTING_ORIGIN;
	} else {
		lightingOrigin[2] = 0;
		lightingOrigin[1] = 0;
		lightingOrigin[0] = 0;
	}
}

/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent ) {
	refEntity_t     ent;
	entityState_t   *s1;
	DObj            *obj;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if ( s1->eFlags & EF_NODRAW ) {
		return;
	}

	CG_PreProcess_GetDObj( s1->number, s1->eType, cgs.gameModels[ s1->index ] );

	obj = trap_GetClientDObj( s1->number );
	if ( !obj ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.dobj = obj;
	ent.owner = cent;
	ent.reType = RT_DOBJ;

	CG_LockLightingOrigin( cent, &ent );

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
==================
CG_PlayerSeesItem

Nothing calls this any more.
==================
*/
qboolean CG_PlayerSeesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t      vorigin, eorigin, viewa, dir;
	float       dot, dist, foo;

	BG_EvaluateTrajectory( &item->pos, atTime, eorigin );

	VectorCopy( ps->origin, vorigin );
	vorigin[2] += ps->viewHeightCurrent;

	VectorSubtract( vorigin, eorigin, dir );

	dist = VectorNormalize( dir );

	if ( dist > 255 ) {
		return qfalse;
	}

	AngleVectors( ps->viewangles, viewa, NULL, NULL );

	dot = DotProduct( viewa, dir );

	foo = -0.94f - ( dist * ( 1.0f / 255.0f ) ) * 0.057f;

	if ( dot > foo ) {
		return qfalse;
	}

	return qtrue;
}

/*
==================
CG_Item
==================
*/
static void CG_Item( centity_t *cent ) {
	refEntity_t     ent;
	entityState_t   *es;
	gitem_t         *item;
	cgItemInfo_t    *itemInfo;
	DObj            *obj;

	es = &cent->currentState;

	// (item index is stored in es->index for item)
	if ( es->index >= BG_NUM_ITEMS ) {
		CG_Error( "Bad item index %i on entity", es->index );
	}

	// if set to invisible, skip
	if ( es->eFlags & EF_NODRAW ) {
		return;
	}

	item = &bg_itemlist[ es->index ];
	itemInfo = &cg_items[ es->index ];

	if ( !itemInfo->registered ) {
		CG_RegisterItemVisuals( es->index );
		return;
	}

	if ( !itemInfo->models[0] ) {
		CG_Error( "No XModel loaded for item index %i (%s)", es->index, item->pickup_name );
	}

	CG_PreProcess_GetDObj( es->number, es->eType, (qhandle_t)itemInfo->models[0] );

	obj = trap_GetClientDObj( es->number );
	if ( !obj ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	if ( item->giType == IT_WEAPON ) {
		AnglesToAxis( cent->lerpAngles, ent.axis );
		ent.nonNormalizedAxes = 1.0f;
	} else {
		AnglesToAxis( cent->lerpAngles, ent.axis );
	}

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	ent.dobj = obj;
	ent.owner = cent;
	ent.reType = RT_DOBJ;

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent ) {
	// add loop sound
	if ( cent->currentState.loopSound ) {
		vec3_t origin;
		const float *soundOrigin;
		const char *soundName;
		int soundNumber;
		if ( cent->currentState.solid == SOLID_BMODEL ) {
			float *v;
			v = cgs.inlineModelMidpoints[ cent->currentState.index ];
			VectorAdd( cent->lerpOrigin, v, origin );
			soundName = CG_ConfigString( CS_SOUNDALIAS + cent->currentState.loopSound );
			soundOrigin = origin;
			soundNumber = cent->currentState.number;
		} else {
			soundName = CG_ConfigString( CS_SOUNDALIAS + cent->currentState.loopSound );
			soundOrigin = cent->lerpOrigin;
			soundNumber = cent->currentState.number;
		}
		CG_PlaySoundAliasByName( soundName, soundNumber, soundOrigin );
	}

	// constant light glow
	if ( cent->currentState.constantLight ) {
		int cl;
		int i, r, g, b;

		cl = cent->currentState.constantLight;
		r = cl & 255;
		g = ( cl >> 8 ) & 255;
		b = ( cl >> 16 ) & 255;
		i = ( ( cl >> 24 ) & 255 ) * 4;

		trap_R_AddLightToScene( cent->lerpOrigin, (float)i,
								(float)r * ( 1.0f / 255.0f ), (float)g * ( 1.0f / 255.0f ), (float)b * ( 1.0f / 255.0f ) );
	}
}

/*
==================
CG_GetMG42Anims

Build the two-animation tree an mg42 barrel plays: the weapon's idle anim
under child 1, its fire anim under child 2.
==================
*/
static void *CG_GetMG42Anims( centity_t *cent ) {
	weaponInfo_t    *weapInfo;
	void            *anims;

	weapInfo = bg_weaponInfo[ cent->currentState.weapon ];

	anims = trap_XAnimCreateAnims( "MG42", 3 );
	trap_XAnimBlend( anims, 0, "root", 1, 2, 0 );

	trap_XAnimPrecache( weapInfo->idleAnim );
	trap_XAnimCreate( anims, 1, weapInfo->idleAnim );

	trap_XAnimPrecache( weapInfo->fireAnim );
	trap_XAnimCreate( anims, 2, weapInfo->fireAnim );

	return anims;
}

/*
==================
CG_mg42_DoControllers

Aim the barrel: angles2[0]/[1] drive tag_aim and tag_aim_animated, angles2[2]
drives tag_flash, and the anim tree is knobbed to idle or fire.
==================
*/
static void CG_mg42_DoControllers( centity_t *cent, int *partBits ) {
	DObj        *obj;
	vec3_t      angles;
	void        *tree;
	union { int packed; scr_anim_t anim; } childAnim;    // keep: matches original codegen
	scr_anim_t  anim;

	obj = trap_GetClientDObj( cent->currentState.number );

	angles[0] = LerpAngle( cent->currentState.angles2[0], cent->nextState.angles2[0],
						   cg.frameInterpolation );
	angles[1] = LerpAngle( cent->currentState.angles2[1], cent->nextState.angles2[1],
						   cg.frameInterpolation );
	angles[2] = 0;

	CG_DObjSetControlTagAngles( obj, partBits, "tag_aim", angles );

	CG_DObjSetControlTagAngles( obj, partBits, "tag_aim_animated", angles );

	angles[0] = LerpAngle( cent->currentState.angles2[2], cent->nextState.angles2[2],
						   cg.frameInterpolation );
	angles[1] = 0;

	CG_DObjSetControlTagAngles( obj, partBits, "tag_flash", angles );

	tree = trap_DObjGetTree( obj );

	// the local player riding this gun sees it idle even while firing
	if ( ( cg.predictedPlayerState.eFlags & PSEF_VIEWLOCKED )
		 && cg.predictedPlayerState.viewlocked_entNum == cent->currentState.number
		 && !cg.renderingThirdPerson ) {
		anim.index = 1;
	} else {
		anim.index = ( cent->currentState.eFlags & EF_MG42_FIRING ) ? 2 : 1;
	}

	childAnim.packed = 0;
	trap_XAnimSetCompleteGoalWeightKnobAll( tree, anim, childAnim.anim, 1.0f, 0.1f, 1.0f, 0, 0 );
}

/*
==================
CG_mg42
==================
*/
static void CG_mg42( centity_t *cent ) {
	refEntity_t     ent;
	entityState_t   *s1;
	DObj            *obj;

	s1 = &cent->currentState;

	if ( s1->eFlags & EF_NODRAW ) {
		return;
	}

	CG_PreProcess_GetDObj( s1->number, s1->eType, cgs.gameModels[ s1->index ] );

	obj = trap_GetClientDObj( s1->number );
	if ( !obj ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	// light the gun from head height rather than from the floor it sits on
	ent.lightingOrigin[0] = ent.origin[0];
	ent.lightingOrigin[1] = ent.origin[1];
	ent.lightingOrigin[2] = ent.origin[2] + 32.0f;
	ent.renderfx = RF_LIGHTING_ORIGIN;

	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.dobj = obj;
	ent.owner = cent;
	ent.reType = RT_DOBJ;

	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t             ent;
	entityState_t           *s1;
	cgWeaponInfo_t          *weapon;
	DObj                    *obj;
	int                     bolt[2];

	s1 = &cent->currentState;

	if ( s1->eFlags & EF_NODRAW ) {
		return;
	}

	if ( s1->weapon > bg_numWeapons ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[ s1->weapon ];

	// add missile sound
	if ( weapon->projectileSound ) {
		CG_PlaySoundAliasByName( weapon->projectileSound, s1->number, cent->lerpOrigin );
	}

	CG_PreProcess_GetDObj( s1->number, s1->eType, weapon->missileModel );

	obj = trap_GetClientDObj( s1->number );
	if ( !obj ) {
		return;
	}

	// add dynamic light
	if ( weapon->missileDlight != 0 ) {
		trap_R_AddLightToScene( cent->lerpOrigin, weapon->missileDlight,
								weapon->missileDlightColor[0], weapon->missileDlightColor[1],
								weapon->missileDlightColor[2] );
	}

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.dobj = obj;
	ent.owner = cent;
	ent.reType = RT_DOBJ;

	trap_R_AddRefEntityToScene( &ent );

	// add trails -- the effect is bolted to tag_origin once and then runs itself
	if ( weapon->missileTrailEffect && !cent->bTrailMade ) {
		bolt[0] = s1->number;
		bolt[1] = trap_FX_GetBoneIndex( s1->number, "tag_origin" );
		if ( bolt[1] >= 0 ) {
			trap_FX_PlayOrientedEffectID( weapon->missileTrailEffect, cent->lerpOrigin, NULL, bolt );
		}
		cent->bTrailMade = 1;
	}
}

/*
===============
CG_Mover
===============
*/
static void CG_Mover( centity_t *cent ) {
	refEntity_t     ent;
	entityState_t   *s1;
	DObj            *obj;

	s1 = &cent->currentState;

	if ( s1->eFlags & EF_NODRAW ) {
		return;
	}

	obj = trap_GetClientDObj( s1->number );
	if ( s1->solid != SOLID_BMODEL && !obj ) {
		return;
	}

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// get the model, either as a bmodel or a DObj (keep: an add call in each arm)
	if ( s1->solid != SOLID_BMODEL ) {
		ent.dobj = obj;
		ent.owner = cent;
		ent.reType = RT_DOBJ;
		trap_R_AddRefEntityToScene( &ent );
	} else {
		ent.hModel = cgs.inlineDrawModel[ s1->index ];
		ent.reType = RT_XMODEL;
		trap_R_AddRefEntityToScene( &ent );
	}
}

/*
===============
CG_ScriptMover

CG_Mover with a DObj built for it and its lighting origin locked -- the
script-driven movers carry XModels, not bmodels, most of the time.
===============
*/
static void CG_ScriptMover( centity_t *cent ) {
	refEntity_t     ent;
	entityState_t   *s1;
	DObj            *obj;

	s1 = &cent->currentState;

	if ( s1->eFlags & EF_NODRAW ) {
		return;
	}

	CG_PreProcess_GetDObj( s1->number, s1->eType, cgs.gameModels[ s1->index ] );

	obj = trap_GetClientDObj( s1->number );
	if ( s1->solid != SOLID_BMODEL && !obj ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	if ( s1->solid != SOLID_BMODEL ) {
		ent.dobj = obj;
		ent.owner = cent;
		ent.reType = RT_DOBJ;
		CG_LockLightingOrigin( cent, &ent );
		trap_R_AddRefEntityToScene( &ent );
	} else {
		ent.hModel = cgs.inlineDrawModel[ s1->index ];
		ent.reType = RT_XMODEL;
		trap_R_AddRefEntityToScene( &ent );
	}
}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent ) {
	refEntity_t     ent;
	entityState_t   *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );

	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent ) {
	refEntity_t     ent;
	entityState_t   *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[0] );
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.frame = 0;                                   // rotation speed
	ent.rotation = s1->angles2[2];                   // roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime,
								vec3_t out, vec3_t outDeltaAngles ) {
	centity_t   *cent;
	vec3_t      oldOrigin, origin, deltaOrigin;
	vec3_t      oldAngles, angles, deltaAngles;

	if ( outDeltaAngles ) {
		VectorClear( outDeltaAngles );
	}

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];

	if ( cent->currentState.eType != ET_MOVER && cent->currentState.eType != ET_SCRIPTMOVER ) {
		VectorCopy( in, out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );
	if ( outDeltaAngles ) {
		VectorCopy( deltaAngles, outDeltaAngles );
	}
}

/*
===============
CG_AddCEntity

===============
*/
void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// call the appropriate function which will add this entity to the view accordingly
	CG_ProcessEntity( cent );
}

/*
===============
CG_SetFrameInterpolation
===============
*/
void CG_SetFrameInterpolation( void ) {
	int delta;

	delta = cg.nextSnap->serverTime - cg.snap->serverTime;
	if ( delta == 0 ) {
		cg.frameInterpolation = 0;
	} else {
		cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		if ( cg.frameInterpolation < 0 ) {
			cg.frameInterpolation = 0;
		}
	}
}

/*
===============
CG_ProcessClientNoteTracks

The animation system hands back the note tracks that fired this frame; the
only ones cgame acts on are the two that move the weapon between hands.
===============
*/
static void CG_ProcessClientNoteTracks( int clientNum ) {
	// the animation system's deferred notify record
	struct noteTrack_s {
		const char      *name;
		unsigned short  notifyType;
		short           pad_0x06;
		float           timeFrac;
	} *notes;
	int         numNotes;
	int         i;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}

	numNotes = trap_XAnimGetDeferredNotifies( &notes );
	if ( numNotes <= 0 ) {
		return;
	}

	for ( i = 0 ; i < numNotes ; i++ ) {
		// keep: one-case switch, matches original codegen
		switch ( notes[i].notifyType ) {
		case 1:
			if ( !_stricmp( notes[i].name, "anim_gunhand = \"left\"" ) ) {
				bg_clientinfo[ clientNum ].gunHandLeft = 1;
			} else if ( !_stricmp( notes[i].name, "anim_gunhand = \"right\"" ) ) {
				bg_clientinfo[ clientNum ].gunHandLeft = 0;
			} else {
				break;
			}

			bg_clientinfo[ clientNum ].dobjNeedsUpdate = 1;
			break;
		}
	}
}

/*
===============
CG_AddPacketEntities

NOTE: the entities are taken from cg.nextSnap, not cg.snap as in RTCW
===============
*/
void CG_AddPacketEntities( void ) {
	int             num;
	centity_t       *cent;
	playerState_t   *ps;
	DObj            *obj;

	trap_DObjInvalidateSkels();

	// step every visible client's animation and collect its note tracks
	for ( num = 0 ; num < cg.nextSnap->numEntities ; num++ ) {
		obj = trap_GetClientDObj( cg.nextSnap->entities[ num ].number );
		if ( obj ) {
			trap_DObjUpdateClientInfo( obj, cg.frametime * 0.001f );
			CG_ProcessClientNoteTracks( cg.nextSnap->entities[ num ].number );
		}
	}

	cg.shakeMinAmplitude = 0;

	// the auto-rotating items will all have the same axis
	cg.autoAnglesSlow[0] = 0;
	cg.autoAnglesSlow[1] = ( cg.time & 4095 ) * 360 / 4095.0f;
	cg.autoAnglesSlow[2] = 0;

	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0f;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAnglesSlow, cg.autoAxisSlow );
	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.nextState, qfalse );
	cg.predictedPlayerEntity.currentState = cg.predictedPlayerEntity.nextState;

	// riding an mg42 puts the local player's own model in the view
	if ( cg.nextSnap->ps.pm_flags & PMF_VIEWLOCKED ) {
		obj = trap_GetClientDObj( cg.nextSnap->ps.clientNum );
		if ( obj ) {
			trap_DObjUpdateClientInfo( obj, cg.frametime * 0.001f );
			CG_ProcessClientNoteTracks( cg.nextSnap->ps.clientNum );
		}

		CG_AddCEntity( &cg.predictedPlayerEntity );
	}

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.nextSnap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.nextSnap->entities[ num ].number ];
		CG_AddCEntity( cent );
	}

	if ( cg_dumpAnims.integer >= 0 && cg_dumpAnims.integer < MAX_GENTITIES
		 && !cl_paused.integer ) {
		obj = trap_GetClientDObj( cg_dumpAnims.integer );
		if ( obj ) {
			trap_DObjDisplayAnim( obj );
		}
	}
}

/*
=============================================================================

	THE DObj TAG LAYER

	The cgame half of the game DLL's g_utils_mp.c block: same four functions,
	same order, with the DObj itself where the game passes a gentity_t *.

=============================================================================
*/

void CG_DObjUpdateInfo( DObj *obj ) {
	trap_DObjUpdateClientInfo( obj, cg.frametime * 0.001f );
}

/*
================
CG_DObjSetLocalTagInternal

Overwrite one bone of the pose with an explicit origin/angles.
================
*/
void CG_DObjSetLocalTagInternal( DObj *obj, const vec3_t origin, const vec3_t angles,
								 int boneIndex ) {
	DObjAnimMat_t   *rotTrans;
	float roll;
	struct { vec4_t yaw, pitch, roll, combined; } quats; // keep: matches original codegen

	rotTrans = &( (DObjAnimMat_t *)trap_DObjGetRotTransArray( obj ) )[boneIndex];

	QuatFromYaw_m( quats.yaw, angles[1] );
	QuatFromPitch_m( quats.pitch, angles[0] );
	roll = angles[2];
	QuatFromRoll_m( roll, quats.roll );

	QuatMultiply( quats.pitch, quats.yaw, quats.combined );
	QuatMultiply( quats.roll, quats.combined, rotTrans->quat );

	rotTrans->accumulatedWeight = 0.0f;
	VectorCopy( origin, rotTrans->translation );
}

qboolean CG_DObjSetLocalTag( DObj *obj, int *partBits, const char *tagName,
							 const vec3_t origin, const vec3_t angles ) {
	int boneIndex;

	boneIndex = trap_DObjGetBoneIndex( obj, tagName );
	if ( boneIndex < 0 ) {
		return qfalse;
	}

	if ( !trap_DObjSetRotTransIndex( obj, partBits, boneIndex ) ) {
		return qfalse;
	}

	CG_DObjSetLocalTagInternal( obj, origin, angles, boneIndex );

	return qtrue;
}

qboolean CG_DObjSetControlTagAngles( DObj *obj, int *partBits, const char *tagName,
									 const vec3_t angles ) {
	int boneIndex;

	boneIndex = trap_DObjGetBoneIndex( obj, tagName );
	if ( boneIndex < 0 ) {
		return qfalse;
	}

	if ( !trap_DObjSetControlRotTransIndex( obj, partBits, boneIndex ) ) {
		return qfalse;
	}

	CG_DObjSetLocalTagInternal( obj, vec3_origin, angles, boneIndex );

	return qtrue;
}

/*
================
CG_DObjGetLocalTagMatrix

The tag's matrix in the model's own space -- no entity transform applied.
================
*/
float *CG_DObjGetLocalTagMatrix( DObj *obj, const char *tagName, centity_t *cent ) {
	int boneIndex;

	boneIndex = trap_DObjGetBoneIndex( obj, tagName );
	if ( boneIndex < 0 ) {
		return NULL;
	}

	CG_DObjCalcBone( obj, boneIndex, cent );

	return DOBJ_BONEMATRIX( trap_DObjGetMatrixArray( obj ), boneIndex );
}

/*
================
CG_DObjGetWorldTagMatrix
================
*/
qboolean CG_DObjGetWorldTagMatrix( DObj *obj, const char *tagName, centity_t *cent,
								   float *outMatrix ) {
	float   *boneMatrix;
	vec3_t  entMatrix[4];
	vec3_t  right;

	boneMatrix = CG_DObjGetLocalTagMatrix( obj, tagName, cent );
	if ( !boneMatrix ) {
		return qfalse;
	}

	AngleVectors( cent->lerpAngles, entMatrix[0], right, entMatrix[2] );
	VectorSubtract( vec3_origin, right, entMatrix[1] );
	VectorCopy( cent->lerpOrigin, entMatrix[3] );

	DObjSkel2MatrixMultiply43( boneMatrix, entMatrix[0], outMatrix );

	return qtrue;
}

/*
================
CG_DObjGetViewModelTagMatrix

Same thing for the first-person weapon, which is placed by cg.viewModel*
rather than by an entity's lerp origin.
================
*/
qboolean CG_DObjGetViewModelTagMatrix( DObj *obj, const char *tagName, float *outMatrix ) {
	int     boneIndex;
	int     partBits[4];
	const float *boneMatrix;
	vec3_t  viewMatrix[4];

	boneIndex = trap_DObjGetBoneIndex( obj, tagName );
	if ( boneIndex < 0 ) {
		return qfalse;
	}

	if ( !trap_DObjCreateSkelForBone( obj, boneIndex ) ) {
		trap_DObjGetHierarchyBits( obj, boneIndex, partBits );
		trap_DObjCalcAnim( obj, partBits );
		trap_DObjCalcSkel( obj, partBits );
	}

	boneMatrix = DOBJ_BONEMATRIX( trap_DObjGetMatrixArray( obj ), boneIndex );

	VectorCopy( cg.viewModelAxis[0], viewMatrix[0] );
	VectorCopy( cg.viewModelAxis[1], viewMatrix[1] );
	VectorCopy( cg.viewModelAxis[2], viewMatrix[2] );
	VectorCopy( cg.viewModelOrigin, viewMatrix[3] );

	DObjSkel2MatrixMultiply43( boneMatrix, viewMatrix[0], outMatrix );

	return qtrue;
}

/*
================
CG_MatrixTransformVector44

Transform one point by a 4x4 bone matrix
================
*/
static void CG_MatrixTransformVector44( const vec3_t in, const float *m, vec3_t out ) {
	out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12];
	out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13];
	out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
}

/*
================
CG_DObjGetLocalBoxEdges

The twelve edges of one DObj part's collision box, as 24 endpoints in model
space.  Debug code; only CG_DObjGetWorldBoxEdges calls it, and nothing calls that.
================
*/
static const int cg_boxEdgeCorners[24][3] = {
	{ 0, 0, 0 }, { 1, 0, 0 },
	{ 0, 0, 0 }, { 0, 1, 0 },
	{ 1, 1, 0 }, { 1, 0, 0 },
	{ 1, 1, 0 }, { 0, 1, 0 },
	{ 0, 0, 1 }, { 1, 0, 1 },
	{ 0, 0, 1 }, { 0, 1, 1 },
	{ 1, 1, 1 }, { 1, 0, 1 },
	{ 1, 1, 1 }, { 0, 1, 1 },
	{ 0, 0, 0 }, { 0, 0, 1 },
	{ 1, 0, 0 }, { 1, 0, 1 },
	{ 0, 1, 0 }, { 0, 1, 1 },
	{ 1, 1, 0 }, { 1, 1, 1 }
};

static qboolean CG_DObjGetLocalBoxEdges( DObj *obj, const char *tagName, vec3_t *points ) {
	int         boneIndex;
	int         numParts;
	float       **partBounds;
	float       *bounds;
	float       *matrix;
	const int   *corner;
	vec3_t      *out;
	vec3_t      v;
	int         i;

	boneIndex = trap_DObjGetBoneIndex( obj, tagName );
	if ( boneIndex < 0 ) {
		return qfalse;
	}

	numParts = trap_DObjNumBones( obj );
	partBounds = (float **)_alloca( numParts * sizeof( float * ) );
	trap_DObjBuildPartCollisionTable( obj, partBounds );

	matrix = DOBJ_BONEMATRIX( trap_DObjGetMatrixArray( obj ), boneIndex );
	bounds = partBounds[ boneIndex ];

	out = points;
	corner = cg_boxEdgeCorners[0];
	for ( i = 0 ; i < 12 ; i++ ) {
		v[0] = bounds[ corner[0] * 3 + 0 ];
		v[1] = bounds[ corner[1] * 3 + 1 ];
		v[2] = bounds[ corner[2] * 3 + 2 ];
		CG_MatrixTransformVector44( v, matrix, *out );
		out++;
		corner += 3;

		v[0] = bounds[ corner[0] * 3 + 0 ];
		v[1] = bounds[ corner[1] * 3 + 1 ];
		v[2] = bounds[ corner[2] * 3 + 2 ];
		CG_MatrixTransformVector44( v, matrix, *out );
		out++;
		corner += 3;
	}

	return qtrue;
}

/*
================
CG_DObjGetWorldBoxEdges

The same 24 points rotated and translated into world space.  Unused.
================
*/
qboolean CG_DObjGetWorldBoxEdges( DObj *obj, centity_t *cent, const char *tagName, vec3_t *points ) {
	float   matrix[16];
	vec3_t  out;
	int     i;

	if ( !CG_DObjGetLocalBoxEdges( obj, tagName, points ) ) {
		return qfalse;
	}

	AngleVectors( cent->lerpAngles, &matrix[0], &matrix[4], &matrix[8] );
	VectorNegate( &matrix[4], &matrix[4] );
	VectorCopy( cent->lerpOrigin, &matrix[12] );

	for ( i = 0 ; i < 24 ; i++ ) {
		CG_MatrixTransformVector44( points[i], matrix, out );
		VectorCopy( out, points[i] );
	}

	return qtrue;
}

/*
===============
CG_SoundBlend

ET_SOUND_BLEND crossfades two sound aliases; leanf carries the blend factor.
===============
*/
static void CG_SoundBlend( centity_t *cent ) {
	const char  *alias1;
	const char  *alias2;
	float       blend;

	if ( !cent->currentState.eventParm ) {
		return;
	}
	if ( !cent->currentState.scale ) {
		return;
	}

	alias1 = trap_Com_PickSoundAlias( CG_ConfigString( CS_SOUNDALIAS + cent->currentState.eventParm ) );
	alias2 = trap_Com_PickSoundAlias( CG_ConfigString( CS_SOUNDALIAS + cent->currentState.scale ) );

	if ( !alias1 || !alias2 ) {
		return;
	}

	blend = cent->currentState.leanf
			+ cg.frameInterpolation * ( cent->nextState.leanf - cent->currentState.leanf );

	trap_MSS_PlayBlendedSoundAliases( alias1, alias2, blend,
									  cent->currentState.number, cent->lerpOrigin, 0 );
}

/*
===============
CG_LoopFx

ET_FX replays one effect on an interval.  angles2[0] is the cull range,
angles2[1] the interval in msec, origin2 the optional direction.
===============
*/
static void CG_LoopFx( centity_t *cent ) {
	int     interval;
	float   range;
	int     fxIndex;
	int     fx;

	if ( cg.time > cent->lastLoopFxTime ) {
		interval = (int)cent->currentState.angles2[1];
		if ( cg.time - cent->lastLoopFxTime < interval ) {
			return;
		}
		do {
			cent->lastLoopFxTime += interval;
		} while ( cg.time - cent->lastLoopFxTime > interval );
	} else {
		cent->lastLoopFxTime = cg.time;
	}

	range = cent->currentState.angles2[0];
	if ( range != 0 ) {
		// keep: argument order matches original codegen
		if ( DistanceSquared( cg.predictedPlayerState.origin, cent->lerpOrigin ) >= range * range ) {
			return;
		}
	}

	fxIndex = cent->currentState.scale;
	if ( fxIndex <= 0 || fxIndex >= MAX_FX ) {
		CG_Printf( "ERROR: CG_PlayFx called with invalid effect id %i\n", fxIndex );
		return;
	}

	fx = cgs.gameEffects[ fxIndex ];
	if ( DotProduct( cent->currentState.origin2, cent->currentState.origin2 ) != 0 ) {
		trap_FX_PlayEffectID( fx, cent->lerpOrigin, cent->currentState.origin2 );
	} else {
		trap_FX_PlaySimpleEffectID( fx, cent->lerpOrigin );
	}
}

/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t          current, next;
	float           f;
	clientInfo_t    *ci;

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

	// a player's view angles drive the animation system, not the model's axis
	if ( cent->nextState.eType == ET_PLAYER ) {
		ci = &bg_clientinfo[ cent->nextState.clientNum ];

		ci->leanAmount = LerpAngle( cent->currentState.angles2[1], cent->nextState.angles2[1], f );
		ci->viewPitch = cent->lerpAngles[0];
		ci->viewYaw = cent->lerpAngles[1];
		ci->viewRoll = cent->lerpAngles[2];

		cent->lerpAngles[0] = 0;
		cent->lerpAngles[2] = 0;

		ci->leanFraction = LerpAngle( cent->currentState.leanf, cent->nextState.leanf, f );
	}
}

/*
===============
CG_CalcEntityLerpPositions
===============
*/
void CG_CalcEntityLerpPositions( centity_t *cent ) {
	clientInfo_t    *ci;

	if ( cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// fix for jittery clients in multiplayer
	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->currentState.pos.trType == TR_LINEAR_STOP
		 && cent->currentState.number < MAX_CLIENTS ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	// keep: one copy per type, matches original codegen
	if ( cent->currentState.eType == ET_PLAYER ) {
		ci = &bg_clientinfo[ cent->currentState.clientNum ];
		ci->leanAmount = cent->currentState.angles2[1];
		ci->viewPitch = cent->lerpAngles[0];
		ci->viewYaw = cent->lerpAngles[1];
		ci->viewRoll = cent->lerpAngles[2];

		cent->lerpAngles[0] = 0;
		cent->lerpAngles[2] = 0;

		ci->leanFraction = cent->currentState.leanf;
	} else if ( cent->currentState.eType == ET_CORPSE ) {
		ci = &cgs.corpseinfo[ cent->currentState.number - MAX_CLIENTS ];
		ci->leanAmount = cent->currentState.angles2[1];
		ci->viewPitch = cent->lerpAngles[0];
		ci->viewYaw = cent->lerpAngles[1];
		ci->viewRoll = cent->lerpAngles[2];

		cent->lerpAngles[0] = 0;
		cent->lerpAngles[2] = 0;

		ci->leanFraction = cent->currentState.leanf;
	}

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum,
								   cg.snap->serverTime, cg.time, cent->lerpOrigin, NULL );
	}
}

/*
===============
CG_GetAnimations
===============
*/
void *CG_GetAnimations( int eType, int entityNum ) {
	if ( eType != ET_MG42 ) {
		return NULL;
	}
	return CG_GetMG42Anims( &cg_entities[ entityNum ] );
}

/*
===============
CG_PreProcess_GetDObj

Keep one DObj alive per entity, rebuilding it whenever the entity's type or
XModel changes.  cg.iEntityLastType / cg.pEntityLastXModel are the cache.
===============
*/
DObj *CG_PreProcess_GetDObj( int entityNum, int eType, qhandle_t hModel ) {
	DObj            *obj;
	void            *xmodel;
	void            *anims;
	void            *tree;
	DObjModel       models[1];

	obj = trap_GetClientDObj( entityNum );
	xmodel = trap_R_GetXModelByHandle( hModel );

	if ( obj ) {
		if ( xmodel
			 && cg.iEntityLastType[ entityNum ] == eType
			 && cg.pEntityLastXModel[ entityNum ] == xmodel ) {
			return obj;
		}

		trap_SafeDObjFree( entityNum, 1 );
		obj = NULL;
		cg.iEntityLastType[ entityNum ] = 0;
		cg.pEntityLastXModel[ entityNum ] = 0;
	}

	if ( xmodel ) {
		anims = CG_GetAnimations( eType, entityNum );
		if ( anims ) {
			tree = trap_XAnimCreateTree( anims );
		} else {
			tree = NULL;
		}

		memset( models, 0, sizeof( models ) );
		models[0].model = xmodel;
		// the model handle, not the negated model index the game uses
		models[0].boneName = (short)hModel;

		trap_DObjCreate( models, 1, tree, entityNum );

		cg.iEntityLastType[ entityNum ] = eType;
		cg.pEntityLastXModel[ entityNum ] = xmodel;

		return trap_GetClientDObj( entityNum );
	}

	return obj;
}

/*
===============
CG_Player_DoControllers

Apply the player's bone controllers
===============
*/
void CG_Player_DoControllers( centity_t *cent, int *partBits ) {
	DObj            *obj;
	clientInfo_t    *ci;

	obj = trap_GetClientDObj( cent->currentState.number );
	ci = &bg_clientinfo[ cent->currentState.clientNum ];

	if ( !ci->infoValid ) {
		return;
	}

	BG_Player_DoControllers( obj, ci, &cent->currentState, partBits );
}

/*
===============
CG_DoControllers
===============
*/
void CG_DoControllers( centity_t *cent, int *partBits ) {
	switch ( cent->currentState.eType ) {
	case ET_PLAYER:
		CG_Player_DoControllers( cent, partBits );
		break;
	case ET_MG42:
		CG_mg42_DoControllers( cent, partBits );
		break;
	default:
		break;
	}
}

/*
===============
CG_DObjCalcPose

Called through vmMain by the renderer, with the refEntity's owner and dobj.
===============
*/
void CG_DObjCalcPose( centity_t *cent, DObj *obj, int *partBits ) {
	if ( trap_DObjCreateSkelForBones( obj, partBits ) ) {
		return;
	}

	trap_DObjCalcAnim( obj, partBits );
	CG_DoControllers( cent, partBits );
	trap_DObjCalcSkel( obj, partBits );
}

/*
===============
CG_DObjCalcBone
===============
*/
void CG_DObjCalcBone( DObj *obj, int boneIndex, centity_t *cent ) {
	int partBits[4];

	if ( trap_DObjCreateSkelForBone( obj, boneIndex ) ) {
		return;
	}

	trap_DObjGetHierarchyBits( obj, boneIndex, partBits );
	trap_DObjCalcAnim( obj, partBits );
	CG_DoControllers( cent, partBits );
	trap_DObjCalcSkel( obj, partBits );
}

/*
===============
CG_DObjCalcBoneGeneric

Called through vmMain.  Takes the entity number rather than a centity_t, and
looks the DObj up itself.
===============
*/
void CG_DObjCalcBoneGeneric( int entityNum, int boneIndex ) {
	DObj *obj;
	int partBits[4];

	obj = trap_GetClientDObj( entityNum );

	if ( trap_DObjCreateSkelForBone( obj, boneIndex ) ) {
		return;
	}

	trap_DObjGetHierarchyBits( obj, boneIndex, partBits );
	trap_DObjCalcAnim( obj, partBits );

	if ( entityNum < MAX_GENTITIES ) {
		CG_DoControllers( &cg_entities[ entityNum ], partBits );
	}

	trap_DObjCalcSkel( obj, partBits );
}

/*
===============
CG_ProcessEntity
===============
*/
void CG_ProcessEntity( centity_t *cent ) {
	// add automatic effects
	CG_EntityEffects( cent );

	// keep: case order matches original codegen
	switch ( cent->currentState.eType ) {
	case ET_MG42:
		CG_mg42( cent );
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_CORPSE:
		CG_Corpse( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_SCRIPTMOVER:
		CG_ScriptMover( cent );
		break;
	case ET_SOUND_BLEND:
		CG_SoundBlend( cent );
		break;
	case ET_FX:
		CG_LoopFx( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_INVISIBLE:
		break;
	default:
		CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
		break;
	}
}
