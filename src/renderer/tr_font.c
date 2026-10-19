/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * tr_font.c -- font registration and 2D text layout / painting.
 *
 * CoD1 ships pre-rendered bitmap fonts (RTCW's fontImage_%i.dat scheme) plus a
 * CJK ("asian") atlas path RTCW never had.  RE_RegisterFont loads a glyph table
 * from disk; R_GetCharacterGlyph / R_GetGlyphHorizAdvance / R_GetAsianScale
 * resolve per-glyph metrics (routing CJK codepoints through the shared
 * g_asianGlyph record); RE_Text_Width / RE_Text_Height measure a string; and the
 * RE_Text_* / R_Text_* console entry points queue text-draw render commands.
 *
 * fontInfo_t / glyphInfo_t come from tr_font.h.
 * The render command RE_Text_PaintWithCursor writes is defined file-locally
 * (textPaintCommand_t -- offsets proven against the retail binary and its
 * RB_Text_Paint / RB_Text_PaintWithCursor backend reader, pinned with asserts);
 * tr_local.h / tr_font.h are intentionally not touched.
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
#include "tr_font.h"
/* ---- renderer-owned globals ---- */
int rendererRegisteredFontCount;   /* 0x014072F4 */
int rendererAsianFontLoaded;   /* 0x014072F8 */
char s_consoleTextBuffer[1024];
int rendererAsianFontLoadedLanguage = -1;   /* 0x00571544  retail .data init 0xFFFFFFFF */
int g_asianFontPageCount;
int g_asianFontPageDimension;
int g_asianFontPageHandles[4];
int rendererAsianFontLastPageHalfHeight;
byte *s_fontData;
int s_fontDataOffset;


extern int RE_RegisterShaderNoMip();
extern int RE_SetColor();
unsigned int __cdecl RE_StretchPic( float x, float y, float w, float h,
                                    float s1, float t1, float s2, float t2, int hShader );
extern void         __cdecl   R_IssueRenderCommands( int runPerformanceCounters );
extern void         __cdecl   R_SyncRenderThread( void );
extern unsigned int __cdecl R_GetCommandBuffer( unsigned int bytes );

int __cdecl SEH_ReadCharFromString( unsigned char **pCursor, unsigned int *pIsPunct );
unsigned int __fastcall Japanese_CollapseShiftJISCode( int uiCode );
unsigned int __fastcall Korean_CollapseKSC5601HangulCode( int uiCode );
int          __fastcall Taiwanese_CollapseBig5Code( int uiCode );

/* RE_Text_PaintWithCursor is defined last but queued by RE_Text_Paint and
 * RE_Text_ConsolePaint above it. */
void __cdecl RE_Text_PaintWithCursor( float x, float y, int font, float scale, float *color,
                                      const char *text, int cursorPos, char cursorChar, int fixedAdvance, int limit, int style );

/* Readable aliases for the two font globals (standalone symbols in the retail
 * image; defined above). */
#define registeredFontCount       rendererRegisteredFontCount    /* 0x014072F4  RE_RegisterFont cache count */
#define asianFontLoaded           rendererAsianFontLoaded    /* 0x014072F8  CJK atlas is registered */
#define asianFontLoadedLanguage   rendererAsianFontLoadedLanguage     /* 0x00571544  init -1, no writer (cache never hits) */

/* RE_RegisterFont's eight-entry font cache (registeredFontCount tracks fill). */
static fontInfo_t s_registeredFonts[8];

/* The CJK atlas glyph record: R_LoadAsianFont seeds it, R_GetCharacterGlyph
 * rewrites its atlas coordinates + page handle in place.  Declared in tr_font.h. */
glyphInfo_t g_asianGlyph;

/* Text-draw render command (RC_TEXT_PAINT == 6).  RE_Text_PaintWithCursor fills
 * it; RB_Text_Paint unpacks it back into RB_Text_PaintWithCursor.  The DWORD
 * slots are copied verbatim from the producer arguments. */
typedef struct {
    int   commandId;     /* +0x00  RC_TEXT_PAINT */
    float x;             /* +0x04 */
    float y;             /* +0x08 */
    int   font;          /* +0x0c  font handle */
    float scale;         /* +0x10 */
    byte  color[4];      /* +0x14  RGBA */
    int   fixedAdvance;  /* +0x18  float bits; fixed per-glyph advance / justify width */
    int   style;         /* +0x1c  justify / style mode */
    int   cursorPos;     /* +0x20  glyph index to draw the cursor at, -1 = none */
    char  cursorChar;    /* +0x24 */
    char  text[1];       /* +0x25  NUL-terminated, variable length */
} textPaintCommand_t;
ASSERT_OFF(textPaintCommand_t, x, 0x04);
ASSERT_OFF(textPaintCommand_t, y, 0x08);
ASSERT_OFF(textPaintCommand_t, font, 0x0c);
ASSERT_OFF(textPaintCommand_t, scale, 0x10);
ASSERT_OFF(textPaintCommand_t, color, 0x14);
ASSERT_OFF(textPaintCommand_t, fixedAdvance, 0x18);
ASSERT_OFF(textPaintCommand_t, style, 0x1c);
ASSERT_OFF(textPaintCommand_t, cursorPos, 0x20);
ASSERT_OFF(textPaintCommand_t, cursorChar, 0x24);
ASSERT_OFF(textPaintCommand_t, text, 0x25);


/* ---- R_LoadAsianFont  0x004DE850 ----  VERIFIED
 * Register the CJK atlas pages for the current language into g_asianFontPage*
 * and seed the shared g_asianGlyph metrics.  The retail cache guard compares
 * asianFontLoadedLanguage (init -1, never written), so a supported language
 * always reloads. */
void __cdecl R_LoadAsianFont( const char *shaderUsage )
{
  int         page;
  const char *prefix;
  int         pageCount;
  int         cellSize;
  char        shaderName[64];

  if ( !g_currentAsian )
  {
    asianFontLoaded = 0;
    return;
  }

  if ( asianFontLoaded && asianFontLoadedLanguage == cl_language->integer )
    return;

  switch ( cl_language->integer )
  {
    case 8:                                        /* Korean */
      g_asianFontPageDimension = 32;
      prefix = "kor";
      pageCount = 3;
      break;
    case 9:                                        /* Taiwanese */
      g_asianFontPageDimension = 64;
      pageCount = 4;
      prefix = "tai";
      break;
    case 10:                                       /* Japanese */
      g_asianFontPageDimension = 64;
      prefix = "jap";
      pageCount = 3;
      break;
    default:
      asianFontLoaded = 0;
      return;
  }

  g_asianFontPageCount = pageCount;
  for ( page = 0; page < g_asianFontPageCount; page++ )
  {
    Com_sprintf( shaderName, sizeof( shaderName ), "font/%s_%d_1024_%d.tga",
                 prefix, 1024 / g_asianFontPageDimension, page );
    g_asianFontPageHandles[page] = RE_RegisterShaderNoMip( shaderName, shaderUsage );
  }

  cellSize = 1024 / g_asianFontPageDimension;
  rendererAsianFontLastPageHalfHeight = 1;
  g_asianGlyph.horzBearing = 0;
  asianFontLoaded = 1;
  g_asianGlyph.height      = cellSize;
  g_asianGlyph.top         = cellSize;
  g_asianGlyph.imageWidth  = cellSize;
  g_asianGlyph.vertBearing = (float)cellSize;
  g_asianGlyph.imageHeight = cellSize;
  g_asianGlyph.xSkip       = g_asianGlyph.vertBearing;
}

/* ---- R_GetAsianCode  0x004DEA00 ----  VERIFIED
 * Collapse a CJK codepoint to its atlas index for the active language, or 0. */
int __cdecl R_GetAsianCode( int character )
{
  int code = 0;
  if ( asianFontLoaded )
  {
    switch ( cl_language->integer )
    {
      case 8:  code = Korean_CollapseKSC5601HangulCode( character ); break;
      case 9:  code = Taiwanese_CollapseBig5Code( character ); break;
      case 10: code = Japanese_CollapseShiftJISCode( character ); break;
    }
  }
  return code;
}

/* ---- R_GetCharacterGlyph  0x004DEA30 ----  VERIFIED
 * Resolve a codepoint to its glyph.  A Latin (or unmapped) code indexes the
 * font's own glyph table; a CJK code rewrites the shared g_asianGlyph atlas
 * coordinates (per-language layout) and returns a pointer to it.  `c` carries
 * the full two-byte code; only the Latin fallback masks it to a byte. */
glyphInfo_t *__cdecl R_GetCharacterGlyph( unsigned int c, fontInfo_t *font )
{
  int code;
  int page;
  int row;
  int column;
  int pageRows;
  int language;

  if ( asianFontLoaded )
  {
    language = cl_language->integer;
    code = R_GetAsianCode(c);

    if ( code )
    {
      page = code / ( g_asianFontPageDimension * g_asianFontPageDimension );
      if ( page > g_asianFontPageCount )
        page = 0;
      code -= page * ( g_asianFontPageDimension * g_asianFontPageDimension );
      column = code % g_asianFontPageDimension;
      row    = code / g_asianFontPageDimension;
      if ( page == g_asianFontPageCount - 1 && rendererAsianFontLastPageHalfHeight )
        pageRows = g_asianFontPageDimension / 2;
      else
        pageRows = g_asianFontPageDimension;

      switch ( language )
      {
        case 8:                                    /* Korean: normalized cell coords */
        default:
          g_asianGlyph.s  = (rfloat)column         / (rfloat)g_asianFontPageDimension;
          g_asianGlyph.t  = (rfloat)row            / (rfloat)pageRows;
          g_asianGlyph.s2 = (rfloat)( column + 1 ) / (rfloat)g_asianFontPageDimension;
          g_asianGlyph.t2 = (rfloat)( row + 1 )    / (rfloat)pageRows;
          break;
        case 9:                                    /* Taiwanese: half-texel inset */
          g_asianGlyph.s  = (rfloat)( ( 1024 / g_asianFontPageDimension ) * column + 1 ) / 1024.0f;
          g_asianGlyph.t  = (rfloat)( ( 1024 / pageRows ) * row + 1 )                     / 1024.0f;
          g_asianGlyph.s2 = (rfloat)( ( 1024 / g_asianFontPageDimension ) * ( column + 1 ) ) / 1024.0f;
          g_asianGlyph.t2 = (rfloat)( ( 1024 / pageRows ) * ( row + 1 ) )                     / 1024.0f;
          break;
        case 10:                                   /* Japanese: full-texel inset */
          g_asianGlyph.s  = (rfloat)( ( 1024 / g_asianFontPageDimension ) * column )           / 1024.0f;
          g_asianGlyph.t  = (rfloat)( ( 1024 / pageRows ) * row )                               / 1024.0f;
          g_asianGlyph.s2 = (rfloat)( ( 1024 / g_asianFontPageDimension ) * ( column + 1 ) - 1 ) / 1024.0f;
          g_asianGlyph.t2 = (rfloat)( ( 1024 / pageRows ) * ( row + 1 ) - 1 )                     / 1024.0f;
          break;
      }

      g_asianGlyph.glyph = g_asianFontPageHandles[page];
      return &g_asianGlyph;
    }
  }

  return &font->glyphs[c & 0xFF];
}

/* ---- R_GetGlyphHorizAdvance  0x004DEC50 ----  VERIFIED
 * Horizontal advance for a codepoint: the CJK atlas xSkip for a mapped CJK
 * code, else the glyph's own xSkip (falling back to '.' when it is zero). */
double __cdecl R_GetGlyphHorizAdvance( unsigned int c, fontInfo_t *font )
{
  unsigned int code;

  if ( asianFontLoaded )
  {
    switch ( cl_language->integer )
    {
      case 8:  code = Korean_CollapseKSC5601HangulCode( c ); break;
      case 9:  code = Taiwanese_CollapseBig5Code( c );        break;
      case 10: code = Japanese_CollapseShiftJISCode( c );     break;
      default: goto notAsian;
    }
    if ( code )
      return g_asianGlyph.xSkip;
  }
notAsian:
  if ( font->glyphs[c & 0xFF].xSkip != 0.0f )
    return font->glyphs[c & 0xFF].xSkip;
  return font->glyphs['.'].xSkip;
}

/* ---- R_GetAsianScale  0x004DECD0 ----  VERIFIED
 * Per-glyph scale for the CJK atlas relative to the font's 48-point base,
 * clamped so an adjusted glyph never renders below 16 screen pixels.  The
 * SCR_AdjustFrom640 result is intentionally discarded (retail retains the
 * independently computed scale). */
double __cdecl R_GetAsianScale( fontInfo_t *font, float scale )
{
  float asianScale;
  float adjustedHeight;

  asianScale = (rfloat)48.0f / font->glyphScale / g_asianGlyph.height;
  if ( scale > 0.25f )
    asianScale = ( ( (rfloat)scale - 0.25f ) * 0.4f + 0.25f ) / scale * asianScale;

  adjustedHeight = (rfloat)g_asianGlyph.imageHeight * asianScale * scale * font->glyphScale;
  ri.SCR_AdjustFrom640( 0, 0, 0, &adjustedHeight );

  if ( adjustedHeight < 16.0f )
    return FCAST( (rfloat)16.0f / adjustedHeight ) * asianScale;
  return asianScale;
}

/* ---- R_GetAsianGlyphHeight  0x004DED70 ----  VERIFIED */
double R_GetAsianGlyphHeight( void )
{
  return (double)g_asianGlyph.height;
}

/* ---- readInt  0x004DED80 ----  VERIFIED
 * Little-endian 32-bit read from the font-data cursor; advances it by 4.
 * RE_RegisterFont's calls are inlined. */
int readInt( void )
{
  int i = s_fontData[s_fontDataOffset] + ( s_fontData[s_fontDataOffset + 1] << 8 )
        + ( s_fontData[s_fontDataOffset + 2] << 16 ) + ( s_fontData[s_fontDataOffset + 3] << 24 );
  s_fontDataOffset += 4;
  return i;
}

/* ---- readFloat  0x004DEDC0 ----  VERIFIED */
typedef union {
  byte  fred[4];
  float ffred;
} poor;

float readFloat( void )
{
  poor me;

  me.fred[0] = s_fontData[s_fontDataOffset + 0];
  me.fred[1] = s_fontData[s_fontDataOffset + 1];
  me.fred[2] = s_fontData[s_fontDataOffset + 2];
  me.fred[3] = s_fontData[s_fontDataOffset + 3];
  s_fontDataOffset += 4;
  return me.ffred;
}

/* ---- RE_RegisterFont  0x004DEE00 ----  VERIFIED
 * Register a point-sized bitmap font (Q3's RE_RegisterFont): return a cached
 * copy if the .dat has been read before, otherwise read the fixed little-endian
 * glyph table through readInt/readFloat, resolve a shader for every glyph, and
 * cache the result.  `fontName` is the legacy renderer-export argument the
 * retail body never reads. */
void __cdecl RE_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font, const char *shaderUsage )
{
  int   i;
  void *faceData;
  char  name[1024];

  (void)fontName;

  if ( pointSize <= 0 )
    pointSize = 12;

  R_SyncRenderThread();
  R_LoadAsianFont( shaderUsage );

  if ( registeredFontCount >= 8 )
  {
    ri.Printf( 0, "RE_RegisterFont: Too many fonts registered already.\n" );
    return;
  }

  Com_sprintf( name, sizeof( name ), "fonts/fontImage_%i.dat", pointSize );
  for ( i = 0; i < registeredFontCount; i++ )
  {
    if ( !Q_stricmp( name, s_registeredFonts[i].name ) )
    {
      memcpy( font, &s_registeredFonts[i], sizeof( fontInfo_t ) );
      return;
    }
  }

  if ( ri.FS_ReadFile( name, 0 ) == (int)sizeof( fontInfo_t ) )
  {
    ri.FS_ReadFile( name, &faceData );
    s_fontDataOffset = 0;
    s_fontData = (byte *)faceData;
    for ( i = 0; i < 256; i++ )
    {
      font->glyphs[i].height      = readInt();
      font->glyphs[i].top         = readInt();
      font->glyphs[i].vertBearing = readFloat();
      font->glyphs[i].horzBearing = readFloat();
      font->glyphs[i].xSkip       = readFloat();
      font->glyphs[i].imageWidth  = readInt();
      font->glyphs[i].imageHeight = readInt();
      font->glyphs[i].s           = readFloat();
      font->glyphs[i].t           = readFloat();
      font->glyphs[i].s2          = readFloat();
      font->glyphs[i].t2          = readFloat();
      font->glyphs[i].glyph       = readInt();
      memcpy( font->glyphs[i].shaderName, &s_fontData[s_fontDataOffset], sizeof( font->glyphs[i].shaderName ) );
      s_fontDataOffset += sizeof( font->glyphs[i].shaderName );
    }
    font->glyphScale = readFloat();
    font->fontHeight = readFloat();
    memcpy( font->name, &s_fontData[s_fontDataOffset], sizeof( font->name ) );
    Q_strncpyz( font->name, name, sizeof( font->name ) );

    /* resolve a shader handle for glyphs 0..254 */
    for ( i = 0; i < 255; i++ )
      font->glyphs[i].glyph = RE_RegisterShaderNoMip( font->glyphs[i].shaderName, shaderUsage );

    memcpy( &s_registeredFonts[registeredFontCount++], font, sizeof( fontInfo_t ) );
    ri.FS_FreeFile( faceData );
    return;
  }

  ri.Printf( 0, "RE_RegisterFont: Unable to load font data %s\n", name );
}

