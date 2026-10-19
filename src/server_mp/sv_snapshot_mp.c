/*
 * @fidelity: verified
 */

#include "server.h"

#include <string.h>

void MSG_WriteByte( msg_t *msg, int c );
void MSG_WriteLong( msg_t *msg, int c );
void MSG_WriteBits( msg_t *msg, int value, int bits );
void MSG_WriteBit0( msg_t *msg );
void MSG_WriteBit1( msg_t *msg );
void MSG_WriteString( msg_t *sb, const char *s );
int MSG_WriteBitsCompress( const byte *datasrc, int bytecount, byte *buffdest );
void MSG_WriteDeltaEntity( msg_t *msg, const byte *from, const byte *to, qboolean force );
void MSG_WriteDeltaClient( msg_t *msg, const byte *from, const byte *to, qboolean force );
void MSG_WriteDeltaPlayerstate( msg_t *msg, const byte *from, const byte *to );

void MSG_WriteDeltaArchivedEntity( msg_t *msg, const byte *from, const byte *to,
								   qboolean force );

void MSG_WriteData( msg_t *msg, int length, const void *data );

int MSG_ReadBit( msg_t *msg );
qboolean MSG_ReadDeltaClient( msg_t *msg, const byte *from, byte *to, int number );
qboolean MSG_ReadDeltaArchivedEntity( msg_t *msg, const byte *from, byte *to,
									  int number );
void MSG_ReadDeltaPlayerstate( msg_t *msg, const byte *from, byte *to );
void MSG_BeginReading( msg_t *msg );

/* qcommon/cm_test.c 0x00421E10. mins@ecx, maxs@eax in retail. */
int CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *list,
					int listsize, int *lastLeaf );

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
void Netchan_TransmitNextFragment( netchan_t *chan );

void SV_Netchan_Encode( client_t *client, byte *data, int length );

void SV_CloseDownload( client_t *cl );
void SV_WriteDownloadToClient( client_t *cl, msg_t *msg );

int CM_PointLeafnum( const vec3_t p );
int CM_LeafCluster( int leafnum );
int CM_LeafArea( int leafnum );
byte *CM_ClusterPVS( int cluster );
qboolean CM_AreasConnected( int area1, int area2 );

/* 0x01407444, client_mp/cl_main_mp.c (null/null_client.c in the dedicated build) */
extern cvar_t   *cl_shownet;

void SV_AddEntToSnapshot( int *eNums, int gEntNum );
void SV_AddArchivedEntToSnapshot( int *eNums, int gEntNum );
/* server_mp/sv_game_mp.c 0x004558B0. Com_Errors "bad gEnt" on NULL or s.number outside [0,1024). */
svEntity_t *SV_SvEntityForGentity( gentity_t *gent );
cachedSnapshot_t *SV_GetCachedSnapshotInternal( int frameNum );
cachedSnapshot_t *SV_GetCachedSnapshot( int *frameTime );
void *SV_GetClientState( int clientNum );
#ifndef COD1_VC7
static void SV_UpdateAverageBPS( int numclients );
#endif
void SV_Netchan_TransmitNextFragment( netchan_t *chan );

/*
 * SV_ARCHIVE_SELFTEST (off by default) builds a tree-only archive diagnostic
 * that retail does not have: the sv_archiveSelfTest / sv_archiveForceTime
 * cvars, the round-trip check SV_ArchiveSelfTest run from SV_ArchiveSnapshot,
 * the forced archive time in SV_GetClientArchiveTime and these counters.
 * Default builds register no extra cvars and match retail exactly.
 */
#ifdef SV_ARCHIVE_SELFTEST
int sv_archiveCachedBuilds;
int sv_archiveCachedClientInfos;
int sv_archiveCachedInfoChecks;
extern cvar_t   *sv_archiveForceTime;
#endif
qboolean SV_GetArchivedClientInfo( int *frameTime, void *clientInfo, int clientNum );

#define svc_nop                 1
#define svc_serverCommand       5
#define svc_snapshot            7
#define svc_EOF                 8

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2

#define MAX_SNAPSHOT_ENTITIES   1024

#define SVF_NOCLIENT            0x00000001
#define SVF_BROADCAST           0x00000008
#define SVF_PORTAL              0x00000010
#define SVF_SINGLECLIENT        0x00000800
#define SVF_NOTSINGLECLIENT     0x00002000

#define GAME_GET_CLIENT_INFO    8
#define GAME_GET_CLIENT_STATE   17
#define GAME_SET_ARCHIVE_TIME   19

#define SV_SNAPSHOT_CLIENT_BYTES 92

/* One 92-byte record of svs.snapshotClients. Retail copies these by struct assignment (bare rep movsd). */
typedef struct svSnapshotClient_s {
	int words[SV_SNAPSHOT_CLIENT_BYTES / 4];
} svSnapshotClient_t;

#define SVS_SNAPSHOT_CLIENTS    ( (svSnapshotClient_t *)svs.snapshotClients )

/* msg.maxsize the writer sets at 0x0045DEEF and the reader at 0x0045CBEB. Not MAX_MSGLEN -- eight times it. Both the archive writer and the archive reader put a buffer this size on the stack. */
#define SV_ARCHIVE_MSG_BYTES    0x20000

#define MSG_CLIENT_NUM_BITS     6

#define MAX_TOTAL_ENT_LEAFS     128

#define SV_NO_MORE_CLIENTS      99999

#define PMF_ARCHIVE_HIDE_VIEWENTITY 0x00010000

/* ---- SV_GentityNum  0x00455870 ---- */
static gentity_t *SV_GentityNum( int num ) {
	return (gentity_t *)( (byte *)sv.gentities + num * sv.gentitySize );
}

/* ---- SV_SvEntityNum  no-address ----
 * A macro: retail has no out-of-line copy of this helper (a called static
 * would leave a tree-only dead copy under /GL). */
#define SV_SvEntityNum( num )   ( &sv.svEntities[num] )

/* ---- SV_EmitPacketEntities  0x0045BF80 ---- */
void SV_EmitPacketEntities( int from_num_entities, int from_first_entity,
							int to_num_entities, int to_first_entity,
							msg_t *msg ) {
	entityState_t   *oldent, *newent;
	int oldindex, newindex;
	int oldnum, newnum;

	newent = NULL;
	oldent = NULL;
	newindex = 0;
	oldindex = 0;

	while ( 1 ) {
		if ( newindex >= to_num_entities ) {
			if ( oldindex >= from_num_entities ) {
				break;
			}
			newnum = 9999;
		} else {
			newent = &svs.snapshotEntities[( to_first_entity + newindex )
										   % svs.numSnapshotEntities];
			newnum = newent->number;
		}

		if ( oldindex >= from_num_entities ) {
			oldnum = 9999;
		} else {
			oldent = &svs.snapshotEntities[( from_first_entity + oldindex )
										   % svs.numSnapshotEntities];
			oldnum = oldent->number;
		}

		if ( newnum == oldnum ) {
			MSG_WriteDeltaEntity( msg, (const byte *)oldent,
								  (const byte *)newent, qfalse );
			oldindex++;
			newindex++;
			continue;
		}

		if ( newnum < oldnum ) {
			MSG_WriteDeltaEntity( msg,
								  (const byte *)&SV_SvEntityNum( newnum )->baseline,
								  (const byte *)newent, qtrue );
			newindex++;
			continue;
		}

		if ( newnum > oldnum ) {
			MSG_WriteDeltaEntity( msg, (const byte *)oldent, NULL, qtrue );
			oldindex++;
			continue;
		}
	}

	MSG_WriteBits( msg, ENTITYNUM_NONE, 10 );
}

/* ---- SV_EmitPacketClients  0x0045C0F0 ---- */
void SV_EmitPacketClients( int from_num_clients, int from_first_client,
						   int to_num_clients, int to_first_client,
						   msg_t *msg ) {
	const byte  *oldclient;
	const byte  *newclient;
	int oldindex, newindex;
	int oldnum, newnum;

	newclient = NULL;
	oldclient = NULL;
	newindex = 0;
	oldindex = 0;

	while ( 1 ) {
		if ( newindex >= to_num_clients ) {
			if ( oldindex >= from_num_clients ) {
				break;
			}
			newnum = 9999;
		} else {
			newclient = (const byte *)svs.snapshotClients
						+ SV_SNAPSHOT_CLIENT_BYTES
						  * ( ( to_first_client + newindex ) % svs.numSnapshotClients );
			newnum = *(const int *)newclient;
		}

		if ( oldindex >= from_num_clients ) {
			oldnum = 9999;
		} else {
			oldclient = (const byte *)svs.snapshotClients
						+ SV_SNAPSHOT_CLIENT_BYTES
						  * ( ( from_first_client + oldindex ) % svs.numSnapshotClients );
			oldnum = *(const int *)oldclient;
		}

		if ( newnum == oldnum ) {
			MSG_WriteDeltaClient( msg, oldclient, newclient, qfalse );
			oldindex++;
			newindex++;
			continue;
		}

		if ( newnum < oldnum ) {
			MSG_WriteDeltaClient( msg, NULL, newclient, qtrue );
			newindex++;
			continue;
		}

		if ( newnum > oldnum ) {
			MSG_WriteDeltaClient( msg, oldclient, NULL, qtrue );
			oldindex++;
			continue;
		}
	}

	MSG_WriteBit0( msg );
}

