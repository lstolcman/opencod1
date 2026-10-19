// q_parse.c -- script and text parsing shared by all program modules

#include "../qcommon/qcommon.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*
============================================================================

PARSING

============================================================================
*/

#define MAX_TOKEN_CHARS 1024

static const char *com_parseSeparators[] = {
	"+=", "-=", "*=", "/=", "&=", "|=",
	"++", "--", "&&", "||", "<=", ">=", "==", "!=",
	NULL
};

static parseInfo_t parseInfo_stack[16] = {
	{ "", 1, qfalse, qtrue, qfalse, qtrue, 1 }
};
parseInfo_t *parseInfo = parseInfo_stack;
static int parseInfo_level;

static char *prevTokenPos;
static char *lastTokenPos;

static char com_lineBuffer[MAX_TOKEN_CHARS];

void Com_BeginParseSession( const char *name ) {
	if ( parseInfo_level == 15 ) {
		Com_Error( ERR_FATAL, "\x15" "Com_BeginParseSession: session overflow" );
	}
	parseInfo_level++;
	parseInfo = &parseInfo_stack[parseInfo_level];
	parseInfo->currentLine = 1;
	parseInfo->ungetReady = qfalse;
	parseInfo->spaceDelimited = qtrue;
	parseInfo->csv = qfalse;
	Q_strncpyz( parseInfo->filename, name, sizeof( parseInfo->filename ) );
}

void Com_EndParseSession( void ) {
	if ( !parseInfo_level ) {
		Com_Error( ERR_FATAL, "\x15" "Com_EndParseSession: session underflow" );
	}
	parseInfo_level--;
	parseInfo = &parseInfo_stack[parseInfo_level];
}

void Com_ResetParseSessions( void ) {
	parseInfo_level = 0;
	parseInfo = parseInfo_stack;
}

void Com_SetSpaceDelimited( qboolean enabled ) {
	parseInfo->spaceDelimited = enabled;
}

void Com_SetCSV( qboolean enabled ) {
	parseInfo->csv = enabled;
}

void Com_SetParseNegativeNumbers( qboolean enabled ) {
	parseInfo->parseNegativeNumbers = enabled;
}

int Com_GetCurrentParseLine( void ) {
	return parseInfo->currentLine;
}

void Com_ScriptError( const char *fmt, ... ) {
	va_list argptr;
	char string[32000];

	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	Com_Error( ERR_DROP, "\x15" "File %s, line %i: %s", parseInfo->filename, parseInfo->currentLine, string );
}

void Com_ScriptWarning( const char *fmt, ... ) {
	va_list argptr;
	char string[32000];

	va_start( argptr, fmt );
	vsprintf( string, fmt, argptr );
	va_end( argptr );

	Com_Printf( "File %s, line %i: %s", parseInfo->filename, parseInfo->currentLine, string );
}

void Com_UngetToken( void ) {
	if ( parseInfo->ungetReady ) {
		Com_ScriptError( "UngetToken called twice" );
	}
	parseInfo->ungetReady = qtrue;
	lastTokenPos = prevTokenPos;
}

void Com_ParseSetMark( int *mark, char **data_p ) {
	mark[0] = parseInfo->currentLine;
	mark[1] = (int)*data_p;
	mark[2] = parseInfo->ungetReady;
	mark[3] = parseInfo->ungetLineSave;
	mark[4] = (int)parseInfo->ungetTokenSave;
}

void Com_ParseReturnToMark( int *mark, char **data_p ) {
	parseInfo->currentLine = mark[0];
	*data_p = (char *)mark[1];
	parseInfo->ungetReady = (qboolean)mark[2];
	parseInfo->ungetLineSave = mark[3];
	parseInfo->ungetTokenSave = (char *)mark[4];
}

