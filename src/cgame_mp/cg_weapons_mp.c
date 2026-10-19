// cg_weapons_mp.c -- events and effects dealing with weapons
// The viewmodel is a DObj driven by XAnim goal weights, weapon selection uses
// the slot bar, and the sway / ADS / recoil maths live in bg_weapon.c.

#include "cg_local.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// com_math.c / q_shared.c
vec_t           VectorNormalize( vec3_t v );
float           GetLeanFraction( float f );

// bg_misc.c / bg_weapon.c
void            BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result );
void            BG_CalculateWeaponAngles( pmWeaponAngleState_t *state, vec3_t angles );
void            BG_CalculateWeaponPosition_Sway( const playerState_t *ps, vec3_t swayOffsets,
										 vec3_t swayAngles, vec3_t previousViewAngles, float scale, int msec );
int             BG_GetWeaponIndexForName( const char *name );
int             BG_GetWeaponSlotForName( const char *name );

// Com_Error level for localization failures; errorParm_t stops short of it
#define ERR_LOCALIZATION    7

// cg_main_mp.c
int             CG_PlaySoundAliasByName( const char *name, int entnum, const vec3_t origin );

// cg_localents_mp.c
localEntity_t  *CG_AllocLocalEntity( void );

// cg_draw_mp.c
qboolean        CG_GetWeapReticleZoom( float *zoom );

// cg_newDraw_mp.c / ui_shared.c
void            Controls_GetConfig( void );
const char     *BindingFromName( const char *name );

/*
=============================================================================

	LOCAL TYPES

=============================================================================
*/

typedef struct cgItemInfo_t {
	qboolean        registered;
	qhandle_t       models[MAX_ITEM_MODELS];
	qhandle_t       icons[MAX_ITEM_ICONS];  // only [0] is used
	const char     *pickupSound;
	const char     *ammoPickupSound;
} cgItemInfo_t;
CG_ASSERT_SIZE( cgItemInfo_t, 36 );
CG_ASSERT_OFFSET( cgItemInfo_t, pickupSound, 0x1C );

// one part of a viewmodel DObj: the model, the tag it hangs off, and the
// handle it was registered under
typedef struct cgDObjModel_t {
	void           *xmodel;
	const char     *tagName;
	short           hModel;
	short           pad;
} cgDObjModel_t;
CG_ASSERT_SIZE( cgDObjModel_t, 12 );

#define RT_DOBJ                 1
#define RT_SPRITE_LOCAL         5
#define RT_RAIL_CORE            7

#define RF_VIEWMODEL            0x0C
#define RF_PLAYERMODEL          0x8C

#define LE_FADE_RGB             0
#define LE_SCALE_FADE           1
#define LE_MOVING_TRACER        2

#define SURF_TYPE_FLESH         7

#define ENTITYNUM_WORLD         ( MAX_GENTITIES - 2 )

#define PMF_FIRSTPERSON         0x50000     // the local client is viewing through this entity
#define PMF_WEAPONSELECTABLE    0x40000
#define EF_WEAPONLOCKED         0x00C000    // ps.eFlags
#define EF_NOVIEWWEAPON         0x00C000
#define PMF_ADS                 0x20

#define ET_PLAYER               1
#define ET_TURRET               0x0B
#define ET_EVENTS               0x0C
#define EV_FIRE_WEAPON_LASTSHOT 0xA1
#define EV_BULLET_HIT_SMALL     0xAD
#define EV_BULLET_HIT_CLIENT_SMALL 0xAF

#define WEAPON_RELOADING        5           // ps.weaponstate

// the viewmodel animations, in weaponInfo_t order (idleAnim .. adsDownAnim);
// slot 0 of the tree is the root
#define WEAPANIM_IDLE           1
#define WEAPANIM_EMPTY_IDLE     2
#define WEAPANIM_FIRE           3
#define WEAPANIM_HOLDFIRE       4
#define WEAPANIM_LASTSHOT       5
#define WEAPANIM_RECHAMBER      6
#define WEAPANIM_MELEE          7
#define WEAPANIM_RELOAD         8
#define WEAPANIM_RELOAD_EMPTY   9
#define WEAPANIM_RELOAD_START   10
#define WEAPANIM_RELOAD_END     11
#define WEAPANIM_RAISE          12
#define WEAPANIM_DROP           13
#define WEAPANIM_ALTRAISE       14
#define WEAPANIM_ALTDROP        15
#define WEAPANIM_ADS_FIRE       16
#define WEAPANIM_ADS_LASTSHOT   17
#define WEAPANIM_ADS_RECHAMBER  18
#define WEAPANIM_ADS_UP         19
#define WEAPANIM_ADS_DOWN       20
#define WEAPANIM_NUM            21

// rows in bg_itemlist
#ifndef MAX_ITEMS
#define MAX_ITEMS               70
#endif

#define CS_ITEMS                8

// how long the weapon select overlay stays up
#define WEAPON_SELECT_TIME      1800

/*
=============================================================================

	GLOBALS

=============================================================================
*/

cgWeaponInfo_t  cg_weapons[MAX_WEAPONS_CG];
cgItemInfo_t    cg_items[MAX_ITEMS];

// world position of the view weapon's tag_brass, for the shell eject effect
static vec3_t   cg_brassOrigin;

// weapon select alt-weapon carousel: the slot and weapon the spin was last
// armed for, and the spin fraction in [-1,1] that decays to zero
static int      cg_weaponSelectSpinSlot;
static int      cg_weaponSelectSpinWeapon;
static float    cg_weaponSelectSpin;

// the tags muzzle flashes and bullet trajectories come from; turrets index
// it by tagIndex, everything else uses [0]
static const char *cg_flashTags[] = {
	"tag_flash",
	"tag_flash_11",
	"tag_flash_2",
	"tag_flash_22"
};

/*
=============================================================================

	VIEWMODEL ANIMATION

=============================================================================
*/

/*
==============
CG_StartWeaponAnim

Drive the viewmodel anim tree to `anim`: full goal weight on the one animation
asked for, zero on the other seventeen, all at this weapon's own rate.
==============
*/
static void CG_StartWeaponAnim( int weapon, void *tree, int anim ) {
	const cgWeaponInfo_t    *cgWeapon;
	unsigned short i;   // keep: matches original codegen

	cgWeapon = &cg_weapons[ weapon ];
	for ( i = 1; i < 19; i++ ) {
		if ( anim == i ) {
			trap_XAnimSetGoalWeight( tree, i, 1.0f, 0.0f, cgWeapon->animRate[i], 1, 1 );
		} else {
			trap_XAnimSetGoalWeight( tree, i, 0.0f, 0.0f, cgWeapon->animRate[i], 0, 0 );
		}
	}
}

/*
==============
CG_PlayADSAnim

Cross-fade the two aim-down-sight animations and scrub both to the transition
fraction Pmove keeps in ps.fWeaponPosFrac.  `weapon` is unused.
==============
*/
static void CG_PlayADSAnim( int weapon, int anim, void *tree ) {
	if ( anim == WEAPANIM_ADS_UP ) {
		trap_XAnimSetGoalWeight( tree, WEAPANIM_ADS_UP, 1.0f, 0.5f, 0.0f, 1, 0 );
		trap_XAnimSetGoalWeight( tree, WEAPANIM_ADS_DOWN, 0.0f, 0.5f, 0.0f, 0, 0 );
	} else {
		trap_XAnimSetGoalWeight( tree, WEAPANIM_ADS_UP, 0.0f, 0.5f, 0.0f, 0, 0 );
		trap_XAnimSetGoalWeight( tree, WEAPANIM_ADS_DOWN, 1.0f, 0.5f, 0.0f, 1, 0 );
	}

	trap_XAnimSetTime( tree, WEAPANIM_ADS_UP, cg.predictedPlayerState.fWeaponPosFrac );
	trap_XAnimSetTime( tree, WEAPANIM_ADS_DOWN, 1.0f - cg.predictedPlayerState.fWeaponPosFrac );
}

/*
==============
CG_ProcessWeaponNoteTracks

Play the sound alias named by any note track the viewmodel anim tree hit this
frame.
==============
*/
static void CG_ProcessWeaponNoteTracks( void ) {
	const cgWeaponInfo_t    *cgWeapon;
	const char        **notes;
	const char         *alias;
	int numNotes;
	int i;

	alias = NULL;

	if ( !cg.predictedPlayerState.weapon ) {
		return;
	}

	cgWeapon = &cg_weapons[ cg.predictedPlayerState.weapon ];
	numNotes = trap_XAnimGetDeferredNotifies( &notes );
	for ( i = 0; i < numNotes; i++ ) {

		if ( !_stricmp( notes[i * 3], "noteTrackSoundA" ) ) {
			alias = cgWeapon->noteTrackSoundA;
		} else if ( !_stricmp( notes[i * 3], "noteTrackSoundB" ) ) {
			alias = cgWeapon->noteTrackSoundB;
		} else if ( !_stricmp( notes[i * 3], "noteTrackSoundC" ) ) {
			alias = cgWeapon->noteTrackSoundC;
		} else if ( !_stricmp( notes[i * 3], "noteTrackSoundD" ) ) {
			alias = cgWeapon->noteTrackSoundD;
		}
		if ( alias ) {
			CG_PlaySoundAliasByName( alias, cg.snap->ps.clientNum, cg.snap->ps.origin );
		}
	}
}

/*
==============
CG_WeaponRunXModelAnims

Translate ps.weapAnim into the viewmodel anim tree, once per change.  Bit 9 of
weapAnim is Pmove's restart toggle and is masked out of the dispatch but kept in
the cached value, so re-firing the same animation restarts it.
==============
*/
static void CG_WeaponRunXModelAnims( playerState_t *ps, cgWeaponInfo_t *cgWeapon ) {
	const weaponInfo_t  *weaponInfo;
	qboolean ads;
	void *tree;
	int i;

	tree = trap_DObjGetTree( cgWeapon->dobj );
	weaponInfo = bg_weaponInfo[ps->weapon];

	// keep: the redundant store matches original codegen
	ads = qfalse;
	if ( ps->weaponstate == WEAPON_RELOADING
		 && ps->weaponTime - weaponInfo->adsReloadTransTime > 0 ) {
		ads = qfalse;
	} else if ( ps->pm_flags & PMF_ADS ) {
		ads = qtrue;
	}
	if ( weaponInfo->aimDownSight ) {
		if ( ads ) {
			CG_PlayADSAnim( ps->weapon, WEAPANIM_ADS_UP, tree );
		} else {
			CG_PlayADSAnim( ps->weapon, WEAPANIM_ADS_DOWN, tree );
		}
	}

	if ( ps->weapAnim == cgWeapon->lastWeapAnim ) {
		return;
	}

	switch ( ps->weapAnim & ~0x200 ) {
	case 0:
		// idle: hold off until every animation in the tree has finished,
		// invalidating the cache until then
		for ( i = 1; i < 19; i++ ) {
			if ( !trap_XAnimHasFinished( tree, i ) ) {
				cgWeapon->lastWeapAnim = -1;
				return;
			}
		}
		if ( !ps->ammoclip[bg_weaponInfo[ps->weapon]->clipIndex] ) {
			CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_EMPTY_IDLE );
		} else {
			CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_IDLE );
		}
		break;
	case 2:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_FIRE );
		break;
	case 3:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_LASTSHOT );
		break;
	case 4:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_RECHAMBER );
		break;
	case 5:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_ADS_FIRE );
		break;
	case 6:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_ADS_LASTSHOT );
		break;
	case 7:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_ADS_RECHAMBER );
		break;
	case 8:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_MELEE );
		break;
	case 9:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_DROP );
		break;
	case 10:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_RAISE );
		break;
	case 11:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_RELOAD );
		break;
	case 12:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_RELOAD_EMPTY );
		break;
	case 13:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_RELOAD_START );
		break;
	case 14:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_RELOAD_END );
		break;
	case 15:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_ALTDROP );
		break;
	case 16:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_ALTRAISE );
		break;
	case 17:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_HOLDFIRE );
		break;
	default:
		CG_StartWeaponAnim( ps->weapon, tree, WEAPANIM_IDLE );
		Com_Printf( "CG_WeaponRunXModelAnims: Unknown weapon animation %i\n",
					ps->weapAnim & ~0x200 );
		break;
	}

	cgWeapon->lastWeapAnim = ps->weapAnim;
}

/*
=============================================================================

	DEBUG LINE DRAWING

=============================================================================
*/

