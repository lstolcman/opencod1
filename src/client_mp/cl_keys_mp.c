/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "cl_vm.h"
#include "cl_records.h"
#include "cl_local.h"

/* field completion state (RTCW cl_keys.c keeps these static) */
char shortestMatch[9420];           /* 0x0087A308 */
int anykeydown;                     /* 0x0142F75C */
int matchCount;
char *completionString;

/* RTCW cl_keys.c */
field_t         historyEditLines[32];       /* 0x014304C0, 9088 == 32 * 284 */
int nextHistoryLine;
int historyLine;
field_t         g_consoleField;             /* 0x0142F640 */
field_t         chatField;                  /* 0x014303A0 */
int chat_team;   /* 0x01430380 */
int key_overstrikeMode;   /* 0x0142F628 */
int keysStorage[768];   /* 0x0142F780, keys[256] (qkey_t) */

/* MAX_EDIT_LINE is RTCW qcommon/qcommon.h:720; it is the size of field_t.buffer
 * (256) and the console/chat edit line length. */
#define MAX_EDIT_LINE           256

extern void CL_AddReliableCommand( const char *cmd );
extern int CL_Disconnect_f();
/* cl_input_mp.c, 0x0040C4E0 */
extern void CL_ClearKeys( void );
extern void Cmd_TokenizeString2( const char *text, int max_tokens );
extern int Con_PageDown();
extern int Con_PageUp();
extern void Con_ToggleConsole_f( void );   /* cl_console_mp.c 0x004083A0 */
extern int Con_Top();
extern void Con_Bottom( void );   /* cl_console_mp.c 0x0040A3A0 */
extern int MSS_StopSounds();
extern int SCR_UpdateScreen();
extern int SEH_PrintStrlen();
char *Sys_GetClipboardData( void );     /* 0x00463300 */
extern int UI_checkKeyExec();

extern int chat_playerNum;

/* Retail cells used by this unit (flattened keys[] fields):
 *
 *   key_overstrikeMode   0x0142F628, owned by Key_Get/SetOverstrikeMode
 *                   (0x0040D480/0x0040D490) -- one storage, as in retail.
 *   keys_ctrlDown   retail keys[K_CTRL].down  -- 0x0142F780 + 159*12 = 0x0142FEF4.
 *   keys_shiftDown   retail keys[K_SHIFT].down -- 0x0142F780 + 160*12 = 0x0142FF00.
 *                   Both are cl_local.h macros reading keysStorage[3*159] /
 *                   keysStorage[3*160], the .down cells CL_KeyEvent writes, as
 *                   retail's absolute reads of those two addresses do.
 *   con_fieldWidthPixels    the console field width; 0x0057C0FC and 0x0057C100 are the
 *                   8.0f/16.0f char metrics beside it.  RTCW cl_console.c calls
 *                   the first g_console_field_width (cl_keys.c:1156 assigns it
 *                   at exactly this point); the three are defined and driven
 *                   from cl_console_mp.c and cl_main_mp.c.
 *
 * Indirect call slots used by Field_Draw and Field_AdjustScroll: re_Text_Width
 * is refexport_t +0xCC re.Text_Width and re_Text_PaintWithCursor is +0xE0
 * re.Text_PaintWithCursor (cl_refapi.c).
 */

/* Defined further down this unit. Retail __usercall(field@<eax>, ch@<ebx>), cdecl here. */
void __cdecl Field_CharEvent( field_t *field, int ch );

/* ---- Field_Draw  0x0040C500 ----  [HIGH] */
/* re_Text_PaintWithCursor is refexport_t +0xE0 re.Text_PaintWithCursor; it is
 * unprototyped in the header, so it is called here through its real signature
 * (float coordinates, a char cursor).  showCursor is pushed by every retail
 * caller and never read. */
typedef void ( *TextPaintWithCursor_t )( float x, float y, int font, float scale, const float *color,
                                         const char *text, int cursorPos, char cursor, float charWidth,
                                         int limit, int style );

void __cdecl Field_Draw( field_t *edit, int x, int y, int showCursor )
{
  char  text[1024];
  float color[4];
  float fx;
  float fy;
  float charWidth;
  float charHeightScale;
  int   style;
  int   charset;
  char  cursorChar;
  int   cursorPos;
  int   len;

  len = MAX_EDIT_LINE - edit->scroll;
  color[0] = 1.0f;
  color[1] = 1.0f;
  color[2] = 1.0f;
  color[3] = 1.0f;
  strncpy( text, edit->buffer + edit->scroll, len - 1 );

  fx              = (float)x;
  charWidth       = edit->charWidth;
  cursorPos       = edit->cursor - edit->scroll;
  fy              = y + edit->charHeight;              /* implicit int->float: a cast schedules the fadd later */
  /* retail 0x0040C58C terminates with no bound check; written as a pointer store because
     text[len - 1] = 0 makes modern /GS add a range-check fast-fail */
  *( text + len - 1 ) = 0;
  charHeightScale = edit->charHeight * 0.020833334f;      /* charHeight / 48; retail float const 0x00568FEC */

  if ( edit->fixedWidth )
  {
    rfloat xadjust;
    rfloat yadjust;

    charset         = 0;
    style           = 5;
    cursorChar      = ( key_overstrikeMode != 0 ) + 10;
    xadjust         = 640.0f / (rfloat)cls_glconfig_vidWidth;
    fx              = fx * xadjust;
    yadjust         = 480.0f / (rfloat)cls_glconfig_vidHeight;
    fy              = fy * yadjust;
    charHeightScale = yadjust * charHeightScale;
    charWidth       = xadjust * charWidth;
  }
  else
  {
    style      = 0;
    charset    = 3;
    cursorChar = key_overstrikeMode ? '_' : '|';
  }

  if ( !edit->widthInChars )
    edit->widthInChars = MAX_EDIT_LINE;

  ( (TextPaintWithCursor_t)re_Text_PaintWithCursor )(   /* re.Text_PaintWithCursor */
    fx,
    fy,
    style,
    charHeightScale,
    color,
    text,
    cursorPos,
    cursorChar,
    charWidth,
    edit->widthInChars,
    charset );
}

/* ---- Field_AdjustScroll  0x0040C660 ----  [HIGH] */
/* re_Text_Width is refexport_t +0xCC re.Text_Width; unprototyped, so the two
 * float metrics are passed as raw bit patterns.  It returns the pixel
 * width of the string (optionally limited to a leading character count). */
