/*
 * @fidelity: likely
 */

#include "../qcommon/qcommon.h"
#include "cl_records.h"
#include "cl_local.h"
#include "cl_vm.h"

/* cl.timeoutcount, the first int of the clientActive_t block (0x01432960; storage in
 * cl_refstorage.c, which sizes it as the whole block) */
extern int cl_active;

/* cls.autoupdateFilename (0x0155F260, char[64]; cls_autoupdateFilename below) */
unsigned char cls_autoupdateFilenameStorage[64];   /* 0x0155F260 */
unsigned char cls_autoupdateFilenameTerminator;   /* 0x0155F29F, its last byte; unreferenced */

/* cvars (RTCW cl_main.c; registered in CL_Init) */
cvar_t *cl_conXOffset;              /* 0x01432840 */
cvar_t *cl_maxpackets;              /* 0x015F7064 */
cvar_t *cl_motdString;              /* 0x015F7070 */
cvar_t *cl_packetdup;               /* 0x015CE844 */
cvar_t *cl_serverStatusResendTime;  /* 0x01617328 */
cvar_t *cl_serverloadgametype;      /* 0x01617300 */
cvar_t *cl_serverloadmap;           /* 0x01432948 */
cvar_t *cl_serverloadwaiting;       /* 0x01617304 */
cvar_t *cl_updateversion;           /* 0x01617330 */
cvar_t *cl_waitForFire;             /* 0x015CE82C */
cvar_t *m_pitch;                    /* 0x0155F248 */
cvar_t *cl_language;                /* 0x0161734C */
cvar_t *cl_languagetranslate;       /* 0x01432954 */
cvar_t *cl_languagewarnings;        /* 0x01617350 */
cvar_t *cl_languagewarningsaserrors; /* 0x01432844 */
cvar_t *cl_shownet;                 /* 0x01407444 */
cvar_t *fx_count;                   /* 0x015CE83C */
cvar_t *fx_cull;                    /* 0x0161733C */
cvar_t *fx_debug;                   /* 0x01617310 */
cvar_t *fx_debugBolt;               /* 0x015CE824 */
cvar_t *fx_draw;                    /* 0x0161730C */
cvar_t *fx_enable;                  /* 0x01432854 */
cvar_t *fx_freeze;                  /* 0x015CE820 */
cvar_t *r_inGameVideo;              /* 0x01617318, the client's (the renderer registers its own) */
cvar_t *rconPassword;               /* 0x01617358 */
cvar_t *activeAction;
cvar_t *cl_allowDownload;
cvar_t *cl_avidemo;
cvar_t *cl_forceavidemo;
cvar_t *cl_freelook;
cvar_t *cl_freezeDemo;
cvar_t *cl_goStandJumpTime;
cvar_t *cl_ingame;
cvar_t *cl_motd;
cvar_t *cl_mouseAccel;
cvar_t *cl_noprint;
cvar_t *cl_sensitivity;
cvar_t *cl_showSend;
cvar_t *cl_showServerCommands;
cvar_t *cl_showTimeDelta;
cvar_t *cl_showmouserate;
cvar_t *cl_shownuments;
cvar_t *cl_timeNudge;
cvar_t *cl_timeout;
cvar_t *cl_updateavailable;
cvar_t *cl_updatefiles;
cvar_t *cl_updateoldversion;
cvar_t *cl_viewPitchCompensate;
cvar_t *cl_viewYawCompensate;
cvar_t *cl_visibleClients;
cvar_t *m_filter;
cvar_t *m_forward;
cvar_t *m_side;
cvar_t *m_yaw;
cvar_t *rconAddress;
cvar_t *timedemo;

clientConnection_t clc;             /* 0x015CE860, cl_static.h */
clientStatic_t cls;                 /* 0x0155F2C0, cl_static.h */

/* the client's copy of the renderer exports, 0x01432860 (cl_refapi.h names its slots) */
refexport_t re;

/* the cubemapShot command's face-name suffixes (CL_CubeMapShot_f, cl_scrn_mp.c; registered in CL_Init) */
const char *cl_cubemapFaceSuffixes[6] = { "_up", "_dn", "_lf", "_rt", "_ft", "_bk" };   /* 0x0057B4B4 */

/* clientActive_t cl fields kept as standalone globals (the 0x01432960 block; tools/vc7
 * exe_layout turns the ones in its client_active_fields table into typed views of `cl`) */
char cl_snap_messageNum[4];
float cgameClientLerpOrigin_x;
float cgameClientLerpOrigin_y;
float cgameClientLerpOrigin_z;
float cgameSensitivity;
float cl_viewanglesRoll;
float cl_snap_ps_viewHeightCurrent;   /* 0x01432A50 */
int cgameUserAim_x;
int cgameUserAim_y;
int cgameUserAim_z;
int cl_cmdNumber;
int cl_extrapolatedSnapshot;
int cl_gameState_stringOffsets_CS_SYSTEMINFO_;
int cl_newSnapshots;
int cl_oldFrameServerTime;
int cl_oldServerTime;
int cl_parseClientsNum;
int cl_parseEntitiesNum;
int cl_snap_deltaNum;
int cl_snap_ping;
int cl_snap_ps_commandTime;
int cl_snap_serverTime;
int cl_snapshots[67584];
int cl_viewanglesPitch;
int cl_viewanglesYaw;
int cl_snap_ps_pm_type;   /* 0x01432984 */
int cl_snap_ps_pm_flags;   /* 0x0143298C */
int cl_snap_ps_deltaPitch;   /* 0x014329C8 */
int cl_snap_ps_eFlags;   /* 0x01432A00 */
int cl_snap_ps_clientNum;   /* 0x01432A2C */
int cl_snap_numClients;   /* 0x01434A54 */
int cl_snap_parseClientsNum;   /* 0x01434A5C */
int cl_mouseDx_1_;   /* 0x0143A94C */
int cl_mouseDy_1_;   /* 0x0143A954 */
int cl_joystickAxis_1_;   /* 0x0143A960 */
int cl_joystickAxis_2_;   /* 0x0143A964 */
int cgameMaxPitchSpeedStorage;   /* 0x0143A980 */
int cgameMaxYawSpeedStorage;   /* 0x0143A984 */
int cl_outPackets_p_cmdNumber[96];   /* 0x0143AFB4 */
int cl_outPackets_p_serverTime[95];   /* 0x0143AFB8 */
int cl_outPackets_p_realtime[94];   /* 0x0143AFBC */
signed __int32 cgameUserCmdInShellshock;
signed __int32 cgameUserCmdValue;
signed __int32 cl_serverTime;
signed __int32 cl_serverTimeDelta;
unsigned char cl_entityBaselinesStorage[245760];   /* 0x0147D134 */
unsigned char cl_parseEntitiesStorage[491520];   /* 0x014B9134 */
unsigned char cl_parseClientsStorage[188676];   /* 0x01531134 */
unsigned char cl_mapname[64];
unsigned char cl_snap_snapFlags[4];
unsigned char cl_cmdsStorage[1536];   /* 0x0143A9B0 */
int cl_snap_valid[2112];             /* 0x01432964, cl.snap */
int cl_gameState_stringOffsets[6049];   /* 0x5E84 bytes, 0x01434A7C */
float yaw;          /* 0x01432A44 cl.snap.ps.viewangles[YAW]; unreferenced */
float leanFrac;     /* 0x014329C0 cl.snap.ps.leanFraction; unreferenced */

/* RTCW cl_main.c client state */
char demoName[8260];
int autoupdateChecked;
int autoupdateStarted;
int serverStatusCount;
unsigned char cl_pinglistStorage[16832];   /* 0x015CA660, cl_pinglist[] */
unsigned char cl_serverIPAddress[8388];   /* 0x0087A710 */
unsigned char cl_serverStatusListStorage[131712];   /* 0x015F7080, cl_serverStatusList[] */
char cl_cdkeychecksum[10] = "    ";

extern void CL_ParseServerMessage( msg_t *msg );
extern void MSS_StopSounds( int flags );        /* 0x0044FA40 */
extern void MSS_FadeAllSounds( float target, int msec );   /* 0x0044E0F0 */

extern int CIN_PlayCinematic();
extern int CIN_StopCinematic();
extern int CIN_UploadCinematic();
void CL_CubeMapShot_f( void );
void CL_InitCGame( void );          /* 0x004049C0 */
extern int CL_InitInput();
int CL_InitUI( void );              /* 0x00418F10, cl_ui_mp.c */
extern int CL_Netchan_Decode();
void CL_Netchan_SendOOBPacket( int length, const void *data, netadr_t to );
extern int CL_PlayCinematic_f();
extern int CL_RestoreCgameState( void *buffer, int size );
extern int CL_ShutdownInput();
extern void CL_UpdateLevelHunkUsage( void );
extern int CL_WritePacket();
extern int CM_GetPlaneNum();
extern int CM_SaveLump();
extern void Cmd_TokenizeString2( const char *text, int max_tokens );
qboolean Com_LoadCvarsFromBuffer( const char **cvarNames, int cvarCount,
                                  char *text, const char *filename );
qboolean Com_SaveCvarsToBuffer( const char **cvarNames, int cvarCount,
                                char *buffer, int bufferSize );
extern int Com_WriteLocalizedSoundAliasFiles();
extern int Con_Close();
extern int Con_Init();
extern int FastRound( float f );
extern int FS_ComparePaks();
extern int FS_FOpenFileRead_Internal();
extern int FS_FileExists();
extern int FS_FileIsInPAK();
extern int FS_LoadedPakNames();
extern int FS_ReferencedPakNames();
extern int FS_ReferencedPakPureChecksums();
extern int FS_ShiftStr();
extern void CL_InitRef( void );
extern void CL_ShutdownRef( void );
extern int Hunk_AllocInternal();
extern int Hunk_FreeTempMemory();
extern char *MSG_ReadBigString( msg_t *msg );
extern int   MSG_ReadLong( msg_t *msg );
extern char *MSG_ReadString( msg_t *msg );
extern char *MSG_ReadStringLine( msg_t *msg );
void MSG_WriteBigString( msg_t *sb, const char *s );
int  MSG_WriteBitsCompress( const byte *datasrc, int bytecount, byte *buffdest );
void MSG_WriteByte( msg_t *msg, int c );
void MSG_WriteShort( msg_t *msg, int c );
void MSG_WriteLong( msg_t *msg, int c );
void MSG_WriteDeltaEntity( msg_t *msg, const byte *from, const byte *to, qboolean force );

/* svc_* -- the canonical order of svc_strings at 0x0057B4D0, plus svc_EOF, which is the `8` CL_Record_f writes as its two end markers. */
#define svc_bad             0
#define svc_nop             1
#define svc_gamestate       2
#define svc_configstring    3
#define svc_baseline        4
#define svc_serverCommand   5
#define svc_download        6
#define svc_snapshot        7
#define svc_EOF             8
void MSG_WriteReliableCommandToBuffer( const char *pszCommand, int iBufferSize,
                                       char *pszBuffer );
extern void MSS_Init();
extern int MSS_Restore();
extern int MSS_Save();
extern int MSS_Shutdown();
extern const char *NET_AdrToString( netadr_t a );
extern qboolean    NET_CompareAdr( netadr_t a, netadr_t b );
extern qboolean    NET_CompareBaseAdr( netadr_t a, netadr_t b );
extern void        NET_OutOfBandData( netsrc_t sock, netadr_t adr, int len, const void *data );
extern void        NET_OutOfBandPrint( netsrc_t sock, netadr_t adr, const char *format, ... );
extern int NET_SendPacket();
extern qboolean    NET_StringToAdr( const char *s, netadr_t *a );
extern void        NetProf_AddPacket( void *pStream, int iSize, int bFragment );
extern void        NetProf_PrepProfiling( void *ppProfile );
extern qboolean    Netchan_Process( netchan_t *chan, msg_t *msg );
extern void        Netchan_Setup( netchan_t *chan, netsrc_t sock, netadr_t adr, int qport );
/* Defined below at 0x00413430; called from CL_ConnectionlessPacket above it. */
void __cdecl CL_ServerStatusResponse( netadr_t from, void *msg );
/* Same shape: CL_DemoCompleted (0x0040E920) tail-jumps to CL_NextDemo (0x0040ED80), and CL_MapLoading (0x0040EE60) calls CL_CheckForResend (0x004103D0); retail places both callees after their callers. */
void CL_NextDemo( void );
void __cdecl CL_CheckForResend( void );
/* Both are dispatched by CL_ConnectionlessPacket (0x004109D0) and both live after it in retail's layout. */
void CL_UpdateInfoPacket( netadr_t from );
void CL_ServerInfoPacket( netadr_t from, msg_t *msg );
extern int SCR_AdjustFrom640();
extern int SCR_Init();
extern int SCR_UpdateScreen();
extern int Sys_LoadingKeepAlive();
extern int Sys_ShowIP();
void Sys_OpenURL( const char *url, int doexit );
extern int Sys_StartProcess();
extern int Z_FreeInternal();
extern int j__atol();
extern int putenv_m();

#define CA_DISCONNECTED     0
#define CA_CONNECTING       1
#define CA_CHALLENGING      2
#define CA_CONNECTED        3
#define CA_LOADING          4
#define CA_PRIMED           5
#define CA_ACTIVE           6
#define CA_CINEMATIC        7
#define CA_LOGO             8

#define MAX_VIDEO_HANDLES   16
#define RETRANSMIT_TIMEOUT  3000

#ifndef PROTOCOL_VERSION
#define PROTOCOL_VERSION    1
#endif

/* `demoName` @ 0x0087A790 is CL_Record_f's own scratch buffer, RTCW cl_main.c:289
 * `static char demoName[MAX_QPATH]`.  It is a different object from clc.demoName
 * below; RTCW carries the same pair of names. */

/* clc.demoName @ 0x015EEFC0, char[64]. `clc_demoNameTerminator` is base+0x3F, the terminator strncpy does not write, not a second object. */

/* cls.updateInfoString @ 0x015CA028, char[MAX_INFO_STRING]. The declaration is char[1044] because the 20-byte cls.authorizeServer (cls_authorizeServer) follows it inside the same gap. */

/* cls.autoupdateFilename @ 0x0155F260, char[64] -- the terminator at cls_autoupdateFilenameTerminator is base+0x3F. */
#define cls_autoupdateFilename  ( (char *)&cls_autoupdateFilenameStorage )

#define MAX_PINGREQUESTS    16

typedef struct ping_s
{
	netadr_t adr;
	int start;
	int time;
	char info[MAX_INFO_STRING];
} ping_t;
COD1_ASSERT_SIZE( ping_t, 1052 );

#define cl_pinglist     ( (ping_t *)cl_pinglistStorage )

typedef struct serverInfo_s
{
	netadr_t adr;                       /* +0x00 */
	char hostName[MAX_NAME_LENGTH];     /* +0x14 */
	char mapName[MAX_NAME_LENGTH];      /* +0x34 */
	char game[MAX_NAME_LENGTH];         /* +0x54 */
	int netType;                        /* +0x74 */
	char gameType[MAX_NAME_LENGTH];     /* +0x78 -- a string in CoD, not an int */
	int clients;                        /* +0x98 */
	int maxClients;                     /* +0x9C */
	int minPing;                        /* +0xA0 */
	int maxPing;                        /* +0xA4 */
	int ping;                           /* +0xA8 */
	qboolean visible;                   /* +0xAC */
	int allowAnonymous;                 /* +0xB0 */
	int password;                       /* +0xB4 -- CoD addition */
} serverInfo_t;
COD1_ASSERT_SIZE( serverInfo_t, 184 );

#define MAX_OTHER_SERVERS   128
#define MAX_GLOBAL_SERVERS  2048
/* `cmp ebp, 100h` at 0x0041088F in CL_ServersResponsePacket. */
#define MAX_SERVERSPERPACKET    256

#define AS_LOCAL            0
#define AS_GLOBAL           1
#define AS_FAVORITES        2

typedef struct serverAddress_s
{
	byte ip[4];
	unsigned short port;
} serverAddress_t;
COD1_ASSERT_SIZE( serverAddress_t, 6 );

#define cls_globalServerAddresses       ( (serverAddress_t *)cls_globalServerAddressesStorage )

#define NUM_SERVER_PORTS    4

/* (0x015CA57C - 0x015CA43C) / 0x40 in CL_CheckAutoUpdate's resolve loop; cls_autoupdateServerNames_N_ are the five objects. */
#define MAX_AUTOUPDATE_SERVERS  5

#define MAX_SERVERSTATUSREQUESTS    16

typedef struct serverStatus_s
{
	char string[BIG_INFO_STRING];       /* +0x0000 */
	netadr_t address;                   /* +0x2000 */
	int time;                           /* +0x2014 */
	int startTime;                      /* +0x2018 */
	qboolean pending;                   /* +0x201C */
	qboolean print;                     /* +0x2020 */
	qboolean retrieved;                 /* +0x2024 */
} serverStatus_t;
COD1_ASSERT_SIZE( serverStatus_t, 8232 );

#define cl_serverStatusList     ( (serverStatus_t *)&cl_serverStatusListStorage )

#define cls_localServers        ( (serverInfo_t *)cls_localServersStorage )
#define cls_globalServers       ( (serverInfo_t *)&cls_globalServersStorage )
#define cls_favoriteServers     ( (serverInfo_t *)cls_favoriteServersStorage )

