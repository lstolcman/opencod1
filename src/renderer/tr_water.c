/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
int s_fftBitReverse[256];
int s_numWaterMaps;


extern int GL_Bind();
extern int __cdecl R_CreateImage( char *name, int flags, int imageTrack,
                                  unsigned __int8 *pixels, int width, int height,
                                  int format, float *colorScale );


/* ------------------------------------------------------------------------- *
 *  File-local water types.
 *
 *  CoD's animated-water shader deforms a heightfield with an inverse FFT of a
 *  Phillips-style ocean spectrum, then bakes the result into a normal map that
 *  is re-uploaded each frame.  The decompiled code reaches the per-map record
 *  and the complex spectrum through `*(unsigned int *)(p + off)` soup; the two record
 *  types below type that pointer-passed data.  Both are file-local (CoD-only,
 *  no RTCW analog); every offset is proven by the decompiled reads and matches
 *  the CoDUO reconstruction (renderer_water.c), pinned with asserts so a wrong
 *  offset fails the build.  Field names follow CoDUO.
 * ------------------------------------------------------------------------- */

/* one complex sample (real, imaginary) -- spectrum bin / FFT working element */
typedef struct {
	float real;             /* +0x00 */
	float imaginary;        /* +0x04 */
} waterComplex_t;
COD1_ASSERT_SIZE(waterComplex_t, 0x08);
ASSERT_OFF(waterComplex_t, imaginary, 0x04);

/* one cached water surface: its baked normal texture plus the parameters and
 * pre-rolled spectrum used to re-derive it every frame.  Retail record is
 * 13 dwords (0x34); R_GetWaterTexture block-copies it whole from the parser. */
typedef struct {
	void           *image;                 /* +0x00  GL image (width @ +64, height @ +66) */
	int             uploadFrame;           /* +0x04  tr.frameCount of the last upload */
	waterComplex_t *initialFrequencies;    /* +0x08  h0(k), textureWidth*textureHeight bins */
	float          *angularFrequencies;    /* +0x0c  w(k), one per bin (== base + 8*texelCount) */
	int             textureWidth;          /* +0x10 */
	int             textureHeight;         /* +0x14 */
	float           horizontalWorldLength; /* +0x18  world span mapped across the height axis */
	float           verticalWorldLength;   /* +0x1c  world span mapped across the width axis */
	float           gravity;               /* +0x20 */
	float           windVelocity;          /* +0x24 */
	float           windDirection[2];      /* +0x28  [0] = x, [1] = y */
	float           amplitude;             /* +0x30 */
} waterMap_t;
COD1_ASSERT_SIZE(waterMap_t, 0x34);
ASSERT_OFF(waterMap_t, uploadFrame, 0x04);
ASSERT_OFF(waterMap_t, initialFrequencies, 0x08);
ASSERT_OFF(waterMap_t, angularFrequencies, 0x0c);
ASSERT_OFF(waterMap_t, textureWidth, 0x10);
ASSERT_OFF(waterMap_t, textureHeight, 0x14);
ASSERT_OFF(waterMap_t, horizontalWorldLength, 0x18);
ASSERT_OFF(waterMap_t, verticalWorldLength, 0x1c);
ASSERT_OFF(waterMap_t, gravity, 0x20);
ASSERT_OFF(waterMap_t, windVelocity, 0x24);
ASSERT_OFF(waterMap_t, windDirection, 0x28);
ASSERT_OFF(waterMap_t, amplitude, 0x30);

/* 0x011EE0B0: 256 { cos, sin } twiddle pairs, angle step 2*pi/256. */
static waterComplex_t s_fftTwiddle[256];

/* 0x011EDD70: the sixteen cached water-map records. */
static waterMap_t s_waterMaps[16];

/* s_fftBitReverse[256] (8-bit reversal table) is defined above; s_numWaterMaps
 * is defined above too (extern in tr_local.h). */


/* Retail's inlined fsincos block (FFT_Init 0x00506F7E) saves eax/ecx around
 * itself: an inline-asm SinCos like cgame's CG_SinCos, not com_math.c's. */
