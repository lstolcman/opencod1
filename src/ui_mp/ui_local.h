#ifndef __UI_LOCAL_H__
#define __UI_LOCAL_H__

#include "../universal/q_shared.h"
#include "ui_public.h"
#include "ui_shared.h"

#define MAX_CLIENTS             64
#define MAX_NAME_LENGTH         32
#define MAX_INFO_STRING         1024
#define BIGCHAR_WIDTH           16
#define BIGCHAR_HEIGHT          16

// parameters to the main Cbuf_ExecuteText
#define EXEC_NOW                0       // don't return until completed
#define EXEC_INSERT             1       // insert at current position, but don't run yet
#define EXEC_APPEND             2       // add to end of the command buffer (normal case)

#define S_COLOR_RED             "^1"
#define S_COLOR_YELLOW          "^3"

extern const vec4_t colorBlack;
extern const vec4_t colorWhite;

#define MAX_ARENAS              64
#define MAX_ARENAS_TEXT         8192

// new ui stuff
#define MAX_HEADS               64
#define MAX_ALIASES             64
#define MAX_TEAMS               64
#define MAX_GAMETYPES           32
#define MAX_MAPS                128
#define TEAM_MEMBERS            5
#define MAX_PINGREQUESTS        16
#define MAX_ADDRESSLENGTH       64
#define MAX_DISPLAY_SERVERS     2048
#define MAX_SERVERSTATUS_LINES  128
#define MAX_SERVERSTATUS_TEXT   1024
#define MAX_SERVERSTATUSREQUESTS 16
#define MAX_FOUNDPLAYER_SERVERS 16
#define MAX_MODS                64
#define MAX_DEMOS               256
#define MAX_MOVIES              256

typedef struct {
	const char  *name;
	const char  *imageName;
	qhandle_t   headImage;
	qboolean    female;
} characterInfo;
UI_ASSERT_SIZE( characterInfo, 16 );

typedef struct {
	const char  *name;
	const char  *ai;
	const char  *action;
} aliasInfo;
UI_ASSERT_SIZE( aliasInfo, 12 );

typedef struct {
	const char  *teamName;
	const char  *imageName;
	const char  *teamMembers[TEAM_MEMBERS];
	qhandle_t   teamIcon;
	qhandle_t   teamIcon_Metal;
	qhandle_t   teamIcon_Name;
	int         cinematic;
} teamInfo;
UI_ASSERT_SIZE( teamInfo, 44 );

typedef struct {
	const char  *gameType;
	const char  *gtEnum;        // display name
} gameTypeInfo;
UI_ASSERT_SIZE( gameTypeInfo, 8 );

typedef struct {
	const char  *mapName;
	const char  *mapLoadName;
	const char  *imageName;
	const char  *opponentName;
	int         teamMembers;
	int         typeBits;       // -1 when no gametype is given
	int         cinematic;
	int         timeToBeat[MAX_GAMETYPES];
	qhandle_t   levelShot;
	qboolean    active;
} mapInfo;
UI_ASSERT_SIZE( mapInfo, 0xA4 );

typedef struct {
	char    adrstr[MAX_ADDRESSLENGTH];
	int     start;
} pinglist_t;
UI_ASSERT_SIZE( pinglist_t, 68 );

typedef struct serverStatus_s {
	pinglist_t  pingList[MAX_PINGREQUESTS];
	int         numqueriedservers;
	int         currentping;
	int         nextpingtime;
	int         maxservers;
	int         refreshtime;
	int         numServers;
	int         sortKey;
	int         sortDir;
	int         lastCount;
	qboolean    refreshActive;
	int         currentServer;
	int         displayServers[MAX_DISPLAY_SERVERS];
	int         numDisplayServers;
	int         numPlayersOnServers;
	int         nextDisplayRefresh;
	int         nextSortTime;
	qhandle_t   currentServerPreview;
	int         currentServerCinematic;
	int         motdLen;
	int         motdWidth;
	int         motdPaintX;
	int         motdPaintX2;
	int         motdOffset;
	int         motdTime;
	char        motd[MAX_STRING_CHARS];
} serverStatus_t;
UI_ASSERT_SIZE( serverStatus_t, 0x289C );

typedef struct {
	char        adrstr[MAX_ADDRESSLENGTH];
	char        name[MAX_ADDRESSLENGTH];
	int         startTime;
	int         serverNum;
	qboolean    valid;
} pendingServer_t;
UI_ASSERT_SIZE( pendingServer_t, 140 );

typedef struct {
	int             num;
	pendingServer_t server[MAX_SERVERSTATUSREQUESTS];
} pendingServerStatus_t;
UI_ASSERT_SIZE( pendingServerStatus_t, 0x8C4 );

typedef struct {
	char        address[MAX_ADDRESSLENGTH];
	char        *lines[MAX_SERVERSTATUS_LINES][4];
	char        text[MAX_SERVERSTATUS_TEXT];
	char        pings[MAX_CLIENTS * 3];
	int         numLines;
} serverStatusInfo_t;
UI_ASSERT_SIZE( serverStatusInfo_t, 0xD04 );

