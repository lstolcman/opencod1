/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"



/* ---- storage-of-record aliases -------------------------------------------
 * Global renderer state stays on the existing tr_records.h / tr_local.h
 * symbols; these #defines only give the ones this file uses their readable
 * (RTCW/CoD-family) names.  Retyping happens on POINTER-passed data only. */

#define tess_indexes_base           tess_indexes    /* 0x017A7F60 = tess base */


/* NV_vertex_array_range dynamic buffer: a mapped ring carved into fixed-size
 * fenced allocations.  The ring entries live as struct { int offset; int size; }
 * [512]; rb_nvFenceRingOffset/Size index the two interleaved words at stride 8. */
#define rb_nvBufferBase             backEnd_dynamicBuffer_storage   /* 0x016D93EC */
#define rb_nvBufferSize             backEnd_dynamicBuffer_capacity   /* 0x016D93F0 */
#define rb_nvBufferCursor           backEnd_dynamicBuffer_currentOffset   /* 0x016D93F4 */
#define rb_nvFenceHead              backEnd_dynamicBuffer_allocationSequence   /* 0x016D9400 */
#define rb_nvFenceTail              backEnd_dynamicBuffer_reclaimSequence   /* 0x016D9404 */
#define rb_nvBytesFree              backEnd_dynamicBuffer_freeBytes   /* 0x016D9408 */
#define rb_nvFenceRingOffset        backEnd_dynamicBuffer_allocations   /* 0x016D940C, ring[i].offset at [2*i] */
#define rb_nvFenceRingSize          ( backEnd_dynamicBuffer_allocations + 1 )   /* 0x016D9410, ring[i].size at [2*i] */

#define backEnd_currentLight        backEnd_currentDlight   /* 0x016D8E60, dlight_t* */
#define backEnd_num_dlights         backEnd_refdef_num_dlights   /* 0x016D8B1C */
#define backEnd_dlights             backEnd_refdef_dlights   /* 0x016D8B24, dlight_t* base */


#define R_MAX_SHADER_STAGES         8
#define R_MAX_TEXTURE_UNITS         8
#define R_DLIGHT_STRIDE             0x88          /* sizeof(dlight_t) */
#define SHADER_STAGE_PER_LIGHT      0x80          /* stage->attributeBits bit: draw per dynamic light */
#define SHADER_LIGHTING_PER_ENTITY  0x80          /* shader->lightingFlags bit: per-entity light pass */



/* -------------------------------------------------------------------------
 *  Shader stage as seen by the NV interleaver (file-local; not in tr_local.h).
 *
 *  A stage's texture bundles begin at +0x04 with a 0xC8 stride (layout as
 *  documented in tr_shade.c); only texCoordComponentCount (+0x8c) is read here.  attributeBits (+0x00) records
 *  which per-vertex arrays the stage streams -- bit (8+tmu) selects texcoord
 *  unit tmu, 0x10000 the color array, 0x20000 the normal array -- and the low
 *  byte's sign bit (0x80) marks a per-light stage.  The bundles are reached
 *  through STAGE_BUNDLES_OFFSET rather than the bundles[] member, as before.  Offsets proven against retail
 *  RB_SingleStageGenericNV (0x0050BD60) and RB_ComputeColors (0x004FFA60).
 * ------------------------------------------------------------------------- */