/* ---- CL_CDDialog  0x0040E2E0 ----  [CONFIRMED] */
void __cdecl CL_CDDialog()
{
  cls_cddialog = 1;
}

/* ---- CL_AddReliableCommand  0x0040E2F0 ----  VERIFIED */
void CL_AddReliableCommand( const char *cmd ) {
	int index;

	if ( clc_reliableSequence - clc_reliableAcknowledge > MAX_RELIABLE_COMMANDS ) {
		Com_Error( ERR_DROP, "EXE_ERR_CLIENT_CMD_OVERFLOW" );
	}
	clc_reliableSequence++;
	index = clc_reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	MSG_WriteReliableCommandToBuffer( cmd, MAX_STRING_CHARS,
									  &clc_reliableCommands[ MAX_STRING_CHARS * index ] );
}

/* ---- CL_ChangeReliableCommand  0x0040E340 ----  VERIFIED */
void CL_ChangeReliableCommand( void ) {
	int index, l;

	rand();
	index = clc_reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	l = strlen( &clc_reliableCommands[ MAX_STRING_CHARS * index ] );
	if ( l >= MAX_STRING_CHARS - 1 ) {
		l = MAX_STRING_CHARS - 2;
	}
	clc_reliableCommands[ MAX_STRING_CHARS * index + l ] = '\n';
	/* retail 0x0040E37E stores with no bound check; pointer store avoids the modern /GS range-check fast-fail */
	*( clc_reliableCommands + MAX_STRING_CHARS * index + l + 1 ) = '\0';
}

/* ---- CL_MakeMonkeyDoLaundry  0x0040E390 ----  VERIFIED */
void __cdecl CL_MakeMonkeyDoLaundry( void )
{
	if ( sys_monkeySpank && ( cls_framecount & 0xFF ) == 0 ) {
		if ( rand() * ( 1.0f / 32768.0f ) < 0.1 ) {
			CL_ChangeReliableCommand();
		}
	}
}

/* ---- CL_WriteDemoMessage  0x0040E3D0 ----  VERIFIED */
void CL_WriteDemoMessage( msg_t *msg, int headerBytes )
{
	int len, swlen;

	len = clc_serverMessageSequence;
	swlen = len;
	FS_Write( &swlen, 4, clc_demofile );

	len = msg->cursize - headerBytes;
	swlen = len;
	FS_Write( &swlen, 4, clc_demofile );

	FS_Write( msg->data + headerBytes, len, clc_demofile );
}

/* ---- CL_StopRecord_f  0x0040E430 ----  VERIFIED */
void CL_StopRecord_f( void )
{
  int len;

  if ( !clc_demorecording )
  {
    Com_Printf("Not recording a demo.\n");
    return;
  }
  len = -1;
  FS_Write(&len, 4, clc_demofile);
  FS_Write(&len, 4, clc_demofile);
  FS_FCloseFile(clc_demofile);
  clc_demofile = 0;
  clc_demorecording = 0;
  Com_Printf("Stopped demo.\n");
}

/* ---- CL_DemoFilename  0x0040E4B0 ----  VERIFIED */
void CL_DemoFilename( int number, char *fileName )
{
	int a, b, c, d;
	if ( number < 0 || number > 9999 ) {
		Com_sprintf( fileName, 256, "demo9999.tga" );
		return;
	}

	a = number / 1000;
	number -= a * 1000;
	b = number / 100;
	number -= b * 100;
	c = number / 10;
	d = number - c * 10;
	Com_sprintf( fileName, 256, "demo%i%i%i%i", a, b, c, d );
}

/* ---- CL_Record_f  0x0040E540 ----  VERIFIED */
void CL_Record_f( void ) {
	char name[MAX_OSPATH];
	byte bufData[MAX_MSGLEN];
	byte packet[MAX_MSGLEN];
	msg_t buf;
	int i;
	int len;
	byte *ent;
	byte nullstate[ES_SIZE];
	char *s;

	if ( Cmd_Argc() > 2 ) {
		Com_Printf( "record <demoname>\n" );
		return;
	}

	if ( clc_demorecording ) {
		Com_Printf( "Already recording.\n" );
		return;
	}

	if ( cls_state != CA_ACTIVE ) {
		Com_Printf( "You must be in a level to record.\n" );
		return;
	}

	if ( Cmd_Argc() == 2 ) {
		Q_strncpyz( demoName, Cmd_Argv( 1 ), 64 );
		Com_sprintf( name, sizeof( name ), "demos/%s.dm_%d", demoName,
					 PROTOCOL_VERSION );
	} else {
		int number;

		for ( number = 0; number <= 9999; number++ ) {
			CL_DemoFilename( number, demoName );
			Com_sprintf( name, sizeof( name ), "demos/%s.dm_%d", demoName,
						 PROTOCOL_VERSION );

			len = FS_ReadFile( name, NULL );
			if ( len <= 0 ) {
				break;
			}
		}
	}

	Com_Printf( "recording to %s.\n", name );
	clc_demofile = FS_FOpenFileWrite( name );
	if ( !clc_demofile ) {
		Com_Printf( "ERROR: couldn't open.\n" );
		return;
	}
	clc_demorecording = qtrue;
	Q_strncpyz( clc_demoName, demoName, sizeof( clc_demoName ) );

	clc_demowaiting = qtrue;

	MSG_Init( &buf, bufData, sizeof( bufData ) );

	MSG_WriteLong( &buf, clc_reliableSequence );

	MSG_WriteByte( &buf, svc_gamestate );
	MSG_WriteLong( &buf, clc_serverCommandSequence );

	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ ) {
		if ( !cl_gameState_stringOffsets[i] ) {
			continue;
		}
		s = cl_gameState_stringData + cl_gameState_stringOffsets[i];
		MSG_WriteByte( &buf, svc_configstring );
		MSG_WriteShort( &buf, i );
		MSG_WriteBigString( &buf, s );
	}

	memset( nullstate, 0, sizeof( nullstate ) );
	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		ent = &cl_entityBaselinesStorage[ES_SIZE * i];
		if ( !*(int *)( ent + ES_NUMBER ) ) {
			continue;
		}
		MSG_WriteByte( &buf, svc_baseline );
		MSG_WriteDeltaEntity( &buf, nullstate, ent, qtrue );
	}

	MSG_WriteByte( &buf, svc_EOF );

	MSG_WriteLong( &buf, clc_clientNum );
	MSG_WriteLong( &buf, clc_checksumFeed );

	MSG_WriteByte( &buf, svc_EOF );

	*(int *)packet = *(int *)buf.data;
	i = MSG_WriteBitsCompress( buf.data + 4, buf.cursize - 4, packet + 4 ) + 4;

	len = clc_serverMessageSequence;
	FS_Write( &len, 4, clc_demofile );

	len = i;
	FS_Write( &len, 4, clc_demofile );

	FS_Write( packet, i, clc_demofile );

}

/* ---- CL_DemoCompleted  0x0040E920 ----  VERIFIED */
void CL_DemoCompleted( void ) {
	if ( timedemo && timedemo->integer ) {
		int time;

		time = Sys_Milliseconds() - clc_timeDemoStart;
		if ( time > 0 ) {
			Com_Printf( "%i frames, %3.1f seconds: %3.1f fps\n",
						clc_timeDemoFrames,
						time * 0.001,   /* 0x0040E98B fmul dbl_568EA8 (0.001), not a divide */
						clc_timeDemoFrames * 1000.0 / time );
		}
	}

	CL_Disconnect( qtrue );
	CL_NextDemo();
}

/* ---- CL_ReadDemoMessage  0x0040E9C0 ----  [CONFIRMED] */
void __cdecl CL_ReadDemoMessage( void )
{
	msg_t msg;
	byte  msgData[MAX_MSGLEN];
	int   sequence;

	if ( !clc_demofile ) {
		CL_DemoCompleted();
		return;
	}

	if ( FS_Read( &sequence, 4, clc_demofile ) != 4 ) {
		CL_DemoCompleted();
		return;
	}
	clc_serverMessageSequence = sequence;

	if ( msgInit == qfalse ) {
		MSG_initHuffman();
	}

	memset( &msg, 0, sizeof( msg ) );
	msg.data = msgData;
	msg.maxsize = sizeof( msgData );

	if ( FS_Read( &msg.cursize, 4, clc_demofile ) != 4 || msg.cursize == -1 ) {
		CL_DemoCompleted();
		return;
	}
	if ( msg.cursize > msg.maxsize ) {
		Com_Error( ERR_DROP, "\x15" "CL_ReadDemoMessage: demoMsglen > MAX_MSGLEN" );   /* 0x005676E8 */
	}

	if ( FS_Read( msg.data, msg.cursize, clc_demofile ) != msg.cursize ) {
		Com_Printf( "Demo file was truncated.\n" );
		CL_DemoCompleted();
		return;
	}

	clc_lastPacketTime = cls_realtime;
	msg.readcount = 0;
	clc_reliableAcknowledge = MSG_ReadLong( &msg );
	if ( clc_reliableAcknowledge < clc_reliableSequence - 64 ) {
		clc_reliableAcknowledge = clc_reliableSequence;
	} else {
		CL_ParseServerMessage( &msg );
	}
}

/* ---- CL_PlayDemo_f  0x0040EB40 ----  VERIFIED */
void CL_PlayDemo_f( void ) {
	char name[MAX_OSPATH], extension[32];
	char *arg;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "playdemo <demoname>\n" );
		return;
	}

	if ( com_sv_running->integer ) {
		Com_Printf( "listen server cannot play a demo.\n" );
		return;
	}

	CL_Disconnect( qtrue );

	arg = Cmd_Argv( 1 );
	Com_sprintf( extension, sizeof( extension ), ".dm_%d", PROTOCOL_VERSION );
	if ( !Q_stricmp( arg + strlen( arg ) - strlen( extension ), extension ) ) {
		Com_sprintf( name, sizeof( name ), "demos/%s", arg );
	} else {
		Com_sprintf( name, sizeof( name ), "demos/%s.dm_%d", arg, PROTOCOL_VERSION );
	}

	fs_loadingMode = 1;
	FS_FOpenFileRead_Internal( name, &clc_demofile, qtrue, 0 );
	if ( !clc_demofile ) {
		Com_Error( ERR_DROP, va( "EXE_ERR_CANT_WRITE" "\x15" "%s", name ) );
	}
	Q_strncpyz( clc_demoName, Cmd_Argv( 1 ), sizeof( clc_demoName ) );

	Con_Close();

	clc_demoplaying = qtrue;
	cls_state = CA_CONNECTED;
	Q_strncpyz( cls_servername, Cmd_Argv( 1 ), sizeof( cls_servername ) );

	while ( cls_state >= CA_CONNECTED && cls_state < CA_PRIMED ) {
		CL_ReadDemoMessage();
	}
	clc_firstDemoFrameSkipped = qfalse;
}

/* ---- CL_StartDemoLoop  0x0040ED60 ----  VERIFIED */
void CL_StartDemoLoop( void ) {
	Cbuf_AddText( "d1\n" );
	cls_keyCatchers = 0;
}

/* ---- CL_NextDemo  0x0040ED80 ----  VERIFIED */
void CL_NextDemo( void ) {
	char v[MAX_STRING_CHARS];

	Q_strncpyz( v, Cvar_VariableString( "nextdemo" ), sizeof( v ) );
	v[MAX_STRING_CHARS - 1] = 0;
	Com_DPrintf( "CL_NextDemo: %s\n", v );
	if ( !v[0] ) {
		return;
	}

	Cvar_Set2( "nextdemo", "", qtrue );
	Cbuf_AddText( v );
	Cbuf_AddText( "\n" );
	Cbuf_Execute();
}

/* ---- CL_ShutdownAll  0x0040EE30 ----  [CONFIRMED] */
void __cdecl CL_ShutdownAll()
{
  CL_ShutdownCGame();
  CL_ShutdownUI();
  if ( re_ShutdownPtr )
    re_ShutdownPtr(0);
  cls_loadingPlaque = 0;
  cls_rendererStarted = 0;
}

/* ---- CL_MapLoading  0x0040EE60 ----  VERIFIED */
void CL_MapLoading( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}

	Con_Close();
	cls_keyCatchers = 0;

	if ( cls_state >= CA_CONNECTED && !Q_stricmp( cls_servername, "localhost" ) ) {
		cls_state = CA_CONNECTED;
		memset( cls_updateInfoString, 0, MAX_INFO_STRING );
		memset( clc_serverMessage, 0, 256 );
		memset( cl_gameState_stringOffsets, 0, GS_SIZE );  /* 0x01434A7C */
		clc_lastPacketSentTime = -9999;
		SCR_UpdateScreen();
	} else {
		Cvar_Set2( "nextmap", "", qtrue );
		CL_Disconnect( qtrue );
		Q_strncpyz( cls_servername, "localhost", sizeof( cls_servername ) );
		cls_state = CA_CHALLENGING;
		cls_keyCatchers = 0;
		SCR_UpdateScreen();
		clc_connectTime = -RETRANSMIT_TIMEOUT;
		NET_StringToAdr( cls_servername, (netadr_t *)clc_serverAddress );

		CL_CheckForResend();
	}

	MSS_FadeAllSounds( 0.0f, 0 );
}

/* ---- CL_ClearState  0x0040EF70 ---- */
void CL_ClearState( void )
{
	Com_Memset( &cl_active, 0, 1231064 );                     /* 0x01432960 <- real extent, cl_refstorage.c */
	Com_Memset( &cl_snap_valid, 0, 8448 );                    /* 0x01432964 <- real extent, defined above */
	Com_Memset( cl_snap_snapFlags, 0, 4 );                    /* 0x01432968 */
	Com_Memset( &cl_snap_serverTime, 0, 4 );                  /* 0x0143296C */
	Com_Memset( cl_snap_messageNum, 0, 4 );                   /* 0x01432970 */
	Com_Memset( &cl_snap_deltaNum, 0, 4 );                    /* 0x01432974 */
	Com_Memset( &cl_snap_ping, 0, 4 );                        /* 0x01432978 */
	Com_Memset( &cl_snap_ps_commandTime, 0, 4 );              /* 0x01432980 */
	Com_Memset( &cl_snap_ps_pm_type, 0, 4 );                       /* 0x01432984 */
	Com_Memset( &cl_snap_ps_pm_flags, 0, 4 );                       /* 0x0143298C */
	Com_Memset( &cl_snap_ps_deltaPitch, 0, 4 );                       /* 0x014329C8 */
	Com_Memset( &cl_snap_ps_eFlags, 0, 4 );                       /* 0x01432A00 */
	Com_Memset( &cl_snap_ps_clientNum, 0, 4 );                       /* 0x01432A2C */
	Com_Memset( &cl_snap_ps_viewHeightCurrent, 0, 4 );                         /* 0x01432A50 */
	Com_Memset( &cl_snap_numClients, 0, 4 );                       /* 0x01434A54 */
	Com_Memset( &cl_snap_parseClientsNum, 0, 4 );                       /* 0x01434A5C */
	Com_Memset( &cl_serverTime, 0, 4 );                       /* 0x01434A64 */
	Com_Memset( &cl_oldServerTime, 0, 4 );                    /* 0x01434A68 */
	Com_Memset( &cl_oldFrameServerTime, 0, 4 );                       /* 0x01434A6C */
	Com_Memset( &cl_serverTimeDelta, 0, 4 );                  /* 0x01434A70 */
	Com_Memset( &cl_extrapolatedSnapshot, 0, 4 );             /* 0x01434A74 */
	Com_Memset( &cl_newSnapshots, 0, 4 );                     /* 0x01434A78 */
	Com_Memset( cl_gameState_stringOffsets, 0, 24196 );       /* 0x01434A7C <- real extent, defined above */
	Com_Memset( &cl_gameState_stringOffsets_CS_SYSTEMINFO_, 0, 4 ); /* 0x01434A80 */
	Com_Memset( cl_gameState_stringData, 0, 16000 );          /* 0x01436A7C */
	Com_Memset( &cl_gameState_dataCount, 0, 4 );              /* 0x0143A8FC */
	Com_Memset( cl_mapname, 0, 64 );                         /* 0x0143A900 */
	Com_Memset( &cl_parseEntitiesNum, 0, 4 );                 /* 0x0143A940 */
	Com_Memset( &cl_parseClientsNum, 0, 4 );                  /* 0x0143A944 */
	Com_Memset( cl_mouseDx, 0, 44 );                       /* 0x0143A948 */
	Com_Memset( &cl_mouseDx_1_, 0, 4 );                       /* 0x0143A94C */
	Com_Memset( cl_mouseDy, 0, 36 );                       /* 0x0143A950 */
	Com_Memset( &cl_mouseDy_1_, 0, 4 );                       /* 0x0143A954 */
	Com_Memset( &cl_mouseIndex, 0, 4 );                       /* 0x0143A958 */
	Com_Memset( cl_joystickAxis, 0, 24 );                       /* 0x0143A95C */
	Com_Memset( &cl_joystickAxis_1_, 0, 4 );                       /* 0x0143A960 */
	Com_Memset( &cl_joystickAxis_2_, 0, 4 );                       /* 0x0143A964 */
	Com_Memset( &cgameUserCmdValue, 0, 4 );                   /* 0x0143A974 */
	Com_Memset( &cgameUserCmdInShellshock, 0, 4 );            /* 0x0143A978 */
	Com_Memset( &cgameSensitivity, 0, 4 );                    /* 0x0143A97C */
	Com_Memset( &cgameMaxPitchSpeedStorage, 0, 4 );                       /* 0x0143A980 */
	Com_Memset( &cgameMaxYawSpeedStorage, 0, 4 );                       /* 0x0143A984 */
	Com_Memset( &cgameClientLerpOrigin_x, 0, 4 );             /* 0x0143A988 */
	Com_Memset( &cgameClientLerpOrigin_y, 0, 4 );             /* 0x0143A98C */
	Com_Memset( &cgameClientLerpOrigin_z, 0, 4 );             /* 0x0143A990 */
	Com_Memset( &cgameUserAim_x, 0, 4 );                      /* 0x0143A994 */
	Com_Memset( &cgameUserAim_y, 0, 4 );                      /* 0x0143A998 */
	Com_Memset( &cgameUserAim_z, 0, 4 );                      /* 0x0143A99C */
	Com_Memset( &cl_viewanglesPitch, 0, 4 );                       /* 0x0143A9A0 */
	Com_Memset( &cl_viewanglesYaw, 0, 4 );                       /* 0x0143A9A4 */
	Com_Memset( &cl_viewanglesRoll, 0, 4 );                         /* 0x0143A9A8 */
	Com_Memset( &cl_serverId, 0, 4 );                       /* 0x0143A9AC */
	Com_Memset( cl_cmdsStorage, 0, 1536 );                      /* 0x0143A9B0 */
	Com_Memset( &cl_cmdNumber, 0, 4 );                        /* 0x0143AFB0 */
	Com_Memset( cl_outPackets_p_cmdNumber, 0, 384 );                      /* 0x0143AFB4 */
	Com_Memset( cl_outPackets_p_serverTime, 0, 380 );                      /* 0x0143AFB8 */
	Com_Memset( cl_outPackets_p_realtime, 0, 376 );                      /* 0x0143AFBC */
	Com_Memset( cl_snapshots, 0, 270336 );                    /* 0x0143B134 */
	Com_Memset( cl_entityBaselinesStorage, 0, 245760 );                    /* 0x0147D134 */
	Com_Memset( cl_parseEntitiesStorage, 0, 491520 );                    /* 0x014B9134 */
	Com_Memset( cl_parseClientsStorage, 0, 188676 );                    /* 0x01531134 */
}