/* ---- R_InitFreeType  0x004DF2E0 ----  VERIFIED  (returns 0, dead EAX) */
int R_InitFreeType( void )
{
  registeredFontCount = 0;
  asianFontLoaded = 0;
  return 0;
}

/* ---- R_DoneFreeType  0x004DF2F0 ----  VERIFIED  (returns 0, dead EAX) */
int R_DoneFreeType( void )
{
  registeredFontCount = 0;
  asianFontLoaded = 0;
  return 0;
}

/* ---- R_GetFontInfo  0x004DF300 ----  VERIFIED */
int __cdecl R_GetFontInfo( int fontHandle, float scale )
{
  return ri.GetFontInfo( fontHandle, *(int *)&scale );
}

/* ---- RE_Text_Width  0x004DF310 ----  VERIFIED
 * Width (in scaled pixels) of the widest line in `text`, up to maxChars glyphs.
 * Characters are read with SEH_ReadCharFromString (CJK codepoints when
 * g_currentAsian, advanced by asianScale), colour escapes (^0..^7) are skipped
 * and newlines reset the line.  With a nonzero fixedWidth every glyph counts as
 * one unit. */
int __cdecl RE_Text_Width( char *text, int fontHandle, float scale, float fixedWidth, int maxChars )
{
  fontInfo_t   *font;
  const char   *s = text;
  unsigned int  code;
  rfloat        advance;
  int           count;
  float         maxWidth;
  float         asianScale;
  float         scaleWidth;
  float         lineWidth;

  font = (fontInfo_t *)ri.GetFontInfo( fontHandle, *(int *)&scale );
  if ( fixedWidth != 0.0f )
    scaleWidth = fixedWidth;
  else
    scaleWidth = scale * font->glyphScale;

  asianScale = 1.0f;
  if ( g_currentAsian )
    asianScale = R_GetAsianScale( font, scale );

  lineWidth = 0.0f;
  maxWidth = 0.0f;
  if ( s )
  {
    if ( maxChars <= 0 )
      maxChars = 0x7FFFFFFF;
    count = 0;
    while ( *s )
    {
      if ( count >= maxChars )
        break;
      code = SEH_ReadCharFromString( (unsigned char **)&s, 0 );
      if ( code == 10 )                            /* '\n' */
      {
        lineWidth = 0.0f;
      }
      else if ( code == 94 && s && *s != 94 && *s >= 48 && *s <= 55 )
      {                                            /* '^' colour escape ^0..^7 */
        s++;
      }
      else
      {
        if ( fixedWidth != 0.0f )
        {
          lineWidth = lineWidth + 1.0f;
        }
        else
        {
          advance = R_GetGlyphHorizAdvance( code, font );
          if ( code > 0xFF )
            advance = advance * asianScale;
          lineWidth = advance + lineWidth;
        }
        if ( lineWidth > maxWidth )
          maxWidth = lineWidth;
        count++;
      }
    }
  }
  return (int)( maxWidth * scaleWidth );
}

