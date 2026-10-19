/*
 * @fidelity: verified
 */

#include "../qcommon/qcommon.h"
#include "cl_records.h"
#include "cl_local.h"
extern int s_hunkTotal, s_hunkHighTemp, hunk_low_temp;
#include "../botlib/l_script.h"
#include "cl_refapi.h"
#include "cl_vm.h"
#include "../universal/com_sndalias.h"

/* cl.snap.valid (0x01432964), the head of the cl.snap record (storage in cl_main_mp.c) */
extern int cl_snap_valid;

vm_t    *uivm = NULL;               /* retail 0x0161747C */

#define UI_GETAPIVERSION        0   /* 0x00418F4C */
#define UI_INIT                 1   /* 0x00418F7C */
#define UI_SHUTDOWN             2   /* 0x00418E4B */
#define UI_KEY_EVENT            3   /* 0x0040DEA8, 0x0040DF48, 0x0040E02D */
#define UI_MOUSE_EVENT          4   /* 0x0040B0BE */
#define UI_REFRESH              5
#define UI_IS_FULLSCREEN        6
#define UI_SET_ACTIVE_MENU      7   /* 0x0040DE91 */
#define UI_GET_ACTIVE_MENU      8   /* 0x0040E012 */
#define UI_API_VERSION          7   /* 0x00418F56 -- UO is 9 */

extern int CIN_DrawCinematic();
extern int CIN_PlayCinematic();
extern int CIN_RunCinematic( int handle );
extern void CIN_SetExtents( int handle, int x, int y, int w, int h );
extern int CIN_StopCinematic();
extern int CL_GetAutoUpdate();
extern int CL_GetPing();
extern int CL_GetPingQueueCount();
extern int CL_ServerStatus();
extern int Hunk_MemoryRemaining( void );
extern int CL_UpdateVisiblePings_f( int source );
extern int *Com_FindSoundAlias( const char *name, int source );
void Com_LoadSoundAliases( const char *name, int source );  /* 0x00433E90 */
extern int Com_RealTime();
void Com_UnloadSoundAliasSounds( int source );   /* 0x00433E20 */
extern int FS_Delete();
extern int Key_ClearStates();
extern int Key_IsDown();
char *Key_KeynumToString( int keynum, qboolean translate );   /* 0x0040D5A0 */
extern int Key_SetBinding();
extern int MSS_FadeAllSounds();
extern int MSS_PlayLocalSoundAlias();
extern const char *NET_AdrToString( netadr_t a );
extern qboolean NET_CompareAdr( netadr_t a, netadr_t b );
extern qboolean NET_StringToAdr( const char *s, netadr_t *a );
extern void SCR_UpdateScreen( void );
extern int SEH_GetLanguageName();
extern int SEH_StringEd_GetString();
extern int SEH_VerifyLanguageSelection();
/* retail 0x00463300 returns a Z_Malloc'd char *, NULL when the clipboard has no CF_TEXT. */
char *Sys_GetClipboardData( void );     /* 0x00463300 */

/* ------------------------------------------------------------------------- *
 *  File-local typed views over the client's server-browser storage.
 *
 *  These mirror the file-local structs cl_main_mp.c uses for the same blobs
 *  (that TU is the storage of record); keeping identical copies here lets the
 *  LAN traps below read server->ping instead of *(int *)(base + 184*i + 168).
 *  Every offset is pinned so a wrong layout is a build error.  Promotion of
 *  serverInfo_t / ping_t to cl_local.h is deferred until the file-local copies
 *  in both TUs are removed together.
 * ------------------------------------------------------------------------- */

#define AS_LOCAL            0
#define AS_GLOBAL           1
#define AS_FAVORITES        2

#define MAX_OTHER_SERVERS   128
#define MAX_GLOBAL_SERVERS  2048
#define MAX_PINGREQUESTS    16

/* trap_GetClientState's target (RTCW's record); GetClientState fills it. */
typedef struct {
	int  connState;                      /* +0x0000 */
	int  connectPacketCount;             /* +0x0004 */
	int  clientNum;                      /* +0x0008 */
	char servername[MAX_STRING_CHARS];       /* +0x000c */
	char updateInfoString[MAX_STRING_CHARS]; /* +0x040c */
	char messageString[MAX_STRING_CHARS];    /* +0x080c */
} uiClientState_t;
COD1_ASSERT_SIZE( uiClientState_t, 3084 );
ASSERT_OFF( uiClientState_t, clientNum, 0x08 );
ASSERT_OFF( uiClientState_t, servername, 0x0c );
ASSERT_OFF( uiClientState_t, updateInfoString, 0x40c );
ASSERT_OFF( uiClientState_t, messageString, 0x80c );

/* one browsed server (cls.localServers / globalServers / favoriteServers). */
typedef struct {
	netadr_t adr;                        /* +0x00 */
	char     hostName[MAX_NAME_LENGTH];  /* +0x14 */
	char     mapName[MAX_NAME_LENGTH];   /* +0x34 */
	char     game[MAX_NAME_LENGTH];      /* +0x54 */
	int      netType;                    /* +0x74 */
	char     gameType[MAX_NAME_LENGTH];  /* +0x78  a string in CoD, not an int */
	int      clients;                    /* +0x98 */
	int      maxClients;                 /* +0x9c */
	int      minPing;                    /* +0xa0 */
	int      maxPing;                    /* +0xa4 */
	int      ping;                       /* +0xa8 */
	int      visible;                    /* +0xac */
	int      allowAnonymous;             /* +0xb0 */
	int      password;                   /* +0xb4  CoD addition */
} serverInfo_t;
COD1_ASSERT_SIZE( serverInfo_t, 184 );
ASSERT_OFF( serverInfo_t, hostName, 0x14 );
ASSERT_OFF( serverInfo_t, gameType, 0x78 );
ASSERT_OFF( serverInfo_t, ping, 0xa8 );
ASSERT_OFF( serverInfo_t, visible, 0xac );
ASSERT_OFF( serverInfo_t, password, 0xb4 );

