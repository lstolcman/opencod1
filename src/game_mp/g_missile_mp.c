// g_missile_mp.c -- grenade and rocket movement, bouncing and impacts

#include "g_local.h"
#include "bg_public.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

/* Declarations that belong in a shared header. */

/* entityState_t.eFlags; EF_BOUNCE / EF_BOUNCE_HALF live in the top byte */
#define EF_BOUNCE                   0x01000000
#define EF_BOUNCE_HALF              0x02000000

/* set by fire_rocket only */
#define EF_ROCKET                   0x00010000

/* entityShared_t.svFlags a live missile is spawned with; G_ExplodeMissile ors
 * in SVF_BROADCAST again on the corpse. */
#define SVF_MISSILE                 ( SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST )

#define MASK_MISSILESHOT            0x02802091

/* forward: q_shared.c / q_math.c */
float           VectorNormalize2( const vec3_t v, vec3_t out );
float           Com_RandFloatRange( float lo, float hi );
void            vectoangles( const vec3_t value1, vec3_t angles );
byte            DirToByte( const vec3_t dir );
float           AngleSubtract( float a1, float a2 );
float           AngleNormalize360( float angle );
float           AngleNormalize180( float angle );
float           PitchOfVectorAlongYaw( float yaw, const vec3_t normal );

/* forward: g_main_mp.c */
void            G_RunThink( gentity_t *ent );

/* forward: g_trigger_mp.c */
void            G_CheckHitTriggerDamage( gentity_t *ent, const vec3_t start, const vec3_t end,
										 int damage, int mod );
void            G_GrenadeTouchTriggerDamage( gentity_t *ent, const vec3_t start, const vec3_t end,
											 int damage, int mod );

/*
================
G_MissileLandAngles

Continues the missile's angular trajectory to the moment of impact, then --
unless forceAngles is set, in which case the missile has come to rest and only
the final orientation matters -- kicks the pitch spin for the next bounce.
================
*/
void G_MissileLandAngles( gentity_t *ent, trace_t *trace, vec3_t angles, qboolean forceAngles ) {
	int impactTime;
	float landPitch;
	float pitchDelta;
	float pitchDeltaAbs;

	impactTime = level.previousTime
				 + (int)( (float)( level.time - level.previousTime ) * trace->fraction );
	BG_EvaluateTrajectory( &ent->s.apos, impactTime, angles );

	if ( trace->normal[2] > 0.1f ) {
		landPitch = PitchOfVectorAlongYaw( angles[1], trace->normal );
		pitchDelta = AngleSubtract( landPitch, angles[0] );
		pitchDeltaAbs = fabs( pitchDelta );

		if ( !forceAngles ) {
			VectorCopy( angles, ent->s.apos.trBase );
			ent->s.apos.trTime = impactTime;
			if ( pitchDeltaAbs < 80.0f ) {
				// keep: "* -1.0f" matches original codegen
				ent->s.apos.trDelta[0] = ent->s.apos.trDelta[0] * ( rand() / 32768.0f * 0.3f + 0.85f ) * -1.0f;
			} else {
				ent->s.apos.trDelta[0] *= rand() / 32768.0f * 0.3f + 0.85f;
			}
		}

		angles[0] = AngleNormalize180( angles[0] );
		if ( forceAngles || pitchDeltaAbs < 45.0f ) {
			// the half turn is added in short space, after the scale
			// keep: one expression per arm matches original codegen
			if ( fabs( angles[0] ) > 90.0 ) {
				angles[0] = ( 360.0f / 65536.0f )
							* ( (int)( landPitch * ( 65536.0f / 360.0f ) + 32768.0f ) & 65535 );
			} else {
				angles[0] = ( 360.0f / 65536.0f )
							* ( (int)( landPitch * ( 65536.0f / 360.0f ) ) & 65535 );
			}
		} else if ( pitchDeltaAbs < 80.0f ) {
			angles[0] = AngleNormalize360( angles[0] + pitchDelta * 0.25f );
		} else {
			angles[0] = AngleNormalize360( angles[0] );
		}
	} else if ( !forceAngles ) {
		ent->s.apos.trDelta[0] = AngleNormalize360( (float)( ( rand() & 0x7F ) - 0x3F )
													+ ent->s.apos.trDelta[0] );
	}
}

