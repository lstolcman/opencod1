/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "cl_records.h"
#include "cl_local.h"
#include "../botlib/l_script.h"
#include <string.h>
#include "cl_refapi.h"
#include "cl_vm.h"
#include "cl_conwindows.h"
#include "../universal/com_sndalias.h"


vm_t    *cgvm = NULL;               /* retail 0x01617348 */

/* 0x015CA590, 160 bytes: cls.glconfig (cls is defined in cl_main_mp.c). */

extern signed __int32 cl_serverTime;   /* 0x01434A64, 4 bytes */
extern signed __int32 sys_sysMBValue;   /* 0x008E3B40, 4 bytes */
extern int theFxHelper;   /* 0x01407508, 21788 bytes */
extern unsigned int theFxScheduler[5409];   /* 0x014075A0, 21636 bytes */
extern int cgameMaxPitchSpeedStorage;   /* 0x0143A980, 8 bytes */
extern int cgameMaxYawSpeedStorage;   /* 0x0143A984, 4 bytes */
extern int hunk_lowTempMark;   /* 0x008931C0, 4 bytes */
extern int scrMemTree_allocBuckets;   /* 0x008E6310, script/scr_memorytree.cpp:81 */
extern int cmd_argc;   /* 0x008930F0, 172 bytes */
extern int CM_NumInlineModels( void );   /* qcommon/cm_load.c, reads cm.numSubModels */
extern int com_hunkMeminfoRunningTotal;   /* 0x0163A204, 4 bytes */
extern int hunk_totalSize;   /* 0x0089319C, 4 bytes */
extern int hunk_highTemp;   /* 0x008931B0, 4 bytes */
extern void *scrAnimPub_trees[258];   /* 0x008E5AD8, 1032 bytes */
extern int xanim_numDeferredNotifies;   /* 0x00A9C638, 24 bytes */
extern int xanim_activePoolSlot;   /* 0x00A9CC58, 4 bytes */
extern unsigned char xanim_deferredNotifies[1536];   /* 0x00A9C650, 1536 bytes */
char cl_bigConfigString[8192];   /* 0x0057C910, 8192 bytes */
extern cvar_t *cl_noprint;   /* 0x0161731C, 4 bytes */
extern int con_boldgamemessagetime;   /* 0x0140EF2C, 4 bytes */
extern int con_gamemessagetime;   /* 0x0140EF30, 16 bytes */
extern int hunk_temp_permanent;   /* 0x008931C4, 4 bytes */
extern int hunk_temp_temp;   /* 0x008931C8, 8 bytes */
extern int hunk_low_permanent;   /* 0x008931C4, the counter (com_memory_core.c) */
extern int hunk_low_temp;   /* 0x008931C8, the counter (com_memory_core.c) */
extern int Hunk_MemoryRemaining( void );   /* com_memory.c; case 89 */
extern float cgameClientLerpOrigin_x;   /* 0x0143A988, 4 bytes */
extern float cgameClientLerpOrigin_y;   /* 0x0143A98C, 4 bytes */
extern float cgameClientLerpOrigin_z;   /* 0x0143A990, 4 bytes */
extern int cgameUserAim_x;   /* 0x0143A994, 4 bytes */
extern int cgameUserAim_y;   /* 0x0143A998, 4 bytes */
extern int cgameUserAim_z;   /* 0x0143A99C, 1556 bytes */
extern signed __int32 cgameUserCmdInShellshock;   /* 0x0143A978, 4 bytes */
extern int cl_gameState_stringOffsets[1];   /* 0x01434A7C, 4 bytes */
extern cvar_t *cl_showServerCommands;   /* 0x0143294C, 8 bytes */

extern unsigned char cl_parseEntitiesStorage[491520];   /* 0x014B9134, 491520 bytes */
extern unsigned char cl_parseClientsStorage[188676];   /* 0x01531134, 188676 bytes */
extern float cgameSensitivity;   /* 0x0143A97C, 12 bytes */
extern signed __int32 cgameUserCmdValue;   /* 0x0143A974, 4 bytes */
extern int cl_cmdNumber;   /* 0x0143AFB0, 388 bytes */
extern int cl_parseClientsNum;   /* 0x0143A944, 48 bytes */
extern int cl_parseEntitiesNum;   /* 0x0143A940, 4 bytes */
extern char cl_snap_messageNum[4];   /* 0x01432970, 4 bytes */
extern int cl_snap_serverTime;   /* 0x0143296C, 4 bytes */
extern int cl_snapshots[67584];   /* 0x0143B134, 270336 bytes */
extern unsigned char cl_cmdsStorage[1536];   /* 0x0143A9B0, 1536 bytes */

extern int CIN_DrawCinematic();
extern int CIN_PlayCinematic();
extern int CIN_RunCinematic( int handle );
extern void CIN_SetExtents( int handle, int x, int y, int w, int h );
extern int CIN_StopCinematic();
extern void CL_AddConsoleInfoColor( int base, const float *rgb );
extern void CL_AddDeathMessageText( const char *txt, int color );

extern int CL_AddReliableCommand();
void CL_DeathMessagePrint( const char *attacker, int channel, const float *attackerColor,
                           const char *victim, const float *victimColor, const char *iconName,
                           float iconWidth, float iconHeight, const float *iconColor, int duration );
extern int CL_ConsolePrint_AddLine( byte *txt, int channel, int duration,
                                    int width, int color );
extern int CL_GetServerIPAddress();
extern int CL_ReadDemoMessage();
extern void CL_SystemInfoChanged( void );
extern void CM_BoxTrace( trace_t *results, const float *start, const float *end,
                         const float *mins, const float *maxs, int model,
                         int brushmask, qboolean capsule );
extern int CM_InlineModel( int index );
extern void CM_LoadMap( const char *name, qboolean clientload, int *checksum );
extern int CM_PointContents( const float *p, int model );
extern int CM_TempBoxModel( const float *mins, const float *maxs, int contents,
                            qboolean capsule );
extern void CM_TransformedBoxTraceExternal( trace_t *results, const float *start,
                                            const float *end, const float *mins,
                                            const float *maxs, int model,
                                            int brushmask, const float *origin,
                                            const float *angles,
                                            qboolean capsule );
extern int CM_TransformedPointContents( const float *p, int model,
                                        const float *origin, const float *angles );
extern void Cmd_TokenizeString2( const char *text, int max_tokens );
extern void Com_ClientDObjCreate( void *models, unsigned short modelCount,
                                   void *tree, int handle, unsigned short scrNotifyId );
extern int *Com_FindSoundAlias( const char *name, int source );
extern void Com_FreeWeaponInfoMemory( int owner, int iSource );
extern void *Com_GetSoundAlias( int source, int index );
extern void *Com_GetWeaponInfoMemory( int size, int *pPrevOwner, int owner );
qboolean Com_LoadCvarsFromBuffer( const char **cvarNames, int cvarCount,
                                  char *text, const char *filename );
qboolean Com_SaveCvarsToBuffer( const char **cvarNames, int cvarCount,
                                char *buffer, int bufferSize );
extern int Com_LoadSoundAliasFile( const char *sourceName, const char *csvPath,
                                   int count, int source );
extern int Com_LoadSoundAliasSounds();
extern void Com_MakeSoundAliasesPermanent( int count, int source );
extern void *Com_PickSoundAlias( const char *name, int source );
extern int Com_RealTime();
extern void Com_SafeClientDObjFree( int handle, unsigned int releaseTree );
extern int Com_SoundList_f();
extern int Com_TouchMemory();
void Com_UnloadSoundAliasSounds( int source );   /* 0x00433E20 */
extern void Con_ClearNotify( void );
extern void Con_ClearSubtitles( void );
extern int Con_Close();
extern void Con_DrawBoldMessages( int charset, float alpha, int orientation, int yStart );
extern void Con_DrawMiniConsole( int yStart, int charset, float alpha );
extern void Con_DrawNotify( int charset, float alpha, int orientation, int yStart );
extern void Con_DrawSay( int y );
extern void Con_DrawSubtitles( int charset, float alpha, int orientation, int yStart );
extern void Con_Linefeed( int channel, int duration );
/* Retail takes no arguments (call at 0x00401DAD). */
extern void Con_OneTimeInit( void );
extern void DObjCalcAnim( int dobj, int partBits );
extern void DObjCalcSkel( int partBits, int dobj );
/* xanim/dobj.cpp 0x00481EA0 */
void DObjCreate( const void *models, unsigned short modelCount, void *tree,
                 void *dobj, unsigned short scrNotifyId );
extern int DObjDumpInfo();
extern int DObjFree();
extern int FS_ListFilteredFiles();
extern int Hunk_AllocAlignInternal();
extern int Hunk_AllocLowAlignInternal();
extern int Hunk_AllocXAnimPrecache();
extern void Key_GetBindingBuf( int keynum, char *buf, int buflen );
extern int Key_GetKey( const char *binding );
extern int Key_IsDown( int keynum );
extern void Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
extern void Key_SetBinding( int keynum, const char *binding );
extern int Key_SetCatcher();
extern int MSS_FadeAllSounds();
extern int MSS_FadeSelectSounds();
extern int MSS_GetSoundOverlay( int which, void *buffer, int maxCount, int *cpuOut );
extern void MSS_PlayAmbientAlias();
extern int MSS_PlayBlendedSoundAliases();
extern int MSS_PlayMusicAlias();
extern int MSS_PlaySoundAlias();
extern int MSS_SetEnvironmentEffects( const char *roomType, float wetTarget, int msec );
extern int MSS_SetListener();
extern int MSS_StopBackground();
extern int MSS_StopSounds();
extern void MSS_UpdateLoopingSounds();
extern float Q_acos( float c );
extern int SCR_UpdateScreen();
extern int SEH_PrintStrlen();
extern int SEH_ReadCharFromString();
extern char *SEH_SafeTranslateString( const char *key );
extern int SEH_StringEd_GetString();
extern int Scr_NearHook();
extern int Sys_SnapVector();
/* xanim/dobj.cpp 0x00483F80 */
void XAnimSetLeafNode( const char *animName, void *tree, unsigned short index );
extern int XAnimPrecacheAnimTree_m();
extern int XModelExists();
extern int FastRound( float f );
extern int DObjGetHierarchyBits();
extern int DObjBuildPartCollisionTable();
int DObjMarkRotTransIndex( void *dobj, const unsigned char *partBits, int boneIndex );
int DObjMarkControlRotTransIndex( void *dobj, const unsigned char *partBits, int boneIndex );
extern int DObjGetBoneIndex();
extern int DObjGetBoneName();
/* xanim/dobj.cpp 0x00483FC0 */
unsigned short *XAnimSetParentNode( unsigned short childCount, unsigned short flags, void *tree,
                                    unsigned short index, int unused,
                                    unsigned short firstChildIndex );
