/*
 * @fidelity: likely
 *
 * cl_scrn_mp.c -- screen-refresh master: 640x480-virtual pic/rect drawing,
 * small-font and console text, the debug graph, the cubemap-shot command and
 * the top-level SCR_UpdateScreen / SCR_DrawScreenField frame driver.  Mirrors
 * RTCW's cl_scrn.c, with CoD1's extra connection/loading/logo screen states.
 *
 * Rendering goes through the client-side refexport function-pointer table at
 * 0x01432860 (re_DrawStretchPic / re_SetColor / re_BeginFrame, plus the slots
 * this tree still spells with their raw addresses: re_Text_Paint = Text_Paint,
 * re_Text_ConsolePaint = Text_ConsolePaint, re_EndFrame = EndFrame,
 * re_CubemapShot = CubemapShot, re_CubemapWaterShot = CubemapWaterShot).  Those
 * names are views onto the slots of the client `re` (cl_refapi.h), which
 * CL_InitRef fills.  The two text-paint slots are
 * re-typed here with float prototypes so coordinates pass as plain floats;
 * that is ABI-identical to the retail LODWORD int pushes, because RE_Text_Paint
 * reads them as 4-byte bit patterns anyway.  The two cubemap slots are likewise
 * re-typed with the renderer's 5-argument prototypes (RE_CubemapShot 0x004B17F0,
 * RE_CubemapWaterShot 0x004B1E60) so n0/n1 pass as 4-byte floats, not the
 * doubles an unprototyped call would push.  EndFrame keeps three parameters
 * only while the tree's RE_EndFrame (tr_cmds.c) carries its tree-only leading
 * a1; retail pushes RTCW's two msec pointers.
 */

#include <stdlib.h>
#include <string.h>

#include "../qcommon/qcommon.h"
#include "cl_records.h"
#include "cl_local.h"

/* -------------------------------------------------------------------------- *
 *  Client refexport table (0x01432860): typed views onto the slots of `re`
 *  (cl_refapi.h), which CL_InitRef fills.
 * -------------------------------------------------------------------------- */
#undef  re_RegisterShader
#define re_RegisterShader ( *(int (**)( const char *name, int type ))&re.RegisterShader )   /* +0x10  0x01432870 */
#undef  re_SetColor
#define re_SetColor ( *(void (**)( const float *rgba ))&re.SetColor )   /* +0x6C  0x014328CC */
#undef  re_DrawStretchPic
#define re_DrawStretchPic ( *(void (**)( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader ))&re.StretchPic )   /* +0x70  0x014328D0 */
#undef  re_BeginFrame
#define re_BeginFrame ( *(void (**)( int stereoFrame ))&re.BeginFrame )   /* +0x88  0x014328E8 */
#undef  re_EndFrame
#define re_EndFrame ( *(void (**)( int *frontEndMsec, int *backEndMsec ))&re.EndFrame )   /* +0x8C  re.EndFrame */
#undef  re_CubemapShot
#define re_CubemapShot ( *(int (**)( const char *fileName, int size, int faceIndex, float n0, float n1 ))&re.CubemapShot )   /* +0xB8  re.CubemapShot */
#undef  re_CubemapWaterShot
#define re_CubemapWaterShot ( *(int (**)( const char *fileName, int size, int faceIndex, float *colorLow, float *colorHigh ))&re.CubemapWaterShot )   /* +0xBC  re.CubemapWaterShot */
#undef  re_Text_Paint
#define re_Text_Paint ( *(void (**)( float x, float y, int font, float scale, const float *color, const char *text, float charWidth, int limit, int style ))&re.Text_Paint )   /* +0xD4  re.Text_Paint */
#undef  re_Text_ConsolePaint
#define re_Text_ConsolePaint ( *(int (**)( float x, float y, int font, float scale, const float *color, const void *text, float charWidth, int limit, int style ))&re.Text_ConsolePaint )   /* +0xDC  re.Text_ConsolePaint */

