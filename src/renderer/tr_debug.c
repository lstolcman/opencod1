/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_shaderregistry.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_dpvsplane.h"   /* dpvsPlane_t (normal/dist/sideOffsets) for the DPVS box cull */
#include "tr_local.h"       /* shared typed views; include AFTER tr_records.h (needs TRG) */
/* rendererDebugState is a view of the last 0x80 bytes of tr (tr_local.h) */
/* Keep the inherited generic names local to this renderer unit. */
#define pointer (rendererDebugState.polygonVertices)
#define mode (rendererDebugState.immediateMode)


extern int GL_Bind();
extern int GL_ClientState();
extern int GL_State();
void RB_BeginImmediateMode( void );
void __cdecl RB_glVertex3f( float x, float y, float z );
unsigned __int64 __cdecl RB_glColor4fv(const float *v);
int __cdecl RB_AddQuadStampExt( float *origin, float *left, float *up, int *color,
                                float s1, float t1, float s2, float t2 );
void __cdecl RB_BeginSurface( void *shader, int vertexComponentCount );
extern int RB_EndMultitexture();
void __cdecl RB_EndSurface( void );
extern int RB_SelectStorageNV();
void __cdecl RE_RegisterFont( int name, int size, int fontInfo, const char *usage );
extern int R_FogOn();
int __cdecl R_AddDebugString( int *pos, int *color, int scaleBits, char *text );

/* -------------------------------------------------------------------------
 *  File-local record layouts for the renderer's debug-draw stores.
 *
 *  The RB_Draw functions consume, and the R_Add / RE_ functions produce,
 *  fixed-stride arrays that the decompiled tree walked as
 *  `*(unsigned int *)(base + off)`.  The record types below
 *  are proven by those reads/writes and cross-checked against the CoDUO
 *  reconstruction (renderer_debug_*), then pinned with offset asserts so a
 *  wrong offset fails the build.  The pool base pointers stay on the existing
 *  globals (rbDebug_lines/strings/polygons/plumes, and `pointer` for the debug
 *  polygon vertices); only the element type is named here.
 * ------------------------------------------------------------------------- */

/* debugPolygon_t (0x18) -- one filled+wireframe debug polygon; its vertices
 * live separately in the `pointer` vec3 array at [firstVertex..+vertexCount). */
typedef struct {
	vec4_t color;         /* +0x00 */
	int    firstVertex;   /* +0x10  index into the debug polygon vertex array */
	int    vertexCount;   /* +0x14 */
} debugPolygon_t;
COD1_ASSERT_SIZE(debugPolygon_t, 0x18);
ASSERT_OFF(debugPolygon_t, firstVertex, 0x10);
ASSERT_OFF(debugPolygon_t, vertexCount, 0x14);

/* debugLine_t (0x2c) -- one colored line segment (CoDUO renderer_debug_line_t).
 * depthTest == 0 draws the line without a depth test. */
typedef struct {
	vec3_t start;         /* +0x00 */
	vec3_t end;           /* +0x0c */
	vec4_t color;         /* +0x18 */
	int    depthTest;     /* +0x28  qboolean */
} debugLine_t;
COD1_ASSERT_SIZE(debugLine_t, 0x2c);
ASSERT_OFF(debugLine_t, end, 0x0c);
ASSERT_OFF(debugLine_t, color, 0x18);
ASSERT_OFF(debugLine_t, depthTest, 0x28);

/* debugString_t (0x80) -- one world-space text label (CoDUO
 * renderer_debug_string_t); text is a fixed 96-byte buffer. */
typedef struct {
	vec3_t origin;        /* +0x00 */
	vec4_t color;         /* +0x0c */
	float  scale;         /* +0x1c */
	char   text[96];      /* +0x20 */
} debugString_t;
COD1_ASSERT_SIZE(debugString_t, 0x80);
ASSERT_OFF(debugString_t, color, 0x0c);
ASSERT_OFF(debugString_t, scale, 0x1c);
ASSERT_OFF(debugString_t, text, 0x20);

/* debugPlume_t (0x28) -- a rising, swaying numeric label (CoDUO
 * renderer_plume_t).  RE_AddPlume fills rgb; RB_AddPlumeStrings owns the alpha
 * lane (color[3]) while the plume is live and fades it over the second half. */
typedef struct {
	vec3_t origin;        /* +0x00 */
	vec4_t color;         /* +0x0c  alpha set live by RB_AddPlumeStrings */
	int    labelValue;    /* +0x1c  the integer shown */
	int    startTime;     /* +0x20  spawn time, ms */
	int    duration;      /* +0x24  lifetime, ms */
} debugPlume_t;
COD1_ASSERT_SIZE(debugPlume_t, 0x28);
ASSERT_OFF(debugPlume_t, color, 0x0c);
ASSERT_OFF(debugPlume_t, labelValue, 0x1c);
ASSERT_OFF(debugPlume_t, startTime, 0x20);
ASSERT_OFF(debugPlume_t, duration, 0x24);

/* debugVertex_t (0x20) -- one immediate-mode vertex.  Texcoords are stored
 * as raw float bit patterns (the texcoord globals are ints); the GL pointers
 * read them as GL_FLOAT.  Two dwords of pad sit before the RGBA8. */
typedef struct {
	float xyz[3];         /* +0x00  position */
	int  s;               /* +0x0c  texcoord S (float bits) */
	int  t;               /* +0x10  texcoord T (float bits) */
	float lightmap[2];   /* +0x14  polyVert_t's lightmap ST (tr_surface.c); unused by the debug path */
	byte color[4];        /* +0x1c  RGBA8 */
} debugVertex_t;
COD1_ASSERT_SIZE(debugVertex_t, 0x20);
ASSERT_OFF(debugVertex_t, s, 0x0c);
ASSERT_OFF(debugVertex_t, color, 0x1c);

/* glyphInfo_t (0x50) -- one entry of the debug font's 256-glyph table (RTCW
 * glyphInfo_t; CoD uses float vertBearing/xSkip, as tr_font.h).  rbDebug_font points at the table. */
typedef struct {
	int   height;         /* +0x00 */
	int   top;            /* +0x04 */
	float vertBearing;    /* +0x08 */
	float horzBearing;    /* +0x0c */
	float xSkip;          /* +0x10  horizontal advance */
	int   imageWidth;     /* +0x14 */
	int   imageHeight;    /* +0x18 */
	float s;              /* +0x1c */
	float t;              /* +0x20 */
	float s2;             /* +0x24 */
	float t2;             /* +0x28 */
	int   glyph;          /* +0x2c  glyph shader/image handle (tr.shaders index) */
	char  shaderName[32]; /* +0x30 */
} glyphInfo_t;
COD1_ASSERT_SIZE(glyphInfo_t, 0x50);
ASSERT_OFF(glyphInfo_t, vertBearing, 0x08);
ASSERT_OFF(glyphInfo_t, xSkip, 0x10);
ASSERT_OFF(glyphInfo_t, imageWidth, 0x14);
ASSERT_OFF(glyphInfo_t, imageHeight, 0x18);
ASSERT_OFF(glyphInfo_t, s, 0x1c);
ASSERT_OFF(glyphInfo_t, glyph, 0x2c);