/* one outstanding ping request (cl.pinglist[MAX_PINGREQUESTS]); a slot is free
 * when adr.port == 0. */
typedef struct {
	netadr_t adr;                        /* +0x00  port @ +0x12 */
	int      start;                      /* +0x14 */
	int      time;                       /* +0x18 */
	char     info[MAX_INFO_STRING];      /* +0x1c */
} ping_t;
COD1_ASSERT_SIZE( ping_t, 1052 );
ASSERT_OFF( ping_t, info, 0x1c );

#define cls_localServers    ( (serverInfo_t *)cls_localServersStorage )   /* 0x0155F404 */
#define cls_globalServers   ( (serverInfo_t *)&cls_globalServersStorage )  /* 0x01565008 */
#define cls_favoriteServers ( (serverInfo_t *)cls_favoriteServersStorage )   /* 0x015C4010 */
#define cl_pinglist         ( (ping_t *)cl_pinglistStorage )         /* 0x015CA660 */

/* ---- GetClientState  0x00417410 ----  VERIFIED */
void __cdecl GetClientState( uiClientState_t *state )
{
  state->connectPacketCount = clc_connectPacketCount;
  state->connState          = cls_state;
  Q_strncpyz( state->servername, cls_servername, sizeof( state->servername ) );
  Q_strncpyz( state->updateInfoString, cls_updateInfoString, sizeof( state->updateInfoString ) );
  Q_strncpyz( state->messageString, clc_serverMessage, sizeof( state->messageString ) );
  state->clientNum = cl_snap_ps_clientNum;   /* cl.snap.ps.clientNum @ 0x01432A2C */
}

/* servercache.dat carries a byte-count guard: sizeof globalServers (2048*184)
 * + favoriteServers (128*184) + globalServerAddresses (2048*6). */
#define SERVERCACHE_GLOBAL_BYTES        ( MAX_GLOBAL_SERVERS * (int)sizeof( serverInfo_t ) )   /* 376832 */
#define SERVERCACHE_FAVORITE_BYTES      ( MAX_OTHER_SERVERS * (int)sizeof( serverInfo_t ) )    /* 23552 */
#define SERVERCACHE_ADDRESS_BYTES       12288                                                  /* 2048 * 6 */
#define SERVERCACHE_CHECKSUM \
	( SERVERCACHE_GLOBAL_BYTES + SERVERCACHE_FAVORITE_BYTES + SERVERCACHE_ADDRESS_BYTES )   /* 412672 */

/* ---- LAN_LoadCachedServers  0x00417490 ----  VERIFIED */
void LAN_LoadCachedServers( void )
{
  fileHandle_t f;
  int          checksum;

  cls_numglobalservers = 0;
  cls_numfavoriteservers = 0;
  cls_numGlobalServerAddresses = 0;

  if ( FS_SV_FOpenFileRead( "servercache.dat", &f ) )
  {
    FS_Read( &cls_numglobalservers, 4, f );
    FS_Read( &cls_numfavoriteservers, 4, f );
    FS_Read( &cls_numGlobalServerAddresses, 4, f );
    FS_Read( &checksum, 4, f );
    if ( checksum == SERVERCACHE_CHECKSUM )
    {
      FS_Read( cls_globalServers, SERVERCACHE_GLOBAL_BYTES, f );
      FS_Read( cls_favoriteServers, SERVERCACHE_FAVORITE_BYTES, f );
      FS_Read( &cls_globalServerAddressesStorage, SERVERCACHE_ADDRESS_BYTES, f );   /* cls.globalServerAddresses */
    }
    else
    {
      cls_numglobalservers = 0;
      cls_numfavoriteservers = 0;
      cls_numGlobalServerAddresses = 0;
    }
    FS_FCloseFile( f );
  }
}

/* ---- LAN_SaveServersToCache  0x00417570 ----  VERIFIED */
void LAN_SaveServersToCache( void )
{
  fileHandle_t f;
  int          checksum;

  f = FS_SV_FOpenFileWrite( "servercache.dat" );
  FS_Write( &cls_numglobalservers, 4, f );
  FS_Write( &cls_numfavoriteservers, 4, f );
  FS_Write( &cls_numGlobalServerAddresses, 4, f );
  checksum = SERVERCACHE_CHECKSUM;
  FS_Write( &checksum, 4, f );
  FS_Write( cls_globalServers, SERVERCACHE_GLOBAL_BYTES, f );
  FS_Write( cls_favoriteServers, SERVERCACHE_FAVORITE_BYTES, f );
  FS_Write( &cls_globalServerAddressesStorage, SERVERCACHE_ADDRESS_BYTES, f );   /* cls.globalServerAddresses */
  FS_FCloseFile( f );
}

/* ---- LAN_ResetPings  0x00417600 ----  VERIFIED */
void __cdecl LAN_ResetPings( int source )
{
  serverInfo_t *servers;
  int           count;
  int           i;

  switch ( source )
  {
    case AS_LOCAL:
      servers = cls_localServers;
      count   = MAX_OTHER_SERVERS;
      break;
    case AS_GLOBAL:
      servers = cls_globalServers;
      count   = MAX_GLOBAL_SERVERS;
      break;
    case AS_FAVORITES:
      servers = cls_favoriteServers;
      count   = MAX_OTHER_SERVERS;
      break;
    default:
      return;
  }

  for ( i = 0; i < count; i++ )
    servers[i].ping = -1;
}

