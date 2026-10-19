//
// gameinfo.c
//

#include <string.h>
#include <stdio.h>

#include "ui_local.h"


//
// arena and bot info
//

static int ui_numArenas;
static char     *ui_arenaInfos[MAX_ARENAS];

/*
===============
UI_ParseInfos
===============
*/
int UI_ParseInfos( char *buf, int max, char *infos[] ) {
	char    *token;
	int count;
	char key[MAX_TOKEN_CHARS];
	char info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = Com_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = Com_Parse( &buf );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = Com_ParseOnLine( &buf );
			if ( !token[0] ) {
				token = "<NULL>";
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = UI_Alloc( strlen( info ) + strlen( "\\num\\" ) + strlen( va( "%d", MAX_ARENAS ) ) + 1 );
		if ( infos[count] ) {
			strcpy( infos[count], info );
			count++;
		}
	}
	return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile( char *filename ) {
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	// keep: pointer store, not buf[len]
	*( buf + len ) = 0;
	trap_FS_FCloseFile( f );

	ui_numArenas += UI_ParseInfos( buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas] );
}

/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas( void ) {
	// keep: declaration order matches original codegen
	const char *directory = "mp";
	const char *extension = ".arena";
	int numdirs;
	char filename[128];
	char dirlist[1024];
	char*       dirptr;
	int i, n, j;
	int dirlen;
	char        *type, *token, *p;

	ui_numArenas = 0;
	uiInfo.mapCount = 0;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList( directory, extension, dirlist, 1024 );
	dirptr  = dirlist;
	for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 ) {
		dirlen = strlen( dirptr );
		sprintf( filename, "%s/%s", directory, dirptr );
		UI_LoadArenasFromFile( filename );
	}
	if ( UI_OutOfMemory() ) {
		trap_Print( S_COLOR_YELLOW "WARNING: not anough memory in pool to load all arenas\n" );
	}

	for ( n = 0; n < ui_numArenas; n++ ) {
		// determine type

		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
		uiInfo.mapList[uiInfo.mapCount].mapLoadName = String_Alloc( Info_ValueForKey( ui_arenaInfos[n], "map" ) );
		uiInfo.mapList[uiInfo.mapCount].mapName = String_Alloc( Info_ValueForKey( ui_arenaInfos[n], "longname" ) );
		uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
		uiInfo.mapList[uiInfo.mapCount].imageName = String_Alloc( va( "levelshots/%s", uiInfo.mapList[uiInfo.mapCount].mapLoadName ) );

		// gametype is a list of gametype names; no key means every gametype
		type = Info_ValueForKey( ui_arenaInfos[n], "gametype" );
		if ( type && *type ) {
			uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

			Com_BeginParseSession( va( ".arena files : %s", uiInfo.mapList[uiInfo.mapCount].mapLoadName ) );
			p = type;
			while ( 1 ) {
				token = Com_Parse( &p );
				if ( !token || !token[0] ) {
					break;
				}
				for ( j = 0; j < uiInfo.numGameTypes; j++ ) {
					if ( !Q_stricmp( token, uiInfo.gameTypes[j].gameType ) ) {
						uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << j );
					}
				}
			}
			Com_EndParseSession();
		} else {
			uiInfo.mapList[uiInfo.mapCount].typeBits = -1;
		}

		uiInfo.mapCount++;
		if ( uiInfo.mapCount >= MAX_MAPS ) {
			break;
		}
	}
}
