// cg_players.c -- handle the media and animation for player entities
// the player model itself is animated by the shared bg_animation.c through
// its XAnim tree; this file adds the entity, its head sprites, shadow and
// splash, and seats mounted players on their turrets

#include <math.h>
#include <string.h>

#include "cg_local.h"

#define RT_DOBJ                 1
#define RT_SPRITE               4

#define RF_THIRD_PERSON         0x02
#define RF_LIGHTING_ORIGIN      0x80

// entityState_t->eFlags
#define EF_DEAD                 0x000001
#define EF_CROUCHING            0x000020    // name inferred
#define EF_PRONE                0x000040    // name inferred
#define EF_NODRAW               0x000100
#define EF_CONNECTION           0x001000
#define EF_MOUNTED              0x00C000
#define EF_TALKING              0x040000    // draw a talk balloon

// playerState_t->pm_flags
#define PMF_FIRSTPERSON         0x50000

#define ANIM_TOGGLEBIT          0x200

#define CS_HEADICONS            28

#ifndef TEAM_SPECTATOR
#define TEAM_SPECTATOR          3
#endif

#define SHADOW_DISTANCE         64.0f

#ifndef CONTENTS_SOLID
#define CONTENTS_SOLID          0x01
#define CONTENTS_WATER          0x20
#endif
#define MASK_PLAYERSOLID_CG     0x2810011

extern const vec3_t vec3_origin;

extern int  ( *Scr_GetAnimsIndex )( int anims );

float      *CG_DObjGetLocalTagMatrix( DObj *obj, const char *tagName, centity_t *cent );
qboolean    CG_DObjGetWorldTagMatrix( DObj *obj, const char *tagName, centity_t *cent,
									  float *outMatrix );

void        CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent,
								int drawGun );

