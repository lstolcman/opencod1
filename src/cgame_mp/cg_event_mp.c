// cg_event.c -- handle entity events at snapshot or playerstate transitions
// Sounds are played by alias name, and impacts and explosions go through the
// effects system.  The nextState events are run a frame early by CG_CheckPreEvents.

#include <string.h>
#include <stdlib.h>

#include "cg_local.h"
#include "../game_mp/bg_public.h"

#define MAX_EVENTS                  4           // max events per frame before we drop events
#define STEP_TIME                   100

// entity_event_t; must match eventnames[] in bg_misc.c
#define EV_NONE                         0
#define EV_FOOTSTEP_RUN_DEFAULT         1
#define EV_FOOTSTEP_RUN_ASPHALT         23
#define EV_FOOTSTEP_WALK_DEFAULT        24
#define EV_FOOTSTEP_WALK_ASPHALT        46
#define EV_FOOTSTEP_PRONE_DEFAULT       47
#define EV_FOOTSTEP_PRONE_ASPHALT       69
#define EV_JUMP_DEFAULT                 70
#define EV_JUMP_ASPHALT                 92
#define EV_LANDING_DEFAULT              93
#define EV_LANDING_ASPHALT              115
#define EV_LANDING_PAIN_DEFAULT         116
#define EV_LANDING_PAIN_ASPHALT         138
#define EV_FOLIAGE_SOUND                139
#define EV_STANCE_FORCE_STAND           140
#define EV_STANCE_FORCE_CROUCH          141
#define EV_STANCE_FORCE_PRONE           142
#define EV_STEP_VIEW                    143
#define EV_WATER_TOUCH                  144
#define EV_WATER_LEAVE                  145
#define EV_ITEM_PICKUP                  146
#define EV_ITEM_PICKUP_QUIET            147
#define EV_AMMO_PICKUP                  148
#define EV_NOAMMO                       149
#define EV_EMPTYCLIP                    150
#define EV_RELOAD                       151
#define EV_RELOAD_FROM_EMPTY            152
#define EV_RELOAD_START                 153
#define EV_RELOAD_END                   154
#define EV_RAISE_WEAPON                 155
#define EV_PUTAWAY_WEAPON               156
#define EV_WEAPON_ALT                   157
#define EV_PULLBACK_WEAPON              158
#define EV_FIRE_WEAPON                  159
#define EV_FIRE_WEAPONB                 160
#define EV_FIRE_WEAPON_LASTSHOT         161
#define EV_RECHAMBER_WEAPON             162
#define EV_EJECT_BRASS                  163
#define EV_MELEE_SWIPE                  164
#define EV_FIRE_MELEE                   165
#define EV_MELEE_HIT                    166
#define EV_MELEE_MISS                   167
#define EV_FIRE_WEAPON_MG42             168
#define EV_FIRE_QUADBARREL_1            169
#define EV_FIRE_QUADBARREL_2            170
#define EV_BULLET_TRACER                171
#define EV_SOUND_ALIAS                  172
#define EV_BULLET_HIT_SMALL             173
#define EV_BULLET_HIT_LARGE             174
#define EV_BULLET_HIT_CLIENT_SMALL      175
#define EV_BULLET_HIT_CLIENT_LARGE      176
#define EV_GRENADE_BOUNCE               177
#define EV_GRENADE_EXPLODE              178
#define EV_ROCKET_EXPLODE               179
#define EV_ROCKET_EXPLODE_NOMARKS       180
#define EV_RAILTRAIL                    185
#define EV_PAIN                         187
#define EV_CROUCH_PAIN                  188
#define EV_DEATH                        189
#define EV_DEBUG_LINE                   190
#define EV_PLAY_FX                      191
#define EV_PLAY_FX_DIR                  192
#define EV_PLAY_FX_ON_TAG               193
#define EV_FLAMEBARREL_BOUNCE           194
#define EV_EARTHQUAKE                   195
#define EV_DROPWEAPON                   196
#define EV_ITEM_RESPAWN                 197
#define EV_ITEM_POP                     198
#define EV_PLAYER_TELEPORT_IN           199
#define EV_PLAYER_TELEPORT_OUT          200
#define EV_OBITUARY                     201

// any of the EV_* events can be added freestanding
// by setting eType to ET_EVENTS + eventNum
#define ET_EVENTS                       12

// configstrings
#define CS_SOUNDALIAS                   524
#define CS_EFFECTS_ONTAG                844
#define MAX_CONFIGSTRINGS               2048

