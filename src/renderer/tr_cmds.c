/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_shaderregistry.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"

/* tr_shader.c (just before surfacetype_bark): the 24 surface-type tr_infoParm_t records; read here
   four ints at a time for the name */
extern int cl_surfaceParmsByType[96];
/* ---- renderer-owned globals ---- */


void CM_Trace( trace_t *results, const float *start, const float *end,
               const float *mins, const float *maxs, int model,
               const float *origin, int brushmask, int capsule,
               const void *sphere );
float AngleMod( float a );
extern int GL_TextureMode();
extern void __cdecl RB_ExecuteRenderCommands(const void *data);
extern int R_IncrementalRefreshOptimizedWorldSurfaces_ARB();
extern int R_IncrementalRefreshStaticModels_ARB();
extern int R_IncrementalRefreshXModels_ARB();
void R_SetColorMappings( void );
extern int R_SetHwLightGlobals();
void R_SetNVFogMode( void );
extern void R_SumOfUsedImages();

static const float colorWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

/* Color byte scale: retail multiplies by the float 255.0f (fmul dword), which the
 * x87 keeps unrounded; the modern build uses a double so its product is exact too. */
#ifdef COD1_VC7
#define R_COLOR_SCALE 255.0f
#else
#define R_COLOR_SCALE 255.0
#endif

/* ------------------------------------------------------------------------- *
 *  Render command buffer.
 *
 *  backEndData holds the base address of the SMP backend data block.  The
 *  render command list (RTCW renderCommandList_t) lives at fixed offsets inside
 *  it: the byte command buffer (commands.cmds) at +0x18F700 and the used
 *  counter (commands.used) at +0x1CF700, which lands immediately past the
 *  0x40000-byte buffer -- so the retail tree also reaches `used` as
 *  cmds[0x10000].  A command is bump-allocated by advancing `used`; the final 8
 *  bytes stay reserved so an RC_END_OF_LIST marker always fits.  Retail inlines
 *  R_GetCommandBuffer into every RE_* producer below, so each keeps the same
 *  allocate-then-fill shape, including the (always true) NULL check on the
 *  returned pointer.
 * ------------------------------------------------------------------------- */
#define R_CMD_BUFFER          (backEndData + 0x18F700)           /* commands.cmds  */
#define R_CMD_USED            (*(int *)(backEndData + 0x1CF700)) /* commands.used  */
#define R_CMD_USABLE_BYTES    0x3FFF8                            /* 0x40000 - 8    */

/* The list as R_GetCommandBuffer sees it; the producers address it the same
 * way so the inlined allocation keeps retail's register shape. */
typedef struct {
    byte         cmds[R_CMD_USABLE_BYTES + 8];
    unsigned int used;
} renderCommandList_t;
#define R_CMD_LIST            ((renderCommandList_t *)R_CMD_BUFFER)

typedef enum {
    RC_END_OF_LIST          = 0,
    RC_SET_COLOR            = 1,
    RC_STRETCH_PIC          = 2,
    RC_STRETCH_PIC_GRADIENT = 3,
    RC_STRETCH_PIC_ROTATE   = 4,
    RC_DRAW_QUAD_PIC        = 5,
    RC_TEXT_PAINT           = 6,     /* RC_TEXT_PAINT_WITH_CURSOR */
    RC_DRAW_SURFS           = 7,
    RC_DRAW_BUFFER          = 8,
    RC_SAVE_SCREEN          = 9,
    RC_BLEND_SAVED_SCREEN   = 10,
    RC_SWAP_BUFFERS         = 11
} renderCommand_t;

/* The 2D/pic commands store their color as packed RGBA bytes; the geometry
 * fields are floats read back verbatim by the backend (RB_DrawStretchPic ...). */
typedef struct {
    int   commandId;              /* +0x00 */
    byte  color[4];               /* +0x04 */
} setColorCommand_t;
COD1_ASSERT_SIZE(setColorCommand_t, 8);

typedef struct {
    int       commandId;          /* +0x00 */
    shader_t *shader;             /* +0x04 */
    float     x, y, w, h;         /* +0x08 */
    float     s1, t1, s2, t2;     /* +0x18 */
} stretchPicCommand_t;
COD1_ASSERT_SIZE(stretchPicCommand_t, 40);

typedef struct {
    int       commandId;          /* +0x00 */
    shader_t *shader;             /* +0x04 */
    float     x, y, w, h;         /* +0x08 */
    float     s1, t1, s2, t2;     /* +0x18 */
    byte      gradientColor[4];   /* +0x28 */
    int       gradientType;       /* +0x2c */
} stretchPicGradientCommand_t;
COD1_ASSERT_SIZE(stretchPicGradientCommand_t, 48);

