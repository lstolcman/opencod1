/*
 * stringed/stringed_hooks.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/stringed/stringed_hooks.cpp
 *
 * Retail range 0x004A99B0-0x004AA35F, 15 functions.
 *
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"

#include <string.h>
/* ---- module-owned globals ---- */
cvar_t *cl_languagesavailable;


extern cvar_t   *cl_language;                    /* 0x0161734C */
extern cvar_t   *cl_languagesavailable;          /* 0x015CE830 */
extern cvar_t   *cl_languagetranslate;           /* 0x01432954 */
extern cvar_t   *cl_languagewarnings;            /* 0x01617350 */
extern cvar_t   *cl_languagewarningsaserrors;    /* 0x01432844 */

extern cvar_t   *fs_ignoreLocalized;

/* 0x00CA26C0; the 1.3 Mac stringed_hooks.c defines g_currentAsian here too */
int             g_currentAsian;

#define SE_GETSTRING    SE_GetString
extern char *SE_GetString( const char *key, int wantReference );
typedef struct { void *package; } SePackageRef;   /* Clear keeps its receiver on the stack */
static __inline SePackageRef SePackageRefArgument(void *package) { SePackageRef ref; ref.package=package; return ref; }
extern int __stdcall   Clear__16CStringEdPackageFi( SePackageRef packageRef, int bKeepFileInfo );
extern int   SE_GetNumLanguages__Fv( void );
extern char *SE_LoadAllLanguageFiles_m( const char *languageName, int force );

extern int  TheStringPackage;                   /* 0x01407450 */

int SEH_StringEd_SetLanguageStrings( int language );

#define NUM_LANGUAGES       14

#define LANGUAGE_FIRST_ASIAN    8
#define LANGUAGE_LAST_ASIAN     12

/* retail 0x00571B30: {name, present} pairs */
typedef struct {
	const char *name;
	int         present;
} seh_language_t;

static seh_language_t seh_languages[NUM_LANGUAGES] = {
	{ "english" },   { "french" },     { "german" },   { "italian" },
	{ "spanish" },   { "british" },    { "russian" },  { "polish" },
	{ "korean" },    { "taiwanese" },  { "japanese" }, { "chinese" },
	{ "thai" },      { "leet" }
};

#define SEH_MARK_LOC_ON     0x14
#define SEH_MARK_LOC_OFF    0x15
#define SEH_MARK_NO_INSERT  0x16

#define LOCMSG_NOERR        1

#define ERR_LOCALIZATION    7

#define MAX_LOCALIZED_LEN   1024

static int  iCurrString;
static char szStrings[2][MAX_LOCALIZED_LEN];

static char szSafeTranslate[MAX_LOCALIZED_LEN];

/* ---- SEH_UpdateCurrentLanguage  0x004A99B0 ---- */
int SEH_UpdateCurrentLanguage( void ) {
	int language;

	/* retail discards the result and reads the cl_language global */
	Cvar_Get( "cl_language", "0", CVAR_ARCHIVE | CVAR_LATCH );

	language = cl_language->integer;
	g_currentAsian = ( language >= LANGUAGE_FIRST_ASIAN && language <= LANGUAGE_LAST_ASIAN );

	return language;
}

/* ---- SEH_GetCurrentLanguage  0x004A99F0 ---- */
int SEH_GetCurrentLanguage( void ) {
	return cl_language->integer;
}

/* ---- SEH_GetLanguageName  0x004A9A00 ---- */
const char *SEH_GetLanguageName( int language ) {
	if ( language >= 0 && language < NUM_LANGUAGES ) {
		return seh_languages[language].name;
	}
	return seh_languages[0].name;
}

/* ---- SEH_GetLanguageIndexForName  0x004A9A20 ---- */
qboolean SEH_GetLanguageIndexForName( const char *name, int *languageOut ) {
	int         i;
	const char  *langName;

	for ( i = 0 ; i < NUM_LANGUAGES ; i++ ) {
		langName = seh_languages[i].name;
		if ( name && langName && !Q_stricmpn( name, langName, 99999 ) ) {
			*languageOut = i;
			return qtrue;
		}
	}

	*languageOut = 0;
	return qfalse;
}

