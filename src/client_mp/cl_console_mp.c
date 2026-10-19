/*
 * @fidelity: verified
 */

#include "../qcommon/qcommon.h"
#include "cl_vm.h"
#include "cl_records.h"
#include "cl_local.h"
#include "cl_conwindows.h"

/* CON_TEXTSIZE and DEFAULT_CONSOLE_WIDTH are RTCW cl_console.c:41 and :77;
 * MAX_EDIT_LINE is RTCW qcommon/qcommon.h:720.  CON_EMPTY_TEXT_CELL is the
 * coduo name (client/console.h:16) for the cell Con_Clear_f and Con_Linefeed
 * write: a space in colour 7. */
#define CON_TEXTSIZE            65536
#define DEFAULT_CONSOLE_WIDTH   78
#define MAX_EDIT_LINE           256
#define CON_EMPTY_TEXT_CELL     0x0720
#define CON_EMPTY_TEXT_PAIR     ( ( CON_EMPTY_TEXT_CELL << 16 ) | CON_EMPTY_TEXT_CELL )

/* The HUD text-draw / text-width entry points (near the cached re. table).  The
 * two Paint slots are prototyped (retail pushes their float arguments with fstp);
 * the others stay unprototyped and take float arguments as raw 4-byte bit
 * patterns -- an unprototyped float would be promoted to an 8-byte double. */
#undef  re_Text_Paint
#define re_Text_Paint ( *(void (**)( float x, float y, int font, float scale, const float *color, const char *text, float charWidth, int limit, int style ))&re.Text_Paint )   /* 0x01432934  re.Text_Paint */
#undef  re_Text_ConsolePaint
#define re_Text_ConsolePaint ( *(int (**)( float x, float y, int font, float scale, const float *color, const void *text, float charWidth, int limit, int style ))&re.Text_ConsolePaint )   /* 0x0143293C  re.Text_Paint (charset) */

/* console field default width/char metrics; defined (initialised) at file end */
extern int con_fieldWidthPixels;             /* 0x0057C0F8  620   (widthInPixels)  */
extern int con_fieldCharWidth;             /* 0x0057C0FC  8.0f  (charWidth bits)  */
extern int con_fieldCharHeight;             /* 0x0057C100  16.0f (charHeight bits) */

cvar_t *con_boldgamemessagetime;   /* 0x0140EF2C */
cvar_t *con_gamemessagetime;       /* 0x0140EF30 */
cvar_t *con_minicontime;           /* 0x0142F5E0 */
cvar_t *con_restricted;

/* These console cvars are held as int (the cvar_t * value); ->value lives at
 * +28 and ->integer at +32, reached with a raw offset (the client's hybrid
 * cvar storage model). */
int con_conspeed;              /* 0x0142F5DC  cvar_t *; ->value @ +28 */
int con_debug;                 /* 0x0140EF20  cvar_t *; ->integer @ +32 */
int con_minicon;                      /* 0x0140EF24  cvar_t * */
int con_miniconlines;          /* 0x0142F5E4  cvar_t *; ->integer @ +32 */

extern cvar_t *cl_noprint;            /* 0x0161731C */
extern int   cl_serverTime;           /* 0x01434A64 */
extern int   cl_snap_ps_pm_type;           /* 0x01432984  cl.snap.ps.pm_type (5 == intermission) */
/* keys_shiftDown (0x0142FF00) is keys[K_SHIFT].down, a cl_local.h view over keysStorage */
extern int   chat_team;           /* 0x01430380  chat is team message (RTCW chat_team) */

/* The mini-console text colour: one vec4 spanning 0x0142F5CC..0x0142F5DB.  IDA
 * names its dwords con_miniconColor/_1_/_2_/_3_, but they are one object:
 * Con_DrawInput and Con_DrawSolidConsole pass the whole vec4 to re_SetColor,
 * so all four components must live in con_miniconColor itself. */
char con_miniconColor[16];     /* 0x0142F5CC  vec4 */

/* the per-window time/index scratch arrays the message-window records point at */
unsigned char con_gameMessageStartTimes[96];   /* 0x0142EF6C */
unsigned char con_gameMessageEndTimes[64];     /* 0x0142EF8C */
unsigned char con_gameMessageLineIndices[32];  /* 0x0142EFAC */
unsigned char con_boldMessageStartTimes[96];   /* 0x0142EFF0 */
unsigned char con_boldMessageEndTimes[64];     /* 0x0142F010 */
unsigned char con_boldMessageLineIndices[32];  /* 0x0142F030 */
unsigned char con_subtitleStartTimes[96];      /* 0x0142F074 */
unsigned char con_subtitleEndTimes[64];        /* 0x0142F094 */
unsigned char con_subtitleLineIndices[32];     /* 0x0142F0B4 */
unsigned char con_miniconStartTimes[1200];     /* 0x0142F0F8 */
unsigned char con_miniconEndTimes[800];        /* 0x0142F288 */
unsigned char con_miniconLineIndices[400];     /* 0x0142F418 */

/* the console scrollback and its cursor state (RTCW console_t con;
 * consoleCore_t is in cl_local.h) */
consoleCore_t con;                    /* 0x0140EF40..0x0142EF6C */

/* every retail caller pushes a 4th argument (1) that Field_Draw (cl_keys_mp.c) never reads */
extern void Field_Draw( void *edit, int x, int y, int showCursor );
extern int  SCR_DrawConsoleString( int x, int y, const void *text, const float *color, int maxChars );
extern void SCR_DrawPic( float x, float y, float w, float h, int hShader );
extern void SCR_FillRect( const float *color, float x, float y, float w, float h );
extern void SCR_DrawSmallChar( int ch, int x, int y );
extern void SCR_DrawSmallStringExt( const char *str, const float *color, int x, int y );
extern int  SEH_PrintStrlen( char *text );
extern int  SEH_SafeTranslateString();
void memset32( unsigned int *dest, unsigned int constant, unsigned int count );

int chat_playerNum;

/* ---- FastRound  0x00408370 ----  VERIFIED
 * Retail is fld float / fadd qword 2^-30 (0x3E10000000000000) / fistp: the
 * FPU's round-to-nearest, not the truncating __ftol2 of an (int) cast.  Same
 * inline-asm pattern as game_mp/bg_slidemove.c Q_ftol. */
int __cdecl FastRound( float f )
{
  double bias = 9.313225746154785e-10;   /* qword 0x3E10000000000000 = 2^-30 */
  int    i;

  __asm fld   f
  __asm fadd  bias
  __asm fistp i

  return i;
}

