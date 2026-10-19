#ifndef __UI_SHARED_H__
#define __UI_SHARED_H__

#include "../universal/q_shared.h"

#define UI_ASSERT_SIZE( type, bytes ) COD1_ASSERT_SIZE( type, bytes )

#ifndef __CG_PUBLIC_H__
typedef int qhandle_t;
typedef int sfxHandle_t;
#endif

#define MAX_QPATH               64
#define MAX_STRING_CHARS        1024

//
// these are the key numbers that should be passed to KeyEvent
//
#define K_TAB               9
#define K_ENTER             13
#define K_ESCAPE            27
#define K_SPACE             32
#define K_BACKSPACE         127
#define K_UPARROW           154
#define K_DOWNARROW         155
#define K_LEFTARROW         156
#define K_RIGHTARROW        157
#define K_ALT               158
#define K_CTRL              159
#define K_SHIFT             160
#define K_INS               161
#define K_DEL               162
#define K_PGDN              163
#define K_PGUP              164
#define K_HOME              165
#define K_END               166
#define K_F1                167
#define K_F11               177
#define K_F12               178
#define K_KP_HOME           182
#define K_KP_UPARROW        183
#define K_KP_PGUP           184
#define K_KP_LEFTARROW      185
#define K_KP_5              186
#define K_KP_RIGHTARROW     187
#define K_KP_END            188
#define K_KP_DOWNARROW      189
#define K_KP_PGDN           190
#define K_KP_ENTER          191
#define K_KP_INS            192
#define K_KP_DEL            193
#define K_MOUSE1            200
#define K_MOUSE2            201
#define K_MOUSE3            202
#define K_MOUSE4            203
#define K_MOUSE5            204
#define K_MWHEELDOWN        205
#define K_MWHEELUP          206

#define ITEM_TYPE_TEXT              0   // simple text
#define ITEM_TYPE_BUTTON            1   // button, basically text with a border
#define ITEM_TYPE_RADIOBUTTON       2   // toggle button, may be grouped
#define ITEM_TYPE_CHECKBOX          3   // check box
#define ITEM_TYPE_EDITFIELD         4   // editable text, associated with a cvar
#define ITEM_TYPE_COMBO             5   // drop down list
#define ITEM_TYPE_LISTBOX           6   // scrollable list
#define ITEM_TYPE_MODEL             7   // model
#define ITEM_TYPE_OWNERDRAW         8   // owner draw, name specs what it is
#define ITEM_TYPE_NUMERICFIELD      9   // editable text, associated with a cvar
#define ITEM_TYPE_SLIDER            10  // mouse speed, volume, etc.
#define ITEM_TYPE_YESNO             11  // yes no cvar setting
#define ITEM_TYPE_MULTI             12  // multiple list setting, enumerated
#define ITEM_TYPE_BIND              13  // multiple list setting, enumerated
#define ITEM_TYPE_MENUMODEL         14  // special menu model
#define ITEM_TYPE_UPREDITFIELD      15  // editable text, forced to upper case

#define ITEM_ALIGN_LEFT             0   // left alignment
#define ITEM_ALIGN_CENTER           1   // center alignment
#define ITEM_ALIGN_RIGHT            2   // right alignment
#define ITEM_ALIGN_CENTER_ADJUSTED  3   // center the text and value together

// list box element types
#define LISTBOX_TEXT                0
#define LISTBOX_IMAGE               1

#define ITEM_TEXTSTYLE_NORMAL       0   // normal text
#define ITEM_TEXTSTYLE_BLINK        1   // fast blinking

// The menu code needs to get both key and char events, but
// to avoid duplicating the paths, the char events are just
// distinguished by or'ing in K_CHAR_FLAG (ugly)
#define K_CHAR_FLAG                 0x400

#define PULSE_DIVISOR               75
#define BLINK_DIVISOR               256

#define WINDOW_STYLE_EMPTY          0   // no background
#define WINDOW_STYLE_FILLED         1   // filled with background color
#define WINDOW_STYLE_GRADIENT       2   // gradient bar based on background color
#define WINDOW_STYLE_SHADER         3   // gradient bar based on background color
#define WINDOW_STYLE_TEAMCOLOR      4   // team color
#define WINDOW_STYLE_CINEMATIC      5   // cinematic
#define WINDOW_STYLE_SHADER_NOTINT  6   // shader, drawn only when background is set