static __inline void R_WaterSinCos( float radians, float *sinOut, float *cosOut )
{
	__asm {
		push    eax
		push    ecx
		fld     radians
		fsincos
		mov     ecx, cosOut
		mov     eax, sinOut
		fstp    dword ptr [ecx]
		fstp    dword ptr [eax]
		pop     ecx
		pop     eax
	}
}

/* ---- FFT_Init  0x00506EE0 ----  VERIFIED
 * Build the two lookup tables the FFT needs: the 8-bit bit-reversal permutation
 * and the twiddle-factor circle (angle step 2*pi/256). */
void FFT_Init( void )
{
  int i;
  int bit;

  for ( i = 0; i < 256; ++i )
  {
    s_fftBitReverse[i] = 0;
    for ( bit = 0; bit < 8; ++bit )
    {
      if ( ( i & ( 1 << bit ) ) != 0 )
        s_fftBitReverse[i] |= 1 << ( 7 - bit );
    }

    R_WaterSinCos( i * 0.02454369328916073f, &s_fftTwiddle[i].imaginary, &s_fftTwiddle[i].real );
  }
}

/* ---- FFT  0x00506FB0 ----  VERIFIED
 * In-place decimation-in-time inverse FFT over `count = 1 << bits` complex
 * samples spaced `stride` apart: a bit-reversal reorder, a radix-4 first pass,
 * then radix-2 combine passes with twiddle factors.  The radix-4 B/D outputs
 * use CoD1's sign convention (the negation of CoDUO's), verified term-by-term
 * against the decompile.  Parameter order is retail's (bits, data, stride);
 * cross-file callers rely on it.  The radix-4 pass is retail's inline asm (a
 * do-while over count >> 2 groups, no guard: count >= 4 assumed); it is why the
 * function has an EBP frame while `bits` still arrives in ECX under LTCG.
 * No double locals here: a double makes LTCG pad the callers' frames. */
void __cdecl FFT( int bits, waterComplex_t *data, int stride )
{
  int            shift;          /* 8 - bits, then the twiddle shift (one slot in retail) */
  int            count;
  int            span;
  int            i;
  int            twiddleIndex;
  int            doubleSpan;
  waterComplex_t twiddle;

  count = 1 << bits;
  shift = 8 - bits;

  /* ---- bit-reversal reorder ---- */
  for ( i = 0; i < count; ++i )
  {
    int reversed = s_fftBitReverse[i] >> shift;

    if ( reversed < i )
    {
      waterComplex_t tmp      = data[i * stride];
      data[i * stride]        = data[reversed * stride];
      data[reversed * stride] = tmp;
    }
  }

  /* ---- radix-4 first pass (retail inline asm) ---- */
  __asm
  {
    mov     eax, stride
    mov     edi, data
    mov     ecx, count
    lea     eax, [eax*8]
    shr     ecx, 2
    lea     esi, [edi+eax]
  radix4:
    fld     dword ptr [edi+4]
    fld     dword ptr [edi+eax*2]
    fld     dword ptr [edi+eax*2+4]
    fld     dword ptr [edi]
    fld     dword ptr [edi]
    fld     dword ptr [edi+eax*2]
    fxch    st(1)
    fadd    dword ptr [esi]
    fxch    st(1)
    fadd    dword ptr [esi+eax*2]
    fld     st(1)
    fld     st(1)
    faddp   st(1), st
    fxch    st(2)
    fsubrp  st(1), st
    fxch    st(1)
    fstp    dword ptr [edi]
    fstp    dword ptr [edi+eax*2]
    fld     dword ptr [edi+4]
    fld     dword ptr [edi+eax*2+4]
    fxch    st(1)
    fadd    dword ptr [esi+4]
    fxch    st(1)
    fadd    dword ptr [esi+eax*2+4]
    fld     st(1)
    fld     st(1)
    faddp   st(1), st
    fxch    st(2)
    fsubrp  st(1), st
    fxch    st(1)
    fstp    dword ptr [edi+4]
    fstp    dword ptr [edi+eax*2+4]
    fsubr   dword ptr [esi]
    fxch    st(1)
    fsubr   dword ptr [esi+eax*2+4]
    fld     st(1)
    fld     st(1)
    fsubp   st(1), st
    fxch    st(2)
    faddp   st(1), st
    fxch    st(1)
    fstp    dword ptr [esi]
    fxch    st(1)
    fsubr   dword ptr [esi+eax*2]
    fxch    st(1)
    fstp    dword ptr [esi+eax*2]
    fxch    st(1)
    fsubr   dword ptr [esi+4]
    fld     st(1)
    fld     st(1)
    faddp   st(1), st
    fxch    st(2)
    fsubp   st(1), st
    fxch    st(1)
    fstp    dword ptr [esi+4]
    fstp    dword ptr [esi+eax*2+4]
    dec     ecx
    lea     edi, [edi+eax*4]
    lea     esi, [esi+eax*4]
    jne     radix4
  }

  /* ---- radix-2 combine passes ---- */
  span  = 4;
  shift = 5;
  while ( span < count )
  {
    doubleSpan = 2 * span;
    for ( twiddleIndex = 0; twiddleIndex < span; ++twiddleIndex )
    {
      unsigned index;

      twiddle = s_fftTwiddle[twiddleIndex << shift];
      for ( index = twiddleIndex * stride; index < (unsigned)( count * stride ); index += doubleSpan * stride )
      {
        waterComplex_t *upper = &data[index + span * stride];
        rfloat          productReal      = twiddle.real * upper->real - twiddle.imaginary * upper->imaginary;
        rfloat          productImaginary = twiddle.real * upper->imaginary;

        productImaginary += twiddle.imaginary * upper->real;   /* this form keeps retail's term order */

        upper->real            = data[index].real - productReal;
        upper->imaginary       = data[index].imaginary - productImaginary;
        data[index].real      += productReal;
        data[index].imaginary += productImaginary;
      }
    }

    span = doubleSpan;
    --shift;
  }
}

