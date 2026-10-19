/*
 * @fidelity: verified
 */

#include "server.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct DObj_s
{
	void            *tree;                  /* +0x00 XAnimTree * */
	byte            *evalStorage;           /* +0x04 */
	int skelCacheKey;                       /* +0x08 vs com_skelTimeStamp */
	byte            *partRemapTable;        /* +0x0C xanim/dobj.cpp DObj name */
	unsigned short unknownState10;          /* +0x10 xanim/dobj.cpp's name, kept in step with it */
	unsigned short scrNotifyId;             /* +0x12 xanim/dobj.cpp DObj name */
	unsigned short tracePartRemapHandle;    /* +0x14 */
	byte childCount;                        /* +0x16 */
	byte partCount;                         /* +0x17 */
	unsigned int childRefs[8];              /* +0x18 */
	short childModelIndices[8];             /* +0x38 */
	byte childParentPartIndices[8];         /* +0x48 */
	byte childPartBaseIndices[8];           /* +0x50 */
} DObj_t;
SV_ASSERT_SIZE( DObj_t, 88 );

#define SVF_NOCLIENT                0x00000001
#define SVF_VISIBILITY_BYPASS       0x00000018

extern int s_hunkTotal, s_hunkHighPermanent;
/* XAnim server-time staging, retail 0x00A7A618 / 0x00A9CC5C. Plain values, one dword each, so no sizing hazard. */
extern int xanim_currentTree;
extern int xanim_currentTime;
#define HUNK_TOTAL          ( *(int *) hunk_totalSize )
#define HUNK_HIGH_PERM      ( *(int *) hunk_highUsed )
/* retail reads the low counters directly (server.h); total/highPerm are still com_memory_core.c statics */
#define HUNK_LOW_MARK       SV_HUNK_GAME_BASE
#define HUNK_LOW_PERM       SV_HUNK_LOW_PERM
#define HUNK_LOW_TEMP       SV_HUNK_LOW_TEMP

#include "../universal/com_sndalias.h"

extern int scrAnimPub_trees;
#define SV_XANIM_TREE( idx ) \
	( (int)( &scrAnimPub_trees )[ 128 * xanim_activePoolSlot + ( idx ) ] )

#define SV_XANIM_POOL_SERVER    1

extern int CM_PointLeafnum( const vec3_t p );
extern int CM_LeafCluster( int leafnum );
extern int CM_LeafArea( int leafnum );
extern byte *CM_ClusterPVS( int cluster );
extern qboolean CM_AreasConnected( int area1, int area2 );
extern void CM_ChangeAreaPortalState( int area1, int area2, qboolean open );
extern int CM_NumInlineModels( void );
extern const char *CM_EntityString( void );
extern float *CM_ClipHandleToModel( clipHandle_t handle );
extern int CM_AreaEntities( const vec3_t mins, const vec3_t maxs,
							int *entityList, int maxcount, int contentmask );
extern void CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						 const vec3_t mins, const vec3_t maxs,
						 clipHandle_t model, int brushmask, qboolean capsule );
extern int CM_BoxSightTrace( int oldHitNum, const vec3_t start,
							 const vec3_t end, const vec3_t mins,
							 const vec3_t maxs, clipHandle_t model,
							 int brushmask, qboolean capsule );
extern void CM_TransformedBoxTrace();

extern int Com_RealTime( void *qtime );
extern void Sys_SnapVector( float *v );
extern void Sys_LoadingKeepAlive( void );
extern void MatrixMultiply( float in1[3][3], float in2[3][3], float out[3][3] );
extern void PerpendicularVector( vec3_t dst, const vec3_t src );
extern void *Hunk_AllocLowAlignInternal( int size, int align );
extern int FS_Rename( const char *from, const char *to );
extern void *Scr_NearHook( void *callbacks );
extern void XModelClearData( void *high, void *low );
/* Retail takes (lowEnd, highEnd) and returns nothing; body at 0x0042CA60 in universal/com_files.c. */
extern void FS_ClearDataForFiles( void *lowEnd, void *highEnd );
extern void Hunk_ClearToLowMark( void );    /* 0x00432050, universal/com_memory.c */
extern int Hunk_SetLowMark( void );         /* 0x00432040, universal/com_memory.c */

extern int *Com_FindSoundAlias( const char *name, int source );
extern int Com_PickSoundAlias( const char *name, int source );
extern int Com_SoundAliasIndex( void *alias, int source );
extern void Com_ServerDObjCreate( void *models, unsigned short modelCount,
								  void *tree, int handle,
								  unsigned short scrNotifyId );
extern void Com_SafeServerDObjFree( int handle, unsigned int releaseTree );
extern void *Com_XAnimCreateTree( void *anims );
extern void *Com_XAnimCreateSmallTree( void *anims );
extern void Com_XAnimFreeSmallTree( void *tree );
extern void *Com_GetWeaponInfoMemory( int size, int *pPrevOwner, int owner );
extern void Com_FreeWeaponInfoMemory( int owner, int iSource );
extern int XModelExists( const char *name );
extern void *XModelPrecache( const char *name, int loadSurfaces,
							 void *(*alloc)( unsigned int ), int allocMesh );
extern void *Hunk_AllocXModelPrecache( unsigned int size );

extern void DObjDumpInfo( void *dobj );                     /* 0x00480F60 */
extern int DObjCalcAnim( int dobj, void *partBits );        /* 0x00487F00 */
extern void DObjCalcSkel( void *partBits, int dobj );        /* 0x00481780 */
extern int DObjFindPartIndex( int dobj, unsigned short nameHandle );   /* 0x00481230 */
extern unsigned short SL_FindLowercaseString( const char *s );   /* 0x0046FA50; returns a 16-bit handle (retail tests only AX) */
extern int DObjGetHierarchyBits( int boneIndex, int dobj, void *partBits );  /* 0x00481440 */
extern int DObjMarkRotTransIndex( void *dobj, const unsigned char *partBits, int boneIndex );   /* 0x004822B0 */
extern int DObjMarkControlRotTransIndex( void *dobj, const unsigned char *partBits, int boneIndex );   /* 0x004822F0 */
extern int DObjUpdateServerInfo( void *dobj, float serverTime, int stopOnNotetrack );
extern int XAnimInitServerTime_m( int unused );
extern int XAnimDisplay( void *tree, int nodeIndex, int depth );
extern int DObjBuildPartCollisionTable( void *out, int dobj );

extern int XAnimClearAnimNode_m( void *tree, int node, float blend );
extern int XAnimClearAnim_m( int node, void *tree, float blend );
extern int XAnimClearAnimChildSubtrees_m( void *tree, int node, float blend );
extern int XAnimGetRelDelta_m( void *tree, int node, float *dirDelta, void *posDelta, int deltaRootNode );
extern int XAnimGetAbsDelta_m( void *tree, int node, float *dirDelta, void *posDelta );
extern int XAnimGetRelDeltaForTime_m( int sourceTree, int node, void *posDelta, float *dirDelta, float time0, float time1 );
extern void XAnimGetAbsDeltaForTime_m( const void *sourceTree, int node, float *posDelta, float *dirDelta, float time );
extern int XAnimSetAnimKnob_m( void *tree, int node, float weight, int blendTime, float rate,
					   short notifyName, int notifyType, int animFlags );
extern int XAnimSetAnimKnobAll_m( int node, void *tree, int stopNode, float weight, int blendTime, float rate,
					   short notifyName, int notifyType, int animFlags );
extern int XAnimSetAnimLimited_m( void *tree, int node, float weight, float blendTime, float rate,
					   short notifyName, int notifyType, int animFlags );
extern int XAnimClearTree( void *tree );
extern void XAnimSetTime_m( void *tree, int node, float time );
extern int XAnimSetAnimInternalLimited_m( int notifyType, short notifyName, int tree, int node, float weight, int blendTime,
					   float rate, int animFlags );
extern int XAnimSetAnimKnobInternal_m( void *tree, int node, float weight, float blendTime, float rate,
					   short notifyName, int notifyType, int animFlags );
extern int XAnimSetAnimRate_m( int tree, int node, float rate );
extern int XAnimIsLooped_m(const void *source, int node);
extern float       XAnimGetTime( void *tree, int nodeIndex );      /* 0x00487CB0, FLOAT */
extern float       XAnimGetWeight( void *tree, int nodeIndex );    /* 0x00487CE0, FLOAT */
extern int         XAnimHasTime( void *tree, int nodeIndex );      /* 0x00487D10 */
extern int         XAnimIsLeafNode_m( const void *tree, unsigned int index );       /* 0x004891C0 */
extern int         XAnimNotetrackExists_m( const void *tree, unsigned int index,
                                           unsigned short name );                 /* 0x00489680 */