#define WINDOW_BORDER_NONE          0   // no border
#define WINDOW_BORDER_FULL          1   // full border based on border color ( single pixel )
#define WINDOW_BORDER_HORZ          2   // horizontal borders only
#define WINDOW_BORDER_VERT          3   // vertical borders only
#define WINDOW_BORDER_KCGRADIENT    4   // horizontal border using the gradient bars

#define MAX_MENUNAME        32
#define MAX_ITEMTEXT        64
#define MAX_ITEMACTION      64
#define MAX_MENUDEFFILE     4096
#define MAX_MENUFILE        32768
#define MAX_MENUS           64
#define MAX_MENUITEMS       128 // JPW NERVE q3ta was 96
#define MAX_COLOR_RANGES    10
#define MAX_OPEN_MENUS      16

#define WINDOW_MOUSEOVER        0x00000001 // mouse is over it, non exclusive
#define WINDOW_HASFOCUS         0x00000002 // has cursor focus, exclusive
#define WINDOW_VISIBLE          0x00000004 // is visible
#define WINDOW_GREY             0x00000008 // is visible but grey ( non-active )
#define WINDOW_DECORATION       0x00000010 // for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT        0x00000020 // fading out, non-active
#define WINDOW_FADINGIN         0x00000040 // fading in
#define WINDOW_MOUSEOVERTEXT    0x00000080 // mouse is over it, non exclusive
#define WINDOW_INTRANSITION     0x00000100 // window is in transition
#define WINDOW_FORECOLORSET     0x00000200 // forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL       0x00000400 // for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW     0x00000800 // mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW    0x00001000 // mouse is over right/down arrow
#define WINDOW_LB_THUMB         0x00002000 // mouse is over thumb
#define WINDOW_LB_PGUP          0x00004000 // mouse is over page up
#define WINDOW_LB_PGDN          0x00008000 // mouse is over page down
#define WINDOW_ORBITING         0x00010000 // item is in orbit
#define WINDOW_OOB_CLICK        0x00020000 // close on out of bounds click
#define WINDOW_WRAPPED          0x00040000 // manually wrap text
#define WINDOW_AUTOWRAPPED      0x00080000 // auto wrap text
#define WINDOW_FORCED           0x00100000 // forced open
#define WINDOW_POPUP            0x00200000 // popup
#define WINDOW_BACKCOLORSET     0x00400000 // backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE     0x00800000 // visibility timing ( NOT implemented )
#define WINDOW_IGNORE_HUDALPHA  0x01000000  // window will apply cg_hudAlpha value to colors unless this flag is set

#define WINDOW_TEXTCVARSHORT    0x00200000  // items only: cvar text stops at the first '.'

// CGAME cursor type bits
#define CURSOR_NONE             0x00000001
#define CURSOR_ARROW            0x00000002
#define CURSOR_SIZER            0x00000004

#ifdef CGAMEDLL
#define MEM_POOL_SIZE       ( 128 * 1024 )
#else
#define MEM_POOL_SIZE       ( 1024 * 1024 )
#endif
#ifdef CGAMEDLL
#define STRING_POOL_SIZE    ( 128 * 1024 )
#else
#define STRING_POOL_SIZE    ( 384 * 1024 )
#endif

#define HASH_TABLE_SIZE     2048
#define KEYWORDHASH_SIZE    512

#define MAX_STRING_HANDLES  4096
#define MAX_SCRIPT_ARGS     12
#define MAX_EDITFIELD       256

// keep: int, not 16.0
#define SCROLLBAR_SIZE      16
// keep: float, not double
#define SLIDER_WIDTH        96.0f
#define SLIDER_HEIGHT       16.0f
#define SLIDER_THUMB_WIDTH  10.0f
#define SLIDER_THUMB_HEIGHT 20.0f

#define CVAR_ENABLE     0x00000001
#define CVAR_DISABLE    0x00000002
#define CVAR_SHOW       0x00000004
#define CVAR_HIDE       0x00000008

#define RANGETYPE_ABSOLUTE  0
#define RANGETYPE_RELATIVE  1

// font support
typedef struct {
	int     height;
	int     top;
	float   vertBearing;
	float   horzBearing;
	float   xSkip;
	int     imageWidth;
	int     imageHeight;
	float   s;
	float   t;
	float   s2;
	float   t2;
	qhandle_t glyph;
	char    shaderName[32];
} glyphInfo_t;
UI_ASSERT_SIZE( glyphInfo_t, 0x50 );

