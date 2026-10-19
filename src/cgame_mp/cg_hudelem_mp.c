// cg_hudelem.c -- draws the HUD elements created by the server scripts
// the live elements are collected from the playerState, sorted by sortKey,
// and drawn as text, a timer, a shader or a clock

#include <stdlib.h>
#include <string.h>

#include "cg_local.h"

typedef enum {
	HE_TYPE_FREE,
	HE_TYPE_TEXT,
	HE_TYPE_VALUE,
	HE_TYPE_SHADER,
	HE_TYPE_TIMER_DOWN,
	HE_TYPE_TIMER_UP,
	HE_TYPE_TENTHS_TIMER_DOWN,
	HE_TYPE_TENTHS_TIMER_UP,
	HE_TYPE_CLOCK_DOWN,
	HE_TYPE_CLOCK_UP
} hudElemType_t;

#define HE_ALIGN_LEFT       0
#define HE_ALIGN_CENTER     1
#define HE_ALIGN_RIGHT      2
#define HE_ALIGN_TOP        0
#define HE_ALIGN_MIDDLE     1
#define HE_ALIGN_BOTTOM     2

#define HE_FONT_DEFAULT     0
#define HE_FONT_BIGFIXED    1
#define HE_FONT_SMALLFIXED  2

#define MAX_CLIENT_HUDELEMS 31

#define CS_HUDELEM_STRINGS  1244

#define ANGLE2SHORT( x )    ( (int)( (x) * ( 65536.0f / 360.0f ) ) & 65535 )
#define SHORT2ANGLE( x )    ( (x) * ( 360.0f / 65536.0f ) )

// one hudelem_t resolved for drawing
typedef struct hudElemInfo_s {
	float       x;
	float       y;
	float       width;
	float       height;
	const char *label;
	float       labelWidth;
	const char *text;
	float       textWidth;
	int         font;                       // the engine font handle
	float       fontScale;
	float       charHeight;
	float       charWidth;                  // 0 for the proportional font
	vec4_t      color;
} hudElemInfo_t;
CG_ASSERT_SIZE( hudElemInfo_t, 64 );

qboolean    CG_ServerShaderName( int index, char *buffer, int size );

static rfloat CG_HudElemShaderWidth( const hudelem_t *elem, const hudElemInfo_t *info );
static float CG_HudElemShaderHeight( const hudelem_t *elem, const hudElemInfo_t *info );

/*
===============
CG_HudElemStringWidth

The proportional font measures through the renderer; the two fixed fonts are
just the printable length times the cell width.
===============
*/
static float CG_HudElemStringWidth( const char *string, const hudElemInfo_t *info ) {
	if ( info->charWidth != 0.0f ) {
		return trap_SE_PrintStrlen( string ) * info->charWidth;
	}
	return (float)trap_R_Text_Width( string, info->font, info->fontScale, 0 );
}

/*
===============
CG_GetHudElemTime

Milliseconds the element's timer still has to run, or has run for.
Count-downs round up so the last second still shows "1".
===============
*/
static int CG_GetHudElemTime( const hudelem_t *elem ) {
	int t;

	switch ( elem->type ) {
	case HE_TYPE_TIMER_DOWN:
		t = elem->timerValue - cg.time + 999;
		break;
	case HE_TYPE_TIMER_UP:
		t = cg.time - elem->timerValue;
		break;
	case HE_TYPE_TENTHS_TIMER_DOWN:
		t = elem->timerValue - cg.time + 99;
		break;
	case HE_TYPE_TENTHS_TIMER_UP:
		t = cg.time - elem->timerValue;
		break;
	case HE_TYPE_CLOCK_DOWN:
		t = elem->timerValue - cg.time;
		break;
	case HE_TYPE_CLOCK_UP:
		t = cg.time - elem->timerValue;
		break;
	default:
		return 0;
	}

	if ( t < 0 ) {
		t = 0;
	}
	return t;
}

