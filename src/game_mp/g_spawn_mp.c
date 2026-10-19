// g_spawn_mp.c -- spawning entities from the map's entity string, and the
// script system's view of entity fields

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bg_public.h"
#include "g_local.h"

#define GAME_VERSION            "cod"

// keep: the case order of the fieldtype_t switches matches original codegen

typedef void ( *fieldSet_t )( gentity_t *ent, int fieldnum );

typedef struct {
	const char *name;
	int ofs;
	fieldtype_t type;
	fieldSet_t setter;      // NULL uses Scr_SetGenericField
} field_t;

extern clientField_t clientFields[];

typedef struct {
	const char *name;
	void ( *spawn )( gentity_t *ent );
} spawn_t;


// keep: original function order
static unsigned short G_NewString( const char *string );

// the key / value pair G_ParseEntityField is working on, for GScr_FieldError
static const char *fieldKey;
static const char *fieldValue;

qboolean G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int i;

	if ( !level.spawningMapEntities ) {
		*out = (char *)defaultString;
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !strcmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = (float)atof( s );
	return present;
}

qboolean G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}

static void Scr_ReadOnlyField( gentity_t *ent, int fieldnum ) {
	Scr_Error( "Tried to set a read only entity field" );
}

//
// fields are needed for spawning from the entity string
// and are also the entity fields the script system sees;
// a field's number is its index here
//
static const field_t fields[] = {
	{"classname", FOFS( classname ), F_STRING, Scr_ReadOnlyField},
	{"origin", FOFS( r.currentOrigin ), F_VECTOR, Scr_SetOrigin},
	{"model", FOFS( model ), F_MODEL, Scr_ReadOnlyField},
	{"spawnflags", FOFS( spawnflags ), F_INT, Scr_ReadOnlyField},
	{"speed", FOFS( speed ), F_FLOAT, NULL},
	{"closespeed", FOFS( closespeed ), F_FLOAT, NULL},
	{"target", FOFS( target ), F_STRING, NULL},
	{"targetname", FOFS( targetname ), F_STRING, NULL},
	{"message", FOFS( message ), F_STRING, NULL},
	{"teamname", FOFS( team ), F_STRING, NULL},
	{"wait", FOFS( wait ), F_FLOAT, NULL},
	{"random", FOFS( random ), F_FLOAT, NULL},
	{"count", FOFS( count ), F_INT, NULL},
	{"health", FOFS( health ), F_INT, Scr_SetHealth},
	{"light", 0, F_IGNORE, NULL},
	{"dmg", FOFS( damage ), F_INT, NULL},
	{"angles", FOFS( r.currentAngles ), F_VECTOR, Scr_SetAngles},
	{"duration", FOFS( duration ), F_FLOAT, NULL},
	{"rotate", FOFS( rotate ), F_VECTOR, NULL},
	{"degrees", FOFS( angle ), F_FLOAT, NULL},
	{"time", FOFS( speed ), F_FLOAT, NULL},
	{"_color", FOFS( dl_color ), F_VECTOR, NULL},
	{"color", FOFS( dl_color ), F_VECTOR, NULL},
	{"key", FOFS( key ), F_INT, NULL},
	{"harc", FOFS( harc ), F_FLOAT, NULL},
	{"varc", FOFS( varc ), F_FLOAT, NULL},
	{"delay", FOFS( delay ), F_FLOAT, NULL},
	{"radius", FOFS( radius ), F_INT, NULL},
	{"missionlevel", FOFS( missionLevel ), F_INT, NULL},
	{"start_size", FOFS( start_size ), F_INT, NULL},
	{"end_size", FOFS( end_size ), F_INT, NULL},
	{"shard", FOFS( count ), F_INT, NULL},
	{"spawnitem", FOFS( spawnitem ), F_STRING, NULL},
	{"track", FOFS( track ), F_STRING, NULL},

	{NULL, 0, F_INT, NULL}
};