/* dpvsOccluder_t -- one antiportal occluder cell tested by R_CullBoxDPVS.
 * Only the three fields the cull reads are named (the head is opaque); no size
 * assert since the record continues past +0x24. */
typedef struct {
	dpvsPlane_t *planes;           /* +0x00  authored face planes */
	int          planeCount;       /* +0x04 */
	int          edgeCount;        /* +0x08 */
	void        *edges;            /* +0x0c */
	int          vertexCount;      /* +0x10 */
	vec3_t      *vertices;         /* +0x14 */
	int          cullPlaneLimit;   /* +0x18  INT_MAX, lowered to the first culling plane index */
	int          activePlaneCount; /* +0x1c */
	dpvsPlane_t *activePlanes;     /* +0x20  into dpvs_activePlanes */
} dpvsOccluder_t;
ASSERT_OFF(dpvsOccluder_t, cullPlaneLimit, 0x18);
ASSERT_OFF(dpvsOccluder_t, activePlaneCount, 0x1c);
ASSERT_OFF(dpvsOccluder_t, activePlanes, 0x20);

/* ---- RB_DrawDebugPolys  0x004DFD00 ----  VERIFIED */
void RB_DrawDebugPolys()
{
  debugPolygon_t *polys;
  GLenum          nvDisable;
  int             fogNum;
  int             i;

  if ( !rbDebug_polygonCount )
    return;

  /* select the hunk vertex storage (storage mode 3) */
  if ( glState_currentStorageMode != 3 )
  {
    if ( glConfig_NVVertexArrayRange )
    {
      if ( glConfig_NVVertexArrayRange == 1 )
        glDisableClientState(34077);
      else
        glDisableClientState(34099);
      glVertexPointer(tess_vertexComponentCount, 0x1406u, 0, tess_xyz);
      glNormalPointer(0x1406u, 0, tess_stageNormals);
      glTexCoordPointer(2, 0x1406u, 0, tess_activeTexCoords[0]);
      glColorPointer(4, 0x1401u, 0, tess_stageVertexColors);
    }
    else if ( glConfig_ATIVertexArrayObject )
    {
      glVertexPointer(tess_vertexComponentCount, 0x1406u, 0, tess_xyz);
      glNormalPointer(0x1406u, 12, tess_stageNormals);
      glTexCoordPointer(2, 0x1406u, 0, tess_activeTexCoords[0]);
    }
    glState_currentStorageMode = 3;
  }

  GL_Bind((GLenum *)tr_whiteImage);
  GL_ClientState(1024);
  glVertexPointer(3, 0x1406u, 0, pointer);            /* the debug polygon vertex array */
  glLoadMatrixf(tr_viewParms_world_modelMatrix);
  glMatrixMode(0x1701u);                               /* GL_PROJECTION */
  glLoadMatrixf(tr_viewParms_projectionMatrix);
  glMatrixMode(0x1700u);                               /* GL_MODELVIEW */
  if ( (glState_glStateBits & 0x200000) != 0 )         /* fog on -> turn it off */
  {
    glDisable(0xB60u);                                 /* GL_FOG */
    glState_glStateBits &= ~0x200000u;
  }
  if ( qglLockArraysEXT )
    qglLockArraysEXT(0, rbDebug_polygonVertexCount);
  if ( glState_faceCulling != 2 )
  {
    glDisable(0xB44u);                                 /* GL_CULL_FACE */
    glState_faceCulling = 2;
  }

  for ( i = 0; i < rbDebug_polygonCount; ++i )
  {

    GL_State(357);                                     /* filled, alpha-blended */
    qglColor4fv(((debugPolygon_t *)rbDebug_polygons)[i].color);
    glDrawArrays(9u, ((debugPolygon_t *)rbDebug_polygons)[i].firstVertex, ((debugPolygon_t *)rbDebug_polygons)[i].vertexCount);   /* GL_POLYGON */

    GL_State(4352);                                    /* wireframe overlay */
    glDepthRange(0.0, 0.0);
    qglColor3fv(((debugPolygon_t *)rbDebug_polygons)[i].color);
    glDrawArrays(9u, ((debugPolygon_t *)rbDebug_polygons)[i].firstVertex, ((debugPolygon_t *)rbDebug_polygons)[i].vertexCount);
    glDepthRange(0.0, 1.0);
  }

  rbDebug_polygonCount = 0;
  rbDebug_polygonVertexCount = 0;
  if ( qglUnlockArraysEXT )
    qglUnlockArraysEXT();

  /* restore fog if it was on for this view (inlined R_FogOn) */
  if ( !backEnd_projection2D && (glState_glStateBits & 0x200000) == 0 )
  {
    if ( r_fog->integer )
    {
      fogNum = tr_fogPortalView_registered;
      if ( (backEnd_refdef_rdflags & 8) == 0 )
        fogNum = glfogNum;
      if ( fogNum )
      {
        glEnable(0xB60u);                              /* GL_FOG */
        glState_glStateBits |= 0x200000u;
      }
    }
  }
}

/* ---- RB_DrawDebugLines  0x004DFFB0 ----  VERIFIED */
void __cdecl RB_DrawDebugLines(int lineCount, debugLine_t *lines)
{
  int fogNum;
  int i;

  if ( !lineCount )
    return;

  GL_Bind((GLenum *)tr_whiteImage);
  RB_BeginImmediateMode();
  glLoadMatrixf(tr_viewParms_world_modelMatrix);
  glMatrixMode(0x1701u);                               /* GL_PROJECTION */
  glLoadMatrixf(tr_viewParms_projectionMatrix);
  glMatrixMode(0x1700u);                               /* GL_MODELVIEW */

  for ( i = 0; i < lineCount; ++i )
  {
    debugLine_t *line = &lines[i];

    GL_State(line->depthTest ? 256 : 65792);           /* 65792 disables the depth test */
    mode = 1;                                           /* GL_LINES */
    RB_glColor4fv(line->color);
    RB_glVertex3f(line->start[0], line->start[1], line->start[2]);
    RB_glVertex3f(line->end[0], line->end[1], line->end[2]);
    glDrawArrays(mode, 0, rbDebug_immediateVertexCount);
    rbDebug_immediateVertexCount = 0;
    mode = 0;
  }

  /* inlined RB_EndImmediateMode: drop immediate mode, restore fog + vertex array */
  rbDebug_immediateModeActive = 0;
  if ( !backEnd_projection2D && (glState_glStateBits & 0x200000) == 0 )
  {
    if ( r_fog->integer )
    {
      fogNum = tr_fogPortalView_registered;
      if ( (backEnd_refdef_rdflags & 8) == 0 )
        fogNum = glfogNum;
      if ( fogNum )
      {
        glEnable(0xB60u);                              /* GL_FOG */
        glState_glStateBits |= 0x200000u;
      }
    }
  }
  glVertexPointer(3, 0x1406u, 0, tess_xyz);
}

