// this file is only included when building a dll
// syscalls.asm is included instead when building a qvm

#include "ui_public.h"

static int ( QDECL * syscall )( int arg, ... ) = ( int ( QDECL * )( int, ... ) ) - 1;

void dllEntry( int ( QDECL *syscallptr )( int arg,... ) ) {
	syscall = syscallptr;
}

int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void trap_Print( const char *string ) {
	syscall( UI_PRINT, string );
}

void trap_Error( const char *string ) {
	syscall( UI_ERROR, string );
}

// name of the language, or of language 0 when out of range
const char *trap_GetLanguagename( int language ) {
	return (const char *)syscall( UI_GETLANGUAGENAME, language );
}

int trap_VerifyLanguageSelection( int language ) {
	return syscall( UI_VERIFYLANGUAGESELECTION, language );
}

int trap_Milliseconds( void ) {
	return syscall( UI_MILLISECONDS );
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	syscall( UI_CVAR_REGISTER, cvar, var_name, value, flags );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	syscall( UI_CVAR_UPDATE, cvar );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( UI_CVAR_SET, var_name, value );
}

float trap_Cvar_VariableValue( const char *var_name ) {
	int temp;
	temp = syscall( UI_CVAR_VARIABLEVALUE, var_name );
	return ( *(float*)&temp );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

void trap_Cvar_SetValue( const char *var_name, float value ) {
	syscall( UI_CVAR_SETVALUE, var_name, PASSFLOAT( value ) );
}

void trap_Cvar_Reset( const char *name ) {
	syscall( UI_CVAR_RESET, name );
}

void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	syscall( UI_CVAR_CREATE, var_name, var_value, flags );
}

void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	syscall( UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize );
}

int trap_Argc( void ) {
	return syscall( UI_ARGC );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( UI_ARGV, n, buffer, bufferLength );
}

void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	syscall( UI_CMD_EXECUTETEXT, exec_when, text );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( UI_FS_FOPENFILE, qpath, f, mode );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( UI_FS_READ, buffer, len, f );
}

void trap_FS_Seek( fileHandle_t f, int offset, int origin ) {
	syscall( UI_FS_SEEK, f, offset, origin );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall( UI_FS_WRITE, buffer, len, f );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( UI_FS_FCLOSEFILE, f );
}

int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return syscall( UI_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

int trap_FS_Delete( const char *filename ) {
	return syscall( UI_FS_DELETEFILE, filename );
}

qhandle_t trap_R_RegisterModel( const char *name, int imageTrack ) {
	return syscall( UI_R_REGISTERMODEL, name, imageTrack );
}

void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font, int imageTrack ) {
	syscall( UI_R_REGISTERFONT, fontName, pointSize, font, imageTrack );
}

int trap_R_Text_Width( const char *text, int font, float scale, int limit ) {
	return syscall( UI_R_TEXT_WIDTH, text, font, PASSFLOAT( scale ), limit );
}

int trap_R_Text_Height( int font, float scale ) {
	return syscall( UI_R_TEXT_HEIGHT, font, PASSFLOAT( scale ) );
}

void trap_R_Text_Paint( float x, float y, int font, float scale, const vec4_t color, const char *text, float adjust, int limit, int style ) {
	syscall( UI_R_TEXT_PAINT, PASSFLOAT( x ), PASSFLOAT( y ), font, PASSFLOAT( scale ), color, text, PASSFLOAT( adjust ), limit, style );
}

// cursor is a char
void trap_R_Text_PaintWithCursor( float x, float y, int font, float scale, const vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style ) {
	syscall( UI_R_TEXT_PAINTWITHCURSOR, PASSFLOAT( x ), PASSFLOAT( y ), font, PASSFLOAT( scale ), color, text, cursorPos, cursor, limit, style );
}

const char *trap_SE_TranslateReference( const char *reference ) {
	return (const char *)syscall( UI_SE_TRANSLATEREFERENCE, reference );
}

const char *trap_SE_LocalizeMessage( const char *msg, const char *reference ) {
	return (const char *)syscall( UI_SE_LOCALIZEMESSAGE, msg, reference );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name, int imageTrack ) {
	return syscall( UI_R_REGISTERSHADERNOMIP, name, imageTrack );
}

