// g_utils_mp.c -- misc utility functions for game module

#include "g_local.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MAXCHOICES                  32

/* universal/q_math.c.  The three-operand forms compose left-to-right:
 * out = in0 applied inside in1. */
void        AnglesToAxis( const vec3_t angles, vec3_t axis[3] );
void        AxisToAngles( vec3_t axis[3], vec3_t angles );
float       vectoyaw( const vec3_t vec );
void        MatrixMultiply( vec3_t in0[3], vec3_t in1[3], vec3_t out[3] );
void        MatrixTranspose( vec3_t in[3], vec3_t out[3] );
void        MatrixMultiply43( const void *in0, const void *in1, void *out );
void        MatrixInverseOrthogonal34( const void *in, void *out );
void        MatrixTransformVector43( const vec3_t in, const void *mat, vec3_t out );
void        DObjSkelMatrixMultiply43( const void *skelMat, const void *in1, void *out );
void        DObjSkel2MatrixMultiply43( const float *boneMatrix, const float *mat43, float *out );
void        QuatFromPitch_m( float q[4], float degrees );
void        QuatFromYaw_m( float q[4], float degrees );
void        QuatFromRoll_m( float degrees, float q[4] );

/* universal/q_shared.c. */
int         Q_stricmp( const char *s1, const char *s2 );
int         Com_sprintf( char *dest, int size, const char *fmt, ... );
char        *va( const char *format, ... );

/* forward: g_main_mp.c */
void        G_Printf( const char *fmt, ... );
void        G_Error( const char *fmt, ... );

/* forward: g_combat_mp.c */
void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec3_t dir,
			   const vec3_t point, int damage, int dflags, int mod, int hitLoc );

/* forward: g_client_mp.c */
void G_ClientStopUsingTurret( gentity_t *ent );

/* forward: g_scr_main_mp.c */
void Scr_FreeEntityConstStrings( gentity_t *ent );

/* The XModel behind every model configstring index, filled in by G_ModelIndex
 * and read back by G_GetModel. */
static XModel *s_modelIndexXModels[CS_MODELS_COUNT];

/*
=========================================================================

model / sound configstring indexes

=========================================================================
*/

/*
================
G_FindConfigstringIndex

================
*/
int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create,
							 const char *fieldname ) {
	int i;
	const char  *s;

	if ( !name || !name[0] ) {
		return 0;
	}

	for ( i = 1 ; i < max ; i++ ) {
		s = trap_GetConfigstringConst( start + i );
		if ( !s[0] ) {
			break;
		}
		if ( !_stricmp( s, name ) ) {
			return i;
		}
	}

	if ( !create ) {
		if ( fieldname ) {
			Scr_Error( va( "%s \"%s\" not precached", fieldname, name ) );
		}
		return 0;
	}

	if ( i == max ) {
		G_Error( "G_FindConfigstringIndex: overflow" );
	}

	trap_SetConfigstring( start + i, name );

	return i;
}

// keep: open-coded rather than a G_FindConfigstringIndex call (original codegen)
int G_LocalizedStringIndex( const char *name ) {
	int i;
	const char  *s;
	qboolean create;

	if ( !name[0] ) {
		return 0;
	}

	create = level.spawning;

	for ( i = 1 ; i < CS_LOCALIZED_STRINGS_COUNT ; i++ ) {
		s = trap_GetConfigstringConst( CS_LOCALIZED_STRINGS + i );
		if ( !s[0] ) {
			break;
		}
		if ( !_stricmp( s, name ) ) {
			return i;
		}
	}

	if ( !create ) {
		Scr_Error( va( "%s \"%s\" not precached", "localized string", name ) );
		return 0;
	}

	if ( i == CS_LOCALIZED_STRINGS_COUNT ) {
		G_Error( "G_FindConfigstringIndex: overflow" );
	}

	trap_SetConfigstring( CS_LOCALIZED_STRINGS + i, name );

	return i;
}

int G_ShaderIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_SHADERS, CS_SHADERS_COUNT,
									level.spawning, "shader" );
}

