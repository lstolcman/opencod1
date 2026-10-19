/*
 * @fidelity: likely
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#ifdef __QCOMMON_H__
#error "server.h must be included before qcommon.h -- see the header comment"
#endif

#include "../qcommon/qcommon.h"

#define SV_ASSERT_SIZE( type, bytes ) COD1_ASSERT_SIZE( type, bytes )
#define SV_ASSERT_OFFSET( type, member, ofs ) COD1_STATIC_ASSERT( offsetof( type, member ) == ( ofs ), #type "." #member )

/* game_mp/g_local.h's value; sizes the attachment arrays of the gentity_t and
   clientSession_t copies below. */
#ifndef MAX_ATTACHED_MODELS
#define MAX_ATTACHED_MODELS     6
#endif

/* Pushed as a literal by SV_Init (0x00459130) and SVC_Info (0x0045A320): `push 1` feeding va("%i", ...). CoD 1.1 really does advertise protocol 1. */
#define PROTOCOL_VERSION        1

#define MAX_MASTER_SERVERS      5
#define MAX_ENT_CLUSTERS        16

/* SV_Frame's map-restart guards; the exact constants the retail compares against (0x0045B28A onwards). */
#define SV_TIME_WRAP_LIMIT              0x70000000
#define SV_SNAPSHOT_COUNTER_LIMIT       0x7FFFFFFE
#define SV_CACHED_SNAP_ENTITIES_LIMIT   0x7FFFBFFE
#define SV_CACHED_SNAP_CLIENTS_LIMIT    0x7FFFEFFE
#define SV_ARCHIVED_SNAP_FRAMES_LIMIT   0x7FFFFB4E
#define SV_ARCHIVED_SNAP_BUFFER_LIMIT   0x7DFFFFFE
#define SV_CACHED_SNAP_FRAMES_LIMIT     0x7FFFFDFE

/* Sizes SV_EnableArchivedSnapshot (0x00458620) hands to Z_MallocInternal. */
#define SV_CACHED_SNAPSHOT_FRAMES_BYTES     0x00440000
#define SV_CACHED_SNAPSHOT_ENTITIES_BYTES   0x02130000
#define SV_ARCHIVED_SNAPSHOT_FRAMES_BYTES   0x00002580
#define SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES   0x02000000
#define SV_CACHED_SNAPSHOT_CLIENTS_BYTES    0x00003800

#define CS_SCRIPT_KV_BASE       140
#define CS_SCRIPT_KV_COUNT      64

#define CS_SERVERINFO           0
#define CS_SYSTEMINFO           1

typedef enum clientConnectState_e {
	CS_FREE      = 0x0,
	CS_ZOMBIE    = 0x1,
	CS_CONNECTED = 0x2,
	CS_PRIMED    = 0x3,
	CS_ACTIVE    = 0x4,
} clientConnectState_t;

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

#define GAME_INIT               0
#define GAME_SHUTDOWN           1
#define GAME_CLIENT_CONNECT     2
#define GAME_CLIENT_BEGIN       3
#define GAME_CLIENT_USERINFO_CHANGED 4
#define GAME_CLIENT_DISCONNECT  5
#define GAME_CLIENT_COMMAND     6
#define GAME_CLIENT_THINK       7
#define GAME_UPDATE_CVARS       9
#define GAME_RUN_FRAME          10
#define GAME_CONSOLE_COMMAND_SV 11
#define GAME_SCRIPT_FAR_HOOK    12
#define GAME_GET_MATCH_STATE    16
#define GAME_GET_ARCHIVE_TIME   18
#define GAME_GET_CLIENT_SCORE   20

typedef struct trajectory_t {
	int trType;
	int trTime;
	int trDuration;
	vec3_t trBase;
	vec3_t trDelta;
} trajectory_t;
SV_ASSERT_SIZE( trajectory_t, 36 );

typedef struct entityState_s {
	int number;
	int eType;
	int eFlags;
	trajectory_t pos;
	trajectory_t apos;
	int time;
	int time2;
	vec3_t origin2;
	vec3_t angles2;
	int otherEntityNum;
	int attackerEntityNum;
	int groundEntityNum;
	int constantLight;
	int loopSound;
	int surfType;
	int index;
	int clientNum;
	int iHeadIcon;
	int iHeadIconTeam;
	int solid;
	int eventParm;
	int eventSequence;
	int events[4];
	unsigned int eventParms[4];
	int weapon;
	int legsAnim;
	int torsoAnim;
	int leanf;
	int scale;
	int dmgFlags;
	int animMovetype;
	float fTorsoHeight;
	float fTorsoPitch;
	float fWaistPitch;
} entityState_t;
SV_ASSERT_SIZE( entityState_t, 240 );

typedef struct entityShared_t {
	qboolean linked;                /* +0xF0 in gentity_t */
	int svFlags;
	int singleClient;
	qboolean bmodel;                /* +0x0C Q3/RTCW/UO name, as game_mp/g_public.h; SV_SetBrushModel sets it, SV_LinkEntity reads it */
	vec3_t mins;
	vec3_t maxs;
	int contents;
	vec3_t absmin;
	vec3_t absmax;
	vec3_t currentOrigin;
	vec3_t currentAngles;
	int ownerNum;
	int eventTime;
} entityShared_t;
SV_ASSERT_SIZE( entityShared_t, 100 );