/* ---- RE_Text_Height  0x004DF540 ----  VERIFIED */
int __cdecl RE_Text_Height( int fontHandle, float scale )
{
  fontInfo_t *font = (fontInfo_t *)ri.GetFontInfo( fontHandle, *(int *)&scale );
  return (int)( scale * font->fontHeight );
}

/* ---- RE_Text_Paint  0x004DF570 ----  VERIFIED */
void __cdecl RE_Text_Paint( float x, float y, int font, float scale, float *color, const char *text, int fixedAdvance, int limit, int style )
{
  RE_Text_PaintWithCursor( x, y, font, scale, color, text, -1, 0, fixedAdvance, limit, style );
}

/* ---- R_Text_GetConsoleString  0x004DF5B0 ----  VERIFIED
 * Unpack one drawable run of the console's 16-bit (attribute:char) word stream
 * into s_consoleTextBuffer, re-inserting ^-colour escapes when the attribute
 * changes and applying colour-value words (attrs 10/11/12) to `color`.  Stops
 * at an icon/embedded control word (13/16/17/18).  Returns 1 when it stopped on
 * such a control word, else 0. */
int __cdecl R_Text_GetConsoleString( int *out, float *color, int srcWords, int *count, int *next )
{
  int            dst;
  int            spacePos;
  int            i;
  int            code;
  int            curColor;
  int            result;
  unsigned short word;
  char           ch;

  *out = (int)s_consoleTextBuffer;
  if ( *count > 1023 )
    *count = 1023;
  s_consoleTextBuffer[0] = 0;
  dst = 0;
  spacePos = -1;
  curColor = 7;
  result = 0;

  for ( i = 0; i < *count; i++ )
  {
    word = ( (const unsigned short *)srcWords )[i];
    code = word >> 8;
    if ( code != curColor )
    {
      if ( code == 13 || code == 16 || code == 17 || code == 18 )
      {
        *next = srcWords + 2 * i;
        spacePos = -1;
        result = 1;
        break;
      }
      if ( code == 10 )
      {
        if ( color )
          color[0] = ( word & 0xFF ) * 0.0039215689f;
        continue;
      }
      if ( code == 11 )
      {
        if ( color )
          color[1] = ( word & 0xFF ) * 0.0039215689f;
        continue;
      }
      if ( code == 12 )
      {
        if ( color )
          color[2] = ( word & 0xFF ) * 0.0039215689f;
        continue;
      }
      s_consoleTextBuffer[dst] = 94;               /* '^' */
      curColor = code;
      dst++;
      s_consoleTextBuffer[dst++] = code + 48;
    }

    ch = (char)( (const unsigned short *)srcWords )[i];
    s_consoleTextBuffer[dst] = ch;
    if ( ch == 32 )                                /* ' ' -- remember last wrap point */
    {
      if ( spacePos == -1 )
        spacePos = dst;
    }
    else
    {
      spacePos = -1;
    }
    ++dst;
  }

  /* retail 0x004DF704/0x004DF70D store with no bound check; pointer stores keep modern /GS from adding a range-check fast-fail */
  *( s_consoleTextBuffer + dst ) = 0;
  if ( spacePos >= 0 )
    *( s_consoleTextBuffer + spacePos ) = 0;
  if ( i == *count )
    *next = 0;
  else
    *next = srcWords + 2 * i;
  *count -= i;
  return result;
}

