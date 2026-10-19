/*
 * client_mp/cl_static.h -- clientStatic_t, the client state that survives
 * map changes: ONE object at 0x0155F2C0 (0x6B39C bytes, CL_Shutdown clears it
 * with a single rep stosd at 0x004129C1).  RTCW cl_local.h layout with CoD's
 * logo, server-list, autoupdate and debug-draw members.
 *
 * The old split names (cls_state, cls_realtime, ...) are kept as member macros.
 * Exe only: the DLLs never include it.
 */
#ifndef CL_STATIC_H
#define CL_STATIC_H

#include "../renderer/tr_glconfig.h"

/* Debug strings/lines drawn for the client and the local server: one record,
   CL_ShutdownDebugData clears all 36 bytes with a single memset. */
typedef struct {
	int             stringCapacity;             /* 0x015CA638 */
	int             stringCount;                /* 0x015CA63C */
	void           *strings;                    /* 0x015CA640 */
	void           *stringFromServer;           /* 0x015CA644 */
	int             lineCapacity;               /* 0x015CA648 */
	int             lineCount;                  /* 0x015CA64C */
	void           *lines;                      /* 0x015CA650 */
	void           *lineFromServer;             /* 0x015CA654 */
	void           *lineDurations;              /* 0x015CA658 */
} clientDebug_t;

typedef struct {
	int             state;                      /* 0x0155F2C0 connstate_t */
	int             keyCatchers;                /* 0x0155F2C4 */
	int             cddialog;                   /* 0x0155F2C8 */
	char            servername[256];            /* 0x0155F2CC */
	int             rendererStarted;            /* 0x0155F3CC */
	int             soundStarted;               /* 0x0155F3D0 */
	int             uiStarted;                  /* 0x0155F3D4 */
	int             framecount;                 /* 0x0155F3D8 */
	int             frametime;                  /* 0x0155F3DC msec since last frame */
	int             realtime;                   /* 0x0155F3E0 */
	int             realFrametime;              /* 0x0155F3E4 */
	int             logoStartTime;              /* 0x0155F3E8 */
	int             logoTotalDuration;          /* 0x0155F3EC */
	int             logoFadeInDuration;         /* 0x0155F3F0 */
	int             logoFadeOutDuration;        /* 0x0155F3F4 */
	int             logoShaderTop;              /* 0x0155F3F8 */
	int             logoShaderBottom;           /* 0x0155F3FC */
	int             numlocalservers;            /* 0x0155F400 */
	unsigned char   localServers[23552];        /* 0x0155F404 serverInfo_t[] */
	int             numglobalservers;           /* 0x01565004 */
	unsigned char   globalServers[0x5C000];     /* 0x01565008 serverInfo_t[] */
	int             numGlobalServerAddresses;   /* 0x015C1008 */
	unsigned char   globalServerAddresses[0x3000];  /* 0x015C100C serverAddress_t[] */
	int             numfavoriteservers;         /* 0x015C400C */
	unsigned char   favoriteServers[23552];     /* 0x015C4010 serverInfo_t[] */
	int             pingUpdateSource;           /* 0x015C9C10 */
	netadr_t        updateServer;               /* 0x015C9C14 */
	char            updateChallenge[MAX_TOKEN_CHARS];   /* 0x015C9C28 */
	char            updateInfoString[MAX_INFO_STRING];  /* 0x015CA028 */
	unsigned char   authorizeServer[20];        /* 0x015CA428 netadr_t */
	char            autoupdateServerNames[5][64];       /* 0x015CA43C */
	unsigned char   autoupdateServer[20];       /* 0x015CA57C netadr_t */
	glconfig_t      glconfig;                   /* 0x015CA590 */
	int             whiteShader;                /* 0x015CA630 */
	int             consoleShader;              /* 0x015CA634 */
	clientDebug_t   debug;                      /* 0x015CA638 */
} clientStatic_t;

extern clientStatic_t cls;

#define cls_state                        cls.state
#define cls_keyCatchers                  cls.keyCatchers
#define cls_cddialog                     cls.cddialog
#define cls_servername                   cls.servername
#define cls_rendererStarted              cls.rendererStarted
#define cls_soundStarted                 cls.soundStarted
#define cls_loadingPlaque                cls.uiStarted
#define cls_framecount                   cls.framecount
#define cls_frametime                    cls.frametime
#define cls_realtime                     cls.realtime
#define cls_realFrametime                cls.realFrametime
#define cls_logoStartTime                cls.logoStartTime
#define cls_logoTotalDuration            cls.logoTotalDuration
#define cls_logoFadeInDuration           cls.logoFadeInDuration
#define cls_logoFadeOutDuration          cls.logoFadeOutDuration
#define cls_logoShaderTop                cls.logoShaderTop
#define cls_logoShaderBottom             cls.logoShaderBottom
#define cls_numlocalservers              cls.numlocalservers
#define cls_localServersStorage          cls.localServers
#define cls_numglobalservers             cls.numglobalservers
#define cls_globalServersStorage         cls.globalServers
#define cls_numGlobalServerAddresses     cls.numGlobalServerAddresses
#define cls_globalServerAddressesStorage cls.globalServerAddresses
#define cls_numfavoriteservers           cls.numfavoriteservers
#define cls_favoriteServersStorage       cls.favoriteServers
#define cls_pingUpdateSource             cls.pingUpdateSource
#define cls_updateServer                 cls.updateServer
#define cls_updateChallenge              cls.updateChallenge
#define cls_updateInfoString             cls.updateInfoString
#define cls_authorizeServer              cls.authorizeServer
#define cls_autoupdateServerNames_0_     cls.autoupdateServerNames[0]
#define cls_autoupdateServerNames_1_     cls.autoupdateServerNames[1]
#define cls_autoupdateServerNames_2_     cls.autoupdateServerNames[2]
#define cls_autoupdateServerNames_3_     cls.autoupdateServerNames[3]
#define cls_autoupdateServerNames_4_     cls.autoupdateServerNames[4]
#define cls_autoupdateServer             cls.autoupdateServer
#define cls_glconfig                     cls.glconfig
#define whiteShader                      cls.whiteShader
#define cls_consoleShader                cls.consoleShader
#define cls_debugStringCapacity          cls.debug.stringCapacity
#define cls_debugStringCount             cls.debug.stringCount
#define cls_debugStrings                 cls.debug.strings
#define cls_debugStringFromServer        cls.debug.stringFromServer
#define cls_debugLineCapacity            cls.debug.lineCapacity
#define cls_debugLineCount               cls.debug.lineCount
#define cls_debugLines                   cls.debug.lines
#define cls_debugLineFromServer          cls.debug.lineFromServer
#define cls_debugLineDurations           cls.debug.lineDurations

