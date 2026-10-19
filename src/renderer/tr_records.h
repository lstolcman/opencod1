/*
 * @fidelity: verified
 */
#ifndef TR_RECORDS_H
#define TR_RECORDS_H

#define BACKEND   ((char *)(void *)backEndStorage)      /* 0x016D89C0, backEnd_t,   6,732 */
#define TRG       ((char *)(void *)&tr_registered)   /* 0x016C4D40, trGlobals_t, 80,852 */

#define backEnd_viewParms_viewportX        (*(int *)(BACKEND + 0x02B0))
#define backEnd_viewParms_viewportY        (*(int *)(BACKEND + 0x02B4))
#define backEnd_viewParms_viewportWidth    (*(int *)(BACKEND + 0x02B8))
#define backEnd_viewParms_viewportHeight   (*(int *)(BACKEND + 0x02BC))
#define backEnd_viewParms_projectionMatrix (*(float (*)[16])(BACKEND + 0x02D0))

typedef struct {
    float origin[3];        /* +0x00 */
    float axis[3][3];       /* +0x0C */
    float viewOrigin[3];    /* +0x30 */
    float modelMatrix[16];  /* +0x3C .. +0x7B */
} tr_orientationr_t;

#define backEnd_or   (*(tr_orientationr_t *)(BACKEND + 0x03E8))