/* ---- SV_WriteSnapshotToClient  0x0045C2B0 ---- */
void SV_WriteSnapshotToClient( client_t *client, msg_t *msg ) {
	clientSnapshot_t    *frame;
	clientSnapshot_t    *oldframe;
	int lastframe;
	int snapFlags;
	int from_num_entities, from_first_entity;
	int from_num_clients, from_first_client;
	int i;

	frame = &client->frames[client->netchan.outgoingSequence & PACKET_MASK];

	if ( client->deltaMessage <= 0 || client->state != CS_ACTIVE ) {
		oldframe = NULL;
		lastframe = 0;
	} else if ( client->netchan.outgoingSequence - client->deltaMessage
				>= PACKET_BACKUP - 3 ) {
		Com_DPrintf( "%s: Delta request from out of date packet.\n",
					 client->name );
		oldframe = NULL;
		lastframe = 0;
	} else {
		oldframe = &client->frames[client->deltaMessage & PACKET_MASK];
		lastframe = client->netchan.outgoingSequence - client->deltaMessage;

		if ( oldframe->first_entity
			 < svs.nextSnapshotEntities - svs.numSnapshotEntities ) {
			Com_DPrintf( "%s: Delta request from out of date entities.\n",
						 client->name );
			oldframe = NULL;
			lastframe = 0;
		}
	}

	MSG_WriteByte( msg, svc_snapshot );

	MSG_WriteLong( msg, svs.time );

	MSG_WriteByte( msg, lastframe );

	snapFlags = svs.snapFlagServerBit;
	if ( client->rateDelayed ) {
		snapFlags |= SNAPFLAG_RATE_DELAYED;
	}
	if ( client->state == CS_ACTIVE ) {
		client->sendAsActive = qtrue;
	} else if ( client->state != CS_ZOMBIE ) {
		client->sendAsActive = qfalse;
	}
	if ( !client->sendAsActive ) {
		snapFlags |= SNAPFLAG_NOT_ACTIVE;
	}
	MSG_WriteByte( msg, snapFlags );

	if ( oldframe ) {
		MSG_WriteDeltaPlayerstate( msg, (const byte *)&oldframe->ps,
								   (const byte *)&frame->ps );
		from_num_entities = oldframe->num_entities;
		from_first_entity = oldframe->first_entity;
		from_num_clients = oldframe->num_clients;
		from_first_client = oldframe->first_client;
	} else {
		MSG_WriteDeltaPlayerstate( msg, NULL, (const byte *)&frame->ps );
		from_num_entities = 0;
		from_first_entity = 0;
		from_num_clients = 0;
		from_first_client = 0;
	}

	SV_EmitPacketEntities( from_num_entities, from_first_entity,
						   frame->num_entities, frame->first_entity, msg );
	SV_EmitPacketClients( from_num_clients, from_first_client,
						  frame->num_clients, frame->first_client, msg );

	if ( sv_padPackets->integer ) {
		for ( i = 0 ; i < sv_padPackets->integer ; i++ ) {
			MSG_WriteByte( msg, svc_nop );
		}
	}
}

/* ---- SV_UpdateServerCommandsToClient  0x0045C4A0 ---- */
void SV_UpdateServerCommandsToClient( client_t *client, msg_t *msg ) {
	int i;

	for ( i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++ ) {
		MSG_WriteByte( msg, svc_serverCommand );
		MSG_WriteLong( msg, i );
		MSG_WriteString( msg, client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS - 1 )].command );
	}
	client->reliableSent = client->reliableSequence;
}

/* ---- SV_UpdateServerCommandsToClient_PreventOverflow  0x0045C530 ---- */
void SV_UpdateServerCommandsToClient_PreventOverflow( msg_t *msg, client_t *client,
													  int maxSize ) {
	int i;
	const char  *cmd;

	for ( i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++ ) {
		cmd = client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS - 1 )].command;

		if ( msg->cursize + (int)strlen( cmd ) + 6 >= maxSize ) {
			break;
		}

		MSG_WriteByte( msg, svc_serverCommand );
		MSG_WriteLong( msg, i );
		MSG_WriteString( msg, cmd );
	}

	i--;
	if ( i > client->reliableSent ) {
		client->reliableSent = i;
	}
}

/* ---- SV_PrintServerCommandsForClient  0x0045C5E0 ---- */
void SV_PrintServerCommandsForClient( client_t *client ) {
	int i;

	Com_Printf( "-- Unacknowledged Server Commands for client %i:%s --\n",
				(int)( client - svs.clients ), client->name );

	for ( i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++ ) {
		Com_Printf( "cmd %5d: %8d: %s\n", i,
					client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS - 1 )].cmdTime,
					client->reliableCommands[i & ( MAX_RELIABLE_COMMANDS - 1 )].command );
	}

	Com_Printf( "----------" );
}

/* ---- SV_AddEntToSnapshot  0x0045C670 ---- */
void SV_AddEntToSnapshot( int *eNums, int gEntNum ) {
	if ( eNums[0] == MAX_SNAPSHOT_ENTITIES ) {
		return;
	}
	eNums[eNums[0] + 1] = gEntNum;
	eNums[0]++;
}

/* ---- SV_AddArchivedEntToSnapshot  0x0045C690 ---- */
void SV_AddArchivedEntToSnapshot( int *eNums, int gEntNum ) {
	if ( eNums[0] == MAX_SNAPSHOT_ENTITIES ) {
		return;
	}
	eNums[eNums[0] + 1] = gEntNum;
	eNums[0]++;
}

/* ---- SV_AddEntitiesVisibleFromPoint  0x0045C6B0 ---- */
void SV_AddEntitiesVisibleFromPoint( int leafnum, int clientNum, int *eNums ) {
	int e, i;
	gentity_t   *ent;
	svEntity_t     *svEnt;
	int svFlags;
	int area;
	int cluster;
	const byte  *bitvector;
	int l;

	area = CM_LeafArea( leafnum );
	if ( area < 0 ) {
		return;
	}
	cluster = CM_LeafCluster( leafnum );
	bitvector = CM_ClusterPVS( cluster );

	for ( e = 0 ; e < sv.num_entities ; e++ ) {
		ent = SV_GentityNum( e );

		if ( !ent->r.linked ) {
			continue;
		}

		svFlags = ent->r.svFlags;

		if ( svFlags & SVF_NOCLIENT ) {
			continue;
		}
		if ( ( svFlags & SVF_SINGLECLIENT ) && ent->r.singleClient != clientNum ) {
			continue;
		}
		if ( ( svFlags & SVF_NOTSINGLECLIENT ) && ent->r.singleClient == clientNum ) {
			continue;
		}

		if ( e == clientNum ) {
			continue;
		}

		if ( svFlags & ( SVF_BROADCAST | SVF_PORTAL ) ) {
			SV_AddEntToSnapshot( eNums, e );
			continue;
		}

		/* 0x0045C7A8: retail goes through the range-checked accessor, not a raw index. */
		svEnt = SV_SvEntityForGentity( ent );

		if ( !CM_AreasConnected( area, svEnt->areanum ) ) {
			if ( !CM_AreasConnected( area, svEnt->areanum2 ) ) {
				continue;
			}
		}

		if ( !svEnt->numClusters ) {
			continue;
		}

		l = 0;
		for ( i = 0 ; i < svEnt->numClusters ; i++ ) {
			l = svEnt->clusternums[i];
			if ( bitvector[l >> 3] & ( 1 << ( l & 7 ) ) ) {
				break;
			}
		}

		if ( i == svEnt->numClusters ) {
			if ( !svEnt->lastCluster ) {
				continue;
			}
			for ( ; l <= svEnt->lastCluster ; l++ ) {
				if ( bitvector[l >> 3] & ( 1 << ( l & 7 ) ) ) {
					break;
				}
			}
			if ( l == svEnt->lastCluster ) {
				continue;
			}
		}

		SV_AddEntToSnapshot( eNums, e );
	}
}

/* SV_GetClientArchiveTime 0x0045C8A0 */

#ifdef SV_ARCHIVE_SELFTEST
cvar_t  *sv_archiveForceTime;
#endif

/* ---- SV_GetClientArchiveTime  0x0045C8A0 ---- */
/* Retail is just push clientNum; push 12h; call VM_Call. */
int SV_GetClientArchiveTime( int clientNum ) {
#ifdef SV_ARCHIVE_SELFTEST
	if ( sv_archiveForceTime && sv_archiveForceTime->integer ) {
		return sv_archiveForceTime->integer;
	}
#endif
	return VM_Call( vm, GAME_GET_ARCHIVE_TIME, clientNum );
}