// size of cgs.gameEffects
#define MAX_FX_CG                       64

#define ENTITYNUM_WORLD                 ( MAX_GENTITIES - 2 )

#define PMF_VIEWLOCKED                  0x00050000

// set when eventParm is a meansOfDeath_t rather than a weapon index
#define MOD_FLAG                        0x80
#define MOD_MELEE                       0x07
#define MOD_HEAD_SHOT                   0x08
#define MOD_WATER                       0x10
#define MOD_SLIME                       0x11
#define MOD_CRUSH                       0x13
#define MOD_FALLING                     0x15
#define MOD_SUICIDE                     0x16
#define MOD_TRIGGER_HURT                0x17

// cg_items[] -- per-item visuals; only the pickup sounds are used here
typedef struct cgItemInfo_t {
	qboolean    registered;
	qhandle_t   models[MAX_ITEM_MODELS];
	qhandle_t   icons[MAX_ITEM_ICONS];
	const char  *pickupSound;
	const char  *ammoPickupSound;
} cgItemInfo_t;

extern cgItemInfo_t cg_items[];

void        CG_Printf( const char *msg, ... );
void        CG_Error( const char *msg, ... );
void        Com_DPrintf( const char *msg, ... );
int         CG_PlaySoundAliasByName( const char *name, int entnum, const vec3_t origin );
void        CG_PriorityCenterPrint( const char *str, float y, float charWidth, int priority );
void        CG_DrawScoreboard_GetTeamColor( vec4_t color, int team );
void        CG_StartShakeCamera( float amplitude, int duration, float radius, const vec3_t origin );

// eventnames[] is reached through bg_misc.c's pointer to it
extern char **pEventNamesList;
#define eventnames pEventNamesList

/*
=================
CG_Obituary
=================
*/
static void CG_Obituary( entityState_t *ent ) {
	int         mod;
	int         means;
	int         target, attacker;
	const char  *icon;
	float       iconWidth;
	float       iconHeight;
	vec4_t      attackerColor;
	vec4_t      targetColor;
	vec4_t      iconColor;
	char        attackerName[36];
	char        targetName[36];
	clientInfo_t *targetInfo;
	clientInfo_t *attackerInfo;
	const char  *s;

	attackerColor[0] = 1.0f;
	attackerColor[1] = 1.0f;
	attackerColor[2] = 1.0f;
	attackerColor[3] = 1.0f;
	targetColor[0] = 1.0f;
	targetColor[1] = 1.0f;
	targetColor[2] = 1.0f;
	targetColor[3] = 1.0f;
	iconColor[0] = 1.0f;
	iconColor[1] = 1.0f;
	iconColor[2] = 1.0f;
	iconColor[3] = 1.0f;

	target = ent->otherEntityNum;
	attacker = ent->attackerEntityNum;
	mod = ent->eventParm;

	iconWidth = 1.4f;
	iconHeight = 1.4f;

	if ( mod & MOD_FLAG ) {
		means = mod & ~MOD_FLAG;
		icon = "killIconDied";
	} else {
		means = 0;
		if ( bg_weaponInfo[mod]->killIcon[0] ) {
			icon = bg_weaponInfo[mod]->killIcon;
			if ( bg_weaponInfo[mod]->wideKillIcon ) {
				iconWidth = 2.8f;
			}
		} else {
			icon = "killIconDied";
		}
	}

	switch ( means ) {
	case MOD_MELEE:         iconWidth = 1.4f;   icon = "killIconMelee";     break;
	case MOD_HEAD_SHOT:     iconWidth = 1.4f;   icon = "killIconHeadShot";  break;
	case MOD_SUICIDE:       iconWidth = 1.4f;   icon = "killIconSuicide";   break;
	case MOD_FALLING:       iconWidth = 1.4f;   icon = "killIconFalling";   break;
	case MOD_CRUSH:         iconWidth = 1.4f;   icon = "killIconCrush";     break;
	case MOD_WATER:         iconWidth = 1.4f;   icon = "killIconDrown";     break;
	case MOD_SLIME:         iconWidth = 1.4f;   icon = "killIconSlime";     break;
	case MOD_TRIGGER_HURT:  iconWidth = 1.4f;   icon = "killIconDied";      break;
	default:                                                                break;
	}

	if ( target < 0 || target >= MAX_CLIENTS ) {
		CG_Error( "CG_Obituary: target out of range" );
	}

	targetInfo = &bg_clientinfo[ target ];
	if ( !targetInfo->infoValid ) {
		return;
	}

	strncpy( targetName, targetInfo->name, 31 );
	targetName[31] = 0;
	strcat( targetName, "^7" );
	CG_DrawScoreboard_GetTeamColor( targetColor, targetInfo->team );

	if ( !bg_clientinfo[ cg.clientNum ].infoValid ) {
		return;
	}

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
		attackerName[0] = 0;
	} else {
		attackerInfo = &bg_clientinfo[ attacker ];
		if ( !attackerInfo->infoValid ) {
			return;
		}
		strncpy( attackerName, attackerInfo->name, 31 );
		attackerName[31] = 0;
		strcat( attackerName, "^7" );
		CG_DrawScoreboard_GetTeamColor( attackerColor, attackerInfo->team );

		if ( target == cg.snap->ps.clientNum ) {
			strncpy( cg.killerName, attackerName, 31 );
			cg.killerName[31] = 0;
		}
	}

	if ( attacker == target ) {
		attackerName[0] = 0;
	} else if ( attacker == cg.snap->ps.clientNum ) {
		if ( attackerInfo->team && targetInfo->team == attackerInfo->team ) {
			s = va( "CGAME_YOUKILLED\x15^1%%s^7 %s\x14%s", targetName, "CGAME_TEAMMATE" );
		} else {
			s = va( "CGAME_YOUKILLED\x15%s", targetName );
		}
		CG_PriorityCenterPrint( s, 400.0f, 9.6f, 1 );
	}

	CG_DreathMessage( attackerName, attackerColor, targetName, targetColor,
					  icon, iconWidth, iconHeight, iconColor );
}