typedef struct {
    int       commandId;          /* +0x00 */
    shader_t *shader;             /* +0x04 */
    float     x, y, w, h;         /* +0x08 */
    float     s1, t1, s2, t2;     /* +0x18 */
    float     angle;              /* +0x28  radians (stored after a 16-bit round-trip) */
} stretchPicRotateCommand_t;
COD1_ASSERT_SIZE(stretchPicRotateCommand_t, 44);

typedef struct {
    int       commandId;          /* +0x00 */
    shader_t *shader;             /* +0x04 */
    float     positions[4][2];    /* +0x08 */
    float     texCoords[4][2];    /* +0x28 */
} drawQuadPicCommand_t;
COD1_ASSERT_SIZE(drawQuadPicCommand_t, 72);

/* Captures the whole frame's refdef + viewParms so the backend can render the
 * surface list independent of the front end.  Both sub-records are copied as
 * blobs from tr.refdef / tr.viewParms; only their sizes matter here. */
typedef struct {
    int         commandId;        /* +0x000 */
    byte        refdef[0x188];    /* +0x004  copy of tr.refdef (trRefdef_t, 0x188) */
    viewParms_t viewParms;        /* +0x18c  copy of tr.viewParms */
    drawSurf_t *drawSurfs;        /* +0x3ec */
    int         numDrawSurfs;     /* +0x3f0 */
} drawSurfsCommand_t;
COD1_ASSERT_SIZE(drawSurfsCommand_t, 1012);
ASSERT_OFF(drawSurfsCommand_t, viewParms, 0x18c);
ASSERT_OFF(drawSurfsCommand_t, drawSurfs, 0x3ec);
ASSERT_OFF(drawSurfsCommand_t, numDrawSurfs, 0x3f0);

typedef struct {
    int commandId;                /* +0x00 */
    int buffer;                   /* +0x04  GL draw-buffer enum */
} drawBufferCommand_t;
COD1_ASSERT_SIZE(drawBufferCommand_t, 8);

typedef struct {
    int commandId;                /* +0x00 */
} saveScreenCommand_t;
COD1_ASSERT_SIZE(saveScreenCommand_t, 4);

typedef struct {
    int commandId;                /* +0x00 */
    int duration;                 /* +0x04 */
} blendSavedScreenCommand_t;
COD1_ASSERT_SIZE(blendSavedScreenCommand_t, 8);

typedef struct {
    int commandId;                /* +0x00 */
} swapBuffersCommand_t;
COD1_ASSERT_SIZE(swapBuffersCommand_t, 4);

/* Client-supplied per-frame statistics sink (RE_TrackStatistics; CoDUO
 * renderer_frame_statistics_t).  tr_performanceCounters holds the pointer;
 * only the fields R_PerformanceCounters writes are named. */
typedef struct {
    int   indexCount;             /* +0x00 */
    int   drawnIndexCount;        /* +0x04 */
    int   vertexCount;            /* +0x08 */
    int   drawCallCount;          /* +0x0c */
    int   imageMemory;            /* +0x10 */
    int   lightmapMemory;         /* +0x14 */
    int   textureMemory;          /* +0x18 */
    int   entityCount;            /* +0x1c */
    float overdrawRatio;          /* +0x20 */
} renderStatistics_t;
ASSERT_OFF(renderStatistics_t, entityCount, 0x1c);
ASSERT_OFF(renderStatistics_t, overdrawRatio, 0x20);

/* ---- R_PerformanceCounters  0x004DD920 ----  VERIFIED
 * Publish the client statistics sink, print the r_speeds report, then clear the
 * front-end (tr.pc) and back-end (backEnd.pc) counters for the next frame.  The
 * qword_* counters pack two int32 stats into 8 bytes each.  Retail clears both
 * blocks with two memsets (rep stosd 9 / 13 dwords); every field of tr.pc and
 * backEnd.pc is now a view of the TRG / BACKEND blobs, so the same memsets work. */