typedef struct {
	glyphInfo_t glyphs[256];
	float       glyphScale;
	float       fontHeight;
	char        name[64];
} fontInfo_t;
UI_ASSERT_SIZE( fontInfo_t, 0x5048 );

#ifndef __CG_PUBLIC_H__
typedef struct {
	int     x;
	int     y;
	int     width;
	int     height;
	float   fov_x;
	float   fov_y;
	vec3_t  vieworg;
	vec3_t  viewaxis[3];            // transformation matrix
	int     time;                   // time in milliseconds for shader effects and other time dependent rendering issues
	int     rdflags;                // RDF_NOWORLDMODEL, etc
} refdef_t;
UI_ASSERT_SIZE( refdef_t, 0x50 );

typedef struct {
	int     reType;
	int     renderfx;
	qhandle_t hModel;               // opaque type outside refresh
	vec3_t  lightingOrigin;         // so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float   shadowPlane;            // projection shadows go here, stencils go slightly lower
	vec3_t  axis[3];                // rotation vectors
	int     nonNormalizedAxes;      // axis are not normalized, i.e. they have scale
	vec3_t  origin;                 // also used as MODEL_BEAM's "from"
	int     frame;                  // also used as MODEL_BEAM's diameter
	vec3_t  oldorigin;              // also used as MODEL_BEAM's "to"
	int     oldframe;
	float   backlerp;               // 0.0 = current, 1.0 = old
	qhandle_t customShader;         // use one image for the entire thing
	byte    shaderRGBA[4];          // colors used by rgbgen entity shaders
	float   shaderTexCoord[2];      // texture coordinates used by tcMod entity modifiers
	float   shaderTime;             // subtracted from refdef time to control effect start times
	float   radius;
	float   rotation;
	vec3_t  fireRiseDir;
	void    *dobj;
	void    *owner;
	void    *staticModel;
} refEntity_t;
UI_ASSERT_SIZE( refEntity_t, 0x9C );
COD1_STATIC_ASSERT( offsetof( refEntity_t, dobj ) == 0x90, "refEntity_t.dobj" );

typedef struct pc_token_s {
	int     type;
	int     subtype;
	int     intvalue;
	float   floatvalue;
	char    string[1024];
} pc_token_t;
UI_ASSERT_SIZE( pc_token_t, 1040 );

#endif  /* __CG_PUBLIC_H__ */

#define RDF_NOWORLDMODEL    1
#define MAX_TOKEN_CHARS     1024
#define TT_NUMBER           3

typedef struct {
	const char *command;
	const char *args[MAX_SCRIPT_ARGS];
} scriptDef_t;

typedef struct {
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
} rectDef_t;
UI_ASSERT_SIZE( rectDef_t, 16 );

typedef rectDef_t Rectangle;

// FIXME: do something to separate text vs window stuff
typedef struct {
	Rectangle   rect;                   // client coord rectangle
	Rectangle   rectClient;             // screen coord rectangle
	const char  *name;
	const char  *model;
	const char  *group;                 // if it belongs to a group
	const char  *cinematicName;         // cinematic name
	int         cinematic;              // cinematic handle
	int         style;
	int         border;
	int         ownerDraw;              // ownerDraw style
	int         ownerDrawFlags;         // show flags for ownerdraw items
	float       borderSize;
	int         flags;                  // visible, focus, mouseover, cursor
	Rectangle   rectEffects;            // for various effects
	Rectangle   rectEffects2;           // for various effects
	int         offsetTime;             // time based value for various effects
	int         nextTime;               // time next effect should cycle
	vec4_t      foreColor;              // text color
	vec4_t      backColor;              // border color
	vec4_t      borderColor;            // border color
	vec4_t      outlineColor;           // border color
	qhandle_t   background;             // background asset
} windowDef_t;
UI_ASSERT_SIZE( windowDef_t, 184 );

typedef windowDef_t Window;

typedef struct {
	vec4_t  color;
	int     type;
	float   low;
	float   high;
} colorRangeDef_t;
UI_ASSERT_SIZE( colorRangeDef_t, 28 );

#define MAX_LB_COLUMNS 16

typedef struct columnInfo_s {
	int pos;
	int width;
	int maxChars;
} columnInfo_t;

typedef struct listBoxDef_s {
	int         startPos;
	int         endPos;
	int         drawPadding;
	int         cursorPos;
	float       elementWidth;
	float       elementHeight;
	int         elementStyle;
	int         numColumns;
	columnInfo_t columnInfo[MAX_LB_COLUMNS];
	const char  *doubleClick;
	qboolean    notselectable;
} listBoxDef_t;
UI_ASSERT_SIZE( listBoxDef_t, 232 );