void __cdecl Field_AdjustScroll( field_t *field )
{
  struct { float width, height; } metrics;
  float  pixelWidth;
  rfloat xadjust;
  rfloat yadjust;
  rfloat width;
  int    charset;
  int    fullWidth;
  int    cursorOffset;
  int    len;
  int    scrollWidth;

  metrics.width   = field->charWidth;
  metrics.height = field->charHeight * 0.020833334f;       /* charHeight / 48; retail float const 0x00568FEC */
  pixelWidth  = (float)field->widthInPixels;

  if ( field->fixedWidth )
  {
    charset     = 5;
    yadjust     = 480.0f / (rfloat)cls_glconfig_vidHeight;
    metrics.height = yadjust * metrics.height;
    xadjust     = 640.0f / (rfloat)cls_glconfig_vidWidth;
    metrics.width   = metrics.width * xadjust;
    pixelWidth  = xadjust * pixelWidth;
  }
  else
  {
    charset = 0;
  }

  fullWidth = re_Text_Width( field->buffer, charset,
                             *(unsigned int *)&metrics.height, *(unsigned int *)&metrics.width, 0 );
  if ( (rfloat)fullWidth < pixelWidth )
  {
    /* the whole line fits, so no horizontal scroll is needed */
    field->scroll       = 0;
    field->widthInChars = SEH_PrintStrlen( field->buffer );
    return;
  }

  /* pull the scroll back while the tail still fits the visible box */
  width = 0;
  while ( width < pixelWidth && field->scroll > 0 )
  {
    width = re_Text_Width( field->buffer + field->scroll - 1, charset,
                           *(unsigned int *)&metrics.height, *(unsigned int *)&metrics.width, 0 );
    if ( width >= pixelWidth )
      break;
    --field->scroll;
  }

  /* nudge the scroll until the cursor lands inside the visible box */
  do
  {
    scrollWidth   = re_Text_Width( field->buffer + field->scroll, charset,
                                   *(unsigned int *)&metrics.height, *(unsigned int *)&metrics.width, 0 );
    cursorOffset  = scrollWidth - re_Text_Width( field->buffer + field->cursor, charset,
                                                 *(unsigned int *)&metrics.height, *(unsigned int *)&metrics.width, 0 );

    if ( cursorOffset >= 0 )
    {
      if ( (rfloat)cursorOffset >= pixelWidth )
        ++field->scroll;
    }
    else
    {
      if ( field->scroll )   /* this test order moves the block out of line, as in retail */
        --field->scroll;
      else
        cursorOffset = 0;
    }
  }
  while ( cursorOffset < 0 || (rfloat)cursorOffset >= pixelWidth );

  /* extend widthInChars to as many characters as fit from the scroll position */
  len = strlen( field->buffer + field->scroll );
  field->widthInChars = field->cursor - field->scroll;
  width = 0;
  while ( width < pixelWidth && field->widthInChars < len )
  {
    width = re_Text_Width( field->buffer + field->scroll, charset,
                           *(unsigned int *)&metrics.height, *(unsigned int *)&metrics.width,
                           field->widthInChars + 1 );
    if ( width < pixelWidth )
      ++field->widthInChars;
  }
}

/* ---- Field_Paste  0x0040C870 ----  [CONFIRMED] */
void __cdecl Field_Paste( field_t *field )
{
  char *cbd;
  int   pasteLen;
  int   i;

  cbd = Sys_GetClipboardData();
  if ( !cbd )
    return;

  pasteLen = strlen( cbd );
  for ( i = 0; i < pasteLen; i++ )
    Field_CharEvent( field, cbd[i] );

  free( cbd );
}

/* ---- Field_KeyDownEvent  0x0040C8C0 ----  [HIGH] */
/* keys_shiftDown / keys_ctrlDown are retail keys[K_SHIFT].down / keys[K_CTRL].down
 * (see the file header) and key_overstrikeMode is the retail overstrike flag. */
void __cdecl Field_KeyDownEvent( field_t *field, int key )
{
  int len;

  len = strlen( field->buffer );

  /* shift + Ins / KP_Ins pastes the clipboard */
  if ( ( key == 161 || key == 192 ) && keys_shiftDown )   /* 161 K_INS, 192 K_KP_INS */
  {
    Field_Paste( field );
  }
  else if ( key == 162 )   /* K_DEL */
  {
    if ( field->cursor < len )
      memmove( field->buffer + field->cursor,
               field->buffer + field->cursor + 1,
               len - field->cursor );
  }
  else if ( key == 157 )   /* K_RIGHTARROW */
  {
    if ( field->cursor < len )
      ++field->cursor;
    if ( keys_ctrlDown )   /* ctrl: skip to the start of the next word */
    {
      while ( field->cursor < len && isalnum( field->buffer[field->cursor] ) )
        ++field->cursor;
      while ( field->cursor < len && !isalnum( field->buffer[field->cursor] ) )
        ++field->cursor;
    }
  }
  else if ( key == 156 )   /* K_LEFTARROW */
  {
    if ( field->cursor > 0 )
      --field->cursor;
    if ( keys_ctrlDown )   /* ctrl: skip to the start of the current word */
    {
      while ( field->cursor > 0 && isalnum( field->buffer[field->cursor - 1] ) )
        --field->cursor;
    }
    if ( field->cursor < field->scroll )
      field->scroll = field->cursor;
  }
  else if ( key == 165 || ( tolower( key ) == 'a' && keys_ctrlDown ) )         /* K_HOME / ctrl+a */
  {
    field->cursor = 0;
  }
  else if ( key == 166 || ( tolower( key ) == 'e' && keys_ctrlDown ) )    /* K_END / ctrl+e */
  {
    field->cursor = len;
  }
  else if ( key == 161 )                                                  /* K_INS toggles overstrike */
  {
    key_overstrikeMode = !key_overstrikeMode;
  }

  if ( uivm )
    Field_AdjustScroll( field );
}