typedef struct clientSession_t {
	sessionState_t sessionState;
	int forceSpectatorClient;
	int statusIcon;
	int archiveTime;
	int score;
	int deaths;
	unsigned short pers;            /* +0x18 script object id, set by ClientConnect (0x20019E89) */
	byte pad_0x1A[2];               /* alignment before connected (game_mp/g_local.h) */
	clientConnected_t connected;
	usercmd_t cmd;
	usercmd_t oldcmd;
	qboolean localClient;
	qboolean predictItemPickup;     /* +0x54 */
	qboolean pmoveFixed;            /* +0x58 */
	/* 32, not MAX_NETNAME: ClientUserinfoChanged (0x20019D10) copies 31 bytes
	   and terminates at sess+0x7B; handicap follows at +0x7C (gclient 0x214C
	   in the game's client field table). */
	char netname[32];
	int handicap;                   /* +0x7C */
	int maxHealth;
	/* +0x84..: the game DLL's names (game_mp/g_local.h clientSession_t, which
	   gives their sources); the engine itself touches none of them. */
	int enterTime;                  /* +0x84 */
	int connectTime;                /* +0x88 */
	int voteCount;                  /* +0x8C */
	int teamVoteCount;              /* +0x90 */
	int complaints;                 /* +0x94 */
	int complaintClient;            /* +0x98 */
	int complaintEndTime;           /* +0x9C */
	int viewmodel;                  /* +0xA0 */
	qboolean teamInfo;              /* +0xA4 */
	int clientNum;                  /* +0xA8 */
	int sessionTeam;                /* +0xAC team_t in the game DLL */
	int modelIndex;                 /* +0xB0 */
	int attachModelIndex[MAX_ATTACHED_MODELS];  /* +0xB4 */
	int attachTagIndex[MAX_ATTACHED_MODELS];    /* +0xCC */
	char name[32];                  /* +0xE4 */
} clientSession_t;
SV_ASSERT_SIZE( clientSession_t, 260 );
SV_ASSERT_OFFSET( clientSession_t, predictItemPickup, 0x54 );
SV_ASSERT_OFFSET( clientSession_t, netname, 0x5C );
SV_ASSERT_OFFSET( clientSession_t, enterTime, 0x84 );
SV_ASSERT_OFFSET( clientSession_t, teamInfo, 0xA4 );
SV_ASSERT_OFFSET( clientSession_t, name, 0xE4 );

typedef struct gclient_s {
	playerState_t ps;
	clientSession_t sess;
	int spectatorClient;
	qboolean noclip;
	qboolean ufo;
	qboolean frozen;                /* +0x21E0 PlayerCmd_FreezeControls 0x2001CD00 */
	int lastCmdTime;                /* +0x21E4 ClientThink 0x200181BB, ClientEndFrame 0x20018D21 */
	int buttons;                    /* +0x21E8 */
	int oldbuttons;                 /* +0x21EC */
	int latched_buttons;            /* +0x21F0 */
	int wbuttons;                   /* +0x21F4 */
	int oldwbuttons;                /* +0x21F8 */
	int latched_wbuttons;           /* +0x21FC */
	vec3_t oldOrigin;               /* +0x2200 */
	/* From +0x220C to the end the game DLL's names (game_mp/g_local.h gclient_s,
	   which gives their sources and accesses). */
	float fGunPitch;                /* +0x220C */
	float fGunYaw;                  /* +0x2210 */
	int damage_blood;               /* +0x2214 P_DamageFeedback 0x20017179 */
	vec3_t damage_from;             /* +0x2218 */
	qboolean damage_fromWorld;      /* +0x2224 */
	int accurateCount;              /* +0x2228 */
	int accuracy_shots;             /* +0x222C */
	int accuracy_hits;              /* +0x2230 */
	int inactivityTime;             /* +0x2234 */
	qboolean inactivityWarning;     /* +0x2238 */
	int switchTeamTime;             /* +0x223C */
	float currentAimSpreadScale;    /* +0x2240 */
	struct gentity_s *persistantPowerup; /* +0x2244 */
	int portalID;                   /* +0x2248 */
	int dropWeaponTime;             /* +0x224C */
	byte reserved_0x2250[12];       /* unnamed in the game DLL too (see g_local.h) */
	int PCSpecialPickedUpCount;     /* +0x225C */
	struct gentity_s *lookatEnt;    /* +0x2260 ScrCmd_IsLookingAt 0x2001CED6 */
	int iLastCompassPlayerInfoEnt;  /* +0x2264 */
	int pingTime;                   /* +0x2268 PlayerCmd_pingPlayer 0x2001B450 */
	/* bg_weapon.c's saved view and weapon angle state */
	int viewKickStartTime;          /* +0x226C */
	float viewKickYaw;              /* +0x2270 */
	float viewKickPitch;            /* +0x2274 */
	vec3_t previousViewAngles;      /* +0x2278 */
	vec3_t swayOffsets;             /* +0x2284 */
	vec3_t swayAngles;              /* +0x2290 */
	vec3_t moveOffset;              /* +0x229C */
	float idleScale;                /* +0x22A8 */
	float recoilPitch;              /* +0x22AC */
	float recoilYaw;                /* +0x22B0 */
	float recoilRoll;               /* +0x22B4 */
	float recoilPitchSpeed;         /* +0x22B8 */
	float recoilYawSpeed;           /* +0x22BC */
	int recoilState;                /* +0x22C0 */
} gclient_t;
SV_ASSERT_SIZE( gclient_t, 8900 );
SV_ASSERT_OFFSET( gclient_t, sess, 0x20D0 );
SV_ASSERT_OFFSET( gclient_t, fGunPitch, 0x220C );
SV_ASSERT_OFFSET( gclient_t, accurateCount, 0x2228 );
SV_ASSERT_OFFSET( gclient_t, inactivityWarning, 0x2238 );
SV_ASSERT_OFFSET( gclient_t, PCSpecialPickedUpCount, 0x225C );
SV_ASSERT_OFFSET( gclient_t, lookatEnt, 0x2260 );
SV_ASSERT_OFFSET( gclient_t, iLastCompassPlayerInfoEnt, 0x2264 );
SV_ASSERT_OFFSET( gclient_t, pingTime, 0x2268 );
SV_ASSERT_OFFSET( gclient_t, viewKickStartTime, 0x226C );
SV_ASSERT_OFFSET( gclient_t, recoilState, 0x22C0 );

