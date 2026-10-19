// g_client_script_cmd_mp.c -- script methods that can be called on players

#include <string.h>

#include "g_local.h"

#define EF_CLONE_TEMP               0x00000800  // player clone, cleared by BodyEnd
#define EF_PING                     0x00080000  // show on the compass

// Scr_ConstructMessageString message types
#define MSGTYPE_CLIENT_CVAR_VALUE   4
#define MSGTYPE_CLIENT_CHAT         5

#define MSGCHAR_LOCALIZED           20          // starts a localized string reference

// bg_weapon.c
// keep: the (byte) casts on the weapon index at the call sites match original codegen
int BG_GetWeaponIndexForName( const char *name );
int BG_GetWeaponSlotForName( const char *name );

// q_math.c
byte DirToByte( const vec3_t dir );

// g_main_mp.c
extern vmCvar_t g_knockback;

// g_client_mp.c
void ClientSpawn( gentity_t *ent, const vec3_t origin, const vec3_t angles );

/*
==============
GetPlayerEntity

Makes sure the method was called on a player.  Scr_Error does not return.
Kept as a macro so it expands at every call site.
==============
*/
#define GET_PLAYER_ENTITY( ent, entnum ) \
	do { \
		if ( ( entnum ) < MAX_GENTITIES ) { \
			( ent ) = &g_entities[( entnum )]; \
			if ( !( ent )->client ) { \
				Scr_Error( va( "entity %i is not a player", ( entnum ) ) ); \
			} \
		} else { \
			Scr_Error( va( "%i is not a valid entity number", ( entnum ) ) ); \
			( ent ) = NULL; \
		} \
	} while ( 0 )

/*
==============
PlayerCmd_giveWeapon

Give the player a weapon along with its starting ammo
==============
*/
void PlayerCmd_giveWeapon( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;
	qboolean hadWeapon;
	int count;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	hadWeapon = ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) != 0;

	if ( !BG_GetEmptySlotForWeapon( &ent->client->ps, weapon ) ) {
		Scr_ParamError( 0, "Can not give player weapon without having an empty weapon slot\n" );
	}

	BG_GivePlayerWeapon( &ent->client->ps, weapon );

	count = bg_weaponInfo[weapon]->startAmmo
			- ent->client->ps.ammo[bg_weaponInfo[weapon]->ammoIndex];
	if ( count > 0 ) {
		Add_Ammo( ent, weapon, count, !hadWeapon );
	}
}

/*
==============
PlayerCmd_takeWeapon

Take a weapon and its ammo away from the player
==============
*/
void PlayerCmd_takeWeapon( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	ent->client->ps.ammo[bg_weaponInfo[weapon]->ammoIndex] = 0;
	ent->client->ps.ammoclip[bg_weaponInfo[weapon]->clipIndex] = 0;

	BG_TakePlayerWeapon( &ent->client->ps, weapon );
}

/*
==============
PlayerCmd_takeAllWeapons

Take every weapon and all ammo away from the player
==============
*/
void PlayerCmd_takeAllWeapons( unsigned int entnum ) {
	gentity_t   *ent;
	int i;

	GET_PLAYER_ENTITY( ent, entnum );

	ent->client->ps.weapon = 0;

	for ( i = 1; i <= bg_numWeapons; i++ ) {
		ent->client->ps.ammo[bg_weaponInfo[i]->ammoIndex] = 0;
		ent->client->ps.ammoclip[bg_weaponInfo[i]->clipIndex] = 0;
		BG_TakePlayerWeapon( &ent->client->ps, i );
	}
}

/*
==============
PlayerCmd_getCurrentWeapon

Returns the name of the weapon in hand, or "none"
==============
*/
void PlayerCmd_getCurrentWeapon( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( (int)ent->client->ps.weapon > 0 ) {
		Scr_AddString( bg_weaponInfo[ent->client->ps.weapon]->szInternalName );
	} else {
		Scr_AddString( "none" );
	}
}

/*
==============
PlayerCmd_hasWeapon

Returns true if the player has the named weapon
==============
*/
void PlayerCmd_hasWeapon( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	if ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		Scr_AddBool( qtrue );
	} else {
		Scr_AddBool( qfalse );
	}
}

/*
==============
PlayerCmd_switchToWeapon

Switch to the named weapon if the player has it, returns true if so
==============
*/
void PlayerCmd_switchToWeapon( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	if ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		trap_SendServerCommand( ent - g_entities, 1, va( "a %i", weapon ) );
		Scr_AddBool( qtrue );
	} else {
		Scr_AddBool( qfalse );
	}
}

/*
==============
PlayerCmd_giveStartAmmo

Top up the named weapon's ammo to its starting amount
==============
*/
void PlayerCmd_giveStartAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;
	int count;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	if ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		count = bg_weaponInfo[weapon]->startAmmo
				- ent->client->ps.ammo[bg_weaponInfo[weapon]->ammoIndex];
		if ( count > 0 ) {
			Add_Ammo( ent, weapon, count, qfalse );
		}
	}
}