typedef struct {
	void  *image[32];               /* +0x00 */
	int   numImageAnimations;       /* +0x80 */
	float imageAnimationSpeed;      /* +0x84 */
	void  *waterMap;                /* +0x88  waterMap_t (tr_shader.c) */
	int   texCoordComponentCount;   /* +0x8c  texcoord components uploaded for this unit */
	int   textureEnvMode;           /* +0x90 */
	void  *texEnvCombine;           /* +0x94  ShaderTexEnvCombine (tr_shader.c) */
	void  *nvTextureShader;         /* +0x98  ShaderNVTextureOperation (tr_shader.c) */
	int   tcGen;                    /* +0x9c */
	float tcGenVectors[2][3];       /* +0xa0  TCGEN_VECTOR (RB_ComputeTexCoords) */
	int   numTexMods;               /* +0xb8 */
	void  *texMods;                 /* +0xbc  texModInfo_t[numTexMods] */
	int   videoMapHandle;           /* +0xc0 */
	byte  isLightmap;               /* +0xc4  r_lightmap forces this unit to REPLACE */
	byte  isVideoMap;               /* +0xc5 */
	byte  clampAnimation;           /* +0xc6 */
	byte  reserved_0xc7;            /* +0xc7  alignment */
} textureBundle_t;
COD1_ASSERT_SIZE(textureBundle_t, 0xC8);

#define STAGE_BUNDLES_OFFSET   0x04    /* textureBundle_t bundle[] base within a stage */

typedef struct {
	unsigned attributeBits;        /* +0x000  streamed arrays: bit (8+tmu)=texcoord,
	                                *          0x10000=color, 0x20000=normal, sign=per-light */
	textureBundle_t bundles[8];      /* +0x004  (STAGE_BUNDLES_OFFSET) */
	int      atiFragmentShader;     /* +0x644 */
	void    *registerCombiners;     /* +0x648 */
	char    *vertexProgram;         /* +0x64c */
	byte     rgbWave[0x14];         /* +0x650  waveForm_t */
	int      rgbGen;               /* +0x664  color source (11 => entity shaderRGBA) */
	byte     alphaWave[0x14];       /* +0x668  waveForm_t */
	int      alphaGen;              /* +0x67c */
	byte     constantColor[4];     /* +0x680  CGEN_CONST RGBA8 */
	unsigned stateBits;            /* +0x684  GL_State bits for this stage */
} shaderStage_t;
ASSERT_OFF(shaderStage_t, rgbGen, 0x664);
ASSERT_OFF(shaderStage_t, constantColor, 0x680);
ASSERT_OFF(shaderStage_t, stateBits, 0x684);


extern int  GL_Cull();
extern int  GL_State();
int  __cdecl RB_BuildDlightArrays( float *texCoords, unsigned char *colors, float *dlight, int hitIndexes );
void __cdecl    RB_ComputeColors( shaderStage_t *stage );
extern int  RB_ComputeTexCoords();
extern int  RB_DeformTessGeometry();
void __cdecl    RB_SetIteratorFog( void );
extern int  RB_SetupMultitexture();

void RB_FinishFenceNV();

/* ---- RB_FinishFenceNV  0x0050BB20 ----  VERIFIED
 * Wait on the oldest outstanding fence (skipping wrap markers, whose offset is
 * -1) and reclaim its bytes back to the free pool. */
void RB_FinishFenceNV()
{
  int slot;

  slot = rb_nvFenceTail & 0x1FF;
  if ( rb_nvFenceRingOffset[2 * slot] >= 0 )
    qglFinishFenceNV( rb_nvFenceTail );
  rb_nvBytesFree += rb_nvFenceRingSize[2 * slot];
  ++rb_nvFenceTail;
}

/* ---- RB_SelectStorageNV  0x0050BB80 ----  VERIFIED
 * Point the client arrays at the requested vertex storage: the primary or
 * secondary static NV_vertex_array_range block (storage 1 / 2), or the
 * system-memory tess arrays (storage 3 / other). */