/* ---- LAN_AddServer  0x00417640 ----  VERIFIED */
int __cdecl LAN_AddServer( int source, char *name, char *address )
{
  int           max, *count, i;
  netadr_t      adr;
  serverInfo_t *servers;

  servers = NULL;
  max     = MAX_OTHER_SERVERS;
  count   = NULL;
  switch ( source )
  {
    case AS_LOCAL:
      count   = &cls_numlocalservers;
      servers = cls_localServers;
      break;
    case AS_GLOBAL:
      max     = MAX_GLOBAL_SERVERS;
      count   = &cls_numglobalservers;
      servers = cls_globalServers;
      break;
    case AS_FAVORITES:
      count   = &cls_numfavoriteservers;
      servers = cls_favoriteServers;
      break;
  }

  if ( servers && *count < max )
  {
    if ( !NET_StringToAdr( address, &adr ) )
      return -2;

    for ( i = 0; i < *count; i++ )
    {
      if ( NET_CompareAdr( servers[i].adr, adr ) )
        break;
    }
    if ( i >= *count )
    {
      servers[*count].adr = adr;
      Q_strncpyz( servers[*count].hostName, name, sizeof( servers[*count].hostName ) );
      servers[*count].visible = qtrue;
      ( *count )++;
      return 1;
    }
    return 0;   /* already in the list */
  }
  return -1;
}

/* ---- LAN_RemoveServer  0x004177D0 ----  VERIFIED */
void __cdecl LAN_RemoveServer( int source, const char *addr )
{
  serverInfo_t *servers;
  int          *count;
  netadr_t      comp;
  int           i, j;

  switch ( source )
  {
    case AS_LOCAL:
      count   = &cls_numlocalservers;
      servers = cls_localServers;
      break;
    case AS_GLOBAL:
      count   = &cls_numglobalservers;
      servers = cls_globalServers;
      break;
    case AS_FAVORITES:
      count   = &cls_numfavoriteservers;
      servers = cls_favoriteServers;
      break;
    default:
      return;
  }

  NET_StringToAdr( addr, &comp );

  for ( i = 0; i < *count; i++ )
  {
    if ( NET_CompareAdr( comp, servers[i].adr ) )
    {
      for ( j = i; j < *count - 1; j++ )
        Com_Memcpy( &servers[j], &servers[j + 1], sizeof( serverInfo_t ) );   /* retail 0x4178CD */
      ( *count )--;
      return;
    }
  }
}

/* ---- LAN_GetServerCount  0x00417900 ---- */
static int LAN_GetServerCount( int source )
{
  switch ( source )
  {
    case AS_LOCAL:
      return cls_numlocalservers;
    case AS_GLOBAL:
      return cls_numglobalservers;
    case AS_FAVORITES:
      return cls_numfavoriteservers;
  }
  return 0;
}

/* ---- LAN_GetServerAddressString  0x00417920 ----  VERIFIED */
void __cdecl LAN_GetServerAddressString( int source, int n, char *buf, int buflen )
{
  switch ( source )
  {
    case AS_LOCAL:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
      {
        Q_strncpyz( buf, NET_AdrToString( cls_localServers[n].adr ), buflen );
        return;
      }
      break;
    case AS_GLOBAL:
      if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
      {
        Q_strncpyz( buf, NET_AdrToString( cls_globalServers[n].adr ), buflen );
        return;
      }
      break;
    case AS_FAVORITES:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
      {
        Q_strncpyz( buf, NET_AdrToString( cls_favoriteServers[n].adr ), buflen );
        return;
      }
      break;
  }
  *buf = 0;
}

/* ---- LAN_GetServerInfo  0x00417A10 ----  VERIFIED */
void __cdecl LAN_GetServerInfo( int source, int n, char *buf, int buflen )
{
  char          info[MAX_INFO_STRING];
  serverInfo_t *server;

  info[0] = 0;
  server = NULL;
  switch ( source )
  {
    case AS_LOCAL:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        server = &cls_localServers[n];
      break;
    case AS_GLOBAL:
      if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
        server = &cls_globalServers[n];
      break;
    case AS_FAVORITES:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        server = &cls_favoriteServers[n];
      break;
  }

  if ( server && buf )
  {
    *buf = 0;
    Info_SetValueForKey( info, "hostname", server->hostName );
    Info_SetValueForKey( info, "mapname", server->mapName );
    Info_SetValueForKey( info, "clients", va( "%i", server->clients ) );
    Info_SetValueForKey( info, "sv_maxclients", va( "%i", server->maxClients ) );
    Info_SetValueForKey( info, "ping", va( "%i", server->ping ) );
    Info_SetValueForKey( info, "minping", va( "%i", server->minPing ) );
    Info_SetValueForKey( info, "maxping", va( "%i", server->maxPing ) );
    Info_SetValueForKey( info, "game", server->game );
    Info_SetValueForKey( info, "gametype", server->gameType );
    Info_SetValueForKey( info, "nettype", va( "%i", server->netType ) );
    Info_SetValueForKey( info, "addr", NET_AdrToString( server->adr ) );
    Info_SetValueForKey( info, "sv_allowAnonymous", va( "%i", server->allowAnonymous ) );
    Info_SetValueForKey( info, "pswrd", va( "%i", server->password ) );
    strncpy( buf, info, buflen - 1 );
    buf[buflen - 1] = 0;
  }
  else
  {
    if ( buf )
      *buf = 0;
  }
}

/* ---- LAN_GetServerPing  0x00417C80 ---- */
static int LAN_GetServerPing( int source, int n )
{
  serverInfo_t *server;

  server = NULL;
  switch ( source )
  {
    case AS_LOCAL:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        server = &cls_localServers[n];
      break;
    case AS_GLOBAL:
      if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
        server = &cls_globalServers[n];
      break;
    case AS_FAVORITES:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        server = &cls_favoriteServers[n];
      break;
  }
  if ( server )
    return server->ping;
  return -1;
}

