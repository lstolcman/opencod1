/*
 * @fidelity: verified
 * @fidelity-default: verified
 */

#include "../qcommon/qcommon.h"
#include "cl_records.h"
#include "cl_local.h"
#include "cl_cinematic.h"
signed __int32 CL_handle = -1;   /* 0x0057C118, retail .data (file-backed) = 0xFFFFFFFF */
signed __int32 currentHandle = -1;   /* 0x0057C114, retail .data (file-backed) = 0xFFFFFFFF */

/* cin_cache[MAX_VIDEO_HANDLES], 0x00877728, 7424 bytes. Defined at the foot of this file. */
extern cinematic_t cinTable[MAX_VIDEO_HANDLES];

/* The five YUV lookup tables, RTCW cl_cin.c:81-85.  Identified by the coefficient
 * each one is filled with in ROQ_GenYUVTables (cl_cin.c:569-573) and by which of
 * y/u/v indexes it in yuv_to_rgb (cl_cin.c:622-626).  The declared extents of
 * VR/YY/UB are not the real 256 longs; they are three consecutive 1024-byte
 * tables running 0x879428..0x87A028. */
int ROQ_UG_tab[256];   /* 0x007EEF28, 1024 bytes */
int ROQ_VG_tab[256];   /* 0x007EF328, 1024 bytes */
int ROQ_VR_tab[768];   /* 0x00879428, 3072 bytes */
int ROQ_YY_tab[512];   /* 0x00879828, 2048 bytes */
int ROQ_UB_tab[256];   /* 0x00879C28, 1024 bytes */
extern glconfig_t glConfig;                 /* 0x016C3A60 */
extern cvar_t *r_inGameVideo;               /* 0x01617318 */

/* q_shared.c's `vec4_t colorBlack = {0,0,0,1}` at 0x00541820 -- retail holds
 * 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F there, with colorRed and
 * colorGreen following at +0x10 and +0x20. */
extern unsigned char colorBlack[];

#undef  re_SetColor
#define re_SetColor ( *(void (**)( const float *rgba ))&re.SetColor )   /* +0x6C */
#undef  re_DrawStretchPic
#define re_DrawStretchPic ( *(void (**)( float x, float y, float w, float h, float s1, float t1, float s2, float t2, int hShader ))&re.StretchPic )   /* +0x70 */
#undef  re_DrawStretchRawPtr
#define re_DrawStretchRawPtr ( *(void (**)( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, int dirty ))&re.StretchRaw )   /* +0x80 */
#undef  re_UploadCinematicPtr
#define re_UploadCinematicPtr ( *(void (**)( int w, int h, int cols, int rows, const byte *data, int client, int dirty ))&re.UploadCinematic )   /* +0x84 */

/* the cinematic scratch block, 0x0057E910 (RTCW cl_cin.c `static cinematics_t cin`) */
cinematicScratch_t cin;

/* cin.sqrTable, 0x0078E910. One short[256], filled as two halves by RllSetupTable; cin carries the storage and cl_records.h aliases the fifteen extra lane names onto it. */


/* cin.qStatus[2][32768], 0x0078EF10 / 0x007AEF10: one array as in Q3's cin_cache (as two
 * separate globals VC7 turned setupQuad's end-of-quad loop into two memsets). */
int             cin_qStatus0[32768];        /* 0x0078EF10 cin.qStatus[0]; unreferenced (the code uses cin.qStatus) */
int cin_qStatus1[32768];                    /* cin.qStatus[1]; unreferenced */

unsigned short  cin_vq4[65536];          /* 0x007CEF28 vq4, RTCW vq4[256*64*4] */
unsigned short  cin_vq8[262144];         /* 0x007EF728 vq8, RTCW vq8[256*256*4] */
/* RTCW cl_cin.c:86 `vq2[256*16*4]` -- 32768 bytes at 0x0086F728, running right up
 * to cinTable, with cin_vq8 immediately below it. */
unsigned short cin_vq2[16384];

extern int CL_ScaledMilliseconds();
extern int Con_Close();
extern int FS_FOpenFileRead_Internal();
extern int Hunk_FreeTempMemory();
extern int MSS_EndRawSamples();
extern void MSS_RawSamples( int samples, int rate, int width, int channels, void *data );
extern int MSS_RawSamplesTime();
extern int MSS_StopSounds();
extern int SCR_AdjustFrom640();

int __cdecl CIN_StopCinematic( int handle );

void __cdecl RoQ_init( void );
void __cdecl CIN_SetExtents( int handle, int x, int y, int w, int h );
void __cdecl CIN_SetLooping( int handle, qboolean loop );
void __cdecl SCR_RunCinematic( void );
void __cdecl SCR_StopCinematic( void );

extern void MSS_FadeAllSounds( float target, int msec );

/* ---- CIN_HandleForVideo  0x00405010 ----  VERIFIED */
int __cdecl CIN_HandleForVideo( void )
{
	int i;

	for ( i = 0; i < MAX_VIDEO_HANDLES; i++ ) {
		if ( !cinTable[i].fileName[0] ) {
			return i;
		}
	}

	Com_Error( ERR_DROP, "\x15" "CIN_HandleForVideo: none free" );
	return 0;
}

/* ---- RllSetupTable  0x00405040 ----  VERIFIED */
void __cdecl RllSetupTable( void )
{
	int z;

	for ( z = 0; z < 128; z++ ) {
		cin_sqrTable[z]       = (short)( z * z );
		cin_sqrTable[z + 128] = (short)( -cin_sqrTable[z] );
	}
}

/* ---- RllDecodeMonoToMono  0x00405110 ----  VERIFIED */
int __cdecl RllDecodeMonoToMono( byte *from, short *to, unsigned int size,
								 char signedOutput, unsigned short flag )
{
	unsigned int z;
	short prev;

	if ( signedOutput ) {
		prev = flag - 0x8000;
	} else {
		prev = flag;
	}

	for ( z = 0; z < size; z++ ) {
		prev = to[z] = (short)( prev + cin_sqrTable[from[z]] );
	}

	return size;
}

/* ---- RllDecodeMonoToStereo  0x00405150 ----  VERIFIED */
int __cdecl RllDecodeMonoToStereo( byte *from, short *to, unsigned int size,
								   char signedOutput, unsigned short flag )
{
	unsigned int z;
	short prev;

	if ( signedOutput ) {
		prev = flag - 0x8000;
	} else {
		prev = flag;
	}

	for ( z = 0; z < size; z++ ) {
		prev = (short)( prev + cin_sqrTable[from[z]] );
		to[z * 2 + 0] = to[z * 2 + 1] = (short)prev;
	}

	return size;
}

/* ---- RllDecodeStereoToStereo  0x00405190 ----  VERIFIED */
int __cdecl RllDecodeStereoToStereo( byte *from, short *to, unsigned int size,
									 char signedOutput, unsigned short flag )
{
	unsigned int z;
	byte *zz = from;
	short prevL, prevR;

	if ( signedOutput ) {
		prevL = ( flag & 0xff00 ) - 0x8000;
		prevR = ( ( flag & 0x00ff ) << 8 ) - 0x8000;
	} else {
		prevL = flag & 0xff00;
		prevR = ( flag & 0x00ff ) << 8;
	}

	for ( z = 0; z < size; z += 2 ) {
		prevL = (short)( prevL + cin_sqrTable[*zz++] );
		prevR = (short)( prevR + cin_sqrTable[*zz++] );
		to[z + 0] = (short)prevL;
		to[z + 1] = (short)prevR;
	}

	return ( size >> 1 );
}

