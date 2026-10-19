/*
 * @fidelity: likely
 */

#include "../qcommon/qcommon.h"
#include <io.h>
#include <direct.h>

/* Win32 types this unit uses (windows.h is not included here). */
typedef void *HWND;
typedef void *HMENU;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *HMONITOR;
typedef struct _GUID GUID;

/* ---- module-owned globals ---- */
char sys_cwd[256];
/* IDA's split name for 0x008E23A7 = sys_cwd + 0xFF; the getcwd helpers store
   to sys_cwd[255] (retail `mov byte ptr ds:[8E23A7h], 0`), not to this byte */
unsigned char sys_cwdLastByte;
int tmp;   /* 0x008E4E60, fastftol's fistp scratch */

/*
 * iid  0x00541810  (.rdata, const IID, 16 bytes)
 *
 * IID_IDirectDraw7 {15E65EC0-3B9C-11D2-B92F-00609797EA5B}: the interface both
 * video-memory probes request from DirectDrawCreateEx.  The bytes are the
 * retail ones, so the tree needs neither dxguid.lib nor INITGUID.  This used
 * to be an uninitialised 68-byte blob (the span up to the next retail label
 * swallowed colorBlack/color/colorDebugGreen), which DirectDrawCreateEx would
 * have rejected as an unknown interface.
 */
const unsigned char iid[16] = {
	0xC0, 0x5E, 0xE6, 0x15, 0x9C, 0x3B, 0xD2, 0x11,
	0xB9, 0x2F, 0x00, 0x60, 0x97, 0x97, 0xEA, 0x5B
};

/*
 * Win32 / DirectDraw imports used by the video-memory probe.
 *
 * windows.h and ddraw.h are not included in this unit (the few Win32 types
 * it needs are declared opaquely at the top), so the five imports are
 * declared by hand with their real __stdcall signatures.  Declared without
 * __stdcall they bound to the undecorated 'return 0' / E_FAIL placeholders in
 * qcommon/link_stubs.c (since removed), and every probe failed.
 *
 * Retail calls the USER32/KERNEL32 entries through their __imp_ slots
 * (dllimport) and the two DDRAW.dll entries through import-library thunks
 * (DirectDrawCreateEx 0x00524FBC, DirectDrawEnumerateExA 0x00524FB6), which
 * is what a plain extern declaration of a ddraw.lib export produces.
 */
#pragma comment( lib, "ddraw.lib" )

extern long __stdcall DirectDrawCreateEx( GUID *lpGuid, void **lplpDD,
                                          const void *riid, void *pUnkOuter );
extern long __stdcall DirectDrawEnumerateExA( void *lpCallback, void *lpContext,
                                              unsigned long dwFlags );

__declspec( dllimport ) HMODULE __stdcall GetModuleHandleA( const char *lpModuleName );
__declspec( dllimport ) HWND    __stdcall CreateWindowExA( unsigned long dwExStyle,
                                                           const char *lpClassName,
                                                           const char *lpWindowName,
                                                           unsigned long dwStyle,
                                                           int X, int Y,
                                                           int nWidth, int nHeight,
                                                           HWND hWndParent, HMENU hMenu,
                                                           HINSTANCE hInstance,
                                                           void *lpParam );
__declspec( dllimport ) int     __stdcall DestroyWindow( HWND hWnd );

void __cdecl CPUID( int function, unsigned int *regs );
int  IsPentium( void );

/* ---- Sys_Mkdir  0x004604E0 ----  VERIFIED */
void Sys_Mkdir( const char *path )
{
	_mkdir( path );
}

/* ---- Sys_Cwd  0x004604F0 ----  VERIFIED */
char *Sys_Cwd()
{
  getcwd(sys_cwd, 255);
  sys_cwd[255] = 0;
  return sys_cwd;
}

/* ---- Sys_DefaultCDPath  0x00460510 ----  VERIFIED */
char *Sys_DefaultCDPath()
{
  return "";
}

/* ---- Sys_DefaultBasePath  0x00460520 ----  VERIFIED */
char *Sys_DefaultBasePath()
{
  getcwd(sys_cwd, 255);
  sys_cwd[255] = 0;
  return sys_cwd;
}

/* ---- Sys_DefaultHomePath  0x00460540 ----  VERIFIED */
char *Sys_DefaultHomePath( void ) {
	return 0;
}

/* ---- Sys_DefaultInstallPath  0x00460550 ----  VERIFIED */
char *Sys_DefaultInstallPath()
{
  getcwd(sys_cwd, 255);
  sys_cwd[255] = 0;
  return sys_cwd;
}