void trap_R_ClearScene( void ) {
	syscall( UI_R_CLEARSCENE );
}

void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( UI_R_ADDREFENTITYTOSCENE, re );
}

void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	syscall( UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

// no overdraw argument
void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	syscall( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ) );
}

// the engine ignores visible
void trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible ) {
	syscall( UI_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, visible );
}

void trap_R_RenderScene( const refdef_t *fd ) {
	syscall( UI_R_RENDERSCENE, fd );
}

void trap_R_SetColor( const float *rgba ) {
	syscall( UI_R_SETCOLOR, rgba );
}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( UI_R_DRAWSTRETCHPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader );
}

void trap_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( UI_R_MODELBOUNDS, model, mins, maxs );
}

void trap_UpdateScreen( void ) {
	syscall( UI_UPDATESCREEN );
}

// sample is a sound alias name
sfxHandle_t trap_S_RegisterSound( const char *sample ) {
	return syscall( UI_S_REGISTERSOUND, sample );
}

// plays a sound alias by name
void trap_S_StartLocalSound( const char *aliasName ) {
	syscall( UI_S_STARTLOCALSOUND, aliasName );
}

void trap_S_FadeAllSounds( float volume, int fadeTime ) {
	syscall( UI_S_FADEALLSOUNDS, PASSFLOAT( volume ), fadeTime );
}

void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	syscall( UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	syscall( UI_KEY_GETBINDINGBUF, keynum, buf, buflen );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	syscall( UI_KEY_SETBINDING, keynum, binding );
}

qboolean trap_Key_IsDown( int keynum ) {
	return syscall( UI_KEY_ISDOWN, keynum );
}

qboolean trap_Key_GetOverstrikeMode( void ) {
	return syscall( UI_KEY_GETOVERSTRIKEMODE );
}

void trap_Key_SetOverstrikeMode( qboolean state ) {
	syscall( UI_KEY_SETOVERSTRIKEMODE, state );
}

void trap_Key_ClearStates( void ) {
	syscall( UI_KEY_CLEARSTATES );
}

int trap_Key_GetCatcher( void ) {
	return syscall( UI_KEY_GETCATCHER );
}

void trap_Key_SetCatcher( int catcher ) {
	syscall( UI_KEY_SETCATCHER, catcher );
}

void trap_GetClipboardData( char *buf, int bufsize ) {
	syscall( UI_GETCLIPBOARDDATA, buf, bufsize );
}

void trap_GetClientState( uiClientState_t *state ) {
	syscall( UI_GETCLIENTSTATE, state );
}

void trap_GetGlconfig( glconfig_t *glconfig ) {
	syscall( UI_GETGLCONFIG, glconfig );
}

int trap_GetConfigString( int index, char *buff, int buffsize ) {
	return syscall( UI_GETCONFIGSTRING, index, buff, buffsize );
}

// copies clientNum's name from the current snapshot
int trap_GetClientName( int clientNum, char *buf, int buflen ) {
	return syscall( UI_GETCLIENTNAME, clientNum, buf, buflen );
}

// not handled by the engine
int trap_LAN_GetLocalServerCount( void ) {
	return syscall( UI_LAN_GETLOCALSERVERCOUNT );
}

// not handled by the engine
void trap_LAN_GetLocalServerAddressString( int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETLOCALSERVERADDRESSSTRING, n, buf, buflen );
}

// not handled by the engine
int trap_LAN_GetGlobalServerCount( void ) {
	return syscall( UI_LAN_GETGLOBALSERVERCOUNT );
}

// not handled by the engine
void trap_LAN_GetGlobalServerAddressString( int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETGLOBALSERVERADDRESSSTRING, n, buf, buflen );
}

int trap_LAN_GetPingQueueCount( void ) {
	return syscall( UI_LAN_GETPINGQUEUECOUNT );
}

void trap_LAN_ClearPing( int n ) {
	syscall( UI_LAN_CLEARPING, n );
}

void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	syscall( UI_LAN_GETPING, n, buf, buflen, pingtime );
}