/* ---- RllDecodeStereoToMono  0x00405220 ----  VERIFIED */
int __cdecl RllDecodeStereoToMono( byte *from, short *to, unsigned int size,
								   char signedOutput, unsigned short flag )
{
	unsigned int z;
	short prevL, prevR;

	if ( signedOutput ) {
		prevL = ( flag & 0xff00 ) - 0x8000;
		prevR = ( ( flag & 0x00ff ) << 8 ) - 0x8000;
	} else {
		prevL = flag & 0xff00;
		prevR = ( flag & 0x00ff ) << 8;
	}

	for ( z = 0; z < size; z++ ) {
		prevL = (short)( prevL + cin_sqrTable[from[z * 2 + 0]] );
		prevR = (short)( prevR + cin_sqrTable[from[z * 2 + 1]] );
		to[z] = (short)( ( prevL + prevR ) / 2 );
	}

	return size;
}

/* ---- move8_32  0x004052C0 ----  VERIFIED */
/* move8_32 / move4_32 / blit8_32 / blit4_32 / blit2_32 copy pixel cells through
 * unsigned __int64 pointers, not RTCW's double pointers.  Retail copies with plain
 * dword moves (bit-exact); a double copy under /arch:IA32 goes through fld/fstp,
 * which quiets 0xFFBBGGRR pixel pairs that form a signalling NaN. */
void __cdecl move8_32( byte *src, byte *dst, int spl )
{
	unsigned __int64 *dsrc, *ddst;
	int dspl;

	dsrc = (unsigned __int64 *)src;
	ddst = (unsigned __int64 *)dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
}

/* ---- move4_32  0x00405490 ----  VERIFIED */
void __cdecl move4_32( byte *src, byte *dst, int spl )
{
	unsigned __int64 *dsrc, *ddst;
	int dspl;

	dsrc = (unsigned __int64 *)src;
	ddst = (unsigned __int64 *)dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
	dsrc += dspl;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
}

/* ---- blit8_32  0x00405510 ----  VERIFIED */
void __cdecl blit8_32( byte *src, byte *dst, int spl )
{
	unsigned __int64 *dsrc, *ddst;
	int dspl;

	dsrc = (unsigned __int64 *)src;
	ddst = (unsigned __int64 *)dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += 4;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += 4;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += 4;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += 4;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += 4;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += 4;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
	dsrc += 4;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];	ddst[2] = dsrc[2];	ddst[3] = dsrc[3];
}

/* ---- blit4_32  0x004056D0 ----  VERIFIED */
void __cdecl blit4_32( byte *src, byte *dst, int spl )
{
	unsigned __int64 *dsrc, *ddst;
	int dspl;

	dsrc = (unsigned __int64 *)src;
	ddst = (unsigned __int64 *)dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
	dsrc += 2;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
	dsrc += 2;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
	dsrc += 2;	ddst += dspl;
	ddst[0] = dsrc[0];	ddst[1] = dsrc[1];
}

/* ---- blit2_32  0x00405750 ----  VERIFIED */
/* RTCW's double pointers under VC7: it still copies with dword moves, and the
 * double it inlines into blitVQQuad32fs gives that caller retail's 8-aligned frame. */
#ifdef COD1_VC7
typedef double cinpix2_t;
#else
typedef unsigned __int64 cinpix2_t;
#endif
void __cdecl blit2_32( byte *src, byte *dst, int spl )
{
	cinpix2_t *dsrc, *ddst;
	int dspl;

	dsrc = (cinpix2_t *)src;
	ddst = (cinpix2_t *)dst;
	dspl = spl >> 3;

	ddst[0] = dsrc[0];
	ddst[dspl] = dsrc[1];
}

/* ---- blitVQQuad32fs  0x00405770 ----  VERIFIED */
void __cdecl blitVQQuad32fs( byte **status, unsigned char *data )
{
	unsigned short newd, celdata, code;
	unsigned int index, i;
	int spl;

	newd    = 0;
	celdata = 0;
	index   = 0;

	spl = cinTable[currentHandle].samplesPerLine;

	do {
		if ( !newd ) {
			newd = 7;
			celdata = data[0] + data[1] * 256;
			data += 2;
		} else {
			newd--;
		}

		code = (unsigned short)( celdata & 0xc000 );
		celdata <<= 2;

		switch ( code ) {
			case 0x8000:
				blit8_32( (byte *)&cin_vq8[( *data ) * 128], status[index], spl );
				data++;
				index += 5;
				break;
			case 0xc000:
				index++;
				for ( i = 0; i < 4; i++ ) {
					if ( !newd ) {
						newd = 7;
						celdata = data[0] + data[1] * 256;
						data += 2;
					} else {
						newd--;
					}

					code = (unsigned short)( celdata & 0xc000 );
					celdata <<= 2;

					switch ( code ) {
						case 0x8000:
							blit4_32( (byte *)&cin_vq4[( *data ) * 32], status[index], spl );
							data++;
							break;
						case 0xc000:
							blit2_32( (byte *)&cin_vq2[( *data ) * 8], status[index], spl );
							data++;
							blit2_32( (byte *)&cin_vq2[( *data ) * 8], status[index] + 8, spl );
							data++;
							blit2_32( (byte *)&cin_vq2[( *data ) * 8], status[index] + spl * 2, spl );
							data++;
							blit2_32( (byte *)&cin_vq2[( *data ) * 8], status[index] + spl * 2 + 8, spl );
							data++;
							break;
						case 0x4000:
							move4_32( status[index] + cin_mcomp[( *data )], status[index], spl );
							data++;
							break;
					}
					index++;
				}
				break;
			case 0x4000:
				move8_32( status[index] + cin_mcomp[( *data )], status[index], spl );
				data++;
				index += 5;
				break;
			case 0x0000:
				index += 5;
				break;
		}
	} while ( status[index] != NULL );
}

/* ---- ROQ_GenYUVTables  0x004059D0 ----  VERIFIED */
void __cdecl ROQ_GenYUVTables( void )
{
  float t_ub, t_vr, t_ug, t_vg;
  int i;
  float x;

  t_ub = ( 1.77200f / 2.0f ) * (float)( 1 << 6 ) + 0.5f;
  t_vr = ( 1.40200f / 2.0f ) * (float)( 1 << 6 ) + 0.5f;
  t_ug = ( 0.34414f / 2.0f ) * (float)( 1 << 6 ) + 0.5f;
  t_vg = ( 0.71414f / 2.0f ) * (float)( 1 << 6 ) + 0.5f;
  for ( i = 0; i < 256; i++ )
  {
    x = (float)( 2 * i - 255 );

    /* retail 0x4059E4..0x405A32: float constants (fld/fmul/fadd dword ptr) */
    ROQ_UB_tab[i] = (int)( ( t_ub * x ) + ( 1 << 5 ) );
    ROQ_VR_tab[i] = (int)( ( t_vr * x ) + ( 1 << 5 ) );
    ROQ_UG_tab[i] = (int)( ( -t_ug * x ) );
    ROQ_VG_tab[i] = (int)( ( -t_vg * x ) + ( 1 << 5 ) );
    ROQ_YY_tab[i] = ( i << 6 ) | ( i >> 2 );
  }
}

