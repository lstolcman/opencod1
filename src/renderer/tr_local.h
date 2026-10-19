/*
 * @fidelity: verified
 *
 * tr_local.h -- typed views over the renderer's data structures.
 *
 * The retail renderer keeps its state in a few large blobs (trGlobals_t in
 * tr_registered, backEnd_t in backEndStorage, the fog table, ...) that the
 * decompiled tree reaches through the offset macros in tr_records.h.  Those
 * macros stay the storage of record; this header only adds C types for the
 * data that is passed around by POINTER -- surfaces, nodes, models, lights,
 * entities, draw surfaces -- so functions can say `surf->data` instead of
 * `*(_DWORD *)(a1 + 8)`.
 *
 * Retyping a pointer never moves storage, so the memory layout is unchanged;
 * every field offset below is proven against the retail binary (VERIFIED
 * decompiled reads) and cross-checked with the CoDUO reconstruction, and each
 * is pinned with a COD1_STATIC_ASSERT so a wrong offset fails the build
 * instead of misreading memory at runtime.  Names follow RTCW/Q3 where the
 * field has an RTCW ancestor; CoD-specific fields are named from CoDUO.
 */
#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../qcommon/qcommon.h"      /* vec3_t, vec4_t, byte, cplane_t, COD1_STATIC_ASSERT */
#include "tr_public.h"

typedef int qhandle_t;