/*
==============
CG_RailTrail2

re-inserted this as a debug mechanism for bullets
One coloured line that fades over cg_railTrailTime msec.
==============
*/
static void CG_RailTrail2( const vec3_t color, const vec3_t start, const vec3_t end ) {
	localEntity_t   *le;
	refEntity_t     *re;

	if ( cg_railTrailTime.integer <= 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->endTime = cg.time + cg_railTrailTime.integer;
	le->lifeRate = 1.0f / (float)cg_railTrailTime.integer;

	re->shaderTime = (float)cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy( start, re->origin );
	VectorCopy( end, re->oldorigin );

	le->color[0] = color[0];
	le->color[1] = color[1];
	le->color[2] = color[2];
	le->color[3] = 1.0f;

	AxisClear( re->axis );
}

/*
==============
CG_RailTrail

modified so we could draw boxes for debugging as well
The EV_RAILTRAIL debug primitive.  `type` picks both the colour and the shape:
0-1 and 'E'-'H' are a single line, 2 and 3 are a wireframe box between the two
points, 5 is a sixteen-segment circle of radius end[0] about start.
==============
*/
void CG_RailTrail( int type, const vec3_t end, const vec3_t start ) {
	vec3_t color;
	vec3_t up, perp, cross;
	vec3_t size;
	vec3_t c1, c2, c3, c4, c5, c6;
	vec3_t points[16];
	float radius;
	float angle, s, c;
	int i;

	color[0] = 0.0f;
	color[1] = color[2] = 1.0f;

	// keep: an if chain, not a switch (matches original codegen)
	if ( type == 0 ) {
		color[0] = 0.0f;
		color[1] = color[2] = 1.0f;
		CG_RailTrail2( color, end, start );
		return;
	} else if ( type == 1 ) {
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		CG_RailTrail2( color, end, start );
		return;
	} else if ( type == 'E' ) {
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 0.0f;
		CG_RailTrail2( color, end, start );
		return;
	} else if ( type == 'F' ) {
		color[0] = 0.0f;
		color[1] = 1.0f;
		color[2] = 0.0f;
		CG_RailTrail2( color, end, start );
		return;
	} else if ( type == 'G' ) {
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		CG_RailTrail2( color, end, start );
		return;
	} else if ( type == 'H' ) {
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 1.0f;
		CG_RailTrail2( color, end, start );
		return;
	} else if ( type == 2 ) {
		// the box in the default colour
	} else if ( type == 3 ) {
		color[0] = 1.0f;
		color[1] = color[2] = 0.0f;
	} else if ( type == 5 ) {
		radius = end[0];
		up[0] = 0.0f;
		up[1] = 0.0f;
		up[2] = 1.0f;
		PerpendicularVector( perp, up );
		CrossProduct( up, perp, cross );
		for ( i = 0; i < sizeof( points ) / sizeof( points[0] ); i++ ) {
			angle = i * 0.39269909f;                // 2*PI / 16
			{
				// keep: inline fsincos, matches original codegen
				float *sinPtr = &s;
				float *sinOut, *cosOut;

				cosOut = &c;
				sinOut = sinPtr;
				__asm {
					push    eax
					push    ecx
					fld     angle
					fsincos
					mov     ecx, cosOut
					mov     eax, sinOut
					fstp    dword ptr [ecx]
					fstp    dword ptr [eax]
					pop     ecx
					pop     eax
				}
			}
			s *= radius;
			c *= radius;
			VectorScale( cross, s, points[i] );
			VectorMA( points[i], c, perp, points[i] );
			VectorAdd( points[i], start, points[i] );
		}
		for ( i = 0; i < sizeof( points ) / sizeof( points[0] ); i++ ) {
			CG_RailTrail2( color, points[i], points[( i + 1 ) & 15] );
		}
		return;
	}

	// the box: six corners off end and start, twelve edges
	VectorSubtract( end, start, size );

	VectorCopy( end, c1 );
	VectorCopy( end, c2 );
	VectorCopy( end, c3 );
	c1[0] -= size[0];
	c2[1] -= size[1];
	c3[2] -= size[2];

	CG_RailTrail2( color, end, c1 );
	CG_RailTrail2( color, end, c2 );
	CG_RailTrail2( color, end, c3 );

	VectorCopy( start, c4 );
	VectorCopy( start, c5 );
	VectorCopy( start, c6 );
	c4[0] += size[0];
	c5[1] += size[1];
	c6[2] += size[2];

	CG_RailTrail2( color, start, c4 );
	CG_RailTrail2( color, start, c5 );
	CG_RailTrail2( color, start, c6 );

	CG_RailTrail2( color, c2, c6 );
	CG_RailTrail2( color, c6, c1 );
	CG_RailTrail2( color, c1, c5 );
	CG_RailTrail2( color, c2, c4 );
	CG_RailTrail2( color, c4, c3 );
	CG_RailTrail2( color, c3, c5 );
}

/*
=============================================================================

	WEAPON REGISTRATION

=============================================================================
*/

/*
==============
CG_WeaponDObjHandle

The DObj slot a weapon's viewmodel lives in; viewmodel DObjs start at 1024.
==============
*/
static int CG_WeaponDObjHandle( int weapon ) {
	return weapon + 1024;
}

/*
==============
CG_RegisterWeapon

The server says this item is used on this level
The one-time load of everything cgame needs for a weapon: the viewmodel DObj
and its anim tree, the world model, the icons, the sound aliases and the
effects.  Idempotent -- `registered` gates the whole body.
==============
*/
void CG_RegisterWeapon( int weaponNum ) {
	const weaponInfo_t  *weaponInfo;
	cgWeaponInfo_t          *cgWeapon;
	cgItemInfo_t        *itemInfo;
	gitem_t             *item;
	cgDObjModel_t dobjModels[2];
	char filename[64];
	void *anims, *tree;
	rfloat rate;
	int i;

	if ( !weaponNum ) {
		return;
	}

	cgWeapon = &cg_weapons[ weaponNum ];
	weaponInfo = bg_weaponInfo[weaponNum];
	if ( cgWeapon->registered ) {
		return;
	}

	memset( cgWeapon, 0, sizeof( *cgWeapon ) );
	cgWeapon->registered = qtrue;

	// keep: read back through cgWeapon (matches original codegen)
	cgWeapon->item = &bg_itemlist[weaponNum];
	item = cgWeapon->item;
	itemInfo = &cg_items[weaponNum];
	CG_RegisterItemVisuals( weaponNum );
	cgWeapon->lastWeapAnim = -1;

	if ( weaponInfo->gunModel[0] ) {
		if ( !weaponInfo->handModel || !weaponInfo->handModel[0] ) {
			// "\x15" spliced: a following hex digit would extend the escape
			Com_Error( ERR_DROP, "\x15" "CG_RegisterWeapon: No hand model specified for [%s]",
					   weaponInfo->displayName );
		}
		if ( !weaponInfo->idleAnim || !weaponInfo->idleAnim[0] ) {
			Com_Error( ERR_DROP, "\x15" "CG_RegisterWeapon: No idle anim specified for [%s]",
					   weaponInfo->displayName );
		}

		// the anim tree: twenty animations, all children of "root", indexed
		// 1..20 in the order weaponInfo_t declares them; a weapon that leaves
		// one blank gets its idle animation in that slot instead
		anims = trap_XAnimCreateAnims( "VIEWMODEL", WEAPANIM_NUM );
		trap_XAnimBlend( anims, 0, "root", 1, 20, 0 );
		// the animation names are consecutive fields starting at idleAnim
		for ( i = 1; i < 21; i++ ) {
			if ( ( &weaponInfo->idleAnim )[i - 1][0] ) {
				trap_XAnimPrecache( ( &weaponInfo->idleAnim )[i - 1] );
				trap_XAnimCreate( anims, i, ( &weaponInfo->idleAnim )[i - 1] );
			} else {
				trap_XAnimPrecache( weaponInfo->idleAnim );
				trap_XAnimCreate( anims, i, weaponInfo->idleAnim );
			}
		}
		tree = trap_XAnimCreateTree( anims );

		// play the animations that have a fixed duration in the weapon file at
		// the rate that makes them last exactly that long
		for ( i = 0; i < WEAPANIM_NUM; i++ ) {
			cgWeapon->animRate[i] = 1.0f;
		}
		// keep: each rate's 0 default set before its test (original codegen)
		rate = 0.0f;
		if ( weaponInfo->holdFireTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_HOLDFIRE ) / (float)weaponInfo->holdFireTime;
		}
		cgWeapon->animRate[WEAPANIM_HOLDFIRE] = rate;
		rate = 0.0f;
		if ( weaponInfo->meleeTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_MELEE ) / (float)weaponInfo->meleeTime;
		}
		cgWeapon->animRate[WEAPANIM_MELEE] = rate;
		rate = 0.0f;
		if ( weaponInfo->reloadTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_RELOAD ) / (float)weaponInfo->reloadTime;
		}
		cgWeapon->animRate[WEAPANIM_RELOAD] = rate;
		rate = 0.0f;
		if ( weaponInfo->reloadEmptyTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_RELOAD_EMPTY ) / (float)weaponInfo->reloadEmptyTime;
		}
		cgWeapon->animRate[WEAPANIM_RELOAD_EMPTY] = rate;
		rate = 0.0f;
		if ( weaponInfo->reloadStartTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_RELOAD_START ) / (float)weaponInfo->reloadStartTime;
		}
		cgWeapon->animRate[WEAPANIM_RELOAD_START] = rate;
		// original quirk: reloadEnd divides by reloadTime, not reloadEndTime
		rate = 0.0f;
		if ( weaponInfo->reloadTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_RELOAD_END ) / (float)weaponInfo->reloadTime;
		}
		cgWeapon->animRate[WEAPANIM_RELOAD_END] = rate;
		rate = 0.0f;
		if ( weaponInfo->raiseTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_RAISE ) / (float)weaponInfo->raiseTime;
		}
		cgWeapon->animRate[WEAPANIM_RAISE] = rate;
		rate = 0.0f;
		if ( weaponInfo->dropTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_DROP ) / (float)weaponInfo->dropTime;
		}
		cgWeapon->animRate[WEAPANIM_DROP] = rate;
		rate = 0.0f;
		if ( weaponInfo->altRaiseTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_ALTRAISE ) / (float)weaponInfo->altRaiseTime;
		}
		cgWeapon->animRate[WEAPANIM_ALTRAISE] = rate;
		rate = 0.0f;
		if ( weaponInfo->altDropTime > 0 ) {
			rate = (float)trap_XAnimGetLength( anims, WEAPANIM_ALTDROP ) / (float)weaponInfo->altDropTime;
		}
		cgWeapon->animRate[WEAPANIM_ALTDROP] = rate;

		trap_XAnimClearTreeGoalWeights( tree, 0, 0.0f );
		trap_XAnimSetGoalWeight( tree, 0, 1.0f, 0.0f, cgWeapon->animRate[0], 1, 0 );

		if ( weaponInfo->adsUpAnim[0] && trap_XAnimIsLoopedIndex( anims, WEAPANIM_ADS_UP ) ) {
			Com_Error( ERR_DROP, "\x15" "CG_RegisterWeapon: ADS anim [%s] cannot be looping",
					   weaponInfo->adsUpAnim );
		}
		if ( weaponInfo->adsDownAnim[0] && trap_XAnimIsLoopedIndex( anims, WEAPANIM_ADS_DOWN ) ) {
			Com_Error( ERR_DROP, "\x15" "CG_RegisterWeapon: ADS anim [%s] cannot be looping",
					   weaponInfo->adsDownAnim );
		}

		// the viewmodel DObj: part 0 is the hands, part 1 is the gun, bolted
		// to the hands' tag_weapon
		sprintf( filename, "%s%s", "xmodel/", weaponInfo->handModel );
		memset( &dobjModels[0], 0, sizeof( dobjModels[0] ) );
		dobjModels[0].hModel = (short)trap_R_RegisterModel( filename, 6 );
		dobjModels[0].xmodel = trap_R_GetXModelByHandle( dobjModels[0].hModel );
		dobjModels[0].tagName = NULL;

		sprintf( filename, "%s%s", "xmodel/", weaponInfo->gunModel );
		memset( &dobjModels[1], 0, sizeof( dobjModels[1] ) );
		dobjModels[1].hModel = (short)trap_R_RegisterModel( filename, 6 );
		dobjModels[1].xmodel = trap_R_GetXModelByHandle( dobjModels[1].hModel );
		dobjModels[1].tagName = "tag_weapon";

		trap_DObjCreate( dobjModels, 2, tree, CG_WeaponDObjHandle( weaponNum ) );
		cgWeapon->dobj = trap_GetClientDObj( CG_WeaponDObjHandle( weaponNum ) );

		Q_strncpyz( cgWeapon->handModelName, weaponInfo->handModel, sizeof( cgWeapon->handModelName ) );
	}

	if ( weaponInfo->worldModel[0] ) {
		cgWeapon->worldModel = trap_R_RegisterModel( weaponInfo->worldModel, 7 );
	}
	if ( weaponInfo->worldModel[0] && !cgWeapon->worldModel ) {
		Com_Printf( "WARNING: Weapon %s could not load world model\n", weaponInfo->worldModel );
	}

	cgWeapon->weaponIcon = trap_R_RegisterShader( item->icon, 5 );
	cgWeapon->weaponSelectIcon = trap_R_RegisterShader( va( "%s_select", item->icon ), 5 );
	cgWeapon->ammoIcon = trap_R_RegisterShader( item->ammoicon, 5 );

	if ( weaponInfo->reticleCenter[0] ) {
		cgWeapon->reticleCenter = trap_R_RegisterShaderNoMip( weaponInfo->reticleCenter, 5 );
	}
	if ( weaponInfo->reticleSide[0] ) {
		cgWeapon->reticleSide = trap_R_RegisterShaderNoMip( weaponInfo->reticleSide, 5 );
	}
	if ( weaponInfo->adsOverlayShader[0] ) {
		cgWeapon->adsOverlayShader = trap_R_RegisterShaderNoMip( weaponInfo->adsOverlayShader, 5 );
	}

	if ( weaponInfo->viewFlashEffect[0] ) {
		cgWeapon->viewFlashEffect = trap_FX_RegisterEffect( weaponInfo->viewFlashEffect );
	}
	if ( weaponInfo->worldFlashEffect[0] ) {
		cgWeapon->worldFlashEffect = trap_FX_RegisterEffect( weaponInfo->worldFlashEffect );
	}

	cgWeapon->projectileSound = (const char *)trap_Com_SoundAliasString( weaponInfo->projectileSound );
	cgWeapon->pullbackSound = (const char *)trap_Com_SoundAliasString( weaponInfo->pullbackSound );
	cgWeapon->fireSound = (const char *)trap_Com_SoundAliasString( weaponInfo->fireSound );
	cgWeapon->fireEchoSound = (const char *)trap_Com_SoundAliasString( weaponInfo->fireEchoSound );
	cgWeapon->lastShotSound = (const char *)trap_Com_SoundAliasString( weaponInfo->lastShotSound );
	cgWeapon->rechamberSound = (const char *)trap_Com_SoundAliasString( weaponInfo->rechamberSound );
	cgWeapon->reloadSound = (const char *)trap_Com_SoundAliasString( weaponInfo->reloadSound );
	cgWeapon->reloadEmptySound = (const char *)trap_Com_SoundAliasString( weaponInfo->reloadEmptySound );
	cgWeapon->reloadStartSound = (const char *)trap_Com_SoundAliasString( weaponInfo->reloadStartSound );
	cgWeapon->reloadEndSound = (const char *)trap_Com_SoundAliasString( weaponInfo->reloadEndSound );
	cgWeapon->raiseSound = (const char *)trap_Com_SoundAliasString( weaponInfo->raiseSound );
	if ( !cgWeapon->raiseSound ) {
		cgWeapon->raiseSound = (const char *)trap_Com_SoundAliasString( "weap_raise" );
	}
	cgWeapon->altSwitchSound = (const char *)trap_Com_SoundAliasString( weaponInfo->altSwitchSound );
	cgWeapon->putawaySound = (const char *)trap_Com_SoundAliasString( weaponInfo->putawaySound );
	if ( !cgWeapon->putawaySound ) {
		cgWeapon->putawaySound = (const char *)trap_Com_SoundAliasString( "weap_putaway" );
	}
	cgWeapon->noteTrackSoundA = (const char *)trap_Com_SoundAliasString( weaponInfo->noteTrackSoundA );
	cgWeapon->noteTrackSoundB = (const char *)trap_Com_SoundAliasString( weaponInfo->noteTrackSoundB );
	cgWeapon->noteTrackSoundC = (const char *)trap_Com_SoundAliasString( weaponInfo->noteTrackSoundC );
	cgWeapon->noteTrackSoundD = (const char *)trap_Com_SoundAliasString( weaponInfo->noteTrackSoundD );

	if ( !itemInfo->pickupSound ) {
		itemInfo->pickupSound = (const char *)trap_Com_SoundAliasString( "weap_pickup" );
	}
	itemInfo->ammoPickupSound = (const char *)trap_Com_SoundAliasString( weaponInfo->ammoPickupSound );
	if ( !itemInfo->ammoPickupSound ) {
		itemInfo->ammoPickupSound = (const char *)trap_Com_SoundAliasString( "weap_ammo_pickup" );
	}

	if ( weaponInfo->shellEjectEffect[0] ) {
		cgWeapon->shellEjectEffect = trap_FX_RegisterEffect( weaponInfo->shellEjectEffect );
	}
	if ( weaponInfo->lastShotEjectEffect[0] ) {
		cgWeapon->lastShotEjectEffect = trap_FX_RegisterEffect( weaponInfo->lastShotEjectEffect );
	} else {
		cgWeapon->lastShotEjectEffect = cgWeapon->shellEjectEffect;
	}

	if ( weaponInfo->projectileModel[0] ) {
		cgWeapon->missileModel = trap_R_RegisterModel( weaponInfo->projectileModel, 7 );
		if ( !cgWeapon->missileModel ) {
			CG_Error( "Weapon %s does not specify a valid projectile model (%s)\n",
					  weaponInfo->szInternalName, weaponInfo->worldModel );
		}
	}
	if ( weaponInfo->projExplosionEffect[0] ) {
		cgWeapon->projExplosionEffect = trap_FX_RegisterEffect( weaponInfo->projExplosionEffect );
	}
	cgWeapon->projExplosionSound =
		(const char *)trap_Com_SoundAliasString( weaponInfo->projExplosionSound );
	if ( weaponInfo->projTrailEffect[0] ) {
		cgWeapon->missileTrailEffect = trap_FX_RegisterEffect( weaponInfo->projTrailEffect );
	}
	cgWeapon->missileDlight = (float)weaponInfo->projectileDLight;

	if ( weaponInfo->hudIcon[0] ) {
		cgWeapon->hudIcon = trap_R_RegisterShader( weaponInfo->hudIcon, 5 );
		cgs.media.hintShaders[HINT_WEAPON_FIRST + weaponNum - 1] = cgWeapon->hudIcon;
	} else {
		cgs.media.hintShaders[HINT_WEAPON_FIRST + weaponNum - 1] = cgs.media.hintShaders[HINT_ACTIVATE];
	}
	if ( weaponInfo->killIcon[0] ) {
		trap_R_RegisterShader( weaponInfo->killIcon, 5 );
	}
	if ( weaponInfo->modeIcon[0] ) {
		cgWeapon->modeIcon = trap_R_RegisterShader( weaponInfo->modeIcon, 5 );
	}
	if ( weaponInfo->ammoIcon[0] ) {
		cgWeapon->ammoHudIcon = trap_R_RegisterShader( weaponInfo->ammoIcon, 5 );
		cgs.media.hintShaders[HINT_AMMO_FIRST + weaponNum - 1] = cgWeapon->ammoHudIcon;
	} else {
		cgs.media.hintShaders[HINT_AMMO_FIRST + weaponNum - 1] = cgs.media.hintShaders[HINT_ACTIVATE];
	}

	cgWeapon->displayName = trap_SE_TranslateReference( weaponInfo->displayName );
	if ( !cgWeapon->displayName ) {
		if ( cl_languagewarnings.integer ) {
			if ( cl_languagewarningsaserrors.integer ) {
				Com_Error( ERR_LOCALIZATION,
						   "Weapon %s: Could not translate display name \"%s\"",
						   weaponInfo->szInternalName, weaponInfo->displayName );
			} else {
				Com_Printf( "^3WARNING: Weapon %s: Could not translate display name \"%s\"\n",
							weaponInfo->szInternalName, weaponInfo->displayName );
			}
		}
		cgWeapon->displayName = weaponInfo->displayName;
	}
	cgWeapon->modeName = trap_SE_TranslateReference( weaponInfo->modeName );
	if ( !cgWeapon->modeName ) {
		if ( cl_languagewarnings.integer ) {
			if ( cl_languagewarningsaserrors.integer ) {
				Com_Error( ERR_LOCALIZATION, "Weapon %s: Could not translate mode name \"%s\"",
						   weaponInfo->szInternalName, weaponInfo->modeName );
			} else {
				Com_Printf( "^3WARNING: Weapon %s: Could not translate mode name \"%s\"\n",
							weaponInfo->szInternalName, weaponInfo->modeName );
			}
		}
		cgWeapon->modeName = weaponInfo->modeName;
	}
	cgWeapon->AIOverlayDescription = trap_SE_TranslateReference( weaponInfo->AIOverlayDescription );
	if ( !cgWeapon->AIOverlayDescription ) {
		if ( cl_languagewarnings.integer ) {
			if ( cl_languagewarningsaserrors.integer ) {
				Com_Error( ERR_LOCALIZATION,
						   "Weapon %s: Could not translate AI overlay description \"%s\"",
						   weaponInfo->szInternalName, weaponInfo->AIOverlayDescription );
			} else {
				Com_Printf( "^3WARNING: Weapon %s: Could not translate AI overlay description \"%s\"\n",
							weaponInfo->szInternalName, weaponInfo->AIOverlayDescription );
			}
		}
		cgWeapon->AIOverlayDescription = weaponInfo->AIOverlayDescription;
	}
}