extern void Con_ToggleConsole_f( void );

/* ---- Con_MessageMode_f  0x00408440 ----  VERIFIED */
void __cdecl Con_MessageMode_f( void )
{
  chat_playerNum         = -1;
  chat_team          = 0;
  memset( chatField.buffer, 0, MAX_EDIT_LINE );
  chatField.charWidth    = 0.0f;
  chatField.cursor       = 0;
  chatField.scroll       = 0;
  chatField.widthInChars = MAX_EDIT_LINE;
  chatField.widthInPixels = 588;
  chatField.charHeight   = 16.0f;
  chatField.fixedWidth   = 0;
  cls_keyCatchers ^= KEYCATCH_MESSAGE;
}

/* ---- Con_MessageMode2_f  0x004084B0 ----  VERIFIED */
void __cdecl Con_MessageMode2_f( void )
{
  memset( chatField.buffer, 0, MAX_EDIT_LINE );
  chatField.cursor       = 0;
  chatField.scroll       = 0;
  chatField.charWidth    = 0.0f;
  chatField.fixedWidth   = 0;
  chat_playerNum         = -1;
  chat_team          = 1;
  chatField.widthInChars = MAX_EDIT_LINE;
  chatField.widthInPixels = 543;
  chatField.charHeight   = 16.0f;
  cls_keyCatchers ^= KEYCATCH_MESSAGE;
}

/* ---- Con_MessageMode3_f  0x00408520 ----  VERIFIED */
void __cdecl Con_MessageMode3_f( void )
{
  chat_playerNum = VM_Call( cgvm, 4 );
  if ( chat_playerNum < 0 || chat_playerNum >= 64 )
  {
    chat_playerNum = -1;
    return;
  }
  memset( chatField.buffer, 0, MAX_EDIT_LINE );
  chatField.charWidth    = 0.0f;
  chat_team          = 0;
  chatField.cursor       = 0;
  chatField.scroll       = 0;
  chatField.widthInChars = MAX_EDIT_LINE;
  chatField.widthInPixels = 588;
  chatField.charHeight   = 16.0f;
  chatField.fixedWidth   = 0;
  cls_keyCatchers ^= KEYCATCH_MESSAGE;
}

/* ---- Con_Clear_f  0x004085B0 ----  VERIFIED */
void __cdecl Con_Clear_f( void )
{
  unsigned int i;
  for ( i = 0; i < CON_TEXTSIZE / 2; ++i )
    ((unsigned int *)con_text)[i] = CON_EMPTY_TEXT_PAIR;
  con_displayLine = con_current;
}

/* ---- Con_Dump_f  0x004085D0 ----  VERIFIED */
void __cdecl Con_Dump_f( void )
{
  int          l;
  int          x;
  int          i;
  __int16     *line;
  fileHandle_t f;
  char         buffer[1024];

  if ( Cmd_Argc() != 2 )
  {
    Com_Printf( "usage: condump <filename>\n" );
    return;
  }

  Com_Printf( "Dumped console text to %s.\n", Cmd_Argv( 1 ) );

  f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
  if ( !f )
  {
    Com_Printf( "ERROR: couldn't open.\n" );
    return;
  }

  /* skip the leading empty lines */
  for ( l = con_current - con_totallines + 1; l <= con_current; l++ )
  {
    line = con_text + ( l % con_totallines ) * con_linewidth;
    for ( x = 0; x < con_linewidth; x++ )
    {
      if ( ( line[x] & 0xff ) != ' ' )
        break;
    }
    if ( x != con_linewidth )
      break;
  }

  /* retail 0x004086AB / 0x004086FB store with no bound check; pointer stores keep modern /GS
     from adding a range-check fast-fail (buffer[n] = 0 with n >= 1024) */
  *( buffer + con_linewidth ) = 0;
  for ( ; l <= con_current; l++ )
  {
    line = con_text + ( l % con_totallines ) * con_linewidth;
    for ( i = 0; i < con_linewidth; i++ )
      buffer[i] = (char)line[i];
    for ( x = con_linewidth - 1; x >= 0; x-- )
    {
      if ( buffer[x] == ' ' )
        *( buffer + x ) = 0;
      else
        break;
    }
    strcat( buffer, "\n" );
    FS_Write( buffer, strlen( buffer ), f );
  }

  FS_FCloseFile( f );
}

/* ---- Con_ClearMessageWindow  0x00408770 ----  VERIFIED */
void __cdecl Con_ClearMessageWindow( conMessageWindow_t *window )
{
  memset( window->times, 0, 4 * window->size );
  memset( window->endTimes, 0, 4 * window->size );
  window->index = 0;
}

/* ---- Con_ClearNotify  0x004087B0 ----  VERIFIED */
void __cdecl Con_ClearNotify( void )
{
  memset( con_notifyWindowRec.times, 0, 4 * con_notifyWindowRec.size );
  memset( con_notifyWindowRec.endTimes, 0, 4 * con_notifyWindowRec.size );
  con_notifyWindowRec.index = 0;
  memset( con_boldWindowRec.times, 0, 4 * con_boldWindowRec.size );
  memset( con_boldWindowRec.endTimes, 0, 4 * con_boldWindowRec.size );
  con_boldWindowRec.index = 0;
}

/* ---- Con_ClearMiniConsole  0x00408800 ----  VERIFIED */
void __cdecl Con_ClearMiniConsole( void )
{
  memset( con_miniconsoleWindowRec.times, 0, 4 * con_miniconsoleWindowRec.size );
  memset( con_miniconsoleWindowRec.endTimes, 0, 4 * con_miniconsoleWindowRec.size );
  con_miniconsoleWindowRec.index = 0;
}

/* ---- Con_ClearSubtitles  0x00408830 ----  VERIFIED */
void __cdecl Con_ClearSubtitles( void )
{
  memset( con_subtitleWindowRec.times, 0, 4 * con_subtitleWindowRec.size );
  memset( con_subtitleWindowRec.endTimes, 0, 4 * con_subtitleWindowRec.size );
  con_subtitleWindowRec.index = 0;
}