/* ---- backEnd.refdef  0x016D89C0..0x016D8B47  392 bytes  -- 16 shards ---- */
#define backEnd_refdef_viewaxis10                  (*(float *)(BACKEND + 0x0030))
#define backEnd_refdef_viewaxis11                  (*(float *)(BACKEND + 0x0034))
#define backEnd_refdef_viewaxis12                  (*(float *)(BACKEND + 0x0038))
#define backEnd_refdef_viewaxis20                  (*(float *)(BACKEND + 0x003C))
#define backEnd_refdef_viewaxis21                  (*(float *)(BACKEND + 0x0040))
#define backEnd_refdef_viewaxis22                  (*(float *)(BACKEND + 0x0044))
#define backEnd_refdef_time          (*(int *)(BACKEND + 0x0048))
#define backEnd_refdef_rdflags                (*(int *)(BACKEND + 0x004C))
#define backEnd_refdef_floatTime     (*(float *)(BACKEND + 0x0050))
#define backEnd_refdef_text                 (*(char (*)[308])(BACKEND + 0x0054))
#define backEnd_refdef_entities                (*(int *)(BACKEND + 0x0158))
#define backEnd_refdef_num_dlights                (*(int *)(BACKEND + 0x015C))
#define backEnd_refdef_entityDlightCount                (*(int *)(BACKEND + 0x0160))
#define backEnd_refdef_dlights                (*(int *)(BACKEND + 0x0164))
#define backEnd_refdef_coronaCount                (*(int *)(BACKEND + 0x0168))
#define backEnd_refdef_coronas                (*(int *)(BACKEND + 0x016C))
/* ---- backEnd.viewParms  0x016D8B48..0x016D8DA7  608 bytes  -- 23 shards ---- */
#define backEnd_viewParms_originX                  (*(float (*)[152])(BACKEND + 0x0188))
#define backEnd_viewParms_originY                  (*(float *)(BACKEND + 0x018C))
#define backEnd_viewParms_originZ                  (*(float *)(BACKEND + 0x0190))
#define backEnd_viewParms_axis00                  (*(float *)(BACKEND + 0x0194))
#define backEnd_viewParms_axis01                  (*(float *)(BACKEND + 0x0198))
#define backEnd_viewParms_axis02                  (*(float *)(BACKEND + 0x019C))
#define backEnd_viewParms_axis10                  (*(float *)(BACKEND + 0x01A0))
#define backEnd_viewParms_axis11                  (*(float *)(BACKEND + 0x01A4))
#define backEnd_viewParms_axis12                  (*(float *)(BACKEND + 0x01A8))
#define backEnd_viewParms_axis20                  (*(float *)(BACKEND + 0x01AC))
#define backEnd_viewParms_axis21                  (*(float *)(BACKEND + 0x01B0))
#define backEnd_viewParms_axis22                  (*(float *)(BACKEND + 0x01B4))
#define backEnd_viewParms_world                  (*(unsigned char (*)[124])(BACKEND + 0x0204))
#define backEnd_viewParms_worldModelMatrix                  (*(float (*)[16])(BACKEND + 0x0240))
#define backEnd_wireframeOverride                (*(int *)(BACKEND + 0x028C))
#define backEnd_viewParms_isMirror   (*(int *)(BACKEND + 0x0290))
#define backEnd_viewParms_frameSceneNum                (*(int *)(BACKEND + 0x0294))
#define backEnd_viewParms_frameCount                (*(int *)(BACKEND + 0x0298))
#define backEnd_viewParms_fovX                  (*(float *)(BACKEND + 0x02C0))
#define backEnd_viewParms_depthHackProjectionMatrix                  (*(float (*)[16])(BACKEND + 0x0310))
#define backEnd_viewParms_zFar                (*(int *)(BACKEND + 0x03A0))
#define backEnd_viewParms_glFogRegistered                (*(int *)(BACKEND + 0x03D8))
#define backEnd_viewParms_glFogDrawSky                (*(int *)(BACKEND + 0x03DC))
/* ---- tr.viewParms  0x016C5350..0x016C55AF  608 bytes  -- 59 shards ---- */
#define tr_viewParms_originX                  (*(float *)(TRG + 0x0610))
#define tr_viewParms_originY                  (*(float *)(TRG + 0x0614))
#define tr_viewParms_originZ                  (*(float *)(TRG + 0x0618))
#define tr_viewParms_axis00                (*(int *)(TRG + 0x061C))
#define tr_viewParms_axis01                (*(int *)(TRG + 0x0620))
#define tr_viewParms_axis02                (*(int *)(TRG + 0x0624))
#define tr_viewParms_axis10                (*(int *)(TRG + 0x0628))
#define tr_viewParms_axis11                (*(int *)(TRG + 0x062C))
#define tr_viewParms_axis12                (*(int *)(TRG + 0x0630))
#define tr_viewParms_axis20                (*(int *)(TRG + 0x0634))
#define tr_viewParms_axis21                (*(int *)(TRG + 0x0638))
#define tr_viewParms_axis22                (*(int *)(TRG + 0x063C))
#define tr_viewParms_world                  (*(unsigned char (*)[124])(TRG + 0x068C))
#define tr_viewParms_world_modelMatrix                  (*(float (*)[16])(TRG + 0x06C8))
#define tr_viewParms_isPortal                (*(int *)(TRG + 0x0714))
#define tr_viewParms_isMirror                (*(int *)(TRG + 0x0718))
#define tr_viewParms_frameSceneNum                (*(int *)(TRG + 0x071C))
#define tr_viewParms_frameCount                (*(int *)(TRG + 0x0720))
#define tr_viewParms_portalPlane                  (*(unsigned char (*)[332])(TRG + 0x0724))
#define tr_viewParms_fovX                  (*(float *)(TRG + 0x0748))
#define tr_viewParms_fovY                  (*(float *)(TRG + 0x074C))
#define tr_viewParms_lodBias                  (*(float *)(TRG + 0x0750))
#define tr_viewParms_lodScale                  (*(float *)(TRG + 0x0754))
#define tr_viewParms_projectionMatrix                  (*(float (*)[16])(TRG + 0x0758))
#define tr_viewParms_depthHackProjectionMatrix                  (*(float (*)[16])(TRG + 0x0798))
#define tr_viewParms_frustum                (*(int (*)[38])(TRG + 0x07D8))
#define tr_viewParms_frustum0NormalY                (*(int (*)[37])(TRG + 0x07DC))
#define tr_viewParms_zFar                  (*(float *)(TRG + 0x0828))
#define tr_viewParms_dirty                  (*(float *)(TRG + 0x082C))
#define tr_viewParms_glFog                (*(int (*)[16])(TRG + 0x0830))
/* ---- tr.refdef  0x016C5638..0x016C57BF  392 bytes  -- 34 shards ---- */
#define tr_refdef_x                (*(int *)(TRG + 0x08F8))
#define tr_refdef_y                (*(int *)(TRG + 0x08FC))
#define tr_refdef_width                (*(int *)(TRG + 0x0900))
#define tr_refdef_height                (*(int *)(TRG + 0x0904))
#define tr_refdef_fov_x                  (*(float *)(TRG + 0x0908))
#define tr_refdef_fov_y                  (*(float *)(TRG + 0x090C))
#define tr_refdef_vieworgX                  (*(float *)(TRG + 0x0910))
#define tr_refdef_vieworgY                  (*(float *)(TRG + 0x0914))
#define tr_refdef_vieworgZ                  (*(float *)(TRG + 0x0918))
#define tr_refdef_viewaxis00                (*(int *)(TRG + 0x091C))
#define tr_refdef_viewaxis01                (*(int *)(TRG + 0x0920))
#define tr_refdef_viewaxis02                (*(int *)(TRG + 0x0924))
#define tr_refdef_viewaxis10                  (*(float *)(TRG + 0x0928))
#define tr_refdef_viewaxis11                  (*(float *)(TRG + 0x092C))
#define tr_refdef_viewaxis12                  (*(float *)(TRG + 0x0930))
#define tr_refdef_viewaxis20                (*(int *)(TRG + 0x0934))
#define tr_refdef_viewaxis21                (*(int *)(TRG + 0x0938))
#define tr_refdef_viewaxis22                (*(int *)(TRG + 0x093C))
#define tr_refdef_time                (*(int *)(TRG + 0x0940))
#define tr_refdef_rdflags                (*(int *)(TRG + 0x0944))
#define tr_refdef_floatTime                  (*(float *)(TRG + 0x0948))
#define tr_refdef_num_entities                (*(int *)(TRG + 0x0A4C))
#define tr_refdef_entities                (*(int *)(TRG + 0x0A50))
#define tr_refdef_num_dlights                (*(int *)(TRG + 0x0A54))
#define tr_refdef_entityDlightCount                (*(int *)(TRG + 0x0A58))
#define tr_refdef_dlights                (*(int *)(TRG + 0x0A5C))
#define tr_refdef_coronaCount                (*(int *)(TRG + 0x0A60))
#define tr_refdef_coronas                (*(int *)(TRG + 0x0A64))
#define tr_refdef_numPolys                (*(int *)(TRG + 0x0A68))
#define tr_refdef_polys                (*(int *)(TRG + 0x0A6C))
#define tr_refdef_numDrawSurfs                (*(int *)(TRG + 0x0A70))
#define tr_refdef_drawSurfs                (*(int *)(TRG + 0x0A74))
#define tr_refdef_entitySurfaceCount                (*(int *)(TRG + 0x0A78))
#define tr_refdef_entitySurfaces                (*(int *)(TRG + 0x0A7C))

