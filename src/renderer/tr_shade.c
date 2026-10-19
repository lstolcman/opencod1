/*
 * @fidelity: likely
 * @fidelity-default: reviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
int c_begins;
int c_vertexes;
int setArraysOnce;
/* RTCW tr_shade.c tess (renderer/tr_tess.h) */
shaderCommands_t tess;      /* 0x017A7F60, 0x218068 bytes */


extern int GL_Bind();
extern int __cdecl GL_CheckErrors( const char *location );
extern int GL_ClientState();
extern int GL_Cull();
extern int GL_State();
extern int GL_TexEnv();
extern float *GlobalPositionToLocal(float *world, float *out);
extern int RB_BeginImmediateMode();
void __cdecl RB_CalcAlphaFromEntity( unsigned char *colors );
void __cdecl RB_CalcAlphaFromOneMinusEntity( unsigned char *colors );
void __fastcall RB_CalcColorFromEntity( unsigned char *colors );
int __fastcall RB_CalcColorFromOneMinusEntity( void *colors );
extern int RB_CalcCubeMapBumpmapFrameTexCoords();
extern void RB_CalcCubeMapDot3ReflectTexCoords( float *out, int startIndex );
int __fastcall RB_CalcCubeMapEyeToVertexTexCoords( int st );
extern int RB_CalcCubeMapLightHalfAngleTexCoords();
int __fastcall RB_CalcCubeMapLightVectorTexCoords( int st );
void __cdecl RB_CalcCubeMapReflectionTexCoords( float *st );
extern int RB_CalcCubeMapSunHalfAngleTexCoords();
extern int RB_CalcCubeMapTbnTexCoords();
int __fastcall RB_CalcCubeMapVertexToEyeTexCoords( int st );
int __cdecl RB_CalcDiffuseColor( void *colors );
extern int RB_CalcEnvironmentTexCoords();
extern float *__cdecl RB_CalcRotateTexCoords( float degsPerSecond, float *st );
void __cdecl RB_CalcScaleTexCoords( const float *scale, float *st );
void __cdecl RB_CalcScrollTexCoords( const float *scrollSpeed, float *st );
int __cdecl RB_CalcSpecularAlpha( int colors );
extern int RB_CalcStretchTexCoords();
void __cdecl RB_CalcSwapTexCoords( float *st );
extern int RB_CalcTangentSpace();
extern int RB_CalcTransformTexCoords();
void __cdecl RB_CalcTurbulentTexCoords( float *st, const float *wf );
int __cdecl RB_CalcWaveAlpha( void *wave, void *colors );
int __cdecl RB_CalcWaveColor( void *colors, void *wave );
int __cdecl RB_DeformTessGeometry( void );
extern int RB_SelectStorageNV();
extern int RB_ShadowTessEnd();
extern int RB_StageIteratorSky();
extern int RB_UploadWaterTexture();
extern int RB_glVertex3f();
void __cdecl RB_glVertex3fv( const float *v );
extern int R_Fog();
void VectorNormalizeFast( float *v );
void VectorRotate( float *in, float *matrix, float *out );

void __cdecl RB_DisableTMU( int tmu );
void __cdecl RB_BeginSurface( void *shader, int vertexComponentCount );
void __cdecl RB_EndSurface( void );

/* -------------------------------------------------------------------------
 *  shaderStage_t (0x688) and its per-unit textureBundle_t (0xC8).
 *
 *  The stage is the vertex-array flag word (+0x000), eight texture bundles
 *  (+0x004, stride 0xC8 -- the `50 * unit` dword indexing), the ATI fragment
 *  shader / NV register combiners / vertex program (+0x644..+0x64c), the
 *  rgb/alpha waves and generators (+0x650..+0x67c), the constant colour
 *  (+0x680) and the GL state bits (+0x684).  A bundle starts at its image[0]:
 *  with that base every backend's reads agree (the ARB view used to start one
 *  dword later and the NV view at texCoordComponentCount; tr_shader.c's parser
 *  view still starts one dword earlier).  Field sources: RB_GetAnimatedImage /
 *  RB_BindAnimatedImage (image, numImageAnimations, imageAnimationSpeed,
 *  waterMap, textureEnvMode, texEnvCombine, nvTextureShader, videoMapHandle,
 *  isLightmap, isVideoMap, clampAnimation), RB_ComputeTexCoords (tcGen,
 *  tcGenVectors, numTexMods) and the array setup (texCoordComponentCount).
 *  The TMU helpers in this file still read the bundles with raw byte offsets.
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
ASSERT_OFF(textureBundle_t, tcGen, 0x9c);
ASSERT_OFF(textureBundle_t, tcGenVectors, 0xa0);
ASSERT_OFF(textureBundle_t, numTexMods, 0xb8);
ASSERT_OFF(textureBundle_t, texMods, 0xbc);

typedef struct shaderStage_s {
	unsigned int  flags;                     /* +0x000  attributeBits: vertex-array + stage-type bits */
	textureBundle_t bundles[8];              /* +0x004  stride 0xC8 (the TMU helpers read them by offset) */
	int           atiFragmentShader;         /* +0x644  (tr_shader.c ParseATIFragmentShader) */
	void         *registerCombiners;         /* +0x648 */
	char         *vertexProgram;             /* +0x64c */
	byte          rgbWave[0x14];             /* +0x650  waveForm_t */
	int           rgbGen;                    /* +0x664  colour generator */
	byte          alphaWave[0x14];           /* +0x668  waveForm_t */
	int           alphaGen;                  /* +0x67c  alpha generator */
	byte          constantColor[4];          /* +0x680  RGBA constant colour */
	unsigned int  stateBits;                 /* +0x684  GL_State bits */
} shaderStage_t;
COD1_ASSERT_SIZE(shaderStage_t, 0x688);
ASSERT_OFF(shaderStage_t, flags, 0x000);
ASSERT_OFF(shaderStage_t, rgbGen, 0x664);
ASSERT_OFF(shaderStage_t, alphaGen, 0x67c);
ASSERT_OFF(shaderStage_t, constantColor, 0x680);
ASSERT_OFF(shaderStage_t, stateBits, 0x684);


/* retail's inlined float->int idiom in RB_ChooseSurfaceCountColor
 * (0x004FDCB1-0x004FDCBD and twice more): fld float / fadd qword 2^-30 /
 * fistp -- round-to-nearest under the default control word, not the truncating
 * __ftol2 an (int) cast compiles to.  The operand is a float, so the caller's
 * product is rounded to float first, as retail's fstp does. */
static int RB_FtolBiased( float f )
{
  double bias = 9.313225746154785e-10;
  int    i;

  __asm fld   f
  __asm fadd  bias
  __asm fistp i
  return i;
}

/* ---- RB_ChooseSurfaceCountColor  0x004FDC10 ----  VERIFIED */
void __cdecl RB_ChooseSurfaceCountColor( int indexCount, byte *color )
{
  unsigned int savedSeed;
  float        normal[3];

  if(r_showtris->integer>6){
    /* pseudo-random colour keyed on the shader + triangle count (rand() times the
     * float 1/32768 at retail 0x00568E98) */
    savedSeed = rand();
    srand( 31337 * ( indexCount + ((shader_t *)tess_shader)->sortedIndex ) );   /* shader.sortedIndex */
    normal[0] = rand() * 0.000030517578125f;
    normal[1] = rand() * 0.000030517578125f;
    normal[2] = rand() * 0.000030517578125f;
    VectorNormalize( normal );
    color[0] = (byte)RB_FtolBiased( normal[0] * 255.0f );
    color[1] = (byte)RB_FtolBiased( normal[1] * 255.0f );
    color[2] = (byte)RB_FtolBiased( normal[2] * 255.0f );
    srand( savedSeed );
    color[3] = -1;
  
  }else{
    /* fixed colour ramp keyed on the triangle/index count */
    if ( indexCount <= 30 )
    {
      color[0] = tr_identityLightByte;
      color[1] = tr_identityLightByte;
      color[2] = tr_identityLightByte;
      color[3] = -1;
    }
    else if ( indexCount <= 150 )
    {
      color[0] = tr_identityLightByte;
      color[1] = tr_identityLightByte >> 1;
      color[2] = tr_identityLightByte >> 1;
      color[3] = -1;
    }
    else if ( indexCount <= 450 )
    {
      color[0] = tr_identityLightByte;
      color[1] = 0;
      color[2] = 0;
      color[3] = -1;
    }
    else if ( indexCount <= 1200 )
    {
      color[0] = tr_identityLightByte;
      color[1] = tr_identityLightByte;
      color[2] = 0;
      color[3] = -1;
    }
    else if ( indexCount > 3000 )
    {
      color[0] = 0;
      color[1] = tr_identityLightByte;
      color[2] = 0;
      color[3] = -1;
    }
    else
    {
      color[0] = tr_identityLightByte >> 2;
      color[1] = tr_identityLightByte >> 2;
      color[2] = tr_identityLightByte;
      color[3] = -1;
    }
  
  }
}

/* ---- RB_SetSurfaceCountColor  0x004FDE30 ----  VERIFIED */
void __cdecl RB_SetSurfaceCountColor(int indexCount)
{
  GLubyte v[4]; // [esp+8h] [ebp-4h] BYREF

  RB_ChooseSurfaceCountColor(indexCount, v);
  glColor4ubv(v);
}

/* ---- RB_MakeNormalVectors  0x004FDE50 ----  VERIFIED
 * Given a unit `forward` vector, build an orthonormal `right` / `up` basis.
 * The basis is returned through the two output vectors.  This parameter order
 * gives retail's LTCG registers (forward ecx, up esi, right edx) and the
 * tangent-based loop in RB_BuildDlightArrays. */
void __cdecl RB_MakeNormalVectors( const float *forward, float *up, float *right )
{

  


  float  d, zz;
  float xx, xy, yy, zy;

  if ( forward[0] > -1.0f )
  {
    d  = 1.0f / ( forward[0] + 1.0f );
    yy = forward[1] * forward[1] * d;
    zy = forward[2] * forward[1] * d;
    zz = forward[2] * forward[2] * d;
    up[0]    = forward[1];
    up[1]    = yy - 1.0f;
    up[2]    = zy;
    right[0] = forward[2];
    right[1] = zy;
    right[2] = zz - 1.0f;
  }
  else
  {
    d  = 1.0f / ( forward[2] + 1.0f );
    xx = forward[0] * forward[0] * d;
    xy = forward[1] * forward[0] * d;
    yy = forward[1] * forward[1] * d;
    up[0]    = -xy;
    up[1]    = 1.0f - yy;
    up[2]    = -forward[1];
    right[0] = xx - 1.0f;
    right[1] = xy;
    right[2] = forward[0];
  }
}

/* ---- R_DrawStripElements  0x004FDF30 ----  VERIFIED
 * Feed a triangle-list index buffer to GL as one or more triangle strips,
 * starting a new strip whenever a triangle cannot extend the current one.
 * Returns the original index count (dead-EAX). */
void __cdecl R_DrawStripElements( unsigned short *indexes,
                                 void (__stdcall *arrayElement)(int),
                                 signed int numIndexes )
{
  int      i;
  int      last[3] = { -1, -1, -1 };
  qboolean even;

  c_begins++;

  if ( numIndexes <= 0 )
    return;

  qglBegin( 5 );                      /* GL_TRIANGLE_STRIP */

  /* prime the strip */
  arrayElement( indexes[0] );
  arrayElement( indexes[1] );
  arrayElement( indexes[2] );
  c_vertexes += 3;

  last[0] = indexes[0];
  last[1] = indexes[1];
  last[2] = indexes[2];

  even = qfalse;

  for ( i = 3; i < numIndexes; i += 3 )
  {
    /* odd numbered triangle in potential strip */
    if ( !even )
    {
      /* check previous triangle to see if we're continuing a strip */
      if ( ( indexes[i + 0] == last[2] ) && ( indexes[i + 1] == last[1] ) )
      {
        arrayElement( indexes[i + 2] );
        c_vertexes++;
        even = qtrue;
      }
      /* otherwise we're done with this strip so finish it and start a new one */
      else
      {
        qglEnd();

        qglBegin( 5 );
        c_begins++;

        arrayElement( indexes[i + 0] );
        arrayElement( indexes[i + 1] );
        arrayElement( indexes[i + 2] );

        c_vertexes += 3;

        even = qfalse;
      }
    }
    else
    {
      /* check previous triangle to see if we're continuing a strip */
      if ( ( last[2] == indexes[i + 1] ) && ( last[0] == indexes[i + 0] ) )
      {
        arrayElement( indexes[i + 2] );
        c_vertexes++;

        even = qfalse;
      }
      /* otherwise we're done with this strip so finish it and start a new one */
      else
      {
        qglEnd();

        qglBegin( 5 );
        c_begins++;

        arrayElement( indexes[i + 0] );
        arrayElement( indexes[i + 1] );
        arrayElement( indexes[i + 2] );
        c_vertexes += 3;

        even = qfalse;
      }
    }

    /* cache the last three vertices */
    last[0] = indexes[i + 0];
    last[1] = indexes[i + 1];
    last[2] = indexes[i + 2];
  }

  qglEnd();
}