/* ---- yuv_to_rgb  0x00405A70 ----  VERIFIED */
int __cdecl yuv_to_rgb( int y, int u, int v )
{
  int r, g, b, YY;

  YY = ROQ_YY_tab[y];

  r = ( YY + ROQ_VR_tab[v] ) >> 9;
  g = ( YY + ROQ_UG_tab[u] + ROQ_VG_tab[v] ) >> 8;
  b = ( YY + ROQ_UB_tab[u] ) >> 9;

  if ( r < 0 )
    r = 0;
  if ( g < 0 )
    g = 0;
  if ( b < 0 )
    b = 0;
  if ( r > 31 )
    r = 31;
  if ( g > 63 )
    g = 63;
  if ( b > 31 )
    b = 31;

  return ( r << 11 ) + ( g << 5 ) + b;
}

/* ---- yuv_to_rgb24  0x00405AF0 ----  VERIFIED */
int __cdecl yuv_to_rgb24( int y, int u, int v )
{
  int r, g, b, YY;

  YY = ROQ_YY_tab[y];

  r = ( YY + ROQ_VR_tab[v] ) >> 6;
  g = ( YY + ROQ_UG_tab[u] + ROQ_VG_tab[v] ) >> 6;
  b = ( YY + ROQ_UB_tab[u] ) >> 6;

  if ( r < 0 )
    r = 0;
  if ( g < 0 )
    g = 0;
  if ( b < 0 )
    b = 0;
  if ( r > 255 )
    r = 255;
  if ( g > 255 )
    g = 255;
  if ( b > 255 )
    b = 255;

  return r | ( g << 8 ) | ( b << 16 ) | ( 255 << 24 );
}

#define VQ2TO4( a, b, c, d ) { \
	*c++ = a[0];    \
	*d++ = a[0];    \
	*d++ = a[0];    \
	*c++ = a[1];    \
	*d++ = a[1];    \
	*d++ = a[1];    \
	*c++ = b[0];    \
	*d++ = b[0];    \
	*d++ = b[0];    \
	*c++ = b[1];    \
	*d++ = b[1];    \
	*d++ = b[1];    \
	*d++ = a[0];    \
	*d++ = a[0];    \
	*d++ = a[1];    \
	*d++ = a[1];    \
	*d++ = b[0];    \
	*d++ = b[0];    \
	*d++ = b[1];    \
	*d++ = b[1];    \
	a += 2; b += 2; }

#define VQ2TO2( a, b, c, d ) { \
	*c++ = a[0];    \
	*d++ = a[0];    \
	*d++ = a[0];    \
	*c++ = b[0];    \
	*d++ = b[0];    \
	*d++ = b[0];    \
	*d++ = a[0];    \
	*d++ = a[0];    \
	*d++ = b[0];    \
	*d++ = b[0];    \
	a++; b++; }

