// g_public.h -- game module information visible to server
// entityState_t and entityShared_t are also declared in server_mp/server.h; keep the two in step

#ifndef __G_PUBLIC_H__
#define __G_PUBLIC_H__

#include "../universal/q_shared.h"

#ifndef QDECL
#define QDECL   __cdecl
#endif

typedef struct trajectory_t {
	int trType;
	int trTime;
	int trDuration;             // if non 0, trTime + trDuration = stop time
	vec3_t trBase;
	vec3_t trDelta;             // velocity, etc
} trajectory_t;
COD1_ASSERT_SIZE( trajectory_t, 36 );

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)

typedef struct entityState_s {
	int number;             // entity index
	int eType;              // entityType_t
	int eFlags;
	trajectory_t pos;       // for calculating position
	trajectory_t apos;      // for calculating angles
	int time;
	int time2;
	vec3_t origin2;
	vec3_t angles2;
	int otherEntityNum;     // shotgun sources, etc
	int attackerEntityNum;
	int groundEntityNum;    // -1 = in air
	int constantLight;      // r + (g<<8) + (b<<16) + (intensity<<24)
	int loopSound;          // constantly loop this sound
	int surfType;
	int index;
	int clientNum;          // 0 to (MAX_CLIENTS - 1), for players and corpses
	int iHeadIcon;
	int iHeadIconTeam;
	int solid;              // for client side prediction, trap_linkentity sets this properly
	int eventParm;
	int eventSequence;      // pmove generated events
	int events[4];
	int eventParms[4];
	int weapon;             // determines weapon and flash model, etc
	int legsAnim;           // mask off ANIM_TOGGLEBIT
	int torsoAnim;          // mask off ANIM_TOGGLEBIT
	float leanf;
	int scale;
	int dmgFlags;
	int animMovetype;       // clients can't derive movetype of other clients for anim scripting system
	float fTorsoHeight;
	float fTorsoPitch;
	float fWaistPitch;
} entityState_t;
COD1_ASSERT_SIZE( entityState_t, 240 );

typedef struct entityShared_t {
	qboolean linked;                // qfalse if not in any good cluster

	int svFlags;                    // SVF_NOCLIENT, SVF_BROADCAST, etc
	int singleClient;               // only send to this client when SVF_SINGLECLIENT is set

	qboolean bmodel;                // if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t mins;
	vec3_t maxs;
	int contents;                   // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin;                  // derived from mins/maxs and origin + rotation
	vec3_t absmax;

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
	int eventTime;
} entityShared_t;
COD1_ASSERT_SIZE( entityShared_t, 100 );

#define GENTITYNUM_BITS         10  // don't need to send any more
#define MAX_GENTITIES           ( 1 << GENTITYNUM_BITS )

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE          ( MAX_GENTITIES - 1 )
#define ENTITYNUM_WORLD         ( MAX_GENTITIES - 2 )

// entityState_t->eFlags
#define EF_DEAD                 0x00000001  // don't draw a foe marker over players with EF_DEAD
#define EF_NONSOLID_BMODEL      0x00000002  // bmodel is visible, but not solid
#define EF_TELEPORT_BIT         0x00000008  // toggled every time the origin abruptly changes
#define EF_CAPSULE              0x00000010  // use capsule for collisions
#define EF_CROUCHING            0x00000020  // player is crouching
#define EF_PRONE                0x00000040  // player is prone
#define EF_NODRAW               0x00000100  // may have an event, but no model (unspawned items)
#define EF_AIMING_DOWN_SIGHT    0x00000200  // mirrors PMF_ADS
#define EF_FIRING               0x00000400
#define EF_CONNECTION           0x00001000  // draw a connection trouble sprite
#define EF_TURRET_ACTIVE_PRONE  0x00004000
#define EF_TURRET_ACTIVE_DUCK   0x00008000
#define EF_TURRET_ACTIVE        ( EF_TURRET_ACTIVE_PRONE | EF_TURRET_ACTIVE_DUCK )
#define EF_VOTED                0x00020000  // already cast a vote
#define EF_TALK                 0x00040000  // draw a talk balloon

// entity->r.svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define SVF_NOCLIENT            0x00000001  // don't send entity to clients, even if it has effects
#define SVF_BROADCAST           0x00000008  // send to all connected clients
#define SVF_USE_CURRENT_ORIGIN  0x00000080  // entity->r.currentOrigin instead of entity->s.origin
											// for link position (missiles and movers)