/*
===============
CG_ItemPickup

A new item was picked up this frame
===============
*/
static void CG_ItemPickup( int itemNum ) {
	int     weapon;

	cg.itemPickup = itemNum;
	cg.itemPickupTime = cg.time;
	cg.itemPickupBlendTime = cg.time;

	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON ) {
		weapon = bg_itemlist[itemNum].giTag;
		if ( !cg_weaponSelect.integer ) {
			cg.weaponSelectTime = cg.time;
			trap_Cvar_Set( "cg_weaponSelect", va( "%i", weapon ) );
		}
	}
}

/*
===============
CG_EquipmentSound

Gear rattle that goes with each footstep
===============
*/
static void CG_EquipmentSound( int entnum, qboolean running ) {
	if ( running ) {
		CG_PlayEntitySoundAliasByName( entnum, (const char *)cgs.media.gearRattleRun );
	} else {
		CG_PlayEntitySoundAliasByName( entnum, (const char *)cgs.media.gearRattleWalk );
	}
}

/*
===============
CG_PlayFx
===============
*/
static void CG_PlayFx( centity_t *cent, const vec3_t dir ) {
	int     fx;

	if ( cent->currentState.eventParm <= 0 || cent->currentState.eventParm >= MAX_FX_CG ) {
		CG_Printf( "ERROR: CG_PlayFx called with invalid effect id %i\n",
				   cent->currentState.eventParm );
		return;
	}

	fx = cgs.gameEffects[ cent->currentState.eventParm ];
	if ( dir ) {
		trap_FX_PlayEffectID( fx, cent->lerpOrigin, dir );
	} else {
		trap_FX_PlaySimpleEffectID( fx, cent->lerpOrigin );
	}
}

