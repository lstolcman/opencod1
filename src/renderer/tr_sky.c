/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
int s_cloudTexCoords[3392];             /* 0x011DAE68 */
float sky_mins[2][6];                   /* 0x011DAE08: s mins, then t mins */
float sky_maxs[2][6];                   /* 0x011DAE38: s maxs, then t maxs */
float sky_max;
float sky_min;
unsigned char s_cloudTexP[15608];
int rendererSkyBox;
/* the sun sprite / flare / blind / glare state (0x011DBDA0); tr_records.h SUNBASE views */
int rendererSunState[39];


extern int GL_Bind();
extern int GL_ClientState();
extern int GL_State();
void __cdecl GL_Cull( int cullType );
extern int Hunk_AllocAlignInternal();
void __cdecl RB_BeginSurface( void *shader, int vertexComponentCount );
extern int RB_EndMultitexture();
void __cdecl RB_EndSurface( void );
extern int RB_SelectStorageATI();
extern int RB_SelectStorageNV();
void __cdecl RB_SelectStorage( int storageMode );
extern void __cdecl RB_UpdateSunFlare(int shader,float brightness,float size,float spriteSize,int fadeInMsec,int fadeOutMsec);
extern int R_CreateBufferARB();
int __cdecl myftol( float value );   /* tr_shade_calc.c 0x00515940; LTCG inlines it */
float Q_acos( float c );             /* com_math.c (/Op); LTCG inlines it */
extern int R_FindShader();
extern int R_FogOn();
void sub_50AF10(char *comment); /* GLimp_LogComment */
 double __cdecl R_UpdateOverTime( float current, float target,
                                  int fadeInRate, int fadeOutRate,
                                  int elapsedMsec );

/* -------------------------------------------------------------------------
 *  Sky grid geometry.  Each cube face is a 9x9 grid of vertices (8 sky
 *  subdivisions), giving 6*9*9 = 486 vertices per sky box.
 * ------------------------------------------------------------------------- */
#define SKYBOX_FACE_COUNT   6
#define SKY_GRID_SIDE       9
#define SKY_GRID_CENTER     4
#define SKY_VERTEX_COUNT    ( SKYBOX_FACE_COUNT * SKY_GRID_SIDE * SKY_GRID_SIDE )   /* 486 */
#define SKY_SUBDIVISIONS        8
#define HALF_SKY_SUBDIVISIONS   ( SKY_SUBDIVISIONS / 2 )

/* -------------------------------------------------------------------------
 *  One sky-grid vertex, exactly as uploaded to the vertex buffer: a 2D
 *  texcoord followed by a homogeneous position (CoDUO renderer_sky_vertex_t).
 * ------------------------------------------------------------------------- */
typedef struct {
	float texCoord[2];        /* +0x00 */
	float position[4];        /* +0x08 */
} skyVertex_t;
COD1_ASSERT_SIZE(skyVertex_t, 0x18);
ASSERT_OFF(skyVertex_t, position, 0x08);

#define SKY_VERTEX_BYTES   ( SKY_VERTEX_COUNT * (int)sizeof(skyVertex_t) )   /* 11664 = 0x2D90 */

/* -------------------------------------------------------------------------
 *  The sky-box vertex-storage descriptor (CoDUO renderer_sky_vertex_storage_t).
 *  rendererSkyBox holds the ADDRESS of one of these; R_BuildSkyBox fills it for
 *  whichever static-vertex backend is active, and DrawSkyBox binds the arrays
 *  accordingly.
 * ------------------------------------------------------------------------- */
typedef struct {
	int      memorySource;    /* +0x00  static-vertex memory source / pool selector */
	int      backend;         /* +0x04  SKYVB_* */
	void    *base;            /* +0x08  client vertex ptr, or ARB/ATI buffer object name */
	unsigned objectOffset;    /* +0x0c  ATI object-buffer byte offset */
} skyVertexStorage_t;
COD1_ASSERT_SIZE(skyVertexStorage_t, 0x10);
ASSERT_OFF(skyVertexStorage_t, base, 0x08);

enum {
	SKYVB_CLIENT     = 0,     /* client memory (glLockArraysEXT path) */
	SKYVB_ARB_BUFFER = 1,     /* ARB vertex buffer object */
	SKYVB_ATI_OBJECT = 2,     /* ATI object buffer */
	SKYVB_NV_MEMORY  = 3      /* NV vertex-array-range / hunk memory */
};

/* shader_t fields reached here by raw offset (not in tr_local.h's shared view):
 *   +0x60 skyCloudHeight (float)     +0x64 skyOuterBox[6] (image_t*)
 *   +0x7c skyInnerBox[6]  (image_t*) +0x154 stages[8]     (shaderStage_t*)
 * shaderStage_t: +0x684 stateBits (u32; 0x100 == GLS_DEPTHMASK_TRUE). */

static const char * const r_sunCvarNames[20] =
{
	"r_sunsprite_shader",       /* 0x005713C8 */
	"r_sunsprite_size",
	"r_sunflare_shader",
	"r_sunflare_min_size",
	"r_sunflare_min_angle",
	"r_sunflare_max_size",
	"r_sunflare_max_angle",
	"r_sunflare_max_alpha",
	"r_sunflare_fadein",
	"r_sunflare_fadeout",
	"r_sunblind_min_angle",
	"r_sunblind_max_angle",
	"r_sunblind_max_darken",
	"r_sunblind_fadein",
	"r_sunblind_fadeout",
	"r_sunglare_min_angle",
	"r_sunglare_max_angle",
	"r_sunglare_max_lighten",
	"r_sunglare_fadein",
	"r_sunglare_fadeout"
};

/* 0x00571418.  "degres" is retail's spelling in six of these; kept. */
static const char * const r_sunCvarHelp[20] =
{
	"name for static sprite; can be any shader",
	"diameter in pixels at 640x480 and 80 fov",
	"name for flare effect; can be any shader",
	"smallest size of flare effect in pixels at 640x480",
	"angle from sun in degrees outside which effect is 0",
	"largest size of flare effect in pixels at 640x480",
	"angle from sun in degrees inside which effect is max",
	"0-1 vertex color and alpha of sun at max effect",
	"time in seconds to fade alpha from 0% to 100%",
	"time in seconds to fade alpha from 100% to 0%",
	"angle from sun in degres outside which blinding is 0",
	"angle from sun in degres inside which blinding is max",
	"0-1 fraction for how black the world is at max blind",
	"time in seconds to fade blind from 0% to 100%",
	"time in seconds to fade blind from 100% to 0%",
	"angle from sun in degres outside which glare is 0",
	"angle from sun in degres inside which glare is max",
	"0-1 fraction for how white the world is at max glare",
	"time in seconds to fade glare from 0% to 100%",
	"time in seconds to fade glare from 100% to 0%"
};


/* 0x005714B0.  AddSkyPolygon's world-vector -> face-ST axis map. */
static const int vec_to_st[6][3] =
{
	{ -2,  3,  1 },
	{  2,  3, -1 },
	{  1,  3,  2 },
	{ -1,  3, -2 },
	{ -2, -1,  3 },
	{ -2,  1, -3 }
};

/* 0x005714F8.  MakeSkyVec's face-ST -> world-vector axis map. */
static const int st_to_vec[6][3] =
{
	{  3, -1,  2 },
	{ -3,  1,  2 },
	{  1,  3,  2 },
	{ -1, -3,  2 },
	{ -2, -1,  3 },
	{  2, -1, -3 }
};