/* ---- CL_Disconnect  0x0040EF90 ----  [HIGH] */
void __cdecl CL_Disconnect( qboolean showMainMenu )
{
  if ( com_cl_running )
  {
    if ( com_cl_running->integer )
    {
      Cvar_Set2("r_uiFullScreen", "1", qtrue);
      if ( clc_demorecording )
        CL_StopRecord_f();
      if ( clc_download )
      {
        FS_FCloseFile(clc_download);
        clc_download = 0;
      }
      cls_downloadName[0] = 0;
      cls_downloadTempName[0] = 0;
      Cvar_Set2("cl_downloadName", "", qtrue);
      autoupdateStarted = 0;
      cls_autoupdateFilename[0] = 0;
      if ( clc_demofile )
      {
        FS_FCloseFile(clc_demofile);
        clc_demofile = 0;
      }
      if ( uivm )
      {
        if ( showMainMenu )
          VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE);
      }
      if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES )
      {
        CIN_StopCinematic(CL_handle);
        MSS_StopSounds(0);
        CL_handle = -1;
      }
      if ( cls_state >= CA_CONNECTED )
      {
        CL_AddReliableCommand("disconnect");
        CL_WritePacket();
        CL_WritePacket();
        CL_WritePacket();
      }
      CL_ClearState();

      /* 0x0040F0A2: one rep stosd over all of clientConnection_t */
      memset( &clc, 0, sizeof( clc ) );

      cls_state = CA_DISCONNECTED;
      cl_connectedToPureServer = 0;
    }
  }
}

/* CL_ForwardCommandToServer (0x0040F0C0) is defined further down this unit. */


#define UPDATE_SERVER_NAME  "codmotd.activision.com"
#define PORT_UPDATE         20520
#define AUTHORIZE_SERVER_NAME   "codauthorize.activision.com"

/* ---- CL_RequestMotd  0x0040F110 ----  VERIFIED */
void CL_RequestMotd( void ) {
	char info[MAX_INFO_STRING];

	if ( !cl_motd->integer ) {
		return;
	}
	Com_Printf( "Resolving %s\n", UPDATE_SERVER_NAME );
	if ( !NET_StringToAdr( UPDATE_SERVER_NAME, &cls_updateServer ) ) {
		Com_Printf( "Couldn't resolve address\n" );
		return;
	}

	cls_updateServer.port = BigShort( PORT_UPDATE );
	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", UPDATE_SERVER_NAME,
				cls_updateServer.ip[0], cls_updateServer.ip[1],
				cls_updateServer.ip[2], cls_updateServer.ip[3],
				BigShort( cls_updateServer.port ) );

	info[0] = 0;

	Com_sprintf( cls_updateChallenge, sizeof( cls_updateChallenge ), "%i", rand() );

	Info_SetValueForKey( info, "challenge", cls_updateChallenge );
	Info_SetValueForKey( info, "renderer", cls_glconfig.renderer_string );
	Info_SetValueForKey( info, "version", ( (cvar_t *)com_shortversion )->string );

	NET_OutOfBandPrint( NS_CLIENT, cls_updateServer, "getmotd \"%s\"\n", info );
}

/* ---- CL_RequestAuthorization  0x0040F260 ----  VERIFIED */
void CL_RequestAuthorization( void ) {
	char nums[64];
	int i, j, l;
	cvar_t *fs;

	if ( CL_CDKeyValidate( cl_cdkey, cl_cdkeychecksum ) == qfalse ) {
		Com_Error( ERR_DROP, "EXE_ERR_INVALID_CD_KEY" );
		return;
	}

	if ( !( (netadr_t *)cls_authorizeServer )->port ) {
		Com_Printf( "Resolving %s\n", AUTHORIZE_SERVER_NAME );
		if ( !NET_StringToAdr( AUTHORIZE_SERVER_NAME, (netadr_t *)cls_authorizeServer ) ) {
			Com_Printf( "Couldn't resolve address\n" );
			return;
		}

		( (netadr_t *)cls_authorizeServer )->port = BigShort( PORT_AUTHORIZE );
		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", AUTHORIZE_SERVER_NAME,
					( (netadr_t *)cls_authorizeServer )->ip[0],
					( (netadr_t *)cls_authorizeServer )->ip[1],
					( (netadr_t *)cls_authorizeServer )->ip[2],
					( (netadr_t *)cls_authorizeServer )->ip[3],
					BigShort( ( (netadr_t *)cls_authorizeServer )->port ) );
	}
	if ( ( (netadr_t *)cls_authorizeServer )->type == NA_BAD ) {
		return;
	}

	if ( Cvar_VariableValue( "fs_restrict" ) ) {
		Q_strncpyz( nums, "demo", sizeof( nums ) );
	} else {
		j = 0;
		l = strlen( cl_cdkey );
		if ( l > 32 ) {
			l = 32;
		}
		for ( i = 0; i < l; i++ ) {
			if ( ( cl_cdkey[i] >= '0' && cl_cdkey[i] <= '9' )
				 || ( cl_cdkey[i] >= 'a' && cl_cdkey[i] <= 'z' )
				 || ( cl_cdkey[i] >= 'A' && cl_cdkey[i] <= 'Z' ) ) {
				nums[j] = cl_cdkey[i];
				j++;
			}
		}
		/* retail 0x0040F3E8 terminates with no bound check; pointer store avoids the modern /GS range-check fast-fail */
		*( nums + j ) = 0;
	}

	fs = Cvar_Get( "cl_anonymous", "0", CVAR_INIT | CVAR_SYSTEMINFO );
	NET_OutOfBandPrint( NS_CLIENT, *(netadr_t *)cls_authorizeServer,
						va( "getKeyAuthorize %i %s", fs->integer, nums ) );
}

/* ---- CL_ForwardToServer_f  0x0040F450 ----  [CONFIRMED] */
void __cdecl CL_ForwardToServer_f( void )
{
	if ( cls_state != CA_ACTIVE || clc_demoplaying ) {
		Com_Printf( "Not connected to a server.\n" );
	} else if ( cmd_argc > 1 ) {
		CL_AddReliableCommand( Cmd_Args() );
	}
}

/* ---- CL_Setenv_f  0x0040F490 ----  [CONFIRMED] */
void __cdecl CL_Setenv_f( void )
{
	int argc = Cmd_Argc();

	if ( argc > 2 ) {
		char buffer[1024];
		int i;

		/* build "NAME=value1 value2 ..." and hand it to putenv */
		strcpy( buffer, Cmd_Argv( 1 ) );
		strcat( buffer, "=" );

		for ( i = 2; i < argc; i++ ) {
			strcat( buffer, Cmd_Argv( i ) );
			strcat( buffer, " " );
		}

		putenv_m( buffer );
	} else if ( Cmd_Argc() == 2 ) {
		char *env = getenv( Cmd_Argv( 1 ) );

		if ( env ) {
			Com_Printf( "%s=%s\n", Cmd_Argv( 1 ), env );
		} else {
			/* retail (like Q3) passes the NULL env as an unused extra argument */
			Com_Printf( "%s undefined\n", Cmd_Argv( 1 ), env );
		}
	}
}

/* ---- CL_Disconnect_f  0x0040F5F0 ----  VERIFIED */
void CL_Disconnect_f( void ) {
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_StopCinematic( CL_handle );
		MSS_StopSounds( 0 );
		CL_handle = -1;
	}

	if ( cls_state != CA_DISCONNECTED && cls_state != CA_CINEMATIC
		 && cls_state != CA_LOGO ) {
		Com_Error( ERR_DISCONNECT, "EXE_DISCONNECTED_FROM_SERVER" );
	}
}

/* ---- CL_Reconnect_f  0x0040F640 ----  VERIFIED */
void CL_Reconnect_f( void )
{
	if ( !strlen( cls_servername ) || !strcmp( cls_servername, "localhost" ) ) {
		Com_Printf( "Can't reconnect to localhost.\n" );
		return;
	}

	Cbuf_AddText( va( "connect %s\n", cls_servername ) );
}

qboolean NET_IsLocalAddress( netadr_t adr );

/* ---- CL_Connect_f  0x0040F6A0 ----  [HIGH] */
void __cdecl CL_Connect_f( void )
{
	const char	*server;

	if ( cmd_argc != 2 ) {
		Com_Printf( "usage: connect [server]\n" );
		return;
	}

	MSS_StopSounds( 0 );
	Cvar_Set2( "r_uiFullScreen", "0", qtrue );
	CL_RequestMotd();
	clc_serverMessage[0] = 0;

	server = Cmd_Argv( 1 );

	if ( com_sv_running->integer && !strcmp( server, "localhost" ) ) {
		Com_Shutdown( "EXE_SERVERQUIT" );
	}

	Cvar_Set2( "sv_killserver", "1", qtrue );
	SV_Frame( 0 );

	CL_Disconnect( qtrue );
	Con_Close();

	strncpy( cls_servername, server, 0xFF );
	cls_servernameTerminator = 0;

	if ( !NET_StringToAdr( cls_servername, (netadr_t *) clc_serverAddress ) ) {
		Com_Printf( "Bad server address\n" );
		cls_state = CA_DISCONNECTED;
		return;
	}
	if ( !( (netadr_t *) clc_serverAddress )->port ) {
		( (netadr_t *) clc_serverAddress )->port = BigShort( 28960 );
	}
	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", cls_servername,
	            ( (netadr_t *) clc_serverAddress )->ip[0],
	            ( (netadr_t *) clc_serverAddress )->ip[1],
	            ( (netadr_t *) clc_serverAddress )->ip[2],
	            ( (netadr_t *) clc_serverAddress )->ip[3],
	            BigShort( ( (netadr_t *) clc_serverAddress )->port ) );

	if ( !NET_IsLocalAddress( *(netadr_t *) clc_serverAddress ) ) {
		if ( CL_CDKeyValidate( cl_cdkey, cl_cdkeychecksum ) == qfalse ) {
			Com_Error( ERR_DROP, "EXE_ERR_INVALID_CD_KEY" );
			return;
		}
	}
	if ( NET_IsLocalAddress( *(netadr_t *) clc_serverAddress ) ) {
		cls_state = CA_CHALLENGING;
	} else {
		cls_state = CA_CONNECTING;
	}

	cls_keyCatchers = 0;
	clc_connectTime = -99999;
	clc_connectPacketCount = 0;
	Cvar_Set2( "cl_currentServerAddress", server, qtrue );
}

/* ---- CL_Rcon_f  0x0040F880 ----  VERIFIED */
void CL_Rcon_f( void ) {
	char message[MAX_STRING_CHARS];
	netadr_t to;
	const char *src;
	const char *arg;
	int i, j, room, len;
	qboolean quote;

	if ( !rconPassword->string ) {
		Com_Printf( "You must set 'rcon_password' before\n"
					"issuing an rcon command.\n" );
		return;
	}

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;
	len = 4;

	src = "rcon ";
	do {
		if ( !*src ) break;
		message[len++] = *src++;
	} while ( len < MAX_STRING_CHARS );

	src = rconPassword->string;
	while ( len < MAX_STRING_CHARS && *src ) {
		message[len++] = *src++;
	}

	for ( i = 1; i < cmd_argc; i++ ) {
		if ( len < MAX_STRING_CHARS ) {
			src = " ";
			do {
				if ( !*src ) break;
				message[len++] = *src++;
			} while ( len < MAX_STRING_CHARS );
		}

		arg = Cmd_Argv( i );

		quote = qfalse;
		if ( !arg[0] ) {
			quote = qtrue;
		} else {
			room = MAX_STRING_CHARS - len;
			for ( j = 0; j < room; j++ ) {
				if ( !arg[j] ) {
					break;
				}
				if ( arg[j] <= ' ' ) {
					quote = qtrue;
					break;
				}
			}
		}

		if ( quote ) {
			if ( len >= MAX_STRING_CHARS ) {
				continue;
			}
			message[len++] = '"';
		}

		if ( len < MAX_STRING_CHARS ) {
			src = arg;
			do {
				if ( !*src ) break;
				message[len++] = *src++;
			} while ( len < MAX_STRING_CHARS );
		}

		if ( quote && len < MAX_STRING_CHARS ) {
			message[len++] = '"';
		}
	}

	if ( len == MAX_STRING_CHARS ) {
		Com_Printf( "rcon command too long\n" );
		return;
	}
	/* retail 0x0040F9EC terminates with no bound check; pointer store avoids the modern /GS range-check fast-fail */
	*( message + len ) = 0;

	if ( cls_state >= CA_CONNECTED ) {
		to = clc.netchan.remoteAddress;
	} else {
		if ( !strlen( rconAddress->string ) ) {
			Com_Printf( "You must either be connected,\n"
						"or set the 'rconAddress' cvar\n"
						"to issue rcon commands\n" );
			return;
		}
		NET_StringToAdr( rconAddress->string, &to );
		if ( to.port == 0 ) {
			to.port = BigShort( PORT_SERVER );
		}
	}

	CL_Netchan_SendOOBPacket( strlen( message ) + 1, message, to );
}

/* ---- CL_SendPureChecksums  0x0040FAC0 ----  VERIFIED */
void CL_SendPureChecksums( void ) {
	const char *pChecksums;
	char cMsg[MAX_INFO_VALUE];
	int i;

	pChecksums = FS_ReferencedPakPureChecksums();

	Com_sprintf( cMsg, sizeof( cMsg ), "Va " );
	Q_strcat( cMsg, sizeof( cMsg ), pChecksums );
	for ( i = 0; i < 2; i++ ) {
		cMsg[i] += 13 + ( i * 2 );
	}
	CL_AddReliableCommand( cMsg );
}

/* ---- CL_ResetPureClientAtServer  0x0040FBC0 ----  [CONFIRMED] */
void __cdecl CL_ResetPureClientAtServer()
{
	/* retail routes the literal through va() (0x0040FBC0), advancing va's ring buffer */
	CL_AddReliableCommand( va( "vdr" ) );
}