/* -------------------------------------------------------------------------- *
 *  cls shaders (standalone globals); the cls.glconfig fields come from cl_local.h.
 * -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *
 *  clientStatic_t fields (standalone globals).
 * -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *
 *  clientConnection_t demo fields.
 * -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- *
 *  Miscellaneous engine globals.
 * -------------------------------------------------------------------------- */
int  scr_initialized;             /* 0x01617478 */
int         scr_updatingScreen;          /* 0x01407440 */
extern int  com_skelInvalidated2;        /* 0x0140743C */
extern int  cl_serverTime;               /* 0x01434A64 */
extern int  net_iProfilingOn;            /* 0x014073EC */
extern int  hunk_low_permanent;          /* 0x008931C4  hunk_lowUsed (com_memory_core.c; hunk_temp_permanent only points at it) */
extern int  hunk_low_temp;               /* 0x008931C8  hunk_lowTemp */

/* -------------------------------------------------------------------------- *
 *  Command tokens + cubemap face suffixes.
 * -------------------------------------------------------------------------- */
extern int         cmd_argc;             /* 0x008930F0 */
extern char       *cmd_argv[];           /* 0x00890BF0 */
extern const char *cl_cubemapFaceSuffixes[6];        /* 0x0057B4B4, cl_main_mp.c */

/* SCR_DebugGraph sample ring (RTCW cl_scrn.c). */
typedef struct {
	float   value;
	int     color;
} graphsamp_t;

static int         current;             /* 0x0087C7D0 */
static graphsamp_t values[1024];        /* 0x0087A7D0 */

/* -------------------------------------------------------------------------- *
 *  Cvars.
 * -------------------------------------------------------------------------- */
cvar_t *cl_timegraph;
cvar_t *cl_debuggraph;
cvar_t *cl_graphheight;
cvar_t *cl_graphscale;
cvar_t *cl_graphshift;
extern cvar_t *cl_debugMove;
extern cvar_t *net_showprofile;          /* 0x0165155C  qcommon/net_chan_mp.c */

/* -------------------------------------------------------------------------- *
 *  Helpers in other translation units.
 * -------------------------------------------------------------------------- */
extern void Sys_LoadingKeepAlive( void );                              /* 0x00463640 */
extern void MSS_StopSounds( int flags );                               /* 0x0044FA40 */
extern void CL_CGameRendering( int stereoFrame );                      /* 0x00404BC0  cl_cgame_mp.c */
extern void CL_DrawLogo( void );                                       /* 0x00411B00  cl_main_mp.c */
extern void SCR_DrawCinematic( void );                                 /* 0x00408240  cl_cin_mp.c */
extern void Con_DrawConsole( void );                                   /* 0x0040A1E0  cl_console_mp.c */
extern void CL_Netchan_PrintProfileStats( qboolean bPrintToConsole );  /* 0x00414920 */
extern void SV_Netchan_PrintProfileStats( qboolean bPrintToConsole );  /* 0x0045B800 */
extern int  Hunk_ClearTempMemory();                                    /* 0x00432450 */
extern int  j__atol();                                                 /* 0x00525445  atol thunk */
#ifdef COD1_VC7
#define j__atol atoi	/* retail calls libcmt's atoi (0x00525445, a jmp to _atol) */
#endif

/* -------------------------------------------------------------------------- *
 *  connstate_t, stereoFrame_t and the UI trap numbers used below.
 * -------------------------------------------------------------------------- */
#define CA_DISCONNECTED     0
#define CA_CONNECTING       1
#define CA_CHALLENGING      2
#define CA_CONNECTED        3
#define CA_LOADING          4
#define CA_PRIMED           5
#define CA_ACTIVE           6
#define CA_CINEMATIC        7
#define CA_LOGO             8

#define STEREO_CENTER       0
#define STEREO_LEFT         1
#define STEREO_RIGHT        2

