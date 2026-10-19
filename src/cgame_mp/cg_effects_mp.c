// cg_effects.c -- loads the impact effect tables from fx/*.csv
//
// Each CSV row is: effect type, surface type, effect file.  An empty third
// column means "no effect here" on purpose and is not reported as missing.

#include <string.h>
#include <stdlib.h>

#include "cg_local.h"

#define NUM_IMPACT_EFFECT_TYPES     9

#define MAX_EFFECT_FILENAME         64

#define MAX_EFFECT_FILE_TEXT        0x10000
#define MAX_EFFECT_FILES            4096

void        CG_Printf( const char *msg, ... );
void        CG_Error( const char *msg, ... );

/*
==============
CG_RegisterImpactEffectsOfType

Registers one effect type's surface entries and returns how many rows the
CSVs never supplied.
==============
*/
static int CG_RegisterImpactEffectsOfType( char ( *fileNames )[MAX_EFFECT_FILENAME],
										   const char *effectType, int *effects ) {
	int missing;
	int i;

	missing = 0;

	for ( i = 0 ; i < SURF_TYPE_NUM ; i++ ) {
		if ( !fileNames[i][0] ) {
			// byte 1 set means the file left this entry blank on purpose
			if ( !fileNames[i][1] ) {
				CG_Printf( "no entry for effect type '%s' on surface type '%s'\n",
						   effectType, trap_SurfaceTypeToName( i ) );
				missing++;
			}
			effects[i] = 0;
		} else {
			effects[i] = trap_FX_RegisterEffect( fileNames[i] );
		}
	}

	return missing;
}

/*
==============
CG_ParseImpactEffects

Returns NULL on success, or a va()'d error message for CG_Error.
==============
*/
static const char *CG_ParseImpactEffects( const char *filename, char *data_p,
										  int numEffectTypes, const char **effectTypes,
										  char ( *fileNames )[SURF_TYPE_NUM][MAX_EFFECT_FILENAME] ) {
	char    *token;
	int     i;
	int     surfaceType;

	while ( 1 ) {
		token = Com_Parse( &data_p );
		if ( !data_p ) {
			return NULL;
		}

		if ( !token[0] || token[0] == '#' ) {
			Com_SkipRestOfLine( 0, &data_p );
			continue;
		}

		for ( i = 0 ; i < numEffectTypes ; i++ ) {
			if ( !stricmp( effectTypes[i], token ) ) {
				break;
			}
		}
		if ( i == numEffectTypes ) {
			return va( "unknown effect type '%s' in first column of file '%s'", token, filename );
		}

		token = Com_ParseOnLine( &data_p );
		if ( !token[0] ) {
			return va( "missing surface type in second column of file '%s'", filename );
		}

		surfaceType = trap_SurfaceTypeFromName( token );
		if ( surfaceType < 0 ) {
			return va( "unknown surface type '%s' in second column of file '%s'", token, filename );
		}

		token = Com_ParseOnLine( &data_p );

		fileNames[i][surfaceType][MAX_EFFECT_FILENAME - 1] = 0;
		strcpy( fileNames[i][surfaceType], token );
		if ( fileNames[i][surfaceType][MAX_EFFECT_FILENAME - 1] ) {
			return va( "effect filename '%s' in third column of file '%s' is longer than %i characters",
					   token, filename, MAX_EFFECT_FILENAME - 1 );
		}
		if ( !token[0] ) {
			fileNames[i][surfaceType][1] = 1;
		}

		Com_SkipRestOfLine( 0, &data_p );
	}
}

/*
==============
compare_impact_files
==============
*/
static int compare_impact_files( const void *a, const void *b ) {
	const char *s1;
	const char *s2;

	s1 = *(const char **)a;
	s2 = *(const char **)b;
	return stricmp( s1, s2 );
}

/*
==============
CG_RegisterImpactEffects
==============
*/
void CG_RegisterImpactEffects( void ) {
	const char  *effectTypes[NUM_IMPACT_EFFECT_TYPES] = {
		"bullet_small_normal",
		"bullet_small_reflect",
		"bullet_large_normal",
		"bullet_large_reflect",
		"grenade_bounce",
		"grenade_explode",
		"rocket_explode",
		"molotov_explode_normal",
		"molotov_explode_reflect"
	};
	int         *effects[NUM_IMPACT_EFFECT_TYPES] = {
		cgs.media.bulletSmallNormalEffects, 
		cgs.media.bulletSmallReflectEffects,
		cgs.media.bulletLargeNormalEffects,
		cgs.media.bulletLargeReflectEffects,
		cgs.media.grenadeBounceEffects,
		cgs.media.grenadeExplodeEffects,
		cgs.media.rocketExplodeEffects,
		cgs.media.molotovExplodeNormalEffects,
		cgs.media.molotovExplodeReflectEffects
	};
	char        fileNames[NUM_IMPACT_EFFECT_TYPES][SURF_TYPE_NUM][MAX_EFFECT_FILENAME];
	char        *files[MAX_EFFECT_FILES];
	char        fileText[MAX_EFFECT_FILE_TEXT];
	const char  *filename;
	const char  *error;
	char        *p;
	char        *buffer;
	fileHandle_t f;
	int         numFiles;
	int         len;
	int         missing;
	int         i;

	numFiles = trap_FS_GetFileList( "fx", "csv", fileText, sizeof( fileText ) );
	if ( !numFiles ) {
		CG_Error( "No CSV files in the fx directory to identify impact effects\n" );
	} else if ( (unsigned)numFiles > MAX_EFFECT_FILES ) {
		numFiles = MAX_EFFECT_FILES;
	}

	p = fileText;
	for ( i = 0 ; i < numFiles ; i++ ) {
		files[i] = p;
		p += strlen( p ) + 1;
	}

	qsort( files, numFiles, sizeof( files[0] ), compare_impact_files );

	memset( fileNames, 0, sizeof( fileNames ) );

	for ( i = 0 ; i < numFiles ; i++ ) {
		filename = va( "fx/%s", files[i] );

		len = trap_FS_FOpenFile( filename, &f, FS_READ );
		if ( len < 0 ) {
			continue;
		}

		buffer = (char *)trap_Z_MallocInternal( len + 1 );
		trap_FS_Read( buffer, len, f );
		trap_FS_FCloseFile( f );
		buffer[len] = 0;

		Com_BeginParseSession( filename );
		Com_SetCSV( qtrue );
		error = CG_ParseImpactEffects( filename, buffer, NUM_IMPACT_EFFECT_TYPES,
									   effectTypes, fileNames );
		Com_EndParseSession();
		trap_Z_FreeInternal( buffer );

		if ( error ) {
			CG_Error( error );
		}
	}

	missing = 0;
	for ( i = 0 ; i < sizeof( effectTypes ) / sizeof( effectTypes[0] ) ; i++ ) {
		missing += CG_RegisterImpactEffectsOfType( fileNames[i], effectTypes[i], effects[i] );
	}

	if ( missing ) {
		CG_Error( "%i missing entries in effect CSV files (see console for details)", missing );
	}
}