extern int XAnimAllocTree();
extern int XAnimAllocRuntimeTree();
extern float XAnimGetTime( void *tree, int nodeIndex );
extern float XAnimGetWeight( void *tree, int nodeIndex );
extern int XAnimHasTime();
extern const char *XAnimGetAnimName( const void *sourceTree, int nodeIndex );
void DObjUpdateClientInfo_m( void *dobj, float dtime );
extern int DObjDisplayAnim();
extern int XAnimGetRelDelta_m();
extern int XAnimGetAbsDelta_m();
extern int XAnimGetRelDeltaForTime_m( int sourceTree, int node, void *posDelta, float *dirDelta, float time0, float time1 );
extern void XAnimGetAbsDeltaForTime_m( const void *sourceTree, int node, float *posDelta, float *dirDelta, float time );
extern int XAnimClearAnimNode_m( void *tree, int node, float blend );
extern int XAnimClearAnim_m( int node, void *tree, float blend );
void XAnimClearAnimChildSubtrees_m( void *tree, int nodeIndex, float blendTime );
extern int XAnimSetAnimKnob_m( void *tree, int node, float weight, int blendTime, float rate,
					   short notifyName, int notifyType, int animFlags );
extern int XAnimSetAnimKnobAll_m( int node, void *tree, int stopNode, float weight, int blendTime, float rate,
					   short notifyName, int notifyType, int animFlags );
extern void XAnimClearAnimChildren_m( int *tree, int nodeIndex, float weight );
extern void XAnimSetTime_m( void *tree, int node, float time );
extern int XAnimSetAnimInternalLimited_m( int notifyType, short notifyName, int tree, int node, float weight, int blendTime,
					   float rate, int animFlags );
extern int XAnimSetAnimRate_m( int tree, int node, float rate );
extern int XAnimIsLooped_m(const void *source, int node);
extern int XAnimSetAnimKnobInternal_m( void *tree, int node, float weight, float blendTime, float rate,
					   short notifyName, int notifyType, int animFlags );
extern int XAnimCopyTree();
extern int __cdecl FX_RegisterEffect( char *name );
extern void __stdcall CFxScheduler__PlayEffect_id_simple_m( int effectId, float *origin, void *self );
extern void __stdcall CFxScheduler__PlayEffect_name_bolt_m( char *name, void *self, float *origin, int *axis, int *bolt );
extern void __stdcall CFxScheduler__PlayEffect_id_axis(void *self, int id, float *origin, void *axis, void *bolt);
extern void __stdcall CFxScheduler__PlayEffect_name_simple_m( char *name, void *self, float *origin );
extern void __stdcall CFxScheduler__PlayEffect_name_m( int *direction, char *name, void *self, float *origin );
extern void CFxScheduler__AddScheduledEffects( unsigned int *self );
extern int SFxHelper__AdjustTime();
extern void __stdcall SFxHelper__AdjustCamera( int self, int view, float zfar );
extern int FX_Free();
extern int FX_Init();
extern int  __cdecl FX_GetBoneIndex( int entnum, const char *boneName );
extern void __cdecl FX_PlayEffectID( int *axis, int effectID, float *origin );
/* SE_GetString 0x004ABF00, stringed/stringed_unassigned.cpp. */
extern char *SE_GetString( const char *key, int wantReference );

static char cl_mapnameBsp[64];

void CL_GetGameState( void *gs );
void CL_GetGlconfig_cgame( glconfig_t *config );
void CL_FirstSnapshot( void );
void CL_AdjustTimeDelta( void );
void CL_SetCGameTime( void );

/* ---- CL_GetUserCmd  0x004010A0 ----  VERIFIED */
qboolean CL_GetUserCmd( int cmdNumber, byte *ucmd )
{
	if ( cmdNumber > cl_cmdNumber ) {
		Com_Error( ERR_DROP, "" "CL_GetUserCmd: %i >= %i",
		           cmdNumber, cl_cmdNumber );
	}

	if ( cmdNumber <= cl_cmdNumber - CMD_BACKUP ) {
		return qfalse;
	}

	*(usercmd_t *)ucmd = ( (usercmd_t *)cl_cmdsStorage )[ cmdNumber & CMD_MASK ];

	return qtrue;
}

/* ---- CL_GetCurrentCmdNumber  0x004010F0 ----  [CONFIRMED] */
int __cdecl CL_GetCurrentCmdNumber()
{
  return cl_cmdNumber;
}

/*
 * Typed views over the client-active blobs.  The byte-array storage stays
 * the storage of record (as do the SNAP_, ES_ and CS_ offset macros); these
 * casts only put a cl_local.h C type over the same memory so the marshalling
 * code can say clSnap->numEntities instead of *(int *)(clSnap + SNAP_NUMENTITIES).
 * Duplicated file-locally with cl_parse_mp.c (separate translation units).
 */
#define cl_parseEntities    ( (entityState_t *)cl_parseEntitiesStorage )  /* cl.parseEntities[2048]  (cl+0x867D4) */
#define cl_parseClients     ( (clientState_t *)cl_parseClientsStorage )  /* cl.parseClients[2048]   (cl+0xFE7D4) */
#define cl_snapshotRing     ( (clSnapshot_t  *)cl_snapshots )  /* cl.snapshots[32]        (cl+0x87D4)  */

/* ---- CL_GetCurrentSnapshotNumber  0x00401100 ----  VERIFIED */
void CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime )
{
  *snapshotNumber = *(int *)cl_snap_messageNum;
  *serverTime = cl_snap_serverTime;
}

/* ---- CL_GetSnapshot  0x00401120 ----  VERIFIED */
qboolean CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot )
{
	const clSnapshot_t *clSnap;
	int   count;
	int   i;

	if ( snapshotNumber > *(int *)cl_snap_messageNum ) {
		Com_Error( ERR_DROP,
		           "\x15" "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	if ( *(int *)cl_snap_messageNum - snapshotNumber >= 32 ) {
		return qfalse;
	}

	clSnap = &cl_snapshotRing[ snapshotNumber & 31 ];

	if ( !clSnap->valid ) {
		return qfalse;
	}

	if ( cl_parseEntitiesNum - clSnap->parseEntitiesNum >= 0x800 ) {
		return qfalse;
	}
	if ( cl_parseClientsNum - clSnap->parseClientsNum >= 0x800 ) {
		return qfalse;
	}

	snapshot->snapFlags             = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping                  = clSnap->ping;
	snapshot->serverTime            = clSnap->serverTime;

	memcpy( snapshot->ps, clSnap->ps, sizeof( snapshot->ps ) );

	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		if ( com_statmon->integer ) {
			StatMon_Warning( 8, 3000, "gfx/2d/warning@snapshotents.jpg" );
		} else {
			Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n",
			             count, MAX_ENTITIES_IN_SNAPSHOT );
		}
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;

	for ( i = 0; i < count; i++ ) {
		snapshot->entities[i] =
		    cl_parseEntities[ ( clSnap->parseEntitiesNum + i ) & 0x7FF ];
	}

	count = clSnap->numClients;
	if ( count > MAX_CLIENTS_IN_SNAPSHOT ) {
		count = MAX_CLIENTS_IN_SNAPSHOT;
	}
	snapshot->numClients = count;

	for ( i = 0; i < count; i++ ) {
		snapshot->clients[i] =
		    cl_parseClients[ ( clSnap->parseClientsNum + i ) & 0x7FF ];
	}

	return qtrue;
}

/* ---- CL_SetUserCmdValue  0x004012C0 ----  [CONFIRMED] */
void __cdecl CL_SetUserCmdValue(int userCmdValue, float sensitivityScale)
{
  cgameUserCmdValue = userCmdValue;
  cgameSensitivity = sensitivityScale;
}

/* ---- CL_SetUserCmdAimValues  0x004012D0 ----  [CONFIRMED] */
void __cdecl CL_SetUserCmdAimValues(int *aim)
{
  cgameUserAim_x = *aim;
  cgameUserAim_y = aim[1];
  cgameUserAim_z = aim[2];
}

/* ---- CL_SetUserCmdInShellshock  0x004012F0 ----  [CONFIRMED] */
void __cdecl CL_SetUserCmdInShellshock(int value)
{
  cgameUserCmdInShellshock = value;
}

/* ---- CL_SetClientLerpOrigin  0x00401300 ----  [CONFIRMED] */
void __cdecl CL_SetClientLerpOrigin(float x, float y, float z)
{
  cgameClientLerpOrigin_x = x;
  cgameClientLerpOrigin_y = y;
  cgameClientLerpOrigin_z = z;
}

/* ---- CL_AddCgameCommand  0x00401320 ----  [CONFIRMED] */
void __cdecl CL_AddCgameCommand(const char *cmdName)
{
  Cmd_AddCommand(cmdName, 0);
}