/* ---- CL_Vid_Restart_f  0x0040FBE0 ---- */
extern int s_hunkTotal, s_hunkHighTemp, hunk_low_temp;
extern int Hunk_MemoryRemaining(void);
void __cdecl CL_Vid_Restart_f( void )
{
	extern cvar_t *fs_gamedirvar;
	extern int CL_SaveCgameState( void *buffer, int size );   /* 0x00404F80 */

	void *hunkBuf;
	int   savedSize = 0;
	void *savedState = NULL;

	if ( com_sv_running->integer ) {
		Com_Printf( "Listen server cannot video restart.\n" );
		return;
	}

	if ( cgvm ) {
		hunkBuf = Hunk_AllocateTempMemoryInternal( 0 );
		savedSize  = CL_SaveCgameState( hunkBuf, Hunk_MemoryRemaining() );
		savedState = Z_MallocInternal( savedSize );
		memcpy( savedState, hunkBuf, savedSize );
		Hunk_FreeTempMemory( hunkBuf );
	}

	Cvar_Set2( "com_expectedhunkusage", "-1", qtrue );
	MSS_StopSounds( 1 );

	CL_ShutdownCGame();
	CL_ShutdownUI();
	CL_ShutdownRef();

	CL_ResetPureClientAtServer();
	FS_ClearPakReferences( 1 );   /* 0x0040FCE0 mov esi,1: keeps the +0x310/+0x313 reference bytes */

	CL_ShutdownCGame();
	CL_ShutdownUI();
	SV_ShutdownGameProgs();
	CIN_CloseAllVideos();
	Hunk_ClearToStart();
	VM_Clear();

	Cvar_Get( "cl_language", "0", 33 );
	Cvar_Get( "cl_languagetranslate", "1", 32 );
	Cvar_Get( "fs_ignoreLozalized", "0", 544 );

	if ( !com_sv_running->integer
	  && ( fs_gamedirvar->modified || clc_checksumFeed != fs_checksumFeed ) ) {
		FS_Restart( clc_checksumFeed );
	}
	SEH_UpdateLanguageInfo();

	cls_rendererStarted = 0;
	cls_loadingPlaque = 0;

	Cvar_Set2( "cl_paused", "0", qtrue );
	CL_InitRef();
	CL_StartHunkUsers();

	if ( cls_state > CA_CONNECTED && cls_state != CA_CINEMATIC && cls_state != CA_LOGO ) {
		CL_InitCGame();
		CL_SendPureChecksums();
	}

	if ( savedState ) {
		if ( cgvm ) {
			CL_RestoreCgameState( savedState, savedSize );
		}
		free( savedState );
	}
}
/* ---- CL_Snd_Restart_f  0x0040FDF0 ----  [CONFIRMED] */
void __cdecl CL_Snd_Restart_f( void )
{
	void *hunkBuf;
	int   savedSize;
	int available;
	void *savedState;

	if ( com_sv_running->integer ) {
		Com_Printf( "Listen server cannot sound restart.\n" );
		return;
	}

	hunkBuf = Hunk_AllocateTempMemoryInternal( 0 );

	available = s_hunkTotal;
	available -= hunk_low_temp;
	available -= s_hunkHighTemp;
	savedSize = MSS_Save( available, hunkBuf );

	savedState = Z_MallocInternal( savedSize );
	memcpy( savedState, hunkBuf, savedSize );
	Hunk_FreeTempMemory( hunkBuf );

	MSS_Shutdown();
	MSS_Init();
	CL_Vid_Restart_f();
	MSS_Restore( savedState, savedSize );
	free( savedState );
}

/* ---- CL_OpenedPK3List_f  0x0040FEC0 ----  [CONFIRMED] */
void __cdecl CL_OpenedPK3List_f( void )
{
	const char *names = FS_LoadedPakNames();
	Com_Printf( "Opened PK3 Names: %s\n", names );
}

/* ---- CL_ReferencedPK3List_f  0x0040FEE0 ----  [CONFIRMED] */
void __cdecl CL_ReferencedPK3List_f( void )
{
	const char *names = FS_ReferencedPakNames();
	Com_Printf( "Referenced PK3 Names: %s\n", names );
}

/* ---- CL_Configstrings_f  0x0040FF00 ----  VERIFIED */
void CL_Configstrings_f( void ) {
	int i;
	int ofs;

	if ( cls_state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n" );
		return;
	}

	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ ) {
		ofs = cl_gameState_stringOffsets[i];
		if ( !ofs ) {
			continue;
		}
		Com_Printf( "%4i: %s\n", i, &cl_gameState_stringData[ofs] );
	}
}

/* ---- CL_Clientinfo_f  0x0040FF50 ----  [CONFIRMED] */
void __cdecl CL_Clientinfo_f( void )
{
	Com_Printf( "--------- Client Information ---------\n" );
	Com_Printf( "state: %i\n", cls_state );
	Com_Printf( "Server: %s\n", cls_servername );
	Com_Printf( "User info settings:\n" );
	Info_Print( Cvar_InfoString( CVAR_USERINFO ) );
	Com_Printf( "--------------------------------------\n" );
}

/* ---- CL_DownloadsComplete  0x0040FFB0 ----  [HIGH] */
void __cdecl CL_DownloadsComplete( void )
{
	if ( autoupdateStarted ) {
		if ( strlen( cls_autoupdateFilename ) > 4 ) {
			char *fn = va( "%s/%s", FS_ShiftStr( "ni]Zm^l", 7 ), cls_autoupdateFilename );
			/* retail pushes only the literal 1 */
			Sys_StartProcess( fn, 1 );
		}
		autoupdateStarted = 0;
		CL_Disconnect( qtrue );
		return;
	}

	if ( cls_downloadRestart ) {
		cls_downloadRestart = 0;
		FS_Restart( clc_checksumFeed );
		CL_AddReliableCommand( "donedl" );
		return;
	}

	cls_state = CA_LOADING;
	Com_EventLoop();
	if ( cls_state != CA_LOADING ) {
		return;
	}

	Cvar_Set2( "r_uiFullScreen", "0", qtrue );
	if ( !com_sv_running->integer ) {
		CL_ShutdownAll();
		Hunk_Clear();
	} else if ( cgvm ) {
		return;
	}

	CL_StartHunkUsers();
	Cvar_Set2( "cl_paused", "1", qtrue );
	CL_InitCGame();
	CL_SendPureChecksums();
	CL_WritePacket();
	CL_WritePacket();
	CL_WritePacket();
}

/* ---- CL_BeginDownload  0x004100D0 ----  VERIFIED */
void CL_BeginDownload( const char *localName, const char *remoteName )
{
	Com_DPrintf(
		"***** CL_BeginDownload *****\nLocalname: %s\nRemotename: %s\n****************************\n",
		localName,
		remoteName );

	strncpy( (char *)&cls_downloadName, localName, 0xFFu );
	cls_downloadNameTerminator = 0;
	Com_sprintf( (char *)&cls_downloadTempName, 256, "%s.tmp", localName );

	Cvar_Set2( "cl_downloadName", remoteName, qtrue );
	Cvar_Set2( "cl_downloadSize", "0", qtrue );
	Cvar_Set2( "cl_downloadCount", "0", qtrue );
	Cvar_SetValue( "cl_downloadTime", (float)cls_realtime );

	*(int *)cls_downloadBlock = 0;
	cls_downloadCount = 0;

	CL_AddReliableCommand( va( "download %s", remoteName ) );
}

/* ---- CL_NextDownload  0x00410190 ----  VERIFIED */
void CL_NextDownload( void )
{
  char *s;
  char *remoteName, *localName;

  if ( *clc_downloadList ) {
    s = (char *)clc_downloadList;

    if ( *s == '@' ) {
      s++;
    }
    remoteName = s;

    if ( ( s = strchr( s, '@' ) ) == NULL ) {
      CL_DownloadsComplete();
      return;
    }

    *s++ = 0;
    localName = s;
    if ( ( s = strchr( s, '@' ) ) != NULL ) {
      *s++ = 0;
    } else {
      s = localName + strlen( localName );
    }
    CL_BeginDownload( localName, remoteName );

    cls_downloadRestart = 1;

    memmove( (char *)clc_downloadList, s, strlen( s ) + 1 );

    return;
  }

  CL_DownloadsComplete();
}

/* ---- CL_InitDownloads  0x00410240 ----  VERIFIED */
void CL_InitDownloads( void ) {
	char missingfiles[1024];
	char *dir = FS_ShiftStr( "ni]Zm^l", 7 );

	if ( autoupdateStarted
		 && NET_CompareAdr( *(netadr_t *)cls_autoupdateServer,
							*(netadr_t *)clc_serverAddress ) ) {
		if ( strlen( cl_updatefiles->string ) > 4 ) {
			Q_strncpyz( cls_autoupdateFilename, cl_updatefiles->string, 64 );
			Q_strncpyz( (char *)clc_downloadList,
						va( "@%s/%s@%s/%s", dir, cl_updatefiles->string,
							dir, cl_updatefiles->string ),
						sizeof( clc_downloadList ) );
			cls_state = CA_CONNECTED;
			CL_NextDownload();
			return;
		}
	} else if ( com_sv_running->integer || !cl_allowDownload->integer ) {
		if ( FS_ComparePaks( missingfiles, sizeof( missingfiles ), qfalse ) ) {
			Com_Printf( "\nWARNING: You are missing some files referenced by the server:\n"
						"%sYou might not be able to join the game\n"
						"Go to the settings menu to turn on autodownload, or get the file elsewhere\n\n",
						missingfiles );
		}
	} else if ( FS_ComparePaks( (char *)clc_downloadList,
								sizeof( clc_downloadList ), qtrue ) ) {
		Com_Printf( "Need paks: %s\n", clc_downloadList );

		if ( *clc_downloadList ) {
			cls_state = CA_CONNECTED;
			CL_NextDownload();
			return;
		}
	}

	CL_DownloadsComplete();
}

/* ---- CL_CheckForResend  0x004103D0 ----  [HIGH] */
void __cdecl CL_CheckForResend( void )
{
	float		qport;
	char	*userinfo;
	int		i;
	/* Retail's info and data are adjacent 1024-byte frame arrays in
	 * that order (esp+0Ch/40Ch, then the /GS cookie).  One buffer in the same
	 * order lets a 1014+ char userinfo run data's copy and closing '"'/NUL
	 * into the cookie, so the process dies at the cookie check after the
	 * packet is sent, as in retail (0x0041061A). */
	char	connectBuf[2 * 1024];
	char	*info = connectBuf;
	char	*data = connectBuf + 1024;

	if ( clc_demoplaying ) {
		return;
	}
	if ( cls_state != CA_CONNECTING && cls_state != CA_CHALLENGING ) {
		return;
	}
	if ( cls_realtime - clc_connectTime < 3000 ) {
		return;
	}

	clc_connectTime = cls_realtime;
	clc_connectPacketCount++;

	switch(cls_state) {
	default:
		Com_Error( ERR_FATAL, "\x15" "CL_CheckForResend: bad cls.state" );
		break;
	case CA_CHALLENGING:
	qport = Cvar_VariableValue( "net_qport" );

	userinfo = Cvar_InfoString( CVAR_USERINFO );
	strncpy( info, userinfo, 0x3FF );
	info[1023] = 0;
	Info_SetValueForKey( info, "protocol", va( "%i", 1 ) );
	Info_SetValueForKey( info, "qport", va( "%i", (int)qport ) );
	Info_SetValueForKey( info, "challenge", va( "%i", *(int *) clc_challenge ) );

	/* retail 0x004104F9..0x00410526: copies "connect " (0x567068) then sets [8] = '"' */
	strcpy( data, "connect " );
	data[8] = '"';
	/* retail 0x00410550..0x0041056E re-measures info on every pass
	 * (unsigned compare) and terminates/sizes the packet from the final i */
	for ( i = 0; (unsigned int)i < strlen( info ); i++ ) {
		data[9 + i] = info[i];
	}
	/* retail 0x00410570/0x00410578 store with no bound check; pointer stores avoid the modern /GS range-check fast-fail */
	*( data + 9 + i ) = '"';
	*( data + 10 + i ) = 0;

	NET_OutOfBandData( NS_CLIENT, *(netadr_t *) clc_serverAddress,
	                   i + 10, data );

	cvar_modifiedFlags &= ~CVAR_USERINFO;
		break;
	case CA_CONNECTING:
		if ( net_lanauthorize->integer
		     || !Sys_IsLANAddress( *(netadr_t *) clc_serverAddress ) ) {
			CL_RequestAuthorization();
		}
		NET_OutOfBandPrint( NS_CLIENT, *(netadr_t *) clc_serverAddress,
		                    "getchallenge" );
		break;
	
	}
}

/* ---- CL_DisconnectPacket  0x00410620 ----  VERIFIED */
void CL_DisconnectPacket( netadr_t from ) {
	if ( cls_state == CA_DISCONNECTED ) {
		return;
	}

	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		return;
	}

	if ( cls_realtime - clc_lastPacketTime >= 3000 ) {
		Com_Error( ERR_DROP, "EXE_SERVER_DISCONNECTED" );
	}
}

/* ---- CL_MotdPacket  0x00410680 ----  VERIFIED */
void CL_MotdPacket( netadr_t from ) {
	char *challenge;
	char *info;

	if ( !NET_CompareAdr( from, cls_updateServer ) ) {
		return;
	}

	info = Cmd_Argv( 1 );

	challenge = Info_ValueForKey( info, "challenge" );
	if ( strcmp( challenge, cls_updateChallenge ) ) {
		return;
	}

	challenge = Info_ValueForKey( info, "motd" );

	Q_strncpyz( cls_updateInfoString, info, MAX_INFO_STRING );
	Cvar_Set2( "cl_motdString", challenge, qtrue );
}

/* ---- CL_InitServerInfo  0x00410750 ----  VERIFIED */
void CL_InitServerInfo( serverInfo_t *server, serverAddress_t *address ) {
	server->adr.type = NA_IP;
	server->adr.ip[0] = address->ip[0];
	server->adr.ip[1] = address->ip[1];
	server->adr.ip[2] = address->ip[2];
	server->adr.ip[3] = address->ip[3];
	server->adr.port = address->port;

	server->clients = 0;
	server->hostName[0] = '\0';
	server->mapName[0] = '\0';
	server->maxClients = 0;
	server->maxPing = 0;
	server->minPing = 0;
	server->ping = -1;
	server->game[0] = '\0';
	server->gameType[0] = '\0';
	server->netType = 0;
	server->allowAnonymous = 0;
}

/* ---- CL_ServersResponsePacket  0x004107B0 ---- */
void CL_ServersResponsePacket( netadr_t from, msg_t *msg ) {
	int i, count, total;
	serverAddress_t addresses[MAX_SERVERSPERPACKET];
	int numservers;
	byte *buffptr;
	byte *buffend;

	Com_Printf( "CL_ServersResponsePacket\n" );

	if ( cls_numglobalservers == -1 ) {
		cls_numglobalservers = 0;
		cls_numGlobalServerAddresses = 0;
	}

	numservers = 0;
	buffptr = msg->data;
	buffend = buffptr + msg->cursize;
	while ( buffptr + 1 < buffend ) {
		do {
			if ( *buffptr++ == '\\' ) {
				break;
			}
		} while ( buffptr < buffend );

		if ( buffptr >= buffend - 6 ) {
			break;
		}

		addresses[numservers].ip[0] = *buffptr++;
		addresses[numservers].ip[1] = *buffptr++;
		addresses[numservers].ip[2] = *buffptr++;
		addresses[numservers].ip[3] = *buffptr++;

		addresses[numservers].port = (unsigned short)( *buffptr++ << 8 );
		addresses[numservers].port += *buffptr++;
		addresses[numservers].port = BigShort( addresses[numservers].port );

		if ( *buffptr != '\\' ) {
			break;
		}

		Com_DPrintf( "server: %d ip: %d.%d.%d.%d:%d\n", numservers,
		             addresses[numservers].ip[0],
		             addresses[numservers].ip[1],
		             addresses[numservers].ip[2],
		             addresses[numservers].ip[3],
		             addresses[numservers].port );

		numservers++;
		if ( numservers >= MAX_SERVERSPERPACKET ) {
			break;
		}

		if ( buffptr[1] == 'E' && buffptr[2] == 'O' && buffptr[3] == 'T' ) {
			break;
		}
	}

	count = cls_numglobalservers;

	for ( i = 0; i < numservers && count < MAX_GLOBAL_SERVERS; i++ ) {
		serverInfo_t *server = &cls_globalServers[count];

		CL_InitServerInfo( server, &addresses[i] );
		count++;
	}

	if ( cls_numGlobalServerAddresses < MAX_GLOBAL_SERVERS && i < numservers ) {
		do {
			if ( count < MAX_GLOBAL_SERVERS ) {
				break;
			}
			{
			 serverAddress_t *addr = &cls_globalServerAddresses[cls_numGlobalServerAddresses++];
			 addr->ip[0] = addresses[i].ip[0];
			 addr->ip[1] = addresses[i].ip[1];
			 addr->ip[2] = addresses[i].ip[2];
			 addr->ip[3] = addresses[i].ip[3];
			 addr->port = addresses[i].port;
			}
			i++;
		} while ( i < numservers );
	}

	cls_numglobalservers = count;
	total = count + cls_numGlobalServerAddresses;

	Com_Printf( "%d servers parsed (total %d)\n", numservers, total );
}

