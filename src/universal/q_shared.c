// q_shared.c -- stateless support routines that are included in each code dll

#include "../qcommon/qcommon.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct {
	byte b0, b1, b2, b3, b4, b5, b6, b7;
} qint64;

const vec4_t colorBlack     = { 0.00f, 0.00f, 0.00f, 1.0f };
const vec4_t colorRed       = { 1.00f, 0.00f, 0.00f, 1.0f };
const vec4_t colorGreen     = { 0.00f, 1.00f, 0.00f, 1.0f };
const vec4_t colorLtGreen   = { 0.00f, 0.70f, 0.00f, 1.0f };
const vec4_t colorBlue      = { 0.00f, 0.00f, 1.00f, 1.0f };
const vec4_t colorYellow    = { 1.00f, 1.00f, 0.00f, 1.0f };
const vec4_t colorLtYellow  = { 0.75f, 0.75f, 0.00f, 1.0f };
const vec4_t colorMdYellow  = { 0.50f, 0.50f, 0.00f, 1.0f };
const vec4_t colorMagenta   = { 1.00f, 0.00f, 1.00f, 1.0f };
const vec4_t colorCyan      = { 0.00f, 1.00f, 1.00f, 1.0f };
const vec4_t colorLtCyan    = { 0.00f, 0.75f, 0.75f, 1.0f };
const vec4_t colorMdCyan    = { 0.00f, 0.50f, 0.50f, 1.0f };
const vec4_t colorDkCyan    = { 0.00f, 0.25f, 0.25f, 1.0f };
const vec4_t colorWhite     = { 1.00f, 1.00f, 1.00f, 1.0f };
const vec4_t colorLtGrey    = { 0.75f, 0.75f, 0.75f, 1.0f };
const vec4_t colorMdGrey    = { 0.50f, 0.50f, 0.50f, 1.0f };
const vec4_t colorDkGrey    = { 0.25f, 0.25f, 0.25f, 1.0f };
const vec4_t colorOrange    = { 1.00f, 0.70f, 0.00f, 1.0f };
const vec4_t colorLtOrange  = { 0.75f, 0.525f, 0.00f, 1.0f };

const vec4_t g_color_table[8] = {
	{ 0.00f, 0.00f, 0.00f, 1.0f },      // COLOR_BLACK   '0'
	{ 1.00f, 0.00f, 0.00f, 1.0f },      // COLOR_RED     '1'
	{ 0.00f, 1.00f, 0.00f, 1.0f },      // COLOR_GREEN   '2'
	{ 1.00f, 1.00f, 0.00f, 1.0f },      // COLOR_YELLOW  '3'
	{ 0.00f, 0.00f, 1.00f, 1.0f },      // COLOR_BLUE    '4'
	{ 0.00f, 1.00f, 1.00f, 1.0f },      // COLOR_CYAN    '5'
	{ 1.00f, 0.00f, 1.00f, 1.0f },      // COLOR_MAGENTA '6'
	{ 1.00f, 1.00f, 1.00f, 1.0f },      // COLOR_WHITE   '7'
};

