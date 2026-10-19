/*
 * zlib_types.h -- the zlib 1.1.4 types this tree's zlib units use.
 *
 * CoDMP.exe links zlib 1.1.4 (zlibVersion returns "1.1.4") and the
 * Quake3/RTCW-style minizip.  The tree has no zlib headers, so the pieces of
 * zlib.h / zconf.h / zutil.h / infblock.h / infcodes.h / inftrees.h /
 * infutil.h / deflate.h that zlib/inflate.c and zlib/unzip.c need are
 * collected here, with zlib's own names (references/RTCW-MP qcommon/unzip.c
 * embeds the same 1.1.x sources).
 *
 * Every size and offset below is pinned by retail: sizeof(z_stream) = 56 (the
 * stream_size inflateInit2_ checks), inflate_blocks_state = 64 (ZALLOC(1,64)),
 * inflate_codes_state = 28 (ZALLOC(1,28)), internal_state = 24 (ZALLOC(1,24)),
 * inflate_huft = 8, and deflate_state's fields at the offsets the trees.c
 * functions address (dyn_ltree 0x8C ... bi_valid 0x16B4).
 *
 * Types only: nothing here emits code or data.
 */
#ifndef ZLIB_TYPES_H
#define ZLIB_TYPES_H

/* zconf.h */
typedef unsigned char  Byte;    /* 8 bits */
typedef unsigned int   uInt;    /* 16 bits or more */
typedef unsigned long  uLong;   /* 32 bits or more */
typedef Byte           Bytef;
typedef void          *voidpf;

#define MAX_WBITS   15 /* 32K LZ77 window */

/* zutil.h */
typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

/* zlib.h */
typedef voidpf (__cdecl *alloc_func)(voidpf opaque, uInt items, uInt size);
typedef void   (__cdecl *free_func)(voidpf opaque, voidpf address);

struct internal_state;

typedef struct z_stream_s {
    Bytef    *next_in;  /* next input byte */
    uInt     avail_in;  /* number of bytes available at next_in */
    uLong    total_in;  /* total nb of input bytes read so far */

    Bytef    *next_out; /* next output byte should be put there */
    uInt     avail_out; /* remaining free space at next_out */
    uLong    total_out; /* total nb of bytes output so far */

    char     *msg;      /* last error message, NULL if no error */
    struct internal_state *state; /* not visible by applications */

    alloc_func zalloc;  /* used to allocate the internal state */
    free_func  zfree;   /* used to free the internal state */
    voidpf     opaque;  /* private data object passed to zalloc and zfree */

    int     data_type;  /* best guess about the data type: ascii or binary */
    uLong   adler;      /* adler32 value of the uncompressed data */
    uLong   reserved;   /* reserved for future use */
} z_stream;

typedef z_stream *z_streamp;

/* zlib.h constants */
#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define Z_BINARY   0
#define Z_ASCII    1
#define Z_UNKNOWN  2

#define Z_DEFLATED   8

#define Z_NULL  0  /* for initializing zalloc, zfree, opaque */

#define ZLIB_VERSION "1.1.4"

/* zutil.h: allocation through the stream's hooks */
#define ZALLOC(strm, items, size) \
           (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))

/* infblock.h / infutil.h: the window check function (adler32 or none) */
typedef uLong (__cdecl *check_func)(uLong check, const Bytef *buf, uInt len);

/*
 * inftrees.h.  Upstream's entry is { union { struct { Byte Exop; Byte Bits; }
 * what; uInt pad; } word; uInt base; } behind `#define exop word.what.Exop`
 * and `#define bits word.what.Bits`.  inflate.c shares its translation unit
 * with trees.c, whose locals are called `bits`, so the two bytes are plain
 * members here; the layout (exop 0, bits 1, pad, base 4 -- 8 bytes) is the
 * same.
 */
typedef struct inflate_huft_s {
    Byte exop;          /* number of extra bits or operation */
    Byte bits;          /* number of bits in this code or subcode */
    uInt base;          /* literal, length base, distance base,
                           or table offset */
} inflate_huft;