#define SVF_CAPSULE             0x00000200  // use capsule for collision detection
#define SVF_SINGLECLIENT        0x00000800  // only send to a single client (entityShared_t->singleClient)
#define SVF_NOTSINGLECLIENT     0x00002000  // send entity to everyone but one client
											// (entityShared_t->singleClient)

// contents flags are seperate bits
// a given brush can contribute multiple content bits

#define CONTENTS_SOLID          0x00000001  // an eye is never valid in a solid
#define CONTENTS_FOLIAGE        0x00000002
#define CONTENTS_GLASS          0x00000010
#define CONTENTS_WATER          0x00000020
#define CONTENTS_CANSHOOTCLIP   0x00000040
#define CONTENTS_MISSILECLIP    0x00000080
#define CONTENTS_VEHICLECLIP    0x00000200
#define CONTENTS_ITEMCLIP       0x00000400
#define CONTENTS_SKY            0x00000800
#define CONTENTS_AI_NOSIGHT     0x00001000
#define CONTENTS_CLIPSHOT       0x00002000
#define CONTENTS_PLAYERCLIP     0x00010000
#define CONTENTS_MONSTERCLIP    0x00020000
#define CONTENTS_ORIGIN         0x01000000  // removed before bsping an entity
#define CONTENTS_BODY           0x02000000  // should never be on a brush, only in game
#define CONTENTS_CORPSE         0x04000000
#define CONTENTS_DETAIL         0x08000000  // brushes not used for the bsp
#define CONTENTS_STRUCTURAL     0x10000000  // brushes used for the bsp
#define CONTENTS_TRIGGER        0x40000000
#define CONTENTS_NODROP         0x80000000  // don't leave bodies or items (death fog, lava)

#define MASK_SOLID              ( CONTENTS_SOLID | CONTENTS_GLASS )

#define MASK_PLAYERSOLID        0x02810191
#define MASK_CLIENTSOLID        0x02810011

#define SURF_NODAMAGE           0x00000001  // never give falling damage
#define SURF_SLICK              0x00000002  // effects game physics
#define SURF_SKY                0x00000004  // lighting from environment map
#define SURF_LADDER             0x00000008
#define SURF_NOIMPACT           0x00000010  // don't make missile explosions
#define SURF_NOMARKS            0x00000020  // don't leave missile marks
#define SURF_NODRAW             0x00000080  // don't generate a drawsurface at all
#define SURF_HINT               0x00000100  // make a primary bsp splitter
#define SURF_NOLIGHTMAP         0x00000400  // surface doesn't need a lightmap
#define SURF_POINTLIGHT         0x00000800  // generate lighting info at vertexes
#define SURF_NOSTEPS            0x00002000  // no footstep sounds
#define SURF_NONSOLID           0x00004000  // don't collide against curves with this set
#define SURF_LIGHTFILTER        0x00008000  // act as a light filter during q3map -light
#define SURF_ALPHASHADOW        0x00010000  // do per-pixel light shadow casting in q3map
#define SURF_NODLIGHT           0x00020000  // don't dlight even if solid (solid lava, skies)
#define SURF_CASTSHADOW         0x00040000
#define SURF_PORTAL             0x80000000

// the surface type is a five-bit field at bit 20 of the surface flags
#define SURF_TYPE_SHIFT         20
#define SURF_TYPE_MASK          0x1F
#define SURF_TYPE( flags )      ( ( ( flags ) >> SURF_TYPE_SHIFT ) & SURF_TYPE_MASK )

typedef enum surfaceType_e {
	SURF_TYPE_DEFAULT       = 0,
	SURF_TYPE_BARK          = 1,
	SURF_TYPE_BRICK         = 2,
	SURF_TYPE_CARPET        = 3,
	SURF_TYPE_CLOTH         = 4,
	SURF_TYPE_CONCRETE      = 5,
	SURF_TYPE_DIRT          = 6,
	SURF_TYPE_FLESH         = 7,
	SURF_TYPE_FOLIAGE       = 8,
	SURF_TYPE_GLASS         = 9,
	SURF_TYPE_GRASS         = 10,
	SURF_TYPE_GRAVEL        = 11,
	SURF_TYPE_ICE           = 12,
	SURF_TYPE_METAL         = 13,
	SURF_TYPE_MUD           = 14,
	SURF_TYPE_PAPER         = 15,
	SURF_TYPE_PLASTER       = 16,
	SURF_TYPE_ROCK          = 17,
	SURF_TYPE_SAND          = 18,
	SURF_TYPE_SNOW          = 19,
	SURF_TYPE_WATER         = 20,
	SURF_TYPE_WOOD          = 21,
	SURF_TYPE_ASPHALT       = 22,

	SURF_TYPE_NUM           = 23
} surfaceType_t;

