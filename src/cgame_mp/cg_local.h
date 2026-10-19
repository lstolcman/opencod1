// cg_local.h -- local definitions for the multiplayer client game module

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#ifndef __CG_LOCAL_H__
#define __CG_LOCAL_H__

#include "../universal/q_shared.h"
#include "../game_mp/bg_public.h"

#define CG_ASSERT_SIZE( type, bytes )   COD1_ASSERT_SIZE( type, bytes )

#include "cg_public.h"

#define MAX_CLIENTS             64
#define GENTITYNUM_BITS         10
#define MAX_GENTITIES           ( 1 << GENTITYNUM_BITS )
#define ENTITYNUM_NONE          ( MAX_GENTITIES - 1 )
#define MAX_QPATH               64
#define MAX_STRING_CHARS        1024

/*
=============================================================================

        CGAME-PRIVATE TYPES

=============================================================================
*/

#define MAX_LOCAL_ENTITIES      128
#define MAX_MARK_POLYS          1024
#define MAX_VERTS_ON_POLY       10
// corpse entities are numbered from MAX_CLIENTS
#define MAX_CORPSES             8
#define MAX_WEAPONS_CG          64
#define MAX_MODELS              256
// the collision model's inline model limit
#define MAX_SUBMODELS           512
#define MAX_FX                  64
#define MAX_SHELLSHOCK_PARMS    16
#define TEAMCHAT_HEIGHT         8
#define TEAMCHAT_WIDTH          90
#define FADE_TIME               100
// number of surface types
#define SURF_TYPE_NUM           23

// cgs.media.hintShaders[] indices (cg.cursorHintIcon); the named hints are
// followed by the per-weapon hud icons and the per-weapon ammo icons
#define HINT_ACTIVATE           2
#define HINT_NOACTIVATE         3
#define HINT_DOOR               4
#define HINT_NODOOR             5
#define HINT_MG42               6
#define HINT_HEALTH             7
#define HINT_LADDER             8
#define HINT_FRIENDLY           9
#define HINT_WEAPON_FIRST       10      // + weapon - 1
#define HINT_WEAPON_LAST        73
#define HINT_AMMO_FIRST         74      // + weapon - 1
#define HINT_AMMO_LAST          137
#define HINT_ICON_COUNT         138

typedef struct centity_s centity_t;

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
// (no playerEntity_t here: the player model is a DObj)
struct centity_s {
	entityState_t currentState;             // from cg.frame
	entityState_t nextState;                // from cg.nextFrame, if available

	qboolean currentValid;                  // true if cg.frame holds this entity
	qboolean bMuzzleFlash;                  // set by CG_FireWeapon, consumed by CG_AddPlayerWeapon
	int previousEventSequence;
	int previousPreEventSequence;
	qboolean bTrailMade;                    // missile trail effect started
	int miscTime;

	// exact interpolated position of entity on this frame
	vec3_t lerpOrigin;
	vec3_t lerpAngles;
	vec3_t lightingOrigin;
	int lastLoopFxTime;
	qhandle_t voiceChatSprite;
	int voiceChatSpriteTime;
};
CG_ASSERT_SIZE( centity_t, 552 );

//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s *prevMark;
	struct markPoly_s *nextMark;
	int time;
	int markShader;
	qboolean alphaFade;                     // fade alpha instead of rgb
	float color[4];
	int polyShader;
	int numVerts;
	void            *verts;
	polyVert_t vertData[MAX_VERTS_ON_POLY];
	int duration;
} markPoly_t;
CG_ASSERT_SIZE( markPoly_t, 372 );

// leType: 0 = fade rgb, 1 = scale fade, 2 = moving tracer
typedef struct localEntity_s {
	struct localEntity_s *prev;
	struct localEntity_s *next;
	int leType;
	int leFlags;
	int endTime;
	float lifeRate;                         // 1.0 / (endTime - startTime)
	trajectory_t pos;
	float color[4];
	float radius;
	refEntity_t refEntity;
} localEntity_t;
CG_ASSERT_SIZE( localEntity_t, 236 );
CG_ASSERT_OFFSET( localEntity_t, refEntity, 0x50 );

//======================================================================

typedef struct score_t {
	int client;
	int score;
	int ping;
	int deaths;
	int team;
	int statusIcon;
} score_t;
CG_ASSERT_SIZE( score_t, 24 );

// one camera shake in flight
typedef struct cameraShake_t {
	int startTime;
	float amplitude;
	float duration;                         // float, not int
	float radius;
	vec3_t origin;
	float weight;                           // the lowest-weight shake is replaced first
	float frameAmplitude;                   // name inferred
} cameraShake_t;
CG_ASSERT_SIZE( cameraShake_t, 36 );

// sound channels; the cg_shock_volume_* cvars are indexed by these
typedef enum {
	SND_CHANNEL_AUTO,
	SND_CHANNEL_MENU,
	SND_CHANNEL_BODY,
	SND_CHANNEL_ITEM,
	SND_CHANNEL_WEAPON,
	SND_CHANNEL_VOICE,
	SND_CHANNEL_LOCAL,
	SND_CHANNEL_MUSIC,
	SND_CHANNEL_ANNOUNCER,
	SND_CHANNEL_SHELLSHOCK,
	SND_CHANNEL_COUNT
} sndChannel_t;

// one shellshock type, set up from the cg_shock_* cvars
typedef struct shellshockParms_t {
	int         lerpTime;                   // always 3000
	float       viewKickFreq;               // 0.001 / cg_shock_viewKickPeriod: a frequency, not a period
	float       viewKickRadius;
	int         screenBlendFadeTime;
	int         screenBlendTime;
	qboolean    soundEnabled;
	int         soundFadeInTime;
	int         soundFadeOutTime;
	float       soundWetLevel;
	char        soundRoomType[16];
	float       soundVolume[SND_CHANNEL_COUNT];     // indexed by sndChannel_t
	int         soundModEndDelay;
	int         soundLoopFadeTime;
	int         soundLoopEndDelay;
	qboolean    mouseEnabled;
	int         mouseFadeTime;
	float       mouseSensitivityScale;
	float       mouseMaxPitchSpeed;
	float       mouseMaxYawSpeed;
} shellshockParms_t;
CG_ASSERT_SIZE( shellshockParms_t, 124 );

// the active shellshock, driven by ps.shellshockIndex / shellshockTime / shellshockDuration
typedef struct shellshock_t {
	shellshockParms_t *parms;               // &cgs.shellshockParms[ps.shellshockIndex]
	int startTime;
	int duration;
	int soundState;
	float mouseSensitivityScale;
	float viewDelta[2];
	int savedScreenBlend;
	int forcedStartTime;                    // cg.time when /shellshock ran
	int forcedDuration;
} shellshock_t;
CG_ASSERT_SIZE( shellshock_t, 40 );

// each weapon has an associated cgWeaponInfo_t that contains media
// references necessary to present the weapon and its effects; the shared
// weapon definition is bg_weaponInfo[weapon]
typedef struct cgWeaponInfo_t {
	DObj            *dobj;
	float           animRate[21];
	char            handModelName[64];
	int             lastWeapAnim;
	qboolean        registered;
	gitem_t        *item;
	const char     *displayName;
	const char     *modeName;
	const char     *AIOverlayDescription;
	qhandle_t       worldModel;
	int             viewFlashEffect;
	int             worldFlashEffect;
	vec3_t          weaponMidpoint;         // so it will rotate centered instead of by tag
	const char     *projectileSound;
	const char     *pullbackSound;
	const char     *fireSound;
	const char     *fireEchoSound;
	const char     *lastShotSound;
	const char     *rechamberSound;
	const char     *reloadSound;
	const char     *reloadEmptySound;
	const char     *reloadStartSound;
	const char     *reloadEndSound;
	const char     *raiseSound;
	const char     *altSwitchSound;
	const char     *putawaySound;
	const char     *noteTrackSoundA;
	const char     *noteTrackSoundB;
	const char     *noteTrackSoundC;
	const char     *noteTrackSoundD;
	qhandle_t       weaponIcon;
	qhandle_t       weaponSelectIcon;
	qhandle_t       ammoIcon;
	qhandle_t       hudIcon;
	qhandle_t       modeIcon;
	qhandle_t       ammoHudIcon;
	qhandle_t       missileModel;
	void            (*missileTrailFunc)( centity_t *cent, const struct cgWeaponInfo_t *wi );
	float           missileDlight;
	vec3_t          missileDlightColor;
	int             missileRenderfx;
	int             shellEjectEffect;
	int             lastShotEjectEffect;
	int             projExplosionEffect;
	const char     *projExplosionSound;
	int             missileTrailEffect;
	float           trailRadius;
	float           wiTrailTime;
	qhandle_t       reticleCenter;
	qhandle_t       reticleSide;
	qhandle_t       adsOverlayShader;
	// names inferred
	vec3_t          vPositionOrg[2];
	vec3_t          vPositionAng[2];
} cgWeaponInfo_t;
CG_ASSERT_SIZE( cgWeaponInfo_t, 408 );
CG_ASSERT_OFFSET( cgWeaponInfo_t, weaponMidpoint, 0x0BC );
CG_ASSERT_OFFSET( cgWeaponInfo_t, missileTrailFunc, 0x128 );
CG_ASSERT_OFFSET( cgWeaponInfo_t, trailRadius, 0x154 );
CG_ASSERT_OFFSET( cgWeaponInfo_t, vPositionOrg, 0x168 );