extern const char *SV_GetConfigstringConstPtr_m( int index );    /* 0x00457E70 */
extern const char *XAnimGetAnimName( const void *sourceTree, int nodeIndex );  /* 0x00487D80 */
extern int cand_XAnimRestoreTree( int tree );
extern int cand_XAnimSaveTree( void *tree );
extern void XAnimCopyTree( const void *from, void *to );

/* Other server units. Retail 0x004556E0 SV_AddTestClient returns the new bot's client number, or -1 ('or eax, -1') when no slot is free or the rescan misses; case G_ADD_TEST_CLIENT tail-calls it (0x00456E4B), so that is the syscall's result. */
extern int SV_AddTestClient( void );
extern void SV_FreeClientScriptPers( void );
extern qboolean SV_GetArchivedClientInfo( int *frameTime, void *clientInfo,
										  int clientNum );
extern clipHandle_t SV_ClipHandleForEntity( gentity_t *gent );
extern void SV_LinkEntity( gentity_t *gent );
extern void SV_UnlinkEntity( gentity_t *gent );
extern int SV_PointContents( const vec3_t p, int passEntityNum, int contentmask );

extern void SV_Trace( trace_t *results, const vec3_t start, const vec3_t mins,
					  const vec3_t maxs, const vec3_t end, int passEntityNum,
					  int contentmask, qboolean capsule, qboolean useDObj,
					  const byte *dobjTracePartState, qboolean locational );
extern void SV_SightTrace( int *hitOut, const vec3_t start,
						   const vec3_t mins, const vec3_t maxs, const vec3_t end,
						   int passEntityNum, int passOwnerNum,
						   int contentmask, qboolean capsule );
extern int SV_SightTraceToEntity( const vec3_t start, const vec3_t mins,
								  const vec3_t maxs, const vec3_t end,
								  int entityNum, int contentmask,
								  qboolean capsule );

/* ---- SV_NumForGentity  0x00455860 ---- */
int SV_NumForGentity( gentity_t *gent ) {
	return (int)( ( (byte *)gent - (byte *)sv.gentities ) / sv.gentitySize );
}

/* ---- SV_GentityNum  0x00455870 ---- */
gentity_t *SV_GentityNum( int num ) {
	return (gentity_t *)( (byte *)sv.gentities + sv.gentitySize * num );
}

/* ---- SV_GameClientNum  0x00455890 ---- */
playerState_t *SV_GameClientNum( int clientNum ) {
	return (playerState_t *)( (byte *)sv.gameClients
							  + sv.gameClientSize * clientNum );
}

/* ---- SV_SvEntityForGentity  0x004558B0 ---- */
svEntity_t *SV_SvEntityForGentity( gentity_t *gent ) {
	if ( !gent || gent->s.number < 0 || gent->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "\x15SV_SvEntityForGentity: bad gEnt" );
	}
	return &sv.svEntities[gent->s.number];
}

/* ---- SV_GEntityForSvEntity  0x004558E0 ---- */
gentity_t *SV_GEntityForSvEntity( svEntity_t *svEnt ) {
	return SV_GentityNum( (int)( svEnt - sv.svEntities ) );
}

/* ---- SV_GameSendServerCommand  0x00455910 ---- */
void SV_GameSendServerCommand( int clientNum, int reliable, const char *command ) {
	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, reliable, "%s", command );
		return;
	}

	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		return;
	}

	SV_SendServerCommand( &svs.clients[clientNum], reliable, "%s", command );
}

/* ---- SV_GameDropClient  0x00455960 ---- */
void SV_GameDropClient( int clientNum, const char *reason ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		return;
	}
	SV_DropClient( &svs.clients[clientNum], reason );
}

/* ---- SV_SetBrushModel  0x00455990 ---- */
void SV_SetBrushModel( gentity_t *gent ) {
	const float *bounds;
	vec3_t mins, maxs;
	int index;

	index = gent->s.index;
	if ( index < 0 || index >= CM_NumInlineModels() ) {
		Com_Error( ERR_DROP, "\x15" "CM_InlineModel: bad number" );
	}

	bounds = CM_ClipHandleToModel( index );

	mins[0] = bounds[0];
	mins[1] = bounds[1];
	mins[2] = bounds[2];
	maxs[0] = bounds[3];
	maxs[1] = bounds[4];
	maxs[2] = bounds[5];

	gent->r.mins[0] = mins[0];
	gent->r.mins[1] = mins[1];
	gent->r.mins[2] = mins[2];
	gent->r.maxs[0] = maxs[0];
	gent->r.maxs[1] = maxs[1];
	gent->r.maxs[2] = maxs[2];

	gent->r.bmodel = 1;
	gent->r.contents = -1;

	SV_LinkEntity( gent );
}

/* ---- SV_inPVS  0x00455A40 ---- */
qboolean SV_inPVS( const vec3_t p1, const vec3_t p2 ) {
	int leafnum;
	int cluster;
	int area1;
	int area2;
	byte    *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	area2 = CM_LeafArea( leafnum );

	if ( mask && !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) {
		return qfalse;
	}
	if ( !CM_AreasConnected( area1, area2 ) ) {
		return qfalse;
	}
	return qtrue;
}

/* ---- SV_inSnapshot  0x00455AF0 ---- */
qboolean SV_inSnapshot( const vec3_t origin, int entityNum ) {
	gentity_t   *gent;
	svEntity_t     *svEnt;
	int svFlags;
	int leafnum;
	int cluster;
	int area;
	int numClusters;
	int lastCluster;
	int i;
	int testCluster;
	byte        *mask;

	gent = SV_GentityNum( entityNum );

	if ( !gent->r.linked ) {
		return qfalse;
	}

	svFlags = gent->r.svFlags;
	if ( svFlags & SVF_NOCLIENT ) {
		return qfalse;
	}
	if ( svFlags & SVF_VISIBILITY_BYPASS ) {
		return qtrue;
	}

	svEnt = SV_SvEntityForGentity( gent );

	numClusters = svEnt->numClusters;
	if ( !numClusters ) {
		return qtrue;
	}

	leafnum = CM_PointLeafnum( origin );
	area = CM_LeafArea( leafnum );
	cluster = CM_LeafCluster( leafnum );
	mask = CM_ClusterPVS( cluster );

	if ( !CM_AreasConnected( area, svEnt->areanum )
		 && !CM_AreasConnected( area, svEnt->areanum2 ) ) {
		return qfalse;
	}

	testCluster = 0;
	for ( i = 0 ; i < numClusters ; i++ ) {
		testCluster = svEnt->clusternums[i];
		if ( mask[testCluster >> 3] & ( 1 << ( testCluster & 7 ) ) ) {
			break;
		}
	}

	if ( i == numClusters ) {
		lastCluster = svEnt->lastCluster;
		if ( lastCluster ) {
			for ( ; testCluster <= lastCluster ; testCluster++ ) {
				if ( mask[testCluster >> 3] & ( 1 << ( testCluster & 7 ) ) ) {
					break;
				}
			}
			if ( testCluster == lastCluster ) {
				return qfalse;
			}
		} else {
			return qfalse;
		}
	}

	return qtrue;
}

/* ---- SV_inPVSIgnorePortals  0x00455C20 ---- */
qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	int leafnum;
	int cluster;
	byte    *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );

	if ( mask && !( mask[cluster >> 3] & ( 1 << ( cluster & 7 ) ) ) ) {
		return qfalse;
	}
	return qtrue;
}

/* ---- SV_AdjustAreaPortalState  0x00455CB0 ---- */
void SV_AdjustAreaPortalState( gentity_t *gent, qboolean open ) {
	svEntity_t *svEnt;

	svEnt = SV_SvEntityForGentity( gent );

	if ( svEnt->areanum2 == -1 ) {
		return;
	}

	CM_ChangeAreaPortalState( svEnt->areanum,
							  svEnt->areanum2, open );
}

/* ---- SV_EntityContact  0x00455D10 ---- */
qboolean SV_EntityContact( gentity_t *gent, const vec3_t mins, const vec3_t maxs, int capsule ) {
	clipHandle_t ch;
	trace_t trace;

	ch = SV_ClipHandleForEntity( gent );

	trace.fraction = 1.0f;
	CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, mins, maxs,
							ch, -1, gent->r.currentOrigin,
							gent->r.currentAngles, capsule );

	return trace.startsolid;
}