float Com_Clamp( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

/*
============
Com_SkipPath
============
*/
char *Com_SkipPath( char *pathname ) {
	char    *last;

	last = pathname;
	while ( *pathname )
	{
		if ( *pathname == '/' ) {
			last = pathname + 1;
		}
		pathname++;
	}
	return last;
}

/*
============
Com_StripExtension
============
*/
void Com_StripExtension( const char *in, char *out ) {
	while ( *in && *in != '.' ) {
		*out++ = *in++;
	}
	*out = 0;
}

void Com_StripFilename( char *in, char *out ) {
	char *end;
	Q_strncpyz( out, in, strlen( in ) );
	end = Com_SkipPath( out );
	*end = 0;
}

/*
==================
Com_DefaultExtension
==================
*/
void Com_DefaultExtension( char *path, int maxSize, const char *extension ) {
	char oldPath[MAX_QPATH];
	char    *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen( path ) - 1;

	while ( *src != '/' && src != path ) {
		if ( *src == '.' ) {
			return;                 // it has an extension
		}
		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

//============================================================================
/*
==================
Com_BitCheck

  Allows bit-wise checks on arrays with more than one item (> 32 bits)
==================
*/
qboolean Com_BitCheck( const int array[], int bitNum ) {
	return ( ( array[bitNum >> 5] & ( 1 << ( bitNum & 31 ) ) ) != 0 );
}

/*
==================
Com_BitSet

  Allows bit-wise SETS on arrays with more than one item (> 32 bits)
==================
*/
void Com_BitSet( int array[], int bitNum ) {
	array[bitNum >> 5] |= ( 1 << ( bitNum & 31 ) );
}

/*
==================
Com_BitClear

  Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
==================
*/
void Com_BitClear( int array[], int bitNum ) {
	array[bitNum >> 5] &= ~( 1 << ( bitNum & 31 ) );
}
//============================================================================

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short ( *_BigShort )( short l );
static short ( *_LittleShort )( short l );
static int ( *_BigLong )( int l );
static int ( *_LittleLong )( int l );
static qint64 ( *_BigLong64 )( qint64 l );
static qint64 ( *_LittleLong64 )( qint64 l );
static float ( *_BigFloat )( float l );
static float ( *_LittleFloat )( float l );

// keep: only the five wrappers that are used -- the exe's float swap is
// LittleFloat, the dlls' is BigFloat; unused extern copies are not stripped
#if !defined( UIDLL ) && !defined( GAMEDLL ) && !defined( CGAMEDLL )
#define Q_SHARED_EXE_SWAPS
#endif
short   BigShort( short l ) {return _BigShort( l );}
int     BigLong( int l ) {return _BigLong( l );}
qint64  BigLong64( qint64 l ) {return _BigLong64( l );}
qint64  LittleLong64( qint64 l ) {return _LittleLong64( l );}
#ifdef Q_SHARED_EXE_SWAPS
float   LittleFloat( float l ) {return _LittleFloat( l );}
#else
float   BigFloat( float l ) {return _BigFloat( l );}
#endif

short   ShortSwap( short l ) {
	byte b1,b2;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;

	return ( b1 << 8 ) + b2;
}

short   ShortNoSwap( short l ) {
	return l;
}

int    LongSwap( int l ) {
	byte b1,b2,b3,b4;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;
	b3 = ( l >> 16 ) & 255;
	b4 = ( l >> 24 ) & 255;

	return ( (int)b1 << 24 ) + ( (int)b2 << 16 ) + ( (int)b3 << 8 ) + b4;
}

int LongNoSwap( int l ) {
	return l;
}

qint64 Long64Swap( qint64 ll ) {
	qint64 result;

	result.b0 = ll.b7;
	result.b1 = ll.b6;
	result.b2 = ll.b5;
	result.b3 = ll.b4;
	result.b4 = ll.b3;
	result.b5 = ll.b2;
	result.b6 = ll.b1;
	result.b7 = ll.b0;

	return result;
}

qint64 Long64NoSwap( qint64 ll ) {
	return ll;
}

float FloatSwap( float f ) {
	union
	{
		float f;
		byte b[4];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap( float f ) {
	return f;
}

/*
================
Swap_Init
================
*/
void Swap_Init( void ) {
	byte swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner
	if ( *(short *)swaptest == 1 ) {
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigLong64 = Long64Swap;
		_LittleLong64 = Long64NoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	} else
	{
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigLong64 = Long64NoSwap;
		_LittleLong64 = Long64Swap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c ) {
	if ( c >= 0x20 && c <= 0x7E ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_islower( int c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isupper( int c ) {
	if ( c >= 'A' && c <= 'Z' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isalpha( int c ) {
	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isnumeric( int c ) {
	if ( c >= '0' && c <= '9' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isalphanumeric( int c ) {
	if ( Q_isalpha( c ) || Q_isnumeric( c ) ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isnamechar( int c ) {
	if ( ( Q_isalphanumeric( c ) || c == '_' || c == '-' ) && c != ' ' ) {
		return ( 1 );
	}
	return ( 0 );
}

char* Q_strrchr( const char* string, int c ) {
	char cc = c;
	char *s;
	char *sp = (char *)0;

	s = (char*)string;

	while ( *s )
	{
		if ( *s == cc ) {
			sp = s;
		}
		s++;
	}
	if ( cc == 0 ) {
		sp = s;
	}

	return sp;
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
(no NULL src / destsize checks in this version)
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {
	strncpy( dest, src, destsize - 1 );
	dest[destsize - 1] = 0;
}

int Q_stricmpn( const char *s1, const char *s2, int n ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;       // strings are equal until end point
		}

		if ( c1 != c2 ) {
			if ( Q_islower( c1 ) ) {
				c1 -= ( 'a' - 'A' );
			}
			if ( Q_islower( c2 ) ) {
				c2 -= ( 'a' - 'A' );
			}
			if ( c1 != c2 ) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while ( c1 );

	return 0;       // strings are equal
}

int Q_strncmp( const char *s1, const char *s2, int n ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;       // strings are equal until end point
		}

		if ( c1 != c2 ) {
			return c1 < c2 ? -1 : 1;
		}
	} while ( c1 );

	return 0;       // strings are equal
}

int Q_stricmp( const char *s1, const char *s2 ) {
	return ( s1 && s2 ) ? Q_stricmpn( s1, s2, 99999 ) : -1;
}

char *Q_strlwr( char *s1 ) {
	char    *s;

	s = s1;
	while ( *s ) {
		*s = tolower( *s );
		s++;
	}
	return s1;
}

char *Q_strupr( char *s1 ) {
	char    *s;

	s = s1;
	while ( *s ) {
		*s = toupper( *s );
		s++;
	}
	return s1;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "\x15" "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

char *Q_CleanStr( char *string ) {
	char*   d;
	char*   s;
	char c;

	s = string;
	d = string;
	while ( ( c = *s ) != 0 ) {
		if ( s[0] == '^' && s[1] && s[1] != '^' && s[1] >= '0' && s[1] <= '7' ) {
			s++;
		} else if ( c >= 0x20 && c <= 0x7E )   {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

// maps a typographic apostrophe to ' and anything else above 0x7F to '.'
char Q_CleanCharacter( char c ) {
	if ( c == (char)0x92 ) {
		return '\'';
	}
	if ( (unsigned char)c > 0x7F ) {
		return '.';
	}
	return c;
}

int QDECL Com_sprintf( char *dest, int size, const char *fmt, ... ) {
	int len;
	va_list argptr;

	va_start( argptr,fmt );
	len = _vsnprintf( dest, size, fmt, argptr );
	va_end( argptr );

	dest[size - 1] = 0;

	return len;
}

int Q_strncasecmp( char *s1, char *s2, int n ) {
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;       // strings are equal until end point

		}
		if ( c1 != c2 ) {
			if ( Q_islower( c1 ) ) {
				c1 -= ( 'a' - 'A' );
			}
			if ( Q_islower( c2 ) ) {
				c2 -= ( 'a' - 'A' );
			}
			if ( c1 != c2 ) {
				return -1;      // strings not equal
			}
		}
	} while ( c1 );

	return 0;       // strings are equal
}

int Q_strcasecmp( char *s1, char *s2 ) {
	return Q_strncasecmp( s1, s2, 99999 );
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.

modified this into a circular list, to further prevent stepping on
previous strings
============
*/
char    * QDECL va( char *format, ... ) {
	va_list argptr;
	#define MAX_VA_STRING   32000
	static char temp_buffer[MAX_VA_STRING];
	static char string[MAX_VA_STRING];      // in case va is called by nested functions
	static int index = 0;
	char    *buf;
	int len;

	va_start( argptr, format );
	len = _vsnprintf( temp_buffer, MAX_VA_STRING, format, argptr );
	va_end( argptr );
	temp_buffer[MAX_VA_STRING - 1] = 0;

	if ( len < 0 || len >= MAX_VA_STRING ) {
		Com_Error( ERR_DROP, "\x15" "Attempted to overrun string in call to va()\n" );
	}

	if ( len + index >= MAX_VA_STRING - 1 ) {
		index = 0;
	}

	buf = &string[index];
	memcpy( buf, temp_buffer, len + 1 );

	index += len + 1;

	return buf;
}

/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float   *tv( float x, float y, float z ) {
	static int index;
	static vec3_t vecs[8];
	float   *v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = ( index + 1 ) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char pkey[BIG_INFO_KEY];
	static char value[2][BIG_INFO_VALUE];   // use two buffers so compares
	                                        // work without stomping on each other
	static int valueindex = 0;
	char    *o;

	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "\x15" "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;
	if ( *s == '\\' ) {
		s++;
	}
	while ( 1 )
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s ) {
				return "";
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while ( *s != '\\' && *s )
		{
			*o++ = *s++;
		}
		*o = 0;

		if ( !Q_stricmp( key, pkey ) ) {
			return value[valueindex];
		}

		if ( !*s ) {
			break;
		}
		s++;
	}

	return "";
}

/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char *key, char *value ) {
	char    *o;
	const char  *s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}

/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char    *start;
	char pkey[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	char    *o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "\x15" "Info_RemoveKey: oversize infostring" );
	}

	if ( strchr( key, '\\' ) ) {
		return;
	}

	while ( 1 )
	{
		start = s;
		if ( *s == '\\' ) {
			s++;
		}
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while ( *s != '\\' && *s )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) ) {
			strcpy( start, s );
			return;
		}

		if ( !*s ) {
			return;
		}
	}

}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big( char *s, const char *key ) {
	char    *start;
	char pkey[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];
	char    *o;

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "\x15" "Info_RemoveKey_Big: oversize infostring" );
	}

	if ( strchr( key, '\\' ) ) {
		return;
	}

	while ( 1 )
	{
		start = s;
		if ( *s == '\\' ) {
			s++;
		}
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while ( *s != '\\' && *s )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) ) {
			strcpy( start, s );
			return;
		}

		if ( !*s ) {
			return;
		}
	}

}

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char newi[MAX_INFO_STRING];
	char cleanValue[MAX_INFO_STRING];
	int i, j;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "\x15" "Info_SetValueForKey: oversize infostring" );
	}

	j = 0;
	for ( i = 0 ; i < MAX_INFO_STRING - 1 ; i++ ) {
		char c = value[i];
		if ( !c ) {
			break;
		}
		if ( c != '\\' && c != ';' && c != '\"' ) {
			cleanValue[j++] = c;
		}
	}
	*( cleanValue + j ) = 0;    // keep: pointer form, not cleanValue[j]

	if ( strchr( key, '\\' ) ) {
		Com_Error( ERR_DROP, "\x15" "Can't use keys with a \\\nkey: '%s'\nvalue: '%s'",
				   key, value );
	}
	if ( strchr( key, ';' ) ) {
		Com_Error( ERR_DROP, "\x15" "Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'",
				   key, value );
	}
	if ( strchr( key, '\"' ) ) {
		Com_Error( ERR_DROP, "\x15" "Can't use keys with a \"\nkey: '%s'\nvalue: '%s'",
				   key, value );
	}

	Info_RemoveKey( s, key );
	if ( !cleanValue[0] ) {
		return;
	}

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, cleanValue );

	if ( strlen( newi ) + strlen( s ) > MAX_INFO_STRING ) {
		Com_Error( ERR_DROP,
				   "\x15" "Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string:\n%s\n",
				   key, value, s );
	}

	strcat( s, newi );
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey_Big( char *s, const char *key, const char *value ) {
	char newi[BIG_INFO_STRING];
	char cleanValue[BIG_INFO_STRING];
	int i, j;

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "\x15" "Info_SetValueForKey: oversize infostring" );
	}

	j = 0;
	for ( i = 0 ; i < BIG_INFO_STRING - 1 ; i++ ) {
		char c = value[i];
		if ( !c ) {
			break;
		}
		if ( c != '\\' && c != ';' && c != '\"' ) {
			cleanValue[j++] = c;
		}
	}
	*( cleanValue + j ) = 0;    // keep: pointer form, not cleanValue[j]

	if ( strchr( key, '\\' ) ) {
		Com_Error( ERR_DROP, "\x15" "Can't use keys with a \\\nkey: '%s'\nvalue: '%s'",
				   key, value );
	}
	if ( strchr( key, ';' ) ) {
		Com_Error( ERR_DROP, "\x15" "Can't use keys with a semicolon\nkey: '%s'\nvalue: '%s'",
				   key, value );
	}
	if ( strchr( key, '\"' ) ) {
		Com_Error( ERR_DROP, "\x15" "Can't use keys with a \"\nkey: '%s'\nvalue: '%s'",
				   key, value );
	}

	Info_RemoveKey_Big( s, key );
	if ( !cleanValue[0] ) {
		return;
	}

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, cleanValue );

	if ( strlen( newi ) + strlen( s ) > BIG_INFO_STRING ) {
		Com_Error( ERR_DROP,
				   "\x15" "BIG Info string length exceeded\nkey: '%s'\nvalue: '%s'\nInfo string:\n%s\n",
				   key, value, s );
	}

	strcat( s, newi );
}

//====================================================================

typedef enum {
	CS_FIELD_CUSTOM     = 0,
	CS_FIELD_STRING1024 = 1,
	CS_FIELD_STRING64   = 2,
	CS_FIELD_STRING256  = 3,
	CS_FIELD_INT        = 4,
	CS_FIELD_QBOOLEAN   = 5,
	CS_FIELD_FLOAT      = 6,
	CS_FIELD_MILLISECONDS = 7,
} csFieldType_t;

typedef struct {
	const char  *name;
	int offset;
	int type;
} csField_t;

/*
==================
ParseConfigStringToStruct

Fills in a structure from the keys of an info string, one field table
entry per key.  Types 8 and up go to the custom handler.
==================
*/
#if defined( UIDLL )
// ui takes the structure first; game and cgame take the field table first
qboolean ParseConfigStringToStruct( void *structBase, const csField_t *fields, int numFields,
#else
qboolean ParseConfigStringToStruct( const csField_t *fields, void *structBase, int numFields,
#endif
									const char *s, int numCustomTypes,
									qboolean ( *customHandler )( void *base, const char *value, int type ),
									void ( *vectorHandler )( void *dest, const char *value ) ) {
	int i;
	char *value;

	for ( i = 0; i < numFields; i++ ) {
		value = Info_ValueForKey( s, fields[i].name );
		if ( !*value ) {
			continue;
		}

		if ( fields[i].type < 8 ) {
			switch ( fields[i].type ) {
			case CS_FIELD_CUSTOM:
				vectorHandler( (byte *)structBase + fields[i].offset, value );
				break;
			case CS_FIELD_STRING1024:
				Q_strncpyz( (char *)( (byte *)structBase + fields[i].offset ), value, 1024 );
				break;
			case CS_FIELD_STRING64:
				Q_strncpyz( (char *)( (byte *)structBase + fields[i].offset ), value, 64 );
				break;
			case CS_FIELD_STRING256:
				Q_strncpyz( (char *)( (byte *)structBase + fields[i].offset ), value, 256 );
				break;
			case CS_FIELD_INT:
				*(int *)( (byte *)structBase + fields[i].offset ) = atoi( value );
				break;
			case CS_FIELD_QBOOLEAN:
				*(int *)( (byte *)structBase + fields[i].offset ) = ( atoi( value ) != 0 );
				break;
			case CS_FIELD_FLOAT:
				*(float *)( (byte *)structBase + fields[i].offset ) = (float)atof( value );
				break;
			case CS_FIELD_MILLISECONDS:
				// keep: FCAST, the double is never rounded to float
				*(int *)( (byte *)structBase + fields[i].offset ) = (int)( FCAST( atof( value ) ) * 1000.0f );
				break;
			default:
				break;
			}
		} else if ( numCustomTypes <= 0 || fields[i].type >= numCustomTypes ) {
			Com_Error( ERR_DROP, "\x15" "Bad field type %i\n", fields[i].type );
		} else if ( !customHandler( structBase, value, fields[i].type ) ) {
			return qfalse;
		}
	}

	return i == numFields;  // keep: not qtrue, differs for numFields < 0
}

#if !defined( GAMEDLL ) && !defined( CGAMEDLL )
// exe and ui only; game and cgame use bg_pmove.c's PM_FloatAbs
static __inline float Q_fabs( float f ) {
	int tmp = *( int * ) &f & 0x7FFFFFFF;
	return *( float * ) &tmp;
}
#endif

#if defined( GAMEDLL ) || defined( CGAMEDLL )
float PM_FloatAbs( float value );
#endif

float GetLeanFraction( float f ) {
#if defined( GAMEDLL ) || defined( CGAMEDLL )
	return ( 2.0f - PM_FloatAbs( f ) ) * f;
#else
	return ( 2.0f - Q_fabs( f ) ) * f;
#endif
}

float UnGetLeanFraction( float f ) {
	return 1.0f - FCAST( sqrt( 1.0f - f ) );    // keep: FCAST, not rounded to float
}

void AddLeanToPosition( float *pos, float yaw, float leanFrac, float leanDist, float scale ) {
	float frac;
	vec3_t angles;
	vec3_t right;

	if ( leanFrac == 0.0f ) {
		return;
	}

	frac = GetLeanFraction( leanFrac );

	VectorSet( angles, 0, yaw, frac * leanDist );   // keep: VectorSet, not three statements
	AngleVectors( angles, NULL, right, NULL );

	frac = frac * scale;

	pos[0] += right[0] * frac;
	pos[1] += right[1] * frac;
	pos[2] += right[2] * frac;
}

// keep: term order in the Orientation* functions matches original codegen
void OrientationPosToWorldPos( const float *orient, const float *pos, float *out ) {
	out[0] = (float)( (rfloat)orient[9] * pos[2] + (rfloat)orient[3] * pos[0] + (rfloat)orient[6] * pos[1] + orient[0] );
	out[1] = (float)( (rfloat)orient[10] * pos[2] + (rfloat)orient[4] * pos[0] + (rfloat)orient[7] * pos[1] + orient[1] );
	out[2] = (float)( (rfloat)orient[11] * pos[2] + (rfloat)orient[5] * pos[0] + (rfloat)orient[8] * pos[1] + orient[2] );
}

void OrientationDirToWorldDir( const float *orient, const float *dir, float *out ) {
	out[0] = (float)( (rfloat)orient[9] * dir[2] + (rfloat)orient[3] * dir[0] + (rfloat)orient[6] * dir[1] );
	out[1] = (float)( (rfloat)orient[10] * dir[2] + (rfloat)orient[4] * dir[0] + (rfloat)orient[7] * dir[1] );
	out[2] = (float)( (rfloat)orient[11] * dir[2] + (rfloat)orient[5] * dir[0] + (rfloat)orient[8] * dir[1] );
}

void OrientationPosFromWorldPos( const float *orient, const float *worldPos, float *out ) {
	rfloat d0, d1, d2;

	d0 = (rfloat)worldPos[0] - orient[0];
	d1 = (rfloat)worldPos[1] - orient[1];
	d2 = (rfloat)worldPos[2] - orient[2];

	out[0] = (float)( d2 * orient[5] + d1 * orient[4] + d0 * orient[3] );
	out[1] = (float)( d2 * orient[8] + d1 * orient[7] + d0 * orient[6] );
	out[2] = (float)( d2 * orient[11] + d1 * orient[10] + d0 * orient[9] );
}

void OrientationDirFromWorldDir( const float *orient, const float *worldDir, float *out ) {
	out[0] = (float)( (rfloat)orient[5] * worldDir[2] + (rfloat)orient[3] * worldDir[0] + (rfloat)orient[4] * worldDir[1] );
	out[1] = (float)( (rfloat)orient[8] * worldDir[2] + (rfloat)orient[6] * worldDir[0] + (rfloat)orient[7] * worldDir[1] );
	out[2] = (float)( (rfloat)orient[11] * worldDir[2] + (rfloat)orient[9] * worldDir[0] + (rfloat)orient[10] * worldDir[1] );
}

// exe only
#if !defined( UIDLL ) && !defined( GAMEDLL ) && !defined( CGAMEDLL )
int Q_vsnprintf( char *dest, size_t size, const char *fmt, va_list ap ) {
	int len;

	if ( !size ) {
		return 0;
	}

	len = _vsnprintf( dest, size, fmt, ap );
	dest[size - 1] = 0;

	return len < 0 ? (int) size : len;
}
#endif
