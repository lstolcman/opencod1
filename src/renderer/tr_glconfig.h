/*
 * tr_glconfig.h -- glconfig_t, shared by the renderer (tr_public.h, and
 * tr_local.h's glConfig) and the client (cl_refapi.h).
 */
#ifndef TR_GLCONFIG_H
#define TR_GLCONFIG_H

/* The renderer's GL configuration, one 0xA0-byte object at 0x016C3A60 (glConfig,
 * renderer/tr_init.c).  RE_BeginRegistration copies it out whole (`rep movsd`,
 * 0x005035B1) into the client's cls_glconfig; the cgame gets the same layout. */
typedef struct {
    /* +0x00 */ char   *renderer_string;         /* GL_RENDERER 0x0050ACB6 */
    /* +0x04 */ char   *vendor_string;           /* GL_VENDOR 0x0050ACA6 */
    /* +0x08 */ char   *version_string;          /* GL_VERSION 0x0050ACC6 */
    /* +0x0C */ char   *extensions_string;       /* GL_EXTENSIONS 0x0050ACD6 */
    /* +0x10 */ char   *wextensions_string;      /* WGL, CoD1 addition, 0x0050ACF1 */
    /* +0x14 */ int     maxTextureSize;          /* GL_MAX_TEXTURE_SIZE, 0x004B0C2A */
    /* +0x18 */ int     maxActiveTextures;
    /* +0x1C */ int     maxLights;               /* GL_MAX_LIGHTS (0x0D31), clamped 8..16 at 0x004B0C52; read by R_SetHwLightGlobals. CoD1 addition -- RTCW has no such field. */
    /* +0x20 */ int     colorBits;
    /* +0x24 */ int     depthBits;
    /* +0x28 */ int     stencilBits;
    /* +0x2C */ int     deviceSupportsGamma;     /* WG_CheckHardwareGamma 0x00515C35 */
    /* +0x30 */ int     maxAnisotropy;
    /* +0x34 */ int     maxTextureFilterAnisotropy;
    /* +0x38 */ int     textureEnvAddAvailable;
    /* +0x3C */ int     textureCubeMap;
    /* +0x40 */ int     textureEnvCombine;
    /* +0x44 */ int     textureEnvDot3;
    /* +0x48 */ int     ARBVertexBufferObject;
    /* +0x4C */ int     ARBVertexProgram;
    /* +0x50 */ int     rescaleNormal;
    /* +0x54 */ int     NVFogAvailable;
    /* +0x58 */ int     NVFogMode;
    /* +0x5C */ int     NVVertexArrayRange;
    /* +0x60 */ int     NVFenceAvailable;        /* set by GLW_InitExtensions for GL_NV_fence */
    /* +0x64 */ int     NVRegisterCombiners;
    /* +0x68 */ int     NVTextureShader;
    /* +0x6C */ int     maxPNTrianglesTessellationLevel;
    /* +0x70 */ int     pnTrianglesNormalMode;
    /* +0x74 */ int     pnTrianglesPointMode;
    /* +0x78 */ int     ATIVertexArrayObject;
    /* +0x7C */ int     ATIElementArray;
    /* +0x80 */ int     ATIFragmentShader;
    /* +0x84 */ int     vidWidth;
    /* +0x88 */ int     vidHeight;
    /* +0x8C */ float   windowAspect;            /* written by GLW_SetMode 0x00508665 */
    /* +0x90 */ int     displayFrequency;
    /* +0x94 */ int     isFullscreen;
    /* +0x98 */ int     stereoEnabled;
    /* +0x9C */ int     smpActive;               /* Q3's last member after stereoEnabled, as cg_public.h;
                                                no xrefs, always 0 */
} glconfig_t;

#endif /* TR_GLCONFIG_H */
