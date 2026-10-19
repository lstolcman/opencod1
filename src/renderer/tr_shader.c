/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_shaderregistry.h"
#include "tr_records.h"

static unsigned char tr_shaderParseScratch[0x79E8];

#define shaderParseTexMods        (*(unsigned char *)(tr_shaderParseScratch + 0x0000))
#define shaderParseTexMods_0_1        (*(unsigned char *)(tr_shaderParseScratch + 0x0110))
#define shaderParseName       ((unsigned char *)(tr_shaderParseScratch + 0x4400))
#define shaderParseName_63       (*(unsigned char *)(tr_shaderParseScratch + 0x443F))
#define shaderParseLightmapIndex      (*(int *)(tr_shaderParseScratch + 0x4440))
#define shaderParseFlags      (*(int *)(tr_shaderParseScratch + 0x444C))
#define shaderParseSurfaceFlags      (*(int *)(tr_shaderParseScratch + 0x4450))
#define shaderParseLightingFlags      (*(int *)(tr_shaderParseScratch + 0x4454))
#define shaderParseSort      (*(int *)(tr_shaderParseScratch + 0x4458))
#define shaderParseSurfaceParmFlags      (*(int *)(tr_shaderParseScratch + 0x445C))
#define shaderParseSkyCloudHeight        (*(float *)(tr_shaderParseScratch + 0x4460))
#define shaderParseFogColor        (*(unsigned char *)(tr_shaderParseScratch + 0x4494))
#define shaderParseFogDepthForOpaque        (*(float *)(tr_shaderParseScratch + 0x44A0))
#define shaderParsePortalRange      (*(int *)(tr_shaderParseScratch + 0x44A4))
#define shaderParseCullType      (*(int *)(tr_shaderParseScratch + 0x44A8))
#define shaderParseNumDeforms      (*(int *)(tr_shaderParseScratch + 0x44AC))
#define shaderParseDeforms        (*(unsigned char *)(tr_shaderParseScratch + 0x44B0))
#define shaderParseTexModsBundle1End       (*(char *)(tr_shaderParseScratch + 0x4510))
#define shaderParseBoundsExpansion        (*(float *)(tr_shaderParseScratch + 0x454C))
#define shaderParseNumUnfoggedPasses      (*(int *)(tr_shaderParseScratch + 0x4550))
#define shaderParseStages      ((int *)(tr_shaderParseScratch + 0x4554))
#define shaderParseOptimalStageIteratorFunc      (*(int *)(tr_shaderParseScratch + 0x4574))
#define shaderParseClampTime        (*(float *)(tr_shaderParseScratch + 0x4578))
#define shaderParseOptimizedBackend      (*(int *)(tr_shaderParseScratch + 0x4588))
#define shaderParseStageStorage      ((int *)(tr_shaderParseScratch + 0x45A8))
#define shaderParseStage0Bundle0Image      ((int *)(tr_shaderParseScratch + 0x45AC))
#define shaderParseStage0Bundle0NumImageAnimations        (*(unsigned char *)(tr_shaderParseScratch + 0x462C))
#define shaderParseStage0Bundle0TexCoordComponentCount      (*(int *)(tr_shaderParseScratch + 0x4638))
#define shaderParseStage0Bundle0TextureEnvMode      ((int *)(tr_shaderParseScratch + 0x463C))
#define shaderParseStage0Bundle0TcGen        (*(unsigned char *)(tr_shaderParseScratch + 0x4648))
#define shaderParseStage0Bundle0TexMods        (*(unsigned char *)(tr_shaderParseScratch + 0x4668))
#define shaderParseStage0Bundle0IsLightmap       (*(unsigned char *)(tr_shaderParseScratch + 0x4670))
#define shaderParseStage0Bundle1Image      (*(int *)(tr_shaderParseScratch + 0x4674))
#define shaderParseStage0Bundle1TextureEnvMode      (*(int *)(tr_shaderParseScratch + 0x4704))
#define shaderParseStage0Bundle1TexMods        (*(unsigned char *)(tr_shaderParseScratch + 0x4730))
#define shaderParseStage0Bundle1IsLightmap       (*(unsigned char *)(tr_shaderParseScratch + 0x4738))
#define shaderParseStage0Bundle2Image      (*(int *)(tr_shaderParseScratch + 0x473C))
#define shaderParseStage0RgbWave        (*(unsigned char *)(tr_shaderParseScratch + 0x4BF8))
#define shaderParseStage0RgbGen      (*(int *)(tr_shaderParseScratch + 0x4C0C))
#define shaderParseStage0AlphaWave        (*(unsigned char *)(tr_shaderParseScratch + 0x4C10))
#define shaderParseStage0AlphaGen      (*(int *)(tr_shaderParseScratch + 0x4C24))
#define shaderParseStage0ConstantColorR       (*(unsigned char *)(tr_shaderParseScratch + 0x4C28))
#define shaderParseStage0ConstantColorG       (*(unsigned char *)(tr_shaderParseScratch + 0x4C29))
#define shaderParseStage0ConstantColorB       (*(unsigned char *)(tr_shaderParseScratch + 0x4C2A))
#define shaderParseStage0StateBits      (*(int *)(tr_shaderParseScratch + 0x4C2C))
#define shaderParseStage1Flags      (*(int *)(tr_shaderParseScratch + 0x4C30))
#define shaderParseStage1Bundle0Image      (*(int *)(tr_shaderParseScratch + 0x4C34))
#define shaderParseStage1Bundle0TexCoordComponentCount      (*(int *)(tr_shaderParseScratch + 0x4CC0))
#define shaderParseStage1Bundle0TextureEnvMode      (*(int *)(tr_shaderParseScratch + 0x4CC4))
#define shaderParseStage1Bundle1Image      (*(int *)(tr_shaderParseScratch + 0x4CFC))
#define shaderParseStage1RgbWave        (*(unsigned char *)(tr_shaderParseScratch + 0x5280))
#define shaderParseStage1RgbGen      (*(int *)(tr_shaderParseScratch + 0x5294))
#define shaderParseStage1AlphaWave        (*(unsigned char *)(tr_shaderParseScratch + 0x5298))
#define shaderParseStage1AlphaGen      (*(int *)(tr_shaderParseScratch + 0x52AC))
#define shaderParseStage1StateBits      (*(int *)(tr_shaderParseScratch + 0x52B4))
#define shaderParseStage2        (*(unsigned char *)(tr_shaderParseScratch + 0x52B8))
#define shaderParseStage7        (*(unsigned char *)(tr_shaderParseScratch + 0x7360))
#define skyOuterBox        ((int *)(tr_shaderParseScratch + 0x4464))
#define skyInnerBox        ((int *)(tr_shaderParseScratch + 0x447C))
#define rendererShaderRequirements ((unsigned char *)(tr_shaderParseScratch + 0x4598))
#define RENDERER_SHADER_REQUIREMENTS_COUNT 15

#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
typedef struct ShaderTextHashNode_s { char *text; struct ShaderTextHashNode_s *next; } ShaderTextHashNode;
ShaderTextHashNode shaderTextHashTable[4096];
int s_shaderText;
int surfacetype_Opaqueglass;


/* -------------------------------------------------------------------------
 *  File-local typed views of the shader-script data the parser builds.
 *
 *  tr_local.h already publishes a THIN shader_t (0x198, only the fields the
 *  renderer core reads).  This file needs the fuller layout, so it declares
 *  shaderDef_t under a distinct name -- same 0x198 record, more fields named --
 *  reusing the proven offsets from tr_bsp.c's bspShader_t / bspShaderStage_t
 *  and tr_shade.c's shaderStage_t.  The per-stage texture-bundle interior is
 *  the layout the sibling back ends disagree on, so it is left as padding and
 *  the parser reaches its sub-fields with raw byte offsets rather than commit
 *  to an unproven map.  waveForm_t / deformStage_t / texModInfo_t match
 *  tr_shade_calc.c exactly.
 * ------------------------------------------------------------------------- */

typedef struct {
	int   func;                 /* +0x00  genFunc_t */
	float base;                 /* +0x04 */
	float amplitude;            /* +0x08 */
	float phase;                /* +0x0c */
	float frequency;            /* +0x10 */
} waveForm_t;
COD1_ASSERT_SIZE(waveForm_t, 0x14);

typedef struct {
	int        deformation;        /* +0x00  deform_t */
	vec3_t     moveVector;         /* +0x04 */
	waveForm_t deformationWave;    /* +0x10 */
	float      deformationSpread;  /* +0x24 */
	float      bulgeWidth;         /* +0x28 */
	float      bulgeHeight;        /* +0x2c */
	float      bulgeSpeed;         /* +0x30 */
} deformStage_t;
COD1_ASSERT_SIZE(deformStage_t, 0x34);
ASSERT_OFF(deformStage_t, deformationWave, 0x10);

/* One tcMod entry (0x44).  type@0 selects the union; the leading wave shares the
 * +0x04 base with waveForm_t; matrix/translate proven by RB_CalcTransformTexCoords. */
typedef struct {
	int        type;               /* +0x00 */
	waveForm_t wave;               /* +0x04  (stretch)     -- overlaps type at +0 conceptually */
	float      matrix[2][2];       /* +0x18  (transform) */
	float      translate[2];       /* +0x28  (transform) */
	float      scale[2];           /* +0x30 */
	float      scroll[2];          /* +0x38 */
	float      rotateSpeed;        /* +0x40 */
} texModInfo_t;
COD1_ASSERT_SIZE(texModInfo_t, 0x44);
ASSERT_OFF(texModInfo_t, matrix, 0x18);
ASSERT_OFF(texModInfo_t, translate, 0x28);

/* Per-TMU texture bundle (0xC8) as the parser indexes it: `(textureBundle_t *)stage`,
 * so each record starts 4 bytes BEFORE image[0] (tr_shade.c's textureBundle_t
 * starts at image[0]).  The leading `flags` dword is therefore the stage flags
 * for bundle 0 and the previous bundle's isLightmap / isVideoMap / clampAnimation
 * bytes for the others.  image[]@0x04, numImageAnimations@0x84, texEnv@0x94 and
 * the tcMod pair @0xBC/0xC0 are proven by this file's own reads (200-byte stride,
 * `v4[47]`/`v4[48]`, `68 * n` tcMod indexing); the rest follow tr_shade.c. */
typedef struct {
	float real;             /* +0x00 */
	float imaginary;        /* +0x04 */
} waterComplex_t;
typedef struct {
	void           *image;                 /* +0x00  GL image (width @ +64, height @ +66) */
	int             uploadFrame;           /* +0x04  tr.frameCount of the last upload */
	waterComplex_t *initialFrequencies;    /* +0x08  h0(k), textureWidth*textureHeight bins */
	float          *angularFrequencies;    /* +0x0c  w(k), one per bin (== base + 8*texelCount) */
	int             textureWidth;          /* +0x10 */
	int             textureHeight;         /* +0x14 */
	float           horizontalWorldLength; /* +0x18  world span mapped across the height axis */
	float           verticalWorldLength;   /* +0x1c  world span mapped across the width axis */
	float           gravity;               /* +0x20 */
	float           windVelocity;          /* +0x24 */
	float           windDirection[2];      /* +0x28  [0] = x, [1] = y */
	float           amplitude;             /* +0x30 */
} waterMap_t;
typedef struct {
 unsigned int operation;
 int textureSource;
 int inputMapping;
 union { int cull[4]; float parameter[4]; } argument;
} ShaderNVTextureOperation;
typedef struct {
	unsigned int flags;
 void *image[32];
 int numImageAnimations;
 float imageAnimationSpeed;
 waterMap_t *waterTexture;
 int texCoordComponentCount;
 int textureEnvMode;
 void *texEnvCombine;   /* ShaderTexEnvCombine */
 ShaderNVTextureOperation *nvTextureShader;
 int tcGen;
 float tcGenVectors[2][3];
 int numTexMods;
 texModInfo_t *texMods;
 int videoMapHandle;
} textureBundle_t;
typedef waterMap_t ShaderWaterConfiguration;
typedef struct {
 int function;
 int source[3];
 int operand[3];
 float scale;
} ShaderTexEnvEquation;
typedef struct {
 float color[4];
 ShaderTexEnvEquation rgb;
 ShaderTexEnvEquation alpha;
} ShaderTexEnvCombine;
/* Actual image-array subobject passed to ParseTextureEnvCombine: flags are
 * the preceding four bytes in the enclosing 200-byte bundle. */
typedef struct {
 void *image[32];
 int numImageAnimations;
 float imageAnimationSpeed;
 waterMap_t *waterTexture;
 int texCoordComponentCount;
 int texEnvMode;
 ShaderTexEnvCombine *combine;
 ShaderNVTextureOperation *nvTextureShader;
 int tcGen;
 float tcGenVectors[2][3];
 int numTexMods;
 texModInfo_t *texMods;
 int videoMapHandle;
} ShaderTextureImageData;

COD1_ASSERT_SIZE(textureBundle_t, 0xC8);

/* One shader stage (0x688).  Two texture bundles fill the head; only the outer
 * generator / state fields are proven and named. */
struct ShaderNVCombinerState;
typedef struct shaderStage_s {
	unsigned int flags;                        /* +0x000  attribute/stage-type bits */
	byte         _pad004[0x644 - 0x004];       /* texture bundles (interior disputed) */
	int          atiFragmentShader;            /* +0x644  ParseATIFragmentShader */
 struct ShaderNVCombinerState *registerCombiners;
 char        *vertexProgram;                /* +0x64c  ParseVertexProgram */
 byte         rgbWave[0x14];                /* +0x650  waveForm_t (CompareWaveForms) */
	int          rgbGen;                       /* +0x664 */
	byte         alphaWave[0x14];              /* +0x668  waveForm_t (CompareWaveForms) */
	int          alphaGen;                     /* +0x67c */
	byte         constantColor[4];             /* +0x680 */
	unsigned int stateBits;                    /* +0x684 */
} shaderStage_t;
COD1_ASSERT_SIZE(shaderStage_t, 0x688);
ASSERT_OFF(shaderStage_t, rgbGen, 0x664);
ASSERT_OFF(shaderStage_t, alphaGen, 0x67c);
ASSERT_OFF(shaderStage_t, constantColor, 0x680);
ASSERT_OFF(shaderStage_t, stateBits, 0x684);

/* Fuller runtime shader (0x198) -- the layout GeneratePermanentShader hunk-copies
 * out of the parse scratch.  Distinct name from tr_local.h's thin shader_t. */
typedef struct shaderDef_s {
	char          name[64];                    /* +0x00 */
	int           lightmapIndex;               /* +0x40 */
	int           index;                       /* +0x44 */
	int           sortedIndex;                 /* +0x48 */
	unsigned int  flags;                       /* +0x4c */
	unsigned int  surfaceFlags;                /* +0x50 */
	unsigned int  lightingFlags;               /* +0x54 */
	float         sort;                        /* +0x58 */
	unsigned int  surfaceParmFlags;            /* +0x5c */
	float         skyCloudHeight;              /* +0x60 */
	void         *skyOuterBoxImages[6];        /* +0x64  (member name avoids the skyOuterBox parse-scratch macro) */
	void         *skyInnerBoxImages[6];        /* +0x7c */
	byte          fogParms[0x0c];              /* +0x94  fog colour + params */
	float         fogDepthForOpaque;           /* +0xa0 */
	float         portalRange;                 /* +0xa4 */
	int           cullType;                    /* +0xa8 */
	int           numDeforms;                  /* +0xac */
	deformStage_t deforms[3];                  /* +0xb0 .. 0x14c */
	float         boundsExpansion;             /* +0x14c */
	int           numUnfoggedPasses;           /* +0x150 */
	shaderStage_t *stages[8];                  /* +0x154 */
	void         *optimalStageIteratorFunc;    /* +0x174 */
	float         clampTime;                   /* +0x178 */
	float         timeOffset;                  /* +0x17c  RTCW's member after clampTime (RB_RenderDrawSurfList reads it) */
	struct shaderDef_s *remappedShader;        /* +0x180  (tr_model.c R_ShaderRemapped) */
 void *mergeImage;
	int           optimizedBackend;            /* +0x188 */
	unsigned int  optimizedVertexStorage;      /* +0x18c */
	int           optimizedVertexStorageOffset;/* +0x190 */
	struct shaderDef_s *next;                  /* +0x194 */
} shaderDef_t;
COD1_ASSERT_SIZE(shaderDef_t, 0x198);
ASSERT_OFF(shaderDef_t, flags, 0x4c);
ASSERT_OFF(shaderDef_t, sort, 0x58);
ASSERT_OFF(shaderDef_t, portalRange, 0xa4);
ASSERT_OFF(shaderDef_t, deforms, 0xb0);
ASSERT_OFF(shaderDef_t, boundsExpansion, 0x14c);
ASSERT_OFF(shaderDef_t, numUnfoggedPasses, 0x150);
ASSERT_OFF(shaderDef_t, stages, 0x154);
ASSERT_OFF(shaderDef_t, optimalStageIteratorFunc, 0x174);
ASSERT_OFF(shaderDef_t, optimizedBackend, 0x188);
ASSERT_OFF(shaderDef_t, next, 0x194);

const char *skySuffix[6] = { "rt", "bk", "lf", "ft", "up", "dn" };  /* 0x005712D8 */

int shaderHashTable[4096];       /* 0x011E9D60, 16384 bytes */


typedef struct {
    const char  *token;
    unsigned int glRegister;
    unsigned int componentUsage;
} tr_nvRegisterDef_t;
#define tr_nvReadWriteRegisters  ((const char **)(void *)&nvReadWriteRegisterDefinitions)  /* 0x00540268, 32 x 12 */
#define tr_nvReadRegisters       ((const char **)(void *)&nvReadOnlyRegisterDefinitions)  /* 0x005403E8, 20 x 12 */

/* === Merged from renderer/tr_shader_rdata.c (NV register tables) (retail linked it as tr_*.obj .rdata/.data). === */

/* ==========================================================================
 * nvReadWriteRegisterDefinitions              0x00540268        384 bytes (32 x 12)
 * nvReadOnlyRegisterDefinitions              0x005403E8        240 bytes (20 x 12)
 * ==========================================================================
 *
 * The two GL_NV_register_combiners register-name tables.  Both are handed to
 * ParseNVRC_RegisterFromTable (0x004F4780), which walks them as `const char **`
 * with `i += 3` -- twelve bytes -- so the entry shape is
 *
 *     { const char *token; GLenum glRegister; GLenum componentUsage }
 *
 * Both call sites pass the count as an immediate: 32 in
 * ParseNVRC_ReadWriteRegister and `mov ebx, 14h` = 20 at 0x004F49AC in
 * ParseNVRC_ReadRegister.  32 x 12 = 384 ends the first table at 0x005403E8,
 * exactly where the second begins; 20 x 12 = 240 ends the second at
 * 0x005404D8, where the multitexture collapse table starts.  (Sizes measured to the
 * next NAMED symbol -- 972 and 588 bytes, straight through the other table --
 * are wrong.)
 *
 * ParseNVRC_RegisterFromTable dereferences `*i` on its first iteration, so a
 * zeroed table does not merely fail to match: every register name in every
 * `nvRegCombiners` block reaches _stricmp with a NULL first argument.
 *
 * Component usage is GL_ALPHA 0x1906 / GL_RGB 0x1907 / GL_BLUE 0x1905, with
 * zero meaning "use the caller's default".  Registers are
 * GL_PRIMARY_COLOR_NV 0x852C, GL_SECONDARY_COLOR_NV 0x852D, GL_SPARE0_NV
 * 0x852E, GL_SPARE1_NV 0x852F and GL_TEXTURE0_ARB..+3 0x84C0..0x84C3 in the
 * read/write table; GL_FOG 0x0B60, GL_ZERO 0, GL_ONE 1 and
 * GL_CONSTANT_COLOR0/1_NV 0x852A/0x852B in the read-only table.
 */

tr_nvRegisterDef_t nvReadWriteRegisterDefinitions[32] = {   /* 0x00540268 */
    { "col0",        0x852C, 0x0000 },   /* +0x000 */
    { "col0.a",      0x852C, 0x1906 },   /* +0x00C */
    { "col0.rgb",    0x852C, 0x1907 },   /* +0x018 */
    { "col0.b",      0x852C, 0x1905 },   /* +0x024 */
    { "col1",        0x852D, 0x0000 },   /* +0x030 */
    { "col1.a",      0x852D, 0x1906 },   /* +0x03C */
    { "col1.rgb",    0x852D, 0x1907 },   /* +0x048 */
    { "col1.b",      0x852D, 0x1905 },   /* +0x054 */
    { "spare0",      0x852E, 0x0000 },   /* +0x060 */
    { "spare0.a",    0x852E, 0x1906 },   /* +0x06C */
    { "spare0.rgb",  0x852E, 0x1907 },   /* +0x078 */
    { "spare0.b",    0x852E, 0x1905 },   /* +0x084 */
    { "spare1",      0x852F, 0x0000 },   /* +0x090 */
    { "spare1.a",    0x852F, 0x1906 },   /* +0x09C */
    { "spare1.rgb",  0x852F, 0x1907 },   /* +0x0A8 */
    { "spare1.b",    0x852F, 0x1905 },   /* +0x0B4 */
    { "tex0",        0x84C0, 0x0000 },   /* +0x0C0 */
    { "tex0.a",      0x84C0, 0x1906 },   /* +0x0CC */
    { "tex0.rgb",    0x84C0, 0x1907 },   /* +0x0D8 */
    { "tex0.b",      0x84C0, 0x1905 },   /* +0x0E4 */
    { "tex1",        0x84C1, 0x0000 },   /* +0x0F0 */
    { "tex1.a",      0x84C1, 0x1906 },   /* +0x0FC */
    { "tex1.rgb",    0x84C1, 0x1907 },   /* +0x108 */
    { "tex1.b",      0x84C1, 0x1905 },   /* +0x114 */
    { "tex2",        0x84C2, 0x0000 },   /* +0x120 */
    { "tex2.a",      0x84C2, 0x1906 },   /* +0x12C */
    { "tex2.rgb",    0x84C2, 0x1907 },   /* +0x138 */
    { "tex2.b",      0x84C2, 0x1905 },   /* +0x144 */
    { "tex3",        0x84C3, 0x0000 },   /* +0x150 */
    { "tex3.a",      0x84C3, 0x1906 },   /* +0x15C */
    { "tex3.rgb",    0x84C3, 0x1907 },   /* +0x168 */
    { "tex3.b",      0x84C3, 0x1905 }    /* +0x174 */
};

tr_nvRegisterDef_t nvReadOnlyRegisterDefinitions[20] = {   /* 0x005403E8 */
    { "fog",         0x0B60, 0x0000 },   /* +0x000 */
    { "fog.a",       0x0B60, 0x1906 },   /* +0x00C */
    { "fog.rgb",     0x0B60, 0x1907 },   /* +0x018 */
    { "fog.b",       0x0B60, 0x1905 },   /* +0x024 */
    { "zero",        0x0000, 0x0000 },   /* +0x030 */
    { "zero.a",      0x0000, 0x1906 },   /* +0x03C */
    { "zero.rgb",    0x0000, 0x1907 },   /* +0x048 */
    { "zero.b",      0x0000, 0x1905 },   /* +0x054 */
    { "one",         0x0001, 0x0000 },   /* +0x060 */
    { "one.a",       0x0001, 0x1906 },   /* +0x06C */
    { "one.rgb",     0x0001, 0x1907 },   /* +0x078 */
    { "one.b",       0x0001, 0x1905 },   /* +0x084 */
    { "const0",      0x852A, 0x0000 },   /* +0x090 */
    { "const0.a",    0x852A, 0x1906 },   /* +0x09C */
    { "const0.rgb",  0x852A, 0x1907 },   /* +0x0A8 */
    { "const0.b",    0x852A, 0x1905 },   /* +0x0B4 */
    { "const1",      0x852B, 0x0000 },   /* +0x0C0 */
    { "const1.a",    0x852B, 0x1906 },   /* +0x0CC */
    { "const1.rgb",  0x852B, 0x1907 },   /* +0x0D8 */
    { "const1.b",    0x852B, 0x1905 }    /* +0x0E4 */
};

typedef struct {
    int srcStateBits;
    int dstStateBits;
    int texEnv;
    int collapsedStateBits;
} tr_collapseRule_t;

static const tr_collapseRule_t tr_collapseRules[9] = {
    { 0x00, 0x31, 0x2100, 0x00 },   /* 0x005404D8 */
    { 0x00, 0x13, 0x2100, 0x00 },   /* 0x005404E8 */
    { 0x13, 0x13, 0x2100, 0x13 },   /* 0x005404F8 */
    { 0x31, 0x13, 0x2100, 0x13 },   /* 0x00540508 */
    { 0x13, 0x31, 0x2100, 0x13 },   /* 0x00540518 */
    { 0x31, 0x31, 0x2100, 0x13 },   /* 0x00540528 */
    { 0x00, 0x22, 0x0104, 0x00 },   /* 0x00540538 */
    { 0x22, 0x22, 0x0104, 0x22 },   /* 0x00540548 */
    { 0x00, 0x65, 0x2101, 0x00 }    /* 0x00540558 */
};

static const int tr_texEnvArgSwap[6] = { 0, 1, 4, 5, 2, 3 };  /* 0x00540250 */

extern char *Com_ParseOnLine(char **data_p);
extern void Com_ParseReturnToMark( int *mark, char **data_p );
extern void Com_ParseSetMark( int *mark, char **data_p );
extern int Com_ScriptError();
extern int Com_SkipBracedSection();
extern void Com_SkipRestOfLine( char **data_p );
extern int Com_UngetToken();
extern void __cdecl RB_ExecuteRenderCommands(const void *data);
void __cdecl R_SyncRenderThread( void );
/* the backend command list (tr_cmds.c renderCommandList_t) as R_IssueRenderCommands addresses it */
typedef struct { unsigned char cmds[0x40000]; unsigned int used; } shaderCmdList_t;
extern int RB_StageIteratorSky();
extern int __cdecl R_FindImageFile( char *name, int target, int flags, int track,
                                    const float *colorScale, float heightmapScale );
extern int R_FindImageInstance();
extern char *__cdecl R_FindVertexProgram( const char *name );
extern waterMap_t *__cdecl R_GetWaterTexture(waterMap_t *config);
extern int __cdecl R_InitSkyTexCoords( float cloudHeight );
extern void R_UpdateDelayLoadImage( int imageAddr, int shader, int forceLoad );
float VectorNormalize2D( float *v );
extern int j__atol();
extern int qsort_m();

/* ---- tr_shader_generateHashValue  0x004F0BA0 ----  VERIFIED */
int __cdecl tr_shader_generateHashValue(char *fname)
{
  int   hash;
  int   i;
  int   letter;

  hash = 0;
  i = 0;
  while ( fname[i] != '\0' )
  {
    letter = tolower( fname[i] );
    if ( letter == '.' )        /* don't include extension */
      break;
    if ( letter == '\\' )       /* damn path names */
      letter = '/';
    hash += letter * ( i + 119 );
    i++;
  }
  return hash & 0xFFF;
}

/* ---- MergableShader  0x004F0BF0 ----  VERIFIED */
int __cdecl MergableShader(shaderDef_t *shader)
{
  void *collected[256];
  int count;
  int i;
  unsigned int j;
  int *bundle;
  void *image;
  shaderStage_t *stage;

  shader->mergeImage = 0;
  if ( shader->flags & 9 )
    return 0;
  shader->flags |= 0x800;
  count = 0;
  for ( i = 0; i < shader->numUnfoggedPasses; i++ )
  {
    stage = shader->stages[i];
    if ( *(int *)((char *)stage + 1612) )   /* stage +0x64c (after registerCombiners) */
    {
      shader->mergeImage = 0;
      return 0;
    }
    for ( j = 0, bundle = (int *)((char *)stage + 4); j < 8; j++, bundle += 50 )
    {
      image = *(void **)bundle;
      if ( !image )
        break;
      if ( bundle[39] != 3 )
        collected[count++] = image;
      else if ( image != (void *)tr_whiteImage || bundle[32] > 1 )
      {
        if ( shader->mergeImage && shader->mergeImage != image )
        {
          shader->mergeImage = 0;
          return 0;
        }
        if ( bundle[46] || bundle[32] > 1 || bundle[38] )
          return 0;
        shader->mergeImage = image;
      }
    }
  }
  if ( !shader->mergeImage )
    return 0;
  for ( i = 0; i < count; i++ )
  {
    if ( collected[i] == shader->mergeImage )
    {
      shader->mergeImage = 0;
      return 0;
    }
  }
  return 1;
}

/* ---- UpdateDelayLoadImagesForShader  0x004F0D20 ----  [HIGH] */
void __cdecl UpdateDelayLoadImagesForShader(shaderDef_t *shader, int forceLoad)
{
  unsigned int    flags;
  int            *bundle;
  int             imageIndex;
  int             image;
  shaderStage_t **stagePtr;
  unsigned int    bundleIndex;
  int             stageIndex;

  flags = shader->flags;
  if ( ( flags & 0x800 ) != 0 )
  {
    if ( !(*(int *)(((char *)(void *)&tr_registered) + 0x0128)) || tr_delayedImageGroupTileMode >= r_optimizeTextures->integer )
      forceLoad = 1;
    shader->flags = flags & 0xFFFFF7FF;

    stageIndex = 0;
    if ( shader->numUnfoggedPasses > 0 )
    {
      stagePtr = shader->stages;
      do
      {
        bundleIndex = 0;
        bundle = (int *)( (char *)*stagePtr + 4 );
        do
        {
          if ( !*bundle )
            break;
          imageIndex = 0;
          do
          {
            image = bundle[imageIndex];
            R_UpdateDelayLoadImage( image, (int)shader, forceLoad );
            if ( *(unsigned char *)( image + 100 ) & 0x80 )
              shader->flags |= 0x800u;
            ++imageIndex;
          }
          while ( imageIndex < bundle[32] );        
          ++bundleIndex;
          bundle += 50;
        }
        while ( bundleIndex < 8 );
        ++stageIndex;
        ++stagePtr;
      }
      while ( stageIndex < shader->numUnfoggedPasses );
    }
  }
  return;
}

/* ---- CompareTexEnvCombineArgs  0x004F0E00 ----  [HIGH] */
int __cdecl CompareTexEnvCombineArgs(int argsA, int argsB)
{
  int result;

  result = *(int *)argsA - *(int *)argsB;
  if ( result )
    return result;
  result = *(int *)( argsA + 4 ) - *(int *)( argsB + 4 );
  if ( result )
    return result;
  result = *(int *)( argsA + 8 ) - *(int *)( argsB + 8 );
  if ( result )
    return result;
  result = *(int *)( argsA + 12 ) - *(int *)( argsB + 12 );
  if ( result )
    return result;
  result = *(int *)( argsA + 16 ) - *(int *)( argsB + 16 );
  if ( result )
    return result;
  result = *(int *)( argsA + 20 ) - *(int *)( argsB + 20 );
  if ( result )
    return result;
  result = *(int *)( argsA + 24 ) - *(int *)( argsB + 24 );
  if ( result )
    return result;
  /* 0x004F0E36: != first (an unordered NaN pair counts as unequal), then
   * argsA < argsB gives -1 and everything else 1 */
  if ( *(float *)( argsA + 28 ) != *(float *)( argsB + 28 ) )
    return ( *(float *)( argsA + 28 ) < *(float *)( argsB + 28 ) ) ? -1 : 1;
  return 0;
}

/* ---- CompareTexEnvCombine  0x004F0E60 ----  [HIGH] */
int __cdecl CompareTexEnvCombine(float *combineA, float *combineB)
{
  int result;

  result = ( (char *)combineA - (char *)combineB ) / 80;
  if ( !result && combineA )
  {
    result = CompareTexEnvCombineArgs( (int)( combineA + 4 ), (int)( combineB + 4 ) );
    if ( result )
      return result;
    result = CompareTexEnvCombineArgs( (int)( combineA + 12 ), (int)( combineB + 12 ) );
    if ( result )
      return result;
    /* 0x004F0EA3: != first (unordered NaN counts as unequal), then
     * combineA < combineB gives -1 and everything else 1 */
    if ( combineA[0] != combineB[0] )
      return ( combineA[0] < combineB[0] ) ? -1 : 1;
    if ( combineA[1] != combineB[1] )
      return ( combineA[1] < combineB[1] ) ? -1 : 1;
    if ( combineA[2] != combineB[2] )
      return ( combineA[2] < combineB[2] ) ? -1 : 1;
    if ( combineA[3] != combineB[3] )
      return ( combineA[3] < combineB[3] ) ? -1 : 1;
    return 0;
  }
  return result;
}

/* ---- CompareNvTexShaders  0x004F0F10 ----  [HIGH] */
int __cdecl CompareNvTexShaders(char *texShaderA, char *texShaderB)
{
  int result;

  result = ( texShaderA - texShaderB ) / 28;
  if ( !result && texShaderA )
  {
    result = *((int *)texShaderA + 0) - *((int *)texShaderB + 0);
    if ( result )
      return result;
    result = *((int *)texShaderA + 1) - *((int *)texShaderB + 1);
    if ( result )
      return result;
    result = *((int *)texShaderA + 2) - *((int *)texShaderB + 2);
    if ( result )
      return result;
    result = *((int *)texShaderA + 3) - *((int *)texShaderB + 3);
    if ( result )
      return result;
    result = *((int *)texShaderA + 4) - *((int *)texShaderB + 4);
    if ( result )
      return result;
    result = *((int *)texShaderA + 5) - *((int *)texShaderB + 5);
    if ( result )
      return result;
    return *((int *)texShaderA + 6) - *((int *)texShaderB + 6);
  }
  return result;
}

/* ---- CompareNvRegisterCombiners  0x004F0F70 ----  [HIGH] */
int __cdecl CompareNvRegisterCombiners(const char *combinersA, const char *combinersB)
{
  int result;

  result = ( combinersA - combinersB ) / 1536;
  if ( !result && combinersA )
    result = memcmp( combinersA, combinersB, 1536 );
  return result;
}

/* ---- CompareWaveForms  0x004F0FB0 ----  [HIGH] */
int __cdecl CompareWaveForms(int waveA, int waveB)
{
  if ( *(int *)waveA != *(int *)waveB )
    return *(int *)waveA - *(int *)waveB;
  /* 0x004F0FBD: fcomp / test ah,44h / jnp -- any inequality (an unordered
   * NaN pair included) decides; only waveA < waveB gives -1, everything else 1 */
  if ( *(float *)( waveA + 4 ) != *(float *)( waveB + 4 ) )
    return ( *(float *)( waveA + 4 ) < *(float *)( waveB + 4 ) ) ? -1 : 1;
  if ( *(float *)( waveA + 8 ) != *(float *)( waveB + 8 ) )
    return ( *(float *)( waveA + 8 ) < *(float *)( waveB + 8 ) ) ? -1 : 1;
  if ( *(float *)( waveA + 12 ) != *(float *)( waveB + 12 ) )
    return ( *(float *)( waveA + 12 ) < *(float *)( waveB + 12 ) ) ? -1 : 1;
  if ( *(float *)( waveA + 16 ) != *(float *)( waveB + 16 ) )
    return ( *(float *)( waveA + 16 ) < *(float *)( waveB + 16 ) ) ? -1 : 1;
  return 0;
}

