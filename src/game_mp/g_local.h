// g_local.h -- local definitions for game module

#ifndef __G_LOCAL_H__
#define __G_LOCAL_H__

#include "../universal/q_shared.h"
#include "g_public.h"
#include "bg_public.h"

#define G_ASSERT_SIZE( type, bytes ) COD1_ASSERT_SIZE( type, bytes )
#define G_ASSERT_OFFSET( type, member, ofs ) COD1_STATIC_ASSERT( offsetof( type, member ) == ( ofs ), #type "." #member )

// G_InitGame prints this plus the compile date
#define GAMEVERSION             "main"

// not in universal/q_shared.h
#define MAX_STRING_CHARS        1024
#define MAX_TOKEN_CHARS         1024

#define BODY_QUEUE_SIZE         8

#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048

// one per playerState_t.objective[] slot
#define MAX_OBJECTIVES          16

// level.objectives[].state
#define OBJST_EMPTY             0
#define OBJST_ACTIVE            1
#define OBJST_INVISIBLE         2
#define OBJST_CURRENT           4

// size of the level.notifyWatch queue
#define MAX_NOTIFY_WATCHES      256

// models attached to an entity or client by tag
#define MAX_ATTACHED_MODELS     6

#define EVENT_VALID_MSEC        300

// gentity->flags
#define FL_GODMODE              0x00000001
#define FL_NOTARGET             0x00000002
#define FL_TEAMSLAVE            0x00000004      // not the first on the team
#define FL_NO_KNOCKBACK         0x00000008
#define FL_DROPPED_ITEM         0x00000010
#define FL_TOGGLE               0x00000080      // ent is toggling (doors use this for ex.)
#define FL_SOFTACTIVATE         0x00000100      // ent has been activated while 'walking' (doors use this too for ex.)
#define FL_NODRAW               0x00001000      // set by hide(), sent to clients as EF_NODRAW
#define FL_SUPPORTS_LINKTO      0x00002000      // linkto() is allowed on this entity
#define FL_GRENADE_TOUCH_DAMAGE 0x00008000      // set by enableGrenadeTouchDamage()
#define FL_NO_GRENADE_BOUNCE    0x00010000      // missiles pass through it, set by disableGrenadeBounce()

// config strings
#define CS_GAME_VERSION         2
#define CS_MUSIC                3
#define CS_MESSAGE              4
#define CS_TEAMSCORE_AXIS       5
#define CS_TEAMSCORE_ALLIES     6
#define CS_ITEMS                8       // hex string of bits that tell which items are present
#define CS_NORTHYAW             11
#define CS_FOGVARS              12
#define CS_LEVEL_START_TIME     13
#define CS_MOTD                 14
#define CS_VOTE_TIME            15
#define CS_VOTE_STRING          16
#define CS_VOTE_YES             17
#define CS_VOTE_NO              18
#define CS_SCORES               20      // the "winner" info string
#define CS_STATUSICONS          21
#define CS_STATUSICONS_COUNT    8
#define CS_HEADICONS            29
#define CS_HEADICONS_COUNT      15
#define CS_LOCATIONS            44
#define CS_TAGS                 108
#define CS_TAGS_COUNT           32
#define CS_MODELS               268
#define CS_MODELS_COUNT         256
#define CS_SOUNDS               524
#define CS_SOUNDS_COUNT         256
#define CS_EFFECTS              780
#define CS_EFFECTS_COUNT        64
#define CS_FX                   844
#define CS_FX_COUNT             256
#define CS_SHELLSHOCKS          1100
#define CS_SHELLSHOCKS_COUNT    16
#define CS_MENUS                1180
#define CS_MENUS_COUNT          32
#define CS_HINTSTRINGS          1212
#define CS_HINTSTRINGS_COUNT    32
#define CS_LOCALIZED_STRINGS    1244
#define CS_LOCALIZED_STRINGS_COUNT 256
#define CS_SHADERS              1500
#define CS_SHADERS_COUNT        128

// trap_SendConsoleCommand exec_when
#define EXEC_NOW                0       // don't return until completed
#define EXEC_INSERT             1       // insert at current position, but don't run yet
#define EXEC_APPEND             2       // add to end of the command buffer (normal case)

// G_Say modes
#define SAY_ALL                 0
#define SAY_TEAM                1
#define SAY_TELL                2

// cursor hints, sent in entityState_t.dmgFlags of a usable entity.
// past the named hints come one weapon and one ammo pickup hint per weapon
#define HINT_INHERIT            ( -1 )  // "cursorhint" "HINT_INHERIT"
#define HINT_ACTIVATE           2
#define HINT_DOOR               4
#define HINT_DOOR_LOCKED        5
#define HINT_MG42               6
#define HINT_HEALTH             7
#define HINT_LADDER             8
#define HINT_WEAPON_FIRST       10      // + weapon - 1
#define HINT_NUM_HINTS          11      // number of named hints
#define HINT_AMMO_FIRST         74      // + weapon - 1
#define HINT_STRING_NONE        255     // entityState_t.scale with no hint string

#define FOFS( x )               ( (int)&( ( (gentity_t *)0 )->x ) )

// movers are things like doors, plats, buttons, etc
typedef enum moverState_e {
	MOVER_POS1        = 0x0,
	MOVER_POS2        = 0x1,
	MOVER_POS3        = 0x2,
	MOVER_1TO2        = 0x3,
	MOVER_2TO1        = 0x4,
	MOVER_2TO3        = 0x5,
	MOVER_3TO2        = 0x6,
	MOVER_POS1ROTATE  = 0x7,
	MOVER_POS2ROTATE  = 0x8,
	MOVER_1TO2ROTATE  = 0x9,
	MOVER_2TO1ROTATE  = 0xA,
} moverState_t;

typedef enum team_e {
	TEAM_FREE       = 0x0,
	TEAM_AXIS       = 0x1,
	TEAM_ALLIES     = 0x2,
	TEAM_SPECTATOR  = 0x3,

	TEAM_NUM_TEAMS  = 0x4,
} team_t;

typedef enum sessionState_e {
	STATE_PLAYING      = 0x0,
	STATE_DEAD         = 0x1,
	STATE_SPECTATOR    = 0x2,
	STATE_INTERMISSION = 0x3,
} sessionState_t;

typedef enum clientConnected_e {
	CON_DISCONNECTED = 0x0,
	CON_CONNECTING   = 0x1,
	CON_CONNECTED    = 0x2,
} clientConnected_t;

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;
typedef struct gitem_s gitem_t;
typedef struct entityLinkInfo_s entityLinkInfo_t;


struct gentity_s {
	entityState_t s;                        // communicated by server to clients
	entityShared_t r;                       // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	// r.worldflags in RTCW; here it starts the game-private part
	int worldflags;                         // worldspawn's spawnflags

	gclient_t       *client;                // NULL if not a client
	struct turret_s *turret;                // turret_t, for turret entities

	byte inuse;
	byte physicsObject;                     // if true, it can be pushed by movers and fall off edges

	// door sound aliases, set by DoorSetSounds
	byte soundCloseEnd;                     // door_close_end
	byte soundOpening;                      // door_opening
	byte soundClosing;                      // door_closing
	byte soundOpenEnd;                      // door_open_end
	byte soundLoop;                         // s.loopSound while a mover is moving; never set
	byte soundOpenLoop;                     // door_open_loop
	byte soundCloseLoop;                    // door_close_loop
	byte soundLocked;                       // door_locked
	byte soundOpeningQuiet;                 // door_opening_quiet
	byte soundOpenQuietEnd;                 // door_open_quiet_end
	byte soundClosingQuiet;                 // door_closing_quiet
	byte soundCloseQuietEnd;                // door_close_quiet_end
	byte noise_index;                       // pickup / hurt sound alias
	byte watertype;
	byte waterlevel;
	byte takedamage;
	byte active;
	byte nopickup;
	byte moverState;                        // moverState_t, stored as a byte
	byte model;                             // model index
	unsigned short classname;               // set in QuakeEd
	int spawnflags;                         // set in QuakeEd
	int flags;                              // FL_* variables
	int eventTime;                          // events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;
	float physicsBounce;                    // 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;                           // brushes with this content value will be collided against when moving
	int lastFrameNum;                       // so an entity is only run once per frame

	gentity_t       *parent;
	gentity_t       *nextTrain;             // also the target_location chain
	gentity_t       *prevTrain;

	vec3_t pos1;
	vec3_t pos2;
	vec3_t pos3;                            // also the damage point set by G_Damage

	unsigned short message;
	byte pad_0x1CA[2];
	int timestamp;                          // trigger_hurt re-trigger time, etc
	float angle;                            // "degrees" key
	unsigned short target;
	unsigned short targetname;
	unsigned short team;                    // "teamname" key
	byte pad_0x1DA[2];
	gentity_t       *target_ent;

	float speed;                            // also the "time" key
	float closespeed;                       // for movers that close at a different speed than they open
	vec3_t movedir;

	int gDuration;
	int gDurationBack;

	int nextthink;
	void ( *think )( gentity_t *self );
	void ( *reached )( gentity_t *self );                                   // movers call this when hitting endpoint
	void ( *blocked )( gentity_t *self, gentity_t *other );
	void ( *touch )( gentity_t *self, gentity_t *other, int touchMode );
	void ( *use )( gentity_t *self, gentity_t *other, gentity_t *activator );
	void ( *pain )( gentity_t *self, gentity_t *attacker, int damage, const float *point, int mod, const float *dir, int hitLoc );
	void ( *die )( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int weapon, const float *dir, int hitLoc );

	byte reserved_0x21C[4];
	void ( *controller )( gentity_t *self, unsigned int *partBits );        // DObj bone controller
	int painDebounceTime;
	int fly_sound_debounce_time;            // wind tunnel
	int last_move_time;

	int health;
	int maxHealth;

	int damage;                             // "dmg" key
	int splashDamage;
	int splashMinDamage;
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int count;                              // also the "shard" key

	gentity_t       *chain;
	gentity_t       *enemy;                 // the last attacker, set by G_Damage
	gentity_t       *activator;
	gentity_t       *teamchain;             // next entity in team
	gentity_t       *teammaster;            // master of the team

	// timing variables
	float wait;
	float random;

	int radius;
	float delay;

	int TargetFlag;
	float duration;
	vec3_t rotate;                          // also the damage direction set by G_Damage
	vec3_t TargetAngles;

	gitem_t         *item;                  // for bonus items

	vec3_t dl_color;                        // "color" / "_color" keys

	int key;

	float harc;
	float varc;

	int missionLevel;                       // mission we are currently trying to complete