/* ---- Field_CharEvent  0x0040CA70 ----  VERIFIED */
void __cdecl Field_CharEvent( field_t *field, int ch )
{
  int len;

  len = strlen( field->buffer );

  if ( ch == 22 )   /* Ctrl-V: paste */
  {
    Field_Paste( field );
    Field_AdjustScroll( field );
    return;
  }

  if ( ch == 3 )    /* Ctrl-C: clear the line */
  {
    memset( field->buffer, 0, MAX_EDIT_LINE );
    field->cursor       = 0;
    field->scroll       = 0;
    field->widthInChars = MAX_EDIT_LINE;
    Field_AdjustScroll( field );
    return;
  }

  if ( ch == 8 )    /* Backspace */
  {
    if ( field->cursor > 0 )
    {
      /* memmove, not memcpy: retail calls the CRT copy here, never the rep movs intrinsic */
      memmove( field->buffer + field->cursor - 1,
               field->buffer + field->cursor,
               len - field->cursor + 1 );
      --field->cursor;
    }
    Field_AdjustScroll( field );
    return;
  }

  if ( ch == 1 )    /* Ctrl-A: home */
  {
    field->cursor = 0;
    field->scroll = 0;
    return;
  }

  if ( ch == 5 )    /* Ctrl-E: end */
  {
    field->cursor = len;
    Field_AdjustScroll( field );
    return;
  }

  if ( ch < 32 )
    return;

  if ( key_overstrikeMode )   /* overstrike */
  {
    if ( field->cursor == 255 )
      return;
    field->buffer[field->cursor] = ch;
    field->cursor++;
  }
  else                   /* insert */
  {
    if ( len == 255 )
      return;
    memmove( field->buffer + field->cursor + 1,
             field->buffer + field->cursor,
             len - field->cursor + 1 );
    field->buffer[field->cursor] = ch;
    field->cursor++;
  }

  if ( field->cursor == len + 1 )
    field->buffer[field->cursor] = 0;

  Field_AdjustScroll( field );
}

/* ---- CL_FindMatches  0x0040CB80 ----  VERIFIED */
void __cdecl CL_FindMatches( const char *s )
{
	int		i;

	if ( Q_stricmpn( s, completionString, strlen( completionString ) ) ) {
		return;
	}

	matchCount++;
	if ( matchCount == 1 ) {
		strncpy( shortestMatch, s, MAX_TOKEN_CHARS - 1 );
		shortestMatch[MAX_TOKEN_CHARS - 1] = 0;
		return;
	}

	for ( i = 0 ; s[i] ; i++ ) {
		if ( tolower( shortestMatch[i] ) != tolower( s[i] ) ) {
			break;
		}
	}
	/* retail 0x0040CC0D terminates with no bound check; written as a pointer store because
	   shortestMatch[i] = 0 makes modern /GS add a range-check fast-fail */
	*( shortestMatch + i ) = 0;
}

/* ---- CL_PrintMatches  0x0040CC20 ----  VERIFIED */
void __cdecl CL_PrintMatches( const char *s )
{
	if ( !Q_stricmpn( s, shortestMatch, strlen( shortestMatch ) ) ) {
		Com_Printf( "    %s\n", s );
	}
}

/* ---- keyConcatArgs  0x0040CC60 ----  VERIFIED */
static void keyConcatArgs( void )
{
	int		i;
	char	*arg;

	for ( i = 1 ; i < Cmd_Argc() ; i++ ) {
		Q_strcat( (char *)text_in, 256, " " );

		arg = Cmd_Argv( i );
		while ( *arg ) {
			if ( *arg == ' ' ) {
				Q_strcat( (char *)text_in, 256, "\"" );
				break;
			}
			arg++;
		}

		Q_strcat( (char *)text_in, 256, Cmd_Argv( i ) );

		if ( *arg == ' ' ) {
			Q_strcat( (char *)text_in, 256, "\"" );
		}
	}
}

/* ---- CL_ConcatRemaining  0x0040CE30 ----  VERIFIED */
void __cdecl CL_ConcatRemaining( const char *src, const char *start )
{
	char	*str;

	str = strstr( src, start );
	if ( !str ) {
		keyConcatArgs();
		return;
	}

	str += strlen( start );
	Q_strcat( (char *)text_in, 256, str );
}

/* ---- CompleteCommand  0x0040CE80 ----  VERIFIED */
void __cdecl CompleteCommand( void )
{
	field_t	temp;
	char	*cmd;

	Cmd_TokenizeString2( (const char *)text_in, 0 );

	cmd = Cmd_Argv( 0 );
	completionString = cmd;
	if ( cmd[0] == '\\' || cmd[0] == '/' ) {
		completionString = ++cmd;
	}

	matchCount = 0;
	shortestMatch[0] = 0;

	if ( !strlen( cmd ) ) {
		return;
	}

	Cmd_CommandCompletion( CL_FindMatches );
	Cvar_CommandCompletion( CL_FindMatches );

	if ( matchCount == 0 ) {
		return;
	}

	/* retail 0x0040CF40 copies the whole field_t (0x11C) from g_consoleField */
	Com_Memcpy( &temp, &g_consoleField, sizeof( temp ) );

	if ( matchCount == 1 ) {
		Com_sprintf( (char *)text_in, 256, "\\%s", shortestMatch );
		if ( cmd_argc == 1 ) {
			Q_strcat( (char *)text_in, 256, " " );
		} else {
			CL_ConcatRemaining( temp.buffer, completionString );
		}
		g_consoleField.cursor = strlen( (const char *)text_in );
		Field_AdjustScroll( &g_consoleField );
		return;
	}

	Com_sprintf( (char *)text_in, 256, "\\%s", shortestMatch );
	g_consoleField.cursor = strlen( (const char *)text_in );
	CL_ConcatRemaining( temp.buffer, completionString );
	Field_AdjustScroll( &g_consoleField );

	Com_Printf( "]%s\n", text_in );

	Cmd_CommandCompletion( CL_PrintMatches );
	Cvar_CommandCompletion( CL_PrintMatches );
}

/* ---- Console_Key  0x0040D050 ----  [HIGH] */
/* keys_ctrlDown / keys_shiftDown are retail keys[K_CTRL].down / keys[K_SHIFT].down
 * (see the file header). */