/* ---- SV_GetServerinfo  0x00455D70 ---- */
void SV_GetServerinfo( char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "\x15SV_GetServerinfo: bufferSize == %i", bufferSize );
	}
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

/* ---- SV_Hunk_AllocInternal  0x00455DB0 ---- */
/* Unlabeled in the retail exe; the Mac 1.3 build names it.  SV_GameSystemCalls
 * tail-calls it for G_HUNK_ALLOC (it is not inlined there). */
void *SV_Hunk_AllocInternal( int size ) {
	if ( sv.state != SS_LOADING ) {
		Com_Error( ERR_DROP, "\x15trap_Hunk_Alloc can only be called in "
							 "G_InitGame and the first few frames of G_RunFrame\n" );
	}
	return Hunk_AllocAlignInternal( size, 32 );
}

/* ---- SV_Hunk_AllocLowInternal  0x00455DE0 ---- */
/* Unlabeled in the retail exe (Mac 1.3 name); tail-called for G_HUNK_ALLOC_LOW. */
void *SV_Hunk_AllocLowInternal( int size ) {
	if ( sv.state != SS_LOADING ) {
		Com_Error( ERR_DROP, "\x15trap_Hunk_AllocLow can only be called in "
							 "G_InitGame and the first few frames of G_RunFrame\n" );
	}
	return Hunk_AllocLowAlignInternal( size, 32 );
}

/* ---- SV_Hunk_AllocAlignInternal  0x00455E10 ---- */
void *SV_Hunk_AllocAlignInternal( int size, int align ) {
	if ( sv.state != SS_LOADING ) {
		Com_Error( ERR_DROP, "\x15trap_Hunk_AllocAlign can only be called in "
							 "G_InitGame and the first few frames of G_RunFrame\n" );
	}
	return Hunk_AllocAlignInternal( size, align );
}

/* ---- SV_Hunk_AllocLowAlignInternal  0x00455E40 ---- */
void *SV_Hunk_AllocLowAlignInternal( int size, int align ) {
	if ( sv.state != SS_LOADING ) {
		Com_Error( ERR_DROP, "\x15trap_Hunk_AllocLowAlign can only be called in "
							 "G_InitGame and the first few frames of G_RunFrame\n" );
	}
	return Hunk_AllocLowAlignInternal( size, align );
}

/* ---- SV_Hunk_AllocateTempMemoryInternal  0x00455E70 ---- */
void *SV_Hunk_AllocateTempMemoryInternal( int size ) {
	return Hunk_AllocateTempMemoryInternal( size );
}

/* ---- SV_Hunk_FreeTempMemoryInternal  0x00455E80 ---- */
void SV_Hunk_FreeTempMemoryInternal( void *buf ) {
	int     *header;

	if ( !s_hunkData ) {
		free( buf );
		return;
	}

	header = (int *)buf - 4;
	if ( header[0] != 0x89537892 ) {
		Com_Error( ERR_FATAL, "\x15Hunk_FreeTempMemory: bad magic" );
	}
	header[0] = 0x89537893;
	HUNK_LOW_TEMP -= header[1];
}

/* ---- SV_LocateGameData  0x00455EC0 ---- */
void SV_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity,
						playerState_t *clients, int sizeofGameClient ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity;
	sv.num_entities = numGEntities;
	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}

/* ---- SV_GetUsercmd  0x00455EF0 ---- */
void SV_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "\x15SV_GetUsercmd: bad clientNum:%i", clientNum );
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

/* ---- FloatAsInt_1  0x00455F40 ---- */
int FloatAsInt_1( float f ) {
	int temp;

	*(float *)&temp = f;

	return temp;
}

/* ---- SV_XModelGet  0x00455F50 ---- */
void *SV_XModelGet( const char *name ) {
	/* Retail 0x00455F64 reads name[6] only after the prefix matched. */
	if ( _strnicmp( name, "xmodel", 6 )
		 || ( name[6] != '/' && name[6] != '\\' ) ) {
		Com_Error( ERR_DROP, "\x15" "bad model name '%s'", name );
	}

	return (void *)XModelPrecache( name + 7, 0, Hunk_AllocXModelPrecache, 0 );
}

/* ---- SV_DObjDumpInfo  0x00455FA0 ---- */
void SV_DObjDumpInfo( int gentNum ) {
	DObj_t  *obj;

	if ( !com_developer->integer ) {
		return;
	}

	obj = (DObj_t *)Com_GetServerDObj( gentNum );
	if ( !obj ) {
		Com_Printf( "no model.\n" );
	} else {
		DObjDumpInfo( obj );
	}
}

/* xanim/dobj.cpp 0x00482180: refresh the eval-storage key, qtrue when the
   skeleton for this timestamp already exists (LTCG inlines it here). */
extern int DObjRefreshEvalStorageKey( void *dobj, int skelCacheKey );

/* ---- SV_DObjCreateSkelForBone  0x00455FF0 ---- */
qboolean SV_DObjCreateSkelForBone( int entityNum, int boneIndex ) {
	DObj_t  *obj;
	byte    *storage;
	int i;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );

	if ( DObjRefreshEvalStorageKey( obj, com_skelTimeStamp ) ) {
		storage = obj->evalStorage;
		return ( ((signed char *)storage)[32 + ( boneIndex >> 3 )]
			 & ( 1 << ( boneIndex & 7 ) ) ) != 0;
	}

	i = 96 * obj->partCount + 48;
	storage = (byte *)Hunk_AllocateTempMemoryInternal( i );
	obj->evalStorage = storage;
	for ( i = 0; i < 4; ++i ) {
		((int *)storage)[i] = 0;
		((int *)storage)[i + 4] = 0;
		((int *)storage)[i + 8] = 0;
	}

	return qfalse;
}

/* xanim/dobj.cpp's skeleton bit test (retail 0x00481740); the tree's copy is
   cl_cgame_mp.c's CL_DObjSkelAreBonesUpToDate.  Retail tail-calls it here. */
extern qboolean CL_DObjSkelAreBonesUpToDate( int dobj, const int *partBits );

/* ---- SV_DObjIsSkelUpToDate  0x00456090 ---- */
/* The same body as CL_DObjCreateSkelForBones, on the server DObj (no NULL test). */
static qboolean SV_DObjIsSkelUpToDate( int entityNum, const int *partBits ) {
	DObj_t  *obj;
	int     *p;
	int i;
	qboolean exists;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );

	/* inlined skeleton-exists test: a current key with NULL evalStorage
	   goes straight to the allocation without the reset */
	do {
		if ( obj->skelCacheKey == com_skelTimeStamp ) {
			exists = obj->evalStorage != NULL;
			break;
		}
		obj->skelCacheKey = com_skelTimeStamp;
		obj->unknownState10 = 0;
		obj->evalStorage = NULL;
		exists = qfalse;
	} while ( 0 );

	if ( exists ) {
		return CL_DObjSkelAreBonesUpToDate( (int)obj, partBits );
	}

	i = 96 * obj->partCount + 48;
	obj->evalStorage = (byte *)Hunk_AllocateTempMemoryInternal( i );

	/* three parallel four-word masks at +0x00, +0x10 and +0x20 */
	p = (int *)( obj->evalStorage + 32 );
	for ( i = 4; i; i-- ) {
		*( p - 8 ) = 0;
		*( p - 4 ) = 0;
		*p++ = 0;
	}

	return qfalse;
}

/* ---- SV_DObjUpdateServerTime  0x00456120 ---- */
int SV_DObjUpdateServerTime( int entityNum, int stopOnNotetrack, float serverTime ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	if ( !obj ) {
		return 0;
	}
	return DObjUpdateServerInfo( obj, serverTime, stopOnNotetrack );
}

/* ---- SV_DObjInitServerTime  0x00456150 ---- */
/* serverTime is a float (stack-passed; retail tail-calls through its slot); xanim_currentTime holds float bits. */
void SV_DObjInitServerTime( int entityNum, float serverTime ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	if ( !obj || !obj->tree ) {
		return;
	}

	*(float *)&xanim_currentTime = serverTime;
	xanim_currentTree = (int)obj->tree;
	XAnimInitServerTime_m( 0 );
}

