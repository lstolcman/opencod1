/*
 * @fidelity: likely
 */

#include "../qcommon/qcommon.h"
#include "../qcommon/win_vars.h"

/* Win32 types this unit uses (windows.h is not included here). */
typedef void *HMIDIIN;
typedef struct { long l, t, r, b; } RECT;

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwXpos;
	unsigned int dwYpos;
	unsigned int dwZpos;
	unsigned int dwRpos;
	unsigned int dwUpos;
	unsigned int dwVpos;
	unsigned int dwButtons;
	unsigned int dwButtonNumber;
	unsigned int dwPOV;
	unsigned int dwReserved1;
	unsigned int dwReserved2;
} joyInfoEx_t;

typedef struct {
	unsigned short wMid;
	unsigned short wPid;
	char           szPname[32];
	unsigned int   wXmin;
	unsigned int   wXmax;
	unsigned int   wYmin;
	unsigned int   wYmax;
	unsigned int   wZmin;
	unsigned int   wZmax;
	unsigned int   wNumButtons;
	unsigned int   wPeriodMin;
	unsigned int   wPeriodMax;
	unsigned int   wRmin;
	unsigned int   wRmax;
	unsigned int   wUmin;
	unsigned int   wUmax;
	unsigned int   wVmin;
	unsigned int   wVmax;
	unsigned int   wCaps;
	unsigned int   wMaxAxes;
	unsigned int   wNumAxes;
	unsigned int   wMaxButtons;
	char           szRegKey[32];
	char           szOEMVxD[260];
} joyCaps_t;

typedef struct {
	unsigned short wMid;
	unsigned short wPid;
	unsigned int   vDriverVersion;
	char           szPname[32];
	unsigned int   dwSupport;
} midiInCaps_t;

/* MIDI/joystick state owned by this unit. Retail keeps both as one
   object each (Q3's s_midiInfo and joy): IN_ShutdownMIDI clears all 360 bytes of s_midiInfo. */
typedef struct {
	int           numDevices;   /* 0x008E23B0 */
	midiInCaps_t  caps[8];      /* 0x008E23B4 */
	HMIDIIN       hMidiIn;      /* 0x008E2514 */
} MidiInfo_t;

MidiInfo_t s_midiInfo;

struct {
	qboolean      avail;            /* 0x008E2528 */
	int           id;               /* 0x008E252C */
	joyCaps_t     jc;               /* 0x008E2530 */
	int           oldbuttonstate;   /* 0x008E26C4 */
	int           oldpovstate;      /* 0x008E26C8 */
	joyInfoEx_t   ji;               /* 0x008E26CC */
} joy;

/* Globals owned by this unit. */
cvar_t *in_debugjoystick;
cvar_t *in_mouse;
cvar_t *in_joystick;
cvar_t *joy_threshold;
cvar_t *in_midichannel;
WinMouseVars_t s_wmv;   /* 0x008E251C */
cvar_t *in_joyBallScale;
cvar_t *in_mididevice;
cvar_t *in_midiport;
cvar_t *in_midi;
int X;   /* 0x008E2518, mouse centre x (RTCW window_center_x) */
int Y;   /* 0x008E23A8, mouse centre y (RTCW window_center_y) */
int in_appactive;   /* 0x016BDD98 */

extern void Sys_QueEvent( int time, int type, int value, int value2,
                          int ptrLength, void *ptr );

/* windows.h-style imports: retail calls them through the IAT (call dword ptr [__imp_...]). */
__declspec( dllimport ) int __stdcall ClipCursor( const void *r );
__declspec( dllimport ) int __stdcall ShowCursor( int show );
extern int  ReleaseCapture( void );
__declspec( dllimport ) unsigned __stdcall midiInClose( void *h );
__declspec( dllimport ) int __stdcall GetSystemMetrics( int index );
__declspec( dllimport ) int __stdcall GetWindowRect( void *hWnd, void *rect );
__declspec( dllimport ) int __stdcall GetCursorPos( void *point );
__declspec( dllimport ) int __stdcall SetCursorPos( int x, int y );
__declspec( dllimport ) void * __stdcall SetCapture( void *hWnd );