#define tr_or          (*(float *)(TRG + 0x087C))
#define tr_or_originY          (*(float *)(TRG + 0x0880))
#define tr_or_originZ          (*(float *)(TRG + 0x0884))
#define tr_or_axis00        (*(int *)(TRG + 0x0888))
#define tr_or_axis01          (*(float *)(TRG + 0x088C))
#define tr_or_axis02          (*(float *)(TRG + 0x0890))
#define tr_or_axis10          (*(float *)(TRG + 0x0894))
#define tr_or_axis11        (*(int *)(TRG + 0x0898))
#define tr_or_axis12          (*(float *)(TRG + 0x089C))
#define tr_or_axis20          (*(float *)(TRG + 0x08A0))
#define tr_or_axis21          (*(float *)(TRG + 0x08A4))
#define tr_or_axis22        (*(int *)(TRG + 0x08A8))
#define tr_or_viewOriginX        (*(int *)(TRG + 0x08AC))
#define tr_or_viewOriginY        (*(int *)(TRG + 0x08B0))
#define tr_or_viewOriginZ        (*(int *)(TRG + 0x08B4))
#define tr_or_modelMatrix          (*(float (*)[16])(TRG + 0x08B8))

#define FOGBASE   ((char *)(void *)&tr_fogTable)

#define tr_fogCurrent_mode   (*(int *)(FOGBASE + 320))   /* fog[5], base+0x140 */