/* ---- R_DrawElements  0x004FE070 ----  VERIFIED */
void __cdecl R_DrawElements( GLsizei numIndexes, GLvoid *indexes )
{
  int primitives = r_primitives->integer;

  /* r_primitives: 0 = auto, 1 = triangle strips, 2 = glDrawElements */
  if ( primitives == 0 )
  {
    if ( qglLockArraysEXT )
      primitives = 2;
    else
      primitives = 1;
  }
  if ( primitives == 2 )
  {
    backEnd_pc_drawnIndexCount += numIndexes;
    ++backEnd_pc_drawCallCount;
    glDrawElements( 4u, numIndexes, 0x1403u, indexes );   /* GL_TRIANGLES, GL_UNSIGNED_SHORT */
    return;
  }
  if ( primitives == 1 )
  {
    R_DrawStripElements( (unsigned short *)indexes,
                         (void (__stdcall *)(int))qglArrayElement, numIndexes );
    return;
  }
  /* any other r_primitives value draws nothing */
}

/* ---- RB_GetAnimatedImage  0x004FE0E0 ----  VERIFIED
 * Resolve the GL image handle for a texture bundle this frame: a cinematic,
 * a live water render, or the current frame of an animated image list.
 * (bundle is the bundle pointer; interior offsets are the disputed bundle layout.
 * a1 is never read: retail has only (bundle, tmu), but tr_xanim.c still
 * passes three arguments, so callers here pass 0.) */
int __cdecl RB_GetAnimatedImage( int tmu, int bundle )
{
  int   frame;
  int   frameCount;
  float t;

  if ( *(byte *)( bundle + 197 ) )                       /* isVideoMap */
  {
    ri.CIN_RunCinematic( *(unsigned int *)( bundle + 192 ) );
    ri.CIN_UploadCinematic( *(unsigned int *)( bundle + 192 ) );
    return tr_scratchImages[*(unsigned int *)( bundle + 192 )];
  }
  if ( *(unsigned int *)( bundle + 136 ) )                       /* waterMap */
  {
    RB_UploadWaterTexture( *(unsigned int *)( bundle + 136 ), tr_refdef_time, tmu );
    return *(unsigned int *)bundle;                              /* image[0] */
  }
  if ( *(int *)( bundle + 128 ) <= 1 )                     /* numAnimationFrames */
    return *(unsigned int *)bundle;

  t = tess_shaderTime * *(float *)( bundle + 132 ) * 1024.0f;  /* imageAnimationSpeed */
  /* inlined myftol (0x4FE154-0x4FE167): the float product through a plain
     round-to-nearest fistp into the static spill, then >> 10 -- no bias and
     no truncating __ftol2 */
  __asm fld   t
  __asm fistp ftol_tempSpill
  frame = ftol_tempSpill >> 10;
  if ( frame < 0 )
    frame = 0;
  if ( *(byte *)( bundle + 198 ) )                        /* clampAnimation */
  {
    frameCount = *(unsigned int *)( bundle + 128 );
    if ( frame >= frameCount )
      frame = frameCount - 1;
  }
  else
  {
    frame %= *(int *)( bundle + 128 );
  }
  return *(unsigned int *)( bundle + 4 * frame );
}

/* ---- RB_BindAnimatedImage  0x004FE1A0 ----  VERIFIED */
void __cdecl RB_BindAnimatedImage(void *this)
{
  GLenum *AnimatedImage;

  AnimatedImage = (GLenum *)RB_GetAnimatedImage( glState_currentTmu, (int)this );

  GL_Bind(AnimatedImage);
}

/* ---- RB_EnableTMU  0x004FE1C0 ----  VERIFIED
 * Bind the texture unit `tmu` to the bundle's animated image and texture
 * environment (plain, ARB combine, or NV texture shader).  `bundle` points at
 * the bundle interior; its sub-offsets are the disputed layout kept raw. */
void __cdecl RB_EnableTMU( int tmu, int bundle )
{
  unsigned int *image;
  GLenum  target;
  GLenum  prevTarget;
  int     texEnvMode;

  texEnvMode = *(unsigned int *)( bundle + 144 );           /* textureEnvMode */
  if ( *(byte *)( bundle + 196 ) )                   /* isLightmap */
  {
    if ( r_lightmap->integer )
      texEnvMode = 7681;                              /* GL_REPLACE */
  }
  image  = (unsigned int *)RB_GetAnimatedImage( tmu, bundle );
  target = image[21];                                 /* image->target */
  if ( glState_cap[tmu] != target
    || image[22] != glState_currentTextures[tmu]      /* image->texnum */
    || texEnvMode != glState_texEnv[tmu] )
  {
    if ( tmu != glState_currentTmu )
    {
      qglActiveTextureARB( tmu + 33984 );
      glState_currentTmu = tmu;
    }
    prevTarget = glState_cap[tmu];
    if ( prevTarget != target )
    {
      if ( prevTarget )
        glDisable( glState_cap[tmu] );
      glEnable( target );
      glState_cap[tmu] = target;
    }
    if ( image[22] != glState_currentTextures[tmu] )
    {
      image[23] = tr_frameCount;                      /* image->frameUsed */
      glBindTexture( target, image[22] );
      glState_currentTextures[tmu] = image[22];
    }
    if ( texEnvMode != glState_texEnv[tmu] )
    {
      glTexEnvi( 0x2300u, 0x2200u, texEnvMode );       /* GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE */
      glState_texEnv[tmu] = texEnvMode;
    }
  }
  if ( texEnvMode == 34160 )                           /* GL_COMBINE_ARB */
  {
    if ( tmu != glState_currentTmu )
    {
      qglActiveTextureARB( tmu + 33984 );
      glState_currentTmu = tmu;
    }
    /* upload the ARB combine environment from the bundle's combiner block (+148) */
    glTexEnvfv( 0x2300u, 0x2201u, *(const GLfloat **)( bundle + 148 ) );
    glTexEnvi( 0x2300u, 0x8571u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 16 ) );
    glTexEnvf( 0x2300u, 0x8573u, *(GLfloat *)( *(unsigned int *)( bundle + 148 ) + 44 ) );
    glTexEnvi( 0x2300u, 0x8580u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 20 ) );
    glTexEnvi( 0x2300u, 0x8581u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 24 ) );
    glTexEnvi( 0x2300u, 0x8582u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 28 ) );
    glTexEnvi( 0x2300u, 0x8590u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 32 ) );
    glTexEnvi( 0x2300u, 0x8591u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 36 ) );
    glTexEnvi( 0x2300u, 0x8592u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 40 ) );
    glTexEnvi( 0x2300u, 0x8572u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 48 ) );
    glTexEnvf( 0x2300u, 0xD1Cu, *(GLfloat *)( *(unsigned int *)( bundle + 148 ) + 76 ) );
    glTexEnvi( 0x2300u, 0x8588u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 52 ) );
    glTexEnvi( 0x2300u, 0x8589u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 56 ) );
    glTexEnvi( 0x2300u, 0x858Au, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 60 ) );
    glTexEnvi( 0x2300u, 0x8598u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 64 ) );
    glTexEnvi( 0x2300u, 0x8599u, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 68 ) );
    glTexEnvi( 0x2300u, 0x859Au, *(unsigned int *)( *(unsigned int *)( bundle + 148 ) + 72 ) );
  }
  if ( *(unsigned int **)( bundle + 152 )                     /* textureShader */
    && ( **(unsigned int **)( bundle + 152 ) || glState_textureShaderEnabled[tmu] ) )
  {
    if ( tmu != glState_currentTmu )
    {
      qglActiveTextureARB( tmu + 33984 );
      glState_currentTmu = tmu;
    }
    /* GL_TEXTURE_SHADER_NV, GL_SHADER_OPERATION_NV */
    glTexEnvi( 0x86DEu, 0x86DFu, **(unsigned int **)( bundle + 152 ) );
    switch ( **(unsigned int **)( bundle + 152 ) )
    {
      case 0x86E7:
        glTexEnvfv( 0x86DEu, 0x86E0u, (const GLfloat *)( *(unsigned int **)( bundle + 152 ) + 3 ) );
        break;
      case 0x86E9:
      case 0x86EA:
        glTexEnvi( 0x86DEu, 0x86E4u, ( *(unsigned int **)( bundle + 152 ) )[1] );
        break;
      case 0x86F3:
        glTexEnvfv( 0x86DEu, 0x86E5u, (const GLfloat *)( *(unsigned int **)( bundle + 152 ) + 3 ) );
        /* fall through */
      case 0x86EC:
      case 0x86EE:
      case 0x86F0:
      case 0x86F1:
      case 0x86F2:
        glTexEnvi( 0x86DEu, 0x86E4u, ( *(unsigned int **)( bundle + 152 ) )[1] );
        glTexEnvi( 0x86DEu, 0x86D9u, ( *(unsigned int **)( bundle + 152 ) )[2] );
        break;
      default:
        break;
    }
    glState_textureShaderEnabled[tmu] = **(unsigned int **)( bundle + 152 ) != 0;
  }
}

/* ---- RB_DisableTMU  0x004FE5C0 ----  VERIFIED */
void __cdecl RB_DisableTMU( int tmu )
{
  if ( !glState_cap[tmu] )
    return;

  if ( tmu != glState_currentTmu )
  {
    qglActiveTextureARB( tmu + 0x84C0 );
    glState_currentTmu = tmu;
  }

  glDisable( glState_cap[tmu] );
  glState_cap[tmu] = 0;

  if ( glState_textureShaderEnabled[tmu] )
  {
    glTexEnvi( 0x86DE, 0x86DF, 0 );
    glState_textureShaderEnabled[tmu] = 0;
  }
}

/* ---- RB_SetupTmu  0x004FE630 ----  VERIFIED */
void __cdecl RB_SetupTmu( int tmu, shaderStage_t *stage )
{
  if ( *(unsigned int *)( (byte *)stage + 200 * tmu + 148 ) )
    RB_EnableTMU( tmu, (int)( (byte *)stage + 200 * tmu + 4 ) );
  else
    RB_DisableTMU( tmu );
}

/* ---- RB_EnableClientTmu  0x004FE660 ----  VERIFIED */
void __cdecl RB_EnableClientTmu( int tmu, int bundle, GLvoid *pointer, GLsizei stride )
{
  if ( glState_currentClientTmu != tmu )
  {
    qglClientActiveTextureARB( tmu + 33984 );
    glState_currentClientTmu = tmu;
  }
  if ( ( glState_clientStateBits & ( 1 << glState_currentClientTmu ) ) == 0 )
  {
    glEnableClientState( 0x8078u );                   /* GL_TEXTURE_COORD_ARRAY */
    glState_clientStateBits |= 1 << glState_currentClientTmu;
  }
  glTexCoordPointer( *(unsigned int *)( bundle + 140 ), 0x1406u, stride, pointer );   /* texCoordComponentCount, GL_FLOAT */
}

/* ---- RB_DisableClientTmu  0x004FE6D0 ----  VERIFIED */
int __cdecl RB_DisableClientTmu( int tmu )
{
  int bit;

  bit = 1 << tmu;
  if ( bit & glState_clientStateBits )
  {
    if ( glState_currentClientTmu != tmu )
    {
      qglClientActiveTextureARB( tmu + 33984 );
      glState_currentClientTmu = tmu;
    }
    glDisableClientState( 0x8078u );
    glState_clientStateBits &= ~bit;
  }
  return glState_clientStateBits;
}