/* ---- RB_DrawDebugStrings  0x004E0130 ----  [HIGH]
 * Draw each label as camera-facing glyph quads: first a black drop-shadow quad
 * offset by `diagonal`, then the text-colored quad. */
void __cdecl RB_DrawDebugStrings(int stringCount, debugString_t *strings)
{
  byte shadowColor[4];      /* opaque black */
  int  currentGlyphShader;
  int  i;

  shadowColor[0] = 0;
  shadowColor[1] = 0;
  shadowColor[2] = 0;
  shadowColor[3] = 255;
  if ( !stringCount )
    return;

  if ( !rbDebug_font )
  {
    rbDebug_font = Z_MallocInternal(0x5048u);
    RE_RegisterFont((int)"fonts/normalFont", 16, (int)rbDebug_font, (const char *)1);
  }
  glLoadMatrixf(tr_viewParms_world_modelMatrix);
  glMatrixMode(0x1701u);                               /* GL_PROJECTION */
  glLoadMatrixf(tr_viewParms_projectionMatrix);
  glMatrixMode(0x1700u);                               /* GL_MODELVIEW */
  glDepthRange(0.0, 0.0);

  currentGlyphShader = -1;
  for ( i = 0; i < stringCount; ++i )
  {
    debugString_t       *str = &strings[i];
    const unsigned char *text;
    byte   textColor[4];
    rfloat halfScale;
    float  right[3];
    float  up[3];
    float  diagonal[3];
    float  origin[3];

    /* clamp each color component to [0,1] and pack as RGBA8 */
    textColor[0] = (byte)((rfloat)max(0.0f, min(1.0f, str->color[0])) * 255.0f);
    textColor[1] = (byte)((rfloat)max(0.0f, min(1.0f, str->color[1])) * 255.0f);
    textColor[2] = (byte)((rfloat)max(0.0f, min(1.0f, str->color[2])) * 255.0f);
    textColor[3] = (byte)((rfloat)max(0.0f, min(1.0f, str->color[3])) * 255.0f);

    VectorCopy(str->origin, origin);
    right[0] = backEnd_refdef_viewaxis10 * (str->scale * 0.5f);
    right[1] = backEnd_refdef_viewaxis11 * (str->scale * 0.5f);
    right[2] = backEnd_refdef_viewaxis12 * (str->scale * 0.5f);
    up[0] = backEnd_refdef_viewaxis20 * (str->scale * 0.5f);
    up[1] = backEnd_refdef_viewaxis21 * (str->scale * 0.5f);
    up[2] = backEnd_refdef_viewaxis22 * (str->scale * 0.5f);
    VectorAdd(up, right, diagonal);
    VectorScale(diagonal, -2.0f, diagonal);

    for ( text = (const unsigned char *)str->text; *text; ++text )
    {
      const glyphInfo_t *glyph = &((const glyphInfo_t *)rbDebug_font)[*text];
      float  widthOffset[3];
      float  heightOffset[3];
      float  glyphOrigin[3];
      rfloat advance;

      if ( currentGlyphShader != glyph->glyph )
      {
        int storage;

        currentGlyphShader = glyph->glyph;
        backEnd_currentEntity = (int)&backEnd_debugEntity;
        storage = storageClass;
        if ( storageClass != glState_currentStorageMode )
        {
          if ( glConfig_NVVertexArrayRange )
          {
            RB_SelectStorageNV(storageClass);
          }
          else if ( glConfig_ATIVertexArrayObject && storageClass == 3 )
          {
            glVertexPointer(tess_vertexComponentCount, 0x1406u, 0, tess_xyz);
            glNormalPointer(0x1406u, 12, tess_stageNormals);
            glTexCoordPointer(2, 0x1406u, 0, tess_activeTexCoords[0]);
          }
          glState_currentStorageMode = storage;
        }
        RB_BeginSurface((void *)tr_shaders[currentGlyphShader], 3);   /* 3 = vertex component count */
      }

      widthOffset[0] = (rfloat)glyph->imageWidth * right[0];
      widthOffset[1] = (rfloat)glyph->imageWidth * right[1];
      widthOffset[2] = (rfloat)glyph->imageWidth * right[2];
      heightOffset[0] = (rfloat)glyph->imageHeight * up[0];
      heightOffset[1] = (rfloat)glyph->imageHeight * up[1];
      heightOffset[2] = (rfloat)glyph->imageHeight * up[2];

      glyphOrigin[0] = origin[0] - (rfloat)glyph->imageWidth * right[0];
      glyphOrigin[1] = origin[1] - (rfloat)glyph->imageWidth * right[1];
      glyphOrigin[2] = origin[2] - (rfloat)glyph->imageWidth * right[2];
      glyphOrigin[0] = ((rfloat)glyph->vertBearing + glyph->vertBearing - glyph->imageHeight) * up[0] + glyphOrigin[0];
      glyphOrigin[1] = ((rfloat)glyph->vertBearing + glyph->vertBearing - glyph->imageHeight) * up[1] + glyphOrigin[1];
      glyphOrigin[2] = ((rfloat)glyph->vertBearing + glyph->vertBearing - glyph->imageHeight) * up[2] + glyphOrigin[2];

      /* drop shadow quad (offset by diagonal), then the text quad */
      VectorAdd(glyphOrigin, diagonal, glyphOrigin);
      RB_AddQuadStampExt(glyphOrigin, widthOffset, heightOffset, (int *)shadowColor,
                         glyph->s, glyph->t, glyph->s2, glyph->t2);
      VectorSubtract(glyphOrigin, diagonal, glyphOrigin);
      RB_AddQuadStampExt(glyphOrigin, widthOffset, heightOffset, (int *)textColor,
                         glyph->s, glyph->t, glyph->s2, glyph->t2);

      advance = glyph->xSkip * -2.0f;
      VectorMA(origin, advance, right, origin);
    }
  }

  RB_EndSurface();
  glDepthRange(0.0, 1.0);
}

