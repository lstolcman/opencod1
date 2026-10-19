/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "cl_records.h"
#include "cl_local.h"

/* cl.snap.valid (0x01432964), the head of the cl.snap record (storage in cl_main_mp.c) */
extern int cl_snap_valid;

/* RTCW cl_parse.c; set from the systeminfo sv_pure, read by Sys_LoadDll */
int cl_connectedToPureServer;   /* 0x01617460 */

const char *svc_strings[256] = {
	"svc_bad",
	"svc_nop",
	"svc_gamestate",
	"svc_configstring",
	"svc_baseline",
	"svc_serverCommand",
	"svc_download",
	"svc_snapshot",
};

int entLastVisible[64];          /* RTCW cl_parse.c entLastVisible[MAX_CLIENTS] */

extern int CL_AddReliableCommand();
extern int CL_InitDownloads();
extern int CL_NextDownload();
extern int CL_WritePacket();
extern void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
                         const vec3_t mins, const vec3_t maxs, clipHandle_t model,
                         int brushmask, qboolean capsule );

extern int Con_Close();
extern char *MSG_ReadBigString( msg_t *msg );
extern int   MSG_ReadShort( msg_t *msg );
extern int   MSG_ReadBits( msg_t *msg, int bits );
extern int   MSG_ReadBit( msg_t *msg );
extern void  CL_ClearState( void );
extern void  FS_Restart( int checksumFeed );
extern cvar_t *fs_gamedirvar;
extern int MSG_ReadBitsCompress( const byte *input, byte *outputBuf, int readsize );
extern qboolean MSG_ReadDeltaClient( msg_t *msg, const byte *from, byte *to,
                                     int number );
extern void MSG_ReadDeltaPlayerstate( msg_t *msg, const byte *from, byte *to );
extern int  MSG_ReadLong( msg_t *msg );
extern int  MSG_ReadByte( msg_t *msg );
extern void MSG_ReadData( msg_t *msg, void *data, int len );
extern qboolean MSG_ReadDeltaStruct( msg_t *msg, const byte *from, byte *to,
                                     unsigned int number, int numFields,
                                     int indexBits, const netField_t *stateFields );
extern char *MSG_ReadString( msg_t *msg );
extern void VectorNormalizeFast( vec3_t v );
extern int j__atol( const char *s );
#ifdef COD1_VC7
#define j__atol atoi	/* retail calls libcmt's atoi (0x00525445, a jmp to _atol) */
#endif

/*
 * Typed views over the client-active blobs.  The byte-array storage stays
 * the storage of record (as do the SNAP_, ES_ and CS_ offset macros); these casts
 * only put a cl_local.h C type over the same memory so the parser can say
 * frame->numEntities instead of *(int *)(frame + SNAP_NUMENTITIES).
 */
#define cl_parseEntities    ( (entityState_t *)cl_parseEntitiesStorage )  /* cl.parseEntities[2048]   (cl+0x867D4) */
#define cl_entityBaselines  ( (entityState_t *)cl_entityBaselinesStorage )  /* cl.entityBaselines[1024] (cl+0x4A7D4) */
#define cl_parseClients     ( (clientState_t *)cl_parseClientsStorage )  /* cl.parseClients[2048]    (cl+0xFE7D4) */
#define cl_snapshotRing     ( (clSnapshot_t  *)cl_snapshots )  /* cl.snapshots[32]         (cl+0x87D4)  */
#define cl_snap             ( *(clSnapshot_t *)&cl_snap_valid )  /* cl.snap                  (0x01432964) */


#define EF_NODRAW           0x100

/* ---- SHOWNET  0x00414EC0 ----  VERIFIED */
void SHOWNET( msg_t *msg, const char *s )
{
	if ( cl_shownet->integer >= 2 ) {
		Com_Printf( "%3i:%s\n", msg->readcount - 1, s );
	}
}