/* ---- decodeCodeBook  0x00405B70 ----  VERIFIED */
void __cdecl decodeCodeBook( byte *input, unsigned short roq_flags )
{
	long i, j, two, four;
	unsigned short *aptr, *bptr, *cptr, *dptr;
	long y0, y1, y2, y3, cr, cb;
	byte *bbptr, *baptr, *bcptr, *bdptr;
	unsigned int *iaptr, *ibptr, *icptr, *idptr;

	if ( !roq_flags ) {
		two = four = 256;
	} else {
		two = roq_flags >> 8;
		if ( !two ) {
			two = 256;
		}
		four = roq_flags & 0xff;
	}

	four *= 2;

	bptr = (unsigned short *)cin_vq2;

	if ( !cinTable[currentHandle].half ) {
		if ( !cinTable[currentHandle].smootheddouble ) {
			/* normal height */
			if ( cinTable[currentHandle].samplesPerPixel == 2 ) {
				for ( i = 0; i < two; i++ ) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*bptr++ = (unsigned short)yuv_to_rgb( y0, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( y1, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( y2, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( y3, cr, cb );
				}

				cptr = (unsigned short *)cin_vq4;
				dptr = (unsigned short *)cin_vq8;

				for ( i = 0; i < four; i++ ) {
					aptr = (unsigned short *)cin_vq2 + ( *input++ ) * 4;
					bptr = (unsigned short *)cin_vq2 + ( *input++ ) * 4;
					for ( j = 0; j < 2; j++ ) {
						VQ2TO4( aptr, bptr, cptr, dptr );
					}
				}
			} else if ( cinTable[currentHandle].samplesPerPixel == 4 ) {
				ibptr = (unsigned int *)bptr;
				for ( i = 0; i < two; i++ ) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*ibptr++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr = (unsigned int *)cin_vq4;
				idptr = (unsigned int *)cin_vq8;

				for ( i = 0; i < four; i++ ) {
					iaptr = (unsigned int *)cin_vq2 + ( *input++ ) * 4;
					ibptr = (unsigned int *)cin_vq2 + ( *input++ ) * 4;
					for ( j = 0; j < 2; j++ ) {
						VQ2TO4( iaptr, ibptr, icptr, idptr );
					}
				}
			} else if ( cinTable[currentHandle].samplesPerPixel == 1 ) {
				bbptr = (byte *)bptr;
				for ( i = 0; i < two; i++ ) {
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input++];
					*bbptr++ = cinTable[currentHandle].gray[*input]; input += 3;
				}

				bcptr = (byte *)cin_vq4;
				bdptr = (byte *)cin_vq8;

				for ( i = 0; i < four; i++ ) {
					baptr = (byte *)cin_vq2 + ( *input++ ) * 4;
					bbptr = (byte *)cin_vq2 + ( *input++ ) * 4;
					for ( j = 0; j < 2; j++ ) {
						VQ2TO4( baptr, bbptr, bcptr, bdptr );
					}
				}
			}
		} else {
			/* double height, smoothed */
			if ( cinTable[currentHandle].samplesPerPixel == 2 ) {
				for ( i = 0; i < two; i++ ) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*bptr++ = (unsigned short)yuv_to_rgb( y0, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( y1, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( y2, cr, cb );
					*bptr++ = (unsigned short)yuv_to_rgb( y3, cr, cb );
				}

				cptr = (unsigned short *)cin_vq4;
				dptr = (unsigned short *)cin_vq8;

				for ( i = 0; i < four; i++ ) {
					aptr = (unsigned short *)cin_vq2 + ( *input++ ) * 8;
					bptr = (unsigned short *)cin_vq2 + ( *input++ ) * 8;
					for ( j = 0; j < 2; j++ ) {
						VQ2TO4( aptr, bptr, cptr, dptr );
						VQ2TO4( aptr, bptr, cptr, dptr );
					}
				}
			} else if ( cinTable[currentHandle].samplesPerPixel == 4 ) {
				ibptr = (unsigned int *)bptr;
				for ( i = 0; i < two; i++ ) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input++;
					cr = (long)*input++;
					cb = (long)*input++;
					*ibptr++ = yuv_to_rgb24( y0, cr, cb );
					*ibptr++ = yuv_to_rgb24( y1, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( ( y0 * 3 ) + y2 ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( ( y1 * 3 ) + y3 ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( y0 + ( y2 * 3 ) ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( ( y1 + ( y3 * 3 ) ) / 4, cr, cb );
					*ibptr++ = yuv_to_rgb24( y2, cr, cb );
					*ibptr++ = yuv_to_rgb24( y3, cr, cb );
				}

				icptr = (unsigned int *)cin_vq4;
				idptr = (unsigned int *)cin_vq8;

				for ( i = 0; i < four; i++ ) {
					iaptr = (unsigned int *)cin_vq2 + ( *input++ ) * 8;
					ibptr = (unsigned int *)cin_vq2 + ( *input++ ) * 8;
					for ( j = 0; j < 2; j++ ) {
						VQ2TO4( iaptr, ibptr, icptr, idptr );
						VQ2TO4( iaptr, ibptr, icptr, idptr );
					}
				}
			} else if ( cinTable[currentHandle].samplesPerPixel == 1 ) {
				bbptr = (byte *)bptr;
				for ( i = 0; i < two; i++ ) {
					y0 = (long)*input++;
					y1 = (long)*input++;
					y2 = (long)*input++;
					y3 = (long)*input; input += 3;
					*bbptr++ = cinTable[currentHandle].gray[y0];
					*bbptr++ = cinTable[currentHandle].gray[y1];
					*bbptr++ = cinTable[currentHandle].gray[( ( y0 * 3 ) + y2 ) / 4];
					*bbptr++ = cinTable[currentHandle].gray[( ( y1 * 3 ) + y3 ) / 4];
					*bbptr++ = cinTable[currentHandle].gray[( y0 + ( y2 * 3 ) ) / 4];
					*bbptr++ = cinTable[currentHandle].gray[( y1 + ( y3 * 3 ) ) / 4];
					*bbptr++ = cinTable[currentHandle].gray[y2];
					*bbptr++ = cinTable[currentHandle].gray[y3];
				}

				bcptr = (byte *)cin_vq4;
				bdptr = (byte *)cin_vq8;

				for ( i = 0; i < four; i++ ) {
					baptr = (byte *)cin_vq2 + ( *input++ ) * 8;
					bbptr = (byte *)cin_vq2 + ( *input++ ) * 8;
					for ( j = 0; j < 2; j++ ) {
						VQ2TO4( baptr, bbptr, bcptr, bdptr );
						VQ2TO4( baptr, bbptr, bcptr, bdptr );
					}
				}
			}
		}
	} else {
		/* 1/4 screen */
		if ( cinTable[currentHandle].samplesPerPixel == 2 ) {
			for ( i = 0; i < two; i++ ) {
				y0 = (long)*input; input += 2;
				y2 = (long)*input; input += 2;
				cr = (long)*input++;
				cb = (long)*input++;
				*bptr++ = (unsigned short)yuv_to_rgb( y0, cr, cb );
				*bptr++ = (unsigned short)yuv_to_rgb( y2, cr, cb );
			}

			cptr = (unsigned short *)cin_vq4;
			dptr = (unsigned short *)cin_vq8;

			for ( i = 0; i < four; i++ ) {
				aptr = (unsigned short *)cin_vq2 + ( *input++ ) * 2;
				bptr = (unsigned short *)cin_vq2 + ( *input++ ) * 2;
				for ( j = 0; j < 2; j++ ) {
					VQ2TO2( aptr, bptr, cptr, dptr );
				}
			}
		} else if ( cinTable[currentHandle].samplesPerPixel == 1 ) {
			bbptr = (byte *)bptr;

			for ( i = 0; i < two; i++ ) {
				*bbptr++ = cinTable[currentHandle].gray[*input]; input += 2;
				*bbptr++ = cinTable[currentHandle].gray[*input]; input += 4;
			}

			bcptr = (byte *)cin_vq4;
			bdptr = (byte *)cin_vq8;

			for ( i = 0; i < four; i++ ) {
				baptr = (byte *)cin_vq2 + ( *input++ ) * 2;
				bbptr = (byte *)cin_vq2 + ( *input++ ) * 2;
				for ( j = 0; j < 2; j++ ) {
					VQ2TO2( baptr, bbptr, bcptr, bdptr );
				}
			}
		} else if ( cinTable[currentHandle].samplesPerPixel == 4 ) {
			ibptr = (unsigned int *)bptr;
			for ( i = 0; i < two; i++ ) {
				y0 = (long)*input; input += 2;
				y2 = (long)*input; input += 2;
				cr = (long)*input++;
				cb = (long)*input++;
				*ibptr++ = yuv_to_rgb24( y0, cr, cb );
				*ibptr++ = yuv_to_rgb24( y2, cr, cb );
			}

			icptr = (unsigned int *)cin_vq4;
			idptr = (unsigned int *)cin_vq8;

			for ( i = 0; i < four; i++ ) {
				iaptr = (unsigned int *)cin_vq2 + ( *input++ ) * 2;
				ibptr = (unsigned int *)cin_vq2 + ( *input++ ) * 2;
				for ( j = 0; j < 2; j++ ) {
					VQ2TO2( iaptr, ibptr, icptr, idptr );
				}
			}
		}
	}
}

/* ---- recurseQuad  0x00406A20 ----  VERIFIED */
void __cdecl recurseQuad( long startX, long startY, long quadSize, long xOff, long yOff )
{
	byte *scroff;
	long bigx, bigy, lowx, lowy, useY;
	long offset;

	offset = cinTable[currentHandle].screenDelta;

	lowx = lowy = 0;
	bigx = cinTable[currentHandle].xsize;
	bigy = cinTable[currentHandle].ysize;

	if ( bigx > cinTable[currentHandle].CIN_WIDTH ) {
		bigx = cinTable[currentHandle].CIN_WIDTH;
	}
	if ( bigy > cinTable[currentHandle].CIN_HEIGHT ) {
		bigy = cinTable[currentHandle].CIN_HEIGHT;
	}

	if ( ( startX >= lowx ) && ( startX + quadSize ) <= ( bigx )
		&& ( startY + quadSize ) <= ( bigy ) && ( startY >= lowy )
		&& quadSize <= MAXSIZE ) {
		useY = startY;
		scroff = cin_linbuf
			+ ( useY + ( ( cinTable[currentHandle].CIN_HEIGHT - bigy ) >> 1 ) + yOff )
			  * cinTable[currentHandle].samplesPerLine
			+ ( ( startX + xOff ) * cinTable[currentHandle].samplesPerPixel );

		cin_qStatus[0][cinTable[currentHandle].onQuad]   = scroff;
		cin_qStatus[1][cinTable[currentHandle].onQuad++] = scroff + offset;
	}

	if ( quadSize != MINSIZE ) {
		quadSize >>= 1;
		recurseQuad( startX,            startY,            quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY,            quadSize, xOff, yOff );
		recurseQuad( startX,            startY + quadSize, quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY + quadSize, quadSize, xOff, yOff );
	}
}

/* ---- setupQuad  0x00406B30 ----  VERIFIED */
void __cdecl setupQuad( long xOff, long yOff )
{
	long numQuadCels, i, x, y;
	byte *temp;

	if ( xOff == cin_oldXOff && yOff == cin_oldYOff
		&& (long)cinTable[currentHandle].ysize == cin_oldysize
		&& (long)cinTable[currentHandle].xsize == cin_oldxsize ) {
		return;
	}

	cin_oldXOff = xOff;
	cin_oldYOff = yOff;
	cin_oldysize = cinTable[currentHandle].ysize;
	cin_oldxsize = cinTable[currentHandle].xsize;

	numQuadCels  = ( cinTable[currentHandle].xsize * cinTable[currentHandle].ysize ) / ( 16 );
	numQuadCels += numQuadCels / 4;
	numQuadCels += 64;

	cinTable[currentHandle].onQuad = 0;

	for ( y = 0; y < (long)cinTable[currentHandle].ysize; y += 16 ) {
		for ( x = 0; x < (long)cinTable[currentHandle].xsize; x += 16 ) {
			recurseQuad( x, y, 16, xOff, yOff );
		}
	}

	temp = NULL;

	for ( i = ( numQuadCels - 64 ); i < numQuadCels; i++ ) {
		cin_qStatus[0][i] = temp;
		cin_qStatus[1][i] = temp;
	}
}

/* ---- readQuadInfo  0x00406C40 ----  VERIFIED */
void __cdecl readQuadInfo( byte *qData )
{
	if ( currentHandle < 0 ) {
		return;
	}

	cinTable[currentHandle].xsize   = qData[0] + qData[1] * 256;
	cinTable[currentHandle].ysize   = qData[2] + qData[3] * 256;
	cinTable[currentHandle].maxsize = qData[4] + qData[5] * 256;
	cinTable[currentHandle].minsize = qData[6] + qData[7] * 256;

	cinTable[currentHandle].CIN_HEIGHT = cinTable[currentHandle].ysize;
	cinTable[currentHandle].CIN_WIDTH  = cinTable[currentHandle].xsize;

	cinTable[currentHandle].samplesPerLine =
		cinTable[currentHandle].CIN_WIDTH * cinTable[currentHandle].samplesPerPixel;
	cinTable[currentHandle].screenDelta =
		cinTable[currentHandle].CIN_HEIGHT * cinTable[currentHandle].samplesPerLine;

	cinTable[currentHandle].half = qfalse;
	cinTable[currentHandle].smootheddouble = qfalse;

	cinTable[currentHandle].VQ0 = cinTable[currentHandle].VQNormal;
	cinTable[currentHandle].VQ1 = cinTable[currentHandle].VQBuffer;

	cinTable[currentHandle].t[0] =  cinTable[currentHandle].screenDelta;
	cinTable[currentHandle].t[1] = -cinTable[currentHandle].screenDelta;

	cinTable[currentHandle].drawX = cinTable[currentHandle].CIN_WIDTH;
	cinTable[currentHandle].drawY = cinTable[currentHandle].CIN_HEIGHT;

	if ( glConfig.maxTextureSize <= 256 ) {
		if ( cinTable[currentHandle].drawX > 256 ) {
			cinTable[currentHandle].drawX = 256;
		}
		if ( cinTable[currentHandle].drawY > 256 ) {
			cinTable[currentHandle].drawY = 256;
		}
		if ( cinTable[currentHandle].CIN_WIDTH != 256 || cinTable[currentHandle].CIN_HEIGHT != 256 ) {
			Com_Printf( "HACK: approxmimating cinematic for Rage Pro or Voodoo\n" );
		}
	}
}

/* ---- RoQPrepMcomp  0x00406D80 ----  VERIFIED */
void __cdecl RoQPrepMcomp( long xoff, long yoff )
{
	long i, j, x, y, temp, temp2;

	i = cinTable[currentHandle].samplesPerLine;
	j = cinTable[currentHandle].samplesPerPixel;
	if ( cinTable[currentHandle].xsize == ( cinTable[currentHandle].ysize * 4 )
		&& !cinTable[currentHandle].half ) {
		j = j + j;
		i = i + i;
	}

	for ( y = 0; y < 16; y++ ) {
		temp2 = ( y + yoff - 8 ) * i;
		for ( x = 0; x < 16; x++ ) {
			temp = ( x + xoff - 8 ) * j;
			cin_mcomp[( x * 16 ) + y] = cinTable[currentHandle].normalBuffer0 - ( temp2 + temp );
		}
	}
}

/* ---- initRoQ  0x00406E30 ----  VERIFIED */
void __cdecl initRoQ( void )
{
	if ( currentHandle < 0 ) {
		return;
	}

	cinTable[currentHandle].VQNormal = (void ( * )( byte *, void * ))blitVQQuad32fs;
	cinTable[currentHandle].VQBuffer = (void ( * )( byte *, void * ))blitVQQuad32fs;
	cinTable[currentHandle].samplesPerPixel = 4;
	ROQ_GenYUVTables();
	RllSetupTable();
}

/* ---- RoQReset  0x00406E70 ----  VERIFIED */
void __cdecl RoQReset( void )
{
	if ( currentHandle < 0 ) {
		return;
	}

	FS_FCloseFile( cinTable[currentHandle].iFile );
	cinTable[currentHandle].iFile = 0;
	fs_loadingMode = 1;
	FS_FOpenFileRead_Internal( cinTable[currentHandle].fileName,
							   &cinTable[currentHandle].iFile, qtrue, 0 );

	FS_Read( cin_file, 16, cinTable[currentHandle].iFile );

	RoQ_init();
	cinTable[currentHandle].status = FMV_LOOPED;
}

/* ---- RoQInterrupt  0x00406F00 ----  VERIFIED */
void __cdecl RoQInterrupt( void )
{
	byte *framedata;
	short sbuf[32768];
	int ssize;

	if ( currentHandle < 0 ) {
		return;
	}

	FS_Read( cin_file, cinTable[currentHandle].RoQFrameSize + 8, cinTable[currentHandle].iFile );

	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) {
		if ( cinTable[currentHandle].holdAtEnd == qfalse ) {
			if ( cinTable[currentHandle].looping ) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return;
	}

	framedata = cin_file;
redump:
	switch ( cinTable[currentHandle].roq_id ) {
		case ROQ_QUAD_VQ:
			if ( ( cinTable[currentHandle].numQuads & 1 ) ) {
				cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[1];
				RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].VQ1( (byte *)cin_qStatus[1], framedata );
				cinTable[currentHandle].buf = cin_linbuf + cinTable[currentHandle].screenDelta;
			} else {
				cinTable[currentHandle].normalBuffer0 = cinTable[currentHandle].t[0];
				RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].VQ0( (byte *)cin_qStatus[0], framedata );
				cinTable[currentHandle].buf = cin_linbuf;
			}
			if ( cinTable[currentHandle].numQuads == 0 ) {
				Com_Memcpy( cin_linbuf + cinTable[currentHandle].screenDelta, cin_linbuf,
							cinTable[currentHandle].samplesPerLine * cinTable[currentHandle].ysize );
			}
			cinTable[currentHandle].numQuads++;
			cinTable[currentHandle].dirty = qtrue;
			break;
		case ROQ_CODEBOOK:
			decodeCodeBook( framedata, (unsigned short)cinTable[currentHandle].roq_flags );
			break;
		case ZA_SOUND_MONO:
			if ( !cinTable[currentHandle].silent ) {
				ssize = RllDecodeMonoToStereo( framedata, sbuf,
											   cinTable[currentHandle].RoQFrameSize, 0,
											   (unsigned short)cinTable[currentHandle].roq_flags );
				MSS_RawSamples( ssize, 22050, 2, 1, sbuf );
				cinTable[currentHandle].soundStarted = qtrue;
			}
			break;
		case ZA_SOUND_STEREO:
			if ( !cinTable[currentHandle].silent ) {
				ssize = RllDecodeStereoToStereo( framedata, sbuf,
												 cinTable[currentHandle].RoQFrameSize, 0,
												 (unsigned short)cinTable[currentHandle].roq_flags );
				MSS_RawSamples( ssize, 22050, 2, 2, sbuf );
				cinTable[currentHandle].soundStarted = qtrue;
			}
			break;
		case ROQ_QUAD_INFO:
			if ( cinTable[currentHandle].numQuads == -1 ) {
				readQuadInfo( framedata );
				setupQuad( 0, 0 );
				/* retail 0x004071B8: fild of the int, fmul dword -- no rounding to float */
				cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime =
					(int)( (double)CL_ScaledMilliseconds() * com_timescale->value );
			}
			if ( cinTable[currentHandle].numQuads != 1 ) {
				cinTable[currentHandle].numQuads = 0;
			}
			break;
		case ROQ_PACKET:
			cinTable[currentHandle].inMemory = cinTable[currentHandle].roq_flags;
			cinTable[currentHandle].RoQFrameSize = 0;
			break;
		case ROQ_QUAD_HANG:
			cinTable[currentHandle].RoQFrameSize = 0;
			break;
		case ROQ_QUAD_JPEG:
			break;
		default:
			cinTable[currentHandle].status = FMV_EOF;
			break;
	}

	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) {
		if ( cinTable[currentHandle].holdAtEnd == qfalse ) {
			if ( cinTable[currentHandle].looping ) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return;
	}

	framedata += cinTable[currentHandle].RoQFrameSize;
	cinTable[currentHandle].roq_id       = framedata[0] + framedata[1] * 256;
	cinTable[currentHandle].RoQFrameSize = framedata[2] + framedata[3] * 256 + framedata[4] * 65536;
	cinTable[currentHandle].roq_flags    = framedata[6] + framedata[7] * 256;
	cinTable[currentHandle].roqF0        = (char)framedata[7];
	cinTable[currentHandle].roqF1        = (char)framedata[6];

	if ( cinTable[currentHandle].RoQFrameSize > 65536 || cinTable[currentHandle].roq_id == 0x1084 ) {
		Com_DPrintf( "roq_size>65536||roq_id==0x1084 (roq_size=%i,roq_id=%i)\n",
					 cinTable[currentHandle].RoQFrameSize, cinTable[currentHandle].roq_id );
		cinTable[currentHandle].status = FMV_EOF;
		if ( cinTable[currentHandle].looping ) {
			RoQReset();
		}
		return;
	}

	while ( cinTable[currentHandle].inMemory && ( cinTable[currentHandle].status != FMV_EOF ) ) {
		cinTable[currentHandle].inMemory--;
		framedata += 8;
		goto redump;
	}

	cinTable[currentHandle].RoQPlayed += cinTable[currentHandle].RoQFrameSize + 8;
}

/* ---- RoQ_init  0x004073E0 ----  VERIFIED */
void __cdecl RoQ_init( void )
{
	/* retail 0x00407416: fild of the int, fmul dword -- no rounding to float */
	cinTable[currentHandle].lastTime = cinTable[currentHandle].startTime =
		(int)( (double)CL_ScaledMilliseconds() * com_timescale->value );
	cinTable[currentHandle].lastAdvanceTime = cinTable[currentHandle].startTime;

	cinTable[currentHandle].RoQPlayed = 24;

	cinTable[currentHandle].roqFPS = cin_file[6] + cin_file[7] * 256;

	if ( !cinTable[currentHandle].roqFPS ) {
		cinTable[currentHandle].roqFPS = 30;
	}

	cinTable[currentHandle].numQuads = -1;

	cinTable[currentHandle].roq_id       = cin_file[8] + cin_file[9] * 256;
	cinTable[currentHandle].RoQFrameSize = cin_file[10] + cin_file[11] * 256 + cin_file[12] * 65536;
	cinTable[currentHandle].roq_flags    = cin_file[14] + cin_file[15] * 256;
}

/* ---- RoQShutdown  0x004074F0 ----  VERIFIED */
void __cdecl RoQShutdown( void )
{
	cvar_t *var;
	const char *s;

	if ( !cinTable[currentHandle].buf ) {
		return;
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return;
	}

	Com_DPrintf( "finished cinematic\n" );
	cinTable[currentHandle].status = FMV_IDLE;

	if ( cinTable[currentHandle].soundStarted ) {
		MSS_EndRawSamples();
	}

	if ( cinTable[currentHandle].iFile ) {
		FS_FCloseFile( cinTable[currentHandle].iFile );
		cinTable[currentHandle].iFile = 0;
	}

	if ( cinTable[currentHandle].alterGameState ) {
		cls_state = CA_DISCONNECTED;

		var = Cvar_FindVar( "nextmap" );
		if ( var ) {
			s = var->string;
			if ( s[0] ) {
				Cbuf_AddText( va( "%s\n", s ) );
				Cvar_Set2( "nextmap", "", qtrue );
			}
		}
		CL_handle = -1;
	}

	cinTable[currentHandle].fileName[0] = 0;
	currentHandle = -1;
}

/* ---- CIN_RunCinematic  0x00407680 ----  VERIFIED */
int __cdecl CIN_RunCinematic( int handle )
{
	int start;
	int thisTime;
	long tfps;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) {
		return FMV_EOF;
	}

	if ( cin_currentHandle != handle ) {
		currentHandle = handle;
		cin_currentHandle = currentHandle;
		cinTable[currentHandle].status = FMV_EOF;
		RoQReset();
	}

	if ( cinTable[handle].playonwalls < -1 ) {
		return cinTable[handle].status;
	}

	currentHandle = handle;

	if ( cinTable[currentHandle].alterGameState ) {
		if ( cls_state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return cinTable[currentHandle].status;
	}

	thisTime = CL_ScaledMilliseconds();
	if ( cinTable[currentHandle].shader
		&& abs( thisTime - (int)cinTable[currentHandle].lastTime ) > 100 ) {
		cinTable[currentHandle].startTime += thisTime - cinTable[currentHandle].lastTime;
	}

	if ( cinTable[currentHandle].soundStarted ) {
		tfps = cinTable[currentHandle].roqFPS * MSS_RawSamplesTime() / 1000 + 1;
	} else {
		tfps = (long)( (unsigned int)( cinTable[currentHandle].roqFPS
				* ( thisTime - cinTable[currentHandle].startTime ) ) / 1000 );
	}

	if ( cinTable[currentHandle].tfps < tfps ) {
		cinTable[currentHandle].tfps = tfps;
		cinTable[currentHandle].lastAdvanceTime = thisTime;
	} else if ( (unsigned int)( ( thisTime - cinTable[currentHandle].lastAdvanceTime )
			* cinTable[currentHandle].roqFPS ) > 4000 ) {
		cinTable[currentHandle].status = FMV_EOF;
	}

	start = cinTable[currentHandle].startTime;
	while ( ( cinTable[currentHandle].tfps != cinTable[currentHandle].numQuads )
		&& ( cinTable[currentHandle].status == FMV_PLAY ) ) {
		RoQInterrupt();
		if ( start != (int)cinTable[currentHandle].startTime ) {
			cinTable[currentHandle].tfps = (long)( (unsigned int)( ( CL_ScaledMilliseconds()
					- cinTable[currentHandle].startTime )
				* cinTable[currentHandle].roqFPS ) / 1000 );
			start = cinTable[currentHandle].startTime;
		}
	}

	cinTable[currentHandle].lastTime = thisTime;

	if ( cinTable[currentHandle].status == FMV_LOOPED ) {
		cinTable[currentHandle].status = FMV_PLAY;
	}

	if ( cinTable[currentHandle].status == FMV_EOF ) {
		if ( cinTable[currentHandle].looping ) {
			RoQReset();
		} else {
			RoQShutdown();
		}
	}

	return cinTable[currentHandle].status;
}

/* ---- CIN_PlayCinematic  0x004078D0 ----  VERIFIED */
int __cdecl CIN_PlayCinematic( const char *arg, int x, int y, int w, int h, int systemBits )
{
	unsigned short RoQID;
	char name[MAX_OSPATH];
	int i;

	if ( strstr( arg, "/" ) == NULL && strstr( arg, "\\" ) == NULL ) {
		Com_sprintf( name, sizeof( name ), "video/%s", arg );
	} else {
		Com_sprintf( name, sizeof( name ), "%s", arg );
	}

	if ( !( systemBits & CIN_system ) ) {
		for ( i = 0; i < MAX_VIDEO_HANDLES; i++ ) {
			if ( !Q_stricmp( cinTable[i].fileName, name ) ) {
				return i;
			}
		}
	}

	if ( ( systemBits & CIN_system ) && cls_state != CA_DISCONNECTED
		&& cls_state != CA_CINEMATIC && cls_state != CA_LOGO ) {
		Com_Printf( "Can't play a cinematic while connected to a server; use 'disconnect' first\n" );
		return -1;
	}

	Com_DPrintf( "SCR_PlayCinematic( %s )\n", arg );

	/* Retail 0x004079BD clears the whole contiguous cin block in one call:
	 * Com_Memset( &cin, 0, 0x250614 ), 0x0057E910..0x007CEF24.  The tree splits
	 * cin into separate globals, so each piece is cleared on its own; together
	 * they cover CIN_SCRATCH_SIZE bytes. */
	Com_Memset( &cin, 0, sizeof( cin ) );
	currentHandle = CIN_HandleForVideo();

	cin_currentHandle = currentHandle;

	strcpy( cinTable[currentHandle].fileName, name );

	cinTable[currentHandle].ROQSize = 0;
	cinTable[currentHandle].ROQSize =
		FS_FOpenFileRead( cinTable[currentHandle].fileName,
						  &cinTable[currentHandle].iFile, qtrue );

	if ( cinTable[currentHandle].ROQSize <= 0 ) {
		Com_DPrintf( "play(%s), ROQSize<=0\n", arg );
		cinTable[currentHandle].fileName[0] = 0;
		return -1;
	}

	CIN_SetExtents( currentHandle, x, y, w, h );
	CIN_SetLooping( currentHandle, ( systemBits & CIN_loop ) != 0 );

	cinTable[currentHandle].CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	cinTable[currentHandle].CIN_WIDTH  = DEFAULT_CIN_WIDTH;
	cinTable[currentHandle].holdAtEnd = ( systemBits & CIN_hold ) != 0;
	cinTable[currentHandle].alterGameState = ( systemBits & CIN_system ) != 0;
	cinTable[currentHandle].playonwalls = 1;
	cinTable[currentHandle].silent = ( systemBits & CIN_silent ) != 0;
	cinTable[currentHandle].shader = ( systemBits & CIN_shader ) != 0;
	cinTable[currentHandle].letterBox = ( systemBits & CIN_letterbox ) != 0;
	cinTable[currentHandle].soundStarted = qfalse;

	if ( cinTable[currentHandle].alterGameState ) {
		if ( uivm ) {
			VM_Call( uivm, UI_SET_ACTIVE_MENU, 0 );
		}
	} else {
		cinTable[currentHandle].playonwalls = r_inGameVideo->integer;
	}

	initRoQ();

	FS_Read( cin_file, 16, cinTable[currentHandle].iFile );

	/* CoD reads the RoQ id at offset 0, not Q3's cin.file[8]; do not restore Q3's indices. */
	RoQID = (unsigned short)( cin_file[0] ) + (unsigned short)( cin_file[1] ) * 256;
	if ( RoQID == ROQ_FILE ) {
		RoQ_init();

		cinTable[currentHandle].status = FMV_PLAY;
		Com_DPrintf( "trFMV::play(), playing %s\n", arg );

		if ( cinTable[currentHandle].alterGameState ) {
			cls_state = CA_CINEMATIC;
		}

		Con_Close();

		return currentHandle;
	}

	Com_DPrintf( "trFMV::play(), invalid RoQ ID\n" );

	RoQShutdown();
	return -1;
}

/* ---- CIN_SetExtents  0x00407CB0 ----  VERIFIED */
void __cdecl CIN_SetExtents( int handle, int x, int y, int w, int h )
{
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES ) {
		return;
	}
	if ( cinTable[handle].status == FMV_EOF ) {
		return;
	}

	cinTable[handle].xpos = x;
	cinTable[handle].ypos = y;
	cinTable[handle].width = w;
	cinTable[handle].height = h;
	cinTable[handle].dirty = qtrue;
}

/* ---- CIN_SetLooping  0x00407D00 ----  VERIFIED */
void __cdecl CIN_SetLooping( int handle, qboolean loop )
{
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES ) {
		return;
	}
	if ( cinTable[handle].status == FMV_EOF ) {
		return;
	}

	cinTable[handle].looping = loop;
}

/* ---- CIN_DrawCinematic  0x00407D30 ----  VERIFIED */
void __cdecl CIN_DrawCinematic( int handle )
{
	float x, y, w, h;
	byte *buf;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) {
		return;
	}

	if ( !cinTable[handle].buf ) {
		return;
	}

	x = (float)cinTable[handle].xpos;
	y = (float)cinTable[handle].ypos;
	w = (float)cinTable[handle].width;
	h = (float)cinTable[handle].height;
	buf = cinTable[handle].buf;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	if ( cinTable[handle].letterBox ) {
		float vh = (float)cls_glconfig_vidHeight;
		float bar = vh * 0.21875f;

		re_SetColor( (const float *)colorBlack );
		re_DrawStretchPic( 0, 0, w, bar, 0, 0, 0, 0, whiteShader );
		re_DrawStretchPic( 0, vh - bar - 1.0f, w, bar + 1.0f, 0, 0, 0, 0, whiteShader );
	}

	if ( cinTable[handle].dirty
		&& ( cinTable[handle].CIN_WIDTH != cinTable[handle].drawX
		  || cinTable[handle].CIN_HEIGHT != cinTable[handle].drawY ) ) {
		int ix, iy, *buf2, *buf3, xm, ym, ll;

		xm = cinTable[handle].CIN_WIDTH / 256;
		ym = cinTable[handle].CIN_HEIGHT / 256;
		ll = 8;
		if ( cinTable[handle].CIN_WIDTH == 512 ) {
			ll = 9;
		}

		buf3 = (int *)buf;
		buf2 = (int *)Hunk_AllocateTempMemoryInternal( 256 * 256 * 4 );

		if ( xm == 2 && ym == 2 ) {
			byte *bc2, *bc3;
			int ic, iiy;

			bc2 = (byte *)buf2;
			bc3 = (byte *)buf3;
			for ( iy = 0; iy < 256; iy++ ) {
				iiy = iy << 12;
				for ( ix = 0; ix < 2048; ix += 8 ) {
					for ( ic = ix; ic < ( ix + 4 ); ic++ ) {
						*bc2 = ( bc3[iiy + ic] + bc3[iiy + 4 + ic]
							   + bc3[iiy + 2048 + ic] + bc3[iiy + 2048 + 4 + ic] ) >> 2;
						bc2++;
					}
				}
			}
		} else if ( xm == 2 && ym == 1 ) {
			byte *bc2, *bc3;
			int ic, iiy;

			bc2 = (byte *)buf2;
			bc3 = (byte *)buf3;
			for ( iy = 0; iy < 256; iy++ ) {
				iiy = iy << 11;
				for ( ix = 0; ix < 2048; ix += 8 ) {
					for ( ic = ix; ic < ( ix + 4 ); ic++ ) {
						*bc2 = ( bc3[iiy + ic] + bc3[iiy + 4 + ic] ) >> 1;
						bc2++;
					}
				}
			}
		} else {
			for ( iy = 0; iy < 256; iy++ ) {
				for ( ix = 0; ix < 256; ix++ ) {
					buf2[( iy << 8 ) + ix] = buf3[( ( iy * ym ) << ll ) + ( ix * xm )];
				}
			}
		}

		re_DrawStretchRawPtr( (int)x, (int)y, (int)w, (int)h, 256, 256, (byte *)buf2, handle, qtrue );
		cinTable[handle].dirty = qfalse;
		Hunk_FreeTempMemory( buf2 );
		return;
	}

	re_DrawStretchRawPtr( (int)x, (int)y, (int)w, (int)h,
						  cinTable[handle].drawX, cinTable[handle].drawY,
						  buf, handle, cinTable[handle].dirty );
	cinTable[handle].dirty = qfalse;
}

