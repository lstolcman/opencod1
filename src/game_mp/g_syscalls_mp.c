// g_syscalls_mp.c -- the game module's system calls into the engine

#include "g_local.h"

static int ( QDECL * syscall )( int arg, ... ) = ( int ( QDECL * )( int, ... ) ) - 1;

void dllEntry( int ( QDECL *syscallptr )( int arg,... ) ) {
	syscall = syscallptr;
}

float IntAsFloat( int x ) {
	int intTemp;
	intTemp = x;
	return *(float *)&intTemp;
}

int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void    trap_Printf( const char *fmt ) {
	syscall( G_PRINT, fmt );
}

void    trap_Error( const char *fmt ) {
	syscall( G_ERROR, fmt );
}

void    trap_Error_Localized( const char *fmt ) {
	syscall( G_ERROR_NOPREFIX, fmt );
}

int     trap_Milliseconds( void ) {
	return syscall( G_MILLISECONDS );
}

int     trap_Argc( void ) {
	return syscall( G_ARGC );
}

void    trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( G_ARGV, n, buffer, bufferLength );
}

void    *trap_Hunk_AllocInternal( int size ) {
	return (void *)syscall( G_HUNK_ALLOC, size );
}

void    *trap_Hunk_AllocLowInternal( int size ) {
	return (void *)syscall( G_HUNK_ALLOC_LOW, size );
}

void    *trap_Hunk_AllocAlignInternal( int size, int align ) {
	return (void *)syscall( G_HUNK_ALLOC_ALIGN, size, align );
}

void    *trap_Hunk_AllocLowAlignInternal( int size, int align ) {
	return (void *)syscall( G_HUNK_ALLOC_LOW_ALIGN, size, align );
}

void    *trap_Hunk_AllocateTempMemoryInternal( int size ) {
	return (void *)syscall( G_HUNK_ALLOC_TEMP, size );
}

void    trap_Hunk_FreeTempMemoryInternal( void *buf ) {
	syscall( G_HUNK_FREE_TEMP, buf );
}

int     trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( G_FS_FOPEN_FILE, qpath, f, mode );
}

void    trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( G_FS_READ, buffer, len, f );
}

int     trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	return syscall( G_FS_WRITE, buffer, len, f );
}

void    trap_FS_Rename( const char *from, const char *to ) {
	syscall( G_FS_RENAME, from, to );
}

void    trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( G_FS_FCLOSE_FILE, f );
}

int     trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	return syscall( G_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

void    trap_SendConsoleCommand( int exec_when, const char *text ) {
	syscall( G_SEND_CONSOLE_COMMAND, exec_when, text );
}

void    trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	syscall( G_CVAR_REGISTER, cvar, var_name, value, flags );
}

void    trap_Cvar_Update( vmCvar_t *cvar ) {
	syscall( G_CVAR_UPDATE, cvar );
}

void    trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( G_CVAR_SET, var_name, value );
}

int     trap_Cvar_VariableIntegerValue( const char *var_name ) {
	return syscall( G_CVAR_VARIABLE_INTEGER_VALUE, var_name );
}

// the engine writes the result through the second argument
float   trap_Cvar_VariableValue( const char *var_name ) {
	float value;

	syscall( G_CVAR_VARIABLE_VALUE, var_name, &value );
	return value;
}

void    trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( G_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize );
}

void    trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
							 playerState_t *clients, int sizeofGameClient ) {
	syscall( G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGameClient );
}

void    trap_DropClient( int clientNum, const char *reason ) {
	syscall( G_DROP_CLIENT, clientNum, reason );
}

void    trap_SendServerCommand( int clientNum, int reliable, const char *command ) {
	syscall( G_SEND_SERVER_COMMAND, clientNum, reliable, command );
}

void    trap_SetConfigstring( int num, const char *string ) {
	syscall( G_SET_CONFIGSTRING, num, string );
}

void    trap_GetConfigstring( int num, char *buffer, int bufferSize ) {
	syscall( G_GET_CONFIGSTRING, num, buffer, bufferSize );
}

const char *trap_GetConfigstringConst( int num ) {
	return (const char *)syscall( G_GET_CONFIGSTRING_CONST, num );
}

