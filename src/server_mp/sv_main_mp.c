/*
 * @fidelity: likely
 */

#include "server.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

serverStatic_t svs;             /* 0x016B2AA0, persistent server info */
server_t sv;                    /* 0x016515A0, local server */

cvar_t   *sv_gametype;          /* 0x016BDB78 */
cvar_t   *sv_mapname;           /* 0x016BDB7C */
cvar_t   *sv_hostname;          /* 0x016BDB5C */
cvar_t   *sv_maxclients;        /* 0x016BDB58 */
cvar_t   *sv_maxRate;           /* 0x016BDB80 */
cvar_t   *sv_allowAnonymous;    /* 0x016BDB64 */
cvar_t   *sv_serverid;          /* 0x016BDBAC */
cvar_t   *sv_pure;              /* 0x016B2A8C */
cvar_t   *sv_fps;               /* 0x016BDBA8 */
cvar_t   *sv_timeout;           /* 0x016BDBA0 */
cvar_t   *sv_killserver;        /* 0x016BDB74 */
cvar_t   *sv_privateClients;    /* 0x016BDB54 */
cvar_t   *sv_minPing;           /* 0x016BDB8C */
cvar_t   *sv_maxPing;           /* 0x01651584 */
cvar_t   *sv_floodProtect;      /* 0x016BDB6C */
cvar_t   *sv_showCommands;      /* 0x016BDB84 */
cvar_t   *sv_rconPassword;      /* 0x016B2A90 */
cvar_t   *sv_privatePassword;   /* 0x016BDB94 */
cvar_t   *sv_zombietime;        /* 0x016BDB90 */
cvar_t   *sv_allowDownload;     /* 0x016BDB70 */
cvar_t   *sv_reconnectlimit;    /* 0x016BDB68 */
cvar_t   *sv_showloss;          /* 0x016BDB60 */
cvar_t   *sv_padPackets;        /* 0x016BDBA4 */
cvar_t   *sv_onlyVisibleClients; /* 0x016BDB98 */
cvar_t   *sv_showAverageBPS;    /* 0x016BDB9C */
cvar_t   *sv_mapRotation;       /* 0x016BDBB0 */
cvar_t   *sv_mapRotationCurrent; /* 0x016BDB88 */

int      sv_rconLastTime;       /* 0x014073D4, SVC_RemoteCommand throttle */

/* The rcon output sink is 0x3FF0 bytes on SVC_RemoteCommand's stack (0x0045A711 `mov dword_8AAB58, 3FF0h`), i.e. MAX_MSGLEN - 16. */
#define SV_OUTPUTBUF_LENGTH     ( MAX_MSGLEN - 16 )

/* 0x0045A6AF and 0x0045A5F8: two rcon packets inside 500 ms and the second is ignored outright, password or not. */
#define SV_RCON_THROTTLE_MS     500

/* ---- SVC_Status  0x00459EE0 ---- */
void SVC_Status( netadr_t from ) {
	char player[1024];
	char status[MAX_MSGLEN];
	int i;
	client_t    *cl;
	int statusLength;
	int playerLength;
	char infostring[MAX_INFO_STRING];
	char keywords[MAX_INFO_STRING];
	cvar_t      *var;

	strcpy( infostring, Cvar_InfoString( CVAR_SERVERINFO ) );

	Info_SetValueForKey( infostring, "challenge", Cmd_Argv( 1 ) );

	var = Cvar_FindVar( "fs_restrict" );
	if ( var && var->value != 0.0f ) {
		Com_sprintf( keywords, sizeof( keywords ), "demo %s",
					 Info_ValueForKey( infostring, "sv_keywords" ) );
		Info_SetValueForKey( infostring, "sv_keywords", keywords );
	}

	status[0] = 0;
	statusLength = 0;

	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		cl = &svs.clients[i];
		if ( cl->state < CS_CONNECTED ) {
			continue;
		}
		Com_sprintf( player, sizeof( player ), "%i %i \"%s\"\n",
					 SV_GetClientScore( cl ), cl->ping, cl->name );
		playerLength = (int)strlen( player );
		/* SIGNEDNESS: retail's test is UNSIGNED -- `cmp esi, 4000h` / `jnb` at 0x0045A045, esi being playerLength + statusLength. Do not add an `(int)` cast; that makes it a signed compare. */
		if ( (unsigned int)( statusLength + playerLength ) >= sizeof( status ) ) {
			break;
		}
		strcpy( status + statusLength, player );
		statusLength += playerLength;
	}

	var = Cvar_FindVar( "g_password" );
	if ( var && var->string && var->string[0] ) {
		Info_SetValueForKey( infostring, "pswrd", "1" );
	} else {
		Info_SetValueForKey( infostring, "pswrd", "0" );
	}

	NET_OutOfBandPrint( NS_SERVER, from, "statusResponse\n%s\n%s",
						infostring, status );
}