/* ---- Con_CheckResize  0x00408860 ----  VERIFIED */
void __cdecl Con_CheckResize( void )
{
  int     i, j, width, oldwidth, oldtotallines, numlines, numchars;
  __int16 tbuf[CON_TEXTSIZE];
  int     vidWidth;

  vidWidth = cls_glconfig_vidWidth;
  if ( vidWidth < 640 )
    vidWidth = 640;

  width = vidWidth / 8 - 2;

  if ( width == con_linewidth )
    return;

  if ( width < 1 )
  {
    con_linewidth  = DEFAULT_CONSOLE_WIDTH;
    con_totallines = CON_TEXTSIZE / DEFAULT_CONSOLE_WIDTH;
    { unsigned int clearIndex; for ( clearIndex = 0; clearIndex < CON_TEXTSIZE / 2; ++clearIndex ) ((unsigned int *)con_text)[clearIndex] = CON_EMPTY_TEXT_PAIR; }
  }
  else
  {
    oldwidth       = con_linewidth;
    con_linewidth  = width;
    oldtotallines  = con_totallines;
    con_totallines = CON_TEXTSIZE / con_linewidth;

    numlines = oldtotallines;
    if ( con_totallines < numlines )
      numlines = con_totallines;

    numchars = oldwidth;
    if ( con_linewidth < numchars )
      numchars = con_linewidth;

    memcpy( tbuf, con_text, sizeof( tbuf ) );
    { unsigned int clearIndex; for ( clearIndex = 0; clearIndex < CON_TEXTSIZE / 2; ++clearIndex ) ((unsigned int *)con_text)[clearIndex] = CON_EMPTY_TEXT_PAIR; }

    for ( i = 0; i < numlines; i++ )
    {
      for ( j = 0; j < numchars; j++ )
      {
        con_text[( con_totallines - 1 - i ) * con_linewidth + j] =
            tbuf[( ( con_current - i + oldtotallines ) % oldtotallines ) * oldwidth + j];
      }
    }

    Con_ClearNotify();
    Con_ClearMiniConsole();
    Con_ClearSubtitles();
  }

  con_current     = con_totallines - 1;
  con_displayLine = con_current;
}

/* ---- Con_InitMessageWindow  0x00408A50 ----  VERIFIED */
/* The retail function is __usercall (window@eax, x@edx, timesPtr@ecx, the rest
 * on the stack); the decompiled parameter labels are as Hex-Rays emitted them
 * and do not line up with the fields they land in, so each store is annotated
 * with the byte offset it actually writes. */
void __cdecl Con_InitMessageWindow(
        conMessageWindow_t *window,
        int                 x,
        int                *timesPtr,
        int                *endTimesPtr,
        int                *linesPtr,
        int                 size,
        int                 y,
        int                 msgtime,
        int                 extra )
{
  window->times             = timesPtr;      /* +0x00 */
  *(int *)&window->endTimes = x;             /* +0x04 */
  window->lines             = endTimesPtr;   /* +0x08 */
  window->size              = (int)linesPtr; /* +0x10 */
  window->activeLines       = size;          /* +0x14 */
  window->scrollTime        = y;             /* +0x18 */
  window->index             = 0;             /* +0x0C */
  window->fadeIn            = msgtime;       /* +0x1C */
  window->fadeOut           = extra;         /* +0x20 */
}

/* ---- Con_Init  0x00408A90 ----  VERIFIED */
void __cdecl Con_Init( void )
{
  int fieldWidthPixels;
  int i;

  con_conspeed   = (int)Cvar_Get( "scr_conspeed", "3", 0 );
  con_debug      = (int)Cvar_Get( "con_debug", "0", 1 );
  con_restricted = Cvar_Get( "con_restricted", "0", 16 );

  fieldWidthPixels = con_fieldWidthPixels;

  memset( g_consoleField.buffer, 0, MAX_EDIT_LINE );
  g_consoleField.cursor        = 0;
  g_consoleField.scroll        = 0;
  g_consoleField.widthInChars  = MAX_EDIT_LINE;
  g_consoleField.widthInPixels = con_fieldWidthPixels;
  /* the char metrics are float globals in retail, reloaded after every memset */
  g_consoleField.charWidth     = *(float *)&con_fieldCharWidth;
  g_consoleField.charHeight    = *(float *)&con_fieldCharHeight;
  g_consoleField.fixedWidth    = 1;

  for ( i = 0; i < COMMAND_HISTORY; i++ )
  {
    field_t *f = &historyEditLines[i];

    memset( f->buffer, 0, MAX_EDIT_LINE );
    f->cursor        = 0;
    f->scroll        = 0;
    f->widthInChars  = MAX_EDIT_LINE;
    f->widthInPixels = fieldWidthPixels;
    f->charWidth     = *(float *)&con_fieldCharWidth;
    f->charHeight    = *(float *)&con_fieldCharHeight;
    f->fixedWidth    = 1;
  }

  Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f );
  Cmd_AddCommand( "messagemode", Con_MessageMode_f );
  Cmd_AddCommand( "messagemode2", Con_MessageMode2_f );
  Cmd_AddCommand( "messagemode3", Con_MessageMode3_f );
  Cmd_AddCommand( "clear", Con_Clear_f );
  Cmd_AddCommand( "condump", Con_Dump_f );
}

/* ---- Con_UpdateMessageWindowLine  0x00408BE0 ----  VERIFIED */
void __cdecl Con_UpdateMessageWindowLine( conMessageWindow_t *window, int extra, int duration )
{
  int i;
  int j;

  window->times[window->index]    = cl_serverTime;
  window->endTimes[window->index] = cl_serverTime + duration;
  window->lines[window->index]    = con_current;

  if ( !extra )
    return;
  if ( window->size <= 0 )
    return;

  window->index = ( window->index + 1 ) % window->size;

  for ( i = 0; i < window->activeLines; i++ )
  {
    j = ( window->index + i ) % window->size;
    if ( window->endTimes[j] - window->fadeOut > cl_serverTime )
    {
      window->times[j]    += cl_serverTime + window->fadeOut - window->endTimes[j];
      window->endTimes[j]  = cl_serverTime + window->fadeOut;
    }
  }
}

/* ---- Con_UpdateNotifyLine  0x00408C90 ----  VERIFIED */
/* (channel, extra, duration) as on the Mac 1.3 build; Con_Linefeed is (channel, duration) */
__declspec(noinline) void __cdecl Con_UpdateNotifyLine( int channel, int extra, int duration )
{
  if ( con_current < 0 )
    return;

  switch ( channel )
  {
    case 0: Con_UpdateMessageWindowLine( &con_miniconsoleWindowRec, extra, duration ); break;
    case 1: Con_UpdateMessageWindowLine( &con_notifyWindowRec, extra, duration ); break;
    case 2: Con_UpdateMessageWindowLine( &con_boldWindowRec, extra, duration ); break;
    case 3: Con_UpdateMessageWindowLine( &con_subtitleWindowRec, extra, duration ); break;
    default: return;
  }
}