/*
================
G_BounceMissile

Returns qtrue when the bounce was hard enough to be worth an event.  qfalse
also means "the missile has come to rest" -- in which case its origin and
angles have already been fixed in place.
================
*/
qboolean G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t velocity;
	vec3_t delta;
	vec3_t nudge;
	vec3_t angles;
	float dot;
	rfloat speed;
	int inWater;
	int impactTime;

	inWater = trap_PointContents( ent->r.currentOrigin, -1, CONTENTS_WATER );

	impactTime = level.previousTime
				 + (int)( (float)( level.time - level.previousTime ) * trace->fraction );
	BG_EvaluateTrajectoryDelta( &ent->s.pos, impactTime, velocity );

	// reflect the velocity on the trace plane
	dot = DotProduct( velocity, trace->normal );
	VectorMA( velocity, -2 * dot, trace->normal, ent->s.pos.trDelta );

	if ( trace->normal[2] > 0.7 ) {
		ent->s.groundEntityNum = trace->entityNum;
	}

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		if ( inWater || ( trace->contents & CONTENTS_BODY ) ) {
			VectorScale( ent->s.pos.trDelta, 0.125f, ent->s.pos.trDelta );
		} else if ( ent->s.eFlags & EF_BOUNCE ) {
			// undo the reflection, damp, then put a third of it back
			VectorMA( ent->s.pos.trDelta, dot, trace->normal, ent->s.pos.trDelta );
			VectorScale( ent->s.pos.trDelta, 0.75f, ent->s.pos.trDelta );
			VectorMA( ent->s.pos.trDelta, -0.3f * dot, trace->normal, ent->s.pos.trDelta );
		} else {
			VectorScale( ent->s.pos.trDelta, 0.5f, ent->s.pos.trDelta );
		}

		// check for stop
		// keep: both lengths in this function are written out (not VectorLength) to match original codegen
		if ( trace->normal[2] > 0.7 ) {
			speed = sqrt( ent->s.pos.trDelta[0] * ent->s.pos.trDelta[0]
						  + ent->s.pos.trDelta[1] * ent->s.pos.trDelta[1]
						  + ent->s.pos.trDelta[2] * ent->s.pos.trDelta[2] );
			if ( speed < 20 ) {
				G_SetOrigin( ent, ent->r.currentOrigin );
				G_MissileLandAngles( ent, trace, angles, qtrue );
				G_SetAngle( ent, angles );
				return qfalse;
			}
		}
	}

	VectorScale( trace->normal, 0.1f, nudge );
	if ( nudge[2] > 0 ) {
		nudge[2] = 0;
	}
	VectorAdd( ent->r.currentOrigin, nudge, ent->r.currentOrigin );
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;

	G_MissileLandAngles( ent, trace, angles, qfalse );
	VectorCopy( angles, ent->s.apos.trBase );
	ent->s.apos.trTime = level.time;

	if ( inWater ) {
		return qfalse;
	}

	VectorSubtract( ent->s.pos.trDelta, velocity, delta );
	return FCAST( sqrt( delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2] ) ) > 100;
}

/*
================
G_MissileImpact
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace, vec3_t dir ) {
	gentity_t   *other;
	gentity_t   *attacker;
	vec3_t velocity;
	qboolean hitClient;

	other = &g_entities[trace->entityNum];
	hitClient = qfalse;

	// bouncing missiles that hit something they cannot hurt just bounce
	if ( !other->takedamage && ( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		if ( !G_BounceMissile( ent, trace ) ) {
			return;
		}
		if ( trace->startsolid ) {
			return;
		}
		if ( ent->classname == scr_const.WP ) {
			return;
		}
		if ( ent->classname == scr_const.flamebarrel ) {
			G_AddEvent( ent, EV_FLAMEBARREL_BOUNCE, 0 );
			return;
		}
		G_AddEvent( ent, EV_GRENADE_BOUNCE,
					SURF_TYPE( trace->surfaceFlags ) );
		return;
	}

	if ( other->takedamage ) {
		if ( ent->damage ) {
			if ( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				hitClient = qtrue;
			}

			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			// keep: VectorLength written out to match original codegen
			if ( FCAST( sqrt( velocity[0] * velocity[0] + velocity[1] * velocity[1]
							  + velocity[2] * velocity[2] ) ) == 0 ) {
				velocity[2] = 1;        // stepped on a grenade
			}

			if ( ent->r.ownerNum != ENTITYNUM_NONE ) {
				attacker = &g_entities[ent->r.ownerNum];
			} else {
				attacker = NULL;
			}

			G_Damage( other, ent, attacker, velocity, ent->r.currentOrigin, ent->damage,
					  0, ent->methodOfDeath, HITLOC_NONE );
		} else {
			G_BounceMissile( ent, trace );
			return;
		}
	}

	if ( ent->damage ) {
		if ( ent->r.ownerNum != ENTITYNUM_NONE ) {
			attacker = &g_entities[ent->r.ownerNum];
		} else {
			attacker = &g_entities[ENTITYNUM_WORLD];
		}
		G_CheckHitTriggerDamage( attacker, ent->r.currentOrigin, trace->endpos,
								 ent->damage, ent->methodOfDeath );
	}

	// a hit on flesh or on a tagged part leaves no impact mark
	hitClient = ( hitClient || trace->partName );

	G_AddEvent( ent, hitClient ? EV_ROCKET_EXPLODE_NOMARKS : EV_ROCKET_EXPLODE, DirToByte( trace->normal ) );

	ent->s.surfType = SURF_TYPE( trace->surfaceFlags );
	ent->freeAfterEvent = qtrue;
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );  // save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	if ( ent->splashDamage ) {
		G_RadiusDamage( trace->endpos, ent, ent->parent, ent->splashDamage,
						ent->splashMinDamage, ent->splashRadius, other,
						ent->splashMethodOfDeath );
	}

	trap_LinkEntity( ent );
}

/*
==============
Concussive_think
==============
*/
void Concussive_think( gentity_t *ent ) {
	if ( level.time > ent->delay ) {
		ent->think = G_FreeEntity;
	}
	ent->nextthink = level.time + 100;
}

