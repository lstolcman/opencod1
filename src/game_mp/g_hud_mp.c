// g_hud_mp.c -- server-side hud elements, created and driven by script and
// sent to each client that can see them through playerState_t.hud

#include <stdio.h>
#include <string.h>

#include "g_local.h"

// bg_slidemove.c
int Q_ftol( float f );

#define MAX_HUDELEMS            1024

#define MAX_CLIENT_HUDELEMS     31     // size of each hudElemState_t array

// g_hudelem_t.type
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

typedef struct hudElemField_s hudElemField_t;
typedef void ( *hudElemFieldFunc_t )( g_hudelem_t *elem, int fieldnum );

struct hudElemField_s {
	const char          *name;
	int                 ofs;
	fieldtype_t         type;
	hudElemFieldFunc_t  setter;     // NULL uses Scr_SetGenericField
	hudElemFieldFunc_t  getter;     // NULL uses Scr_GetGenericField
};

typedef struct hudElemMethod_s {
	const char  *name;
	void        ( *call )( int elemnum );
} hudElemMethod_t;

#define HEOFS( x )      ( (int)&( ( (g_hudelem_t *)0 )->x ) )

// keep: HudElem_SetColor's MAX-of-MIN clamp matches original codegen
#define HE_MAX( a, b )  ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define HE_MIN( a, b )  ( ( a ) < ( b ) ? ( a ) : ( b ) )

static void HudElem_SetLocalizedString( g_hudelem_t *elem, int fieldnum );
static void HudElem_SetBoolean( g_hudelem_t *elem, int fieldnum );
static void HudElem_SetColor( g_hudelem_t *elem, int fieldnum );
static void HudElem_GetColor( g_hudelem_t *elem, int fieldnum );
static void HudElem_SetAlpha( g_hudelem_t *elem, int fieldnum );
static void HudElem_GetAlpha( g_hudelem_t *elem, int fieldnum );
static void HudElem_SetFontScale( g_hudelem_t *elem, int fieldnum );
static void HudElem_SetFont( g_hudelem_t *elem, int fieldnum );
static void HudElem_GetFont( g_hudelem_t *elem, int fieldnum );
static void HudElem_SetAlignX( g_hudelem_t *elem, int fieldnum );
static void HudElem_GetAlignX( g_hudelem_t *elem, int fieldnum );
static void HudElem_SetAlignY( g_hudelem_t *elem, int fieldnum );
static void HudElem_GetAlignY( g_hudelem_t *elem, int fieldnum );

static void HECmd_SetText( int elemnum );
static void HECmd_SetShader( int elemnum );
static void HECmd_SetTimer( int elemnum );
static void HECmd_SetTimerUp( int elemnum );
static void HECmd_SetTenthsTimer( int elemnum );
static void HECmd_SetTenthsTimerUp( int elemnum );
static void HECmd_SetClock( int elemnum );
static void HECmd_SetClockUp( int elemnum );
static void HECmd_SetValue( int elemnum );
static void HECmd_FadeOverTime( int elemnum );
static void HECmd_ScaleOverTime( int elemnum );
static void HECmd_MoveOverTime( int elemnum );
static void HECmd_Reset( int elemnum );
static void HECmd_Destroy( int elemnum );

g_hudelem_t g_hudelems[MAX_HUDELEMS];

static const hudElemField_t hudelemFields[] = {
	{ "x",          HEOFS( x ),         F_INT,      NULL,                       NULL },
	{ "y",          HEOFS( y ),         F_INT,      NULL,                       NULL },
	{ "fontscale",  HEOFS( fontScale ), F_FLOAT,    HudElem_SetFontScale,       NULL },
	{ "font",       HEOFS( font ),      F_INT,      HudElem_SetFont,            HudElem_GetFont },
	{ "alignx",     HEOFS( alignX ),    F_INT,      HudElem_SetAlignX,          HudElem_GetAlignX },
	{ "aligny",     HEOFS( alignY ),    F_INT,      HudElem_SetAlignY,          HudElem_GetAlignY },
	{ "color",      HEOFS( color ),     F_INT,      HudElem_SetColor,           HudElem_GetColor },
	{ "alpha",      HEOFS( color ),     F_INT,      HudElem_SetAlpha,           HudElem_GetAlpha },
	{ "label",      HEOFS( label ),     F_INT,      HudElem_SetLocalizedString, NULL },
	{ "sort",       HEOFS( sortKey ),   F_FLOAT,    NULL,                       NULL },
	{ "archived",   HEOFS( archived ),  F_INT,      HudElem_SetBoolean,         NULL },

	{ NULL,         0,                  F_INT,      NULL,                       NULL }
};