/* ---- RB_AddPlumeStrings  0x004E0660 ----  VERIFIED */
#define rbDebug_plumeArray ((debugPlume_t *)rbDebug_plumes)
void RB_AddPlumeStrings()
{
  int    i;
  int    elapsed;
  rfloat sway;
  float  labelOrigin[3];

  for ( i = 0; i < rbDebug_plumeCount; )
  {
    elapsed = tr_refdef_time - rbDebug_plumeArray[i].startTime;
    if ( elapsed < 0 || elapsed > rbDebug_plumeArray[i].duration )
    {
      /* expired -- swap the last plume into this slot and re-test it */
      --rbDebug_plumeCount;
      rbDebug_plumeArray[i] = rbDebug_plumeArray[rbDebug_plumeCount];
      continue;
    }

    rbDebug_plumeArray[i].color[3] = 1.0f;
    if ( 2 * elapsed > rbDebug_plumeArray[i].duration )       /* fade over the second half of life */
      rbDebug_plumeArray[i].color[3] = 2.0f - ((rfloat)elapsed + (rfloat)elapsed) / rbDebug_plumeArray[i].duration;

    sway = FCAST(sin((rfloat)elapsed * 0.012566371f + i)) * 4.0f;   /* flt_56913C = 0x3C4DE32E */
    labelOrigin[0] = tr_refdef_viewaxis10 * sway + rbDebug_plumeArray[i].origin[0];
    labelOrigin[1] = tr_refdef_viewaxis11 * sway + rbDebug_plumeArray[i].origin[1];
    labelOrigin[2] = tr_refdef_viewaxis12 * sway + rbDebug_plumeArray[i].origin[2] + (rfloat)elapsed * 0.064000003f;   /* flt_569138 = 0x3D83126F */
    R_AddDebugString((int *)labelOrigin, (int *)rbDebug_plumeArray[i].color, 0x3F000000,
                     va("%i", rbDebug_plumeArray[i].labelValue));
    ++i;
  }
}
#undef rbDebug_plumeArray

/* ---- RB_DrawDebug  0x004E07A0 ----  VERIFIED */
void __cdecl RB_DrawDebug(void)
{
  RB_AddPlumeStrings();
  RB_DrawDebugPolys();
  RB_DrawDebugLines(rbDebug_lineCount, (debugLine_t *)rbDebug_lines);
  RB_DrawDebugLines(rbDebug_locatedLineCount, (debugLine_t *)rbDebug_locatedLines);
  rbDebug_lineCount = 0;
  RB_DrawDebugStrings(rbDebug_stringCount, (debugString_t *)rbDebug_strings);
  RB_DrawDebugStrings(rbDebug_locatedStringCount, (debugString_t *)rbDebug_locatedStrings);
  rbDebug_stringCount = 0;
}

/* ---- R_AddDebugPolygon  0x004E0810 ----  VERIFIED */
void __cdecl R_AddDebugPolygon(unsigned int *color, int vertexCount, const void *vertices)
{
  debugPolygon_t *poly;

  if ( vertexCount + rbDebug_polygonVertexCount <= rbDebug_polygonVertexCapacity )
  {

    if ( rbDebug_polygonCount + 1 <= rbDebug_polygonCapacity )
    {
      if ( !rbDebug_polygons || !pointer )
      {
        rbDebug_polygons = Z_MallocInternal(24 * rbDebug_polygonCapacity);
        pointer = Z_MallocInternal(12 * rbDebug_polygonVertexCapacity);

      }

      ((debugPolygon_t *)rbDebug_polygons)[rbDebug_polygonCount].firstVertex = rbDebug_polygonVertexCount;
      ((debugPolygon_t *)rbDebug_polygons)[rbDebug_polygonCount].vertexCount = vertexCount;
      ((unsigned int *)((debugPolygon_t *)rbDebug_polygons)[rbDebug_polygonCount].color)[0] = color[0];
      ((unsigned int *)((debugPolygon_t *)rbDebug_polygons)[rbDebug_polygonCount].color)[1] = color[1];
      ((unsigned int *)((debugPolygon_t *)rbDebug_polygons)[rbDebug_polygonCount].color)[2] = color[2];
      ((unsigned int *)((debugPolygon_t *)rbDebug_polygons)[rbDebug_polygonCount].color)[3] = color[3];

      ++rbDebug_polygonCount;
      memcpy(&((vec3_t *)pointer)[rbDebug_polygonVertexCount], vertices, 12 * vertexCount);
      rbDebug_polygonVertexCount += vertexCount;
    }
  }

}

/* ---- R_AddDebugLine  0x004E0940 ----  VERIFIED */
int __cdecl R_AddDebugLine(unsigned int *start, unsigned int *end, unsigned int *color)
{
  debugLine_t *line;

  if ( rbDebug_lineCount + 1 > rbDebug_lineCapacity )
    return rbDebug_lineCount;
  if ( !rbDebug_lines )
    rbDebug_lines = Z_MallocInternal(44 * rbDebug_lineCapacity);
  line = &((debugLine_t *)rbDebug_lines)[rbDebug_lineCount];
  VectorCopy( (float *)start, line->start );
  VectorCopy( (float *)end, line->end );
  line->color[0] = ((float *)color)[0];
  line->color[1] = ((float *)color)[1];
  line->color[2] = ((float *)color)[2];
  line->color[3] = ((float *)color)[3];
  line->depthTest = 0;
  ++rbDebug_lineCount;
  return (int)line;   /* dead EAX: the line record */
}

static const int r_debugBoxEdges[24] =
{
	0,1,  0,2,  0,4,  1,3,  1,5,  2,3,
	2,6,  3,7,  4,5,  4,6,  5,7,  6,7
};

/* ---- R_AddDebugBox  0x004E09C0 ----  VERIFIED */
int __cdecl R_AddDebugBox(float *mins, float *maxs, unsigned int *color)
{
  int   result;
  int   corner;
  int   axis;
  unsigned int i;
  float corners[8][3]; // [esp+10h] [ebp-60h] BYREF

  /* maxs = maxs, mins = mins; bit N of the corner index selects maxs on axis N */
  for ( corner = 0; corner < 8; ++corner )
  {
    for ( axis = 0; axis < 3; axis++ )
      corners[corner][axis] = (corner & (1 << axis)) ? maxs[axis] : mins[axis];
  }
  for ( i = 0; i < 24; i += 2 )
    result = R_AddDebugLine((unsigned int *)corners[r_debugBoxEdges[i]],
                            (unsigned int *)corners[r_debugBoxEdges[i + 1]], color);
  return result;
}