/*
==============
PlayerCmd_giveMaxAmmo

Top up the named weapon's ammo to the maximum for its ammo type
==============
*/
void PlayerCmd_giveMaxAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;
	const weaponInfo_t  *weapInfo;
	int count;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	if ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		weapInfo = bg_weaponInfo[weapon];
		count = bg_ammoTypeMax[weapInfo->ammoIndex]
				- ent->client->ps.ammo[weapInfo->ammoIndex];
		if ( count > 0 ) {
			Add_Ammo( ent, weapon, count, qfalse );
		}
	}
}

/*
==============
PlayerCmd_getFractionStartAmmo

Returns the named weapon's ammo as a fraction of its starting amount
==============
*/
void PlayerCmd_getFractionStartAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;
	int ammo;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	if ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		if ( bg_weaponInfo[weapon]->startAmmo < 1 ) {
			Scr_AddFloat( 1.0 );
			return;
		}

		ammo = ent->client->ps.ammo[bg_weaponInfo[weapon]->ammoIndex];
		if ( ammo < 1 ) {
			Scr_AddFloat( 0 );
			return;
		}

		Scr_AddFloat( (float)ammo / (float)bg_weaponInfo[weapon]->startAmmo );
	} else {
		Scr_AddFloat( 1.0 );
	}
}

/*
==============
PlayerCmd_getFractionMaxAmmo

Returns the named weapon's ammo as a fraction of the maximum
==============
*/
void PlayerCmd_getFractionMaxAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;
	int ammoIndex;
	int ammo;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	if ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		ammoIndex = bg_weaponInfo[weapon]->ammoIndex;
		if ( bg_ammoTypeMax[ammoIndex] < 1 ) {
			Scr_AddFloat( 1.0 );
			return;
		}

		ammo = ent->client->ps.ammo[ammoIndex];
		if ( ammo < 1 ) {
			Scr_AddFloat( 0 );
			return;
		}

		Scr_AddFloat( (float)ammo / (float)bg_ammoTypeMax[ammoIndex] );
	} else {
		Scr_AddFloat( 1.0 );
	}
}

/*
==============
PlayerCmd_setOrigin

Move the player without lerping
==============
*/
void PlayerCmd_setOrigin( unsigned int entnum ) {
	gentity_t   *ent;
	vec3_t origin;

	GET_PLAYER_ENTITY( ent, entnum );

	Scr_GetVector( 0, origin );

	trap_UnlinkEntity( ent );

	VectorCopy( origin, ent->client->ps.origin );
	ent->client->ps.origin[2] += 1.0f;

	/* toggle the teleport bit so the client doesn't lerp the jump */
	ent->client->ps.eFlags ^= EF_TELEPORT_BIT;

	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	trap_LinkEntity( ent );
}

/*
==============
PlayerCmd_setAngles

Set the player's view angles
==============
*/
void PlayerCmd_setAngles( unsigned int entnum ) {
	gentity_t   *ent;
	vec3_t angles;

	GET_PLAYER_ENTITY( ent, entnum );

	Scr_GetVector( 0, angles );
	SetClientViewAngle( ent, angles );
}

/*
==============
PlayerCmd_useButtonPressed

Returns 1 if the use button is held
==============
*/
void PlayerCmd_useButtonPressed( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( ent->client->buttons & BUTTON_ACTIVATE ) {
		Scr_AddInt( 1 );
	} else {
		Scr_AddInt( 0 );
	}
}

/*
==============
PlayerCmd_attackButtonPressed

Returns 1 if the attack button is held
==============
*/
void PlayerCmd_attackButtonPressed( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( ent->client->buttons & BUTTON_ATTACK ) {
		Scr_AddInt( 1 );
	} else {
		Scr_AddInt( 0 );
	}
}

/*
==============
PlayerCmd_meleeButtonPressed

Returns 1 if the melee button is held
==============
*/
void PlayerCmd_meleeButtonPressed( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( ent->client->buttons & BUTTON_MELEE ) {
		Scr_AddInt( 1 );
	} else {
		Scr_AddInt( 0 );
	}
}

/*
==============
PlayerCmd_isOnGround

Returns 1 if the player is standing on something
==============
*/
void PlayerCmd_isOnGround( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( ent->client->ps.groundEntityNum != ENTITYNUM_NONE ) {
		Scr_AddInt( 1 );
	} else {
		Scr_AddInt( 0 );
	}
}

/*
==============
PlayerCmd_pingPlayer

Show the player on the compass for three seconds
==============
*/
void PlayerCmd_pingPlayer( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	ent->client->ps.eFlags |= EF_PING;
	ent->client->pingTime = level.time + 3000;
}

/*
==============
PlayerCmd_SetViewmodel

Set the player's first person weapon model
==============
*/
void PlayerCmd_SetViewmodel( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	ent->client->sess.viewmodel = G_ModelIndex( Scr_GetString( 0 ) );
}

/*
==============
PlayerCmd_GetViewmodel

Returns the player's first person weapon model
==============
*/
void PlayerCmd_GetViewmodel( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	Scr_AddString( trap_GetConfigstringConst( CS_MODELS + ent->client->sess.viewmodel ) );
}