/*
==============
CG_ChangeViewmodelDobj

Rebuild a weapon's viewmodel DObj around a different pair of hands, keeping the
anim tree it already has.  Called when the hand model changes at runtime.
==============
*/
static void CG_ChangeViewmodelDobj( int weapon, const char *handModel ) {
	const weaponInfo_t  *weaponInfo;
	cgWeaponInfo_t          *cgWeapon;
	cgDObjModel_t dobjModels[2];
	char handFile[64];
	char gunFile[64];
	void *tree;

	if ( !weapon ) {
		return;
	}

	weaponInfo = bg_weaponInfo[weapon];
	cgWeapon = &cg_weapons[ weapon ];
	if ( !cgWeapon->dobj ) {
		return;
	}
	if ( !weaponInfo->gunModel[0] ) {
		return;
	}

	tree = trap_DObjGetTree( cgWeapon->dobj );
	trap_SafeDObjFree( CG_WeaponDObjHandle( weapon ), 0 );

	memset( &dobjModels[0], 0, sizeof( dobjModels[0] ) );

	sprintf( handFile, "%s%s", "xmodel/", weaponInfo->handModel );
	if ( handModel[0] ) {
		dobjModels[0].hModel = (short)trap_R_RegisterModel( handModel, 6 );
	} else {
		dobjModels[0].hModel = (short)trap_R_RegisterModel( handFile, 6 );
	}
	dobjModels[0].xmodel = trap_R_GetXModelByHandle( (short)trap_R_RegisterModel( handFile, 6 ) );
	dobjModels[0].tagName = NULL;
	memset( &dobjModels[1], 0, sizeof( dobjModels[1] ) );

	sprintf( gunFile, "%s%s", "xmodel/", weaponInfo->gunModel );
	dobjModels[1].hModel = (short)trap_R_RegisterModel( gunFile, 6 );
	dobjModels[1].xmodel = trap_R_GetXModelByHandle( dobjModels[1].hModel );
	dobjModels[1].tagName = "tag_weapon";

	trap_DObjCreate( dobjModels, 2, tree, CG_WeaponDObjHandle( weapon ) );
	cgWeapon->dobj = trap_GetClientDObj( CG_WeaponDObjHandle( weapon ) );

	Q_strncpyz( cgWeapon->handModelName, handModel, sizeof( cgWeapon->handModelName ) );
}

/*
==============
CG_UpdateHandViewmodels

Swap every registered weapon over to a new hand model, skipping the ones that
already have it.
==============
*/
void CG_UpdateHandViewmodels( const char *handModel ) {
	int i;

	for ( i = 1; i <= bg_numWeapons; i++ ) {
		if ( strcmp( cg_weapons[ i ].handModelName, handModel ) ) {
			CG_ChangeViewmodelDobj( i, handModel );
		}
	}
}

/*
==============
CG_FreeWeapons

Drop every viewmodel DObj at shutdown.
==============
*/
void CG_FreeWeapons( void ) {
	int i;

	for ( i = 1; i <= bg_numWeapons; i++ ) {
		trap_SafeDObjFree( CG_WeaponDObjHandle( i ), 1 );
	}
}

/*
==============
CG_RegisterItemVisuals

The server says this item is used on this level
RTCW's sizeof( &itemInfo ) is kept: the memset clears only `registered`.
==============
*/
void CG_RegisterItemVisuals( int itemNum ) {
	cgItemInfo_t    *itemInfo;
	gitem_t         *item;
	int i;

	itemInfo = &cg_items[itemNum];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[itemNum];
	memset( itemInfo, 0, sizeof( &itemInfo ) );

	for ( i = 0; i < MAX_ITEM_MODELS; i++ ) {
		if ( item->world_model[i] && item->world_model[i][0] ) {
			itemInfo->models[i] = trap_R_RegisterModel( item->world_model[i], i ? 6 : 7 );
		}
	}

	itemInfo->icons[0] = trap_R_RegisterShader( item->icon, 5 );

	if ( item->pickup_sound ) {
		itemInfo->pickupSound = (const char *)trap_Com_SoundAliasString( item->pickup_sound );
	}
	itemInfo->ammoPickupSound = itemInfo->pickupSound;

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	itemInfo->registered = qtrue;
}

/*
==============
CG_RegisterItems

The items configstring is one hex nibble per four item indexes; register the
visuals of every item the server says is in the level.
==============
*/
void CG_RegisterItems( void ) {
	char items[256 + 1];    // RTCW's MAX_ITEMS + 1
	int value;
	int i;

	// only register the items that the server says we need
	strcpy( items, CG_ConfigString( CS_ITEMS ) );

	for ( i = 1; i < MAX_ITEMS; i++ ) {
		value = items[i / 4];
		if ( value <= '9' ) {
			value -= '0';
		} else {
			value -= 'W';
		}
		if ( value & ( 1 << ( i & 3 ) ) ) {
			CG_RegisterItemVisuals( i );
		}
	}
}

/*
=============================================================================

	WEAPON POSITION

	bg_weapon.c owns the maths; this half owns the cg_t state it runs on.

=============================================================================
*/

/*
==============
CG_CalculateWeaponPosition_Sway

Feed the view-angle delta to the shared sway solver, scaled up while a
shellshock is running.
==============
*/
static void CG_CalculateWeaponPosition_Sway( void ) {
	const weaponInfo_t  *weaponInfo;
	float scale;
	rfloat frac;
	int remaining;

	weaponInfo = BG_GetInfoForWeapon( cg.predictedPlayerState.weapon );

	remaining = cg.shellshock.startTime - cg.time + cg.shellshock.duration;
	if ( remaining > 0 ) {
		frac = 1.0f;
		if ( remaining < cg.shellshock.parms->lerpTime ) {
			frac = (rfloat)remaining / cg.shellshock.parms->lerpTime;
		}
		scale = ( weaponInfo->swayShellShockScale - 1.0f )
				* ( ( 3.0f - ( frac + frac ) ) * frac * frac ) + 1.0f;
	} else {
		scale = 1.0f;
	}

	BG_CalculateWeaponPosition_Sway( &cg.predictedPlayerState, cg.swayOffsets, cg.swayAngles,
									 cg.previousViewAngles, scale, cg.frametime );
}

/*
==============
CG_CalculateWeaponPosition_SwayMovement
==============
*/
static void CG_CalculateWeaponPosition_SwayMovement( vec3_t offset ) {
	offset[1] = offset[1] - cg.swayOffsets[1];
	offset[2] = cg.swayOffsets[2] + offset[2];
}