void IN_JoyMove( void );
/* IN_Startup (0x004618B0) and IN_Shutdown call these, which are defined below them. */
void IN_StartupJoystick( void );
void IN_StartupMIDI( void );
void IN_ShutdownMIDI( void );
void MidiInfo_f( void );

/* ---- IN_InitWin32Mouse  0x004615C0 ----  VERIFIED */
void IN_InitWin32Mouse()
{
  ;
}

/* ---- IN_ShutdownWin32Mouse  0x004615D0 ----  VERIFIED */
void IN_ShutdownWin32Mouse()
{
  ;
}

/* ---- IN_ActivateWin32Mouse  0x004615E0 ----  VERIFIED */
int IN_ActivateWin32Mouse( void )
{
  int width;
  int height;
  int result;
  RECT Rect;

  width  = GetSystemMetrics( 0 );
  height = GetSystemMetrics( 1 );
  GetWindowRect( g_wv_hWnd, &Rect );
  if ( Rect.l < 0 )
    Rect.l = 0;
  if ( Rect.t < 0 )
    Rect.t = 0;
  if ( Rect.r >= width )
    Rect.r = width - 1;
  if ( Rect.b >= height - 1 )
    Rect.b = height - 1;
  X = ( Rect.l + Rect.r ) / 2;
  Y = ( Rect.b + Rect.t ) / 2;
  SetCursorPos( X, Y );
  SetCapture( g_wv_hWnd );
  if ( !com_developer->integer )
    ClipCursor( &Rect );
  do
    result = ShowCursor( 0 );
  while ( result >= 0 );
  return result;
}

/* ---- IN_DeactivateWin32Mouse  0x004616B0 ----  VERIFIED */
int IN_DeactivateWin32Mouse()
{
  int result;

  if ( !com_developer->integer )
    ClipCursor(0);
  ReleaseCapture();
  do
    result = ShowCursor(1);
  while ( result < 0 );
  return result;
}

/* ---- IN_Win32Mouse  0x004616E0 ----  VERIFIED */
void IN_Win32Mouse( int *mx, int *my )
{
  long Point[2];

  GetCursorPos( Point );
  SetCursorPos( X, Y );
  *mx = (int) Point[0] - X;
  *my = (int) Point[1] - Y;
}

/* ---- IN_ActivateMouse  0x00461730 ---- */
void IN_ActivateMouse( void )
{
  if ( !s_wmv_mouseInitialized )
    return;

  if ( !in_mouse->integer )
  {
    s_wmv_mouseActive = 0;
    return;
  }

  if ( s_wmv_mouseActive )
    return;

  s_wmv_mouseActive = 1;
  IN_ActivateWin32Mouse();
}

/* ---- IN_DeactivateMouse  0x00461770 ----  VERIFIED */
int IN_DeactivateMouse()
{
  int result;

  result = s_wmv_mouseInitialized;
  if ( s_wmv_mouseInitialized )
  {
    result = s_wmv_mouseActive;
    if ( s_wmv_mouseActive )
    {
      s_wmv_mouseActive = 0;
      return IN_DeactivateWin32Mouse();
    }
  }
  return result;
}

/* ---- IN_StartupMouse  0x004617A0 ---- */
void IN_StartupMouse( void )
{
  s_wmv_mouseInitialized = 0;

  if ( in_mouse->integer == 0 )
  {
    Com_Printf("Mouse control not active.\n");
    return;
  }

  s_wmv_mouseInitialized = 1;
}