static const int sky_texorder[6] = { 0, 2, 1, 3, 4, 5 };

/*
 * Sky s/t bounds accumulated per cube face.  Retail keeps them as four separate
 * 6-float rows: the s-bounds in sky_mins/sky_maxs and the t-bounds in the rows
 * immediately after (sky_mins_1 = sky_mins[1], sky_maxs_1 = sky_maxs[1],
 * both aliased in tr_tess.h).
 */

/* ---- AddSkyPolygon  0x004E61D0 ----  VERIFIED
 * Fold a clipped polygon into the dominant cube face's s/t bounds. */
void __cdecl AddSkyPolygon(int nump, float *vecs)
{
  int    i, j;
  vec3_t v, av;
  float  s, t, dv;
  int    axis;
  float *vp;

  /* decide which face it maps to */
  VectorCopy(vec3_origin, v);
  for ( i = 0, vp = vecs; i < nump; i++, vp += 3 )
  {
    VectorAdd(vp, v, v);
  }
  av[0] = fabs(v[0]);
  av[1] = fabs(v[1]);
  av[2] = fabs(v[2]);
  if ( av[0] > av[1] && av[0] > av[2] )
  {
    if ( v[0] < 0 )
      axis = 1;
    else
      axis = 0;
  }
  else if ( av[1] > av[2] && av[1] > av[0] )
  {
    if ( v[1] < 0 )
      axis = 3;
    else
      axis = 2;
  }
  else
  {
    if ( v[2] < 0 )
      axis = 5;
    else
      axis = 4;
  }

  /* project new texture coords */
  for ( i = 0; i < nump; i++, vecs += 3 )
  {
    j = vec_to_st[axis][2];
    if ( j > 0 )
      dv = vecs[j - 1];
    else
      dv = -vecs[-j - 1];
    if ( dv < 0.001 )
      continue;                            /* don't divide by zero */
    j = vec_to_st[axis][0];
    if ( j < 0 )
      s = -vecs[-j - 1] / dv;
    else
      s = vecs[j - 1] / dv;
    j = vec_to_st[axis][1];
    if ( j < 0 )
      t = -vecs[-j - 1] / dv;
    else
      t = vecs[j - 1] / dv;

    if ( s < sky_mins[0][axis] )
      sky_mins[0][axis] = s;
    if ( t < sky_mins_1[axis] )
      sky_mins_1[axis] = t;
    if ( s > sky_maxs[0][axis] )
      sky_maxs[0][axis] = s;
    if ( t > sky_maxs_1[axis] )
      sky_maxs_1[axis] = t;
  }
}

static const float sky_clip[6][3] = {
	{  1.0f,  1.0f,  0.0f },
	{  1.0f, -1.0f,  0.0f },
	{  0.0f, -1.0f,  1.0f },
	{  0.0f,  1.0f,  1.0f },
	{  1.0f,  0.0f,  1.0f },
	{ -1.0f,  0.0f,  1.0f }
};

/* ---- ClipSkyPolygon  0x004E6450 ----  VERIFIED
 * Recursively clip a polygon against the six sky-box planes; a fully clipped
 * polygon (stage 6) is projected onto the sky bounds.  `vecs` needs one spare
 * vertex because the polygon is closed in place before it is split. */
void __cdecl ClipSkyPolygon(int nump, float *vecs, int stage)
{
  const float *norm;
  float       *v;
  qboolean     front, back;
  rfloat       d, e;                       /* kept on the x87 stack by retail */
  float        dists[64];                  /* MAX_CLIP_VERTS */
  int          sides[64];
  vec3_t       newv[2][64];
  int          newc[2];
  int          i, j;

  if ( nump > 64 - 2 )                     /* MAX_CLIP_VERTS - 2 */
    ri.Error(1, "\x15" "ClipSkyPolygon: MAX_CLIP_VERTS");
  if ( stage == 6 )
  {
    /* fully clipped, so draw it */
    AddSkyPolygon(nump, vecs);
    return;
  }

  front = back = qfalse;
  norm = sky_clip[stage];
  for ( i = 0, v = vecs; i < nump; i++, v += 3 )
  {
    d = DotProduct(v, norm);
    if ( d > 0.1f )                        /* ON_EPSILON */
    {
      front = qtrue;
      sides[i] = 0;                        /* SIDE_FRONT */
    }
    else if ( d < -0.1f )
    {
      back = qtrue;
      sides[i] = 1;                        /* SIDE_BACK */
    }
    else
      sides[i] = 2;                        /* SIDE_ON */
    dists[i] = d;
  }

  if ( !front || !back )
  {
    /* not clipped */
    ClipSkyPolygon(nump, vecs, stage + 1);
    return;
  }

  /* clip it */
  sides[i] = sides[0];
  dists[i] = dists[0];
  VectorCopy(vecs, (vecs + (i * 3)));
  newc[0] = newc[1] = 0;

  for ( i = 0, v = vecs; i < nump; i++, v += 3 )
  {
    switch ( sides[i] )
    {
      case 0:
        VectorCopy(v, newv[0][newc[0]]);
        newc[0]++;
        break;
      case 1:
        VectorCopy(v, newv[1][newc[1]]);
        newc[1]++;
        break;
      case 2:
        VectorCopy(v, newv[0][newc[0]]);
        newc[0]++;
        VectorCopy(v, newv[1][newc[1]]);
        newc[1]++;
        break;
    }

    if ( sides[i] == 2 || sides[i + 1] == 2 || sides[i + 1] == sides[i] )
      continue;

    d = dists[i] / (dists[i] - dists[i + 1]);
    for ( j = 0; j < 3; j++ )
    {
      e = v[j] + d * (v[j + 3] - v[j]);
      newv[0][newc[0]][j] = e;
      newv[1][newc[1]][j] = e;
    }
    newc[0]++;
    newc[1]++;
  }

  /* continue */
  ClipSkyPolygon(newc[0], newv[0][0], stage + 1);
  ClipSkyPolygon(newc[1], newv[1][0], stage + 1);
}

/* ---- ClearSkyBox  0x004E67A0 ----  [HIGH]
 * Reset every face's s/t bounds.  EAX is left at 6*4 (the strength-reduced
 * loop counter; dead). */
void ClearSkyBox()
{
  int i;

  for ( i = 0; i < 6; i++ )
  {
    sky_mins[0][i] = sky_mins_1[i] =  9999.0f;
    sky_maxs[0][i] = sky_maxs_1[i] = -9999.0f;
  }
}

/* ---- RB_ClipSkyPolygons  0x004E67E0 ----  VERIFIED
 * Project every tessellated triangle onto the sky box to grow the per-face
 * bounds.  input is &tess (retail passes it in ESI). */
void __cdecl RB_ClipSkyPolygons(shaderCommands_t *input)
{
  vec3_t p[5];                             /* one spare vertex for the in-place close */
  int    i, j;

  ClearSkyBox();

  for ( i = 0; i < input->numIndexes; i += 3 )
  {
    for ( j = 0; j < 3; j++ )
    {
      VectorSubtract( &input->xyz[input->indexes[i + j] * input->vertexComponentCount],
                      backEnd_viewParms_originX, p[j] );
    }
    ClipSkyPolygon(3, p[0], 0);
  }
}