/* ---- Sys_DirectoryHasContents_m  0x004609C0 ---- */
int Sys_DirectoryHasContents_m( const char *path )
{
	struct _finddata_t findinfo;
	long findhandle;
	char search[256];

	Com_sprintf( search, sizeof( search ), "%s\\*", path );

	findhandle = _findfirst( search, &findinfo );
	if ( findhandle == -1 ) {
		return 0;
	}

	do {
		if ( !( findinfo.attrib & _A_SUBDIR )
		  || ( Q_stricmp( findinfo.name, "." )
		    && Q_stricmp( findinfo.name, ".." )
		    && Q_stricmp( findinfo.name, "CVS" ) ) ) {
			return 1;
		}
	} while ( _findnext( findhandle, &findinfo ) != -1 );

	return 0;
}

/* ---- Sys_InitStreamThread  0x00460AB0 ----  VERIFIED */
void Sys_InitStreamThread() { ; }
/* ---- Sys_ShutdownStreamThread  0x00460AC0 ---- */
void Sys_ShutdownStreamThread() { ; }
/* ---- Sys_BeginStreamedFile  0x00460AD0 ---- */
void Sys_BeginStreamedFile() { ; }
/* ---- Sys_EndStreamedFile  0x00460AE0 ---- */
void Sys_EndStreamedFile() { ; }

/* ---- FS_FReadCallback_m  0x00460AF0 ----  VERIFIED */
int __cdecl FS_FReadCallback_m(fileHandle_t f, void *buffer, int size, int count)
{
  return FS_Read(buffer, count * size, f);
}

/* ---- FS_FSeekCallback_m  0x00460B10 ----  VERIFIED */
/* Unreferenced in retail too.  offset/f arrive in ecx/edx under LTCG but origin
   is read from the stack ([esp+4]): a 64-bit parameter is never enregistered. */
int FS_FSeekCallback_m( int offset, fileHandle_t f, __int64 origin )
{
	return FS_Seek( f, offset, (int)origin );
}

/* ---- FS_FTellCallback_m  0x00460B20 ---- */
/* Unreferenced in retail too (`or eax,-1; ret`): the stream "tell" callback
   that sits between the seek callback and the critical-section stubs. */
int FS_FTellCallback_m( fileHandle_t f )
{
	return -1;
}

/* ---- Sys_EnterCriticalSection  0x00460B30 ---- */
void Sys_EnterCriticalSection() { ; }
/* ---- Sys_LeaveCriticalSection  0x00460B40 ---- */
void Sys_LeaveCriticalSection() { ; }

/* ---- RoundFloatToInt_m  0x00460B50 ----  VERIFIED */
int __cdecl RoundFloatToInt_m( float f )
{
	int i;
	double half = 0.4999999990686774;

	__asm {
		fld     f
		fadd    half
		fistp   i
	}

	return i;
}

/*
 * Minimal IDirectDraw7 shim.  This file cannot include <ddraw.h> (it
 * hand-declares DirectDrawCreateEx above and passes the interface GUID as the
 * raw `iid` bytes), so the few COM methods used to probe video
 * memory are described by a local vtable whose slot offsets are pinned to the
 * retail IDirectDraw7 layout: Release at 8, SetCooperativeLevel at 80,
 * GetAvailableVidMem at 92.  Only those three are named; the rest are padding.
 */
typedef struct ddSurfaceCaps_s {
	unsigned long caps;    /* DDSCAPS.dwCaps (DDSCAPS_VIDEOMEMORY = 0x4000) */
	unsigned long caps2;
	unsigned long caps3;
	unsigned long caps4;
} ddSurfaceCaps_t;

struct ddDraw_s;

typedef int ( __stdcall *ddReleaseFn_t )( struct ddDraw_s *self );
typedef int ( __stdcall *ddSetCooperativeLevelFn_t )( struct ddDraw_s *self, HWND wnd, int flags );
typedef int ( __stdcall *ddGetAvailableVidMemFn_t )( struct ddDraw_s *self, ddSurfaceCaps_t *caps, unsigned long *total, unsigned long *freeMem );

typedef struct ddDrawVtbl_s {
	void                       *_iunknown[2];       /* QueryInterface (0), AddRef (4) */
	ddReleaseFn_t               Release;            /* 8 */
	void                       *_reserved0C[17];    /* 0x0C .. 0x4C */
	ddSetCooperativeLevelFn_t   SetCooperativeLevel;/* 80 */
	void                       *_reserved54[2];     /* 0x54, 0x58 */
	ddGetAvailableVidMemFn_t    GetAvailableVidMem; /* 92 */
} ddDrawVtbl_t;