// the local player's weapon position state (names inferred)
typedef struct playerEntity_t {
	float fLastWeaponPosFrac;
	qboolean bPositionToADS;                // moving into ADS
	vec3_t vPositionLastOrg;
	float fLastIdleFactor;
	vec3_t vLastMoveOrg;
	vec3_t vLastMoveAng;
} playerEntity_t;
CG_ASSERT_SIZE( playerEntity_t, 48 );

// one compass blip
typedef struct compassFriendly_s {
	int time;                       // cg.time when this entry was refreshed
	vec3_t origin;                  // world x, world y, yaw
	int flashEndTime;               // cg.time + 3000 while the client is talking
} compassFriendly_t;
CG_ASSERT_SIZE( compassFriendly_t, 20 );

//======================================================================

// all cg.stepTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for some msec after

typedef struct cg_s {
	int clientFrame;                        // incremented each frame
	int clientNum;
	qboolean demoPlayback;
	qboolean levelShot;                     // taking a level menu screenshot
	int cubemapShot;                        // /r_cubemapshot face, 0 = none
	int cubemapSize;                        // edge length of the face

	// there are only one or two snapshot_t that are relevent at a time
	int latestSnapshotNum;                  // the number of snapshots the client system has received
	int latestSnapshotTime;                 // the time from latestSnapshotNum, so we don't need to read the snapshot yet
	snapshot_t      *snap;                  // cg.snap->serverTime <= cg.time
	snapshot_t      *nextSnap;              // cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t activeSnapshots[2];

	float frameInterpolation;               // (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)
	int frametime;                          // cg.time - cg.oldTime
	int time;                               // this is the time value that the client is rendering at
	int oldTime;                            // time at last frame, used for missile trails and prediction checking
	int physicsTime;                        // either cg.snap->time or cg.nextSnap->time
	qboolean mapRestart;                    // set on a map restart to set back the weapon
	qboolean renderingThirdPerson;          // during deaths, chasecams, etc

	// prediction state
	playerState_t predictedPlayerState;
	centity_t predictedPlayerEntity;

	playerEntity_t playerEntity;
	weaponInfo_t    *weaponInfo;            // bg_weaponInfo[ps.weapon], not a cgWeaponInfo_t
	int predictedErrorTime;
	vec3_t predictedError;
	int eventSequence;
	int predictableEvents[16];
	float stepChange;                       // for stair up smoothing
	int stepTime;
	float landChange;                       // for landing hard
	int landTime;
	// auto rotating items
	vec3_t autoAnglesSlow;
	vec3_t autoAxisSlow[3];
	vec3_t autoAngles;
	vec3_t autoAxis[3];
	vec3_t autoAnglesFast;
	vec3_t autoAxisFast[3];

	// view rendering
	refdef_t refdef;
	vec3_t refdefViewAngles;                // will be converted to refdef.viewaxis
	// weapon sway state
	vec3_t previousViewAngles;
	vec3_t swayAngles;
	vec3_t swayOffsets;

	// per-entity DObj cache (CG_SetDObjInfo)
	int iEntityLastType[ENTITYNUM_NONE];
	void            *pEntityLastXModel[ENTITYNUM_NONE];

	float zoomSensitivity;                  // fov_x / cg_fov, the mouse sensitivity scale
	// information screen text during loading
	char infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int scoresRequestTime;
	int numScores;
	int teamScores[4];                      // indexed by team
	int teamPing[4];
	int teamPlayers[4];
	score_t scores[MAX_CLIENTS];

	qboolean showScores;
	qboolean scoreBoardShowing;
	int scoreFadeTime;
	int scoreboardScrollPos;
	qboolean scoreboardListFull;            // set when the list runs off the bottom; name inferred
	char killerName[32];
	char spectatorList[MAX_STRING_CHARS];   // list of names
	int spectatorLen;                       // length of list
	float spectatorWidth;                   // width in device units
	int spectatorTime;                      // next time to offset
	int spectatorPaintX;                    // current paint x
	int spectatorPaintX2;                   // current paint x
	int spectatorOffset;                    // current offset from start
	int spectatorPaintLen;                  // current offset from start
	qboolean showItems;
	int itemFadeTime;

	// centerprinting
	int centerPrintTime;
	int centerPrintCharWidth;
	int centerPrintY;
	char centerPrint[MAX_STRING_CHARS];
	int centerPrintLines;
	int centerPrintPriority;

	// fade in/out
	int fadeTime;
	float fadeRate;                         // 1 / duration; 0 disables
	float fadeFrom[4];
	float fadeTo[4];

	// low ammo warning state
	int lowAmmoWarning;

	// crosshair client ID
	int crosshairClientNum;
	int crosshairClientTime;
	int crosshairPowerupNum;
	int crosshairPowerupTime;

	// copied from ps.stats[3] / ps.stats[4] by CG_SetNextSnap
	int identifyClientNum;
	int identifyClientHealth;               // 0..100
	int identifyNextTime;

	// cursorhints
	int cursorHintIcon;
	int cursorHintTime;
	int cursorHintFade;
	int cursorHintValue;
	int cursorHintString;

	// powerup active flashing
	int powerupActive;
	int powerupTime;

	// attacking player
	int damageFeedbackTime;
	int voicetime;

	// reward medals
	int rewardTime;
	int rewardCount;
	qhandle_t rewardShader;

	int itemPickup;
	int itemPickupTime;
	int itemPickupBlendTime;                // the pulse around the crosshair is timed seperately
	int weaponSelectTime;
	int weaponAnimation;
	int weaponAnimationTime;
	float weaponSlotFade[6];                // per-slot fade fractions
	int lastWeaponSelectDrawTime;           // cg.time of the previous CG_DrawWeaponSelect

	// damage direction indicators: { time, duration, yaw }
	int damageIndicators[8][3];

	int damageTime;                         // last time any kind of damage was recieved
	int damageIndex;                        // slot that was filled in
	float damageX;
	float damageY;
	float damageValue;
	int grenLastTime;
	qboolean noMarks;                       // suppresses CG_ImpactMark (EV_ROCKET_EXPLODE_NOMARKS)

	// status bar head
	float headYaw;
	float headEndPitch;
	float headEndYaw;
	int headEndTime;
	float headStartPitch;
	float headStartYaw;
	int headStartTime;

	// view movement
	int v_dmg_time;
	float v_dmg_pitch;
	float v_dmg_roll;

	// temp working variables for player view
	float bobCycle;                         // bob phase in radians
	float xyspeed;
	int nextOrbitTime;

	// development tool
	refEntity_t testModelEntity;
	char testModelName[MAX_QPATH];

	qboolean testGun;
	// /fxTest console command state
	char fxTestName[64];
	vec3_t fxTestPos;
	int fxTestTime;
	int fxTestRestart;                      // respawn interval in msec, 0 = once
	vec3_t kick_angles;                     // weapon kicks
	vec3_t kick_origin;
	// names inferred; unused
	float vKickScatter[2];
	float vNewKickScatterChange[2];
	float vKickScatterLerp;
	float vKickScatterFrac;
	int iLastKickScatterDir;
	float vLastCompensationViewAngles[2];
	float vKickCompensation[2];
	float vKickCompensationCentering[2];
	qboolean bManualKickCompensation[2];
	// view model placement (names inferred)
	float gunPitch;
	float gunYaw;
	float gunXOfs;                          // ADS world offset
	float gunYOfs;
	float gunZOfs;
	// BG_CalculateWeaponAngles' recoil state, carried between frames
	float recoilPitch;
	float recoilYaw;
	float recoilRoll;
	float recoilPitchSpeed;
	float recoilYawSpeed;
	int recoilState;
	vec3_t viewModelOrigin;                 // view weapon placement
	vec3_t viewModelAxis[3];
	qboolean cameraMode;                    // if rendering from a camera

	// objective info display
	int oidTeam;
	int oidPrintTime;
	int oidPrintCharWidth;
	int oidPrintY;
	char oidPrint[1024];
	int oidPrintLines;

	cameraShake_t cameraShakes[4];
	// names inferred
	float shakePhase;
	float shakeMinAmplitude;

	// for voice chat buffer
	int voiceChatTime;
	int voiceChatBufferIn;
	int voiceChatBufferOut;
	qboolean latchVictorySound;

	// compass
	float compassNorthYaw;
	float compassPlayerAngle;
	float compassPlayerAngleVel;
	float compassPointerAngle;
	float compassPointerAngleVel;
	compassFriendly_t compassFriendlies[MAX_CLIENTS];

	shellshock_t shellshock;
} cg_t;
CG_ASSERT_SIZE( cg_t, 0x2B498 );
CG_ASSERT_OFFSET( cg_t, playerEntity, 0x27314 );
CG_ASSERT_OFFSET( cg_t, refdef, 0x2743C );
CG_ASSERT_OFFSET( cg_t, zoomSensitivity, 0x294B4 );
CG_ASSERT_OFFSET( cg_t, scoreBoardShowing, 0x29EF4 );
CG_ASSERT_OFFSET( cg_t, spectatorList, 0x29F24 );
CG_ASSERT_OFFSET( cg_t, spectatorPaintLen, 0x2A33C );
CG_ASSERT_OFFSET( cg_t, showItems, 0x2A340 );
CG_ASSERT_OFFSET( cg_t, crosshairPowerupNum, 0x2A790 );
CG_ASSERT_OFFSET( cg_t, identifyClientNum, 0x2A798 );
CG_ASSERT_OFFSET( cg_t, identifyNextTime, 0x2A7A0 );
CG_ASSERT_OFFSET( cg_t, powerupActive, 0x2A7B8 );
CG_ASSERT_OFFSET( cg_t, rewardTime, 0x2A7C8 );
CG_ASSERT_OFFSET( cg_t, weaponAnimation, 0x2A7E4 );
CG_ASSERT_OFFSET( cg_t, damageIndex, 0x2A86C );
CG_ASSERT_OFFSET( cg_t, grenLastTime, 0x2A87C );
CG_ASSERT_OFFSET( cg_t, headYaw, 0x2A884 );
CG_ASSERT_OFFSET( cg_t, v_dmg_pitch, 0x2A8A4 );
CG_ASSERT_OFFSET( cg_t, nextOrbitTime, 0x2A8B4 );
CG_ASSERT_OFFSET( cg_t, testModelName, 0x2A954 );
CG_ASSERT_OFFSET( cg_t, testGun, 0x2A994 );
CG_ASSERT_OFFSET( cg_t, vKickScatter, 0x2AA04 );
CG_ASSERT_OFFSET( cg_t, bManualKickCompensation, 0x2AA38 );
CG_ASSERT_OFFSET( cg_t, gunPitch, 0x2AA40 );
CG_ASSERT_OFFSET( cg_t, cameraMode, 0x2AA9C );
CG_ASSERT_OFFSET( cg_t, oidTeam, 0x2AAA0 );
CG_ASSERT_OFFSET( cg_t, oidPrintTime, 0x2AAA4 );
CG_ASSERT_OFFSET( cg_t, oidPrintLines, 0x2AEB0 );
CG_ASSERT_OFFSET( cg_t, latchVictorySound, 0x2AF58 );
CG_ASSERT_OFFSET( cg_t, compassFriendlies, 0x2AF70 );
CG_ASSERT_OFFSET( cg_t, shellshock, 0x2B470 );