	qboolean is_dead;
	int start_size;
	int end_size;
	byte reserved_0x2C4[4];
	unsigned short spawnitem;
	byte pad_0x2CA[2];
	int clipCount;                          // rounds left in a dropped weapon's clip
	int grenadeExplodeTime;                 // we've caught a grenade, which was due to explode at this time
	int grenadeFired;                       // the grenade entity we last fired
	int mg42ClampTime;                      // time to wait before an AI decides to ditch the mg42
	unsigned short track;
	byte reserved_0x2DE[6];

	entityLinkInfo_t *linkInfo;             // set while linked to another entity
	gentity_t       *firstChild;            // head of the list of entities linked to this one
	byte attachModelIndex[MAX_ATTACHED_MODELS];
	unsigned short attachTagName[MAX_ATTACHED_MODELS];       // const strings
	byte pad_0x2FE[2];

	int spawnCount;                         // bumped by G_FreeEntity so stale references can be detected
	gentity_t       *nextFree;              // free list link

	int voiceChatSquelch;
	int voiceChatPreviousTime;
	int lastBurnedFrameNumber;
};
G_ASSERT_SIZE( gentity_t, 788 );
G_ASSERT_OFFSET( gentity_t, worldflags, 0x154 );
G_ASSERT_OFFSET( gentity_t, client, 0x158 );
G_ASSERT_OFFSET( gentity_t, nopickup, 0x173 );
G_ASSERT_OFFSET( gentity_t, fly_sound_debounce_time, 0x228 );
G_ASSERT_OFFSET( gentity_t, last_move_time, 0x22C );
G_ASSERT_OFFSET( gentity_t, TargetFlag, 0x278 );
G_ASSERT_OFFSET( gentity_t, is_dead, 0x2B8 );
G_ASSERT_OFFSET( gentity_t, grenadeExplodeTime, 0x2D0 );
G_ASSERT_OFFSET( gentity_t, track, 0x2DC );
G_ASSERT_OFFSET( gentity_t, lastBurnedFrameNumber, 0x310 );

// client data that stays across respawns: this is the part of gclient_t
// that ClientSpawn() doesn't clear
typedef struct clientSession_t {
	sessionState_t sessionState;    // script field "sessionstate"
	int forceSpectatorClient;       // script field "spectatorclient"
	int statusIcon;                 // script field "statusicon"
	int archiveTime;                // script field "archivetime"
	int score;                      // script field "score"
	int deaths;                     // script field "deaths"
	unsigned short pers;            // script field "pers", a script object
	byte pad_0x1A[2];
	clientConnected_t connected;
	usercmd_t cmd;                  // we would lose angles if not persistant
	usercmd_t oldcmd;               // previous command processed by pmove()
	qboolean localClient;           // true if "ip" info key is "localhost"
	qboolean predictItemPickup;     // based on cg_predictItems userinfo
	qboolean pmoveFixed;
	char netname[32];
	int handicap;                   // script field "handicap"
	int maxHealth;                  // script field "maxhealth"
	int enterTime;                  // level.time the client entered the game
	int connectTime;                // level.time the client first connected to the server
	int voteCount;                  // to prevent people from constantly calling votes
	int teamVoteCount;              // to prevent people from constantly calling votes
	int complaints;                 // number of complaints lodged against this client
	int complaintClient;            // able to lodge complaint against this client
	int complaintEndTime;           // until this time has expired
	int viewmodel;                  // model index, setViewModel()
	qboolean teamInfo;              // send team overlay updates?
	int clientNum;
	team_t sessionTeam;             // script field "sessionteam"
	int modelIndex;
	int attachModelIndex[MAX_ATTACHED_MODELS];
	int attachTagIndex[MAX_ATTACHED_MODELS];
	char name[32];                  // script field "name", the raw userinfo name
} clientSession_t;
G_ASSERT_SIZE( clientSession_t, 260 );
G_ASSERT_OFFSET( clientSession_t, connectTime, 0x88 );
G_ASSERT_OFFSET( clientSession_t, teamVoteCount, 0x90 );
G_ASSERT_OFFSET( clientSession_t, teamInfo, 0xA4 );

struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t ps;               // communicated by server to clients

	// the rest of the structure is private to game
	clientSession_t sess;

	int spectatorClient;            // for chasecam and follow mode
	qboolean noclip;
	qboolean ufo;
	qboolean frozen;                // set by freezeControls()

	int lastCmdTime;                // level.time of last usercmd_t, for EF_CONNECTION
	int buttons;
	int oldbuttons;
	int latched_buttons;
	int wbuttons;
	int oldwbuttons;
	int latched_wbuttons;
	vec3_t oldOrigin;

	// the angles the weapon is fired along
	float fGunPitch;
	float fGunYaw;
	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_blood;               // damage taken out of health
	vec3_t damage_from;             // origin for vector calculation
	qboolean damage_fromWorld;      // if true, don't use the damage_from vector
	int accurateCount;
	int accuracy_shots;             // total number of shots
	int accuracy_hits;              // total number of hits
	int inactivityTime;             // kick players when time > this
	qboolean inactivityWarning;     // qtrue if the five second warning has been given
	int switchTeamTime;             // time the player switched teams
	float currentAimSpreadScale;    // ps.aimSpreadScale / 255
	gentity_t       *persistantPowerup;
	int portalID;
	int dropWeaponTime;             // last time a weapon was dropped
	byte reserved_0x2250[12];
	int PCSpecialPickedUpCount;
	gentity_t       *lookatEnt;     // isLookingAt() target
	int iLastCompassPlayerInfoEnt;  // where G_GetNonPVSFriendlyInfo resumes its scan
	int pingTime;
	// view kick, sway and recoil state for the bg weapon code
	int viewKickStartTime;
	float viewKickYaw;
	float viewKickPitch;
	vec3_t previousViewAngles;      // last frame's view angles; the delta drives the sway
	vec3_t swayOffsets;
	vec3_t swayAngles;              // the smoothed sway angles
	vec3_t moveOffset;
	float idleScale;
	float recoilPitch;
	float recoilYaw;
	float recoilRoll;
	float recoilPitchSpeed;
	float recoilYawSpeed;
	int recoilState;
};
G_ASSERT_SIZE( gclient_t, 8900 );
G_ASSERT_OFFSET( gclient_t, fGunPitch, 0x220C );
G_ASSERT_OFFSET( gclient_t, fGunYaw, 0x2210 );
G_ASSERT_OFFSET( gclient_t, accurateCount, 0x2228 );
G_ASSERT_OFFSET( gclient_t, switchTeamTime, 0x223C );
G_ASSERT_OFFSET( gclient_t, currentAimSpreadScale, 0x2240 );
G_ASSERT_OFFSET( gclient_t, persistantPowerup, 0x2244 );
G_ASSERT_OFFSET( gclient_t, dropWeaponTime, 0x224C );
G_ASSERT_OFFSET( gclient_t, PCSpecialPickedUpCount, 0x225C );
G_ASSERT_OFFSET( gclient_t, lookatEnt, 0x2260 );
G_ASSERT_OFFSET( gclient_t, iLastCompassPlayerInfoEnt, 0x2264 );
G_ASSERT_OFFSET( gclient_t, pingTime, 0x2268 );

// a trigger touch queued by G_TouchTriggers and notified in G_RunFrame;
// the spawnCounts catch either entity being freed in between
typedef struct notifyWatch_t {
	unsigned short entNum;          // the trigger
	unsigned short otherEntNum;     // the entity that touched it
	int spawnCount;                 // spawnCount of entNum when queued
	int otherSpawnCount;            // spawnCount of otherEntNum when queued
} notifyWatch_t;
G_ASSERT_SIZE( notifyWatch_t, 12 );

//
// this structure is cleared as each map is entered
//
typedef struct level_locals_s {
	gclient_t       *clients;       // [maxclients]

	gentity_t       *gentities;
	int gentitySize;
	int num_entities;               // current number, <= MAX_GENTITIES

	gentity_t       *freeListHead;  // free list for G_Spawn / G_FreeEntity
	gentity_t       *freeListTail;

	int logFile;
	qboolean spawning;              // qtrue while G_InitGame runs

	objective_t objectives[MAX_OBJECTIVES];

	// store latched cvars here that we want to get at often
	int maxclients;
	int framenum;
	int time;                       // in msec
	int previousTime;               // so movers can back up when blocked
	int frameTime;
	int startTime;                  // level.time the map was started

	int teamScores[TEAM_NUM_TEAMS];
	int lastTeamLocationTime;       // last time of client team location update
	int scoreboardChanged;
	int clientNameMode;             // 0 = auto_change, 1 = manual_change
	int numConnectedClients;
	int sortedClients[MAX_CLIENTS]; // sorted by score

	// voting state
	char voteString[MAX_STRING_CHARS];
	char voteDisplayString[MAX_STRING_CHARS];
	int voteTime;                   // level.time vote was called
	int voteExecuteTime;            // time the vote is executed
	int voteYes;
	int voteNo;
	int numVotingClients;           // set by CalculateRanks

	// team voting state
	char teamVoteString[2][MAX_STRING_CHARS];
	int teamVoteTime[2];            // level.time vote was called
	int teamVoteYes[2];
	int teamVoteNo[2];

	// spawn variables
	qboolean spawningMapEntities;   // the G_Spawn*() functions are valid
	int numSpawnVars;
	char            *spawnVars[MAX_SPAWN_VARS][2];  // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	int matchState;                 // set and read by the server through vmMain
	qboolean locationLinked;        // target_locations get linked
	gentity_t       *locationHead;  // head of the location list
	int itemCue[32];                // dropped item slots, see GetFreeCueSpot

	float fogOpaqueDist;
	float fogOpaqueDistSq;
	int remapCount;
	int playerCloneCursor;          // next body slot for G_SpawnPlayerClone

	notifyWatch_t notifyWatch[MAX_NOTIFY_WATCHES];
	int numNotifyWatches;

	int exitRequest;                // 0 none, 1 map_restart, 2 exitLevel
	int radiusDamageIgnorePlayers;  // set while the script radiusDamage() runs
	int scriptIgnoreRadiusDamage;   // the entity radiusDamage() excludes

	// the cvar values last pushed for the player bounds and view heights
	float playerSizeWidth;
	float playerSizeHeight;
	float viewHeightStanding;
	float viewHeightCrouched;
	float viewHeightProne;
	int registeredItemsDirty;       // forces SaveRegisteredItems
} level_locals_t;
G_ASSERT_SIZE( level_locals_t, 0x2A14 );
G_ASSERT_OFFSET( level_locals_t, remapCount, 0x1DE4 );

extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];
extern gclient_t g_clients[MAX_CLIENTS];