/*
===============
CG_PlayFxOnTag

The configstring is "%02i" of the effect index followed by the tag name.
===============
*/
static void CG_PlayFxOnTag( centity_t *cent, int eventParm ) {
	const char  *cs;
	int         effect;
	int         bolt[2];

	cs = CG_ConfigString( CS_EFFECTS_ONTAG + eventParm );
	effect = cgs.gameEffects[ ( cs[0] - '0' ) * 10 + ( cs[1] - '0' ) ];

	bolt[0] = cent->currentState.number;
	bolt[1] = trap_FX_GetBoneIndex( cent->currentState.number, cs + 2 );
	if ( bolt[1] < 0 ) {
		return;
	}

	trap_FX_PlayOrientedEffectID( effect, cent->lerpOrigin, NULL, bolt );
}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
void CG_EntityEvent( centity_t *cent, int event, qboolean predicted ) {
	entityState_t   *es;
	int             clientNum;
	int             eventParm;
	int             item;
	cgItemInfo_t    *itemInfo;
	int             kick;
	rfloat          height;     // keep: rfloat, matches original codegen
	float           *position;
	const char      *s;
	vec3_t          dir;

	if ( !event ) {
		if ( cg_debugevents.integer ) {
			CG_Printf( "CG_EntityEvent:ZERO EVENT\n" );
		}
		return;
	}

	es = &cent->currentState;
	eventParm = es->eventParm;
	position = cent->lerpOrigin;

	if ( cg_debugevents.integer ) {
		CG_Printf( "ent:%3i  event:%3i ", es->number, event );
		if ( cg_debugevents.integer ) {
			CG_Printf( "CG_EntityEvent:%s\n", eventnames[event] );
		}
	}

	clientNum = es->clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}

	if ( event >= EV_FOOTSTEP_RUN_DEFAULT && event < EV_FOOTSTEP_WALK_DEFAULT ) {
		if ( cg_footsteps.integer ) {
			CG_PlayEntitySoundAliasByName( es->number,
										   (const char *)cgs.media.footstepRunSounds[ event - EV_FOOTSTEP_RUN_DEFAULT ] );
		}
		CG_EquipmentSound( es->number, qtrue );
		return;
	}

	if ( event >= EV_FOOTSTEP_WALK_DEFAULT && event < EV_FOOTSTEP_PRONE_DEFAULT ) {
		if ( cg_footsteps.integer ) {
			CG_PlayEntitySoundAliasByName( es->number,
										   (const char *)cgs.media.footstepWalkSounds[ event - EV_FOOTSTEP_WALK_DEFAULT ] );
		}
		CG_EquipmentSound( es->number, qfalse );
		return;
	}

	if ( event >= EV_FOOTSTEP_PRONE_DEFAULT && event < EV_JUMP_DEFAULT ) {
		if ( cg_footsteps.integer ) {
			CG_PlayEntitySoundAliasByName( es->number,
										   (const char *)cgs.media.footstepProneSounds[ event - EV_FOOTSTEP_PRONE_DEFAULT ] );
		}
		CG_EquipmentSound( es->number, qfalse );
		return;
	}

	if ( event >= EV_JUMP_DEFAULT && event < EV_LANDING_DEFAULT ) {
		CG_PlayEntitySoundAliasByName( es->number,
									   (const char *)cgs.media.footstepRunSounds[ event - EV_JUMP_DEFAULT ] );
		CG_EquipmentSound( es->number, qtrue );
		return;
	}

	if ( event >= EV_LANDING_DEFAULT && event < EV_LANDING_PAIN_DEFAULT ) {
		CG_PlayEntitySoundAliasByName( es->number,
									   (const char *)cgs.media.landingSounds[ event - EV_LANDING_DEFAULT ] );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landTime = cg.time;
			cg.landChange = -(float)eventParm;
		}
		return;
	}

	if ( event >= EV_LANDING_PAIN_DEFAULT && event < EV_FOLIAGE_SOUND ) {
		CG_PlayEntitySoundAliasByName( es->number,
									   (const char *)cgs.media.landingSounds[ event - EV_LANDING_PAIN_DEFAULT ] );
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.landDamageSound );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			height = ( bg_fallDamageMaxHeight.value - bg_fallDamageMinHeight.value )
					 * ( eventParm * 0.01f ) + bg_fallDamageMinHeight.value;
			if ( height <= 12.0f ) {
				return;
			}
			kick = (int)( ( ( height - 12.0f ) * ( 1.0f / 26.0f ) + 1.0f ) * 4.0f );
			if ( kick > 24 ) {
				kick = 24;
			} else if ( kick <= 0 ) {
				return;
			}
			cg.landTime = cg.time;
			cg.landChange = -(float)kick;
		}
		return;
	}

	// keep: case order matches original codegen
	switch ( event ) {
	case EV_FOLIAGE_SOUND:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.movementFoliage );
		break;

	case EV_STANCE_FORCE_STAND:
		if ( clientNum != cg.predictedPlayerState.clientNum ) {
			Com_DPrintf( "Event %s just for client %i was sent to other clients\n",
						 eventnames[event], clientNum );
			return;     // keep: return, not break (original codegen)
		}
		if ( !cl_stanceTemp.integer ) {
			trap_Cvar_Setvar( &cl_stance, "0" );
		}
		break;

	case EV_STANCE_FORCE_CROUCH:
		if ( clientNum != cg.predictedPlayerState.clientNum ) {
			Com_DPrintf( "Event %s just for client %i was sent to other clients\n",
						 eventnames[event], clientNum );
			break;
		}
		if ( !cl_stanceTemp.integer ) {
			trap_Cvar_Setvar( &cl_stance, "1" );
		}
		break;

	case EV_STANCE_FORCE_PRONE:
		if ( clientNum != cg.predictedPlayerState.clientNum ) {
			Com_DPrintf( "Event %s just for client %i was sent to other clients\n",
						 eventnames[event], clientNum );
			break;
		}
		if ( !cl_stanceTemp.integer ) {
			trap_Cvar_Setvar( &cl_stance, "2" );
		}
		break;

	case EV_STEP_VIEW:      // smooth out step up transitions
		{
			int     delta;
			rfloat  oldStep;    // keep: rfloat, matches original codegen
			float   step;

			if ( clientNum != cg.predictedPlayerState.clientNum ) {
				Com_DPrintf( "Event %s just for client %i was sent to other clients\n",
							 eventnames[event], clientNum );
				break;
			}
			// if we are interpolating, we don't need to smooth steps
			if ( cg.demoPlayback || cg_nopredict.integer || g_synchronousClients.integer ) {
				break;
			}

			// check for stepping up before a previous step is completed
			delta = cg.time - cg.stepTime;
			if ( delta < STEP_TIME ) {
				// keep: two statements, matches original codegen
				oldStep = cg.stepChange * (rfloat)( STEP_TIME - delta ) * 0.01f;
				oldStep *= 0.9f;
			} else {
				oldStep = 0.0f;
			}

			// add this amount
			step = (float)( eventParm - 128 );
			cg.stepChange = step + oldStep;
			if ( cg.stepChange > 24.0f ) {
				cg.stepChange = 24.0f;
			} else if ( cg.stepChange < -16.0f ) {
				cg.stepChange = -16.0f;
			}
			cg.stepTime = cg.time;
		}
		break;

	case EV_WATER_TOUCH:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.playerWaterIn );
		break;

	case EV_WATER_LEAVE:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.playerWaterOut );
		break;

	case EV_ITEM_PICKUP:
	case EV_ITEM_PICKUP_QUIET:
	case EV_AMMO_PICKUP:
		item = es->eventParm;      // player predicted
		if ( item < 1 || item >= 70 ) {
			break;
		}
		itemInfo = &cg_items[item];
		if ( event == EV_ITEM_PICKUP ) {
			CG_PlayEntitySoundAliasByName( es->number, itemInfo->pickupSound );
		} else if ( event == EV_AMMO_PICKUP ) {
			CG_PlayEntitySoundAliasByName( es->number, itemInfo->ammoPickupSound );
		}
		// show icon and name on status bar
		if ( ( cg.snap->ps.pm_flags & PMF_VIEWLOCKED ) && es->number == cg.snap->ps.clientNum ) {
			CG_ItemPickup( item );
		}
		break;

		//
		// weapon events
		//
	case EV_RELOAD:
		if ( cg_weapons[ es->weapon ].reloadSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].reloadSound );
		} else if ( cg_weapons[ es->weapon ].reloadEmptySound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].reloadEmptySound );
		}
		break;

	case EV_RELOAD_FROM_EMPTY:
		if ( cg_weapons[ es->weapon ].reloadEmptySound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].reloadEmptySound );
		} else if ( cg_weapons[ es->weapon ].reloadSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].reloadSound );
		}
		break;

	case EV_RELOAD_START:
		if ( cg_weapons[ es->weapon ].reloadStartSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].reloadStartSound );
		}
		break;

	case EV_RELOAD_END:
		if ( cg_weapons[ es->weapon ].reloadEndSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].reloadEndSound );
		}
		break;

	case EV_NOAMMO:
		if ( !bg_weaponInfo[ es->weapon ]->clipOnly ) {
			CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.noAmmoSound );
		}
		if ( ( cg.snap->ps.pm_flags & PMF_VIEWLOCKED ) && es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange();
		}
		break;

	case EV_DROPWEAPON:
		if ( cg_weapons[ es->weapon ].putawaySound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].putawaySound );
		}
		if ( ( cg.snap->ps.pm_flags & PMF_VIEWLOCKED ) && es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange();
		}
		break;

	case EV_EMPTYCLIP:
		break;

	case EV_RAISE_WEAPON:
		if ( cg_weapons[ es->weapon ].raiseSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].raiseSound );
		}
		break;

	case EV_PUTAWAY_WEAPON:
		if ( cg_weapons[ es->weapon ].putawaySound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].putawaySound );
		}
		break;

	case EV_WEAPON_ALT:
		if ( cg_weapons[ es->weapon ].altSwitchSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].altSwitchSound );
		}
		break;

	case EV_FIRE_QUADBARREL_1:
		if ( predicted ) {
			CG_FireWeapon( cent, es, event, 0 );
			CG_FireWeapon( cent, es, event, 1 );
		}
		break;

	case EV_FIRE_QUADBARREL_2:
		if ( predicted ) {
			CG_FireWeapon( cent, es, event, 2 );
			CG_FireWeapon( cent, es, event, 3 );
		}
		break;

	case EV_FIRE_WEAPON:
	case EV_FIRE_WEAPONB:
	case EV_FIRE_WEAPON_LASTSHOT:
	case EV_FIRE_WEAPON_MG42:
		if ( predicted ) {
			CG_FireWeapon( cent, es, event, 0 );
		}
		break;

	case EV_PULLBACK_WEAPON:
		if ( cg_weapons[ es->weapon ].pullbackSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].pullbackSound );
		}
		break;

	case EV_RECHAMBER_WEAPON:
		if ( predicted && cg_weapons[ es->weapon ].rechamberSound ) {
			CG_PlayEntitySoundAliasByName( es->number, cg_weapons[ es->weapon ].rechamberSound );
		}
		break;

	case EV_EJECT_BRASS:
		if ( predicted ) {
			CG_EjectWeaponBrass( es, event );
		}
		break;

	case EV_MELEE_SWIPE:
		if ( bg_weaponInfo[ es->weapon ]->rifleBullet ) {
			CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.meleeSwingLarge );
		} else {
			CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.meleeSwingSmall );
		}
		break;

	case EV_FIRE_MELEE:
	case EV_MELEE_MISS:
		break;

	case EV_MELEE_HIT:
		ByteToDir( es->eventParm, dir );
		CG_PlayEntitySoundAliasByName( es->otherEntityNum, (const char *)cgs.media.meleeHit );
		break;

		//=================================================================

		//
		// other events
		//
	case EV_PLAYER_TELEPORT_IN:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.teleInSound );
		break;

	case EV_PLAYER_TELEPORT_OUT:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.teleOutSound );
		break;

	case EV_ITEM_POP:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.respawnSound );
		break;

	case EV_ITEM_RESPAWN:
		cent->miscTime = cg.time;    // scale up from this
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.respawnSound );
		break;

	case EV_FLAMEBARREL_BOUNCE:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.flamebarrelBounce );
		break;

	case EV_RAILTRAIL:
		// ev_railtrail is now sent standalone rather than by a player entity
		CG_RailTrail( es->dmgFlags, es->origin2, es->pos.trBase );
		break;

	case EV_BULLET_HIT_SMALL:
	case EV_BULLET_HIT_LARGE:
	case EV_BULLET_HIT_CLIENT_SMALL:
	case EV_BULLET_HIT_CLIENT_LARGE:
		break;

	case EV_GRENADE_BOUNCE:
		ByteToDir( es->eventParm, dir );
		CG_PlaySoundAliasByName( (const char *)cgs.media.grenadeBounceSounds[ es->surfType ],
								 ENTITYNUM_WORLD, position );
		if ( cgs.media.grenadeBounceEffects[ es->surfType ] ) {
			trap_FX_PlayEffectID( cgs.media.grenadeBounceEffects[ es->surfType ],
							   position, dir );
		}
		break;

	case EV_GRENADE_EXPLODE:
		ByteToDir( es->eventParm, dir );
		CG_PlaySoundAliasByName( (const char *)cgs.media.grenadeExplodeSounds[ es->surfType ],
								 ENTITYNUM_WORLD, position );
		if ( cgs.media.grenadeExplodeEffects[ es->surfType ] ) {
			trap_FX_PlayEffectID( cgs.media.grenadeExplodeEffects[ es->surfType ],
							   position, dir );
		}
		if ( cg_weapons[ es->weapon ].projExplosionEffect ) {
			trap_FX_PlayEffectID( cg_weapons[ es->weapon ].projExplosionEffect,
							   position, dir );
		}
		if ( cg_weapons[ es->weapon ].projExplosionSound
			 && cg_weapons[ es->weapon ].projExplosionSound[0] ) {
			CG_PlaySoundAliasByName( cg_weapons[ es->weapon ].projExplosionSound,
									 ENTITYNUM_WORLD, position );
		}
		break;

	case EV_ROCKET_EXPLODE_NOMARKS:
		cg.noMarks = 1;
		// fall through
	case EV_ROCKET_EXPLODE:
		ByteToDir( es->eventParm, dir );
		CG_PlaySoundAliasByName( (const char *)cgs.media.rocketExplodeSounds[ es->surfType ],
								 ENTITYNUM_WORLD, position );
		if ( cgs.media.rocketExplodeEffects[ es->surfType ] ) {
			trap_FX_PlayEffectID( cgs.media.rocketExplodeEffects[ es->surfType ],
							   position, dir );
		}
		if ( cg_weapons[ es->weapon ].projExplosionEffect ) {
			trap_FX_PlayEffectID( cg_weapons[ es->weapon ].projExplosionEffect,
							   position, dir );
		}
		if ( cg_weapons[ es->weapon ].projExplosionSound
			 && cg_weapons[ es->weapon ].projExplosionSound[0] ) {
			CG_PlaySoundAliasByName( cg_weapons[ es->weapon ].projExplosionSound,
									 ENTITYNUM_WORLD, position );
		}
		cg.noMarks = 0;
		break;

	case EV_SOUND_ALIAS:
		s = CG_ConfigString( CS_SOUNDALIAS + es->eventParm );
		CG_PlaySoundAliasByName( s, es->number, es->pos.trBase );
		break;

	case EV_PAIN:
	case EV_CROUCH_PAIN:
		break;

	case EV_DEATH:
		CG_PlayEntitySoundAliasByName( es->number, (const char *)cgs.media.deathSound );
		break;

	case EV_OBITUARY:
		CG_Obituary( es );
		break;

	case EV_DEBUG_LINE:
		CG_Beam( cent );
		break;

	case EV_PLAY_FX:
		CG_PlayFx( cent, NULL );
		break;

	case EV_PLAY_FX_DIR:
		// the direction is packed into scale
		ByteToDir( es->scale, dir );
		CG_PlayFx( cent, dir );
		break;

	case EV_PLAY_FX_ON_TAG:
		CG_PlayFxOnTag( cent, es->eventParm );
		break;

	case EV_EARTHQUAKE:
		break;

	default:
		CG_Error( "Unknown event: '%s'", eventnames[event] );
		break;
	}
}

