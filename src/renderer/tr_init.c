/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"


#include "tr_records.h"

int reverseBitsTableNeedsInitialization = 1;
int screenShotTGA_lastNumber = -1;
int screenShotJPEG_lastNumber = -1;
char aSun[4] = "sun";

#include "tr_shaderregistry.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"      /* typed structs; AFTER tr_records.h (uses TRG) */
/* ---- renderer-owned globals ---- */
glstate_t glState;                      /* 0x016C38C0 */
glconfig_t glConfig;                    /* 0x016C3A60 (RTCW tr_init.c) */
cvar_t *r_fullscreen;                   /* 0x016C35C4 */
cvar_t *cg_shadows;                     /* 0x016C36F8 */
cvar_t *cg_skybox;                      /* 0x016C39C4 */
cvar_t *r_lightmap;                     /* 0x016C36D4 */
cvar_t *r_lodbias;                      /* 0x016C3750 */
cvar_t *r_nv_fogdist_mode;              /* 0x016C3878 */
cvar_t *r_optimize;                     /* 0x016C383C */
cvar_t *r_optimizeXModels;              /* 0x016C3738 */
cvar_t *r_textureMode;                  /* 0x016C3734 */
cvar_t *r_vc_compile;                   /* 0x016C3894 */
cvar_t *sv_cheats;                      /* 0x016C39B4 */
cvar_t *r_arb_texture_cube_map;         /* 0x016C3880 */
cvar_t *r_arb_texture_env_add;          /* 0x016C37A8 */
cvar_t *r_arb_texture_env_combine;      /* 0x016C3798 */
cvar_t *r_arb_texture_env_dot3;         /* 0x016C374C */
cvar_t *r_arb_vertex_buffer_object;     /* 0x016C37C4 */
cvar_t *r_arb_vertex_program;           /* 0x016C39D0 */
cvar_t *r_ati_element_array;            /* 0x016C3820 */
cvar_t *r_ati_fragment_shader;          /* 0x016C3810 */
cvar_t *r_ati_pntriangles;              /* 0x016C3794 */
cvar_t *r_ati_vertex_array_object;      /* 0x016C36F0 */
cvar_t *r_colorbits;                    /* 0x016C36C8 */
cvar_t *r_depthbits;                    /* 0x016C3B10 */
cvar_t *r_displayRefresh;               /* 0x016C3A34 */
cvar_t *r_drawBuffer;                   /* 0x016C3808 */
cvar_t *r_ext_compiled_vertex_array;    /* 0x016C3B18 */
cvar_t *r_ext_draw_range_elements;      /* 0x016C3730 */
cvar_t *r_ext_rescale_normal;           /* 0x016C3A28 */
cvar_t *r_ext_texture_filter_anisotropic; /* 0x016C3B08 */
cvar_t *r_ignorehwgamma;                /* 0x016C3A14 */
cvar_t *r_logFile;                      /* 0x016C3A24 */
cvar_t *r_maxActiveTextures;            /* 0x016C39CC */
cvar_t *r_mode;                         /* 0x016C3784 */
cvar_t *r_nv_fence;                     /* 0x016C37E4 */
cvar_t *r_nv_fog_dist;                  /* 0x016C3760 */
cvar_t *r_nv_register_combiners;        /* 0x016C399C */
cvar_t *r_nv_texture_shader;            /* 0x016C37CC */
cvar_t *r_nv_vertex_array_range;        /* 0x016C37F8 */
cvar_t *r_stencilbits;                  /* 0x016C3754 */
cvar_t *r_swapInterval;                 /* 0x016C3A18 */
cvar_t *r_verbose;                      /* 0x016C36B4 */
/* the renderer cvars R_Register creates (RTCW tr_init.c) */
cvar_t *X_r_inGameVideo;
cvar_t *outsideMapEnts;
cvar_t *r_ati_fsaa_samples;
cvar_t *r_ati_truform_normalmode;
cvar_t *r_ati_truform_pointmode;
cvar_t *r_ati_truform_tess;
cvar_t *r_clear;
cvar_t *r_colorMipLevels;
cvar_t *r_cullBModels;
cvar_t *r_cullXModels;
cvar_t *r_customaspect;
cvar_t *r_customheight;
cvar_t *r_customwidth;
cvar_t *r_debugEntLight;
cvar_t *r_debugGLErrors;
cvar_t *r_debugOptTex;
cvar_t *r_debugSort;
cvar_t *r_detailtextures;
cvar_t *r_diffuseSunQuality;
cvar_t *r_diffuseSunSteps;
cvar_t *r_dlightQuality;
cvar_t *r_drawBModels;
cvar_t *r_drawSModels;
cvar_t *r_drawSun;
cvar_t *r_drawXModels;
cvar_t *r_drawentities;
cvar_t *r_drawworld;
cvar_t *r_dynamiclight;
cvar_t *r_entFullbright;
cvar_t *r_entLightCutoff;
cvar_t *r_entMinLight;
cvar_t *r_errorOnConflicts;
cvar_t *r_fastsky;
cvar_t *r_finish;
cvar_t *r_flareFadeIn;
cvar_t *r_flareFadeOut;
cvar_t *r_flareSize;
cvar_t *r_flares;
cvar_t *r_fog;
cvar_t *r_fullbright;
cvar_t *r_gamma;
cvar_t *r_graymap;
cvar_t *r_highLodDist;
cvar_t *r_ignore;
cvar_t *r_ignoreGLErrors;
cvar_t *r_intensity;
cvar_t *r_lockpvs;
cvar_t *r_lodViewDist;
cvar_t *r_lodscale;
cvar_t *r_lowLodDist;
cvar_t *r_maxEntLights;
cvar_t *r_maxTextureSize;
cvar_t *r_maxpolys;
cvar_t *r_maxpolyverts;
cvar_t *r_measureOverdraw;
cvar_t *r_mediumLodDist;
cvar_t *r_mem_agp;
cvar_t *r_mem_backend;
cvar_t *r_mem_manual;
cvar_t *r_mem_video;
cvar_t *r_minEntLightIntensity;
cvar_t *r_nobind;
cvar_t *r_nocull;
cvar_t *r_noportals;
cvar_t *r_norefresh;
cvar_t *r_offsetfactor;
cvar_t *r_offsetunits;
cvar_t *r_optimizeBackend;
cvar_t *r_optimizeSModels;
cvar_t *r_optimizeTextures;
cvar_t *r_optimizeWorld;
cvar_t *r_overBrightBits;
cvar_t *r_picmip;
cvar_t *r_picmip2;
cvar_t *r_portalOnly;
cvar_t *r_portalbevels;
cvar_t *r_primitives;
cvar_t *r_printShaders;
cvar_t *r_profileDrawElements;
cvar_t *r_railCoreWidth;
cvar_t *r_railSegmentLength;
cvar_t *r_railWidth;
cvar_t *r_saveFontData;
cvar_t *r_showCullBModels;
cvar_t *r_showCullSModels;
cvar_t *r_showCullXModels;
cvar_t *r_showImages;
cvar_t *r_showLeafLights;
cvar_t *r_showaabbtrees;
cvar_t *r_shownormals;
cvar_t *r_showportals;
cvar_t *r_showsky;
cvar_t *r_showsurfcounts;
cvar_t *r_showtricounts;
cvar_t *r_showtris;
cvar_t *r_simpleMipMaps;
cvar_t *r_singleShader;
cvar_t *r_singlecell;
cvar_t *r_skipBackEnd;
cvar_t *r_skip_auto_config;
cvar_t *r_smc_enable;
cvar_t *r_speeds;
cvar_t *r_sunblind_fadein;
cvar_t *r_sunblind_fadeout;
cvar_t *r_sunblind_max_angle;
cvar_t *r_sunblind_max_darken;
cvar_t *r_sunblind_min_angle;
cvar_t *r_sunflare_fadein;
cvar_t *r_sunflare_fadeout;
cvar_t *r_sunflare_max_alpha;
cvar_t *r_sunflare_max_angle;
cvar_t *r_sunflare_max_size;
cvar_t *r_sunflare_min_angle;
cvar_t *r_sunflare_min_size;
cvar_t *r_sunflare_shader;
cvar_t *r_sunglare_fadein;
cvar_t *r_sunglare_fadeout;
cvar_t *r_sunglare_max_angle;
cvar_t *r_sunglare_max_lighten;
cvar_t *r_sunglare_min_angle;
cvar_t *r_sunsprite_shader;
cvar_t *r_sunsprite_size;
cvar_t *r_suntest;
cvar_t *r_swapDelay;
cvar_t *r_texturebits;
cvar_t *r_uifullscreen;
cvar_t *r_vbo_interleave;
cvar_t *r_vbo_paranoia;
cvar_t *r_vbo_smc_static_draw;
cvar_t *r_vbo_stream_draw;
cvar_t *r_vc_makelog;
cvar_t *r_vc_showlog;
cvar_t *r_weightMipMaps;
cvar_t *r_xdebug;
cvar_t *r_zfar;
cvar_t *r_znear;
cvar_t *r_znear_depthhack;

/* RTCW tr_init.c: the scene polygon limits R_Init sets from r_maxpolys / r_maxpolyverts */
int max_polys;
int max_polyverts;
COD1_ASSERT_SIZE( glconfig_t, 0xA0 );


void GfxInfo_f( void );
void GL_SetDefaultState( void );   /* defined below; forward-declared for InitOpenGL */
extern unsigned char s_gammatable[256];
extern unsigned char s_intensitytable[256];

extern int FFT_Init();
extern int GL_TextureMode();
extern void GLimp_Init( void );
extern void GLimp_Shutdown( void );
void R_SyncRenderThread( void );
extern void __cdecl RB_ExecuteRenderCommands( const void *data );
extern void RB_ShowImages( void );
extern int RE_AddCoronaToScene();
extern int RE_AddLightToScene();
extern int RE_AddPlume();
extern int RE_AddPolyToScene();
extern int RE_AddPolysToScene();
extern int RE_AddRefEntityToScene();
extern int RE_BeginFrame();
extern void __cdecl RE_BeginRegistration(glconfig_t *glConfigOut);
extern int RE_BlendSavedScreen();
extern int RE_ClearFlares();
extern int RE_ClearScene();
extern int RE_DrawQuadPic();
extern int RE_EndFrame();
extern int RE_FinishLoadingModels();
extern int RE_GetFarPlaneDist();
extern int RE_GetIgnorePrecacheErrors();
extern int RE_GetImageMemory();
extern int RE_GetShaderFromModel();
extern int RE_GetShaderName();
extern int RE_GetXModelByHandle();
extern int RE_LoadWorldMap();
extern int RE_MarkFragments();
extern int RE_LocateDebugLines();
extern int RE_LocateDebugStrings();
extern int RE_PickShader();
extern int RE_RegisterFont();
extern int RE_RegisterModel();
extern int RE_RegisterShader();
extern int RE_RegisterShaderNoMip();
extern int RE_RenderScene();
extern int RE_RestoreFogState();
extern int RE_SaveFogState();
extern int RE_SaveScreen();
extern int RE_SetColor();
extern int RE_SetIgnorePrecacheErrors();
extern int RE_StretchPic();
extern int RE_StretchPicGradient();
extern int RE_StretchPicRotate();
extern int RE_StretchRaw();
extern int RE_Text_ConsolePaint();
extern int RE_Text_ConsoleWidth();
extern int RE_Text_Height();
extern int RE_Text_Paint();
extern int RE_Text_PaintWithCursor();
extern int RE_Text_Width();
extern int RE_TrackStatistics();
extern int RE_UploadCinematic();
extern int R_CreateBuiltinImages();
extern int R_DeleteBuffersARB();
extern int R_DeleteTextures();
extern int R_DeleteVertexPrograms();
extern int R_FreeImageAllocations();
extern int R_GammaCorrect();
extern int R_GetEntityToken();
extern int R_ImageList_f();
extern int R_InitAllocators();
extern void R_InitDebug();
extern char *__cdecl R_InitShaders( void );
extern void R_LoadSun_f( void );
void R_MemInfo_f( void );
extern int R_ModelBounds();
extern int R_RefreshOptimizedWorldSurfaces_ARB();
extern int R_RefreshStaticModels_ARB();
extern int R_RefreshXModels_ARB();
extern int R_ResetImageAllocations();
extern int R_SaveLightVisHistory();
extern int R_SaveSun_f();
extern int R_SetColorMappings();
extern int R_SetFog();
extern int R_SetHwLightGlobals();
extern int R_ShaderList_f();
void R_ShutdownAllocators( void );
extern int R_ShutdownDebug();
extern int R_ShutdownStaticModels();
extern int R_StaticModelCacheFlush_f();
extern int R_StaticModelCacheStats_f();
extern int R_SunHelp_f();
extern int R_VC_stats_f();
void __cdecl SaveJPG( int fileName, int quality, int width, unsigned int height,
                     int flipped, int pixels );