void    trap_GetUserinfo( int num, char *buffer, int bufferSize ) {
	syscall( G_GET_USERINFO, num, buffer, bufferSize );
}

void    trap_SetUserinfo( int num, const char *buffer ) {
	syscall( G_SET_USERINFO, num, buffer );
}

void    trap_GetServerinfo( char *buffer, int bufferSize ) {
	syscall( G_GET_SERVERINFO, buffer, bufferSize );
}

void    trap_SetBrushModel( gentity_t *ent ) {
	syscall( G_SET_BRUSH_MODEL, ent );
}

void    trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
					const vec3_t end, int passEntityNum, int contentmask ) {
	syscall( G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask );
}

void    trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs,
						   const vec3_t end, int passEntityNum, int contentmask ) {
	syscall( G_TRACE_CAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask );
}

void    trap_SightTrace( int *hitOut, const vec3_t start, const vec3_t mins, const vec3_t maxs,
						 const vec3_t end, int passEntityNum, int passOwnerNum, int contentmask ) {
	syscall( G_SIGHT_TRACE, hitOut, start, mins, maxs, end, passEntityNum, passOwnerNum, contentmask );
}

void    trap_SightTraceCapsule( int *hitOut, const vec3_t start, const vec3_t mins, const vec3_t maxs,
								const vec3_t end, int passEntityNum, int passOwnerNum, int contentmask ) {
	syscall( G_SIGHT_TRACE_CAPSULE, hitOut, start, mins, maxs, end, passEntityNum, passOwnerNum, contentmask );
}

int     trap_SightTraceToEntity( const vec3_t start, const vec3_t mins, const vec3_t maxs,
								 const vec3_t end, int entityNum, int contentmask ) {
	return syscall( G_SIGHT_TRACE_TO_ENTITY, start, mins, maxs, end, entityNum, contentmask );
}

void    trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask ) {
	syscall( G_CM_BOX_TRACE, results, start, end, mins, maxs, model, brushmask );
}

void    trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask ) {
	syscall( G_CM_BOX_TRACE_2, results, start, end, mins, maxs, model, brushmask );
}

int     trap_CM_BoxSightTrace( const vec3_t start, const vec3_t end, const vec3_t mins,
							   const vec3_t maxs, clipHandle_t model, int brushmask ) {
	return syscall( G_CM_BOX_SIGHT_TRACE, start, end, mins, maxs, model, brushmask );
}

int     trap_CM_CapsuleSightTrace( const vec3_t start, const vec3_t end, const vec3_t mins,
								   const vec3_t maxs, clipHandle_t model, int brushmask ) {
	return syscall( G_CM_BOX_SIGHT_TRACE_2, start, end, mins, maxs, model, brushmask );
}

void    trap_LocationalTrace( trace_t *results, const vec3_t start, const vec3_t end,
							  int passEntityNum, int contentmask, const byte *dobjTracePartState ) {
	syscall( G_LOCATIONAL_TRACE, results, start, end, passEntityNum, contentmask, dobjTracePartState );
}

int     trap_PointContents( const vec3_t point, int passEntityNum, int contentmask ) {
	return syscall( G_POINT_CONTENTS, point, passEntityNum, contentmask );
}

int     trap_InPVS( const vec3_t p1, const vec3_t p2 ) {
	return syscall( G_IN_PVS, p1, p2 );
}

int     trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 ) {
	return syscall( G_IN_PVS_IGNORE_PORTALS, p1, p2 );
}

int     trap_InSnapshot( const vec3_t origin, int entityNum ) {
	return syscall( G_IN_SNAPSHOT, origin, entityNum );
}

void    trap_AdjustAreaPortalState( gentity_t *ent, qboolean open ) {
	syscall( G_ADJUST_AREA_PORTAL_STATE, ent, open );
}

int     trap_AreasConnected( int area1, int area2 ) {
	return syscall( G_AREAS_CONNECTED, area1, area2 );
}

void    trap_LinkEntity( gentity_t *ent ) {
	syscall( G_LINKENTITY, ent );
}

void    trap_UnlinkEntity( gentity_t *ent ) {
	syscall( G_UNLINKENTITY, ent );
}