/* ---- CompareTextureBundles  0x004F1030 ----  [HIGH] */
int __cdecl CompareTextureBundles(int *bundleA, int *bundleB, int mergeImageA, int mergeImageB)
{
  int result;
  int type;
  int i;

  result = ( bundleA[0] - bundleB[0] ) / 120;
  /* comparable only when both bundles refer to the same slot (or match the
   * requested mergeImageA/mergeImageB images) and both have a non-zero primary image
   * (0x004F106C test esi,esi / test edi,edi: integer pointer tests) */
  if ( !( ( !result || ( bundleA[0] == mergeImageA && bundleB[0] == mergeImageB ) )
          && bundleA[0] != 0 && bundleB[0] != 0 ) )
    return result;

  if ( bundleA[32] > 1 || bundleB[32] > 1 )
  {
    result = bundleA[32] - bundleB[32];
    if ( result )
      return result;
    /* 0x004F10A3: != first (unordered NaN counts as unequal), then bundleA < bundleB
     * gives -1 and everything else 1 */
    if ( *((float *)bundleA + 33) != *((float *)bundleB + 33) )
      return ( *((float *)bundleA + 33) < *((float *)bundleB + 33) ) ? -1 : 1;
    for ( i = 1; i < bundleA[32]; ++i )
    {
      result = ( bundleA[i] - bundleB[1] ) / 120;
      if ( result )
        return result;
    }
  }

  result = ( bundleA[34] - bundleB[34] ) / 52;
  if ( result )
    return result;
  result = bundleA[35] - bundleB[35];
  if ( result )
    return result;
  result = bundleA[36] - bundleB[36];
  if ( result )
    return result;
  type = bundleA[39];
  result = type - bundleB[39];
  if ( result )
    return result;

  if ( type != 3 )
  {
    if ( type == 6 )
    {
      result = memcmp( (const char *)bundleA + 160, (const char *)bundleB + 160, 24 );
      if ( result )
        return result;
    }
    result = bundleA[46] - bundleB[46];
    if ( result )
      return result;
    for ( i = 0; i < bundleA[46]; ++i )
    {
      result = memcmp( (const char *)bundleA[47] + 68 * i, (const char *)bundleB[47] + 68 * i, 68 );
      if ( result )
        return result;
    }
    result = CompareNvTexShaders( (char *)bundleA[38], (char *)bundleB[38] );
    if ( result )
      return result;
  }

  result = bundleA[48] - bundleB[48];
  if ( result )
    return result;
  result = *((unsigned char *)bundleA + 196) - *((unsigned char *)bundleB + 196);
  if ( result )
    return result;
  result = *((unsigned char *)bundleA + 197) - *((unsigned char *)bundleB + 197);
  if ( result )
    return result;
  result = *((unsigned char *)bundleA + 198) - *((unsigned char *)bundleB + 198);
  if ( result )
    return result;
  return CompareTexEnvCombine( (float *)bundleA[37], (float *)bundleB[37] );
}

/* ---- CompareMergableShaders  0x004F1280 ----  [HIGH] */
int __cdecl CompareMergableShaders(int shaderA, int shaderB, int mergeImageA, int mergeImageB)
{
  shaderDef_t   *s1 = (shaderDef_t *)shaderA;
  shaderDef_t   *s2 = (shaderDef_t *)shaderB;
  shaderStage_t *st1;
  shaderStage_t *st2;
  int            result;
  int            numStages;
  int            stageIndex;
  int            bundleIndex;

  if ( shaderB == shaderA )
    return 0;
  result = s2->lightmapIndex - s1->lightmapIndex;
  if ( result )
    return result;
  result = (int)s2->flags - (int)s1->flags;
  if ( result )
    return result;
  result = (int)s2->surfaceFlags - (int)s1->surfaceFlags;
  if ( result )
    return result;
  result = (int)s2->lightingFlags - (int)s1->lightingFlags;
  if ( result )
    return result;
  /* 0x004F12C1: != first (unordered NaN counts as unequal), then
   * s2 < s1 gives -1 and everything else 1 */
  if ( s2->sort != s1->sort )
    return ( s2->sort < s1->sort ) ? -1 : 1;
  result = s2->cullType - s1->cullType;
  if ( result )
    return result;
  result = s2->numUnfoggedPasses - s1->numUnfoggedPasses;
  if ( result )
    return result;

  numStages = s2->numUnfoggedPasses;
  for ( stageIndex = 0; stageIndex < numStages; ++stageIndex )
  {
    st2 = s2->stages[stageIndex];
    st1 = s1->stages[stageIndex];
    result = (int)st2->flags - (int)st1->flags;
    if ( result )
      return result;

    for ( bundleIndex = 0; bundleIndex < 8; ++bundleIndex )
    {
      int *b2 = (int *)( (char *)st2 + 4 + 200 * bundleIndex );
      int *b1 = (int *)( (char *)st1 + 4 + 200 * bundleIndex );
      result = CompareTextureBundles( b2, b1, mergeImageA, mergeImageB );
      if ( result )
        return result;
    }

    result = *(int *)( (char *)st2 + 1604 ) - *(int *)( (char *)st1 + 1604 );
    if ( result )
      return result;
    result = CompareNvRegisterCombiners( *(const char **)( (char *)st2 + 1608 ),
                                         *(const char **)( (char *)st1 + 1608 ) );
    if ( result )
      return result;

    result = st2->rgbGen - st1->rgbGen;
    if ( result )
      return result;
    switch ( st2->rgbGen )
    {
    case 8:
      result = CompareWaveForms( (int)( (char *)st2 + 1616 ), (int)( (char *)st1 + 1616 ) );
      if ( result )
        return result;
      /* fall through */
    case 12:
      result = st2->constantColor[0] - st1->constantColor[0];
      if ( result )
        return result;
      result = st2->constantColor[1] - st1->constantColor[1];
      if ( result )
        return result;
      result = st2->constantColor[2] - st1->constantColor[2];
      if ( result )
        return result;
      break;
    }

    result = st2->alphaGen - st1->alphaGen;
    if ( result )
      return result;
    switch ( st2->alphaGen )
    {
    case 7:
      result = CompareWaveForms( (int)( (char *)st2 + 1640 ), (int)( (char *)st1 + 1640 ) );
      if ( result )
        return result;
      /* fall through */
    case 9:
      result = st2->constantColor[3] - st1->constantColor[3];
      if ( result )
        return result;
      break;
    }

    result = (int)st2->stateBits - (int)st1->stateBits;
    if ( result )
      return result;
  }
  return 0;
}

/* ---- MatchShaderTokenOnLine  0x004F14C0 ----  VERIFIED */
int __cdecl MatchShaderTokenOnLine(char **data_p, const char *expect, const char *context)
{
  const char *token;

  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    if ( parseInfo->spaceDelimited == qfalse )
    {
      token = parseInfo->token;               /* reuse the unget'd token */
    }
    else
    {
      *data_p = parseInfo->ungetTokenSave;
      parseInfo->currentLine = parseInfo->ungetLineSave;
      token = Com_ParseExt( data_p, qfalse );
    }
  }
  else
  {
    token = Com_ParseExt( data_p, qfalse );
  }
  if ( _stricmp( token, expect ) )
  {
    ri.Printf( 2, "WARNING: %s missing '%s', found '%s' instead in shader '%s'\n",
               context, expect, token, shaderParseName );
    return 0;
  }
  return 1;
}

/* ---- MatchShaderToken  0x004F1540 ----  VERIFIED */
int __cdecl MatchShaderToken(char **data_p, const char *expect, const char *context)
{
  char *token;

  token = Com_Parse( data_p );
  if ( _stricmp( token, expect ) )
  {
    ri.Printf( 2, "WARNING: %s missing '%s', found '%s' instead in shader '%s'\n",
               context, expect, token, shaderParseName );
    return 0;
  }
  return 1;
}

/* ---- ParseVector  0x004F15B0 ----  VERIFIED */
int __cdecl ParseVector(char **data_p, int count, float *out)
{
  char *token;
  int   i;

  
  token = Com_ParseOnLine( data_p );
  if ( strcmp( token, "(" ) )
  {
    ri.Printf( 2, "WARNING: missing parenthesis in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
    return 0;
  }

  for ( i = 0; i < count; ++i )
  {
    token = Com_ParseOnLine( data_p );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing vector element in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return 0;
    }
    out[i] = atof( token );
  }

  
  token = Com_ParseOnLine( data_p );
  if ( strcmp( token, ")" ) )
  {
    ri.Printf( 2, "WARNING: missing parenthesis in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
    return 0;
  }
  return 1;
}

/* ---- NameToAFunc  0x004F1710 ----  VERIFIED */
int __cdecl NameToAFunc( const char *funcname )
{
	if ( funcname )
	{
		if ( !Q_stricmp( funcname, "GT0" ) )
			return 0x10000000;
		if ( !Q_stricmp( funcname, "LT128" ) )
			return 0x20000000;
		if ( !Q_stricmp( funcname, "GE128" ) )
			return 0x40000000;
	}

	ri.Printf( 2, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n",
	           funcname, shaderParseName );
	return 0;
}

/* ---- NameToSrcBlendMode  0x004F1780 ----  VERIFIED */
int __cdecl NameToSrcBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
		return 2;
	if ( !Q_stricmp( name, "GL_ZERO" ) )
		return 1;
	if ( !Q_stricmp( name, "GL_DST_COLOR" ) )
		return 3;
	if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )
		return 4;
	if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
		return 5;
	if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
		return 6;
	if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
		return 7;
	if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
		return 8;
	if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )
		return 9;

	ri.Printf( 2, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n",
	           name, shaderParseName );
	return 2;
}

/* ---- NameToDstBlendMode  0x004F1890 ----  VERIFIED */
int __cdecl NameToDstBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
		return 0x20;
	if ( !Q_stricmp( name, "GL_ZERO" ) )
		return 0x10;
	if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
		return 0x50;
	if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
		return 0x60;
	if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
		return 0x70;
	if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
		return 0x80;
	if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )
		return 0x30;
	if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )
		return 0x40;

	ri.Printf( 2, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n",
	           name, shaderParseName );
	return 0x20;
}

/* ---- NameToGenFunc  0x004F1980 ----  VERIFIED */
int __cdecl NameToGenFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "sin" ) )
		return 1;
	if ( !Q_stricmp( funcname, "square" ) )
		return 2;
	if ( !Q_stricmp( funcname, "triangle" ) )
		return 3;
	if ( !Q_stricmp( funcname, "sawtooth" ) )
		return 4;
	if ( !Q_stricmp( funcname, "inversesawtooth" ) )
		return 5;
	if ( !Q_stricmp( funcname, "noise" ) )
		return 6;

	ri.Printf( 2, "WARNING: invalid genfunc name '%s' in shader '%s'\n",
	           funcname, shaderParseName );
	return 1;
}

/* ---- ParseWaveForm  0x004F1A40 ----  [HIGH] */
void __cdecl ParseWaveForm(int wave, char **data_p)
{
  char *token;

  token = Com_ParseOnLine( data_p );
  if ( !*token )
    goto missing;

  *(int *)wave = NameToGenFunc( token );
  token = Com_ParseOnLine( data_p );
  if ( !*token )
    goto missing;
  *(float *)( wave + 4 ) = atof( token );
  token = Com_ParseOnLine( data_p );
  if ( !*token )
    goto missing;
  *(float *)( wave + 8 ) = atof( token );
  token = Com_ParseOnLine( data_p );
  if ( !*token )
    goto missing;
  *(float *)( wave + 12 ) = atof( token );
  token = Com_ParseOnLine( data_p );
  if ( !*token )
    goto missing;
  *(float *)( wave + 16 ) = atof( token );
  return;

missing:
  ri.Printf( 2, "WARNING: missing waveform parm in shader '%s'\n", shaderParseName );
}

/* ---- MaxWaveFormDeformation  0x004F1B10 ----  VERIFIED */
double __cdecl MaxWaveFormDeformation(int wave)
{
  /* max( |base - amplitude|, |amplitude + base| ): retail keeps both in x87 temporaries, no double locals */
  return fabs( *(float *)( wave + 4 ) - *(float *)( wave + 8 ) ) > fabs( *(float *)( wave + 8 ) + *(float *)( wave + 4 ) )
       ? fabs( *(float *)( wave + 4 ) - *(float *)( wave + 8 ) )
       : fabs( *(float *)( wave + 8 ) + *(float *)( wave + 4 ) );
}

/* ---- ParseTexMod  0x004F1B40 ----  [HIGH] */
void __cdecl ParseTexMod(int bundleIndex, textureBundle_t *bundleBase, char *data_p)
{
  textureBundle_t *bundle;
  texModInfo_t    *tm;
  int             *numTexMods;
  int              image;
  char            *token;
  char           **text = &data_p;

  bundle     = &bundleBase[bundleIndex];
  numTexMods = &bundleBase[bundleIndex].numTexMods;
  if ( *numTexMods == 4 )
  {
    ri.Error( 1, "\x15" "ERROR: too many tcMod stages in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
    return;
  }
  tm    = &bundle->texMods[*numTexMods];
  token = Com_ParseOnLine( text );
  image = (int)bundle->image[0];
  if ( !image )
  {
    ri.Error( 1, "\x15" "ERROR: tcMod stage before texture in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
    return;
  }

  
  if ( *(int *)( image + 84 ) == 34067 )        
  {
    if ( !Q_stricmp( token, "negate" ) || !Q_stricmp( token, "reverse" ) )
      goto invertCube;
    if ( !Q_stricmp( token, "bumpmapFrame" ) )
    {
      tm->type = 10;
      (*(int *)(tr_shaderParseScratch + 0x4450)) |= 2u;
      goto modifierComplete;
    }
    goto unknown;
invertCube:
    tm->type = 9;
    goto modifierComplete;
  }

  {
    if ( !Q_stricmp( token, "swap" ) )
    {
      tm->type = 8;
      goto modifierComplete;
    }
    if ( !Q_stricmp( token, "turb" ) )
    {
      token = Com_ParseOnLine( text );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing tcMod turb parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
        return;
      }
      tm->wave.base = atof( token );
      token = Com_ParseOnLine( text );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing tcMod turb in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
        return;
      }
      tm->wave.amplitude = atof( token );
      token = Com_ParseOnLine( text );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing tcMod turb in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
        return;
      }
      tm->wave.phase = atof( token );
      token = Com_ParseOnLine( text );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing tcMod turb in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
        return;
      }
      tm->wave.frequency = atof( token );
      tm->type = 2;
      (*(int *)(tr_shaderParseScratch + 0x4450)) |= 1u;
      goto modifierComplete;
    }
  }

  
  if ( !Q_stricmp( token, "scale" ) )
  {
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing scale parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->scale[0] = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing scale parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->scale[1] = atof( token );
    tm->type = 4;
    goto modifierComplete;
  }
  if ( !Q_stricmp( token, "scroll" ) )
  {
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing scale scroll parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->scroll[0] = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing scale scroll parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->scroll[1] = atof( token );
    tm->type = 3;
    (*(int *)(tr_shaderParseScratch + 0x4450)) |= 1u;
    goto modifierComplete;
  }
  if ( !Q_stricmp( token, "stretch" ) )
  {
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing stretch parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->wave.func = NameToGenFunc( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing stretch parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->wave.base = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing stretch parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->wave.amplitude = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing stretch parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->wave.phase = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing stretch parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->wave.frequency = atof( token );
    tm->type = 5;
    (*(int *)(tr_shaderParseScratch + 0x4450)) |= 1u;
    goto modifierComplete;
  }
  if ( !Q_stricmp( token, "transform" ) )
  {
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing transform parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->matrix[0][0] = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing transform parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->matrix[0][1] = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing transform parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->matrix[1][0] = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing transform parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->matrix[1][1] = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing transform parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->translate[0] = atof( token );
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing transform parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->translate[1] = atof( token );
    tm->type = 1;
    goto modifierComplete;
  }
  if ( !Q_stricmp( token, "rotate" ) )
  {
    token = Com_ParseOnLine( text );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing tcMod rotate parms in shader '%s'\n", ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
      return;
    }
    tm->rotateSpeed = atof( token );
    tm->type = 6;
    (*(int *)(tr_shaderParseScratch + 0x4450)) |= 1u;
    goto modifierComplete;
  }
  if ( !Q_stricmp( token, "entityTranslate" ) )
  {
    tm->type = 7;
    goto modifierComplete;
  }

unknown:
  ri.Printf( 2, "WARNING: unknown or invalid tcMod '%s' in shader '%s'\n", token, ((unsigned char *)(tr_shaderParseScratch + 0x4400)) );
modifierComplete:
  ++*numTexMods;
}

/* ---- ParseTexEnvCombineFunction  0x004F20A0 ----  VERIFIED */
int __cdecl ParseTexEnvCombineFunction(ShaderTexEnvEquation *func, int isAlpha, char **data_p)
{
  char  *token;
  int    numArgs;
  int    argIndex;
  int    oneMinus;
  char   expected[2];

  expected[0] = 0;
  expected[1] = 0;

  if ( !MatchShaderTokenOnLine( data_p, "=", "texEnvCombine funxtion" ) )
    return 0;
  token = Com_ParseOnLine( data_p );
  if ( !*token )
  {
    ri.Printf( 2, "WARNING: missing equation name in texEnvCombine function in shader '%s'\n", shaderParseName );
    return 0;
  }

  if ( !Q_stricmpn( token, "REPLACE", 99999 ) )
  {
    func->function = 7681;                 
    numArgs = 1;
  }
  else if ( !Q_stricmpn( token, "MODULATE", 99999 ) )
  {
    func->function = 8448;                 
    numArgs = 2;
  }
  else if ( !Q_stricmpn( token, "ADD", 99999 ) )
  {
    func->function = 260;                  
    numArgs = 2;
  }
  else if ( !Q_stricmp( token, "ADD_SIGNED_ARB" ) )
  {
    func->function = 34164;
    numArgs = 2;
  }
  else if ( !Q_stricmp( token, "SUBTRACT_ARB" ) )
  {
    func->function = 34023;
    numArgs = 2;
  }
  else if ( !Q_stricmp( token, "INTERPOLATE_ARB" ) )
  {
    func->function = 34165;
    numArgs = 3;
  }
  else if ( !Q_stricmp( token, "DOT3_RGB_ARB" ) )
  {
    if ( !rendererShaderRequirements[9] )
    {
      ri.Printf( 2, "WARNING: shader '%s' uses DOT3_RGB_ARB without 'requires GL_ARB_texture_env_dot3'\n", shaderParseName );
      return 0;
    }
    if ( isAlpha )
    {
      ri.Printf( 2, "WARNING: DOT3_RGB_ARB only valid for rgb channel\n" );
      return 0;
    }
    func->function = 34478;
    numArgs = 2;
  }
  else if ( !Q_stricmp( token, "DOT3_RGBA_ARB" ) )
  {
    if ( !rendererShaderRequirements[9] )
    {
      ri.Printf( 2, "WARNING: shader '%s' uses DOT3_RGBA_ARB without 'requires GL_ARB_texture_env_dot3'\n", shaderParseName );
      return 0;
    }
    if ( isAlpha )
    {
      ri.Printf( 2, "WARNING: DOT3_RGBA_ARB only valid for rgb channel\n" );
      return 0;
    }
    func->function = 34479;
    numArgs = 2;
  }
  else
  {
    ri.Printf( 2, "WARNING: unknown equation name '%s' in texEnvCombine in shader '%s'\n", token, shaderParseName );
    return 0;
  }

  if ( !MatchShaderTokenOnLine( data_p, "(", "texEnvCombine function" ) )
    return 0;

  for ( argIndex = 0; argIndex < numArgs; argIndex++ )
  {
    
    token = Com_ParseOnLine( data_p );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing argument in texEnvCombine function in shader '%s'\n", shaderParseName );
      return 0;
    }

    oneMinus = 0;
    if ( !Q_stricmpn( token, "1", 99999 ) )
    {
      oneMinus = 1;
      if ( !MatchShaderTokenOnLine( data_p, "-", "after 1 for 1 - source in texEnvCombine function:" ) )
        return 0;
      if ( argIndex == 2 )
      {
        ri.Printf( 2, "WARNING: the third argument cannot be 1 - source for texEnvCombine function in shader '%s'\n", shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing source for 1 - source in texEnvCombine function in shader '%s'\n", shaderParseName );
        return 0;
      }
    }

    if ( !token[1] || token[2] )         
    {
      ri.Printf( 2, "WARNING: '%s' is not a valid source for texEnvCombine function in shader '%s'\n", token, shaderParseName );
      return 0;
    }
    if ( token[0] == 'C' )
    {
      if ( isAlpha )
      {
        ri.Printf( 2, "WARNING: '%s' is not a valid source for texEnvCombine alpha function in shader '%s'\n", token, shaderParseName );
        return 0;
      }
      if ( argIndex == 2 )
      {
        ri.Printf( 2, "WARNING: the third argument must be an alpha for texEnvCombine function in shader '%s'\n", shaderParseName );
        return 0;
      }
      func->operand[argIndex] = ( oneMinus != 0 ) + 768;    
    }
    else
    {
      if ( token[0] != 'A' )
      {
        ri.Printf( 2, "WARNING: '%s' is not a valid source for texEnvCombine function in shader '%s'\n", token, shaderParseName );
        return 0;
      }
      func->operand[argIndex] = ( oneMinus != 0 ) + 770;    
    }
    if ( token[1] == 't' )
      func->source[argIndex] = 5890;             /* GL_TEXTURE */
    else if ( token[1] == 'c' )
      func->source[argIndex] = 34166;            /* GL_CONSTANT_ARB */
    else if ( token[1] == 'f' )
      func->source[argIndex] = 34167;            /* GL_PRIMARY_COLOR_ARB */
    else if ( token[1] == 'p' )
      func->source[argIndex] = 34168;            /* GL_PREVIOUS_ARB */
    else
    {
      ri.Printf( 2, "WARNING: '%s' is not a valid source for texEnvCombine function in shader '%s'\n", token, shaderParseName );
      return 0;
    }

    expected[0] = ( argIndex + 1 == numArgs ) ? ')' : ',';
    if ( !MatchShaderTokenOnLine( data_p, expected, "texEnvCombine function" ) )
      return 0;
  }

  
  func->scale = 1.0f;
  token = Com_ParseOnLine( data_p );
  if ( token[0] )
  {
    char  *scale;
    float scaleVal;

    if ( token[0] != '*' )
    {
      ri.Printf( 2, "WARNING: unknown token '%s' following texEnvCombine function in shader '%s'\n", token, shaderParseName );
      return 0;
    }
    scale = Com_ParseOnLine( data_p );
    if ( !*scale )
    {
      ri.Printf( 2, "WARNING: missing scale for texEnvCombine function in shader '%s'\n", shaderParseName );
      return 0;
    }
    scaleVal = atof( scale );
    func->scale = scaleVal;
    if ( scaleVal != 1.0f && scaleVal != 2.0f && scaleVal != 4.0f )
    {
      ri.Printf( 2, "WARNING: bad scale '%s' for texEnvCombine function in shader '%s'\n", scale, shaderParseName );
      return 0;
    }
  }
  return 1;
}