/* ---- IN_MouseEvent  0x004617D0 ---- */
void IN_MouseEvent( int mstate )
{
  int i;

  if ( s_wmv_mouseInitialized )
  {
    for ( i = 0; i < 3; ++i )
    {
      if ( ( mstate & ( 1 << i ) ) != 0 && ( s_wmv_oldButtonState & ( 1 << i ) ) == 0 )
        Sys_QueEvent( g_wv_sysMsgTime, 1 , i + 200 , 1, 0, 0 );
      if ( ( mstate & ( 1 << i ) ) == 0 && ( s_wmv_oldButtonState & ( 1 << i ) ) != 0 )
        Sys_QueEvent( g_wv_sysMsgTime, 1 , i + 200 , 0, 0, 0 );
    }
    s_wmv_oldButtonState = mstate;
  }
}

/* ---- IN_MouseMove  0x00461850 ---- */
void IN_MouseMove( void )
{
  int mx, my;

  IN_Win32Mouse( &mx, &my );

  if ( !mx && !my )
    return;

  Sys_QueEvent( 0, 3 , mx, my, 0, 0 );
}

/* ---- IN_Startup  0x004618B0 ---- */
void IN_Startup( void )
{
  IN_StartupMouse();
  IN_StartupJoystick();
  IN_StartupMIDI();
  in_mouse->modified = qfalse;
  in_joystick->modified = qfalse;
}

/* ---- IN_Shutdown  0x00461910 ----  VERIFIED */
void IN_Shutdown()
{
  IN_DeactivateMouse();
  IN_ShutdownMIDI();
  Cmd_RemoveCommand("midiinfo");
}

/* ---- IN_Init  0x00461960 ----  VERIFIED */
void IN_Init( void )
{
  in_midi          = Cvar_Get( "in_midi",          "0",    CVAR_ARCHIVE );
  in_midiport      = Cvar_Get( "in_midiport",      "1",    CVAR_ARCHIVE );
  in_midichannel   = Cvar_Get( "in_midichannel",   "1",    CVAR_ARCHIVE );
  in_mididevice    = Cvar_Get( "in_mididevice",    "0",    CVAR_ARCHIVE );
  Cmd_AddCommand( "midiinfo", (xcommand_t) MidiInfo_f );
  in_mouse         = Cvar_Get( "in_mouse",         "1",    CVAR_ARCHIVE | CVAR_LATCH );
  in_joystick      = Cvar_Get( "in_joystick",      "0",    CVAR_ARCHIVE | CVAR_LATCH );
  in_joyBallScale  = Cvar_Get( "in_joyBallScale",  "0.02", CVAR_ARCHIVE );
  in_debugjoystick = Cvar_Get( "in_debugjoystick", "0",    CVAR_TEMP );
  joy_threshold    = Cvar_Get( "joy_threshold",    "0.15", CVAR_ARCHIVE );

  IN_Startup();
}

/* ---- IN_Activate  0x00461A50 ----  VERIFIED */
int __cdecl IN_Activate(int result)
{
  in_appactive = result;
  if ( !result )
  {
    result = s_wmv_mouseInitialized;
    if ( s_wmv_mouseInitialized )
    {
      result = s_wmv_mouseActive;
      if ( s_wmv_mouseActive )
      {
        s_wmv_mouseActive = 0;
        return IN_DeactivateWin32Mouse();
      }
    }
  }
  return result;
}

/* ---- IN_Frame  0x00461A80 ----  VERIFIED */
void IN_Frame( void )
{
  IN_JoyMove();

  if ( !s_wmv_mouseInitialized )
    return;

  if ( cls_keyCatchers & 1 )
  {
    if ( Cvar_VariableValue( "r_fullscreen" ) == 0 )
    {
      IN_DeactivateMouse();
      return;
    }
  }

  if ( !in_appactive )
  {
    IN_DeactivateMouse();
    return;
  }

  IN_ActivateMouse();
  IN_MouseMove();
}

/* ---- IN_ClearStates  0x00461AF0 ----  VERIFIED */
void IN_ClearStates()
{
  s_wmv_oldButtonState = 0;
}

__declspec( dllimport ) unsigned int __stdcall joyGetNumDevs( void );
__declspec( dllimport ) unsigned int __stdcall joyGetPosEx( unsigned int uJoyID, void *pji );
__declspec( dllimport ) unsigned int __stdcall joyGetDevCapsA( unsigned int uJoyID, void *pjc, unsigned int cbjc );