static const char *hudelemFontNames[] = {
	"default",
	"bigfixed",
	"smallfixed"
};

static const char *hudelemAlignXNames[] = {
	"left",
	"center",
	"right"
};

static const char *hudelemAlignYNames[] = {
	"top",
	"middle",
	"bottom"
};

/*
==================
HudElem_SetDefaults

Clears everything the element's type owns; every method that changes the type
starts here.  Leaves x/y, the color, the label, the sort key and the move
state alone.
==================
*/
static void HudElem_SetDefaults( g_hudelem_t *elem ) {
	elem->width = 0;
	elem->height = 0;
	elem->materialIndex = 0;
	elem->scaleFromWidth = 0;
	elem->scaleFromHeight = 0;
	elem->scaleStartTime = 0;
	elem->scaleTime = 0;
	elem->timerValue = 0;
	elem->rotationPeriodMs = 0;
	elem->value = 0;
	elem->text = 0;
}

/*
==================
HudElem_Reset

Everything back to a plain white text element.
==================
*/
static void HudElem_Reset( g_hudelem_t *elem ) {
	elem->type = HE_TYPE_TEXT;
	elem->x = 0;
	elem->y = 0;
	elem->fontScale = 1.0f;
	elem->font = 0;
	elem->alignX = 0;
	elem->alignY = 0;
	elem->color = -1;
	elem->fromColor = 0;
	elem->fadeStartTime = 0;
	elem->fadeTime = 0;
	elem->label = 0;
	elem->sortKey = 0;
	elem->archived = qtrue;

	HudElem_SetDefaults( elem );
}

/*
==================
HudElem_Alloc
==================
*/
static g_hudelem_t *HudElem_Alloc( int clientNum, int team ) {
	unsigned int i;

	for ( i = 0; i < MAX_HUDELEMS; i++ ) {
		if ( !g_hudelems[i].type ) {
			HudElem_Reset( &g_hudelems[i] );
			g_hudelems[i].clientNum = clientNum;
			g_hudelems[i].team = team;
			return &g_hudelems[i];
		}
	}

	return NULL;
}

/*
==================
HudElem_Free
==================
*/
static void HudElem_Free( g_hudelem_t *elem ) {
	Scr_FreeHudElem( elem );
	elem->type = HE_TYPE_FREE;
}

/*
==================
HudElem_ClientDisconnect
==================
*/
void HudElem_ClientDisconnect( gentity_t *ent ) {
	unsigned int i;

	for ( i = 0; i < MAX_HUDELEMS; i++ ) {
		if ( !g_hudelems[i].type ) {
			continue;
		}
		if ( g_hudelems[i].clientNum == ent->s.number ) {
			HudElem_Free( &g_hudelems[i] );
		}
	}
}

/*
==================
HudElem_DestroyAll
==================
*/
void HudElem_DestroyAll( void ) {
	unsigned int i;

	for ( i = 0; i < MAX_HUDELEMS; i++ ) {
		if ( g_hudelems[i].type ) {
			HudElem_Free( &g_hudelems[i] );
		}
	}

	memset( g_hudelems, 0, sizeof( g_hudelems ) );
}

/*
==================
HudElem_SetEnumString
==================
*/
static void HudElem_SetEnumString( g_hudelem_t *elem, const hudElemField_t *field,
								   const char **names, int count ) {
	int         i;
	const char  *string;
	char        buffer[2048];

	string = Scr_GetString( 0 );

	for ( i = 0; i < count; i++ ) {
		if ( !Q_stricmp( string, names[i] ) ) {
			*(int *)( (byte *)elem + field->ofs ) = i;
			return;
		}
	}

	sprintf( buffer, "\"%s\" is not a valid value for hudelem field \"%s\"\nShould be one of:",
			 string, field->name );
	// keep: the length is the whole buffer, not what is left of it (original behavior)
	for ( i = 0; i < count; i++ ) {
		strncat( buffer, va( " %s", names[i] ), sizeof( buffer ) );
		buffer[sizeof( buffer ) - 1] = 0;
	}
	Scr_Error( buffer );
}