/* The game DLL's gentity_s (game_mp/g_local.h, which gives the sources and
   accesses for every name below); the engine uses only s, r, client and the
   frame latch. */
typedef struct gentity_s {
	entityState_t s;
	entityShared_t r;
	int worldflags;                 /* +0x154 */
	gclient_t       *client;
	void            *turret;        /* +0x15C turret state record (G_FreeEntity 0x2003C1E0) */
	byte inuse;                     /* +0x160 */
	byte physicsObject;             /* +0x161 */
	byte soundCloseEnd;             /* +0x162 */
	byte soundOpening;              /* +0x163 */
	byte soundClosing;              /* +0x164 */
	byte soundOpenEnd;              /* +0x165 */
	byte soundLoop;                 /* +0x166 */
	byte soundOpenLoop;             /* +0x167 */
	byte soundCloseLoop;            /* +0x168 */
	byte soundLocked;               /* +0x169 */
	byte soundOpeningQuiet;         /* +0x16A */
	byte soundOpenQuietEnd;         /* +0x16B */
	byte soundClosingQuiet;         /* +0x16C */
	byte soundCloseQuietEnd;        /* +0x16D */
	byte noise_index;               /* +0x16E */
	byte watertype;                 /* +0x16F */
	byte waterlevel;                /* +0x170 */
	byte takedamage;                /* +0x171 */
	byte active;                    /* +0x172 */
	byte nopickup;                  /* +0x173 */
	byte moverState;                /* +0x174 moverState_t, stored as a byte */
	byte model;                     /* +0x175 */
	unsigned short classname;       /* +0x176 const string id */
	int spawnflags;                 /* +0x178 */
	int flags;                      /* +0x17C */
	int eventTime;                  /* +0x180 */
	qboolean freeAfterEvent;        /* +0x184 */
	qboolean unlinkAfterEvent;      /* +0x188 */
	float physicsBounce;            /* +0x18C */
	int clipmask;
	int lastFrameNum;               /* +0x194 G_RunFrameForEntity's once-per-frame latch */
	struct gentity_s *parent;       /* +0x198 */
	struct gentity_s *nextTrain;    /* +0x19C */
	struct gentity_s *prevTrain;    /* +0x1A0 */
	vec3_t pos1;                    /* +0x1A4 */
	vec3_t pos2;                    /* +0x1B0 */
	vec3_t pos3;                    /* +0x1BC */
	unsigned short message;         /* +0x1C8 const string id */
	byte pad_0x1CA[2];              /* alignment */
	int timestamp;                  /* +0x1CC */
	float angle;                    /* +0x1D0 */
	unsigned short target;          /* +0x1D4 */
	unsigned short targetname;      /* +0x1D6 */
	unsigned short team;            /* +0x1D8 */
	byte pad_0x1DA[2];              /* alignment */
	struct gentity_s *target_ent;   /* +0x1DC */
	float speed;                    /* +0x1E0 */
	float closespeed;               /* +0x1E4 */
	vec3_t movedir;                 /* +0x1E8 */
	int gDuration;                  /* +0x1F4 */
	int gDurationBack;              /* +0x1F8 */
	int nextthink;                  /* +0x1FC */
	void ( *think )( struct gentity_s *self );                              /* +0x200 */
	void ( *reached )( struct gentity_s *self );                            /* +0x204 */
	void ( *blocked )( struct gentity_s *self, struct gentity_s *other );   /* +0x208 */
	void ( *touch )( struct gentity_s *self, struct gentity_s *other, int touchMode );  /* +0x20C */
	void ( *use )( struct gentity_s *self, struct gentity_s *other, struct gentity_s *activator );  /* +0x210 */
	void ( *pain )( struct gentity_s *self, struct gentity_s *attacker, int damage, const float *point, int mod, const float *dir, int hitLoc );  /* +0x214 */
	void ( *die )( struct gentity_s *self, struct gentity_s *inflictor, struct gentity_s *attacker, int damage, int mod, int weapon, const float *dir, int hitLoc );  /* +0x218 */
	byte reserved_0x21C[4];         /* unnamed in the game DLL too */
	void ( *controller )( struct gentity_s *self, unsigned int *partBits ); /* +0x220 */
	int painDebounceTime;           /* +0x224 */
	int fly_sound_debounce_time;    /* +0x228 */
	int last_move_time;             /* +0x22C */
	int health;                     /* +0x230 */
	int maxHealth;                  /* +0x234 */
	int damage;                     /* +0x238 */
	int splashDamage;               /* +0x23C */
	int splashMinDamage;            /* +0x240 */
	int splashRadius;               /* +0x244 */
	int methodOfDeath;              /* +0x248 */
	int splashMethodOfDeath;        /* +0x24C */
	int count;                      /* +0x250 */
	struct gentity_s *chain;        /* +0x254 */
	struct gentity_s *enemy;        /* +0x258 */
	struct gentity_s *activator;    /* +0x25C */
	struct gentity_s *teamchain;    /* +0x260 */
	struct gentity_s *teammaster;   /* +0x264 */
	float wait;                     /* +0x268 */
	float random;                   /* +0x26C */
	int radius;                     /* +0x270 */
	float delay;                    /* +0x274 */
	int TargetFlag;                 /* +0x278 */
	float duration;                 /* +0x27C */
	vec3_t rotate;                  /* +0x280 */
	vec3_t TargetAngles;            /* +0x28C */
	struct gitem_s *item;           /* +0x298 */
	vec3_t dl_color;                /* +0x29C */
	int key;                        /* +0x2A8 */
	float harc;                     /* +0x2AC */
	float varc;                     /* +0x2B0 */
	int missionLevel;               /* +0x2B4 */
	qboolean is_dead;               /* +0x2B8 */
	int start_size;                 /* +0x2BC */
	int end_size;                   /* +0x2C0 */
	byte reserved_0x2C4[4];         /* unnamed in the game DLL too */
	unsigned short spawnitem;       /* +0x2C8 */
	byte pad_0x2CA[2];              /* alignment */
	int clipCount;                  /* +0x2CC */
	int grenadeExplodeTime;         /* +0x2D0 */
	int grenadeFired;               /* +0x2D4 */
	int mg42ClampTime;              /* +0x2D8 */
	unsigned short track;           /* +0x2DC */
	byte reserved_0x2DE[6];         /* unnamed in the game DLL too */
	struct entityLinkInfo_s *linkInfo;  /* +0x2E4 */
	struct gentity_s *firstChild;   /* +0x2E8 */
	byte attachModelIndex[MAX_ATTACHED_MODELS];         /* +0x2EC */
	unsigned short attachTagName[MAX_ATTACHED_MODELS];  /* +0x2F2 const string ids */
	byte pad_0x2FE[2];              /* alignment */
	int spawnCount;                 /* +0x300 */
	struct gentity_s *nextFree;     /* +0x304 */
	int voiceChatSquelch;           /* +0x308 */
	int voiceChatPreviousTime;      /* +0x30C */
	int lastBurnedFrameNumber;      /* +0x310 */
} gentity_t;
SV_ASSERT_SIZE( gentity_t, 788 );
SV_ASSERT_OFFSET( gentity_t, r, 0xF0 );
SV_ASSERT_OFFSET( gentity_t, worldflags, 0x154 );
SV_ASSERT_OFFSET( gentity_t, client, 0x158 );
SV_ASSERT_OFFSET( gentity_t, nopickup, 0x173 );
SV_ASSERT_OFFSET( gentity_t, clipmask, 0x190 );
SV_ASSERT_OFFSET( gentity_t, lastFrameNum, 0x194 );
SV_ASSERT_OFFSET( gentity_t, parent, 0x198 );
SV_ASSERT_OFFSET( gentity_t, fly_sound_debounce_time, 0x228 );
SV_ASSERT_OFFSET( gentity_t, TargetFlag, 0x278 );
SV_ASSERT_OFFSET( gentity_t, is_dead, 0x2B8 );
SV_ASSERT_OFFSET( gentity_t, track, 0x2DC );
SV_ASSERT_OFFSET( gentity_t, spawnCount, 0x300 );
SV_ASSERT_OFFSET( gentity_t, lastBurnedFrameNumber, 0x310 );