/*
==============
CG_TRACKMOVEMENT

Move one component of the tracked weapon offset towards its target at the
weapon's position rate, by at least a minimum step a frame.
keep: a macro, frametime set in both arms, and the sum stored rounded but
compared unrounded -- all to match the original codegen.
==============
*/
#define CG_TRACKMOVEMENT( p, t, wi ) do { \
	float *pos_ = ( p ); \
	rfloat target_ = ( t ); \
	rfloat rate_, step_, sum_; \
	float frametime_, limit_; \
	if ( *pos_ == target_ ) { \
		break; \
	} \
	if ( (float)cg.predictedPlayerState.proneViewHeight == cg.predictedPlayerState.viewHeightCurrent ) { \
		frametime_ = (float)cg.frametime; \
		rate_ = (rfloat)cg_gun_move_rate.value + ( wi )->posProneMoveRate; \
	} else { \
		frametime_ = (float)cg.frametime; \
		rate_ = (rfloat)cg_gun_move_rate.value + ( wi )->posMoveRate; \
	} \
	step_ = rate_ * ( target_ - *pos_ ) * frametime_ * 0.001f; \
	if ( *pos_ < target_ ) { \
		limit_ = frametime_ * 0.00010000000474974513f; \
		if ( step_ < limit_ ) { \
			step_ = limit_; \
		} \
		sum_ = step_ + *pos_; \
		*pos_ = (float)sum_; \
		if ( sum_ > target_ ) { \
			*pos_ = (float)target_; \
		} \
	} else { \
		limit_ = frametime_ * -0.00010000000474974513f; \
		if ( step_ > limit_ ) { \
			step_ = limit_; \
		} \
		sum_ = step_ + *pos_; \
		*pos_ = (float)sum_; \
		if ( sum_ < target_ ) { \
			*pos_ = (float)target_; \
		} \
	} \
} while ( 0 )

/*
==============
CG_CalculateWeaponPosition_BasePosition_movement

The weapon's own idea of where it sits: a stance-dependent offset plus a
speed-scaled movement offset, each tracked towards its target at the weapon's
position rate and folded out again as the sights come up.
==============
*/
static void CG_CalculateWeaponPosition_BasePosition_movement( vec3_t offset ) {
	const weaponInfo_t  *weaponInfo;
	const playerState_t *ps;
	// keep: rfloat and the (rfloat) / (float) casts reproduce the original
	// rounding points
	rfloat move[3];
	rfloat minSpeed;
	float frac;
	rfloat adsScale;

	weaponInfo = cg.weaponInfo;
	ps = &cg.predictedPlayerState;

	if ( cg.predictedPlayerEntity.currentState.eFlags & 0x40 ) {
		minSpeed = (rfloat)cg_gun_move_minspeed.value + weaponInfo->proneMoveMinSpeed;
	} else if ( cg.predictedPlayerEntity.currentState.eFlags & 0x20 ) {
		minSpeed = (rfloat)cg_gun_move_minspeed.value + weaponInfo->duckedMoveMinSpeed;
	} else {
		minSpeed = (rfloat)cg_gun_move_minspeed.value + weaponInfo->standMoveMinSpeed;
	}

	if ( cg.xyspeed > minSpeed && ps->weaponstate != WEAPON_RELOADING ) {
		frac = ( cg.xyspeed - minSpeed ) / ( (rfloat)ps->speed - minSpeed );
		// keep: inverted test, matches original codegen
		if ( !( 0.0f > frac ) ) {
			if ( 1.0f < frac ) {
				frac = 1.0f;
			}
		} else {
			frac = 0.0f;
		}

		if ( cg.predictedPlayerEntity.currentState.eFlags & 0x40 ) {
			move[0] = (rfloat)frac * weaponInfo->proneMoveF;
			move[1] = (float)( frac * weaponInfo->proneMoveR );
			move[2] = (float)( frac * weaponInfo->proneMoveU );
		} else if ( cg.predictedPlayerEntity.currentState.eFlags & 0x20 ) {
			move[0] = (rfloat)frac * weaponInfo->duckedMoveF;
			move[1] = (float)( frac * weaponInfo->duckedMoveR );
			move[2] = (float)( frac * weaponInfo->duckedMoveU );
		} else {
			move[0] = (rfloat)frac * weaponInfo->standMoveF;
			move[1] = (float)( frac * weaponInfo->standMoveR );
			move[2] = (float)( frac * weaponInfo->standMoveU );
		}
		move[0] = (rfloat)cg_gun_move_f.value * frac + move[0];
		move[1] = (float)( (rfloat)cg_gun_move_r.value * frac + move[1] );
		move[2] = (float)( (rfloat)cg_gun_move_u.value * frac + move[2] );
	} else {
		VectorClear( move );
	}

	if ( ps->viewHeightTarget == ps->crouchViewHeight ) {
		move[0] += weaponInfo->duckedOfsF;
		move[1] = (float)( move[1] + weaponInfo->duckedOfsR );
		move[2] = (float)( move[2] + weaponInfo->duckedOfsU );
		move[0] += cg_gun_ofs_f.value;
		move[1] = (float)( move[1] + cg_gun_ofs_r.value );
		move[2] = (float)( move[2] + cg_gun_ofs_u.value );
	} else if ( ps->viewHeightTarget == ps->proneViewHeight ) {
		move[0] += weaponInfo->proneOfsF;
		move[1] = (float)( move[1] + weaponInfo->proneOfsR );
		move[2] = (float)( move[2] + weaponInfo->proneOfsU );
		move[0] += cg_gun_ofs_f.value;
		move[1] = (float)( move[1] + cg_gun_ofs_r.value );
		move[2] = (float)( move[2] + cg_gun_ofs_u.value );
	}

	CG_TRACKMOVEMENT( &cg.playerEntity.vLastMoveOrg[0], move[0], weaponInfo );
	CG_TRACKMOVEMENT( &cg.playerEntity.vLastMoveOrg[1], move[1], weaponInfo );
	CG_TRACKMOVEMENT( &cg.playerEntity.vLastMoveOrg[2], move[2], weaponInfo );

	if ( cg.predictedPlayerState.fWeaponPosFrac != 0.0f ) {
		if ( cg.predictedPlayerState.fWeaponPosFrac < 0.5f ) {
			adsScale = 1.0f - ( (rfloat)cg.predictedPlayerState.fWeaponPosFrac + cg.predictedPlayerState.fWeaponPosFrac );
			offset[0] += cg.playerEntity.vLastMoveOrg[0] * adsScale;
			offset[1] += cg.playerEntity.vLastMoveOrg[1] * adsScale;
			offset[2] += cg.playerEntity.vLastMoveOrg[2] * adsScale;
		}
	} else {
		offset[0] += cg.playerEntity.vLastMoveOrg[0];
		offset[1] += cg.playerEntity.vLastMoveOrg[1];
		offset[2] += cg.playerEntity.vLastMoveOrg[2];
	}
}

/*
==============
CG_CalculateWeaponPosition_BasePosition
==============
*/
static void CG_CalculateWeaponPosition_BasePosition( vec3_t offset ) {
	vec3_t move;

	VectorClear( move );
	CG_CalculateWeaponPosition_BasePosition_movement( move );

	VectorCopy( move, cg.playerEntity.vPositionLastOrg );

	offset[0] += move[0];
	offset[1] += move[1];
	offset[2] += move[2];
}

/*
==============
CG_CalculateWeaponPosition_ToWorldPosition

Turn a forward/right/up offset into a world point off the view origin.
==============
*/
static void CG_CalculateWeaponPosition_ToWorldPosition( vec3_t pos ) {
	vec3_t offset;
	vec3_t axis[3];     // forward, right, up

	VectorCopy( pos, offset );
	AngleVectors( cg.refdefViewAngles, axis[0], axis[1], axis[2] );

	VectorCopy( cg.refdef.vieworg, pos );
	VectorMA( pos, offset[0], axis[0], pos );
	VectorMA( pos, -offset[1], axis[1], pos );
	VectorMA( pos, offset[2], axis[2], pos );
}

/*
==============
CG_CalculateWeaponPosition_SaveOffsetMovement

Remember how far the weapon has drifted from the view origin while the sights
are up, scaled by the ADS fraction; CG_AddViewWeapon feeds it back to the
crosshair.
==============
*/
static void CG_CalculateWeaponPosition_SaveOffsetMovement( const vec3_t origin ) {
	rfloat frac;

	if ( BG_IsAimDownSightWeapon( cg.predictedPlayerState.weapon ) ) {
		frac = cg.predictedPlayerState.fWeaponPosFrac;
		if ( frac != 0.0f ) {
			cg.gunXOfs = ( origin[0] - cg.refdef.vieworg[0] ) * frac;
			cg.gunYOfs = ( origin[1] - cg.refdef.vieworg[1] ) * frac;
			cg.gunZOfs = ( origin[2] - cg.refdef.vieworg[2] ) * frac;
			return;
		}
	}

	cg.gunXOfs = 0.0f;
	cg.gunYOfs = 0.0f;
	cg.gunZOfs = 0.0f;
}

/*
==============
CG_CalculateWeaponPosition

The view weapon's world origin: lean, the weapon's own base position, sway, and
RTCW's landing drop.
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin ) {
	int delta;
	float frac;
	float scale;
	vec3_t angles;
	vec3_t right;

	VectorClear( origin );

	if ( cg.predictedPlayerState.leanf != 0.0f
		 && cg.predictedPlayerState.fWeaponPosFrac < 1.0f ) {
		// adjust 'lean' into weapon
		// keep: AddLeanToPosition written out, for the original rounding
		frac = GetLeanFraction( cg.predictedPlayerState.leanf );
		angles[0] = 0.0f;
		angles[1] = 0.0f;
		angles[2] = frac * -2.0f;
		scale = ( 1.0f - cg.predictedPlayerState.fWeaponPosFrac ) * frac * 1.6f;
		AngleVectors( angles, NULL, right, NULL );
		origin[0] += right[0] * scale;
		origin[1] += right[1] * scale;
		origin[2] += right[2] * scale;
	}

	CG_CalculateWeaponPosition_BasePosition( origin );
	CG_CalculateWeaponPosition_SwayMovement( origin );
	CG_CalculateWeaponPosition_ToWorldPosition( origin );

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < 150 ) {
		origin[2] += cg.landChange * 0.25f * (float)delta / 150.0f;
	} else if ( delta < 150 + 300 ) {
		origin[2] += cg.landChange * 0.25f * (float)( 150 + 300 - delta ) / 300.0f;
	}

	CG_CalculateWeaponPosition_SaveOffsetMovement( origin );
}

/*
==============
CG_CalculateWeaponPostion_PositionToADS

Latch which way the sights are travelling, but only on a transition that starts
from a settled position -- once the fraction is moving, the direction is kept.
==============
*/
static void CG_CalculateWeaponPostion_PositionToADS( void ) {
	qboolean goingUp;

	if ( !BG_IsAimDownSightWeapon( cg.predictedPlayerState.weapon ) ) {
		return;
	}

	if ( cg.predictedPlayerState.fWeaponPosFrac != 1.0f && cg.predictedPlayerState.fWeaponPosFrac != 0.0f
		 && ( cg.playerEntity.fLastWeaponPosFrac == 1.0f || cg.playerEntity.fLastWeaponPosFrac == 0.0f )
		 && cg.predictedPlayerState.fWeaponPosFrac != cg.playerEntity.fLastWeaponPosFrac ) {
		// keep: through a local, matches original codegen
		if ( cg.predictedPlayerState.fWeaponPosFrac >= cg.playerEntity.fLastWeaponPosFrac ) {
			goingUp = qtrue;
		} else {
			goingUp = qfalse;
		}
		cg.playerEntity.bPositionToADS = goingUp;
	}

	cg.playerEntity.fLastWeaponPosFrac = cg.predictedPlayerState.fWeaponPosFrac;
}

/*
=============================================================================

	MUZZLE FLASH AND THE WEAPON MODELS

=============================================================================
*/

/*
==============
CG_WeaponFlash

Play a weapon's flash effect off a bone of the DObj it is attached to.
`handle` is that DObj's Com_GetClientDObj slot: the entity number, or
CG_WeaponDObjHandle for the viewmodel.
==============
*/
static void CG_WeaponFlash( int handle, int weapon, const vec3_t origin, qboolean viewmodel,
							const char *tagName ) {
	int effect;
	int bolt[2];

	if ( viewmodel ) {
		effect = cg_weapons[ weapon ].viewFlashEffect;
	} else {
		effect = cg_weapons[ weapon ].worldFlashEffect;
	}
	if ( !effect ) {
		return;
	}

	bolt[0] = handle;
	bolt[1] = trap_FX_GetBoneIndex( handle, tagName );
	if ( bolt[1] < 0 ) {
		return;
	}

	trap_FX_PlayOrientedEffectID( effect, origin, NULL, bolt );
}

