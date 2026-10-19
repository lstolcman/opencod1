/*
 * name:		bg_public.h
 *
 * desc:		definitions shared by both the server game and client game modules
 *
*/

#ifndef __BG_PUBLIC_H__
#define __BG_PUBLIC_H__

#include "../universal/q_shared.h"

/*
=============================================================================

						ENTITY STATE

=============================================================================
*/

// angle indexes
#define PITCH                   0       // up / down
#define YAW                     1       // left / right
#define ROLL                    2       // fall over

#define MAX_EVENTS              4       // max events per frame before we drop events

#define DEFAULT_GRAVITY         800

//
// entityState_t->eType
//
typedef enum entityType_e {
	ET_GENERAL              = 0,
	ET_PLAYER               = 1,
	ET_PLAYER_CORPSE        = 2,
	ET_ITEM                 = 3,
	ET_MISSILE              = 4,
	ET_MOVER                = 5,
	ET_PORTAL               = 6,
	ET_INVISIBLE            = 7,
	ET_SCRIPTMOVER          = 8,
	ET_SOUND_BLEND          = 9,       // crossfades between two sound aliases
	ET_LOOP_FX              = 10,      // looping effect
	ET_TURRET               = 11,
	ET_EVENTS               = 12       // any of the EV_* events can be added freestanding
									   // by setting eType to ET_EVENTS + eventNum
} entityType_t;

typedef enum trType_e {
	TR_STATIONARY           = 0,
	TR_INTERPOLATE          = 1,       // non-parametric, but interpolate between snapshots
	TR_LINEAR               = 2,
	TR_LINEAR_STOP          = 3,
	TR_SINE                 = 4,       // value = base + sin( time / duration ) * delta
	TR_GRAVITY              = 5,
	TR_GRAVITY_LOW          = 6,
	TR_GRAVITY_FLOAT        = 7,       // super low grav with no gravity acceleration (floating feathers/fabric/leaves/...)
	TR_GRAVITY_PAUSED       = 8,       // has stopped, but will still do a short trace to see if it should be switched back to TR_GRAVITY
	TR_ACCELERATE           = 9,       // trDelta is the ultimate speed
	TR_DECCELERATE          = 10       // trDelta is the starting speed
} trType_t;

// player_state->stats[] indexes
typedef enum statIndex_e {
	STAT_HEALTH              = 0,
	STAT_DEAD_YAW            = 1,      // look this direction when dead
	STAT_MAX_HEALTH          = 2,      // health limit
	STAT_IDENT_CLIENT_NUM    = 3,      // client under the crosshair
	STAT_IDENT_CLIENT_HEALTH = 4,
	STAT_SPAWN_COUNT         = 5       // bumped on every respawn
} statIndex_t;

// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// NOTE: this must be synched with eventnames[] in bg_misc.c
// the surface runs are indexed by SURF_TYPE( surfaceFlags )
typedef enum entity_event_e {
	EV_NONE                         = 0,
	EV_FOOTSTEP_RUN_DEFAULT         = 1,
	EV_FOOTSTEP_RUN_BARK            = 2,
	EV_FOOTSTEP_RUN_BRICK           = 3,
	EV_FOOTSTEP_RUN_CARPET          = 4,
	EV_FOOTSTEP_RUN_CLOTH           = 5,
	EV_FOOTSTEP_RUN_CONCRETE        = 6,
	EV_FOOTSTEP_RUN_DIRT            = 7,
	EV_FOOTSTEP_RUN_FLESH           = 8,
	EV_FOOTSTEP_RUN_FOLIAGE         = 9,
	EV_FOOTSTEP_RUN_GLASS           = 10,
	EV_FOOTSTEP_RUN_GRASS           = 11,
	EV_FOOTSTEP_RUN_GRAVEL          = 12,
	EV_FOOTSTEP_RUN_ICE             = 13,
	EV_FOOTSTEP_RUN_METAL           = 14,
	EV_FOOTSTEP_RUN_MUD             = 15,
	EV_FOOTSTEP_RUN_PAPER           = 16,
	EV_FOOTSTEP_RUN_PLASTER         = 17,
	EV_FOOTSTEP_RUN_ROCK            = 18,
	EV_FOOTSTEP_RUN_SAND            = 19,
	EV_FOOTSTEP_RUN_SNOW            = 20,
	EV_FOOTSTEP_RUN_WATER           = 21,
	EV_FOOTSTEP_RUN_WOOD            = 22,
	EV_FOOTSTEP_RUN_ASPHALT         = 23,
	EV_FOOTSTEP_WALK_DEFAULT        = 24,
	EV_FOOTSTEP_WALK_BARK           = 25,
	EV_FOOTSTEP_WALK_BRICK          = 26,
	EV_FOOTSTEP_WALK_CARPET         = 27,
	EV_FOOTSTEP_WALK_CLOTH          = 28,
	EV_FOOTSTEP_WALK_CONCRETE       = 29,
	EV_FOOTSTEP_WALK_DIRT           = 30,
	EV_FOOTSTEP_WALK_FLESH          = 31,
	EV_FOOTSTEP_WALK_FOLIAGE        = 32,
	EV_FOOTSTEP_WALK_GLASS          = 33,
	EV_FOOTSTEP_WALK_GRASS          = 34,
	EV_FOOTSTEP_WALK_GRAVEL         = 35,
	EV_FOOTSTEP_WALK_ICE            = 36,
	EV_FOOTSTEP_WALK_METAL          = 37,
	EV_FOOTSTEP_WALK_MUD            = 38,
	EV_FOOTSTEP_WALK_PAPER          = 39,
	EV_FOOTSTEP_WALK_PLASTER        = 40,
	EV_FOOTSTEP_WALK_ROCK           = 41,
	EV_FOOTSTEP_WALK_SAND           = 42,
	EV_FOOTSTEP_WALK_SNOW           = 43,
	EV_FOOTSTEP_WALK_WATER          = 44,
	EV_FOOTSTEP_WALK_WOOD           = 45,
	EV_FOOTSTEP_WALK_ASPHALT        = 46,
	EV_FOOTSTEP_PRONE_DEFAULT       = 47,
	EV_FOOTSTEP_PRONE_BARK          = 48,
	EV_FOOTSTEP_PRONE_BRICK         = 49,
	EV_FOOTSTEP_PRONE_CARPET        = 50,
	EV_FOOTSTEP_PRONE_CLOTH         = 51,
	EV_FOOTSTEP_PRONE_CONCRETE      = 52,
	EV_FOOTSTEP_PRONE_DIRT          = 53,
	EV_FOOTSTEP_PRONE_FLESH         = 54,
	EV_FOOTSTEP_PRONE_FOLIAGE       = 55,
	EV_FOOTSTEP_PRONE_GLASS         = 56,
	EV_FOOTSTEP_PRONE_GRASS         = 57,
	EV_FOOTSTEP_PRONE_GRAVEL        = 58,
	EV_FOOTSTEP_PRONE_ICE           = 59,
	EV_FOOTSTEP_PRONE_METAL         = 60,
	EV_FOOTSTEP_PRONE_MUD           = 61,
	EV_FOOTSTEP_PRONE_PAPER         = 62,
	EV_FOOTSTEP_PRONE_PLASTER       = 63,
	EV_FOOTSTEP_PRONE_ROCK          = 64,
	EV_FOOTSTEP_PRONE_SAND          = 65,
	EV_FOOTSTEP_PRONE_SNOW          = 66,
	EV_FOOTSTEP_PRONE_WATER         = 67,
	EV_FOOTSTEP_PRONE_WOOD          = 68,
	EV_FOOTSTEP_PRONE_ASPHALT       = 69,
	EV_JUMP_DEFAULT                 = 70,
	EV_JUMP_BARK                    = 71,
	EV_JUMP_BRICK                   = 72,
	EV_JUMP_CARPET                  = 73,
	EV_JUMP_CLOTH                   = 74,
	EV_JUMP_CONCRETE                = 75,
	EV_JUMP_DIRT                    = 76,
	EV_JUMP_FLESH                   = 77,
	EV_JUMP_FOLIAGE                 = 78,
	EV_JUMP_GLASS                   = 79,
	EV_JUMP_GRASS                   = 80,
	EV_JUMP_GRAVEL                  = 81,
	EV_JUMP_ICE                     = 82,
	EV_JUMP_METAL                   = 83,
	EV_JUMP_MUD                     = 84,
	EV_JUMP_PAPER                   = 85,
	EV_JUMP_PLASTER                 = 86,
	EV_JUMP_ROCK                    = 87,
	EV_JUMP_SAND                    = 88,
	EV_JUMP_SNOW                    = 89,
	EV_JUMP_WATER                   = 90,
	EV_JUMP_WOOD                    = 91,
	EV_JUMP_ASPHALT                 = 92,
	EV_LANDING_DEFAULT              = 93,
	EV_LANDING_BARK                 = 94,
	EV_LANDING_BRICK                = 95,
	EV_LANDING_CARPET               = 96,
	EV_LANDING_CLOTH                = 97,
	EV_LANDING_CONCRETE             = 98,
	EV_LANDING_DIRT                 = 99,
	EV_LANDING_FLESH                = 100,
	EV_LANDING_FOLIAGE              = 101,
	EV_LANDING_GLASS                = 102,
	EV_LANDING_GRASS                = 103,
	EV_LANDING_GRAVEL               = 104,
	EV_LANDING_ICE                  = 105,
	EV_LANDING_METAL                = 106,
	EV_LANDING_MUD                  = 107,
	EV_LANDING_PAPER                = 108,
	EV_LANDING_PLASTER              = 109,
	EV_LANDING_ROCK                 = 110,
	EV_LANDING_SAND                 = 111,
	EV_LANDING_SNOW                 = 112,
	EV_LANDING_WATER                = 113,
	EV_LANDING_WOOD                 = 114,
	EV_LANDING_ASPHALT              = 115,
	EV_LANDING_PAIN_DEFAULT         = 116,
	EV_LANDING_PAIN_BARK            = 117,
	EV_LANDING_PAIN_BRICK           = 118,
	EV_LANDING_PAIN_CARPET          = 119,
	EV_LANDING_PAIN_CLOTH           = 120,
	EV_LANDING_PAIN_CONCRETE        = 121,
	EV_LANDING_PAIN_DIRT            = 122,
	EV_LANDING_PAIN_FLESH           = 123,
	EV_LANDING_PAIN_FOLIAGE         = 124,
	EV_LANDING_PAIN_GLASS           = 125,
	EV_LANDING_PAIN_GRASS           = 126,
	EV_LANDING_PAIN_GRAVEL          = 127,
	EV_LANDING_PAIN_ICE             = 128,
	EV_LANDING_PAIN_METAL           = 129,
	EV_LANDING_PAIN_MUD             = 130,
	EV_LANDING_PAIN_PAPER           = 131,
	EV_LANDING_PAIN_PLASTER         = 132,
	EV_LANDING_PAIN_ROCK            = 133,
	EV_LANDING_PAIN_SAND            = 134,
	EV_LANDING_PAIN_SNOW            = 135,
	EV_LANDING_PAIN_WATER           = 136,
	EV_LANDING_PAIN_WOOD            = 137,
	EV_LANDING_PAIN_ASPHALT         = 138,
	EV_FOLIAGE_SOUND                = 139,
	EV_STANCE_FORCE_STAND           = 140,
	EV_STANCE_FORCE_CROUCH          = 141,
	EV_STANCE_FORCE_PRONE           = 142,
	EV_STEP_VIEW                    = 143,
	EV_WATER_TOUCH                  = 144,
	EV_WATER_LEAVE                  = 145,
	EV_ITEM_PICKUP                  = 146,
	EV_ITEM_PICKUP_QUIET            = 147,
	EV_AMMO_PICKUP                  = 148,
	EV_NOAMMO                       = 149,
	EV_EMPTYCLIP                    = 150,
	EV_RELOAD                       = 151,
	EV_RELOAD_FROM_EMPTY            = 152,
	EV_RELOAD_START                 = 153,
	EV_RELOAD_END                   = 154,
	EV_RAISE_WEAPON                 = 155,
	EV_PUTAWAY_WEAPON               = 156,
	EV_WEAPON_ALT                   = 157,
	EV_PULLBACK_WEAPON              = 158,
	EV_FIRE_WEAPON                  = 159,
	EV_FIRE_WEAPONB                 = 160,
	EV_FIRE_WEAPON_LASTSHOT         = 161,
	EV_RECHAMBER_WEAPON             = 162,
	EV_EJECT_BRASS                  = 163,
	EV_MELEE_SWIPE                  = 164,
	EV_FIRE_MELEE                   = 165,
	EV_MELEE_HIT                    = 166,
	EV_MELEE_MISS                   = 167,
	EV_FIRE_WEAPON_MG42             = 168,
	EV_FIRE_QUADBARREL_1            = 169,
	EV_FIRE_QUADBARREL_2            = 170,
	EV_BULLET_TRACER                = 171,
	EV_SOUND_ALIAS                  = 172,
	EV_BULLET_HIT_SMALL             = 173,
	EV_BULLET_HIT_LARGE             = 174,
	EV_BULLET_HIT_CLIENT_SMALL      = 175,
	EV_BULLET_HIT_CLIENT_LARGE      = 176,
	EV_GRENADE_BOUNCE               = 177,
	EV_GRENADE_EXPLODE              = 178,
	EV_ROCKET_EXPLODE               = 179,
	EV_ROCKET_EXPLODE_NOMARKS       = 180,
	EV_MOLOTOV_EXPLODE              = 181,
	EV_MOLOTOV_EXPLODE_NOMARKS      = 182,
	EV_CUSTOM_EXPLODE               = 183,
	EV_CUSTOM_EXPLODE_NOMARKS       = 184,
	EV_RAILTRAIL                    = 185,
	EV_BULLET                       = 186,
	EV_PAIN                         = 187,
	EV_CROUCH_PAIN                  = 188,
	EV_DEATH                        = 189,
	EV_DEBUG_LINE                   = 190,
	EV_PLAY_FX                      = 191,
	EV_PLAY_FX_DIR                  = 192,
	EV_PLAY_FX_ON_TAG               = 193,
	EV_FLAMEBARREL_BOUNCE           = 194,
	EV_EARTHQUAKE                   = 195,
	EV_DROPWEAPON                   = 196,
	EV_ITEM_RESPAWN                 = 197,
	EV_ITEM_POP                     = 198,
	EV_PLAYER_TELEPORT_IN           = 199,
	EV_PLAYER_TELEPORT_OUT          = 200,
	EV_OBITUARY                     = 201,

	EV_MAX_EVENTS                   = 202
} entity_event_t;