/* ---- SEH_InitLanguage  0x004A9A60 ---- */
int SEH_InitLanguage( void ) {
	cl_language                 = Cvar_Get( "cl_language", "0", CVAR_ARCHIVE | CVAR_LATCH );
	cl_languagesavailable       = Cvar_Get( "cl_languagesavailable", "0", 0 );
	cl_languagetranslate        = Cvar_Get( "cl_languagetranslate", "1", CVAR_LATCH );
	cl_languagewarnings         = Cvar_Get( "cl_languagewarnings", "0", 0 );
	cl_languagewarningsaserrors = Cvar_Get( "cl_languagewarningsaserrors", "0", 0 );

	return SEH_UpdateCurrentLanguage();
}

/* universal/com_files.c 0x00428D20, inlined into SEH_UpdateLanguageInfo by LTCG */
extern qboolean FS_LanguageHasAssets( int language );

/* ---- SEH_UpdateLanguageInfo  0x004A9B10 ---- */
/* Cvar_Set (inlined) rather than Cvar_Set2(.., qtrue): retail's ebx/edi choice for 0/1 */
void SEH_UpdateLanguageInfo( void ) {
	int i;
	int numLanguages;

	SEH_UpdateCurrentLanguage();

	numLanguages = 0;
	for ( i = 0 ; i < NUM_LANGUAGES ; i++ ) {
		if ( FS_LanguageHasAssets( i ) ) {
			seh_languages[i].present = 1;
			numLanguages++;
		} else {
			seh_languages[i].present = 0;
		}
	}

	if ( numLanguages < 1 ) {
		Com_Printf( "^1ERROR: No languages available because no localized assets were found\n" );
	}

	Cvar_Set( "cl_languagesavailable", va( "%i", numLanguages ) );

	if ( !SEH_StringEd_SetLanguageStrings( cl_language->integer ) ) {
		for ( i = 0 ; i < NUM_LANGUAGES ; i++ ) {
			if ( seh_languages[i].present ) {
				Cvar_Set( "cl_language", va( "%i", i ) );
				SEH_UpdateCurrentLanguage();
				if ( SEH_StringEd_SetLanguageStrings( i ) ) {
					break;
				}
			}
		}

		if ( i == NUM_LANGUAGES ) {
			Cvar_Set( "cl_language", "0" );
			SEH_UpdateCurrentLanguage();
		}
	}
}

/* retail 0x004A9C90 indexes {name,present}[14] at 0x571B30 with no range check.
 * The words it reads out of range are static .data: indices -13..-1 (0x571ACC..0x571B2C)
 * are zero. After the table come zlib's z_errmsg, static_l/d/bl_desc, inflate_mask and
 * fixed tables: indices 14..63 are nonzero except 22, 26, 36 and 49. Further out (below -13
 * the infoParm table, above 63 the rest of zlib's fixed tables and beyond) is not emulated
 * and is treated as 0. */
#ifndef COD1_VC7
static int SEH_RetailLanguagePresentWord( int k ) {
	if ( k >= 0 && k < NUM_LANGUAGES ) {
		return seh_languages[k].present;
	}
	return ( k >= NUM_LANGUAGES && k <= 63 && k != 22 && k != 26 && k != 36 && k != 49 );
}
#endif

/* ---- SEH_VerifyLanguageSelection  0x004A9C90 ---- */
/* No range clamp, as in retail: an out-of-range language reads the neighbouring
 * retail words (see SEH_RetailLanguagePresentWord), so 14..21 (and the other nonzero
 * words up to 63) come back unchanged and the rest scan from the signed remainder. */
int SEH_VerifyLanguageSelection( int language ) {
	int i;

#ifdef COD1_VC7
	/* retail's own unchecked reads; the out-of-range words are whatever lies next to the table */
	if ( seh_languages[language].present ) {
		return language;
	}

	for ( i = 0 ; i < NUM_LANGUAGES ; i++ ) {
		if ( seh_languages[( i + language ) % NUM_LANGUAGES].present ) {
			return ( i + language ) % NUM_LANGUAGES;
		}
	}

	return 0;
#else
	if ( SEH_RetailLanguagePresentWord( language ) ) {
		return language;
	}

	for ( i = 0 ; i < NUM_LANGUAGES ; i++ ) {
		if ( SEH_RetailLanguagePresentWord( ( i + language ) % NUM_LANGUAGES ) ) {	/* signed %, same as cdq/idiv */
			return ( i + language ) % NUM_LANGUAGES;
		}
	}

	return 0;
#endif
}