/*
================
G_ModelIndex

Not a G_FindConfigstringIndex wrapper: it caches the XModel alongside the
configstring, and a model that was not precached is an error but still gets
registered.
================
*/
int G_ModelIndex( const char *name ) {
	int i;
	const char  *s;

	if ( !name[0] ) {
		return 0;
	}

	for ( i = 1 ; i < CS_MODELS_COUNT ; i++ ) {
		s = trap_GetConfigstringConst( CS_MODELS + i );
		if ( !s[0] ) {
			break;
		}
		if ( !_stricmp( s, name ) ) {
			return i;
		}
	}

	if ( !level.spawning ) {
		Scr_Error( va( "model '%s' not precached", name ) );
	}

	if ( i == CS_MODELS_COUNT ) {
		G_Error( "G_ModelIndex: overflow" );
	}

	s_modelIndexXModels[i] = trap_XModelGet( name );
	trap_SetConfigstring( CS_MODELS + i, name );

	return i;
}

static XModel *G_GetModel( int modelIndex ) {
	return s_modelIndexXModels[modelIndex];
}

const char *G_ModelName( int modelIndex ) {
	return trap_GetConfigstringConst( CS_MODELS + modelIndex );
}

int G_TagIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_TAGS, CS_TAGS_COUNT, qtrue, NULL );
}

int G_EffectIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_EFFECTS, CS_EFFECTS_COUNT,
									level.spawning, "effect" );
}

int G_ShellShockIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_SHELLSHOCKS, CS_SHELLSHOCKS_COUNT, qtrue, NULL );
}

byte G_SoundAliasIndex( const char *name ) {
	return (byte)G_FindConfigstringIndex( name, CS_SOUNDS, CS_SOUNDS_COUNT, qtrue, NULL );
}

unsigned short G_GetGameId( gentity_t *ent ) {
	int entnum;

	entnum = ent->s.number;
	return Scr_GetEntityId( entnum, SCR_CLASS_ENTITY );
}

/*
================
G_DObjUpdate

Rebuild the entity's DObj from its model and its attach slots.  Clients keep
their DObj in sync from ClientEndFrame instead, so they are left alone.
================
*/
void G_DObjUpdate( gentity_t *ent ) {
	DObjModel models[MAX_ATTACHED_MODELS + 2];
	unsigned short gameId;
	int modelCount;
	int modelIndex;
	int i;

	if ( ent->client ) {
		return;
	}

	G_SafeDObjFree( ent );

	modelIndex = ent->model;
	if ( !modelIndex ) {
		G_UpdateTagInfoOfChildren( ent, qfalse );
		return;
	}

	gameId = G_GetGameId( ent );

	models[0].model = G_GetModel( modelIndex );
	models[0].tagName = NULL;
	models[0].boneName = -modelIndex;
	modelCount = 1;

	switch ( ent->s.eType ) {
	case ET_GENERAL:
	case ET_SCRIPTMOVER:
	case ET_TURRET:
		ent->s.index = modelIndex;
		break;
	}

	for ( i = 0 ; i < MAX_ATTACHED_MODELS ; i++ ) {
		modelIndex = ent->attachModelIndex[i];
		if ( !modelIndex ) {
			continue;
		}
		models[modelCount].model = G_GetModel( modelIndex );
		models[modelCount].tagName = SL_ConvertToString( ent->attachTagName[i] );
		models[modelCount].boneName = -modelIndex;
		modelCount++;
	}

	trap_DObjCreate( models, (unsigned short)modelCount, NULL, ent->s.number, gameId );

	G_UpdateTagInfoOfChildren( ent, qtrue );
}

void G_SetModel( gentity_t *ent, const char *modelName ) {
	if ( !modelName[0] ) {
		ent->model = 0;
		return;
	}

	ent->model = (byte)G_ModelIndex( modelName );
}

qboolean G_EntAttach( gentity_t *ent, const char *modelName, const char *tagName ) {
	int i;

	for ( i = 0 ; i < MAX_ATTACHED_MODELS ; i++ ) {
		if ( !ent->attachModelIndex[i] ) {
			ent->attachModelIndex[i] = (byte)G_ModelIndex( modelName );
			ent->attachTagName[i] = SL_GetLowercaseString( tagName, 0 );
			G_DObjUpdate( ent );
			return qtrue;
		}
	}

	return qfalse;
}

qboolean G_EntDetach( gentity_t *ent, const char *modelName, const char *tagName ) {
	unsigned short tagId;
	int i;

	tagId = SL_FindLowercaseString( tagName );
	if ( !tagId ) {
		return qfalse;
	}

	i = 0;
	while ( ent->attachTagName[i] != tagId
			|| _stricmp( G_ModelName( ent->attachModelIndex[i] ), modelName ) ) {
		if ( ++i >= MAX_ATTACHED_MODELS ) {
			return qfalse;
		}
	}

	ent->attachModelIndex[i] = 0;
	Scr_SetString( &ent->attachTagName[i], 0 );

	for ( ; i < MAX_ATTACHED_MODELS - 1 ; i++ ) {
		ent->attachModelIndex[i] = ent->attachModelIndex[i + 1];
		ent->attachTagName[i] = ent->attachTagName[i + 1];
	}

	ent->attachModelIndex[i] = 0;
	ent->attachTagName[i] = 0;

	G_DObjUpdate( ent );

	return qtrue;
}

