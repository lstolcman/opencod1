/*
 * universal/com_files.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/universal/com_files.c
 *
 * Retail range 0x00428C80-0x0042D5C0.
 *
 * @fidelity-default: likely
 */

#include "../qcommon/qcommon.h"
#include "files_local.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <io.h>
#include <direct.h>
#include <windows.h>

#define BASEGAME            "main"
#define DEMOGAME            "demomain"
#define MAX_ZIPPED_FILES    16384

searchpath_t        *fs_searchpaths;
fileHandleData_t fsh[MAX_FILE_HANDLES];

fs_dirFileList_t    *fs_dirFileLists;        /* 0x01407424 */
searchpath_t        *fs_savedSearchpaths;    /* 0x01407428 */
fs_dirFileList_t    *fs_savedDirFileLists;   /* 0x0140742C */

cvar_t  *fs_debug;
#define NUM_LANGUAGES  14

/* stringed/stringed_hooks.cpp (C linkage): the language table (retail 0x00571B30)
   is that unit's; LTCG inlines both here */
extern qboolean SEH_GetLanguageIndexForName( const char *name, int *languageOut );
extern const char *SEH_GetLanguageName( int language );

cvar_t  *fs_homepath;
cvar_t  *fs_basepath;
cvar_t  *fs_basegame;
cvar_t  *fs_cdpath;
cvar_t  *fs_gamedirvar;
cvar_t  *fs_restrict;
cvar_t  *fs_ignoreLocalized;
cvar_t  *fs_copyfiles;                  /* 0x0162562C */

static int fs_loadStack;
static int fs_readCount;
static int fs_loadCount;
static int fs_packFiles;

int fs_checksumFeed;                    /* 0x0162D768 */
int fs_fakeChkSum;                      /* 0x01631788 */
int fs_loadingMode;                     /* 0x01631780 */

int fs_numServerPaks;                   /* 0x0162D764 */
int fs_serverPaks[MAX_SEARCH_PATHS];    /* 0x016318C0 */
char        *fs_serverPakNames[MAX_SEARCH_PATHS];   /* 0x01625640 */

int fs_numServerReferencedPaks;
int fs_serverReferencedPaks[MAX_SEARCH_PATHS];
char        *fs_serverReferencedPakNames[MAX_SEARCH_PATHS];

char fs_gamedir[MAX_OSPATH];

static char lastValidBase[MAX_OSPATH];
static char lastValidGame[MAX_OSPATH];

const char *SEH_GetLanguageName( int language );                  /* 0x004A9A00 */
int  SEH_InitLanguage( void );                                  /* 0x004A9A60 */
int __stdcall  Clear__16CStringEdPackageFi( void *package, int keepFileInfo ); /* 0x004AAAA0 */
extern int TheStringPackage;                                    /* 0x01407450 */

int  FS_SetRestrictions( void );                                /* 0x0043BAD0 */

int  Sys_DirectoryHasContents_m( const char *path );

unzFile Unz_Open( const char *path );
int      Unz_Close( unzFile file );
unzFile Unz_ReOpen( const char *path, unzFile file );
int      Unz_GetGlobalNumFiles( unzFile file, int *numFiles );
int      Unz_GoToFirstFile( unzFile file );
int      Unz_GoToNextFile( unzFile file );
int      Unz_GetCurrentFileInfo( unzFile file, void *pfile_info,
								 char *szFileName, unsigned fileNameBufferSize,
								 void *extraField, unsigned extraFieldBufferSize,
								 char *szComment, unsigned commentBufferSize );
unsigned long Unz_GetCurrentFileInfoPosition( unzFile file );
int      Unz_SetCurrentFileInfoPosition( unzFile file, unsigned long pos );
int      Unz_GetCurrentFileUncompressedSize( unzFile file );
int      Unz_OpenCurrentFile( unzFile file );
int      Unz_ReadCurrentFile( unzFile file, void *buf, unsigned len );
/* retail FS_Read calls zlib's (file, len, buf) unzReadCurrentFile directly, not the Unz_ adapter */
int      unzReadCurrentFile( unzFile file, unsigned len, void *buf );
int      Unz_CloseCurrentFile( unzFile file );

int      unztell( unzFile file );
/* retail FS_Seek calls minizip directly too */
int      unzSetCurrentFileInfoPosition( unzFile file, unsigned long pos );
int      unzOpenCurrentFile( unzFile file );
/* ... and so do FS_FCloseFile, FS_FOpenFileRead_Internal, FS_LoadZipFile and
   FS_Shutdown: retail-order (permuted) minizip entry points, as zlib/unzip.c emits them */
int      unzCloseCurrentFile( unzFile file );
int      unzClose( unzFile file );
unzFile  unzReOpen( const char *path, unzFile file );
int      unzGetCurrentFileInfo( char *szComment, unsigned extraFieldBufferSize,
								unsigned commentBufferSize, unzFile file,
								void *pfile_info, char *szFileName,
								unsigned fileNameBufferSize, void *extraField );

/* ---- FS_Initialized  0x00428C80 ---- VERIFIED */
qboolean FS_Initialized( void ) {
	return ( fs_searchpaths != NULL );
}

/* ---- FS_CheckFileSystemStarted  0x00428C90 ---- VERIFIED */
void FS_CheckFileSystemStarted( void ) {
}

/* ---- FS_PakIsPure  0x00428CA0 ---- VERIFIED */
qboolean FS_PakIsPure( pack_t *pack ) {
	int i;

	if ( fs_numServerPaks ) {
		for ( i = 0 ; i < fs_numServerPaks ; i++ ) {
			if ( pack->checksum == fs_serverPaks[i] ) {
				return qtrue;
			}
		}
		return qfalse;
	}
	return qtrue;
}

/* ---- FS_LoadStack  0x00428CD0 ---- VERIFIED */
int FS_LoadStack( void ) {
	return fs_loadStack;
}

/* ---- FS_UseSearchPath  0x00428CE0 ---- VERIFIED */
qboolean FS_UseSearchPath( searchpath_t *sp ) {

	if ( sp->localized && fs_ignoreLocalized->integer ) {
		return qfalse;
	}
	if ( sp->localized && sp->language != cl_language->integer ) {
		return qfalse;
	}
	return qtrue;
}

/* ---- FS_LanguageHasAssets  0x00428D20 ---- VERIFIED */
qboolean FS_LanguageHasAssets( int language ) {
	searchpath_t *sp;

	for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
		if ( sp->localized && sp->language == language ) {
			return qtrue;
		}
	}
	return qfalse;
}

/* ---- FS_HashFileName  0x00428D50 ---- VERIFIED */
long FS_HashFileName( const char *fname, int hashSize ) {
	int i;
	long hash;
	int letter;     /* retail 0x00428D70: tolower's int result is compared and multiplied untruncated */

	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		if ( letter == '.' ) {
			break;
		}
		if ( letter == '\\' ) {
			letter = '/';
		}
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash = hash ^ ( ( hash ^ ( hash >> 10 ) ) >> 10 );
	hash &= ( hashSize - 1 );
	return hash;
}

/* ---- FS_HandleForFile  0x00428DB0 ---- VERIFIED */
fileHandle_t FS_HandleForFile( int streamed ) {
	int i;
	int start, count;

	if ( streamed ) {
		start = 51;
		count = 13;
	} else {
		start = 1;
		count = 50;
	}

	for ( i = 0 ; i < count ; i++ ) {
		if ( fsh[start + i].handleFiles.file.o == NULL ) {
			return start + i;
		}
	}

	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		Com_Printf( "FILE %2i: '%s'\n", i, fsh[i].name );
	}
	Com_Error( ERR_DROP, "\x15" "FS_HandleForFile: none free" );
	return -1;
}

/* ---- FS_FileForHandle  0x00428E40 ---- VERIFIED */
FILE *FS_FileForHandle( fileHandle_t f ) {
	return fsh[f].handleFiles.file.o;
}

/* ---- FS_ForceFlush  0x00428E50 ---- VERIFIED */
void FS_ForceFlush( fileHandle_t f ) {
	FILE *file;

	file = FS_FileForHandle( f );
	setvbuf( file, NULL, _IONBF, 0 );
}

/* ---- FS_filelength  0x00428E70 ---- VERIFIED */
int FS_filelength( fileHandle_t f ) {
	FILE *h;
	int pos;
	int end;

	if ( fsh[f].zipFile == qtrue ) {
		/* unz_s.cur_file_info.uncompressed_size, read straight out of the handle */
		return ( (int *) fsh[f].handleFiles.file.z )[17];
	}

	h = FS_FileForHandle( f );
	pos = ftell( h );
	fseek( h, 0, SEEK_END );
	end = ftell( h );
	fseek( h, pos, SEEK_SET );

	return end;
}

/* ---- FS_ReplaceSeparators  0x00428EC0 ---- VERIFIED */
void FS_ReplaceSeparators( char *path ) {
	char *s;

	for ( s = path ; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = '\\';
		}
	}
}

/* ---- FS_BuildOSPath3  0x00428EE0 ---- VERIFIED */
void FS_BuildOSPath3( const char *base, const char *game, const char *qpath,
					  char *fs_path, int allowOverflow ) {
	int baseLen, gameLen, qLen;

	if ( !game || !*game ) {
		game = fs_gamedir;
	}

	baseLen = strlen( base );
	gameLen = strlen( game );
	qLen = strlen( qpath );

	if ( baseLen + gameLen + qLen + 2 >= MAX_OSPATH ) {
		if ( allowOverflow ) {
			*fs_path = 0;
			return;
		}
		Com_Error( ERR_FATAL, "\x15" "FS_BuildOSPath: os path length exceeded\n" );
	}
	memcpy( fs_path, base, baseLen );
	fs_path[baseLen] = '/';
	memcpy( fs_path + baseLen + 1, game, gameLen );
	fs_path[baseLen + gameLen + 1] = '/';
	memcpy( fs_path + baseLen + gameLen + 2, qpath, qLen + 1 );
	FS_ReplaceSeparators( fs_path + baseLen );
}

/* ---- FS_BuildOSPath  0x00428FE0 ---- VERIFIED */
void FS_BuildOSPath( const char *base, const char *game, const char *qpath, char *fs_path ) {
	FS_BuildOSPath3( base, game, qpath, fs_path, 0 );
}

/* ---- FS_CreatePath  0x00429000 ---- VERIFIED */
qboolean FS_CreatePath( char *OSPath ) {
	char *ofs;

	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
		Com_Printf( "WARNING: refusing to create relative path \"%s\"\n", OSPath );
		return qtrue;
	}

	for ( ofs = OSPath + 1 ; *ofs ; ofs++ ) {
		if ( *ofs == '\\' ) {
			*ofs = 0;
			_mkdir( OSPath );
			*ofs = '\\';
		}
	}
	return qfalse;
}

