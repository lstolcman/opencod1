/*
 * @fidelity: verified
 */

#include "server.h"

#include <string.h>
#include <stdlib.h>

extern void Hunk_FreeTempMemory( void *buf );   /* universal/com_memory.c */

vm_t        *vm;                                /* retail 0x014073D0 */
cvar_t      *sv_master[MAX_MASTER_SERVERS];    /* retail 0x016BDB40 */
int         sv_rollingServerId;                 /* retail 0x014073DC, rolling sv_serverid */

/* Tree-only global, no longer registered or read: retail CoDMP.exe has no
 * sv_debugSpawn cvar (SV_Init 0x00459130 ends at sv_mapRotationCurrent). */
cvar_t      *sv_debugSpawn;

extern void FS_ClearMemory( void );

/* Retail pushes the pooled "" literal (0x00559228), not a pointer variable. */
#define sv_emptyString  ""

/* ---- SV_SetConfigstring  0x00457BE0 ---- */
void SV_SetConfigstring( unsigned int index, const char *val ) {
	int i;
	int len;
	client_t    *client;

	/* Retail 0x00457BFB tests the index signed (jl/jl). */
	if ( (int)index < 0 || (int)index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "\x15SV_SetConfigstring: bad index %i\n", index );
	}

	if ( !val ) {
		val = sv_emptyString;
	}

	if ( !strcmp( val, sv.configstrings[index] ) ) {
		return;
	}

	free( sv.configstrings[index] );
	sv.configstrings[index] = CopyStringInternal( val );

	if ( sv.state == SS_GAME || sv.restarting ) {
		for ( i = 0, client = svs.clients ; i < sv_maxclients->integer ; i++, client++ ) {
			if ( client->state < CS_PRIMED ) {
				continue;
			}

			len = strlen( val );
			if ( len >= 1000 ) {
				int sent = 0;
				int remaining = len;
				const char  *cmd;
				char buf[MAX_STRING_CHARS];

				while ( remaining > 0 ) {
					if ( sent == 0 ) {
						cmd = "x";
					} else if ( remaining < 1000 ) {
						cmd = "z";
					} else {
						cmd = "y";
					}
					Q_strncpyz( buf, &val[sent], 1000 );

					SV_SendServerCommand( client, qtrue, "%s %i %s", cmd, index, buf );

					sent += ( 1000 - 1 );
					remaining -= ( 1000 - 1 );
				}
			} else {
				SV_SendServerCommand( client, qtrue, "d %i %s", index, val );
			}
		}
	}
}

/* ---- SV_GetConfigstring  0x00457E10 ---- */
void SV_GetConfigstring( unsigned int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "\x15SV_GetConfigstring: bufferSize == %i", bufferSize );
	}
	/* Retail 0x00457E25 tests the index signed (jl/jl), like SV_SetConfigstring. */
	if ( (int)index < 0 || (int)index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "\x15SV_GetConfigstring: bad index %i\n", index );
	}

	if ( !sv.configstrings[index] ) {
		buffer[0] = 0;
		return;
	}

	strncpy( buffer, sv.configstrings[index], bufferSize - 1 );
	buffer[bufferSize - 1] = 0;
}

/* ---- SV_GetConfigstringConstPtr_m  0x00457E70 ----
 * The G_GET_CONFIGSTRING_CONST trap's worker: retail SV_GameSystemCalls
 * tail-jumps here with the index in EAX (LTCG register argument). */
const char *SV_GetConfigstringConstPtr_m( int index ) {
	if ( sv.configstrings[index] ) {
		return sv.configstrings[index];
	}
	return sv_emptyString;
}

/* ---- SV_GetConfigstringConst  0x00457E90 ---- */
const char *SV_GetConfigstringConst( int start, const char *key, int max ) {
	int i;

	for ( i = 0 ; i < max ; i++ ) {
		if ( !sv.configstrings[start + i][0] ) {
			break;
		}
		/* Retail 0x00457EB1 calls the CRT _stricmp, not Q_stricmp. */
		if ( !_stricmp( key, sv.configstrings[start + i] ) ) {
			return sv.configstrings[start + max + i];
		}
	}

	return sv_emptyString;
}