void G_EntDetachAll( gentity_t *ent ) {
	int i;

	for ( i = 0 ; i < MAX_ATTACHED_MODELS ; i++ ) {
		ent->attachModelIndex[i] = 0;
		Scr_SetString( &ent->attachTagName[i], 0 );
	}

	G_DObjUpdate( ent );
}

/*
================
G_EntLinkToInternal

Build the link record.  Refuses a link that would close a cycle, and a tagged
link whose tag is not a bone of the parent's DObj.
================
*/
static qboolean G_EntLinkToInternal( gentity_t *child, gentity_t *parent, const char *tagName ) {
	int parentTagIndex;
	gentity_t           *ancestor;
	entityLinkInfo_t    *linkInfo;

	G_EntUnlink( child );

	if ( tagName[0] ) {
		if ( !trap_DObjExists( parent ) ) {
			return qfalse;
		}
		parentTagIndex = trap_DObjGetBoneIndex( parent, tagName );
		if ( parentTagIndex < 0 ) {
			return qfalse;
		}
	} else {
		parentTagIndex = -1;
	}

	for ( ancestor = parent ; ; ancestor = ancestor->linkInfo->parent ) {
		if ( ancestor == child ) {
			return qfalse;
		}
		if ( !ancestor->linkInfo ) {
			break;
		}
	}

	linkInfo = MT_Alloc( sizeof( entityLinkInfo_t ), 16 );
	linkInfo->parent = parent;
	linkInfo->tagName = tagName[0] ? SL_GetLowercaseString( tagName, 0 ) : 0;
	linkInfo->nextChild = parent->firstChild;
	linkInfo->parentTagIndex = parentTagIndex;
	memset( &linkInfo->relAxis, 0, sizeof( linkInfo->relAxis ) );
	parent->firstChild = child;
	child->linkInfo = linkInfo;
	memset( &linkInfo->parentRelAxis, 0, sizeof( linkInfo->parentRelAxis ) );

	return qtrue;
}

qboolean G_EntLinkTo( gentity_t *child, gentity_t *parent, const char *tagName ) {
	if ( !G_EntLinkToInternal( child, parent, tagName ) ) {
		return qfalse;
	}

	G_CalcTagAxis( child, qfalse );

	return qtrue;
}

qboolean G_EntLinkToWithOffset( gentity_t *child, gentity_t *parent, const char *tagName,
								const vec3_t originOffset, const vec3_t anglesOffset ) {
	entityLinkInfo_t    *linkInfo;

	if ( !G_EntLinkToInternal( child, parent, tagName ) ) {
		return qfalse;
	}

	linkInfo = child->linkInfo;
	AnglesToAxis( anglesOffset, linkInfo->relAxis.axis );
	VectorCopy( originOffset, linkInfo->relAxis.origin );

	return qtrue;
}

/*
================
G_EntUnlink

Freeze the entity where the link left it, then drop it out of its parent's
child list.
================
*/
void G_EntUnlink( gentity_t *ent ) {
	entityLinkInfo_t    *linkInfo;
	gentity_t           *prev;
	gentity_t           *child;

	linkInfo = ent->linkInfo;
	if ( !linkInfo ) {
		return;
	}

	G_SetOrigin( ent, ent->r.currentOrigin );
	G_SetAngle( ent, ent->r.currentAngles );

	prev = NULL;
	for ( child = linkInfo->parent->firstChild ; child != ent ;
		  child = child->linkInfo->nextChild ) {
		prev = child;
	}

	if ( prev ) {
		prev->linkInfo->nextChild = linkInfo->nextChild;
	} else {
		linkInfo->parent->firstChild = linkInfo->nextChild;
	}

	ent->linkInfo = NULL;
	Scr_SetString( &linkInfo->tagName, 0 );
	MT_Free( linkInfo, sizeof( entityLinkInfo_t ) );
}

qboolean G_EntIsLinkedTo( gentity_t *child, gentity_t *parent ) {
	entityLinkInfo_t    *linkInfo;

	linkInfo = child->linkInfo;

	return (qboolean)( linkInfo && linkInfo->parent == parent );
}