/* ---- CL_ConfigstringModified  0x00401330 ----  VERIFIED */
void CL_ConfigstringModified( void )
{
	const char  *old;
	const char  *s;
	const char  *dup;
	int         index;
	int         i;
	int         len;
	gameState_t oldGs;

	index = atoi( Cmd_Argv( 1 ) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "\x15" "configstring > MAX_CONFIGSTRINGS" );
	}

	s = Cmd_Argv( 2 );

	old = &cl_gameState_stringData[ cl_gameState_stringOffsets[ index ] ];
	if ( !strcmp( old, s ) ) {
		return;
	}

	oldGs = *(gameState_t *)cl_gameState_stringOffsets;
	memset( cl_gameState_stringOffsets, 0, sizeof( gameState_t ) );

	cl_gameState_dataCount = 1;

	for ( i = 0; i < MAX_CONFIGSTRINGS; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}

		if ( !dup[0] ) {
			continue;
		}

		len = strlen( dup );

		if ( len + 1 + cl_gameState_dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "\x15" "MAX_GAMESTATE_CHARS exceeded" );
		}

		cl_gameState_stringOffsets[ i ] = cl_gameState_dataCount;
		memcpy( &cl_gameState_stringData[ cl_gameState_dataCount ], dup, len + 1 );
		cl_gameState_dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		CL_SystemInfoChanged();
	}
}

/* ---- CL_GetServerCommand  0x004014E0 ----  VERIFIED */
qboolean CL_GetServerCommand( int serverCommandNumber )
{
	const char *s;
	const char *cmd;
	const char *arg;
	int         i;

	if ( serverCommandNumber <= clc_serverCommandSequence - 64 ) {
		if ( clc_demoplaying ) {
			return qfalse;
		}
		Com_Printf( "===== CL_GetServerCommand =====\n" );
		Com_Printf( "serverCommandNumber: %d\n", serverCommandNumber & 63 );
		for ( i = 0; i < 64; i++ ) {
			Com_Printf( "cmd %5d: %s\n", i,
			            &clc_serverCommands[ 1024 * ( i & 63 ) ] );
		}
		Com_Error( ERR_DROP,
		           "\x15" "CL_GetServerCommand: " "\x14" "EXE_ERR_RELIABLE_CYCLED_OUT" );
	}

	if ( serverCommandNumber > clc_serverCommandSequence ) {
		Com_Error( ERR_DROP,
		           "\x15" "CL_GetServerCommand: " "\x14" "EXE_ERR_NOT_RECEIVED" );
	}

	s = &clc_serverCommands[ 1024 * ( serverCommandNumber & 63 ) ];
	clc_lastExecutedServerCommand = serverCommandNumber;

	if ( cl_showServerCommands->integer ) {
		Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );
	}