/* ---- MakeSkyVec  0x004E68E0 ----  [HIGH]
 * Map a face-local (s,t) in [-1,1] to a world direction (position, a vec4 whose
 * w is 0) and, if texCoord is non-NULL, the clamped sky-image texcoord. */
void __cdecl MakeSkyVec(float s, float t, int axis, float *texCoord, float *position)
{
  float source[3];
  int   sel, c;

  source[0] = s;
  source[1] = t;
  source[2] = 1.0f;

  for ( c = 0; c < 3; c++ )
  {
    sel = st_to_vec[axis][c];
    if ( sel < 0 )
      position[c] = -source[-sel - 1];
    else
      position[c] = source[sel - 1];
  }
  position[3] = 0.0f;

  /* avoid bilerp seam */
  s = (s + 1.0f) * 0.5f;
  t = (t + 1.0f) * 0.5f;
  if ( s < sky_min )      s = sky_min;
  else if ( s > sky_max ) s = sky_max;
  if ( t < sky_min )      t = sky_min;
  else if ( t > sky_max ) t = sky_max;

  t = 1.0f - t;

  if ( texCoord )
  {
    texCoord[0] = s;
    texCoord[1] = t;
  }
}

/* rendererSkyBox holds the address of the active skyVertexStorage_t; retail
 * re-reads it at every use. */
#define SKYBOX   ( (skyVertexStorage_t *)rendererSkyBox )

/* ---- DrawSkyBox  0x004E6A30 ----  VERIFIED
 * Draw one sky box (innerBox == 0: the opaque outer box; innerBox != 0: the blended inner
 * box) as GL_QUADS over the pre-built 9x9 vertex grid, one face at a time.
 * shader is the sky shader.  RB_SelectStorage and GL_Cull are inlined by retail. */
void __cdecl DrawSkyBox(int shader, int innerBox)
{
  unsigned short indices[256];
  int arraysLocked;
  int i;
  int indexCount;

  RB_EndMultitexture();
  RB_SelectStorage(SKYBOX->memorySource);
  GL_State(256);                           /* GLS_DEPTHMASK_TRUE */
  GL_Cull(0);                              /* CT_FRONT_SIDED */
  GL_ClientState(1025);                    /* texcoord0 + vertex arrays */
  qglColor3f(tr_identityLight, tr_identityLight, tr_identityLight);

  if ( innerBox )                                /* inner box: alpha blended */
  {
    qglBlendFunc(770, 771);                /* GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA */
    glEnable(0xBE2u);                      /* GL_BLEND */
    if ( glState_texEnv[glState_currentTmu] != 8448 )
    {
      glState_texEnv[glState_currentTmu] = 8448;   /* GL_MODULATE */
      glTexEnvi(0x2300u, 0x2200u, 8448);
    }
  }

  arraysLocked = 0;
  switch ( SKYBOX->backend )
  {
    case SKYVB_CLIENT:
      glTexCoordPointer(2, 0x1406u, 24, SKYBOX->base);                   /* GL_FLOAT */
      glVertexPointer(4, 0x1406u, 24, (char *)SKYBOX->base + 8);
      if ( qglLockArraysEXT )
      {
        qglLockArraysEXT(0, 486);
        arraysLocked = 1;
      }
      break;
    case SKYVB_ARB_BUFFER:
      qglBindBufferARB(34962, (int)SKYBOX->base);                       /* GL_ARRAY_BUFFER_ARB */
      glTexCoordPointer(2, 0x1406u, 24, 0);
      glVertexPointer(4, 0x1406u, 24, (const GLvoid *)8);
      break;
    case SKYVB_ATI_OBJECT:
      qglArrayObjectATI(32888, 2, 5126, 24, (int)SKYBOX->base, SKYBOX->objectOffset);      /* GL_TEXTURE_COORD_ARRAY, GL_FLOAT */
      qglArrayObjectATI(32884, 4, 5126, 24, (int)SKYBOX->base, SKYBOX->objectOffset + 8);  /* GL_VERTEX_ARRAY, GL_FLOAT */
      break;
    case SKYVB_NV_MEMORY:
      glTexCoordPointer(2, 0x1406u, 24, SKYBOX->base);
      glVertexPointer(4, 0x1406u, 24, (char *)SKYBOX->base + 8);
      break;
    default:
      return;
  }

  indexCount = 0;
  for ( i = 0; i < 6; i++ )
  {
    int      sky_mins_subd[2], sky_maxs_subd[2];
    int      s, t;
    GLenum  *image;

    sky_mins[0][i]   = FCAST(floor(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS)) / HALF_SKY_SUBDIVISIONS;
    sky_mins_1[i] = FCAST(floor(sky_mins_1[i] * HALF_SKY_SUBDIVISIONS)) / HALF_SKY_SUBDIVISIONS;
    /* plain (float) on the maxs: FCAST's extra parentheses move the reload of the
     * spilled &sky_maxs[0][i] before the fmul (ceil is integral: same value) */
    sky_maxs[0][i]   = (float)ceil(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;
    sky_maxs_1[i] = (float)ceil(sky_maxs_1[i] * HALF_SKY_SUBDIVISIONS) / HALF_SKY_SUBDIVISIONS;

    if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) || ( sky_mins_1[i] >= sky_maxs_1[i] ) )
      continue;

    sky_mins_subd[0] = sky_mins[0][i]   * HALF_SKY_SUBDIVISIONS;
    sky_mins_subd[1] = sky_mins_1[i] * HALF_SKY_SUBDIVISIONS;
    sky_maxs_subd[0] = sky_maxs[0][i]   * HALF_SKY_SUBDIVISIONS;
    sky_maxs_subd[1] = sky_maxs_1[i] * HALF_SKY_SUBDIVISIONS;

    if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS )
      sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
    else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS )
      sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
    if ( sky_mins_subd[1] < -HALF_SKY_SUBDIVISIONS )
      sky_mins_subd[1] = -HALF_SKY_SUBDIVISIONS;
    else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS )
      sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

    if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS )
      sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
    else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS )
      sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
    if ( sky_maxs_subd[1] < -HALF_SKY_SUBDIVISIONS )
      sky_maxs_subd[1] = -HALF_SKY_SUBDIVISIONS;
    else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS )
      sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

    for ( t = sky_mins_subd[1] + HALF_SKY_SUBDIVISIONS; t < sky_maxs_subd[1] + HALF_SKY_SUBDIVISIONS; t++ )
    {
      for ( s = sky_mins_subd[0] + HALF_SKY_SUBDIVISIONS; s < sky_maxs_subd[0] + HALF_SKY_SUBDIVISIONS; s++ )
      {
        unsigned short vertex = 81 * i + 9 * t + s;
        indices[indexCount + 0] = vertex + 1;
        indices[indexCount + 1] = vertex;
        indices[indexCount + 2] = vertex + 9;
        indices[indexCount + 3] = vertex + 10;
        indexCount += 4;
      }
    }

    if ( innerBox )
      image = *(GLenum **)(shader + 4 * sky_texorder[i] + 124);   /* shader->skyInnerBox[texorder] */
    else
      image = *(GLenum **)(shader + 4 * sky_texorder[i] + 100);   /* shader->skyOuterBox[texorder] */
    GL_Bind(image);
    backEnd_pc_drawnIndexCount += indexCount;
    ++backEnd_pc_drawCallCount;
    glDrawElements(7u, indexCount, 0x1403u, indices);            /* GL_QUADS, GL_UNSIGNED_SHORT */

    if ( r_showtris->integer )
    {
      glPolygonMode(0x408u, 0x1B01u);      /* GL_FRONT_AND_BACK, GL_LINE */
      glDisable(0xDE1u);                   /* GL_TEXTURE_2D */
      qglColor3f(1.0f, 1.0f, 1.0f);
      backEnd_pc_drawnIndexCount += indexCount;
      ++backEnd_pc_drawCallCount;
      glDrawElements(7u, indexCount, 0x1403u, indices);
      glEnable(0xDE1u);
      glPolygonMode(0x408u, 0x1B02u);      /* GL_FILL */
    }

    backEnd_pc_indexCount  += indexCount;
    backEnd_pc_vertexCount += 486;
    indexCount = 0;
  }

  if ( arraysLocked )
    qglUnlockArraysEXT();
  if ( innerBox )
    glDisable(0xBE2u);                     /* GL_BLEND */
  if ( SKYBOX->backend == SKYVB_ARB_BUFFER )
    qglBindBufferARB(34962, 0);
}