void __cdecl Console_Key( int key )
{
  char temp[1024];

  /* Ctrl-L clears the console */
  if ( key == 'l' && keys_ctrlDown )
  {
    Cbuf_AddText( "clear\n" );
    return;
  }

  if ( key != 13 && key != 191 )   /* not ENTER / KP_ENTER: an editing or navigation key */
  {
    if ( key == 9 )   /* TAB: command completion */
    {
      CompleteCommand();
      return;
    }

    /* recall the previous (older) history line */
    if ( ( key == 206 && keys_shiftDown )             /* shift + MWHEELUP */
         || key == 154 || key == 183                  /* UPARROW / KP_UPARROW */
         || ( tolower( key ) == 'p' && keys_ctrlDown ) )   /* Ctrl-P */
    {
      if ( nextHistoryLine - historyLine < COMMAND_HISTORY && historyLine > 0 )
        --historyLine;
      g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
      Field_AdjustScroll( &g_consoleField );
    }
    /* recall the next (newer) history line */
    else if ( ( key == 205 && keys_shiftDown )             /* shift + MWHEELDOWN */
         || key == 155 || key == 189                  /* DOWNARROW / KP_DOWNARROW */
         || ( tolower( key ) == 'n' && keys_ctrlDown ) )   /* Ctrl-N */
    {
      if ( historyLine == nextHistoryLine )
        return;
      ++historyLine;
      g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
      Field_AdjustScroll( &g_consoleField );
    }
    /* scrollback navigation */
    else if ( key == 164 )   /* K_PGUP */
    {
      Con_PageUp();
    }
    else if ( key == 163 )   /* K_PGDN */
    {
      Con_PageDown();
    }
    else if ( key == 206 )   /* K_MWHEELUP -- Ctrl scrolls a whole page */
    {
      Con_PageUp();
      if ( keys_ctrlDown )
      {
        Con_PageUp();
        Con_PageUp();
      }
    }
    else if ( key == 205 )   /* K_MWHEELDOWN -- Ctrl scrolls a whole page */
    {
      Con_PageDown();
      if ( keys_ctrlDown )
      {
        Con_PageDown();
        Con_PageDown();
      }
    }
    else if ( key == 165 && keys_ctrlDown )   /* Ctrl-HOME jumps to the top */
    {
      Con_Top();
    }
    else if ( key == 166 && keys_ctrlDown )   /* Ctrl-END jumps to the bottom */
    {
      Con_Bottom();
    }
    else
    {
      Field_KeyDownEvent( &g_consoleField, key );
    }
    return;
  }

  /* ENTER / KP_ENTER: dispatch the command line */
  if ( cls_state != CA_ACTIVE && text_in[0] != '\\' && text_in[0] != '/' )
  {
    strncpy( temp, (const char *)text_in, 0x3FFu );
    temp[1023] = 0;
    Com_sprintf( (char *)text_in, MAX_EDIT_LINE, "\\%s", temp );
    ++g_consoleField.cursor;
  }

  Com_Printf( "]%s\n", text_in );

  if ( text_in[0] != '\\' && text_in[0] != '/' )
  {
    if ( !text_in[0] )
      return;
    Cbuf_AddText( "cmd say " );
    Cbuf_AddText( (const char *)text_in );
  }
  else
  {
    Cbuf_AddText( (const char *)text_in + 1 );
  }
  Cbuf_AddText( "\n" );

  historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
  ++nextHistoryLine;
  historyLine = nextHistoryLine;
  memset( text_in, 0, MAX_EDIT_LINE );
  g_consoleField.cursor        = 0;
  g_consoleField.scroll        = 0;
  g_consoleField.widthInChars  = MAX_EDIT_LINE;
  g_consoleField.widthInPixels = con_fieldWidthPixels;
  *(int *)&g_consoleField.charWidth  = con_fieldCharWidth;
  *(int *)&g_consoleField.charHeight = con_fieldCharHeight;
  g_consoleField.fixedWidth    = 1;

  if ( cls_state == CA_DISCONNECTED )
    SCR_UpdateScreen();
}

/* ---- Message_Key  0x0040D380 ----  VERIFIED */
void __cdecl Message_Key( int key )
{
	char	buffer[MAX_STRING_CHARS];

	/* chat_team below is RTCW cl_keys.c:45 `qboolean chat_team`; the three
	 * Con_MessageMode*_f in cl_console_mp.c drive it exactly as RTCW does. */
	if ( key == 27 ) {
		cls_keyCatchers &= ~KEYCATCH_MESSAGE;
		Field_Clear( &chatField );
		return;
	}

	if ( key == 13 || key == 0xBF ) {
		if ( chatField.buffer[0] && cls_state == CA_ACTIVE ) {
			if ( chat_playerNum != -1 ) {
				Com_sprintf( buffer, sizeof( buffer ), "tell %i \"" "\x15" "%s\"\n",
							 chat_playerNum, chatField.buffer );
			} else if ( chat_team ) {
				Com_sprintf( buffer, sizeof( buffer ), "say_team \"" "\x15" "%s\"\n",
							 chatField.buffer );
			} else {
				Com_sprintf( buffer, sizeof( buffer ), "say \"" "\x15" "%s\"\n",
							 chatField.buffer );
			}
			CL_AddReliableCommand( buffer );
		}
		cls_keyCatchers &= ~KEYCATCH_MESSAGE;
		Field_Clear( &chatField );
		return;
	}

	Field_KeyDownEvent( &chatField, key );
}

/* ---- Key_GetOverstrikeMode  0x0040D480 ----  [CONFIRMED] */
qboolean Key_GetOverstrikeMode(void)
{
    return key_overstrikeMode;
}

/* ---- Key_SetOverstrikeMode  0x0040D490 ----  [CONFIRMED] */
void Key_SetOverstrikeMode(qboolean enabled)
{
    key_overstrikeMode = enabled;
}

/* ---- Key_IsDown  0x0040D4A0 ----  VERIFIED */
qboolean __cdecl Key_IsDown( int keynum )
{
	if ( keynum == -1 ) {
		return qfalse;
	}
	return (qboolean)keysStorage[3 * keynum];
}

/* Key_StringToKeynum (0x0040D4C0) is defined further down this unit, along with the keynames[] table at 0x0057B8D0. */

/* Key_SetBinding (0x0040D6A0) is defined further down this unit, where the keys[] table lives. */

/* Key_GetBinding (0x0040D700) is defined further down this unit. */

/* ---- Key_GetKey  0x0040D730 ----  VERIFIED */
int __cdecl Key_GetKey( const char *binding )
{
	int		i;

	if ( binding ) {
		for ( i = 0 ; i < MAX_KEYS ; i++ ) {
			if ( keysStorage[3 * i + 2]
				 && !Q_stricmpn( binding, (const char *)keysStorage[3 * i + 2], 99999 ) ) {
				return i;
			}
		}
	}

	return -1;
}
/* Key_Unbind_f (0x0040D770) is defined further down this unit. */

/* Key_UnbindAll_f (0x0040D7C0) is defined further down this unit. */

/* Key_Bind_f (0x0040D800) is defined further down this unit. */

/* Key_WriteBindings (0x0040D9A0) is defined further down this unit. */

/* Key_Bindlist_f (0x0040DAD0) is defined further down this unit. */

/* ---- CL_InitKeyCommands  0x0040DBF0 ----  [CONFIRMED] */
void __cdecl CL_InitKeyCommands()
{
  Cmd_AddCommand("bind", Key_Bind_f);
  Cmd_AddCommand("unbind", Key_Unbind_f);
  Cmd_AddCommand("unbindall", Key_UnbindAll_f);
  Cmd_AddCommand("bindlist", Key_Bindlist_f);
}