#define KEYCATCH_UI         0x0002

#define UI_REFRESH              5
#define UI_IS_FULLSCREEN        6
#define UI_SET_ACTIVE_MENU      7
#define UI_DRAW_CONNECT_SCREEN  12

#define UIMENU_MAIN         1

void __cdecl SCR_AdjustFrom640( float *x, float *y, float *w, float *h );

/* ---- SCR_DrawNamedPic  0x004167C0 ----  VERIFIED */
void __cdecl SCR_DrawNamedPic( const char *picname, int type,
                               float x, float y, float w, float h )
{
    int hShader;

    hShader = re_RegisterShader( picname, type );
    SCR_AdjustFrom640( &x, &y, &w, &h );
    re_DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, hShader );
}

/* ---- SCR_AdjustFrom640  0x00416810 ----  VERIFIED
 * Scale a 640x480-virtual rectangle to the physical resolution.
 * Retail multiplies by the float constants flt_568FE4 (0x3ACCCCCD, ~1/640)
 * and flt_568FE0 (0x3B088889, ~1/480).  xscale is never stored: it stays in
 * st(0) at double precision for both *x and *w (rfloat), while yscale is
 * spilled to a 4-byte stack slot (fstp [esp+var_4]) and so is rounded to
 * float.  The (rfloat) constant casts keep the modern build's products double. */
void __cdecl SCR_AdjustFrom640( float *x, float *y, float *w, float *h )
{
    rfloat xscale;
    float  yscale;

    xscale = cls_glconfig_vidWidth * (rfloat)0.0015625f;                  /* flt_568FE4, kept in st(0) */
    yscale = cls_glconfig_vidHeight * (rfloat)0.0020833334f;              /* flt_568FE0, fstp [esp+var_4] */

    if ( x )
        *x = xscale * *x;
    if ( y )
        *y = yscale * *y;
    if ( w )
        *w = xscale * *w;
    if ( h )
        *h = yscale * *h;
}

/* ---- SCR_FillRect  0x00416870 ----  VERIFIED */
void __cdecl SCR_FillRect( const float *color, float x, float y, float w, float h )
{
    re_SetColor( color );
    SCR_AdjustFrom640( &x, &y, &w, &h );
    re_DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 0.0f, 0.0f, whiteShader );
    re_SetColor( NULL );
}

/* ---- SCR_DrawPic  0x004168D0 ----  VERIFIED */
void __cdecl SCR_DrawPic( float x, float y, float w, float h, int hShader )
{
    SCR_AdjustFrom640( &x, &y, &w, &h );
    re_DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, hShader );
}

/* ---- SCR_AdjustTo640  0x00416920 ----  VERIFIED
 * Inverse of SCR_AdjustFrom640: physical back to 640x480-virtual.  The cast
 * around 640.0f / vidWidth stops VC7 reassociating it into *x / vidWidth * 640. */
void __cdecl SCR_AdjustTo640( float *x, float *y, float *w, float *h )
{
    if ( x )
        *x = FCAST( (rfloat)640.0f / cls_glconfig_vidWidth ) * *x;
    if ( y )
        *y = FCAST( (rfloat)480.0f / cls_glconfig_vidHeight ) * *y;
    if ( h )
        *h = FCAST( (rfloat)480.0f / cls_glconfig_vidHeight ) * *h;
    if ( w )
        *w = FCAST( (rfloat)640.0f / cls_glconfig_vidWidth ) * *w;
}

/* ---- SCR_DrawSmallChar  0x00416980 ----  VERIFIED
 * Small chars are drawn at native resolution; the color is a fixed white.
 * Retail keeps 640.0f / vidWidth and 480.0f / vidHeight on the x87 stack at
 * double precision (no fstp to a float slot) and derives every output from
 * them; the 1/3 multiplier is the float constant flt_568FE8 (0x3EAAAAAB).
 * The scales are written inline, not as locals: reading the globals pins the
 * four products before the va() call, as in retail. */