/* ---- R_AddDebugString  0x004E0A40 ----  VERIFIED */
int __cdecl R_AddDebugString(int *origin, int *color, int scaleBits, char *text)
{
  int            count;
  int            result;
  debugString_t *str;

  count = rbDebug_stringCount;
  result = rbDebug_stringCount + 1;
  if ( rbDebug_stringCount + 1 <= rbDebug_stringCapacity )
  {
    if ( !rbDebug_strings )
    {
      rbDebug_strings = Z_MallocInternal(rbDebug_stringCapacity << 7);   /* capacity * 128 */
      count = rbDebug_stringCount;
    }
    str = &((debugString_t *)rbDebug_strings)[count];
    memcpy(str->origin, origin, sizeof(str->origin));
    memcpy(str->color,  color, sizeof(str->color));
    str->scale = *(float *)&scaleBits;
    strncpy(str->text, text, 0x5Fu);
    str->text[0x5F] = 0;
    return ++rbDebug_stringCount;
  }
  return result;
}

/* ---- R_AddScaledDebugString  0x004E0AD0 ----  VERIFIED */
int __cdecl R_AddScaledDebugString(int origin, int *color, char *text)
{
  float dist;
  float  scale;
  float  dir[3]; // [esp+Ch] [ebp-Ch] BYREF -- retail vec3_t

  /* scale grows for points nearly collinear with the forward axis */
  dir[0] = *(float *)origin - tr_viewParms_originX;
  dir[1] = *(float *)(origin + 4) - tr_viewParms_originY;
  dir[2] = *(float *)(origin + 8) - tr_viewParms_originZ;
  dist = VectorNormalize(dir);
  scale = (*(float *)&tr_viewParms_axis02 * dir[2]
         + *(float *)&tr_viewParms_axis01 * dir[1]
         + *(float *)&tr_viewParms_axis00 * dir[0] - 0.995f) * dist;   /* flt_569168 = 0x3F7EB852 */
  if ( scale < 1.0f )
    scale = 1.0f;
  return R_AddDebugString((int *)origin, color, *(int *)&scale, text);
}

/* ---- R_InitDebug  0x004E0B70 ----  VERIFIED */
void R_InitDebug()
{


  

  memset(&rendererDebugState, 0, sizeof(rendererDebugState));

  rendererDebugState.polygonVertexCapacity = 4096;
  rendererDebugState.stringCapacity = 4096;
  rendererDebugState.plumeCapacity = 4096;
  rendererDebugState.polygonCapacity = 512;
  rendererDebugState.lineCapacity = 0x4000;
  mode = 0;
  rendererDebugState.immediateLineWidth = 0.0f;
  rendererDebugState.immediateTexCoordS = 0.0f;
  rendererDebugState.immediateTexCoordT = 0.0f;
  rendererDebugState.immediateColorR = 1;
  rendererDebugState.immediateColorG = 1;
  rendererDebugState.immediateColorB = 1;
  rendererDebugState.immediateColorA = 1;
  rendererDebugState.immediateVertexCapacity = 0x2000;

}

/* ---- R_ShutdownDebug  0x004E0C00 ----  VERIFIED */
void R_ShutdownDebug()
{
  if ( rbDebug_polygons )
  {
    free(rbDebug_polygons);
    rbDebug_polygons = 0;
  }
  if ( pointer )
  {
    free(pointer);
    pointer = 0;
  }
  if ( rbDebug_strings )
  {
    free(rbDebug_strings);
    rbDebug_strings = 0;
  }
  if ( rbDebug_lines )
  {
    free(rbDebug_lines);
    rbDebug_lines = 0;
  }
  if ( rbDebug_immediateVertices )
  {
    free(rbDebug_immediateVertices);
    rbDebug_immediateVertices = 0;
  }
  if ( rbDebug_plumes )
  {
    free(rbDebug_plumes);
    rbDebug_plumes = 0;
  }
  if ( rbDebug_font )
  {
    free(rbDebug_font);
    rbDebug_font = 0;
  }
}

/* ---- RE_LocateDebugStrings  0x004E0CB0 ----  VERIFIED */
int __cdecl RE_LocateDebugStrings(int strings, int count)
{
  int result;

  result = strings;
  rbDebug_locatedStrings = strings;
  rbDebug_locatedStringCount = count;
  return result;
}

/* ---- RE_LocateDebugLines  0x004E0CD0 ----  VERIFIED */
int __cdecl RE_LocateDebugLines(int lines, int count)
{
  int result;

  result = lines;
  rbDebug_locatedLines = lines;
  rbDebug_locatedLineCount = count;
  return result;
}