typedef struct editFieldDef_s {
	float   minVal;                 //	edit field limits
	float   maxVal;                 //
	float   defVal;                 //
	float   range;                  //
	int     maxChars;               // for edit fields
	int     maxCharsGotoNext;       // move to the next field when full
	int     maxPaintChars;          // for edit fields
	int     paintOffset;            //
} editFieldDef_t;
UI_ASSERT_SIZE( editFieldDef_t, 32 );

#define MAX_MULTI_CVARS 32

typedef struct multiDef_s {
	const char  *cvarList[MAX_MULTI_CVARS];
	const char  *cvarStr[MAX_MULTI_CVARS];
	float       cvarValue[MAX_MULTI_CVARS];
	int         count;
	qboolean    strDef;
} multiDef_t;
UI_ASSERT_SIZE( multiDef_t, 392 );

typedef struct modelDef_s {
	int     angle;
	vec3_t  origin;
	float   fov_x;
	float   fov_y;
	int     rotationSpeed;

	int     animated;
	int     startframe;
	int     numframes;
	int     loopframes;
	int     fps;

	int     frame;
	int     oldframe;
	float   backlerp;
	int     frameTime;
} modelDef_t;
UI_ASSERT_SIZE( modelDef_t, 64 );

struct menuDef_s;

typedef struct itemDef_s {
	Window      window;                 // common positional, border, style, layout info
	Rectangle   textRect;               // rectangle the text ( if any ) consumes
	int         type;                   // text, button, radiobutton, checkbox, textfield, listbox, combo
	int         alignment;              // left center right
	int         font;                   // (SA)
	int         textalignment;          // ( optional ) alignment for text within rect based on text width
	float       textalignx;             // ( optional ) text alignment x coord
	float       textaligny;             // ( optional ) text alignment x coord
	float       textscale;              // scale percentage from 72pts
	int         textStyle;              // ( optional ) style, normal and shadowed are it for now
	const char  *text;                  // display text
	struct menuDef_s *parent;           // menu owner
	qhandle_t   asset;                  // handle to asset
	const char  *mouseEnterText;        // mouse enter script
	const char  *mouseExitText;         // mouse exit script
	const char  *mouseEnter;            // mouse enter script
	const char  *mouseExit;             // mouse exit script
	const char  *action;                // select script
	const char  *onAccept;              // NERVE - SMF - run when the users presses the enter key
	const char  *onFocus;               // select script
	const char  *leaveFocus;            // select script
	const char  *cvar;                  // associated cvar
	const char  *cvarTest;              // associated cvar for enable actions
	const char  *enableCvar;            // enable, disable, show, or hide based on value, this can contain a list
	int         cvarFlags;              //	what type of action to take on cvarenables
	sfxHandle_t focusSound;
	int         numColors;              // number of color ranges
	colorRangeDef_t colorRanges[MAX_COLOR_RANGES];
	int         colorRangeType;         // either
	float       special;                // used for feeder id's etc.. diff per type
	int         cursorPos;              // cursor position in characters
	void        *typeData;              // type specific data ptr's
	int         imageTrack;             // image track for the item's assets
} itemDef_t;
UI_ASSERT_SIZE( itemDef_t, 600 );

typedef struct menuDef_s {
	Window      window;
	const char  *font;                  // font
	qboolean    fullScreen;             // covers entire screen
	int         itemCount;              // number of items;
	int         fontIndex;
	int         cursorItem;             // which item as the cursor
	int         fadeCycle;
	float       fadeClamp;
	float       fadeAmount;
	float       fadeInAmount;
	const char  *onOpen;                // run when the menu is first opened
	const char  *onClose;               // run when the menu is closed
	const char  *onESC;                 // run when the menu is closed
	const char  *onKey[255];            // NERVE - SMF - execs commands when a key is pressed
	const char  *soundName;             // background loop sound for menu
	int         imageTrack;             // image track, copied to the items
	vec4_t      focusColor;             // focus color for items
	vec4_t      disableColor;           // focus color for items
	itemDef_t   *items[MAX_MENUITEMS];  // items this menu contains
} menuDef_t;
UI_ASSERT_SIZE( menuDef_t, 1804 );

