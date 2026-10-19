// g_misc_mp.c -- info_* spawns, teleporters, misc_spawner and the
// player-operated turrets

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bg_public.h"
#include "g_local.h"

// DObj bone matrices are 4x4 (three 4-float rows, then the translation),
// so a matrix43_t does not overlay one
#define DOBJ_TAG_AXIS               0
#define DOBJ_TAG_ORIGIN             12

/* level.time is advanced in 100 msec steps, but the turret code runs off the
 * 50 msec client frame. */
#define FRAMETIME                   100
#define TURRET_FRAMETIME            50

#define MAX_TURRETS                 32

// gentity_t.turret points at one of the fixed pool; G_SpawnTurret takes the
// first free slot and G_FreeTurret releases it
typedef struct turret_s {
	int inUse;
	int flags;                      // 0x100/0x200/0x400 drive the pitch return, 0x800 toggles the teleport bit on the next aim
	int fireTimeRemaining;          // msec until it can fire again
	float minArc[2];                // pitch, yaw ("toparc", "rightarc"), <= 0
	float maxArc[2];                // pitch, yaw ("bottomarc", "leftarc"), >= 0
	float restPitch;                // -90 until turret_think_init traces for it
	int useMode;                    // weaponInfo_t.stance
	int stopUseEventType;           // -1 none, 0 stand, 1 crouch, 2 prone
	int fireSoundTime;              // counts the sustained-fire loop down
	vec3_t stopUseOrigin;           // where the player is put back
	float restPitchClamp;
	byte sustainedFireLoopSound;    // sound alias index
	byte sustainedFireStopSound;    // sound alias index
	byte pad_0x3E[2];
} turret_t;
G_ASSERT_SIZE( turret_t, 64 );

turret_t g_turrets[MAX_TURRETS];

/* forward: bg_weapon.c */
byte BG_GetWeaponIndexForName( const char *name );

/* forward: g_utils_mp.c -- G_DObjSetLocalTag by bone index instead of tag name */
void G_DObjSetLocalTagInternal( gentity_t *ent, const vec3_t origin, const vec3_t angles,
								int boneIndex );

/* forward: universal/com_math.c */
float AngleSubtract( float a1, float a2 );
float AngleNormalize180( float angle );
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
void AxisToAngles( vec3_t axis[3], vec3_t angles );
void MatrixTransformVector43( const vec3_t in, const matrix43_t *mat, vec3_t out );
float *MatrixTransformVector( const vec3_t in, float m[9], vec3_t out );
void MatrixMultiply43( const void *in0, const void *in1, void *out );
float vectosignedyaw( const vec3_t vec );
void YawVectors( float yaw, vec3_t forward, vec3_t right );
float Q_acos( float c );
vec_t Distance( const vec3_t p1, const vec3_t p2 );
void RotateVector2D( vec3_t v, float degrees );
float RotationToYaw( const float v[2] );
void YawToAxis( float yaw, vec3_t axis[3] );


// keep: unused stubs (original layout)
void G_UnusedStub1( void ) {
}


void G_UnusedStub2( void ) {
}


// keep: unused (original layout)
float G_UnusedFloatIdentity( float f ) {
	return f;
}


/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_camp( gentity_t *self ) {
	G_SetOrigin( self, self->r.currentOrigin );
}


/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null( gentity_t *self ) {
	G_FreeEntity( self );
}


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( gentity_t *self ) {
	G_SetOrigin( self, self->r.currentOrigin );
}


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point q3map_non-dynamic
Non-displayed light.
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
"q3map_non-dynamic" specifies that this light should not contribute to the world's 'light grid' and therefore will not light dynamic models in the game.(wolf)
*/
void SP_light( gentity_t *self ) {
	G_FreeEntity( self );
}


