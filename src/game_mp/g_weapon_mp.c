// g_weapon_mp.c -- perform the server side effects of a weapon firing

#include "g_local.h"
#include "bg_public.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Declarations that belong in a shared header. */

#define MASK_SHOT                   0x02802031

/* forward: q_shared.c / q_math.c */
byte            DirToByte( const vec3_t dir );
void            AddLeanToPosition( vec3_t pos, float yaw, float leanFrac,
								   float rightScale, float upScale );
void            gunrandom( float *right, float *up );

/* forward: g_main_mp.c */
void            Com_DPrintf( const char *fmt, ... );
extern vmCvar_t g_debugBullets;

/* forward: g_team_mp.c */
qboolean        OnSameTeam( gentity_t *ent1, gentity_t *ent2 );

/* forward: g_trigger_mp.c */
void            G_CheckHitTriggerDamage( gentity_t *ent, const vec3_t start, const vec3_t end,
										 int damage, int mod );

/*
======================
Weapon_Melee
======================
*/
void Weapon_Melee( weaponFireInfo_t *fireInfo, gentity_t *self ) {
	vec3_t end;
	trace_t tr;
	gentity_t   *tent;
	gentity_t   *traceEnt;
	int damage;

	VectorMA( fireInfo->start, 64.0f, fireInfo->forward, end );
	trap_LocationalTrace( &tr, fireInfo->start, end, self->s.number, MASK_SHOT,
						  bulletPriorityMap );

	damage = bg_weaponInfo[self->s.weapon]->meleeDamage;
	G_CheckHitTriggerDamage( self, fireInfo->start, tr.endpos, damage, MOD_MELEE );

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}
	if ( tr.fraction == 1.0f ) {
		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	if ( traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MELEE_HIT );
	} else {
		tent = G_TempEntity( tr.endpos, EV_MELEE_MISS );
	}
	tent->s.otherEntityNum = tr.entityNum;
	tent->s.eventParm = DirToByte( tr.normal );
	tent->s.weapon = self->s.weapon;

	if ( tr.entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( !traceEnt->takedamage ) {
		return;
	}

	G_Damage( traceEnt, self, self, fireInfo->forward, tr.endpos, damage + rand() % 5,
			  0, MOD_MELEE, tr.partGroup );
}

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network transmission, but make
sure that it rounds towards a given point rather than blindly truncating.
This prevents it from truncating into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, const vec3_t to ) {
	int i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = floor( v[i] );
		} else {
			v[i] = ceil( v[i] );
		}
	}
}

/*
==============
Bullet_Endpos
	find target end position for bullet trace based on entities weapon and accuracy
==============
*/
void Bullet_Endpos( float spread, vec3_t end, const weaponFireInfo_t *fireInfo ) {
	float r, u;
	float spreadDist;
	rfloat rs, us;	// keep: matches original codegen

	// keep: parentheses match original codegen
	spreadDist = FCAST( tan( ( spread * M_PI ) * ( 1.0f / 180.0f ) ) ) * 8192.0f;
	gunrandom( &r, &u );
	rs = (double)r * spreadDist;
	us = (double)u * spreadDist;

	end[0] = fireInfo->forward[0] * 8192.0f + fireInfo->start[0];
	end[1] = fireInfo->forward[1] * 8192.0f + fireInfo->start[1];
	end[2] = fireInfo->forward[2] * 8192.0f + fireInfo->start[2];

	end[0] = rs * fireInfo->right[0] + end[0];
	end[1] = rs * fireInfo->right[1] + end[1];
	end[2] = rs * fireInfo->right[2] + end[2];

	end[0] = us * fireInfo->up[0] + end[0];
	end[1] = us * fireInfo->up[1] + end[1];
	end[2] = us * fireInfo->up[2] + end[2];
}