/* ---- RB_SetupClientTmu  0x004FE720 ----  VERIFIED */
void __cdecl RB_SetupClientTmu( int tmu, GLsizei stride, shaderStage_t *stage, GLvoid **texCoordPointers )
{
  /* bundle[tmu].image and &bundle[tmu] as two expressions (a shared unit local
     folds the add into the address and moves stage to another register) */
  if ( *(unsigned int *)( (byte *)stage + 200 * tmu + 148 ) )
    RB_EnableClientTmu( tmu, (int)( (byte *)stage + 200 * tmu + 4 ), texCoordPointers[tmu], stride );
  else
    RB_DisableClientTmu( tmu );
}

/* ---- RB_EnableClientTmuATI  0x004FE760 ----  VERIFIED */
void __cdecl RB_EnableClientTmuATI( int tmu, int bundle, int arrayObject, int offset, int stride )
{
  if ( glState_currentClientTmu != tmu )
  {
    qglClientActiveTextureARB( tmu + 33984 );
    glState_currentClientTmu = tmu;
  }
  if ( ( glState_clientStateBits & ( 1 << glState_currentClientTmu ) ) == 0 )
  {
    glEnableClientState( 0x8078u );
    glState_clientStateBits |= 1 << glState_currentClientTmu;
  }
  qglArrayObjectATI( 32888, *(unsigned int *)( bundle + 140 ), 5126, stride, arrayObject, offset );
}

/* ---- RB_SetupClientTmuATI  0x004FE7E0 ----  VERIFIED */
void __cdecl RB_SetupClientTmuATI( int tmu, int stride, shaderStage_t *stage, int arrayObject, int *offsets )
{
  if ( *(unsigned int *)( (byte *)stage + 200 * tmu + 148 ) )
    RB_EnableClientTmuATI( tmu, (int)( (byte *)stage + 200 * tmu + 4 ), arrayObject, offsets[tmu], stride );
  else
    RB_DisableClientTmu( tmu );
}

/* ---- RB_SetupRegisterCombiners  0x004FE820 ----  VERIFIED
 * Program the NV_register_combiners state machine from an opaque combiner
 * state block: the two constant colours, each general combiner input mapping
 * (four RGB and four alpha) and its RGB / alpha output, then the final
 * combiner. */
/* NV combiner records mirror the parser's ShaderNV* records in tr_shader.c.
 * Keep the public integer handle while accessing the verified record fields. */
typedef struct {
  int registerName;
  int inputMapping;
  int componentUsage;
} BackendNVRegisterMapping;
typedef struct {
  BackendNVRegisterMapping input[4];
  int output[3];
  int scale;
  int bias;
  unsigned char flags[3];
} BackendNVGeneralCombiner;
typedef struct {
  float constants[2][4];
  BackendNVGeneralCombiner portion[2];
} BackendNVGeneralBlock;
typedef struct {
  BackendNVRegisterMapping input[7];
  unsigned char clampColorSum;
} BackendNVFinalCombiner;
typedef struct {
  float constants[2][4];
  BackendNVGeneralBlock general[8];
  BackendNVFinalCombiner final;
  int numCombiners;
  qboolean perCombinerConstants;
} BackendNVCombinerState;
COD1_ASSERT_SIZE(BackendNVRegisterMapping,12);
COD1_ASSERT_SIZE(BackendNVGeneralCombiner,72);
COD1_ASSERT_SIZE(BackendNVGeneralBlock,176);
ASSERT_OFF(BackendNVCombinerState,general,32);
ASSERT_OFF(BackendNVCombinerState,final,1440);
ASSERT_OFF(BackendNVCombinerState,numCombiners,1528);
ASSERT_OFF(BackendNVCombinerState,perCombinerConstants,1532);
COD1_ASSERT_SIZE(BackendNVCombinerState,1536);
int __cdecl RB_SetupRegisterCombiners( int combiners )
{
  BackendNVCombinerState *state = (BackendNVCombinerState *)combiners;
  BackendNVGeneralBlock *block;
  BackendNVGeneralCombiner *rgb, *alpha;
  BackendNVRegisterMapping *rgbInput, *alphaInput;
  int i, j;
  glCombinerParameterfvNV(34090, state->constants[0]);
  glCombinerParameterfvNV(34091, state->constants[1]);
  glCombinerParameteriNV(34126, state->numCombiners);
  if (glConfig_NVRegisterCombiners >= 2) {
    if (state->perCombinerConstants) glEnable(0x8535u);
    else glDisable(0x8535u);
  }
  for (i=0; i<state->numCombiners; ++i) {
    block=&state->general[i];
    rgb=&block->portion[0];
    alpha=&block->portion[1];
    for (j=0; j<4; ++j) {
      rgbInput=&rgb->input[j];
      alphaInput=&alpha->input[j];
      glCombinerInputNV(i+34128,6407,j+34083,rgbInput->registerName,rgbInput->inputMapping,rgbInput->componentUsage);
      glCombinerInputNV(i+34128,6406,j+34083,alphaInput->registerName,alphaInput->inputMapping,alphaInput->componentUsage);
    }
    glCombinerOutputNV(i+34128,6407,
                       rgb->output[0],rgb->output[1],rgb->output[2],rgb->scale,rgb->bias,
                       rgb->flags[0],rgb->flags[1],rgb->flags[2]);
    glCombinerOutputNV(i+34128,6406,
                       alpha->output[0],alpha->output[1],alpha->output[2],alpha->scale,alpha->bias,
                       alpha->flags[0],alpha->flags[1],alpha->flags[2]);
    if (state->perCombinerConstants) {
      glCombinerStageParameterfvNV(i+34128,34090,state->general[i].constants[0]);
      glCombinerStageParameterfvNV(i+34128,34091,state->general[i].constants[1]);
    }
  }
  for(i=0;i<7;++i) {
    glFinalCombinerInputNV(i+34083,state->final.input[i].registerName,state->final.input[i].inputMapping,state->final.input[i].componentUsage);
  }
  return GL_CheckErrors("RB_SetupRegisterCombiners");
}

/* ---- RB_SetupVertexProgram  0x004FEA20 ----  VERIFIED
 * Bind the ARB vertex program and upload its local parameters: the local view
 * origin, and (for a dynamic light) the light origin, colour and attenuation. */
void __cdecl RB_SetupVertexProgram( int program )
{
  float local[4];

  if ( program != glState_boundVertexProgram )
  {
    glBindProgramARB( 34336, *(unsigned int *)( program + 64 ) );
    glState_boundVertexProgram = program;
  }
  GlobalPositionToLocal( backEnd_viewParms_originX, local );
  local[3] = 1.0;
  glProgramLocalParameter4fvARB( 34336, 0, local );
  if ( backEnd_currentDlight )
  {
    GlobalPositionToLocal( (float *)( backEnd_currentDlight + 68 ), local );
    local[3] = 1.0;
    glProgramLocalParameter4fvARB( 34336, 1, local );
    /* scaled colour is computed but the GL call below uses the UNSCALED
       pointer (backEnd_currentDlight + 36) -- retail */
    local[0] = backEnd_currentLightScale * *(float *)( backEnd_currentDlight + 36 );
    local[1] = backEnd_currentLightScale * *(float *)( backEnd_currentDlight + 40 );
    local[2] = backEnd_currentLightScale * *(float *)( backEnd_currentDlight + 44 );
    local[3] = backEnd_currentLightScale * *(float *)( backEnd_currentDlight + 48 );
    glProgramLocalParameter4fvARB( 34336, 2, (const GLfloat *)( backEnd_currentDlight + 36 ) );
    glProgramLocalParameter4fARB( 34336, 3,
                                  *(float *)( backEnd_currentDlight + 96 ),
                                  *(float *)( backEnd_currentDlight + 100 ),
                                  *(float *)( backEnd_currentDlight + 104 ),
                                  1.0f );
  }
}

/* ---- GL_BindFragmentShaderATI  0x004FEB10 ----  VERIFIED */
void __cdecl GL_BindFragmentShaderATI(int shaderId)
{
  if ( shaderId != glState_boundFragmentShader )
  {
    glBindFragmentShaderATI(shaderId);
    glState_boundFragmentShader = shaderId;
  }
}

/* ---- RB_SetupMultitexture  0x004FEB30 ----  VERIFIED
 * Configure every texture unit for a shader stage: the current client / server
 * units first, then the remaining ones, then the register combiners, vertex
 * program and fragment shader.  The per-unit helpers are inlined (their
 * out-of-line copies have no callers in retail). */
void __cdecl RB_SetupMultitexture( unsigned int *stage, GLvoid **texCoordPointers, GLsizei stride )
{
  int clientTmu;
  int serverTmu;
  int tmu;

  clientTmu = glState_currentClientTmu;
  RB_SetupClientTmu( glState_currentClientTmu, stride, (shaderStage_t *)stage, texCoordPointers );
  serverTmu = glState_currentTmu;
  RB_SetupTmu( glState_currentTmu, (shaderStage_t *)stage );
  for ( tmu = 0; tmu < glConfig_maxActiveTextures; tmu++ )                      /* glConfig.maxActiveTextures */
  {
    if ( tmu != clientTmu )
    {
      if ( stage[50 * tmu + 37] )
        RB_EnableClientTmu( tmu, (int)&stage[50 * tmu + 1], texCoordPointers[tmu], stride );
      else if ( ( ( 1 << tmu ) & glState_clientStateBits ) != 0 )
      {
        if ( glState_currentClientTmu != tmu )
        {
          qglClientActiveTextureARB( tmu + 33984 );
          glState_currentClientTmu = tmu;
        }
        glDisableClientState( 0x8078u );
        glState_clientStateBits &= ~( 1 << tmu );
      }
    }
    if ( tmu != serverTmu )
      RB_SetupTmu( tmu, (shaderStage_t *)stage );
  }
  if ( stage[402] )                                        /* registerCombiners */
    RB_SetupRegisterCombiners( stage[402] );
  if ( stage[403] )                                        /* vertexProgram */
    RB_SetupVertexProgram( stage[403] );
  GL_BindFragmentShaderATI( stage[401] );                  /* fragmentShaderATI */
}

/* ---- RB_SetupMultitextureATI  0x004FECB0 ----  VERIFIED
 * ATI array-object variant of RB_SetupMultitexture. */
void __cdecl RB_SetupMultitextureATI( unsigned int *stage, int arrayObject, int *offsets, int stride )
{
  int clientTmu;
  int serverTmu;
  int tmu;

  clientTmu = glState_currentClientTmu;
  RB_SetupClientTmuATI( glState_currentClientTmu, stride, (shaderStage_t *)stage, arrayObject, offsets );
  serverTmu = glState_currentTmu;
  RB_SetupTmu( glState_currentTmu, (shaderStage_t *)stage );
  for ( tmu = 0; tmu < glConfig_maxActiveTextures; tmu++ )
  {
    if ( tmu != clientTmu )
    {
      if ( stage[50 * tmu + 37] )
        RB_EnableClientTmuATI( tmu, (int)&stage[50 * tmu + 1], arrayObject, offsets[tmu], stride );
      else if ( ( ( 1 << tmu ) & glState_clientStateBits ) != 0 )
      {
        if ( glState_currentClientTmu != tmu )
        {
          qglClientActiveTextureARB( tmu + 33984 );
          glState_currentClientTmu = tmu;
        }
        glDisableClientState( 0x8078u );
        glState_clientStateBits &= ~( 1 << tmu );
      }
    }
    if ( tmu != serverTmu )
      RB_SetupTmu( tmu, (shaderStage_t *)stage );
  }
  if ( stage[402] )
    RB_SetupRegisterCombiners( stage[402] );
  if ( stage[403] )
    RB_SetupVertexProgram( stage[403] );
  GL_BindFragmentShaderATI( stage[401] );
}

/* ---- RB_EndMultitexture  0x004FEE40 ----  VERIFIED
 * Disable every texture unit above 0, return to unit 0 and restore GL_MODULATE. */