/* ---- SV_DObjGetHierarchyBits_m  0x00456190 ---- */
int SV_DObjGetHierarchyBits_m( int entityNum, int boneIndex, int *partBits ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	return DObjGetHierarchyBits( boneIndex, (int)obj, partBits );
}

/* ---- SV_DObjCalcAnim  0x004561D0 ---- */
int SV_DObjCalcAnim( int entityNum, void *partBits ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	return DObjCalcAnim( (int)obj, partBits );
}

/* ---- SV_DObjCalcSkel  0x00456200 ---- */
void SV_DObjCalcSkel( int entityNum, void *partBits ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	DObjCalcSkel( partBits, (int)obj );
}

/* ---- SV_DObjNumParts  0x00456240 ---- */
static int SV_DObjNumParts( int entityNum ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	return obj->partCount;
}

/* ---- SV_DObjGetPartIndex  0x00456270 ---- */
static int SV_DObjGetPartIndex( int entityNum, const char *partName ) {
	DObj_t  *obj;
	unsigned short handle;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	if ( !obj ) {
		return -1;
	}

	handle = SL_FindLowercaseString( partName );
	if ( handle ) {
		return DObjFindPartIndex( (int)obj, handle );
	}
	return -1;
}

/* ---- SV_DObjGetSkelBase  0x004562C0 ---- */
static void *SV_DObjGetSkelBase( int entityNum ) {
	DObj_t  *obj = (DObj_t *)Com_GetServerDObj( entityNum );

	return obj->evalStorage + 64 * obj->childPartBaseIndices[0] + 48;
}

/* ---- SV_DObjGetSkelEnd  0x00456350 ---- */
static void *SV_DObjGetSkelEnd( int entityNum ) {
	DObj_t  *obj = (DObj_t *)Com_GetServerDObj( entityNum );

	return obj->evalStorage + 64 * obj->partCount + 48;
}

/* ---- SV_DObjDisplayAnim  0x00456300 ---- */
void SV_DObjDisplayAnim( int entityNum ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	if ( !obj ) {
		return;
	}

	if ( !obj->tree ) {
		Com_Printf( "NO TREE\n" );
		return;
	}
	XAnimDisplay( obj->tree, 0, 0 );
	Com_Printf( "\n" );
}

/* ---- SV_DObjSetRotTransIndex  0x00456390 ---- */
qboolean SV_DObjSetRotTransIndex( int entityNum, int boneIndex, const int *partBits ) {
	return DObjMarkRotTransIndex( Com_GetServerDObj( entityNum ),
								  (const unsigned char *)partBits, boneIndex );
}

/* ---- SV_DObjSetControlRotTransIndex  0x004563F0 ---- */
qboolean SV_DObjSetControlRotTransIndex( int entityNum, int boneIndex,
										 const int *partBits ) {
	return DObjMarkControlRotTransIndex( Com_GetServerDObj( entityNum ),
										 (const unsigned char *)partBits, boneIndex );
}

/* ---- SV_DObjGetTree  0x00456450 ----
 * Unlabeled in the retail exe; SV_GameSystemCalls tail-calls it for
 * G_DOBJ_GET_TREE. */
static int SV_DObjGetTree( int entityNum ) {
	DObj_t  *obj;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	return (int)obj->tree;
}

/*
 * ---- MatrixTransformVector43  0x00456470 ----
 *
 * universal/com_math.c's exe MatrixTransformVector43 (four-float matrix rows)
 * is the code at 0x00456470; SV_DObjDebugDrawBounds_m calls it and LTCG
 * inlines it there.
 */

/* 0x005759D8: the 12 edges of a part box, each as two corners of
   (x, y, z) picks, where 0 takes the mins value of that axis and 1 the maxs. */
static const int sv_dobjBoundsEdges[72] = {
	0,0,0,  1,0,0,
	0,0,0,  0,1,0,
	1,1,0,  1,0,0,
	1,1,0,  0,1,0,
	0,0,1,  1,0,1,
	0,0,1,  0,1,1,
	1,1,1,  1,0,1,
	1,1,1,  0,1,1,
	0,0,0,  0,0,1,
	1,0,0,  1,0,1,
	0,1,0,  0,1,1,
	1,1,0,  1,1,1
};

/*
 * ---- SV_DObjDebugDrawBounds_m  0x004564C0 ----
 *
 * Trap 131.  Draws the collision box of every part of the entity's DObj as
 * 12 debug lines per part, through CL_AddDebugLine with fromServer 1.  Each
 * corner goes through the part's skeleton matrix (the MatrixTransformVector43
 * formula above, inlined) and then the entity's currentAngles frame, and
 * currentOrigin is added as a separate float step.  The sums are written in
 * retail's x87 order: the second corner's third row adds x * m[2] first
 * (0x00456714).  Retail does not test the DObj for NULL.  Retail sizes the
 * part table with alloca( 4 * partCount ); partCount is a byte, so 256 slots
 * always cover it.  Both MatrixTransformVector43 calls spell the part matrix
 * as &mats[16 * p] (no cached pointer local): that spelling sets VC7's x87
 * term order of the inlined rows to retail's (all but the second corner's
 * row 0, where retail starts with y * m[4]).
 */
extern void *__cdecl _alloca( unsigned int size );

extern void AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
extern void MatrixTransformVector( const vec3_t in, const vec3_t matrix[3], vec3_t out );
extern void MatrixTransformVector43( const vec3_t in, const float m[], vec3_t out );

void SV_DObjDebugDrawBounds_m( int entityNum ) {
	DObj_t      *obj;
	gentity_t   *ent;
	void        **table;
	const float *mats;
	const float *bounds;
	const int   *edge;
	vec4_t      color;
	vec3_t      axis[3];
	vec3_t      corner, pt;
	vec3_t      start, end;
	int         n, p, e;

	obj = (DObj_t *)Com_GetServerDObj( entityNum );
	n = obj->partCount;
	table = (void **)_alloca( n * sizeof( *table ) );
	DObjBuildPartCollisionTable( table, (int)obj );
	mats = (const float *)( obj->evalStorage
							+ 64 * obj->childPartBaseIndices[0] + 48 );

	color[0] = 1.0f;
	color[1] = 1.0f;
	color[2] = 1.0f;
	color[3] = 0.0f;

	ent = SV_GentityNum( entityNum );
	AnglesToAxis( ent->r.currentAngles, axis );

	for ( p = 0 ; p < n ; p++, table++ ) {
		bounds = (const float *)*table;
		edge = sv_dobjBoundsEdges;

		for ( e = 0 ; e < 12 ; e++ ) {
			corner[0] = bounds[edge[0] * 3 + 0];
			corner[1] = bounds[edge[1] * 3 + 1];
			corner[2] = bounds[edge[2] * 3 + 2];
			MatrixTransformVector43( corner, &mats[16 * p], pt );
			MatrixTransformVector( pt, axis, start );
			VectorAdd( start, ent->r.currentOrigin, start );
			edge += 3;

			corner[0] = bounds[edge[0] * 3 + 0];
			corner[1] = bounds[edge[1] * 3 + 1];
			corner[2] = bounds[edge[2] * 3 + 2];
			MatrixTransformVector43( corner, &mats[16 * p], pt );
			MatrixTransformVector( pt, axis, end );
			VectorAdd( end, ent->r.currentOrigin, end );

			edge += 3;

			CL_AddDebugLine( start, end, color, 0, 0, 1 );
		}
	}
}

/* ---- SV_ResetEntityParsePoint_m  0x004567F0 ---- */
void SV_ResetEntityParsePoint_m( void ) {
	sv.entityParsePoint = CM_EntityString();
}

    /* Trap numbers are the real ones; retail remaps them through the table at 0x0045789C. */