/*
==============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
Also caches the tag_brass position for the shell effect and fires any pending
muzzle flash.
==============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int drawGun ) {
	refEntity_t gun;
	cgWeaponInfo_t  *cgWeapon;
	qboolean firstPersonOwner;
	qboolean isViewWeapon;
	int partBits[4];
	int weaponNum;
	int boneIndex;
	const float *boneMatrix;
	float gunMatrix[12];
	float tagMatrix[16];

	if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
		 && cent->currentState.number == cg.snap->ps.clientNum ) {
		firstPersonOwner = qtrue;
	} else {
		firstPersonOwner = qfalse;
	}

	weaponNum = cent->currentState.weapon;
	isViewWeapon = ( ps != NULL );

	if ( isViewWeapon && cg.cameraMode ) {
		return;
	}
	if ( cent->currentState.eFlags & EF_NOVIEWWEAPON ) {
		return;
	}

	CG_RegisterWeapon( weaponNum );
	cgWeapon = &cg_weapons[ weaponNum ];

	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	if ( isViewWeapon ) {
		gun.shaderRGBA[0] = 255;
		gun.shaderRGBA[1] = 255;
		gun.shaderRGBA[2] = 255;
		gun.shaderRGBA[3] = 255;

		gun.dobj = cgWeapon->dobj;
		VectorCopy( ps->origin, gun.lightingOrigin );
		gun.lightingOrigin[2] += ps->viewHeightCurrent;
		gun.reType = RT_DOBJ;
		gun.renderfx = RF_PLAYERMODEL;
		AddLeanToPosition( gun.lightingOrigin, ps->viewangles[1], ps->leanf,
						   16.0f, 20.0f );

		trap_DObjUpdateClientInfo( cgWeapon->dobj, (float)cg.frametime * 0.001f );

		memset( partBits, 0xFF, sizeof( partBits ) );
		if ( !trap_DObjCreateSkelForBones( cgWeapon->dobj, partBits ) ) {
			trap_DObjCalcAnim( cgWeapon->dobj, partBits );
			trap_DObjCalcSkel( cgWeapon->dobj, partBits );
		}

		CG_ProcessWeaponNoteTracks();

		VectorCopy( parent->origin, gun.origin );
		AxisCopy( parent->axis, gun.axis );

		// keep: the x row's operand order matches original codegen
		gun.origin[0] += cg_gunX.value * cg.refdef.viewaxis[0][0];
		gun.origin[0] += cg_gunY.value * cg.refdef.viewaxis[1][0];
		gun.origin[0] += cg_gunZ.value * cg.refdef.viewaxis[2][0];
		gun.origin[1] += cg.refdef.viewaxis[0][1] * cg_gunX.value;
		gun.origin[1] += cg.refdef.viewaxis[1][1] * cg_gunY.value;
		gun.origin[1] += cg.refdef.viewaxis[2][1] * cg_gunZ.value;
		gun.origin[2] += cg.refdef.viewaxis[0][2] * cg_gunX.value;
		gun.origin[2] += cg.refdef.viewaxis[1][2] * cg_gunY.value;
		gun.origin[2] += cg.refdef.viewaxis[2][2] * cg_gunZ.value;

		if ( drawGun ) {
			trap_R_AddRefEntityToScene( &gun );
		}

		VectorCopy( gun.origin, cg.viewModelOrigin );
		VectorCopy( gun.axis[0], cg.viewModelAxis[0] );
		VectorCopy( gun.axis[1], cg.viewModelAxis[1] );
		VectorCopy( gun.axis[2], cg.viewModelAxis[2] );

		boneIndex = trap_DObjGetBoneIndex( cgWeapon->dobj, "tag_brass" );
		if ( boneIndex > -1 ) {
			boneMatrix = DOBJ_BONEMATRIX( trap_DObjGetMatrixArray( cgWeapon->dobj ), boneIndex );
			// the gun pose as nine axis floats followed by the origin
			gunMatrix[0] = gun.axis[0][0];
			gunMatrix[1] = gun.axis[0][1];
			gunMatrix[2] = gun.axis[0][2];
			gunMatrix[3] = gun.axis[1][0];
			gunMatrix[4] = gun.axis[1][1];
			gunMatrix[5] = gun.axis[1][2];
			gunMatrix[6] = gun.axis[2][0];
			gunMatrix[7] = gun.axis[2][1];
			gunMatrix[8] = gun.axis[2][2];
			gunMatrix[9] = gun.origin[0];
			gunMatrix[10] = gun.origin[1];
			gunMatrix[11] = gun.origin[2];
			DObjSkel2MatrixMultiply43( boneMatrix, gunMatrix, tagMatrix );
			cg_brassOrigin[0] = tagMatrix[12];
			cg_brassOrigin[1] = tagMatrix[13];
			cg_brassOrigin[2] = tagMatrix[14];
		}
	}

	if ( !cent->bMuzzleFlash ) {
		return;
	}
	if ( firstPersonOwner && !isViewWeapon ) {
		return;
	}
	cent->bMuzzleFlash = 0;
	if ( !drawGun ) {
		return;
	}

	if ( isViewWeapon ) {
		CG_WeaponFlash( CG_WeaponDObjHandle( weaponNum ), weaponNum,
						cg.viewModelOrigin, qtrue, cg_flashTags[0] );
	} else {
		CG_WeaponFlash( cent->currentState.number, weaponNum,
						cent->lerpOrigin, qfalse, cg_flashTags[0] );
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t hand;
	pmWeaponAngleState_t state;
	cgWeaponInfo_t      *cgWeapon;
	vec3_t angles;
	vec3_t viewAxis[3];
	vec3_t weaponAxis[3];
	float zoom;
	vec3_t gunOffset;
	qboolean drawGun;

	drawGun = qtrue;

	// no gun if in third person view or dead
	if ( ps->pm_type == 4 || ps->pm_type == 5 ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// allow the gun to be completely removed
	if ( cg_drawGun.integer != 2
		 && ( cg_drawGun.integer == 0 || CG_GetWeapReticleZoom( &zoom ) ) ) {
		drawGun = qfalse;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}
	if ( ps->eFlags & EF_NOVIEWWEAPON ) {
		return;
	}

	// keep: signed test, matches original codegen
	if ( (int)ps->weapon > 0 ) {
		CG_RegisterWeapon( ps->weapon );
		cgWeapon = &cg_weapons[ ps->weapon ];

		CG_CalculateWeaponPosition_Sway();
		CG_CalculateWeaponPostion_PositionToADS();

		memset( &hand, 0, sizeof( hand ) );

		// set up gun position
		CG_CalculateWeaponPosition( hand.origin );

		gunOffset[0] = cg_gunX.value;
		gunOffset[1] = cg_gunY.value;
		gunOffset[2] = cg_gunZ.value;
		VectorMA( hand.origin, gunOffset[0], cg.refdef.viewaxis[0], hand.origin );
		VectorMA( hand.origin, gunOffset[1], cg.refdef.viewaxis[1], hand.origin );
		VectorMA( hand.origin, gunOffset[2], cg.refdef.viewaxis[2], hand.origin );

		AnglesToAxis( cg.refdefViewAngles, viewAxis );

		state.ps = ps;
		state.speed = cg.xyspeed;
		state.frametime = (float)cg.frametime * 0.001f;
		state.moveOffset[0] = cg.playerEntity.vLastMoveAng[0];
		state.moveOffset[1] = cg.playerEntity.vLastMoveAng[1];
		state.moveOffset[2] = cg.playerEntity.vLastMoveAng[2];
		state.idleScale = cg.playerEntity.fLastIdleFactor;
		state.time = cg.time - ps->deltaTime;
		if ( cg.damageTime ) {
			state.viewKickStartTime = cg.damageTime - ps->deltaTime;
		} else {
			state.viewKickStartTime = 0;
		}
		state.viewKickPitch = cg.v_dmg_pitch;
		state.viewKickYaw = cg.v_dmg_roll;
		state.recoilPitch = cg.recoilPitch;
		state.recoilYaw = cg.recoilYaw;
		state.recoilRoll = cg.recoilRoll;
		state.recoilPitchSpeed = cg.recoilPitchSpeed;
		state.recoilYawSpeed = cg.recoilYawSpeed;
		state.recoilState = cg.recoilState;
		state.baseAngles[0] = cg.swayAngles[0];
		state.baseAngles[1] = cg.swayAngles[1];
		state.baseAngles[2] = cg.swayAngles[2];

		BG_CalculateWeaponAngles( &state, angles );

		AnglesToAxis( angles, weaponAxis );
		MatrixMultiply( weaponAxis, viewAxis, hand.axis );

		if ( bg_weaponInfo[ps->weapon]->aimDownSight && ps->fWeaponPosFrac != 0.0f ) {
			AxisToAngles( hand.axis, angles );
			cg.gunPitch = AngleNormalize360( angles[0] );
			cg.gunYaw = AngleNormalize360( angles[1] );
		} else {
			cg.gunPitch = cg.refdefViewAngles[0];
			cg.gunYaw = cg.refdefViewAngles[1];
		}

		cg.playerEntity.vLastMoveAng[0] = state.moveOffset[0];
		cg.playerEntity.vLastMoveAng[1] = state.moveOffset[1];
		cg.playerEntity.vLastMoveAng[2] = state.moveOffset[2];
		cg.playerEntity.fLastIdleFactor = state.idleScale;
		cg.recoilPitch = state.recoilPitch;
		cg.recoilYaw = state.recoilYaw;
		cg.recoilRoll = state.recoilRoll;
		cg.recoilPitchSpeed = state.recoilPitchSpeed;
		cg.recoilYawSpeed = state.recoilYawSpeed;
		cg.recoilState = state.recoilState;

		CG_WeaponRunXModelAnims( ps, cgWeapon );

		hand.renderfx = RF_VIEWMODEL;

		// add everything onto the hand
		CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, drawGun );
	} else {
		cg.gunPitch = cg.refdefViewAngles[0];
		cg.gunYaw = cg.refdefViewAngles[1];
		cg.gunXOfs = 0.0f;
		cg.gunYOfs = 0.0f;
		cg.gunZOfs = 0.0f;
	}
}

/*
=============================================================================

	WEAPON SELECTION

=============================================================================
*/

/*
==============
CG_DrawWeapSlotBinding

The key bound to "weaponslot <name>", drawn with a one-pixel drop shadow beside
a weapon-select entry.
==============
*/
static void CG_DrawWeapSlotBinding( int slot, const float *color, float x, float y ) {
	const char *binding;
	vec4_t textColor;

	textColor[3] = color[3];
	binding = BindingFromName( va( "weaponslot %s", BG_GetWeaponSlotNameForIndex( slot ) ) );

	textColor[0] = 0.0f;
	textColor[1] = 0.0f;
	textColor[2] = 0.0f;
	CG_DrawStringExt( x + 1.0f, y + 1.0f - 4.0f, binding, textColor, qfalse, qfalse,
					  6.0f, 12.0f, 0, qtrue );

	textColor[0] = 1.0f;
	textColor[1] = 1.0f;
	textColor[2] = 1.0f;
	CG_DrawStringExt( x, y - 4.0f, binding, textColor, qfalse, qfalse,
					  6.0f, 12.0f, 0, qtrue );

	trap_R_SetColor( color );
}