typedef struct ddDraw_s {
	ddDrawVtbl_t *vtbl;
} ddDraw_t;

COD1_STATIC_ASSERT( offsetof( ddDrawVtbl_t, Release ) == 8, "IDirectDraw7::Release vtable slot" );
COD1_STATIC_ASSERT( offsetof( ddDrawVtbl_t, SetCooperativeLevel ) == 80, "IDirectDraw7::SetCooperativeLevel vtable slot" );
COD1_STATIC_ASSERT( offsetof( ddDrawVtbl_t, GetAvailableVidMem ) == 92, "IDirectDraw7::GetAvailableVidMem vtable slot" );

/* ---- Sys_GetDDrawVidMem_m  0x00460E70 ----  VERIFIED */
/* Retail takes the GUID in ecx plus a stacked driver-description string
   ("NULL device" from 0x4614F1, lpDriverDescription from the enum callback
   0x460EF8) that the body never reads; the dead parameter is kept so the
   callers pass the same arguments. */
int __cdecl Sys_GetDDrawVidMem_m( GUID *lpGuid, const char *driverDesc )
{
	ddDraw_t       *lpDD;
	ddSurfaceCaps_t caps;
	unsigned long           totalMem;
	unsigned long           freeMem;
	int             hr;

	if ( DirectDrawCreateEx( lpGuid, (void **)&lpDD, iid, 0 ) < 0 ) {
		return 0;
	}

	memset( &caps, 0, sizeof( caps ) );
	caps.caps = 0x4000;   /* DDSCAPS_VIDEOMEMORY */

	hr = lpDD->vtbl->GetAvailableVidMem( lpDD, &caps, &totalMem, &freeMem );
	lpDD->vtbl->Release( lpDD );

	return ( hr < 0 ) ? 0 : (int)totalMem;
}

/* ---- Sys_DDrawEnumVidMemCallback_m  0x00460EF0 ----  VERIFIED */
int __stdcall Sys_DDrawEnumVidMemCallback_m(GUID *lpGuid, char *lpDriverDescription, char *lpDriverName, unsigned int *maxVidMem, HMONITOR hMonitor)
{
  unsigned int vidMem;

  if ( !hMonitor )
  {
    vidMem = Sys_GetDDrawVidMem_m(lpGuid, lpDriverDescription);
    if ( *maxVidMem < vidMem )
      *maxVidMem = vidMem;
  }
  return 1;
}

/* ---- Sys_QueryDDrawVidMem_m  0x00460F20 ----  VERIFIED */
/* Same unused driver-description parameter as Sys_GetDDrawVidMem_m
   ("NULL device" from 0x461520, lpDriverDescription from 0x461000). */
unsigned int __cdecl Sys_QueryDDrawVidMem_m( GUID *lpGuid, const char *driverDesc )
{
	ddDraw_t       *lpDD;
	HWND            hWnd;
	ddSurfaceCaps_t caps;
	unsigned long           totalMem;
	unsigned long           freeMem;
	int             hr;

	if ( DirectDrawCreateEx( lpGuid, (void **)&lpDD, iid, 0 ) < 0 ) {
		return 0;
	}

	hWnd = CreateWindowExA( 0, "static", "dummy", 0, 0, 0, 1, 1, 0, 0, GetModuleHandleA( 0 ), 0 );

	if ( !hWnd ) {
		lpDD->vtbl->Release( lpDD );
		return 0;
	}

	if ( lpDD->vtbl->SetCooperativeLevel( lpDD, hWnd, 17 ) < 0 ) {
		DestroyWindow( hWnd );
		lpDD->vtbl->Release( lpDD );
		return 0;
	}

	memset( &caps, 0, sizeof( caps ) );
	caps.caps = 0x4000;   /* DDSCAPS_VIDEOMEMORY */

	hr = lpDD->vtbl->GetAvailableVidMem( lpDD, &caps, &totalMem, &freeMem );

	DestroyWindow( hWnd );
	lpDD->vtbl->Release( lpDD );

	return ( hr < 0 ) ? 0 : totalMem;
}

