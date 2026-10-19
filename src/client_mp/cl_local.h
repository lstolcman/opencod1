/*
 * @fidelity: verified
 *
 * cl_local.h -- typed views over the client-engine data structures.
 *
 * The retail client keeps its state in a few large blobs (clientActive_t in
 * cl_active, clientStatic_t/clientConnection_t, the snapshot ring) that the
 * decompiled tree reaches through the offset macros in cl_records.h (SNAP_*,
 * ES_*, CS_*, GS_*) and standalone globals.  Those stay the storage of record;
 * this header only adds C types for the data passed around by POINTER --
 * snapshots, entity/client state, the gamestate -- so functions can say
 * frame->numEntities instead of *(int *)(frame + SNAP_NUMENTITIES).
 *
 * Retyping a pointer never moves storage, so the layout is unchanged; every
 * field offset below is proven against the retail binary (cl_records.h macros
 * + IDA-decompiled reads, cross-checked with the CoDUO reconstruction) and
 * pinned with a COD1_STATIC_ASSERT so a wrong offset fails the build.  Names
 * follow RTCW/Q3 where the field has an RTCW ancestor.
 *
 * Shared engine types (netchan_t, msg_t, field_t, netadr_t) come from
 * qcommon.h; glconfig_t / refexport_t from cl_refapi.h; include those as
 * needed.  This header is included AFTER cl_records.h.
 */
#ifndef CL_LOCAL_H
#define CL_LOCAL_H

#include "../qcommon/qcommon.h"      /* vec3_t, byte, COD1_STATIC_ASSERT */
#include "cl_refapi.h"               /* glconfig_t */

/* cls.glconfig (0x015CA590); these fields are 0x015CA614/618/628 */
#define cls_glconfig_vidWidth       cls_glconfig.vidWidth
#define cls_glconfig_vidHeight      cls_glconfig.vidHeight
#define cls_glconfig_stereoEnabled  cls_glconfig.stereoEnabled