//
// g_main_mp.c
//
int vmMain( int command, int arg0, int arg1, int arg2, int arg3 );
void G_Printf( const char *fmt, ... );
void G_DPrintf( const char *fmt, ... );
void G_Error( const char *fmt, ... );
void G_Error_Localized( const char *fmt, ... );
void G_CheckForCursorHints( gentity_t *ent );
void G_CheckForPreventFriendlyFire( gentity_t *ent );
void G_FindTeams( void );
void G_RegisterCvars( void );
void G_UpdateCvars( void );
void G_FreeEntities( void );
void G_InitGame( int levelTime, int randomSeed, int restart, int matchState );
void G_ShutdownGame( int restart );
void Com_Error( int level, const char *error, ... );
void Com_Printf( const char *msg, ... );
void Com_DPrintf( const char *msg, ... );
void SendScoreboardMessageToAllIntermissionClients( void );
int SortRanks( const void *a, const void *b );
void CalculateRanks( void );
void ExitLevel( void );
void G_LogPrintf( const char *fmt, ... );
void CheckVote( void );
void G_UpdateObjectiveToClients( void );
void G_UpdateHudElemsToClients( void );
void G_RunThink( gentity_t *ent );
void DebugDumpAnims( void );
void G_XAnimUpdateEnt( gentity_t *ent );
void G_XAnimUpdate( void );
void G_RunFrameForEntity( gentity_t *ent );
void G_RunFrame( int levelTime );

// q_shared.c / com_shared.c
void Swap_Init( void );
int Com_sprintf( char *dest, int size, const char *fmt, ... );
char *va( const char *format, ... );
void Info_SetValueForKey( char *s, const char *key, const char *value );
qboolean Com_BitCheck( const int array[], int bitNum );
extern int com_randSeed;

// com_math.c -- note the destination is the middle argument
void QuatMultiply( const float b[4], const float a[4], float out[4] );

// bg_misc.c -- the player bounding box
extern vec3_t playerMins;
extern vec3_t playerMaxs;

// bg_weapons.c / bg_animation.c
void BG_SetupWeaponInfo( void );
qboolean BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps, int arg );

// one entry of the array trap_DObjGetRotTransArray returns
typedef struct DObjAnimMat_s {
	vec4_t quat;
	float accumulatedWeight;
	vec3_t translation;
} DObjAnimMat;

//
// g_syscalls_mp.c
//
// engine objects the game module only hands back to the engine:
// a model, a script anim tree's animations, and an anim tree
typedef struct XModel_s XModel;
typedef struct XAnim_s XAnim;
typedef struct XAnimTree_s XAnimTree;