/*
==================
HudElem_GetEnumString

count is unused; it mirrors HudElem_SetEnumString.
==================
*/
static void HudElem_GetEnumString( g_hudelem_t *elem, const hudElemField_t *field,
								   const char **names, int count ) {
	Scr_AddString( names[ *(int *)( (byte *)elem + field->ofs ) ] );
}

/*
==================
HudElem_SetLocalizedString
==================
*/
static void HudElem_SetLocalizedString( g_hudelem_t *elem, int fieldnum ) {
	*(int *)( (byte *)elem + hudelemFields[fieldnum].ofs ) =
		G_LocalizedStringIndex( Scr_GetIString( 0 ) );
}

/*
==================
HudElem_SetBoolean
==================
*/
static void HudElem_SetBoolean( g_hudelem_t *elem, int fieldnum ) {
	*(int *)( (byte *)elem + hudelemFields[fieldnum].ofs ) = Scr_GetBool( 0 );
}

/*
==================
HudElem_SetColor

Each component is clamped to 0..1 and scaled to a byte.  The alpha byte is
left alone; "color" and "alpha" share the field offset and both address the
packed color directly.
==================
*/
static void HudElem_SetColor( g_hudelem_t *elem, int fieldnum ) {
	vec3_t  color;

	Scr_GetVector( 0, color );

	( (byte *)&elem->color )[0] = Q_ftol( HE_MAX( 0.0f, HE_MIN( 1.0f, color[0] ) ) * 255.0f );
	( (byte *)&elem->color )[1] = Q_ftol( HE_MAX( 0.0f, HE_MIN( 1.0f, color[1] ) ) * 255.0f );
	( (byte *)&elem->color )[2] = Q_ftol( HE_MAX( 0.0f, HE_MIN( 1.0f, color[2] ) ) * 255.0f );
}

/*
==================
HudElem_GetColor
==================
*/
static void HudElem_GetColor( g_hudelem_t *elem, int fieldnum ) {
	const byte  *rgba;
	vec3_t      color;

	rgba = (const byte *)&elem->color;
	color[0] = rgba[0] * ( 1.0f / 255.0f );
	color[1] = rgba[1] * ( 1.0f / 255.0f );
	color[2] = rgba[2] * ( 1.0f / 255.0f );

	Scr_AddVector( color );
}

/*
==================
HudElem_SetAlpha
==================
*/
static void HudElem_SetAlpha( g_hudelem_t *elem, int fieldnum ) {
	byte    *rgba;
	float   alpha;

	alpha = Scr_GetFloat( 0 );
	// keep: inverted test matches original codegen
	if ( !( 1.0f < alpha ) ) {
		if ( 0.0f > alpha ) {
			alpha = 0.0f;
		}
	} else {
		alpha = 1.0f;
	}
	alpha = alpha * 255.0f;

	rgba = (byte *)&elem->color;
	rgba[3] = Q_ftol( alpha );
}

/*
==================
HudElem_GetAlpha
==================
*/
static void HudElem_GetAlpha( g_hudelem_t *elem, int fieldnum ) {
	const byte *rgba;

	rgba = (const byte *)&elem->color;
	Scr_AddFloat( rgba[3] * ( 1.0f / 255.0f ) );
}

/*
==================
HudElem_SetFontScale
==================
*/
static void HudElem_SetFontScale( g_hudelem_t *elem, int fieldnum ) {
	float scale;

	scale = Scr_GetFloat( 0 );
	if ( scale <= 0.0f ) {
		Scr_Error( va( "font scale was %g; should be > 0", scale ) );
	}

	elem->fontScale = scale;
}

/*
==================
HudElem_SetFont
==================
*/
static void HudElem_SetFont( g_hudelem_t *elem, int fieldnum ) {
	HudElem_SetEnumString( elem, &hudelemFields[fieldnum], hudelemFontNames, 3 );
}

/*
==================
HudElem_GetFont
==================
*/
static void HudElem_GetFont( g_hudelem_t *elem, int fieldnum ) {
	HudElem_GetEnumString( elem, &hudelemFields[fieldnum], hudelemFontNames, 3 );
}

