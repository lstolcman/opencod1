/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_shaderregistry.h"
#include "tr_gl_types.h"
#include "tr_orientation.h"   /* backEnd.or 0x016D8DA8 -- one object, not 16 shards */
#include "tr_tess.h"
#include "tr_font.h"
#include "tr_local.h"         /* typed structs + asserts; after tr_records.h (TRG) */
#include "../client_mp/cl_refapi.h"   /* re: RB_SwapBuffers locates the client's debug draws through it */

/* backEnd (RTCW backEndState_t backEnd, tr_backend.c): one 0x1A4C-byte object at 0x016D89C0 that
 * the renderer addresses through tr_records.h's BACKEND views.  backEndStorage is defined in
 * client_mp/cl_refstorage.c: a definition in a renderer unit (linked after l_precomp_mp.obj)
 * renumbers the symbols before it and flips the PC_EvaluateTokens / Sys_GetVideoRam scheduling
 * ties (tools/vc7/tie_probe.py). */

/* backEndData (RTCW tr_backend.c backEndData_t *backEndData[SMP_FRAMES]) */
int backEndData;
COD1_ASSERT_SIZE( backEndDynamicBuffer_t, 0x1020 );

/* fog[FOG_PORTALVIEW=2].color inside tr_fogTable (0x016C4B90..0x016C4B9C; IDA
 * names them red/green/blue/alpha).  R_SetFog writes FOG_TABLE[2].color here;
 * RB_BeginDrawingView reads it as the skybox-portal clear colour.  The
 * red/green/blue/alpha names IDA gave them are not separate storage. */
#define tr_fogPortalView_colorR  (*(float *)(FOGBASE + 0x090))   /* 0x016C4B90 */
#define tr_fogPortalView_colorG  (*(float *)(FOGBASE + 0x094))   /* 0x016C4B94 */
#define tr_fogPortalView_colorB  (*(float *)(FOGBASE + 0x098))   /* 0x016C4B98 */
#define tr_fogPortalView_colorA  (*(float *)(FOGBASE + 0x09C))   /* 0x016C4B9C */

/* backEnd_t fields at backEndStorage (0x016D89C0) + offset, so R_Init's
 * memset(&backEndStorage, 0, 0x1A4C) clears them as in retail (they used to be
 * standalone ints). */
#define backEnd_isHyperspace     (*(int *)(BACKEND + 0x0498))   /* 0x016D8E58 */
#define backEnd_endFramePending  (*(int *)(BACKEND + 0x04AC))   /* 0x016D8E6C */
#define backEnd_color2D          (*(int *)(BACKEND + 0x04B4))   /* 0x016D8E74 */


extern void GLimp_EndFrame( void );
void RB_BeginImmediateMode( void );
void RB_EndImmediateMode( void );
void __cdecl RB_BeginSurface( void *shader, int vertexComponentCount );
void __cdecl RB_CheckOverflow( int verts, int indexes );
extern int FastRound( float f );   /* client_mp/cl_console_mp.c 0x00408370 */
void __cdecl RB_DrawDebug( void );
extern int RB_EndMultitexture();
void __cdecl RB_EndSurface( void );
extern int RB_RenderFlares();
extern int RB_SelectStorageATI();
extern int RB_SelectStorageNV();
extern int RB_ShadowFinish();
void __cdecl RB_ExecuteRenderCommands( const void *data );
void __cdecl R_SyncRenderThread( void );
__declspec(dllimport) unsigned long __stdcall timeGetTime( void );
GLenum __cdecl RB_glBegin( GLenum result );
void RB_glEnd( void );
void __cdecl RB_glVertex2f( float x, float y );
void __cdecl RB_glTexCoord2f( float s, float t );
unsigned __int64 __cdecl RB_glColor3fv( const float *v );
extern int RE_SetColor();
extern int R_FogOn();
extern double __cdecl R_GetAsianScale( int font, float scale );
extern int *R_GetCharacterGlyph( int character, int fontBase );
extern void R_RotateForModelEntity();
extern int R_SetFogColor();
extern int R_TransformDlights();

#define rb_surfaceTable   ((rb_surfaceFunc_t *)(void *)funcs_4D4A37)

/* === Merged from renderer/tr_backend_rdata.c (retail linked it as tr_*.obj .rdata/.data). === */

/* renderer/tr_surface.c */
void __cdecl RB_SurfaceBad( void *surf );                          /* 0x00512960 */
void __cdecl RB_SurfaceSkip( void *surf );                         /* 0x00512980 */
void __cdecl RB_SurfacePolychain( void *surf );                    /* 0x005105B0 */
void __cdecl RB_SurfaceEntity( void *surf );                       /* 0x005128A0 */
void __cdecl RB_SurfaceTriangles( void *surf );                    /* 0x005122B0 */
void __cdecl RB_SurfaceOptimized( void *surf );                    /* 0x00512660 */

/* renderer/tr_xanim.c */
void __cdecl RB_SurfaceXModelRigid( void *surf );                  /* 0x0050EA40 */
void __cdecl RB_SurfaceXModelRigidSSE( void *surf );               /* 0x0050EB40 */
void __cdecl RB_SurfaceXModelRigidARB( void *surf );               /* 0x0050EC90 */
void __cdecl RB_SurfaceXModelRigidATI( void *surf );               /* 0x0050EF20 */
void __cdecl RB_SurfaceXModelRigidNV( void *surf );                /* 0x0050F380 */
void __cdecl RB_SurfaceXModelWeight( void *surf );                 /* 0x0050F5E0 */
void __cdecl RB_SurfaceXModelWeightSSE( void *surf );              /* 0x0050F780 */

/* renderer/tr_staticmodel.c */
void __cdecl RB_SurfaceStaticModel( void *surf );                  /* 0x00505580 */
void __cdecl RB_SurfaceStaticModelCached( void *surf );            /* 0x005056A0 */
void __cdecl RB_SurfaceStaticModelT2V3_Generic( void *surf );      /* 0x005065C0 */
void __cdecl RB_SurfaceStaticModelT2V3_ARB( void *surf );          /* 0x005057A0 */
void __cdecl RB_SurfaceStaticModelATI( void *surf );               /* 0x00505C80 */
void __cdecl RB_SurfaceStaticModelT2V3_NV( void *surf );           /* 0x00506130 */
void __cdecl RB_SurfaceStaticModelT2N3V3_Generic( void *surf );    /* 0x00506390 */
void __cdecl RB_SurfaceStaticModelT2N3V3_ARB( void *surf );        /* 0x00505A30 */
void __cdecl RB_SurfaceStaticModelT2N3V3_NV( void *surf );         /* 0x00506820 */

/* 0x00571140.  The retail dword is in the comment on each line. */
rb_surfaceFunc_t funcs_4D4A37[29] =
{
    /*  0  0x00512960 */  RB_SurfaceBad,
    /*  1  0x00512980 */  RB_SurfaceSkip,
    /*  2  0x005105B0 */  RB_SurfacePolychain,
    /*  3  0x005128A0 */  RB_SurfaceEntity,
    /*  4  0x0050EA40 */  RB_SurfaceXModelRigid,
    /*  5  0x0050EB40 */  RB_SurfaceXModelRigidSSE,
    /*  6  0x0050EC90 */  RB_SurfaceXModelRigidARB,
    /*  7  0x0050EF20 */  RB_SurfaceXModelRigidATI,
    /*  8  0x0050F380 */  RB_SurfaceXModelRigidNV,
    /*  9  0x0050F5E0 */  RB_SurfaceXModelWeight,
    /* 10  0x0050F780 */  RB_SurfaceXModelWeightSSE,
    /* 11  0x00505580 */  RB_SurfaceStaticModel,
    /* 12  0x005056A0 */  RB_SurfaceStaticModelCached,
    /* 13  0x005056A0 */  RB_SurfaceStaticModelCached,
    /* 14  0x005056A0 */  RB_SurfaceStaticModelCached,
    /* 15  0x005056A0 */  RB_SurfaceStaticModelCached,
    /* 16  0x005065C0 */  RB_SurfaceStaticModelT2V3_Generic,
    /* 17  0x005057A0 */  RB_SurfaceStaticModelT2V3_ARB,
    /* 18  0x00505C80 */  RB_SurfaceStaticModelATI,
    /* 19  0x00506130 */  RB_SurfaceStaticModelT2V3_NV,
    /* 20  0x00506390 */  RB_SurfaceStaticModelT2N3V3_Generic,
    /* 21  0x00505A30 */  RB_SurfaceStaticModelT2N3V3_ARB,
    /* 22  0x00505C80 */  RB_SurfaceStaticModelATI,
    /* 23  0x00506820 */  RB_SurfaceStaticModelT2N3V3_NV,
    /* 24  0x005122B0 */  RB_SurfaceTriangles,
    /* 25  0x00512660 */  RB_SurfaceOptimized,
    /* 26  0x00512660 */  RB_SurfaceOptimized,
    /* 27  0x00512660 */  RB_SurfaceOptimized,
    /* 28  0x00512660 */  RB_SurfaceOptimized
};

/* ------------------------------------------------------------------------- *
 *  File-local typed views over the data this file receives by POINTER.
 *
 *  The renderer's globals stay on the tr_records.h / tr_tess.h offset macros
 *  (hybrid blob+symbol storage); these structs only add C types for the
 *  records that arrive through a pointer -- images and the render-command
 *  buffer -- so the field soup ( *(unsigned int *)(x + off) ) becomes named access.
 *  Retyping a pointer never moves storage; each offset is pinned so a wrong
 *  one is a build error.  The command structs mirror the producers in
 *  tr_cmds.c (a separate translation unit); duplicating them file-locally is
 *  intentional.
 * ------------------------------------------------------------------------- */

/* image_t -- the tr.images / scratch-image record.  Only the fields the
 * backend touches are named; the tr_* image globals are declared `int` but
 * hold an image_t*, so they are cast to this type at the point of use. */
typedef struct {
	char           imgName[64];       /* +0x00  (tr_image.c image_t) */
	unsigned short width;             /* +0x40 */
	unsigned short height;            /* +0x42 */
	unsigned short uploadWidth;       /* +0x44 */
	unsigned short uploadHeight;      /* +0x46 */
	int            cardMemory;        /* +0x48 */
	int            textureMemory;     /* +0x4c */
	int            imageTrack;        /* +0x50 */
	GLenum         textureTarget;     /* +0x54  GL_TEXTURE_2D ... (per-image enable cap) */
	GLuint         texnum;            /* +0x58 */
	int            frameUsed;         /* +0x5c  tr.frameCount when last bound */
	unsigned int   internalFormat;    /* +0x60 */
	unsigned int   flags;             /* +0x64  IMAGE_FLAG_* */
	int            TMU;               /* +0x68  RB_ShowImages skips images with a nonzero value */
} image_t;
ASSERT_OFF(image_t, width, 0x40);
ASSERT_OFF(image_t, uploadWidth, 0x44);
ASSERT_OFF(image_t, textureTarget, 0x54);
ASSERT_OFF(image_t, texnum, 0x58);
ASSERT_OFF(image_t, frameUsed, 0x5c);
ASSERT_OFF(image_t, TMU, 0x68);

/* Render-command ids (renderCommand_t).  RB_ExecuteRenderCommands dispatches
 * on these; the retail switch is written over (id - 1) so it is zero-based. */
typedef enum {
	RC_END_OF_LIST          = 0,
	RC_SET_COLOR            = 1,
	RC_STRETCH_PIC          = 2,
	RC_STRETCH_PIC_GRADIENT = 3,
	RC_STRETCH_PIC_ROTATE   = 4,
	RC_DRAW_QUAD_PIC        = 5,
	RC_TEXT_PAINT           = 6,
	RC_DRAW_SURFS           = 7,
	RC_DRAW_BUFFER          = 8,
	RC_SAVE_SCREEN          = 9,
	RC_BLEND_SAVED_SCREEN   = 10,
	RC_SWAP_BUFFERS         = 11
} renderCommand_t;

typedef struct {
	int   commandId;                  /* +0x00 */
	byte  color[4];                   /* +0x04  packed RGBA */
} setColorCommand_t;
COD1_ASSERT_SIZE(setColorCommand_t, 8);

typedef struct {
	int       commandId;              /* +0x00 */
	shader_t *shader;                 /* +0x04 */
	float     x, y, w, h;             /* +0x08 */
	float     s1, t1, s2, t2;         /* +0x18 */
} stretchPicCommand_t;
COD1_ASSERT_SIZE(stretchPicCommand_t, 40);

typedef struct {
	int       commandId;              /* +0x00 */
	shader_t *shader;                 /* +0x04 */
	float     x, y, w, h;             /* +0x08 */
	float     s1, t1, s2, t2;         /* +0x18 */
	byte      gradientColor[4];       /* +0x28 */
	int       gradientType;           /* +0x2c */
} stretchPicGradientCommand_t;
COD1_ASSERT_SIZE(stretchPicGradientCommand_t, 48);

typedef struct {
	int       commandId;              /* +0x00 */
	shader_t *shader;                 /* +0x04 */
	float     x, y, w, h;             /* +0x08 */
	float     s1, t1, s2, t2;         /* +0x18 */
	float     angle;                  /* +0x28  radians */
} stretchPicRotateCommand_t;
COD1_ASSERT_SIZE(stretchPicRotateCommand_t, 44);

typedef struct {
	int       commandId;              /* +0x00 */
	shader_t *shader;                 /* +0x04 */
	float     positions[4][2];        /* +0x08 */
	float     texCoords[4][2];        /* +0x28 */
} drawQuadPicCommand_t;
COD1_ASSERT_SIZE(drawQuadPicCommand_t, 72);