//======================================================================

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to weapons are stored in cgWeaponInfo_t
typedef struct cgMedia_t {
	qhandle_t whiteShader;
	qhandle_t hudSoftLineShader;
	qhandle_t hudSoftLineHShader;
	qhandle_t hudAxisIcon;
	qhandle_t hudAlliedIcon;
	qhandle_t hudColorBar;
	qhandle_t reserved_0x018;               // unused
	qhandle_t railCoreShader;
	qhandle_t headiconVoiceChat;
	qhandle_t headiconTalkBalloon;
	qhandle_t headiconDisconnected;
	qhandle_t headiconYouInKillCam;
	qhandle_t selectShader;
	qhandle_t viewBloodShader;              // unused
	qhandle_t tracerShader;
	qhandle_t disconnectShader;
	qhandle_t lagometerShader;
	qhandle_t backTileShader;
	qhandle_t hudNoWeaponIcon;
	// cursor hint icons, indexed by cg.cursorHintIcon (see HINT_*)
	qhandle_t hintShaders[HINT_ICON_COUNT];
	qhandle_t hudStanceStand;
	qhandle_t hudStanceCrouch;
	qhandle_t hudStanceProne;
	qhandle_t hudStanceFlash;
	qhandle_t hudObjective;
	qhandle_t hudObjectiveUp;
	qhandle_t hudObjectiveDown;
	qhandle_t objectiveFriendly;
	qhandle_t objectiveFriendlyChat;
	qhandle_t hudHitDirection;
	qhandle_t numberShaders[11];
	qhandle_t markShadowShader;
	qhandle_t flareShader;
	qhandle_t wakeMarkShader;

	// sounds
	sfxHandle_t gibSound;
	sfxHandle_t gibBounceSound;
	sfxHandle_t noAmmoSound;
	sfxHandle_t landDamageSound;
	// played by CG_EntityEvent but never registered
	sfxHandle_t deathSound;                 // name inferred
	sfxHandle_t teleInSound;
	sfxHandle_t teleOutSound;
	sfxHandle_t respawnSound;
	sfxHandle_t talkSound;
	// surface type tables, indexed by the trace's surfaceType
	sfxHandle_t grenadeBounceSounds[SURF_TYPE_NUM];
	sfxHandle_t grenadeExplodeSounds[SURF_TYPE_NUM];
	sfxHandle_t rocketExplodeSounds[SURF_TYPE_NUM];
	// never registered or read; name inferred
	sfxHandle_t molotovExplodeSounds[SURF_TYPE_NUM];
	sfxHandle_t bulletSmallSounds[SURF_TYPE_NUM];
	sfxHandle_t bulletLargeSounds[SURF_TYPE_NUM];
	sfxHandle_t footstepRunSounds[SURF_TYPE_NUM];
	sfxHandle_t footstepWalkSounds[SURF_TYPE_NUM];
	sfxHandle_t footstepProneSounds[SURF_TYPE_NUM];
	sfxHandle_t landingSounds[SURF_TYPE_NUM];

	sfxHandle_t gearRattleRun;
	sfxHandle_t gearRattleWalk;
	sfxHandle_t movementFoliage;
	sfxHandle_t whizbySound;
	sfxHandle_t meleeSwingLarge;
	sfxHandle_t meleeSwingSmall;
	sfxHandle_t meleeHit;
	sfxHandle_t gameMessageSound;
	sfxHandle_t objectiveCompleteSound;
	sfxHandle_t announceGermanTwoMinutes;
	sfxHandle_t announceAlliedTwoMinutes;
	sfxHandle_t announceGermanThirtySeconds;
	sfxHandle_t announceAlliedThirtySeconds;
	sfxHandle_t playerWaterIn;
	sfxHandle_t playerWaterOut;
	sfxHandle_t debrisBounce;
	sfxHandle_t grenadePulse[4];
	sfxHandle_t spotlightSpark;
	sfxHandle_t flameSound;
	sfxHandle_t flameBlowSound;
	sfxHandle_t flameStreamSound;
	sfxHandle_t flameCrackSound;
	sfxHandle_t playerBoneBounce;
	sfxHandle_t debrisHitPlayer;
	sfxHandle_t flamebarrelBounce;
	qhandle_t checkboxClear;
	qhandle_t checkboxChecked;
	qhandle_t checkboxFail;

	// impact effect tables, indexed by surface type
	int bulletSmallNormalEffects[SURF_TYPE_NUM];
	int bulletSmallReflectEffects[SURF_TYPE_NUM];
	int bulletLargeNormalEffects[SURF_TYPE_NUM];
	int bulletLargeReflectEffects[SURF_TYPE_NUM];
	int grenadeBounceEffects[SURF_TYPE_NUM];
	int grenadeExplodeEffects[SURF_TYPE_NUM];
	int rocketExplodeEffects[SURF_TYPE_NUM];
	int molotovExplodeNormalEffects[SURF_TYPE_NUM];
	int molotovExplodeReflectEffects[SURF_TYPE_NUM];
	int fleshHitNoBloodEffect;
} cgMedia_t;
CG_ASSERT_SIZE( cgMedia_t, 2636 );
CG_ASSERT_OFFSET( cgMedia_t, viewBloodShader, 0x034 );
CG_ASSERT_OFFSET( cgMedia_t, grenadeBounceSounds, 0x2F8 );
CG_ASSERT_OFFSET( cgMedia_t, molotovExplodeSounds, 0x40C );
CG_ASSERT_OFFSET( cgMedia_t, gearRattleRun, 0x690 );
CG_ASSERT_OFFSET( cgMedia_t, bulletSmallNormalEffects, 0x70C );

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct cgs_s {
	gameState_t gameState;                  // gamestate from server
	glconfig_t glconfig;                    // rendering configuration
	float screenXScale;                     // derived from glconfig
	float screenYScale;
	float screenXBias;
	int serverCommandSequence;              // reliable command stream counter
	int processedSnapshotNum;               // the number of snapshots cgame has requested
	qboolean localServer;                   // detected on startup by checking sv_running

	// parsed from serverinfo
	char gametype[32];                      // a string, not an enum
	char sv_hostname[256];
	int maxclients;
	char mapname[64];
	char redTeam[MAX_QPATH];                // A team
	char blueTeam[MAX_QPATH];               // B team

	int voteTime;
	int voteYes;
	int voteNo;
	qboolean voteModified;                  // beep whenever changed
	char voteString[256];
	int teamVoteTime[2];
	int teamVoteYes[2];
	int teamVoteNo[2];
	qboolean teamVoteModified[2];           // beep whenever changed
	char teamVoteString[2][256];

	int levelStartTime;
	// from configstrings
	int scores1;
	int scores2;

	//
	// locally derived information from gamestate
	//
	qhandle_t gameModels[MAX_MODELS];
	int gameEffects[MAX_FX];
	shellshockParms_t shellshockParms[MAX_SHELLSHOCK_PARMS];

	int numInlineModels;
	qhandle_t inlineDrawModel[MAX_SUBMODELS];
	vec3_t inlineModelMidpoints[MAX_SUBMODELS];

	// teamchat width is *3 because of embedded color codes
	char teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH * 3 + 1];
	int teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int teamChatPos;
	int teamLastChatPos;

	// read by the menu system, never written
	int cursorX;
	int cursorY;
	byte reserved_0x099C8[12];              // RTCW's cursor/event block, partly dropped
	// screen fading
	float scriptFade[2];
	int scriptFadeStartTime;
	int scriptFadeDuration;

	// media
	cgMedia_t media;

	// teamkill complaint, driven by the 'm' server command
	int complaintClient;
	int complaintEndTime;
	float smokeWindDir;                     // for smoke puffs & wind (arty, airstrikes, bullet impacts)

	// ET_CORPSE entities keep a frozen copy of their client's clientInfo_t
	clientInfo_t corpseinfo[MAX_CORPSES];
} cgs_t;
CG_ASSERT_SIZE( cgs_t, 0xC67C );
CG_ASSERT_OFFSET( cgs_t, redTeam, 0x060A0 );
CG_ASSERT_OFFSET( cgs_t, teamVoteTime, 0x06230 );
CG_ASSERT_OFFSET( cgs_t, teamVoteString, 0x06250 );
CG_ASSERT_OFFSET( cgs_t, levelStartTime, 0x06450 );
CG_ASSERT_OFFSET( cgs_t, inlineModelMidpoints, 0x07920 );
CG_ASSERT_OFFSET( cgs_t, teamChatMsgs, 0x09120 );

