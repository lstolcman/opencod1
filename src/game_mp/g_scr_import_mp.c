// g_scr_import_mp.c -- forwarders for the script system functions the engine
// hands the module in Scr_FarHook
// keep: the function pointer casts on some of the forwarded calls are deliberate

#include "g_local.h"

typedef void ( *scrImportSourceIO_t )( void *buffer, int len );

int Scr_Import_GetBool( unsigned int num ) {
	return Scr_GetBool( num );
}

int Scr_Import_GetInt( unsigned int num ) {
	return Scr_GetInt( num );
}

scr_anim_t Scr_Import_GetAnim( unsigned int num, void *anims ) {
	return Scr_GetAnim( num, anims );
}

void * Scr_Import_GetAnimTree( unsigned int num ) {
	return ((void * ( * )( unsigned int num ))Scr_GetAnimTree)( num );
}

float Scr_Import_GetFloat( unsigned int num ) {
	return Scr_GetFloat( num );
}

const char  * Scr_Import_GetString( unsigned int num ) {
	return Scr_GetString( num );
}

unsigned short Scr_Import_GetConstString( unsigned int num ) {
	return Scr_GetConstString( num );
}

const char  * Scr_Import_GetDebugString( unsigned int num ) {
	return Scr_GetDebugString( num );
}

const char  * Scr_Import_GetIString( unsigned int num ) {
	return Scr_GetIString( num );
}

unsigned short Scr_Import_GetConstIString( unsigned int num ) {
	return ((unsigned short ( * )( unsigned int num ))Scr_GetConstIString)( num );
}

void Scr_Import_GetVector( unsigned int num, vec3_t out ) {
	Scr_GetVector( num, out );
}

int Scr_Import_GetFunc( unsigned int num ) {
	return ((int ( * )( unsigned int num ))Scr_GetFunc)( num );
}

unsigned int Scr_Import_GetEntityNum( unsigned int num, int *classnum ) {
	return Scr_GetEntityNum( num, classnum );
}

int Scr_Import_GetType( unsigned int num ) {
	return Scr_GetType( num );
}

int Scr_Import_GetPointerType( unsigned int num ) {
	return Scr_GetPointerType( num );
}

unsigned int Scr_Import_GetNumParam( void ) {
	return Scr_GetNumParam(  );
}

void Scr_Import_AddBool( int value ) {
	Scr_AddBool( value );
}

void Scr_Import_AddInt( int value ) {
	Scr_AddInt( value );
}

void Scr_Import_AddFloat( float value ) {
	Scr_AddFloat( value );
}

void Scr_Import_AddAnim( scr_anim_t anim ) {
	((void ( * )( scr_anim_t anim ))Scr_AddAnim)( anim );
}

void Scr_Import_AddUndefined( void ) {
	Scr_AddUndefined(  );
}

void Scr_Import_AddEntityNum( int entnum, int classnum ) {
	Scr_AddEntityNum( entnum, classnum );
}

void Scr_Import_AddStruct( void ) {
	Scr_AddStruct(  );
}

void Scr_Import_AddString( const char *string ) {
	Scr_AddString( string );
}

void Scr_Import_AddIString( const char *string ) {
	((void ( * )( const char *string ))Scr_AddIString)( string );
}

void Scr_Import_AddConstString( unsigned short id ) {
	Scr_AddConstString( id );
}

void Scr_Import_AddVector( const float *value ) {
	Scr_AddVector( value );
}

void Scr_Import_AddObject( unsigned short id ) {
	Scr_AddObject( id );
}

void Scr_Import_MakeArray( void ) {
	Scr_MakeArray(  );
}

void Scr_Import_AddArray( void ) {
	Scr_AddArray(  );
}

void Scr_Import_AddArrayStringIndexed( unsigned short id ) {
	Scr_AddArrayStringIndexed( id );
}

void Scr_Import_Error( const char *error ) {
	Scr_Error( error );
}

void Scr_Import_ErrorWithDialogMessage( const char *error, const char *dialog ) {
	((void ( * )( const char *error, const char *dialog ))Scr_ErrorWithDialogMessage)( error, dialog );
}