/* ---- FS_CopyFile  0x00429070 ---- VERIFIED */
void FS_CopyFile( char *fromOSPath, char *toOSPath ) {
	FILE *f;
	int len;
	byte *buf;

	if ( strstr( fromOSPath, "journal.dat" ) || strstr( fromOSPath, "journaldata.dat" ) ) {
		return;
	}

	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fseek( f, 0, SEEK_SET );

	buf = malloc( len );
	if ( fread( buf, 1, len, f ) != (size_t)len ) {
		Com_Error( ERR_FATAL, "\x15" "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	if ( FS_CreatePath( toOSPath ) ) {
		free( buf );
		return;
	}

	f = fopen( toOSPath, "wb" );
	if ( !f ) {
		free( buf );
		return;
	}
	if ( fwrite( buf, 1, len, f ) != (size_t)len ) {
		Com_Error( ERR_FATAL, "\x15" "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	free( buf );
}

/* ---- FS_CopyFiles  0x00429170 ---- VERIFIED */
void FS_CopyFiles( const char *fromDir, const char *toDir, const char *ext ) {
	char fromOSPath[MAX_OSPATH];
	char toOSPath[MAX_OSPATH];
	FILE *f;
	int len;
	byte *buf;

	/* retail 0x00429182-0x004291C7: base fs_homepath->string (EAX), game fs_gamedir. */
	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, fromDir, fromOSPath, 0 );
	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, toDir, toOSPath, 0 );

	if ( strstr( fromOSPath, "journal.dat" ) || strstr( fromOSPath, "journaldata.dat" ) ) {
		Com_Printf( "Ignoring journal files\n" );
		return;
	}

	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fseek( f, 0, SEEK_SET );
	buf = malloc( len );
	if ( fread( buf, 1, len, f ) != (size_t)len ) {
		Com_Error( ERR_FATAL, "\x15" "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	if ( FS_CreatePath( toOSPath ) ) {
		free( buf );
		return;
	}

	f = fopen( toOSPath, "wb" );
	if ( !f ) {
		free( buf );
		return;
	}
	if ( fwrite( buf, 1, len, f ) != (size_t)len ) {
		Com_Error( ERR_FATAL, "\x15" "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	free( buf );
}

/* ---- FS_Remove  0x00429300 ---- VERIFIED */
void FS_Remove( const char *osPath ) {
	remove( osPath );
}

/* ---- FS_FileExists  0x00429310 ---- VERIFIED */
qboolean FS_FileExists( const char *file ) {
	FILE *f;
	char testpath[MAX_OSPATH];

	/* retail 0x00429329-0x00429346: base fs_homepath->string (EAX), game fs_gamedir. */
	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, file, testpath, 0 );

	f = fopen( testpath, "rb" );
	if ( f ) {
		fclose( f );
		return qtrue;
	}
	return qfalse;
}

/* ---- FS_Rename  0x004293B0 ---- VERIFIED */
void FS_Rename( const char *from, const char *to ) {
	char fromOSPath[MAX_OSPATH];
	char toOSPath[MAX_OSPATH];

	/* retail 0x004293C2-0x00429404: base fs_homepath->string (EAX), game fs_gamedir. */
	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, from, fromOSPath, 0 );
	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, to, toOSPath, 0 );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_Rename: %s --> %s\n", fromOSPath, toOSPath );
	}

	if ( rename( fromOSPath, toOSPath ) ) {
		remove( toOSPath );
		if ( rename( fromOSPath, toOSPath ) ) {
			FS_CopyFile( fromOSPath, toOSPath );
			remove( fromOSPath );
		}
	}
}

/* ---- FS_FCloseFile  0x004294B0 ---- VERIFIED */
void FS_FCloseFile( fileHandle_t f ) {
	if ( fsh[f].zipFile == qtrue ) {
		unzCloseCurrentFile( fsh[f].handleFiles.file.z );
		if ( fsh[f].handleFiles.unique ) {
			unzClose( fsh[f].handleFiles.file.z );
		}
		Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
		return;
	}

	if ( fsh[f].handleFiles.file.o ) {
		fclose( fsh[f].handleFiles.file.o );
	}
	Com_Memset( &fsh[f], 0, sizeof( fsh[f] ) );
}

/* ---- FS_FOpenFileWrite  0x00429550 ---- VERIFIED */
fileHandle_t FS_FOpenFileWrite( const char *filename ) {
	fileHandle_t f;
	char ospath[MAX_OSPATH];

	f = FS_HandleForFile( 0 );
	fsh[f].zipFile = qfalse;

	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, filename, ospath, 0 );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	if ( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "wb" );
	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
	if ( !fsh[f].handleFiles.file.o ) {
		f = 0;
	}
	return f;
}

/* ---- FS_FOpenTextFileWrite  0x00429640 ---- VERIFIED */
fileHandle_t FS_FOpenTextFileWrite( const char *filename ) {
	fileHandle_t f;
	char ospath[MAX_OSPATH];

	f = FS_HandleForFile( 0 );
	fsh[f].zipFile = qfalse;

	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, filename, ospath, 0 );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileWrite: %s\n", ospath );
	}

	if ( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "wt" );
	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	fsh[f].handleSync = qfalse;
	if ( !fsh[f].handleFiles.file.o ) {
		f = 0;
	}
	return f;
}

/* ---- FS_FOpenFileAppend  0x00429730 ---- VERIFIED */
fileHandle_t FS_FOpenFileAppend( const char *filename ) {
	fileHandle_t f;
	char ospath[MAX_OSPATH];

	f = FS_HandleForFile( 0 );
	fsh[f].zipFile = qfalse;

	Q_strncpyz( fsh[f].name, filename, sizeof( fsh[f].name ) );

	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, filename, ospath, 0 );

	if ( fs_debug->integer ) {
		Com_Printf( "FS_FOpenFileAppend: %s\n", ospath );
	}

	if ( FS_CreatePath( ospath ) ) {
		return 0;
	}

	fsh[f].handleFiles.file.o = fopen( ospath, "ab" );
	fsh[f].handleSync = qfalse;
	if ( !fsh[f].handleFiles.file.o ) {
		f = 0;
	}
	return f;
}

/* ---- FS_FilenameCompare  0x00429820 ---- VERIFIED */
qboolean FS_FilenameCompare( const char *s1, const char *s2 ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' ) {
			c1 -= ( 'a' - 'A' );
		}
		if ( c2 >= 'a' && c2 <= 'z' ) {
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}

		if ( c1 != c2 ) {
			return -1;
		}
	} while ( c1 );

	return 0;
}

/* ---- FS_FileCompare  0x00429880 ---- VERIFIED */
qboolean FS_FileCompare( const char *s1, const char *s2 ) {
	FILE *f1, *f2;
	int len1, len2, pos;
	byte *buf1, *buf2, *p1, *p2;

	f1 = fopen( s2, "rb" );
	if ( !f1 ) {
		Com_Error( ERR_FATAL, "\x15" "FS_FileCompare: %s does not exist\n", s2 );
	}
	f2 = fopen( s1, "rb" );
	if ( !f2 ) {
		fclose( f1 );
		return qfalse;
	}

	pos = ftell( f1 );
	fseek( f1, 0, SEEK_END );
	len1 = ftell( f1 );
	fseek( f1, pos, SEEK_SET );

	pos = ftell( f2 );
	fseek( f2, 0, SEEK_END );
	len2 = ftell( f2 );
	fseek( f2, pos, SEEK_SET );

	if ( len1 != len2 ) {
		fclose( f1 );
		fclose( f2 );
		return qfalse;
	}

	buf1 = Z_MallocInternal( len1 );
	if ( fread( buf1, 1, len1, f1 ) != (size_t)len1 ) {
		Com_Error( ERR_FATAL, "\x15" "Short read in FS_FileCompare()\n" );
	}
	fclose( f1 );

	buf2 = Z_MallocInternal( len2 );
	if ( fread( buf2, 1, len2, f2 ) != (size_t)len2 ) {
		Com_Error( ERR_FATAL, "\x15" "Short read in FS_FileCompare()\n" );
	}
	fclose( f2 );

	p1 = buf1;
	p2 = buf2;
	for ( pos = 0 ; pos < len1 ; pos++, p1++, p2++ ) {
		if ( *p1 != *p2 ) {
			free( buf1 );
			free( buf2 );
			return qfalse;
		}
	}

	free( buf1 );
	free( buf2 );
	return qtrue;
}

/* ---- FS_ShiftedStrStr  0x004299F0 ---- VERIFIED */
char *FS_ShiftedStrStr( const char *string, const char *substring, int shift ) {
	char buf[MAX_OSPATH];
	int i;

	for ( i = 0 ; substring[i] ; i++ ) {
		buf[i] = substring[i] + shift;
	}
	/* retail 0x00429A43 terminates with no bound check; written as a pointer store because
	   buf[i] = 0 makes modern /GS add a range-check fast-fail for i >= 256 */
	*( buf + i ) = '\0';
	return strstr( string, buf );
}

