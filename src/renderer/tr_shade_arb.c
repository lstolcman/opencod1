/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"

/* -------------------------------------------------------------------------
 *  The ARB_vertex_buffer_object storage backend of the shader/tess system.
 *  For each shader stage it packs the tessellated vertices into a dynamic
 *  ARB buffer (interleaved by RB_SingleStageGenericARB, or one array per
 *  attribute by RB_SingleStageGenericARB2) and issues the draw.
 *
 *  Only the shader STAGE is retyped here (RB_SingleStageGenericARB's `a3`);
 *  the tess arrays and the backEnd/tr globals keep the retail storage-of-record
 *  in tr_tess.h / tr_records.h / tr_local.h per the hybrid blob+symbol
 *  scheme.
 * ------------------------------------------------------------------------- */

#define R_MAX_SHADER_STAGES         8
#define R_MAX_TEXTURE_UNITS         8
#define RB_ARB_MAX_UPLOADS          11           /* 8 texcoords + colour + normal + position */

#define SHADER_STAGE_PER_LIGHT      0x80         /* stage.flags: replay this stage once per entity light */
#define SHADER_LIGHTING_PER_ENTITY  0x80         /* shader.lightingFlags: shader is lit per entity */

#define CGEN_LIGHTING_PRECALC       11           /* stage.rgbGen: take the entity's constant colour */

/* shaderStage_t.flags -- which vertex arrays this stage feeds.  The per-unit
 * texcoord bit for unit n is (SS_TEXCOORD_ARRAY0 << n).  Masks proven by the
 * decompiled bit tests; names from CoDUO renderer_shader_stage_flag_e. */
#define SS_TEXCOORD_ARRAY0          0x00000100
#define SS_COLOR_ARRAY              0x00010000
#define SS_NORMAL_ARRAY             0x00020000
#define SS_VERTEX_ARRAY_MASK        0x0003FF00   /* 8 texcoords + colour + normal */

/* -------------------------------------------------------------------------
 *  shaderStage_t (0x688) and its per-unit textureBundle_t (0xC8).
 *
 *  The ARB backend only reads: the vertex-array flags (+0x000), each unit's
 *  texcoord component count (bundle +0x8c) and lightmap marker (bundle +0xC4),
 *  the rgbGen selector (+0x664), the constant colour (+0x680) and the GL state
 *  bits (+0x684).  Offsets are proven by the decompiled reads (stage +0x90 +
 *  0xC8 * unit for the component count; stage[409]==rgbGen@0x664,
 *  stage+1664==constantColor@0x680, stage[417]==stateBits@0x684); the bundle
 *  layout (base = image[0] = stage + 4) is the one documented in tr_shade.c.
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
ASSERT_OFF(textureBundle_t, texCoordComponentCount, 0x8c);
ASSERT_OFF(textureBundle_t, isLightmap, 0xc4);

typedef struct shaderStage_s {
	unsigned int    flags;                        /* +0x000  SS_* vertex-array + stage-type bits */
	textureBundle_t bundle[R_MAX_TEXTURE_UNITS];  /* +0x004  stride 0xC8 */
	int             atiFragmentShader;            /* +0x644 */
	void           *registerCombiners;            /* +0x648 */
	char           *vertexProgram;                /* +0x64c */
	byte            rgbWave[0x14];                /* +0x650  waveForm_t */
	int             rgbGen;                        /* +0x664  shader_color_gen_t */
	byte            alphaWave[0x14];              /* +0x668  waveForm_t */
	int             alphaGen;                     /* +0x67c */
	byte            constantColor[4];             /* +0x680  RGBA */
	unsigned int    stateBits;                    /* +0x684  GL_State bits */
} shaderStage_t;
COD1_ASSERT_SIZE(shaderStage_t, 0x688);
ASSERT_OFF(shaderStage_t, bundle, 0x04);
ASSERT_OFF(shaderStage_t, rgbGen, 0x664);
ASSERT_OFF(shaderStage_t, constantColor, 0x680);
ASSERT_OFF(shaderStage_t, stateBits, 0x684);


extern int GL_Cull();
extern int GL_State();
int __cdecl RB_BuildDlightArrays( float *texCoords, unsigned char *colors, float *dlight, int hitIndexes );
void __cdecl RB_ComputeColors( int stage );
extern int RB_ComputeTexCoords();
extern int RB_DeformTessGeometry();
void __cdecl RB_SetIteratorFog( void );
extern int RB_SetupMultitexture();