void dllEntry( int ( QDECL *syscallptr )( int arg,... ) );
float IntAsFloat( int x );
int PASSFLOAT( float x );
void trap_Printf( const char *fmt );
void trap_Error( const char *fmt );
void trap_Error_Localized( const char *fmt );
int trap_Milliseconds( void );
int trap_Argc( void );
void trap_Argv( int n, char *buffer, int bufferLength );
void *trap_Hunk_AllocInternal( int size );
void *trap_Hunk_AllocLowInternal( int size );
void *trap_Hunk_AllocAlignInternal( int size, int align );
void *trap_Hunk_AllocLowAlignInternal( int size, int align );
void *trap_Hunk_AllocateTempMemoryInternal( int size );
void trap_Hunk_FreeTempMemoryInternal( void *buf );
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void trap_FS_Read( void *buffer, int len, fileHandle_t f );
int trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void trap_FS_Rename( const char *from, const char *to );
void trap_FS_FCloseFile( fileHandle_t f );
int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
void trap_SendConsoleCommand( int exec_when, const char *text );
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void trap_Cvar_Update( vmCvar_t *cvar );
void trap_Cvar_Set( const char *var_name, const char *value );
int trap_Cvar_VariableIntegerValue( const char *var_name );
float trap_Cvar_VariableValue( const char *var_name );
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient );
void trap_DropClient( int clientNum, const char *reason );
void trap_SendServerCommand( int clientNum, int reliable, const char *command );
void trap_SetConfigstring( int num, const char *string );
void trap_GetConfigstring( int num, char *buffer, int bufferSize );
const char *trap_GetConfigstringConst( int num );
void trap_GetUserinfo( int num, char *buffer, int bufferSize );
void trap_SetUserinfo( int num, const char *buffer );
void trap_GetServerinfo( char *buffer, int bufferSize );
void trap_SetBrushModel( gentity_t *ent );
void trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
void trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
void trap_SightTrace( int *hitOut, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int passOwnerNum, int contentmask );
void trap_SightTraceCapsule( int *hitOut, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int passOwnerNum, int contentmask );
int trap_SightTraceToEntity( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask );
void trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
void trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
int trap_CM_BoxSightTrace( const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
int trap_CM_CapsuleSightTrace( const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
void trap_LocationalTrace( trace_t *results, const vec3_t start, const vec3_t end, int passEntityNum, int contentmask, const byte *dobjTracePartState );
int trap_PointContents( const vec3_t point, int passEntityNum, int contentmask );
int trap_InPVS( const vec3_t p1, const vec3_t p2 );
int trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
int trap_InSnapshot( const vec3_t origin, int entityNum );
void trap_AdjustAreaPortalState( gentity_t *ent, qboolean open );
int trap_AreasConnected( int area1, int area2 );
void trap_LinkEntity( gentity_t *ent );
void trap_UnlinkEntity( gentity_t *ent );
int trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount, int contentmask );
int trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
int trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
void trap_GetUsercmd( int clientNum, usercmd_t *cmd );
qboolean trap_GetEntityToken( char *buffer, int bufferSize );
void trap_AddDebugString( const vec3_t origin, const vec3_t color, float scale, const char *text );
void trap_AddDebugLine( const vec3_t start, const vec3_t end, const vec3_t color, int depthTest, int duration );
void trap_SetArchive( qboolean enable );
int trap_RealTime( void *qtime );
void trap_SnapVector( float *v );
int trap_Com_SoundAliasString( const char *name );
int trap_Com_PickSoundAlias( const char *name );
int trap_Com_SoundAliasIndex( void *alias );
int trap_SurfaceTypeFromName( const char *name );
const char *trap_SurfaceTypeToName( int surfaceType );
void *trap_Z_MallocInternal( int size );
void trap_Z_FreeInternal( void *ptr );
gentity_t *trap_AddTestClient( void );
qboolean trap_GetArchivedPlayerState( int clientNum, int *frameTime, playerState_t *ps );
XAnimTree *trap_XAnimCreateTree( XAnim *anims );
XAnimTree *trap_XAnimCreateSmallTree( XAnim *anims );
void trap_XAnimFreeSmallTree( XAnimTree *tree );
int trap_XModelExists( const char *name );
XModel *trap_XModelGet( const char *name );
void trap_DObjCreate( DObjModel *models, unsigned short modelCount, XAnimTree *tree, int handle, unsigned short scrNotifyId );
void trap_SafeDObjFree( int handle, unsigned int releaseTree );
qboolean trap_DObjExists( gentity_t *ent );
XAnim *trap_XAnimGetAnims( const XAnimTree *tree );
scr_anim_t trap_XAnimGetRoot( const XAnimTree *tree );
void trap_XAnimClearTreeGoalWeights( XAnimTree *tree, scr_anim_t anim, float goalWeight );
void trap_XAnimClearGoalWeight( XAnimTree *tree, int animIndex, float goalWeight );
void trap_XAnimClearTreeGoalWeightsStrict( XAnimTree *tree, scr_anim_t anim, float goalWeight );
void trap_XAnimSetCompleteGoalWeightKnob( XAnimTree *tree, unsigned short animIndex, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimSetCompleteGoalWeightKnobAll( XAnimTree *tree, scr_anim_t anim, scr_anim_t childAnim, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimSetAnimRate( XAnimTree *tree, scr_anim_t anim, float rate );
void trap_XAnimSetTime( XAnimTree *tree, int animIndex, float time );
void trap_XAnimSetGoalWeightKnob( XAnimTree *tree, unsigned short animIndex, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimClearTree( XAnimTree *tree );
int trap_XAnimHasTime( scr_anim_t anim );
int trap_XAnimIsPrimitive( scr_anim_t anim );
int trap_XAnimGetLength( XAnimTree *tree, unsigned short animIndex );
float trap_XAnimGetLengthSeconds( scr_anim_t anim );
void trap_XAnimSetCompleteGoalWeight( XAnimTree *tree, scr_anim_t anim, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimSetGoalWeight( XAnimTree *tree, scr_anim_t anim, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags );
void trap_XAnimCalcAbsDelta( XAnimTree *tree, scr_anim_t anim, float *deltaRot, float *deltaMove );
void trap_XAnimCalcDelta( XAnimTree *tree, unsigned short animIndex, float *deltaPos, void *deltaQuat, int animIndexEnd );
void trap_XAnimGetRelDelta( scr_anim_t anim, float *deltaRot, float *deltaMove, float startTime, float endTime );
void trap_XAnimGetAbsDelta( scr_anim_t anim, float *deltaPos, void *deltaQuat, float time );
int trap_XAnimIsLooped( scr_anim_t anim );
int trap_XAnimNotetrackExists( scr_anim_t anim, unsigned short notetrack );
float trap_XAnimGetTime( XAnimTree *tree, scr_anim_t anim );
float trap_XAnimGetWeight( XAnimTree *tree, scr_anim_t anim );
void trap_DObjDumpInfo( gentity_t *ent );
qboolean trap_DObjCreateSkelForBone( gentity_t *ent, int boneIndex );
qboolean trap_DObjCreateSkelForBones( gentity_t *ent, const int *partBits );
int trap_DObjUpdateServerTime( gentity_t *ent, float serverTime, int stopOnNotetrack );
void trap_DObjInitServerTime( gentity_t *ent, float serverTime );
void trap_DObjGetHierarchyBits( gentity_t *ent, int boneIndex, int *partBits );
int trap_DObjCalcAnim( gentity_t *ent, void *partBits );
int trap_DObjCalcSkel( gentity_t *ent, void *partBits );
void trap_XAnimLoadAnimTree( int tree );
void trap_XAnimSaveAnimTree( XAnimTree *tree );
void trap_XAnimCloneAnimTree( const XAnimTree *from, XAnimTree *to );
int trap_DObjNumBones( gentity_t *ent );
int trap_DObjGetBoneIndex( gentity_t *ent, const char *boneName );
float *trap_DObjGetMatrixArray( gentity_t *ent );
void trap_DObjDisplayAnim( gentity_t *ent );
int trap_XAnimHasFinished( XAnimTree *tree, scr_anim_t anim );
int trap_XAnimGetNumChildren( scr_anim_t anim );
scr_anim_t trap_XAnimGetChildAt( scr_anim_t anim, int childIndex );
int trap_XModelNumBones( XModel *model );
unsigned short *trap_XModelGetBoneNames( XModel *model );
DObjAnimMat *trap_DObjGetRotTransArray( gentity_t *ent );
qboolean trap_DObjSetRotTransIndex( gentity_t *ent, const int *partBits, int boneIndex );
qboolean trap_DObjSetControlRotTransIndex( gentity_t *ent, const int *partBits, int boneIndex );
const char *trap_XAnimGetAnimName( scr_anim_t anim );
XAnimTree *trap_DObjGetTree( gentity_t *ent );
int trap_XAnimGetAnimTreeSize( XAnim *anims );
void trap_XModelDebugBoxes( gentity_t *ent );
void *trap_GetWeaponInfoMemory( int size, int *pPrevOwner );
void trap_FreeWeaponInfoMemory( int iSource );
void trap_FreeClientScriptPers( void );
void trap_ResetEntityParsePoint( void );

//
// g_utils_mp.c
//
// a 4x3 transform: three axis rows then the origin
typedef struct matrix43_s {
	vec3_t axis[3];
	vec3_t origin;
} matrix43_t;

// gentity_t.linkInfo, set up by linkto()
struct entityLinkInfo_s {
	gentity_t       *parent;
	gentity_t       *nextChild;             // next sibling in parent->firstChild
	unsigned short tagName;                 // const string, 0 for an untagged link
	int parentTagIndex;                     // bone index in the parent's DObj, -1 for none
	matrix43_t relAxis;
	matrix43_t parentRelAxis;
};
G_ASSERT_SIZE( entityLinkInfo_t, 112 );

int         G_FindConfigstringIndex( const char *name, int start, int max, qboolean create,
									 const char *fieldname );
int         G_LocalizedStringIndex( const char *name );
int         G_ShaderIndex( const char *name );
int         G_ModelIndex( const char *name );
const char  *G_ModelName( int modelIndex );
int         G_TagIndex( const char *name );
int         G_EffectIndex( const char *name );
int         G_ShellShockIndex( const char *name );
byte        G_SoundAliasIndex( const char *name );
unsigned short G_GetGameId( gentity_t *ent );
void        G_DObjUpdate( gentity_t *ent );
void        G_SetModel( gentity_t *ent, const char *modelName );
qboolean    G_EntAttach( gentity_t *ent, const char *modelName, const char *tagName );
qboolean    G_EntDetach( gentity_t *ent, const char *modelName, const char *tagName );
void        G_EntDetachAll( gentity_t *ent );
qboolean    G_EntLinkTo( gentity_t *child, gentity_t *parent, const char *tagName );
qboolean    G_EntLinkToWithOffset( gentity_t *child, gentity_t *parent, const char *tagName,
								   const vec3_t originOffset, const vec3_t anglesOffset );
void        G_EntUnlink( gentity_t *ent );
qboolean    G_EntIsLinkedTo( gentity_t *child, gentity_t *parent );
void        G_UpdateTagInfo( gentity_t *ent, qboolean updateBoneIndex );
void        G_UpdateTagInfoOfChildren( gentity_t *parent, qboolean updateBoneIndex );
void        G_CalcTagParentAxis( gentity_t *child, matrix43_t *outAxis );
void        G_CalcTagParentRelAxis( gentity_t *child, matrix43_t *outAxis );
void        G_CalcTagAxis( gentity_t *ent, int useLinkedAngles );
void        G_SetFixedLink( gentity_t *ent, int mode );
void        G_GeneralLink( gentity_t *ent );
void        Think_GeneralLink( gentity_t *ent );
void        G_SafeDObjFree( gentity_t *ent );
int         G_DObjUpdateServerTime( gentity_t *ent, int stopOnNotetrack );
qboolean    G_DObjSetLocalTag( gentity_t *ent, int *partBits, const char *tagName,
							   const vec3_t origin, const vec3_t angles );
qboolean    G_DObjSetControlTagAngles( gentity_t *ent, int *partBits, const char *tagName,
									   const vec3_t angles );
void        G_DObjCalcPose( gentity_t *ent );
void        G_DObjCalcBone( gentity_t *ent, int boneIndex );
float       *G_DObjGetLocalTagMatrix( gentity_t *ent, const char *tagName );
qboolean    G_DObjGetWorldTagMatrix( gentity_t *ent, const char *tagName, void *outMatrix );
gentity_t   *G_Find( gentity_t *from, int fieldofs, unsigned short match );
gentity_t   *G_FindStr( gentity_t *from, int fieldofs, const char *match );
gentity_t   *G_PickTarget( unsigned short targetname );
char        *vtos( const vec3_t v );
char        *vtosf( const vec3_t v );
void        G_SetMovedir( vec3_t angles, vec3_t movedir );
void        G_InitGentity( gentity_t *e );
gentity_t   *G_Spawn( void );
gentity_t   *G_SpawnPlayerClone( void );
void        G_FreeEntityRefs( gentity_t *ent );
void        G_FreeEntity( gentity_t *ent );
gentity_t   *G_TempEntity( const vec3_t origin, int event );
void        G_KillBox( gentity_t *ent );
void        G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void        G_AddEvent( gentity_t *ent, int event, int eventParm );
void        G_PlaySoundAliasAtPoint( const vec3_t origin, byte alias );
void        G_PlaySoundAlias( gentity_t *ent, byte alias );
void        G_AnimScriptSound( int client, const char *name );
void        G_SetOrigin( gentity_t *ent, const vec3_t origin );
void        G_SetAngle( gentity_t *ent, const vec3_t angles );
qboolean    infront( gentity_t *self, gentity_t *other );
int         DebugLine( const vec3_t start, const vec3_t end, int color );
void        G_SetConstString( unsigned short *dest, const char *string );

//
// g_spawn_mp.c
//

// script variable types, as Scr_GetType / Scr_GetPointerType return them
typedef enum scriptVarType_e {
	VAR_UNDEFINED           = 0,
	VAR_STRING              = 1,
	VAR_LOCALIZED_STRING    = 2,
	VAR_VECTOR              = 3,
	VAR_FLOAT               = 4,
	VAR_INTEGER             = 5,
	VAR_CODEPOS             = 6,
	VAR_OBJECT              = 7,
	VAR_KEY_VALUE           = 8,
	VAR_FUNCTION            = 9,
	VAR_STACK               = 10,
	VAR_ANIMATION           = 11,
	VAR_THREAD              = 12,
	VAR_ENTITY              = 13,
	VAR_STRUCT              = 14,
	VAR_ARRAY               = 15,
	VAR_DEAD_THREAD         = 16,
	VAR_DEAD_ENTITY         = 17,
	VAR_DEAD_OBJECT         = 18,

	VAR_COUNT               = 19
} scriptVarType_t;

// field types for fields[], clientFields[] and hudelemFields[].
// F_VECTOR_Y keeps only the y of a vector; reading it gives { 0, y, 0 }
typedef enum {
	F_INT,
	F_FLOAT,
	F_CSTRING,              // char[] stored in place
	F_STRING,               // const string, an unsigned short
	F_VECTOR,
	F_ENTITY,               // gentity_t * in memory
	F_VECTOR_Y,
	F_OBJECT,               // script object, an unsigned short
	F_MODEL,                // model index, a byte
	F_IGNORE
} fieldtype_t;

// one row of clientFields[]; unlike field_t it has a getter as well
// as a setter
typedef struct clientField_s clientField_t;
typedef void ( *clientFieldFunc_t )( gclient_t *client, gclient_t *self, const clientField_t *field );

struct clientField_s {
	const char *name;
	int ofs;
	fieldtype_t type;
	clientFieldFunc_t setter;       // NULL falls through to Scr_SetGenericField
	clientFieldFunc_t getter;       // NULL falls through to Scr_GetGenericField
};

// a script field number with both mask bits set is a client field;
// the remaining bits index clientFields[]
#define CLIENT_FIELD_MASK       0x0000C000
#define CLIENT_FIELD_INDEX_MASK 0xFFFF3FFF

// game side hud element.  The first 112 bytes are the networked hudelem_t
// (q_shared.h only carries it as opaque bytes); the rest never reaches a client
typedef struct g_hudelem_s {
	int type;                       // hudElemType_t, 0 = free slot
	int x;
	int y;
	float fontScale;
	int font;
	int alignX;
	int alignY;
	int color;                      // packed rgba
	int fromColor;                  // fadeOverTime source color
	int fadeStartTime;
	int fadeTime;
	int label;                      // localized string index
	int width;
	int height;
	int materialIndex;              // shader index
	int scaleFromWidth;
	int scaleFromHeight;
	int scaleStartTime;
	int scaleTime;
	int moveFromX;
	int moveFromY;
	int moveStartTime;
	int moveTime;
	int timerValue;                 // level.time the timer runs out
	int rotationPeriodMs;           // setClock's total clock time
	float value;
	int text;                       // localized string index
	float sortKey;                  // script field "sort"

	// server side only, never copied into ps.hud
	int clientNum;                  // ENTITYNUM_NONE = shown to everyone
	int team;                       // 0 = shown to every team
	qboolean archived;              // selects ps.hud.archival over .current
} g_hudelem_t;
G_ASSERT_SIZE( g_hudelem_t, 124 );

qboolean G_SpawnString( const char *key, const char *defaultString, char **out );
qboolean G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean G_SpawnVector( const char *key, const char *defaultString, float *out );
void GScr_FieldError( gentity_t *ent, const char *error );
void G_ParseEntityFields( gentity_t *ent );
void G_DuplicateEntityFields( gentity_t *dest, gentity_t *source );
void G_DuplicateScriptFields( gentity_t *dest, gentity_t *source );
void G_CallSpawn( void );
qboolean G_CallSpawnEntity( gentity_t *ent );
void GScr_AddFieldsForEntity( void );
void GScr_AddFieldsForRadiant( void );
void Scr_SetEntityField( int entnum, unsigned int fieldnum );
void Scr_SetGenericField( void *base, int type, int ofs );
void Scr_SetObjectField( int classnum, int objectNum, int fieldnum );
void Scr_GetEntityField( int entnum, unsigned int fieldnum );
void Scr_GetGenericField( void *base, int type, int ofs );
void Scr_GetObjectField( int classnum, int objectNum, int fieldnum );
void Scr_FreeEntityConstStrings( gentity_t *ent );
void Scr_FreeEntity( gentity_t *ent );
void Scr_AddEntity( gentity_t *ent );
gentity_t *Scr_GetEntity( unsigned int num );
void Scr_FreeHudElem( g_hudelem_t *elem );
void Scr_AddHudElem( g_hudelem_t *elem );
g_hudelem_t *Scr_GetHudElem( unsigned int num );
unsigned short Scr_ExecEntThread( gentity_t *ent, int handle, unsigned int paramcount );
void Scr_AddExecEntThread( gentity_t *ent, int handle, unsigned int paramcount );
void Scr_Notify( gentity_t *ent, unsigned short event, unsigned int paramcount );
void Scr_GetEnt( void );
void Scr_GetEntArray( void );
void GScr_SetDynamicEntityField( gentity_t *ent, int fieldnum );
void G_SpawnGEntityFromSpawnVars( void );
qboolean G_ParseSpawnVars( void );
void SP_worldspawn( void );
void G_SpawnEntitiesFromString( void );

// q_shared.c
int Q_stricmp( const char *s1, const char *s2 );

// g_main_mp.c
extern vmCvar_t g_motd;

//
// g_client_mp.c
//
void        SetClientViewAngle( gentity_t *ent, const vec3_t angle );
void        ClientUserinfoChanged( int clientNum );
const char  *ClientConnect( int clientNum, unsigned int scriptId );
void        ClientBegin( int clientNum );
void        ClientSpawn( gentity_t *ent, const vec3_t origin, const vec3_t angles );
void        ClientDisconnect( int clientNum );
void        G_SetPlayerSize( void );
void        G_AddLean( gentity_t *ent, vec3_t pos );
int         G_GetNonPVSFriendlyInfo( gentity_t *ent, const vec3_t origin, int lastEntNum );

//
// g_client_fields_mp.c
//
void GScr_AddFieldsForClient( unsigned short classnum );
void Scr_SetClientField( gclient_t *client, unsigned int fieldnum );
void Scr_GetClientField( gclient_t *client, unsigned int fieldnum );

//
// g_client_script_cmd_mp.c
//

// player script methods get the number of the entity they were called on
typedef void ( *PlayerCmd_t )( unsigned int entnum );

void        PlayerCmd_giveWeapon( unsigned int entnum );
void        PlayerCmd_takeWeapon( unsigned int entnum );
void        PlayerCmd_takeAllWeapons( unsigned int entnum );
void        PlayerCmd_getCurrentWeapon( unsigned int entnum );
void        PlayerCmd_hasWeapon( unsigned int entnum );
void        PlayerCmd_switchToWeapon( unsigned int entnum );
void        PlayerCmd_giveStartAmmo( unsigned int entnum );
void        PlayerCmd_giveMaxAmmo( unsigned int entnum );
void        PlayerCmd_getFractionStartAmmo( unsigned int entnum );
void        PlayerCmd_getFractionMaxAmmo( unsigned int entnum );
void        PlayerCmd_setOrigin( unsigned int entnum );
void        PlayerCmd_setAngles( unsigned int entnum );
void        PlayerCmd_useButtonPressed( unsigned int entnum );
void        PlayerCmd_attackButtonPressed( unsigned int entnum );
void        PlayerCmd_meleeButtonPressed( unsigned int entnum );
void        PlayerCmd_isOnGround( unsigned int entnum );
void        PlayerCmd_pingPlayer( unsigned int entnum );
void        PlayerCmd_SetViewmodel( unsigned int entnum );
void        PlayerCmd_GetViewmodel( unsigned int entnum );
void        PlayerCmd_allowComplaint( unsigned int entnum );
void        PlayerCmd_showScoreboard( unsigned int entnum );
void        PlayerCmd_setSpawnWeapon( unsigned int entnum );
void        PlayerCmd_dropItem( unsigned int entnum );
void        PlayerCmd_finishPlayerDamage( unsigned int entnum );
void        PlayerCmd_Suicide( unsigned int entnum );
void        PlayerCmd_OpenMenu( unsigned int entnum );
void        PlayerCmd_OpenMenuNoMouse( unsigned int entnum );
void        PlayerCmd_CloseMenu( unsigned int entnum );
qboolean    WeaponSlotsNotValid( gentity_t *ent );
void        PlayerCmd_GetWeaponSlotWeapon( unsigned int entnum );
void        PlayerCmd_SetWeaponSlotWeapon( unsigned int entnum );
void        PlayerCmd_GetWeaponSlotAmmo( unsigned int entnum );
void        PlayerCmd_SetWeaponSlotAmmo( unsigned int entnum );
void        PlayerCmd_GetWeaponSlotClipAmmo( unsigned int entnum );
void        PlayerCmd_SetWeaponSlotClipAmmo( unsigned int entnum );
void        iclientprintln( unsigned int entnum );
void        iclientprintlnbold( unsigned int entnum );
void        PlayerCmd_spawn( unsigned int entnum );
void        PlayerCmd_setEnterTime( unsigned int entnum );
void        BodyEnd( gentity_t *ent );
void        PlayerCmd_ClonePlayer( unsigned int entnum );
void        PlayerCmd_SetClientCvar( unsigned int entnum );
void        PlayerCmd_FreezeControls( unsigned int entnum );
void        PlayerCmd_SetReverb( unsigned int entnum );
void        ScrCmd_IsLookingAt( unsigned int entnum );
void        ScrCmd_PlayLocalSound( unsigned int entnum );
void        PlayerCmd_SayAll( unsigned int entnum );
void        PlayerCmd_SayTeam( unsigned int entnum );
PlayerCmd_t Player_GetMethod( const char **pName );

//
// g_active_mp.c
//
void        P_DamageFeedback( gentity_t *player );
void        G_SetClientSound( gentity_t *ent );
void        ClientImpacts( gentity_t *ent, pmove_t *pm );
void        G_TouchTriggers( gentity_t *ent );
void        SpectatorThink( gentity_t *ent, usercmd_t *ucmd );
qboolean    ClientInactivityTimer( gclient_t *client );
void        ClientIntermissionThink( gentity_t *ent );
void        ClientEvents( gentity_t *ent, int oldEventSequence );
void        G_SetClientContents( gentity_t *ent );
void        ClientThink_real( gentity_t *ent, usercmd_t *ucmd );
void        ClientThink( int clientNum );
void        G_RunClient( gentity_t *ent );
void        IntermissionClientEndFrame( gentity_t *ent );
void        SpectatorClientEndFrame( gentity_t *ent );
int         GetFollowPlayerState( int clientNum, playerState_t *ps );
qboolean    StuckInClient( gentity_t *ent );
void        G_PlayerController( gentity_t *self, unsigned int *partBits );
void        ClientEndFrame( gentity_t *ent );


//
// g_cmds_mp.c
//
void        DeathmatchScoreboardMessage( gentity_t *ent );
void        Cmd_Score_f( gentity_t *ent );
qboolean    CheatsOk( gentity_t *ent );
char        *ConcatArgs( int start );
int         ClientNumberFromString( gentity_t *to, char *s );
// takes the fog configstring
void        G_setfog( const char *fogString );
void        Cmd_Fogswitch_f( void );
void        Cmd_Give_f( gentity_t *ent );
void        Cmd_Take_f( gentity_t *ent );
void        Cmd_God_f( gentity_t *ent );
void        Cmd_Notarget_f( gentity_t *ent );
void        Cmd_Noclip_f( gentity_t *ent );
void        Cmd_UFO_f( gentity_t *ent );
void        Cmd_Kill_f( gentity_t *ent );
void        StopFollowing( gentity_t *ent );
qboolean    Cmd_FollowCycle_f( gentity_t *ent, int dir );
qboolean    G_IsPlaying( gentity_t *ent );
void        G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color,
					 const char *name, const char *message );
void        G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText );
void        Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 );
void        G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id,
					   qboolean voiceonly );
void        G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id,
					 qboolean voiceonly );