void __cdecl RB_SelectStorageNV( int storage )
{
  if ( glState_currentStorageMode == 3 )
  {
    glEnableClientState( 0x851Du );      /* GL_VERTEX_ARRAY_RANGE_NV */
  }
  else if ( storage == 3 )
  {
    /* entering system-memory mode: drop the VAR client state, then bind tess */
    if ( glConfig_NVVertexArrayRange == 1 )
      glDisableClientState( 0x851Du );   /* GL_VERTEX_ARRAY_RANGE_NV */
    else
      glDisableClientState( 0x8533u );   /* GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV */
  }

  if ( storage == 1 )
  {
    qglVertexArrayRangeNV( tr_staticVertexMemoryPrimaryLimit,
                           (const GLvoid *)tr_staticVertexMemoryPrimary );
  }
  else if ( storage == 2 )
  {
    qglVertexArrayRangeNV( tr_staticVertexMemorySecondaryLimit,
                           (const GLvoid *)tr_staticVertexMemorySecondary );
  }
  else
  {
    glVertexPointer( tess_vertexComponentCount, 0x1406u, 0, tess_xyz );
    glNormalPointer( 0x1406u, 0, tess_stageNormals );
    glTexCoordPointer( 2, 0x1406u, 0, tess_activeTexCoords[0] );   /* retail 0x0050BBF5: loads the pointer in slot 0 */
    glColorPointer( 4, 0x1401u, 0, tess_stageVertexColors );
  }
}

/* ---- RB_GetBuffersNV  0x0050BC40 ----  VERIFIED
 * Reserve `size` bytes (rounded up to 32) from the fenced NV ring, wrapping the
 * cursor and finishing fences as needed, and return the mapped address.  The
 * (_WORD) casts Hex-Rays emits on the ring index are noise -- the sequence is a
 * full 32-bit counter masked with 0x1FF -- so they are dropped. */
int __cdecl RB_GetBuffersNV( int size )
{
  int allocSize;
  int slot;
  int tailSpace;

  allocSize = ( size + 31 ) & ~31;
  slot = ++backEnd_dynamicBuffer.allocationSequence & 0x1FF;
  if ( backEnd_dynamicBuffer.allocationSequence == backEnd_dynamicBuffer.reclaimSequence + 512 )
    RB_FinishFenceNV();

  if ( backEnd_dynamicBuffer.currentOffset + allocSize > backEnd_dynamicBuffer.capacity )
  {
    

    tailSpace = backEnd_dynamicBuffer.capacity - backEnd_dynamicBuffer.currentOffset;
    if ( tailSpace )
    {
      backEnd_dynamicBuffer.allocations[slot].size   = tailSpace;
      backEnd_dynamicBuffer.allocations[slot].offset = -1;
      backEnd_dynamicBuffer.freeBytes -= tailSpace;
      slot = ++backEnd_dynamicBuffer.allocationSequence & 0x1FF;
      if ( backEnd_dynamicBuffer.allocationSequence == backEnd_dynamicBuffer.reclaimSequence + 512 )
        RB_FinishFenceNV();
    }
    backEnd_dynamicBuffer.currentOffset = 0;
  }

  
  while ( allocSize > backEnd_dynamicBuffer.freeBytes )
    RB_FinishFenceNV( backEnd_dynamicBuffer.reclaimSequence );

  backEnd_dynamicBuffer.allocations[slot].offset = backEnd_dynamicBuffer.currentOffset;
  backEnd_dynamicBuffer.allocations[slot].size   = allocSize;
  backEnd_dynamicBuffer.freeBytes     -= allocSize;
  backEnd_dynamicBuffer.currentOffset += allocSize;
  return backEnd_dynamicBuffer.storage + backEnd_dynamicBuffer.allocations[slot].offset;
}

/* ---- RB_SetFenceNV  0x0050BD40 ----  VERIFIED
 * Outlined fragment with zero callers; the live copy is the inlined tail of
 * RB_SingleStageGenericNV.  Preserved for completeness. */
void RB_SetFenceNV( void )
{
  qglSetFenceNV( rb_nvFenceHead, 0x84F2 );   /* GL_ALL_COMPLETED_NV */
}