// pmove->pm_flags
#define PMF_PRONE               0x00000001
#define PMF_DUCKED              0x00000002
#define PMF_PRONE_DIVE          0x00000004  // shortens the duck->prone transition
#define PMF_JUMP_HELD           0x00000008
#define PMF_LADDER              0x00000010  // player is on a ladder
#define PMF_ADS                 0x00000020  // aiming down the sight
#define PMF_BACKWARDS_RUN       0x00000040  // coast down to backwards run
#define PMF_WALKING             0x00000080
#define PMF_TIME_LAND           0x00000100  // pm_time is time before rejump
#define PMF_TIME_KNOCKBACK      0x00000200  // pm_time is an air-accelerate only time
#define PMF_PRONE_MOVEOVERRIDE  0x00000400
#define PMF_RESPAWNED           0x00000800  // clear after attack button comes up
#define PMF_MELEE_LATCH         0x00001000  // one melee swing per press
#define PMF_TIME_HARDLAND       0x00002000
#define PMF_IGNORE_INPUT        0x00004000  // also blocks weapon changes
#define PMF_PRONE_BLOCKED       0x00008000  // cleared every frame
#define PMF_FOLLOW              0x00010000  // spectate following another player
#define PMF_FOLLOWLOCKED        0x00020000  // spectator is locked to a forced client
#define PMF_INWORLD             0x00040000  // client is actually in the world

//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define BUTTON_ATTACK           0x00000001
#define BUTTON_ADS              0x00000010
#define BUTTON_MELEE            0x00000020
#define BUTTON_ACTIVATE         0x00000040

#define WBUTTON_LEANLEFT        0x10
#define WBUTTON_LEANRIGHT       0x20
#define WBUTTON_PRONE           0x40
#define WBUTTON_CROUCH          0x80

typedef struct gentity_s gentity_t;

//===============================================================