/*
================
G_UpdateTagInfo

The parent's DObj was rebuilt, so the cached bone index is stale.  A link whose
tag no longer exists is dropped.
================
*/
void G_UpdateTagInfo( gentity_t *ent, qboolean updateBoneIndex ) {
	entityLinkInfo_t    *linkInfo;

	linkInfo = ent->linkInfo;

	if ( linkInfo->tagName ) {
		if ( updateBoneIndex ) {
			linkInfo->parentTagIndex = trap_DObjGetBoneIndex( linkInfo->parent,
															  SL_ConvertToString( linkInfo->tagName ) );
			if ( linkInfo->parentTagIndex >= 0 ) {
				return;
			}
		}
		G_EntUnlink( ent );
	} else {
		linkInfo->parentTagIndex = -1;
	}
}

void G_UpdateTagInfoOfChildren( gentity_t *parent, qboolean updateBoneIndex ) {
	gentity_t   *child;
	gentity_t   *nextChild;

	for ( child = parent->firstChild ; child ; child = nextChild ) {
		nextChild = child->linkInfo->nextChild;
		G_UpdateTagInfo( child, updateBoneIndex );
	}
}

/*
================
G_CalcTagParentAxis

The world transform of whatever the child hangs off: the parent itself for an
untagged link, otherwise the parent's tagged bone.
================
*/
void G_CalcTagParentAxis( gentity_t *child, matrix43_t *outAxis ) {
	entityLinkInfo_t    *linkInfo;
	gentity_t           *parent;
	matrix43_t parentAxis;

	linkInfo = child->linkInfo;
	parent = linkInfo->parent;

	if ( linkInfo->parentTagIndex >= 0 ) {
		AnglesToAxis( parent->r.currentAngles, parentAxis.axis );
		VectorCopy( parent->r.currentOrigin, parentAxis.origin );

		G_DObjCalcBone( parent, linkInfo->parentTagIndex );
		// keep: integer offset arithmetic matches original codegen
		DObjSkelMatrixMultiply43( (float *)( linkInfo->parentTagIndex * 64
								  + (int)trap_DObjGetMatrixArray( parent ) ), &parentAxis, outAxis );
	} else {
		AnglesToAxis( parent->r.currentAngles, outAxis->axis );
		VectorCopy( parent->r.currentOrigin, outAxis->origin );
	}
}

void G_CalcTagParentRelAxis( gentity_t *child, matrix43_t *outAxis ) {
	entityLinkInfo_t    *linkInfo;
	matrix43_t parentAxis;

	linkInfo = child->linkInfo;
	G_CalcTagParentAxis( child, &parentAxis );
	MatrixMultiply43( &linkInfo->parentRelAxis, &parentAxis, outAxis );
}

/*
================
G_CalcTagAxis

Record where the entity currently sits relative to its link parent.  With
useLinkedAngles the origin is left alone and only the rotation is captured.
================
*/
void G_CalcTagAxis( gentity_t *ent, int useLinkedAngles ) {
	entityLinkInfo_t    *linkInfo;
	matrix43_t inverseParentAxis;
	matrix43_t parentAxis;
	matrix43_t localAxis;

	G_CalcTagParentAxis( ent, &parentAxis );
	AnglesToAxis( ent->r.currentAngles, localAxis.axis );
	linkInfo = ent->linkInfo;

	if ( useLinkedAngles ) {
		MatrixTranspose( parentAxis.axis, inverseParentAxis.axis );
		MatrixMultiply( localAxis.axis, inverseParentAxis.axis, linkInfo->relAxis.axis );
	} else {
		MatrixInverseOrthogonal34( &parentAxis, &inverseParentAxis );
		VectorCopy( ent->r.currentOrigin, localAxis.origin );
		MatrixMultiply43( &localAxis, &inverseParentAxis, &linkInfo->relAxis );
	}
}

/*
================
G_SetFixedLink

Drive the entity from its stored relative transform.  Mode 0 takes the whole
transform, 1 takes the origin plus the yaw, 2 takes the origin only.
================
*/
void G_SetFixedLink( gentity_t *ent, int mode ) {
	entityLinkInfo_t    *linkInfo;
	matrix43_t parentAxis;
	matrix43_t linkedAxis;

	G_CalcTagParentAxis( ent, &parentAxis );
	linkInfo = ent->linkInfo;

	switch ( mode ) {
	case 0:
		MatrixMultiply43( &linkInfo->relAxis, &parentAxis, &linkedAxis );
		VectorCopy( linkedAxis.origin, ent->r.currentOrigin );
		AxisToAngles( linkedAxis.axis, ent->r.currentAngles );
		break;
	case 1:
		MatrixMultiply43( &linkInfo->relAxis, &parentAxis, &linkedAxis );
		VectorCopy( linkedAxis.origin, ent->r.currentOrigin );
		ent->r.currentAngles[1] = vectoyaw( linkedAxis.axis[0] );
		break;
	case 2:
		MatrixTransformVector43( linkInfo->relAxis.origin, &parentAxis, linkedAxis.origin );
		VectorCopy( linkedAxis.origin, ent->r.currentOrigin );
		break;
	}
}