/*
===============
CG_HudElemTimerString
===============
*/
static const char *CG_HudElemTimerString( const hudelem_t *elem ) {
	int seconds;
	int hours;
	int mins;
	int secs;

	seconds = CG_GetHudElemTime( elem ) / 1000;
	hours = seconds / 3600;
	seconds -= hours * 3600;
	mins = seconds / 60;
	secs = seconds - mins * 60;

	if ( hours ) {
		return va( "%i:%02i:%02i", hours, mins, secs );
	}
	return va( "%i:%02i", mins, secs );
}

/*
===============
CG_HudElemTenthsTimerString
===============
*/
static const char *CG_HudElemTenthsTimerString( const hudelem_t *elem ) {
	int t;
	int hours;
	int mins;
	int secs;
	int tenths;

	t = CG_GetHudElemTime( elem ) / 100;
	hours = t / 36000;
	t -= hours * 36000;
	mins = t / 600;
	t -= mins * 600;
	secs = t / 10;
	tenths = t - secs * 10;

	if ( hours ) {
		return va( "%i:%02i:%02i.%i", hours, mins, secs, tenths );
	}
	return va( "%i:%02i.%i", mins, secs, tenths );
}

/*
===============
CG_HudElemShaderDimension

A zero width or height on a shader element means "one character cell"
===============
*/
static float CG_HudElemShaderDimension( int dimension, const hudElemInfo_t *info ) {
	if ( dimension ) {
		return (float)dimension;
	}
	return info->charHeight;
}

/*
===============
CG_HudElemShaderWidth

keep: rfloat return matches original codegen
===============
*/
static rfloat CG_HudElemShaderWidth( const hudelem_t *elem, const hudElemInfo_t *info ) {
	rfloat  w;
	rfloat  from;
	int     t;

	w = CG_HudElemShaderDimension( elem->width, info );

	if ( elem->scaleTime > 0 ) {
		t = cg.time - elem->scaleStartTime;
		if ( t < elem->scaleTime ) {
			from = CG_HudElemShaderDimension( elem->scaleFromWidth, info );
			return ( w - from ) * ( t * ( 1.0f / (rfloat)elem->scaleTime ) ) + from;
		}
	}

	return w;
}

/*
===============
CG_HudElemShaderHeight
===============
*/
static float CG_HudElemShaderHeight( const hudelem_t *elem, const hudElemInfo_t *info ) {
	float   h;
	float   from;
	int     t;

	h = CG_HudElemShaderDimension( elem->height, info );

	if ( elem->scaleTime > 0 ) {
		t = cg.time - elem->scaleStartTime;
		if ( t < elem->scaleTime ) {
			from = CG_HudElemShaderDimension( elem->scaleFromHeight, info );
			return ( h - from ) * ( t * ( 1.0f / elem->scaleTime ) ) + from;
		}
	}

	return h;
}

/*
===============
CG_HudElemWidth

===============
*/
static float CG_HudElemWidth( const hudelem_t *elem, const hudElemInfo_t *info ) {
	rfloat width;

	switch ( elem->type ) {
	case HE_TYPE_TEXT:
	case HE_TYPE_VALUE:
	case HE_TYPE_TIMER_DOWN:
	case HE_TYPE_TIMER_UP:
	case HE_TYPE_TENTHS_TIMER_DOWN:
	case HE_TYPE_TENTHS_TIMER_UP:
		width = info->textWidth;
		break;

	case HE_TYPE_SHADER:
	case HE_TYPE_CLOCK_DOWN:
	case HE_TYPE_CLOCK_UP:
		width = CG_HudElemShaderWidth( elem, info );
		break;

	default:
		return 0.0f;
	}

	width += info->labelWidth;
	return width;
}

/*
===============
CG_HudElemHeight

A labelled element is never shorter than one line of its own font
===============
*/
static float CG_HudElemHeight( const hudelem_t *elem, const hudElemInfo_t *info ) {
	float h;

	switch ( elem->type ) {
	case HE_TYPE_TEXT:
	case HE_TYPE_VALUE:
	case HE_TYPE_TIMER_DOWN:
	case HE_TYPE_TIMER_UP:
	case HE_TYPE_TENTHS_TIMER_DOWN:
	case HE_TYPE_TENTHS_TIMER_UP:
		h = info->charHeight;
		break;

	case HE_TYPE_SHADER:
	case HE_TYPE_CLOCK_DOWN:
	case HE_TYPE_CLOCK_UP:
		h = CG_HudElemShaderHeight( elem, info );
		break;

	default:
		return 0.0f;
	}

	if ( info->label && h < info->charHeight ) {
		h = info->charHeight;
	}
	return h;
}