/*
==============
PlayerCmd_allowComplaint

Let the player file a complaint against the given player for a short time
==============
*/
void PlayerCmd_allowComplaint( unsigned int entnum ) {
	gentity_t   *ent;
	gentity_t   *other;

	GET_PLAYER_ENTITY( ent, entnum );

	other = Scr_GetEntity( 0 );
	if ( ent == other ) {
		return;
	}

	// you cannot complain about the listen server host
	if ( other->client->sess.localClient ) {
		trap_SendServerCommand( ent - g_entities, 1, "m -4" );
		return;
	}

	trap_SendServerCommand( ent - g_entities, 1, va( "m %i", other->s.number ) );
	ent->client->sess.complaintClient = other->s.clientNum;
	ent->client->sess.complaintEndTime = level.time + 20500;
}

/*
==============
PlayerCmd_showScoreboard

Send the scoreboard to the player
==============
*/
void PlayerCmd_showScoreboard( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	DeathmatchScoreboardMessage( ent );
}

/*
==============
PlayerCmd_setSpawnWeapon

Make the named weapon the one in hand, ready to fire
==============
*/
void PlayerCmd_setSpawnWeapon( unsigned int entnum ) {
	gentity_t   *ent;
	int weapon;

	GET_PLAYER_ENTITY( ent, entnum );

	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 0 ) );

	if ( ent->client->ps.weapons[weapon >> 5] & ( 1 << ( weapon & 31 ) ) ) {
		ent->client->ps.weapon = weapon;
		ent->client->ps.weaponstate = WEAPON_READY;
	}
}

/*
==============
PlayerCmd_dropItem

Drop a weapon or item, returns the dropped entity
==============
*/
void PlayerCmd_dropItem( unsigned int entnum ) {
	gentity_t   *ent;
	gentity_t   *drop;
	const char  *name;
	const char  *tag;
	int weapon;
	gitem_t     *item;

	GET_PLAYER_ENTITY( ent, entnum );

	name = Scr_GetString( 0 );
	weapon = (byte)BG_GetWeaponIndexForName( name );

	if ( weapon ) {
		if ( Scr_GetNumParam() > 1 ) {
			tag = Scr_GetString( 1 );
		} else {
			tag = "tag_weapon_right";
		}
		drop = Drop_Weapon( ent, weapon, tag );
	} else {
		item = BG_FindItem( name );
		if ( item ) {
			drop = Drop_Item( ent, item, 0, qfalse );
		} else {
			drop = NULL;
		}
	}

	if ( drop ) {
		Scr_AddEntity( drop );
	} else {
		Scr_AddUndefined();
	}
}