rescan:
	Cmd_TokenizeString2( s, 0 );
	cmd = Cmd_Argv( 0 );

	switch ( cmd[0] ) {

	case 'w':
		if ( Cmd_Argc() >= 2 ) {
			Com_Error( ERR_SERVERDISCONNECT,
			           va( SEH_SafeTranslateString( "EXE_SERVERDISCONNECTREASON" ),
			               SEH_SafeTranslateString( Cmd_Argv( 1 ) ) ) );
		} else {
			Com_Error( ERR_SERVERDISCONNECT, "EXE_SERVER_DISCONNECTED" );
		}
		/* Com_Error does not return; retail falls through into 'x' */
	case 'x':
		Com_sprintf( cl_bigConfigString, 0x2000, "d %s \"%s",
		             Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
		return qfalse;

	case 'y':
		Cmd_TokenizeString2( s, 3 );
		arg = Cmd_Argv( 2 );
		if ( strlen( cl_bigConfigString ) + strlen( arg ) >= 0x2000 ) {
			Com_Error( ERR_DROP, "\x15" "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( cl_bigConfigString, arg );
		break;

	case 'z':
		Cmd_TokenizeString2( s, 3 );
		arg = Cmd_Argv( 2 );
		if ( strlen( cl_bigConfigString ) + strlen( arg ) + 1 >= 0x2000 ) {
			Com_Error( ERR_DROP, "\x15" "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( cl_bigConfigString, arg );
		/* Retail 0x00401678 copies the word at 0x005682F8 (bytes 22 00, a
		   lone double quote plus NUL) onto the end of the string, closing
		   the quote opened by the 'x' case.  The tree's Source global is
		   zero-filled, so the literal is used here. */
		strcat( cl_bigConfigString, "\"" );
		s = cl_bigConfigString;
		goto rescan;

	case 'd':
		Cmd_TokenizeString2( s, 3 );
		CL_ConfigstringModified();
		Cmd_TokenizeString2( s, 3 );
		return qtrue;

	case 'n':
		Con_ClearNotify();
		Con_ClearSubtitles();
		memset( cl_cmdsStorage, 0, UCMD_SIZE * CMD_BACKUP );
		re_ClearFlares();
		return qtrue;

	default:
		return qtrue;
	}

	return qfalse;
}

/* ---- CL_SetExpectedHunkUsage  0x00401840 ----  VERIFIED */
void CL_SetExpectedHunkUsage( const char *mapname ) {
	fileHandle_t f;
	char        *buf;
	char        *buftrav;
	char        *token;
	int         len;

	len = FS_FOpenFileByMode( "hunkusage.dat", &f, FS_READ );
	if ( len >= 0 ) {
		buf = (char *)Z_MallocInternal( len + 1 );
		memset( buf, 0, len + 1 );

		FS_Read( buf, len, f );
		FS_FCloseFile( f );

		buftrav = buf;
		while ( ( token = Com_Parse( &buftrav ) ) != 0 && token[0] ) {
			if ( !Q_strcasecmp( token, (char *)mapname ) ) {
				token = Com_Parse( &buftrav );
				if ( token && token[0] ) {
					Cvar_Set2( "com_expectedhunkusage", token, qtrue );
					free( buf );
					return;
				}
			}
		}

		free( buf );
	}

	Cvar_Set2( "com_expectedhunkusage", "-1", qtrue );
}

/* ---- CL_CM_LoadMap  0x004019F0 ----  VERIFIED */
void CL_CM_LoadMap( const char *mapname ) {
	int checksum;

	if ( !com_sv_running->integer ) {
		CL_SetExpectedHunkUsage( mapname );
	}

	CM_LoadMap( mapname, qtrue, &checksum );
}

/* ---- CL_DObjInvalidateSkels  0x00401B00 ----  VERIFIED */
void CL_DObjInvalidateSkels( void )
{
  if ( !++com_skelTimeStamp )
    com_skelTimeStamp = 1;
  if ( s_hunkData )
    hunk_low_temp = hunk_low_permanent;   /* retail hunk_lowTemp = hunk_lowUsed, read directly */
}

/* ---- CL_DObjCalcAnim  0x00401B30 ----  VERIFIED */
void CL_DObjCalcAnim( int dobj, int partBits )
{
  DObjCalcAnim( dobj, partBits );
}

/* ---- FloatAsInt_0  0x00401B40 ----  [CONFIRMED] */
int FloatAsInt_0( float f ) {
	int temp;

	*(float *)&temp = f;

	return temp;
}

static qboolean CL_DObjSkelExists( int dobj, int timeStamp );

/* xanim/dobj.cpp 0x00481720 */
int DObjSkelIsBoneUpToDate( void *dobj, int boneIndex );

/* ---- CL_DObjCreateSkelForBone  0x00401B50 ----  VERIFIED */
qboolean CL_DObjCreateSkelForBone( int dobj, int boneIndex )
{
  int   skel;
  int  *p;
  int   i;

  if ( CL_DObjSkelExists( dobj, com_skelTimeStamp ) )
    return DObjSkelIsBoneUpToDate( (void *)dobj, boneIndex );

  i = 96 * *(unsigned __int8 *)( dobj + 23 ) + 48;
  skel = (int)Hunk_AllocateTempMemoryInternal( i );
  *(unsigned int *)( dobj + 4 ) = skel;

  /* three parallel four-word masks at +0x00, +0x10 and +0x20 */
  p = (int *)( skel + 32 );
  for ( i = 4; i; i-- )
  {
    *( p - 8 ) = 0;
    *( p - 4 ) = 0;
    *p++ = 0;
  }

  return qfalse;
}

/* ---- CL_DObjSkelExists  no-address (inlined) ---- */
static qboolean CL_DObjSkelExists( int dobj, int timeStamp )
{
  if ( *(unsigned int *)( dobj + 8 ) == timeStamp )
    return *(unsigned int *)( dobj + 4 ) != 0;
  *(unsigned int *)( dobj + 8 ) = timeStamp;
  *(unsigned short *)( dobj + 16 ) = 0;
  *(unsigned int *)( dobj + 4 ) = 0;
  return qfalse;
}

/* ---- CL_DObjSkelAreBonesUpToDate  no-address ----
 * retail tail-calls dobj.obj's copy (0x00481740, missing from xanim/dobj.cpp) */
qboolean CL_DObjSkelAreBonesUpToDate( int dobj, const int *partBits )
{
  int   skel;
  int   i;

  skel = *(unsigned int *)( dobj + 4 );
  for ( i = 0; i < 4; i++ )
  {
    if ( partBits[i] & ~( (const int *)( skel + 32 ) )[i] )
      return qfalse;
  }
  return qtrue;
}

/* ---- CL_DObjCreateSkelForBones  0x00401BE0 ----  VERIFIED */
qboolean CL_DObjCreateSkelForBones( int dobj, int partBits )
{
  int   skel;
  int  *p;
  int   i;

  if ( CL_DObjSkelExists( dobj, com_skelTimeStamp ) )
    return CL_DObjSkelAreBonesUpToDate( dobj, (const int *)partBits );

  skel = (int)Hunk_AllocateTempMemoryInternal(
                96 * *(unsigned __int8 *)( dobj + 23 ) + 48 );
  *(unsigned int *)( dobj + 4 ) = skel;

  p = (int *)( skel + 32 );
  for ( i = 4; i; i-- )
  {
    *( p - 8 ) = 0;
    *( p - 4 ) = 0;
    *p++ = 0;
  }

  return qfalse;
}

/* ---- CL_DObjCalcSkel  0x00401C50 ----  VERIFIED */
void CL_DObjCalcSkel( int dobj, int partBits )
{
  DObjCalcSkel( partBits, dobj );
}

typedef struct {
	const char  *name;
	int         a;
	int         surfaceBits;
	int         b;
} cl_surfacetype_t;

extern cl_surfacetype_t cl_surfaceParmsByType[24];

#define CL_SURFACETYPE_FIRST    1       /* table[1] == "bark" 0x005717A0 */
#define CL_SURFACETYPE_LIMIT    23      /* table[23] == the bound 0x00571900 */

/* ---- CL_SurfaceTypeFromName  0x00401C60 ----  VERIFIED */
int CL_SurfaceTypeFromName( const char *name ) {
	int i;

	if ( !_stricmp( name, "default" ) ) {
		return 0;
	}

	/* 0-based index from table[1]: retail starts edi at 0 (0x00401C75) */
	for ( i = 0 ; i < CL_SURFACETYPE_LIMIT - CL_SURFACETYPE_FIRST ; i++ ) {
		if ( !_stricmp( name, cl_surfaceParmsByType[CL_SURFACETYPE_FIRST + i].name ) ) {
			return ( cl_surfaceParmsByType[CL_SURFACETYPE_FIRST + i].surfaceBits >> 20 ) & 0x1F;
		}
	}

	return -1;
}

/* ---- CL_SurfaceTypeToName  0x00401CC0 ----  VERIFIED */
const char *CL_SurfaceTypeToName( int type ) {
	if ( type <= 0 || type >= CL_SURFACETYPE_LIMIT ) {
		return "default";
	}

	return cl_surfaceParmsByType[type].name;
}

/* ---- Hunk_AllocXAnimClientCreate  0x00401CE0 ----  [CONFIRMED] */
void *__cdecl Hunk_AllocXAnimClientCreate(unsigned int size)
{
  return Hunk_AllocAlignInternal(size, 32);
}

/* ---- Hunk_AllocXAnimClientCreateTree  0x00401CF0 ----  [CONFIRMED] */
void *__cdecl Hunk_AllocXAnimClientCreateTree(unsigned int size)
{
  return Hunk_AllocAlignInternal(size, 32);
}

extern cvar_t *cl_languagetranslate;          /* 0x01432954 */
extern cvar_t *cl_languagewarnings;           /* 0x01617350 */
extern cvar_t *cl_languagewarningsaserrors;   /* 0x01432844 */

/* ---- CL_SubtitlePrint  0x00401D00 ----  VERIFIED */
void CL_SubtitlePrint( const char *text, int duration, int pixelWidth ) {
	const char *out;

	if ( cl_languagetranslate && cl_languagetranslate->integer
		 && text[0] && text[1] ) {
		out = SE_GetString( text, 0 );
	} else {
		out = text;
	}

	if ( !out ) {
		if ( cl_languagewarnings->integer ) {
			if ( cl_languagewarningsaserrors->integer ) {
				Com_Error( 7, "Could not translate subtitle text: \"%s\"", text );
			} else {
				Com_Printf( "^3WARNING: Could not translate subtitle text: \"%s\"\n",
							text );
			}
			out = va( "^1UNLOCALIZED(^7%s^1)^7", text );
		} else {
			out = text;
		}
	}

	CL_ConsolePrint( 3, out, duration, pixelWidth );
}

/* ---- CL_CgameSystemCalls  0x00401DF0 ----  VERIFIED */
int __cdecl CL_CgameSystemCalls( int *args )
{
  int   numfiles;
  int   source;
  char  filename[64];

  switch ( *args )
  {
    case 0:
      Com_Printf( "%s", (const char *)args[1] );
      return 0;
    case 1:
      Com_Error( ERR_DROP, "\x15%s", (const char *)args[1] );
    case 2:   /* CG_MESSAGE_PRINT, channel 1 (game message) */
      CL_ConsolePrint( 1, (const char *)args[1], 0, args[2] );
      return 0;
    case 3:   /* CG_MESSAGE_PRINT, channel 2 (bold game message) */
      CL_ConsolePrint( 2, (const char *)args[1], 0, args[2] );
      return 0;
    case 4:   /* obituary: two-color death-message line */
      CL_DeathMessagePrint( (const char *)args[1], 1, (const float *)args[2],
                            (const char *)args[3], (const float *)args[4],
                            (const char *)args[5], *(float *)&args[6],
                            *(float *)&args[7], (const float *)args[8], 0 );
      return 0;
    case 5:
      CL_SubtitlePrint( (const char *)args[1], args[2], args[3] );
      return 0;
    case 6:
      return Sys_Milliseconds();
    case 7:
      Cvar_Register( (vmCvar_t *)args[1], (const char *)args[2], (const char *)args[3], args[4] );
      return 0;
    case 8:
      Cvar_Update( (vmCvar_t *)args[1] );
      return 0;
    case 9:
      Cvar_Set2( (const char *)args[1], (const char *)args[2], qtrue );
      return 0;
    case 10:
      Cvar_VMSet( (vmCvar_t *)args[1], (const char *)args[2] );
      return 0;
    case 11:
      Cvar_VariableStringBuffer( (const char *)args[1], (char *)args[2], args[3] );
      return 0;
    case 12:
      return cmd_argc;
    case 13:
      Cmd_ArgvBuffer( args[1], (char *)args[2], args[3] );
      return 0;
    case 14:
      Cmd_ArgsBuffer( (char *)args[1], args[2] );
      return 0;
    case 15:
      return FS_FOpenFileByMode( (const char *)args[1], (fileHandle_t *)args[2], (fsMode_t)args[3] );
    case 16:
      return FS_Read( (void *)args[1], args[2], args[3] );
    case 17:
      return FS_Write( (const void *)args[1], args[2], args[3] );
    case 18:
      FS_FCloseFile( args[1] );
      return 0;
    case 19:   /* retail loads args[1] into EAX and pushes args[2..4], 0x00402444 */
      return FS_GetFileList( (const char *)args[1], (const char *)args[2],
                             (char *)args[3], args[4] );
    case 20:
      return Com_SaveCvarsToBuffer( (const char **)args[1], args[2],
                                    (char *)args[3], args[4] );
    case 21:
      return Com_LoadCvarsFromBuffer( (const char **)args[1], args[2],
                                      (char *)args[3], (const char *)args[4] );
    case 22:
      Cbuf_AddText( (const char *)args[1] );   /* text@<eax>, 0x004024CD */
      return 0;
    case 23:
      Cmd_AddCommand( (const char *)args[1], 0 );
      return 0;
    case 101:
      Cmd_RemoveCommand( (const char *)args[1] );
      return 0;
    case 24:
      CL_AddReliableCommand( (const char *)args[1] );
      return 0;
    case 25:   /* one instruction in retail (0x0040250F); SCR_UpdateScreen takes no arguments */
      SCR_UpdateScreen();
      return 0;
    /* cases 26-30: yStart/y is args[1], live-in via ECX (EDI for case 30) at the retail sites */
    case 26:
      Con_DrawNotify( args[2], *(float *)&args[3], args[4], args[1] );
      return 0;
    case 27:
      Con_DrawBoldMessages( args[2], *(float *)&args[3], args[4], args[1] );
      return 0;
    case 28:
      Con_DrawMiniConsole( args[1], args[2], *(float *)&args[3] );
      return 0;
    case 29:
      Con_DrawSubtitles( args[2], *(float *)&args[3], args[4], args[1] );
      return 0;
    case 30:
      Con_DrawSay( args[1] );
      return 0;
    case 31:
      CL_CM_LoadMap( (const char *)args[1] );   /* mapname@<esi>, 0x00402587 */
      return 0;
    case 32:
      return CM_NumInlineModels();
    case 33:
      return CM_InlineModel( args[1] );   /* index@<esi>, 0x004025AF */
    case 35:
      return CM_TempBoxModel( (const float *)args[1], (const float *)args[2],
                              args[3], qfalse );
    case 42:   /* case 35 with capsule = 1 (0x004025FD pushes 1) */
      return CM_TempBoxModel( (const float *)args[1], (const float *)args[2],
                              args[3], qtrue );
    case 36:
      return CM_PointContents( (const float *)args[1], args[2] );
    case 37:   /* p@<ecx>=args[1], model=args[2], origin@<eax>=args[3], angles@<edx>=args[4], 0x00402642 */
      return CM_TransformedPointContents( (const float *)args[1], args[2],
                                          (const float *)args[3],
                                          (const float *)args[4] );
    case 38:
      CM_BoxTrace(
        (trace_t *)args[1],
        (const float *)args[2],
        (const float *)args[3],
        (const float *)args[4],
        (const float *)args[5],
        args[6],
        args[7],
        qfalse );
      return 0;
    case 39:
      CM_TransformedBoxTraceExternal(
        (trace_t *)args[1],
        (const float *)args[2],
        (const float *)args[3],
        (const float *)args[4],
        (const float *)args[5],
        args[6],
        args[7],
        (const float *)args[8],
        (const float *)args[9],
        qfalse );
      return 0;
    case 40:
      CM_BoxTrace(
        (trace_t *)args[1],
        (const float *)args[2],
        (const float *)args[3],
        (const float *)args[4],
        (const float *)args[5],
        args[6],
        args[7],
        qtrue );
      return 0;
    case 41:
      CM_TransformedBoxTraceExternal(
        (trace_t *)args[1],
        (const float *)args[2],
        (const float *)args[3],
        (const float *)args[4],
        (const float *)args[5],
        args[6],
        args[7],
        (const float *)args[8],
        (const float *)args[9],
        qtrue );
      return 0;
    case 43:
      return re_LerpTag(
               args[1], args[2], args[3], args[4], args[5],
               args[6], args[7], args[8], args[9], args[10] );
    case 209:
      MSS_SetListener( (int *)args[3], (int *)args[2], args[1] );
      return 0;
    case 210:
      MSS_UpdateLoopingSounds();
      return 0;
    case 211:
      MSS_StopSounds( args[1] );
      return 0;
    case 212:
      MSS_PlayMusicAlias( (float *)args[1] );
      return 0;
    case 213:
      MSS_StopBackground( 0, args[1] );
      return 0;
    case 214:
      MSS_PlayAmbientAlias( args[1], args[2] );
      return 0;
    case 215:
      MSS_FadeAllSounds( args[1], args[2] );
      return 0;
    case 216:
      MSS_FadeSelectSounds( (float *)args[1], args[2] );
      return 0;
    case 217:
      MSS_SetEnvironmentEffects( (const char *)args[1], *(float *)&args[2], args[3] );
      return 0;
    case 218:
      return MSS_GetSoundOverlay( args[1], (void *)args[2], args[3], (int *)args[4] );
    case 190:
      Com_LoadSoundAliases( (const char *)args[1], 1 );
      return 0;
    case 191:
    {
      const char *name = (const char *)args[1];
      int *alias = (int *)Com_FindSoundAlias( name, 1 );
      if ( !alias )
        return 0;
      return *alias;
    }
    case 192:
      return (int)Com_PickSoundAlias( (const char *)args[1], 1 );
    case 193:
      return (int)Com_GetSoundAlias( 1, args[1] );
    case 194:
      return MSS_PlaySoundAlias( (float *)args[1], args[2], (float *)args[3], args[4] );
    case 195:
      MSS_PlayBlendedSoundAliases( (int *)args[1], (float *)args[2], args[3], args[4], (float *)args[5], args[6] );
      return 0;
    case 196:
      return CL_SurfaceTypeFromName( (const char *)args[1] );
    case 197:
      return (int)CL_SurfaceTypeToName( args[1] );
    case 44:
      re_LoadWorldMap( args[1], 0 );
      return 0;
    case 45:
      return re_RegisterModel( args[1], args[2] );
    case 46:
      ( (void (__cdecl *)())re_FinishLoadingModels )();
      return 0;
    case 47:
      re_SetIgnorePrecacheErrors( args[1] );
      return 0;
    case 49:
      return re_GetShaderFromModel( args[1], args[2] );
    case 50:
      return re_GetXModelByHandle( args[1] );
    case 48:
      return re_RegisterShader( args[1], args[2] );
    case 51:
      re_RegisterFont( args[1], args[2], args[3], args[4] );
      /* fall through: retail registers the font and then returns case 88's value */
    case 88:
      return re_RegisterShaderNoMip( args[1], args[2] );
    case 52:
      return re_Text_Width( args[1], args[2], args[3], 0, args[4] );
    case 53:
      return re_Text_Height( args[1], args[2] );
    case 54:
      re_Text_Paint( args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9] );
      return 0;
    case 55:
      ( (void (*)( int, int, int, int, int, int, int, char, int, int, int ))re_Text_PaintWithCursor )(
        args[1],
        args[2],
        args[3],
        args[4],
        args[5],
        args[6],
        args[7],
        (char) args[8],
        0,
        args[9],
        args[10] );
      return 0;
    case 56:
      return SEH_StringEd_GetString( (const char *)args[1] );
    case 57:
      return SEH_LocalizeTextMessage( (char *)args[1], (const char *)args[2], 0 );
    case 58:
      return SEH_PrintStrlen( (char *)args[1] );
    case 59:
      return SEH_ReadCharFromString( (unsigned char **)args[1], (int *)args[2] );
    case 60:
      ( (void (__cdecl *)())re_ClearScene )();
      return 0;
    case 61:
      re_AddRefEntityToScene( args[1], 0 );
      return 0;
    case 64:
      re_AddPolyToScene( args[1], args[2], args[3] );
      return 0;
    case 65:
      re_AddPolysToScene( args[1], args[2], args[3], args[4] );
      return 0;
    case 66:
      re_AddLightToScene( args[1], args[2], args[3], args[4], args[5] );
      return 0;
    case 67:
      re_AddCoronaToScene( args[1], args[2], args[3], args[4], args[5], args[6], args[7] );
      return 0;
    case 68:
      re_SetFog( args[1], args[2], args[3], args[4], args[5], args[6], args[7] );
      return 0;
    case 69:
      re_RenderScene( args[1] );
      return 0;
    case 70:
      ( (void (__cdecl *)())re_SaveScreen )();
      return 0;
    case 71:
      re_BlendSavedScreen( args[1] );
      return 0;
    case 72:
      re_SetColor( args[1] );
      return 0;
    case 73:
      re_DrawStretchPic( args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9] );
      return 0;
    case 74:
      re_StretchPicGradient( args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11] );
      return 0;
    case 75:
      re_StretchPicRotate( args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10] );
      return 0;
    case 76:
      re_DrawQuadPic( args[1], args[2], args[3] );
      return 0;
    case 77:
      re_ModelBounds( args[1], args[2], args[3] );
      return 0;
    case 78:
      CL_GetGlconfig_cgame( (glconfig_t *)args[1] );
      return 0;
    case 79:
      CL_GetGameState( (void *)args[1] );
      return 0;
    case 80:
      CL_GetCurrentSnapshotNumber( (int *)args[1], (int *)args[2] );
      return 0;
    case 81:
      return CL_GetSnapshot( args[1], (snapshot_t *)args[2] );   /* snapshotNumber@<eax>, 0x00402F71 */
    case 82:
      return CL_GetServerCommand( args[1] );   /* @<eax>, 0x00402F96 */
    case 83:
      return cl_cmdNumber;
    case 84:
      return CL_GetUserCmd( args[1], (byte *)args[2] );   /* cmdNumber@<eax>, 0x00402FCF */
    case 85:
      CL_SetUserCmdValue( args[1], *(float *)&args[2] );
      return 0;
    case 86:
    {
      const int *aim = (const int *)args[1];
      cgameUserAim_x = aim[0];
      cgameUserAim_y = aim[1];
      cgameUserAim_z = aim[2];
      return 0;
    }
    case 87:
      cgameUserCmdInShellshock = args[1];
      return 0;
    case 208:
      CL_SetClientLerpOrigin( *(float *)&args[1], *(float *)&args[2], *(float *)&args[3] );
      return 0;
    case 89:   /* hunk total - high temp - low temp, as in CL_UISystemCalls */
      return Hunk_MemoryRemaining();
    case 90:
      return Key_IsDown( args[1] );   /* @<eax>, 0x00403084 */
    case 91:
      return cls_keyCatchers;
    case 92:
      Key_SetCatcher( args[1] );
      return 0;
    case 93:
      return Key_GetKey( (const char *)args[1] );   /* binding@<ebx>, 0x004030CA */
    case 109:   /* inlined memset(dst, fillByte, count) returning dst */
      return (int)memset( (void *)args[1], args[2], args[3] );
    case 110:
      return (int)memcpy( (void *)args[1], (const void *)args[2], args[3] );
    case 111:
      return (int)strncpy( (char *)args[1], (const char *)args[2], args[3] );
    case 112:
    {
      float r = sin( *(float *)&args[1] );
      return *(int *)&r;
    }
    case 113:
    {
      float r = cos( *(float *)&args[1] );
      return *(int *)&r;
    }
    case 114:
    {
      float r = atan2( *(float *)&args[1], *(float *)&args[2] );
      return *(int *)&r;
    }
    case 115:
    {
      float r = sqrt( *(float *)&args[1] );
      return *(int *)&r;
    }
    case 116:
    {
      float r = floor( *(float *)&args[1] );
      return *(int *)&r;
    }
    case 117:
    {
      float r = ceil( *(float *)&args[1] );
      return *(int *)&r;
    }
    case 120:
    {
      float r = Q_acos( *(float *)&args[1] );
      return *(int *)&r;
    }
    case 94:
      return PC_AddGlobalDefine( (const char *)args[1] );
    case 95:
      return PC_LoadSourceHandle( (const char *)args[1] );
    case 96:
      return PC_FreeSourceHandle( args[1] );
    case 97:
      return PC_ReadTokenHandle( args[1], (pc_token_t *)args[2] );
    case 98:
      return PC_SourceFileAndLine( args[1], (char *)args[2], (int *)args[3] );
    case 99:
      return Com_RealTime( (int *)args[1] );
    case 100:
      Sys_SnapVector( (float *)args[1] );
      return 0;
    case 102:
      return CIN_PlayCinematic( (const char *)args[1], args[2], args[3], args[4], args[5], args[6] );
    case 103:
      return CIN_StopCinematic( args[1] );   /* handle@<ecx>, 0x004033A0 */
    case 104:
      return CIN_RunCinematic( args[1] );
    case 105:
      CIN_DrawCinematic( args[1] );
      return 0;
    case 106:   /* retail: eax=args[1] handle, edx=args[3] y; stack args[2]=x, args[4]=w, args[5]=h */
      CIN_SetExtents( args[1], args[2], args[3], args[4], args[5] );
      return 0;
    case 107:
      re_TrackStatistics( args[1] );
      return 0;
    case 108:
      return re_PickShader( args[1], args[2], args[3], args[4], args[5], args[6] );
    case 118:
      Com_Printf( "%s%i\n", (const char *)args[1], args[2] );
      return 0;
    case 119:
      Com_Printf( "%s%f\n", (const char *)args[1], *(float *)&args[2] );
      return 0;
    case 62:
      return re_GetEntityToken( args[1], args[2] );
    case 63:
      return com_hunkMeminfoRunningTotal;
    case 121:
      return VM_Call( uivm, 15, args[1], 5 );
    case 122:
    {
      const char *menu;

      if ( cls_state == 6 && !clc_demoplaying )
      {
        menu = (const char *)args[1];
        if ( menu )
        {
          if ( !Q_stricmp( menu, "UIMENU_WM_QUICKMESSAGE" ) )
          {
            VM_Call( uivm, 7, 8 );
          }
          else if ( !Q_stricmp( menu, "UIMENU_WM_AUTOUPDATE" ) )
          {
            VM_Call( uivm, 7, 9 );
          }
          /* Q_stricmpn is (s1@<edx>, s2@<ecx>, n@<eax>), no stack args, 0x00403562 */
          else if ( !Q_stricmpn( menu, "UIMENU_SCRIPT_POPUP", 19 ) )
          {
            if ( !Q_stricmp( menu, "UIMENU_SCRIPT_POPUP_NO_MOUSE" ) )
              return VM_Call( uivm, 7, 11 );
            return VM_Call( uivm, 7, 10 );
          }
        }
      }
      return 1;
    }
    case 123:
      if ( args[1]
        && ( ( !Q_stricmp( (const char *)args[1], "UIMENU_SCRIPT_POPUP_NO_MOUSE" ) && VM_Call( uivm, 8 ) == 10 )
          || ( !Q_stricmp( (const char *)args[1], "UIMENU_SCRIPT_POPUP" ) && VM_Call( uivm, 8 ) == 10 ) ) )
      {
        VM_Call( uivm, 3, 27, 1 );
      }
      return 0;
    case 124:
      if ( VM_Call( uivm, 8 ) == 1 )
        return 0;
      VM_Call( uivm, 3, 27, 1 );
      VM_Call( uivm, 3, 27, 1 );
      VM_Call( uivm, 3, 27, 1 );
      return 0;
    case 125:
      return VM_Call( uivm, 9, args[1] );
    case 126:
      return VM_Call( uivm, 10, args[1] );
    case 127:   /* no arguments; the returned pointer is the trap result, 0x004036D1 */
      return CL_GetServerIPAddress();
    case 219:
      Key_GetBindingBuf( args[1], (char *)args[2], args[3] );
      return 0;
    case 220:
      Key_SetBinding( args[1], (const char *)args[2] );
      return 0;
    case 221:
      Key_KeynumToStringBuf( args[1], (char *)args[2], args[3] );
      return 0;
    case 128:
      XAnimPrecacheAnimTree_m( (int *)args[1], (int (__cdecl *)(int))Hunk_AllocXAnimPrecache );
      return 0;
    case 129:
      return XAnimAllocTree( args[2], args[1], (int (__cdecl *)(int))Hunk_AllocXAnimClientCreate );
    case 130:
      XAnimSetLeafNode( (const char *)args[3], (void *)args[1], (unsigned short)args[2] );
      return 0;
    case 131:
      return XAnimAllocRuntimeTree( args[1], (int (__cdecl *)(int))Hunk_AllocXAnimClientCreateTree );
    case 132:
      XAnimSetParentNode(
        *( (unsigned short *)args + 10 ),
        *( (unsigned short *)args + 12 ),
        (void *)args[1],
        *( (unsigned short *)args + 4 ),
        args[3],
        *( (unsigned short *)args + 8 ) );
      return 0;
    case 133:
      DObjFree( 1, args[1] );
      return 0;
    case 134:
      XAnimClearAnimNode_m( (int *)args[1], args[2], *(float *)&args[3] );
      return 0;
    case 135:
      XAnimClearAnim_m( args[2], (int *)args[1], *(float *)&args[3] );
      return 0;
    case 136:
      XAnimClearAnimChildSubtrees_m( (void *)args[1], args[2], *(float *)&args[3] );
      return 0;
    case 137:
      XAnimSetAnimKnob_m(
        (int *)args[1],
        args[2],
        *(float *)&args[3],
        args[4],
        *(float *)&args[5],
        *( (unsigned short *)args + 12 ),
        0,
        args[7] );
      return 0;
    case 138:
      return XAnimSetAnimKnobAll_m(
               args[2],
               (int *)args[1],
               args[3],
               *(float *)&args[4],
               args[5],
               *(float *)&args[6],
               *( (unsigned short *)args + 14 ),
               0,
               args[8] );
    case 139:   /* nodeIndex (args[2]) is live-in via ECX in retail */
      XAnimClearAnimChildren_m( (int *)args[1], args[2], *(float *)&args[3] );
      return 0;
    case 140:
      XAnimSetAnimInternalLimited_m(
        *( (unsigned short *)args + 12 ),
        0,
        args[1],
        args[2],
        *(float *)&args[3],
        args[4],
        *(float *)&args[5],
        args[7] );
      return 0;
    case 141:
      XAnimSetAnimKnobInternal_m(
        (int *)args[1],
        args[2],
        *(float *)&args[3],
        *(float *)&args[4],
        *(float *)&args[5],
        0,
        *( (unsigned short *)args + 12 ),
        args[7] );
      return 0;
    case 142:
      XAnimSetAnimRate_m( args[1], args[2], *(float *)&args[3] );
      return 0;
    case 143:
      return XAnimIsLooped_m((const void *)(scrAnimPub_trees[128 * xanim_activePoolSlot + args[1]] ), args[2]);
    case 144:
      return XAnimIsLooped_m((const void *)(args[1] ), args[2]);
    case 145:
      XAnimSetTime_m( (void *)args[1], args[2], *(float *)&args[3] );
      return 0;
    case 146:
    {
      float t = XAnimGetTime( args[1], args[2] );
      return *(int *)&t;
    }
    case 147:
    {
      float w = XAnimGetWeight( args[1], args[2] );
      return *(int *)&w;
    }
    case 148:
      CL_DObjInvalidateSkels();   /* 0x004039E3; no arguments */
      return 0;
    case 149:
      DObjUpdateClientInfo_m( (void *)args[1], *(float *)&args[2] );
      return 0;
    case 150:
      *(int *)args[1] = (int)xanim_deferredNotifies;
      return xanim_numDeferredNotifies;
    case 151:
      DObjCalcAnim( args[1], args[2] );   /* dobj@<eax>, partBits@<edx>, 0x00403A25 */
      return 0;
    case 152:
      DObjDisplayAnim( (int **)args[1] );
      return 0;
    case 153:
      XAnimGetRelDelta_m( (int *)args[1], args[2], (float *)args[3], (int *)args[4], args[5] );
      return 0;
    case 154:
      XAnimGetAbsDelta_m( (int *)args[1], args[2], (float *)args[3], (int *)args[4] );
      return 0;
    case 155:
      XAnimGetRelDeltaForTime_m(
        scrAnimPub_trees[128 * xanim_activePoolSlot + args[1]],
        args[2],
        (int *)args[4],
        (float *)args[3],
        *(float *)&args[5],
        *(float *)&args[6] );
      return 0;
    case 156:
      XAnimGetAbsDeltaForTime_m((const void*)scrAnimPub_trees[128 * xanim_activePoolSlot + args[1]], args[2], (float*)(int *)args[4], (float*)(float *)args[3],
        *(float *)&args[5] );
      return 0;
    case 157:
      return ( *(unsigned char *)( args[2] + args[1] + 80 ) << 6 ) + *(int *)( args[1] + 4 ) + 48;
    case 158:
      return ( *(unsigned char *)( args[1] + 23 ) << 6 ) + *(int *)( args[1] + 4 ) + 48;
    case 159:
      return DObjMarkRotTransIndex( (void *)args[1], (const unsigned char *)args[2], args[3] );
    case 160:
      return DObjMarkControlRotTransIndex( (void *)args[1], (const unsigned char *)args[2], args[3] );
    case 161:
      return (int)XAnimGetAnimName( (const void*)scrAnimPub_trees[128 * xanim_activePoolSlot + args[1]], args[2] );
    case 162:
      return (int)Com_GetClientDObj( args[1] );   /* handle@<eax>, 0x00403BB3 */
    case 163:
      DObjCreate( (const void *)args[1], *( (unsigned short *)args + 4 ), (void *)args[3], (void *)args[4], 0 );
      return 0;
    case 164:
      Com_ClientDObjCreate( (void *)args[1], (unsigned short)args[2],
                            (void *)args[3], args[4], 0 );
      return 0;
    case 165:
      Com_SafeClientDObjFree( args[1], args[2] );
      return 0;
    case 166:
      return *(int *)args[1];
    case 167:
      return 96 * *(unsigned char *)( args[1] + 23 ) + 48;
    case 168:
      return CL_DObjCreateSkelForBone( args[1], args[2] );
    case 169:
      return CL_DObjCreateSkelForBones( args[1], args[2] );
    case 170:
      DObjGetHierarchyBits( args[2], args[1], (int *)args[3] );
      return 0;
    case 171:
      DObjCalcSkel( args[2], args[1] );   /* partBits@<eax>, dobj@<stack>, 0x00403CA5 */
      return 0;
    case 172:
      return XModelExists( (int *)args[1] );
    case 173:
      return *(unsigned char *)( args[1] + 23 );
    case 174:
      return DObjGetBoneIndex( (const char *)args[2], args[1] );
    case 175:
      return DObjGetBoneName( args[1], args[2] );
    case 176:
      DObjBuildPartCollisionTable( (int *)args[2], args[1] );
      return 0;
    case 177:
      return *(int *)args[1];
    case 178:
      return *(unsigned short *)( (int)scrAnimPub_trees[128 * xanim_activePoolSlot + args[1]] + 8 * args[2] + 8 ) == 0;
    case 179:
    {
      float *v = *(float **)( *(int *)( args[1] + 8 * args[2] + 12 ) + 4 );
      return (int)( (rfloat)*(unsigned short *)v / v[1] * 1000.0f );
    }
    case 180:
      return XAnimHasTime( args[1], args[2] );
    case 181:
      return *(unsigned short *)( (int)scrAnimPub_trees[128 * xanim_activePoolSlot + args[1]] + 8 * args[2] + 8 );
    case 182:
      return args[3] + *(unsigned short *)( (int)scrAnimPub_trees[128 * xanim_activePoolSlot + args[1]] + 8 * args[2] + 14 );
    case 183:
      return *(int *)( args[1] + 4 );
    case 184:
      XAnimCopyTree( args[1], args[2] );
      return 0;
    case 185:
      if ( com_developer->integer )
        DObjDumpInfo( (void *)args[1] );
      return 0;
    case 186:
      StatMon_Warning( args[1], args[2], (const char *)args[3] );
      return 0;
    case 187:   /* StatMon_GetStatsArray inlined in retail (0x00403ED6); its stats array is private to statmonitor.c */
      StatMon_GetStatsArray( (const statmonitor_t **)args[1], (int *)args[2] );
      return 0;
    case 188:
      return Z_MallocInternal( args[1] );
    case 189:
      free( (void *)args[1] );
      return 0;
    case 198:   /* CG_ADD_DEBUG_LINE, retail 0x00403F22 */
      CL_AddDebugLine(
        (const float *)args[1], (const float *)args[2], (const float *)args[3],
        args[4], args[5], 0 );
      return 0;
    case 199:   /* 0x00403F42: edx=args[1] size, esi=args[2] pPrevOwner, edi=2 owner */
      return (int)Com_GetWeaponInfoMemory( args[1], (int *)args[2], 2 );
    case 200:
      Com_FreeWeaponInfoMemory( 2, args[1] );
      return 0;
    case 201:
      return Hunk_AllocAlignInternal( args[1], 32 );
    case 202:
      return Hunk_AllocLowAlignInternal( args[1], 32 );
    case 203:
      return Hunk_AllocAlignInternal( args[1], args[2] );
    case 204:
      return Hunk_AllocLowAlignInternal( args[1], args[2] );
    case 205:
    case 206:
      return 0;
    case 207:
      return scrMemTree_allocBuckets;
    case 222:
      return FX_RegisterEffect( (char *)args[1] );
    case 223:
      return FX_GetBoneIndex( args[1], (const char *)args[2] );
    case 224:
      CFxScheduler__PlayEffect_name_simple_m( (char *)args[1], theFxScheduler, (float *)args[2] );
      return 0;
    case 225:
      CFxScheduler__PlayEffect_name_m( (int *)args[3], (char *)args[1], theFxScheduler, (float *)args[2] );
      return 0;
    case 226:
      CFxScheduler__PlayEffect_name_bolt_m( (char *)args[1], theFxScheduler, (float *)args[2], (int *)args[3], (int *)args[4] );
      return 0;
    case 227:
      CFxScheduler__PlayEffect_id_simple_m( args[1], (float *)args[2], theFxScheduler );
      return 0;
    case 228:
      FX_PlayEffectID( (int *)args[3], args[1], (float *)args[2] );
      return 0;
    case 229:
      CFxScheduler__PlayEffect_id_axis( theFxScheduler, args[1], (float *)args[2], (int *)args[3], (int *)args[4] );
      return 0;
    case 230:
      CFxScheduler__AddScheduledEffects( (unsigned int *)theFxScheduler );
      return 0;
    case 231:
      return FX_Init();
    case 232:
      return (unsigned char)FX_Free( 1 );
    case 233:
      return (unsigned char)FX_Free( 0 );
    case 234:
      SFxHelper__AdjustTime( &theFxHelper, args[1] );
      return 0;
    case 235:
    {
      float zfar = ( (double (__cdecl *)())re_GetFarPlaneDist )();
      SFxHelper__AdjustCamera( (int)&theFxHelper, args[1], zfar );
      return 0;
    }
    case 240:
      cgameMaxPitchSpeedStorage = args[1];
      cgameMaxYawSpeedStorage = args[2];
      return 0;
    case 241:
      if ( cls_state == 6 )   /* retail 0x004041B6 cmp cls_state, 6 */
        CL_FirstSnapshot();
      return 0;
    default:
      Com_Error( ERR_DROP, "\x15" "Bad cgame system trap: %i", *args );
  }

  return 0;
}