void __cdecl RB_SingleStageGenericARB( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indexes );
void __cdecl RB_SingleStageGenericARB2( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indexes );

/* RB_SingleStageGenericARB's general interleave path, one texcoord unit
 * (retail has the 8 units unrolled with constant masks) */
#define RB_ARB_INTERLEAVE_TEXCOORDS( n ) \
  if ( ( stage->flags & (SS_TEXCOORD_ARRAY0 << (n)) ) != 0 ) \
  { \
    *(int *)( (char *)out + texCoordOfs[n] + 0 ) = *texCoordSrc[n]++; \
    *(int *)( (char *)out + texCoordOfs[n] + 4 ) = *texCoordSrc[n]++; \
    if ( texCoordComps[n] >= 3 ) \
    { \
      *(int *)( (char *)out + texCoordOfs[n] + 8 ) = *texCoordSrc[n]++; \
      if ( texCoordComps[n] == 4 ) \
        *(int *)( (char *)out + texCoordOfs[n] + 12 ) = *texCoordSrc[n]++; \
    } \
  }

/* RB_SingleStageGenericARB's draw, written once per upload path (retail merged the copies) */
#define RB_ARB_DRAW_ELEMENTS( count, elements ) \
  { \
    GLsizei  drawCount = (count); \
    GLvoid  *drawElements = (elements); \
    backEnd_pc_drawnIndexCount += drawCount; \
    ++backEnd_pc_drawCallCount; \
    glDrawElements( 4u, drawCount, 0x1403u, drawElements );   /* GL_TRIANGLES, GL_UNSIGNED_SHORT */ \
  }


/* ---- RB_SingleStageGenericARB  0x0050C9D0 ----  VERIFIED
 * Interleaved path (r_vbo_interleave): pack every vertex as
 * [texcoord0..N][colour][normal][position] into one dynamic ARB buffer, wire
 * the client-array pointers to the interleaved offsets, then draw.  Four common
 * attribute layouts get a flat copy loop; anything else uses the general path.
 * The write target is a Hunk scratch buffer (uploaded with one qglBufferSubDataARB)
 * when a persistent VBO exists or the buffer map fails, otherwise the mapped
 * buffer itself -- and there the unmap result GATES the draw. */