/* ---- CL_ConnectionlessPacket  0x004109D0 ----  [HIGH] */
void CL_ConnectionlessPacket( netadr_t from, msg_t *msg )
{
	char	*s;
	char	*c;
	int		size;

	size = msg->cursize;
	MSG_BeginReading( msg );
	MSG_ReadLong( msg );

	if ( net_profile->integer ) {
		NetProf_PrepProfiling( &clc.pProf );
		NetProf_AddPacket( (void *) clc.pProf, size, 0 );
	}

	s = MSG_ReadStringLine( msg );
	Cmd_TokenizeString2( s, 0 );

	c = Cmd_Argv( 0 );

	Com_DPrintf( "CL packet %s: %s\n", NET_AdrToString( from ), c );

	if ( !Q_stricmp( c, "challengeResponse" ) ) {
		if ( cls_state != CA_CONNECTING  ) {
			Com_Printf( "Unwanted challenge response received.  Ignored.\n" );
			return;
		}
		*(int *) clc_challenge = atoi( Cmd_Argv( 1 ) );
		if ( Cmd_Argc() > 2 ) {
			clc_onlyVisibleClients = atoi( Cmd_Argv( 2 ) );
		} else {
			clc_onlyVisibleClients = 0;
		}
		clc_connectPacketCount = 0;
		clc_connectTime = -99999;
		memcpy(clc_serverAddress, &from, sizeof(from));
		cls_state = CA_CHALLENGING;
		Com_DPrintf( "challenge: %d\n", *(int *) clc_challenge );
		return;
	}
	if ( !Q_stricmp( c, "connectResponse" ) ) {
		if ( cls_state >= CA_CONNECTED ) {
			Com_Printf( "Dup connect received.  Ignored.\n" );
			return;
		}
		if ( cls_state != CA_CHALLENGING ) {
			Com_Printf( "connectResponse packet while not connecting.  Ignored.\n" );
			return;
		}
		if ( !NET_CompareBaseAdr( from, *(netadr_t *) clc_serverAddress ) ) {
			Com_Printf( "connectResponse from a different address.  Ignored.\n" );
			Com_Printf( "%s should have been %s\n",
			            NET_AdrToString( from ),
			            NET_AdrToString( *(netadr_t *) clc_serverAddress ) );
			return;
		}
		/* CoD autoupdate arm, 0x00410BF9 */
		if ( autoupdateChecked
		     && NET_CompareAdr( *(netadr_t *) cls_autoupdateServer,
		                        *(netadr_t *) clc_serverAddress )
		     && cl_updateavailable->integer ) {
			autoupdateStarted = 1;
		}
		Netchan_Setup( &clc.netchan, NS_CLIENT, from,
		               (int) Cvar_VariableValue( "net_qport" ) );
		cls_state = CA_CONNECTED;
		clc_lastPacketTime = cls_realtime;
		clc_lastPacketSentTime = -9999;
		return;
	}
	if ( !Q_stricmp( c, "infoResponse" ) ) {
		CL_ServerInfoPacket( from, msg );
		return;
	}
	if ( !Q_stricmp( c, "statusResponse" ) ) {
		CL_ServerStatusResponse( from, msg );
		return;
	}
	if ( !Q_stricmp( c, "disconnect" ) ) {
		CL_DisconnectPacket( from );
		return;
	}
	if ( !Q_stricmp( c, "echo" ) ) {
		NET_OutOfBandPrint( NS_CLIENT, from, "%s", Cmd_Argv( 1 ) );
		return;
	}
	if ( !Q_stricmp( c, "keyAuthorize" ) ) {
		return;
	}
	if ( !Q_stricmp( c, "motd" ) ) {
		CL_MotdPacket( from );
		return;
	}
	if ( !Q_stricmp( c, "print" ) ) {
		s = MSG_ReadBigString( msg );
		strncpy( clc_serverMessage, s, 0xFF );
		clc_serverMessageTerminator[0] = 0;
		Com_Printf( "%s", s );
		return;
	}
	if ( !Q_stricmp( c, "error" ) ) {
		if ( cls_state
		     && NET_CompareBaseAdr( from, *(netadr_t *) clc_serverAddress ) ) {
			s = MSG_ReadBigString( msg );
			Com_Error( ERR_DROP, "%s",
			           SEH_LocalizeTextMessage( s, "server error", 0 ) );
		}
		return;
	}
	if ( !Q_stricmp( c, "updateResponse" ) ) {
		CL_UpdateInfoPacket( from );
		return;
	}
	{ int prefixResult = 0;
	{ int remaining = 18; const char *left = c, *right = "getserversResponse"; int leftByte, rightByte;
	 do {
	  leftByte = *left++; rightByte = *right++;
	  if (!remaining--) break;
	  if (leftByte != rightByte) { prefixResult = leftByte < rightByte ? -1 : 1; break; }
	 } while(leftByte);
	}
	if ( !prefixResult ) {
		CL_ServersResponsePacket( from, msg );
		return;
	}
	}
	if ( !Q_strncmp( c, "needcdkey", 9 ) ) {
		strncpy( clc_serverMessage, "EXE_AWAITINGCDKEYAUTH", 0xFF );
		clc_serverMessageTerminator[0] = 0;
		SEH_LocalizeTextMessage( "EXE_AWAITINGCDKEYAUTH", "need cd key message", 0 );   /* result discarded, 0x00410F4F */
		Com_Printf( "%s\n", clc_serverMessage );   /* retail prints the raw key, not the localized text */
		CL_RequestAuthorization();
		return;
	}
	if ( !Q_stricmp( c, "loadingnewmap" ) ) {
		if ( NET_CompareBaseAdr( from, *(netadr_t *) clc_serverAddress ) ) {
			const char *mapname = va( "%s", MSG_ReadStringLine( msg ) );
			const char *gametype = MSG_ReadStringLine( msg );
			CL_SetupForNewServerMap( mapname, gametype );
		}
		return;
	}

	Com_DPrintf( "Unknown connectionless packet command.\n" );
}

/* CL_PacketEvent (0x00410FF0) is defined further down this unit. */

/* ---- CL_CheckTimeout  0x00411130 ----  VERIFIED */
void CL_CheckTimeout( void ) {
	if ( ( !cl_paused->integer || !sv_paused->integer )
		 && cls_state >= CA_CONNECTED && cls_state != CA_CINEMATIC
		 && cls_state != CA_LOGO
		 && cls_realtime - clc_lastPacketTime > cl_timeout->value * 1000 ) {
		if ( ++cl_active > 5 ) {
			Com_Error( ERR_DROP, "EXE_ERR_SERVER_TIMEOUT" );
			return;
		}
	} else {
		cl_active = 0;
	}
}

/* ---- CL_CheckUserinfo  0x004111C0 ----  VERIFIED */
void CL_CheckUserinfo( void ) {
	if ( cls_state < CA_CHALLENGING ) {
		return;
	}
	if ( cl_paused->integer ) {
		return;
	}
	if ( cvar_modifiedFlags & CVAR_USERINFO ) {
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		CL_AddReliableCommand( va( "userinfo \"%s\"", Cvar_InfoString( CVAR_USERINFO ) ) );
	}
}

/* ---- CL_UpdateInGameState  0x00411220 ----  VERIFIED */
void CL_UpdateInGameState( void ) {
	if ( cls_state == CA_ACTIVE ) {
		if ( !cl_ingame->integer ) {
			Cvar_Set2( "cl_ingame", "1", qtrue );
		}
	} else {
		if ( cl_ingame->integer ) {
			Cvar_Set2( "cl_ingame", "0", qtrue );
		}
	}
}

/* ---- CL_SetRecommended_f  0x00411450 ----  [CONFIRMED] */
void __cdecl CL_SetRecommended_f()
{
  Com_SetRecommended(1);
}

/* ---- CL_RefPrintf  0x00411460 ----  [CONFIRMED] */
void CL_RefPrintf( int print_level, const char *fmt, ... )
{
	char    text[4096];
	va_list argptr;

	/* retail 0x00411492: unbounded vsprintf, no forced terminator */
	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	if ( print_level == 0 ) {
		Com_Printf( "%s", text );
	} else if ( print_level == 2 ) {
		Com_Printf( "^3%s", text );
	} else if ( print_level == 1 ) {
		Com_DPrintf( "^1%s", text );
	}
}

/* ---- CL_InitRenderer  0x00411540 ---- */
void CL_InitRenderer( void )
{
    /* retail 0x00411540 calls re_BeginRegistration unconditionally: no bound check, no diagnostics */
    re.BeginRegistration( &cls_glconfig );

    whiteShader   = re.RegisterShader( "white", 2 );
    cls_consoleShader = re.RegisterShader( "console", 2 );

    con_fieldWidthPixels  = cls_glconfig.vidWidth - 32;
    g_consoleField.widthInPixels = cls_glconfig.vidWidth - 32;

    *(int *)&g_consoleField.charWidth  = con_fieldCharWidth;
    *(int *)&g_consoleField.charHeight = con_fieldCharHeight;

    g_consoleField.fixedWidth = 1;

    StatMon_Reset();
}

/* ---- CL_StartHunkUsers  0x004115C0 ----  [CONFIRMED] */
void __cdecl CL_StartHunkUsers()
{
  if ( com_cl_running )
  {
    if ( com_cl_running->integer )
    {
      xmodel_enforceExist = Cvar_Get("cl_xmodelcheck", "0", 33)->integer;
      if ( !cls_rendererStarted )
      {
        cls_rendererStarted = 1;
        CL_InitRenderer();
        Sys_LoadingKeepAlive();
      }
      if ( !cls_soundStarted )
      {
        cls_soundStarted = 1;
        MSS_Init();
        Sys_LoadingKeepAlive();
      }
      if ( !cls_loadingPlaque )
      {
        cls_loadingPlaque = 1;
        CL_InitUI();
        Sys_LoadingKeepAlive();
      }
    }
  }
}

/* ---- CL_CheckAutoUpdate  0x00411650 ----  VERIFIED */
void CL_CheckAutoUpdate( void ) {
	int validServerNum = 0;
	int i = 0, rnd = 0;
	netadr_t temp;
	char *servername[MAX_AUTOUPDATE_SERVERS];

	if ( autoupdateChecked ) {
		return;
	}

	srand( Com_Milliseconds() );

	for ( i = 0; i < MAX_AUTOUPDATE_SERVERS; i++ ) {
		if ( NET_StringToAdr( cls.autoupdateServerNames[i], &temp ) ) {
			servername[validServerNum++] = cls.autoupdateServerNames[i];
		}
	}

	if ( !validServerNum ) {
		Com_DPrintf( "Couldn't resolve an AutoUpdate Server address.\n" );
	} else {
		rnd = rand() % validServerNum;

		Com_DPrintf( "Resolving AutoUpdate Server... " );
		if ( !NET_StringToAdr( servername[rnd], (netadr_t *)cls_autoupdateServer ) ) {
			Com_DPrintf( "\nCouldn't resolve first address, trying others... " );

			for ( i = 1; i < validServerNum; i++ ) {
				if ( NET_StringToAdr( servername[( i + rnd ) % validServerNum],
									  (netadr_t *)cls_autoupdateServer ) ) {
					Com_DPrintf( "\nAlternate server address resolved... " );
					break;
				}
			}

			if ( i == validServerNum ) {
				Com_DPrintf( "\nFailed to resolve any Auto-update servers.\n" );
				autoupdateChecked = qtrue;
				return;
			}
		}

		( (netadr_t *)cls_autoupdateServer )->port = BigShort( PORT_SERVER );
		Com_DPrintf( "%i.%i.%i.%i:%i\n",
					 ( (netadr_t *)cls_autoupdateServer )->ip[0],
					 ( (netadr_t *)cls_autoupdateServer )->ip[1],
					 ( (netadr_t *)cls_autoupdateServer )->ip[2],
					 ( (netadr_t *)cls_autoupdateServer )->ip[3],
					 BigShort( ( (netadr_t *)cls_autoupdateServer )->port ) );

		NET_OutOfBandPrint( NS_CLIENT, *(netadr_t *)cls_autoupdateServer,
							"getUpdateInfo \"%s\" \"%s\"\n", "1.1", "win-x86" );

		CL_RequestMotd();
	}

	autoupdateChecked = qtrue;
}

/* ---- CL_GetAutoUpdate  0x004117E0 ----  VERIFIED */
void CL_GetAutoUpdate( void ) {
	if ( !autoupdateChecked ) {
		return;
	}
	if ( !strlen( cl_updatefiles->string ) ) {
		return;
	}

	Sys_OpenURL( cl_updatefiles->string, qtrue );
}

/* ---- CL_ScaledMilliseconds  0x00411860 ----  VERIFIED */
int __cdecl CL_ScaledMilliseconds()
{
  return Sys_Milliseconds() * com_timescale->value;
}

/* ---- CG_GetGameModel  0x004118B0 ----  [CONFIRMED] */
void *__cdecl CG_GetGameModel( int handle )
{
	return (void *)VM_Call( cgvm, 9, (short)handle );
}

/* ---- CG_DObjCalcPose  0x004118D0 ----  VERIFIED */
void __cdecl CG_DObjCalcPose( int cent, int dobj, int partBits )
{
  VM_Call( cgvm, 10, cent, dobj, partBits );
}

/* ---- CL_GetFontInfo  0x004118F0 ----  VERIFIED */
int __cdecl CL_GetFontInfo( void *font, float scale )
{
  return VM_Call( uivm, 16, font, (int)( scale * 100.0f ) );
}

/* ---- CL_startSingleplayer_f  0x00411AE0 ----  VERIFIED */
void CL_startSingleplayer_f( void )
{
	Sys_StartProcess( "CoDSP.exe", 1 );
}

/* ---- CL_DrawLogo  0x00411B00 ----  [HIGH] */
void __cdecl CL_DrawLogo( void )
{
	/* re_DrawStretchPic is declared int(*)() by cl_refapi.h; call it through
	 * a float-typed signature so the width/height pass as floats instead of the
	 * LODWORD int-bit reinterpretation the decompile used (ABI-identical). */
	typedef void ( __cdecl *drawStretchPic_t )( float, float, float, float,
	                                            float, float, float, float, int );

	int    elapsed;
	float alpha;
	float  color[4];
	float  width, top, bottom;

	elapsed = cls_realtime - cls_logoStartTime;
	if (elapsed < cls_logoFadeInDuration) {
 alpha = (float)elapsed / (float)cls_logoFadeInDuration;
 } else if (elapsed > cls_logoTotalDuration - cls_logoFadeOutDuration) {
 alpha = (float)(cls_logoTotalDuration - elapsed) / (float)cls_logoFadeOutDuration;
 } else alpha = 1.0f;

	if ( alpha < 0.0f ) {
		alpha = 0.0f;
	} else if ( alpha > 1.0f ) {
		alpha = 1.0f;
	}

	color[0] = (float)alpha;
	color[1] = (float)alpha;
	color[2] = (float)alpha;
	color[3] = 1.0f;

	width  = (float)cls_glconfig_vidWidth;
	top    = (float)( ( (float)cls_glconfig_vidHeight + (float)cls_glconfig_vidHeight ) * 0.33333334f );
	bottom = (float)( (float)cls_glconfig_vidHeight - top );

	re_SetColor( color );
	( (drawStretchPic_t)re_DrawStretchPic )( 0.0f, 0.0f, width, top,
	                                         0.0f, 0.0f, 1.0f, 1.0f,
	                                         cls_logoShaderTop );
	( (drawStretchPic_t)re_DrawStretchPic )( 0.0f, top, width, bottom,
	                                         0.0f, 0.0f, 1.0f, 1.0f,
	                                         cls_logoShaderBottom );
	re_SetColor( NULL );

	if ( elapsed > cls_logoTotalDuration ) {
		cls_state = CA_DISCONNECTED;
	}
}

/* ---- CL_StopLogo  0x00411C40 ----  [CONFIRMED] */
void __cdecl CL_StopLogo()
{
  cls_state = CA_DISCONNECTED;
}

/* ---- CL_PlayLogo_f  0x00411C50 ----  [CONFIRMED] */
void __cdecl CL_PlayLogo_f( void )
{
 char *image;
	if ( cmd_argc != 5 ) {
		Com_Printf( "USAGE: logo <image name> <fadein seconds> <full duration seconds> <fadeout seconds>\n" );
		return;
	}

	Com_DPrintf( "CL_PlayLogo_f\n" );

	if ( cls_state == CA_CINEMATIC ) {
		if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
			CIN_StopCinematic( CL_handle );
			MSS_StopSounds( 0 );
			CL_handle = -1;
		}
	} else if ( cls_state != CA_LOGO && cls_state != CA_DISCONNECTED ) {
		return;
	}

	cls_state = CA_LOGO;
	if ( uivm ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
	}
	MSS_StopSounds( 0 );
	MSS_FadeAllSounds( 1.0f, 0 );
 image = Cmd_Argv( 1 );   /* 1.0f, stored as an int bit pattern */

	/* 0x00411D27-0x00411DF7: atof * 1000.0f, fstp to a float, then the inlined
	 * FastRound body (fld float / fadd qword 2^-30 / fistp) -- round to nearest,
	 * not the truncating __ftol2 of an (int) cast. */
	cls_logoFadeInDuration  = FastRound( (float)( (rfloat)atof( Cmd_Argv( 2 ) ) * 1000.0f ) );
	cls_logoTotalDuration   = FastRound( (float)( (rfloat)atof( Cmd_Argv( 3 ) ) * 1000.0f ) );
	cls_logoFadeOutDuration = FastRound( (float)( (rfloat)atof( Cmd_Argv( 4 ) ) * 1000.0f ) );
	cls_logoTotalDuration  += cls_logoFadeOutDuration + cls_logoFadeInDuration;

	cls_logoShaderTop    = re_RegisterShaderNoMip( va( "%s1", image ), 2 );
	cls_logoShaderBottom = re_RegisterShaderNoMip( va( "%s2", image ), 2 );
	cls_logoStartTime    = cls_realtime + 100;
}

/* CL_Init (0x00411E60) is defined further down this unit. */