/*
==============
PlayerCmd_finishPlayerDamage

The second half of G_Damage for players: the script gets the damage callback
first, decides on a final amount, then calls this to apply it.
==============
*/
void PlayerCmd_finishPlayerDamage( unsigned int entnum ) {
	gentity_t   *ent;
	gentity_t   *inflictor;
	gentity_t   *attacker;
	gentity_t   *tent;
	const weaponInfo_t  *weapInfo;
	vec3_t point;
	vec3_t vDir;
	vec3_t dir;
	vec3_t kvel;
	const float *pPoint;
	const float *pDir;
	int damage;
	int dflags;
	int mod;
	int weapon;
	int hitLoc;
	int knockback;
	int t;
	float scale;

	inflictor = &g_entities[ENTITYNUM_WORLD];
	attacker = &g_entities[ENTITYNUM_WORLD];
	pDir = NULL;
	pPoint = NULL;

	GET_PLAYER_ENTITY( ent, entnum );

	damage = Scr_GetInt( 2 );
	if ( damage <= 0 ) {
		return;
	}

	// keep: the inflictor is tested on parameter 0 but read from parameter 1,
	// so it can only ever be the attacker or the world
	if ( Scr_GetType( 0 ) && Scr_GetPointerType( 0 ) == VAR_ENTITY ) {
		inflictor = Scr_GetEntity( 1 );
	}
	if ( Scr_GetType( 1 ) && Scr_GetPointerType( 1 ) == VAR_ENTITY ) {
		attacker = Scr_GetEntity( 1 );
	}

	dflags = Scr_GetInt( 3 );
	mod = G_IndexForMeansOfDeath( Scr_GetString( 4 ) );
	weapon = (byte)BG_GetWeaponIndexForName( Scr_GetString( 5 ) );

	if ( Scr_GetType( 6 ) ) {
		Scr_GetVector( 6, point );
		pPoint = point;
	}
	if ( Scr_GetType( 7 ) ) {
		Scr_GetVector( 7, vDir );
		pDir = vDir;
	}

	hitLoc = G_GetHitLocationIndexFromString( Scr_GetConstString( 8 ) );

	if ( pDir ) {
		VectorNormalize2( pDir, dir );
	} else {
		VectorClear( dir );
	}

	if ( !( ent->flags & FL_NO_KNOCKBACK ) && !( dflags & DAMAGE_NO_KNOCKBACK ) ) {
		scale = 0.3f;
		if ( ent->client->ps.pm_flags & PMF_PRONE ) {
			scale = 0.02f;
		} else if ( ent->client->ps.pm_flags & PMF_DUCKED ) {
			scale = 0.15f;
		}

		knockback = (int)( (float)damage * scale );
		if ( knockback > 60 ) {
			knockback = 60;
		}

		if ( knockback ) {
			// keep: 0.004f is 1 / 250, matches original codegen
			VectorScale( dir, (float)knockback * g_knockback.value * 0.004f, kvel );
			VectorAdd( ent->client->ps.velocity, kvel, ent->client->ps.velocity );

			// set the timer so that the other client can't cancel
			// out the movement immediately
			if ( !ent->client->ps.pm_time ) {
				t = knockback * 2;
				if ( t < 50 ) {
					t = 50;
				}
				if ( t > 200 ) {
					t = 200;
				}
				ent->client->ps.pm_time = t;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
		}
	}

	if ( ent->flags & FL_GODMODE ) {
		return;
	}

	if ( weapon ) {
		weapInfo = bg_weaponInfo[weapon];
		if ( weapInfo->weaponType == WEAPTYPE_BULLET ) {
			// the blood puff everyone but the victim sees, then the hit
			// feedback only the victim sees.  point is not set if parameter 6
			// was left out (original behavior)
			tent = G_TempEntity( point, EV_BULLET_HIT_SMALL + ( weapInfo->rifleBullet != 0 ) );
			tent->s.eventParm = DirToByte( dir ) & 0xFF;
			tent->s.scale = DirToByte( dir ) & 0xFF;
			tent->s.surfType = SURF_TYPE_FLESH;
			tent->s.otherEntityNum = attacker->s.number;
			tent->r.svFlags |= SVF_NOTSINGLECLIENT;
			tent->r.singleClient = ent->client->ps.clientNum;

			tent = G_TempEntity( point, EV_BULLET_HIT_CLIENT_SMALL
								 + ( bg_weaponInfo[weapon]->rifleBullet != 0 ) );
			tent->s.surfType = SURF_TYPE_FLESH;
			tent->s.otherEntityNum = attacker->s.number;
			tent->s.clientNum = ent->client->ps.clientNum;
			tent->r.svFlags = SVF_SINGLECLIENT;
			tent->r.singleClient = ent->client->ps.clientNum;
		}
	}

	ent->client->damage_blood += damage;

	if ( pDir ) {
		VectorCopy( dir, ent->client->damage_from );
		ent->client->damage_fromWorld = qfalse;
	} else {
		VectorCopy( ent->r.currentOrigin, ent->client->damage_from );
		ent->client->damage_fromWorld = qtrue;
	}

	ent->health -= damage;

	Scr_AddEntity( attacker );
	Scr_AddInt( damage );
	Scr_Notify( ent, scr_const.damage, 2 );

	if ( ent->health <= 0 ) {
		if ( ent->health < -999 ) {
			ent->health = -999;
		}

		ent->enemy = attacker;

		if ( ent->die ) {
			ent->die( ent, inflictor, attacker, damage, mod, weapon, dir, hitLoc );
		}

		if ( !ent->inuse ) {
			return;
		}
	} else {
		if ( ent->pain ) {
			ent->pain( ent, attacker, damage, pPoint, mod, dir, hitLoc );
		}
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;
}

/*
==============
PlayerCmd_Suicide

Kill the player
==============
*/
void PlayerCmd_Suicide( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	ent->client->ps.stats[STAT_HEALTH] = 0;

	player_die( ent, ent, ent, 100000, MOD_SUICIDE, 0, NULL, HITLOC_NONE );
}

/*
==============
PlayerCmd_OpenMenu

Open a script menu on the player's screen, returns 1 if sent
==============
*/
void PlayerCmd_OpenMenu( unsigned int entnum ) {
	gentity_t   *ent;
	int menuIndex;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( ent->client->sess.connected == CON_CONNECTED ) {
		menuIndex = GScr_GetScriptMenuIndex( Scr_GetString( 0 ) );
		trap_SendServerCommand( entnum, 1, va( "t %i", menuIndex ) );
		Scr_AddInt( 1 );
	} else {
		Scr_AddInt( 0 );
	}
}

/*
==============
PlayerCmd_OpenMenuNoMouse

Open a script menu without the mouse cursor, returns 1 if sent
==============
*/
void PlayerCmd_OpenMenuNoMouse( unsigned int entnum ) {
	gentity_t   *ent;
	int menuIndex;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( ent->client->sess.connected == CON_CONNECTED ) {
		menuIndex = GScr_GetScriptMenuIndex( Scr_GetString( 0 ) );
		trap_SendServerCommand( entnum, 1, va( "t %i 1", menuIndex ) );
		Scr_AddInt( 1 );
	} else {
		Scr_AddInt( 0 );
	}
}

/*
==============
PlayerCmd_CloseMenu

Close the player's script menu.  Does not validate the entity.
==============
*/
void PlayerCmd_CloseMenu( unsigned int entnum ) {
	trap_SendServerCommand( entnum, 1, "u" );
}

/*
==============
WeaponSlotsNotValid

The weapon slots only mean anything while the client is in the game
==============
*/
qboolean WeaponSlotsNotValid( gentity_t *ent ) {
	return ent->client->sess.sessionState != STATE_PLAYING;
}

/*
==============
PlayerCmd_GetWeaponSlotWeapon

Returns the name of the weapon in a weapon slot, or "none"
==============
*/
void PlayerCmd_GetWeaponSlotWeapon( unsigned int entnum ) {
	gentity_t   *ent;
	unsigned short name;
	int slot;
	int weapon;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( WeaponSlotsNotValid( ent ) ) {
		Scr_AddConstString( scr_const.none );
		return;
	}

	name = Scr_GetConstString( 0 );
	slot = BG_GetWeaponSlotForName( SL_ConvertToString( name ) );
	if ( !slot ) {
		Scr_ParamError( 0, va( "Unknown weaponslot name %s. Valid weaponslots are "
							   "\"primary\", \"primaryb\", \"pistol\", \"grenade\", and \"smokegrenade\"",
							   SL_ConvertToString( name ) ) );
	}

	weapon = (char)ent->client->ps.weaponslots[slot];
	if ( !weapon ) {
		Scr_AddConstString( scr_const.none );
	} else {
		Scr_AddString( bg_weaponInfo[weapon]->szInternalName );
	}
}

/*
==============
PlayerCmd_SetWeaponSlotWeapon

Put a weapon in a weapon slot, replacing the one there
==============
*/
void PlayerCmd_SetWeaponSlotWeapon( unsigned int entnum ) {
	gentity_t   *ent;
	unsigned short name;
	int slot;
	const char  *weaponName;
	int weapon;
	int oldWeapon;
	qboolean moveToPrimaryB;
	const char *actualSlotName;
	const char *requestedSlotName;

	moveToPrimaryB = qfalse;

	GET_PLAYER_ENTITY( ent, entnum );

	name = Scr_GetConstString( 0 );
	slot = BG_GetWeaponSlotForName( SL_ConvertToString( name ) );
	if ( !slot ) {
		Scr_ParamError( 0, va( "Unknown weaponslot name %s. Valid weaponslots are "
							   "\"primary\", \"primaryb\", \"pistol\", \"grenade\", and \"smokegrenade\"",
							   SL_ConvertToString( name ) ) );
	}

	weaponName = Scr_GetString( 1 );
	if ( !Q_stricmp( weaponName, "none" ) ) {
		weapon = 0;
	} else {
		struct { const weaponInfo_t *info; int slot; } descriptor;
		weapon = (byte)BG_GetWeaponIndexForName( weaponName );
		if ( !weapon ) {
			Scr_ParamError( 1, va( "Unknown weapon %s.", weaponName ) );
		}

		/* primary and primaryb hold the same class of weapon */
		descriptor.info = bg_weaponInfo[weapon];
		descriptor.slot = descriptor.info->weaponSlot;
		if ( descriptor.slot != slot
			 && ( ( descriptor.slot != WEAPSLOT_PRIMARY && descriptor.slot != WEAPSLOT_PRIMARYB )
				  || ( slot != WEAPSLOT_PRIMARY && slot != WEAPSLOT_PRIMARYB ) ) ) {
			requestedSlotName = bg_weaponSlotNames[slot];
			actualSlotName = bg_weaponSlotNames[descriptor.slot];
			Scr_ParamError( 1, va( "Weapon %s goes in the %s weaponslot, not the %s weaponslot.",
								   weaponName, actualSlotName,
								   requestedSlotName ) );
		}
	}

	oldWeapon = (char)ent->client->ps.weaponslots[slot];
	if ( oldWeapon ) {
		BG_TakePlayerWeapon( &ent->client->ps, oldWeapon );
	}

	if ( weapon ) {
		/* BG_GivePlayerWeapon fills the first free slot, so a primaryb with an
		   empty primary lands in the wrong one and has to be moved across */
		if ( slot == WEAPSLOT_PRIMARYB && !ent->client->ps.weaponslots[WEAPSLOT_PRIMARY] ) {
			moveToPrimaryB = qtrue;
		}

		BG_GivePlayerWeapon( &ent->client->ps, weapon );

		if ( moveToPrimaryB ) {
			ent->client->ps.weaponslots[WEAPSLOT_PRIMARYB] =
				ent->client->ps.weaponslots[WEAPSLOT_PRIMARY];
			ent->client->ps.weaponslots[WEAPSLOT_PRIMARY] = 0;
		}
	}
}

/*
==============
PlayerCmd_GetWeaponSlotAmmo

Returns the ammo of the weapon in a slot (the clip for clip-only weapons)
==============
*/
void PlayerCmd_GetWeaponSlotAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	unsigned short name;
	int slot;
	int weapon;
	const weaponInfo_t  *weapInfo;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( WeaponSlotsNotValid( ent ) ) {
		Scr_AddInt( 0 );
		return;
	}

	name = Scr_GetConstString( 0 );
	slot = BG_GetWeaponSlotForName( SL_ConvertToString( name ) );
	if ( !slot ) {
		Scr_ParamError( 0, va( "Unknown weaponslot name %s. Valid weaponslots are "
							   "\"primary\", \"primaryb\", \"pistol\", \"grenade\", and \"smokegrenade\"",
							   SL_ConvertToString( name ) ) );
	}

	weapon = (char)ent->client->ps.weaponslots[slot];
	if ( !weapon ) {
		Scr_AddInt( 0 );
		return;
	}

	weapInfo = bg_weaponInfo[weapon];
	if ( weapInfo->clipOnly ) {
		Scr_AddInt( ent->client->ps.ammoclip[weapInfo->clipIndex] );
	} else {
		Scr_AddInt( ent->client->ps.ammo[weapInfo->ammoIndex] );
	}
}

/*
==============
PlayerCmd_SetWeaponSlotAmmo

Set the ammo of the weapon in a slot (the clip for clip-only weapons)
==============
*/
void PlayerCmd_SetWeaponSlotAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	unsigned short name;
	int slot;
	int ammo;
	int weapon;
	const weaponInfo_t  *weapInfo;
	int index;

	GET_PLAYER_ENTITY( ent, entnum );

	name = Scr_GetConstString( 0 );
	slot = BG_GetWeaponSlotForName( SL_ConvertToString( name ) );
	if ( !slot ) {
		Scr_ParamError( 0, va( "Unknown weaponslot name %s. Valid weaponslots are "
							   "\"primary\", \"primaryb\", \"pistol\", \"grenade\", and \"smokegrenade\"",
							   SL_ConvertToString( name ) ) );
	}

	ammo = Scr_GetInt( 1 );

	weapon = (char)ent->client->ps.weaponslots[slot];
	if ( !weapon ) {
		return;
	}

	weapInfo = bg_weaponInfo[weapon];
	if ( weapInfo->clipOnly ) {
		index = weapInfo->clipIndex;
		if ( index ) {
			if ( ammo < 0 ) {
				ammo = 0;
			} else if ( ammo > bg_ammoClipSizes[index] ) {
				ammo = bg_ammoClipSizes[index];
			}
			ent->client->ps.ammoclip[index] = ammo;
		}
	} else {
		index = weapInfo->ammoIndex;
		if ( index ) {
			if ( ammo < 0 ) {
				ammo = 0;
			} else if ( ammo > bg_ammoTypeMax[index] ) {
				ammo = bg_ammoTypeMax[index];
			}
			ent->client->ps.ammo[index] = ammo;
		}
	}
}