typedef struct {
	const char  *modName;
	const char  *modDescr;
} modInfo_t;
UI_ASSERT_SIZE( modInfo_t, 8 );

typedef struct {
	displayContextDef_t uiDC;

	int         newHighScoreTime;
	int         newBestTime;
	int         showPostGameTime;
	qboolean    newHighScore;
	qboolean    demoAvailable;
	qboolean    soundHighScore;

	int         characterCount;
	int         botIndex;
	characterInfo characterList[MAX_HEADS];

	int         aliasCount;
	aliasInfo   aliasList[MAX_ALIASES];

	int         teamCount;
	teamInfo    teamList[MAX_TEAMS];

	int         numGameTypes;
	gameTypeInfo gameTypes[MAX_GAMETYPES];

	int         numJoinGameTypes;
	gameTypeInfo joinGameTypes[MAX_GAMETYPES];

	int         redBlue;
	int         playerCount;
	int         myTeamCount;
	int         teamIndex;
	int         playerRefresh;
	int         playerIndex;
	int         playerNumber;
	qboolean    teamLeader;
	char        playerNames[MAX_CLIENTS][MAX_NAME_LENGTH];
	char        teamNames[MAX_CLIENTS][MAX_NAME_LENGTH];
	int         teamClientNums[MAX_CLIENTS];

	int         mapCount;
	mapInfo     mapList[MAX_MAPS];

	int         reserved_0x25B34;           // unused

	modInfo_t   modList[MAX_MODS];
	int         modCount;
	int         modIndex;

	const char  *demoList[MAX_DEMOS];
	int         demoCount;
	int         demoIndex;

	const char  *movieList[MAX_MOVIES];
	int         movieCount;
	int         movieIndex;
	int         previewMovie;

	serverStatus_t serverStatus;

	// for the showing the status of a server
	char        serverStatusAddress[MAX_ADDRESSLENGTH];
	serverStatusInfo_t serverStatusInfo;
	int         nextServerStatusRefresh;

	// to retrieve the status of server to find a player
	pendingServerStatus_t pendingServerStatus;
	char        findPlayerName[MAX_STRING_CHARS];
	char        foundPlayerServerAddresses[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	char        foundPlayerServerNames[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	int         currentFoundPlayerServer;
	int         numFoundPlayerServers;
	int         nextFindPlayerRefresh;

	int         reserved_0x2B008;           // unused
	sfxHandle_t newHighScoreSound;

	int         activeFont;
} uiInfo_t;
UI_ASSERT_SIZE( uiInfo_t, 0x2B014 );

extern uiInfo_t uiInfo;

typedef struct {
	vmCvar_t    *vmCvar;
	char        *cvarName;
	char        *defaultString;
	int         cvarFlags;
} cvarTable_t;
UI_ASSERT_SIZE( cvarTable_t, 16 );

extern vmCvar_t ui_arenasFile;
extern vmCvar_t ui_allowVote;
extern vmCvar_t ui_brass;
extern vmCvar_t ui_marks;
extern vmCvar_t ui_server1;
extern vmCvar_t ui_server2;
extern vmCvar_t ui_server3;
extern vmCvar_t ui_server4;
extern vmCvar_t ui_server5;
extern vmCvar_t ui_server6;
extern vmCvar_t ui_server7;
extern vmCvar_t ui_server8;
extern vmCvar_t ui_server9;
extern vmCvar_t ui_server10;
extern vmCvar_t ui_server11;
extern vmCvar_t ui_server12;
extern vmCvar_t ui_server13;
extern vmCvar_t ui_server14;
extern vmCvar_t ui_server15;
extern vmCvar_t ui_server16;
extern vmCvar_t ui_dedicated;
extern vmCvar_t ui_smallFont;
extern vmCvar_t ui_bigFont;
extern vmCvar_t ui_extraBigFont;
extern vmCvar_t ui_cdkeychecked;
extern vmCvar_t ui_selectedPlayer;
extern vmCvar_t ui_netSource;
extern vmCvar_t ui_menuFiles;
extern vmCvar_t ui_gameType;
extern vmCvar_t ui_joinGameType;
extern vmCvar_t ui_netGameType;
extern vmCvar_t ui_netGameTypeName;
extern vmCvar_t ui_newScriptMenu;
extern vmCvar_t ui_newScriptMenuIndex;
extern vmCvar_t ui_scriptMenu;
extern vmCvar_t ui_scriptMenuIndex;
extern vmCvar_t ui_scriptMenuAllowResponse;
extern vmCvar_t ui_waitingScriptMenu;
extern vmCvar_t ui_waitingScriptMenuIndex;
extern vmCvar_t ui_waitingScriptMenuNoMouse;
extern vmCvar_t ui_mapIndex;
extern vmCvar_t ui_currentMap;
extern vmCvar_t ui_currentNetMap;
extern vmCvar_t ui_browserMaster;
extern vmCvar_t ui_browserGameType;
extern vmCvar_t ui_browserSortKey;
extern vmCvar_t ui_browserShowFull;
extern vmCvar_t ui_browserShowEmpty;
extern vmCvar_t ui_browserShowPassword;
extern vmCvar_t ui_browserShowNoPassword;
extern vmCvar_t ui_serverStatusTimeOut;
extern vmCvar_t ui_cmd;
extern vmCvar_t ui_isSpectator;
extern vmCvar_t ui_hudAlpha;
extern vmCvar_t ui_languagewarnings;
extern vmCvar_t ui_languagewarningsaserrors;

//
// q_shared.c / q_parse.c
//
void        Com_DPrintf( const char *fmt, ... );
int QDECL   Com_sprintf( char *dest, int size, const char *fmt, ... );
char       *Q_CleanStr( char *string );
char       *Info_ValueForKey( const char *s, const char *key );
void        Info_SetValueForKey( char *s, const char *key, const char *value );
void        Com_BeginParseSession( const char *name );
void        Com_EndParseSession( void );
char       *Com_Parse( char **data_p );

//
// ui_main_mp.c
//
// q_shared.c
float       Com_Clamp( float min, float max, float value );
char       *Com_SkipPath( char *pathname );
void        Com_StripFilename( char *in, char *out );
char       *Q_strupr( char *s1 );
extern const vec4_t colorRed;
extern const vec4_t colorYellow;
extern const vec4_t colorLtGrey;

int         vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 );
void        AssetCache( void );
void        _UI_DrawSides( float x, float y, float w, float h, float size );
void        _UI_DrawTopBottom( float x, float y, float w, float h, float size );
void        _UI_DrawRect( float x, float y, float width, float height, float size, const float *color );
fontInfo_t *UI_GetFontInfo( int font, float scale );
void        Text_SetActiveFont( int font );
void        UI_ShowPostGame( qboolean newHigh );
void        UI_DrawCenteredPic( qhandle_t image, int w, int h );
void        _UI_Refresh( int realtime );
void        _UI_Shutdown( void );
char       *getMenuBuffer( const char *filename );
qboolean    Asset_Parse( int handle, int imageTrack );
void        Font_Report( void );
void        UI_Report( void );
qboolean    UI_ParseMenu( const char *menuFile, int imageTrack );
qboolean    Load_ScriptMenu( const char *menuname, int imageTrack );
qboolean    Load_Menu( int handle, int imageTrack );
void        UI_LoadMenus( const char *menuFile, qboolean reset, int imageTrack );
void        UI_Load( void );
const char *UI_GetMapDisplayName( const char *mapname );
const char *UI_GetGameTypeDisplayName( const char *gametype );
void        UI_ServersSort( int column, qboolean force );
void        WM_setItemPic( char *name, const char *shader );
void        WM_setVisibility( char *name, qboolean show );
qboolean    UI_CheckExecKey( int key );
void        UI_VerifyLanguage( void );
void        UI_GetGameTypesList( void );
void        _UI_Init( void );
void        _UI_KeyEvent( int key, qboolean down );
void        _UI_MouseEvent( int dx, int dy );
uiMenuCommand_t _UI_GetActiveMenu( void );
qboolean    _UI_SetActiveMenu( uiMenuCommand_t menu );
qboolean    _UI_IsFullscreen( void );
void        Text_PaintCenter( float x, float y, int font, float scale, const vec4_t color, const char *text, float adjust );
void        UI_DrawConnectScreen( qboolean overlay );
void        UI_RegisterCvars( void );
void        UI_UpdateCvars( void );
const char *UI_ConfigString( int index );
const char *UI_SafeTranslateString( const char *reference );
void        UI_UnusedStub1( void );
void        UI_UnusedStub2( void );
void        UI_UnusedStub3( void );
float       UI_UnusedFloatIdentity( float a1 );
int         UI_FloorToInt( float f );

//
// ui_atoms_mp.c
//
void QDECL  Com_Error( int level, const char *error, ... );
void QDECL  Com_Printf( const char *msg, ... );
float       UI_ClampCvar( float min, float max, float value );
void        UI_StartDemoLoop( void );
char       *UI_Argv( int arg );
char       *UI_Cvar_VariableString( const char *var_name );
qboolean    UI_ConsoleCommand( int realTime );
void        UI_Shutdown( void );
void        UI_AdjustFrom640( float *x, float *y, float *w, float *h );
void        UI_DrawNamedPic( float x, float y, float width, float height, const char *picname, int imageTrack );
void        UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader );
void        UI_FillRect( float x, float y, float width, float height, const float *color );
void        UI_DrawSides( float x, float y, float w, float h );
void        UI_DrawTopBottom( float x, float y, float w, float h );
void        UI_DrawRect( float x, float y, float width, float height, const float *color );
void        UI_SetColor( const float *rgba );
void        UI_UpdateScreen( void );
void        UI_DrawTextBox( int x, int y, int width, int lines );
qboolean    UI_CursorInRect( int x, int y, int width, int height );

//
// ui_gameinfo_mp.c
//
int         UI_ParseInfos( char *buf, int max, char *infos[] );
void        UI_LoadArenas( void );

#endif  /* __UI_LOCAL_H__ */