void __cdecl RB_SingleStageGenericARB( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indexes )
{
  int   texCoordOfs[R_MAX_TEXTURE_UNITS];          /* byte offset of each unit's texcoords in the vertex */
  int   texCoordComps[R_MAX_TEXTURE_UNITS];        /* components per unit (2/3/4) */
  int  *texCoordSrc[R_MAX_TEXTURE_UNITS];          /* source array per unit (advances per vertex) */
  int   uploadTexCoordOffsets[R_MAX_TEXTURE_UNITS];/* buffer offsets handed to RB_SetupMultitexture */
  int  *stageColors;                               /* tess.stageVertexColors as packed RGBA dwords */
  int  *xyz;                                        /* tess.xyz read cursor */
  int  *out;                                        /* interleave write cursor */
  int  *tempMem;                                    /* Hunk scratch (NULL when writing a mapped VBO) */
  int   colorOfs;
  int   normalOfs;
  int   positionOfs;
  int   vertexStride;                               /* running attribute offset, then bytes per vertex */
  int   totalBytes;                                 /* vertexStride * tess_numVertexes */
  int   bufBase;                                    /* base byte offset into the persistent VBO */
  int   unit;
  int   v;

  colorOfs     = 0;
  vertexStride = 0;
  normalOfs    = 0;
  stageColors  = (int *)tess_stageVertexColors;

  RB_ComputeTexCoords( (int)stage );

  /* reserve a texcoord block for each active unit */
  for ( unit = 0; unit < glConfig_maxActiveTextures; ++unit )
  {
    if ( ( stage->flags & (SS_TEXCOORD_ARRAY0 << unit) ) != 0 )
    {
      int comps = stage->bundle[unit].texCoordComponentCount;
      texCoordOfs[unit]   = vertexStride;
      texCoordComps[unit] = comps;
      vertexStride += 4 * comps;
      texCoordSrc[unit]   = (int *)tess_activeTexCoords[unit];   /* retail 0x0050CA28: loads the pointer in slot `unit` */
    }
  }

  if ( ( stage->flags & SS_COLOR_ARRAY ) != 0 )
  {
    colorOfs = vertexStride;
    vertexStride += 4;
    RB_ComputeColors( (int)stage );
  }
  if ( ( stage->flags & SS_NORMAL_ARRAY ) != 0 )
  {
    normalOfs = vertexStride;
    vertexStride += 12;
  }

  positionOfs  = vertexStride;
  vertexStride += 4 * tess_vertexComponentCount;
  xyz          = (int *)tess_xyz;
  bufBase      = 0;
  totalBytes   = vertexStride * tess_numVertexes;

  if ( backEnd_dynamicBuffer_storage )
  {
    qglBindBufferARB( 34962, backEnd_dynamicBuffer_storage );   /* GL_ARRAY_BUFFER_ARB */
    if ( backEnd_dynamicBuffer_currentOffset + totalBytes <= backEnd_dynamicBuffer_capacity )
      bufBase = backEnd_dynamicBuffer_currentOffset;
  }
  else
  {
    qglBindBufferARB( 34962, ++backEnd_dynamicBuffer_frameSerial );
  }

  for ( unit = 0; unit < glConfig_maxActiveTextures; ++unit )
    uploadTexCoordOffsets[unit] = bufBase + texCoordOfs[unit];
  RB_SetupMultitexture( stage, uploadTexCoordOffsets, vertexStride );

  /* colour: a client array, else the entity colour or the stage constant */
  if ( ( stage->flags & SS_COLOR_ARRAY ) != 0 )
  {
    if ( ( glState_clientStateBits & 0x100 ) == 0 )
    {
      glEnableClientState( 0x8076u );              /* GL_COLOR_ARRAY */
      glState_clientStateBits |= 0x100u;
    }
    glColorPointer( 4, 0x1401u, vertexStride, (const GLvoid *)( colorOfs + bufBase ) );   /* GL_UNSIGNED_BYTE */
  }
  else
  {
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
      glColor4ubv( (const GLubyte *)( backEnd_currentEntity + 108 ) );   /* entity shaderRGBA */
    else
      glColor4ubv( (const GLubyte *)stage->constantColor );
  }

  /* normal client array */
  if ( ( stage->flags & SS_NORMAL_ARRAY ) != 0 )
  {
    if ( ( glState_clientStateBits & 0x200 ) == 0 )
    {
      glEnableClientState( 0x8075u );              /* GL_NORMAL_ARRAY */
      glState_clientStateBits |= 0x200u;
    }
    glNormalPointer( 0x1406u, vertexStride, (const GLvoid *)( normalOfs + bufBase ) );    /* GL_FLOAT */
  }
  else if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }

  /* position client array (always present) */
  if ( ( glState_clientStateBits & 0x400 ) == 0 )
  {
    glEnableClientState( 0x8074u );                /* GL_VERTEX_ARRAY */
    glState_clientStateBits |= 0x400u;
  }
  glVertexPointer( tess_vertexComponentCount, 0x1406u, vertexStride, (const GLvoid *)( positionOfs + bufBase ) );

  /* choose the interleave target */
  if ( backEnd_dynamicBuffer_storage )
  {
    out     = (int *)ri.Hunk_AllocateTempMemory( totalBytes );
    tempMem = out;
  }
  else
  {
    tempMem = 0;
    qglBufferDataARB( 34962, totalBytes, 0, 35040 );             /* GL_STREAM_DRAW_ARB */
    out = (int *)qglMapBufferARB( 34962, 35001 );               /* GL_WRITE_ONLY_ARB */
    if ( !out )
    {
      out     = (int *)ri.Hunk_AllocateTempMemory( totalBytes );
      tempMem = out;
    }
  }

  /* ---- interleave the vertices ---- */
  if ( (stage->flags & SS_VERTEX_ARRAY_MASK) == (SS_TEXCOORD_ARRAY0 | SS_NORMAL_ARRAY)
       && texCoordComps[0] == 2 && tess_vertexComponentCount == 3 )
  {
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *out++ = *texCoordSrc[0]++;
      *out++ = *texCoordSrc[0]++;
      *out++ = ((int *)tess_stageNormals)[3 * v + 0];
      *out++ = ((int *)tess_stageNormals)[3 * v + 1];
      *out++ = ((int *)tess_stageNormals)[3 * v + 2];
      *out++ = *xyz++;
      *out++ = *xyz++;
      *out++ = *xyz++;
    }
  }
  else if ( (stage->flags & SS_VERTEX_ARRAY_MASK) == (SS_TEXCOORD_ARRAY0 | SS_COLOR_ARRAY)
            && texCoordComps[0] == 2 && tess_vertexComponentCount == 3 )
  {
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *out++ = *texCoordSrc[0]++;
      *out++ = *texCoordSrc[0]++;
      *out++ = stageColors[v];
      *out++ = *xyz++;
      *out++ = *xyz++;
      *out++ = *xyz++;
    }
  }
  else if ( (stage->flags & SS_VERTEX_ARRAY_MASK) == SS_TEXCOORD_ARRAY0
            && texCoordComps[0] == 2 && tess_vertexComponentCount == 3 )
  {
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *out++ = *texCoordSrc[0]++;
      *out++ = *texCoordSrc[0]++;
      *out++ = *xyz++;
      *out++ = *xyz++;
      *out++ = *xyz++;
    }
  }
  else if ( (stage->flags & SS_VERTEX_ARRAY_MASK)
              == (SS_TEXCOORD_ARRAY0 | (SS_TEXCOORD_ARRAY0 << 1) | SS_COLOR_ARRAY)
            && texCoordComps[0] == 2 && texCoordComps[1] == 2 && tess_vertexComponentCount == 3 )
  {
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      *out++ = *texCoordSrc[0]++;
      *out++ = *texCoordSrc[0]++;
      *out++ = *texCoordSrc[1]++;
      *out++ = *texCoordSrc[1]++;
      *out++ = stageColors[v];
      *out++ = *xyz++;
      *out++ = *xyz++;
      *out++ = *xyz++;
    }
  }
  else
  {
    /* general path: each attribute written at its own byte offset */
    for ( v = 0; v < tess_numVertexes; ++v )
    {
      RB_ARB_INTERLEAVE_TEXCOORDS( 0 );
      RB_ARB_INTERLEAVE_TEXCOORDS( 1 );
      RB_ARB_INTERLEAVE_TEXCOORDS( 2 );
      RB_ARB_INTERLEAVE_TEXCOORDS( 3 );
      RB_ARB_INTERLEAVE_TEXCOORDS( 4 );
      RB_ARB_INTERLEAVE_TEXCOORDS( 5 );
      RB_ARB_INTERLEAVE_TEXCOORDS( 6 );
      RB_ARB_INTERLEAVE_TEXCOORDS( 7 );
      if ( ( stage->flags & SS_COLOR_ARRAY ) != 0 )
        *(int *)( (char *)out + colorOfs ) = stageColors[v];
      if ( ( stage->flags & SS_NORMAL_ARRAY ) != 0 )
      {
        *(int *)( (char *)out + normalOfs + 0 ) = ((int *)tess_stageNormals)[3 * v + 0];
        *(int *)( (char *)out + normalOfs + 4 ) = ((int *)tess_stageNormals)[3 * v + 1];
        *(int *)( (char *)out + normalOfs + 8 ) = ((int *)tess_stageNormals)[3 * v + 2];
      }
      *(int *)( (char *)out + positionOfs + 0 ) = *xyz++;
      *(int *)( (char *)out + positionOfs + 4 ) = *xyz++;
      *(int *)( (char *)out + positionOfs + 8 ) = *xyz++;
      if ( tess_vertexComponentCount == 4 )
        *(int *)( (char *)out + positionOfs + 12 ) = *xyz++;
      out = (int *)( (char *)out + vertexStride );
    }
  }

  GL_State( stage->stateBits );

  if ( backEnd_dynamicBuffer_storage )
  {
    qglBufferSubDataARB( 34962, bufBase, totalBytes, tempMem );
    backEnd_dynamicBuffer_currentOffset = ( totalBytes + bufBase + 31 ) & 0xFFFFFFE0;
    ri.Hunk_FreeTempMemory( tempMem );
    RB_ARB_DRAW_ELEMENTS( numIndexes, indexes );
  }
  else
  {
    if ( !tempMem )
    {
      if ( !(unsigned char)qglUnmapBufferARB( 34962 ) )   /* the buffer contents are lost: skip the draw */
        goto done;
    }
    else
    {
      qglBufferDataARB( 34962, vertexStride * tess_numVertexes, tempMem, 35040 );
      ri.Hunk_FreeTempMemory( tempMem );
    }
    RB_ARB_DRAW_ELEMENTS( numIndexes, indexes );
  }