void RB_EndMultitexture( void )
{
  int i;

  for ( i = 1; i < glConfig_maxActiveTextures; ++i )
  {
    if ( ( glState_clientStateBits & ( 1 << i ) ) != 0 )
    {
      if ( glState_currentClientTmu != i )
      {
        qglClientActiveTextureARB( i + 33984 );
        glState_currentClientTmu = i;
      }
      glDisableClientState( 0x8078u );
      glState_clientStateBits &= ~( 1 << i );
    }
    RB_DisableTMU( i );
  }
  if ( glState_currentTmu )
  {
    qglActiveTextureARB( 33984 );
    glState_currentTmu = 0;
  }
  if ( glState_currentClientTmu )
  {
    qglClientActiveTextureARB( 33984 );
    glState_currentClientTmu = 0;
  }
  if ( glState_texEnv[glState_currentTmu] != 8448 )         /* GL_MODULATE */
  {
    glState_texEnv[glState_currentTmu] = 8448;
    glTexEnvi( 0x2300u, 0x2200u, 8448 );
  }
}

/* ---- RB_SetupStage  0x004FEF20 ----  VERIFIED */
void __cdecl RB_SetupStage( shaderStage_t *stage, int texCoordPointers, GLsizei stride )
{
  GL_State( stage->stateBits );
  RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)texCoordPointers, stride );
}

/* ---- RB_SetupStageATI  0x004FEF40 ----  VERIFIED */
void __cdecl RB_SetupStageATI( shaderStage_t *stage, int arrayObject, int offsets, int stride )
{
  GL_State( stage->stateBits );
  RB_SetupMultitextureATI( (unsigned int *)stage, arrayObject, (int *)offsets, stride );
}

/* ---- DrawTris  0x004FEF70 ----  VERIFIED
 * Re-draw the current surface's triangles with the r_showtris debug shader.
 * (only input->entity is read through the pointer; the rest are tess_ globals.) */
void __cdecl DrawTris( shaderCommands_t *input )
{
  int savedStageCount;
  int savedShader;
  int savedStages;

  if ( r_showtris->integer > 2
    || ( (void *)input->entity != (void *)&backEnd_entity2D
      && (void *)input->entity != (void *)&backEnd_debugEntity ) )
  {
    savedShader     = tess_shader;
    savedStages     = tess_activeStages;
    savedStageCount = tess_activeStageCount;
    tess_shader           = tr_showTrisShader;
    tess_activeStages     = tr_showTrisShader + 340;                    /* showTris->stages */
    tess_activeStageCount = ((shader_t *)tr_showTrisShader)->numUnfoggedPasses;     /* showTris->numStages */
    if ( ( r_showtris->integer & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f = 0x3FB99999A0000000, retail 0x004FEFD8 */
    tr_stageIteratorFunc( 0 );
    glDepthRange( 0.0, 1.0 );
    tess_activeStages     = savedStages;
    tess_shader           = savedShader;
    tess_activeStageCount = savedStageCount;
  }
}

/* ---- DrawNormals  0x004FF020 ----  VERIFIED
 * r_shownormals "white": draw the normal (and, when valid, tangent and
 * bitangent) at each vertex as coloured lines. */
void __cdecl DrawNormals( shaderCommands_t *input )
{
  int    i;
  float *xyz;
  float  end[3];

  if ( !input->requiresVertexBasis )
    return;

  RB_BeginImmediateMode();
  glDepthRange( 0.0, 0.9990000128746033 );
  GL_Bind( (GLenum *)tr_whiteImage );
  rendererDebugState.immediateColorR = -1;
  rendererDebugState.immediateColorG = -1;
  rendererDebugState.immediateColorB = -1;
  rendererDebugState.immediateColorA = -1;
  if ( !input->stageBitangentsValid || !input->stageTangentsValid )
    RB_CalcTangentSpace();
  if ( r_shownormals->integer == 1 )
    glDepthRange( 0.0, 0.0 );
  GL_State( 4352 );
  rendererDebugState.immediateMode = 1;

  xyz = tess_xyz;
  for ( i = 0; i < input->numVertexes; i++ )
  {
    if ( input->stageTangentsValid && input->stageBitangentsValid )
    {
      rendererDebugState.immediateColorR = -1;
      rendererDebugState.immediateColorG = 127;
      rendererDebugState.immediateColorB = 127;
      rendererDebugState.immediateColorA = -1;
      RB_glVertex3fv( xyz );
      VectorMA( xyz, 2, &input->stageTangents[3 * i], end );
      RB_glVertex3fv( end );

      rendererDebugState.immediateColorR = 127;
      rendererDebugState.immediateColorG = -1;
      rendererDebugState.immediateColorB = 127;
      rendererDebugState.immediateColorA = -1;
      RB_glVertex3fv( xyz );
      VectorMA( xyz, 2, &input->stageBitangents[3 * i], end );
      RB_glVertex3fv( end );

      rendererDebugState.immediateColorR = 127;
      rendererDebugState.immediateColorG = 127;
      rendererDebugState.immediateColorB = -1;
      rendererDebugState.immediateColorA = -1;
    }

    RB_glVertex3fv( xyz );
    VectorMA( xyz, 2, &input->stageNormals[3 * i], end );
    RB_glVertex3fv( end );

    if ( (unsigned char)i == 0xFF )                 /* flush the immediate buffer every 256 verts */
    {
      glDrawArrays( rendererDebugState.immediateMode, 0, rendererDebugState.immediateVertexCount );
      rendererDebugState.immediateVertexCount = 0;
      rendererDebugState.immediateMode = 1;
    }
    xyz += tess_vertexComponentCount;
  }

  glDrawArrays( rendererDebugState.immediateMode, 0, rendererDebugState.immediateVertexCount );
  rendererDebugState.immediateVertexCount = 0;
  rendererDebugState.immediateMode = 0;
  glDepthRange( 0.0, 1.0 );
  rendererDebugState.immediateModeActive = 0;
  if ( !backEnd_projection2D && ( glState_glStateBits & 0x200000 ) == 0 && r_fog->integer )
  {
    int fogNum = tr_fogPortalView_registered;
    if ( ( backEnd_refdef_rdflags & 8 ) == 0 )
      fogNum = glfogNum;
    if ( fogNum )
    {
      glEnable( 0xB60u );                            /* GL_FOG */
      glState_glStateBits |= 0x200000u;
    }
  }
  glVertexPointer( 3, 0x1406u, 0, tess_xyz );
}

/* ---- DrawColoredNormals  0x004FF310 ----  VERIFIED
 * r_shownormals "color": draw each vertex normal as a line coloured by the
 * normal direction.  RETAIL BUG: the green / blue channels read normal[3] /
 * normal[6] (the X of the next two vertices' normals) instead of normal[1] /
 * normal[2]; reproduced as shipped. */
void __cdecl DrawColoredNormals( shaderCommands_t *input )
{
  int    i;
  int   *xyz;

  RB_BeginImmediateMode();
  GL_Bind( (GLenum *)tr_whiteImage );
  rendererDebugState.immediateColorR = -1;
  rendererDebugState.immediateColorG = -1;
  rendererDebugState.immediateColorB = -1;
  rendererDebugState.immediateColorA = -1;
  GL_State( 4352 );
  rendererDebugState.immediateMode = 1;

  xyz = (int *)tess_xyz;
  for ( i = 0; i < input->numVertexes; i++ )
  {
    const float *pos    = (const float *)xyz;
    rfloat       r, g, b;
    float        end[3];

    b = ( (rfloat)input->stageNormals[3 * i + 6] + 1.0f ) * 0.5f;   /* retail bug */
    g = ( (rfloat)input->stageNormals[3 * i + 3] + 1.0f ) * 0.5f;   /* retail bug */
    r = ( (rfloat)input->stageNormals[3 * i + 0] + 1.0f ) * 0.5f;
    rendererDebugState.immediateColorR = (unsigned char)( r * 255.0f );
    rendererDebugState.immediateColorG = (unsigned char)( g * 255.0f );
    rendererDebugState.immediateColorB = (unsigned char)( b * 255.0f );
    rendererDebugState.immediateColorA = -1;

    RB_glVertex3fv( pos );
    VectorMA( pos, 2, &input->stageNormals[3 * i], end );
    RB_glVertex3fv( end );

    xyz += tess_vertexComponentCount;
  }

  glDrawArrays( rendererDebugState.immediateMode, 0, rendererDebugState.immediateVertexCount );
  rendererDebugState.immediateVertexCount = 0;
  rendererDebugState.immediateMode = 0;
  glDepthRange( 0.0, 1.0 );
  rendererDebugState.immediateModeActive = 0;
  if ( !backEnd_projection2D && ( glState_glStateBits & 0x200000 ) == 0 && r_fog->integer )
  {
    int fogNum = tr_fogPortalView_registered;
    if ( ( backEnd_refdef_rdflags & 8 ) == 0 )
      fogNum = glfogNum;
    if ( fogNum )
    {
      glEnable( 0xB60u );
      glState_glStateBits |= 0x200000u;
    }
  }
  glVertexPointer( 3, 0x1406u, 0, tess_xyz );
}

/* ---- RB_SelectStorageATI  0x004FF4E0 ----  VERIFIED */
void __cdecl RB_SelectStorageATI(int storageMode)
{
  if ( storageMode == 3 )
  {
    glVertexPointer(tess_vertexComponentCount, 0x1406u, 0, tess_xyz);
    glNormalPointer(0x1406u, 12, tess_stageNormals);
    /* 0x4FF50F `mov ecx, tess_activeTexCoords`: slot 0's pointer, not the table */
    glTexCoordPointer(2, 0x1406u, 0, tess_activeTexCoords[0]);
  }
}

/* ---- RB_SelectStorage  0x004FF530 ----  VERIFIED */
void __cdecl RB_SelectStorage(int storageMode)
{
  if ( storageMode != glState_currentStorageMode )
  {
    if ( glConfig_NVVertexArrayRange )
    {
      RB_SelectStorageNV(storageMode);
      glState_currentStorageMode = storageMode;
    }
    else
    {
      if ( glConfig_ATIVertexArrayObject )
        RB_SelectStorageATI(storageMode);
      glState_currentStorageMode = storageMode;
    }
  }
}



/* ---- RB_BeginSurface  0x004FF570 ----  VERIFIED */
void __cdecl RB_BeginSurface( void *shader, int vertexComponentCount )
{
  int *shd = (int *)shader;

  tess_numIndexes = 0;
  tess_numVertexes = 0;
  tess_optimizedFirstVertex = 0;
  tess_optimizedVertexEnd = 0;
  tess_dlightBits = 0;

  tess_shader = (int)shd;
  tess_activeStages = (int)shd + 0x154;
  tess_activeStageCount = shd[0x150 / 4];
  tess_stageIterator = *(int (**)())&shd[0x174 / 4];
  tess_entity = backEnd_currentEntity;
  tess_requiresVertexBasis = ( (unsigned int)shd[0x50 / 4] >> 1 ) & 1;
  tess_vertexComponentCount = vertexComponentCount;

  tess_stageBitangentsValid = 0;
  tess_stageTangentsValid = 0;
  tess_renderedIndexCount = 0;
  tess_renderedVertexCount = 0;

  tess_shaderTime = backEnd_refdef_floatTime - *(float *)&shd[0x17C / 4];

  if ( *(float *)&shd[0x178 / 4] != 0.0f
    && tess_shaderTime >= *(float *)&shd[0x178 / 4] )
  {
    tess_shaderTime = *(float *)&shd[0x178 / 4];
  }
}

/* ---- DrawMultitextured  0x004FF640 ----  VERIFIED */
void __cdecl DrawMultitextured( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indexes )
{
  GL_State( stage->stateBits );
  if ( backEnd_wireframeOverride )
    glPolygonMode( 0x408u, 0x1B02u );          /* GL_FRONT_AND_BACK, GL_LINE */
  RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)&tess_activeTexCoords, 0 );   /* 0x4FF666 `push offset`: the table address */
  R_DrawElements( numIndexes, indexes );
}

int __cdecl myftol( float a1 );   /* tr_shade_calc.c 0x00515940; LTCG inlines it */

