/*
 * @fidelity: verified
 */

#include <stdlib.h>
#include <string.h>

#include "../qcommon/qcommon.h"
#include "cl_refapi.h"
#include "cl_local.h"
#include "../renderer/tr_public.h"

extern glconfig_t glConfig;   /* renderer/tr_init.c */
extern int tr_registered;   /* the renderer's tr (renderer/tr_local.h) */

/* cl.timeoutcount, the first int of the clientActive_t block (0x01432960; storage in
 * cl_refstorage.c, which sizes it as the whole block) */
extern int cl_active;


extern int  CL_RefPrintf();                     /* 0x00411460 cl_main_mp.c */
extern int  CL_ScaledMilliseconds();            /* 0x00411860 */
extern int  CL_GetFontInfo();                   /* 0x004118F0 */
extern int  CG_GetGameModel();                  /* 0x004118B0 */
extern int  CG_DObjCalcPose();                  /* 0x004118D0 */
extern int  SCR_AdjustFrom640();                /* 0x00416810 cl_scrn_mp.c */
extern int  CIN_UploadCinematic();              /* 0x004082B0 */
extern int  CIN_PlayCinematic();                /* 0x004078D0 */
extern int  CIN_RunCinematic();                 /* 0x00407680 */
extern int  CM_SaveLump();                      /* 0x00419F00 */
extern int  CM_GetPlaneNum();                   /* 0x00428130 */
extern int  Hunk_AllocInternal();               /* 0x00432160 */
extern int  Hunk_FreeTempMemory();              /* 0x00432410 */
extern int  Z_FreeInternal();                   /* 0x004318F0 */
extern int  Com_SaveCvarsToBuffer();            /* 0x00438E30 */
extern int  Com_LoadCvarsFromBuffer();          /* 0x00438EB0 */
extern int  FS_FileIsInPAK();                   /* 0x0043AE20 */
extern int  FS_FileExists();                    /* 0x00429310 */

extern int *GetRefAPI( int apiVersion, const void *rimp );

extern void StatMon_Reset( void );

/* ---- CL_InitRef  0x00411920 ----  [CONFIRMED] */
void CL_InitRef( void )
{
    refimport_t  rimp;
    int         *refapi;

    Com_Printf( "----- Initializing Renderer ----\n" );

    /* retail 0x0041193B: the import table is a stack local, filled in this
     * (Q3 source) order and copied into the renderer's ri by GetRefAPI */
    rimp.Cmd_AddCommand          = (int (*)()) Cmd_AddCommand;
    rimp.Cmd_RemoveCommand       = (int (*)()) Cmd_RemoveCommand;
    rimp.Cmd_Argc                = (int (*)()) Cmd_Argc;
    rimp.Cmd_Argv                = (int (*)()) Cmd_Argv;
    rimp.Cmd_ExecuteText         = (int (*)()) Cbuf_ExecuteText;
    rimp.Printf                  = (int (*)()) CL_RefPrintf;
    rimp.Error                   = (int (*)()) Com_Error;
    rimp.Milliseconds            = (int (*)()) CL_ScaledMilliseconds;
    rimp.Hunk_Alloc              = (int (*)()) Hunk_AllocInternal;
    rimp.Hunk_AllocateTempMemory = (int (*)()) Hunk_AllocateTempMemoryInternal;
    rimp.Malloc                  = (int (*)()) Z_MallocInternal;
    rimp.Free                    = (int (*)()) Z_FreeInternal;
    rimp.Hunk_FreeTempMemory     = (int (*)()) Hunk_FreeTempMemory;
    rimp.FS_ReadFile             = (int (*)()) FS_ReadFile;
    rimp.FS_FreeFile             = (int (*)()) FS_FreeFile;
    rimp.FS_WriteFile            = (int (*)()) FS_WriteFile;
    rimp.FS_FreeFileList         = (int (*)()) FS_FreeFileList;
    rimp.FS_ListFiles            = (int (*)()) FS_ListFiles;
    rimp.FS_FileIsInPAK          = (int (*)()) FS_FileIsInPAK;
    rimp.FS_FileExists           = (int (*)()) FS_FileExists;
    rimp.FS_FOpenFileByMode      = (int (*)()) FS_FOpenFileByMode;
    rimp.FS_FCloseFile           = (int (*)()) FS_FCloseFile;
    rimp.FS_Read                 = (int (*)()) FS_Read;
    rimp.FS_Write                = (int (*)()) FS_Write;
    rimp.CM_SaveLump             = (int (*)()) CM_SaveLump;
    rimp.CM_GetPlaneNum          = (int (*)()) CM_GetPlaneNum;
    rimp.Cvar_Get                = (int (*)()) Cvar_Get;
    rimp.Cvar_FindVar            = (int (*)()) Cvar_FindVar;
    rimp.Cvar_Set                = (int (*)()) Cvar_Set;
    rimp.SaveCvarsToBuffer       = (int (*)()) Com_SaveCvarsToBuffer;
    rimp.LoadCvarsFromBuffer     = (int (*)()) Com_LoadCvarsFromBuffer;
    rimp.CIN_UploadCinematic     = (int (*)()) CIN_UploadCinematic;
    rimp.CIN_PlayCinematic       = (int (*)()) CIN_PlayCinematic;
    rimp.CIN_RunCinematic        = (int (*)()) CIN_RunCinematic;
    rimp.CG_GetGameModel         = (int (*)( short )) CG_GetGameModel;
    rimp.CG_DObjCalcPose         = (int (*)()) CG_DObjCalcPose;
    rimp.SCR_AdjustFrom640       = (int (*)()) SCR_AdjustFrom640;
    rimp.GetFontInfo             = (int (*)()) CL_GetFontInfo;

    /* GetRefAPI (0x004B4A30) copies rimp into ri */
    refapi = GetRefAPI( REF_API_VERSION, &rimp );

    Com_Printf( "-------------------------------\n" );

    if ( !refapi ) {
        Com_Error( ERR_FATAL, "EXE_ERR_COULDNT_INIT_REFRESH" );
    }

    /* retail 0x00411AC5: rep movs, 0x39 dwords -- nothing else runs between the
     * copy and Cvar_Set2 (no slot dump, no cl_bssWatch cvar) */
    re = *(refexport_t *)refapi;

    Cvar_Set2( "cl_paused", "0", qtrue );
}