/*
==============
Concussive_fx
	shake the player
	caused by explosives (grenades/dynamite/etc.)
==============
*/
gentity_t *Concussive_fx( const vec3_t origin ) {
	gentity_t   *concussive;

	concussive = G_Spawn();
	VectorCopy( origin, concussive->r.currentOrigin );
	concussive->think = Concussive_think;
	concussive->nextthink = level.time + 100;
	concussive->delay = level.time + 500.0f;

	return concussive;
}

/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t origin;
	vec3_t end;
	trace_t tr;
	int dirByte;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	origin[0] = (int)origin[0];
	origin[1] = (int)origin[1];
	origin[2] = (int)origin[2];

	G_SetOrigin( ent, origin );
	ent->s.eType = ET_GENERAL;
	ent->s.eFlags |= EF_NODRAW;
	ent->flags |= FL_NODRAW;
	ent->r.svFlags |= SVF_BROADCAST;

	if ( ent->classname == scr_const.flamebarrel ) {
		ent->freeAfterEvent = qtrue;
		trap_LinkEntity( ent );
		return;
	}

	// find the surface underneath so the explosion mark picks the right decal
	VectorCopy( origin, end );
	end[2] -= 16;
	trap_Trace( &tr, ent->r.currentOrigin, vec3_origin, vec3_origin, end, ent->s.number, MASK_SOLID );

	dirByte = DirToByte( tr.normal );
	G_AddEvent( ent, EV_GRENADE_EXPLODE, dirByte );

	if ( trap_PointContents( ent->r.currentOrigin, -1, CONTENTS_WATER ) ) {
		ent->s.surfType = SURF_TYPE_WATER;
	} else {
		ent->s.surfType = SURF_TYPE( tr.surfaceFlags );
	}

	ent->freeAfterEvent = qtrue;

	if ( ent->splashDamage ) {
		G_RadiusDamage( ent->r.currentOrigin, ent, ent->parent, ent->splashDamage, ent->splashMinDamage,
						ent->splashRadius, ent, ent->splashMethodOfDeath );
	}

	trap_LinkEntity( ent );

	Concussive_fx( ent->r.currentOrigin );
}