/* ---- RB_BuildDlightArrays  0x004FF690 ----  VERIFIED
 * RTCW's ProjectDlightTexture vertex and triangle passes for one light: the
 * light's texture coordinates, modulated colours (myftol, inlined) and the
 * list of triangles that are not entirely clipped.  The texCoords, colors
 * parameter order gives retail's LTCG registers (colors in ecx, texCoords in
 * edx at every caller). */
int __cdecl RB_BuildDlightArrays( float *texCoords, byte *colors, float *dlight, unsigned short *hitIndexes )
{
  const dlight_t *dl;
  int     i;
  int     numIndexes;
  float  *xyz;
  float  *tangent;
  float  *bitangent;
  float  *normal;
  vec3_t  origin;
  vec3_t  delta;
  vec3_t  dist;
  vec3_t  floatColor;
  float   radius;
  float   scale;
  float   modulate;
  char    clip;
  byte   clipBits[8192];             /* SHADER_MAX_VERTEXES */

  dl = (const dlight_t *)dlight;
  VectorCopy( dl->transformedPosition, origin );
  radius = dl->radius;
  scale = 1.0f / radius;
  floatColor[0] = dl->color[0] * 255.0f;
  floatColor[1] = dl->color[1] * 255.0f;
  floatColor[2] = dl->color[2] * 255.0f;

  if ( r_dlightQuality->integer && tess_requiresVertexBasis )
  {
    if ( !tess_stageBitangentsValid || !tess_stageTangentsValid )
    {
      for ( i = 0; i < tess_numVertexes; i++ )
      {
        RB_MakeNormalVectors( &tess_stageNormals[3 * i], &tess_stageTangents[3 * i], &tess_stageBitangents[3 * i] );
      }
      tess_stageBitangentsValid = 1;
      tess_stageTangentsValid = 1;
    }
  }

  xyz = tess_xyz;
  for ( i = 0; i < tess_numVertexes; i++, texCoords += 2, colors += 4, xyz += tess_vertexComponentCount )
  {
    if ( r_dlightQuality->integer && tess_requiresVertexBasis )
    {
      tangent   = &tess_stageTangents[3 * i];
      bitangent = &tess_stageBitangents[3 * i];
      normal    = &tess_stageNormals[3 * i];
      VectorSubtract( origin, xyz, delta );
      dist[0] = delta[2] * tangent[2] + delta[0] * tangent[0] + delta[1] * tangent[1];
      dist[1] = delta[2] * bitangent[2] + delta[1] * bitangent[1] + delta[0] * bitangent[0];
      dist[2] = delta[2] * normal[2] + delta[1] * normal[1] + delta[0] * normal[0];
    }
    else
    {
      VectorSubtract( origin, xyz, dist );
    }

    backEnd_pc_dlightVertexCount++;

    texCoords[0] = 0.5f + dist[0] * scale;
    texCoords[1] = 0.5f + dist[1] * scale;

    clip = 0;
    if ( texCoords[0] < 0.0f )
      clip |= 1;
    else if ( texCoords[0] > 1.0f )
      clip |= 2;

    if ( texCoords[1] < 0.0f )
      clip |= 4;
    else if ( texCoords[1] > 1.0f )
      clip |= 8;

    /* modulate the strength based on the height and color */
    if ( dist[2] > radius )
    {
      clip |= 0x10;
      modulate = 0.0f;
    }
    else if ( dist[2] < -radius )
    {
      clip |= 0x20;
      modulate = 0.0f;
    }
    else
    {
      {
        /* Q_fabs */
        float f = dist[2];
        int   tmp = *(int *)&f & 0x7FFFFFFF;
        dist[2] = *(float *)&tmp;
      }
      if ( dist[2] < radius * 0.5f )
        modulate = 1.0f;
      else
      {
        modulate = ( radius - dist[2] ) * scale;
        modulate = modulate + modulate;
      }
    }

    clipBits[i] = clip;
    colors[0] = (byte)myftol( modulate * floatColor[0] );
    colors[1] = (byte)myftol( modulate * floatColor[1] );
    colors[2] = (byte)myftol( modulate * floatColor[2] );
    colors[3] = 0xFF;

  }

  /* build a list of triangles that need light */
  numIndexes = 0;
  for ( i = 0; i < tess_numIndexes; i += 3 )
  {
    unsigned short a, b, c;

    a = tess_indexes[i];
    b = tess_indexes[i + 1];
    c = tess_indexes[i + 2];
    if ( clipBits[a] & clipBits[b] & clipBits[c] )
      continue;   /* not lighted */
    hitIndexes[numIndexes] = a;
    hitIndexes[numIndexes + 1] = b;
    hitIndexes[numIndexes + 2] = c;
    numIndexes += 3;
  }
  return numIndexes;
}

/* ---- RB_ComputeColors  0x004FFA60 ----  VERIFIED
 * stage->rgbGen / alphaGen / constantColor are re-read at each use and the
 * colour arrays are addressed directly, as retail. */
#define RB_COLORS        ( (unsigned char (*)[4])tess_stageVertexColors )
#define RB_VERTEXCOLORS  ( (const unsigned char (*)[4])tess_vertexColors )
#define RB_STAGE_RGBGEN      ( *(int *)( stage + 1636 ) )
#define RB_STAGE_ALPHAGEN    ( *(int *)( stage + 1660 ) )
#define RB_STAGE_CONSTCOLOR  ( *(int *)( stage + 1664 ) )

/* alphaGen 10-13 (view-angle alpha): retail expands one specialised copy
 * per alphaGen in place, each with its own local view origin */
#define RB_VIEW_ANGLE_ALPHA( alphaGen ) \
{ \
  float        direction[3]; \
  float        localViewOrigin[3]; \
  const float *xyz; \
  const float *normal; \
  int          i; \
 \
  /* the local view origin (retail inlines this per copy; VectorRotate stays a call) */ \
  if ( (void *)backEnd_currentEntity == (void *)tr_worldEntity ) \
  { \
    localViewOrigin[0] = backEnd_viewParms_originX[0]; \
    localViewOrigin[1] = backEnd_viewParms_originY; \
    localViewOrigin[2] = backEnd_viewParms_originZ; \
  } \
  else \
  { \
    direction[0] = backEnd_viewParms_originX[0] - ((trRefEntity_t *)backEnd_currentEntity)->e.origin[0]; \
    direction[1] = backEnd_viewParms_originY    - ((trRefEntity_t *)backEnd_currentEntity)->e.origin[1]; \
    direction[2] = backEnd_viewParms_originZ    - ((trRefEntity_t *)backEnd_currentEntity)->e.origin[2]; \
    VectorRotate( direction, (float *)( backEnd_currentEntity + 28 ), localViewOrigin ); \
  } \
 \
  xyz    = tess_xyz; \
  normal = tess_stageNormals; \
  for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount, normal += 3 ) \
  { \
    float  alpha; \
 \
    direction[0] = localViewOrigin[0] - xyz[0]; \
    direction[1] = localViewOrigin[1] - xyz[1]; \
    direction[2] = localViewOrigin[2] - xyz[2]; \
    VectorNormalizeFast( direction ); \
 \
    if ( alphaGen == 12 ) \
    { \
      alpha = direction[0] * normal[0] + direction[2] * normal[2] \
            + direction[1] * normal[1]; \
    } \
    else \
    { \
      alpha = direction[2] * normal[2] + direction[0] * normal[0] \
            + direction[1] * normal[1]; \
      if ( alphaGen == 13 ) \
        alpha = -alpha; \
    } \
 \
    if ( alphaGen == 12 ) \
      *(int *)&alpha &= ( *(int *)&alpha >> 31 ); \
    else \
      *(int *)&alpha &= ~( *(int *)&alpha >> 31 ); \
 \
    if ( alphaGen == 11 ) \
      RB_COLORS[i][3] = (unsigned char)(int)( ( 1.0f - alpha ) * 255.0f + 0.5f ); \
    else if ( alphaGen == 12 ) \
      RB_COLORS[i][3] = (unsigned char)(int)( ( alpha + 1.0f ) * 255.0f + 0.5f ); \
    else \
      RB_COLORS[i][3] = (unsigned char)(int)( alpha * 255.0f + 0.5f ); \
  } \
}

void __cdecl RB_ComputeColors( int stage )
{
  int          i;
  const float *xyz;

  switch ( RB_STAGE_RGBGEN )
  {
    case 9:
    case 10:
      if ( *(int *)( tr_world + 280 ) )
        memset( RB_COLORS, 0xFF, 4 * tess_numVertexes );
      else
        RB_CalcDiffuseColor( RB_COLORS );
      break;

    case 2:
      memset( RB_COLORS, 0xFF, 4 * tess_numVertexes );
      break;

    case 12:
      for ( i = 0; i < tess_numVertexes; i++ )
        *(int *)RB_COLORS[i] = RB_STAGE_CONSTCOLOR;
      break;

    case 6:
      if ( !tr_overbrightBits )
      {
        memcpy( RB_COLORS, RB_VERTEXCOLORS, 4 * tess_numVertexes );
      }
      else
      {
        for ( i = 0; i < tess_numVertexes; i++ )
        {
          RB_COLORS[i][0] = (unsigned char)( RB_VERTEXCOLORS[i][0] >> tr_overbrightBits );
          RB_COLORS[i][1] = (unsigned char)( RB_VERTEXCOLORS[i][1] >> tr_overbrightBits );
          RB_COLORS[i][2] = (unsigned char)( RB_VERTEXCOLORS[i][2] >> tr_overbrightBits );
          RB_COLORS[i][3] = RB_VERTEXCOLORS[i][3];
        }
      }
      break;

    case 5:
      memcpy( RB_COLORS, RB_VERTEXCOLORS, 4 * tess_numVertexes );
      break;

    case 7:
      if ( tr_identityLight == 1.0f )
      {
        for ( i = 0; i < tess_numVertexes; i++ )
        {
          RB_COLORS[i][0] = (unsigned char)( 255 - RB_VERTEXCOLORS[i][0] );
          RB_COLORS[i][1] = (unsigned char)( 255 - RB_VERTEXCOLORS[i][1] );
          RB_COLORS[i][2] = (unsigned char)( 255 - RB_VERTEXCOLORS[i][2] );
        }
      }
      else
      {
        for ( i = 0; i < tess_numVertexes; i++ )
        {
          RB_COLORS[i][0] = (unsigned char)(int)( (float)( 255 - RB_VERTEXCOLORS[i][0] ) * tr_identityLight );
          RB_COLORS[i][1] = (unsigned char)(int)( (float)( 255 - RB_VERTEXCOLORS[i][1] ) * tr_identityLight );
          RB_COLORS[i][2] = (unsigned char)(int)( (float)( 255 - RB_VERTEXCOLORS[i][2] ) * tr_identityLight );
        }
      }
      break;

    case 8:
      RB_CalcWaveColor( RB_COLORS, (void *)( stage + 1616 ) );
      break;

    case 3:
      RB_CalcColorFromEntity( (unsigned char *)RB_COLORS );
      break;

    case 4:
      RB_CalcColorFromOneMinusEntity( RB_COLORS );
      break;

    case 13:
      if ( r_showtris->integer >= 5 )
        RB_ChooseSurfaceCountColor( tess_numIndexes, RB_COLORS[0] );
      else
        *(int *)RB_COLORS[0] = RB_STAGE_CONSTCOLOR;
      for ( i = 1; i < tess_numVertexes; i++ )
        *(int *)RB_COLORS[i] = *(int *)RB_COLORS[0];
      break;

    default:
      memset( RB_COLORS, tr_identityLightByte, 4 * tess_numVertexes );
      break;
  }

  switch ( RB_STAGE_ALPHAGEN )
  {
    case 0:
      if ( RB_STAGE_RGBGEN == 2 )
        return;
      if ( RB_STAGE_RGBGEN == 6 && tr_identityLight == 1.0f )
        return;
      for ( i = 0; i < tess_numVertexes; i++ )
        RB_COLORS[i][3] = 0xFF;
      return;

    case 9:
      if ( RB_STAGE_RGBGEN != 12 )
      {
        for ( i = 0; i < tess_numVertexes; i++ )
          RB_COLORS[i][3] = *(unsigned char *)( stage + 1667 );
      }
      return;

    case 7:
      RB_CalcWaveAlpha( (void *)( stage + 1640 ), RB_COLORS );
      return;

    case 6:
      RB_CalcSpecularAlpha( (int)RB_COLORS );
      return;

    case 2:
      RB_CalcAlphaFromEntity( (unsigned char *)RB_COLORS );
      return;

    case 3:
      RB_CalcAlphaFromOneMinusEntity( (unsigned char *)RB_COLORS );
      return;

    case 4:
      if ( RB_STAGE_RGBGEN != 6 )
      {
        for ( i = 0; i < tess_numVertexes; i++ )
          RB_COLORS[i][3] = RB_VERTEXCOLORS[i][3];
      }
      return;

    case 5:
      for ( i = 0; i < tess_numVertexes; i++ )
        RB_COLORS[i][3] = (unsigned char)( 255 - RB_VERTEXCOLORS[i][3] );
      return;

    case 8:
      xyz = tess_xyz;
      for ( i = 0; i < tess_numVertexes; i++, xyz += tess_vertexComponentCount )
      {
        unsigned char value;
        /* 0x4FFEA0-0x4FFEDE: dx/dy/dz stay on the x87 stack (never rounded
           to float); only the quotient is stored as a float */
        double dx = (double)xyz[0] - backEnd_viewParms_originX[0];
        double dy = (double)xyz[1] - backEnd_viewParms_originY;
        double dz = (double)xyz[2] - backEnd_viewParms_originZ;
        float alpha;

        alpha = (float)sqrt( dz * dz + dy * dy + dx * dx );
        alpha = alpha / ((shader_t *)tess_shader)->portalRange;

        if ( alpha < 0.0f )
          value = 0;
        else if ( alpha > 1.0f )
          value = 0xFF;
        else
          value = (unsigned char)(int)( alpha * 255.0f );
        RB_COLORS[i][3] = value;
      }
      return;

    case 10:
      RB_VIEW_ANGLE_ALPHA( 10 )
      return;

    case 11:
      RB_VIEW_ANGLE_ALPHA( 11 )
      return;

    case 12:
      RB_VIEW_ANGLE_ALPHA( 12 )
      return;

    case 13:
      RB_VIEW_ANGLE_ALPHA( 13 )
      return;

    default:
      return;
  }
}
#undef RB_VIEW_ANGLE_ALPHA
#undef RB_COLORS
#undef RB_VERTEXCOLORS
#undef RB_STAGE_RGBGEN
#undef RB_STAGE_ALPHAGEN
#undef RB_STAGE_CONSTCOLOR