/* clientConnection_t clc: ONE object at 0x015CE860, 0x28804 bytes.
   CL_Disconnect clears it with a single rep stosd (0xA201 dwords). */
typedef struct {
	int             clientNum;                  /* 0x015CE860 */
	int             lastPacketSentTime;         /* 0x015CE864 */
	int             lastPacketTime;             /* 0x015CE868 */
	unsigned char   serverAddress[20];          /* 0x015CE86C netadr_t */
	int             connectTime;                /* 0x015CE880 */
	int             connectPacketCount;         /* 0x015CE884 */
	char            serverMessage[256];         /* 0x015CE888 */
	char            challenge[4];               /* 0x015CE988 int */
	int             checksumFeed;               /* 0x015CE98C */
	int             onlyVisibleClients;         /* 0x015CE990 */
	int             reliableSequence;           /* 0x015CE994 */
	int             reliableAcknowledge;        /* 0x015CE998 */
	char            reliableCommands[65536];    /* 0x015CE99C [64][1024] */
	int             serverMessageSequence;      /* 0x015DE99C */
	int             serverCommandSequence;      /* 0x015DE9A0 */
	int             lastExecutedServerCommand;  /* 0x015DE9A4 */
	char            serverCommands[65536];      /* 0x015DE9A8 [64][1024] */
	int             download;                   /* 0x015EE9A8 fileHandle_t */
	unsigned char   downloadTempName[256];      /* 0x015EE9AC */
	unsigned char   downloadName[256];          /* 0x015EEAAC */
	int             downloadNumber;             /* 0x015EEBAC */
	char            downloadBlock[4];           /* 0x015EEBB0 int */
	int             downloadCount;              /* 0x015EEBB4 */
	int             downloadSize;               /* 0x015EEBB8 */
	unsigned char   downloadList[1024];         /* 0x015EEBBC */
	int             downloadRestart;            /* 0x015EEFBC */
	char            demoName[64];               /* 0x015EEFC0 */
	int             demorecording;              /* 0x015EF000 */
	int             demoplaying;                /* 0x015EF004 */
	int             demowaiting;                /* 0x015EF008 */
	int             firstDemoFrameSkipped;      /* 0x015EF00C */
	int             demofile;                   /* 0x015EF010 */
	int             timeDemoFrames;             /* 0x015EF014 */
	int             timeDemoStart;              /* 0x015EF018 */
	int             timeDemoBaseTime;           /* 0x015EF01C */
	netchan_t       netchan;                    /* 0x015EF020 */
	netProfileInfo_t *pProf;                    /* 0x015F7060 out-of-band profile */
} clientConnection_t;

extern clientConnection_t clc;

#define clc_clientNum                    clc.clientNum
#define clc_lastPacketSentTime           clc.lastPacketSentTime
#define clc_lastPacketTime               clc.lastPacketTime
#define clc_serverAddress                clc.serverAddress
#define clc_connectTime                  clc.connectTime
#define clc_connectPacketCount           clc.connectPacketCount
#define clc_serverMessage                clc.serverMessage
#define clc_challenge                    clc.challenge
#define clc_checksumFeed                 clc.checksumFeed
#define clc_onlyVisibleClients           clc.onlyVisibleClients
#define clc_reliableSequence             clc.reliableSequence
#define clc_reliableAcknowledge          clc.reliableAcknowledge
#define clc_reliableCommands             clc.reliableCommands
#define clc_serverMessageSequence        clc.serverMessageSequence
#define clc_serverCommandSequence        clc.serverCommandSequence
#define clc_lastExecutedServerCommand    clc.lastExecutedServerCommand
#define clc_serverCommands               clc.serverCommands
#define clc_download                     clc.download
#define cls_downloadTempName             clc.downloadTempName
#define cls_downloadName                 clc.downloadName
#define cls_downloadBlock                clc.downloadBlock
#define cls_downloadCount                clc.downloadCount
#define clc_downloadSize                 clc.downloadSize
#define clc_downloadList                 clc.downloadList
#define cls_downloadRestart              clc.downloadRestart
#define clc_demoName                     clc.demoName
#define clc_demorecording                clc.demorecording
#define clc_demoplaying                  clc.demoplaying
#define clc_demowaiting                  clc.demowaiting
#define clc_firstDemoFrameSkipped        clc.firstDemoFrameSkipped
#define clc_demofile                     clc.demofile
#define clc_timeDemoFrames               clc.timeDemoFrames
#define clc_timeDemoStart                clc.timeDemoStart
#define clc_timeDemoBaseTime             clc.timeDemoBaseTime

#endif /* CL_STATIC_H */