/* ---- CL_PlayCinematic_f  0x004080D0 ----  VERIFIED */
void __cdecl CL_PlayCinematic_f( void )
{
	char *arg, *s;
	int bits = CIN_system;

	Com_DPrintf( "CL_PlayCinematic_f\n" );

	if ( cls_state == CA_CINEMATIC ) {
		SCR_StopCinematic();
	} else if ( cls_state == CA_LOGO ) {
		cls_state = CA_DISCONNECTED;
	}

	arg = Cmd_Argv( 1 );
	s = Cmd_Argv( 2 );

	if ( ( s && s[0] == '1' ) || Q_stricmp( arg, "demoend.roq" ) == 0
		|| Q_stricmp( arg, "end.roq" ) == 0 ) {
		bits |= CIN_hold;
	}
	if ( s ) {
		if ( s[0] == '2' ) {
			bits |= CIN_loop;
		}
		if ( s[0] == '3' ) {
			bits |= CIN_letterbox;
		}
	}

	if ( bits & CIN_letterbox ) {
		CL_handle = CIN_PlayCinematic( arg, 0, LETTERBOX_OFFSET, 640, 270, bits );
	} else {
		CL_handle = CIN_PlayCinematic( arg, 0, 0, 640, 480, bits );
	}

	if ( CL_handle >= 0 ) {
		MSS_StopSounds( 0 );

		MSS_FadeAllSounds( 1.0f, 0 );

		do {
			SCR_RunCinematic();
		} while ( cinTable[currentHandle].buf == NULL
				  && cinTable[currentHandle].status == FMV_PLAY );
	}
}