/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

#define MAXTOUCH 32

typedef struct pmove_t
{
	// state (in / out)
	playerState_t *ps;

	// command (in)
	usercmd_t cmd, oldcmd;
	int tracemask;                  // collide against these types of surfaces
	int debugLevel;                 // if set, diagnostic output will be printed

	// results (out)
	int numtouch;
	int touchents[MAXTOUCH];

	vec3_t mins, maxs;              // bounding box size

	byte watertype;                 // byte, not int
	byte waterlevel;
	byte pad_0xDA;
	byte pad_0xDB;

	float xyspeed;

	// for fixed msec Pmove
	qboolean pmove_fixed;
	int pmove_msec;

	// callbacks to test the world
	// these will be different functions during game and cgame
	void ( *trace )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	void ( *trace2 )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	void ( *trace3 )( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int ( *pointcontents )( const vec3_t point, int passEntityNum, int contentMask );
} pmove_t;
COD1_ASSERT_SIZE( pmove_t, 248 );
COD1_STATIC_ASSERT( offsetof( pmove_t, pad_0xDA ) == 0xDA, "pmove_t.pad_0xDA" );
COD1_STATIC_ASSERT( offsetof( pmove_t, xyspeed ) == 0xDC, "pmove_t.xyspeed" );

void Pmove( pmove_t *pmove );
void PmoveSingle( pmove_t *pmove );

/*
=============================================================================

						WEAPONS

=============================================================================
*/

#define MAX_WEAPONS 64

// weapon file values
// NOTE: these must all be in sync with the name tables in bg_weapon.c

typedef enum weapType_t
{
	WEAPTYPE_BULLET     = 0x0,
	WEAPTYPE_GRENADE    = 0x1,
	WEAPTYPE_PROJECTILE = 0x2,
	WEAPTYPE_NUM        = 0x3,
} weapType_t;

typedef enum weapClass_t
{
	WEAPCLASS_RIFLE          = 0x0,
	WEAPCLASS_MG             = 0x1,
	WEAPCLASS_SMG            = 0x2,
	WEAPCLASS_PISTOL         = 0x3,
	WEAPCLASS_GRENADE        = 0x4,
	WEAPCLASS_ROCKETLAUNCHER = 0x5,
	WEAPCLASS_TURRET         = 0x6,
	WEAPCLASS_NON_PLAYER     = 0x7,        // "non-player"
	WEAPCLASS_NUM            = 0x8,
} weapClass_t;

typedef enum weapSlot_t
{
	WEAPSLOT_NONE         = 0x0,
	WEAPSLOT_PRIMARY      = 0x1,
	WEAPSLOT_PRIMARYB     = 0x2,
	WEAPSLOT_PISTOL       = 0x3,
	WEAPSLOT_GRENADE      = 0x4,
	WEAPSLOT_SMOKEGRENADE = 0x5,
	WEAPSLOT_NUM          = 0x6,
} weapSlot_t;

typedef enum weapStance_t
{
	WEAPSTANCE_STAND = 0x0,
	WEAPSTANCE_DUCK  = 0x1,
	WEAPSTANCE_PRONE = 0x2,
	WEAPSTANCE_NUM   = 0x3,
} weapStance_t;

typedef enum weapOverlayReticle_t
{
	WEAPOVERLAYRETICLE_NONE        = 0x0,
	WEAPOVERLAYRETICLE_CROSSHAIR   = 0x1,
	WEAPOVERLAYRETICLE_FG42        = 0x2,
	WEAPOVERLAYRETICLE_SPRINGFIELD = 0x3,
	WEAPOVERLAYRETICLE_GEWEHR43    = 0x4,
	WEAPOVERLAYRETICLE_NUM         = 0x5,
} weapOverlayReticle_t;

typedef enum weapProjExposion_t
{
	WEAPPROJEXP_GRENADE = 0x0,
	WEAPPROJEXP_ROCKET  = 0x1,
	WEAPPROJEXP_MOLOTOV = 0x2,
	WEAPPROJEXP_NONE    = 0x3,
	WEAPPROJEXP_NUM     = 0x4,
} weapProjExposion_t;

// weapon definition, parsed from the files in weapons/mp
typedef struct weaponInfo_t
{
	int weapIndex;
	const char *szInternalName;
	const char *displayName;
	const char *AIOverlayDescription;
	const char *gunModel;
	const char *handModel;
	// viewmodel animations; slot 0 is the root and is never parsed
	const char *rootAnim;
	const char *idleAnim;
	const char *emptyIdleAnim;
	const char *fireAnim;
	const char *holdFireAnim;
	const char *lastShotAnim;
	const char *rechamberAnim;
	const char *meleeAnim;
	const char *reloadAnim;
	const char *reloadEmptyAnim;
	const char *reloadStartAnim;
	const char *reloadEndAnim;
	const char *raiseAnim;
	const char *dropAnim;
	const char *altRaiseAnim;
	const char *altDropAnim;
	const char *adsFireAnim;
	const char *adsLastShotAnim;
	const char *adsRechamberAnim;
	const char *adsUpAnim;
	const char *adsDownAnim;
	const char *modeName;
	weapType_t weaponType;
	weapClass_t weaponClass;
	weapSlot_t weaponSlot;
	qboolean slotStackable;
	weapStance_t stance;
	const char *viewFlashEffect;
	const char *worldFlashEffect;
	const char *pickupSound;
	const char *ammoPickupSound;
	const char *projectileSound;
	const char *pullbackSound;
	const char *fireSound;
	const char *loopFireSound;
	const char *stopFireSound;
	const char *fireEchoSound;
	const char *lastShotSound;
	const char *rechamberSound;
	const char *reloadSound;
	const char *reloadEmptySound;
	const char *reloadStartSound;
	const char *reloadEndSound;
	const char *raiseSound;
	const char *altSwitchSound;
	const char *putawaySound;
	const char *noteTrackSoundA;
	const char *noteTrackSoundB;
	const char *noteTrackSoundC;
	const char *noteTrackSoundD;
	const char *shellEjectEffect;
	const char *lastShotEjectEffect;
	const char *reticleCenter;
	const char *reticleSide;
	int reticleCenterSize;
	int reticleSideSize;
	int reticleMinOfs;
	float standMoveF;
	float standMoveR;
	float standMoveU;
	float standRotP;
	float standRotY;
	float standRotR;
	float duckedOfsF;
	float duckedOfsR;
	float duckedOfsU;
	float duckedMoveF;
	float duckedMoveR;
	float duckedMoveU;
	float duckedRotP;
	float duckedRotY;
	float duckedRotR;
	float proneOfsF;
	float proneOfsR;
	float proneOfsU;
	float proneMoveF;
	float proneMoveR;
	float proneMoveU;
	float proneRotP;
	float proneRotY;
	float proneRotR;
	float posMoveRate;
	float posProneMoveRate;
	float standMoveMinSpeed;
	float duckedMoveMinSpeed;
	float proneMoveMinSpeed;
	float posRotRate;
	float posProneRotRate;
	float standRotMinSpeed;
	float duckedRotMinSpeed;
	float proneRotMinSpeed;
	const char *radiantName;
	const char *worldModel;
	const char *hudIcon;
	const char *modeIcon;
	const char *ammoIcon;
	int startAmmo;
	const char *ammoName;
	int ammoIndex;
	const char *clipName;
	int clipIndex;
	int maxAmmo;
	int clipSize;
	const char *sharedAmmoCapName;
	int sharedAmmoCapIndex;
	int sharedAmmoCap;
	int damage;
	int meleeDamage;
	int damageType;
	int fireDelay;
	int meleeDelay;
	int fireTime;
	int rechamberTime;
	int rechamberBoltTime;
	int holdFireTime;
	int meleeTime;
	int reloadTime;
	int reloadEmptyTime;
	int reloadAddTime;
	int reloadStartTime;
	int reloadStartAddTime;
	int reloadEndTime;
	int dropTime;
	int raiseTime;
	int altDropTime;
	int altRaiseTime;
	int fuseTime;
	float moveSpeedScale;
	float adsZoomFov;
	float adsZoomInFrac;
	float adsZoomOutFrac;
	const char *adsOverlayShader;
	weapOverlayReticle_t adsOverlayReticle;
	float adsOverlayWidth;
	float adsOverlayHeight;
	float adsBobFactor;
	float adsViewBobMult;
	float hipSpreadStandMin;
	float hipSpreadDuckedMin;
	float hipSpreadProneMin;
	float hipSpreadMax;
	float hipSpreadDecayRate;
	float hipSpreadFireAdd;
	float hipSpreadTurnAdd;
	float hipSpreadMoveAdd;
	float hipSpreadDuckedDecay;
	float hipSpreadProneDecay;
	float hipReticleSidePos;
	int adsTransInTime;
	int adsTransOutTime;
	float adsIdleAmount;
	float hipIdleAmount;
	float idleCrouchFactor;
	float idleProneFactor;
	float gunMaxPitch;
	float gunMaxYaw;
	float swayMaxAngle;
	float swayLerpSpeed;
	float swayPitchScale;
	float swayYawScale;
	float swayHorizScale;
	float swayVertScale;
	float swayShellShockScale;
	float adsSwayMaxAngle;
	float adsSwayLerpSpeed;
	float adsSwayPitchScale;
	float adsSwayYawScale;
	float adsSwayHorizScale;
	float adsSwayVertScale;
	qboolean twoHanded;
	qboolean rifleBullet;
	qboolean semiAuto;
	qboolean boltAction;
	qboolean aimDownSight;
	qboolean cookOffHold;
	qboolean clipOnly;
	qboolean wideListIcon;
	qboolean adsFire;
	const char *killIcon;
	qboolean wideKillIcon;
	qboolean noPartialReload;
	qboolean segmentedReload;
	int reloadAmmoAdd;
	int reloadStartAdd;
	const char *altWeapon;
	int altWeaponIndex;
	int dropAmmoMin;
	int dropAmmoMax;
	int explosionRadius;
	int explosionInnerDamage;
	int explosionOuterDamage;
	int projectileSpeed;
	int projectileSpeedUp;
	const char *projectileModel;
	weapProjExposion_t projExplosionType;
	const char *projExplosionEffect;
	const char *projExplosionSound;
	qboolean projImpactExplode;
	const char *projTrailEffect;
	int projectileDLight;
	float projectileRed;
	float projectileGreen;
	float projectileBlue;
	float adsAimPitch;
	float adsCrosshairInFrac;
	float adsCrosshairOutFrac;
	float adsGunKickPitchMin;
	float adsGunKickPitchMax;
	float adsGunKickYawMin;
	float adsGunKickYawMax;
	float adsGunKickAccel;
	float adsGunKickSpeedMax;
	float adsGunKickSpeedDecay;
	float adsGunKickStaticDecay;
	float adsViewKickPitchMin;
	float adsViewKickPitchMax;
	float adsViewKickYawMin;
	float adsViewKickYawMax;
	float adsViewKickCenterSpeed;
	float adsViewScatterMin;
	float adsViewScatterMax;
	float adsSpread;
	float hipGunKickPitchMin;
	float hipGunKickPitchMax;
	float hipGunKickYawMin;
	float hipGunKickYawMax;
	float hipGunKickAccel;
	float hipGunKickSpeedMax;
	float hipGunKickSpeedDecay;
	float hipGunKickStaticDecay;
	float hipViewKickPitchMin;
	float hipViewKickPitchMax;
	float hipViewKickYawMin;
	float hipViewKickYawMax;
	float hipViewKickCenterSpeed;
	float hipViewScatterMin;
	float hipViewScatterMax;
	float aiEffectiveRange;
	float aiMissRange;
	int adsReloadTransTime;
	int adsTransBlendTime;
	float leftArc;
	float rightArc;
	float topArc;
	float bottomArc;
	float accuracy;
	float vertTurnSpeed;
	float horTurnSpeed;
	float convergenceTime;
	float maxRange;
	float animHorRotateInc;
	float playerPositionDist;
	const char *useHintString;
	int useHintStringIndex;
	const char *script;
	// derived from adsTransInTime / adsTransOutTime
	float adsTransInRate;
	float adsTransOutRate;
} weaponInfo_t;
COD1_ASSERT_SIZE( weaponInfo_t, 1052 );
COD1_STATIC_ASSERT( offsetof( weaponInfo_t, rootAnim ) == 0x18, "weaponInfo_t.rootAnim" );
COD1_STATIC_ASSERT( offsetof( weaponInfo_t, damageType ) == 0x1C8, "weaponInfo_t.damageType" );
COD1_STATIC_ASSERT( offsetof( weaponInfo_t, adsViewScatterMin ) == 0x384, "weaponInfo_t.adsViewScatterMin" );
COD1_STATIC_ASSERT( offsetof( weaponInfo_t, hipViewScatterMin ) == 0x3C4, "weaponInfo_t.hipViewScatterMin" );

extern weaponInfo_t **bg_weaponInfo;
extern int bg_numWeapons;

weaponInfo_t *BG_GetInfoForWeapon( int weaponIndex );
int BG_GetNumWeapons( void );
int BG_GetNumAmmoTypes( void );
int BG_GetAmmoTypeMax( int ammoIndex );
int BG_GetNumAmmoClips( void );
int BG_GetAmmoClipSize( int clipIndex );
int BG_GetSharedAmmoCapSize( int sharedAmmoCapIndex );
const char *BG_GetAmmoTypeName( int ammoIndex );
const char *BG_GetAmmoClipName( int clipIndex );
const char *BG_GetWeaponTypeName( weapType_t weaponType );
const char *BG_GetWeaponSlotNameForIndex( weapSlot_t weaponSlot );
int BG_AmmoForWeapon( int weaponIndex );
int BG_ClipForWeapon( int weaponIndex );
int BG_WeaponIsClipOnly( int weaponIndex );
qboolean BG_IsAimDownSightWeapon( int weaponIndex );

/*
=============================================================================

						ITEMS

=============================================================================
*/

#define MAX_ITEM_MODELS 2
#define MAX_ITEM_ICONS 4

// gitem_t->type
typedef enum itemType_t
{
	IT_BAD    = 0x0,
	IT_WEAPON = 0x1,
	IT_AMMO   = 0x2,
	IT_HEALTH = 0x3,
} itemType_t;

typedef struct gitem_s
{
	const char *classname;          // spawning name
	const char *pickup_sound;
	const char *world_model[MAX_ITEM_MODELS];
	const char *icon;
	const char *ammoicon;
	const char *pickup_name;        // for printing on pickup
	int quantity;                   // for ammo how much, or duration of powerup
	itemType_t giType;              // IT_* flags
	int giTag;
	int giAmmoIndex;                // type of weapon ammo this uses
	int giClipIndex;                // which clip this weapon uses
} gitem_t;
COD1_ASSERT_SIZE( gitem_t, 48 );

// included in both the game dll and the client
extern gitem_t bg_itemlist[];

#define BG_NUM_ITEMS            70      // not counting the terminator

gitem_t *BG_FindItem( const char *pickupName );
gitem_t *BG_FindItemForWeapon( int weaponIndex );

/*
=============================================================================

						ANIMATION SCRIPT SYSTEM

=============================================================================
*/

#define MAX_MODEL_ANIMATIONS 512     // animations per model
#define MAX_ANIMSCRIPT_ANIMCOMMANDS 8
#define MAX_ANIMSCRIPT_ITEMS 128
#define MAX_ANIMSCRIPT_ITEMS_PER_MODEL 2048

// NOTE: these must all be in sync with string tables in bg_animation.c

typedef enum aistateEnum_t
{
	AISTATE_RELAXED = 0x0,
	AISTATE_QUERY   = 0x1,
	AISTATE_ALERT   = 0x2,
	AISTATE_COMBAT  = 0x3,
	MAX_AISTATES    = 0x4,
} aistateEnum_t;

typedef enum scriptAnimMoveTypes_t
{
	ANIM_MT_UNUSED      = 0x0,
	ANIM_MT_IDLE        = 0x1,
	ANIM_MT_IDLECR      = 0x2,
	ANIM_MT_IDLEPRONE   = 0x3,
	ANIM_MT_WALK        = 0x4,
	ANIM_MT_WALKBK      = 0x5,
	ANIM_MT_WALKCR      = 0x6,
	ANIM_MT_WALKCRBK    = 0x7,
	ANIM_MT_WALKPRONE   = 0x8,
	ANIM_MT_WALKPRONEBK = 0x9,
	ANIM_MT_RUN         = 0xA,
	ANIM_MT_RUNBK       = 0xB,
	ANIM_MT_RUNCR       = 0xC,
	ANIM_MT_RUNCRBK     = 0xD,
	ANIM_MT_TURNRIGHT   = 0xE,
	ANIM_MT_TURNLEFT    = 0xF,
	ANIM_MT_CLIMBUP     = 0x10,
	ANIM_MT_CLIMBDOWN   = 0x11,
	NUM_ANIM_MOVETYPES  = 0x12,
} scriptAnimMoveTypes_t;

typedef enum scriptAnimEventTypes_t
{
	ANIM_ET_PAIN            = 0x0,
	ANIM_ET_DEATH           = 0x1,
	ANIM_ET_FIREWEAPON      = 0x2,
	ANIM_ET_JUMP            = 0x3,
	ANIM_ET_JUMPBK          = 0x4,
	ANIM_ET_LAND            = 0x5,
	ANIM_ET_DROPWEAPON      = 0x6,
	ANIM_ET_RAISEWEAPON     = 0x7,
	ANIM_ET_CLIMBMOUNT      = 0x8,
	ANIM_ET_CLIMBDISMOUNT   = 0x9,
	ANIM_ET_RELOAD          = 0xA,
	ANIM_ET_CROUCH_TO_PRONE = 0xB,
	ANIM_ET_PRONE_TO_CROUCH = 0xC,
	ANIM_ET_MELEEATTACK     = 0xD,
	NUM_ANIM_EVENTTYPES     = 0xE,
} scriptAnimEventTypes_t;

typedef enum animBodyPart_t
{
	ANIM_BP_UNUSED     = 0x0,
	ANIM_BP_LEGS       = 0x1,
	ANIM_BP_TORSO      = 0x2,
	ANIM_BP_BOTH       = 0x3,
	NUM_ANIM_BODYPARTS = 0x4,
} animBodyPart_t;

typedef enum scriptAnimConditions_t
{
	ANIM_COND_WEAPONS         = 0x0,
	ANIM_COND_WEAPONCLASS     = 0x1,
	ANIM_COND_MOUNTED         = 0x2,
	ANIM_COND_MOVETYPE        = 0x3,
	ANIM_COND_UNDERHAND       = 0x4,
	ANIM_COND_CROUCHING       = 0x5,
	ANIM_COND_FIRING          = 0x6,
	ANIM_COND_WEAPON_POSITION = 0x7,
	ANIM_COND_STRAFING        = 0x8,
	NUM_ANIM_CONDITIONS       = 0x9,
} scriptAnimConditions_t;

typedef enum animScriptConditionTypes_t
{
	ANIM_CONDTYPE_BITFLAGS = 0x0,
	ANIM_CONDTYPE_VALUE    = 0x1,
	NUM_ANIM_CONDTYPES     = 0x2,
} animScriptConditionTypes_t;

typedef enum animScriptMountedType_t
{
	MOUNTED_UNUSED = 0x0,
	MOUNTED_MG42   = 0x1,
} animScriptMountedType_t;

typedef enum animScriptWeaponPosition_t
{
	WEAPON_POSITION_HIP = 0x0,
	WEAPON_POSITION_ADS = 0x1,
} animScriptWeaponPosition_t;

typedef enum animScriptStrafing_t
{
	STRAFING_NOT   = 0x0,
	STRAFING_LEFT  = 0x1,
	STRAFING_RIGHT = 0x2,
} animScriptStrafing_t;

typedef struct animStringItem_t
{
	const char *string;
	int hash;
} animStringItem_t;
COD1_ASSERT_SIZE( animStringItem_t, 8 );

// script animation handle: anim tree and index into it
typedef struct scr_anim_s {
	unsigned short index;
	unsigned short anims;
} scr_anim_t;
COD1_ASSERT_SIZE( scr_anim_t, 4 );

typedef struct animation_t
{
	char name[64];
	int blendTime;                  // take this long to blend to next anim
	int moveSpeed;
	//
	// derived
	//
	int duration;
	int hash;
	int flags;
	int movetype;                   // bit set of scriptAnimMoveTypes_t
	int noteType;
} animation_t;
COD1_ASSERT_SIZE( animation_t, 92 );

// animation_t->flags
#define ANIMFL_LADDER           0x002
#define ANIMFL_TURRET           0x004
#define ANIMFL_FIRING           0x008
#define ANIMFL_STRAFELEFT       0x010
#define ANIMFL_STRAFERIGHT      0x020
#define ANIMFL_DEATH            0x040
#define ANIMFL_LOOPED           0x080

// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
#define ANIM_TOGGLEBIT          0x200

typedef struct animScriptCondition_t
{
	int index;      // reference into the table of possible conditionals
	int value[2];       // can store anything from weapon bits, to position enums, etc
} animScriptCondition_t;
COD1_ASSERT_SIZE( animScriptCondition_t, 12 );

typedef struct animScriptCommand_t
{
	short bodyPart[2];      // play this animation on legs/torso/both
	short animIndex[2];     // animation index in our list of animations
	short animDuration[2];
	int soundIndex;         // int, not short
} animScriptCommand_t;
COD1_ASSERT_SIZE( animScriptCommand_t, 16 );

typedef struct animScriptItem_t
{
	int numConditions;
	animScriptCondition_t conditions[NUM_ANIM_CONDITIONS];
	int numCommands;
	animScriptCommand_t commands[MAX_ANIMSCRIPT_ANIMCOMMANDS];
} animScriptItem_t;
COD1_ASSERT_SIZE( animScriptItem_t, 244 );

typedef struct animScript_t
{
	int numItems;
	animScriptItem_t *items[MAX_ANIMSCRIPT_ITEMS];   // pointers into a global list of items
} animScript_t;
COD1_ASSERT_SIZE( animScript_t, 516 );

// this is the main structure that is duplicated on the client and server
typedef struct animScriptData_t
{
	animation_t animations[MAX_MODEL_ANIMATIONS];           // anim names, etc
	int numAnimations;

	// parsed from script file
	animScript_t scriptAnims[MAX_AISTATES][NUM_ANIM_MOVETYPES];             // locomotive anims, etc
	animScript_t scriptCannedAnims[MAX_AISTATES][NUM_ANIM_MOVETYPES];       // played randomly
	animScript_t scriptStateChange[MAX_AISTATES][MAX_AISTATES];             // state change events
	animScript_t scriptEvents[NUM_ANIM_EVENTTYPES];                         // events that trigger special anims

	// global list of script items for this model
	animScriptItem_t scriptItems[MAX_ANIMSCRIPT_ITEMS_PER_MODEL];
	int numScriptItems;

	int animTreeIndex;
	scr_anim_t torsoAnim;
	scr_anim_t legsAnim;
	scr_anim_t turningAnim;
	//
	// pointers to functions from the owning module
	//
	int ( *soundIndex )( const char *name );
	void ( *playSound )( int clientNum, const char *name );
} animScriptData_t;
COD1_ASSERT_SIZE( animScriptData_t, 636632 );

#ifndef MAX_CLIENTS
#define MAX_CLIENTS             64
#endif

#ifndef MAX_ATTACHED_MODELS
#define MAX_ATTACHED_MODELS     6
#endif

// one animation slot (legs or torso) of a clientInfo_t
typedef struct lerpFrame_t
{
	float yawAngle;
	qboolean yawing;
	float pitchAngle;
	qboolean pitching;
	int animationNumber;            // may include ANIM_TOGGLEBIT
	animation_t *animation;
	int animationTime;              // blend time in msec
	vec3_t oldFramePos;
	float animSpeedScale;
	int oldFrameSnapshotTime;
} lerpFrame_t;
COD1_ASSERT_SIZE( lerpFrame_t, 48 );

// per-client animation and model info, shared by the game and cgame
typedef struct clientInfo_t
{
	qboolean infoValid;
	qboolean nextValid;                             // present in the newest snapshot
	int clientNum;
	char name[32];
	int team;
	int score;
	int location;
	int health;
	char modelName[64];
	char attachModelNames[MAX_ATTACHED_MODELS][64];
	char attachTagNames[MAX_ATTACHED_MODELS][64];
	lerpFrame_t legs;
	lerpFrame_t torso;
	float leanAmount;
	float leanFraction;
	float viewPitch;
	float viewYaw;
	float viewRoll;
	int gunHandLeft;
	int dobjNeedsUpdate;                            // rebuild the DObj
	int conditions[NUM_ANIM_CONDITIONS][2];
	void *animTree;                                 // kept across ClientConnect
	int dobjWeapon;                                 // weapon the DObj was built for
} clientInfo_t;
COD1_ASSERT_SIZE( clientInfo_t, 1096 );

// animation data shared by the bg code; G_InitGame and CG_Init clear it in one go
typedef struct bgs_s {
	animScriptData_t animScriptData;
	void *animTree;
	scr_anim_t rootAnim, torsoAnim, legsAnim, turningAnim;
	clientInfo_t clientinfo[MAX_CLIENTS];
} bgs_t;
COD1_ASSERT_SIZE( bgs_t, 0xAC8EC );
COD1_STATIC_ASSERT( offsetof( bgs_t, animTree ) == 0x9B6D8, "bgs animation tree offset" );
COD1_STATIC_ASSERT( offsetof( bgs_t, clientinfo ) == 0x9B6EC, "bgs client array offset" );
extern bgs_t bgs;
#define bgs_animScriptData bgs.animScriptData
#define bgs_animTree bgs.animTree
#define bgs_rootAnim bgs.rootAnim
#define bgs_torsoAnim bgs.torsoAnim
#define bgs_legsAnim bgs.legsAnim
#define bgs_turningAnim bgs.turningAnim
#define bg_clientinfo bgs.clientinfo

#if defined( GAMEDLL ) || defined( CGAMEDLL )
// animation clock
typedef struct {
	int time;
	int animTime;
	int frametime;
} bgGameClock_t;
COD1_ASSERT_SIZE( bgGameClock_t, 12 );
extern bgGameClock_t bg;
#define bgs_time bg.time
#define bgs_animTime bg.animTime
#define bgs_frametime bg.frametime
#endif

// one entry of the model list handed to trap_DObjCreate
typedef struct DObjModel_s {
	void            *model;
	const char      *tagName;               // NULL for the base model
	short boneName;                         // negated model index: root bone of model n
	byte pad_0x0A[2];
} DObjModel;

// view and weapon angle state, handed to the bg_weapon.c calculators
typedef struct pmWeaponAngleState_t
{
	playerState_t *ps;
	float speed;
	float frametime;
	vec3_t moveOffset;
	float idleScale;
	int time;
	int viewKickStartTime;
	float viewKickPitch;
	float viewKickYaw;
	float recoilPitch;
	float recoilYaw;
	float recoilRoll;
	float recoilPitchSpeed;
	float recoilYawSpeed;
	int recoilState;
	vec3_t baseAngles;
} pmWeaponAngleState_t;

typedef struct bgViewAngleState_t
{
	playerState_t *ps;
	int viewKickStartTime;
	int time;
	float viewKickPitch;
	float viewKickRoll;
	float speed;
} bgViewAngleState_t;

//
// bg_animation.c
//
// entityState_t comes from g_public.h or cg_public.h, neither of which is included here
struct entityState_s;
#ifdef CGAMEDLL
struct DObj_s;
void        BG_Player_DoControllers( struct DObj_s *obj, clientInfo_t *ci,
									 struct entityState_s *es, int *partBits );
void        BG_UpdatePlayerDObj( struct DObj_s *obj, struct entityState_s *es, clientInfo_t *ci );
void        BG_PlayerAnimation( struct DObj_s *obj, struct entityState_s *es,
                                clientInfo_t *ci );
#else
struct gentity_s;
void        BG_Player_DoControllers( struct gentity_s *ent, clientInfo_t *ci,
									 struct entityState_s *es, int *partBits );
void        BG_UpdatePlayerDObj( struct gentity_s *ent, struct entityState_s *es,
								 clientInfo_t *ci );
void        BG_PlayerAnimation( struct gentity_s *ent, struct entityState_s *es,
                                clientInfo_t *ci );
#endif

// anims is the caller's animation list (NULL when re-parsing)
void        BG_AnimParseAnimScript( animScriptData_t *scriptData, void *anims, int *numAnims );
int         BG_AnimScriptAnimation( playerState_t *ps, aistateEnum_t state,
                                    scriptAnimMoveTypes_t movetype, qboolean isContinue );
int         BG_AnimScriptEvent( playerState_t *ps, scriptAnimEventTypes_t event,
                                qboolean isContinue, qboolean force );

//
// bg_misc.c
//
struct trajectory_t;
void        BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm,
                                                 playerState_t *ps );