/* ---- CL_SetServerInfo  0x004129F0 ----  VERIFIED */
void CL_SetServerInfo( serverInfo_t *server, const char *info, int ping ) {
	if ( server ) {
		if ( info ) {
			server->clients = atoi( Info_ValueForKey( info, "clients" ) );
			Q_strncpyz( server->hostName, Info_ValueForKey( info, "hostname" ), MAX_NAME_LENGTH );
			Q_strncpyz( server->mapName, Info_ValueForKey( info, "mapname" ), MAX_NAME_LENGTH );
			server->maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
			Q_strncpyz( server->game, Info_ValueForKey( info, "game" ), MAX_NAME_LENGTH );
			Q_strncpyz( server->gameType, Info_ValueForKey( info, "gametype" ), MAX_NAME_LENGTH );
			server->netType = atoi( Info_ValueForKey( info, "nettype" ) );
			server->minPing = atoi( Info_ValueForKey( info, "minping" ) );
			server->maxPing = atoi( Info_ValueForKey( info, "maxping" ) );
			server->allowAnonymous = atoi( Info_ValueForKey( info, "sv_allowAnonymous" ) );
			server->password = atoi( Info_ValueForKey( info, "pswrd" ) );
		}
		server->ping = ping;
	}
}

/* ---- CL_SetServerInfoByAddress  0x00412B30 ----  VERIFIED */
void CL_SetServerInfoByAddress( netadr_t from, const char *info, int ping ) {
	int i;

	for ( i = 0; i < MAX_OTHER_SERVERS; i++ ) {
		if ( NET_CompareAdr( from, cls_localServers[i].adr ) ) {
			CL_SetServerInfo( &cls_localServers[i], info, ping );
		}
	}

	for ( i = 0; i < MAX_GLOBAL_SERVERS; i++ ) {
		if ( NET_CompareAdr( from, cls_globalServers[i].adr ) ) {
			CL_SetServerInfo( &cls_globalServers[i], info, ping );
		}
	}

	for ( i = 0; i < MAX_OTHER_SERVERS; i++ ) {
		if ( NET_CompareAdr( from, cls_favoriteServers[i].adr ) ) {
			CL_SetServerInfo( &cls_favoriteServers[i], info, ping );
		}
	}
}

/* ---- CL_ServerInfoPacket  0x00412C30 ----  VERIFIED */
void CL_ServerInfoPacket( netadr_t from, msg_t *msg ) {
	int i, type;
	char info[MAX_INFO_STRING];
	char *infoString;
	int prot;

	infoString = MSG_ReadString( msg );

	prot = atoi( Info_ValueForKey( infoString, "protocol" ) );
	if ( prot != PROTOCOL_VERSION ) {
		Com_DPrintf( "Different protocol info packet: %s\n", infoString );
		return;
	}

	for ( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		if ( cl_pinglist[i].adr.port && !cl_pinglist[i].time
			 && NET_CompareAdr( from, cl_pinglist[i].adr ) ) {
			cl_pinglist[i].time = cls_realtime - cl_pinglist[i].start + 1;
			Com_DPrintf( "ping time %dms from %s\n", cl_pinglist[i].time,
						 NET_AdrToString( from ) );

			Q_strncpyz( cl_pinglist[i].info, infoString,
						sizeof( cl_pinglist[i].info ) );

			switch ( from.type ) {
			case NA_BROADCAST:
			case NA_IP:
				type = 1;
				break;

			case NA_IPX:
			case NA_BROADCAST_IPX:
				type = 2;
				break;

			default:
				type = 0;
				break;
			}
			Info_SetValueForKey( cl_pinglist[i].info, "nettype", va( "%d", type ) );
			CL_SetServerInfoByAddress( from, infoString, cl_pinglist[i].time );

			return;
		}
	}

	if ( cls_pingUpdateSource != AS_LOCAL ) {
		return;
	}

	for ( i = 0; i < MAX_OTHER_SERVERS; i++ ) {
		if ( cls_localServers[i].adr.port == 0 ) {
			break;
		}

		if ( NET_CompareAdr( from, cls_localServers[i].adr ) ) {
			return;
		}
	}

	if ( i == MAX_OTHER_SERVERS ) {
		Com_DPrintf( "MAX_OTHER_SERVERS hit, dropping infoResponse\n" );
		return;
	}

	cls_numlocalservers = i + 1;
	cls_localServers[i].adr = from;
	cls_localServers[i].clients = 0;
	cls_localServers[i].hostName[0] = '\0';
	cls_localServers[i].mapName[0] = '\0';
	cls_localServers[i].maxClients = 0;
	cls_localServers[i].maxPing = 0;
	cls_localServers[i].minPing = 0;
	cls_localServers[i].ping = -1;
	cls_localServers[i].game[0] = '\0';
	cls_localServers[i].gameType[0] = '\0';
	cls_localServers[i].netType = from.type;
	cls_localServers[i].allowAnonymous = 0;

	Q_strncpyz( info, MSG_ReadString( msg ), MAX_INFO_STRING );
	if ( strlen( info ) ) {
		if ( info[strlen( info ) - 1] != '\n' ) {
			strncat( info, "\n", sizeof( info ) );
		}
		Com_Printf( "%s: %s", NET_AdrToString( from ), info );
	}
}

/* ---- CL_UpdateInfoPacket  0x00412F70 ----  VERIFIED */
void CL_UpdateInfoPacket( netadr_t from ) {

	if ( ( (netadr_t *)cls_autoupdateServer )->type == NA_BAD ) {
		Com_DPrintf( "CL_UpdateInfoPacket:  Auto-Updater has bad address\n" );
		return;
	}

	Com_DPrintf( "Auto-Updater resolved to %i.%i.%i.%i:%i\n",
				 ( (netadr_t *)cls_autoupdateServer )->ip[0],
				 ( (netadr_t *)cls_autoupdateServer )->ip[1],
				 ( (netadr_t *)cls_autoupdateServer )->ip[2],
				 ( (netadr_t *)cls_autoupdateServer )->ip[3],
				 BigShort( ( (netadr_t *)cls_autoupdateServer )->port ) );

	if ( !NET_CompareAdr( from, *(netadr_t *)cls_autoupdateServer ) ) {
		Com_DPrintf( "CL_UpdateInfoPacket:  Received packet from %i.%i.%i.%i:%i\n",
					 from.ip[0], from.ip[1], from.ip[2], from.ip[3],
					 BigShort( from.port ) );
		return;
	}

	Cvar_Set2( "cl_updateavailable", Cmd_Argv( 1 ), qtrue );

	if ( cl_updateavailable->string
		 && !Q_stricmp( cl_updateavailable->string, "1" ) ) {
		Cvar_Set2( "cl_updatefiles", Cmd_Argv( 2 ), qtrue );
		Cvar_Set2( "cl_updateversion", Cmd_Argv( 3 ), qtrue );
		Cvar_Set2( "cl_updateoldversion", "1.1", qtrue );
	}
}

/* ---- CL_GetServerStatus  0x004130E0 ----  [HIGH] */
serverStatus_t *__cdecl CL_GetServerStatus( netadr_t from )
{
	int i, oldest, oldestTime;

	/* an existing request for this address */
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			return &cl_serverStatusList[i];
		}
	}

	/* a slot whose reply has already been retrieved */
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( cl_serverStatusList[i].retrieved ) {
			return &cl_serverStatusList[i];
		}
	}

	/* otherwise recycle the oldest request */
	oldest = -1;
	oldestTime = 0;
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( oldest == -1 || cl_serverStatusList[i].startTime < oldestTime ) {
			oldest = i;
			oldestTime = cl_serverStatusList[i].startTime;
		}
	}

	if ( oldest != -1 ) {
		return &cl_serverStatusList[oldest];
	}

	serverStatusCount++;
	return &cl_serverStatusList[serverStatusCount & ( MAX_SERVERSTATUSREQUESTS - 1 )];
}

/* ---- CL_ServerStatus  0x00413260 ----  VERIFIED */
qboolean CL_ServerStatus( char *serverAddress, char *serverStatusString, int maxLen ) {
	int i;
	netadr_t to;
	serverStatus_t *serverStatus;

	if ( !serverAddress ) {
		for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
			cl_serverStatusList[i].address.port = 0;
			cl_serverStatusList[i].retrieved = qtrue;
		}
		return qfalse;
	}
	if ( !NET_StringToAdr( serverAddress, &to ) ) {
		return qfalse;
	}
	serverStatus = CL_GetServerStatus( to );
	if ( !serverStatusString ) {
		serverStatus->retrieved = qtrue;
		return qfalse;
	}

	if ( NET_CompareAdr( to, serverStatus->address ) ) {
		if ( !serverStatus->pending ) {
			Q_strncpyz( serverStatusString, serverStatus->string, maxLen );
			serverStatus->retrieved = qtrue;
			serverStatus->startTime = 0;
			return qtrue;
		}
		else if ( serverStatus->startTime
				  < Sys_Milliseconds() - cl_serverStatusResendTime->integer ) {
			serverStatus->print = qfalse;
			serverStatus->pending = qtrue;
			serverStatus->retrieved = qfalse;
			serverStatus->time = 0;
			serverStatus->startTime = Sys_Milliseconds();
			NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
			return qfalse;
		}
	}
	else if ( serverStatus->retrieved ) {
		serverStatus->address = to;
		serverStatus->print = qfalse;
		serverStatus->pending = qtrue;
		serverStatus->retrieved = qfalse;
		serverStatus->startTime = Sys_Milliseconds();
		serverStatus->time = 0;
		NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );
		return qfalse;
	}
	return qfalse;
}

/* ---- CL_ServerStatusResponse  0x00413430 ----  VERIFIED */
void __cdecl CL_ServerStatusResponse( netadr_t from, void *msg )
{
	serverStatus_t *serverStatus;
	char *s;
	char info[MAX_INFO_STRING];
	int  i, len;
	int  playerNum;

	serverStatus = NULL;
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( NET_CompareAdr( from, cl_serverStatusList[i].address ) ) {
			serverStatus = &cl_serverStatusList[i];
			break;
		}
	}
	if ( !serverStatus ) {
		return;
	}

	s = MSG_ReadStringLine( (msg_t *)msg );
	Com_sprintf( serverStatus->string, sizeof( serverStatus->string ), "%s", s );

	/* the server cvars, printed as key/value pairs (two columns per line) */
	if ( serverStatus->print ) {
		Com_Printf( "Server settings:\n" );
		while ( *s ) {
			int col;
			for ( col = 0; col < 2 && *s; col++ ) {
				if ( *s == '\\' ) {
					s++;
				}
				len = 0;
				while ( *s ) {
					info[len++] = *s;
					if ( len >= MAX_INFO_STRING - 1 ) {
						break;
					}
					s++;
					if ( *s == '\\' ) {
						break;
					}
				}
				/* retail 0x00413520 terminates with no bound check; pointer store avoids the modern /GS range-check fast-fail */
				*( info + len ) = '\0';
				if ( col ) {
					Com_Printf( "%s\n", info );
				} else {
					Com_Printf( "%-24s", info );
				}
			}
		}
	}

	len = strlen( serverStatus->string );
	Com_sprintf( &serverStatus->string[len], sizeof( serverStatus->string ) - len, "\\" );

	if ( serverStatus->print ) {
		Com_Printf( "\nPlayers:\n" );
		Com_Printf( "num: score: ping: name:\n" );
	}

	playerNum = 0;
	s = MSG_ReadStringLine( (msg_t *)msg );
	while ( *s ) {
		len = strlen( serverStatus->string );
		Com_sprintf( &serverStatus->string[len], sizeof( serverStatus->string ) - len, "\\%s", s );
		if ( serverStatus->print ) {
			int score, ping;
			char *sp;
			const char *name;

			score = ping = 0;
			sscanf( s, "%d %d", &score, &ping );
			sp = strchr( s, ' ' );
			if ( sp && ( sp = strchr( sp + 1, ' ' ) ) != NULL ) {
				name = sp + 1;
			} else {
				name = "unknown";
			}
			Com_Printf( "%-2d   %-3d    %-3d   %s\n", playerNum, score, ping, name );
		}
		s = MSG_ReadStringLine( (msg_t *)msg );
		playerNum++;
	}

	len = strlen( serverStatus->string );
	Com_sprintf( &serverStatus->string[len], sizeof( serverStatus->string ) - len, "\\" );

	if ( !sys_timeBaseInit ) {
		sys_timeBase = timeGetTime();
		sys_timeBaseInit = 1;
	}
	serverStatus->time = timeGetTime() - sys_timeBase;
	serverStatus->address = from;
	serverStatus->pending = qfalse;
	if ( serverStatus->print ) {
		serverStatus->retrieved = qtrue;
	}
}

/* ---- CL_LocalServers_f  0x00413710 ----  VERIFIED */
void CL_LocalServers_f( void ) {
	char *message;
	int i, j, len;
	struct { netadr_t to, packet; } addresses;

	Com_Printf( "Scanning for servers on the local network...\n" );

	cls_numlocalservers = 0;
	cls_pingUpdateSource = AS_LOCAL;

	for ( i = 0; i < MAX_OTHER_SERVERS; i++ ) {
		qboolean b = cls_localServers[i].visible;
		Com_Memset( &cls_localServers[i], 0, sizeof( cls_localServers[i] ) );
		cls_localServers[i].visible = b;
	}
	Com_Memset( &addresses.to, 0, sizeof( addresses.to ) );

	message = "\377\377\377\377getinfo xxx";

	for ( i = 0; i < 2; i++ ) {
		for ( j = 0; j < NUM_SERVER_PORTS; j++ ) {
			addresses.to.port = BigShort( (short)( PORT_SERVER + j ) );
			addresses.to.type = NA_BROADCAST;

			len = strlen( message );
			memcpy(&addresses.packet, &addresses.to, sizeof(addresses.packet));
			NetProf_PrepProfiling( &clc.pProf );
			NET_SendPacket( NS_CLIENT, len, message, addresses.packet );
			if ( net_profile->integer ) {
				NetProf_PrepProfiling( &clc.pProf );
				NetProf_AddPacket( &clc.pProf->send, len, qfalse );
			}
		}
	}
}

/* ---- CL_GlobalServers_f  0x00413890 ----  [CONFIRMED] */
void __cdecl CL_GlobalServers_f( void )
{
	netadr_t to;
	cvar_t   *fsRestrict;
	char     command[1024];
	char     *cmd;
	int      i;
	int      count;

	if ( cmd_argc < 3 ) {
		Com_Printf( "usage: globalservers <master# 0-1> <protocol> [keywords]\n" );
		return;
	}

	Com_Printf( "Requesting servers from the master...\n" );

	NET_StringToAdr( "codmaster.activision.com", &to );
	cls_numglobalservers = -1;
	cls_pingUpdateSource = AS_GLOBAL;
	to.type = NA_IP;
	to.port = BigShort( 20510 );

	sprintf( command, "getservers %s", Cmd_Argv( 2 ) );
	cmd = command + strlen( command );

	count = Cmd_Argc();
	for ( i = 3; i < count; i++ ) {
		cmd += sprintf( cmd, " %s", Cmd_Argv( i ) );
	}

	fsRestrict = Cvar_FindVar( "fs_restrict" );
	if ( fsRestrict && fsRestrict->value != 0.0f ) {
		sprintf( cmd, " demo" );
	}

	NET_OutOfBandPrint( NS_SERVER, to, command );
}

/* ---- CL_GetPing  0x004139F0 ----  VERIFIED */
void CL_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	const char *str;
	int time;
	int maxPing;

	if ( !cl_pinglist[n].adr.port ) {
		buf[0] = '\0';
		*pingtime = 0;
		return;
	}

	str = NET_AdrToString( cl_pinglist[n].adr );
	Q_strncpyz( buf, str, buflen );

	time = cl_pinglist[n].time;
	if ( !time ) {
		time = cls_realtime - cl_pinglist[n].start;
		maxPing = Cvar_VariableIntegerValue( "cl_maxPing" );
		if ( maxPing < 100 ) {
			maxPing = 100;
		}
		if ( time < maxPing ) {
			time = 0;
		}
	}

	CL_SetServerInfoByAddress( cl_pinglist[n].adr, cl_pinglist[n].info,
							   cl_pinglist[n].time );

	*pingtime = time;
}

/* ---- CL_UpdateServerInfo  0x00413AC0 ----  VERIFIED */
void CL_UpdateServerInfo( int n ) {
	if ( !cl_pinglist[n].adr.port ) {
		return;
	}

	CL_SetServerInfoByAddress( cl_pinglist[n].adr, cl_pinglist[n].info,
							   cl_pinglist[n].time );
}

/* ---- CL_GetPingInfo  0x00413B00 ----  VERIFIED */
void CL_GetPingInfo( int n, char *buf, int buflen ) {
	if ( !cl_pinglist[n].adr.port ) {
		if ( buflen ) {
			buf[0] = '\0';
		}
		return;
	}

	Q_strncpyz( buf, cl_pinglist[n].info, buflen );
}

/* ---- CL_ClearPing  0x00413B40 ----  VERIFIED */
void CL_ClearPing( int n ) {
	if ( n < 0 || n >= MAX_PINGREQUESTS ) {
		return;
	}

	cl_pinglist[n].adr.port = 0;
}

/* ---- CL_GetPingQueueCount  0x00413B60 ---- */
int __cdecl CL_GetPingQueueCount()
{
  int i;
  int count;
  ping_t *pingptr;

  count = 0;
  pingptr = cl_pinglist;

  for ( i = 0; i < MAX_PINGREQUESTS; i++, pingptr++ )
  {
    if ( pingptr->adr.port )
      count++;
  }
  return count;
}