done:
  qglBindBufferARB( 34962, 0 );
}

/* ---- RB_PickBufferOffsetARB  0x0050D440 ----  VERIFIED
 * Reserve `size` bytes in the dynamic buffer: return the current cursor, or 0
 * (wrap to the start) when the block would overrun `capacity`, then advance the
 * cursor past the reservation.  The capacity compare is signed.  The compiler
 * outlined this and inlined a copy at every ARB2 upload site, so it has no
 * callers of its own; kept for the 1:1 mapping.  The (cursor, size, capacity)
 * parameter order gives retail's register convention (esi, ecx, edi). */
int __cdecl RB_PickBufferOffsetARB( int *cursor, int size, int capacity )
{
  int result;

  result  = ( *cursor + size > capacity ) ? 0 : *cursor;
  *cursor = result + size;
  return result;
}

/* ---- RB_SingleStageGenericARB2  0x0050D460 ----  VERIFIED
 * Non-interleaved path: upload each attribute array to its own region of the
 * dynamic ARB buffer (colour and normal only when present) and draw.  Without a
 * persistent VBO the capacity is treated as unbounded and the buffer name is a
 * frame serial rather than a real GL object.  uploadOffset/uploadSource/uploadSize
 * are three parallel runs of RB_ARB_MAX_UPLOADS entries; position is the last. */