/* ---- GaussianRandom  0x005071D0 ----  VERIFIED
 * Marsaglia polar method: two standard normals from rejection-sampled points
 * in the unit disc.  The x87 fyl2x that produced a natural log is written as
 * log(); 2 * (r / 32768) is retail's fmul 1/32768 + fadd st,st.  Returning
 * void keeps it cdecl under LTCG (a pointer return moved `real` into ESI). */
void __cdecl GaussianRandom( float *real, float *imaginary )
{
  float  x;
  rfloat y;
  rfloat radiusSquared;
  rfloat scale;

  do
  {
    x = 2.0f * ( rand() / 32768.0f ) - 1.0f;
    y = 2.0f * ( rand() / 32768.0f ) - 1.0f;
    radiusSquared = y * y + x * x;
  }
  while ( radiusSquared > 1.0f );

  scale      = sqrt( log( radiusSquared ) * -2.0 / radiusSquared );
  *real      = x * scale;
  *imaginary = scale * y;
}

/* ---- R_WaterExp  no-address ----  [HIGH]
 * exp(x) the way retail's x87 computes it (2^(x*log2 e), split into integer +
 * fractional scale).  Under VC7 this is the exp() intrinsic, which emits exactly
 * that sequence inline in PickWaterFrequencies; the modern build keeps the x87
 * asm so it does not switch to the CRT's SSE exp. */
#ifdef COD1_VC7
#define R_WaterExp( x ) exp( x )
#else
static double R_WaterExp( double x )
{
	double r;

	__asm
	{
		fld     x
		fldl2e
		fmulp   st(1), st
		fld     st
		frndint
		fxch    st(1)
		fsub    st, st(1)
		f2xm1
		fld1
		faddp   st(1), st
		fscale
		fstp    st(1)
		fstp    r
	}
	return r;
}
#endif

/* ---- R_WaterFtol  no-address ----  [HIGH]
 * myftol (0x00515940) as retail inlines it: the float argument is stored to a
 * 4-byte slot, then fld / fistp ftol_tempSpill / mov eax, ftol_tempSpill
 * (round to nearest under the default control word). */
static int R_WaterFtol( float f )
{
	__asm fld   f
	__asm fistp ftol_tempSpill
	return ftol_tempSpill;
}