/* ---- SCR_DrawCinematic  0x00408240 ----  [CONFIRMED] */
void __cdecl SCR_DrawCinematic( void )
{
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_DrawCinematic( CL_handle );
	}
}

/* ---- SCR_RunCinematic  0x00408260 ----  [CONFIRMED] */
void __cdecl SCR_RunCinematic( void )
{
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_RunCinematic( CL_handle );
	}
}

/* ---- SCR_StopCinematic  0x00408280 ----  VERIFIED */
void __cdecl SCR_StopCinematic( void )
{
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_StopCinematic( CL_handle );
		MSS_StopSounds( 0 );
		CL_handle = -1;
	}
}

/* ---- CIN_UploadCinematic  0x004082B0 ----  VERIFIED */
void __cdecl CIN_UploadCinematic( int handle )
{
	if ( handle >= 0 && handle < MAX_VIDEO_HANDLES ) {
		if ( !cinTable[handle].buf ) {
			return;
		}

		if ( cinTable[handle].playonwalls <= 0 && cinTable[handle].dirty ) {
			if ( cinTable[handle].playonwalls == 0 ) {
				cinTable[handle].playonwalls = -1;
			} else {
				if ( cinTable[handle].playonwalls == -1 ) {
					cinTable[handle].playonwalls = -2;
				} else {
					cinTable[handle].dirty = qfalse;
				}
			}
		}

		re_UploadCinematicPtr( 256, 256, 256, 256, cinTable[handle].buf,
							   handle, cinTable[handle].dirty );

		if ( r_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1 ) {
			cinTable[handle].playonwalls = 0;
		}
	}
}