/*
==============
CG_EntityPreEvent

The nextState half of the event stream: the same event fires one frame early so
the muzzle flash and the bullet impact are not a snapshot late.
==============
*/
static void CG_EntityPreEvent( centity_t *cent, int event ) {
	entityState_t   *ns;
	vec3_t          dir;
	vec3_t          dir2;

	if ( !event ) {
		if ( cg_debugevents.integer ) {
			CG_Printf( "CG_EntityPreEvent:ZERO EVENT\n" );
		}
		return;
	}

	ns = &cent->nextState;

	if ( cg_debugevents.integer ) {
		CG_Printf( "ent:%3i  preevent:%3i CG_EntityPreEvent:%s\n",
				   ns->number, event, eventnames[event] );
	}

	switch ( event ) {
	case EV_BULLET_HIT_SMALL:
	case EV_BULLET_HIT_LARGE:
		ByteToDir( ns->eventParm, dir );
		ByteToDir( ns->scale, dir2 );
		CG_BulletHitEvent( ns->otherEntityNum, cent->lerpOrigin, ns->surfType, event,
						   dir, dir2 );
		break;

	case EV_BULLET_HIT_CLIENT_SMALL:
	case EV_BULLET_HIT_CLIENT_LARGE:
		CG_BulletHitClientEvent( ns->otherEntityNum, cent->lerpOrigin, ns->surfType, event );
		break;

	case EV_FIRE_QUADBARREL_1:
		CG_FireWeapon( cent, ns, event, 0 );
		CG_FireWeapon( cent, ns, event, 1 );
		break;

	case EV_FIRE_QUADBARREL_2:
		CG_FireWeapon( cent, ns, event, 2 );
		CG_FireWeapon( cent, ns, event, 3 );
		break;

	case EV_FIRE_WEAPON_MG42:
		CG_StartShakeCamera( 0.05f, 100, 100.0f, cent->lerpOrigin );
		// fall through
	case EV_FIRE_WEAPON:
	case EV_FIRE_WEAPONB:
	case EV_FIRE_WEAPON_LASTSHOT:
		CG_FireWeapon( cent, ns, event, 0 );
		break;

	case EV_RECHAMBER_WEAPON:
		if ( cg_weapons[ ns->weapon ].rechamberSound ) {
			CG_PlayEntitySoundAliasByName( ns->number, cg_weapons[ ns->weapon ].rechamberSound );
		}
		break;

	case EV_EJECT_BRASS:
		CG_EjectWeaponBrass( ns, event );
		break;

	case EV_EARTHQUAKE:
		CG_StartShakeCamera( ns->angles2[0], ns->time, ns->angles2[1], cent->lerpOrigin );
		break;

	default:
		break;
	}
}