/* ---- RB_ComputeTexCoords  0x00500400 ----  VERIFIED
 * For each of the 8 texture units, generate that unit's texture coordinates
 * (from a tess array or a tcGen function) into per-unit scratch and point the
 * active-texcoord slot at them, then apply the unit's texture-coordinate
 * modifiers. The bundle cursor uses the existing stage layout for tcGen, its
 * vectors and texture modifiers. `scratch` mirrors a reused register kept for
 * the (immaterial) case-15 argument. */
void __cdecl RB_ComputeTexCoords( int stage )
{
  int      texUnit;
  int      tcGen;
  int      scratch;             /* ecx scratch in retail */

  textureBundle_t *bundle;        /* stage + 4 + 200*unit */
  unsigned int  *generated;      /* per-unit generated-texcoord scratch */
  GLvoid **active;         /* &tess_activeTexCoords[unit] */
  int      numMods;
  int      m;
  int      b;

  scratch   = 0;
  bundle = ((shaderStage_t *)stage)->bundles;
  for ( b = 0; b < 8; b++ )
  {
    generated = (unsigned int *)&tess_generatedTexCoords[0x20000 * b];
    active    = &tess_activeTexCoords[b];
    *active   = generated;
    tcGen   = bundle->tcGen;

    switch ( tcGen )
    {
      case 0:                                     /* stop */
        return;
      case 1:
        memset( generated, 0, 8 * tess_numVertexes );
        scratch = 0;
        break;
      case 2:                                     /* lightmap coords */
        if ( setArraysOnce || bundle->numTexMods )
        {
          memcpy( generated, tess_texCoords1, 8 * tess_numVertexes );
          scratch = 0;
        }
        else
        {
          *active = tess_texCoords1;
        }
        break;
      case 3:                                     /* base coords */
        if ( setArraysOnce || bundle->numTexMods )
        {
          memcpy( generated, tess_texCoords0, 8 * tess_numVertexes );
          scratch = 0;
        }
        else
        {
          *active = tess_texCoords0;
        }
        break;
      case 6:                                     /* TCGEN_VECTOR */
      {
        const float *xyz = (const float *)tess_xyz;
        for ( scratch = 0; scratch < tess_numVertexes; scratch++, xyz += tess_vertexComponentCount )
        {
          /* retail sums (v1*x1 + v0*x0) + x2*v2 and (w0*x0 + x2*w2) + w1*x1 */
          ((float *)*active)[2 * scratch + 0] = bundle->tcGenVectors[0][1] * xyz[1]
                                         + bundle->tcGenVectors[0][0] * xyz[0]
                                         + xyz[2] * bundle->tcGenVectors[0][2];
          ((float *)*active)[2 * scratch + 1] = bundle->tcGenVectors[1][0] * xyz[0]
                                         + xyz[2] * bundle->tcGenVectors[1][2]
                                         + bundle->tcGenVectors[1][1] * xyz[1];
        }
        break;
      }
      case 4:
        RB_CalcEnvironmentTexCoords( (int)generated );
        break;
      case 7:                                     /* normals */
        if ( !bundle->numTexMods )
        {
          *active = tess_stageNormals;
        }
        else
        {
          memcpy( generated, tess_stageNormals, 12 * tess_numVertexes );
          scratch = 0;
        }
        break;
      case 8:                                     /* tangents */
        if ( !tess_stageTangentsValid )
          RB_CalcTangentSpace();
        if ( !bundle->numTexMods )
        {
          *active = tess_stageTangents;
        }
        else
        {
          memcpy( *active, tess_stageTangents, 12 * tess_numVertexes );
          scratch = 0;
        }
        break;
      case 9:                                     /* bitangents */
        if ( !tess_stageBitangentsValid )
          RB_CalcTangentSpace();
        if ( !bundle->numTexMods )
        {
          *active = tess_stageBitangents;
        }
        else
        {
          memcpy( *active, tess_stageBitangents, 12 * tess_numVertexes );
          scratch = 0;
        }
        break;
      case 10:
        RB_CalcCubeMapTbnTexCoords( generated, 0 );
        break;
      case 11:
        RB_CalcCubeMapTbnTexCoords( generated, 1 );
        break;
      case 12:
        RB_CalcCubeMapTbnTexCoords( generated, 2 );
        break;
      case 13:
        RB_CalcCubeMapVertexToEyeTexCoords( (int)generated );
        break;
      case 14:
        RB_CalcCubeMapEyeToVertexTexCoords( (int)generated );
        break;
      case 15:
        RB_CalcCubeMapReflectionTexCoords( generated );
        break;
      case 16:
        RB_CalcCubeMapLightVectorTexCoords( (int)generated );
        break;
      case 17:
        RB_CalcCubeMapLightHalfAngleTexCoords( (int)generated );
        break;
      case 18:
        RB_CalcCubeMapSunHalfAngleTexCoords( (int)generated );
        break;
      case 19:
        RB_CalcCubeMapDot3ReflectTexCoords((float *)generated, 0);
        break;
      case 20:
        RB_CalcCubeMapDot3ReflectTexCoords((float *)generated, 1);
        break;
      case 21:
        RB_CalcCubeMapDot3ReflectTexCoords((float *)generated, 2);
        break;
      default:
        break;
    }

    /* apply this unit's texture-coordinate modifiers */
    numMods = bundle->numTexMods;
    for(m=0;m<bundle->numTexMods;++m)
    {
      float *texMod = (float *)( 68 * m + (int)bundle->texMods );
      scratch = *(unsigned int *)texMod;
      switch ( *(unsigned int *)texMod )
      {
        case 0:                                   /* TMOD_NONE terminates the list (TR_MAX_TEXMODS) */
          m = 4;
          break;
        case 8:
          RB_CalcSwapTexCoords( (float *)*active );
          break;
        case 2:
          RB_CalcTurbulentTexCoords( (float *)*active, texMod + 1 );
          break;
        case 7:
          RB_CalcScrollTexCoords( (float *)( backEnd_currentEntity + 112 ), (float *)*active );
          break;
        case 3:
          RB_CalcScrollTexCoords( texMod + 14, (float *)*active );
          break;
        case 4:
          RB_CalcScaleTexCoords( texMod + 12, (float *)*active );
          break;
        case 5:
          RB_CalcStretchTexCoords( (int)( texMod + 1 ), (float *)*active );
          break;
        case 1:
          RB_CalcTransformTexCoords( texMod, (float *)*active );
          break;
        case 6:
          RB_CalcRotateTexCoords( texMod[16], (float *)*active );
          break;
        case 9:                                   /* negate all coords */
        {
          float *st = (float *)*active;
          int    n  = 3 * tess_numVertexes;
          while ( n-- > 0 )
          {
            *st = -*st;
            st++;
          }
          break;
        }
        case 10:
          RB_CalcCubeMapBumpmapFrameTexCoords( (float *)*active );
          break;
        default:
          ri.Error( 1, "\x15" "ERROR: unknown texmod '%d' in shader '%s'\n", scratch, tess_shader );
          break;
      }

    }

    ++bundle;
  }
  return;
}

/* ---- R_FogOff  0x004D2A50 ----  (retail's lives beside R_FogOn in tr_main.c;
 * RB_SetIteratorFog tail-jumps to it) */
static void R_FogOff( void )
{
  if ( ( glState_glStateBits & 0x200000 ) == 0 )
    return;
  glDisable( 0xB60u );                                     /* GL_FOG */
  glState_glStateBits &= ~0x200000u;
}

/* ---- RB_SetIteratorFog  0x00500810 ----  VERIFIED
 * Set up (or disable) the fixed-function GL fog for the current view / surface. */
void __cdecl RB_SetIteratorFog( void )
{
  if ( backEnd_refdef_rdflags & 1 )                        /* RDF_NOWORLDMODEL */
  {
    R_FogOff();
    return;
  }

  if ( backEnd_refdef_rdflags & 0x40 )                     /* RDF_DRAWINGSKY */
  {
    if ( tr_fogSky_registered )
      R_Fog( (int)&tr_fogSky );
    else
      R_FogOff();
    return;
  }

  if ( rendererSkyboxPortal && ( backEnd_refdef_rdflags & 8 ) )
  {
    if ( tr_fogPortalView_registered )
      R_Fog( (int)&tr_fogPortalView );
    else
      R_FogOff();
  }
  else
  {
    if ( glfogNum > 0 )
      R_Fog( (int)&tr_fogCurrent_mode );
    else
      R_FogOff();
  }
}

/* ---- RB_SingleStageGeneric  0x005008A0 ----  VERIFIED
 * Compute colours and texcoords for one shader stage and draw it, either
 * through the multitexture collapse path or as a single textured pass. */
void __cdecl RB_SingleStageGeneric( unsigned int *stage, GLsizei numIndexes, GLvoid *indexes )
{
  int     program;
  int     fragmentShader;

  RB_ComputeColors( (int)stage );
  RB_ComputeTexCoords( (int)stage );

  if ( !setArraysOnce )
  {
    if ( ( glState_clientStateBits & 0x100 ) == 0 )
    {
      glEnableClientState( 0x8076u );                      /* GL_COLOR_ARRAY */
      glState_clientStateBits |= 0x100u;
    }
    glColorPointer( 4, 0x1401u, 0, tess_stageVertexColors );
  }

  if ( stage[51] )                                         /* multitexture collapse */
  {
    GL_State( stage[417] );                                /* stateBits */
    if ( backEnd_wireframeOverride )
      glPolygonMode( 0x408u, 0x1B02u );
    RB_SetupMultitexture( stage, (GLvoid **)&tess_activeTexCoords, 0 );
    R_DrawElements( numIndexes, indexes );
  }
  else
  {
    if ( !setArraysOnce )   /* 0x50094B loads slot 0's pointer (the multitexture call above pushes the table address) */
      glTexCoordPointer( stage[36], 0x1406u, 0, tess_activeTexCoords[0] );

    if ( ( *(unsigned int *)( *(unsigned int *)( tess_shader + 340 ) + 1668 ) & 0x100000 ) != 0
      && r_debugEntLight->integer )
      GL_Bind( (GLenum *)tr_whiteImage );
    else
      GL_Bind( (GLenum *)RB_GetAnimatedImage( glState_currentTmu, (int)( stage + 1 ) ) );
    GL_State( stage[417] );
    if ( stage[402] )
      RB_SetupRegisterCombiners( stage[402] );
    program = stage[403];
    if ( program )
      RB_SetupVertexProgram( program );
    fragmentShader = stage[401];
    if ( fragmentShader != glState_boundFragmentShader )
    {
      glBindFragmentShaderATI( fragmentShader );
      glState_boundFragmentShader = fragmentShader;
    }
    R_DrawElements( numIndexes, indexes );
  }
}