/* ---- PickWaterFrequencies  0x00507270 ----  VERIFIED
 * Fill the map's h0(k) spectrum and per-bin angular frequency from a
 * Phillips-style ocean spectrum shaped by wind speed/direction.  Both CRT
 * random draws are consumed for every bin, before the spectrum gate.  The
 * 8-byte gaussian struct local is what gives retail its and esp,-8 frame. */
void __cdecl PickWaterFrequencies( waterMap_t *waterMap )
{
  int   textureWidth  = waterMap->textureWidth;
  int   textureHeight = waterMap->textureHeight;
  float windLengthScaleSquared;
  float rowWaveStep;
  float columnWaveStep;
  int   frequencyIndex = 0;
  int   row, column;

  windLengthScaleSquared = waterMap->windVelocity * waterMap->windVelocity / waterMap->gravity
                         * ( waterMap->windVelocity * waterMap->windVelocity );
#ifdef COD1_VC7
  rowWaveStep    = 6.2831855f / ( textureHeight * waterMap->horizontalWorldLength );
  columnWaveStep = 6.2831855f / ( textureWidth  * waterMap->verticalWorldLength );
#else   /* modern: retail's x87 keeps the product and the quotient at double precision */
  rowWaveStep    = 6.2831855f / ( (double)textureHeight * waterMap->horizontalWorldLength );
  columnWaveStep = 6.2831855f / ( (double)textureWidth  * waterMap->verticalWorldLength );
#endif

  for ( row = -waterMap->textureHeight / 2; row < waterMap->textureHeight / 2; ++row )
  {
    float rowWaveNumber = row * rowWaveStep;

    for ( column = -waterMap->textureWidth / 2; column < waterMap->textureWidth / 2; ++column )
    {
      float  columnWaveNumber = column * columnWaveStep;
      float  rowWaveNumberSquared;
      rfloat waveNumberSquared;
      float  angularFrequencySquared;
      rfloat windDotWave;
      waterComplex_t gaussian;

      GaussianRandom( &gaussian.real, &gaussian.imaginary );

      rowWaveNumberSquared    = rowWaveNumber * rowWaveNumber;
      waveNumberSquared       = columnWaveNumber * columnWaveNumber + rowWaveNumberSquared;
      angularFrequencySquared = sqrt( waveNumberSquared ) * waterMap->gravity;
      windDotWave             = columnWaveNumber * waterMap->windDirection[1]
                              + rowWaveNumber    * waterMap->windDirection[0];

      if ( windDotWave <= 0.0f )
      {
        waterMap->initialFrequencies[frequencyIndex].real      = 0.0f;
        waterMap->initialFrequencies[frequencyIndex].imaginary = 0.0f;
        waterMap->angularFrequencies[frequencyIndex]           = 0.0f;
      }
      else
      {
        rfloat phillips;
        rfloat scale;

        phillips = R_WaterExp( -1.0f / ( waveNumberSquared * windLengthScaleSquared ) ) * waterMap->amplitude
                 / ( waveNumberSquared * waveNumberSquared * waveNumberSquared )
                 * ( windDotWave * windDotWave );
        scale    = sqrt( phillips * 0.5f ) * waterMap->amplitude;

        waterMap->initialFrequencies[frequencyIndex].real      = gaussian.real * scale;
        waterMap->initialFrequencies[frequencyIndex].imaginary = gaussian.imaginary * scale;
        waterMap->angularFrequencies[frequencyIndex]           = sqrt( angularFrequencySquared );
      }

      ++frequencyIndex;
    }
  }
}

/* ---- WaterFrequenciesAtTime  0x00507430 ----  VERIFIED
 * Rotate each spectrum bin by its phase at `timeSeconds`, sampling tr.sinTable
 * (the +255 offset gives the cosine term for the real part).  The phase is
 * spilled to a float, then converted by an inlined myftol (round-to-nearest
 * fistp into ftol_tempSpill) and masked to the 1024-entry table. */