typedef struct svEntity_t {
	/* +0x00/+0x04: RTCW svEntity_s's world-sector link (opencod2's svEntity_s
	   keeps the pair as shorts); qcommon/cm_world.c's cmEntityLink_t view of
	   this record uses them as worldSector / next. */
	struct worldSector_t *worldSector;            /* +0x00 */
	struct svEntity_t *nextEntityInWorldSector;   /* +0x04 */
	entityState_t baseline;
	int baselineSvFlags;            /* +0xF8, from gentity->r.svFlags */
	int baselineSingleClient;       /* +0xFC, from gentity->r.singleClient */
	float baselineAbsmin[3];        /* +0x100, from gentity->r.absmin */
	float baselineAbsmax[3];        /* +0x10C, from gentity->r.absmax */
	/* RTCW svEntity_s's PVS cluster block (read by sv_game_mp.c, sv_world_mp.c
	   and sv_snapshot_mp.c at these offsets), then opencod2 svEntity_s's link
	   bounds (cm_world.c's cmEntityLink_t linkContents/linkMins/linkMaxs). */
	int numClusters;                /* +0x118 */
	int clusternums[MAX_ENT_CLUSTERS];  /* +0x11C */
	int lastCluster;                /* +0x15C */
	int areanum;                    /* +0x160 */
	int areanum2;                   /* +0x164 */
	int linkcontents;               /* +0x168 */
	float linkmin[2];               /* +0x16C */
	float linkmax[2];               /* +0x174 */
} svEntity_t;
SV_ASSERT_SIZE( svEntity_t, 380 );
SV_ASSERT_OFFSET( svEntity_t, baseline, 0x08 );
SV_ASSERT_OFFSET( svEntity_t, numClusters, 0x118 );
SV_ASSERT_OFFSET( svEntity_t, lastCluster, 0x15C );
SV_ASSERT_OFFSET( svEntity_t, linkcontents, 0x168 );

