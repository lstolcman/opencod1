// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

// Every structure below is checked with COD1_ASSERT_SIZE; if one fires, the
// declaration is wrong, not the assertion.  The assertion macros live here
// because this is the one header every structure header reaches.

#ifndef __Q_SHARED_H__
#define __Q_SHARED_H__

#include <stddef.h>

// FCAST / rfloat precision markers
#include "../qcommon/vc7_compat.h"

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define COD1_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
// pre-C11 fallback: a negative-size array typedef.  The name must be unique
// per use, and __LINE__ only expands if it is passed through two levels of
// macro before being pasted.
#define COD1_SA_CAT_(a, b) a##b
#define COD1_SA_CAT(a, b) COD1_SA_CAT_(a, b)
#define COD1_STATIC_ASSERT(cond, msg) \
	typedef char COD1_SA_CAT(cod1_static_assert_, __LINE__)[(cond) ? 1 : -1]
#endif

#define COD1_ASSERT_SIZE(type, bytes) \
	COD1_STATIC_ASSERT(sizeof(type) == (bytes), #type " must be " #bytes " bytes")

typedef unsigned char byte;

typedef enum qboolean_e
{
	qfalse = 0x0,
	qtrue  = 0x1,
} qboolean;

// parameters to the main Error routine
typedef enum errorParm_e
{
	ERR_FATAL            = 0x0,     // exit the entire game with a popup window
	ERR_DROP             = 0x1,     // print to console and disconnect from game
	ERR_SERVERDISCONNECT = 0x2,     // don't kill server
	ERR_DISCONNECT       = 0x3,     // client disconnected from the server
	ERR_NEED_CD          = 0x4,     // pop up the need-cd dialog
	ERR_AUTOUPDATE       = 0x5,
} errorParm_t;

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef float vec3_t[3];
typedef float vec4_t[4];

#ifndef M_PI
#define M_PI        3.14159265358979323846f // matches value in gcc v2 math.h
#endif

#if defined( UIDLL ) || defined( GAMEDLL ) || defined( CGAMEDLL )
extern const vec3_t vec3_origin;
#else
extern vec3_t vec3_origin;
#endif

extern const vec4_t g_color_table[8];

#define DotProduct( x,y )           ( (x)[0]*(y)[0] + (x)[1]*(y)[1] + (x)[2]*(y)[2] )
#define VectorSubtract( a,b,c )     ( (c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2] )
#define VectorAdd( a,b,c )          ( (c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2] )
#define VectorCopy( a,b )           ( (b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2] )
#define VectorScale( v,s,o )        ( (o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s) )
#define VectorMA( v,s,b,o )         ( (o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s) )
#define VectorClear( a )            ( (a)[0]=(a)[1]=(a)[2]=0 )
#define VectorNegate( a,b )         ( (b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2] )
#define VectorSet( v, x, y, z )     ( (v)[0]=(x),(v)[1]=(y),(v)[2]=(z) )

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross );
vec_t VectorLength( const vec3_t v );
vec_t VectorNormalize( vec3_t v );
void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );
void SinCos( float radians, float *sinOut, float *cosOut );

qboolean Com_BitCheck( const int array[], int bitNum );
float GetLeanFraction( float f );
float UnGetLeanFraction( float f );
void AddLeanToPosition( float *pos, float yaw, float leanFrac, float leanDist, float scale );

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_t
{
	vec3_t normal;
	float dist;
	byte type;                      // for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte signbits;                  // signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte pad[2];
} cplane_t;
COD1_ASSERT_SIZE( cplane_t, 20 );

// a trace is returned when a box is swept through the world
typedef struct trace_t
{
	float fraction;                 // time completed, 1.0 = didn't hit anything
	vec3_t endpos;                  // final position
	vec3_t normal;                  // surface normal at impact, transformed to world space
	int surfaceFlags;               // surface hit
	int contents;                   // contents on other side of surface hit
	int material;
	unsigned short entityNum;       // entity the contacted surface is a part of
	unsigned short partName;
	unsigned short partGroup;
	byte allsolid;
	byte startsolid;                // if true, the initial point was in a solid area
} trace_t;
COD1_ASSERT_SIZE( trace_t, 48 );

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD

/*
=============================================================================

						HANDLES AND FILE ACCESS

=============================================================================
*/

typedef int clipHandle_t;
typedef int fileHandle_t;

// mode parm for FS_FOpenFile
typedef enum fsMode_t
{
	FS_READ        = 0x0,
	FS_WRITE       = 0x1,
	FS_APPEND      = 0x2,
	FS_APPEND_SYNC = 0x3,
} fsMode_t;

typedef enum fsOrigin_t
{
	FS_SEEK_CUR = 0x0,
	FS_SEEK_END = 0x1,
	FS_SEEK_SET = 0x2,
} fsOrigin_t;

/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

typedef int cvarHandle_t;