static int joyDirectionKeys[16] = {
	156, 157,
	154, 155,
	222, 223, 224, 225, 226, 227,
	228, 229, 230, 231, 232, 233
};

/* ---- IN_StartupJoystick  0x00461B00 ----  VERIFIED */
void IN_StartupJoystick( void )
{
  int  numdevs;
  unsigned int mmr;

  joy.avail = 0;

  if ( !in_joystick->integer )
    return;

  if ( ( numdevs = joyGetNumDevs() ) == 0 )
  {
    Com_DPrintf( "joystick not found -- driver not present\n" );
    return;
  }

  mmr = 0;
  for ( joy.id = 0; joy.id < numdevs; joy.id++ )
  {
    memset( &joy.ji, 0, sizeof( joyInfoEx_t ) );
    joy.ji.dwSize  = sizeof( joyInfoEx_t );
    joy.ji.dwFlags = 0x400;

    if ( ( mmr = joyGetPosEx( joy.id, &joy.ji ) ) == 0 )
      break;
  }

  if ( mmr != 0 )
  {
    Com_Printf( "joystick not found -- no valid joysticks (%x)\n", mmr );
    return;
  }

  memset( &joy.jc, 0, sizeof( joyCaps_t ) );
  if ( ( mmr = joyGetDevCapsA( joy.id, &joy.jc, sizeof( joyCaps_t ) ) ) != 0 )
  {
    Com_Printf( "joystick not found -- invalid joystick capabilities (%x)\n", mmr );
    return;
  }

  Com_DPrintf( "Joystick found.\n" );
  Com_DPrintf( "Pname: %s\n", joy.jc.szPname );
  Com_DPrintf( "OemVxD: %s\n", joy.jc.szOEMVxD );
  Com_DPrintf( "RegKey: %s\n", joy.jc.szRegKey );
  Com_DPrintf( "Numbuttons: %i / %i\n", joy.jc.wNumButtons, joy.jc.wMaxButtons );
  Com_DPrintf( "Axis: %i / %i\n", joy.jc.wNumAxes, joy.jc.wMaxAxes );
  Com_DPrintf( "Caps: 0x%x\n", joy.jc.wCaps );
  if ( joy.jc.wCaps & 0x10 )
    Com_DPrintf( "HASPOV\n" );
  else
    Com_DPrintf( "no POV\n" );

  joy.oldbuttonstate = 0;
  joy.oldpovstate = 0;

  joy.avail = 1;
}

/* ---- JoyToF  0x00461CA0 ----  VERIFIED */
rfloat __cdecl JoyToF(int value)
{
  rfloat result;

  result = (rfloat)(value - 0x8000) * ( 1.0f / 32768.0f );
  if ( result < -1.0f )
    return -1.0f;
  if ( result > 1.0f )
    return 1.0f;
  return result;
}

/* ---- JoyToI  0x00461CF0 ----  VERIFIED */
int __cdecl JoyToI(int value)
{
  return value - 0x8000;
}