void        Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly );
void        Cmd_Where_f( gentity_t *ent );
void        Cmd_CallVote_f( gentity_t *ent );
void        Cmd_Vote_f( gentity_t *ent );
void        Cmd_SetViewpos_f( gentity_t *ent );
qboolean    Cmd_Activate_f( gentity_t *ent );
void        Cmd_EntityCount_f( gentity_t *ent );
void        Cmd_MenuResponse_f( gentity_t *ent );
void        ClientCommand( int clientNum );

// q_shared.c
int         Q_stricmpn( const char *s1, const char *s2, int n );
void        Q_strncpyz( char *dest, const char *src, int destsize );
void        Q_CleanStr( char *string );
void        AddLeanToPosition( vec3_t pos, float yaw, float leanFrac,
							   float leanAngle, float leanDist );

// g_main_mp.c
extern vmCvar_t g_cheats;                               // "sv_cheats"
extern vmCvar_t g_dedicated;                            // "dedicated"
extern vmCvar_t g_gametype;
extern vmCvar_t g_allowVote;
extern vmCvar_t g_complaintlimit;
extern vmCvar_t g_voiceChatsAllowed;

//
// g_combat_mp.c
//

// means of death
typedef enum meansOfDeath_e {
	MOD_UNKNOWN             = 0x0,
	MOD_PISTOL_BULLET       = 0x1,
	MOD_RIFLE_BULLET        = 0x2,
	MOD_GRENADE             = 0x3,
	MOD_GRENADE_SPLASH      = 0x4,
	MOD_PROJECTILE          = 0x5,
	MOD_PROJECTILE_SPLASH   = 0x6,
	MOD_MELEE               = 0x7,
	MOD_HEAD_SHOT           = 0x8,
	MOD_MORTAR              = 0x9,
	MOD_MORTAR_SPLASH       = 0xA,
	MOD_KICKED              = 0xB,
	MOD_GRABBER             = 0xC,
	MOD_DYNAMITE            = 0xD,
	MOD_DYNAMITE_SPLASH     = 0xE,
	MOD_AIRSTRIKE           = 0xF,
	MOD_WATER               = 0x10,
	MOD_SLIME               = 0x11,
	MOD_LAVA                = 0x12,
	MOD_CRUSH               = 0x13,
	MOD_TELEFRAG            = 0x14,
	MOD_FALLING             = 0x15,
	MOD_SUICIDE             = 0x16,
	MOD_TRIGGER_HURT        = 0x17,
	MOD_EXPLOSIVE           = 0x18,

	MOD_NUM_MODS            = 0x19,
} meansOfDeath_t;

// obituary eventParm flag: the low bits are a meansOfDeath_t, not a weapon
#define MOD_FLAG                0x80

extern const char *modNames[MOD_NUM_MODS];