/*
================
G_MissileDie
================
*/
void G_MissileDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage,
				   int mod, int weapon, const float *dir, int hitLoc ) {
	if ( attacker != self ) {
		self->takedamage = qfalse;
		self->think = G_ExplodeMissile;
		self->nextthink = level.time + 10;
	}
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t origin;
	vec3_t end;
	vec3_t dir;
	trace_t tr;
	gentity_t   *tent;
	int hitNum;
	int contents;

	VectorCopy( ent->r.currentOrigin, origin );

	// get the current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, end );

	VectorSubtract( end, ent->r.currentOrigin, dir );
	if ( VectorNormalize( dir ) < 0.001f ) {
		G_RunThink( ent );
		return;
	}

	// a fast-falling missile also collides with the water surface, so it can
	// raise a splash before it carries on underneath
	if ( fabs( ent->s.pos.trDelta[2] ) > 30.0
		 && !trap_PointContents( ent->r.currentOrigin, -1, CONTENTS_WATER ) ) {
		trap_LocationalTrace( &tr, ent->r.currentOrigin, end, ent->r.ownerNum,
							  ent->clipmask | CONTENTS_WATER, bulletPriorityMap );
	} else {
		trap_LocationalTrace( &tr, ent->r.currentOrigin, end, ent->r.ownerNum,
							  ent->clipmask, bulletPriorityMap );
	}

	if ( ( tr.surfaceFlags & ( SURF_TYPE_MASK << SURF_TYPE_SHIFT ) )
		 == ( SURF_TYPE_WATER << SURF_TYPE_SHIFT ) ) {
		VectorNormalize2( ent->s.pos.trDelta, dir );
		if ( dir[2] < 0 ) {
			dir[2] *= -1.0f;
		}

		tent = G_TempEntity( ent->r.currentOrigin, EV_BULLET_HIT_SMALL );
		tent->s.eventParm = DirToByte( tr.normal );
		tent->s.scale = DirToByte( dir );
		tent->s.surfType = SURF_TYPE( tr.surfaceFlags );
		tent->s.otherEntityNum = ent->s.number;

		// and now for real
		trap_LocationalTrace( &tr, ent->r.currentOrigin, end, ent->r.ownerNum,
							  ent->clipmask, bulletPriorityMap );
	}

	// a grenade is allowed through anything flagged for it
	if ( ent->methodOfDeath == MOD_GRENADE ) {
		hitNum = tr.entityNum;
		if ( g_entities[hitNum].flags & FL_NO_GRENADE_BOUNCE ) {
			contents = g_entities[hitNum].r.contents;
			g_entities[hitNum].r.contents = 0;
			trap_LocationalTrace( &tr, ent->r.currentOrigin, end, ent->r.ownerNum,
								  ent->clipmask, bulletPriorityMap );
			g_entities[hitNum].r.contents = contents;
		}
	}

	VectorCopy( tr.endpos, ent->r.currentOrigin );
	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	// a resting bouncer is dropped onto whatever is under it
	if ( ( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) )
		 && ( tr.fraction == 1.0f || ( tr.fraction < 1.0f && tr.normal[2] > 0.7f ) ) ) {
		VectorCopy( ent->r.currentOrigin, end );
		end[2] -= 1.5f;
		trap_LocationalTrace( &tr, ent->r.currentOrigin, end, ent->r.ownerNum,
							  ent->clipmask, bulletPriorityMap );
		if ( tr.fraction != 1.0f ) {
			ent->s.pos.trBase[2] += ( tr.endpos[2] + 1.5f ) - ent->r.currentOrigin[2];
			ent->r.currentOrigin[0] = tr.endpos[0];
			ent->r.currentOrigin[1] = tr.endpos[1];
			ent->r.currentOrigin[2] = tr.endpos[2] + 1.5f;
		}
	}

	trap_LinkEntity( ent );

	if ( ent->methodOfDeath == MOD_GRENADE ) {
		G_GrenadeTouchTriggerDamage( ent, origin, ent->r.currentOrigin, ent->splashDamage,
									 MOD_GRENADE );
	}

	// keep: the trDelta length below is written out (not VectorLength) to match original codegen
	if ( tr.fraction != 1.0f ) {
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			G_FreeEntity( ent );
			return;
		}

		G_MissileImpact( ent, &tr, dir );
		if ( ent->s.eType != ET_MISSILE ) {
			// the missile exploded and turned into something else
			return;
		}
	} else if ( FCAST( sqrt( ent->s.pos.trDelta[0] * ent->s.pos.trDelta[0]
							 + ent->s.pos.trDelta[1] * ent->s.pos.trDelta[1]
							 + ent->s.pos.trDelta[2] * ent->s.pos.trDelta[2] ) ) != 0 ) {
		ent->s.groundEntityNum = ENTITYNUM_NONE;
	}

	G_RunThink( ent );
}

/*
=================
DynaSink

Disable collision while the spent missile sinks, then schedule its removal
once its timestamp has elapsed.
=================
*/
void DynaSink( gentity_t *ent ) {
	ent->clipmask = 0;
	ent->r.contents = 0;
	if ( ent->timestamp < level.time ) {
		ent->think = G_FreeEntity;
		ent->nextthink = level.time + 100;
		return;
	}
	ent->s.pos.trBase[2] -= 0.5f;
	ent->nextthink = level.time + 50;
}