/*
==============
Bullet_Fire
==============
*/
void Bullet_Fire( weaponFireInfo_t *fireInfo, gentity_t *self, gentity_t *attacker,
				  float spread, int damage ) {
	vec3_t end;

	Bullet_Endpos( spread, end, fireInfo );
	Bullet_Fire_Extended( self, attacker, fireInfo->start, end, damage, 0, fireInfo, self );
}

/*
==============
Bullet_Fire_Extended
	A modified Bullet_Fire with more parameters.

	depth counts the recursions: a bullet re-enters here once for every surface
	it penetrates and, if it is a rifle round, once more after it has passed
	through a body at half damage.
==============
*/
void Bullet_Fire_Extended( gentity_t *passEnt, gentity_t *attacker, vec3_t start, vec3_t end,
						   int damage, int depth, const weaponFireInfo_t *fireInfo,
						   gentity_t *owner ) {
	trace_t tr;
	gentity_t   *tent;
	gentity_t   *traceEnt;
	vec3_t dir;
	vec3_t boxMin, boxMax;
	rfloat dot, scale;	// keep: matches original codegen
	int mod;
	int dflags;

	dflags = 0;

	if ( depth > 12 ) {
		Com_DPrintf( "Bullet_Fire_Extended: Too many resursions, bullet aborted\n" );
		return;
	}

	if ( fireInfo->weapInfo->rifleBullet ) {
		mod = MOD_RIFLE_BULLET;
		dflags = DAMAGE_PASSTHRU;
	} else {
		mod = MOD_PISTOL_BULLET;
	}

	trap_LocationalTrace( &tr, start, end, passEnt->s.number, MASK_SHOT,
						  fireInfo->weapInfo->rifleBullet ? riflePriorityMap
														  : bulletPriorityMap );

	if ( g_debugBullets.integer & 1 ) {
		tent = G_TempEntity( start, EV_RAILTRAIL );
		VectorCopy( tr.endpos, tent->s.origin2 );
		tent->s.attackerEntityNum = attacker->s.number;
	}

	G_CheckHitTriggerDamage( attacker, start, tr.endpos, damage, mod );

	traceEnt = &g_entities[tr.entityNum];

	if ( g_debugBullets.integer <= -2 ) {
		VectorCopy( traceEnt->r.currentOrigin, boxMin );
		VectorCopy( traceEnt->r.currentOrigin, boxMax );
		VectorAdd( boxMin, traceEnt->r.mins, boxMin );
		VectorAdd( boxMax, traceEnt->r.maxs, boxMax );
		tent = G_TempEntity( boxMin, EV_RAILTRAIL );
		VectorCopy( boxMax, tent->s.origin2 );
		tent->s.dmgFlags = 2;
	}

	// the impact effect is drawn along the reflected shot direction
	VectorSubtract( end, start, dir );
	VectorNormalize( dir );
	dot = DotProduct( dir, tr.normal ) * -2.0f;
	dir[0] = tr.normal[0] * dot + dir[0];
	dir[1] = tr.normal[1] * dot + dir[1];
	dir[2] = tr.normal[2] * dot + dir[2];

	if ( !( tr.surfaceFlags & SURF_SKY ) && !traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, fireInfo->weapInfo->rifleBullet ? EV_BULLET_HIT_LARGE
																		 : EV_BULLET_HIT_SMALL );
		tent->s.eventParm = DirToByte( tr.normal ) & 0xFF;
		tent->s.scale = DirToByte( dir ) & 0xFF;
		tent->s.surfType = SURF_TYPE( tr.surfaceFlags );
		tent->s.otherEntityNum = owner->s.number;
	}

	if ( tr.contents & CONTENTS_GLASS ) {
		vec3_t fwd;	// keep: fwd (not dir) and the explicit dot product match original codegen

		// come out the far side, further along the more oblique the hit was
		VectorSubtract( end, start, fwd );
		VectorNormalize( fwd );
		dot = -( ( fwd[2] * tr.normal[2] + fwd[1] * tr.normal[1] ) + fwd[0] * tr.normal[0] );
		if ( dot >= 0.125f ) {
			scale = 0.25f / dot;
		} else {
			scale = 0;
		}
		start[0] = fwd[0] * scale + tr.endpos[0];
		start[1] = fwd[1] * scale + tr.endpos[1];
		start[2] = fwd[2] * scale + tr.endpos[2];
		Bullet_Fire_Extended( passEnt, attacker, start, end, damage, depth + 1, fireInfo, owner );
		return;
	}

	if ( !traceEnt->takedamage ) {
		return;
	}

	G_Damage( traceEnt, attacker, attacker, fireInfo->forward, tr.endpos, damage, dflags,
			  mod, tr.partGroup );

	// a rifle round carries half its damage out the back of a body
	if ( traceEnt->client && ( dflags & DAMAGE_PASSTHRU ) && damage / 2 > 0 ) {
		Bullet_Fire_Extended( traceEnt, attacker, tr.endpos, end, damage / 2, depth + 1,
							  fireInfo, owner );
	}
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