/* Maximum size of dynamic tree (inftrees.h). */
#define MANY 1440

/* infcodes.c */
typedef enum {        /* waiting for "i:"=input, "o:"=output, "x:"=nothing */
      START,    /* x: set up for LEN */
      LEN,      /* i: get length/literal/eob next */
      LENEXT,   /* i: getting length extra (have base) */
      DIST,     /* i: get distance next */
      DISTEXT,  /* i: getting distance extra */
      COPY,     /* o: copying bytes in window, waiting for space */
      LIT,      /* o: got literal, waiting for output space */
      WASH,     /* o: got eob, possibly still output waiting */
      END,      /* x: got eob and all data flushed */
      BADCODE}  /* x: got error */
inflate_codes_mode;

/* inflate codes private state (28 bytes) */
typedef struct inflate_codes_state {

  /* mode */
  inflate_codes_mode mode;      /* current inflate_codes mode */

  /* mode dependent information */
  uInt len;
  union {
    struct {
      inflate_huft *tree;       /* pointer into tree */
      uInt need;                /* bits needed */
    } code;             /* if LEN or DIST, where in tree */
    uInt lit;           /* if LIT, literal */
    struct {
      uInt get;                 /* bits to get for extra */
      uInt dist;                /* distance back to copy from */
    } copy;             /* if EXT or COPY, where and how much */
  } sub;                /* submode */

  /* mode independent information */
  Byte lbits;           /* ltree bits decoded per branch */
  Byte dbits;           /* dtree bits decoder per branch */
  inflate_huft *ltree;          /* literal/length/eob tree */
  inflate_huft *dtree;          /* distance tree */

} inflate_codes_statef;

/* infutil.h */
typedef enum {
      TYPE,     /* get type bits (3, including end bit) */
      LENS,     /* get lengths for stored */
      STORED,   /* processing stored block */
      TABLE,    /* get table lengths */
      BTREE,    /* get bit lengths tree for a dynamic block */
      DTREE,    /* get length, distance trees for a dynamic block */
      CODES,    /* processing fixed or dynamic block */
      DRY,      /* output remaining window bytes */
      DONE,     /* finished last block, done */
      BAD}      /* got a data error--stuck here */
inflate_block_mode;

/* inflate blocks semi-private state (64 bytes) */
typedef struct inflate_blocks_state {

  /* mode */
  inflate_block_mode  mode;     /* current inflate_block mode */

  /* mode dependent information */
  union {
    uInt left;          /* if STORED, bytes left to copy */
    struct {
      uInt table;               /* table lengths (14 bits) */
      uInt index;               /* index into blens (or border) */
      uInt *blens;              /* bit lengths of codes */
      uInt bb;                  /* bit length tree depth */
      inflate_huft *tb;         /* bit length decoding tree */
    } trees;            /* if DTREE, decoding info for trees */
    struct {
      inflate_codes_statef
         *codes;
    } decode;           /* if CODES, current state */
  } sub;                /* submode */
  uInt last;            /* true if this block is the last block */

  /* mode independent information */
  uInt bitk;            /* bits in bit buffer */
  uLong bitb;           /* bit buffer */
  inflate_huft *hufts;  /* single malloc for tree space */
  Bytef *window;        /* sliding window */
  Bytef *end;           /* one byte after sliding window */
  Bytef *read;          /* window read pointer */
  Bytef *write;         /* window write pointer */
  check_func checkfn;   /* check function */
  uLong check;          /* check on output */

} inflate_blocks_statef;