void R_PerformanceCounters( void )
{
  renderStatistics_t *stats;
  int                 imageMemory, lightmapMemory, textureMemory;

  if ( com_statmon->integer )
    R_SumOfUsedImages( 0, 0, 0 );

  if ( tr_performanceCounters )
  {
    ((renderStatistics_t *)tr_performanceCounters)->indexCount      = backEnd_pc_indexCount;
    ((renderStatistics_t *)tr_performanceCounters)->drawnIndexCount = backEnd_pc_drawnIndexCount;
    ((renderStatistics_t *)tr_performanceCounters)->vertexCount     = backEnd_pc_vertexCount;
    ((renderStatistics_t *)tr_performanceCounters)->drawCallCount   = backEnd_pc_drawCallCount;
    R_SumOfUsedImages( &((renderStatistics_t *)tr_performanceCounters)->imageMemory, &((renderStatistics_t *)tr_performanceCounters)->lightmapMemory, &((renderStatistics_t *)tr_performanceCounters)->textureMemory );
    ((renderStatistics_t *)tr_performanceCounters)->overdrawRatio   = backEnd_pc_overdrawSum / (rfloat)(int)(glConfig.vidWidth * glConfig.vidHeight);
    ((renderStatistics_t *)tr_performanceCounters)->entityCount     = r_numentities;
  }

  if ( !r_speeds->integer )
  {
    /* clear the counters even if we aren't printing (VC7 merges this tail with the one below) */
    memset( &tr_pc_patchSphereCullInClip, 0, 0x24 );
    memset( &backEnd_pc_surfaceCount, 0, 0x34 );
    return;
  }

  if ( r_speeds->integer == 1 )
  {
    R_SumOfUsedImages( &imageMemory, &lightmapMemory, &textureMemory );
    ri.Printf(
      0,
      "%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f/%.2f/%.2f MB %.2f dc\n",
      backEnd_pc_shaderCount,
      backEnd_pc_surfaceCount,
      tr_pc_leafCount,
      backEnd_pc_vertexCount,
      backEnd_pc_indexCount / 3,
      backEnd_pc_drawnIndexCount / 3,
      (rfloat)textureMemory  * 0.00000095367431640625f,
      (rfloat)(imageMemory - lightmapMemory) * 0.00000095367431640625f,
      (rfloat)imageMemory    * 0.00000095367431640625f,
      backEnd_pc_overdrawSum / (rfloat)(int)(glConfig.vidWidth * glConfig.vidHeight) );
  }
  else if ( r_speeds->integer == 2 )
  {
    ri.Printf(
      0,
      "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
      ((int *)&tr_pc_patchSphereCullInClip)[0],
      ((int *)&tr_pc_patchSphereCullInClip)[1],
      ((int *)&tr_pc_patchSphereCullOutBoxCullIn)[0],
      ((int *)&tr_pc_patchSphereCullOutBoxCullIn)[1],
      ((int *)&tr_pc_patchBoxCullClipOut)[0],
      ((int *)&tr_pc_patchBoxCullClipOut)[1] );
  }
  else if ( r_speeds->integer == 3 )
  {
    ri.Printf( 0, "viewcluster: %i\n", tr_viewCluster );
  }
  else if ( r_speeds->integer == 4 )
  {
    if ( backEnd_pc_dlightVertexCount )
      ri.Printf(
        0,
        "dlight srf:%i  culled:%i  verts:%i  tris:%i\n",
        ((int *)&tr_pc_dlightSurfaceAndCullCount)[0],
        ((int *)&tr_pc_dlightSurfaceAndCullCount)[1],
        backEnd_pc_dlightVertexCount,
        backEnd_pc_dlightIndexCount / 3 );
  }
  else if ( r_speeds->integer == 6 )
  {
    ri.Printf(
      0,
      "flare adds:%i tests:%i renders:%i\n",
      backEnd_pc_flareAddCount,
      ((int *)&backEnd_pc_flareTests)[0],
      ((int *)&backEnd_pc_flareTests)[1] );
  }

  memset( &tr_pc_patchSphereCullInClip, 0, 0x24 );   /* memset( &tr.pc, 0, sizeof( tr.pc ) ): 0x016C57EC, 9 dwords */
  memset( &backEnd_pc_surfaceCount, 0, 0x34 );       /* memset( &backEnd.pc, 0, sizeof( backEnd.pc ) ): 0x016D8E24, 13 dwords */
}

/* ---- R_IssueRenderCommands  0x004DDBA0 ----  VERIFIED
 * Terminate and hand the current command list to the backend (non-SMP).
 * Retail's RB_ExecuteRenderCommands takes only cmds, in eax. */
void __cdecl R_IssueRenderCommands( int runPerformanceCounters )
{
  renderCommandList_t *cmdList;

  cmdList = R_CMD_LIST;
  *(int *)( cmdList->cmds + cmdList->used ) = RC_END_OF_LIST;
  cmdList->used = 0;
  if ( runPerformanceCounters )
    R_PerformanceCounters();
  if ( !r_skipBackEnd->integer )
    RB_ExecuteRenderCommands( cmdList->cmds );
}