/*
=============================================================================

        GLOBALS

=============================================================================
*/

extern cg_t cg;
extern cgs_t cgs;
extern centity_t cg_entities[MAX_GENTITIES];
// cgame's per-weapon media; the shared definitions are bg_weaponInfo[]
extern cgWeaponInfo_t cg_weapons[MAX_WEAPONS_CG];
extern markPoly_t cg_markPolys[MAX_MARK_POLYS];
extern localEntity_t cg_localEntities[MAX_LOCAL_ENTITIES];

/*
=============================================================================

        CVARS

=============================================================================
*/

extern vmCvar_t cg_ignore;
extern vmCvar_t cg_drawGun;
extern vmCvar_t cg_cursorHints;
extern vmCvar_t cg_hintFadeTime;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_viewsize;
extern vmCvar_t cg_letterbox;
extern vmCvar_t cg_stereoSeparation;
extern vmCvar_t cg_shadows;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawSoundOverlay;
extern vmCvar_t cg_drawScriptUsage;
extern vmCvar_t cg_drawShader;
extern vmCvar_t cg_drawSnapshot;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawCrosshairNames;
extern vmCvar_t cg_drawCrosshairPickups;
extern vmCvar_t cg_drawRewards;
extern vmCvar_t cg_hudAlpha;
extern vmCvar_t cg_hudCompassSize;
extern vmCvar_t cg_hudCompassMaxRange;
extern vmCvar_t cg_hudCompassMinRange;
extern vmCvar_t cg_hudCompassMinRadius;
extern vmCvar_t cg_hudCompassSpringyPointers;
extern vmCvar_t cg_hudObjectiveMinHeight;
extern vmCvar_t cg_hudObjectiveMaxHeight;
extern vmCvar_t cg_hudObjectiveMaxRange;
extern vmCvar_t cg_hudObjectiveMinAlpha;
extern vmCvar_t cg_hudStanceFlash_r;
extern vmCvar_t cg_hudStanceFlash_g;
extern vmCvar_t cg_hudStanceFlash_b;
extern vmCvar_t cg_hudStanceHintPrints;
extern vmCvar_t cg_hudDamageIconWidth;
extern vmCvar_t cg_hudDamageIconHeight;
extern vmCvar_t cg_hudDamageIconOffset;
extern vmCvar_t cg_hudDamageIconTime;
extern vmCvar_t cg_hudDamageIconInScope;
extern vmCvar_t cg_weaponCycleDelay;
extern vmCvar_t cg_weaponSelect;
extern vmCvar_t cg_crosshairAlpha;
extern vmCvar_t cg_crosshairAlphaMin;
extern vmCvar_t cg_crosshairDynamic;
extern vmCvar_t cg_crosshairNoGun;
extern vmCvar_t cg_brass;
extern vmCvar_t cg_marks;
extern vmCvar_t cg_lagometer;
extern vmCvar_t cg_railTrailTime;
extern vmCvar_t cg_gunX;
extern vmCvar_t cg_gunY;
extern vmCvar_t cg_gunZ;
extern vmCvar_t cg_gun_move_f;
extern vmCvar_t cg_gun_move_r;
extern vmCvar_t cg_gun_move_u;
extern vmCvar_t cg_gun_ofs_f;
extern vmCvar_t cg_gun_ofs_r;
extern vmCvar_t cg_gun_ofs_u;
extern vmCvar_t cg_gun_move_rate;
extern vmCvar_t cg_gun_move_minspeed;
extern vmCvar_t cg_centertime;
extern vmCvar_t cg_bobAmplitudeStanding;
extern vmCvar_t cg_bobAmplitudeDucked;
extern vmCvar_t cg_bobAmplitudeProne;
extern vmCvar_t cg_bobMax;
extern vmCvar_t cg_skybox;
extern vmCvar_t cg_debugProneCheck;
extern vmCvar_t cg_debugProneCheckDepthCheck;
extern vmCvar_t cg_debugposition;
extern vmCvar_t cg_debugevents;
extern vmCvar_t cg_errordecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noplayeranims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_footsteps;
extern vmCvar_t cg_tracerchance;
extern vmCvar_t cg_tracerwidth;
extern vmCvar_t cg_tracerSpeed;
extern vmCvar_t cg_tracerlength;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
extern vmCvar_t cg_chatTime;
extern vmCvar_t cg_chatHeight;
extern vmCvar_t cg_predictItems;
extern vmCvar_t cg_drawTeamOverlay;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_timescale;                   // registered as "timescale"
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;
extern vmCvar_t cg_hudFiles;
extern vmCvar_t cl_stance;
extern vmCvar_t cl_stanceTemp;
extern vmCvar_t cg_noTaunt;
extern vmCvar_t cg_voiceSpriteTime;
extern vmCvar_t cg_teamChatsOnly;
extern vmCvar_t cg_noVoiceChats;
extern vmCvar_t cg_noVoiceText;
extern vmCvar_t cl_paused;
extern vmCvar_t g_synchronousClients;
extern vmCvar_t cg_currentSelectedPlayer;
extern vmCvar_t cg_currentSelectedPlayerName;
extern vmCvar_t cg_deadbodyque;
extern vmCvar_t cg_gametype;                    // registered as "g_gametype"
extern vmCvar_t cg_norender;
extern vmCvar_t cg_animState;
extern vmCvar_t cl_waitForFire;
extern vmCvar_t cg_dumpAnims;
extern vmCvar_t cg_developer;                   // registered as "developer"
extern vmCvar_t con_minicon;
extern vmCvar_t cg_version;                     // registered as "version"
extern vmCvar_t cg_subtitles;
extern vmCvar_t cg_subtitleMinTime;
extern vmCvar_t cg_subtitleWidth;
extern vmCvar_t cg_gameMessageWidth;
extern vmCvar_t cg_gameBoldMessageWidth;
extern vmCvar_t cl_languagewarnings;
extern vmCvar_t cl_languagewarningsaserrors;
extern vmCvar_t cg_objectiveText;
extern vmCvar_t cg_scoreboardScrollStep;
extern vmCvar_t cg_descriptiveText;
extern vmCvar_t cg_r_optimize;                  // registered as "r_optimize"
extern vmCvar_t cg_r_optimizeXModels;           // registered as "r_optimizeXModels"
extern vmCvar_t bg_viewheight_standing;
extern vmCvar_t bg_viewheight_crouched;
extern vmCvar_t bg_viewheight_prone;
extern vmCvar_t bg_duck2prone_time;
extern vmCvar_t bg_prone2duck_time;
extern vmCvar_t bg_ladder_yawcap;
extern vmCvar_t bg_prone_yawcap;
extern vmCvar_t bg_prone_softyawedge;
extern vmCvar_t bg_foliagesnd_minspeed;
extern vmCvar_t bg_foliagesnd_maxspeed;
extern vmCvar_t bg_foliagesnd_slowinterval;
extern vmCvar_t bg_foliagesnd_fastinterval;
extern vmCvar_t bg_foliagesnd_resetinterval;
extern vmCvar_t bg_fallDamageMinHeight;
extern vmCvar_t bg_fallDamageMaxHeight;
extern vmCvar_t bg_debugWeaponAnim;
extern vmCvar_t bg_debugWeaponState;
extern vmCvar_t cg_shock_screenBlendTime;
extern vmCvar_t cg_shock_screenBlendFadeTime;
extern vmCvar_t cg_shock_viewKickPeriod;
extern vmCvar_t cg_shock_viewKickRadius;
extern vmCvar_t cg_shock_sound;
extern vmCvar_t cg_shock_soundFadeInTime;
extern vmCvar_t cg_shock_soundFadeOutTime;
extern vmCvar_t cg_shock_soundLoopFadeTime;
extern vmCvar_t cg_shock_soundLoopEndDelay;
extern vmCvar_t cg_shock_soundRoomType;
extern vmCvar_t cg_shock_soundWetLevel;
extern vmCvar_t cg_shock_soundModEndDelay;
extern vmCvar_t cg_shock_volume_auto;
extern vmCvar_t cg_shock_volume_menu;
extern vmCvar_t cg_shock_volume_weapon;
extern vmCvar_t cg_shock_volume_voice;
extern vmCvar_t cg_shock_volume_item;
extern vmCvar_t cg_shock_volume_body;
extern vmCvar_t cg_shock_volume_local;
extern vmCvar_t cg_shock_volume_music;
extern vmCvar_t cg_shock_volume_announcer;
extern vmCvar_t cg_shock_volume_shellshock;
extern vmCvar_t cg_shock_mouse;
extern vmCvar_t cg_shock_mouse_maxpitchspeed;
extern vmCvar_t cg_shock_mouse_maxyawspeed;
extern vmCvar_t cg_shock_mouse_sensitivityscale;
extern vmCvar_t cg_shock_mouse_fadeTime;
extern vmCvar_t cg_debuganim;
extern vmCvar_t bg_swingSpeed;
extern vmCvar_t cg_blood;
extern vmCvar_t cl_serverloadmap;
extern vmCvar_t cl_serverloadgametype;
extern vmCvar_t cl_serverloadwaiting;