typedef enum cvarFlags_e
{
	CVAR_ARCHIVE             = 0x1,     // set to cause it to be saved to vars.rc
	                                    // used for system variables, not for player
	                                    // specific configurations
	CVAR_USERINFO            = 0x2,     // sent to server on connect or change
	CVAR_SERVERINFO          = 0x4,     // sent in response to front end requests
	CVAR_SYSTEMINFO          = 0x8,     // these cvars will be duplicated on all clients
	CVAR_INIT                = 0x10,    // don't allow change from console at all,
	                                    // but can be set from the command line
	CVAR_LATCH               = 0x20,    // will only change when C code next does
	                                    // a Cvar_Get(), so it can't be changed
	                                    // without proper initialization.  modified
	                                    // will be set, even though the value hasn't
	                                    // changed yet
	CVAR_ROM                 = 0x40,    // display only, cannot be set by user at all
	CVAR_USER_CREATED        = 0x80,    // created by a set command
	CVAR_TEMP                = 0x100,   // can be set even when cheats are disabled, but is not archived
	CVAR_CHEAT               = 0x200,   // can not be changed if cheats are disabled
	CVAR_NORESTART           = 0x400,   // do not clear when a cvar_restart is issued
	CVAR_WOLFINFO            = 0x800,   // like userinfo, but for multiplayer info
	CVAR_UNSAFE              = 0x1000,
	CVAR_SERVERINFO_NOUPDATE = 0x2000,
} cvarFlags_t;