int     trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *entityList,
							int maxcount, int contentmask ) {
	return syscall( G_ENTITIES_IN_BOX, mins, maxs, entityList, maxcount, contentmask );
}

int     trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return syscall( G_ENTITY_CONTACT, mins, maxs, ent );
}

int     trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent ) {
	return syscall( G_ENTITY_CONTACT_CAPSULE, mins, maxs, ent );
}

void    trap_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	syscall( G_GET_USERCMD, clientNum, cmd );
}

qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return syscall( G_GET_ENTITY_TOKEN, buffer, bufferSize );
}

void    trap_AddDebugString( const vec3_t origin, const vec3_t color, float scale, const char *text ) {
	syscall( G_ADD_DEBUG_STRING, origin, color, PASSFLOAT( scale ), text );
}

void    trap_AddDebugLine( const vec3_t start, const vec3_t end, const vec3_t color,
						   int depthTest, int duration ) {
	syscall( G_ADD_DEBUG_LINE, start, end, color, depthTest, duration );
}

void    trap_SetArchive( qboolean enable ) {
	syscall( G_ENABLE_ARCHIVED_SNAPSHOT, enable );
}

int     trap_RealTime( void *qtime ) {
	return syscall( G_REAL_TIME, qtime );
}

void    trap_SnapVector( float *v ) {
	syscall( G_SNAPVECTOR, v );
}

int     trap_Com_SoundAliasString( const char *name ) {
	return syscall( G_FIND_SOUND_ALIAS, name );
}

int     trap_Com_PickSoundAlias( const char *name ) {
	return syscall( G_PICK_SOUND_ALIAS, name );
}

int     trap_Com_SoundAliasIndex( void *alias ) {
	return syscall( G_SOUND_ALIAS_INDEX, alias );
}

int     trap_SurfaceTypeFromName( const char *name ) {
	return syscall( G_SURFACE_TYPE_FROM_NAME, name );
}

const char *trap_SurfaceTypeToName( int surfaceType ) {
	return (const char *)syscall( G_SURFACE_TYPE_TO_NAME, surfaceType );
}

void    *trap_Z_MallocInternal( int size ) {
	return (void *)syscall( G_Z_MALLOC, size );
}

void    trap_Z_FreeInternal( void *ptr ) {
	syscall( G_Z_FREE, ptr );
}

// returns NULL if no client slot is free
gentity_t *trap_AddTestClient( void ) {
	int clientNum;

	clientNum = syscall( G_ADD_TEST_CLIENT );
	if ( clientNum >= 0 ) {
		return &level.gentities[clientNum];
	}
	return NULL;
}

qboolean trap_GetArchivedPlayerState( int clientNum, int *frameTime, playerState_t *ps ) {
	return syscall( G_GET_ARCHIVED_CLIENT_INFO, clientNum, frameTime, ps );
}

XAnimTree *trap_XAnimCreateTree( XAnim *anims ) {
	return (XAnimTree *)syscall( G_XANIM_CREATE_TREE, anims );
}

XAnimTree *trap_XAnimCreateSmallTree( XAnim *anims ) {
	return (XAnimTree *)syscall( G_XANIM_CREATE_SMALL_TREE, anims );
}

void    trap_XAnimFreeSmallTree( XAnimTree *tree ) {
	syscall( G_XANIM_FREE_SMALL_TREE, tree );
}

int     trap_XModelExists( const char *name ) {
	return syscall( G_XMODEL_EXISTS, name );
}

XModel  *trap_XModelGet( const char *name ) {
	return (XModel *)syscall( G_XMODEL_GET, name );
}

void    trap_DObjCreate( DObjModel *models, unsigned short modelCount, XAnimTree *tree,
						 int handle, unsigned short scrNotifyId ) {
	syscall( G_DOBJ_CREATE, models, modelCount, tree, handle, scrNotifyId );
}

void    trap_SafeDObjFree( int handle, unsigned int releaseTree ) {
	syscall( G_DOBJ_FREE, handle, releaseTree );
}

// the DObj traps hand the engine the entity number, which is the entity's DObj handle
qboolean trap_DObjExists( gentity_t *ent ) {
	return syscall( G_DOBJ_EXISTS, ent->s.number );
}