/* ---- Sys_DDrawEnumQueryCallback_m  0x00461000 ----  VERIFIED */
int __stdcall Sys_DDrawEnumQueryCallback_m(GUID *lpGuid, char *lpDriverDescription, char *lpDriverName, unsigned int *maxVidMem, HMONITOR hMonitor)
{
  unsigned int vidMem;

  vidMem = Sys_QueryDDrawVidMem_m(lpGuid, lpDriverDescription);
  if ( *maxVidMem < vidMem )
    *maxVidMem = vidMem;
  return 1;
}

typedef struct {
	const char *renderer;
	int         megs;
} sys_vidMemEntry_t;

/* ---- Sys_VideoRamFromRenderer  0x00461030 ----  VERIFIED */
/* Retail has no NULL check on renderer (ebx goes straight into _strnicmp
   at 0x46135C), and its only caller Sys_GetVideoRam 0x461390 passes
   glGetString(GL_RENDERER) unchecked, so a NULL string faults as in retail. */
int __cdecl Sys_VideoRamFromRenderer( const char *renderer )
{
	sys_vidMemEntry_t table[] = {
		{ "Radeon 7200",              64 },
		{ "Radeon 7500",              64 },
		{ "Radeon 8500",              64 },
		{ "Radeon 9000",              64 },
		{ "Radeon 9100",              64 },
		{ "Radeon 9200",              64 },
		{ "Radeon 9500",              64 },
		{ "Radeon 9600",             128 },
		{ "Radeon 9700",             128 },
		{ "Radeon 9800",             128 },
		{ "Radeon",                   32 },
		{ "GeForce 256",              32 },
		{ "GeForce2 GTS",             32 },
		{ "GeForce2 MX",              32 },
		{ "GeForce2",                 32 },
		{ "GeForce3",                 64 },
		{ "GeForce4 420 Go",          64 },
		{ "GeForce4 4200 Go",         64 },
		{ "GeForce4 440 Go",          64 },
		{ "GeForce4 460 Go",          64 },
		{ "GeForce4 MX 420",          64 },
		{ "GeForce4 MX 440",          64 },
		{ "GeForce4 MX 460",          64 },
		{ "GeForce4 MX",              64 },
		{ "GeForce4 Ti 4200",         64 },
		{ "GeForce4 Ti 4400",        128 },
		{ "GeForce4 Ti 4600",        128 },
		{ "GeForce4 Ti 4800",        128 },
		{ "GeForce4",                 64 },
		{ "GeForce FX 5200",          64 },
		{ "GeForce FX 5600",         128 },
		{ "GeForce FX 5800 Ultra",   128 },
		{ "GeForce FX 5800",         128 },
		{ "GeForce FX 5900 Ultra",   128 },
		{ "GeForce FX 5900",         128 },
		{ "Quadro FX 1000",          128 },
		{ "Quadro FX",               128 },
		{ "Quadro4 500",              64 },
		{ "Quadro4 700",              64 },
		{ "Quadro4 900",              64 },
		{ "Quadro4",                  64 },
		{ "Quadro2 Pro",              32 },
		{ "Quadro2 MXR",              32 },
		{ "Quadro2",                  32 },
		{ "Quadro DCC",               32 },
		{ "Quadro",                   32 },
		{ "Matrox ICD for Parhelia", 128 }
	};
	unsigned int i;

	for ( i = 0; i < sizeof( table ) / sizeof( table[0] ); i++ ) {
		if ( !_strnicmp( table[i].renderer, renderer, strlen( table[i].renderer ) ) ) {
			return table[i].megs << 20;
		}
	}

	return 0;
}

/* ---- Sys_DetectVideoRamMegs_m  0x004614F0 ----  VERIFIED */
unsigned int Sys_DetectVideoRamMegs_m( void )
{
	unsigned int bytes;
	unsigned int megs;
	int          pow2;

	bytes = Sys_GetDDrawVidMem_m( 0, "NULL device" );

	if ( !bytes ) {
		DirectDrawEnumerateExA( (void *) Sys_DDrawEnumVidMemCallback_m, &bytes, 0 );
	}
	if ( !bytes ) {
		bytes = Sys_QueryDDrawVidMem_m( NULL, "NULL device" );
	}
	if ( !bytes ) {
		DirectDrawEnumerateExA( (void *) Sys_DDrawEnumQueryCallback_m, &bytes, 0 );
	}
	if ( !bytes ) {
		bytes = Sys_GetVideoRam();
	}
	if ( !bytes ) {
		return 0;
	}

	megs = ( ( bytes - 1 ) >> 20 ) + 1;

	for ( pow2 = 1; pow2 < megs; pow2 <<= 1 ) {
		;
	}
	if ( pow2 - megs > 0x20 ) {
		return pow2 >> 1;
	}
	return pow2;
}