/* ---- SVC_GameCompleteStatus  0x0045A120 ---- */
void SVC_GameCompleteStatus( netadr_t from ) {
	char player[1024];
	char status[MAX_MSGLEN];
	int i;
	client_t    *cl;
	int statusLength;
	int playerLength;
	char infostring[MAX_INFO_STRING];
	char keywords[MAX_INFO_STRING];
	cvar_t      *var;

	strcpy( infostring, Cvar_InfoString( CVAR_SERVERINFO ) );

	Info_SetValueForKey( infostring, "challenge", Cmd_Argv( 1 ) );

	var = Cvar_FindVar( "fs_restrict" );
	if ( var && var->value != 0.0f ) {
		Com_sprintf( keywords, sizeof( keywords ), "demo %s",
					 Info_ValueForKey( infostring, "sv_keywords" ) );
		Info_SetValueForKey( infostring, "sv_keywords", keywords );
	}

	status[0] = 0;
	statusLength = 0;

	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		cl = &svs.clients[i];
		if ( cl->state < CS_CONNECTED ) {
			continue;
		}
		Com_sprintf( player, sizeof( player ), "%i %i \"%s\"\n",
					 SV_GetClientScore( cl ), cl->ping, cl->name );
		playerLength = (int)strlen( player );
		/* SIGNEDNESS: unsigned in retail -- `cmp esi, 4000h` / `jnb` at 0x0045A285, the twin of SVC_Status' test above. */
		if ( (unsigned int)( statusLength + playerLength ) >= sizeof( status ) ) {
			break;
		}
		strcpy( status + statusLength, player );
		statusLength += playerLength;
	}

	NET_OutOfBandPrint( NS_SERVER, from, "gameCompleteStatus\n%s\n%s",
						infostring, status );
}

/* ---- SVC_Info  0x0045A320 ---- */
void SVC_Info( netadr_t from ) {
	int i;
	int count;
	char infostring[MAX_INFO_STRING];
	cvar_t      *var;

	count = 0;
	for ( i = sv_privateClients->integer ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			count++;
		}
	}

	infostring[0] = 0;

	Info_SetValueForKey( infostring, "challenge", Cmd_Argv( 1 ) );

	Info_SetValueForKey( infostring, "protocol", va( "%i", PROTOCOL_VERSION ) );
	Info_SetValueForKey( infostring, "hostname", sv_hostname->string );
	Info_SetValueForKey( infostring, "mapname", sv_mapname->string );
	Info_SetValueForKey( infostring, "clients", va( "%i", count ) );
	Info_SetValueForKey( infostring, "sv_maxclients",
						 va( "%i", sv_maxclients->integer
									- sv_privateClients->integer ) );
	Info_SetValueForKey( infostring, "gametype", sv_gametype->string );
	Info_SetValueForKey( infostring, "pure", va( "%i", sv_pure->integer ) );

	if ( sv_minPing->integer ) {
		Info_SetValueForKey( infostring, "minPing", va( "%i", sv_minPing->integer ) );
	}
	if ( sv_maxPing->integer ) {
		Info_SetValueForKey( infostring, "maxPing", va( "%i", sv_maxPing->integer ) );
	}

	var = Cvar_FindVar( "fs_game" );
	if ( var && var->string[0] ) {
		Info_SetValueForKey( infostring, "game", var->string );
	}

	Info_SetValueForKey( infostring, "sv_allowAnonymous",
						 va( "%i", sv_allowAnonymous->integer ) );

	var = Cvar_FindVar( "g_password" );
	if ( var && var->string && var->string[0] ) {
		Info_SetValueForKey( infostring, "pswrd", "1" );
	} else {
		Info_SetValueForKey( infostring, "pswrd", "0" );
	}

	NET_OutOfBandPrint( NS_SERVER, from, "infoResponse\n%s", infostring );
}