/* ---- SV_SetConfigValueForKey  0x00457EF0 ---- */
void SV_SetConfigValueForKey( int start, const char *key, const char *value, int max ) {
	int i;

	for ( i = 0 ; i < max ; i++ ) {
		if ( !sv.configstrings[start + i][0] ) {
			SV_SetConfigstring( start + i, key );
			break;
		}
		/* Retail 0x00457F11 calls the CRT _stricmp, not Q_stricmp. */
		if ( !_stricmp( key, sv.configstrings[start + i] ) ) {
			break;
		}
	}

	if ( i == max ) {
		Com_Error( ERR_DROP, "\x15SV_SetConfigValueForKey: overflow" );
	}

	SV_SetConfigstring( max + start + i, value );
}

/* ---- SV_SetUserinfo  0x00457F70 ---- */
void SV_SetUserinfo( int index, const char *userinfo ) {
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "\x15SV_SetUserinfo: bad index %i\n", index );
	}

	if ( !userinfo ) {
		userinfo = sv_emptyString;
	}

	Q_strncpyz( svs.clients[index].userinfo, userinfo, sizeof( svs.clients[index].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( userinfo, "name" ), sizeof( svs.clients[index].name ) );
}

/* ---- SV_GetUserinfo  0x00458000 ---- */
void SV_GetUserinfo( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "\x15SV_GetUserinfo: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "\x15SV_GetUserinfo: bad index %i\n", index );
	}

	strncpy( buffer, svs.clients[index].userinfo, bufferSize - 1 );
	buffer[bufferSize - 1] = 0;
}

/* ---- SV_CreateBaseline  0x00458060 ---- */
void SV_CreateBaseline( void ) {
	gentity_t   *svent;
	svEntity_t     *base;
	int entnum;

	for ( entnum = 1 ; entnum < sv.num_entities ; entnum++ ) {
		svent = (gentity_t *)( (byte *)sv.gentities + entnum * sv.gentitySize );

		if ( !svent->r.linked ) {
			continue;
		}

		svent->s.number = entnum;

		base = &sv.svEntities[entnum];
		base->baseline = svent->s;
		base->baselineSvFlags = svent->r.svFlags;
		base->baselineSingleClient = svent->r.singleClient;
		VectorCopy( svent->r.absmin, base->baselineAbsmin );
		VectorCopy( svent->r.absmax, base->baselineAbsmax );
	}
}

/* ---- SV_BoundMaxClients  0x00458110 ---- */
void SV_BoundMaxClients( int minimum ) {
	Cvar_Get( "sv_maxclients", "20", 0 );
	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		Cvar_Set2( "sv_maxclients", va( "%i", minimum ), qtrue );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		Cvar_Set2( "sv_maxclients", va( "%i", MAX_CLIENTS ), qtrue );
	}
}

/* ---- SV_Startup  0x00458160 ---- */
void SV_Startup( void ) {
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "\x15SV_Startup: svs.initialized" );
	}

	SV_BoundMaxClients( 1 );

	svs.clients = (client_t *)calloc( sv_maxclients->integer * sizeof( client_t ), 1 );
	if ( !svs.clients ) {
		Com_Error( ERR_FATAL, "\x15SV_Startup: unable to allocate svs.clients" );
	}

	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
		svs.numSnapshotClients = sv_maxclients->integer * sv_maxclients->integer * PACKET_BACKUP;
	} else {
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
		svs.numSnapshotClients = sv_maxclients->integer * sv_maxclients->integer * 4;
	}
	svs.initialized = qtrue;
	Cvar_Set2( "sv_running", "1", qtrue );
}

/* ---- SV_ChangeMaxClients  0x00458220 ---- */
void SV_ChangeMaxClients( void ) {
	int i;
	int count;
	client_t *current;
	int oldMaxClients;
	client_t    *oldClients;

	count = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if ( i > count ) {
				count = i;
			}
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	SV_BoundMaxClients( count );
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	oldClients = (client_t *)Hunk_AllocateTempMemoryInternal( count * sizeof( client_t ) );
	for ( i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		} else {
			Com_Memset( &oldClients[i], 0, sizeof( client_t ) );
		}
	}

	free( svs.clients );
	svs.clients = (client_t *)calloc( sv_maxclients->integer * sizeof( client_t ), 1 );
	if ( !svs.clients ) {
		Com_Error( ERR_FATAL, "\x15SV_Startup: unable to allocate svs.clients" );
	}
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof( client_t ) );

	for ( i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	Hunk_FreeTempMemory( oldClients );

	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * 64;
		svs.numSnapshotClients = sv_maxclients->integer * sv_maxclients->integer * PACKET_BACKUP;
	} else {
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * 64;
		svs.numSnapshotClients = sv_maxclients->integer * sv_maxclients->integer * 4;
	}
}