void __cdecl SCR_DrawSmallChar( int ch, int x, int y )
{
    float  color[4];
    float  sx, sy, scale, charWidth;

    color[0] = 1.0f;
    color[1] = 1.0f;
    color[2] = 1.0f;
    color[3] = 1.0f;

    sx        = x * ( (rfloat)640.0f / cls_glconfig_vidWidth );
    sy        = ( (rfloat)y + 16.0f ) * ( (rfloat)480.0f / cls_glconfig_vidHeight );
    scale     = (rfloat)480.0f / cls_glconfig_vidHeight * 0.33333334f;
    charWidth = (rfloat)640.0f / cls_glconfig_vidWidth * 8.0f;

    re_Text_Paint( sx, sy, 5, scale, color, va( "%c", ch ), charWidth, 0, 0 );
}

/* ---- SCR_DrawSmallStringExt  0x00416A30 ----  VERIFIED
 * xscale/yscale stay on the x87 stack at double precision, as in
 * SCR_DrawSmallChar; the 1/3 multiplier is float flt_568FE8. */
void __cdecl SCR_DrawSmallStringExt( const char *str, const float *color, int x, int y )
{
    rfloat xscale, yscale;
    float  sx, sy, scale, charWidth;

    xscale    = (rfloat)640.0f / cls_glconfig_vidWidth;
    yscale    = (rfloat)480.0f / cls_glconfig_vidHeight;

    charWidth = xscale * 8.0f;
    scale     = 0.33333334f * yscale;
    sy        = ( (rfloat)y + 16.0f ) * yscale;
    sx        = x * xscale;

    re_Text_Paint( sx, sy, 5, scale, color, str, charWidth, 0, 0 );
}

/* ---- SCR_DrawConsoleString  0x00416AA0 ----  VERIFIED
 * xscale/yscale stay on the x87 stack at double precision, as in
 * SCR_DrawSmallChar; the 1/3 multiplier is float flt_568FE8.  (x, y, text, ...)
 * as in the Mac 1.3 build: the order decides where Con_DrawSolidConsole
 * evaluates the text pointer. */
int __cdecl SCR_DrawConsoleString( int x, int y, const void *text, const float *color, int maxChars )
{
    rfloat xscale, yscale;
    float  sx, sy, scale, charWidth;

    xscale    = (rfloat)640.0f / cls_glconfig_vidWidth;
    yscale    = (rfloat)480.0f / cls_glconfig_vidHeight;

    charWidth = xscale * 8.0f;
    scale     = 0.33333334f * yscale;
    sy        = ( (rfloat)y + 16.0f ) * yscale;
    sx        = x * xscale;

    return re_Text_ConsolePaint( sx, sy, 5, scale, color, text, charWidth, maxChars, 0 );
}

/* ---- SCR_DrawDemoRecording  0x00416B10 ----  VERIFIED */
void __cdecl SCR_DrawDemoRecording( void )
{
    char string[1024];
    int  pos;

    if ( !clc_demorecording )
        return;

    pos = FS_FTell( clc_demofile );
    sprintf( string, "RECORDING %s: %ik", clc_demoName, pos / 1024 );

    SCR_DrawSmallStringExt( string, g_color_table[7], 5, 470 );
}

/* ---- SCR_DebugGraph  0x00416BC0 ----  VERIFIED */
void __cdecl SCR_DebugGraph( float value, int color )
{
    values[current & 1023].value = value;
    values[current & 1023].color = color;
    current++;
}

/* ---- SCR_DrawDebugGraph  0x00416BF0 ----  VERIFIED
 * The scaled/shifted sample v stays in st(0) at double precision from the
 * fld/fimul/fiadd through the negative wrap to the final __ftol2; it is
 * never rounded to float. */