/* ---- LAN_GetServerPtr  0x00417CE0 ----  VERIFIED */
serverInfo_t *__cdecl LAN_GetServerPtr( int n, int source )
{
  switch ( source )
  {
    case AS_LOCAL:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        return &cls_localServers[n];
      break;
    case AS_GLOBAL:
      if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
        return &cls_globalServers[n];
      break;
    case AS_FAVORITES:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        return &cls_favoriteServers[n];
      break;
    default:
      break;
  }
  return NULL;
}

/* ---- LAN_CompareServers  0x00417D40 ----  VERIFIED */
int __cdecl LAN_CompareServers( int source, int sortKey, int reverse, unsigned int n1, unsigned int n2 )
{
  serverInfo_t *s1;
  serverInfo_t *s2;
  int           result;

  s1 = LAN_GetServerPtr( n1, source );
  s2 = LAN_GetServerPtr( n2, source );
  if ( !s1 || !s2 )
    return 0;

  result = 0;
  switch ( sortKey )
  {
    case 0:   /* password */
      if ( s1->password == s2->password )
        result = 0;
      else
        result = s1->password ? 1 : -1;
      break;
    case 1:   /* host name -- retail 0x00417D97: s1 in EDX (Q_stricmp's first operand), s2 in ECX */
      result = Q_stricmp( s1->hostName, s2->hostName );
      break;
    case 2:   /* map name */
      result = Q_stricmp( s1->mapName, s2->mapName );
      break;
    case 3:   /* client count */
      if ( s1->clients < s2->clients )
        result = -1;
      else if ( s1->clients > s2->clients )
        result = 1;
      else
        result = 0;
      break;
    case 4:   /* game type */
    {
      int cmp = Q_stricmp( s1->gameType, s2->gameType );
      if ( cmp < 0 )
        result = -1;
      else if ( cmp > 0 )
        result = 1;
      else
        result = 0;
      break;
    }
    case 5:   /* ping */
      if ( s1->ping < s2->ping )
        result = -1;
      else if ( s1->ping > s2->ping )
        result = 1;
      else
        result = 0;
      break;
  }

  if ( reverse )
  {
    if ( result < 0 )
      return 1;
    if ( result > 0 )
      return -1;
    return 0;
  }
  return result;
}

/* ---- LAN_GetPingQueueCount  0x00417E40 ----  [CONFIRMED] */
int __cdecl LAN_GetPingQueueCount()
{
  return CL_GetPingQueueCount();
}

/* ---- LAN_ClearPing  0x00417E50 ----  VERIFIED */
void __cdecl LAN_ClearPing( int n )
{
  /* clearing adr.port frees the slot */
  if ( n < 0 || n >= MAX_PINGREQUESTS )
    return;
  cl_pinglist[n].adr.port = 0;
}

/* ---- LAN_GetPing  0x00417E70 ----  VERIFIED */
void __cdecl LAN_GetPing( int n, char *buf, int buflen, int *pingtime )
{
  CL_GetPing( n, buf, buflen, pingtime );
}

/* ---- LAN_GetPingInfo  0x00417E80 ----  VERIFIED */
void __cdecl LAN_GetPingInfo( int n, char *buf, int buflen )
{
  if ( !cl_pinglist[n].adr.port )   /* slot free */
  {
    if ( buflen )
      *buf = 0;
    return;
  }
  strncpy( buf, cl_pinglist[n].info, buflen - 1 );
  buf[buflen - 1] = 0;
}

/* ---- LAN_MarkServerVisible  0x00417EC0 ----  VERIFIED */
void __cdecl LAN_MarkServerVisible( int source, int n, int visible )
{
  if ( n == -1 )
  {
    int           count = MAX_OTHER_SERVERS;
    serverInfo_t *server = NULL;

    switch ( source )
    {
      case AS_LOCAL:
        server = &cls_localServers[0];
        break;
      case AS_GLOBAL:
        server = &cls_globalServers[0];
        count  = MAX_GLOBAL_SERVERS;
        break;
      case AS_FAVORITES:
        server = &cls_favoriteServers[0];
        break;
    }
    if ( server )
    {
      for ( n = 0; n < count; n++ )
        server[n].visible = visible;
    }
  }
  else
  {
    switch ( source )
    {
      case AS_LOCAL:
        if ( n >= 0 && n < MAX_OTHER_SERVERS )
          cls_localServers[n].visible = visible;
        break;
      case AS_GLOBAL:
        if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
          cls_globalServers[n].visible = visible;
        break;
      case AS_FAVORITES:
        if ( n >= 0 && n < MAX_OTHER_SERVERS )
          cls_favoriteServers[n].visible = visible;
        break;
    }
  }
}

/* ---- LAN_ServerIsVisible  0x00417F60 ---- */
static int LAN_ServerIsVisible( int source, int n )
{
  switch ( source )
  {
    case AS_LOCAL:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        return cls_localServers[n].visible;
      break;
    case AS_GLOBAL:
      if ( n >= 0 && n < MAX_GLOBAL_SERVERS )
        return cls_globalServers[n].visible;
      break;
    case AS_FAVORITES:
      if ( n >= 0 && n < MAX_OTHER_SERVERS )
        return cls_favoriteServers[n].visible;
      break;
  }
  return qfalse;
}

/* ---- LAN_UpdateVisiblePings  0x00417FC0 ----  [CONFIRMED] */
int __cdecl LAN_UpdateVisiblePings( int source )
{
  return CL_UpdateVisiblePings_f( source );
}

/* ---- LAN_GetServerStatus  0x00417FD0 ----  [CONFIRMED] */
qboolean __cdecl LAN_GetServerStatus( char *serverAddress, char *serverStatus, int maxLen )
{
  return CL_ServerStatus( serverAddress, serverStatus, maxLen );
}