static char *SkipWhitespace( char *data, qboolean *hasNewLines ) {
	int c;

	while ( ( c = *data ) <= ' ' ) {
		if ( !c ) {
			return NULL;
		}
		if ( c == '\n' ) {
			parseInfo->currentLine++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

int Com_Compress( char *data_p ) {
	char    *datai, *datao;
	char c;
	int size;

	size = 0;
	datao = datai = data_p;
	if ( datai ) {
		while ( ( c = *datai ) != 0 ) {
			// keep: newlines are copied in their own arm, ahead of the comment tests
			if ( c == '\r' || c == '\n' ) {
				*datao++ = c;
				datai++;
				size++;
			} else if ( c == '/' && datai[1] == '/' ) {
				// skip double slash comments
				while ( *datai && *datai != '\n' ) {
					datai++;
				}
			}
			else if ( c == '/' && datai[1] == '*' ) {
				// skip /* */ comments
				if ( *datai ) {
					while ( *datai && ( *datai != '*' || datai[1] != '/' ) ) {
						datai++;
					}
					if ( *datai ) {
						datai += 2;
					}
				}
			} else {
				*datao++ = c;
				datai++;
				size++;
			}
		}
	}
	*datao = 0;
	return size;
}

char *Com_GetLastTokenPos( void ) {
	return lastTokenPos;
}

static char *Com_ParseCSV( qboolean allowLineBreaks, char **data_p ) {
	char    *data;
	int len;

	len = 0;
	data = *data_p;
	parseInfo->token[0] = 0;

	// keep: the loops stay in the else arms (here and for '"' below)
	if ( !allowLineBreaks ) {
		if ( *data == '\r' || *data == '\n' ) {
			return parseInfo->token;
		}
	} else {
		while ( *data == '\r' || *data == '\n' ) {
			data++;
		}
	}

	prevTokenPos = lastTokenPos;
	lastTokenPos = data;

	while ( *data && *data != ',' && *data != '\n' ) {
		if ( *data == '\r' ) {
		} else if ( *data != '"' ) {
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				parseInfo->token[len++] = *data;
			}
		} else {
			data++;
			while ( 1 ) {
				if ( *data == '"' ) {
					if ( data[1] != '"' ) {
						break;
					}
					if ( len < MAX_TOKEN_CHARS - 1 ) {
						parseInfo->token[len++] = '"';
					}
					data += 2;
				} else {
					if ( len < MAX_TOKEN_CHARS - 1 ) {
						parseInfo->token[len++] = *data;
					}
					data++;
				}
			}
		}
		data++;
	}

	if ( *data ) {
		if ( *data != '\n' ) {
			data++;
		}
		*data_p = data;
	} else {
		*data_p = NULL;
	}
	parseInfo->token[len] = 0;
	return parseInfo->token;
}

/*
==============
Com_ParseExt

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
char *Com_ParseExt( char **data_p, qboolean allowLineBreaks ) {
	char c = 0;
	int len;
	qboolean hasNewLines = qfalse;
	char    *data;
	const char **punc;

	if ( !data_p ) {
		Com_Error( ERR_FATAL, "\x15" "Com_ParseExt: NULL data_p" );
	}
	data = *data_p;
	len = 0;
	parseInfo->token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return parseInfo->token;
	}

	// backup the session data so we can unget easily
	parseInfo->ungetLineSave = parseInfo->currentLine;
	parseInfo->ungetTokenSave = *data_p;

	if ( parseInfo->csv ) {
		return Com_ParseCSV( allowLineBreaks, data_p );
	}

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data ) {
			*data_p = NULL;
			return parseInfo->token;
		}
		if ( hasNewLines && !allowLineBreaks ) {
			*data_p = data;
			return parseInfo->token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' ) {
			while ( *data && *data != '\n' ) {
				data++;
			}
			continue;
		}

		// skip /* */ comments
		if ( c == '/' && data[1] == '*' ) {
			while ( *data && ( *data != '*' || data[1] != '/' ) ) {
				if ( *data == '\n' ) {
					parseInfo->currentLine++;
				}
				data++;
			}
			if ( *data ) {
				data += 2;
			}
			continue;
		}

		break;
	}

	prevTokenPos = lastTokenPos;
	lastTokenPos = data;

	// handle quoted strings
	if ( c == '\"' ) {
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\\' && *data == '\"' ) {
				c = '\"';
				data++;
			} else if ( c == '\"' || !c ) {
				parseInfo->token[len] = 0;
				*data_p = data;
				return parseInfo->token;
			} else if ( *data == '\n' ) {
				parseInfo->currentLine++;
			}
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				parseInfo->token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	if ( parseInfo->spaceDelimited ) {
		do
		{
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				parseInfo->token[len] = c;
				len++;
			}
			data++;
			c = *data;
		} while ( c > ' ' );

		if ( len == MAX_TOKEN_CHARS ) {
			len = 0;
		}
		parseInfo->token[len] = 0;
		*data_p = data;
		return parseInfo->token;
	}

	if ( ( c >= '0' && c <= '9' ) ||
		 ( parseInfo->parseNegativeNumbers && c == '-' && data[1] >= '0' && data[1] <= '9' ) ||
		 ( c == '.' && data[1] >= '0' && data[1] <= '9' ) ) {
		do {
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				parseInfo->token[len] = c;
				len++;
			}
			data++;
			c = *data;
		} while ( ( c >= '0' && c <= '9' ) || c == '.' );

		if ( c == 'e' || c == 'E' ) {
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				parseInfo->token[len] = c;
				len++;
			}
			data++;
			c = *data;
			if ( c == '-' || c == '+' ) {
				if ( len < MAX_TOKEN_CHARS - 1 ) {
					parseInfo->token[len] = c;
					len++;
				}
				data++;
				c = *data;
			}
			// keep: do-while, the first character after the exponent is always taken
			do {
				if ( len < MAX_TOKEN_CHARS - 1 ) {
					parseInfo->token[len] = c;
					len++;
				}
				data++;
				c = *data;
			} while ( c >= '0' && c <= '9' );
		}

		if ( len == MAX_TOKEN_CHARS ) {
			len = 0;
		}
		parseInfo->token[len] = 0;
		*data_p = data;
		return parseInfo->token;
	}

	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
		 c == '_' || c == '/' || c == '\\' ) {
		do {
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				parseInfo->token[len] = c;
				len++;
			}
			data++;
			c = *data;
		} while ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
				  c == '_' || ( c >= '0' && c <= '9' ) ||
				  c == '/' || c == '\\' || c == ':' || c == '.' );

		if ( len == MAX_TOKEN_CHARS ) {
			len = 0;
		}
		parseInfo->token[len] = 0;
		*data_p = data;
		return parseInfo->token;
	}

	for ( punc = com_parseSeparators; *punc; punc++ ) {
		int l;
		int j;

		l = strlen( *punc );
		for ( j = 0; j < l; j++ ) {
			if ( data[j] != ( *punc )[j] ) {
				break;
			}
		}
		if ( j == l ) {
			memcpy( parseInfo->token, *punc, l );
			parseInfo->token[l] = 0;
			data += l;
			*data_p = data;
			return parseInfo->token;
		}
	}

	parseInfo->token[0] = *data;
	parseInfo->token[1] = 0;
	data++;
	*data_p = data;
	return parseInfo->token;
}