/* ---- Con_Linefeed  0x00408D00 ----  VERIFIED */
void __cdecl Con_Linefeed( int channel, int duration )
{
  int i;

  Con_UpdateNotifyLine( channel, 1, duration );

  con_display = 0;
  if ( con_displayLine == con_current )
    con_displayLine++;
  con_current++;

  for ( i = 0; i < con_linewidth; i++ )
    con_text[( con_current % con_totallines ) * con_linewidth + i] = CON_EMPTY_TEXT_CELL;
}

conMessageWindow_t con_notifyWindowRec;
conMessageWindow_t con_boldWindowRec;
conMessageWindow_t con_subtitleWindowRec;
conMessageWindow_t con_miniconsoleWindowRec;

/* ---- Con_OneTimeInit  0x00408D70 ----  VERIFIED */
void __cdecl Con_OneTimeInit( void )
{
  cvar_t *miniconLinesCvar;
  int     lines;
  char   *clamped;

  con_gamemessagetime     = Cvar_Get( "con_gamemessagetime", "5", 0 );
  con_boldgamemessagetime = Cvar_Get( "con_boldgamemessagetime", "8", 0 );
  con_minicontime         = Cvar_Get( "con_minicontime", "4", 1 );
  con_minicon             = (int)Cvar_Get( "con_minicon", "0", 1 );
  miniconLinesCvar        = Cvar_Get( "con_miniconlines", "5", 1 );

  con_notifyWindowRec.times       = (int *)con_gameMessageStartTimes;
  con_notifyWindowRec.endTimes    = (int *)con_gameMessageEndTimes;
  con_notifyWindowRec.lines       = (int *)con_gameMessageLineIndices;
  con_notifyWindowRec.index       = 0;
  con_notifyWindowRec.size        = 8;
  con_notifyWindowRec.activeLines = 3;
  con_notifyWindowRec.scrollTime  = 250;
  con_notifyWindowRec.fadeIn      = 250;
  con_notifyWindowRec.fadeOut     = 500;

  con_boldWindowRec.times       = (int *)con_boldMessageStartTimes;
  con_boldWindowRec.endTimes    = (int *)con_boldMessageEndTimes;
  con_boldWindowRec.lines       = (int *)con_boldMessageLineIndices;
  con_boldWindowRec.index       = 0;
  con_boldWindowRec.size        = 8;
  con_boldWindowRec.activeLines = 3;
  con_boldWindowRec.scrollTime  = 250;
  con_boldWindowRec.fadeIn      = 250;
  con_boldWindowRec.fadeOut     = 500;

  con_subtitleWindowRec.times       = (int *)con_subtitleStartTimes;
  con_subtitleWindowRec.endTimes    = (int *)con_subtitleEndTimes;
  con_subtitleWindowRec.lines       = (int *)con_subtitleLineIndices;
  con_subtitleWindowRec.index       = 0;
  con_subtitleWindowRec.size        = 8;
  con_subtitleWindowRec.activeLines = 3;
  con_subtitleWindowRec.scrollTime  = 250;
  con_subtitleWindowRec.fadeIn      = 250;
  con_subtitleWindowRec.fadeOut     = 500;

  lines            = miniconLinesCvar->integer;
  con_miniconlines = (int)miniconLinesCvar;
  if ( lines > 100 )
  {
    clamped = va( "%d", 100 );
    Cvar_Set2( "con_miniconlines", clamped, qtrue );
    miniconLinesCvar = (cvar_t *)con_miniconlines;
  }

  lines = miniconLinesCvar->integer;
  con_miniconsoleWindowRec.times       = (int *)con_miniconStartTimes;
  con_miniconsoleWindowRec.endTimes    = (int *)con_miniconEndTimes;
  con_miniconsoleWindowRec.lines       = (int *)con_miniconLineIndices;
  con_miniconsoleWindowRec.index       = 0;
  con_miniconsoleWindowRec.size        = lines;
  con_miniconsoleWindowRec.activeLines = 0;
  con_miniconsoleWindowRec.scrollTime  = 0;
  con_miniconsoleWindowRec.fadeIn      = 0;
  con_miniconsoleWindowRec.fadeOut     = 0;

  ((float *)con_miniconColor)[3] = 1.0f;   /* 0x0142F5D8 */
  ((float *)con_miniconColor)[2] = 1.0f;   /* 0x0142F5D4 */
  ((float *)con_miniconColor)[1] = 1.0f;   /* 0x0142F5D0 */
  ((float *)con_miniconColor)[0] = 1.0f;   /* 0x0142F5CC */

  con_linewidth = -1;
  Con_CheckResize();
  con_initialized = qtrue;
}

/* ---- CL_ConsolePrint_AddLine  0x00408F80 ----  VERIFIED */
int __cdecl CL_ConsolePrint_AddLine(
        byte *txt,
        int   channel,
        int   duration,
        int   width,
        int   color )
{
  const byte *s;
  int         l;
  int         softWidth;
  int         len;
  int         lines;
  int         wrapped;
  int         c;

  if ( width <= 0 || width > con_linewidth )
    width = con_linewidth;

  softWidth = width;

  if ( channel == 1 || channel == 2 )
  {
    len = SEH_PrintStrlen( (char *)txt );
    if ( len > width )
    {
      lines     = (int)ceil( (float)len / width );
      softWidth = (int)( (float)len / lines );
    }
  }

  if ( channel != con_prevChannel && con_display > 0 )
    Con_Linefeed( con_prevChannel, duration );

  wrapped = 0;
  s       = txt;

  while ( ( c = *s ) != 0 )
  {
    if ( *s == '^' && s[1] != 0 && s[1] != '^' && s[1] >= '0' && s[1] <= '7' )
    {
      color = s[1] & 7;
      s += 2;
      continue;
    }

    for ( l = 0; l < width; l++ )
    {
      if ( s[l] <= ' ' )
        break;
    }

    if ( l != width && con_display + l > width )
    {
      Con_Linefeed( channel, duration );
      wrapped = 1;
    }

    s++;

    switch ( c )
    {
    case '\n':
      Con_Linefeed( channel, duration );
      break;
    case '\r':
      con_display = 0;
      break;
    default:
      if ( con_display != 0 || c != ' ' || !wrapped )
      {
        con_text[( con_current % con_totallines ) * con_linewidth + con_display] =
            (__int16)( ( color << 8 ) | c );
        con_display++;
        if ( con_display >= width || ( con_display >= softWidth && c == ' ' ) )
        {
          Con_Linefeed( channel, duration );
          wrapped = 1;
        }
      }
      break;
    }
  }

  if ( con_display > 0 )
  {
    if ( channel )
      Con_Linefeed( channel, duration );
    else
      Con_UpdateNotifyLine( 0, 0, duration );
  }
  con_prevChannel = channel;

  return color;
}