/* ---- CL_UpdateLevelHunkUsage  0x004045C0 ----  VERIFIED */
void CL_UpdateLevelHunkUsage( void ) {
	const char      *memlistfile = "hunkusage.dat";
	fileHandle_t    handle;
	char            *buf;
	char            *outbuf;
	char            *buftrav;
	char            *token;
	char            outstr[256];
	int             len;
	int             memusage;

	memusage = com_hunkMeminfoRunningTotal;

	len = FS_FOpenFileByMode( memlistfile, &handle, FS_READ );
	if ( len >= 0 ) {
		buf = (char *)Z_MallocInternal( len + 1 );
		memset( buf, 0, len + 1 );
		outbuf = (char *)Z_MallocInternal( len + 1 );
		memset( outbuf, 0, len + 1 );

		FS_Read( buf, len, handle );
		FS_FCloseFile( handle );

		buftrav = buf;
		outbuf[0] = '\0';

		while ( ( token = Com_Parse( &buftrav ) ) != 0 && token[0] ) {
			if ( !Q_strcasecmp( token, cl_mapnameBsp ) ) {
				token = Com_Parse( &buftrav );
				if ( token && token[0] ) {
					if ( atoi( token ) == memusage ) {
						free( buf );
						free( outbuf );
						return;
					}
				}
			} else {
				Q_strcat( outbuf, len + 1, token );
				Q_strcat( outbuf, len + 1, " " );
				token = Com_Parse( &buftrav );
				if ( token && token[0] ) {
					Q_strcat( outbuf, len + 1, token );
					Q_strcat( outbuf, len + 1, "\n" );
				} else {
					Com_Error( ERR_DROP, "EXE_ERR_HUNGUSAGE_CORRUPT" );
				}
			}
		}

		handle = FS_FOpenFileWrite( memlistfile );
		if ( handle < 0 ) {
			Com_Error( ERR_DROP,
					   va( "EXE_ERR_CANT_CREATE" "\x15" "%s", memlistfile ) );
		}

		len = strlen( outbuf );
		if ( FS_Write( outbuf, len, handle ) != len ) {
			Com_Error( ERR_DROP,
					   va( "EXE_ERR_CANT_WRITE" "\x15" "%s", memlistfile ) );
		}
		FS_FCloseFile( handle );

		free( buf );
		free( outbuf );
	}

	FS_FOpenFileByMode( memlistfile, &handle, FS_APPEND );
	if ( handle < 0 ) {
		Com_Error( ERR_DROP, "EXE_ERR_HUNKUSAGE_CANT_WRITE" );
	}
	Com_sprintf( outstr, sizeof( outstr ), "%s %i\n", cl_mapnameBsp, memusage );
	FS_Write( outstr, strlen( outstr ), handle );
	FS_FCloseFile( handle );

	if ( FS_FOpenFileByMode( memlistfile, &handle, FS_READ ) >= 0 ) {
		FS_FCloseFile( handle );
	}
}

