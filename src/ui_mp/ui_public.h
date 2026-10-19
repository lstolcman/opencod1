#ifndef __UI_PUBLIC_H__
#define __UI_PUBLIC_H__

#include "../universal/q_shared.h"
#include "ui_shared.h"

#ifndef QDECL
#define QDECL   __cdecl
#endif

#define UI_API_VERSION  7

typedef enum {
	CA_DISCONNECTED,    // not talking to a server
	CA_CONNECTING,      // sending request packets to the server
	CA_CHALLENGING,     // sending challenge packets to the server
	CA_CONNECTED,       // netchan_t established, getting gamestate
	CA_LOADING,         // only during cgame initialization, never during main loop
	CA_PRIMED,          // got gamestate, waiting for first frame
	CA_ACTIVE,          // game views should be displayed
	CA_CINEMATIC,       // playing a cinematic or a static pic, not connected to a server
	CA_LOGO
} connstate_t;

typedef struct {
	connstate_t connState;
	int connectPacketCount;
	int clientNum;
	char servername[MAX_STRING_CHARS];
	char updateInfoString[MAX_STRING_CHARS];
	char messageString[MAX_STRING_CHARS];
} uiClientState_t;
UI_ASSERT_SIZE( uiClientState_t, 3084 );

typedef struct polyVert_t {
	vec3_t xyz;
	float st[2];
	float st2[2];
	byte modulate[4];
} polyVert_t;
UI_ASSERT_SIZE( polyVert_t, 32 );

// real time
//=============================================

typedef struct {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
} qtime_t;

// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,       // play
	FMV_EOF,        // all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} e_status;

// server browser sort keys
#define SORT_PASSWORD       0
#define SORT_HOST           1
#define SORT_MAP            2
#define SORT_CLIENTS        3
#define SORT_GAME           4
#define SORT_PING           5