/*
=================================================================================

TELEPORTERS

=================================================================================
*/
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles ) {
	gentity_t   *tent;
	qboolean linked;

	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if ( player->client->sess.sessionState == STATE_PLAYING ) {
		tent = G_TempEntity( player->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = player->s.clientNum;
	}

	linked = player->r.linked;

	// unlink to make sure it can't possibly interfere with anything
	trap_UnlinkEntity( player );

	VectorCopy( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;

	// keep: original bug, '|' where '&' was meant -- the block below never runs
	if ( !( player->client->ps.eFlags | EF_TURRET_ACTIVE ) ) {
		// spit the player out
		AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
		VectorScale( player->client->ps.velocity, 400, player->client->ps.velocity );
		player->client->ps.pm_time = 160;       // hold time
		player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( linked ) {
		trap_LinkEntity( player );
	}
}


/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest( gentity_t *ent ) {
}


/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
Turned into a static brush model by the utilities; removed at load.
*/
void SP_misc_model( gentity_t *ent ) {
	G_FreeEntity( ent );
}


/*
==============
use_corona
	so level designers can toggle them on/off
==============
*/
void use_corona( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	} else {
		ent->active = 0;
		trap_LinkEntity( ent );
	}
}


/*QUAKED corona (0 1 0) (-4 -4 -4) (4 4 4) START_OFF
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
"scale" will designate a multiplier to the default size.  (so 2.0 is 2xdefault size, 0.5 is half)
*/
void SP_corona( gentity_t *ent ) {
	G_FreeEntity( ent );
}


/*
=================================================================================

TURRETS

=================================================================================
*/

void G_InitTurrets( void ) {
	int i;

	for ( i = 0; i < MAX_TURRETS; i++ ) {
		g_turrets[i].inUse = 0;
	}
}


/*
==============
Turret_FillWeaponParms

Build the fire block for a shot from the barrel: the axis comes from where the
gunner is looking, and the muzzle is pushed out along it by the distance from
the player tag to the flash tag, so a rotated barrel still fires from its own
tip.
==============
*/
static qboolean Turret_FillWeaponParms( gentity_t *self, gentity_t *player,
										weaponFireInfo_t *fireInfo ) {
	// G_DObjGetWorldTagMatrix writes a full 4x4 matrix; the translation is row 3
	float flashMat[4][4];
	float playerMat[4][4];
	vec3_t dir;
	float dist;

	if ( !G_DObjGetWorldTagMatrix( self, "tag_flash", flashMat ) ) {
		Com_Printf( "Couldn't find %s on turret (entity %d, classname '%s').\n",
					"tag_flash", self->s.number, SL_ConvertToString( self->classname ) );
		return qfalse;
	}

	if ( !G_DObjGetWorldTagMatrix( self, "tag_player", playerMat ) ) {
		Com_Printf( "Couldn't find %s on turret (entity %d, classname '%s').\n",
					"tag_player", self->s.number, SL_ConvertToString( self->classname ) );
		return qfalse;
	}

	AngleVectors( player->client->ps.viewangles, fireInfo->forward, fireInfo->right,
				  fireInfo->up );

	VectorCopy( fireInfo->forward, fireInfo->gunForward );

	VectorSubtract( flashMat[3], playerMat[3], dir );
	dist = VectorNormalize( dir );

	VectorMA( playerMat[3], dist, fireInfo->forward, fireInfo->start );

	return qtrue;
}


/* the fourth argument is never read; turret_shoot_internal passes 0 */
static void Fire_Lead( gentity_t *self, gentity_t *attacker, int damage, int unused ) {
	weaponFireInfo_t fireInfo;
	vec3_t end;

	if ( attacker == &g_entities[ENTITYNUM_NONE] ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	if ( !Turret_FillWeaponParms( self, attacker, &fireInfo ) ) {
		return;
	}

	fireInfo.weapInfo = bg_weaponInfo[self->s.weapon];

	if ( fireInfo.weapInfo->weaponType == WEAPTYPE_BULLET ) {
		Bullet_Endpos( 0, end, &fireInfo );
		Bullet_Fire_Extended( self, attacker, fireInfo.start, end, damage, 0, &fireInfo, self );
	} else {
		Weapon_RocketLauncher_Fire( self, &fireInfo, 0 );
	}

	G_AddEvent( self, EV_FIRE_WEAPON_MG42, 0 );
}


// Drive one goal weight at the rate that closes the gap in one server frame.
// keep: plain { } block with its own locals (original codegen)
#define SET_TURRET_GOAL_WEIGHT( animTree, anim, weight ) \
	{ \
		rfloat      sgw_rate; \
		float       sgw_time; \
		sgw_rate = fabs( trap_XAnimGetWeight( ( animTree ), ( anim ) ) - ( weight ) ) \
				   * ( 1000.0f / level.frameTime ); \
		if ( sgw_rate > 0 ) { \
			sgw_time = FCAST( 1.0f / sgw_rate ); \
		} else { \
			sgw_time = 0; \
		} \
		trap_XAnimSetGoalWeight( ( animTree ), ( anim ), ( weight ), sgw_time, 1.0f, 0, 0 ); \
	}

/*
==============
G_PlayerTurretPositionAndBlend

Stand the gunner where his own turret animation says he stands.

The animation is a two level XAnim tree.  The root's children are the mount's
vertical stops and each of those children's children are its horizontal stops,
so the barrel's yaw picks a grandchild pair and the height the player has to
reach picks a child pair; the four weights between them are the blend.  Walking
the root's children until one of them lifts the player high enough gives the
bracketing pair, and the pose that comes out of it -- its absolute delta folded
through tag_weapon and then through the turret's own transform -- is the
player's new origin and facing.

Every goal weight is driven at the rate that closes what is left of the gap in
one server frame.
==============
*/
void G_PlayerTurretPositionAndBlend( gentity_t *player, gentity_t *turret ) {
	clientInfo_t        *ci;
	const weaponInfo_t  *weapInfo;
	XAnimTree           *animTree;
	const float         *weaponMat;
	scr_anim_t rootAnim;
	scr_anim_t childAnim;
	scr_anim_t grandChildAnim;
	scr_anim_t nextGrandChildAnim;
	matrix43_t turretMat;
	matrix43_t barrelMat;
	matrix43_t playerMat;
	trace_t tr;
	vec3_t start, end;
	float deltaRot[2];      /* XAnimGetAbsDelta_m writes two floats */
	vec3_t deltaMove;
	float weaponYaw;
	float alongUp, targetAlongUp;
	float frac, lastFrac;
	float lastAlongUp, blend;
	// keep: rfloat / double locals match original codegen
	rfloat yawPos;
	rfloat delta[3];
	int yawIndex, lastYawIndex;
	int i, numRootChildren, numChildren;
	int treeIndex;
	double moveUp;

	ci = &bg_clientinfo[player->s.clientNum];

	if ( !ci->legs.animationNumber ) {
		return;
	}
	if ( !ci->legs.animation ) {
		return;
	}
	if ( !( ci->legs.animation->flags & ANIMFL_TURRET ) ) {
		return;
	}

	weaponMat = G_DObjGetLocalTagMatrix( turret, "tag_weapon" );
	if ( !weaponMat ) {
		Com_Printf( "WARNING: aborting player positioning on turret since 'tag_weapon' does not exist\n" );
		return;
	}

	animTree = ci->animTree;
	weapInfo = bg_weaponInfo[turret->s.weapon];
	// keep: own local (original codegen)
	treeIndex = bgs_animScriptData.animTreeIndex;
	rootAnim.anims = (unsigned short)Scr_GetAnimsIndex( treeIndex );
	rootAnim.index = (unsigned short)( ci->legs.animationNumber & ~ANIM_TOGGLEBIT );

	weaponYaw = vectosignedyaw( &weaponMat[DOBJ_TAG_AXIS] );

	AnglesToAxis( turret->r.currentAngles, turretMat.axis );
	VectorCopy( turret->r.currentOrigin, turretMat.origin );

	/* How far up the turret's own up axis the player already sits, and how far
	   up it the pose has to put him: the barrel's tag is the reference. */
	VectorSubtract( player->r.currentOrigin, turretMat.origin, delta );
	alongUp = DotProduct( delta, turretMat.axis[2] );
	targetAlongUp = alongUp - weaponMat[DOBJ_TAG_ORIGIN + 2];

	trap_XAnimClearTreeGoalWeightsStrict( animTree, rootAnim, 0.0f );

	numRootChildren = trap_XAnimGetNumChildren( rootAnim );

	lastAlongUp = 0;
	lastFrac = 0;
	lastYawIndex = 0;
	nextGrandChildAnim.index = 0;
	nextGrandChildAnim.anims = 0;

	if ( !numRootChildren ) {
		Com_Error( ERR_DROP, "\x15" "Player anim '%s' has no children",
				   trap_XAnimGetAnimName( rootAnim ) );
	}

	/* no entry test: Com_Error above does not return, so there is at least
	   one child */
	i = 0;
	do {
		childAnim = trap_XAnimGetChildAt( rootAnim, i );
		trap_XAnimSetGoalWeight( animTree, childAnim, 1.0f, 1.0f, 1.0f, 0, 0 );

		numChildren = trap_XAnimGetNumChildren( childAnim );
		if ( !numChildren ) {
			Com_Error( ERR_DROP, "\x15" "Player anim '%s' has no children",
					   trap_XAnimGetAnimName( childAnim ) );
		}

		/* The horizontal stops run either side of centre, one every
		   animHorRotateInc degrees. */
		yawPos = numChildren * 0.5f - (double)weaponYaw / weapInfo->animHorRotateInc;
		if ( yawPos < 0 ) {
			yawPos = 0;
		} else if ( yawPos >= (float)( numChildren - 1 ) ) {
			yawPos = (float)( numChildren - 1 );
		}

		yawIndex = (int)yawPos;
		frac = (float)( yawPos - yawIndex );

		grandChildAnim = trap_XAnimGetChildAt( childAnim, yawIndex );
		trap_XAnimSetGoalWeight( animTree, grandChildAnim, 1.0f - frac,
								 1.0f, 1.0f, 0, 0 );

		if ( frac != 0 ) {
			nextGrandChildAnim = trap_XAnimGetChildAt( childAnim, yawIndex + 1 );
			trap_XAnimSetGoalWeight( animTree, nextGrandChildAnim, frac,
									 1.0f, 1.0f, 0, 0 );
		}

		trap_XAnimCalcAbsDelta( animTree, childAnim, deltaRot, deltaMove );

		moveUp = deltaMove[2];
		if ( (float)moveUp >= targetAlongUp ) {
			break;
		}

		lastAlongUp = deltaMove[2];
		lastFrac = frac;
		lastYawIndex = yawIndex;
	} while ( ++i < numRootChildren );

	trap_XAnimClearTreeGoalWeightsStrict( animTree, rootAnim, 0.0f );

	SET_TURRET_GOAL_WEIGHT( animTree, grandChildAnim, 1.0f - frac );
	if ( frac != 0 ) {
		SET_TURRET_GOAL_WEIGHT( animTree, nextGrandChildAnim, frac );
	}

	if ( i != 0 && i != numRootChildren ) {
		/* The walk stopped on a child that overshoots: split the two vertical
		   stops it sits between, and give the lower one the yaw pair the walk
		   was holding when it left it. */
		blend = ( targetAlongUp - lastAlongUp ) / ( deltaMove[2] - lastAlongUp );
		SET_TURRET_GOAL_WEIGHT( animTree, childAnim, blend );

		childAnim = trap_XAnimGetChildAt( rootAnim, i - 1 );
		SET_TURRET_GOAL_WEIGHT( animTree, childAnim, 1.0f - blend );

		grandChildAnim = trap_XAnimGetChildAt( childAnim, lastYawIndex );
		SET_TURRET_GOAL_WEIGHT( animTree, grandChildAnim, 1.0f - lastFrac );

		if ( lastFrac != 0 ) {
			grandChildAnim = trap_XAnimGetChildAt( childAnim, lastYawIndex + 1 );
			SET_TURRET_GOAL_WEIGHT( animTree, grandChildAnim, lastFrac );
		}
	} else {
		/* The walk ran off either end: there is nothing to split, so the last
		   child takes the whole weight.  The tag lookup is only here for the
		   bone recalculation it does on the way. */
		if ( !G_DObjGetLocalTagMatrix( turret, "tag_aim" ) ) {
			Com_Printf( "WARNING: aborting player positioning on turret since 'tag_aim' does not exist\n" );
			return;
		}

		SET_TURRET_GOAL_WEIGHT( animTree, childAnim, 1.0f );
	}

	trap_XAnimCalcAbsDelta( animTree, rootAnim, deltaRot, deltaMove );

	/* The pose's delta is in the barrel's own frame; tag_weapon's yaw turns it
	   back into the turret's, and its origin puts it on the barrel.  The
	   height stays the one the player already had. */
	RotateVector2D( deltaMove, weaponYaw );
	barrelMat.origin[2] = alongUp;      // keep: store order matches original codegen
	barrelMat.origin[0] = deltaMove[0] + weaponMat[DOBJ_TAG_ORIGIN + 0];
	barrelMat.origin[1] = deltaMove[1] + weaponMat[DOBJ_TAG_ORIGIN + 1];

	YawToAxis( RotationToYaw( deltaRot ) + weaponYaw, barrelMat.axis );

	MatrixMultiply43( &barrelMat, &turretMat, &playerMat );

	VectorCopy( playerMat.origin, player->client->ps.origin );

	/* Come up from the turret's own base rather than dropping the player
	   through whatever the pose put him inside of. */
	VectorCopy( player->client->ps.origin, start );
	VectorCopy( player->client->ps.origin, end );
	start[2] = turret->r.currentOrigin[2];

	trap_Trace( &tr, start, NULL, NULL, end, player->s.number, MASK_CLIENTSOLID );
	if ( tr.fraction < 1.0f ) {
		player->client->ps.origin[2] = tr.endpos[2];
	}

	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	VectorCopy( player->client->ps.origin, player->r.currentOrigin );
	AxisToAngles( playerMat.axis, player->r.currentAngles );

	trap_LinkEntity( player );
}


/*
==============
turret_clientaim

Clamp the gunner's view into the turret's arcs and into a 15 degree per frame
slew, store the result as the barrel's aim (s.angles2), and snap his view back
whenever the clamp actually bit.
==============
*/
static void turret_clientaim( gentity_t *player, gentity_t *turret ) {
	turret_t *t;
	vec3_t angles;
	float aim[2];
	float diff;
	qboolean clamped;
	int i;

	t = turret->turret;

	player->client->ps.viewlocked = 1;
	player->client->ps.viewlocked_entNum = turret->s.number;
	player->client->ps.gunfx = 0;

	clamped = qfalse;

	for ( i = 0; i < 2; i++ ) {
		aim[i] = AngleSubtract( player->client->ps.viewangles[i], turret->r.currentAngles[i] );

		if ( aim[i] > t->maxArc[i] ) {
			clamped = qtrue;
			aim[i] = t->maxArc[i];
		} else if ( aim[i] < t->minArc[i] ) {
			clamped = qtrue;
			aim[i] = t->minArc[i];
		}

		diff = AngleSubtract( aim[i], turret->s.angles2[i] );
		if ( fabs( diff ) > 15.0 ) {
			clamped = qtrue;
			if ( diff > 0 ) {
				aim[i] = turret->s.angles2[i] + 15.0f;
			} else {
				aim[i] = turret->s.angles2[i] - 15.0f;
			}
		}
	}

	turret->s.angles2[0] = aim[0];
	turret->s.angles2[1] = aim[1];
	turret->s.angles2[2] = 0;

	if ( t->flags & 0x800 ) {
		t->flags &= ~0x800;
		turret->s.eFlags ^= EF_TELEPORT_BIT;
	}

	if ( clamped ) {
		angles[2] = 0;
		angles[0] = aim[0] + turret->r.currentAngles[0];
		angles[1] = aim[1] + turret->r.currentAngles[1];
		SetClientViewAngle( player, angles );
	}
}


static void turret_shoot_internal( gentity_t *self, gentity_t *player ) {
	self->turret->fireSoundTime =
		3 * bg_weaponInfo[self->s.weapon]->fireTime;

	if ( player->client ) {
		Fire_Lead( self, player, self->damage, 0 );
		player->client->ps.viewlocked = 2;
	}
}


static void turret_track( gentity_t *player, gentity_t *turret ) {
	turret_t *t;
	const weaponInfo_t *weapon;

	t = turret->turret;

	turret_clientaim( player, turret );
	G_PlayerTurretPositionAndBlend( player, turret );

	weapon = bg_weaponInfo[turret->s.weapon];

	player->client->ps.viewlocked = 1;
	turret->s.eFlags &= ~EF_FIRING;

	t->fireTimeRemaining -= TURRET_FRAMETIME;
	if ( t->fireTimeRemaining <= 0 ) {
		t->fireTimeRemaining = 0;

		if ( player->client->buttons & BUTTON_ATTACK ) {
			t->fireTimeRemaining = weapon->fireTime;
			turret_shoot_internal( turret, player );
			turret->s.eFlags |= EF_FIRING;
		}
	}
}


static void turret_UpdateSound( gentity_t *self ) {
	turret_t *turret;

	turret = self->turret;

	self->s.loopSound = 0;

	if ( turret->fireSoundTime > 0 ) {
		self->s.loopSound = turret->sustainedFireLoopSound;

		turret->fireSoundTime -= TURRET_FRAMETIME;
		if ( turret->fireSoundTime <= 0 && turret->sustainedFireStopSound ) {
			self->s.loopSound = 0;
			G_PlaySoundAlias( self, turret->sustainedFireStopSound );
		}
	}
}


void G_ClientStopUsingTurret( gentity_t *self ) {
	gentity_t *player;
	turret_t *turret;

	player = &g_entities[self->r.ownerNum];
	turret = self->turret;

	turret->fireSoundTime = 0;
	self->s.loopSound = 0;

	if ( turret->stopUseEventType != -1 ) {
		if ( turret->stopUseEventType == 2 ) {
			G_AddEvent( player, EV_STANCE_FORCE_PRONE, 0 );
		} else if ( turret->stopUseEventType == 1 ) {
			G_AddEvent( player, EV_STANCE_FORCE_CROUCH, 0 );
		} else {
			G_AddEvent( player, EV_STANCE_FORCE_STAND, 0 );
		}
		turret->stopUseEventType = -1;
	}

	TeleportPlayer( player, turret->stopUseOrigin, player->r.currentAngles );

	player->client->ps.eFlags &= ~EF_TURRET_ACTIVE;
	player->client->ps.viewlocked = 0;
	player->client->ps.viewlocked_entNum = ENTITYNUM_NONE;
	player->active = 0;
	player->client->ps.gunfx = 0;
	player->s.otherEntityNum = 0;

	self->active = 0;
	self->r.ownerNum = ENTITYNUM_NONE;

	turret->flags &= ~0x800;
}


void turret_think_client( gentity_t *self ) {
	gentity_t *player;

	player = &g_entities[self->r.ownerNum];

	if ( player->active != 1 || player->client->sess.sessionState != STATE_PLAYING ) {
		G_ClientStopUsingTurret( self );
		return;
	}

	turret_track( player, self );
	turret_UpdateSound( self );
}


/*
==============
turret_UpdateTargetAngles

Step the barrel towards targetAngles, no faster than the weapon's turn rates
(or 200 deg/sec when driven without them).  s.angles2[2] carries the pitch
that has not been applied yet, so that the pitch can be re-clamped against
restPitchClamp after the step.  Returns true once nothing had to be limited.
==============
*/
static qboolean turret_UpdateTargetAngles( gentity_t *ent, const float *targetAngles,
										   int useWeaponRates ) {
	turret_t *turret;
	float savePitch;
	float maxStep[2];
	float target;
	float diff;
	double newPitch;
	qboolean atRest;
	int i;

	turret = ent->turret;

	savePitch = ent->s.angles2[0];
	ent->s.angles2[0] = ent->s.angles2[0] + ent->s.angles2[2];

	atRest = qtrue;

	if ( useWeaponRates ) {
		maxStep[0] = bg_weaponInfo[ent->s.weapon]->vertTurnSpeed;
		maxStep[1] = bg_weaponInfo[ent->s.weapon]->horTurnSpeed;
	} else {
		maxStep[0] = 200.0f;
		maxStep[1] = 200.0f;
	}

	if ( ( turret->flags & 0x200 ) && ( turret->flags & 0x100 ) && maxStep[0] < 360.0f ) {
		maxStep[0] = 360.0f;
	}

	for ( i = 0; i < 2; i++ ) {
		maxStep[i] = maxStep[i] * 0.05f;

		diff = AngleSubtract( targetAngles[i], ent->s.angles2[i] );
		if ( diff > maxStep[i] ) {
			diff = maxStep[i];
			atRest = qfalse;
		} else if ( diff < -maxStep[i] ) {
			diff = -maxStep[i];
			atRest = qfalse;
		}

		ent->s.angles2[i] = ent->s.angles2[i] + diff;
	}

	ent->s.angles2[2] = target = ent->s.angles2[0];

	if ( turret->flags & 0x200 ) {
		if ( !( turret->flags & 0x400 ) ) {
			if ( ent->s.angles2[0] > turret->restPitchClamp ) {
				target = turret->restPitchClamp;
			} else {
				turret->flags &= ~0x100;
			}
		} else {
			if ( ent->s.angles2[0] < turret->restPitchClamp ) {
				target = turret->restPitchClamp;
			} else {
				turret->flags &= ~0x100;
			}
		}
	}

	diff = AngleSubtract( target, savePitch );
	if ( diff > maxStep[0] ) {
		diff = maxStep[0];
		atRest = qfalse;
	} else if ( diff < -maxStep[0] ) {
		diff = -maxStep[0];
		atRest = qfalse;
	}

	// keep: double residual matches original codegen
	newPitch = (double)savePitch + diff;
	ent->s.angles2[0] = (float)newPitch;
	ent->s.angles2[2] = (float)( ent->s.angles2[2] - newPitch );

	return atRest;
}


qboolean turret_ReturnToDefaultPos( gentity_t *ent, int useWeaponRates ) {
	float targetAngles[2];

	if ( useWeaponRates ) {
		targetAngles[0] = 0;
	} else {
		targetAngles[0] = ent->turret->restPitch;
	}
	targetAngles[1] = 0;

	return turret_UpdateTargetAngles( ent, targetAngles, useWeaponRates );
}


void turret_think( gentity_t *self ) {
	float targetAngles[2];

	self->nextthink = level.time + TURRET_FRAMETIME;

	if ( self->linkInfo ) {
		G_GeneralLink( self );
	}

	if ( g_entities[self->r.ownerNum].client ) {
		return;         // a gunner is driving it from ClientEndFrame instead
	}

	turret_UpdateSound( self );
	self->s.eFlags &= ~EF_FIRING;

	targetAngles[0] = self->turret->restPitch;
	targetAngles[1] = 0;
	turret_UpdateTargetAngles( self, targetAngles, 0 );
}


/*
==============
turret_think_init

Find how far the barrel can drop before the stock hits something, and keep
that as the rest pitch.  Sweeps down in 3 degree steps from level, tracing
tag_aim to where tag_butt would end up at each one.
==============
*/
void turret_think_init( gentity_t *self ) {
	turret_t *turret;
	matrix43_t entMat;
	const float *aimMat;
	const float *buttMat;
	vec3_t angles;
	vec3_t axis[3];
	vec3_t delta, offset, start, end;
	trace_t trace;
	int aimBone, buttBone;
	int i;

	turret = self->turret;

	self->think = turret_think;
	self->nextthink = level.time + TURRET_FRAMETIME;

	aimBone = trap_DObjGetBoneIndex( self, "tag_aim" );
	if ( aimBone < 0 ) {
		return;
	}
	G_DObjCalcBone( self, aimBone );
	aimMat = trap_DObjGetMatrixArray( self ) + 16 * aimBone;
	if ( !aimMat ) {
		return;
	}

	buttBone = trap_DObjGetBoneIndex( self, "tag_butt" );
	if ( buttBone < 0 ) {
		return;
	}
	G_DObjCalcBone( self, buttBone );
	buttMat = trap_DObjGetMatrixArray( self ) + 16 * buttBone;
	if ( !buttMat ) {
		return;
	}

	AnglesToAxis( self->r.currentAngles, entMat.axis );
	VectorCopy( self->r.currentOrigin, entMat.origin );

	/* the bone matrices are 4x4; the translation is the fourth row */
	VectorSubtract( buttMat + 12, aimMat + 12, delta );

	MatrixTransformVector43( aimMat + 12, &entMat, start );

	for ( i = 0; i <= 30; i++ ) {
		angles[0] = (float)i * -3.0f;
		angles[1] = 0;
		angles[2] = 0;

		AnglesToAxis( angles, axis );
		MatrixTransformVector( delta, (float *)axis, offset );

		VectorAdd( offset, aimMat + 12, offset );

		MatrixTransformVector43( offset, &entMat, end );

		trap_LocationalTrace( &trace, start, end, self->s.number, MASK_SOLID, bulletPriorityMap );
		if ( trace.fraction < 1.0f ) {
			turret->restPitch = angles[0];
			return;
		}
	}
}


/*
==============
turret_controller

DObj bone controller: aim the barrel bones by s.angles2.  tag_aim and
tag_aim_animated take the pitch/yaw pair; the third bone takes the pitch that
turret_UpdateTargetAngles has not applied yet.
==============
*/
void turret_controller( gentity_t *self, unsigned int *partBits ) {
	vec3_t angles;
	int boneIndex;

	angles[1] = self->s.angles2[1];
	angles[0] = self->s.angles2[0];
	angles[2] = 0;

	boneIndex = trap_DObjGetBoneIndex( self, "tag_aim" );
	if ( boneIndex >= 0 && trap_DObjSetControlRotTransIndex( self, (const int *)partBits,
															 boneIndex ) ) {
		G_DObjSetLocalTagInternal( self, vec3_origin, angles, boneIndex );
	}

	boneIndex = trap_DObjGetBoneIndex( self, "tag_aim_animated" );
	if ( boneIndex >= 0 && trap_DObjSetControlRotTransIndex( self, (const int *)partBits,
															 boneIndex ) ) {
		G_DObjSetLocalTagInternal( self, vec3_origin, angles, boneIndex );
	}

	angles[0] = self->s.angles2[2];
	angles[1] = 0;

	boneIndex = trap_DObjGetBoneIndex( self, "tag_flash" );
	if ( boneIndex >= 0 && trap_DObjSetControlRotTransIndex( self, (const int *)partBits,
															 boneIndex ) ) {
		G_DObjSetLocalTagInternal( self, vec3_origin, angles, boneIndex );
	}
}


/* forward: bg_pmove.c */
float PM_FloatAbs( float value );

/*
==============
turret_behind

True when the player is standing inside the half-angle the turret's yaw arc
spans, measured from the middle of that arc -- i.e. behind the gun where he
could take hold of it.
==============
*/
static qboolean turret_behind( gentity_t *player, gentity_t *turret ) {
	turret_t *t;
	float yaw;
	float halfArc;
	float angle;
	vec3_t dir;
	vec3_t delta;

	t = turret->turret;

	yaw = turret->r.currentAngles[1] + t->minArc[1];
	halfArc = ( PM_FloatAbs( t->minArc[1] ) + PM_FloatAbs( t->maxArc[1] ) ) * 0.5f;
	YawVectors( AngleNormalize180( yaw + halfArc ), dir, NULL );
	VectorNormalize( dir );

	delta[0] = turret->r.currentOrigin[0] - player->r.currentOrigin[0];
	delta[1] = turret->r.currentOrigin[1] - player->r.currentOrigin[1];
	delta[2] = 0;
	VectorNormalize( delta );

	angle = Q_acos( DotProduct( dir, delta ) ) * 180.0f / M_PI;

	if ( angle > halfArc ) {
		return qfalse;
	}
	return qtrue;
}


void G_FreeTurret( gentity_t *self ) {
	if ( g_entities[self->r.ownerNum].client ) {
		G_ClientStopUsingTurret( self );
	}

	self->active = 0;
	self->turret->inUse = 0;
	self->turret = NULL;
}


qboolean G_IsTurretUsable( gentity_t *turret, gentity_t *player ) {
	if ( turret->active || !turret->turret || !turret->takedamage
		 || !turret_behind( player, turret ) || player->client->ps.grenadeTimeLeft ) {
		return qfalse;
	}

	return player->client->ps.groundEntityNum != ENTITYNUM_NONE;
}


void turret_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	turret_t *turret;
	vec3_t angles;
	int i;

	turret = self->turret;

	other->active = 1;
	self->active = 1;
	self->r.ownerNum = other->s.number;

	other->client->ps.viewlocked = 1;
	other->client->ps.viewlocked_entNum = self->s.number;

	turret->flags |= 0x800;
	VectorCopy( other->r.currentOrigin, turret->stopUseOrigin );

	other->s.otherEntityNum = self->s.number;

	if ( other->client->ps.pm_flags & PMF_PRONE ) {
		turret->stopUseEventType = 2;
	} else if ( other->client->ps.pm_flags & PMF_DUCKED ) {
		turret->stopUseEventType = 1;
	} else {
		turret->stopUseEventType = 0;
	}

	if ( turret->useMode == WEAPSTANCE_PRONE ) {
		other->client->ps.eFlags |= EF_TURRET_ACTIVE_PRONE;
		other->client->ps.eFlags &= ~EF_TURRET_ACTIVE_DUCK;
	} else if ( turret->useMode == WEAPSTANCE_DUCK ) {
		other->client->ps.eFlags |= EF_TURRET_ACTIVE_DUCK;
		other->client->ps.eFlags &= ~EF_TURRET_ACTIVE_PRONE;
	} else {
		other->client->ps.eFlags |= EF_TURRET_ACTIVE;
	}

	VectorCopy( self->r.currentAngles, self->TargetAngles );

	for ( i = 0; i < 2; i++ ) {
		self->s.angles2[i] = AngleSubtract( other->client->ps.viewangles[i],
											self->r.currentAngles[i] );

		if ( self->s.angles2[i] > turret->maxArc[i] ) {
			self->s.angles2[i] = turret->maxArc[i];
		} else if ( self->s.angles2[i] < turret->minArc[i] ) {
			self->s.angles2[i] = turret->minArc[i];
		}
	}

	angles[2] = 0;
	angles[0] = self->r.currentAngles[0] + self->s.angles2[0];
	angles[1] = self->s.angles2[1] + self->r.currentAngles[1];
	SetClientViewAngle( other, angles );
}


void G_SpawnTurret( const char *weaponName, gentity_t *ent ) {
	turret_t *turret;
	const weaponInfo_t *weapon;
	char *arc, *damage;
	qboolean fromMap;
	int i;

	for ( i = 0; i < MAX_TURRETS; i++ ) {
		turret = &g_turrets[i];
		if ( !turret->inUse ) {
			break;
		}
	}
	if ( i == MAX_TURRETS ) {
		Com_Error( ERR_DROP, "\x15" "G_SpawnTurret: max number of turrets (%d) exceeded",
				   MAX_TURRETS );
	}

	memset( turret, 0, sizeof( *turret ) );

	ent->turret = turret;
	turret->inUse = 1;

	ent->s.weapon = BG_GetWeaponIndexForName( weaponName );
	if ( !ent->s.weapon ) {
		Com_Error( ERR_DROP, "\x15" "bad weaponinfo '%s' specified for turret", weaponName );
	}

	weapon = bg_weaponInfo[ent->s.weapon];

	if ( !level.spawning && !itemRegistered[ent->s.weapon] ) {
		Scr_Error( va( "turret '%s' not precached", weaponName ) );
	}

	RegisterItem( ent->s.weapon, 1 );

	turret->fireTimeRemaining = 0;
	turret->useMode = weapon->stance;
	turret->stopUseEventType = -1;
	turret->fireSoundTime = 0;

	if ( weapon->loopFireSound && *weapon->loopFireSound ) {
		turret->sustainedFireLoopSound = G_SoundAliasIndex( weapon->loopFireSound );
	} else {
		turret->sustainedFireLoopSound = 0;
	}

	if ( weapon->stopFireSound && *weapon->stopFireSound ) {
		turret->sustainedFireStopSound = G_SoundAliasIndex( weapon->stopFireSound );
	} else {
		turret->sustainedFireStopSound = 0;
	}

	/* the key wins if the map set it, otherwise the weapon file's arc; the
	   two "min" arcs are negated on the way in so that both pairs read
	   min <= 0 <= max */
	fromMap = G_SpawnString( "rightarc", "", &arc );
	turret->minArc[1] = (float)atof( arc );
	if ( !fromMap ) {
		turret->minArc[1] = weapon->rightArc;
	}
	turret->minArc[1] = turret->minArc[1] * -1.0f;
	if ( turret->minArc[1] > 0 ) {
		turret->minArc[1] = 0;
	}

	fromMap = G_SpawnString( "leftarc", "", &arc );
	turret->maxArc[1] = (float)atof( arc );
	if ( !fromMap ) {
		turret->maxArc[1] = weapon->leftArc;
	}
	if ( turret->maxArc[1] < 0 ) {
		turret->maxArc[1] = 0;
	}

	fromMap = G_SpawnString( "toparc", "", &arc );
	turret->minArc[0] = (float)atof( arc );
	if ( !fromMap ) {
		turret->minArc[0] = weapon->topArc;
	}
	turret->minArc[0] = turret->minArc[0] * -1.0f;
	if ( turret->minArc[0] > 0 ) {
		turret->minArc[0] = 0;
	}

	fromMap = G_SpawnString( "bottomarc", "", &arc );
	turret->maxArc[0] = (float)atof( arc );
	if ( !fromMap ) {
		turret->maxArc[0] = weapon->bottomArc;
	}
	if ( turret->maxArc[0] < 0 ) {
		turret->maxArc[0] = 0;
	}

	turret->restPitch = -90.0f;

	if ( !ent->health ) {
		ent->health = 100;
	}

	fromMap = G_SpawnString( "damage", "0", &damage );
	ent->damage = atoi( damage );
	if ( !fromMap ) {
		ent->damage = weapon->damage;
	}
	if ( ent->damage < 0 ) {
		ent->damage = 0;
	}

	turret->flags = 3;

	ent->clipmask = CONTENTS_SOLID;
	ent->r.contents = 0x200004;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_TURRET;
	ent->flags |= FL_SUPPORTS_LINKTO;

	G_DObjUpdate( ent );

	ent->r.mins[0] = -32;
	ent->r.mins[1] = -32;
	ent->r.mins[2] = 0;
	ent->r.maxs[0] = 32;
	ent->r.maxs[1] = 32;
	ent->r.maxs[2] = 56;

	G_SetOrigin( ent, ent->r.currentOrigin );
	G_SetAngle( ent, ent->r.currentAngles );
	VectorClear( ent->s.angles2 );

	ent->think = turret_think_init;
	ent->nextthink = level.time + FRAMETIME;
	ent->controller = turret_controller;
	ent->use = turret_use;

	ent->s.apos.trType = TR_LINEAR_STOP;
	ent->takedamage = 1;

	trap_LinkEntity( ent );
}


/*QUAKED turret (0 0 1) (-16 -16 0) (16 16 40)
"weaponinfo"	mandatory, the weapon file the mount fires
"toparc" "bottomarc" "leftarc" "rightarc"	override the weapon file's arcs
"damage"		override the weapon file's damage
*/
void SP_turret( gentity_t *ent ) {
	char *weaponName;

	if ( !G_SpawnString( "weaponinfo", "", &weaponName ) ) {
		Com_Error( ERR_DROP, "\x15" "no weaponinfo specified for turret" );
	}

	G_SpawnTurret( weaponName, ent );
}


/*
=================================================================================

misc_spawner

=================================================================================
*/

void misc_spawner_think( gentity_t *ent ) {
	gitem_t *item;

	item = BG_FindItem( SL_ConvertToString( ent->spawnitem ) );
	if ( !Drop_Item( ent, item, 0, 0 ) ) {
		G_Printf( "-----> WARNING <-------\n" );
		G_Printf( "misc_spawner used at %s failed to drop!\n", vtos( ent->r.currentOrigin ) );
	}
}


void misc_spawner_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	self->think = misc_spawner_think;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity( self );
}


/*QUAKED misc_spawner (1 0 0) (-16 -16 -16) (16 16 16)
use the pickup name
  when this entity gets used it will spawn an item
that matches its spawnitem field
*/
void SP_misc_spawner( gentity_t *ent ) {
	if ( !ent->spawnitem ) {
		G_Printf( "-----> WARNING <-------\n" );
		G_Printf( "misc_spawner at loc %s has no spawnitem!\n", vtos( ent->r.currentOrigin ) );
		return;
	}

	ent->use = misc_spawner_use;
	trap_LinkEntity( ent );
}


/*
==============
miscGunnerEnemyScan

Take the first live client inside the gunner's radius as its enemy.
==============
*/
void miscGunnerEnemyScan( gentity_t *self ) {
	gentity_t *ent;

	for ( ent = g_entities; ent < &g_entities[level.maxclients]; ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( ent->health < 0 ) {
			continue;
		}

		if ( Distance( self->r.currentOrigin, ent->r.currentOrigin ) > (float)self->radius ) {
			continue;
		}

		self->enemy = ent;
		return;
	}
}
