/*
 * @fidelity: verified
 */

#ifndef CL_REFAPI_H
#define CL_REFAPI_H

#define REF_API_VERSION     14      /* `cmp edx, 0Eh` at 0x004B4A5E */


#include "../renderer/tr_public.h"


extern refexport_t  re;

/* The client's mirror names for the slots of `re` (0x01432860 + offset).  They
 * keep the types the decompiled callers were written against: an unprototyped
 * int (*)() for calls, a void * for the four slots read only as data. */
#define re_ShutdownPtr               ( *(int (**)())&re.Shutdown )                /* +0x00 */
#define re_BeginRegistration         ( *(int (**)())&re.BeginRegistration )       /* +0x04 */
#define re_GetXModelByHandle         re.GetXModelByHandle                         /* +0x08 */
#define re_RegisterModel             ( *(int (**)())&re.RegisterModel )           /* +0x0C */
#define re_RegisterShader            ( *(int (**)())&re.RegisterShader )          /* +0x10 */
#define re_RegisterShaderNoMip       ( *(int (**)())&re.RegisterShaderNoMip )     /* +0x14 */
#define re_LoadWorldMap              re.LoadWorldMap                              /* +0x18 */
#define re_FinishLoadingModels       ( *(void **)&re.FinishLoadingModels )        /* +0x1C */
#define re_SetIgnorePrecacheErrors   re.SetIgnorePrecacheErrors                   /* +0x20 */
#define re_GetShaderFromModel        re.GetShaderFromModel                        /* +0x28 */
#define re_GetShaderName             re.GetShaderName                             /* +0x30 */
#define re_GetFarPlaneDist           ( *(void **)&re.GetFarPlaneDist )            /* +0x34 */
#define re_EndRegistration           re.EndRegistration                           /* +0x38 */
#define re_ClearScene                ( *(void **)&re.ClearScene )                 /* +0x3C */
#define re_AddRefEntityToScene       ( *(int (**)())&re.AddRefEntityToScene )     /* +0x40 */
#define re_AddPolyToScene            ( *(int (**)())&re.AddPolyToScene )          /* +0x44 */
#define re_AddPolysToScene           ( *(int (**)())&re.AddPolysToScene )         /* +0x48 */
#define re_AddLightToScene           ( *(int (**)())&re.AddLightToScene )         /* +0x4C */
#define re_AddCoronaToScene          ( *(int (**)())&re.AddCoronaToScene )        /* +0x54 */
#define re_SetFog                    re.SetFog                                    /* +0x58 */
#define re_SaveFogState              re.SaveFogState                              /* +0x5C */
#define re_RestoreFogState           re.RestoreFogState                           /* +0x60 */
#define re_RenderScene               ( *(int (**)())&re.RenderScene )             /* +0x64 */
#define re_ClearFlares               re.ClearFlares                               /* +0x68 */
#define re_SetColor                  ( *(int (**)())&re.SetColor )                /* +0x6C */
#define re_DrawStretchPic            ( *(int (**)())&re.StretchPic )              /* +0x70 */
#define re_StretchPicGradient        re.StretchPicGradient                        /* +0x74 */
#define re_StretchPicRotate          re.StretchPicRotate                          /* +0x78 */
#define re_DrawQuadPic               re.DrawQuadPic                               /* +0x7C */
#define re_DrawStretchRawPtr         re.StretchRaw                                /* +0x80 */
#define re_UploadCinematicPtr        re.UploadCinematic                           /* +0x84 */
#define re_BeginFrame                ( *(int (**)())&re.BeginFrame )              /* +0x88 */
#define re_EndFrame                  ( *(int (**)())&re.EndFrame )                /* +0x8C */
#define re_SaveScreen                ( *(void **)&re.SaveScreen )                 /* +0x90 */
#define re_BlendSavedScreen          re.BlendSavedScreen                          /* +0x94 */
#define re_LerpTag                   re.LerpTag                                   /* +0x98 */
#define re_ModelBounds               ( *(int (**)())&re.ModelBounds )             /* +0x9C */
#define re_TrackStatistics           re.TrackStatistics                           /* +0xA0 */
#define re_PickShader                re.PickShader                                /* +0xA4 */
#define re_RegisterFont              re.RegisterFont                              /* +0xA8 */
#define re_GetEntityToken            re.GetEntityToken                            /* +0xAC */
#define re_ResetImageAllocations     re.ResetImageAllocations                     /* +0xB0 */
#define re_CubemapShot               re.CubemapShot                               /* +0xB8 */
#define re_CubemapWaterShot          re.CubemapWaterShot                          /* +0xBC */
#define re_LocateDebugStrings        re.LocateDebugStrings                        /* +0xC0 */
#define re_LocateDebugLines          re.LocateDebugLines                          /* +0xC4 */
#define re_AddPlume                  ( *(void **)&re.AddPlume )                   /* +0xC8 */
#define re_Text_Width                re.Text_Width                                /* +0xCC */
#define re_Text_Height               re.Text_Height                               /* +0xD0 */
#define re_Text_Paint                re.Text_Paint                                /* +0xD4 */
#define re_Text_ConsoleWidth         re.Text_ConsoleWidth                         /* +0xD8 */
#define re_Text_ConsolePaint         re.Text_ConsolePaint                         /* +0xDC */
#define re_Text_PaintWithCursor      re.Text_PaintWithCursor                      /* +0xE0 */


void CL_RefApiIntegrityCheck( const char *where );

void CL_BssSnapshot( void );
int  CL_BssDiff( const char *where );

#endif