/* ---- SV_SetExpectedHunkUsage  0x00458450 ---- */
void SV_SetExpectedHunkUsage( const char *mapname ) {
	int len;
	fileHandle_t f;
	char        *buf;
	char        *parse;
	char        *token;

	len = FS_FOpenFileByMode( "hunkusage.dat", &f, FS_READ );
	if ( len >= 0 ) {
		buf = (char *)Z_MallocInternal( len + 1 );
		memset( buf, 0, len + 1 );
		FS_Read( buf, len, f );
		FS_FCloseFile( f );

		parse = buf;
		while ( 1 ) {
			/* Com_Parse, inlined by retail at 0x004584C0 and 0x00458559: a
			 * pending parseInfo->ungetReady is honoured before Com_ParseExt. */
			token = Com_Parse( &parse );
			if ( !token || !token[0] ) {
				break;
			}

			if ( Q_strcasecmp( token, (char *)mapname ) ) {
				continue;
			}

			token = Com_Parse( &parse );
			if ( token && token[0] ) {
				Cvar_Set2( "com_expectedhunkusage", token, qtrue );
				free( buf );
				return;
			}
		}

		free( buf );
	}
	Cvar_Set2( "com_expectedhunkusage", "-1", qtrue );
}

/* ---- SV_ClearServer  0x00458600 ---- */
void SV_ClearServer( void ) {
	int i;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( sv.configstrings[i] ) {
			free( sv.configstrings[i] );
		}
	}

	Com_Memset( &sv, 0, sizeof( sv ) );
}

/* ---- SV_EnableArchivedSnapshot  0x00458640 ---- */
void SV_EnableArchivedSnapshot( qboolean enable ) {
	svs.archiveEnabled = enable;
	if ( !enable ) {
		return;
	}
	if ( svs.archivedSnapshotFrames ) {
		return;
	}

	svs.cachedSnapshotFrames   = (cachedSnapshot_t *)Z_MallocInternal( SV_CACHED_SNAPSHOT_FRAMES_BYTES );
	svs.cachedSnapshotEntities = (archivedEntity_t *)Z_MallocInternal( SV_CACHED_SNAPSHOT_ENTITIES_BYTES );
	svs.archivedSnapshotFrames = (archivedSnapshot_t *)Z_MallocInternal( SV_ARCHIVED_SNAPSHOT_FRAMES_BYTES );
	svs.archivedSnapshotBuffer = (byte *)Z_MallocInternal( SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES );
	svs.cachedSnapshotClients  = (cachedClient_t *)Z_MallocInternal( SV_CACHED_SNAPSHOT_CLIENTS_BYTES );
}

/* ---- SV_InitArchivedSnapshot  0x004586B0 ---- */
void SV_InitArchivedSnapshot( void ) {
	svs.archiveEnabled = qfalse;
	svs.nextArchivedSnapshotFrames = 0;
	svs.nextArchivedSnapshotBuffer = 0;
	svs.nextCachedSnapshotEntities = 0;
	svs.nextCachedSnapshotClients = 0;
	svs.nextCachedSnapshotFrames = 0;
}

/* ---- SV_FreeArchivedSnapshot  0x004586E0 ---- */
void SV_FreeArchivedSnapshot( void ) {
	if ( svs.cachedSnapshotFrames ) {
		free( svs.cachedSnapshotFrames );
		svs.cachedSnapshotFrames = NULL;
	}
	if ( svs.cachedSnapshotEntities ) {
		free( svs.cachedSnapshotEntities );
		svs.cachedSnapshotEntities = NULL;
	}
	if ( svs.archivedSnapshotFrames ) {
		free( svs.archivedSnapshotFrames );
		svs.archivedSnapshotFrames = NULL;
	}
	if ( svs.archivedSnapshotBuffer ) {
		free( svs.archivedSnapshotBuffer );
		svs.archivedSnapshotBuffer = NULL;
	}
	if ( svs.cachedSnapshotClients ) {
		free( svs.cachedSnapshotClients );
		svs.cachedSnapshotClients = NULL;
	}
}