/* ---- CL_GetFreePing  0x00413BD0 ----  [HIGH] */
ping_t *__cdecl CL_GetFreePing( void )
{
	ping_t *ping;
	ping_t *oldest;
	int     i, time, oldestTime;

	ping = cl_pinglist;
	for ( i = 0; i < MAX_PINGREQUESTS; i++, ping++ ) {
		/* an unused slot, or one whose reply has arrived, or that has waited too long */
		if ( ping->adr.port ) {
			if ( !ping->time ) {
				if ( cls_realtime - ping->start < 500 ) {
					continue;
				}
			} else if ( ping->time < 500 ) {
				continue;
			}
		}

		ping->adr.port = 0;
		return ping;
	}

	/* all slots busy: recycle the one that has been outstanding longest */
	ping = cl_pinglist;
	oldest = cl_pinglist;
	oldestTime = (int)0x80000000;
	for ( i = 0; i < MAX_PINGREQUESTS; i++, ping++ ) {
		time = cls_realtime - ping->start;
		if ( time > oldestTime ) {
			oldestTime = time;
			oldest = ping;
		}
	}

	return oldest;
}

/* ---- CL_Ping_f  0x00413D00 ----  [CONFIRMED] */
void __cdecl CL_Ping_f( void )
{
	netadr_t adr;
	ping_t   *ping;

	if ( cmd_argc != 2 ) {
		Com_Printf( "usage: ping [server]\n" );
		return;
	}

	memset( &adr, 0, sizeof( adr ) );
	if ( !NET_StringToAdr( cmd_argv[1], &adr ) ) {
		return;
	}

	ping = CL_GetFreePing();
	memcpy( &ping->adr, &adr, sizeof( netadr_t ) );
	ping->start = cls_realtime;
	ping->time = 0;

	CL_SetServerInfoByAddress( ping->adr, NULL, 0 );
	NET_OutOfBandPrint( NS_CLIENT, adr, "getinfo xxx" );
}

/* ---- CL_UpdateVisiblePings_f  0x00413DE0 ----  VERIFIED */
qboolean CL_UpdateVisiblePings_f( int source ) {
	int slots, i;
	char buff[MAX_STRING_CHARS];
	int pingTime;
	int max;
	qboolean status = qfalse;

	if ( source < AS_LOCAL || source > AS_FAVORITES ) {
		return qfalse;
	}

	cls_pingUpdateSource = source;

	slots = CL_GetPingQueueCount();
	if ( slots < MAX_PINGREQUESTS ) {
		serverInfo_t *server = NULL;

		max = ( source == AS_GLOBAL ) ? MAX_GLOBAL_SERVERS : MAX_OTHER_SERVERS;
		switch ( source ) {
		case AS_LOCAL:
			server = &cls_localServers[0];
			max = cls_numlocalservers;
			break;
		case AS_GLOBAL:
			server = &cls_globalServers[0];
			max = cls_numglobalservers;
			break;
		case AS_FAVORITES:
			server = &cls_favoriteServers[0];
			max = cls_numfavoriteservers;
			break;
		}
		for ( i = 0; i < max; i++ ) {
			if ( server[i].visible ) {
				if ( server[i].ping == -1 ) {
					int j;

					if ( slots >= MAX_PINGREQUESTS ) {
						break;
					}
					for ( j = 0; j < MAX_PINGREQUESTS; j++ ) {
						if ( !cl_pinglist[j].adr.port ) {
							continue;
						}
						if ( NET_CompareAdr( cl_pinglist[j].adr, server[i].adr ) ) {
							break;
						}
					}
					if ( j >= MAX_PINGREQUESTS ) {
						status = qtrue;
						for ( j = 0; j < MAX_PINGREQUESTS; j++ ) {
							if ( !cl_pinglist[j].adr.port ) {
								break;
							}
						}
						memcpy( &cl_pinglist[j].adr, &server[i].adr,
									sizeof( netadr_t ) );
						cl_pinglist[j].start = cls_realtime;
						cl_pinglist[j].time = 0;
						NET_OutOfBandPrint( NS_CLIENT, cl_pinglist[j].adr,
											"getinfo xxx" );
						slots++;
					}
				}
				else if ( server[i].ping == 0 ) {
					if ( source == AS_GLOBAL ) {
						if ( cls_numGlobalServerAddresses > 0 ) {
							cls_numGlobalServerAddresses--;
							CL_InitServerInfo( &server[i],
								&cls_globalServerAddresses[cls_numGlobalServerAddresses] );
						}
					}
				}
			}
		}
	}

	if ( slots ) {
		status = qtrue;
	}
	for ( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		if ( !cl_pinglist[i].adr.port ) {
			continue;
		}
		CL_GetPing( i, buff, MAX_STRING_CHARS, &pingTime );
		if ( pingTime != 0 ) {
			CL_ClearPing( i );
			status = qtrue;
		}
	}

	return status;
}

/* ---- CL_ServerStatus_f  0x004140F0 ----  [CONFIRMED] */
void __cdecl CL_ServerStatus_f( void )
{
	netadr_t to;
	const char *server;
	serverStatus_t *serverStatus;

	Com_Memset( &to, 0, sizeof( to ) );

if (cmd_argc != 2) { if (cls_state != CA_ACTIVE || clc_demoplaying) { Com_Printf( "Not connected to a server.\n" ); Com_Printf( "Usage: serverstatus [server]\n" ); return; } server=cls_servername; } else server=cmd_argv[1];

	if ( !NET_StringToAdr( server, &to ) ) {
		return;
	}

	NET_OutOfBandPrint( NS_CLIENT, to, "getstatus" );

	serverStatus = CL_GetServerStatus( to );
	serverStatus->address = to;
	serverStatus->print = qtrue;
	serverStatus->pending = qtrue;
}

/* ---- CL_ShowIP_f  0x004141E0 ----  [CONFIRMED] */
void __cdecl CL_ShowIP_f()
{
  Sys_ShowIP();
}

/* ---- CL_GetServerIPAddress  0x004141F0 ----  VERIFIED */
char *CL_GetServerIPAddress( void ) {
	char *s = (char *)cl_serverIPAddress;

	if ( cls_state < CA_CONNECTED ) {
		memset( s, 0, 128 );
		return s;
	}

	Com_sprintf( s, 128, "%i.%i.%i.%i:%i",
				 ( (netadr_t *)clc_serverAddress )->ip[0],
				 ( (netadr_t *)clc_serverAddress )->ip[1],
				 ( (netadr_t *)clc_serverAddress )->ip[2],
				 ( (netadr_t *)clc_serverAddress )->ip[3],
				 BigShort( ( (netadr_t *)clc_serverAddress )->port ) );
	return s;
}

/* ---- CL_CDKeyValidate  0x00414260 ----  [HIGH] */
qboolean __cdecl CL_CDKeyValidate( const char *key, const char *checksum )
{
	unsigned int crc;
	int i, bit;
	char chs[8];

	crc = 0;
	for ( i = 0; i < 16; i++ ) {
		crc ^= key[i];
		for ( bit = 0; bit < 8; bit++ ) {
			if ( crc & 1 ) {
				crc = ( crc ^ 0x14002 ) >> 1;
			} else {
				crc >>= 1;
			}
		}
	}

	/* retail: CRT sprintf into an 8-byte buffer; crc stays below 0x1000000, so at most 6 digits */
	sprintf( chs, "%04x", crc );

	if ( checksum && !Q_stricmp( chs, checksum ) ) {
		return qtrue;
	}

	if ( !checksum ) {
		return qtrue;
	}

	return qfalse;
}

/* ---- CL_SetupForNewServerMap  0x004142F0 ----  VERIFIED */
void CL_SetupForNewServerMap( const char *mapname, const char *gametype )
{
	Com_Printf( "Server changing map %s, gametype %s\n", mapname, gametype );
	Cvar_Set2( "cl_serverloadmap", mapname, qtrue );
	Cvar_Set2( "cl_serverloadgametype", gametype, qtrue );
	Cvar_Set2( "cl_serverloadwaiting", "0", qtrue );
}

/* ---- CL_AddDebugString  0x00414340 ----  VERIFIED */
void CL_AddDebugString( const float *origin, const float *color, float scale,
                        const char *text, int fromServer )
{
	char *rec;
	float *str;

	if ( !cls_rendererStarted ) {
		return;
	}

	cls_debugStringCapacity = 256;
	if ( cls_debugStringCount + 1 > 256 ) {
		return;
	}

	if ( !cls_debugStrings ) {
		cls_debugStrings = Z_MallocInternal( 0x8000 );
		cls_debugStringFromServer = Z_MallocInternal( cls_debugStringCapacity );
		cls_debugStringCount = 0;
	}

	rec = (char *)cls_debugStrings + 128 * cls_debugStringCount;
	str = (float *)rec;

	str[0] = origin[0];
	str[1] = origin[1];
	str[2] = origin[2];

	str[3] = color[0];
	str[4] = color[1];
	str[5] = color[2];
	str[6] = color[3];

	str[7] = scale;

	strncpy( rec + 0x20, text, 0x5F );
	rec[0x7F] = 0;

	((unsigned char *)cls_debugStringFromServer)[cls_debugStringCount] = (unsigned char)fromServer;
	++cls_debugStringCount;
}

/* ---- CL_AddDebugLine  0x00414410 ----  VERIFIED */
void CL_AddDebugLine( const float *start, const float *end, const float *color,
                      int depthTest, int duration, int fromServer )
{
	int index;
	struct { float start[3]; float end[3]; float color[4]; int depthTest; } *line;

	if ( !cls_rendererStarted ) {
		return;
	}

	index = cls_debugLineCount;
	cls_debugLineCapacity = 4096;
	if ( index + 1 > 4096 ) {
		return;
	}

	if ( !cls_debugLines ) {
		cls_debugLines = Z_MallocInternal( 0x2C000 );
		cls_debugLineFromServer = Z_MallocInternal( cls_debugLineCapacity );
		cls_debugLineDurations = Z_MallocInternal( 4 * cls_debugLineCapacity );
		index = 0;
		cls_debugLineCount = 0;
	}

	line = (void*)((char*)cls_debugLines + 44 * index);

	VectorCopy( start, line->start );
	VectorCopy( end, line->end );

	line->color[0] = color[0];
	line->color[1] = color[1];
	line->color[2] = color[2];
	line->color[3] = color[3];

	line->depthTest = depthTest;

	((unsigned char *)cls_debugLineFromServer)[cls_debugLineCount] = (unsigned char)fromServer;
	((int *)cls_debugLineDurations)[cls_debugLineCount] = duration;
	++cls_debugLineCount;
}

/* ---- CL_FlushDebugData  0x00414500 ----  VERIFIED */
void CL_FlushDebugData( int fromServer )
{
	int i;

	if ( !cls_rendererStarted ) {
		return;
	}

	if ( cls_debugStrings ) {
		for ( i = 0; i < cls_debugStringCount; ) {
			if ( ( (unsigned char *)cls_debugStringFromServer )[i] == fromServer ) {
				--cls_debugStringCount;
				( (unsigned char *)cls_debugStringFromServer )[i] =
					( (unsigned char *)cls_debugStringFromServer )[cls_debugStringCount];
				memcpy( (char *)cls_debugStrings + 128 * i,
						(char *)cls_debugStrings + 128 * cls_debugStringCount, 128 );
			} else {
				++i;
			}
		}
		re_LocateDebugStrings( cls_debugStrings, cls_debugStringCount );
	}

	if ( cls_debugLines ) {
		for ( i = 0; i < cls_debugLineCount; ) {
			if ( ( (unsigned char *)cls_debugLineFromServer )[i] == fromServer
				&& --( (int *)cls_debugLineDurations )[i] <= 0 ) {
				--cls_debugLineCount;
				( (unsigned char *)cls_debugLineFromServer )[i] =
					( (unsigned char *)cls_debugLineFromServer )[cls_debugLineCount];
				( (int *)cls_debugLineDurations )[i] =
					( (int *)cls_debugLineDurations )[cls_debugLineCount];
				memcpy( (char *)cls_debugLines + 44 * i,
						(char *)cls_debugLines + 44 * cls_debugLineCount, 44 );
			} else {
				++i;
			}
		}
		re_LocateDebugLines( cls_debugLines, cls_debugLineCount );
	}
}

/* ---- CL_UpdateDebugData  0x00414640 ----  [CONFIRMED] */
void __cdecl CL_UpdateDebugData()
{
  if ( cls_rendererStarted )
  {
    if ( cls_debugStrings )
      re_LocateDebugStrings(cls_debugStrings, cls_debugStringCount);
    if ( cls_debugLines )
      re_LocateDebugLines(cls_debugLines, cls_debugLineCount);
  }
}

/* ---- CL_ShutdownDebugData  0x00414680 ----  [CONFIRMED] */
/* Takes no arguments: the `push edi` before its only call (0x0041281F in
 * CL_Shutdown) is a register save, not a parameter. */
void __cdecl CL_ShutdownDebugData( void )
{
  if ( cls_debugLines )
  {
    free(cls_debugLines);
    cls_debugLines = 0;
  }
  if ( cls_debugLineFromServer )
  {
    free(cls_debugLineFromServer);
    cls_debugLineFromServer = 0;
  }
  if ( cls_debugLineDurations )
  {
    free(cls_debugLineDurations);
    cls_debugLineDurations = 0;
  }
  if ( cls_debugStrings )
  {
    free(cls_debugStrings);
    cls_debugStrings = 0;
  }
  if ( cls_debugStringFromServer )
  {
    free(cls_debugStringFromServer);
    cls_debugStringFromServer = 0;
  }
  memset( &cls.debug, 0, sizeof( cls.debug ) );
}

/* ---- CL_ForwardCommandToServer  0x0040F0C0 ----  [CONFIRMED] */
void CL_ForwardCommandToServer( const char *string ) {
	const char *cmd;

	/* Cmd_Argv( 0 ) inlined by hand in retail's shape (the arg >= argc test first) */
	if ( 0 >= (unsigned int)cmd_argc ) {
		cmd = "";
	} else {
		cmd = cmd_argv[0];
	}

	if ( cmd[0] == '-' ) {
		return;
	}

	if ( clc_demoplaying || cls_state < CA_CONNECTED || cmd[0] == '+' ) {
		Com_Printf( "Unknown command \"%s\"\n", cmd );
		return;
	}

	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( string );
	} else {
		CL_AddReliableCommand( cmd );
	}
}


extern qboolean    NET_CompareAdr( netadr_t a, netadr_t b );
extern qboolean    Netchan_Process( netchan_t *chan, msg_t *msg );
extern int         MSG_ReadLong( msg_t *msg );
extern void        CL_ConnectionlessPacket( netadr_t from, msg_t *msg );
extern int         CL_Netchan_Decode();
extern void        CL_WriteDemoMessage( msg_t *msg, int headerBytes );

/* ---- CL_PacketEvent  0x00410FF0 ----  VERIFIED */
void CL_PacketEvent( netadr_t from, msg_t *msg ) {
	int headerBytes;

	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		CL_ConnectionlessPacket( from, msg );
		return;
	}

	if ( cls_state < CA_CONNECTED ) {
		return;
	}

	if ( msg->cursize < 4 ) {
		Com_Printf( "%s: Runt packet\n", NET_AdrToString( from ) );
		return;
	}

	if ( !NET_CompareAdr( from, clc.netchan.remoteAddress ) ) {
		Com_DPrintf( "%s:sequenced packet without connection\n",
		             NET_AdrToString( from ) );
		return;
	}

	clc_lastPacketTime = cls_realtime;

	if ( !Netchan_Process( &clc.netchan, msg ) ) {
		return;
	}

	headerBytes = msg->readcount;

	clc_serverMessageSequence = *(int *)msg->data;
	clc_reliableAcknowledge = MSG_ReadLong( msg );
	if ( clc_reliableAcknowledge < clc_reliableSequence - 64 ) {
		clc_reliableAcknowledge = clc_reliableSequence;
		return;
	}

	CL_Netchan_Decode( msg->data + msg->readcount,
	                   msg->cursize - msg->readcount );
	CL_ParseServerMessage( msg );

	if ( clc_demorecording && !clc_demowaiting ) {
		CL_WriteDemoMessage( msg, headerBytes );
	}
}