/* ---- ParseTextureEnvCombine  0x004F2630 ----  [HIGH] */
int __cdecl ParseTextureEnvCombine(ShaderTextureImageData *bundle, char **data_p)
{
  ShaderTexEnvCombine *tex;                 
  char *token;

  bundle->texEnvMode = 34160;                 
  tex = (ShaderTexEnvCombine *)ri.Hunk_Alloc( 80 );
  bundle->combine = tex;
  bundle->combine->rgb.function   = 7681;                
  bundle->combine->rgb.scale = 1.0f;                
  bundle->combine->rgb.source[0]   = 5890;               
  bundle->combine->rgb.source[1]   = 5890;
  bundle->combine->rgb.source[2]   = 5890;
  bundle->combine->rgb.operand[0]   = 768;                
  bundle->combine->rgb.operand[1]   = 768;
  bundle->combine->rgb.operand[2]   = 770;                
  bundle->combine->alpha.function   = 7681;               
  bundle->combine->alpha.scale = 1.0f;               
  bundle->combine->alpha.source[0]   = 5890;               
  bundle->combine->alpha.source[1]   = 5890;
  bundle->combine->alpha.source[2]   = 5890;
  bundle->combine->alpha.operand[0]   = 770;                
  bundle->combine->alpha.operand[1]   = 770;
  bundle->combine->alpha.operand[2]   = 770;

  if ( !MatchShaderToken( data_p, "{", "texEnvCombine" ) )
    return 0;

  
  Com_SkipRestOfLine( data_p );

  bundle->combine->color[0]  = 1.0f;               
  bundle->combine->color[1]  = 1.0f;
  bundle->combine->color[2]  = 1.0f;
  bundle->combine->color[3] = 1.0f;

  token = Com_Parse( data_p );

  if ( *token && !Q_stricmpn( token, "const", 99999 ) )
  {
    char *g, *b;

    if ( !MatchShaderTokenOnLine( data_p, "=", "texEnvCombine const" )
      || !MatchShaderTokenOnLine( data_p, "(", "texEnvCombine const" ) )
      return 0;
    token = Com_ParseOnLine( data_p );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing color in texEnvCombine const in shader '%s'\n", shaderParseName );
      return 0;
    }
    bundle->combine->color[0] = atof( token );
    g = Com_ParseOnLine( data_p );
    if ( !*g )
    {
      ri.Printf( 2, "WARNING: truncated color in texEnvCombine const in shader '%s'\n", shaderParseName );
      return 0;
    }
    bundle->combine->color[1] = atof( g );
    b = Com_ParseOnLine( data_p );
    if ( !*b )
    {
      ri.Printf( 2, "WARNING: truncated color in texEnvCombine const in shader '%s'\n", shaderParseName );
      return 0;
    }
    bundle->combine->color[2] = atof( b );

    token = Com_ParseOnLine( data_p );
    if ( *token && *token != ')' )
    {
      bundle->combine->color[3] = atof( token );      
      token = Com_Parse( data_p );
    }
    if ( *token != ')' )
    {
      ri.Printf( 2, "WARNING: missing ')' in texEnvCombine const in shader '%s'\n", shaderParseName );
      return 0;
    }

    
    token = Com_ParseOnLine( data_p );
    if ( *token )
    {
      if ( *token != '*' )
      {
        ri.Printf( 2, "WARNING: unexpected '%s' in texEnvCombine const in shader '%s'\n", token, shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( Q_stricmp( token, "identityLighting" ) )
      {
        ri.Printf( 2, "WARNING: expected 'identityLighting' in texEnvCombine const in shader '%s'\n", shaderParseName );
        return 0;
      }
      bundle->combine->color[0] = tr_identityLight * bundle->combine->color[0];
      bundle->combine->color[1] = tr_identityLight * bundle->combine->color[1];
      bundle->combine->color[2] = tr_identityLight * bundle->combine->color[2];
    }
    token = Com_Parse( data_p );
  }

  if ( !*token || Q_stricmpn( token, "rgb", 99999 ) )
  {
    ri.Printf( 2, "WARNING: missing 'rgb' equation in texEnvCombine in shader '%s'\n", shaderParseName );
    return 0;
  }
  if ( !ParseTexEnvCombineFunction( &bundle->combine->rgb, 0, data_p ) )
    return 0;

  token = Com_Parse( data_p );
  if ( *token && !Q_stricmp( token, "alpha" ) )
  {
    if ( bundle->combine->rgb.function == 34479 )         
    {
      ri.Printf( 2, "WARNING: alpha not allowed in texEnvCombine if rgb is DOT3_RGBA_ARB in shader '%s'\n", shaderParseName );
      return 0;
    }
    if ( !ParseTexEnvCombineFunction( &bundle->combine->alpha, 1, data_p ) )
      return 0;
    token = Com_Parse( data_p );
  }
  else
  {
    
    bundle->combine->alpha.function   = 7681;               
    bundle->combine->alpha.scale = 1.0f;
    bundle->combine->alpha.source[0]   = 5890;               
    bundle->combine->alpha.operand[0]   = 770;                
  }

  if ( *token != '}' )
  {
    ri.Printf( 2, "WARNING: missing '}' in texEnvCombine in shader '%s'\n", shaderParseName );
    return 0;
  }
  return 1;
}

typedef struct shaderRequiresOperand_s
{
  int    kind;         /* +0x00  0 unresolved, 1 non-numeric, 2 numeric      */
  short  dependency;   /* +0x04  index into rendererShaderRequirements, -1 none */
  short  notCount;     /* +0x06  count of leading '!' -- only bit 0 is read   */
  char   value[248];   /* +0x08  strncpy'd 247 + NUL                          */
} shaderRequiresOperand_t;

/* ---- ParseStageRequirementsOperand  0x004F2A90 ----  [HIGH] */
int __cdecl ParseStageRequirementsOperand(char **data_p, shaderRequiresOperand_t *op)
{
  const char *string;
  int         digitBase;
  int         len;

  op->kind = 0;
  op->dependency = -1;
  op->notCount = 0;

nextToken:
  string = Com_ParseOnLine( data_p );
  if ( !*string )
    return 0;

  if ( *string == '!' )
  {
    while ( ++op->notCount != 0 )      
    {
      ++string;
      if ( *string != '!' )
      {
        if ( !*string )
          goto nextToken;              
        goto processString;
      }
    }
    return 0;                          
  }

processString:
  if ( !Q_stricmpn( string, "GL_MAX_TEXTURE_UNITS_ARB", 99999 ) )
  {
    op->dependency = 0;
    op->kind = 2;
    if ( qgls_ActiveTextureARB )
    {
      _itoa( glConfig.maxActiveTextures, op->value, 10 );
      string = op->value;
    }
    else
      string = "1";
  }
  else if ( !Q_stricmpn( string, "GL_ARB_texture_cube_map", 99999 ) )
  {
    op->dependency = 6;
    op->kind = 2;
    _itoa( glConfig.textureCubeMap, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmpn( string, "GL_ARB_texture_env_add", 99999 ) )
  {
    op->dependency = 7;
    op->kind = 2;
    _itoa( glConfig.textureEnvAddAvailable, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmpn( string, "GL_ARB_texture_env_combine", 99999 ) )
  {
    op->dependency = 8;
    op->kind = 2;
    _itoa( glConfig.textureEnvCombine, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmpn( string, "GL_ARB_texture_env_dot3", 99999 ) )
  {
    op->dependency = 9;
    op->kind = 2;
    _itoa( glConfig.textureEnvDot3, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmp( string, "GL_ARB_vertex_program" ) )
  {
    op->dependency = 10;
    op->kind = 2;
    _itoa( glConfig.ARBVertexProgram, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmp( string, "GL_NV_register_combiners" ) )
  {
    op->dependency = 11;
    op->kind = 2;
    _itoa( glConfig.NVRegisterCombiners >= 1, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmp( string, "GL_NV_register_combiners2" ) )
  {
    op->dependency = 12;
    op->kind = 2;
    _itoa( glConfig.NVRegisterCombiners >= 2, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmp( string, "GL_NV_texture_shader" ) )
  {
    op->dependency = 13;
    op->kind = 2;
    _itoa( glConfig.NVTextureShader, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmp( string, "GL_ATI_fragment_shader" ) )
  {
    op->dependency = 14;
    op->kind = 2;
    _itoa( glConfig.ATIFragmentShader, op->value, 10 );
    string = op->value;
  }
  else if ( !Q_stricmp( string, "cvar" ) )
  {
    char *name = Com_ParseOnLine( data_p );
    cvar_t *cv;
    if ( !isalpha( *name ) && *name != '_' )
      return 0;
    cv = ri.Cvar_Get( name, "", 32 );
    if ( !Q_stricmp( cv->name, "BADNAME" ) )
      return 0;
    string = cv->string;
  }

haveValue:
  digitBase = 0;
  len = strlen( string );
  if ( !len )
  {
    op->kind = 1;                       
    goto finalize;
  }
  op->kind = 2;
  if ( *string == '-' || *string == '+' )
    digitBase = 1;
  for ( ; digitBase < len; ++digitBase ) {
    if (!isdigit(string[digitBase]) && string[digitBase] != '.') {
      op->kind = 1;
      break;
    }
  }

finalize:
  if ( op->notCount )
  {
    op->kind = 2;
    op->value[1] = 0;
    if (op->notCount & 1)
      op->value[0] = atof(string) == 0.0 ? '1' : '0';
    else
      op->value[0] = atof(string) != 0.0 ? '1' : '0';
  }
  else
  {
    strncpy( op->value, string, 0xF7u );
    op->value[247] = 0;
  }
  return 1;
}

/* ---- UpdateRequiresCondition  0x004F2EB0 ----  VERIFIED */
void __cdecl UpdateRequiresCondition(shaderRequiresOperand_t *depOp, shaderRequiresOperand_t *valOp, int op)
{
  shaderRequiresOperand_t *swap;
  float val;
  float  valF;
  int    depIndex;
  int    units;

  

  /* Mac 1.3: swap the parameters in place when the first is not the dependency */
  if ( depOp->dependency < 0 )
  {
    if ( valOp->dependency < 0 )
      return;
    swap  = depOp;
    depOp = valOp;
    valOp = swap;
    op    = tr_texEnvArgSwap[op];
  }
  else if ( valOp->dependency >= 0 )
    return;

  if ( valOp->kind != 2 )
    return;
  val  = atof( valOp->value );
  valF = (float)val;
  if ( (float)(int)val != valF )       
    return;

  depIndex = depOp->dependency;
  if (depIndex != 0) {
    switch (op) {
      case 0:
        if (depOp->notCount & 1) {
          if (valF != 0.0f) return;
        } else if (valF == 0.0f) return;
        break;
      case 1:
        if (depOp->notCount & 1) {
          if (valF == 0.0f) return;
        } else if (valF != 0.0f) return;
        break;
      default: return;
    }
    rendererShaderRequirements[depIndex] = 1;
    return;
  }

  if (depOp->notCount) return;
  switch (op) {
    case 0: case 5:
      for (units = 3; units <= 8; ++units) {
        if ((float)units == valF) {
          rendererShaderRequirements[units-3] = 1;
          return;
        }
      }
      return;
    case 4:
      for (units = 3; units <= 8; ++units) {
        if ((float)(units-1) == valF) {
          rendererShaderRequirements[units-3] = 1;
          return;
        }
      }
      return;
    default: return;
  }
}

/* ---- ParseStageRequirements  0x004F3000 ----  [HIGH] */
qboolean __cdecl ParseStageRequirements(char **data_p, qboolean fromOr)
{
  shaderRequiresOperand_t op1;
  shaderRequiresOperand_t op2;
  char *op;
  char  opStr[8];

  if ( !ParseStageRequirementsOperand( data_p, (shaderRequiresOperand_t *)&op1 ) )
  {
    ri.Error( 1, "\x15" "^1bad or missing arguments to 'requires' in shader '%s'\n", shaderParseName );
    return 1;
  }

  op = Com_ParseOnLine( data_p );
  if ( *op && strcmp( op, "||" ) )
  {
    int    cmpCode;
    qboolean result;
    long double val1;
    long double val2;
    char  *next;

    if ( strlen( op ) > 7 )
    {
      /* retail (and Mac 1.3) pass only shaderParseName although the format has two %s */
      ri.Error( 1, "\x15" "^1bad operator '%s' in 'requires' in shader '%s'\n", shaderParseName );
      return 1;
    }
    strcpy( opStr, op );
    if ( !ParseStageRequirementsOperand( data_p, (shaderRequiresOperand_t *)&op2 ) )
    {
      ri.Error( 1, "\x15" "^1bad or missing second operand to 'requires' in shader '%s'\n", shaderParseName );
      return 1;
    }
    if ( op1.kind != op2.kind )
      ri.Printf( 2, "WARNING: operands to 'requires' in shader '%s' have different types, comparing as strings\n", shaderParseName );

    if ( !strcmp( opStr, "==" ) )
    {
      cmpCode = 0;
      val1 = atof( op1.value );
      val2 = atof( op2.value );
      result = ( val2 == val1 );
    }
    else if ( !strcmp( opStr, "!=" ) )
    {
      cmpCode = 1;
      val1 = atof( op1.value );
      val2 = atof( op2.value );
      result = ( val2 != val1 );
    }
    else if ( !strcmp( opStr, ">" ) )
    {
      cmpCode = 4;
      val1 = atof( op1.value );
      val2 = atof( op2.value );
      result = ( val2 < val1 );
    }
    else if ( !strcmp( opStr, ">=" ) )
    {
      cmpCode = 5;
      val1 = atof( op1.value );
      val2 = atof( op2.value );
      result = ( val2 <= val1 );
    }
    else if ( !strcmp( opStr, "<" ) )
    {
      cmpCode = 2;
      val1 = atof( op1.value );
      val2 = atof( op2.value );
      result = ( val2 > val1 );
    }
    else if ( !strcmp( opStr, "<=" ) )
    {
      cmpCode = 3;
      val1 = atof( op1.value );
      val2 = atof( op2.value );
      result = ( val2 >= val1 );
    }
    else
    {
      ri.Error( 1, "\x15" "^1unknown operator '%s' in 'requires' for shader '%s'\n", opStr, shaderParseName );
      return 1;
    }

    next = Com_ParseOnLine( data_p );
    if ( *next && !strcmp( next, "||" ) )
      result |= ParseStageRequirements( data_p, 1 );
    else if ( fromOr == qfalse )
      UpdateRequiresCondition( (shaderRequiresOperand_t *)&op1, (shaderRequiresOperand_t *)&op2, cmpCode );
    return result;
  }

  {
    qboolean single = ( atof( op1.value ) != 0.0 );
    if ( *op )
      single |= ParseStageRequirements( data_p, 1 );
    else if ( !fromOr && op1.dependency >= 0 && op1.dependency && op1.dependency != 1 && !( op1.notCount & 1 ) )
      rendererShaderRequirements[op1.dependency] = 1;
    return single;
  }
}

/* ---- ParseImage  0x004F3410 ----  [HIGH] */
int __cdecl ParseImage(char **data_p, int allowTextureName, int loadImage, int imageTrack, int colorScale, int *outImage)
{
  unsigned int flags;
  char        *token;
  char        *sub;
  qboolean     only16bit;
  qboolean     only32bit;
  float        heightmapScale;

  *outImage = 0;
  flags = ( (unsigned int)shaderParseFlags >> 7 ) & 4;
  if ( ( shaderParseFlags & 0x100 ) == 0 )
    flags |= 2u;
  if ( ((unsigned int)shaderParseFlags & 0x80u) == 0 )
    flags |= 1u;
  only16bit = 0;
  only32bit = 0;
  heightmapScale = 1.0f;

  
  for ( ;; )
  {
    token = Com_ParseOnLine( data_p );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing image name in shader %s\n", shaderParseName );
      return 0;
    }

    if ( !Q_stricmp( token, "nopicmip" ) )  { flags &= ~2u; continue; }
    if ( !Q_stricmp( token, "picmip" ) )    { flags |= 2u;  continue; }
    if ( !Q_stricmp( token, "nomipmaps" ) ) { flags &= ~1u; continue; }
    if ( !Q_stricmp( token, "mipmaps" ) )   { flags |= 1u;  continue; }
    if ( !Q_stricmp( token, "clamp" ) )     { flags |= 0x30u; continue; }
    if ( !Q_stricmp( token, "clampx" ) || !Q_stricmp( token, "clamps" ) )
    {
      flags |= 0x10u;
      continue;
    }
    if ( !Q_stricmp( token, "clampy" ) || !Q_stricmp( token, "clampt" ) )
    {
      flags |= 0x20u;
      continue;
    }
/* retail's inliner stops expanding Q_stricmp / Com_ParseOnLine here (its frequency-ordered
 * inline budget runs out); the tree's would inline every call, so the cold tail of the loop is
 * compiled with inlining off and the post-loop name checks with it back on. */
#pragma inline_depth(0)
    if ( !Q_stricmp( token, "heightToNormal" ) )
    {
      char  *scaleTok;

      flags |= 0x40u;
      scaleTok = Com_ParseOnLine( data_p );
      heightmapScale = atof( scaleTok );   
      if ( heightmapScale == 0.0f )              
      {
        Com_UngetToken();
        heightmapScale = 1.0f;
      }
      continue;
    }
    if ( !Q_stricmp( token, "only16bit" ) ) { only16bit = 1; continue; }
    if ( !Q_stricmp( token, "only32bit" ) ) { only32bit = 1; continue; }
    if ( !Q_stricmp( token, "noopt" ) )     { flags |= 0x100u; continue; }
    break;
  }

#pragma inline_depth()
  if ( !Q_stricmp( token, "$lightmap" ) )
  {
    if ( colorScale )
    {
      ri.Printf( 2, "WARNING: shader '%s' tried to use multiplyImage with image '%s'\n", shaderParseName, token );
      return 0;
    }
    shaderParseSurfaceFlags |= 0x20u;
    if ( shaderParseLightmapIndex < 0 )
      *outImage = tr_identityLightImage;
    else
      *outImage = tr_lightmaps[shaderParseLightmapIndex];
    return 1;
  }

  if ( !Q_stricmp( token, "$whiteimage" ) || !Q_stricmp( token, "*white" ) )
  {
    if ( colorScale )
    {
      ri.Printf( 2, "WARNING: shader '%s' tried to use multiplyImage with image '%s'\n", shaderParseName, token );
      return 0;
    }
    shaderParseSurfaceFlags |= 0x10u;
    *outImage = tr_whiteImage;
    return 1;
  }
  if ( !Q_stricmp( token, "$dlight" ) )
  {
    if ( colorScale )
    {
      ri.Printf( 2, "WARNING: shader '%s' tried to use multiplyImage with image '%s'\n", shaderParseName, token );
      return 0;
    }
    *outImage = tr_dlightImage;
    return 1;
  }
#pragma inline_depth(0)
  if ( !Q_stricmp( token, "$screen" ) )
  {
    if ( colorScale )
    {
      ri.Printf( 2, "WARNING: shader '%s' tried to use multiplyImage with image '%s'\n", shaderParseName, token );
      return 0;
    }
    *outImage = tr_screenImage;
    return 1;
  }

  if ( ( flags & 0x40 ) != 0 || !r_graymap->integer
    || shaderParseLightmapIndex == -4 || shaderParseLightmapIndex == -2 )
  {
    if ( !Q_stricmp( token, "$texturename" ) )
    {
      shaderParseSurfaceFlags |= 0x10u;
      if ( !allowTextureName )
      {
        ri.Printf( 2, "WARNING: $texturename used in shader '%s', which is not a shader type file\n", shaderParseName );
        return 0;
      }
      sub = Com_ParseOnLine( data_p );
      if ( !Q_stricmp( sub, "+" ) )
        token = Com_ParseOnLine( data_p );
      else
        token = "";
      *outImage = R_FindImageFile( token, 3553, flags, 10, (const float *)colorScale, heightmapScale );
    }
    
  }
  else
  {
    shaderParseSurfaceFlags |= 0x10u;
    *outImage = tr_grayImage;
  }

  
  if ( only16bit )
  {
    if ( only32bit )
    {
      ri.Printf( 2, "WARNING: only16bit and only32bit are mutually exclusive in shader %s\n", shaderParseName );
      return 0;
    }
    if ( glConfig.colorBits > 16 )
      return 1;
  }
  else if ( only32bit && glConfig.colorBits <= 16 )
  {
    return 1;
  }

  if ( loadImage && !*outImage )
  {
    if ( (*(int *)(((char *)(void *)&tr_registered) + 0x0128)) && shaderParseLightmapIndex == -1 )
      flags &= ~0x80u;
    *outImage = R_FindImageFile( token, 3553, flags, imageTrack, (const float *)colorScale, heightmapScale );
    if ( !*outImage )
    {
      ri.Printf( 2, "WARNING: Couldn't load image '%s' in shader %s\n", token, shaderParseName );
      return 0;
    }
  }
  return 1;
#pragma inline_depth()
}

/* ---- ParseWaterMapPositiveFloat  0x004F3940 ----  [HIGH] */
float __cdecl ParseWaterMapPositiveFloat(const char *paramName, char **data_p, float *outValue)
{
  char  *token;
  float val;

  token = Com_ParseOnLine( data_p );
  if ( !token[0] )
  {
    ri.Printf( 2, "WARNING: waterMap missing %s in shader %s\n", (const char *)paramName, shaderParseName );
    return 0.0f;
  }
  val = atof( token );
  *outValue = val;
  if ( val <= 0.0f )
  {
    ri.Printf( 2, "WARNING: %s must be > 0 in waterMap in shader %s\n", (const char *)paramName, shaderParseName );
    return 0.0f;
  }
  return 1.0f;
}

/* ---- ParseWaterMapFloat  0x004F39E0 ----  VERIFIED */
float __cdecl ParseWaterMapFloat(const char *paramName, char **data_p, float *outValue)
{
  char *token;

  token = Com_ParseOnLine( data_p );
  if ( !token[0] )
  {
  ri.Printf( 2, "WARNING: waterMap missing %s in shader %s\n", paramName, shaderParseName );
  return 0.0f;
  }
  *outValue = atof(token);
  return 1.0f;
}

/* ---- ParseWaterMapInt  0x004F3A60 ----  [HIGH] */
int __cdecl ParseWaterMapInt(const char *paramName, char **data_p, int minValue, int maxValue, int *outValue)
{
  char *token;
  int   value;

  token = Com_ParseOnLine( data_p );
  if ( !token[0] )
  {
    ri.Printf( 2, "WARNING: waterMap missing %s in shader %s\n", paramName, shaderParseName );
    return 0;
  }
  value = j__atol( token );
  *outValue = value;
  if ( value < minValue || value > maxValue )
  {
    ri.Printf( 2, "WARNING: %s must be >= %i and <= %i in waterMap in shader %s\n", paramName, minValue, maxValue, shaderParseName );
    return 0;
  }
  if ( ( ( value - 1 ) & value ) != 0 )
  {
    ri.Printf( 2, "WARNING: %s must be a power of 2 in waterMap in shader %s\n", paramName, shaderParseName );
    return 0;
  }
  return 1;
}

/* ---- ParseWaterMap  0x004F3B20 ----  [HIGH] */
int __cdecl ParseWaterMap(textureBundle_t *bundles, char **data_p, int bundleIndex)
{
  waterMap_t *waterTexture;
  int   width;          
  int   height;         
  float horizLength;    
  float vertLength;     
  float windVelocity;   
  float amplitude;      
  float windDir[2];     
  ShaderWaterConfiguration config;     

  if ( !ParseWaterMapInt( "texture width", data_p, 4, 256, &width ) )
    return 0;
  if ( !ParseWaterMapInt( "texture width", data_p, 4, 256, &height ) )
    return 0;
  if ( ParseWaterMapPositiveFloat( "horizontal world length", data_p, &horizLength ) == 0.0f )
    return 0;
  if ( ParseWaterMapPositiveFloat( "vertical world length", data_p, &vertLength ) == 0.0f )
    return 0;
  if ( ParseWaterMapPositiveFloat( "wind velocity", data_p, &windVelocity ) == 0.0f )
    return 0;
  if ( ParseWaterMapFloat( "wind x direction", data_p, &windDir[0] ) == 0.0f )
    return 0;
  if ( ParseWaterMapFloat( "wind y direction", data_p, &windDir[1] ) == 0.0f )
    return 0;
  if ( VectorNormalize2D( windDir ) == 0.0f )
  {
    ri.Printf( 2, "WARNING: wind direction is 0 0 in waterMap in shader %s\n", shaderParseName );
    return 0;
  }
  if ( ParseWaterMapPositiveFloat( "amplitude", data_p, &amplitude ) == 0.0f )
    return 0;

  config.textureWidth  = width;
  config.textureHeight  = height;
  config.horizontalWorldLength  = horizLength;
  config.verticalWorldLength  = vertLength;
  config.gravity  = 800.0f;        
  config.windVelocity  = windVelocity;
  config.windDirection[0] = windDir[0];
  config.windDirection[1] = windDir[1];
  config.amplitude = amplitude;

  waterTexture = R_GetWaterTexture( &config );
  bundles[bundleIndex].waterTexture = waterTexture;
  if ( !waterTexture )
    return 0;
  bundles[bundleIndex].image[0] = waterTexture->image;
  return 1;
}

/* ---- ParseNVCullParm  0x004F3CF0 ----  [HIGH] */
int __cdecl ParseNVCullParm(char **data_p, int bundleIndex, int bundles, int paramIndex)
{
  const char *token;

  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    if ( parseInfo->spaceDelimited == qfalse )
    {
      token = parseInfo->token;               /* reuse the unget'd token */
    }
    else
    {
      *data_p = parseInfo->ungetTokenSave;
      parseInfo->currentLine = parseInfo->ungetLineSave;
      token = Com_ParseExt( data_p, qfalse );
    }
  }
  else
  {
    token = Com_ParseExt( data_p, qfalse );
  }

  if ( !_stricmp( token, "LESS_THAN_ZERO" ) )
  {
    *(int *)( *(int *)( 200 * bundleIndex + bundles + 156 ) + 4 * paramIndex + 12 ) = 513;
    return 1;
  }
  else if ( !_stricmp( token, "GEQUAL_TO_ZERO" ) )
  {
    *(int *)( *(int *)( 200 * bundleIndex + bundles + 156 ) + 4 * paramIndex + 12 ) = 518;
    return 1;
  }
  else
  {
    ri.Printf( 2, "WARNING: shader '%s' - cull argument must be 'LESS_THAN_ZERO' or 'GEQUAL_TO_ZERO'\n", shaderParseName );
    return 0;
  }
}

/* ---- ParseNVFloatParm  0x004F3DC0 ----  [HIGH] */
int __cdecl ParseNVFloatParm(char **data_p, int bundleIndex, int bundles, int paramIndex)
{
  const char *token;

  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    if ( parseInfo->spaceDelimited == qfalse )
    {
      token = parseInfo->token;               
    }
    else
    {
      *data_p = parseInfo->ungetTokenSave;
      parseInfo->currentLine = parseInfo->ungetLineSave;
      token = Com_ParseExt( data_p, qfalse );
    }
  }
  else
  {
    token = Com_ParseExt( data_p, qfalse );
  }

  if ( !token[0] )
  {
    ri.Printf(2, "WARNING: shader '%s' - missing value in 'nvTexShader'\n", shaderParseName);
    return 0;
  }
  *(float *)( *(int *)(200 * bundleIndex + bundles + 156) + 4 * paramIndex + 12 ) = atof(token);
  return 1;
}

/* ---- ParseNVTextureInput  0x004F3E50 ----  [HIGH] */
int __cdecl ParseNVTextureInput(int bundleIndex, textureBundle_t *bundles, char **data_p, int allowExpand)
{
  textureBundle_t *bundle;
  ShaderNVTextureOperation *prevOp;
  ShaderNVTextureOperation *prevPrevOp;
  unsigned int op;
  char *token;
  int texIndex;

  if ( !bundleIndex )
  {
    ri.Printf( 2, "WARNING: texture op in 'nvTexShader' invalid for first bundle in shader %s\n", shaderParseName );
    return 0;
  }

  bundle = &bundles[bundleIndex];
  prevOp = bundles[bundleIndex - 1].nvTextureShader;

  if ( !prevOp || prevOp->operation != 34540 )
  {
    switch ( bundle->nvTextureShader->operation )
    {
      case 0x86F2:
      case 0x86F3:
        if ( prevOp && prevOp->operation == 34545 )
          break;
        ri.Printf( 2, "WARNING: bundle %i (from 0) must use 'nvTexShader dot3' or 'nvTexShader dot3_diffuse_cubemap' in shader %s\n", bundleIndex - 1, shaderParseName );
        return 0;
      case 0x86E9:
      case 0x86EA:
      case 0x86EC:
        break;
      default:
        ri.Printf( 2, "WARNING: bundle %i (from 0) must use 'nvTexShader dot3' in shader %s\n", bundleIndex - 1, shaderParseName );
        return 0;
    }
  }

  op = bundle->nvTextureShader->operation;
  if ( op < 0x86F2
    || op > 0x86F3
    || bundleIndex >= 2 && (prevPrevOp = bundle[-2].nvTextureShader) != 0 && prevPrevOp->operation == 34540 )
  {
    token = Com_ParseOnLine( data_p );
    if ( allowExpand )
    {
      bundle->nvTextureShader->inputMapping = 34102;
      if ( !_stricmp( token, "expand" ) )
      {
        bundle->nvTextureShader->inputMapping = 34104;
        token = Com_ParseOnLine( data_p );
      }
    }
    if ( !_strnicmp( token, "tex", 3u ) && isdigit( token[3] ) && (texIndex = token[3], texIndex - 48 < bundleIndex) && !token[4] )
    {
      bundle->nvTextureShader->textureSource = texIndex + 33936;
      return 1;
    }
    else
    {
      ri.Printf(
        2,
        "WARNING: input source to texture op in 'nvTexShader' in shader %s must be tex0%s\n",
        shaderParseName,
        va( " to tex%i", bundleIndex ) );
      return 0;
    }
  }
  else
  {
    ri.Printf( 2, "WARNING: bundle %i (from 0) must use 'nvTexShader dot3' in shader %s\n", bundleIndex - 2, shaderParseName );
    return 0;
  }
}

/* ---- ParseNVTexShaderArgs  0x004F4010 ----  [HIGH] */
int __cdecl ParseNVTexShaderArgs(char **data_p, textureBundle_t *bundles, int bundleIndex, int argCount, const int *argTypes)
{
  int i;
  int argType;

  if ( !MatchShaderTokenOnLine( data_p, "(", "nvTexShader function" ) )
    return 0;

  for ( i = 0; i < argCount; i++ )
  {
    if ( i != 0 && !MatchShaderTokenOnLine( data_p, ",", "nvTexShader argument list" ) )
      return 0;

    argType = argTypes[i];
    switch ( argType )
    {
      case 0:
        if ( !ParseNVTextureInput( bundleIndex, bundles, data_p, 0 ) )
          return 0;
        break;
      case 1:
        if ( !ParseNVTextureInput( bundleIndex, bundles, data_p, 1 ) )
          return 0;
        break;
      case 2:
      case 3:
      case 4:
      case 5:
        if ( !ParseNVCullParm( data_p, bundleIndex, (int)bundles, argType - 2 ) )
          return 0;
        break;
      case 6:
      case 7:
      case 8:
      case 9:
        if ( !ParseNVFloatParm( data_p, bundleIndex, (int)bundles, argType - 6 ) )
          return 0;
        break;
      default:
        break;
    }
  }

  return MatchShaderTokenOnLine( data_p, ")", "nvTexShader function" ) != 0;
}

/* ---- ParseNVTexShader  0x004F4140 ----  VERIFIED */
int __cdecl ParseNVTexShader(textureBundle_t *bundles, char **data_p, int bundleIndex)
{
  ShaderNVTextureOperation **shaderSlot;
  const char *token;

  if ( !rendererShaderRequirements[13] )
  {
    ri.Printf( 2, "WARNING: used nvTexShader without 'requires GL_NV_texture_shader' in shader '%s'\n", shaderParseName );
    return 0;
  }

  shaderSlot = &bundles[bundleIndex].nvTextureShader;
  *shaderSlot = (ShaderNVTextureOperation *)ri.Hunk_Alloc( sizeof(ShaderNVTextureOperation) );

  token = Com_ParseOnLine( data_p );
  if ( !token[0] )
  {
    ri.Printf( 2, "WARNING: missing arguments to nvTexShader in shader '%s'\n", shaderParseName );
    return 0;
  }

  if ( !_stricmp( token, "texture_2d" ) )
  {
    (*shaderSlot)->operation = 3553;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 0, 0 );
  }
  if ( !_stricmp( token, "texture_cube_map" ) )
  {
    (*shaderSlot)->operation = 34067;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 0, 0 );
  }
  if ( !_stricmp( token, "cull_fragment" ) )
  {
    int args[4] = { 2, 3, 4, 5 };

    (*shaderSlot)->operation = 34535;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 4, args );
  }
  if ( !_stricmp( token, "pass_through" ) )
  {
    (*shaderSlot)->operation = 34534;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 0, 0 );
  }
  if ( !_stricmp( token, "dependent_ar" ) )
  {
    int args[1] = { 0 };

    (*shaderSlot)->operation = 34537;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 1, args );
  }
  if ( !_stricmp( token, "dependent_gb" ) )
  {
    int args[1] = { 0 };

    (*shaderSlot)->operation = 34538;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 1, args );
  }
  if ( !_stricmp( token, "dot_product_2d_1of2" )
    || !_stricmp( token, "dot_product_cube_map_1of3" )
    || !_stricmp( token, "dot_product_cube_map_2of3" )
    || !_stricmp( token, "dot_product_depth_replace_1of2" )
    || !_stricmp( token, "dot_product_reflect_cube_map_const_eye_1of3" )
    || !_stricmp( token, "dot_product_reflect_cube_map_const_eye_2of3" )
    || !_stricmp( token, "dot_product_reflect_cube_map_eye_from_qs_1of3" )
    || !_stricmp( token, "dot_product_reflect_cube_map_eye_from_qs_2of3" )
    || !_stricmp( token, "dot_product_cube_map_and_reflect_cube_map_const_eye_1of3" )
    || !_stricmp( token, "dot_product_cube_map_and_reflect_cube_map_eye_from_qs_1of3" ) )
  {
    int args[1] = { 1 };

    (*shaderSlot)->operation = 34540;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 1, args );
  }
  if ( !_stricmp( token, "dot_product_2d_2of2" ) )
  {
    int args[1] = { 1 };

    (*shaderSlot)->operation = 34542;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 1, args );
  }
  if ( !_stricmp( token, "dot_product_cube_map_3of3" ) )
  {
    int args[1] = { 1 };

    (*shaderSlot)->operation = 34544;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 1, args );
  }
  if ( !_stricmp( token, "dot_product_cube_map_and_reflect_cube_map_const_eye_2of3" )
    || !_stricmp( token, "dot_product_cube_map_and_reflect_cube_map_eye_from_qs_2of3" ) )
  {
    int args[1] = { 1 };

    (*shaderSlot)->operation = 34545;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 1, args );
  }
  if ( !_stricmp( token, "dot_product_reflect_cube_map_eye_from_qs_3of3" )
    || !_stricmp( token, "dot_product_cube_map_and_reflect_cube_map_eye_from_qs_3of3" ) )
  {
    int args[1] = { 1 };

    (*shaderSlot)->operation = 34546;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 1, args );
  }
  if ( !_stricmp( token, "dot_product_reflect_cube_map_const_eye_3of3" )
    || !_stricmp( token, "dot_product_cube_map_and_reflect_cube_map_const_eye_3of3" ) )
  {
    int args[4] = { 1, 6, 7, 8 };

    (*shaderSlot)->operation = 34547;
    return ParseNVTexShaderArgs( data_p, bundles, bundleIndex, 4, args );
  }
  else
  {
    ri.Printf( 2, "WARNING: unknown nvTexShader function '%s' in shader '%s'\n", token, shaderParseName );
    return 0;
  }
}

/* ---- ParseNVRC_ConstColor  0x004F45A0 ----  [HIGH] */
int __cdecl ParseNVRC_ConstColor(int *outNotConst, char **data_p, float *constBase)
{
  char  *token;
  float *out;
  float value;
  int    index;
  int    component;

  *outNotConst = 0;
  token = Com_Parse( data_p );
  if ( !_stricmp( token, "const0" ) )
    index = 0;
  else if ( !_stricmp( token, "const1" ) )
    index = 1;
  else
    goto notConst;
  if ( !MatchShaderToken( data_p, "=", "nvRegCombiners const" )
    || !MatchShaderToken( data_p, "(", "nvRegCombiners const" ) )
    goto failed;
  component = 0;
  for ( out = constBase + 4 * index; ; ++out )
  {
    token = Com_Parse( data_p );
    value = atof( token );
    *out = value;
    if ( value == 0.0f && !isdigit( *token ) )
    {
      ri.Printf( 2, "WARNING: unexpected token '%s' in nvRegCombiners const in shader '%s'\n", token, shaderParseName );
      goto failed;
    }
    if ( component == 3 )
      break;
    if ( !MatchShaderToken( data_p, ",", "nvRegCombiners const" ) )
      goto failed;
    ++component;
  }
  return MatchShaderToken( data_p, ")", "nvRegCombiners const" );
notConst:
  *outNotConst = 1;
  Com_UngetToken();
  goto failed;
failed:
  return 0;
}

/* ---- ParseNVRC_ConstColors  0x004F4720 ----  VERIFIED */
int __cdecl ParseNVRC_ConstColors(char **data_p, qboolean perCombiner, float *constBase)
{
  int notConst;

  while ( ParseNVRC_ConstColor( &notConst, data_p, constBase ) )
  {
    if ( perCombiner && !rendererShaderRequirements[12] )
    {
      ri.Printf(
        2,
        "WARNING: bundle uses per-combiner constants without 'requires GL_NV_register_combiners2' in shader '%s'\n",
        shaderParseName );
      return 0;
    }
  }
  return notConst;
}

/* ---- ParseNVRC_RegisterFromTable  0x004F4780 ----  [HIGH] */
int __cdecl ParseNVRC_RegisterFromTable(
        int count,
        char *token,
        const tr_nvRegisterDef_t *table,
        int *outRegister,
        unsigned int *outComponent,
        unsigned int defaultComponent)
{
  unsigned int component;
  int index;

  for ( index = 0; index < count; index++ )
  {
    if ( !_stricmp( table[index].token, token ) )
    {
      *outRegister = table[index].glRegister;
      component = table[index].componentUsage;
      *outComponent = component;
      if ( !component )
        *outComponent = defaultComponent;
      return 1;
    }
  }
  return 0;
}

/* ---- ParseNVRC_ReadWriteRegister  0x004F47E0 ----  [HIGH] */
int __cdecl ParseNVRC_ReadWriteRegister(
        char **data_p,
        int *outRegister,
        int *outOk,
        unsigned int *outComponent,
        unsigned int defaultComponent)
{
  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    *data_p = parseInfo->ungetTokenSave;
    parseInfo->currentLine = parseInfo->ungetLineSave;
  }
  if ( ParseNVRC_RegisterFromTable( 32, Com_ParseExt( data_p, qtrue ), nvReadWriteRegisterDefinitions, outRegister, outComponent, defaultComponent ) )
  {
    if ( *outRegister == 33986 )
    {
      if ( !rendererShaderRequirements[0] && !rendererShaderRequirements[1] )
      {
        ri.Printf( 2, "WARNING: register combiner uses tex2 without suitable 'requires GL_MAX_TEXTURE_UNITS_ARB' in shader '%s'\n", shaderParseName );
        *outOk = 0;
        return 0;
      }
    }
    else if ( *outRegister == 33987 && !rendererShaderRequirements[1] )
    {
      ri.Printf( 2, "WARNING: register combiner uses tex3 without suitable 'requires GL_MAX_TEXTURE_UNITS_ARB' in shader '%s'\n", shaderParseName );
      *outOk = 0;
      return 0;
    }
    *outOk = 1;
    return 1;
  }
  Com_UngetToken();
  *outOk = 1;
  return 0;
}

/* ---- ParseNVRC_GeneralOutputRegister  0x004F48C0 ----  VERIFIED */
int __cdecl ParseNVRC_GeneralOutputRegister(char **data_p, int *outRegister, unsigned int expectComponent, int *outOk)
{
  char *token;
  unsigned int component;

  *outOk = 1;
  token = Com_Parse( data_p );
  if ( !_stricmp( token, "discard" ) )
  {
    *outRegister = 34096;
    return 1;
  }
  Com_UngetToken();
  if ( ParseNVRC_ReadWriteRegister( data_p, outRegister, outOk, &component, expectComponent ) )
  {
    if ( expectComponent == component )
      return 1;
    *outOk = 0;
    ri.Printf( 2, "WARNING: output type must be %s in general combiner function in shader '%s'\n", shaderParseName );   
  }
  return 0;
}

/* ---- ParseNVRC_ReadRegister  0x004F4970 ----  VERIFIED */
int __cdecl ParseNVRC_ReadRegister(char **data_p, int *outRegister, unsigned int *outComponent, unsigned int defaultComponent)
{
  int ok;

  if ( ParseNVRC_ReadWriteRegister( data_p, outRegister, &ok, outComponent, defaultComponent ) )
    return 1;
  if ( ok )
  {
    if ( ParseNVRC_RegisterFromTable( 20, Com_Parse( data_p ), nvReadOnlyRegisterDefinitions, outRegister, outComponent, defaultComponent ) )
      return 1;
    ri.Printf( 2, "WARNING: missing register in nvRegCombiners in shader '%s'\n", shaderParseName );
    Com_UngetToken();
  }
  return 0;
}

/* ---- ParseNVRC_FinalRegister  0x004F49F0 ----  [HIGH] */
int __cdecl ParseNVRC_FinalRegister(char **data_p, int *outRegister, unsigned int *outComponent, unsigned int component)
{
  char *token;

  if ( component == 6407u )
  {
    token = Com_Parse( data_p );
    if ( !_stricmp( token, "final_product" ) )
    {
      *outRegister = 34097;
      *outComponent = 6407u;
      return 1;
    }
    if ( !_stricmp( token, "color_sum" ) )
    {
      *outRegister = 34098;
      *outComponent = 6407u;
      return 1;
    }
    Com_UngetToken();
  }
  return ParseNVRC_ReadRegister( data_p, outRegister, outComponent, component );
}

/* ---- ParseNVRC_GeneralMappedRegister  0x004F4AB0 ----  [HIGH] */
typedef struct { int registerName; int inputMapping; int componentUsage; } ShaderNVRegisterMapping;
int __cdecl ParseNVRC_GeneralMappedRegister(char **data_p, ShaderNVRegisterMapping *regMap, unsigned int component)
{
  char *token;
  int negate = 0;
  int expectParen = 1;
  int mapping;

  token = Com_Parse( data_p );
  if ( !strcmp( token, "-" ) )
  {
    negate = 1;
    token = Com_Parse( data_p );
  }

  if ( !_stricmp( token, "expand" ) )
  {
    regMap->inputMapping = ( negate != 0 ) + 34104;
  }
  else if ( !_stricmp( token, "half_bias" ) )
  {
    regMap->inputMapping = ( negate != 0 ) + 34106;
  }
  else if ( !_stricmp( token, "unsigned" ) )
  {
    if ( negate )
    {
      ri.Printf( 2, "'-' not valid with 'unsigned' in nvRegCombiners in shader '%s'\n", shaderParseName );
      return 0;
    }
    regMap->inputMapping = 34102;
  }
  else if ( !_stricmp( token, "unsigned_invert" ) )
  {
    if ( negate )
    {
      ri.Printf( 2, "'-' not valid with 'unsigned_invert' in nvRegCombiners in shader '%s'\n", shaderParseName );
      return 0;
    }
    regMap->inputMapping = 34103;
  }
  else
  {
    regMap->inputMapping = ( negate != 0 ) + 34108;
    expectParen = 0;
    Com_UngetToken();
  }
  if ( expectParen && !MatchShaderToken( data_p, "(", "nvRegCombiners register mapping" ) )
    return 0;

  if ( !ParseNVRC_ReadRegister( data_p, &regMap->registerName, (unsigned int *)&regMap->componentUsage, component ) )
    return 0;
  if ( regMap->registerName == 2912 && regMap->componentUsage == 6406 )
  {
    ri.Printf( 2, "WARNING: tried to use non-existent fog.a in nvRegCombiners in shader '%s'\n", shaderParseName );
    return 0;
  }
  if ( expectParen && !MatchShaderToken( data_p, ")", "nvRegCombiners register mapping" ) )
    return 0;
  if ( regMap->registerName != 1 )
    return 1;
  mapping = regMap->inputMapping;
  regMap->registerName = 0;
  switch ( mapping )
  {
    case 34107:
      regMap->inputMapping = 34106;
      return 1;
    case 34106:
      regMap->inputMapping = 34107;
      return 1;
    case 34102:
      regMap->inputMapping = 34103;
      return 1;
    case 34103:
      regMap->inputMapping = 34102;
      return 1;
    case 34105:
    case 34109:
      regMap->inputMapping = 34104;
      return 1;
    case 34104:
    case 34108:
      regMap->inputMapping = 34105;
      break;
    default:
      break;
  }
  return 1;
}

/* ---- ParseNVRC_FinalMappedRegister  0x004F4D80 ----  [HIGH] */
int __cdecl ParseNVRC_FinalMappedRegister(char **data_p, int regMap, unsigned int component)
{
  char *token;
  int expectParen;

  expectParen = 0;
  token = Com_Parse( data_p );
  if ( !_stricmp( token, "unsigned" ) )
  {
    *(int *)( regMap + 4 ) = 34102;
    expectParen = 1;
  }
  else if ( !_stricmp( token, "unsigned_invert" ) )
  {
    *(int *)( regMap + 4 ) = 34103;
    expectParen = 1;
  }
  else
  {
    *(int *)( regMap + 4 ) = 34102;
    Com_UngetToken();
  }
  if ( expectParen && !MatchShaderToken( data_p, "(", "nvRegCombiners final combiner register mapping" ) )
    return 0;

  if ( !ParseNVRC_FinalRegister( data_p, (int *)regMap, (unsigned int *)( regMap + 8 ), component ) )
    return 0;
  if ( component == 6406u && *(int *)( regMap + 8 ) == 6407 )
  {
    ri.Printf(
      2,
      "WARNING: alpha component of final combiner can only access alpha and blue in nvRegCombiners in shader '%s'\n",
      shaderParseName );
    return 0;
  }
  if ( expectParen && !MatchShaderToken( data_p, ")", "nvRegCombiners register mapping" ) )
    return 0;
  if ( *(int *)regMap == 1 )
  {
    *(int *)regMap = 0;
    switch ( *(int *)( regMap + 4 ) )
    {
      case 34102:
        *(int *)( regMap + 4 ) = 34103;
        break;
      case 34103:
        *(int *)( regMap + 4 ) = 34102;
        break;
    }
  }
  return 1;
}

/* ---- ParseNVRC_SingleGeneralFunction  0x004F4EC0 ----  [HIGH] */
typedef struct { ShaderNVRegisterMapping input[4]; int output[3]; int scale; int bias; unsigned char flags[3]; } ShaderNVGeneralCombiner;
int __cdecl ParseNVRC_SingleGeneralFunction(char **data_p, ShaderNVGeneralCombiner *combiner, unsigned int component, int funcIndex, int *outParsed)
{
  char *token;
  ShaderNVRegisterMapping *mapBase;

  if ( !ParseNVRC_GeneralOutputRegister( data_p, &combiner->output[funcIndex], component, outParsed ) )
    return 0;
  *outParsed = 0;
  if ( !MatchShaderToken( data_p, "=", "nvRegCombiners general combiner function" ) )
    return 0;
  mapBase = &combiner->input[2 * funcIndex];
  if ( !ParseNVRC_GeneralMappedRegister( data_p, mapBase, component ) )
    return 0;
  token = Com_Parse( data_p );
  if ( !strcmp( token, "." ) )
    combiner->flags[2 * funcIndex] = 1;
  else if ( !strcmp( token, "*" ) )
    combiner->flags[2 * funcIndex] = 0;
  else
    goto invalidOperator;
  if ( ParseNVRC_GeneralMappedRegister( data_p, mapBase + 1, component )
    && MatchShaderToken( data_p, ";", "nvRegCombiners general combiner function" ) )
  {
    *outParsed = 1;
    return 1;
  }
  return 0;
invalidOperator:
  ri.Printf( 2, "WARNING: expected '.' or '*' operator in general combiner function in shader '%s'\n", shaderParseName );
  return 0;
}

/* ---- ParseNVRC_GeneralFunction  0x004F4FE0 ----  [HIGH] */
int __cdecl ParseNVRC_GeneralFunction(char **data_p, unsigned int component, int combiner)
{
  char *token;
  int parsed;

  if ( !ParseNVRC_SingleGeneralFunction( data_p, (ShaderNVGeneralCombiner *)(combiner), component, 0, &parsed ) )
  {
    if ( parsed )
      ri.Printf( 2, "WARNING: general combiner doesn't specify any function in shader '%s'\n", shaderParseName );
    return 0;
  }
  if ( !ParseNVRC_SingleGeneralFunction( data_p, (ShaderNVGeneralCombiner *)(combiner), component, 1, &parsed ) )
    return parsed;
  if ( *(unsigned char *)( combiner + 68 ) || *(unsigned char *)( combiner + 69 ) )
    return 1;
  if ( !ParseNVRC_GeneralOutputRegister( data_p, (int *)( combiner + 56 ), component, &parsed ) )
    return parsed;
  if ( !MatchShaderToken( data_p, "=", "sum/mux function in nvRegCombiners" ) )
    return 0;
  token = Com_Parse( data_p );
  if ( !_stricmp( token, "sum" ) )
  {
    *(unsigned char *)( combiner + 70 ) = 0;
  }
  else if ( !_stricmp( token, "mux" ) )
  {
    *(unsigned char *)( combiner + 70 ) = 1;
  }
  else
  {
    ri.Printf( 2, "WARNING: final function must be mux or sum in nvRegCombiners in shader '%s'\n", shaderParseName );
    return 0;
  }
  if ( MatchShaderToken( data_p, "(", "sum/mux function in nvRegCombiners" )
    && MatchShaderToken( data_p, ")", "sum/mux function in nvRegCombiners" ) )
  {
    return MatchShaderToken( data_p, ";", "sum/mux function in nvRegCombiners" ) != 0;
  }
  return 0;
}