typedef enum
{
	UI_ERROR                               =   0,
	UI_PRINT                               =   1,
	UI_GETLANGUAGENAME                     =   2,
	UI_VERIFYLANGUAGESELECTION             =   3,
	UI_MILLISECONDS                        =   4,
	UI_CVAR_REGISTER                       =   5,
	UI_CVAR_UPDATE                         =   6,
	UI_CVAR_SET                            =   7,
	UI_CVAR_VARIABLEVALUE                  =   8,
	UI_CVAR_VARIABLESTRINGBUFFER           =   9,
	UI_CVAR_SETVALUE                       =  10,
	UI_CVAR_RESET                          =  11,
	UI_CVAR_CREATE                         =  12,
	UI_CVAR_INFOSTRINGBUFFER               =  13,
	UI_ARGC                                =  14,
	UI_ARGV                                =  15,
	UI_CMD_EXECUTETEXT                     =  16,
	UI_FS_FOPENFILE                        =  17,
	UI_FS_READ                             =  18,
	UI_FS_SEEK                             =  19,
	UI_FS_WRITE                            =  20,
	UI_FS_FCLOSEFILE                       =  21,
	UI_FS_GETFILELIST                      =  22,
	UI_FS_DELETEFILE                       =  23,
	UI_R_REGISTERMODEL                     =  24,
	UI_R_REGISTERSHADERNOMIP               =  25,
	UI_R_CLEARSCENE                        =  26,
	UI_R_ADDREFENTITYTOSCENE               =  27,
	UI_R_ADDPOLYTOSCENE                    =  28,
	// 29 is unused
	UI_R_ADDLIGHTTOSCENE                   =  30,
	UI_R_ADDCORONATOSCENE                  =  31,
	UI_R_RENDERSCENE                       =  32,
	UI_R_SETCOLOR                          =  33,
	UI_R_DRAWSTRETCHPIC                    =  34,
	UI_R_MODELBOUNDS                       =  35,
	UI_UPDATESCREEN                        =  36,
	// 37 is unused
	UI_S_REGISTERSOUND                     =  38,
	UI_S_STARTLOCALSOUND                   =  39,
	UI_S_FADEALLSOUNDS                     =  40,
	UI_KEY_KEYNUMTOSTRINGBUF               =  41,
	UI_KEY_GETBINDINGBUF                   =  42,
	UI_KEY_SETBINDING                      =  43,
	UI_KEY_ISDOWN                          =  44,
	UI_KEY_GETOVERSTRIKEMODE               =  45,
	UI_KEY_SETOVERSTRIKEMODE               =  46,
	UI_KEY_CLEARSTATES                     =  47,
	UI_KEY_GETCATCHER                      =  48,
	UI_KEY_SETCATCHER                      =  49,
	UI_GETCLIPBOARDDATA                    =  50,
	UI_GETCLIENTSTATE                      =  51,
	UI_GETGLCONFIG                         =  52,
	UI_GETCONFIGSTRING                     =  53,
	UI_GETCLIENTNAME                       =  54,
	UI_LAN_GETLOCALSERVERCOUNT             =  55,   // not handled by the engine
	UI_LAN_GETLOCALSERVERADDRESSSTRING     =  56,   // not handled by the engine
	UI_LAN_GETGLOBALSERVERCOUNT            =  57,   // not handled by the engine
	UI_LAN_GETGLOBALSERVERADDRESSSTRING    =  58,   // not handled by the engine
	UI_LAN_GETPINGQUEUECOUNT               =  59,
	UI_LAN_CLEARPING                       =  60,
	UI_LAN_GETPING                         =  61,
	UI_LAN_GETPINGINFO                     =  62,
	UI_MEMORY_REMAINING                    =  63,
	UI_GETCDKEY                            =  64,
	UI_SETCDKEY                            =  65,
	UI_R_REGISTERFONT                      =  66,
	UI_R_TEXT_WIDTH                        =  67,
	UI_R_TEXT_HEIGHT                       =  68,
	UI_R_TEXT_PAINT                        =  69,
	UI_R_TEXT_PAINTWITHCURSOR              =  70,
	UI_SE_TRANSLATEREFERENCE               =  71,
	UI_SE_LOCALIZEMESSAGE                  =  72,
	UI_PC_ADD_GLOBAL_DEFINE                =  73,
	UI_PC_LOADSOURCE                       =  74,
	UI_PC_FREESOURCE                       =  75,
	UI_PC_READTOKEN                        =  76,
	UI_PC_SOURCEFILEANDLINE                =  77,
	UI_REALTIME                            =  78,
	UI_LAN_GETSERVERCOUNT                  =  79,
	UI_LAN_GETSERVERADDRESSSTRING          =  80,
	UI_LAN_GETSERVERINFO                   =  81,
	UI_CLEARDISPLAYEDSERVERS               =  82,
	UI_LAN_UPDATEVISIBLEPINGS              =  83,
	UI_LAN_RESETPINGS                      =  84,
	UI_LAN_LOADCACHEDSERVERS               =  85,
	UI_LAN_SAVECACHEDSERVERS               =  86,
	UI_LAN_ADDSERVER                       =  87,
	UI_LAN_REMOVESERVER                    =  88,
	UI_CIN_PLAYCINEMATIC                   =  89,
	UI_CIN_STOPCINEMATIC                   =  90,
	UI_CIN_RUNCINEMATIC                    =  91,
	UI_CIN_DRAWCINEMATIC                   =  92,
	UI_CIN_SETEXTENTS                      =  93,
	UI_VERIFYCDKEY                         =  94,
	UI_LAN_SERVERSTATUS                    =  95,
	UI_LAN_GETSERVERPING                   =  96,
	UI_LAN_SERVERISDIRTY                   =  97,
	UI_LAN_COMPARESERVERS                  =  98,
	UI_SET_PBCLSTATUS                      =  99,
	UI_CHECKAUTOUPDATE                     = 100,
	UI_GETAUTOUPDATE                       = 101,
	// 102 - 110 are the QVM memory and math calls
	UI_Z_MALLOCINTERNAL                    = 111,
	UI_Z_FREEINTERNAL                      = 112
} uiImport_t;