#undef  tr_fogSky
#define tr_fogSky      (*(unsigned char *)(FOGBASE + 0x040))
#undef  tr_fogSky_registered
#define tr_fogSky_registered    (*(int *)(FOGBASE + 0x070))
#undef  tr_fogPortalView
#define tr_fogPortalView      (*(unsigned char *)(FOGBASE + 0x080))
#undef  tr_fogPortalView_registered
#define tr_fogPortalView_registered    (*(int *)(FOGBASE + 0x0B0))
#undef  tr_fogPortalView_clearScreen
#define tr_fogPortalView_clearScreen    (*(int *)(FOGBASE + 0x0B8))
#undef  tr_fogReset_registered
#define tr_fogReset_registered    (*(int *)(FOGBASE + 0x0F0))
#undef  tr_fogMap_registered
#define tr_fogMap_registered    (*(int *)(FOGBASE + 0x130))
#undef  tr_fogCurrent_mode
#define tr_fogCurrent_mode    (*(int *)(FOGBASE + 0x140))
#undef  tr_fogCurrent_colorR
#define tr_fogCurrent_colorR      (*(float *)(FOGBASE + 0x150))
#undef  tr_fogCurrent_colorG
#define tr_fogCurrent_colorG      (*(float *)(FOGBASE + 0x154))
#undef  tr_fogCurrent_colorB
#define tr_fogCurrent_colorB      (*(float *)(FOGBASE + 0x158))
#undef  tr_fogCurrent_colorA
#define tr_fogCurrent_colorA      (*(float *)(FOGBASE + 0x15C))
#undef  tr_fogCurrent_end
#define tr_fogCurrent_end      (*(float *)(FOGBASE + 0x164))
#undef  tr_fogCurrent_registered
#define tr_fogCurrent_registered    (*(int *)(FOGBASE + 0x170))
#undef  tr_fogCurrent_drawSky
#define tr_fogCurrent_drawSky    (*(int *)(FOGBASE + 0x174))
#undef  tr_fogCurrent_clearScreen
#define tr_fogCurrent_clearScreen    (*(int *)(FOGBASE + 0x178))
#undef  tr_fogTarget_registered
#define tr_fogTarget_registered    (*(int *)(FOGBASE + 0x1F0))


#define SUNBASE   ((char *)(void *)&rendererSunState)