/* the cloud texcoords as retail's [side][t][s][st] grid */
#define s_cloudTexCoordsGrid ( (float (*)[9][9][2])s_cloudTexCoords )
#define s_cloudTexPGrid      ( (float (*)[9][9])s_cloudTexP )

/* ---- FillCloudySkySide  0x004E6F80 ----  VERIFIED
 * Emit the cloud-layer vertices (and, for the first stage, the indices) for one
 * cube face.  mins/maxs are the clamped [-4,4] grid extents.  Returns
 * tHeight-1 when indices were added, else 0 (dead EAX). */
void __cdecl FillCloudySkySide(int *mins, int *maxs, int side, int addIndexes)
{
  int vertexStart = tess_numVertexes;
  unsigned short tHeight;
  unsigned short sWidth;
  unsigned short s,t;

  tHeight = maxs[1] - mins[1] + 1;
  sWidth  = maxs[0] - mins[0] + 1;

  for ( t = mins[1] + 4; t <= maxs[1] + 4; t++ )
  {
    for ( s = mins[0] + 4; s <= maxs[0] + 4; s++ )
    {
      MakeSkyVec((float)(s - 4) * 0.25f, (float)(t - 4) * 0.25f, side, 0,
                 &tess_xyz[tess_numVertexes * tess_vertexComponentCount]);
      tess_texCoords0[2 * tess_numVertexes + 0] = s_cloudTexCoordsGrid[side][t][s][0];
      tess_texCoords0[2 * tess_numVertexes + 1] = s_cloudTexCoordsGrid[side][t][s][1];
      tess_numVertexes++;
      if ( tess_numVertexes >= 0x2000 )
        ri.Error(1, "\x15" "SHADER_MAX_VERTEXES hit in FillCloudySkySide()\n");
    }
  }

  /* only the first stage adds indices, else the box would draw once per stage */
  if ( addIndexes )
  {
    for ( t = 0; t < tHeight - 1; t++ )
    {
      for ( s = 0; s < sWidth - 1; s++ )
      {
        tess_indexes[tess_numIndexes] = vertexStart + s + t * sWidth;
        tess_numIndexes++;
        tess_indexes[tess_numIndexes] = vertexStart + s + ( t + 1 ) * sWidth;
        tess_numIndexes++;
        tess_indexes[tess_numIndexes] = vertexStart + s + 1 + t * sWidth;
        tess_numIndexes++;

        tess_indexes[tess_numIndexes] = vertexStart + s + ( t + 1 ) * sWidth;
        tess_numIndexes++;
        tess_indexes[tess_numIndexes] = vertexStart + s + 1 + ( t + 1 ) * sWidth;
        tess_numIndexes++;
        tess_indexes[tess_numIndexes] = vertexStart + s + 1 + t * sWidth;
        tess_numIndexes++;
      }
    }
  }
}

/* ---- FillCloudBox  0x004E71D0 ----  VERIFIED
 * Build the cloud geometry for all faces except the bottom (Q3's FillCloudBox
 * with fullClouds; MIN_T stays a float, so the t clamps compare as floats).
 * shader (the shader) is unused by retail. */
void __cdecl FillCloudBox(int shader, int stage)
{
  int   i;
  int   sky_mins_subd[2], sky_maxs_subd[2];
  float MIN_T;

  (void)shader;

  for ( i = 0; i < 6; i++ )
  {
    MIN_T = -HALF_SKY_SUBDIVISIONS;

    /* still don't want to draw the bottom, even if fullClouds */
    if ( i == 5 )
      continue;

    sky_mins[0][i]   = FCAST(floor(sky_mins[0][i] * HALF_SKY_SUBDIVISIONS)) / HALF_SKY_SUBDIVISIONS;
    sky_mins_1[i] = FCAST(floor(sky_mins_1[i] * HALF_SKY_SUBDIVISIONS)) / HALF_SKY_SUBDIVISIONS;
    sky_maxs[0][i]   = FCAST(ceil(sky_maxs[0][i] * HALF_SKY_SUBDIVISIONS)) / HALF_SKY_SUBDIVISIONS;
    sky_maxs_1[i] = FCAST(ceil(sky_maxs_1[i] * HALF_SKY_SUBDIVISIONS)) / HALF_SKY_SUBDIVISIONS;

    if ( ( sky_mins[0][i] >= sky_maxs[0][i] ) || ( sky_mins_1[i] >= sky_maxs_1[i] ) )
      continue;

    sky_mins_subd[0] = myftol(sky_mins[0][i]   * HALF_SKY_SUBDIVISIONS);
    sky_mins_subd[1] = myftol(sky_mins_1[i] * HALF_SKY_SUBDIVISIONS);
    sky_maxs_subd[0] = myftol(sky_maxs[0][i]   * HALF_SKY_SUBDIVISIONS);
    sky_maxs_subd[1] = myftol(sky_maxs_1[i] * HALF_SKY_SUBDIVISIONS);

    if ( sky_mins_subd[0] < -HALF_SKY_SUBDIVISIONS )
      sky_mins_subd[0] = -HALF_SKY_SUBDIVISIONS;
    else if ( sky_mins_subd[0] > HALF_SKY_SUBDIVISIONS )
      sky_mins_subd[0] = HALF_SKY_SUBDIVISIONS;
    if ( sky_mins_subd[1] < MIN_T )
      sky_mins_subd[1] = MIN_T;
    else if ( sky_mins_subd[1] > HALF_SKY_SUBDIVISIONS )
      sky_mins_subd[1] = HALF_SKY_SUBDIVISIONS;

    if ( sky_maxs_subd[0] < -HALF_SKY_SUBDIVISIONS )
      sky_maxs_subd[0] = -HALF_SKY_SUBDIVISIONS;
    else if ( sky_maxs_subd[0] > HALF_SKY_SUBDIVISIONS )
      sky_maxs_subd[0] = HALF_SKY_SUBDIVISIONS;
    if ( sky_maxs_subd[1] < MIN_T )
      sky_maxs_subd[1] = MIN_T;
    else if ( sky_maxs_subd[1] > HALF_SKY_SUBDIVISIONS )
      sky_maxs_subd[1] = HALF_SKY_SUBDIVISIONS;

    /* only add indexes for the first stage */
    FillCloudySkySide(sky_mins_subd, sky_maxs_subd, i, ( stage == 0 ));
  }
}