/* ---- FS_FOpenFileRead_Internal  0x00429A70 ---- */
int FS_FOpenFileRead_Internal( const char *qpath, fileHandle_t *file, qboolean uniqueFILE, int filter_flag ) {
	extern int com_fullyInitialized;
	searchpath_t *search;
	pack_t *pak;
	fileInPack_t *pakFile;
	directory_t *dir;
	long hash;
	char netpath[MAX_OSPATH];
	int len;
	unzFile z;

	hash = 0;

	/* retail 0x00429A96-0x00429B7D: with no handle to fill, this is an
	 * existence probe.  It uses the raw qpath (no leading-slash strip, no
	 * '..' / '::' / codkey test, no FS_PakIsPure) and returns 1 when the
	 * file is found in a pak hash chain or opens from a directory, 0 when
	 * no searchpath has it.  Reached through FS_FOpenFileByMode( qpath,
	 * NULL, FS_READ ). */
	if ( !file ) {
		for ( search = fs_searchpaths ; search ; search = search->next ) {
			if ( !FS_UseSearchPath( search ) ) {
				continue;
			}
			if ( search->pack ) {
				hash = FS_HashFileName( qpath, search->pack->hashSize );
			}
			if ( search->pack && search->pack->hashTable[hash] ) {
				for ( pakFile = search->pack->hashTable[hash] ; pakFile ; pakFile = pakFile->next ) {
					if ( !FS_FilenameCompare( pakFile->name, qpath ) ) {
						return 1;
					}
				}
			} else if ( search->dir ) {
				FILE *temp;

				FS_BuildOSPath3( search->dir->path, search->dir->gamedir, qpath, netpath, filter_flag );
				temp = fopen( netpath, "rb" );
				if ( temp ) {
					fclose( temp );
					return 1;
				}
			}
		}
		return 0;
	}

	while ( *qpath == '/' || *qpath == '\\' ) {
		qpath++;
	}

	if ( strstr( qpath, ".." ) || strstr( qpath, "::" )
		 || ( com_fullyInitialized && strstr( qpath, "codkey" ) ) ) {
		*file = 0;
		return -1;
	}

	*file = FS_HandleForFile( filter_flag );
	fsh[*file].handleFiles.unique = uniqueFILE;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		if ( !FS_UseSearchPath( search ) ) {
			continue;
		}

		if ( search->pack ) {
			hash = FS_HashFileName( qpath, search->pack->hashSize );
		}
		if ( search->pack && search->pack->hashTable[hash] ) {
			pak = search->pack;
			if ( FS_PakIsPure( pak ) ) {
				pakFile = pak->hashTable[hash];
				do {
					if ( !FS_FilenameCompare( pakFile->name, qpath ) ) {
						len = strlen( qpath );

						/* retail 0x00429E30-0x00429F41: pack_t.referenced is four
						 * independent byte flags, each tested and set to 1 on its own:
						 * +0x310 general, +0x313 qagame_mp_x86.dll,
						 * +0x312 cgame_mp_x86.dll, +0x311 ui_mp_x86.dll. */
						{
							byte *ref = (byte *) &pak->referenced;

							if ( !ref[0]
								 && Q_stricmp( &qpath[len - 7], ".shader" )
								 && Q_stricmp( &qpath[len - 4], ".txt" )
								 && Q_stricmp( &qpath[len - 4], ".cfg" )
								 && Q_stricmp( &qpath[len - 7], ".config" )
								 && !strstr( qpath, "levelshots" )
								 && Q_stricmp( &qpath[len - 4], ".bot" )
								 && Q_stricmp( &qpath[len - 6], ".arena" )
								 && Q_stricmp( &qpath[len - 5], ".menu" ) ) {
								ref[0] = 1;
							}

							if ( !ref[3] && FS_ShiftedStrStr( qpath, "wgmgskesve~><4jrr", -6 ) ) {
								ref[3] = 1;
							}
							if ( !ref[2] && FS_ShiftedStrStr( qpath, "eicogaoraz:80fnn", -2 ) ) {
								ref[2] = 1;
							}
							if ( !ref[1] && FS_ShiftedStrStr( qpath, "zndrud}=;3iqq", -5 ) ) {
								ref[1] = 1;
							}
						}

						if ( uniqueFILE ) {
							fsh[*file].handleFiles.file.z = unzReOpen( pak->pakFilename, pak->handle );
							if ( !fsh[*file].handleFiles.file.z ) {
								if ( filter_flag ) {
									FS_FCloseFile( *file );
									*file = 0;
									return -1;
								}
								Com_Error( ERR_FATAL, "\x15" "Couldn't reopen %s", pak->pakFilename );
							}
						} else {
							fsh[*file].handleFiles.file.z = pak->handle;
						}

						Q_strncpyz( fsh[*file].name, qpath, sizeof( fsh[*file].name ) );
						fsh[*file].zipFile = qtrue;

						/* retail 0x0042A014-0x0042A069: the zip state is handed over
						 * from the PACK's handle.  It seeks pak->handle, copies the
						 * whole unz_s (128 bytes) over our handle, restores our own
						 * FILE* (unz_s.file, +0), then opens the entry on our handle.
						 * With a non-unique handle z == pak->handle and the copy is a
						 * self-copy.
						 * RETAIL HAZARD: this changes pak->handle's current-file state
						 * (pos_in_central_dir, current_file_ok, cur_file_info,
						 * cur_file_info_internal).  For uniqueFILE it also copies
						 * pak->handle's pfile_in_zip_read (+0x7C) over the NULL that
						 * unzReOpen stored, and unzOpenCurrentFile then closes and
						 * frees it -- so a non-unique file still open on the same pak
						 * is left with a freed pfile_in_zip_read. */
						z = fsh[*file].handleFiles.file.z;
						{
							void *savedFile = *(void **) z;         /* unz_s.file, +0 */

							unzSetCurrentFileInfoPosition( pak->handle, pakFile->zipPosition );
							Com_Memcpy( z, pak->handle, 128 );      /* sizeof(unz_s) */
							*(void **) z = savedFile;
							unzOpenCurrentFile( fsh[*file].handleFiles.file.z );
						}
						fsh[*file].zipFilePos = pakFile->zipPosition;

						if ( fs_debug->integer && !filter_flag ) {
							Com_Printf( "FS_FOpenFileRead: %s (found in '%s')\n", qpath, pak->pakFilename );
						}
						return (int) ( (unsigned long *) z )[17];   /* unz_s + 68: cur_file_info.uncompressed_size */
					}
					pakFile = pakFile->next;
				} while ( pakFile );
			}
		} else if ( search->dir ) {
			dir = search->dir;
			len = strlen( qpath );

			/* retail 0x00429D1A: the restricted-directory gate reads fs_restrict
			 * (0x0162D760), not fs_ignoreLocalized.  Written as a continue, as the
			 * Linux 1.1 dedicated server has it; a guarding if around the open path
			 * allocates registers differently. */
			if ( fs_restrict->integer || fs_numServerPaks ) {
				if ( Q_stricmp( &qpath[len - 4], ".cfg" )
					 && Q_stricmp( &qpath[len - 5], ".menu" )
					 && Q_stricmp( &qpath[len - 4], ".svg" )
					 && Q_stricmp( &qpath[len - 5], ".game" )
					 && Q_stricmp( &qpath[len - 20], ".dm_PROTOCOL_VERSION" )
					 && Q_stricmp( &qpath[len - 4], ".dat" ) ) {
					continue;
				}
			}
			FS_BuildOSPath3( dir->path, dir->gamedir, qpath, netpath, filter_flag );
			fsh[*file].handleFiles.file.o = fopen( netpath, "rb" );
			if ( fsh[*file].handleFiles.file.o ) {
				/* retail 0x0042A0B3-0x0042A11C: any directory file other than
				 * .cfg / .menu / .game / .dm_PROTOCOL_VERSION / .dat (.svg is
				 * NOT exempt here) sets fs_fakeChkSum = rand() + 1, which
				 * FS_ReferencedPakPureChecksums appends to the pure string. */
				if ( Q_stricmp( &qpath[len - 4], ".cfg" )
					 && Q_stricmp( &qpath[len - 5], ".menu" )
					 && Q_stricmp( &qpath[len - 5], ".game" )
					 && Q_stricmp( &qpath[len - 20], ".dm_PROTOCOL_VERSION" )
					 && Q_stricmp( &qpath[len - 4], ".dat" ) ) {
					fs_fakeChkSum = rand() + 1;
				}
				Q_strncpyz( fsh[*file].name, qpath, sizeof( fsh[*file].name ) );
				fsh[*file].zipFile = qfalse;
				if ( fs_debug->integer && !filter_flag ) {
					Com_Printf( "FS_FOpenFileRead: %s (found in '%s/%s')\n",
								qpath, dir->path, dir->gamedir );
				}
				/* retail 0x0042A18F-0x0042A1E2: with fs_copyfiles set, a file
				 * found under the cd path is copied to the same place under
				 * fs_basepath.  The FS_CopyFile result is ignored. */
				if ( fs_copyfiles->integer && !Q_stricmp( dir->path, fs_cdpath->string ) ) {
					char copypath[MAX_OSPATH];

					FS_BuildOSPath3( fs_basepath->string, dir->gamedir, qpath, copypath, filter_flag );
					FS_CopyFile( netpath, copypath );
				}
				return FS_filelength( *file );
			}
		}
	}

	if ( fs_debug->integer && !filter_flag ) {
		Com_Printf( "Can't find %s\n", qpath );
	}
	*file = 0;
	return -1;
}

/* ---- FS_FOpenFileReadStream  0x0042A200 ---- */
int FS_FOpenFileReadStream( const char *qpath, fileHandle_t *file, qboolean uniqueFILE ) {
	return FS_FOpenFileRead_Internal( qpath, file, uniqueFILE, 1 );
}

/* ---- FS_FOpenFileRead  0x0042A210 ---- */
int FS_FOpenFileRead( const char *qpath, fileHandle_t *file, qboolean uniqueFILE ) {
	fs_loadingMode = 1;
	return FS_FOpenFileRead_Internal( qpath, file, uniqueFILE, 0 );
}

/* ---- FS_TouchFile  0x0042A230 ---- VERIFIED */
qboolean FS_TouchFile( const char *qpath ) {
	fileHandle_t f;

	fs_loadingMode = 1;
	FS_FOpenFileRead_Internal( qpath, &f, qfalse, 0 );
	if ( !f ) {
		return qfalse;
	}
	FS_FCloseFile( f );
	return qtrue;
}

/* ---- FS_ShortOSFilePath  0x0042A270 ---- VERIFIED */
char *FS_ShortOSFilePath( const char *fullpath ) {
	searchpath_t *search;
	directory_t *dir;
	FILE *f;
	char netpath[MAX_OSPATH];

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		if ( !FS_UseSearchPath( search ) ) {
			continue;
		}
		dir = search->dir;
		if ( dir ) {
			FS_BuildOSPath3( dir->path, dir->gamedir, fullpath, netpath, 0 );
			f = fopen( netpath, "rb" );
			if ( f ) {
				fclose( f );
				return va( "%s/%s", dir->gamedir, fullpath );
			}
		}
	}
	return NULL;
}

/* ---- FS_Delete  0x0042A330 ---- VERIFIED */
int FS_Delete( const char *filename ) {
	char osPath[MAX_OSPATH];
	char c;

	if ( filename && filename[0] && !Q_strncmp( filename, "save", 4 ) ) {
		c = filename[4];
		if ( c == '/' || c == '\\' ) {
			/* retail 0x0042A396: FS_BuildOSPath3( fs_homepath->string, fs_gamedir,
			 * filename, out, 0 ), the same three-part join FS_MakeReadOnly uses. */
			FS_BuildOSPath3( fs_homepath->string, fs_gamedir, filename, osPath, 0 );
			if ( remove( osPath ) != -1 ) {
				return 1;
			}
		}
	}
	return 0;
}

/* ---- FS_MakeReadOnly  0x0042A3F0 ---- VERIFIED */
qboolean FS_MakeReadOnly( const char *filename, qboolean readOnly ) {
	char osPath[MAX_OSPATH];
	DWORD attr, newAttr;

	/* retail 0x0042A402: FS_BuildOSPath3( fs_homepath->string, fs_gamedir, filename, out, 0 ),
	 * matching the path FS_FOpenFileWrite writes. */
	FS_BuildOSPath3( fs_homepath->string, fs_gamedir, filename, osPath, 0 );
	attr = GetFileAttributesA( osPath );

	if ( !readOnly ) {
		newAttr = attr & ~FILE_ATTRIBUTE_READONLY;
	} else {
		newAttr = attr | FILE_ATTRIBUTE_READONLY;
	}

	if ( newAttr == attr ) {
		return qfalse;
	}
	SetFileAttributesA( osPath, newAttr );
	return qtrue;
}

/* ---- FS_Read  0x0042A490 ---- VERIFIED */
int FS_Read( void *buffer, int len, fileHandle_t f ) {
	int block, remaining;
	int read;
	byte *buf;
	int tries;

	if ( !f ) {
		return 0;
	}

	buf = (byte *)buffer;

	if ( !fsh[f].zipFile ) {
		remaining = len;
		tries = 0;
		while ( remaining ) {
			block = remaining;
			read = fread( buf, 1, block, fsh[f].handleFiles.file.o );
			if ( read == 0 ) {
				if ( !tries ) {
					tries = 1;
				} else {
					return len - remaining;
				}
			}

			if ( read == -1 ) {
				if ( f >= 51 && f < MAX_FILE_HANDLES ) {
					return -1;
				}
				Com_Error( ERR_FATAL, "\x15" "FS_Read: -1 bytes read" );
			}

			remaining -= read;
			buf += read;
		}
		return len;
	}
	return unzReadCurrentFile( fsh[f].handleFiles.file.z, len, buffer );
}