// keep: original data layout
static spawn_t spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_group", SP_info_null},
	{"func_door_rotating", SP_func_door_rotating},
	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_once", SP_trigger_once},
	{"target_location", SP_target_location},
	{"mp_target_location", SP_target_location},
	{"light", SP_light},
	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_mg42", SP_turret},
	{"misc_turret", SP_turret},
	{"misc_spawner", SP_misc_spawner},
	{"corona", SP_corona},
	{"trigger_use", trigger_use},
	{"trigger_damage", SP_trigger_damage},
	{"trigger_lookat", SP_trigger_lookat},
	{"script_brushmodel", SP_script_brushmodel},
	{"script_model", SP_script_model},
	{"script_origin", SP_script_origin},

	{NULL, NULL}
};

/*
===============
GScr_FieldError

While the map is being spawned there is no script thread to blame, so the
failure is fatal and names the key / value pair instead.
===============
*/
void GScr_FieldError( gentity_t *ent, const char *error ) {
	char *classname;

	if ( level.spawningMapEntities ) {
		G_SpawnString( "classname", "", &classname );
		Com_Error( ERR_DROP, va( "\x15" "classname '%s', key '%s', value '%s': %s",
						  classname, fieldKey, fieldValue, error ) );
	}

	Scr_ObjectError( error );
}

/*
===============
G_SetEntityScriptVariable

A spawn key that is not in fields[] may still be a field some script
declared; set it on the entity's script object if the VM knows it.
===============
*/
static void G_SetEntityScriptVariable( const char *key, const char *value, gentity_t *ent ) {
	unsigned short fieldnum;
	int type;
	float f;

	fieldnum = Scr_FindField( key, &type );
	if ( !fieldnum ) {
		return;
	}

	switch ( type ) {
	case VAR_STRING:
		Scr_AddString( value );
		break;
	case VAR_FLOAT:
		f = (float)atof( value );
		Scr_AddFloat( f );
		break;
	case VAR_INTEGER:
		Scr_AddInt( atoi( value ) );
		break;
	default:
		return;
	}

	Scr_SetDynamicEntityField( ent->s.number, SCR_CLASS_ENTITY, fieldnum );
}

/*
===============
G_ParseEntityField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
static void G_ParseEntityField( const char *key, const char *value, gentity_t *ent ) {
	const field_t *f;
	byte *b;
	float v[3];

	fieldKey = key;
	fieldValue = value;

	for ( f = fields ; f->name ; f++ ) {
		if ( !Q_stricmp( f->name, key ) ) {
			// found it
			b = (byte *)ent;

			switch ( f->type ) {
			case F_STRING:
				Scr_SetString( (unsigned short *)( b + f->ofs ), 0 );
				*(unsigned short *)( b + f->ofs ) = G_NewString( value );
				break;
			case F_VECTOR:
				sscanf( value, "%f %f %f", &v[0], &v[1], &v[2] );
				( (float *)( b + f->ofs ) )[0] = v[0];
				( (float *)( b + f->ofs ) )[1] = v[1];
				( (float *)( b + f->ofs ) )[2] = v[2];
				break;
			case F_INT:
				*(int *)( b + f->ofs ) = atoi( value );
				break;
			case F_FLOAT:
				*(float *)( b + f->ofs ) = (float)atof( value );
				break;
			case F_MODEL:
				if ( *value == '*' ) {
					ent->s.index = (unsigned short)atoi( value + 1 );
				} else {
					ent->model = G_ModelIndex( value );
				}
				break;
			default:
				break;
			}
			return;
		}
	}

	G_SetEntityScriptVariable( key, value, ent );
}

void G_ParseEntityFields( gentity_t *ent ) {
	int i;

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseEntityField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	G_SetOrigin( ent, ent->r.currentOrigin );
	G_SetAngle( ent, ent->r.currentAngles );
}

void G_DuplicateEntityFields( gentity_t *dest, gentity_t *source ) {
	const field_t *f;

	for ( f = fields ; f->name ; f++ ) {
		switch ( f->type ) {
		case F_STRING:
			Scr_SetString( (unsigned short *)( (byte *)dest + f->ofs ),
						   *(unsigned short *)( (byte *)source + f->ofs ) );
			break;
		case F_VECTOR:
			( (float *)( (byte *)dest + f->ofs ) )[0] = ( (float *)( (byte *)source + f->ofs ) )[0];
			( (float *)( (byte *)dest + f->ofs ) )[1] = ( (float *)( (byte *)source + f->ofs ) )[1];
			( (float *)( (byte *)dest + f->ofs ) )[2] = ( (float *)( (byte *)source + f->ofs ) )[2];
			break;
		case F_INT:
			*(int *)( (byte *)dest + f->ofs ) = *(int *)( (byte *)source + f->ofs );
			break;
		case F_FLOAT:
			*(float *)( (byte *)dest + f->ofs ) = *(float *)( (byte *)source + f->ofs );
			break;
		case F_MODEL:
			*( (byte *)dest + f->ofs ) = *( (byte *)source + f->ofs );
			break;
		default:
			break;
		}
	}
}

void G_DuplicateScriptFields( gentity_t *dest, gentity_t *source ) {
	Scr_CopyEntityNum( source->s.number, dest->s.number, SCR_CLASS_ENTITY );
}

/*
===============
G_CallSpawn

Spawns an entity for the current spawn vars, then finds its spawn
function and calls it.  An entity without one is left as parsed.
===============
*/
void G_CallSpawn( void ) {
	spawn_t *s;
	gitem_t *item;
	gentity_t *ent;
	char *classname;

	G_SpawnString( "classname", "", &classname );
	if ( !classname ) {
		G_Printf( "G_CallSpawn: NULL classname\n" );
		return;
	}

	// check item spawn functions
	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( !strcmp( item->classname, classname ) ) {
			ent = G_Spawn();
			G_ParseEntityFields( ent );
			G_SpawnItem( ent, item );
			return;
		}
	}

	// check normal spawn functions
	for ( s = spawns ; s->name ; s++ ) {
		if ( !strcmp( s->name, classname ) ) {
			ent = G_Spawn();
			G_ParseEntityFields( ent );
			s->spawn( ent );
			return;
		}
	}

	ent = G_Spawn();
	G_ParseEntityFields( ent );
}