#define CG_INIT                 0   /* 0x00404AA8 */
#define CG_SCRIPT_HOOK          16  /* 0x00404A86, takes Scr_NearHook(NULL) */

/* ---- CL_InitCGame  0x004049C0 ----  VERIFIED */
void CL_InitCGame( void )
{
	int start;
	int msec;
	const char *info;
	const char *mapname;

	start = Sys_Milliseconds();

	if ( !com_sv_running->integer ) {
		Com_InitDObj();
	}

	/* 0x00A9CC58. qcommon/cmd.c sets the same word before handing a command to the game module. */
	xanim_activePoolSlot = 0;

	Con_Close();

	info = (const char *)&cl_gameState_stringData[cl_gameState_stringOffsets[0]];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl_mapnameBsp, sizeof( cl_mapnameBsp ), "maps/mp/%s.bsp", mapname );

	cgvm = VM_Create( "cgame", CL_CgameSystemCalls );
	if ( !cgvm ) {
		Com_Error( ERR_DROP, "\x15VM_Create on cgame failed" );
	}

	cls_state = 4;

	VM_Call( cgvm, CG_SCRIPT_HOOK, Scr_NearHook( 0 ) );
	VM_Call( cgvm, CG_INIT, clc_serverMessageSequence, clc_lastExecutedServerCommand, clc_clientNum );

	cls_state = 5;

	msec = Sys_Milliseconds() - start;
	Com_Printf( "CL_InitCGame: %5.2f seconds\n", (double)msec * 0.001 );

	re_EndRegistration();

	if ( sys_sysMBValue > 100663296 ) {   /* 0x6000000 */
		Com_TouchMemory();
	}

	memset( (void *)con_notifyWindow, 0, 4 * con_gamemessagelines );
	memset( (void *)con_notifyWindowEndTimes, 0, 4 * con_gamemessagelines );
	con_notifyWindowIndex = 0;
	memset( (void *)con_boldWindow, 0, 4 * con_boldWindowSize );
	memset( (void *)con_boldWindowEndTimes, 0, 4 * con_boldWindowSize );
	con_boldWindowIndex = 0;
	memset( (void *)con_subtitleWindow, 0, 4 * con_subtitleWindowSize );
	memset( (void *)con_subtitleWindowEndTimes, 0, 4 * con_subtitleWindowSize );
	con_subtitleWindowIndex = 0;

	CL_UpdateLevelHunkUsage();
}