/*
==================
HudElem_SetAlignX
==================
*/
static void HudElem_SetAlignX( g_hudelem_t *elem, int fieldnum ) {
	HudElem_SetEnumString( elem, &hudelemFields[fieldnum], hudelemAlignXNames, 3 );
}

/*
==================
HudElem_GetAlignX
==================
*/
static void HudElem_GetAlignX( g_hudelem_t *elem, int fieldnum ) {
	HudElem_GetEnumString( elem, &hudelemFields[fieldnum], hudelemAlignXNames, 3 );
}

/*
==================
HudElem_SetAlignY
==================
*/
static void HudElem_SetAlignY( g_hudelem_t *elem, int fieldnum ) {
	HudElem_SetEnumString( elem, &hudelemFields[fieldnum], hudelemAlignYNames, 3 );
}

/*
==================
HudElem_GetAlignY
==================
*/
static void HudElem_GetAlignY( g_hudelem_t *elem, int fieldnum ) {
	HudElem_GetEnumString( elem, &hudelemFields[fieldnum], hudelemAlignYNames, 3 );
}

/*
==================
Scr_GetHudElemField
==================
*/
void Scr_GetHudElemField( int elemnum, int fieldnum ) {
	g_hudelem_t             *elem;
	const hudElemField_t    *field;

	elem = &g_hudelems[elemnum];
	field = &hudelemFields[fieldnum];

	if ( field->getter ) {
		field->getter( elem, fieldnum );
	} else {
		Scr_GetGenericField( elem, field->type, field->ofs );
	}
}

/*
==================
Scr_SetHudElemField
==================
*/
void Scr_SetHudElemField( int elemnum, int fieldnum ) {
	g_hudelem_t             *elem;
	const hudElemField_t    *field;

	field = &hudelemFields[fieldnum];
	elem = &g_hudelems[elemnum];

	if ( field->setter ) {
		field->setter( elem, fieldnum );
	} else {
		Scr_SetGenericField( elem, field->type, field->ofs );
	}
}

/*
==================
Scr_FreeHudElemConstStrings

No hudelem field is F_STRING, so this currently frees nothing.
==================
*/
void Scr_FreeHudElemConstStrings( g_hudelem_t *elem ) {
	const hudElemField_t *field;

	for ( field = hudelemFields; field->name; field++ ) {
		if ( field->type == F_STRING ) {
			Scr_SetString( (unsigned short *)( (byte *)elem + field->ofs ), 0 );
		}
	}
}

/*
==================
GScr_NewHudElem
==================
*/
void GScr_NewHudElem( void ) {
	g_hudelem_t *elem;

	elem = HudElem_Alloc( ENTITYNUM_NONE, TEAM_FREE );
	if ( !elem ) {
		Scr_Error( "out of hudelems" );
	}

	Scr_AddHudElem( elem );
}

/*
==================
GScr_NewClientHudElem
==================
*/
void GScr_NewClientHudElem( void ) {
	gentity_t   *ent;
	g_hudelem_t *elem;

	ent = Scr_GetEntity( 0 );
	if ( !ent->client ) {
		Scr_ParamError( 0, "not a client" );
	}

	elem = HudElem_Alloc( ent->s.number, TEAM_FREE );
	if ( !elem ) {
		Scr_Error( "out of hudelems" );
	}

	Scr_AddHudElem( elem );
}

/*
==================
GScr_NewTeamHudElem
==================
*/
void GScr_NewTeamHudElem( void ) {
	unsigned short  string;
	team_t          team;
	g_hudelem_t     *elem;

	string = Scr_GetConstString( 0 );
	if ( string == scr_const.allies ) {
		team = TEAM_ALLIES;
	} else if ( string == scr_const.axis ) {
		team = TEAM_AXIS;
	} else if ( string == scr_const.spectator ) {
		team = TEAM_SPECTATOR;
	} else {
		Scr_ParamError( 0, va( "team \"%s\" should be \"allies\", \"axis\", or \"spectator\"",
							   Scr_GetString( 0 ) ) );
		team = TEAM_FREE;
	}

	elem = HudElem_Alloc( ENTITYNUM_NONE, team );
	if ( !elem ) {
		Scr_Error( "out of hudelems" );
	}

	Scr_AddHudElem( elem );
}