typedef struct server_t {
	serverState_t state;
	qboolean restarting;
	int serverId;
	int checksumFeed;
	int timeResidual;
	int nextFrameTime;              /* +0x14 RTCW / opencod2 server_t name */
	struct cmodel_t *models[256];   /* +0x18 RTCW / opencod2 server_t: MAX_MODELS inline-model pointers */
	char            *configstrings[MAX_CONFIGSTRINGS];
	svEntity_t svEntities[MAX_GENTITIES];

	const char      *entityParsePoint;

	gentity_t       *gentities;
	int gentitySize;
	int num_entities;
	playerState_t   *gameClients;
	int gameClientSize;

	int bpsWindow[MAX_BPS_WINDOW];
	int bpsWindowSteps;
	int bpsTotalBytes;
	int bpsMaxBytes;
	int ubpsWindow[MAX_BPS_WINDOW];
	int ubpsTotalBytes;
	int ubpsMaxBytes;
	float ucompAve;
	int ucompNum;
} server_t;
SV_ASSERT_SIZE( server_t, 398572 );
SV_ASSERT_OFFSET( server_t, configstrings, 0x418 );

typedef struct clientSnapshot_t {
	playerState_t ps;
	int num_entities;
	int num_clients;
	int first_entity;
	int first_client;
	unsigned int messageSent;
	unsigned int messageAcked;
	int messageSize;
} clientSnapshot_t;
SV_ASSERT_SIZE( clientSnapshot_t, 8428 );

typedef struct reliableCommands_t {
	char command[MAX_STRING_CHARS];
	int cmdTime;
	int cmdType;
} reliableCommands_t;
SV_ASSERT_SIZE( reliableCommands_t, 1032 );

typedef struct client_t {
	clientConnectState_t state;
	qboolean sendAsActive;
	const char      *dropReason;
	char userinfo[MAX_INFO_STRING];

	reliableCommands_t reliableCommands[MAX_RELIABLE_COMMANDS];
	int reliableSequence;
	int reliableAcknowledge;
	int reliableSent;
	int messageAcknowledge;

	int gamestateMessageNum;
	int challenge;

	usercmd_t lastUsercmd;
	int lastClientCommand;
	char lastClientCommandString[MAX_STRING_CHARS];
	gentity_t       *gentity;
	char name[MAX_NAME_LENGTH];

	char downloadName[MAX_QPATH];
	fileHandle_t download;
	int downloadSize;
	int downloadCount;
	int downloadClientBlock;
	int downloadCurrentBlock;
	int downloadXmitBlock;
	byte            *downloadBlocks[MAX_DOWNLOAD_WINDOW];
	int downloadBlockSize[MAX_DOWNLOAD_WINDOW];
	qboolean downloadEOF;
	int downloadSendTime;

	int deltaMessage;
	int nextReliableTime;
	int lastPacketTime;
	int lastConnectTime;
	int nextSnapshotTime;
	qboolean rateDelayed;
	int timeoutCount;

	clientSnapshot_t frames[PACKET_BACKUP];
	int ping;
	int rate;
	int snapshotMsec;
	int pureAuthentic;

	netchan_t netchan;

	unsigned short scriptId;
	int bIsTestClient;
	int serverId;
} client_t;
SV_ASSERT_SIZE( client_t, 370940 );