/* ---- SV_SpawnServer  0x00458760 ---- */
void SV_SpawnServer( const char *server ) {
	int i;
	int checksum;
	int matchState;
	char systemInfo[MAX_STRING_CHARS];
	client_t    *client;

	xmodel_enforceExist = Cvar_Get( "cl_xmodelcheck", "0", CVAR_ARCHIVE | CVAR_LATCH )->integer;

	if ( Cvar_VariableValue( "sv_running" ) ) {
		matchState = VM_Call( vm, GAME_GET_MATCH_STATE );

		for ( i = 0, client = svs.clients ; i < sv_maxclients->integer ; i++, client++ ) {
			if ( client->state >= CS_PRIMED ) {
				/* Retail 0x004587F5: the packet is built with va() and passed
				 * as the FORMAT, with no varargs after it. */
				NET_OutOfBandPrint( NS_SERVER, client->netchan.remoteAddress,
									va( "loadingnewmap\n%s\n%s", server, sv_gametype->string ) );
			}
		}

		NET_Sleep( 250 );
	} else {
		matchState = 0;
	}

	CL_MapLoading();
	CL_ShutdownCGame();
	CL_ShutdownUI();
	if ( re_ShutdownPtr ) {
		re_ShutdownPtr( 0 );
	}
	cls_loadingPlaque = 0;
	cls_rendererStarted = 0;
	SV_ShutdownGameProgs();

	Com_Printf( "------ Server Initialization ------\n" );
	Com_Printf( "Server: %s\n", server );

	Hunk_Clear();
	SV_ClearServer();

	FS_Shutdown( qtrue );
	FS_ClearPakReferences( 0 );

	srand( Sys_Milliseconds() );
	sv.checksumFeed = ( ( (int)rand() << 16 ) ^ rand() ) ^ Sys_Milliseconds();
	FS_Restart( sv.checksumFeed );

	FS_ClearMemory();

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		sv.configstrings[i] = CopyStringInternal( sv_emptyString );
	}

	if ( !Cvar_VariableValue( "sv_running" ) ) {
		Com_Restart();
		SV_Startup();
	} else {
		/* Com_InitDObj (0x00438B70), inlined by retail at 0x00458BA4..0x00458CAD. */
		Com_Memset( com_dobjAllocBits, 0, sizeof( com_dobjAllocBits ) );
		Com_Memset( com_clientDObjHandles, 0, sizeof( com_clientDObjHandles ) );
		Com_Memset( com_serverDObjHandles, 0, sizeof( com_serverDObjHandles ) );
		com_dobjLastIndex = 1;   /* retail 0x00894240, also named com_dobjPoolInited (never read) */
		com_dobjInited = 1;
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	svs.snapshotEntities = (entityState_t *)Hunk_AllocAlignInternal(
		sizeof( entityState_t ) * svs.numSnapshotEntities, 32 );
	svs.nextSnapshotEntities = 0;
	svs.snapshotClients = Hunk_AllocAlignInternal(
		SV_SNAPSHOT_CLIENT_SIZE * svs.numSnapshotClients, 32 );
	svs.nextSnapshotClients = 0;
	SV_InitArchivedSnapshot();

	svs.snapFlagServerBit ^= 4;

	Cvar_Set2( "nextmap", "map_restart", qtrue );
	SV_SetExpectedHunkUsage( va( "maps/mp/%s.bsp", server ) );
	Cvar_Set2( "cl_paused", "0", qtrue );

	CM_LoadMap( va( "maps/mp/%s.bsp", server ), qfalse, &checksum );
	Cvar_Set2( "mapname", server, qtrue );

	sv_rollingServerId = (unsigned char)( sv_rollingServerId + 16 );
	if ( ( sv_rollingServerId & 0xF0 ) == 0 ) {
		sv_rollingServerId += 16;
	}
	Cvar_Set2( "sv_serverid", va( "%i", sv_rollingServerId ), qtrue );

	sv.serverId = com_frameTime;
	sv.state = SS_LOADING;
	Cvar_Set2( "sv_serverRestarting", "1", qtrue );

	Com_LoadSoundAliases( va( "maps/mp/%s.bsp", server ), 2 );

	xanim_activePoolSlot = 1;
	SV_InitGameProgs( matchState );

	for ( i = 0 ; i < 3 ; i++ ) {
		svs.time += 100;
		VM_Call( vm, GAME_UPDATE_CVARS );
		if ( !++com_skelTimeStamp ) {
			com_skelTimeStamp = 1;
		}
		com_skelInvalidated = 1;
		VM_Call( vm, GAME_RUN_FRAME, svs.time );
		com_skelInvalidated = 0;
		if ( s_hunkData ) {
			SV_HUNK_LOW_TEMP = SV_HUNK_LOW_PERM;
		}
	}

	SV_CreateBaseline();

	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			const char  *reject;

			reject = (const char *)VM_Call( vm, GAME_CLIENT_CONNECT, i, svs.clients[i].scriptId );
			if ( reject ) {
				SV_DropClient( &svs.clients[i], reject );
			} else {
				svs.clients[i].state = CS_CONNECTED;
			}
		}
	}

	if ( sv_pure->integer ) {
		const char  *sums = FS_LoadedPakChecksums();

		Cvar_Set2( "sv_paks", sums, qtrue );
		if ( strlen( sums ) == 0 ) {
			Com_Printf( "WARNING: sv_pure set but no PK3 files loaded\n" );
		}
		Cvar_Set2( "sv_pakNames", FS_LoadedPakNames(), qtrue );
	} else {
		Cvar_Set2( "sv_paks", sv_emptyString, qtrue );
		Cvar_Set2( "sv_pakNames", sv_emptyString, qtrue );
	}

	Cvar_Set2( "sv_referencedPaks", FS_ReferencedPakChecksums(), qtrue );
	Cvar_Set2( "sv_referencedPakNames", FS_ReferencedPakNames(), qtrue );

	Q_strncpyz( systemInfo, Cvar_InfoString_Big( CVAR_SYSTEMINFO ), sizeof( systemInfo ) );
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring( CS_SYSTEMINFO, systemInfo );

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	SV_SetConfig( CS_SCRIPT_KV_BASE, CS_SCRIPT_KV_COUNT, CVAR_WOLFINFO );
	cvar_modifiedFlags &= ~CVAR_WOLFINFO;

	sv.state = SS_GAME;
	svs.nextHeartbeatTime = -9999999;
	Cvar_Set2( "sv_serverRestarting", "0", qtrue );
	Com_Printf( "-----------------------------------\n" );
}