/* CL_ConsolePrint (0x00409190) is defined further down this unit. */

/* CL_ConsoleFixPosition (0x004093B0) is defined further down this unit. */

/* ---- CL_AddConsoleInfoChar  0x00409450 ----  VERIFIED */
void __cdecl CL_AddConsoleInfoChar( int ch, char color )
{
  con_text[( con_current % con_totallines ) * con_linewidth + con_display] =
      (__int16)( ( color << 8 ) | ch );
  con_display++;
}

/* ---- CL_AddConsoleInfoColor  0x00409490 ----  VERIFIED */
void __cdecl CL_AddConsoleInfoColor( int base, const float *rgb )
{
  int v;
  v = (int)( rgb[0] * 255.0f );
  if ( v < 0 ) v = 0; else if ( v > 255 ) v = 255;
  con_text[(con_current % con_totallines) * con_linewidth + con_display] = (__int16)(((unsigned char)base << 8) | v);
  con_display++;
  v = (int)( rgb[1] * 255.0f );
  if ( v < 0 ) v = 0; else if ( v > 255 ) v = 255;
  con_text[(con_current % con_totallines) * con_linewidth + con_display] = (__int16)(((base + 1) << 8) | v);
  con_display++;
  v = (int)( rgb[2] * 255.0f );
  if ( v < 0 ) v = 0; else if ( v > 255 ) v = 255;
  con_text[(con_current % con_totallines) * con_linewidth + con_display] = (__int16)(((base + 2) << 8) | v);
  con_display++;
}

/* ---- CL_AddDeathMessageText  0x00409590 ----  VERIFIED */
void __cdecl CL_AddDeathMessageText( const char *txt, int color )
{
  int  row;
  int  c;
  int col;

  
  col = color;
  if ( color < 0 )
    col = 7;

  row = con_current % con_totallines;

  while ( ( c = *txt ) != 0 )
  {
    if ( *txt == '^' && txt[1] != 0 && txt[1] != '^' && txt[1] >= '0' && txt[1] <= '7' )
    {
      if ( color < 0 )
        col = txt[1] & 7;
      txt += 2;
    }
    else
    {
      txt++;
      if ( c != '\n' && c != '\r' )
      {
        con_text[row * con_linewidth + con_display] = (__int16)( ( col << 8 ) | c );
        con_display++;
      }
    }
  }
}

/* ---- CL_DeathMessagePrint  0x00409630 ----  VERIFIED */
void __cdecl CL_DeathMessagePrint(
        const char  *attacker,
        int          channel,
        const float *attackerColor,
        const char  *victim,
        const float *victimColor,
        const char  *iconName,
        float        iconWidth,
        float        iconHeight,
        const float *iconColor,
        int          duration )
{

  if ( cl_noprint && cl_noprint->integer )
    return;
  if ( channel == 4 )
    return;

  if ( con_initialized == qfalse )
    Con_OneTimeInit();

  if ( !duration )
  {
    switch ( channel )
    {
      case 0:
        duration = FastRound( con_minicontime->value * 1000.0f );
        break;
      case 1:
        duration = FastRound( con_gamemessagetime->value * 1000.0f );
        break;
      case 2:
        duration = FastRound( con_boldgamemessagetime->value * 1000.0f );
        break;
      case 3:
        duration = 5000;
        break;
      default:
        break;
    }
  }
  if ( duration < 0 )
    duration = 0;

  if ( con_display > 0 )
    Con_Linefeed( con_prevChannel, duration );

  if ( *attacker )
  {
    CL_AddConsoleInfoColor( 10, attackerColor );
    CL_AddDeathMessageText( attacker, 7 );
    CL_AddDeathMessageText( " ", 7 );
  }

  CL_AddConsoleInfoColor( 13, iconColor );

  CL_AddConsoleInfoChar( (int)( iconWidth * 32.0f ), 16 );
  CL_AddConsoleInfoChar( (int)( iconHeight * 32.0f ), 17 );

  CL_AddDeathMessageText( iconName, 18 );
  CL_AddDeathMessageText( " ", 7 );

  CL_AddConsoleInfoColor( 10, victimColor );
  CL_AddDeathMessageText( victim, 7 );

  if ( channel )
    Con_Linefeed( channel, duration );
  else
    Con_UpdateNotifyLine( 0, 0, duration );
  con_prevChannel = channel;
}

/* ---- Con_DrawInput  0x00409810 ----  VERIFIED */
void __cdecl Con_DrawInput( void )
{
  int y;

  if ( !cls_state || ( cls_keyCatchers & KEYCATCH_CONSOLE ) != 0 )
  {
    y = con_vislines - 32;
    re_SetColor( con_miniconColor );
    SCR_DrawSmallChar( ']', (int)con_xadjust, y );
    Field_Draw( &g_consoleField, (int)con_xadjust + 8, y, 1 );
  }
}

/* ---- Con_DrawStringOnHUD  0x00409870 ----  VERIFIED */
/* Retail is __usercall (len@ebx, charset@edi); the parameter labels do not
 * match their use (`len` is the encoded-text pointer, `charset` the cell count).
 * Declaration order follows the Mac 1.3 call (x, y, text, ...): it decides
 * where Con_DrawMessageWindowBottomUp evaluates the text pointer. */
int __cdecl Con_DrawStringOnHUD( int x, int y, int len, int charset, float alpha, int center )
{
  int   flags;
  float scale;
  float colorVec[4];

  flags = 0;
  colorVec[0] = 1.0f;
  colorVec[1] = 1.0f;
  colorVec[2] = 1.0f;
  scale = 0.25f;
  colorVec[3] = alpha;
  if ( center )
  {
    flags = 4;
    scale = 0.33333334f;
    x += re_Text_ConsoleWidth( len, 4, 1051372203 /* 0.33333334f */, 0, charset ) / -2;
  }
  return re_Text_ConsolePaint( (float)x, (float)( y + 12 ), flags, scale, colorVec, (const void *)len, 0, charset, 3 );
}