#define rendererSunState_spriteShader          (*(int *)(SUNBASE +   0))
#define rendererSunState_spriteVert0X            (*(float *)(SUNBASE +   4))
#define rendererSunState_spriteVert0Y            (*(float *)(SUNBASE +   8))
#define rendererSunState_spriteVert0Z            (*(float *)(SUNBASE +  12))
#define rendererSunState_spriteVert0W          (*(int *)(SUNBASE +  16))
#define rendererSunState_spriteVert1X            (*(float *)(SUNBASE +  20))
#define rendererSunState_spriteVert1Y            (*(float *)(SUNBASE +  24))
#define rendererSunState_spriteVert1Z            (*(float *)(SUNBASE +  28))
#define rendererSunState_spriteVert1W          (*(int *)(SUNBASE +  32))
#define rendererSunState_spriteVert2X            (*(float *)(SUNBASE +  36))
#define rendererSunState_spriteVert2Y            (*(float *)(SUNBASE +  40))
#define rendererSunState_spriteVert2Z            (*(float *)(SUNBASE +  44))
#define rendererSunState_spriteVert2W          (*(int *)(SUNBASE +  48))
#define rendererSunState_spriteVert3X            (*(float *)(SUNBASE +  52))
#define rendererSunState_spriteVert3Y            (*(float *)(SUNBASE +  56))
#define rendererSunState_spriteVert3Z            (*(float *)(SUNBASE +  60))
#define rendererSunState_spriteVert3W          (*(int *)(SUNBASE +  64))
#define rendererSunState_spriteSize            (*(float *)(SUNBASE +  68))
#define rendererSunState_flareShader          (*(int *)(SUNBASE +  72))
#define rendererSunState_flareMinHalfSize            (*(float *)(SUNBASE +  76))
#define rendererSunState_flareMinCosAngle            (*(float *)(SUNBASE +  80))
#define rendererSunState_flareMaxHalfSize            (*(float *)(SUNBASE +  84))
#define rendererSunState_flareMaxCosAngle            (*(float *)(SUNBASE +  88))
#define rendererSunState_flareMaxAlpha          (*(int *)(SUNBASE +  92))
#define rendererSunState_flareFadeInMsec          (*(int *)(SUNBASE +  96))
#define rendererSunState_flareFadeOutMsec          (*(int *)(SUNBASE + 100))
#define rendererSunState_blindMinCosAngle            (*(float *)(SUNBASE + 104))
#define rendererSunState_blindMaxCosAngle            (*(float *)(SUNBASE + 108))
#define rendererSunState_blindMaxDarken          (*(int *)(SUNBASE + 112))
#define rendererSunState_blindFadeInMsec          (*(int *)(SUNBASE + 116))
#define rendererSunState_blindFadeOutMsec          (*(int *)(SUNBASE + 120))
#define rendererSunState_glareMinCosAngle            (*(float *)(SUNBASE + 124))
#define rendererSunState_glareMaxCosAngle            (*(float *)(SUNBASE + 128))
#define rendererSunState_glareMaxLighten          (*(int *)(SUNBASE + 132))
#define rendererSunState_glareFadeInMsec          (*(int *)(SUNBASE + 136))
#define rendererSunState_glareFadeOutMsec          (*(int *)(SUNBASE + 140))
#define rendererSunState_currentBlindFraction            (*(float *)(SUNBASE + 144))
#define rendererSunState_currentGlareFraction            (*(float *)(SUNBASE + 148))
#define rendererSunState_lastUpdateTime          (*(int *)(SUNBASE + 152))

#define WLDBASE   ((char *)(void *)&s_worldData)