// hit locations
typedef enum hitLocation_e {
	HITLOC_NONE             = 0x0,
	HITLOC_HELMET           = 0x1,
	HITLOC_HEAD             = 0x2,
	HITLOC_NECK             = 0x3,
	HITLOC_TORSO_UPPER      = 0x4,
	HITLOC_TORSO_LOWER      = 0x5,
	HITLOC_R_ARM_UPPER      = 0x6,
	HITLOC_L_ARM_UPPER      = 0x7,
	HITLOC_R_ARM_LOWER      = 0x8,
	HITLOC_L_ARM_LOWER      = 0x9,
	HITLOC_R_HAND           = 0xA,
	HITLOC_L_HAND           = 0xB,
	HITLOC_R_LEG_UPPER      = 0xC,
	HITLOC_L_LEG_UPPER      = 0xD,
	HITLOC_R_LEG_LOWER      = 0xE,
	HITLOC_L_LEG_LOWER      = 0xF,
	HITLOC_R_FOOT           = 0x10,
	HITLOC_L_FOOT           = 0x11,
	HITLOC_GUN              = 0x12,

	HITLOC_NUM              = 0x13,
} hitLocation_t;

extern float g_fHitLocDamageMult[HITLOC_NUM];
extern unsigned short hitLocationConstStrings[HITLOC_NUM];

// damage flags; the game module passes them on to the script as iDFlags
#define DAMAGE_RADIUS               0x00000001  // damage was indirect
#define DAMAGE_NO_ARMOR             0x00000002  // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK         0x00000004  // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION        0x00000008  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION   0x00000010  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_PASSTHRU             0x00000020  // bullet went through something first

void        G_ParseHitLocDmgTable( void );
void        AddScore( gentity_t *ent, int score );
void        LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker );
int         G_IndexForMeansOfDeath( const char *name );
void        player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage,
						int mod, int weapon, const float *dir, int hitLoc );
void        G_DamageClient( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
							const vec3_t dir, const vec3_t point, int damage, int dflags,
							int mod, int hitLoc );
void        G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
					  const vec3_t dir, const vec3_t point, int damage, int dflags,
					  int mod, int hitLoc );
float       CanDamage( gentity_t *targ, const vec3_t origin );
qboolean    G_RadiusDamage( const vec3_t origin, gentity_t *inflictor, gentity_t *attacker,
							float damage, float minDamage, float radius, gentity_t *ignore,
							int mod );
unsigned short G_GetHitLocationString( int hitLoc );
int         G_GetHitLocationIndexFromString( unsigned short name );

//
// g_items_mp.c
//
extern qboolean itemRegistered[];

void        Fill_Clip( playerState_t *ps, int weapon );
int         Add_Ammo( gentity_t *ent, int weapon, int count, qboolean fillClip );
int         Pickup_Ammo( gentity_t *ent, gentity_t *other, int touchMode );
int         Pickup_Weapon( gentity_t *ent, gentity_t *other, int *event, int touchMode );
int         Pickup_Health( gentity_t *ent, gentity_t *other );
void        RespawnItem( gentity_t *ent );
void        Touch_Item_Auto( gentity_t *ent, gentity_t *other, int touchMode );
void        Touch_Item( gentity_t *ent, gentity_t *other, int touchMode );
int         GetFreeCueSpot( void );
void        DroppedItemClearOwner( gentity_t *ent );
gentity_t   *LaunchItem( gitem_t *item, const vec3_t origin, const vec3_t velocity, int ownerNum );
gentity_t   *Drop_Item( gentity_t *ent, gitem_t *item, float angle, qboolean novelocity );
gentity_t   *Drop_Weapon( gentity_t *ent, int weapon, const char *tagName );
void        Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        FinishSpawningItem( gentity_t *ent );
void        ClearRegisteredItems( void );
void        SaveRegisteredItems( void );
void        RegisterItem( int itemIndex, qboolean updateConfigString );
qboolean    IsItemRegistered( int itemIndex );
void        G_SpawnItem( gentity_t *ent, gitem_t *item );
void        G_BounceItem( gentity_t *ent, trace_t *trace );
void        G_RunItem( gentity_t *ent );

//
// g_missile_mp.c
//
void        G_MissileLandAngles( gentity_t *ent, trace_t *trace, vec3_t angles,
								 qboolean forceAngles );
qboolean    G_BounceMissile( gentity_t *ent, trace_t *trace );
void        G_MissileImpact( gentity_t *ent, trace_t *trace, vec3_t dir );
void        Concussive_think( gentity_t *ent );
gentity_t   *Concussive_fx( const vec3_t origin );
void        G_ExplodeMissile( gentity_t *ent );
void        G_MissileDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
						  int damage, int mod, int weapon, const float *dir, int hitLoc );
void        G_RunMissile( gentity_t *ent );
gentity_t   *fire_grenade( gentity_t *self, const vec3_t start, const vec3_t dir, int weapon );
gentity_t   *fire_rocket( gentity_t *self, const vec3_t start, vec3_t dir );

//
// g_weapon_mp.c
//

// the fire parameters FireWeapon builds and passes down the fire path
struct weaponInfo_t;

typedef struct weaponFireInfo_s {
	vec3_t forward;
	vec3_t right;
	vec3_t up;
	vec3_t start;                   // muzzle point
	vec3_t gunForward;
	const struct weaponInfo_t   *weapInfo;
} weaponFireInfo_t;
G_ASSERT_SIZE( weaponFireInfo_t, 64 );

// hit location priority maps for trap_LocationalTrace
extern byte bulletPriorityMap[HITLOC_NUM];
extern byte riflePriorityMap[HITLOC_NUM];

void        Weapon_Melee( weaponFireInfo_t *fireInfo, gentity_t *self );
void        SnapVectorTowards( vec3_t v, const vec3_t to );
void        Bullet_Endpos( float spread, vec3_t end, const weaponFireInfo_t *fireInfo );
void        Bullet_Fire( weaponFireInfo_t *fireInfo, gentity_t *self, gentity_t *attacker,
						 float spread, int damage );
void        Bullet_Fire_Extended( gentity_t *passEnt, gentity_t *attacker, vec3_t start,
								  vec3_t end, int damage, int depth,
								  const weaponFireInfo_t *fireInfo, gentity_t *owner );
gentity_t   *weapon_grenadelauncher_fire( gentity_t *self, int weapon,
										  const weaponFireInfo_t *fireInfo );
void        Weapon_RocketLauncher_Fire( gentity_t *ent, const weaponFireInfo_t *fireInfo,
										float spread );
qboolean    LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void        CalcMuzzlePoint( gentity_t *ent, vec3_t muzzlePoint );
void        CalcMuzzlePoints( weaponFireInfo_t *fireInfo, gentity_t *ent );
void        FireWeapon( gentity_t *ent );
void        FireWeaponMelee( gentity_t *ent );