/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	int     i;
	int     event;
	byte    oldEventParm;

	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEventSequence ) {
			return;         // already fired
		}
		cent->previousEventSequence = 1;
		CG_CalcEntityLerpPositions( cent );
		CG_EntityEvent( cent, cent->currentState.eType - ET_EVENTS, qfalse );
		return;
	}

	if ( !cent->currentState.eventSequence ) {
		cent->previousEventSequence = 0;
		return;
	}

	// check the sequencial list
	// if we've added more events than can fit into the list, make sure we only add them once
	if ( cent->currentState.eventSequence < cent->previousEventSequence ) {
		cent->previousEventSequence -= ( 1 << 8 );    // eventSequence is sent as an 8-bit through network stream
	}
	if ( cent->currentState.eventSequence - cent->previousEventSequence > MAX_EVENTS ) {
		cent->previousEventSequence = cent->currentState.eventSequence - MAX_EVENTS;
	}

	if ( cent->previousEventSequence >= cent->currentState.eventSequence ) {
		cent->previousEventSequence = cent->currentState.eventSequence;
		return;
	}

	CG_CalcEntityLerpPositions( cent );

	oldEventParm = (byte)cent->currentState.eventParm;
	for ( i = cent->previousEventSequence ; i != cent->currentState.eventSequence ; i++ ) {
		event = cent->currentState.events[ i & ( MAX_EVENTS - 1 ) ];
		cent->currentState.eventParm = cent->currentState.eventParms[ i & ( MAX_EVENTS - 1 ) ];
		CG_EntityEvent( cent, event, qfalse );
	}
	cent->currentState.eventParm = oldEventParm;

	cent->previousEventSequence = cent->currentState.eventSequence;
}