/*
===============
G_CallSpawnEntity

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawnEntity( gentity_t *ent ) {
	spawn_t *s;
	gitem_t *item;
	const char *classname;

	if ( !ent->classname ) {
		G_Printf( "G_CallSpawnEntity: NULL classname\n" );
		return qfalse;
	}

	classname = SL_ConvertToString( ent->classname );

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( !strcmp( item->classname, classname ) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	for ( s = spawns ; s->name ; s++ ) {
		if ( !strcmp( s->name, classname ) ) {
			s->spawn( ent );
			return qtrue;
		}
	}

	G_Printf( "%s doesn't have a spawn function\n", SL_ConvertToString( ent->classname ) );
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line, and returns it as a script string
=============
*/
static unsigned short G_NewString( const char *string ) {
	char newb[65536];
	char *new_p;
	int i, l;

	l = strlen( string ) + 1;
	if ( l > sizeof( newb ) ) {
		G_Error( "G_NewString: len = %i > %i\n", l, sizeof( newb ) );
	}

	new_p = newb;

	// turn \n into a real linefeed
	for ( i = 0 ; i < l ; i++ ) {
		if ( string[i] == '\\' && i < l - 1 ) {
			i++;
			if ( string[i] == 'n' ) {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}

	return SL_GetString( newb, 0 );
}

/*
===============
GScr_AddFieldsForEntity

Registers fields[], then the client fields, with the script entity class
===============
*/
void GScr_AddFieldsForEntity( void ) {
	const field_t *f;
	unsigned short classnum;

	classnum = g_scr_data.classMap[SCR_CLASS_ENTITY].classnum;

	for ( f = fields ; f->name ; f++ ) {
		switch ( f->type ) {
		case F_INT:
		case F_FLOAT:
		case F_CSTRING:
		case F_STRING:
		case F_VECTOR:
		case F_ENTITY:
		case F_OBJECT:
		case F_MODEL:
			Scr_AddClassField( classnum, f->name, (unsigned short)( f - fields ) );
			break;
		default:
			break;
		}
	}

	GScr_AddFieldsForClient( classnum );
}

void GScr_AddFieldsForRadiant( void ) {
	Scr_AddFields( "radiant", ".txt" );
}

void Scr_SetEntityField( int entnum, unsigned int fieldnum ) {
	gentity_t *ent;
	const field_t *f;

	ent = &g_entities[entnum];

	if ( ( fieldnum & CLIENT_FIELD_MASK ) == CLIENT_FIELD_MASK ) {
		if ( !ent->client ) {
			GScr_FieldError( ent, "field must be applied to a player" );
		}
		Scr_SetClientField( ent->client, fieldnum & CLIENT_FIELD_INDEX_MASK );
		return;
	}

	f = &fields[fieldnum];
	if ( f->setter ) {
		f->setter( ent, fieldnum );
		return;
	}
	Scr_SetGenericField( ent, f->type, f->ofs );
}

void Scr_SetGenericField( void *base, int type, int ofs ) {
	vec3_t value;

	switch ( type ) {
	case F_STRING:
		Scr_SetString( (unsigned short *)( (byte *)base + ofs ), Scr_GetConstString( 0 ) );
		break;
	case F_VECTOR:
		Scr_GetVector( 0, value );
		( (float *)( (byte *)base + ofs ) )[0] = value[0];
		( (float *)( (byte *)base + ofs ) )[1] = value[1];
		( (float *)( (byte *)base + ofs ) )[2] = value[2];
		break;
	case F_INT:
		*(int *)( (byte *)base + ofs ) = Scr_GetInt( 0 );
		break;
	case F_FLOAT:
		*(float *)( (byte *)base + ofs ) = Scr_GetFloat( 0 );
		break;
	case F_VECTOR_Y:
		Scr_GetVector( 0, value );
		*(float *)( (byte *)base + ofs ) = value[1];
		break;
	case F_ENTITY:
		*(gentity_t **)( (byte *)base + ofs ) = Scr_GetEntity( 0 );
		break;
	default:
		break;
	}
}

void Scr_SetObjectField( int classnum, int objectNum, int fieldnum ) {
	switch ( classnum ) {
	case SCR_CLASS_ENTITY:
		Scr_SetEntityField( objectNum, fieldnum );
		break;
	case SCR_CLASS_HUDELEM:
		Scr_SetHudElemField( objectNum, fieldnum );
		break;
	}
}

void Scr_GetEntityField( int entnum, unsigned int fieldnum ) {
	gentity_t *ent;
	const field_t *f;

	ent = &g_entities[entnum];

	if ( ( fieldnum & CLIENT_FIELD_MASK ) == CLIENT_FIELD_MASK ) {
		if ( !ent->client ) {
			GScr_FieldError( ent, "field must be applied to a player" );
		}
		Scr_GetClientField( ent->client, fieldnum & CLIENT_FIELD_INDEX_MASK );
		return;
	}

	f = &fields[fieldnum];
	Scr_GetGenericField( ent, f->type, f->ofs );
}

void Scr_GetGenericField( void *base, int type, int ofs ) {
	unsigned short id;
	float f;
	vec3_t value;

	switch ( type ) {
	case F_STRING:
		id = *(unsigned short *)( (byte *)base + ofs );
		if ( id ) {
			Scr_AddConstString( id );
		}
		break;
	case F_CSTRING:
		Scr_AddString( (const char *)( (byte *)base + ofs ) );
		break;
	case F_VECTOR:
		Scr_AddVector( (const float *)( (byte *)base + ofs ) );
		break;
	case F_INT:
		Scr_AddInt( *(int *)( (byte *)base + ofs ) );
		break;
	case F_FLOAT:
		f = *(float *)( (byte *)base + ofs );
		Scr_AddFloat( f );
		break;
	case F_VECTOR_Y:
		value[0] = 0;
		value[1] = *(float *)( (byte *)base + ofs );
		value[2] = 0;
		Scr_AddVector( value );
		break;
	case F_ENTITY:
		if ( *(gentity_t **)( (byte *)base + ofs ) ) {
			Scr_AddEntity( *(gentity_t **)( (byte *)base + ofs ) );
		}
		break;
	case F_OBJECT:
		id = *(unsigned short *)( (byte *)base + ofs );
		if ( id ) {
			Scr_AddObject( id );
		}
		break;
	case F_MODEL:
		Scr_AddString( G_ModelName( *( (byte *)base + ofs ) ) );
		break;
	default:
		break;
	}
}

void Scr_GetObjectField( int classnum, int objectNum, int fieldnum ) {
	switch ( classnum ) {
	case SCR_CLASS_ENTITY:
		Scr_GetEntityField( objectNum, fieldnum );
		break;
	case SCR_CLASS_HUDELEM:
		Scr_GetHudElemField( objectNum, fieldnum );
		break;
	}
}

void Scr_FreeEntityConstStrings( gentity_t *ent ) {
	const field_t *f;
	int i;

	for ( f = fields ; f->name ; f++ ) {
		if ( f->type == F_STRING ) {
			Scr_SetString( (unsigned short *)( (byte *)ent + f->ofs ), 0 );
		}
	}

	for ( i = 0 ; i < MAX_ATTACHED_MODELS ; i++ ) {
		ent->attachModelIndex[i] = 0;
		Scr_SetString( &ent->attachTagName[i], 0 );
	}
}

void Scr_FreeEntity( gentity_t *ent ) {
	Scr_FreeEntityConstStrings( ent );
	Scr_FreeEntityNum( ent->s.number, SCR_CLASS_ENTITY );
}

void Scr_AddEntity( gentity_t *ent ) {
	Scr_AddEntityNum( ent->s.number, SCR_CLASS_ENTITY );
}

gentity_t *Scr_GetEntity( unsigned int num ) {
	unsigned int entnum;
	int classnum;

	entnum = Scr_GetEntityNum( num, &classnum );
	if ( classnum == SCR_CLASS_ENTITY && entnum < MAX_GENTITIES ) {
		return &g_entities[entnum];
	}

	Scr_ParamError( num, "not an entity" );
	return NULL;
}

void Scr_FreeHudElem( g_hudelem_t *elem ) {
	Scr_FreeHudElemConstStrings( elem );
	Scr_FreeEntityNum( elem - g_hudelems, SCR_CLASS_HUDELEM );
}

void Scr_AddHudElem( g_hudelem_t *elem ) {
	Scr_AddEntityNum( elem - g_hudelems, SCR_CLASS_HUDELEM );
}

g_hudelem_t *Scr_GetHudElem( unsigned int num ) {
	unsigned int elemnum;
	int classnum;

	elemnum = Scr_GetEntityNum( num, &classnum );
	// hudelems share the entity bound
	if ( classnum == SCR_CLASS_HUDELEM && elemnum < MAX_GENTITIES ) {
		return &g_hudelems[elemnum];
	}

	Scr_ParamError( num, "not a hudelem" );
	return NULL;
}

unsigned short Scr_ExecEntThread( gentity_t *ent, int handle, unsigned int paramcount ) {
	int entnum;

	entnum = ent->s.number;     // keep: matches original codegen
	return Scr_ExecEntThreadNum( entnum, SCR_CLASS_ENTITY, handle, paramcount );
}

void Scr_AddExecEntThread( gentity_t *ent, int handle, unsigned int paramcount ) {
	Scr_AddExecEntThreadNum( ent->s.number, SCR_CLASS_ENTITY, handle, paramcount );
}

void Scr_Notify( gentity_t *ent, unsigned short event, unsigned int paramcount ) {
	Scr_NotifyNum( ent->s.number, SCR_CLASS_ENTITY, event, paramcount );
}

/*
===============
Scr_GetEnt

getent( <value>, <key> ): the one entity whose string field <key> is
<value>.  Only F_STRING fields can be searched.
===============
*/
void Scr_GetEnt( void ) {
	unsigned short name;
	const char *key;
	int fieldnum;
	const field_t *f;
	gentity_t *ent, *found;
	int i;

	name = Scr_GetConstString( 0 );
	key = Scr_GetString( 1 );

	fieldnum = Scr_GetOffset( g_scr_data.classMap[SCR_CLASS_ENTITY].classnum, key );
	if ( fieldnum < 0 ) {
		return;
	}

	f = &fields[fieldnum];
	if ( f->type != F_STRING ) {
		return;
	}

	found = NULL;
	for ( i = 0, ent = g_entities ; i < level.num_entities ; i++, ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( !*(unsigned short *)( (byte *)ent + f->ofs ) ) {
			continue;
		}
		if ( *(unsigned short *)( (byte *)ent + f->ofs ) != name ) {
			continue;
		}
		if ( found ) {
			Scr_Error( "getent used with more than one entity" );
		}
		found = ent;
	}

	if ( found ) {
		Scr_AddEntity( found );
	}
}

/*
===============
Scr_GetEntArray

getentarray(): every entity in use.
getentarray( <value>, <key> ): every entity whose string field <key> is <value>.
===============
*/
void Scr_GetEntArray( void ) {
	unsigned short name;
	const char *key;
	int fieldnum;
	const field_t *f;
	gentity_t *ent;
	int i;

	if ( !Scr_GetNumParam() ) {
		Scr_MakeArray();
		for ( i = 0, ent = g_entities ; i < level.num_entities ; i++, ent++ ) {
			if ( ent->inuse ) {
				Scr_AddEntity( ent );
				Scr_AddArray();
			}
		}
		return;
	}

	name = Scr_GetConstString( 0 );
	key = Scr_GetString( 1 );

	fieldnum = Scr_GetOffset( g_scr_data.classMap[SCR_CLASS_ENTITY].classnum, key );
	if ( fieldnum < 0 ) {
		return;
	}

	f = &fields[fieldnum];
	if ( f->type != F_STRING ) {
		return;
	}

	Scr_MakeArray();
	for ( i = 0, ent = g_entities ; i < level.num_entities ; i++, ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( !*(unsigned short *)( (byte *)ent + f->ofs ) ) {
			continue;
		}
		if ( *(unsigned short *)( (byte *)ent + f->ofs ) != name ) {
			continue;
		}
		Scr_AddEntity( ent );
		Scr_AddArray();
	}
}

void GScr_SetDynamicEntityField( gentity_t *ent, int fieldnum ) {
	Scr_SetDynamicEntityField( ent->s.number, SCR_CLASS_ENTITY, fieldnum );
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	G_CallSpawn();
}

/*
====================
G_AddSpawnVarToken
====================
*/
static char *G_AddSpawnVarToken( const char *string ) {
	int l;
	char *dest;

	l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {", com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}

/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"ambienttrack"  Music track
"gravity"   800 is default gravity
"message" Text to print during connection process
"northyaw"  Direction of north for the compass
*/
void SP_worldspawn( void ) {
	char *s;

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime ) );

	G_SpawnString( "ambienttrack", "", &s );
	if ( *s ) {
		trap_SetConfigstring( CS_MUSIC, va( "n\\%s", s ) );
	} else {
		trap_SetConfigstring( CS_MUSIC, "" );
	}

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );              // map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );     // message of the day

	G_SpawnString( "gravity", "800", &s );
	trap_Cvar_Set( "g_gravity", s );

	G_SpawnString( "northyaw", "", &s );
	if ( *s ) {
		trap_SetConfigstring( CS_NORTHYAW, s );
	} else {
		trap_SetConfigstring( CS_NORTHYAW, "0" );
	}

	G_SpawnString( "spawnflags", "0", &s );
	g_entities[ENTITYNUM_WORLD].spawnflags = atoi( s );
	g_entities[ENTITYNUM_WORLD].worldflags = g_entities[ENTITYNUM_WORLD].spawnflags;

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	Scr_SetString( &g_entities[ENTITYNUM_WORLD].classname, scr_const.worldspawn );
	g_entities[ENTITYNUM_WORLD].inuse = qtrue;
}

/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	// allow calls to G_Spawn*()
	level.spawningMapEntities = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		G_Error( "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars() ) {
		G_CallSpawn();
	}

	level.spawningMapEntities = qfalse;         // any future calls to G_Spawn*() will be errors
}