/* ---- CL_GetGlconfig  0x00417FE0 ----  VERIFIED */
char **__cdecl CL_GetGlconfig( char **result )
{
  memcpy( result, &cls_glconfig, 0xA0u );
  return result;
}

/* ---- GetClipboardData_CL_ui  0x00418000 ----  VERIFIED */
void __cdecl GetClipboardData_CL_ui( char *buf, int buflen )
{
  char *cbd;

  cbd = Sys_GetClipboardData();
  if ( !cbd )
  {
    *buf = 0;
    return;
  }
  strncpy( buf, cbd, buflen - 1 );
  buf[buflen - 1] = 0;
  free( cbd );
}

/* ---- Key_KeynumToStringBuf  0x00418030 ----  VERIFIED */
void __cdecl Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
  char *value;

  value = Key_KeynumToString( keynum, qtrue );   /* retail 0x00418031 mov edi,1 */
  strncpy( buf, value, buflen - 1 );
  buf[buflen - 1] = 0;
}

/* ---- Key_GetBindingBuf  0x00418050 ----  VERIFIED */
void __cdecl Key_GetBindingBuf( int keynum, char *buf, int buflen )
{
  char *value;

  if ( keynum == -1 )
    value = "";
  else
    value = (char *)keysStorage[3 * keynum + 2];

  if ( value )
  {
    strncpy( buf, value, buflen - 1 );
    buf[buflen - 1] = 0;
  }
  else
  {
    *buf = 0;
  }
}

/* ---- Key_GetCatcher  0x00418090 ----  [CONFIRMED] */
int Key_GetCatcher()
{
  return cls_keyCatchers;
}

/* ---- Key_SetCatcher  0x004180A0 ----  [CONFIRMED] */
int __cdecl Key_SetCatcher(int result)
{
  if ( (cls_keyCatchers & 1) != 0 )
    result |= 1u;
  cls_keyCatchers = result;
  return result;
}

/* ---- CLUI_GetCDKey  0x004180C0 ----  VERIFIED */
void __cdecl CLUI_GetCDKey( char *buf, int buflen, char *checksum, int checksumlen )
{
  cvar_t *fs;

  fs = Cvar_Get( "fs_game", "", 24 );
  /* retail 0x4180C0 copies inline (memcpy intrinsic), no Com_Memcpy call */
  if ( UI_usesUniqueCDKey() && fs && fs->string[0] != 0 )
  {
    memcpy( buf, &cl_cdkey[16], 16 );
    buf[16] = 0;
    memcpy( checksum, &cl_cdkeychecksum[4], 4 );
    checksum[4] = 0;
  }
  else
  {
    memcpy( buf, cl_cdkey, 16 );
    buf[16] = 0;
    memcpy( checksum, cl_cdkeychecksum, 4 );
    checksum[4] = 0;
  }
}

/* ---- CLUI_SetCDKey  0x00418170 ----  VERIFIED */
void __cdecl CLUI_SetCDKey( const char *key, const char *checksum )
{
  cvar_t *fs;

  fs = Cvar_Get( "fs_game", "", 24 );
  /* retail 0x418170 copies inline (memcpy intrinsic), no Com_Memcpy call */
  if ( UI_usesUniqueCDKey() && fs && fs->string[0] != 0 )
  {
    memcpy( &cl_cdkey[16], key, 16 );
    cl_cdkey[32] = 0;
    memcpy( &cl_cdkeychecksum[4], checksum, 4 );
    cl_cdkey[8] = 0;
    Com_WriteCDKey( fs->string, &cl_cdkey[16], &cl_cdkeychecksum[4] );
  }
  else
  {
    memcpy( cl_cdkey, key, 16 );
    memcpy( cl_cdkeychecksum, checksum, 4 );
    Com_WriteCDKey( "main", cl_cdkey, cl_cdkeychecksum );
  }
}

/* ---- GetConfigString  0x00418250 ----  VERIFIED */
int __cdecl GetConfigString( int index, char *buf, int size )
{
  int offset;

  if ( index < 0 || index >= 2048 )
    return 0;

  offset = cl_gameState_stringOffsets[index];
  if ( !offset )
  {
    if ( size )
      *buf = 0;
    return 0;
  }

  strncpy( buf, &cl_gameState_stringData[offset], size - 1 );
  buf[size - 1] = 0;
  return 1;
}

/* ---- GetClientname  0x00418290 ----  VERIFIED */
int __cdecl GetClientname( int clientNum, char *buf, size_t count )
{
  clientState_t *parseClients;   /* cl.parseClients[], global @ 0x01531134 */
  int            i;

  *buf = 0;
  if ( !cl_snap_valid )
    return 0;

  parseClients = (clientState_t *)cl_parseClientsStorage;

  /* scan cl.snap's client run (numClients @ 0x01434A54, parseClientsNum @ 0x01434A5C). */
  for ( i = 0; i < cl_snap_numClients; i++ )
  {
    clientState_t *cs = &parseClients[ ( cl_snap_parseClientsNum + i ) & 0x7FF ];
    if ( cs->clientNum == clientNum )
    {
      strncpy( buf, cs->name, count );
      return 1;
    }
  }
  return 0;
}

/* ---- FloatAsInt_CL_ui  0x004182F0 ----  [CONFIRMED] */
int __cdecl FloatAsInt_CL_ui( float f )
{
  int temp;

  *(float *)&temp = f;
  return temp;
}

/* ---- VMF  no-address ----  VERIFIED */
/* Q3's VMF: retail reads the float straight out of args[] (fld dword ptr [ebp+N]). */
#define VMF( x )    ( (float *)args )[x]

/* ---- CL_UISystemCalls  0x00418300 ----  VERIFIED */
/* Retail 0x00418300 goes straight to the jump table (no prologue call), and the
 * renderer cases call the re slots with no bound check.  Case bodies are in
 * retail's jump-table order (Q3 cl_ui.c's trap order). */
