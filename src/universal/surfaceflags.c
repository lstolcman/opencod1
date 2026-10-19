/*
 * universal/surfaceflags.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/universal/surfaceflags.c
 *
 * Retail range 0x004390F0-0x0043917.
 *
 * @fidelity-default: verified
 */

#include "../qcommon/qcommon.h"

#include <string.h>

/* Rows 0..22 of the retail infoParm table at 0x005717A0 (16-byte rows:
   name, clearSolid, surfaceFlags, contents); renderer/tr_shader.c holds the
   full 53-row copy. Only name and surfaceFlags are read here. */
typedef struct {
	const char  *name;
	int nonOpaque;
	int surfaceFlags;
	int contents;
} surfaceType_t;

static surfaceType_t surfaceTypes[] = {
	{ "bark",        0, 0x00100000, 0x00 },
	{ "brick",       0, 0x00200000, 0x00 },
	{ "carpet",      0, 0x00300000, 0x00 },
	{ "cloth",       0, 0x00400000, 0x00 },
	{ "concrete",    0, 0x00500000, 0x00 },
	{ "dirt",        0, 0x00600000, 0x00 },
	{ "flesh",       0, 0x00700000, 0x00 },
	{ "foliage",     1, 0x00800000, 0x02 },
	{ "glass",       1, 0x00900000, 0x10 },
	{ "grass",       0, 0x00A00000, 0x00 },
	{ "gravel",      0, 0x00B00000, 0x00 },
	{ "ice",         0, 0x00C00000, 0x00 },
	{ "metal",       0, 0x00D00000, 0x00 },
	{ "mud",         0, 0x00E00000, 0x00 },
	{ "paper",       0, 0x00F00000, 0x00 },
	{ "plaster",     0, 0x01000000, 0x00 },
	{ "rock",        0, 0x01100000, 0x00 },
	{ "sand",        0, 0x01200000, 0x00 },
	{ "snow",        0, 0x01300000, 0x00 },
	{ "water",       1, 0x01400000, 0x20 },
	{ "wood",        0, 0x01500000, 0x00 },
	{ "asphalt",     0, 0x01600000, 0x00 },
	{ "opaqueglass", 0, 0x00900000, 0x00 },
};

#define NUM_SURFACE_TYPES ( sizeof( surfaceTypes ) / sizeof( surfaceTypes[0] ) )

#define NUM_MATCHABLE_SURFACE_TYPES ( NUM_SURFACE_TYPES - 1 )

/* ---- Com_SurfaceTypeFromName  0x004390F0 ---- */
/* Retail calls the CRT _stricmp (0x0053549F, locale-aware, no NULL guard)
   for both compares, not Q_stricmp (0x0044AB30). */
int Com_SurfaceTypeFromName( const char *name ) {
	int i;

	if ( !_stricmp( name, "default" ) ) {
		return 0;
	}

	for ( i = 0 ; i < (int)NUM_MATCHABLE_SURFACE_TYPES ; i++ ) {
		if ( !_stricmp( name, surfaceTypes[i].name ) ) {
			return ( surfaceTypes[i].surfaceFlags >> 20 ) & 0x1F;
		}
	}

	return -1;
}

/* ---- Com_SurfaceTypeToName  0x00439150 ---- */
/* Retail rejects type >= 23 (cmp eax,17h / jge), so only 1..22 map to names;
   entry 22 ("opaqueglass") is never returned and type 23 yields "default". */
const char *Com_SurfaceTypeToName( int type ) {
	if ( type <= 0 || type >= (int)NUM_SURFACE_TYPES ) {
		return "default";
	}
	return surfaceTypes[type - 1].name;
}

/* ---- Com_AddToString  0x00439170 ---- */
int Com_AddToString( const char *src, char *msg, int len, int maxlen, int mayAddQuotes ) {
	int addQuotes;
	int i;

	addQuotes = 0;
	if ( mayAddQuotes ) {
		if ( src[0] ) {
			for ( i = 0 ; i < maxlen - len && src[i] ; i++ ) {
				if ( src[i] <= ' ' ) {
					addQuotes = 1;
					break;
				}
			}
		} else {
			addQuotes = 1;
		}
		if ( addQuotes && len < maxlen ) {
			msg[len++] = '"';
		}
	}

	for ( i = 0 ; len < maxlen && src[i] ; i++ ) {
		msg[len++] = src[i];
	}

	if ( addQuotes && len < maxlen ) {
		msg[len++] = '"';
	}

	return len;
}