/* ---- CL_GameCommand  0x00404BA0 ----  [CONFIRMED] */
qboolean CL_GameCommand( void ) {
	if ( !cgvm ) {
		return qfalse;
	}

	return VM_Call( cgvm, CG_CONSOLE_COMMAND );
}

/* ---- CL_CGameRendering  0x00404BC0 ----  [CONFIRMED] */
void __cdecl CL_CGameRendering(int stereoFrame)
{
  VM_Call(cgvm, 3, cl_serverTime, stereoFrame, clc_demoplaying, 0, 0);
}

extern int cl_newSnapshots;             /* cl.newSnapshots 0x01434A78 */
extern int cl_serverTimeDelta;          /* cl.serverTimeDelta 0x01434A70 */
extern int cl_oldServerTime;            /* cl.oldServerTime 0x01434A68 */
extern int cl_oldFrameServerTime;               /* cl.oldFrameServerTime 0x01434A6C */
extern int cl_extrapolatedSnapshot;     /* cl.extrapolatedSnapshot 0x01434A74 */
extern int cl_snap_valid;               /* cl.snap.valid 0x01432964 */
extern unsigned char cl_snap_snapFlags[4];  /* cl.snap.snapFlags 0x01432968 */
extern cvar_t *activeAction;            /* cl_activeAction 0x0155F24C */
extern cvar_t *cl_freezeDemo;           /* 0x015F7074 */
extern cvar_t *cl_paused;               /* 0x0163A22C */
extern cvar_t *cl_showTimeDelta;        /* 0x015F7078 */
extern cvar_t *cl_timeNudge;            /* 0x01617308 */
extern cvar_t *sv_paused;               /* 0x0163B3C8 */
extern cvar_t *timedemo;                /* cl_timedemo 0x015F706C */