int CL_UISystemCalls( int *args ) {
	switch ( args[0] ) {

	case 0x00:
		Com_Error( ERR_DROP, "\x15%s", (const char *) args[1] );
		return 0;
	case 0x01:
		Com_Printf( "%s", (const char *) args[1] );
		return 0;
	case 0x02:
		return SEH_GetLanguageName( args[1] );
	case 0x03:
		return SEH_VerifyLanguageSelection( args[1] );
	case 0x04:
		return Sys_Milliseconds();

	case 0x05:
		Cvar_Register( (vmCvar_t *) args[1], (const char *) args[2],
		               (const char *) args[3], args[4] );
		return 0;
	case 0x06:
		Cvar_Update( (vmCvar_t *) args[1] );
		return 0;
	case 0x07:
		Cvar_Set2( (const char *) args[1], (const char *) args[2], qtrue );
		return 0;
	case 0x08:
		return FloatAsInt_CL_ui( Cvar_VariableValue( (const char *) args[1] ) );
	case 0x09:
		Cvar_VariableStringBuffer( (const char *) args[1], (char *) args[2], args[3] );
		return 0;
	case 0x0A:
		Cvar_SetValue( (const char *) args[1], VMF( 2 ) );
		return 0;
	case 0x0B:
		Cvar_Set2( (const char *) args[1], NULL, qfalse );
		return 0;
	case 0x0C:
		Cvar_Get( (const char *) args[1], (const char *) args[2], args[3] );
		return 0;
	case 0x0D:
		Cvar_InfoStringBuffer( args[1], (char *) args[2], args[3] );
		return 0;

	case 0x0E:  /* UI_ARGC -- inlined in retail; cmd_argc read at 0x0041843E */
		return Cmd_Argc();
	case 0x0F:
		Cmd_ArgvBuffer( args[1], (char *) args[2], args[3] );
		return 0;
	case 0x10:
		Cbuf_ExecuteText( args[1], (const char *) args[2] );
		return 0;

	case 0x11:
		return FS_FOpenFileByMode( (const char *) args[1], (int *) args[2], args[3] );
	case 0x12:
		FS_Read( (void *) args[1], args[2], args[3] );
		return 0;
	case 0x13:
		FS_Seek( args[1], args[2], args[3] );
		return 0;
	case 0x14:
		FS_Write( (const void *) args[1], args[2], args[3] );
		return 0;
	case 0x15:
		FS_FCloseFile( args[1] );
		return 0;
	case 0x17:
		return FS_Delete( args[1] );
	case 0x16:
		return FS_GetFileList( (const char *) args[1], (const char *) args[2],
		                       (char *) args[3], args[4] );

	case 0x18:  /* UI_R_REGISTERMODEL re +0x0C */
		return re.RegisterModel( (const char *) args[1], args[2] );
	case 0x19:  /* UI_R_REGISTERSHADERNOMIP re +0x14 */
		return re.RegisterShaderNoMip( (const char *) args[1], args[2] );
	case 0x1A:  /* UI_R_CLEARSCENE re +0x3C */
		re.ClearScene();
		return 0;
	case 0x1B:  /* UI_R_ADDREFENTITYTOSCENE re +0x40 -- retail passes a literal 0 */
		re.AddRefEntityToScene( (const void *) args[1], 0 );
		return 0;
	case 0x1C:  /* UI_R_ADDPOLYTOSCENE re +0x44 */
		re.AddPolyToScene( args[1], args[2], (const void *) args[3] );
		return 0;
	case 0x1D:  /* UI_R_ADDPOLYSTOSCENE re +0x48 */
		re.AddPolysToScene( args[1], args[2], (const void *) args[3], args[4] );
		return 0;
	case 0x1E:  /* UI_R_ADDLIGHTTOSCENE re +0x4C */
		re.AddLightToScene( (const float *) args[1], VMF( 2 ),
		                    VMF( 3 ), VMF( 4 ), VMF( 5 ) );
		return 0;
	case 0x1F:  /* UI_R_ADDCORONATOSCENE re +0x54 -- retail passes a literal 1 last */
		re.AddCoronaToScene( (const float *) args[1], VMF( 2 ),
		                     VMF( 3 ), VMF( 4 ), VMF( 5 ),
		                     args[6], 1 );
		return 0;
	case 0x20:  /* UI_R_RENDERSCENE re +0x64 */
		re.RenderScene( (const void *) args[1] );
		return 0;
	case 0x21:  /* UI_R_SETCOLOR re +0x6C A POINTER to four floats, not four packed values: RE_SetColor (0x004DDCF0) dereferences it and substitutes a default when it is NULL. */
		re.SetColor( (const float *) args[1] );
		return 0;
	case 0x22:  /* UI_R_DRAWSTRETCHPIC re +0x70 */
		re.StretchPic( VMF( 1 ), VMF( 2 ), VMF( 3 ),
		               VMF( 4 ), VMF( 5 ), VMF( 6 ),
		               VMF( 7 ), VMF( 8 ), args[9] );
		return 0;
	case 0x23:  /* UI_R_MODELBOUNDS re +0x9C */
		re.ModelBounds( args[1], (float *) args[2], (float *) args[3] );
		return 0;

	case 0x24:
		SCR_UpdateScreen();
		return 0;
	case 0x26: {
		int *alias = Com_FindSoundAlias( (const char *) args[1], 0 );
		return alias ? *alias : 0;
	}
	case 0x27:
		MSS_PlayLocalSoundAlias( 0, args[1] );
		return 0;
	case 0x28:
		MSS_FadeAllSounds( args[1], args[2] );
		return 0;

	case 0x29:  /* UI_KEY_KEYNUM_TO_STRING_BUF -- retail case 41, 0x004186B8 */
		Key_KeynumToStringBuf( args[1], (char *) args[2], args[3] );
		return 0;
	case 0x2A:
		Key_GetBindingBuf( args[1], (char *) args[2], args[3] );
		return 0;
	case 0x2B:
		Key_SetBinding( args[1], args[2] );
		return 0;
	case 0x2C:
		return Key_IsDown( args[1] );
	/* 0x0142F628 is the word Key_GetOverstrikeMode (0x0040D480) and Key_SetOverstrikeMode (0x0040D490) own (key_overstrikeMode). */
	case 0x2D:  /* UI_KEY_GET_OVERSTRIKE_MODE -- inlined; 0x00418700 */
		return key_overstrikeMode;
	case 0x2E:  /* UI_KEY_SET_OVERSTRIKE_MODE -- 0x00418710 */
		key_overstrikeMode = args[1];
		return 0;
	case 0x2F:
		Key_ClearStates();
		return 0;
	case 0x30:  /* UI_KEY_GET_CATCHER -- inlined; cls.keyCatchers at 0x00418726 */
		return Key_GetCatcher();
	case 0x31:
		Key_SetCatcher( args[1] );
		return 0;

	case 0x32:  /* UI_GET_CLIPBOARD_DATA -- retail case 50, 0x0041873F */
		GetClipboardData_CL_ui( (char *) args[1], args[2] );
		return 0;
	case 0x33:
		GetClientState( (uiClientState_t *) args[1] );
		return 0;
	case 0x34:
		memcpy( (void *) args[1], &cls_glconfig, 0xA0u );
		return 0;
	case 0x35:
		return GetConfigString( args[1], (char *) args[2], args[3] );
	case 0x36:
		return GetClientname( args[1], (char *) args[2], args[3] );

	case 0x55:
		LAN_LoadCachedServers();
		return 0;
	case 0x56:
		LAN_SaveServersToCache();
		return 0;
	case 0x57:
		return LAN_AddServer( args[1], (char *) args[2], (char *) args[3] );
	case 0x58:
		LAN_RemoveServer( args[1], (char *) args[2] );
		return 0;
	case 0x3B:
		return LAN_GetPingQueueCount();
	case 0x3C:
		LAN_ClearPing( args[1] );
		return 0;
	case 0x3D:
		/* Retail case 61 (0x00418800) loads args[1] into EAX -- CL_GetPing's register argument n -- and pushes args[4], args[3], args[2], so the trap order is Q3's (n, buf, buflen, pingtime). */
		LAN_GetPing( args[1], (char *) args[2], args[3], (int *) args[4] );
		return 0;
	case 0x3E:
		LAN_GetPingInfo( args[1], (char *) args[2], args[3] );
		return 0;
	case 0x4F:  /* retail 0x0041883A tail-jumps LAN_GetServerCount */
		return LAN_GetServerCount( args[1] );
	case 0x50:
		LAN_GetServerAddressString( args[1], args[2], (char *) args[3], args[4] );
		return 0;
	case 0x51:
		LAN_GetServerInfo( args[1], args[2], (char *) args[3], args[4] );
		return 0;
	case 0x60:  /* retail 0x00418883 tail-jumps LAN_GetServerPing */
		return LAN_GetServerPing( args[1], args[2] );
	case 0x52:
		LAN_MarkServerVisible( args[1], args[2], args[3] );
		return 0;
	case 0x61:  /* retail 0x004188A7 tail-jumps LAN_ServerIsVisible */
		return LAN_ServerIsVisible( args[1], args[2] );
	case 0x53:
		return LAN_UpdateVisiblePings( args[1] );
	case 0x54:
		LAN_ResetPings( args[1] );
		return 0;
	case 0x5F:
		return LAN_GetServerStatus( (char *) args[1], (char *) args[2], args[3] );
	case 0x62:
		return LAN_CompareServers( args[1], args[2], args[3], args[4], args[5] );

	case 0x3F:
		return Hunk_MemoryRemaining();
	case 0x40:  /* UI_GET_CD_KEY -- retail case 64, 0x0041891D */
		CLUI_GetCDKey( (char *) args[1], args[2], (char *) args[3], args[4] );
		return 0;
	case 0x41:  /* UI_SET_CD_KEY -- retail case 65, 0x0041893A */
		CLUI_SetCDKey( (const char *) args[1], (const char *) args[2] );
		return 0;

	case 0x42:  /* UI_R_REGISTERFONT re +0xA8, 4 args. Retail discards the (void) callee's EAX and returns 0 (xor eax,eax at 0x0041896C). */
		re.RegisterFont( args[1], args[2], args[3], args[4] );
		return 0;
	case 0x43:  /* UI_R_TEXT_WIDTH re +0xCC, 5 args */
		return re.Text_Width( args[1], args[2], args[3], 0, args[4] );
	case 0x44:  /* UI_R_TEXT_HEIGHT re +0xD0, 2 args */
		return re.Text_Height( args[1], args[2] );
	case 0x45:  /* UI_R_TEXT_PAINT re +0xD4, 9 args. Retail returns 0 (xor eax,eax at 0x004189D6). */
		re.Text_Paint( args[1], args[2], args[3], args[4], args[5],
		               args[6], args[7], args[8], args[9] );
		return 0;
	case 0x46:  /* UI_R_TEXT_PAINTWITHCURSOR re +0xE0, 11 args. args[8] is narrowed to its low byte at 0x00418A06 and a literal 0 follows it. Retail returns 0 (xor eax,eax at 0x00418A12). */
		( (void (*)( int, int, int, int, int, int, int, char, int, int, int ))re.Text_PaintWithCursor )(
		                         args[1], args[2], args[3], args[4],
		                         args[5], args[6], args[7],
		                         (char) args[8], 0,
		                         args[9], args[10] );
		return 0;

	case 0x47:
		return SEH_StringEd_GetString( (const char *) args[1] );
	case 0x48:
		return (int) SEH_LocalizeTextMessage( (char *) args[1], (const char *) args[2], 0 );

	case 0x66:
		return (int) memset( (void *) args[1], args[2], (size_t) args[3] );
	case 0x67:
		return (int) memcpy( (void *) args[1], (const void *) args[2], (size_t) args[3] );
	case 0x68:
		return (int) strncpy( (char *) args[1], (const char *) args[2], (size_t) args[3] );

	case 0x69:
		return FloatAsInt_CL_ui( (float) sin( VMF( 1 ) ) );
	case 0x6A:
		return FloatAsInt_CL_ui( (float) cos( VMF( 1 ) ) );
	case 0x6B:
		return FloatAsInt_CL_ui( (float) atan2( VMF( 1 ), VMF( 2 ) ) );
	case 0x6C:
		return FloatAsInt_CL_ui( (float) sqrt( VMF( 1 ) ) );
	case 0x6D:
		return FloatAsInt_CL_ui( (float) floor( VMF( 1 ) ) );
	case 0x6E:
		return FloatAsInt_CL_ui( (float) ceil( VMF( 1 ) ) );

	case 0x49:
		return PC_AddGlobalDefine( (const char *) args[1] );
	case 0x4A:  /* UI_PC_LOAD_SOURCE -- retail 0x00418B2F tail-jumps PC_LoadSourceHandle, no debug print */
		return PC_LoadSourceHandle( (const char *) args[1] );
	case 0x4B:
		return PC_FreeSourceHandle( args[1] );
	case 0x4C:
		return PC_ReadTokenHandle( args[1], (pc_token_t *) args[2] );
	case 0x4D:
		return PC_SourceFileAndLine( args[1], (char *) args[2], (int *) args[3] );
	case 0x4E:
		return Com_RealTime( (int *) args[1] );

	case 0x59:
		Com_DPrintf( "UI_CIN_PlayCinematic\n" );
		return CIN_PlayCinematic( (const char *) args[1], args[2], args[3],
		                          args[4], args[5], args[6] );
	case 0x5A:
		return CIN_StopCinematic( args[1] );
	case 0x5B:
		return CIN_RunCinematic( args[1] );
	case 0x5C:
		CIN_DrawCinematic( args[1] );
		return 0;
	case 0x5D:
		CIN_SetExtents( args[1], args[2], args[3], args[4], args[5] );
		return 0;

	case 0x5E:
		return CL_CDKeyValidate( (const char *) args[1], (const char *) args[2] );
	case 0x64:
		CL_GetAutoUpdate();
		return 0;
	case 0x65:
		return ( cls_state == CA_ACTIVE );

	case 0x6F:
		return (int) Z_MallocInternal( args[1] );
	case 0x70:
		free( (void *) args[1] );
		return 0;

	case 0x63:
		return 0;

	default:
		Com_Error( ERR_DROP, "\x15" "Bad UI system trap: %i", args[0] );
		return 0;
	}
}