// a tree's first field is its XAnim, so the engine just dereferences the pointer
XAnim   *trap_XAnimGetAnims( const XAnimTree *tree ) {
	return (XAnim *)syscall( G_DEREF_DWORD, tree );
}

// returns a script reference to the tree's root anim
scr_anim_t trap_XAnimGetRoot( const XAnimTree *tree ) {
	scr_anim_t anim;

	anim.index = 0;
	anim.anims = (unsigned short)Scr_GetAnimsIndex( syscall( G_DEREF_DWORD, tree ) );
	return anim;
}

// keep: the scr_anim_t / int anim parameters below match original codegen

void    trap_XAnimClearTreeGoalWeights( XAnimTree *tree, scr_anim_t anim, float goalWeight ) {
	syscall( G_XANIM_CLEAR_ANIM, tree, anim.index, PASSFLOAT( goalWeight ) );
}

void    trap_XAnimClearGoalWeight( XAnimTree *tree, int animIndex, float goalWeight ) {
	syscall( G_XANIM_CLEAR_ANIM_NODE, tree, (unsigned short)animIndex, PASSFLOAT( goalWeight ) );
}

void    trap_XAnimClearTreeGoalWeightsStrict( XAnimTree *tree, scr_anim_t anim, float goalWeight ) {
	syscall( G_XANIM_CLEAR_ANIM_CHILDREN, tree, anim.index, PASSFLOAT( goalWeight ) );
}