/*
================
G_GeneralLink

Per-frame follow for a linked entity: recompute the world transform and push it
out through the trajectories so the clients interpolate it.
================
*/
void G_GeneralLink( gentity_t *ent ) {
	G_SetFixedLink( ent, 0 );

	G_SetOrigin( ent, ent->r.currentOrigin );
	G_SetAngle( ent, ent->r.currentAngles );

	ent->s.pos.trType = TR_INTERPOLATE;
	ent->s.apos.trType = TR_INTERPOLATE;

	trap_LinkEntity( ent );
}

void Think_GeneralLink( gentity_t *ent ) {
	ent->nextthink = level.time + 50;

	if ( ent->linkInfo ) {
		G_GeneralLink( ent );
	}
}

void G_SafeDObjFree( gentity_t *ent ) {
	trap_SafeDObjFree( ent->s.number, qfalse );
}

int G_DObjUpdateServerTime( gentity_t *ent, int stopOnNotetrack ) {
	return trap_DObjUpdateServerTime( ent, level.frameTime * 0.001f, stopOnNotetrack );
}

/*
================
G_DObjSetLocalTagInternal

Overwrite one bone of the pose with an explicit origin/angles.
Not static: g_misc_mp.c calls it.
================
*/
void G_DObjSetLocalTagInternal( gentity_t *ent, const vec3_t origin, const vec3_t angles,
								int boneIndex ) {
	DObjAnimMat *rotTrans;
	float roll;
	struct { vec4_t yaw; vec4_t pitch; vec4_t roll; vec4_t pitchYaw; } quats;   // keep: original stack layout

	rotTrans = &trap_DObjGetRotTransArray( ent )[boneIndex];

	QuatFromYaw_m( quats.yaw, angles[1] );
	QuatFromPitch_m( quats.pitch, angles[0] );
	roll = angles[2];
	QuatFromRoll_m( roll, quats.roll );

	QuatMultiply( quats.pitch, quats.yaw, quats.pitchYaw );
	QuatMultiply( quats.roll, quats.pitchYaw, rotTrans->quat );

	rotTrans->accumulatedWeight = 0.0f;
	VectorCopy( origin, rotTrans->translation );
}

qboolean G_DObjSetLocalTag( gentity_t *ent, int *partBits, const char *tagName,
							const vec3_t origin, const vec3_t angles ) {
	int boneIndex;

	boneIndex = trap_DObjGetBoneIndex( ent, tagName );
	if ( boneIndex < 0 ) {
		return qfalse;
	}

	if ( !trap_DObjSetRotTransIndex( ent, partBits, boneIndex ) ) {
		return qfalse;
	}

	G_DObjSetLocalTagInternal( ent, origin, angles, boneIndex );

	return qtrue;
}

qboolean G_DObjSetControlTagAngles( gentity_t *ent, int *partBits, const char *tagName,
									const vec3_t angles ) {
	int boneIndex;

	boneIndex = trap_DObjGetBoneIndex( ent, tagName );
	if ( boneIndex < 0 ) {
		return qfalse;
	}

	if ( !trap_DObjSetControlRotTransIndex( ent, partBits, boneIndex ) ) {
		return qfalse;
	}

	G_DObjSetLocalTagInternal( ent, vec3_origin, angles, boneIndex );

	return qtrue;
}

void G_DObjCalcPose( gentity_t *ent ) {
	int partBits[4];

	memset( partBits, 255, sizeof( partBits ) );

	if ( trap_DObjCreateSkelForBones( ent, partBits ) ) {
		return;
	}

	trap_DObjCalcAnim( ent, partBits );
	if ( ent->controller ) {
		ent->controller( ent, (unsigned int *)partBits );
	}
	trap_DObjCalcSkel( ent, partBits );
}