#define ASSERT_OFF(type, field, off) \
	COD1_STATIC_ASSERT(offsetof(type, field) == (off), \
	                   #type "." #field " must be at " #off)

/* ------------------------------------------------------------------------- *
 *  cull results -- R_CullLocalBox / R_CullPointAndRadius return values
 * ------------------------------------------------------------------------- */
typedef enum {
	CULL_IN   = 0,     /* completely unclipped */
	CULL_CLIP = 1,     /* clipped by one or more planes */
	CULL_OUT  = 2      /* completely outside the frustum */
} cullResult_t;

/* ------------------------------------------------------------------------- *
 *  glfog_t  (0x40) -- one fog record; RTCW glfog_t plus CoD's trailing dirty.
 *  R_Fog operates on a pointer to one of these.
 * ------------------------------------------------------------------------- */
typedef struct {
	int      mode;             /* +0x00  GL_LINEAR / GL_EXP */
	int      hint;             /* +0x04  GL_DONT_CARE ... */
	int      startTime;        /* +0x08  transition start, ms */
	int      finishTime;       /* +0x0c  transition end, ms */
	float    color[4];         /* +0x10  RGBA */
	float    start;            /* +0x20  near */
	float    end;              /* +0x24  far */
	int      useEndForClip;    /* +0x28  qboolean */
	float    density;          /* +0x2c */
	int      registered;       /* +0x30  qboolean -- fog is set up */
	int      drawSky;          /* +0x34  qboolean */
	int      clearScreen;      /* +0x38  qboolean */
	int      dirty;            /* +0x3c  CoD-only */
} glfog_t;
COD1_ASSERT_SIZE(glfog_t, 0x40);
ASSERT_OFF(glfog_t, color, 0x10);
ASSERT_OFF(glfog_t, density, 0x2c);
ASSERT_OFF(glfog_t, registered, 0x30);

/* ------------------------------------------------------------------------- *
 *  orientationr_t (0x7c) and viewParms_t (0x260).  These live inside
 *  trGlobals_t (tr.or, tr.viewParms) and backEnd_t, reached through the
 *  offset macros in tr_records.h.  The macros are individually typed (some
 *  int, some float), which forces a cast on every float access; the typed
 *  views trOrient / trView below let a function read tr.viewParms.frustum[i]
 *  or tr.or.axis[0] directly.  Same bytes, no storage moved.
 * ------------------------------------------------------------------------- */
typedef struct {
	vec3_t   origin;              /* +0x00 */
	vec3_t   axis[3];             /* +0x0c */
	vec3_t   viewOrigin;          /* +0x30  viewParms origin in local coords */
	float    modelMatrix[16];     /* +0x3c */
} orientationr_t;
COD1_ASSERT_SIZE(orientationr_t, 0x7c);

/* the lighter orientation used for portal/mirror surface + camera frames */
typedef struct {
	vec3_t   origin;              /* +0x00 */
	vec3_t   axis[3];             /* +0x0c */
} orientation_t;
COD1_ASSERT_SIZE(orientation_t, 0x30);

typedef struct {
	orientationr_t orientation;                 /* +0x000  ("or" in RTCW) */
	orientationr_t world;                       /* +0x07c */
	vec3_t         pvsOrigin;                   /* +0x0f8 */
	int            isPortal;                    /* +0x104 */
	int            isMirror;                    /* +0x108 */
	int            frameSceneNum;               /* +0x10c */
	int            frameCount;                  /* +0x110 */
	cplane_t       portalPlane;                 /* +0x114 */
	int            viewportX;                   /* +0x128 */
	int            viewportY;                   /* +0x12c */
	int            viewportWidth;               /* +0x130 */
	int            viewportHeight;              /* +0x134 */
	float          fovX;                        /* +0x138 */
	float          fovY;                        /* +0x13c */
	float          lodBias;                     /* +0x140 */
	float          lodScale;                    /* +0x144 */
	float          projectionMatrix[16];        /* +0x148 */
	float          depthHackProjectionMatrix[16]; /* +0x188  CoD-added */
	cplane_t       frustum[4];                  /* +0x1c8  stride 0x14 */
	float          zFar;                        /* +0x218 */
	int            dirty;                       /* +0x21c */
	glfog_t        glFog;                       /* +0x220 */
} viewParms_t;
COD1_ASSERT_SIZE(viewParms_t, 0x260);
ASSERT_OFF(viewParms_t, world, 0x07c);
ASSERT_OFF(viewParms_t, fovX, 0x138);
ASSERT_OFF(viewParms_t, projectionMatrix, 0x148);
ASSERT_OFF(viewParms_t, frustum, 0x1c8);
ASSERT_OFF(viewParms_t, zFar, 0x218);
ASSERT_OFF(viewParms_t, glFog, 0x220);

/* ------------------------------------------------------------------------- *
 *  dlight_t  (0x88) -- CoD's large per-light record (CoDUO renderer_light_t),
 *  not RTCW's 0x34 dlight_t.  R_DlightTris walks these at a 34-float stride
 *  and reads radius / transformedPosition.
 * ------------------------------------------------------------------------- */
typedef struct {
	int      type;                    /* +0x00  R_LIGHT_TYPE_* */
	vec3_t   color;                   /* +0x04 */
	float    intensity;               /* +0x10 */
	vec4_t   ambient;                 /* +0x14 */
	vec4_t   diffuse;                 /* +0x24 */
	vec4_t   specular;                /* +0x34 */
	vec4_t   position;                /* +0x44 */
	vec3_t   spotDirection;           /* +0x54 */
	float    constantAttenuation;     /* +0x60 */
	float    linearAttenuation;       /* +0x64 */
	float    quadraticAttenuation;    /* +0x68 */
	float    spotExponent;            /* +0x6c */
	float    spotCutoff;              /* +0x70 */
	float    radius;                  /* +0x74 */
	vec3_t   transformedPosition;     /* +0x78  origin in local coords */
	int      overdraw;                /* +0x84  dormant in CoD */
} dlight_t;
COD1_ASSERT_SIZE(dlight_t, 0x88);
ASSERT_OFF(dlight_t, radius, 0x74);
ASSERT_OFF(dlight_t, transformedPosition, 0x78);

/* ------------------------------------------------------------------------- *
 *  Surfaces.
 *
 *  msurface_t (0x0c) is the world-surface handle; its `data` points at a
 *  surface header whose first int is the surface type.  Types < 24 are
 *  entity / model surfaces; types >= SF_INDEXED_FIRST (24) are indexed world
 *  geometry and carry the lit-surface prefix (storage mode, dlight mask,
 *  bounds).  CoD drops RTCW's fogIndex, so `data` sits at +8 and the struct
 *  is 12 bytes.
 * ------------------------------------------------------------------------- */
#define SF_INDEXED_FIRST   24     /* R_SURFACE_WORLD_UNOPTIMIZED; >= this is world geometry */

typedef struct shader_s shader_t;

typedef struct {
	int         surfaceType;      /* +0x00 */
} srfGeneric_t;

/* lit world surface (surfaceType >= SF_INDEXED_FIRST) */
typedef struct {
	int         surfaceType;      /* +0x00 */
	int         storageMode;      /* +0x04  static-vertex memory source */
	unsigned    dlightBits;       /* +0x08  R_DlightTris result */
	vec3_t      boundsMin;        /* +0x0c */
	vec3_t      boundsMax;        /* +0x18 */
} srfLit_t;
ASSERT_OFF(srfLit_t, dlightBits, 0x08);
ASSERT_OFF(srfLit_t, boundsMin, 0x0c);
ASSERT_OFF(srfLit_t, boundsMax, 0x18);

typedef struct {
	int           viewCount;      /* +0x00  == tr.viewCount when already added this view */
	shader_t     *shader;         /* +0x04 */
	srfGeneric_t *data;           /* +0x08  surface header (any srf*_t) */
} msurface_t;
COD1_ASSERT_SIZE(msurface_t, 0x0c);
ASSERT_OFF(msurface_t, data, 0x08);

/* ------------------------------------------------------------------------- *
 *  BSP nodes (0x1c).  Internal nodes have contents == -1 and use plane +
 *  children; leaves reuse the same words for cluster / light info.
 * ------------------------------------------------------------------------- */
#define NODE_INTERNAL   (-1)      /* contents value for a non-leaf node */

typedef struct mnode_s {
	int              contents;        /* +0x00  -1 => internal node */
	struct mnode_s  *parent;          /* +0x04 */
	int              cellIndex;       /* +0x08 */
	union {
		struct {                      /* internal node */
			cplane_t        *plane;       /* +0x0c */
			struct mnode_s  *children[2]; /* +0x10 front, +0x14 back */
		} node;
		struct {                      /* leaf */
			int   cluster;            /* +0x0c */
			int   hasSunLight;        /* +0x10 */
			int   firstLightIndex;    /* +0x14 */
			int   lightCount;         /* +0x18 */
		} leaf;
	} u;
} mnode_t;
ASSERT_OFF(mnode_t, u.node.plane, 0x0c);
ASSERT_OFF(mnode_t, u.node.children, 0x10);

/* ------------------------------------------------------------------------- *
 *  Inline (brush) models.
 * ------------------------------------------------------------------------- */
typedef struct {
	vec3_t       bounds[2];       /* +0x00  mins / maxs */
	msurface_t  *firstSurface;    /* +0x18 */
	int          numSurfaces;     /* +0x1c */
} bmodel_t;
COD1_ASSERT_SIZE(bmodel_t, 0x20);
ASSERT_OFF(bmodel_t, firstSurface, 0x18);
ASSERT_OFF(bmodel_t, numSurfaces, 0x1c);

typedef enum {
	MOD_BAD    = 0,
	MOD_BRUSH  = 1,
	MOD_XMODEL = 2
} modtype_t;

typedef struct {
	char       name[64];          /* +0x00 */
	int        type;              /* +0x40  modtype_t */
	int        index;             /* +0x44  tr.models[index] */
	int        dataSize;          /* +0x48 */
	bmodel_t  *bmodel;            /* +0x4c  when type == MOD_BRUSH */
	void      *shaderHandles;     /* +0x50 */
	void      *xmodel;            /* +0x54 */
	void      *legacyMd3Lod2;     /* +0x58 */
	int        numLods;           /* +0x5c */
} model_t;
COD1_ASSERT_SIZE(model_t, 0x60);
ASSERT_OFF(model_t, type, 0x40);
ASSERT_OFF(model_t, bmodel, 0x4c);

/* ------------------------------------------------------------------------- *
 *  Entities.  refEntity_t is the public prefix; trRefEntity_t adds the
 *  renderer's per-frame lighting/cull state.  Only offsets proven by the
 *  decompiled reads are named; unproven interior ranges are reserved padding
 *  so every named field lands on a verified offset.  Total record is 0x2b8.
 * ------------------------------------------------------------------------- */
/*
 * The cgame builds this record and hands it over through
 * trap_R_AddRefEntityToScene, so the names follow cgame_mp/cg_public.h's
 * refEntity_t (Q3/RTCW names; CoD drops skinNum / customSkin, so everything
 * from customShader on sits 8 bytes below Q3).  Fields the renderer reads are
 * noted with the renderer's use.
 */
typedef struct {
	int        reType;                /* +0x00  RT_* */
	int        renderfx;              /* +0x04  RF_* */
	qhandle_t  hModel;                /* +0x08 */
	vec3_t     lightingOrigin;        /* +0x0c  cg_public.h (CG_LockLightingOrigin); no renderer read */
	float      shadowPlane;           /* +0x18  tr_shadows.c's shadowEntity_t reads it */
	vec3_t     axis[3];               /* +0x1c */
	int        nonNormalizedAxes;     /* +0x40  qboolean (cgame stores 1.0f; R_GetLodDist divides by it) */
	vec3_t     origin;                /* +0x44 */
	int        frame;                 /* +0x50  portal rotation speed (deg/sec) */
	vec3_t     oldorigin;             /* +0x54  portal camera origin */
	int        oldframe;              /* +0x60  portal: nonzero => animated */
	float      radius2;               /* +0x64  sprite vertical radius / cylinder start radius
	                                   * (tr_surface.c); cg_public.h calls this slot spriteSize */
	qhandle_t  customShader;          /* +0x68  sprite/beam shader handle */
	byte       shaderRGBA[4];         /* +0x6c  rgbGen/alphaGen entity colour */
	float      shaderTexCoord[2];     /* +0x70  Q3's; no CoD 1.1 access found */
	float      shaderTime;            /* +0x78 */
	float      radius;                /* +0x7c  sprite radius */
	float      rotation;              /* +0x80  sprite roll / portal rotation offset (deg) */
	vec3_t     fireRiseDir;           /* +0x84  RTCW's member after rotation; no CoD 1.1 access found */
	void      *dobj;                  /* +0x90  DObj of an xmodel entity (tr_xanim.c R_EntDObj) */
	void      *owner;                 /* +0x94  cgame's centity_t; passed to ri.CG_DObjCalcPose as
	                                   * the pose (tr_xanim.c R_EntPose) */
	void      *staticModel;           /* +0x98  smRegisteredModel_t * of a static-model entity
	                                   * (tr_staticmodel.c R_EntStaticModel); cgame leaves it 0 */
} refEntity_t;
ASSERT_OFF(refEntity_t, lightingOrigin, 0x0c);
ASSERT_OFF(refEntity_t, shadowPlane, 0x18);
ASSERT_OFF(refEntity_t, axis, 0x1c);
ASSERT_OFF(refEntity_t, nonNormalizedAxes, 0x40);
ASSERT_OFF(refEntity_t, origin, 0x44);
ASSERT_OFF(refEntity_t, frame, 0x50);
ASSERT_OFF(refEntity_t, oldorigin, 0x54);
ASSERT_OFF(refEntity_t, oldframe, 0x60);
ASSERT_OFF(refEntity_t, radius2, 0x64);
ASSERT_OFF(refEntity_t, customShader, 0x68);
ASSERT_OFF(refEntity_t, shaderRGBA, 0x6c);
ASSERT_OFF(refEntity_t, shaderTexCoord, 0x70);
ASSERT_OFF(refEntity_t, shaderTime, 0x78);
ASSERT_OFF(refEntity_t, radius, 0x7c);
ASSERT_OFF(refEntity_t, rotation, 0x80);
ASSERT_OFF(refEntity_t, fireRiseDir, 0x84);
ASSERT_OFF(refEntity_t, dobj, 0x90);
ASSERT_OFF(refEntity_t, owner, 0x94);
ASSERT_OFF(refEntity_t, staticModel, 0x98);
COD1_ASSERT_SIZE(refEntity_t, 0x9c);

/* one picked-light contribution stored in trRefEntity_t.lights[] */
typedef struct {
	dlight_t *light;              /* +0x00 */
	float     scale;              /* +0x04 */
} lightRef_t;
COD1_ASSERT_SIZE(lightRef_t, 0x08);

/*
 * The lighting block (+0x0a0..+0x2ab) is filled per frame by tr_light.c
 * (R_SetupEntityLighting / R_PickFinalLights) and read by the backends.
 */
typedef struct {
	refEntity_t  e;                       /* +0x000 */
	unsigned     dlightBits;              /* +0x09c */
	byte         hasDynamicLights;        /* +0x0a0  any dynamic light in range */
	byte         lightingCalculated;      /* +0x0a1  lighting already computed this frame */
	byte         reserved_0xa2[2];        /* +0x0a2  no access found (alignment) */
	vec3_t       lightDir;                /* +0x0a4  sun direction, entity-local (stencil shadows) */
	vec3_t       ambientLight;            /* +0x0b0  RTCW's members after lightDir (same sizes); */
	int          ambientLightInt;         /* +0x0bc  no CoD 1.1 access found */
	vec3_t       directedLight;           /* +0x0c0 */
	int          lightCount;              /* +0x0cc  number of picked lights */
	lightRef_t   lights[8];               /* +0x0d0  picked lights, strongest first */
	float        diffuseSun;              /* +0x110  diffuse-sun visibility fraction */
	dlight_t     generatedLights[3];      /* +0x114  synthesised merge/sun lights */
	int          normalizationTarget;     /* +0x2ac  axis-normalization / rescale flag */
	void        *staticModelLighting;     /* +0x2b0 */
	int          cullState;               /* +0x2b4  cullResult_t; != CULL_OUT => visible */
} trRefEntity_t;
ASSERT_OFF(trRefEntity_t, dlightBits, 0x09c);
ASSERT_OFF(trRefEntity_t, hasDynamicLights, 0x0a0);
ASSERT_OFF(trRefEntity_t, lightingCalculated, 0x0a1);
ASSERT_OFF(trRefEntity_t, lightDir, 0x0a4);
ASSERT_OFF(trRefEntity_t, ambientLight, 0x0b0);
ASSERT_OFF(trRefEntity_t, ambientLightInt, 0x0bc);
ASSERT_OFF(trRefEntity_t, directedLight, 0x0c0);
ASSERT_OFF(trRefEntity_t, lightCount, 0x0cc);
ASSERT_OFF(trRefEntity_t, lights, 0x0d0);
ASSERT_OFF(trRefEntity_t, diffuseSun, 0x110);
ASSERT_OFF(trRefEntity_t, generatedLights, 0x114);
ASSERT_OFF(trRefEntity_t, normalizationTarget, 0x2ac);
ASSERT_OFF(trRefEntity_t, cullState, 0x2b4);
COD1_ASSERT_SIZE(trRefEntity_t, 0x2b8);

/* ------------------------------------------------------------------------- *
 *  Draw surfaces and the packed sort key.
 *
 *  sort layout (from R_AddDrawSurf / R_DecomposeSort):
 *    bit  0        : fog-ish flag A
 *    bit  1        : portal flag  (entity => ENTITYNUM_WORLD)
 *    bit  2        : fog-ish flag B
 *    bits 8..17    : entity number        (0x3ff)
 *    bits 18..29   : shader sorted index  (0xfff) -> tr.sortedShaders[]
 *    bits 30..31   : storage class
 * ------------------------------------------------------------------------- */
#define QSORT_ENTITYNUM_SHIFT     8
#define QSORT_ENTITYNUM_MASK      0x3ff
#define QSORT_SHADERNUM_SHIFT     18
#define QSORT_SHADERNUM_MASK      0xfff
#define QSORT_STORAGECLASS_SHIFT  30
#ifndef ENTITYNUM_WORLD
#define ENTITYNUM_WORLD           1022
#endif

typedef struct {
	unsigned    sort;             /* +0x00 */
	void       *surface;          /* +0x04 */
} drawSurf_t;
COD1_ASSERT_SIZE(drawSurf_t, 0x08);

/* ------------------------------------------------------------------------- *
 *  shader_t (0x198).  Field names and offsets follow tr_shader.c's fuller
 *  shaderDef_t (the record GeneratePermanentShader hunk-copies); the stage and
 *  deform records are not typed here, so those members stay opaque.
 * ------------------------------------------------------------------------- */
struct shader_s {
	char       name[64];          /* +0x00 */
	int        lightmapIndex;     /* +0x40 */
	int        index;             /* +0x44 */
	int        sortedIndex;       /* +0x48 */
	unsigned   flags;             /* +0x4c */
	unsigned   surfaceFlags;      /* +0x50 */
	unsigned   lightingFlags;     /* +0x54 */
	float      sort;              /* +0x58 */
	unsigned   surfaceParmFlags;  /* +0x5c */
	float      skyCloudHeight;    /* +0x60 */
	void      *skyOuterBoxImages[6];  /* +0x64 */
	void      *skyInnerBoxImages[6];  /* +0x7c */
	byte       fogParms[0x0c];    /* +0x94  fog colour + params */
	float      fogDepthForOpaque; /* +0xa0 */
	float      portalRange;       /* +0xa4 */
	int        cullType;          /* +0xa8  (tr_xanim.c R_ShaderCull) */
	int        numDeforms;        /* +0xac */
	byte       deforms[0x14c - 0xb0];  /* +0xb0  deformStage_t[3] (tr_shader.c) */
	float      boundsExpansion;   /* +0x14c */
	int        numUnfoggedPasses; /* +0x150  (tr_xanim.c R_ShaderNumStages) */
	struct shaderStage_s *stages[8];   /* +0x154  (tr_xanim.c R_ShaderStages) */
	void      *optimalStageIteratorFunc;  /* +0x174 */
	float      clampTime;         /* +0x178 */
	float      timeOffset;        /* +0x17c  RTCW's member after clampTime (RB_RenderDrawSurfList reads it) */
	struct shader_s *remappedShader;  /* +0x180  used when flags & SHADER_REMAPPED (tr_model.c) */
	void      *mergeImage;        /* +0x184 */
	int        optimizedBackend;  /* +0x188 */
	unsigned   optimizedVertexStorage;        /* +0x18c */
	int        optimizedVertexStorageOffset;  /* +0x190 */
	struct shader_s *next;        /* +0x194 */
};
COD1_ASSERT_SIZE(shader_t, 0x198);
ASSERT_OFF(shader_t, sort, 0x58);
ASSERT_OFF(shader_t, skyOuterBoxImages, 0x64);
ASSERT_OFF(shader_t, fogParms, 0x94);
ASSERT_OFF(shader_t, portalRange, 0xa4);
ASSERT_OFF(shader_t, deforms, 0xb0);
ASSERT_OFF(shader_t, boundsExpansion, 0x14c);
ASSERT_OFF(shader_t, numUnfoggedPasses, 0x150);
ASSERT_OFF(shader_t, stages, 0x154);
ASSERT_OFF(shader_t, clampTime, 0x178);
ASSERT_OFF(shader_t, remappedShader, 0x180);
ASSERT_OFF(shader_t, mergeImage, 0x184);
ASSERT_OFF(shader_t, next, 0x194);

/* ------------------------------------------------------------------------- *
 *  Typed views over the trGlobals_t blob.  Available only when tr_records.h
 *  (which defines TRG) has already been included; they alias the exact bytes
 *  the tr_* offset macros reach, so the two spellings stay interchangeable.
 * ------------------------------------------------------------------------- */
#ifdef TRG
#define trOrient   (*(orientationr_t *)(TRG + 0x087c))   /* tr.or */
#define trView     (*(viewParms_t   *)(TRG + 0x0610))    /* tr.viewParms */
#endif



/* ------------------------------------------------------------------------- *
 *  Renderer-owned cvars.  Defined in tr_init.c (as in RTCW); every renderer
 *  translation unit sees them through this header.
 * ------------------------------------------------------------------------- */
extern cvar_t *X_r_inGameVideo;
extern cvar_t *outsideMapEnts;
extern cvar_t *r_ati_fsaa_samples;
extern cvar_t *r_ati_truform_normalmode;
extern cvar_t *r_ati_truform_pointmode;
extern cvar_t *r_ati_truform_tess;
extern cvar_t *r_clear;
extern cvar_t *r_colorMipLevels;
extern cvar_t *r_cullBModels;
extern cvar_t *r_cullXModels;
extern cvar_t *r_customaspect;
extern cvar_t *r_customheight;
extern cvar_t *r_customwidth;
extern cvar_t *r_debugEntLight;
extern cvar_t *r_debugGLErrors;
extern cvar_t *r_debugOptTex;
extern cvar_t *r_debugSort;
extern cvar_t *r_detailtextures;
extern cvar_t *r_diffuseSunQuality;
extern cvar_t *r_diffuseSunSteps;
extern cvar_t *r_dlightQuality;
extern cvar_t *r_drawBModels;
extern cvar_t *r_drawSModels;
extern cvar_t *r_drawSun;
extern cvar_t *r_drawXModels;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawworld;
extern cvar_t *r_dynamiclight;
extern cvar_t *r_entFullbright;
extern cvar_t *r_entLightCutoff;
extern cvar_t *r_entMinLight;
extern cvar_t *r_errorOnConflicts;
extern cvar_t *r_fastsky;
extern cvar_t *r_finish;
extern cvar_t *r_flareFadeIn;
extern cvar_t *r_flareFadeOut;
extern cvar_t *r_flareSize;
extern cvar_t *r_flares;
extern cvar_t *r_fog;
extern cvar_t *r_fullbright;
extern cvar_t *r_gamma;
extern cvar_t *r_graymap;
extern cvar_t *r_highLodDist;
extern cvar_t *r_ignore;
extern cvar_t *r_ignoreGLErrors;
extern cvar_t *r_intensity;
extern cvar_t *r_lockpvs;
extern cvar_t *r_lodViewDist;
extern cvar_t *r_lodscale;
extern cvar_t *r_lowLodDist;
extern cvar_t *r_maxEntLights;
extern cvar_t *r_maxTextureSize;
extern cvar_t *r_maxpolys;
extern cvar_t *r_maxpolyverts;
extern cvar_t *r_measureOverdraw;
extern cvar_t *r_mediumLodDist;
extern cvar_t *r_mem_agp;
extern cvar_t *r_mem_backend;
extern cvar_t *r_mem_manual;
extern cvar_t *r_mem_video;
extern cvar_t *r_minEntLightIntensity;
extern cvar_t *r_nobind;
extern cvar_t *r_nocull;
extern cvar_t *r_noportals;
extern cvar_t *r_norefresh;
extern cvar_t *r_offsetfactor;
extern cvar_t *r_offsetunits;
extern cvar_t *r_optimizeBackend;
extern cvar_t *r_optimizeSModels;
extern cvar_t *r_optimizeTextures;
extern cvar_t *r_optimizeWorld;
extern cvar_t *r_overBrightBits;
extern cvar_t *r_picmip;
extern cvar_t *r_picmip2;
extern cvar_t *r_portalOnly;
extern cvar_t *r_portalbevels;
extern cvar_t *r_primitives;
extern cvar_t *r_printShaders;
extern cvar_t *r_profileDrawElements;
extern cvar_t *r_railCoreWidth;
extern cvar_t *r_railSegmentLength;
extern cvar_t *r_railWidth;
extern cvar_t *r_saveFontData;
extern cvar_t *r_showCullBModels;
extern cvar_t *r_showCullSModels;
extern cvar_t *r_showCullXModels;
extern cvar_t *r_showImages;
extern cvar_t *r_showLeafLights;
extern cvar_t *r_showaabbtrees;
extern cvar_t *r_shownormals;
extern cvar_t *r_showportals;
extern cvar_t *r_showsky;
extern cvar_t *r_showsurfcounts;
extern cvar_t *r_showtricounts;
extern cvar_t *r_showtris;
extern cvar_t *r_simpleMipMaps;
extern cvar_t *r_singleShader;
extern cvar_t *r_singlecell;
extern cvar_t *r_skipBackEnd;
extern cvar_t *r_skip_auto_config;
extern cvar_t *r_smc_enable;
extern cvar_t *r_speeds;
extern cvar_t *r_sunblind_fadein;
extern cvar_t *r_sunblind_fadeout;
extern cvar_t *r_sunblind_max_angle;
extern cvar_t *r_sunblind_max_darken;
extern cvar_t *r_sunblind_min_angle;
extern cvar_t *r_sunflare_fadein;
extern cvar_t *r_sunflare_fadeout;
extern cvar_t *r_sunflare_max_alpha;
extern cvar_t *r_sunflare_max_angle;
extern cvar_t *r_sunflare_max_size;
extern cvar_t *r_sunflare_min_angle;
extern cvar_t *r_sunflare_min_size;
extern cvar_t *r_sunflare_shader;
extern cvar_t *r_sunglare_fadein;
extern cvar_t *r_sunglare_fadeout;
extern cvar_t *r_sunglare_max_angle;
extern cvar_t *r_sunglare_max_lighten;
extern cvar_t *r_sunglare_min_angle;
extern cvar_t *r_sunsprite_shader;
extern cvar_t *r_sunsprite_size;
extern cvar_t *r_suntest;
extern cvar_t *r_swapDelay;
extern cvar_t *r_texturebits;
extern cvar_t *r_uifullscreen;
extern cvar_t *r_vbo_interleave;
extern cvar_t *r_vbo_paranoia;
extern cvar_t *r_vbo_smc_static_draw;
extern cvar_t *r_vbo_stream_draw;
extern cvar_t *r_vc_makelog;
extern cvar_t *r_vc_showlog;
extern cvar_t *r_weightMipMaps;
extern cvar_t *r_xdebug;
extern cvar_t *r_zfar;
extern cvar_t *r_znear;
extern cvar_t *r_znear_depthhack;

/* ------------------------------------------------------------------------- *
 *  Shared renderer globals (each defined in the renderer unit that owns it).
 * ------------------------------------------------------------------------- */
extern char *tr_defaultShader;
extern double tr_pc_patchSphereCullInClip;
extern double tr_pc_patchSphereCullOutBoxCullIn;
extern double tr_pc_patchBoxCullClipOut;
extern double tr_pc_dlightSurfaceAndCullCount;
extern float tr_diffuseSunSampleScale;
extern float tr_identityLight;
extern float tr_inverseSawToothTable[1043];
extern float tr_sawToothTable[2067];
extern float tr_screenImageSMax;
extern float tr_screenImageTMax;
extern float tr_sinTable[5139];
extern float tr_squareTable[4115];
extern float tr_triangleTable[3091];
extern int backEndData;
/* The back end's dynamic vertex buffer, one object at 0x016D93EC (inside backEnd_t in retail).
 * RB_GetBuffersNV reloads every field after each ring store, so they share one object; the old
 * per-field names are macros onto the members. */
typedef struct {
	int     storage;                /* +0x00 0x016D93EC */
	int     capacity;               /* +0x04 0x016D93F0 */
	int     currentOffset;          /* +0x08 0x016D93F4 */
	int     frameSerial;            /* +0x0C 0x016D93F8 */
	int     unknown;                /* +0x10 0x016D93FC */
	int     allocationSequence;     /* +0x14 0x016D9400 */
	int     reclaimSequence;        /* +0x18 0x016D9404 */
	int     freeBytes;              /* +0x1C 0x016D9408 */
	struct {
		int offset;
		int size;
	}       allocations[512];       /* +0x20 0x016D940C, the NV fence ring */
} backEndDynamicBuffer_t;
#define backEnd_dynamicBuffer_storage               backEnd_dynamicBuffer.storage
#define backEnd_dynamicBuffer_capacity              backEnd_dynamicBuffer.capacity
#define backEnd_dynamicBuffer_currentOffset         backEnd_dynamicBuffer.currentOffset
#define backEnd_dynamicBuffer_frameSerial           backEnd_dynamicBuffer.frameSerial
#define backEnd_dynamicBuffer_allocationSequence    backEnd_dynamicBuffer.allocationSequence
#define backEnd_dynamicBuffer_reclaimSequence       backEnd_dynamicBuffer.reclaimSequence
#define backEnd_dynamicBuffer_freeBytes             backEnd_dynamicBuffer.freeBytes
/* the ring as interleaved words: [2*i] = allocations[i].offset, [2*i+1] = allocations[i].size */
#define backEnd_dynamicBuffer_allocations           ( (int *)backEnd_dynamicBuffer.allocations )
extern int rendererRegisteredFontCount;
extern int rendererAsianFontLoaded;
extern int tr_fragmentShaderCount;
extern int rendererSkyboxPortalActive;
extern int rendererSkyboxPortal;
extern int ftol_tempSpill;
extern int hashtable[4096];
extern int lightVisCache_entriesFilledAtRuntime;
extern int lightVisCache_entriesFlushed;
extern int lightVisCache_entriesUsed;
extern int lightVisCache_maxAssociativity;
extern int max_polys;
extern int max_polyverts;
extern int r_firstSceneCorona;
extern int r_firstSceneDlight;
extern int r_firstSceneDrawSurf;
extern int r_firstSceneEntity;
extern int r_firstScenePoly;
extern int r_numcoronas;
extern int r_numdlights;
extern int r_numentities;
extern int r_numpolys;
extern int r_numpolyverts;
/* Retail debug state 0x016D8894..0x016D8914; the three reserved words have no named source consumer. */
typedef struct rendererDebugState_s {
  int polygonVertexCapacity;
  int polygonVertexCount;
  void * polygonVertices;
  int polygonCapacity;
  int polygonCount;
  void * polygons;
  int stringCapacity;
  int stringCount;
  void * strings;
  void * font;
  int locatedStringCount;
  int locatedStrings;
  int lineCapacity;
  int lineCount;
  void * lines;
  int locatedLineCount;
  int locatedLines;
  int plumeCount;
  int plumeCapacity;
  void * plumes;
  int immediateModeActive;
  unsigned int immediateMode;
  float immediateLineWidth;
  float immediateTexCoordS;
  float immediateTexCoordT;
  unsigned char immediateColorR;
  unsigned char immediateColorG;
  unsigned char immediateColorB;
  unsigned char immediateColorA;
  int reserved[3];
  int immediateVertexCount;
  int immediateVertexCapacity;
  void * immediateVertices;
} rendererDebugState_t;
/* rendererDebugState: the last 0x80 bytes of tr (view under TRG below) */
COD1_ASSERT_SIZE(rendererDebugState_t, 0x80);
ASSERT_OFF(rendererDebugState_t, immediateColorR, 0x64);
ASSERT_OFF(rendererDebugState_t, immediateVertexCount, 0x74);
ASSERT_OFF(rendererDebugState_t, immediateVertexCapacity, 0x78);
ASSERT_OFF(rendererDebugState_t, immediateVertices, 0x7C);
#define rbDebug_polygonVertexCapacity (rendererDebugState.polygonVertexCapacity)
#define rbDebug_polygonVertexCount (rendererDebugState.polygonVertexCount)
#define rbDebug_polygonCapacity (rendererDebugState.polygonCapacity)
#define rbDebug_polygonCount (rendererDebugState.polygonCount)
#define rbDebug_polygons (rendererDebugState.polygons)
#define rbDebug_stringCapacity (rendererDebugState.stringCapacity)
#define rbDebug_stringCount (rendererDebugState.stringCount)
#define rbDebug_strings (rendererDebugState.strings)
#define rbDebug_font (rendererDebugState.font)
#define rbDebug_locatedStringCount (rendererDebugState.locatedStringCount)
#define rbDebug_locatedStrings (rendererDebugState.locatedStrings)
#define rbDebug_lineCapacity (rendererDebugState.lineCapacity)
#define rbDebug_lineCount (rendererDebugState.lineCount)
#define rbDebug_lines (rendererDebugState.lines)
#define rbDebug_locatedLineCount (rendererDebugState.locatedLineCount)
#define rbDebug_locatedLines (rendererDebugState.locatedLines)
#define rbDebug_plumeCount (rendererDebugState.plumeCount)
#define rbDebug_plumeCapacity (rendererDebugState.plumeCapacity)
#define rbDebug_plumes (rendererDebugState.plumes)
#define rbDebug_immediateModeActive (rendererDebugState.immediateModeActive)
#define rbDebug_immediateLineWidth ((*(int *)&rendererDebugState.immediateLineWidth))
#define rbDebug_immediateTexCoordS ((*(int *)&rendererDebugState.immediateTexCoordS))
#define rbDebug_immediateTexCoordT ((*(int *)&rendererDebugState.immediateTexCoordT))
#define rbDebug_immediateColorR (rendererDebugState.immediateColorR)
#define rbDebug_immediateColorG (rendererDebugState.immediateColorG)
#define rbDebug_immediateColorB (rendererDebugState.immediateColorB)
#define rbDebug_immediateColorA (rendererDebugState.immediateColorA)
#define rbDebug_immediateVertexCount (rendererDebugState.immediateVertexCount)
#define rbDebug_immediateVertexCapacity (rendererDebugState.immediateVertexCapacity)
#define rbDebug_immediateVertices (rendererDebugState.immediateVertices)
extern int rendererSkyBox;
extern int s_numWaterMaps;
extern int tr_cachedStaticModelStorage;
extern int tr_cachedStaticModelStorageOffset;
extern int tr_cachedStaticModelStorageSource;
extern int tr_cachedStaticModelSurfaceType;
extern int tr_currentEntity;
extern int tr_currentEntityNumber;
extern int tr_defaultImage;
extern int tr_delayedImageCount;
extern int tr_delayedImageGroupTileMode;
extern int tr_diffuseSunQuality;
extern int tr_diffuseSunSteps;
extern int tr_dlightImage;
extern int tr_dlightShader;
extern int tr_dynamicBufferFrameSerial;
extern int tr_dynamicBufferMaxFrameSerial;
extern int tr_flareShader;
extern int tr_frameSceneNum;
extern int tr_frontEndMsec;
extern int tr_grayImage;
extern int tr_identityLightByte;
extern int tr_identityLightImage;
extern int tr_imageMemory;
extern int tr_lightmapCount;
extern int tr_lightmaps[460];
extern int tr_maxEntityLights;
/* Retail model handles and count share one registry (0x016C5818..0x016C781C). */
typedef struct trModelRegistry_s {
  int models[2048];
  int numModels;
} trModelRegistry_t;
#define tr_models (trModelRegistry.models)
#define tr_numModels (trModelRegistry.numModels)
extern int tr_numImages;

extern int tr_overbrightBits;
extern int tr_pc_leafCount;
extern int tr_performanceCounters;
extern int tr_scratchImages[32];
extern int tr_screenImage;
extern int tr_screenImageHeight;
extern int tr_screenImageSaveTime;
extern int tr_screenImageShader;
extern int tr_screenImageWidth;
extern int tr_shiftedEntityNumber;
extern int tr_showImagesShader;
extern int tr_showTrisShader;
extern int tr_spotLightShader;
extern int tr_staticModelRefreshLodIndex;
extern int tr_staticModelRefreshModelIndex;
extern int tr_staticModelRefreshSurfaceIndex;
extern int tr_stencilShadowShader;
extern int tr_vboInterleaved;
extern int tr_vboStreamDraw;
extern int tr_vboUsage;
extern int tr_viewCluster;
extern int tr_viewCount;
extern int tr_whiteImage;
extern int tr_world;
extern int tr_worldMapLoaded;
extern int tr_worldRefreshSurfaceIndex;
extern int tr_xmodelRefreshLodIndex;
extern int tr_xmodelRefreshModelIndex;
extern int tr_xmodelRefreshSurfaceIndex;
extern unsigned char tr_worldEntity[1324];

/* renderer-owned globals that already had authoritative defs in renderer/*.c */
extern unsigned int acceptedBevelDebugColor[68];
extern unsigned int clippedLeafDebugColor[40];
extern unsigned int cullGroupDebugColor[84];
extern unsigned int colorDebugPortal[51];
extern unsigned int colorDebugWhite[56];
extern unsigned int modelBoundsDebugColor[72];
extern unsigned int rejectedBevelDebugColor[64];
extern unsigned int staticModelDebugColor[52];
extern int reverseBitsTableNeedsInitialization;
extern int dpvs_initialSetup;
extern unsigned char entitySurface[232];
extern float s_flipMatrix[74];

/* trGlobals_t fields kept as views of the TRG blob (retail's one `tr` object) */
#ifdef TRG
#define tr_frameCount                            (*(int *)(TRG + 0x0004))   /* 0x016C4D44 */
#define tr_viewCount                             (*(int *)(TRG + 0x0008))
#define tr_frameSceneNum                         (*(int *)(TRG + 0x000C))
#define tr_worldMapLoaded                        (*(int *)(TRG + 0x0010))
#define tr_world                                 (*(int *)(TRG + 0x0014))
#define tr_defaultImage                          (*(int *)(TRG + 0x0018))
#define tr_scratchImages                         (*(int (*)[32])(TRG + 0x001C))
#define tr_dlightImage                           (*(int *)(TRG + 0x009C))
#define tr_whiteImage                            (*(int *)(TRG + 0x00A4))
#define tr_identityLightImage                    (*(int *)(TRG + 0x00A8))
#define tr_grayImage                             (*(int *)(TRG + 0x00AC))
#define tr_screenImage                           (*(int *)(TRG + 0x00B0))
#define tr_screenImageWidth                      (*(int *)(TRG + 0x00B4))
#define tr_screenImageHeight                     (*(int *)(TRG + 0x00B8))
#define tr_screenImageSaveTime                   (*(int *)(TRG + 0x00BC))
#define tr_screenImageSMax                       (*(float *)(TRG + 0x00C0))
#define tr_screenImageTMax                       (*(float *)(TRG + 0x00C4))
#define tr_defaultShader                         (*(char * *)(TRG + 0x00C8))
#define tr_stencilShadowShader                   (*(int *)(TRG + 0x00CC))
#define tr_screenImageShader                     (*(int *)(TRG + 0x00D0))
#define tr_dlightShader                          (*(int *)(TRG + 0x00D4))
#define tr_flareShader                           (*(int *)(TRG + 0x00D8))
#define tr_spotLightShader                       (*(int *)(TRG + 0x00DC))
#define tr_delayedImageGroupTileMode             (*(int *)(TRG + 0x0130))
#define tr_delayedImageCount                     (*(int *)(TRG + 0x0138))
#define tr_showTrisShader                        (*(int *)(TRG + 0x013C))
#define tr_showImagesShader                      (*(int *)(TRG + 0x0140))
#define tr_lightmapCount                         (*(int *)(TRG + 0x0144))
#define tr_lightmaps                             (*(int (*)[460])(TRG + 0x0148))
#define tr_currentEntity                         (*(int *)(TRG + 0x0348))
#define tr_worldEntity                           (*(unsigned char (*)[1324])(TRG + 0x034C))
#define tr_currentEntityNumber                   (*(int *)(TRG + 0x0604))
#define tr_shiftedEntityNumber                   (*(int *)(TRG + 0x0608))
#define tr_identityLight                         (*(float *)(TRG + 0x0870))
#define tr_identityLightByte                     (*(int *)(TRG + 0x0874))
#define tr_overbrightBits                        (*(int *)(TRG + 0x0878))
#define tr_viewCluster                           (*(int *)(TRG + 0x0A80))
#define tr_sunLight                              (*(float (*)[3])(TRG + 0x0A84))
#define tr_sunDirection                          (*(float (*)[3])(TRG + 0x0A90))
#define tr_maxEntityLights                       (*(int *)(TRG + 0x0A9C))
#define tr_diffuseSunQuality                     (*(int *)(TRG + 0x0AA0))
#define tr_diffuseSunSteps                       (*(int *)(TRG + 0x0AA4))
#define tr_diffuseSunSampleScale                 (*(float *)(TRG + 0x0AA8))
#define tr_pc_patchSphereCullInClip              (*(double *)(TRG + 0x0AAC))
#define tr_pc_patchSphereCullOutBoxCullIn        (*(double *)(TRG + 0x0AB4))
#define tr_pc_patchBoxCullClipOut                (*(double *)(TRG + 0x0ABC))
#define tr_pc_leafCount                          (*(int *)(TRG + 0x0AC4))
#define tr_pc_dlightSurfaceAndCullCount          (*(double *)(TRG + 0x0AC8))
#define tr_frontEndMsec                          (*(int *)(TRG + 0x0AD0))
#define tr_performanceCounters                   (*(int *)(TRG + 0x0AD4))
#define trModelRegistry                          (*(trModelRegistry_t *)(TRG + 0x0AD8))   /* 0x016C5818: tr.models[2048], tr.numModels */
#define tr_xmodelRefreshModelIndex               (*(int *)(TRG + 0x2ADC))
#define tr_xmodelRefreshLodIndex                 (*(int *)(TRG + 0x2AE0))
#define tr_xmodelRefreshSurfaceIndex             (*(int *)(TRG + 0x2AE4))
#define tr_staticModelRefreshModelIndex          (*(int *)(TRG + 0x2AE8))
#define tr_staticModelRefreshLodIndex            (*(int *)(TRG + 0x2AEC))
#define tr_staticModelRefreshSurfaceIndex        (*(int *)(TRG + 0x2AF0))
#define tr_worldRefreshSurfaceIndex              (*(int *)(TRG + 0x2AF4))
#define tr_numImages                             (*(int *)(TRG + 0x4AFC))
#define tr_imageSlots                            (*(int (*)[2048])(TRG + 0x4B00))   /* 0x016C9840: tr.images */
#define tr_imageMemory                           (*(int *)(TRG + 0x6B00))
#define tr_fragmentShaderCount                   (*(int *)(TRG + 0xEB0C))
#define tr_sinTable                              (*(float (*)[5139])(TRG + 0xEB10))
#define tr_squareTable                           (*(float (*)[4115])(TRG + 0xFB10))
#define tr_triangleTableMinus512                 (*(float (*)[3603])(TRG + 0x10310))
#define tr_triangleTableMinus256                 (*(float (*)[3347])(TRG + 0x10710))
#define tr_triangleTable                         (*(float (*)[3091])(TRG + 0x10B10))
#define tr_sawToothTable                         (*(float (*)[2067])(TRG + 0x11B10))
#define tr_inverseSawToothTable                  (*(float (*)[1043])(TRG + 0x12B10))
#define tr_dynamicBufferFrameSerial              (*(int *)(TRG + 0x13B2C))
#define tr_dynamicBufferMaxFrameSerial           (*(int *)(TRG + 0x13B30))
#define tr_cachedStaticModelStorageSource        (*(int *)(TRG + 0x13B34))
#define tr_cachedStaticModelSurfaceType          (*(int *)(TRG + 0x13B38))
#define tr_cachedStaticModelStorage              (*(int *)(TRG + 0x13B3C))
#define tr_cachedStaticModelStorageOffset        (*(int *)(TRG + 0x13B40))
#define tr_vboStreamDraw                         (*(int *)(TRG + 0x13B44))
#define tr_vboInterleaved                        (*(int *)(TRG + 0x13B48))
#define tr_vboUsage                              (*(int *)(TRG + 0x13B4C))
#define storageClass                             (*(int *)(TRG + 0x13B28))   /* 0x016D8868 */
#define tr_stageIteratorFunc                     (*(int (**)())(TRG + 0x13B50))   /* 0x016D8890 */
#define rendererDebugState                       (*(rendererDebugState_t *)(TRG + 0x13B54))   /* 0x016D8894..0x016D8913 */
#endif


/* backEnd_t fields kept as views of the BACKEND blob (retail's one backEnd object, 0x016D89C0) */
#ifdef BACKEND
#define backEnd_pc_surfaceCount              (*(int *)(BACKEND + 0x0464))   /* 0x016D8E24 */
#define backEnd_pc_shaderCount               (*(int *)(BACKEND + 0x0468))   /* 0x016D8E28 */
#define backEnd_pc_vertexCount               (*(int *)(BACKEND + 0x046C))   /* 0x016D8E2C */
#define backEnd_pc_indexCount                (*(int *)(BACKEND + 0x0470))   /* 0x016D8E30 */
#define backEnd_pc_drawnIndexCount           (*(int *)(BACKEND + 0x0474))   /* 0x016D8E34 */
#define backEnd_pc_drawCallCount             (*(int *)(BACKEND + 0x0478))   /* 0x016D8E38 */
#define backEnd_pc_overdrawSum               (*(float *)(BACKEND + 0x047C))   /* 0x016D8E3C */
#define backEnd_pc_dlightVertexCount         (*(int *)(BACKEND + 0x0480))   /* 0x016D8E40 */
#define backEnd_pc_dlightIndexCount          (*(int *)(BACKEND + 0x0484))   /* 0x016D8E44 */
#define backEnd_pc_flareAddCount             (*(int *)(BACKEND + 0x0488))   /* 0x016D8E48 */
#define backEnd_pc_flareTests                (*(double *)(BACKEND + 0x048C))   /* 0x016D8E4C */
#define backEnd_pc_msec                      (*(int *)(BACKEND + 0x0494))   /* 0x016D8E54 */
#define backEnd_currentEntity                (*(int *)(BACKEND + 0x049C))   /* 0x016D8E5C */
#define backEnd_currentDlight                (*(int *)(BACKEND + 0x04A0))   /* 0x016D8E60 */
#define backEnd_currentLightScale            (*(float *)(BACKEND + 0x04A4))   /* 0x016D8E64 */
#define backEnd_skyRenderedThisView          (*(int *)(BACKEND + 0x04A8))   /* 0x016D8E68 */
#define backEnd_projection2D                 (*(int *)(BACKEND + 0x04B0))   /* 0x016D8E70 */
#define backEnd_entity2D                     (*(unsigned char (*)[0x2B8])(BACKEND + 0x04BC))   /* 0x016D8E7C, trRefEntity_t */
#define backEnd_debugEntity                  (*(unsigned char (*)[0x2B8])(BACKEND + 0x0774))   /* 0x016D9134, trRefEntity_t */
#define backEnd_dynamicBuffer                (*(backEndDynamicBuffer_t *)(BACKEND + 0x0A2C))   /* 0x016D93EC */
#endif

/* ------------------------------------------------------------------------- *
 *  Renderer data tables shared between units.
 * ------------------------------------------------------------------------- */
/* the back end's surface dispatch table, indexed by surfaceType_t (RTCW
 * rb_surfaceTable); defined in tr_backend.c */
typedef void (__cdecl *rb_surfaceFunc_t)( void *surf );
extern rb_surfaceFunc_t funcs_4D4A37[29];   /* 0x00571140 */
/* the `surfaceparm` keyword table (RTCW infoParm_t infoParms[]); defined in
 * tr_shader.c, also walked by the brushmask formatter in tr_cmds.c */
typedef struct {
    const char *name;
    int         clearSolid;
    int         surfaceFlags;
    int         contents;
} tr_infoParm_t;
extern tr_infoParm_t surfacetype_bark[53];   /* 0x005717A0 */

/* ------------------------------------------------------------------------- *
 *  Renderer state the win32 GL layer (win_glimp.c, win_qgl.c, win_gamma.c)
 *  shares; those units include this header as in RTCW.  glState is defined in
 *  tr_init.c with the cvars below, backEndStorage in client_mp/cl_refstorage.c.
 * ------------------------------------------------------------------------- */
/* The renderer's GL state cache, one object at 0x016C38C0 (renderer/tr_init.c).  Retail
 * addresses it as a whole, so a store through one field may alias another under a variable
 * index; the old per-field names are kept as macros onto the members. */
typedef struct {
	int             currentTmu;                 /* +0x00 0x016C38C0 */
	int             currentClientTmu;           /* +0x04 */
	int             currentTextures[8];         /* +0x08 */
	int             texEnv[8];                  /* +0x28 */
	unsigned int    cap[8];                     /* +0x48 GLenum: the enabled texture target per TMU */
	int             textureShaderEnabled[8];    /* +0x68 */
	int             finishCalled;               /* +0x88 */
	int             faceCulling;                /* +0x8C */
	int             glStateBits;                /* +0x90 */
	int             clientStateBits;            /* +0x94 */
	int             fogMode;                    /* +0x98 */
	int             fogHint;                    /* +0x9C */
	float           fogColor[4];                /* +0xA0 */
	float           fogStart;                   /* +0xB0 */
	float           fogEnd;                     /* +0xB4 */
	float           fogDensity;                 /* +0xB8 */
	int             currentStorageMode;         /* +0xBC */
	int             numEnabledLights;           /* +0xC0 */
	int             currentLightingEntity;      /* +0xC4 */
	int             currentLightingFlags;       /* +0xC8 */
	int             normalizeTarget;            /* +0xCC */
	int             boundFragmentShader;        /* +0xD0 */
	int             boundVertexProgram;         /* +0xD4 */
} glstate_t;
COD1_ASSERT_SIZE( glstate_t, 0xD8 );
extern glstate_t glState;   /* 0x016C38C0 */
#define glState_currentTmu              glState.currentTmu
#define glState_currentClientTmu        glState.currentClientTmu
#define glState_currentTextures         glState.currentTextures
#define glState_texEnv                  glState.texEnv
#define glState_cap                     glState.cap
#define glState_textureShaderEnabled    glState.textureShaderEnabled
#define glState_finishCalled            glState.finishCalled
#define glState_faceCulling             glState.faceCulling
#define glState_glStateBits             glState.glStateBits
#define glState_clientStateBits         glState.clientStateBits
#define glState_fogMode                 glState.fogMode
#define glState_fogHint                 glState.fogHint
#define glState_fogColor                glState.fogColor[0]
#define glState_fogColorG               glState.fogColor[1]
#define glState_fogColorB               glState.fogColor[2]
#define glState_fogColorA               glState.fogColor[3]
#define glState_fogStart                glState.fogStart
#define glState_fogEnd                  glState.fogEnd
#define glState_fogDensity              glState.fogDensity
#define glState_currentStorageMode      glState.currentStorageMode
#define glState_numEnabledLights        glState.numEnabledLights
#define glState_currentLightingEntity   glState.currentLightingEntity
#define glState_currentLightingFlags    glState.currentLightingFlags
#define glState_normalizeTarget         glState.normalizeTarget
#define glState_boundFragmentShader     glState.boundFragmentShader
#define glState_boundVertexProgram      glState.boundVertexProgram
extern cvar_t *r_arb_texture_cube_map;   /* 0x016C3880, 4 bytes */
extern cvar_t *r_arb_texture_env_add;   /* 0x016C37A8, 4 bytes */
extern cvar_t *r_arb_texture_env_combine;   /* 0x016C3798, 4 bytes */
extern cvar_t *r_arb_texture_env_dot3;   /* 0x016C374C, 4 bytes */
extern cvar_t *r_arb_vertex_buffer_object;   /* 0x016C37C4, 4 bytes */
extern cvar_t *r_arb_vertex_program;   /* 0x016C39D0, 4 bytes */
extern cvar_t *r_ati_element_array;   /* 0x016C3820, 4 bytes */
extern cvar_t *r_ati_fragment_shader;   /* 0x016C3810, 4 bytes */
extern cvar_t *r_ati_pntriangles;   /* 0x016C3794, 4 bytes */
extern cvar_t *r_ati_vertex_array_object;   /* 0x016C36F0, 4 bytes */
extern cvar_t *r_colorbits;   /* 0x016C36C8, 4 bytes */
extern cvar_t *r_depthbits;   /* 0x016C3B10, 4 bytes */
extern cvar_t *r_displayRefresh;   /* 0x016C3A34, 4 bytes */
extern cvar_t *r_drawBuffer;   /* 0x016C3808, 4 bytes */
extern cvar_t *r_ext_compiled_vertex_array;   /* 0x016C3B18, 4 bytes */
extern cvar_t *r_ext_draw_range_elements;   /* 0x016C3730, 4 bytes */
extern cvar_t *r_ext_rescale_normal;   /* 0x016C3A28, 4 bytes */
extern cvar_t *r_ext_texture_filter_anisotropic;   /* 0x016C3B08, 4 bytes */
extern cvar_t *r_ignorehwgamma;   /* 0x016C3A14, 4 bytes */
extern cvar_t *r_logFile;   /* 0x016C3A24, 4 bytes */
extern cvar_t *r_maxActiveTextures;   /* 0x016C39CC, 4 bytes */
extern cvar_t *r_mode;   /* 0x016C3784, 4 bytes */
extern cvar_t *r_nv_fence;   /* 0x016C37E4, 4 bytes */
extern cvar_t *r_nv_fog_dist;   /* 0x016C3760, 4 bytes */
extern cvar_t *r_nv_register_combiners;   /* 0x016C399C, 4 bytes */
extern cvar_t *r_nv_texture_shader;   /* 0x016C37CC, 4 bytes */
extern cvar_t *r_nv_vertex_array_range;   /* 0x016C37F8, 4 bytes */
extern cvar_t *r_stencilbits;   /* 0x016C3754, 4 bytes */
extern cvar_t *r_swapInterval;   /* 0x016C3A18, 4 bytes */
extern cvar_t *r_verbose;   /* 0x016C36B4, 4 bytes */
extern unsigned char backEndStorage[6732];   /* 0x016D89C0, 0x1A4C bytes, client_mp/cl_refstorage.c */
extern FILE *Stream;   /* 0x019C0004: the QGL log file, defined in win32/win_glimp.c */

/* ------------------------------------------------------------------------- *
 *  Renderer globals shared between units.  Owners follow RTCW: tr_init.c the cvars, tr_main.c the glfog state
 *  and the fog table, tr_bsp.c s_worldData, tr_sky.c the sky bounds and the sun state, tr_shadows.c the
 *  shadow-volume tables.  tr_lightVisCache is sized in client_mp/cl_refstorage.c (link-order tie, see
 *  there); the type here is the one the code addresses it through.
 * ------------------------------------------------------------------------- */
extern cvar_t *cg_shadows;   /* 0x016C36F8, 4 bytes */
extern cvar_t *cg_skybox;   /* 0x016C39C4, 4 bytes */
extern cvar_t *r_lightmap;   /* 0x016C36D4, 4 bytes */
extern cvar_t *r_lodbias;   /* 0x016C3750, 4 bytes */
extern cvar_t *r_nv_fogdist_mode;   /* 0x016C3878, 4 bytes */
extern cvar_t *r_optimize;   /* 0x016C383C, 4 bytes */
extern cvar_t *r_optimizeXModels;   /* 0x016C3738, 4 bytes */
extern cvar_t *r_textureMode;   /* 0x016C3734, 4 bytes */
extern cvar_t *r_vc_compile;   /* 0x016C3894, 4 bytes */
extern cvar_t *sv_cheats;   /* 0x016C39B4, 4 bytes */
extern int s_cloudTexCoords[3392];   /* 0x011DAE68, 13568 bytes */
extern float sky_mins[2][6];   /* 0x011DAE08: s mins, then t mins (sky_mins_1, renderer/tr_tess.h) */
extern float sky_maxs[2][6];   /* 0x011DAE38: s maxs, then t maxs (sky_maxs_1) */
extern int facing[16384];   /* 0x011EECE0, 65536 bytes */
extern int edgeDefs[524288];   /* 0x011FECE0, 2097152 bytes */
extern int glfogNum;   /* 0x01407300, 44 bytes */
extern unsigned char s_worldData[340];   /* 0x011A30E8, tr_bsp.c */
extern int rendererSunState[39];   /* 0x011DBDA0, tr_sky.c */
extern int tr_fogTable[144];   /* 0x016C4B00, 576 bytes, tr_main.c */
extern unsigned char tr_lightVisCache[2097152];   /* 0x00CA28D0, client_mp/cl_refstorage.c */
extern cvar_t *r_fullscreen;   /* 0x016C35C4, tr_init.c */

/* tr (RTCW tr_main.c `trGlobals_t tr`): the renderer's globals, 0x13BD4 bytes at 0x016C4D40, defined
 * in tr_main.c.  tr_registered[0] is tr.registered; tr_records.h (TRG) and the views below address
 * the rest. */
extern int tr_registered[20213];
/* tr's static vertex-memory pools: AGP (secondary) at 0x016D8850, video (primary) right after at 0x016D885C */
typedef struct {
	int memory;
	int limit;
	int used;
} trVertexMemoryPool_t;
/* fields of the renderer's tr object (tr_registered blob, 0x016C4D40), not separate globals */
#define tr_staticVertexMemorySecondaryPool  ( *(trVertexMemoryPool_t *)( (char *)(void *)&tr_registered + 0x13B10 ) )   /* 0x016D8850 */
#define tr_staticVertexMemoryPrimaryPool    ( *(trVertexMemoryPool_t *)( (char *)(void *)&tr_registered + 0x13B1C ) )   /* 0x016D885C */
#define tr_staticVertexMemorySecondary        tr_staticVertexMemorySecondaryPool.memory
#define tr_staticVertexMemorySecondaryLimit   tr_staticVertexMemorySecondaryPool.limit
#define tr_staticVertexMemorySecondaryUsed    tr_staticVertexMemorySecondaryPool.used
#define tr_staticVertexMemoryPrimary          tr_staticVertexMemoryPrimaryPool.memory
#define tr_staticVertexMemoryPrimaryLimit     tr_staticVertexMemoryPrimaryPool.limit
#define tr_staticVertexMemoryPrimaryUsed      tr_staticVertexMemoryPrimaryPool.used

/* the debug-draw colours, tr_debug.c */
extern unsigned int colorDebugRed[9];   /* 0x00541830 */
extern unsigned int colorDebugGreen[5];   /* 0x00541840 */
extern unsigned int colorDebugBlue[92];   /* 0x00541860 */
extern unsigned int colorDebugYellow[88];   /* 0x00541870 */

/* The renderer's GL configuration, one glconfig_t at 0x016C3A60 (tr_init.c; the type is in
 * tr_glconfig.h).  Retail addresses it as a whole, so the old per-field names are macros onto
 * the members.  vidWidth / vidHeight were IDA's dwStyle / dwExStyle and maxActiveTextures its
 * Value. */
extern glconfig_t glConfig;   /* 0x016C3A60 */
#define glConfig_renderer_string                 glConfig.renderer_string
#define glConfig_vendor_string                   glConfig.vendor_string
#define glConfig_version_string                  glConfig.version_string
#define glConfig_extensions_string               glConfig.extensions_string
#define glConfig_wextensions_string              glConfig.wextensions_string
#define glConfig_maxTextureSize                  glConfig.maxTextureSize
#define glConfig_maxActiveTextures               glConfig.maxActiveTextures
#define glConfig_maxLights                       glConfig.maxLights
#define glConfig_colorBits                       glConfig.colorBits
#define glConfig_depthBits                       glConfig.depthBits
#define glConfig_stencilBits                     glConfig.stencilBits
#define glConfig_deviceSupportsGamma             glConfig.deviceSupportsGamma
#define glConfig_maxAnisotropy                   glConfig.maxAnisotropy
#define glConfig_maxTextureFilterAnisotropy      glConfig.maxTextureFilterAnisotropy
#define glConfig_textureEnvAddAvailable          glConfig.textureEnvAddAvailable
#define glConfig_textureCubeMap                  glConfig.textureCubeMap
#define glConfig_textureEnvCombine               glConfig.textureEnvCombine
#define glConfig_textureEnvDot3                  glConfig.textureEnvDot3
#define glConfig_ARBVertexBufferObject           glConfig.ARBVertexBufferObject
#define glConfig_ARBVertexProgram                glConfig.ARBVertexProgram
#define glConfig_rescaleNormal                   glConfig.rescaleNormal
#define glConfig_NVFogAvailable                  glConfig.NVFogAvailable
#define glConfig_NVFogMode                       glConfig.NVFogMode
#define glConfig_NVVertexArrayRange              glConfig.NVVertexArrayRange
#define glConfig_NVFenceAvailable                glConfig.NVFenceAvailable
#define glConfig_NVRegisterCombiners             glConfig.NVRegisterCombiners
#define glConfig_NVTextureShader                 glConfig.NVTextureShader
#define glConfig_maxPNTrianglesTessellationLevel glConfig.maxPNTrianglesTessellationLevel
#define glConfig_pnTrianglesNormalMode           glConfig.pnTrianglesNormalMode
#define glConfig_pnTrianglesPointMode            glConfig.pnTrianglesPointMode
#define glConfig_ATIVertexArrayObject            glConfig.ATIVertexArrayObject
#define glConfig_ATIElementArray                 glConfig.ATIElementArray
#define glConfig_ATIFragmentShader               glConfig.ATIFragmentShader
#define glConfig_vidWidth                        glConfig.vidWidth
#define glConfig_vidHeight                       glConfig.vidHeight
#define glConfig_windowAspect                    glConfig.windowAspect
#define glConfig_displayFrequency                glConfig.displayFrequency
#define glConfig_isFullscreen                    glConfig.isFullscreen
#define glConfig_stereoEnabled                   glConfig.stereoEnabled

#endif /* TR_LOCAL_H */