typedef enum
{
	G_PRINT                     = 0,
	G_ERROR                     = 1,
	G_ERROR_NOPREFIX            = 2,
	G_MILLISECONDS              = 3,
	G_CVAR_REGISTER             = 4,
	G_CVAR_UPDATE               = 5,
	G_CVAR_SET                  = 6,
	G_CVAR_VARIABLE_INTEGER_VALUE = 7,
	G_CVAR_VARIABLE_VALUE       = 8,
	G_CVAR_VARIABLE_STRING_BUFFER = 9,
	G_ARGC                      = 10,
	G_ARGV                      = 11,
	G_HUNK_ALLOC                = 12,
	G_HUNK_ALLOC_LOW            = 13,
	G_HUNK_ALLOC_ALIGN          = 14,
	G_HUNK_ALLOC_LOW_ALIGN      = 15,
	G_HUNK_ALLOC_TEMP           = 16,
	G_HUNK_FREE_TEMP            = 17,
	G_FS_FOPEN_FILE             = 18,
	G_FS_READ                   = 19,
	G_FS_WRITE                  = 20,
	G_FS_RENAME                 = 21,
	G_FS_FCLOSE_FILE            = 22,
	G_SEND_CONSOLE_COMMAND      = 23,
	G_LOCATE_GAME_DATA          = 24,
	G_DROP_CLIENT               = 25,
	G_SEND_SERVER_COMMAND       = 26,
	G_SET_CONFIGSTRING          = 27,
	G_GET_CONFIGSTRING          = 28,
	G_GET_CONFIGSTRING_CONST    = 29,
	G_GET_USERINFO              = 30,
	G_SET_USERINFO              = 31,
	G_GET_SERVERINFO            = 32,
	G_SET_BRUSH_MODEL           = 33,
	G_TRACE                     = 34,
	G_TRACE_CAPSULE             = 35,
	G_SIGHT_TRACE               = 36,
	G_SIGHT_TRACE_CAPSULE       = 37,
	G_SIGHT_TRACE_TO_ENTITY     = 38,
	G_CM_BOX_TRACE              = 39,
	G_CM_BOX_TRACE_2            = 40,
	G_CM_BOX_SIGHT_TRACE        = 41,
	G_CM_BOX_SIGHT_TRACE_2      = 42,
	G_LOCATIONAL_TRACE          = 43,
	G_POINT_CONTENTS            = 44,
	G_IN_PVS                    = 45,
	G_IN_PVS_IGNORE_PORTALS     = 46,
	G_IN_SNAPSHOT               = 47,
	G_ADJUST_AREA_PORTAL_STATE  = 48,
	G_AREAS_CONNECTED           = 49,
	G_LINKENTITY                = 50,
	G_UNLINKENTITY              = 51,
	G_ENTITIES_IN_BOX           = 52,
	G_ENTITY_CONTACT            = 53,
	G_GET_USERCMD               = 54,
	G_GET_ENTITY_TOKEN          = 55,
	G_FS_GETFILELIST            = 56,
	G_REAL_TIME                 = 57,
	G_SNAPVECTOR                = 58,
	G_ENTITY_CONTACT_CAPSULE    = 59,
	G_FIND_SOUND_ALIAS          = 60,
	G_PICK_SOUND_ALIAS          = 61,
	G_SOUND_ALIAS_INDEX         = 62,
	G_SURFACE_TYPE_FROM_NAME    = 63,
	G_SURFACE_TYPE_TO_NAME      = 64,
	G_ADD_TEST_CLIENT           = 65,
	G_GET_ARCHIVED_CLIENT_INFO  = 66,
	G_ADD_DEBUG_STRING          = 67,
	G_ADD_DEBUG_LINE            = 68,
	G_ENABLE_ARCHIVED_SNAPSHOT  = 69,
	G_Z_MALLOC                  = 70,
	G_Z_FREE                    = 71,
	G_XANIM_CREATE_TREE         = 72,
	G_XANIM_CREATE_SMALL_TREE   = 73,
	G_XANIM_FREE_SMALL_TREE     = 74,
	G_XMODEL_EXISTS             = 75,
	G_XMODEL_GET                = 76,
	G_DOBJ_CREATE               = 77,
	G_DOBJ_EXISTS               = 78,
	G_DOBJ_FREE                 = 79,
	G_DEREF_DWORD               = 80,
	G_XANIM_CLEAR_ANIM          = 82,
	G_XANIM_CLEAR_ANIM_NODE     = 83,
	G_XANIM_CLEAR_ANIM_CHILDREN = 84,
	G_XANIM_SET_ANIM_KNOB       = 85,
	G_XANIM_SET_ANIM_KNOB_ALL   = 86,
	G_XANIM_SET_ANIM_RATE       = 87,
	G_XANIM_SET_TIME            = 88,
	G_XANIM_SET_ANIM_LIMITED    = 89,
	G_XANIM_CLEAR_TREE          = 90,
	G_XANIM_IS_PRIMITIVE        = 91,
	G_XANIM_IS_LEAF             = 92,
	G_XANIM_LENGTH_MSEC         = 93,
	G_XANIM_LENGTH_SEC          = 94,
	G_XANIM_SET_ANIM_KNOB_INT   = 95,
	G_XANIM_SET_ANIM_INT_LIMITED = 96,
	G_XANIM_GET_ABS_DELTA       = 97,
	G_XANIM_GET_REL_DELTA       = 98,
	G_XANIM_GET_REL_DELTA_TIME  = 99,
	G_XANIM_GET_ABS_DELTA_TIME  = 100,
	G_XANIM_IS_LOOPED           = 101,
	G_XANIM_HAS_NOTETRACK       = 102,
	G_XANIM_GET_TIME            = 103,
	G_XANIM_GET_WEIGHT          = 104,
	G_DOBJ_DUMP_INFO            = 105,
	G_DOBJ_CREATE_SKEL_FOR_BONE = 106,
	G_DOBJ_SKEL_UP_TO_DATE      = 107,
	G_DOBJ_UPDATE_SERVER_TIME   = 108,
	G_DOBJ_INIT_SERVER_TIME     = 109,
	G_DOBJ_GET_HIERARCHY_BITS   = 110,
	G_DOBJ_CALC_ANIM            = 111,
	G_DOBJ_CALC_SKEL            = 112,
	G_XANIM_RESTORE_TREE        = 113,
	G_XANIM_SAVE_TREE           = 114,
	G_XANIM_COPY_TREE           = 115,
	G_DOBJ_NUM_PARTS            = 116,
	G_DOBJ_GET_PART_INDEX       = 117,
	G_DOBJ_GET_SKEL_BASE        = 118,
	G_DOBJ_DISPLAY_ANIM         = 119,
	G_XANIM_HAS_TIME            = 120,
	G_XANIM_CHILD_COUNT         = 121,
	G_XANIM_CHILD_BASE          = 122,
	G_XANIM_ROOT_PART_COUNT     = 123,
	G_XANIM_ROOT_PART_LIST      = 124,
	G_DOBJ_GET_SKEL_END         = 125,
	G_DOBJ_SET_ROT_TRANS_INDEX  = 126,
	G_DOBJ_SET_CONTROL_ROT_TRANS_INDEX = 127,
	G_XANIM_GET_ANIM_NAME       = 128,
	G_DOBJ_GET_TREE             = 129,
	G_XANIM_TREE_FIELD          = 130,
	G_DOBJ_DEBUG_DRAW_BOUNDS    = 131,
	G_GET_WEAPON_INFO_MEMORY    = 132,
	G_FREE_WEAPON_INFO_MEMORY   = 133,
	G_FREE_CLIENT_SCRIPT_PERS   = 134,
	G_RESET_ENTITY_PARSE_POINT  = 135,
	G_MEMSET                    = 200,
	G_MEMCPY                    = 201,
	G_STRNCPY                   = 202,
	G_SIN                       = 203,
	G_COS                       = 204,
	G_ATAN2                     = 205,
	G_SQRT                      = 206,
	G_MATRIXMULTIPLY            = 207,
	G_ANGLEVECTORS              = 208,
	G_PERPENDICULARVECTOR       = 209,
	G_FLOOR                     = 210,
	G_CEIL                      = 211
} gameImport_t;