static unsigned char *cl_bssCopy;
static unsigned char *cl_bssLo;
static unsigned int   cl_bssLen;

/* ---- CL_BssBounds  no-address ---- */
static void CL_BssBounds( void )
{
    void *probe[] = {
        (void *) &re_ShutdownPtr,
        (void *) &ri.Printf,          (void *) &cl_debuggraph,
        (void *) &cl_timegraph,       (void *) &cl_graphheight,
        (void *) &sv_maxRate,         (void *) &sv_gametype,
        /* &sv_hostname (0x016BDB5C) stands in for its neighbour &sv_maxclients: taking
           sv_maxclients' address costs LTCG its no-alias guarantee and reschedules the
           sv_maxclients->integer loads in SV_DirectConnect and SV_DropClient */
        (void *) &sv_hostname,        (void *) &cls_state,
        (void *) &whiteShader,        (void *) &glConfig.vidWidth,
        (void *) &glConfig.vidHeight,           (void *) &tr_registered,
        (void *) &re_Text_PaintWithCursor,      (void *) &re_RegisterShaderNoMip,
        (void *) &re_SaveFogState, (void *) &ri.GetFontInfo,
        (void *) &cl_active,          (void *) &clc_clientNum
    };
    unsigned int lo = 0xFFFFFFFFu, hi = 0;
    int i;

    for ( i = 0; i < (int) ( sizeof( probe ) / sizeof( probe[0] ) ); i++ ) {
        unsigned int a = (unsigned int) probe[i];
        if ( a < lo ) { lo = a; }
        if ( a > hi ) { hi = a; }
    }
    cl_bssLo  = (unsigned char *) lo;
    cl_bssLen = ( hi + 4 ) - lo;
}

/* ---- CL_BssSnapshot  no-address ---- */
void CL_BssSnapshot( void )
{
    static int enabled = -1;

    if ( enabled < 0 ) {
        enabled = Cvar_Get( "cl_bssWatch", "0", 0 )->integer ? 1 : 0;
    }
    if ( !enabled ) {
        return;
    }

    if ( !cl_bssCopy ) {
        CL_BssBounds();
        cl_bssCopy = (unsigned char *) malloc( cl_bssLen );
        if ( !cl_bssCopy ) {
            Com_Printf( "CL_BssSnapshot: malloc(%u) failed\n", cl_bssLen );
            return;
        }
        Com_Printf( "CL_BssSnapshot: watching %p..%p (%u bytes)\n",
                    (void *) cl_bssLo, (void *) ( cl_bssLo + cl_bssLen ),
                    cl_bssLen );
    }
    memcpy( cl_bssCopy, cl_bssLo, cl_bssLen );
}