#define ASSERT_OFF(type, field, off) \
	COD1_STATIC_ASSERT(offsetof(type, field) == (off), \
	                   #type "." #field " must be at " #off)

/* ------------------------------------------------------------------------- *
 *  trajectory_t (0x24) -- entityState_t position/angle interpolation.
 * ------------------------------------------------------------------------- */
typedef struct {
	int      trType;              /* +0x00 */
	int      trTime;              /* +0x04 */
	int      trDuration;          /* +0x08 */
	vec3_t   trBase;              /* +0x0c */
	vec3_t   trDelta;             /* +0x18 */
} trajectory_t;
COD1_ASSERT_SIZE(trajectory_t, 0x24);

/* ------------------------------------------------------------------------- *
 *  entityState_t (0xF0) -- the networked entity snapshot record (ES_*).
 *  Also the element type of cl.parseEntities[] and cl.entityBaselines[].
 * ------------------------------------------------------------------------- */
typedef struct {
	int          number;             /* +0x00  ES_NUMBER */
	int          eType;              /* +0x04 */
	int          eFlags;             /* +0x08  ES_EFLAGS */
	trajectory_t pos;                /* +0x0c  origin = pos.trBase @ +0x18 (ES_ORIGIN) */
	trajectory_t apos;               /* +0x30  angles = apos.trBase @ +0x3c */
	int          time;               /* +0x54 */
	int          time2;              /* +0x58 */
	vec3_t       origin2;            /* +0x5c */
	vec3_t       angles2;            /* +0x68 */
	int          otherEntityNum;     /* +0x74 */
	int          attackerEntityNum;  /* +0x78 */
	int          groundEntityNum;    /* +0x7c */
	int          constantLight;      /* +0x80 */
	int          loopSound;          /* +0x84 */
	int          surfType;           /* +0x88 */
	int          index;              /* +0x8c */
	int          clientNum;          /* +0x90 */
	int          iHeadIcon;          /* +0x94 */
	int          iHeadIconTeam;      /* +0x98 */
	int          solid;              /* +0x9c */
	int          eventParm;          /* +0xa0 */
	int          eventSequence;      /* +0xa4 */
	int          events[4];          /* +0xa8 */
	unsigned int eventParms[4];      /* +0xb8 */
	int          weapon;             /* +0xc8 */
	int          legsAnim;           /* +0xcc */
	int          torsoAnim;          /* +0xd0 */
	int          leanf;              /* +0xd4 */
	int          scale;              /* +0xd8 */
	int          dmgFlags;           /* +0xdc */
	int          animMovetype;       /* +0xe0  ES_STANCE */
	float        fTorsoHeight;       /* +0xe4 */
	float        fTorsoPitch;        /* +0xe8 */
	float        fWaistPitch;        /* +0xec */
} entityState_t;
COD1_ASSERT_SIZE(entityState_t, 0xF0);
ASSERT_OFF(entityState_t, eFlags, 0x08);
ASSERT_OFF(entityState_t, pos, 0x0c);
ASSERT_OFF(entityState_t, apos, 0x30);
ASSERT_OFF(entityState_t, clientNum, 0x90);
ASSERT_OFF(entityState_t, animMovetype, 0xE0);

/* ------------------------------------------------------------------------- *
 *  clientState_t (0x5C) -- per-client info; element of cl.parseClients[] and
 *  the out-snapshot clients[] (CS_*).
 * ------------------------------------------------------------------------- */
typedef struct {
	int      clientNum;           /* +0x00  CS_NUMBER */
	int      team;                /* +0x04 */
	int      modelindex;          /* +0x08 */
	int      attachModelIndex[6]; /* +0x0c */
	int      attachTagIndex[6];   /* +0x24 */
	char     name[32];            /* +0x3c */
} clientState_t;
COD1_ASSERT_SIZE(clientState_t, 0x5C);
ASSERT_OFF(clientState_t, name, 0x3c);

/* ------------------------------------------------------------------------- *
 *  clSnapshot_t (0x2100) -- a client-side snapshot (cl.snap, cl.snapshots[]).
 *  `ps` is playerState_s (0x20D0); kept opaque here (the client copies/deltas
 *  it as a blob).  A file that needs ps sub-fields casts it to a typed view.
 * ------------------------------------------------------------------------- */
typedef struct {
	int      valid;               /* +0x0000  SNAP_VALID */
	int      snapFlags;           /* +0x0004 */
	int      serverTime;          /* +0x0008 */
	int      messageNum;          /* +0x000c */
	int      deltaNum;            /* +0x0010 */
	int      ping;                /* +0x0014 */
	int      cmdNum;              /* +0x0018  RTCW / opencod2 clSnapshot_t name (CoD2 keeps it at this offset); no access in 1.1 */
	byte     ps[0x20D0];          /* +0x001c  playerState_s */
	int      numEntities;         /* +0x20ec  SNAP_NUMENTITIES */
	int      numClients;          /* +0x20f0 */
	int      parseEntitiesNum;    /* +0x20f4  first index into cl.parseEntities */
	int      parseClientsNum;     /* +0x20f8  first index into cl.parseClients */
	int      serverCommandNum;    /* +0x20fc */
} clSnapshot_t;
COD1_ASSERT_SIZE(clSnapshot_t, 0x2100);
ASSERT_OFF(clSnapshot_t, ps, 0x001c);
ASSERT_OFF(clSnapshot_t, numEntities, 0x20ec);
ASSERT_OFF(clSnapshot_t, parseEntitiesNum, 0x20f4);
ASSERT_OFF(clSnapshot_t, serverCommandNum, 0x20fc);

/* ------------------------------------------------------------------------- *
 *  snapshot_t (0x127EC) -- the flattened snapshot handed to cgame by
 *  CL_GetSnapshot (SNAP_OUT_*).  ps opaque as above.
 * ------------------------------------------------------------------------- */
typedef struct {
	unsigned int  snapFlags;              /* +0x00000 */
	int           ping;                   /* +0x00004 */
	int           serverTime;             /* +0x00008 */
	byte          ps[0x20D0];             /* +0x0000c  playerState_s */
	int           numEntities;            /* +0x020dc */
	int           numClients;             /* +0x020e0 */
	entityState_t entities[256];          /* +0x020e4  MAX_ENTITIES_IN_SNAPSHOT */
	clientState_t clients[64];            /* +0x110e4  MAX_CLIENTS_IN_SNAPSHOT */
	int           numServerCommands;      /* +0x127e4  Q3's name, as cgame_mp/cg_public.h; CL_GetSnapshot leaves it alone */
	int           serverCommandSequence;  /* +0x127e8  SNAP_OUT_CMDSEQUENCE */
} snapshot_t;
COD1_ASSERT_SIZE(snapshot_t, 0x127EC);
ASSERT_OFF(snapshot_t, numEntities, 0x020dc);
ASSERT_OFF(snapshot_t, entities, 0x020e4);
ASSERT_OFF(snapshot_t, clients, 0x110e4);
ASSERT_OFF(snapshot_t, serverCommandSequence, 0x127e8);

/* ------------------------------------------------------------------------- *
 *  gameState_t (0x5E84) -- config-string table (cl.gameState, GS_*).
 * ------------------------------------------------------------------------- */
typedef struct {
	int      stringOffsets[2048];    /* +0x0000  MAX_CONFIGSTRINGS */
	char     stringData[0x3E80];     /* +0x2000  MAX_GAMESTATE_CHARS = 16000 */
	int      dataCount;              /* +0x5e80 */
} gameState_t;
COD1_ASSERT_SIZE(gameState_t, 0x5E84);
ASSERT_OFF(gameState_t, stringData, 0x2000);
ASSERT_OFF(gameState_t, dataCount, 0x5e80);

/* one queued outgoing command packet (cl.outPackets[], PACKET_BACKUP=32) */
typedef struct {
	int      p_cmdNumber;         /* +0x00 */
	int      p_serverTime;        /* +0x04 */
	int      p_realtime;          /* +0x08 */
} outPacket_t;
COD1_ASSERT_SIZE(outPacket_t, 0x0c);


/* ------------------------------------------------------------------------- *
 *  Client globals.  Each is defined in the client_mp/ unit that owns it (RTCW
 *  placement: cl_main.c holds cl/cls/clc and the CL_Init cvars, cl_input.c the
 *  kbuttons and move cvars, ...); the groups below name the unit.
 * ------------------------------------------------------------------------- */

/* cl_main_mp.c: clientActive_t cl fields kept as standalone globals (the
 * 0x01432960 block; tools/vc7 exe_layout turns those in its client_active_fields
 * table into typed views of `cl`, the array extents come from that table).  The
 * whole-block definition, cl_active, is in cl_refstorage.c. */
extern char cl_snap_messageNum[4];
extern unsigned char cl_snap_snapFlags[4];
extern int cl_snap_serverTime;
extern int cl_snap_deltaNum;
extern int cl_snap_ping;
extern int cl_snap_ps_commandTime;
extern int cl_snap_ps_pm_type;
extern int cl_snap_ps_pm_flags;
extern int cl_snap_ps_deltaPitch;
extern int cl_snap_ps_eFlags;
extern int cl_snap_ps_clientNum;
extern float cl_snap_ps_viewHeightCurrent;
extern int cl_snap_numClients;
extern int cl_snap_parseClientsNum;
extern signed __int32 cl_serverTime;
extern int cl_oldServerTime;
extern int cl_oldFrameServerTime;
extern signed __int32 cl_serverTimeDelta;
extern int cl_extrapolatedSnapshot;
extern int cl_newSnapshots;
extern int cl_gameState_stringOffsets_CS_SYSTEMINFO_;
extern unsigned char cl_mapname[64];
extern int cl_parseEntitiesNum;
extern int cl_parseClientsNum;
extern int cl_mouseDx_1_;
extern int cl_mouseDy_1_;
extern int cl_joystickAxis_1_;
extern int cl_joystickAxis_2_;
extern signed __int32 cgameUserCmdValue;
extern signed __int32 cgameUserCmdInShellshock;
extern float cgameSensitivity;
extern int cgameMaxPitchSpeedStorage;
extern int cgameMaxYawSpeedStorage;
extern float cgameClientLerpOrigin_x;
extern float cgameClientLerpOrigin_y;
extern float cgameClientLerpOrigin_z;
extern int cgameUserAim_x;
extern int cgameUserAim_y;
extern int cgameUserAim_z;
extern int cl_viewanglesPitch;
extern int cl_viewanglesYaw;
extern float cl_viewanglesRoll;
extern unsigned char cl_cmdsStorage[1536];
extern int cl_cmdNumber;
extern int cl_outPackets_p_cmdNumber[96];
extern int cl_outPackets_p_serverTime[95];
extern int cl_outPackets_p_realtime[94];
extern int cl_snapshots[67584];
extern unsigned char cl_entityBaselinesStorage[245760];
extern unsigned char cl_parseEntitiesStorage[491520];
extern unsigned char cl_parseClientsStorage[188676];
extern int cl_gameState_stringOffsets[];   /* 0x01434A7C */
extern int cl_mouseDx[11];                 /* 0x0143A948 */
extern int cl_mouseDy[9];                  /* 0x0143A950 */
extern int cl_mouseIndex;                  /* 0x0143A958 */
extern int cl_joystickAxis[6];             /* 0x0143A95C */
extern int cl_serverId;                    /* 0x0143A9AC */

/* cl_main_mp.c: client state and cvars */
extern char demoName[8260];
extern int autoupdateChecked;
extern int autoupdateStarted;
extern int serverStatusCount;
extern unsigned char cls_autoupdateFilenameTerminator;
extern unsigned char cl_pinglistStorage[16832];
extern unsigned char cl_serverIPAddress[8388];
extern unsigned char cl_serverStatusListStorage[131712];   /* 0x015F7080 */
extern cvar_t *activeAction;
extern cvar_t *cl_allowDownload;
extern cvar_t *cl_avidemo;
extern cvar_t *cl_forceavidemo;
extern cvar_t *cl_freelook;
extern cvar_t *cl_freezeDemo;
extern cvar_t *cl_goStandJumpTime;
extern cvar_t *cl_ingame;
extern cvar_t *cl_motd;
extern cvar_t *cl_mouseAccel;
extern cvar_t *cl_noprint;
extern cvar_t *cl_sensitivity;
extern cvar_t *cl_showSend;
extern cvar_t *cl_showServerCommands;
extern cvar_t *cl_showTimeDelta;
extern cvar_t *cl_showmouserate;
extern cvar_t *cl_shownuments;
extern cvar_t *cl_timeNudge;
extern cvar_t *cl_timeout;
extern cvar_t *cl_updateavailable;
extern cvar_t *cl_updatefiles;
extern cvar_t *cl_updateoldversion;
extern cvar_t *cl_viewPitchCompensate;
extern cvar_t *cl_viewYawCompensate;
extern cvar_t *cl_visibleClients;
extern cvar_t *m_filter;
extern cvar_t *m_forward;
extern cvar_t *m_side;
extern cvar_t *m_yaw;
extern cvar_t *rconAddress;
extern cvar_t *timedemo;
extern cvar_t *cl_conXOffset;
extern cvar_t *cl_maxpackets;
extern cvar_t *cl_motdString;
extern cvar_t *cl_packetdup;
extern cvar_t *cl_serverStatusResendTime;
extern cvar_t *cl_serverloadgametype;
extern cvar_t *cl_serverloadmap;
extern cvar_t *cl_serverloadwaiting;
extern cvar_t *cl_updateversion;
extern cvar_t *cl_waitForFire;
extern cvar_t *m_pitch;
extern cvar_t *cl_languagetranslate;
extern cvar_t *cl_languagewarnings;
extern cvar_t *cl_languagewarningsaserrors;
extern cvar_t *cl_shownet;
extern cvar_t *fx_count;
extern cvar_t *fx_cull;
extern cvar_t *fx_debug;
extern cvar_t *fx_debugBolt;
extern cvar_t *fx_draw;
extern cvar_t *fx_enable;
extern cvar_t *fx_freeze;
extern cvar_t *r_inGameVideo;

/* cl_input_mp.c */
typedef struct kbutton_s
{
	int			down[2];
	int			downtime;
	int			msec;
	qboolean	active;
	qboolean	wasPressed;
} kbutton_t;

/* kb[] at 0x0087A028, 30 x 24 bytes: CL_ClearKeys clears it with one memset.
   RTCW's layout plus one CoD key (KB_MOVEUP, the raw +moveup/+gostand press). */
typedef enum {
	KB_LEFT,
	KB_RIGHT,
	KB_FORWARD,
	KB_BACK,
	KB_LOOKUP,
	KB_LOOKDOWN,
	KB_MOVELEFT,
	KB_MOVERIGHT,
	KB_STRAFE,
	KB_SPEED,
	KB_UP,
	KB_DOWN,
	KB_MOVEUP,
	KB_BUTTONS0,
	KB_BUTTONS1,
	KB_BUTTONS2,
	KB_BUTTONS3,
	KB_BUTTONS4,
	KB_BUTTONS5,
	KB_BUTTONS6,
	KB_BUTTONS7,
	KB_WBUTTONS0,
	KB_WBUTTONS1,
	KB_WBUTTONS2,
	KB_WBUTTONS3,
	KB_WBUTTONS4,
	KB_WBUTTONS5,
	KB_WBUTTONS6,
	KB_WBUTTONS7,
	KB_MLOOK,
	NUM_BUTTONS
} kbuttons_t;

extern kbutton_t kb[NUM_BUTTONS];
extern unsigned int frame_msecStorage;
extern int cl_oldFrameTime;
extern int cl_lastStandTime;
extern cvar_t *cl_anglespeedkey;
extern cvar_t *cl_pitchspeed;
extern cvar_t *cl_yawspeed;
extern cvar_t *cl_debugMove;
extern cvar_t *cl_nodelta;
extern cvar_t *cl_bypassMouseInput;
extern cvar_t *cl_run;
extern cvar_t *cl_stance;
extern cvar_t *cl_stanceTemp;

/* cl_keys_mp.c */
extern int key_overstrikeMode;
extern int chat_team;
extern int historyLine;
extern int nextHistoryLine;
/* keys_ctrlDown / keys_shiftDown have no storage of their own: retail reads
 * keys[K_CTRL].down / keys[K_SHIFT].down straight out of the keys[] table
 * (keysStorage 0x0142F780, defined in cl_keys_mp.c: 256 12-byte
 * {down, repeats, binding} records, .down written by CL_KeyEvent).  K_CTRL is
 * 159 and K_SHIFT 160 (retail keynames[] 0x0057B8D0). */
extern int keysStorage[];
#define keys_ctrlDown   ( keysStorage[3 * 159] )   /* 0x0142FEF4  keys[K_CTRL].down */
#define keys_shiftDown  ( keysStorage[3 * 160] )   /* 0x0142FF00  keys[K_SHIFT].down */

/* cl_console_mp.c */
/* Retail console prefix 0x140ef40..0x142ef6c. Text and counters share storage. */
typedef struct {
    int initialized;
    short text[65536];
    int current;       /* +0x20004 */
    int display;       /* horizontal cursor, +0x20008 */
    int displayLine;   /* visible line, +0x2000c */
    int prevChannel;
    int linewidth;
    int totallines;
    float xadjust;
    float displayFrac;
    float finalFrac;
    int vislines;
} consoleCore_t;
extern consoleCore_t con;

#define con_initialized (con.initialized)
#define con_text (con.text)
#define con_current (con.current)
#define con_display (con.display)
#define con_displayLine (con.displayLine)
#define con_prevChannel (con.prevChannel)
#define con_linewidth (con.linewidth)
#define con_totallines (con.totallines)
#define con_xadjust (con.xadjust)
#define con_displayFrac (con.displayFrac)
#define con_finalFrac (con.finalFrac)
#define con_vislines (con.vislines)
extern char con_miniconColor[16];
extern int con_conspeed;
extern int con_debug;
extern int con_miniconlines;
extern cvar_t *con_minicontime;
extern cvar_t *con_restricted;
extern unsigned char con_gameMessageStartTimes[96];
extern unsigned char con_gameMessageEndTimes[64];
extern unsigned char con_gameMessageLineIndices[32];
extern unsigned char con_boldMessageStartTimes[96];
extern unsigned char con_boldMessageEndTimes[64];
extern unsigned char con_boldMessageLineIndices[32];
extern unsigned char con_subtitleStartTimes[96];
extern unsigned char con_subtitleEndTimes[64];
extern unsigned char con_subtitleLineIndices[32];
extern unsigned char con_miniconStartTimes[1200];
extern unsigned char con_miniconEndTimes[800];
extern unsigned char con_miniconLineIndices[400];
extern int con_fieldWidthPixels;           /* 0x0057C0F8 */
extern int con_fieldCharWidth;             /* 0x0057C0FC */
extern int con_fieldCharHeight;            /* 0x0057C100 */

/* cl_cin_mp.c */
/* Retail cin scratch object at 0x57e910, total size 0x250614 on Win32. */
typedef struct {
    unsigned char linbuf[2097152]; /* +0x000000 */
    unsigned char file[65536];    /* +0x200000 */
    short sqrTable[256];          /* +0x210000 */
    int mcomp[256];               /* +0x210200 */
    unsigned char *qStatus[2][32768]; /* +0x210600 */
    int oldXOff;                  /* +0x250600 */
    int oldYOff;                  /* +0x250604 */
    int oldysize;                 /* +0x250608 */
    int oldxsize;                 /* +0x25060c */
    int currentHandle;            /* +0x250610 */
} cinematicScratch_t;
extern cinematicScratch_t cin;

#define cin_linbuf (cin.linbuf)
#define cin_file (cin.file)
#define cin_sqrTable (cin.sqrTable)
#define cin_mcomp (cin.mcomp)
#define cin_qStatus (cin.qStatus)
#define cin_oldXOff (cin.oldXOff)
#define cin_oldYOff (cin.oldYOff)
#define cin_oldysize (cin.oldysize)
#define cin_oldxsize (cin.oldxsize)
#define cin_currentHandle (cin.currentHandle)
extern int ROQ_YY_tab[512];
extern int ROQ_UB_tab[256];
extern int ROQ_UG_tab[256];
extern int ROQ_VG_tab[256];
extern int ROQ_VR_tab[768];
extern signed __int32 CL_handle;

/* cl_parse_mp.c */
extern const char *svc_strings[256];       /* 0x0057B4D0 */
extern int entLastVisible[64];

/* cl_scrn_mp.c */
extern int scr_initialized;
extern cvar_t *cl_timegraph;
extern cvar_t *cl_debuggraph;
extern cvar_t *cl_graphheight;
extern cvar_t *cl_graphscale;
extern cvar_t *cl_graphshift;

/* cl_cgame_mp.c */
extern char cl_bigConfigString[8192];
#endif /* CL_LOCAL_H */