enum
{
	MAX_CVAR_VALUE_STRING = 0x100,
	MAX_CVAR_NAME_LENGTH  = 0x40,
	MAX_CVARS             = 0x400,
};

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct vmCvar_s
{
	cvarHandle_t handle;
	int modificationCount;
	float value;
	int integer;
	char string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;
COD1_ASSERT_SIZE( vmCvar_t, 272 );

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s
{
	char            *name;
	char            *string;
	char            *resetString;       // cvar_restart will reset to this value
	char            *latchedString;     // for CVAR_LATCH vars
	cvarFlags_t flags;
	qboolean modified;                  // set each time the cvar is changed
	int modificationCount;              // incremented each time the cvar is changed
	float value;                        // atof( string )
	int integer;                        // atoi( string )
	struct cvar_s   *next;
	struct cvar_s   *hashNext;
} cvar_t;
COD1_ASSERT_SIZE( cvar_t, 44 );

/*
=============================================================================

						CONFIGSTRINGS

=============================================================================
*/

enum
{
	MAX_CONFIGSTRINGS = 0x800,
};

/*
=============================================================================

						PLAYER STATE

=============================================================================
*/

typedef enum pmtype_t
{
	PM_NORMAL        = 0x0,     // can accelerate and turn
	PM_NORMAL_LINKED = 0x1,
	PM_NOCLIP        = 0x2,     // noclip movement
	PM_UFO           = 0x3,
	PM_SPECTATOR     = 0x4,     // still run into walls
	PM_INTERMISSION  = 0x5,     // no movement or status bar
	PM_DEAD          = 0x6,     // no acceleration or turning, but free falling
	PM_DEAD_LINKED   = 0x7,
} pmtype_t;

typedef enum weaponstate_t
{
	WEAPON_READY                 = 0x0,
	WEAPON_RAISING               = 0x1,
	WEAPON_DROPPING              = 0x2,
	WEAPON_FIRING                = 0x3,
	WEAPON_RECHAMBERING          = 0x4,
	WEAPON_RELOADING             = 0x5,
	WEAPON_RELOADING_INTERUPT    = 0x6,
	WEAPON_RELOAD_START          = 0x7,
	WEAPON_RELOAD_START_INTERUPT = 0x8,
	WEAPON_RELOAD_END            = 0x9,
	WEAPON_MELEE_INIT            = 0xA,
	WEAPON_MELEE_FIRE            = 0xB,
	WEAPONSTATES_NUM             = 0xC,
} weaponstate_t;

// usercmd_t is sent to the server each client frame
typedef struct usercmd_t
{
	int serverTime;
	byte buttons;                   // console, chat, ads, attack, use
	byte wbuttons;                  // lean left, lean right, reload
	byte weapon;
	byte flags;
	int angles[3];
	signed char forwardmove;
	signed char rightmove;
	signed char upmove;
	byte reserved_0x17;             // unused
} usercmd_t;
COD1_ASSERT_SIZE( usercmd_t, 24 );

typedef struct objective_t
{
	int state;
	vec3_t origin;
	int entNum;
	int teamNum;
	int icon;
} objective_t;
COD1_ASSERT_SIZE( objective_t, 28 );

// a script-created HUD element as it is sent in playerState_t.hud: the
// leading 112 bytes of the server's g_hudelem_t.  The server writes
// color / fromColor as one packed int; the client reads them a byte at a time.
typedef struct hudelem_t
{
	int type;                       // hudElemType_t; 0 = free slot
	int x;
	int y;
	float fontScale;
	int font;
	int alignX;
	int alignY;
	byte color[4];
	byte fromColor[4];              // fadeOverTime source colour
	int fadeStartTime;
	int fadeTime;
	int label;                      // localized string index
	int width;
	int height;
	int materialIndex;
	int scaleFromWidth;
	int scaleFromHeight;
	int scaleStartTime;
	int scaleTime;
	int moveFromX;
	int moveFromY;
	int moveStartTime;
	int moveTime;
	int timerValue;
	int rotationPeriodMs;
	float value;
	int text;                       // localized string index
	float sortKey;
} hudelem_t;
COD1_ASSERT_SIZE( hudelem_t, 112 );

typedef struct hudElemState_t
{
	hudelem_t current[31];
	hudelem_t archival[31];
} hudElemState_t;
COD1_ASSERT_SIZE( hudElemState_t, 6944 );

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
typedef struct playerState_s
{
	int commandTime;                // cmd->serverTime of last executed command
	pmtype_t pm_type;
	int bobCycle;                   // for view bobbing and footstep generation
	int pm_flags;                   // ducked, jump_held, etc
	int pm_time;
	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
	int weaponDelay;                // for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
	int grenadeTimeLeft;            // for delayed grenade throwing.  this is set to a #define for grenade
	                                // lifetime when the attack button goes down, then when attack is released
	                                // this is the amount of time left before the grenade goes off (or if it
	                                // gets to 0 while in players hand, it explodes)
	int iFoliageSoundTime;
	int gravity;
	float leanf;                    // amount of 'lean' when player is looking around corner
	int speed;
	int delta_angles[3];            // add to command angles to get view direction
	                                // changed by spawns, rotating objects, and teleporters
	int groundEntityNum;            // ENTITYNUM_NONE = in air
	vec3_t vLadderVec;
	int jumpTime;
	float fJumpOriginZ;
	int legsTimer;                  // don't change low priority animations until this runs out
	int legsAnim;                   // mask off ANIM_TOGGLEBIT
	int torsoTimer;                 // don't change low priority animations until this runs out
	int torsoAnim;                  // mask off ANIM_TOGGLEBIT
	int movementDir;                // a number 0 to 7 that represents the reletive angle
	                                // of movement to the view angle (axial and diagonals)
	                                // when at rest, the value will remain unchanged
	                                // used to twist the legs during strafing
	int eFlags;                     // copied to entityState_t->eFlags
	int eventSequence;              // pmove generated events
	int events[4];
	int eventParms[4];
	int oldEventSequence;           // so we can see which events have been added since we last converted to entityState_t
	int clientNum;                  // ranges from 0 to MAX_CLIENTS-1

	// weapon info
	unsigned int weapon;            // copied to entityState_t->weapon
	weaponstate_t weaponstate;
	float fWeaponPosFrac;
	int viewmodelIndex;
	vec3_t viewangles;              // for fixed views
	int viewHeightTarget;
	float viewHeightCurrent;
	int viewHeightLerpTime;
	int viewHeightLerpTarget;
	int viewHeightLerpDown;
	float viewHeightLerpPosAdj;

	// damage feedback
	int damageEvent;                // when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;
	int stats[6];
	int ammo[64];                   // total amount of ammo
	int ammoclip[64];               // ammo in clip
	unsigned int weapons[2];        // 64 bits for weapons held
	byte weaponslots[8];
	unsigned int weaponrechamber[2];

	// allow for individual bounding boxes
	vec3_t mins;
	vec3_t maxs;
	int proneViewHeight;
	int crouchViewHeight;
	int standViewHeight;
	int deadViewHeight;

	// variable movement speed
	float walkSpeedScale;
	float runSpeedScale;
	float proneSpeedScale;
	float crouchSpeedScale;
	float strafeSpeedScale;
	float backSpeedScale;
	float leanSpeedScale;
	float proneDirection;
	float proneDirectionPitch;
	float proneTorsoPitch;

	// view locking for mg42
	int viewlocked;
	int viewlocked_entNum;
	float friction;
	int gunfx;
	int serverCursorHint;           // what type of cursor hint the server is dictating
	int serverCursorHintVal;        // a value (0-255) associated with the above
	int serverCursorHintString;
	trace_t serverCursorHintTrace;  // not communicated over net, but used to store the current server-side cursorhint trace
	int iCompassFriendInfo;
	float fTorsoHeight;
	float fTorsoPitch;
	float fWaistPitch;
	int entityEventSequence;
	int weapAnim;
	float aimSpreadScale;
	int shellshockIndex;
	int shellshockTime;
	int shellshockDuration;
	objective_t objective[16];
	hudElemState_t hud;
	int ping;                       // server to game info for scoreboard
	int deltaTime;
} playerState_t;
COD1_ASSERT_SIZE( playerState_t, 8400 );

/*
=============================================================================

						PARSER

	The parse session itself is public: q_parse.c owns it, but the
	renderer reads it directly.

=============================================================================
*/

typedef struct parseInfo_t
{
	char token[1024];
	int currentLine;
	qboolean ungetReady;
	qboolean spaceDelimited;
	qboolean csv;
	qboolean parseNegativeNumbers;
	int ungetLineSave;
	char            *ungetTokenSave;
	char filename[64];
} parseInfo_t;
COD1_ASSERT_SIZE( parseInfo_t, 1116 );

#endif  /* __Q_SHARED_H__ */