extern int RE_SetFarPlaneDist();

/* ---- AssertCvarRange  0x004B0A30 ----  DONE
 * Warn (and clamp via Cvar_Set) when a cvar is non-integral or out of range.
 * Q3 arg order (cv, minVal, maxVal, shouldBeIntegral): Hex-Rays listed it as
 * (shouldBeIntegral@eax, cv@esi, ...); the Q3 order gives R_Register's retail
 * store scheduling. */
void __cdecl AssertCvarRange( cvar_t *cv, float minVal, float maxVal, int shouldBeIntegral )
{
  if ( shouldBeIntegral )
  {
    if ( (int)cv->value != cv->integer )
    {
      ri.Printf( 2, "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value );
      ri.Cvar_Set( cv->name, va( "%d", cv->integer ) );
    }
  }
  if ( cv->value < minVal )
  {
    ri.Printf( 2, "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal );
    ri.Cvar_Set( cv->name, va( "%f", minVal ) );
  }
  else if ( cv->value > maxVal )
  {
    ri.Printf( 2, "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal );
    ri.Cvar_Set( cv->name, va( "%f", maxVal ) );
  }
}

/* ---- GL_SetupVBO  0x004B0B30 ----  VERIFIED
 * Refresh the VBO cvars.  RETAIL BUG (do not "fix"): r_vbo_smc_static_draw is
 * tested as a POINTER, not ->integer, and R_Register always assigns it, so
 * tr_vboUsage is permanently GL_STATIC_DRAW_ARB (0x88E4) and GL_STREAM_DRAW_ARB
 * (0x88E8) is unreachable.  The return is dead EAX (the cvar pointer). */
int GL_SetupVBO( void )
{
  tr_vboInterleaved = r_vbo_interleave->integer != 0;
  tr_vboStreamDraw  = r_vbo_stream_draw->integer != 0;
  tr_vboUsage = r_vbo_smc_static_draw ? 35044 : 35048;
  return (int)(size_t)r_vbo_smc_static_draw;
}

/* ---- InitOpenGL  0x004B0B80 ----  VERIFIED
 * First-time GL bring-up: create the context (GLimp_Init), then query and clamp
 * the texture-size / light limits.  On later calls (context already up, i.e.
 * glConfig_vidWidth != 0) only the banner, default state and VBO cvars are refreshed. */
void InitOpenGL( void )
{
  char renderer_buffer[1024];
  int  temp;

  if ( !glConfig_vidWidth )                            /* glConfig.vidWidth == 0 => not yet initialised */
  {
    GLimp_Init();

    /* lower-cased copy of GL_RENDERER (vestigial: the copy is never read) */
    strcpy( renderer_buffer, (const char *)glConfig_renderer_string );
    Q_strlwr( renderer_buffer );

    qglGetIntegerv( 0x0D33, &temp );                /* GL_MAX_TEXTURE_SIZE */
    glConfig_maxTextureSize = temp;
    if ( glConfig_maxTextureSize <= 0 )
      glConfig_maxTextureSize = 0;
    else if ( glConfig_maxTextureSize > 0x2000 )
      glConfig_maxTextureSize = 0x2000;

    /* honour the r_maxTextureSize (>= 1024) cap by halving down */
    if ( r_maxTextureSize->integer >= 1024 )
    {
      while ( glConfig_maxTextureSize > r_maxTextureSize->integer )
        glConfig_maxTextureSize >>= 1;
    }

    qglGetIntegerv( 0x0D31, &temp );                /* GL_MAX_LIGHTS */
    glConfig_maxLights = temp;
    if ( glConfig_maxLights <= 0 )
      glConfig_maxLights = 8;
    else if ( glConfig_maxLights > 16 )
      glConfig_maxLights = 16;
  }

  GfxInfo_f();
  glState_numEnabledLights = 0;
  GL_SetDefaultState();
  GL_SetupVBO();                 /* inlined tail: refresh tr_vbo* cvars */
}

/* ---- GL_CheckErrors  0x004B0CF0 ----  [HIGH] */
int __cdecl GL_CheckErrors( const char *location )
{
	int		err;
	char	s[64];

	err = qglGetError();
	if ( !err || r_ignoreGLErrors->integer ) {
		return err;
	}

	switch ( err ) {
	case 0x0500:	strcpy( s, "GL_INVALID_ENUM" );			break;
	case 0x0501:	strcpy( s, "GL_INVALID_VALUE" );		break;
	case 0x0502:	strcpy( s, "GL_INVALID_OPERATION" );	break;
	case 0x0503:	strcpy( s, "GL_STACK_OVERFLOW" );		break;
	case 0x0504:	strcpy( s, "GL_STACK_UNDERFLOW" );		break;
	case 0x0505:	strcpy( s, "GL_OUT_OF_MEMORY" );		break;
	default:		Com_sprintf( s, sizeof( s ), "%i", err );	break;
	}

	return ri.Error( 0, "\x15" "GL_CheckErrors: %s (location = %s)", s, location );   /* 0x558A24; does not return */
}

/* ---- R_GetModeInfo  0x004B0EC0 ---- DONE */
typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;              // pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
	//stock CoD1 modes -- retail r_vidModes @0x005716C0 (13 entries, count @0x571790)
	{ "Mode  0: 320x240",           320,  240, 1 },
	{ "Mode  1: 400x300",           400,  300, 1 },
	{ "Mode  2: 512x384",           512,  384, 1 },
	{ "Mode  3: 640x480",           640,  480, 1 },
	{ "Mode  4: 800x600",           800,  600, 1 },
	{ "Mode  5: 960x720",           960,  720, 1 },
	{ "Mode  6: 1024x768",         1024,  768, 1 },
	{ "Mode  7: 1152x864",         1152,  864, 1 },
	{ "Mode  8: 1280x1024",        1280, 1024, 1 },
	{ "Mode  9: 1600x1200",        1600, 1200, 1 },
	{ "Mode 10: 2048x1536",        2048, 1536, 1 },
	{ "Mode 11: 856x480 (wide)",    856,  480, 1 },
	{ "Mode 12: 1920x1200 (wide)", 1920, 1200, 1 },
};

static int s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

qboolean R_GetModeInfo(int *width, int *height, float *windowAspect, int mode){
  vidmode_t   *vm;

  if ( mode < -1 )
    return qfalse;

  if ( mode >= s_numVidModes )
    return qfalse;

  if ( mode == -1 ){
    *width = r_customwidth->integer;
    *height = r_customheight->integer;
    *windowAspect = r_customaspect->value;
    return qtrue;
  }
  
  vm = &r_vidModes[mode];

	*width  = vm->width;
	*height = vm->height;
	*windowAspect = (float)vm->width / ( vm->height * vm->pixelAspect );
  return qtrue;
}

/* ---- R_ModeList_f  0x004B0F30 ----  DONE  */
void R_ModeList_f()
{
  int i;

  ri.Printf(0, "\n");
  for ( i = 0; i < s_numVidModes; i++ )
	{
      ri.Printf(0, "%s\n", r_vidModes[i].description );
  }
  
  ri.Printf(0, "\n");
}

/* ---- R_TakeScreenshot  0x004B0F90 ----  [HIGH]
 * Read the framebuffer into an uncompressed 24-bit BGR TGA (header + RGB->BGR
 * swap + optional gamma) and write it out.  The gamma pass is R_GammaCorrect
 * inlined over the pixels (buffer + 18), NOT the whole buffer.  Dead EAX =
 * ri.Hunk_FreeTempMemory's return. */
int __cdecl R_TakeScreenshot( GLsizei width, GLint x, GLint y, GLsizei height, const char *fileName )
{
  byte *buffer;
  int   c;
  int   i;

  buffer = (byte *)ri.Hunk_AllocateTempMemory( 3 * ( glConfig_vidWidth * glConfig_vidHeight + 6 ) );   /* 18 + w*h*3 */

  memset( buffer, 0, 18 );                   /* TGA header */
  buffer[2]  = 2;                            /* uncompressed true-colour */
  buffer[12] = (byte)width;
  buffer[13] = (byte)( width >> 8 );
  buffer[14] = (byte)height;
  buffer[15] = (byte)( height >> 8 );
  buffer[16] = 24;                           /* bits per pixel */

  glReadPixels( x, y, width, height, 0x1907u, 0x1401u, buffer + 18 );   /* GL_RGB, GL_UNSIGNED_BYTE */

  /* swap RGB -> BGR in place */
  c = 3 * ( height * width + 6 );
  for ( i = 18; i < c; i += 3 )
  {
    byte tmp      = buffer[i];
    buffer[i]     = buffer[i + 2];
    buffer[i + 2] = tmp;
  }

  if ( tr_overbrightBits > 0 && glConfig_deviceSupportsGamma )
    R_GammaCorrect( buffer + 18, 3 * glConfig_vidWidth * glConfig_vidHeight );

  ri.FS_WriteFile( fileName, buffer, c );
  return ri.Hunk_FreeTempMemory( buffer );
}

/* ---- R_TakeScreenshotJPEG  0x004B10A0 ----  VERIFIED
 * Read the framebuffer as 32-bit BGRA, gamma-correct it and hand it to the JPEG
 * encoder.  The decompiled arg order is (fileName, x, y, width, height) -- note
 * SaveJPG's width arrives in ecx (glConfig.vidWidth) in the retail call.  Dead
 * EAX = ri.Hunk_FreeTempMemory's return. */
int __cdecl R_TakeScreenshotJPEG( const char *fileName, GLint x, GLint y, GLsizei width, GLsizei height )
{
  GLvoid *buffer;

  buffer = (GLvoid *)ri.Hunk_AllocateTempMemory( 4 * glConfig_vidWidth * glConfig_vidHeight );
  glReadPixels( x, y, width, height, 0x1908u, 0x1401u, buffer );   /* GL_BGRA, GL_UNSIGNED_BYTE */

  if ( tr_overbrightBits > 0 && glConfig_deviceSupportsGamma )
    R_GammaCorrect( (int)buffer, 4 * glConfig_vidWidth * glConfig_vidHeight );

  ri.FS_WriteFile( fileName, buffer, 1 );
  SaveJPG( (int)fileName, 95, glConfig_vidWidth, glConfig_vidHeight, 1, (int)buffer );

  return ri.Hunk_FreeTempMemory( buffer );
}

/* ---- R_ScreenshotFilename  0x004B1140 ----  DONE  */
void __cdecl R_ScreenshotFilename(int lastNumber, char *fileName)
{
  if ( lastNumber < 0 || lastNumber > 9999 )
  {
    Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.tga");
    return;
  }
  Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot%04i.tga", lastNumber);
}

/* ---- R_ScreenshotFilenameJPEG  0x004B1180 ----  DONE  */
void __cdecl R_ScreenshotFilenameJPEG(int lastNumber, char *fileName)
{
  if ( lastNumber < 0 || lastNumber > 9999 )
  {
    Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot9999.jpg");
    return;
  }
  Com_sprintf(fileName, MAX_OSPATH, "screenshots/shot%04i.jpg", lastNumber);
}

/* ---- R_LevelShot  0x004B11C0 ----  [HIGH]
 * Q3's 128x128 box-filtered levelshot (Q3 text; R_GammaCorrect inlines). */
void R_LevelShot( void )
{
  char    checkname[MAX_OSPATH];
  byte   *buffer;
  byte   *source;
  byte   *src, *dst;
  int     x, y;
  int     r, g, b;
  rfloat  xScale, yScale;
  int     xx, yy;

  sprintf( checkname, "levelshots/%s.tga", (const char *)( tr_world + 64 ) );

  source = (byte *)ri.Hunk_AllocateTempMemory( glConfig_vidWidth * glConfig_vidHeight * 3 );

  buffer = (byte *)ri.Hunk_AllocateTempMemory( 128 * 128 * 3 + 18 );
  memset( buffer, 0, 18 );
  buffer[2] = 2;          /* uncompressed type */
  buffer[12] = 128;
  buffer[14] = 128;
  buffer[16] = 24;        /* pixel size */

  glReadPixels( 0, 0, glConfig_vidWidth, glConfig_vidHeight, 0x1907u, 0x1401u, source );   /* GL_RGB, GL_UNSIGNED_BYTE */

  /* resample from source */
  xScale = glConfig_vidWidth * (rfloat)( 1.0f / 512.0f );    /* Q3's / 512.0f, / 384.0f: retail multiplies */
  yScale = glConfig_vidHeight * (rfloat)( 1.0f / 384.0f );   /* by the float reciprocals, unrounded */
  for ( y = 0 ; y < 128 ; y++ ) {
    for ( x = 0 ; x < 128 ; x++ ) {
      r = g = b = 0;
      for ( yy = 0 ; yy < 3 ; yy++ ) {
        for ( xx = 0 ; xx < 4 ; xx++ ) {
          src = source + 3 * ( glConfig_vidWidth * (int)( ( y * 3 + yy ) * yScale ) + (int)( ( x * 4 + xx ) * xScale ) );
          r += src[0];
          g += src[1];
          b += src[2];
        }
      }
      dst = buffer + 18 + 3 * ( y * 128 + x );
      dst[0] = b / 12;
      dst[1] = g / 12;
      dst[2] = r / 12;
    }
  }

  /* gamma correct */
  if ( tr_overbrightBits > 0 && glConfig_deviceSupportsGamma ) {
    R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
  }

  ri.FS_WriteFile( checkname, buffer, 128 * 128 * 3 + 18 );

  ri.Hunk_FreeTempMemory( buffer );
  ri.Hunk_FreeTempMemory( source );

  ri.Printf( 0, "Wrote %s\n", checkname );
}

/* ---- R_SaveGameShot  0x004B1520 ----  VERIFIED
 * R_LevelShot's box filter at 512x512 RGBA, written as "<baseName>.jpg". */
void __cdecl R_SaveGameShot( const char *baseName )
{
  char    fileName[MAX_OSPATH];
  byte   *buffer;
  byte   *source;
  byte   *src, *dst;
  int     x, y;
  int     r, g, b;
  rfloat  xScale, yScale;
  int     xx, yy;
  int     i;

  sprintf( fileName, "%s.jpg", baseName );

  source = (byte *)ri.Hunk_AllocateTempMemory( glConfig_vidWidth * glConfig_vidHeight * 4 );
  glReadPixels( 0, 0, glConfig_vidWidth, glConfig_vidHeight, 0x1908u, 0x1401u, source );   /* GL_RGBA, GL_UNSIGNED_BYTE */

  buffer = (byte *)ri.Hunk_AllocateTempMemory( 512 * 512 * 4 );

  /* resample from source */
  xScale = glConfig_vidWidth * (rfloat)( 1.0f / 2048.0f );   /* as R_LevelShot */
  yScale = glConfig_vidHeight * (rfloat)( 1.0f / 1536.0f );
  for ( y = 0 ; y < 512 ; y++ ) {
    for ( x = 0 ; x < 512 ; x++ ) {
      r = g = b = 0;
      for ( yy = 0 ; yy < 3 ; yy++ ) {
        for ( xx = 0 ; xx < 4 ; xx++ ) {
          src = source + 4 * ( glConfig_vidWidth * (int)( ( y * 3 + yy ) * yScale ) + (int)( ( x * 4 + xx ) * xScale ) );
          r += src[0];
          g += src[1];
          b += src[2];
        }
      }
      dst = buffer + 4 * ( y * 512 + x );
      dst[0] = r / 12;
      dst[1] = g / 12;
      dst[2] = b / 12;
      dst[3] = 0;
    }
  }

  /* gamma correct */
  if ( tr_overbrightBits > 0 && glConfig_deviceSupportsGamma ) {
    i = 0;
    do {                  /* do-while: retail's loop is neither unrolled nor strength-reduced */
      buffer[i] = s_gammatable[buffer[i]];
    } while ( ++i < 512 * 512 * 4 );
  }

  SaveJPG( (int)fileName, 90, 512, 512, 1, (int)buffer );

  ri.Hunk_FreeTempMemory( buffer );
  ri.Hunk_FreeTempMemory( source );
}

/* Q_ftol-style round-to-nearest (fld / fadd qword 2^-30 / fistp), inlined into
 * the cubemap shots. */
static __inline int R_ftol( float f )
{
  double  bias = 9.313225746154785e-10;
  int     i;

  __asm fld   f
  __asm fadd  bias
  __asm fistp i

  return i;
}

/* ---- RE_CubemapShot  0x004B17F0 ----  VERIFIED
 * Read one rendered cube face back and write it as a 32-bit TGA whose alpha is
 * the Fresnel reflectance (unpolarised, indices n1/n2) of each texel's view ray.
 * axis[face] holds the face's forward / column / row axes (faces 1..6; face 0
 * is all zero). */
void __cdecl RE_CubemapShot( const char *fileName, int size, int face, float n1, float n2 )
{
  vec3_t  axis[7][3] = {
    { {  0, 0, 0 }, {  0, 0, 0 }, {  0, 0, 0 } },
    { {  0, 0, 1 }, {  0, 1, 0 }, { -1, 0, 0 } },
    { {  0, 0,-1 }, {  0, 1, 0 }, {  1, 0, 0 } },
    { { -1, 0, 0 }, {  0,-1, 0 }, {  0, 0, 1 } },
    { {  1, 0, 0 }, {  0, 1, 0 }, {  0, 0, 1 } },
    { {  0,-1, 0 }, {  1, 0, 0 }, {  0, 0, 1 } },
    { {  0, 1, 0 }, { -1, 0, 0 }, {  0, 0, 1 } } };
  byte   *source, *buffer, *src, *dst;
  int     bufSize;
  int     x, y;
  vec3_t  dir;
  double  incident, refract;
  double  sum, diff;
  double  rs, rp;
  rfloat  R;

  source = (byte *)ri.Hunk_AllocateTempMemory( size * size * 3 );
  bufSize = size * size * 4 + 18;
  buffer = (byte *)ri.Hunk_AllocateTempMemory( bufSize );
  memset( buffer, 0, 18 );
  buffer[2] = 2;          /* uncompressed type */
  buffer[12] = size & 255;
  buffer[13] = size >> 8;
  buffer[14] = size & 255;
  buffer[15] = size >> 8;
  buffer[16] = 32;        /* pixel size */

  qglFinish();
  glReadPixels( 1, glConfig_vidHeight - size - 1, size, size, 0x1907u, 0x1401u, source );   /* GL_RGB, GL_UNSIGNED_BYTE */

  if ( tr_overbrightBits > 0 && glConfig_deviceSupportsGamma )
    R_GammaCorrect( source, size * size * 3 );

  for ( y = 0; y < size; y++ ) {
    for ( x = 0; x < size; x++ ) {
      VectorScale( axis[face][0], size / 2 - 0.5f, dir );
      VectorMA( dir, ( x - size / 2 ) + 0.5f, axis[face][1], dir );
      VectorMA( dir, ( y - size / 2 ) + 0.5f, axis[face][2], dir );
      VectorNormalize( dir );
      if ( dir[2] > 0 ) {
        incident = acos( dir[2] );
        refract = asin( sin( incident ) * ( n1 / n2 ) );
      } else {
        incident = acos( -dir[2] );
        refract = asin( sin( incident ) * ( n2 / n1 ) );
      }
      sum = incident + refract;
      diff = incident - refract;
      rs = sin( diff ) / sin( sum );
      rp = tan( diff ) / tan( sum );
      R = ( rs * rs + rp * rp ) * 0.5;
      if ( R < 0 )
        R = 0;
      else if ( R > 1 )
        R = 1;
      src = source + 3 * ( y * size + x );
      dst = buffer + 18 + 4 * ( y * size + x );
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = R_ftol( R * 255.0f );
    }
  }

  ri.FS_WriteFile( fileName, buffer, bufSize );
  ri.Hunk_FreeTempMemory( buffer );
  ri.Hunk_FreeTempMemory( source );
}

/* ---- RE_CubemapWaterShot  0x004B1E60 ----  VERIFIED
 * As RE_CubemapShot, but writes a 24-bit TGA that blends colorLow -> colorHigh
 * by the view ray's up component and (when the world has a sun light) lights
 * it with the sun's ambient + diffuse colour, read from the loaded world at
 * tr_world + 276 (s_worldData_sunLight).  The COD1_VC7 form is retail's source
 * shape; the modern build keeps the decompile-shaped form, whose double/float
 * locals reproduce retail's x87 roundings (VC7 keeps vec3 temporaries partly
 * unrounded on the FPU stack). */
#ifdef COD1_VC7
void __cdecl RE_CubemapWaterShot( const char *fileName, int size, int face, const float *colorLow, const float *colorHigh )
{
  vec3_t  axis[7][3] = {
    { {  0, 0, 0 }, {  0, 0, 0 }, {  0, 0, 0 } },
    { {  0, 0, 1 }, {  0, 1, 0 }, { -1, 0, 0 } },
    { {  0, 0,-1 }, {  0, 1, 0 }, {  1, 0, 0 } },
    { { -1, 0, 0 }, {  0,-1, 0 }, {  0, 0, 1 } },
    { {  1, 0, 0 }, {  0, 1, 0 }, {  0, 0, 1 } },
    { {  0,-1, 0 }, {  1, 0, 0 }, {  0, 0, 1 } },
    { {  0, 1, 0 }, { -1, 0, 0 }, {  0, 0, 1 } } };
  byte   *buffer, *dst;
  int     bufSize;
  int     x, y;
  vec3_t  dir;
  vec3_t  color;
  vec3_t  sun;
  rfloat  frac;
  rfloat  d;
  float  *sunLight;

  bufSize = 3 * ( size * size + 6 );
  buffer = (byte *)ri.Hunk_AllocateTempMemory( bufSize );
  memset( buffer, 0, 18 );
  buffer[2] = 2;          /* uncompressed type */
  buffer[12] = size & 255;
  buffer[13] = size >> 8;
  buffer[14] = size & 255;
  buffer[15] = size >> 8;
  buffer[16] = 24;        /* pixel size */

  for ( y = 0; y < size; y++ ) {
    for ( x = 0; x < size; x++ ) {
      VectorScale( axis[face][0], size / 2 - 0.5f, dir );
      VectorMA( dir, ( x - size / 2 ) + 0.5f, axis[face][1], dir );
      VectorMA( dir, ( y - size / 2 ) + 0.5f, axis[face][2], dir );
      VectorNormalize( dir );

      frac = 0 > dir[2] ? 0 : dir[2];
      VectorScale( colorLow, 1.0f - frac, color );
      VectorMA( color, frac, colorHigh, color );

      sunLight = *(float **)( tr_world + 276 );
      if ( sunLight ) {
        d = DotProduct( dir, &sunLight[17] );
        if ( 0 > d )
          d = 0;
        VectorAdd( &sunLight[5], &sunLight[9], sun );
        VectorMA( sun, d, &sunLight[9], sun );
        color[0] *= sun[0];
        color[1] *= sun[1];
        color[2] *= sun[2];
      }

      /* clamp to [0,2]; this shape gives retail's block order */
      if ( !( 2 < color[0] ) ) {
        if ( 0 > color[0] )
          color[0] = 0;
      } else
        color[0] = 2;
      if ( !( 2 < color[1] ) ) {
        if ( 0 > color[1] )
          color[1] = 0;
      } else
        color[1] = 2;
      if ( !( 2 < color[2] ) ) {
        if ( 0 > color[2] )
          color[2] = 0;
      } else
        color[2] = 2;

      dst = buffer + 18 + 3 * ( y * size + x );
      dst[0] = R_ftol( color[2] * 255.0f );
      dst[1] = R_ftol( color[1] * 255.0f );
      dst[2] = R_ftol( color[0] * 255.0f );
    }
  }

  ri.FS_WriteFile( fileName, buffer, bufSize );
  ri.Hunk_FreeTempMemory( buffer );
}
#else
int __cdecl RE_CubemapWaterShot(int fileName, int size, int faceIndex, float *colorLow, float *colorHigh)
{
  int dstSize;
  int dstBuf;
  double centerOfs;
  double baseXd;
  double baseYd;
  double baseZd;
  byte *dstRow;
  byte *dst;
  double rowCoord;
  double rowZd;
  double colCoord;
  double dirYd;
  double dirZd;
  double nz;
  float *sunLight;
  double colB;
  double sunDot;
  double sunAmbientR;
  double sunAmbientG;
  double sunG;
  int colPrev;
  int loopDone; // zf
  float colCoordF;
  float bScaled;
  float gScaled;
  float rScaled;
  float colR;
  float colGbase;
  float colG;
  float colBbase;
  int rowsLeft;
  float dir[3]; // [ebp-0x180] BYREF  -- retail vec3_t
  int row;
  int col;
  int colsLeft;
  float *mtx5;
  int bOut;
  int gOut;
  float baseX;
  int rOut;
  float baseZ;
  int dstBufBase;
  float *mtx4;
  int dstSizeBase;
  float rowX;
  float *mtx6;
  float sunR;
  float sunB;
  float rowY;
  float baseY;
  float rowZ;
  byte *dstRowNext;
  float *mtx7;
  float *mtx3;
  float *mtx8;
  double biasB;
  double biasG;
  double biasR;
  /* retail: one contiguous 63-float stack table var_FC..var_4 (7 faces x 9),
   * indexed 36*faceIndex (0x004B218E); same values as RE_CubemapShot's. */
  float faceMtx[7][9] = {
    {  0, 0, 0,   0, 0, 0,   0, 0, 0 },
    {  0, 0, 1,   0, 1, 0,  -1, 0, 0 },
    {  0, 0,-1,   0, 1, 0,   1, 0, 0 },
    { -1, 0, 0,   0,-1, 0,   0, 0, 1 },
    {  1, 0, 0,   0, 1, 0,   0, 0, 1 },
    {  0,-1, 0,   1, 0, 0,   0, 0, 1 },
    {  0, 1, 0,  -1, 0, 0,   0, 0, 1 } };
  float *m;

  dstSize = 3 * (size * size + 6);
  dstSizeBase = dstSize;
  dstBuf = ri.Hunk_AllocateTempMemory(dstSize);
  *(unsigned int *)dstBuf = 0;
  dstBufBase = dstBuf;
  *(unsigned int *)(dstBuf + 4) = 0;
  *(unsigned int *)(dstBuf + 8) = 0;
  *(unsigned int *)(dstBuf + 12) = 0;
  *(unsigned short *)(dstBuf + 16) = 0;
  *(byte *)(dstBuf + 2) = 2;
  *(unsigned short *)(dstBuf + 12) = size;
  *(unsigned short *)(dstBuf + 14) = size;
  *(byte *)(dstBuf + 16) = 24;
  if ( size > 0 )
  {
    centerOfs = (double)(size / 2) - 0.5;
    m = faceMtx[faceIndex];
    mtx3 = &m[3];
    mtx4 = &m[4];
    mtx5 = &m[5];
    mtx6 = &m[6];
    mtx7 = &m[7];
    mtx8 = &m[8];
    baseXd = centerOfs * m[0];
    baseX = baseXd;
    baseYd = centerOfs * m[1];
    biasB = 9.313225746154785e-10;
    biasG = 9.313225746154785e-10;
    baseY = baseYd;
    biasR = 9.313225746154785e-10;
    baseZd = centerOfs * m[2];
    baseZ = baseZd;
    row = size / -2;
    dstRow = (byte *)(dstBuf + 19);
    rowsLeft = size;
    while ( 1 )
    {
      dst = dstRow;
      rowCoord = (double)row + 0.5;
      col = size / -2;
      colsLeft = size;
      rowX = rowCoord * *mtx6;
      rowY = rowCoord * *mtx7;
      rowZd = rowCoord * *mtx8;
      dstRowNext = &dstRow[3 * size];
      rowZ = rowZd;
      do
      {
        dir[2] = baseZ;
        colCoord = (double)col + 0.5;
        colCoordF = colCoord;
        dir[0] = colCoord * *mtx3 + baseX;
        dirYd = colCoordF * *mtx4 + baseY;
        dirZd = colCoordF * *mtx5 + baseZ;
        dir[0] = rowX + dir[0];
        dir[1] = rowY + dirYd;
        dir[2] = rowZ + dirZd;
        VectorNormalize(dir);
        if ( dir[2] >= 0.0 )
          nz = dir[2];
        else
          nz = 0.0;
        colGbase = (1.0 - nz) * colorLow[1];
        colBbase = (1.0 - nz) * colorLow[2];
        colR = nz * *colorHigh + (1.0 - nz) * *colorLow;
        colG = nz * colorHigh[1] + colGbase;
        sunLight = *(float **)(tr_world + 276);
        colB = nz * colorHigh[2] + colBbase;
        if ( sunLight )
        {
          sunDot = dir[2] * sunLight[19] + dir[1] * sunLight[18] + dir[0] * sunLight[17];
          if ( sunDot < 0.0 )
            sunDot = 0.0;
          sunAmbientR = sunLight[5] + sunLight[9];
          sunAmbientG = sunLight[6] + sunLight[10];
          sunB = sunLight[7] + sunLight[11];
          sunR = sunDot * sunLight[9] + sunAmbientR;
          sunG = sunDot * sunLight[10] + sunAmbientG;
          sunB = sunDot * sunLight[11] + sunB;
          colR = sunR * colR;
          colG = sunG * colG;
          colB = colB * sunB;
        }
        if ( colR > 2.0 )
        {
          colR = 2.0;
        }
        else if ( colR < 0.0 )
        {
          colR = 0.0;
        }
        if ( colG > 2.0 )
        {
          colG = 2.0;
        }
        else if ( colG < 0.0 )
        {
          colG = 0.0;
        }
        if ( colB > 2.0 )
        {
          colB = 2.0;
        }
        else if ( colB < 0.0 )
        {
          colB = 0.0;
        }
        /* 0x004B24D5..0x004B252B: fld x; fadd qword 2^-30; fistp -- rounds
         * (FPU mode), no __ftol2 */
        bScaled = colB * 255.0;
        __asm fld   bScaled
        __asm fadd  biasB
        __asm fistp bOut
        *(dst - 1) = bOut;
        gScaled = colG * 255.0;
        __asm fld   gScaled
        __asm fadd  biasG
        __asm fistp gOut
        *dst = gOut;
        rScaled = colR * 255.0;
        __asm fld   rScaled
        __asm fadd  biasR
        __asm fistp rOut
        colPrev = col;
        dst[1] = rOut;
        dst += 3;
        loopDone = colsLeft == 1;
        col = colPrev + 1;
        --colsLeft;
      }
      while ( !loopDone );
      loopDone = rowsLeft == 1;
      ++row;
      --rowsLeft;
      if ( loopDone )
        break;
      dstRow = dstRowNext;
    }
    dstBuf = dstBufBase;
    dstSize = dstSizeBase;
  }
  ri.FS_WriteFile(fileName, dstBuf, dstSize);
  return ri.Hunk_FreeTempMemory(dstBuf);
}
#endif

/* ---- R_ScreenShot_f  0x004B25A0 ----  DONE
 * "screenshot [name|silent]" -- writes a TGA.  The free-slot scan resumes from
 * the persistent module counter screenShotTGA_lastNumber so demo recording does
 * not rescan thousands of files each frame. */
void __cdecl R_ScreenShot_f( void )
{
  char     checkname[MAX_OSPATH];
  int      len;
  qboolean silent;

  if ( !strcmp(ri.Cmd_Argv(1), "levelshot") ) {
    R_LevelShot();
    return;
  }
  if ( !strcmp(ri.Cmd_Argv(1), "savegame") && ri.Cmd_Argc() == 3 && *(char *)ri.Cmd_Argv(2) ) {
    R_SaveGameShot(ri.Cmd_Argv(2));
    return;
  }

  if ( !strcmp( ri.Cmd_Argv( 1 ), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

  if ( ri.Cmd_Argc() == 2 && !silent ) {
    Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv(1));
  } else{
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( screenShotTGA_lastNumber == -1 ) {
			screenShotTGA_lastNumber = 0;
		}
		// scan for a free number
		for ( ; screenShotTGA_lastNumber <= 9999 ; screenShotTGA_lastNumber++ ) {
			R_ScreenshotFilename( screenShotTGA_lastNumber, checkname );

			len = ri.FS_ReadFile( checkname, NULL );
			if ( len <= 0 ) {
				break;  // file doesn't exist
			}
		}

		if ( screenShotTGA_lastNumber >= 9999 ) {
			ri.Printf( 0, "ScreenShot: Couldn't create a file\n" );
			return;
		}

		screenShotTGA_lastNumber++;
	}

  R_TakeScreenshot(glConfig_vidWidth, 0, 0, glConfig_vidHeight, checkname);

  if ( !silent ){
    ri.Printf(0, "Wrote %s\n", checkname);
  }
}

/* ---- R_ScreenShotJPEG_f  0x004B2770 ----  DONE
 * "screenshotJPEG [name|silent]" -- writes a JPEG.  Uses the persistent module
 * counter screenShotJPEG_lastNumber (see R_ScreenShot_f). */
void __cdecl R_ScreenShotJPEG_f( void )
{
  char     checkname[MAX_OSPATH];
  int      len;
  qboolean silent;

  if ( !strcmp(ri.Cmd_Argv(1), "levelshot") ) {
    R_LevelShot();
    return;
  }
  if ( !strcmp(ri.Cmd_Argv(1), "savegame") && ri.Cmd_Argc() == 3 && *(char *)ri.Cmd_Argv(2) ) {
    R_SaveGameShot(ri.Cmd_Argv(2));
    return;
  }

  if ( !strcmp( ri.Cmd_Argv( 1 ), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

  if ( ri.Cmd_Argc() == 2 && !silent ) {
    Com_sprintf(checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv(1));
  } else{
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( screenShotJPEG_lastNumber == -1 ) {
			screenShotJPEG_lastNumber = 0;
		}
		// scan for a free number
		for ( ; screenShotJPEG_lastNumber <= 9999 ; screenShotJPEG_lastNumber++ ) {
			R_ScreenshotFilenameJPEG( screenShotJPEG_lastNumber, checkname );

			len = ri.FS_ReadFile( checkname, NULL );
			if ( len <= 0 ) {
				break;  // file doesn't exist
			}
		}

		if ( screenShotJPEG_lastNumber == 10000 ) {
			ri.Printf( 0, "ScreenShot: Couldn't create a file\n" );
			return;
		}

		screenShotJPEG_lastNumber++;
	}

  R_TakeScreenshotJPEG(checkname, 0, 0, glConfig_vidWidth, glConfig_vidHeight);

  if ( !silent ){
    ri.Printf(0, "Wrote %s\n", checkname);
  }
}

#define GL_FOG_DISTANCE_MODE_NV     0x855A
#define GL_EYE_RADIAL_NV            0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV    0x855C
#define GL_EYE_PLANE                0x2502

/* ---- R_SetNVFogMode  0x004B2950 ----  [HIGH] */
void R_SetNVFogMode( void )
{
	if ( !Q_stricmp( r_nv_fogdist_mode->string, "GL_EYE_PLANE_ABSOLUTE_NV" ) ) {
		glConfig_NVFogMode = GL_EYE_PLANE_ABSOLUTE_NV;	/* glConfig +0x58 */
	} else if ( !Q_stricmp( r_nv_fogdist_mode->string, "GL_EYE_PLANE" ) ) {
		glConfig_NVFogMode = GL_EYE_PLANE;
	} else if ( !Q_stricmp( r_nv_fogdist_mode->string, "GL_EYE_RADIAL_NV" ) ) {
		glConfig_NVFogMode = GL_EYE_RADIAL_NV;
	} else {
		glConfig_NVFogMode = GL_EYE_RADIAL_NV;
		ri.Cvar_Set( "r_nv_fogdist_mode", "GL_EYE_RADIAL_NV" );
	}
	qglFogi( GL_FOG_DISTANCE_MODE_NV, glConfig_NVFogMode );
}

static const GLfloat s_matAmbientDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };	/* 0x00540604 */
static const GLfloat s_matSpecular[4]       = { 0.0f, 0.0f, 0.0f, 1.0f };	/* 0x00540614 */
static const GLfloat s_matEmission[4]       = { 0.0f, 0.0f, 0.0f, 1.0f };	/* 0x00540624 */
static const GLfloat s_lightModelAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };	/* 0x00540634 */

/* ---- GL_SetDefaultState  0x004B2A40 ----  [HIGH] */
void GL_SetDefaultState( void )
{
  int tmu;
  int numTextureUnits;
  char *tessStr;
  int glQueryScratch; // [esp+ECh] [ebp-4h] BYREF

  qglClearDepth(1.0);
  qglCullFace(1028);
  qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  if ( qglActiveTextureARB )
  {
    tmu = 1;
    if ( glConfig_maxActiveTextures > 1 )
    {
      do
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
        GL_TextureMode((int)r_textureMode->string);
        if ( glState_texEnv[glState_currentTmu] != 8448 )
        {
          glState_texEnv[glState_currentTmu] = 8448;
          glTexEnvi(0x2300u, 0x2200u, 8448);
        }
        if ( glConfig_textureCubeMap )
          glDisable(0x8513u);
        glDisable(0xDE1u);
        glDisableClientState(0x8078u);
        numTextureUnits = glConfig_maxActiveTextures;
        glState_cap[tmu++] = 0;
      }
      while ( tmu < numTextureUnits );
    }
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
  }
  if ( glConfig_textureCubeMap )
    glDisable(0x8513u);
  glEnable(0xDE1u);
  glDisableClientState(0x8078u);
  glState_cap[0] = 3553;
  GL_TextureMode((int)r_textureMode->string);
  if ( glState_texEnv[glState_currentTmu] != 8448 )
  {
    glState_texEnv[glState_currentTmu] = 8448;
    glTexEnvi(0x2300u, 0x2200u, 8448);
  }
  qglShadeModel(7425);
  glDepthFunc(0x203u);
  glLightModelfv(0xB53u, s_lightModelAmbient);
  qglLightModelf(2897, 0.0f);
  qglLightModelf(2898, 0.0f);
  qglMaterialfv(1028, 5634, s_matAmbientDiffuse);
  qglMaterialfv(1028, 4610, s_matSpecular);
  qglMaterialf(1028, 5633, 0.0f);
  qglMaterialfv(1028, 5632, s_matEmission);
  glEnableClientState(0x8074u);
  glDisableClientState(0x8076u);
  glDisableClientState(0x8075u);
  glState_clientStateBits = 1024;
  glState_glStateBits = 65792;
  glPolygonMode(0x408u, 0x1B02u);
  qglDepthMask(1);
  glDisable(0xB71u);
  glEnable(0xC11u);
  glDisable(0xB44u);
  glDisable(0xBE2u);
  if ( lConfig_textureCompression )
  {
    qglGetIntegerv(34801, &glQueryScratch);   /* GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI */
    glConfig_maxPNTrianglesTessellationLevel = glQueryScratch;
#ifdef COD1_VC7
    if ( glQueryScratch < r_ati_truform_tess->value )   /* x87: exact int compare */
#else
    if ( (double)glQueryScratch < r_ati_truform_tess->value )
#endif
    {
      tessStr = va("%d", glQueryScratch);
      ri.Cvar_Set("r_ati_truform_tess", tessStr);
    }
    lConfig_textureCompression(34804, r_ati_truform_tess->integer);
  }
  if ( glConfig_maxAnisotropy )
  {
    qglGetFloatv(34047, (GLfloat *)&glQueryScratch);
    glConfig_maxTextureFilterAnisotropy = glQueryScratch;
  }
  glState_fogMode = 2048;
  glState_fogHint = 4352;
  glState_fogColor = 0.0;
  glState_fogColorG = 0.0;
  glState_fogColorB = 0.0;
  glState_fogColorA = 0.0;
  glState_fogStart = 0.0;
  glState_fogEnd = 1.0;
  glState_fogDensity = 1.0;
  qglFogi(2917, 2048);
  qglHint(3156, glState_fogHint);
  qglFogf(2914, glState_fogDensity);
  qglFogf(2915, glState_fogStart);
  qglFogf(2916, glState_fogEnd);
  if ( glConfig_NVFogAvailable )
    R_SetNVFogMode();
  glState_numEnabledLights = 0;
  glState_currentStorageMode = 3;
}

/* ---- GfxInfo_f  0x004B2E40 ----  [HIGH]
 * Print the GL driver / mode / feature report (the "gfxinfo" command). */
void GfxInfo_f( void )
{
  cvar_t     *cpuCvar = ri.Cvar_Get( "sys_cpustring", "", 0 );
  const char *enabledStr[]    = { "disabled", "enabled" };
  const char *fullscreenStr[] = { "windowed", "fullscreen" };
  int         primitives;


  ri.Printf( 0, "\nGL_VENDOR: %s\n",    (const char *)glConfig_vendor_string );
  ri.Printf( 0, "GL_RENDERER: %s\n",    (const char *)glConfig_renderer_string );
  ri.Printf( 0, "GL_VERSION: %s\n",     glConfig_version_string );
  ri.Printf( 0, "GL_EXTENSIONS: %s\n",  (const char *)glConfig_extensions_string );
  ri.Printf( 0, "WGL_EXTENSIONS: %s\n", (const char *)glConfig_wextensions_string );
  ri.Printf( 0, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig_maxTextureSize );
  ri.Printf( 0, "GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig_maxActiveTextures );   /* glConfig.maxActiveTextures */
  ri.Printf( 0, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n",
             glConfig_colorBits, glConfig_depthBits, glConfig_stencilBits );
  ri.Printf( 0, "MODE: %d, %d x %d %s hz:",
             r_mode->integer, glConfig_vidWidth, glConfig_vidHeight,
             fullscreenStr[r_fullscreen->integer == 1] );
  if ( glConfig_displayFrequency )
    ri.Printf( 0, "%d\n", glConfig_displayFrequency );
  else
    ri.Printf( 0, "N/A\n" );

  if ( glConfig_deviceSupportsGamma )
    ri.Printf( 0, "GAMMA: hardware w/ %d overbright bits\n", tr_overbrightBits );
  else
    ri.Printf( 0, "GAMMA: software w/ %d overbright bits\n", tr_overbrightBits );

  ri.Printf( 0, "CPU: %s\n", cpuCvar->string );

  /* default is to use triangles if compiled vertex arrays are present */
  ri.Printf( 0, "rendering primitives: " );
  primitives = r_primitives->integer;
  if ( primitives == 0 )
  {
    if ( qglLockArraysEXT )
      primitives = 2;
    else
      primitives = 1;
  }
  if ( primitives == -1 )
    ri.Printf( 0, "none\n" );
  else if ( primitives == 2 )
    ri.Printf( 0, "single glDrawElements\n" );
  else if ( primitives == 1 )
    ri.Printf( 0, "multiple glArrayElement\n" );
  else if ( primitives == 3 )
    ri.Printf( 0, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );

  ri.Printf( 0, "texturemode: %s\n", r_textureMode->string );
  ri.Printf( 0, "picmip: %d\n", r_picmip->integer );
  ri.Printf( 0, "picmip2: %d\n", r_picmip2->integer );
  ri.Printf( 0, "texture bits: %d\n", r_texturebits->integer );
  ri.Printf( 0, "multitexture: %s\n",           enabledStr[qglActiveTextureARB != 0] );
  ri.Printf( 0, "compiled vertex arrays: %s\n", enabledStr[qglLockArraysEXT != 0] );
  ri.Printf( 0, "texenv add: %s\n",             enabledStr[glConfig_textureEnvAddAvailable != 0] );
  ri.Printf( 0, "ATI truform: %s\n",            enabledStr[lConfig_textureCompression != 0] );
  if ( lConfig_textureCompression )
  {
    ri.Printf( 0, "Truform Tess: %d\n", r_ati_truform_tess->integer );
    ri.Printf( 0, "Truform Point Mode: %s\n", r_ati_truform_pointmode->string );
    ri.Printf( 0, "Truform Normal Mode: %s\n", r_ati_truform_normalmode->string );
  }
  ri.Printf( 0, "NV distance fog: %s\n", enabledStr[glConfig_NVFogAvailable != 0] );
  if ( glConfig_NVFogAvailable )
    ri.Printf( 0, "Fog Mode: %s\n", r_nv_fogdist_mode->string );
  if ( r_finish->integer )
    ri.Printf( 0, "Forcing glFinish\n" );
}

/* ---- R_VboRefresh_f  0x004B31D0 ----  [HIGH]
 * "r_vbo_refresh [list]" -- force a rebuild of the named VBO groups.  Prints the
 * usage line when given no arguments or an unrecognised token. */
void R_VboRefresh_f( void )
{
  const char *arg;
  int         argc;
  int         i;

  if ( !tr_world || !glConfig_ARBVertexBufferObject )
    return;

  argc = Cmd_Argc();
  i = 1;
  if ( i < Cmd_Argc() ) do
  {
    arg = Cmd_Argv( i );

    if      ( !_stricmp( arg, "world" ) )    R_RefreshOptimizedWorldSurfaces_ARB();
    else if ( !_stricmp( arg, "smodels" ) )  R_RefreshStaticModels_ARB( 3 );
    else if ( !_stricmp( arg, "sverts" ) )   R_RefreshStaticModels_ARB( 1 );
    else if ( !_stricmp( arg, "sindexes" ) ) R_RefreshStaticModels_ARB( 2 );
    else if ( !_stricmp( arg, "xmodels" ) )  R_RefreshXModels_ARB( 3 );
    else if ( !_stricmp( arg, "xverts" ) )   R_RefreshXModels_ARB( 1 );
    else if ( !_stricmp( arg, "xindexes" ) ) R_RefreshXModels_ARB( 2 );
    else                                     break;   /* unrecognised token */
  } while (++i < argc);

  if ( argc == 1 || i < argc )
    ri.Printf( 0,
      "usage: r_vbo_refresh [list], where list is one or more of world, smodels, sverts, sindexes, xmodels, xverts, and xindexes\n" );
}

#define MAX_POLYS       4096
#define MAX_POLYVERTS   8192

/* ---- R_Register  0x004B3300 ----  DONE  */
int R_Register()
{
  sv_cheats = Cvar_Get("sv_cheats", "0", CVAR_SYSTEMINFO|CVAR_ROM);

  r_maxActiveTextures           = ri.Cvar_Get("r_maxActiveTextures", "0", CVAR_LATCH);
  r_maxTextureSize              = ri.Cvar_Get("r_maxTextureSize", "0", CVAR_LATCH);
  r_ext_compiled_vertex_array   = ri.Cvar_Get("r_ext_compiled_vertex_array", "1", CVAR_LATCH);
  r_ext_rescale_normal          = ri.Cvar_Get("r_ext_rescale_normal", "1", CVAR_LATCH);
  r_ext_draw_range_elements     = ri.Cvar_Get("r_ext_draw_range_elements", "1", CVAR_LATCH);

  r_ati_pntriangles                 = ri.Cvar_Get("r_ati_pntriangles", "0", CVAR_ARCHIVE|CVAR_LATCH);
  r_ati_truform_tess                = ri.Cvar_Get("r_ati_truform_tess", "1", CVAR_ARCHIVE);
  r_ati_truform_normalmode          = ri.Cvar_Get("r_ati_truform_normalmode", "QUADRATIC", CVAR_ARCHIVE);
  r_ati_truform_pointmode           = ri.Cvar_Get("r_ati_truform_pointmode", "CUBIC", CVAR_ARCHIVE);
  r_ati_fsaa_samples                = ri.Cvar_Get("r_ati_fsaa_samples", "1", CVAR_ARCHIVE);
  r_ext_texture_filter_anisotropic  = ri.Cvar_Get("r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE);

  r_nv_fog_dist       = ri.Cvar_Get("r_nv_fog_dist", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_nv_fogdist_mode   = ri.Cvar_Get("r_nv_fogdist_mode", "GL_EYE_RADIAL_NV", CVAR_ARCHIVE);

  ri.Cvar_Get("r_nv_fog_available", "1", CVAR_ROM);
  r_arb_texture_env_add       = ri.Cvar_Get("r_arb_texture_env_add", "1", CVAR_LATCH);
  r_arb_texture_cube_map      = ri.Cvar_Get("r_arb_texture_cube_map", "1", CVAR_LATCH);
  r_arb_texture_env_combine   = ri.Cvar_Get("r_arb_texture_env_combine", "1", CVAR_LATCH);
  r_arb_texture_env_dot3      = ri.Cvar_Get("r_arb_texture_env_dot3", "1", CVAR_LATCH);
  r_arb_vertex_buffer_object  = ri.Cvar_Get("r_arb_vertex_buffer_object", "1", CVAR_LATCH);
  r_arb_vertex_program        = ri.Cvar_Get("r_arb_vertex_program", "1", CVAR_LATCH);

  r_nv_register_combiners     = ri.Cvar_Get("r_nv_register_combiners", "2", CVAR_LATCH);
  r_nv_texture_shader         = ri.Cvar_Get("r_nv_texture_shader", "1", CVAR_LATCH);
  r_nv_fence                  = ri.Cvar_Get("r_nv_fence", "1", CVAR_LATCH);
  r_nv_vertex_array_range     = ri.Cvar_Get("r_nv_vertex_array_range", "2", CVAR_LATCH);

  r_ati_vertex_array_object   = ri.Cvar_Get("r_ati_vertex_array_object", "1", CVAR_LATCH);
  r_ati_element_array         = ri.Cvar_Get("r_ati_element_array", "1", CVAR_LATCH);
  r_ati_fragment_shader       = ri.Cvar_Get("r_ati_fragment_shader", "1", CVAR_LATCH);

  r_vbo_smc_static_draw       = ri.Cvar_Get("r_vbo_smc_static_draw", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_vbo_stream_draw           = ri.Cvar_Get("r_vbo_stream_draw", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_vbo_interleave            = ri.Cvar_Get("r_vbo_interleave", "0", CVAR_ARCHIVE|CVAR_LATCH);
  r_vbo_paranoia              = ri.Cvar_Get("r_vbo_paranoia", "0", CVAR_ARCHIVE);

  r_skip_auto_config          = ri.Cvar_Get("r_skip_auto_config", "0", CVAR_ARCHIVE|CVAR_LATCH);


  r_picmip          = ri.Cvar_Get("r_picmip", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_picmip2         = ri.Cvar_Get("r_picmip2", "2", CVAR_ARCHIVE|CVAR_LATCH);
  r_colorMipLevels  = ri.Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH);
  AssertCvarRange(r_picmip, 0.0f, 3.0f, qtrue);
  AssertCvarRange(r_picmip2, 0.0f, 3.0f, qtrue);

  r_detailtextures  = ri.Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_texturebits     = ri.Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE|CVAR_LATCH);
  r_colorbits       = ri.Cvar_Get("r_colorbits", "32", CVAR_ARCHIVE|CVAR_LATCH);
  r_stencilbits     = ri.Cvar_Get("r_stencilbits", "8", CVAR_ARCHIVE|CVAR_LATCH);
  r_depthbits       = ri.Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE|CVAR_LATCH);
  r_overBrightBits  = ri.Cvar_Get("r_overBrightBits", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_ignorehwgamma   = ri.Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE|CVAR_LATCH);

  r_mode            = ri.Cvar_Get("r_mode", "3", CVAR_ARCHIVE|CVAR_LATCH);
  r_fullscreen      = ri.Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_customwidth     = ri.Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE|CVAR_LATCH);
  r_customheight    = ri.Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE|CVAR_LATCH);
  r_customaspect    = ri.Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE|CVAR_LATCH);

  r_simpleMipMaps   = ri.Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE|CVAR_LATCH);
  r_weightMipMaps   = ri.Cvar_Get("r_weightMipMaps", "0", CVAR_LATCH);

  r_uifullscreen    = ri.Cvar_Get("r_uifullscreen", "0", 0);
  r_displayRefresh  = ri.Cvar_Get("r_displayRefresh", "0", CVAR_LATCH);
  AssertCvarRange(r_displayRefresh, 0.0f, 200.0f, qtrue);

  r_fullbright      = ri.Cvar_Get("r_fullbright", "0", CVAR_LATCH|CVAR_CHEAT);
  r_intensity       = ri.Cvar_Get("r_intensity", "1", CVAR_LATCH);
  r_singleShader    = ri.Cvar_Get("r_singleShader", "0", CVAR_LATCH|CVAR_CHEAT);
  r_lodbias         = ri.Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE);
  r_flares          = ri.Cvar_Get("r_flares", "1", CVAR_CHEAT);
  r_znear           = ri.Cvar_Get("r_znear", "4", CVAR_CHEAT);
  AssertCvarRange(r_znear, 0.001f, 200.0f, qtrue);

  r_zfar            = ri.Cvar_Get("r_zfar", "0", CVAR_CHEAT);
  r_znear_depthhack = ri.Cvar_Get("r_znear_depthhack", "0.1", CVAR_CHEAT);
  AssertCvarRange(r_znear_depthhack, 0.001f, 100.0f, qtrue);

  r_ignoreGLErrors  = ri.Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
  r_fastsky         = ri.Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE);
  X_r_inGameVideo   = ri.Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE);   /* renderer's own slot (0x016C39BC), not the client's r_inGameVideo */
  r_drawSun         = ri.Cvar_Get("r_drawSun", "1", CVAR_ARCHIVE);
  r_dynamiclight    = ri.Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE);
  r_dlightQuality   = ri.Cvar_Get("r_dlightQuality", "1", CVAR_ARCHIVE);
  r_finish          = ri.Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
  r_textureMode     = ri.Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE);
  r_swapDelay       = ri.Cvar_Get("r_swapDelay", "0", CVAR_ARCHIVE);
  r_swapInterval    = ri.Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
  r_gamma           = ri.Cvar_Get("r_gamma", "1.0", CVAR_ARCHIVE);

  r_railWidth           = ri.Cvar_Get("r_railWidth", "16", CVAR_ARCHIVE);
  r_railCoreWidth       = ri.Cvar_Get("r_railCoreWidth", "1", CVAR_ARCHIVE);
  r_railSegmentLength   = ri.Cvar_Get("r_railSegmentLength", "32", CVAR_ARCHIVE);

  r_primitives      = ri.Cvar_Get("r_primitives", "0", CVAR_ARCHIVE);

  r_showImages      = ri.Cvar_Get("r_showImages", "0", CVAR_TEMP);

  r_debugSort       = ri.Cvar_Get("r_debugSort", "0", CVAR_CHEAT);
  r_printShaders    = ri.Cvar_Get("r_printShaders", "0", 0);
  r_saveFontData    = ri.Cvar_Get("r_saveFontData", "0", 0);

  r_showLeafLights        = ri.Cvar_Get("r_showLeafLights", "0", CVAR_CHEAT);
  r_debugEntLight         = ri.Cvar_Get("r_debugEntLight", "0", CVAR_CHEAT);
  r_maxEntLights          = ri.Cvar_Get("r_maxEntLights", "8", CVAR_ARCHIVE);
  r_minEntLightIntensity  = ri.Cvar_Get("r_minEntLightIntensity", "0.02", CVAR_ARCHIVE);
  r_entLightCutoff        = ri.Cvar_Get("r_entLightCutoff", "0.2", CVAR_ARCHIVE);
  r_entFullbright         = ri.Cvar_Get("r_entFullbright", "0", CVAR_CHEAT);
  r_entMinLight           = ri.Cvar_Get("r_entMinLight", ".15", CVAR_CHEAT);
  r_diffuseSunSteps       = ri.Cvar_Get("r_diffuseSunSteps", "3", CVAR_ARCHIVE);
  r_diffuseSunQuality     = ri.Cvar_Get("r_diffuseSunQuality", "2", CVAR_ARCHIVE);

  r_vc_makelog  = ri.Cvar_Get("r_vc_makelog", "0", CVAR_LATCH);
  r_vc_showlog  = ri.Cvar_Get("r_vc_showlog", "0", 0);
  r_vc_compile  = ri.Cvar_Get("r_vc_compile", "0", CVAR_LATCH);

  r_fog         = ri.Cvar_Get("r_fog", "1", CVAR_CHEAT);
  r_drawworld   = ri.Cvar_Get("r_drawworld", "1", CVAR_CHEAT);
  r_lightmap    = ri.Cvar_Get("r_lightmap", "0", CVAR_CHEAT);
  r_graymap     = ri.Cvar_Get("r_graymap", "0", CVAR_LATCH|CVAR_CHEAT);
  r_portalOnly  = ri.Cvar_Get("r_portalOnly", "0", CVAR_CHEAT);

  r_flareSize     = ri.Cvar_Get("r_flareSize", "96", CVAR_CHEAT);
  r_flareFadeIn   = ri.Cvar_Get("r_flareFadeIn", ".2", CVAR_CHEAT);
  r_flareFadeOut  = ri.Cvar_Get("r_flareFadeOut", ".2", CVAR_CHEAT);

  r_skipBackEnd   = ri.Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);

  r_measureOverdraw       = ri.Cvar_Get("r_measureOverdraw", "0", CVAR_CHEAT);
  r_lodscale              = ri.Cvar_Get("r_lodscale", "1", CVAR_ARCHIVE);
  r_norefresh             = ri.Cvar_Get("r_norefresh", "0", CVAR_CHEAT);
  r_drawentities          = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
  r_drawBModels           = ri.Cvar_Get("r_drawBModels", "1", CVAR_CHEAT);
  r_drawSModels           = ri.Cvar_Get("r_drawSModels", "1", CVAR_CHEAT);
  r_drawXModels           = ri.Cvar_Get("r_drawXModels", "1", CVAR_CHEAT);
  r_ignore                = ri.Cvar_Get("r_ignore", "1", 0);
  r_nocull                = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);
  outsideMapEnts          = ri.Cvar_Get("outsideMapEnts", "0", CVAR_CHEAT);
  r_speeds                = ri.Cvar_Get("r_speeds", "0", CVAR_CHEAT);
  r_verbose               = ri.Cvar_Get("r_verbose", "0", 0);
  r_logFile               = ri.Cvar_Get("r_logFile", "0", 0);
  r_debugGLErrors         = ri.Cvar_Get("r_debugGLErrors", "0", 0);
  r_profileDrawElements   = ri.Cvar_Get("r_profileDrawElements", "0", CVAR_CHEAT);
  r_nobind                = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
  r_showtris              = ri.Cvar_Get("r_showtris", "0", CVAR_CHEAT);
  r_showtricounts         = ri.Cvar_Get("r_showtricounts", "0", CVAR_CHEAT);
  r_showsurfcounts        = ri.Cvar_Get("r_showsurfcounts", "0", CVAR_CHEAT);
  r_showsky               = ri.Cvar_Get("r_showsky", "0", CVAR_CHEAT);
  r_shownormals           = ri.Cvar_Get("r_shownormals", "", CVAR_CHEAT);
  r_clear                 = ri.Cvar_Get("r_clear", "0", 0);
  r_offsetfactor          = ri.Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT);
  r_offsetunits           = ri.Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT);
  r_drawBuffer            = ri.Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
  r_lockpvs               = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);
  r_noportals             = ri.Cvar_Get("r_noportals", "0", CVAR_CHEAT);
  cg_shadows              = ri.Cvar_Get("cg_shadows", "0", CVAR_ARCHIVE|CVAR_CHEAT);
  cg_skybox               = ri.Cvar_Get("cg_skybox", "1", 0);

  r_maxpolys      = ri.Cvar_Get( "r_maxpolys", va( "%d", MAX_POLYS ), 0 );
	r_maxpolyverts  = ri.Cvar_Get( "r_maxpolyverts", va( "%d", MAX_POLYVERTS ), 0 );

  r_showportals       = ri.Cvar_Get("r_showportals", "0", CVAR_CHEAT);
  r_showaabbtrees     = ri.Cvar_Get("r_showaabbtrees", "0", CVAR_CHEAT);
  r_cullBModels       = ri.Cvar_Get("r_cullBModels", "1", 0);
  r_showCullBModels   = ri.Cvar_Get("r_showCullBModels", "0", CVAR_CHEAT);
  r_showCullSModels   = ri.Cvar_Get("r_showCullSModels", "0", CVAR_CHEAT);
  r_cullXModels       = ri.Cvar_Get("r_cullXModels", "1", 0);
  r_showCullXModels   = ri.Cvar_Get("r_showCullXModels", "0", CVAR_CHEAT);
  r_singlecell        = ri.Cvar_Get("r_singlecell", "0", CVAR_CHEAT);
  r_portalbevels      = ri.Cvar_Get("r_portalbevels", "0.7", CVAR_ARCHIVE);
  r_xdebug            = ri.Cvar_Get("r_xdebug", "", CVAR_CHEAT);
  r_errorOnConflicts  = ri.Cvar_Get("r_errorOnConflicts", "1", 0);

  r_highLodDist     = ri.Cvar_Get("r_highLodDist", "-1", CVAR_CHEAT);
  r_mediumLodDist   = ri.Cvar_Get("r_mediumLodDist", "0", CVAR_CHEAT);
  r_lowLodDist      = ri.Cvar_Get("r_lowLodDist", "0", CVAR_CHEAT);
  r_lodViewDist     = ri.Cvar_Get("r_lodViewDist", "0", CVAR_CHEAT);

  r_suntest           = ri.Cvar_Get("r_suntest", "0", CVAR_CHEAT);
  r_sunsprite_shader  = ri.Cvar_Get("r_sunsprite_shader", &aSun, 0);
  r_sunsprite_size    = ri.Cvar_Get("r_sunsprite_size", "16", 0);

  r_sunflare_shader       = ri.Cvar_Get("r_sunflare_shader", "sunFlareShader", 0);
  r_sunflare_min_size     = ri.Cvar_Get("r_sunflare_min_size", "0", 0);
  r_sunflare_min_angle    = ri.Cvar_Get("r_sunflare_min_angle", "45", 0);
  r_sunflare_max_size     = ri.Cvar_Get("r_sunflare_max_size", "2500", 0);
  r_sunflare_max_angle    = ri.Cvar_Get("r_sunflare_max_angle", "2", 0);
  r_sunflare_max_alpha    = ri.Cvar_Get("r_sunflare_max_alpha", "1", 0);
  r_sunflare_fadein       = ri.Cvar_Get("r_sunflare_fadein", "1", 0);
  r_sunflare_fadeout      = ri.Cvar_Get("r_sunflare_fadeout", "1", 0);

  r_sunblind_min_angle    = ri.Cvar_Get("r_sunblind_min_angle", "30", 0);
  r_sunblind_max_angle    = ri.Cvar_Get("r_sunblind_max_angle", "5", 0);
  r_sunblind_max_darken   = ri.Cvar_Get("r_sunblind_max_darken", ".75", 0);
  r_sunblind_fadein       = ri.Cvar_Get("r_sunblind_fadein", ".5", 0);
  r_sunblind_fadeout      = ri.Cvar_Get("r_sunblind_fadeout", "3", 0);

  r_sunglare_min_angle    = ri.Cvar_Get("r_sunglare_min_angle", "30", 0);
  r_sunglare_max_angle    = ri.Cvar_Get("r_sunglare_max_angle", "5", 0);
  r_sunglare_max_lighten  = ri.Cvar_Get("r_sunglare_max_lighten", ".75", 0);
  r_sunglare_fadein       = ri.Cvar_Get("r_sunglare_fadein", ".5", 0);
  r_sunglare_fadeout      = ri.Cvar_Get("r_sunglare_fadeout", "3", 0);

  r_optimize          = ri.Cvar_Get("r_optimize", "1", CVAR_LATCH);
  r_optimizeBackend   = ri.Cvar_Get("r_optimizeBackend", "1", CVAR_LATCH);
  r_optimizeSModels   = ri.Cvar_Get("r_optimizeSModels", "1", CVAR_LATCH);
  r_optimizeXModels   = ri.Cvar_Get("r_optimizeXModels", "100", CVAR_LATCH);
  r_optimizeWorld     = ri.Cvar_Get("r_optimizeWorld", "1", CVAR_LATCH);
  r_optimizeTextures  = ri.Cvar_Get("r_optimizeTextures", "2", CVAR_LATCH);
  r_debugOptTex       = ri.Cvar_Get("r_debugOptTex", "0", 0);

  r_mem_manual    = ri.Cvar_Get("r_mem_manual", "0", CVAR_ARCHIVE|CVAR_LATCH);
  r_mem_agp       = ri.Cvar_Get("r_mem_agp", "8", CVAR_ARCHIVE|CVAR_LATCH);
  r_mem_video     = ri.Cvar_Get("r_mem_video", "2", CVAR_ARCHIVE|CVAR_LATCH);
  r_mem_backend   = ri.Cvar_Get("r_mem_backend", "0.5", CVAR_ARCHIVE|CVAR_LATCH);
  r_smc_enable    = ri.Cvar_Get("r_smc_enable", "1", 0);

  ri.Cmd_AddCommand("imagelist", R_ImageList_f);
  ri.Cmd_AddCommand("shaderlist", R_ShaderList_f);
  ri.Cmd_AddCommand("modelist", R_ModeList_f);
  ri.Cmd_AddCommand("screenshot", R_ScreenShot_f);
  ri.Cmd_AddCommand("screenshotJPEG", R_ScreenShotJPEG_f);
  ri.Cmd_AddCommand("gfxinfo", GfxInfo_f);
  ri.Cmd_AddCommand("r_meminfo", R_MemInfo_f);
  ri.Cmd_AddCommand("r_vc_stats", R_VC_stats_f);
  ri.Cmd_AddCommand("r_smc_stats", R_StaticModelCacheStats_f);
  ri.Cmd_AddCommand("r_smc_flush", R_StaticModelCacheFlush_f);
  ri.Cmd_AddCommand("r_loadsun", R_LoadSun_f);
  ri.Cmd_AddCommand("r_savesun", R_SaveSun_f);
  ri.Cmd_AddCommand("r_sunhelp", R_SunHelp_f);
  return ri.Cmd_AddCommand("r_vbo_refresh", R_VboRefresh_f);
}


