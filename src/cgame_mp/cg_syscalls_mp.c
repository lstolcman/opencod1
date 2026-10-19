// cg_syscalls.c -- this file is only included when building a dll

#include "cg_local.h"

static int ( QDECL * syscall )( int arg, ... ) = ( int ( QDECL * )( int, ... ) ) - 1;

void dllEntry( int ( QDECL *syscallptr )( int arg,... ) ) {
	syscall = syscallptr;
}

int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void trap_Print( const char *fmt ) {
	syscall( CG_PRINT, fmt );
}

void trap_Error( const char *fmt ) {
	syscall( CG_ERROR, fmt );
}

void trap_GameMessage( const char *msg, int width ) {
	syscall( CG_GAMEMESSAGE, msg, width );
}

void trap_BoldGameMessage( const char *msg, int width ) {
	syscall( CG_BOLDGAMEMESSAGE, msg, width );
}

void trap_DeathMessage( const char *attacker, const float *attackerColor, const char *victim, const float *victimColor, const char *icon, float iconWidth, float iconHeight, const float *iconColor ) {
	syscall( CG_DEATHMESSAGE, attacker, attackerColor, victim, victimColor, icon, PASSFLOAT( iconWidth ), PASSFLOAT( iconHeight ), iconColor );
}

void trap_Subtitle( const char *msg, int duration, int channel ) {
	syscall( CG_SUBTITLE, msg, duration, channel );
}

int trap_Milliseconds( void ) {
	return syscall( CG_MILLISECONDS );
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	syscall( CG_CVAR_REGISTER, cvar, var_name, value, flags );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	syscall( CG_CVAR_UPDATE, cvar );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( CG_CVAR_SET, var_name, value );
}

void trap_Cvar_Setvar( vmCvar_t *cvar, const char *var_name ) {
	syscall( CG_CVAR_SETVAR, cvar, var_name );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( CG_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

int trap_Argc( void ) {
	return syscall( CG_ARGC );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( CG_ARGV, n, buffer, bufferLength );
}

void trap_Args( char *buffer, int bufferLength ) {
	syscall( CG_ARGS, buffer, bufferLength );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( CG_FS_FOPENFILE, qpath, f, mode );
}

int trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	return syscall( CG_FS_READ, buffer, len, f );
}

int trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	return syscall( CG_FS_WRITE, buffer, len, f );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( CG_FS_FCLOSEFILE, f );
}

int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return syscall( CG_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

int trap_Com_SaveCvarsToBuffer( const char **cvars, int numCvars, char *buffer, int size ) {
	return syscall( CG_COM_SAVECVARSTOBUFFER, cvars, numCvars, buffer, size );
}

int trap_Com_LoadCvarsFromBuffer( const char **cvars, int numCvars, char *buffer, const char *text ) {
	return syscall( CG_COM_LOADCVARSFROMBUFFER, cvars, numCvars, buffer, text );
}

void trap_SendConsoleCommand( const char *text ) {
	syscall( CG_SENDCONSOLECOMMAND, text );
}

void trap_AddCommand( const char *cmdName ) {
	syscall( CG_ADDCOMMAND, cmdName );
}

void trap_SendClientCommand( const char *s ) {
	syscall( CG_SENDCLIENTCOMMAND, s );
}

void trap_UpdateScreen( void ) {
	syscall( CG_UPDATESCREEN );
}

// alpha fades the text; orientation selects how the lines stack
void trap_DrawNotifyLines( int x, int yStart, float alpha, int orientation ) {
	syscall( CG_DRAWNOTIFYLINES, x, yStart, PASSFLOAT( alpha ), orientation );
}

void trap_DrawBoldMessages( int x, int yStart, float alpha, int orientation ) {
	syscall( CG_DRAWBOLDMESSAGES, x, yStart, PASSFLOAT( alpha ), orientation );
}

void trap_DrawMiniConsole( int x, int yStart, float alpha ) {
	syscall( CG_DRAWMINICONSOLE, x, yStart, PASSFLOAT( alpha ) );
}

void trap_DrawSubtitles( int x, int yStart, float alpha, int orientation ) {
	syscall( CG_DRAWSUBTITLES, x, yStart, PASSFLOAT( alpha ), orientation );
}

void trap_DrawSay( int y ) {
	syscall( CG_DRAWSAY, y );
}

void trap_CM_LoadMap( const char *mapname ) {
	syscall( CG_CM_LOADMAP, mapname );
}

int trap_CM_NumInlineModels( void ) {
	return syscall( CG_CM_NUMINLINEMODELS );
}

int trap_CM_InlineModel( int index ) {
	return syscall( CG_CM_INLINEMODEL, index );
}

// keep: the original passes only mins and maxs -- the engine reads a
// contents argument that is never pushed, so `contents` goes unused
int trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int contents ) {
	return syscall( CG_CM_TEMPBOXMODEL, mins, maxs );
}

int trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs, int contents ) {
	return syscall( CG_CM_TEMPCAPSULEMODEL, mins, maxs );
}

int trap_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return syscall( CG_CM_POINTCONTENTS, p, model );
}

int trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return syscall( CG_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles );
}

void trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask ) {
	syscall( CG_CM_BOXTRACE, results, start, end, mins, maxs, model, brushmask );
}

void trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles ) {
	syscall( CG_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs, model, brushmask, origin, angles );
}

void trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask ) {
	syscall( CG_CM_CAPSULETRACE, results, start, end, mins, maxs, model, brushmask );
}

void trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles ) {
	syscall( CG_CM_TRANSFORMEDCAPSULETRACE, results, start, end, mins, maxs, model, brushmask, origin, angles );
}

// fills in both texture coordinate pairs of the poly verts, and the
// shader of the surface each fragment was clipped from
int trap_R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
						  const vec3_t normal, float radius, int maxPoints, polyVert_t *pointBuffer,
						  int maxFragments, markFragment_t *fragmentBuffer, qhandle_t markShader ) {
	return syscall( CG_R_MARKFRAGMENTS, numPoints, points, projection, normal, PASSFLOAT( radius ),
					maxPoints, pointBuffer, maxFragments, fragmentBuffer, markShader );
}

void trap_R_LoadWorldMap( const char *mapname ) {
	syscall( CG_R_LOADWORLDMAP, mapname );
}

void trap_R_FinishLoadingModels( void ) {
	syscall( CG_R_FINISHLOADINGMODELS );
}

void trap_R_SetIgnorePrecacheErrors( int ignore ) {
	syscall( CG_R_SETIGNOREPRECACHEERRORS, ignore );
}

qhandle_t trap_R_RegisterModel( const char *name, int arg2 ) {
	CG_DrawInformation( 0 );
	return syscall( CG_R_REGISTERMODEL, name, arg2 );
}

int trap_R_GetShaderFromModel( int modelHandle, int surfaceIndex ) {
	return syscall( CG_R_GETSHADERFROMMODEL, modelHandle, surfaceIndex );
}

void *trap_R_GetXModelByHandle( qhandle_t hModel ) {
	return (void *)syscall( CG_R_GETXMODELBYHANDLE, hModel );
}

qhandle_t trap_R_RegisterShader( const char *name, int arg2 ) {
	CG_DrawInformation( 0 );
	return syscall( CG_R_REGISTERSHADER, name, arg2 );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name, int arg2 ) {
	CG_DrawInformation( 0 );
	return syscall( CG_R_REGISTERSHADERNOMIP, name, arg2 );
}

int trap_R_RegisterFont( const char *name, int pointSize, int font, int imageTrack ) {
	return syscall( CG_R_REGISTERFONT, name, pointSize, font, imageTrack );
}

int trap_R_Text_Width( const char *text, int font, float scale, int limit ) {
	return syscall( CG_R_TEXT_WIDTH, text, font, PASSFLOAT( scale ), limit );
}

int trap_R_Text_Height( int font, float scale ) {
	return syscall( CG_R_TEXT_HEIGHT, font, PASSFLOAT( scale ) );
}

void trap_R_Text_Paint( float x, float y, int font, float scale, const float *color, const char *text, float adjust, int limit, int style ) {
	syscall( CG_R_TEXT_PAINT, PASSFLOAT( x ), PASSFLOAT( y ), font, PASSFLOAT( scale ), color, text, PASSFLOAT( adjust ), limit, style );
}

void trap_R_Text_PaintWithCursor( float x, float y, int font, float scale, const float *color, const char *text, int cursorPos, char cursor, int limit, int style ) {
	syscall( CG_R_TEXT_PAINTWITHCURSOR, PASSFLOAT( x ), PASSFLOAT( y ), font, PASSFLOAT( scale ), color, text, cursorPos, cursor, limit, style );
}

const char *trap_SE_TranslateReference( const char *reference ) {
	return (const char *)syscall( CG_SE_TRANSLATEREFERENCE, reference );
}

const char *trap_SE_LocalizeMessage( const char *msg, const char *reference ) {
	return (const char *)syscall( CG_SE_LOCALIZEMESSAGE, msg, reference );
}