/*
==============
PlayerCmd_GetWeaponSlotClipAmmo

Returns the ammo in the clip of the weapon in a slot
==============
*/
void PlayerCmd_GetWeaponSlotClipAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	unsigned short name;
	int slot;
	int weapon;
	int clipIndex;

	GET_PLAYER_ENTITY( ent, entnum );

	if ( WeaponSlotsNotValid( ent ) ) {
		Scr_AddInt( 0 );
		return;
	}

	name = Scr_GetConstString( 0 );
	slot = BG_GetWeaponSlotForName( SL_ConvertToString( name ) );
	if ( !slot ) {
		Scr_ParamError( 0, va( "Unknown weaponslot name %s. Valid weaponslots are "
							   "\"primary\", \"primaryb\", \"pistol\", \"grenade\", and \"smokegrenade\"",
							   SL_ConvertToString( name ) ) );
	}

	weapon = (char)ent->client->ps.weaponslots[slot];
	if ( !weapon ) {
		Scr_AddInt( 0 );
		return;
	}

	clipIndex = bg_weaponInfo[weapon]->clipIndex;
	if ( !clipIndex ) {
		Scr_AddInt( 0 );
		return;
	}

	Scr_AddInt( ent->client->ps.ammoclip[clipIndex] );
}

/*
==============
PlayerCmd_SetWeaponSlotClipAmmo

Set the ammo in the clip of the weapon in a slot.  Pushes a return value
when the slot is empty.
==============
*/
void PlayerCmd_SetWeaponSlotClipAmmo( unsigned int entnum ) {
	gentity_t   *ent;
	unsigned short name;
	int slot;
	int ammo;
	int weapon;
	int clipIndex;

	GET_PLAYER_ENTITY( ent, entnum );

	name = Scr_GetConstString( 0 );
	slot = BG_GetWeaponSlotForName( SL_ConvertToString( name ) );
	if ( !slot ) {
		Scr_ParamError( 0, va( "Unknown weaponslot name %s. Valid weaponslots are "
							   "\"primary\", \"primaryb\", \"pistol\", \"grenade\", and \"smokegrenade\"",
							   SL_ConvertToString( name ) ) );
	}

	ammo = Scr_GetInt( 1 );

	weapon = (char)ent->client->ps.weaponslots[slot];
	if ( !weapon ) {
		Scr_AddInt( 0 );
		return;
	}

	clipIndex = bg_weaponInfo[weapon]->clipIndex;
	if ( clipIndex ) {
		if ( ammo < 0 ) {
			ammo = 0;
		}
		if ( ammo > bg_ammoClipSizes[clipIndex] ) {
			ammo = bg_ammoClipSizes[clipIndex];
		}
		ent->client->ps.ammoclip[clipIndex] = ammo;
	}
}