/* ---- RB_BuildCloudData  0x004E73F0 ----  VERIFIED
 * Generate the cloud vertices for every active stage of the sky shader.  input
 * is &tess (retail passes it in EDI); the counters are written through tess. */
void __cdecl RB_BuildCloudData(shaderCommands_t *input)
{
  int i;

  tess_numIndexes = 0;
  tess_numVertexes = 0;
  tess_vertexComponentCount = 4;
  sky_min = 0.00390625f;                   /* 1/256 */
  sky_max = 0.99609375f;                   /* 255/256 */

  if ( *(float *)(input->shader + 96) != 0.0f )   /* shader->skyCloudHeight */
  {
    for ( i = 0; i < 8; i++ )
    {
      if ( !*(int *)(tess_activeStages + 4 * i) )
        break;
      FillCloudBox(input->shader, i);
    }
  }
}

/* Static vertex memory for the NV vertex-array-range path: primary pool, then
 * secondary, then an aligned hunk block.  Returns the memory source (1/2/3). */
static __forceinline int R_AllocSkyVertexMemoryNV(void **base, int size)
{
  if ( tr_staticVertexMemoryPrimaryUsed + size < tr_staticVertexMemoryPrimaryLimit )
  {
    *base = (void *)(tr_staticVertexMemoryPrimary + tr_staticVertexMemoryPrimaryUsed);
    tr_staticVertexMemoryPrimaryUsed += size;
    return 1;
  }
  if ( tr_staticVertexMemorySecondaryUsed + size < tr_staticVertexMemorySecondaryLimit )
  {
    *base = (void *)(tr_staticVertexMemorySecondary + tr_staticVertexMemorySecondaryUsed);
    tr_staticVertexMemorySecondaryUsed += size;
    return 2;
  }
  *base = (void *)Hunk_AllocAlignInternal(size, 32);
  return 3;
}

/* ATI object-buffer offset in the primary pool, else the secondary.  Returns
 * the pool (1/2), or 0 when both are full. */
static __forceinline int R_AllocSkyVertexMemoryATI(unsigned *offset, int size)
{
  if ( tr_staticVertexMemoryPrimaryUsed + size <= tr_staticVertexMemoryPrimaryLimit )
  {
    *offset = tr_staticVertexMemoryPrimaryUsed;
    tr_staticVertexMemoryPrimaryUsed += size;
    return 1;
  }
  if ( tr_staticVertexMemorySecondaryUsed + size <= tr_staticVertexMemorySecondaryLimit )
  {
    *offset = tr_staticVertexMemorySecondaryUsed;
    tr_staticVertexMemorySecondaryUsed += size;
    return 2;
  }
  return 0;
}

/* ---- R_BuildSkyBox  0x004E7470 ----  VERIFIED
 * Upload the 486-vertex sky box through whichever static-vertex backend is
 * available (ARB VBO, NV vertex-array range, ATI object buffer) and fall back
 * to a client-side hunk copy otherwise.  The NV/ATI pool helpers are inlined
 * by retail. */
void __cdecl R_BuildSkyBox(const skyVertex_t *skyVertices)
{
  if ( glConfig_ARBVertexBufferObject )
  {
    SKYBOX->memorySource = storageClass;
    SKYBOX->backend = SKYVB_ARB_BUFFER;
    SKYBOX->base = (void *)R_CreateBufferARB(34962, 11664, (int)skyVertices, 35044);   /* GL_ARRAY_BUFFER_ARB, GL_STATIC_DRAW_ARB */
    if ( !SKYBOX->base )
    {
      /* buffer creation failed (or no usable backend): client copy */
clientCopy:
      SKYBOX->memorySource = 3;
      SKYBOX->backend = SKYVB_CLIENT;
      SKYBOX->base = (void *)ri.Hunk_Alloc(11664);
      memcpy(SKYBOX->base, skyVertices, 0x2D90u);
    }
  }
  else if ( glConfig_NVVertexArrayRange )
  {
    SKYBOX->memorySource = R_AllocSkyVertexMemoryNV(&SKYBOX->base, 11680);
    SKYBOX->backend = SKYVB_NV_MEMORY;
    memcpy(SKYBOX->base, skyVertices, 0x2D90u);
  }
  else if ( glConfig_ATIVertexArrayObject )
  {
    SKYBOX->memorySource = R_AllocSkyVertexMemoryATI(&SKYBOX->objectOffset, 11680);
    SKYBOX->backend = SKYVB_ATI_OBJECT;
    switch ( SKYBOX->memorySource )
    {
      case 1:
        SKYBOX->memorySource = 3;
        SKYBOX->base = (void *)tr_staticVertexMemoryPrimary;
        qglUpdateObjectBufferATI((GLuint)SKYBOX->base, SKYBOX->objectOffset, 11664, skyVertices, 34658);   /* GL_PRESERVE_ATI */
        break;
      case 2:
        SKYBOX->memorySource = 3;
        SKYBOX->base = (void *)tr_staticVertexMemorySecondary;
        qglUpdateObjectBufferATI((GLuint)SKYBOX->base, SKYBOX->objectOffset, 11664, skyVertices, 34658);
        break;
      default:
        goto clientCopy;                   /* both ATI pools full */
    }
  }
  else
  {
    goto clientCopy;
  }
}

/* ---- R_SetSkyBox  0x004E7690 ----  [HIGH] */
int __cdecl R_SetSkyBox(int result)
{
  rendererSkyBox = result;
  return result;
}

/* ---- R_InitSkyTexCoords  0x004E76A0 ----  VERIFIED
 * Called when a sky shader is parsed: for every point of every cube face,
 * intersect the sky ray with the cloud sphere, storing the ray scale
 * (s_cloudTexP) and the angular cloud texcoord (s_cloudTexCoords), then upload
 * the sky box. */
void __cdecl R_InitSkyTexCoords(float cloudHeight)
{
  skyVertex_t  skyVertices[SKY_VERTEX_COUNT];
  skyVertex_t *vert;
  float        cloudHeightSq;
  float        cloudHeightSqPlusRadSq;
  float        xx, yy, zz;
  float        root, p;
  float        sRad, tRad;
  float        tScale;
  vec3_t       skyVec;
  vec3_t       v;
  int          side, t, s;

  cloudHeightSq          = cloudHeight * cloudHeight;
  cloudHeightSqPlusRadSq = cloudHeightSq + 16777216.0f;   /* + radiusWorld^2 (4096^2) */

  sky_min = 0.0f;
  /* init zFar so MakeSkyVec works before a world is bounded; R_RenderView
   * overwrites it with the active value before rendering.  1090519040 == 8.0f. */
  backEnd_viewParms_zFar = 1090519040;
  sky_max = 1.0f;

  for ( side = 0; side < SKYBOX_FACE_COUNT; side++ )
  {
    for ( t = 0; t <= SKY_GRID_SIDE - 1; t++ )
    {
      tScale = ( t - SKY_GRID_CENTER ) / 4.0f;
      for ( s = 0; s <= SKY_GRID_SIDE - 1; s++ )
      {
        vert = &skyVertices[( side * SKY_GRID_SIDE + t ) * SKY_GRID_SIDE + s];
        MakeSkyVec( ( s - SKY_GRID_CENTER ) / 4.0f, tScale, side, vert->texCoord, vert->position );
        VectorCopy( vert->position, skyVec );

        /* parametric 'p' where the sky ray meets the cloud sphere */
        zz = skyVec[2] * skyVec[2];
        yy = skyVec[1] * skyVec[1];
        xx = skyVec[0] * skyVec[0];
        root = sqrt( ( xx + yy + zz ) * cloudHeight * 8192.0f
                   + ( xx + yy ) * cloudHeightSq
                   + cloudHeightSqPlusRadSq * zz );
        p = ( 2 * root - skyVec[2] * 8192.0f ) * ( 1.0f / ( 2 * ( xx + yy + zz ) ) );
        s_cloudTexPGrid[side][t][s] = p;

        VectorScale( skyVec, p, v );
        v[2] += 4096.0f;
        VectorNormalize( v );

        sRad = Q_acos( v[0] );
        tRad = Q_acos( v[1] );

        s_cloudTexCoordsGrid[side][t][s][0] = sRad;
        s_cloudTexCoordsGrid[side][t][s][1] = tRad;
      }
    }
  }

  R_BuildSkyBox(skyVertices);
}

