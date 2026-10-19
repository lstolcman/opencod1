// cg_scr_main.c -- the client's half of the script system
// the client runs no scripts of its own: this is only the block of script
// functions the engine hands over, used to load the animation trees

#include <string.h>

#include "cg_local.h"

/*
==============================================================================

SCRIPT IMPORTS

Filled in one block copy from the engine, in this order

==============================================================================
*/

// the module callbacks handed back to the engine
void ( *scr_cgameExports[5] )( void );

// keep: the 102 slots must stay one contiguous run in this order, since the
// engine fills them with a single copy; each gets its own ordered bss section
#pragma bss_seg( ".bss$scri000" )
int         ( *Scr_GetBool )( unsigned int num );
#pragma bss_seg( ".bss$scri001" )
int         ( *Scr_GetInt )( unsigned int num );
#pragma bss_seg( ".bss$scri002" )
scr_anim_t  ( *Scr_GetAnim )( unsigned int num, void *anims );
#pragma bss_seg( ".bss$scri003" )
void        ( *Scr_GetAnimTree )();
#pragma bss_seg( ".bss$scri004" )
float       ( *Scr_GetFloat )( unsigned int num );
#pragma bss_seg( ".bss$scri005" )
const char  *( *Scr_GetString )( unsigned int num );
#pragma bss_seg( ".bss$scri006" )
unsigned short ( *Scr_GetConstString )( unsigned int num );
#pragma bss_seg( ".bss$scri007" )
const char  *( *Scr_GetDebugString )( unsigned int num );
#pragma bss_seg( ".bss$scri008" )
const char  *( *Scr_GetIString )( unsigned int num );
#pragma bss_seg( ".bss$scri009" )
void        ( *Scr_GetConstIString )();
#pragma bss_seg( ".bss$scri010" )
void        ( *Scr_GetVector )( unsigned int num, vec3_t out );
#pragma bss_seg( ".bss$scri011" )
void        ( *Scr_GetFunc )();
#pragma bss_seg( ".bss$scri012" )
int         ( *Scr_GetType )( unsigned int num );
#pragma bss_seg( ".bss$scri013" )
int         ( *Scr_GetPointerType )( unsigned int num );
#pragma bss_seg( ".bss$scri014" )
unsigned int ( *Scr_GetEntityNum )( unsigned int num, int *classnum );
#pragma bss_seg( ".bss$scri015" )
unsigned int ( *Scr_GetNumParam )( void );
#pragma bss_seg( ".bss$scri016" )
void        ( *Scr_AddBool )( int value );
#pragma bss_seg( ".bss$scri017" )
void        ( *Scr_AddInt )( int value );
#pragma bss_seg( ".bss$scri018" )
void        ( *Scr_AddFloat )( float value );
#pragma bss_seg( ".bss$scri019" )
void        ( *Scr_AddAnim )();
#pragma bss_seg( ".bss$scri020" )
void        ( *Scr_AddUndefined )( void );
#pragma bss_seg( ".bss$scri021" )
void        ( *Scr_AddEntityNum )( int entnum, int classnum );
#pragma bss_seg( ".bss$scri022" )
void        ( *Scr_AddStruct )( void );
#pragma bss_seg( ".bss$scri023" )
void        ( *Scr_AddString )( const char *string );
#pragma bss_seg( ".bss$scri024" )
void        ( *Scr_AddIString )();
#pragma bss_seg( ".bss$scri025" )
void        ( *Scr_AddConstString )( unsigned short id );
#pragma bss_seg( ".bss$scri026" )
void        ( *Scr_AddVector )( const float *value );
#pragma bss_seg( ".bss$scri027" )
void        ( *Scr_AddObject )( unsigned short id );
#pragma bss_seg( ".bss$scri028" )
void        ( *Scr_AddArray )( void );
#pragma bss_seg( ".bss$scri029" )
void        ( *Scr_AddArrayStringIndexed )( unsigned short id );
#pragma bss_seg( ".bss$scri030" )
void        ( *Scr_MakeArray )( void );
#pragma bss_seg( ".bss$scri031" )
void        ( *Scr_BeginLoadScripts )( int type );
#pragma bss_seg( ".bss$scri032" )
void        ( *Scr_BeginLoadAnimTrees )( void );
#pragma bss_seg( ".bss$scri033" )
void        ( *Scr_EndLoadScripts )( void );
#pragma bss_seg( ".bss$scri034" )
void        ( *Scr_EndLoadAnimTrees )( void );
#pragma bss_seg( ".bss$scri035" )
void        ( *Scr_PrecacheAnimTrees )( void *( *allocFn )( int size ) );
#pragma bss_seg( ".bss$scri036" )
void        ( *Scr_FreeScripts )( int bComplete );
#pragma bss_seg( ".bss$scri037" )
void        ( *Scr_FreeGameVariable )( int bComplete );
#pragma bss_seg( ".bss$scri038" )
void        ( *Scr_ShutdownSystem )( int bComplete );
#pragma bss_seg( ".bss$scri039" )
int         ( *Scr_IsSystemActive )( int bComplete );
#pragma bss_seg( ".bss$scri040" )
void        ( *Scr_AddExecThread )();
#pragma bss_seg( ".bss$scri041" )
void        ( *Scr_AddExecEntThreadNum )( int entnum, int classnum, int handle, unsigned int paramcount );
#pragma bss_seg( ".bss$scri042" )
int         ( *Scr_ExecThread )( unsigned int handle, unsigned int paramcount );
#pragma bss_seg( ".bss$scri043" )
unsigned short ( *Scr_ExecEntThreadNum )( int entnum, int classnum, int handle, unsigned int paramcount );
#pragma bss_seg( ".bss$scri044" )
void        ( *Scr_IsThreadAlive )();
#pragma bss_seg( ".bss$scri045" )
void        ( *Scr_Error )( const char *error );
#pragma bss_seg( ".bss$scri046" )
void        ( *Scr_ErrorWithDialogMessage )();
#pragma bss_seg( ".bss$scri047" )
void        ( *Scr_ParamError )( unsigned int num, const char *error );
#pragma bss_seg( ".bss$scri048" )
void        ( *Scr_ObjectError )( const char *error );
#pragma bss_seg( ".bss$scri049" )
void        ( *Scr_SetDynamicEntityField )( int entnum, int classnum, int fieldnum );
#pragma bss_seg( ".bss$scri050" )
void        ( *Scr_FreeEntityNum )( int entnum, int classnum );
#pragma bss_seg( ".bss$scri051" )
unsigned short ( *Scr_GetEntityId )( int entnum, int classnum );
#pragma bss_seg( ".bss$scri052" )
void        ( *Scr_SetClassMap )( void *classMap, unsigned int count );
#pragma bss_seg( ".bss$scri053" )
void        ( *Scr_RemoveClassMap )( void );
#pragma bss_seg( ".bss$scri054" )
void        ( *Scr_Unused36 )();
#pragma bss_seg( ".bss$scri055" )
void        ( *Scr_Unused37 )();
#pragma bss_seg( ".bss$scri056" )
void        ( *Scr_AddClassField )( unsigned short classnum, const char *name, unsigned short fieldnum );
#pragma bss_seg( ".bss$scri057" )
void        ( *Scr_AddFields )( const char *path, const char *extension );
#pragma bss_seg( ".bss$scri058" )
unsigned short ( *Scr_FindField )( const char *name, int *type );
#pragma bss_seg( ".bss$scri059" )
int         ( *Scr_GetOffset )( unsigned short classnum, const char *name );
#pragma bss_seg( ".bss$scri060" )
void        ( *Scr_CopyEntityNum )( int sourceEntnum, int destEntnum, int classnum );
#pragma bss_seg( ".bss$scri061" )
void        ( *Scr_Init )();
#pragma bss_seg( ".bss$scri062" )
void        ( *Scr_Shutdown )();
#pragma bss_seg( ".bss$scri063" )
void        ( *Scr_Abort )();
#pragma bss_seg( ".bss$scri064" )
void        ( *Scr_SetLoading )( int loading );
#pragma bss_seg( ".bss$scri065" )
void        ( *Scr_AllocGameVariable )( int classnum, int time );
#pragma bss_seg( ".bss$scri066" )
void        ( *Scr_InitSystem )( void );
#pragma bss_seg( ".bss$scri067" )
void        ( *Scr_GetChecksum )();
#pragma bss_seg( ".bss$scri068" )
void        ( *Scr_HasSourceFiles )();
#pragma bss_seg( ".bss$scri069" )
void        ( *Scr_SaveSource )();
#pragma bss_seg( ".bss$scri070" )
void        ( *Scr_LoadSource )();
#pragma bss_seg( ".bss$scri071" )
void        ( *Scr_SkipSource )();
#pragma bss_seg( ".bss$scri072" )
void        ( *Scr_SavePre )();
#pragma bss_seg( ".bss$scri073" )
void        ( *Scr_SavePost )();
#pragma bss_seg( ".bss$scri074" )
void        ( *Scr_SaveShutdown )();
#pragma bss_seg( ".bss$scri075" )
void        ( *Scr_Unused4B )();
#pragma bss_seg( ".bss$scri076" )
void        ( *Scr_LoadPre )();
#pragma bss_seg( ".bss$scri077" )
void        ( *Scr_LoadShutdown )();
#pragma bss_seg( ".bss$scri078" )
void        ( *Scr_Unused4E )();
#pragma bss_seg( ".bss$scri079" )
int         ( *Scr_LoadScript )( const char *scriptName );
#pragma bss_seg( ".bss$scri080" )
void        *( *Scr_FindAnimTree )( const char *treeName );
#pragma bss_seg( ".bss$scri081" )
void        ( *Scr_FindAnim )( const char *treeName, const char *animName, scr_anim_t *anim );
#pragma bss_seg( ".bss$scri082" )
int         ( *Scr_GetFunctionHandle )( const char *scriptName, const char *label );
#pragma bss_seg( ".bss$scri083" )
void        ( *Scr_FreeThread )( int threadId );
#pragma bss_seg( ".bss$scri084" )
void        ( *Scr_ConvertThreadToSave )();
#pragma bss_seg( ".bss$scri085" )
void        ( *Scr_ConvertThreadFromLoad )();
#pragma bss_seg( ".bss$scri086" )
void        ( *Scr_SetString )( unsigned short *dest, unsigned short id );
#pragma bss_seg( ".bss$scri087" )
unsigned short ( *Scr_AllocString )( const char *string, unsigned int user );
#pragma bss_seg( ".bss$scri088" )
void        ( *Scr_NotifyNum )( int entnum, int classnum, unsigned short event, unsigned int paramcount );
#pragma bss_seg( ".bss$scri089" )
void        ( *Scr_NotifyId )();
#pragma bss_seg( ".bss$scri090" )
const char  *( *SL_ConvertToStringImport )( unsigned short id );
#pragma bss_seg( ".bss$scri091" )
unsigned short ( *SL_GetString )( const char *string, unsigned int user );
#pragma bss_seg( ".bss$scri092" )
unsigned short ( *SL_GetLowercaseString )( const char *string, unsigned int user );
#pragma bss_seg( ".bss$scri093" )
unsigned short ( *SL_FindLowercaseString )( const char *string );
#pragma bss_seg( ".bss$scri094" )
void        ( *Scr_CreateCanonicalFilename )();
#pragma bss_seg( ".bss$scri095" )
void        ( *Scr_SetTime )( int time );
#pragma bss_seg( ".bss$scri096" )
void        ( *Scr_RunCurrentThreads )( void );
#pragma bss_seg( ".bss$scri097" )
void        ( *Scr_ResetTimeout )( void );
#pragma bss_seg( ".bss$scri098" )
int         ( *Scr_GetAnimsIndex )( int anims );
#pragma bss_seg( ".bss$scri099" )
void        ( *Scr_GetAnims )();
#pragma bss_seg( ".bss$scri100" )
void        *( *MT_Alloc )( int size, int type );
#pragma bss_seg( ".bss$scri101" )
void        ( *MT_Free )( void *p, int size );
#pragma bss_seg()