/*
==============
iclientprintln

Print a message to the player's console
==============
*/
void iclientprintln( unsigned int entnum ) {
	Scr_MakeGameMessage( entnum, "f" );
}

/*
==============
iclientprintlnbold

Print a message in the middle of the player's screen
==============
*/
void iclientprintlnbold( unsigned int entnum ) {
	Scr_MakeGameMessage( entnum, "g" );
}

/*
==============
PlayerCmd_spawn

Spawn the player at the given origin and angles
==============
*/
void PlayerCmd_spawn( unsigned int entnum ) {
	gentity_t   *ent;
	vec3_t origin;
	vec3_t angles;

	GET_PLAYER_ENTITY( ent, entnum );

	Scr_GetVector( 0, origin );
	Scr_GetVector( 1, angles );

	ClientSpawn( ent, origin, angles );
}

/*
==============
PlayerCmd_setEnterTime

Set the time the player entered the game
==============
*/
void PlayerCmd_setEnterTime( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	ent->client->sess.enterTime = Scr_GetInt( 0 );
}

/*
==============
BodyEnd

The clone's think: drop the temp flag so the client stops drawing it
==============
*/
void BodyEnd( gentity_t *ent ) {
	ent->s.eFlags &= ~EF_CLONE_TEMP;
}

/*
==============
PlayerCmd_ClonePlayer

Leave a copy of the player's body behind, outside the body queue.
Returns the clone.
==============
*/
void PlayerCmd_ClonePlayer( unsigned int entnum ) {
	gentity_t   *ent;
	gentity_t   *body;
	gclient_t   *client;

	GET_PLAYER_ENTITY( ent, entnum );
	client = ent->client;

	body = G_SpawnPlayerClone();

	body->s.clientNum = client->ps.clientNum;
	body->s.eFlags = ( body->s.eFlags & EF_TELEPORT_BIT )
					 | ( client->ps.eFlags & ~EF_TELEPORT_BIT )
					 | EF_CLONE_TEMP;

	G_SetOrigin( body, ent->r.currentOrigin );
	G_SetAngle( body, ent->r.currentAngles );

	body->s.pos.trType = TR_GRAVITY;
	body->s.pos.trTime = level.time;
	VectorCopy( client->ps.velocity, body->s.pos.trDelta );

	body->s.eType = ET_PLAYER_CORPSE;
	body->physicsObject = qtrue;
	body->s.groundEntityNum = ENTITYNUM_NONE;

	body->r.svFlags = SVF_CAPSULE;
	VectorCopy( ent->r.mins, body->r.mins );
	VectorCopy( ent->r.maxs, body->r.maxs );
	VectorCopy( ent->r.absmin, body->r.absmin );
	VectorCopy( ent->r.absmax, body->r.absmax );

	body->s.legsAnim = client->ps.legsAnim;
	body->s.torsoAnim = client->ps.torsoAnim;

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;

	trap_LinkEntity( body );

	body->nextthink = level.time + 250;
	body->think = BodyEnd;

	Scr_AddEntity( body );
}