/* ---- SVC_RemoteCommand  0x0045A5D0 ---- */
void SVC_RemoteCommand( netadr_t from, msg_t *msg ) {
	qboolean valid;
	int i;
	int len;
	int time;
	char remaining[1024];
	/* the size of the redirect buffer really is MAX_MSGLEN - 16, not MAX_MSGLEN: 0x0045A711 stores 0x3FF0. */
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

	(void)msg;

	time = Com_Milliseconds();
	if ( sv_rconLastTime && time - sv_rconLastTime < SV_RCON_THROTTLE_MS ) {
		return;
	}
	sv_rconLastTime = time;

	if ( !strlen( sv_rconPassword->string )
		 || strcmp( Cmd_Argv( 1 ), sv_rconPassword->string ) ) {
		valid = qfalse;
		Com_Printf( "Bad rcon from %s:\n%s\n", NET_AdrToString( from ),
					Cmd_Argv( 2 ) );
	} else {
		valid = qtrue;
		Com_Printf( "Rcon from %s:\n%s\n", NET_AdrToString( from ),
					Cmd_Argv( 2 ) );
	}

	svs.redirectAddress = from;
	Com_BeginRedirect( sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect );

	if ( !strlen( sv_rconPassword->string ) ) {
		Com_Printf( "No rconpassword set on the server.\n" );
	} else if ( !valid ) {
		Com_Printf( "Bad rconpassword.\n" );
	} else {
		len = 0;
		for ( i = 2 ; i < Cmd_Argc() ; i++ ) {
			len = Com_AddToString( Cmd_Argv( i ), remaining, len,
								   sizeof( remaining ), 1 );
			len = Com_AddToString( " ", remaining, len,
								   sizeof( remaining ), 0 );
		}
		if ( len < (int)sizeof( remaining ) ) {
			/* retail 0x0045A7CD stores with no bound check; a pointer store keeps modern /GS from adding a range-check fast-fail */
			*( remaining + len ) = 0;
			Cmd_ExecuteString( remaining );
		}
	}

	Com_EndRedirect();
}

unsigned short  Scr_AllocArray( void );         /* script/scr_variable.cpp 0x00471580 */
void            RemoveRefToObject( unsigned short id );  /* 0x004716B0 */

/* ---- SV_AllocClientScriptPers  no-address ---- */
unsigned short SV_AllocClientScriptPers( void ) {
	return Scr_AllocArray();
}

/* ---- SV_FreeClientScriptId  0x0045AE20 ---- */
/* No zero test: retail 0x0045AE20 and every inlined copy (SV_DropClient
 * 0x00453F0D, SV_FreeClients 0x0045958C, ...) run RemoveRefToObject on
 * scriptId unconditionally. */
void SV_FreeClientScriptId( client_t *client ) {
	RemoveRefToObject( client->scriptId );
	client->scriptId = 0;
}

netadr_t sv_masterAdr[MAX_MASTER_SERVERS];
static netadr_t sv_masterGcsAdr[MAX_MASTER_SERVERS];

/* ---- SV_ExpandNewlines  0x004596C0 ---- */
char *SV_ExpandNewlines( const char *string ) {
	static char buf[1024];
	int l;

	l = 0;
	while ( *string && l < sizeof( buf ) - 3 ) {
		if ( *string == '\n' ) {
			buf[l++] = '\\';
			buf[l++] = 'n';
		} else if ( *string != 20 && *string != 21 ) {
			buf[l++] = *string;
		}
		string++;
	}
	/* retail 0x004596FE terminates with no bound check; a pointer store keeps modern /GS from adding a range-check fast-fail */
	*( buf + l ) = 0;

	return buf;
}

/* ---- SV_IsFirstTokenEqual  0x00459710 ---- */
qboolean SV_IsFirstTokenEqual( const char *a, const char *b ) {
	while ( *a && *b && *a != ' ' && *b != ' ' ) {
		if ( *a != *b ) {
			return qfalse;
		}
		a++;
		b++;
	}

	if ( *a && *a != ' ' ) {
		return qfalse;
	}

	return ( !*b || *b == ' ' ) ? qtrue : qfalse;
}

/* ---- SV_CanReplaceServerCommand  0x00459760 ---- */
int SV_CanReplaceServerCommand( client_t *client, const char *cmd ) {
	int i;
	int index;

	for ( i = client->reliableSent + 1 ; i <= client->reliableSequence ; i++ ) {
		index = i & ( MAX_RELIABLE_COMMANDS - 1 );
		if ( !client->reliableCommands[index].cmdType ) {
			continue;
		}
		if ( cmd[0] != client->reliableCommands[index].command[0] ) {
			continue;
		}
		if ( cmd[0] >= 'x' && cmd[0] <= 'z' ) {
			continue;
		}
		if ( !strcmp( cmd + 1, client->reliableCommands[index].command + 1 ) ) {
			return i;
		}

		switch ( cmd[0] ) {
		case 'a':
		case 'b':
		case 'o':
		case 'p':
		case 'q':
		case 'r':
		case 't':
			return i;
		case 'd':
		case 'v':
			if ( SV_IsFirstTokenEqual( cmd + 2, client->reliableCommands[index].command + 2 ) ) {
				return i;
			}
			break;
		}
	}

	return -1;
}