/* ---- IN_JoyMove  0x00461D00 ----  VERIFIED */
void IN_JoyMove( void )
{
  float        fAxisValue;
  int          i;
  unsigned int buttonstate, povstate;
  int          x, y;

  if ( !joy.avail )
    return;

  memset( &joy.ji, 0, sizeof( joyInfoEx_t ) );
  joy.ji.dwSize  = sizeof( joyInfoEx_t );
  joy.ji.dwFlags = 0xFF;

  if ( joyGetPosEx( joy.id, &joy.ji ) != 0 )
    return;

  if ( in_debugjoystick->integer )
  {
    Com_Printf( "%8x %5i %5.2f %5.2f %5.2f %5.2f %6i %6i\n",
                joy.ji.dwButtons,
                joy.ji.dwPOV,
                JoyToF( joy.ji.dwXpos ), JoyToF( joy.ji.dwYpos ),
                JoyToF( joy.ji.dwZpos ), JoyToF( joy.ji.dwRpos ),
                JoyToI( joy.ji.dwUpos ), JoyToI( joy.ji.dwVpos ) );
  }

  buttonstate = joy.ji.dwButtons;
  for ( i = 0; (unsigned int) i < joy.jc.wNumButtons; i++ )
  {
    if ( ( buttonstate & ( 1 << i ) ) && !( joy.oldbuttonstate & ( 1 << i ) ) )
      Sys_QueEvent( g_wv_sysMsgTime, 1 , 207  + i, 1, 0, 0 );
    if ( !( buttonstate & ( 1 << i ) ) && ( joy.oldbuttonstate & ( 1 << i ) ) )
      Sys_QueEvent( g_wv_sysMsgTime, 1 , 207  + i, 0, 0, 0 );
  }
  joy.oldbuttonstate = buttonstate;

  povstate = 0;

  for ( i = 0; (unsigned int) i < joy.jc.wNumAxes && i < 4; i++ )
  {
    fAxisValue = (float) JoyToF( ( &joy.ji.dwXpos )[i] );

    if ( fAxisValue < -joy_threshold->value )
      povstate |= ( 1 << ( i * 2 ) );
    else if ( fAxisValue > joy_threshold->value )
      povstate |= ( 1 << ( i * 2 + 1 ) );
  }

  if ( joy.jc.wCaps & 0x10 )
  {
    if ( joy.ji.dwPOV != 0xFFFF )
    {
      if ( joy.ji.dwPOV == 0 )
        povstate |= 1 << 12;
      if ( joy.ji.dwPOV == 0x4650 )
        povstate |= 1 << 13;
      if ( joy.ji.dwPOV == 0x2328 )
        povstate |= 1 << 14;
      if ( joy.ji.dwPOV == 0x6978 )
        povstate |= 1 << 15;
    }
  }

  for ( i = 0; i < 16; i++ )
  {
    if ( ( povstate & ( 1 << i ) ) && !( joy.oldpovstate & ( 1 << i ) ) )
      Sys_QueEvent( g_wv_sysMsgTime, 1 , joyDirectionKeys[i], 1, 0, 0 );
    if ( !( povstate & ( 1 << i ) ) && ( joy.oldpovstate & ( 1 << i ) ) )
      Sys_QueEvent( g_wv_sysMsgTime, 1 , joyDirectionKeys[i], 0, 0, 0 );
  }
  joy.oldpovstate = povstate;

  if ( joy.jc.wNumAxes >= 6 )
  {
    /* retail 0x461FBB: fild int, fmul value, __ftol2 - the int is never rounded to float */
    x = (int) ( (double) JoyToI( joy.ji.dwUpos ) * in_joyBallScale->value );
    y = (int) ( (double) JoyToI( joy.ji.dwVpos ) * in_joyBallScale->value );
    if ( x || y )
      Sys_QueEvent( g_wv_sysMsgTime, 3 , x, y, 0, 0 );
  }
}

__declspec( dllimport ) unsigned int __stdcall midiInGetNumDevs( void );
__declspec( dllimport ) unsigned int __stdcall midiInGetDevCapsA( unsigned int uDeviceID, void *pmic, unsigned int cbmic );
__declspec( dllimport ) unsigned int __stdcall midiInOpen( HMIDIIN *phmi, unsigned int uDeviceID, unsigned long dwCallback,
                                  unsigned long dwInstance, unsigned long fdwOpen );
__declspec( dllimport ) unsigned int __stdcall midiInStart( HMIDIIN hmi );

/* ---- MIDI_NoteOff  0x00462010 ----  VERIFIED */
void MIDI_NoteOff( int note )
{
  int qkey;

  qkey = note + 179;

  if ( qkey > 255 || qkey < 239 )
    return;

  Sys_QueEvent( g_wv_sysMsgTime, 1 , qkey, 0 , 0, 0 );
}