typedef struct challenge_t {
	netadr_t adr;
	int challenge;
	int time;
	int pingTime;
	int firstTime;
	int firstPing;
	qboolean connected;
} challenge_t;
SV_ASSERT_SIZE( challenge_t, 44 );

/* 1200 entries x 8 bytes == SV_ARCHIVED_SNAPSHOT_FRAMES_BYTES (0x2580). Stride 8: `[eax+edx*8]` / `[eax+edx*8+4]` at 0x0045E797. */
typedef struct archivedSnapshot_s {
	int start;                      /* +0x00 svs.nextArchivedSnapshotBuffer before the write -- an ever-increasing byte cursor, NOT pre-wrapped */
	int len;                        /* +0x04 bytes written for this frame */
} archivedSnapshot_t;
SV_ASSERT_SIZE( archivedSnapshot_t, 8 );

/* 512 entries x 28 bytes == SV_CACHED_SNAPSHOT_CLIENTS_BYTES (0x3800). Stride 28: `imul esi, 1Ch` at 0x0045DF32 and 0x0045E36D. */
typedef struct cachedSnapshot_s {
	int archivedFrame;              /* +0x00 svs.nextArchivedSnapshotFrames when cached */
	int time;                       /* +0x04 svs.time */
	int num_entities;               /* +0x08 incremented per entity archived */
	int first_entity;               /* +0x0C svs.nextCachedSnapshotEntities at capture */
	int num_clients;                /* +0x10 incremented per client archived */
	int first_client;               /* +0x14 svs.nextCachedSnapshotClients at capture */
	int usesDelta;                  /* +0x18 name inferred; written 0 by the writer, a non-zero value disqualifies the frame as a delta base. The setter is presumably the reader SV_GetCachedSnapshotInternal. */
} cachedSnapshot_t;
SV_ASSERT_SIZE( cachedSnapshot_t, 28 );

typedef struct archivedEntity_s {
	entityState_t s;                /* +0x000 memcpy 0xF0 from gentity->s */
	int svFlags;                    /* +0x0F0 gentity->r.svFlags (ent+0x0F4) */
	int singleClient;               /* +0x0F4 gentity->r.singleClient (ent+0x0F8) */
	float absmin[3];                /* +0x0F8 gentity->r.absmin (ent+0x11C) */
	float absmax[3];                /* +0x104 gentity->r.absmax (ent+0x128) */
} archivedEntity_t;
SV_ASSERT_SIZE( archivedEntity_t, 272 );

/* 4096 entries x 8496 bytes == SV_CACHED_SNAPSHOT_ENTITIES_BYTES (0x2130000). Stride 8496: `imul ebx, 2130h` at 0x0045E032 and 0x0045E3E7. */
typedef struct cachedClient_s {
	int havePlayerState;            /* +0x0000 VM_Call(GAME_GET_CLIENT_INFO) result */
	byte cs[92];                    /* +0x0004 the 92-byte snapshot client record; its first dword is the client number */
	playerState_t ps;               /* +0x0060 8400 bytes */
} cachedClient_t;
SV_ASSERT_SIZE( cachedClient_t, 8496 );

#define SV_NUM_ARCHIVED_SNAPSHOT_FRAMES     1200
#define SV_NUM_CACHED_SNAPSHOT_FRAMES       512
#define SV_NUM_CACHED_SNAPSHOT_ENTITIES     0x4000      /* 0x440000 / 272 */
#define SV_NUM_CACHED_SNAPSHOT_CLIENTS      4096        /* 0x2130000/ 8496 */

#define SV_SNAPSHOT_CLIENT_SIZE 92

typedef struct serverStatic_t {
	qboolean initialized;
	int time;
	int snapFlagServerBit;

	client_t        *clients;
	int numSnapshotEntities;
	int numSnapshotClients;
	int nextSnapshotEntities;
	int nextSnapshotClients;

	entityState_t   *snapshotEntities;      /* +0x20, Hunk_AllocAlign(240 * numSnapshotEntities, 32) */
	void            *snapshotClients;       /* +0x24, Hunk_AllocAlign( 92 * numSnapshotClients, 32) */
	qboolean archiveEnabled;                /* +0x28, SV_EnableArchivedSnapshot */
	int nextArchivedSnapshotFrames;         /* +0x2C */

	archivedSnapshot_t  *archivedSnapshotFrames;
	byte                *archivedSnapshotBuffer;
	int nextArchivedSnapshotBuffer;
	int nextCachedSnapshotEntities;
	int nextCachedSnapshotClients;
	int nextCachedSnapshotFrames;
	void                *cachedSnapshotFrames;      /* +0x48 -- really the ENTITY ring */
	void                *cachedSnapshotEntities;    /* +0x4C -- really the CLIENT ring */
	void                *cachedSnapshotClients;     /* +0x50 -- really the FRAME ring */

	int nextHeartbeatTime;
	challenge_t challenges[MAX_CHALLENGES];
	netadr_t redirectAddress;
	netadr_t authorizeAddress;
	void            *pOOBProf;
} serverStatic_t;
SV_ASSERT_SIZE( serverStatic_t, 45188 );