/* ---- isEntVisible  0x00414EF0 ----  VERIFIED */
/*
 * The eye position uses cl.snap.ps.viewHeightCurrent (0x01432A50), .viewangles[YAW]
 * (0x01432A44) and .leanf (0x014329C0).  Retail refreshes these only through the
 * whole-cl.snap copy in CL_ParseSnapshot and the CL_ClearState memset, so they are
 * read straight out of the cl.snap blob (cl_snap_valid) that CL_ParseSnapshot copies into.
 */
qboolean isEntVisible( const entityState_t *ent )
{
	const playerState_t *snapPs = (const playerState_t *)( (const byte *)&cl_snap_valid + SNAP_PS );
	trace_t tr;
	vec3_t  start, end, temp;
	vec3_t  forward, up, right, right2;
	float   view_height;

	start[0] = cgameClientLerpOrigin_x;
	start[1] = cgameClientLerpOrigin_y;
	start[2] = cgameClientLerpOrigin_z;
	start[2] += ( snapPs->viewHeightCurrent - 1 );
	AddLeanToPosition( start, snapPs->viewangles[1], snapPs->leanf, 16, 20 );

	VectorCopy( ent->pos.trBase, end );

	/* vector perpendicular to the view-to-ent line */
	VectorSubtract( end, start, forward );
	VectorNormalizeFast( forward );
	VectorSet( up, 0, 0, 1 );
	CrossProduct( forward, up, right );
	VectorNormalizeFast( right );
	VectorScale( right, 10, right2 );
	VectorScale( right, 18, right );

	if ( ent->animMovetype ) {
		view_height = 16;
	} else {
		view_height = 40;
	}

	/* viewpoint to viewpoint */
	end[2] += view_height;
	CM_BoxTrace( &tr, start, end, NULL, NULL, 0, 1, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	/* viewpoint to top of head */
	end[2] += 16;
	CM_BoxTrace( &tr, start, end, NULL, NULL, 0, 1, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}
	end[2] -= 16;

	/* viewpoint to the ent's origin */
	end[2] -= view_height;
	CM_BoxTrace( &tr, start, end, NULL, NULL, 0, 1, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	/* right knee */
	VectorAdd( end, right, temp );
	temp[2] += 8;
	CM_BoxTrace( &tr, start, temp, NULL, NULL, 0, 1, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	/* right shoulder */
	VectorAdd( end, right2, temp );
	if ( ent->animMovetype ) {
		temp[2] += 28;
	} else {
		temp[2] += 52;
	}
	CM_BoxTrace( &tr, start, temp, NULL, NULL, 0, 1, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	/* left knee */
	VectorScale( right, -1, right );
	VectorScale( right2, -1, right2 );
	VectorAdd( end, right2, temp );
	temp[2] += 2;
	CM_BoxTrace( &tr, start, temp, NULL, NULL, 0, 1, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	/* left shoulder */
	VectorAdd( end, right, temp );
	if ( ent->animMovetype ) {
		temp[2] += 16;
	} else {
		temp[2] += 36;
	}
	CM_BoxTrace( &tr, start, temp, NULL, NULL, 0, 1, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	return qfalse;
}

/* ---- CL_DeltaEntity  0x00415350 ----  VERIFIED */
void CL_DeltaEntity( msg_t *msg, clSnapshot_t *frame, unsigned int newnum,
                     const entityState_t *old, qboolean unchanged )
{
	entityState_t *state;

	state = &cl_parseEntities[ cl_parseEntitiesNum & 0x7FF ];

	if ( unchanged ) {
		*state = *old;
	} else if ( MSG_ReadDeltaStruct( msg, (const byte *)old, (byte *)state, newnum,
	                                 0x3B, 10, (const netField_t *)stateFields ) ) {
		return;
	}

	if ( clc_onlyVisibleClients ) {
		if ( state->number < 64 ) {
			if ( isEntVisible( state ) ) {
				entLastVisible[ state->number ] = frame->serverTime;
				state->eFlags &= ~EF_NODRAW;
			} else {
				if ( entLastVisible[ state->number ] < ( frame->serverTime - 600 ) ) {
					state->eFlags |= EF_NODRAW;
				}
			}
		}
	}

	cl_parseEntitiesNum++;
	frame->numEntities++;
}

/* ---- CL_DeltaClient  0x00415410 ----  VERIFIED */
void CL_DeltaClient( msg_t *msg, clSnapshot_t *frame, int number,
                     const clientState_t *old, qboolean unchanged )
{
	clientState_t *state;

	state = &cl_parseClients[ cl_parseClientsNum & 0x7FF ];

	if ( unchanged ) {
		*state = *old;
	} else if ( MSG_ReadDeltaClient( msg, (const byte *)old, (byte *)state, number ) ) {
		return;
	}

	cl_parseClientsNum++;
	frame->numClients++;
}

/* ---- CL_ParsePacketEntities  0x00415460 ----  VERIFIED */
void CL_ParsePacketEntities( msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe )
{
	int            newnum;
	entityState_t *oldstate;
	int            oldindex;
	int            oldnum;

	newframe->parseEntitiesNum = cl_parseEntitiesNum;
	newframe->numEntities      = 0;

	oldindex = 0;
	oldstate = NULL;
	if ( !oldframe ) {
		oldnum = 99999;
	} else if ( oldindex >= oldframe->numEntities ) {
		oldnum = 99999;
	} else {
		oldstate = &cl_parseEntities[ ( oldframe->parseEntitiesNum + oldindex ) & 0x7FF ];
		oldnum = oldstate->number;
	}

	while ( 1 ) {
		newnum = MSG_ReadBits( msg, 10 );

		if ( newnum == 0x3FF ) {
			break;
		}

		if ( msg->readcount > msg->cursize ) {
			/* Split literal: C's \x escape is greedy, so "\x15CL_..." lexes as one character. */
			Com_Error( ERR_DROP, "\x15" "CL_ParsePacketEntities: end of message" );
		}

		while ( oldnum < newnum ) {
			if ( cl_shownet->integer == 3 ) {
				Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
			}
			CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

			oldindex++;
			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl_parseEntities[ ( oldframe->parseEntitiesNum + oldindex ) & 0x7FF ];
				oldnum = oldstate->number;
			}
		}

		if ( oldnum == newnum ) {
			if ( cl_shownet->integer == 3 ) {
				Com_Printf( "%3i:  delta: %i\n", msg->readcount, newnum );
			}
			CL_DeltaEntity( msg, newframe, newnum, oldstate, qfalse );

			oldindex++;
			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl_parseEntities[ ( oldframe->parseEntitiesNum + oldindex ) & 0x7FF ];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( cl_shownet->integer == 3 ) {
			Com_Printf( "%3i:  baseline: %i\n", msg->readcount, newnum );
		}
		CL_DeltaEntity( msg, newframe, newnum, &cl_entityBaselines[ newnum ], qfalse );
	}

	while ( oldnum != 99999 ) {
		if ( cl_shownet->integer == 3 ) {
			Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
		}
		CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

		oldindex++;
		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl_parseEntities[ ( oldframe->parseEntitiesNum + oldindex ) & 0x7FF ];
			oldnum = oldstate->number;
		}
	}

	if ( cl_shownuments->integer ) {
		Com_Printf( "Entities in packet: %i\n", newframe->numEntities );
	}
}

/* ---- CL_ParsePacketClients  0x004157F0 ----  VERIFIED */
void CL_ParsePacketClients( msg_t *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe )
{
	int            newnum;
	clientState_t *oldstate;
	int            oldindex;
	int            oldnum;
	clientState_t  dummy;

	newframe->parseClientsNum = cl_parseClientsNum;
	newframe->numClients      = 0;

	oldindex = 0;
	oldstate = NULL;
	if ( !oldframe ) {
		oldnum = 99999;
	} else if ( oldindex >= oldframe->numClients ) {
		oldnum = 99999;
	} else {
		oldstate = &cl_parseClients[ ( oldframe->parseClientsNum + oldindex ) & 0x7FF ];
		oldnum = oldstate->clientNum;
	}

	while ( 1 ) {
		if ( !MSG_ReadBit( msg ) ) {
			break;
		}

		newnum = MSG_ReadBits( msg, 6 );

		if ( msg->readcount > msg->cursize ) {
			Com_Error( ERR_DROP, "\x15" "CL_ParsePacketClients: end of message" );
		}

		while ( oldnum < newnum ) {
			if ( cl_shownet->integer == 3 ) {
				Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
			}
			CL_DeltaClient( msg, newframe, oldnum, oldstate, qtrue );

			oldindex++;
			if ( oldindex >= oldframe->numClients ) {
				oldnum = 99999;
			} else {
				oldstate = &cl_parseClients[ ( oldframe->parseClientsNum + oldindex ) & 0x7FF ];
				oldnum = oldstate->clientNum;
			}
		}

		if ( oldnum == newnum ) {
			if ( cl_shownet->integer == 3 ) {
				Com_Printf( "%3i:  delta: %i\n", msg->readcount, newnum );
			}
			CL_DeltaClient( msg, newframe, newnum, oldstate, qfalse );

			oldindex++;
			if ( oldindex >= oldframe->numClients ) {
				oldnum = 99999;
			} else {
				oldstate = &cl_parseClients[ ( oldframe->parseClientsNum + oldindex ) & 0x7FF ];
				oldnum = oldstate->clientNum;
			}
			continue;
		}

		if ( cl_shownet->integer == 3 ) {
			Com_Printf( "%3i:  baseline: %i\n", msg->readcount, newnum );
		}
		memset( &dummy, 0, sizeof( dummy ) );
		CL_DeltaClient( msg, newframe, newnum, &dummy, qfalse );
	}

	while ( oldnum != 99999 ) {
		if ( cl_shownet->integer == 3 ) {
			Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
		}
		CL_DeltaClient( msg, newframe, oldnum, oldstate, qtrue );

		oldindex++;
		if ( oldindex >= oldframe->numClients ) {
			oldnum = 99999;
		} else {
			oldstate = &cl_parseClients[ ( oldframe->parseClientsNum + oldindex ) & 0x7FF ];
			oldnum = oldstate->clientNum;
		}
	}

	if ( cl_shownuments->integer ) {
		Com_Printf( "Clients in packet: %i\n", newframe->numClients );
	}
}

/* cl.outPackets[32], 12 bytes each, based at 0x0143AFB4; serverTime is +4 and realtime +8.
 * (i) is a packet number already masked to 0..31, as in retail. */
#define OUTPACKET_SERVERTIME( i )  cl_outPackets_p_serverTime[ (i) * 3 ]
#define OUTPACKET_REALTIME( i )    cl_outPackets_p_realtime[ (i) * 3 ]

/* ---- CL_ParseSnapshot  0x00415BB0 ----  VERIFIED */
void CL_ParseSnapshot( msg_t *msg )
{
	clSnapshot_t  newSnap;
	clSnapshot_t *old;
	int   deltaNum;
	int   oldMessageNum;
	int   packetNum;
	int   i;

	memset( &newSnap, 0, sizeof( newSnap ) );

	newSnap.serverCommandNum = clc_serverCommandSequence;
	newSnap.serverTime       = MSG_ReadLong( msg );
	newSnap.messageNum       = clc_serverMessageSequence;

	deltaNum = MSG_ReadByte( msg );
	if ( !deltaNum ) {
		newSnap.deltaNum = -1;
	} else {
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}

	newSnap.snapFlags = MSG_ReadByte( msg );

	if ( newSnap.deltaNum <= 0 ) {
		newSnap.valid = qtrue;
		old = NULL;
		clc_demowaiting = qfalse;
	} else {
		old = &cl_snapshotRing[ newSnap.deltaNum & 31 ];

		if ( !old->valid ) {
			Com_Printf( "Delta from invalid frame (not supposed to happen!).\n" );
		} else if ( old->messageNum != newSnap.deltaNum ) {
			Com_DPrintf( "Delta frame too old.\n" );
		} else if ( cl_parseEntitiesNum - old->parseEntitiesNum > 0x780 ) {
			Com_DPrintf( "Delta parseEntitiesNum too old.\n" );
		} else if ( cl_parseClientsNum - old->parseClientsNum > 0x780 ) {
			Com_DPrintf( "Delta parseClientsNum too old.\n" );
		} else {
			newSnap.valid = qtrue;
		}
	}

	SHOWNET( msg, "playerstate" );
	if ( old ) {
		MSG_ReadDeltaPlayerstate( msg, old->ps, newSnap.ps );
	} else {
		MSG_ReadDeltaPlayerstate( msg, NULL, newSnap.ps );
	}

	SHOWNET( msg, "packet entities" );
	CL_ParsePacketEntities( msg, old, &newSnap );

	SHOWNET( msg, "packet clients" );
	CL_ParsePacketClients( msg, old, &newSnap );

	if ( !newSnap.valid ) {
		return;
	}

	oldMessageNum = cl_snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= 32 ) {
		oldMessageNum = newSnap.messageNum - 31;
	}
	for ( ; oldMessageNum < newSnap.messageNum; oldMessageNum++ ) {
		cl_snapshotRing[ oldMessageNum & 31 ].valid = qfalse;
	}

	cl_snap = newSnap;
	cl_snap.ping = 999;

	for ( i = 0; i < 32; i++ ) {
		packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & 31;
		if ( *(const int *)cl_snap.ps >= OUTPACKET_SERVERTIME( packetNum ) ) {
			cl_snap.ping = cls_realtime - OUTPACKET_REALTIME( packetNum );
			break;
		}
	}

	/* retail has no CL_PublishSnap copy: its `cl.snap = newSnap` (0x415DE6, rep movsd
	 * 0x2100 bytes) refreshes every cl.snap field at once; the tree refreshes each split
	 * mirror global that some reader uses (CL_ClearState zeroes the same set) */
	{
		const clSnapshot_t  *snap = (const clSnapshot_t *)&cl_snap_valid;
		const playerState_t *ps   = (const playerState_t *)snap->ps;

		*(int *)cl_snap_snapFlags  = snap->snapFlags;
		cl_snap_serverTime         = snap->serverTime;
		*(int *)cl_snap_messageNum = snap->messageNum;
		cl_snap_deltaNum           = snap->deltaNum;
		cl_snap_ping               = snap->ping;
		cl_snap_ps_commandTime     = *(const int *)snap->ps;
		cl_snap_ps_pm_type         = ps->pm_type;
		cl_snap_ps_pm_flags        = ps->pm_flags;
		cl_snap_ps_deltaPitch      = ps->delta_angles[0];
		cl_snap_ps_eFlags          = ps->eFlags;
		cl_snap_ps_clientNum       = ps->clientNum;
		cl_snap_numClients         = snap->numClients;
		cl_snap_parseClientsNum    = snap->parseClientsNum;
	}
	cl_snapshotRing[ cl_snap.messageNum & 31 ] = cl_snap;

	if ( cl_shownet->integer == 3 ) {
		Com_Printf( "   snapshot:%i  delta:%i  ping:%i\n",
		            cl_snap.messageNum, cl_snap.deltaNum, cl_snap.ping );
	}

	cl_newSnapshots = qtrue;
}

/* ---- CL_SystemInfoChanged  0x00415EB0 ----  VERIFIED */
void CL_SystemInfoChanged( void )
{
	const char *systemInfo;
	const char *s, *t;
	char        key[8192];
	char        value[8192];

	systemInfo = &cl_gameState_stringData[ cl_gameState_stringOffsets[1] ];

	cl_serverId = j__atol( Info_ValueForKey( systemInfo, "sv_serverid" ) );

	memset( entLastVisible, 0, sizeof( entLastVisible ) );

	if ( clc_demoplaying ) {
		return;
	}

	s = Info_ValueForKey( systemInfo, "sv_cheats" );
	if ( !j__atol( s ) ) {
		Cvar_SetCheatState();
	}

	s = Info_ValueForKey( systemInfo, "sv_paks" );
	t = Info_ValueForKey( systemInfo, "sv_pakNames" );
	FS_PureServerSetLoadedPaks( s, t );

	s = Info_ValueForKey( systemInfo, "sv_referencedPaks" );
	t = Info_ValueForKey( systemInfo, "sv_referencedPakNames" );
	FS_PureServerSetReferencedPaks( s, t );

	if ( !com_sv_running->integer ) {
		s = systemInfo;
		while ( s ) {
			Info_NextPair( &s, key, value );
			if ( !key[0] ) {
				break;
			}
			Cvar_Set2( key, value, qtrue );
		}
	}

	cl_connectedToPureServer = Cvar_VariableValue( "sv_pure" );
}

/* ---- CL_ParseGamestate  0x00416050 ----  VERIFIED */
void CL_ParseGamestate( msg_t *msg )
{
	int   cmd;
	int   i;
	int   newnum;
	int   len;
	char *s;
	entityState_t nullstate;

	Con_Close();

	clc_connectPacketCount = 0;
	CL_ClearState();

	clc_serverCommandSequence = MSG_ReadLong( msg );

	cl_gameState_dataCount = 1;

	while ( 1 ) {
		cmd = MSG_ReadByte( msg );

		if ( cmd == 8 ) {
			break;
		}

		if ( cmd == 3 ) {
			i = MSG_ReadShort( msg );
			if ( i < 0 || i >= 0x800 ) {
				Com_Error( ERR_DROP, "\x15" "configstring > MAX_CONFIGSTRINGS" );
			}

			s   = MSG_ReadBigString( msg );
			len = strlen( s );

			if ( len + 1 + cl_gameState_dataCount > 0x3E80 ) {
				Com_Error( ERR_DROP, "\x15" "MAX_GAMESTATE_CHARS exceeded" );
			}

			cl_gameState_stringOffsets[ i ] = cl_gameState_dataCount;
			memcpy( &cl_gameState_stringData[ cl_gameState_dataCount ],
			        s, len + 1 );
			cl_gameState_dataCount += len + 1;

		} else if ( cmd == 4 ) {
			newnum = MSG_ReadBits( msg, 10 );
			if ( newnum < 0 || newnum >= 0x400 ) {
				Com_Error( ERR_DROP, "\x15" "Baseline number out of range: %i",
				           newnum );
			}
			memset( &nullstate, 0, sizeof( nullstate ) );
			MSG_ReadDeltaStruct( msg, (const byte *)&nullstate,
			                     (byte *)&cl_entityBaselines[ newnum ],
			                     (unsigned int)newnum, 0x3B, 10,
			                     (const netField_t *)stateFields );

		} else {
			Com_Error( ERR_DROP, "\x15" "CL_ParseGamestate: bad command byte" );
		}
	}

	clc_clientNum    = MSG_ReadLong( msg );
	clc_checksumFeed = MSG_ReadLong( msg );

	CL_SystemInfoChanged();

	if ( !com_sv_running->integer
	  && ( fs_gamedirvar->modified || clc_checksumFeed != fs_checksumFeed ) ) {
		FS_Restart( clc_checksumFeed );
	}

	CL_InitDownloads();

	Cvar_Set2( "cl_paused", "0", qtrue );
}

/* ---- CL_ParseDownload  0x004162A0 ----  [HIGH] */
void __cdecl CL_ParseDownload( msg_t *msg )
{
	int          block;
	int          size;
	fileHandle_t f;
	byte         data[16384];

	block = MSG_ReadShort( msg );
	if ( !block ) {
		/* block zero is special: it carries the total file size */
		clc_downloadSize = MSG_ReadLong( msg );
		Cvar_SetValue( "cl_downloadSize", (float)clc_downloadSize );
		if ( clc_downloadSize < 0 ) {
			Com_Error( ERR_DROP, MSG_ReadString( msg ) );
			return;
		}
	}

	size = MSG_ReadShort( msg );
	if ( size > 0 ) {
		MSG_ReadData( msg, data, size );
	}

	if ( *(int *)cls_downloadBlock != block ) {
		Com_DPrintf( "CL_ParseDownload: Expected block %d, got %d\n",
		             *(int *)cls_downloadBlock, block );
		if ( block > *(int *)cls_downloadBlock ) {
			Com_DPrintf( "CL_ParseDownload: Sending retransmitt request to get the missed block\n" );
			CL_AddReliableCommand( va( "retransdl %d", *(int *)cls_downloadBlock ) );
		}
		return;
	}

	f = clc_download;
	if ( !clc_download ) {
		if ( !cls_downloadTempName[0] ) {
			Com_Printf( "Server sending download, but no download was requested\n" );
			CL_AddReliableCommand( "stopdl" );
			return;
		}
		f = FS_SV_FOpenFileWrite( (const char *)cls_downloadTempName );
		clc_download = f;
		if ( !f ) {
			Com_Printf( "Could not create %s\n", (char *)cls_downloadTempName );
			CL_AddReliableCommand( "stopdl" );
			CL_NextDownload();
			return;
		}
	}

	if ( size ) {
		FS_Write( data, size, f );
	}

	CL_AddReliableCommand( va( "nextdl %d", *(int *)cls_downloadBlock ) );
	++*(int *)cls_downloadBlock;
	cls_downloadCount += size;
	Cvar_SetValue( "cl_downloadCount", (float)cls_downloadCount );

	if ( !size ) {
		/* a zero-length block means EOF */
		if ( clc_download ) {
			FS_FCloseFile( clc_download );
			clc_download = 0;
			FS_SV_Rename( (const char *)cls_downloadTempName,
			              (const char *)cls_downloadName );
		}
		cls_downloadName[0]     = 0;
		cls_downloadTempName[0] = 0;
		Cvar_Set2( "cl_downloadName", "", qtrue );
		CL_WritePacket();
		CL_WritePacket();
		CL_NextDownload();
	}
}

/* ---- CL_ParseCommandString  0x00416590 ----  [CONFIRMED] */
char *__cdecl CL_ParseCommandString( msg_t *msg )
{
	int   seq;
	char *s;
	char *cmd;

	seq = MSG_ReadLong( msg );
	s   = MSG_ReadString( msg );

	if ( clc_serverCommandSequence < seq ) {
		clc_serverCommandSequence = seq;
		cmd = &clc_serverCommands[ 1024 * ( seq & 0x3F ) ];
		s   = strncpy( cmd, s, 0x3FF );
		cmd[1023] = 0;
	}
	return s;
}

/* ---- CL_ParseServerMessage  0x004165F0 ----  VERIFIED */
void CL_ParseServerMessage( msg_t *source )
{
	msg_t msg;
	byte  to[0x4000];
	int   cmd;

	if ( cl_shownet->integer == 1 ) {
		Com_Printf( "%i ", source->cursize );
	} else if ( cl_shownet->integer >= 2 ) {
		Com_Printf( "------------------\n" );
	}

	MSG_Init( &msg, to, sizeof( to ) );
	msg.cursize = MSG_ReadBitsCompress( source->data + source->readcount, to,
	                                    source->cursize - source->readcount );

	while ( 1 ) {
		if ( msg.readcount > msg.cursize ) {
			Com_Error( ERR_DROP,
			           "\x15" "CL_ParseServerMessage: read past end of server message" );
		}

		cmd = MSG_ReadByte( &msg );

		if ( cmd == 8 ) {
			SHOWNET( &msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 ) {
			if ( !( (const char **)&svc_strings )[ cmd ] ) {
				Com_Printf( "%3i:BAD CMD %i\n", msg.readcount - 1, cmd );
			} else {
				SHOWNET( &msg, ( (const char **)&svc_strings )[ cmd ] );
			}
		}

		switch ( cmd ) {
		default:
			Com_Error( ERR_DROP,
			           "\x15" "CL_ParseServerMessage: Illegible server message %d\n",
			           cmd );
			break;
		case 1:
			break;
		case 5:
			CL_ParseCommandString( &msg );
			break;
		case 2:
			CL_ParseGamestate( &msg );
			break;
		case 7:
			CL_ParseSnapshot( &msg );
			break;
		case 6:
			CL_ParseDownload( &msg );
			break;
		}
	}
}