/* ---- FS_Write  0x0042A560 ---- VERIFIED */
int FS_Write( const void *buffer, int len, fileHandle_t f ) {
	int block, remaining;
	int written;
	byte *buf;
	int tries;
	FILE *file;

	if ( !f ) {
		return 0;
	}

	file = fsh[f].handleFiles.file.o;
	buf = (byte *)buffer;
	remaining = len;
	tries = 0;
	while ( remaining ) {
		block = remaining;
		written = fwrite( buf, 1, block, file );
		if ( written == 0 ) {
			if ( tries ) {
				Com_Printf( "FS_Write: 0 bytes written\n" );
				return 0;
			}
			tries = 1;
		} else if ( written == -1 ) {
			Com_Printf( "FS_Write: -1 bytes written\n" );
			return 0;
		}
		remaining -= written;
		buf += written;
	}

	if ( fsh[f].handleSync ) {
		fflush( file );
	}
	return len;
}

/* ---- FS_Printf  0x0042A610 ---- VERIFIED */
void QDECL FS_Printf( fileHandle_t f, const char *fmt, ... ) {
	va_list argptr;
	char msg[4096];

	va_start( argptr, fmt );
	vsprintf( msg, fmt, argptr );
	va_end( argptr );

	FS_Write( msg, strlen( msg ), f );
}

/* ---- FS_Seek  0x0042A690 ---- VERIFIED */
/* RTCW's FS_Seek with CoD1's seekable zip branch (Mac 1.3 structure): the
   position comes from minizip's unztell, the stdio branch takes the FILE
   first and maps the origin in a switch. */
int FS_Seek( fileHandle_t f, int offset, int origin ) {
	int _origin;
	int remaining, read;
	int pos;
	byte buffer[65536];

	if ( fsh[f].streamed ) {
		fsh[f].streamed = qfalse;
		FS_Seek( f, offset, origin );
		fsh[f].streamed = qtrue;
	}

	if ( fsh[f].zipFile == qtrue ) {
		if ( offset == 0 && origin == FS_SEEK_SET ) {
			unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
			return unzOpenCurrentFile( fsh[f].handleFiles.file.z );
		}
		if ( offset == 0 && origin == FS_SEEK_CUR ) {
			return 0;
		}

		pos = unztell( fsh[f].handleFiles.file.z );

		if ( origin == FS_SEEK_CUR ) {
			if ( offset < 0 ) {
				unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
				unzOpenCurrentFile( fsh[f].handleFiles.file.z );
				remaining = pos + offset;
			} else {
				remaining = offset;
			}
		} else if ( origin == FS_SEEK_END ) {
			if ( FS_filelength( f ) + offset < pos ) {
				unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
				unzOpenCurrentFile( fsh[f].handleFiles.file.z );
				remaining = FS_filelength( f ) + offset;
			} else {
				remaining = FS_filelength( f ) + offset - pos;
			}
		} else if ( origin == FS_SEEK_SET ) {
			if ( offset < pos ) {
				unzSetCurrentFileInfoPosition( fsh[f].handleFiles.file.z, fsh[f].zipFilePos );
				unzOpenCurrentFile( fsh[f].handleFiles.file.z );
				remaining = offset;
			} else {
				remaining = offset - pos;
			}
		} else {
			return -1;
		}

		while ( remaining ) {
			if ( remaining < (int)sizeof( buffer ) ) {
				read = FS_Read( buffer, remaining, f );
				remaining = 0;
			} else {
				read = FS_Read( buffer, sizeof( buffer ), f );
				remaining -= sizeof( buffer );
			}
			if ( !read ) {
				return -1;
			}
		}
		return 0;
	} else {
		FILE *file;

		file = FS_FileForHandle( f );
		switch ( origin ) {
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			return 0;
		}
		return fseek( file, offset, _origin );
	}
}

/* ---- FS_ReadFile  0x0042A8B0 ---- VERIFIED */
int FS_ReadFile( const char *qpath, void **buffer ) {
	extern cvar_t *com_journal;
	extern fileHandle_t com_journalFile;
	fileHandle_t h;
	byte *buf;
	int len;
	qboolean isConfig;

	if ( !qpath || !qpath[0] ) {
		Com_Error( ERR_FATAL, "\x15" "FS_ReadFile with empty name\n" );
	}

	buf = NULL;

	if ( strstr( qpath, ".cfg" ) ) {
		isConfig = qtrue;
		if ( com_journal && com_journal->integer == 2 ) {
			Com_DPrintf( "Loading %s from journal file.\n", qpath );
			if ( FS_Read( &len, sizeof( len ), com_journalFile ) != sizeof( len ) ) {
				if ( buffer ) {
					*buffer = NULL;
				}
				return -1;
			}
			if ( !len ) {
				if ( buffer == NULL ) {
					return 1;
				}
				*buffer = NULL;
				return -1;
			}
			if ( buffer == NULL ) {
				return len;
			}
			buf = Hunk_AllocateTempMemoryInternal( len + 1 );
			*buffer = buf;
			if ( FS_Read( buf, len, com_journalFile ) != len ) {
				Com_Error( ERR_FATAL, "EXE_ERR_JOURNAL_FILE_READ" );
			}
			fs_loadStack++;
			buf[len] = 0;
			return len;
		}
	} else {
		isConfig = qfalse;
	}

	fs_loadingMode = 1;
	len = FS_FOpenFileRead_Internal( qpath, &h, qfalse, 0 );
	if ( !h ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing zero for %s to journal file.\n", qpath );
			len = 0;
			FS_Write( &len, sizeof( len ), com_journalFile );
			fflush( fsh[com_journalFile].handleFiles.file.o );
		}
		return -1;
	}

	if ( !buffer ) {
		if ( isConfig && com_journal && com_journal->integer == 1 ) {
			Com_DPrintf( "Writing len for %s to journal file.\n", qpath );
			FS_Write( &len, sizeof( len ), com_journalFile );
			FS_Flush( com_journalFile );
		}
		FS_FCloseFile( h );
		return len;
	}

	fs_loadStack++;
	buf = Hunk_AllocateTempMemoryInternal( len + 1 );
	*buffer = buf;
	FS_Read( buf, len, h );
	buf[len] = 0;
	FS_FCloseFile( h );

	if ( isConfig && com_journal && com_journal->integer == 1 ) {
		Com_DPrintf( "Writing %s to journal file.\n", qpath );
		FS_Write( &len, sizeof( len ), com_journalFile );
		FS_Write( buf, len, com_journalFile );
		FS_Flush( com_journalFile );
	}
	return len;
}

/* ---- FS_ResetFiles  0x0042AB40 ---- VERIFIED */
void FS_ResetFiles( void ) {
	fs_loadStack = 0;
}

/* ---- FS_FreeFile  0x0042AB50 ---- VERIFIED */
void FS_FreeFile( void *buffer ) {
	extern void Hunk_FreeTempMemory( void *buf );

	if ( !buffer ) {
		Com_Error( ERR_FATAL, "\x15" "FS_FreeFile( NULL )" );
	}
	fs_loadStack--;

	/* Hunk_FreeTempMemory falls back to free() itself when there is no hunk */
	Hunk_FreeTempMemory( buffer );
}

/* ---- FS_WriteFile  0x0042ABC0 ---- VERIFIED */
void FS_WriteFile( const char *qpath, const void *buffer, int size ) {
	fileHandle_t f;

	if ( !qpath || !buffer ) {
		Com_Error( ERR_FATAL, "\x15" "FS_WriteFile: NULL parameter" );
	}

	f = FS_FOpenFileWrite( qpath );
	if ( !f ) {
		Com_Printf( "Failed to open %s\n", qpath );
		return;
	}

	FS_Write( buffer, size, f );
	FS_FCloseFile( f );
}

/* ---- FS_LoadZipFile  0x0042AC20 ---- */
/* Q3's FS_LoadZipFile over minizip, with CoD1's malloc/Sys_OutOfMemoryError
   allocations and the per-file basename. */
typedef struct {
	unsigned long number_entry;
	unsigned long size_comment;
} unz_global_info;

unzFile  unzOpen( const char *path );
int      unzGetGlobalInfo( unzFile file, unz_global_info *pglobal_info );
int      unzGoToFirstFile( unzFile file );
int      unzGoToNextFile( unzFile file );
int      unzGetCurrentFileInfoPosition( unzFile file, unsigned long *pos );

pack_t *FS_LoadZipFile( char *zipfile, const char *basename ) {
	fileInPack_t    *buildBuffer;
	pack_t          *pack;
	unzFile         uf;
	int             err;
	unz_global_info gi;
	char            filename_inzip[MAX_ZPATH];
	unz_file_info   file_info;
	int             i, len;
	long            hash;
	int             fs_numHeaderLongs;
	int             *fs_headerLongs;
	char            *namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen( zipfile );
	err = unzGetGlobalInfo( uf, &gi );

	if ( err != 0 ) {
		return NULL;
	}

	fs_packFiles += gi.number_entry;

	len = 0;
	unzGoToFirstFile( uf );
	for ( i = 0; i < gi.number_entry; i++ )
	{
		err = unzGetCurrentFileInfo( NULL, 0, 0, uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL );
		if ( err != 0 ) {
			break;
		}
		len += strlen( filename_inzip ) + 1;
		unzGoToNextFile( uf );
	}

	buildBuffer = Z_MallocInternal( ( gi.number_entry * sizeof( fileInPack_t ) ) + len );
	namePtr = ( (char *) buildBuffer ) + gi.number_entry * sizeof( fileInPack_t );
	fs_headerLongs = Z_MallocInternal( gi.number_entry * sizeof( int ) );

	// get the hash table size from the number of files in the zip
	// because lots of custom pk3 files have less than 32 or 64 files
	for ( i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1 ) {
		if ( i > gi.number_entry ) {
			break;
		}
	}

	pack = Z_MallocInternal( sizeof( pack_t ) + i * sizeof( fileInPack_t * ) );
	pack->hashSize = i;
	pack->hashTable = ( fileInPack_t ** )( ( (char *) pack ) + sizeof( pack_t ) );
	for ( i = 0; i < pack->hashSize; i++ ) {
		pack->hashTable[i] = NULL;
	}

	Q_strncpyz( pack->pakFilename, zipfile, sizeof( pack->pakFilename ) );
	Q_strncpyz( pack->pakBasename, basename, sizeof( pack->pakBasename ) );

	// strip .pk3 if needed
	if ( strlen( pack->pakBasename ) > 4 && !Q_stricmp( pack->pakBasename + strlen( pack->pakBasename ) - 4, ".pk3" ) ) {
		pack->pakBasename[strlen( pack->pakBasename ) - 4] = 0;
	}

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile( uf );

	for ( i = 0; i < gi.number_entry; i++ )
	{
		int k;

		err = unzGetCurrentFileInfo( NULL, 0, 0, uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL );
		if ( err != 0 ) {
			break;
		}
		if ( file_info.uncompressed_size > 0 ) {
			fs_headerLongs[fs_numHeaderLongs++] = (int) file_info.crc;
		}
		Q_strlwr( filename_inzip );
		hash = FS_HashFileName( filename_inzip, pack->hashSize );
		buildBuffer[i].name = namePtr;
		strcpy( namePtr, filename_inzip );
		for ( k = strlen( namePtr ) - 1; k >= 0; k-- ) {
			if ( namePtr[k] == '/' || namePtr[k] == '\\' ) {
				break;
			}
		}
		buildBuffer[i].basename = namePtr + k + 1;
		namePtr += strlen( filename_inzip ) + 1;
		// store the file position in the zip
		unzGetCurrentFileInfoPosition( uf, (unsigned long *) &buildBuffer[i].zipPosition );
		//
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile( uf );
	}

	pack->checksum = Com_BlockChecksum( fs_headerLongs, 4 * fs_numHeaderLongs );
	pack->pure_checksum = Com_BlockChecksumKey( fs_headerLongs, 4 * fs_numHeaderLongs, fs_checksumFeed );

	free( fs_headerLongs );

	pack->buildBuffer = buildBuffer;
	return pack;
}