void __cdecl WaterFrequenciesAtTime( waterComplex_t *frequencies, waterMap_t *waterMap, float timeSeconds )
{
  int index = 0;
  int row, column;

  for ( row = -waterMap->textureHeight / 2; row < waterMap->textureHeight / 2; ++row )
  {
    for ( column = -waterMap->textureWidth / 2; column < waterMap->textureWidth / 2; ++column )
    {
      if ( waterMap->angularFrequencies[index] == 0.0f )
      {
        frequencies[index].real      = 0.0f;
        frequencies[index].imaginary = 0.0f;
      }
      else
      {
        int sineIndex   = R_WaterFtol( timeSeconds * waterMap->angularFrequencies[index] * 162.97466f ) & 0x3FF; /* 1024/(2*pi) */
        int cosineIndex = ( sineIndex + 255 ) & 0x3FF;

        frequencies[index].real      = tr_sinTable[cosineIndex] * waterMap->initialFrequencies[index].real;
        frequencies[index].imaginary = tr_sinTable[sineIndex]   * waterMap->initialFrequencies[index].imaginary;
      }

      ++index;
    }
  }
}

/* ---- WaterAmplitudesFromFrequencies  0x00507520 ----  VERIFIED
 * Inverse-transform the time-rotated spectrum into a spatial heightfield: an
 * FFT along every row, then along every column.  Retail's EAX is left at
 * textureWidth (dead). */
void __cdecl WaterAmplitudesFromFrequencies( waterMap_t *waterMap, waterComplex_t *amplitudes )
{
  int rowLog2;       /* log2(textureHeight) */
  int columnLog2;    /* log2(textureWidth)  */
  int i;

  rowLog2 = 0;
  while ( waterMap->textureHeight != ( 1 << rowLog2 ) )
    ++rowLog2;
  columnLog2 = 0;
  while ( waterMap->textureWidth != ( 1 << columnLog2 ) )
    ++columnLog2;

  for ( i = 0; i < waterMap->textureHeight; ++i )
    FFT( columnLog2, amplitudes + i * waterMap->textureWidth, 1 );

  for ( i = 0; i < waterMap->textureWidth; ++i )
    FFT( rowLog2, amplitudes + i, waterMap->textureWidth );
}

/* ---- WaterNormalsFromAmplitudes  0x005075B0 ----  VERIFIED
 * Turn the heightfield into a packed RGBA normal map.  The height at each texel
 * is the magnitude of its complex amplitude (overwriting the real part), and
 * each normal is the cross of the toroidally-wrapped column/row height slopes
 * against a constant Z.  The Z term is texelCount^2 in wrapped 32-bit int math
 * (0 for a 256x256 map), matching retail exactly.  Retail's per-row texel loop
 * is an inline-asm block (EBP frame, stack arguments, fistp per component =
 * round to nearest, `fmul st(0), st` squares); the locals it names live in the
 * frame in declaration order. */