/*
=============================================================================

        PROTOTYPES

=============================================================================
*/

//
// q_shared.c, q_parse.c, com_math.c
//
int         Q_stricmp( const char *s1, const char *s2 );
int         Q_stricmpn( const char *s1, const char *s2, int n );
char       *Q_strlwr( char *s1 );
char       *Q_CleanStr( char *string );
void        Q_strncpyz( char *dest, const char *src, int destsize );
int         Q_ftol( float f );
int  QDECL  Com_sprintf( char *dest, int size, const char *fmt, ... );
char       *va( const char *format, ... );
float       Com_Clamp( float min, float max, float value );
char       *Info_ValueForKey( const char *s, const char *key );
void        Com_BeginParseSession( const char *name );
void        Com_EndParseSession( void );
void        Com_SetCSV( qboolean enabled );
char       *Com_Parse( char **data_p );
char       *Com_ParseExt( char **data_p, qboolean allowLineBreaks );
char       *Com_ParseOnLine( char **data_p );
void        Com_SkipRestOfLine( int unused, char **data_p );
void        Com_UngetToken( void );
int         Com_Compress( char *data_p );

void        AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
void        YawToAxis( float yaw, vec3_t axis[3] );
void        AxisClear( vec3_t axis[3] );
void        AxisCopy( vec3_t in[3], vec3_t out[3] );
void        CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross );
void        PerpendicularVector( vec3_t dst, const vec3_t src );
void        ByteToDir( int b, vec3_t dir );
float       LerpAngle( float from, float to, float frac );
float       AngleSubtract( float a1, float a2 );
float       AngleNormalize360( float angle );
float       AngleNormalize180( float angle );
void        SinCos( float radians, float *sinOut, float *cosOut );
vec_t       VectorNormalize2( const vec3_t v, vec3_t out );
vec_t       VectorNormalize2D( float *v );
double      Distance( const vec3_t p1, const vec3_t p2 );  // keep: double, matches original codegen
float       vectoyaw( const vec3_t vec );
float       vectosignedyaw( const vec3_t vec );
float       RotationToYaw( const float v[2] );
void        RotateVector2D( vec3_t v, float degrees );
void        RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
								     float degrees );
void        AddLeanToPosition( float *pos, float yaw, float leanFrac, float leanDist, float scale );
// the destination is the middle argument
void        QuatMultiply( const float b[4], const float a[4], float out[4] );
void        MatrixMultiply( const vec3_t in1[3], const vec3_t in2[3], vec3_t out[3] );
void        MatrixMultiply43( const float in1[4][3], const float in2[4][3], float out[4][3] );
// the axis is first and the angles are the destination
void        AxisToAngles( const float axis[4][3], vec3_t angles );
void        DObjSkel2MatrixMultiply43( const float *boneMatrix, const float *mat43, float *out );

//
// cg_animtree_mp.c
//
void       *Hunk_AllocXAnimCreate( int size );
void        CGScr_LoadAnimTrees( void );
void        CG_FreeClientDObjInfo( void );
// the three below are always inlined by their callers
void        CG_SetDObjInfo( int entityNum, int eType, void *xmodel );
qboolean    CG_CheckDObjInfoMatches( int entityNum, int eType, void *xmodel );
void        CG_SafeDObjFree( int entityNum );
void        CG_FreeEntityDObjInfo( void );

//
// cg_compass_mp.c
//
// shader is unused
void        CG_DrawCompassFriendlies( float *rect, qhandle_t shader, const float *color );

//
// cg_consolecmds_mp.c
//
qboolean    CG_ConsoleCommand( void );
void        CG_InitConsoleCommands( void );

//
// cg_draw_mp.c
//
// these two are unused
int         CG_DrawFieldWidth( int x, int y, int width, int value, int charWidth,
							   int charHeight );
int         CG_DrawField( int x, int y, int width, int value, int charWidth, int charHeight,
						  qboolean dodrawpic, qboolean leftAlign );
// the rectangle is floats, not RTCW's ints
void        CG_DrawTeamBackground( float x, float y, float w, float h, float alpha, int team );
void        CG_DrawScriptUsage( void );
void        CG_AddLagometerFrameInfo( void );
void        CG_AddLagometerSnapshotInfo( snapshot_t *snap );
// y and charWidth are floats, stored as ints
void        CG_PriorityCenterPrint( const char *str, float y, float charWidth, int priority );
void        CG_CenterPrint( const char *str, float y, float charWidth );
void        CG_CheckForCursorHints( void );
// r, g and b are ignored; unused
void        CG_Fade( int r, int g, int b, int a, int startTime, int duration );
void        CG_DrawSoundOverlay( void );
void        CG_StartShakeCamera( float amplitude, int duration, float radius,
								 const vec3_t origin );
void        CG_ShakeCamera( void );
void        CG_DrawSavedScreenBlend( void );    // unused
void        CG_DrawActive( int stereoView );

//
// cg_drawtools_mp.c
//
void        CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void        G_AdjustCoordinates( float *x, float *y );
void        CG_FillRect( float x, float y, float width, float height, const float *color );
void        CG_FillRectGradient( float x, float y, float width, float height,
								 const float *color, const float *gradcolor, int gradientType );
void        CG_FilledBar( float x, float y, float w, float h, const float *startColor,
						  float *endColor, const float *bgColor, float frac, int flags );
void        CG_HorizontalPercentBar( float x, float y, float width, float height, float percent );
void        CG_DrawSides( float x, float y, float w, float h, float size );
void        CG_DrawTopBottom( float x, float y, float w, float h, float size );
void        CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void        CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void        CG_DrawRotatedPic( float x, float y, float width, float height, float angle,
							   qhandle_t hShader );
void        CG_DrawRotatedQuadPic( float x, float y, const float *verts, const float *texCoords,
								   float angle, qhandle_t hShader );
void        CG_DrawStringExt( float x, float y, const char *string, const float *setColor,
							  qboolean forceColor, qboolean shadow, float charWidth,
							  float charHeight, int maxChars, qboolean virtualScreen );
void        CG_DrawBigString( float x, float y, const char *s, float alpha );
void        CG_DrawBigStringColor( float x, float y, const char *s, const float *color );
void        CG_DrawSmallString( float x, float y, const char *s, float alpha );
void        CG_DrawSmallStringColor( float x, float y, const char *s, const float *color );
int         Q_DrawStrlen( const char *str );
void        CG_TileClear( void );
float      *CG_FadeColor( int startMsec, int totalMsec );
float      *CG_TeamColor( int team );
void        CG_GetColorForHealth( int health, int armor, vec4_t hcolor );
void        CG_ColorForHealth( vec4_t hcolor );
void        CG_UpdateCompassOrientation( void );
void        CG_UpdateCompPointerOrientation( void );
// duration is ignored
void        CG_DebugBox( const vec3_t mins, const vec3_t maxs, const float *color, int depthTest, int duration );
void        CG_DebugCircleEx( const vec3_t dir, const vec3_t org, float radius, const float *color,
							  int depthTest, int duration );
void        CG_DebugArc( const vec3_t org, float radius, float startAngle, float endAngle,
						 const float *color, int depthTest, int duration );