/* ---- SV_GameSystemCalls  0x00456800 ---- */
int SV_GameSystemCalls( int *args ) {
	switch ( args[0] ) {

	case G_PRINT:
		Com_Printf( "%s", (const char *)args[1] );
		return 0;

	case G_ERROR:
		Com_Error( ERR_DROP, "\x15%s", (const char *)args[1] );
		/* retail falls through (Com_Error does not return) */

	case G_ERROR_NOPREFIX:
		Com_Error( ERR_DROP, "%s", (const char *)args[1] );
		/* retail falls through */

	case G_MILLISECONDS:
		return Sys_Milliseconds();

	case G_CVAR_REGISTER:
		Cvar_Register( (vmCvar_t *)args[1], (const char *)args[2],
					   (const char *)args[3], args[4] );
		return 0;

	case G_CVAR_UPDATE:
		Cvar_Update( (vmCvar_t *)args[1] );
		return 0;

	case G_CVAR_SET:
		Cvar_Set2( (const char *)args[1], (const char *)args[2], qtrue );
		return 0;

	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue( (const char *)args[1] );

	case G_CVAR_VARIABLE_VALUE:
		*(float *)args[2] = Cvar_VariableValue( (const char *)args[1] );
		return 0;

	case G_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer( (const char *)args[1], (char *)args[2],
								   args[3] );
		return 0;

	case G_ARGC:
		return Cmd_Argc();

	case G_ARGV:
		Cmd_ArgvBuffer( args[1], (char *)args[2], args[3] );
		return 0;

	case G_HUNK_ALLOC:
		return (int)SV_Hunk_AllocInternal( args[1] );

	case G_HUNK_ALLOC_LOW:
		return (int)SV_Hunk_AllocLowInternal( args[1] );

	case G_HUNK_ALLOC_ALIGN:
		return (int)SV_Hunk_AllocAlignInternal( args[1], args[2] );

	case G_HUNK_ALLOC_LOW_ALIGN:
		return (int)SV_Hunk_AllocLowAlignInternal( args[1], args[2] );

	case G_HUNK_ALLOC_TEMP:
		return (int)SV_Hunk_AllocateTempMemoryInternal( args[1] );

	case G_HUNK_FREE_TEMP:
		SV_Hunk_FreeTempMemoryInternal( (void *)args[1] );
		return 0;

	case G_FS_FOPEN_FILE:
		return FS_FOpenFileByMode( (const char *)args[1],
								   (fileHandle_t *)args[2], (fsMode_t)args[3] );

	case G_FS_READ:
		FS_Read( (void *)args[1], args[2], args[3] );
		return 0;

	case G_FS_WRITE:
		return FS_Write( (const void *)args[1], args[2], args[3] );

	case G_FS_RENAME:
		FS_Rename( (const char *)args[1], (const char *)args[2] );
		return 0;

	case G_FS_FCLOSE_FILE:
		FS_FCloseFile( args[1] );
		return 0;

	case G_FS_GETFILELIST:
		return FS_GetFileList( (const char *)args[1], (const char *)args[2],
							   (char *)args[3], args[4] );

	case G_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText( args[1], (const char *)args[2] );
		return 0;

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData( (gentity_t *)args[1], args[2], args[3],
						   (playerState_t *)args[4], args[5] );
		return 0;

	case G_DROP_CLIENT:
		SV_GameDropClient( args[1], (const char *)args[2] );
		return 0;

	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand( args[1], args[2], (const char *)args[3] );
		return 0;

	case G_LINKENTITY:
		SV_LinkEntity( (gentity_t *)args[1] );
		return 0;

	case G_UNLINKENTITY:
		SV_UnlinkEntity( (gentity_t *)args[1] );
		return 0;

	case G_ENTITIES_IN_BOX:
		return CM_AreaEntities( (const float *)args[1], (const float *)args[2],
								(int *)args[3], args[4], args[5] );

	case G_ENTITY_CONTACT:
		return SV_EntityContact( (gentity_t *)args[3], (const float *)args[1],
								 (const float *)args[2], 0 );

	case G_ENTITY_CONTACT_CAPSULE:
		return SV_EntityContact( (gentity_t *)args[3], (const float *)args[1],
								 (const float *)args[2], 1 );

	case G_TRACE:
		SV_Trace( (trace_t *)args[1], (const float *)args[2],
				  (const float *)args[3], (const float *)args[4],
				  (const float *)args[5], args[6], args[7], 0, 0, NULL, 0 );
		return 0;

	case G_TRACE_CAPSULE:
		SV_Trace( (trace_t *)args[1], (const float *)args[2],
				  (const float *)args[3], (const float *)args[4],
				  (const float *)args[5], args[6], args[7], 1, 0, NULL, 0 );
		return 0;

	case G_SIGHT_TRACE:
		SV_SightTrace( (int *)args[1], (const float *)args[2],
					   (const float *)args[3], (const float *)args[4],
					   (const float *)args[5], args[6], args[7], args[8], 0 );
		return 0;

	case G_SIGHT_TRACE_CAPSULE:
		SV_SightTrace( (int *)args[1], (const float *)args[2],
					   (const float *)args[3], (const float *)args[4],
					   (const float *)args[5], args[6], args[7], args[8], 1 );
		return 0;

	case G_SIGHT_TRACE_TO_ENTITY:
		return SV_SightTraceToEntity( (const float *)args[1],
									  (const float *)args[2],
									  (const float *)args[3],
									  (const float *)args[4],
									  args[5], args[6], 1 );

	case G_CM_BOX_TRACE:
		CM_BoxTrace( (trace_t *)args[1], (const float *)args[2],
					 (const float *)args[3], (const float *)args[4],
					 (const float *)args[5], args[6], args[7], 0 );
		return 0;

	case G_CM_BOX_TRACE_2:
		CM_BoxTrace( (trace_t *)args[1], (const float *)args[2],
					 (const float *)args[3], (const float *)args[4],
					 (const float *)args[5], args[6], args[7], 1 );
		return 0;

	/* Same shape: oldHitNum is a literal 0 and the capsule flag lands in eax (`xor eax,eax` at 0x00456C14 vs `mov eax,1` at 0x00456C3B). */
	case G_CM_BOX_SIGHT_TRACE:
		return CM_BoxSightTrace( 0, (const float *)args[1],
								 (const float *)args[2],
								 (const float *)args[3],
								 (const float *)args[4], args[5], args[6], 0 );

	case G_CM_BOX_SIGHT_TRACE_2:
		return CM_BoxSightTrace( 0, (const float *)args[1],
								 (const float *)args[2],
								 (const float *)args[3],
								 (const float *)args[4], args[5], args[6], 1 );

	case G_LOCATIONAL_TRACE:
		SV_Trace( (trace_t *)args[1], (const float *)args[2],
				  NULL, NULL, (const float *)args[3], args[4], args[5],
				  0, 1, (const byte *)args[6], 1 );
		return 0;

	case G_POINT_CONTENTS:
		return SV_PointContents( (const float *)args[1], args[2], args[3] );

	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel( (gentity_t *)args[1] );
		return 0;

	case G_IN_PVS:
		return SV_inPVS( (const float *)args[1], (const float *)args[2] );

	case G_IN_PVS_IGNORE_PORTALS:
		return SV_inPVSIgnorePortals( (const float *)args[1],
									  (const float *)args[2] );

	case G_IN_SNAPSHOT:
		return SV_inSnapshot( (const float *)args[1], args[2] );

	case G_SET_CONFIGSTRING:
		SV_SetConfigstring( args[1], (const char *)args[2] );
		return 0;

	case G_GET_CONFIGSTRING:
		SV_GetConfigstring( args[1], (char *)args[2], args[3] );
		return 0;

	case G_GET_CONFIGSTRING_CONST:
		return (int)SV_GetConfigstringConstPtr_m( args[1] );

	case G_SET_USERINFO:
		SV_SetUserinfo( args[1], (const char *)args[2] );
		return 0;

	case G_GET_USERINFO:
		SV_GetUserinfo( args[1], (char *)args[2], args[3] );
		return 0;

	case G_GET_SERVERINFO:
		SV_GetServerinfo( (char *)args[1], args[2] );
		return 0;

	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState( (gentity_t *)args[1], args[2] );
		return 0;

	case G_AREAS_CONNECTED:
		return CM_AreasConnected( args[1], args[2] );

	case G_GET_USERCMD:
		SV_GetUsercmd( args[1], (usercmd_t *)args[2] );
		return 0;

	case G_GET_ENTITY_TOKEN:
	{
		char        *token;
		int size;
		char        *buffer;

		/* retail 0x00456D96: Com_Parse first, then size and buffer are read */
		token = Com_Parse( (char **)&sv.entityParsePoint );
		size = args[2];
		buffer = (char *)args[1];
		strncpy( buffer, token, size - 1 );
		buffer[size - 1] = 0;

		if ( !sv.entityParsePoint && !token[0] ) {
			return 0;
		}
		return 1;
	}

	case G_REAL_TIME:
		return Com_RealTime( (void *)args[1] );

	case G_SNAPVECTOR:
		Sys_SnapVector( (float *)args[1] );
		return 0;

	case G_FIND_SOUND_ALIAS:
	{
		int *alias = Com_FindSoundAlias( (const char *)args[1], 2 );

		return alias ? *alias : 0;
	}

	case G_PICK_SOUND_ALIAS:
		return Com_PickSoundAlias( (const char *)args[1], 2 );

	case G_SOUND_ALIAS_INDEX:
		return Com_SoundAliasIndex( (void *)args[1], 2 );

	case G_SURFACE_TYPE_FROM_NAME:
		return Com_SurfaceTypeFromName( (const char *)args[1] );

	case G_SURFACE_TYPE_TO_NAME:
		return (int)Com_SurfaceTypeToName( args[1] );

	case G_ADD_TEST_CLIENT:
		return SV_AddTestClient();

	case G_GET_ARCHIVED_CLIENT_INFO:
		return SV_GetArchivedClientInfo( (int *)args[2], (void *)args[3],
										 args[1] );

	case G_MEMSET:
		memset( (void *)args[1], args[2], args[3] );
		return 0;

	case G_MEMCPY:
		memcpy( (void *)args[1], (const void *)args[2], args[3] );
		return 0;

	case G_STRNCPY:
		return (int)strncpy( (char *)args[1], (const char *)args[2], args[3] );

	case G_SIN:
		return FloatAsInt_1( (float)sin( *(const float *)&args[1] ) );

	case G_COS:
		return FloatAsInt_1( (float)cos( *(const float *)&args[1] ) );

	case G_ATAN2:
		return FloatAsInt_1( (float)atan2( *(const float *)&args[1],
											 *(const float *)&args[2] ) );

	case G_SQRT:
		return FloatAsInt_1( (float)sqrt( *(const float *)&args[1] ) );

	case G_MATRIXMULTIPLY:
		MatrixMultiply( (float (*)[3])args[1], (float (*)[3])args[2],
						(float (*)[3])args[3] );
		return 0;

	case G_ANGLEVECTORS:
		AngleVectors( (const float *)args[1], (float *)args[2],
					  (float *)args[3], (float *)args[4] );
		return 0;

	case G_PERPENDICULARVECTOR:
		PerpendicularVector( (float *)args[1], (const float *)args[2] );
		return 0;

	case G_FLOOR:
		return FloatAsInt_1( (float)floor( *(const float *)&args[1] ) );

	case G_CEIL:
		return FloatAsInt_1( (float)ceil( *(const float *)&args[1] ) );

	case G_ADD_DEBUG_STRING:
		CL_AddDebugString( (const float *)args[1], (const float *)args[2],
						   *(const float *)&args[3], (const char *)args[4], 1 );
		return 0;

	case G_ADD_DEBUG_LINE:
		CL_AddDebugLine( (const float *)args[1], (const float *)args[2],
						 (const float *)args[3], args[4], args[5], 1 );
		return 0;

	case G_ENABLE_ARCHIVED_SNAPSHOT:
		SV_EnableArchivedSnapshot( args[1] );
		return 0;

	case G_Z_MALLOC:
		return (int)Z_MallocInternal( args[1] );

	case G_Z_FREE:
		free( (void *)args[1] );
		return 0;

	/* XAnim tree creation ---------------- ONE argument each, not two -- see the prototypes at the top of the file. Retail loads a single register at 0x00457001 / 0x0045700E / 0x0045701B and pushes nothing. */
	case G_XANIM_CREATE_TREE:
		return (int)Com_XAnimCreateTree( (void *)args[1] );

	case G_XANIM_CREATE_SMALL_TREE:
		return (int)Com_XAnimCreateSmallTree( (void *)args[1] );

	case G_XANIM_FREE_SMALL_TREE:
		Com_XAnimFreeSmallTree( (void *)args[1] );
		return 0;

	case G_XMODEL_EXISTS:
		return XModelExists( (const char *)args[1] );

	case G_XMODEL_GET:
		return (int)SV_XModelGet( (const char *)args[1] );

	case G_DOBJ_CREATE:
		Com_ServerDObjCreate( (void *)args[1], (unsigned short)args[2],
							  (void *)args[3], args[4],
							  (unsigned short)args[5] );
		return 0;

	case G_DOBJ_EXISTS:
		return Com_GetServerDObj( args[1] ) != NULL;

	case G_DOBJ_FREE:
		Com_SafeServerDObjFree( args[1], args[2] );
		return 0;

	case G_DEREF_DWORD:
		return *(int *)args[1];

	case G_XANIM_CLEAR_ANIM:
		XAnimClearAnim_m( args[2], (void *)args[1], *(const float *)&args[3] );
		return 0;

	case G_XANIM_CLEAR_ANIM_NODE:
		XAnimClearAnimNode_m( (void *)args[1], args[2], *(const float *)&args[3] );
		return 0;

	case G_XANIM_CLEAR_ANIM_CHILDREN:
		XAnimClearAnimChildSubtrees_m( (void *)args[1], args[2], *(const float *)&args[3] );
		return 0;

	case G_XANIM_SET_ANIM_KNOB:
		XAnimSetAnimKnob_m( (void *)args[1], args[2], *(const float *)&args[3], args[4],
					*(const float *)&args[5], (short)args[6], 0, args[7] );
		return 0;

	case G_XANIM_SET_ANIM_KNOB_ALL:
		return XAnimSetAnimKnobAll_m( args[2], (void *)args[1], args[3],
						   *(const float *)&args[4], args[5],
						   *(const float *)&args[6], (short)args[7], 0, args[8] );

	case G_XANIM_SET_ANIM_RATE:
		XAnimSetAnimRate_m( args[1], args[2], *(const float *)&args[3] );
		return 0;

	case G_XANIM_SET_TIME:
		XAnimSetTime_m( (void *)args[1], args[2], *(const float *)&args[3] );
		return 0;

	case G_XANIM_SET_ANIM_LIMITED:
		XAnimSetAnimLimited_m( (void *)args[1], args[2], *(const float *)&args[3], *(const float *)&args[4],
					*(const float *)&args[5], (short)args[6], 0, args[7] );
		return 0;

	case G_XANIM_CLEAR_TREE:
		XAnimClearTree( (void *)args[1] );
		return 0;

	case G_XANIM_IS_PRIMITIVE:
		return XAnimIsLeafNode_m( (const void *)SV_XANIM_TREE( args[1] ), args[2] );

	case G_XANIM_IS_LEAF:
		/* retail 0x004571DC: no NULL-tree test */
		return *(short *)( SV_XANIM_TREE( args[1] ) + 8 * args[2] + 8 ) == 0;

	case G_XANIM_LENGTH_MSEC:
	{
		float *parts = *(float **)( *(int *)( args[1] + 8 * args[2] + 12 ) + 4 );

		return (int)( *(unsigned short *)parts / parts[1] * 1000.0f );
	}

	case G_XANIM_LENGTH_SEC:
	{
		/* retail 0x0045722F: no NULL-tree test */
		float *parts = *(float **)( *(int *)( SV_XANIM_TREE( args[1] ) + 8 * args[2] + 12 ) + 4 );

		return FloatAsInt_1( *(unsigned short *)parts / parts[1] );
	}

	case G_XANIM_SET_ANIM_KNOB_INT:
		XAnimSetAnimKnobInternal_m( (void *)args[1], args[2], *(const float *)&args[3],
					*(const float *)&args[4], *(const float *)&args[5],
					(short)args[6], 0, args[7] );
		return 0;

	case G_XANIM_SET_ANIM_INT_LIMITED:
		XAnimSetAnimInternalLimited_m( 0, (short)args[6], args[1], args[2],
					*(const float *)&args[3], args[4], *(const float *)&args[5],
					args[7] );
		return 0;

	case G_XANIM_GET_ABS_DELTA:
		XAnimGetAbsDelta_m( (void *)args[1], args[2], (float *)args[3],
					(void *)args[4] );
		return 0;

	case G_XANIM_GET_REL_DELTA:
		XAnimGetRelDelta_m( (void *)args[1], args[2], (float *)args[3], (void *)args[4],
					args[5] );
		return 0;

	case G_XANIM_GET_REL_DELTA_TIME:
		XAnimGetRelDeltaForTime_m( SV_XANIM_TREE( args[1] ), args[2], (void *)args[4],
					(float *)args[3], *(const float *)&args[5],
					*(const float *)&args[6] );
		return 0;

	case G_XANIM_GET_ABS_DELTA_TIME:
		XAnimGetAbsDeltaForTime_m((const void*)SV_XANIM_TREE( args[1] ), args[2], (float*)(void *)args[4], (float*)(float *)args[3], *(const float *)&args[5] );
		return 0;

	case G_XANIM_IS_LOOPED:
		return XAnimIsLooped_m((const void *)(SV_XANIM_TREE( args[1] ) ), args[2]);

	case G_XANIM_HAS_NOTETRACK:
		return XAnimNotetrackExists_m( (const void *)SV_XANIM_TREE( args[1] ), args[2],
									   (unsigned short)args[3] );

	case G_XANIM_GET_TIME:
		return FloatAsInt_1( (float)XAnimGetTime( (void *)args[1], args[2] ) );

	case G_XANIM_GET_WEIGHT:
		return FloatAsInt_1( (float)XAnimGetWeight( (void *)args[1], args[2] ) );

	case G_DOBJ_DUMP_INFO:
		SV_DObjDumpInfo( args[1] );
		return 0;

	case G_DOBJ_CREATE_SKEL_FOR_BONE:
		return SV_DObjCreateSkelForBone( args[1], args[2] );

	case G_DOBJ_SKEL_UP_TO_DATE:
		return SV_DObjIsSkelUpToDate( args[1], (const int *)args[2] );

	case G_DOBJ_UPDATE_SERVER_TIME:
		return SV_DObjUpdateServerTime( args[1], args[3],
										*(const float *)&args[2] );

	case G_DOBJ_INIT_SERVER_TIME:
		SV_DObjInitServerTime( args[1], *(const float *)&args[2] );
		return 0;

	case G_DOBJ_GET_HIERARCHY_BITS:
		SV_DObjGetHierarchyBits_m( args[1], args[2], (int *)args[3] );
		return 0;

	case G_DOBJ_CALC_ANIM:
		SV_DObjCalcAnim( args[1], (void *)args[2] );
		return 0;

	case G_DOBJ_CALC_SKEL:
		SV_DObjCalcSkel( args[1], (void *)args[2] );
		return 0;

	case G_XANIM_RESTORE_TREE:
		cand_XAnimRestoreTree( args[1] );
		return 0;

	case G_XANIM_SAVE_TREE:
		cand_XAnimSaveTree( (void *)args[1] );
		return 0;

	case G_XANIM_COPY_TREE:
		XAnimCopyTree( (const void *)args[1], (void *)args[2] );
		return 0;

	case G_DOBJ_NUM_PARTS:
		return SV_DObjNumParts( args[1] );

	case G_DOBJ_GET_PART_INDEX:
		return SV_DObjGetPartIndex( args[1], (const char *)args[2] );

	case G_DOBJ_GET_SKEL_BASE:
		return (int)SV_DObjGetSkelBase( args[1] );

	case G_DOBJ_DISPLAY_ANIM:
		SV_DObjDisplayAnim( args[1] );
		return 0;

	case G_XANIM_HAS_TIME:
		return XAnimHasTime( (void *)args[1], args[2] );

	case G_XANIM_CHILD_COUNT:
		/* retail 0x004574F1: no NULL-tree test */
		return *(unsigned short *)( SV_XANIM_TREE( args[1] ) + 8 * args[2] + 8 );

	case G_XANIM_CHILD_BASE:
		/* retail 0x00457512: no NULL-tree test */
		return args[3] + *(unsigned short *)( SV_XANIM_TREE( args[1] ) + 8 * args[2] + 14 );

	case G_XANIM_ROOT_PART_COUNT:
		return ****(short ****)( **(int **)( args[1] + 4 ) + 4 );

	case G_XANIM_ROOT_PART_LIST:
		return ***(int ***)( **(int **)( args[1] + 4 ) + 4 ) + 2;

	case G_DOBJ_GET_SKEL_END:
		return (int)SV_DObjGetSkelEnd( args[1] );

	case G_DOBJ_SET_ROT_TRANS_INDEX:
		return SV_DObjSetRotTransIndex( args[1], args[3], (const int *)args[2] );

	case G_DOBJ_SET_CONTROL_ROT_TRANS_INDEX:
		return SV_DObjSetControlRotTransIndex( args[1], args[3],
											   (const int *)args[2] );

	case G_XANIM_GET_ANIM_NAME:
		return (int)XAnimGetAnimName( (const void *)SV_XANIM_TREE( args[1] ), args[2] );

	case G_DOBJ_GET_TREE:
		return SV_DObjGetTree( args[1] );

	case G_XANIM_TREE_FIELD:
		return *(int *)( args[1] + 4 );

	case G_DOBJ_DEBUG_DRAW_BOUNDS:
		SV_DObjDebugDrawBounds_m( args[1] );
		return 0;

	case G_GET_WEAPON_INFO_MEMORY:
		return (int) Com_GetWeaponInfoMemory( args[1], (int *)args[2], 1 );

	case G_FREE_WEAPON_INFO_MEMORY:
		Com_FreeWeaponInfoMemory( 1, args[1] );
		return 0;

	case G_FREE_CLIENT_SCRIPT_PERS:
		SV_FreeClientScriptPers();
		return 0;

	case G_RESET_ENTITY_PARSE_POINT:
		sv.entityParsePoint = CM_EntityString();
		return 0;

	default:
		Com_Error( ERR_DROP, "\x15" "Bad game system trap: %i", args[0] );
		return -1;
	}
}