char *Com_Parse( char **data_p ) {
	if ( parseInfo->ungetReady ) {
		parseInfo->ungetReady = qfalse;
		*data_p = parseInfo->ungetTokenSave;
		parseInfo->currentLine = parseInfo->ungetLineSave;
	}
	return Com_ParseExt( data_p, qtrue );
}

char *Com_ParseOnLine( char **data_p ) {
	if ( parseInfo->ungetReady ) {
		parseInfo->ungetReady = qfalse;
		if ( !parseInfo->spaceDelimited ) {
			return parseInfo->token;
		}
		*data_p = parseInfo->ungetTokenSave;
		parseInfo->currentLine = parseInfo->ungetLineSave;
	}
	return Com_ParseExt( data_p, qfalse );
}

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( char **data_p, const char *match, int warn ) {
	char    *token;

	token = Com_Parse( data_p );

	if ( strcmp( token, match ) ) {
		if ( warn ) {
			Com_ScriptWarning( "MatchToken: %s != %s", token, match );
		} else {
			Com_ScriptError( "MatchToken: %s != %s", token, match );
		}
	}
}

/*
=================
Com_SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int Com_SkipBracedSection( char **data_p, int depth ) {
	char    *token;
	int braceLevel;
	qboolean found;

	found = qfalse;
	braceLevel = 0;
	do {
		token = Com_Parse( data_p );
		if ( token[1] == 0 ) {
			if ( token[0] == '{' ) {
				if ( braceLevel == depth ) {
					found = qtrue;
				} else {
					braceLevel++;
				}
			} else if ( token[0] == '}' ) {
				braceLevel--;
			}
		}
	} while ( braceLevel && *data_p );
	return found;
}

/*
=================
Com_SkipRestOfLine
=================
*/
// the exe version takes data_p alone; the dll versions keep a leading unused argument
#if !defined( UIDLL ) && !defined( GAMEDLL ) && !defined( CGAMEDLL )
void Com_SkipRestOfLine( char **data_p ) {
	char    *p;
	int c;
#else
void Com_SkipRestOfLine( int unused, char **data_p ) {
	char    *p;
	int c;

	(void)unused;
#endif

	p = *data_p;
	if ( !p ) {
		return;
	}
	while ( ( c = *p ) != 0 ) {
		p++;
		if ( c == '\n' ) {
			parseInfo->currentLine++;
			break;
		}
	}
	*data_p = p;
}

char *Com_ParseRestOfLine( char **data_p ) {
	char    *token;

	com_lineBuffer[0] = 0;
	while ( 1 ) {
		token = Com_ParseOnLine( data_p );
		if ( !token[0] ) {
			break;
		}
		if ( com_lineBuffer[0] ) {
			Q_strcat( com_lineBuffer, sizeof( com_lineBuffer ), " " );
		}
		Q_strcat( com_lineBuffer, sizeof( com_lineBuffer ), token );
	}

	return com_lineBuffer;
}

float Com_ParseFloat( char **data_p ) {
	char    *token;

	token = Com_Parse( data_p );
	if ( !token[0] ) {
		return 0;
	}
	return (float)atof( token );
}

int Com_ParseInt( char **data_p ) {
	char    *token;

	token = Com_Parse( data_p );
	if ( !token[0] ) {
		return 0;
	}
	return atoi( token );
}

void Parse_1DMatrix( char **data_p, int x, float *m ) {
	char    *token;
	int i;

	COM_MatchToken( data_p, "(", 0 );

	for ( i = 0 ; i < x ; i++ ) {
		token = Com_Parse( data_p );
		m[i] = (float)atof( token );
	}

	COM_MatchToken( data_p, ")", 0 );
}

void Parse_2DMatrix( char **data_p, int y, int x, float *m ) {
	int i;

	COM_MatchToken( data_p, "(", 0 );

	for ( i = 0 ; i < y ; i++ ) {
		Parse_1DMatrix( data_p, x, m + i * x );
	}

	COM_MatchToken( data_p, ")", 0 );
}

void Parse_3DMatrix( char **data_p, int z, int y, int x, float *m ) {
	int i;

	COM_MatchToken( data_p, "(", 0 );

	for ( i = 0 ; i < z ; i++ ) {
		Parse_2DMatrix( data_p, y, x, m + i * x * y );
	}

	COM_MatchToken( data_p, ")", 0 );
}