void __cdecl SCR_DrawDebugGraph( void )
{
    int    a, x, y, w, h;
    rfloat v;
    float  fh;

    w = cls_glconfig_vidWidth;
    x = 0;
    y = cls_glconfig_vidHeight;

    re_SetColor( g_color_table[0] );
    fh = (float)cl_graphheight->integer;
    re_DrawStretchPic( x, (rfloat)y - fh, w, fh, 0.0f, 0.0f, 0.0f, 0.0f, whiteShader );
    re_SetColor( NULL );

    for ( a = 0; a < w; a++ )
    {
        v = values[( current - a - 1 ) & 1023].value;
        v = v * cl_graphscale->integer + cl_graphshift->integer;

        if ( v < 0 )
            v += cl_graphheight->integer * ( 1 - (int)( v / cl_graphheight->integer ) );
        h = (int)v % cl_graphheight->integer;

        re_DrawStretchPic( x + w - 1 - a, y - h, 1.0f, h, 0.0f, 0.0f, 0.0f, 0.0f, whiteShader );
    }
}

/* ---- CL_CubemapShotUsage  0x004170B0 ----  VERIFIED */
void __cdecl CL_CubemapShotUsage( void )
{
    Com_Printf( "Syntax: cubemapShot size basefilename [water r0 g0 b0 r90 g90 b90 | fresnel n0 n1]\n" );
    Com_Printf( "size must be a power of 2 >= 4 and <= 1024\n" );
    Com_Printf( "screenshots will be written to 'env/basefilename_*.tga'\n" );
    Com_Printf( "basefilename must not exceed %i chars\n", 40 );
    Com_Printf( "If 'water' is specified, a diffuse water color cubemap is generated using local lighting.\n" );
    Com_Printf( "The water has the given colors at the given angles, and blends between them in the middle.\n" );
    Com_Printf( "If 'fresnel' is specified, the alpha channel of the cubemap contains the reflection factor.\n" );
    Com_Printf( "n0 and n1 are the index of refraction of the 'air' and 'water' surfaces, respectively.\n" );
    Com_Printf( "The index of refraction must always be 1 or greater.\n" );
    Com_Printf( "This is always calculated, and defaults to air-water interface (n0 = 1, n1 = 1.333).\n" );
}

/* ---- CL_CubeMapShot_f  0x00417120 ----  [HIGH]
 * cubemapShot console command: render the six axis views to env/<base>_<face>.tga.
 * The "water" branch calls CubemapWaterShot( file, size, face, colorLow, colorHigh )
 * per face, colorLow = args 4..6 and colorHigh = args 7..9 (0x00417296..0x004172BE);
 * the plain/"fresnel" path re-renders the scene each face and calls
 * CubemapShot( file, size, face, n0, n1 ) (0x004173BF..0x004173D7).  n0/n1 are
 * seeded with the air-water defaults 1.0f / 1.333f (0x3F800000 / 0x3FAA9FBE at
 * 0x00417138 / 0x00417140) and only overwritten in the "fresnel" case, whose
 * range check is an ordered `<' (test ah,5 / jnp: a NaN index passes).  Face
 * numbers run 1..6 on both paths.  Each failed check prints the usage and
 * returns on its own: one shared `goto usage' label lets VC7 sink the edi save
 * past the !cgvm return, where retail pushes it at entry. */