//
// g_mover_mp.c
//
gentity_t   *G_TestEntityPosition( gentity_t *ent, const vec3_t origin );
void        G_CreateRotationMatrix( const vec3_t angles, vec3_t matrix[3] );
void        G_TransposeMatrix( const vec3_t matrix[3], vec3_t transpose[3] );
void        G_RotatePoint( vec3_t point, const vec3_t matrix[3] );
qboolean    G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove );
qboolean    G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle );
void        G_MoverTeam( gentity_t *ent );
void        G_RunMover( gentity_t *ent );
void        SetMoverState( gentity_t *ent, moverState_t moverState, int time );
void        MatchTeam( gentity_t *teamLeader, int moverState, int time );
void        MatchTeamReverseAngleOnSlaves( gentity_t *teamLeader, int moverState, int time );
void        ReturnToPos1( gentity_t *ent );
void        ReturnToPos2( gentity_t *ent );
void        GotoPos3( gentity_t *ent );
void        ReturnToPos1Rotate( gentity_t *ent );
void        Reached_BinaryMover( gentity_t *ent );
qboolean    IsBinaryMoverBlocked( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        InitMover( gentity_t *ent );
void        InitMoverRotate( gentity_t *ent );
void        Blocked_Door( gentity_t *ent, gentity_t *other );
void        Blocked_DoorRotate( gentity_t *ent, gentity_t *other );
void        Touch_DoorTrigger( gentity_t *ent, gentity_t *other, int touchMode );
gentity_t   *Think_SpawnNewDoorTriggerInternal( gentity_t *ent );
void        Think_SpawnNewDoorTrigger( gentity_t *ent );
void        DoorRotateStartOpen( gentity_t *ent );
void        Think_SpawnNewAutoDoorTrigger( gentity_t *ent );
void        Think_MatchTeam( gentity_t *ent );
void        finishSpawningKeyedMover( gentity_t *ent );
void        Door_reverse_sounds( gentity_t *ent );
void        DoorSetSounds( gentity_t *ent, qboolean isRotating );
void        G_TryDoor( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        SP_func_door( gentity_t *ent );
void        Use_Static( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        Static_Pain( gentity_t *ent, gentity_t *attacker, int damage, const float *point,
						 int mod, const float *dir, int hitLoc );
void        SP_func_leaky( gentity_t *ent );
void        SP_func_static( gentity_t *ent );
void        Use_Func_Rotate( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        SP_func_rotating( gentity_t *ent );
void        SP_func_bobbing( gentity_t *ent );
void        SP_func_pendulum( gentity_t *ent );
void        SP_func_door_rotating( gentity_t *ent );
void        use_trigger_use( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        trigger_use( gentity_t *ent );
void        G_Activate( gentity_t *ent, gentity_t *activator );

//
// g_scr_mover_mp.c
//
void        Reached_ScriptMover( gentity_t *ent );
void        InitScriptMover( gentity_t *ent );
void        SP_script_brushmodel( gentity_t *ent );
void        SP_script_model( gentity_t *ent );
void        SP_script_origin( gentity_t *ent );
void        ScriptEntCmd_MoveTo( int entnum );
void        ScriptEntCmd_GravityMove( int entnum );
void        ScriptEntCmd_MoveX( int entnum );
void        ScriptEntCmd_MoveY( int entnum );
void        ScriptEntCmd_MoveZ( int entnum );
void        ScriptEntCmd_RotateTo( int entnum );
void        ScriptEntCmd_RotatePitch( int entnum );
void        ScriptEntCmd_RotateYaw( int entnum );
void        ScriptEntCmd_RotateRoll( int entnum );
void        ScriptEntCmd_RotateVelocity( int entnum );
void        ScriptEntCmd_Solid( int entnum );
void        ScriptEntCmd_NotSolid( int entnum );

// Scr_GetMethod chains into this; it returns an xmethod_t
void ( *ScriptEnt_GetMethod( const char **pName ) )( int entnum );

//
// g_trigger_mp.c
//
void        G_Trigger( gentity_t *self, gentity_t *other );
void        InitTrigger( gentity_t *self );
void        InitSentientTrigger( gentity_t *self );
void        multi_wait( gentity_t *ent );
void        multi_trigger( gentity_t *ent, gentity_t *activator );
void        Use_Multi( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        Touch_Multi( gentity_t *self, gentity_t *other, int touchMode );
void        SP_trigger_multiple( gentity_t *ent );
void        hurt_touch( gentity_t *self, gentity_t *other, int touchMode );
void        hurt_think( gentity_t *ent );
void        hurt_use( gentity_t *self, gentity_t *other, gentity_t *activator );
void        SP_trigger_hurt( gentity_t *self );
void        SP_trigger_once( gentity_t *ent );
qboolean    Respond_trigger_damage( gentity_t *ent, int mod );
void        Activate_trigger_damage( gentity_t *ent, gentity_t *other, int damage, int mod );
void        Use_trigger_damage( gentity_t *self, gentity_t *other, gentity_t *activator );
void        Pain_trigger_damage( gentity_t *self, gentity_t *attacker, int damage,
								 const float *point, int mod, const float *dir, int hitLoc );
void        Die_trigger_damage( gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
								int damage, int mod, int weapon, const float *dir, int hitLoc );
void        SP_trigger_damage( gentity_t *ent );
void        G_CheckHitTriggerDamage( gentity_t *attacker, const vec3_t start, const vec3_t end,
									 int damage, int mod );
void        G_GrenadeTouchTriggerDamage( gentity_t *attacker, const vec3_t start, const vec3_t end,
										 int damage, int mod );
void        explosive_indicator_think( gentity_t *ent );
void        SP_trigger_lookat( gentity_t *ent );

//
// g_misc_mp.c
//
void        SP_info_camp( gentity_t *self );
void        SP_info_null( gentity_t *self );
void        SP_info_notnull( gentity_t *self );
void        SP_light( gentity_t *self );
void        TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );
void        SP_misc_teleporter_dest( gentity_t *ent );
void        SP_misc_model( gentity_t *ent );
void        use_corona( gentity_t *ent, gentity_t *other, gentity_t *activator );
void        SP_corona( gentity_t *ent );
void        G_InitTurrets( void );
void        G_PlayerTurretPositionAndBlend( gentity_t *player, gentity_t *turret );
void        G_ClientStopUsingTurret( gentity_t *self );
void        turret_think_client( gentity_t *self );
void        turret_think( gentity_t *self );
void        turret_think_init( gentity_t *self );
void        turret_controller( gentity_t *self, unsigned int *partBits );
void        G_FreeTurret( gentity_t *self );
qboolean    G_IsTurretUsable( gentity_t *turret, gentity_t *player );
void        turret_use( gentity_t *self, gentity_t *other, gentity_t *activator );
void        G_SpawnTurret( const char *weaponName, gentity_t *ent );
void        SP_turret( gentity_t *ent );
void        misc_spawner_think( gentity_t *ent );
void        misc_spawner_use( gentity_t *self, gentity_t *other, gentity_t *activator );
void        SP_misc_spawner( gentity_t *ent );
void        miscGunnerEnemyScan( gentity_t *self );

//
// g_team_mp.c
//
void        target_location_linkup( gentity_t *ent );
void        SP_target_location( gentity_t *self );
qboolean    OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
gentity_t   *Team_GetLocation( gentity_t *ent );
qboolean    Team_GetLocationMsg( gentity_t *ent, char *loc, int loclen );
void        TeamplayInfoMessage( gentity_t *ent );
void        CheckTeamStatus( void );

// g_main_mp.c
extern vmCvar_t g_maxclients;                    // "sv_maxclients"

//
// g_hud_mp.c
//

// HudElem_UpdateClient flags: which of the two ps.hud arrays to rebuild
#define HE_UPDATE_ARCHIVAL      0x0001
#define HE_UPDATE_CURRENT       0x0002

void        HudElem_ClientDisconnect( gentity_t *ent );
void        HudElem_DestroyAll( void );
void        Scr_GetHudElemField( int elemnum, int fieldnum );
void        Scr_SetHudElemField( int elemnum, int fieldnum );
void        Scr_FreeHudElemConstStrings( g_hudelem_t *elem );
void        GScr_NewHudElem( void );
void        GScr_NewClientHudElem( void );
void        GScr_NewTeamHudElem( void );
void        GScr_AddFieldsForHudElems( void );
void        ( *HudElem_GetMethod( const char **pName ) )( int elemnum );
void        HudElem_UpdateClient( gclient_t *client, int entNum, int flags );

extern g_hudelem_t g_hudelems[];


//
// g_svcmds_mp.c
//
qboolean    G_FilterPacket( char *from );
void        G_ProcessIPBans( void );
void        Svcmd_EntityList_f( void );
gclient_t   *ClientForString( const char *s );
qboolean    ConsoleCommand( void );

// g_main_mp.c
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;                     // not registered

//
// g_scr_main_mp.c
//

// Scr_SetClassMap entry; the VM replaces classnum with the class handle
typedef struct scr_classStruct_s {
	unsigned short classnum;
	byte pad[2];
	const char      *name;
} scr_classStruct_t;
G_ASSERT_SIZE( scr_classStruct_t, 8 );

// g_scr_data.classMap[] slots
#define SCR_CLASS_ENTITY            0
#define SCR_CLASS_HUDELEM           1
#define SCR_CLASS_NUM               2

#define MAX_GAMETYPES               32

typedef struct scr_gameTypeEntry_s {
	char            *name;          // the script's file name, lowercased
	char            *displayName;   // first token of the .txt, or name
	int teamBased;                  // second token is "team"
} scr_gameTypeEntry_t;

typedef struct scr_data_s {
	unsigned int levelScriptMain;
	unsigned int gameTypeScript;
	unsigned int gameTypeMain;
	unsigned int gameTypeStartup;   // CodeCallback_StartGameType
	unsigned int playerConnect;     // CodeCallback_PlayerConnect
	unsigned int playerDisconnect;  // CodeCallback_PlayerDisconnect
	unsigned int playerDamage;      // CodeCallback_PlayerDamage
	unsigned int playerKilled;      // CodeCallback_PlayerKilled
	int gameTypeCount;
	scr_gameTypeEntry_t gameTypes[MAX_GAMETYPES];
	scr_classStruct_t classMap[SCR_CLASS_NUM];
	// keep: original data layout
	byte pad_0x1B4[12];
} scr_data_t;
G_ASSERT_SIZE( scr_data_t, 0x1C0 );
G_ASSERT_OFFSET( scr_data_t, gameTypeScript, 0x004 );

extern scr_data_t g_scr_data;

// interned script strings, allocated by GScr_LoadConsts.  Several names are
// the original's own typos: activate interns "active", croutch "crouch",
// pisol "pistol" and place_waypoit "plane_waypoint"
typedef struct scr_const_s {
	unsigned short activate;
	unsigned short air_strike;
	unsigned short allies;
	unsigned short animdone;
	unsigned short axis;
	unsigned short bodyque;
	unsigned short combat;
	unsigned short connected;
	unsigned short connecting;
	unsigned short count;
	unsigned short current;
	unsigned short croutch;
	unsigned short crowbar;
	unsigned short damage;
	unsigned short death;
	unsigned short disconnected;
	unsigned short dlight;
	unsigned short done;
	unsigned short empty;
	unsigned short enemy;
	unsigned short enemyhidden;
	unsigned short enemyvisible;
	unsigned short entity;
	unsigned short failed;
	unsigned short flamebarrel;
	unsigned short fraction;
	unsigned short func_door;
	unsigned short func_door_rotating;
	unsigned short func_rotating;
	unsigned short func_tramcar;
	unsigned short goal;
	unsigned short grenade;
	unsigned short info_notnull;
	unsigned short invisible;
	unsigned short key1;
	unsigned short key2;
	unsigned short killanimscript;
	unsigned short left;
	unsigned short misc_flak;
	unsigned short misc_mg42;
	unsigned short misc_tagemitter;
	unsigned short mortar;
	unsigned short movedone;
	unsigned short noclass;
	unsigned short noenemy;
	unsigned short noncombat;
	unsigned short normal;
	unsigned short pisol;
	unsigned short place_waypoit;
	unsigned short player;
	unsigned short position;
	unsigned short primary;
	unsigned short primaryb;
	unsigned short prone;
	unsigned short right;
	unsigned short rocket;
	unsigned short rotatedone;
	unsigned short script_brushmodel;
	unsigned short script_model;
	unsigned short script_origin;
	unsigned short scriptcamera;
	unsigned short spawned;
	unsigned short spectator;
	unsigned short stand;
	unsigned short surfacetype;
	unsigned short tag_engine1;
	unsigned short tag_engine2;
	unsigned short target_location;
	unsigned short target_script_trigger;
	unsigned short tempEntity;
	unsigned short muzzleEntity;
	unsigned short smokegrenade;
	unsigned short touch;
	unsigned short trigger;
	unsigned short trigger_use;
	unsigned short trigger_damage;
	unsigned short trigger_lookat;
	unsigned short truck_cam;
	unsigned short worldspawn;
	unsigned short xmodel;
	unsigned short wehrmacht;
	unsigned short begin;
	unsigned short dynamite;
	unsigned short explosive_indicator;
	unsigned short flamechunk;
	unsigned short follow;
	unsigned short free;
	unsigned short freed;
	unsigned short func_leaky;
	unsigned short info_player_checkpoint;
	unsigned short initialize;
	unsigned short intermission;
	unsigned short item_stamina_brandy;
	unsigned short menuresponse;
	unsigned short misc_gunner_gun;
	unsigned short misc_gunner_ring;
	unsigned short mp_info_player_deathmatch;
	unsigned short mp_info_player_intermission;
	unsigned short mp_team_alliedplayer_respawn;
	unsigned short mp_team_alliedplayer_start;
	unsigned short mp_team_axisplayer_respawn;
	unsigned short mp_team_axisplayer_start;
	unsigned short nail;
	unsigned short not;
	unsigned short playing;
	unsigned short prox_mine;
	unsigned short reset;
	unsigned short script_mover;
	unsigned short script_multiplayer;
	unsigned short spear;
	unsigned short tag_hand;
	unsigned short tag_rider;
	unsigned short tag_ring;
	unsigned short team_CTF_blueflag;
	unsigned short team_CTF_redflag;
	unsigned short team_WOLF_checkpoint;
	unsigned short team_WOLF_objective;
	unsigned short trigger_aidoor;
	unsigned short trigger_flagonly;
	unsigned short trigger_multiple;
	unsigned short trigger_objective_info;
	unsigned short waiting_for_players;
	unsigned short WP;
	unsigned short zombiespit;
	unsigned short none;
	unsigned short dead;
	unsigned short auto_change;
	unsigned short manual_change;
} scr_const_t;
G_ASSERT_SIZE( scr_const_t, 256 );

extern scr_const_t scr_const;

// script VM imports, filled in by Scr_FarHook from the engine's export table
extern int         ( *Scr_GetBool )( unsigned int num );
extern int         ( *Scr_GetInt )( unsigned int num );
extern scr_anim_t  ( *Scr_GetAnim )( unsigned int num, void *anims );
extern void        ( *Scr_GetAnimTree )();
extern float       ( *Scr_GetFloat )( unsigned int num );
extern const char  *( *Scr_GetString )( unsigned int num );
extern unsigned short ( *Scr_GetConstString )( unsigned int num );
extern const char  *( *Scr_GetDebugString )( unsigned int num );
extern const char  *( *Scr_GetIString )( unsigned int num );
extern void        ( *Scr_GetConstIString )();
extern void        ( *Scr_GetVector )( unsigned int num, vec3_t out );
extern void        ( *Scr_GetFunc )();
extern int         ( *Scr_GetType )( unsigned int num );
extern int         ( *Scr_GetPointerType )( unsigned int num );
extern unsigned int ( *Scr_GetEntityNum )( unsigned int num, int *classnum );
extern unsigned int ( *Scr_GetNumParam )( void );
extern void        ( *Scr_AddBool )( int value );
extern void        ( *Scr_AddInt )( int value );
extern void        ( *Scr_AddFloat )( float value );
extern void        ( *Scr_AddAnim )();
extern void        ( *Scr_AddUndefined )( void );
extern void        ( *Scr_AddEntityNum )( int entnum, int classnum );
extern void        ( *Scr_AddStruct )( void );
extern void        ( *Scr_AddString )( const char *string );
extern void        ( *Scr_AddIString )();
extern void        ( *Scr_AddConstString )( unsigned short id );
extern void        ( *Scr_AddVector )( const float *value );
extern void        ( *Scr_AddObject )( unsigned short id );
extern void        ( *Scr_AddArray )( void );
extern void        ( *Scr_AddArrayStringIndexed )( unsigned short id );
extern void        ( *Scr_MakeArray )( void );
extern void        ( *Scr_BeginLoadScripts )( void );
extern void        ( *Scr_BeginLoadAnimTrees )();
extern void        ( *Scr_EndLoadScripts )( void );
extern void        ( *Scr_EndLoadAnimTrees )( void );
extern void        ( *Scr_PrecacheAnimTrees )( void *( *allocFn )( int size ) );
extern void        ( *Scr_FreeScripts )( int bComplete );
extern void        ( *Scr_FreeGameVariable )( int bComplete );
extern void        ( *Scr_ShutdownSystem )( int bComplete );
extern int         ( *Scr_IsSystemActive )( int bComplete );
extern void        ( *Scr_AddExecThread )();
extern void        ( *Scr_AddExecEntThreadNum )( int entnum, int classnum, int handle, unsigned int paramcount );
extern unsigned short ( *Scr_ExecThread )( unsigned int handle, unsigned int paramcount );
extern unsigned short ( *Scr_ExecEntThreadNum )( int entnum, int classnum, int handle, unsigned int paramcount );
extern void        ( *Scr_IsThreadAlive )();
extern void        ( *Scr_Error )( const char *error );
extern void        ( *Scr_ErrorWithDialogMessage )();
extern void        ( *Scr_ParamError )( unsigned int num, const char *error );
extern void        ( *Scr_ObjectError )( const char *error );
extern void        ( *Scr_SetDynamicEntityField )( int entnum, int classnum, unsigned short fieldnum );
extern void        ( *Scr_FreeEntityNum )( int entnum, int classnum );
extern unsigned short ( *Scr_GetEntityId )( int entnum, int classnum );
extern void        ( *Scr_SetClassMap )( scr_classStruct_t *classMap, unsigned int count );
extern void        ( *Scr_RemoveClassMap )( void );
extern void        ( *Scr_Unused36 )();
extern void        ( *Scr_Unused37 )();
extern void        ( *Scr_AddClassField )( unsigned short classnum, const char *name, unsigned short fieldnum );
extern void        ( *Scr_AddFields )( const char *path, const char *extension );
extern unsigned short ( *Scr_FindField )( const char *name, int *type );
extern int         ( *Scr_GetOffset )( unsigned short classnum, const char *name );
extern void        ( *Scr_CopyEntityNum )( int sourceEntnum, int destEntnum, int classnum );
extern void        ( *Scr_Init )();
extern void        ( *Scr_Shutdown )();
extern void        ( *Scr_Abort )();
extern void        ( *Scr_SetLoading )( int loading );
extern void        ( *Scr_AllocGameVariable )( int classnum, int time );
extern void        ( *Scr_InitSystem )( void );
extern void        ( *Scr_GetChecksum )();
extern void        ( *Scr_HasSourceFiles )();
extern void        ( *Scr_SaveSource )();
extern void        ( *Scr_LoadSource )();
extern void        ( *Scr_SkipSource )();
extern void        ( *Scr_SavePre )();
extern void        ( *Scr_SavePost )();
extern void        ( *Scr_SaveShutdown )();
extern void        ( *Scr_Unused4B )();
extern void        ( *Scr_LoadPre )();
extern void        ( *Scr_LoadShutdown )();
extern void        ( *Scr_Unused4E )();
extern int         ( *Scr_LoadScript )( const char *scriptName );
extern void        *( *Scr_FindAnimTree )( const char *treeName );
extern void        ( *Scr_FindAnim )( const char *treeName, const char *animName, scr_anim_t *anim );
extern int         ( *Scr_GetFunctionHandle )( const char *scriptName, const char *label );
extern void        ( *Scr_FreeThread )( unsigned short thread );
extern void        ( *Scr_ConvertThreadToSave )();
extern void        ( *Scr_ConvertThreadFromLoad )();
extern void        ( *Scr_SetString )( unsigned short *dest, unsigned short id );
extern unsigned short ( *Scr_AllocString )( const char *string, unsigned int user );
extern void        ( *Scr_NotifyNum )( int entnum, int classnum, unsigned short event, unsigned int paramcount );
extern void        ( *Scr_NotifyId )();
extern const char  *( *SL_ConvertToStringImport )( unsigned short id );
extern unsigned short ( *SL_GetString )( const char *string, unsigned int user );
extern unsigned short ( *SL_GetLowercaseString )( const char *string, unsigned int user );
extern unsigned short ( *SL_FindLowercaseString )( const char *string );
extern void        ( *Scr_CreateCanonicalFilename )();
extern void        ( *Scr_SetTime )( int time );
extern void        ( *Scr_RunCurrentThreads )( void );
extern void        ( *Scr_ResetTimeout )( void );
extern int         ( *Scr_GetAnimsIndex )( int anims );
extern void        ( *Scr_GetAnims )();
extern void        *( *MT_Alloc )( int size, int type );
extern void        ( *MT_Free )( void *p, int size );

// keep: wrapper around SL_ConvertToStringImport, matches original codegen
const char *SL_ConvertToString( unsigned short id );

struct objective_t;

void        GScr_LoadConsts( void );
void        Scr_LoadLevel( void );
int         GScr_LoadScriptAndLabel( const char *scriptName, const char *label,
                                     qboolean mandatory );
void        GScr_LoadGameTypeScript( void );
void        GScr_LoadLevelScript( void );
void        GScr_PostLoadScripts( void );
void        *Hunk_AllocXAnimCreate( int size );
void        GScr_LoadScripts( void );
void        GScr_FreeScripts( void );
gentity_t   *GetEntity( unsigned int entnum );
gentity_t   *GetPlayerEntity( unsigned int entnum );
unsigned short GScr_AllocString( const char *string );
void        Scr_LocalizationError( const char *error );
void        Scr_ConstructMessageString( int firstParam, int msgType, char *string,
                                        int maxLen );
void        Scr_MakeGameMessage( int clientNum, const char *command );
qboolean    G_GetHintStringIndex( int *index, const char *string );
void        G_InitObjectives( void );
qboolean    ObjectiveStateIndexFromString( int *outState, unsigned short stateName );
void        ClearObjective_OnEntity( objective_t *objective );
void        ClearObjective( objective_t *objective );
void        SetObjectiveIcon( unsigned int paramIndex, objective_t *objective );
int         GScr_GetScriptMenuIndex( const char *name );
int         GScr_GetStatusIconIndex( const char *name );
int         GScr_GetHeadIconIndex( const char *name );
void        Scr_SetFog( const char *cmdName, float nearDist, float farDist, float density,
                        float red, float green, float blue, float transitionTime );
void        Scr_SetOrigin( gentity_t *ent, int fieldnum );
void        Scr_SetAngles( gentity_t *ent, int fieldnum );
void        Scr_SetHealth( gentity_t *ent, int fieldnum );
void        GScr_AddVector( const float *value );
void        GScr_AddEntity( gentity_t *ent );
void        Scr_ParseGameTypeList( void );
// returns the display name of a gametype, NULL if it is not a valid one
const char  *Scr_GetGameTypeNameForScript( const char *gameType );
qboolean    Scr_IsValidGameType( const char *gameType );
void        Scr_LoadGameType( void );
void        Scr_StartupGameType( void );
void        Scr_PlayerConnect( gentity_t *ent );
void        Scr_PlayerDisconnect( gentity_t *ent );
void        Scr_PlayerKilled( gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
                              int damage, int mod, int weapon, const float *dir, int hitLoc );
int         Scr_LoadRead( int len );
void        *Scr_FarHook( void *scrImports );

//
// g_debug_mp.c
//
void        G_DebugLine( const vec3_t start, const vec3_t end, const vec3_t color,
						 int depthTest, int duration );
void        G_DebugBox( const vec3_t mins, const vec3_t maxs, const vec3_t color,
						int depthTest, int duration );
void        G_DebugCircle( const vec3_t org, qboolean flat, float radius, const vec3_t color,
						   int depthTest, int duration );
void        G_DebugCircleEx( const vec3_t dir, const vec3_t org, float radius, const vec3_t color,
							 int depthTest, int duration );
void        G_DebugArc( const vec3_t org, float radius, float startAngle, float endAngle,
						const vec3_t color, int depthTest, int duration );
int         Q_rint( float in );

// q_math.c
vec_t       VectorNormalize2( const vec3_t v, vec3_t out );
void        PerpendicularVector( vec3_t dst, const vec3_t src );

#endif