void Scr_Import_ParamError( unsigned int num, const char *error ) {
	Scr_ParamError( num, error );
}

void Scr_Import_ObjectError( const char *error ) {
	Scr_ObjectError( error );
}

void Scr_Import_SetDynamicEntityField( int entnum, int classnum, unsigned short fieldnum ) {
	Scr_SetDynamicEntityField( entnum, classnum, fieldnum );
}

void Scr_Import_FreeEntityNum( int entnum, int classnum ) {
	Scr_FreeEntityNum( entnum, classnum );
}

unsigned short Scr_Import_GetEntityId( int entnum, int classnum ) {
	return Scr_GetEntityId( entnum, classnum );
}

void Scr_Import_SetClassMap( scr_classStruct_t *classMap, unsigned int count ) {
	Scr_SetClassMap( classMap, count );
}

void Scr_Import_RemoveClassMap( void ) {
	Scr_RemoveClassMap(  );
}

void Scr_Import_AddClassField( unsigned short classnum, const char *name, unsigned short fieldnum ) {
	Scr_AddClassField( classnum, name, fieldnum );
}

void Scr_Import_AddFields( const char *path, const char *extension ) {
	Scr_AddFields( path, extension );
}

unsigned short Scr_Import_FindField( const char *name, int *type ) {
	return Scr_FindField( name, type );
}

int Scr_Import_GetOffset( unsigned short classnum, const char *name ) {
	return Scr_GetOffset( classnum, name );
}

void Scr_Import_CopyEntityNum( int sourceEntnum, int destEntnum, int classnum ) {
	Scr_CopyEntityNum( sourceEntnum, destEntnum, classnum );
}

void Scr_Import_Init( int developer, int developerScript ) {
	((void ( * )( int developer, int developerScript ))Scr_Init)( developer, developerScript );
}

void Scr_Import_Shutdown( void ) {
	Scr_Shutdown(  );
}

void Scr_Import_Abort( void ) {
	Scr_Abort(  );
}

void Scr_Import_SetLoading( int loading ) {
	Scr_SetLoading( loading );
}

void Scr_Import_InitSystem( void ) {
	Scr_InitSystem(  );
}

void Scr_Import_AllocGameVariable( int classnum, int time ) {
	Scr_AllocGameVariable( classnum, time );
}

void Scr_Import_GetChecksum( unsigned int *out ) {
	((void ( * )( unsigned int *out ))Scr_GetChecksum)( out );
}

int Scr_Import_HasSourceFiles( void ) {
	return ((int ( * )( void ))Scr_HasSourceFiles)(  );
}

void Scr_Import_SaveSource( scrImportSourceIO_t write ) {
	((void ( * )( scrImportSourceIO_t write ))Scr_SaveSource)( write );
}

void Scr_Import_LoadSource( scrImportSourceIO_t read ) {
	((void ( * )( scrImportSourceIO_t read ))Scr_LoadSource)( read );
}

void Scr_Import_SkipSource( scrImportSourceIO_t read ) {
	((void ( * )( scrImportSourceIO_t read ))Scr_SkipSource)( read );
}

void Scr_Import_SavePre( void *context ) {
	((void ( * )( void *context ))Scr_SavePre)( context );
}

void Scr_Import_SavePost( void *context ) {
	((void ( * )( void *context ))Scr_SavePost)( context );
}

void Scr_Import_SaveShutdown( void ) {
	Scr_SaveShutdown(  );
}

void Scr_Import_LoadPre( void *context, void *allocator ) {
	((void ( * )( void *context, void *allocator ))Scr_LoadPre)( context, allocator );
}

void Scr_Import_LoadShutdown( void ) {
	Scr_LoadShutdown(  );
}

int Scr_Import_LoadScript( const char *scriptName ) {
	return Scr_LoadScript( scriptName );
}

void        * Scr_Import_FindAnimTree( const char *treeName ) {
	return Scr_FindAnimTree( treeName );
}

void Scr_Import_FindAnim( const char *treeName, const char *animName, scr_anim_t *anim ) {
	Scr_FindAnim( treeName, animName, anim );
}