/* ---- ProjectDlightTexture  0x00500A10 ----  VERIFIED
 * For each dynamic light touching this surface, build its projected index /
 * colour arrays and draw the shared dlight stage (shader not lit per entity)
 * or every per-light stage.  Returns backEnd.refdef.num_dlights (dead-EAX). */
void __cdecl ProjectDlightTexture( void )
{

  int      dlightIndex;
  int      dlight;
  GLsizei  hitCount;
  int      stageIndex;
  char    *stage;
  unsigned int  *dlightStage;
  unsigned short hitIndexes[49152];       /* projected index scratch [esp+18h] */


  if ( !backEnd_refdef_num_dlights )
    return;

  RB_EndMultitexture();

  for ( dlightIndex = 0; dlightIndex < backEnd_refdef_num_dlights; dlightIndex++ )
  {
    if ( ( ( 1 << dlightIndex ) & tess_dlightBits ) != 0 )
    {
      dlight   = (int)&( (dlight_t *)backEnd_refdef_dlights )[dlightIndex];
      hitCount = RB_BuildDlightArrays( tess_texCoords1, tess_vertexColors,
                                       (float *)&( (dlight_t *)backEnd_refdef_dlights )[dlightIndex],
                                       hitIndexes );
      if ( hitCount )
      {
        if ( *(char *)( tess_shader + 84 ) & 0x80 )         /* shader.lightingFlags: lit per entity */
        {
          backEnd_currentDlight = dlight;
          for ( stageIndex = 0; stageIndex < 8; stageIndex++ )
          {
            stage = ((char **)tess_activeStages)[stageIndex];
            if ( !stage )
              break;
            if ( *stage & 0x80 )                            /* stage.flags: per-light stage */
              RB_SingleStageGeneric( (unsigned int *)stage, hitCount, hitIndexes );
          }
        }
        else
        {
          dlightStage = *(unsigned int **)( tr_dlightShader + 340 );
          if ( dlightStage )
            RB_SingleStageGeneric( dlightStage, hitCount, hitIndexes );
        }
      }
    }
  }
  return;
}

/* ---- RB_IterateStagesGeneric  0x00500B30 ----  VERIFIED */
void __cdecl RB_IterateStagesGeneric( shaderCommands_t *input )
{
  int   i;
  char *stage;

  RB_EndMultitexture();

  for ( i = 0; i < 8; i++ )
  {
    stage = ( (char **)tess_activeStages )[i];
    if ( !stage )
      break;

    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      RB_SingleStageGeneric( (unsigned int *)stage, tess_numIndexes, tess_indexes );
      if ( r_lightmap->integer )
      {
        if ( *(unsigned char *)( stage + 200 ) )
          break;
        if ( *(unsigned char *)( stage + 400 ) )
          break;
      }
    }
  }
}

/* ---- RB_StageIteratorGeneric  0x00500BA0 ----  VERIFIED
 * The generic (fixed-function / non-optimized) tess stage iterator: deform,
 * fog and cull the current surface, set up the vertex arrays, draw the ordinary
 * stages, then the dynamic-light pass (the projected dlight texture, or each
 * per-light stage once per entity light).  `portalPass` runs only the dlight
 * pass.  Returns qglUnlockArraysEXT (dead-EAX). */
void __cdecl RB_StageIteratorGeneric(int portalPass)
{

  int     needsNormals;
  char   *logText;
  int     entity;
  int     lightCount;
  int     lightIndex;
  int     stageIndex;
  char   *stage;

  if ( tess_activeStageCount <= 0 )
    return;
  if ( portalPass )
  {
    if ( !tess_dlightBits )
      return;
  }

  RB_DeformTessGeometry();
  needsNormals = ( ((shader_t *)tess_shader)->surfaceFlags >> 8 ) & 1;      /* shader needs normals */
  if ( r_logFile->integer )
  {
    logText = va( "--- RB_StageIteratorGeneric( %s ) ---\n", (const char *)tess_shader );
    if ( Stream )
      fprintf( Stream, "%s", (int)logText );
  }
  RB_EndMultitexture();
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );

  if ( tess_activeStageCount > 1
    || *(unsigned int *)( *(unsigned int *)( tess_shader + 340 ) + 348 ) )      /* collapsed multitexture */
  {
    if ( needsNormals )
      GL_ClientState( 1536 );
    else
      GL_ClientState( 1024 );
  }
  else
  {
    setArraysOnce = 1;
    if ( needsNormals )
      GL_ClientState( 1793 );
    else
      GL_ClientState( 1281 );
    glColorPointer( 4, 0x1401u, 0, tess_stageVertexColors );
    glTexCoordPointer( *(unsigned int *)( *(unsigned int *)( tess_shader + 340 ) + 144 ), 0x1406u, 0,
                       &tess_generatedTexCoords );
  }
  if ( needsNormals )
    glNormalPointer( 0x1406u, 12, tess_stageNormals );

  glVertexPointer( tess_vertexComponentCount, 0x1406u, 0, tess_xyz );
  if ( qglLockArraysEXT )
    qglLockArraysEXT( 0, tess_numVertexes );
  if ( !setArraysOnce )
  {
    if ( needsNormals )
      GL_ClientState( 1793 );
    else
      GL_ClientState( 1281 );
  }
  if ( !portalPass )
    RB_IterateStagesGeneric( &tess );

  if ( tess_dlightBits
    && ((shader_t *)tess_shader)->sort <= 5.0f                       /* shader.sort <= SS_OPAQUE */
    && ( ((shader_t *)tess_shader)->surfaceParmFlags & 0x20004 ) == 0 )        /* SURF_NODLIGHT | SURF_SKY */
  {
    ProjectDlightTexture();
  }
  else
  {
    if(*(unsigned char *)(tess_shader+84)&0x80u)                        /* shader lit per entity */
    {
      entity = backEnd_currentEntity;
      if ( entity )
      {
        lightCount = *(unsigned int *)( entity + 204 );
        if ( lightCount )
        {
          for ( lightIndex = 0; lightIndex < lightCount; lightIndex++ )
          {
            backEnd_currentDlight = *(unsigned int *)( entity + 8 * lightIndex + 208 );
            if ( *(unsigned int *)backEnd_currentDlight != 8 )            /* light type != null */
            {
              backEnd_currentLightScale = *(float *)( entity + 8 * lightIndex + 212 );
              for ( stageIndex = 0; stageIndex < 8; stageIndex++ )
              {
                stage = ((char **)tess_activeStages)[stageIndex];
                if ( !stage )
                  break;
                if((unsigned char)*stage&0x80u)                                   /* per-light stage */
                {
                  RB_SingleStageGeneric( (unsigned int *)stage, tess_numIndexes, tess_indexes );
                  entity = backEnd_currentEntity;
                }
              }
            }
            lightCount = *(unsigned int *)( entity + 204 );
          }
          backEnd_currentDlight = 0;
        }
      }
    }
  }

  if ( qglUnlockArraysEXT )
    qglUnlockArraysEXT();
  setArraysOnce = 0;
  return;
}

/* ---- RB_EndSurface_OptimizedGeneric  0x00500E40 ----  VERIFIED
 * Draw an optimized (pre-tessellated) surface from its static vertex buffer
 * with the generic (client-array) backend, then the r_showtris overlay. */
void __cdecl RB_EndSurface_OptimizedGeneric( void )
{
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  unsigned int  vertexArrays[9];

  if ( r_lightmap->integer )
  {
    if ( glState_texEnv[glState_currentTmu] != 7681 )       /* GL_REPLACE */
    {
      glState_texEnv[glState_currentTmu] = 7681;
      glTexEnvi( 0x2300u, 0x2200u, 7681 );
    }
  }
  else
  {
    GL_TexEnv( *(unsigned int *)( *(unsigned int *)( tess_shader + 340 ) + 348 ) );
  }
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  glColorPointer( 4, 0x1401u, 32, (const GLvoid *)( ((shader_t *)tess_shader)->optimizedVertexStorage + 16 ) );
  glVertexPointer( 3, 0x1406u, 32, (const GLvoid *)( ((shader_t *)tess_shader)->optimizedVertexStorage + 20 ) );
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  vertexArrays[0] = ((shader_t *)tess_shader)->optimizedVertexStorage;
  vertexArrays[1] = vertexArrays[0] + 8;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)vertexArrays, 32 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x0050101E */
    GL_Cull( ((shader_t *)tr_showTrisShader)->cullType );
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitexture( trisStage, (GLvoid **)vertexArrays, 32 );
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      qglColor3f( 0.0f, tr_identityLight, 0.0f );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}

/* ---- RB_EndSurface_OptimizedARB  0x00501180 ----  VERIFIED
 * ARB vertex-buffer-object variant of RB_EndSurface_OptimizedGeneric. */
void __cdecl RB_EndSurface_OptimizedARB( void )
{
  char   *logText;
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  unsigned int  vertexArrays[9];

  if ( r_logFile->integer )
  {
    logText = va( "--- RB_SurfaceOptimizedARB( %s ) ---\n", (const char *)tess_shader );
    if ( Stream )
      fprintf( Stream, "%s", (int)logText );
  }
  if ( r_lightmap->integer )
  {
    if ( glState_texEnv[glState_currentTmu] != 7681 )
    {
      glState_texEnv[glState_currentTmu] = 7681;
      glTexEnvi( 0x2300u, 0x2200u, 7681 );
    }
  }
  else
  {
    GL_TexEnv( *(unsigned int *)( *(unsigned int *)( tess_shader + 340 ) + 348 ) );
  }
  qglBindBufferARB( 34962, ((shader_t *)tess_shader)->optimizedVertexStorage );
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  glColorPointer( 4, 0x1401u, 32, (const GLvoid *)0x10 );
  glVertexPointer( 3, 0x1406u, 32, (const GLvoid *)0x14 );
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  vertexArrays[0] = 0;
  vertexArrays[1] = 8;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)vertexArrays, 32 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x0050138F */
    GL_Cull( ((shader_t *)tr_showTrisShader)->cullType );
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitexture( trisStage, (GLvoid **)vertexArrays, 32 );
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      qglColor3f( 0.0f, tr_identityLight, 0.0f );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
  qglBindBufferARB( 34962, 0 );
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}

/* ---- RB_EndSurface_OptimizedATI  0x00501500 ----  VERIFIED
 * ATI array-object variant of RB_EndSurface_OptimizedGeneric. */