#define FUNCTABLE_SIZE 1024

/* ---- R_Init  0x004B4590 ----  VERIFIED
 * One-time renderer bring-up: clear the state blobs, build the sin/tri/... wave
 * lookup tables, register cvars/commands, allocate the frame command block,
 * init GL, images, shaders and the default model, then the light-vis cache.
 * The wave tables match the retail constants (divisor FUNCTABLE_SIZE, not
 * FUNCTABLE_SIZE-1); the sin table uses retail's operation order, a multiply
 * by the float reciprocal of 180 (MSVC 7.1 folded DEG2RAD's '/180.0F') rather
 * than a true division. */
void __cdecl R_Init( void )
{
  model_t *model;
  int      i;
  int      err;

  ri.Printf(0, "----- R_Init -----\n");
  /* retail memset(&tr), memset(&backEnd), memset(&tess) */
  memset(&tr_registered, 0, 0x13BD4u);
  memset(&backEndStorage, 0, 0x1A4Cu);
  memset(&tess, 0, sizeof(tess));

  Swap_Init();

	if ( (int)tess_xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}

  memset(tess_constantColor255, 255, sizeof(tess_constantColor255));

  for ( i = 0; i < FUNCTABLE_SIZE; i++ )
  {
    /* retail 0x004B4600: fild i; fmul 0x3EB40000; fmul 0x40490FDB; fmul 0x3BB60B61; fsin */
    tr_sinTable[i] = sin( ( ( (float)i * 0.3515625f ) * M_PI ) * 0.0055555557f );   /* DEG2RAD's grouping */
    tr_squareTable[i] = ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
    tr_sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
    tr_inverseSawToothTable[i] = 1.0f - tr_sawToothTable[i];

    if ( i < FUNCTABLE_SIZE / 2 )
    {
      if ( i < FUNCTABLE_SIZE / 4 )
        tr_triangleTable[i] = (float)i / ( FUNCTABLE_SIZE / 4 );
      else
        tr_triangleTable[i] = 1.0f - tr_triangleTable[i - FUNCTABLE_SIZE / 4];
    }
    else
    {
      tr_triangleTable[i] = -tr_triangleTable[i - FUNCTABLE_SIZE / 2];
    }
  }

  Com_NoiseInit();
  R_Register();

  max_polys = r_maxpolys->integer;
  if ( max_polys < MAX_POLYS ) {
    max_polys = MAX_POLYS;
  }

  max_polyverts = r_maxpolyverts->integer;
  if ( max_polyverts < MAX_POLYVERTS ) {
    max_polyverts = MAX_POLYVERTS;
  }
  backEndData = ri.Hunk_Alloc(16 * (max_polys + 2 * max_polyverts) + 1898244);

  /* inlined R_ToggleSmpFrame: reset the per-frame scene builders */
  *(int *)( backEndData + 1898240 ) = 0;   /* renderCommandList.used */
  r_firstSceneDrawSurf = 0;
  r_numdlights         = 0;
  r_firstSceneDlight   = 0;
  r_numcoronas         = 0;
  r_firstSceneCorona   = 0;
  r_numentities        = 0;
  r_firstSceneEntity   = 0;
  r_numpolys           = 0;
  r_firstScenePoly     = 0;
  r_numpolyverts       = 0;

  InitOpenGL();
  R_InitAllocators();

  memset(hashtable, 0, sizeof(hashtable));
  R_SetColorMappings();
  R_CreateBuiltinImages();
  R_DeleteVertexPrograms();

  R_InitShaders();

  /* default model 0 (MOD_BAD placeholder) */
  tr_numModels = 0;
  model = (model_t *)ri.Hunk_Alloc( sizeof( model_t ) );
  model->index = tr_numModels;
  tr_models[tr_numModels++] = (int)model;
  model->type = MOD_BAD;

  rendererRegisteredFontCount = 0;
  rendererAsianFontLoaded = 0;
  R_SetHwLightGlobals();
  memset(&tr_lightVisCache, 0, 0x200000u);

  lightVisCache_maxAssociativity         = 0;
  lightVisCache_entriesUsed             = 0;
  lightVisCache_entriesFlushed          = 0;
  lightVisCache_entriesFilledAtRuntime  = 0;

  err = qglGetError_0();
  if ( err ){
    ri.Printf(0, "glGetError() = 0x%x\n", err);
  }

  R_InitDebug();

  xmodel_animCheck  = 0;
  s_numWaterMaps    = 0;

  FFT_Init();

  ri.Printf(0, "----- finished R_Init -----\n");
}

