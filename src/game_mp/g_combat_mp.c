// g_combat_mp.c -- damage, death and the hit location tables

#include "g_local.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define MASK_ALL                    ( -1 )
#define MASK_CANDAMAGE              0x02802091

// the floor G_Damage clamps a dead entity's health to
#define GIB_HEALTH                  -999

// these divide by 32768, not 32767
// keep: crandom's form matches original codegen
#define random()                    ( rand() * ( 1.0f / 32768.0f ) )
#define crandom()                   ( 2.0f * random() - 1.0f )

// a field of the table ParseConfigStringToStruct walks
typedef struct parseField_s {
	const char      *key;
	int ofs;
	int type;
} parseField_t;

#define PARSE_FIELD_FLOAT           6

/* forward: q_shared.c / q_math.c */
float           VectorNormalize2( const vec3_t v, vec3_t out );
qboolean        Info_Validate( const char *s );
float           vectoyaw( const vec3_t vec );
void            AddLeanToPosition( vec3_t pos, float yaw, float leanFrac,
								   float rightScale, float upScale );
int             ParseConfigStringToStruct( const parseField_t *fields, void *base, int numFields,
										   const char *buffer, int numGenericTypes, void *parseSpecific,
										   void ( *copyString )( char *dest, const char *src ) );

/* forward: g_main_mp.c */
void            Com_Printf( const char *fmt, ... );
extern vmCvar_t g_debugDamage;

/* forward: g_cmds_mp.c */
void            DeathmatchScoreboardMessage( gentity_t *ent );

/* forward: g_scr_main_mp.c -- the script callbacks this unit raises */
void            Scr_PlayerDamage( gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
								  int damage, int dflags, int mod, int weapon, const vec3_t point,
								  const vec3_t dir, int hitLoc );
void            Scr_PlayerKilled( gentity_t *self, gentity_t *inflictor, gentity_t *attacker,
								  int damage, int mod, int weapon, const float *dir, int hitLoc );

/*
============================================================================

HIT LOCATIONS AND MEANS OF DEATH

============================================================================
*/

// also the keys the hitloc damage multipliers are parsed from
static const char *hitLocationNames[HITLOC_NUM] = {
	"none",
	"helmet",
	"head",
	"neck",
	"torso_upper",
	"torso_lower",
	"right_arm_upper",
	"left_arm_upper",
	"right_arm_lower",
	"left_arm_lower",
	"right_hand",
	"left_hand",
	"right_leg_upper",
	"left_leg_upper",
	"right_leg_lower",
	"left_leg_lower",
	"right_foot",
	"left_foot",
	"gun"
};

float g_fHitLocDamageMult[HITLOC_NUM];
unsigned short hitLocationConstStrings[HITLOC_NUM];

/*
==================
G_HitLocStrcpy

The string-copy hook ParseConfigStringToStruct calls back for string fields;
the hitloc table has none, so this only exists to fill the slot.
==================
*/
static void G_HitLocStrcpy( char *dest, const char *src ) {
	strcpy( dest, src );
}

/*
==================
G_ParseHitLocDmgTable
==================
*/
void G_ParseHitLocDmgTable( void ) {
	parseField_t fields[HITLOC_NUM];
	char buffer[8192];
	const char  *path = "info/mp_lochit_dmgtable";
	const char  *magic = "LOCDMGTABLE";
	fileHandle_t f;
	int len;
	int magicLen;
	int i;

	magicLen = strlen( magic );

	for ( i = 0 ; i < HITLOC_NUM ; i++ ) {
		g_fHitLocDamageMult[i] = 1.0;
		fields[i].key = hitLocationNames[i];
		fields[i].ofs = i * sizeof( float );
		fields[i].type = PARSE_FIELD_FLOAT;
		hitLocationConstStrings[i] = Scr_AllocString( hitLocationNames[i], 1 );
	}

	g_fHitLocDamageMult[HITLOC_GUN] = 0;

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( len <= 0 ) {
		Com_Error( ERR_DROP, "\x15" "Could not load hitloc damage table %s\n", path );
		return;
	}

	// note: a file shorter than the magic is not caught here
	trap_FS_Read( buffer, magicLen, f );
	buffer[magicLen] = 0;
	if ( strncmp( buffer, magic, magicLen ) ) {
		Com_Error( ERR_DROP, "\x15" "\"%s\" does not appear to be a hitloc damage table\n", path );
		return;
	}

	len -= magicLen;
	if ( len >= 8192 ) {
		Com_Error( ERR_DROP, "\x15" "\"%s\" Is too long of a hitloc damage table to parse\n", path );
		return;
	}

	trap_FS_Read( buffer, len, f );
	// keep: matches original codegen
	*( buffer + len ) = 0;
	trap_FS_FCloseFile( f );

	if ( !Info_Validate( buffer ) ) {
		Com_Error( ERR_DROP, "\x15" "\"%s\" is not a valid hitloc damage table\n", path );
		return;
	}

	if ( !ParseConfigStringToStruct( fields, g_fHitLocDamageMult, HITLOC_NUM, buffer,
									 0, NULL, G_HitLocStrcpy ) ) {
		G_Error( "Error parsing hitloc damage table %s\n", path );
	}
}