int Scr_Import_GetFunctionHandle( const char *scriptName, const char *label ) {
	return Scr_GetFunctionHandle( scriptName, label );
}

void Scr_Import_BeginLoadScripts( void ) {
	Scr_BeginLoadScripts(  );
}

void Scr_Import_BeginLoadAnimTrees( void ) {
	Scr_BeginLoadAnimTrees(  );
}

void Scr_Import_EndLoadScripts( void ) {
	Scr_EndLoadScripts(  );
}

void Scr_Import_EndLoadAnimTrees( void ) {
	Scr_EndLoadAnimTrees(  );
}

void Scr_Import_PrecacheAnimTrees( void *( *allocFn )( int size ) ) {
	Scr_PrecacheAnimTrees( allocFn );
}

void Scr_Import_FreeScripts( int bComplete ) {
	Scr_FreeScripts( bComplete );
}

void Scr_Import_FreeGameVariable( int bComplete ) {
	Scr_FreeGameVariable( bComplete );
}

void Scr_Import_ShutdownSystem( int bComplete ) {
	Scr_ShutdownSystem( bComplete );
}

int Scr_Import_IsSystemActive( int bComplete ) {
	return Scr_IsSystemActive( bComplete );
}

unsigned short Scr_Import_ExecThread( unsigned int handle, unsigned int paramcount ) {
	return Scr_ExecThread( handle, paramcount );
}

unsigned short Scr_Import_ExecEntThreadNum( int entnum, int classnum, int handle, unsigned int paramcount ) {
	return Scr_ExecEntThreadNum( entnum, classnum, handle, paramcount );
}

void Scr_Import_AddExecThread( int handle, unsigned int paramcount ) {
	((void ( * )( int handle, unsigned int paramcount ))Scr_AddExecThread)( handle, paramcount );
}

void Scr_Import_AddExecEntThreadNum( int entnum, int classnum, int handle, unsigned int paramcount ) {
	Scr_AddExecEntThreadNum( entnum, classnum, handle, paramcount );
}

int Scr_Import_IsThreadAlive( unsigned short thread ) {
	return ((int ( * )( unsigned short thread ))Scr_IsThreadAlive)( thread );
}

void Scr_Import_FreeThread( unsigned short thread ) {
	Scr_FreeThread( thread );
}

short Scr_Import_ConvertThreadToSave( unsigned short thread ) {
	return ((short ( * )( unsigned short thread ))Scr_ConvertThreadToSave)( thread );
}

unsigned short Scr_Import_ConvertThreadFromLoad( unsigned int handle ) {
	return ((unsigned short ( * )( unsigned int handle ))Scr_ConvertThreadFromLoad)( handle );
}

void Scr_Import_SetString( unsigned short *dest, unsigned short id ) {
	Scr_SetString( dest, id );
}

unsigned short Scr_Import_AllocString( const char *string, unsigned int user ) {
	return Scr_AllocString( string, user );
}

void Scr_Import_SetTime( int time ) {
	Scr_SetTime( time );
}

void Scr_Import_RunCurrentThreads( void ) {
	Scr_RunCurrentThreads(  );
}

void Scr_Import_ResetTimeout( void ) {
	Scr_ResetTimeout(  );
}

void Scr_Import_NotifyNum( int entnum, int classnum, unsigned short event, unsigned int paramcount ) {
	Scr_NotifyNum( entnum, classnum, event, paramcount );
}

const char  * SL_ConvertToString( unsigned short id ) {
	return SL_ConvertToStringImport( id );
}

unsigned short Scr_Import_SL_GetString( const char *string, unsigned int user ) {
	return SL_GetString( string, user );
}

unsigned short Scr_Import_SL_GetLowercaseString( const char *string, unsigned int user ) {
	return SL_GetLowercaseString( string, user );
}

unsigned short Scr_Import_SL_FindLowercaseString( const char *string ) {
	return SL_FindLowercaseString( string );
}

void Scr_Import_NotifyId( unsigned short id, unsigned short event, unsigned int paramcount ) {
	((void ( * )( unsigned short id, unsigned short event, unsigned int paramcount ))Scr_NotifyId)( id, event, paramcount );
}