/* ---- Con_DrawMessageWindowBottomUp  0x00409920 ----  VERIFIED */
void __cdecl Con_DrawMessageWindowBottomUp( conMessageWindow_t *window, int charset,
                                            int yStart, float alpha, int isSubtitle )
{
  int    slot;
  int    line;
  int    idx;
  int    startTime;
  int    endTime;
  int    lineHeight;
  float  alphaOut;

  lineHeight = isSubtitle ? 16 : 12;

  /* age out expired lines and accumulate the scroll-in offset */
  for ( slot = window->index; slot < window->index + window->size; slot++ )
  {
    idx       = slot % window->size;
    startTime = window->times[idx];
    if ( !startTime )
      continue;

    if ( startTime > cl_serverTime )
      window->times[idx] = 0;
    else if ( cl_serverTime - startTime < window->scrollTime )
      yStart += FastRound( (float)( ( 1.0f - (rfloat)( cl_serverTime - startTime ) / window->scrollTime ) * lineHeight ) );
  }

  /* draw the surviving lines from the bottom up */
  for ( line = window->index + window->size - 1; line >= window->index; line-- )
  {
    idx       = line % window->size;
    startTime = window->times[idx];
    endTime   = window->endTimes[idx];
    if ( !startTime )
      continue;

    if ( cl_serverTime - endTime >= 0 )
    {
      window->times[idx] = 0;
      continue;
    }

    if ( cl_serverTime - startTime < window->fadeIn )
      alphaOut = (float)( (double)( cl_serverTime - startTime ) * alpha / window->fadeIn );
    else if ( endTime - cl_serverTime < window->fadeOut )
      alphaOut = (float)( (double)( endTime - cl_serverTime ) * alpha / window->fadeOut );
    else
      alphaOut = alpha;

    yStart -= lineHeight;
    Con_DrawStringOnHUD( charset, yStart,
                         (int)&con_text[con_linewidth * ( window->lines[idx] % con_totallines )],
                         con_linewidth, alphaOut, isSubtitle );
  }
}

/* ---- Con_DrawMessageWindow  0x00409AD0 ----  VERIFIED */
/* Retail is __usercall (yStart@eax, window@edx, orientation@ecx); the wrapper
 * callers below keep the register/stack positions, so the argument labels look
 * shuffled but the byte flow is exact. */
void __cdecl Con_DrawMessageWindow(
        int                 yStart,
        conMessageWindow_t *window,
        int                 orientation,
        int                 charset,
        float               alpha )
{
  int   upward;
  int   groupCount;
  int   slot;
  int   slotIdx;
  int   y;
  int   savedY;
  int   startTime;
  int   endTime;
  int   textPtr;
  int   scanIdx;
  float colorAlpha;

  savedY = 0;

  

  if ( cl_snap_ps_pm_type != 5 && ( cls_keyCatchers & ( 0x0002 | 0x0008 ) ) != 0 )
    return;

  switch ( orientation )
  {
    case 0:
      upward = 0;
      break;
    case 1:
      upward = 1;
      break;
    case 2:
    case 3:
      Con_DrawMessageWindowBottomUp( window, charset, yStart, alpha, orientation == 3 );
      return;
    default:
      return;
  }

  groupCount = 0;
  y = yStart;
  if ( upward )
    y = yStart - 12;

  for ( slot = window->index; slot < window->index + window->size; slot++ )
  {
    slotIdx   = slot % window->size;
    startTime = window->times[slotIdx];
    if ( !startTime )
      continue;

    if ( startTime > cl_serverTime )
    {
      
      window->times[slotIdx] = 0;
      continue;
    }

    endTime = window->endTimes[slotIdx];
    if ( cl_serverTime > endTime )
    {
      
      if ( endTime + window->scrollTime - cl_serverTime > 0 )
      {
        if ( upward )
          y -= FastRound( (rfloat)( endTime + window->scrollTime - cl_serverTime ) / window->scrollTime * 12.0f );
        else
          y += FastRound( (rfloat)( endTime + window->scrollTime - cl_serverTime ) / window->scrollTime * 12.0f );
      }
      continue;
    }

    
    colorAlpha = alpha;
    if ( cl_serverTime - startTime < window->fadeIn )
      colorAlpha = (rfloat)( cl_serverTime - startTime ) / window->fadeIn * alpha;
    else if ( endTime - cl_serverTime < window->fadeOut )
      colorAlpha = (rfloat)( endTime - cl_serverTime ) / window->fadeOut * alpha;

    textPtr = (int)&con_text[con_linewidth * ( window->lines[slotIdx] % con_totallines )];

    if ( upward )
    {
      if ( groupCount )
      {
        y = y + 24;
      }
      else
      {
        for ( scanIdx = slot + 1; scanIdx < window->index + window->size; scanIdx++ )
        {
          if ( window->times[scanIdx % window->size] != window->times[slot % window->size] )
            break;
          groupCount++;
        }
        if ( groupCount )
        {
          y = y - 12 * groupCount;
          savedY = y;
          groupCount++;
        }
      }
    }

    Con_DrawStringOnHUD( charset, y, textPtr, con_linewidth, colorAlpha, 0 );

    if ( upward )
    {
      if ( groupCount )
      {
        if ( --groupCount == 0 )
          y = savedY;
      }
      y = y - 12;
    }
    else
    {
      y = y + 12;
    }
  }
}

/* ---- Con_DrawSay  0x00409D80 ----  VERIFIED */
void __cdecl Con_DrawSay( int y )
{
  char *text;
  int   textWidth;
  float textColor[4];

  textColor[0] = 1.0f;
  textColor[1] = 1.0f;
  textColor[2] = 1.0f;
  textColor[3] = 1.0f;

  if ( ( cls_keyCatchers & KEYCATCH_MESSAGE ) != 0 )
  {
    /* retail has the va(), label paint and width query in both arms (the width call
       in each arm lets VC7 fold its argument pop into the paint call's) */
    if ( chat_team )
    {
      text = va( "%s:", (const char *)SEH_SafeTranslateString( (int *)"EXE_SAYTEAM" ) );
      re_Text_Paint( 8.0f, (float)( y + 16 ), 0, 0.33333334f, textColor, text, 0, 0, 3 );
      textWidth = re_Text_Width( text, 0, 1051372203, 0, 0 );
    }
    else
    {
      text = va( "%s:", (const char *)SEH_SafeTranslateString( (int *)"EXE_SAY" ) );
      re_Text_Paint( 8.0f, (float)( y + 16 ), 0, 0.33333334f, textColor, text, 0, 0, 3 );
      textWidth = re_Text_Width( text, 0, 1051372203, 0, 0 );
    }
    Field_Draw( &chatField, textWidth + 8, y, 1 );
  }
}

