/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include <malloc.h>

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"         /* typed structs + asserts; after tr_records.h (TRG) */

#include "../jpeg-6/jinclude.h"
#include "../jpeg-6/jpeglib.h"
/* ---- renderer-owned globals ---- */
int rendererImageAllocationCount;   /* 0x014072F0 */
int hackSize;
int r_usedImageFrame;
int hashtable[4096];
int r_usedImageMemory;
int r_usedLightmapMemory;
int r_usedTextureMemory;


int param         = 0x2701;   /* 0x005712F0  gl_filter_min, GL_LINEAR_MIPMAP_NEAREST */
int gl_filter_max  = 0x2601;   /* 0x005712F4  gl_filter_max, GL_LINEAR               */


void CoD_jpeg_mem_src( j_decompress_ptr cinfo, const unsigned char *inbuffer );

typedef struct image_s image_t;   /* full definition below; forward-declared for the prototypes here */

int __cdecl CompareMergableShaders( int shaderA, int shaderB, int mergeImageA, int mergeImageB );
void __cdecl GL_Bind( image_t *image );
void __cdecl GL_SelectTexture( int tmu );
extern int __cdecl GL_CheckErrors( const char *location );
extern int R_FindShader();
float VectorNormalize2( const float *v, float *out );

void __cdecl R_LoadImage( const char *name, int *data, unsigned __int16 *width, unsigned __int16 *height, int *format, unsigned int *hasMipmaps, int readFlag );

extern int j__atol();
extern int jinit_compress_master();
extern int qsort_m();
extern void GLimp_SetGamma( unsigned char *red, unsigned char *green, unsigned char *blue );


#define IMAGE_FLAG_MIPMAP            0x0001
#define IMAGE_FLAG_ALLOW_PICMIP      0x0002
#define IMAGE_FLAG_USE_PICMIP2       0x0004
#define IMAGE_FLAG_LIGHTMAP          0x0008
#define IMAGE_FLAG_CLAMP_S           0x0010
#define IMAGE_FLAG_CLAMP_T           0x0020
#define IMAGE_FLAG_HEIGHT_TO_NORMAL  0x0040
#define IMAGE_FLAG_DELAYED_UPLOAD    0x0080
#define IMAGE_FLAG_NO_TEXTURE_SHEET  0x0100
#define IMAGE_FLAG_COLOR_DEPTH       0x0200
#define IMAGE_FLAG_NO_OVERBRIGHT     0x0400

#define R_MAX_IMAGES            2048   /* R_AllocImage cap test, 0x004EC58D */
#define R_IMAGE_HASH_SIZE       4096
#define R_IMAGE_FIRST_TEXTURE   1024
#define R_IMAGE_NAME_SIZE         64
#define R_MAX_IMAGE_ALLOCATIONS   32

/* image track categories (image->imageTrack); r_imageTrackNames indexes these */
#define R_IMAGE_TRACK_LIGHTMAP   3

/* ------------------------------------------------------------------------- *
 *  image_t (0x78) -- the tr.images record.
 *
 *  R_AllocImage requests exactly 0x78 bytes and R_LoadSingleDelayedImage
 *  copies all 0x78 when it swaps a failed delayed image for the default one.
 *  The +0x68 `link` slot is a shader/sheet handle: the merge-target shader
 *  while IMAGE_FLAG_DELAYED_UPLOAD is set, otherwise NULL or the physical
 *  texture-sheet image.  The +0x6c `state` slot is either the sheet x/y/
 *  rotated triple or the delayed group / groupTriCount pair.
 *
 *  This file owns the FULL layout (tr_backend.c keeps a partial file-local
 *  copy of the same record); the tr_* image globals are declared `int` but
 *  hold an image_t*, so tr_images / imageHashTable cast them here.  Retyping
 *  never moves storage; every offset is pinned so a wrong one is a build error.
 * ------------------------------------------------------------------------- */
struct shader_s;   /* tr_local.h already forward-declares this */

typedef union {
	struct image_s  *textureSheet;    /* sheet image once DELAYED is cleared */
	struct shader_s *delayedShader;   /* merge-target shader while DELAYED set */
} imageLink_t;
COD1_ASSERT_SIZE(imageLink_t, 0x04);

typedef struct {
	unsigned short x;                 /* +0x6c */
	unsigned short y;                 /* +0x6e */
	unsigned char  rotated;           /* +0x70  set when width < height */
} imageSheetState_t;

typedef struct {
	int group;                        /* +0x6c */
	int groupTriCount;                /* +0x70 */
} imageDelayState_t;

typedef union {
	imageSheetState_t sheet;
	imageDelayState_t delay;
} imageState_t;
COD1_ASSERT_SIZE(imageState_t, 0x08);

typedef struct image_s
{
	char            imgName[R_IMAGE_NAME_SIZE]; /* +0x00  strcpy in R_AllocImage */
	unsigned short  width;          /* +0x40 */
	unsigned short  height;         /* +0x42 */
	unsigned short  uploadWidth;    /* +0x44  out-param of UploadImage */
	unsigned short  uploadHeight;   /* +0x46 */
	int             cardMemory;     /* +0x48  += level cost */
	int             textureMemory;  /* +0x4c */
	int             imageTrack;     /* +0x50 */
	unsigned int    target;         /* +0x54  glBindTexture(target,0) */
	unsigned int    texnum;         /* +0x58  = index + 1024 */
	int             frameUsed;      /* +0x5c */
	unsigned int    internalFormat; /* +0x60  out-param of UploadImage */
	unsigned int    flags;          /* +0x64  IMAGE_FLAG_* */
	imageLink_t     link;           /* +0x68 */
	imageState_t    state;          /* +0x6c */
	struct image_s *hashNext;       /* +0x74 */
} image_t;
COD1_ASSERT_SIZE(image_t, 0x78);
ASSERT_OFF(image_t, width, 0x40);
ASSERT_OFF(image_t, uploadWidth, 0x44);
ASSERT_OFF(image_t, cardMemory, 0x48);
ASSERT_OFF(image_t, textureMemory, 0x4c);
ASSERT_OFF(image_t, imageTrack, 0x50);
ASSERT_OFF(image_t, target, 0x54);
ASSERT_OFF(image_t, texnum, 0x58);
ASSERT_OFF(image_t, frameUsed, 0x5c);
ASSERT_OFF(image_t, internalFormat, 0x60);
ASSERT_OFF(image_t, flags, 0x64);
ASSERT_OFF(image_t, link, 0x68);
ASSERT_OFF(image_t, state, 0x6c);
ASSERT_OFF(image_t, state.sheet.rotated, 0x70);
ASSERT_OFF(image_t, hashNext, 0x74);

/* ------------------------------------------------------------------------- *
 *  imageGroupNode_t (0x18) -- texture-sheet packing list/tree node.  A
 *  nonterminal element uses `u.child`; the terminal one (next == NULL) uses
 *  `u.image`.  MergeImageList builds these lists/trees and UploadImageGroup_r
 *  consumes the same tagged shape.  `width` holds the larger of the source
 *  dimensions, `height` the smaller (the packer rotates so width >= height).
 * ------------------------------------------------------------------------- */
typedef struct imageGroupNode_s imageGroupNode_t;
struct imageGroupNode_s {
	union {
		imageGroupNode_t *child;      /* +0x00  nonterminal */
		image_t          *image;      /* +0x00  terminal (next == NULL) */
	} u;
	imageGroupNode_t *next;           /* +0x04 */
	int               triangleCount;  /* +0x08  packing priority, not pixel area */
	int               group;          /* +0x0c */
	int               width;          /* +0x10  larger dimension */
	int               height;         /* +0x14  smaller dimension */
};
COD1_ASSERT_SIZE(imageGroupNode_t, 0x18);
ASSERT_OFF(imageGroupNode_t, next, 0x04);
ASSERT_OFF(imageGroupNode_t, triangleCount, 0x08);
ASSERT_OFF(imageGroupNode_t, group, 0x0c);
ASSERT_OFF(imageGroupNode_t, width, 0x10);
ASSERT_OFF(imageGroupNode_t, height, 0x14);

#define tr_images ( (image_t **)(void *)tr_imageSlots )

#define imageHashTable ( (image_t **)(void *)hashtable )

unsigned char s_gammatable[ 256 ];
unsigned char s_intensitytable[ 256 ];

#define R_IMAGE_ALLOCATION_FILE        0
#define R_IMAGE_ALLOCATION_TEMP_MEMORY 1

typedef struct
{
	int   kind;
	void *memory;
} imageAllocation_t;

imageAllocation_t r_imageAllocations[ R_MAX_IMAGE_ALLOCATIONS ];

static unsigned char mipBlendColors[16][4] = {
	{   0,   0,   0,   0 },
	{ 255,   0,   0, 128 },
	{   0, 255,   0, 128 },
	{   0,   0, 255, 128 },
	{ 255, 255,   0, 128 },
	{   0, 255, 255, 128 },
	{ 255,   0, 255, 128 },
	{ 255,   0,   0, 128 },
	{   0, 255,   0, 128 },
	{   0,   0, 255, 128 },
	{ 255, 128,   0, 128 },
	{   0, 255, 128, 128 },
	{ 128,   0, 255, 128 },
	{ 255,   0,   0, 128 },
	{   0, 255,   0, 128 },
	{   0,   0, 255, 128 }
};

#define ri_Hunk_Alloc(sz) \
	( (void *)( (*(int (__cdecl **)(int))&ri.Hunk_Alloc)( (int)(sz) ) ) )
#define ri_Hunk_AllocateTempMemory(sz) \
	( (void *)( (*(int (__cdecl **)(int))&ri.Hunk_AllocateTempMemory)( (int)(sz) ) ) )
#define ri_Hunk_FreeTempMemory(p) \
	( (*(void (__cdecl **)(void *))&ri.Hunk_FreeTempMemory)( (p) ) )
#define ri_FS_ReadFile(name, buf) \
	( (*(int (__cdecl **)(const char *, void **))&ri.FS_ReadFile)( (name), (buf) ) )
#define ri_FS_FreeFile(p) \
	( (*(void (__cdecl **)(void *))&ri.FS_FreeFile)( (p) ) )

#define R_CVAR(g) ( *(cvar_t **)&(g) )

int  __cdecl R_PicmipForImageFlags( int flags );
void __cdecl R_RememberImageAllocation( void *memory, int kind );

/* ---- R_PicmipForImageFlags  0x004EA380 ----  VERIFIED */
int __cdecl R_PicmipForImageFlags( int flags )
{
	int picmip;
	int clamped;

	if ( flags & IMAGE_FLAG_USE_PICMIP2 )
		picmip = R_CVAR( r_picmip2 )->integer;
	else
		picmip = R_CVAR( r_picmip )->integer;

	if ( !( flags & IMAGE_FLAG_ALLOW_PICMIP ) )
		return 0;

	clamped = picmip > 3 ? 3 : picmip;
	return ~( clamped >> 31 ) & clamped;
}

/* ---- R_RememberImageAllocation  0x004EA3C0 ----  VERIFIED */
void __cdecl R_RememberImageAllocation( void *memory, int kind )
{
	r_imageAllocations[ rendererImageAllocationCount ].kind   = kind;
	r_imageAllocations[ rendererImageAllocationCount ].memory = memory;
	++rendererImageAllocationCount;
}

/* ---- R_ResetImageAllocations  0x004EA3E0 ----  VERIFIED */
void R_ResetImageAllocations( void )
{
	rendererImageAllocationCount = 0;
}

/* ---- R_FreeImageAllocations  0x004EA3F0 ----  VERIFIED */
void R_FreeImageAllocations( void )
{
	while ( rendererImageAllocationCount > 0 )
	{
		imageAllocation_t *a = &r_imageAllocations[ --rendererImageAllocationCount ];

		switch ( a->kind )
		{
		case R_IMAGE_ALLOCATION_FILE:
			ri_FS_FreeFile( a->memory );
			break;
		case R_IMAGE_ALLOCATION_TEMP_MEMORY:
			ri_Hunk_FreeTempMemory( a->memory );
			break;
		}
	}
}

/* ---- R_AllocTempMemory  0x004EA440 ----  VERIFIED */
void * __cdecl R_AllocTempMemory( int size, const char *name )
{
	void *memory = ri_Hunk_AllocateTempMemory( size );

	if ( memory )
		R_RememberImageAllocation( memory, R_IMAGE_ALLOCATION_TEMP_MEMORY );
	return memory;
}

/* ---- R_ReadFile  0x004EA470 ----  VERIFIED */
int __cdecl R_ReadFile( const char *name, void **buffer )
{
	int len = ri_FS_ReadFile( name, buffer );

	if ( len >= 0 )
		R_RememberImageAllocation( *buffer, R_IMAGE_ALLOCATION_FILE );
	return len;
}

/* ---- R_GammaCorrect  0x004EA4B0 ----  VERIFIED */
void __cdecl R_GammaCorrect( unsigned char *buffer, int bytes )
{
	int i;

	for ( i = 0; i < bytes; ++i )
		buffer[ i ] = s_gammatable[ buffer[ i ] ];
}

/* ---- tr_image_generateHashValue  0x004EA4E0 ----  [HIGH] */
/* Q3 generateHashValue: hash = sum over path chars (until '.') of
 * tolower(c) * (119 + index), masked to 12 bits.  VC7 strength-reduces i into
 * the walking pointer (119 - name + p == 119 + i).  '\' folds to '/'. */