#define SVS_CACHED_ENTITIES     ( (archivedEntity_t *)svs.cachedSnapshotFrames )
#define SVS_CACHED_CLIENTS      ( (cachedClient_t *)svs.cachedSnapshotEntities )
#define SVS_CACHED_FRAMES       ( (cachedSnapshot_t *)svs.cachedSnapshotClients )

extern server_t sv;                     /* 0x016515A0, local to the server */
extern serverStatic_t svs;              /* 0x016B2AA0, persistent across maps */

extern cvar_t   *sv_gametype;           /* 0x016BDB78 */
extern cvar_t   *sv_mapname;            /* 0x016BDB7C */
extern cvar_t   *sv_privateClients;     /* 0x016BDB54 */
extern cvar_t   *sv_hostname;           /* 0x016BDB5C */
extern cvar_t   *sv_maxclients;         /* 0x016BDB58 */
extern cvar_t   *sv_maxRate;            /* 0x016BDB80 */
extern cvar_t   *sv_minPing;            /* 0x016BDB8C */
extern cvar_t   *sv_maxPing;            /* 0x01651584 */
extern cvar_t   *sv_floodProtect;       /* 0x016BDB6C */
extern cvar_t   *sv_allowAnonymous;     /* 0x016BDB64 */
extern cvar_t   *sv_showCommands;       /* 0x016BDB84 */
extern cvar_t   *sv_serverid;           /* 0x016BDBAC */
extern cvar_t   *sv_pure;               /* 0x016B2A8C */
extern cvar_t   *sv_rconPassword;       /* 0x016B2A90 */
extern cvar_t   *sv_privatePassword;    /* 0x016BDB94 */
extern cvar_t   *sv_fps;                /* 0x016BDBA8 */
extern cvar_t   *sv_timeout;            /* 0x016BDBA0 */
extern cvar_t   *sv_zombietime;         /* 0x016BDB90 */
extern cvar_t   *sv_allowDownload;      /* 0x016BDB70 */
extern cvar_t   *sv_reconnectlimit;     /* 0x016BDB68 */
extern cvar_t   *sv_showloss;           /* 0x016BDB60 */
extern cvar_t   *sv_debugSpawn;
extern cvar_t   *sv_padPackets;         /* 0x016BDBA4 */
extern cvar_t   *sv_killserver;         /* 0x016BDB74 */
extern cvar_t   *sv_onlyVisibleClients; /* 0x016BDB98 */
extern cvar_t   *sv_showAverageBPS;     /* 0x016BDB9C */
extern cvar_t   *sv_mapRotation;        /* 0x016BDBB0 */
extern cvar_t   *sv_mapRotationCurrent; /* 0x016BDB88 */

extern cvar_t   *sv_master[MAX_MASTER_SERVERS];   /* 0x016BDB40 */

/* Resolved master addresses, one per sv_masterN. Retail 0x008E21D8, stride 20; storage owned by this unit. */
extern netadr_t sv_masterAdr[MAX_MASTER_SERVERS];

/* The game VM. Retail 0x014073D0; storage owned by the server. */
extern vm_t     *vm;

extern int xanim_activePoolSlot;                /* 0x00A9CC58, XAnim pool selector; UO calls this XAnimSetUser */
extern int xmodel_enforceExist;                /* 0x00572D68, cl_xmodelcheck latch */
extern int sv_rollingServerId;               /* 0x014073DC, rolling sv_serverid */
#include "../client_mp/cl_refapi.h"   /* re_ShutdownPtr, 0x01432860 */
extern int sv_rconLastTime;               /* 0x014073D4, SVC_RemoteCommand throttle */

/* script/scr_vm_state.c owns scrVmGlob.loading (retail 0x00A7A5E0) and exposes it through this setter, which is what SV_Frame's inline store becomes. */
void Scr_SetLoading( int bLoading );

/* the hunk low counters themselves (universal/com_memory_core.c), read directly as retail does */
extern int hunk_low_mark;               /* 0x008931BC */
extern int hunk_low_permanent;          /* 0x008931C4 */
extern int hunk_low_temp;               /* 0x008931C8 */
#define SV_HUNK_LOW_PERM        hunk_low_permanent
#define SV_HUNK_LOW_TEMP        hunk_low_temp
#define SV_HUNK_GAME_BASE       hunk_low_mark

void SV_SetConfigstring( unsigned int index, const char *val );
/* Retail is __usercall(index@edi, buffer@ebx, bufferSize@esi); see game syscall 28 in sv_game_mp.c. */
void SV_GetConfigstring( unsigned int index, char *buffer, int bufferSize );
/* Same parameter order as SV_SetConfigValueForKey (UO's SV_GetConfigValueForKey); the order fixes VC7's start+max+i add order. */
const char *SV_GetConfigstringConst( int start, const char *key, int max );
void SV_SetConfigValueForKey( int start, const char *key, const char *value, int max );
void SV_SetUserinfo( int index, const char *userinfo );
void SV_GetUserinfo( int index, char *buffer, int bufferSize );
void SV_CreateBaseline( void );
void SV_BoundMaxClients( int minimum );
void SV_Startup( void );
void SV_ChangeMaxClients( void );
void SV_SetExpectedHunkUsage( const char *mapname );
void SV_ClearServer( void );
void SV_EnableArchivedSnapshot( qboolean enable );
void SV_InitArchivedSnapshot( void );
void SV_FreeArchivedSnapshot( void );
void SV_SpawnServer( const char *server );
void SV_Init( void );
void SV_FinalMessage( const char *message );
void SV_FreeClients( void );
void SV_Shutdown( const char *finalmsg );