/* ---- ParseNVRC_GeneralPortion  0x004F5140 ----  [HIGH] */
typedef struct { float constants[2][4]; ShaderNVGeneralCombiner portion[2]; } ShaderNVGeneralBlock;
int __cdecl ParseNVRC_GeneralPortion(ShaderNVGeneralBlock *block, char **data_p, int *outParsed)
{
  char *token;
  ShaderNVGeneralCombiner *portion;
  int component;
  int result;

  *outParsed = 0;
  token = Com_Parse( data_p );
  if ( !_stricmp( token, "rgb" ) )
  {
    portion = &block->portion[0];
    component = 6407;
  }
  else if ( !_stricmp( token, "alpha" ) )
  {
    portion = &block->portion[1];
    component = 6406;
  }
  else
  {
    *outParsed = 1;
    Com_UngetToken();
    return 0;
  }
  if ( !MatchShaderToken( data_p, "{", "nvRegCombiners general combiner" ) )
    return 0;
  if ( !ParseNVRC_GeneralFunction( data_p, component, (int)portion ) )
    return 0;
  token = Com_Parse( data_p );
  if ( !_stricmp( token, "bias_by_negative_one_half_scale_by_two" ) )
  {
    portion->scale = 34110;
    portion->bias = 34113;
  }
  else if ( !_stricmp( token, "bias_by_negative_one_half" ) )
  {
    portion->scale = 0;
    portion->bias = 34113;
  }
  else if ( !_stricmp( token, "scale_by_one_half" ) )
  {
    portion->scale = 34112;
    portion->bias = 0;
  }
  else if ( !_stricmp( token, "scale_by_two" ) )
  {
    portion->scale = 34110;
    portion->bias = 0;
  }
  else if ( !_stricmp( token, "scale_by_four" ) )
  {
    portion->scale = 34111;
    portion->bias = 0;
  }
  else
  {
    Com_UngetToken();
    result = MatchShaderToken( data_p, "}", "nvRegCombiners general combiner" );
    *outParsed = result;
    return result;
  }
  if ( !MatchShaderToken( data_p, "(", "nvRegCombiners general combiner scale-and-bias" ) )
    return 0;
  if ( !MatchShaderToken( data_p, ")", "nvRegCombiners general combiner scale-and-bias" ) )
    return 0;
  if ( !MatchShaderToken( data_p, ";", "nvRegCombiners general combiner scale-and-bias" ) )
    return 0;
  result = MatchShaderToken( data_p, "}", "nvRegCombiners general combiner" );
  *outParsed = result;
  return result;
}

/* ---- ParseNVRC_General  0x004F5330 ----  [HIGH] */
int __cdecl ParseNVRC_General(char **data_p, ShaderNVGeneralBlock *block)
{
  int parsed;

  if ( !ParseNVRC_ConstColors( data_p, qtrue, block->constants[0] ) )
    return 0;
  if ( !ParseNVRC_GeneralPortion( block, data_p, &parsed ) )
  {
    if ( parsed )
      ri.Printf( 2, "WARNING: general combiner doesn't specify alpha or rgb in shader '%s'\n", shaderParseName );
    return 0;
  }
  ParseNVRC_GeneralPortion( block, data_p, &parsed );
  if ( !parsed )
    return 0;
  return MatchShaderToken( data_p, "}", "nvRegCombiners general combiner" );
}

/* ---- ParseNVRC_FinalMulSum  0x004F53B0 ----  [HIGH] */
typedef struct { ShaderNVRegisterMapping input[7]; unsigned char clampColorSum; } ShaderNVFinalCombiner;
int __cdecl ParseNVRC_FinalMulSum(char **data_p, ShaderNVFinalCombiner *finalBase)
{
  char *token;
  int haveProduct = 0;
  int haveClamp = 0;
  while ( 1 )
  {
    token = Com_Parse( data_p );
    if ( !_stricmp( token, "final_product" ) )
    {
      if ( haveProduct )
      {
        ri.Printf( 2, "WARNING: duplicate final_product in nvRegCombiners in shader '%s'\n", shaderParseName );
        return 0;
      }
      haveProduct = 1;
      if ( !ParseNVRC_FinalMappedRegister( data_p, (int)&finalBase->input[4], 0x1907u ) )
        return 0;
      if ( !MatchShaderToken( data_p, "*", "nvRegCombiners final combiner final_product" ) )
        return 0;
      if ( !ParseNVRC_FinalMappedRegister( data_p, (int)&finalBase->input[5], 0x1907u ) )
        return 0;
      if ( finalBase->input[4].componentUsage != 6407 || finalBase->input[5].componentUsage != 6407 )
      {
        ri.Printf( 2, "WARNING: final_product can only multiply rgb in nvRegCombiners in shader '%s'\n", shaderParseName );
        return 0;
      }
      if ( !MatchShaderToken( data_p, ";", "nvRegCombiners final combiner final_product" ) )
        return 0;
    }
    else if ( !_stricmp( token, "clamp_color_sum" ) )
    {
      if ( haveClamp )
      {
        ri.Printf( 2, "WARNING: duplicate clamp_color_sum in nvRegCombiners in shader '%s'\n", shaderParseName );
        return 0;
      }
      haveClamp = 1;
      if ( !MatchShaderToken( data_p, "(", "nvRegCombiners final combiner clamp_color_sum" )
        || !MatchShaderToken( data_p, ")", "nvRegCombiners final combiner clamp_color_sum" )
        || !MatchShaderToken( data_p, ";", "nvRegCombiners final combiner clamp_color_sum" ) )
        return 0;
      finalBase->clampColorSum = 1;
    }
    else
    {
      Com_UngetToken();
      return 1;
    }
  }
}

/* ---- ParseNVRC_FinalLerp  0x004F56B0 ----  [HIGH] */
int __cdecl ParseNVRC_FinalLerp(char **data_p, int *args)
{
  if ( !MatchShaderToken( data_p, "(", "final rgb lerp function in nvRegCombiners" ) )
    return 0;
  if ( !ParseNVRC_FinalMappedRegister( data_p, (int)args, 0x1907u ) )
    return 0;
  if ( *args == 34097 )
  {
    ri.Printf(
      2,
      "Cannot use final_product as first argument to lerp in nvRegCombiners in shader '%s'\n",
      shaderParseName );
    return 0;
  }
  if ( !MatchShaderToken( data_p, ",", "final rgb lerp function in nvRegCombiners" )
    || !ParseNVRC_FinalMappedRegister( data_p, (int)( args + 3 ), 0x1907u )
    || !MatchShaderToken( data_p, ",", "final rgb lerp function in nvRegCombiners" )
    || !ParseNVRC_FinalMappedRegister( data_p, (int)( args + 6 ), 0x1907u ) )
  {
    return 0;
  }
  return MatchShaderToken( data_p, ")", "final rgb lerp function in nvRegCombiners" ) != 0;
}

/* ---- ParseNVRC_FinalRgbFunc  0x004F5780 ----  [HIGH] */
int __cdecl ParseNVRC_FinalRgbFunc(char **data_p, ShaderNVRegisterMapping *args)
{
  char *token;
  ShaderNVRegisterMapping reg[3];

  token = Com_Parse( data_p );
  if ( !_stricmp( token, "lerp" ) )
  {
    if ( !ParseNVRC_FinalLerp( data_p, (int *)args ) )
      goto failed;
    goto parse_add;
  }

  Com_UngetToken();
  if ( !ParseNVRC_FinalMappedRegister( data_p, (int)&reg[0], 0x1907u ) )
    goto failed;
  token = Com_Parse( data_p );
  if ( !strcmp( token, "+" ) )
  {
    token = Com_Parse( data_p );
    if ( !_stricmp( token, "lerp" ) )
    {
      args[3] = reg[0];
      if ( ParseNVRC_FinalLerp( data_p, (int *)args ) )
        return MatchShaderToken( data_p, ";", "final rgb function in nvRegCombiners" );
      goto failed;
    }
    Com_UngetToken();
    if ( !ParseNVRC_FinalMappedRegister( data_p, (int)&reg[1], 0x1907u ) )
      goto failed;
    token = Com_Parse( data_p );
    if ( !strcmp( token, "*" ) )
    {
      if ( !ParseNVRC_FinalMappedRegister( data_p, (int)&reg[2], 0x1907u ) )
        goto failed;
      if ( reg[1].registerName == 34097 )
      {
        if ( reg[2].registerName == 34097 )
          goto product_by_itself;
        args[0] = reg[2];
        args[1] = reg[1];
      }
      else
      {
        args[0] = reg[1];
        args[1] = reg[2];
      }
      args[2].registerName = 0;
      args[2].inputMapping = 34102;
      args[2].componentUsage = 6407;
      args[3] = reg[0];
      return MatchShaderToken( data_p, ";", "final rgb function in nvRegCombiners" );
    }
    if ( !strcmp( token, ";" ) )
    {
      args[0].registerName = 0;
      args[0].inputMapping = 34102;
      args[0].componentUsage = 6407;
      args[1].registerName = 0;
      args[1].inputMapping = 34102;
      args[1].componentUsage = 6407;
      args[2] = reg[0];
      args[3] = reg[1];
      return 1;
    }
  }
  else
  {
    if ( !strcmp( token, "*" ) )
    {
      if ( !ParseNVRC_FinalMappedRegister( data_p, (int)&reg[2], 0x1907u ) )
        goto failed;
      if ( reg[0].registerName == 34097 )
      {
        if ( reg[2].registerName == 34097 )
          goto product_by_itself;
        args[0] = reg[2];
        args[1] = reg[0];
      }
      else
      {
        args[0] = reg[0];
        args[1] = reg[2];
      }
      args[2].registerName = 0;
      args[2].inputMapping = 34102;
      args[2].componentUsage = 6407;
parse_add:
      token = Com_Parse( data_p );
      if ( !strcmp( token, "+" ) )
      {
        if ( ParseNVRC_FinalMappedRegister( data_p, (int)&args[3], 0x1907u ) )
          return MatchShaderToken( data_p, ";", "final rgb function in nvRegCombiners" );
        goto failed;
      }
      if ( !strcmp( token, ";" ) )
      {
        args[3].registerName = 0;
        args[3].inputMapping = 34102;
        args[3].componentUsage = 6407;
        return 1;
      }
      goto unexpected;
    }
    if ( !strcmp( token, ";" ) )
    {
      args[0].registerName = 0;
      args[0].inputMapping = 34102;
      args[0].componentUsage = 6407;
      args[1].registerName = 0;
      args[1].inputMapping = 34102;
      args[1].componentUsage = 6407;
      args[2].registerName = 0;
      args[2].inputMapping = 34102;
      args[2].componentUsage = 6407;
      args[3] = reg[0];
      return 1;
    }
  }

unexpected:
  ri.Printf( 2, "unexpected token '%s' in final rgb function in nvRegCombiners in shader '%s'\n", token, shaderParseName );
  goto failed;

product_by_itself:
  ri.Printf( 2, "cannot multiply final_product by itself in nvRegcombiners in shader '%s'\n", shaderParseName );
  goto failed;
failed:
  return 0;
}

/* ---- ParseNVRC_FinalRgbAlpha  0x004F5BC0 ----  [HIGH] */
int __cdecl ParseNVRC_FinalRgbAlpha(char **data_p, ShaderNVFinalCombiner *args)
{
  char *token;
  int haveRgb = 0;
  int haveAlpha = 0;
  while ( 1 )
  {
    token = Com_Parse( data_p );
    if ( !strcmp( token, "}" ) )
      return 1;
    if ( !_stricmp( token, "out.rgb" ) )
    {
      if ( haveRgb )
      {
        ri.Printf( 2, "WARNING: duplicate rgb in final stage in nvRegCombiners in shader '%s'\n", shaderParseName );
        return 0;
      }
      haveRgb = 1;
      if ( !MatchShaderToken( data_p, "=", "out.rgb in nvRegCombiners" ) )
        return 0;
      if ( !ParseNVRC_FinalRgbFunc( data_p, args->input ) )
        return 0;
    }
    else if ( !_stricmp( token, "out.a" ) )
    {
      if ( haveAlpha )
      {
        ri.Printf( 2, "WARNING: duplicate alpha in final stage nvRegCombiners in shader '%s'\n", shaderParseName );
        return 0;
      }
      haveAlpha = 1;
      if ( !MatchShaderToken( data_p, "=", "out.a in nvRegCombiners" ) )
        return 0;
      if ( !ParseNVRC_FinalMappedRegister( data_p, (int)&args->input[6], 0x1906u ) )
        return 0;
      if ( !MatchShaderToken( data_p, ";", "out.a in nvRegCombiners" ) )
        return 0;
    }
    else
    {
      ri.Printf( 2, "WARNING: unexpected '%s' in final stage in nvRegCombiners in shader '%s'\n", token, shaderParseName );
      return 0;
    }
  }
}

/* ---- ParseNVRC_Final  0x004F5E10 ----  VERIFIED */
int __cdecl ParseNVRC_Final(char **data_p, int *args)
{
  if ( !ParseNVRC_FinalMulSum( data_p, (ShaderNVFinalCombiner *)(args) ) )
    return 0;
  return ParseNVRC_FinalRgbAlpha( data_p, (ShaderNVFinalCombiner *)(args) );
}

/* ---- ParseNVRC_ClearGeneralCombinerPortion  0x004F5E30 ----  [HIGH] */
int __cdecl ParseNVRC_ClearGeneralCombinerPortion(int portion, int component)
{
  int *reg;
  int i;

  reg = (int *)( portion + 8 );
  i = 4;
  do
  {
    *(reg - 2) = 0;
    *(reg - 1) = 34102;
    *reg = component;
    reg += 3;
    --i;
  }
  while ( i );
  *(int *)( portion + 48 ) = 34096;
  *(int *)( portion + 52 ) = 34096;
  *(int *)( portion + 56 ) = 34096;
  *(int *)( portion + 60 ) = 0;
  *(int *)( portion + 64 ) = 0;
  *(unsigned char *)( portion + 68 ) = 0;
  *(unsigned char *)( portion + 69 ) = 0;
  *(unsigned char *)( portion + 70 ) = 0;
  return 34096;
}

/* ---- ParseNVRegCombiners  0x004F5E80 ----  [HIGH] */
typedef struct ShaderNVCombinerState { float constants[2][4]; ShaderNVGeneralBlock general[8]; ShaderNVFinalCombiner final; int numCombiners; qboolean perCombinerConstants; } ShaderNVCombinerState;
int __cdecl ParseNVRegCombiners(shaderStage_t *stage, char **data_p)
{
  char *token;
  int *block;
  int i;
  int notConst;
  int dupIndex;
  void *hunk;
  ShaderNVCombinerState state;   

  
  if ( !rendererShaderRequirements[11] && !rendererShaderRequirements[12] )
  {
    ri.Printf(
      2,
      "WARNING: stage uses nvRegCombiners without 'requires GL_NV_register_combiners(2)' in shader '%s'\n",
      shaderParseName );
    return 0;
  }

  if ( !MatchShaderToken( data_p, "{", "nvRegCombiners" ) )
    return 0;

  
  block = (int *)&state.general[0];
  i = 8;
  do
  {
    ParseNVRC_ClearGeneralCombinerPortion( (int)(block + 8), 6407 );
    ParseNVRC_ClearGeneralCombinerPortion( (int)(block + 26), 6406 );
    block += 44;
    --i;
  }
  while ( i );

  for ( i = 0; i < 7; i++ )
  {
    state.final.input[i].registerName = 0;
    state.final.input[i].inputMapping = 34102;
    state.final.input[i].componentUsage = 6407;
  }
  state.final.input[6].componentUsage = 6406;
  state.final.clampColorSum = 0;

  memset( state.constants, 0, sizeof(state.constants) );
  while ( ParseNVRC_ConstColor( &notConst, data_p, state.constants[0] ) )
    ;
  if ( !notConst )
    return 0;

  
  memcpy( state.general[0].constants, state.constants, sizeof(state.constants) );
  memcpy( state.general[1].constants, state.constants, sizeof(state.constants) );
  memcpy( state.general[2].constants, state.constants, sizeof(state.constants) );
  memcpy( state.general[3].constants, state.constants, sizeof(state.constants) );
  memcpy( state.general[4].constants, state.constants, sizeof(state.constants) );
  memcpy( state.general[5].constants, state.constants, sizeof(state.constants) );
  memcpy( state.general[6].constants, state.constants, sizeof(state.constants) );
  memcpy( state.general[7].constants, state.constants, sizeof(state.constants) );

  for ( state.numCombiners = 0; ; ++state.numCombiners )
  {
    token = Com_Parse( data_p );
    if ( strcmp( token, "{" ) )
      break;
    if ( state.numCombiners == 2 )
    {
      if ( !rendererShaderRequirements[12] )
      {
        ri.Printf(
          state.numCombiners,
          "WARNING: stage uses more that 2 general combiners without 'requires GL_NV_register_combiners2' in shader '%s'\n",
          shaderParseName );
        return 0;
      }
    }
    else if ( state.numCombiners == 8 )
    {
      ri.Printf( 2, "WARNING: stage uses more that %i general combiners in shader '%s'\n", 8, shaderParseName );
      return 0;
    }
    if ( !ParseNVRC_General( data_p, &state.general[state.numCombiners] ) )
      return 0;
  }
  Com_UngetToken();

  if ( !ParseNVRC_Final( data_p, (int *)&state.final ) )
    return 0;

  
  state.perCombinerConstants = 0;
  for ( dupIndex = 0; dupIndex < state.numCombiners; ++dupIndex )
  {
    if ( memcmp( state.constants, state.general[dupIndex].constants, sizeof(state.constants) ) )
    {
      state.perCombinerConstants = 1;
      break;
    }
  }

  hunk = (void *)ri.Hunk_Alloc( sizeof(state) );
  stage->registerCombiners = (ShaderNVCombinerState *)hunk;
  memcpy( hunk, &state, sizeof(state) );
  return 1;
}

/* ---- ParseATIFS_ConstReg  0x004F6240 ----  [HIGH] */
int __cdecl ParseATIFS_ConstReg(char **data_p)
{
  char *token;
  char digit;

  token = Com_Parse( data_p );
  if ( !_strnicmp( token, "const", 5u ) )
  {
    digit = token[5];
    if ( digit >= '0' && digit <= '8' && !token[6] )
      return digit + 35089;
  }
  Com_UngetToken();
  return 0;
}

/* ---- ParseATIFS_Reg  0x004F62C0 ----  [HIGH] */
int __cdecl ParseATIFS_Reg(char **data_p)
{
  char *token;
  char digit;

  token = Com_Parse( data_p );
  if ( *token == 'r' || *token == 'R' )
  {
    digit = token[1];
    if ( digit >= '0' && digit < '6' && !token[2] )
      return digit + 35057;
  }
  Com_UngetToken();
  return 0;
}

/* ---- ParseATIFS_TexCoord  0x004F6330 ----  [HIGH] */
int __cdecl ParseATIFS_TexCoord(char **data_p)
{
  char *token;
  char digit;

  token = Com_Parse( data_p );
  if ( !_strnicmp( token, "tc", 2u ) )
  {
    digit = token[2];
    if ( digit >= '0' && digit < '8' && !token[3] )
      return digit + 33936;
  }
  Com_UngetToken();
  return 0;
}

/* ---- ParseATIFS_ConstDefs  0x004F63B0 ----  [HIGH] */
typedef struct { float value[4]; qboolean defined; } ShaderATIConstantDefinition;
int __cdecl ParseATIFS_ConstDefs(char **data_p, ShaderATIConstantDefinition *constants)
{
  unsigned int reg;
  reg = ParseATIFS_ConstReg( data_p );
  if ( !reg )
    return 1;
  {
  int i;
  ShaderATIConstantDefinition *definition;
  float *comp;
  char *token;
  float value;
  const char *expected;

  if ( !MatchShaderToken( data_p, "=", "atiFragmentShader constant definitions" )
    || !MatchShaderToken( data_p, "(", "atiFragmentShader constant definitions" ) )
  {
    goto failed;
  }
  i = 0;   /* counter set before the cursor: retail's xor ebp,ebp precedes the two stores */
  definition = &constants[reg - 0x8941];
  comp = definition->value;
  for ( ; i < 4; ++i, ++comp )
  {
    token = Com_Parse( data_p );
    value = atof( token );
    *comp = value;
    if ( ( value == 0.0f && *token != '0' && *token != '.' )
      || value < 0.0f
      || value > 1.0f )
    {
      ri.Printf(
        2,
        "WARNING: expected a number from 0 to 1, found '%s' instead in atiFragmentShader constant in shader %s\n",
        token,
        shaderParseName );
      goto failed;
    }
    expected = ",";
    if ( i >= 3 )
      expected = ")";
    if ( !MatchShaderToken( data_p, expected, "atiFragmentShader constant definitions" ) )
      goto failed;
  }
  if ( !MatchShaderToken( data_p, ";", "atiFragmentShader constant definitions" ) )
    goto failed;
  definition->defined = qtrue;
  return 1;
failed:
  return 0;
  }
}

typedef struct shaderParseMark_s
{
  int       currentLine;      /* +0x00  ebp-14h */
  char     *data;             /* +0x04  ebp-10h  the saved *data_p */
  qboolean  ungetReady;       /* +0x08  ebp-0Ch */
  int       ungetLineSave;    /* +0x0C  ebp-08h */
  char     *ungetTokenSave;   /* +0x10  ebp-04h */
} shaderParseMark_t;