/* ---- MIDI_NoteOn  0x00462040 ----  VERIFIED */
void MIDI_NoteOn( int note, int velocity )
{
  int qkey;

  if ( velocity == 0 )
    MIDI_NoteOff( note );

  qkey = note + 179;

  if ( qkey > 255 || qkey < 239 )
    return;

  Sys_QueEvent( g_wv_sysMsgTime, 1 , qkey, 1 , 0, 0 );
}

/* ---- MidiInProc  0x004620A0 ----  VERIFIED */
void __stdcall MidiInProc( HMIDIIN hMidiIn, unsigned int uMsg, unsigned long dwInstance,
                           unsigned long dwParam1, unsigned long dwParam2 )
{
  int message;

  (void) hMidiIn;
  (void) dwInstance;
  (void) dwParam2;

  if ( uMsg != 0x3C3  )
    return;

  message = dwParam1 & 0xFF;

  if ( ( message & 0xF0 ) == 0x90 )
  {
    if ( ( ( message & 0x0F ) + 1 ) == in_midichannel->integer )
      MIDI_NoteOn( ( dwParam1 & 0xFF00 ) >> 8, ( dwParam1 & 0xFF0000 ) >> 16 );
  }
  else if ( ( message & 0xF0 ) == 0x80 )
  {
    if ( ( ( message & 0x0F ) + 1 ) == in_midichannel->integer )
      MIDI_NoteOff( ( dwParam1 & 0xFF00 ) >> 8 );
  }
}

/* ---- MidiInfo_f  0x00462110 ----  VERIFIED */
void MidiInfo_f( void )
{
  int i;
  const char *enabled[2];

  enabled[0] = "disabled";
  enabled[1] = "enabled";

  Com_Printf( "\nMIDI control:       %s\n", enabled[ in_midi->integer != 0 ] );
  Com_Printf( "port:               %d\n", in_midiport->integer );
  Com_Printf( "channel:            %d\n", in_midichannel->integer );
  Com_Printf( "current device:     %d\n", in_mididevice->integer );
  Com_Printf( "number of devices:  %d\n", s_midiInfo.numDevices );

  for ( i = 0; i < s_midiInfo.numDevices; i++ )
  {
    if ( i == Cvar_VariableValue( "in_mididevice" ) )
      Com_Printf( "***" );
    else
      Com_Printf( "..." );

    Com_Printf( "device %2d:       %s\n", i, s_midiInfo.caps[i].szPname );
    Com_Printf( "...manufacturer ID: 0x%hx\n", s_midiInfo.caps[i].wMid );
    Com_Printf( "...product ID:      0x%hx\n", s_midiInfo.caps[i].wPid );
    Com_Printf( "\n" );
  }
}

/* ---- IN_StartupMIDI  0x00462250 ----  VERIFIED */
void IN_StartupMIDI( void )
{
  int i;

  if ( !Cvar_VariableValue( "in_midi" ) )
    return;

  s_midiInfo.numDevices = midiInGetNumDevs();

  for ( i = 0; i < s_midiInfo.numDevices; i++ )
    midiInGetDevCapsA( i, &s_midiInfo.caps[i], sizeof( midiInCaps_t ) );

  if ( midiInOpen( &s_midiInfo.hMidiIn, in_mididevice->integer,
                   (unsigned long) MidiInProc, 0, 0x30000  ) )
  {
    /* Retail bug reproduced: the whole 44-byte MIDIINCAPS goes into the %s, not .szPname. */
    Com_Printf( "WARNING: could not open MIDI device %d: '%s'\n",
                in_mididevice->integer,
                s_midiInfo.caps[ (int) in_mididevice->value ] );
    return;
  }

  midiInStart( s_midiInfo.hMidiIn );
}

/* ---- IN_ShutdownMIDI  0x00462320 ----  VERIFIED */
void IN_ShutdownMIDI( void )
{
  if ( s_midiInfo.hMidiIn )
    midiInClose( s_midiInfo.hMidiIn );
  memset( &s_midiInfo, 0, sizeof( s_midiInfo ) );
}