// hit location priorities trap_LocationalTrace picks the winning part with;
// the rifle map weights the head and torso far above the limbs
// keep: original data layout
byte bulletPriorityMap[HITLOC_NUM] = {
	1,
	3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3,
	0
};

byte riflePriorityMap[HITLOC_NUM] = {
	1,
	9, 9, 9, 8, 7, 6, 6, 6, 6,
	5, 5, 4, 4, 4, 4, 3, 3,
	0
};

// means of death names, as the script sees them
const char *modNames[MOD_NUM_MODS] = {
	"MOD_UNKNOWN",
	"MOD_PISTOL_BULLET",
	"MOD_RIFLE_BULLET",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_PROJECTILE",
	"MOD_PROJECTILE_SPLASH",
	"MOD_MELEE",
	"MOD_HEAD_SHOT",
	"MOD_MORTAR",
	"MOD_MORTAR_SPLASH",
	"MOD_KICKED",
	"MOD_GRABBER",
	"MOD_DYNAMITE",
	"MOD_DYNAMITE_SPLASH",
	"MOD_AIRSTRIKE",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TRIGGER_HURT",
	"MOD_EXPLOSIVE"
};

/*
=================
AddScore

Multiplayer scoring is done by the script.
=================
*/
void AddScore( gentity_t *ent, int score ) {
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t dir;

	if ( attacker && attacker != self ) {
		VectorSubtract( attacker->r.currentOrigin, self->r.currentOrigin, dir );
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract( inflictor->r.currentOrigin, self->r.currentOrigin, dir );
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->r.currentAngles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw( dir );
}

/*
==================
G_IndexForMeansOfDeath
==================
*/
int G_IndexForMeansOfDeath( const char *name ) {
	int i;

	for ( i = 0 ; i < MOD_NUM_MODS ; i++ ) {
		if ( !Q_stricmp( name, modNames[i] ) ) {
			return i;
		}
	}

	Com_Printf( "Unknown means of death string '%s'\n", name );
	return MOD_UNKNOWN;
}

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage,
				 int mod, int weapon, const float *dir, int hitLoc ) {
	gentity_t   *turret;
	gclient_t   *client;
	vec3_t start, throwDir;
	int i;

	if ( self->client->ps.pm_type >= PM_DEAD ) {
		return;
	}

	Scr_AddEntity( attacker );
	Scr_Notify( self, scr_const.death, 1 );

	// a kill from a mounted turret is credited to the turret's weapon
	if ( weapon && attacker->client ) {
		if ( attacker->client->ps.eFlags & EF_TURRET_ACTIVE ) {
			turret = &g_entities[attacker->s.otherEntityNum];
			if ( turret->s.eType == ET_TURRET ) {
				weapon = turret->s.weapon;
			}
		}
	}

	self->enemy = attacker;

	// a cooking grenade goes off where the player fell
	if ( self->client->ps.grenadeTimeLeft ) {
		throwDir[0] = crandom();
		throwDir[1] = crandom();
		throwDir[2] = random();
		VectorScale( throwDir, 160, throwDir );

		VectorCopy( self->r.currentOrigin, start );
		start[2] += 40;

		fire_grenade( self, start, throwDir, self->s.weapon );
	}

	client = self->client;
	BG_AnimScriptEvent( &client->ps, ANIM_ET_DEATH, qfalse, qtrue );

	G_AddEvent( self, EV_DEATH, 0 );

	Scr_PlayerKilled( self, inflictor, attacker, damage, mod, weapon, dir, hitLoc );

	// refresh the scoreboard of everyone spectating the dead player
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.connected != CON_CONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionState != STATE_SPECTATOR ) {
			continue;
		}
		if ( level.clients[i].spectatorClient != self->s.number ) {
			continue;
		}
		DeathmatchScoreboardMessage( &g_entities[i] );
	}

	self->takedamage = qtrue;               // can still be gibbed
	self->r.contents = CONTENTS_CORPSE;
	self->r.currentAngles[ROLL] = 0;

	LookAtKiller( self, inflictor, attacker );

	VectorCopy( self->r.currentAngles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	trap_UnlinkEntity( self );
	self->r.maxs[2] = 30;
	trap_LinkEntity( self );

	self->health = 0;
	self->die = NULL;

	trap_LinkEntity( self );
}