void G_DObjCalcBone( gentity_t *ent, int boneIndex ) {
	int partBits[4];

	if ( trap_DObjCreateSkelForBone( ent, boneIndex ) ) {
		return;
	}

	trap_DObjGetHierarchyBits( ent, boneIndex, partBits );
	trap_DObjCalcAnim( ent, partBits );
	if ( ent->controller ) {
		ent->controller( ent, (unsigned int *)partBits );
	}
	trap_DObjCalcSkel( ent, partBits );
}

float *G_DObjGetLocalTagMatrix( gentity_t *ent, const char *tagName ) {
	int boneIndex;

	boneIndex = trap_DObjGetBoneIndex( ent, tagName );
	if ( boneIndex < 0 ) {
		return NULL;
	}

	G_DObjCalcBone( ent, boneIndex );

	return trap_DObjGetMatrixArray( ent ) + boneIndex * 16;
}

qboolean G_DObjGetWorldTagMatrix( gentity_t *ent, const char *tagName, void *outMatrix ) {
	const float *localMatrix;
	matrix43_t entityAxis;

	localMatrix = G_DObjGetLocalTagMatrix( ent, tagName );
	if ( !localMatrix ) {
		return qfalse;
	}

	AnglesToAxis( ent->r.currentAngles, entityAxis.axis );
	VectorCopy( ent->r.currentOrigin, entityAxis.origin );
	DObjSkel2MatrixMultiply43( localMatrix, (const float *)&entityAxis, (float *)outMatrix );

	return qtrue;
}

/*
=============
G_Find

Searches all active entities for the next one that holds the const string id
`match` in the field at `fieldofs`.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.
=============
*/
gentity_t *G_Find( gentity_t *from, int fieldofs, unsigned short match ) {
	unsigned short s;

	if ( !from ) {
		from = g_entities;
	} else {
		from++;
	}

	for ( ; from < &g_entities[level.num_entities] ; from++ ) {
		if ( !from->inuse ) {
			continue;
		}
		s = *( unsigned short * )( (byte *)from + fieldofs );
		if ( !s ) {
			continue;
		}
		if ( s == match ) {
			return from;
		}
	}

	return NULL;
}

gentity_t *G_FindStr( gentity_t *from, int fieldofs, const char *match ) {
	char    *s;

	if ( !from ) {
		from = g_entities;
	} else {
		from++;
	}

	for ( ; from < &g_entities[level.num_entities] ; from++ ) {
		if ( !from->inuse ) {
			continue;
		}
		s = *( char ** )( (byte *)from + fieldofs );
		if ( !s ) {
			continue;
		}
		if ( !Q_stricmp( s, match ) ) {
			return from;
		}
	}

	return NULL;
}

/*
=============
G_PickTarget

Selects a random entity from among the targets
=============
*/
gentity_t *G_PickTarget( unsigned short targetname ) {
	gentity_t   *ent = NULL;
	int num_choices = 0;
	gentity_t   *choice[MAXCHOICES];

	if ( !targetname ) {
		return NULL;
	}

	while ( 1 ) {
		ent = G_Find( ent, FOFS( targetname ), targetname );
		if ( !ent ) {
			break;
		}
		choice[num_choices++] = ent;
		if ( num_choices == MAXCHOICES ) {
			break;
		}
	}

	if ( !num_choices ) {
		G_Printf( "G_PickTarget: target %s not found\n", SL_ConvertToString( targetname ) );
		return NULL;
	}

	return choice[rand() % num_choices];
}

/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char *vtos( const vec3_t v ) {
	static int index;
	static char str[8][32];
	char    *s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2] );

	return s;
}

char *vtosf( const vec3_t v ) {
	static int index;
	static char str[8][64];
	char    *s;

	s = str[index];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 64, "(%f %f %f)", v[0], v[1], v[2] );

	return s;
}