int trap_SE_PrintStrlen( const char *text ) {
	return syscall( CG_SE_PRINTSTRLEN, text );
}

int trap_SE_ReadCharFromString( const unsigned char **text, int *advance ) {
	return syscall( CG_SE_READCHARFROMSTRING, text, advance );
}

void trap_R_ClearScene( void ) {
	syscall( CG_R_CLEARSCENE );
}

void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( CG_R_ADDREFENTITYTOSCENE, re );
}

void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	syscall( CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) {
	syscall( CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, numPolys );
}

void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	syscall( CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ) );
}

void trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int visible ) {
	syscall( CG_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, visible );
}

void trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density ) {
	syscall( CG_R_SETFOG, fogvar, var1, var2, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( density ) );
}

void trap_R_RenderScene( const refdef_t *fd ) {
	syscall( CG_R_RENDERSCENE, fd );
}

void trap_R_SaveScreen( void ) {
	syscall( CG_R_SAVESCREEN );
}

void trap_R_BlendSavedScreen( int blend ) {
	syscall( CG_R_BLENDSAVEDSCREEN, blend );
}

void trap_R_SetColor( const float *rgba ) {
	syscall( CG_R_SETCOLOR, rgba );
}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( CG_R_DRAWSTRETCHPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader );
}

void trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader, const float *gradientColor, int gradientType ) {
	syscall( CG_R_DRAWSTRETCHPICGRADIENT, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, gradientColor, gradientType );
}

void trap_R_DrawStretchPicRotate( float x, float y, float w, float h, float s1, float t1, float s2, float t2, float angle, int hShader ) {
	syscall( CG_R_DRAWSTRETCHPICROTATE, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), PASSFLOAT( angle ), hShader );
}

// four ( x, y ) corners and their four ( s, t ) pairs
void trap_R_DrawQuadPic( const float *verts, const float *texCoords, int hShader ) {
	syscall( CG_R_DRAWQUADPIC, verts, texCoords, hShader );
}

void trap_R_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( CG_R_MODELBOUNDS, model, mins, maxs );
}

// the renderer writes its frame counters into *stats; NULL stops it
void trap_R_TrackStatistics( void *stats ) {
	syscall( CG_R_TRACKSTATISTICS, stats );
}

// traces from start along dir and describes what it hits in the three buffers
int trap_R_PickShader( const float *start, const float *dir, char *shaderName, char *surfaceNames, char *contentsNames, int size ) {
	return syscall( CG_R_PICKSHADER, start, dir, shaderName, surfaceNames, contentsNames, size );
}

void trap_GetGlconfig( glconfig_t *vidConfig ) {
	syscall( CG_GETGLCONFIG, vidConfig );
}

void trap_GetGameState( gameState_t *gamestate ) {
	syscall( CG_GETGAMESTATE, gamestate );
}

void trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	syscall( CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime );
}

int trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return syscall( CG_GETSNAPSHOT, snapshotNumber, snapshot );
}

int trap_GetServerCommand( int serverCommandNumber ) {
	return syscall( CG_GETSERVERCOMMAND, serverCommandNumber );
}

int trap_GetCurrentCmdNumber( void ) {
	return syscall( CG_GETCURRENTCMDNUMBER );
}

int trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return syscall( CG_GETUSERCMD, cmdNumber, ucmd );
}

void trap_SetUserCmdAimValues( const vec3_t aim ) {
	syscall( CG_SETUSERCMDAIMVALUES, aim );
}

void trap_SetUserCmdInShellshock( int inShellshock ) {
	syscall( CG_SETUSERCMDINSHELLSHOCK, inShellshock );
}

void trap_PrintInt( const char *s, int i ) {
	syscall( CG_PRINT_INT, s, i );
}

void trap_PrintFloat( const char *s, float f ) {
	syscall( CG_PRINT_FLOAT, s, PASSFLOAT( f ) );
}

int trap_MemoryRemaining( void ) {
	return syscall( CG_MEMORYREMAINING );
}

int trap_Key_IsDown( int keynum ) {
	return syscall( CG_KEY_ISDOWN, keynum );
}

int trap_Key_GetCatcher( void ) {
	return syscall( CG_KEY_GETCATCHER );
}

void trap_Key_SetCatcher( int catcher ) {
	syscall( CG_KEY_SETCATCHER, catcher );
}

int trap_Key_GetKey( const char *binding ) {
	return syscall( CG_KEY_GETKEY, binding );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return syscall( CG_PC_ADDGLOBALDEFINE, define );
}

int trap_PC_LoadSource( const char *filename ) {
	return syscall( CG_PC_LOADSOURCE, filename );
}