/* CL_KeyEvent (0x0040DC30) is defined further down this unit. */

/* ---- Key_ClearStates  0x0040E260 ----  VERIFIED */
void __cdecl Key_ClearStates( void )
{
  int  i;

  anykeydown = 0;
  for ( i = 0; i < MAX_KEYS; i++ )
  {
    if ( keysStorage[3 * i] )         /* keys[i].down */
      CL_KeyEvent( i, qfalse, 0 );
    keysStorage[3 * i] = 0;           /* keys[i].down */
    keysStorage[3 * i + 1] = 0;       /* keys[i].repeats */
  }
}

                    /* KEY BINDINGS 0x0040D4C0-0x0040DBF0 */

typedef struct {
	qboolean down;
	int repeats;
	char *binding;
} qkey_t;

/* RTCW cl_keys.c:52 `qkey_t keys[MAX_KEYS];`.  Key_IsDown, Key_GetKey and
 * Key_ClearStates above index keysStorage[3*keynum] by hand; keys[n].down
 * emits a different multiply. */
#define keys        ( (qkey_t *)keysStorage )
#define anykeydown  anykeydown           /* 0x0142F75C */

typedef struct {
	const char *name;
	int keynum;
} keyname_t;

static keyname_t keynames[] = {
	{ "TAB", 9 }, { "ENTER", 13 }, { "ESCAPE", 27 }, { "SPACE", 32 },
	{ "BACKSPACE", 127 }, { "UPARROW", 154 }, { "DOWNARROW", 155 },
	{ "LEFTARROW", 156 }, { "RIGHTARROW", 157 },
	{ "ALT", 158 }, { "CTRL", 159 }, { "SHIFT", 160 }, { "CAPSLOCK", 151 },
	{ "F1", 167 }, { "F2", 168 }, { "F3", 169 }, { "F4", 170 }, { "F5", 171 },
	{ "F6", 172 }, { "F7", 173 }, { "F8", 174 }, { "F9", 175 }, { "F10", 176 },
	{ "F11", 177 }, { "F12", 178 },
	{ "INS", 161 }, { "DEL", 162 }, { "PGDN", 163 }, { "PGUP", 164 },
	{ "HOME", 165 }, { "END", 166 },
	{ "MOUSE1", 200 }, { "MOUSE2", 201 }, { "MOUSE3", 202 }, { "MOUSE4", 203 },
	{ "MOUSE5", 204 }, { "MWHEELUP", 206 }, { "MWHEELDOWN", 205 },
	{ "JOY1", 207 }, { "JOY2", 208 }, { "JOY3", 209 }, { "JOY4", 210 },
	{ "JOY5", 211 }, { "JOY6", 212 }, { "JOY7", 213 }, { "JOY8", 214 },
	{ "JOY9", 215 }, { "JOY10", 216 }, { "JOY11", 217 }, { "JOY12", 218 },
	{ "JOY13", 219 }, { "JOY14", 220 }, { "JOY15", 221 }, { "JOY16", 222 },
	{ "JOY17", 223 }, { "JOY18", 224 }, { "JOY19", 225 }, { "JOY20", 226 },
	{ "JOY21", 227 }, { "JOY22", 228 }, { "JOY23", 229 }, { "JOY24", 230 },
	{ "JOY25", 231 }, { "JOY26", 232 }, { "JOY27", 233 }, { "JOY28", 234 },
	{ "JOY29", 235 }, { "JOY30", 236 }, { "JOY31", 237 }, { "JOY32", 238 },
	{ "AUX1", 239 }, { "AUX2", 240 }, { "AUX3", 241 }, { "AUX4", 242 },
	{ "AUX5", 243 }, { "AUX6", 244 }, { "AUX7", 245 }, { "AUX8", 246 },
	{ "AUX9", 247 }, { "AUX10", 248 }, { "AUX11", 249 }, { "AUX12", 250 },
	{ "AUX13", 251 }, { "AUX14", 252 }, { "AUX15", 253 }, { "AUX16", 254 },
	{ "KP_HOME", 182 }, { "KP_UPARROW", 183 }, { "KP_PGUP", 184 },
	{ "KP_LEFTARROW", 185 }, { "KP_5", 186 }, { "KP_RIGHTARROW", 187 },
	{ "KP_END", 188 }, { "KP_DOWNARROW", 189 }, { "KP_PGDN", 190 },
	{ "KP_ENTER", 191 }, { "KP_INS", 192 }, { "KP_DEL", 193 },
	{ "KP_SLASH", 194 }, { "KP_MINUS", 195 }, { "KP_PLUS", 196 },
	{ "KP_NUMLOCK", 197 }, { "KP_STAR", 198 }, { "KP_EQUALS", 199 },
	{ "PAUSE", 153 },
	{ "SEMICOLON", 59 },
	{ "COMMAND", 150 },
	{ "181", 128 }, { "191", 129 }, { "223", 130 }, { "224", 131 },
	{ "225", 132 }, { "228", 133 }, { "229", 134 }, { "230", 135 },
	{ "231", 136 }, { "232", 137 }, { "233", 138 }, { "236", 139 },
	{ "241", 140 }, { "242", 141 }, { "243", 142 }, { "246", 143 },
	{ "248", 144 }, { "249", 145 }, { "250", 146 }, { "252", 147 },
	{ NULL, 0 }
};

/* Retail 0x0057BCD0: the localization-reference twin of keynames[], in the same
 * order with a "KEY_" prefix on every name.  Its 128..147 entries are the
 * Latin-1 characters themselves.  Key_KeynumToString reads it only when
 * translate is set, which only Key_KeynumToStringBuf does. */