/* ---- SV_AddCachedEntitiesVisibleFromPoint  0x0045C8C0 ---- */
void SV_AddCachedEntitiesVisibleFromPoint( int leafnum, int count, int firstEnt,
										   int clientNum, int *eNums,
										   const playerState_t *ps ) {
	const archivedEntity_t  *ae;
	const byte  *bitvector;
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int lastLeaf;
	int area;
	int i, j, num;
	int svFlags;
	int leafCluster;

	area = CM_LeafArea( leafnum );
	if ( area < 0 ) {
		return;
	}
	bitvector = CM_ClusterPVS( CM_LeafCluster( leafnum ) );

	for ( i = 0 ; i < count ; i++ ) {
		ae = &SVS_CACHED_ENTITIES[ ( firstEnt + i )
								   % SV_NUM_CACHED_SNAPSHOT_ENTITIES ];
		svFlags = ae->svFlags;

		if ( ( svFlags & SVF_SINGLECLIENT ) && ae->singleClient != clientNum ) {
			continue;
		}
		if ( ( svFlags & SVF_NOTSINGLECLIENT ) && ae->singleClient == clientNum ) {
			continue;
		}

		if ( ae->s.number == clientNum
			 && ( ps->pm_flags & PMF_ARCHIVE_HIDE_VIEWENTITY ) ) {
			continue;
		}

		if ( svFlags & ( SVF_BROADCAST | SVF_PORTAL ) ) {
			SV_AddArchivedEntToSnapshot( eNums, i );
			continue;
		}

		num = CM_BoxLeafnums( ae->absmin, ae->absmax, leafs,
							  MAX_TOTAL_ENT_LEAFS, &lastLeaf );
		if ( !num ) {
			continue;
		}

		for ( j = 0 ; j < num ; j++ ) {
			if ( CM_AreasConnected( area, CM_LeafArea( leafs[j] ) ) ) {
				break;
			}
		}
		if ( j == num ) {
			continue;
		}

		for ( j = 0 ; j < num ; j++ ) {
			leafCluster = CM_LeafCluster( leafs[j] );
			if ( leafCluster == -1 ) {
				continue;
			}
			if ( bitvector[leafCluster >> 3] & ( 1 << ( leafCluster & 7 ) ) ) {
				break;
			}
		}
		if ( j == num ) {
			continue;
		}

		SV_AddArchivedEntToSnapshot( eNums, i );
	}
}

/* ---- SV_GetClientState  0x0045CAC0 ---- */
void *SV_GetClientState( int clientNum ) {
	return (void *)VM_Call( vm, GAME_GET_CLIENT_STATE, clientNum );
}

/* ---- SV_SetClientArchiveTime  0x0045CAE0 ---- */
void SV_SetClientArchiveTime( int clientNum, int time ) {
	VM_Call( vm, GAME_SET_ARCHIVE_TIME, clientNum, time );
}

/* ---- SV_ArchivedFrameIsLive  no-address ---- */
static qboolean SV_ArchivedFrameIsLive( int frameNum ) {
	const archivedSnapshot_t *archived;

	archived = &svs.archivedSnapshotFrames[ frameNum
											% SV_NUM_ARCHIVED_SNAPSHOT_FRAMES ];
	return (qboolean)( archived->start >= svs.nextArchivedSnapshotBuffer
										 - SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES );
}