/* ---- Con_DrawNotify  0x00409E70 ----  VERIFIED */
void __cdecl Con_DrawNotify( int charset, float alpha, int orientation, int yStart )
{
  Con_DrawMessageWindow( charset, &con_notifyWindowRec, orientation, yStart, alpha );
}

/* ---- Con_DrawBoldMessages  0x00409E90 ----  VERIFIED */
void __cdecl Con_DrawBoldMessages( int charset, float alpha, int orientation, int yStart )
{
  Con_DrawMessageWindow( charset, &con_boldWindowRec, orientation, yStart, alpha );
}

/* ---- Con_DrawMiniConsole  0x00409EB0 ----  VERIFIED */
void __cdecl Con_DrawMiniConsole( int yStart, int charset, float alpha )
{
  char *clamped;

  if ( *(int *)( con_miniconlines + 32 ) > 100 )
  {
    clamped = va( "%d", 100 );
    Cvar_Set2( "con_miniconlines", clamped, qtrue );
  }
  con_miniconsoleWindowRec.size = *(int *)( con_miniconlines + 32 );
  Con_DrawMessageWindow( charset, &con_miniconsoleWindowRec, 0, yStart, alpha );
}

/* ---- Con_DrawSubtitles  0x00409F10 ----  VERIFIED */
void __cdecl Con_DrawSubtitles( int charset, float alpha, int orientation, int yStart )
{
  Con_DrawMessageWindow( charset, &con_subtitleWindowRec, orientation, yStart, alpha );
}

/* ---- Con_DrawSolidConsole  0x00409F30 ----  VERIFIED */
void __cdecl Con_DrawSolidConsole( float frac )
{
  int         i, x, y;
  int         rows;
  int         row;
  int         lines;
  vec4_t      color;
  const char *version;

  lines = (int)( (double)cls_glconfig_vidHeight * frac );
  if ( lines <= 0 )
    return;
  if ( lines > cls_glconfig_vidHeight )
    lines = cls_glconfig_vidHeight;

  con_xadjust = FCAST( cls_glconfig_vidWidth * (rfloat)0.0015625f ) * 8.0f;

  /* draw the background */
  y = (int)( (rfloat)frac * 480.0f - 2.0f );
  if ( y < 1 )
    y = 0;
  else
    SCR_DrawPic( 0, 0, 640, (float)y, cls_consoleShader );

  color[0] = 0.0f;
  color[1] = 0.0f;
  color[2] = 0.0f;
  color[3] = 0.6f;
  SCR_FillRect( color, 0, (float)y, 640, 2 );

  /* draw the version number */
  version = com_version->string;
  SCR_DrawSmallStringExt( version, g_color_table[7],
                          cls_glconfig_vidWidth - 8 * (int)strlen( version ), lines - 18 );

  /* draw the text */
  con_vislines = lines;
  rows = ( lines - 8 ) / 8;
  y    = lines - 48;

  /* draw from the bottom up */
  if ( con_displayLine != con_current )
  {
    /* draw arrows to show the buffer is backscrolled */
    re_SetColor( g_color_table[7] );
    for ( x = 0; x < con_linewidth; x += 4 )
      SCR_DrawSmallChar( '^', (int)con_xadjust + ( x + 1 ) * 8, y );
    y -= 16;
    rows--;
  }

  row = con_displayLine;
  if ( con_display == 0 )
    row--;

  for ( i = 0; i < rows; i++, y -= 16, row-- )
  {
    if ( row < 0 )
      break;
    if ( con_current - row >= con_totallines )
      continue;   /* past scrollback wrap point */

    SCR_DrawConsoleString( (int)con_xadjust, y,
                           &con_text[( row % con_totallines ) * con_linewidth],
                           g_color_table[7], con_linewidth );
  }

  /* draw the input prompt, user text, and cursor if desired */
  Con_DrawInput();

  re_SetColor( 0 );
}

/* ---- Con_DrawConsole  0x0040A1E0 ----  VERIFIED */
void __cdecl Con_DrawConsole( void )
{
  Con_CheckResize();

  if ( cls_state )
  {
    if ( cls_state == CA_ACTIVE )
    {
      if ( con_displayFrac == 0.0f )
        return;
      if ( *(int *)( con_debug + 32 ) == 2 )
      {
        Con_DrawSolidConsole( con_displayFrac + con_displayFrac );
        return;
      }
    }
  }
  else if ( ( cls_keyCatchers & ( KEYCATCH_UI | KEYCATCH_CGAME ) ) == 0 )
  {
    Con_DrawSolidConsole( 1.0 );
    return;
  }

  if ( con_displayFrac != 0.0f )
    Con_DrawSolidConsole( con_displayFrac );
}

/* ---- Con_RunConsole  0x0040A260 ----  VERIFIED
 * The slide step multiplies by the float constant flt_568E6C (0.001f,
 * 0x3A83126F), not a double 0.001. */
void __cdecl Con_RunConsole( void )
{
  con_finalFrac = 0.5f;
  if ( ( cls_keyCatchers & KEYCATCH_CONSOLE ) == 0 )
    con_finalFrac = 0.0f;

  if ( con_finalFrac < con_displayFrac )
  {
    con_displayFrac -= (rfloat)cls_realFrametime * *(float *)( con_conspeed + 28 ) * 0.001f;
    if ( con_finalFrac > con_displayFrac )
      con_displayFrac = con_finalFrac;
  }
  else if ( con_finalFrac > con_displayFrac )
  {
    con_displayFrac += (rfloat)cls_realFrametime * *(float *)( con_conspeed + 28 ) * 0.001f;
    if ( con_finalFrac < con_displayFrac )
      con_displayFrac = con_finalFrac;
  }
}