void trap_LAN_GetPingInfo( int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETPINGINFO, n, buf, buflen );
}

qboolean trap_LAN_UpdateVisiblePings( int source ) {
	return syscall( UI_LAN_UPDATEVISIBLEPINGS, source );
}

int trap_LAN_GetServerCount( int source ) {
	return syscall( UI_LAN_GETSERVERCOUNT, source );
}

int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return syscall( UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
}

void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen );
}

void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETSERVERINFO, source, n, buf, buflen );
}

int trap_LAN_AddServer( int source, const char *name, const char *addr ) {
	return syscall( UI_LAN_ADDSERVER, source, name, addr );
}

void trap_LAN_RemoveServer( int source, const char *addr ) {
	syscall( UI_LAN_REMOVESERVER, source, addr );
}

// -1 when the slot is empty
int trap_LAN_GetServerPing( int source, int n ) {
	return syscall( UI_LAN_GETSERVERPING, source, n );
}

// returns the server's visible flag
int trap_LAN_ServerIsDirty( int source, int n ) {
	return syscall( UI_LAN_SERVERISDIRTY, source, n );
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return syscall( UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen );
}

void trap_LAN_LoadCachedServers( void ) {
	syscall( UI_LAN_LOADCACHEDSERVERS );
}

void trap_LAN_SaveCachedServers( void ) {
	syscall( UI_LAN_SAVECACHEDSERVERS );
}

// marks server n visible or not
void trap_ClearDisplayedServers( int source, int n, qboolean visible ) {
	syscall( UI_CLEARDISPLAYEDSERVERS, source, n, visible );
}

// the engine does nothing with this
void trap_SetPbClStatus( int status ) {
	syscall( UI_SET_PBCLSTATUS, status );
}

void trap_LAN_ResetPings( int n ) {
	syscall( UI_LAN_RESETPINGS, n );
}

int trap_MemoryRemaining( void ) {
	return syscall( UI_MEMORY_REMAINING );
}

// key and checksum
void trap_GetCDKey( char *buf, int buflen, char *checksum, int checksumlen ) {
	syscall( UI_GETCDKEY, buf, buflen, checksum, checksumlen );
}

// key and checksum
void trap_SetCDKey( const char *key, const char *checksum ) {
	syscall( UI_SETCDKEY, key, checksum );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return syscall( UI_PC_ADD_GLOBAL_DEFINE, define );
}

int trap_PC_LoadSource( const char *filename ) {
	return syscall( UI_PC_LOADSOURCE, filename );
}

int trap_PC_FreeSource( int handle ) {
	return syscall( UI_PC_FREESOURCE, handle );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return syscall( UI_PC_READTOKEN, handle, pc_token );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return syscall( UI_PC_SOURCEFILEANDLINE, handle, filename, line );
}

int trap_RealTime( qtime_t *qtime ) {
	return syscall( UI_REALTIME, qtime );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return syscall( UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits );
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic( int handle ) {
	return syscall( UI_CIN_STOPCINEMATIC, handle );
}

// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic( int handle ) {
	return syscall( UI_CIN_RUNCINEMATIC, handle );
}

// draws the current frame
void trap_CIN_DrawCinematic( int handle ) {
	syscall( UI_CIN_DRAWCINEMATIC, handle );
}

// allows you to resize the animation dynamically
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	syscall( UI_CIN_SETEXTENTS, handle, x, y, w, h );
}

void *trap_Z_MallocInternal( int size ) {
	return (void *)syscall( UI_Z_MALLOCINTERNAL, size );
}

void trap_Z_FreeInternal( void *ptr ) {
	syscall( UI_Z_FREEINTERNAL, ptr );
}

qboolean trap_VerifyCDKey( const char *key, const char *chksum ) {
	return syscall( UI_VERIFYCDKEY, key, chksum );
}

// trap_GetAutoUpdate is installed as DC->runningGame: qtrue while a game is active
void trap_CheckAutoUpdate( void ) {
	syscall( UI_CHECKAUTOUPDATE );
}

qboolean trap_GetAutoUpdate( void ) {
	return syscall( UI_GETAUTOUPDATE );
}