/*
===============
G_SetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void G_SetMovedir( vec3_t angles, vec3_t movedir ) {
	static vec3_t VEC_UP        = { 0, -1, 0 };
	static vec3_t MOVEDIR_UP    = { 0, 0, 1 };
	static vec3_t VEC_DOWN      = { 0, -2, 0 };
	static vec3_t MOVEDIR_DOWN  = { 0, 0, -1 };

	if ( angles[0] == VEC_UP[0] && angles[1] == VEC_UP[1] && angles[2] == VEC_UP[2] ) {
		VectorCopy( MOVEDIR_UP, movedir );
	} else if ( angles[0] == VEC_DOWN[0] && angles[1] == VEC_DOWN[1] && angles[2] == VEC_DOWN[2] ) {
		VectorCopy( MOVEDIR_DOWN, movedir );
	} else {
		AngleVectors( angles, movedir, NULL, NULL );
	}

	VectorClear( angles );
}

void G_InitGentity( gentity_t *e ) {
	e->inuse = qtrue;
	Scr_SetString( &e->classname, scr_const.noclass );
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
	e->eventTime = 0;
	e->freeAfterEvent = qfalse;
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_Spawn( void ) {
	gentity_t   *e;
	int i;

	if ( !level.freeListHead ) {
		if ( level.num_entities == MAX_GENTITIES - 2 ) {
			for ( i = 0 ; i < level.num_entities ; i++ ) {
				G_Printf( "%4i: '%s', origin: %f %f %f\n", i,
						  g_entities[i].classname ? SL_ConvertToString( g_entities[i].classname ) : "",
						  g_entities[i].r.currentOrigin[0],
						  g_entities[i].r.currentOrigin[1],
						  g_entities[i].r.currentOrigin[2] );
			}
			G_Error( "G_Spawn: no free entities" );
		}

		e = &level.gentities[level.num_entities];
		level.num_entities++;
		trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
							 &level.clients[0].ps, sizeof( gclient_t ) );
	} else {
		e = level.freeListHead;
		level.freeListHead = level.freeListHead->nextFree;
		if ( !level.freeListHead ) {
			level.freeListTail = NULL;
		}
		e->nextFree = NULL;
	}

	G_InitGentity( e );

	return e;
}

/*
=================
G_SpawnPlayerClone

The corpse queue: BODY_QUEUE_SIZE slots immediately above the clients, reused
round-robin.  Bit 3 of eFlags is flipped on every reuse so the client sees the
slot as a different entity and does not interpolate the old corpse into the
new one.
=================
*/
gentity_t *G_SpawnPlayerClone( void ) {
	gentity_t   *e;
	int eFlags;

	e = &level.gentities[MAX_CLIENTS + level.playerCloneCursor];
	level.playerCloneCursor = ( level.playerCloneCursor + 1 ) % BODY_QUEUE_SIZE;

	eFlags = ~e->s.eFlags & EF_TELEPORT_BIT;

	if ( e->inuse ) {
		G_FreeEntity( e );
	}

	G_InitGentity( e );
	e->s.eFlags = eFlags;

	return e;
}

/*
=================
G_FreeEntityRefs

Drop every pointer and entity number the rest of the game holds to ent, so
nothing follows it into the free list.
=================
*/
void G_FreeEntityRefs( gentity_t *ent ) {
	gentity_t   *e;
	int entnum;
	int i;

	entnum = ent->s.number;

	for ( i = 0 ; i < level.num_entities ; i++ ) {
		e = &g_entities[i];
		if ( !e->inuse ) {
			continue;
		}
		if ( e->parent == ent ) {
			e->parent = NULL;
		}
		if ( e->r.ownerNum == entnum ) {
			e->r.ownerNum = ENTITYNUM_NONE;
			if ( e->s.eType == ET_TURRET ) {
				e->active = qfalse;
			}
		}
		if ( e->s.groundEntityNum == entnum ) {
			e->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}

	for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
		e = &g_entities[i];
		if ( !e->inuse ) {
			continue;
		}
		if ( e->client->lookatEnt == ent ) {
			e->client->lookatEnt = NULL;
		}
	}
}

/*
=================
G_FreeEntity

Marks the entity as free, and pushes it on the tail of the free list so the
oldest free slot is reused first.  The client and corpse slots are never queued.
=================
*/
void G_FreeEntity( gentity_t *ent ) {
	int spawnCount;

	G_EntUnlink( ent );
	while ( ent->firstChild ) {
		G_EntUnlink( ent->firstChild );
	}

	trap_UnlinkEntity( ent );       // unlink from world
	trap_SafeDObjFree( ent->s.number, qtrue );   // also release the anim tree
	G_FreeEntityRefs( ent );

	if ( ent->turret ) {
		G_FreeTurret( ent );
	}

	Scr_FreeEntityConstStrings( ent );
	Scr_FreeEntityNum( ent->s.number, SCR_CLASS_ENTITY );

	spawnCount = ent->spawnCount;

	memset( ent, 0, sizeof( gentity_t ) );

	if ( ent - level.gentities >= MAX_CLIENTS + BODY_QUEUE_SIZE ) {
		if ( level.freeListTail ) {
			level.freeListTail->nextFree = ent;
		} else {
			level.freeListHead = ent;
		}
		level.freeListTail = ent;
		ent->nextFree = NULL;
	}

	ent->spawnCount = spawnCount + 1;
}