/* one texcoord unit of the general interleaver; retail repeats it for units 0..7 */
#define NV_INTERLEAVE_TEXCOORD( unit )                                                    \
  if ( stage->attributeBits & ( 0x100 << ( unit ) ) )                                     \
  {                                                                                       \
    *(int *)( (char *)dst + texCoordOfs[unit] + 0 ) = *texCoordSrc[unit]++;               \
    *(int *)( (char *)dst + texCoordOfs[unit] + 4 ) = *texCoordSrc[unit]++;               \
    if ( texCoordComps[unit] >= 3 )                                                       \
    {                                                                                     \
      *(int *)( (char *)dst + texCoordOfs[unit] + 8 ) = *texCoordSrc[unit]++;             \
      if ( texCoordComps[unit] == 4 )                                                     \
        *(int *)( (char *)dst + texCoordOfs[unit] + 12 ) = *texCoordSrc[unit]++;          \
    }                                                                                     \
  }

/* ---- RB_SingleStageGenericNV  0x0050BD60 ----  VERIFIED
 * Interleave one shader stage's active per-vertex arrays (texcoords, optional
 * color and normal, position) into a freshly fenced NV buffer, set the client
 * pointers, and draw.  Four common attribute combos take a hand-specialised
 * fast path; anything else uses the general interleaver.  The per-vertex byte
 * stride is computed at run time, so the general interleaver writes each array
 * at its byte offset into the mapped buffer (texcoord units unrolled 0..7, as
 * retail has them). */