unsigned short Scr_Import_CreateCanonicalFilename( const char *filename ) {
	return ((unsigned short ( * )( const char *filename ))Scr_CreateCanonicalFilename)( filename );
}

int Scr_Import_GetAnimsIndex( int anims ) {
	return Scr_GetAnimsIndex( anims );
}

int Scr_Import_GetAnims( int index ) {
	return ((int ( * )( int index ))Scr_GetAnims)( index );
}

void        * Scr_Import_MT_Alloc( int size, int type ) {
	return MT_Alloc( size, type );
}

void Scr_Import_MT_Free( void *p, int size ) {
	MT_Free( p, size );
}

/*
=============
Scr_FarHook

Copies the engine's block of script entry points over the import pointers
and returns the entry points the script VM calls into the module with.
=============
*/

// 102 pointers, Scr_GetBool through MT_Free
#define SCR_IMPORT_BLOCK_SIZE       0x198

typedef enum scrExport_e {
	SCR_EXP_GET_FUNCTION        = 0,
	SCR_EXP_GET_METHOD          = 1,
	SCR_EXP_SET_OBJECT_FIELD    = 2,
	SCR_EXP_GET_OBJECT_FIELD    = 3,
	SCR_EXP_LOAD_READ           = 4,

	SCR_EXP_COUNT               = 5
} scrExport_t;

#ifdef CGAMEDLL
void        *Scr_GetFunction( const char **pName, int *pDeveloper );
void        *Scr_GetMethod( const char **pName, int *pDeveloper );
void        Scr_SetObjectField( int classnum, int objectNum, int fieldnum );
void        Scr_GetObjectField( int classnum, int objectNum, int fieldnum );
int         Scr_LoadRead( int len );
extern void ( *scr_cgameExports[SCR_EXP_COUNT] )( void );

// cg_main_mp.c declares this as int *Scr_FarHook( const void * )
void *Scr_FarHook( void *hooks ) {
	if ( hooks ) {
		memcpy( &Scr_GetBool, hooks, SCR_IMPORT_BLOCK_SIZE );
	}

	scr_cgameExports[SCR_EXP_GET_FUNCTION] = (void ( * )( void ))Scr_GetFunction;
	scr_cgameExports[SCR_EXP_GET_METHOD] = (void ( * )( void ))Scr_GetMethod;
	scr_cgameExports[SCR_EXP_SET_OBJECT_FIELD] = (void ( * )( void ))Scr_SetObjectField;
	scr_cgameExports[SCR_EXP_GET_OBJECT_FIELD] = (void ( * )( void ))Scr_GetObjectField;
	scr_cgameExports[SCR_EXP_LOAD_READ] = (void ( * )( void ))Scr_LoadRead;

	return scr_cgameExports;
}
#else
typedef void ( *xfunction_t )( void );
typedef void ( *xmethod_t )( unsigned int entnum );
xfunction_t Scr_GetFunction( const char **pName, int *pDeveloper );
xmethod_t   Scr_GetMethod( const char **pName, int *pDeveloper );
extern void ( *scr_gameExports[SCR_EXP_COUNT] )( void );

void *Scr_FarHook( void *scrImports ) {
	if ( scrImports ) {
		memcpy( &Scr_GetBool, scrImports, SCR_IMPORT_BLOCK_SIZE );
	}

	scr_gameExports[SCR_EXP_GET_FUNCTION] = (void ( * )( void ))Scr_GetFunction;
	scr_gameExports[SCR_EXP_GET_METHOD] = (void ( * )( void ))Scr_GetMethod;
	scr_gameExports[SCR_EXP_SET_OBJECT_FIELD] = (void ( * )( void ))Scr_SetObjectField;
	scr_gameExports[SCR_EXP_GET_OBJECT_FIELD] = (void ( * )( void ))Scr_GetObjectField;
	scr_gameExports[SCR_EXP_LOAD_READ] = (void ( * )( void ))Scr_LoadRead;

	return scr_gameExports;
}
#endif