void __cdecl WaterNormalsFromAmplitudes( unsigned int *normalPixels, waterComplex_t *amplitudes, waterMap_t *waterMap )
{
  int   currentIndex;
  int   previousColumnIndex;
  int   texelCount;
  int   previousRowIndex;
  int   row;
  float normalScale;
  float normalZSquared;
  int   textureWidth;
  int   component;
  int   texelMask;
  int   index;

  texelCount     = waterMap->textureWidth * waterMap->textureHeight;
  normalZSquared = (int)( (unsigned)texelCount * (unsigned)texelCount );   /* wrapped int square */
  textureWidth   = waterMap->textureWidth;
  normalScale    = 127.0f;

  /* collapse each complex amplitude to its magnitude, stored back in .real */
  for ( index = 0; index < texelCount; ++index )
#ifdef COD1_VC7
    amplitudes[index].real = sqrt( amplitudes[index].imaginary * amplitudes[index].imaginary
                                 + amplitudes[index].real * amplitudes[index].real );
#else   /* modern: keep the products at double precision like retail's x87 */
    amplitudes[index].real = sqrt( (double)amplitudes[index].imaginary * amplitudes[index].imaginary
                                 + (double)amplitudes[index].real * amplitudes[index].real );
#endif

  texelMask           = texelCount - 1;
  currentIndex        = 0;
  previousColumnIndex = waterMap->textureWidth - 1;
  previousRowIndex    = texelCount - waterMap->textureWidth;

  for ( row = 0; row < waterMap->textureHeight; ++row )
  {
    __asm
    {
      mov     esi, amplitudes
      mov     ebx, previousColumnIndex
      mov     edx, previousRowIndex
      mov     eax, currentIndex
      mov     ecx, textureWidth
      mov     edi, normalPixels
      lea     ebx, [esi+ebx*8]
      lea     edx, [esi+edx*8]
      lea     esi, [esi+eax*8]
    nextTexel:
      fld     dword ptr [ebx]          /* previous column height */
      fld     dword ptr [edx]          /* previous row height */
      fxch    st(1)
      fsub    dword ptr [esi]          /* columnDifference */
      fxch    st(1)
      fsub    dword ptr [esi]          /* rowDifference */
      fld     st(1)
      fmul    st(0), st
      fld     st(1)
      fmul    st(0), st
      fxch    st(1)
      fadd    normalZSquared           /* ( cd^2 + Z ) + rd^2 */
      faddp   st(1), st
      fsqrt
      fld     normalScale
      fdivrp  st(1), st                /* scale = 127 / length */
      mov     ebx, esi
      add     edx, 8
      add     esi, 8
      fild    texelCount
      mov     eax, 0FF7Fh
      fmul    st, st(1)
      fistp   component                /* normal Z */
      add     eax, component
      fxch    st(1)
      shl     eax, 8
      fmul    st, st(1)
      add     eax, 7Fh
      fistp   component                /* normal Y */
      add     eax, component
      fmulp   st(1), st
      shl     eax, 8
      fistp   component                /* normal X */
      add     eax, 7Fh
      add     eax, component
      mov     [edi], eax
      add     edi, 4
      dec     ecx
      jne     nextTexel
      mov     normalPixels, edi
    }

    currentIndex        += waterMap->textureWidth;
    previousColumnIndex += waterMap->textureWidth;
    previousRowIndex     = ( currentIndex - waterMap->textureWidth + texelCount ) & texelMask;
  }
}

/* ---- WatersEquivalent  0x005076F0 ----  VERIFIED
 * Two water configs match if their dimensions and world lengths are identical
 * and their amplitude/gravity/wind speed and wind direction are within
 * tolerance -- lets several water surfaces share one generated texture.
 * `a` is the cached map, `b` the new config (LTCG passes them in EDX / ECX).
 * The accumulated dot keeps retail's term order; the float `mag` is retail's
 * dead fst dword (the unrounded value feeds the divide). */
int __fastcall WatersEquivalent( const waterMap_t *a, const waterMap_t *b )
{
  rfloat dot;
  rfloat mag;

  if ( a->textureWidth != b->textureWidth
    || a->textureHeight != b->textureHeight
    || a->horizontalWorldLength != b->horizontalWorldLength
    || a->verticalWorldLength != b->verticalWorldLength
    || fabs( a->amplitude - b->amplitude ) > 0.001f
    || fabs( a->gravity - b->gravity ) > 0.1
    || fabs( a->windVelocity - b->windVelocity ) > 0.1 )
    return 0;

  /* compare wind directions by the cosine of the angle between them */
  dot  = a->windDirection[0] * b->windDirection[0];
  dot += a->windDirection[1] * b->windDirection[1];
  mag = sqrt( (double)( b->windDirection[0] * b->windDirection[0] + b->windDirection[1] * b->windDirection[1] )
            * (double)( a->windDirection[0] * a->windDirection[0] + a->windDirection[1] * a->windDirection[1] ) )
      + 1e-10;
  if ( fabs( dot / mag - 1.0f ) > 0.001f )
    return 0;

  return 1;
}

/* ---- R_InitWater  0x005077E0 ----  VERIFIED
 * Reset the water-map cache and (re)build the FFT tables. */
void R_InitWater( void )
{
  s_numWaterMaps = 0;
  FFT_Init();
}

/* ---- R_GetWaterTexture  0x005077F0 ----  VERIFIED
 * Look up (or create) the cached water map for a shader's configuration: reuse
 * an equivalent existing map, else allocate the spectrum, pre-roll it, and
 * create the *water%i normal texture.  Returns the map, or NULL if the image
 * failed. */