/* ---- CL_BssDiff  no-address ---- */
int CL_BssDiff( const char *where )
{
    unsigned int i, runStart = 0, runs = 0, bytes = 0;
    unsigned int lowest = 0xFFFFFFFFu, highest = 0;
    int          inRun = 0;

    if ( !cl_bssCopy ) {
        return 0;
    }

    for ( i = 0; i < cl_bssLen; i++ ) {
        int cleared = ( cl_bssCopy[i] != 0 && cl_bssLo[i] == 0 );

        if ( cleared && !inRun ) {
            inRun = 1;
            runStart = i;
        } else if ( !cleared && inRun ) {
            inRun = 0;
            runs++;
            bytes += i - runStart;
            if ( runs <= 120 ) {
                Com_Printf( "  cleared %p .. %p  (%u bytes)\n",
                            (void *) ( cl_bssLo + runStart ),
                            (void *) ( cl_bssLo + i ), i - runStart );
            }
            if ( (unsigned int) ( cl_bssLo + runStart ) < lowest ) {
                lowest = (unsigned int) ( cl_bssLo + runStart );
            }
            if ( (unsigned int) ( cl_bssLo + i ) > highest ) {
                highest = (unsigned int) ( cl_bssLo + i );
            }
        }
    }
    if ( inRun ) {
        runs++;
        bytes += cl_bssLen - runStart;
        if ( (unsigned int) ( cl_bssLo + runStart ) < lowest ) {
            lowest = (unsigned int) ( cl_bssLo + runStart );
        }
        highest = (unsigned int) ( cl_bssLo + cl_bssLen );
    }

    if ( runs ) {
        Com_Printf( "CL_BssDiff(%s): %u cleared runs, %u bytes, "
                    "outermost %p..%p (span %u)\n",
                    where ? where : "?", runs, bytes,
                    (void *) lowest, (void *) highest,
                    highest - lowest );

        if ( highest > lowest ) {
            unsigned int j, survivors = 0, firstSurv = 0;
            for ( j = lowest; j < highest; j++ ) {
                if ( *(unsigned char *) j ) {
                    if ( !survivors ) { firstSurv = j; }
                    survivors++;
                }
            }
            Com_Printf( "  inside that span, %u of %u bytes are STILL "
                        "non-zero (first at %p) -- %s\n",
                        survivors, highest - lowest, (void *) firstSurv,
                        survivors ? "SCATTERED STORES, not one memset"
                                  : "one contiguous clear" );
        }
    }

    memcpy( cl_bssCopy, cl_bssLo, cl_bssLen );
    return (int) runs;
}

/* ---- CL_RefApiIntegrityCheck  no-address ---- */
void CL_RefApiIntegrityCheck( const char *where )
{
    static int reported;
    int        dead;

    {
        static int  uiDiffSeq;
        static int  uiDiffHits;
        char        label[64];

        if ( uiDiffHits < 100000 ) {
            Com_sprintf( label, sizeof( label ), "UI syscall #%d", ++uiDiffSeq );
            if ( CL_BssDiff( label ) ) {
                uiDiffHits++;
            }
        }

    }

    if ( reported || !re.Shutdown ) {
        return;
    }

    dead = 0;

    /* refimport_t, 0x016D8920 */
    if ( !ri.Printf )                dead |= 0x0020;   /* +0x00 */
    if ( !ri.FS_ListFiles )          dead |= 0x0040;   /* +0x54 */
    if ( !ri.GetFontInfo )            dead |= 0x0080;   /* +0x94 */

    /* refexport_t, client side, 0x01432860 */
    if ( !re_ShutdownPtr )       dead |= 0x0100;   /* +0x00 */
    if ( !re_RegisterShaderNoMip )  dead |= 0x0200;   /* +0x14 */
    if ( !re_SaveFogState )    dead |= 0x0400;   /* +0x5C */
    if ( !re_BeginFrame )            dead |= 0x0800;   /* +0x88 */
    if ( !re_Text_PaintWithCursor )            dead |= 0x1000;   /* +0xE0 */

    if ( !dead ) {
        return;
    }

    reported = 1;
    Com_Printf( "CL_RefApiIntegrityCheck(%s): BOUNDARY TABLE SLOTS WENT NULL "
                "AFTER CL_InitRef, mask 0x%04X\n", where ? where : "?", dead );
    {
        const int *slot = (const int *) &re;
        int        i, nulls = 0;

        for ( i = 0; i < (int) ( sizeof( re ) / sizeof( int ) ); i++ ) {
            if ( !slot[i] ) {
                nulls++;
            }
        }
        Com_Printf( "  hand-written `re` at %p: %d of %d slots still bound\n",
                    (void *) &re,
                    (int) ( sizeof( re ) / sizeof( int ) ) - nulls,
                    (int) ( sizeof( re ) / sizeof( int ) ) );
    }
    Com_Printf( "  refimport          0x016D8920: +0x00 %s +0x54 %s +0x94 %s\n",
                ri.Printf ? "ok" : "NULL", ri.FS_ListFiles ? "ok" : "NULL",
                ri.GetFontInfo ? "ok" : "NULL" );
    Com_Printf( "  refexport client   0x01432860: +0x00 %s +0x14 %s +0x5C %s "
                "+0x88 %s +0xE0 %s\n",
                re_ShutdownPtr ? "ok" : "NULL",
                re_RegisterShaderNoMip ? "ok" : "NULL",
                re_SaveFogState ? "ok" : "NULL",
                re_BeginFrame ? "ok" : "NULL", re_Text_PaintWithCursor ? "ok" : "NULL" );
}

/* ---- CL_ShutdownRef  0x00411500 ----  [CONFIRMED] */
void CL_ShutdownRef( void )
{
    if ( !re.Shutdown ) {
        return;
    }

    re.Shutdown( 1 );

    memset( &re, 0, sizeof( re ) );
    StatMon_Reset();
}