/* ---- R_DeleteFragmentShaders  0x004B4830 ----  [HIGH] */
int R_DeleteFragmentShaders()
{
  int result;
  int i;

  result = glConfig_ATIFragmentShader;
  if ( glConfig_ATIFragmentShader )
  {
    if ( glState_boundFragmentShader )
    {
      glBindFragmentShaderATI(0);
      glState_boundFragmentShader = 0;
    }
    result = tr_fragmentShaderCount;
    for ( i = 1; i <= tr_fragmentShaderCount; ++i )
    {
      qglDeleteFragmentShaderATI(i);
      result = tr_fragmentShaderCount;
    }
    tr_fragmentShaderCount = 0;
    glState_boundFragmentShader = 0;
  }
  return result;
}

/* ---- RE_Shutdown  0x004B4890 ----  [HIGH] */
void __cdecl RE_Shutdown(qboolean destroyWindow)
{
  ri.Printf(0, "RE_Shutdown( %i )\n", destroyWindow);
  ri.Cmd_RemoveCommand("modellist");
  ri.Cmd_RemoveCommand("screenshotJPEG");
  ri.Cmd_RemoveCommand("screenshot");
  ri.Cmd_RemoveCommand("imagelist");
  ri.Cmd_RemoveCommand("shaderlist");
  ri.Cmd_RemoveCommand("skinlist");
  ri.Cmd_RemoveCommand("gfxinfo");
  ri.Cmd_RemoveCommand("modelist");
  ri.Cmd_RemoveCommand("shaderstate");
  ri.Cmd_RemoveCommand("taginfo");
  ri.Cmd_RemoveCommand("cropimages");
  ri.Cmd_RemoveCommand("r_meminfo");
  ri.Cmd_RemoveCommand("r_vc_stats");
  ri.Cmd_RemoveCommand("r_smc_stats");
  ri.Cmd_RemoveCommand("r_smc_flush");
  ri.Cmd_RemoveCommand("r_loadsun");
  ri.Cmd_RemoveCommand("r_savesun");
  ri.Cmd_RemoveCommand("r_sunhelp");
  ri.Cmd_RemoveCommand("r_vbo_refresh");

  if ( tr_registered[0] )
  {
    R_DeleteTextures();
    R_DeleteVertexPrograms();
    R_DeleteFragmentShaders();
#ifdef COD1_VC7
    R_DeleteBuffersARB();    /* retail pushes nothing: the int parameter is dead */
#else
    R_DeleteBuffersARB(0);
#endif
  }
  R_SaveLightVisHistory();
  rendererRegisteredFontCount = 0;
  rendererAsianFontLoaded = 0;
  R_ShutdownAllocators();
  R_ShutdownStaticModels();
  if ( destroyWindow )
    GLimp_Shutdown();
  R_ShutdownDebug();
  tr_registered[0] = qfalse;
}