/* ---- R_Text_GetConsoleIcon  0x004DF750 ----  VERIFIED
 * Decode an embedded console icon (attribute-18 run naming a shader) plus the
 * colour / size control words that precede it.  Outputs the icon width/height
 * (defaulting to scale*48), its shader handle, and (via colorOut) the running
 * colour; on an icon, *count is left as the number of unconsumed words. */
void __cdecl R_Text_GetConsoleIcon( int *cursor, int srcWords, int *count, float scale,
                                    float *widthOut, float *heightOut, int *shaderOut, float *colorOut )
{
  const unsigned short *src = (const unsigned short *)srcWords;
  rfloat defaultSize;
  int    i;
  short word;
  int    code;
  int    value;
  char  *namePtr;
  char   iconName[64];

  defaultSize = scale * 48.0f;
  *widthOut = defaultSize;
  *heightOut = defaultSize;
  *shaderOut = 0;
  memset( iconName, 0, sizeof( iconName ) );

  for ( i = 0; i < *count; i++ )
  {
    word  = src[i];
    code  = ( word >> 8 ) & 0xFF;
    value = word & 0xFF;
    switch ( code )
    {
      case 13:                                     /* colour */
        if ( colorOut )
          colorOut[0] = value * 0.0039215689f;
        *cursor += 2;
        break;
      case 14:
        if ( colorOut )
          colorOut[1] = value * 0.0039215689f;
        *cursor += 2;
        break;
      case 15:
        if ( colorOut )
          colorOut[2] = value * 0.0039215689f;
        *cursor += 2;
        break;
      case 16:                                     /* scale icon width */
        *widthOut = FCAST( value * 0.03125f ) * *widthOut;
        *cursor += 2;
        break;
      case 17:                                     /* scale icon height */
        *heightOut = FCAST( value * 0.03125f ) * *heightOut;
        *cursor += 2;
        break;
      case 18:                                     /* icon shader name run */
        if ( code == 18 )                          /* retail re-tests the code here */
        {
          namePtr = iconName;
          do
          {
            if ( !value )
              break;
            if ( i >= *count - 1 )
              break;
            *namePtr++ = (char)value;
            *cursor += 2;
            i++;
            word  = src[i];
            value = word & 0xFF;
          }
          while ( ( word & 0xFF00 ) == 0x1200 );
        }
        *shaderOut = RE_RegisterShaderNoMip( iconName, 5 );
        *count -= i;
        return;
    }
  }
}