/*
==============
PlayerCmd_SetClientCvar

Set a cvar on the player's client
==============
*/
void PlayerCmd_SetClientCvar( unsigned int entnum ) {
	gentity_t   *ent;
	const char  *cvarName;
	const char  *string;
	char value[1024];
	char message[1024];
	int i;

	GET_PLAYER_ENTITY( ent, entnum );

	cvarName = Scr_GetString( 0 );

	if ( Scr_GetType( 1 ) == VAR_LOCALIZED_STRING ) {
		Scr_ConstructMessageString( 1, MSGTYPE_CLIENT_CVAR_VALUE, message, sizeof( message ) );
		string = message;
	} else {
		string = Scr_GetString( 1 );
	}

	// keep: the loop bound is larger than the destination (original behavior)
	memset( value, 0, sizeof( value ) );
	for ( i = 0; i < 8192; i++ ) {
		char c;

		c = string[i];
		if ( !c ) {
			break;
		}
		if ( c == (char)0x92 ) {
			c = '\'';
		} else if ( (byte)c > 0x7F ) {
			c = '.';
		}
		value[i] = c;
		if ( value[i] == '"' ) {
			value[i] = '\'';
		}
	}

	trap_SendServerCommand( entnum, 1, va( "v %s \"%s\"", cvarName, value ) );
}

/*
==============
PlayerCmd_FreezeControls

Freeze or unfreeze the player's controls
==============
*/
void PlayerCmd_FreezeControls( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	ent->client->frozen = Scr_GetBool( 0 );
}

/*
==============
PlayerCmd_SetReverb

Set the reverb room type, wet level and fade time for the player
==============
*/
void PlayerCmd_SetReverb( unsigned int entnum ) {
	gentity_t   *ent;
	const char  *roomType;
	float wetLevel;
	float fadeTime;

	GET_PLAYER_ENTITY( ent, entnum );

	fadeTime = 0;
	wetLevel = 0.5f;

	switch ( Scr_GetNumParam() ) {
	case 3:
		fadeTime = Scr_GetFloat( 2 );
	case 2:
		wetLevel = Scr_GetFloat( 1 );
	case 1:
		roomType = Scr_GetString( 0 );
		trap_SendServerCommand( entnum, 1, va( "r \"%s\" %g %g", roomType, wetLevel, fadeTime ) );
		break;
	default:
		Scr_Error( "USAGE: player setReverb(\"roomtype\", wetlevel = 0.5, fadetime = 1);\n"
				   "wetlevel is a float from 0 (no effect) to 1 (full effect), "
				   "fadetime is in sec and just modifies wetlevel\n" );
		break;
	}
}

/*
==============
ScrCmd_IsLookingAt

Returns 1 if the player is looking at the given entity
==============
*/
void ScrCmd_IsLookingAt( unsigned int entnum ) {
	gentity_t   *ent;

	GET_PLAYER_ENTITY( ent, entnum );

	Scr_AddInt( ent->client->lookatEnt == Scr_GetEntity( 0 ) );
}

/*
==============
ScrCmd_PlayLocalSound

Play a sound alias only this player hears
==============
*/
void ScrCmd_PlayLocalSound( unsigned int entnum ) {
	gentity_t   *ent;
	byte alias;

	GET_PLAYER_ENTITY( ent, entnum );

	alias = G_SoundAliasIndex( Scr_GetString( 0 ) );
	trap_SendServerCommand( entnum, 0, va( "s %i", alias ) );
}