cinematic_t cinTable[MAX_VIDEO_HANDLES];        /* 0x00877728, 7424 bytes */

/* ---- CIN_StopCinematic  0x00407600 ----  [CONFIRMED] */
int __cdecl CIN_StopCinematic( int handle ) {
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES ) {
		return FMV_EOF;
	}
	if ( cinTable[handle].status == FMV_EOF ) {
		return FMV_EOF;
	}

	currentHandle = handle;

	Com_DPrintf( "trFMV::stop(), closing %s\n", cinTable[handle].fileName );

	/* +0x1C4 is `buf`, not `iFile` -- see cl_cinematic.h. The test is Q3's `if (!cinTable[currentHandle].buf) return FMV_EOF;`: nothing was ever decoded, so there is nothing to shut down. */
	if ( !cinTable[currentHandle].buf ) {
		return FMV_EOF;
	}

	if ( cinTable[currentHandle].alterGameState && cls_state != CA_CINEMATIC ) {
		return cinTable[currentHandle].status;
	}

	cinTable[currentHandle].status = FMV_EOF;
	RoQShutdown();

	return FMV_EOF;
}

/* ---- CIN_CloseAllVideos  0x00404FE0 ----  [CONFIRMED] */
void CIN_CloseAllVideos( void ) {
	int i;

	for ( i = 0; i < MAX_VIDEO_HANDLES; i++ ) {
		if ( cinTable[i].fileName[0] ) {
			CIN_StopCinematic( i );
		}
	}
}