/* ---- Con_PageUp  0x0040A330 ----  VERIFIED */
void __cdecl Con_PageUp( void )
{
  con_displayLine -= 2;
  if ( con_current - con_displayLine >= con_totallines )
    con_displayLine = con_current - con_totallines + 1;
}

/* ---- Con_PageDown  0x0040A360 ----  VERIFIED */
void __cdecl Con_PageDown( void )
{
  con_displayLine += 2;
  if ( con_displayLine > con_current )
    con_displayLine = con_current;
}

/* ---- Con_Top  0x0040A380 ----  VERIFIED */
void __cdecl Con_Top( void )
{
  con_displayLine = con_totallines;
  if ( con_current - con_totallines >= con_totallines )
    con_displayLine = con_current - con_totallines + 1;
}

/* ---- Con_Bottom  0x0040A3A0 ----  VERIFIED */
void __cdecl Con_Bottom( void )
{
  con_displayLine = con_current;
}

/* ---- Con_Close  0x0040A3B0 ----  VERIFIED */
void __cdecl Con_Close( void )
{
  if ( com_cl_running->integer )
  {
    memset( g_consoleField.buffer, 0, MAX_EDIT_LINE );
    g_consoleField.cursor       = 0;
    g_consoleField.scroll       = 0;
    g_consoleField.widthInChars = MAX_EDIT_LINE;
    Con_ClearNotify();
    memset( con_miniconsoleWindowRec.times, 0, 4 * con_miniconsoleWindowRec.size );
    memset( con_miniconsoleWindowRec.endTimes, 0, 4 * con_miniconsoleWindowRec.size );
    con_miniconsoleWindowRec.index = 0;
    memset( con_subtitleWindowRec.times, 0, 4 * con_subtitleWindowRec.size );
    memset( con_subtitleWindowRec.endTimes, 0, 4 * con_subtitleWindowRec.size );
    con_subtitleWindowRec.index = 0;
    cls_keyCatchers &= ~KEYCATCH_CONSOLE;
    con_finalFrac   = 0.0f;
    con_displayFrac = 0.0f;
  }
}

int con_fieldWidthPixels = 620;   /* 0x0057C0F8 */
int con_fieldCharWidth = 0x41000000;   /* 8.0f */   /* 0x0057C0FC */
int con_fieldCharHeight = 0x41800000;   /* 16.0f */   /* 0x0057C100 */

/* ---- Con_ToggleConsole_f  0x004083A0 ----  VERIFIED */
void Con_ToggleConsole_f( void )
{
  if ( cls_state == 0 && cls_keyCatchers == KEYCATCH_CONSOLE )
  {
    Cbuf_AddText( "d1\n" );
    cls_keyCatchers = 0;
    return;
  }

  if ( con_restricted->integer && !keys_shiftDown
       && !( cls_keyCatchers & KEYCATCH_CONSOLE ) )
  {
    return;
  }

  memset( g_consoleField.buffer, 0, MAX_EDIT_LINE );
  g_consoleField.cursor        = 0;
  g_consoleField.scroll        = 0;
  g_consoleField.widthInPixels = con_fieldWidthPixels;
  g_consoleField.widthInChars  = MAX_EDIT_LINE;
  *(int *)&g_consoleField.charWidth  = con_fieldCharWidth;
  *(int *)&g_consoleField.charHeight = con_fieldCharHeight;
  g_consoleField.fixedWidth    = 1;

  cls_keyCatchers ^= KEYCATCH_CONSOLE;
}

/* ---- CL_ConsolePrint  0x00409190 ----  VERIFIED */
void CL_ConsolePrint( int channel, const char *txt, int msgtime, int width )
{
  char        line[0x1000];
  const char *nl;
  int         defaultColor;
  int         len;

  if ( cl_noprint && cl_noprint->integer )
    return;
  if ( channel == CON_CHANNEL_DROP )
    return;
  if ( !con_initialized )
    Con_OneTimeInit();

  if ( !msgtime )
  {
    switch ( channel )
    {
      case 0:
        msgtime = FastRound( con_minicontime->value * 1000.0f );
        break;
      case 1:
        msgtime = FastRound( con_gamemessagetime->value * 1000.0f );
        break;
      case 2:
        msgtime = FastRound( con_boldgamemessagetime->value * 1000.0f );
        break;
      case 3:
        msgtime = 5000;
        break;
    }
  }
  if ( msgtime < 0 )
    msgtime = 0;

  defaultColor = 7;

  if ( channel != 1 && channel != 2 )
  {
    CL_ConsolePrint_AddLine( (byte *)txt, channel, msgtime, width, 7 );
    return;
  }

  if ( strstr( txt, "\n" ) )
  {
    nl = strstr( txt, "\n" );
    do
    {
      if ( *txt == '\n' )
      {
        Con_Linefeed( channel, msgtime );
        txt++;
      }
      else
      {
        len = (int)( nl - txt ) + 1;
        if ( (unsigned int)len >= sizeof( line ) )
        {
          Com_Printf( "Text line too long. Clipping to fit\n" );
          len = sizeof( line );
        }
        strncpy( line, txt, len - 1 );
        /* retail 0x004092F7 terminates with no bound check; pointer store avoids the /GS range-check fast-fail */
        *( line + ( len - 1 ) ) = 0;

        defaultColor = CL_ConsolePrint_AddLine( (byte *)line, channel, msgtime, width, defaultColor );
        txt = nl;
        if ( *nl == '\n' && channel )
          txt = nl + 1;
      }

      if ( !strstr( txt, "\n" ) )
      {
        CL_ConsolePrint_AddLine( (byte *)txt, channel, msgtime, width, defaultColor );
        return;
      }
      if ( !txt || !*txt )
        return;
      nl = strstr( txt, "\n" );
    }
    while ( nl );
    return;
  }

  CL_ConsolePrint_AddLine( (byte *)txt, channel, msgtime, width, 7 );
}

/* ---- CL_ConsoleFixPosition  0x004093B0 ----  VERIFIED */
/* CL_ConsolePrint( 0, "\n", 0, 0 ) then con.display = con.current - 1, so an
 * ERR_DROP message is not left scrolled off (Com_ErrorCleanup, !dedicated).
 * LTCG inlines the print and folds it for channel 0; the /GS cookie is the
 * inlined line[0x1000] the folding removed.  Takes no real argument. */
void CL_ConsoleFixPosition( void )
{


  CL_ConsolePrint( 0, "\n", 0, 0 );
  con_displayLine = con_current - 1;
}