/* ---- FS_ReturnPath  0x0042B1A0 ---- VERIFIED */
int FS_ReturnPath( const char *zname, char *zpath, int *depth ) {
	int len, at, newdep;

	newdep = 0;
	zpath[0] = 0;
	len = 0;
	at = 0;

	while ( zname[at] != 0 ) {
		if ( zname[at] == '/' || zname[at] == '\\' ) {
			len = at;
			newdep++;
		}
		at++;
	}
	strcpy( zpath, zname );
	zpath[len] = 0;
	if ( len + 1 == at ) {
		newdep--;
	}
	*depth = newdep;

	return len;
}

/* ---- FS_AddFileToList  0x0042B200 ---- VERIFIED */
static int FS_AddFileToList( char *name, char *list[MAX_FOUND_FILES], int nfiles ) {
	int i;

	if ( nfiles == MAX_FOUND_FILES - 1 ) {
		return nfiles;
	}
	for ( i = 0 ; i < nfiles ; i++ ) {
		if ( !Q_stricmp( name, list[i] ) ) {
			return nfiles;
		}
	}
	list[nfiles] = CopyStringInternal( name );
	nfiles++;
	return nfiles;
}

/* ---- FS_ListFilteredFiles  0x0042B2A0 ---- VERIFIED */
char **FS_ListFilteredFiles( const char *path, const char *extension, char *filter, int *numfiles ) {
	int nfiles;
	char        **listCopy;
	char        *list[MAX_FOUND_FILES];
	searchpath_t *search;
	int i;
	int pathLength;
	int extensionLength;
	int length, pathDepth, temp;
	qboolean wantSubs;
	pack_t      *pak;
	fileInPack_t *buildBuffer;
	char zpath[MAX_ZPATH];
	/* retail's 64-byte buffer (frame 0x4270): its copy is an unbounded strcpy
	 * that runs on into the adjacent 256-byte frame buffer.  Other compilers
	 * lay the frame out differently, so they get room for any pak name. */
#ifdef COD1_VC7
	char dirName[64];
#else
	char dirName[MAX_ZPATH];
#endif

	if ( !path ) {
		*numfiles = 0;
		return NULL;
	}
	if ( !extension ) {
		extension = "";
	}
	if ( *path == '/' || *path == '\\' ) {
		path++;
	}

	wantSubs = Q_stricmp( extension, "/" ) == 0;

	pathLength = strlen( path );
	if ( pathLength ) {
		if ( path[pathLength - 1] == '\\' || path[pathLength - 1] == '/' ) {
			pathLength--;
		}
	}
	extensionLength = strlen( extension );
	nfiles = 0;
	FS_ReturnPath( path, zpath, &pathDepth );
	if ( *path ) {
		pathDepth++;
	}

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		if ( !FS_UseSearchPath( search ) ) {
			continue;
		}

		if ( search->pack ) {
			pak = search->pack;
			if ( !FS_PakIsPure( pak ) ) {
				continue;
			}
			buildBuffer = pak->buildBuffer;
			for ( i = 0 ; i < pak->numfiles ; i++ ) {
				char    *name;
				int zpathLen, depth;

				name = buildBuffer[i].name;

				if ( filter ) {
					if ( !Com_FilterPath( filter, name, qfalse ) ) {
						continue;
					}
					nfiles = FS_AddFileToList( name, list, nfiles );
				} else {
					zpathLen = FS_ReturnPath( name, zpath, &depth );

					if ( depth != pathDepth || pathLength > zpathLen ) {
						continue;
					}
					if ( pathLength > 0 && name[pathLength] != '/' ) {
						continue;
					}
					if ( Q_stricmpn( name, path, pathLength ) ) {
						continue;
					}

					length = strlen( name );
					if ( length < extensionLength ) {
						continue;
					}
					if ( Q_stricmp( &name[length - extensionLength], extension ) ) {
						continue;
					}
					temp = pathLength;
					if ( pathLength ) {
						temp++;
					}
					if ( wantSubs ) {
						/* retail 0x0042B514-0x0042B542: unbounded strcpy, then the
						 * last character is stripped. */
						strcpy( dirName, &name[temp] );
						/* retail strips unconditionally (with retail's frame an empty
						 * name's dirName[-1] store lands on a dead slot).  Pointer store
						 * because dirName[len - 1] = 0 makes modern /GS add a
						 * range-check fast-fail. */
#ifndef COD1_VC7
						if ( dirName[0] )
#endif
						*( dirName + strlen( dirName ) - 1 ) = 0;
						nfiles = FS_AddFileToList( dirName, list, nfiles );
					} else {
						nfiles = FS_AddFileToList( &name[temp], list, nfiles );
					}
				}
			}
		} else if ( search->dir ) {
			char netpath[MAX_OSPATH];
			int numSysFiles;
			char    **sysFiles;
			char    *name;

			if ( ( fs_restrict->integer || fs_numServerPaks )
				 && Q_stricmp( extension, "svg" ) ) {
				continue;
			}

			FS_BuildOSPath( search->dir->path, search->dir->gamedir, path, netpath );
			sysFiles = Sys_ListFiles( netpath, extension, filter, &numSysFiles, wantSubs );
			for ( i = 0 ; i < numSysFiles ; i++ ) {
				name = sysFiles[i];
				nfiles = FS_AddFileToList( name, list, nfiles );
			}
			Sys_FreeFileList( sysFiles );
		}
	}

	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = Z_MallocInternal( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

/* ---- FS_ListFiles  0x0042B6F0 ---- VERIFIED */
char **FS_ListFiles( const char *path, const char *extension, int *numfiles ) {
	return FS_ListFilteredFiles( path, extension, NULL, numfiles );
}

/* ---- FS_FreeFileList  0x0042B710 ---- VERIFIED */
void FS_FreeFileList( char **list ) {
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		free( list[i] );
	}
	free( list );
}

/* ---- FS_GetFileList  0x0042B750 ---- VERIFIED */
int FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize ) {
	int nFiles, i, nTotal, nLen;
	char        **pFiles;

	*listbuf = 0;
	nFiles = 0;
	nTotal = 0;

	if ( Q_stricmp( path, "$modlist" ) == 0 ) {
		return FS_GetModList( listbuf, bufsize );
	}

	pFiles = FS_ListFilteredFiles( path, extension, NULL, &nFiles );

	for ( i = 0 ; i < nFiles ; i++ ) {
		nLen = strlen( pFiles[i] ) + 1;
		if ( nTotal + nLen + 1 < bufsize ) {
			strcpy( listbuf, pFiles[i] );
			listbuf += nLen;
			nTotal += nLen;
		} else {
			nFiles = i;
			break;
		}
	}

	FS_FreeFileList( pFiles );

	return nFiles;
}

/* ---- FS_ConvertPath  0x0042B840 ---- VERIFIED */
void FS_ConvertPath( char *s ) {
	while ( *s ) {
		if ( *s == '\\' || *s == ':' ) {
			*s = '/';
		}
		s++;
	}
}

/* ---- FS_PathCmp  0x0042B860 ---- VERIFIED */
int FS_PathCmp( const char *s1, const char *s2 ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' ) {
			c1 -= ( 'a' - 'A' );
		}
		if ( c2 >= 'a' && c2 <= 'z' ) {
			c2 -= ( 'a' - 'A' );
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}

		if ( c1 < c2 ) {
			return -1;
		}
		if ( c1 > c2 ) {
			return 1;
		}
	} while ( c1 );

	return 0;
}

/* ---- FS_SortFileList  0x0042B8C0 ---- VERIFIED */
void FS_SortFileList( char **filelist, int numfiles ) {
	char **sortedlist;
	int i, j;

	sortedlist = Z_MallocInternal( ( numfiles + 1 ) * sizeof( *sortedlist ) );
	sortedlist[0] = NULL;

	for ( i = 0 ; i < numfiles ; i++ ) {
		for ( j = 0 ; j < i ; j++ ) {
			if ( FS_PathCmp( filelist[i], sortedlist[j] ) < 0 ) {
				break;
			}
		}
		{
			int k;
			for ( k = i ; k > j ; k-- ) {
				sortedlist[k] = sortedlist[k - 1];
			}
		}
		sortedlist[j] = filelist[i];
	}

	Com_Memcpy( filelist, sortedlist, numfiles * sizeof( *filelist ) );
	free( sortedlist );
}

/* ---- FS_DisplayPath  0x0042B960 ---- VERIFIED */
void FS_DisplayPath( qboolean showAll ) {
	extern const char *SEH_GetLanguageName( int language );
	extern int SEH_GetCurrentLanguage( void );
	searchpath_t *s;
	int i;

	Com_Printf( "Current language: %s\n", SEH_GetLanguageName( SEH_GetCurrentLanguage() ) );
	if ( fs_ignoreLocalized->integer ) {
		Com_Printf( "    localized assets are being ignored\n" );
	}

	Com_Printf( "Current search path:\n" );
	for ( s = fs_searchpaths ; s ; s = s->next ) {
		if ( showAll && !FS_UseSearchPath( s ) ) {
			continue;
		}

		if ( s->pack ) {
			Com_Printf( "%s (%i files)\n", s->pack->pakFilename, s->pack->numfiles );
			if ( s->localized ) {
				Com_Printf( "    localized assets pak file for %s\n", SEH_GetLanguageName( s->language ) );
			}
			if ( fs_numServerPaks ) {
				if ( !FS_PakIsPure( s->pack ) ) {
					Com_Printf( "    not on the pure list\n" );
				} else {
					Com_Printf( "    on the pure list\n" );
				}
			}
		} else {
			Com_Printf( "%s/%s\n", s->dir->path, s->dir->gamedir );
			if ( s->localized ) {
				Com_Printf( "    localized assets game folder for %s\n", SEH_GetLanguageName( s->language ) );
			}
		}
	}

	Com_Printf( "\nFile Handles:\n" );
	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].handleFiles.file.o ) {
			Com_Printf( "handle %i: %s\n", i, fsh[i].name );
		}
	}
}