/*
===============
CG_HudElemX

Moves from moveFromX over moveTime, then applies the alignment.
keep: rfloat return matches original codegen
===============
*/
static rfloat CG_HudElemX( const hudelem_t *elem, const hudElemInfo_t *info ) {
	rfloat  x;
	int     t;

	if ( elem->moveTime > 0 && ( t = cg.time - elem->moveStartTime ) < elem->moveTime ) {
		x = ( elem->x - elem->moveFromX ) * t * ( 1.0f / (rfloat)elem->moveTime ) + elem->moveFromX;
	} else {
		x = elem->x;
	}

	switch ( elem->alignX ) {
	case HE_ALIGN_LEFT:
		return x;
	case HE_ALIGN_CENTER:
		return x - info->width * 0.5f;
	case HE_ALIGN_RIGHT:
		return x - info->width;
	default:
		return x;
	}
}

/*
===============
CG_HudElemY

===============
*/
static rfloat CG_HudElemY( const hudelem_t *elem, const hudElemInfo_t *info ) {
	rfloat  y;
	int     t;

	if ( elem->moveTime > 0 && ( t = cg.time - elem->moveStartTime ) < elem->moveTime ) {
		y = ( elem->y - elem->moveFromY ) * t * ( 1.0f / (rfloat)elem->moveTime ) + elem->moveFromY;
	} else {
		y = elem->y;
	}

	switch ( elem->alignY ) {
	case HE_ALIGN_TOP:
		return y;
	case HE_ALIGN_MIDDLE:
		return y - info->height * 0.5f;
	case HE_ALIGN_BOTTOM:
		return y - info->height;
	default:
		return y;
	}
}

/*
===============
CG_ConsolidateHudElemText

An element with both a label and a text draws as one string: the label with its
"%s" replaced by the text, or the text appended if the label has no "%s"
===============
*/
static void CG_ConsolidateHudElemText( hudElemInfo_t *info, int size, char *buffer ) {
	int     len;
	int     labelPos;
	int     textPos;
	char    c;

	len = 0;
	labelPos = 0;

	while ( len < size - 1 ) {
		c = info->label[labelPos];
		if ( !c ) {
			break;
		}
		if ( c == '%' && info->label[labelPos + 1] == 's' ) {
			labelPos += 2;
			break;
		}
		len++;
		buffer[labelPos] = c;
		labelPos++;
	}

	textPos = 0;
	while ( len < size - 1 ) {
		c = info->text[textPos];
		if ( !c ) {
			break;
		}
		buffer[len] = c;
		len++;
		textPos++;
	}

	while ( len < size - 1 ) {
		c = info->label[labelPos];
		if ( !c ) {
			break;
		}
		buffer[len] = c;
		len++;
		labelPos++;
	}

	buffer[len] = 0;

	info->textWidth = info->textWidth + info->labelWidth;
	info->text = buffer;
	info->label = "";
	info->labelWidth = 0;
}