/*
==================
GScr_AddFieldsForHudElems
==================
*/
void GScr_AddFieldsForHudElems( void ) {
	const hudElemField_t    *field;
	unsigned short          classnum;

	classnum = g_scr_data.classMap[SCR_CLASS_HUDELEM].classnum;

	for ( field = hudelemFields; field->name; field++ ) {
		switch ( field->type ) {
		case F_INT:
		case F_FLOAT:
		case F_CSTRING:
		case F_STRING:
		case F_VECTOR:
		case F_ENTITY:
		case F_OBJECT:
		case F_MODEL:
			Scr_AddClassField( classnum, field->name,
							   (unsigned short)( field - hudelemFields ) );
			break;
		default:
			break;
		}
	}
}

/*
==================
HECmd_SetText
==================
*/
static void HECmd_SetText( int elemnum ) {
	const char  *string;

	string = Scr_GetIString( 0 );

	HudElem_SetDefaults( &g_hudelems[elemnum] );
	g_hudelems[elemnum].type = HE_TYPE_TEXT;
	g_hudelems[elemnum].text = G_LocalizedStringIndex( string );
}

/*
==================
HECmd_SetShader
==================
*/
static void HECmd_SetShader( int elemnum ) {
	g_hudelem_t *elem;
	int         numParam;
	int         shader;
	int         width;
	int         height;

	numParam = Scr_GetNumParam();
	if ( numParam != 1 && numParam != 3 ) {
		Scr_Error( "USAGE: <hudelem> setShader(\"shadername\"[, optional_width, optional_height]);" );
	}

	elem = &g_hudelems[elemnum];
	shader = G_ShaderIndex( Scr_GetString( 0 ) );

	if ( numParam == 1 ) {
		width = 0;
		height = 0;
	} else {
		width = Scr_GetInt( 1 );
		if ( width < 0 ) {
			Scr_ParamError( 1, va( "width %i < 0", width ) );
		}
		height = Scr_GetInt( 2 );
		if ( height < 0 ) {
			Scr_ParamError( 2, va( "height %i < 0", height ) );
		}
	}

	HudElem_SetDefaults( elem );
	elem->type = HE_TYPE_SHADER;
	elem->width = width;
	elem->height = height;
	elem->materialIndex = shader;
}

/*
==================
HECmd_SetTimer_Internal

Shared by the four timer methods.  Any fraction of a millisecond rounds the
time up.
==================
*/
static void HECmd_SetTimer_Internal( int elemnum, hudElemType_t type, const char *name ) {
	g_hudelem_t *elem;
	float       seconds;
	int         msec;

	if ( Scr_GetNumParam() != 1 ) {
		Scr_Error( va( "USAGE: <hudelem> %s(time_in_seconds);\n", name ) );
	}

	elem = &g_hudelems[elemnum];

	seconds = Scr_GetFloat( 0 ) * 1000.0f;
	msec = Q_rint( seconds );
	if ( msec <= 0 && type != HE_TYPE_TIMER_UP ) {
		Scr_ParamError( 0, va( "time %g should be > 0", msec * 0.001f ) );
	}

	HudElem_SetDefaults( elem );
	elem->type = type;
	elem->timerValue = level.time + msec;
}

/*
==================
HECmd_SetClock_Internal
==================
*/
static void HECmd_SetClock_Internal( int elemnum, hudElemType_t type, const char *name ) {
	g_hudelem_t *elem;
	int         numParam;
	int         msec;
	int         duration;
	int         shader;
	int         width;
	int         height;

	numParam = Scr_GetNumParam();
	if ( numParam != 3 && numParam != 5 ) {
		Scr_Error( va( "USAGE: <hudelem> %s(time_in_seconds, total_clock_time_in_seconds, shadername[, width, height]);\n",
					   name ) );
	}

	elem = &g_hudelems[elemnum];

	msec = Q_rint( Scr_GetFloat( 0 ) * 1000.0f );
	if ( msec <= 0 && type != HE_TYPE_CLOCK_UP ) {
		Scr_ParamError( 0, va( "time %g should be > 0", msec * 0.001f ) );
	}

	duration = Q_rint( Scr_GetFloat( 1 ) * 1000.0f );
	if ( duration <= 0 ) {
		Scr_ParamError( 1, va( "duration %g should be > 0", duration * 0.001f ) );
	}

	shader = G_ShaderIndex( Scr_GetString( 2 ) );

	if ( numParam == 3 ) {
		width = 0;
		height = 0;
	} else {
		width = Scr_GetInt( 3 );
		if ( width < 0 ) {
			Scr_ParamError( 3, va( "width %i < 0", width ) );
		}
		height = Scr_GetInt( 4 );
		if ( height < 0 ) {
			Scr_ParamError( 4, va( "height %i < 0", height ) );
		}
	}

	HudElem_SetDefaults( elem );
	elem->type = type;
	elem->timerValue = level.time + msec;
	elem->width = width;
	elem->height = height;
	elem->rotationPeriodMs = duration;
	elem->materialIndex = shader;
}