void    trap_XAnimSetCompleteGoalWeightKnob( XAnimTree *tree, unsigned short animIndex, float goalWeight,
											 float goalTime, float rate, unsigned short notifyIndex, int notifyFlags ) {
	syscall( G_XANIM_SET_ANIM_KNOB, tree, animIndex, PASSFLOAT( goalWeight ), PASSFLOAT( goalTime ),
			 PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void    trap_XAnimSetCompleteGoalWeightKnobAll( XAnimTree *tree, scr_anim_t anim,
												scr_anim_t childAnim, float goalWeight,
												float goalTime, float rate,
												unsigned short notifyIndex, int notifyFlags ) {
	syscall( G_XANIM_SET_ANIM_KNOB_ALL, tree, anim.index, childAnim.index, PASSFLOAT( goalWeight ),
			 PASSFLOAT( goalTime ), PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void    trap_XAnimSetAnimRate( XAnimTree *tree, scr_anim_t anim, float rate ) {
	syscall( G_XANIM_SET_ANIM_RATE, tree, anim.index, PASSFLOAT( rate ) );
}

void    trap_XAnimSetTime( XAnimTree *tree, int animIndex, float time ) {
	syscall( G_XANIM_SET_TIME, tree, (unsigned short)animIndex, PASSFLOAT( time ) );
}

void    trap_XAnimSetGoalWeightKnob( XAnimTree *tree, unsigned short animIndex, float goalWeight,
									 float goalTime, float rate, unsigned short notifyIndex, int notifyFlags ) {
	syscall( G_XANIM_SET_ANIM_LIMITED, tree, animIndex, PASSFLOAT( goalWeight ), PASSFLOAT( goalTime ),
			 PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void    trap_XAnimClearTree( XAnimTree *tree ) {
	syscall( G_XANIM_CLEAR_TREE, tree );
}

// G_XANIM_IS_PRIMITIVE is really XAnimHasTime, and G_XANIM_IS_LEAF XAnimIsPrimitive
int     trap_XAnimHasTime( scr_anim_t anim ) {
	return syscall( G_XANIM_IS_PRIMITIVE, anim.anims, anim.index );
}

int     trap_XAnimIsPrimitive( scr_anim_t anim ) {
	return syscall( G_XANIM_IS_LEAF, anim.anims, anim.index );
}

int     trap_XAnimGetLength( XAnimTree *tree, unsigned short animIndex ) {
	return syscall( G_XANIM_LENGTH_MSEC, tree, animIndex );
}

float   trap_XAnimGetLengthSeconds( scr_anim_t anim ) {
	return IntAsFloat( syscall( G_XANIM_LENGTH_SEC, anim.anims, anim.index ) );
}

void    trap_XAnimSetCompleteGoalWeight( XAnimTree *tree, scr_anim_t anim, float goalWeight,
										 float goalTime, float rate, unsigned short notifyIndex,
										 int notifyFlags ) {
	syscall( G_XANIM_SET_ANIM_KNOB_INT, tree, anim.index, PASSFLOAT( goalWeight ),
			 PASSFLOAT( goalTime ), PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void    trap_XAnimSetGoalWeight( XAnimTree *tree, scr_anim_t anim, float goalWeight,
								 float goalTime, float rate, unsigned short notifyIndex,
								 int notifyFlags ) {
	syscall( G_XANIM_SET_ANIM_INT_LIMITED, tree, anim.index, PASSFLOAT( goalWeight ),
			 PASSFLOAT( goalTime ), PASSFLOAT( rate ), notifyIndex, notifyFlags );
}

void    trap_XAnimCalcAbsDelta( XAnimTree *tree, scr_anim_t anim, float *deltaRot, float *deltaMove ) {
	syscall( G_XANIM_GET_ABS_DELTA, tree, anim.index, deltaRot, deltaMove );
}

void    trap_XAnimCalcDelta( XAnimTree *tree, unsigned short animIndex, float *deltaPos,
							 void *deltaQuat, int animIndexEnd ) {
	syscall( G_XANIM_GET_REL_DELTA, tree, animIndex, deltaPos, deltaQuat, animIndexEnd );
}

void    trap_XAnimGetRelDelta( scr_anim_t anim, float *deltaRot, float *deltaMove,
							   float startTime, float endTime ) {
	syscall( G_XANIM_GET_REL_DELTA_TIME, anim.anims, anim.index, deltaRot, deltaMove,
			 PASSFLOAT( startTime ), PASSFLOAT( endTime ) );
}

void    trap_XAnimGetAbsDelta( scr_anim_t anim, float *deltaPos, void *deltaQuat, float time ) {
	syscall( G_XANIM_GET_ABS_DELTA_TIME, anim.anims, anim.index, deltaPos, deltaQuat,
			 PASSFLOAT( time ) );
}

int     trap_XAnimIsLooped( scr_anim_t anim ) {
	return syscall( G_XANIM_IS_LOOPED, anim.anims, anim.index );
}

int     trap_XAnimNotetrackExists( scr_anim_t anim, unsigned short notetrack ) {
	return syscall( G_XANIM_HAS_NOTETRACK, anim.anims, anim.index, notetrack );
}

float   trap_XAnimGetTime( XAnimTree *tree, scr_anim_t anim ) {
	return IntAsFloat( syscall( G_XANIM_GET_TIME, tree, anim.index ) );
}

float   trap_XAnimGetWeight( XAnimTree *tree, scr_anim_t anim ) {
	return IntAsFloat( syscall( G_XANIM_GET_WEIGHT, tree, anim.index ) );
}

void    trap_DObjDumpInfo( gentity_t *ent ) {
	syscall( G_DOBJ_DUMP_INFO, ent->s.number );
}

qboolean trap_DObjCreateSkelForBone( gentity_t *ent, int boneIndex ) {
	return syscall( G_DOBJ_CREATE_SKEL_FOR_BONE, ent->s.number, boneIndex );
}

qboolean trap_DObjCreateSkelForBones( gentity_t *ent, const int *partBits ) {
	return syscall( G_DOBJ_SKEL_UP_TO_DATE, ent->s.number, partBits );
}

int     trap_DObjUpdateServerTime( gentity_t *ent, float serverTime, int stopOnNotetrack ) {
	return syscall( G_DOBJ_UPDATE_SERVER_TIME, ent->s.number,
					PASSFLOAT( serverTime ), stopOnNotetrack );
}

void    trap_DObjInitServerTime( gentity_t *ent, float serverTime ) {
	syscall( G_DOBJ_INIT_SERVER_TIME, ent->s.number,
			 PASSFLOAT( serverTime ) );
}

void    trap_DObjGetHierarchyBits( gentity_t *ent, int boneIndex, int *partBits ) {
	syscall( G_DOBJ_GET_HIERARCHY_BITS, ent->s.number, boneIndex, partBits );
}

int     trap_DObjCalcAnim( gentity_t *ent, void *partBits ) {
	return syscall( G_DOBJ_CALC_ANIM, ent->s.number, partBits );
}

int     trap_DObjCalcSkel( gentity_t *ent, void *partBits ) {
	return syscall( G_DOBJ_CALC_SKEL, ent->s.number, partBits );
}

void    trap_XAnimLoadAnimTree( int tree ) {
	syscall( G_XANIM_RESTORE_TREE, tree );
}

void    trap_XAnimSaveAnimTree( XAnimTree *tree ) {
	syscall( G_XANIM_SAVE_TREE, tree );
}

void    trap_XAnimCloneAnimTree( const XAnimTree *from, XAnimTree *to ) {
	syscall( G_XANIM_COPY_TREE, from, to );
}

int     trap_DObjNumBones( gentity_t *ent ) {
	return syscall( G_DOBJ_NUM_PARTS, ent->s.number );
}

int     trap_DObjGetBoneIndex( gentity_t *ent, const char *boneName ) {
	return syscall( G_DOBJ_GET_PART_INDEX, ent->s.number, boneName );
}

float   *trap_DObjGetMatrixArray( gentity_t *ent ) {
	return (float *)syscall( G_DOBJ_GET_SKEL_BASE, ent->s.number );
}

void    trap_DObjDisplayAnim( gentity_t *ent ) {
	syscall( G_DOBJ_DISPLAY_ANIM, ent->s.number );
}

// G_XANIM_HAS_TIME is really XAnimHasFinished
int     trap_XAnimHasFinished( XAnimTree *tree, scr_anim_t anim ) {
	return syscall( G_XANIM_HAS_TIME, tree, anim.index );
}

int     trap_XAnimGetNumChildren( scr_anim_t anim ) {
	return syscall( G_XANIM_CHILD_COUNT, anim.anims, anim.index );
}

// only the index is replaced; the anims handle is kept
scr_anim_t trap_XAnimGetChildAt( scr_anim_t anim, int childIndex ) {
	anim.index = (unsigned short)syscall( G_XANIM_CHILD_BASE, anim.anims, anim.index, childIndex );
	return anim;
}

int     trap_XModelNumBones( XModel *model ) {
	return syscall( G_XANIM_ROOT_PART_COUNT, model );
}

unsigned short *trap_XModelGetBoneNames( XModel *model ) {
	return (unsigned short *)syscall( G_XANIM_ROOT_PART_LIST, model );
}

DObjAnimMat *trap_DObjGetRotTransArray( gentity_t *ent ) {
	return (DObjAnimMat *)syscall( G_DOBJ_GET_SKEL_END, ent->s.number );
}

qboolean trap_DObjSetRotTransIndex( gentity_t *ent, const int *partBits, int boneIndex ) {
	return syscall( G_DOBJ_SET_ROT_TRANS_INDEX, ent->s.number,
					partBits, boneIndex );
}

qboolean trap_DObjSetControlRotTransIndex( gentity_t *ent, const int *partBits, int boneIndex ) {
	return syscall( G_DOBJ_SET_CONTROL_ROT_TRANS_INDEX, ent->s.number,
					partBits, boneIndex );
}

const char *trap_XAnimGetAnimName( scr_anim_t anim ) {
	return (const char *)syscall( G_XANIM_GET_ANIM_NAME, anim.anims, anim.index );
}

XAnimTree *trap_DObjGetTree( gentity_t *ent ) {
	return (XAnimTree *)syscall( G_DOBJ_GET_TREE, ent->s.number );
}

int     trap_XAnimGetAnimTreeSize( XAnim *anims ) {
	return syscall( G_XANIM_TREE_FIELD, anims );
}

void    trap_XModelDebugBoxes( gentity_t *ent ) {
	syscall( G_DOBJ_DEBUG_DRAW_BOUNDS, ent->s.number );
}

void    *trap_GetWeaponInfoMemory( int size, int *pPrevOwner ) {
	return (void *)syscall( G_GET_WEAPON_INFO_MEMORY, size, pPrevOwner );
}

void    trap_FreeWeaponInfoMemory( int iSource ) {
	syscall( G_FREE_WEAPON_INFO_MEMORY, iSource );
}

void    trap_FreeClientScriptPers( void ) {
	syscall( G_FREE_CLIENT_SCRIPT_PERS );
}

void    trap_ResetEntityParsePoint( void ) {
	syscall( G_RESET_ENTITY_PARSE_POINT );
}