/* ---- SEH_StringEd_SetLanguageStrings  0x004A9D00 ---- */
int SEH_StringEd_SetLanguageStrings( int language ) {
	char        *err;

	Clear__16CStringEdPackageFi( SePackageRefArgument( (void *)&TheStringPackage ), 1 );

	/* retail reads the cvars with no NULL checks */
	if ( !SE_GetNumLanguages__Fv() ) {
		if ( !fs_ignoreLocalized->integer && cl_languagewarnings->integer ) {
			if ( cl_languagewarningsaserrors->integer ) {
				Com_Error( ERR_LOCALIZATION,
						   "No language string information available for %s", SEH_GetLanguageName( language ) );
			} else {
				Com_Printf( "^3WARNING: No language string information available for %s\n", SEH_GetLanguageName( language ) );
			}
		}
		return qfalse;
	}

	err = SE_LoadAllLanguageFiles_m( SEH_GetLanguageName( language ), 1 );
	if ( err ) {
		if ( !fs_ignoreLocalized->integer && cl_languagewarnings->integer ) {
			if ( cl_languagewarningsaserrors->integer ) {
				Com_Error( ERR_LOCALIZATION,
						   "Could not load localization strings for %s: %s", SEH_GetLanguageName( language ), err );
			} else {
				Com_Printf( "^3WARNING: Could not load localization strings for %s: %s\n", SEH_GetLanguageName( language ), err );
			}
		}
		return qfalse;
	}
	return qtrue;
}

/* ---- SEH_StringEd_GetString  0x004A9E20 ---- */
char *SEH_StringEd_GetString( const char *key ) {
	if ( cl_languagetranslate && cl_languagetranslate->integer && key[0] && key[1] ) {
		return SE_GETSTRING( key, 0 );
	}
	return (char *)key;
}

/* ---- SEH_SafeTranslateString  0x004A9E50 ---- */
char *SEH_SafeTranslateString( const char *key ) {
	char *translated;

	translated = SEH_StringEd_GetString( key );

	if ( translated ) {
		return translated;
	}

	/* retail 0x004A9E87: no NULL checks on the warning cvars, Com_Error/Com_Printf
	 * is an if/else, and the result is built with unbounded strcpy/strcat. */
	if ( cl_languagewarnings->integer ) {
		if ( cl_languagewarningsaserrors->integer ) {
			Com_Error( ERR_LOCALIZATION, "Could not translate exe string \"%s\"", key );
		} else {
			Com_Printf( "^3WARNING: Could not translate exe string \"%s\"\n", key );
		}
		strcpy( szSafeTranslate, "^1UNLOCALIZED(^7" );
		strcat( szSafeTranslate, key );
		strcat( szSafeTranslate, "^1)^7" );
	} else {
		strcpy( szSafeTranslate, key );
	}

	return szSafeTranslate;
}

/* ---- SEH_GetLocalizedTokenReference  0x004A9F80 ---- */
static qboolean SEH_GetLocalizedTokenReference( char *out, const char *token,
												const char *type, int flag ) {
	char *translated;

	if ( cl_languagetranslate && cl_languagetranslate->integer && token[0] && token[1] ) {
		translated = SE_GETSTRING( token, 0 );
	} else {
		translated = (char *)token;
	}

	if ( !translated ) {
		if ( cl_languagewarnings && cl_languagewarnings->integer ) {
			if ( cl_languagewarningsaserrors && cl_languagewarningsaserrors->integer
				 && flag != LOCMSG_NOERR ) {
				Com_Error( ERR_LOCALIZATION,
						   "Could not translate part of %s: \"%s\"", type, token );
			} else {
				Com_Printf( "^3WARNING: Could not translate part of %s: \"%s\"\n", type, token );
			}
			translated = va( "^1UNLOCALIZED(^7%s^1)^7", token );
		} else {
			translated = va( "%s", token );
		}

		if ( flag == LOCMSG_NOERR ) {
			return qfalse;
		}
	}

#ifdef COD1_VC7
	strcpy( out, translated );	/* retail copies even when translated == out */
#else
	if ( translated != out ) {
		strcpy( out, translated );
	}
#endif
	return qtrue;
}