/*
==============
CG_DrawWeaponSelect

The weapon-select overlay: one icon per occupied slot, the selected slot grown
and lit, and its alternate weapons spread out beside it with a spin that decays
back to the selected one.
==============
*/
void CG_DrawWeaponSelect( void ) {
	const weaponInfo_t  *weaponInfo;
	const weaponInfo_t  *selectedInfo;
	const cgWeaponInfo_t    *cgWeapon;
	float       *color;
	struct {
		vec4_t  drawColor;
		float   positions[6];   // original size; a 7-weapon alt chain overruns it
	} hud;
	float x, width, height, size, pos;
	float halfWidth;
	float spinAbs;
	float spinSign, spinSign2;
	float iconSize, spinFrac, iconY;
	float scale;
	int msec;
	int selectedSlot;
	int alt;
	signed char slotWeapon;
	int numAlts;
	int slot;
	int index;

	msec = cg.time - cg.lastWeaponSelectDrawTime;
	cg.lastWeaponSelectDrawTime = cg.time;

	if ( cg.predictedPlayerState.pm_type >= 6 ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}

	Controls_GetConfig();

	hud.drawColor[0] = 0.5f;
	hud.drawColor[1] = 0.5f;
	hud.drawColor[2] = 0.5f;
	hud.drawColor[3] = color[3];
	trap_R_SetColor( hud.drawColor );

	selectedSlot = BG_IsPlayerWeaponInSlot( &cg.predictedPlayerState, cg_weaponSelect.integer, qtrue );

	x = 632.0f;
	for ( slot = 5; slot > 0; slot-- ) {
		slotWeapon = cg.predictedPlayerState.weaponslots[slot];

		if ( !slotWeapon
			 || !( cg.predictedPlayerState.weapons[slotWeapon >> 5] & ( 1 << ( slotWeapon & 31 ) ) ) ) {
			x -= 32.0f;
			cg.weaponSlotFade[slot] = 0.0f;
			x -= 2.0f;
		} else {
			weaponInfo = bg_weaponInfo[slotWeapon];
			cgWeapon = &cg_weapons[ slotWeapon ];

			if ( !cgWeapon->hudIcon ) {
				x -= 32.0f;
				CG_DrawPic( x, 8.0f, 32.0f, 32.0f, cgs.media.hudNoWeaponIcon );
				cg.weaponSlotFade[slot] = 0.0f;
				x -= 2.0f;
			} else {
				if ( selectedSlot != slot ) {
					if ( cg.weaponSlotFade[slot] > 0.0f && msec ) {
						cg.weaponSlotFade[slot] -= (float)msec * 0.01f;
						if ( cg.weaponSlotFade[slot] < 0.0f ) {
							cg.weaponSlotFade[slot] = 0.0f;
						}
					}
					height = ( cg.weaponSlotFade[slot] + 1.0f ) * 32.0f;
					width = height;
					if ( weaponInfo->wideListIcon ) {
						width = height + height;
					}
					x -= width;
					CG_DrawPic( x, 8.0f, width, height, cgWeapon->hudIcon );
				} else {
					// the selected slot
					if ( cg.weaponSlotFade[slot] < 1.0f && msec ) {
						cg.weaponSlotFade[slot] += (float)msec * 0.01f;
						if ( cg.weaponSlotFade[slot] > 1.0f ) {
							cg.weaponSlotFade[slot] = 1.0f;
						}
					}

					numAlts = 1;
					selectedInfo = bg_weaponInfo[cg_weaponSelect.integer];
					alt = selectedInfo->altWeaponIndex;
					while ( alt && alt != cg_weaponSelect.integer ) {
						alt = bg_weaponInfo[alt]->altWeaponIndex;
						numAlts++;
					}

					if ( numAlts == 1 ) {
						cg_weaponSelectSpin = 0.0f;
						cg_weaponSelectSpinSlot = selectedSlot;
						cg_weaponSelectSpinWeapon = cg_weaponSelect.integer;

						hud.drawColor[0] = 1.0f;
						hud.drawColor[1] = 1.0f;
						hud.drawColor[2] = 1.0f;
						trap_R_SetColor( hud.drawColor );

						iconSize = ( cg.weaponSlotFade[slot] + 1.0f ) * 32.0f;
						width = iconSize;
						if ( weaponInfo->wideListIcon ) {
							width = iconSize + iconSize;
						}
						CG_DrawPic( x - width, 8.0f, width, iconSize, cgWeapon->hudIcon );
						x -= width;

						hud.drawColor[0] = 0.5f;
						hud.drawColor[1] = 0.5f;
						hud.drawColor[2] = 0.5f;
						trap_R_SetColor( hud.drawColor );
					} else {
						// more than one weapon on the alt chain: arm and decay the spin
						if ( cg_weaponSelectSpinSlot != selectedSlot ) {
							cg_weaponSelectSpinSlot = selectedSlot;
							cg_weaponSelectSpinWeapon = cg_weaponSelect.integer;
							cg_weaponSelectSpin = 0.0f;
						} else {
							if ( cg_weaponSelectSpinWeapon != cg_weaponSelect.integer ) {
								if ( bg_weaponInfo[cg_weaponSelectSpinWeapon]->altWeaponIndex == cg_weaponSelect.integer ) {
									cg_weaponSelectSpin = 1.0f;
								} else {
									cg_weaponSelectSpin = -1.0f;
								}
								cg_weaponSelectSpinWeapon = cg_weaponSelect.integer;
							}
							if ( cg_weaponSelectSpin != 0.0f ) {
								if ( cg_weaponSelectSpin < 0.0f ) {
									cg_weaponSelectSpin += (float)msec * 0.0066666668f;
									if ( cg_weaponSelectSpin > 0.0f ) {
										cg_weaponSelectSpin = 0.0f;
									}
								} else {
									cg_weaponSelectSpin -= (float)msec * 0.0066666668f;
									if ( cg_weaponSelectSpin < 0.0f ) {
										cg_weaponSelectSpin = 0.0f;
									}
								}
							}
						}

						// original bug, kept: with a negative spin the selected-icon
						// lookup below reads positions[-1], which the struct pins to
						// drawColor[3]
						height = ( cg.weaponSlotFade[slot] + 1.0f ) * 32.0f;
						halfWidth = height;
						// keep: positions[0] set in both arms (original codegen)
						if ( weaponInfo->wideListIcon ) {
							hud.positions[0] = 8.0f;
							halfWidth = height + height;
						} else {
							hud.positions[0] = 8.0f;
						}
						// the slot's full width, kept for the end
						size = halfWidth;
						hud.positions[1] = height + 10.0f;
						for ( index = 2; index < numAlts; index++ ) {
							hud.positions[index] = hud.positions[index - 1] + 34.0f;
						}

						halfWidth *= -0.5f;

						// the alternates, walking the chain forwards from the selected one
						alt = selectedInfo->altWeaponIndex;
						while ( alt && alt != cg_weaponSelect.integer ) {
							const weaponInfo_t  *altInfo = bg_weaponInfo[alt];
							const cgWeaponInfo_t    *altWeapon = &cg_weapons[ alt ];
							float iconWidth;

							height = 32.0f;
							iconWidth = height;
							if ( altInfo->wideListIcon ) {
								iconWidth = height * 2.0f;
							}

							if ( cg_weaponSelectSpin != 0.0f ) {
								spinAbs = (float)fabs( cg_weaponSelectSpin );
								// keep: sign bit read through a stored copy (original codegen)
								spinSign = cg_weaponSelectSpin;
								index = ( 2 - ( *(int *)&spinSign < 0 ) * 2 ) % numAlts;
								pos = hud.positions[1] - ( hud.positions[1] - hud.positions[index] ) * spinAbs;
								if ( cg_weaponSelectSpin > 0.0f
									 || ( numAlts - 1 == 1 && cg_weaponSelectSpin < 0.0f ) ) {
									scale = cg.weaponSlotFade[slot] * spinAbs + 1.0f;
									iconWidth *= scale;
									height *= scale;
								}
							} else {
								pos = hud.positions[1];
							}

							CG_DrawPic( halfWidth + x - iconWidth * 0.5f, pos, iconWidth, height, altWeapon->hudIcon );

							alt = altInfo->altWeaponIndex;
						}

						// and the selected weapon itself, on top
						hud.drawColor[0] = 1.0f;
						hud.drawColor[1] = 1.0f;
						hud.drawColor[2] = 1.0f;
						trap_R_SetColor( hud.drawColor );

						height = 32.0f;
						width = height;
						if ( weaponInfo->wideListIcon ) {
							width = height * 2.0f;
						}
						if ( cg_weaponSelectSpin != 0.0f ) {
							spinFrac = (float)fabs( cg_weaponSelectSpin );
							spinSign2 = cg_weaponSelectSpin;
							index = ( 1 - ( *(int *)&spinSign2 < 0 ) * 2 ) % numAlts;
							iconY = hud.positions[0] - ( hud.positions[0] - hud.positions[index] ) * spinFrac;
							scale = ( 1.0f - spinFrac ) * cg.weaponSlotFade[slot] + 1.0f;
							width *= scale;
							height *= scale;
						} else {
							iconY = hud.positions[0];
							scale = cg.weaponSlotFade[slot] + 1.0f;
							width *= scale;
							height *= scale;
						}
						CG_DrawPic( halfWidth + x - width * 0.5f, iconY, width, height, cgWeapon->hudIcon );
						x -= size;

						hud.drawColor[0] = 0.5f;
						hud.drawColor[1] = 0.5f;
						hud.drawColor[2] = 0.5f;
						trap_R_SetColor( hud.drawColor );
					}
				}
				CG_DrawWeapSlotBinding( slot, hud.drawColor, x, 8.0f );
				x -= 2.0f;
			}
		}
	}
}

/*
==============
CG_WeaponSelectable
==============
*/
static qboolean CG_WeaponSelectable( int weapon ) {
	// keep: if/return form matches original codegen
	if ( !( cg.predictedPlayerState.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
==============
CG_FinishWeaponChange

Hand the choice to the engine through cg_weaponSelect, and drop out of walk
mode unless this is a switch to the same weapon's alternate.
==============
*/
void CG_FinishWeaponChange( int oldWeapon, int newWeapon ) {
	qboolean isAlt;

	cg.weaponSelectTime = cg.time;  // flash the weapon icon
	if ( oldWeapon == newWeapon ) {   // no need to do any more than flash the icon
		return;
	}

	if ( newWeapon && newWeapon == bg_weaponInfo[oldWeapon]->altWeaponIndex ) {
		isAlt = qtrue;
	} else {
		isAlt = qfalse;
	}

	trap_Cvar_Set( "cg_weaponSelect", va( "%i", newWeapon ) );

	if ( !isAlt ) {
		trap_Cvar_Set( "cl_run", "1" );
	}
}

/*
==============
CG_AltWeapon_f

Switch to the selected weapon's alternate.
==============
*/
void CG_AltWeapon_f( void ) {
	int alt;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.predictedPlayerState.pm_flags & 0x4000 ) {
		return;
	}
	if ( !( cg.snap->ps.pm_flags & PMF_WEAPONSELECTABLE ) ) {
		return;
	}
	if ( cg.predictedPlayerState.eFlags & EF_WEAPONLOCKED ) {
		return;
	}
	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast
	}

	alt = bg_weaponInfo[cg_weaponSelect.integer]->altWeaponIndex;
	if ( !alt ) {
		CG_GameMessage( trap_SE_LocalizeMessage(
							"CGAME_THIS_WEAPON_HAS_NO_ALTERNATE", "game message" ) );
		return;
	}

	if ( CG_WeaponSelectable( alt ) ) {
		CG_FinishWeaponChange( cg_weaponSelect.integer, alt );
	}
}

/*
==============
CG_SelectWeaponIndex

Select a weapon by index.  Unreferenced.
==============
*/
void CG_SelectWeaponIndex( int weapon ) {
	CG_FinishWeaponChange( cg_weaponSelect.integer, weapon );
}

/*
==============
CG_NextWeapon_f

Doubles as the scoreboard scroll-up when the scoreboard has focus.
==============
*/
void CG_NextWeapon_f( void ) {
	if ( cg.showScores ) {
		if ( cg.scoreboardScrollPos <= 0 ) {
			return;
		}
		cg.scoreboardScrollPos -= cg_scoreboardScrollStep.integer;
		if ( cg.scoreboardScrollPos < 0 ) {
			cg.scoreboardScrollPos = 0;
		}
		return;
	}

	if ( !cg.snap ) {
		return;
	}
	if ( cg.predictedPlayerState.pm_flags & 0x4000 ) {
		return;
	}
	if ( !( cg.snap->ps.pm_flags & PMF_WEAPONSELECTABLE ) ) {
		return;
	}
	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon
	CG_CycleWeap( qtrue, qfalse );
}

/*
==============
CG_PrevWeapon_f
==============
*/
void CG_PrevWeapon_f( void ) {
	if ( cg.showScores ) {
		if ( !cg.scoreboardListFull ) {
			return;
		}
		cg.scoreboardScrollPos += cg_scoreboardScrollStep.integer;
		if ( cg.scoreboardScrollPos > cg.numScores - 1 ) {
			cg.scoreboardScrollPos = cg.numScores - 1;
		}
		return;
	}

	if ( !cg.snap ) {
		return;
	}
	if ( cg.predictedPlayerState.pm_flags & 0x4000 ) {
		return;
	}
	if ( !( cg.snap->ps.pm_flags & PMF_WEAPONSELECTABLE ) ) {
		return;
	}
	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon
	CG_CycleWeap( qfalse, qfalse );
}

/*
==============
CG_Weapon_f

"weapon <name|index>"
==============
*/
void CG_Weapon_f( void ) {
	int weapon;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.predictedPlayerState.pm_flags & 0x4000 ) {
		return;
	}
	if ( !( cg.snap->ps.pm_flags & PMF_WEAPONSELECTABLE ) ) {
		return;
	}

	weapon = (unsigned char)BG_GetWeaponIndexForName( CG_Argv( 1 ) );
	if ( !weapon ) {
		weapon = atoi( CG_Argv( 1 ) );
		if ( !weapon ) {
			return;
		}
	}

	CG_FinishWeaponChange( cg_weaponSelect.integer, weapon );
}

/*
==============
CG_OutOfAmmoChange

The current weapon has just run out of ammo
Prefer another weapon of the same slot when the slot stacks; otherwise walk the
five slots for anything with ammo; failing both, hand it to CG_CycleWeap.
==============
*/
void CG_OutOfAmmoChange( void ) {
	const weaponInfo_t  *weaponInfo;
	int slots[5];
	int i;
	weaponInfo_t **weaponTable;
	signed char weapon;
	int weaponIndex;

	if ( !cg.snap ) {
		return;
	}

	weaponTable = bg_weaponInfo;
	weaponInfo = cg.weaponInfo;

	if ( weaponInfo->slotStackable ) {
		for ( i = 1; i <= bg_numWeapons; i++ ) {
			if ( !CG_WeaponSelectable( i ) ) {
				continue;
			}
			if ( !weaponTable[i]->slotStackable ) {
				continue;
			}
			if ( weaponTable[i]->weaponSlot != weaponInfo->weaponSlot ) {
				continue;
			}
			if ( cg.predictedPlayerState.ammoclip[weaponTable[i]->clipIndex]
				 + cg.predictedPlayerState.ammo[weaponTable[i]->ammoIndex] ) {
				CG_FinishWeaponChange( cg_weaponSelect.integer, i );
				return;
			}
		}
	}

	if ( BG_IsPlayerWeaponInSlot( &cg.predictedPlayerState, weaponInfo->weapIndex, qtrue ) ) {
		// keep: a local array of slot numbers, matches original codegen
		slots[0] = 1;
		slots[1] = 2;
		slots[2] = 3;
		slots[3] = 4;
		slots[4] = 5;

		for ( i = 0; i < sizeof( slots ) / sizeof( slots[0] ); i++ ) {
			weapon = (signed char)cg.predictedPlayerState.weaponslots[slots[i]];
			if ( !weapon ) {
				continue;
			}
			weaponIndex = weapon;
			if ( cg.predictedPlayerState.ammoclip[weaponTable[weaponIndex]->clipIndex]
				 + cg.predictedPlayerState.ammo[weaponTable[weaponIndex]->ammoIndex] ) {
				CG_FinishWeaponChange( cg_weaponSelect.integer,
									   (signed char)cg.predictedPlayerState.weaponslots[slots[i]] );
				return;
			}
		}
	}

	CG_CycleWeap( qtrue, qtrue );
}

/*
==============
CG_WeaponSlot_f

"weaponslot <name|index>"
==============
*/
void CG_WeaponSlot_f( void ) {
	int slot;
	int weapon;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.predictedPlayerState.pm_flags & 0x4000 ) {
		return;
	}
	if ( !( cg.snap->ps.pm_flags & PMF_WEAPONSELECTABLE ) ) {
		return;
	}
	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast
	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	slot = BG_GetWeaponSlotForName( CG_Argv( 1 ) );
	if ( !slot ) {
		slot = atoi( CG_Argv( 1 ) );
	}
	if ( slot <= 0 || slot >= WEAPSLOT_NUM ) {
		return;
	}

	weapon = (signed char)cg.predictedPlayerState.weaponslots[slot];
	if ( !weapon ) {
		return;
	}
	if ( !CG_WeaponSelectable( weapon ) ) {
		return;
	}

	CG_FinishWeaponChange( cg_weaponSelect.integer, weapon );
}

/*
==============
CG_SelectFirstWeaponInSlot

Walk the six slots in `next`'s direction and select the first one holding a
weapon; with checkAmmo set, one that still has ammunition.
==============
*/
static qboolean CG_SelectFirstWeaponInSlot( qboolean next, qboolean checkAmmo ) {
	int step;
	weaponInfo_t **weaponTable;
	int slot;
	signed char weapon;
	int index;

	if ( next ) {
		step = 1;
		slot = 1;
	} else {
		step = -1;
		slot = 5;
	}

	weaponTable = bg_weaponInfo;

	for ( ;; ) {
		weapon = cg.predictedPlayerState.weaponslots[slot];
		if ( weapon ) {
			if ( !checkAmmo ) {
				break;
			}
			index = weapon;   // keep: matches original codegen
			if ( cg.predictedPlayerState.ammoclip[weaponTable[index]->clipIndex]
				 + cg.predictedPlayerState.ammo[weaponTable[index]->ammoIndex] ) {
				break;
			}
		}
		slot += step;
		if ( !slot || slot == WEAPSLOT_NUM ) {
			return qfalse;
		}
	}

	CG_FinishWeaponChange( cg_weaponSelect.integer,
						   (signed char)cg.predictedPlayerState.weaponslots[slot] );
	return qtrue;
}