/* ---- ParseATIFS_TexReads  0x004F6520 ----  [HIGH] */
typedef struct { qboolean sampleTexture; unsigned int source; unsigned int swizzle; } ShaderATIRoutingInstruction;
int __cdecl ParseATIFS_TexReads(char **data_p, ShaderATIRoutingInstruction *routeInstructions, qboolean phaseIndex)
{
  int regEnum;
  int regIndex;
  ShaderATIRoutingInstruction *entry;
  char *token;
  char *routeToken;
  int source;
  int outerIdx;
  int innerIdx;
  shaderParseMark_t mark; 

  mark.currentLine = parseInfo->currentLine;
  mark.data = *data_p;
  mark.ungetReady = parseInfo->ungetReady;
  mark.ungetLineSave = parseInfo->ungetLineSave;
  mark.ungetTokenSave = parseInfo->ungetTokenSave;
  while ( ( regEnum = ParseATIFS_Reg( data_p ) ) != 0 )
  {
    regIndex = regEnum - 35105;
    token = Com_Parse( data_p );
    if ( *token != '=' || token[1] )
    {
      Com_ParseReturnToMark( &mark.currentLine, data_p );
      break;
    }
    routeToken = Com_Parse( data_p );
    if ( !_stricmp( routeToken, "copy" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->sampleTexture = 0;
      entry->swizzle = 35190;
    }
    else if ( !_stricmp( routeToken, "copy_dr" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->sampleTexture = 0;
      entry->swizzle = 35192;
    }
    else if ( !_stricmp( routeToken, "tex" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->swizzle = 35190;
      entry->sampleTexture = 1;
    }
    else if ( !_stricmp( routeToken, "tex_dr" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->swizzle = 35192;
      entry->sampleTexture = 1;
    }
    else if ( !_stricmp( routeToken, "copy_stq" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->sampleTexture = 0;
      entry->swizzle = 35191;
    }
    else if ( !_stricmp( routeToken, "copy_stq_dq" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->sampleTexture = 0;
      entry->swizzle = 35193;
    }
    else if ( !_stricmp( routeToken, "tex_stq" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->swizzle = 35191;
      entry->sampleTexture = 1;
    }
    else if ( !_stricmp( routeToken, "tex_stq_dq" ) )
    {
      entry = &routeInstructions[regIndex];
      entry->sampleTexture = 1;
      entry->swizzle = 35193;
    }
    else
    {
      Com_ParseReturnToMark( &mark.currentLine, data_p );
      break;
    }
    if ( entry->source )
    {
      ri.Printf(
        2,
        "WARNING: already encountered a routing instruction for r%i in atiFragmentShader in shader '%s'\n",
        regIndex,
        shaderParseName );
      return 0;
    }
    if ( !MatchShaderToken( data_p, "(", "routing instruction of atiFragmentShader" ) )
      return 0;
    source = ParseATIFS_TexCoord( data_p );
    if ( !source )
    {
      source = ParseATIFS_Reg( data_p );
      if ( !source )
      {
        token = Com_Parse( data_p );
        ri.Printf(
          2,
          "WARNING: expected r# or tc# instead of '%s' in routing instruction of atiFragmentShader in shader '%s'\n",
          token,
          shaderParseName );
        return 0;
      }
      
      if ( entry->swizzle == 35191 || entry->swizzle == 35193 )
      {
        ri.Printf(
          2,
          "WARNING: cannot use _stq forms with r# in routing instruction of atiFragmentShader in shader '%s'\n",
          routeToken,
          shaderParseName );
        return 0;
      }
      if ( !phaseIndex )
      {
        ri.Printf(
          2,
          "WARNING: cannot use r# in routing instruction of first phase of atiFragmentShader in shader '%s'\n",
          routeToken,
          shaderParseName );
        return 0;
      }
    }
    entry->source = source;
    if ( !MatchShaderToken( data_p, ")", "routing instruction of atiFragmentShader" )
      || !MatchShaderToken( data_p, ";", "routing instruction of atiFragmentShader" ) )
    {
      return 0;
    }
    Com_ParseSetMark( &mark.currentLine, data_p );
  }
  
  for ( outerIdx = 0; outerIdx < 6; ++outerIdx )
  {
    if ( !routeInstructions[outerIdx].source )
      continue;
    for ( innerIdx = 0; innerIdx < outerIdx; ++innerIdx )
    {
      if ( routeInstructions[innerIdx].source
        && routeInstructions[outerIdx].source == routeInstructions[innerIdx].source
        && routeInstructions[outerIdx].swizzle != routeInstructions[innerIdx].swizzle
        && routeInstructions[outerIdx].source >= 0x84C0
        && routeInstructions[outerIdx].source <= 0x84C8 )
      {
        ri.Printf(
          2,
          "WARNING: cannot use tc%c with different instruction suffixes (such as _stq and _stq_dq) in atiFragmentShader in shader '%s'\n",
          routeInstructions[outerIdx].source - 33936,
          shaderParseName );
        return 0;
      }
    }
  }
  return 1;
}

/* ---- ParseATIFS_DestReg  0x004F6A50 ----  [HIGH] */
void __cdecl ParseATIFS_DestReg(
        char **data_p,
        int *reg,
        int *writeColor,
        int *colorMask,
        int *writeAlpha)
{
  char *token;
  char digit;
  char comp;

  *reg = 0;
  *colorMask = 0;
  *writeColor = 0;
  *writeAlpha = 0;
  token = Com_Parse( data_p );
  if ( *token == 'r' || *token == 'R' )
  {
    digit = token[1];
    if ( digit >= '0' && digit < '6' )
    {
      *reg = digit + 35057;
      if ( !token[2] )
      {
        /* "rN" -> whole register, colour write */
        *colorMask = 0;
        *writeColor = 1;
        return;
      }
      if ( token[2] == '.' )
      {
        if ( !token[4] )
        {
          /* "rN.X" -> single component */
          comp = token[3];
          if ( comp == 'r' || comp == 'R' )
          {
            *colorMask = 1;
            *writeColor = 1;
            return;
          }
          if ( comp == 'g' || comp == 'G' )
          {
            *colorMask = 2;
            *writeColor = 1;
            return;
          }
          if ( comp == 'b' || comp == 'B' )
          {
            *colorMask = 4;
            *writeColor = 1;
            return;
          }
          if ( comp == 'a' || comp == 'A' )
          {
            *colorMask = 0;
            *writeAlpha = 1;
            return;
          }
        }
        else if ( ( token[4] == 'a' || token[4] == 'A' ) && !token[5] )
        {
          /* "rN.Xa" -> single colour component plus alpha */
          *writeAlpha = 1;
          comp = token[3];
          if ( comp == 'r' || comp == 'R' )
          {
            *colorMask = 1;
            *writeColor = 1;
            return;
          }
          if ( comp == 'g' || comp == 'G' )
          {
            *colorMask = 2;
            *writeColor = 1;
            return;
          }
          if ( comp == 'b' || comp == 'B' )
          {
            *colorMask = 4;
            *writeColor = 1;
            return;
          }
        }
        else
        {
          /* "rN.rgb" / "rN.rgba" */
          if ( !_stricmp( token + 3, "rgba" ) )
          {
            *colorMask = 6407;
            *writeColor = 1;
            *writeAlpha = 1;
            return;
          }
          if ( !_stricmp( token + 3, "rgb" ) )
          {
            *colorMask = 6407;
            *writeColor = 1;
            return;
          }
        }
      }
    }
  }
  Com_UngetToken();
}

/* ---- ParseATIFS_ArgReg  0x004F6BF0 ----  [HIGH] */
int __cdecl ParseATIFS_ArgReg(char **data_p, int *outArg)
{
  char *token;
  char digit;
  int nameLen;
  char suffix;
  char *swizzle;
  char swz;

  token = Com_Parse( data_p );
  if ( *token == 'r' || *token == 'R' )
  {
    digit = token[1];
    if ( digit < '0' || digit > '6' )
      goto badArg;
    *outArg = digit + 35057;
    nameLen = 2;
  }
  else if ( !_strnicmp( token, "const", 5u ) )
  {
    digit = token[5];
    if ( digit < '0' || digit > '8' )
      goto badArg;
    *outArg = digit + 35089;
    nameLen = 6;
  }
  else if ( !_strnicmp( token, "col0", 4u ) )
  {
    *outArg = 34167;
    nameLen = 4;
  }
  else if ( !_strnicmp( token, "col1", 4u ) )
  {
    *outArg = 35181;
    nameLen = 4;
  }
  else
  {
    goto badArg;
  }
  suffix = token[nameLen];
  if ( !suffix )
    return 1;
  if ( suffix != '.' )
    goto badArg;
  swizzle = &token[nameLen + 1];
  if ( !swizzle[1] )
  {
    swz = *swizzle;
    if ( swz == 'r' || swz == 'R' )
    {
      outArg[1] = 6403;
      return 1;
    }
    if ( swz == 'g' || swz == 'G' )
    {
      outArg[1] = 6404;
      return 1;
    }
    if ( swz == 'b' || swz == 'B' )
    {
      outArg[1] = 6405;
      return 1;
    }
    if ( swz == 'a' || swz == 'A' )
    {
      outArg[1] = 6406;
      return 1;
    }
  }
  ri.Printf(
    2,
    "WARNING: expected .r, .g, .b or .a instead of '.%s' in function argument in atiFragmentShader in shader '%s'\n",
    swizzle,
    shaderParseName );
  return 0;

badArg:
  ri.Printf(
    2,
    "WARNING: expected r#, const#, col0 or col1 instead of '%s' in function argument in atiFragmentShader in shader '%s'\n",
    token,
    shaderParseName );
  return 0;
}

/* ---- ParseATIFS_FunctionArgs  0x004F6D80 ----  [HIGH] */
typedef struct { unsigned int registerName; unsigned int swizzle; unsigned int modifier; } ShaderATIFragmentArgument;
int __cdecl ParseATIFS_FunctionArgs(char **data_p, ShaderATIFragmentArgument *fragArgs, int argCount, int *outUsesInterpolator)
{
  int argIndex;
  ShaderATIFragmentArgument *arg;
  char *firstTok;
  char *tok;
  const char *delim;

  for ( argIndex = 0; argIndex < argCount; ++argIndex )
  {
    firstTok = Com_Parse( data_p );
    if ( !strcmp( firstTok, "-" ) )
    {
      
      tok = Com_Parse( data_p );
      if ( !strcmp( tok, "2" ) )
      {
        tok = Com_Parse( data_p );
        if ( !strcmp( tok, "*" ) )
        {
          
          arg = &fragArgs[argIndex];
          if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
            return 0;
          arg = &fragArgs[argIndex];
          arg->modifier = 5;
        }
        else
        {
          
          Com_UngetToken();
          arg = &fragArgs[argIndex];
          arg->modifier = 5;
          arg = &fragArgs[argIndex];
          arg->registerName = 1;
        }
      }
      else if ( !strcmp( tok, "0.5" ) || !strcmp( tok, ".5" ) )
      {
        
        arg = &fragArgs[argIndex];
        arg->registerName = 0;
        arg = &fragArgs[argIndex];
        arg->modifier = 8;
      }
      else if ( !strcmp( tok, "1" ) )
      {
        
        arg = &fragArgs[argIndex];
        arg->modifier = 4;
        arg = &fragArgs[argIndex];
        arg->registerName = 1;
      }
      else
      {
        
        Com_UngetToken();
        arg = &fragArgs[argIndex];
        if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
          return 0;
        tok = Com_Parse( data_p );
        if ( !strcmp( tok, "*" ) )
        {
          tok = Com_Parse( data_p );
          if ( strcmp( tok, "2" ) )
          {
            ri.Printf(
              2,
              "WARNING: expected 2 instead of '%s' in function argument in atiFragmentShader in shader '%s'\n",
              tok,
              shaderParseName );
            return 0;
          }
          arg = &fragArgs[argIndex];
          arg->modifier = 5;
        }
        else
        {
          Com_UngetToken();
          arg = &fragArgs[argIndex];
          arg->modifier = 4;
        }
      }
    }
    else if ( !strcmp( firstTok, "2" ) )
    {
      tok = Com_Parse( data_p );
      if ( !strcmp( tok, "*" ) )
      {
        
        arg = &fragArgs[argIndex];
        if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
          return 0;
        tok = Com_Parse( data_p );
        if ( !strcmp( tok, "-" ) )
{
          tok = Com_Parse( data_p );
          if ( !strcmp( tok, "1" ) )
          {
            arg = &fragArgs[argIndex];
            arg->modifier = 9;
          }
          else
          {
            if ( strcmp( tok, "2" ) )
            {
              ri.Printf(
                2,
                "WARNING: expected N to be 1 or 2 instead of '%s' in 2 * arg - N in atiFragmentShader in shader '%s'\n",
                tok,
                shaderParseName );
              return 0;
            }
            arg = &fragArgs[argIndex];
            arg->modifier = 3;
          }
        }
else
{
          
          Com_UngetToken();
          arg = &fragArgs[argIndex];
          arg->modifier = 1;
        }
      }
      else if ( !strcmp( tok, "-" ) )
      {
        
        tok = Com_Parse( data_p );
        if ( !strcmp( tok, "2" ) )
        {
          if ( !MatchShaderToken( data_p, "*", "2 - 2 * arg in fragment argument in atiFragmentShader" ) )
            return 0;
          arg = &fragArgs[argIndex];
          if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
            return 0;
          arg = &fragArgs[argIndex];
          arg->modifier = 3;
        }
        else
        {
          Com_UngetToken();
          arg = &fragArgs[argIndex];
          if ( !ParseATIFS_ArgReg( data_p, (int *)arg )
            || !MatchShaderToken( data_p, "*", "2 - arg * 2 in fragment argument in atiFragmentShader" )
            || !MatchShaderToken( data_p, "2", "2 - arg * 2 in fragment argument in atiFragmentShader" ) )
          {
            return 0;
          }
          arg = &fragArgs[argIndex];
          arg->modifier = 3;
        }
      }
      else
      {
        
        arg = &fragArgs[argIndex];
        arg->modifier = 1;
        arg = &fragArgs[argIndex];
        arg->registerName = 1;
      }
    }
    else if ( !strcmp( firstTok, "1" ) )
    {
      tok = Com_Parse( data_p );
      if ( !strcmp( tok, "-" ) )
{
        tok = Com_Parse( data_p );
        if ( !strcmp( tok, "2" ) )
        {
          
          if ( !MatchShaderToken( data_p, "*", "1 - 2 * arg in fragment argument in atiFragmentShader" ) )
            return 0;
          arg = &fragArgs[argIndex];
          if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
            return 0;
          arg = &fragArgs[argIndex];
          arg->modifier = 7;
        }
        else
        {
          
          arg = &fragArgs[argIndex];
          if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
            return 0;
          tok = Com_Parse( data_p );
          if ( !strcmp( tok, "*" ) )
          {
            if ( !MatchShaderToken( data_p, "2", "1 - arg * 2 in fragment argument in atiFragmentShader" ) )
              return 0;
            arg = &fragArgs[argIndex];
            arg->modifier = 7;
          }
          else
          {
            Com_UngetToken();
            arg = &fragArgs[argIndex];
            arg->modifier = 2;
          }
        }
      }
else
{
        
        Com_UngetToken();
        arg = &fragArgs[argIndex];
        arg->registerName = 1;
      }
    }
    else if ( !strcmp( firstTok, ".5" ) || !strcmp( firstTok, "0.5" ) )
    {
      tok = Com_Parse( data_p );
      if ( !strcmp( tok, "-" ) )
{
        
        arg = &fragArgs[argIndex];
        if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
          return 0;
        arg = &fragArgs[argIndex];
        arg->modifier = 12;
      }
else
{
        
        Com_UngetToken();
        arg = &fragArgs[argIndex];
        arg->modifier = 8;
        arg = &fragArgs[argIndex];
        arg->registerName = 1;
      }
    }
    else if ( !strcmp( firstTok, "0" ) )
    {
      
      arg = &fragArgs[argIndex];
      arg->registerName = 0;
    }
    else
    {
      
      Com_UngetToken();
      arg = &fragArgs[argIndex];
      if ( !ParseATIFS_ArgReg( data_p, (int *)arg ) )
        return 0;
      tok = Com_Parse( data_p );
      if ( !strcmp( tok, "-" ) )
      {
        tok = Com_Parse( data_p );
        if ( !strcmp( tok, "0.5" ) || !strcmp( tok, ".5" ) )
        {
          arg = &fragArgs[argIndex];
          arg->modifier = 8;
        }
        else
        {
          if ( strcmp( tok, "1" ) )
          {
            ri.Printf(
              2,
              "WARNING: expected N to be 1 or .5 instead of '%s' in arg - N in atiFragmentShader in shader '%s'\n",
              tok,
              shaderParseName );
            return 0;
          }
          arg = &fragArgs[argIndex];
          arg->modifier = 6;
        }
      }
      else if ( !strcmp( tok, "*" ) )
      {
        if ( !MatchShaderToken( data_p, "2", "arg * 2 in function argument in atiFragmentShader" ) )
          return 0;
        tok = Com_Parse( data_p );
        if ( !strcmp( tok, "-" ) )
{
          tok = Com_Parse( data_p );
          if ( !strcmp( tok, "1" ) )
          {
            arg = &fragArgs[argIndex];
            arg->modifier = 9;
          }
          else if ( !strcmp( tok, "2" ) )
          {
            arg = &fragArgs[argIndex];
            arg->modifier = 3;
          }
          else
          {
            ri.Printf(
              2,
              "WARNING: expected N to be 1 or 2 instead of '%s' in arg * 2 - N in atiFragmentShader in shader '%s'\n",
              tok,
              shaderParseName );
            return 0;
          }
        }
else
{
          
          Com_UngetToken();
          arg = &fragArgs[argIndex];
          arg->modifier = 1;
        }
      }
      else
      {
        
        Com_UngetToken();
      }
    }

    delim = ")";
    if ( argIndex != argCount - 1 )
      delim = ",";
    if ( !MatchShaderToken( data_p, delim, "instruction argument list in atiFragmentShader" ) )
      return 0;
    arg = &fragArgs[argIndex];
    if ( arg->registerName == 34167 || arg->registerName == 35181 )
      *outUsesInterpolator = 1;
  }
  return 1;
}

/* ---- ParseATIFS_FragOps  0x004F7670 ----  [HIGH] */
typedef struct { unsigned int operation; unsigned int destination; int colorMask; unsigned int destinationModifier; ShaderATIFragmentArgument argument[3]; } ShaderATIFragmentInstruction;
int __cdecl ParseATIFS_FragOps(char **data_p, ShaderATIFragmentInstruction *fragInstructions, int phaseIndex, int *outUsesInterpolator)
{
  int destReg;         
  int writeColor;      
  int colorMask;       
  int writeAlpha;      
  int instrSlot;
  ShaderATIFragmentInstruction *instructionPair;
  int prevWriteColor;
  int mergeAlpha;
  int clampFlag;
  int prescale;
  int hadPrescale;
  int scaleMod;
  int op;
  int argCount;
  int *instr;
  int isMultiply;
  char *opTok;
  char *scaleTok;
  char *numTok;
  shaderParseMark_t mark; 

  Com_ParseSetMark( &mark.currentLine, data_p );
  prevWriteColor = 0;
  mergeAlpha = 0;
  ParseATIFS_DestReg( data_p, &destReg, &writeColor, &colorMask, &writeAlpha );
  if ( !destReg )
    return 1;
  instrSlot = 0;
  instructionPair = fragInstructions;
  while ( 1 )
  {
    if ( !MatchShaderToken( data_p, "=", "fragment instruction of atiFragmentShader" ) )
      return 0;
    scaleMod = 0;
    opTok = Com_Parse( data_p );
    if ( !_stricmp( opTok, "clamp" ) )
    {
      clampFlag = 1;
      opTok = Com_Parse( data_p );
      if ( _stricmp( opTok, "(" ) )
      {
        ri.Printf( 2, "WARNING: %s missing '%s', found '%s' instead in shader '%s'\n", "clamp fragment instruction of atiFragmentShader", "(", opTok, shaderParseName );
        return 0;
      }
      opTok = Com_Parse( data_p );
    }
    else
    {
      clampFlag = 0;
    }
    
    hadPrescale = 0;
    prescale = 0;
    if ( !opTok[1] && ( *opTok == '2' || *opTok == '4' || *opTok == '8' ) )
    {
      if ( *opTok == '2' )
        scaleMod = 1;
      else if ( *opTok == '4' )
        scaleMod = 2;
      else
        scaleMod = 4;
      hadPrescale = 1;
      prescale = 1;
      if ( !MatchShaderToken( data_p, "*", "prescaled fragment instruction of atiFragmentShader" ) )
        return 0;
      opTok = Com_Parse( data_p );
    }
    if ( !_stricmp( opTok, "mov" ) )
    {
      op = 35169;
      argCount = 1;
    }
    else if ( !_stricmp( opTok, "add" ) )
    {
      op = 35171;
      argCount = 2;
    }
    else if ( !_stricmp( opTok, "sub" ) )
    {
      op = 35173;
      argCount = 2;
    }
    else if ( !_stricmp( opTok, "mul" ) )
    {
      op = 35172;
      argCount = 2;
    }
    else if ( !_stricmp( opTok, "dot3" ) )
    {
      if ( !writeColor )
      {
        ri.Printf( 2, "WARNING: dot3 cannot write only alpha in atiTexShader in shader '%s'\n", shaderParseName );
        return 0;
      }
      op = 35174;
      argCount = 2;
    }
    else if ( !_stricmp( opTok, "dot4" ) )
    {
      if ( !writeColor )
      {
        ri.Printf( 2, "WARNING: dot4 cannot write only alpha in atiTexShader in shader '%s'\n", shaderParseName );
        return 0;
      }
      op = 35175;
      argCount = 2;
    }
    else if ( !_stricmp( opTok, "mad" ) )
    {
      op = 35176;
      argCount = 3;
    }
    else if ( !_stricmp( opTok, "lerp" ) )
    {
      op = 35177;
      argCount = 3;
    }
    else if ( !_stricmp( opTok, "cnd" ) )
    {
      op = 35178;
      argCount = 3;
    }
    else if ( !_stricmp( opTok, "cnd0" ) )
    {
      op = 35179;
      argCount = 3;
    }
    else if ( !_stricmp( opTok, "dot2_add" ) )
    {
      if ( !writeColor )
      {
        ri.Printf( 2, "WARNING: dot2_add cannot write only alpha in atiTexShader in shader '%s'\n", shaderParseName );
        return 0;
      }
      op = 35180;
      argCount = 3;
    }
    else if ( !clampFlag && !hadPrescale && !colorMask )
    {
      
      Com_ParseReturnToMark( &mark.currentLine, data_p );
      return 1;
    }
    else
    {
      ri.Printf( 2, "WARNING: unknown function '%s' in atiFragmentShader in shader '%s'\n", opTok, shaderParseName );
      return 0;
    }
    
    if ( ( writeColor && prevWriteColor ) || mergeAlpha )
    {
      instrSlot += 2;
      instructionPair += 2;
      if ( instrSlot >= 16 )
      {
        ri.Printf( 2, "WARNING: more than %i merged operations in atiFragmentShader in shader '%s'\n", 8, shaderParseName );
        return 0;
      }
    }
    if ( !MatchShaderToken( data_p, "(", "fragment instruction of atiFragmentShader" ) )
      return 0;
    instr = (int *)&fragInstructions[instrSlot + writeAlpha];
    if ( !ParseATIFS_FunctionArgs( data_p, (ShaderATIFragmentArgument *)((instr + 4)), argCount, outUsesInterpolator ) )
      return 0;
    if ( !prescale )
    {
      
      scaleTok = Com_Parse( data_p );
      if ( !scaleTok[1] && ( *scaleTok == '*' || *scaleTok == '/' ) )
      {
        isMultiply = *scaleTok == '*';
        numTok = Com_Parse( data_p );
        if ( numTok[1] || ( *numTok != '2' && *numTok != '4' && *numTok != '8' ) )
        {
          ri.Printf( 2, "WARNING: bad scale '%s' in atiFragmentShader in shader '%s'\n", numTok, shaderParseName );
          return 0;
        }
        if ( *numTok == '2' )
          scaleMod = isMultiply ? 1 : 8;
        else if ( *numTok == '4' )
          scaleMod = isMultiply ? 2 : 16;
        else
          scaleMod = isMultiply ? 4 : 32;
      }
      else
      {
        Com_UngetToken();
      }
    }
    if ( clampFlag )
    {
      scaleTok = Com_Parse( data_p );
      if ( _stricmp( scaleTok, ")" ) )
      {
        ri.Printf( 2, "WARNING: %s missing '%s', found '%s' instead in shader '%s'\n", "clamped fragment instruction of atiFragmentShader", ")", scaleTok, shaderParseName );
        return 0;
      }
      scaleMod |= 0x40u;
    }
    if ( !MatchShaderToken( data_p, ";", "fragment instruction of atiFragmentShader" ) )
      return 0;
    *instr = op;
    instr[1] = destReg;
    instr[3] = scaleMod;
    if ( writeColor )
    {
      if ( writeAlpha )
        memcpy( instructionPair, instructionPair + 1, sizeof(*instructionPair) );
      instructionPair->colorMask = colorMask == 6407 ? 0 : colorMask;
    }
    prevWriteColor = writeColor;
    mergeAlpha = writeAlpha || op == 35175;
    Com_ParseSetMark( &mark.currentLine, data_p );
    ParseATIFS_DestReg( data_p, &destReg, &writeColor, &colorMask, &writeAlpha );
    if ( !destReg )
      return 1;
  }
}

/* ---- ParseATIFS_Phase  0x004F7EC0 ----  VERIFIED */
int __cdecl ParseATIFS_Phase(char **data_p, int phase, int phaseIndex, int *outUsesInterpolator)
{
  if ( !ParseATIFS_TexReads( data_p, (ShaderATIRoutingInstruction *)(phase), phaseIndex ) )
    return 0;
  return ParseATIFS_FragOps( data_p, phase + 72, phaseIndex, outUsesInterpolator ) != 0;
}

/* ---- ATIFS_ColorOp  0x004F7EF0 ----  [HIGH] */
void __cdecl ATIFS_ColorOp(int *result)
{
  int op;

  op = *result;
  switch ( op )
  {
    case 35169:
      qglColorFragmentOp1ATI( op, result[1], result[2], result[3], result[4], result[5], result[6] );
      break;
    case 35171:
    case 35172:
    case 35173:
    case 35174:
    case 35175:
      qglColorFragmentOp2ATI(
        op,
        result[1],
        result[2],
        result[3],
        result[4],
        result[5],
        result[6],
        result[7],
        result[8],
        result[9] );
      break;
    case 35176:
    case 35177:
    case 35178:
    case 35179:
    case 35180:
      qglColorFragmentOp3ATI(
        op,
        result[1],
        result[2],
        result[3],
        result[4],
        result[5],
        result[6],
        result[7],
        result[8],
        result[9],
        result[10],
        result[11],
        result[12] );
      break;
    default:
      return;
  }
  return;
}

/* ---- ATIFS_AlphaOp  0x004F7FB0 ----  VERIFIED */
void __cdecl ATIFS_AlphaOp(int *result)
{
  int op;

  op = *result;
  switch ( op )
  {
    case 35169:
      qglAlphaFragmentOp1ATI( op, result[1], result[3], result[4], result[5], result[6] );
      break;
    case 35171:
    case 35172:
    case 35173:
    case 35174:
    case 35175:
      qglAlphaFragmentOp2ATI(
        op,
        result[1],
        result[3],
        result[4],
        result[5],
        result[6],
        result[7],
        result[8],
        result[9] );
      break;
    case 35176:
    case 35177:
    case 35178:
    case 35179:
    case 35180:
      qglAlphaFragmentOp3ATI(
        op,
        result[1],
        result[3],
        result[4],
        result[5],
        result[6],
        result[7],
        result[8],
        result[9],
        result[10],
        result[11],
        result[12] );
      break;
    default:
      return;
  }
  return;
}

/* ---- ParseATIFragmentShader  0x004F8070 ----  VERIFIED */
typedef struct { int sampleTexture; unsigned int source; unsigned int swizzle; } ShaderATITextureRead;
typedef struct { ShaderATITextureRead textureRead[6]; ShaderATIFragmentInstruction instruction[16]; } ShaderATIFragmentPhase;
typedef struct { ShaderATIConstantDefinition constants[8]; ShaderATIFragmentPhase phase[2]; } ShaderATIFragmentShaderState;
int __cdecl ParseATIFragmentShader(char **data_p, int stage, int stageActive)
{
  char *closingToken;
  int shaderId;
  int c;
  int reg;
  int *routeEntry;
  int j;
  int *instrPtr;
  int i;
  int err;
  int phaseCount; 
  int phaseRemaining;
  int phasePtr;
  ShaderATIFragmentShaderState scratch; 

  if ( !rendererShaderRequirements[14] )
  {
    ri.Printf(
      2,
      "WARNING: shader '%s' uses atiFragmentShader without 'requires GL_ATI_fragment_shader'\n",
      shaderParseName );
    return 0;
  }
  if ( !MatchShaderToken( data_p, "{", "start of atiFragmentShader" ) )
    return 0;
  memset( &scratch, 0, sizeof( scratch ) );
  phaseCount = 0;
  if ( !ParseATIFS_ConstDefs( data_p, (int)scratch.constants ) )
    return 0;
  if ( !ParseATIFS_TexReads( data_p, (int)&scratch.phase[0].textureRead, 0 ) )
    return 0;
  if ( !ParseATIFS_FragOps( data_p, (ShaderATIFragmentInstruction *)((int)&scratch.phase[0].instruction), 0, &phaseCount ) )
    return 0;
  if ( phaseCount )
  {
    
    closingToken = Com_Parse( data_p );
    if ( strcmp( closingToken, "}" ) )
    {
      ri.Printf(
        2,
        "WARNING: missing '}' at end of atiFragmentShader... might be because 'col0' or 'col1' was referenced in first phase in shader %s\n",
        shaderParseName );
      return 0;
    }
    phaseCount = 1;
  }
  else
  {
    if ( !ParseATIFS_Phase( data_p, (int)&scratch.phase[1], 1, &phaseCount ) )
      return 0;
    if ( !MatchShaderToken( data_p, "}", "end of atiFragmentShader" ) )
      return 0;
    phaseCount = 2;
  }
  if ( stageActive )
  {
    shaderId = tr_fragmentShaderCount + 1;
    qglGetError();
    qglBindFragmentShaderATI( shaderId );
    qglBeginFragmentShaderATI();
    for ( c = 0; c < 8; c++ )
    {
      if ( scratch.constants[c].defined )
        qglSetFragmentShaderConstantATI( c + 35137, scratch.constants[c].value );
    }
    if ( phaseCount > 0 )
    {
      phasePtr = (int)&scratch.phase[0].instruction[1];
      phaseRemaining = phaseCount;
      do
      {
        reg = 35105;
        routeEntry = (int *)(phasePtr - 120);
        j = 6;
        do
        {
          if ( routeEntry[0] )
          {
            if ( routeEntry[-1] )
              qglSampleMapATI( reg, routeEntry[0], routeEntry[1] );
            else
              qglPassTexCoordATI( reg, routeEntry[0], routeEntry[1] );
          }
          routeEntry += 3;
          ++reg;
          --j;
        }
        while ( j );
        instrPtr = (int *)phasePtr;
        for ( i = 0; i < 8; )
        {
          if ( !instrPtr[-13] && !instrPtr[0] )
            break;
          ATIFS_ColorOp( instrPtr - 13 );
          ATIFS_AlphaOp( instrPtr );
          ++i;
          instrPtr += 26;
        }
        phasePtr += 904;
        --phaseRemaining;
      }
      while ( phaseRemaining );
    }
    qglEndFragmentShaderATI();
    err = qglGetError();
    if ( err )
    {
      qglBindFragmentShaderATI( 0 );
      qglDeleteFragmentShaderATI( shaderId );
      ri.Printf(
        2,
        "WARNING: glGetError() returned 0x%04X when compiling atiFragmentShader in shader %s\n",
        err,
        shaderParseName );
      return 0;
    }
    *(int *)(stage + 1604) = shaderId;
    qglBindFragmentShaderATI( 0 );
    ++tr_fragmentShaderCount;
  }
  return 1;
}

/* ---- ParseVertexProgram  0x004F8310 ----  [HIGH] */
int __cdecl ParseVertexProgram(char **data_p, int stage)
{
  char *token;
  char *vertexProgram;

  token = Com_ParseOnLine( data_p );
  vertexProgram = R_FindVertexProgram( token );
  *(char **)(stage + 1612) = vertexProgram;
  if ( !vertexProgram )
  {
  ri.Printf( 2, "WARNING: couldn't load vertex program '%s' in shader '%s'\n", token, shaderParseName );
    return 0;
  }
  return 1;
}

/* ---- ParseStage  0x004F8390 ----  [HIGH] */
int __cdecl ParseStage(
    int    *stage,
    char  **data_p,
    int     allowTextureName,
    int     imageTrack,
    int    *outStageActive )
{
  char   *token;
  int     bundleIndex;
  int     stageActive;
  int     multiplyImage;
  int     explicitDepthWrite;

  unsigned int depthMaskBits;
  unsigned int srcBlendBits;
  unsigned int dstBlendBits;
  unsigned int alphaTestBits;
  unsigned int depthFuncBits;
  unsigned int lightingBits;
  unsigned int textureShaderBits;          
  unsigned int registerCombinerBits;          
  unsigned int fragmentShaderBits;          
  unsigned int vertexProgramBits;          

  float imageColorScale[4];
  float constColor[3];

  int   i, j, k;
  int   parsedSrcBlend, parsedDstBlend;
  int   cubeImageFlags;
  int   image;
  int   oneShot;
  char *tcModCursor;

  {
  *stage |= 1u;
  stage[36] = 2;
  stage[37] = 8448;
  depthMaskBits = 256;
  stageActive = 1;
  srcBlendBits = 0;
  dstBlendBits = 0;
  alphaTestBits = 0;
  depthFuncBits = 0;
  textureShaderBits = 0;
  registerCombinerBits = 0;
  fragmentShaderBits = 0;
  vertexProgramBits = 0;
  lightingBits = 0;
  bundleIndex = 0;
  explicitDepthWrite = 0;
  multiplyImage = 0;
  }
  imageColorScale[0] = 1.0f;
  imageColorScale[1] = 1.0f;
  imageColorScale[2] = 1.0f;
  imageColorScale[3] = 1.0f;

  memset( rendererShaderRequirements, 0, 15 );

  for ( ;; )
  {

    token = Com_Parse( data_p );

    if ( !*token )
    {
      ri.Printf( 2, "WARNING: no matching '}' found\n" );
      return 0;
    }

    if ( *token == '}' )
    {
      if ( stageActive && !stage[50 * bundleIndex + (1)] )
      {
        ri.Printf( 2, "WARNING: shader '%s' has bundle with no map\n", shaderParseName );
        return 0;
      }

      if ( textureShaderBits )
      {
        for ( i = 0; i < glConfig.maxActiveTextures; ++i )
        {
          int *b = &stage[50 * i + 1];
          if ( !b[38] )
          {
            int *ts = (int *)ri.Hunk_Alloc( 28 );
            b[38] = (int)ts;
            if ( !b[0] )
              *ts = 0;
            else
              *ts = *(int *)( b[0] + 84 );
          }
        }
      }

      if ( !stage[409] )
      {
        if ( !srcBlendBits || srcBlendBits == 2 || srcBlendBits == 5 )
          stage[409] = 1;
        else
          stage[409] = 2;
      }

      if ( srcBlendBits == 2 && dstBlendBits == 16 )
      {
        srcBlendBits  = 0;
        dstBlendBits  = 0;
        depthMaskBits = 256;
      }

      if ( !stage[415] )
      {
        int rgbGen = stage[409];
        if ( rgbGen == 2 || rgbGen == 9 || rgbGen == 10 )
          stage[415] = 1;
      }

      stage[417] = depthMaskBits | srcBlendBits | dstBlendBits | alphaTestBits
                 | depthFuncBits | textureShaderBits | registerCombinerBits
                 | fragmentShaderBits | vertexProgramBits | lightingBits;
      *outStageActive = stageActive;
      return 1;
    }

    if ( registerCombinerBits )
    {
      ri.Printf( 2, "WARNING: nvRegCombiners must be the last thing in a stage\n" );
      return 0;
    }

    if ( !Q_stricmp( token, "map" ) )
    {
      int *outImage = &stage[50 * bundleIndex + 1 + stage[50 * bundleIndex + (33)]];

      if ( !ParseImage( data_p, allowTextureName, stageActive, imageTrack,
                        multiplyImage ? (int)imageColorScale : 0, outImage ) )
        return 0;

      image = *outImage;
      if ( image )
      {
        if ( ( shaderParseLightmapIndex >= 0 && image == tr_lightmaps[shaderParseLightmapIndex] )
          || image == tr_identityLightImage
          || image == tr_dlightImage )
        {
          *( (char *)&stage[50 * bundleIndex] + 200 ) = 1;
        }
        ++stage[50 * bundleIndex + (33)];
        if ( stage[50 * bundleIndex + (33)] > 1 && *(float *)&stage[50 * bundleIndex + (34)] == 0.0f )
        {
          ri.Printf( 2,
            "WARNING: multiple 'map' specifications without preceding 'animmap' in shader'%s'\n",
            shaderParseName );
          return 0;
        }
      }
      continue;
    }

    if ( !Q_stricmp( token, "animMap" )
      || !Q_stricmp( token, "oneshotanimMap" ) )
    {
      oneShot = Q_stricmp( token, "oneshotanimMap" ) == 0;
#pragma inline_depth(0)
      shaderParseSurfaceFlags |= 0x10u;
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2,
          "WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n",
          shaderParseName );
        return 0;
      }
      *(float *)&stage[50 * bundleIndex + 34] = (float)atof( token );
      *( (char *)stage + 202 ) = (char)oneShot;
      Com_SkipRestOfLine( data_p );
      continue;
    }

#pragma inline_depth()
    if ( !Q_stricmp( token, "videoMap" ) )
    {
#pragma inline_depth(0)
      int handle;
      shaderParseSurfaceFlags |= 0x11u;
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2,
          "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n",
          shaderParseName );
        return 0;
      }
      if ( multiplyImage )
      {
        ri.Printf( 2,
          "WARNING: tried to use multiplyImage with videoMap in shader '%s'\n",
          shaderParseName );
        return 0;
      }
      handle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, 26 );
      stage[50 * bundleIndex + (49)] = handle;
      if ( handle != -1 )
      {
        *( (char *)&stage[50 * bundleIndex] + 201 ) = 1;
        stage[50 * bundleIndex + (1)] = tr_scratchImages[handle];
      }
      continue;
    }

#pragma inline_depth()
    if ( !Q_stricmp( token, "cubeMap" ) )
    {
#pragma inline_depth(0)
      cubeImageFlags = 51;
      if ( ( shaderParseFlags & 0x200 ) != 0 )
        cubeImageFlags = 55;
      if ( ( shaderParseFlags & 0x80u ) != 0 )
        cubeImageFlags &= ~1u;
      if ( ( shaderParseFlags & 0x100 ) != 0 )
        cubeImageFlags &= ~2u;

      if ( !rendererShaderRequirements[6] )
      {
        ri.Printf( 2,
          "WARNING: shader '%s' uses cubeMap without 'requires GL_ARB_texture_cube_map'\n",
          shaderParseName );
        return 0;
      }

      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2,
          "WARNING: missing parameter for 'cubeMap' in shader '%s'\n",
          shaderParseName );
        return 0;
      }

      if ( !Q_stricmp( token, "$texturename" ) )
      {
        if ( !allowTextureName )
        {
          ri.Printf( 2,
            "WARNING: $texturename used in shader '%s', which is not a shader type file\n",
            shaderParseName );
          return 0;
        }
        if ( !Q_stricmp( Com_ParseOnLine( data_p ), "+" ) )
          token = Com_ParseOnLine( data_p );
        else
          token = "";
        if ( stageActive )
        {
        image = R_FindImageFile( token, 34067, cubeImageFlags, 10,
                                 multiplyImage ? imageColorScale : 0, 1.0f );
        if ( !image )
        {
          ri.Printf( 2,
            "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n",
            token, shaderParseName );
          return 0;
        }
        stage[50 * bundleIndex + 1 + stage[50 * bundleIndex + (33)]] = image;
        if ( !stage[50 * bundleIndex + (33)] )
          stage[50 * bundleIndex + (36)] = 3;
        ++stage[50 * bundleIndex + (33)];
        if ( stage[50 * bundleIndex + (33)] > 1 && *(float *)&stage[50 * bundleIndex + (34)] == 0.0f )
        {
          ri.Printf( 2,
            "WARNING: multiple 'cubemap' specifications without preceding 'animmap' in shader'%s'\n",
            shaderParseName );
          return 0;
        }
        }
      }
      else if ( stageActive )
      {
        image = R_FindImageFile( token, 34067, cubeImageFlags, imageTrack,
                                 multiplyImage ? imageColorScale : 0, 1.0f );
        if ( !image )
        {
          ri.Printf( 2,
            "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n",
            token, shaderParseName );
          return 0;
        }
        stage[50 * bundleIndex + 1 + stage[50 * bundleIndex + (33)]] = image;
        if ( !stage[50 * bundleIndex + (33)] )
          stage[50 * bundleIndex + (36)] = 3;
        ++stage[50 * bundleIndex + (33)];
        if ( stage[50 * bundleIndex + (33)] > 1 && *(float *)&stage[50 * bundleIndex + (34)] == 0.0f )
        {
          ri.Printf( 2,
            "WARNING: multiple 'cubemap' specifications without preceding 'animmap' in shader'%s'\n",
            shaderParseName );
          return 0;
        }
      }
      Com_SkipRestOfLine( data_p );
      continue;
    }

#pragma inline_depth()
    if ( !Q_stricmp( token, "waterMap" ) )
    {
#pragma inline_depth(0)
      if ( stage[50 * bundleIndex + (33)] )
      {
        ri.Printf( 2,
          "WARNING: multiple 'waterMap' must be only map in shader '%s'\n",
          shaderParseName );
        return 0;
      }
      if ( !ParseWaterMap( (textureBundle_t *)stage, data_p, bundleIndex ) )
        return 0;
      Com_SkipRestOfLine( data_p );
      continue;
    }

    if ( !Q_stricmp( token, "nvTexShader" ) )
    {
      int ok;
      parseInfo->spaceDelimited = qfalse;
      ok = ParseNVTexShader( (textureBundle_t *)(int)stage, data_p, bundleIndex );
      parseInfo->spaceDelimited = qtrue;
      if ( !ok )
        return 0;
      Com_SkipRestOfLine( data_p );
      textureShaderBits = 0x400000;
      continue;
    }

    if ( !Q_stricmp( token, "nvRegCombiners" ) )
    {
      int ok;
      parseInfo->spaceDelimited = qfalse;
      ok = ParseNVRegCombiners( (shaderStage_t *)((int)stage), data_p );
      parseInfo->spaceDelimited = qtrue;
      if ( !ok )
        return 0;
      Com_SkipRestOfLine( data_p );
      registerCombinerBits = 0x800000;
      continue;
    }

    if ( !Q_stricmp( token, "atiFragmentShader" ) )
    {
      int ok;
      parseInfo->spaceDelimited        = qfalse;
      parseInfo->parseNegativeNumbers  = qfalse;
      ok = ParseATIFragmentShader( data_p, (int)stage, stageActive );
      parseInfo->spaceDelimited        = qtrue;
      parseInfo->parseNegativeNumbers  = qtrue;
      if ( !ok )
        return 0;
      Com_SkipRestOfLine( data_p );
      fragmentShaderBits = 0x1000000;
      continue;
    }

    if ( !Q_stricmp( token, "alphaFunc" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'alphaFunc' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2,
          "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n",
          shaderParseName );
        return 0;
      }
      alphaTestBits = NameToAFunc( token );
      continue;
    }

    if ( !Q_stricmp( token, "depthfunc" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'depthFunc' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2,
          "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n",
          shaderParseName );
        return 0;
      }
      if ( !Q_stricmp( token, "lequal" ) )
        depthFuncBits = 0;
      else if ( !Q_stricmp( token, "equal" ) )
        depthFuncBits = 0x20000;
      else if ( !Q_stricmp( token, "always" ) )
        depthFuncBits = 0x40000;
      else
        ri.Printf( 2, "WARNING: unknown depthfunc '%s' in shader '%s'\n",
                   token, shaderParseName );
      continue;
    }

    if ( !Q_stricmp( token, "detail" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'detail' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      *stage |= 2u;
      continue;
    }

    if ( !Q_stricmp( token, "perLight" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'perLight' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      *stage |= 0x80u;
      continue;
    }

    if ( !Q_stricmp( token, "fog" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'fog' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing parm for fog in shader '%s'\n", shaderParseName );
      }
      else if ( !Q_stricmp( token, "on" ) )
      {
        *stage |= 4u;
      }
      else
      {
        *stage &= ~4;
      }
      continue;
    }

    if ( !Q_stricmp( token, "blendfunc" ) )
    {
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing parm for blendFunc in shader '%s'\n",
                   shaderParseName );
        continue;
      }

      if ( !Q_stricmp( token, "add" ) )
      {
        parsedSrcBlend = 2;  parsedDstBlend = 32;
      }
      else if ( !Q_stricmp( token, "filter" ) )
      {
        parsedSrcBlend = 3;  parsedDstBlend = 16;
      }
      else if ( !Q_stricmp( token, "blend" ) )
      {
        parsedSrcBlend = 5;  parsedDstBlend = 96;
      }
      else
      {
        parsedSrcBlend = NameToSrcBlendMode( token );
        token = Com_ParseOnLine( data_p );
        if ( !*token )
        {
          ri.Printf( 2, "WARNING: missing parm for localFunc in shader '%s'\n",
                     shaderParseName );
          continue;
        }
        parsedDstBlend = NameToDstBlendMode( token );
      }

      if ( bundleIndex )
      {
        if ( parsedSrcBlend == 2 && parsedDstBlend == 32 )
        {
          if ( !rendererShaderRequirements[7] )
          {
            ri.Printf( 2,
              "WARNING: shader '%s' uses optional GL_ARB_texture_env_add without suitable 'requires' statement\n",
              shaderParseName );
            return 0;
          }
          stage[50 * bundleIndex + (37)] = 260;
        }
        else if ( ( parsedSrcBlend == 3 && parsedDstBlend == 16 )
               || ( parsedSrcBlend == 1 && parsedDstBlend == 48 ) )
        {
          stage[50 * bundleIndex + (37)] = 8448;
        }
        else if ( parsedSrcBlend == 5 && parsedDstBlend == 96 )
        {
          stage[50 * bundleIndex + (37)] = 8449;
        }
        else
        {
          ri.Printf( 2, "WARNING: bad blendFunc for nextbundle %i in shader '%s'\n",
                     bundleIndex, shaderParseName );
          return 0;
        }
      }
      else
      {
        srcBlendBits = parsedSrcBlend;
        dstBlendBits = parsedDstBlend;
      }

      if ( !explicitDepthWrite )
        depthMaskBits = 0;
      continue;
    }

    if ( !Q_stricmp( token, "rgbGen" ) )
    {
      int constantLighting;

      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'rgbGen' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing parameters for rgbGen in shader '%s'\n",
                   shaderParseName );
        continue;
      }

      lightingBits = 0;
      if ( !Q_stricmp( token, "wave" ) )
      {
        shaderParseSurfaceFlags |= 1u;
        ParseWaveForm( (int)&stage[404], data_p );
        stage[409] = 8;
        continue;
      }

      if ( !Q_stricmp( token, "const" ) || !Q_stricmp( token, "constLighting" ) )
      {
        constantLighting = !Q_stricmp( token, "constLighting" );
        ParseVector( data_p, 3, (float *)constColor );
        if ( constantLighting )
        {
          constColor[0] = tr_identityLight * constColor[0];
          constColor[1] = tr_identityLight * constColor[1];
          constColor[2] = tr_identityLight * constColor[2];
        }
        *( (char *)stage + 1664 ) = (char)(int)( constColor[0] * 255.0f );
        *( (char *)stage + 1665 ) = (char)(int)( constColor[1] * 255.0f );
        *( (char *)stage + 1666 ) = (char)(int)( constColor[2] * 255.0f );
        stage[409] = 12;
        continue;
      }
      if ( !Q_stricmp( token, "identity" ) )
      {
        stage[409] = 2;
        continue;
      }
      if ( !Q_stricmp( token, "identityLighting" ) )
      {
        stage[409] = 1;
        continue;
      }
      if ( !Q_stricmp( token, "entity" ) )
      {
        stage[409] = 3;
        continue;
      }
      if ( !Q_stricmp( token, "oneMinusEntity" ) )
      {
        stage[409] = 4;
        continue;
      }
      if ( !Q_stricmp( token, "vertex" ) )
      {
        shaderParseSurfaceFlags |= 0x40u;
        stage[409] = 6;
        if ( !stage[415] )
          stage[415] = 4;
        continue;
      }
      if ( !Q_stricmp( token, "exactVertex" ) )
      {
        shaderParseSurfaceFlags |= 0x40u;
        stage[409] = 5;
        if ( !stage[415] )
          stage[415] = 4;
        continue;
      }
      if ( !Q_stricmp( token, "lightingAmbient" ) )
      {
        stage[409]   = 9;
        lightingBits = 0x100000;
        continue;
      }
      if ( !Q_stricmp( token, "lightingDiffuse" ) )
      {
        stage[409]   = 10;
        lightingBits = 0x100000;
        continue;
      }
      if ( !Q_stricmp( token, "oneMinusVertex" ) )
      {
        shaderParseSurfaceFlags |= 0x40u;
        stage[409] = 7;
        continue;
      }
      if ( !Q_stricmp( token, "lightingPrecalc" ) )
      {
        stage[409] = 11;
        continue;
      }
      {
        ri.Printf( 2, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n",
                   token, shaderParseName );
        Com_SkipRestOfLine( data_p );
      }
      continue;
    }

    if ( !Q_stricmp( token, "alphaGen" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'alphaGen' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing parameters for alphaGen in shader '%s'\n",
                   shaderParseName );
        continue;
      }

      if ( !Q_stricmp( token, "wave" ) )
      {
        shaderParseSurfaceFlags |= 1u;
        ParseWaveForm( (int)&stage[410], data_p );
        stage[415] = 7;
        continue;
      }
      if ( !Q_stricmp( token, "const" ) )
      {
        *( (char *)stage + 1667 ) =
            (char)(int)( atof( Com_ParseOnLine( data_p ) ) * 255.0 );
        stage[415] = 9;
        continue;
      }
      if ( !Q_stricmp( token, "constLighting" ) )
      {
        *( (char *)stage + 1667 ) =
            (char)(int)( atof( Com_ParseOnLine( data_p ) ) * tr_identityLight * 255.0 );
        stage[415] = 9;
        continue;
      }
      if ( !Q_stricmp( token, "identity" ) )
      {
        stage[415] = 0;
        continue;
      }
      if ( !Q_stricmp( token, "entity" ) )
      {
        stage[415] = 2;
        continue;
      }
      if ( !Q_stricmp( token, "oneMinusEntity" ) )
      {
        stage[415] = 3;
        continue;
      }
      if ( !Q_stricmp( token, "vertex" ) )
      {
        shaderParseSurfaceFlags |= 0x40u;
        stage[415] = 4;
        continue;
      }
      if ( !Q_stricmp( token, "lightingSpecular" ) )
      {
        stage[415] = 6;
        continue;
      }
      if ( !Q_stricmp( token, "oneMinusVertex" ) )
      {
        shaderParseSurfaceFlags |= 0x40u;
        stage[415] = 5;
        continue;
      }
      if ( !Q_stricmp( token, "portal" ) )
      {
        stage[415] = 8;
        token = Com_ParseOnLine( data_p );
        if ( token[0] == 0 )
        {
          shaderParsePortalRange = 1132462080;
          ri.Printf( 2,
            "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n",
            shaderParseName );
        }
        else
        {
          *(float *)&shaderParsePortalRange = (float)atof( token );
        }
        continue;
      }
      if ( !Q_stricmp( token, "dot" ) )
      {
        shaderParseSurfaceFlags |= 2u;
        stage[415] = 10;
        continue;
      }
      if ( !Q_stricmp( token, "oneMinusDot" ) )
      {
        shaderParseSurfaceFlags |= 2u;
        stage[415] = 11;
        continue;
      }
      if ( !Q_stricmp( token, "onePlusDot" ) )
      {
        shaderParseSurfaceFlags |= 2u;
        stage[415] = 12;
        continue;
      }
      if ( !Q_stricmp( token, "negativeDot" ) )
      {
        shaderParseSurfaceFlags |= 2u;
        stage[415] = 13;
        continue;
      }
      {
        ri.Printf( 2, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n",
                   token, shaderParseName );
      }
      continue;
    }

    if ( !Q_stricmp( token, "texgen" ) || !Q_stricmp( token, "tcGen" ) )
    {
      if ( !stageActive )
      {
        Com_SkipRestOfLine( data_p );
        continue;
      }
      if ( !stage[50 * bundleIndex + (1)] )
      {
        ri.Printf( 2, "WARNING: texgen before image in shader '%s'\n", shaderParseName );
        return 0;
      }
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing texgen parm in shader '%s'\n", shaderParseName );
        continue;
      }

      if ( !Q_stricmp( token, "normal" ) )
      {
        shaderParseSurfaceFlags |= 2u;   stage[50 * bundleIndex + (40)] = 7;  stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "tangent" ) )
      {
        shaderParseSurfaceFlags |= 4u;   stage[50 * bundleIndex + (40)] = 8;  stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "binormal" ) )
      {
        shaderParseSurfaceFlags |= 8u;   stage[50 * bundleIndex + (40)] = 9;  stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "tbn_x" ) )
      {
        shaderParseSurfaceFlags |= 0xEu; stage[50 * bundleIndex + (40)] = 10; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "tbn_y" ) )
      {
        shaderParseSurfaceFlags |= 0xEu; stage[50 * bundleIndex + (40)] = 11; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "tbn_z" ) )
      {
        shaderParseSurfaceFlags |= 0xEu; stage[50 * bundleIndex + (40)] = 12; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "eyeToVertex" ) )
      {
        stage[50 * bundleIndex + (40)] = 14; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "vertexToEye" ) )
      {
        stage[50 * bundleIndex + (40)] = 13; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "reflection" ) )
      {
        shaderParseSurfaceFlags |= 2u;   stage[50 * bundleIndex + (40)] = 15; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "lightvector" ) )
      {
        stage[50 * bundleIndex + (40)] = 16; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "lighthalfangle" ) )
      {
        stage[50 * bundleIndex + (40)] = 17; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "sunhalfangle" ) )
      {
        stage[50 * bundleIndex + (40)] = 18; stage[50 * bundleIndex + (36)] = 3;
      }
      else if ( !Q_stricmp( token, "nv_dot_product_reflect_cube_map_eye_from_qs_1of3" ) )
      {
        shaderParseSurfaceFlags |= 0xEu; stage[50 * bundleIndex + (40)] = 19; stage[50 * bundleIndex + (36)] = 4;
      }
      else if ( !Q_stricmp( token, "nv_dot_product_reflect_cube_map_eye_from_qs_2of3" ) )
      {
        shaderParseSurfaceFlags |= 0xEu; stage[50 * bundleIndex + (40)] = 20; stage[50 * bundleIndex + (36)] = 4;
      }
      else if ( !Q_stricmp( token, "nv_dot_product_reflect_cube_map_eye_from_qs_3of3" ) )
      {
        shaderParseSurfaceFlags |= 0xEu; stage[50 * bundleIndex + (40)] = 21; stage[50 * bundleIndex + (36)] = 4;
      }
      else if ( !Q_stricmp( token, "environment" ) )
      {
        shaderParseSurfaceFlags |= 2u;   stage[50 * bundleIndex + (40)] = 4;
      }
      else if ( !Q_stricmp( token, "lightmap" ) )
      {
        shaderParseSurfaceFlags |= 0x20u; stage[50 * bundleIndex + (40)] = 2;
      }
      else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )
      {
        shaderParseSurfaceFlags |= 0x10u; stage[50 * bundleIndex + (40)] = 3;
      }
      else if ( !Q_stricmp( token, "vector" ) )
      {
        ParseVector( data_p, 3, (float *)&stage[50 * bundleIndex + (41)] );
        ParseVector( data_p, 3, (float *)&stage[50 * bundleIndex + (44)] );
        stage[50 * bundleIndex + (40)] = 6;
      }
      else
      {
        ri.Printf( 2, "WARNING: unknown or invalid texgen parm '%s' in shader '%s'\n",
                   token, shaderParseName );
      }
      continue;
    }

    if ( !Q_stricmp( token, "tcMod" ) )
    {
      char tcModText[1024] = "";
      for ( ;; )
      {
        token = Com_ParseOnLine( data_p );
        if ( !*token )
          break;
        strcat( tcModText, token );
        strcat( tcModText, " " );
      }
      if ( stageActive )
      {
        tcModCursor = tcModText;
        ParseTexMod( bundleIndex, (textureBundle_t *)stage, tcModCursor );
      }
      continue;
    }

    if ( !Q_stricmp( token, "depthwrite" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'depthwrite' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      depthMaskBits      = 256;
      explicitDepthWrite = 1;
      continue;
    }

    if ( !Q_stricmp( token, "nextbundle" ) )
    {
      if ( stageActive && !stage[50 * bundleIndex + (1)] )
      {
        ri.Printf( 2, "WARNING: shader '%s' has bundle with no map\n", shaderParseName );
        return 0;
      }

      ++bundleIndex;
      if ( bundleIndex >= 2 )
      {
        for ( k = bundleIndex; ; ++k )
        {
          if ( k >= 8 )
          {
          ri.Printf( 2,
            "WARNING: shader '%s' has %i or more bundles without suitable 'requires GL_MAX_TEXTURE_UNITS_ARB\n",
            shaderParseName, bundleIndex + 1 );
          return 0;
          }
          if ( rendererShaderRequirements[k - 2] )
            break;
        }
      }
      stage[50 * bundleIndex + (36)] = 2;
      stage[50 * bundleIndex + (37)] = 8448;
      imageColorScale[0] = 1.0f;
      imageColorScale[1] = 1.0f;
      imageColorScale[2] = 1.0f;
      imageColorScale[3] = 1.0f;
      multiplyImage = 0;
      continue;
    }

    if ( !Q_stricmp( token, "requires" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'requires' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      if ( stage[1] )
      {
        ri.Printf( 2,
          "WARNING: 'requires' should be before textures are specified in shader '%s'\n",
          shaderParseName );
        return 0;
      }
      stageActive &= ParseStageRequirements( data_p, 0 );
      continue;
    }

    if ( !Q_stricmp( token, "texEnvCombine" ) )
    {
      int ok;
      if ( !rendererShaderRequirements[8] )
      {
        ri.Printf( 2,
          "WARNING: shader '%s' uses texEnvCombine without 'requires GL_ARB_texture_env_combine'\n",
          shaderParseName );
        return 0;
      }
      parseInfo->spaceDelimited = qfalse;
      ok = ParseTextureEnvCombine( (ShaderTextureImageData *)&stage[50 * bundleIndex + 1], data_p );
      parseInfo->spaceDelimited = qtrue;
      if ( !ok )
        return 0;
      continue;
    }

    if ( !Q_stricmp( token, "vertexProgram" ) )
    {
      if ( bundleIndex )
      {
        ri.Printf( 2, "WARNING: 'vertexProgram' not allowed in nextbundle in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      if ( !rendererShaderRequirements[10] )
      {
        ri.Printf( 2,
          "WARNING: shader '%s' uses vertexProgram without 'requires GL_ARB_vertex_program'\n",
          shaderParseName );
        return 0;
      }
      vertexProgramBits = 0x2000000;
      if ( !ParseVertexProgram( data_p, (int)stage ) )
        return 0;
      shaderParseSurfaceFlags |= 0x102u;
      *stage |= 0x20000u;
      continue;
    }

    if ( !Q_stricmp( token, "multiplyImage" ) )
    {
      if ( stage[50 * bundleIndex + (1)] )
      {
        ri.Printf( 2,
          "WARNING: shader '%s' uses 'multiplyImage' after the images are defined\n",
          shaderParseName );
        return 0;
      }

      for ( j = 0; j < 4; ++j )
      {
        token = Com_ParseOnLine( data_p );
        if ( !*token )
          break;
        if ( !_stricmp( token, "identityLighting" ) )
        {
          imageColorScale[j] = tr_identityLight;
        }
        else
        {
          char c = *token;
          if ( ( c < '0' || c > '9' ) && c != '.' && c != '+' )
          {
            ri.Printf( 2,
              "WARNING: argument '%s' to multiplyImage in shader '%s' should be 'identityLighting' or a number >= 0\n",
              token, shaderParseName );
            return 0;
          }
          imageColorScale[j] = (float)atof( token );
        }
      }

      if ( !j )
      {
        ri.Printf( 2, "WARNING: missing arguments to 'multiplyImage' in shader '%s'\n",
                   shaderParseName );
        return 0;
      }
      if ( j == 2 )
        imageColorScale[3] = imageColorScale[2];
      else if ( j != 4 )
        imageColorScale[3] = 1.0f;
      if ( j < 3 )
      {
        imageColorScale[1] = imageColorScale[0];
        imageColorScale[2] = imageColorScale[0];
      }
      multiplyImage = 1;
      continue;
    }

    ri.Printf( 2, "WARNING: unknown parameter '%s' in shader '%s'\n",
               token, shaderParseName );
    return 0;
  }
#pragma inline_depth()
}


/* ---- ParseDeform  0x004F9D20 ----  [HIGH]  (the text cursor stays in the by-value argument: retail rereads its stack slot) */
typedef struct { char **cursor; } ShaderTextCursorArgument;
void __cdecl ParseDeform(ShaderTextCursorArgument cursorArgument)
{
  float *deform;
  char  *token;
  int    prevNumDeforms;
  int textNum;
  double scale;
  float mag;
  int    count;

  token = Com_ParseOnLine( cursorArgument.cursor );

  if ( !token[0] )
  {
    ri.Printf( 2, "WARNING: missing deform parm in shader '%s'\n", shaderParseName );
    return;
  }

  prevNumDeforms = shaderParseNumDeforms;
  if ( shaderParseNumDeforms == 3 )
  {
    ri.Printf( 2, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shaderParseName );
    return;
  }

  deform = (float *)( (char *)&shaderParseDeforms + 52 * shaderParseNumDeforms );

  if ( !Q_stricmp( token, "projectionShadow" ) )
  {
    *(int *)deform = 8;
    ++shaderParseNumDeforms;
    return;
  }
  if ( !Q_stricmp( token, "autosprite" ) )
  {
    *(int *)deform = 9;
    ++shaderParseNumDeforms;
    return;
  }
  if ( !Q_stricmp( token, "autosprite2" ) )
  {
    *(int *)deform = 10;
    ++shaderParseNumDeforms;
    return;
  }
  if ( !Q_stricmpn( token, "text", 4 ) )
  {
    textNum = token[4] - 48;
    if ( textNum < 0 || textNum > 7 )
      textNum = 0;
    *(int *)deform = textNum + 11;
    ++shaderParseNumDeforms;
    return;
  }
  if ( !Q_stricmp( token, "bulge" ) )
  {
    shaderParseSurfaceFlags |= 0x13u;
    token = Com_ParseOnLine( cursorArgument.cursor );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shaderParseName );
      return;
    }
    deform[10] = atof( token );
    token = Com_ParseOnLine( cursorArgument.cursor );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shaderParseName );
      return;
    }
    deform[11] = atof( token );
    token = Com_ParseOnLine( cursorArgument.cursor );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shaderParseName );
      return;
    }
    deform[12] = atof( token );
    shaderParseBoundsExpansion = fabs( deform[11] ) + shaderParseBoundsExpansion;
    *(int *)deform = 6;
    ++shaderParseNumDeforms;
    return;
  }

  if ( !Q_stricmp( token, "wave" ) )
  {
    shaderParseSurfaceFlags |= 3u;
    token = Com_ParseOnLine( cursorArgument.cursor );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing deformVertexes parm in shader '%s'\n", shaderParseName );
      return;
    }
    if ( atof( token ) != 0 )
    {
      deform[9] = 1.0f / (float)atof( token );
    }
    else
    {
      deform[9] = 100.0f;
      ri.Printf( 2, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shaderParseName );
    }
    ParseWaveForm( (int)(deform + 4), cursorArgument.cursor );
    shaderParseBoundsExpansion = MaxWaveFormDeformation( (int)(deform + 4) ) + shaderParseBoundsExpansion;
    *(int *)deform = 1;
    ++shaderParseNumDeforms;
  }
  else if ( !Q_stricmp( token, "flap" ) )
  {
    shaderParseSurfaceFlags |= 0x13u;
    token = Com_ParseOnLine( cursorArgument.cursor );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: missing flap axis in shader '%s'\n", shaderParseName );
      return;
    }
    if ( !Q_stricmp( token, "s" ) || !Q_stricmp( token, "x" ) )
    {
      *(int *)deform = 2;
    }
    else if ( !Q_stricmp( token, "t" ) || !Q_stricmp( token, "y" ) )
    {
      *(int *)deform = 3;
    }
    else
    {
      ri.Printf( 2, "WARNING: flap axis must be one of (s, t, x, y) in shader '%s'\n", shaderParseName );
      return;
    }
    token = Com_ParseOnLine( cursorArgument.cursor );
    if ( atof( token ) != 0 )
    {
      deform[9] = 1.0f / (float)atof( token );
    }
    else
    {
      deform[9] = 100.0f;
      ri.Printf( 2, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shaderParseName );
    }
    ParseWaveForm( (int)(deform + 4), cursorArgument.cursor );
    shaderParseBoundsExpansion = MaxWaveFormDeformation( (int)(deform + 4) ) + shaderParseBoundsExpansion;
    ++shaderParseNumDeforms;
  }
  else if ( !Q_stricmp( token, "normal" ) )
  {
    shaderParseSurfaceFlags |= 3u;
    token = Com_ParseOnLine( cursorArgument.cursor );
    if ( *token )
    {
      deform[6] = atof( token );
      token = Com_ParseOnLine( cursorArgument.cursor );
      if ( *token )
      {
        deform[8] = atof( token );
        *(int *)deform = 4;
        ++shaderParseNumDeforms;
        return;
      }
    }
    ri.Printf( 2, "WARNING: missing deformVertexes parm in shader '%s'\n", shaderParseName );
  }
  else if ( !Q_stricmp( token, "syncnormal" ) )
  {
    shaderParseSurfaceFlags |= 3u;
    if ( prevNumDeforms && *(int *)(deform - 13) == 1 )
    {
      memcpy( deform, deform - 13, 0x34u );
      token = Com_ParseOnLine( cursorArgument.cursor );
      if ( *token )
      {
        scale = atof( token ) * deform[6];
        *(int *)deform = 5;
        deform[6] = scale;
        ++shaderParseNumDeforms;
      }
      else
      {
        ri.Printf( 2, "WARNING: missing scale to syncNormal in shader '%s'\n", shaderParseName );
      }
    }
    else
    {
      ri.Printf(
        2,
        "WARNING: deformVertexes syncNormal must follow deformVertexes wave in shader '%s'\n",
        shaderParseName );
    }
  }
  else if ( !Q_stricmp( token, "move" ) )
  {
    shaderParseSurfaceFlags |= 1u;
    for ( count = 0; count < 3; count++ )
    {
      token = Com_ParseOnLine( cursorArgument.cursor );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing deformVertexes parm in shader '%s'\n", shaderParseName );
        return;
      }
      deform[1 + count] = atof( token );
    }
    ParseWaveForm( (int)(deform + 4), cursorArgument.cursor );
    mag = sqrt( deform[1] * deform[1] + deform[2] * deform[2] + deform[3] * deform[3] );
    shaderParseBoundsExpansion = MaxWaveFormDeformation( (int)(deform + 4) ) * mag + shaderParseBoundsExpansion;
    *(int *)deform = 7;
    ++shaderParseNumDeforms;
  }
  else
  {
    ri.Printf( 2, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shaderParseName );
  }
}