//
// system traps provided by the main engine
//
typedef enum
{
	//============== general Quake services ==================

	G_PRINT                    = 0,  // ( const char *string );
	// print message on the local console
	G_ERROR                     = 1,  // ( const char *string );
	// abort the game
	G_ERROR_NOPREFIX            = 2,
	G_MILLISECONDS              = 3,  // ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	// console variable interaction
	G_CVAR_REGISTER             = 4,  // ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	G_CVAR_UPDATE               = 5,  // ( vmCvar_t *vmCvar );
	G_CVAR_SET                  = 6,  // ( const char *var_name, const char *value );
	G_CVAR_VARIABLE_INTEGER_VALUE = 7,  // ( const char *var_name );
	G_CVAR_VARIABLE_VALUE       = 8,
	G_CVAR_VARIABLE_STRING_BUFFER = 9,  // ( const char *var_name, char *buffer, int bufsize );
	G_ARGC                      = 10,  // ( void );
	// ClientCommand and ServerCommand parameter access
	G_ARGV                      = 11,  // ( int n, char *buffer, int bufferLength );
	G_HUNK_ALLOC                = 12,
	G_HUNK_ALLOC_LOW            = 13,
	G_HUNK_ALLOC_ALIGN          = 14,
	G_HUNK_ALLOC_LOW_ALIGN      = 15,
	G_HUNK_ALLOC_TEMP           = 16,
	G_HUNK_FREE_TEMP            = 17,
	G_FS_FOPEN_FILE             = 18,  // ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	G_FS_READ                   = 19,  // ( void *buffer, int len, fileHandle_t f );
	G_FS_WRITE                  = 20,  // ( const void *buffer, int len, fileHandle_t f );
	G_FS_RENAME                 = 21,
	G_FS_FCLOSE_FILE            = 22,  // ( fileHandle_t f );
	G_SEND_CONSOLE_COMMAND      = 23,  // ( int exec_when, const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc

	//=========== server specific functionality =============

	G_LOCATE_GAME_DATA          = 24,  // ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							playerState_t *clients, int sizeofGameClient );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface
	G_DROP_CLIENT               = 25,  // ( int clientNum, const char *reason );
	// kick a client off the server with a message
	G_SEND_SERVER_COMMAND       = 26,  // ( int clientNum, int reliable, const char *command );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients
	G_SET_CONFIGSTRING          = 27,  // ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.
	G_GET_CONFIGSTRING          = 28,  // ( int num, char *buffer, int bufferSize );
	G_GET_CONFIGSTRING_CONST    = 29,
	G_GET_USERINFO              = 30,  // ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset
	G_SET_USERINFO              = 31,  // ( int num, const char *buffer );
	G_GET_SERVERINFO            = 32,  // ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers
	G_SET_BRUSH_MODEL           = 33,  // ( gentity_t *ent );
	// sets mins and maxs based on the brushmodel name
	G_TRACE                     = 34,  // ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities
	G_TRACE_CAPSULE             = 35,  // ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection using capsule against all linked entities
	G_SIGHT_TRACE               = 36,
	G_SIGHT_TRACE_CAPSULE       = 37,
	G_SIGHT_TRACE_TO_ENTITY     = 38,
	G_CM_BOX_TRACE              = 39,
	G_CM_BOX_TRACE_2            = 40,
	G_CM_BOX_SIGHT_TRACE        = 41,
	G_CM_BOX_SIGHT_TRACE_2      = 42,
	G_LOCATIONAL_TRACE          = 43,
	G_POINT_CONTENTS            = 44,  // ( const vec3_t point, int passEntityNum, int contentmask );
	// point contents against all linked entities
	G_IN_PVS                    = 45,  // ( const vec3_t p1, const vec3_t p2 );
	G_IN_PVS_IGNORE_PORTALS     = 46,  // ( const vec3_t p1, const vec3_t p2 );
	G_IN_SNAPSHOT               = 47,
	G_ADJUST_AREA_PORTAL_STATE  = 48,  // ( gentity_t *ent, qboolean open );
	G_AREAS_CONNECTED           = 49,  // ( int area1, int area2 );
	G_LINKENTITY                = 50,  // ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	G_UNLINKENTITY              = 51,  // ( gentity_t *ent );
	// call before removing an interactive entity
	G_ENTITIES_IN_BOX           = 52,  // ( const vec3_t mins, const vec3_t maxs, int *list, int maxcount, int contentmask );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact
	G_ENTITY_CONTACT            = 53,  // ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape
	G_GET_USERCMD               = 54,  // ( int clientNum, usercmd_t *cmd )
	G_GET_ENTITY_TOKEN          = 55,  // qboolean ( char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.
	// This should only be done at GAME_INIT time.
	G_FS_GETFILELIST            = 56,
	G_REAL_TIME                 = 57,
	G_SNAPVECTOR                = 58,
	G_ENTITY_CONTACT_CAPSULE    = 59,  // ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape
	G_FIND_SOUND_ALIAS          = 60,
	G_PICK_SOUND_ALIAS          = 61,
	G_SOUND_ALIAS_INDEX         = 62,
	G_SURFACE_TYPE_FROM_NAME    = 63,
	G_SURFACE_TYPE_TO_NAME      = 64,
	G_ADD_TEST_CLIENT           = 65,
	G_GET_ARCHIVED_CLIENT_INFO  = 66,
	G_ADD_DEBUG_STRING          = 67,
	G_ADD_DEBUG_LINE            = 68,
	G_ENABLE_ARCHIVED_SNAPSHOT  = 69,
	G_Z_MALLOC                  = 70,
	G_Z_FREE                    = 71,
	G_XANIM_CREATE_TREE         = 72,
	G_XANIM_CREATE_SMALL_TREE   = 73,
	G_XANIM_FREE_SMALL_TREE     = 74,
	G_XMODEL_EXISTS             = 75,
	G_XMODEL_GET                = 76,
	G_DOBJ_CREATE               = 77,
	G_DOBJ_EXISTS               = 78,
	G_DOBJ_FREE                 = 79,
	G_DEREF_DWORD               = 80,
	// 81 is unused
	G_XANIM_CLEAR_ANIM          = 82,
	G_XANIM_CLEAR_ANIM_NODE     = 83,
	G_XANIM_CLEAR_ANIM_CHILDREN = 84,
	G_XANIM_SET_ANIM_KNOB       = 85,
	G_XANIM_SET_ANIM_KNOB_ALL   = 86,
	G_XANIM_SET_ANIM_RATE       = 87,
	G_XANIM_SET_TIME            = 88,
	G_XANIM_SET_ANIM_LIMITED    = 89,
	G_XANIM_CLEAR_TREE          = 90,
	G_XANIM_IS_PRIMITIVE        = 91,
	G_XANIM_IS_LEAF             = 92,
	G_XANIM_LENGTH_MSEC         = 93,
	G_XANIM_LENGTH_SEC          = 94,
	G_XANIM_SET_ANIM_KNOB_INT   = 95,
	G_XANIM_SET_ANIM_INT_LIMITED = 96,
	G_XANIM_GET_ABS_DELTA       = 97,
	G_XANIM_GET_REL_DELTA       = 98,
	G_XANIM_GET_REL_DELTA_TIME  = 99,
	G_XANIM_GET_ABS_DELTA_TIME  = 100,
	G_XANIM_IS_LOOPED           = 101,
	G_XANIM_HAS_NOTETRACK       = 102,
	G_XANIM_GET_TIME            = 103,
	G_XANIM_GET_WEIGHT          = 104,
	G_DOBJ_DUMP_INFO            = 105,
	G_DOBJ_CREATE_SKEL_FOR_BONE = 106,
	G_DOBJ_SKEL_UP_TO_DATE      = 107,
	G_DOBJ_UPDATE_SERVER_TIME   = 108,
	G_DOBJ_INIT_SERVER_TIME     = 109,
	G_DOBJ_GET_HIERARCHY_BITS   = 110,
	G_DOBJ_CALC_ANIM            = 111,
	G_DOBJ_CALC_SKEL            = 112,
	G_XANIM_RESTORE_TREE        = 113,
	G_XANIM_SAVE_TREE           = 114,
	G_XANIM_COPY_TREE           = 115,
	G_DOBJ_NUM_PARTS            = 116,
	G_DOBJ_GET_PART_INDEX       = 117,
	G_DOBJ_GET_SKEL_BASE        = 118,
	G_DOBJ_DISPLAY_ANIM         = 119,
	G_XANIM_HAS_TIME            = 120,
	G_XANIM_CHILD_COUNT         = 121,
	G_XANIM_CHILD_BASE          = 122,
	G_XANIM_ROOT_PART_COUNT     = 123,
	G_XANIM_ROOT_PART_LIST      = 124,
	G_DOBJ_GET_SKEL_END         = 125,
	G_DOBJ_SET_ROT_TRANS_INDEX  = 126,
	G_DOBJ_SET_CONTROL_ROT_TRANS_INDEX = 127,
	G_XANIM_GET_ANIM_NAME       = 128,
	G_DOBJ_GET_TREE             = 129,
	G_XANIM_TREE_FIELD          = 130,
	G_DOBJ_DEBUG_DRAW_BOUNDS    = 131,
	G_GET_WEAPON_INFO_MEMORY    = 132,
	G_FREE_WEAPON_INFO_MEMORY   = 133,
	G_FREE_CLIENT_SCRIPT_PERS   = 134,
	G_RESET_ENTITY_PARSE_POINT  = 135,
	// 200 and up are the QVM math/memory helpers; the DLL never issues them
	G_MEMSET                    = 200,
	G_MEMCPY                    = 201,
	G_STRNCPY                   = 202,
	G_SIN                       = 203,
	G_COS                       = 204,
	G_ATAN2                     = 205,
	G_SQRT                      = 206,
	G_MATRIXMULTIPLY            = 207,
	G_ANGLEVECTORS              = 208,
	G_PERPENDICULARVECTOR       = 209,
	G_FLOOR                     = 210,
	G_CEIL                      = 211
} gameImport_t;