void __cdecl RB_SingleStageGenericARB2( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indexes )
{
  GLint         uploadOffset[RB_ARB_MAX_UPLOADS];
  const GLvoid *uploadSource[RB_ARB_MAX_UPLOADS];
  GLsizei       uploadSize[RB_ARB_MAX_UPLOADS];
  int           texCoordUploadOffsets[R_MAX_TEXTURE_UNITS];
  int           buffer;
  int           cursor;
  int           capacity;
  int           uploadCount;
  int           size;
  int           ofs;
  int           positionEnd;
  int           totalUploads;
  int           unit;
  int           i;

  if ( backEnd_dynamicBuffer_storage )
  {
    buffer   = backEnd_dynamicBuffer_storage;
    cursor   = backEnd_dynamicBuffer_currentOffset;
    capacity = backEnd_dynamicBuffer_capacity;
  }
  else
  {
    cursor   = 0;
    capacity = 0x7FFFFFFF;
    buffer   = ++backEnd_dynamicBuffer_frameSerial;
  }
  qglBindBufferARB( 34962, buffer );

  uploadCount = 0;
  RB_ComputeTexCoords( (int)stage );

  /* one upload per active texcoord unit */
  for ( unit = 0; unit < glConfig_maxActiveTextures; ++unit )
  {
    texCoordUploadOffsets[unit] = 0;
    if ( ( stage->flags & (SS_TEXCOORD_ARRAY0 << unit) ) != 0 )
    {
      size = 4 * tess_numVertexes * stage->bundle[unit].texCoordComponentCount;
      ofs  = ( cursor + size > capacity ) ? 0 : cursor;
      cursor = ofs + size;
      uploadSize[uploadCount]     = size;
      uploadOffset[uploadCount]   = ofs;
      uploadSource[uploadCount]   = (const GLvoid *)tess_activeTexCoords[unit];   /* retail 0x0050D516: pointer in slot `unit` */
      texCoordUploadOffsets[unit] = ofs;
      ++uploadCount;
    }
  }
  RB_SetupMultitexture( stage, texCoordUploadOffsets, 0 );

  /* colour */
  if ( ( stage->flags & SS_COLOR_ARRAY ) != 0 )
  {
    size = 4 * tess_numVertexes;
    ofs  = ( cursor + size > capacity ) ? 0 : cursor;
    cursor = ofs + size;
    uploadSize[uploadCount]   = size;
    uploadSource[uploadCount] = (const GLvoid *)tess_stageVertexColors;
    uploadOffset[uploadCount] = ofs;
    RB_ComputeColors( (int)stage );
    if ( ( glState_clientStateBits & 0x100 ) == 0 )
    {
      glEnableClientState( 0x8076u );
      glState_clientStateBits |= 0x100u;
    }
    glColorPointer( 4, 0x1401u, 0, (const GLvoid *)ofs );
    ++uploadCount;
  }
  else
  {
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
      glColor4ubv( (const GLubyte *)( backEnd_currentEntity + 108 ) );
    else
      glColor4ubv( (const GLubyte *)stage->constantColor );
  }

  /* normal */
  if ( ( stage->flags & SS_NORMAL_ARRAY ) != 0 )
  {
    size = 12 * tess_numVertexes;
    ofs  = ( cursor + size > capacity ) ? 0 : cursor;
    cursor = ofs + size;
    uploadSize[uploadCount]   = size;
    uploadSource[uploadCount] = (const GLvoid *)tess_stageNormals;
    uploadOffset[uploadCount] = ofs;
    if ( ( glState_clientStateBits & 0x200 ) == 0 )
    {
      glEnableClientState( 0x8075u );
      glState_clientStateBits |= 0x200u;
    }
    glNormalPointer( 0x1406u, 0, (const GLvoid *)ofs );
    ++uploadCount;
  }
  else if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }

  /* position -- always uploaded, and the final upload */
  size = 4 * tess_numVertexes * tess_vertexComponentCount;
  ofs  = ( cursor + size > capacity ) ? 0 : cursor;
  positionEnd = ofs + size;
  uploadSize[uploadCount]   = size;
  uploadSource[uploadCount] = (const GLvoid *)tess_xyz;
  uploadOffset[uploadCount] = ofs;
  if ( ( glState_clientStateBits & 0x400 ) == 0 )
  {
    glEnableClientState( 0x8074u );
    glState_clientStateBits |= 0x400u;
  }
  glVertexPointer( tess_vertexComponentCount, 0x1406u, 0, (const GLvoid *)ofs );
  totalUploads = uploadCount + 1;

  if ( !backEnd_dynamicBuffer_storage )
    qglBufferDataARB( 34962, positionEnd, 0, 35040 );
  else
    backEnd_dynamicBuffer_currentOffset = positionEnd;

  for ( i = 0; i < totalUploads; ++i )
    qglBufferSubDataARB( 34962, uploadOffset[i], uploadSize[i], uploadSource[i] );

  GL_State( stage->stateBits );
  backEnd_pc_drawnIndexCount += numIndexes;
  ++backEnd_pc_drawCallCount;
  glDrawElements( 4u, numIndexes, 0x1403u, indexes );
  qglBindBufferARB( 34962, 0 );
}