//
// cg_effects_mp.c
//
void        CG_RegisterImpactEffects( void );

//
// cg_ent_mp.c
//
void        CG_Beam( centity_t *cent );
void        CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime,
									   vec3_t out, vec3_t outDeltaAngles );
void        CG_AddCEntity( centity_t *cent );
void        CG_SetFrameInterpolation( void );
void        CG_AddPacketEntities( void );
void        CG_DObjUpdateInfo( DObj *obj );
void        CG_DObjSetLocalTagInternal( DObj *obj, const vec3_t origin, const vec3_t angles,
										int boneIndex );
qboolean    CG_DObjSetLocalTag( DObj *obj, int *partBits, const char *tagName,
								const vec3_t origin, const vec3_t angles );
qboolean    CG_DObjSetControlTagAngles( DObj *obj, int *partBits, const char *tagName,
										const vec3_t angles );
float      *CG_DObjGetLocalTagMatrix( DObj *obj, const char *tagName, centity_t *cent );
qboolean    CG_DObjGetWorldTagMatrix( DObj *obj, const char *tagName, centity_t *cent,
									  float *outMatrix );
qboolean    CG_DObjGetViewModelTagMatrix( DObj *obj, const char *tagName, float *outMatrix );
void        CG_CalcEntityLerpPositions( centity_t *cent );
void       *CG_GetAnimations( int eType, int entityNum );
DObj       *CG_PreProcess_GetDObj( int entityNum, int eType, qhandle_t hModel );
void        CG_Player_DoControllers( centity_t *cent, int *partBits );
void        CG_DoControllers( centity_t *cent, int *partBits );
void        CG_DObjCalcPose( centity_t *cent, DObj *obj, int *partBits );
void        CG_DObjCalcBone( DObj *obj, int boneIndex, centity_t *cent );
void        CG_DObjCalcBoneGeneric( int entityNum, int boneIndex );
void        CG_ProcessEntity( centity_t *cent );

//
// cg_event_mp.c
//
// predicted is qtrue from cg_playerstate.c, qfalse from cg_ents.c
void        CG_EntityEvent( centity_t *cent, int event, qboolean predicted );
void        CG_CheckEvents( centity_t *cent );
void        CG_CheckPreEvents( centity_t *cent );

//
// cg_hudelem_mp.c
//
void        CG_DrawHudElems( void );

//
// cg_info_mp.c
//
void        CG_LoadingString( const char *s );
// also called from the trap_R_Register* wrappers
void        CG_DrawInformation( int localized );

//
// cg_localents_mp.c
//
void            CG_InitLocalEntities( void );
void            CG_FreeLocalEntity( localEntity_t *le );
localEntity_t  *CG_AllocLocalEntity( void );
void            CG_AddLocalEntities( void );
extern int      localEntCount;

//
// cg_main_mp.c
//
int         vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5,
					int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 );
void        CG_RegisterCvars( void );
void        CG_UpdateCvars( void );
int         CG_CrosshairPlayer( void );
void QDECL  CG_Printf( const char *msg, ... );
void QDECL  CG_Error( const char *msg, ... );
void        CG_GameMessage( const char *msg );
void        CG_BoldGameMessage( const char *msg );
void        CG_DreathMessage( const char *attacker, const float *attackerColor, const char *victim,
							  const float *victimColor, const char *icon, float iconWidth,
							  float iconHeight, const float *iconColor );
void QDECL  Com_Error( int level, const char *error, ... );
void QDECL  Com_Printf( const char *msg, ... );
void QDECL  Com_DPrintf( const char *msg, ... );
const char *CG_Argv( int arg );
const char *CG_ConfigString( int index );
void        CG_StartAmbient( void );
// takes the alias name; a few callers pass a cast sfxHandle_t -- keep
void        CG_PlayClientSoundAliasByName( const char *name );
void        CG_PlayEntitySoundAliasByName( int entnum, const char *name );
// returns the sound's length in msec
int         CG_PlaySoundAliasByName( const char *name, int entnum, const vec3_t origin );
void        CG_ParseMenu( const char *menuFile, int loadMode );
qboolean    CG_Load_Menu( char **p, int loadMode );
void        CG_LoadMenus( const char *menuFile, int loadMode );
const char *CG_SafeTranslateString_Internal( const char *reference, const char *domain );
const char *CG_SafeTranslateString( const char *reference );
const char *CG_SafeTranslateHudElemString( int index );
const char *CG_TranslateMessage( const char *src, const char *reference );
void        CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void        CG_Shutdown( void );

//
// cg_marks_mp.c
//
void        CG_InitMarkPolys( void );
void        CG_FreeMarkPoly( markPoly_t *le );
// expireTime is unused
markPoly_t *CG_AllocMark( int expireTime );
void        CG_AddMarks( void );
void        CG_ImpactMark( qhandle_t markShader, const vec3_t origin, const vec3_t dir,
						   float orientation, float red, float green, float blue, float alpha,
						   qboolean alphaFade, float radius, qboolean temporary, int duration );

//
// cg_newDraw_mp.c
//
extern int  sortedTeamPlayers[8];
extern int  numSortedTeamPlayers;

// the only writer of the chat buffers; unused
void        CG_SetPrintString( int type, const char *p );
int         CG_GetSelectedPlayer( void );
void        CG_SelectNextPlayer( void );
void        CG_SelectPrevPlayer( void );
float       CG_GetValue( int ownerDraw, int type );
qboolean    CG_OwnerDrawVisible( int flags );
const char *CG_GetKillerText( void );
const char *CG_GameTypeString( void );
// the arguments are displayContextDef_t::ownerDrawItem's, including CoD's extra font
void        CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y,
						  int ownerDraw, int ownerDrawFlags, int align, float special,
						  int font, float scale, vec4_t color, qhandle_t shader, int textStyle );
void        CG_MouseEvent( int x, int y );
void        CG_EventHandling( int type );
void        CG_KeyEvent( int key, qboolean down );
qboolean    CG_KeyInterceptEvent( int key, qboolean down );
int         CG_ClientNumFromName( const char *p );
void        CG_ShowResponseHead( void );
void        CG_RunMenuScript( char **args );
void        CG_GetTeamColor( vec4_t *color );

//
// cg_players_mp.c
//
void        CG_Player( centity_t *cent );
void        CG_Corpse( centity_t *cent );
void        CG_UpdatePlayerDObj( centity_t *cent );     // unused
void        CG_ResetPlayerEntity( centity_t *cent );

//
// cg_playerstate_mp.c
//
void        CG_CheckAmmo( void );
void        CG_DamageFeedback( int yawByte, int pitchByte, int damage );
void        CG_Respawn( void );
void        CG_CheckPlayerstateEvents( playerState_t *ps, playerState_t *ops );
void        CG_CheckChangedPredictableEvents( playerState_t *ps );      // unused
void        CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );

//
// cg_predict_mp.c
//
void        CG_BuildSolidList( void );
void        CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
                      const vec3_t end, int skipNumber, int mask );
void        CG_TraceCapsule( trace_t *result, const vec3_t start, const vec3_t mins,
                             const vec3_t maxs, const vec3_t end, int skipNumber, int mask );
int         CG_PointContents( const vec3_t point, int passEntityNum, int contentMask );
void        CG_PredictPlayerState( void );

//
// cg_scoreboard_mp.c
//
qboolean    CG_DrawScoreboard( void );
qboolean    CG_ScoreboardDisplayed( void );
void        CG_ScrollScoreboardUp( void );
void        CG_ScrollScoreboardDown( void );
void        CG_RegisterScoreboardGraphics( void );

//
// cg_scr_main_mp.c
//
// the script import table is not declared here; see cg_scr_main_mp.c
int         *Scr_FarHook( const void *hooks );
// the five VM callbacks Scr_FarHook publishes, all empty in cgame
void        *Scr_GetFunction( const char **pName, int *pDeveloper );
void        *Scr_GetMethod( const char **pName, int *pDeveloper );
void        Scr_SetObjectField( int classnum, int objectNum, int fieldnum );
void        Scr_GetObjectField( int classnum, int objectNum, int fieldnum );
int         Scr_LoadRead( int len );

//
// cg_servercmds_mp.c
//
// voice chats (CG_RegisterSounds in cg_main_mp.c parses them too)
#define MAX_VOICEFILES          8
#define MAX_VOICECHATS          64
#define MAX_VOICESOUNDS         64
#define MAX_CHATSIZE            64

typedef struct voiceChat_s
{
	char id[64];
	int numSounds;
	const char  *sounds[MAX_VOICESOUNDS];
	char chats[MAX_VOICESOUNDS][MAX_CHATSIZE];
	qhandle_t sprite[MAX_VOICESOUNDS];
} voiceChat_t;
CG_ASSERT_SIZE( voiceChat_t, 4676 );

typedef struct voiceChatList_s
{
	char name[64];
	int gender;
	int numVoiceChats;
	voiceChat_t voiceChats[MAX_VOICECHATS];
} voiceChatList_t;
CG_ASSERT_SIZE( voiceChatList_t, 299336 );