typedef struct {
	int         commandId;            /* +0x000 */
	byte        refdef[0x188];        /* +0x004  copy of tr.refdef (trRefdef_t) */
	viewParms_t viewParms;            /* +0x18c  copy of tr.viewParms */
	drawSurf_t *drawSurfs;            /* +0x3ec */
	int         numDrawSurfs;         /* +0x3f0 */
} drawSurfsCommand_t;
COD1_ASSERT_SIZE(drawSurfsCommand_t, 1012);
ASSERT_OFF(drawSurfsCommand_t, viewParms, 0x18c);
ASSERT_OFF(drawSurfsCommand_t, drawSurfs, 0x3ec);
ASSERT_OFF(drawSurfsCommand_t, numDrawSurfs, 0x3f0);

typedef struct {
	int commandId;                    /* +0x00 */
	int buffer;                       /* +0x04  GL draw-buffer enum */
} drawBufferCommand_t;
COD1_ASSERT_SIZE(drawBufferCommand_t, 8);

typedef struct {
	int commandId;                    /* +0x00 */
} saveScreenCommand_t;
COD1_ASSERT_SIZE(saveScreenCommand_t, 4);

typedef struct {
	int commandId;                    /* +0x00 */
	int duration;                     /* +0x04 */
} blendSavedScreenCommand_t;
COD1_ASSERT_SIZE(blendSavedScreenCommand_t, 8);

typedef struct {
	int commandId;                    /* +0x00 */
} swapBuffersCommand_t;
COD1_ASSERT_SIZE(swapBuffersCommand_t, 4);

/* shader flag bits the draw-surface dispatch tests */
#define SHADER_FLAG_ENTITY_MERGABLE   0x04    /* shader->flags & 4 */
#define SHADER_LIGHTING_ENTITY_MASK   0x18    /* shader->lightingFlags & 0x18 */
#define RF_DEPTH_RANGE_FLAGS          0x18    /* renderfx bits that select a depth range */
#define RF_FORCE_MIN_LIGHT            0x20    /* renderfx: clamp entity ambient up to r_entMinLight */

/* one entry of trRefEntity_t.lights[8] (at entity+0xD0); count at entity+0xCC */
typedef struct {
	dlight_t *light;                  /* +0x00 */
	float     scale;                  /* +0x04  per-light intensity */
} rbEntityLight_t;
COD1_ASSERT_SIZE(rbEntityLight_t, 8);

/* rendererDebugState.immediateVertices entry -- the immediate-mode vertex (position, one
 * texcoord, packed color) used by RE_StretchRaw / the overdraw overlay. */
typedef struct {
	float xyz[3];                     /* +0x00 */
	float s;                          /* +0x0c */
	float t;                          /* +0x10 */
	float lightmap[2];                /* +0x14  polyVert_t's lightmap ST (tr_surface.c); not written here */
	byte  color[4];                   /* +0x1c */
} rbImmediateVertex_t;
COD1_ASSERT_SIZE(rbImmediateVertex_t, 0x20);
ASSERT_OFF(rbImmediateVertex_t, s, 0x0c);
ASSERT_OFF(rbImmediateVertex_t, color, 0x1c);

/* ---- GL_Bind  0x004D5450 ----  VERIFIED */
void __cdecl GL_Bind(image_t *image)
{
  GLuint texnum;

  if ( !image )
  {
    ri.Printf(2, "GL_Bind: NULL image\n");
    texnum = ((image_t *)tr_defaultImage)->texnum;
  }
  else
  {
    texnum = image->texnum;
  }

  if ( r_nobind->integer )
  {
    if ( tr_dlightImage )
      texnum = ((image_t *)tr_dlightImage)->texnum;
  }
  if ( glState_cap[glState_currentTmu] != image->textureTarget )
  {
    if ( glState_cap[glState_currentTmu] )
      glDisable(glState_cap[glState_currentTmu]);
    glEnable(image->textureTarget);
    glState_cap[glState_currentTmu] = image->textureTarget;
  }
  if ( glState_currentTextures[glState_currentTmu] != texnum )
  {
    image->frameUsed = tr_frameCount;
    glState_currentTextures[glState_currentTmu] = texnum;
    glBindTexture(image->textureTarget, texnum);
  }
}

/* ---- GL_SetTextureTarget  0x004D5500 ----  VERIFIED */
void __cdecl GL_SetTextureTarget(GLenum target)
{
  GLenum current = glState_cap[glState_currentTmu];

  if ( current != target )
  {
    if ( current )
      glDisable(glState_cap[glState_currentTmu]);
    if ( target )
      glEnable(target);
    glState_cap[glState_currentTmu] = target;
  }
}

/* ---- GL_SelectTexture  0x004D5540 ----  VERIFIED */
void __cdecl GL_SelectTexture(int tmu)
{
  if ( glState_currentTmu != tmu )
  {
    qglActiveTextureARB(tmu + 33984);
    glState_currentTmu = tmu;
  }
  if ( glState_currentClientTmu != tmu )
  {
    qglClientActiveTextureARB(tmu + 33984);
    glState_currentClientTmu = tmu;
  }
}

/* ---- GL_BindMultitexture  0x004D5580 ----  VERIFIED
 * env0 is the (unused) TMU-0 texture env; only the two images bind. */
void __cdecl GL_BindMultitexture(image_t *image0, int env0, image_t *image1)
{
  GLuint texnum0;
  GLuint texnum1;

  texnum1 = image1->texnum;
  texnum0 = image0->texnum;

  if ( r_nobind->integer )
  {
    if ( tr_dlightImage )
    {
      texnum1 = ((image_t *)tr_dlightImage)->texnum;
      texnum0 = texnum1;
    }
  }
  if ( glState_currentTextures[1] != texnum1 )
  {
    if ( glState_currentTmu != 1 )
    {
      qglActiveTextureARB(33985);
      glState_currentTmu = 1;
    }
    if ( glState_currentClientTmu != 1 )
    {
      qglClientActiveTextureARB(33985);
      glState_currentClientTmu = 1;
    }
    image1->frameUsed = tr_frameCount;
    glState_currentTextures[1] = texnum1;
    glBindTexture(0xDE1u, texnum1);
  }
  if ( glState_currentTextures[0] != texnum0 )
  {
    if ( glState_currentTmu )
    {
      qglActiveTextureARB(33984);
      glState_currentTmu = 0;
    }
    if ( glState_currentClientTmu )
    {
      qglClientActiveTextureARB(33984);
      glState_currentClientTmu = 0;
    }
    image0->frameUsed = tr_frameCount;
    glState_currentTextures[0] = texnum0;
    glBindTexture(0xDE1u, texnum0);
  }
}

/* ---- GL_Cull  0x004D5670 ----  VERIFIED */
void __cdecl GL_Cull(int cullType)
{
  if ( glState_faceCulling == cullType )
    return;
  if ( cullType == 2 )
  {
    glDisable(0xB44u);
  }
  else
  {
    if ( glState_faceCulling == 2 )
      glEnable(0xB44u);
    if ( cullType == 1 )
    {
      if ( backEnd_viewParms_isMirror )
        qglCullFace(1028);
      else
        qglCullFace(1029);
    }
    else
    {
      if ( backEnd_viewParms_isMirror )
        qglCullFace(1029);
      else
        qglCullFace(1028);
    }
  }
  glState_faceCulling = cullType;
}

/* ---- GL_TexEnv  0x004D56E0 ----  VERIFIED */
void __cdecl GL_TexEnv(int env)
{
  if ( env == glState_texEnv[glState_currentTmu] )
    return;

  glState_texEnv[glState_currentTmu] = env;

  switch ( env )
  {
    case 0x2100:  /* GL_MODULATE */
      glTexEnvi(0x2300u, 0x2200u, 0x2100);
      break;
    case 0x1E01:  /* GL_REPLACE */
      glTexEnvi(0x2300u, 0x2200u, 0x1E01);
      break;
    case 0x2101:  /* GL_DECAL */
      glTexEnvi(0x2300u, 0x2200u, 0x2101);
      break;
    case 0x104:   /* GL_ADD */
      glTexEnvi(0x2300u, 0x2200u, 0x104);
      break;
    default:
      ri.Error(1, "\x15" "GL_TexEnv: invalid env '%x' passed\n", env);
      break;
  }
}

/* ---- GL_Normalize  0x004D5794 ----  VERIFIED
   real entry 0x004D5780 (retail list starts it at its tail); uncalled in retail */
void __cdecl GL_Normalize(GLenum target)
{
  if ( target == glState_normalizeTarget )
    return;
  if ( glState_normalizeTarget )
    glDisable(glState_normalizeTarget);
  glState_normalizeTarget = target;
  if ( target )
    glEnable(target);
}

/* ---- GL_State  0x004D57B0 ----  VERIFIED */
void __cdecl GL_State( unsigned long stateBits )
{
	unsigned long diff;
	GLenum        srcFactor;          /* edi */
	GLenum        dstFactor;          /* esi */
	float         offsetFactor;
	float         offsetUnits;

	diff = stateBits ^ glState_glStateBits;
	if ( !diff )                                        /* 0x004D57BF */
		return;

	/* ---- depth test and depth func  0x004D57C5 ---- */
	if ( diff & 0x70000 )
	{
		if ( diff & 0x10000 )
		{
			/* NOTE the polarity: the bit DISABLES.  0x004D57D3. */
			if ( stateBits & 0x10000 )
				glDisable( 0xB71u );
			else
				glEnable( 0xB71u );
		}
		if ( stateBits & 0x20000 )
			glDepthFunc( 0x202u );
		else if ( stateBits & 0x40000 )
			glDepthFunc( 0x207u );
		else
			glDepthFunc( 0x203u );
	}

	/* ---- depth mask  0x004D5817 ---- */
	if ( diff & 0x100 )
	{
		if ( stateBits & 0x100 )
			qglDepthMask( 1 );
		else
			qglDepthMask( 0 );
	}

	/* ---- polygon mode  0x004D5835 ---- */
	if ( diff & 0x1000 )
	{
		if ( stateBits & 0x1000 )
			glPolygonMode( 0x408u, 0x1B01u );
		else
			glPolygonMode( 0x408u, 0x1B02u );
	}

	/* ---- alpha test  0x004D585A ---- */
	if ( diff & 0x70000000 )
	{
		switch ( stateBits & 0x70000000 )
		{
			case 0:
				glDisable( 0xBC0u );
				break;
			case 0x10000000:
				glEnable( 0xBC0u );
				glAlphaFunc( 0x204u, 0.0f );
				break;
			case 0x20000000:
				glEnable( 0xBC0u );
				glAlphaFunc( 0x201u, 0.5f );
				break;
			case 0x40000000:
				glEnable( 0xBC0u );
				glAlphaFunc( 0x206u, 0.5f );
				break;
		}
	}

	if ( diff & 0x100000 )
	{
		if ( *(unsigned int *)(tr_world + 280) && !r_entFullbright->integer && ( stateBits & 0x100000 ) )
		{
			glEnable( 0xB50u );
		}
		else
		{
			glDisable( 0xB50u );
			if ( stateBits & 0x100000 )
			{
				if ( glState_clientStateBits & 0x100 )
				{
					glDisableClientState( 0x8076u );
					glState_clientStateBits &= ~0x100;
				}
				qglColor3f( tr_identityLight,
				               tr_identityLight,
				               tr_identityLight);
				stateBits &= ~0x100000;
			}
		}
	}

	if ( diff & 0x200000 )
	{
		if ( stateBits & 0x200000 )
		{
			R_FogOn();
		}
		else if ( glState_glStateBits & 0x200000 )
		{
			glDisable( 0xB60u );
			glState_glStateBits &= ~0x200000;
		}
	}

	/* ---- polygon offset  0x004D598D ---- */
	if ( diff & 0xE000 )
	{
		if ( stateBits & 0xE000 )
		{
			glEnable( 0x8037u );
			if ( stateBits & 0x2000 )
			{
				qglPolygonOffset( r_offsetfactor->value,
				               r_offsetunits->value);
			}
			else if ( stateBits & 0x4000 )
			{
				offsetUnits  = r_offsetunits->value  + r_offsetunits->value;
				offsetFactor = r_offsetfactor->value + r_offsetfactor->value;
				qglPolygonOffset( offsetFactor, offsetUnits);
			}
			else
			{
				qglPolygonOffset( 0.0f, r_offsetunits->value);
			}
		}
		else
		{
			glDisable( 0x8037u );
		}
	}

	/* ---- programmable pipeline toggles  0x004D5A0D ---- */
	if ( diff & 0x2000000 )
	{
		if ( stateBits & 0x2000000 )
			glEnable( 0x8620u );
		else
			glDisable( 0x8620u );
	}
	if ( diff & 0x400000 )
	{
		if ( stateBits & 0x400000 )
			glEnable( 0x86DEu );
		else
			glDisable( 0x86DEu );
	}
	if ( diff & 0x800000 )
	{
		if ( stateBits & 0x800000 )
			glEnable( 0x8522u );
		else
			glDisable( 0x8522u );
	}
	if ( diff & 0x1000000 )
	{
		if ( stateBits & 0x1000000 )
			glEnable( 0x8920u );
		else
			glDisable( 0x8920u );
	}

	glState_glStateBits = stateBits;

	/* ---- blend  0x004D5AA1 ---- */
	if ( diff & 0xFF )
	{
		if ( stateBits & 0xFF )
		{
			switch ( stateBits & 0xF )
			{
				case 1:  srcFactor = 0x000; break;
				case 2:  srcFactor = 0x001; break;
				case 3:  srcFactor = 0x306; break;
				case 4:  srcFactor = 0x307; break;
				case 5:  srcFactor = 0x302; break;
				case 6:  srcFactor = 0x303; break;
				case 7:  srcFactor = 0x304; break;
				case 8:  srcFactor = 0x305; break;
				case 9:  srcFactor = 0x308; break;
				default:
					srcFactor = 1;
					ri.Error( 1, "\x15" "GL_State: invalid src blend state bits\n" );
					break;
			}

			switch ( stateBits & 0xF0 )
			{
				case 0x10:  dstFactor = 0x000; break;
				case 0x20:  dstFactor = 0x001; break;
				case 0x30:  dstFactor = 0x300; break;
				case 0x40:  dstFactor = 0x301; break;
				case 0x50:  dstFactor = 0x302; break;
				case 0x60:  dstFactor = 0x303; break;
				case 0x70:  dstFactor = 0x304; break;
				case 0x80:  dstFactor = 0x305; break;
				default:
					dstFactor = 1;
					ri.Error( 1, "\x15" "GL_State: invalid dst blend state bits\n" );
					break;
			}

			glEnable( 0xBE2u );
			qglBlendFunc( srcFactor, dstFactor );
		}
		else
		{
			glDisable( 0xBE2u );
		}
		R_SetFogColor();                                /* tail jumps 0x004D5B9E / 0x004D5BB3 */
	}
}