/* ---- SV_GetCachedSnapshotInternal  0x0045CB00 ---- */
cachedSnapshot_t *SV_GetCachedSnapshotInternal( int frameNum ) {
	msg_t msg;
	const archivedSnapshot_t    *archived;
	cachedSnapshot_t    *cache;
	cachedSnapshot_t    *base;
	const cachedClient_t    *oldcc;
	cachedClient_t      *cc;
	int oldestFrame, frameIndex;
	int baseFrame;
	int number, cachedNum, cachedIndex;
	int offset, room;
	byte buf[SV_ARCHIVE_MSG_BYTES];

	archived = &svs.archivedSnapshotFrames[ frameNum
											% SV_NUM_ARCHIVED_SNAPSHOT_FRAMES ];
	if ( archived->start
		 < svs.nextArchivedSnapshotBuffer - SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES ) {
		return NULL;
	}

	oldestFrame = svs.nextCachedSnapshotFrames - SV_NUM_CACHED_SNAPSHOT_FRAMES;
	if ( oldestFrame < 0 ) {
		oldestFrame = 0;
	}
	for ( frameIndex = svs.nextCachedSnapshotFrames - 1
		; frameIndex >= oldestFrame
		; frameIndex-- ) {
		cache = &SVS_CACHED_FRAMES[ frameIndex % SV_NUM_CACHED_SNAPSHOT_FRAMES ];
		if ( cache->archivedFrame != frameNum ) {
			continue;
		}
		if ( cache->first_entity
			 >= svs.nextCachedSnapshotEntities - SV_NUM_CACHED_SNAPSHOT_ENTITIES
			 && cache->first_client
				>= svs.nextCachedSnapshotClients - SV_NUM_CACHED_SNAPSHOT_CLIENTS ) {
			return cache;
		}
		break;
	}

	MSG_Init( &msg, buf, SV_ARCHIVE_MSG_BYTES );
	msg.cursize = archived->len;

	offset = archived->start % SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES;
	room = SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES - offset;
	if ( msg.cursize <= room ) {
		memcpy( msg.data, svs.archivedSnapshotBuffer + offset, (size_t)msg.cursize );
	} else {
		memcpy( msg.data, svs.archivedSnapshotBuffer + offset, (size_t)room );
		memcpy( msg.data + room, svs.archivedSnapshotBuffer,
				(size_t)( msg.cursize - room ) );
	}

	if ( !MSG_ReadBit( &msg ) ) {
		baseFrame = MSG_ReadLong( &msg );
		if ( baseFrame < svs.nextArchivedSnapshotFrames
						 - SV_NUM_ARCHIVED_SNAPSHOT_FRAMES ) {
			return NULL;
		}
		if ( !SV_ArchivedFrameIsLive( baseFrame ) ) {
			return NULL;
		}
		base = SV_GetCachedSnapshotInternal( baseFrame );
		if ( !base ) {
			return NULL;
		}

		cache = &SVS_CACHED_FRAMES[ svs.nextCachedSnapshotFrames
									% SV_NUM_CACHED_SNAPSHOT_FRAMES ];
		cache->archivedFrame = frameNum;
		cache->num_entities  = 0;
		cache->first_entity  = svs.nextCachedSnapshotEntities;
		cache->num_clients   = 0;
		cache->first_client  = svs.nextCachedSnapshotClients;
		cache->usesDelta     = 1;
		cache->time          = MSG_ReadLong( &msg );

		cachedIndex = 0;
		oldcc = NULL;
		if ( cachedIndex >= base->num_clients ) {
			cachedNum = SV_NO_MORE_CLIENTS;
		} else {
			oldcc = &SVS_CACHED_CLIENTS[ ( base->first_client + cachedIndex )
										 % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
			cachedNum = *(const int *)oldcc->cs;
		}

		while ( MSG_ReadBit( &msg ) ) {
			number = MSG_ReadBits( &msg, MSG_CLIENT_NUM_BITS );
			if ( msg.readcount > msg.cursize ) {
				Com_Error( ERR_DROP, "\x15SV_GetCachedSnapshot: end of message" );
			}

			while ( cachedNum < number ) {
				cachedIndex++;
				if ( cachedIndex >= base->num_clients ) {
					cachedNum = SV_NO_MORE_CLIENTS;
				} else {
					oldcc = &SVS_CACHED_CLIENTS[ ( base->first_client + cachedIndex )
												 % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
					cachedNum = *(const int *)oldcc->cs;
				}
			}

			if ( cachedNum == number ) {
				cc = &SVS_CACHED_CLIENTS[ svs.nextCachedSnapshotClients
										  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
				MSG_ReadDeltaClient( &msg, oldcc->cs, cc->cs, number );
				cc->havePlayerState = MSG_ReadBit( &msg );
				if ( cc->havePlayerState ) {
					MSG_ReadDeltaPlayerstate( &msg, (const byte *)&oldcc->ps,
											  (byte *)&cc->ps );
				}

				svs.nextCachedSnapshotClients++;
				if ( svs.nextCachedSnapshotClients >= SV_SNAPSHOT_COUNTER_LIMIT ) {
					Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotClients wrapped" );
				}
				cache->num_clients++;

				cachedIndex++;
				if ( cachedIndex >= base->num_clients ) {
					cachedNum = SV_NO_MORE_CLIENTS;
				} else {
					oldcc = &SVS_CACHED_CLIENTS[ ( base->first_client + cachedIndex )
												 % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
					cachedNum = *(const int *)oldcc->cs;
				}
			} else {
				cc = &SVS_CACHED_CLIENTS[ svs.nextCachedSnapshotClients
										  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
				MSG_ReadDeltaClient( &msg, NULL, cc->cs, number );
				cc->havePlayerState = MSG_ReadBit( &msg );
				if ( cc->havePlayerState ) {
					MSG_ReadDeltaPlayerstate( &msg, NULL, (byte *)&cc->ps );
				}

				svs.nextCachedSnapshotClients++;
				if ( svs.nextCachedSnapshotClients >= SV_SNAPSHOT_COUNTER_LIMIT ) {
					Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotClients wrapped" );
				}
				cache->num_clients++;
			}
		}

		while ( ( number = MSG_ReadBits( &msg, 10 ) ) != ENTITYNUM_NONE ) {
			if ( msg.readcount > msg.cursize ) {
				Com_Error( ERR_DROP, "\x15SV_GetCachedSnapshot: end of message" );
			}

			MSG_ReadDeltaArchivedEntity( &msg,
				(const byte *)&SV_SvEntityNum( number )->baseline,
				(byte *)&SVS_CACHED_ENTITIES[ svs.nextCachedSnapshotEntities
											  % SV_NUM_CACHED_SNAPSHOT_ENTITIES ],
				number );

			svs.nextCachedSnapshotEntities++;
			if ( svs.nextCachedSnapshotEntities >= SV_SNAPSHOT_COUNTER_LIMIT ) {
				Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotEntities wrapped" );
			}
			cache->num_entities++;
		}
	} else {
		cache = &SVS_CACHED_FRAMES[ svs.nextCachedSnapshotFrames
									% SV_NUM_CACHED_SNAPSHOT_FRAMES ];
		cache->archivedFrame = frameNum;
		cache->num_entities  = 0;
		cache->first_entity  = svs.nextCachedSnapshotEntities;
		cache->num_clients   = 0;
		cache->first_client  = svs.nextCachedSnapshotClients;
		cache->usesDelta     = 0;
		cache->time          = MSG_ReadLong( &msg );

		while ( MSG_ReadBit( &msg ) ) {
			number = MSG_ReadBits( &msg, MSG_CLIENT_NUM_BITS );
			if ( msg.readcount > msg.cursize ) {
				Com_Error( ERR_DROP, "\x15SV_GetCachedSnapshot: end of message" );
			}

			cc = &SVS_CACHED_CLIENTS[ svs.nextCachedSnapshotClients
									  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
			MSG_ReadDeltaClient( &msg, NULL, cc->cs, number );
			cc->havePlayerState = MSG_ReadBit( &msg );
			if ( cc->havePlayerState ) {
				MSG_ReadDeltaPlayerstate( &msg, NULL, (byte *)&cc->ps );
			}

			svs.nextCachedSnapshotClients++;
			if ( svs.nextCachedSnapshotClients >= SV_SNAPSHOT_COUNTER_LIMIT ) {
				Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotClients wrapped" );
			}
			cache->num_clients++;
		}

		while ( ( number = MSG_ReadBits( &msg, 10 ) ) != ENTITYNUM_NONE ) {
			if ( msg.readcount > msg.cursize ) {
				Com_Error( ERR_DROP, "\x15SV_GetCachedSnapshot: end of message" );
			}

			MSG_ReadDeltaArchivedEntity( &msg,
				(const byte *)&SV_SvEntityNum( number )->baseline,
				(byte *)&SVS_CACHED_ENTITIES[ svs.nextCachedSnapshotEntities
											  % SV_NUM_CACHED_SNAPSHOT_ENTITIES ],
				number );

			svs.nextCachedSnapshotEntities++;
			if ( svs.nextCachedSnapshotEntities >= SV_SNAPSHOT_COUNTER_LIMIT ) {
				Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotEntities wrapped" );
			}
			cache->num_entities++;
		}
	}

	svs.nextCachedSnapshotFrames++;
	if ( svs.nextCachedSnapshotFrames >= SV_SNAPSHOT_COUNTER_LIMIT ) {
		Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotFrames wrapped" );
	}

	return cache;
}

/* ---- SV_GetCachedSnapshot  0x0045D400 ---- */
cachedSnapshot_t *SV_GetCachedSnapshot( int *frameTime ) {
	cachedSnapshot_t    *cache;
	int frameNum;

	if ( !svs.archiveEnabled ) {
		return NULL;
	}
	if ( *frameTime <= 0 ) {
		return NULL;
	}

	frameNum = svs.nextArchivedSnapshotFrames - ( *frameTime * sv_fps->integer ) / 1000;

	if ( frameNum < svs.nextArchivedSnapshotFrames - SV_NUM_ARCHIVED_SNAPSHOT_FRAMES ) {
		frameNum = svs.nextArchivedSnapshotFrames - SV_NUM_ARCHIVED_SNAPSHOT_FRAMES;
		*frameTime = ( svs.nextArchivedSnapshotFrames - frameNum ) * 1000 / sv_fps->integer;
	}
	if ( frameNum < 0 ) {
		frameNum = 0;
		*frameTime = ( svs.nextArchivedSnapshotFrames - frameNum ) * 1000 / sv_fps->integer;
	}

	for ( ; frameNum < svs.nextArchivedSnapshotFrames ; frameNum++ ) {
		cache = SV_GetCachedSnapshotInternal( frameNum );
		if ( cache ) {
			return cache;
		}
	}

	*frameTime = 0;
	return NULL;
}

/* ---- SV_GetFollowPlayerState  0x0045D4B0 ---- */
void *SV_GetFollowPlayerState( int clientNum, void *playerState ) {
	return (void *)VM_Call( vm, GAME_GET_CLIENT_INFO, clientNum, playerState );
}

/* ---- SV_GetCurrentClientInfo  0x0045D4D0 ---- */
void *SV_GetCurrentClientInfo( int clientNum, void *clientInfo ) {
	if ( svs.clients[clientNum].state != CS_ACTIVE ) {
		return NULL;
	}
	return (void *)VM_Call( vm, GAME_GET_CLIENT_INFO, clientNum, clientInfo );
}

#define SV_PS_HUD_ELEMS             31

/* ---- SV_RebaseTime  no-address ----
 * A macro for the same reason as SV_SvEntityNum (no retail dead copy). */
#define SV_RebaseTime( t, timeDelta ) \
	do { \
		if ( *( t ) ) { \
			*( t ) += ( timeDelta ); \
		} \
	} while ( 0 )

/* ---- SV_GetArchivedClientInfo  0x0045D500 ---- */
qboolean SV_GetArchivedClientInfo( int *frameTime, void *clientInfo, int clientNum ) {
	const cachedSnapshot_t  *cache;
	const cachedClient_t    *cc;
	playerState_t   *ps;
	hudelem_t       *he;
	int timeDelta;
	int i;

	cache = SV_GetCachedSnapshot( frameTime );
	if ( !cache ) {
		if ( *frameTime <= 0 && svs.clients[clientNum].state == CS_ACTIVE ) {
			return (qboolean)VM_Call( vm, GAME_GET_CLIENT_INFO, clientNum, clientInfo );
		}
		return qfalse;
	}

#ifdef SV_ARCHIVE_SELFTEST
	sv_archiveCachedClientInfos++;
#endif
	timeDelta = svs.time - cache->time;

	for ( i = 0 ; i < cache->num_clients ; i++ ) {
		cc = &SVS_CACHED_CLIENTS[ ( cache->first_client + i )
								  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
		if ( *(const int *)cc->cs != clientNum ) {
			continue;
		}
		if ( !cc->havePlayerState ) {
			return qfalse;
		}
		ps = (playerState_t *)clientInfo;
		*ps = cc->ps;

		if ( ps->commandTime ) {
			ps->commandTime += timeDelta;
		}
		if ( ps->pm_time ) {
			ps->pm_time += timeDelta;
		}
		if ( ps->iFoliageSoundTime ) {
			ps->iFoliageSoundTime += timeDelta;
		}
		if ( ps->jumpTime ) {
			ps->jumpTime += timeDelta;
		}
		if ( ps->viewHeightLerpTime ) {
			ps->viewHeightLerpTime += timeDelta;
		}
		if ( ps->shellshockTime ) {
			ps->shellshockTime += timeDelta;
		}

		/* Only the ARCHIVAL hud array is rebased, not the current one. */
		for ( i = 0 ; i < SV_PS_HUD_ELEMS ; i++ ) {
			he = &ps->hud.archival[i];
			if ( he->timerValue ) {
				he->timerValue += timeDelta;
			}
			if ( he->fadeStartTime ) {
				he->fadeStartTime += timeDelta;
			}
			if ( he->scaleStartTime ) {
				he->scaleStartTime += timeDelta;
			}
			if ( he->moveStartTime ) {
				he->moveStartTime += timeDelta;
			}
		}

		/* Not guarded -- retail adds unconditionally. */
		ps->deltaTime += timeDelta;
		return qtrue;
	}
	return qfalse;
}

/* ---- SV_BuildClientSnapshot  0x0045D650 ---- */
void SV_BuildClientSnapshot( client_t *cl ) {
	clientSnapshot_t    *frame;
	const cachedSnapshot_t  *cache;
	const cachedClient_t    *cc;
	const archivedEntity_t  *ae;
	entityState_t       *es;
	playerState_t       *ps;
	svSnapshotClient_t  *snapClient;
	int clientNum;
	int viewEnt;
	int frameTime;
	int timeDelta;
	int i;
	int leafnum;
	vec3_t org;
	int eNums[MAX_SNAPSHOT_ENTITIES + 1];

	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	frame->num_entities = 0;
	frame->num_clients = 0;

	if ( !cl->gentity ) {
		return;
	}
	if ( cl->state == CS_ZOMBIE ) {
		return;
	}

	frame->first_entity = svs.nextSnapshotEntities;
	frame->first_client = svs.nextSnapshotClients;

	if ( sv.state == SS_DEAD ) {
		return;
	}

	clientNum = (int)( cl - svs.clients );
	eNums[0] = 0;

	frameTime = SV_GetClientArchiveTime( clientNum );
	cache = SV_GetCachedSnapshot( &frameTime );
	SV_SetClientArchiveTime( clientNum, frameTime );

	if ( cache ) {
		timeDelta = svs.time - cache->time;
	} else {
		timeDelta = 0;
	}

	ps = (playerState_t *)( (byte *)sv.gameClients + clientNum * sv.gameClientSize );
	frame->ps = *ps;

	viewEnt = frame->ps.clientNum;
	if ( viewEnt < 0 || viewEnt >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "\x15SV_BuildClientSnapshot: bad gEnt" );
	}

	VectorCopy( frame->ps.origin, org );
	org[2] += frame->ps.viewHeightCurrent;
	AddLeanToPosition( org, frame->ps.viewangles[1], frame->ps.leanf,
					   16.0f, 20.0f );

	leafnum = CM_PointLeafnum( org );

	if ( !cache ) {
		SV_AddEntitiesVisibleFromPoint( leafnum, viewEnt, eNums );

		for ( i = 0 ; i < eNums[0] ; i++ ) {
			svs.snapshotEntities[svs.nextSnapshotEntities % svs.numSnapshotEntities] =
				*(const entityState_t *)SV_GentityNum( eNums[i + 1] );

			svs.nextSnapshotEntities++;
			if ( svs.nextSnapshotEntities >= SV_SNAPSHOT_COUNTER_LIMIT ) {
				Com_Error( ERR_FATAL, "\x15svs.nextSnapshotEntities wrapped" );
			}
			frame->num_entities++;
		}

		/* cl is dead here; retail walks the clients in its incoming stack slot. */
		for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
			if ( cl->state < CS_CONNECTED ) {
				continue;
			}
			snapClient = &SVS_SNAPSHOT_CLIENTS[svs.nextSnapshotClients
											   % svs.numSnapshotClients];
			*snapClient = *(const svSnapshotClient_t *)SV_GetClientState( i );

			svs.nextSnapshotClients++;
			if ( svs.nextSnapshotClients >= SV_SNAPSHOT_COUNTER_LIMIT ) {
				Com_Error( ERR_FATAL, "\x15svs.nextSnapshotClients wrapped" );
			}
			frame->num_clients++;
		}
		return;
	}

#ifdef SV_ARCHIVE_SELFTEST
	sv_archiveCachedBuilds++;
#endif
	SV_AddCachedEntitiesVisibleFromPoint( leafnum, cache->num_entities,
										  cache->first_entity, viewEnt,
										  eNums, &frame->ps );

	for ( i = 0 ; i < eNums[0] ; i++ ) {
		ae = &SVS_CACHED_ENTITIES[ ( eNums[i + 1] + cache->first_entity )
								   % SV_NUM_CACHED_SNAPSHOT_ENTITIES ];

		es = &svs.snapshotEntities[ svs.nextSnapshotEntities
									% svs.numSnapshotEntities ];
		*es = ae->s;

		SV_RebaseTime( &es->pos.trTime, timeDelta );
		SV_RebaseTime( &es->apos.trTime, timeDelta );
		SV_RebaseTime( &es->time, timeDelta );
		SV_RebaseTime( &es->time2, timeDelta );

		svs.nextSnapshotEntities++;
		if ( svs.nextSnapshotEntities >= SV_SNAPSHOT_COUNTER_LIMIT ) {
			Com_Error( ERR_FATAL, "\x15svs.nextSnapshotEntities wrapped" );
		}
		frame->num_entities++;
	}

	for ( i = 0 ; i < cache->num_clients ; i++ ) {
		cc = &SVS_CACHED_CLIENTS[ ( cache->first_client + i )
								  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
		memcpy( &SVS_SNAPSHOT_CLIENTS[svs.nextSnapshotClients % svs.numSnapshotClients],
				cc->cs, SV_SNAPSHOT_CLIENT_BYTES );

		svs.nextSnapshotClients++;
		if ( svs.nextSnapshotClients >= SV_SNAPSHOT_COUNTER_LIMIT ) {
			Com_Error( ERR_FATAL, "\x15svs.nextSnapshotClients wrapped" );
		}
		frame->num_clients++;
	}
}

/* ---- SV_RateMsec  0x0045DA80 ---- */
int SV_RateMsec( client_t *client, int messageSize ) {
	int rate;

	if ( messageSize > 1500 ) {
		messageSize = 1500;
	}

	rate = client->rate;
	if ( sv_maxRate->integer ) {
		if ( sv_maxRate->integer < 1000 ) {
			Cvar_Set2( "sv_MaxRate", "1000", qtrue );
		}
		if ( sv_maxRate->integer < rate ) {
			rate = sv_maxRate->integer;
		}
	}

	return ( messageSize + 48 ) * 1000 / rate;
}

/* ---- SV_SendMessageToClient  0x0045DAE0 ---- */
void SV_SendMessageToClient( msg_t *msg, client_t *client ) {
	int rateMsec;
	int len;
	byte outBuf[MAX_MSGLEN];

	*(int *)outBuf = *(const int *)msg->data;
	len = 4 + MSG_WriteBitsCompress( msg->data + 4, msg->cursize - 4, outBuf + 4 );

	if ( client->dropReason ) {
		SV_DropClient( client, client->dropReason );
	}

	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSize = len;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageSent = svs.time;
	client->frames[client->netchan.outgoingSequence & PACKET_MASK].messageAcked = -1;

	SV_Netchan_Encode( client, outBuf + 4, len - 4 );
	Netchan_Transmit( &client->netchan, len, outBuf );

	if ( client->netchan.remoteAddress.type == NA_LOOPBACK
		 || Sys_IsLANAddress( client->netchan.remoteAddress ) ) {
		client->nextSnapshotTime = svs.time - 1;
		return;
	}

	rateMsec = SV_RateMsec( client, len );
	if ( rateMsec < client->snapshotMsec ) {
		rateMsec = client->snapshotMsec;
		client->rateDelayed = qfalse;
	} else {
		client->rateDelayed = qtrue;
	}

	client->nextSnapshotTime = svs.time + rateMsec;

	if ( client->state != CS_ACTIVE && !client->downloadName[0]
		 && client->nextSnapshotTime < svs.time + 1000 ) {
		client->nextSnapshotTime = svs.time + 1000;
	}

	sv.bpsTotalBytes += len;
}

/* ---- SV_SendClientSnapshot  0x0045DC70 ---- */
void SV_SendClientSnapshot( client_t *cl ) {
	msg_t msg;
	byte msgBuffer[MAX_MSGLEN];

	if ( cl->state == CS_ACTIVE || cl->state == CS_ZOMBIE ) {
		SV_BuildClientSnapshot( cl );
	}

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );

	MSG_WriteLong( &msg, cl->lastClientCommand );

	if ( cl->state == CS_ACTIVE || cl->state == CS_ZOMBIE ) {
		SV_UpdateServerCommandsToClient( cl, &msg );
		SV_WriteSnapshotToClient( cl, &msg );
	}
	if ( cl->state != CS_ZOMBIE ) {
		SV_WriteDownloadToClient( cl, &msg );
	}

	MSG_WriteByte( &msg, svc_EOF );

	if ( msg.overflowed ) {
		Com_Printf( "WARNING: msg overflowed for %s, trying to recover\n",
					cl->name );

		if ( cl->state == CS_ACTIVE || cl->state == CS_ZOMBIE ) {
			SV_PrintServerCommandsForClient( cl );

			MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );
			MSG_WriteLong( &msg, cl->lastClientCommand );
			SV_UpdateServerCommandsToClient_PreventOverflow( &msg, cl, MAX_MSGLEN );
			MSG_WriteByte( &msg, svc_EOF );
		}

		if ( msg.overflowed ) {
			Com_Printf( "WARNING: client disconnected for msg overflow: %s\n",
						cl->name );
			NET_OutOfBandPrint( NS_SERVER, cl->netchan.remoteAddress,
								"disconnect" );
			SV_DropClient( cl, "EXE_SERVERMESSAGEOVERFLOW" );
		}
	}

	SV_SendMessageToClient( &msg, cl );
}

/* SV_ArchiveSnapshot 0x0045DE80 */

/* svFlags bits that force an entity into the archive regardless of whether it is in any PVS cluster. Retail tests `& 18h` (0x0045E5D9, 0x0045E20C). */
#define SVF_ARCHIVE_ALWAYS      ( SVF_BROADCAST | SVF_PORTAL )

#ifdef SV_ARCHIVE_SELFTEST
/* Tree-only diagnostic, not in retail; see SV_ARCHIVE_SELFTEST at the top of the file. */
/* ---- SV_FillArchivedEntity  no-address ---- */
static void SV_FillArchivedEntity( archivedEntity_t *ae, const gentity_t *ent ) {
	ae->s = ent->s;
	ae->svFlags = ent->r.svFlags;
	ae->singleClient = ent->r.singleClient;
	ae->absmin[0] = ent->r.absmin[0];
	ae->absmin[1] = ent->r.absmin[1];
	ae->absmin[2] = ent->r.absmin[2];
	ae->absmax[0] = ent->r.absmax[0];
	ae->absmax[1] = ent->r.absmax[1];
	ae->absmax[2] = ent->r.absmax[2];
}

/* ---- SV_ArchiveEntityVisible  no-address ---- */
static qboolean SV_ArchiveEntityVisible( const gentity_t *ent ) {
	const svEntity_t *svEnt;

	if ( !ent->r.linked ) {
		return qfalse;
	}
	if ( ent->r.svFlags & SVF_NOCLIENT ) {
		return qfalse;
	}
	if ( ent->s.number < 0 || ent->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "\x15SV_SvEntityForGentity: bad gEnt" );
	}
	svEnt = SV_SvEntityNum( ent->s.number );
	if ( ( ent->r.svFlags & SVF_ARCHIVE_ALWAYS ) == 0
		 && !svEnt->numClusters ) {
		return qfalse;
	}
	return qtrue;
}

static cvar_t   *sv_archiveSelfTest;

static byte     st_scratch[0x8000];
static playerState_t    st_canonPs;
static archivedEntity_t st_canonEnt;

/* ---- SV_CanonicalPlayerState  no-address ---- */
static void SV_CanonicalPlayerState( const playerState_t *live ) {
	msg_t m;

	MSG_Init( &m, st_scratch, sizeof( st_scratch ) );
	MSG_WriteDeltaPlayerstate( &m, NULL, (const byte *)live );
	MSG_BeginReading( &m );
	MSG_ReadDeltaPlayerstate( &m, NULL, (byte *)&st_canonPs );
}

/* ---- SV_CanonicalArchivedEntity  no-address ---- */
static void SV_CanonicalArchivedEntity( const byte *baseline,
										const archivedEntity_t *live ) {
	msg_t m;
	int number;

	MSG_Init( &m, st_scratch, sizeof( st_scratch ) );
	MSG_WriteDeltaArchivedEntity( &m, baseline, (const byte *)live, qtrue );
	MSG_BeginReading( &m );

	number = MSG_ReadBits( &m, 10 );

	MSG_ReadDeltaArchivedEntity( &m, baseline, (byte *)&st_canonEnt, number );
}

/* ---- SV_ArchiveSelfTest  no-address ---- */
static void SV_ArchiveSelfTest( int frameNum ) {
	const cachedSnapshot_t  *decoded;
	cachedSnapshot_t        *writerEntry;
	const cachedClient_t    *cc;
	const archivedEntity_t  *ae;
	const gentity_t         *ent;
	archivedEntity_t expectEnt;
	playerState_t expectPs;
	int savedArchivedFrame;
	int frameIndex, oldestFrame;
	int i, k, bad;

	writerEntry = NULL;
	savedArchivedFrame = 0;
	oldestFrame = svs.nextCachedSnapshotFrames - SV_NUM_CACHED_SNAPSHOT_FRAMES;
	if ( oldestFrame < 0 ) {
		oldestFrame = 0;
	}
	for ( frameIndex = svs.nextCachedSnapshotFrames - 1
		; frameIndex >= oldestFrame
		; frameIndex-- ) {
		writerEntry = &SVS_CACHED_FRAMES[ frameIndex
										  % SV_NUM_CACHED_SNAPSHOT_FRAMES ];
		if ( writerEntry->archivedFrame == frameNum ) {
			break;
		}
		writerEntry = NULL;
	}
	if ( writerEntry ) {
		savedArchivedFrame = writerEntry->archivedFrame;
		writerEntry->archivedFrame = -1;
	}

	decoded = SV_GetCachedSnapshotInternal( frameNum );

	if ( writerEntry ) {
		writerEntry->archivedFrame = savedArchivedFrame;
	}

	if ( !decoded ) {
		Com_Printf( "archiveSelfTest %d: FAIL -- decode returned NULL\n",
					frameNum );
		return;
	}

	bad = 0;

	k = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state < CS_CONNECTED ) {
			continue;
		}
		if ( k >= decoded->num_clients ) {
			Com_Printf( "archiveSelfTest %d: FAIL -- client %d missing"
						" (decoded %d)\n", frameNum, i, decoded->num_clients );
			bad++;
			break;
		}
		cc = &SVS_CACHED_CLIENTS[ ( decoded->first_client + k )
								  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];

		if ( memcmp( cc->cs, SV_GetClientState( i ),
					 SV_SNAPSHOT_CLIENT_BYTES ) != 0 ) {
			Com_Printf( "archiveSelfTest %d: FAIL -- client %d record differs\n",
						frameNum, i );
			bad++;
		}

		memset( &expectPs, 0, sizeof( expectPs ) );
		if ( VM_Call( vm, GAME_GET_CLIENT_INFO, i, &expectPs ) ) {
			if ( !cc->havePlayerState ) {
				Com_Printf( "archiveSelfTest %d: FAIL -- client %d lost its"
							" playerState\n", frameNum, i );
				bad++;
			} else if ( memcmp( &cc->ps, &expectPs, sizeof( expectPs ) ) != 0 ) {
				const int *got;
				const int *want;
				const int *canon;
				int wrong = 0;
				int w;

				SV_CanonicalPlayerState( &expectPs );
				got = (const int *)&cc->ps;
				want = (const int *)&expectPs;
				canon = (const int *)&st_canonPs;

				for ( w = 0 ; w < (int)( sizeof( expectPs ) / 4 ) ; w++ ) {
					if ( got[w] == want[w] ) {
						continue;
					}
					if ( got[w] == canon[w] ) {
						continue;
					}
					if ( canon[w] != want[w] ) {
						continue;
					}
					if ( wrong < 8 ) {
						Com_Printf( "archiveSelfTest %d: FAIL -- client %d"
									" playerState +0x%04X got %08X want %08X"
									" canon %08X\n",
									frameNum, i, w * 4, got[w], want[w],
									canon[w] );
					}
					wrong++;
				}
				if ( wrong ) {
					Com_Printf( "    %d words wrong of %d\n",
								wrong, (int)( sizeof( expectPs ) / 4 ) );
					bad++;
				}
			}
		} else if ( cc->havePlayerState ) {
			Com_Printf( "archiveSelfTest %d: FAIL -- client %d gained a"
						" playerState\n", frameNum, i );
			bad++;
		}
		k++;
	}
	if ( k != decoded->num_clients ) {
		Com_Printf( "archiveSelfTest %d: FAIL -- %d clients decoded, %d live\n",
					frameNum, decoded->num_clients, k );
		bad++;
	}

	k = 0;
	for ( i = 0 ; i < sv.num_entities ; i++ ) {
		ent = SV_GentityNum( i );
		if ( !SV_ArchiveEntityVisible( ent ) ) {
			continue;
		}
		if ( k >= decoded->num_entities ) {
			Com_Printf( "archiveSelfTest %d: FAIL -- entity %d missing"
						" (decoded %d)\n", frameNum, i, decoded->num_entities );
			bad++;
			break;
		}
		ae = &SVS_CACHED_ENTITIES[ ( decoded->first_entity + k )
								   % SV_NUM_CACHED_SNAPSHOT_ENTITIES ];
		SV_FillArchivedEntity( &expectEnt, ent );

		SV_CanonicalArchivedEntity(
			(const byte *)&SV_SvEntityNum( ent->s.number )->baseline,
			&expectEnt );

		if ( memcmp( ae, &st_canonEnt, sizeof( st_canonEnt ) ) != 0 ) {
			const int *got = (const int *)ae;
			const int *canon = (const int *)&st_canonEnt;
			const int *want = (const int *)&expectEnt;
			int shown = 0;
			int w;

			for ( w = 0 ; w < (int)( sizeof( expectEnt ) / 4 ) ; w++ ) {
				if ( got[w] == canon[w] ) {
					continue;
				}
				if ( shown < 6 ) {
					Com_Printf( "archiveSelfTest %d: FAIL -- entity %d"
								" +0x%04X got %08X canon %08X live %08X\n",
								frameNum, i, w * 4, got[w], canon[w], want[w] );
				}
				shown++;
			}
			bad++;
		}
		k++;
	}
	if ( k != decoded->num_entities ) {
		Com_Printf( "archiveSelfTest %d: FAIL -- %d entities decoded, %d live\n",
					frameNum, decoded->num_entities, k );
		bad++;
	}

	if ( sv_archiveForceTime && sv_archiveForceTime->integer && !bad ) {
		static playerState_t probePs;
		const cachedSnapshot_t *served;
		int askTime, lookTime;

		for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
			if ( svs.clients[i].state < CS_CONNECTED ) {
				continue;
			}

			lookTime = sv_archiveForceTime->integer;
			served = SV_GetCachedSnapshot( &lookTime );
			if ( !served ) {
				continue;
			}
			cc = NULL;
			for ( k = 0 ; k < served->num_clients ; k++ ) {
				const cachedClient_t *probe =
					&SVS_CACHED_CLIENTS[ ( served->first_client + k )
										 % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
				if ( *(const int *)probe->cs == i ) {
					cc = probe;
					break;
				}
			}
			if ( !cc || !cc->havePlayerState ) {
				continue;
			}

			askTime = sv_archiveForceTime->integer;
			memset( &probePs, 0, sizeof( probePs ) );
			if ( !SV_GetArchivedClientInfo( &askTime, &probePs, i ) ) {
				Com_Printf( "archiveSelfTest %d: FAIL -- GetArchivedClientInfo"
							" refused client %d\n", frameNum, i );
				bad++;
				continue;
			}

			{
				const int *served_ps = (const int *)&cc->ps;
				const int *out = (const int *)&probePs;
				int stray = 0;
				int w, off;

				for ( w = 0 ; w < (int)( sizeof( probePs ) / 4 ) ; w++ ) {
					off = w * 4;
					if ( out[w] == served_ps[w] ) {
						continue;
					}
					if ( off == 0x000 || off == 0x010 || off == 0x038
						 || off == 0x064 || off == 0x0D4 || off == 0x3E0
						 || off == 0x20CC ) {
						continue;
					}
					if ( off >= 4920 && off < 4920 + 31 * 112 ) {
						int within = ( off - 4920 ) % 112;
						if ( within == 0x24 || within == 0x44
							 || within == 0x54 || within == 0x5C ) {
							continue;
						}
					}
					if ( stray < 6 ) {
						Com_Printf( "archiveSelfTest %d: FAIL -- client %d"
									" archived info +0x%04X got %08X cached"
									" %08X\n", frameNum, i, off, out[w],
									served_ps[w] );
					}
					stray++;
				}
				if ( stray ) {
					bad++;
				}
			}
			sv_archiveCachedInfoChecks++;
		}
	}

	if ( !bad ) {
		Com_Printf( "archiveSelfTest %d: PASS %s -- %d clients, %d entities,"
					" time %d, cachedBuilds %d, cachedInfos %d, infoChecks %d\n",
					frameNum, writerEntry ? "keyframe" : "delta",
					decoded->num_clients, decoded->num_entities,
					decoded->time,
					sv_archiveCachedBuilds, sv_archiveCachedClientInfos,
					sv_archiveCachedInfoChecks );
	}
}
#endif /* SV_ARCHIVE_SELFTEST */

/* ---- SV_ArchiveSnapshot  0x0045DE80 ---- */
void SV_ArchiveSnapshot( void ) {
	msg_t msg;
	cachedSnapshot_t    *cache;
	cachedSnapshot_t    *candidate;
	archivedSnapshot_t  *archived;
	cachedClient_t      *cc;
	archivedEntity_t    *ae;
	archivedEntity_t localEnt;
	gentity_t           *ent;
	svEntity_t          *svEnt;
	client_t            *cl;
	playerState_t ps;
	int clientIndex, cachedIndex, cachedNum;
	int maxclients, numCached;
	int frameIndex, oldestFrame, staleBefore;
	int i, len, offset, room;
	byte buf[SV_ARCHIVE_MSG_BYTES];

	if ( sv.state == SS_DEAD ) {
		return;
	}
	if ( !svs.archiveEnabled ) {
		return;
	}

	/* MSG_Init is retail's own lazy MSG_initHuffman gate plus the memset, data and maxsize stores -- exactly what is inlined at 0x0045DEBC. */
	MSG_Init( &msg, buf, SV_ARCHIVE_MSG_BYTES );

	oldestFrame = svs.nextCachedSnapshotFrames - SV_NUM_CACHED_SNAPSHOT_FRAMES;
	if ( oldestFrame < 0 ) {
		oldestFrame = 0;
	}
	staleBefore = svs.nextArchivedSnapshotFrames - sv_fps->integer;

	cache = NULL;
	for ( frameIndex = svs.nextCachedSnapshotFrames - 1
		; frameIndex >= oldestFrame
		; frameIndex-- ) {
		candidate = &SVS_CACHED_FRAMES[ frameIndex % SV_NUM_CACHED_SNAPSHOT_FRAMES ];
		if ( candidate->archivedFrame >= staleBefore && !candidate->usesDelta ) {
			cache = candidate;
			break;
		}
	}

	if ( cache
		 && cache->first_entity
			>= svs.nextCachedSnapshotEntities - SV_NUM_CACHED_SNAPSHOT_ENTITIES
		 && cache->first_client
			>= svs.nextCachedSnapshotClients - SV_NUM_CACHED_SNAPSHOT_CLIENTS ) {
		/* Delta path: retail calls MSG_WriteBit0 and SV_SvEntityForGentity out of
		   line here (the keyframe path inlines them); inline_depth(0) marks each site. */
#pragma inline_depth(0)
		MSG_WriteBit0( &msg );
#pragma inline_depth()
		MSG_WriteLong( &msg, cache->archivedFrame );
		MSG_WriteLong( &msg, svs.time );

		maxclients = sv_maxclients->integer;
		numCached = cache->num_clients;
		cc = NULL;
		clientIndex = 0;
		cachedIndex = 0;

		while ( clientIndex < maxclients || cachedIndex < numCached ) {
			if ( clientIndex < maxclients
				 && svs.clients[clientIndex].state < CS_CONNECTED ) {
				clientIndex++;
				continue;
			}

			if ( cachedIndex >= numCached ) {
				cachedNum = 9999;
			} else {
				cc = &SVS_CACHED_CLIENTS[ ( cache->first_client + cachedIndex )
										  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
				cachedNum = *(const int *)cc->cs;
			}

			if ( clientIndex == cachedNum ) {
				MSG_WriteDeltaClient( &msg, cc->cs,
									  (const byte *)SV_GetClientState( clientIndex ),
									  qtrue );
				if ( VM_Call( vm, GAME_GET_CLIENT_INFO, clientIndex, &ps ) ) {
					MSG_WriteBit1( &msg );
					MSG_WriteDeltaPlayerstate( &msg, (const byte *)&cc->ps,
											   (const byte *)&ps );
				} else {
#pragma inline_depth(0)
					MSG_WriteBit0( &msg );
#pragma inline_depth()
				}
				cachedIndex++;
				clientIndex++;
				continue;
			}

			if ( clientIndex < cachedNum ) {
				MSG_WriteDeltaClient( &msg, NULL,
									  (const byte *)SV_GetClientState( clientIndex ),
									  qtrue );
				if ( VM_Call( vm, GAME_GET_CLIENT_INFO, clientIndex, &ps ) ) {
					MSG_WriteBit1( &msg );
					MSG_WriteDeltaPlayerstate( &msg, NULL, (const byte *)&ps );
				} else {
#pragma inline_depth(0)
					MSG_WriteBit0( &msg );
#pragma inline_depth()
				}
				clientIndex++;
				continue;
			}

			if ( clientIndex > cachedNum ) {
				cachedIndex++;
				continue;
			}
		}

#pragma inline_depth(0)
		MSG_WriteBit0( &msg );
#pragma inline_depth()

		for ( i = 0 ; i < sv.num_entities ; i++ ) {
			ent = SV_GentityNum( i );
			if ( !ent->r.linked ) {
				continue;
			}
			if ( ent->r.svFlags & SVF_NOCLIENT ) {
				continue;
			}
#pragma inline_depth(0)
			svEnt = SV_SvEntityForGentity( ent );
#pragma inline_depth()
			if ( !( ent->r.svFlags & SVF_ARCHIVE_ALWAYS )
				 && !svEnt->numClusters ) {
				continue;
			}

			localEnt.s = ent->s;
			localEnt.svFlags = ent->r.svFlags;
			localEnt.singleClient = ent->r.singleClient;
			VectorCopy( ent->r.absmin, localEnt.absmin );
			VectorCopy( ent->r.absmax, localEnt.absmax );
			MSG_WriteDeltaArchivedEntity( &msg,
				(const byte *)&SV_SvEntityNum( ent->s.number )->baseline,
				(const byte *)&localEnt, qtrue );
		}
	} else {
		MSG_WriteBit1( &msg );
		MSG_WriteLong( &msg, svs.time );

		cache = &SVS_CACHED_FRAMES[ svs.nextCachedSnapshotFrames
									% SV_NUM_CACHED_SNAPSHOT_FRAMES ];
		cache->archivedFrame = svs.nextArchivedSnapshotFrames;
		cache->num_entities  = 0;
		cache->first_entity  = svs.nextCachedSnapshotEntities;
		cache->num_clients   = 0;
		cache->first_client  = svs.nextCachedSnapshotClients;
		cache->usesDelta     = 0;
		cache->time          = svs.time;

		for ( i = 0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++ ) {
			if ( cl->state < CS_CONNECTED ) {
				continue;
			}

			cc = &SVS_CACHED_CLIENTS[ svs.nextCachedSnapshotClients
									  % SV_NUM_CACHED_SNAPSHOT_CLIENTS ];
			memcpy( cc->cs, SV_GetClientState( i ), sizeof( cc->cs ) );
			MSG_WriteDeltaClient( &msg, NULL, cc->cs, qtrue );

			cc->havePlayerState = VM_Call( vm, GAME_GET_CLIENT_INFO, i, &cc->ps );
			if ( cc->havePlayerState ) {
				MSG_WriteBit1( &msg );
				MSG_WriteDeltaPlayerstate( &msg, NULL, (const byte *)&cc->ps );
			} else {
				MSG_WriteBit0( &msg );
			}

			svs.nextCachedSnapshotClients++;
			if ( svs.nextCachedSnapshotClients >= SV_SNAPSHOT_COUNTER_LIMIT ) {
				Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotClients wrapped" );
			}
			cache->num_clients++;
		}

		MSG_WriteBit0( &msg );

		for ( i = 0 ; i < sv.num_entities ; i++ ) {
			ent = SV_GentityNum( i );
			if ( !ent->r.linked ) {
				continue;
			}
			if ( ent->r.svFlags & SVF_NOCLIENT ) {
				continue;
			}
			svEnt = SV_SvEntityForGentity( ent );
			if ( !( ent->r.svFlags & SVF_ARCHIVE_ALWAYS )
				 && !svEnt->numClusters ) {
				continue;
			}

			ae = &SVS_CACHED_ENTITIES[ svs.nextCachedSnapshotEntities
									   % SV_NUM_CACHED_SNAPSHOT_ENTITIES ];
			ae->s = ent->s;
			ae->svFlags = ent->r.svFlags;
			ae->singleClient = ent->r.singleClient;
			VectorCopy( ent->r.absmin, ae->absmin );
			VectorCopy( ent->r.absmax, ae->absmax );
			MSG_WriteDeltaArchivedEntity( &msg,
				(const byte *)&SV_SvEntityNum( ent->s.number )->baseline,
				(const byte *)ae, qtrue );

			svs.nextCachedSnapshotEntities++;
			if ( svs.nextCachedSnapshotEntities >= SV_SNAPSHOT_COUNTER_LIMIT ) {
				Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotEntities wrapped" );
			}
			cache->num_entities++;
		}

		svs.nextCachedSnapshotFrames++;
		if ( svs.nextCachedSnapshotFrames >= SV_SNAPSHOT_COUNTER_LIMIT ) {
			Com_Error( ERR_FATAL, "\x15svs.nextCachedSnapshotFrames wrapped" );
		}
	}

	MSG_WriteBits( &msg, ENTITYNUM_NONE, 10 );
	len = msg.cursize;

	archived = &svs.archivedSnapshotFrames[ svs.nextArchivedSnapshotFrames
											% SV_NUM_ARCHIVED_SNAPSHOT_FRAMES ];
	archived->start = svs.nextArchivedSnapshotBuffer;
	archived->len = len;

	offset = svs.nextArchivedSnapshotBuffer % SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES;

	svs.nextArchivedSnapshotBuffer += len;
	if ( svs.nextArchivedSnapshotBuffer >= SV_SNAPSHOT_COUNTER_LIMIT ) {
		Com_Error( ERR_FATAL, "\x15svs.nextArchivedSnapshotBuffer wrapped" );
	}

	room = SV_ARCHIVED_SNAPSHOT_BUFFER_BYTES - offset;
	if ( len <= room ) {
		memcpy( svs.archivedSnapshotBuffer + offset, msg.data, (size_t)len );
	} else {
		memcpy( svs.archivedSnapshotBuffer + offset, msg.data, (size_t)room );
		memcpy( svs.archivedSnapshotBuffer, msg.data + room, (size_t)( len - room ) );
	}

	svs.nextArchivedSnapshotFrames++;
	if ( svs.nextArchivedSnapshotFrames >= SV_SNAPSHOT_COUNTER_LIMIT ) {
		Com_Error( ERR_FATAL, "\x15svs.nextArchivedSnapshotFrames wrapped" );
	}

	/* Retail ends here (0x0045E83D); the self test below is tree-only. */
#ifdef SV_ARCHIVE_SELFTEST
	if ( !sv_archiveSelfTest ) {
		sv_archiveSelfTest = Cvar_Get( "sv_archiveSelfTest", "0", 0 );
		sv_archiveForceTime = Cvar_Get( "sv_archiveForceTime", "0", 0 );
	}
	if ( sv_archiveSelfTest->integer ) {
		SV_ArchiveSelfTest( svs.nextArchivedSnapshotFrames - 1 );
	}
#endif
}

/* ---- SV_SendClientMessages  0x0045E860 ---- */
void SV_SendClientMessages( void ) {
	int i;
	client_t    *c;
	int numclients = 0;

	sv.bpsTotalBytes = 0;
	sv.ubpsTotalBytes = 0;

	for ( i = 0, c = svs.clients ; i < sv_maxclients->integer ; i++, c++ ) {
		if ( !c->state ) {
			continue;
		}
		if ( svs.time < c->nextSnapshotTime ) {
			continue;
		}

		numclients++;

		if ( c->netchan.unsentFragments ) {
			c->nextSnapshotTime = svs.time
								  + SV_RateMsec( c,
												 c->netchan.unsentLength
												 - c->netchan.unsentFragmentStart );
			SV_Netchan_TransmitNextFragment( &c->netchan );
			continue;
		}

		SV_SendClientSnapshot( c );
	}

	if ( sv_showAverageBPS->integer && numclients > 0 ) {
#ifdef COD1_VC7
		float ave = 0, uave = 0;

		for ( i = 0 ; i < MAX_BPS_WINDOW - 1 ; i++ ) {
			sv.bpsWindow[i] = sv.bpsWindow[i + 1];
			ave += sv.bpsWindow[i];

			sv.ubpsWindow[i] = sv.ubpsWindow[i + 1];
			uave += sv.ubpsWindow[i];
		}

		sv.bpsWindow[MAX_BPS_WINDOW - 1] = sv.bpsTotalBytes;
		ave += sv.bpsTotalBytes;

		sv.ubpsWindow[MAX_BPS_WINDOW - 1] = sv.ubpsTotalBytes;
		uave += sv.ubpsTotalBytes;

		if ( sv.bpsTotalBytes >= sv.bpsMaxBytes ) {
			sv.bpsMaxBytes = sv.bpsTotalBytes;
		}
		if ( sv.ubpsTotalBytes >= sv.ubpsMaxBytes ) {
			sv.ubpsMaxBytes = sv.ubpsTotalBytes;
		}

		sv.bpsWindowSteps++;

		if ( sv.bpsWindowSteps >= MAX_BPS_WINDOW ) {
			float comp_ratio;

			sv.bpsWindowSteps = 0;

			ave = ( ave / (float)MAX_BPS_WINDOW );
			uave = ( uave / (float)MAX_BPS_WINDOW );

			comp_ratio = ( 1 - ave / uave ) * 100.f;
			sv.ucompAve += comp_ratio;
			sv.ucompNum++;

			Com_DPrintf( "bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f) acr(%2.2f)\n",
						 ave / (float)numclients, ave, sv.bpsMaxBytes, uave, sv.ubpsMaxBytes,
						 comp_ratio, sv.ucompAve / sv.ucompNum );
		}
#else
		SV_UpdateAverageBPS( numclients );
#endif
	}
}

#ifndef COD1_VC7
/* ---- SV_UpdateAverageBPS  no-address ---- */
/*
 * Modern build only: under VC7 SV_SendClientMessages carries the plain-float
 * RTCW block, which compiles to retail. This emulates that block's x87
 * rounding for modern MSVC.
 *
 * Tail of SV_SendClientMessages 0x0045E860 (0x0045E915..0x0045EBA9). Retail
 * keeps `total` in an x87 register for the whole shuffle and never stores it;
 * ave, uave and comp are also register values passed to Com_DPrintf as
 * qwords. Under the CRT's _PC_53 those are C doubles. Only utotal is a float
 * stack slot, and the eightfold unroll spills it after each group of eight
 * (i = 0..7, 8..15), then after each of i = 16..18 and after the final add;
 * usum is the register-held partial sum between spills.
 */
static void SV_UpdateAverageBPS( int numclients ) {
	int i;
	double total;
	float utotal;
	double usum;
	double ave, uave, comp;

	total = 0.0;
	utotal = 0.0f;
	usum = 0.0;

	for ( i = 0 ; i < MAX_BPS_WINDOW - 1 ; i++ ) {
		sv.bpsWindow[i] = sv.bpsWindow[i + 1];
		total += sv.bpsWindow[i];
		sv.ubpsWindow[i] = sv.ubpsWindow[i + 1];
		usum += sv.ubpsWindow[i];
		if ( ( i & 7 ) == 7 || i >= 16 ) {
			utotal = (float)usum;
			usum = utotal;
		}
	}

	sv.bpsWindow[MAX_BPS_WINDOW - 1] = sv.bpsTotalBytes;
	total += sv.bpsTotalBytes;
	sv.ubpsWindow[MAX_BPS_WINDOW - 1] = sv.ubpsTotalBytes;
	utotal = (float)( (double)utotal + sv.ubpsTotalBytes );

	if ( sv.bpsTotalBytes >= sv.bpsMaxBytes ) {
		sv.bpsMaxBytes = sv.bpsTotalBytes;
	}
	if ( sv.ubpsTotalBytes >= sv.ubpsMaxBytes ) {
		sv.ubpsMaxBytes = sv.ubpsTotalBytes;
	}

	sv.bpsWindowSteps++;
	if ( sv.bpsWindowSteps < MAX_BPS_WINDOW ) {
		return;
	}
	sv.bpsWindowSteps = 0;

	/* flt_5690F0 is the float 0.05f; the products are formed in double. */
	ave = total * ( 1.0f / MAX_BPS_WINDOW );
	uave = (double)utotal * ( 1.0f / MAX_BPS_WINDOW );
	comp = ( 1.0 - ave / uave ) * 100.0;

	sv.ucompNum++;
	/* 0x0045EB69: only the sum is rounded to float, comp itself is not. */
	sv.ucompAve = (float)( sv.ucompAve + comp );

	Com_DPrintf( "bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f) acr(%2.2f)\n",
				 ave / numclients, ave,
				 sv.bpsMaxBytes, uave, sv.ubpsMaxBytes,
				 comp, (double)sv.ucompAve / sv.ucompNum );
}
#endif /* !COD1_VC7 */