void        BG_EvaluateTrajectory( const struct trajectory_t *tr, int atTime, vec3_t result );
void        BG_EvaluateTrajectoryDelta( const struct trajectory_t *tr, int atTime, vec3_t result );
void        BG_PlayerStateToEntityState( playerState_t *ps, struct entityState_s *s,
                                         qboolean snap );
void        BG_PlayerStateToEntityStateExtrapolate( playerState_t *ps, struct entityState_s *s,
                                                    int time, qboolean snap );
int         BG_CheckProneValid( int clientNum, const vec3_t origin, float radius, float height,
                                float yaw, float *groundOffset, float *pitchDown, float *pitchUp,
                                qboolean skipInitialTrace, qboolean allowFallback,
                                const vec3_t groundNormal,
                                void ( *traceFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
                                void ( *traceDownFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
                                qboolean useAltContentMask );
int         BG_CheckProne( int clientNum, const vec3_t origin, float radius, float height,
                           float yaw, float *groundOffset, float *pitchDown, float *pitchUp,
                           qboolean skipInitialTrace, qboolean allowFallback,
                           const vec3_t groundNormal,
                           void ( *traceFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
                           void ( *traceDownFunc )( trace_t *, const vec3_t, const vec3_t, const vec3_t, const vec3_t, int, int ),
                           qboolean useAltContentMask );

//
// bg_weapon.c
//
extern int bg_ammoTypeMax[MAX_WEAPONS];
extern int bg_ammoClipSizes[MAX_WEAPONS];
extern const char *bg_weaponSlotNames[WEAPSLOT_NUM];

qboolean    BG_GivePlayerWeapon( playerState_t *ps, int weapon );
int         BG_TakePlayerWeapon( playerState_t *ps, int weapon );
int         BG_IsPlayerWeaponInSlot( const playerState_t *ps, int weapon, qboolean includeAltWeapons );
int         BG_GetEmptySlotForWeapon( const playerState_t *ps, int weapon );
int         BG_GetStackSlotForWeapon( const playerState_t *ps, int weapon, int preferredSlot );
int         BG_GetMaxPickupableAmmo( const playerState_t *ps, int weapon );
double      BG_GetMinSpreadForWeapon( int time, const playerState_t *ps, int weaponIndex );

#endif /* __BG_PUBLIC_H__ */