// not in cg_local.h: vmMain's all-int prototype for it would conflict
void        CG_ImpactMark( qhandle_t markShader, const vec3_t origin, const vec3_t dir,
						   float orientation, float red, float green, float blue,
						   float alpha, qboolean alphaFade, float radius,
						   qboolean temporary, int duration );

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
DHM - Nerve :: added height parameter
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader, int height,
								  qboolean scaleByDistance ) {
	int         rf;
	DObj        *obj;
	refEntity_t ent;
	float       tagMatrix[16];
	vec3_t      origin;
	rfloat      z;
	rfloat      radius;
	rfloat      dx, dy, dz;

	if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
		 && cent->currentState.number == cg.snap->ps.clientNum
		 && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;       // only show in mirrors
	} else {
		rf = 0;
	}

	// keep: the rfloat temporaries and z, y, x sum order match the original
	obj = trap_GetClientDObj( cent->currentState.number );
	if ( obj && CG_DObjGetWorldTagMatrix( obj, "Bip01 Head", cent, tagMatrix ) ) {
		origin[0] = tagMatrix[12];
		origin[1] = tagMatrix[13];
		z = (rfloat)height + tagMatrix[14] + 18.0f;
	} else {
		VectorCopy( cent->lerpOrigin, origin );
		z = (rfloat)origin[2] + height + 72.0f;
	}

	if ( scaleByDistance ) {
		dx = (rfloat)origin[0] - cg.refdef.vieworg[0];
		dy = (rfloat)origin[1] - cg.refdef.vieworg[1];
		dz = z - cg.refdef.vieworg[2];
		radius = FCAST( sqrt( dz * dz + dy * dy + dx * dx ) ) * ( 1.0f / 256.0f ) + 0.2f;
		if ( radius < 0.6f ) {
			radius = 0.6f;
		}
		z += 8.0f * radius - 8.0f;
		radius *= 6.66f;
	} else {
		radius = 6.66f;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.origin[0] = origin[0];
	ent.origin[1] = origin[1];
	ent.origin[2] = (float)z;
	ent.reType = RT_SPRITE;
	ent.renderfx = rf;
	ent.radius = (float)radius;
	ent.spriteSize = (float)radius;
	ent.customShader = shader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;

	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	float       height;
	int         team;
	int         myTeam;
	const char  *name;
	qhandle_t   shader;

	height = 0.0f;

	if ( !bg_clientinfo[cent->currentState.clientNum].infoValid ) {
		return;
	}
	team = bg_clientinfo[cent->currentState.clientNum].team;

	if ( !bg_clientinfo[cg.snap->ps.clientNum].infoValid ) {
		return;
	}
	myTeam = bg_clientinfo[cg.snap->ps.clientNum].team;

	if ( cent->currentState.iHeadIcon ) {
		if ( !cent->currentState.iHeadIconTeam
			 || myTeam == TEAM_SPECTATOR
			 || cent->currentState.iHeadIconTeam == myTeam ) {
			name = CG_ConfigString( cent->currentState.iHeadIcon + CS_HEADICONS );
			shader = trap_R_RegisterShader( name, 5 );
			if ( shader ) {
				CG_PlayerFloatSprite( cent, shader, 0, qfalse );
				height = 16.0f;
			}
		}
	}

	if ( cent->currentState.number == cg.clientNum
		 && cg.snap->ps.clientNum != cg.clientNum ) {
		CG_PlayerFloatSprite( cent, cgs.media.headiconYouInKillCam, (int)height, qtrue );
		return;
	}

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.headiconDisconnected, (int)height, qfalse );
		return;
	}

	if ( team != myTeam && myTeam != TEAM_SPECTATOR ) {
		return;
	}

	// DHM - Nerve :: show voice chat signal so players know who's talking
	if ( cent->voiceChatSpriteTime > cg.time ) {
		CG_PlayerFloatSprite( cent, cent->voiceChatSprite, (int)height, qfalse );
		return;
	}

	if ( cent->currentState.eFlags & EF_TALKING ) {
		CG_PlayerFloatSprite( cent, cgs.media.headiconTalkBalloon, (int)( height - 5.0f ),
							  qfalse );
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
static qboolean CG_PlayerShadow( float *shadowPlane, centity_t *cent ) {
	vec3_t  end;
	trace_t trace;
	float   alpha;
	float   dist;
	rfloat  fade;

	*shadowPlane = 0;

	if ( !cg_shadows.integer ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, NULL, NULL, 0, MASK_PLAYERSOLID_CG );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.fraction == 0.0f ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1.0f;

	if ( cg_shadows.integer != 1 ) {    // no mark for stencil or projection shadows
		return qtrue;
	}

	// no shadows when dead
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return qfalse;
	}

	// fade the shadow out with height
	alpha = (float)( 1.0 - trace.fraction );

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	dist = Distance( cent->lerpOrigin, cg.snap->ps.origin );
	if ( dist > 250.0 ) {
		if ( dist < 512.0 ) {
			fade = ( dist - 250.0 ) * ( 1.0 / 262.0 );
		} else if ( dist > 1024.0 ) {
			return qfalse;
		} else { // fade out
			fade = 1.0 - ( dist - 512.0 ) * ( 1.0 / 512.0 );
		}
		alpha *= fade;

		CG_ImpactMark( cgs.media.markShadowShader, trace.endpos, trace.normal, 0.0f,
					   alpha, alpha, alpha, 1.0f, qfalse, 16.0f, qtrue, -1 );
	}

	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent ) {
	vec3_t      start, end;
	trace_t     trace;
	int         contents;
	polyVert_t  verts[4];

	if ( !cg_shadows.integer ) {
		return;
	}

	end[0] = cent->lerpOrigin[0];
	end[1] = cent->lerpOrigin[1];
	end[2] = cent->lerpOrigin[2] - 24.0f;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & CONTENTS_WATER ) ) {
		return;
	}

	start[0] = cent->lerpOrigin[0];
	start[1] = cent->lerpOrigin[1];
	start[2] = cent->lerpOrigin[2] + 32.0f;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER ) ) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	verts[0].xyz[0] = trace.endpos[0] - 32.0f;
	verts[0].xyz[1] = trace.endpos[1] - 32.0f;
	verts[0].xyz[2] = trace.endpos[2];
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	verts[1].xyz[0] = trace.endpos[0] - 32.0f;
	verts[1].xyz[1] = trace.endpos[1] + 32.0f;
	verts[1].xyz[2] = trace.endpos[2];
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	verts[2].xyz[0] = trace.endpos[0] + 32.0f;
	verts[2].xyz[1] = trace.endpos[1] + 32.0f;
	verts[2].xyz[2] = trace.endpos[2];
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	verts[3].xyz[0] = trace.endpos[0] + 32.0f;
	verts[3].xyz[1] = trace.endpos[1] - 32.0f;
	verts[3].xyz[2] = trace.endpos[2];
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}