typedef enum {
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_NEED_CD,
	UIMENU_BAD_CD_KEY,
	UIMENU_TEAM,
	UIMENU_POSTGAME,
	UIMENU_HELP,
	UIMENU_WM_QUICKMESSAGE,
	UIMENU_WM_AUTOUPDATE,
	UIMENU_SCRIPT_POPUP,
	UIMENU_SCRIPT_POPUP_NO_MOUSE
} uiMenuCommand_t;

typedef enum {
	UI_GETAPIVERSION = 0,   // system reserved

	UI_INIT,
//	void	UI_Init( void );

	UI_SHUTDOWN,
//	void	UI_Shutdown( void );

	UI_KEY_EVENT,
//	void	UI_KeyEvent( int key, int down );

	UI_MOUSE_EVENT,
//	void	UI_MouseEvent( int dx, int dy );

	UI_REFRESH,
//	void	UI_Refresh( int time );

	UI_IS_FULLSCREEN,
//	qboolean UI_IsFullscreen( void );

	UI_SET_ACTIVE_MENU,
//	void	UI_SetActiveMenu( uiMenuCommand_t menu );

	UI_GET_ACTIVE_MENU,
//	uiMenuCommand_t UI_GetActiveMenu( void );

	UI_GET_MAP_DISPLAY_NAME,
//	const char *UI_GetMapDisplayName( const char *mapname );

	UI_GET_GAME_TYPE_DISPLAY_NAME,
//	const char *UI_GetGameTypeDisplayName( const char *gametype );

	UI_CONSOLE_COMMAND,
//	qboolean UI_ConsoleCommand( int realTime );

	UI_DRAW_CONNECT_SCREEN,
//	void	UI_DrawConnectScreen( qboolean overlay );
// if !overlay, the background will be drawn, otherwise it will be
// overlayed over whatever the cgame has drawn.
// a GetClientState syscall will be made to get the current strings

	UI_HASUNIQUECDKEY,
//	qboolean UI_HasUniqueCDKey( void );

	UI_CHECKEXECKEY,
//	qboolean UI_CheckExecKey( void );

	UI_LOAD_SCRIPT_MENU,
//	qboolean Load_ScriptMenu( const char *menuname );

	UI_GET_FONT_INFO
//	fontInfo_t *UI_GetFontInfo( int font, float scale );
} uiExport_t;

void        dllEntry( int ( QDECL *syscallptr )( int arg,... ) );
int         PASSFLOAT( float x );