/* ---- SV_ShutdownGameProgs  0x00457970 ---- */
void SV_ShutdownGameProgs( void ) {
	if ( !vm ) {
		return;
	}

	xanim_activePoolSlot = SV_XANIM_POOL_SERVER;
	VM_Call( vm, GAME_SHUTDOWN, qfalse );
	Hunk_ClearToLowMark();

	VM_Free( vm );
	vm = NULL;

	if ( snd_aliasSetLoaded[SND_LOCALE_LOCALIZED] ) {
		snd_aliasLocalizedName[0] = '\0';
		if ( snd_aliasTable[SND_LOCALE_LOCALIZED] ) {
			free( (void *)snd_aliasTable[SND_LOCALE_LOCALIZED] );
			snd_aliasTable[SND_LOCALE_LOCALIZED] = NULL;
			snd_aliasTableCount[SND_LOCALE_LOCALIZED] = 0;
			memset( snd_aliasHashTable[SND_LOCALE_LOCALIZED], 0,
					SND_ALIAS_HASH_SLICE_BYTES );
		}
		snd_aliasSetLoaded[SND_LOCALE_LOCALIZED] = 0;
	}
}

/* ---- SV_InitGameVM  0x00457A60 ---- */
void SV_InitGameVM( qboolean restart, int matchState ) {
	void    *engineCallbacks;
	void    *gameCallbacks;
	int i;

	sv.entityParsePoint = CM_EntityString();

	engineCallbacks = Scr_NearHook( NULL );
	gameCallbacks = (void *)VM_Call( vm, GAME_SCRIPT_FAR_HOOK, engineCallbacks );
	Scr_NearHook( gameCallbacks );

	Sys_LoadingKeepAlive();
	VM_Call( vm, GAME_INIT, svs.time, Com_Milliseconds(), restart, matchState );
	Sys_LoadingKeepAlive();

	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		svs.clients[i].gentity = NULL;
	}

	if ( com_dedicated->integer ) {
		Com_CvarDump( 4 );
	}
}