/* ---- Sys_HasSSE_m  0x00461590 ----  VERIFIED */
/* CPUID.1:EDX bit 25, then an SSE instruction as an OS-support probe before
   the result is set; one inline-asm block in retail.  The #UD on an OS without
   SSE support is unhandled: there is no SEH frame in 0x461590,
   Sys_DetectHardware_m 0x4626E0 or WinMain 0x4640B0. */
__declspec(noinline) int Sys_HasSSE_m( void )
{
	int hasSSE = 0;

	__asm {
		mov   eax, 1
		cpuid
		xor   eax, eax
		test  edx, 2000000h
		jz    noSSE
		xorps xmm0, xmm0
		mov   eax, 1
		mov   hasSSE, eax
noSSE:
	}
	return hasSSE;
}

/* ---- fastftol  0x00465A00 ----  VERIFIED */
int fastftol( float f )
{
	__asm fld   f
	__asm fistp tmp
	return tmp;
}

/* ---- Sys_SnapVector  0x00465A10 ----  VERIFIED */
void Sys_SnapVector( float *v )
{
	int   i;
	float f;

	f = *v;
	__asm fld   f
	__asm fistp i
	*v = i;
	v++;
	f = *v;
	__asm fld   f
	__asm fistp i
	*v = i;
	v++;
	f = *v;
	__asm fld   f
	__asm fistp i
	*v = i;
}

/* Retail 0x465A60..0x465C1A is unoptimized code (ebp frames, and/test/jmp
   returns): the stock Q3 "disable all optimizations" bracket. */
#pragma optimize( "", off )

/* ---- CPUID  0x00465A60 ----  VERIFIED */
void __cdecl CPUID( int func, unsigned int *regs )
{
	unsigned int regEAX, regEBX, regECX, regEDX;

	__asm pusha
	__asm mov eax, func
	__asm _emit 0x0F
	__asm _emit 0xA2
	__asm mov regEAX, eax
	__asm mov regEBX, ebx
	__asm mov regECX, ecx
	__asm mov regEDX, edx
	__asm popa

	regs[0] = regEAX;
	regs[1] = regEBX;
	regs[2] = regECX;
	regs[3] = regEDX;
}

/* ---- IsPentium  0x00465AB0 ----  VERIFIED */
/* Retail 0x465AB0 is the stock Q3 test: it flips the EFLAGS ID bit (clear if set,
   set if clear), reads it back, and does not restore the original EFLAGS. */
int IsPentium( void )
{
	__asm {
		pushfd
		pop  eax
		test eax, 200000h
		jz   set21
		and  eax, 0FFDFFFFFh
		push eax
		popfd
		pushfd
		pop  eax
		test eax, 200000h
		jz   good
		jmp  err
set21:
		or   eax, 200000h
		push eax
		popfd
		pushfd
		pop  eax
		test eax, 200000h
		jnz  good
		jmp  err
	}

err:
	return 0;
good:
	return 1;
}

/* ---- Is3DNOW  0x00465AF0 ----  VERIFIED */
int Is3DNOW( void )
{
	unsigned int regs[4];
	char         pstring[16];

	CPUID( 0, (unsigned int *)pstring );

	CPUID( 0x80000000, regs );
	if ( regs[0] < 0x80000000u ) {
		return 0;
	}

	CPUID( 0x80000001, regs );
	if ( regs[3] & 0x80000000u ) {
		return 1;
	}
	return 0;
}

/* ---- IsKNI  0x00465B70 ----  VERIFIED */
int IsKNI( void )
{
	unsigned int regs[4];

	CPUID( 1, regs );
	if ( regs[3] & 0x02000000u ) {
		return 1;
	}
	return 0;
}

/* ---- IsMMX  0x00465BA0 ----  VERIFIED */
int IsMMX( void )
{
	unsigned int regs[4];

	CPUID( 1, regs );
	if ( regs[3] & 0x00800000u ) {
		return 1;
	}
	return 0;
}

/* ---- Sys_GetProcessorId  0x00465BD0 ----  VERIFIED */
int Sys_GetProcessorId( void )
{
	if ( !IsPentium() ) {
		return 32;
	}
	if ( !IsMMX() ) {
		return 33;
	}
	if ( Is3DNOW() ) {
		return 48;
	}
	if ( IsKNI() ) {
		return 35;
	}
	return 34;
}

#pragma optimize( "", on )

/* Sys_GetCurrentUser (0x00465C20) is defined in win_platform.c. */