void __cdecl RB_SingleStageGenericNV( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indices )
{
  int      colorOffset;
  int      normalOffset;
  int      positionOffset;
  int      vertexStride;
  int      i;
  int      v;
  int     *dst;
  int     *xyz;

  int     *texCoordSrc[R_MAX_TEXTURE_UNITS];    /* per-unit source texcoord streams */
  int      texCoordComps[R_MAX_TEXTURE_UNITS];  /* per-unit component counts (1..4) */
  int      texCoordOfs[R_MAX_TEXTURE_UNITS];    /* per-unit byte offset within a vertex */
  int      baseTexCoords[R_MAX_TEXTURE_UNITS];  /* per-unit mapped-buffer pointer for RB_SetupMultitexture */

  colorOffset  = 0;
  vertexStride = 0;   /* running attribute offset until the position is appended */
  normalOffset = 0;
  RB_ComputeTexCoords( stage );

  /* lay out the active texcoord arrays back to back */
  for ( i = 0; i < glConfig_maxActiveTextures; ++i )
  {
    if ( stage->attributeBits & ( 0x100 << i ) )
    {
      int comps = ( (textureBundle_t *)( (char *)stage + STAGE_BUNDLES_OFFSET ) )[i].texCoordComponentCount;
      texCoordOfs[i]   = vertexStride;
      texCoordComps[i] = comps;
      vertexStride    += 4 * comps;
      texCoordSrc[i]   = (int *)tess_activeTexCoords[i];   /* retail 0x0050BDB8: loads the pointer in slot i */
    }
  }

  /* then the color and normal arrays, then the position at the end */
  if ( stage->attributeBits & 0x10000 )
  {
    colorOffset   = vertexStride;
    vertexStride += 4;
    RB_ComputeColors( stage );
  }
  if ( stage->attributeBits & 0x20000 )
  {
    normalOffset  = vertexStride;
    vertexStride += 12;
  }
  positionOffset = vertexStride;
  vertexStride  += 4 * tess_vertexComponentCount;
  xyz            = (int *)tess_xyz;

  dst = (int *)RB_GetBuffersNV( vertexStride * tess_numVertexes );
  for ( i = 0; i < glConfig_maxActiveTextures; ++i )
    baseTexCoords[i] = (int)dst + texCoordOfs[i];
  RB_SetupMultitexture( stage, baseTexCoords, vertexStride );

  /* ---- point the client color / normal / vertex arrays at the buffer ---- */
  if ( stage->attributeBits & 0x10000 )
  {
    if ( ( glState_clientStateBits & 0x100 ) == 0 )
    {
      glEnableClientState( 0x8076u );          /* GL_COLOR_ARRAY */
      glState_clientStateBits |= 0x100u;
    }
    glColorPointer( 4, 0x1401u, vertexStride, (const GLvoid *)( (char *)dst + colorOffset ) );
  }
  else
  {
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( stage->rgbGen == 11 )
      glColor4ubv( (const GLubyte *)( backEnd_currentEntity + 108 ) );   /* entity->e.shaderRGBA */
    else
      glColor4ubv( (const GLubyte *)stage->constantColor );
  }

  if ( stage->attributeBits & 0x20000 )
  {
    if ( ( glState_clientStateBits & 0x200 ) == 0 )
    {
      glEnableClientState( 0x8075u );          /* GL_NORMAL_ARRAY */
      glState_clientStateBits |= 0x200u;
    }
    glNormalPointer( 0x1406u, vertexStride, (const GLvoid *)( (char *)dst + normalOffset ) );
  }
  else if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }

  if ( ( glState_clientStateBits & 0x400 ) == 0 )
  {
    glEnableClientState( 0x8074u );            /* GL_VERTEX_ARRAY */
    glState_clientStateBits |= 0x400u;
  }
  glVertexPointer( tess_vertexComponentCount, 0x1406u, vertexStride,
                   (const GLvoid *)( (char *)dst + positionOffset ) );

  /* ---- interleave the vertices ---- */
  if ( ( stage->attributeBits & 0x3FF00 ) == 0x20100 && texCoordComps[0] == 2 && tess_vertexComponentCount == 3 )
  {
    /* texcoord0 + normal + position (stride 32) */
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *dst++ = *texCoordSrc[0]++;
      *dst++ = *texCoordSrc[0]++;
      *dst++ = ( (int *)tess_stageNormals )[3 * v + 0];
      *dst++ = ( (int *)tess_stageNormals )[3 * v + 1];
      *dst++ = ( (int *)tess_stageNormals )[3 * v + 2];
      *dst++ = *xyz++;
      *dst++ = *xyz++;
      *dst++ = *xyz++;
    }
  }
  else if ( ( stage->attributeBits & 0x3FF00 ) == 0x10100 && texCoordComps[0] == 2 && tess_vertexComponentCount == 3 )
  {
    /* texcoord0 + color + position (stride 24) */
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *dst++ = *texCoordSrc[0]++;
      *dst++ = *texCoordSrc[0]++;
      *dst++ = tess_stageVertexColorsPacked[v];               /* packed RGBA8 vertex color */
      *dst++ = *xyz++;
      *dst++ = *xyz++;
      *dst++ = *xyz++;
    }
  }
  else if ( ( stage->attributeBits & 0x3FF00 ) == 0x100 && texCoordComps[0] == 2 && tess_vertexComponentCount == 3 )
  {
    /* texcoord0 + position (stride 20) */
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *dst++ = *texCoordSrc[0]++;
      *dst++ = *texCoordSrc[0]++;
      *dst++ = *xyz++;
      *dst++ = *xyz++;
      *dst++ = *xyz++;
    }
  }
  else if ( ( stage->attributeBits & 0x3FF00 ) == 0x10300 && texCoordComps[0] == 2 && texCoordComps[1] == 2
            && tess_vertexComponentCount == 3 )
  {
    /* texcoord0 + texcoord1 + color + position (stride 32) */
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *dst++ = *texCoordSrc[0]++;
      *dst++ = *texCoordSrc[0]++;
      *dst++ = *texCoordSrc[1]++;
      *dst++ = *texCoordSrc[1]++;
      *dst++ = tess_stageVertexColorsPacked[v];               /* packed RGBA8 vertex color */
      *dst++ = *xyz++;
      *dst++ = *xyz++;
      *dst++ = *xyz++;
    }
  }
  else
  {
    /* general interleaver: honor each array's own byte offset */
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      NV_INTERLEAVE_TEXCOORD( 0 )
      NV_INTERLEAVE_TEXCOORD( 1 )
      NV_INTERLEAVE_TEXCOORD( 2 )
      NV_INTERLEAVE_TEXCOORD( 3 )
      NV_INTERLEAVE_TEXCOORD( 4 )
      NV_INTERLEAVE_TEXCOORD( 5 )
      NV_INTERLEAVE_TEXCOORD( 6 )
      NV_INTERLEAVE_TEXCOORD( 7 )
      if ( stage->attributeBits & 0x10000 )
        *(int *)( (char *)dst + colorOffset ) = tess_stageVertexColorsPacked[v];
      if ( stage->attributeBits & 0x20000 )
      {
        *(int *)( (char *)dst + normalOffset + 0 ) = ( (int *)tess_stageNormals )[3 * v + 0];
        *(int *)( (char *)dst + normalOffset + 4 ) = ( (int *)tess_stageNormals )[3 * v + 1];
        *(int *)( (char *)dst + normalOffset + 8 ) = ( (int *)tess_stageNormals )[3 * v + 2];
      }
      *(int *)( (char *)dst + positionOffset + 0 ) = *xyz++;
      *(int *)( (char *)dst + positionOffset + 4 ) = *xyz++;
      *(int *)( (char *)dst + positionOffset + 8 ) = *xyz++;
      if ( tess_vertexComponentCount == 4 )
        *(int *)( (char *)dst + positionOffset + 12 ) = *xyz++;
      dst = (int *)( (char *)dst + vertexStride );
    }
  }

  /* pad the tail up to the next 32-byte boundary the ring allocator rounded to */
  if ( ( (int)dst & 0x1F ) != 0 )
  {
    int pad = ( 32 - ( (int)dst & 0x1F ) ) >> 2;
    do
      *dst++ = 0;
    while ( --pad );
  }

  GL_State( stage->stateBits );
  backEnd_pc_drawnIndexCount += numIndexes;
  ++backEnd_pc_drawCallCount;
  glDrawElements( 4u, numIndexes, 0x1403u, indices );   /* GL_TRIANGLES, GL_UNSIGNED_SHORT */
  qglSetFenceNV( rb_nvFenceHead, 0x84F2 );               /* GL_ALL_COMPLETED_NV */
}