static keyname_t keynames_loc[] = {
	{ "KEY_TAB", 9 }, { "KEY_ENTER", 13 }, { "KEY_ESCAPE", 27 }, { "KEY_SPACE", 32 },
	{ "KEY_BACKSPACE", 127 }, { "KEY_UPARROW", 154 }, { "KEY_DOWNARROW", 155 },
	{ "KEY_LEFTARROW", 156 }, { "KEY_RIGHTARROW", 157 },
	{ "KEY_ALT", 158 }, { "KEY_CTRL", 159 }, { "KEY_SHIFT", 160 }, { "KEY_CAPSLOCK", 151 },
	{ "KEY_F1", 167 }, { "KEY_F2", 168 }, { "KEY_F3", 169 }, { "KEY_F4", 170 }, { "KEY_F5", 171 },
	{ "KEY_F6", 172 }, { "KEY_F7", 173 }, { "KEY_F8", 174 }, { "KEY_F9", 175 }, { "KEY_F10", 176 },
	{ "KEY_F11", 177 }, { "KEY_F12", 178 },
	{ "KEY_INS", 161 }, { "KEY_DEL", 162 }, { "KEY_PGDN", 163 }, { "KEY_PGUP", 164 },
	{ "KEY_HOME", 165 }, { "KEY_END", 166 },
	{ "KEY_MOUSE1", 200 }, { "KEY_MOUSE2", 201 }, { "KEY_MOUSE3", 202 }, { "KEY_MOUSE4", 203 },
	{ "KEY_MOUSE5", 204 }, { "KEY_MWHEELUP", 206 }, { "KEY_MWHEELDOWN", 205 },
	{ "KEY_JOY1", 207 }, { "KEY_JOY2", 208 }, { "KEY_JOY3", 209 }, { "KEY_JOY4", 210 },
	{ "KEY_JOY5", 211 }, { "KEY_JOY6", 212 }, { "KEY_JOY7", 213 }, { "KEY_JOY8", 214 },
	{ "KEY_JOY9", 215 }, { "KEY_JOY10", 216 }, { "KEY_JOY11", 217 }, { "KEY_JOY12", 218 },
	{ "KEY_JOY13", 219 }, { "KEY_JOY14", 220 }, { "KEY_JOY15", 221 }, { "KEY_JOY16", 222 },
	{ "KEY_JOY17", 223 }, { "KEY_JOY18", 224 }, { "KEY_JOY19", 225 }, { "KEY_JOY20", 226 },
	{ "KEY_JOY21", 227 }, { "KEY_JOY22", 228 }, { "KEY_JOY23", 229 }, { "KEY_JOY24", 230 },
	{ "KEY_JOY25", 231 }, { "KEY_JOY26", 232 }, { "KEY_JOY27", 233 }, { "KEY_JOY28", 234 },
	{ "KEY_JOY29", 235 }, { "KEY_JOY30", 236 }, { "KEY_JOY31", 237 }, { "KEY_JOY32", 238 },
	{ "KEY_AUX1", 239 }, { "KEY_AUX2", 240 }, { "KEY_AUX3", 241 }, { "KEY_AUX4", 242 },
	{ "KEY_AUX5", 243 }, { "KEY_AUX6", 244 }, { "KEY_AUX7", 245 }, { "KEY_AUX8", 246 },
	{ "KEY_AUX9", 247 }, { "KEY_AUX10", 248 }, { "KEY_AUX11", 249 }, { "KEY_AUX12", 250 },
	{ "KEY_AUX13", 251 }, { "KEY_AUX14", 252 }, { "KEY_AUX15", 253 }, { "KEY_AUX16", 254 },
	{ "KEY_KP_HOME", 182 }, { "KEY_KP_UPARROW", 183 }, { "KEY_KP_PGUP", 184 },
	{ "KEY_KP_LEFTARROW", 185 }, { "KEY_KP_5", 186 }, { "KEY_KP_RIGHTARROW", 187 },
	{ "KEY_KP_END", 188 }, { "KEY_KP_DOWNARROW", 189 }, { "KEY_KP_PGDN", 190 },
	{ "KEY_KP_ENTER", 191 }, { "KEY_KP_INS", 192 }, { "KEY_KP_DEL", 193 },
	{ "KEY_KP_SLASH", 194 }, { "KEY_KP_MINUS", 195 }, { "KEY_KP_PLUS", 196 },
	{ "KEY_KP_NUMLOCK", 197 }, { "KEY_KP_STAR", 198 }, { "KEY_KP_EQUALS", 199 },
	{ "KEY_PAUSE", 153 },
	{ "KEY_SEMICOLON", 59 },
	{ "KEY_COMMAND", 150 },
	{ "\xB5", 128 }, { "\xBF", 129 }, { "\xDF", 130 }, { "\xE0", 131 },
	{ "\xE1", 132 }, { "\xE4", 133 }, { "\xE5", 134 }, { "\xE6", 135 },
	{ "\xE7", 136 }, { "\xE8", 137 }, { "\xE9", 138 }, { "\xEC", 139 },
	{ "\xF1", 140 }, { "\xF2", 141 }, { "\xF3", 142 }, { "\xF6", 143 },
	{ "\xF8", 144 }, { "\xF9", 145 }, { "\xFA", 146 }, { "\xFC", 147 },
	{ NULL, 0 }
};

/* Retail 0x0057C0D0 (read as off_57C010[keynum] for keynum '0'..'9'): the
 * unshifted top-row characters of the French AZERTY layout, which
 * Key_KeynumToString returns for the digit keys when translate is set and
 * cl_language is 1. */
static const char *keynames_digitsFR[10] = {
	"&", "\xE9", "\"", "'", "(", "-", "\xE8", "_", "\xE7", "\xE0"
};

/* ---- Key_StringToKeynum  0x0040D4C0 ----  [CONFIRMED] */
int Key_StringToKeynum( const char *str ) {
	keyname_t *kn;

	if ( !str || !str[0] ) {
		return -1;
	}
	if ( !str[1] ) {
		/* retail 0x0040D4DA `movsx eax, al`: plain signed char, so a single
		 * Latin-1 character >= 0x80 comes back as a negative keynum (-128..-1).
		 * Key_Bind_f and Key_Unbind_f only reject -1, so retail then indexes
		 * keys[] with that negative value (out of bounds, bug-for-bug). */
		return str[0];
	}

	if ( str[0] == '0' && str[1] == 'x' && strlen( str ) == 4 ) {
		int n1, n2;

		/* retail 0x0040D504 / 0x0040D529 test the digits with Q_isnumeric (0x0044A9B0) */
		n1 = str[2];
		if ( Q_isnumeric( n1 ) ) {
			n1 -= '0';
		} else if ( n1 >= 'a' && n1 <= 'f' ) {
			n1 = n1 - 'a' + 10;
		} else {
			n1 = 0;
		}

		n2 = str[3];
		if ( Q_isnumeric( n2 ) ) {
			n2 -= '0';
		} else if ( n2 >= 'a' && n2 <= 'f' ) {
			n2 = n2 - 'a' + 10;
		} else {
			n2 = 0;
		}

		return n1 * 16 + n2;
	}

	for ( kn = keynames; kn->name; kn++ ) {
		if ( !Q_stricmp( str, kn->name ) ) {
			return kn->keynum;
		}
	}
	return -1;
}