int trap_PC_FreeSource( int handle ) {
	return syscall( CG_PC_FREESOURCE, handle );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return syscall( CG_PC_READTOKEN, handle, pc_token );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return syscall( CG_PC_SOURCEFILEANDLINE, handle, filename, line );
}

void trap_Com_LoadSoundAliases( const char *mapname ) {
	syscall( CG_COM_LOADSOUNDALIASES, mapname );
}

int trap_Com_SoundAliasString( const char *alias ) {
	return syscall( CG_COM_SOUNDALIASSTRING, alias );
}

const char *trap_Com_PickSoundAlias( const char *alias ) {
	return (const char *)syscall( CG_COM_PICKSOUNDALIAS, alias );
}

const char *trap_Com_GetSoundAlias( int index ) {
	return (const char *)syscall( CG_COM_GETSOUNDALIAS, index );
}

int trap_MSS_PlaySoundAlias( const char *alias, int entnum, const float *origin, int timeOffset ) {
	return syscall( CG_MSS_PLAYSOUNDALIAS, alias, entnum, origin, timeOffset );
}

void trap_MSS_PlayBlendedSoundAliases( const char *alias1, const char *alias2, float blend, int entnum, const float *origin, int timeOffset ) {
	syscall( CG_MSS_PLAYBLENDEDSOUNDALIASES, alias1, alias2, PASSFLOAT( blend ), entnum, origin, timeOffset );
}

int trap_SurfaceTypeFromName( const char *name ) {
	return syscall( CG_SURFACETYPEFROMNAME, name );
}

const char *trap_SurfaceTypeToName( int surfaceType ) {
	return (const char *)syscall( CG_SURFACETYPETONAME, surfaceType );
}

// qtime is a qtime_t
int trap_RealTime( int *qtime ) {
	return syscall( CG_REALTIME, qtime );
}

void trap_SnapVector( float *v ) {
	syscall( CG_SNAPVECTOR, v );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return syscall( CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits );
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
int trap_CIN_StopCinematic( int handle ) {
	return syscall( CG_CIN_STOPCINEMATIC, handle );
}

// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
int trap_CIN_RunCinematic( int handle ) {
	return syscall( CG_CIN_RUNCINEMATIC, handle );
}

// draws the current frame
void trap_CIN_DrawCinematic( int handle ) {
	syscall( CG_CIN_DRAWCINEMATIC, handle );
}

// allows you to resize the animation dynamically
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	syscall( CG_CIN_SETEXTENTS, handle, x, y, w, h );
}

int trap_R_GetEntityToken( char *buffer, int bufferSize ) {
	return syscall( CG_R_GETENTITYTOKEN, buffer, bufferSize );
}

int trap_hunkUsed( void ) {
	return syscall( CG_HUNKUSED );
}

int trap_UI_LoadMenu( const char *menuname ) {
	return syscall( CG_UI_LOADMENU, menuname );
}

// bring up a popup menu
int trap_UI_Popup( const char *menuname ) {
	return syscall( CG_UI_POPUP, menuname );
}

void trap_UI_ClosePopup( const char *menuname ) {
	syscall( CG_UI_CLOSEPOPUP, menuname );
}

void trap_UI_CloseAllMenus( void ) {
	syscall( CG_UI_CLOSEALLMENUS );
}

const char *trap_UI_GetMapDisplayName( const char *mapname ) {
	return (const char *)syscall( CG_UI_GETMAPDISPLAYNAME, mapname );
}

const char *trap_UI_GetGameTypeDisplayName( const char *gametype ) {
	return (const char *)syscall( CG_UI_GETGAMETYPEDISPLAYNAME, gametype );
}

const char *trap_CL_GetServerIPAddress( void ) {
	return (const char *)syscall( CG_CL_GETSERVERIPADDRESS );
}

void trap_XAnimPrecache( const char *animName ) {
	syscall( CG_XANIMPRECACHE, animName );
}

void *trap_XAnimCreateAnims( const char *name, int size ) {
	return (void *)syscall( CG_XANIMCREATEANIMS, name, size );
}

void trap_XAnimCreate( void *anims, unsigned short animIndex, const char *animName ) {
	syscall( CG_XANIMCREATE, anims, animIndex, animName );
}

void *trap_XAnimCreateTree( void *anims ) {
	return (void *)syscall( CG_XANIMCREATETREE, anims );
}

void trap_XAnimBlend( void *anims, int animIndex, const char *animName, unsigned short firstChild, unsigned short numChildren, unsigned short flags ) {
	syscall( CG_XANIMBLEND, anims, (unsigned short)animIndex, animName, firstChild, numChildren, flags );
}