#undef NV_INTERLEAVE_TEXCOORD

/* ---- RB_IterateStagesGenericNV  0x0050C6E0 ----  VERIFIED
 * Draw each ordinary stage (attributeBits sign bit clear) of the current tess
 * shader.  Under r_lightmap, stop after the first stage whose bundle[0] or
 * bundle[1] lightmap flag is set. */
void __cdecl RB_IterateStagesGenericNV( shaderCommands_t *input )   /* input = &tess */
{
  int            stageIndex;
  shaderStage_t *stage;

  for ( stageIndex = 0; stageIndex < R_MAX_SHADER_STAGES; stageIndex++ )
  {
    stage = ( (shaderStage_t **)tess_activeStages )[stageIndex];
    if ( !stage )
      break;

    if ( !( stage->attributeBits & SHADER_STAGE_PER_LIGHT ) )
    {
      RB_SingleStageGenericNV( stage, input->numIndexes, input->indexes );
      if ( r_lightmap->integer )
      {
        if ( ( (byte *)stage )[200] )          /* bundle[0] lightmap gate */
          break;
        if ( ( (byte *)stage )[400] )          /* bundle[1] lightmap gate */
          break;
      }
    }
  }
}

/* ---- ProjectDlightTextureNV  0x0050C740 ----  VERIFIED
 * For every dynamic light touching this surface, build the clipped index set
 * and redraw the lit triangles: either the shared dlight shader's first stage
 * (non per-entity-lit shaders) or each per-light stage of the tess shader. */