// drive a goal weight at the rate that reaches it in one frame
// keep: a macro with block locals, so each use gets its own stack slots
#define SET_TURRET_GOAL_WEIGHT( animTree, anim, weight ) \
	do { \
		scr_anim_t  sgw_anim; \
		float       sgw_weight; \
		rfloat      sgw_rate; \
		float       sgw_time; \
		sgw_anim = ( anim ); \
		sgw_weight = ( weight ); \
		sgw_rate = fabs( (rfloat)trap_XAnimGetWeight( ( animTree ), sgw_anim ) - sgw_weight ) \
				   * ( 1000.0f / (rfloat)cg.frametime ); \
		if ( sgw_rate > 0.0f ) { \
			sgw_time = FCAST( 1.0f / sgw_rate ); \
		} else { \
			sgw_time = 0.0f; \
		} \
		trap_XAnimSetGoalWeight( ( animTree ), sgw_anim.index, sgw_weight, sgw_time, 1.0f, 0, 0 ); \
	} while ( 0 )

/*
===============
CG_PlayerTurretPositionAndBlend

Snap a mounted player onto the turret he is holding.

The "mounted" animation is a two-level tree: rows by stand height, columns
by yaw.  The yaw picks the column; the rows are walked until the hands rise
above the turret's tag_weapon and the two rows either side are blended.  The
player is then moved so the animation lines up with the turret grip and
dropped onto the floor underneath.
===============
*/
static void CG_PlayerTurretPositionAndBlend( centity_t *cent ) {
	centity_t       *turret;
	float           *tagMatrix;
	DObj            *obj;
	clientInfo_t    *ci;
	animation_t     *anim;
	weaponInfo_t    *weapon;
	scr_anim_t      rootAnim;
	scr_anim_t      row;
	scr_anim_t      col;
	scr_anim_t      nextCol;
	float           lastFrac;
	int             lastCol;
	void            *animTree;
	int             turretNum;
	float           yaw;
	vec3_t          axis[4];
	rfloat          d[3];
	float           localZ;
	float           targetZ;
	float           lastZ;
	int             numRows;
	int             numCols;
	int             row_i;
	int             col_i;
	int             treeIndex;

	rfloat          colValue;
	rfloat          colFrac;
	float           frac;

	float           blend;
	float           rot[2];
	vec3_t          delta;
	float           localMatrix[4][3];
	float           worldMatrix[4][3];
	vec3_t          start, end;
	trace_t         trace;

	turretNum = cent->currentState.otherEntityNum;
	if ( turretNum < MAX_CLIENTS ) {
		return;
	}
	if ( turretNum == ENTITYNUM_NONE ) {
		return;
	}

	ci = &bg_clientinfo[cent->currentState.clientNum];
	if ( !ci->infoValid ) {
		return;
	}
	// legs, not torso
	if ( !ci->legs.animationNumber ) {
		return;
	}
	anim = ci->legs.animation;
	if ( !anim ) {
		return;
	}
	if ( !( anim->flags & 4 ) ) {
		return;
	}

	turret = &cg_entities[turretNum];
	if ( !turret->currentValid ) {
		return;
	}

	obj = trap_GetClientDObj( turret->currentState.number );
	if ( !obj ) {
		return;
	}

	tagMatrix = CG_DObjGetLocalTagMatrix( obj, "tag_weapon", turret );
	if ( !tagMatrix ) {
		Com_Printf( "WARNING: aborting player positioning on turret since 'tag_weapon' does not exist\n" );
		return;
	}

	weapon = BG_GetInfoForWeapon( turret->currentState.weapon );
	animTree = ci->animTree;

	// keep: own local, matches original codegen
	treeIndex = bgs_animScriptData.animTreeIndex;
	rootAnim.anims = (unsigned short)Scr_GetAnimsIndex( treeIndex );
	rootAnim.index = (unsigned short)( ci->legs.animationNumber & ~ANIM_TOGGLEBIT );

	yaw = vectosignedyaw( tagMatrix );

	AnglesToAxis( turret->lerpAngles, axis );
	VectorCopy( turret->lerpOrigin, axis[3] );

	// keep: rfloat differences and the z, y, x sum order match the original
	d[0] = (rfloat)cent->lerpOrigin[0] - axis[3][0];
	d[1] = (rfloat)cent->lerpOrigin[1] - axis[3][1];
	d[2] = (rfloat)cent->lerpOrigin[2] - axis[3][2];
	localZ = axis[2][2] * d[2] + axis[2][1] * d[1] + axis[2][0] * d[0];
	targetZ = localZ - tagMatrix[14];

	trap_XAnimClearTreeGoalWeightsStrict( animTree, rootAnim, 0.0f );

	numRows = trap_XAnimGetNumChildren( rootAnim );

	lastZ = 0.0f;
	lastFrac = 0.0f;
	lastCol = 0;
	nextCol.index = 0;
	nextCol.anims = 0;

	if ( !numRows ) {
		Com_Error( 1, "\x15Player anim '%s' has no children",
				   trap_XAnimGetAnimName( rootAnim ) );
	}

	row_i = 0;
	do {
		row = trap_XAnimGetChildAt( rootAnim, row_i );
		trap_XAnimSetGoalWeight( animTree, row.index, 1.0f, 1.0f, 1.0f, 0, 0 );

		numCols = trap_XAnimGetNumChildren( row );
		if ( !numCols ) {
			Com_Error( 1, "\x15Player anim '%s' has no children",
					   trap_XAnimGetAnimName( row ) );
		}

		colValue = numCols * 0.5f - (rfloat)yaw / weapon->animHorRotateInc;
		if ( colValue < 0.0f ) {
			colValue = 0.0f;
		} else if ( colValue >= (float)( numCols - 1 ) ) {
			colValue = (float)( numCols - 1 );
		}

		col_i = (int)colValue;
		colFrac = colValue - col_i;
		frac = FCAST( colFrac );

		col = trap_XAnimGetChildAt( row, col_i );
		trap_XAnimSetGoalWeight( animTree, col.index, FCAST( 1.0f - colFrac ), 1.0f, 1.0f, 0, 0 );

		if ( frac != 0.0f ) {
			nextCol = trap_XAnimGetChildAt( row, col_i + 1 );
			trap_XAnimSetGoalWeight( animTree, nextCol.index, frac, 1.0f, 1.0f, 0, 0 );
		}

		trap_XAnimCalcAbsDelta( animTree, row.index, rot, delta );

		if ( delta[2] >= targetZ ) {
			break;
		}

		lastZ = delta[2];
		lastFrac = frac;
		lastCol = col_i;
	} while ( ++row_i < numRows );

	trap_XAnimClearTreeGoalWeightsStrict( animTree, rootAnim, 0.0f );

	SET_TURRET_GOAL_WEIGHT( animTree, col, 1.0f - frac );
	if ( frac != 0.0f ) {
		SET_TURRET_GOAL_WEIGHT( animTree, nextCol, frac );
	}

	if ( row_i && row_i != numRows ) {
		// blend the two rows the target height falls between
		blend = ( targetZ - lastZ ) / ( delta[2] - lastZ );
		SET_TURRET_GOAL_WEIGHT( animTree, row, blend );

		// the lower row reuses row
		row = trap_XAnimGetChildAt( rootAnim, row_i - 1 );
		SET_TURRET_GOAL_WEIGHT( animTree, row, 1.0f - blend );

		col = trap_XAnimGetChildAt( row, lastCol );
		SET_TURRET_GOAL_WEIGHT( animTree, col, 1.0f - lastFrac );

		if ( lastFrac != 0.0f ) {
			nextCol = trap_XAnimGetChildAt( row, lastCol + 1 );
			SET_TURRET_GOAL_WEIGHT( animTree, nextCol, lastFrac );
		}
	} else {
		if ( !CG_DObjGetLocalTagMatrix( obj, "tag_aim", turret ) ) {
			Com_Printf( "WARNING: aborting player positioning on turret since 'tag_aim' does not exist\n" );
			return;
		}
		SET_TURRET_GOAL_WEIGHT( animTree, row, 1.0f );
	}

	trap_XAnimCalcAbsDelta( animTree, rootAnim.index, rot, delta );

	RotateVector2D( delta, yaw );

	localMatrix[3][0] = delta[0] + tagMatrix[12];
	localMatrix[3][1] = delta[1] + tagMatrix[13];
	localMatrix[3][2] = localZ;
	YawToAxis( RotationToYaw( rot ) + yaw, localMatrix );

	// the local offset is transformed into the turret's frame
	MatrixMultiply43( localMatrix, axis, worldMatrix );
	AxisToAngles( worldMatrix, cent->lerpAngles );

	if ( cg_debuganim.integer == 5 ) {
		return;
	}

	VectorCopy( worldMatrix[3], cent->lerpOrigin );

	// drop the player onto the floor under the turret
	VectorCopy( cent->lerpOrigin, start );
	VectorCopy( cent->lerpOrigin, end );
	start[2] = turret->lerpOrigin[2];

	CG_Trace( &trace, start, NULL, NULL, end, cent->currentState.number,
			  MASK_PLAYERSOLID_CG );
	if ( trace.fraction < 1.0f ) {
		cent->lerpOrigin[2] = trace.endpos[2];
	}
}

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	entityState_t   *es;
	int             clientNum;
	refEntity_t     ent;
	DObj            *obj;
	int             renderfx;
	float           shadowPlane;

	es = &cent->nextState;

	// if set to invisible, skip
	if ( es->eFlags & EF_NODRAW ) {
		return;
	}
	if ( es->eFlags & EF_DEAD ) {
		return;
	}

	clientNum = es->clientNum;

	obj = trap_GetClientDObj( es->number );
	if ( !obj ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;

	BG_PlayerAnimation( obj, es, &bg_clientinfo[clientNum] );

	if ( es->eFlags & EF_MOUNTED ) {
		CG_PlayerTurretPositionAndBlend( cent );
	}

	AnglesToAxis( cent->lerpAngles, ent.axis );

	CG_PlayerSprites( cent );
	CG_PlayerSplash( cent );

	renderfx = 0;
	if ( ( cg.snap->ps.pm_flags & PMF_FIRSTPERSON )
		 && es->number == cg.snap->ps.clientNum
		 && !cg.renderingThirdPerson ) {
		renderfx = RF_THIRD_PERSON;         // only draw in mirrors
	}
	if ( cg.cameraMode ) {
		renderfx &= ~RF_THIRD_PERSON;
	}
	renderfx |= RF_LIGHTING_ORIGIN;         // use the same origin for all parts

	VectorCopy( cent->lerpOrigin, ent.origin );

	ent.lightingOrigin[0] = ent.origin[0];
	ent.lightingOrigin[1] = ent.origin[1];
	if ( es->eFlags & EF_PRONE ) {
		ent.lightingOrigin[2] = ent.origin[2] + es->fTorsoHeight + 12.0f;
	} else if ( es->eFlags & EF_CROUCHING ) {
		ent.lightingOrigin[2] = ent.origin[2] + es->fTorsoHeight + 20.0f;
	} else {
		ent.lightingOrigin[2] = ent.origin[2] + es->fTorsoHeight + 32.0f;
	}

	ent.reType = RT_DOBJ;
	ent.dobj = obj;
	ent.owner = cent;
	ent.renderfx = renderfx;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	CG_PlayerShadow( &shadowPlane, cent );

	if ( !( es->eFlags & EF_DEAD ) ) {
		CG_AddPlayerWeapon( &ent, NULL, cent, 1 );
	}
}