/* ---- ParseSkyParms  0x004FA320 ----  VERIFIED */
void __cdecl ParseSkyParms(char **data_p, int imageTrack)
{
  char  *token;
  int    i, j;
  int    image;
  float cloudHeight;
  char   filename[64];

  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    if ( parseInfo->spaceDelimited == qfalse )
    {
      token = parseInfo->token;
      goto sky_first_token;
    }
    *data_p = parseInfo->ungetTokenSave;
    parseInfo->currentLine = parseInfo->ungetLineSave;
  }
  token = Com_ParseExt( data_p, qfalse );
sky_first_token:

  if ( !*token )
  {
    ri.Printf( 2, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shaderParseName );
    return;
  }

  if ( strcmp( token, "-" ) )
  {
    for ( i = 0; i < 6; ++i )
    {
      Com_sprintf( filename, 64, "%s_%s.tga", token, skySuffix[i] );
      image = R_FindImageFile( filename, 3553, 51, imageTrack, 0, 1.0f );
      skyOuterBox[i] = image;
      if ( !image )
        skyOuterBox[i] = tr_defaultImage;
    }
  }

  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    if ( parseInfo->spaceDelimited == qfalse )
      token = parseInfo->token;
    else
    {
      *data_p = parseInfo->ungetTokenSave;
      parseInfo->currentLine = parseInfo->ungetLineSave;
      token = Com_ParseExt( data_p, qfalse );
    }
  }
  else
    token = Com_ParseExt( data_p, qfalse );

  if ( !token[0] )
  {
    ri.Printf( 2, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shaderParseName );
    return;
  }

  cloudHeight = atof( token );
  shaderParseSkyCloudHeight = cloudHeight;
  if ( cloudHeight == 0.0f )
    shaderParseSkyCloudHeight = 512.0f;
  R_InitSkyTexCoords( shaderParseSkyCloudHeight );

  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    if ( parseInfo->spaceDelimited == qfalse )
      token = parseInfo->token;
    else
    {
      *data_p = parseInfo->ungetTokenSave;
      parseInfo->currentLine = parseInfo->ungetLineSave;
      token = Com_ParseExt( data_p, qfalse );
    }
  }
  else
    token = Com_ParseExt( data_p, qfalse );

  if ( !token[0] )
  {
    ri.Printf( 2, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shaderParseName );
    return;
  }

  if ( strcmp( token, "-" ) )
  {
    for ( j = 0; j < 6; ++j )
    {
      Com_sprintf( filename, 64, "%s_%s.tga", token, skySuffix[j] );
      image = R_FindImageFile( filename, 3553, 51, imageTrack, 0, 1.0f );
      skyInnerBox[j] = image;
      if ( !image )
        skyInnerBox[j] = tr_defaultImage;
    }
  }

  shaderParseFlags |= 8u;
}

/* ---- ParseSort  0x004FA580 ----  VERIFIED */
int ParseSort( char **data_p )                              /* EAX */
{
	char  *token;
	double sortValue;

	token = Com_ParseOnLine( data_p );                  /* 0x004FA581 */
	if ( !token[0] ) {
		ri.Printf( 2, "WARNING: missing sort parameter in shader '%s'\n",
		           shaderParseName );
		return 0;
	}

	if ( !Q_stricmpn( token, "portal",     99999 ) ) { shaderParseSort = 0x3F800000; return 1; }
	if ( !Q_stricmpn( token, "sky",        99999 ) ) { shaderParseSort = 0x40000000; return 1; }
	if ( !Q_stricmpn( token, "ocean",      99999 ) ) { shaderParseSort = 0x40400000; return 1; }
	if ( !Q_stricmpn( token, "boathull",   99999 ) ) { shaderParseSort = 0x40800000; return 1; }
	if ( !Q_stricmp ( token, "opaque"          ) )   { shaderParseSort = 0x40A00000; return 1; }
	if ( !Q_stricmp ( token, "decal"           ) )   { shaderParseSort = 0x40C00000; return 1; }
	if ( !Q_stricmp ( token, "seeThrough"      ) )   { shaderParseSort = 0x40E00000; return 1; }
	if ( !Q_stricmp ( token, "banner"          ) )   { shaderParseSort = 0x41000000; return 1; }
	if ( !Q_stricmp ( token, "underwater"      ) )   { shaderParseSort = 0x41200000; return 1; }
	if ( !Q_stricmp ( token, "water"           ) )   { shaderParseSort = 0x41300000; return 1; }
	if ( !Q_stricmp ( token, "corona"          ) )   { shaderParseSort = 0x41400000; return 1; }
	if ( !Q_stricmp ( token, "innerBlend"      ) )   { shaderParseSort = 0x41500000; return 1; }
	if ( !Q_stricmp ( token, "outerBlend"      ) )   { shaderParseSort = 0x41600000; return 1; }
	if ( !Q_stricmp ( token, "blend"           ) )   { shaderParseSort = 0x41700000; return 1; }
	if ( !Q_stricmp ( token, "blend2"          ) )   { shaderParseSort = 0x41800000; return 1; }
	if ( !Q_stricmp ( token, "blend3"          ) )   { shaderParseSort = 0x41880000; return 1; }
	if ( !Q_stricmp ( token, "blend4"          ) )   { shaderParseSort = 0x41900000; return 1; }
	if ( !Q_stricmp ( token, "additive"        ) )   { shaderParseSort = 0x41980000; return 1; }
	if ( !Q_stricmp ( token, "nearest"         ) )   { shaderParseSort = 0x41B00000; return 1; }

	/* 0x004FA86B */
	sortValue = atof( token );                                  /* 0x004FA86C */
	*(float *)&shaderParseSort = (float)sortValue;              /* 0x004FA871 fst (ST0 stays unrounded) */
	if ( FCAST( sortValue ) != 0.0f || token[0] == '0' ) {     /* 0x004FA877 fcomp flt_568E64 on ST0 ; 0x004FA88B cmp [esi],'0' */
		return 1;
	}
	ri.Printf( 2, "WARNING: bad shader sort '%s'\n", token );
	return 0;
}

/* cl_surfaceParmsByType  0x00571790: the surface-type table the client indexes by type
 * (CL_SurfaceTypeFromName / the type-name lookup in client_mp/cl_cgame_mp.c) and the brushmask
 * formatter in tr_cmds.c reads.  Retail places it immediately before surfacetype_bark, whose first
 * 23 entries it repeats after its own entry 0. */
tr_infoParm_t cl_surfaceParmsByType[24] = {   /* 0x00571790 */
    { (const char *) 13, -1, -1, 0 },
    { "bark",        0, 0x00100000, 0x00 },
    { "brick",       0, 0x00200000, 0x00 },
    { "carpet",      0, 0x00300000, 0x00 },
    { "cloth",       0, 0x00400000, 0x00 },
    { "concrete",    0, 0x00500000, 0x00 },
    { "dirt",        0, 0x00600000, 0x00 },
    { "flesh",       0, 0x00700000, 0x00 },
    { "foliage",     1, 0x00800000, 0x02 },
    { "glass",       1, 0x00900000, 0x10 },
    { "grass",       0, 0x00A00000, 0x00 },
    { "gravel",      0, 0x00B00000, 0x00 },
    { "ice",         0, 0x00C00000, 0x00 },
    { "metal",       0, 0x00D00000, 0x00 },
    { "mud",         0, 0x00E00000, 0x00 },
    { "paper",       0, 0x00F00000, 0x00 },
    { "plaster",     0, 0x01000000, 0x00 },
    { "rock",        0, 0x01100000, 0x00 },
    { "sand",        0, 0x01200000, 0x00 },
    { "snow",        0, 0x01300000, 0x00 },
    { "water",       1, 0x01400000, 0x20 },
    { "wood",        0, 0x01500000, 0x00 },
    { "asphalt",     0, 0x01600000, 0x00 },
    { "opaqueglass", 0, 0x00900000, 0x00 }
};

#define tr_surfaceParms   ((const tr_infoParm_t *)(const void *)&surfacetype_bark)

/* === Merged from renderer/tr_shader_rdata.c (surfaceParm table) (retail linked it as tr_*.obj .rdata/.data). === */

/* ==========================================================================
 * surfacetype_bark        0x005717A0        848 bytes (53 x 16)
 * ==========================================================================
 *
 * The `surfaceparm` keyword table.  ParseSurfaceParm (0x004FA8B0) walks it:
 *
 *   0x004FA903  mov esi, offset surfacetype_bark   -- walk cursor
 *   0x004FA920  mov ecx, [esi+10h]                 -- next entry's NAME
 *   0x004FA923  add esi, 10h                       -- stride is 16
 *   0x004FA927  test ecx,ecx / jnz                 -- NULL name terminates
 *   0x004FA935  shl ebx, 4                         -- index * 16
 *   0x004FA938  mov eax, dword_5717A8[ebx]         -- base+8 of entry ebx
 *   0x004FA93E  or  ecx, eax                       -- into shaderParseSurfaceParmFlags
 *
 * So: stride 16, name at +0, and the field the parser consumes is at +8 --
 * RTCW's infoParm_t { name, clearSolid, surfaceFlags, contents }.
 * CL_SurfaceTypeFromName (client_mp/cl_cgame_mp.c) and the brushmask
 * formatter in renderer/tr_cmds.c walk the same table.
 *
 * RTCW's ParseSurfaceParm applies all three payload fields:
 *
 *     if ( infoParms[i].clearSolid ) shader.contentFlags &= ~CONTENTS_SOLID;
 *     shader.surfaceFlags |= infoParms[i].surfaceFlags;
 *     shader.contentFlags |= infoParms[i].contents;
 *
 * CoD1 0x004FA92F-0x004FA946 applies ONLY the +8 word, into a single global.
 * `clearSolid` and `contents` have no reader in the renderer -- they are what
 * the BSP compiler and the collision side of the engine consume.
 *
 * ENTRIES 0..22 are the CoD surface types: the +8 word is (type << 20), so
 * bark is 1 and asphalt is 22, matching CL_SurfaceTypeFromName's
 * `(surfaceFlags >> 20) & 0x1F`.  Entry 22 "opaqueglass" reuses glass's type 9
 * -- the binary holds 0x00900000 twice.  Entries 23..51 are the flag
 * keywords.  Entry 52 is the NULL terminator the walk tests for at 0x004FA927.
 *
 * SIZE.  0x00571900 also carries the name `surfacetype_Opaqueglass`, which
 * is why a size measured to the next name gives surfacetype_bark 352 bytes, and IDA
 * names the +8 column a second time (dword_5717A8); both overlap this object in
 * retail and cannot in C.  The true
 * extent is 0x005717A0..0x00571AEF inclusive: the sixteen bytes at 0x00571AE0
 * are the terminator, and 0x00571AF0 holds 0x00000080, unrelated .data.
 */
tr_infoParm_t surfacetype_bark[53] = {
    { "bark",          0, 0x00100000, 0x00000000 },   /* 0x005717A0 */
    { "brick",         0, 0x00200000, 0x00000000 },   /* 0x005717B0 */
    { "carpet",        0, 0x00300000, 0x00000000 },   /* 0x005717C0 */
    { "cloth",         0, 0x00400000, 0x00000000 },   /* 0x005717D0 */
    { "concrete",      0, 0x00500000, 0x00000000 },   /* 0x005717E0 */
    { "dirt",          0, 0x00600000, 0x00000000 },   /* 0x005717F0 */
    { "flesh",         0, 0x00700000, 0x00000000 },   /* 0x00571800 */
    { "foliage",       1, 0x00800000, 0x00000002 },   /* 0x00571810 */
    { "glass",         1, 0x00900000, 0x00000010 },   /* 0x00571820 */
    { "grass",         0, 0x00A00000, 0x00000000 },   /* 0x00571830 */
    { "gravel",        0, 0x00B00000, 0x00000000 },   /* 0x00571840 */
    { "ice",           0, 0x00C00000, 0x00000000 },   /* 0x00571850 */
    { "metal",         0, 0x00D00000, 0x00000000 },   /* 0x00571860 */
    { "mud",           0, 0x00E00000, 0x00000000 },   /* 0x00571870 */
    { "paper",         0, 0x00F00000, 0x00000000 },   /* 0x00571880 */
    { "plaster",       0, 0x01000000, 0x00000000 },   /* 0x00571890 */
    { "rock",          0, 0x01100000, 0x00000000 },   /* 0x005718A0 */
    { "sand",          0, 0x01200000, 0x00000000 },   /* 0x005718B0 */
    { "snow",          0, 0x01300000, 0x00000000 },   /* 0x005718C0 */
    { "water",         1, 0x01400000, 0x00000020 },   /* 0x005718D0 */
    { "wood",          0, 0x01500000, 0x00000000 },   /* 0x005718E0 */
    { "asphalt",       0, 0x01600000, 0x00000000 },   /* 0x005718F0 */
    { "opaqueglass",   0, 0x00900000, 0x00000000 },   /* 0x00571900 */
    { "clipmissile",   1, 0x00000000, 0x00000080 },   /* 0x00571910 */
    { "ai_nosight",    1, 0x00000000, 0x00001000 },   /* 0x00571920 */
    { "clipshot",      1, 0x00000000, 0x00002000 },   /* 0x00571930 */
    { "playerclip",    1, 0x00000000, 0x00010000 },   /* 0x00571940 */
    { "monsterclip",   1, 0x00000000, 0x00020000 },   /* 0x00571950 */
    { "vehicleclip",   1, 0x00000000, 0x00000200 },   /* 0x00571960 */
    { "itemclip",      1, 0x00000000, 0x00000400 },   /* 0x00571970 */
    { "nodrop",        1, 0x00000000, (int)0x80000000 }, /* 0x00571980 */
    { "nonsolid",      1, 0x00004000, 0x00000000 },   /* 0x00571990 */
    { "origin",        1, 0x00000000, 0x01000000 },   /* 0x005719A0 */
    { "detail",        0, 0x00000000, 0x08000000 },   /* 0x005719B0 */
    { "structural",    0, 0x00000000, 0x10000000 },   /* 0x005719C0 */
    { "portal",        1, (int)0x80000000, 0x00000000 }, /* 0x005719D0 */
    { "canshootclip",  0, 0x00000000, 0x00000040 },   /* 0x005719E0 */
    { "sky",           0, 0x00000004, 0x00000800 },   /* 0x005719F0 */
    { "lightfilter",   0, 0x00008000, 0x00000000 },   /* 0x00571A00 */
    { "alphashadow",   0, 0x00010000, 0x00000000 },   /* 0x00571A10 */
    { "castshadow",    0, 0x00040000, 0x00000000 },   /* 0x00571A20 */
    { "hint",          0, 0x00000100, 0x00000000 },   /* 0x00571A30 */
    { "slick",         0, 0x00000002, 0x00000000 },   /* 0x00571A40 */
    { "noimpact",      0, 0x00000010, 0x00000000 },   /* 0x00571A50 */
    { "nomarks",       0, 0x00000020, 0x00000000 },   /* 0x00571A60 */
    { "ladder",        0, 0x00000008, 0x00000000 },   /* 0x00571A70 */
    { "nodamage",      0, 0x00000001, 0x00000000 },   /* 0x00571A80 */
    { "nosteps",       0, 0x00002000, 0x00000000 },   /* 0x00571A90 */
    { "nodraw",        0, 0x00000080, 0x00000000 },   /* 0x00571AA0 */
    { "pointlight",    0, 0x00000800, 0x00000000 },   /* 0x00571AB0 */
    { "nolightmap",    0, 0x00000400, 0x00000000 },   /* 0x00571AC0 */
    { "nodlight",      0, 0x00020000, 0x00000000 },   /* 0x00571AD0 */
    { 0,               0, 0x00000000, 0x00000000 }    /* 0x00571AE0  terminator */
};

/* ---- ParseSurfaceParm  0x004FA8B0 ----  VERIFIED */
void __cdecl ParseSurfaceParm(char **data_p)
{
  const char *token;
  int index;
  const tr_infoParm_t *entry;
  if ( parseInfo->ungetReady )
  {
    parseInfo->ungetReady = qfalse;
    if ( parseInfo->spaceDelimited == qfalse )
    {
      token = parseInfo->token;
      goto surface_token;
    }
    *data_p = parseInfo->ungetTokenSave;
    parseInfo->currentLine = parseInfo->ungetLineSave;
  }
  token = Com_ParseExt( data_p, qfalse );
surface_token:
  entry = (const tr_infoParm_t *)(const void *)&surfacetype_bark;
  for ( index = 0; entry[index].name; index++ )
  {
    if ( !Q_stricmp( token, entry[index].name ) )
    {
      shaderParseSurfaceParmFlags |= entry[index].surfaceFlags;
      return;
    }
  }
}