/* ---- FS_FullPath_f  0x0042BB20 ---- VERIFIED */
void FS_FullPath_f( void ) {
	FS_DisplayPath( qfalse );
}

/* ---- FS_Path_f  0x0042BB30 ---- VERIFIED */
void FS_Path_f( void ) {
	FS_DisplayPath( qtrue );
}

/* ---- PakFileLanguage  0x0042BB40 ---- VERIFIED */
char *PakFileLanguage( const char *pakName ) {
	static char buffers[CODUO_FS_LANGUAGE_NAME_BUFFER_COUNT][CODUO_FS_LANGUAGE_NAME_BUFFER_SIZE];
	static int index;
	int i;

	index ^= 1;

	if ( strlen( pakName ) < 10 ) {
		buffers[index][0] = 0;
	} else {
		memset( buffers[index], 0, CODUO_FS_LANGUAGE_NAME_BUFFER_SIZE );
		for ( i = 10 ; i < CODUO_FS_LANGUAGE_NAME_BUFFER_SIZE ; i++ ) {
			if ( !pakName[i] ) {
				break;
			}
			if ( !isalpha( pakName[i] ) ) {
				break;
			}
			buffers[index][i - 10] = pakName[i];
		}
	}
	return buffers[index];
}

/* ---- paksort  0x0042BBE0 ---- VERIFIED */
int paksort( const void *a, const void *b ) {
	char        *aa, *bb;
	char        *langA, *langB;

	aa = *(char **)a;
	bb = *(char **)b;

	if ( Q_strncmp( aa, "          ", 10 ) ) {
		return FS_PathCmp( aa, bb );
	}
	if ( Q_strncmp( bb, "          ", 10 ) ) {
		return FS_PathCmp( aa, bb );
	}

	langA = PakFileLanguage( aa );
	langB = PakFileLanguage( bb );

	if ( !Q_stricmp( langA, "english" ) ) {
		if ( !Q_stricmp( langB, "english" ) ) {
			return FS_PathCmp( aa, bb );
		}
		return -1;
	}

	if ( !Q_stricmp( langB, "english" ) ) {
		return 1;
	}

	return FS_PathCmp( aa, bb );
}

/* ---- FS_AddSearchPath  0x0042BCF0 ---- VERIFIED */
void FS_AddSearchPath( searchpath_t *sp ) {
	searchpath_t *s;

	if ( sp->localized && fs_searchpaths ) {
		s = fs_searchpaths;
		while ( s->next ) {
			if ( s->next->localized ) {
				break;
			}
			s = s->next;
		}
		sp->next = s->next;
		s->next = sp;
	} else {
		sp->next = fs_searchpaths;
		fs_searchpaths = sp;
	}
}

/* 0x01407434: set once the "Supported languages are:" list has been printed
 * for a localized pak with a bad language name; only touched at 0x0042C08B
 * (read) and 0x0042C0E6 (write). */
static int fs_listedLanguages;

/* ---- FS_AddPakFilesForGameDirectory  0x0042BD30 ---- */
void FS_AddPakFilesForGameDirectory( const char *path, const char *dir ) {
	char pakfile[MAX_OSPATH];
	char        **pakfiles;
	char        *sorted[1024];      /* retail frame: char *[0x400] at esp+0x120 */
	int numfiles;
	int i, j;
	searchpath_t *search;
	pack_t      *pak;

	/* 0x0042BD5F-0x0042BD89: FS_BuildOSPath3( path@<eax>, dir, "", out, 0 ),
	 * then the trailing separator is cut unconditionally (the join always
	 * holds both separators, so strlen >= 2). */
	FS_BuildOSPath3( path, dir, "", pakfile, 0 );
	/* pointer store: pakfile[len - 1] = 0 makes modern /GS add a range-check fast-fail */
	*( pakfile + strlen( pakfile ) - 1 ) = 0;

	pakfiles = Sys_ListFiles( pakfile, ".pk3", NULL, &numfiles, qfalse );

	/* 0x0042BDAB: cmp esi,400h -- the cap is sorted[]'s size, not
	 * MAX_SEARCH_PATHS.  0x0042BDB9-0x0042BDC6: retail's literal is "(%1/%1)"
	 * with numfiles and 0x400 pushed but never consumed.  Both the 2003 CRT
	 * and the UCRT standard_base (_vsnprintf) treat "%1" + non-type char as
	 * width -> other -> normal, so each "%1" is dropped, the next character
	 * is emitted, and the line renders as "(/)" in both. */
	if ( numfiles > 1024 ) {
		Com_Printf( "WARNING: Exceeded max number of pak files in %s/%s (%1/%1)\n", path, dir, numfiles, 1024 );
		numfiles = 1024;
	}

	/* 0x0042BE00-0x0042BEBB: copy into sorted[] and rename in place so that
	 * paksort's english-first branch fires and mp_ paks sort last. */
	for ( i = 0 ; i < numfiles ; i++ ) {
		sorted[i] = pakfiles[i];
		if ( !Q_strncmp( sorted[i], "mp_", 3 ) ) {          /* 0x0042BE48 */
			memcpy( sorted[i], "zz", 2 );
		}
		if ( !Q_strncmp( sorted[i], "localized_", 10 ) ) {  /* 0x0042BE90 */
			memcpy( sorted[i], "          ", 10 );
		}
	}

	qsort( sorted, numfiles, sizeof( char * ), paksort );

	for ( i = 0 ; i < numfiles ; i++ ) {
		int localized;
		int language;
		char        *lang;

		if ( !Q_strncmp( sorted[i], "sp_", 3 ) ) {          /* 0x0042BEFB */
			continue;
		}
		/* 0x0042BF79: retail's "restore" is strncpy(s, "mp_", 1); s[1] = 0;
		 * s[2] = '_'.  That truncates the name to "m", so FS_LoadZipFile below
		 * fails to open "<path>/<dir>/m" and every mp_*.pk3 and zz_*.pk3 is
		 * silently skipped.  Retail bug, kept. */
		if ( !Q_strncmp( sorted[i], "zz_", 3 ) ) {
			char *s = sorted[i];
			strncpy( s, "mp_", 1 );
			s[1] = 0;
			sorted[i][2] = '_';
		}

		if ( !Q_strncmp( sorted[i], "          ", 10 ) ) {
			memcpy( sorted[i], "localized_", 10 );
			localized = 1;
			lang = PakFileLanguage( sorted[i] );
			if ( !lang[0] ) {
				/* no language: the pak is not loaded */
				Com_Printf( "WARNING: Localized assets pak file %s/%s/%s has invalid name "
							"(no language specified). Proper naming convention is: "
							"localized_[language]_pak#.pk3\n", path, dir, sorted[i] );
				continue;
			}
			if ( !SEH_GetLanguageIndexForName( lang, &language ) ) {
				/* unknown language: the pak is not loaded; the first time,
				 * the supported names are listed too */
				Com_Printf( "WARNING: Localized assets pak file %s/%s/%s has invalid name "
							"(bad language name specified). Proper naming convention is: "
							"localized_[language]_pak#.pk3\n", path, dir, sorted[i] );
				if ( !fs_listedLanguages ) {
					Com_Printf( "Supported languages are:\n" );
					for ( j = 0 ; j < NUM_LANGUAGES ; j++ ) {
						Com_Printf( "    %s\n", SEH_GetLanguageName( j ) );
					}
					fs_listedLanguages = 1;
				}
				continue;
			}
		} else {
			localized = 0;
			language = 0;
		}

		FS_BuildOSPath3( path, dir, sorted[i], pakfile, 0 );
		pak = FS_LoadZipFile( pakfile, sorted[i] );
		if ( !pak ) {
			continue;
		}
		strcpy( pak->pakGamename, dir );

		search = Z_MallocInternal( sizeof( searchpath_t ) );
		search->pack = pak;
		search->localized = localized;
		search->language = language;
		FS_AddSearchPath( search );
	}

	Sys_FreeFileList( pakfiles );
}

/* ---- FS_AddGameDirectory  0x0042C250 ---- VERIFIED */
void FS_AddGameDirectory( const char *path, const char *gameName, qboolean localized, int language ) {
	searchpath_t *sp;
	searchpath_t *search;
	directory_t *d;
	char folder[64];
	char fs_path[MAX_OSPATH];

	if ( localized ) {
		/* 0x0042C281: SEH_GetLanguageName, inlined (out of range takes entry 0) */
		Com_sprintf( folder, sizeof( folder ), "%s_%s",
					 gameName, SEH_GetLanguageName( language ) );
	} else {
		Q_strncpyz( folder, gameName, sizeof( folder ) );
	}

	for ( sp = fs_searchpaths ; sp ; sp = sp->next ) {
		if ( sp->dir && !Q_stricmp( sp->dir->path, path ) && !Q_stricmp( sp->dir->gamedir, folder ) ) {
			if ( sp->localized != localized ) {
				Com_Printf( "WARNING: game folder %s/%s added as both localized & non-localized. "
							"Using folder as %s\n",
							path, folder, sp->localized ? "localized" : "non-localized" );
			}
			if ( sp->localized && sp->language != language ) {
				/* 0x0042C3AF: retail pushes only the format, so the two %s read
				 * the caller's saved EDI/ESI; one of them is always a language
				 * loop counter (0..13, or -1 after the loop), so retail faults
				 * here (the Mac 1.3 build passes no values either).  The VC7
				 * build keeps that call shape; other builds pass path and folder. */
#ifdef COD1_VC7
				Com_Printf( "WARNING: game golder %s/%s re-added as localized folder with "
							"different language\n" );
#else
				Com_Printf( "WARNING: game golder %s/%s re-added as localized folder with "
							"different language\n", path, folder );
#endif
			}
			return;
		}
	}

	if ( localized ) {
		FS_BuildOSPath3( path, folder, "", fs_path, 0 );
		/* 0x0042C34B: unconditional cut (the join always holds both separators,
		 * so len >= 2); pointer store because fs_path[len - 1] = 0 makes modern
		 * /GS add a range-check fast-fail */
		*( fs_path + strlen( fs_path ) - 1 ) = 0;
		if ( !Sys_DirectoryHasContents_m( fs_path ) ) {
			return;
		}
	} else {
		Q_strncpyz( fs_gamedir, folder, sizeof( fs_gamedir ) );
	}

	search = Z_MallocInternal( sizeof( searchpath_t ) );
	d = Z_MallocInternal( sizeof( directory_t ) );
	search->dir = d;
	Q_strncpyz( search->dir->path, path, sizeof( search->dir->path ) );
	Q_strncpyz( search->dir->gamedir, folder, sizeof( search->dir->gamedir ) );

	search->localized = localized;
	search->language = language;

	FS_AddSearchPath( search );

	FS_AddPakFilesForGameDirectory( path, folder );
}