/* ---- Key_KeynumToString  0x0040D5A0 ----  [CONFIRMED] */
/* Retail __usercall(keynum@<eax>, translate@<edi>).  The only out-of-line
 * caller is Key_KeynumToStringBuf (translate 1).  Key_WriteBindings,
 * Key_Bindlist_f and CL_KeyEvent inline the translate 0 path, so they pass
 * qfalse here. */
char *Key_KeynumToString( int keynum, qboolean translate ) {
	static char tinystr[5];
	keyname_t *kn;
	int i, j;

	if ( keynum == -1 ) {
		return "<KEY NOT FOUND>";
	}
	if ( keynum < 0 || keynum > 255 ) {
		return "<OUT OF RANGE>";
	}

	/* retail 0x0040D5C3-0x0040D5E4 */
	if ( translate && cl_language->integer == 1 && keynum >= '0' && keynum <= '9' ) {
		return (char *)keynames_digitsFR[keynum - '0'];
	}

	if ( keynum > 32 && keynum < 127 && keynum != '"' ) {
		tinystr[0] = (char)toupper( keynum );
		tinystr[1] = 0;
		/* retail 0x0040D5FD-0x0040D610: only the untranslated ';' goes on
		 * to the keynames[] lookup ("SEMICOLON"). */
		if ( keynum != ';' || translate ) {
			return tinystr;
		}
	}

	kn = translate ? keynames_loc : keynames;

	for ( ; kn->name; kn++ ) {
		if ( keynum == kn->keynum ) {
			return (char *)kn->name;
		}
	}

	i = keynum >> 4;
	j = keynum & 15;
	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = (char)( i > 9 ? i + 'a' - 10 : i + '0' );
	tinystr[3] = (char)( j > 9 ? j + 'a' - 10 : j + '0' );
	tinystr[4] = 0;
	return tinystr;
}

/* ---- Key_SetBinding  0x0040D6A0 ----  [HIGH] */
void Key_SetBinding( int keynum, const char *binding ) {
	if ( keynum == -1 ) {
		return;
	}

	if ( keys[keynum].binding ) {
		Z_FreeInternal( keys[keynum].binding );
	}

	keys[keynum].binding = CopyStringInternal( binding );

	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/* ---- Key_GetBinding  0x0040D700 ----  [CONFIRMED] */
char *Key_GetBinding( int keynum ) {
	if ( keynum == -1 ) {
		return "";
	}
	return keys[keynum].binding;
}

/* ---- Key_Unbind_f  0x0040D770 ----  [CONFIRMED] */
void Key_Unbind_f( void ) {
	int b;

	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "unbind <key> : remove commands from a key\n" );
		return;
	}

	b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if ( b == -1 ) {
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	Key_SetBinding( b, "" );
}

/* ---- Key_UnbindAll_f  0x0040D7C0 ----  [CONFIRMED] */
void Key_UnbindAll_f( void ) {
	int i;

	for ( i = 0; i < MAX_KEYS; i++ ) {
		if ( keys[i].binding ) {
			Key_SetBinding( i, "" );
		}
	}
}