/* ---- RB_IterateStagesGenericARB  0x0050D7B0 ----  VERIFIED
 * Draw every ordinary (non per-light) stage of the current surface.  With
 * r_lightmap, stop after the first stage whose first or second unit is a
 * lightmap.  `input` is &tess (Q3's shaderCommands_t *input; retail passes it
 * in ebx). */
void __cdecl RB_IterateStagesGenericARB( shaderCommands_t *input )
{
  int            i;
  shaderStage_t *stage;

  for ( i = 0; i < R_MAX_SHADER_STAGES; ++i )
  {
    stage = ((shaderStage_t **)tess_activeStages)[i];
    if ( !stage )
      break;

    if ( ( stage->flags & SHADER_STAGE_PER_LIGHT ) == 0 )
    {
      if ( tr_vboInterleaved )
        RB_SingleStageGenericARB( stage, input->numIndexes, (GLvoid *)input->indexes );
      else
        RB_SingleStageGenericARB2( stage, input->numIndexes, (GLvoid *)input->indexes );

      if ( r_lightmap->integer )
      {
        if ( stage->bundle[0].isLightmap )
          break;
        if ( stage->bundle[1].isLightmap )
          break;
      }
    }
  }
}

/* ---- ProjectDlightTextureARB  0x0050D820 ----  VERIFIED
 * For every dynamic light touching this surface, build its projected index /
 * colour arrays and draw either the shared dlight stage (shader not lit per
 * entity) or each per-light stage of the surface. */