/* ---- RE_EndRegistration  0x004B49E0 ----  VERIFIED
 * Flush the registration command list to the backend (inlined
 * R_IssueRenderCommands: write RC_END_OF_LIST at cmds[used], reset used), then
 * dump the image list when the app has plenty of system memory (> 96 MB).  This
 * command buffer sits at backEndData + 0x18F700 with its used counter 0x40000
 * bytes past it; the raw offsets are kept verbatim. */
void __cdecl RE_EndRegistration( const char *label )
{
  R_SyncRenderThread();
  if ( sys_sysMBValue > 100663296 )   /* 0x6000000 = 96 MB */
    RB_ShowImages();
}

#define REF_API_VERSION 14

/* ---- GetRefAPI  0x004B4A30 ----  CONFIRMED
 * Copy the import table into ri, clear this renderer's export table (0x00CA26C8)
 * and fill it with the entry points, Q3's order of business.  Every name and
 * calling convention below is load-bearing (cross-file function pointers): keep
 * them exact. */
int *__cdecl GetRefAPI( int apiVersion, const void *rimp )
{
  static refexport_t re;   /* 0x00CA26C8 */

  ri = *(const refimport_t *)rimp;

  memset( &re, 0, sizeof( re ) );

  if ( apiVersion != REF_API_VERSION )
  {
    ri.Printf( 0, "Mismatched REF_API_VERSION: expected %i, got %i\n", REF_API_VERSION, apiVersion );
    return NULL;
  }

  re.Shutdown                 = (void (*)( int destroyWindow ))RE_Shutdown;
  re.BeginRegistration        = (void (*)( glconfig_t *glconfigOut ))RE_BeginRegistration;
  re.GetXModelByHandle        = (int (*)())RE_GetXModelByHandle;
  re.RegisterModel            = (int (*)( const char *name, int arg2 ))RE_RegisterModel;
  re.GetShaderFromModel       = (int (*)())RE_GetShaderFromModel;
  re.GetImageMemory           = (int (*)())RE_GetImageMemory;
  re.RegisterShader           = (int (*)( const char *name, int arg2 ))RE_RegisterShader;
  re.RegisterShaderNoMip      = (int (*)( const char *name, int arg2 ))RE_RegisterShaderNoMip;
  re.LoadWorldMap             = (int (*)())RE_LoadWorldMap;
  re.FinishLoadingModels      = (int (*)())RE_FinishLoadingModels;
  re.SetIgnorePrecacheErrors  = (int (*)())RE_SetIgnorePrecacheErrors;
  re.GetIgnorePrecacheErrors  = (int (*)())RE_GetIgnorePrecacheErrors;
  re.EndRegistration          = (int (*)())RE_EndRegistration;
  re.GetShaderName            = (int (*)())RE_GetShaderName;
  re.GetFarPlaneDist          = (int (*)())RE_GetFarPlaneDist;
  re.BeginFrame               = (void (*)( int stereoFrame ))RE_BeginFrame;
  re.EndFrame                 = (void (*)( int *frontEndMsec, int *backEndMsec ))RE_EndFrame;
  re.SaveScreen               = (int (*)())RE_SaveScreen;
  re.BlendSavedScreen         = (int (*)())RE_BlendSavedScreen;
  re.LerpTag                  = (int (*)())RE_MarkFragments;
  re.ModelBounds              = (void (*)( int model, float *mins, float *maxs ))R_ModelBounds;
  re.ClearScene               = (void (*)( void ))RE_ClearScene;
  re.AddRefEntityToScene      = (void (*)( const void *ent, int arg2 ))RE_AddRefEntityToScene;
  re.AddPolyToScene           = (void (*)( int hShader, int numVerts, const void *verts ))RE_AddPolyToScene;
  re.AddPolysToScene          = (void (*)( int hShader, int numVerts, const void *verts, int numPolys ))RE_AddPolysToScene;
  re.AddLightToScene          = (void (*)( const float *org, float intensity, float r, float g, float b ))RE_AddLightToScene;
  re.AddCoronaToScene         = (void (*)( const float *org, float r, float g, float b, float scale, int id, int visible ))RE_AddCoronaToScene;
  re.SetFarPlaneDist          = (int (*)())RE_SetFarPlaneDist;   /* retail 0x004B4B78 */
  re.SetFog                   = (int (*)())R_SetFog;
  re.SaveFogState             = (int (*)())RE_SaveFogState;
  re.RestoreFogState          = (int (*)())RE_RestoreFogState;
  re.RenderScene              = (void (*)( const void *fd ))RE_RenderScene;
  re.ClearFlares              = (int (*)())RE_ClearFlares;
  re.SetColor                 = (void (*)( const float *rgba ))RE_SetColor;
  re.StretchPic               = (void (*)( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader ))RE_StretchPic;
  re.StretchPicGradient       = (int (*)())RE_StretchPicGradient;
  re.StretchPicRotate         = (int (*)())RE_StretchPicRotate;
  re.DrawQuadPic              = (int (*)())RE_DrawQuadPic;
  re.StretchRaw               = (int (*)())RE_StretchRaw;
  re.UploadCinematic          = (int (*)())RE_UploadCinematic;
  re.RegisterFont             = (int (*)())RE_RegisterFont;
  re.GetEntityToken           = (int (*)())R_GetEntityToken;
  re.ResetImageAllocations    = (int (*)())R_ResetImageAllocations;
  re.FreeImageAllocations     = (int (*)())R_FreeImageAllocations;
  re.CubemapShot              = (int (*)())RE_CubemapShot;
  re.CubemapWaterShot         = (int (*)())RE_CubemapWaterShot;
  re.LocateDebugStrings       = (int (*)())RE_LocateDebugStrings;
  re.LocateDebugLines         = (int (*)())RE_LocateDebugLines;
  re.AddPlume                 = (int (*)())RE_AddPlume;
  re.TrackStatistics          = (int (*)())RE_TrackStatistics;
  re.PickShader               = (int (*)())RE_PickShader;
  re.Text_Width               = (int (*)())RE_Text_Width;
  re.Text_Height              = (int (*)())RE_Text_Height;
  re.Text_Paint               = (int (*)())RE_Text_Paint;
  re.Text_ConsoleWidth        = (int (*)())RE_Text_ConsoleWidth;
  re.Text_ConsolePaint        = (int (*)())RE_Text_ConsolePaint;
  re.Text_PaintWithCursor     = (int (*)())RE_Text_PaintWithCursor;
  return (int *)&re;
}