/* ---- SV_RestartGameProgs  0x00457B00 ---- */
void SV_RestartGameProgs( int matchState ) {
	void    *lowEnd;
	void    *highEnd;

	VM_Call( vm, GAME_SHUTDOWN, qtrue );

	/* retail 0x00457B21 stores the low counters before forming the range */
	HUNK_LOW_TEMP = HUNK_LOW_MARK;
	HUNK_LOW_PERM = HUNK_LOW_MARK;

	lowEnd = (char *)s_hunkData + HUNK_LOW_MARK;
	highEnd = (char *)s_hunkData + s_hunkTotal - s_hunkHighPermanent;

	XModelClearData( highEnd, lowEnd );
	FS_ClearDataForFiles( lowEnd, highEnd );

	SV_InitGameVM( qtrue, matchState );
}

/* ---- SV_InitGameProgs  0x00457B60 ---- */
void SV_InitGameProgs( int matchState ) {
	vm = VM_Create( "game", SV_GameSystemCalls );
	if ( !vm ) {
		Com_Error( ERR_FATAL, "\x15VM_Create on game failed" );
	}

	Hunk_SetLowMark();

	SV_InitGameVM( qfalse, matchState );
}

/* ---- SV_GameCommand  0x00457BB0 ---- */
qboolean SV_GameCommand( void ) {
	if ( sv.state != SS_GAME ) {
		return qfalse;
	}

	xanim_activePoolSlot = SV_XANIM_POOL_SERVER;
	return VM_Call( vm, GAME_CONSOLE_COMMAND_SV );
}