void __cdecl CL_CubeMapShot_f( void )
{
    float        n1, n0;                   /* this order puts n0 at retail's var_64 */
    float        colorHigh[3];             /* water args 7..9 */
    float        colorLow[3];              /* water args 4..6 */
    char         basefilename[64];
    int          size;
    int          i;

    n0 = 1.0f;                             /* 0x00417138 mov [var_64], 3F800000h */
    n1 = 1.333f;                           /* 0x00417140 mov [var_60], 3FAA9FBEh */

    if ( !cgvm )
    {
        Com_Printf( "must be in a map to use this command\n" );
        return;
    }

    if ( Cmd_Argc() < 3 || strlen( Cmd_Argv( 2 ) ) > 40 )
    {
        CL_CubemapShotUsage();
        return;
    }
    strcpy( basefilename, Cmd_Argv( 2 ) );

    size = j__atol( Cmd_Argv( 1 ) );
    if ( size < 4 || size > 1024 || ( size & ( size - 1 ) ) )
    {
        CL_CubemapShotUsage();
        return;
    }

    if ( Cmd_Argc() == 10 )
    {
        if ( _stricmp( Cmd_Argv( 3 ), "water" ) )
        {
            CL_CubemapShotUsage();
            return;
        }

        colorLow[0]  = atof( Cmd_Argv( 4 ) );
        colorLow[1]  = atof( Cmd_Argv( 5 ) );
        colorLow[2]  = atof( Cmd_Argv( 6 ) );
        colorHigh[0] = atof( Cmd_Argv( 7 ) );
        colorHigh[1] = atof( Cmd_Argv( 8 ) );
        colorHigh[2] = atof( Cmd_Argv( 9 ) );

        for ( i = 1; i <= 6; i++ )
            re_CubemapWaterShot( va( "env/%s%s.tga", basefilename, cl_cubemapFaceSuffixes[i - 1] ),
                                 size, i, colorLow, colorHigh );
        return;
    }

    if ( Cmd_Argc() == 6 )
    {
        if ( _stricmp( Cmd_Argv( 3 ), "fresnel" ) )
        {
            CL_CubemapShotUsage();
            return;
        }

        n0 = atof( Cmd_Argv( 4 ) );
        n1 = atof( Cmd_Argv( 5 ) );
        if ( n0 < 1.0f || n1 < 1.0f )
        {
            CL_CubemapShotUsage();
            return;
        }
    }
    else if ( Cmd_Argc() != 3 )
    {
        CL_CubemapShotUsage();
        return;
    }

    if ( !++com_skelTimeStamp )
        com_skelTimeStamp = 1;
    com_skelInvalidated2 = 1;

    for ( i = 1; i <= 6; i++ )
    {
        re_BeginFrame( 0 );
        VM_Call( cgvm, 3, cl_serverTime, 0, clc_demoplaying, i, size );
        re_EndFrame( NULL, NULL );
        re_CubemapShot( va( "env/%s%s.tga", basefilename, cl_cubemapFaceSuffixes[i - 1] ),
                        size, i, n0, n1 );
    }

    com_skelInvalidated2 = 0;
    Hunk_ClearTempMemory();
}

/* ---- SCR_Init  0x00416D40 ----  VERIFIED */
void SCR_Init( void )
{
    cl_timegraph   = Cvar_Get( "timegraph",   "0",  CVAR_CHEAT );
    cl_debuggraph  = Cvar_Get( "debuggraph",  "0",  CVAR_CHEAT );
    cl_graphheight = Cvar_Get( "graphheight", "32", CVAR_CHEAT );
    cl_graphscale  = Cvar_Get( "graphscale",  "1",  CVAR_CHEAT );
    cl_graphshift  = Cvar_Get( "graphshift",  "0",  CVAR_CHEAT );

    scr_initialized = qtrue;
}

/* ---- SCR_DrawScreenField  0x00416DD0 ----  VERIFIED
 * Drawn once per eye; CoD1 adds the CA_LOGO and cinematic branches and the net
 * profile overlay to RTCW's connect/loading/active dispatch. */