void __cdecl ProjectDlightTextureARB( void )
{
  shaderStage_t *stage;
  shaderStage_t *dlightStage;
  int      stageIndex;
  int      i;
  GLsizei  hitIndexCount;
  unsigned short hitIndexes[49152];

  if ( !backEnd_refdef_num_dlights )
    return;

  for ( i = 0; i < backEnd_refdef_num_dlights; ++i )
  {
    if ( ( ( 1 << i ) & tess_dlightBits ) == 0 )
      continue;

    hitIndexCount = RB_BuildDlightArrays( (float *)tess_texCoords1,
                                          (unsigned char *)tess_vertexColors,
                                          (float *)&( (dlight_t *)backEnd_refdef_dlights )[i],
                                          (int)hitIndexes );
    if ( !hitIndexCount )
      continue;

    if ( *(char *)( tess_shader + 84 ) & SHADER_LIGHTING_PER_ENTITY )   /* shader.lightingFlags */
    {
      backEnd_currentDlight = (int)&( (dlight_t *)backEnd_refdef_dlights )[i];
      for ( stageIndex = 0; stageIndex < R_MAX_SHADER_STAGES; ++stageIndex )
      {
        stage = ((shaderStage_t **)tess_activeStages)[stageIndex];
        if ( !stage )
          break;
        if ( ( stage->flags & SHADER_STAGE_PER_LIGHT ) != 0 )
        {
          if ( tr_vboInterleaved )
            RB_SingleStageGenericARB( stage, hitIndexCount, hitIndexes );
          else
            RB_SingleStageGenericARB2( stage, hitIndexCount, hitIndexes );
        }
      }
    }
    else
    {
      dlightStage = *(shaderStage_t **)( tr_dlightShader + 340 );   /* tr.dlightShader->stages[0] */
      if ( dlightStage )
      {
        if ( tr_vboInterleaved )
          RB_SingleStageGenericARB( dlightStage, hitIndexCount, hitIndexes );
        else
          RB_SingleStageGenericARB2( dlightStage, hitIndexCount, hitIndexes );
      }
    }
  }
}

/* ---- RB_StageIteratorGenericARB  0x0050D970 ----  VERIFIED
 * ARB backend stage iterator (tr.stageIteratorFunc): deform, fog and cull the
 * current surface, draw the ordinary stages, then run the dynamic-light pass --
 * either the projected dlight texture or, for per-entity-lit shaders, each
 * per-light stage once for every entity light. */
void __cdecl RB_StageIteratorGenericARB( int portalPass )
{
  char          *logText;
  shaderStage_t *stage;
  int            entity;
  int            lightCount;
  int            lightIndex;
  int            stageIndex;

  if ( tess_activeStageCount <= 0 )
    return;
  if ( portalPass && !tess_dlightBits )
    return;

  if ( r_logFile->integer )
  {
    logText = va( "--- RB_StageIteratorGenericARB( %s ) ---\n", (const char *)tess_shader );
    if ( Stream )
      fprintf( Stream, "%s", logText );
  }

  RB_DeformTessGeometry();
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );          /* shader.cullType */

  if ( !portalPass )
    RB_IterateStagesGenericARB( &tess );

  /* dynamic-light pass */
  if ( tess_dlightBits )
  {
    if ( ((shader_t *)tess_shader)->sort <= 5.0f      /* shader.sort */
      || ( *(unsigned char *)( tess_shader + 84 ) & SHADER_LIGHTING_PER_ENTITY ) != 0 )
    {
      if ( ( ((shader_t *)tess_shader)->surfaceParmFlags & 0x20004 ) == 0 )
      {
        ProjectDlightTextureARB();
        return;
      }
    }
  }

  /* per-entity light stages */
  if ( ( *(unsigned char *)( tess_shader + 84 ) & SHADER_LIGHTING_PER_ENTITY ) == 0 )
    return;

  entity = backEnd_currentEntity;
  if ( !entity )
    return;
  lightCount = *(int *)( entity + 204 );             /* trRefEntity.lightCount */
  if ( !lightCount )
    return;

  for ( lightIndex = 0; lightIndex < lightCount; ++lightIndex )
  {
    backEnd_currentDlight = *(int *)( entity + 8 * lightIndex + 208 );   /* lights[i].light */
    if ( *(int *)backEnd_currentDlight != 8 )        /* light.type != R_LIGHT_TYPE (8) */
    {
      backEnd_currentLightScale = *(float *)( entity + 8 * lightIndex + 212 );   /* lights[i].scale */

      for ( stageIndex = 0; stageIndex < R_MAX_SHADER_STAGES; ++stageIndex )
      {
        stage = ((shaderStage_t **)tess_activeStages)[stageIndex];
        if ( !stage )
          break;
        if ( ( stage->flags & SHADER_STAGE_PER_LIGHT ) != 0 )
        {
          if ( tr_vboInterleaved )
            RB_SingleStageGenericARB( stage, tess_numIndexes, (GLvoid *)tess_indexes );
          else
            RB_SingleStageGenericARB2( stage, tess_numIndexes, (GLvoid *)tess_indexes );
        }
        entity = backEnd_currentEntity;
      }
    }
    lightCount = *(int *)( entity + 204 );
  }

  backEnd_currentDlight = 0;
}