/* ---- R_DrawStrlen  0x004DF8E0 ----  VERIFIED
 * Visible length of `str`, skipping ^0..^7 colour escapes (an escape also
 * swallows the following character). */
int R_DrawStrlen( const char *str )
{
  const char *s = str;
  int count = 0;

  while ( *s )
  {
    if ( s[0] == '^' && s[1] && s[1] != '^' && s[1] >= '0' && s[1] <= '7' )
      s += 2;
    else
      count++;
    s++;
  }
  return count;
}

/* ---- RE_Text_ConsoleWidth  0x004DF920 ----  VERIFIED
 * Total width of a console line, summing each drawable run (RE_Text_Width, or
 * R_DrawStrlen*fixedWidth) plus embedded icon widths. */
int __cdecl RE_Text_ConsoleWidth( int text, int font, float scale, float fixedWidth, int count )
{
  int   next;
  int   hasIcon;
  int   strPtr;
  float iconWidth;
  float iconHeight;
  int   iconShader;
  float total;

  total = 0.0f;
  next = text;
  if ( next )
  {
    do
    {
      hasIcon = R_Text_GetConsoleString( &strPtr, 0, next, &count, &next );
      if ( strPtr && *(char *)strPtr )
      {
        if ( fixedWidth != 0.0f )
          total += (rfloat)R_DrawStrlen( (const char *)strPtr ) * fixedWidth;
        else
          total += RE_Text_Width( (char *)strPtr, font, scale, fixedWidth, count );
      }
      if ( hasIcon )
      {
        R_Text_GetConsoleIcon( &next, next, &count, scale, &iconWidth, &iconHeight, &iconShader, 0 );
        total += iconWidth;
      }
    }
    while ( next );
  }

  return (int)total;
}