/*
==============
CG_CheckPreEvents

Same as CG_CheckEvents, for the nextState events.
The loop changes nextState.eventParm but saves and restores
currentState.eventParm; keep: that is how the original does it.
==============
*/
void CG_CheckPreEvents( centity_t *cent ) {
	int     i;
	int     event;
	byte    oldEventParm;

	if ( cent->nextState.eType > ET_EVENTS ) {
		if ( cent->previousPreEventSequence ) {
			return;         // already fired
		}
		cent->previousPreEventSequence = 1;
		CG_CalcEntityLerpPositions( cent );
		CG_EntityPreEvent( cent, cent->nextState.eType - ET_EVENTS );
		return;
	}

	if ( !cent->nextState.eventSequence ) {
		cent->previousPreEventSequence = 0;
		return;
	}

	if ( cent->nextState.eventSequence < cent->previousPreEventSequence ) {
		cent->previousPreEventSequence -= ( 1 << 8 );
	}
	if ( cent->nextState.eventSequence - cent->previousPreEventSequence > MAX_EVENTS ) {
		cent->previousPreEventSequence = cent->nextState.eventSequence - MAX_EVENTS;
	}

	if ( cent->previousPreEventSequence < cent->nextState.eventSequence ) {
		CG_CalcEntityLerpPositions( cent );

		oldEventParm = (byte)cent->currentState.eventParm;
		for ( i = cent->previousPreEventSequence ; i != cent->nextState.eventSequence ; i++ ) {
			event = cent->nextState.events[ i & ( MAX_EVENTS - 1 ) ];
			cent->nextState.eventParm = cent->nextState.eventParms[ i & ( MAX_EVENTS - 1 ) ];
			CG_EntityPreEvent( cent, event );
		}
		event = cent->nextState.eventSequence;
		cent->currentState.eventParm = oldEventParm;
	} else {
		event = cent->nextState.eventSequence;
	}

	cent->previousPreEventSequence = event;
}