/*
=================
G_TempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_TempEntity( const vec3_t origin, int event ) {
	gentity_t   *e;
	vec3_t snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	Scr_SetString( &e->classname, scr_const.tempEntity );
	e->eventTime = level.time;
	e->r.eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	snapped[0] = (float)(int)snapped[0];    // save network bandwidth
	snapped[1] = (float)(int)snapped[1];
	snapped[2] = (float)(int)snapped[2];
	G_SetOrigin( e, snapped );

	// find cluster for PVS
	trap_LinkEntity( e );

	return e;
}

/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
G_KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
void G_KillBox( gentity_t *ent ) {
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t   *hit;
	vec3_t mins, maxs;

	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES, CONTENTS_BODY );

	for ( i = 0 ; i < num ; i++ ) {
		hit = &g_entities[touch[i]];
		if ( !hit->client ) {
			continue;
		}
		if ( !hit->r.linked ) {
			continue;
		}

		// nail it
		G_Damage( hit, ent, ent, NULL, NULL, 100000, DAMAGE_NO_TEAM_PROTECTION, MOD_TELEFRAG, HITLOC_NONE );
	}
}

//==============================================================================

/*
===============
G_AddPredictableEvent

Use for non-pmove events that would also be predicted on the
client side: jumppads and item pickups
Adds an event+parm and twiddles the event counter
===============
*/
void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm ) {
	if ( !ent->client ) {
		return;
	}

	BG_AddPredictableEventToPlayerstate( event, eventParm, &ent->client->ps );
}

/*
===============
G_AddEvent

Adds an event+parm and twiddles the event counter
===============
*/
void G_AddEvent( gentity_t *ent, int event, int eventParm ) {
	// clients need to add the event in playerState_t instead of entityState_t
	if ( ent->client ) {
		ent->client->ps.events[ent->client->ps.eventSequence & 3] = event;
		ent->client->ps.eventParms[ent->client->ps.eventSequence & 3] = eventParm;
		ent->client->ps.eventSequence++;
	} else {
		ent->s.events[ent->s.eventSequence & 3] = event;
		ent->s.eventParms[ent->s.eventSequence & 3] = eventParm;
		ent->s.eventSequence++;
	}

	ent->eventTime = level.time;
	ent->r.eventTime = level.time;
}

void G_PlaySoundAliasAtPoint( const vec3_t origin, byte alias ) {
	gentity_t   *te;

	if ( !alias ) {
		return;
	}

	te = G_TempEntity( origin, EV_SOUND_ALIAS );
	te->s.eventParm = alias;
}

void G_PlaySoundAlias( gentity_t *ent, byte alias ) {
	if ( !alias ) {
		return;
	}

	G_AddEvent( ent, EV_SOUND_ALIAS, alias );
}

/*
=============
G_AnimScriptSound

The animation script's sound callback
=============
*/
void G_AnimScriptSound( int client, const char *name ) {
	byte alias;

	alias = G_SoundAliasIndex( name );
	if ( !alias ) {
		return;
	}

	G_AddEvent( &g_entities[client], EV_SOUND_ALIAS, alias );
}

//==============================================================================

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *ent, const vec3_t origin ) {
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( ent->s.pos.trDelta );

	VectorCopy( origin, ent->r.currentOrigin );
}

/*
==============
G_SetAngle
==============
*/
void G_SetAngle( gentity_t *ent, const vec3_t angles ) {
	VectorCopy( angles, ent->s.apos.trBase );
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.apos.trTime = 0;
	ent->s.apos.trDuration = 0;
	VectorClear( ent->s.apos.trDelta );

	VectorCopy( angles, ent->r.currentAngles );
}

/*
====================
infront
====================
*/
qboolean infront( gentity_t *self, gentity_t *other ) {
	vec3_t vec;
	float dot;
	vec3_t forward;

	AngleVectors( self->r.currentAngles, forward, NULL, NULL );
	VectorSubtract( other->r.currentOrigin, self->r.currentOrigin, vec );
	VectorNormalize( vec );
	dot = DotProduct( vec, forward );

	if ( dot > 0.0 ) {
		return qtrue;
	}

	return qfalse;
}

/* Debug polygons only work when running a local game; the multiplayer module
 * keeps the entry point and does nothing. */
int DebugLine( const vec3_t start, const vec3_t end, int color ) {
	return 0;
}

void G_SetConstString( unsigned short *dest, const char *string ) {
	Scr_SetString( dest, 0 );
	*dest = SL_GetString( string, 0 );
}