/* ---- SV_CullIgnorableServerCommands  0x00459870 ---- */
void SV_CullIgnorableServerCommands( client_t *client ) {
	int i;
	int keep;
	int from;
	int to;

	keep = client->reliableSent + 1;

	for ( i = keep ; i <= client->reliableSequence ; i++ ) {
		from = i & ( MAX_RELIABLE_COMMANDS - 1 );
		if ( !client->reliableCommands[from].cmdType ) {
			continue;
		}
		to = keep & ( MAX_RELIABLE_COMMANDS - 1 );
		if ( to != from ) {
			client->reliableCommands[to] = client->reliableCommands[from];
		}
		keep++;
	}

	client->reliableSequence = keep - 1;
}

/* ---- SV_AddServerCommand  0x004598E0 ---- */
void SV_AddServerCommand( client_t *client, int isReliable, const char *cmd ) {
	int i, j;
	int replace;

	if ( client->bIsTestClient ) {
		return;
	}

	if ( !( client->reliableSequence - client->reliableAcknowledge < 32
			&& client->state == CS_ACTIVE ) ) {
		SV_CullIgnorableServerCommands( client );
		if ( !isReliable ) {
			return;
		}
	}

	replace = SV_CanReplaceServerCommand( client, cmd );
	if ( replace >= 0 ) {
		for ( i = replace + 1 ; i <= client->reliableSequence ; i++, replace++ ) {
			client->reliableCommands[replace & ( MAX_RELIABLE_COMMANDS - 1 )]
				= client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS - 1 )];
		}
	} else {
		client->reliableSequence++;
	}

	if ( client->reliableSequence - client->reliableAcknowledge == 65 ) {
		Com_Printf( "===== pending server commands =====\n" );
		for ( j = client->reliableAcknowledge + 1 ; j <= client->reliableSequence ; j++ ) {
			Com_Printf( "cmd %5d: %8d: %s\n", j,
						client->reliableCommands[j & ( MAX_RELIABLE_COMMANDS - 1 )].cmdTime,
						client->reliableCommands[j & ( MAX_RELIABLE_COMMANDS - 1 )].command );
		}
		Com_Printf( "cmd %5d: %8d: %s\n", j, svs.time, cmd );

		NET_OutOfBandPrint( NS_SERVER, client->netchan.remoteAddress, "disconnect" );
		if ( client->state != CS_ZOMBIE && !client->dropReason ) {
			client->dropReason = "EXE_SERVERCOMMANDOVERFLOW";
		}
		isReliable = 1;
		cmd = "w \"EXE_SERVERCOMMANDOVERFLOW\"";
	}

	replace = client->reliableSequence & ( MAX_RELIABLE_COMMANDS - 1 );
	MSG_WriteReliableCommandToBuffer( cmd, sizeof( client->reliableCommands[replace].command ),
									  client->reliableCommands[replace].command );
	client->reliableCommands[replace].cmdTime = svs.time;
	client->reliableCommands[replace].cmdType = isReliable;
}

/* ---- SV_SendServerCommand  0x00459A90 ---- */
void QDECL SV_SendServerCommand( client_t *client, int isReliable, const char *fmt, ... ) {
	va_list argptr;
	client_t    *cl;
	int i;
	char message[MAX_MSGLEN];

	va_start( argptr, fmt );
	vsprintf( message, fmt, argptr );
	va_end( argptr );

	if ( client ) {
		SV_AddServerCommand( client, isReliable, message );
		return;
	}

	if ( com_dedicated->integer && !strncmp( message, "print", 5 ) ) {
		Com_Printf( "broadcast: %s\n", SV_ExpandNewlines( message ) );
	}

	for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
		if ( cl->state < CS_PRIMED ) {
			continue;
		}
		SV_AddServerCommand( cl, isReliable, message );
	}
}