typedef struct {
	const char  *fontStr;
	const char  *cursorStr;
	const char  *gradientStr;
	fontInfo_t  textFont;
	fontInfo_t  smallFont;
	fontInfo_t  bigFont;
	fontInfo_t  extraBigFont;
	fontInfo_t  boldFont;
	fontInfo_t  consoleFont;
	qhandle_t   cursor;
	qhandle_t   gradientBar;
	qhandle_t   scrollBarArrowUp;
	qhandle_t   scrollBarArrowDown;
	qhandle_t   scrollBarArrowLeft;
	qhandle_t   scrollBarArrowRight;
	qhandle_t   scrollBar;
	qhandle_t   scrollBarThumb;
	qhandle_t   buttonMiddle;
	qhandle_t   buttonInside;
	qhandle_t   solidBox;
	qhandle_t   sliderBar;
	qhandle_t   sliderThumb;
	// keep: sound alias indices, though they are played back by name
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	float       fadeClamp;
	int         fadeCycle;
	float       fadeAmount;
	float       fadeInAmount;
	float       shadowX;
	float       shadowY;
	vec4_t      shadowColor;
	float       shadowFadeClamp;
	qboolean    fontRegistered;
} cachedAssets_t;

typedef struct {
	const char *name;
	void ( *handler )( itemDef_t *item, char **args );
} commandDef_t;

// cgame declares its own glconfig_t in cg_public.h
#ifndef CGAMEDLL
typedef struct glconfig_t {
	char   *renderer_string;
	char   *vendor_string;
	char   *version_string;
	char   *extensions_string;
	char   *wextensions_string;
	int     maxTextureSize;
	int     maxActiveTextures;
	int     maxLights;
	int     colorBits;
	int     depthBits;
	int     stencilBits;
	int     deviceSupportsGamma;
	int     maxAnisotropy;
	int     maxTextureFilterAnisotropy;
	int     textureEnvAddAvailable;
	int     textureCubeMap;
	int     textureEnvCombine;
	int     textureEnvDot3;
	int     ARBVertexBufferObject;
	int     ARBVertexProgram;
	int     rescaleNormal;
	int     NVFogAvailable;
	int     NVFogMode;
	int     NVVertexArrayRange;
	int     NVFenceAvailable;
	int     NVRegisterCombiners;
	int     NVTextureShader;
	int     maxPNTrianglesTessellationLevel;
	int     pnTrianglesNormalMode;
	int     pnTrianglesPointMode;
	int     ATIVertexArrayObject;
	int     ATIElementArray;
	int     ATIFragmentShader;
	int     vidWidth;
	int     vidHeight;
	float   windowAspect;
	int     displayFrequency;
	int     isFullscreen;
	int     stereoEnabled;
	qboolean smpActive;         // dual processor
} glconfig_t;
UI_ASSERT_SIZE( glconfig_t, 160 );
#endif