int __cdecl tr_image_generateHashValue( char *name )
{
	int  i;
	long hash;
	int  letter;

	hash = 0;
	i = 0;
	while ( name[i] != '\0' )
	{
		letter = tolower( name[i] );
		if ( letter == '.' )
			break;
		if ( letter == '\\' )
			letter = '/';
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= 0xFFF;
	return hash;
}

typedef struct textureMode_s
{
	const char *name;
	int         minimize;
	int         maximize;
} textureMode_t;

static const textureMode_t r_textureModes[6] =      /* 0x005712F8 */
{
	{ "GL_NEAREST",                0x2600, 0x2600 },
	{ "GL_LINEAR",                 0x2601, 0x2601 },
	{ "GL_NEAREST_MIPMAP_NEAREST", 0x2700, 0x2600 },
	{ "GL_LINEAR_MIPMAP_NEAREST",  0x2701, 0x2601 },
	{ "GL_NEAREST_MIPMAP_LINEAR",  0x2702, 0x2600 },
	{ "GL_LINEAR_MIPMAP_LINEAR",   0x2703, 0x2601 }
};

static const char *const r_imageTrackNames[11] =    /* 0x00571340 */
{
	"misc  ", "debug ", "ui    ", "lmap  ", "effect", "hud   ",
	"vmodel", "model ", "world ", "f/x ", "$tex+?"    /* "f/x " is 4 chars in retail (0x0054C108) */
};

/* ---- GL_TextureMode  0x004EA530 ----  [HIGH] */
void __cdecl GL_TextureMode( const char *string )
{
	int i;

	for ( i = 0; i < 6; ++i )
	{
		if ( !Q_stricmp( r_textureModes[i].name, string ) )
			break;
	}
	if ( i == 6 )
	{
		ri.Printf( 0, "bad filter name\n" );
		return;
	}

	param          = r_textureModes[i].minimize;   /* gl_filter_min */
	gl_filter_max  = r_textureModes[i].maximize;

	for ( i = 0; i < tr_numImages; ++i )
	{
		image_t *image = tr_images[i];

		if ( image->flags & IMAGE_FLAG_MIPMAP )
		{
			GL_Bind( image );
			glTexParameteri( image->target, 0x2801u , param );
			glTexParameteri( image->target, 0x2800u , gl_filter_max );
		}
	}
}

/* ---- R_SumOfUsedImages  0x004EA600 ----  VERIFIED */
void __cdecl R_SumOfUsedImages( int *imageMemoryOut, unsigned int *lightmapMemoryOut,
                                   unsigned int *textureMemoryOut )
{
	int      i;
	image_t *image;

	if ( r_usedImageFrame != tr_frameCount )
	{
		r_usedImageFrame     = tr_frameCount;
		r_usedImageMemory    = 0;
		r_usedLightmapMemory = 0;
		r_usedTextureMemory  = 0;

		for ( i = 0; i < tr_numImages; i++ )
		{
			image = ( (image_t **)(void *)tr_imageSlots )[i];
			if ( image->frameUsed == tr_frameCount || image->frameUsed == tr_frameCount - 1 )
			{
				r_usedImageMemory += image->cardMemory;
				if ( image->imageTrack == 3 )
					r_usedLightmapMemory += image->cardMemory;
				r_usedTextureMemory += image->textureMemory;
			}
		}

		if ( com_statmon->integer && r_usedTextureMemory > 0x1400000 )
			StatMon_Warning( 6, 3000, "gfx/2d/warning@textures.jpg" );
	}

	if ( imageMemoryOut )
		*imageMemoryOut = r_usedImageMemory;
	if ( lightmapMemoryOut )
		*lightmapMemoryOut = r_usedLightmapMemory;
	if ( textureMemoryOut )
		*textureMemoryOut = r_usedTextureMemory;
}

/* ---- imagecompare  0x004EA750 ----  [HIGH] */
/* qsort comparator over image indices: sort by imageTrack, then cardMemory */
int __cdecl imagecompare( unsigned int *indexA, unsigned int *indexB )
{
	image_t *imageA = tr_images[ *indexA ];
	image_t *imageB = tr_images[ *indexB ];

	if ( imageA->imageTrack < imageB->imageTrack )
		return -1;
	if ( imageA->imageTrack > imageB->imageTrack )
		return 1;
	return imageA->cardMemory - imageB->cardMemory;
}

/* ---- RE_GetImageMemory  0x004EA790 ----  [HIGH] */
int RE_GetImageMemory()
{
  return tr_imageMemory;
}

/* ---- R_ImageList_f  0x004EA7A0 ----  VERIFIED */
void R_ImageList_f()
{
  int          i;
  image_t     *image;
  int          texels;
  int          totalBytes;
  int          trackBytes[11];
  int          sorted[2048];
  const char  *yesno[] = { "no ", "yes" };

  totalBytes = 0;
  memset( trackBytes, 0, sizeof( trackBytes ) );

  for ( i = 0; i < tr_numImages; i++ )
    sorted[i] = i;

  qsort_m( (unsigned int)sorted, tr_numImages, 4u,
           (int (__cdecl *)(unsigned int, byte *))imagecompare );

  ri.Printf( 0, "\n      -w-- -h-- -mm- -if--- wrap- -type- -KB--- --name-------\n" );

  texels = 0;
  for ( i = 0; i < tr_numImages; i++ )
  {
    image = ( (image_t **)(void *)tr_imageSlots )[ sorted[i] ];

    texels += image->uploadWidth * image->uploadHeight;
    ri.Printf( 0, "%4i: %4i %4i  %s  ",
               sorted[i], image->uploadWidth, image->uploadHeight, yesno[ image->flags & 0x0001 ] );

    switch ( (int)image->internalFormat )
    {
      case 1:
        ri.Printf( 0, "I    " );
        break;
      case 2:
        ri.Printf( 0, "IA   " );
        break;
      case 3:
        ri.Printf( 0, "RGB  " );
        break;
      case 4:
      case 6408:
        ri.Printf( 0, "RGBA " );
        break;
      case 32856:
        ri.Printf( 0, "RGBA8" );
        break;
      case 32849:
        ri.Printf( 0, "RGB8 " );
        break;
      case 33697:
        ri.Printf( 0, "S3TC4" );
        break;
      case 32854:
        ri.Printf( 0, "RGBA4" );
        break;
      case 32848:
        ri.Printf( 0, "RGB5 " );
        break;
      case 33776:
      case 33777:
        ri.Printf( 0, "DXT1 " );
        break;
      case 33778:
        ri.Printf( 0, "DXT3 " );
        break;
      case 33779:
        ri.Printf( 0, "DXT5 " );
        break;
      default:
        ri.Printf( 0, "?????" );
        break;
    }

    switch ( image->flags & 0x30 )
    {
      case 0x00:
        ri.Printf( 0, " repeat " );
        break;
      case 0x10:
        ri.Printf( 0, " clampx " );
        break;
      case 0x20:
        ri.Printf( 0, " clampy " );
        break;
      case 0x30:
        ri.Printf( 0, " clamp  " );
        break;
    }

    ri.Printf( 0, " %s %6.1f %s\n", r_imageTrackNames[ image->imageTrack ], image->cardMemory / 1024.0f, image->imgName );
    totalBytes += image->cardMemory;
    trackBytes[ image->imageTrack ] += image->cardMemory;
  }

  ri.Printf( 0, " ---------\n" );
  ri.Printf( 0, " %i total texels (not including mipmaps)\n", texels );
  ri.Printf( 0, " %i total images\n", tr_numImages );
  ri.Printf( 0, " %.2f MB total image size\n\n", totalBytes / ( 1024.0f * 1024.0f ) );
  for ( i = 0; i < 11; i++ )
    ri.Printf( 0, "%s: %.2f MB\n", r_imageTrackNames[i], trackBytes[i] / ( 1024.0f * 1024.0f ) );
}

/* ---- R_LightScaleTexture  0x004EAA90 ----  VERIFIED */
void __cdecl R_LightScaleTexture( unsigned char *pixels, int width, int height,
                                  int onlyGamma, int format )
{
	if ( format != 0x1908 )                      /* GL_RGBA */
		return;

	if ( onlyGamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int   i, c;
			byte *p;

			p = pixels;
			c = width * height;
			for ( i = 0; i < c; i++, p += 4 )
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int   i, c;
		byte *p;

		p = pixels;
		c = width * height;
		if ( glConfig.deviceSupportsGamma )
		{
			for ( i = 0; i < c; i++, p += 4 )
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for ( i = 0; i < c; i++, p += 4 )
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}

/* ---- GetCardMemoryAmount  0x004EABA0 ----  VERIFIED */
/* (format, width, height): retail callers load width before format. */
int __cdecl GetCardMemoryAmount(int format, int width, int height)
{
  switch ( format )
  {
    case 3:
    case 4:
    case 6408:    /* GL_RGBA */
    case 32849:   /* GL_RGB8 */
    case 32856:   /* GL_RGBA8 */
    case 32992:   /* GL_BGR_EXT */
    case 32993:   /* GL_BGRA_EXT */
      return 4 * width * height;
    case 6407:    /* GL_RGB */
      return ( glConfig_colorBits > 16 ? 4 : 2 ) * width * height;
    case 6409:    /* GL_LUMINANCE */
      return width * height;
    case 32848:   /* GL_RGB5 */
    case 32854:   /* GL_RGBA4 */
      return 2 * width * height;
    case 33776:   /* GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0 */
    case 33777:   /* GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1 */
      return 8 * ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
    case 33778:   /* GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2 */
    case 33779:   /* GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3 */
      return 16 * ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 );
    default:
      return 0;
  }
}

/* ---- R_MipMap2  0x004EAC90 ----  VERIFIED */
/* In-place quarter-size downsample with a proper 4x4 linear filter (RTCW
 * R_MipMap2).  The retail form folds the wrap masks and tap addresses into a
 * long chain of temporaries; the weight pattern is corners*1 / edges*2 /
 * centre*4, summed and divided by 36.  Called as R_MipMap2(data,width,height). */
void __cdecl R_MipMap2( unsigned char *data, int inWidth, int inHeight )
{
	unsigned int *in        = (unsigned int *)data;
	int           outWidth  = inWidth  >> 1;
	int           outHeight = inHeight >> 1;
	int           tempBytes = 4 * outWidth * outHeight;
	unsigned int *temp      = (unsigned int *)( (void *)( (*(int (__cdecl **)(int))&ri.Hunk_AllocateTempMemory)( (int)(tempBytes) ) ) );
	int           widthMask  = inWidth  - 1;
	int           heightMask = inHeight - 1;
	int           i, j, k;

	for ( i = 0; i < outHeight; ++i )
	{
		for ( j = 0; j < outWidth; ++j )
		{
			unsigned char *out = (unsigned char *)( temp + i * outWidth + j );

			for ( k = 0; k < 4; ++k )
			{
				int total =
				    1 * ((unsigned char *)&in[((i*2-1)&heightMask)*inWidth + ((j*2-1)&widthMask)])[k]
				  + 2 * ((unsigned char *)&in[((i*2-1)&heightMask)*inWidth + ((j*2  )&widthMask)])[k]
				  + 2 * ((unsigned char *)&in[((i*2-1)&heightMask)*inWidth + ((j*2+1)&widthMask)])[k]
				  + 1 * ((unsigned char *)&in[((i*2-1)&heightMask)*inWidth + ((j*2+2)&widthMask)])[k]

				  + 2 * ((unsigned char *)&in[((i*2  )&heightMask)*inWidth + ((j*2-1)&widthMask)])[k]
				  + 4 * ((unsigned char *)&in[((i*2  )&heightMask)*inWidth + ((j*2  )&widthMask)])[k]
				  + 4 * ((unsigned char *)&in[((i*2  )&heightMask)*inWidth + ((j*2+1)&widthMask)])[k]
				  + 2 * ((unsigned char *)&in[((i*2  )&heightMask)*inWidth + ((j*2+2)&widthMask)])[k]

				  + 2 * ((unsigned char *)&in[((i*2+1)&heightMask)*inWidth + ((j*2-1)&widthMask)])[k]
				  + 4 * ((unsigned char *)&in[((i*2+1)&heightMask)*inWidth + ((j*2  )&widthMask)])[k]
				  + 4 * ((unsigned char *)&in[((i*2+1)&heightMask)*inWidth + ((j*2+1)&widthMask)])[k]
				  + 2 * ((unsigned char *)&in[((i*2+1)&heightMask)*inWidth + ((j*2+2)&widthMask)])[k]

				  + 1 * ((unsigned char *)&in[((i*2+2)&heightMask)*inWidth + ((j*2-1)&widthMask)])[k]
				  + 2 * ((unsigned char *)&in[((i*2+2)&heightMask)*inWidth + ((j*2  )&widthMask)])[k]
				  + 2 * ((unsigned char *)&in[((i*2+2)&heightMask)*inWidth + ((j*2+1)&widthMask)])[k]
				  + 1 * ((unsigned char *)&in[((i*2+2)&heightMask)*inWidth + ((j*2+2)&widthMask)])[k];

				out[k] = (unsigned char)( total / 36 );
			}
		}
	}

	Com_Memcpy( data, temp, tempBytes );
	( (*(void (__cdecl **)(void *))&ri.Hunk_FreeTempMemory)( (temp) ) );

}

/* ---- R_MipMap8  0x004EB120 ----  VERIFIED */
/* In-place quarter-size box filter for single-byte (alpha/luminance) images.
 * 2D path averages each 2x2 block; when one dimension is 1 it averages pairs
 * along the other.  Called as R_MipMap8(data,width,height).  The int return is
 * a dead-EAX value (0 on the 2D path, the last computed average otherwise). */
void __cdecl R_MipMap8( unsigned char *data, int width, int height )
{
	int            i, j;
	unsigned char *out;
	int            row;

	if ( width == 1 && height == 1 )
		return;

	row = width;
	out = data;
	width >>= 1;
	height >>= 1;

	if ( width && height )
	{
		for ( i = 0; i < height; i++, data += row )
		{
			for ( j = 0; j < width; j++, out++, data += 2 )
				out[0] = (unsigned char)( ( data[0] + data[1] + data[row] + data[row + 1] ) >> 2 );
		}
	}
	else
	{
		width += height;
		for ( i = 0; i < width; i++, out++, data += 2 )
			out[0] = (unsigned char)( ( data[0] + data[1] ) >> 1 );
	}
}

/* ---- R_MipMap  0x004EB1D0 ----  VERIFIED
 * Quarter one mip level in place and return the start of the next level's
 * data.  RGBA levels may be replaced by a custom "name#N+1" image; otherwise
 * they are box filtered (r_simpleMipMaps), optionally weighted toward the
 * top-left texel by r_weightMipMaps, or 4x4 filtered by R_MipMap2. */
unsigned __int8 *__cdecl R_MipMap(unsigned int format, unsigned __int8 *data,
                                  char *name, int width, int height, int flags)
{
	int               i, j;
	unsigned __int8  *out;
	unsigned __int8  *result;
	int               row;
	float             weight, blend;
	char             *hash;
	char             *p;
	unsigned __int8  *pic;
	unsigned __int16  picWidth;
	unsigned __int16  picHeight;
	int               picFormat;
	int               picHasMipMaps;
	char              tail[64];

	switch ( format )
	{
	case 0x1908:   /* GL_RGBA */
		break;
	case 0x1909:   /* GL_LUMINANCE */
		R_MipMap8( data, width, height );
		return data;
	case 0x83F0:   /* GL_COMPRESSED_RGB_S3TC_DXT1_EXT */
	case 0x83F1:   /* GL_COMPRESSED_RGBA_S3TC_DXT1_EXT */
	case 0x83F2:   /* GL_COMPRESSED_RGBA_S3TC_DXT3_EXT */
	case 0x83F3:   /* GL_COMPRESSED_RGBA_S3TC_DXT5_EXT */
		return &data[ GetCardMemoryAmount( format, width, height ) ];
	default:
		return data;
	}

	hash = strstr( name, "#" );
	if ( hash )
	{
		for ( p = hash + 1; isdigit( *p ); p++ )
			;
		strcpy( tail, p );
		*p = 0;
		i = j__atol( hash + 1 );
		sprintf( hash + 1, "%i%s", i + 1, tail );

		R_LoadImage( name, (int *)&pic, &picWidth, &picHeight, &picFormat, (unsigned int *)&picHasMipMaps, flags );
		if ( pic )
		{
			if ( picWidth == ( width + 1 ) >> 1 && picHeight == ( height + 1 ) >> 1 )
			{
				Com_Memcpy( data, pic, 4 * picWidth * picHeight );
				return data;
			}
			ri.Printf( 2,
			           "WARNING: custom mipmap image '%s' should have dimensions %i x %i instead of %i x %i, refusing to load\n",
			           name, ( width + 1 ) >> 1, ( height + 1 ) >> 1, picWidth, picHeight );
			return 0;
		}
	}

	if ( !r_simpleMipMaps->integer )
	{
		R_MipMap2( data, width, height );
		return data;
	}

	if ( width == 1 && height == 1 )
		return data;

	row = width * 4;
	out = data;
	width >>= 1;
	height >>= 1;
	result = data;

	if ( width && height )
	{
		weight = r_weightMipMaps->value;
		if ( weight <= 0.0f || weight > 1.0f )
		{
			for ( i = 0; i < height; i++, data += row )
			{
				for ( j = 0; j < width; j++, out += 4, data += 8 )
				{
					out[0] = (unsigned __int8)( ( data[0] + data[4] + data[row + 0] + data[row + 4] ) >> 2 );
					out[1] = (unsigned __int8)( ( data[1] + data[5] + data[row + 1] + data[row + 5] ) >> 2 );
					out[2] = (unsigned __int8)( ( data[2] + data[6] + data[row + 2] + data[row + 6] ) >> 2 );
					out[3] = (unsigned __int8)( ( data[3] + data[7] + data[row + 3] + data[row + 7] ) >> 2 );
				}
			}
		}
		else
		{
			blend = ( 1.0f - weight ) * 0.25f;
			for ( i = 0; i < height; i++, data += row )
			{
				for ( j = 0; j < width; j++, out += 4, data += 8 )
				{
					out[0] = (unsigned __int8)( ( data[0] + data[4] + data[row + 4] + data[row + 0] ) * blend + data[0] * weight );
					out[1] = (unsigned __int8)( ( data[1] + data[5] + data[row + 5] + data[row + 1] ) * blend + data[1] * weight );
					out[2] = (unsigned __int8)( ( data[2] + data[6] + data[row + 6] + data[row + 2] ) * blend + data[2] * weight );
					out[3] = (unsigned __int8)( ( data[3] + data[7] + data[row + 7] + data[row + 3] ) * blend + data[3] * weight );
				}
			}
		}
		return result;
	}

	width += height;   /* one dimension is already 1: average along the other */
	for ( i = 0; i < width; i++, out += 4, data += 8 )
	{
		out[0] = (unsigned __int8)( ( data[0] + data[4] ) >> 1 );
		out[1] = (unsigned __int8)( ( data[1] + data[5] ) >> 1 );
		out[2] = (unsigned __int8)( ( data[2] + data[6] ) >> 1 );
		out[3] = (unsigned __int8)( ( data[3] + data[7] ) >> 1 );
	}
	return result;
}

/* ---- R_BlendOverTexture_RGBA  0x004EBB90 ----  VERIFIED */
unsigned __int8 *__cdecl R_BlendOverTexture_RGBA(
        unsigned __int8 *pixels,
        int count,
        unsigned __int8 *blendColor)
{
  int i;
  int inverseAlpha;
  int premult[3];

  inverseAlpha = 255 - blendColor[3];
  premult[0] = blendColor[0] * blendColor[3];
  premult[1] = blendColor[1] * blendColor[3];
  premult[2] = blendColor[2] * blendColor[3];

  for ( i = 0; i < count; i++, pixels += 4 )
  {
    pixels[0] = (unsigned __int8)( ( pixels[0] * inverseAlpha + premult[0] ) >> 8 );
    pixels[1] = (unsigned __int8)( ( pixels[1] * inverseAlpha + premult[1] ) >> 8 );
    pixels[2] = (unsigned __int8)( ( pixels[2] * inverseAlpha + premult[2] ) >> 8 );
  }
  return pixels;
}

/* ---- R_BlendOverTexture_S3TC  0x004EBC00 ----  VERIFIED */
/* Blends the two RGB565 endpoint colours of each DXT colour block toward
 * blendColor.  For 16-byte blocks the endpoints are swapped back (and the
 * 0/1 indices flipped) when the blend reversed their order. */
void __cdecl R_BlendOverTexture_S3TC( int pixelCount, const unsigned char *blendColor,
                                      unsigned char *data, int stride )
{
  int alpha;
  int inverseAlpha;
  int premult[3];
  int p, i;
  unsigned short oldColor[2];
  unsigned short newColor[2];
  int r[2], g[2], b[2];

  alpha        = blendColor[3];
  premult[0]   = alpha * ( ( blendColor[0] >> 3 ) << 11 );
  premult[1]   = alpha * ( ( blendColor[1] >> 2 ) << 5 );
  premult[2]   = alpha * ( blendColor[2] >> 3 );
  inverseAlpha = 255 - alpha;

  for ( p = 0; p < pixelCount; p += 16 )
  {
    for ( i = 0; i < 2; i++ )
    {
      oldColor[i] = ( (unsigned short *)data )[i];
      r[i] = ( ( oldColor[i] & 0xF800 ) * inverseAlpha + premult[0] ) >> 8 & 0xF800;
      g[i] = ( ( oldColor[i] & 0x07E0 ) * inverseAlpha + premult[1] ) >> 8 & 0x07E0;
      b[i] = ( ( oldColor[i] & 0x001F ) * inverseAlpha + premult[2] ) >> 8 & 0x001F;
      newColor[i] = (unsigned short)( g[i] | b[i] | r[i] );
    }
    if ( stride == 8 || ( oldColor[1] < oldColor[0] ) == ( newColor[1] < newColor[0] ) )
    {
      ( (unsigned short *)data )[0] = newColor[0];
      ( (unsigned short *)data )[1] = newColor[1];
    }
    else
    {
      ( (unsigned short *)data )[0] = newColor[1];
      ( (unsigned short *)data )[1] = newColor[0];
      ( (unsigned int *)data )[1] ^= ( ~( (unsigned int *)data )[1] >> 1 ) & 0x55555555;
    }
    data += stride;
  }
}

/* ---- R_BlendOverTexture  0x004EBD60 ----  VERIFIED */
void __cdecl R_BlendOverTexture(
        unsigned char *pixels,
        int pixelCount,
        unsigned char *blendColor,
        unsigned int format)
{
  switch ( format )
  {
    case 6408:      /* GL_RGBA */
      R_BlendOverTexture_RGBA(pixels, pixelCount, blendColor);
      break;
    case 0x83F0:    /* DXT1 */
    case 0x83F1:
      R_BlendOverTexture_S3TC(pixelCount, blendColor, pixels, 8);
      break;
    case 0x83F2:    /* DXT3 / DXT5: colour block follows the 8-byte alpha block */
    case 0x83F3:
      R_BlendOverTexture_S3TC(pixelCount, blendColor, pixels + 8, 16);
      break;
  }
}

/* ---- R_TexImage2D  0x004EBDC0 ----  VERIFIED */
void __cdecl R_TexImage2D( unsigned int target, int level,
                           int internalFormat, int width, int height,
                           unsigned int format, const void *pixels )
{
	qglGetError_0();

	if ( internalFormat >= 0x83F0 && internalFormat <= 0x83F3 )
	{
		qglCompressedTexImage2DARB(
			target, level, internalFormat, width, height, 0,
			GetCardMemoryAmount( internalFormat, width, height ),
			pixels );
	}
	else
	{
		glTexImage2D( target, level, internalFormat, width, height, 0,
		              format, 0x1401u , pixels );
	}

	qglGetError_0();
}

/* ---- PickInternalFormat  0x004EBE30 ----  VERIFIED */
int __cdecl PickInternalFormat( const unsigned char *pixels, int format,
                                int width, int height, int flags,
                                int isLightmap )
{
	int count;
	int i;
	int samples;

	if ( format != 0x1908  )
		return format;

	if ( isLightmap )
		return 0x8058;

	if ( flags & IMAGE_FLAG_COLOR_DEPTH )
	{
		switch ( glConfig_colorBits )
		{
		case 32:
			return 0x1908;
		case 24:
			return 0x1907;
		case 15:
		case 16:
			return 0x8050;
		default:
			break;
		}
	}

	samples = 3;
	count   = width * height;
	for ( i = 0; i < count; ++i )
	{
		if ( pixels[ 4 * i + 3 ] != 0xFF )
		{
			samples = 4;
			break;
		}
	}

	if ( samples == 3 )
	{
		if ( R_CVAR( r_texturebits )->integer == 16 )
			return 0x8050;
		if ( R_CVAR( r_texturebits )->integer == 32 )
			return 0x8051;
		return 3;
	}
	if ( R_CVAR( r_texturebits )->integer == 16 )
		return 0x8056;
	if ( R_CVAR( r_texturebits )->integer == 32 )
		return 0x8058;
	return 4;
}

/* ---- UploadImage  0x004EBF20 ----  VERIFIED
 * RTCW Upload32 shape: a full-size non-mipmapped image is uploaded directly;
 * otherwise it is mipped down to the scaled size, light-scaled, uploaded, and
 * (when mipmapped) each further level is generated and uploaded.  The card and
 * texture memory totals skip the levels that picmip would have dropped. */
int __cdecl UploadImage( const char *name, unsigned char *pixels,
                         unsigned int textureTarget, unsigned int uploadTarget,
                         unsigned int format, int width, int height,
                         int flags, int isLightmap,
                         unsigned int *internalFormat,
                         unsigned short *uploadWidth,
                         unsigned short *uploadHeight,
                         int *cardMemory, int *textureMemory )
{
	char mipName[ 64 ];
	int  scaledWidth;
	int  scaledHeight;
	int  picmip;
	int  textureMipSkips;
	int  selectedFormat;
	int  mipmap;
	int  levelCost;
	int  level;

	strcpy( mipName, name );
	*cardMemory    = 0;
	*textureMemory = 0;

	if ( ( ( width - 1 ) & width ) != 0 || ( ( height - 1 ) & height ) != 0 )
	{
		ri.Printf( 2, "WARNING: image '%s' is %i x %i, which is not a power of 2 on both sides\n",
		           name, width, height );
		return 0;
	}

	picmip = R_PicmipForImageFlags( flags );

	if ( flags & IMAGE_FLAG_ALLOW_PICMIP )
	{
		textureMipSkips = 2 - picmip < 0 ? 0 : 2 - picmip;
	}
	else
	{
		textureMipSkips = 0;
	}

	scaledWidth  = width  >> picmip;
	scaledHeight = height >> picmip;
	while ( scaledWidth > glConfig_maxTextureSize || scaledHeight > glConfig_maxTextureSize )
	{
		scaledWidth  >>= 1;
		scaledHeight >>= 1;
	}
	if ( scaledWidth < 1 )
		scaledWidth = 1;
	if ( scaledHeight < 1 )
		scaledHeight = 1;

	selectedFormat = PickInternalFormat( pixels, (int)format, width, height,
	                                     flags, isLightmap );

	if ( scaledWidth == width && scaledHeight == height )
	{
		mipmap = flags & IMAGE_FLAG_MIPMAP;
		if ( !( flags & IMAGE_FLAG_MIPMAP ) )
		{
			R_TexImage2D( uploadTarget, 0, (unsigned int)selectedFormat,
			              scaledWidth, scaledHeight, format, pixels );

			*uploadWidth    = (unsigned short)scaledWidth;
			*uploadHeight   = (unsigned short)scaledHeight;
			*internalFormat = (unsigned int)selectedFormat;

			levelCost = GetCardMemoryAmount( selectedFormat, scaledWidth, scaledHeight );
			*cardMemory    += levelCost;
			*textureMemory += levelCost;
			goto done;
		}
	}
	else
	{
		/* use the normal mip-mapping function to go down from here */
		while ( width > scaledWidth || height > scaledHeight )
		{
			pixels = R_MipMap( format, pixels, mipName, width, height, 1 );
			if ( !pixels )
				return 0;

			width  >>= 1;
			height >>= 1;
			if ( width < 1 )
				width = 1;
			if ( height < 1 )
				height = 1;
		}
	}

	R_LightScaleTexture( pixels, scaledWidth, scaledHeight, !( flags & IMAGE_FLAG_MIPMAP ), (int)format );

	*uploadWidth    = (unsigned short)scaledWidth;
	*uploadHeight   = (unsigned short)scaledHeight;
	*internalFormat = (unsigned int)selectedFormat;

	levelCost = GetCardMemoryAmount( selectedFormat, scaledWidth, scaledHeight );
	*cardMemory += levelCost;

	if ( textureMipSkips == 0 )
	{
		*textureMemory += levelCost;
	}
	else if ( flags & IMAGE_FLAG_MIPMAP )
	{
		--textureMipSkips;
	}
	else
	{
		*textureMemory += levelCost >> ( 2 * textureMipSkips );
	}

	R_TexImage2D( uploadTarget, 0, (unsigned int)selectedFormat,
	              scaledWidth, scaledHeight, format, pixels );

	mipmap = flags & IMAGE_FLAG_MIPMAP;
	if ( flags & IMAGE_FLAG_MIPMAP )
	{
		level = 0;

		while ( scaledWidth > 1 || scaledHeight > 1 )
		{
			pixels = R_MipMap( format, pixels, mipName, scaledWidth, scaledHeight, 1 );
			if ( !pixels )
				return 0;

			scaledWidth  >>= 1;
			scaledHeight >>= 1;
			if ( scaledWidth < 1 )
				scaledWidth = 1;
			if ( scaledHeight < 1 )
				scaledHeight = 1;

			++level;

			levelCost = GetCardMemoryAmount( selectedFormat, scaledWidth, scaledHeight );
			*cardMemory += levelCost;
			if ( textureMipSkips )
				--textureMipSkips;
			else
				*textureMemory += levelCost;

			if ( R_CVAR( r_colorMipLevels )->integer )
				R_BlendOverTexture( pixels, scaledWidth * scaledHeight,
				                    mipBlendColors[ level ], format );

			R_TexImage2D( uploadTarget, level, (unsigned int)selectedFormat,
			              scaledWidth, scaledHeight, format, pixels );
		}
	}

done:
	if ( mipmap )
	{
		glTexParameteri( textureTarget, 0x2801u , param );
		glTexParameteri( textureTarget, 0x2800u , gl_filter_max );
	}
	else
	{
		glTexParameteri( textureTarget, 0x2801u, 9729  );
		glTexParameteri( textureTarget, 0x2800u, 9729  );
	}

	GL_CheckErrors( "uploading an image" );   /* retail 0x004EC2B8: one push, the string */
	return 1;
}

/* ---- R_AllocImage  0x004EC310 ----  VERIFIED */
image_t * __cdecl R_AllocImage( const char *name, unsigned int target,
                                int width, int height, unsigned int flags,
                                int imageTrack )
{
	image_t *image;
	int      hash;

	if ( strlen( name ) >= 64 )
		ri.Error( 1, "\025R_AllocImage: \"%s\" is too long\n", name );
	if ( tr_numImages == 2048 )
		ri.Error( 1, "\025R_AllocImage: MAX_DRAWIMAGES hit\n" );

	( (image_t **)(void *)tr_imageSlots )[ tr_numImages ] = (image_t *)( (void *)( (*(int (__cdecl **)(int))&ri.Hunk_Alloc)( (int)(sizeof( *image )) ) ) );
	image = ( (image_t **)(void *)tr_imageSlots )[ tr_numImages ];
	image->texnum = (unsigned int)tr_numImages + 1024;
	++tr_numImages;

	strcpy( image->imgName, name );
	image->width         = (unsigned short)width;
	image->height        = (unsigned short)height;
	image->flags         = flags;
	image->target        = target;
	image->cardMemory    = 0;
	image->textureMemory = 0;



	hash = tr_image_generateHashValue( (char *)name );
	image->hashNext = ( (image_t **)(void *)hashtable )[ hash ];
	( (image_t **)(void *)hashtable )[ hash ] = image;
	image->imageTrack = imageTrack;
	return image;
}

/* ---- R_DeleteImage  0x004EC3F0 ----  VERIFIED */
void __cdecl R_DeleteImage( image_t *image )
{
	if ( (unsigned int)glState_currentTextures[ glState_currentTmu ] == image->texnum )
	{
		glBindTexture( image->target, 0 );
		glState_currentTextures[ glState_currentTmu ] = 0;
	}
	qglDeleteTextures( 1, &image->texnum );
}

/* ---- R_FreeImage  0x004EC430 ----  VERIFIED */
void __cdecl R_FreeImage( image_t *image )
{
	int hash = tr_image_generateHashValue( image->imgName );

	imageHashTable[ hash ] = image->hashNext;
	--tr_numImages;

	if ( (unsigned int)glState_currentTextures[ glState_currentTmu ] == image->texnum )
	{
		glBindTexture( image->target, 0 );
		glState_currentTextures[ glState_currentTmu ] = 0;
	}
	qglDeleteTextures( 1, &image->texnum );
}

#define IMAGE_MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define IMAGE_MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )

/* ---- R_CreateImageInternal  0x004EC490 ----  VERIFIED */
int __cdecl R_CreateImageInternal( image_t *image, const float *colorScale,
                                   unsigned char *pixels,
                                   unsigned int uploadTarget,
                                   unsigned int format )
{
	int isLightmap;
	int cardMemory;
	int textureMemory;
	int i;
	int c;

	isLightmap = 0;
	if ( !strncmp( image->imgName, "*lightmap", 9 ) )
		isLightmap = 1;

	if ( colorScale )
	{
		for ( i = 0; i < image->width * image->height; ++i )
		{
			/* a NaN product is left alone and reaches __ftol2 (integer
			 * indefinite -> low byte 0) */
			for ( c = 0; c < 4; ++c )
				pixels[ 4 * i + c ] = (unsigned char)(int)IMAGE_MIN( 255.0f,
					IMAGE_MAX( 0.0f, (rfloat)pixels[ 4 * i + c ] * colorScale[ c ] ) );
		}
	}

	GL_Bind( image );

	if ( !UploadImage( image->imgName, pixels, image->target, uploadTarget,
	                   format, (int)image->width, (int)image->height,
	                   (int)image->flags, isLightmap,
	                   &image->internalFormat,
	                   &image->uploadWidth, &image->uploadHeight,
	                   &cardMemory, &textureMemory ) )
	{
		return 0;
	}

	image->cardMemory    += cardMemory;
	image->textureMemory += textureMemory;
	tr_imageMemory        += cardMemory;

	glTexParameteri( image->target, 0x2802u ,
	                 ( image->flags & IMAGE_FLAG_CLAMP_S ) ? 33071 : 10497 );
	glTexParameteri( image->target, 0x2803u ,
	                 ( image->flags & IMAGE_FLAG_CLAMP_T ) ? 33071 : 10497 );
	return 1;
}

/* ---- R_CreateImage  0x004EC730 ----  VERIFIED */
int __cdecl R_CreateImage( char *name, int flags, int imageTrack,
                           unsigned __int8 *pixels, int width, int height,
                           int format, float *colorScale )
{
  image_t *image = R_AllocImage( name, 3553, width, height, flags, imageTrack );

  if ( flags & IMAGE_FLAG_DELAYED_UPLOAD )   /* 0x004EC74B `test bl, bl / jns` */
  {
    if ( format == 6408 || ( width >= 4 && height >= 4 ) )
    {
      image->internalFormat            = format;
      image->link.delayedShader        = 0;
      image->state.delay.group         = tr_delayedImageGroup;
      image->state.delay.groupTriCount = tr_delayedImageGroupTriCount;
      return (int)image;
    }
    image->flags &= ~IMAGE_FLAG_DELAYED_UPLOAD;
    --tr_delayedImageCount;
  }
  if ( !R_CreateImageInternal( image, colorScale, pixels, 0xDE1u, format ) )
  {
    R_FreeImage( image );
    image = 0;                          /* 0x004EC797 `xor esi, esi`      */
  }
  return (int)image;
}

/* ---- TransposeDDSBlockDXT1  0x004EC7C0 ----  [HIGH] */
/* Transposes one 4x4 DXT1 colour block: copies the two endpoint colours and
 * swaps the 2-bit index of pixel (r,c) with pixel (c,r). */
void __cdecl TransposeDDSBlockDXT1( const unsigned char *src, unsigned char *dst )
{
  *(int *)dst = *(const int *)src;
  dst[4] = (unsigned char)( ( ( ( ( src[7] << 2 ) | ( src[6] & 3 ) ) << 2 | ( src[5] & 3 ) ) << 2 ) | ( src[4] & 3 ) );
  dst[5] = (unsigned char)( ( ( ( ( src[7] & 0xFC ) << 2 ) | ( src[6] & 0x0C ) ) << 2 | ( ( src[4] >> 2 ) & 3 ) ) | ( src[5] & 0x0C ) );
  dst[6] = (unsigned char)( ( ( ( ( src[4] >> 2 ) & 0x0C ) | ( src[5] & 0x30 ) ) >> 2 ) | ( ( src[7] & 0xF0 ) << 2 ) | ( src[6] & 0x30 ) );
  dst[7] = (unsigned char)( ( ( ( ( ( src[4] >> 2 ) | ( src[5] & 0xCF ) ) >> 2 ) | ( src[6] & 0xC3 ) ) >> 2 ) | ( src[7] & 0xC0 ) );
}

/* ---- TransposeDDSBlockDXT3  0x004EC870 ----  [HIGH] */
/* DXT1 colour block at +8, then the 4-bit explicit alpha nibbles transposed. */
void __cdecl TransposeDDSBlockDXT3( unsigned char *dst, const unsigned char *src )
{
  TransposeDDSBlockDXT1( src + 8, dst + 8 );
  dst[0] = (unsigned char)( ( src[0] & 0x0F ) | ( src[2] << 4 ) );
  dst[1] = (unsigned char)( ( src[6] << 4 ) | ( src[4] & 0x0F ) );
  dst[2] = (unsigned char)( ( src[0] >> 4 ) | ( src[2] & 0xF0 ) );
  dst[3] = (unsigned char)( ( src[4] >> 4 ) | ( src[6] & 0xF0 ) );
  dst[4] = (unsigned char)( ( src[3] << 4 ) | ( src[1] & 0x0F ) );
  dst[5] = (unsigned char)( ( src[7] << 4 ) | ( src[5] & 0x0F ) );
  dst[6] = (unsigned char)( ( src[1] >> 4 ) | ( src[3] & 0xF0 ) );
  dst[7] = (unsigned char)( ( src[5] >> 4 ) | ( src[7] & 0xF0 ) );
}

/* ---- TransposeDDSBlockDXT5  0x004EC910 ----  [HIGH] */
void __cdecl TransposeDDSBlockDXT5( const unsigned char *in, unsigned char *out )
{
  /* the sixteen 3-bit alpha indices: rows 0-1 in in[2..4], rows 2-3 in in[5..7] */
  unsigned int bits[4];

  TransposeDDSBlockDXT1( in + 8, out + 8 );

  ( (unsigned char *)bits )[0] = in[2];
  ( (unsigned char *)bits )[1] = in[3];
  ( (unsigned char *)bits )[2] = in[4];
  ( (unsigned char *)bits )[4] = in[5];
  ( (unsigned char *)bits )[5] = in[6];
  ( (unsigned char *)bits )[6] = in[7];

  bits[2] = ( ( ( ( ( bits[1] & 0x38 ) << 6 | ( bits[0] & 0x38 ) ) << 3 | ( bits[1] & 0x38007 ) ) << 6 )
             | ( ( ( bits[1] & 0x7000 ) | ( ( bits[0] >> 6 ) & 0x1C0 ) ) >> 3 )
             | ( bits[0] & 0x38007 ) );
  bits[3] = ( ( ( ( ( bits[0] >> 6 ) & 0x7000 ) | ( bits[1] & 0x1C0000 ) ) >> 3 | ( bits[0] & 0xE001C0 ) ) >> 6 )
             | ( ( bits[1] & 0xE00 ) << 6 | ( bits[0] & 0xE00 ) ) << 3
             | ( bits[1] & 0xE001C0 );

  out[0] = in[0];
  out[1] = in[1];
  out[2] = ( (unsigned char *)bits )[8];
  out[3] = ( (unsigned char *)bits )[9];
  out[4] = ( (unsigned char *)bits )[10];
  out[5] = ( (unsigned char *)bits )[12];
  out[6] = ( (unsigned char *)bits )[13];
  out[7] = ( (unsigned char *)bits )[14];
}

/* ---- LoadDDS  0x004ECA30 ----  VERIFIED */
void __cdecl LoadDDS(const char *name, int *pixels, unsigned short *width, unsigned short *height, unsigned int *internalFormat, unsigned int *hasMipmaps, int loadData)
{
  unsigned int *header;
  int fileLen;
  int fileData;
  int allocIndex;
  int fourCC;
  int ddsWidth;
  unsigned int mipMapCount;
  unsigned int maxDim;
  int fileHandle; 
  int fileBuffer; 
  unsigned int headerBuf[32]; 

  if ( !loadData )
  {

    if ( ri.FS_FOpenFileByMode(name, &fileHandle, 0) <= 128 )
      return;
    ri.FS_Read(headerBuf, 128, fileHandle);
    ri.FS_FCloseFile(fileHandle);
    header = headerBuf;
  }
  else
  {
    fileLen = ( (*(int (__cdecl **)(const char *, void **))&ri.FS_ReadFile)( (name), (&fileBuffer) ) );
    fileData = fileBuffer;
    if ( fileLen >= 0 )
    {
      allocIndex = rendererImageAllocationCount;
      r_imageAllocations[rendererImageAllocationCount].kind = 0;
      *(int *)&r_imageAllocations[allocIndex].memory = fileData;
      rendererImageAllocationCount = allocIndex + 1;
    }
    if ( fileLen <= 128 )
      return;
    header = (unsigned int *)fileData;
  }
  if ( *header != 542327876 )
    return;
  fourCC = header[21];
  switch ( fourCC )
  {
    case 827611204:
      *internalFormat = 33777;
      break;
    case 861165636:
      *internalFormat = 33778;
      break;
    case 894720068:
      *internalFormat = 33779;
      break;
    default:
      return;
  }
  if ( header[1] == 124 )
  {
    ddsWidth = header[4];
    if ( ((ddsWidth - 1) & ddsWidth) != 0 || ((header[3] - 1) & header[3]) != 0 )
    {
      ri.Printf(2, "WARNING: image '%s' is %i x %i, which is not a power of 2 on both sides\n", name, ddsWidth, header[3]);
      *internalFormat = 0;
    }
    else
    {
      *hasMipmaps = 0;
      mipMapCount = header[7];
      if ( mipMapCount != 0 && mipMapCount != 1 )
      {
        maxDim = header[4];
        if ( maxDim <= header[3] )
          maxDim = header[3];
        if ( 1 << mipMapCount != 2 * maxDim )
        {
          ri.Printf(2, "WARNING: image '%s' does not have the correct number of mipmaps", name);
          *internalFormat = 0;
          return;
        }
        *hasMipmaps = 1;
      }
      if ( header[4] > 0x8000u || header[3] > 0x8000u )
      {
        ri.Printf(2, "WARNING: image '%s' is greater than %i on one or more axes", name, 0x8000);
        *internalFormat = 0;
      }
      else
      {
        *width = *((unsigned short *)header + 8);
        *height = *((unsigned short *)header + 6);
        if ( loadData )
          *pixels = (int)(header + 32);
      }
    }
  }
}

/* ---- LoadTGA  0x004ECC20 ----  [HIGH] */
#pragma pack( push, 1 )
typedef struct _TargaHeader
{
	unsigned char  id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	unsigned char  colormap_size;
	unsigned short x_origin, y_origin, width, height;
	unsigned char  pixel_size, attributes;
} TargaHeader;
#pragma pack( pop )

void __cdecl LoadTGA( const char *name, int *pic, unsigned __int16 *width, unsigned __int16 *height,
                      unsigned int *format, int readMode )
{
	unsigned short columns, rows;
	int            numPixels;
	unsigned char *pixbuf;
	int            row, column;
	unsigned char *buf_p;
	TargaHeader    targa_header;
	unsigned char *targa_rgba;
	int            pixelStride, rowAdvance;

	if ( !readMode )
	{
		int f;

		if ( ri.FS_FOpenFileByMode( name, &f, 0 ) < 18 )
			return;
		ri.FS_Read( &targa_header, 18, f );
		ri.FS_FCloseFile( f );
		buf_p = 0;
	}
	else
	{
		void *buffer;

		R_ReadFile( name, &buffer );
		if ( !buffer )
			return;
		buf_p = (unsigned char *)buffer;

		targa_header.id_length       = *buf_p++;
		targa_header.colormap_type   = *buf_p++;
		targa_header.image_type      = *buf_p++;
		targa_header.colormap_index  = *(unsigned short *)buf_p;
		buf_p += 2;
		targa_header.colormap_length = *(unsigned short *)buf_p;
		buf_p += 2;
		targa_header.colormap_size   = *buf_p++;
		targa_header.x_origin        = *(unsigned short *)buf_p;
		buf_p += 2;
		targa_header.y_origin        = *(unsigned short *)buf_p;
		buf_p += 2;
		targa_header.width           = *(unsigned short *)buf_p;
		buf_p += 2;
		targa_header.height          = *(unsigned short *)buf_p;
		buf_p += 2;
		targa_header.pixel_size      = *buf_p++;
		targa_header.attributes      = *buf_p++;
	}

	if ( targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3 )
		ri.Error( 1, "\x15" "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n" );
	if ( targa_header.colormap_type != 0 )
		ri.Error( 1, "\x15" "LoadTGA: colormaps not supported\n" );
	if ( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 )
		ri.Error( 1, "\x15" "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n" );

	columns   = targa_header.width;
	rows      = targa_header.height;
	numPixels = columns * rows;

	*width  = columns;
	*height = rows;
	*format = 0x1908;

	if ( !readMode )
		return;

	if ( targa_header.attributes & 0x20 )
	{
		pixelStride = 4;
		rowAdvance = 0;
	}
	else
	{
		pixelStride = 4;
		rowAdvance = -columns * 8;
	}

	targa_rgba = (unsigned char *)R_AllocTempMemory( numPixels * 4, "LoadTGA" );
	*pic       = (int)targa_rgba;

	pixbuf = targa_rgba;
	if ( rowAdvance < 0 )
		pixbuf = targa_rgba + ( rows - 1 ) * columns * 4;

	if ( readMode == 2 && *width < *height )
	{
		/* sheet load of a tall image: store it transposed (landscape) */
		*width  = rows;
		*height = columns;
		pixbuf  = targa_rgba;
		if ( targa_header.attributes & 0x20 )
		{
			pixelStride = rows * 4;
			rowAdvance  = 4 - columns * pixelStride;
		}
		else
		{
			pixelStride = rows * 4;
			rowAdvance  = -4 - columns * pixelStride;
			pixbuf      = targa_rgba + 4 * ( rows - 1 );
		}
	}

	if ( targa_header.id_length != 0 )
		buf_p += targa_header.id_length;  /* skip TARGA image comment */

	if ( targa_header.image_type == 2 || targa_header.image_type == 3 )
	{
		/* uncompressed RGB or gray scale image */
		for ( row = 0; row < rows; row++ )
		{
			for ( column = 0; column < columns; column++ )
			{
				unsigned char red, green, blue, alphabyte;

				switch ( targa_header.pixel_size )
				{
				case 8:
					blue  = *buf_p++;
					green = blue;
					red   = blue;
					pixbuf[0] = red;
					pixbuf[1] = green;
					pixbuf[2] = blue;
					pixbuf[3] = 255;
					break;
				case 24:
					blue  = *buf_p++;
					green = *buf_p++;
					red   = *buf_p++;
					pixbuf[0] = red;
					pixbuf[1] = green;
					pixbuf[2] = blue;
					pixbuf[3] = 255;
					break;
				case 32:
					blue      = *buf_p++;
					green     = *buf_p++;
					red       = *buf_p++;
					alphabyte = *buf_p++;
					pixbuf[0] = red;
					pixbuf[1] = green;
					pixbuf[2] = blue;
					pixbuf[3] = alphabyte;
					break;
				default:
					ri.Error( 1, "\x15" "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
					break;
				}
				pixbuf += pixelStride;
			}
			pixbuf += rowAdvance;
		}
	}
	else if ( targa_header.image_type == 10 )
	{
		/* run-length encoded RGB images */
		unsigned char red, green, blue, alphabyte, packetHeader, j;
		int           packetSize;

		red       = 0;
		green     = 0;
		blue      = 0;
		alphabyte = 0xff;

		for ( row = 0; row < rows; )
		{
			for ( column = 0; column < columns; )
			{
				packetHeader = *buf_p++;
				packetSize   = 1 + ( packetHeader & 0x7f );
				if ( packetHeader & 0x80 )
				{
					/* run-length packet */
					switch ( targa_header.pixel_size )
					{
					case 24:
						blue      = *buf_p++;
						green     = *buf_p++;
						red       = *buf_p++;
						alphabyte = 255;
						break;
					case 32:
						blue      = *buf_p++;
						green     = *buf_p++;
						red       = *buf_p++;
						alphabyte = *buf_p++;
						break;
					default:
						ri.Error( 1, "\x15" "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
						break;
					}

					for ( j = 0; j < packetSize; j++ )
					{
						pixbuf[0] = red;
						pixbuf[1] = green;
						pixbuf[2] = blue;
						pixbuf[3] = alphabyte;
						pixbuf += pixelStride;
						column++;
						if ( column == columns )
						{
							/* run spans across rows */
							column = 0;
							row++;
							if ( row == rows )
								return;
							pixbuf += rowAdvance;
						}
					}
				}
				else
				{
					/* non run-length packet */
					for ( j = 0; j < packetSize; j++ )
					{
						switch ( targa_header.pixel_size )
						{
						case 24:
							blue  = *buf_p++;
							green = *buf_p++;
							red   = *buf_p++;
							pixbuf[0] = red;
							pixbuf[1] = green;
							pixbuf[2] = blue;
							pixbuf[3] = 255;
							break;
						case 32:
							blue      = *buf_p++;
							green     = *buf_p++;
							red       = *buf_p++;
							alphabyte = *buf_p++;
							pixbuf[0] = red;
							pixbuf[1] = green;
							pixbuf[2] = blue;
							pixbuf[3] = alphabyte;
							break;
						default:
							ri.Error( 1, "\x15" "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
							break;
						}
						pixbuf += pixelStride;
						column++;
						if ( column == columns )
						{
							/* pixel packet run spans across rows */
							column = 0;
							row++;
							if ( row == rows )
								return;
							pixbuf += rowAdvance;
						}
					}
				}
			}
		}
	}
}

#define JPEG_DECOMPRESS_SIZE        424
#define JD_ERR                        0
#define JD_MEM                        4   /* 0x00516550 mov eax,[esi+4]      */
#define JD_OUTPUT_WIDTH              92   /* 0x004ED238                      */
#define JD_OUTPUT_HEIGHT             96   /* 0x004ED24A                      */
#define JD_OUTPUT_COMPONENTS        104   /* 0x004ED25B                      */
#define JD_OUTPUT_SCANLINE          120   /* 0x004ED306                      */
#define JPEG_ERROR_MGR_SIZE         132
/* jpeg_memory_mgr::self_destruct, slot 10 of 11 -- 0x004ED423 `call [eax+28h]`. */
#define JM_SELF_DESTRUCT             40

typedef char cod1_jpeg_abi_check[
    (   sizeof( struct jpeg_decompress_struct ) == JPEG_DECOMPRESS_SIZE
     && sizeof( struct jpeg_error_mgr )         == JPEG_ERROR_MGR_SIZE
     && offsetof( struct jpeg_decompress_struct, output_width )      == JD_OUTPUT_WIDTH
     && offsetof( struct jpeg_decompress_struct, output_height )     == JD_OUTPUT_HEIGHT
     && offsetof( struct jpeg_decompress_struct, output_components ) == JD_OUTPUT_COMPONENTS
     && offsetof( struct jpeg_decompress_struct, output_scanline )   == JD_OUTPUT_SCANLINE
     && offsetof( struct jpeg_memory_mgr,        self_destruct )     == JM_SELF_DESTRUCT
     && sizeof( boolean ) == 1 ) ? 1 : -1 ];
/* ---- LoadJPG  0x004ED160 ----  VERIFIED */
void __cdecl LoadJPG( const char *name, unsigned int *pixelsOut, unsigned short *widthOut, unsigned short *heightOut, unsigned int *formatOut, int readMode )
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;

  void         *fileBuf;
  JSAMPROW      rowPtr;
  unsigned int  x;
  byte        *out;
  byte        *src;
  int           rowBytes;
  int           j;

  if ( ( (*(int (__cdecl **)(const char *, void **))&ri.FS_ReadFile)( (name), (&fileBuf) ) ) >= 0 )
    R_RememberImageAllocation( fileBuf, 0 );
  if ( !fileBuf )
    return;

  cinfo.err = jpeg_std_error( &jerr );

  jpeg_create_decompress( &cinfo );
  CoD_jpeg_mem_src( &cinfo, (const unsigned char *)fileBuf );
  jpeg_read_header( &cinfo, 1 );
  jpeg_start_decompress( &cinfo );

  if ( cinfo.output_width > 0x8000 || cinfo.output_height > 0x8000 )
  {
    ri.Printf( 2, "WARNING: image '%s' is larger than %i on at least one side\n", name, 0x8000 );
    if ( cinfo.mem )
      ( *cinfo.mem->self_destruct )( (j_common_ptr)&cinfo );
    return;
  }

  if ( cinfo.output_components != 4 )
  {
    ri.Printf( 2, "WARNING: jpeg image '%s' is not RGB\n", name );
    if ( cinfo.mem )
      ( *cinfo.mem->self_destruct )( (j_common_ptr)&cinfo );
    return;
  }

  *widthOut  = (unsigned short)cinfo.output_width;
  *heightOut = (unsigned short)cinfo.output_height;
  *formatOut = 6408;

  if ( !readMode )
  {
    jpeg_destroy_decompress( &cinfo );
    return;
  }

  out        = (byte *)R_AllocTempMemory( 4 * cinfo.output_width * cinfo.output_height, "LoadJPG" );
  *pixelsOut = (unsigned int)out;
  rowBytes   = 4 * cinfo.output_width;

  if ( readMode == 2 && cinfo.output_width < cinfo.output_height )
  {
    /* sheet load of a tall image: read each row into a scratch line and store
       it as a column, so the tile comes out rotated to landscape */
    src = (byte *)_alloca( rowBytes );
    while ( cinfo.output_scanline < cinfo.output_height )
    {
      rowPtr = (JSAMPROW)src;
      jpeg_read_scanlines( &cinfo, &rowPtr, 1 );

      for ( x = 0; x < cinfo.output_width; x++ )
      {
        out[0] = src[4 * x + 0];
        out[1] = src[4 * x + 1];
        out[2] = src[4 * x + 2];
        out[3] = 0xFF;
        out   += rowBytes;
      }
      out += 4 - cinfo.output_width * rowBytes;
    }
    *widthOut  = (unsigned short)cinfo.output_height;
    *heightOut = (unsigned short)cinfo.output_width;
  }
  else
  {
    while ( cinfo.output_scanline < cinfo.output_height )
    {
      rowPtr = (JSAMPROW)out;
      jpeg_read_scanlines( &cinfo, &rowPtr, 1 );
      for ( j = 3; j < rowBytes; j += 4 )
        out[j] = 0xFF;
      out += rowBytes;
    }
  }

  jpeg_finish_decompress( &cinfo );
  if ( cinfo.mem )
    ( *cinfo.mem->self_destruct )( (j_common_ptr)&cinfo );
}

typedef struct {
  struct jpeg_destination_mgr pub;
  unsigned char *buffer;
  unsigned int   bufferSize;
} cod_destination_mgr;

typedef cod_destination_mgr *cod_dest_ptr;

/* ---- init_destination  0x004ED450 ----  [HIGH] */
unsigned int *__cdecl init_destination(int cinfo)
{
  cod_dest_ptr dest;
  int bufferSize;

  dest = (cod_dest_ptr)((j_compress_ptr)cinfo)->dest;
  bufferSize = dest->bufferSize;
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = bufferSize;
  return (unsigned int *)dest;
}

/* ---- empty_output_buffer  0x004ED470 ----  VERIFIED */
boolean __cdecl empty_output_buffer( j_compress_ptr cinfo )
{
  (void)cinfo;
  return TRUE;
}

/* ---- jpeg_start_compress  0x004ED480 ----  VERIFIED */
void __cdecl jpeg_start_compress( j_compress_ptr cinfo, boolean write_all_tables )
{
  char rawDataIn;

  if ( cinfo->global_state != 100 )                    /* CSTATE_START */
  {
    cinfo->err->msg_code = 17;                         /* ERREXIT1(JERR_BAD_STATE) */
    cinfo->err->msg_parm.i[0] = cinfo->global_state;
    (*cinfo->err->error_exit)((j_common_ptr)cinfo);
  }
  if ( write_all_tables )
    jpeg_suppress_tables(cinfo, FALSE);
  (*cinfo->err->reset_error_mgr)((j_common_ptr)cinfo);
  (*cinfo->dest->init_destination)(cinfo);
  jinit_compress_master((int)cinfo);
  (*(void (__cdecl **)(j_compress_ptr))cinfo->master)(cinfo);   /* master->prepare_for_pass */
  rawDataIn = cinfo->raw_data_in;
  cinfo->next_scanline = 0;
  cinfo->global_state = (rawDataIn != 0) + 101;        /* CSTATE_RAW_OK : CSTATE_SCANNING */
}

/* ---- jpeg_write_scanlines  0x004ED4F0 ----  VERIFIED */
unsigned int __cdecl jpeg_write_scanlines( j_compress_ptr cinfo, JSAMPARRAY scanlines,
                                           unsigned int num_lines )
{
  unsigned int lineCount = num_lines;
  {
  struct jpeg_progress_mgr *progress;
  int masterMgr;                  /* struct jpeg_comp_master * (jpegint.h, not included here) */
  int mainMgr;                    /* struct jpeg_c_main_controller * */
  int result;
  int rowsWritten; // [esp+4h] [ebp-4h] BYREF

  if ( cinfo->global_state != 101 )                    /* CSTATE_SCANNING */
  {
    cinfo->err->msg_code = 17;                         /* ERREXIT1(JERR_BAD_STATE) */
    cinfo->err->msg_parm.i[0] = cinfo->global_state;
    (*cinfo->err->error_exit)((j_common_ptr)cinfo);
  }
  if ( cinfo->next_scanline >= cinfo->image_height )
  {
    cinfo->err->msg_code = 117;                        /* WARNMS(JWRN_TOO_MUCH_DATA) */
    (*cinfo->err->emit_message)((j_common_ptr)cinfo, -1);
  }
  progress = cinfo->progress;
  if ( progress )
  {
    progress->pass_counter = cinfo->next_scanline;
    cinfo->progress->pass_limit = cinfo->image_height;
    (*cinfo->progress->progress_monitor)((j_common_ptr)cinfo);
  }
  masterMgr = (int)cinfo->master;
  if ( *(byte *)(masterMgr + 12) )                    /* master->call_pass_startup */
    (*(void (__cdecl **)(j_compress_ptr))(masterMgr + 4))(cinfo);   /* master->pass_startup */
  if ( lineCount > cinfo->image_height - cinfo->next_scanline )
    lineCount = cinfo->image_height - cinfo->next_scanline;
  mainMgr = (int)cinfo->main;
  rowsWritten = 0;
  (*(void (__cdecl **)(j_compress_ptr, JSAMPARRAY, int *, unsigned int))(mainMgr + 4))(cinfo, scanlines, &rowsWritten, lineCount);   /* main->process_data */
  result = rowsWritten;
  cinfo->next_scanline += rowsWritten;
  return (unsigned int)result;
  }
}

/* ---- term_destination  0x004ED5B0 ----  [HIGH] */
int __cdecl term_destination(int cinfo)
{
  cod_dest_ptr dest;

  dest = (cod_dest_ptr)((j_compress_ptr)cinfo)->dest;
  hackSize = dest->bufferSize - dest->pub.free_in_buffer;
  return (int)dest;
}

/* ---- jpegDest  0x004ED5D0 ----  VERIFIED */
void
jpegDest( j_compress_ptr cinfo, unsigned char *buffer, unsigned int bufferSize )
{
  cod_dest_ptr dest;

  if ( cinfo->dest == NULL ) {
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ( (j_common_ptr) cinfo, JPOOL_PERMANENT,
                                   SIZEOF( cod_destination_mgr ) );
  }

  dest = (cod_dest_ptr) cinfo->dest;
  dest->pub.init_destination    = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination    = term_destination;
  dest->buffer                  = buffer;
  dest->bufferSize               = bufferSize;
}

/* ---- SaveJPG  0x004ED610 ----  [HIGH] */
void __cdecl SaveJPG(int fileName, int quality, int width, unsigned int height, int flipped, int pixels)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  unsigned char *buffer;
  unsigned int   bufferSize;
  unsigned int   rowBytes;
  JSAMPROW       rowPointer[1];

  cinfo.err = jpeg_std_error( &jerr );
  jpeg_create_compress( &cinfo );

  bufferSize = 4 * height * width;
  buffer = (unsigned char *)( (void *)( (*(int (__cdecl **)(int))&ri.Hunk_AllocateTempMemory)( (int)(bufferSize) ) ) );
  jpegDest( &cinfo, buffer, bufferSize );

  cinfo.image_width      = width;
  cinfo.image_height     = height;
  cinfo.input_components = 4;
  cinfo.in_color_space   = JCS_RGB;

  jpeg_set_defaults( &cinfo );
  jpeg_set_quality( &cinfo, quality, 1 );
  jpeg_start_compress( &cinfo, 1 );

  rowBytes = 4 * width;
  while ( cinfo.next_scanline < cinfo.image_height )
  {
    if ( flipped )
      rowPointer[0] = (JSAMPROW)(pixels + rowBytes * ( cinfo.image_height - 1 - cinfo.next_scanline ));
    else
      rowPointer[0] = (JSAMPROW)(pixels + cinfo.next_scanline * rowBytes);
    jpeg_write_scanlines( &cinfo, rowPointer, 1u );
  }

  jpeg_finish_compress( &cinfo );
  ri.FS_WriteFile( fileName, buffer, hackSize );
  ( (*(void (__cdecl **)(void *))&ri.Hunk_FreeTempMemory)( (buffer) ) );

  if ( cinfo.mem )
    (*cinfo.mem->self_destruct)( (j_common_ptr)&cinfo );
}

/* ---- R_MangleTextureName  0x004ED8E0 ----  VERIFIED */
/* Encodes the target/flags (and optional colour scale or height scale) into a
 * synthetic "$tex_"/"$h2n_" image name.  The retail 1.0f comparisons are exact
 * bit tests (0x3F800000); colour scales are only ever set to exact 1.0f, so
 * the ordered float compares here are equivalent (the retail __UNORDERED__
 * terms only diverge for NaN inputs, which never occur). */
char *__cdecl R_MangleTextureName( int target, int flags, float *colorScale,
                                   const char *name, float heightScale )
{
	char *result;

	if ( flags & 0x0040 )
	{
		result = va( "$h2n_%s:%x/%x/%g", name, target, flags, heightScale );
	}
	else if ( !colorScale )
	{
		result = va( "$tex_%s:%x/%x", name, target, flags );
	}
	else if ( *(const unsigned int *)&colorScale[3] != 0x3F800000u )
	{
		result = va( "$tex_%s:%x/%x/%g/%g/%g/%g", name, target, flags,
		             colorScale[0], colorScale[1], colorScale[2], colorScale[3] );
	}
	else
	{
		if ( colorScale[0] == colorScale[1] && colorScale[0] == colorScale[2] )
		{
			if ( *(const unsigned int *)&colorScale[0] != 0x3F800000u )
				result = va( "$tex_%s:%x/%x/%g", name, target, flags, colorScale[0] );
			else
				result = va( "$tex_%s:%x/%x", name, target, flags );
		}
		else
		{
			result = va( "$tex_%s:%x/%x/%g/%g/%g", name, target, flags,
			             colorScale[0], colorScale[1], colorScale[2] );
		}
	}

	if ( strlen( result ) >= 0x40 )
	{
		ri.Printf( 2, "WARNING: mangled name for $texturename + %s is too long\n", name );
		return 0;
	}
	return result;
}

/* ---- R_UnmangleTextureName  0x004ED9F0 ----  VERIFIED */
/* Splits a "$tex_<prefix>:<params>" / "$h2n_<prefix>:<params>" instance name:
 * outName = baseName (extension stripped and <prefix> appended when present),
 * then the hex target/flags and the colour or height scales from <params>. */
void __cdecl R_UnmangleTextureName( char *outName, float *colorScale,
                                    const char *baseName, const char *mangledName,
                                    int *outTarget, int *outFlags,
                                    float *outHeightScale )
{
	const char *colon;
	const char *prefix;
	int         prefixLen;
	int         nameLen;
	int         count;

	prefix    = mangledName + 5;
	colon     = strchr( prefix, ':' );
	prefixLen = colon - prefix;
	if ( !prefixLen )
	{
		strcpy( outName, baseName );
	}
	else
	{
		Com_StripExtension( baseName, outName );
		nameLen = strlen( outName );
		Com_Memcpy( outName + nameLen, prefix, prefixLen );
		outName[ nameLen + prefixLen ] = 0;
	}

	colon++;
	if ( !strncmp( mangledName, "$h2n", 4 ) )
	{
		sscanf( colon, "%x/%x/%g", outTarget, outFlags, outHeightScale );
		colorScale[0] = 1.0f;
		colorScale[1] = 1.0f;
		colorScale[2] = 1.0f;
		colorScale[3] = 1.0f;
	}
	else
	{
		count = sscanf( colon, "%x/%x/%g/%g/%g/%g", outTarget, outFlags,
		                &colorScale[0], &colorScale[1], &colorScale[2], &colorScale[3] );
		if ( count < 3 )
			colorScale[0] = 1.0f;
		if ( count < 5 )
		{
			colorScale[1] = colorScale[0];
			colorScale[2] = colorScale[0];
		}
		if ( count < 6 )
			colorScale[3] = 1.0f;
		*outHeightScale = 1.0f;
	}
}

/* ---- R_ImageFistpBias  no-address ----
 * retail's inlined fld dword / fadd qword 2^-30 / fistp dword: round-to-nearest
 * under the default control word, the bias breaking x.5 ties upward.  An (int)
 * cast truncates (__ftol2 under /arch:IA32).  Same pattern as the DLLs'
 * Q_ftol (game_mp/bg_slidemove.c). */
static int R_ImageFistpBias( float f )
{
	double bias = 9.313225746154785e-10;
	int    i;

	__asm fld   f
	__asm fadd  bias
	__asm fistp i

	return i;
}

/* ---- R_HeightmapImage  0x004EDB30 ----  VERIFIED */
void __cdecl R_HeightmapImage( unsigned char *pixels, int width, int height, float heightScale )
{
	float          invScale;
	unsigned char *out;
	int            x, y;
	int            yUp, yDown;
	int            xLeft, xRight;
	int            left, right, up, down, center;
	float          dx, dy, invLen;

	invScale = 1.0f;
	if ( heightScale > 0.0f )
		invScale = 1.0f / heightScale;

	out = (unsigned char *)( (void *)( (*(int (__cdecl **)(int))&ri.Hunk_AllocateTempMemory)( (int)(4 * width * height) ) ) );
	for ( y = 0; y < height; y++ )
	{
		/* neighbours wrap around; the column wrap uses the height (square maps) */
		yUp   = ( !y ? height : y ) - 1;
		yDown = ( y + 1 == height ) ? 0 : y + 1;
		for ( x = 0; x < width; x++ )
		{
			xLeft  = ( !x ? height : x ) - 1;
			xRight = ( x + 1 == height ) ? 0 : x + 1;

			center = 4 * ( y * width + x );
			left   = 4 * ( y * width + xLeft );
			right  = 4 * ( y * width + xRight );
			up     = 4 * ( yUp * width + x );
			down   = 4 * ( yDown * width + x );

			dx = (float)( ( pixels[right + 0] + pixels[right + 1] + pixels[right + 2] ) - ( pixels[left + 0] + pixels[left + 1] + pixels[left + 2] ) ) * ( 1.0f / 1530.0f );
			dy = (float)( ( pixels[down + 0] + pixels[down + 1] + pixels[down + 2] ) - ( pixels[up + 0] + pixels[up + 1] + pixels[up + 2] ) ) * ( 1.0f / 1530.0f );
			invLen = 1.0 / sqrt( dx * dx + dy * dy + invScale * invScale );

			out[center + 0] = (unsigned char)R_ImageFistpBias( ( invLen * dx + 1.0f ) * 127.5f );
			out[center + 1] = (unsigned char)R_ImageFistpBias( ( (float)(invLen * dy) + 1.0f ) * 127.5f );
			out[center + 2] = (unsigned char)R_ImageFistpBias( ( invLen * invScale + 1.0f ) * 127.5f );
			out[center + 3] = pixels[center + 3];
		}
	}
	Com_Memcpy( pixels, out, 4 * width * height );
	( (*(void (__cdecl **)(void *))&ri.Hunk_FreeTempMemory)( (out) ) );
}

/* ---- R_FlipImageDiagonally  0x004EDDC0 ----  VERIFIED */
/* In-place transpose of a square (width == height) RGBA image; swaps (i,j)
 * with (j,i).  Called as R_FlipImageDiagonally(pixels, height, width). */
void R_FlipImageDiagonally( unsigned int *pixels, int height, int width )
{
	int i, j;

	if ( width == height )
	{
		for ( i = 1; i < width; ++i )
		{
			for ( j = 0; j < i; ++j )
			{
				unsigned int tmp = pixels[ j * width + i ];
				pixels[ j * width + i ] = pixels[ i * width + j ];
				pixels[ i * width + j ] = tmp;
			}
		}
	}
}

/* ---- R_FlipImageHorizontally  0x004EDE40 ----  VERIFIED */
/* Mirror each row left-to-right.  Called as R_FlipImageHorizontally(width,height,pixels). */
void R_FlipImageHorizontally( int width, int height, unsigned int *pixels )
{
	int i, j;

	for ( i = 0; i < height; ++i )
	{
		unsigned int *left  = pixels + i * width;
		unsigned int *right = left + width - 1;

		for ( j = 0; j < width / 2; ++j )
		{
			unsigned int tmp = left[j];
			left[j]   = right[-j];
			right[-j] = tmp;
		}
	}
}

/* ---- R_FlipImageVertically  0x004EDEA0 ----  VERIFIED */
/* Mirror rows top-to-bottom, column by column.  Called as
 * R_FlipImageVertically(height,pixels,width). */
void R_FlipImageVertically( int height, unsigned int *pixels, int width )
{
	int i, j;
	int lastRow = ( height - 1 ) * width;

	for ( i = 0; i < width; ++i )
	{
		unsigned int *top    = pixels + i;
		unsigned int *bottom = top + lastRow;

		for ( j = 0; j < height / 2; ++j )
		{
			unsigned int tmp = top[j * width];
			top[j * width]     = bottom[-j * width];
			bottom[-j * width] = tmp;
		}
	}
}

/* ---- R_LoadImage  0x004EDF20 ----  VERIFIED */
/* Tries <name>.dds, then <name>.tga, then <name>.jpg (each loader sets the
 * format out-param on success and stops the chain).  readFlag is passed
 * through to the loaders (0 = header scan only, 1 = full load, 2 = sheet load).
 * Returns the low byte of the resulting format (dead EAX). */
void __cdecl R_LoadImage( const char *name, int *data, unsigned __int16 *width,
                          unsigned __int16 *height, int *format,
                          unsigned int *hasMipmaps, int readFlag )
{
	char filename[64];
	int  len;
	int  result = 0;

	*data       = 0;
	*width      = 0;
	*height     = 0;
	*format     = 0;
	*hasMipmaps = 0;

	len = strlen( name );
	if ( len < 5 )
		return;

	if ( len >= 64 )
		ri.Error( 1, "\x15" "image name '%s' is longer than %i characters\n", name, 63 );
	if ( name[len - 4] == '.' )
		len -= 4;                                 
	if ( len >= 60 )
		ri.Error( 1, "\x15" "image name '%s' with its extension is longer than %i characters\n", name, 63 );

	memcpy( filename, name, len );
	filename[len]     = '.';
	
	*( filename + len + 4 ) = 0;

	
	filename[len + 1] = 'd';
	filename[len + 2] = 'd';
	filename[len + 3] = 's';
	LoadDDS( filename, data, width, height, format, hasMipmaps, readFlag );
	result = *format;

	if ( !*format )
	{
		
		*hasMipmaps = 1;
		filename[len + 1] = 't';
		filename[len + 2] = 'g';
		filename[len + 3] = 'a';
		LoadTGA( filename, data, width, height, format, readFlag );
		result = *format;

		if ( !*format )
		{
			
			filename[len + 1] = 'j';
			filename[len + 2] = 'p';
			filename[len + 3] = 'g';
			LoadJPG( filename, data, width, height, format, readFlag );
			result = *format;
		}
	}

	if ( *format && !*data )    
		++tr_delayedImageCount;

	return;
}

/* ---- R_LoadCubeMapImage  0x004EE0C0 ----  VERIFIED */
int __cdecl R_LoadCubeMapImage( const char *name, int flags, int imageTrack,
                                const float *colorScale )
{
  const char    *suffix[6];
  unsigned char *faces[6];
  char           filename[64];
  image_t       *image;
  unsigned __int16 loadedWidth, loadedHeight;
  int   loadedFormat, loadedSpare;
  int   width, height, format;
  int   face, s, t, i;
  int   len;

  suffix[0] = "rt";
  suffix[1] = "lf";
  suffix[2] = "bk";
  suffix[3] = "ft";
  suffix[4] = "up";
  suffix[5] = "dn";

  width  = 0;
  height = 0;
  format = 0;

  if ( name && Q_stricmpn( name, "$renormalize", 99999 ) == 0 )
  {
    
    int   axis[6][3];
    float sign[6][3];
    float dir[3];
    float normalized[3];
    unsigned char *out;

    axis[0][0] = 0; axis[0][1] = 2; axis[0][2] = 1;
    axis[1][0] = 0; axis[1][1] = 2; axis[1][2] = 1;
    axis[2][0] = 1; axis[2][1] = 0; axis[2][2] = 2;
    axis[3][0] = 1; axis[3][1] = 0; axis[3][2] = 2;
    axis[4][0] = 2; axis[4][1] = 0; axis[4][2] = 1;
    axis[5][0] = 2; axis[5][1] = 0; axis[5][2] = 1;

    sign[0][0] =  1.0f; sign[0][1] = -1.0f; sign[0][2] = -1.0f;
    sign[1][0] = -1.0f; sign[1][1] =  1.0f; sign[1][2] = -1.0f;
    sign[2][0] =  1.0f; sign[2][1] =  1.0f; sign[2][2] =  1.0f;
    sign[3][0] = -1.0f; sign[3][1] =  1.0f; sign[3][2] = -1.0f;
    sign[4][0] =  1.0f; sign[4][1] =  1.0f; sign[4][2] = -1.0f;
    sign[5][0] = -1.0f; sign[5][1] = -1.0f; sign[5][2] = -1.0f;

    flags &= 0xFFFFFFF8;                          
    width  = 64;
    height = 64;

    for ( face = 0; face < 6; face++ )
    {
      out = (unsigned char *)( (void *)( (*(int (__cdecl **)(int))&ri.Hunk_AllocateTempMemory)( (int)(0x4000) ) ) );
      if ( out )
        R_RememberImageAllocation( out, 1 );
      faces[face] = out;

      dir[ axis[face][0] ] = sign[face][0];

      for ( t = 1; t < 129; t += 2 )
      {
        dir[ axis[face][2] ] = ( (float)t * 0.015625f - 1.0f ) * sign[face][2];

        for ( s = 1; s < 129; s += 2 )
        {
          dir[ axis[face][1] ] = ( (float)s * 0.015625f - 1.0f ) * sign[face][1];

          VectorNormalize2( dir, normalized );

          

          out[0] = (unsigned char)R_ImageFistpBias( (float)( ( normalized[0] + 1.0f ) * 127.5f ) );
          out[1] = (unsigned char)R_ImageFistpBias( (float)( ( normalized[1] + 1.0f ) * 127.5f ) );
          out[2] = (unsigned char)R_ImageFistpBias( (float)( ( normalized[2] + 1.0f ) * 127.5f ) );
          out[3] = 0xFF;
          out += 4;
        }
      }
    }
  }
  else
  {
    
    len = strlen( name );
    if ( len + 7 >= 64 )
    {
      ri.Printf( 2, "Cube map name will exceed max qpath\n" );
      return 0;
    }

    strcpy( filename, name );
    strcat( filename, "_**.tga" );                

    for ( face = 0; face < 6; face++ )
    {
      strcpy( filename + len + 1, suffix[face] );

      R_LoadImage( filename, (int *)&faces[face], &loadedWidth, &loadedHeight,
                   &loadedFormat, (unsigned int *)&loadedSpare, 1 );

      if ( !faces[face] )
        return 0;

      if ( face == 0 )
      {
        width  = loadedWidth;
        height = loadedHeight;
        format = loadedFormat;
        if ( loadedWidth != loadedHeight )
        {
          ri.Printf( 2, "Cube map face images should be square ('%s')\n", filename );
          return 0;
        }
      }
      else
      {
        if ( width != loadedWidth || height != loadedHeight )
        {
          ri.Printf( 2, "Cube map face images are not all the same size ('%s')\n", filename );
          return 0;
        }
        if ( format != loadedFormat )
        {
          ri.Printf( 2, "Cube map face images are not all the same format ('%s')\n", filename );
          return 0;
        }
      }
    }

    
    R_FlipImageDiagonally( (unsigned int *)faces[0], height, width );
    R_FlipImageDiagonally( (unsigned int *)faces[1], height, width );
    R_FlipImageHorizontally( width, height, (unsigned int *)faces[1] );
    R_FlipImageVertically( height, (unsigned int *)faces[1], width );
    R_FlipImageVertically( height, (unsigned int *)faces[2], width );
    R_FlipImageHorizontally( width, height, (unsigned int *)faces[3] );
    R_FlipImageDiagonally( (unsigned int *)faces[4], height, width );
    R_FlipImageDiagonally( (unsigned int *)faces[5], height, width );
  }

  image = R_AllocImage( name, 0x8513u, width, height, flags | 0x30, imageTrack );

  for ( i = 0; i < 6; i++ )
  {
    if ( !R_CreateImageInternal( image, colorScale, faces[i],
                                 0x8515u + i, 0x1908u ) )
    {
      R_FreeImage( image );                       
      return 0;
    }
  }

  return (int)image;
}

/* ---- R_FindExistingImage  0x004EE770 ----  VERIFIED */
int __cdecl R_FindExistingImage( char *name, int target, int flags, int trackType )
{
	image_t *image = imageHashTable[ tr_image_generateHashValue( name ) ];
	int      diff;

	while ( image )
	{
		if ( name && image && !Q_stricmpn( name, image->imgName, 99999 ) )
			break;
		image = image->hashNext;
	}
	if ( !image )
		return 0;

	diff = (int)image->flags ^ flags;
	if ( diff
	  && strcmp( name, "*white" )
	  && strcmp( name, "*gray" ) )
	{
		if ( diff & IMAGE_FLAG_MIPMAP )
			ri.Printf( 1, "WARNING: reused image %s with mixed mipmap allowance\n", name );
		if ( diff & IMAGE_FLAG_ALLOW_PICMIP )
			ri.Printf( 1, "WARNING: reused image %s with mixed picmip allowance\n", name );
		if ( diff & IMAGE_FLAG_USE_PICMIP2 )
			ri.Printf( 1, "WARNING: reused image %s with mixed picmip type (model / world)\n", name );
		if ( diff & ( IMAGE_FLAG_CLAMP_S | IMAGE_FLAG_CLAMP_T ) )
			ri.Printf( 0, "WARNING: reused image %s with mixed clamp modes\n", name );
	}

	if ( image->target != (unsigned int)target )
		ri.Printf( 2, "ERROR: image '%s' cannot be used in a cube map and in a normal texture", name );

	if ( image->imageTrack != trackType )
		ri.Printf( 0, "WARNING: image '%s' changed type from %s to %s.\n", name,
		           r_imageTrackNames[ image->imageTrack ],
		           r_imageTrackNames[ trackType ] );
	image->imageTrack = trackType;

	return (int)image;
}

/* ---- R_LoadSingleDelayedImage  0x004EE8A0 ----  VERIFIED */
/* Reloads a delayed image now, verifies it still matches the record's cached
 * dimensions/format/mipmap state, then uploads it.  If the upload fails the
 * record is replaced with a copy of the default image (keeping its name and
 * hash link).  The error-message argument order is kept exactly as retail. */
void __cdecl R_LoadSingleDelayedImage( int imageAddr )
{
	image_t        *image = (image_t *)imageAddr;
	int             loadedData;
	unsigned short  loadedWidth;
	unsigned short  loadedHeight;
	int             loadedFormat;
	int             hasMipmaps;
	char            savedName[64];
	image_t        *savedHashNext;

	image->flags &= ~0x0080;
	image->link.textureSheet   = 0;
	image->state.sheet.x       = 0;
	image->state.sheet.y       = 0;
	image->state.sheet.rotated = 0;
	--tr_delayedImageCount;
	loadedData = 0;
	loadedWidth  = 0;
	loadedHeight = 0;
	loadedFormat = 0;

	R_LoadImage( image->imgName, &loadedData, &loadedWidth, &loadedHeight,
	             &loadedFormat, (unsigned int *)&hasMipmaps, 1 );

	if ( loadedWidth  != image->width
	  || loadedHeight != image->height
	  || loadedFormat != (int)image->internalFormat
	  || ( loadedFormat != 6408 && !hasMipmaps && ( image->flags & 0x0001 ) ) )
	{
		R_FreeImageAllocations();
		ri.Error(
			1,
			"\x15" "delayed-load image '%s' changed from %ix%i type 0x%04x %s mipmaps to %ix%i type 0x%04x %s mipmaps\n",
			image->imgName,
			loadedWidth,
			loadedHeight,
			loadedFormat,
			( image->flags & 0x0001 ) ? "with" : "without",
			image->width,
			image->height,
			image->internalFormat,
			hasMipmaps ? "with" : "without" );
	}

	if ( !R_CreateImageInternal( image, 0, (unsigned char *)loadedData, 0xDE1u, loadedFormat ) )
	{
		strcpy( savedName, image->imgName );
		savedHashNext = image->hashNext;
		R_DeleteImage( image );
		memcpy( image, (const void *)tr_defaultImage, 0x78u );
		strcpy( image->imgName, savedName );
		image->hashNext = savedHashNext;
	}
	R_FreeImageAllocations();
}

/* ---- R_UpdateDelayLoadImage  0x004EEA60 ----  VERIFIED */
/* For a still-delayed image: force-load it now, or record the requesting
 * shader as its merge target (loading immediately if a second, non-mergable
 * shader wants it), then fold its delay group into the current one. */
void __cdecl R_UpdateDelayLoadImage( int imageAddr, int shader, int forceLoad )
{
	image_t         *image  = (image_t *)imageAddr;
	struct shader_s *existingShader;
	int              group;
	int              i;

	if ( !( image->flags & 0x80 ) )             /* not delay-loaded */
		return;

	if ( forceLoad )
	{
		R_LoadSingleDelayedImage( imageAddr );
		return;
	}

	existingShader = image->link.delayedShader;
	if ( shader == (int)existingShader )
		return;

	if ( !existingShader )
		image->link.delayedShader = (struct shader_s *)shader;
	else if ( CompareMergableShaders( shader, (int)existingShader, imageAddr, imageAddr ) )
	{
		R_LoadSingleDelayedImage( imageAddr );
		return;
	}

	group = image->state.delay.group;
	if ( group != (*(int *)(((char *)(void *)&tr_registered) + 0x0128)) )
	{
		for ( i = 0; i < tr_numImages; ++i )
		{
			image_t *other = ( (image_t **)(void *)tr_imageSlots )[i];

			if ( other
			  && ( other->flags & 0x80 )
			  && other->state.delay.group == group )
				other->state.delay.group = (*(int *)(((char *)(void *)&tr_registered) + 0x0128));
		}
	}
}

/* ---- R_FindImageFile  0x004EEAE0 ----  VERIFIED */
int __cdecl R_FindImageFile( char *name, int target, int flags, int trackType,
                             const float *colorScale, float heightScale )
{
	int             image;
	unsigned short  width;
	unsigned short  height;
	unsigned char  *pic;
	unsigned int    format;
	int             hasMipmaps;

	if ( !name )
		return 0;

	if ( !r_optimize->integer || !r_optimizeTextures->integer )
		flags &= ~0x0080;

	if ( trackType == 10 )
	{
		name = R_MangleTextureName( target, flags, (float *)colorScale, name, heightScale );
		if ( !name )
			return 0;
	}

	image = R_FindExistingImage( name, target, flags, trackType );
	if ( image )
	{
		/* flag 0x80 (delay-loaded) is bit 7 of the low byte: retail tests its sign */
		if ( (signed char)( (image_t *)image )->flags >> 7 )
		{
			R_UpdateDelayLoadImage( image, 0, ( flags & 0x0080 ) == 0 );
			if ( (signed char)( (image_t *)image )->flags >> 7 )
				( (image_t *)image )->state.delay.groupTriCount += (*(int *)(((char *)(void *)&tr_registered) + 0x012C));
		}
		else if ( ( (image_t *)image )->link.delayedShader )
		{
			ri.Error( 1, "\x15" "tried to load image '%s', which has been merged with other textures, after the map finished loading\n", name );
			return 0;
		}
		return image;
	}

	if ( trackType == 10 )
	{
		image = (int)R_AllocImage( name, target, 0, 0, flags, 10 );
	}
	else if ( target == 3553 )
	{
		if ( ( flags & 0x0080 )
		  && ( ( flags & 0x0040 )
		    || ( colorScale
		      && ( colorScale[0] != 1.0f || colorScale[1] != 1.0f
		        || colorScale[2] != 1.0f || colorScale[3] != 1.0f ) ) ) )
		{
			flags &= ~0x0080;
		}

		R_LoadImage( name, (int *)&pic, &width, &height, (int *)&format, (unsigned int *)&hasMipmaps,
		             ( flags & 0x0080 ) == 0 );

		if ( !hasMipmaps && ( flags & 0x0001 ) )
		{
			ri.Printf( 2, "image '%s' requested mipmaps, but they aren't available\n", name );
			image = 0;
		}
		else if ( !format )
		{
			image = 0;
		}
		else if ( flags & 0x0040 )
		{
			switch ( format )
			{
			case 0x83F0:
			case 0x83F1:
			case 0x83F2:
			case 0x83F3:
				ri.Printf( 2, "image '%s': heightToNormal not valid for DDS textures\n", name );
				image = 0;
				break;
			case 6408:
				if ( !( flags & 0x0080 ) )
					R_HeightmapImage( pic, width, height, heightScale );
				image = R_CreateImage( name, flags, trackType, pic, width, height, 6408, (float *)colorScale );
				break;
			default:
				image = 0;
				break;
			}
		}
		else
		{
			image = R_CreateImage( name, flags, trackType, pic, width, height, format, (float *)colorScale );
		}
	}
	else if ( target == 34067 )
	{
		image = R_LoadCubeMapImage( name, flags, trackType, colorScale );
	}
	else
	{
		ri.Printf( 2, "Unknown texture target 0x%04x\n", target );
		image = 0;
	}
	R_FreeImageAllocations();
	return image;
}

/* ---- R_FindImageInstance  0x004EED80 ----  VERIFIED */
/* Decodes a mangled "$tex_"/"$h2n_" instance name back into a base name plus
 * target/flags/colour-scale/height-scale, decides whether the upload may be
 * deferred, then resolves it through R_FindImageFile.  (The retail 1.0f tests
 * are exact bit compares; only NaN colour scales would diverge.) */
int __cdecl R_FindImageInstance( int disableDelay, char *baseName, int trackType, char *mangledName, int shader )
{
	float  colorScale[4];
	float  heightScale;
	int    target;
	int    flags;
	char   name[64];
	float *colorScalePtr;
	int    result;

	R_UnmangleTextureName( name, colorScale, baseName, mangledName,
	                       &target, &flags, &heightScale );

	if ( colorScale[0] == 1.0f && colorScale[1] == 1.0f
	  && colorScale[2] == 1.0f && colorScale[3] == 1.0f )
		colorScalePtr = 0;
	else
		colorScalePtr = colorScale;

	if ( disableDelay || !r_optimizeTextures->integer || !r_optimize->integer
	  || ( flags & IMAGE_FLAG_NO_TEXTURE_SHEET ) )
		flags &= ~IMAGE_FLAG_DELAYED_UPLOAD;
	else
		flags |= IMAGE_FLAG_DELAYED_UPLOAD;

	result = R_FindImageFile( name, target, flags, trackType, colorScalePtr, heightScale );
	if ( result )
		R_UpdateDelayLoadImage( result, shader, disableDelay );
	return result;
}

/* ---- R_BeginDelayedImageGroup  0x004EEE80 ----  [HIGH] */
void __cdecl R_BeginDelayedImageGroup(const char *modelName)
{
  if ( !tr_modelsFinishedLoading )
  {
    tr_delayedImageGroup = ++tr_delayedImageGroupSequence;
  }
  else if ( r_errorOnConflicts->integer && !tr_ignorePrecacheErrorCount )
  {
    ri.Error(1, "\x15" "model '%s' not precached\n", modelName);
  }
}

/* ---- R_SetImageGroupTriCount  0x004EEED0 ----  [HIGH] */
void __cdecl R_SetImageGroupTriCount(int triCount)
{
  if ( tr_delayedImageGroup )
    tr_delayedImageGroupTriCount = triCount;
}

/* ---- R_SetImageGroupTileMode  0x004EEEF0 ----  [HIGH] */
void __cdecl R_SetImageGroupTileMode(int tileMode)
{
  if ( tr_delayedImageGroup )
    tr_delayedImageGroupTileMode = tileMode;
}

/* ---- R_EndDelayedImageGroup  0x004EEF10 ----  [HIGH] */
int R_EndDelayedImageGroup()
{
  int result;

  result = 0;
  tr_delayedImageGroup = 0;
  tr_delayedImageGroupTriCount = 0;
  return result;
}

/* ---- CopyImageTileLevel_DXT1  0x004EEF20 ----  VERIFIED */
void __cdecl CopyImageTileLevel_DXT1( unsigned short imageWidth, unsigned char *dst, const unsigned char *src,
                                      unsigned short x, unsigned short y, unsigned short width, unsigned short height )
{
  unsigned char       *out;
  const unsigned char *in;
  const unsigned char *cursor;
  unsigned short       i;
  unsigned short       j;

  cursor = src;
  out = dst + y * imageWidth / 2 + 2 * x;
  if ( width >= height )
  {
    for ( j = 0; j < height; j += 4 )
    {
      Com_Memcpy( out, cursor, 2 * width );
      cursor += 2 * width;
      out += 2 * imageWidth;
    }
  }
  else
  {
    for ( i = 0; i < width; i += 4 )
    {
      in = src + 2 * i;
      for ( j = 0; j < height; j += 4 )
      {
        TransposeDDSBlockDXT1( in, out );
        in += 2 * width;
        out += 8;
      }
      out += 2 * ( imageWidth - height );
    }
  }
}

/* ---- CopyImageTile_DXT1  0x004EF020 ----  VERIFIED */
void __cdecl CopyImageTile_DXT1( int sheetPtr, unsigned char *dst, const unsigned char *src, int bufferEnd,
                                 unsigned short x, unsigned short y, unsigned short width, unsigned short height )
{
  unsigned short levelWidth;
  unsigned short levelHeight;

  (void)bufferEnd;
  CopyImageTileLevel_DXT1( ((image_t *)sheetPtr)->width, dst, src, x, y, width, height );
  if ( ((image_t *)sheetPtr)->flags & IMAGE_FLAG_MIPMAP )
  {
    levelWidth  = ((image_t *)sheetPtr)->width;
    levelHeight = ((image_t *)sheetPtr)->height;
    do
    {
      dst += GetCardMemoryAmount( (int)((image_t *)sheetPtr)->internalFormat, levelWidth, levelHeight );
      levelWidth >>= 1;
      levelHeight >>= 1;
      src += GetCardMemoryAmount( (int)((image_t *)sheetPtr)->internalFormat, width, height );
      x >>= 1;
      width >>= 1;
      height >>= 1;
      y >>= 1;
      if ( x & 3 )
        break;
      if ( y & 3 )
        break;
      CopyImageTileLevel_DXT1( levelWidth, dst, src, x, y, width, height );
    }
    while ( width > 1 || height > 1 );
  }
}

/* ---- CopyImageTileLevel_DXT3  0x004EF130 ----  VERIFIED */
void __cdecl CopyImageTileLevel_DXT3( unsigned short imageWidth, unsigned char *dst, const unsigned char *src,
                                      unsigned short x, unsigned short y, unsigned short width, unsigned short height )
{
  unsigned char       *out;
  const unsigned char *in;
  const unsigned char *cursor;
  unsigned short       i;
  unsigned short       j;

  cursor = src;
  out = dst + y * imageWidth + 4 * x;
  if ( width >= height )
  {
    for ( j = 0; j < height; j += 4 )
    {
      Com_Memcpy( out, cursor, 4 * width );
      cursor += 4 * width;
      out += 4 * imageWidth;
    }
  }
  else
  {
    for ( i = 0; i < width; i += 4 )
    {
      in = src + 4 * i;
      for ( j = 0; j < height; j += 4 )
      {
        TransposeDDSBlockDXT3( out, in );
        in += 4 * width;
        out += 16;
      }
      out += 4 * ( imageWidth - height );
    }
  }
}

/* ---- CopyImageTile_DXT3  0x004EF220 ----  VERIFIED */
void __cdecl CopyImageTile_DXT3( int sheetPtr, unsigned char *dst, const unsigned char *src, int bufferEnd,
                                 unsigned short x, unsigned short y, unsigned short width, unsigned short height )
{
  unsigned short levelWidth;
  unsigned short levelHeight;

  (void)bufferEnd;
  CopyImageTileLevel_DXT3( ((image_t *)sheetPtr)->width, dst, src, x, y, width, height );
  if ( ((image_t *)sheetPtr)->flags & IMAGE_FLAG_MIPMAP )
  {
    levelWidth  = ((image_t *)sheetPtr)->width;
    levelHeight = ((image_t *)sheetPtr)->height;
    do
    {
      dst += GetCardMemoryAmount( (int)((image_t *)sheetPtr)->internalFormat, levelWidth, levelHeight );
      levelWidth >>= 1;
      levelHeight >>= 1;
      src += GetCardMemoryAmount( (int)((image_t *)sheetPtr)->internalFormat, width, height );
      x >>= 1;
      width >>= 1;
      height >>= 1;
      y >>= 1;
      if ( x & 3 )
        break;
      if ( y & 3 )
        break;
      CopyImageTileLevel_DXT3( levelWidth, dst, src, x, y, width, height );
    }
    while ( width > 1 || height > 1 );
  }
}

/* ---- CopyImageTileLevel_DXT5  0x004EF330 ----  VERIFIED */
void __cdecl CopyImageTileLevel_DXT5( unsigned short imageWidth, unsigned char *dst, const unsigned char *src,
                                      unsigned short x, unsigned short y, unsigned short width, unsigned short height )
{
  unsigned char       *out;
  const unsigned char *in;
  const unsigned char *cursor;
  unsigned short       i;
  unsigned short       j;

  cursor = src;
  out = dst + y * imageWidth + 4 * x;
  if ( width >= height )
  {
    for ( j = 0; j < height; j += 4 )
    {
      Com_Memcpy( out, cursor, 4 * width );
      cursor += 4 * width;
      out += 4 * imageWidth;
    }
  }
  else
  {
    for ( i = 0; i < width; i += 4 )
    {
      in = src + 4 * i;
      for ( j = 0; j < height; j += 4 )
      {
        TransposeDDSBlockDXT5( in, out );
        in += 4 * width;
        out += 16;
      }
      out += 4 * ( imageWidth - height );
    }
  }
}

/* ---- CopyImageTile_DXT5  0x004EF420 ----  VERIFIED */
void __cdecl CopyImageTile_DXT5( int sheetPtr, unsigned char *dst, const unsigned char *src, int bufferEnd,
                                 unsigned short x, unsigned short y, unsigned short width, unsigned short height )
{
  unsigned short levelWidth;
  unsigned short levelHeight;

  (void)bufferEnd;
  CopyImageTileLevel_DXT5( ((image_t *)sheetPtr)->width, dst, src, x, y, width, height );
  if ( ((image_t *)sheetPtr)->flags & IMAGE_FLAG_MIPMAP )
  {
    levelWidth  = ((image_t *)sheetPtr)->width;
    levelHeight = ((image_t *)sheetPtr)->height;
    do
    {
      dst += GetCardMemoryAmount( (int)((image_t *)sheetPtr)->internalFormat, levelWidth, levelHeight );
      levelWidth >>= 1;
      levelHeight >>= 1;
      src += GetCardMemoryAmount( (int)((image_t *)sheetPtr)->internalFormat, width, height );
      x >>= 1;
      width >>= 1;
      height >>= 1;
      y >>= 1;
      if ( x & 3 )
        break;
      if ( y & 3 )
        break;
      CopyImageTileLevel_DXT5( levelWidth, dst, src, x, y, width, height );
    }
    while ( width > 1 || height > 1 );
  }
}

/* ---- CopyImageTile_RGBA  0x004EF530 ----  VERIFIED */
void __cdecl CopyImageTile_RGBA(
        unsigned short height,
        const unsigned char *src,
        unsigned short tileY,
        image_t *sheet,
        unsigned char *dst,
        unsigned short tileX,
        unsigned short width)
{
  unsigned short y;

  dst += 4 * (tileX + sheet->width * tileY);
  for ( y = 0; y < height; y++ )
  {
    Com_Memcpy(dst, src, 4 * width);
    src += 4 * width;
    dst += 4 * sheet->width;
  }
}

/* ---- LoadImageTile  0x004EF590 ----  VERIFIED */
/* Reloads a delayed image, verifies it still matches its scan, picmip-reduces
 * it, then blits it into its slot (slotX,slotY) on the sheet, dispatched by the
 * sheet's compressed/uncompressed format. */
void __cdecl LoadImageTile( int imagePtr, int sheetPtr, int buffer, int bufferEnd,
                           unsigned __int16 slotX, unsigned __int16 slotY )
{
	image_t        *image = (image_t *)imagePtr;
	image_t        *sheet = (image_t *)sheetPtr;
	unsigned char  *loadedPixels;
	unsigned short  loadedWidth;
	unsigned short  loadedHeight;
	unsigned int    loadedFormat;
	int             hasMipmaps;
	int             picmip;

	image->flags &= ~0x0080;
	image->texnum              = sheet->texnum;
	image->link.textureSheet   = sheet;
	image->state.sheet.x       = slotX;
	image->state.sheet.y       = slotY;
	image->state.sheet.rotated = (unsigned char)( image->width < image->height );
	--tr_delayedImageCount;

	R_LoadImage( image->imgName, (int *)&loadedPixels, &loadedWidth, &loadedHeight,
	             (int *)&loadedFormat, (unsigned int *)&hasMipmaps, 2 );
	if ( !loadedPixels )
		{ ri.Error( 1, "\x15" "image '%s' got deleted between initial scan and actual load\n", image->imgName ); return; }

	if ( loadedWidth + loadedHeight != image->width + image->height
	  || ( loadedWidth != image->width && loadedWidth != image->height )
	  || loadedFormat != image->internalFormat
	  || ( loadedFormat != 6408 && !hasMipmaps && ( image->flags & 0x0001 ) ) )
	{
		{ ri.Error( 1, "\x15" "image '%s' changed between initial scan and actual load\n", image->imgName ); return; }
	}

	picmip = R_PicmipForImageFlags( image->flags );
	if ( picmip )
	{
		do
		{
			if ( loadedWidth <= 4u || loadedHeight <= 4u )
				break;
			loadedPixels = R_MipMap( loadedFormat, loadedPixels, image->imgName,
			                         loadedWidth, loadedHeight, 2 );
			if ( !loadedPixels )
				ri.Error( 1, "\x15" "tried to put image '%s' on a texture sheet, but it has an invalid custom mipmap\n", image->imgName );
			loadedWidth  >>= 1;
			loadedHeight >>= 1;
			--picmip;
		}
		while ( picmip );
	}

	switch ( (int)sheet->internalFormat )   /* signed compares in retail: an int-typed format there */
	{
		case 33779:   
			CopyImageTile_DXT5( sheetPtr, (unsigned char *)buffer, (const unsigned char *)loadedPixels, bufferEnd, slotX, slotY, loadedWidth, loadedHeight );
			break;
		case 33778:   
			CopyImageTile_DXT3( sheetPtr, (unsigned char *)buffer, (const unsigned char *)loadedPixels, bufferEnd, slotX, slotY, loadedWidth, loadedHeight );
			break;
		case 6408:    
			CopyImageTile_RGBA( loadedHeight, loadedPixels, slotY, sheet, (unsigned char *)buffer, slotX, loadedWidth );
			break;
		case 33776:   
		case 33777:   
			CopyImageTile_DXT1( sheetPtr, (unsigned char *)buffer, (const unsigned char *)loadedPixels, bufferEnd, slotX, slotY, loadedWidth, loadedHeight );
			break;
	}

	R_FreeImageAllocations();
}

/* ---- UploadImageGroup_r  0x004EF800 ----  VERIFIED */
/* Walks the packing tree, accumulating the (x,y) offset of each leaf, then
 * blits each leaf image into the sheet via LoadImageTile.  Siblings sharing
 * the group width stack vertically (advance y); otherwise they advance x. */
void __cdecl UploadImageGroup_r( imageGroupNode_t *node, int sheet, int buffer, int bufferEnd,
                                 unsigned short x, unsigned short y )
{
	if ( node->next )
	{
		if ( node->width == node->u.child->width )
		{
			UploadImageGroup_r( node->u.child, sheet, buffer, bufferEnd, x, y );
			UploadImageGroup_r( node->next, sheet, buffer, bufferEnd, x, y + node->u.child->height );
		}
		else
		{
			UploadImageGroup_r( node->u.child, sheet, buffer, bufferEnd, x, y );
			UploadImageGroup_r( node->next, sheet, buffer, bufferEnd, x + node->u.child->width, y );
		}
	}
	else
	{
		if ( r_debugOptTex->integer )
			ri.Printf(
				0,
				"  %-36s(%4i,%4i)%4i x %4i from %4i x %4i\n",
				node->u.image->imgName,
				x,
				y,
				node->width,
				node->height,
				node->u.image->width,
				node->u.image->height );
		LoadImageTile( (int)node->u.image, sheet, buffer, bufferEnd, x, y );
	}
}

/* ---- UploadImageGroup  0x004EF8E0 ----  VERIFIED */
/* A single node uploads directly; a multi-node group allocates a "*sheetNNN"
 * atlas, sizes it (mip chain included for compressed formats), packs every
 * member into it, and uploads the atlas. */
void __cdecl UploadImageGroup( int *nodePtr, int groupIndex, int format, int flags )
{
	imageGroupNode_t *node = (imageGroupNode_t *)nodePtr;
	image_t          *sheet;
	int               cardMemory;
	int               w, h;
	unsigned char    *buffer;

	if ( !node->next )
	{
		if ( r_debugOptTex->integer )
			ri.Printf(
				0,
				"image %2i: %4i x %4i from %4i x %4i -- %s\n",
				groupIndex,
				node->width,
				node->height,
				node->u.image->width,
				node->u.image->height,
				node->u.image->imgName );
		R_LoadSingleDelayedImage( (int)node->u.image );
		return;
	}

	if ( r_debugOptTex->integer )
		ri.Printf( 0, "sheet %2i: %4i x %4i\n", groupIndex, node->width, node->height );
	sheet = R_AllocImage( va( "*sheet%03i", groupIndex ), 3553, node->width, node->height, flags, 7 );
	sheet->internalFormat = format;
	cardMemory = GetCardMemoryAmount( format, node->width, node->height );
	if ( format != 6408 && ( sheet->flags & 0x0001 ) )
	{
		w = node->width;
		h = node->height;
		do
		{
			do
			{
				w = ( w + 1 ) >> 1;
				h = ( h + 1 ) >> 1;
				cardMemory += GetCardMemoryAmount( format, w, h );
			}
			while ( w != 1 );
		}
		while ( h != 1 );
	}
	buffer = (unsigned char *)ri.Malloc( cardMemory );
	UploadImageGroup_r( node, (int)sheet, (int)buffer, (int)&buffer[cardMemory], 0, 0 );
	if ( format == 6408 && r_debugOptTex->integer )
		SaveJPG( (int)va( "%s.jpg", sheet->imgName + 1 ), 100, sheet->width, sheet->height, 0, (int)buffer );
	R_CreateImageInternal( sheet, 0, buffer, 0xDE1u, format );
	ri.Free( buffer );
}

/* ---- compare_mergable_common  0x004EFA60 ----  [HIGH] */
/* Orders two packing nodes by min(width,height), then by width+height, then by
 * triangleCount -- the packer's "which is harder to place" ordering. */
int __cdecl compare_mergable_common(unsigned int *nodeA, unsigned int *nodeB)
{
  imageGroupNode_t *n1 = (imageGroupNode_t *)nodeA;
  imageGroupNode_t *n2 = (imageGroupNode_t *)nodeB;
  int min1 = ( n1->width < n1->height ) ? n1->width : n1->height;
  int min2 = ( n2->width < n2->height ) ? n2->width : n2->height;
  int result;

  result = min1 - min2;
  if ( !result )
  {
    result = ( n1->width + n1->height ) - ( n2->width + n2->height );
    if ( !result )
      return n1->triangleCount - n2->triangleCount;
  }
  return result;
}

/* ---- compare_mergable_xxxxx  0x004EFAB0 ----  [HIGH] */
/* qsort comparator over node pointers: group first, then compare_mergable_common. */
int __cdecl compare_mergable_xxxxx(unsigned int **slotA, unsigned int **slotB)
{
  imageGroupNode_t *n1 = (imageGroupNode_t *)*slotA;
  imageGroupNode_t *n2 = (imageGroupNode_t *)*slotB;
  int result = n1->group - n2->group;

  if ( !result )
    return compare_mergable_common( (unsigned int *)n1, (unsigned int *)n2 );
  return result;
}

/* ---- compare_mergable_xxxx  0x004EFAE0 ----  [HIGH] */
/* qsort comparator over node pointers that sinks NULL entries to the end,
 * otherwise ordering by compare_mergable_common. */
int __cdecl compare_mergable_xxxx(unsigned int **slotA, unsigned int **slotB)
{
  imageGroupNode_t *n1 = (imageGroupNode_t *)*slotA;
  imageGroupNode_t *n2 = (imageGroupNode_t *)*slotB;

  if ( !n1 )
    return n2 != 0;
  if ( !n2 )
    return -1;
  return compare_mergable_common( (unsigned int *)n1, (unsigned int *)n2 );
}

/* ---- PickMergeDirection  0x004EFB10 ----  VERIFIED */
int __cdecl PickMergeDirection(int nodes, int count, int startIndex)
{
  imageGroupNode_t **list = (imageGroupNode_t **)nodes;
  int width;
  int height;
  int i;

  height = list[startIndex]->height;
  width  = list[startIndex]->width;
  if ( width == height )
    return 0;
  if ( width != glConfig_maxTextureSize )
  {
    for ( i = startIndex + 1; i < count; ++i )
    {
      if ( list[i]->height != height )
        break;
      if ( list[i]->width > 2 * width )
        break;
      if ( list[i]->width == 2 * width )
      {
        for ( ; i < count; ++i )
        {
          if ( list[i]->height != height )
            break;
        }
        for ( ; i < count; ++i )
        {
          if ( list[i]->width >= width || list[i]->height != 2 * height )
            break;
        }
        if ( i < count && list[i]->width == width && list[i]->height == 2 * height )
          return 1;
        return 0;
      }
    }
  }
  return 1;
}

/* ---- MergeImageList  0x004EFBB0 ----  VERIFIED */
int __cdecl MergeImageList(int pool, int nodes, int count, int outIndex)
{
  imageGroupNode_t **list = (imageGroupNode_t **)nodes;
  int                i;
  int                j;

  if ( count == 1 )
    return outIndex;
  for ( i = 1; i < count; )
  {
    if ( list[i]->height == glConfig.maxTextureSize )
      return outIndex;
    if ( list[i]->width == list[i - 1]->width && list[i]->height == list[i - 1]->height )
    {
      ((imageGroupNode_t *)pool)[outIndex].u.child       = list[i - 1];
      ((imageGroupNode_t *)pool)[outIndex].next          = list[i];
      ((imageGroupNode_t *)pool)[outIndex].group         = list[i]->group;
      ((imageGroupNode_t *)pool)[outIndex].triangleCount = list[i]->triangleCount + list[i - 1]->triangleCount;
      switch ( PickMergeDirection( nodes, count, i ) )
      {
        case 0:
          ((imageGroupNode_t *)pool)[outIndex].width  = 2 * list[i]->width;
          ((imageGroupNode_t *)pool)[outIndex].height = list[i]->height;
          break;
        case 1:
          ((imageGroupNode_t *)pool)[outIndex].width  = list[i]->width;
          ((imageGroupNode_t *)pool)[outIndex].height = 2 * list[i]->height;
          break;
      }
      for ( j = i + 1; j < count; j++ )
      {
        if ( list[j]->height > ((imageGroupNode_t *)pool)[outIndex].height
          || ( list[j]->height == ((imageGroupNode_t *)pool)[outIndex].height && list[j]->width >= ((imageGroupNode_t *)pool)[outIndex].width ) )
          break;
        list[j - 2] = list[j];
      }
      list[j - 2] = &((imageGroupNode_t *)pool)[outIndex];
      for ( ; j < count; j++ )
        list[j - 1] = list[j];
      list[j - 1] = 0;
      count--;
      outIndex++;
    }
    else
    {
      i++;
    }
  }
  return outIndex;
}

/* ---- CombineImageGroups  0x004EFD00 ----  VERIFIED */
int __cdecl CombineImageGroups(unsigned int *countPtr, unsigned int nodes)
{
  imageGroupNode_t **list = (imageGroupNode_t **)nodes;
  int                i;
  int                j;
  int                keepGroup;
  int                mergeGroup;

  qsort_m(nodes, *countPtr, 4u, (int (__cdecl *)(unsigned int, byte *))compare_mergable_xxxx);
  while ( *countPtr && !list[*countPtr - 1] )
    --*countPtr;
  for ( i = 1; i < (int)*countPtr; i++ )
  {
    if ( list[i]->height == glConfig_maxTextureSize )
      break;
    if ( list[i]->width == list[i - 1]->width && list[i]->height == list[i - 1]->height )
    {
      mergeGroup = list[i]->group;
      keepGroup  = list[i - 1]->group;
      for ( j = 0; j < (int)*countPtr; j++ )
      {
        if ( list[j]->group == mergeGroup )
          list[j]->group = keepGroup;
      }
      qsort_m(nodes, *countPtr, 4u, (int (__cdecl *)(unsigned int, byte *))compare_mergable_xxxxx);
      return keepGroup;
    }
  }
  return 0;
}

/* ---- MergeAndLoadDelayedImages  0x004EFDC0 ----  VERIFIED */
void __cdecl MergeAndLoadDelayedImages( image_t **images, int count, int format, int flags )
{
  imageGroupNode_t  *records;
  imageGroupNode_t **list;
  int                i;
  int                picmip;
  int                outIndex;
  int                group;
  int                start;
  int                end;

  if ( r_debugOptTex->integer )
    ri.Printf( 0, "%i groups -> ", count );
  /* n leaves merge into at most n - 1 interior records */
  records = (imageGroupNode_t *)_alloca( ( 2 * count - 1 ) * sizeof( imageGroupNode_t ) );
  list    = (imageGroupNode_t **)_alloca( ( 2 * count - 1 ) * sizeof( imageGroupNode_t * ) );
  for ( i = 0; i < count; i++ )
  {
    records[i].u.image       = images[i];
    records[i].next          = 0;
    records[i].group         = images[i]->state.delay.group;
    records[i].triangleCount = images[i]->state.delay.groupTriCount;
    records[i].width  = images[i]->width > images[i]->height ? images[i]->width : images[i]->height;
    records[i].height = images[i]->width + images[i]->height - records[i].width;
    if ( images[i]->flags & IMAGE_FLAG_ALLOW_PICMIP )
    {
      for ( picmip = R_PicmipForImageFlags( images[i]->flags ); picmip && records[i].width > 4 && records[i].height > 4; picmip-- )
      {
        records[i].width  >>= 1;
        records[i].height >>= 1;
      }
    }
    while ( records[i].width > glConfig.maxTextureSize )
    {
      records[i].width  >>= 1;
      records[i].height >>= 1;
    }
    list[i] = &records[i];
  }
  outIndex = count;
  qsort_m( (unsigned int)list, count, 4u, (int (__cdecl *)(unsigned int, byte *))compare_mergable_xxxxx );
  for ( start = 0; start < count; start = end )
  {
    for ( end = start + 1; end < count; end++ )
    {
      if ( list[start]->group != list[end]->group )
        break;
    }
    outIndex = MergeImageList( (int)records, (int)&list[start], end - start, outIndex );
  }
  while ( ( group = CombineImageGroups( (unsigned int *)&count, (unsigned int)list ) ) != 0 )
  {
    for ( start = 0; list[start]->group != group; start++ )
      ;
    for ( end = start + 1; end < count; end++ )
    {
      if ( list[start]->group != list[end]->group )
        break;
    }
    outIndex = MergeImageList( (int)records, (int)&list[start], end - start, outIndex );
  }
  if ( r_debugOptTex->integer )
    ri.Printf( 0, "%i groups\n", count );
  for ( i = 0; i < count; i++ )
    UploadImageGroup( (int *)list[i], i, format, flags );
}

/* ---- compare_image_types  0x004F0030 ----  [HIGH] */
/* qsort comparator over image-pointer slots: internalFormat then mipmap flag. */
int __cdecl compare_image_types(int slotA, int slotB)
{
  image_t *imageA = *(image_t **)slotA;
  image_t *imageB = *(image_t **)slotB;
  int result = (int)imageA->internalFormat - (int)imageB->internalFormat;

  if ( !result )
    return ( imageA->flags & IMAGE_FLAG_MIPMAP ) - ( imageB->flags & IMAGE_FLAG_MIPMAP );
  return result;
}

/* ---- R_LoadDelayedImages  0x004F0060 ----  VERIFIED */
/* Gathers every delayed image, sorts by (format, mipmap), then hands each run
 * of same-format/same-mipmap images to MergeAndLoadDelayedImages (which packs
 * them into sheets and uploads). */
void R_LoadDelayedImages()
{
	image_t **delayed;
	int       count;
	int       i;
	int       start;
	int       end;

	(*(int *)(((char *)(void *)&tr_registered) + 0x0128)) = 0;
	if ( !tr_delayedImageCount )
		return;

	delayed = (image_t **)_alloca( tr_delayedImageCount * sizeof( image_t * ) );
	count = 0;
	for ( i = 0; i < tr_numImages; i++ )
	{
		if ( tr_images[i] && ( tr_images[i]->flags & IMAGE_FLAG_DELAYED_UPLOAD ) )
			delayed[count++] = tr_images[i];
	}

	qsort_m( (unsigned int)delayed, count, 4u, (int (__cdecl *)(unsigned int, byte *))compare_image_types );

	for ( start = 0; start < count; start = end )
	{
		for ( end = start + 1; end < count; end++ )
		{
			if ( delayed[start]->internalFormat != delayed[end]->internalFormat )
				break;
			if ( ( delayed[start]->flags ^ delayed[end]->flags ) & IMAGE_FLAG_MIPMAP )
				break;
		}
		MergeAndLoadDelayedImages( &delayed[start], end - start, delayed[start]->internalFormat,
		                           delayed[start]->flags & IMAGE_FLAG_MIPMAP );
	}

	tr_delayedImageCount = 0;
}

/* ---- R_CreateDlightImage  0x004F0150 ----  VERIFIED */
void R_CreateDlightImage()
{
	int            x, y;
	int            b;
	unsigned char  data[64][64][4];
	unsigned char  tga[18 + sizeof( data )];   /* one block: 18-byte header, pixels at +18 */
	cvar_t        *radiusCvar;
	cvar_t        *shiftCvar;
	rfloat         shiftSq;
	float          scale;
	image_t       *image;

	radiusCvar = ri.Cvar_Get( "r_dlightRadius", "0.1124278", 0 );
	shiftCvar  = ri.Cvar_Get( "r_dlightShift", "0.3162278", 0 );
	shiftSq = (rfloat)shiftCvar->value * shiftCvar->value;
	scale   = 255.0f / ( 1.0f / ( (rfloat)radiusCvar->value * radiusCvar->value + shiftSq )
	                     - 1.0f / ( shiftSq + 1.0f ) );

	for ( x = 0; x < 64; ++x )
	{
		for ( y = 0; y < 64; ++y )
		{
			/* signed __ftol2 at 0x004F022E (out-of-range / NaN -> 0x80000000 -> 0);
			 * flt_569188 = 0x3A841883 = (float)(1 / 31.5^2) */
			b = (int)( scale / ( (rfloat)( ( 31.5f - x ) * ( 31.5f - x ) + ( 31.5f - y ) * ( 31.5f - y ) )
			                     * 0.0010078106f + shiftSq )
			           - 1.0f / ( shiftSq + 1.0f ) * scale );
			if ( b > 255 )
				b = 255;
			else if ( b < 0 )
				b = 0;
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = (unsigned char)b;
			data[y][x][3] = 255;
		}
	}

	image = R_AllocImage( "*dlight", 3553, 64, 64, 48, 0 );
	if ( !R_CreateImageInternal( image, 0, (unsigned char *)data, 0xDE1u, 6408 ) )
	{
		R_FreeImage( image );
		image = 0;
	}
	tr_dlightImage = (int)image;

	if ( radiusCvar->modificationCount > 1 || shiftCvar->modificationCount > 1 )
	{
		memset( tga, 0, 18 );
		tga[2]  = 2;    /* uncompressed true-colour */
		tga[12] = 64;   /* width */
		tga[13] = 0;
		tga[14] = 64;   /* height */
		tga[15] = 0;
		tga[16] = 32;   /* bits per pixel */
		memcpy( tga + 18, data, sizeof( data ) );
		ri.FS_WriteFile( "dlight.tga", tga, sizeof( tga ) );
	}
}

/* ---- R_CreateDefaultImage  0x004F0380 ----  VERIFIED */
/* 16x16 dark-grey (0x20202020) square with an opaque black one-pixel border. */
void R_CreateDefaultImage()
{
	int           x;
	unsigned char data[16][16][4];
	image_t      *image;

	memset( data, 32, sizeof( data ) );
	for ( x = 0; x < 16; ++x )
	{
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] = 0;
		data[0][x][3] = 255;

		data[15][x][0] =
		data[15][x][1] =
		data[15][x][2] = 0;
		data[15][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] = 0;
		data[x][0][3] = 255;

		data[x][15][0] =
		data[x][15][1] =
		data[x][15][2] = 0;
		data[x][15][3] = 255;
	}

	image = R_AllocImage( "*default", 3553, 16, 16, 1, 0 );
	if ( !R_CreateImageInternal( image, 0, (unsigned char *)data, 0xDE1u, 6408 ) )
	{
		R_FreeImage( image );
		image = 0;
	}
	tr_defaultImage = (int)image;
}

/* ---- SmallestTextureSizeFitting  0x004F0480 ----  [HIGH] */
/* Rounds `size` up to the next power of two (>= 1). */
int __cdecl SmallestTextureSizeFitting( int size )
{
	int result = 1;

	for ( --size; size; size >>= 1 )
		result <<= 1;
	return result;
}

/* ---- R_CreateScreenImage  0x004F04A0 ----  VERIFIED */
/* Full-white capture image sized to the next power of two on each axis.
 * (glConfig_vidWidth / glConfig_vidHeight hold the screen width / height.) */
void R_CreateScreenImage()
{
	int      width  = SmallestTextureSizeFitting( glConfig_vidWidth );
	int      height = SmallestTextureSizeFitting( glConfig_vidHeight );
	int     *pixels;
	image_t *image;

	pixels = (int *)ri_Hunk_AllocateTempMemory( 4 * width * height );
	Com_Memset( pixels, 255, 4 * width * height );

	image = R_AllocImage( "*screen", 3553, width, height, 560, 0 );
	if ( !R_CreateImageInternal( image, 0, (unsigned char *)pixels, 0xDE1u, 6408 ) )
	{
		R_FreeImage( image );
		image = 0;
	}
	tr_screenImage       = (int)image;
	tr_screenImageWidth  = width;
	tr_screenImageHeight = height;
	ri_Hunk_FreeTempMemory( pixels );
}

/* ---- R_CreateBuiltinImages  0x004F0560 ----  VERIFIED */
/* Creates the *default / *white / *identityLight / *gray images, 32 *scratch
 * images, the *dlight image and the *screen image. */
void R_CreateBuiltinImages()
{
	int           x, y;
	unsigned char data[16][16][4];
	image_t      *image;

	R_CreateDefaultImage();

	/* *white -- solid 0xFF */
	memset( data, 255, sizeof( data ) );
	image = R_AllocImage( "*white", 3553, 8, 8, 0, 0 );
	if ( !R_CreateImageInternal( image, 0, (unsigned char *)data, 0xDE1u, 6408 ) )
	{
		R_FreeImage( image );
		image = 0;
	}
	tr_whiteImage = (int)image;

	/* *identityLight -- RGB = tr_identityLightByte, A = 255 */
	for ( x = 0; x < 16; ++x )
	{
		for ( y = 0; y < 16; ++y )
		{
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = (unsigned char)tr_identityLightByte;
			data[y][x][3] = 255;
		}
	}
	image = R_AllocImage( "*identityLight", 3553, 8, 8, 0, 0 );
	if ( !R_CreateImageInternal( image, 0, (unsigned char *)data, 0xDE1u, 6408 ) )
	{
		R_FreeImage( image );
		image = 0;
	}
	tr_identityLightImage = (int)image;

	/* *gray -- solid 0xC0 */
	memset( data, 0xC0, sizeof( data ) );
	image = R_AllocImage( "*gray", 3553, 8, 8, 0, 0 );
	if ( !R_CreateImageInternal( image, 0, (unsigned char *)data, 0xDE1u, 6408 ) )
	{
		R_FreeImage( image );
		image = 0;
	}
	tr_grayImage = (int)image;

	/* 32 *scratch images (reuse the *gray pixels) */
	for ( x = 0; x < 32; ++x )
	{
		image = R_AllocImage( "*scratch", 3553, 16, 16, 50, 0 );
		if ( !R_CreateImageInternal( image, 0, (unsigned char *)data, 0xDE1u, 6408 ) )
		{
			R_FreeImage( image );
			image = 0;
		}
		tr_scratchImages[x] = (int)image;
	}

	R_CreateDlightImage();
	R_CreateScreenImage();
}

/* ---- R_SetColorMappings  0x004F0780 ----  VERIFIED */
/* Builds the gamma and intensity lookup tables from r_overBrightBits /
 * r_gamma / r_intensity (clamping each), then pushes the gamma ramp to the
 * device if it supports hardware gamma.  (The retail 1.0/0.5/3.0 tests are the
 * same ordered comparisons once the NaN-only __UNORDERED__ terms are dropped.) */
void R_SetColorMappings()
{
	int   i, j;
	float g;
	int   inf;
	int   shift;

	tr_overbrightBits = r_overBrightBits->integer;
	if ( !glConfig_deviceSupportsGamma )
		tr_overbrightBits = 0;
	if ( !glConfig_isFullscreen )
		tr_overbrightBits = 0;

	if ( glConfig_colorBits > 16 )
	{
		if ( tr_overbrightBits > 2 )
			tr_overbrightBits = 2;
	}
	else
	{
		if ( tr_overbrightBits > 1 )
			tr_overbrightBits = 1;
	}
	if ( tr_overbrightBits < 0 )
		tr_overbrightBits = 0;

	tr_identityLight     = 1.0f / ( 1 << tr_overbrightBits );
	tr_identityLightByte = (unsigned char)(int)( 255 * tr_identityLight );   /* __ftol2 at 0x004F0805, movzx al */

	if ( r_intensity->value <= 1 )
		ri.Cvar_Set( "r_intensity", "1" );

	if ( r_gamma->value < 0.5f )
		ri.Cvar_Set( "r_gamma", "0.5" );
	else if ( r_gamma->value > 3.0f )
		ri.Cvar_Set( "r_gamma", "3.0" );

	g = r_gamma->value;

	shift = tr_overbrightBits;

	for ( i = 0; i < 256; ++i )
	{
		if ( g == 1 )
			inf = i;
		else
			inf = (int)( 255 * pow( (rfloat)i * 0.0039215689f, 1.0f / (rfloat)g ) + 0.5f );   /* __ftol2 at 0x004F08DA; flt_568EC4 = (float)(1/255) = 0x3B808081 */
		inf <<= shift;
		if ( inf < 0 )
			inf = 0;
		if ( inf > 255 )
			inf = 255;
		s_gammatable[i] = (unsigned char)inf;
	}

	for ( i = 0; i < 256; ++i )
	{
		/* signed __ftol2 at 0x004F0927: a product >= 2^31 gives 0x80000000,
		 * which the signed > 255 test lets through as byte 0 */
		j = (int)( (rfloat)i * r_intensity->value );
		if ( j > 255 )
			j = 255;
		s_intensitytable[i] = (unsigned char)j;
	}

	if ( glConfig_deviceSupportsGamma )
		GLimp_SetGamma( (unsigned char *)s_gammatable,
		                (unsigned char *)s_gammatable,
		                (unsigned char *)s_gammatable );
}

/* ---- R_InitImages  0x004F0980 ----  [HIGH] */
void R_InitImages()
{
  memset(hashtable, 0, sizeof(hashtable));
  R_SetColorMappings();
  R_CreateBuiltinImages();
}

/* ---- R_DeleteTextures  0x004F09A0 ----  VERIFIED */
void R_DeleteTextures()
{
  int i;

  for ( i = 0; i < tr_numImages; ++i )
    qglDeleteTextures( 1, &tr_images[i]->texnum );
  memset(tr_imageSlots, 0, R_MAX_IMAGES * sizeof(image_t *));   /* rep stosd 0x800 dwords at 0x004F09D1 */
  memset(glState_currentTextures, 0, 0x20u);
  if ( glBindTexture )
  {
    if ( qglActiveTextureARB )
    {
      GL_SelectTexture( 1 );
      glBindTexture( 0xDE1u, 0 );
      GL_SelectTexture( 0 );
      glBindTexture( 0xDE1u, 0 );
    }
    else
    {
      glBindTexture( 0xDE1u, 0 );
    }
  }
}

int __cdecl R_GetModelByHandle( int a1 );   /* tr_model.c */

/* ---- RE_GetShaderFromModel  0x004F0AA0 ----  VERIFIED */
/* Resolves the material for one brush-model surface.  If the surface's shader
 * is lightmapped, an existing image of the same name decides whether mipmaps
 * are allowed, then R_FindShader is (re)invoked and two of its stage words are
 * patched.  The material-internal offsets (+85 dwords, +1636, +1668, +17) are
 * left raw -- they index a stage record beyond this file's shader_t view. */
int __cdecl RE_GetShaderFromModel( int modelIndex, int surfaceNum )
{
	model_t     *model;
	bmodel_t    *bmodel;
	msurface_t  *surfaces;
	msurface_t  *surf;
	shader_t    *shader;
	image_t     *image;
	int          mipmapAllowed;
	int          hash;

	if ( surfaceNum < 0 )
		surfaceNum = 0;
	model = (model_t *)R_GetModelByHandle( modelIndex );
	if ( !model || !( bmodel = model->bmodel ) || !( surfaces = bmodel->firstSurface ) )
		return 0;
	if ( surfaceNum >= bmodel->numSurfaces )
		surfaceNum = 0;
	surf = &surfaces[ surfaceNum ];

	shader = (shader_t *)surf->shader;
	if ( shader->lightmapIndex > -1 )
	{
		mipmapAllowed = 1;
		hash = tr_image_generateHashValue( (char *)surf->shader );
		for ( image = ( (image_t **)(void *)hashtable )[ hash ]; image; image = image->hashNext )
		{
			if ( !strcmp( (char *)surf->shader, image->imgName ) )
			{
				mipmapAllowed = image->flags & 0x0001;
				break;
			}
		}

		shader = (shader_t *)R_FindShader( (char *)surf->shader, -1, mipmapAllowed, (const char *)8 );
		*(unsigned int *)( *( (unsigned int *)shader + 85 ) + 1636 ) = 10;
		*(unsigned int *)( *( (unsigned int *)shader + 85 ) + 1668 ) = 1048832;
	}
	return shader->index;
}