/* ---- SV_ResolveMaster  no-address ---- */
static qboolean SV_ResolveMaster( cvar_t *master, netadr_t *adr ) {
	if ( !master->string[0] ) {
		return qfalse;
	}

	if ( !master->modified ) {
		return qtrue;
	}
	master->modified = qfalse;

	Com_Printf( "Resolving %s\n", master->string );
	if ( !NET_StringToAdr( master->string, adr ) ) {
		Com_Printf( "Couldn't resolve address: %s\n", master->string );
		Cvar_Set2( master->name, "", qtrue );
		master->modified = qfalse;
		return qfalse;
	}

	if ( !strstr( ":", master->string ) ) {
		adr->port = BigShort( PORT_MASTER );
	}

	Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", master->string,
				adr->ip[0], adr->ip[1], adr->ip[2], adr->ip[3],
				BigShort( adr->port ) );
	return qtrue;
}

/* ---- SV_MasterHeartbeat  0x00459BA0 ---- */
void SV_MasterHeartbeat( const char *hbname ) {
	int i;

	if ( !com_dedicated || com_dedicated->integer != 2 ) {
		return;
	}
	if ( svs.time < svs.nextHeartbeatTime ) {
		return;
	}
	svs.nextHeartbeatTime = svs.time + 180000;

	for ( i = 0 ; i < MAX_MASTER_SERVERS ; i++ ) {
		if ( !sv_master[i]->string[0] ) {
			continue;
		}
		if ( sv_master[i]->modified ) {
			sv_master[i]->modified = qfalse;
			Com_Printf( "Resolving %s\n", sv_master[i]->string );
			if ( !NET_StringToAdr( sv_master[i]->string, &sv_masterAdr[i] ) ) {
				Com_Printf( "Couldn't resolve address: %s\n", sv_master[i]->string );
				Cvar_Set2( sv_master[i]->name, "", qtrue );
				sv_master[i]->modified = qfalse;
				continue;
			}
			if ( !strstr( ":", sv_master[i]->string ) ) {
				sv_masterAdr[i].port = BigShort( PORT_MASTER );
			}
			Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
				sv_masterAdr[i].ip[0], sv_masterAdr[i].ip[1], sv_masterAdr[i].ip[2], sv_masterAdr[i].ip[3], BigShort( sv_masterAdr[i].port ) );
		}

		Com_Printf( "Sending heartbeat to %s\n", sv_master[i]->string );
		NET_OutOfBandPrint( NS_SERVER, sv_masterAdr[i], "heartbeat %s\n", hbname );
	}
}

/* ---- SV_MasterGameCompleteStatus  0x00459D20 ---- */
void SV_MasterGameCompleteStatus( void ) {
	int i;

	if ( !com_dedicated || com_dedicated->integer != 2 ) {
		return;
	}

	for ( i = 0 ; i < MAX_MASTER_SERVERS ; i++ ) {
		if ( !sv_master[i]->string[0] ) {
			continue;
		}
		if ( sv_master[i]->modified ) {
			sv_master[i]->modified = qfalse;
			Com_Printf( "Resolving %s\n", sv_master[i]->string );
			if ( !NET_StringToAdr( sv_master[i]->string, &sv_masterGcsAdr[i] ) ) {
				Com_Printf( "Couldn't resolve address: %s\n", sv_master[i]->string );
				Cvar_Set2( sv_master[i]->name, "", qtrue );
				sv_master[i]->modified = qfalse;
				continue;
			}
			if ( !strstr( ":", sv_master[i]->string ) ) {
				sv_masterGcsAdr[i].port = BigShort( PORT_MASTER );
			}
			Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
				sv_masterGcsAdr[i].ip[0], sv_masterGcsAdr[i].ip[1], sv_masterGcsAdr[i].ip[2], sv_masterGcsAdr[i].ip[3], BigShort( sv_masterGcsAdr[i].port ) );
		}

		Com_Printf( "Sending gameCompleteStatus to %s\n", sv_master[i]->string );
		SVC_GameCompleteStatus( sv_masterGcsAdr[i] );
	}
}

/* ---- SV_MasterShutdown  0x00459E90 ---- */
void SV_MasterShutdown( void ) {
	svs.nextHeartbeatTime = -9999;
	SV_MasterHeartbeat( "flatline" );
}

/* ---- SV_GetClientScore  0x00459EA0 ---- */
int SV_GetClientScore( client_t *client ) {
	if ( vm ) {
		return VM_Call( vm, GAME_GET_CLIENT_SCORE, (int)( client - svs.clients ) );
	}
	return 0;
}

/* ---- SV_FlushRedirect  0x0045A5A0 ---- */
void SV_FlushRedirect( char *outputbuf ) {
	NET_OutOfBandPrint( NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf );
}