/* inflate.c */
typedef enum {
      METHOD,   /* waiting for method byte */
      FLAG,     /* waiting for flag byte */
      DICT4,    /* four dictionary check bytes to go */
      DICT3,    /* three dictionary check bytes to go */
      DICT2,    /* two dictionary check bytes to go */
      DICT1,    /* one dictionary check byte to go */
      DICT0,    /* waiting for inflateSetDictionary */
      BLOCKS,   /* decompressing blocks */
      CHECK4,   /* four check bytes to go */
      CHECK3,   /* three check bytes to go */
      CHECK2,   /* two check bytes to go */
      CHECK1,   /* one check byte to go */
      IDONE,    /* finished check, done (zlib's DONE; renamed, infblock's
                   DONE shares this translation unit) */
      IBAD}     /* got an error--stay here (zlib's BAD, likewise) */
inflate_mode;

/* inflate private state (24 bytes) */
struct internal_state {

  /* mode */
  inflate_mode  mode;   /* current inflate mode */

  /* mode dependent information */
  union {
    uInt method;        /* if FLAGS, method byte */
    struct {
      uLong was;                /* computed check value */
      uLong need;               /* stream check value */
    } check;            /* if CHECK, check values to compare */
    uInt marker;        /* if BAD, inflateSync's marker bytes count */
  } sub;        /* submode */

  /* mode independent information */
  int  nowrap;          /* flag for no wrapper */
  uInt wbits;           /* log2(window size)  (8..15, defaults to 15) */
  inflate_blocks_statef
    *blocks;            /* current inflate_blocks state */

};

/*
 * deflate.h.  Upstream calls this `struct internal_state` too (deflate.c and
 * inflate.c are separate units there); trees.c shares inflate.c's unit here,
 * so the tag is deflate_internal_state.  Offsets are the ones the trees.c
 * functions in inflate.c address.
 */
#define LENGTH_CODES 29     /* number of length codes, not counting the special END_BLOCK code */
#define LITERALS  256       /* number of literal bytes 0..255 */
#define L_CODES (LITERALS+1+LENGTH_CODES)  /* number of Literal or Length codes, including the END_BLOCK code */
#define D_CODES   30        /* number of distance codes */
#define BL_CODES  19        /* number of codes used to transfer the bit lengths */
#define HEAP_SIZE (2*L_CODES+1)  /* maximum heap size */
#define MAX_BITS 15         /* All codes must not exceed MAX_BITS bits */
#define END_BLOCK 256       /* end of block literal code */

/* Data structure describing a single value and its code string. */
typedef struct ct_data_s {
    union {
        ush  freq;       /* frequency count */
        ush  code;       /* bit string */
    } fc;
    union {
        ush  dad;        /* father node in Huffman tree */
        ush  len;        /* length of bit string */
    } dl;
} ct_data;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

typedef struct static_tree_desc_s  static_tree_desc;

typedef struct tree_desc_s {
    ct_data *dyn_tree;           /* the dynamic tree */
    int     max_code;            /* largest code with non zero frequency */
    static_tree_desc *stat_desc; /* the corresponding static tree */
} tree_desc;

/* trees.c */
struct static_tree_desc_s {
    const ct_data *static_tree;  /* static tree or NULL */
    const int *extra_bits;       /* extra bits for each code or NULL */
    int     extra_base;          /* base index for extra_bits */
    int     elems;               /* max number of elements in the tree */
    int     max_length;          /* max bit length for the codes */
};

typedef ush Pos;
typedef Pos Posf;
typedef unsigned IPos;

