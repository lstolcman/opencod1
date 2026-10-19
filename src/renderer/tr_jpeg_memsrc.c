/*
 * @fidelity: verified
 */

#include "../jpeg-6/jinclude.h"
#include "../jpeg-6/jpeglib.h"
#include "../jpeg-6/jerror.h"

/* 0x005163DE `mov ecx, 400h / rep movsd` -- 1024 dwords. */
#define COD_JPEG_INPUT_BUF_SIZE  4096

/*
 * CoD's in-memory decompression source manager -- libjpeg's jdatasrc.c
 * `my_source_mgr` pattern, except `infile` is a raw byte pointer into the
 * already-loaded file image rather than a FILE*.  Layout is IDA-verified
 * against CoDMP.exe: pub at +0x00 (the 28-byte jpeg_source_mgr), infile at
 * +0x1c, buffer at +0x20, start_of_file at +0x24 (a 1-byte boolean), for a
 * total of 0x28 (40) bytes -- matching the alloc_small(cinfo, JPOOL_PERMANENT,
 * 40) at 0x00516440.
 */
typedef struct {
  struct jpeg_source_mgr pub;   /* +0x00  base source-mgr (jpeglib.h) */
  const JOCTET *infile;         /* +0x1c  next byte in the source image */
  JOCTET *buffer;               /* +0x20  4 KiB staging buffer */
  boolean start_of_file;        /* +0x24  set by init_source, cleared by fill */
} cod_source_mgr;

typedef cod_source_mgr *cod_src_ptr;

/* Pin the subclass size to the retail allocation.  Self-contained (this TU is
 * the libjpeg boundary and deliberately pulls in no q_shared.h), so a header-
 * free negative-array guard stands in for COD1_ASSERT_SIZE. */
typedef char cod_source_mgr_size_check[ SIZEOF( cod_source_mgr ) == 40 ? 1 : -1 ];


/* ---- CoD_jpeg_init_source  0x00516360 ----  VERIFIED */
static void
CoD_jpeg_init_source( j_decompress_ptr cinfo )
{
  cod_src_ptr src = (cod_src_ptr) cinfo->src;

  src->start_of_file = TRUE;
}


/* ---- CoD_jpeg_fill_input_buffer  0x00516370 ----  VERIFIED */
static boolean
CoD_jpeg_fill_input_buffer( j_decompress_ptr cinfo )
{
  cod_src_ptr src = (cod_src_ptr) cinfo->src;

  MEMCOPY( src->buffer, src->infile, COD_JPEG_INPUT_BUF_SIZE );
  src->infile += COD_JPEG_INPUT_BUF_SIZE;

  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = COD_JPEG_INPUT_BUF_SIZE;
  src->start_of_file = FALSE;

  return TRUE;
}


/* ---- CoD_jpeg_skip_input_data  0x005163B0 ----  VERIFIED */
static void
CoD_jpeg_skip_input_data( j_decompress_ptr cinfo, long num_bytes )
{
  cod_src_ptr src = (cod_src_ptr) cinfo->src;

  if ( num_bytes > 0 ) {
    while ( num_bytes > (long) src->pub.bytes_in_buffer ) {
      num_bytes -= (long) src->pub.bytes_in_buffer;
      (void) CoD_jpeg_fill_input_buffer( cinfo );
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}


/* ---- CoD_jpeg_term_source  0x00516420 ----  VERIFIED */
static void
CoD_jpeg_term_source( j_decompress_ptr cinfo )
{
  (void) cinfo;
}


/* ---- CoD_jpeg_mem_src  0x00516430 ----  VERIFIED */
void
CoD_jpeg_mem_src( j_decompress_ptr cinfo, const unsigned char *inbuffer )
{
  cod_src_ptr src;

  if ( cinfo->src == NULL ) {
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ( (j_common_ptr) cinfo, JPOOL_PERMANENT,
                                   SIZEOF( cod_source_mgr ) );
    src = (cod_src_ptr) cinfo->src;
    src->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ( (j_common_ptr) cinfo, JPOOL_PERMANENT,
                                   COD_JPEG_INPUT_BUF_SIZE * SIZEOF( JOCTET ) );
  }

  src = (cod_src_ptr) cinfo->src;
  src->pub.init_source       = CoD_jpeg_init_source;
  src->pub.fill_input_buffer = CoD_jpeg_fill_input_buffer;
  src->pub.skip_input_data   = CoD_jpeg_skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart;   /* 0x00519A60 */
  src->pub.term_source       = CoD_jpeg_term_source;
  src->infile                = (const JOCTET *) inbuffer;
  src->pub.bytes_in_buffer   = 0;
  src->pub.next_input_byte   = NULL;
}