/* ---- SV_ConnectionlessPacket  0x0045A820 ---- */
void SV_ConnectionlessPacket( netadr_t from, msg_t *msg ) {
	char    *s;
	char    *c;
	int     size;

	/* retail reads cursize once, before MSG_BeginReading, and passes that value to NetProf_AddPacket */
	size = msg->cursize;
	MSG_BeginReading( msg );
	MSG_ReadLong( msg );

	if ( net_profile->integer ) {
		NetProf_PrepProfiling( &svs.pOOBProf );
		/* An incoming packet is added to the OOB profile's SEND stream. Retail bug; preserved. */
		NetProf_AddPacket( &( (netProfileInfo_t *)svs.pOOBProf )->send, size, 0 );
	}

	if ( !Q_strncmp( "connect", (const char *)msg->data + 4, 7 ) ) {
		Huff_Decompress( msg, 12 );
	}

	s = MSG_ReadStringLine( msg );
	Cmd_TokenizeString2( s, 0 );
	c = Cmd_Argv( 0 );

	Com_DPrintf( "SV packet %s : %s\n", NET_AdrToString( from ), c );

	if ( !Q_stricmp( c, "getstatus" ) ) {
		SVC_Status( from );
	} else if ( !Q_stricmp( c, "getinfo" ) ) {
		SVC_Info( from );
	} else if ( !Q_stricmp( c, "getchallenge" ) ) {
		SV_GetChallenge( from );
	} else if ( !Q_stricmp( c, "connect" ) ) {
		SV_DirectConnect( from );
	} else if ( !Q_stricmp( c, "ipAuthorize" ) ) {
		SV_AuthorizeIpPacket( from );
	} else if ( !Q_stricmp( c, "rcon" ) ) {
		SVC_RemoteCommand( from, msg );
	} else if ( Q_stricmp( c, "disconnect" ) ) {
		Com_DPrintf( "bad connectionless packet from %s:\n%s\n",
					 NET_AdrToString( from ), s );
	}
}

/* ---- SV_PacketEvent  0x0045AA70 ---- */
void SV_PacketEvent( netadr_t from, msg_t *msg ) {
	int i;
	int qport;
	client_t    *cl;

	if ( msg->cursize >= 4 && *(int *)msg->data == -1 ) {
		SV_ConnectionlessPacket( from, msg );
		return;
	}

	if ( !++com_skelTimeStamp ) {
		com_skelTimeStamp = 1;
	}
	com_skelInvalidated = 1;

	msg->readcount = 0;
	msg->bit = 0;
	if ( msg->cursize >= 4 ) {
		msg->readcount = 4;
	}
	qport = MSG_ReadShort( msg ) & 0xffff;

	for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( !NET_CompareBaseAdr( from, cl->netchan.remoteAddress ) ) {
			continue;
		}
		if ( cl->netchan.qport != qport ) {
			continue;
		}

		if ( cl->netchan.remoteAddress.port != from.port ) {
			Com_Printf( "SV_ReadPackets: fixing up a translated port\n" );
			cl->netchan.remoteAddress.port = from.port;
		}

		if ( Netchan_Process( &cl->netchan, msg ) ) {
			cl->serverId = MSG_ReadByte( msg );
			cl->messageAcknowledge = MSG_ReadLong( msg );
			if ( cl->messageAcknowledge >= 0 ) {
				cl->reliableAcknowledge = MSG_ReadLong( msg );

				if ( cl->reliableSequence - cl->reliableAcknowledge
					 >= MAX_RELIABLE_COMMANDS ) {
					cl->reliableAcknowledge = cl->reliableSequence;
				} else {
					SV_Netchan_Decode( cl, msg->data + msg->readcount,
									   msg->cursize - msg->readcount );
					if ( cl->state != CS_ZOMBIE ) {
						cl->lastPacketTime = svs.time;
						SV_ExecuteClientMessage( cl, msg );
					}
				}
			}
		}

		goto done;
	}

	NET_OutOfBandPrint( NS_SERVER, from, "disconnect" );

done:
	com_skelInvalidated = 0;
	if ( s_hunkData ) {
		SV_HUNK_LOW_TEMP = SV_HUNK_LOW_PERM;
	}
}