/* ---- FS_AddLocalizedGameDirectory  0x0042C530 ---- VERIFIED */
void FS_AddLocalizedGameDirectory( const char *path, const char *gameName ) {
	int i;

	for ( i = NUM_LANGUAGES - 1 ; i >= 0 ; i-- ) {
		FS_AddGameDirectory( path, gameName, qtrue, i );
	}
	FS_AddGameDirectory( path, gameName, qfalse, 0 );
}

/* ---- FS_AddNonPackFileDirectory_Internal  0x0042C560 ---- VERIFIED */
/* Retail passes gamedir in ECX and path, dir, ext on the stack.  Builds one
 * fs_dirFileList_t for <path>/<gamedir>/<dir> and pushes it on the live head
 * (0x01407424); FS_GetDataForFile's second lookup reads the published head. */
void FS_AddNonPackFileDirectory_Internal( const char *path, const char *gamedir,
										  const char *dir, const char *ext ) {
	char              gamePath[MAX_OSPATH];     /* ebp-0x104 */
	char              fs_path[MAX_OSPATH];      /* ebp-0x204 */
	char            **files;
	char             *names;
	char             *p;
	char             *name;
	int               numFiles;
	int               namesLen;
	int               hashSize;
	int               i;
	long              hash;
	fs_dirFileList_t *list;
	fs_dirFile_t     *buf;
	fs_dirFile_t     *e;
	MSG               msg;

	/* 0x0042C587-0x0042C5CD */
	sprintf( gamePath, "%s/%s", gamedir, dir );
	FS_BuildOSPath3( path, gamePath, "", fs_path, 0 );
	/* retail 0x0042C5CD: unconditional cut; pointer store because fs_path[len - 1] = 0
	   makes modern /GS add a range-check fast-fail */
	*( fs_path + strlen( fs_path ) - 1 ) = 0;

	/* 0x0042C5E6: ECX = directory, EDX = filter (NULL) */
	files = Sys_ListFiles( fs_path, ext, NULL, &numFiles, qfalse );

	namesLen = 0;
	for ( i = 0 ; i < numFiles ; i++ ) {
		namesLen += strlen( files[i] ) + 1;
	}

	/* 0x0042C618-0x0042C62C */
	for ( i = 1 ; i <= 1024 ; i <<= 1 ) {
		if ( i > numFiles ) {
			break;
		}
	}

	/* 0x0042C636: sizeof( fs_dirFileList_t ) == 0x114 */
	list = Z_MallocInternal( sizeof( fs_dirFileList_t ) + i * sizeof( fs_dirFile_t * ) );
	list->hashTable = (fs_dirFile_t **)( list + 1 );
	list->hashSize = i;
	for ( i = 0 ; i < list->hashSize ; i++ ) {
		list->hashTable[i] = NULL;
	}

	/* 0x0042C6C8-0x0042C6EF */
	Q_strncpyz( list->dir, dir, sizeof( list->dir ) );
	list->next = fs_dirFileLists;
	fs_dirFileLists = list;
	list->numFiles = numFiles;

	/* 0x0042C6F9: 16 bytes per record, then the name pool */
	buf = Z_MallocInternal( numFiles * sizeof( fs_dirFile_t ) + namesLen );

	names = (char *)( buf + numFiles );
	e = buf;
	for ( i = 0 ; i < numFiles ; i++, e++ ) {
		name = files[i];
		for ( p = name ; *p ; p++ ) {
			*p = tolower( *p );
		}

		hash = FS_HashFileName( name, list->hashSize );
		e->name = names;
		strcpy( names, name );
		names += strlen( name ) + 1;
		e->next = list->hashTable[hash];
		e->data = NULL;
		e->clearCallback = NULL;
		list->hashTable[hash] = e;
	}
	list->buildBuffer = buf;

	/* 0x0042C81D-0x0042C849: inlined Sys_FreeFileList (0x00460990) */
	Sys_FreeFileList( files );

	/* 0x0042C84C-0x0042C89C */
	if ( PeekMessageA( &msg, NULL, WM_POWERBROADCAST, WM_POWERBROADCAST, PM_NOREMOVE ) ) {
		if ( GetMessageA( &msg, NULL, WM_POWERBROADCAST, WM_POWERBROADCAST ) > 0 ) {
			TranslateMessage( &msg );
			DispatchMessageA( &msg );
		}
	}
	SetThreadExecutionState( 2 /* ES_DISPLAY_REQUIRED */ );
}

/* ---- FS_AddNonPackFileDirectory  0x0042C8C0 ---- VERIFIED */
void FS_AddNonPackFileDirectory( const char *dir, const char *ext ) {
	searchpath_t *search;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		if ( search->pack ) {
			continue;
		}
		FS_AddNonPackFileDirectory_Internal( search->dir->path, search->dir->gamedir,
											 dir, ext );
	}
}

/* ---- FS_GetDataForFile  0x0042C900 ---- VERIFIED */
fileData_t *FS_GetDataForFile( const char *dir, const char *name, const char *ext ) {

	char              filename[1024];   /* retail's frame: Buffer[1024] at ebp-0x404 */
	searchpath_t     *search;
	fileInPack_t     *pakFile;
	fs_dirFileList_t *list;
	fs_dirFile_t     *entry;
	long              hash;

	sprintf( filename, "%s/%s%s", dir, name, ext );

	/* 0x0042C93C reads 0x01407428 (fs_savedSearchpaths), the same generation
	 * FS_ClearDataForFiles walks and the dir-list half below uses. */
	for ( search = fs_savedSearchpaths ; search ; search = search->next ) {
		if ( !FS_UseSearchPath( search ) ) {
			continue;
		}
		if ( !search->pack ) {
			continue;
		}

		hash = FS_HashFileName( filename, search->pack->hashSize );
		for ( pakFile = search->pack->hashTable[hash] ; pakFile ; pakFile = pakFile->next ) {
			if ( !FS_FilenameCompare( pakFile->name, filename ) ) {
				/* 0x0042CA53: lea eax, [esi+4] */
				return (fileData_t *)&pakFile->basename;
			}
		}
	}

	sprintf( filename, "%s%s", name, ext );

	for ( list = fs_savedDirFileLists ; list ; list = list->next ) {
		/* 0x0042C9E9: the CRT _stricmp, not Q_stricmp */
		if ( _stricmp( list->dir, dir ) ) {
			continue;
		}

		hash = FS_HashFileName( filename, list->hashSize );
		for ( entry = list->hashTable[hash] ; entry ; entry = entry->next ) {
			if ( !FS_FilenameCompare( entry->name, filename ) ) {
				return (fileData_t *)entry;
			}
		}
	}

	return NULL;
}

/* ---- FS_ClearDataForFiles  0x0042CA60 ---- VERIFIED */
void FS_ClearDataForFiles( void *lowEnd, void *highEnd ) {
	searchpath_t     *search;
	fileInPack_t     *pakFile;
	fs_dirFileList_t *list;
	fs_dirFile_t     *entry;
	fileData_t       *fd;
	int i;

	for ( search = fs_savedSearchpaths ; search ; search = search->next ) {
		if ( !search->pack ) {
			continue;
		}
		for ( i = 0 ; i < search->pack->hashSize ; i++ ) {
			for ( pakFile = search->pack->hashTable[i] ; pakFile ; pakFile = pakFile->next ) {
				if ( (unsigned char *)pakFile->payload < (unsigned char *)lowEnd ||
					 (unsigned char *)pakFile->payload >= (unsigned char *)highEnd ) {
					continue;
				}
				/* 0x0042CA53's shape: the callback sees fileInPack_t + 4 */
				fd = (fileData_t *)&pakFile->basename;
				if ( pakFile->clearCallback ) {
					pakFile->clearCallback( fd );
					pakFile->clearCallback = NULL;
				}
				pakFile->payload = NULL;
			}
		}
	}

	for ( list = fs_savedDirFileLists ; list ; list = list->next ) {
		for ( i = 0 ; i < list->hashSize ; i++ ) {
			for ( entry = list->hashTable[i] ; entry ; entry = entry->next ) {
				if ( (unsigned char *)entry->data < (unsigned char *)lowEnd ||
					 (unsigned char *)entry->data >= (unsigned char *)highEnd ) {
					continue;
				}
				if ( entry->clearCallback ) {
					entry->clearCallback( (fileData_t *)entry );
					entry->clearCallback = NULL;
				}
				entry->data = NULL;
			}
		}
	}
}

/* ---- FS_ShutdownSearchPaths  0x0042CB50 ---- VERIFIED */
static void FS_ShutdownSearchPaths( searchpath_t *paths ) {
	searchpath_t *p, *next;

	for ( p = paths ; p ; p = next ) {
		next = p->next;

		if ( p->pack ) {
			Unz_Close( p->pack->handle );
			free( p->pack->buildBuffer );
			free( p->pack );
		}
		if ( p->dir ) {
			free( p->dir );
		}
		free( p );
	}
}

/* ---- FS_ShutdownFileLists  0x0042CBD0 ---- VERIFIED */
/* retail calls it at all three sites; VC7 would inline it into FS_Shutdown */
#pragma auto_inline(off)
static void FS_ShutdownFileLists( fs_dirFileList_t *lists ) {
	fs_dirFileList_t *n, *next;

	for ( n = lists ; n ; n = next ) {
		next = n->next;
		free( n->buildBuffer );
		free( n );
	}
}
#pragma auto_inline(on)

/* ---- FS_ClearMemory  0x0042CC10 ---- VERIFIED */
void FS_ClearMemory( void ) {
	if ( fs_savedSearchpaths != fs_searchpaths ) {
		FS_ShutdownSearchPaths( fs_savedSearchpaths );
		fs_savedSearchpaths = fs_searchpaths;
	}
	if ( fs_savedDirFileLists != fs_dirFileLists ) {
		FS_ShutdownFileLists( fs_savedDirFileLists );
		fs_savedDirFileLists = fs_dirFileLists;
	}
}

/* ---- FS_ShutdownServerPakNames  0x0042CC50 ---- VERIFIED */
void FS_ShutdownServerPakNames( void ) {
	int i;

	for ( i = 0 ; i < fs_numServerPaks ; i++ ) {
		if ( fs_serverPakNames[i] ) {
			free( fs_serverPakNames[i] );
		}
		fs_serverPakNames[i] = NULL;
	}
	fs_numServerPaks = 0;
}

/* ---- FS_ShutdownServerReferencedPaks  0x0042CC90 ---- VERIFIED */
void FS_ShutdownServerReferencedPaks( void ) {
	int i;

	for ( i = 0 ; i < fs_numServerReferencedPaks ; i++ ) {
		if ( fs_serverReferencedPakNames[i] ) {
			free( fs_serverReferencedPakNames[i] );
		}
		fs_serverReferencedPakNames[i] = NULL;
	}
	fs_numServerReferencedPaks = 0;
}