/* ---- ParseShader  0x004FA950 ----  [HIGH] */
int __cdecl ParseShader(char **data_p, int allowTextureName, int imageTrack)
{
  char  *token;
  int    numStages;
  int   *stageStore;
  char  *texModStore;
  int    stageActive;
  int    savedFlags;
  int   *bundlePtr;
  char  *cursor;
  int    k;
  float intensity;
  float  a;
  float b;
  double cosB;
  int    stage[418];
  unsigned char stageBundles[2176];

  numStages = 0;
  token = Com_Parse( data_p );
  if ( *token != '{' )
  {
    ri.Printf( 2, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shaderParseName );
    return 0;
  }

  stageStore = shaderParseStageStorage;
  texModStore = (char *)&shaderParseTexMods;

  for ( ;; )
  {
    token = Com_Parse( data_p );
    if ( !*token )
    {
      ri.Printf( 2, "WARNING: no concluding '}' in shader %s\n", shaderParseName );
      return 0;
    }

    if ( *token == '}' )
      break;

    if ( *token == '{' )
    {
      savedFlags = shaderParseFlags;
      memset( stage, 0, sizeof(stage) );
      bundlePtr = &stage[48];
      cursor = (char *)stageBundles;
      for ( k = 8; k > 0; --k )
      {
        *bundlePtr = (int)cursor;
        cursor += 272;
        bundlePtr += 50;
      }
      if ( !ParseStage( stage, data_p, allowTextureName, imageTrack, &stageActive ) )
        return 0;
      if ( stageActive )
      {
        cursor = texModStore;
        memcpy( texModStore, stageBundles, 0x880u );
        memcpy( stageStore, stage, 0x688u );
        bundlePtr = stageStore + 48;
        for ( k = 8; k > 0; --k )
        {
          *bundlePtr = (int)cursor;
          cursor += 272;
          bundlePtr += 50;
        }
        *stageStore |= 1u;
        ++numStages;
        texModStore = cursor;
        stageStore += 418;
        continue;
      }
      shaderParseFlags = savedFlags;
    }
    else if ( !Q_stricmpn( token, "qer", 3 ) )
    {
      Com_SkipRestOfLine( data_p );
    }
    else if ( !Q_stricmp( token, "radialNormals" ) )
    {
      Com_SkipRestOfLine( data_p );
    }
    else if ( !Q_stricmp( token, "q3map_sun" ) )
    {
      token = Com_ParseOnLine( data_p );
      (*(float *)((char *)tr_sunLight + 0)) = atof( token );
      token = Com_ParseOnLine( data_p );
      (*(float *)((char *)tr_sunLight + 4)) = atof( token );
      token = Com_ParseOnLine( data_p );
      (*(float *)((char *)tr_sunLight + 8)) = atof( token );
      VectorNormalize( &tr_sunLight );
      token = Com_ParseOnLine( data_p );
      intensity = atof( token );
      (*(float *)((char *)tr_sunLight + 0)) = (*(float *)((char *)tr_sunLight + 0)) * intensity;
      (*(float *)((char *)tr_sunLight + 4)) = (*(float *)((char *)tr_sunLight + 4)) * intensity;
      (*(float *)((char *)tr_sunLight + 8)) = (*(float *)((char *)tr_sunLight + 8)) * intensity;
      token = Com_ParseOnLine( data_p );
      a = (float)atof( token ) * 0.017453292f;   
      token = Com_ParseOnLine( data_p );
      b = (float)atof( token ) * 0.017453292f;   
      cosB = cos( b );
      *(float *)&tr_sunDirection = cos( a ) * cosB;
      *(float *)&(*(int *)((char *)tr_sunDirection + 4)) = sin( a ) * cosB;
      *(float *)&(*(int *)((char *)tr_sunDirection + 8)) = sin( b );
    }
    else if ( !Q_stricmp( token, "deformVertexes" ) )
    {
      { ShaderTextCursorArgument cursorArgument; cursorArgument.cursor = data_p; ParseDeform( cursorArgument ); }
    }
    else if ( !Q_stricmp( token, "tesssize" ) )
    {
      Com_SkipRestOfLine( data_p );
    }
    else if ( !Q_stricmp( token, "clampTime" ) )
    {
      token = Com_ParseOnLine( data_p );
      if ( *token )
        shaderParseClampTime = atof( token );
    }
    else if ( !Q_stricmpn( token, "q3map", 5 ) )
    {
      Com_SkipRestOfLine( data_p );
    }
    else if ( !Q_stricmp( token, "surfaceParm" ) )
    {
      ParseSurfaceParm( data_p );
    }
    else if ( !Q_stricmp( token, "nomipmaps" ) )
    {
      shaderParseFlags |= 0x80u;
    }
    else if ( !Q_stricmp( token, "nopicmip" ) )
    {
      shaderParseFlags |= 0x100u;
    }
    else if ( !Q_stricmp( token, "picmip2" ) )
    {
      shaderParseFlags |= 0x200u;
    }
    else if ( !Q_stricmp( token, "polygonOffset" ) )
    {
      shaderParseFlags = shaderParseFlags & 0xFFFFFF8F | 0x10;
    }
    else if ( !Q_stricmp( token, "polygonOffset2" ) )
    {
      shaderParseFlags = shaderParseFlags & 0xFFFFFF8F | 0x20;
    }
    else if ( !Q_stricmp( token, "polygonOffsetConst" ) )
    {
      shaderParseFlags = shaderParseFlags & 0xFFFFFF8F | 0x40;
    }
    else if ( !Q_stricmp( token, "entityMergable" ) )
    {
      shaderParseFlags |= 4u;
    }
    else if ( !Q_stricmp( token, "fogParms" ) )
    {
      if ( !ParseVector( data_p, 3, (float *)&shaderParseFogColor ) )
        return 0;
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shaderParseName );
        continue;
      }
      shaderParseFogDepthForOpaque = atof( token );
      Com_SkipRestOfLine( data_p );
    }
    else if ( !Q_stricmp( token, "portal" ) )
    {
      shaderParseSort = 1065353216;
      shaderParseSurfaceFlags |= 2u;
    }
    else if ( !Q_stricmp( token, "skyparms" ) )
    {
      ParseSkyParms( data_p, imageTrack );
    }
    else if ( !Q_stricmp( token, "sunfile" ) )
    {
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing sun name for 'sunfile'\n" );
        continue;
      }
      (*(unsigned char *)(((char *)(void *)&tr_registered) + 0x011F)) = 0;
      strncpy( &(*(unsigned char *)(((char *)(void *)&tr_registered) + 0x00E0)), token, 0x40u );
      if ( (*(unsigned char *)(((char *)(void *)&tr_registered) + 0x011F)) )
      {
        ri.Printf( 2, "WARNING: name '%s' too long for sunfile\n", token );
        (*(unsigned char *)(((char *)(void *)&tr_registered) + 0x00E0)) = 0;
      }
    }
    else if ( !Q_stricmp( token, "nofog" ) )
    {
      shaderParseFlags |= 0x400u;
    }
    else if ( !Q_stricmp( token, "light" ) )
    {
      Com_ParseOnLine( data_p );
    }
    else if ( !Q_stricmp( token, "cull" ) )
    {
      token = Com_ParseOnLine( data_p );
      if ( !*token )
      {
        ri.Printf( 2, "WARNING: missing cull parms in shader '%s'\n", shaderParseName );
        continue;
      }
      if ( Q_stricmp( token, "none" )
        && Q_stricmp( token, "twosided" )
        && Q_stricmp( token, "disable" ) )
      {
        if ( Q_stricmp( token, "back" )
          && Q_stricmp( token, "backside" )
          && Q_stricmp( token, "backsided" ) )
        {
          ri.Printf( 2, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shaderParseName );
        }
        else
        {
          shaderParseCullType = 1;
        }
      }
      else
      {
        shaderParseCullType = 2;
      }
    }
    else if ( !Q_stricmp( token, "sort" ) )
    {
      if ( !ParseSort( data_p ) )
        return 0;
    }
    else
    {
      ri.Printf( 2, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shaderParseName );
      return 0;
    }
  }

  if ( numStages || (shaderParseFlags & 8) != 0 )
  {
    shaderParseFlags |= 2u;
    return 1;
  }
  return 0;
}

/* ---- ComputeStageIteratorFunc_tail  0x004FB0ED ----  VERIFIED */
/* retail 0x004FB0ED, IDA nullsub_91: the shared retn of retail's dead out-of-line ComputeStageIteratorFunc copy at 0x004FB0D0 (RTCW tr_shader.c; IDA has no function there, its jz lands on this byte); stands in for that tail only. */
void ComputeStageIteratorFunc_tail()
{
  ;
}

/* ---- ComputeHardwareNeeds  0x004FB0F0 ----  [HIGH] */
void ComputeHardwareNeeds()
{
  int stageIndex;
  int surfaceFlags;
  int lightingFlags;
  int lightingModes;
  int *stage;
  int  stageFlags;
  int  rgbGen;
  unsigned int alphaGen;
  unsigned char lightByte;
  int  numBundles;
  int *bundleImg;
  int  bundleIndex;
  int *bundle;
  int  tcGen;
  int *tcMod;
  int  numTcMods;
  int  numDeforms;
  int *deform;

  surfaceFlags = shaderParseSurfaceFlags;
  lightingFlags = 0;
  shaderParseLightingFlags = 0;
  stage = &shaderParseStage0RgbGen;

  for ( ; stage <= &shaderParseStage0RgbGen + 7 * 418; )
  {
    stageFlags = *(stage - 409);
    if ( (stageFlags & 1) == 0 )
      break;

    rgbGen = *stage;
    stageFlags |= 0x40000;
    *(stage - 409) = stageFlags;

    switch ( rgbGen )
    {
      case 1:
      case 11:
      case 12:
        surfaceFlags |= 0x20000u;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
      case 2:
        break;
      case 5:
      case 6:
      case 7:
        surfaceFlags |= 0x20000u;
        stageFlags |= 0x10000;
        *(stage - 409) = stageFlags;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
      case 9:
        surfaceFlags |= 0x102u;
        stageFlags |= 0x20008;
        *(stage - 409) = stageFlags;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
      case 10:
        surfaceFlags |= 0x102u;
        stageFlags |= 0x20010;
        *(stage - 409) = stageFlags;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
      default:
        surfaceFlags |= 0x10020000u;
        stageFlags |= 0x10000;
        *(stage - 409) = stageFlags;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
    }

    alphaGen = stage[6];
    switch ( alphaGen )
    {
      case 0:
      case 1:
        break;
      case 4:
      case 5:
        surfaceFlags |= 0x20000u;
        *(stage - 409) |= 0x10000u;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
      case 9:
        surfaceFlags |= 0x20000u;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
      default:
        surfaceFlags |= 0x10020000u;
        *(stage - 409) |= 0x10000u;
        shaderParseSurfaceFlags = surfaceFlags;
        break;
    }

    if ( (*(stage - 409) & 0x10000) == 0 )
    {
      if ( rgbGen == 1 )
      {
        lightByte = tr_identityLightByte;
        *((unsigned char *)stage + 28) = lightByte;
        *((unsigned char *)stage + 29) = lightByte;
        *((unsigned char *)stage + 30) = lightByte;
      }
      else if ( rgbGen == 2 )
      {
        *((unsigned char *)stage + 28) = -1;
        *((unsigned char *)stage + 29) = -1;
        *((unsigned char *)stage + 30) = -1;
      }
      if ( alphaGen == 0 || alphaGen == 1 )
        *((unsigned char *)stage + 31) = -1;
    }

    numBundles = 0;
    bundleImg = stage - 372;
    do
    {
      if ( !*bundleImg )
        break;
      bundleImg += 50;
      surfaceFlags |= 512 << numBundles;
      *(stage - 409) |= 256 << numBundles;
      ++numBundles;
    }
    while ( numBundles < 8 );

    bundleIndex = 0;
    shaderParseSurfaceFlags = surfaceFlags;
    if ( numBundles > 0 )
    {
      bundle = stage - 376;
      do
      {
        if ( *bundle > 1 )
        {
          surfaceFlags |= 0x80001u;
          shaderParseSurfaceFlags = surfaceFlags;
        }
        else
        {
          *bundle = 0;
        }
        tcGen = bundle[7];
        if ( tcGen <= 0 || tcGen > 3 )
        {
          surfaceFlags |= 0x100000 << bundleIndex;
          shaderParseSurfaceFlags = surfaceFlags;
        }
        if ( bundle[14] > 0 )
        {
          tcMod = (int *)bundle[15];
          numTcMods = bundle[14];
          do
          {
            switch ( *tcMod )
            {
              case 2:
              case 3:
              case 5:
              case 6:
              case 7:
                surfaceFlags |= 0x100000 << bundleIndex;
                shaderParseSurfaceFlags = surfaceFlags;
                break;
              default:
                break;
            }
            tcMod += 17;
            --numTcMods;
          }
          while ( numTcMods );
        }
        ++bundleIndex;
        bundle += 50;
      }
      while ( bundleIndex < numBundles );
      lightingFlags = shaderParseLightingFlags;
    }

    lightingFlags |= *(stage - 409);
    stage += 418;
    shaderParseLightingFlags = lightingFlags;

  }

  lightingModes = lightingFlags & 0x18;
  if ( lightingModes && (lightingModes & (lightingModes - 1)) )
  {
    ri.Printf(
      0,
      "WARNING: shader '%s' uses more than one of rgbGen lightingAmbient, lightingDiffuse, and lightingSpecular\n",
      shaderParseName);
    surfaceFlags = shaderParseSurfaceFlags;
  }

  numDeforms = shaderParseNumDeforms;
  if ( numDeforms > 0 )
  {
    deform = (int *)&shaderParseDeforms;
    do
    {
      if ( *deform == 4 )
        surfaceFlags |= 0x40000u;
      else
        surfaceFlags |= 0x20000000u;
      deform += 13;
      --numDeforms;
      shaderParseSurfaceFlags = surfaceFlags;
    }
    while ( numDeforms );
  }
}

/* ---- CreateDlightStage  0x004FB3C0 ----  [HIGH] */
void CreateDlightStage()
{
  shaderStage_t *sourceStage = (shaderStage_t *)(tr_shaderParseScratch + 0x45A8);
  shaderStage_t *dlightStage = (shaderStage_t *)(tr_shaderParseScratch + 0x4C30);
  unsigned int originalStateBits;
  int dlightImage;
  if ( shaderParseNumUnfoggedPasses != 1 )
    return;
  if ( !shaderParseStage0Bundle0Image[0] )
    return;
  if ( !shaderParseStage0Bundle1Image )
    return;
  if ( shaderParseStage0Bundle2Image || shaderParseStage0Bundle0IsLightmap )
    return;
  if ( !shaderParseStage0Bundle1IsLightmap )
    return;
  if ( shaderParseStage0Bundle0TextureEnvMode[0] != 8448 || shaderParseStage0Bundle1TextureEnvMode != 8448 )
    return;

  originalStateBits = sourceStage->stateBits;
  originalStateBits &= 0xFFu;
  switch ( originalStateBits )
  {
    case 0:
    case 18:
    case 34:
    case 37:
    case 101:
      dlightImage = tr_dlightImage;
      memcpy( dlightStage, sourceStage, sizeof(*dlightStage) );
      *(unsigned char *)&dlightStage->stateBits = 0;
      shaderParseStage1Bundle1Image = dlightImage;
      if ( (originalStateBits & 0xFu) == 5 )
        dlightStage->stateBits |= 0x25;
      else
        dlightStage->stateBits |= 0x22;
      dlightStage->flags |= 0x80u;
      *(unsigned int *)(tr_shaderParseScratch + 0x4454) |= dlightStage->flags;   /* shader.lightingFlags */
      shaderParseNumUnfoggedPasses = 2;
      break;
    default:
      break;
  }
}

/* ---- CollapseMultitexture  0x004FB530 ----  VERIFIED */
typedef struct {
 void *image[32];
 int numImageAnimations;
 float imageAnimationSpeed;
 waterMap_t *waterTexture;
 int textureType;
 int texEnvMode;
 ShaderTexEnvCombine *combine;
 ShaderNVTextureOperation *nvTextureShader;
 int tcGen;
 float tcGenVectors[2][3];
 int numTexMods;
 texModInfo_t *texMods;
 int videoMapHandle;
 byte isLightmap;
 byte isVideoMap;
 byte isOneShot;
} ShaderTextureBundleCopy;
int CollapseMultitexture()
{
  unsigned int abits, bbits;
  unsigned int i;
  int stageIndex;
  int numBundles[2];
  ShaderTextureBundleCopy tmpBundle;

  if ( !qgls_ActiveTextureARB )
    return 0;
  if ( (shaderParseStageStorage[0] & 1) == 0 )
    return 0;
  if ( (shaderParseStage1Flags & 1) == 0 )
    return 0;
  abits = shaderParseStage0StateBits;
  bbits = shaderParseStage1StateBits;
  if ( ((abits ^ bbits) & 0xFFFFFE00u) != 0 )
    return 0;

  /* count each stage's bundles (Q3: for ( i = 0; stages[0].bundle[i].image[0]; i++ ) ;) */
  for ( stageIndex = 0; stageIndex < 2; stageIndex++ )
  {
    for ( numBundles[stageIndex] = 0;
          *(int *)((char *)(tr_shaderParseScratch + 0x463C) + 1672 * stageIndex + 200 * numBundles[stageIndex]);
          numBundles[stageIndex]++ )
      ;
  }

  if ( numBundles[1] != 1 )
    return 0;
  if ( numBundles[0] + 1 > glConfig.maxActiveTextures )
    return 0;

  abits &= 0xFFu;
  bbits &= 0xFFu;
  i = 0;
  do
  {
    if ( abits == tr_collapseRules[i].srcStateBits
      && bbits == tr_collapseRules[i].dstStateBits )
      break;
    ++i;
  }
  while ( i < 9 );
  if ( i == 9 )
    return 0;

  /* GL_ADD is a separate extension, and an add collapse can only have identity
     colors: two separate tests of the env, as in Q3 (retail re-tests the env with
     `cmp edx,edx` on the shared block). */
  if ( tr_collapseRules[i].texEnv == 260 && !glConfig.textureEnvAddAvailable )
    return 0;
  if ( tr_collapseRules[i].texEnv == 260 && shaderParseStage0RgbGen != 2 )
    return 0;
  /* different color/alpha gens collapse only into a modulate with an identity
     second stage */
  if ( shaderParseStage0RgbGen != shaderParseStage1RgbGen
    || shaderParseStage0AlphaGen != shaderParseStage1AlphaGen )
  {
    if ( tr_collapseRules[i].texEnv != 8448
      || shaderParseStage1RgbGen != 2
      || (shaderParseStage1AlphaGen != 0 && shaderParseStage1AlphaGen != 1) )
      return 0;
  }
  if ( shaderParseStage0RgbGen == 8 && memcmp(&shaderParseStage0RgbWave, &shaderParseStage1RgbWave, 0x14u) )
    return 0;
  if ( shaderParseStage0AlphaGen == 7 && memcmp(&shaderParseStage0AlphaWave, &shaderParseStage1AlphaWave, 0x14u) )
    return 0;

  if ( numBundles[0] == 1 && shaderParseStage0Bundle0IsLightmap )
  {
    memcpy(&tmpBundle, shaderParseStage0Bundle0Image, sizeof(tmpBundle));
    memcpy(shaderParseStage0Bundle0Image, &shaderParseStage1Bundle0Image, 0xC8u);
    memcpy(&shaderParseStage0Bundle1Image, &tmpBundle, 0xC8u);
  }
  else
  {
    memcpy(&shaderParseStage0Bundle0Image[50 * numBundles[0]], &shaderParseStage1Bundle0Image, 0xC8u);
  }
  shaderParseStage0Bundle0TextureEnvMode[50 * numBundles[0]] = tr_collapseRules[i].texEnv;
  *(unsigned char *)&shaderParseStage0StateBits = 0;
  shaderParseStage0StateBits |= tr_collapseRules[i].collapsedStateBits;
  memmove(&shaderParseStage1Flags, &shaderParseStage2, 0x2730u);
  memset(&shaderParseStage7, 0, 0x688u);
  return 1;
}

/* ---- SortNewShader  0x004FB790 ----  [HIGH] */
void SortNewShader()
{
  int newShader;
  float newSort;
  int newSub;
  unsigned int newSubASort;
  unsigned int newSubBSort;
  int subA, subB;
  int i;
  int cur;
  int curSub;

  newShader = tr_numShaders[tr_numShaders[0]];
  newSort = *(float *)(newShader + 88);
  newSub = *(int *)(newShader + 340);
  newSubASort = 0;
  newSubBSort = 0;
  if ( newSub )
  {
    subA = *(int *)(newSub + 4);
    if ( subA )
      newSubASort = *(unsigned int *)(subA + 88);
    subB = *(int *)(newSub + 204);
    if ( subB )
      newSubBSort = *(unsigned int *)(subB + 88);
  }
  for ( i = tr_numShaders[0] - 2; i >= 0; --i )
  {
    cur = tr_sortedShaderSlots[i];
    if ( *(float *)(cur + 88) < newSort )
      break;
    if ( *(float *)(cur + 88) == newSort )
    {
      curSub = *(int *)(cur + 340);
      if ( curSub )
      {
        int useSecond = 1;
        int sub = *(int *)(curSub + 4);
        if ( sub )
        {
          unsigned int curSubASort = *(unsigned int *)(sub + 88);
          if ( curSubASort < newSubASort )
            break;
          if ( curSubASort != newSubASort )
            useSecond = 0;
        }
        if ( useSecond )
        {
          sub = *(int *)(curSub + 204);
          if ( sub && *(unsigned int *)(sub + 88) <= newSubBSort )
            break;
        }
      }
    }
    tr_sortedShaderSlots_1[i] = cur;
    ++*(int *)(cur + 72);
  }
  *(int *)(newShader + 72) = i + 1;
  tr_sortedShaderSlots_1[i] = newShader;
}