/*
==============
CG_SelectFirstWeaponNotInSlot

The fallback when nothing in the slot bar will do: walk every weapon the player
owns and take the first one that is not already occupying a slot -- unless it
is slot-stackable and a stack slot is still open for it (its class's slot, or
either primary slot, is empty or holds a stackable weapon).
==============
*/
static qboolean CG_SelectFirstWeaponNotInSlot( qboolean next, qboolean checkAmmo ) {
	int weapon;
	int step;
	int first;
	int stackSlot;
	signed char primaryB;
	signed char occupant;

	if ( next ) {
		first = 1;
		step = 1;
	} else {
		first = bg_numWeapons;
		step = -1;
	}

	primaryB = cg.predictedPlayerState.weaponslots[WEAPSLOT_PRIMARYB];

	weapon = first;

	// keep: the gotos, the separate pistol case and the never-taken jump back
	// to inslot all match original codegen
	for ( ;; ) {
		if ( !( cg.predictedPlayerState.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) ) {
			goto advance;
		}
		{
		inslot:
			if ( BG_IsPlayerWeaponInSlot( &cg.predictedPlayerState, weapon, qtrue ) ) {
				goto advance;
			}

			// BG_GetStackSlotForWeapon, expanded
			stackSlot = WEAPSLOT_NONE;
			if ( bg_weaponInfo[weapon]->slotStackable ) {
				switch ( bg_weaponInfo[weapon]->weaponSlot ) {
				case WEAPSLOT_PRIMARY:
				case WEAPSLOT_PRIMARYB:
					occupant = cg.predictedPlayerState.weaponslots[WEAPSLOT_PRIMARY];
					if ( !occupant || bg_weaponInfo[occupant]->slotStackable ) {
						goto advance;
					}
					occupant = primaryB;
					if ( !occupant || bg_weaponInfo[occupant]->slotStackable ) {
						goto advance;
					}
					break;
				case WEAPSLOT_PISTOL:
					occupant = cg.predictedPlayerState.weaponslots[bg_weaponInfo[weapon]->weaponSlot];
					if ( !occupant || bg_weaponInfo[occupant]->slotStackable ) {
						stackSlot = bg_weaponInfo[weapon]->weaponSlot;
					}
					break;
				case WEAPSLOT_GRENADE:
				case WEAPSLOT_SMOKEGRENADE:
					occupant = cg.predictedPlayerState.weaponslots[bg_weaponInfo[weapon]->weaponSlot];
					if ( !occupant || bg_weaponInfo[occupant]->slotStackable ) {
						stackSlot = bg_weaponInfo[weapon]->weaponSlot;
					}
					break;
				default:
					break;
				}
			}
			if ( stackSlot ) {
				goto advance;
			}
		}

		if ( checkAmmo
			 && !( cg.predictedPlayerState.ammoclip[bg_weaponInfo[weapon]->clipIndex]
				   + cg.predictedPlayerState.ammo[bg_weaponInfo[weapon]->ammoIndex] ) ) {
			if ( stackSlot ) {
				goto inslot;    // never taken: stackSlot is WEAPSLOT_NONE here
			}
			// original bug, kept: no advance, so an owned, unslotted, empty
			// candidate is re-tested forever and the client hangs
			continue;
		}

		CG_FinishWeaponChange( cg_weaponSelect.integer, weapon );
		return qtrue;

	advance:
		weapon += step;
		if ( !weapon || weapon == bg_numWeapons ) {
			return qfalse;
		}
	}
}

// the weapon in a slot; weaponslots[] holds signed bytes
#define CG_SLOTWEAPON( slot )   ( (signed char)cg.predictedPlayerState.weaponslots[slot] )

/*
==============
CG_CycleWeap

The weapon cycle behind "weapnext" / "weapprev".  If the current weapon sits in
a slot, step round the slot bar; otherwise step round the weapon list itself,
skipping every weapon that is only an alternate of one already there.
==============
*/
void CG_CycleWeap( qboolean next, qboolean checkAmmo ) {
	const weaponInfo_t  *weaponInfo;
	int slot, endSlot, step, endWeapon;
	int weapon;
	int alt;
	qboolean isAlt;
	int stackSlot;

	if ( !cg.snap ) {
		return;
	}
	if ( !( cg.snap->ps.pm_flags & PMF_WEAPONSELECTABLE ) ) {
		return;
	}

	if ( next ) {
		step = 1;
		endSlot = 1;
		endWeapon = 1;
	} else {
		step = -1;
		endSlot = 5;
		endWeapon = bg_numWeapons;
	}

	slot = BG_IsPlayerWeaponInSlot( &cg.predictedPlayerState, cg_weaponSelect.integer, qtrue );
	if ( !slot ) {
		slot = BG_GetStackSlotForWeapon( &cg.predictedPlayerState, cg_weaponSelect.integer, WEAPSLOT_NONE );
	}

	if ( slot ) {
		slot = ( slot + step + 4 ) % 5 + 1;
		while ( slot != endSlot ) {
			if ( CG_SLOTWEAPON( slot ) ) {
				if ( !checkAmmo
					 || cg.predictedPlayerState.ammoclip[bg_weaponInfo[CG_SLOTWEAPON( slot )]->clipIndex]
						+ cg.predictedPlayerState.ammo[bg_weaponInfo[CG_SLOTWEAPON( slot )]->ammoIndex] ) {
					CG_FinishWeaponChange( cg_weaponSelect.integer, CG_SLOTWEAPON( slot ) );
					return;
				}
			}
			slot = ( slot + step + 4 ) % 5 + 1;
		}

		if ( CG_SelectFirstWeaponNotInSlot( next, checkAmmo ) ) {
			return;
		}
		CG_SelectFirstWeaponInSlot( next, checkAmmo );
	} else {
		weapon = cg_weaponSelect.integer;
		while ( 1 ) {
			weapon = ( bg_numWeapons + step - 1 + weapon ) % bg_numWeapons + 1;
			if ( weapon == endWeapon ) {
				break;
			}
			if ( !( cg.predictedPlayerState.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) ) {
				continue;
			}

			weaponInfo = bg_weaponInfo[weapon];

			// skip a weapon that is only an alternate of the one selected
			isAlt = qfalse;
			for ( alt = weaponInfo->altWeaponIndex; alt; alt = bg_weaponInfo[alt]->altWeaponIndex ) {
				if ( alt == cg_weaponSelect.integer ) {
					isAlt = qtrue;
					break;
				}
				if ( alt == weapon ) {
					break;
				}
			}
			if ( isAlt ) {
				continue;
			}

			if ( BG_IsPlayerWeaponInSlot( &cg.predictedPlayerState, weapon, qtrue ) ) {
				continue;
			}

			// BG_GetStackSlotForWeapon( ps, weapon, WEAPSLOT_NONE ), expanded
			stackSlot = WEAPSLOT_NONE;
			if ( weaponInfo->slotStackable ) {
				switch ( weaponInfo->weaponSlot ) {
				case WEAPSLOT_PRIMARY:
				case WEAPSLOT_PRIMARYB:
					if ( !CG_SLOTWEAPON( WEAPSLOT_PRIMARY )
						 || bg_weaponInfo[CG_SLOTWEAPON( WEAPSLOT_PRIMARY )]->slotStackable ) {
						stackSlot = WEAPSLOT_PRIMARY;
					} else if ( !CG_SLOTWEAPON( WEAPSLOT_PRIMARYB )
								|| bg_weaponInfo[CG_SLOTWEAPON( WEAPSLOT_PRIMARYB )]->slotStackable ) {
						stackSlot = WEAPSLOT_PRIMARYB;
					}
					break;
				case WEAPSLOT_PISTOL:
				case WEAPSLOT_GRENADE:
				case WEAPSLOT_SMOKEGRENADE:
					if ( !CG_SLOTWEAPON( weaponInfo->weaponSlot )
						 || bg_weaponInfo[CG_SLOTWEAPON( weaponInfo->weaponSlot )]->slotStackable ) {
						stackSlot = weaponInfo->weaponSlot;
					}
					break;
				default:
					break;
				}
			}
			if ( stackSlot ) {
				continue;
			}

			if ( checkAmmo
				 && !( cg.predictedPlayerState.ammoclip[weaponInfo->clipIndex]
					   + cg.predictedPlayerState.ammo[weaponInfo->ammoIndex] ) ) {
				continue;
			}

			CG_FinishWeaponChange( cg_weaponSelect.integer, weapon );
			return;
		}

		if ( CG_SelectFirstWeaponInSlot( next, checkAmmo ) ) {
			return;
		}
		CG_SelectFirstWeaponNotInSlot( next, checkAmmo );
	}

	// nothing selectable is left: clear the selection outright
	weapon = cg_weaponSelect.integer;
	if ( cg.predictedPlayerState.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		return;
	}
	cg.weaponSelectTime = cg.time;
	if ( !weapon ) {
		return;
	}
	trap_Cvar_Set( "cg_weaponSelect", va( "%i", 0 ) );
	trap_Cvar_Set( "cl_run", "1" );
}

/*
=============================================================================

	WEAPON EVENTS

=============================================================================
*/

/*
==============
CG_WeaponFireRecoil

One shot's worth of view kick and gun kick, each a random draw between the
weapon's min and max for the stance the sights are in.
==============
*/
static void CG_WeaponFireRecoil( void ) {
	vec3_t kick;
	float pitch;
	rfloat yaw;
	float frac;

	frac = cg.predictedPlayerState.fWeaponPosFrac;

	if ( frac == 1.0f ) {
		pitch = ( (float)rand() * ( 1.0f / 32768.0f ) )
				* ( cg.weaponInfo->adsViewKickPitchMax - cg.weaponInfo->adsViewKickPitchMin )
				+ cg.weaponInfo->adsViewKickPitchMin;
		yaw = ( (float)rand() * ( 1.0f / 32768.0f ) )
			  * ( cg.weaponInfo->adsViewKickYawMax - cg.weaponInfo->adsViewKickYawMin )
			  + cg.weaponInfo->adsViewKickYawMin;
	} else {
		pitch = ( (float)rand() * ( 1.0f / 32768.0f ) )
				* ( cg.weaponInfo->hipViewKickPitchMax - cg.weaponInfo->hipViewKickPitchMin )
				+ cg.weaponInfo->hipViewKickPitchMin;
		yaw = ( (float)rand() * ( 1.0f / 32768.0f ) )
			  * ( cg.weaponInfo->hipViewKickYawMax - cg.weaponInfo->hipViewKickYawMin )
			  + cg.weaponInfo->hipViewKickYawMin;
	}

	// keep: built in a local, matches original codegen
	kick[0] = -pitch;
	kick[1] = yaw;
	kick[2] = yaw * -0.5f;
	VectorCopy( kick, cg.kick_angles );

	if ( frac > 0.0f ) {
		pitch = ( (float)rand() * ( 1.0f / 32768.0f ) )
				* ( cg.weaponInfo->adsGunKickPitchMax - cg.weaponInfo->adsGunKickPitchMin )
				+ cg.weaponInfo->adsGunKickPitchMin;
		yaw = ( (rfloat)( (float)rand() * ( 1.0f / 32768.0f ) )
				* ( (rfloat)cg.weaponInfo->adsGunKickYawMax - (rfloat)cg.weaponInfo->adsGunKickYawMin ) )
			  + (rfloat)cg.weaponInfo->adsGunKickYawMin;
	} else {
		pitch = ( (float)rand() * ( 1.0f / 32768.0f ) )
				* ( cg.weaponInfo->hipGunKickPitchMax - cg.weaponInfo->hipGunKickPitchMin )
				+ cg.weaponInfo->hipGunKickPitchMin;
		yaw = ( (rfloat)( (float)rand() * ( 1.0f / 32768.0f ) )
				* ( (rfloat)cg.weaponInfo->hipGunKickYawMax - (rfloat)cg.weaponInfo->hipGunKickYawMin ) )
			  + (rfloat)cg.weaponInfo->hipGunKickYawMin;
	}

	// keep: the gun yaw kick is rounded only after the sum, and the
	// parenthesised products above match original codegen
	cg.recoilPitchSpeed = cg.recoilPitchSpeed + pitch;
	cg.recoilYawSpeed = cg.recoilYawSpeed + yaw;
}

/*
==============
CG_EjectWeaponBrass

Play the weapon's shell eject effect off tag_brass.
==============
*/
void CG_EjectWeaponBrass( entityState_t *es, int event ) {
	const cgWeaponInfo_t    *cgWeapon;
	int bolt[2];
	int weapon;

	if ( !cg_brass.integer ) {
		return;
	}
	if ( es->eType >= ET_EVENTS ) {
		return;
	}

	weapon = es->weapon;
	if ( !weapon ) {
		return;
	}
	if ( weapon > bg_numWeapons ) {
		CG_Error( "CG_FireWeapon: ent->weapon > BG_GetNumWeapons()" );
		return;
	}

	cgWeapon = &cg_weapons[ weapon ];

	if ( cgWeapon->lastShotEjectEffect && event == EV_FIRE_WEAPON_LASTSHOT ) {
		if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
			 && es->number == cg.snap->ps.clientNum ) {
			bolt[0] = CG_WeaponDObjHandle( weapon );
		} else {
			bolt[0] = es->number;
		}
		bolt[1] = trap_FX_GetBoneIndex( bolt[0], "tag_brass" );
		if ( bolt[1] < 0 ) {
			return;
		}
		trap_FX_PlayOrientedEffectID( cgWeapon->lastShotEjectEffect, cg_brassOrigin, NULL, bolt );
	} else if ( cgWeapon->shellEjectEffect ) {
		if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
			 && es->number == cg.snap->ps.clientNum ) {
			bolt[0] = CG_WeaponDObjHandle( weapon );
		} else {
			bolt[0] = es->number;
		}
		bolt[1] = trap_FX_GetBoneIndex( bolt[0], "tag_brass" );
		if ( bolt[1] < 0 ) {
			return;
		}
		trap_FX_PlayOrientedEffectID( cgWeapon->shellEjectEffect, cg_brassOrigin, NULL, bolt );
	}
}