/* ---- FS_Shutdown  0x0042CCD0 ---- VERIFIED */
void FS_Shutdown( qboolean closemfp ) {
	extern void MSS_StopSounds( int flags );
	extern int __stdcall Clear__16CStringEdPackageFi( void *package, int bKeepFileInfo );
	extern int TheStringPackage;
	int i;

#ifndef DEDICATED
	MSS_StopSounds( 8 );
#endif
	Clear__16CStringEdPackageFi( &TheStringPackage, 0 );

	for ( i = 1 ; i < MAX_FILE_HANDLES ; i++ ) {
		if ( fsh[i].fileSize ) {
			FS_FCloseFile( i );
		}
	}

	if ( closemfp ) {
		FS_ShutdownSearchPaths( fs_searchpaths );
		FS_ShutdownFileLists( fs_dirFileLists );
		fs_savedSearchpaths = NULL;
		fs_savedDirFileLists = NULL;
	} else {
		if ( fs_savedSearchpaths != fs_searchpaths ) {
			FS_ShutdownSearchPaths( fs_searchpaths );
		}
		if ( fs_savedDirFileLists != fs_dirFileLists ) {
			FS_ShutdownFileLists( fs_dirFileLists );
		}
	}

	fs_searchpaths  = NULL;
	fs_dirFileLists = NULL;
	fs_fakeChkSum   = 0;

	Cmd_RemoveCommand( "path" );
	Cmd_RemoveCommand( "fullpath" );
	Cmd_RemoveCommand( "dir" );
	Cmd_RemoveCommand( "fdir" );
	Cmd_RemoveCommand( "touchFile" );
}

/* ---- FS_Startup  0x0042CDB0 ---- */
void FS_Startup( const char *gameName ) {
	Com_Printf( "----- FS_Startup -----\n" );

	fs_debug = Cvar_Get( "fs_debug", "0", 0 );
	/* 0x0042CDD3/0x0042CDF5: registered second, stored in fs_copyfiles;
	 * FS_FOpenFileRead_Internal reads it for the cd-path to base-path copy. */
	fs_copyfiles = Cvar_Get( "fs_copyfiles", "0", CVAR_INIT );
	fs_cdpath = Cvar_Get( "fs_cdpath", Sys_DefaultCDPath(), CVAR_INIT );
	fs_basepath = Cvar_Get( "fs_basepath", Sys_DefaultBasePath(), CVAR_INIT );
	fs_basegame = Cvar_Get( "fs_basegame", "", CVAR_INIT );
	{
		const char *homePath = Sys_DefaultHomePath();
		if ( !homePath || !homePath[0] ) {
			homePath = fs_basepath->string;
		}
		fs_homepath = Cvar_Get( "fs_homepath", (char *) homePath, CVAR_INIT );
	}
	fs_gamedirvar = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );
	fs_restrict = Cvar_Get( "fs_restrict", "", CVAR_INIT );

	fs_ignoreLocalized = Cvar_Get( "fs_ignoreLozalized", "0", CVAR_CHEAT | CVAR_LATCH );

	if ( fs_cdpath->string[0] ) {
		FS_AddLocalizedGameDirectory( fs_cdpath->string, gameName );
	}
	if ( fs_basepath->string[0] ) {
		FS_AddLocalizedGameDirectory( fs_basepath->string, gameName );
	}
	/* 0x0042CF13-0x0042CF43: tests basepath[0], not homepath[0] */
	if ( fs_basepath->string[0] && Q_stricmp( fs_homepath->string, fs_basepath->string ) ) {
		FS_AddLocalizedGameDirectory( fs_homepath->string, gameName );
	}

	if ( fs_basegame->string[0] && !Q_stricmp( gameName, BASEGAME )
		 && Q_stricmp( fs_basegame->string, gameName ) ) {
		if ( fs_cdpath->string[0] ) {
			FS_AddLocalizedGameDirectory( fs_cdpath->string, fs_basegame->string );
		}
		if ( fs_basepath->string[0] ) {
			FS_AddLocalizedGameDirectory( fs_basepath->string, fs_basegame->string );
		}
		/* 0x0042CFBE-0x0042CFF5: tests homepath[0], not basepath[0] */
		if ( fs_homepath->string[0] && Q_stricmp( fs_homepath->string, fs_basepath->string ) ) {
			FS_AddLocalizedGameDirectory( fs_homepath->string, fs_basegame->string );
		}
	}

	if ( fs_gamedirvar->string[0] && !Q_stricmp( gameName, BASEGAME )
		 && Q_stricmp( fs_gamedirvar->string, gameName ) ) {
		if ( fs_cdpath->string[0] ) {
			FS_AddLocalizedGameDirectory( fs_cdpath->string, fs_gamedirvar->string );
		}
		if ( fs_basepath->string[0] ) {
			FS_AddLocalizedGameDirectory( fs_basepath->string, fs_gamedirvar->string );
		}
		/* 0x0042D071-0x0042D0A8: tests homepath[0], not basepath[0] */
		if ( fs_homepath->string[0] && Q_stricmp( fs_homepath->string, fs_basepath->string ) ) {
			FS_AddLocalizedGameDirectory( fs_homepath->string, fs_gamedirvar->string );
		}
	}

	FS_AddNonPackFileDirectory( "xanim", "" );
	FS_AddNonPackFileDirectory( "xmodel", "" );
	FS_AddNonPackFileDirectory( "xmodelparts", "" );
	FS_AddNonPackFileDirectory( "xmodelsurfs", "" );
	FS_AddNonPackFileDirectory( "weapons", "" );
	FS_AddNonPackFileDirectory( "animtrees", ".atr" );

	Com_ReadCDKey( BASEGAME );
	/* 0x0042D0FD: retail re-fetches fs_game through Cvar_Get and NULL-checks it */
	{
		cvar_t *fs = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );
		if ( fs && fs->string[0] ) {
			Com_AppendCDKey( fs->string );
		}
	}

	FS_AddCommands();

	FS_DisplayPath( qtrue );

	fs_gamedirvar->modified = qfalse;

	Com_Printf( "----------------------\n" );
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}

/* ---- FS_ClearPakReferences  0x0042D170 ---- VERIFIED */
void FS_ClearPakReferences( int flags ) {
	searchpath_t *search;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		if ( !search->pack ) {
			continue;
		}
		( (unsigned char *)&search->pack->referenced )[2] = 0;     /* +0x312 */
		( (unsigned char *)&search->pack->referenced )[1] = 0;     /* +0x311 */

		if ( !flags ) {
			( (unsigned char *)&search->pack->referenced )[0] = 0; /* +0x310 */
			( (unsigned char *)&search->pack->referenced )[3] = 0; /* +0x313 */
		}
	}
}

/* ---- FS_InitFilesystem  0x0042D1C0 ---- VERIFIED */
void FS_InitFilesystem( void ) {
	Com_StartupVariable( "fs_cdpath" );
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_homepath" );
	Com_StartupVariable( "fs_game" );
	Com_StartupVariable( "fs_copyfiles" );
	Com_StartupVariable( "fs_restrict" );
	Com_StartupVariable( "fs_usewolf" );
	Com_StartupVariable( "cl_language" );

	SEH_InitLanguage();

	FS_Startup( BASEGAME );

	Clear__16CStringEdPackageFi( &TheStringPackage, 0 );
	SEH_UpdateLanguageInfo();
	FS_SetRestrictions();

	if ( FS_ReadFile( "default_mp.cfg", NULL ) <= 0 ) {
		Com_Error( ERR_FATAL, "Couldn't load %s.  Make sure Call of Duty is run from "
							  "the correct folder.", "default_mp.cfg" );
	}

	Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}

/* ---- FS_Restart  0x0042D2B0 ---- VERIFIED */
void FS_Restart( int checksumFeed ) {
	FS_Shutdown( qfalse );

	fs_checksumFeed = checksumFeed;
	FS_ClearPakReferences( 0 );

	FS_Startup( BASEGAME );

	Clear__16CStringEdPackageFi( &TheStringPackage, 0 );
	SEH_UpdateLanguageInfo();
	FS_SetRestrictions();

	if ( FS_ReadFile( "default_mp.cfg", NULL ) <= 0 ) {
		if ( lastValidBase[0] ) {
			FS_PureServerSetLoadedPaks( "", "" );
			Cvar_Set2( "fs_basepath", lastValidBase, qtrue );
			Cvar_Set2( "fs_gamedirvar", lastValidGame, qtrue );
			lastValidBase[0] = 0;
			lastValidGame[0] = 0;
			Cvar_Set2( "fs_restrict", "0", qtrue );
			FS_Restart( checksumFeed );
			/* retail 0x0042D375: no return here, falls into the ERR_FATAL call */
			Com_Error( ERR_DROP, "Invalid game folder\n" );
		}
		Com_Error( ERR_FATAL, "Couldn't load %s.  Make sure Call of Duty is run from "
							  "the correct folder.", "default_mp.cfg" );
	}

	if ( Q_stricmp( fs_gamedirvar->string, lastValidGame ) ) {
		if ( !Com_SafeMode() ) {
			Cbuf_AddText( va( "exec %s\n", "config_mp.cfg" ) );
		}
	}

	Q_strncpyz( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	Q_strncpyz( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}

/* ---- FS_ConditionalRestart  0x0042D410 ---- VERIFIED */
qboolean FS_ConditionalRestart( int checksumFeed ) {
	if ( com_sv_running->integer ) {
		return qfalse;
	}
	if ( !fs_gamedirvar->modified && checksumFeed == fs_checksumFeed ) {
		return qfalse;
	}
	FS_Restart( checksumFeed );
	return qtrue;
}

/* ---- FS_FOpenFileByMode  0x0042D450 ---- VERIFIED */
int FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	int r;
	qboolean sync;

	r = 6969;   /* Q3's placeholder; retail 0x0042D45F keeps it for the bad-mode path */
	sync = qfalse;

	switch ( mode ) {
	case FS_READ:
		r = FS_FOpenFileRead( qpath, f, qtrue );
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite( qpath );
		r = 0;
		if ( *f == 0 ) {
			r = -1;
		}
		break;
	case FS_APPEND_SYNC:
		sync = qtrue;
	case FS_APPEND:
		*f = FS_FOpenFileAppend( qpath );
		r = 0;
		if ( *f == 0 ) {
			r = -1;
		}
		break;
	default:
		Com_Error( ERR_FATAL, "\x15" "FSH_FOpenFile: bad mode" );
		break;
	}

	if ( !f ) {
		return r;
	}

	if ( *f ) {
		if ( fsh[*f].zipFile == qtrue ) {
			fsh[*f].baseOffset = unztell( fsh[*f].handleFiles.file.z );
		} else {
			fsh[*f].baseOffset = ftell( fsh[*f].handleFiles.file.o );
		}
		fsh[*f].fileSize = r;
		fsh[*f].streamed = qfalse;
	}

	fsh[*f].handleSync = sync;

	return r;
}

/* ---- FS_FTell  0x0042D580 ---- VERIFIED */
int FS_FTell( fileHandle_t f ) {
	int pos;

	if ( fsh[f].zipFile == qtrue ) {
		pos = unztell( fsh[f].handleFiles.file.z );
	} else {
		pos = ftell( fsh[f].handleFiles.file.o );
	}
	return pos;
}

/* ---- FS_Flush  0x0042D5C0 ---- VERIFIED */
void FS_Flush( fileHandle_t f ) {
	fflush( fsh[f].handleFiles.file.o );
}