/*
======================
weapon_grenadelauncher_fire
======================
*/
gentity_t *weapon_grenadelauncher_fire( gentity_t *self, int weapon,
										const weaponFireInfo_t *fireInfo ) {
	gentity_t   *grenade;
	vec3_t dir;
	rfloat dot;	// keep: matches original codegen, as does the z, y, x sum below

	dir[0] = fireInfo->weapInfo->projectileSpeed * fireInfo->forward[0];
	dir[1] = fireInfo->weapInfo->projectileSpeed * fireInfo->forward[1];
	dir[2] = fireInfo->weapInfo->projectileSpeed * fireInfo->forward[2]
			 + fireInfo->weapInfo->projectileSpeedUp;

	grenade = fire_grenade( self, fireInfo->start, dir, weapon );

	// add whatever the thrower's own velocity contributes along the throw
	VectorNormalize( dir );
	dot = dir[2] * self->client->ps.velocity[2] + dir[1] * self->client->ps.velocity[1]
		  + dir[0] * self->client->ps.velocity[0];
	grenade->s.pos.trDelta[0] = dir[0] * dot + grenade->s.pos.trDelta[0];
	grenade->s.pos.trDelta[1] = dir[1] * dot + grenade->s.pos.trDelta[1];
	grenade->s.pos.trDelta[2] = dot * dir[2] + grenade->s.pos.trDelta[2];

	return grenade;
}

/*
======================================================================

ROCKET

======================================================================
*/