/*
==============
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event.
==============
*/
void CG_FireWeapon( centity_t *cent, entityState_t *es, int event, int tagIndex ) {
	const cgWeaponInfo_t    *cgWeapon;
	const char *sound;
	vec3_t origin;
	float tagMatrix[16];
	DObj *obj;

	if ( !es->weapon ) {
		return;
	}
	if ( es->weapon > bg_numWeapons ) {
		CG_Error( "CG_FireWeapon: ent->weapon > BG_GetNumWeapons()" );
		return;
	}

	cgWeapon = &cg_weapons[ es->weapon ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->bMuzzleFlash = 1;

	if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
		 && es->number == cg.snap->ps.clientNum ) {
		CG_WeaponFireRecoil();
	}

	if ( es->eType == ET_TURRET ) {
		CG_WeaponFlash( es->number, es->weapon, cent->lerpOrigin, qfalse,
						cg_flashTags[tagIndex] );
	}

	sound = cgWeapon->fireSound;
	if ( cgWeapon->lastShotSound && event == EV_FIRE_WEAPON_LASTSHOT ) {
		sound = cgWeapon->lastShotSound;
	}

	if ( sound ) {
		if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
			 && es->number == cg.snap->ps.clientNum ) {
			if ( cgWeapon->dobj
				 && CG_DObjGetViewModelTagMatrix( cgWeapon->dobj, "tag_flash", tagMatrix ) ) {
				origin[0] = tagMatrix[12];
				origin[1] = tagMatrix[13];
				origin[2] = tagMatrix[14];
			} else {
				BG_EvaluateTrajectory( &es->pos, cg.time, origin );
			}
		} else if ( ( obj = trap_GetClientDObj( es->number ) ) != 0
					&& CG_DObjGetWorldTagMatrix( obj, "tag_flash", cent, tagMatrix ) ) {
			origin[0] = tagMatrix[12];
			origin[1] = tagMatrix[13];
			origin[2] = tagMatrix[14];
		} else {
			BG_EvaluateTrajectory( &es->pos, cg.time, origin );
		}
		CG_PlaySoundAliasByName( sound, es->number, origin );
	}

	if ( !bg_weaponInfo[es->weapon]->boltAction ) {
		CG_EjectWeaponBrass( es, event );
	}
}

/*
=============================================================================

	BULLETS

=============================================================================
*/

/*
==============
CG_WaterRipple

Unreferenced; dir is unused.
==============
*/
void CG_WaterRipple( qhandle_t shader, const vec3_t origin, const vec3_t dir, int radius, int duration ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_SCALE_FADE;
	le->leFlags = 1;
	le->endTime = cg.time + duration;
	le->lifeRate = 1.0f / (float)duration;

	VectorCopy( origin, re->origin );
	re->shaderTime = (float)cg.time * 0.001f;
	re->reType = RT_SPRITE_LOCAL;
	re->radius = (float)radius;
	re->customShader = shader;
	re->shaderRGBA[0] = 255;
	re->shaderRGBA[1] = 255;
	re->shaderRGBA[2] = 255;
	re->shaderRGBA[3] = 255;
	le->color[3] = 1.0f;
}

/*
==============
CG_WhizbySound

A bullet that passes close to the listener gets a whizby, played 16 units back
along its path from the point of closest approach.
==============
*/
static void CG_WhizbySound( const vec3_t end, const vec3_t start ) {
	vec3_t delta;
	vec3_t normal;
	vec3_t dir;
	vec3_t toView;
	vec3_t point;
	vec3_t diff;
	float along;
	float dist;

	VectorSubtract( end, start, delta );
	VectorNormalize2( delta, normal );
	VectorCopy( normal, dir );  // keep: matches original codegen
	VectorSubtract( cg.refdef.vieworg, start, toView );
	along = DotProduct( toView, dir );
	if ( along < 64.0f ) {
		return;
	}

	// the point of closest approach has to be far enough inside the trace
	// (keep: subtracting -64, as the original does)
	if ( along - -64.0f > DotProduct( dir, delta ) ) {
		return;
	}

	point[0] = along * dir[0] + start[0];
	point[1] = along * dir[1] + start[1];
	point[2] = along * dir[2] + start[2];

	VectorSubtract( point, cg.refdef.vieworg, diff );
	dist = (float)sqrt( DotProduct( diff, diff ) );
	if ( dist > 140.0f ) {
		return;
	}

	point[0] = point[0] - dir[0] * 16.0f;
	point[1] = point[1] - dir[1] * 16.0f;
	point[2] = point[2] - dir[2] * 16.0f;

	CG_PlaySoundAliasByName( (const char *)cgs.media.whizbySound, ENTITYNUM_WORLD, point );
}

/*
==============
CG_SpawnTracer

A tracer that has to travel: a moving-tracer local entity that starts
cg_tracerlength behind the impact and reaches it at cg_tracerSpeed.
==============
*/
static void CG_SpawnTracer( const vec3_t start, const vec3_t end ) {
	localEntity_t   *le;
	vec3_t origin;
	vec3_t tail;
	vec3_t dir;
	float len;
	float dist;
	int offset;
	int spawnTime;

	VectorCopy( start, origin );
	VectorCopy( end, tail );
	VectorSubtract( tail, origin, dir );
	VectorNormalize( dir );

	// subtract the length of the tracer from the end point, so we dont go through the end point
	len = -cg_tracerlength.value;
	tail[0] = len * dir[0] + tail[0];
	tail[1] = len * dir[1] + tail[1];
	tail[2] = len * dir[2] + tail[2];

	dist = Distance( origin, tail );

	le = CG_AllocLocalEntity();
	le->leType = LE_MOVING_TRACER;

	// spread the spawn back over the frame so a burst does not arrive at once
	if ( cg.frametime ) {
		offset = ( rand() % cg.frametime ) / 2;
	} else {
		offset = 0;
	}
	spawnTime = cg.time - offset;

	le->endTime = spawnTime - (int)( dist * -1000.0f / cg_tracerSpeed.value );
	le->pos.trTime = spawnTime;
	le->pos.trType = 2;    // TR_LINEAR
	le->pos.trBase[0] = origin[0];
	le->pos.trBase[1] = origin[1];
	le->pos.trBase[2] = origin[2];
	le->pos.trDelta[0] = cg_tracerSpeed.value * dir[0];
	le->pos.trDelta[1] = cg_tracerSpeed.value * dir[1];
	le->pos.trDelta[2] = cg_tracerSpeed.value * dir[2];
}

/*
==============
CG_DrawTracer

One camera-facing quad between two points, cg_tracerwidth wide.
==============
*/
void CG_DrawTracer( const vec3_t start, const vec3_t end ) {
	polyVert_t verts[4];
	vec3_t dir;
	vec3_t line;
	vec3_t right;

	VectorSubtract( end, start, dir );
	line[0] = DotProduct( dir, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( dir, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( end, cg_tracerwidth.value, right, verts[0].xyz );
	verts[0].st[0] = 1.0f;
	verts[0].st[1] = 1.0f;
	verts[0].st2[0] = 1.0f;
	verts[0].st2[1] = 1.0f;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( end, -cg_tracerwidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1.0f;
	verts[1].st[1] = 0.0f;
	verts[1].st2[0] = 1.0f;
	verts[1].st2[1] = 0.0f;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerwidth.value, right, verts[2].xyz );
	verts[2].st[0] = 0.0f;
	verts[2].st[1] = 0.0f;
	verts[2].st2[0] = 0.0f;
	verts[2].st2[1] = 0.0f;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerwidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0.0f;
	verts[3].st[1] = 1.0f;
	verts[3].st2[0] = 0.0f;
	verts[3].st2[1] = 1.0f;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );
}

/*
==============
CG_Tracer

A tracer for a bullet that arrives inside one frame: drawn straight away, at a
random length between 50 and 60 units, unless the shot was too short to see.
==============
*/
static void CG_Tracer( const vec3_t start, const vec3_t end, int forceDraw ) {
	vec3_t dir;
	vec3_t head, tail;
	float length;
	float nearDist;     // not near/far: MSVC reserves both
	float farDist;

	// tracer
	VectorSubtract( end, start, dir );
	length = VectorNormalize( dir );

	// start at least a little ways from the muzzle
	if ( length < 100.0f && !forceDraw ) {
		return;
	}

	nearDist = ( (float)rand() * ( 1.0f / 32768.0f ) ) * ( length - 60.0f ) + 50.0f;
	farDist = cg_tracerlength.value + nearDist;
	if ( farDist > length ) {
		farDist = length;
	}

	head[0] = nearDist * dir[0] + start[0];
	head[1] = nearDist * dir[1] + start[1];
	head[2] = nearDist * dir[2] + start[2];

	tail[0] = dir[0] * farDist + start[0];
	tail[1] = dir[1] * farDist + start[1];
	tail[2] = dir[2] * farDist + start[2];

	CG_DrawTracer( head, tail );
}

/*
==============
CG_CalcMuzzlePoint

Where a bullet from `entityNum` left the world.  The local client's own shots
come out of the eye, everything else off the firing model's tag.
==============
*/
static qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle, const char *tagName ) {
	centity_t   *cent;
	float tagMatrix[16];
	DObj *obj;

	if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
		 && entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewHeightCurrent;
		AddLeanToPosition( muzzle, cg.refdefViewAngles[1], cg.snap->ps.leanf, 16.0f, 20.0f );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	obj = trap_GetClientDObj( cent->currentState.number );
	if ( !obj ) {
		return qfalse;
	}

	if ( CG_DObjGetWorldTagMatrix( obj, tagName, cent, tagMatrix ) ) {
		muzzle[0] = tagMatrix[12];
		muzzle[1] = tagMatrix[13];
		muzzle[2] = tagMatrix[14];
		return qtrue;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );
	if ( entityNum < MAX_CLIENTS ) {
		Com_Printf( "No %s in CG_CalcMuzzlePoint on entity %d.\n", tagName, entityNum );
		if ( cent->currentState.eFlags & 0x40 ) {
			muzzle[2] += (float)bg_viewheight_prone.integer;
		} else if ( cent->currentState.eFlags & 0x20 ) {
			muzzle[2] += (float)bg_viewheight_crouched.integer;
		} else {
			muzzle[2] += (float)bg_viewheight_standing.integer;
		}
	}
	return qtrue;
}

/*
==============
CG_BulletTrajectoryEffects

The tracer and whizby for one bullet.  The local client's own shots get the
whizby only; everyone else's get a tracer cg_tracerchance of the time.
==============
*/
static void CG_BulletTrajectoryEffects( int entityNum, const vec3_t end, int surfaceType,
										const char *tagName ) {
	vec3_t muzzle;

	if ( cg_tracerchance.value <= 0.0f ) {
		return;
	}
	if ( !CG_CalcMuzzlePoint( entityNum, muzzle, tagName ) ) {
		return;
	}

	if ( !( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
			&& entityNum == cg.snap->ps.clientNum )
		 && (float)rand() < cg_tracerchance.value * 32768.0f ) {
		if ( surfaceType == SURF_TYPE_FLESH ) {
			CG_Tracer( muzzle, end, 0 );
			CG_WhizbySound( end, muzzle );
			return;
		}
		CG_SpawnTracer( muzzle, end );
	}

	CG_WhizbySound( end, muzzle );
}

/*
==============
CG_BulletHitEvent

A bullet that hit the world: the impact sound, the surface's impact and mark
effects, and the trajectory effects behind it.
==============
*/
void CG_BulletHitEvent( int entityNum, const vec3_t origin, int surfaceType, int event,
						const vec3_t dir, const vec3_t dir2 ) {
	const char *sound;
	int impact;
	int mark;

	if ( event == EV_BULLET_HIT_SMALL ) {
		sound = (const char *)cgs.media.bulletSmallSounds[surfaceType];
		impact = cgs.media.bulletSmallNormalEffects[surfaceType];
		mark = cgs.media.bulletSmallReflectEffects[surfaceType];
	} else {
		sound = (const char *)cgs.media.bulletLargeSounds[surfaceType];
		impact = cgs.media.bulletLargeNormalEffects[surfaceType];
		mark = cgs.media.bulletLargeReflectEffects[surfaceType];
	}

	if ( !cg_blood.integer && surfaceType == SURF_TYPE_FLESH ) {
		impact = cgs.media.fleshHitNoBloodEffect;
		mark = 0;
	}

	CG_PlaySoundAliasByName( sound, ENTITYNUM_WORLD, origin );

	if ( impact ) {
		trap_FX_PlayEffectID( impact, origin, dir );
	}
	if ( mark ) {
		trap_FX_PlayEffectID( mark, origin, dir2 );
	}

	CG_BulletTrajectoryEffects( entityNum, origin, surfaceType, cg_flashTags[0] );
}

/*
==============
CG_BulletHitClientEvent

A bullet that hit a player: only the sound and the trajectory effects; the
impact effects come from the hit player's own event.
==============
*/
void CG_BulletHitClientEvent( int entityNum, const vec3_t origin, int surfaceType, int event ) {
	const char *sound;

	if ( event == EV_BULLET_HIT_CLIENT_SMALL ) {
		sound = (const char *)cgs.media.bulletSmallSounds[surfaceType];
	} else {
		sound = (const char *)cgs.media.bulletLargeSounds[surfaceType];
	}

	CG_PlaySoundAliasByName( sound, ENTITYNUM_WORLD, origin );
	CG_BulletTrajectoryEffects( entityNum, origin, surfaceType, cg_flashTags[0] );
}