void __cdecl RB_EndSurface_OptimizedATI( void )
{
  int     storage;
  int     base;
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  unsigned int  vertexArrays[9];

  storage = ((shader_t *)tess_shader)->optimizedVertexStorage;
  base    = ((shader_t *)tess_shader)->optimizedVertexStorageOffset;
  if ( r_lightmap->integer )
  {
    if ( glState_texEnv[glState_currentTmu] != 7681 )
    {
      glState_texEnv[glState_currentTmu] = 7681;
      glTexEnvi( 0x2300u, 0x2200u, 7681 );
    }
  }
  else
  {
    GL_TexEnv( *(unsigned int *)( *(unsigned int *)( tess_shader + 340 ) + 348 ) );
  }
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  qglArrayObjectATI( 32886, 4, 5121, 32, storage, base + 16 );
  qglArrayObjectATI( 32884, 3, 5126, 32, storage, base + 20 );
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  vertexArrays[0] = base;
  vertexArrays[1] = base + 8;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitextureATI( (unsigned int *)stage, storage, (int *)vertexArrays, 32 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x005016D3 */
    GL_Cull( ((shader_t *)tr_showTrisShader)->cullType );
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitextureATI( trisStage, storage, (int *)vertexArrays, 32 );
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      qglColor3f( 0.0f, tr_identityLight, 0.0f );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}

/* ---- RB_EndSurface_OptimizedNV  0x00501830 ----  VERIFIED
 * NV variant of RB_EndSurface_OptimizedGeneric. */
void __cdecl RB_EndSurface_OptimizedNV( void )
{
  char   *logText;
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  unsigned int  vertexArrays[9];

  if ( r_logFile->integer )
  {
    logText = va( "--- RB_SurfaceOptimizedNV( %s ) ---\n", (const char *)tess_shader );
    if ( Stream )
      fprintf( Stream, "%s", (int)logText );
  }
  if ( r_lightmap->integer )
  {
    if ( glState_texEnv[glState_currentTmu] != 7681 )
    {
      glState_texEnv[glState_currentTmu] = 7681;
      glTexEnvi( 0x2300u, 0x2200u, 7681 );
    }
  }
  else
  {
    GL_TexEnv( *(unsigned int *)( *(unsigned int *)( tess_shader + 340 ) + 348 ) );
  }
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  glColorPointer( 4, 0x1401u, 32, (const GLvoid *)( ((shader_t *)tess_shader)->optimizedVertexStorage + 16 ) );
  glVertexPointer( 3, 0x1406u, 32, (const GLvoid *)( ((shader_t *)tess_shader)->optimizedVertexStorage + 20 ) );
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  vertexArrays[0] = ((shader_t *)tess_shader)->optimizedVertexStorage;
  vertexArrays[1] = vertexArrays[0] + 8;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)vertexArrays, 32 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x00501A4F */
    GL_Cull( ((shader_t *)tr_showTrisShader)->cullType );
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitexture( trisStage, (GLvoid **)vertexArrays, 32 );
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      qglColor3f( 0.0f, tr_identityLight, 0.0f );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}

/* ---- RB_EndSurface_CachedStaticModelGeneric  0x00501BB0 ----  VERIFIED
 * Draw a cached static-model surface (stride 24) with the generic backend. */
void __cdecl RB_EndSurface_CachedStaticModelGeneric( void )
{
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  float   trisGrey;
  unsigned int  vertexArrays[9];

  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  glColorPointer( 4, 0x1401u, 24, (const GLvoid *)( tr_cachedStaticModelStorage + 8 ) );
  glVertexPointer( 3, 0x1406u, 24, (const GLvoid *)( tr_cachedStaticModelStorage + 12 ) );
  vertexArrays[0] = tr_cachedStaticModelStorage;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)vertexArrays, 24 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x00501D21 */
    GL_Cull( ((shader_t *)tr_showTrisShader)->cullType );
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitexture( trisStage, (GLvoid **)vertexArrays, 24 );
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      trisGrey = tr_identityLight * 0.5f;
      qglColor3f( trisGrey, tr_identityLight, tr_identityLight );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}

/* ---- RB_EndSurface_CachedStaticModelARB  0x00501E90 ----  VERIFIED
 * ARB VBO variant of RB_EndSurface_CachedStaticModelGeneric. */
void __cdecl RB_EndSurface_CachedStaticModelARB( void )
{
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  float   trisGrey;
  unsigned int  vertexArrays[9];

  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  qglBindBufferARB( 34962, tr_cachedStaticModelStorage );
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  glColorPointer( 4, 0x1401u, 24, (const GLvoid *)8 );
  glVertexPointer( 3, 0x1406u, 24, (const GLvoid *)0xC );
  vertexArrays[0] = 0;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)vertexArrays, 24 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x00502002 */
    GL_Cull( ((shader_t *)tr_showTrisShader)->cullType );
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitexture( trisStage, (GLvoid **)vertexArrays, 24 );
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      trisGrey = tr_identityLight * 0.5f;
      qglColor3f( trisGrey, tr_identityLight, tr_identityLight );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
  qglBindBufferARB( 34962, 0 );
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}

/* ---- RB_EndSurface_CachedStaticModelATI  0x00502170 ----  VERIFIED
 * ATI array-object variant of RB_EndSurface_CachedStaticModelGeneric.  (Its
 * r_showtris overlay omits the GL_Cull, the colour-array disable and the log
 * line the other variants emit -- reproduced as shipped.) */
void __cdecl RB_EndSurface_CachedStaticModelATI( void )
{
  int     storage;
  int     base;
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  float   trisGrey;
  unsigned int  vertexArrays[9];

  storage = tr_cachedStaticModelStorage;
  base    = tr_cachedStaticModelStorageOffset;
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  qglArrayObjectATI( 32886, 4, 5121, 24, storage, base + 8 );
  qglArrayObjectATI( 32884, 3, 5126, 24, storage, base + 12 );
  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  vertexArrays[0] = base;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitextureATI( (unsigned int *)stage, storage, (int *)vertexArrays, 24 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x005022EF */
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitextureATI( trisStage, storage, (int *)vertexArrays, 24 );
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      trisGrey = tr_identityLight * 0.5f;
      qglColor3f( trisGrey, tr_identityLight, tr_identityLight );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
}

/* ---- RB_EndSurface_CachedStaticModelNV  0x00502410 ----  VERIFIED
 * NV variant of RB_EndSurface_CachedStaticModelGeneric. */
void __cdecl RB_EndSurface_CachedStaticModelNV( void )
{
  int     i;
  char   *stage;
  int     showtris;
  unsigned int *trisStage;
  float   trisGrey;
  unsigned int  vertexArrays[9];

  RB_SetIteratorFog();
  GL_Cull( ((shader_t *)tess_shader)->cullType );
  if ( ( glState_clientStateBits & 0x100 ) == 0 )
  {
    glEnableClientState( 0x8076u );
    glState_clientStateBits |= 0x100u;
  }
  if ( ( glState_clientStateBits & 0x200 ) != 0 )
  {
    glDisableClientState( 0x8075u );
    glState_clientStateBits &= ~0x200u;
  }
  glColorPointer( 4, 0x1401u, 24, (const GLvoid *)( tr_cachedStaticModelStorage + 8 ) );
  glVertexPointer( 3, 0x1406u, 24, (const GLvoid *)( tr_cachedStaticModelStorage + 12 ) );
  vertexArrays[0] = tr_cachedStaticModelStorage;
  for ( i = 0; i < tess_activeStageCount; ++i )
  {
    stage = *(char **)( tess_activeStages + 4 * i );
    if ( !( *(unsigned int *)stage & 0x80 ) )
    {
      GL_State( *( (unsigned int *)stage + 417 ) );
      RB_SetupMultitexture( (unsigned int *)stage, (GLvoid **)vertexArrays, 24 );
      if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
      }
      else
      {
        backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
        ++backEnd_pc_drawCallCount;
        glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
      }
    }
  }
  showtris = r_showtris->integer;
  if ( showtris )
  {
    if ( ( showtris & 1 ) != 0 )
      glDepthRange( 0.0, 0.10000000149011612 );   /* (double)0.1f, retail 0x00502581 */
    GL_Cull( ((shader_t *)tr_showTrisShader)->cullType );
    trisStage = *(unsigned int **)( tr_showTrisShader + 340 );
    GL_State( trisStage[417] );
    RB_SetupMultitexture( trisStage, (GLvoid **)vertexArrays, 24 );
    if ( ( glState_clientStateBits & 0x100 ) != 0 )
    {
      glDisableClientState( 0x8076u );
      glState_clientStateBits &= ~0x100u;
    }
    if ( r_showtris->integer >= 5 )
    {
      RB_SetSurfaceCountColor( tess_renderedIndexCount );
    }
    else
    {
      trisGrey = tr_identityLight * 0.5f;
      qglColor3f( trisGrey, tr_identityLight, tr_identityLight );
    }
    if ( qglDrawRangeElementsEXT && tess_optimizedVertexEnd )
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      qglDrawRangeElementsEXT( 4, tess_optimizedFirstVertex, tess_optimizedVertexEnd, tess_renderedIndexCount, 5123, &tess_optimizedIndexes );
    }
    else
    {
      backEnd_pc_drawnIndexCount += tess_renderedIndexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements( 4u, tess_renderedIndexCount, 0x1403u, &tess_optimizedIndexes );
    }
    glDepthRange( 0.0, 1.0 );
  }
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}

/* ---- RB_EndSurface_Optimized  0x005026F0 ----  VERIFIED */
void __cdecl RB_EndSurface_Optimized( void )
{
  switch ( ((shader_t *)tess_shader)->optimizedBackend )
  {
    case 0x19: RB_EndSurface_OptimizedGeneric();         break;
    case 0x1A: RB_EndSurface_OptimizedARB();             break;
    case 0x1B: RB_EndSurface_OptimizedATI();             break;
    case 0x1C: RB_EndSurface_OptimizedNV();              break;
    case 0xC:  RB_EndSurface_CachedStaticModelGeneric(); break;
    case 0xD:  RB_EndSurface_CachedStaticModelARB();     break;
    case 0xE:  RB_EndSurface_CachedStaticModelATI();     break;
    case 0xF:  RB_EndSurface_CachedStaticModelNV();      break;
    default:   break;
  }
  backEnd_pc_indexCount += tess_renderedIndexCount;
  backEnd_pc_vertexCount += tess_renderedVertexCount;
  tess_renderedIndexCount = 0;
  tess_renderedVertexCount = 0;
}

/* ---- RB_EndSurface  0x005027C0 ----  VERIFIED */
void __cdecl RB_EndSurface( void )
{
  int   optimizedDrawn;
  char *string;

  optimizedDrawn = 0;
  if ( tess_renderedIndexCount )
  {
    RB_EndSurface_Optimized();
    optimizedDrawn = 1;
  }

  if ( !tess_numIndexes )
    return;

  if ( tess_indexes_last )
    ri.Error( 1, "\x15" "RB_EndSurface() - SHADER_MAX_INDEXES hit" );
  if ( *(float *)&tess_xyz[0x1FFF * tess_vertexComponentCount] != 0.0f )
    ri.Error( 1, "\x15" "RB_EndSurface() - SHADER_MAX_VERTEXES hit" );

  if ( tess_shader == tr_stencilShadowShader )
  {
    RB_ShadowTessEnd();
    tess_numIndexes = 0;
    return;
  }

  if ( r_debugSort->integer
    && FCAST( r_debugSort->integer ) < ((shader_t *)tess_shader)->sort )
  {
    tess_numIndexes = 0;
    return;
  }

  if ( rendererSkyboxPortal )
  {
    if ( !( backEnd_refdef_rdflags & 8 ) )
    {
      if ( (char *)tess_stageIterator == (char *)RB_StageIteratorSky )
      {
        tess_numIndexes = 0;
        return;
      }
    }
    else if ( !rendererSkyboxPortalActive && (char *)tess_stageIterator != (char *)RB_StageIteratorSky )
    {
      tess_numIndexes = 0;
      return;
    }
  }

  ++backEnd_pc_shaderCount;
  if ( tess_activeStageCount )
  {
    backEnd_pc_vertexCount += tess_numVertexes;
    backEnd_pc_indexCount += tess_numIndexes;
  }

  tess_stageIterator( optimizedDrawn );

  if ( r_showtris->integer )
  {
    if ( tess_numIndexes )
      DrawTris( &tess );
  }

  string = r_shownormals->string;
  if ( *string )
  {
    if ( !_stricmp( string, "white" ) )
    {
      DrawNormals( &tess );
    }
    else if ( !_stricmp( r_shownormals->string, "color" ) )
    {
      DrawColoredNormals( &tess );
    }
    else
    {
      Cvar_Set2( "r_shownormals", "", qtrue );
      Com_Printf( "Proper Usage: r_shownormals white | color\n" );
    }
  }

  tess_numIndexes = 0;
  if ( Stream )
    fprintf( Stream, "%s", (int)"----------\n" );
}