/* ---- R_FindSunSpriteShader  0x004E7930 ----  VERIFIED
 * Find the sun sprite shader and clear the depth-write bit on each of its
 * stages so the sprite never occludes anything. */
char *__cdecl R_FindSunSpriteShader(char *shaderName)
{
  char *shader;
  int   stage;
  int   i;

  shader = (char *)R_FindShader(shaderName, -3, 0, (const char *)4);
  for ( i = 0; i < 8; i++ )
  {
    stage = ((int *)(shader + 340))[i];    /* shader->stages[i] (+0x154) */
    if ( !stage )
      break;
    *(int *)(stage + 1668) &= ~0x100u;     /* stage->stateBits &= ~GLS_DEPTHMASK_TRUE (+0x684) */
  }
  return shader;
}

void Perp2D( vec3_t out, const vec3_t in );   /* universal/com_math.c */

/* ---- R_SetSunSpriteSize  0x004E7980 ----  VERIFIED
 * Build the four sun-sprite corner vertices from tr.sunDirection.
 * tr_sunDirection[0..2] are tr.sunDirection's components.  Retail inlines the
 * /Op com_math.c helpers (Perp2D, CrossProduct), so every vector lives in a
 * stack slot. */
void __cdecl R_SetSunSpriteSize(float spriteSize)
{
  vec3_t reference;
  vec3_t right;
  vec3_t up;
  vec3_t rpu;                              /* right + up */
  vec3_t rmu;                              /* right - up */
  float  scale;
  float  z;

  scale = spriteSize * 0.001311093f;               /* angular size -> displacement */

  /* pick a stable reference perpendicular to the sun direction */
  z = tr_sunDirection[2];
  if ( z * z > 0.99000001f )
    VectorSet(reference, 1.0f, 0.0f, 0.0f);
  else
    Perp2D(reference, tr_sunDirection);

  CrossProduct(tr_sunDirection, reference, right);
  VectorNormalize(right);
  VectorScale(right, scale, right);
  CrossProduct(right, tr_sunDirection, up);

  VectorAdd(up, right, rpu);
  VectorSubtract(right, up, rmu);

  VectorAdd(tr_sunDirection, rpu, &rendererSunState_spriteVert0X);
  VectorAdd(tr_sunDirection, rmu, &rendererSunState_spriteVert1X);
  VectorSubtract(tr_sunDirection, rpu, &rendererSunState_spriteVert2X);
  VectorSubtract(tr_sunDirection, rmu, &rendererSunState_spriteVert3X);
  rendererSunState_spriteVert0W = 0;
  rendererSunState_spriteVert1W = 0;
  rendererSunState_spriteVert2W = 0;
  rendererSunState_spriteVert3W = 0;
  rendererSunState_spriteSize = spriteSize;
}

/* client_mp/cl_console_mp.c 0x00408370, inlined by LTCG: fld / fadd qword
 * 2^-30 / fistp -- round-to-nearest, not the truncating __ftol2 of an (int)
 * cast.  So r_sunflare_fadein 0.7 gives 700, not 699. */
extern int FastRound( float f );

/* DEG2RAD with the float M_PI: retail fmul flt_568F90 (pi) then fmul
 * flt_569098 (0x3BB60B61, VC7's reciprocal of the 180.0f divisor). */
#ifdef COD1_VC7
#define SUN_DEG2RAD( a )   ( ( (a) * M_PI ) / 180.0f )
#else
#define SUN_DEG2RAD( a )   ( (double)(a) * (double)M_PI * (double)0.0055555557f )
#endif

/* ---- R_SetSunFromCvars  0x004E7BE0 ----  [HIGH]
 * Rebuild the whole sun state from the r_sun* cvars.  Returns dead EAX (the
 * last glare fade-in value).  Fields holding float bit patterns in int slots
 * (flareMaxAlpha / blindMaxDarken / glareMaxLighten) are reinterpreted. */
int R_SetSunFromCvars()
{
  int result;

  rendererSunState_spriteShader = 0;
  if ( r_sunsprite_shader->string[0] )
    rendererSunState_spriteShader = (int)R_FindSunSpriteShader(r_sunsprite_shader->string);
  R_SetSunSpriteSize(r_sunsprite_size->value);

  rendererSunState_flareShader = 0;
  if ( r_sunflare_shader->string[0] )
    rendererSunState_flareShader = (int)R_FindShader(r_sunflare_shader->string, -3, 0, (const char *)4);

  rendererSunState_flareMinHalfSize = r_sunflare_min_size->value * 0.5f;
  rendererSunState_flareMinCosAngle = cos(SUN_DEG2RAD(r_sunflare_min_angle->value));
  rendererSunState_flareMaxHalfSize = r_sunflare_max_size->value * 0.5f;
  rendererSunState_flareMaxCosAngle = cos(SUN_DEG2RAD(r_sunflare_max_angle->value));
  rendererSunState_flareMaxAlpha    = *(int *)&r_sunflare_max_alpha->value;
  rendererSunState_flareFadeInMsec  = FastRound(r_sunflare_fadein->value * 1000.0f);
  rendererSunState_flareFadeOutMsec = FastRound(r_sunflare_fadeout->value * 1000.0f);

  rendererSunState_blindMinCosAngle = cos(SUN_DEG2RAD(r_sunblind_min_angle->value));
  rendererSunState_blindMaxCosAngle = cos(SUN_DEG2RAD(r_sunblind_max_angle->value));
  rendererSunState_blindMaxDarken   = *(int *)&r_sunblind_max_darken->value;
  rendererSunState_blindFadeInMsec  = FastRound(r_sunblind_fadein->value * 1000.0f);
  rendererSunState_blindFadeOutMsec = FastRound(r_sunblind_fadeout->value * 1000.0f);

  rendererSunState_glareMinCosAngle = cos(SUN_DEG2RAD(r_sunglare_min_angle->value));
  rendererSunState_glareMaxCosAngle = cos(SUN_DEG2RAD(r_sunglare_max_angle->value));
  rendererSunState_glareMaxLighten  = *(int *)&r_sunglare_max_lighten->value;
  result                            = FastRound(r_sunglare_fadein->value * 1000.0f);
  rendererSunState_glareFadeInMsec  = result;
  rendererSunState_glareFadeOutMsec = FastRound(r_sunglare_fadeout->value * 1000.0f);
  return result;
}