#define s_worldData_nameTerminator           (*(unsigned char *)(WLDBASE +  63))
#define s_worldData_baseName           (*(char *)(WLDBASE +  64))
#define s_worldData_baseNameTerminator           (*(unsigned char *)(WLDBASE + 127))
#define s_worldData_dataSize          (*(int *)(WLDBASE + 128))
#define s_worldData_numShaders          (*(int *)(WLDBASE + 132))
#define s_worldData_shaders          (*(int *)(WLDBASE + 136))
#define s_worldData_bmodels          (*(int *)(WLDBASE + 140))
#define s_worldData_numnodes          (*(int *)(WLDBASE + 144))
#define s_worldData_numDecisionNodes          (*(int *)(WLDBASE + 148))
#define s_worldData_nodes          (*(int *)(WLDBASE + 152))
#define s_worldData_numsurfaces          (*(int *)(WLDBASE + 156))
#define s_worldData_surfaces          (*(int *)(WLDBASE + 160))
#define s_worldData_skySurfaceCount          (*(int *)(WLDBASE + 164))
#define s_worldData_skySurfaces          (*(int *)(WLDBASE + 168))
#define s_worldData_aabbTreeCount          (*(int *)(WLDBASE + 172))
#define s_worldData_aabbTrees          (*(int *)(WLDBASE + 176))
#define s_worldData_numClusters          (*(int *)(WLDBASE + 180))
#define entityParseBufferHead          (*(int *)(WLDBASE + 184))
#define s_worldData_coronas          (*(int *)(WLDBASE + 192))
#define s_worldData_coronaCount          (*(int *)(WLDBASE + 196))
#define s_worldData_skyVertexStorage            (*(unsigned char *)(WLDBASE + 200))
#define s_worldData_entityAmbientBaseR            (*(float *)(WLDBASE + 216))
#define s_worldData_entityAmbientBaseG            (*(float *)(WLDBASE + 220))
#define s_worldData_entityAmbientBaseB            (*(float *)(WLDBASE + 224))
#define s_worldData_entityAmbientBaseA          (*(int *)(WLDBASE + 228))
#define s_worldData_sunDiffuseColorR            (*(float *)(WLDBASE + 232))
#define s_worldData_sunDiffuseColorG            (*(float *)(WLDBASE + 236))
#define s_worldData_sunDiffuseColorB            (*(float *)(WLDBASE + 240))
#define s_worldData_sunDiffuseColorA          (*(int *)(WLDBASE + 244))
#define s_worldData_entityAmbientScaleR            (*(float *)(WLDBASE + 248))
#define s_worldData_entityAmbientScaleG            (*(float *)(WLDBASE + 252))
#define s_worldData_entityAmbientScaleB            (*(float *)(WLDBASE + 256))
#define s_worldData_entityAmbientScaleA          (*(int *)(WLDBASE + 260))
#define s_worldData_entitySunLightIntensity            (*(float *)(WLDBASE + 264))
#define s_worldData_lightCount          (*(int *)(WLDBASE + 268))
#define s_worldData_lights          (*(int *)(WLDBASE + 272))
#define s_worldData_sunLight          (*(int *)(WLDBASE + 276))
#define s_worldData_lightIndexCount          (*(int *)(WLDBASE + 280))
#define s_worldData_lightIndexes          (*(int *)(WLDBASE + 284))
#define s_worldData_cellCount          (*(int *)(WLDBASE + 288))
#define s_worldData_cells          (*(int *)(WLDBASE + 292))
#define s_worldData_occluderCount          (*(int *)(WLDBASE + 296))
#define s_worldData_occluders          (*(int *)(WLDBASE + 300))
#define s_worldData_occluderIndexCount          (*(int *)(WLDBASE + 304))
#define s_worldData_occluderIndexes          (*(int *)(WLDBASE + 308))
#define s_worldData_portalCount          (*(int *)(WLDBASE + 312))
#define s_worldData_portals          (*(int *)(WLDBASE + 316))
#define s_worldData_portalVerts          (*(int *)(WLDBASE + 320))
#define s_worldData_cullGroupCount          (*(int *)(WLDBASE + 324))
#define s_worldData_cullGroups          (*(int *)(WLDBASE + 328))
#define s_worldData_cullGroupIndexCount          (*(int *)(WLDBASE + 332))
#define s_worldData_cullGroupIndexes          (*(int *)(WLDBASE + 336))

#define tr_sunName     (*(unsigned char *)(TRG + 0x00E0))
#define tr_sunName_63   (*(unsigned char *)(TRG + 0x011F))

/* Registration state R_RegisterStaticModel, R_RegisterModel and the image loader share:
 * fields of tr in retail, so their stores keep retail's order against each other. */
#define tr_ignorePrecacheErrorCount     (*(int *)(TRG + 0x0120))   /* 0x016C4E60 */
#define tr_modelsFinishedLoading        (*(int *)(TRG + 0x0124))   /* 0x016C4E64 */
#define tr_delayedImageGroup            (*(int *)(TRG + 0x0128))   /* 0x016C4E68 */
#define tr_delayedImageGroupTriCount    (*(int *)(TRG + 0x012C))   /* 0x016C4E6C */
#define tr_delayedImageGroupSequence    (*(int *)(TRG + 0x0134))   /* 0x016C4E74 */
#define tr_registeredStaticModels       (*(int (*)[2048])(TRG + 0x2AF8))   /* 0x016C7838; R_RegisterStaticModel caps at 2048 */
#define tr_registeredStaticModelCount   (*(int *)(TRG + 0x4AF8))   /* 0x016C9838 */

#endif
