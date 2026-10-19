/*
 * Undecorated cdecl forwarders for imports the tree still calls by their bare
 * name.  ClipCursor, ShowCursor, GetSystemMetrics, GetWindowRect, GetCursorPos,
 * SetCursorPos, SetCapture, MessageBoxA, FreeLibrary and midiInClose are called
 * through the IAT with dllimport prototypes, as in retail; a forwarder here
 * would take the name of LINK's import thunk (retail 0x00524D4A-0x005250EE).
 */
extern int cod1_ReleaseCapture( void );
extern int cod1_timeGetTime( void );

/* ---- ReleaseCapture  no-address ---- */
int ReleaseCapture( void )             { return cod1_ReleaseCapture(); }
/* ---- timeGetTime  no-address ---- */
int timeGetTime( void )                { return cod1_timeGetTime(); }