/*
============
G_DamageClient

Everything a client takes goes through the script: the hit location scales the
damage, then Callback_PlayerDamage decides what to do with it.
============
*/
void G_DamageClient( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec3_t dir,
					 const vec3_t point, int damage, int dflags, int mod, int hitLoc ) {
	int weapon;

	if ( !targ->takedamage ) {
		return;
	}
	if ( targ->client->noclip || targ->client->ufo ) {
		return;
	}
	if ( targ->client->sess.connected != CON_CONNECTED ) {
		return;
	}

	damage = (int)( damage * g_fHitLocDamageMult[hitLoc] );

	if ( inflictor ) {
		weapon = inflictor->s.weapon;
	} else if ( attacker ) {
		weapon = attacker->s.weapon;
	} else {
		weapon = 0;
	}

	Scr_PlayerDamage( targ, inflictor, attacker, damage, dflags, mod, weapon, point, dir, hitLoc );
}

/*
============
G_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
damage		amount of damage being inflicted
dflags		passed on to the script as iDFlags
mod			means of death
hitLoc		hit location, scales the damage taken by clients

inflictor, attacker, dir, and point can be NULL for environmental effects
============
*/
void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec3_t dir,
			   const vec3_t point, int damage, int dflags, int mod, int hitLoc ) {
	vec3_t normalizedDir;
	int take;

	if ( targ->client ) {
		G_DamageClient( targ, inflictor, attacker, dir, point, damage, dflags, mod, hitLoc );
		return;
	}

	if ( !targ->takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER ) {
		if ( targ->use && !targ->moverState ) {
			Scr_AddEntity( attacker );
			Scr_Notify( targ, scr_const.trigger, 1 );
			targ->use( targ, inflictor, attacker );
		}
		return;
	}

	// note: done ahead of the godmode test, even for a NULL dir
	VectorNormalize2( dir, normalizedDir );

	// check for godmode
	if ( targ->flags & FL_GODMODE ) {
		return;
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;

	if ( g_debugDamage.integer ) {
		G_Printf( "target:%i health:%i damage:%i\n", targ->s.number, targ->health, take );
	}

	// do the damage
	targ->health -= take;

	Scr_AddEntity( attacker );
	Scr_AddInt( take );
	Scr_Notify( targ, scr_const.damage, 2 );

	if ( targ->health <= 0 ) {
		if ( targ->health < GIB_HEALTH ) {
			targ->health = GIB_HEALTH;
		}

		Scr_AddEntity( attacker );
		Scr_Notify( targ, scr_const.death, 1 );

		targ->enemy = attacker;
		if ( targ->die ) {
			targ->die( targ, inflictor, attacker, take, mod, inflictor->s.weapon,
					   normalizedDir, hitLoc );
		}
	} else if ( targ->pain ) {
		if ( dir ) {
			VectorCopy( normalizedDir, targ->rotate );
			VectorCopy( point, targ->pos3 );
		} else {
			VectorClear( targ->rotate );
			VectorClear( targ->pos3 );
		}
		targ->pain( targ, attacker, take, point, mod, normalizedDir, hitLoc );
	}
}

/*
============
CanDamage

Returns the fraction of the target an explosion at origin can see.  A client
is sampled at five points spread across its body, so it can be partly covered;
anything else is all or nothing.
============
*/
float CanDamage( gentity_t *targ, const vec3_t origin ) {
	vec3_t dest[5];
	vec3_t dir, perp;
	vec3_t eyePos;
	trace_t tr;
	float halfHeight;
	float side;
	int hits;
	int i;

	if ( targ->client ) {
		VectorCopy( targ->r.currentOrigin, eyePos );
		eyePos[2] += targ->client->ps.viewHeightCurrent;
		G_AddLean( targ, eyePos );

		halfHeight = ( eyePos[2] - targ->r.currentOrigin[2] ) * 0.5f;

		dir[0] = origin[0] - targ->r.currentOrigin[0];
		dir[1] = origin[1] - targ->r.currentOrigin[1];
		dir[2] = 0;
		VectorNormalize( dir );

		perp[0] = -dir[1];
		perp[1] = dir[0];
		perp[2] = dir[2];

		VectorAdd( eyePos, targ->r.currentOrigin, dest[0] );
		VectorScale( dest[0], 0.5f, dest[0] );

		// keep: matches original codegen
		side = 15;
		VectorMA( dest[0], side, perp, dest[1] );
		dest[1][2] += halfHeight;
		VectorMA( dest[0], side, perp, dest[2] );
		dest[2][2] -= halfHeight;
		side = -15;
		VectorMA( dest[0], side, perp, dest[3] );
		dest[3][2] += halfHeight;
		VectorMA( dest[0], side, perp, dest[4] );
		dest[4][2] -= halfHeight;

		hits = 0;
		for ( i = 0 ; i < 5 ; i++ ) {
			trap_LocationalTrace( &tr, dest[i], origin, targ->s.number, MASK_CANDAMAGE,
								  bulletPriorityMap );
			if ( tr.fraction == 1.0 ) {
				hits++;
			}
		}

		if ( !hits ) {
			return 0;
		}
		if ( hits <= 3 ) {
			return hits * 0.33333334f;
		}
		return 1.0;
	}

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, dest[0] );
	VectorScale( dest[0], 0.5f, dest[0] );

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy( dest[0], dest[1] );
	dest[1][0] += 15;
	dest[1][1] += 15;
	VectorCopy( dest[0], dest[2] );
	dest[2][0] += 15;
	dest[2][1] -= 15;
	VectorCopy( dest[0], dest[3] );
	dest[3][0] -= 15;
	dest[3][1] += 15;
	VectorCopy( dest[0], dest[4] );
	dest[4][0] -= 15;
	dest[4][1] -= 15;

	for ( i = 0 ; i < 5 ; i++ ) {
		trap_LocationalTrace( &tr, dest[i], origin, targ->s.number, MASK_CANDAMAGE,
							  bulletPriorityMap );
		if ( tr.fraction == 1.0 ) {
			return 1.0;
		}
	}

	return 0;
}