/* ---- R_SaveSunFromCvars  0x004E7EA0 ----  VERIFIED
 * ri.SaveCvarsToBuffer is passed the ADDRESS of the r_sunCvarNames table. */
void __cdecl R_SaveSunFromCvars(const char *sunName)
{
  char buffer[65536];

  if ( ri.SaveCvarsToBuffer(r_sunCvarNames, 20, buffer, 0x10000) )
    ri.FS_WriteFile(va("scripts/%s.sun", sunName), buffer, strlen(buffer));
}

/* ---- R_LoadSunThroughCvars  0x004E7F30 ----  VERIFIED */
void __cdecl R_LoadSunThroughCvars(const char *sunName)
{
  char *fileName;
  int   fileBuffer;                        /* [ebp-4h] BYREF */

  fileName = va("scripts/%s.sun", sunName);
  if ( ri.FS_ReadFile(fileName, &fileBuffer) < 0 )
  {
    ri.Printf(0, "^3WARNING: couldn't load sun file '%s'\n", fileName);
    return;
  }
  if ( ri.LoadCvarsFromBuffer(r_sunCvarNames, 20, fileBuffer, fileName) )
    R_SetSunFromCvars();
  ri.FS_FreeFile(fileBuffer);
}

/* ---- R_SaveSun_f  0x004E7FA0 ----  [HIGH] */
void R_SaveSun_f()
{
  if ( ri.Cmd_Argc() != 2 )
  {
    ri.Printf(0, "USAGE: r_savesun <sunname>\n  sunname must not have an extension\n");
    return;
  }
  R_SaveSunFromCvars((const char *)ri.Cmd_Argv(1));
}

/* ---- R_LoadSun_f  0x004E7FD0 ----  [HIGH] */
void R_LoadSun_f()
{
  if ( ri.Cmd_Argc() != 2 )
  {
    ri.Printf(0, "USAGE: r_loadsun <sunname>\n  sunname must not have an extension\n");
    return;
  }
  if ( !sv_cheats->integer )
  {
    ri.Printf(0, "You must have cheats enabled to use r_loadsun\n");
    return;
  }
  R_LoadSunThroughCvars((const char *)ri.Cmd_Argv(1));
}

/* ---- R_SunHelp_f  0x004E8020 ----  VERIFIED */
void R_SunHelp_f()
{
	int i;

	ri.Printf(0, "\n=== SUN COMMANDS ===\n");
	ri.Printf(0, "r_loadsun <sunname> -- loads sun from 'scripts/<sunname>.sun'\n");
	ri.Printf(0, "r_savesun <sunname> -- saves sun as 'scripts/<sunname>.sun'\n");
	ri.Printf(0, "\n=== SUN CVARS ===\n");
	ri.Printf(0, "(must have r_suntest set to 1 to tweak these values)\n");
	for ( i = 0; i < sizeof( r_sunCvarNames ) / sizeof( r_sunCvarNames[0] ); ++i )
	{
		if ( i > 0 )
		{
			if ( _strnicmp(r_sunCvarNames[i - 1], r_sunCvarNames[i], 8u) )
				ri.Printf(0, "\n");
		}
		ri.Printf(0, "^2%-22s^7 %s\n", r_sunCvarNames[i], r_sunCvarHelp[i]);
	}
}

/* ---- RB_DrawSunSprite  0x004E80C0 ----  VERIFIED
 * Emit the sun sprite (the pre-built quad in rendererSunState) into tess. */
void RB_DrawSunSprite()
{
  if ( !rendererSunState_spriteShader )
    return;

  RB_SelectStorage(storageClass);          /* inlined by retail */

  RB_BeginSurface((void *)rendererSunState_spriteShader, 4);
  tess_stageIterator = tr_stageIteratorFunc;

  memcpy(tess_xyz, &rendererSunState_spriteVert0X, 0x40u);   /* 4 vec4 corner verts */

  tess_texCoords0[0] = 1.0f;   tess_texCoords0[1] = 1.0f;     /* v0 (1,1) */
  tess_texCoords0[2] = 0.0f;   tess_texCoords0[3] = 1.0f;     /* v1 (0,1) */
  tess_texCoords0[4] = 0.0f;   tess_texCoords0[5] = 0.0f;     /* v2 (0,0) */
  tess_texCoords0[6] = 1.0f;   tess_texCoords0[7] = 0.0f;     /* v3 (1,0) */

  tess_numVertexes = 4;
  tess_indexes[0] = 0;
  tess_indexes[1] = 1;
  tess_indexes[2] = 2;
  tess_indexes[3] = 0;
  tess_indexes[4] = 2;
  tess_indexes[5] = 3;
  tess_numIndexes = 6;

  RB_EndSurface();
}

/* ---- RB_CalcSunFlare  0x004E81E0 ----  [HIGH]
 * If the sun is within the flare cone, submit a sun flare whose alpha/size ramp
 * with the view-to-sun angle.  a2/a3 to RB_UpdateSunFlare carry float bit
 * patterns in int slots. */
void __cdecl RB_CalcSunFlare(float sunCosAngle)
{
  rfloat frac;
  float alpha,size;

  if ( !rendererSunState_flareShader )
    return;
  if ( sunCosAngle <= rendererSunState_flareMinCosAngle )
    return;

  if ( sunCosAngle >= rendererSunState_flareMaxCosAngle )
    frac = 1.0f;
  else
    frac = (sunCosAngle - rendererSunState_flareMinCosAngle)
         / (rendererSunState_flareMaxCosAngle - rendererSunState_flareMinCosAngle);

  size  = rendererSunState_flareMaxHalfSize * frac + rendererSunState_flareMinHalfSize;
  alpha = *(float *)&rendererSunState_flareMaxAlpha * frac;
  RB_UpdateSunFlare(rendererSunState_flareShader, alpha,size,
                    rendererSunState_spriteSize,
                    rendererSunState_flareFadeInMsec, rendererSunState_flareFadeOutMsec);
}

/* ---- RB_CalcSunBlind  0x004E8270 ----  [HIGH]
 * Age the sun blind/glare fractions toward the view-angle target scaled by the
 * sun's current visibility (visibility), writing *outGlare = glare rgb level and *outBlind = blind
 * overlay alpha. */