/* ---- Key_Bind_f  0x0040D800 ----  [CONFIRMED] */
void Key_Bind_f( void ) {
	int i, c, b;
	char cmd[MAX_STRING_CHARS];

	c = Cmd_Argc();
	if ( Cmd_Argc() < 2 ) {   /* retail tests the inlined count itself, not a copy of c */
		Com_Printf( "bind <key> [command] : attach a command to a key\n" );
		return;
	}

	b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if ( b == -1 ) {
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	/* Retail folds the keynum here (0x0040D891); Key_Unbind_f (0x0040D770) deliberately does not. */
	b = tolower( b );

	/* retail 0x0040D8B5-0x0040D8F2 echoes the key exactly as typed (Cmd_Argv(1)), not Key_KeynumToString. */
	if ( c == 2 ) {
		if ( keys[b].binding ) {
			Com_Printf( "\"%s\" = \"%s\"\n", Cmd_Argv( 1 ), keys[b].binding );
		} else {
			Com_Printf( "\"%s\" is not bound\n", Cmd_Argv( 1 ) );
		}
		return;
	}

	cmd[0] = 0;
	for ( i = 2; i < c; i++ ) {
		strcat( cmd, Cmd_Argv( i ) );
		if ( i != ( c - 1 ) ) {
			strcat( cmd, " " );
		}
	}

	Key_SetBinding( b, cmd );
}

/* ---- Key_WriteBindings  0x0040D9A0 ----  [CONFIRMED] */
void Key_WriteBindings( fileHandle_t f ) {
	int i;

	FS_Printf( f, "unbindall\n" );

	for ( i = 0; i < MAX_KEYS; i++ ) {
		if ( keys[i].binding && keys[i].binding[0] ) {
			FS_Printf( f, "bind %s \"%s\"\n", Key_KeynumToString( i, qfalse ), keys[i].binding );
		}
	}
}

/* ---- Key_Bindlist_f  0x0040DAD0 ----  [CONFIRMED] */
void Key_Bindlist_f( void ) {
	int i;

	for ( i = 0; i < MAX_KEYS; i++ ) {
		if ( keys[i].binding && keys[i].binding[0] ) {
			Com_Printf( "%s \"%s\"\n", Key_KeynumToString( i, qfalse ), keys[i].binding );
		}
	}
}

/* ---- CL_KeyEvent  0x0040DC30 ----  VERIFIED */
void CL_KeyEvent( int key, int down, unsigned time ) {
	char *kb;
	char cmd[1024];
	qboolean bypassMenu = qfalse;

	/* retail 0x0040DC6B stores the raw down value, not a normalised 0/1 */
	keys[key].down = down;

	if ( down ) {
		keys[key].repeats++;
		if ( keys[key].repeats == 1 ) {
			anykeydown++;
		}
	} else {
		keys[key].repeats = 0;
		anykeydown--;
		if ( anykeydown < 0 ) {
			anykeydown = 0;
		}
	}

	if ( down ) {
		/* retail 0x0040DD2C: auto-repeats are swallowed unless a text field (console or chat) is up,
		 * and the toggle/escape keys never repeat; jle, so a count below 1 skips the test */
		if ( keys[key].repeats > 1
		     && ( !( cls_keyCatchers & ( KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) )
		          || key == '`' || key == '~' || key == 27 ) ) {
			return;
		}

		if ( cl_waitForFire && cl_waitForFire->integer ) {
			if ( cls_keyCatchers & KEYCATCH_CONSOLE ) {
				Con_ToggleConsole_f();
			}
			kb = keys[key].binding;
			CL_ClearKeys();
			if ( !Q_stricmp( kb, "+attack" ) ) {
				Cvar_Set2( "cl_waitForFire", "0", qtrue );
			}
			return;
		}
	}

	if ( key == '`' || key == '~' ) {
		if ( down ) {
			Con_ToggleConsole_f();
		}
		return;
	}

	if ( down && ( key < 128 || key == 200  )
	     && ( clc_demoplaying || cls_state == CA_CINEMATIC || cls_state == CA_LOGO  )
	     && !cls_keyCatchers ) {
		Cvar_Set2( "nextdemo", "", qtrue );
		key = 27;
	}

	/* Escape is always handled specially, retail 0x0040DDC1-0x0040DE9B -- but only on the way down; an ESC key-up (or any other key) falls through to the CG_CHECK_EXEC_KEY gate below like normal. */
	if ( key == 27 && down ) {
		if ( cls_keyCatchers & KEYCATCH_MESSAGE ) {
			Message_Key( key );
			return;
		}
		if ( cls_keyCatchers & KEYCATCH_CGAME ) {
			cls_keyCatchers &= ~KEYCATCH_CGAME;
			VM_Call( cgvm, CG_EVENT_HANDLING, 0  );
			return;
		}
		if ( !( cls_keyCatchers & KEYCATCH_UI ) ) {
			switch ( cls_state ) {
			case CA_ACTIVE:
				if ( !clc_demoplaying ) {
					if ( !cl_serverloadwaiting->integer ) {
						VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_INGAME );
					} else {
						VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
					}
				} else {
					VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
				}
				break;
			case CA_CINEMATIC:
			case CA_LOGO:
				CL_Disconnect_f();
				MSS_StopSounds( 0 );
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
				break;
			default:
				if ( uivm ) {
					VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
				}
				break;
			}
			return;
		}

		VM_Call( uivm, UI_KEY_EVENT, key, down );
		return;
	}

	/* retail 0x0040DEB7-0x0040DECE: cgame gets first refusal on every key that isn't a down-ESC, regardless of catcher state -- a non-zero return consumes the key entirely before catcher dispatch even runs. */
	if ( cgvm && VM_Call( cgvm, CG_CHECK_EXEC_KEY, key, down ) ) {
		return;
	}

	if ( !down ) {
		kb = keys[key].binding;
		if ( kb && kb[0] == '+' ) {
			Com_sprintf( cmd, sizeof( cmd ), "-%s %i %i\n", kb + 1, key, time );
			Cbuf_AddText( cmd );
		}

		/* retail 0x0040DF36-0x0040DF59: a NULL uivm falls through to the cgame test. */
		if ( ( cls_keyCatchers & KEYCATCH_UI ) && uivm ) {
			VM_Call( uivm, UI_KEY_EVENT, key, down );
		} else if ( ( cls_keyCatchers & KEYCATCH_CGAME ) && cgvm ) {
			VM_Call( cgvm, CG_KEY_EVENT, key, down );
		}
		return;
	}

	if ( cl_bypassMouseInput && cl_bypassMouseInput->integer ) {
		if ( key == 200 || key == 201 || key == 202  ) {
			if ( cl_bypassMouseInput->integer == 1 ) {
				bypassMenu = qtrue;
			}
		} else if ( !UI_checkKeyExec( key ) ) {
			bypassMenu = qtrue;
		}
	}

	if ( cls_keyCatchers & KEYCATCH_CONSOLE ) {
		Console_Key( key );
		return;
	}
	if ( ( cls_keyCatchers & KEYCATCH_UI ) && !bypassMenu ) {
		kb = keys[key].binding;
		if ( kb && !Q_stricmp( "help", kb ) && VM_Call( uivm, 8  ) == 7 ) {
			key = 27;
		}
		VM_Call( uivm, UI_KEY_EVENT, key, down );
		return;
	}
	if ( cls_keyCatchers & KEYCATCH_CGAME ) {
		if ( cgvm ) {
			VM_Call( cgvm, CG_KEY_EVENT, key, down );
		}
		return;
	}
	if ( cls_keyCatchers & KEYCATCH_MESSAGE ) {
		Message_Key( key );
		return;
	}
	if ( cls_state == CA_DISCONNECTED ) {
		Console_Key( key );
		return;
	}

	/* send the bound action, retail 0x0040E073-0x0040E1DA. Unlike the key-up tail above, a plain (non-'+') binding fires unconditionally here since this whole tail only runs on the way down. */
	kb = keys[key].binding;
	if ( !kb ) {
		if ( key >= 200 ) {
			Com_Printf( "%s is unbound, use controls menu to set.\n", Key_KeynumToString( key, qfalse ) );
		}
		return;
	}

	if ( kb[0] == '+' ) {
		Com_sprintf( cmd, sizeof( cmd ), "%s %i %i\n", kb, key, time );
		Cbuf_AddText( cmd );
	} else {
		Cbuf_AddText( kb );
		Cbuf_AddText( "\n" );
	}
}

/* ---- CL_CharEvent  0x0040E200 ----  VERIFIED */
void CL_CharEvent( int key ) {
	if ( key == '`' || key == '~' ) {
		return;
	}

	if ( cls_keyCatchers & KEYCATCH_CONSOLE ) {
		Field_CharEvent( &g_consoleField, key );
		return;
	}

	/* retail 0x0040E216-0x0040E233: no uivm NULL test before the call. */
	if ( cls_keyCatchers & KEYCATCH_UI ) {
		VM_Call( uivm, UI_KEY_EVENT, key | K_CHAR_FLAG, qtrue );
		return;
	}

	if ( cls_keyCatchers & KEYCATCH_MESSAGE ) {
		Field_CharEvent( &chatField, key );
		return;
	}

	if ( cls_state == CA_DISCONNECTED ) {
		Field_CharEvent( &g_consoleField, key );
	}
}

#define cl_mouseDx      cl_mouseDx       /* 0x0143A948, int[2] */
#define cl_mouseDy      cl_mouseDy       /* 0x0143A950, int[2] */
#define cl_mouseIndex   cl_mouseIndex       /* 0x0143A958 */
#define cl_joystickAxis cl_joystickAxis       /* 0x0143A95C, int[6] */

/* ---- Key_Shutdown  0x0040E2B0 ----  VERIFIED */
void Key_Shutdown( void ) {
	unsigned int i;   /* retail walks a byte offset with an unsigned (jb) compare */

	for ( i = 0 ; i < MAX_KEYS ; i++ ) {
		if ( keys[i].binding ) {
			Z_FreeInternal( keys[i].binding );
			keys[i].binding = NULL;
		}
	}
}