/*
===============
CG_Corpse

A corpse keeps its own clientInfo_t in cgs.corpseinfo, so its DObj
is brought up to date here.
===============
*/
void CG_Corpse( centity_t *cent ) {
	entityState_t   *es;
	clientInfo_t    *ci;
	refEntity_t     ent;
	DObj            *obj;

	es = &cent->nextState;

	if ( es->eFlags & EF_NODRAW ) {
		return;
	}

	ci = &cgs.corpseinfo[es->number - MAX_CLIENTS];

	BG_UpdatePlayerDObj( trap_GetClientDObj( es->number ), es, ci );

	obj = trap_GetClientDObj( es->number );
	if ( !obj ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;

	BG_PlayerAnimation( obj, es, ci );

	AnglesToAxis( cent->lerpAngles, ent.axis );

	VectorCopy( cent->lerpOrigin, ent.origin );

	ent.lightingOrigin[0] = ent.origin[0];
	ent.lightingOrigin[1] = ent.origin[1];
	if ( es->eFlags & EF_PRONE ) {
		ent.lightingOrigin[2] = ent.origin[2] + es->fTorsoHeight + 12.0f;
	} else if ( es->eFlags & EF_CROUCHING ) {
		ent.lightingOrigin[2] = ent.origin[2] + es->fTorsoHeight + 20.0f;
	} else {
		ent.lightingOrigin[2] = ent.origin[2] + es->fTorsoHeight + 32.0f;
	}

	ent.reType = RT_DOBJ;
	ent.dobj = obj;
	ent.owner = cent;
	ent.renderfx = RF_LIGHTING_ORIGIN;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_UpdatePlayerDObj
===============
*/
void CG_UpdatePlayerDObj( centity_t *cent ) {
	entityState_t *es;

	if ( !cent->currentValid ) {
		return;
	}

	es = &cent->nextState;
	BG_UpdatePlayerDObj( trap_GetClientDObj( es->clientNum ), es, &bg_clientinfo[es->clientNum] );
}

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
	clientInfo_t *ci;
	void         *tree;

	ci = &bg_clientinfo[cent->currentState.clientNum];

	if ( !( cent->currentState.eFlags & EF_DEAD ) ) {
		tree = ci->animTree;
		trap_XAnimClearTreeGoalWeights( tree, bgs_rootAnim.index, 0.0f );
		trap_XAnimSetCompleteGoalWeight( tree, bgs_animScriptData.torsoAnim,
										 0.0f, 0.0f, 1.0f, 0, 0 );
		trap_XAnimSetCompleteGoalWeight( tree, bgs_animScriptData.legsAnim,
										 1.0f, 0.0f, 1.0f, 0, 0 );
		trap_XAnimSetCompleteGoalWeight( tree, bgs_animScriptData.turningAnim,
										 0.0f, 0.0f, 1.0f, 0, 0 );

		memset( &ci->legs, 0, sizeof( ci->legs ) );
		ci->legs.yawAngle = ci->viewYaw;
		ci->legs.yawing = qfalse;
		ci->legs.pitchAngle = 0;
		ci->legs.pitching = qfalse;

		memset( &ci->torso, 0, sizeof( ci->torso ) );
		ci->torso.yawAngle = ci->viewYaw;
		ci->torso.yawing = qfalse;
		ci->torso.pitchAngle = ci->viewPitch;
		ci->torso.pitching = qfalse;
	}

	if ( cg_debugposition.integer ) {
		CG_Printf( "%i ResetPlayerEntity yaw=%i\n", cent->currentState.number,
				   ci->torso.yawAngle );
	}
}