#define RESET_TIME              500

#define SNAPFLAG_NOT_ACTIVE     2

#define CA_LOADING_UNUSED       4
#define CA_PRIMED_STATE         5

/* ---- CL_AdjustTimeDelta  0x00404BF0 ----  VERIFIED */
void CL_AdjustTimeDelta( void ) {
	int newDelta;
	int deltaDelta;

	cl_newSnapshots = qfalse;

	if ( clc_demoplaying ) {
		return;
	}

	newDelta = cl_snap_serverTime - cls_realtime;
	deltaDelta = abs( newDelta - cl_serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl_serverTimeDelta = newDelta;
		cl_oldServerTime = cl_snap_serverTime;
		cl_serverTime = cl_snap_serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl_serverTimeDelta = ( cl_serverTimeDelta + newDelta ) >> 1;
	} else {
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl_extrapolatedSnapshot ) {
				cl_extrapolatedSnapshot = qfalse;
				cl_serverTimeDelta -= 2;
			} else {
				cl_serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl_serverTimeDelta );
	}
}

/* ---- CL_FirstSnapshot  0x00404D00 ----  VERIFIED */
void CL_FirstSnapshot( void ) {
	if ( *(int *)cl_snap_snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}

	cls_state = CA_ACTIVE;

	cl_serverTimeDelta = cl_snap_serverTime - cls_realtime;
	cl_oldServerTime = cl_snap_serverTime;

	clc_timeDemoBaseTime = cl_snap_serverTime;

	if ( *activeAction->string ) {
		Cbuf_AddText( activeAction->string );
		Cvar_Set2( "activeAction", "", qtrue );
	}
}

/* ---- CL_SetCGameTime  0x00404D60 ----  VERIFIED */
void CL_SetCGameTime( void ) {
	int tn;

	if ( cls_state != CA_ACTIVE ) {
		if ( cls_state != CA_PRIMED_STATE ) {
			return;
		}
		if ( clc_demoplaying ) {
			if ( !clc_firstDemoFrameSkipped ) {
				clc_firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if ( cl_newSnapshots ) {
			cl_newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if ( cls_state != CA_ACTIVE ) {
			return;
		}
	}

	if ( !cl_snap_valid ) {
		Com_Error( ERR_DROP, "\x15" "CL_SetCGameTime: !cl.snap.valid" );
	}

	if ( sv_paused->integer && cl_paused->integer && com_sv_running->integer ) {
		return;
	}

	if ( cl_snap_serverTime < cl_oldFrameServerTime ) {
		if ( !Q_stricmpn( cls_servername, "localhost", 99999 ) ) {
			CL_FirstSnapshot();
		} else {
			Com_Error( ERR_DROP,
					   "\x15" "cl.snap.serverTime < cl.oldFrameServerTime" );
		}
	}
	cl_oldFrameServerTime = cl_snap_serverTime;

	if ( clc_demoplaying && cl_freezeDemo->integer ) {
	} else {
		tn = cl_timeNudge->integer;
		if ( tn < -30 ) {
			tn = -30;
		} else if ( tn > 30 ) {
			tn = 30;
		}

		cl_serverTime = cls_realtime + cl_serverTimeDelta - tn;

		if ( cl_serverTime < cl_oldServerTime ) {
			cl_serverTime = cl_oldServerTime;
		}
		cl_oldServerTime = cl_serverTime;

		if ( cls_realtime + cl_serverTimeDelta >= cl_snap_serverTime - 5 ) {
			cl_extrapolatedSnapshot = qtrue;
		}
	}

	if ( cl_newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc_demoplaying ) {
		return;
	}

	if ( timedemo->integer ) {
		if ( !clc_timeDemoStart ) {
			clc_timeDemoStart = Sys_Milliseconds();
		}
		( clc_timeDemoFrames )++;
		cl_serverTime = clc_timeDemoBaseTime + clc_timeDemoFrames * 50;
	}

	while ( cl_serverTime >= cl_snap_serverTime ) {
		CL_ReadDemoMessage();
		if ( cls_state != CA_ACTIVE ) {
			return;
		}
	}
}

#define CG_PROFILE_DRAW_TRAP    15  /* 0x00404F74; cl_net_chan_mp.c spells it the same */

/* ---- CL_DrawString  0x00404F50 ----  VERIFIED
 *
 * Retail pushes ECX, EDX, [esp+14h], [esp+10h], [esp+0Ch], [esp+8], [esp+4], 0Fh.
 * Under /LTCG the register pair is chosen freely, so ECX/EDX are not params 1 and 2.
 *
 * vmMain export 15 (0x3002042B): arg0 -> x, arg1 -> y, arg2 -> string,
 * arg3 -> EAX, arg4 -> charWidth, arg5 -> charHeight, arg6 -> maxChars, with
 * arg0/1/4/5 converted by `fild`.  So the five stack slots are x, y, text, arg3,
 * charWidth and the two register slots are charHeight (EDX) and maxChars (ECX).
 * CL_ProfDraw (0x00414900) reaches the same trap with 32, y, text, 0, 8, 10, 0.
 *
 * arg3 is SHADOW, not forceColor: CG_DrawStringExt takes it in EAX and does
 * neg/sbb/and 3 (0x300196C7) to pick text style 3 or 0.  forceColor is the dead
 * constant 0 in the cgame's fourth stack slot.
 *
 * 0x00404F50 has no code or data reference anywhere in the image, so the source
 * order of the seven parameters is unrecoverable; they are written here in the
 * trap's order with __fastcall dropped, as cl_net_chan_mp.c does for the
 * same register pair.
 */
int CL_DrawString(int x, int y, const char *text, int shadow, int charWidth, int charHeight, int maxChars)
{
  int result;

  result = (int)cgvm;
  if ( cgvm )
    return VM_Call(cgvm, CG_PROFILE_DRAW_TRAP, x, y, text, shadow, charWidth, charHeight, maxChars);
  return result;
}

/* ---- CL_SaveCgameState  0x00404F80 ----  VERIFIED */
int CL_SaveCgameState( void *buffer, int size )
{
  int used;

  used = re_SaveFogState( buffer, size );
  return used + VM_Call( cgvm, 17, (char *)buffer + used, size - used );
}

/* ---- CL_RestoreCgameState  0x00404FB0 ----  VERIFIED */
int CL_RestoreCgameState( void *buffer, int size )
{
  int used;

  used = re_RestoreFogState( buffer, size );
  return used + VM_Call( cgvm, 18, (char *)buffer + used, size - used );
}

/* 0x015CA590, 160 bytes: cls.glconfig (cls is defined in cl_main_mp.c). */

extern int cl_gameState_stringOffsets[];

/* ---- CL_UnusedStub1  0x00401030 ----  VERIFIED */
/* retail 0x00401030, IDA nullsub_8: bare retn, no xrefs. game/cgame/ui carry the same unnamed pair of empty stubs + float identity (G_/CG_/UI_UnusedStub*, *_UnusedFloatIdentity): name lost, descriptive name. */
void CL_UnusedStub1( void ) {
}

/* ---- CL_UnusedStub2  0x00401040 ----  VERIFIED */
/* retail 0x00401040, IDA nullsub_9: bare retn, no xrefs. game/cgame/ui carry the same unnamed pair of empty stubs + float identity (G_/CG_/UI_UnusedStub*, *_UnusedFloatIdentity): name lost, descriptive name. */
void CL_UnusedStub2( void ) {
}

/* ---- CL_UnusedFloatIdentity  0x00401050 ----  VERIFIED */
/* retail 0x00401050, IDA sub_401050: fld [esp+4]; retn, no xrefs; same trio as G_/CG_/UI_UnusedFloatIdentity in the DLLs: descriptive name. */
float CL_UnusedFloatIdentity( float f ) {
	return f;
}

/* ---- CL_GetGameState  0x00401060 ----  VERIFIED */
void CL_GetGameState( void *gs ) {
	memcpy( gs, cl_gameState_stringOffsets, 0x5E84 );
}

/* ---- CL_GetGlconfig_cgame  0x00401080 ----  VERIFIED */
void CL_GetGlconfig_cgame( glconfig_t *config ) {
	*config = cls_glconfig;
}

/* ---- CL_ShutdownCGame  0x00401A20 ----  [CONFIRMED] */
void CL_ShutdownCGame( void ) {
	cls_keyCatchers &= ~KEYCATCH_CGAME;

	if ( !cgvm ) {
		return;
	}

	VM_Call( cgvm, CG_SHUTDOWN );
	VM_Free( cgvm );
	cgvm = NULL;

	if ( snd_aliasSetLoaded[SND_LOCALE_INGAME] ) {
		Com_UnloadSoundAliasSounds( SND_LOCALE_INGAME );

		if ( snd_aliasTable[SND_LOCALE_INGAME] ) {
			if ( !snd_aliasTable[SND_LOCALE_LOCALIZED] ) {
				free( (void *)snd_aliasTable[SND_LOCALE_INGAME] );
			}
			snd_aliasTable[SND_LOCALE_INGAME]      = NULL;
			snd_aliasTableCount[SND_LOCALE_INGAME] = 0;
			memset( snd_aliasHashTable[SND_LOCALE_INGAME], 0,
					SND_ALIAS_HASH_SLICE_BYTES );
		}

		snd_aliasSetLoaded[SND_LOCALE_INGAME] = 0;

		if ( !snd_aliasSetLoaded[SND_LOCALE_MENU] ) {
			Cmd_RemoveCommand( "snd_list" );
		}
	}
}