// setFont, setOverstrikeMode, getOverstrikeMode, fileText, feederAddItem,
// getAutoUpdate, runningGame and executeText are left NULL by cgame
typedef struct {
	qhandle_t   ( *registerShaderNoMip )( const char *p, int imageTrack );
	void        ( *setColor )( const vec4_t v );
	void        ( *drawHandlePic )( float x, float y, float w, float h, qhandle_t asset );
	void        ( *drawStretchPic )( float x, float y, float w, float h,
									 float s1, float t1, float s2, float t2,
									 qhandle_t hShader );
	void        ( *drawText )( float x, float y, int font, float scale, const vec4_t color,
							   const char *text, float adjust, int limit, int style );
	int         ( *textWidth )( const char *text, int font, float scale, int limit );
	int         ( *textHeight )( int font, float scale );
	const char  *( *translateReference )( const char *reference );
	const char  *( *safeTranslateString )( const char *reference );
	const char  *( *translatedMessage )( const char *msg, const char *context );
	void        ( *setFont )( int font );
	qhandle_t   ( *registerModel )( const char *p, int imageTrack );
	void        ( *modelBounds )( qhandle_t model, vec3_t mins, vec3_t maxs );
	void        ( *fillRect )( float x, float y, float w, float h, const vec4_t color );
	void        ( *drawRect )( float x, float y, float w, float h, float size,
							   const vec4_t color );
	void        ( *drawSides )( float x, float y, float w, float h, float size );
	void        ( *drawTopBottom )( float x, float y, float w, float h, float size );
	void        ( *clearScene )( void );
	void        ( *addRefEntityToScene )( const refEntity_t *re );
	void        ( *renderScene )( const refdef_t *fd );
	void        ( *registerFont )( const char *pFontname, int pointSize, fontInfo_t *font,
								   int imageTrack );
	void        ( *ownerDrawItem )( float x, float y, float w, float h,
									float text_x, float text_y,
									int ownerDraw, int ownerDrawFlags, int align,
									float special, int font, float scale,
									vec4_t color, qhandle_t shader, int textStyle );
	float       ( *getValue )( int ownerDraw, int type );
	qboolean    ( *ownerDrawVisible )( int flags );
	void        ( *runScript )( char **p );
	void        ( *getTeamColor )( vec4_t *color );
	void        ( *getCVarString )( const char *cvar, char *buffer, int bufsize );
	float       ( *getCVarValue )( const char *cvar );
	void        ( *setCVar )( const char *cvar, const char *value );
	const char  *( *configString )( int index );
	void        ( *drawTextWithCursor )( float x, float y, int font, float scale,
										 const vec4_t color, const char *text,
										 int cursorPos, char cursor, int limit,
										 int style );
	void        ( *setOverstrikeMode )( qboolean b );
	qboolean    ( *getOverstrikeMode )( void );
	void        ( *playClientSoundAliasByName )( const char *name );
	qboolean    ( *ownerDrawHandleKey )( int ownerDraw, int flags, float *special, int key );
	int         ( *feederCount )( float feederID );
	const char  *( *feederItemText )( float feederID, int index, int column,
									  qhandle_t *handle );
	const char  *( *fileText )( const char *fileName );
	qhandle_t   ( *feederItemImage )( float feederID, int index );
	void        ( *feederSelection )( float feederID, int index );
	void        ( *feederAddItem )( float feederID, const char *name, int index );
	void        ( *getAutoUpdate )( void );
	qboolean    ( *runningGame )( void );
	void        ( *keynumToStringBuf )( int keynum, char *buf, int buflen );
	void        ( *getBindingBuf )( int keynum, char *buf, int buflen );
	void        ( *setBinding )( int keynum, const char *binding );
	void        ( *executeText )( int exec_when, const char *text );
	void        ( *Error )( int level, const char *error, ... );
	void        ( *Print )( const char *msg, ... );
	void        ( *Pause )( qboolean b );
	int         ( *ownerDrawWidth )( int ownerDraw, int font, float scale );
	// returns the index of a sound alias
	int         ( *registerSound )( const char *alias );
	int         ( *playCinematic )( const char *name, float x, float y, float w, float h );
	void        ( *stopCinematic )( int handle );
	void        ( *drawCinematic )( int handle, float x, float y, float w, float h );
	void        ( *runCinematicFrame )( int handle );

	float       yscale;
	float       xscale;
	float       bias;
	int         realTime;
	int         frameTime;
	int         cursorx;
	int         cursory;
	qboolean    debug;

	cachedAssets_t Assets;

	// only the ui reads glconfig
#ifdef CGAMEDLL
	byte        glconfig[160];
#else
	glconfig_t  glconfig;
#endif
	qhandle_t   whiteShader;
	qhandle_t   gradientImage;
	qhandle_t   cursor;
	float       FPS;
} displayContextDef_t;
UI_ASSERT_SIZE( displayContextDef_t, 0x1E3E0 );

typedef struct keywordHash_s {
	char        *keyword;
	qboolean    ( *func )( itemDef_t *item, int handle );
	struct keywordHash_s *next;
} keywordHash_t;
UI_ASSERT_SIZE( keywordHash_t, 12 );

typedef struct {
	char    *command;
	int     id;
	int     defaultbind1;
	int     defaultbind2;
	int     bind1;
	int     bind2;
} bind_t;
UI_ASSERT_SIZE( bind_t, 24 );

extern displayContextDef_t *DC;
extern qboolean g_waitingForKey;
extern qboolean g_editingField;
extern itemDef_t *g_editItem;
extern int menuCount;

/*
 * ui_shared.c
 */