waterMap_t *__cdecl R_GetWaterTexture( waterMap_t *config )
{
  waterMap_t *waterMap;
  int         mapIndex;
  int         texelCount;
  void       *pixels;

  for ( mapIndex = 0; mapIndex < s_numWaterMaps; ++mapIndex )
  {
    if ( WatersEquivalent( &s_waterMaps[mapIndex], config ) )
      return &s_waterMaps[mapIndex];
  }

  if ( mapIndex == 16 )
    ri.Error( 1, "\x15" "map uses more than %i waterMap textures", 16 );

  texelCount = config->textureWidth * config->textureHeight;
  waterMap   = &s_waterMaps[mapIndex];

  /* copy the whole parser record (including the uninitialised uploadFrame,
   * which retail leaves for RB_UploadWaterTexture to compare against) before
   * replacing the image and the two spectrum pointers. */
  memcpy( waterMap, config, sizeof( waterMap_t ) );
  s_waterMaps[mapIndex].initialFrequencies = (waterComplex_t *)ri.Hunk_Alloc( 12 * texelCount );
  s_waterMaps[mapIndex].angularFrequencies = (float *)&s_waterMaps[mapIndex].initialFrequencies[texelCount];
  PickWaterFrequencies( waterMap );

  pixels = (void *)ri.Hunk_AllocateTempMemory( 4 * config->textureWidth * config->textureHeight );
  memset( pixels, 0xFF, 4 * config->textureWidth * config->textureHeight );

  /* R_CreateImage is inlined by LTCG here (flags 0: no delayed upload) */
  waterMap->image = (void *)R_CreateImage( va( "*water%i", mapIndex ), 0, 8, (unsigned char *)pixels,
                                           config->textureWidth, config->textureHeight, 6408, 0 );   /* GL_RGBA */
  ri.Hunk_FreeTempMemory( pixels );

  if ( !waterMap->image )
    return 0;

  ++s_numWaterMaps;
  return waterMap;
}

/* ---- RB_UploadWaterTexture  0x00507950 ----  VERIFIED
 * Once per frame, re-derive the water normal map for the current time and blit
 * it into the map's texture on the given texture unit.  The two big locals are
 * the retail 256x256 stack workspaces (spectrum + packed RGBA normals); the
 * byte-typed normal buffer is what gives retail its /GS cookie. */
void __cdecl RB_UploadWaterTexture( waterMap_t *water, int time, int textureUnit )
{
  byte           normalPixels[65536 * 4];  /* 256*256 packed RGBA normals */
  waterComplex_t amplitudes[65536];        /* 256*256 complex working buffer */

  if ( water->uploadFrame != tr_frameCount )
  {
    water->uploadFrame = tr_frameCount;

#ifdef COD1_VC7
    WaterFrequenciesAtTime( amplitudes, water, time * 0.001f );   /* ms -> s */
#else   /* modern: retail's fild/fmul product is exact before the float rounding */
    WaterFrequenciesAtTime( amplitudes, water, (float)( (double)time * 0.001f ) );
#endif
    WaterAmplitudesFromFrequencies( water, amplitudes );
    WaterNormalsFromAmplitudes( (unsigned int *)normalPixels, amplitudes, water );

    /* GL_SelectTexture, inlined by retail */
    if ( glState_currentTmu != textureUnit )
    {
      qglActiveTextureARB( textureUnit + 33984 );          /* GL_TEXTURE0_ARB */
      glState_currentTmu = textureUnit;
    }
    if ( glState_currentClientTmu != textureUnit )
    {
      qglClientActiveTextureARB( textureUnit + 33984 );
      glState_currentClientTmu = textureUnit;
    }

    GL_Bind( (GLenum *)water->image );
    glTexSubImage2D(
      0xDE1u,                                              /* GL_TEXTURE_2D */
      0,
      0,
      0,
      *(unsigned short *)( (char *)water->image + 64 ),    /* image->width */
      *(unsigned short *)( (char *)water->image + 66 ),    /* image->height */
      0x1908u,                                             /* GL_RGBA */
      0x1401u,                                             /* GL_UNSIGNED_BYTE */
      normalPixels );
  }
}