void __cdecl ProjectDlightTextureNV( void )
{
  int      i;
  int      stageIndex;
  GLsizei  hitCount;
  shaderStage_t *stage;
  shaderStage_t *dlightStage;
  unsigned short hitIndexes[49152];   /* clipped-index scratch (retail adds a 4-byte canary) */

  if ( !backEnd_num_dlights )
    return;
  if ( !rb_nvBufferSize )
    return;

  for ( i = 0; i < backEnd_num_dlights; ++i )
  {
    if ( ( ( 1 << i ) & tess_dlightBits ) == 0 )
      continue;

    hitCount = RB_BuildDlightArrays( (float *)tess_texCoords1,
                                     (unsigned char *)tess_vertexColors,
                                     (float *)( backEnd_dlights + i * R_DLIGHT_STRIDE ),
                                     (int)hitIndexes );
    if ( !hitCount )
      continue;

    if ( *(byte *)( tess_shader + 84 ) & SHADER_LIGHTING_PER_ENTITY )   /* shader->lightingFlags */
    {
      backEnd_currentLight = backEnd_dlights + i * R_DLIGHT_STRIDE;
      for ( stageIndex = 0; stageIndex < R_MAX_SHADER_STAGES; stageIndex++ )
      {
        stage = ( (shaderStage_t **)tess_activeStages )[stageIndex];
        if ( !stage )
          break;
        if ( stage->attributeBits & SHADER_STAGE_PER_LIGHT )
          RB_SingleStageGenericNV( stage, hitCount, hitIndexes );
      }
    }
    else
    {
      dlightStage = *(shaderStage_t **)( tr_dlightShader + 340 );   /* dlightShader->stages[0] */
      if ( dlightStage )
        RB_SingleStageGenericNV( dlightStage, hitCount, hitIndexes );
    }
  }
}

/* ---- RB_StageIteratorGenericNV  0x0050C870 ----  VERIFIED
 * Top-level generic-stage iterator for the NV path: deform, fog, cull, draw the
 * base stages (unless portal-only), then apply dynamic-light and per-entity
 * light passes. */
void __cdecl RB_StageIteratorGenericNV( int portalPass )
{
  char    *logText;
  int      entity;
  int      lightCount;
  int      lightIndex;
  int      stageIndex;
  shaderStage_t *stage;

  if ( tess_activeStageCount <= 0 )
    return;
  if ( portalPass && !tess_dlightBits )
    return;

  if ( r_logFile->integer )
  {
    logText = va( "--- RB_StageIteratorGenericNV( %s ) ---\n", (const char *)tess_shader );
    if ( Stream )
      fprintf( Stream, "%s", logText );
  }

  RB_DeformTessGeometry();
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );          /* shader->cullType */

  if ( !portalPass )
    RB_IterateStagesGenericNV( &tess );

  if ( tess_dlightBits
    && ( ((shader_t *)tess_shader)->sort <= 5.0f                              /* shader->sort */
      || ( *(unsigned *)( tess_shader + 84 ) & SHADER_LIGHTING_PER_ENTITY ) )
    && !( ((shader_t *)tess_shader)->surfaceParmFlags & 0x20004 ) )                        /* shader->surfaceParmFlags */
  {
    ProjectDlightTextureNV();
    return;
  }

  if ( !( *(unsigned *)( tess_shader + 84 ) & SHADER_LIGHTING_PER_ENTITY ) )
    return;

  entity = backEnd_currentEntity;
  if ( !entity )
    return;
  lightCount = ((trRefEntity_t *)entity)->lightCount;
  if ( !lightCount )
    return;

  for ( lightIndex = 0; lightIndex < lightCount; ++lightIndex )
  {
    backEnd_currentLight = (int)((trRefEntity_t *)entity)->lights[lightIndex].light;
    if ( *(int *)backEnd_currentLight != 8 )         /* skip light type 8 */
    {
      backEnd_currentLightScale = ((trRefEntity_t *)entity)->lights[lightIndex].scale;

      for ( stageIndex = 0; stageIndex < R_MAX_SHADER_STAGES; stageIndex++ )
      {
        stage = ( (shaderStage_t **)tess_activeStages )[stageIndex];
        if ( !stage )
          break;
        if ( stage->attributeBits & SHADER_STAGE_PER_LIGHT )
        {
          RB_SingleStageGenericNV( stage, tess_numIndexes, (GLvoid *)tess_indexes_base );
          entity = backEnd_currentEntity;
        }
      }
    }
    lightCount = ((trRefEntity_t *)entity)->lightCount;
  }

  backEnd_currentLight = 0;
}