/* ---- SV_CalcPings  0x0045ACB0 ---- */
void SV_CalcPings( void ) {
	int i, j;
	int delta;
	int total, count;
	client_t    *cl;

	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		cl = &svs.clients[i];

		if ( cl->state != CS_ACTIVE ) {
			cl->ping = 999;
			continue;
		}
		if ( !cl->gentity ) {
			cl->ping = 999;
			continue;
		}

		total = 0;
		count = 0;
		for ( j = 0 ; j < PACKET_BACKUP ; j++ ) {
			if ( (int)cl->frames[j].messageAcked <= 0 ) {
				continue;
			}
			delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
			count++;
			total += delta;
		}

		if ( !count ) {
			cl->ping = 999;
		} else {
			cl->ping = total / count;
			if ( cl->ping > 999 ) {
				cl->ping = 999;
			}
		}

		( (playerState_t *)( (byte *)sv.gameClients + i * sv.gameClientSize ) )->ping
			= cl->ping;
	}
}

/* ---- SV_CheckTimeouts  0x0045AE80 ---- */
void SV_CheckTimeouts( void ) {
	int i;
	int droppoint;
	int zombiepoint;
	client_t    *cl;

	droppoint = svs.time - 1000 * sv_timeout->integer;
	zombiepoint = svs.time - 1000 * sv_zombietime->integer;

	for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
		if ( cl->lastPacketTime > svs.time ) {
			cl->lastPacketTime = svs.time;
		}

		if ( cl->bIsTestClient ) {
			continue;
		}

		if ( cl->state == CS_ZOMBIE && cl->lastPacketTime < zombiepoint ) {
			Com_DPrintf( "Going from CS_ZOMBIE to CS_FREE for %s\n", cl->name );
			cl->state = CS_FREE;
			continue;
		}
		if ( cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint ) {
			if ( ++cl->timeoutCount > 5 ) {
				SV_DropClient( cl, "EXE_TIMEDOUT" );
				cl->state = CS_FREE;
			}
		} else {
			cl->timeoutCount = 0;
		}
	}
}

/* ---- SV_CheckPaused  0x0045AF70 ---- */
qboolean SV_CheckPaused( void ) {
	int i;
	int count;
	client_t    *cl;

	if ( !cl_paused->integer ) {
		return qfalse;
	}

	count = 0;
	for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
		if ( cl->state >= CS_CONNECTED ) {
			count++;
		}
	}

	if ( count > 1 ) {
		sv_paused->integer = 0;
		return qfalse;
	}

	sv_paused->integer = 1;
	return qtrue;
}

/* ---- SV_RunFrame  0x0045AFD0 ---- */
void SV_RunFrame( void ) {
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

playerState_t   *SV_GameClientNum( int clientNum );   /* sv_game_mp.c 0x00455890 */

/* ---- SV_BotUserMove  0x0045B030 ---- */
void SV_BotUserMove( client_t *client ) {
	usercmd_t cmd;

	if ( !client->gentity ) {
		return;
	}

	memset( &cmd, 0, sizeof( cmd ) );

	cmd.weapon = (byte)SV_GameClientNum( (int)( client - svs.clients ) )->weapon;

	if ( !VM_Call( vm, GAME_GET_ARCHIVE_TIME, (int)( client - svs.clients ) ) ) {
		if ( rand() * ( 1.0f / 32768.0f ) < 0.5f ) {
			cmd.buttons |= 1;
		}
		if ( rand() * ( 1.0f / 32768.0f ) < 0.5f ) {
			cmd.buttons |= 0x40;
		}
		if ( rand() * ( 1.0f / 32768.0f ) < 0.33f ) {
			cmd.forwardmove = 127;
		} else if ( rand() * ( 1.0f / 32768.0f ) < 0.5f ) {
			cmd.forwardmove = -127;
		}
	}

	client->deltaMessage = client->netchan.outgoingSequence - 1;
	SV_ClientThink( client, &cmd );
}

/* ---- SV_UpdateBots  0x0045B150 ---- */
void SV_UpdateBots( void ) {
	int i;
	client_t    *cl;

	if ( !++com_skelTimeStamp ) {
		com_skelTimeStamp = 1;
	}
	com_skelInvalidated = 1;

	for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( cl->netchan.remoteAddress.type == NA_BOT ) {
			SV_BotUserMove( cl );
		}
	}

	com_skelInvalidated = 0;
	if ( s_hunkData ) {
		SV_HUNK_LOW_TEMP = SV_HUNK_LOW_PERM;
	}
}