/* ---- CL_Frame  0x00411280 ----  VERIFIED */
void CL_Frame( int msec ) {
	extern cvar_t *com_cl_running;
	extern cvar_t *com_sv_running;
	extern cvar_t *com_timescale;

	extern cvar_t *cl_avidemo;              /* 0x0143285C */
	extern cvar_t *cl_forceavidemo;         /* 0x0155F23C */

	extern int xanim_activePoolSlot;
#define xanim_activePoolSlot xanim_activePoolSlot
	extern int sys_monkeySpank;                /* 0x008E3C48, reliable-cmd probe gate */
	extern int CL_handle;                   /* 0x0057C118, current cinematic handle */

	extern void CL_ChangeReliableCommand( void );   /* 0x0040E340 */
	extern void CL_CheckUserinfo( void );           /* 0x004111C0 */
	extern void CL_CheckTimeout( void );            /* 0x00411130 */
	extern void CL_SendCmd( void );                 /* 0x0040BF30 */
	extern void CL_CheckForResend( void );          /* 0x004103D0 */
	extern void CL_SetCGameTime( void );            /* 0x00404D60 */
	extern void CL_UpdateInGameState( void );       /* 0x00411220 */
	extern void MSS_Update( void );                 /* 0x0044F920 */
	extern int CIN_RunCinematic( int handle );      /* 0x00407680, returns e_status */
	extern void Con_RunConsole( void );             /* 0x0040A260 */
	extern void SCR_DebugGraph( float value, int color );   /* 0x00416BC0 */

	/* Retail 0x00411281..0x00411440 dereferences every cvar below without a
	 * NULL test (all are registered in Com_Init / CL_Init before cl_running
	 * is set), calls VM_Call on uivm untested (0x004112E2; CL_StartHunkUsers
	 * has started the UI whenever cl_running is set) and calls Con_RunConsole
	 * unconditionally. */
	if ( !com_cl_running->integer ) {
		return;
	}

	xanim_activePoolSlot = 0;

	if ( cls_cddialog ) {
		cls_cddialog = 0;
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NEED_CD );
	} else if ( cls_state == CA_DISCONNECTED
	         && !( cls_keyCatchers & KEYCATCH_UI )
	         && !com_sv_running->integer ) {
		MSS_StopSounds( 0 );
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
	}

	if ( cl_avidemo->integer && msec ) {
		if ( cls_state == CA_ACTIVE || cl_forceavidemo->integer ) {
			Cbuf_ExecuteText( EXEC_NOW, "screenshot silent\n" );
		}
		/* 1000.0f (0x447A0000) is a pooled .rdata literal in retail; retail
		 * fild's the integer straight into the divide (0x00411349) with no
		 * rounding to float first, hence the rfloat (double in the modern build);
		 * the FCAST keeps VC7 from folding the constant to the end. */
		msec = (int) ( FCAST( (rfloat)1000.0f / cl_avidemo->integer ) * com_timescale->value );
		if ( !msec ) {
			msec = 1;
		}
	}

	if ( sys_monkeySpank && ( cls_framecount & 0xFF ) == 0 ) {
		if ( rand() * ( 1.0f / 32768.0f ) < 0.1 ) {
			CL_ChangeReliableCommand();
		}
	}

	cls_realFrametime = msec;
	cls_frametime = msec;
	cls_realtime += cls_frametime;

	/* 0x004113C9..0x004113F7: SCR_DebugGraph inlined -- push msec * 0.25f
	 * (flt_568ED0) with color 0 into the 1024-entry ring.  msec is the value
	 * after the avidemo adjustment above.  Retail's stores come after the
	 * cl_timegraph->integer load only because cls and the graph ring are
	 * single structs there (the tree splits both into separate globals). */
	if ( ((cvar_t *)cl_timegraph)->integer ) {
		SCR_DebugGraph( msec * 0.25f, 0 );
	}

	CL_CheckUserinfo();
	CL_CheckTimeout();
	CL_SendCmd();
	CL_CheckForResend();
	CL_SetCGameTime();
	CL_UpdateInGameState();

	SCR_UpdateScreen();

	MSS_Update();

	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_RunCinematic( CL_handle );
	}

	Con_RunConsole();

	cls_framecount++;
}

/* ---- CL_Init  0x00411E60 ----  [CONFIRMED] */
void CL_Init( void ) {
	Com_Printf( "----- Client Initialization -----\n" );

	Con_Init();
	/* 0x00411E71..0x00411E7F: rep stosd of 0x4B236 dwords over cl_active --
	 * the same body as CL_ClearState 0x0040EF70. */
	CL_ClearState();
	cls_state = CA_DISCONNECTED;
	cls_realtime = 0;

	CL_InitInput();

	cl_noprint = Cvar_Get( "cl_noprint", "0", 0 );
	cl_motd = Cvar_Get( "cl_motd", "1", 0 );
	cl_timeout = Cvar_Get( "cl_timeout", "200", 0 );
	cl_timeNudge = Cvar_Get( "cl_timeNudge", "0", CVAR_TEMP );
	cl_shownet = Cvar_Get( "cl_shownet", "0", CVAR_TEMP );
	cl_shownuments = Cvar_Get( "cl_shownuments", "0", CVAR_TEMP );
	cl_visibleClients = Cvar_Get( "cl_visibleClients", "0", CVAR_TEMP );
	cl_showServerCommands = Cvar_Get( "cl_showServerCommands", "0", 0 );
	cl_showSend = Cvar_Get( "cl_showSend", "0", CVAR_TEMP );
	cl_showTimeDelta = Cvar_Get( "cl_showTimeDelta", "0", CVAR_TEMP );
	cl_freezeDemo = Cvar_Get( "cl_freezeDemo", "0", CVAR_TEMP );
	rconPassword = Cvar_Get( "rconPassword", "", CVAR_TEMP );
	activeAction = Cvar_Get( "activeAction", "", CVAR_TEMP );
	timedemo = Cvar_Get( "timedemo", "0", 0 );
	cl_avidemo = Cvar_Get( "cl_avidemo", "0", 0 );
	cl_forceavidemo = Cvar_Get( "cl_forceavidemo", "0", 0 );
	rconAddress = Cvar_Get( "rconAddress", "", 0 );
	cl_yawspeed = Cvar_Get( "cl_yawspeed", "140", CVAR_ARCHIVE );
	cl_pitchspeed = Cvar_Get( "cl_pitchspeed", "140", CVAR_ARCHIVE );
	cl_anglespeedkey = Cvar_Get( "cl_anglespeedkey", "1.5", 0 );
	cl_maxpackets = Cvar_Get( "cl_maxpackets", "30", CVAR_ARCHIVE );
	cl_packetdup = Cvar_Get( "cl_packetdup", "1", CVAR_ARCHIVE );
	cl_run = Cvar_Get( "cl_run", "1", CVAR_TEMP );
	cl_stance = Cvar_Get( "cl_stance", "0", CVAR_TEMP );
	cl_stanceTemp = Cvar_Get( "cl_stanceTemp", "0", CVAR_TEMP );
	cl_goStandJumpTime = Cvar_Get( "cl_goStandJumpTime", "0", CVAR_ARCHIVE );
	cl_sensitivity = Cvar_Get( "sensitivity", "5", CVAR_ARCHIVE );
	cl_mouseAccel = Cvar_Get( "cl_mouseAccel", "0", CVAR_ARCHIVE );
	cl_freelook = Cvar_Get( "cl_freelook", "1", CVAR_ARCHIVE );
	cl_showmouserate = Cvar_Get( "cl_showmouserate", "0", 0 );
	cl_allowDownload = Cvar_Get( "cl_allowDownload", "0", CVAR_ARCHIVE );
	cl_conXOffset = Cvar_Get( "cl_conXOffset", "0", 0 );
	r_inGameVideo = Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	cl_serverStatusResendTime = Cvar_Get( "cl_serverStatusResendTime", "750", 0 );
	cl_viewPitchCompensate = Cvar_Get( "cl_viewPitchCompensate", "0", CVAR_ROM );
	cl_viewYawCompensate = Cvar_Get( "cl_viewYawCompensate", "0", CVAR_ROM );
	cl_bypassMouseInput = Cvar_Get( "cl_bypassMouseInput", "0", 0 );
	m_pitch = Cvar_Get( "m_pitch", "0.022", CVAR_ARCHIVE );
	m_yaw = Cvar_Get( "m_yaw", "0.022", CVAR_ARCHIVE );
	m_forward = Cvar_Get( "m_forward", "0.25", CVAR_ARCHIVE );
	m_side = Cvar_Get( "m_side", "0.25", CVAR_ARCHIVE );
	m_filter = Cvar_Get( "m_filter", "0", CVAR_ARCHIVE );
	cl_motdString = Cvar_Get( "cl_motdString", "", CVAR_ROM );
	cl_ingame = Cvar_Get( "cl_ingame", "0", CVAR_ROM );
	Cvar_Get( "cl_maxPing", "800", CVAR_ARCHIVE );

	Cvar_Get( "cg_drawCompass", "1", CVAR_ARCHIVE );
	Cvar_Get( "cg_drawNotifyText", "1", CVAR_ARCHIVE );
	Cvar_Get( "cg_descriptiveText", "1", CVAR_ARCHIVE );
	Cvar_Get( "cg_drawTeamOverlay", "2", CVAR_ARCHIVE );
	Cvar_Get( "cg_drawGun", "1", CVAR_ARCHIVE );
	Cvar_Get( "cg_cursorHints", "4", CVAR_ARCHIVE );
	Cvar_Get( "cg_voiceSpriteTime", "6000", CVAR_ARCHIVE );
	Cvar_Get( "cg_teamChatsOnly", "0", CVAR_ARCHIVE );
	Cvar_Get( "cg_noVoiceChats", "0", CVAR_ARCHIVE );
	Cvar_Get( "cg_noVoiceText", "0", CVAR_ARCHIVE );
	Cvar_Get( "cg_crosshairSize", "48", CVAR_ARCHIVE );
	Cvar_Get( "cg_drawCrosshair", "1", CVAR_ARCHIVE );

	Cvar_Get( "name", "Unknown Soldier", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "rate", "5000", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "snaps", "20", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "model", "", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "head", "", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "handicap", "100", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "cl_anonymous", "0", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "password", "", CVAR_USERINFO );
	Cvar_Get( "cg_predictItems", "1", CVAR_ARCHIVE | CVAR_USERINFO );
	Cvar_Get( "cg_viewsize", "100", CVAR_ARCHIVE );

	cl_waitForFire = Cvar_Get( "cl_waitForFire", "0", CVAR_ROM );

	fx_enable = Cvar_Get( "fx_enable", "1", CVAR_CHEAT );
	fx_draw = Cvar_Get( "fx_draw", "1", CVAR_CHEAT );
	fx_cull = Cvar_Get( "fx_cull", "1", 0 );
	fx_freeze = Cvar_Get( "fx_freeze", "0", CVAR_CHEAT );
	fx_debug = Cvar_Get( "fx_debug", "0", CVAR_CHEAT );
	fx_debugBolt = Cvar_Get( "fx_debugBolt", "0", CVAR_CHEAT );
	fx_count = Cvar_Get( "fx_count", "0", CVAR_CHEAT );

	cl_updateavailable = Cvar_Get( "cl_updateavailable", "0", CVAR_ROM );
	cl_updatefiles = Cvar_Get( "cl_updatefiles", "", CVAR_ROM );
	cl_updateoldversion = Cvar_Get( "cl_updateoldversion", "", CVAR_ROM );
	cl_updateversion = Cvar_Get( "cl_updateversion", "", CVAR_ROM );

	cl_serverloadmap = Cvar_Get( "cl_serverloadmap", "", CVAR_ROM );
	cl_serverloadgametype = Cvar_Get( "cl_serverloadgametype", "", CVAR_ROM );
	cl_serverloadwaiting = Cvar_Get( "cl_serverloadwaiting", "0", CVAR_ROM );

	/* cls.autoupdateServerNames[5][64], 0x00412575..0x004125F4: strncpy 63 then byte 63 = 0 */
	strncpy( cls_autoupdateServerNames_0_, "au2cod1.activision.com", 63 ); cls_autoupdateServerNames_0_[63] = 0;
	strncpy( cls_autoupdateServerNames_1_, "au2cod2.activision.com", 63 ); cls_autoupdateServerNames_1_[63] = 0;
	strncpy( cls_autoupdateServerNames_2_, "au2cod3.activision.com", 63 ); cls_autoupdateServerNames_2_[63] = 0;
	strncpy( cls_autoupdateServerNames_3_, "au2cod4.activision.com", 63 ); cls_autoupdateServerNames_3_[63] = 0;
	strncpy( cls_autoupdateServerNames_4_, "au2cod5.activision.com", 63 ); cls_autoupdateServerNames_4_[63] = 0;

	Cmd_AddCommand( "cmd", CL_ForwardToServer_f );
	Cmd_AddCommand( "configstrings", CL_Configstrings_f );
	Cmd_AddCommand( "clientinfo", CL_Clientinfo_f );
	Cmd_AddCommand( "snd_restart", CL_Snd_Restart_f );
	Cmd_AddCommand( "vid_restart", CL_Vid_Restart_f );
	Cmd_AddCommand( "disconnect", CL_Disconnect_f );
	Cmd_AddCommand( "record", CL_Record_f );
	Cmd_AddCommand( "demo", CL_PlayDemo_f );
	Cmd_AddCommand( "cinematic", CL_PlayCinematic_f );
	Cmd_AddCommand( "logo", CL_PlayLogo_f );
	Cmd_AddCommand( "stoprecord", CL_StopRecord_f );
	Cmd_AddCommand( "connect", CL_Connect_f );
	Cmd_AddCommand( "reconnect", CL_Reconnect_f );
	Cmd_AddCommand( "localservers", CL_LocalServers_f );
	Cmd_AddCommand( "globalservers", CL_GlobalServers_f );
	Cmd_AddCommand( "rcon", CL_Rcon_f );
	Cmd_AddCommand( "setenv", CL_Setenv_f );
	Cmd_AddCommand( "ping", CL_Ping_f );
	Cmd_AddCommand( "serverstatus", CL_ServerStatus_f );
	Cmd_AddCommand( "showip", CL_ShowIP_f );
	Cmd_AddCommand( "fs_openedList", CL_OpenedPK3List_f );
	Cmd_AddCommand( "fs_referencedList", CL_ReferencedPK3List_f );
	Cmd_AddCommand( "updatehunkusage", CL_UpdateLevelHunkUsage );
	Cmd_AddCommand( "updatescreen", SCR_UpdateScreen );
	Cmd_AddCommand( "startSingleplayer", CL_startSingleplayer_f );
	Cmd_AddCommand( "setRecommended", CL_SetRecommended_f );
	Cmd_AddCommand( "cubemapShot", CL_CubeMapShot_f );
	Cmd_AddCommand( "localizeSoundAliasFiles", Com_WriteLocalizedSoundAliasFiles );

	/* 0x004127A0..0x004127AC -- CL_Init is CL_CheckAutoUpdate's only caller */
	autoupdateChecked = 0;
	autoupdateStarted = 0;
	CL_CheckAutoUpdate();

	CL_InitRef();
	SCR_Init();
	Cbuf_Execute();     /* 0x004127BB */
	Cvar_Set2( "cl_running", "1", qtrue );

	Com_Printf( "----- Client Initialization Complete -----\n" );
}

/* ---- CL_Shutdown  0x004127F0 ----  [CONFIRMED] */
void CL_Shutdown( void ) {
	static int recursive = 0;

	Com_Printf( "----- CL_Shutdown -----\n" );

	if ( recursive ) {
		printf( "recursive shutdown\n" );
		return;
	}
	recursive = 1;

	/* 0x0041281F: CL_ShutdownDebugData -- its only caller. */
	CL_ShutdownDebugData();

	/* 0x00412826: retail disconnects the client before tearing it down -- push 1;
	 * call CL_Disconnect, right after CL_ShutdownDebugData and before
	 * CL_ShutdownCGame.  CL_Disconnect(qtrue) queues the "disconnect" reliable
	 * command and flushes it with three CL_WritePacket calls, so a quitting
	 * player drops from the server at once instead of lingering until
	 * SV_CheckTimeouts fires the sv_timeout path. */
	CL_Disconnect( qtrue );

	CL_ShutdownCGame();
	MSS_Shutdown();     /* 0x00412833 */
	/* Retail inlines CL_ShutdownRef here -- 0x00412838 loads the refexport
	 * slot and 0x00412841 is `push 1`, so RE_Shutdown(1) reaches GLimp_Shutdown
	 * and WG_RestoreGamma.  CL_Shutdown is the ONLY site in the binary passing
	 * 1; every other caller passes 0 and leaves the window up. */
	CL_ShutdownRef();
	CL_ShutdownUI();
	CL_ShutdownInput(); /* 0x0041286C -- its only caller */

	Cmd_RemoveCommand( "cmd" );
	Cmd_RemoveCommand( "configstrings" );
	Cmd_RemoveCommand( "clientinfo" );
	Cmd_RemoveCommand( "snd_restart" );
	Cmd_RemoveCommand( "vid_restart" );
	Cmd_RemoveCommand( "disconnect" );
	Cmd_RemoveCommand( "record" );
	Cmd_RemoveCommand( "demo" );
	Cmd_RemoveCommand( "cinematic" );
	Cmd_RemoveCommand( "stoprecord" );
	Cmd_RemoveCommand( "connect" );
	Cmd_RemoveCommand( "reconnect" );
	Cmd_RemoveCommand( "localservers" );
	Cmd_RemoveCommand( "globalservers" );
	Cmd_RemoveCommand( "rcon" );
	Cmd_RemoveCommand( "setenv" );
	Cmd_RemoveCommand( "ping" );
	Cmd_RemoveCommand( "serverstatus" );
	Cmd_RemoveCommand( "showip" );
	Cmd_RemoveCommand( "fs_openedList" );
	Cmd_RemoveCommand( "fs_referencedList" );
	Cmd_RemoveCommand( "updatehunkusage" );
	Cmd_RemoveCommand( "updatescreen" );
	Cmd_RemoveCommand( "SaveTranslations" );
	Cmd_RemoveCommand( "SaveNewTranslations" );
	Cmd_RemoveCommand( "LoadTranslations" );
	Cmd_RemoveCommand( "startSingleplayer" );
	Cmd_RemoveCommand( "buyNow" );
	Cmd_RemoveCommand( "singlePlayLink" );
	Cmd_RemoveCommand( "setRecommended" );
	Cmd_RemoveCommand( "cubemapShot" );

	Cvar_Set2( "cl_running", "0", qtrue );

	recursive = 0;

	/* 0x004129C1..0x004129DC: one rep stosd over all of clientStatic_t */
	memset( &cls, 0, sizeof( cls ) );
	Com_Printf( "-----------------------\n" );
}