extern voiceChatList_t voiceChatLists[MAX_VOICEFILES];
int         CG_ParseVoiceChats( const char *filename, voiceChatList_t *voiceChatList,
								int maxVoiceChats );
void        CG_ParseServerinfo( void );
void        CG_ParseWolfinfo( void );
void        CG_SetConfigValues( void );
void        CG_AddToTeamChat( const char *str );
void        CG_LoadVoiceChats( void );
// unused
int         CG_HeadModelVoiceChats( char *filename );
void        CG_PlayBufferedVoiceChats( void );
void        CG_CloseScriptMenu( void );
void        CG_ExecuteNewServerCommands( int latestSequence );

//
// cg_shellshock_mp.c
//
void        CG_PerturbCamera( void );
qboolean    CG_DrawShellShockSavedScreenBlend( shellshockParms_t *parmsIn,
											 int startTime, int duration );
qboolean    CG_SaveShellShockCvars( const char *name );
int         CG_LoadShellShockCvars( const char *name );
void        CG_SetShellShockParmsFromCvars( shellshockParms_t *parms );
void        CG_EndShellShockSound( void );
void        CG_EndShellShockMouse( void );
void        CG_EndShellShockCamera( void );        // unused
void        CG_EndShellShock( void );
void        CG_UpdateShellShockSound( shellshockParms_t *parms, int msec, int duration );
void        CG_UpdateShellShockMouse( shellshockParms_t *parms, int msec, int duration );
void        CG_UpdateShellShockCamera( shellshockParms_t *parms, int msec, int duration );
void        CG_UpdateShellShock( int startTime, shellshockParms_t *parms, int duration );
void        CG_UpdateShellShockSavedScreenBlend( shellshockParms_t *parms, int startTime,
												 int duration );   // unused

//
// cg_snapshot_mp.c
//
void        CG_ProcessSnapshots( void );

//
// cg_syscalls_mp.c -- system traps
// These functions are how the cgame communicates with the main game system
//
int         PASSFLOAT( float x );
float       IntAsFloat( int x );

// print message on the local console
void trap_Print( const char *fmt );

// abort the game
void trap_Error( const char *fmt );

void trap_GameMessage( const char *msg, int width );
void trap_BoldGameMessage( const char *msg, int width );
void trap_DeathMessage( const char *attacker, const float *attackerColor, const char *victim, const float *victimColor, const char *icon, float iconWidth, float iconHeight, const float *iconColor );
void trap_Subtitle( const char *msg, int duration, int channel );

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int trap_Milliseconds( void );

// console variable interaction
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void trap_Cvar_Update( vmCvar_t *cvar );
void trap_Cvar_Set( const char *var_name, const char *value );
void trap_Cvar_Setvar( vmCvar_t *cvar, const char *var_name );
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

// ServerCommand and ConsoleCommand parameter access
int trap_Argc( void );
void trap_Argv( int n, char *buffer, int bufferLength );
void trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
int trap_FS_Read( void *buffer, int len, fileHandle_t f );
int trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void trap_FS_FCloseFile( fileHandle_t f );
int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );

int trap_Com_SaveCvarsToBuffer( const char **cvars, int numCvars, char *buffer, int size );
int trap_Com_LoadCvarsFromBuffer( const char **cvars, int numCvars, char *buffer, const char *text );

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void trap_SendConsoleCommand( const char *text );

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void trap_AddCommand( const char *cmdName );

// send a string to the server over the network
void trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void trap_UpdateScreen( void );

void trap_DrawNotifyLines( int x, int yStart, float alpha, int orientation );
void trap_DrawBoldMessages( int x, int yStart, float alpha, int orientation );
void trap_DrawMiniConsole( int x, int yStart, float alpha );
void trap_DrawSubtitles( int x, int yStart, float alpha, int orientation );
void trap_DrawSay( int y );

// model collision
void trap_CM_LoadMap( const char *mapname );
int trap_CM_NumInlineModels( void );
int trap_CM_InlineModel( int index );  // 0 = world, 1+ = bmodels
int trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int contents );
int trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs, int contents );
int trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
void trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles );
void trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
void trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int trap_R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
						  const vec3_t normal, float radius, int maxPoints, polyVert_t *pointBuffer,
						  int maxFragments, markFragment_t *fragmentBuffer, qhandle_t markShader );

void trap_R_LoadWorldMap( const char *mapname );
void trap_R_FinishLoadingModels( void );
void trap_R_SetIgnorePrecacheErrors( int ignore );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t trap_R_RegisterModel( const char *name, int arg2 ); // returns rgb axis if not found
int trap_R_GetShaderFromModel( int modelHandle, int surfaceIndex );
void *trap_R_GetXModelByHandle( qhandle_t hModel );
qhandle_t trap_R_RegisterShader( const char *name, int arg2 ); // returns all white if not found
qhandle_t trap_R_RegisterShaderNoMip( const char *name, int arg2 );  // returns all white if not found

int trap_R_RegisterFont( const char *name, int pointSize, int font, int imageTrack );
int trap_R_Text_Width( const char *text, int font, float scale, int limit );
int trap_R_Text_Height( int font, float scale );
void trap_R_Text_Paint( float x, float y, int font, float scale, const float *color,
						const char *text, float adjust, int limit, int style );
void trap_R_Text_PaintWithCursor( float x, float y, int font, float scale, const float *color,
								  const char *text, int cursorPos, char cursor, int limit,
								  int style );

const char *trap_SE_TranslateReference( const char *reference );
const char *trap_SE_LocalizeMessage( const char *msg, const char *reference );
int trap_SE_PrintStrlen( const char *text );
int trap_SE_ReadCharFromString( const unsigned char **text, int *advance );

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void trap_R_ClearScene( void );
void trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int visible );
void trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density );
void trap_R_RenderScene( const refdef_t *fd );
void trap_R_SaveScreen( void );
void trap_R_BlendSavedScreen( int blend );
void trap_R_SetColor( const float *rgba ); // NULL = 1,1,1,1
void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader, const float *gradientColor, int gradientType );
void trap_R_DrawStretchPicRotate( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float angle, int hShader );
void trap_R_DrawQuadPic( const float *verts, const float *texCoords, int hShader );
void trap_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs );
void trap_R_TrackStatistics( void *stats );
int trap_R_PickShader( const float *start, const float *dir, char *shaderName, char *surfaceNames, char *contentsNames, int size );

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void trap_GetGlconfig( glconfig_t *vidConfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
int trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
int trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int trap_GetCurrentCmdNumber( void );

int trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );
void trap_SetUserCmdAimValues( const vec3_t aim );
void trap_SetUserCmdInShellshock( int inShellshock );

// aids for VM testing
void trap_PrintInt( const char *s, int i );
void trap_PrintFloat( const char *s, float f );