/* ---- R_Text_PaintConsoleIcon  0x004DFA10 ----  VERIFIED
 * Draw one embedded console icon after mapping its 640-relative rect to screen
 * space.  Dead EAX = the raw bits of s2 (1.0f) from RE_StretchPic. */
unsigned int __cdecl R_Text_PaintConsoleIcon( float x, float y, float w, float h, int shader )
{
  ri.SCR_AdjustFrom640( &x, &y, &w, &h );
  return RE_StretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, shader );
}

/* ---- RE_Text_ConsolePaint  0x004DFA60 ----  VERIFIED
 * Paint a console line: for each run, draw the text, advance by its width, then
 * draw and advance past any embedded icon (centred on the baseline). */
int __cdecl RE_Text_ConsolePaint( float x, float y, int font, float scale, int *colorIn, int text, float fixedWidth, int count, int style )
{
  int    color[4];
  int    strPtr;
  int    hasIcon;
  char  *str;
  float  penX;
  float  iconWidth, iconHeight;
  int    iconShader;
  int    cursor;

  color[0] = colorIn[0];
  color[1] = colorIn[1];
  color[2] = colorIn[2];
  color[3] = colorIn[3];
  penX = 0.0f;
  cursor = text;
  if ( cursor )
  {
    do
    {
      hasIcon = R_Text_GetConsoleString( &strPtr, (float *)color, cursor, &count, &cursor );
      str = (char *)strPtr;

      if ( str && *str )
      {
        RE_Text_PaintWithCursor( penX + x, y, font, scale, (float *)color, str, -1, 0, *(int *)&fixedWidth, 0, style );
        if ( hasIcon )
          penX += RE_Text_Width( str, font, scale, fixedWidth, count );
      }

      if ( hasIcon )
      {
        R_Text_GetConsoleIcon( &cursor, cursor, &count, scale, &iconWidth, &iconHeight, &iconShader, (float *)color );
        RE_SetColor( (float *)color );
        /* the redundant parentheses are VC7 scheduling weight (retail's x87/push interleave) */
        R_Text_PaintConsoleIcon( penX + x, ( y - ( ( ( scale * 38.400002f ) + iconHeight ) * 0.5f ) ),
                                 iconWidth, iconHeight, iconShader );
        penX += iconWidth;
      }
    }
    while ( cursor );
  }
  return cursor;
}