/*
==================
HECmd_SetTimer
==================
*/
static void HECmd_SetTimer( int elemnum ) {
	HECmd_SetTimer_Internal( elemnum, HE_TYPE_TIMER_DOWN, "setTimer" );
}

/*
==================
HECmd_SetTimerUp
==================
*/
static void HECmd_SetTimerUp( int elemnum ) {
	HECmd_SetTimer_Internal( elemnum, HE_TYPE_TIMER_UP, "setTimerUp" );
}

/*
==================
HECmd_SetTenthsTimer
==================
*/
static void HECmd_SetTenthsTimer( int elemnum ) {
	HECmd_SetTimer_Internal( elemnum, HE_TYPE_TENTHS_TIMER_DOWN, "setTenthsTimer" );
}

/*
==================
HECmd_SetTenthsTimerUp
==================
*/
static void HECmd_SetTenthsTimerUp( int elemnum ) {
	HECmd_SetTimer_Internal( elemnum, HE_TYPE_TENTHS_TIMER_UP, "setTenthsTimerUp" );
}

/*
==================
HECmd_SetClock
==================
*/
static void HECmd_SetClock( int elemnum ) {
	HECmd_SetClock_Internal( elemnum, HE_TYPE_CLOCK_DOWN, "setClock" );
}

/*
==================
HECmd_SetClockUp
==================
*/
static void HECmd_SetClockUp( int elemnum ) {
	HECmd_SetClock_Internal( elemnum, HE_TYPE_CLOCK_UP, "setClockUp" );
}

/*
==================
HECmd_SetValue
==================
*/
static void HECmd_SetValue( int elemnum ) {
	float       value;

	value = Scr_GetFloat( 0 );

	HudElem_SetDefaults( &g_hudelems[elemnum] );
	g_hudelems[elemnum].value = value;
	g_hudelems[elemnum].type = HE_TYPE_VALUE;
}

/*
==================
HECmd_FadeOverTime
==================
*/
static void HECmd_FadeOverTime( int elemnum ) {
	g_hudelem_t *elem;
	float       seconds;

	elem = &g_hudelems[elemnum];

	seconds = Scr_GetFloat( 0 );
	if ( seconds <= 0.0f ) {
		Scr_ParamError( 0, va( "fade time %g <= 0", seconds ) );
	} else if ( seconds > 60.0f ) {
		Scr_ParamError( 0, va( "fade time %g > 60", seconds ) );
	}

	elem->fadeStartTime = level.time;
	elem->fadeTime = Q_ftol( seconds * 1000.0f );
	elem->fromColor = elem->color;
}

/*
==================
HECmd_ScaleOverTime
==================
*/
static void HECmd_ScaleOverTime( int elemnum ) {
	g_hudelem_t *elem;
	float       seconds;
	int         width;
	int         height;

	elem = &g_hudelems[elemnum];

	if ( Scr_GetNumParam() != 3 ) {
		Scr_Error( "hudelem scaleOverTime(time_in_seconds, new_width, new_height)" );
	}

	seconds = Scr_GetFloat( 0 );
	if ( seconds <= 0.0f ) {
		Scr_ParamError( 0, va( "scale time %g <= 0", seconds ) );
	} else if ( seconds > 60.0f ) {
		Scr_ParamError( 0, va( "scale time %g > 60", seconds ) );
	}

	width = Scr_GetInt( 1 );
	height = Scr_GetInt( 2 );

	elem->scaleStartTime = level.time;
	elem->scaleTime = Q_ftol( seconds * 1000.0f );
	elem->scaleFromWidth = elem->width;
	elem->scaleFromHeight = elem->height;
	elem->width = width;
	elem->height = height;
}

