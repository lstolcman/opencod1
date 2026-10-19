/*
 * @fidelity: verified
 *
 * tr_public.h -- the renderer's import table (refimport_t), RTCW-style.
 *
 * Canonical definition of refimport_t (was duplicated in cl_refapi.h).  The
 * engine's table reaches `ri` through GetRefAPI (renderer/tr_init.c) and the
 * renderer/GLimp/jpeg glue call ri.Printf(...), ri.Cvar_Get(...), etc.
 * Members are unprototyped int-returning pointers, at the retail table's
 * offsets; this replaces the flat ri_* globals the decompile emitted.
 */
#ifndef TR_PUBLIC_H
#define TR_PUBLIC_H

#include "tr_glconfig.h"   /* glconfig_t */

typedef struct {
    int (*Printf)();                    /* +0x00 CL_RefPrintf */
    int (*Error)();                     /* +0x04 Com_Error */
    int (*Milliseconds)();              /* +0x08 CL_ScaledMilliseconds */
    int (*Hunk_Alloc)();                /* +0x0C Hunk_AllocInternal */
    int (*Hunk_AllocateTempMemory)();   /* +0x10 */
    int (*Malloc)();                    /* +0x14 Z_MallocInternal */
    int (*Free)();                      /* +0x18 Z_FreeInternal */
    int (*Hunk_FreeTempMemory)();       /* +0x1C */
    int (*Cvar_Get)();                  /* +0x20 */
    int (*Cvar_FindVar)();              /* +0x24 */
    int (*Cvar_Set)();                  /* +0x28 */
    int (*Cmd_AddCommand)();            /* +0x2C */
    int (*Cmd_RemoveCommand)();         /* +0x30 */
    int (*Cmd_Argc)();                  /* +0x34 */
    int (*Cmd_Argv)();                  /* +0x38 */
    int (*Cmd_ExecuteText)();           /* +0x3C Cbuf_ExecuteText */
    int (*SaveCvarsToBuffer)();         /* +0x40 Com_SaveCvarsToBuffer */
    int (*LoadCvarsFromBuffer)();       /* +0x44 Com_LoadCvarsFromBuffer */
    int (*FS_FileIsInPAK)();            /* +0x48 */
    int (*FS_ReadFile)();               /* +0x4C */
    int (*FS_FreeFile)();               /* +0x50 */
    int (*FS_ListFiles)();              /* +0x54 */
    int (*FS_FreeFileList)();           /* +0x58 */
    int (*FS_WriteFile)();              /* +0x5C */
    int (*FS_FileExists)();             /* +0x60 */
    int (*FS_FOpenFileByMode)();        /* +0x64 */
    int (*FS_FCloseFile)();             /* +0x68 */
    int (*FS_Read)();                   /* +0x6C */
    int (*FS_Write)();                  /* +0x70 */
    int (*CM_SaveLump)();               /* +0x74 */
    int (*CM_GetPlaneNum)();            /* +0x78 */
    int (*CIN_UploadCinematic)();       /* +0x7C */
    int (*CIN_PlayCinematic)();         /* +0x80 */
    int (*CIN_RunCinematic)();          /* +0x84 */
    int (*CG_GetGameModel)( short handle ); /* +0x88 callee reads movsx word [esp+4] (0x004118B0) */
    int (*CG_DObjCalcPose)();           /* +0x8C */
    int (*SCR_AdjustFrom640)();         /* +0x90 */
    int (*GetFontInfo)();               /* +0x94 CL_GetFontInfo */
} refimport_t;

extern refimport_t ri;

/* The renderer's export table (0x39 slots).  GetRefAPI fills its own copy
 * (0x00CA26C8) and returns it; CL_InitRef copies it whole into the client's
 * `re` (0x01432860). */