/*
======================
Weapon_RocketLauncher_Fire
======================
*/
void Weapon_RocketLauncher_Fire( gentity_t *ent, const weaponFireInfo_t *fireInfo, float spread ) {
	vec3_t dir;
	vec3_t start;
	float r, u;
	float spreadDist;
	rfloat rs, us;	// keep: matches original codegen

	// keep: parentheses match original codegen
	spreadDist = FCAST( tan( ( spread * M_PI ) * ( 1.0f / 180.0f ) ) ) * 16.0f;
	gunrandom( &r, &u );
	rs = (double)r * spreadDist;
	us = (double)u * spreadDist;

	dir[0] = fireInfo->forward[0] * 16.0f;
	dir[1] = fireInfo->forward[1] * 16.0f;
	dir[2] = fireInfo->forward[2] * 16.0f;

	dir[0] = rs * fireInfo->right[0] + dir[0];
	dir[1] = rs * fireInfo->right[1] + dir[1];
	dir[2] = rs * fireInfo->right[2] + dir[2];

	dir[0] = us * fireInfo->up[0] + dir[0];
	dir[1] = us * fireInfo->up[1] + dir[1];
	dir[2] = us * fireInfo->up[2] + dir[2];

	VectorNormalize( dir );

	VectorCopy( fireInfo->start, start );
	fire_rocket( ent, start, dir );

	if ( ent->client ) {
		ent->client->ps.velocity[0] -= fireInfo->forward[0] * 64.0f;
		ent->client->ps.velocity[1] -= fireInfo->forward[1] * 64.0f;
		ent->client->ps.velocity[2] -= fireInfo->forward[2] * 64.0f;
	}
}

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if ( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if ( !target->client ) {
		return qfalse;
	}
	if ( !attacker->client ) {
		return qfalse;
	}

	if ( target->client->ps.pm_type >= PM_DEAD ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint( gentity_t *ent, vec3_t muzzlePoint ) {
	VectorCopy( ent->r.currentOrigin, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewHeightCurrent;
	AddLeanToPosition( muzzlePoint, ent->client->ps.viewangles[YAW], ent->client->ps.leanf,
					   16.0, 20.0 );

	// snap to integer coordinates for more efficient network bandwidth usage
	muzzlePoint[0] = (int)muzzlePoint[0];
	muzzlePoint[1] = (int)muzzlePoint[1];
	muzzlePoint[2] = (int)muzzlePoint[2];
}

/*
===============
CalcMuzzlePoints
===============
*/
void CalcMuzzlePoints( weaponFireInfo_t *fireInfo, gentity_t *ent ) {
	vec3_t angles;

	angles[2] = ent->client->ps.viewangles[ROLL];
	angles[0] = ent->client->fGunPitch;
	angles[1] = ent->client->fGunYaw;

	AngleVectors( angles, fireInfo->forward, fireInfo->right, fireInfo->up );
	CalcMuzzlePoint( ent, fireInfo->start );
}

/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	weaponFireInfo_t fireInfo;
	float spread;
	float spreadFrac;
	rfloat minSpread;   // keep: matches original codegen

	if ( ( ent->client->ps.eFlags & EF_TURRET_ACTIVE ) && ent->active ) {
		return;
	}

	fireInfo.weapInfo = bg_weaponInfo[ent->s.weapon];

	CalcMuzzlePoints( &fireInfo, ent );

	spreadFrac = ent->client->currentAimSpreadScale;
	if ( ent->client->ps.fWeaponPosFrac == 1.0f ) {
		spread = ( fireInfo.weapInfo->hipSpreadMax - fireInfo.weapInfo->adsSpread )
				 * spreadFrac + fireInfo.weapInfo->adsSpread;
	} else {
		minSpread = BG_GetMinSpreadForWeapon( level.time, &ent->client->ps, ent->s.weapon );
		spread = ( fireInfo.weapInfo->hipSpreadMax - minSpread ) * spreadFrac + minSpread;
	}

	if ( fireInfo.weapInfo->weaponType == WEAPTYPE_BULLET ) {
		Bullet_Fire( &fireInfo, ent, ent, spread, fireInfo.weapInfo->damage );
	} else if ( fireInfo.weapInfo->weaponType == WEAPTYPE_GRENADE ) {
		weapon_grenadelauncher_fire( ent, ent->s.weapon, &fireInfo );
	} else if ( fireInfo.weapInfo->weaponType == WEAPTYPE_PROJECTILE ) {
		Weapon_RocketLauncher_Fire( ent, &fireInfo, spread );
	} else {
		G_Error( "Unknown weapon type %i for %s\n", fireInfo.weapInfo->weaponType,
				 fireInfo.weapInfo->szInternalName );
	}
}

/*
===============
FireWeaponMelee
===============
*/
void FireWeaponMelee( gentity_t *ent ) {
	weaponFireInfo_t fireInfo;

	if ( ( ent->client->ps.eFlags & EF_TURRET_ACTIVE ) && ent->active ) {
		return;
	}

	fireInfo.weapInfo = bg_weaponInfo[ent->s.weapon];

	AngleVectors( ent->client->ps.viewangles, fireInfo.forward, fireInfo.right, fireInfo.up );
	CalcMuzzlePoint( ent, fireInfo.start );

	Weapon_Melee( &fireInfo, ent );
}