void        trap_Print( const char *string );
void        trap_Error( const char *string );
const char  *trap_GetLanguagename( int language );
int         trap_VerifyLanguageSelection( int language );
int         trap_Milliseconds( void );
void        trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void        trap_Cvar_Update( vmCvar_t *cvar );
void        trap_Cvar_Set( const char *var_name, const char *value );
float       trap_Cvar_VariableValue( const char *var_name );
void        trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void        trap_Cvar_SetValue( const char *var_name, float value );
void        trap_Cvar_Reset( const char *name );
void        trap_Cvar_Create( const char *var_name, const char *var_value, int flags );
void        trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize );
int         trap_Argc( void );
void        trap_Argv( int n, char *buffer, int bufferLength );
void        trap_Cmd_ExecuteText( int exec_when, const char *text );
int         trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void        trap_FS_Read( void *buffer, int len, fileHandle_t f );
void        trap_FS_Seek( fileHandle_t f, int offset, int origin );
void        trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void        trap_FS_FCloseFile( fileHandle_t f );
int         trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int         trap_FS_Delete( const char *filename );
qhandle_t   trap_R_RegisterModel( const char *name, int imageTrack );
void        trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font, int imageTrack );
int         trap_R_Text_Width( const char *text, int font, float scale, int limit );
int         trap_R_Text_Height( int font, float scale );
void        trap_R_Text_Paint( float x, float y, int font, float scale, const vec4_t color, const char *text, float adjust, int limit, int style );
void        trap_R_Text_PaintWithCursor( float x, float y, int font, float scale, const vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style );
const char  *trap_SE_TranslateReference( const char *reference );
const char  *trap_SE_LocalizeMessage( const char *msg, const char *reference );
qhandle_t   trap_R_RegisterShaderNoMip( const char *name, int imageTrack );
void        trap_R_ClearScene( void );
void        trap_R_AddRefEntityToScene( const refEntity_t *re );
void        trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void        trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void        trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible );
void        trap_R_RenderScene( const refdef_t *fd );
void        trap_R_SetColor( const float *rgba );
void        trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void        trap_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs );
void        trap_UpdateScreen( void );
sfxHandle_t trap_S_RegisterSound( const char *sample );
void        trap_S_StartLocalSound( const char *aliasName );
void        trap_S_FadeAllSounds( float volume, int fadeTime );
void        trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void        trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void        trap_Key_SetBinding( int keynum, const char *binding );
qboolean    trap_Key_IsDown( int keynum );
qboolean    trap_Key_GetOverstrikeMode( void );
void        trap_Key_SetOverstrikeMode( qboolean state );
void        trap_Key_ClearStates( void );
int         trap_Key_GetCatcher( void );
void        trap_Key_SetCatcher( int catcher );
void        trap_GetClipboardData( char *buf, int bufsize );
void        trap_GetClientState( uiClientState_t *state );
void        trap_GetGlconfig( glconfig_t *glconfig );
int         trap_GetConfigString( int index, char *buff, int buffsize );
int         trap_GetClientName( int clientNum, char *buf, int buflen );
int         trap_LAN_GetLocalServerCount( void );
void        trap_LAN_GetLocalServerAddressString( int n, char *buf, int buflen );
int         trap_LAN_GetGlobalServerCount( void );
void        trap_LAN_GetGlobalServerAddressString( int n, char *buf, int buflen );
int         trap_LAN_GetPingQueueCount( void );
void        trap_LAN_ClearPing( int n );
void        trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime );
void        trap_LAN_GetPingInfo( int n, char *buf, int buflen );
qboolean    trap_LAN_UpdateVisiblePings( int source );
int         trap_LAN_GetServerCount( int source );
int         trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );
void        trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void        trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int         trap_LAN_AddServer( int source, const char *name, const char *addr );
void        trap_LAN_RemoveServer( int source, const char *addr );
int         trap_LAN_GetServerPing( int source, int n );
int         trap_LAN_ServerIsDirty( int source, int n );
int         trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
void        trap_LAN_LoadCachedServers( void );
void        trap_LAN_SaveCachedServers( void );
void        trap_ClearDisplayedServers( int source, int n, qboolean visible );
void        trap_SetPbClStatus( int status );
void        trap_LAN_ResetPings( int n );
int         trap_MemoryRemaining( void );
void        trap_GetCDKey( char *buf, int buflen, char *checksum, int checksumlen );
void        trap_SetCDKey( const char *key, const char *checksum );
int         trap_PC_AddGlobalDefine( char *define );
int         trap_PC_LoadSource( const char *filename );
int         trap_PC_FreeSource( int handle );
int         trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int         trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
int         trap_RealTime( qtime_t *qtime );
int         trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status    trap_CIN_StopCinematic( int handle );
void        trap_CIN_DrawCinematic( int handle );
e_status    trap_CIN_RunCinematic( int handle );
void        trap_CIN_SetExtents( int handle, int x, int y, int w, int h );
void        *trap_Z_MallocInternal( int size );
void        trap_Z_FreeInternal( void *ptr );
qboolean    trap_VerifyCDKey( const char *key, const char *chksum );
void        trap_CheckAutoUpdate( void );
qboolean    trap_GetAutoUpdate( void );

#endif  /* __UI_PUBLIC_H__ */