/* ---- RE_Text_PaintWithCursor  0x004DFC10 ----  VERIFIED
 * Queue a text-draw render command; limit clamps the drawn length.  cursorPos/cursorChar place an
 * optional cursor glyph.  Colour is packed to RGBA bytes; the text is copied and
 * NUL-terminated into the command's trailing buffer. */
void __cdecl RE_Text_PaintWithCursor( float x, float y, int font, float scale, float *color,
                                      const char *text, int cursorPos, char cursorChar, int fixedAdvance, int limit, int style )
{
  textPaintCommand_t *cmd;
  int                 len;

  if ( !text )
    return;

  len = strlen( text );
  if ( limit > 0 && len > limit )
    len = limit;

  cmd = (textPaintCommand_t *)R_GetCommandBuffer( ( len + 41 ) & ~3 );
  if ( !cmd )
    return;

  cmd->commandId    = 6;                           /* RC_TEXT_PAINT */
  cmd->x            = x;
  cmd->y            = y;
  cmd->font         = font;
  cmd->scale        = scale;
  cmd->color[0]     = (byte)( (rfloat)*color   * 255.0f );
  cmd->color[1]     = (byte)( (rfloat)color[1] * 255.0f );
  cmd->color[2]     = (byte)( (rfloat)color[2] * 255.0f );
  cmd->color[3]     = (byte)( (rfloat)color[3] * 255.0f );
  cmd->fixedAdvance = fixedAdvance;
  cmd->style        = style;
  cmd->cursorPos    = cursorPos;
  cmd->cursorChar   = cursorChar;
  memcpy( cmd->text, text, len );
  cmd->text[len]    = 0;
}