/*
=================
fire_grenade

	NOTE!!!! NOTE!!!!!

	This accepts a /non-normalized/ direction vector to allow specification
	of how hard it's thrown.  Please scale the vector before calling.

=================
*/
gentity_t *fire_grenade( gentity_t *self, const vec3_t start, const vec3_t dir, int weapon ) {
	gentity_t   *grenade;
	const weaponInfo_t  *weapInfo;

	grenade = G_Spawn();

	if ( self->client && self->client->ps.grenadeTimeLeft ) {
		grenade->nextthink = level.time + self->client->ps.grenadeTimeLeft;
	} else {
		grenade->nextthink = level.time + 2500;
	}

	if ( self->client ) {
		self->client->ps.grenadeTimeLeft = 0;       // reset the grenade timer
	}

	grenade->s.weapon = weapon;
	grenade->think = G_ExplodeMissile;
	grenade->s.eType = ET_MISSILE;
	grenade->r.svFlags = SVF_MISSILE;
	grenade->r.ownerNum = self->s.number;
	grenade->parent = self;

	weapInfo = bg_weaponInfo[weapon];
	Scr_SetString( &grenade->classname, scr_const.grenade );

	grenade->damage = weapInfo->damage;
	grenade->splashDamage = weapInfo->explosionInnerDamage;
	grenade->splashMinDamage = weapInfo->explosionOuterDamage;
	grenade->splashRadius = weapInfo->explosionRadius;
	grenade->methodOfDeath = MOD_GRENADE;
	grenade->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	grenade->s.eFlags = EF_BOUNCE | EF_BOUNCE_HALF;
	grenade->clipmask = MASK_MISSILESHOT;

	grenade->s.pos.trType = TR_GRAVITY;
	grenade->s.pos.trTime = level.time;
	VectorCopy( start, grenade->s.pos.trBase );
	VectorCopy( dir, grenade->s.pos.trDelta );
	grenade->s.pos.trDelta[0] = (int)grenade->s.pos.trDelta[0];
	grenade->s.pos.trDelta[1] = (int)grenade->s.pos.trDelta[1];
	grenade->s.pos.trDelta[2] = (int)grenade->s.pos.trDelta[2];

	grenade->s.apos.trType = TR_LINEAR;
	grenade->s.apos.trTime = level.time;
	vectoangles( dir, grenade->s.apos.trBase );
	grenade->s.apos.trBase[0] = AngleNormalize360( grenade->s.apos.trBase[0] - 120 );
	grenade->s.apos.trDelta[0] = Com_RandFloatRange( -45, 45 ) + 720;
	grenade->s.apos.trDelta[1] = 0;
	grenade->s.apos.trDelta[2] = Com_RandFloatRange( -45, 45 ) + 360;

	VectorCopy( start, grenade->r.currentOrigin );
	VectorCopy( grenade->s.apos.trBase, grenade->r.currentAngles );

	return grenade;
}

/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket( gentity_t *self, const vec3_t start, vec3_t dir ) {
	gentity_t   *bolt;
	const weaponInfo_t  *weapInfo;

	VectorNormalize( dir );

	weapInfo = bg_weaponInfo[self->s.weapon];

	bolt = G_Spawn();
	Scr_SetString( &bolt->classname, scr_const.rocket );
	bolt->nextthink = level.time + 30000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_MISSILE;
	bolt->s.eFlags |= EF_ROCKET;
	bolt->s.weapon = self->s.weapon;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = weapInfo->damage;
	bolt->splashDamage = weapInfo->explosionInnerDamage;
	bolt->splashMinDamage = weapInfo->explosionOuterDamage;
	bolt->splashRadius = weapInfo->explosionRadius;
	bolt->methodOfDeath = MOD_PROJECTILE;
	bolt->splashMethodOfDeath = MOD_PROJECTILE_SPLASH;
	bolt->clipmask = MASK_MISSILESHOT;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - 50;       // move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, weapInfo->projectileSpeed, bolt->s.pos.trDelta );
	bolt->s.pos.trDelta[0] = (int)bolt->s.pos.trDelta[0];
	bolt->s.pos.trDelta[1] = (int)bolt->s.pos.trDelta[1];
	bolt->s.pos.trDelta[2] = (int)bolt->s.pos.trDelta[2];

	VectorCopy( start, bolt->r.currentOrigin );

	vectoangles( bolt->s.pos.trDelta, bolt->r.currentAngles );
	G_SetAngle( bolt, bolt->r.currentAngles );

	return bolt;
}