int trap_MemoryRemaining( void );
int trap_Key_IsDown( int keynum );
int trap_Key_GetCatcher( void );
void trap_Key_SetCatcher( int catcher );
int trap_Key_GetKey( const char *binding );
int trap_PC_AddGlobalDefine( char *define );
int trap_PC_LoadSource( const char *filename );
int trap_PC_FreeSource( int handle );
int trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
void trap_Com_LoadSoundAliases( const char *mapname );
int trap_Com_SoundAliasString( const char *alias );
const char *trap_Com_PickSoundAlias( const char *alias );
const char *trap_Com_GetSoundAlias( int index );
int trap_MSS_PlaySoundAlias( const char *alias, int entnum, const float *origin, int timeOffset );
void trap_MSS_PlayBlendedSoundAliases( const char *alias1, const char *alias2, float blend, int entnum, const float *origin, int timeOffset );
int trap_SurfaceTypeFromName( const char *name );
const char *trap_SurfaceTypeToName( int surfaceType );
int trap_RealTime( int *qtime );
void trap_SnapVector( float *v );
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
int trap_CIN_StopCinematic( int handle );
int trap_CIN_RunCinematic( int handle );
void trap_CIN_DrawCinematic( int handle );
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h );
int trap_R_GetEntityToken( char *buffer, int bufferSize );
int trap_hunkUsed( void );
int trap_UI_LoadMenu( const char *menuname );
int trap_UI_Popup( const char *menuname );
void trap_UI_ClosePopup( const char *menuname );
void trap_UI_CloseAllMenus( void );
const char *trap_UI_GetMapDisplayName( const char *mapname );
const char *trap_UI_GetGameTypeDisplayName( const char *gametype );
const char *trap_CL_GetServerIPAddress( void );
void trap_XAnimPrecache( const char *animName );
void *trap_XAnimCreateAnims( const char *name, int size );
void trap_XAnimCreate( void *anims, unsigned short animIndex, const char *animName );
void *trap_XAnimCreateTree( void *anims );
void trap_XAnimBlend( void *anims, int animIndex, const char *animName, unsigned short firstChild, unsigned short numChildren, unsigned short flags );
void trap_DObjFree( DObj *obj );
void trap_XAnimClearGoalWeight( void *tree, int animIndex, float goalTime );
void trap_XAnimClearTreeGoalWeights( void *tree, int animIndex, float goalWeight );
void trap_XAnimClearTreeGoalWeightsStrict( void *tree, scr_anim_t anim, float goalWeight );
void trap_XAnimSetCompleteGoalWeightKnob( void *tree, unsigned short animIndex, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
int trap_XAnimSetCompleteGoalWeightKnobAll( void *tree, scr_anim_t anim, scr_anim_t rootAnim, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimClearAnimChildren( void *tree, scr_anim_t anim, float goalTime );
void trap_XAnimSetGoalWeight( void *tree, unsigned short animIndex, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimSetCompleteGoalWeight( void *tree, scr_anim_t anim, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimSetAnimRate( void *tree, scr_anim_t anim, float rate );
int trap_XAnimIsLooped( scr_anim_t anim );
int trap_XAnimIsLoopedIndex( void *anims, int animIndex );
void trap_XAnimSetTime( void *tree, int animIndex, float time );
float trap_XAnimGetTime( void *tree, scr_anim_t anim );
float trap_XAnimGetWeight( void *tree, scr_anim_t anim );
void trap_DObjInvalidateSkels( void );
void trap_DObjUpdateClientInfo( DObj *obj, float dtime );
int trap_XAnimGetDeferredNotifies( void *notifiesOut );
void trap_DObjCalcAnim( DObj *obj, const int *partBits );
void trap_DObjDisplayAnim( DObj *obj );
void trap_XAnimCalcDelta( void *tree, unsigned short animIndex, float *deltaRot, float *deltaMove, int animIndexEnd );
void trap_XAnimCalcAbsDelta( void *tree, int animIndex, float *deltaRot, float *deltaMove );
void trap_XAnimGetRelDelta( scr_anim_t anim, float *deltaRot, float *deltaMove,
							float startTime, float endTime );
void trap_XAnimGetAbsDelta( scr_anim_t anim, float *deltaRot, float *deltaMove, float time );
void *trap_DObjGetMatrixArray( DObj *obj );
// one 64-byte matrix per bone
#define DOBJ_BONEMATRIX( base, boneIndex )  ( (float *)( (byte *)( base ) + ( ( boneIndex ) << 6 ) ) )
void *trap_DObjGetRotTransArray( DObj *obj );
int trap_DObjSetRotTransIndex( DObj *obj, const int *partBits, int boneIndex );
int trap_DObjSetControlRotTransIndex( DObj *obj, const int *partBits, int boneIndex );
const char *trap_XAnimGetAnimName( scr_anim_t anim );
DObj *trap_GetClientDObj( int handle );
void trap_DObjCreateDirect( void *models, unsigned short numModels, void *tree, DObj *obj );
// also called by game_mp/bg_animation.c
void trap_DObjCreate( void *models, unsigned short modelCount, void *tree, int handle );
void trap_SafeDObjFree( int handle, int localClientNum );
int trap_XAnimGetAnims( const int *p );
int trap_DObjGetSkelSize( DObj *obj );
int trap_DObjCreateSkelForBone( DObj *obj, int boneIndex );
int trap_DObjCreateSkelForBones( DObj *obj, const int *partBits );
void trap_DObjGetHierarchyBits( DObj *obj, int boneIndex, int *partBits );
void trap_DObjCalcSkel( DObj *obj, const int *partBits );
int trap_XModelExists( const char *modelName );
int trap_DObjNumBones( DObj *obj );
int trap_DObjGetBoneIndex( DObj *obj, const char *tagName );
const char *trap_DObjGetBoneName( DObj *obj, int boneIndex );
void trap_DObjBuildPartCollisionTable( DObj *obj, float **partBounds );
void *trap_DObjGetTree( DObj *obj );
int trap_XAnimIsPrimitive( scr_anim_t anim );
int trap_XAnimGetLength( void *tree, unsigned short animIndex );
int trap_XAnimHasFinished( void *tree, int animIndex );
int trap_XAnimGetNumChildren( scr_anim_t anim );
scr_anim_t trap_XAnimGetChildAt( scr_anim_t anim, int childIndex );
int trap_XAnimGetAnimTreeSize( void *tree );
void trap_XAnimCloneAnimTree( const void *from, void *to );
void trap_DObjDumpInfo( DObj *obj );
void trap_StatMon_Warning( int type, int duration, const char *material );
void trap_StatMon_GetStatsArray( void *statsOut, int *countOut );
void *trap_Z_MallocInternal( int size );
void trap_Z_FreeInternal( void *ptr );
void trap_AddDebugLine( const vec3_t start, const vec3_t end, const vec3_t color,
						int depthTest, int duration );
void *trap_GetWeaponInfoMemory( int size, int *pPrevOwner );
void trap_FreeWeaponInfoMemory( int slot );
void *trap_Hunk_AllocInternal( int size );  // 32-byte aligned
void *trap_Hunk_AllocLowInternal( int size );  // 32-byte aligned
void *trap_Hunk_AllocAlignInternal( int size, int alignment );
void *trap_Hunk_AllocLowAlignInternal( int size, int align );
int trap_Scr_GetNumVars( void );
int trap_Scr_GetNumThreads( void );
int trap_Scr_GetStringUsage( void );
void trap_MSS_SetListener( int clientNum, const vec3_t origin, vec3_t axis[3] );
void trap_MSS_UpdateLoopingSounds( void );
void trap_MSS_StopSounds( int flags );
void trap_MSS_PlayMusicAlias( const char *alias );
void trap_MSS_StopBackground( int fadeTime );
void trap_MSS_PlayAmbientAlias( const char *alias, int fadeTime );
void trap_MSS_FadeAllSounds( float volume, int fadeTime );
void trap_MSS_FadeSelectSounds( const float *channelVolumes, int fadeTime );
void trap_MSS_SetEnvironmentEffects( const char *roomType, float dryLevel, int fadeTime );
int trap_MSS_GetSoundOverlay( int mode, void *channels, int maxChannels, int *cpuOut );
void trap_SetUserCmdValue( int userCmdValue, float sensitivityScale );
void trap_SetClientLerpOrigin( float x, float y, float z );
void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void trap_Key_SetBinding( int keynum, const char *binding );
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
int trap_FX_RegisterEffect( const char *name );
int trap_FX_GetBoneIndex( int entityNum, const char *boneName );
void trap_FX_PlaySimpleEffect( const char *name, const vec3_t origin );
void trap_FX_PlayEffect( const char *name, const vec3_t origin, const vec3_t dir );
void trap_FX_PlayBoltedEffect( const char *name, const vec3_t origin, int boltInfo, int entityNum );
void trap_FX_PlaySimpleEffectID( int id, const vec3_t origin );
void trap_FX_PlayEffectID( int id, const vec3_t origin, const vec3_t dir );
void trap_FX_PlayOrientedEffectID( int id, const vec3_t origin, vec3_t axis[3], const int *boltInfo );
void trap_FX_AddScheduledEffects( void );
int trap_FX_Init( void );
int trap_FX_FreeAll( void );
int trap_FX_FreeEffects( void );
void trap_FX_AdjustTime( int time );
void trap_FX_AdjustCamera( const refdef_t *refdef );
int trap_Unused236( int arg );  // no engine handler
int trap_Unused237( int arg );  // no engine handler
int trap_Unused238( int arg );  // no engine handler
int trap_Unused239( int arg );  // no engine handler
void trap_SetMouseMaxSpeeds( float maxPitchSpeed, float maxYawSpeed );
void trap_CL_FirstSnapshot( void );

//
// cg_view_mp.c
//
void        CG_FxSetTestPosition( void );
void        CG_FxRestart( void );
void        CG_FxTest( void );
void        CG_DrawActiveFrame( int serverTime, int stereoView, int demoPlayback,
								int cubemapShot, int cubemapSize );

//
// cg_weapons_mp.c
//
void        CG_RegisterWeapon( int weaponNum );
void        CG_UpdateHandViewmodels( const char *handModel );
void        CG_FreeWeapons( void );
void        CG_RegisterItemVisuals( int itemNum );
void        CG_RegisterItems( void );
void        CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent,
								int drawGun );
void        CG_AddViewWeapon( playerState_t *ps );
void        CG_DrawWeaponSelect( void );
void        CG_AltWeapon_f( void );
void        CG_NextWeapon_f( void );
void        CG_PrevWeapon_f( void );
void        CG_Weapon_f( void );
void        CG_WeaponSlot_f( void );
void        CG_CycleWeap( qboolean next, qboolean checkAmmo );
void        CG_OutOfAmmoChange( void );
void        CG_RailTrail( int type, const vec3_t end, const vec3_t start );
void        CG_FireWeapon( centity_t *cent, entityState_t *es, int event, int tagIndex );
void        CG_EjectWeaponBrass( entityState_t *es, int event );
void        CG_DrawTracer( const vec3_t start, const vec3_t end );
// keep: parameter order matches original codegen
void        CG_BulletHitEvent( int entityNum, const vec3_t origin, int surfaceType, int event,
							   const vec3_t dir, const vec3_t dir2 );
void        CG_BulletHitClientEvent( int entityNum, const vec3_t origin, int surfaceType,
									 int event );

#endif  // __CG_LOCAL_H__