/* ---- GL_ClientState  0x004D5C80 ----  VERIFIED */
int __cdecl GL_ClientState(int stateBits)
{
  int diff;

  diff = glState_clientStateBits ^ stateBits;
  if ( !diff )
    return stateBits;
  if ( diff & 0x1 )
  {
    if ( glState_currentClientTmu != 0 )
      qglClientActiveTextureARB(0x84C0);
    if ( stateBits & 0x1 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 0 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x2 )
  {
    if ( glState_currentClientTmu != 1 )
      qglClientActiveTextureARB(0x84C1);
    if ( stateBits & 0x2 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 1 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x4 )
  {
    if ( glState_currentClientTmu != 2 )
      qglClientActiveTextureARB(0x84C2);
    if ( stateBits & 0x4 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 2 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x8 )
  {
    if ( glState_currentClientTmu != 3 )
      qglClientActiveTextureARB(0x84C3);
    if ( stateBits & 0x8 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 3 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x10 )
  {
    if ( glState_currentClientTmu != 4 )
      qglClientActiveTextureARB(0x84C4);
    if ( stateBits & 0x10 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 4 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x20 )
  {
    if ( glState_currentClientTmu != 5 )
      qglClientActiveTextureARB(0x84C5);
    if ( stateBits & 0x20 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 5 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x40 )
  {
    if ( glState_currentClientTmu != 6 )
      qglClientActiveTextureARB(0x84C6);
    if ( stateBits & 0x40 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 6 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x80 )
  {
    if ( glState_currentClientTmu != 7 )
      qglClientActiveTextureARB(0x84C7);
    if ( stateBits & 0x80 )
      glEnableClientState(0x8078u);
    else
      glDisableClientState(0x8078u);
    if ( glState_currentClientTmu != 7 )
      qglClientActiveTextureARB(glState_currentClientTmu + 0x84C0);
  }
  if ( diff & 0x100 )
  {
    if ( stateBits & 0x100 )
      glEnableClientState(0x8076u);
    else
      glDisableClientState(0x8076u);
  }
  if ( diff & 0x200 )
  {
    if ( stateBits & 0x200 )
      glEnableClientState(0x8075u);
    else
      glDisableClientState(0x8075u);
  }
  if ( diff & 0x400 )
  {
    if ( stateBits & 0x400 )
      glEnableClientState(0x8074u);
    else
      glDisableClientState(0x8074u);
  }
  glState_clientStateBits = stateBits;
  return stateBits;
}

/* ---- GL_DrawElements  0x004D5F60 ----  VERIFIED */
void __cdecl GL_DrawElements(GLsizei numIndexes, GLenum mode, GLenum type, GLvoid *indices)
{
  backEnd_pc_drawnIndexCount += numIndexes;
  ++backEnd_pc_drawCallCount;
  glDrawElements(mode, numIndexes, type, indices);
}

/* ---- GL_DrawRangeElements  0x004D5FA0 ----  VERIFIED */
void __cdecl GL_DrawRangeElements(
        GLsizei numIndexes,
        GLenum mode,
        int end,
        GLenum type,
        const GLvoid *indices,
        int start)
{
  if ( qglDrawRangeElementsEXT && end )
  {
    backEnd_pc_drawnIndexCount += numIndexes;
    ++backEnd_pc_drawCallCount;
    qglDrawRangeElementsEXT(mode, start, end, numIndexes, type, indices);
  }
  else
  {
    backEnd_pc_drawnIndexCount += numIndexes;
    ++backEnd_pc_drawCallCount;
    glDrawElements(mode, numIndexes, type, indices);
  }
}

/* ---- GL_DrawElementArrayATI  0x004D6000 ----  VERIFIED */
void __cdecl GL_DrawElementArrayATI(int numIndexes, int mode)
{
  backEnd_pc_drawnIndexCount += numIndexes;
  ++backEnd_pc_drawCallCount;
  glDrawElementArrayATI(mode, numIndexes);
}

/* ---- SetViewportAndScissor  0x004D6080 ----  VERIFIED */
void SetViewportAndScissor( void )
{
	glMatrixMode( 0x1701u );
	glLoadMatrixf( backEnd_viewParms_projectionMatrix );  /* viewParms +328, 0x016D8C90 */
	glMatrixMode( 0x1700u );
	glViewport( backEnd_viewParms_viewportX, backEnd_viewParms_viewportY,
	            backEnd_viewParms_viewportWidth, backEnd_viewParms_viewportHeight );
	glScissor( backEnd_viewParms_viewportX, backEnd_viewParms_viewportY,
	           backEnd_viewParms_viewportWidth, backEnd_viewParms_viewportHeight );
}

/* ---- RB_BeginDrawingView  0x004D60F0 ----  VERIFIED */
__declspec(noinline) static void RB_Hyperspace(void) { float grey=(float)(backEnd_refdef_time & 255) * (1.0f/255.0f); glClearColor(grey,grey,grey,1.0f);glClear(0x4000u);backEnd_isHyperspace=1;}
void RB_BeginDrawingView( void )
{
	int   clearBits;      /* esi */
	unsigned char flags;          /* cl -- only the LOW BYTE of backEnd_refdef_rdflags */
	float grey;
	float one = 1.0f;

	if ( r_finish->integer == 1 && !glState_finishCalled )
	{
		qglFinish();
		glState_finishCalled = 1;
	}
	if ( !r_finish->integer )
		glState_finishCalled = 1;

	backEnd_projection2D = 0;
	SetViewportAndScissor();
	GL_State( 0x100 );
	glState_currentLightingEntity = 0;

	flags     = backEnd_refdef_rdflags & 0xFF;
	clearBits = 0;

	if ( cg_shadows->integer == 2
	  || ( r_measureOverdraw->integer && ( !rendererSkyboxPortal || ( flags & 8 ) ) ) )
	{
		clearBits = 0x400;
	}

	if ( r_uifullscreen->integer )
	{
		clearBits = 0x100;
		goto clear;
	}

	clearBits |= 0x100;

	if ( rendererSkyboxPortal )
	{
		if ( flags & 8 )
		{
			if ( !r_fastsky->integer && !( flags & 1 ) )
			{
				if ( tr_fogPortalView_registered && tr_fogPortalView_clearScreen )
				{
					glClearColor( tr_fogPortalView_colorR, tr_fogPortalView_colorG,
					              tr_fogPortalView_colorB, tr_fogPortalView_colorA );
					clearBits |= 0x4000;
				}
			}
			else
			{
				clearBits |= 0x4000;
				if ( tr_fogPortalView_registered )
				{
					glClearColor( tr_fogPortalView_colorR, tr_fogPortalView_colorG,
					              tr_fogPortalView_colorB, tr_fogPortalView_colorA );
				}
				else if ( glfogNum > 0 && tr_fogCurrent_registered )
				{
					glClearColor( tr_fogCurrent_colorR, tr_fogCurrent_colorG,
					              tr_fogCurrent_colorB, tr_fogCurrent_colorA);
				}
				else
				{
					grey = tr_identityLight * 0.5f;
					glClearColor( grey, grey, grey, one );
				}
			}
		}
		else if ( glfogNum > 0 && tr_fogCurrent_registered )
		{
			if ( flags & 0x20 )
			{
				if ( tr_fogCurrent_mode == 0x2601 )
					clearBits |= 0x4000;
			}
			else if ( !cg_skybox->integer )
			{
				clearBits |= 0x4000;
			}
			if ( clearBits & 0x4000 )
			{
				glClearColor( tr_fogCurrent_colorR, tr_fogCurrent_colorG,
				              tr_fogCurrent_colorB, tr_fogCurrent_colorA);
			}
		}
	}
	else if ( flags & 1 )
	{
		clearBits &= ~0x4000;                           /* 0x004D62F3 */
	}
	else if ( r_fastsky->integer )
	{
		clearBits |= 0x4000;
		if ( tr_fogCurrent_registered )
		{
			glClearColor( tr_fogCurrent_colorR, tr_fogCurrent_colorG,
			              tr_fogCurrent_colorB, tr_fogCurrent_colorA);
		}
		else
		{
			grey = tr_identityLight * 0.5f;             /* VC7 cross-jumps this into the copy above */
			glClearColor( grey, grey, grey, one );
		}
	}
	else if ( tr_fogCurrent_registered && tr_fogCurrent_clearScreen )
	{
		glClearColor( tr_fogCurrent_colorR, tr_fogCurrent_colorG,
		              tr_fogCurrent_colorB, tr_fogCurrent_colorA);
		clearBits |= 0x4000;
	}

clear:
	if ( clearBits )
		glClear( clearBits );

done:
	if ( backEnd_refdef_rdflags & 4 )
	{
		RB_Hyperspace();
	}
	else
	{
		backEnd_isHyperspace = 0;
		backEnd_skyRenderedThisView = 0;
	}

}


/* ---- RB_SetupLight  0x004D63A0 ----  VERIFIED
 * GL enums: 0x1200 GL_AMBIENT, 0x1201 GL_DIFFUSE, 0x1202 GL_SPECULAR,
 * 0x1203 GL_POSITION, 0x1204 GL_SPOT_DIRECTION, 0x1205 GL_SPOT_EXPONENT,
 * 0x1206 GL_SPOT_CUTOFF, 0x1207/8/9 GL_{CONSTANT,LINEAR,QUADRATIC}_ATTENUATION,
 * 0x4000 GL_LIGHT0.  Parameter order kept as decompiled (intensity, index, light). */
void __cdecl RB_SetupLight( float intensity, int lightIndex, dlight_t *light )
{
  GLenum lightName;
  float  ambient[4];
  float  diffuse[4];

  lightName = lightIndex + 0x4000;

  if ( intensity != 1.0f )                          /* 0x004D63A7 */
  {
    ambient[0] = intensity * light->ambient[0];
    ambient[1] = intensity * light->ambient[1];
    ambient[2] = intensity * light->ambient[2];
    ambient[3] = 1.0f;
    diffuse[0] = intensity * light->diffuse[0];
    diffuse[1] = intensity * light->diffuse[1];
    diffuse[2] = intensity * light->diffuse[2];
    diffuse[3] = 1.0f;
    qglLightfv( lightName, 0x1200u, ambient );
    qglLightfv( lightName, 0x1201u, diffuse );
  }
  else
  {
    qglLightfv( lightName, 0x1200u, light->ambient );
    qglLightfv( lightName, 0x1201u, light->diffuse );
  }

  qglLightfv( lightName, 0x1202u, light->specular );
  qglLightfv( lightName, 0x1203u, light->position );
  qglLightfv( lightName, 0x1204u, light->spotDirection );

  qglLightf( lightName, 0x1205u, light->spotExponent );
  qglLightf( lightName, 0x1206u, light->spotCutoff );
  qglLightf( lightName, 0x1207u, light->constantAttenuation );
  qglLightf( lightName, 0x1208u, light->linearAttenuation );
  qglLightf( lightName, 0x1209u, light->quadraticAttenuation );

  if ( lightIndex >= glState_numEnabledLights )                /* 0x004D64C5 */
    glEnable( lightName );
}

/* ---- RB_EnableHWLights  0x004D64E0 ----  VERIFIED
 * Set up the fixed-function lights for backEnd.currentEntity: the light-model
 * ambient (world base + sun-scaled contribution, optionally clamped up to
 * r_entMinLight) plus the entity's own lights, insertion-sorted by light type
 * then by light address.  Returns 1 if it loaded the world modelview matrix
 * (so the caller reloads the entity matrix afterwards).
 *
 * The lights[] fields (0xCC count, 0xD0 array) and diffuseSunContribution
 * (0x110) sit in trRefEntity_t's unproven interior, so they are reached by raw
 * offset.
 *
 * ORIGINAL BUG (faithful): the light-model ambient is a 3-float R,G,B; its 4th
 * (alpha) component is the aliased first sort slot and is passed to
 * glLightModelfv uninitialized. */
int __cdecl RB_EnableHWLights(void)
{
  struct { float ambient[3];rbEntityLight_t *sortedLights[8]; } lightFrame; float *ambient=lightFrame.ambient;rbEntityLight_t **sortedLights=lightFrame.sortedLights;
  rbEntityLight_t  *lights;
  int               lightCount;
  int               loadedMatrix;
  int               i;
  int               j;
  rbEntityLight_t **tail;
  float             luminance;
  rfloat            minLight;          /* x87 stack values in retail, never rounded */
  rfloat            scale;

  if ( glState_currentLightingEntity == backEnd_currentEntity
    && glState_currentLightingFlags == (((shader_t *)tess_shader)->lightingFlags & SHADER_LIGHTING_ENTITY_MASK) )
  {
    return 0;
  }
  glState_currentLightingEntity = backEnd_currentEntity;
  glState_currentLightingFlags  = ((shader_t *)tess_shader)->lightingFlags & SHADER_LIGHTING_ENTITY_MASK;

  /* entityAmbientScale * entity->diffuseSunContribution + entityAmbientBase */
  loadedMatrix = 0;
  ambient[0] = (rfloat)*(float *)(tr_world + 248) * ((trRefEntity_t *)backEnd_currentEntity)->diffuseSun + *(float *)(tr_world + 216);
  ambient[1] = (rfloat)*(float *)(tr_world + 252) * ((trRefEntity_t *)backEnd_currentEntity)->diffuseSun + *(float *)(tr_world + 220);
  ambient[2] = (rfloat)*(float *)(tr_world + 256) * ((trRefEntity_t *)backEnd_currentEntity)->diffuseSun + *(float *)(tr_world + 224);

  if ( (((refEntity_t *)backEnd_currentEntity)->renderfx & RF_FORCE_MIN_LIGHT) != 0 )
  {
    luminance = (rfloat)ambient[2] * 0.114f + (rfloat)ambient[1] * 0.58700001f + (rfloat)ambient[0] * 0.29899999f;
    minLight  = (rfloat)tr_identityLight * r_entMinLight->value;

    if ( luminance == 0.0f )
    {
      ambient[0] = minLight;
      ambient[1] = minLight;
      ambient[2] = minLight;
    }
    else if ( luminance < minLight )
    {
      scale = minLight / luminance;
      ambient[0] = ambient[0] * scale;
      ambient[1] = ambient[1] * scale;
      ambient[2] = ambient[2] * scale;
    }
  }

  glLightModelfv(0xB53u, ambient);   /* GL_LIGHT_MODEL_AMBIENT */

  lightCount = ((trRefEntity_t *)backEnd_currentEntity)->lightCount;   /* entity->lightCount */
  lights     = (rbEntityLight_t *)(backEnd_currentEntity + 208);  /* entity->lights */

  /* insertion sort: keep lights ordered by type, then by light address */
  tail = sortedLights - 1;
  for ( i = 0; i < lightCount; ++i, ++tail )
  {
    rbEntityLight_t  *entry = &lights[i];
    rbEntityLight_t **walk;
    rbEntityLight_t  *prev;
    int               type;

    j = i;
    if ( j > 0 )
    {
      type = entry->light->type;
      walk = tail;
      do
      {
        prev = *walk;
        if ( type > prev->light->type )
          break;
        if ( (unsigned int)entry->light > (unsigned int)prev->light )
          break;
        walk[1] = prev;
        --j;
        --walk;
      }
      while ( j > 0 );
    }
    sortedLights[j] = entry;
  }
  if ( lightCount != 0 )
  {
    glLoadMatrixf(tr_viewParms_world_modelMatrix);
    loadedMatrix = 1;
  }

  for ( i = 0; i < ((trRefEntity_t *)backEnd_currentEntity)->lightCount; ++i )
    RB_SetupLight(sortedLights[i]->scale, i, sortedLights[i]->light);
  for ( ; i < glState_numEnabledLights; ++i )
    glDisable(i + 0x4000);
  glState_numEnabledLights = ((trRefEntity_t *)backEnd_currentEntity)->lightCount;
  return loadedMatrix;
}

/* ---- RB_RenderDrawSurfList  0x004D6710 ----  [HIGH] */
void __cdecl RB_RenderDrawSurfList(int surfCount, int surfList)
{
  const int   numDrawSurfs = surfCount;
  drawSurf_t *drawSurfs    = (drawSurf_t *)surfList;
  trRefEntity_t *currentEntity;
  float       originalTime;
  unsigned    oldSort;
  int         oldShader;                 /* shader_t* as int (0 => none) */
  int         oldEntityNum;
  int         oldStorageMode;
  int         oldBatchFlag0;
  int         oldBatchFlag2;
  int         oldDepthRange;
  int         depthRange;                /* depth-range flags of the current entity */
  int         shaderFlagChanges;
  int         i;
  drawSurf_t *drawSurf;
  int         entityNum;
  int         shader;
  int         storageMode;
  int         batchFlag0;
  int         batchFlag2;
  rfloat      entityTime;                /* x87 st(0): fst rounds only the floatTime copy */

  originalTime = backEnd_refdef_floatTime;
  RB_BeginDrawingView();

  oldStorageMode        = 0;
  oldEntityNum          = -1;
  backEnd_currentEntity = (int)&tr_worldEntity;
  oldShader             = 0;
  oldDepthRange         = 0;
  oldBatchFlag0         = 0;
  oldSort               = (unsigned)-1;
  depthRange            = 0;
  shaderFlagChanges     = 0;
  oldBatchFlag2         = -1;
  backEnd_pc_surfaceCount += numDrawSurfs;

  for ( i = 0, drawSurf = drawSurfs; i < numDrawSurfs; i++, drawSurf++ )
  {
    /* fast path: identical sort key to the previous surface */
    if ( drawSurf->sort == oldSort )
    {
      rb_surfaceTable[*(unsigned int *)drawSurf->surface](drawSurf->surface);
      continue;
    }
    oldSort = drawSurf->sort;

    /* decompose the sort key */
    entityNum   = (oldSort >> 8) & 0x3FF;
    shader      = tr_sortedShaderSlots[(oldSort >> 18) & 0xFFF];   /* tr.sortedShaders[] */
    storageMode = oldSort >> 30;
    batchFlag0  = oldSort & 1;
    batchFlag2  = (oldSort >> 2) & 1;
    if ( oldSort & 2 )                                   /* world-entity flag */
      entityNum = 1022;

    /* start a new tess batch when the batch key changed, or when the
     * entity changed and the shader is not entity-mergable */
    if ( shader != oldShader || storageMode != oldStorageMode
      || batchFlag0 != oldBatchFlag0 || batchFlag2 != oldBatchFlag2
      || ( entityNum != oldEntityNum
        && (((shader_t *)shader)->flags & SHADER_FLAG_ENTITY_MERGABLE) == 0 ) )
    {
      if ( oldShader )
      {
        RB_EndSurface();
        shaderFlagChanges = ((shader_t *)oldShader)->flags ^ ((shader_t *)shader)->flags;
      }

      if ( entityNum != 1022 )
        {currentEntity=(trRefEntity_t *)(backEnd_refdef_entities+696*entityNum);backEnd_currentEntity=(int)currentEntity;}
      else
        backEnd_currentEntity = (int)&tr_worldEntity;

      if ( storageMode != glState_currentStorageMode )
      {
        if ( glConfig_NVVertexArrayRange )
          RB_SelectStorageNV(storageMode);
        else if ( glConfig_ATIVertexArrayObject )
          RB_SelectStorageATI(storageMode);
        glState_currentStorageMode = storageMode;
      }
      RB_BeginSurface((void *)shader, 3);   /* 0x004D6886: push 3 / mov ecx,ebp */
      oldShader      = shader;
      oldBatchFlag0  = batchFlag0;
      oldBatchFlag2  = batchFlag2;
      oldStorageMode = storageMode;
    }

    if ( entityNum != oldEntityNum )
    {
      /* ---- entity changed: rebuild the modelview / lighting ---- */
      depthRange = 0;
      if ( entityNum != 1022 )
      {
        {currentEntity=(trRefEntity_t *)(backEnd_refdef_entities+696*entityNum);backEnd_currentEntity=(int)currentEntity;}
        entityTime = (rfloat)originalTime - *(float *)((char *)currentEntity+0x78);   /* e.shaderTime */
        backEnd_refdef_floatTime = entityTime;
        tess_shaderTime = entityTime - ((shader_t *)tess_shader)->timeOffset;   /* shader->timeOffset */

        if ( (((shader_t *)tess_shader)->flags & SHADER_FLAG_ENTITY_MERGABLE) != 0 )
          backEnd_or = *(tr_orientationr_t *)backEnd_viewParms_world;
        else if ( currentEntity->e.reType != 1
               && currentEntity->e.reType != 2
               && currentEntity->e.reType != 0 )
          backEnd_or = *(tr_orientationr_t *)backEnd_viewParms_world;
        else
          R_RotateForModelEntity((int)currentEntity, backEnd_viewParms_originX, &backEnd_or);

        /* after the call retail re-reads backEnd.currentEntity (Q3 form), keeping 0 in ebp */
        if ( ((trRefEntity_t *)backEnd_currentEntity)->dlightBits )
          R_TransformDlights(backEnd_refdef_entityDlightCount, (float *)backEnd_refdef_dlights, &backEnd_or);

        depthRange = ((trRefEntity_t *)backEnd_currentEntity)->e.renderfx & RF_DEPTH_RANGE_FLAGS;
        if ( (((shader_t *)tess_shader)->lightingFlags & SHADER_LIGHTING_ENTITY_MASK) != 0 )
        {
          RB_EnableHWLights();
          GL_Normalize(((trRefEntity_t *)backEnd_currentEntity)->normalizationTarget);
        }
        else
        {
          GL_Normalize(0);
        }
      }
      else
      {
        backEnd_refdef_floatTime = originalTime;
        backEnd_currentEntity    = (int)&tr_worldEntity;
        backEnd_or = *(tr_orientationr_t *)backEnd_viewParms_world;
        tess_shaderTime = originalTime - ((shader_t *)tess_shader)->timeOffset;   /* shader->timeOffset */
        R_TransformDlights(backEnd_refdef_num_dlights, (float *)backEnd_refdef_dlights, &backEnd_or);
        GL_Normalize(0);
      }

      glLoadMatrixf(backEnd_or.modelMatrix);

      if ( oldDepthRange != depthRange )
      {
        switch ( depthRange )
        {
          case 0:
            glDepthRange(0.0, 1.0);
            break;
          case 8:                                          /* RF_DEPTHHACK */
            glMatrixMode(0x1701u);
            glLoadMatrixf(backEnd_viewParms_depthHackProjectionMatrix);
            glMatrixMode(0x1700u);
            glDepthRange(0.0, 0.20000000298023224);
            break;
          case 0x10:                                       /* RF_CROSSHAIR */
          case 0x18:
            glDepthRange(0.0, 0.5);
            break;
        }
        if ( oldDepthRange == 8 )                          /* leaving the depth hack: restore projection */
        {
          glMatrixMode(0x1701u);
          glLoadMatrixf(backEnd_viewParms_projectionMatrix);
          glMatrixMode(0x1700u);
        }
        oldDepthRange = depthRange;
      }

      oldEntityNum = entityNum;
    }
    else
    {
      /* ---- entity unchanged: refresh lighting only ---- */
      if ( (((shader_t *)tess_shader)->lightingFlags & SHADER_LIGHTING_ENTITY_MASK) != 0 )
      {
        if ( RB_EnableHWLights() )
          glLoadMatrixf(backEnd_or.modelMatrix);
        GL_Normalize(((trRefEntity_t *)backEnd_currentEntity)->normalizationTarget);
      }
      else
      {
        GL_Normalize(0);
      }

      if ( entityNum != 1022 && (shaderFlagChanges & SHADER_FLAG_ENTITY_MERGABLE) != 0 )
      {
        if ( (((shader_t *)tess_shader)->flags & SHADER_FLAG_ENTITY_MERGABLE) != 0 )
          backEnd_or = *(tr_orientationr_t *)backEnd_viewParms_world;
        else if ( ((trRefEntity_t *)backEnd_currentEntity)->e.reType != 1
               && ((trRefEntity_t *)backEnd_currentEntity)->e.reType != 2
               && ((trRefEntity_t *)backEnd_currentEntity)->e.reType != 0 )
          backEnd_or = *(tr_orientationr_t *)backEnd_viewParms_world;
        else
          R_RotateForModelEntity(backEnd_currentEntity, backEnd_viewParms_originX, &backEnd_or);
        glLoadMatrixf(backEnd_or.modelMatrix);
      }
    }

    rb_surfaceTable[*(unsigned int *)drawSurf->surface](drawSurf->surface);
  }

  if ( oldShader )
    RB_EndSurface();

  /* restore the default storage class and the world modelview (the class read
     once into a local, as RB_DrawStretchPic does: retail holds it in edi) */
  {
    int storage = storageClass;
    if ( storageClass != glState_currentStorageMode )
    {
      if ( glConfig_NVVertexArrayRange )
        RB_SelectStorageNV(storageClass);
      else if ( glConfig_ATIVertexArrayObject )
        RB_SelectStorageATI(storageClass);
      glState_currentStorageMode = storage;
    }
  }
  backEnd_refdef_floatTime = originalTime;
  backEnd_currentEntity    = (int)&tr_worldEntity;
  backEnd_or = *(tr_orientationr_t *)backEnd_viewParms_world;
  R_TransformDlights(backEnd_refdef_num_dlights, (float *)backEnd_refdef_dlights, &backEnd_or);
  glLoadMatrixf(backEnd_viewParms_worldModelMatrix);
  if ( depthRange )
    glDepthRange(0.0, 1.0);
  RB_ShadowFinish();
  RB_RenderFlares();
}

/* ---- RB_SetGL2D  0x004D6CB0 ----  VERIFIED */
void RB_SetGL2D( void )
{
	backEnd_projection2D = 1;
	RB_EndMultitexture();
	glViewport( 0, 0, glConfig_vidWidth, glConfig_vidHeight );
	glScissor( 0, 0, glConfig_vidWidth, glConfig_vidHeight );
	glMatrixMode( 0x1701u );
	qglLoadIdentity();
	qglOrtho( 0.0, (double)glConfig_vidWidth, (double)glConfig_vidHeight,
	               0.0, 0.0, 1.0 );
	glMatrixMode( 0x1700u );
	qglLoadIdentity();
	GL_State( 0x10065 );

	if ( glState_glStateBits & 0x200000 )
	{
		glDisable( 0xB60u );
		glState_glStateBits &= ~0x200000;
	}
	if ( glState_faceCulling != 2 )
	{
		glDisable( 0xB44u );
		glState_faceCulling = 2;
	}
	glDisable( 0x3000u );

	backEnd_refdef_time = ri.Milliseconds();
	backEnd_refdef_floatTime = (rfloat)backEnd_refdef_time * 0.001f;
}


/* ---- RE_StretchRaw  0x004D6DB0 ----  VERIFIED
 * Retail's Hex-Rays signature carries a leading `const char *@<ebx>`: that is
 * only the caller's EBX leaking through (RB_ExecuteRenderCommands takes only
 * cmds, in eax), NOT a stack argument -- the nine stack args below are what
 * CIN_DrawCinematic pushes through re.DrawStretchRaw. */
void __cdecl RE_StretchRaw(
        int destX,
        int destY,
        int destW,
        int destH,
        GLsizei width,
        GLsizei height,
        GLvoid *pixels,
        int scratchIndex,
        int dirty)
{
  int      powW, powH;
  int      startMsec;
  int      fogEnabled;
  image_t *image;
  byte     colorByte;
  rbImmediateVertex_t *vtx;

  if ( !tr_registered[0] )
    return;

  R_SyncRenderThread();

  /* the cinematics path syncs every frame */
  qglFinish();
  startMsec = 0;
  if ( r_speeds->integer )
    startMsec = ri.Milliseconds();

  /* require power-of-two dimensions */
  powW = 0;
  if ( width > 1 )
  {
    do
      ++powW;
    while ( 1 << powW < width );
  }
  powH = 0;
  if ( height > 1 )
  {
    do
      ++powH;
    while ( 1 << powH < height );
  }
  if ( 1 << powW != width || 1 << powH != height )
    ri.Error(1, "\x15" "Draw_StretchRaw: size not a power of 2: %i by %i", width, height);

  image = (image_t *)tr_scratchImages[scratchIndex];
  GL_Bind(image);
  if ( width == ((image_t *)tr_scratchImages[scratchIndex])->width && height == ((image_t *)tr_scratchImages[scratchIndex])->height )
  {
    if ( dirty )
      glTexSubImage2D(0xDE1u, 0, 0, 0, width, height, 0x1908u, 0x1401u, pixels);
  }
  else
  {
    ((image_t *)tr_scratchImages[scratchIndex])->uploadWidth  = width;
    ((image_t *)tr_scratchImages[scratchIndex])->width        = width;
    ((image_t *)tr_scratchImages[scratchIndex])->uploadHeight = height;
    ((image_t *)tr_scratchImages[scratchIndex])->height       = height;
    glTexImage2D(0xDE1u, 0, 3, width, height, 0, 0x1908u, 0x1401u, pixels);
    glTexParameteri(0xDE1u, 0x2801u, 9729);       /* GL_TEXTURE_MIN_FILTER, GL_LINEAR */
    glTexParameteri(0xDE1u, 0x2800u, 9729);       /* GL_TEXTURE_MAG_FILTER, GL_LINEAR */
    glTexParameteri(0xDE1u, 0x2802u, 33071);      /* GL_TEXTURE_WRAP_S, GL_CLAMP */
    glTexParameteri(0xDE1u, 0x2803u, 33071);      /* GL_TEXTURE_WRAP_T, GL_CLAMP */
  }
  if ( r_speeds->integer )
    ri.Printf(0, "qglTexSubImage2D %i, %i: %i msec\n", width, height, ri.Milliseconds() - startMsec);

  RB_SetGL2D();
  RB_BeginImmediateMode();

  colorByte = (byte)(unsigned __int64)(tr_identityLight * 255.0f);
  rendererDebugState.immediateColorR = colorByte;
  rendererDebugState.immediateColorG = colorByte;
  rendererDebugState.immediateColorB = colorByte;
  rendererDebugState.immediateColorA = -1;
  rendererDebugState.immediateMode = 7;           /* GL_QUADS */

  /* four corners: (destX,destY) .. (destX+destW, destY+destH), texels centered by half a pixel.
   * Retail's inlined RB_glTexCoord2f stores the texcoords as floats (fst straight to
   * rendererDebugState, written out here; the int copy the tree's out-of-line
   * RB_glTexCoord2f uses forces them through a stack slot), and each vertex, emitted
   * only while the immediate buffer has room, reads them back.  The primitive mode is
   * the rendererDebugState field: through the separate `mode` global VC7 would forward
   * the constant 7 to glDrawArrays, where retail reloads it. */
  rendererDebugState.immediateTexCoordS = 0.5f / width;
  rendererDebugState.immediateTexCoordT = 0.5f / height;
  if ( rendererDebugState.immediateVertexCount < rendererDebugState.immediateVertexCapacity )
  {
    vtx = &((rbImmediateVertex_t *)rendererDebugState.immediateVertices)[rendererDebugState.immediateVertexCount];
    vtx->color[0] = rendererDebugState.immediateColorR;  vtx->color[1] = rendererDebugState.immediateColorG;
    vtx->color[2] = rendererDebugState.immediateColorB;  vtx->color[3] = rendererDebugState.immediateColorA;
    vtx->s = *(float *)&rendererDebugState.immediateTexCoordS;  vtx->t = *(float *)&rendererDebugState.immediateTexCoordT;
    vtx->xyz[0] = (float)destX;  vtx->xyz[1] = (float)destY;  vtx->xyz[2] = 0.0f;
    ++rendererDebugState.immediateVertexCount;
  }
  rendererDebugState.immediateTexCoordS = ( width - 0.5f ) / width;
  rendererDebugState.immediateTexCoordT = 0.5f / height;
  if ( rendererDebugState.immediateVertexCount < rendererDebugState.immediateVertexCapacity )
  {
    vtx = &((rbImmediateVertex_t *)rendererDebugState.immediateVertices)[rendererDebugState.immediateVertexCount];
    vtx->color[0] = rendererDebugState.immediateColorR;  vtx->color[1] = rendererDebugState.immediateColorG;
    vtx->color[2] = rendererDebugState.immediateColorB;  vtx->color[3] = rendererDebugState.immediateColorA;
    vtx->s = *(float *)&rendererDebugState.immediateTexCoordS;  vtx->t = *(float *)&rendererDebugState.immediateTexCoordT;
    vtx->xyz[0] = (float)(destW + destX);  vtx->xyz[1] = (float)destY;  vtx->xyz[2] = 0.0f;
    ++rendererDebugState.immediateVertexCount;
  }
  rendererDebugState.immediateTexCoordS = ( width - 0.5f ) / width;
  rendererDebugState.immediateTexCoordT = ( height - 0.5f ) / height;
  if ( rendererDebugState.immediateVertexCount < rendererDebugState.immediateVertexCapacity )
  {
    vtx = &((rbImmediateVertex_t *)rendererDebugState.immediateVertices)[rendererDebugState.immediateVertexCount];
    vtx->color[0] = rendererDebugState.immediateColorR;  vtx->color[1] = rendererDebugState.immediateColorG;
    vtx->color[2] = rendererDebugState.immediateColorB;  vtx->color[3] = rendererDebugState.immediateColorA;
    vtx->s = *(float *)&rendererDebugState.immediateTexCoordS;  vtx->t = *(float *)&rendererDebugState.immediateTexCoordT;
    vtx->xyz[0] = (float)(destW + destX);  vtx->xyz[1] = (float)(destY + destH);  vtx->xyz[2] = 0.0f;
    ++rendererDebugState.immediateVertexCount;
  }
  rendererDebugState.immediateTexCoordS = 0.5f / width;
  rendererDebugState.immediateTexCoordT = ( height - 0.5f ) / height;
  if ( rendererDebugState.immediateVertexCount < rendererDebugState.immediateVertexCapacity )
  {
    vtx = &((rbImmediateVertex_t *)rendererDebugState.immediateVertices)[rendererDebugState.immediateVertexCount];
    vtx->color[0] = rendererDebugState.immediateColorR;  vtx->color[1] = rendererDebugState.immediateColorG;
    vtx->color[2] = rendererDebugState.immediateColorB;  vtx->color[3] = rendererDebugState.immediateColorA;
    vtx->s = *(float *)&rendererDebugState.immediateTexCoordS;  vtx->t = *(float *)&rendererDebugState.immediateTexCoordT;
    vtx->xyz[0] = (float)destX;  vtx->xyz[1] = (float)(destH + destY);  vtx->xyz[2] = 0.0f;
    ++rendererDebugState.immediateVertexCount;
  }

  glDrawArrays(rendererDebugState.immediateMode, 0, rendererDebugState.immediateVertexCount);
  rendererDebugState.immediateVertexCount = 0;
  rendererDebugState.immediateMode = 0;
  rendererDebugState.immediateModeActive = 0;

  if ( !backEnd_projection2D && (glState_glStateBits & 0x200000) == 0 )
  {
    if ( r_fog->integer )
    {
      fogEnabled = tr_fogPortalView_registered;
      if ( (backEnd_refdef_rdflags & 8) == 0 )
        fogEnabled = glfogNum;
      if ( fogEnabled )
      {
        glEnable(0xB60u);
        glState_glStateBits |= 0x200000u;
      }
    }
  }
  glVertexPointer(3, 0x1406u, 0, tess_xyz);
}

/* ---- RE_UploadCinematic  0x004D7290 ----  VERIFIED
 * Upload one cinematic frame into a scratch image (w/h unused). */
void __cdecl RE_UploadCinematic(int w, int h, GLsizei width, GLsizei height, GLvoid *pixels, int scratchIndex, int dirty)
{
  GL_Bind((image_t *)tr_scratchImages[scratchIndex]);
  if ( width == ((image_t *)tr_scratchImages[scratchIndex])->width
    && height == ((image_t *)tr_scratchImages[scratchIndex])->height )
  {
    if ( dirty )
      glTexSubImage2D(0xDE1u, 0, 0, 0, width, height, 0x1908u, 0x1401u, pixels);
  }
  else
  {
    ((image_t *)tr_scratchImages[scratchIndex])->width  = ((image_t *)tr_scratchImages[scratchIndex])->uploadWidth  = width;
    ((image_t *)tr_scratchImages[scratchIndex])->height = ((image_t *)tr_scratchImages[scratchIndex])->uploadHeight = height;
    glTexImage2D(0xDE1u, 0, 3, width, height, 0, 0x1908u, 0x1401u, pixels);
    glTexParameteri(0xDE1u, 0x2801u, 9729);        /* GL_TEXTURE_MIN_FILTER, GL_LINEAR */
    glTexParameteri(0xDE1u, 0x2800u, 9729);        /* GL_TEXTURE_MAG_FILTER, GL_LINEAR */
    glTexParameteri(0xDE1u, 0x2802u, 33071);       /* GL_TEXTURE_WRAP_S, GL_CLAMP */
    glTexParameteri(0xDE1u, 0x2803u, 33071);       /* GL_TEXTURE_WRAP_T, GL_CLAMP */
  }
}

/* ---- RB_SetColor  0x004D7390 ----  VERIFIED */
int __cdecl RB_SetColor(setColorCommand_t *cmd)
{
  backEnd_color2D = *(int *)cmd->color;
  return (int)(cmd + 1);
}

/* ---- RB_DrawStretchPic  0x004D73A0 ----  VERIFIED
 * Emit one screen-space quad (2 triangles) into the tess buffers; color2D is
 * the packed RGBA.  Retail passes shader in ebx (LTCG); the dead EAX at return
 * is the s1 bits.  Retail calls RB_CheckOverflow here (the tree inlines it). */
void __cdecl RB_DrawStretchPic(
        float x,
        float y,
        float w,
        float h,
        float s1,
        float t1,
        float s2,
        float t2,
        int shader,
        int *color2D)
{
  unsigned short numVerts;
  int            numIndexes;
  float         *xyz;
  float         *st;
  int            storage;

  if ( !backEnd_projection2D )
    RB_SetGL2D();

  if ( shader != tess_shader )
  {
    if ( tess_numIndexes )
      RB_EndSurface();
    backEnd_currentEntity = (int)&backEnd_entity2D;
    storage = storageClass;
    if ( storageClass != glState_currentStorageMode )
    {
      if ( glConfig_NVVertexArrayRange )
        RB_SelectStorageNV(storageClass);
      else if ( glConfig_ATIVertexArrayObject )
        RB_SelectStorageATI(storageClass);
      glState_currentStorageMode = storage;
    }
    RB_BeginSurface((void *)shader, 3);   /* 0x004D740B: push 3 / mov ecx,ebx */
  }
  if ( tess_numVertexes + 4 >= 0x2000 || tess_numIndexes + 6 >= 49152 )
#pragma inline_depth(0)
    RB_CheckOverflow(4, 6);
#pragma inline_depth()

  numVerts   = tess_numVertexes;
  numIndexes = tess_numIndexes;

  tess_numVertexes += 4;
  tess_numIndexes  += 6;

  tess_indexes[numIndexes + 0] = numVerts + 3;
  tess_indexes[numIndexes + 1] = numVerts;
  tess_indexes[numIndexes + 2] = numVerts + 2;
  tess_indexes[numIndexes + 3] = numVerts + 2;
  tess_indexes[numIndexes + 4] = numVerts;
  tess_indexes[numIndexes + 5] = numVerts + 1;

  tess_vertexColorsPacked[numVerts + 0] =
    tess_vertexColorsPacked[numVerts + 1] =
    tess_vertexColorsPacked[numVerts + 2] =
    tess_vertexColorsPacked[numVerts + 3] = *color2D;

  xyz = &tess_xyz[numVerts * tess_vertexComponentCount];
  st  = &tess_texCoords0[2 * numVerts];

  *xyz++ = x;      *xyz++ = y;      *xyz++ = 0.0f;
  *st++ = s1;      *st++ = t1;
  *xyz++ = x + w;  *xyz++ = y;      *xyz++ = 0.0f;
  *st++ = s2;      *st++ = t1;
  *xyz++ = x + w;  *xyz++ = y + h;  *xyz++ = 0.0f;
  *st++ = s2;      *st++ = t2;
  *xyz++ = x;      *xyz++ = y + h;  *xyz++ = 0.0f;
  *st++ = s1;      *st++ = t2;
}

/* ---- RB_StretchPic  0x004D75A0 ----  VERIFIED */
int __cdecl RB_StretchPic(int data)
{
  stretchPicCommand_t *cmd = (stretchPicCommand_t *)data;

  RB_DrawStretchPic(
    cmd->x,
    cmd->y,
    cmd->w,
    cmd->h,
    cmd->s1,
    cmd->t1,
    cmd->s2,
    cmd->t2,
    (int)cmd->shader,
    &backEnd_color2D);
  return (int)(cmd + 1);
}

/* ---- RB_StretchPicGradient  0x004D75E0 ----  VERIFIED
 * Like RB_StretchPic, but the bottom two vertices take the gradient color. */
int __cdecl RB_StretchPicGradient(int data)
{
  stretchPicGradientCommand_t *cmd = (stretchPicGradientCommand_t *)data;
  shader_t      *shader;
  unsigned short numVerts;
  int            numIndexes;
  float         *xyz;
  float         *st;
  int            storage;

  if ( !backEnd_projection2D )
    RB_SetGL2D();

  shader = cmd->shader;
  if ( (int)shader != tess_shader )
  {
    if ( tess_numIndexes )
      RB_EndSurface();
    backEnd_currentEntity = (int)&backEnd_entity2D;
    storage = storageClass;
    if ( storageClass != glState_currentStorageMode )
    {
      if ( glConfig_NVVertexArrayRange )
        RB_SelectStorageNV(storageClass);
      else if ( glConfig_ATIVertexArrayObject )
        RB_SelectStorageATI(storageClass);
      glState_currentStorageMode = storage;
    }
    RB_BeginSurface(shader, 3);   /* 0x004D7650: push 3 / mov ecx,ebp; ebp = [edi+4] */
  }
  if ( tess_numVertexes + 4 >= 0x2000 || tess_numIndexes + 6 >= 49152 )
#pragma inline_depth(0)
    RB_CheckOverflow(4, 6);
#pragma inline_depth()

  numVerts   = tess_numVertexes;
  numIndexes = tess_numIndexes;

  tess_numVertexes += 4;
  tess_numIndexes  += 6;

  tess_indexes[numIndexes + 0] = numVerts + 3;
  tess_indexes[numIndexes + 1] = numVerts;
  tess_indexes[numIndexes + 2] = numVerts + 2;
  tess_indexes[numIndexes + 3] = numVerts + 2;
  tess_indexes[numIndexes + 4] = numVerts;
  tess_indexes[numIndexes + 5] = numVerts + 1;

  tess_vertexColorsPacked[numVerts + 0] =
    tess_vertexColorsPacked[numVerts + 1] = backEnd_color2D;
  tess_vertexColorsPacked[numVerts + 2] =
    tess_vertexColorsPacked[numVerts + 3] = *(int *)cmd->gradientColor;

  xyz = &tess_xyz[numVerts * tess_vertexComponentCount];
  st  = &tess_texCoords0[2 * numVerts];

  *xyz++ = cmd->x;           *xyz++ = cmd->y;           *xyz++ = 0.0f;
  *st++ = cmd->s1;           *st++ = cmd->t1;
  *xyz++ = cmd->x + cmd->w;  *xyz++ = cmd->y;           *xyz++ = 0.0f;
  *st++ = cmd->s2;           *st++ = cmd->t1;
  *xyz++ = cmd->x + cmd->w;  *xyz++ = cmd->y + cmd->h;  *xyz++ = 0.0f;
  *st++ = cmd->s2;           *st++ = cmd->t2;
  *xyz++ = cmd->x;           *xyz++ = cmd->y + cmd->h;  *xyz++ = 0.0f;
  *st++ = cmd->s1;           *st++ = cmd->t2;

  return (int)(cmd + 1);
}

/* Retail's inlined fsincos block saves eax/ecx around itself (com_math.c's
 * SinCos is /Op and has no push/pop). */
static __inline void RB_SinCos( float radians, float *sinOut, float *cosOut )
{
  __asm {
    push    eax
    push    ecx
    fld     radians
    fsincos
    mov     ecx, cosOut
    mov     eax, sinOut
    fstp    dword ptr [ecx]
    fstp    dword ptr [eax]
    pop     ecx
    pop     eax
  }
}

/* ---- RB_StretchPicRotate  0x004D77E0 ----  VERIFIED
 * Emit a quad rotated about its center by cmd->angle (degrees). */
int __cdecl RB_StretchPicRotate(int data)
{
  stretchPicRotateCommand_t *cmd = (stretchPicRotateCommand_t *)data;
  shader_t      *shader;
  unsigned short numVerts;
  int            numIndexes;
  float         *xyz;
  int            storage;
  struct { float halfW, halfH, centerX, centerY; } quad;
  float          radians, s, c;
  rfloat         cw, sw;              /* x87 stack values in retail, never rounded */
  float          negSH, ch;

  if ( !backEnd_projection2D )
    RB_SetGL2D();

  shader = cmd->shader;
  if ( (int)shader != tess_shader )
  {
    if ( tess_numIndexes )
      RB_EndSurface();
    backEnd_currentEntity = (int)&backEnd_entity2D;
    storage = storageClass;
    if ( storageClass != glState_currentStorageMode )
    {
      if ( glConfig_NVVertexArrayRange )
        RB_SelectStorageNV(storageClass);
      else if ( glConfig_ATIVertexArrayObject )
        RB_SelectStorageATI(storageClass);
      glState_currentStorageMode = storage;
    }
    RB_BeginSurface(shader, 3);   /* 0x004D7852: push 3 / mov ecx,ebp; ebp = [edi+4] */
  }
  if ( tess_numVertexes + 4 >= 0x2000 || tess_numIndexes + 6 >= 49152 )
#pragma inline_depth(0)
    RB_CheckOverflow(4, 6);
#pragma inline_depth()

  numVerts   = tess_numVertexes;
  numIndexes = tess_numIndexes;

  tess_numVertexes += 4;
  tess_numIndexes  += 6;

  tess_indexes[numIndexes + 0] = numVerts + 3;
  tess_indexes[numIndexes + 1] = numVerts;
  tess_indexes[numIndexes + 2] = numVerts + 2;
  tess_indexes[numIndexes + 3] = numVerts + 2;
  tess_indexes[numIndexes + 4] = numVerts;
  tess_indexes[numIndexes + 5] = numVerts + 1;

  tess_vertexColorsPacked[numVerts + 0] =
    tess_vertexColorsPacked[numVerts + 1] =
    tess_vertexColorsPacked[numVerts + 2] =
    tess_vertexColorsPacked[numVerts + 3] = backEnd_color2D;

  tess_texCoords0[2 * numVerts + 0] = cmd->s1;
  tess_texCoords0[2 * numVerts + 1] = cmd->t1;
  tess_texCoords0[2 * numVerts + 2] = cmd->s2;
  tess_texCoords0[2 * numVerts + 3] = cmd->t1;
  tess_texCoords0[2 * numVerts + 4] = cmd->s2;
  tess_texCoords0[2 * numVerts + 5] = cmd->t2;
  tess_texCoords0[2 * numVerts + 6] = cmd->s1;
  tess_texCoords0[2 * numVerts + 7] = cmd->t2;

  quad.halfW   = cmd->w * 0.5f;
  quad.halfH   = cmd->h * 0.5f;
  quad.centerX = quad.halfW + cmd->x;
  quad.centerY = quad.halfH + cmd->y;
  radians = FCAST((rfloat)cmd->angle * 3.1415927f) * 0.0055555557f;   /* deg->rad (PI/180), two fmuls */
  RB_SinCos(radians, &s, &c);

  cw    = (rfloat)c * quad.halfW;
  sw    = (rfloat)s * quad.halfW;
  negSH = -(s * quad.halfH);
  ch    = c * quad.halfH;

  xyz = &tess_xyz[numVerts * tess_vertexComponentCount];
#ifdef COD1_VC7
  /* VC7 CSEs quad.centerX - cw / quad.centerY - sw and spills them as floats for vertex 3 */
  *xyz++ = quad.centerX - cw - negSH;
  *xyz++ = quad.centerY - sw - ch;
  *xyz++ = 0.0f;
  *xyz++ = quad.centerX + cw - negSH;
  *xyz++ = quad.centerY + sw - ch;
  *xyz++ = 0.0f;
  *xyz++ = quad.centerX + cw + negSH;
  *xyz++ = quad.centerY + sw + ch;
  *xyz++ = 0.0f;
  *xyz++ = quad.centerX - cw + negSH;
  *xyz++ = quad.centerY - sw + ch;
  *xyz++ = 0.0f;
#else
  {
    double x0d = quad.centerX - cw, y0d = quad.centerY - sw;
    float  x0 = (float)x0d, y0 = (float)y0d;

    *xyz++ = (float)(x0d - negSH);
    *xyz++ = (float)(y0d - ch);
    *xyz++ = 0.0f;
    *xyz++ = (float)(quad.centerX + cw - negSH);
    *xyz++ = (float)(quad.centerY + sw - ch);
    *xyz++ = 0.0f;
    *xyz++ = (float)(quad.centerX + cw + negSH);
    *xyz++ = (float)(quad.centerY + sw + ch);
    *xyz++ = 0.0f;
    *xyz++ = x0 + negSH;
    *xyz++ = y0 + ch;
    *xyz++ = 0.0f;
  }
#endif

  return (int)(cmd + 1);
}

/* ---- RB_DrawQuadPic  0x004D7AB0 ----  VERIFIED
 * Emit a quad from four explicit positions and texcoords.
 * The dead EAX return is the next command pointer. */
unsigned int *__cdecl RB_DrawQuadPic(unsigned int *data)
{
  drawQuadPicCommand_t *cmd = (drawQuadPicCommand_t *)data;
  shader_t      *shader;
  unsigned short numVerts;
  int            numIndexes;
  float         *xyz;
  int            storage;

  if ( !backEnd_projection2D )
    RB_SetGL2D();

  shader = cmd->shader;
  if ( (int)shader != tess_shader )
  {
    if ( tess_numIndexes )
      RB_EndSurface();
    backEnd_currentEntity = (int)&backEnd_entity2D;
    storage = storageClass;
    if ( storageClass != glState_currentStorageMode )
    {
      if ( glConfig_NVVertexArrayRange )
        RB_SelectStorageNV(storageClass);
      else if ( glConfig_ATIVertexArrayObject )
        RB_SelectStorageATI(storageClass);
      glState_currentStorageMode = storage;
    }
    RB_BeginSurface(shader, 3);   /* 0x004D7B20: push 3 / mov ecx,ebp; ebp = [edi+4] */
  }
  if ( tess_numVertexes + 4 >= 0x2000 || tess_numIndexes + 6 >= 49152 )
#pragma inline_depth(0)
    RB_CheckOverflow(4, 6);
#pragma inline_depth()

  numVerts   = tess_numVertexes;
  numIndexes = tess_numIndexes;

  tess_numVertexes += 4;
  tess_numIndexes  += 6;

  tess_indexes[numIndexes + 0] = numVerts + 3;
  tess_indexes[numIndexes + 1] = numVerts;
  tess_indexes[numIndexes + 2] = numVerts + 2;
  tess_indexes[numIndexes + 3] = numVerts + 2;
  tess_indexes[numIndexes + 4] = numVerts;
  tess_indexes[numIndexes + 5] = numVerts + 1;

  tess_vertexColorsPacked[numVerts + 0] =
    tess_vertexColorsPacked[numVerts + 1] =
    tess_vertexColorsPacked[numVerts + 2] =
    tess_vertexColorsPacked[numVerts + 3] = backEnd_color2D;

  tess_texCoords0[2 * numVerts + 0] = cmd->texCoords[0][0];
  tess_texCoords0[2 * numVerts + 1] = cmd->texCoords[0][1];
  tess_texCoords0[2 * numVerts + 2] = cmd->texCoords[1][0];
  tess_texCoords0[2 * numVerts + 3] = cmd->texCoords[1][1];
  tess_texCoords0[2 * numVerts + 4] = cmd->texCoords[2][0];
  tess_texCoords0[2 * numVerts + 5] = cmd->texCoords[2][1];
  tess_texCoords0[2 * numVerts + 6] = cmd->texCoords[3][0];
  tess_texCoords0[2 * numVerts + 7] = cmd->texCoords[3][1];

  xyz = &tess_xyz[numVerts * tess_vertexComponentCount];
  *xyz++ = cmd->positions[0][0];  *xyz++ = cmd->positions[0][1];  *xyz++ = 0.0f;
  *xyz++ = cmd->positions[1][0];  *xyz++ = cmd->positions[1][1];  *xyz++ = 0.0f;
  *xyz++ = cmd->positions[2][0];  *xyz++ = cmd->positions[2][1];  *xyz++ = 0.0f;
  *xyz++ = cmd->positions[3][0];  *xyz++ = cmd->positions[3][1];  *xyz++ = 0.0f;

  return data + 18;
}

/* R_GetShaderByHandle (tr_shader.c), which retail inlines into its callers. */
static __inline char *R_GetShaderByHandle( int hShader )
{
  if ( hShader < 0 )
  {
    ri.Printf(2, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
    return tr_defaultShader;
  }
  if ( hShader >= tr_numShaders[0] )
  {
    ri.Printf(2, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
    return tr_defaultShader;
  }
  return (char *)tr_shaders[hShader];
}

/* ---- RB_Text_PaintChar  0x004D7CB0 ----  VERIFIED
 * w/h land in the dead hShader/scale argument slots (VC7 slot reuse). */
void __cdecl RB_Text_PaintChar(
        float x,
        int y,           /* float bits */
        float width,
        float height,
        int font,          /* font handle; unused here */
        float scale,
        int s1,
        int t1,
        int s2,
        int t2,
        int hShader,
        int *color2D)
{
  float w;
  float h;
  char *shader;

  w = width * scale;
  h = height * scale;
  ri.SCR_AdjustFrom640(&x, &y, &w, &h);
  shader = R_GetShaderByHandle(hShader);
  RB_DrawStretchPic(x, *(float *)&y, w, h, *(float *)&s1, *(float *)&t1, *(float *)&s2, *(float *)&t2,
                    (int)shader, color2D);
}

static const unsigned int s_consoleColorTable[8] =
{
	0xFF000000u, 0xFF0000FFu, 0xFF00FF00u, 0xFF00FFFFu,
	0xFFFF0000u, 0xFFFFFF00u, 0xFFFF00FFu, 0xFFFFFFFFu
};

/* ---- RB_Text_PaintWithCursor  0x004D7D60 ----  VERIFIED */
int __cdecl RB_Text_PaintWithCursor(
        float penX,
        float penY,
        int font,
        float scale,
        int colorPtr,
        const char *text,
        int cursorPos,
        unsigned __int8 cursorChar,
        float fixedWidth,
        int style)
{
  int fontInfo;
  int baseColor;
  int alpha;
  const char *str;
  signed int len;
  unsigned __int8 ch;
  char trailByteA;
  unsigned int charCode;
  unsigned __int8 trailByte9;
  unsigned int leadByte9;
  unsigned __int8 trailByte8;
  char colorCode;
  char savedAlpha;
  int *glyphPtr;
  int *glyph;
  int shadowS2;
  int shadowT1;
  float shadowOffset;
  int glyphHandle;  /* glyph shader HANDLE (retail: mov esi, [esp+arg_28]) */
  int mainT2;
  int mainS2;
  float glyphX;
  int cursorOffMid;
  int cursorHandleMid;  /* glyph shader HANDLE */
  int cursorGlyphMid;
  float nextX;
  int cursorOffEnd;
  int cursorHandleEnd;  /* glyph shader HANDLE */
  int cursorGlyphEnd;
  float shadowX;
  int shadowY;
  int mainY;
  int cursorYMid;
  float cursorXEnd;
  float shadowW;
  float mainW;
  float cursorWMid;
  int cursorYEnd;
  float shadowH;
  float mainH;
  float cursorHMid;
  float cursorWEnd;
  float cursorHEnd;
  int glyphHandleShadow;  /* glyph shader HANDLE */
  float glyphScale;
  int color; // [esp+44h] [ebp-2Ch] BYREF
  float xOffset;
  float baseScale;
  float yOffset;
  signed int charIndex;
  float AsianScale;
  float lineStartX;
  float asianYAdjust;
  int shadowColor; // [esp+64h] [ebp-Ch] BYREF
  signed int savedLen;
  int scratch;

  fontInfo = ri.GetFontInfo(font, *(unsigned int *)&scale);
  baseScale = scale * *(float *)(fontInfo + 20480);
  AsianScale = 1.0;
  asianYAdjust = 0.0;
  if ( g_currentAsian )
  {
    AsianScale = (float)R_GetAsianScale(fontInfo, scale);
    asianYAdjust = ((float)g_asianGlyph.height * AsianScale * scale - scale * *(float *)(fontInfo + 20484)) * 0.125f;
  }
  baseColor = *(unsigned int *)colorPtr;
  alpha=(unsigned int)baseColor >> 24;
  shadowColor = 0;
  ((byte *)&shadowColor)[3] = alpha;
  str = text;
  color = baseColor;
  len = strlen(text);
  savedLen = len;
  lineStartX = penX;
  xOffset = 0.0;
  charIndex = 0;
  while ( str )
  {
    ch = *str;
    if ( !*str || charIndex >= len )
      break;
    if ( g_currentAsian )
    {
      switch ( cl_language->integer )
      {
        case 8:
          trailByte8 = str[1];
          if ( ch >= 0xB0u && ch <= 0xC8u && trailByte8 > 0xA0u && trailByte8 != 0xFF )
          {
            charCode = trailByte8 + (ch << 8);
            str += 2;
            goto LABEL_33;
          }
          break;
        case 9:
          trailByte9 = str[1];
          leadByte9 = (unsigned int)(trailByte9 + (ch << 8)) >> 8;
          if ( ((unsigned __int8)leadByte9 >= 0xA1u && (unsigned __int8)leadByte9 <= 0xC6u
             || (unsigned __int8)leadByte9 >= 0xC9u && (unsigned __int8)leadByte9 <= 0xF9u)
            && (trailByte9 >= 0x40u && trailByte9 <= 0x7Eu || trailByte9 >= 0xA1u && trailByte9 != 0xFF) )
          {
            charCode = *((unsigned __int8 *)str + 1) + (ch << 8);
            str += 2;
            goto LABEL_33;
          }
          break;
        case 0xA:
          if ( (trailByteA = str[1], ch >= 0x81u) && ch <= 0x9Fu || ch >= 0xE0u && ch <= 0xEFu )
          {
            if ( (unsigned __int8)trailByteA >= 0x40u && (unsigned __int8)trailByteA <= 0x7Eu || trailByteA <= -4 )
            {
              charCode = (unsigned __int8)trailByteA + (ch << 8);
              str += 2;
              goto LABEL_33;
            }
          }
          break;
      }
    }
    charCode = ch;
    ++str;
LABEL_33:
    switch ( charCode )
    {
      case 0x5Eu:
        if ( !str )
          goto LABEL_47;
        colorCode = *str;
        if ( *str == 94 || (unsigned __int8)colorCode < 0x30u || (unsigned __int8)colorCode > 0x37u )
          goto LABEL_47;
        if ( (colorCode & 7) == 7 )
        {
          color = *(unsigned int *)colorPtr;
        }
        else
        {
          savedAlpha = *(byte *)(colorPtr + 3);
          color = s_consoleColorTable[colorCode & 7];
          ((byte *)&color)[3] = savedAlpha;
        }
        ++str;
        break;
      case 0xAu:
        penX = lineStartX;
        penY = scale * *(float *)(fontInfo + 20484) + penY;
        if ( g_currentAsian )
          penY = penY + 4.0f;
        break;
      case 0xDu:
        penX = lineStartX;
        break;
      default:
LABEL_47:
        glyphPtr = R_GetCharacterGlyph(charCode, fontInfo);
        glyph = glyphPtr;
        glyphScale = baseScale;
        if ( charCode > 0xFF )
          glyphScale = AsianScale * baseScale;
        yOffset = glyphScale * *((float *)glyphPtr + 2);
        xOffset = glyphScale * *((float *)glyphPtr + 3);
        if ( charCode > 0xFF )
          yOffset = yOffset + asianYAdjust;
        if ( fixedWidth != 0.0f )
          xOffset = (fixedWidth - glyphScale * *((float *)glyphPtr + 4)) * 0.5f + xOffset;
        if ( style == 3 || style == 6 )
        {
          shadowS2 = glyphPtr[9];
          glyphHandleShadow = glyph[11];
          shadowT1 = glyph[8];
          scratch = (style != 3) + 1;
          shadowOffset = (double)scratch;
          shadowH = (float)glyph[6];
          shadowW = (float)glyph[5];
          *(float *)&shadowY = penY - yOffset + shadowOffset;
          shadowX = shadowOffset + xOffset + penX;
          RB_Text_PaintChar(shadowX, shadowY, shadowW, shadowH, font, glyphScale, glyph[7], shadowT1, shadowS2, glyph[10], glyphHandleShadow, &shadowColor);
        }
        glyphHandle = glyph[11];
        mainT2 = glyph[10];
        mainS2 = glyph[9];
        *(float *)&scratch = xOffset + penX;
        glyphX = *(float *)&scratch;
        mainH = (float)glyph[6];
        mainW = (float)glyph[5];
        *(float *)&mainY = penY - yOffset;
        RB_Text_PaintChar(*(float *)&scratch, mainY, mainW, mainH, font, glyphScale, glyph[7], glyph[8], mainS2, mainT2, glyphHandle, &color);
        if ( charIndex == cursorPos )
        {
          if ( !sys_timeBaseInit )
          {
            sys_timeBase = timeGetTime();
            sys_timeBaseInit = 1;
          }
          if ( (((int)(timeGetTime() - sys_timeBase) / 256) & 1) != 0 )
          {
            cursorOffMid = 80 * cursorChar;
            cursorHandleMid = *(int *)(cursorOffMid + fontInfo + 44);
            cursorGlyphMid = fontInfo + cursorOffMid;
            cursorHMid = (float)*(int *)(cursorGlyphMid + 24);
            cursorWMid = (float)*(int *)(cursorGlyphMid + 20);
            *(float *)&cursorYMid = penY - glyphScale * *(float *)(cursorGlyphMid + 8);
            RB_Text_PaintChar(
              glyphX,
              cursorYMid,
              cursorWMid,
              cursorHMid,
              font,
              baseScale,
              *(unsigned int *)(cursorGlyphMid + 28),
              *(unsigned int *)(cursorGlyphMid + 32),
              *(unsigned int *)(cursorGlyphMid + 36),
              *(unsigned int *)(cursorGlyphMid + 40),
              cursorHandleMid,
              &color);
          }
        }
        if ( fixedWidth == 0.0f )
          nextX = glyphScale * *((float *)glyph + 4) + penX;
        else
          nextX = penX + fixedWidth;
        penX = nextX;
        ++charIndex;
        break;
    }
    len = savedLen;
  }
  if ( cursorPos == len )
  {
    if ( !sys_timeBaseInit )
    {
      sys_timeBase = timeGetTime();
      sys_timeBaseInit = 1;
    }
    if ( (((int)(timeGetTime() - sys_timeBase) / 256) & 1) != 0 )
    {
      cursorOffEnd = 80 * cursorChar;
      cursorHandleEnd = *(int *)(cursorOffEnd + fontInfo + 44);
      cursorGlyphEnd = fontInfo + cursorOffEnd;
      cursorHEnd = (float)*(int *)(cursorGlyphEnd + 24);
      cursorWEnd = (float)*(int *)(cursorGlyphEnd + 20);
      *(float *)&cursorYEnd = penY - baseScale * *(float *)(cursorGlyphEnd + 8);
      cursorXEnd = xOffset + penX;
      RB_Text_PaintChar(
        cursorXEnd,
        cursorYEnd,
        cursorWEnd,
        cursorHEnd,
        font,
        baseScale,
        *(unsigned int *)(cursorGlyphEnd + 28),
        *(unsigned int *)(cursorGlyphEnd + 32),
        *(unsigned int *)(cursorGlyphEnd + 36),
        *(unsigned int *)(cursorGlyphEnd + 40),
        cursorHandleEnd,
        &color);
    }
  }
  RE_SetColor(0);
  return len;
}

/* ---- RB_Text_Paint  0x004D82B0 ----  VERIFIED */
int __cdecl RB_Text_Paint(int cmd)
{
  return cmd
       + ((RB_Text_PaintWithCursor(
             *(float *)(cmd + 4),
             *(float *)(cmd + 8),
             *(unsigned int *)(cmd + 12),
             *(float *)(cmd + 16),
             cmd + 20,
             (const char *)(cmd + 37),
             *(unsigned int *)(cmd + 32),
             *(byte *)(cmd + 36),
             *(float *)(cmd + 24),
             *(unsigned int *)(cmd + 28))
         + 41)
        & 0xFFFFFFFC);
}

/* ---- RB_DrawSurfs  0x004D82F0 ----  VERIFIED */
int *__cdecl RB_DrawSurfs(int *data)
{
  drawSurfsCommand_t *cmd = (drawSurfsCommand_t *)data;

  if ( tess_numIndexes )
    RB_EndSurface();
  memcpy(BACKEND, cmd->refdef, 0x188u);                    /* backEnd.refdef   = cmd->refdef   */
  memcpy(backEnd_viewParms_originX, &cmd->viewParms, 0x260u);  /* backEnd.viewParms = cmd->viewParms */
  RB_RenderDrawSurfList(cmd->numDrawSurfs, (int)cmd->drawSurfs);
  return (int *)(cmd + 1);
}

/* ---- RB_DrawBuffer  0x004D8350 ----  VERIFIED */
int __cdecl RB_DrawBuffer(int data)
{
  drawBufferCommand_t *cmd = (drawBufferCommand_t *)data;
  GLclampf blue;

  glDrawBuffer(cmd->buffer);
  if ( r_clear->integer )
  {
    blue = tr_identityLight * 0.5f;
    glClearColor(tr_identityLight, 0.0, blue, 1.0);
    glClear(0x4100u);                                       /* GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT */
  }
  return (int)(cmd + 1);
}

/* ---- RB_ShowImages  0x004D83A0 ----  [HIGH]
 * Draw every loaded image as a tile over the screen (texture-thrash test).
 * Each image is bound onto the show-images shader's first stage, then a single
 * quad is emitted directly into the (empty) tess buffers. */
void RB_ShowImages()
{
  int    i;
  rfloat w, h, y;   /* kept on the x87 stack in retail, never rounded */
  float  x;         /* the only fstp'd term (var_C) */
  int    start;
  int    end;

  if ( !backEnd_projection2D )
    RB_SetGL2D();
  glClear(0x4000u);
  qglFinish();
  start = ri.Milliseconds();
  GL_Bind((image_t *)tr_screenImage);
  glCopyTexSubImage2D(0xDE1u, 0, 0, 0, 0, 0, glConfig_vidWidth, glConfig_vidHeight);

  for ( i = 0; i < tr_numImages; ++i )
  {
    if ( ((image_t *)tr_imageSlots[i])->TMU )   /* only images not bound to a fixed TMU */
      continue;

    /* point the show-images shader's first stage at this image */
    *(int *)(*(int *)(tr_showImagesShader + 340) + 4) = tr_imageSlots[i];   /* stage[0] image */
    RB_BeginSurface((void *)tr_showImagesShader, 3);

    w = (rfloat)glConfig_vidWidth  * 0.03125f;       /* / 32 */
    h = (rfloat)glConfig_vidHeight * 0.041666668f;   /* / 24: float 1/24 = 0x3D2AAAAB (flt_569094) */
    x = i % 32 * w;
    y = i / 32 * h;
    if ( r_showImages->integer == 2 )                /* proportional to upload size */
    {
      w = w * ((image_t *)tr_imageSlots[i])->uploadWidth  * 0.0009765625f;   /* / 1024 */
      h = h * ((image_t *)tr_imageSlots[i])->uploadHeight * 0.0009765625f;
    }

    tess_texCoords0[0] = 0.0f;  tess_texCoords0[1] = 0.0f;
    tess_xyz[0]  = x;           tess_xyz[1]  = y;      tess_xyz[2]  = 0.0f;
    tess_texCoords0[2] = 1.0f;  tess_texCoords0[3] = 0.0f;
    tess_xyz[3]  = x + w;       tess_xyz[4]  = y;      tess_xyz[5]  = 0.0f;
    tess_texCoords0[4] = 1.0f;  tess_texCoords0[5] = 1.0f;
    tess_xyz[6]  = x + w;       tess_xyz[7]  = y + h;  tess_xyz[8]  = 0.0f;
    tess_texCoords0[6] = 0.0f;  tess_texCoords0[7] = 1.0f;
    tess_xyz[9]  = x;           tess_xyz[10] = y + h;  tess_xyz[11] = 0.0f;

    tess_indexes[0] = 0;  tess_indexes[1] = 1;  tess_indexes[2] = 3;
    tess_indexes[3] = 3;  tess_indexes[4] = 1;  tess_indexes[5] = 2;
    tess_numIndexes  = 6;
    tess_numVertexes = 4;
    RB_EndSurface();
  }

  qglFinish();
  end = ri.Milliseconds();
  ri.Printf(0, "%i msec to draw all images\n", end - start);
}

/* ---- RB_SaveScreen  0x004D8620 ----  [HIGH]
 * Copy the framebuffer into tr.screenImage and record its size ratios
 * (SMax/TMax hold float bit patterns in int slots). */
int __cdecl RB_SaveScreen(int data)
{
  saveScreenCommand_t *cmd = (saveScreenCommand_t *)data;

  if ( !backEnd_projection2D )
    RB_SetGL2D();
  if ( tess_numVertexes )
    RB_EndSurface();
  RB_EndMultitexture();
  GL_Bind((image_t *)tr_screenImage);
  glCopyTexSubImage2D(0xDE1u, 0, 0, 0, 0, 0, glConfig_vidWidth, glConfig_vidHeight);
  tr_screenImageSaveTime = backEnd_refdef_time;
  *(float *)&tr_screenImageSMax = (double)(int)glConfig_vidWidth  / (double)tr_screenImageWidth;
  *(float *)&tr_screenImageTMax = (double)(int)glConfig_vidHeight / (double)tr_screenImageHeight;
  return (int)(cmd + 1);
}

/* ---- RB_BlendSavedScreen  0x004D86B0 ----  VERIFIED
 * Fade the saved screen back in over cmd->duration ms as a full-screen quad
 * (white with a decaying alpha). */
int __cdecl RB_BlendSavedScreen(int data)
{
  blendSavedScreenCommand_t *cmd = (blendSavedScreenCommand_t *)data;
  int    elapsed;
  int    duration;
  rfloat frac;
  float  scaledAlpha;
  byte   color[4];                     /* white, decaying alpha */

  if ( !backEnd_projection2D )
    RB_SetGL2D();

  elapsed  = backEnd_refdef_time - tr_screenImageSaveTime;
  duration = cmd->duration;
  if ( elapsed < duration )
  {
    frac = pow(0.009999999776482582, (double)elapsed / (double)duration);
    if ( frac > 0.99000001f )                /* float clamp flt_569020 = 0x3F7D70A4 */
      frac = 0.99000001f;
    color[0] = 255;
    color[1] = 255;
    color[2] = 255;
    scaledAlpha = frac * 255.0f;
    /* FastRound (cl_console_mp.c) inlined at 0x004D8722-0x004D873A: fld float / fadd
     * qword 2^-30 / fistp -- round-to-nearest, not the truncating __ftol2 of an (int)
     * cast.  Spelled as the call (not a local asm copy) so this frame keeps retail's
     * 8-byte-aligned layout. */
    color[3] = FastRound( scaledAlpha );
    RB_DrawStretchPic(0.0, 0.0, (float)glConfig_vidWidth, (float)glConfig_vidHeight,
                      0.0f, tr_screenImageTMax, tr_screenImageSMax, 0.0f, tr_screenImageShader, (int *)color);
  }
  return (int)(cmd + 1);
}

static const float s_overdrawColors[14][3] =
{
	{ 0.00f, 0.00f, 0.00f },
	{ 0.00f, 1.00f, 0.00f },
	{ 0.00f, 1.00f, 1.00f },
	{ 0.00f, 0.00f, 1.00f },
	{ 1.00f, 1.00f, 0.00f },
	{ 1.00f, 0.70f, 0.00f },
	{ 1.00f, 0.00f, 0.00f },
	{ 1.00f, 0.30f, 0.30f },
	{ 1.00f, 0.50f, 0.50f },
	{ 1.00f, 0.00f, 0.70f },
	{ 1.00f, 0.00f, 1.00f },
	{ 0.52f, 0.41f, 0.30f },
	{ 1.00f, 1.00f, 1.00f },
	{ 0.50f, 0.50f, 0.50f }
};
static const GLint s_overdrawStencilRefs[14] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 20, 40, 80, 200 };

/* ---- RB_SwapBuffers  0x004D8790 ----  VERIFIED
 * Flush debug geometry, optionally measure/colorize stencil overdraw, then end
 * the frame. */
int __cdecl RB_SwapBuffers(int data)
{
  swapBuffersCommand_t *cmd = (swapBuffersCommand_t *)data;
  int            i;
  long           sum;
  unsigned char *stencilReadback;

  if ( tess_numIndexes )
    RB_EndSurface();
  if ( cls_rendererStarted )
  {
    if ( cls_debugStrings )
      re_LocateDebugStrings(cls_debugStrings, cls_debugStringCount);
    if ( cls_debugLines )
      re_LocateDebugLines(cls_debugLines, cls_debugLineCount);
  }
  RB_DrawDebug();
  CL_FlushDebugData( 0 );
  if ( r_showImages->integer )
    RB_ShowImages();

  if ( r_measureOverdraw->integer )
  {
    sum = 0;

    /* sum the stencil buffer to count overdraw */
    if ( r_measureOverdraw->integer >= 2 )
    {
      stencilReadback = (unsigned char *)ri.Hunk_AllocateTempMemory(glConfig_vidWidth * glConfig_vidHeight);
      glReadPixels(0, 0, glConfig_vidWidth, glConfig_vidHeight, 0x1901u, 0x1401u, stencilReadback);   /* GL_STENCIL_INDEX, GL_UNSIGNED_BYTE */
      for ( i = 0; i < glConfig_vidWidth * glConfig_vidHeight; i++ )
        sum += stencilReadback[i];
      backEnd_pc_overdrawSum = (rfloat)sum + backEnd_pc_overdrawSum;
      ri.Hunk_FreeTempMemory(stencilReadback);
    }

    /* colorize overdraw levels as stacked full-screen quads */
    if ( r_measureOverdraw->integer != 2 )
    {
      RB_SetGL2D();
      RB_BeginImmediateMode();
      GL_State(65554);
      GL_Bind((image_t *)tr_whiteImage);
      GL_Cull(2);                                    /* CT_TWO_SIDED */
      qglLoadIdentity();
      glStencilOp(0x1E00u, 0x1E00u, 0x1E00u);       /* GL_KEEP x3 */

      for ( i = 13; i >= 0; i-- )
      {
        glStencilFunc(0x206u, s_overdrawStencilRefs[i], 0xFFFFFFFF);   /* GL_EQUAL */
        RB_glColor3fv(s_overdrawColors[i]);
        RB_glBegin(9);                               /* GL_POLYGON */
        RB_glVertex2f(0.0f, 0.0f);
        RB_glVertex2f((float)glConfig_vidWidth, 0.0f);
        RB_glVertex2f((float)glConfig_vidWidth, (float)glConfig_vidHeight);
        RB_glVertex2f(0.0f, (float)glConfig_vidHeight);
        RB_glEnd();
      }

      RB_EndImmediateMode();
    }
  }

  if ( !glState_finishCalled )
    qglFinish();
  if ( Stream )
    fprintf(Stream, "%s", (int)"***************** RB_SwapBuffers *****************\n\n\n");
  if ( !r_swapDelay->integer )
  {
    GLimp_EndFrame();
    backEnd_projection2D = 0;
  }
  else
  {
    qglFlush();
    backEnd_projection2D = 0;
    backEnd_endFramePending = 1;
  }
  return (int)(cmd + 1);
}

/* ---- RB_ExecuteRenderCommands  0x004D8AB0 ----  [HIGH]
 * Walk the render-command list, dispatching each command to its RB_* handler
 * until RC_END_OF_LIST (or an unknown id).  Each handler returns the pointer to
 * the next command. */
void __cdecl RB_ExecuteRenderCommands( const void *data )
{
  int t1, t2;

  t1 = ri.Milliseconds();
  if ( backEnd_endFramePending )
  {
    backEnd_endFramePending = 0;
    GLimp_EndFrame();
  }

  backEnd_dynamicBuffer_freeBytes          = backEnd_dynamicBuffer_capacity;
  backEnd_dynamicBuffer_allocationSequence = 0;
  backEnd_dynamicBuffer_reclaimSequence    = 1;
  backEnd_dynamicBuffer_allocations[0]     = -1;
  backEnd_dynamicBuffer_currentOffset      = 0;
  backEnd_dynamicBuffer_frameSerial        = tr_dynamicBufferFrameSerial;

  while ( 1 )
  {
    switch ( *(const int *)data )
    {
      case RC_SET_COLOR:
        data = (const void *)RB_SetColor((setColorCommand_t *)data);
        break;
      case RC_STRETCH_PIC:
        data = (const void *)RB_StretchPic((int)data);
        break;
      case RC_STRETCH_PIC_GRADIENT:
        data = (const void *)RB_StretchPicGradient((int)data);
        break;
      case RC_STRETCH_PIC_ROTATE:
        data = (const void *)RB_StretchPicRotate((int)data);
        break;
      case RC_DRAW_QUAD_PIC:
        data = RB_DrawQuadPic((unsigned int *)data);
        break;
      case RC_TEXT_PAINT:
        /* RB_Text_Paint, expanded: calling it gives its out-of-line copy a frame pad */
        data = (const char *)data
             + ((RB_Text_PaintWithCursor(
                   ((const float *)data)[1],
                   ((const float *)data)[2],
                   ((const int *)data)[3],
                   ((const float *)data)[4],
                   (int)((const int *)data + 5),
                   (const char *)data + 37,
                   ((const int *)data)[8],
                   ((const unsigned char *)data)[36],
                   ((const float *)data)[6],
                   ((const int *)data)[7])
               + 41)
              & 0xFFFFFFFC);
        break;
      case RC_DRAW_SURFS:
        data = RB_DrawSurfs((int *)data);
        break;
      case RC_DRAW_BUFFER:
        data = (const void *)RB_DrawBuffer((int)data);
        break;
      case RC_SAVE_SCREEN:
        data = (const void *)RB_SaveScreen((int)data);
        break;
      case RC_BLEND_SAVED_SCREEN:
        data = (const void *)RB_BlendSavedScreen((int)data);
        break;
      case RC_SWAP_BUFFERS:
        data = (const void *)RB_SwapBuffers((int)data);
        break;
      case RC_END_OF_LIST:
      default:
        t2 = ri.Milliseconds();
        backEnd_pc_msec = t2 - t1;
        if ( tr_dynamicBufferMaxFrameSerial < backEnd_dynamicBuffer_frameSerial )
          tr_dynamicBufferMaxFrameSerial = backEnd_dynamicBuffer_frameSerial;
        return;
    }
  }
}