void __cdecl RB_CalcSunBlind(float *outBlind, float *outGlare, float visibility)
{
  int    elapsed;
  rfloat frac;
  rfloat updated;
  float  sunViewDot;

  if ( !rendererSunState_lastUpdateTime )
    elapsed = 10;
  else
    elapsed = backEnd_refdef_time - rendererSunState_lastUpdateTime;
  rendererSunState_lastUpdateTime = backEnd_refdef_time;

#ifdef COD1_VC7
  sunViewDot = tr_sunDirection[0] * backEnd_viewParms_axis00   /* VC7 sums this as [2],[1],[0], like retail */
             + tr_sunDirection[1] * backEnd_viewParms_axis01
             + tr_sunDirection[2] * backEnd_viewParms_axis02;
#else
  sunViewDot = tr_sunDirection[2] * backEnd_viewParms_axis02
             + tr_sunDirection[1] * backEnd_viewParms_axis01
             + tr_sunDirection[0] * backEnd_viewParms_axis00;
#endif

  if ( *(float *)&rendererSunState_blindMaxDarken <= 0.0f )
  {
    *outBlind = 0.0f;
  }
  else
  {
    if ( sunViewDot <= rendererSunState_blindMinCosAngle )
      frac = 0.0f;
    else if ( sunViewDot >= rendererSunState_blindMaxCosAngle )
      frac = 1.0f;
    else
      frac = (sunViewDot - rendererSunState_blindMinCosAngle)
           / (rendererSunState_blindMaxCosAngle - rendererSunState_blindMinCosAngle);
    updated = R_UpdateOverTime(rendererSunState_currentBlindFraction, FCAST(frac * visibility),
                               rendererSunState_blindFadeInMsec, rendererSunState_blindFadeOutMsec,
                               elapsed);
    rendererSunState_currentBlindFraction = updated;
#ifdef COD1_VC7
    *outBlind = rendererSunState_currentBlindFraction * *(float *)&rendererSunState_blindMaxDarken;   /* VC7 reuses the unrounded value */
#else
    *outBlind = updated * *(float *)&rendererSunState_blindMaxDarken;
#endif
  }

  if ( *(float *)&rendererSunState_glareMaxLighten <= 0.0f )
  {
    *outGlare = 0.0f;
  }
  else
  {
    if ( sunViewDot <= rendererSunState_glareMinCosAngle )
      frac = 0.0f;
    else if ( sunViewDot >= rendererSunState_glareMaxCosAngle )
      frac = 1.0f;
    else
      frac = (sunViewDot - rendererSunState_glareMinCosAngle)
           / (rendererSunState_glareMaxCosAngle - rendererSunState_glareMinCosAngle);
    updated = R_UpdateOverTime(rendererSunState_currentGlareFraction, FCAST(frac * visibility),
                               rendererSunState_glareFadeInMsec, rendererSunState_glareFadeOutMsec,
                               elapsed);
    rendererSunState_currentGlareFraction = updated;
#ifdef COD1_VC7
    *outGlare = rendererSunState_currentGlareFraction * *(float *)&rendererSunState_glareMaxLighten;   /* VC7 reuses the unrounded value */
#else
    *outGlare = updated * *(float *)&rendererSunState_glareMaxLighten;
#endif
  }
}

/* ---- RB_AddSunEffects  0x004E83F0 ----  [HIGH]
 * When a sun exists and it is in front of the eye, draw the sun sprite and
 * queue its flare. */
void RB_AddSunEffects()
{
  float sunViewDot;

  if ( !*(int *)(tr_world + 276) )         /* tr.world->sunLight */
    return;

  if ( r_suntest->integer )
    R_SetSunFromCvars();

#ifdef COD1_VC7
  sunViewDot = tr_sunDirection[0] * backEnd_viewParms_axis00   /* VC7 sums this as [2],[1],[0], like retail */
             + tr_sunDirection[1] * backEnd_viewParms_axis01
             + tr_sunDirection[2] * backEnd_viewParms_axis02;
#else
  sunViewDot = tr_sunDirection[2] * backEnd_viewParms_axis02
             + tr_sunDirection[1] * backEnd_viewParms_axis01
             + tr_sunDirection[0] * backEnd_viewParms_axis00;
#endif
  if ( sunViewDot <= 0.0f )
    return;

  RB_DrawSunSprite();
  RB_CalcSunFlare(sunViewDot);
}

/* ---- RB_DrawSun  0x004E8460 ----  [HIGH]
 * The sky shader is captured BEFORE RB_AddSunEffects (retail holds it in ESI,
 * 0x004E846D / 0x004E847A): RB_DrawSunSprite re-begins tess with the sun
 * sprite shader, so tess_shader read after the call would be the wrong one. */
void RB_DrawSun()
{
  void *skyShader;

  if ( !r_drawSun->integer )
    return;

  skyShader = (void *)tess_shader;         /* retail keeps it in ESI across the call */
  RB_AddSunEffects();
  RB_BeginSurface(skyShader, 3);
}

/* ---- R_ClearSun  0x004E8490 ----  [HIGH] */
void R_ClearSun()
{
  rendererSunState_currentBlindFraction = 0.0;
  rendererSunState_currentGlareFraction = 0.0;
  rendererSunState_lastUpdateTime = 0;
}

/* ---- R_FlushSun  0x004E84B0 ----  [HIGH]
 * Zero the whole 156-byte sun state. */
void R_FlushSun()
{
  Com_Memset(&rendererSunState, 0, 0x9Cu);
}

/* ---- RB_StageIteratorSky  0x004E84D0 ----  VERIFIED
 * The sky-shader stage iterator: clip the sky polygons, draw the outer box, the
 * sun, the cloud stages, then the inner box.  RB_DrawSun and R_FogOff are
 * inlined by retail; the log-comment file guards are kept. */
void RB_StageIteratorSky()
{
  int outerImage;
  int innerImage;

  if ( r_fastsky->integer )
  {
    RB_DrawSun();                          /* inlined by retail (0x004E84EF / 0x004E84FC) */
    return;
  }

  /* with a skybox-portal view present, only render the sky for that view */
  if ( rendererSkyboxPortal && (backEnd_refdef_rdflags & 8) == 0 )
    return;

  /* the active fog can suppress the sky */
  if ( backEnd_viewParms_glFogRegistered )
  {
    if ( !backEnd_viewParms_glFogDrawSky )
      return;
  }
  else if ( glfogNum > 0 )
  {
    if ( !tr_fogCurrent_drawSky )                  /* glfogsettings[FOG_CURRENT].drawsky */
      return;
  }

  sub_50AF10("--- RB_StageIteratorSky ---\n");

  backEnd_refdef_rdflags |= 0x40u;         /* RDF_DRAWINGSKY */
  RB_ClipSkyPolygons(&tess);

  /* r_showsky draws the sky in front of everything to reveal how much is drawn */
  if ( r_showsky->integer )
    glDepthRange(0.0, 0.0);
  else
    glDepthRange(1.0, 1.0);

  /* R_FogOff (inlined) */
  if ( (glState_glStateBits & 0x200000) != 0 )
  {
    glDisable(0xB60u);                     /* GL_FOG */
    glState_glStateBits &= ~0x200000u;
  }

  outerImage = *(int *)(tess_shader + 100);   /* shader->skyOuterBox[0] */
  if ( outerImage && outerImage != tr_defaultImage )
    DrawSkyBox(tess_shader, 0);

  tess_numIndexes = 0;

  /* RB_DrawSun (inlined by retail, 0x004E8600 / 0x004E8609) re-begins tess with
   * the sky shader, not the sun sprite's */
  RB_DrawSun();

  RB_BuildCloudData(&tess);
  if ( tess_numIndexes )
    tr_stageIteratorFunc(0);

  innerImage = *(int *)(tess_shader + 124);   /* shader->skyInnerBox[0] */
  if ( innerImage && innerImage != tr_defaultImage )
    DrawSkyBox(tess_shader, 1);

  R_FogOn();
  glDepthRange(0.0, 1.0);
  backEnd_refdef_rdflags &= ~0x40u;
  backEnd_skyRenderedThisView = 1;

  sub_50AF10("----------\n");
}