//
// functions exported by the game subsystem
// (vmMain also handles 13, 14 and 15, which have no names on the engine side)
//
typedef enum
{
	GAME_INIT                    = 0,
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.
	GAME_SHUTDOWN                = 1,
	GAME_CLIENT_CONNECT          = 2,
	// return NULL if the client is allowed to connect, otherwise return
	// a text string with the reason for denial
	GAME_CLIENT_BEGIN            = 3,
	GAME_CLIENT_USERINFO_CHANGED = 4,
	GAME_CLIENT_DISCONNECT       = 5,
	GAME_CLIENT_COMMAND          = 6,
	GAME_CLIENT_THINK            = 7,
	GAME_GET_CLIENT_INFO         = 8,
	GAME_UPDATE_CVARS            = 9,
	GAME_RUN_FRAME               = 10,
	GAME_CONSOLE_COMMAND_SV      = 11,
	// ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.
	GAME_SCRIPT_FAR_HOOK         = 12,
	GAME_GET_MATCH_STATE         = 16,
	GAME_GET_CLIENT_STATE        = 17,
	GAME_GET_ARCHIVE_TIME        = 18,
	GAME_SET_ARCHIVE_TIME        = 19,
	GAME_GET_CLIENT_SCORE        = 20
} gameExport_t;

#endif  // __G_PUBLIC_H__