/* ---- RE_AddPlume  0x004E0CF0 ----  VERIFIED */
void __cdecl RE_AddPlume(int *origin, int labelValue, unsigned int *color, int duration)
{
  if ( !rbDebug_plumes )
  {
    rbDebug_plumes = Z_MallocInternal(40 * rbDebug_plumeCapacity);
    rbDebug_plumeCount = 0;
  }
  if ( rbDebug_plumeCount != rbDebug_plumeCapacity )
  {
    ((unsigned int *)((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].origin)[0] = origin[0];
    ((unsigned int *)((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].origin)[1] = origin[1];
    ((unsigned int *)((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].origin)[2] = origin[2];
    ((unsigned int *)((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].color)[0] = color[0];
    ((unsigned int *)((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].color)[1] = color[1];
    ((unsigned int *)((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].color)[2] = color[2];
    ((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].labelValue = labelValue;
    ((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].startTime = tr_refdef_time;
    ((debugPlume_t *)rbDebug_plumes)[rbDebug_plumeCount].duration = duration;
    ++rbDebug_plumeCount;
  }
}

/* ---- RB_BeginImmediateMode  0x004E0E00 ----  VERIFIED */
void RB_BeginImmediateMode()
{
  debugVertex_t *verts;
  GLenum         nvDisable;

  if ( !rbDebug_immediateVertices )
    rbDebug_immediateVertices = Z_MallocInternal(32 * rbDebug_immediateVertexCapacity);
  rbDebug_immediateColorR = -1;
  rbDebug_immediateColorG = -1;
  rbDebug_immediateColorB = -1;
  rbDebug_immediateColorA = -1;
  mode = 0;
  rendererDebugState.immediateLineWidth = 0.0f;
  rendererDebugState.immediateTexCoordS = 0.0f;
  rendererDebugState.immediateTexCoordT = 0.0f;
  if ( glState_currentStorageMode != 3 )
  {
    if ( glConfig_NVVertexArrayRange )
    {
      if ( glConfig_NVVertexArrayRange == 1 )
        glDisableClientState(34077);
      else
        glDisableClientState(34099);
      glVertexPointer(tess_vertexComponentCount, 0x1406u, 0, tess_xyz);
      glNormalPointer(0x1406u, 0, tess_stageNormals);
      glTexCoordPointer(2, 0x1406u, 0, tess_activeTexCoords[0]);
      glColorPointer(4, 0x1401u, 0, tess_stageVertexColors);
    }
    else if ( glConfig_ATIVertexArrayObject )
    {
      glVertexPointer(tess_vertexComponentCount, 0x1406u, 0, tess_xyz);
      glNormalPointer(0x1406u, 12, tess_stageNormals);
      glTexCoordPointer(2, 0x1406u, 0, tess_activeTexCoords[0]);
    }
    glState_currentStorageMode = 3;
  }
  RB_EndMultitexture();
  GL_ClientState(1281);
  if ( (glState_glStateBits & 0x200000) != 0 )
  {
    glDisable(0xB60u);                                 /* GL_FOG */
    glState_glStateBits &= ~0x200000u;
  }
  glTexCoordPointer(2, 0x1406u, 32, &((debugVertex_t *)rbDebug_immediateVertices)->s);
  glColorPointer(4, 0x1401u, 32, ((debugVertex_t *)rbDebug_immediateVertices)->color);
  qglNormal3f(0.0f, 0.0f, 1.0f);
  glVertexPointer(3, 0x1406u, 32, rbDebug_immediateVertices);
  rbDebug_immediateModeActive = 1;
}

/* ---- RB_EndImmediateMode  0x004E0FD0 ----  VERIFIED */
void RB_EndImmediateMode()
{
  rbDebug_immediateModeActive = 0;
  R_FogOn();
  glVertexPointer(3, 0x1406u, 0, tess_xyz);
}

/* ---- RB_glBegin  0x004E1000 ----  VERIFIED */
GLenum __cdecl RB_glBegin(GLenum result)
{
  mode = result;
  return result;
}

/* ---- RB_glEnd  0x004E1010 ----  VERIFIED */
void RB_glEnd()
{
  glDrawArrays(mode, 0, rbDebug_immediateVertexCount);
  rbDebug_immediateVertexCount = 0;
  mode = 0;
}

/* ---- RB_glVertex2i  0x004E1040 ----  VERIFIED */
void __cdecl RB_glVertex2i(int x, int y)
{
  RB_glVertex3f((float)x, (float)y, 0.0f);
}

/* ---- RB_glVertex2f  0x004E1060 ----  VERIFIED */
void __cdecl RB_glVertex2f(float x, float y)
{
  RB_glVertex3f(x, y, 0.0f);
}

/* ---- RB_glVertex2fv  0x004E1080 ----  VERIFIED */
void __cdecl RB_glVertex2fv(const float *v)
{
  RB_glVertex3f(v[0], v[1], 0.0f);
}

/* ---- RB_glVertex3f  0x004E10A0 ----  VERIFIED */
void __cdecl RB_glVertex3f( float x, float y, float z )
{
	debugVertex_t *v;

	if ( rbDebug_immediateVertexCount < rbDebug_immediateVertexCapacity )
	{
		v = &((debugVertex_t *)rbDebug_immediateVertices)[rbDebug_immediateVertexCount];
		v->color[0] = rbDebug_immediateColorR;
		v->color[1] = rbDebug_immediateColorG;
		v->color[2] = rbDebug_immediateColorB;
		v->color[3] = rbDebug_immediateColorA;
		v->s = rbDebug_immediateTexCoordS;
		v->t = rbDebug_immediateTexCoordT;
		v->xyz[0] = x;
		v->xyz[1] = y;
		v->xyz[2] = z;
		++rbDebug_immediateVertexCount;
	}
}

/* ---- RB_glVertex3fv  0x004E1110 ----  VERIFIED */
void __cdecl RB_glVertex3fv(const float *v)
{
  RB_glVertex3f(v[0], v[1], v[2]);
}

/* ---- RB_glTexCoord2f  0x004E1130 ----  VERIFIED */
void __cdecl RB_glTexCoord2f(float s, float t)
{
  rbDebug_immediateTexCoordS = *(int *)&s;
  rbDebug_immediateTexCoordT = *(int *)&t;
}

/* ---- RB_glTexCoord2fv  0x004E1150 ----  VERIFIED */
int *__cdecl RB_glTexCoord2fv(int *result)
{
  rbDebug_immediateTexCoordS = *result;
  rbDebug_immediateTexCoordT = result[1];
  return result;
}

/* ---- RB_glColor3f  0x004E1170 ----  VERIFIED */
unsigned __int64 __cdecl RB_glColor3f(float r, float g, float b)
{
  unsigned __int64 result; // rax

  rbDebug_immediateColorR = (unsigned __int64)((rfloat)r * 255.0f);
  rbDebug_immediateColorG = (unsigned __int64)((rfloat)g * 255.0f);
  result = (unsigned __int64)((rfloat)b * 255.0f);
  rbDebug_immediateColorB = result;
  rbDebug_immediateColorA = -1;
  return result;
}

/* ---- RB_glColor4f  0x004E11C0 ----  VERIFIED */
unsigned __int64 __cdecl RB_glColor4f(float r, float g, float b, float a)
{
  unsigned __int64 result; // rax

  rbDebug_immediateColorR = (unsigned __int64)((rfloat)r * 255.0f);
  rbDebug_immediateColorG = (unsigned __int64)((rfloat)g * 255.0f);
  rbDebug_immediateColorB = (unsigned __int64)((rfloat)b * 255.0f);
  result = (unsigned __int64)((rfloat)a * 255.0f);
  rbDebug_immediateColorA = result;
  return result;
}

/* ---- RB_glColor3fv  0x004E1220 ----  VERIFIED */
unsigned __int64 __cdecl RB_glColor3fv(const float *v)
{
  return RB_glColor3f(v[0], v[1], v[2]);
}

/* ---- RB_glColor4fv  0x004E1260 ----  VERIFIED */
unsigned __int64 __cdecl RB_glColor4fv(const float *v)
{
  return RB_glColor4f(v[0], v[1], v[2], v[3]);
}

/* ---- RB_glLineWidth  0x004E12B0 ----  VERIFIED */
void __cdecl RB_glLineWidth(float width)
{
  if ( *(float *)&rbDebug_immediateLineWidth != width )
  {
    *(float *)&rbDebug_immediateLineWidth = width;
    qglLineWidth(width);
  }
}

/* ---- R_BoxBehindPlane  0x004E12E0 ----  VERIFIED */
int __cdecl R_BoxBehindPlane(dpvsPlane_t *plane, int box)
{
  rfloat dot;

  /* sideOffsets pick the box corner furthest along +normal; if even that is
   * behind the plane, the whole box is behind. */
  dot = *(float *)(box + plane->sideOffsets[2]) * plane->normal[2]
      + *(float *)(box + plane->sideOffsets[1]) * plane->normal[1]
      + *(float *)(box + plane->sideOffsets[0]) * plane->normal[0];
  return dot < plane->distance;
}

/* ---- R_BoxInFrontOfPlane  0x004E1320 ----  VERIFIED */
int __cdecl R_BoxInFrontOfPlane(dpvsPlane_t *plane, int box)
{
    float z = *(float *)(box - plane->sideOffsets[2] + 28);
    float y = *(float *)(box - plane->sideOffsets[1] + 20);
    float x = *(float *)(box - plane->sideOffsets[0] + 12);
    return z * plane->normal[2] + y * plane->normal[1] + x * plane->normal[0] > plane->distance;
}

/* ---- R_CullBoxDPVS  0x004E1360 ----  VERIFIED */
int __cdecl R_CullBoxDPVS(int numPlanes, int box, dpvsPlane_t *planes, int planeIndex)
{
  dpvsPlane_t     *nearPlane;
  dpvsPlane_t     *farPlane;
  dpvsOccluder_t **occluders;
  rfloat           dot;
  float x,y,z;
  int              i;
  int              j;

  
  if ( planeIndex < dpvs_cullPlaneLimit )
  {
    dpvs_cullPlaneLimit = 0x7FFFFFFF;
    for ( i = 0; i < numPlanes; ++i )
    {
      dpvsPlane_t *p = &planes[i];
      dot = *(float *)(box + p->sideOffsets[0]) * p->normal[0]
          + *(float *)(box + p->sideOffsets[1]) * p->normal[1]
          + *(float *)(box + p->sideOffsets[2]) * p->normal[2];
      if ( dot < p->distance )
        return 1;
    }
  }

  
  nearPlane = (dpvsPlane_t *)dpvs_activeNearPlane;
  z = *(float *)(box + nearPlane->sideOffsets[2]);
  y = *(float *)(box + nearPlane->sideOffsets[1]);
  x = *(float *)(box + nearPlane->sideOffsets[0]);
  dot = z * nearPlane->normal[2] + y * nearPlane->normal[1] + x * nearPlane->normal[0];
  if ( dot < nearPlane->distance )
    return 1;
  farPlane = (dpvsPlane_t *)dpvs_activeFarPlane;
  if ( farPlane )
  {
    z = *(float *)(box + farPlane->sideOffsets[2]);
  y = *(float *)(box + farPlane->sideOffsets[1]);
  x = *(float *)(box + farPlane->sideOffsets[0]);
  dot = z * farPlane->normal[2] + y * farPlane->normal[1] + x * farPlane->normal[0];
    if ( dot < farPlane->distance )
      return 1;
  }

  
  for ( i = 0; i < dpvs_occluderCount; ++i )
  {
    dpvsOccluder_t *occluder;
    occluder = ((dpvsOccluder_t **)dpvs_occluders)[i];

    if ( planeIndex >= occluder->cullPlaneLimit )
      continue;
    occluder->cullPlaneLimit = 0x7FFFFFFF;
    occluder = ((dpvsOccluder_t **)dpvs_occluders)[i];
    for ( j = 0; ; ++j )
    {
      dpvsPlane_t *p;
      if ( j >= occluder->activePlaneCount )
        return 1;
      p = &occluder->activePlanes[j];
      dot = *(float *)(box + p->sideOffsets[2]) * p->normal[2]
          + *(float *)(box + p->sideOffsets[1]) * p->normal[1]
          + *(float *)(box + p->sideOffsets[0]) * p->normal[0];
      

      if ( !(dot < p->distance) )
        break;                                           
    }
                                          
  }
  return 0;
}

/* ---- R_CullBoxDPVSStrict  0x004E14C0 ----  VERIFIED */
int __cdecl R_CullBoxDPVSStrict(int numPlanes, dpvsPlane_t *planes, int box, int planeIndex)
{
  dpvsOccluder_t **occluders;
  rfloat           dot;
  float x,y,z;
  int              i;
  int              j;

  

  if ( planeIndex < dpvs_cullPlaneLimit )
  {
    for ( i = 0; i < numPlanes; ++i )
    {
      dpvsPlane_t *p = &planes[i];
      y = *(float *)(box - p->sideOffsets[1] + 20);
      x = *(float *)(box - p->sideOffsets[0] + 12);
      z = *(float *)(box - p->sideOffsets[2] + 28);
      dot = y * p->normal[1] + x * p->normal[0] + z * p->normal[2];
      
      if ( !(dot > p->distance) )
        return 1;
    }
    dpvs_cullPlaneLimit = planeIndex + 1;
  }

  for ( i = 0; i < dpvs_occluderCount; ++i )
  {
    dpvsOccluder_t *occluder;
    occluder = ((dpvsOccluder_t **)dpvs_occluders)[i];

    if ( planeIndex >= occluder->cullPlaneLimit )
      continue;
    for ( j = 0; ; ++j )
    {
      dpvsPlane_t *p;
      if ( j >= occluder->activePlaneCount )
        return 1;
      p = &occluder->activePlanes[j];
      z = *(float *)(box - p->sideOffsets[2] + 28);
      y = *(float *)(box - p->sideOffsets[1] + 20);
      x = *(float *)(box - p->sideOffsets[0] + 12);
      dot = z * p->normal[2] + y * p->normal[1] + x * p->normal[0];
      if ( dot > p->distance )
        break;                                           
    }
                                          
    occluder->cullPlaneLimit = planeIndex + 1;
  }
  return 0;
}

/* ---- R_SetPlaneSidesDPVS_dup  0x004E15D0 ----  VERIFIED */
dpvsPlane_t *__cdecl R_SetPlaneSidesDPVS_dup(dpvsPlane_t *plane)
{
  /* signed-int tests on the raw float bits classify each normal component's
   * sign, choosing the box-corner byte offsets; also bevels dist inward */
  plane->distance -= 0.001f;   /* flt_568E6C = 0x3A83126F */
  plane->sideOffsets[0] = *(int *)&plane->normal[0] > 0 ? 12 : 0;
  plane->sideOffsets[1] = *(int *)&plane->normal[1] > 0 ? 16 : 4;
  plane->sideOffsets[2] = *(int *)&plane->normal[2] > 0 ? 20 : 8;
  return plane;
}

unsigned int colorDebugPortal[51] =   /* 0x00540568 */
{
	0x00000000, 0x3F800000, 0x3F800000, 0x3E800000,      /* 0x00540568  0      1      1      0.25  */
	0x00000000, 0x00000001, 0x00000000, 0x00000002,      /* 0x00540578  r_debugBoxEdges (cube edge pairs) */
	0x00000000, 0x00000004, 0x00000001, 0x00000003,      /* 0x00540588  r_debugBoxEdges (cube edge pairs) */
	0x00000001, 0x00000005, 0x00000002, 0x00000003,      /* 0x00540598  r_debugBoxEdges (cube edge pairs) */
	0x00000002, 0x00000006, 0x00000003, 0x00000007,      /* 0x005405A8  r_debugBoxEdges (cube edge pairs) */
	0x00000004, 0x00000005, 0x00000004, 0x00000006,      /* 0x005405B8  r_debugBoxEdges (cube edge pairs) */
	0x00000005, 0x00000007, 0x00000006, 0x00000007,      /* 0x005405C8  r_debugBoxEdges (cube edge pairs) */
	0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF,      /* 0x005405D8  s_consoleColorTable (packed RGBA bytes) */
	0xFFFF0000, 0xFFFFFF00, 0xFFFF00FF, 0xFFFFFFFF,      /* 0x005405E8  s_consoleColorTable (packed RGBA bytes) */
	0x3E991687, 0x3F1645A2, 0x3DE978D5, 0x3F800000,      /* 0x005405F8  0.299  0.587  0.114  1     <- luminance weights (R_LoadLights) */
	0x3F800000, 0x3F800000, 0x3F800000, 0x00000000,      /* 0x00540608  1      1      1      0 */
	0x00000000, 0x00000000, 0x3F800000, 0x00000000,      /* 0x00540618  0      0      1      0 */
	0x00000000, 0x00000000, 0x3F800000,                  /* 0x00540628  0      0      1 */
};
/* ---- colorDebugRed   0x00541830   retail .rdata, read with get_bytes ---- */
unsigned int colorDebugRed[9] =   /* 0x00541830 */
{
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541830  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541840  0      1      0      1     */
	0x00000000,                                          /* 0x00541850  0     */
};
/* ---- colorDebugGreen   0x00541840   retail .rdata, read with get_bytes ---- */
unsigned int colorDebugGreen[5] =   /* 0x00541840 */
{
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541840  0      1      0      1     */
	0x00000000,                                          /* 0x00541850  0     */
};
/* ---- colorDebugBlue   0x00541860   retail .rdata, read with get_bytes ---- */
unsigned int colorDebugBlue[92] =   /* 0x00541860 */
{
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541860  0      0      1      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541870  1      1      0      1     */
	0x3F400000, 0x3F400000, 0x00000000, 0x3F800000,      /* 0x00541880  0.75   0.75   0      1     */
	0x3F000000, 0x3F000000, 0x00000000, 0x3F800000,      /* 0x00541890  0.5    0.5    0      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005418A0  1      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418B0  0      1      1      1     */
	0x00000000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x005418C0  0      0.75   0.75   1     */
	0x00000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x005418D0  0      0.5    0.5    1     */
	0x00000000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x005418E0  0      0.25   0.25   1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418F0  1      1      1      1     */
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- colorDebugYellow   0x00541870   retail .rdata, read with get_bytes ---- */
unsigned int colorDebugYellow[88] =   /* 0x00541870 */
{
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541870  1      1      0      1     */
	0x3F400000, 0x3F400000, 0x00000000, 0x3F800000,      /* 0x00541880  0.75   0.75   0      1     */
	0x3F000000, 0x3F000000, 0x00000000, 0x3F800000,      /* 0x00541890  0.5    0.5    0      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005418A0  1      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418B0  0      1      1      1     */
	0x00000000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x005418C0  0      0.75   0.75   1     */
	0x00000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x005418D0  0      0.5    0.5    1     */
	0x00000000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x005418E0  0      0.25   0.25   1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418F0  1      1      1      1     */
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- cullGroupDebugColor   0x00541880   retail .rdata, read with get_bytes ---- */
unsigned int cullGroupDebugColor[84] =
{
	0x3F400000, 0x3F400000, 0x00000000, 0x3F800000,      /* 0x00541880  0.75   0.75   0      1     */
	0x3F000000, 0x3F000000, 0x00000000, 0x3F800000,      /* 0x00541890  0.5    0.5    0      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005418A0  1      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418B0  0      1      1      1     */
	0x00000000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x005418C0  0      0.75   0.75   1     */
	0x00000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x005418D0  0      0.5    0.5    1     */
	0x00000000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x005418E0  0      0.25   0.25   1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418F0  1      1      1      1     */
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- modelBoundsDebugColor   0x005418B0   retail .rdata, read with get_bytes ---- */
unsigned int modelBoundsDebugColor[72] =
{
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418B0  0      1      1      1     */
	0x00000000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x005418C0  0      0.75   0.75   1     */
	0x00000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x005418D0  0      0.5    0.5    1     */
	0x00000000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x005418E0  0      0.25   0.25   1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418F0  1      1      1      1     */
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- acceptedBevelDebugColor   0x005418C0   retail .rdata, read with get_bytes ---- */
unsigned int acceptedBevelDebugColor[68] =
{
	0x00000000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x005418C0  0      0.75   0.75   1     */
	0x00000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x005418D0  0      0.5    0.5    1     */
	0x00000000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x005418E0  0      0.25   0.25   1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418F0  1      1      1      1     */
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- rejectedBevelDebugColor   0x005418D0   retail .rdata, read with get_bytes ---- */
unsigned int rejectedBevelDebugColor[64] =
{
	0x00000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x005418D0  0      0.5    0.5    1     */
	0x00000000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x005418E0  0      0.25   0.25   1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418F0  1      1      1      1     */
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- colorDebugWhite   0x005418F0   retail .rdata, read with get_bytes ---- */
unsigned int colorDebugWhite[56] =   /* 0x005418F0 */
{
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005418F0  1      1      1      1     */
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- staticModelDebugColor   0x00541900   retail .rdata, read with get_bytes ---- */
unsigned int staticModelDebugColor[52] =
{
	0x3F400000, 0x3F400000, 0x3F400000, 0x3F800000,      /* 0x00541900  0.75   0.75   0.75   1     */
	0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,      /* 0x00541910  0.5    0.5    0.5    1     */
	0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,      /* 0x00541920  0.25   0.25   0.25   1     */
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};
/* ---- clippedLeafDebugColor   0x00541930   retail .rdata, read with get_bytes ---- */
unsigned int clippedLeafDebugColor[40] =
{
	0x3F800000, 0x3F333333, 0x00000000, 0x3F800000,      /* 0x00541930  1      0.7    0      1     */
	0x3F400000, 0x3F066666, 0x00000000, 0x3F800000,      /* 0x00541940  0.75   0.525  0      1     */
	0x00000000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541950  0      0      0      1     */
	0x3F800000, 0x00000000, 0x00000000, 0x3F800000,      /* 0x00541960  1      0      0      1     */
	0x00000000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541970  0      1      0      1     */
	0x3F800000, 0x3F800000, 0x00000000, 0x3F800000,      /* 0x00541980  1      1      0      1     */
	0x00000000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x00541990  0      0      1      1     */
	0x00000000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419A0  0      1      1      1     */
	0x3F800000, 0x00000000, 0x3F800000, 0x3F800000,      /* 0x005419B0  1      0      1      1     */
	0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,      /* 0x005419C0  1      1      1      1     */
};