/* ---- R_SyncRenderThread  0x004DDBF0 ----  VERIFIED
 * Flush any pending commands and (non-SMP) run them immediately. */
void __cdecl R_SyncRenderThread( void )
{
  if ( !tr_registered[0] )
    return;
  R_IssueRenderCommands( qfalse );
}

/* ---- R_GetCommandBuffer  0x004DDC30 ----  VERIFIED
 * Reserve `bytes` bytes in the command buffer, dropping the command (returning 0)
 * when it would not fit.  The byte count arrives in bytes; a1 is unused. */
unsigned int __cdecl R_GetCommandBuffer( unsigned int bytes )
{
  renderCommandList_t *cmdList;

  cmdList = R_CMD_LIST;
  if ( cmdList->used + bytes > R_CMD_USABLE_BYTES )
  {
    if ( bytes > R_CMD_USABLE_BYTES )
      ri.Error( 0, "\x15" "R_GetCommandBuffer: bad size %i", bytes );
    return 0;
  }
  cmdList->used += bytes;
  return (unsigned int)( cmdList->cmds + cmdList->used - bytes );
}

/* ---- R_AddDrawSurfCmd  0x004DDC80 ----  VERIFIED
 * Queue the sorted draw-surface list along with a snapshot of the refdef and
 * view.  The return value is dead EAX (the command pointer, or the would-be
 * used offset when the buffer is full); every caller ignores it. */
unsigned int *__cdecl R_AddDrawSurfCmd( int numDrawSurfs, int drawSurfs )
{
  renderCommandList_t *cmdList;
  drawSurfsCommand_t  *cmd;
  unsigned int         newUsed;
  unsigned int              *result;

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( drawSurfsCommand_t );
  result  = (unsigned int *)newUsed;
  if ( newUsed <= R_CMD_USABLE_BYTES )
  {
    cmdList->used = newUsed;
    cmd    = (drawSurfsCommand_t *)( cmdList->cmds + cmdList->used - sizeof( drawSurfsCommand_t ) );
    result = (unsigned int *)cmd;
    if ( cmd )
    {
      cmd->commandId    = RC_DRAW_SURFS;
      cmd->drawSurfs    = (drawSurf_t *)drawSurfs;
      cmd->numDrawSurfs = numDrawSurfs;
      memcpy( &cmd->refdef,    &tr_refdef_x,          0x188u );
      memcpy( &cmd->viewParms, &tr_viewParms_originX, 0x260u );
    }
  }
  return result;
}

/* ---- RE_SetColor  0x004DDCF0 ----  VERIFIED
 * Set the 2D draw color; a NULL rgba selects white.  Dead EAX = ftol of the
 * alpha byte. */
int __cdecl RE_SetColor( const float *rgba )
{
  renderCommandList_t *cmdList;
  setColorCommand_t   *cmd;
  const float         *srcColor;
  unsigned int         newUsed;
  int                  ret;

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( setColorCommand_t );
  if ( newUsed > R_CMD_USABLE_BYTES )
    return (int)newUsed;
  cmdList->used = newUsed;
  cmd = (setColorCommand_t *)( cmdList->cmds + cmdList->used - sizeof( setColorCommand_t ) );
  if ( !cmd )
    return (int)newUsed;
  cmd->commandId = RC_SET_COLOR;
  srcColor = rgba ? rgba : colorWhite;
  cmd->color[0] = (byte)( srcColor[0] * R_COLOR_SCALE );
  cmd->color[1] = (byte)( srcColor[1] * R_COLOR_SCALE );
  cmd->color[2] = (byte)( srcColor[2] * R_COLOR_SCALE );
  /* retail 0x4DDD67: __ftol2 (64-bit truncation, low dword), not __ftol2_sse. */
  ret = (int)(__int64)( srcColor[3] * R_COLOR_SCALE );
  cmd->color[3] = (byte)ret;
  return ret;
}

/* R_GetShaderByHandle (tr_shader.c), which retail inlines into each pic
 * producer; this copy reproduces retail's block order there. */