/* ---- SV_RestartAfterCounterWrap  no-address ---- */
static __forceinline void SV_RestartAfterCounterWrap( const char *message ) {
	char mapname[MAX_QPATH];

	strncpy( mapname, sv_mapname->string, MAX_QPATH - 1 );
	mapname[MAX_QPATH - 1] = 0;
	Com_Shutdown( message );
	Cbuf_AddText( va( "map %s\n", mapname ) );
}

/* ---- SV_Frame  0x0045B1D0 ---- */
void SV_Frame( int msec ) {
	int frameMsec;
	int startTime;
	qboolean more;

	if ( sv_killserver->integer ) {
		Com_Shutdown( "EXE_SERVERKILLED" );
		Cvar_Set2( "sv_killserver", "0", qtrue );
		return;
	}

	if ( !com_sv_running->integer ) {
		return;
	}
	if ( SV_CheckPaused() ) {
		return;
	}

	if ( sv_fps->integer < 1 ) {
		Cvar_Set2( "sv_fps", "10", qtrue );
	}
	frameMsec = 1000 / sv_fps->integer;

	sv.timeResidual += msec;
	if ( sv.timeResidual < frameMsec ) {
		return;
	}

	if ( svs.time > SV_TIME_WRAP_LIMIT ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTTIMEWRAP" );
		return;
	}
	if ( svs.nextSnapshotEntities >= SV_SNAPSHOT_COUNTER_LIMIT - svs.numSnapshotEntities ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTMISC\x15numSnapshotEntities" );
		return;
	}
	if ( svs.nextCachedSnapshotEntities >= SV_CACHED_SNAP_ENTITIES_LIMIT ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTMISC\x15nextCachedSnapshotEntities" );
		return;
	}
	if ( svs.nextCachedSnapshotClients >= SV_CACHED_SNAP_CLIENTS_LIMIT ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTMISC\x15nextCachedSnapshotClients" );
		return;
	}
	if ( svs.nextArchivedSnapshotFrames >= SV_ARCHIVED_SNAP_FRAMES_LIMIT ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTMISC\x15nextArchivedSnapshotFrames" );
		return;
	}
	if ( svs.nextArchivedSnapshotBuffer >= SV_ARCHIVED_SNAP_BUFFER_LIMIT ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTMISC\x15nextArchivedSnapshotBuffer" );
		return;
	}
	if ( svs.nextCachedSnapshotFrames >= SV_CACHED_SNAP_FRAMES_LIMIT ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTMISC\x15nextCachedSnapshotFrames" );
		return;
	}
	if ( svs.nextSnapshotClients >= SV_SNAPSHOT_COUNTER_LIMIT - svs.numSnapshotClients ) {
		SV_RestartAfterCounterWrap( "EXE_SERVERRESTARTMISC\x15numSnapshotClients" );
		return;
	}

	xanim_activePoolSlot = 1;

	if ( cvar_modifiedFlags & CVAR_SERVERINFO ) {
		SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if ( cvar_modifiedFlags & CVAR_SYSTEMINFO ) {
		SV_SetConfigstring( CS_SYSTEMINFO, Cvar_InfoString_Big( CVAR_SYSTEMINFO ) );
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}
	if ( cvar_modifiedFlags & CVAR_WOLFINFO ) {
		SV_SetConfig( CS_SCRIPT_KV_BASE, CS_SCRIPT_KV_COUNT, CVAR_WOLFINFO );
		cvar_modifiedFlags &= ~CVAR_WOLFINFO;
	}

	SV_UpdateBots();

	startTime = com_speeds->integer ? Sys_Milliseconds() : 0;

	SV_CalcPings();

	do {
		sv.timeResidual -= frameMsec;
		svs.time += frameMsec;

		CL_FlushDebugData( 1 );
		SV_RunFrame();
		/* retail stores 0 to scrVmGlob.loading (0x00A7A5E0) inline; script/scr_vm_state.c owns that word and exposes this setter. */
		Scr_SetLoading( qfalse );

		/* retail (Mac 1.3: while(1) {...; if (residual < frameMsec) break; SV_ArchiveSnapshot();})
		   jumps straight back to the loop top after the archive; a plain mid-exit loop gets rotated
		   by VC7, a constant flag threads into retail's shape */
		more = qfalse;
		if ( sv.timeResidual >= frameMsec ) {
			SV_ArchiveSnapshot();
			more = qtrue;
		}
	} while ( more );

	if ( com_speeds->integer ) {
		time_frontend = Sys_Milliseconds() - startTime;
	}

	SV_CheckTimeouts();
	SV_SendClientMessages();
	SV_ArchiveSnapshot();

	SV_MasterHeartbeat( "COD-1" );
}