static void SCR_DrawScreenField( int stereoFrame )
{
    re_BeginFrame( stereoFrame );

    if ( cls_state != CA_ACTIVE )
    {
        if ( cls_glconfig_vidWidth * 480 > cls_glconfig_vidHeight * 640 )
        {
            re_SetColor( g_color_table[0] );
            re_DrawStretchPic( 0.0f, 0.0f, cls_glconfig_vidWidth, cls_glconfig_vidHeight,
                               0.0f, 0.0f, 0.0f, 0.0f, whiteShader );
            re_SetColor( NULL );
        }
    }

    if ( !uivm )
    {
        Com_DPrintf( "draw screen without UI loaded\n" );
        return;
    }

    if ( !VM_Call( uivm, UI_IS_FULLSCREEN ) )
    {
        switch ( cls_state )
        {

        case CA_CINEMATIC:
            SCR_DrawCinematic();
            break;

        case CA_LOGO:
            CL_DrawLogo();
            if ( cls_state != CA_LOGO )
                return;
            break;

        case CA_DISCONNECTED:
            MSS_StopSounds( 0 );
            VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
            break;

        case CA_CONNECTING:
        case CA_CHALLENGING:
        case CA_CONNECTED:
            VM_Call( uivm, UI_REFRESH, cls_realtime );
            VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qfalse );
            break;

        case CA_LOADING:
        case CA_PRIMED:
            CL_CGameRendering( stereoFrame );
            VM_Call( uivm, UI_REFRESH, cls_realtime );
            VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qtrue );
            break;

        case CA_ACTIVE:
            CL_CGameRendering( stereoFrame );
            SCR_DrawDemoRecording();
            break;

        default:
            /* the literal at 0x005659B4 carries a leading 0x15, CoD's do-not-localise marker, exactly as the UI_ERROR trap's format string does */
            Com_Error( ERR_FATAL, "\x15SCR_DrawScreenField: bad cls.state" );
            break;
        }
    }

    if ( ( cls_keyCatchers & KEYCATCH_UI ) && uivm )
        VM_Call( uivm, UI_REFRESH, cls_realtime );

    Con_DrawConsole();

    /* no NULL checks, as in retail: CL_InitInput and SCR_Init register these before scr_initialized */
    if ( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer )
        SCR_DrawDebugGraph();

    if ( net_showprofile->integer && net_iProfilingOn )
    {
        if ( net_iProfilingOn == 1 )
            CL_Netchan_PrintProfileStats( qfalse );
        else
            SV_Netchan_PrintProfileStats( qfalse );
    }
}

/* ---- SCR_UpdateScreen  0x00416FE0 ----  VERIFIED
 * The IDB types this int(int,char*,int); it is void(void).  The saved esi that
 * IDA models as a `buf' parameter is just a callee-saved register carrying
 * SCR_DrawScreenField's stereoFrame.  Bump com_skelTimeStamp / -Invalidated2
 * across the draw when active, draw once or twice for stereo, EndFrame, then
 * roll the temp hunk back to the permanent mark (hunk_lowTemp = hunk_lowUsed).
 * Retail has no "refexport bound" guard: 0x00416FFB goes straight from
 * scr_initialized to scr_updatingScreen. */
void SCR_UpdateScreen( void )
{
    if ( cls_state == CA_LOADING )
        Sys_LoadingKeepAlive();

    if ( !scr_initialized )
        return;

    if ( scr_updatingScreen )
        return;
    scr_updatingScreen = qtrue;

    if ( cls_state == CA_ACTIVE )
    {
        if ( ++com_skelTimeStamp == 0 )
            com_skelTimeStamp = 1;
        com_skelInvalidated2 = qtrue;
    }

    if ( cls_glconfig_stereoEnabled )
    {
        SCR_DrawScreenField( STEREO_LEFT );
        SCR_DrawScreenField( STEREO_RIGHT );
    }
    else
    {
        SCR_DrawScreenField( STEREO_CENTER );
    }

    if ( com_speeds->integer )
        re_EndFrame( &time_backend, &time_game );
    else
        re_EndFrame( NULL, NULL );

    if ( cls_state == CA_ACTIVE )
    {
        com_skelInvalidated2 = qfalse;
        if ( s_hunkData )
            hunk_low_temp = hunk_low_permanent;   /* Hunk_ClearTempMemory's body, inline in retail */
    }

    scr_updatingScreen = qfalse;
}