/*
===============
CG_GetHudElemInfo

Resolves one hudelem_t into everything the drawing functions need
===============
*/
static void CG_GetHudElemInfo( hudElemInfo_t *info, const hudelem_t *elem,
							   char *buffer, int size ) {
	int     t;
	rfloat  frac;   // keep: matches original codegen
	int     i;

	switch ( elem->font ) {
	case HE_FONT_DEFAULT:
		info->font = 0;
		info->fontScale = elem->fontScale * 0.25f;
		info->charHeight = (float)trap_R_Text_Height( 0, info->fontScale );
		info->charWidth = 0.0f;
		break;
	case HE_FONT_BIGFIXED:
		info->font = 4;
		info->fontScale = elem->fontScale * ( 1.0f / 3.0f );
		info->charHeight = 16.0f;
		info->charWidth = 16.0f;
		break;
	case HE_FONT_SMALLFIXED:
		info->font = 5;
		info->fontScale = elem->fontScale * ( 1.0f / 3.0f );
		info->charHeight = 16.0f;
		info->charWidth = 8.0f;
		break;
	default:
		break;
	}

	if ( elem->label ) {
		info->label = CG_SafeTranslateHudElemString( elem->label );
	} else {
		info->label = "";
	}

	switch ( elem->type ) {
	case HE_TYPE_TEXT:
		if ( elem->text ) {
			info->text = CG_SafeTranslateHudElemString( elem->text );
		} else {
			info->text = "";
		}
		break;
	case HE_TYPE_TIMER_DOWN:
	case HE_TYPE_TIMER_UP:
		info->text = CG_HudElemTimerString( elem );
		break;
	case HE_TYPE_TENTHS_TIMER_DOWN:
	case HE_TYPE_TENTHS_TIMER_UP:
		info->text = CG_HudElemTenthsTimerString( elem );
		break;
	case HE_TYPE_VALUE:
		info->text = va( "%g", elem->value );
		break;
	default:
		info->text = "";
		break;
	}

	if ( info->label[0] && info->text[0] ) {
		CG_ConsolidateHudElemText( info, size, buffer );
	}

	if ( info->label[0] ) {
		info->labelWidth = CG_HudElemStringWidth( info->label, info );
	} else {
		info->labelWidth = 0.0f;
	}

	if ( info->text[0] ) {
		info->textWidth = CG_HudElemStringWidth( info->text, info );
	} else {
		info->textWidth = 0.0f;
	}

	info->width = CG_HudElemWidth( elem, info );
	info->height = CG_HudElemHeight( elem, info );
	info->x = CG_HudElemX( elem, info );
	info->y = CG_HudElemY( elem, info );

	if ( elem->fadeTime > 0 ) {
		t = cg.time - elem->fadeStartTime;
		if ( t < elem->fadeTime ) {
			frac = (rfloat)t / elem->fadeTime;
			for ( i = 0; i < 4; i++ ) {
				info->color[i] = ( ( elem->color[i] - elem->fromColor[i] ) * frac
								   + elem->fromColor[i] ) * ( 1.0f / 255.0f );
			}
			return;
		}
	}

	for ( i = 0; i < 4; i++ ) {
		info->color[i] = elem->color[i] * ( 1.0f / 255.0f );
	}
}

/*
===============
CG_HudElemAlignY

The vertical placement of an item of height h inside the element's box
===============
*/
static float CG_HudElemAlignY( const hudelem_t *elem, const hudElemInfo_t *info, float h ) {
	switch ( elem->alignY ) {
	case HE_ALIGN_TOP:
		return info->y;
	case HE_ALIGN_MIDDLE:
		return ( info->height - h ) * 0.5f + info->y;
	case HE_ALIGN_BOTTOM:
		return info->height + info->y - h;
	default:
		return 0.0f;
	}
}

/*
===============
CG_DrawHudElemString

trap_R_Text_Paint takes the baseline, one character cell below the top
===============
*/
static void CG_DrawHudElemString( const hudelem_t *elem, const hudElemInfo_t *info,
								  const char *string ) {
	float y;

	y = CG_HudElemAlignY( elem, info, info->charHeight );

	trap_R_Text_Paint( info->x, y + info->charHeight, info->font, info->fontScale,
					   info->color, string, info->charWidth, 0, 3 );
}

/*
===============
CG_DrawHudElemClock

Draws the element's material, then the same name with "Needle" appended
rotated by the elapsed time.  A zero rotationPeriodMs means once a minute.
===============
*/
static void CG_DrawHudElemClock( const hudelem_t *elem, const hudElemInfo_t *info ) {
	char        name[64];
	qhandle_t   hFace;
	qhandle_t   hNeedle;
	int         t;
	float       angle;
	float       w, h, y;

	if ( !CG_ServerShaderName( elem->materialIndex, name, sizeof( name ) - 6 ) ) {
		return;
	}

	hFace = trap_R_RegisterShaderNoMip( name, 5 );
	strcat( name, "Needle" );
	hNeedle = trap_R_RegisterShaderNoMip( name, 5 );

	t = CG_GetHudElemTime( elem );
	if ( elem->rotationPeriodMs ) {
		angle = AngleNormalize360( (float)t * 360.0f / elem->rotationPeriodMs );
	} else {
		angle = AngleNormalize360( (float)t * ( 360.0f / 60000.0f ) );
	}

	w = CG_HudElemShaderWidth( elem, info );
	h = CG_HudElemShaderHeight( elem, info );
	y = CG_HudElemAlignY( elem, info, h );

	trap_R_SetColor( info->color );
	CG_DrawPic( info->x, y, w, h, hFace );
	CG_DrawRotatedPic( info->x, y, w, h, angle, hNeedle );
	trap_R_SetColor( NULL );
}