void        *UI_Alloc( int size );
void        UI_InitMemory( void );
qboolean    UI_OutOfMemory( void );
const char  *String_Alloc( const char *p );
void        String_Report( void );
void        String_Init( void );
void        PC_SourceWarning( int handle, char *format, ... );
void        PC_SourceError( int handle, char *format, ... );
void        LerpColor( vec4_t a, vec4_t b, vec4_t c, float t );
qboolean    Float_Parse( char **p, float *f );
qboolean    PC_Float_Parse( int handle, float *f );
qboolean    Color_Parse( char **p, vec4_t *c );
qboolean    PC_Color_Parse( int handle, vec4_t *c );
qboolean    Int_Parse( char **p, int *i );
qboolean    PC_Int_Parse( int handle, int *i );
qboolean    Rect_Parse( char **p, rectDef_t *r );
qboolean    PC_Rect_Parse( int handle, rectDef_t *r );
qboolean    String_Parse( char **p, const char **out );
qboolean    PC_String_Parse( int handle, const char **out );
qboolean    PC_Char_Parse( int handle, char *out );
qboolean    PC_Script_Parse( int handle, const char **out );
void        Init_Display( displayContextDef_t *dc );
void        Window_Init( Window *w );
void        Window_Paint( Window *w, float fadeAmount, float fadeInAmount, float fadeClamp, float fadeCycle );
void        Fade( int *flags, float *f, float clamp, int *nextTime, int offsetTime,
				  qboolean bFlags, float fadeAmount, float fadeInAmount );
void        GradientBar_Paint( rectDef_t *rect, vec4_t color );
void        Item_SetScreenCoords( itemDef_t *item, float x, float y );
void        Item_UpdatePosition( itemDef_t *item );
qboolean    IsVisible( int flags );
void        ToWindowCoords( float *x, float *y, windowDef_t *window );
void        Item_TextColor( itemDef_t *item, vec4_t *newColor );
void        Item_Text_Paint( itemDef_t *item );
void        Item_Text_Wrapped_Paint( itemDef_t *item, const char *text, vec4_t color );
void        Item_Text_AutoWrapped_Paint( itemDef_t *item, const char *text, vec4_t color );
void        Item_TextField_Paint( itemDef_t *item );
void        Item_YesNo_Paint( itemDef_t *item );
void        Item_Multi_Paint( itemDef_t *item );
void        Item_Slider_Paint( itemDef_t *item );
void        Item_Bind_Paint( itemDef_t *item );
void        Item_Model_Paint( itemDef_t *item );
void        Item_Image_Paint( itemDef_t *item );
void        Item_ListBox_Paint( itemDef_t *item );
void        Item_OwnerDraw_Paint( itemDef_t *item );
void        adjustfrom640( float *x, float *y, float *w, float *h );
int         Item_ListBox_OverLB( itemDef_t *item, float x, float y );
void        Item_ListBox_MouseEnter( itemDef_t *item, float x, float y );
qboolean    Item_ListBox_HandleKey( itemDef_t *item, int key, qboolean down, qboolean force );
qboolean    Item_YesNo_HandleKey( itemDef_t *item, int key );
int         Item_Multi_CountSettings( itemDef_t *item );
int         Item_Multi_FindCvarByValue( itemDef_t *item );
const char  *Item_Multi_Setting( itemDef_t *item );
qboolean    Item_Multi_HandleKey( itemDef_t *item, int key );
qboolean    Item_TextField_HandleKey( itemDef_t *item, int key );
qboolean    Item_Slider_HandleKey( itemDef_t *item, int key, qboolean down );
qboolean    Item_Bind_HandleKey( itemDef_t *item, int key, qboolean down );
void        Item_StartCapture( itemDef_t *item, int key );
void        Item_MouseEnter( itemDef_t *item, float x, float y );
void        Item_MouseLeave( itemDef_t *item );
void        Item_SetMouseOver( itemDef_t *item, qboolean focus );
itemDef_t   *Menu_HitTest( menuDef_t *menu, float x, float y );
qboolean    Menus_RemoveFromStack( menuDef_t *menu );
void        Menus_AddToStack( menuDef_t *menu );
qboolean    Menus_MenuIsInStack( menuDef_t *menu );
void        Menu_RunCloseScript( menuDef_t *menu );
void        Menus_HandleOOBClick( menuDef_t *menu, int key, qboolean down );
qboolean    ParseColorRange( itemDef_t *item, int handle, int type );
void        Menu_UpdatePosition( menuDef_t *menu );
void        Menu_PostParse( menuDef_t *menu );
itemDef_t   *Menu_ClearFocus( menuDef_t *menu );
qboolean    Rect_ContainsPoint( rectDef_t *rect, float x, float y );
int         Menu_ItemsMatchingGroup( menuDef_t *menu, const char *name );
itemDef_t   *Menu_GetMatchingItemByNumber( menuDef_t *menu, int index, const char *name );
itemDef_t   *Menu_FindItemByName( menuDef_t *menu, const char *p );
void        Menu_ShowItemByName( menuDef_t *menu, const char *p, qboolean bShow );
void        Menu_FadeItemByName( menuDef_t *menu, const char *p, qboolean fadeOut );
menuDef_t   *Menus_FindByName( const char *p );
void        Menus_Close( menuDef_t *menu );
void        Menus_CloseByName( const char *p );
void        Menus_CloseAll( void );
void        Menu_TransitionItemByName( menuDef_t *menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt );
void        Menu_OrbitItemByName( menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time );
void        Item_RunScript( itemDef_t *item, const char *s );
qboolean    Item_EnableShowViaCvar( itemDef_t *item, int flag );
qboolean    Item_SetFocus( itemDef_t *item, float x, float y );
qboolean    Item_HandleKey( itemDef_t *item, int key, qboolean down );
void        Item_Action( itemDef_t *item );
itemDef_t   *Menu_SetPrevCursorItem( menuDef_t *menu );
itemDef_t   *Menu_SetNextCursorItem( menuDef_t *menu );
void        Menu_CloseCinematics( menuDef_t *menu );
void        Display_CloseCinematics( void );
void        Menus_Open( menuDef_t *menu );
void        Menus_OpenByName( const char *p );
int         Display_VisibleMenuCount( void );
rectDef_t   *Item_CorrectedTextRect( itemDef_t *item );
void        Menu_HandleKey( menuDef_t *menu, int key, qboolean down );
void        Item_SetTextExtents( itemDef_t *item, int *width, int *height, const char *text );
void        Item_Paint( itemDef_t *item );
void        Menu_Init( menuDef_t *menu, int imageTrack );
itemDef_t   *Menu_GetFocusedItem( menuDef_t *menu );
menuDef_t   *Menu_GetFocused( void );
void        Menu_ScrollFeeder( menuDef_t *menu, int feeder, qboolean down );
void        Menu_SetFeederSelection( menuDef_t *menu, int feeder, int index, const char *name );
qboolean    UI_IsFullscreen( void );
void        Item_Init( itemDef_t *item, int imageTrack );
qboolean    Menu_HandleMouseMove( menuDef_t *menu, float x, float y );
void        Menu_Paint( menuDef_t *menu, qboolean forcePaint );
void        Item_ValidateTypeData( itemDef_t *item );
qboolean    Item_Parse( int handle, itemDef_t *item );
void        Item_InitControls( itemDef_t *item );
qboolean    Menu_Parse( int handle, menuDef_t *menu );
void        Menu_New( int handle, int imageTrack );
int         Menu_Count( void );
void        Menu_PaintAll( void );
void        Menu_Reset( void );
displayContextDef_t *Display_GetContext( void );
void        *Display_CaptureItem( int x, int y );
qboolean    Display_MouseMove( void *p, int x, int y );
int         Display_CursorType( int x, int y );
qboolean    Display_KeyBindPending( void );
void        Display_HandleKey( int key, qboolean down, int x, int y );
void        Display_CacheAll( void );