/* ---- CL_InitUI  0x00418F10 ----  VERIFIED */
void CL_InitUI( void )
{
	int version;

	Com_LoadSoundAliases( "menu", 0 );

	uivm = VM_Create( "ui", CL_UISystemCalls );
	if ( !uivm ) {
		Com_Error( ERR_FATAL, "\x15VM_Create on UI failed" );
	}

	version = VM_Call( uivm, UI_GETAPIVERSION );
	if ( version != UI_API_VERSION ) {
		Com_Error( ERR_FATAL, "\x15User Interface is version %d, expected %d",
		           version, UI_API_VERSION );
		cls_loadingPlaque = 0;
	}

	VM_Call( uivm, UI_INIT );
}

/* ---- UI_usesUniqueCDKey  0x00418F90 ----  VERIFIED */
qboolean __cdecl UI_usesUniqueCDKey( void )
{
  if ( uivm )
    return VM_Call( uivm, 13 ) == qtrue;
  else
    return qfalse;
}

/* ---- UI_checkKeyExec  0x00418FB0 ----  [CONFIRMED] */
int __cdecl UI_checkKeyExec(void *this)
{
  if ( uivm )
    return VM_Call(uivm, 14, this);
  else
    return 0;
}

/* ---- UI_GameCommand  0x00418FD0 ----  VERIFIED */
int __cdecl UI_GameCommand( void )
{
  if ( !uivm )
    return 0;
  return VM_Call( uivm, 11, cls_realtime );
}

/* ---- CL_ShutdownUI  0x00418E30 ----  [CONFIRMED] */
void CL_ShutdownUI( void ) {

	cls_keyCatchers &= ~KEYCATCH_UI;
	cls_loadingPlaque = 0;

	if ( !uivm ) {
		return;
	}

	VM_Call( uivm, UI_SHUTDOWN );
	VM_Free( uivm );
	uivm = NULL;

	if ( snd_aliasSetLoaded[SND_LOCALE_MENU] ) {
		Com_UnloadSoundAliasSounds( SND_LOCALE_MENU );

		if ( snd_aliasTable[SND_LOCALE_MENU] ) {
			free( (void *)snd_aliasTable[SND_LOCALE_MENU] );
			snd_aliasTable[SND_LOCALE_MENU]      = NULL;
			snd_aliasTableCount[SND_LOCALE_MENU] = 0;
			memset( snd_aliasHashTable[SND_LOCALE_MENU], 0,
					SND_ALIAS_HASH_SLICE_BYTES );
		}

		snd_aliasSetLoaded[SND_LOCALE_MENU] = 0;

		if ( !snd_aliasSetLoaded[SND_LOCALE_INGAME] ) {
			Cmd_RemoveCommand( "snd_list" );
		}
	}
}