/* ---- SV_Init  0x00459130 ---- */
void SV_Init( void ) {
	SV_AddOperatorCommands();

	sv_gametype = Cvar_Get( "g_gametype", "dm", CVAR_LATCH | CVAR_SERVERINFO );
	Cvar_Get( "sv_keywords", "", CVAR_SERVERINFO );
	Cvar_Get( "protocol", va( "%i", PROTOCOL_VERSION ), CVAR_ROM | CVAR_SERVERINFO );
	sv_mapname = Cvar_Get( "mapname", "nomap", CVAR_ROM | CVAR_SERVERINFO );
	sv_privateClients = Cvar_Get( "sv_privateClients", "0", CVAR_SERVERINFO );
	sv_hostname = Cvar_Get( "sv_hostname", "CoDHost", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxclients = Cvar_Get( "sv_maxclients", "20", CVAR_LATCH | CVAR_SERVERINFO );

	sv_maxRate = Cvar_Get( "sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_minPing = Cvar_Get( "sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxPing = Cvar_Get( "sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_floodProtect = Cvar_Get( "sv_floodProtect", "1", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_allowAnonymous = Cvar_Get( "sv_allowAnonymous", "0", CVAR_SERVERINFO );
	sv_showCommands = Cvar_Get( "sv_showCommands", "0", 0 );

	Cvar_Get( "sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO );
	sv_serverid = Cvar_Get( "sv_serverid", "0", CVAR_ROM | CVAR_SYSTEMINFO );
	sv_pure = Cvar_Get( "sv_pure", "1", CVAR_SERVERINFO | CVAR_SYSTEMINFO );
	Cvar_Get( "sv_paks", "", CVAR_ROM | CVAR_SYSTEMINFO );
	Cvar_Get( "sv_pakNames", "", CVAR_ROM | CVAR_SYSTEMINFO );
	Cvar_Get( "sv_referencedPaks", "", CVAR_ROM | CVAR_SYSTEMINFO );
	Cvar_Get( "sv_referencedPakNames", "", CVAR_ROM | CVAR_SYSTEMINFO );

	sv_rconPassword = Cvar_Get( "rconPassword", "", CVAR_TEMP );
	sv_privatePassword = Cvar_Get( "sv_privatePassword", "", CVAR_TEMP );
	sv_fps = Cvar_Get( "sv_fps", "20", CVAR_TEMP );
	sv_timeout = Cvar_Get( "sv_timeout", "240", CVAR_TEMP );
	sv_zombietime = Cvar_Get( "sv_zombietime", "2", CVAR_TEMP );
	Cvar_Get( "nextmap", "", CVAR_TEMP );

	sv_allowDownload = Cvar_Get( "sv_allowDownload", "1", CVAR_ARCHIVE );

	sv_master[0] = Cvar_Get( "sv_master1", "codmaster.activision.com", 0 );
	sv_master[1] = Cvar_Get( "sv_master2", "", CVAR_ARCHIVE );
	sv_master[2] = Cvar_Get( "sv_master3", "", CVAR_ARCHIVE );
	sv_master[3] = Cvar_Get( "sv_master4", "", CVAR_ARCHIVE );
	sv_master[4] = Cvar_Get( "sv_master5", "", CVAR_ARCHIVE );

	sv_reconnectlimit = Cvar_Get( "sv_reconnectlimit", "3", 0 );
	sv_showloss = Cvar_Get( "sv_showloss", "0", 0 );
	sv_padPackets = Cvar_Get( "sv_padPackets", "0", 0 );
	sv_killserver = Cvar_Get( "sv_killserver", "0", 0 );
	sv_onlyVisibleClients = Cvar_Get( "sv_onlyVisibleClients", "0", 0 );
	sv_showAverageBPS = Cvar_Get( "sv_showAverageBPS", "0", 0 );

	Cvar_Get( "g_complaintlimit", "3", CVAR_ARCHIVE );
	sv_mapRotation = Cvar_Get( "sv_mapRotation", "", 0 );
	sv_mapRotationCurrent = Cvar_Get( "sv_mapRotationCurrent", "", 0 );
}

/* ---- SV_FinalMessage  0x004594D0 ---- */
void SV_FinalMessage( const char *message ) {
	int i, j;
	client_t    *cl;

	for ( j = 0 ; j < 2 ; j++ ) {
		for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
			if ( cl->state < CS_CONNECTED ) {
				continue;
			}

			if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
				/* Retail 0x0045950E: "e" goes unreliable (ebx = 0), only "w" is reliable. */
				SV_SendServerCommand( cl, qfalse, "e \"%s\"", message );
				SV_SendServerCommand( cl, qtrue, "w" );
			}

			cl->nextSnapshotTime = -1;
			SV_SendClientSnapshot( cl );
		}
	}
}

/* ---- SV_FreeClients  0x00459570 ---- */
void SV_FreeClients( void ) {
	int i;
	client_t    *cl;

	for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
		if ( cl->state >= CS_CONNECTED ) {
			SV_FreeClientScriptId( cl );
		}
	}

	free( svs.clients );
}

/* ---- SV_Shutdown  0x00459600 ---- */
void SV_Shutdown( const char *finalmsg ) {
	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown -----\n" );

	if ( svs.clients && !com_errorEntered ) {
		SV_FinalMessage( finalmsg );
	}

	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat( "flatline" );

	SV_ShutdownGameProgs();
	SV_ClearServer();

	if ( svs.clients ) {
		SV_FreeClients();
	}
	SV_FreeArchivedSnapshot();

	memset( &svs, 0, sizeof( svs ) );

	Cvar_Set2( "sv_running", "0", qtrue );
	Com_Printf( "---------------------------\n" );

	CL_Disconnect( qfalse );
}