/*
==================
HECmd_MoveOverTime
==================
*/
static void HECmd_MoveOverTime( int elemnum ) {
	g_hudelem_t *elem;
	float       seconds;

	elem = &g_hudelems[elemnum];

	seconds = Scr_GetFloat( 0 );
	if ( seconds <= 0.0f ) {
		Scr_ParamError( 0, va( "move time %g <= 0", seconds ) );
	} else if ( seconds > 60.0f ) {
		Scr_ParamError( 0, va( "move time %g > 60", seconds ) );
	}

	elem->moveStartTime = level.time;
	elem->moveTime = Q_ftol( seconds * 1000.0f );
	elem->moveFromX = elem->x;
	elem->moveFromY = elem->y;
}

/*
==================
HECmd_Reset
==================
*/
static void HECmd_Reset( int elemnum ) {
	HudElem_Reset( &g_hudelems[elemnum] );
}

/*
==================
HECmd_Destroy
==================
*/
static void HECmd_Destroy( int elemnum ) {
	HudElem_Free( &g_hudelems[elemnum] );
}

// no terminator; HudElem_GetMethod uses the array size
// keep: defined after the methods (original data layout)
static const hudElemMethod_t hudelemMethods[] = {
	{ "settext",            HECmd_SetText },
	{ "setshader",          HECmd_SetShader },
	{ "settimer",           HECmd_SetTimer },
	{ "settimerup",         HECmd_SetTimerUp },
	{ "settenthstimer",     HECmd_SetTenthsTimer },
	{ "settenthstimerup",   HECmd_SetTenthsTimerUp },
	{ "setclock",           HECmd_SetClock },
	{ "setclockup",         HECmd_SetClockUp },
	{ "setvalue",           HECmd_SetValue },
	{ "fadeovertime",       HECmd_FadeOverTime },
	{ "scaleovertime",      HECmd_ScaleOverTime },
	{ "moveovertime",       HECmd_MoveOverTime },
	{ "reset",              HECmd_Reset },
	{ "destroy",            HECmd_Destroy }
};

/*
==================
HudElem_GetMethod

Scr_GetMethod's HUD lookup.  The caller handles the developer flag; this
lookup receives only the method name.
==================
*/
void ( *HudElem_GetMethod( const char **pName ) )( int elemnum ) {
	int i;

	for ( i = 0; i < sizeof( hudelemMethods ) / sizeof( hudelemMethods[0] ); i++ ) {
		if ( !strcmp( *pName, hudelemMethods[i].name ) ) {
			*pName = hudelemMethods[i].name;
			return hudelemMethods[i].call;
		}
	}

	return NULL;
}

/*
==================
HudElem_UpdateClient

Repacks one client's two hudelem arrays out of the global pool.  Anything past
MAX_CLIENT_HUDELEMS is counted and dropped, not written.
==================
*/
void HudElem_UpdateClient( gclient_t *client, int entNum, int flags ) {
	g_hudelem_t *elem;
	hudelem_t   *he;
	int         i;
	int         archivalCount;
	int         currentCount;

	if ( flags & HE_UPDATE_ARCHIVAL ) {
		memset( client->ps.hud.archival, 0, sizeof( client->ps.hud.archival ) );
	}
	if ( flags & HE_UPDATE_CURRENT ) {
		memset( client->ps.hud.current, 0, sizeof( client->ps.hud.current ) );
	}

	archivalCount = 0;
	currentCount = 0;

	for ( i = 0, elem = g_hudelems; i < MAX_HUDELEMS; i++, elem++ ) {
		if ( !elem->type ) {
			continue;
		}
		if ( elem->team && elem->team != client->sess.sessionTeam ) {
			continue;
		}
		if ( elem->clientNum != ENTITYNUM_NONE && elem->clientNum != entNum ) {
			continue;
		}

		if ( elem->archived ) {
			if ( !( flags & HE_UPDATE_ARCHIVAL ) ) {
				continue;
			}
			he = &client->ps.hud.archival[archivalCount++];
			if ( archivalCount > MAX_CLIENT_HUDELEMS ) {
				continue;
			}
		} else {
			if ( !( flags & HE_UPDATE_CURRENT ) ) {
				continue;
			}
			he = &client->ps.hud.current[currentCount++];
			if ( currentCount > MAX_CLIENT_HUDELEMS ) {
				continue;
			}
		}

		memcpy( he, elem, sizeof( *he ) );
	}
}