/* ---- GeneratePermanentShader  0x004FB850 ----  VERIFIED */
char *GeneratePermanentShader()
{
  shaderDef_t *shader;
  int  stageIndex;
  int  bundleIndex;
  int tcModBytes;
  int  hashValue;

  if ( ((int *)(void *)&tr_shaderRegistry)[0] == 4096 )
  {
    ri.Printf(2, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
    UpdateDelayLoadImagesForShader(shaderParseName, 1);
    return tr_defaultShader;
  }

  shader = (shaderDef_t *)ri.Hunk_Alloc(sizeof(*shader));
  memcpy(shader, shaderParseName, 0x198u);
  (tr_shaderRegistry.shaders)[((int *)(void *)&tr_shaderRegistry)[0]] = (int)shader;
  shader->index = ((int *)(void *)&tr_shaderRegistry)[0];
  (tr_shaderRegistry.sortedShaders)[((int *)(void *)&tr_shaderRegistry)[0]] = (int)shader;
  shader->sortedIndex = ((int *)(void *)&tr_shaderRegistry)[0]++;

  /* RTCW/Mac 1.3 shape: index the parse-scratch stages (+0x45A8) and the bundles directly
     (retail keeps the source texMods pointer as its own induction variable) */
  for ( stageIndex = 0; stageIndex < shader->numUnfoggedPasses; stageIndex++ )
  {
    if ( ( *(unsigned char *)&((shaderStage_t *)(tr_shaderParseScratch + 0x45A8))[stageIndex] & 1 ) == 0 )
    {
      shader->stages[stageIndex] = 0;
      break;
    }
    shader->stages[stageIndex] = (shaderStage_t *)ri.Hunk_Alloc(sizeof(shaderStage_t));
    memcpy(shader->stages[stageIndex], &((shaderStage_t *)(tr_shaderParseScratch + 0x45A8))[stageIndex], 0x688u);
    for ( bundleIndex = 0; bundleIndex < 8; bundleIndex++ )
    {
      if ( !((textureBundle_t *)shader->stages[stageIndex])[bundleIndex].numTexMods )
        ((textureBundle_t *)shader->stages[stageIndex])[bundleIndex].texMods = 0;
      else
      {
        tcModBytes = (int)sizeof(texModInfo_t) * ((textureBundle_t *)shader->stages[stageIndex])[bundleIndex].numTexMods;
        ((textureBundle_t *)shader->stages[stageIndex])[bundleIndex].texMods = ri.Hunk_Alloc(tcModBytes);
        memcpy(((textureBundle_t *)shader->stages[stageIndex])[bundleIndex].texMods,
               ((textureBundle_t *)&((shaderStage_t *)(tr_shaderParseScratch + 0x45A8))[stageIndex])[bundleIndex].texMods, tcModBytes);
      }
    }
  }
  UpdateDelayLoadImagesForShader((int *)shader, !MergableShader((int *)shader));
  SortNewShader();
  hashValue = tr_shader_generateHashValue((char *)shader);
  shader->next = (shaderDef_t *)shaderHashTable[hashValue];
  shaderHashTable[hashValue] = (int)shader;
  return (char *)shader;
}

/* ---- FinishShader  0x004FBA30 ----  [HIGH] */
char *FinishShader()
{
  shaderDef_t *shaderDefinition = (shaderDef_t *)(tr_shaderParseScratch + 0x4400);
  int   stageNum;
  int   hasLightmap;
  int  *bundle;
  int  *src;
  int  *stage;
  char *bundleBase;
  int  *srcSave;
  int   bundleIndex;
  int  *bundle0;
  int  *animImg;
  int   animFrame;
  int   image;
  int   flagsByte1;
  int   stateBits;
  int   tailIndex;
  unsigned char *tailStage;

  stageNum = 0;
  hasLightmap = 0;
  if ( (shaderParseFlags & 8) != 0 )
    shaderDefinition->sort = 2.0f;
  if ( (shaderParseFlags & 0x70) != 0 )
  {
    if ( shaderDefinition->sort == 0.0f )
      shaderDefinition->sort = 6.0f;
  }
  bundle = &shaderParseStage0Bundle0TcGen;
  src = &shaderParseStage1Flags;
  stage = shaderParseStageStorage;
  bundleBase = (char *)&shaderParseStage0Bundle0TcGen;
  srcSave = &shaderParseStage1Flags;
  while ( stageNum < 8 && (*stage & 1) != 0 )
  {

    if ( (*stage & 2) != 0 && !r_detailtextures->integer )
    {
if ( stageNum >= 7 )
    {
      memset(stage, 0, 0x688u);
    }
    else
    {
      memmove(stage, src, 1672 * (7 - stageNum));
      tailIndex = 7;
      tailStage = &shaderParseStage7;
      while ( (*tailStage & 1) == 0 )
      {
        --tailIndex;
        tailStage -= 1672;
        if ( tailIndex <= stageNum )
          goto afterRemove;
      }
      memset(&shaderParseStageStorage[418 * tailIndex], 0, 0x688u);   
afterRemove:
      --stageNum;
      stage -= 418;
      src -= 418;
      bundleBase -= 1672;
    }
    }
    else
    {
      bundleIndex = 0;
      while ( 1 )
      {
        bundle0 = bundle - 39;
        if ( (int)*(bundle - 7) > 0 )
        {
          int uniform = 1;
          animImg = bundle - 39;
          animFrame = 0;
          while ( 1 )
          {
            if ( !*animImg )
            {
              ri.Printf(2, "Shader %s has a missing image\n", shaderParseName);
              *stage &= ~1u;
              goto stageDone;
            }
            if ( *(int *)(*animImg + 84) != *(int *)(*bundle0 + 84) )
            {
              uniform = 0;
              break;
            }
            ++animFrame;
            ++animImg;
            if ( animFrame >= bundle0[32] )
              break;
          }
          if ( !uniform )
          {
            ri.Printf(2, "Shader %s has non-uniform image types in an animMap\n", shaderParseName);
            *stage &= ~1u;
            goto stageDone;
          }
        }

        if ( *((unsigned char *)bundle0 + 196) )
        {
          if ( !*bundle )
            *bundle = 2;
          hasLightmap = 1;
        }
        else
        {
          image = *bundle0;
          if ( image && *(int *)(image + 84) == 34067 )
          {
            if ( !*bundle )
              *bundle = 15;
          }
          else if ( !*bundle )
          {
            *bundle = 3;
          }
        }
        bundle += 50;
        if ( ++bundleIndex >= 8 )
          break;
      }

      flagsByte1 = (shaderParseFlags >> 8) & 0xFF;
      if ( (shaderParseFlags & 0x20) != 0 )
        stage[417] |= 0x4000;
      else if ( (shaderParseFlags & 0x10) != 0 )
        stage[417] |= 0x2000;
      else if ( (shaderParseFlags & 0x40) != 0 )
        stage[417] |= 0x8000;
      if ( (flagsByte1 & 4) == 0 || (*(unsigned char *)stage & 4) != 0 )
        stage[417] |= 0x200000u;
      stateBits = stage[417];
      if ( (stateBits & 0x70000000) == 0 && ((stateBits & 0xFF) == 37 || (stateBits & 0xFF) == 101) )
        stage[417] = stateBits | 0x10000000;
      stateBits = stage[417];
      if ( (unsigned char)stateBits )
      {
        if ( (unsigned char)shaderParseStage0StateBits )
        {
          if ( shaderDefinition->sort == 0.0f )
          {
            if ( (stateBits & 0x100) != 0 )
            {
              shaderDefinition->sort = 7.0f;
            }
            else
            {
              shaderDefinition->sort = 19.0f;
              if ( (shaderParseStage0StateBits & 0xF0) != 0x20 )
                shaderDefinition->sort = 15.0f;
            }
          }
        }
      }
stageDone:
      src = srcSave;
    }
    ++stageNum;
    src += 418;
    bundle = (int *)(bundleBase + 1672);
    stage += 418;
    srcSave = src;
    bundleBase += 1672;
  }

  if ( shaderDefinition->sort == 0.0f )
    shaderDefinition->sort = 5.0f;
  for ( ; stageNum > 1; --stageNum )
  {
    if ( !CollapseMultitexture() )
      break;
  }
  if ( shaderParseLightmapIndex >= 0 && !hasLightmap )
  {
    ri.Printf(1, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shaderParseName);
    shaderParseLightmapIndex = -1;
  }
  shaderParseNumUnfoggedPasses = stageNum;
  if ( !stageNum )
    shaderDefinition->sort = 9.0f;
  if ( (char)shaderParseLightingFlags >= 0 )
    CreateDlightStage();
  ComputeHardwareNeeds();
  shaderParseOptimalStageIteratorFunc = (int)tr_stageIteratorFunc;
  if ( (shaderParseFlags & 8) != 0 )
    shaderParseOptimalStageIteratorFunc = (int)RB_StageIteratorSky;
  return GeneratePermanentShader();
}

/* ---- FindShaderInShaderText  0x004FBDE0 ----  VERIFIED */
char *FindShaderInShaderText( const char *shadername )
{
 int h;
 ShaderTextHashNode *node;
 char *p;
 char *token;
 if ( !s_shaderText )
   return 0;
 h = (unsigned short)tr_shader_generateHashValue( (char *)shadername );
 node = &shaderTextHashTable[h];
 while ( node )
 {
   if ( !node->text )
     return 0;
   p = node->text;
		if ( parseInfo->ungetReady ) {
			parseInfo->ungetReady = qfalse;
			p = parseInfo->ungetTokenSave;
			parseInfo->currentLine = parseInfo->ungetLineSave;
		}
		token = Com_ParseExt( &p, qtrue );

		if ( token[0] && shadername
		     && !Q_stricmpn( token, shadername, 99999 ) ) {
			return p;
		}

   node = node->next;
 }
 return 0;
}

/* ---- R_FindShaderByName  0x004FBE80 ----  VERIFIED */
char *__fastcall R_FindShaderByName(const char *name)
{
  char strippedName[64];
  int  sh;

  if ( !name || !*name )
    return tr_defaultShader;

  Com_StripExtension( name, strippedName );

  for ( sh = shaderHashTable[ tr_shader_generateHashValue(strippedName) ]; sh; sh = *(int *)(sh + 404) )
  {
    if ( !Q_stricmp((const char *)sh, strippedName) )
      return (char *)sh;
  }
  return tr_defaultShader;
}

/* ---- R_LoadShaderType  0x004FBF20 ----  [HIGH] */
char *__cdecl R_LoadShaderType(const char *name, int lightmapIndex, int context)
{
  char *fileData;
  char *parsePtr;
  char fileName[64];
  strcpy(fileName, "shadertypes/");
  if ( strlen(name + 1) + 17 >= 0x40 )
  {
    ri.Printf(2, "WARNING: shader type '%s' is too long\n", name);
    return 0;
  }
  strcat(fileName, name + 1);
  strcat(fileName, ".stype");
  if ( ri.FS_ReadFile(fileName, &fileData) < 0 )
  {
    ri.Printf(2, "WARNING: could not read shader type file '%s'\n", fileName);
    return 0;
  }
  parsePtr = fileData;
  if ( !ParseShader(&parsePtr, 1, context) )
  {
    ri.FS_FreeFile(fileData);
    ri.Error(1, "\x15" "ERROR: invalid shader type in file '%s'\n", fileName);
    return 0;
  }
  ri.FS_FreeFile(fileData);
  strcpy((char *)shaderParseName, name);
  return FinishShader();
}

/* ---- ShaderFromShaderType  0x004FC0C0 ----  [HIGH] */
char *__cdecl ShaderFromShaderType(char *name, int lightmapIndex, int context)
{
  const char *namePtr;
  char       *atSign;
  char        c;
  char       *lastSlash;
  char       *scan;
  const char *segStart;
  size_t      segLen;
  unsigned int prefixLen;
  char       *foundShader;
  int         lmIndex;
  int        *dstBundle;
  int         bundleIndex;
  int         imgIndex;
  int        *imgPtr;
  int         image;
  int         instance;
  int        moreToGo;
  const char *usage;
  int         elemBase;
  const void **srcStage;
  int         passByteOff;
  int        *passBase;
  int        *bundleSave;
  int         passIndex;
  char        typeName[64]; 

  namePtr = name;
  atSign = strrchr(name, '@');
  c = *name;
  lastSlash = name;
  if ( *name )
  {
    scan = name;
    do
    {
      if ( c == '/' || c == '\\' )
        lastSlash = scan;
      c = *++scan;
    }
    while ( c );
  }
  if ( lastSlash > atSign )
  {
    ri.Printf(2, "WARNING: found '/' or '\\' after '@' in shader '%s'\n", name);
    return 0;
  }
  segStart = lastSlash + 1;
  segLen = atSign - segStart;
  typeName[0] = '@';
  if ( lightmapIndex == -1 )
  {
    memcpy(&typeName[1], "model/", 7);   
  }
  else if ( lightmapIndex == -4 )
  {
    memcpy(&typeName[1], "2d/", 4);
  }
  else if ( lightmapIndex != -2 && lightmapIndex < 0 )
  {
    ri.Error(1, "\x15" "Bad lightmap index %i loading shader %s\n", lightmapIndex, name);
  }
  else
  {
    memcpy(&typeName[1], "world/", 7);   
  }
  prefixLen = strlen(typeName);
  if ( prefixLen + segLen >= 0x40 )
  {
    ri.Printf(2, "WARNING: shader type in '%s' is too long\n", name);
    return 0;
  }
  {
    strncpy(typeName + prefixLen, segStart, segLen);
    typeName[prefixLen + segLen] = 0;
    foundShader = R_FindShaderByName(typeName);
    if ( foundShader == tr_defaultShader && (foundShader = R_LoadShaderType(typeName, lightmapIndex, context)) == 0 )
    {
      return 0;
    }
    else
    {
      memcpy(shaderParseName, foundShader, 0x198u);
      passIndex = 0;
      if ( shaderParseNumUnfoggedPasses > 0 )
      {
        lmIndex = shaderParseLightmapIndex;
        dstBundle = &shaderParseStage0Bundle0NumImageAnimations;
        passByteOff = 0;
        passBase = &shaderParseStage0Bundle0NumImageAnimations;
        srcStage = (const void **)(foundShader + 340);
        while ( 1 )
        {
          memcpy(dstBundle - 33, *srcStage, 0x688u);
          bundleIndex = 0;
          elemBase = passByteOff;
          while ( 1 )
          {
            imgIndex = *dstBundle - 1;
            if ( imgIndex < 0 )
              imgIndex = 0;
            imgPtr = &shaderParseStage0Bundle0Image[elemBase + imgIndex];
            do
            {
              image = *imgPtr;
              if ( *imgPtr && *(int *)(image + 80) == 10 )
              {
                instance = R_FindImageInstance((*(int *)(((char *)(void *)&tr_registered) + 0x0128)) == 0, name, context, (char *)image, 0);
                *imgPtr = instance;
                if ( !instance )
                {
                  ri.Printf(2, "WARNING: shadertype '%s' couldn't load image for '%s'\n", typeName, name);
                  return 0;
                }
                lmIndex = shaderParseLightmapIndex;
              }
              else if ( lmIndex >= 0 && image == tr_lightmaps[lmIndex] )
              {
                if ( lightmapIndex < 0 )
                {
                  if ( lightmapIndex == -1 )
                    usage = "used as a model skin";
                  else if ( lightmapIndex == -2 )
                    usage = "not used as a world shader";
                  else if ( lightmapIndex == -3 )
                    usage = "used as a vertex-lit surface";
                  else
                  {
                    usage = "used as a 2D image";
                    if ( lightmapIndex != -4 )
                      usage = "used some other way";
                  }
                  ri.Printf(
                    2,
                    "WARNING: shader type '%s' is a world shader type, but '%s' is %s\n",
                    typeName,
                    name,
                    usage);
                  return 0;
                }
                *imgPtr = tr_lightmaps[lightmapIndex];
              }
              --imgIndex;
              --imgPtr;
            }
            while ( imgIndex > 0 );
            ++bundleIndex;
            dstBundle += 50;
            elemBase += 50;
            if ( bundleIndex < 8 )
              continue;
            break;
          }
          ++passIndex;
          ++srcStage;
          passByteOff += 418;
          passBase += 418;
          dstBundle = passBase;
          if ( passIndex >= shaderParseNumUnfoggedPasses )
            break;
        }
        namePtr = name;
      }
      strcpy((char *)shaderParseName, namePtr);
      shaderParseLightmapIndex = lightmapIndex;
      return GeneratePermanentShader();
    }
  }
}

/* ---- R_BuildShaderFromImage  0x004FC440 ----  [HIGH] */
void __cdecl R_BuildShaderFromImage(int image, int lightmapIndex)
{
  if ( lightmapIndex == -1 )
  {
    /* dynamic colors at vertexes */
    shaderParseFlags = 0;
    shaderParseSurfaceFlags = 16;
    shaderParseStage0Bundle0Image[0] = image;
    shaderParseStage0Bundle0TexCoordComponentCount = 2;
    shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
    shaderParseStageStorage[0] = 1;
    shaderParseStage0RgbGen = 10;
    shaderParseStage0AlphaGen = 1;
    shaderParseStage0StateBits = 1048832;
  }
  else if ( lightmapIndex == -3 )
  {
    /* explicit colors at vertexes */
    shaderParseFlags = 0;
    shaderParseSurfaceFlags = 131152;
    shaderParseStage0Bundle0Image[0] = image;
    shaderParseStage0Bundle0TexCoordComponentCount = 2;
    shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
    shaderParseStageStorage[0] = 1;
    shaderParseStage0RgbGen = 5;
    shaderParseStage0AlphaGen = 1;
    shaderParseStage0StateBits = 256;
  }
  else if ( lightmapIndex == -4 )
  {
    /* GUI elements */
    shaderParseFlags = 0;
    shaderParseSurfaceFlags = 131152;
    shaderParseStage0Bundle0Image[0] = image;
    shaderParseStage0Bundle0TexCoordComponentCount = 2;
    shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
    shaderParseStageStorage[0] = 1;
    shaderParseStage0RgbGen = 6;
    shaderParseStage0AlphaGen = 4;
    shaderParseStage0StateBits = 65637;
    shaderParseCullType = 2;
  }
  else if ( lightmapIndex == -2 )
  {
    /* fullbright level */
    shaderParseFlags = 0;
    shaderParseSurfaceFlags = 48;
    shaderParseStage0Bundle0Image[0] = tr_identityLightImage;
    shaderParseStage0Bundle0TexCoordComponentCount = 2;
    shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
    shaderParseStageStorage[0] = 1;
    shaderParseStage0RgbGen = 1;
    shaderParseStage0StateBits = 256;
    shaderParseStage1Bundle0Image = image;
    shaderParseStage1Bundle0TexCoordComponentCount = 2;
    shaderParseStage1Bundle0TextureEnvMode = 8448;
    shaderParseStage1Flags = 1;
    shaderParseStage1RgbGen = 2;
    shaderParseStage0AlphaGen = 1;
    shaderParseStage1StateBits |= 0x13u;
  }
  else
  {
    /* two pass lightmap */
    shaderParseFlags = 0;
    shaderParseSurfaceFlags = 48;
    shaderParseStage0Bundle0Image[0] = tr_lightmaps[lightmapIndex];
    shaderParseStage0Bundle0TexCoordComponentCount = 2;
    shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
    shaderParseStage0Bundle0IsLightmap = 1;
    shaderParseStageStorage[0] = 1;
    shaderParseStage0RgbGen = 2;
    shaderParseStage0StateBits = 256;
    shaderParseStage1Bundle0Image = image;
    shaderParseStage1Bundle0TexCoordComponentCount = 2;
    shaderParseStage1Bundle0TextureEnvMode = 8448;
    shaderParseStage1Flags = 1;
    shaderParseStage1RgbGen = 2;
    shaderParseStage0AlphaGen = 1;
    shaderParseStage1StateBits |= 0x13u;
  }
}

/* ---- R_FindShader  0x004FC5C0 ----  [HIGH] */
char *__cdecl R_FindShader(char *name, int lightmapIndex, int mipmap, const char *context)
{
  const char *in;
  char nameCharacter;
  char       *dst;
  shaderDef_t *sh;
  int         stage;
  int         bundle;
  char       *typeShader;
  int         image;
  int         imageFlags;
  char       *shaderText;  
  char        strippedName[64];  
  char        fileName[64]; 

  nameCharacter = *name;
  if ( !nameCharacter )
    return tr_defaultShader;
  if ( lightmapIndex >= 0 && lightmapIndex >= tr_lightmapCount )
    lightmapIndex = -2;
  in = name;
  dst = strippedName;
  while ( nameCharacter && nameCharacter != '.' )
  {
    *dst++ = nameCharacter;
    nameCharacter = *++in;
  }
  *dst = 0;

  for ( sh = (shaderDef_t *)shaderHashTable[tr_shader_generateHashValue(strippedName)]; sh; sh = sh->next )
  {
    if ( (sh->lightmapIndex == lightmapIndex || (sh->lightmapIndex == -1 && lightmapIndex >= 0 && context == (const char *)9))
         && !Q_stricmp((const char *)sh, strippedName) )
    {
      UpdateDelayLoadImagesForShader((int *)sh, 0);
      return (char *)sh;
    }
  }

  
  R_SyncRenderThread();
  memset(shaderParseName, 0, 0x198u);
  memset(shaderParseStageStorage, 0, 0x3440u);
  strncpy((char *)shaderParseName, strippedName, 0x3Fu);
  shaderParseName_63 = 0;
  shaderParseLightmapIndex = lightmapIndex;
  if ( lightmapIndex == -1 )
    shaderParseFlags = 512;
  for ( stage = 0; stage < 8; stage++ )
    for ( bundle = 0; bundle < 8; bundle++ )
      *(char **)( (char *)&shaderParseStage0Bundle0TexMods + stage * 1672 + bundle * 200 ) = (char *)&shaderParseTexMods + ( stage * 8 + bundle ) * 272;
  if ( strrchr(strippedName, '@') )
  {
    typeShader = ShaderFromShaderType(strippedName, lightmapIndex, (int)context);
    if ( typeShader )
      return typeShader;
    return tr_defaultShader;
  }
  shaderText = FindShaderInShaderText(strippedName);
  if ( shaderText )
  {
    if ( r_printShaders->integer )
      ri.Printf(0, "*SHADER* %s\n", name);
    if ( !ParseShader(&shaderText, 0, (int)context) )
    {
      shaderParseFlags = 3;
      shaderParseSurfaceFlags = 16;
    }
    return FinishShader();
  }
  if ( !r_graymap->integer || shaderParseLightmapIndex == -4 || shaderParseLightmapIndex == -2 )
  {
    imageFlags = mipmap != 0 ? 3 : 48;
    if ( (*(int *)(((char *)(void *)&tr_registered) + 0x0128)) )
    {
      if ( shaderParseLightmapIndex == -1 )
        imageFlags &= ~0x80u;
    }
    strncpy(fileName, name, 0x3Fu);
    fileName[63] = 0;
    Com_DefaultExtension(fileName, 0x40, ".tga");
    image = R_FindImageFile(fileName, 3553, imageFlags, (int)context, 0, 1.0);
    if ( !image )
    {
      ri.Printf(1, "Couldn't find image for shader %s\n", name);
      shaderParseFlags |= 1u;
      return FinishShader();
    }
  }
  else
  {
    image = tr_grayImage;
  }
  R_BuildShaderFromImage(image, lightmapIndex);
  return FinishShader();
}

/* ---- R_RegisterShaderFromImage  0x004FC8C0 ----  VERIFIED */
int __cdecl R_RegisterShaderFromImage(char *name, int lightmapIndex, int image)
{
  shaderDef_t *sh;
  int      stage;
  int      bundle;

  sh = (shaderDef_t *)shaderHashTable[tr_shader_generateHashValue(name)];
  while ( sh )
  {
    if ( (sh->lightmapIndex == lightmapIndex || (sh->flags & 1) != 0)
         && !Q_stricmp((const char *)sh, name) )
    {
      UpdateDelayLoadImagesForShader((int *)sh, 0);
      return sh->index;
    }
    sh = sh->next;
  }

  
  R_SyncRenderThread();
  Com_Memset(shaderParseName, 0, 0x198u);
  Com_Memset(shaderParseStageStorage, 0, 0x3440u);
  strncpy((char *)shaderParseName, name, 0x3Fu);
  shaderParseName_63 = 0;
  shaderParseLightmapIndex = lightmapIndex;
  for ( stage = 0; stage < 8; stage++ )
    for ( bundle = 0; bundle < 8; bundle++ )
      *(char **)( (char *)&shaderParseStage0Bundle0TexMods + stage * 1672 + bundle * 200 ) = (char *)&shaderParseTexMods + ( stage * 8 + bundle ) * 272;  
  R_BuildShaderFromImage(image, lightmapIndex);
  return *((int *)FinishShader() + 17);
}

/* ---- RE_RegisterShaderLightMap  0x004FCA30 ----  VERIFIED */
int __cdecl RE_RegisterShaderLightMap(char *name, int lightmapIndex, const char *context)
{
  char *shader;

  if ( strlen(name) >= 0x40 )
  {
    Com_Printf("Shader name exceeds MAX_QPATH\n");
    return 0;
  }
  shader = R_FindShader(name, lightmapIndex, 1, context);
  if ( (shader[76] & 1) != 0 )
    return 0;
  return *((int *)shader + 17);
}

/* ---- RE_RegisterShader  0x004FCA80 ----  [HIGH] */
int __cdecl RE_RegisterShader(char *name, const char *context)
{
  char *shader;

  if ( !tr_registered[0] )
    return 0;
  if ( strlen(name) >= 0x40 )
  {
    Com_Printf("Shader name exceeds MAX_QPATH\n");
    return 0;
  }
  shader = R_FindShader(name, -4, 1, context);
  if ( (shader[76] & 1) != 0 )
    return 0;
  return *((int *)shader + 17);
}

/* ---- RE_RegisterShaderNoMip  0x004FCAE0 ----  [HIGH] */
int __cdecl RE_RegisterShaderNoMip(char *name, const char *context)
{
  char *shader;

  if ( !tr_registered[0] )
    return 0;
  if ( strlen(name) >= 0x40 )
  {
    Com_Printf("Shader name exceeds MAX_QPATH\n");
    return 0;
  }
  shader = R_FindShader(name, -4, 0, context);
  if ( (shader[76] & 1) != 0 )
    return 0;
  return *((int *)shader + 17);
}

/* ---- R_CanOptimizeStaticModelStage  0x004FCB40 ----  [HIGH] */
int __cdecl R_CanOptimizeStaticModelStage(int stage, int refStage)
{
  int stageType;
  int stageGen;

  if ( *(int *)(stage + 188) )
    return 0;
  stageType = *(int *)(stage + 1636);
  switch ( stageType )
  {
    case 1:
    case 2:
    case 10:
    case 11:
    case 12:
      stageGen = *(int *)(stage + 1660);
      if ( stageGen < 0
        || stageGen > 1 && stageGen != 9
        || refStage
        && ( stageType != *(int *)(refStage + 1636)
          || stageType == 12
          && ( *(unsigned char *)(stage + 1664) != *(unsigned char *)(refStage + 1664)
            || *(unsigned char *)(stage + 1665) != *(unsigned char *)(refStage + 1665)
            || *(unsigned char *)(stage + 1666) != *(unsigned char *)(refStage + 1666) )
          || stageGen != *(int *)(refStage + 1660)
          || stageGen == 9 && *(unsigned char *)(stage + 1667) != *(unsigned char *)(refStage + 1667) ) )
      {
        return 0;
      }
      return 1;
    default:
      return 0;
  }
}

/* ---- CloneShader  0x004FCC00 ----  [HIGH] */
int __cdecl CloneShader(const void **source)
{
  int dest;
  int stageIndex;
  const void **stageSrc;

  memcpy( shaderParseName, source, 0x198u );
  memset( shaderParseStageStorage, 0, 0x3440u );
  dest = (int)source[84];
  stageIndex = 0;
  if ( dest > 0 )
  {
    dest = (int)shaderParseStageStorage;
    stageSrc = source + 85;
    do
    {
      shaderParseStages[stageIndex] = dest;
      memcpy( (void *)dest, *stageSrc, 0x688u );
      ++stageIndex;
      ++stageSrc;
      dest += 1672;
    }
    while ( stageIndex < (int)source[84] );
  }
  return dest;
}

/* ---- R_CacheableStaticModelShader  0x004FCC70 ----  VERIFIED */
char *__cdecl R_CacheableStaticModelShader(int shader)
{
  shaderDef_t *sourceShader = (shaderDef_t *)shader;
  int flags;
  char *result;
  int count;
  int i;
  char c;
  int stageIndex;
  int *stageBits;
  unsigned int newBits;
  char name[68];

  if ( sourceShader->numDeforms )
    return 0;
  if ( *(unsigned char *)(shader + 84) & 0x80 )   /* lightingFlags bit 7: mov al / test al,al / js */
    return 0;
  flags = sourceShader->surfaceFlags;
  if ( (flags & 0x3FF5FD00) != 0 && (flags & 0x3FF7FC00) != 0 )
    return 0;
  if ( !sourceShader->numUnfoggedPasses )
    return 0;
  if ( !R_CanOptimizeStaticModelStage( (int)sourceShader->stages[0], 0 ) )
    return 0;
  for ( count = 1; count < sourceShader->numUnfoggedPasses; count++ )
  {
    if ( !R_CanOptimizeStaticModelStage( (int)sourceShader->stages[count], (int)sourceShader->stages[0] ) )
      return 0;
  }
  name[0] = 63;
  strcpy( &name[1], (const char *)shader );
  result = R_FindShaderByName( name );
  if ( result == tr_defaultShader )
  {
    CloneShader( (const void **)shader );
    i = 0;
    do
    {
      c = name[i];
      shaderParseName[i++] = c;
    }
    while ( c );
    /* Mac 1.3 order: lightingFlags &= ~0x18, then flags |= 4 */
    (*(unsigned int *)(tr_shaderParseScratch + 0x4454)) &= 0xFFFFFFE7;
    (*(unsigned int *)(tr_shaderParseScratch + 0x444C)) |= 4u;
    shaderParseOptimizedBackend = tr_cachedStaticModelSurfaceType;
    stageIndex = 0;
    if ( sourceShader->numUnfoggedPasses > 0 )
    {
      stageBits = &shaderParseStage0StateBits;
      do
      {
        newBits = *stageBits & 0xFFEFFFFF;
        *(stageBits - 417) &= 0xFFFFFFE7;
        *stageBits = newBits;
        ++stageIndex;
        stageBits += 418;
      }
      while ( stageIndex < sourceShader->numUnfoggedPasses );
    }
    return GeneratePermanentShader();
  }
  return result;
}

/* ---- R_MergeShaderList  0x004FCDF0 ----  [HIGH] */
void __cdecl R_MergeShaderList(char **shaders, int count, int sheetIndex)
{
  char *mergedShader;
  int stageIdx;
  int *stagePtr;
  unsigned int bundleIdx;
  int *bundleImage;
  int i;
  int oldImage;
  int newImage;

  if ( count == 1 )
  {
    mergedShader = *shaders;
  }
  else
  {
    CloneShader( (const void **)*shaders );
    sprintf( (char *)shaderParseName, "*sheet%03i", sheetIndex );
    mergedShader = GeneratePermanentShader();
    if ( mergedShader == tr_defaultShader )
    {
      ri.Printf( 2, "WARNING: too many shaders after building texture sheets\n" );
      return;
    }
  }
  *((int *)mergedShader + 19) &= ~0x800u;
  oldImage = *((int *)*shaders + 97);
  newImage = *(int *)(oldImage + 104);
  stageIdx = 0;
  if ( *((int *)mergedShader + 84) > 0 )
  {
    stagePtr = (int *)(mergedShader + 340);
    do
    {
      bundleIdx = 0;
      bundleImage = (int *)(*stagePtr + 4);
      do
      {
        if ( !*bundleImage )
          break;
        if ( *bundleImage == oldImage )
          *bundleImage = newImage;
        ++bundleIdx;
        bundleImage += 50;
      }
      while ( bundleIdx < 8 );
      ++stageIdx;
      ++stagePtr;
    }
    while ( stageIdx < *((int *)mergedShader + 84) );
  }
  if ( r_debugOptTex->integer )
    ri.Printf( 0, "merging %i shader(s):\n", count );
  for ( i = 0; i < count; ++i )
  {
    if ( r_debugOptTex->integer )
      ri.Printf( 0, "-> %s\n", shaders[i] );
    *((int *)shaders[i] + 19) |= 0x1000u;
    *((int *)shaders[i] + 96) = (int)mergedShader;
  }
}

/* ---- compare_mergable_shaders  0x004FCF20 ----  VERIFIED */
int __cdecl compare_mergable_shaders(int *elemA, int *elemB)
{
  int shaderA;
  int shaderB;
  int result;

  shaderA = *elemA;
  shaderB = *elemB;
  result = (*(int *)(*(int *)(shaderA + 388) + 104) - *(int *)(*(int *)(shaderB + 388) + 104)) / 120;
  /* a single return of `result` (not an early return): the copy inlined into
     R_MergeShadersForImageSheets keeps retail's registers */
  if ( !result )
    result = CompareMergableShaders( shaderB, shaderA, *(int *)(shaderA + 388), *(int *)(shaderB + 388) );
  return result;
}

/* ---- R_MergeShadersForImageSheets  0x004FCF70 ----  [HIGH] */
void R_MergeShadersForImageSheets()
{
  int shaderList[4096];
  int numCollected;
  int i;
  int shader;
  int flags;
  int sheetIndex;
  int groupStart;
  int groupEnd;

  numCollected = 0;
  for ( i = 0; i < ((int *)(void *)&tr_shaderRegistry)[0]; ++i )
  {
    shader = (tr_shaderRegistry.shaders)[i];
    flags = *(int *)(shader + 76);
    if ( (flags & 0x800) != 0 )
    {
      *(int *)(shader + 76) = flags & 0xFFFFF7FF;
      shader = (tr_shaderRegistry.shaders)[i];
      if ( !*(int *)(*(int *)(shader + 388) + 104) )
        *(int *)(shader + 388) = 0;
      else
        shaderList[numCollected++] = shader;
    }
  }
  sheetIndex = 0;
  qsort_m( (unsigned int)shaderList, numCollected, 4u, (int (__cdecl *)(unsigned int, unsigned char *))compare_mergable_shaders );
  for ( groupStart = 0; groupStart < numCollected; groupStart = groupEnd )
  {
    for ( groupEnd = groupStart + 1;
          groupEnd < numCollected && !compare_mergable_shaders( &shaderList[groupStart], &shaderList[groupEnd] );
          groupEnd++ )
      ;
    R_MergeShaderList( (char **)&shaderList[groupStart], groupEnd - groupStart, sheetIndex++ );
  }
}

/* ---- R_SetupTextureCoordinateRemap  0x004FD0B0 ----  [HIGH] */
void __cdecl R_SetupTextureCoordinateRemap(
        int shader,
        float *scale,
        float *offset,
        int *outTallAxis,
        int *outWideAxis)
{
  int sheet;
  int image;
  int flags;
  cvar_t *picmipCvar;
  int picmipValue;
  int picmip;
  int clamped;

  sheet = *(int *)(shader + 388);
  image = *(int *)(sheet + 104);
  /* 0x004FD0BC cmp cx,[eax+42h] / sbb / neg: unsigned 16-bit compare */
  *outTallAxis = *(unsigned short *)(sheet + 64) < *(unsigned short *)(sheet + 66);
  *outWideAxis = 1 - *outTallAxis;
  flags = *(int *)(sheet + 100);
  picmipCvar = r_picmip2;
  if ( (flags & 4) == 0 )
    picmipCvar = r_picmip;
  picmipValue = picmipCvar->integer;
  if ( (flags & 2) == 0 )
  {
    picmip = 0;                         /* 0x004FD0F6 xor esi,esi (whole register) */
  }
  else
  {
    clamped = picmipValue > 3 ? 3 : picmipValue;
    picmip = clamped & ~(clamped >> 31);
  }
  scale[0] = 1.0f / (float)*(unsigned short *)(image + 64);
  scale[1] = 1.0f / (float)*(unsigned short *)(image + 66);
  offset[0] = (float)*(unsigned short *)(sheet + 108) * scale[0];
  offset[1] = (float)*(unsigned short *)(sheet + 110) * scale[1];
  /* 0x004FD16B shr bp,cl / 0x004FD189 shr ax,cl: logical 16-bit shifts of the unsigned sheet size */
  scale[*outTallAxis] = (float)(unsigned short)(*(unsigned short *)(sheet + 64) >> picmip) * scale[*outTallAxis];
  scale[*outWideAxis] = (float)(unsigned short)(*(unsigned short *)(sheet + 66) >> picmip) * scale[*outWideAxis];
}

/* ---- R_RemapTextureCoordinatesForSheet  0x004FD1A0 ----  [HIGH] */
int __cdecl R_RemapTextureCoordinatesForSheet(int numVerts, float (*texCoords)[2], int shader)
{
  int i;
  rfloat s;
  rfloat t;
  int minorAxis;
  int majorAxis;
  float scale[2];
  float offset[2];

  R_SetupTextureCoordinateRemap( shader, scale, offset, &minorAxis, &majorAxis );
  for ( i = 0; i < numVerts; ++i )
  {
    s = texCoords[i][minorAxis];
    t = texCoords[i][majorAxis];
    texCoords[i][0] = s * scale[0] + offset[0];
    texCoords[i][1] = t * scale[1] + offset[1];
  }
  return i;
}

char *__cdecl R_GetShaderByHandle(int a1);

/* ---- RE_GetShaderName  0x004FD210 ----  VERIFIED */
char *__cdecl RE_GetShaderName(int hShader)
{
  char *result;

  result = R_GetShaderByHandle(hShader);
  if ( result == tr_defaultShader )
    return "";
  return result;
}

/* ---- R_GetShaderByHandle  0x004FD250 ----  VERIFIED */
char *__cdecl R_GetShaderByHandle(int hShader)
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

/* ---- R_ShaderList_f  0x004FD280 ----  [HIGH] */
void R_ShaderList_f()
{
  int count;
  int i;
  int shader;
  int stage;
  int mtState;
  void (*iterator)();
  const char *nameFmt;
  const char *lightStr;
  const char *mtStr;
  const char *edgeStr;
  const char *iterStr;

  ri.Printf( 0, "-----------------------\n" );
  count = 0;
  for ( i = 0; i < ((int *)(void *)&tr_shaderRegistry)[0]; ++i )
  {
    if ( ri.Cmd_Argc() > 1 )
      shader = (tr_shaderRegistry.sortedShaders)[i];
    else
      shader = (tr_shaderRegistry.shaders)[i];
    ri.Printf( 0, "%i ", *(int *)(shader + 336) );
    if ( *(int *)(shader + 64) >= 0 )
      ri.Printf(0, "L ");
    else
      ri.Printf(0, "  ");
    stage = *(int *)(shader + 340);
    if ( !stage )
      ri.Printf(0, "      ");
    else
    {
      mtState = *(int *)(stage + 348);
      if ( mtState == 260 )
        ri.Printf(0, "MT(a) ");
      else if ( mtState == 8448 )
        ri.Printf(0, "MT(m) ");
      else if ( mtState == 8449 )
        ri.Printf(0, "MT(d) ");
      else
        ri.Printf(0, "      ");
    }
    if ( (*(unsigned char *)(shader + 76) & 2) != 0 )
      ri.Printf(0, "E ");
    else
      ri.Printf(0, "  ");
    iterator = *(void (**)())(shader + 372);
    if ( (char *)iterator == (char *)tr_stageIteratorFunc )
      ri.Printf(0, "gen ");
    else if ( iterator == RB_StageIteratorSky )
      ri.Printf(0, "sky ");
    else
      ri.Printf(0, "    ");
    if ( (*(unsigned char *)(shader + 76) & 1) != 0 )
      ri.Printf(0, ": %s (DEFAULTED)\n", shader);
    else
      ri.Printf(0, ": %s\n", shader);
    ++count;
  }
  ri.Printf( 0, "%i total shaders\n", count );
  ri.Printf( 0, "------------------\n" );
}

static ShaderTextHashNode tr_shaderStringPointers[100000];   /* 0x016DCA60 */

/* ---- BuildShaderChecksumLookup  0x004FD3F0 ----  VERIFIED
 * Retail scales the hash to a byte offset once (shl esi,3) and addresses both
 * node fields from the table base ([esi+table], [esi+table+4]); a node
 * pointer local or a table[hash] index makes VC7 materialise the address. */
#define SHADER_TEXT_HASH_NODE_AT( offset ) ( (ShaderTextHashNode *)( (char *)shaderTextHashTable + (offset) ) )
void __cdecl BuildShaderChecksumLookup(void)
{
  int    braceError;
  ShaderTextHashNode *poolPtr;
  char  *tokenStart;
  char  *token;
  int    hash;
  ShaderTextHashNode *next;
  ShaderTextHashNode *entry;
  char  *data_p;

  data_p = (char *)s_shaderText;
  memset( shaderTextHashTable, 0, 0x8000u );
  if ( s_shaderText )
  {
    braceError = 0;
    poolPtr = tr_shaderStringPointers;
    while ( 1 )
    {
      tokenStart = data_p;
      if ( parseInfo->ungetReady )
      {
        parseInfo->ungetReady = qfalse;
        data_p = parseInfo->ungetTokenSave;
        parseInfo->currentLine = parseInfo->ungetLineSave;
      }
      token = Com_ParseExt( &data_p, qtrue );
      if ( !*token )
        break;
      if ( !Q_stricmpn( token, "{", 99999 ) )
      {
        Com_UngetToken();
        if ( Com_SkipBracedSection( &data_p, 5 ) )
        {
          ri.Printf(2, "Shader brace nesting depth exceeded... missing '}'?\n");
          braceError = 1;
        }
        continue;
      }
      {
        if ( braceError )
        {
          ri.Printf( 2, "Look before shader '%s'\n", token );
          braceError = 0;
        }
        hash = (unsigned short)tr_shader_generateHashValue( token );
        hash *= sizeof( ShaderTextHashNode );
        if ( !SHADER_TEXT_HASH_NODE_AT( hash )->text )
        {
          SHADER_TEXT_HASH_NODE_AT( hash )->text = tokenStart;
          continue;
        }
        {
          if ( (int)poolPtr >= (int)(tr_shaderStringPointers + 100000) )
            ri.Error( 1, "\x15" "MAX_SHADER_STRING_POINTERS exceeded, too many shaders" );
          next = SHADER_TEXT_HASH_NODE_AT( hash )->next;
          entry = poolPtr;
          ++poolPtr;
          entry->text = tokenStart;
          entry->next = next;
          SHADER_TEXT_HASH_NODE_AT( hash )->next = entry;
        }

      }

    }
  }
}
#undef SHADER_TEXT_HASH_NODE_AT

/* ---- ScanAndLoadShaderFiles  0x004FD560 ----  [HIGH] */
void __cdecl ScanAndLoadShaderFiles( void )
{
  char       **scriptsList;
  char       **fxList;
  int          numScripts;
  int          numFx;
  int          numFiles;
  int          sum;
  int          size;
  int          loadIndex;
  int          fileIndex;
  int          j;
  const char  *pathFormat;
  const char  *shaderFile;
  char        *shaderTextBuffer;
  char         shadername[64];
  char        *buffers[4096];

  sum = 0;
  scriptsList = (char **)ri.FS_ListFiles( "scripts", ".shader", &numScripts );
  numFiles = numScripts;
  fxList = (char **)ri.FS_ListFiles( "fxshaders", ".shader", &numFx );
  numFiles += numFx;
  if ( scriptsList && numScripts )
  {
    if ( numFiles > 4096 )
      numFiles = 4096;
    for ( loadIndex = 0; loadIndex < numFiles; loadIndex++ )
    {
      if ( loadIndex < numScripts )
        Com_sprintf(shadername, 64, "scripts/%s", scriptsList[loadIndex]);
      else
        Com_sprintf(shadername, 64, "fxshaders/%s", fxList[loadIndex - numScripts]);
      ri.Printf( 0, "...loading '%s'\n", shadername );
      size = ri.FS_ReadFile( shadername, &buffers[loadIndex] );
      sum += size;
      if ( !buffers[loadIndex] )
        ri.Error( 1, va( "EXE_ERR_COULDNT_LOAD\x15%s", shadername ) );
    }
    shaderTextBuffer = (char *)ri.Hunk_Alloc( sum + 2 * numFiles );
    s_shaderText = (int)shaderTextBuffer;
    for ( fileIndex = 0; fileIndex < numFiles; fileIndex++ )
    {
      strcat( shaderTextBuffer, "\n" );
      strcat(shaderTextBuffer, buffers[fileIndex]);
    }
    for ( j = numFiles - 1; j >= 0; j-- )
    {
      ri.FS_FreeFile( buffers[j] );
      buffers[j] = 0;
    }
    ri.FS_FreeFileList( fxList );
    ri.FS_FreeFileList( scriptsList );
    BuildShaderChecksumLookup();
  }
  else
  {
    ri.Printf( 2, "WARNING: no shader files found\n" );
  }
}

/* ---- CreateDefaultShader  0x004FD780 ----  [HIGH] */
char *CreateDefaultShader()
{
  char *result;

  memset(shaderParseName, 0, 0x198u);
  memset(shaderParseStageStorage, 0, 0x3440u);
  strncpy(shaderParseName, "<default>", 0x3Fu);
  shaderParseName_63 = 0;
  shaderParseLightmapIndex = -1;
  shaderParseFlags = 1;
  shaderParseSurfaceFlags = 16;
  shaderParseStage0Bundle0Image[0] = tr_defaultImage;
  shaderParseStage0Bundle0TexCoordComponentCount = 2;
  shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
  shaderParseStageStorage[0] = 1;
  shaderParseStage0StateBits = 256;
  result = FinishShader();
  tr_defaultShader = result;
  return result;
}

/* ---- CreateShadowShader  0x004FD810 ----  [HIGH] */
char *CreateShadowShader()
{
  char *result;

  memset(shaderParseName, 0, 0x198u);
  memset(shaderParseStageStorage, 0, 0x3440u);
  strncpy(shaderParseName, "<stencil shadow>", 0x3Fu);
  shaderParseName_63 = 0;
  shaderParseLightmapIndex = -1;
  shaderParseSort = 1101004800;
  result = FinishShader();
  tr_stencilShadowShader = (int)result;
  return result;
}

/* ---- CreateShowTrisShader  0x004FD870 ----  [HIGH] */
void CreateShowTrisShader(void)
{
  shaderDef_t   *shader = (shaderDef_t *)(tr_shaderParseScratch + 0x4400);
  shaderStage_t *stage  = (shaderStage_t *)(tr_shaderParseScratch + 0x45A8);

  memset(shaderParseName, 0, 0x198u);
  memset(shaderParseStageStorage, 0, 0x3440u);
  Q_strncpyz((char *)shaderParseName, "<showtris>", 64);
  shader->lightmapIndex = -1;
  shader->flags = 1024;
  shader->surfaceFlags = 16;
  shader->cullType = 2;
  shaderParseStage0Bundle0Image[0] = tr_whiteImage;
  shaderParseStage0Bundle0TexCoordComponentCount = 2;
  shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
  stage->flags = 1;
  stage->stateBits = 4352;
  stage->rgbGen = 13;
  stage->alphaGen = 0;
  if ( r_optimize->integer )
  {
    stage->constantColor[0] = tr_identityLightByte;
    stage->constantColor[1] = tr_identityLightByte;
    stage->constantColor[2] = 0;
  }
  else
  {
    stage->constantColor[0] = tr_identityLightByte;
    stage->constantColor[1] = tr_identityLightByte;
    stage->constantColor[2] = tr_identityLightByte;
  }
  tr_showTrisShader = (int)FinishShader();
}

/* ---- CreateShowImagesShader  0x004FD940 ----  [HIGH] */
void CreateShowImagesShader(void)
{
  shaderDef_t   *shader = (shaderDef_t *)(tr_shaderParseScratch + 0x4400);
  shaderStage_t *stage  = (shaderStage_t *)(tr_shaderParseScratch + 0x45A8);

  memset(shaderParseName, 0, 0x198u);
  memset(shaderParseStageStorage, 0, 0x3440u);
  Q_strncpyz((char *)shaderParseName, "<showimages>", 64);
  shader->lightmapIndex = -1;
  shader->flags = 1024;
  shader->surfaceFlags = 16;
  shader->cullType = 2;
  shaderParseStage0Bundle0Image[0] = tr_whiteImage;
  shaderParseStage0Bundle0TexCoordComponentCount = 2;
  shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
  stage->flags = 1;
  stage->stateBits = 0x10000;
  stage->rgbGen = 1;
  stage->alphaGen = 0;
  stage->constantColor[0] = tr_identityLightByte;
  stage->constantColor[1] = tr_identityLightByte;
  stage->constantColor[2] = tr_identityLightByte;
  tr_showImagesShader = (int)FinishShader();
}

/* ---- CreateScreenShader  0x004FDA00 ----  [HIGH] */
char *CreateScreenShader()
{
  char *result;

  memset(shaderParseName, 0, 0x198u);
  memset(shaderParseStageStorage, 0, 0x3440u);
  strncpy(shaderParseName, "<screen>", 0x3Fu);
  shaderParseName_63 = 0;
  shaderParseLightmapIndex = -4;
  shaderParseFlags = 1024;
  shaderParseSurfaceFlags = 80;
  shaderParseCullType = 2;
  shaderParseStage0Bundle0Image[0] = tr_screenImage;
  shaderParseStage0Bundle0TexCoordComponentCount = 2;
  shaderParseStage0Bundle0TextureEnvMode[0] = 8448;
  shaderParseStageStorage[0] = 1;
  shaderParseStage0StateBits = 101;
  shaderParseStage0RgbGen = 5;
  shaderParseStage0AlphaGen = 4;
  result = FinishShader();
  tr_screenImageShader = (int)result;
  return result;
}

/* ---- CreateInternalShaders  0x004FDAB0 ----  [HIGH] */
char *CreateInternalShaders()
{
  tr_numShaders[0] = 0;
  CreateDefaultShader();
  CreateShadowShader();
  CreateShowTrisShader();
  CreateShowImagesShader();
  return CreateScreenShader();
}

/* ---- CreateExternalShaders  0x004FDAE0 ----  [HIGH] */
char *CreateExternalShaders()
{
  char *result;

  tr_flareShader = (int)R_FindShader("flareShader", -1, 1, (const char *)4);
  tr_spotLightShader = (int)R_FindShader("spotLight", -1, 1, (const char *)4);
  result = R_FindShader("dlightshader", -1, 1, (const char *)4);
  tr_dlightShader = (int)result;
  return result;
}

/* ---- R_InitShaders  0x004FDB30 ----  [HIGH] */
char *__cdecl R_InitShaders( void )
{
  char *result;

  glfogNum = 0;
  ri.Printf(0, "Initializing Shaders\n");
  memset(shaderHashTable, 0, 0x4000u);
  tr_numShaders[0] = 0;
  CreateDefaultShader();
  CreateShadowShader();
  CreateShowTrisShader();
  CreateShowImagesShader();
  CreateScreenShader();
  ScanAndLoadShaderFiles();
  tr_flareShader = (int)R_FindShader("flareShader", -1, 1, (const char *)4);
  tr_spotLightShader = (int)R_FindShader("spotLight", -1, 1, (const char *)4);
  result = R_FindShader("dlightshader", -1, 1, (const char *)4);
  tr_dlightShader = (int)result;
  return result;
}