/* ---- SEH_LocalizeTextMessage  0x004AA040 ---- */
char *SEH_LocalizeTextMessage( const char *text, const char *type, int flag ) {
	char        szTokenBuf[MAX_LOCALIZED_LEN];
	char        szInsertBuf[MAX_LOCALIZED_LEN];
	char        *out;
	const char  *in;
	const char  *tokStart;
	int         outLen;
	int         tokLen;
	int         insertLevel;
	int         tokenHasInsert;
	int         locOn;
	int         insertEnabled;
	int         locSkipped;
	int         at;
	int         i;

	iCurrString = ( iCurrString + 1 ) % 2;
	memset( szStrings[iCurrString], 0, sizeof( szStrings[0] ) );
	out = szStrings[iCurrString];

#ifndef COD1_VC7
	szInsertBuf[0] = '\0';	/* retail leaves it uninitialised (read only if no "%s" is found in out) */
#endif

	outLen         = 0;
	locOn          = 1;
	insertEnabled  = 1;
	insertLevel    = 0;
	tokenHasInsert = 0;
	locSkipped     = 0;
	tokStart       = text;
	in             = text;

	while ( *tokStart ) {
		if ( *in && *in != SEH_MARK_LOC_ON && *in != SEH_MARK_LOC_OFF
			 && *in != SEH_MARK_NO_INSERT ) {
			in++;
		} else {
			if ( in > tokStart ) {
				tokLen = in - tokStart;
				/* inlined in retail (0x004AA0F4 strncpy + terminator, no bound check) */
				Q_strncpyz( szTokenBuf, tokStart, tokLen + 1 );

				if ( locOn ) {
					if ( !SEH_GetLocalizedTokenReference( szTokenBuf, szTokenBuf, type, flag ) ) {
						return NULL;
					}
					tokLen = strlen( szTokenBuf );
				}

				if ( tokLen + outLen >= sizeof( szTokenBuf ) ) {
					if ( cl_languagewarnings && cl_languagewarnings->integer
						 && cl_languagewarningsaserrors && cl_languagewarningsaserrors->integer
						 && flag != LOCMSG_NOERR ) {
						Com_Error( ERR_DROP, "%s too long when translated: \"%s\"", type, text );
					}
					Com_Printf( "%s too long when translated: \"%s\"\n", type, text );
				}

				for ( i = 0 ; i < tokLen - 1 ; i++ ) {
					if ( szTokenBuf[i] == '%' && szTokenBuf[i + 1] == 's' ) {
						if ( insertEnabled && !tokenHasInsert ) {
							insertLevel++;
							tokenHasInsert = 1;
						} else {
							szTokenBuf[i] = SEH_MARK_NO_INSERT;
							locSkipped = 1;
						}
					}
				}

				if ( insertLevel - tokenHasInsert ) {
					for ( at = 0 ; at < outLen - 1 ; at++ ) {
						if ( out[at] == '%' && out[at + 1] == 's' ) {
							strcpy( szInsertBuf, &out[at + 2] );
							out[at] = '\0';
							break;
						}
					}
					strcpy( &out[at], szTokenBuf );
					strcpy( &out[at + tokLen], szInsertBuf );
					outLen -= 2;
					insertLevel--;
				} else {
					strcpy( &out[outLen], szTokenBuf );
				}

				tokenHasInsert = 0;
				outLen += tokLen;
			}

			insertEnabled = 1;
			if ( *in == SEH_MARK_LOC_ON ) {
				locOn = 1;
				in++;
			} else if ( *in == SEH_MARK_LOC_OFF ) {
				locOn = 0;
				in++;
			}
			if ( *in == SEH_MARK_NO_INSERT ) {
				insertEnabled = 0;
				in++;
			}

			tokStart = in;
		}
	}

	if ( locSkipped ) {
		for ( i = 0 ; i < outLen ; i++ ) {
			if ( out[i] == SEH_MARK_NO_INSERT ) {
				out[i] = '%';
			}
		}
	}

	return out;
}

#ifdef __cplusplus
}
#endif