/*
===============
CG_DrawHudElemShader
===============
*/
static void CG_DrawHudElemShader( const hudelem_t *elem, const hudElemInfo_t *info ) {
	char        name[64];
	qhandle_t   hShader;
	float       w, h, y;

	if ( !CG_ServerShaderName( elem->materialIndex, name, sizeof( name ) ) ) {
		return;
	}

	hShader = trap_R_RegisterShader( name, 5 );

	w = CG_HudElemShaderWidth( elem, info );
	h = CG_HudElemShaderHeight( elem, info );
	y = CG_HudElemAlignY( elem, info, h );

	trap_R_SetColor( info->color );
	CG_DrawPic( info->x, y, w, h, hShader );
	trap_R_SetColor( NULL );
}

/*
===============
CG_DrawSingleHudElem
===============
*/
static void CG_DrawSingleHudElem( const hudelem_t *elem ) {
	hudElemInfo_t   info;
	char            buffer[8192];

	CG_GetHudElemInfo( &info, elem, buffer, sizeof( buffer ) );

	if ( info.label[0] ) {
		CG_DrawHudElemString( elem, &info, info.label );
		info.x += info.labelWidth;
	}

	switch ( elem->type ) {
	case HE_TYPE_TEXT:
	case HE_TYPE_VALUE:
	case HE_TYPE_TIMER_DOWN:
	case HE_TYPE_TIMER_UP:
	case HE_TYPE_TENTHS_TIMER_DOWN:
	case HE_TYPE_TENTHS_TIMER_UP:
		if ( info.text[0] ) {
			CG_DrawHudElemString( elem, &info, info.text );
		}
		break;

	case HE_TYPE_CLOCK_DOWN:
	case HE_TYPE_CLOCK_UP:
		CG_DrawHudElemClock( elem, &info );
		break;

	case HE_TYPE_SHADER:
		CG_DrawHudElemShader( elem, &info );
		break;

	default:
		break;
	}
}

/*
===============
compare_hudelems
===============
*/
static int compare_hudelems( const void *a, const void *b ) {
	float d;

	d = ( *(const hudelem_t **)a )->sortKey - ( *(const hudelem_t **)b )->sortKey;

	if ( d < 0.0f ) {
		return -1;
	}
	if ( d > 0.0f ) {
		return 1;
	}
	return 0;
}

/*
===============
CG_GetSortedHudElems

Both playerState arrays, archival first, sorted by sortKey
===============
*/
static int CG_GetSortedHudElems( const hudelem_t **list ) {
	const playerState_t *ps;
	int             count;
	int             i;

	ps = &cg.snap->ps;
	count = 0;

	for ( i = 0; i < MAX_CLIENT_HUDELEMS; i++ ) {
		if ( !ps->hud.archival[i].type ) {
			break;
		}
		list[count] = &ps->hud.archival[i];
		count++;
	}

	for ( i = 0; i < MAX_CLIENT_HUDELEMS; i++ ) {
		if ( !ps->hud.current[i].type ) {
			break;
		}
		list[count] = &ps->hud.current[i];
		count++;
	}

	qsort( list, count, sizeof( list[0] ), compare_hudelems );

	return count;
}

/*
===============
CG_DrawHudElems
===============
*/
void CG_DrawHudElems( void ) {
	const hudelem_t *list[2 * MAX_CLIENT_HUDELEMS];
	int             count;
	int             i;

	count = CG_GetSortedHudElems( list );

	if ( count ) {
		for ( i = 0; i < count; i++ ) {
			CG_DrawSingleHudElem( list[i] );
		}
	}
}