/*
==============
PlayerCmd_SayAll

Say a localized message to everyone as the player
==============
*/
void PlayerCmd_SayAll( unsigned int entnum ) {
	gentity_t   *ent;
	char text[1024];

	GET_PLAYER_ENTITY( ent, entnum );

	Scr_ConstructMessageString( 0, MSGTYPE_CLIENT_CHAT, &text[1], sizeof( text ) - 1 );
	text[0] = MSGCHAR_LOCALIZED;

	G_Say( ent, NULL, SAY_ALL, text );
}

/*
==============
PlayerCmd_SayTeam

Say a localized message to the player's team
==============
*/
void PlayerCmd_SayTeam( unsigned int entnum ) {
	gentity_t   *ent;
	char text[1024];

	GET_PLAYER_ENTITY( ent, entnum );

	Scr_ConstructMessageString( 0, MSGTYPE_CLIENT_CHAT, &text[1], sizeof( text ) - 1 );
	text[0] = MSGCHAR_LOCALIZED;

	G_Say( ent, NULL, SAY_TEAM, text );
}

typedef struct {
	const char  *name;
	PlayerCmd_t call;
} playerMethod_t;

static const playerMethod_t player_methods[] = {
	{ "giveweapon",             PlayerCmd_giveWeapon },
	{ "takeweapon",             PlayerCmd_takeWeapon },
	{ "takeallweapons",         PlayerCmd_takeAllWeapons },
	{ "getcurrentweapon",       PlayerCmd_getCurrentWeapon },
	{ "hasweapon",              PlayerCmd_hasWeapon },
	{ "switchtoweapon",         PlayerCmd_switchToWeapon },
	{ "givestartammo",          PlayerCmd_giveStartAmmo },
	{ "givemaxammo",            PlayerCmd_giveMaxAmmo },
	{ "getfractionstartammo",   PlayerCmd_getFractionStartAmmo },
	{ "getfractionmaxammo",     PlayerCmd_getFractionMaxAmmo },
	{ "setorigin",              PlayerCmd_setOrigin },
	{ "setplayerangles",        PlayerCmd_setAngles },
	{ "usebuttonpressed",       PlayerCmd_useButtonPressed },
	{ "attackbuttonpressed",    PlayerCmd_attackButtonPressed },
	{ "meleebuttonpressed",     PlayerCmd_meleeButtonPressed },
	{ "isonground",             PlayerCmd_isOnGround },
	{ "pingplayer",             PlayerCmd_pingPlayer },
	{ "setviewmodel",           PlayerCmd_SetViewmodel },
	{ "getviewmodel",           PlayerCmd_GetViewmodel },
	{ "sayall",                 PlayerCmd_SayAll },
	{ "sayteam",                PlayerCmd_SayTeam },
	{ "allowcomplaint",         PlayerCmd_allowComplaint },
	{ "showscoreboard",         PlayerCmd_showScoreboard },
	{ "setspawnweapon",         PlayerCmd_setSpawnWeapon },
	{ "dropitem",               PlayerCmd_dropItem },
	{ "finishplayerdamage",     PlayerCmd_finishPlayerDamage },
	{ "suicide",                PlayerCmd_Suicide },
	{ "openmenu",               PlayerCmd_OpenMenu },
	{ "openmenunomouse",        PlayerCmd_OpenMenuNoMouse },
	{ "closemenu",              PlayerCmd_CloseMenu },
	{ "freezecontrols",         PlayerCmd_FreezeControls },
	{ "setreverb",              PlayerCmd_SetReverb },
	{ "getweaponslotweapon",    PlayerCmd_GetWeaponSlotWeapon },
	{ "setweaponslotweapon",    PlayerCmd_SetWeaponSlotWeapon },
	{ "getweaponslotammo",      PlayerCmd_GetWeaponSlotAmmo },
	{ "setweaponslotammo",      PlayerCmd_SetWeaponSlotAmmo },
	{ "getweaponslotclipammo",  PlayerCmd_GetWeaponSlotClipAmmo },
	{ "setweaponslotclipammo",  PlayerCmd_SetWeaponSlotClipAmmo },
	{ "iprintln",               iclientprintln },
	{ "iprintlnbold",           iclientprintlnbold },
	{ "spawn",                  PlayerCmd_spawn },
	{ "setentertime",           PlayerCmd_setEnterTime },
	{ "cloneplayer",            PlayerCmd_ClonePlayer },
	{ "setclientcvar",          PlayerCmd_SetClientCvar },
	{ "islookingat",            ScrCmd_IsLookingAt },
	{ "playlocalsound",         ScrCmd_PlayLocalSound },
};

/*
==============
Player_GetMethod

Resolves a method name for Scr_GetMethod and points *pName at the
table's own copy of the string
==============
*/
PlayerCmd_t Player_GetMethod( const char **pName ) {
	unsigned int i;

	for ( i = 0; i < sizeof( player_methods ) / sizeof( player_methods[0] ); i++ ) {
		if ( !strcmp( *pName, player_methods[i].name ) ) {
			*pName = player_methods[i].name;
			return player_methods[i].call;
		}
	}

	return NULL;
}