/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( const vec3_t origin, gentity_t *inflictor, gentity_t *attacker,
						 float damage, float minDamage, float radius, gentity_t *ignore,
						 int mod ) {
	float points, dist;
	gentity_t   *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	vec3_t midpoint;
	vec3_t dest;
	trace_t tr;
	qboolean hitClient;
	float visible;
	rfloat boxRadius;
	int i, e;

	hitClient = qfalse;

	if ( !attacker ) {
		return qfalse;
	}

	if ( radius < 1 ) {
		radius = 1;
	}

	// radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	boxRadius = radius * 1.4142135f;
	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - boxRadius;
		maxs[i] = origin[i] + boxRadius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES, MASK_ALL );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[e]];

		if ( ent == ignore ) {
			continue;
		}
		if ( !ent->takedamage ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		if ( !ent->r.bmodel ) {
			VectorSubtract( ent->r.currentOrigin, origin, v );
		} else {
			for ( i = 0 ; i < 3 ; i++ ) {
				if ( origin[i] < ent->r.absmin[i] ) {
					v[i] = ent->r.absmin[i] - origin[i];
				} else if ( origin[i] > ent->r.absmax[i] ) {
					v[i] = origin[i] - ent->r.absmax[i];
				} else {
					v[i] = 0;
				}
			}
		}

		// keep: matches original codegen
		dist = sqrt( v[2] * v[2] + v[1] * v[1] + v[0] * v[0] );
		if ( dist >= radius ) {
			continue;
		}
		if ( ent->client && level.radiusDamageIgnorePlayers ) {
			continue;
		}

		points = minDamage + ( 1.0f - dist / radius ) * ( damage - minDamage );

		visible = CanDamage( ent, origin );
		if ( visible > 0 ) {
			if ( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage( ent, inflictor, attacker, dir, origin, (int)( points * visible ),
					  DAMAGE_RADIUS, mod, HITLOC_NONE );
		} else {
			// no line of sight to the body: let a tenth of the damage through thin
			// geometry if the box centre is close enough
			VectorAdd( ent->r.absmin, ent->r.absmax, midpoint );
			VectorScale( midpoint, 0.5, midpoint );
			VectorCopy( midpoint, dest );

			trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE,
						MASK_SOLID );
			// keep: negated tests, so a NaN skips the entity
			if ( !( tr.fraction < 1.0 ) ) {
				continue;
			}

			VectorSubtract( dest, origin, dest );
			if ( !( sqrt( dest[2] * dest[2] + dest[1] * dest[1] + dest[0] * dest[0] ) < radius * 0.2f ) ) {
				continue;
			}

			if ( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			dir[2] += 24;
			G_Damage( ent, inflictor, attacker, dir, origin, (int)( points * 0.1f ),
					  DAMAGE_RADIUS, mod, HITLOC_NONE );
		}
	}

	return hitClient;
}

/*
==================
G_GetHitLocationString
==================
*/
unsigned short G_GetHitLocationString( int hitLoc ) {
	return hitLocationConstStrings[hitLoc];
}

/*
==================
G_GetHitLocationIndexFromString
==================
*/
int G_GetHitLocationIndexFromString( unsigned short name ) {
	int i;

	for ( i = 0 ; i < HITLOC_NUM ; i++ ) {
		if ( hitLocationConstStrings[i] == name ) {
			return i;
		}
	}

	return HITLOC_NONE;
}