void trap_DObjFree( DObj *obj ) {
	syscall( CG_DOBJFREE, obj );
}

// keep: anim indexes are narrowed to unsigned short, as in the original
void trap_XAnimClearGoalWeight( void *tree, int animIndex, float goalTime ) {
	syscall( CG_XANIMCLEARGOALWEIGHT, tree, (unsigned short)animIndex, PASSFLOAT( goalTime ) );
}

void trap_XAnimClearTreeGoalWeights( void *tree, int animIndex, float goalWeight ) {
	syscall( CG_XANIMCLEARTREEGOALWEIGHTS, tree, (unsigned short)animIndex, PASSFLOAT( goalWeight ) );
}

void trap_XAnimClearTreeGoalWeightsStrict( void *tree, scr_anim_t anim, float goalWeight ) {
	syscall( CG_XANIMCLEARTREEGOALWEIGHTSSTRICT, tree, anim.index, PASSFLOAT( goalWeight ) );
}

void trap_XAnimSetCompleteGoalWeightKnob( void *tree, unsigned short animIndex, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags ) {
	syscall( CG_XANIMSETANIMKNOB, tree, animIndex, PASSFLOAT( goalWeight ), PASSFLOAT( goalTime ), PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

int trap_XAnimSetCompleteGoalWeightKnobAll( void *tree, scr_anim_t anim, scr_anim_t rootAnim, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags ) {
	return syscall( CG_XANIMSETCOMPLETEGOALWEIGHTKNOBALL, tree, anim.index, rootAnim.index, PASSFLOAT( goalWeight ), PASSFLOAT( goalTime ), PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void trap_XAnimClearAnimChildren( void *tree, scr_anim_t anim, float goalTime ) {
	syscall( CG_XANIMCLEARANIMCHILDREN, tree, anim.index, PASSFLOAT( goalTime ) );
}

void trap_XAnimSetGoalWeight( void *tree, unsigned short animIndex, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags ) {
	syscall( CG_XANIMSETANIMLIMITED, tree, animIndex, PASSFLOAT( goalWeight ), PASSFLOAT( goalTime ), PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void trap_XAnimSetCompleteGoalWeight( void *tree, scr_anim_t anim, float goalWeight, float goalTime, float rate, unsigned short notifyIndex, int notifyFlags ) {
	syscall( CG_XANIMSETCOMPLETEGOALWEIGHT, tree, anim.index, PASSFLOAT( goalWeight ), PASSFLOAT( goalTime ), PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void trap_XAnimSetAnimRate( void *tree, scr_anim_t anim, float rate ) {
	syscall( CG_XANIMSETANIMRATE, tree, anim.index, PASSFLOAT( rate ) );
}

int trap_XAnimIsLooped( scr_anim_t anim ) {
	return syscall( CG_XANIMISLOOPED, anim.anims, anim.index );
}

int trap_XAnimIsLoopedIndex( void *anims, int animIndex ) {
	return syscall( CG_XANIMISLOOPEDINDEX, anims, (unsigned short)animIndex );
}

void trap_XAnimSetTime( void *tree, int animIndex, float time ) {
	syscall( CG_XANIMSETTIME, tree, (unsigned short)animIndex, PASSFLOAT( time ) );
}

float trap_XAnimGetTime( void *tree, scr_anim_t anim ) {
	int temp;
	temp = syscall( CG_XANIMGETTIME, tree, anim.index );
	return *(float *)&temp;
}

float trap_XAnimGetWeight( void *tree, scr_anim_t anim ) {
	int temp;
	temp = syscall( CG_XANIMGETWEIGHT, tree, anim.index );
	return *(float *)&temp;
}

void trap_DObjInvalidateSkels( void ) {
	syscall( CG_DOBJINVALIDATESKELS );
}

void trap_DObjUpdateClientInfo( DObj *obj, float dtime ) {
	syscall( CG_DOBJUPDATECLIENTINFO, obj, PASSFLOAT( dtime ) );
}

// returns the notify count; the notifies are stored through notifiesOut
int trap_XAnimGetDeferredNotifies( void *notifiesOut ) {
	return syscall( CG_XANIMGETDEFERREDNOTIFIES, notifiesOut );
}

void trap_DObjCalcAnim( DObj *obj, const int *partBits ) {
	syscall( CG_DOBJCALCANIM, obj, partBits );
}

void trap_DObjDisplayAnim( DObj *obj ) {
	syscall( CG_DOBJDISPLAYANIM, obj );
}

void trap_XAnimCalcDelta( void *tree, unsigned short animIndex, float *deltaRot, float *deltaMove, int animIndexEnd ) {
	syscall( CG_XANIMCALCRELDELTA, tree, animIndex, deltaRot, deltaMove, animIndexEnd );
}

void trap_XAnimCalcAbsDelta( void *tree, int animIndex, float *deltaRot, float *deltaMove ) {
	syscall( CG_XANIMCALCABSDELTA, tree, (unsigned short)animIndex, deltaRot, deltaMove );
}

void trap_XAnimGetRelDelta( scr_anim_t anim, float *deltaRot, float *deltaMove,
							float startTime, float endTime ) {
	syscall( CG_XANIMGETRELDELTA, anim.anims, anim.index, deltaRot, deltaMove,
			 PASSFLOAT( startTime ), PASSFLOAT( endTime ) );
}

void trap_XAnimGetAbsDelta( scr_anim_t anim, float *deltaRot, float *deltaMove, float time ) {
	syscall( CG_XANIMGETABSDELTA, anim.anims, anim.index, deltaRot, deltaMove, PASSFLOAT( time ) );
}

void *trap_DObjGetMatrixArray( DObj *obj ) {
	return (void *)syscall( CG_DOBJGETMATRIXARRAY, obj, 0 );
}

void *trap_DObjGetRotTransArray( DObj *obj ) {
	return (void *)syscall( CG_DOBJGETROTTRANSARRAY, obj );
}

int trap_DObjSetRotTransIndex( DObj *obj, const int *partBits, int boneIndex ) {
	return syscall( CG_DOBJSETROTTRANSINDEX, obj, partBits, boneIndex );
}

int trap_DObjSetControlRotTransIndex( DObj *obj, const int *partBits, int boneIndex ) {
	return syscall( CG_DOBJSETCONTROLROTTRANSINDEX, obj, partBits, boneIndex );
}

const char *trap_XAnimGetAnimName( scr_anim_t anim ) {
	return (const char *)syscall( CG_XANIMGETANIMNAME, anim.anims, anim.index );
}

DObj *trap_GetClientDObj( int handle ) {
	return (DObj *)syscall( CG_GETCLIENTDOBJ, handle );
}

void trap_DObjCreateDirect( void *models, unsigned short numModels, void *tree, DObj *obj ) {
	syscall( CG_DOBJCREATEDIRECT, models, numModels, tree, obj );
}

// unlike the game's trap_DObjCreate, the engine supplies the notify id
void trap_DObjCreate( void *models, unsigned short modelCount, void *tree, int handle ) {
	syscall( CG_DOBJCREATE, models, modelCount, tree, handle );
}

void trap_SafeDObjFree( int handle, int localClientNum ) {
	syscall( CG_SAFEDOBJFREE, handle, localClientNum );
}

int trap_XAnimGetAnims( const int *p ) {
	return syscall( CG_DEREF_DWORD, p );
}

int trap_DObjGetSkelSize( DObj *obj ) {
	return syscall( CG_DOBJGETSKELSIZE, obj );
}

int trap_DObjCreateSkelForBone( DObj *obj, int boneIndex ) {
	return syscall( CG_DOBJCREATESKELFORBONE, obj, boneIndex );
}

int trap_DObjCreateSkelForBones( DObj *obj, const int *partBits ) {
	return syscall( CG_DOBJCREATESKELFORBONES, obj, partBits );
}

void trap_DObjGetHierarchyBits( DObj *obj, int boneIndex, int *partBits ) {
	syscall( CG_DOBJGETHIERARCHYBITS, obj, boneIndex, partBits );
}

void trap_DObjCalcSkel( DObj *obj, const int *partBits ) {
	syscall( CG_DOBJCALCSKEL, obj, partBits );
}

int trap_XModelExists( const char *modelName ) {
	return syscall( CG_XMODELEXISTS, modelName );
}

int trap_DObjNumBones( DObj *obj ) {
	return syscall( CG_DOBJNUMPARTS, obj );
}

int trap_DObjGetBoneIndex( DObj *obj, const char *tagName ) {
	return syscall( CG_DOBJGETBONEINDEX, obj, tagName );
}

const char *trap_DObjGetBoneName( DObj *obj, int boneIndex ) {
	return (const char *)syscall( CG_DOBJGETBONENAME, obj, boneIndex );
}

void trap_DObjBuildPartCollisionTable( DObj *obj, float **partBounds ) {
	syscall( CG_DOBJBUILDPARTCOLLISIONTABLE, obj, partBounds );
}

void *trap_DObjGetTree( DObj *obj ) {
	return (void *)syscall( CG_DOBJGETTREE, obj );
}

// true if the anim has no children
int trap_XAnimIsPrimitive( scr_anim_t anim ) {
	return syscall( CG_XANIMISPRIMITIVE, anim.anims, anim.index );
}

// anim length in msec
int trap_XAnimGetLength( void *tree, unsigned short animIndex ) {
	return syscall( CG_XANIMGETLENGTH, tree, animIndex );
}

int trap_XAnimHasFinished( void *tree, int animIndex ) {
	return syscall( CG_XANIMHASTIME, tree, (unsigned short)animIndex );
}

int trap_XAnimGetNumChildren( scr_anim_t anim ) {
	return syscall( CG_XANIMGETNUMCHILDREN, anim.anims, anim.index );
}

scr_anim_t trap_XAnimGetChildAt( scr_anim_t anim, int childIndex ) {
	anim.index = (unsigned short)syscall( CG_XANIMGETCHILDAT, anim.anims, anim.index, childIndex );
	return anim;
}

int trap_XAnimGetAnimTreeSize( void *tree ) {
	return syscall( CG_XANIMGETANIMTREESIZE, tree );
}

void trap_XAnimCloneAnimTree( const void *from, void *to ) {
	syscall( CG_XANIMCOPYTREE, from, to );
}

void trap_DObjDumpInfo( DObj *obj ) {
	syscall( CG_DOBJDUMPINFO, obj );
}

void trap_StatMon_Warning( int type, int duration, const char *material ) {
	syscall( CG_STATMON_WARNING, type, duration, material );
}

// stores the engine's warning array through statsOut
void trap_StatMon_GetStatsArray( void *statsOut, int *countOut ) {
	syscall( CG_STATMON_GETSTATSARRAY, statsOut, countOut );
}

void *trap_Z_MallocInternal( int size ) {
	return (void *)syscall( CG_Z_MALLOCINTERNAL, size );
}

void trap_Z_FreeInternal( void *ptr ) {
	syscall( CG_Z_FREEINTERNAL, ptr );
}

void trap_AddDebugLine( const vec3_t start, const vec3_t end, const vec3_t color,
						int depthTest, int duration ) {
	syscall( CG_ADD_DEBUG_LINE, start, end, color, depthTest, duration );
}

void *trap_GetWeaponInfoMemory( int size, int *pPrevOwner ) {
	return (void *)syscall( CG_GETWEAPONINFOMEMORY, size, pPrevOwner );
}

void trap_FreeWeaponInfoMemory( int slot ) {
	syscall( CG_FREEWEAPONINFOMEMORY, slot );
}

// 32 byte aligned
void *trap_Hunk_AllocInternal( int size ) {
	return (void *)syscall( CG_HUNK_ALLOCINTERNAL, size );
}

// 32 byte aligned
void *trap_Hunk_AllocLowInternal( int size ) {
	return (void *)syscall( CG_HUNKALLOCLOWINTERNAL, size );
}

void *trap_Hunk_AllocAlignInternal( int size, int alignment ) {
	return (void *)syscall( CG_HUNK_ALLOCALIGNINTERNAL, size, alignment );
}

void *trap_Hunk_AllocLowAlignInternal( int size, int align ) {
	return (void *)syscall( CG_HUNKALLOCLOWALIGNINTERNAL, size, align );
}

// stubbed out in the engine
int trap_Scr_GetNumVars( void ) {
	return syscall( CG_SCR_GETNUMVARS );
}

// stubbed out in the engine
int trap_Scr_GetNumThreads( void ) {
	return syscall( CG_SCR_GETNUMTHREADS );
}

int trap_Scr_GetStringUsage( void ) {
	return syscall( CG_SCR_GETSTRINGUSAGE );
}

void trap_MSS_SetListener( int clientNum, const vec3_t origin, vec3_t axis[3] ) {
	syscall( CG_MSS_SETLISTENER, clientNum, origin, axis );
}

void trap_MSS_UpdateLoopingSounds( void ) {
	syscall( CG_MSS_UPDATELOOPINGSOUNDS );
}

void trap_MSS_StopSounds( int flags ) {
	syscall( CG_MSS_STOPSOUNDS, flags );
}

// `alias` is what trap_Com_PickSoundAlias returns
void trap_MSS_PlayMusicAlias( const char *alias ) {
	syscall( CG_MSS_PLAYMUSICALIAS, alias );
}

void trap_MSS_StopBackground( int fadeTime ) {
	syscall( CG_MSS_STOPBACKGROUND, fadeTime );
}

void trap_MSS_PlayAmbientAlias( const char *alias, int fadeTime ) {
	syscall( CG_MSS_PLAYAMBIENTALIAS, alias, fadeTime );
}

void trap_MSS_FadeAllSounds( float volume, int fadeTime ) {
	syscall( CG_MSS_FADEALLSOUNDS, PASSFLOAT( volume ), fadeTime );
}

void trap_MSS_FadeSelectSounds( const float *channelVolumes, int fadeTime ) {
	syscall( CG_MSS_FADESELECTSOUNDS, channelVolumes, fadeTime );
}

void trap_MSS_SetEnvironmentEffects( const char *roomType, float dryLevel, int fadeTime ) {
	syscall( CG_MSS_SETENVIRONMENTEFFECTS, roomType, PASSFLOAT( dryLevel ), fadeTime );
}

int trap_MSS_GetSoundOverlay( int mode, void *channels, int maxChannels, int *cpuOut ) {
	return syscall( CG_MSS_GETSOUNDOVERLAY, mode, channels, maxChannels, cpuOut );
}

void trap_SetUserCmdValue( int userCmdValue, float sensitivityScale ) {
	syscall( CG_SETUSERCMDVALUE, userCmdValue, PASSFLOAT( sensitivityScale ) );
}

void trap_SetClientLerpOrigin( float x, float y, float z ) {
	syscall( CG_SETCLIENTLERPORIGIN, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( z ) );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	syscall( CG_KEY_GETBINDINGBUF, keynum, buf, buflen );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	syscall( CG_KEY_SETBINDING, keynum, binding );
}

void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	syscall( CG_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

int trap_FX_RegisterEffect( const char *name ) {
	return syscall( CG_FX_REGISTEREFFECT, name );
}

int trap_FX_GetBoneIndex( int entityNum, const char *boneName ) {
	return syscall( CG_FX_GETBONEINDEX, entityNum, boneName );
}

void trap_FX_PlaySimpleEffect( const char *name, const vec3_t origin ) {
	syscall( CG_FX_PLAYSIMPLEEFFECT, name, origin );
}

void trap_FX_PlayEffect( const char *name, const vec3_t origin, const vec3_t dir ) {
	syscall( CG_FX_PLAYEFFECT, name, origin, dir );
}

void trap_FX_PlayBoltedEffect( const char *name, const vec3_t origin, int boltInfo, int entityNum ) {
	syscall( CG_FX_PLAYBOLTEDEFFECT, name, origin, boltInfo, entityNum );
}

void trap_FX_PlaySimpleEffectID( int id, const vec3_t origin ) {
	syscall( CG_FX_PLAYSIMPLEEFFECTID, id, origin );
}

void trap_FX_PlayEffectID( int id, const vec3_t origin, const vec3_t dir ) {
	syscall( CG_FX_PLAYEFFECTID, id, origin, dir );
}

// boltInfo is { entity number, bone index }; every caller passes a NULL axis
void trap_FX_PlayOrientedEffectID( int id, const vec3_t origin, vec3_t axis[3], const int *boltInfo ) {
	syscall( CG_FX_PLAYORIENTEDEFFECTID, id, origin, axis, boltInfo );
}

void trap_FX_AddScheduledEffects( void ) {
	syscall( CG_FX_ADDSCHEDULEDEFFECTS );
}

int trap_FX_Init( void ) {
	return syscall( CG_FX_INIT );
}

int trap_FX_FreeAll( void ) {
	return syscall( CG_FX_FREEALL );
}

int trap_FX_FreeEffects( void ) {
	return syscall( CG_FX_FREEEFFECTS );
}

void trap_FX_AdjustTime( int time ) {
	syscall( CG_FX_ADJUSTTIME, time );
}

void trap_FX_AdjustCamera( const refdef_t *refdef ) {
	syscall( CG_FX_ADJUSTCAMERA, refdef );
}

// the engine has no case for traps 236-239
int trap_Unused236( int arg ) {
	return syscall( CG_UNUSED_236, arg );
}

int trap_Unused237( int arg ) {
	return syscall( CG_UNUSED_237, arg );
}

int trap_Unused238( int arg ) {
	return syscall( CG_UNUSED_238, arg );
}

int trap_Unused239( int arg ) {
	return syscall( CG_UNUSED_239, arg );
}

void trap_SetMouseMaxSpeeds( float maxPitchSpeed, float maxYawSpeed ) {
	syscall( CG_SETMOUSEMAXSPEEDS, PASSFLOAT( maxPitchSpeed ), PASSFLOAT( maxYawSpeed ) );
}

void trap_CL_FirstSnapshot( void ) {
	syscall( CG_CL_FIRSTSNAPSHOT );
}