typedef struct {
    /* +0x00 */ void  (*Shutdown)( int destroyWindow );
    /* +0x04 */ void  (*BeginRegistration)( glconfig_t *glconfigOut );
    /* +0x08 */ int   (*GetXModelByHandle)();
    /* +0x0C */ int   (*RegisterModel)( const char *name, int arg2 );
    /* +0x10 */ int   (*RegisterShader)( const char *name, int arg2 );
    /* +0x14 */ int   (*RegisterShaderNoMip)( const char *name, int arg2 );
    /* +0x18 */ int   (*LoadWorldMap)();
    /* +0x1C */ int   (*FinishLoadingModels)();
    /* +0x20 */ int   (*SetIgnorePrecacheErrors)();
    /* +0x24 */ int   (*GetIgnorePrecacheErrors)();
    /* +0x28 */ int   (*GetShaderFromModel)();
    /* +0x2C */ int   (*GetImageMemory)();
    /* +0x30 */ int   (*GetShaderName)();
    /* +0x34 */ int   (*GetFarPlaneDist)();
    /* +0x38 */ int   (*EndRegistration)();
    /* +0x3C */ void  (*ClearScene)( void );
    /* +0x40 */ void  (*AddRefEntityToScene)( const void *ent, int arg2 );
    /* +0x44 */ void  (*AddPolyToScene)( int hShader, int numVerts,
                                         const void *verts );
    /* +0x48 */ void  (*AddPolysToScene)( int hShader, int numVerts,
                                          const void *verts, int numPolys );
    /* +0x4C */ void  (*AddLightToScene)( const float *org, float intensity,
                                          float r, float g, float b );
    /* +0x50 */ int   (*SetFarPlaneDist)();
    /* +0x54 */ void  (*AddCoronaToScene)( const float *org, float r, float g,
                                           float b, float scale, int id,
                                           int visible );
    /* +0x58 */ int   (*SetFog)();
    /* +0x5C */ int   (*SaveFogState)();
    /* +0x60 */ int   (*RestoreFogState)();
    /* +0x64 */ void  (*RenderScene)( const void *fd );
    /* +0x68 */ int   (*ClearFlares)();
    /* +0x6C */ void  (*SetColor)( const float *rgba );
                        /* RE_SetColor (0x004DDCF0) dereferences four floats through the argument and substitutes a default when it is NULL -- a POINTER, not four packed values. */
    /* +0x70 */ void  (*StretchPic)( float x, float y, float w, float h,
                                     float s1, float t1, float s2, float t2,
                                     int hShader );
    /* +0x74 */ int   (*StretchPicGradient)();
    /* +0x78 */ int   (*StretchPicRotate)();
    /* +0x7C */ int   (*DrawQuadPic)();
    /* +0x80 */ int   (*StretchRaw)();
    /* +0x84 */ int   (*UploadCinematic)();
    /* +0x88 */ void  (*BeginFrame)( int stereoFrame );
    /* +0x8C */ void  (*EndFrame)( int *frontEndMsec, int *backEndMsec );
    /* +0x90 */ int   (*SaveScreen)();
    /* +0x94 */ int   (*BlendSavedScreen)();
    /* +0x98 */ int   (*LerpTag)();
    /* +0x9C */ void  (*ModelBounds)( int model, float *mins, float *maxs );
    /* +0xA0 */ int   (*TrackStatistics)();
    /* +0xA4 */ int   (*PickShader)();
    /* +0xA8 */ int   (*RegisterFont)();
    /* +0xAC */ int   (*GetEntityToken)();
    /* +0xB0 */ int   (*ResetImageAllocations)();
    /* +0xB4 */ int   (*FreeImageAllocations)();
    /* +0xB8 */ int   (*CubemapShot)();
    /* +0xBC */ int   (*CubemapWaterShot)();
    /* +0xC0 */ int   (*LocateDebugStrings)();
    /* +0xC4 */ int   (*LocateDebugLines)();
    /* +0xC8 */ int   (*AddPlume)();
    /* +0xCC */ int   (*Text_Width)();
    /* +0xD0 */ int   (*Text_Height)();
    /* +0xD4 */ int   (*Text_Paint)();
    /* +0xD8 */ int   (*Text_ConsoleWidth)();
    /* +0xDC */ int   (*Text_ConsolePaint)();
    /* +0xE0 */ int   (*Text_PaintWithCursor)();
} refexport_t;

#endif /* TR_PUBLIC_H */