static __inline shader_t *R_CmdShaderByHandle( int hShader )
{
  if ( hShader < 0 )
  {
    ri.Printf( 2, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
    return (shader_t *)tr_defaultShader;
  }
  if ( hShader >= tr_numShaders[0] )
  {
    ri.Printf( 2, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
    return (shader_t *)tr_defaultShader;
  }
  return (shader_t *)tr_shaders[hShader];
}

/* ---- RE_StretchPic  0x004DDD80 ----  VERIFIED
 * Dead EAX = the raw bits of s2 (a void producer in RTCW/CoDUO). */
unsigned int __cdecl RE_StretchPic( float x, float y, float w, float h,
                                    float s1, float t1, float s2, float t2, int hShader )
{
  renderCommandList_t *cmdList;
  stretchPicCommand_t *cmd;
  unsigned int         newUsed;

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( stretchPicCommand_t );
  if ( newUsed > R_CMD_USABLE_BYTES )
    return newUsed;
  cmdList->used = newUsed;
  cmd = (stretchPicCommand_t *)( cmdList->cmds + cmdList->used - sizeof( stretchPicCommand_t ) );
  if ( !cmd )
    return newUsed;
  cmd->commandId = RC_STRETCH_PIC;
  cmd->shader = R_CmdShaderByHandle( hShader );
  cmd->x  = x;
  cmd->y  = y;
  cmd->w  = w;
  cmd->h  = h;
  cmd->s1 = s1;
  cmd->t1 = t1;
  cmd->s2 = s2;
  cmd->t2 = t2;
  return *(unsigned int *)&s2;
}

/* ---- RE_StretchPicGradient  0x004DDE20 ----  VERIFIED
 * Dead EAX = ftol of the gradient alpha byte. */
int __cdecl RE_StretchPicGradient( float x, float y, float w, float h,
                                   float s1, float t1, float s2, float t2,
                                   int hShader, const float *gradientColor, int gradientType )
{
  renderCommandList_t         *cmdList;
  stretchPicGradientCommand_t *cmd;
  const float                 *srcColor;
  unsigned int                 newUsed;
  int                          ret;

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( stretchPicGradientCommand_t );
  if ( newUsed > R_CMD_USABLE_BYTES )
    return (int)newUsed;
  cmdList->used = newUsed;
  cmd = (stretchPicGradientCommand_t *)( cmdList->cmds + cmdList->used - sizeof( stretchPicGradientCommand_t ) );
  if ( !cmd )
    return (int)newUsed;
  cmd->commandId = RC_STRETCH_PIC_GRADIENT;
  cmd->shader = R_CmdShaderByHandle( hShader );
  cmd->x  = x;
  cmd->y  = y;
  cmd->w  = w;
  cmd->h  = h;
  cmd->s1 = s1;
  cmd->t1 = t1;
  cmd->s2 = s2;
  cmd->t2 = t2;
  srcColor = gradientColor ? gradientColor : colorWhite;
  cmd->gradientColor[0] = (byte)( srcColor[0] * R_COLOR_SCALE );
  cmd->gradientColor[1] = (byte)( srcColor[1] * R_COLOR_SCALE );
  cmd->gradientColor[2] = (byte)( srcColor[2] * R_COLOR_SCALE );
  /* retail 0x4DDF09: __ftol2 (64-bit truncation, low dword); a plain (int)
   * cast becomes __ftol2_sse, which gives 0x80000000 at |v| >= 2^31. */
  ret = (int)(__int64)( srcColor[3] * R_COLOR_SCALE );
  cmd->gradientColor[3] = (byte)ret;
  cmd->gradientType = gradientType;
  return ret;
}

/* ---- RE_StretchPicRotate  0x004DDF20 ----  VERIFIED
 * `angle` goes through AngleMod (com_math.c, /Op, inlined by LTCG): quantized to
 * 16-bit units (65536/360) and converted back to degrees. */
void __cdecl RE_StretchPicRotate( float x, float y, float w, float h,
                                  float s1, float t1, float s2, float t2,
                                  float angle, int hShader )
{
  renderCommandList_t       *cmdList;
  stretchPicRotateCommand_t *cmd;
  unsigned int               newUsed;
#ifndef COD1_VC7
  unsigned int               angleUnits;
#endif

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( stretchPicRotateCommand_t );
  if ( newUsed > R_CMD_USABLE_BYTES )
    return;
  cmdList->used = newUsed;
  cmd = (stretchPicRotateCommand_t *)( cmdList->cmds + cmdList->used - sizeof( stretchPicRotateCommand_t ) );
  if ( !cmd )
    return;
  cmd->commandId = RC_STRETCH_PIC_ROTATE;
  cmd->shader = R_CmdShaderByHandle( hShader );
  cmd->x  = x;
  cmd->y  = y;
  cmd->w  = w;
  cmd->h  = h;
  cmd->s1 = s1;
  cmd->t1 = t1;
  cmd->s2 = s2;
  cmd->t2 = t2;
#ifdef COD1_VC7
  cmd->angle = AngleMod( angle );
#else
  /* modern AngleMod's (int) is cvttsd2si (0x80000000 out of range); retail's __ftol2 wraps the 64-bit value */
  angleUnits = (unsigned int)(__int64)( (rfloat)angle * 182.04444885253906f ) & 0xFFFF;
  cmd->angle = (float)(int)angleUnits * 0.0054931640625f;
#endif
}

/* ---- RE_DrawQuadPic  0x004DE000 ----  VERIFIED
 * Dead EAX = the raw bits of texCoords[3][1]. */
unsigned int __cdecl RE_DrawQuadPic( const float (*positions)[2], const float (*texCoords)[2], int hShader )
{
  renderCommandList_t  *cmdList;
  drawQuadPicCommand_t *cmd;
  unsigned int          newUsed;
  int                   i;

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( drawQuadPicCommand_t );
  if ( newUsed > R_CMD_USABLE_BYTES )
    return newUsed;
  cmdList->used = newUsed;
  cmd = (drawQuadPicCommand_t *)( cmdList->cmds + cmdList->used - sizeof( drawQuadPicCommand_t ) );
  if ( !cmd )
    return newUsed;
  cmd->commandId = RC_DRAW_QUAD_PIC;
  cmd->shader = R_CmdShaderByHandle( hShader );
  for ( i = 0; i < 4; ++i )
  {
    cmd->positions[i][0] = positions[i][0];
    cmd->positions[i][1] = positions[i][1];
    cmd->texCoords[i][0] = texCoords[i][0];
    cmd->texCoords[i][1] = texCoords[i][1];
  }
  return *(unsigned int *)&cmd->texCoords[3][1];
}

/* ---- RE_BeginFrame  0x004DE0D0 ----  VERIFIED */
void __cdecl RE_BeginFrame( int stereoFrame )
{
  renderCommandList_t *cmdList;
  drawBufferCommand_t *cmd;
  unsigned int         newUsed;
  int                  err;

  if ( !tr_registered[0] )
    return;

  glState_finishCalled = 0;
  ++tr_frameCount;
  tr_frameSceneNum = 0;

  if ( r_measureOverdraw->integer )
  {
    if ( glConfig_stencilBits < 4 )
    {
      ri.Printf(0, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig_stencilBits);
      ri.Cvar_Set("r_measureOverdraw", "0");
      r_measureOverdraw->modified = 0;
    }
    else if ( cg_shadows->integer == 2 )
    {
      ri.Printf(0, "Warning: stencil shadows and overdraw measurement are mutually exclusive\n");
      ri.Cvar_Set("r_measureOverdraw", "0");
      r_measureOverdraw->modified = 0;
    }
    else
    {
      R_SyncRenderThread();
      qglEnable(0xB90u);
      qglStencilMask(0xFFFFFFFFu);
      qglClearStencil(0);
      qglStencilFunc(0x207u, 0, 0xFFFFFFFFu);
      qglStencilOp(0x1E00u, 0x1E02u, 0x1E02u);
    }
    r_measureOverdraw->modified = 0;
  }
  else
  {
    if ( r_measureOverdraw->modified )
    {
      R_SyncRenderThread();
      qglDisable(0xB90u);
    }
    r_measureOverdraw->modified = 0;
  }

  if ( r_textureMode->modified )
  {
    R_SyncRenderThread();
    GL_TextureMode((int)r_textureMode->string);
    r_textureMode->modified = 0;
  }

  R_SetHwLightGlobals();

  if ( qglPNTrianglesiATI )
  {
    if ( r_ati_truform_tess->modified )
    {
      r_ati_truform_tess->modified = 0;
      if ( r_ati_truform_tess->value > (rfloat)glConfig_maxPNTrianglesTessellationLevel )
        ri.Cvar_Set("r_ati_truform_tess", va("%d", glConfig_maxPNTrianglesTessellationLevel));
      /* 0x87F4 GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI */
      qglPNTrianglesiATI(0x87F4u, r_ati_truform_tess->integer);
    }

    if ( r_ati_truform_pointmode->modified )
    {
      r_ati_truform_pointmode->modified = 0;
      if ( !Q_stricmp(r_ati_truform_pointmode->string, "LINEAR") )
      {
        glConfig_pnTrianglesPointMode = 0x87F5;
      }
      else if ( !Q_stricmp(r_ati_truform_pointmode->string, "CUBIC") )
      {
        glConfig_pnTrianglesPointMode = 0x87F6;
      }
      else
      {
        glConfig_pnTrianglesPointMode = 0x87F6;
        ri.Cvar_Set("r_ati_truform_pointmode", "LINEAR");
      }
      qglPNTrianglesiATI(0x87F2u, glConfig_pnTrianglesPointMode);
    }

    if ( r_ati_truform_normalmode->modified )
    {
      r_ati_truform_normalmode->modified = 0;
      if ( !Q_stricmp(r_ati_truform_normalmode->string, "LINEAR") )
      {
        glConfig_pnTrianglesNormalMode = 0x87F7;
      }
      else if ( !Q_stricmp(r_ati_truform_normalmode->string, "QUADRATIC") )
      {
        glConfig_pnTrianglesNormalMode = 0x87F8;
      }
      else
      {
        glConfig_pnTrianglesNormalMode = 0x87F7;
        ri.Cvar_Set("r_ati_truform_normalmode", "LINEAR");
      }
      qglPNTrianglesiATI(0x87F3u, glConfig_pnTrianglesNormalMode);
    }
  }

  if ( glConfig_NVFogAvailable && r_nv_fogdist_mode->modified )
  {
    r_nv_fogdist_mode->modified = 0;
    R_SetNVFogMode();
  }

  if ( r_gamma->modified )
  {
    r_gamma->modified = 0;
    R_SyncRenderThread();
    R_SetColorMappings();
  }

  if ( glConfig_ARBVertexBufferObject && r_vbo_paranoia->integer )
  {
    R_IncrementalRefreshOptimizedWorldSurfaces_ARB();
    R_IncrementalRefreshStaticModels_ARB(3);
    R_IncrementalRefreshXModels_ARB(3);
  }

  if ( !r_ignoreGLErrors->integer )
  {
    R_SyncRenderThread();
    err = qglGetError();
    if ( err )
      ri.Error(0, "\x15" "RE_BeginFrame() - glGetError() failed (0x%x)!\n", err);
  }

  /* draw-buffer command */
  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( drawBufferCommand_t );
  if ( newUsed > R_CMD_USABLE_BYTES )
    return;
  cmdList->used = newUsed;
  cmd = (drawBufferCommand_t *)( cmdList->cmds + cmdList->used - sizeof( drawBufferCommand_t ) );
  if ( !cmd )                          /* inlined R_GetCommandBuffer NULL check (0x004DE415) */
    return;

  cmd->commandId = RC_DRAW_BUFFER;

  if ( glConfig_stereoEnabled )
  {
    if ( stereoFrame == 1 )
      cmd->buffer = 0x402;             /* GL_BACK_LEFT */
    else if ( stereoFrame == 2 )
      cmd->buffer = 0x403;             /* GL_BACK_RIGHT */
    else
      ri.Error(0, "\x15" "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame);
  }
  else
  {
    if ( stereoFrame )
      ri.Error(0, "\x15" "RE_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame);
    if ( !Q_stricmp(r_drawBuffer->string, "GL_FRONT") )
      cmd->buffer = 0x404;             /* GL_FRONT */
    else
      cmd->buffer = 0x405;             /* GL_BACK */
  }
}

/* ---- RE_EndFrame  0x004DE4B0 ----  VERIFIED
 * Emit RC_SWAP_BUFFERS, flush the list (inlined R_IssueRenderCommands), reset
 * this frame's scene builders (inlined R_ToggleSmpFrame) and report the
 * front/back-end msec.  Retail is RE_EndFrame( int *frontEndMsec, int
 * *backEndMsec ) and passes RB_ExecuteRenderCommands only cmds (in eax); the
 * leading a1 here is a tree-only parameter (callers pass NULL), kept until the
 * re.EndFrame callers and RB_ExecuteRenderCommands drop it together. */
void __cdecl RE_EndFrame( unsigned int *frontEndMsec, unsigned int *backEndMsec )
{
  if ( !tr_registered[0] )
    return;

  *(int *)( R_CMD_BUFFER + R_CMD_USED ) = RC_SWAP_BUFFERS;
  R_CMD_USED += sizeof( swapBuffersCommand_t );

  R_IssueRenderCommands( qtrue );

  R_CMD_USED = 0;
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

  if ( frontEndMsec )
    *frontEndMsec = tr_frontEndMsec;
  tr_frontEndMsec = 0;
  if ( backEndMsec )
    *backEndMsec = backEnd_pc_msec;
  backEnd_pc_msec = 0;
}

/* ---- RE_SaveScreen  0x004DE590 ----  VERIFIED */
unsigned int *RE_SaveScreen( void )
{
  renderCommandList_t *cmdList;
  saveScreenCommand_t *cmd;
  unsigned int         newUsed;
  unsigned int              *result;

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( saveScreenCommand_t );
  result  = (unsigned int *)newUsed;
  if ( newUsed <= R_CMD_USABLE_BYTES )
  {
    cmdList->used = newUsed;
    cmd    = (saveScreenCommand_t *)( cmdList->cmds + cmdList->used - sizeof( saveScreenCommand_t ) );
    result = (unsigned int *)cmd;
    if ( cmd )
      cmd->commandId = RC_SAVE_SCREEN;
  }
  return result;
}

/* ---- RE_BlendSavedScreen  0x004DE5C0 ----  VERIFIED */
void __cdecl RE_BlendSavedScreen( int duration )
{
  renderCommandList_t       *cmdList;
  blendSavedScreenCommand_t *cmd;
  unsigned int               newUsed;

  if ( duration <= 0 )
    return;

  cmdList = R_CMD_LIST;
  newUsed = cmdList->used + sizeof( blendSavedScreenCommand_t );
  if ( newUsed <= R_CMD_USABLE_BYTES )
  {
    cmdList->used = newUsed;
    cmd = (blendSavedScreenCommand_t *)( cmdList->cmds + cmdList->used - sizeof( blendSavedScreenCommand_t ) );
    if ( cmd )
    {
      cmd->commandId = RC_BLEND_SAVED_SCREEN;
      cmd->duration  = duration;
    }
  }
}

/* ---- RE_TrackStatistics  0x004DE600 ----  VERIFIED */
int __cdecl RE_TrackStatistics( int statistics )
{
  tr_performanceCounters = statistics;
  return statistics;
}

#define tr_surfaceParms   ((const tr_infoParm_t *)(void *)&surfacetype_bark)
#define SURFACEPARM_FIRST 22    /* 0x00571900 = surfacetype_bark + 22*16 */

/* ---- RE_PickShader  0x004DE610 ----  VERIFIED */
int __cdecl RE_PickShader( float *start, float *dir, char *shaderName,
                           char *surfaceNames, char *contentsNames, size_t size )
{
  trace_t trace;
  float   end[3];
  const tr_infoParm_t *parm;
  int     surfaceType;
  size_t  surfaceLen;
  size_t  contentsLen;

  end[0] = (rfloat)dir[0] * 262144.0f + start[0];
  trace.fraction = 1.0;
  end[1] = (rfloat)dir[1] * 262144.0f + start[1];
  end[2] = (rfloat)dir[2] * 262144.0f + start[2];

  CM_Trace( &trace, start, end, 0, 0, 0, vec3_origin, 260308983, 0, 0 );

  if ( trace.startsolid || trace.allsolid )
    return 0;
  if ( trace.fraction == 1.0f )
    return 0;
  if ( !trace.material )
    return 0;

  /* +0x24 holds the shader NAME POINTER in 1.1. */
  strcpy( shaderName, (const char *)trace.material );

  surfaceNames[0] = 0;
  surfaceNames[size - 1] = 0;
  contentsNames[0] = 0;
  contentsNames[size - 1] = 0;

  surfaceType = ( trace.surfaceFlags >> 20 ) & 0x1F;
  if ( surfaceType > 0 && surfaceType < 23 )
    strncpy( surfaceNames, (const char *)cl_surfaceParmsByType[4 * surfaceType], size );
  else
    strncpy( surfaceNames, "^1default^7", size );

  if ( surfaceNames[size - 1] )
    return 0;
  surfaceLen = strlen( surfaceNames );

  if ( ( trace.contents & 1 ) != 0 )
    strncpy( contentsNames, "solid", size );
  else
    strncpy( contentsNames, "^3nonsolid^7", size );

  if ( contentsNames[size - 1] )
    return 0;
  contentsLen = strlen( contentsNames );

  for ( parm = &tr_surfaceParms[SURFACEPARM_FIRST]; parm->name; parm++ )
  {
    if ( ( trace.surfaceFlags & parm->surfaceFlags ) != 0 )
    {
      surfaceNames[surfaceLen] = ' ';
      surfaceLen++;
      strncpy( surfaceNames + surfaceLen, parm->name, size - surfaceLen );
      if ( surfaceNames[size - 1] )
        return 0;
      surfaceLen += strlen( surfaceNames + surfaceLen );
    }

    if ( ( trace.contents & parm->contents ) != 0 )
    {
      contentsNames[contentsLen] = ' ';
      contentsLen++;
      strncpy( contentsNames + contentsLen, parm->name, size - contentsLen );
      if ( contentsNames[size - 1] )
        return 0;
      contentsLen += strlen( contentsNames + contentsLen );
    }
  }

  return 1;
}