void        Controls_GetKeyAssignment( char *command, int *twokeys );
void        Controls_GetConfig( void );
void        Controls_SetConfig( qboolean restart );
void        Controls_SetDefaults( void );
int         BindingIDFromName( const char *name );
char        *BindingFromName( const char *name );
qboolean    GetCommandHasBinding( const char *name );
int         GetKeyBindings( const char *name, const char **bind1, const char **bind2 );
int         GetKeyBindingLocalizedString( const char *name, char **out );

int         trap_PC_AddGlobalDefine( char *define );
int         trap_PC_LoadSource( const char *filename );
int         trap_PC_FreeSource( int handle );
int         trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int         trap_PC_SourceFileAndLine( int handle, char *filename, int *line );

// q_shared.c / com_shared.c
int         Q_stricmp( const char *s1, const char *s2 );
int         Q_stricmpn( const char *s1, const char *s2, int n );
int         Q_strncmp( const char *s1, const char *s2, int n );
void        Q_strncpyz( char *dest, const char *src, int destsize );
void        Q_strcat( char *dest, int size, const char *src );
qboolean    Q_isnumeric( int c );
void        Com_StripExtension( const char *in, char *out );
char        *va( const char *format, ... );
void        Com_Error( int level, const char *fmt, ... );
void        Com_Printf( const char *msg, ... );
char        *Com_ParseOnLine( char **data_p );
extern parseInfo_t *parseInfo;

// com_math.c
void        AxisClear( vec3_t axis[3] );
void        AnglesToAxis( const vec3_t angles, vec3_t axis[3] );

#endif  /* __UI_SHARED_H__ */