char *SV_ExpandNewlines( const char *string );
qboolean SV_IsFirstTokenEqual( const char *a, const char *b );
int SV_CanReplaceServerCommand( client_t *client, const char *cmd );
void SV_CullIgnorableServerCommands( client_t *client );
void SV_AddServerCommand( client_t *client, int isReliable, const char *cmd );
void QDECL SV_SendServerCommand( client_t *client, int isReliable, const char *fmt, ... );
void SV_MasterHeartbeat( const char *hbname );
void SV_MasterGameCompleteStatus( void );
void SV_MasterShutdown( void );
int SV_GetClientScore( client_t *client );
void SV_FlushRedirect( char *outputbuf );
void SV_ConnectionlessPacket( netadr_t from, msg_t *msg );
void SV_PacketEvent( netadr_t from, msg_t *msg );
void SV_CalcPings( void );
void SV_FreeClientScriptId( client_t *client );
void SV_CheckTimeouts( void );
qboolean SV_CheckPaused( void );
void SV_RunFrame( void );
void SV_BotUserMove( client_t *client );
void SV_UpdateBots( void );
void SV_Frame( int msec );

void SV_AddOperatorCommands( void );
void SV_DropClient( client_t *drop, const char *reason );
void SV_ClientThink( client_t *cl, usercmd_t *cmd );
void SV_ExecuteClientMessage( client_t *cl, msg_t *msg );
/* SV_UserMove is __usercall(delta@eax, unused@edi, cl, msg) in retail. */
qboolean SV_ClientCommand( msg_t *msg, client_t *cl );
void SV_UserMove( qboolean delta, client_t *cl, msg_t *msg );
void SV_SendClientGameState( client_t *client );
void SV_ClientEnterWorld( client_t *cl, usercmd_t *cmd );
int MSG_ReadBitsCompress( const byte *input, byte *outputBuf, int readsize );
void SV_SendClientMessages( void );
void SV_SendClientSnapshot( client_t *client );
void SV_ArchiveSnapshot( void );
void SV_ShutdownGameProgs( void );
/* Two stack arguments at 0x00458E37: restart, then the GAME_GET_MATCH_STATE result SV_SpawnServer saved before the map load. Both forwarded into GAME_INIT. */
void SV_InitGameVM( qboolean restart, int matchState );
void SV_InitGameProgs( int matchState );
void SVC_Status( netadr_t from );
void SVC_GameCompleteStatus( netadr_t from );
void SVC_Info( netadr_t from );
void SVC_RemoteCommand( netadr_t from, msg_t *msg );
void SV_GetChallenge( netadr_t from );
void SV_DirectConnect( netadr_t from );
void SV_AuthorizeIpPacket( netadr_t from );
void SV_Netchan_Decode( client_t *client, byte *data, int len );

void MSG_BeginReading( msg_t *msg );
int MSG_ReadByte( msg_t *msg );
int MSG_ReadShort( msg_t *msg );
int MSG_ReadLong( msg_t *msg );
char *MSG_ReadStringLine( msg_t *msg );
void MSG_WriteReliableCommandToBuffer( const char *pszCommand, int iBufferSize, char *pszBuffer );

qboolean Netchan_Process( netchan_t *chan, msg_t *msg );
qboolean NET_CompareBaseAdr( netadr_t a, netadr_t b );
qboolean NET_StringToAdr( const char *s, netadr_t *a );
char *NET_AdrToString( netadr_t a );
qboolean QDECL NET_OutOfBandPrint( netsrc_t sock, netadr_t adr, const char *format, ... );
void NetProf_PrepProfiling( void **pProf );
void NetProf_AddPacket( netProfileStream_t *pStream, int iSize, int bFragment );

int SV_GameSystemCalls( int *parms );
void NET_Sleep( int msec );
void *Hunk_AllocAlignInternal( int size, int align );
void Cmd_TokenizeString2( const char *text_in, int max_tokens );
void CL_MapLoading( void );
void CIN_StopCinematic( int handle );
void CM_LoadMap( const char *name, qboolean clientload, int *checksum );
void Com_LoadSoundAliases( const char *name, int localized );
void FS_ClearPakReferences( int flags );
const char *FS_LoadedPakChecksums( void );
const char *FS_LoadedPakNames( void );
const char *FS_ReferencedPakChecksums( void );
const char *FS_ReferencedPakNames( void );
void Info_SetValueForKey_Big( char *s, const char *key, const char *value );
void ClearObjectInternal( unsigned short id );
void RemoveRefToObject( unsigned short id );
void FreeVariable( unsigned short id );

#endif