typedef struct deflate_internal_state {
    z_streamp strm;      /* pointer back to this zlib stream */
    int   status;        /* as the name implies */
    Bytef *pending_buf;  /* output still pending */
    ulg   pending_buf_size; /* size of pending_buf */
    Bytef *pending_out;  /* next pending byte to output to the stream */
    int   pending;       /* nb of bytes in the pending buffer */
    int   noheader;      /* suppress zlib header and adler32 */
    Byte  data_type;     /* UNKNOWN, BINARY or ASCII */
    Byte  method;        /* STORED (for zip only) or DEFLATED */
    int   last_flush;    /* value of flush param for previous deflate call */

    uInt  w_size;        /* LZ77 window size (32K by default) */
    uInt  w_bits;        /* log2(w_size)  (8..16) */
    uInt  w_mask;        /* w_size - 1 */
    Bytef *window;
    ulg window_size;
    Posf *prev;
    Posf *head;          /* Heads of the hash chains or NIL. */

    uInt  ins_h;          /* hash index of string to be inserted */
    uInt  hash_size;      /* number of elements in hash table */
    uInt  hash_bits;      /* log2(hash_size) */
    uInt  hash_mask;      /* hash_size-1 */
    uInt  hash_shift;

    long block_start;

    uInt match_length;           /* length of best match */
    IPos prev_match;             /* previous match */
    int match_available;         /* set if previous match exists */
    uInt strstart;               /* start of string to insert */
    uInt match_start;            /* start of matching string */
    uInt lookahead;              /* number of valid bytes ahead in window */

    uInt prev_length;
    uInt max_chain_length;
    uInt max_lazy_match;
    int level;    /* compression level (1..9) */
    int strategy; /* favor or force Huffman coding*/

    uInt good_match;
    int nice_match; /* Stop searching when current match exceeds this */

    struct ct_data_s dyn_ltree[HEAP_SIZE];   /* literal and length tree */
    struct ct_data_s dyn_dtree[2*D_CODES+1]; /* distance tree */
    struct ct_data_s bl_tree[2*BL_CODES+1];  /* Huffman tree for bit lengths */

    struct tree_desc_s l_desc;               /* desc. for literal tree */
    struct tree_desc_s d_desc;               /* desc. for distance tree */
    struct tree_desc_s bl_desc;              /* desc. for bit length tree */

    ush bl_count[MAX_BITS+1];
    /* number of codes at each bit length for an optimal tree */

    int heap[2*L_CODES+1];      /* heap used to build the Huffman trees */
    int heap_len;               /* number of elements in the heap */
    int heap_max;               /* element of largest frequency */

    uch depth[2*L_CODES+1];
    /* Depth of each subtree used as tie breaker for trees of equal frequency */

    uch *l_buf;          /* buffer for literals or lengths */
    uInt  lit_bufsize;
    uInt last_lit;      /* running index in l_buf */
    ush *d_buf;
    ulg opt_len;        /* bit length of current block with optimal trees */
    ulg static_len;     /* bit length of current block with static trees */
    uInt matches;       /* number of string matches in current block */
    int last_eob_len;   /* bit length of EOB code for last block */

    ush bi_buf;
    /* Output buffer. bits are inserted starting at the bottom (least
     * significant bits). */
    int bi_valid;
    /* Number of valid bits in bi_buf.  All bits above the last valid bit
     * are always zero. */
} deflate_state;

/*
 * The deflate/inflate tables and z_errmsg, defined (initialized) in zlib_rdata.c, which does not
 * include this header: these are the views inflate.c and unzip.c address them by.  IDA's
 * z_errmsg label is &z_errmsg[2] (see zlib_rdata.c).
 */
extern const unsigned char bl_order[19];   /* 0x005407AC */
extern const unsigned char _dist_code[512];   /* 0x00540CB8 (IDA's byte_540DB8 is &_dist_code[256]) */
extern const unsigned char _length_code[256];   /* 0x00540EB8 */
extern const int extra_lbits[29];   /* 0x00540670 */
extern const int extra_dbits[30];   /* 0x005406E8 */
extern const int base_length[29];   /* 0x00540FB8 */
extern const int base_dist[30];   /* 0x00541030 */
extern const unsigned char static_ltree[1152];   /* 0x005407C0, ct_data[288] */
extern const unsigned char static_dtree[120];   /* 0x00540C40, ct_data[30] */
extern int z_errmsg;   /* 0x00571BA8 */
extern int static_l_desc;   /* 0x00571BC8 */
extern int static_d_desc;   /* 0x00571BDC */
extern unsigned char static_bl_desc[24];   /* 0x00571BF0 */

#endif /* ZLIB_TYPES_H */
