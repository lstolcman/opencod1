/*
 * universal/memorytree.cpp
 *
 * @fidelity: likely
 */

#ifdef __cplusplus
/* retail built this unit as C++ (GP2 classes, EH frames); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"
/* zlib/zlib_rdata.c (this unit carries its own deflate types, so not zlib_types.h) */
extern const unsigned char _dist_code[512];   /* 0x00540CB8 */
extern const unsigned char _length_code[256];   /* 0x00540EB8 */
extern int z_errmsg;   /* 0x00571BA8 == &z_errmsg[2] */
extern unsigned char fxMemBlocks[0x200000];   /* EffectsCore/FxMemMgr.cpp, the FX arena */
extern unsigned int theFxScheduler[5409];   /* EffectsCore/fx_rdata.c */
extern unsigned char theFxHelper[132];   /* EffectsCore/fx_rdata.c */

/* Called through the IAT, as with windows.h. */
__declspec( dllimport ) int __stdcall MessageBoxA( void *hWnd, const char *text, const char *caption, unsigned type );

/* Owned by this unit.
 *
 * zlib 1.1.4 crc32.c's `local const uLongf crc_table[256]`, the precomputed
 * CRC-32 table (polynomial 0xEDB88320), which get_crc_table hands out and
 * crc32 indexes.  Retail 0x005413D0 in .rdata, 1024 bytes; IDA's [272] was
 * only the distance to the next label.  Every entry was read out of the
 * retail PE and matches zlib's make_crc_table() output.  It used to be a
 * zeroed `int crc_table[272]` here, with crc32 reading a private copy built
 * at run time instead. */
const unsigned long crc_table[256] = {   /* 0x005413D0 */
	0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL,
	0x076dc419UL, 0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL,
	0x0edb8832UL, 0x79dcb8a4UL, 0xe0d5e91eUL, 0x97d2d988UL,
	0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL, 0x90bf1d91UL,
	0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
	0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL,
	0x136c9856UL, 0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL,
	0x14015c4fUL, 0x63066cd9UL, 0xfa0f3d63UL, 0x8d080df5UL,
	0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL, 0xa2677172UL,
	0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
	0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL,
	0x32d86ce3UL, 0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL,
	0x26d930acUL, 0x51de003aUL, 0xc8d75180UL, 0xbfd06116UL,
	0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL, 0xb8bda50fUL,
	0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
	0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL,
	0x76dc4190UL, 0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL,
	0x71b18589UL, 0x06b6b51fUL, 0x9fbfe4a5UL, 0xe8b8d433UL,
	0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL, 0xe10e9818UL,
	0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
	0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL,
	0x6c0695edUL, 0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL,
	0x65b0d9c6UL, 0x12b7e950UL, 0x8bbeb8eaUL, 0xfcb9887cUL,
	0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL, 0xfbd44c65UL,
	0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
	0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL,
	0x4369e96aUL, 0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL,
	0x44042d73UL, 0x33031de5UL, 0xaa0a4c5fUL, 0xdd0d7cc9UL,
	0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL, 0xc90c2086UL,
	0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
	0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL,
	0x59b33d17UL, 0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL,
	0xedb88320UL, 0x9abfb3b6UL, 0x03b6e20cUL, 0x74b1d29aUL,
	0xead54739UL, 0x9dd277afUL, 0x04db2615UL, 0x73dc1683UL,
	0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
	0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL,
	0xf00f9344UL, 0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL,
	0xf762575dUL, 0x806567cbUL, 0x196c3671UL, 0x6e6b06e7UL,
	0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL, 0x67dd4accUL,
	0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
	0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL,
	0xd1bb67f1UL, 0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL,
	0xd80d2bdaUL, 0xaf0a1b4cUL, 0x36034af6UL, 0x41047a60UL,
	0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL, 0x4669be79UL,
	0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
	0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL,
	0xc5ba3bbeUL, 0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL,
	0xc2d7ffa7UL, 0xb5d0cf31UL, 0x2cd99e8bUL, 0x5bdeae1dUL,
	0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL, 0x026d930aUL,
	0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
	0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL,
	0x92d28e9bUL, 0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL,
	0x86d3d2d4UL, 0xf1d4e242UL, 0x68ddb3f8UL, 0x1fda836eUL,
	0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL, 0x18b74777UL,
	0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
	0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL,
	0xa00ae278UL, 0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL,
	0xa7672661UL, 0xd06016f7UL, 0x4969474dUL, 0x3e6e77dbUL,
	0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL, 0x37d83bf0UL,
	0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
	0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL,
	0xbad03605UL, 0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL,
	0xb3667a2eUL, 0xc4614ab8UL, 0x5d681b02UL, 0x2a6f2b94UL,
	0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL, 0x2d02ef8dUL
};

extern void MakeNormalVectors( const float *forward, float *right, float *up );
extern int SEH_GetLocalizedString_m( const char *key );
extern int SL_FindLowercaseString( const char *text );
extern void Sys_OutOfMemoryPrep( void );
extern int DObjFindPartIndex( int dobj, unsigned short nameHandle );   /* 0x00481230, defined in xanim/dobj.cpp */
struct fxPool_s;
struct fxPoolBlock_s;
extern struct fxPoolBlock_s *FxMem_ClaimBlock_m( int count, int elemSize, struct fxPoolBlock_s *prev );
extern int FxPool_MakeBlockCurrentLight( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentParticle( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentLine( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentElectricity( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentOrientedParticle( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentTail( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentCylinder( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentEmitter( struct fxPool_s *pool, struct fxPoolBlock_s *block );
extern int FxPool_MakeBlockCurrentDecal( struct fxPool_s *pool, struct fxPoolBlock_s *block );
struct CFxScheduler { int RegisterEffect( const char *file, bool isPath ); };   /* EffectsCore: __thiscall member */
extern void __stdcall CFxScheduler__PlayEffect_id_simple_m( int effectId, float *origin, void *self );
extern void __stdcall CFxScheduler__PlayEffect_name_bolt_m( char *name, void *self, float *origin, int *axis, int *bolt );
extern void __stdcall CFxScheduler__PlayEffect_id_axis(void *self, int id, float *origin, void *axis, void *bolt);
extern void __stdcall CFxScheduler__PlayEffect_name_simple_m( char *name, void *self, float *origin );
extern void __stdcall CFxScheduler__PlayEffect_name_m( int *direction, char *name, void *self, float *origin );
extern int CFxScheduler__AddScheduledEffects( void *self );
extern int SFxHelper__AdjustTime( void *self, int realTime );
extern void __stdcall SFxHelper__AdjustCamera( void *self, int view, float zfar );
extern int FX_Free( int freeAll );
extern int FX_Init();
extern int _tr_init( struct internal_state *s );
extern int _tr_stored_block( struct internal_state *s, int eof, char *buf, int stored_len );
extern int _tr_align( struct internal_state *s );
extern void __cdecl _tr_flush_block(struct internal_state *s, char *buf, unsigned int stored_len, int eof);
extern int zcalloc();
extern int zcfree();

inline struct fxPoolBlock_s *__cdecl FxPool_MakeBlockCurrent( struct fxPool_s *pool, struct fxPoolBlock_s *block );  /* see definition */

/* GenericParser2 classes (defined below as textPool_t, gpObject_t, gpValue_t, gpGroup_t, genericParser2_t) */
struct textPool_s;
struct gpObject_s;
struct gpValue_s;
struct gpGroup_s;
struct genericParser2_s;
char *__cdecl GP2_GetToken( char allowLineBreaks, char readUntilNewline, char **text );
struct textPool_s *__cdecl CTextPool__CTextPool( size_t initSize, struct textPool_s *this_ );
#ifndef __cplusplus
char *__stdcall CTextPool__AllocText( struct textPool_s *this_, const char *text, int addNULL, struct textPool_s **poolPtr );
#endif
void *__cdecl CTextPool__operator_new( size_t size );
struct textPool_s *__cdecl CTextPool__GetNext( struct textPool_s *this_ );
struct textPool_s *__cdecl CTextPool__SetNext( struct textPool_s *this_, struct textPool_s *which );
char __stdcall CGPObject__WriteText( const char *text, struct textPool_s **textPool, struct gpObject_s *this_ );
__declspec(noinline) void __cdecl CGPValue__AddValue( struct textPool_s **textPool, const char *newValue, struct gpValue_s *this_ );
char __stdcall CGPValue__Parse( struct textPool_s **textPool, struct gpValue_s *this_, char **dataPtr );
char __stdcall CGPValue__Write( struct gpValue_s *this_, struct textPool_s **textPool, int depth );
/* __stdcall/__fastcall below: retail's C++ methods keep thiscall's callee-pop
 * (and Clean its ecx `this`) under LTCG. */
void __fastcall CGPGroup__Clean( struct gpGroup_s *this_ );
typedef struct { void *value; } GpPointerArgument;
struct gpGroup_s *__fastcall CGPGroup__Duplicate(struct gpGroup_s *receiver, GpPointerArgument poolReference, GpPointerArgument parentReference);
/* thiscall method: the receiver in ecx, the pool pointer on the stack */
struct gpValue_s *__fastcall CGPValue__Duplicate( struct gpValue_s *receiver, GpPointerArgument poolReference );
#ifdef __cplusplus
/* retail drops the EH state store before every SortObject call: it cannot throw */
#define GP_NOTHROW throw()
#else
#define GP_NOTHROW
#endif
void __stdcall CGP_InsertInOrder( struct gpGroup_s *this_, struct gpObject_s *object, struct gpObject_s **unsortedList,
                                  struct gpObject_s **sortedList, struct gpObject_s **lastObject ) GP_NOTHROW;
struct gpValue_s *__stdcall CGPGroup__AddPair( const char *value, struct textPool_s **textPool, struct gpGroup_s *this_, const char *name );
struct gpGroup_s *__stdcall CGPGroup__AddGroup( const char *name, struct textPool_s **textPool, struct gpGroup_s *this_ );
char __fastcall CGPGroup__Parse(struct gpGroup_s *receiver, GpPointerArgument dataReference, GpPointerArgument poolReference);
void __cdecl CGenericParser2__CleanTextPool( struct genericParser2_s *this_ );

int __cdecl adler32( unsigned int adler, unsigned char *buf, unsigned int len );
int __cdecl crc32( int crc, const unsigned char *buf, unsigned int len );
unsigned int __cdecl compress2( int sourceLen, int dest, int destLen, int *source, unsigned int level );
int __cdecl deflateInit2_( struct z_stream_s *strm, int level, int method, int windowBits, int memLevel,
                           int strategy, const char *version, int stream_size );
int __cdecl deflateReset( struct z_stream_s *strm );

/* ---- zlib 1.1.4 zlib.h / deflate.h (the deflate.c part of this unit) ---- */
typedef unsigned char   Byte;
typedef unsigned int    uInt;
typedef unsigned long   uLong;
typedef unsigned char   uch;
typedef unsigned short  ush;
typedef unsigned long   ulg;
typedef ush             Pos;
typedef unsigned        IPos;

typedef struct z_stream_s {
	Byte   *next_in;    /* +0x00 */
	uInt    avail_in;   /* +0x04 */
	uLong   total_in;   /* +0x08 */
	Byte   *next_out;   /* +0x0C */
	uInt    avail_out;  /* +0x10 */
	uLong   total_out;  /* +0x14 */
	char   *msg;        /* +0x18 */
	struct internal_state *state;   /* +0x1C */
	void *( __cdecl *zalloc )( void *opaque, uInt items, uInt size );   /* +0x20 */
	void  ( __cdecl *zfree )( void *opaque, void *address );            /* +0x24 */
	void   *opaque;     /* +0x28 */
	int     data_type;  /* +0x2C */
	uLong   adler;      /* +0x30 */
	uLong   reserved;   /* +0x34 */
} z_stream;
typedef z_stream *z_streamp;
typedef void *( __cdecl *alloc_func )( void *opaque, uInt items, uInt size );
typedef void  ( __cdecl *free_func )( void *opaque, void *address );

typedef struct ct_data_s {
	union { ush freq; ush code; } fc;
	union { ush dad;  ush len;  } dl;
} ct_data;

typedef struct tree_desc_s {
	ct_data *dyn_tree;
	int      max_code;
	void    *stat_desc;
} tree_desc;

#define Z_PARTIAL_FLUSH 1
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_STREAM_ERROR  (-2)
#define Z_DATA_ERROR    (-3)
#define Z_MEM_ERROR     (-4)
#define Z_BUF_ERROR     (-5)
#define Z_FILTERED      1
#define Z_HUFFMAN_ONLY  2
#define Z_DEFLATED      8
#define Z_DEFAULT_COMPRESSION (-1)
#define Z_NO_FLUSH      0

#define LENGTH_CODES 29
#define LITERALS     256
#define L_CODES      ( LITERALS + 1 + LENGTH_CODES )
#define D_CODES      30
#define BL_CODES     19
#define HEAP_SIZE    ( 2 * L_CODES + 1 )
#define MAX_BITS     15
#define INIT_STATE   42
#define BUSY_STATE   113
#define FINISH_STATE 666
#define MIN_MATCH    3
#define MAX_MATCH    258
#define MIN_LOOKAHEAD ( MAX_MATCH + MIN_MATCH + 1 )
#define MAX_DIST( s ) ( ( s )->w_size - MIN_LOOKAHEAD )
#define NIL          0
#define TOO_FAR      4096
#define PRESET_DICT  0x20

typedef struct internal_state {
	z_streamp strm;          /* +0x0000 */
	int       status;        /* +0x0004 */
	Byte     *pending_buf;   /* +0x0008 */
	ulg       pending_buf_size; /* +0x000C */
	Byte     *pending_out;   /* +0x0010 */
	int       pending;       /* +0x0014 */
	int       noheader;      /* +0x0018 */
	Byte      data_type;     /* +0x001C */
	Byte      method;        /* +0x001D */
	int       last_flush;    /* +0x0020 */
	uInt      w_size;        /* +0x0024 */
	uInt      w_bits;        /* +0x0028 */
	uInt      w_mask;        /* +0x002C */
	Byte     *window;        /* +0x0030 */
	ulg       window_size;   /* +0x0034 */
	Pos      *prev;          /* +0x0038 */
	Pos      *head;          /* +0x003C */
	uInt      ins_h;         /* +0x0040 */
	uInt      hash_size;     /* +0x0044 */
	uInt      hash_bits;     /* +0x0048 */
	uInt      hash_mask;     /* +0x004C */
	uInt      hash_shift;    /* +0x0050 */
	long      block_start;   /* +0x0054 */
	uInt      match_length;  /* +0x0058 */
	IPos      prev_match;    /* +0x005C */
	int       match_available; /* +0x0060 */
	uInt      strstart;      /* +0x0064 */
	uInt      match_start;   /* +0x0068 */
	uInt      lookahead;     /* +0x006C */
	uInt      prev_length;   /* +0x0070 */
	uInt      max_chain_length; /* +0x0074 */
	uInt      max_lazy_match;   /* +0x0078 */
	int       level;         /* +0x007C */
	int       strategy;      /* +0x0080 */
	uInt      good_match;    /* +0x0084 */
	int       nice_match;    /* +0x0088 */
	ct_data   dyn_ltree[HEAP_SIZE];     /* +0x008C */
	ct_data   dyn_dtree[2 * D_CODES + 1];  /* +0x0980 */
	ct_data   bl_tree[2 * BL_CODES + 1];   /* +0x0A74 */
	tree_desc l_desc;        /* +0x0B10 */
	tree_desc d_desc;        /* +0x0B1C */
	tree_desc bl_desc;       /* +0x0B28 */
	ush       bl_count[MAX_BITS + 1];   /* +0x0B34 */
	int       heap[2 * L_CODES + 1];    /* +0x0B54 */
	int       heap_len;      /* +0x1448 */
	int       heap_max;      /* +0x144C */
	uch       depth[2 * L_CODES + 1];   /* +0x1450 */
	uch      *l_buf;         /* +0x1690 */
	uInt      lit_bufsize;   /* +0x1694 */
	uInt      last_lit;      /* +0x1698 */
	ush      *d_buf;         /* +0x169C */
	ulg       opt_len;       /* +0x16A0 */
	ulg       static_len;    /* +0x16A4 */
	uInt      matches;       /* +0x16A8 */
	int       last_eob_len;  /* +0x16AC */
	ush       bi_buf;        /* +0x16B0 */
	int       bi_valid;      /* +0x16B4 */
} deflate_state;
typedef char deflate_state_size_check[ sizeof( deflate_state ) == 5816 ? 1 : -1 ];

typedef enum { need_more, block_done, finish_started, finish_done } block_state;

int __cdecl deflateParams( z_streamp strm, int level, int strategy );
void __cdecl putShortMSB( deflate_state *s, uInt b );
void __cdecl flush_pending( z_streamp strm );
int __cdecl deflate( z_streamp strm, int flush );
int __cdecl deflateEnd( z_streamp strm );
unsigned int __cdecl deflate_read_buf( z_streamp strm, Byte *buf, unsigned int size );
void __cdecl lm_init( deflate_state *s );
uInt __cdecl longest_match( deflate_state *s, IPos cur_match );
void __cdecl deflate_fill_window( deflate_state *s );
int __cdecl deflate_stored( deflate_state *s, int flush );
int __cdecl deflate_fast( deflate_state *s, int flush );
int __cdecl deflate_slow( deflate_state *s, int flush );

#define GP2_MAX_TOKEN_SIZE  1024
static char gp2_token[ GP2_MAX_TOKEN_SIZE + 4 ];

typedef struct {
	unsigned short good_length;
	unsigned short max_lazy;
	unsigned short nice_length;
	unsigned short max_chain;
	int ( __cdecl *func )( deflate_state *s, int flush );
} deflate_config_t;

static const deflate_config_t deflate_configuration_table[10] = {
	{  0,   0,   0,    0, deflate_stored },  /* store only */
	{  4,   4,   8,    4, deflate_fast   },  /* maximum speed, no lazy matches */
	{  4,   5,  16,    8, deflate_fast   },
	{  4,   6,  32,   32, deflate_fast   },
	{  4,   4,  16,   16, deflate_slow   },  /* lazy matches */
	{  8,  16,  32,   32, deflate_slow   },
	{  8,  16, 128,  128, deflate_slow   },
	{  8,  32, 128,  256, deflate_slow   },
	{ 32, 128, 258, 1024, deflate_slow   },
	{ 32, 258, 258, 4096, deflate_slow   }   /* maximum compression */
};

/* zlib 1.1.4 deflate.c macros (no DEBUG: the inline _tr_tally forms) */
#define ERR_MSG( err ) ( ( (char **)&z_errmsg )[Z_NEED_DICT - ( err )] )
#define ERR_RETURN( strm, err ) return ( ( strm )->msg = ERR_MSG( err ), ( err ) )
#define ZALLOC( strm, items, size ) ( *( ( strm )->zalloc ) )( ( strm )->opaque, ( items ), ( size ) )
#define ZFREE( strm, addr ) ( *( ( strm )->zfree ) )( ( strm )->opaque, (void *)( addr ) )
#define TRY_FREE( s, p ) { if ( p ) ZFREE( s, p ); }
#define UPDATE_HASH( s, h, c ) ( h = ( ( ( h ) << s->hash_shift ) ^ ( c ) ) & s->hash_mask )
#define INSERT_STRING( s, str, match_head ) \
	( UPDATE_HASH( s, s->ins_h, s->window[( str ) + ( MIN_MATCH - 1 )] ), \
	  s->prev[( str ) & s->w_mask] = match_head = s->head[s->ins_h], \
	  s->head[s->ins_h] = (Pos)( str ) )
#define CLEAR_HASH( s ) \
	s->head[s->hash_size - 1] = NIL; \
	memset( (Byte *)s->head, 0, (unsigned)( s->hash_size - 1 ) * sizeof( *s->head ) );
#define d_code( dist ) \
	( ( dist ) < 256 ? (int)_dist_code[dist] : (int)_dist_code[256 + ( ( dist ) >> 7 )] )
#define _tr_tally_lit( s, c, flush ) \
	{ uch cc = ( c ); \
	  s->d_buf[s->last_lit] = 0; \
	  s->l_buf[s->last_lit++] = cc; \
	  s->dyn_ltree[cc].fc.freq++; \
	  flush = ( s->last_lit == s->lit_bufsize - 1 ); \
	}
#define _tr_tally_dist( s, distance, length, flush ) \
	{ uch len = ( length ); \
	  ush dist = ( distance ); \
	  s->d_buf[s->last_lit] = dist; \
	  s->l_buf[s->last_lit++] = len; \
	  dist--; \
	  s->dyn_ltree[_length_code[len] + LITERALS + 1].fc.freq++; \
	  s->dyn_dtree[d_code( dist )].fc.freq++; \
	  flush = ( s->last_lit == s->lit_bufsize - 1 ); \
	}
#define FLUSH_BLOCK_ONLY( s, eof ) { \
	_tr_flush_block( s, ( s->block_start >= 0L ? \
	                 (char *)&s->window[(unsigned)s->block_start] : \
	                 (char *)NULL ), \
	                 (ulg)( (long)s->strstart - s->block_start ), \
	                 ( eof ) ); \
	s->block_start = s->strstart; \
	flush_pending( s->strm ); \
}
#define FLUSH_BLOCK( s, eof ) { \
	FLUSH_BLOCK_ONLY( s, eof ); \
	if ( s->strm->avail_out == 0 ) return ( eof ) ? finish_started : need_more; \
}

/* ------------------------------------------------------------------------ *
 *  File-local typed views for the data this unit manipulates.
 *
 *  The field offsets below are taken from the decompiled bodies (they are
 *  internally consistent across every function in this file) but were NOT
 *  re-verified against IDA -- the MCP server was unavailable for this pass.
 *  The COD1_STATIC_ASSERTs turn any layout mistake into a build error.
 * ------------------------------------------------------------------------ */
#define MT_ASSERT_OFF(type, field, off) \
	COD1_STATIC_ASSERT(offsetof(type, field) == (off), \
	                   #type "." #field " must be at " #off)

/* FX primitive pool.  Objects are carved from 32 KiB, 32 KiB-aligned slabs;
 * the per-slab bookkeeping lives in the last 16 bytes of each slab.  A slab's
 * free list is intrusive: each free object's first dword points at the next. */
#define FXPOOL_SLAB_SIZE  0x8000

typedef struct fxPoolBlock_s {
	byte                  storage[FXPOOL_SLAB_SIZE - 16];
	int                   freeCount;   /* +0x7FF0 (idx 8188) */
	void                 *freeList;    /* +0x7FF4 (idx 8189) */
	struct fxPoolBlock_s *next;        /* +0x7FF8 (idx 8190) */
	struct fxPoolBlock_s *prev;        /* +0x7FFC (idx 8191) */
} fxPoolBlock_t;
COD1_ASSERT_SIZE( fxPoolBlock_t, FXPOOL_SLAB_SIZE );
MT_ASSERT_OFF( fxPoolBlock_t, freeCount, 32752 );
MT_ASSERT_OFF( fxPoolBlock_t, freeList,  32756 );
MT_ASSERT_OFF( fxPoolBlock_t, next,      32760 );
MT_ASSERT_OFF( fxPoolBlock_t, prev,      32764 );

typedef struct fxPool_s {
	int            blockCapacity;   /* +0x00 objects a full slab holds */
	fxPoolBlock_t *currentBlock;    /* +0x04 slab currently handing out objects */
} fxPool_t;
MT_ASSERT_OFF( fxPool_t, blockCapacity, 0 );
MT_ASSERT_OFF( fxPool_t, currentBlock,  4 );

/* GenericParser2 object model (Ravensoft GP2).  CGPObject is the common base;
 * CGPValue and CGPGroup extend it. */
typedef struct textPool_s {
	char               *base;   /* +0x00 allocation base */
	struct textPool_s  *next;   /* +0x04 */
	int                 size;   /* +0x08 capacity in bytes */
	int                 used;   /* +0x0C */
} textPool_t;
MT_ASSERT_OFF( textPool_t, base, 0 );
MT_ASSERT_OFF( textPool_t, next, 4 );
MT_ASSERT_OFF( textPool_t, size, 8 );
MT_ASSERT_OFF( textPool_t, used, 12 );
COD1_ASSERT_SIZE( textPool_t, 16 );

#ifdef __cplusplus
/*
 * Raven's CTextPool (GP2's string arena) as a C++ class over textPool_t.  Its
 * operator new is CTextPool__operator_new (retail 0x004A1900, inlined); its
 * operator delete (retail 0x004A1910) only serves the unwind funclets of the
 * `new CTextPool` expressions.
 */
struct CTextPool : textPool_t
{
  CTextPool( int initSize = 10240 ) { CTextPool__CTextPool( initSize, this ); }
  static void *operator new( size_t size ) { return CTextPool__operator_new( size ); }
  static void operator delete( void *block );
  char *AllocText( const char *text, bool addNULL, CTextPool **poolPtr );
};
/* AllocText on the current pool *textPool, chaining a new pool through textPool when it is full. */
#define GP2_AllocText( textPool, text, addNULL ) \
  ( ( (CTextPool *)*(textPool) )->AllocText( (text), (addNULL) != 0, (CTextPool **)(textPool) ) )
#else
#define GP2_AllocText( textPool, text, addNULL ) CTextPool__AllocText( *(textPool), (text), (addNULL), (textPool) )
#endif

typedef struct gpObject_s {
	const char        *name;             /* +0x00 */
	struct gpObject_s *next;             /* +0x04 sibling list                        */
	struct gpObject_s *inOrderNext;      /* +0x08 (reused as list tail on a list head) */
	struct gpObject_s *inOrderPrevious;  /* +0x0C */
} gpObject_t;
MT_ASSERT_OFF( gpObject_t, name, 0 );
MT_ASSERT_OFF( gpObject_t, next, 4 );
MT_ASSERT_OFF( gpObject_t, inOrderNext, 8 );
MT_ASSERT_OFF( gpObject_t, inOrderPrevious, 12 );
COD1_ASSERT_SIZE( gpObject_t, 16 );

typedef struct gpValue_s {
	gpObject_t  base;   /* +0x00 */
	gpObject_t *list;   /* +0x10 value list (a single value or a [ ... ] list) */
} gpValue_t;
MT_ASSERT_OFF( gpValue_t, list, 16 );
COD1_ASSERT_SIZE( gpValue_t, 20 );

#ifdef __cplusplus
/* Raven's CGPObject allocation: operator new is Z_Malloc (retail 0x004A1960,
 * CGPObject__operator_new, inlined); operator delete (retail 0x004A1970) only
 * serves the unwind funclets of `new CGPValue`. */
struct CGPObject : gpObject_t
{
  static void *operator new( size_t size ) { return Z_MallocInternal( size ); }
  static void operator delete( void *block );
};
#endif

typedef struct gpGroup_s {
	gpObject_t         base;              /* +0x00 */
	gpValue_t         *pairs;             /* +0x10 */
	gpValue_t         *pairsInOrder;      /* +0x14 */
	gpValue_t         *currentPair;       /* +0x18 */
	struct gpGroup_s  *subGroups;         /* +0x1C */
	struct gpGroup_s  *subGroupsInOrder;  /* +0x20 */
	struct gpGroup_s  *currentSubGroup;   /* +0x24 */
	struct gpGroup_s  *parent;            /* +0x28 */
	char               writeable;         /* +0x2C */
} gpGroup_t;
MT_ASSERT_OFF( gpGroup_t, pairs, 16 );
MT_ASSERT_OFF( gpGroup_t, subGroups, 28 );
MT_ASSERT_OFF( gpGroup_t, parent, 40 );
MT_ASSERT_OFF( gpGroup_t, writeable, 44 );
COD1_ASSERT_SIZE( gpGroup_t, 48 );

typedef struct genericParser2_s {
	gpGroup_t    topLevel;    /* +0x00 */
	textPool_t  *textPool;    /* +0x30 */
	char         writeable;   /* +0x34 */
} genericParser2_t;
MT_ASSERT_OFF( genericParser2_t, textPool, 48 );
MT_ASSERT_OFF( genericParser2_t, writeable, 52 );

#ifdef __cplusplus
/* Raven's CGPGroup as a C++ class over gpGroup_t: its destructor
 * (retail 0x004A2230) is Clean(). */
struct CGPGroup : gpGroup_t
{
  ~CGPGroup();
  char Write( textPool_t **textPool, int depth );
};
/* Raven's CGenericParser2 over genericParser2_t: the top-level group is a
 * CGPGroup member, so its destructor runs after the body's CleanTextPool,
 * whose unwind action destroys it too. */
struct CGenericParser2
{
  CGPGroup    mTopLevel;    /* +0x00 */
  textPool_t *mTextPool;    /* +0x30 */
  char        mWriteable;   /* +0x34 */
  ~CGenericParser2();
};
#endif

/* ---- FxPool_AllocPrimType14  0x004A10E0 ---- VERIFIED */
/* The original pool template's Alloc: scan the slab list for a free object,
 * moving that slab to the front, else claim a new slab. */
void *__stdcall FxPool_AllocPrimType14(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrent( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 560 );
  return obj;
}

/* ---- FxMem_ReleaseBlock  0x0048D370 ---- HIGH
 * Unnamed in the retail IDB (between FxMem_UnlinkBlock and FxMem_FreeElem);
 * every FxPool_Free* tail-jumps to it: unlink the slab, mark it unclaimed. */
fxPoolBlock_t *__cdecl FxMem_ReleaseBlock( fxPoolBlock_t *block )
{
  if ( block->prev )
    block->prev->next = block->next;
  if ( block->next )
    block->next->prev = block->prev;
  block->prev = NULL;
  block->next = NULL;
  block->freeCount = -1;
  return block;
}

/* ---- FxPool_FreePrimType14  0x004A1170 ---- VERIFIED */
fxPoolBlock_t *__fastcall FxPool_FreePrimType14(fxPool_t *pool, char *elem)
{
  fxPoolBlock_t *block;
  fxPoolBlock_t *next;

  block = (fxPoolBlock_t *)( (char *)&fxMemBlocks
                             + ( ( elem - (char *)&fxMemBlocks ) & 0xFFFF8000 ) );
  *(void **)elem = block->freeList;   /* push the freed object onto the slab free list */
  block->freeList = elem;
  block->freeCount++;

  if ( !pool->currentBlock->freeList )
    return FxPool_MakeBlockCurrent( pool, block );

  if ( block->freeCount >= pool->blockCapacity )
  {
    if ( block == pool->currentBlock )
    {
      next = block->next;
      if ( !next || !next->freeCount )
        return block;
      pool->currentBlock = next;
    }
    return FxMem_ReleaseBlock( block );
  }
  return block;
}

/* ---- FxPool_AllocLight  0x004A11E0 ---- HIGH */
void *__stdcall FxPool_AllocLight(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentLight( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 260 );
  return obj;
}

/* ---- FxPool_AllocParticle  0x004A1270 ---- HIGH */
void *__stdcall FxPool_AllocParticle(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentParticle( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 316 );
  return obj;
}

/* ---- FxPool_AllocLine  0x004A1300 ---- HIGH */
void *__stdcall FxPool_AllocLine(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentLine( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 328 );
  return obj;
}

/* ---- FxPool_AllocElectricity  0x004A1390 ---- HIGH */
void *__stdcall FxPool_AllocElectricity(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentElectricity( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 716 );
  return obj;
}

/* ---- FxPool_AllocOrientedParticle  0x004A1420 ---- HIGH */
void *__stdcall FxPool_AllocOrientedParticle(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentOrientedParticle( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 328 );
  return obj;
}

/* ---- FxPool_AllocTail  0x004A14B0 ---- HIGH */
void *__stdcall FxPool_AllocTail(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentTail( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 344 );
  return obj;
}

/* ---- FxPool_AllocCylinder  0x004A1540 ---- HIGH */
void *__stdcall FxPool_AllocCylinder(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentCylinder( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 344 );
  return obj;
}

/* ---- FxPool_AllocEmitter  0x004A15D0 ---- HIGH */
void *__stdcall FxPool_AllocEmitter(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentEmitter( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 396 );
  return obj;
}

/* ---- FxPool_AllocFlash  0x004A1660 ---- HIGH */
void *__stdcall FxPool_AllocFlash(fxPool_t *pool, int elemSize)
{
  fxPoolBlock_t *block;
  void          *obj;

  /* same pool-template body as FxPool_AllocPrimType14; matches once the
   * EffectsCore FxPool_MakeBlockCurrent* helpers are inline (pool, block)
   * like FxPool_MakeBlockCurrent (verified on FxPool_AllocLight) */
  for ( block = pool->currentBlock; block; block = block->next )
  {
    if ( block->freeList )
    {
      if ( block != pool->currentBlock )
        FxPool_MakeBlockCurrentDecal( pool, block );
      goto pop;
    }
  }
  block = FxMem_ClaimBlock_m( pool->blockCapacity, elemSize, pool->currentBlock );
  if ( !block )
    return NULL;
  pool->currentBlock = block;
pop:
  obj = block->freeList;
  block->freeList = *(void **)obj;
  block->freeCount--;
  Com_Memset( obj, 0, 436 );
  return obj;
}

/* ---- FxPool_MakeBlockCurrent  0x004A16F0 ---- VERIFIED */
/* A member of the original pool template (`this` = pool first, implicitly
 * inline).  As an inline (COMDAT) function LTCG compiles it after its callers,
 * so FxPool_AllocPrimType14 does not see its register usage and keeps
 * block/pool in esi/edi across the call like retail. */
inline fxPoolBlock_t *__cdecl FxPool_MakeBlockCurrent(fxPool_t *pool, fxPoolBlock_t *block)
{
  /* unlink the slab from whatever list it is on ... */
  if ( block->prev )
    block->prev->next = block->next;
  if ( block->next )
    block->next->prev = block->prev;
  block->next = NULL;
  block->prev = NULL;

  /* ... then push it to the front of the pool's current list. */
  block->next = pool->currentBlock;
  pool->currentBlock->prev = block;
  pool->currentBlock = block;
  return block;
}

/* ---- FX_RegisterEffect  0x004A1750 ---- VERIFIED */
int __cdecl FX_RegisterEffect(char *name)
{
  return ( (CFxScheduler *)theFxScheduler )->RegisterEffect( name, false );
}

/* ---- FX_GetBoneIndex  0x004A1760 ---- VERIFIED */
int __cdecl FX_GetBoneIndex(int entnum, const char *boneName)
{
  short handle;
  int   dobj;
  short nameHandle;

  /* Com_GetClientDObj inlined: retail jumps from the zero-handle test straight
   * into the !dobj return.  retail 0x004A1774 `add esi, offset dobj_pool`:
   * dobj_pool IS com_dobjPool, the table common.c defines. */
  handle = (short)com_clientDObjHandles[entnum];
  if ( handle )
    dobj = (int)&com_dobjPool[handle * 0x58];
  else
    dobj = 0;
  if ( !dobj )
    return -1;

  nameHandle = (short)SL_FindLowercaseString(boneName);
  if ( nameHandle )
    return DObjFindPartIndex( dobj, nameHandle );
  return -1;
}

/* ---- FX_PlaySimpleEffect  0x004A17B0 ---- HIGH */
void __cdecl FX_PlaySimpleEffect(float *origin, char *name)
{
  CFxScheduler__PlayEffect_name_simple_m(name, theFxScheduler, origin);
}

/* ---- FX_PlayEffect  0x004A17C0 ---- HIGH */
void __cdecl FX_PlayEffect(float *origin, char *name, int *direction)
{
  CFxScheduler__PlayEffect_name_m(direction, name, theFxScheduler, origin);
}

/* ---- FX_PlayEntityEffect  0x004A17D0 ---- HIGH */
void __cdecl FX_PlayEntityEffect(int *bolt, float *origin, int *axis, char *name)
{
  CFxScheduler__PlayEffect_name_bolt_m(name, theFxScheduler, origin, axis, bolt);
}

/* ---- FX_PlaySimpleEffectID  0x004A17F0 ---- HIGH */
void __fastcall FX_PlaySimpleEffectID(float *origin, int effectId)
{
  CFxScheduler__PlayEffect_id_simple_m(effectId, origin, theFxScheduler);
}

/* ---- FX_PlayEffectID  0x004A1800 ---- VERIFIED */
void __cdecl FX_PlayEffectID(int *forward, int effectId, float *origin)
{
  /* retail var_24/var_20/var_1C + right + up occupy ONE contiguous 36-byte
   * slot, frame 0x08..0x2C, and 0x004A1832 hands that base to
   * PlayEffect_id_axis as axis[3][3].  Keeping the three forward rows in one
   * array reproduces that contiguous stack slot. */
  int axis[9];   /* forward, right, up: retail var_24, one axis[3][3] slot at ebp-24h */

  axis[0] = forward[0];
  axis[1] = forward[1];
  axis[2] = forward[2];
  MakeNormalVectors((const float *)forward, (float *)&axis[3], (float *)&axis[6]);
  CFxScheduler__PlayEffect_id_axis(theFxScheduler, effectId, origin, axis, 0);
}

/* ---- FX_PlayEntityEffectID  0x004A1850 ---- HIGH */
void __cdecl FX_PlayEntityEffectID(int *bolt, float *origin, int *axis, int effectId)
{
  CFxScheduler__PlayEffect_id_axis(theFxScheduler, effectId, origin, axis, bolt);
}

/* ---- FX_AddScheduledEffects  0x004A1870 ---- HIGH */
void FX_AddScheduledEffects()
{
  CFxScheduler__AddScheduledEffects(theFxScheduler);
}

/* ---- FX_InitSystem  0x004A1880 ---- HIGH */
int __fastcall FX_InitSystem()
{
  return FX_Init();
}

/* ---- FX_FreeSystem  0x004A1890 ---- HIGH */
int FX_FreeSystem()
{
  return (unsigned char)FX_Free(1);
}

/* ---- FX_FreeActive  0x004A18A0 ---- HIGH */
int FX_FreeActive()
{
  return (unsigned char)FX_Free(0);
}

/* ---- FX_AdjustCamera  0x004A18B0 ---- HIGH */
void __cdecl FX_AdjustCamera(int view, float zfar)
{
  SFxHelper__AdjustCamera(&theFxHelper, view, zfar);
}

/* ---- FX_AdjustTime  0x004A18D0 ---- HIGH */
int __cdecl FX_AdjustTime(int realTime)
{
  return SFxHelper__AdjustTime(&theFxHelper, realTime);
}

/* ---- CTextPool__GetNext  0x004A18E0 ---- VERIFIED */
textPool_t *__cdecl CTextPool__GetNext(textPool_t *this_)
{
  return this_->next;
}

/* ---- CTextPool__SetNext  0x004A18F0 ---- HIGH */
textPool_t *__cdecl CTextPool__SetNext(textPool_t *this_, textPool_t *which)
{
  this_->next = which;
  return this_;
}

/* ---- CTextPool__operator_new  0x004A1900 ---- HIGH */
void *__cdecl CTextPool__operator_new(size_t size)
{
  return Z_MallocInternal(size);
}

#ifdef __cplusplus
/* ---- CTextPool::operator delete  0x004A1910 ---- */
void CTextPool::operator delete( void *block )
{
  free( block );
}
#endif

/* ---- CGPObject__SetNext  0x004A1920 ---- HIGH */
gpObject_t *__cdecl CGPObject__SetNext(gpObject_t *this_, gpObject_t *which)
{
  this_->next = which;
  return this_;
}

/* ---- CGPObject__GetInOrderNext  0x004A1930 ---- HIGH */
gpObject_t *__cdecl CGPObject__GetInOrderNext(gpObject_t *this_)
{
  return this_->inOrderNext;
}

/* ---- CGPObject__SetInOrderNext  0x004A1940 ---- HIGH */
gpObject_t *__cdecl CGPObject__SetInOrderNext(gpObject_t *this_, gpObject_t *which)
{
  this_->inOrderNext = which;
  return this_;
}

/* ---- CGPObject__SetInOrderPrevious  0x004A1950 ---- HIGH */
gpObject_t *__cdecl CGPObject__SetInOrderPrevious(gpObject_t *this_, gpObject_t *which)
{
  this_->inOrderPrevious = which;
  return this_;
}

/* ---- CGPObject__operator_new  0x004A1960 ---- HIGH */
void *__cdecl CGPObject__operator_new(size_t size)
{
  return Z_MallocInternal(size);
}

#ifdef __cplusplus
/* ---- CGPObject::operator delete  0x004A1970 ---- */
void CGPObject::operator delete( void *block )
{
  free( block );
}
#endif

/* ---- CGPGroup__SetWriteable  0x004A1980 ---- HIGH */
gpGroup_t *__cdecl CGPGroup__SetWriteable(gpGroup_t *this_, char writeable)
{
  this_->writeable = writeable;
  return this_;
}

/* ---- CGenericParser2__SetWriteable  0x004A1990 ---- HIGH */
genericParser2_t *__cdecl CGenericParser2__SetWriteable(genericParser2_t *this_, char writeable)
{
  this_->writeable = writeable;
  return this_;
}

/* ---- GP2_GetToken  0x004A19A0 ---- CONFIRMED */
/* GP2 GetToken (Raven genericparser2.cpp) with SkipWhiteSpace written inline;
 * CoD adds the quote stripping before the terminator. */
char *__cdecl GP2_GetToken( char allowLineBreaks, char readUntilNewline, char **text )
{
  char *pointer = *text;
  int   length = 0;
  char  c = 0;
  char  foundLineBreak;

  gp2_token[0] = 0;
  if ( !pointer )
  {
    return gp2_token;
  }

  while ( 1 )
  {
    /* SkipWhiteSpace */
    foundLineBreak = 0;
    while ( 1 )
    {
      c = *pointer;
      if ( c > ' ' )
      {
        break;
      }
      if ( !c )
      {
        *text = 0;
        return gp2_token;
      }
      if ( c == '\n' )
      {
        foundLineBreak = 1;
      }
      pointer++;
    }
    if ( foundLineBreak && !allowLineBreaks )
    {
      *text = pointer;
      return gp2_token;
    }

    c = *pointer;
    if ( c == '/' && pointer[1] == '/' )
    {  /* skip // comments */
      pointer += 2;
      while ( *pointer && *pointer != '\n' )
      {
        pointer++;
      }
    }
    else if ( c == '/' && pointer[1] == '*' )
    {  /* skip slash-star comments */
      pointer += 2;
      while ( *pointer && ( *pointer != '*' || pointer[1] != '/' ) )
      {
        pointer++;
      }
      if ( *pointer )
      {
        pointer += 2;
      }
    }
    else
    {  /* found the start of a token */
      break;
    }
  }

  if ( c == '\"' && !readUntilNewline )
  {  /* handle a string */
    pointer++;
    while ( 1 )
    {
      c = *pointer++;
      if ( c == '\"' )
      {
        break;
      }
      else if ( !c )
      {
        break;
      }
      else if ( length < GP2_MAX_TOKEN_SIZE )
      {
        gp2_token[length++] = c;
      }
    }
  }
  else if ( readUntilNewline )
  {
    /* absorb all characters until EOL */
    while ( c != '\n' && c != '\r' )
    {
      if ( c == '/' && ( ( *( pointer + 1 ) ) == '/' || ( *( pointer + 1 ) ) == '*' ) )
      {
        break;
      }

      if ( length < GP2_MAX_TOKEN_SIZE )
      {
        gp2_token[length++] = c;
      }
      pointer++;
      c = *pointer;
    }
    /* remove trailing white space */
    while ( length && gp2_token[length - 1] < ' ' )
    {
      length--;
    }
  }
  else
  {
    while ( c > ' ' )
    {
      if ( length < GP2_MAX_TOKEN_SIZE )
      {
        gp2_token[length++] = c;
      }
      pointer++;
      c = *pointer;
    }
  }

  if ( gp2_token[0] == '\"' )
  {  /* strip the quotes of a quoted value read to end of line */
    length--;
    memmove( gp2_token, gp2_token + 1, length );
    if ( length && gp2_token[length - 1] == '\"' )
    {
      length--;
    }
  }
  if ( length >= GP2_MAX_TOKEN_SIZE )
  {
    length = 0;
  }
  gp2_token[length] = 0;
  *text = pointer;

  return gp2_token;
}

/* ---- CTextPool__CTextPool  0x004A1B40 ---- HIGH */
textPool_t *__cdecl CTextPool__CTextPool(size_t initSize, textPool_t *this_)
{
  textPool_t *pool = this_;

  pool->next = NULL;
  pool->size = (int)initSize;
  pool->used = 0;
  pool->base = (char *)Z_MallocInternal( initSize );
  return this_;
}

/* ---- CTextPool__dtor  0x004A1B60 ---- HIGH */
void __cdecl CTextPool__dtor(textPool_t *this_)
{
  free( this_->base );
}

/* ---- CTextPool::AllocText  0x004A1B70 ---- CONFIRMED */
#ifdef __cplusplus
/*
 * Copies text (and, with addNULL, one more NUL) into this pool.  When it does
 * not fit and poolPtr is given, a new pool of the same size is chained after
 * *poolPtr, becomes *poolPtr and takes the text; without poolPtr it fails (NULL).
 */
char *CTextPool::AllocText( const char *text, bool addNULL, CTextPool **poolPtr )
{
  int length = (int)strlen( text ) + ( addNULL ? 1 : 0 );

  if ( used + length + 1 > size )   /* the extra 1 puts a NUL on the end */
  {
    if ( poolPtr )
    {
      CTextPool__SetNext( *poolPtr, new CTextPool( size ) );
      *poolPtr = (CTextPool *)CTextPool__GetNext( *poolPtr );
      return ( *poolPtr )->AllocText( text, addNULL, NULL );
    }
    return NULL;
  }

  strcpy( base + used, text );
  used += length;
  base[used] = 0;
  return base + used - length;
}
#else
char *__stdcall CTextPool__AllocText(textPool_t *this_, const char *text, int addNULL, textPool_t **poolPtr)
{
  textPool_t  *pool    = this_;
  int          len     = (int)strlen( text );
  int          addNull = ( (char)addNULL != 0 );
  int          advance = len + addNull;   /* bytes this string consumes in the pool */
  textPool_t  *fresh;
  textPool_t **current;
  int          start;

  if ( pool->used + len + 1 + addNull <= pool->size )
  {
    strcpy( pool->base + pool->used, text );
    start = pool->used;
    pool->used += advance;
    pool->base[pool->used] = 0;
    return pool->base + start;
  }

  /* copied before the test: keeps poolPtr a stack argument, as in retail's EH-framed thiscall */
  current = poolPtr;
  if ( !current )
    return 0;

  /* The current pool is full: allocate another of the same size and chain it. */
  fresh = (textPool_t *)Z_MallocInternal( 0x10u );
  if ( fresh )
  {
    fresh->next = NULL;
    fresh->size = pool->size;
    fresh->used = 0;
    fresh->base = (char *)Z_MallocInternal( pool->size );
  }

  ( *current )->next = fresh;
  *current = ( *current )->next;
  return CTextPool__AllocText( *current, text, addNULL, NULL );
}
#endif

/* ---- CTextPool__DeleteChain  0x004A1C60 ---- HIGH */
void __cdecl CTextPool__DeleteChain(textPool_t *first)
{
  textPool_t *pool = first;
  textPool_t *next;

  if ( pool )
  {
    do
    {
      next = pool->next;
      free( pool->base );
      free( pool );
      pool = next;
    }
    while ( next );
  }
}

/* ---- CTextPool__scalar_dtor  0x004A1C90 ---- HIGH */
textPool_t *__stdcall CTextPool__scalar_dtor(textPool_t *this_, char deleteFlags)
{
  free( this_->base );
  if ( (deleteFlags & 1) != 0 )
    free( this_ );
  return this_;
}

/* ---- CGPObject__CGPObject  0x004A1CC0 ---- HIGH */
gpObject_t *__cdecl CGPObject__CGPObject(gpObject_t *this_, const char *initName)
{
  this_->name = initName;
  this_->next = NULL;
  this_->inOrderNext = NULL;
  this_->inOrderPrevious = NULL;
  return this_;
}

/* ---- CGPObject__WriteText  0x004A1CD0 ---- HIGH */
char __stdcall CGPObject__WriteText(const char *text, textPool_t **textPool, gpObject_t *this_)
{
  if ( strchr(text, 32) || !*text )
  {
    GP2_AllocText( textPool, "\"", 0 );
    GP2_AllocText( textPool, text, 0 );
    GP2_AllocText( textPool, "\"", 0 );
    return 1;
  }
  else
  {
    GP2_AllocText( textPool, text, 0 );
    return 1;
  }
}

/* ---- CGPValue__CGPValue  0x004A1D30 ---- HIGH */
gpValue_t *__cdecl CGPValue__CGPValue(const char *initName, gpValue_t *this_, const char *initValue)
{
  gpValue_t *self = this_;

  self->base.name = initName;
  self->base.next = NULL;
  self->base.inOrderNext = NULL;
  self->base.inOrderPrevious = NULL;
  self->list = NULL;
  if ( initValue )
    CGPValue__AddValue( NULL, initValue, this_ );
  return this_;
}

/* ---- CGPValue__Clean  0x004A1D50 ---- HIGH */
void __cdecl CGPValue__Clean(gpValue_t *this_)
{
 gpValue_t *self=this_;
 gpObject_t *next;
 while(self->list) {
   next=self->list->next;
   free(self->list);
   self->list=next;
 }
}

/* GP2's inline CGPObject::GetName(); the accessor's temporaries give retail's registers. */
static __inline const char *CGPObject_GetName( gpObject_t *obj ) { return obj->name; }

/* ---- CGPValue__Duplicate  0x004A1D80 ---- HIGH */
#ifdef __cplusplus
/* Retail CGPValue::Duplicate is C++: `new CGPValue(name)` inlines the class
 * operator new (Z_Malloc) and the constructor, whose optional initial-value
 * AddValue gives the new-expression an EH state (fs:0 frame, /EHs) even though
 * the call is dead here; each copied value is a `new CGPObject(name)` through
 * the out-of-memory-checked malloc, inlined with AddValue's list append. */
struct CGPValue : CGPObject {
  gpObject_t *list;   /* gpValue_t.list */
  CGPValue( const char *initName, const char *initValue = 0 )
  {
    name = initName; next = NULL; inOrderNext = NULL; inOrderPrevious = NULL; list = NULL;
    if ( initValue )
      CGPValue__AddValue( NULL, initValue, (gpValue_t *)this );
  }
};
struct GpNewListObject : gpObject_t {
  GpNewListObject( const char *initName ) { name = initName; next = NULL; inOrderNext = NULL; inOrderPrevious = NULL; }
  void *operator new( size_t size )
  {
    void *block = malloc( size );
    if ( !block )
    {
      Sys_OutOfMemoryPrep();
      MessageBoxA( 0, (const char *)SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_BODY" ),
                   (const char *)SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_TITLE" ), 0x10u );
      exit( -1 );
    }
    Com_Memset( block, 0, size );
    return block;
  }
  void operator delete( void *block ) { free( block ); }
};
gpValue_t *__fastcall CGPValue__Duplicate( gpValue_t *self, GpPointerArgument poolReference )
{
  CGPValue    *newValue;
  gpObject_t  *iterator;
  const char  *name;   /* Raven's three locals: the pool stays the argument (a fourth local moves the EH temporary's slot) */

  /* taking the address keeps the declared receiver-in-ecx convention under LTCG */
  (void)&CGPValue__Duplicate;

  if ( poolReference.value )
    name = GP2_AllocText( (textPool_t **)poolReference.value, self->base.name, 1 );
  else
    name = self->base.name;

  newValue = new CGPValue( name );
  iterator = self->list;
  while ( iterator )
  {
    if ( poolReference.value )
      name = GP2_AllocText( (textPool_t **)poolReference.value, CGPObject_GetName( iterator ), 1 );
    else
      name = CGPObject_GetName( iterator );

    /* newValue->AddValue( name ) */
    if ( !newValue->list )
    {
      newValue->list = new GpNewListObject( name );
      newValue->list->inOrderNext = newValue->list;
    }
    else
    {
      newValue->list->inOrderNext->next = new GpNewListObject( name );
      newValue->list->inOrderNext = newValue->list->inOrderNext->next;
    }
    iterator = iterator->next;
  }
  return (gpValue_t *)newValue;
}
#else
gpValue_t *__fastcall CGPValue__Duplicate( gpValue_t *receiver, GpPointerArgument poolReference )
{
  gpValue_t  *src = receiver;
  textPool_t **textPool = (textPool_t **)poolReference.value;
  gpValue_t  *dest;
  gpObject_t *srcNode;
  gpObject_t *node;
  gpObject_t *tail;
  const char *dupName;
  const char *dupValue;
  const char *title;
  const char *body;

  /* taking the address keeps the declared receiver-in-ecx convention under LTCG */
  (void)&CGPValue__Duplicate;

  if ( textPool )
    dupName = GP2_AllocText( textPool, src->base.name, 1 );
  else
    dupName = src->base.name;

  dest = (gpValue_t *)Z_MallocInternal( 0x14u );
  if ( dest )
  {
    dest->base.name = dupName;
    dest->base.next = NULL;
    dest->base.inOrderNext = NULL;
    dest->base.inOrderPrevious = NULL;
    dest->list = NULL;
  }

  for ( srcNode = src->list; srcNode; srcNode = srcNode->next )
  {
    dupValue = textPool ? GP2_AllocText( textPool, srcNode->name, 1 )
                  : srcNode->name;

    node = (gpObject_t *)malloc( 0x10u );
    if ( !node )
      goto outOfMemory;
    Com_Memset( node, 0, 16 );
    node->name = dupValue;

    if ( dest->list )
    {
      /* the list head's inOrderNext doubles as the tail pointer */
      tail = dest->list->inOrderNext;
      tail->next = node;
      dest->list->inOrderNext = node;
    }
    else
    {
      dest->list = node;
      node->inOrderNext = node;
    }
  }
  return dest;

outOfMemory:
  Sys_OutOfMemoryPrep();
  title = (const char *)SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_TITLE" );
  body  = (const char *)SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_BODY" );
  MessageBoxA( 0, body, title, 0x10u );
  exit( -1 );
}
#endif

/* ---- CGPValue__IsList  0x004A1EE0 ---- HIGH */
char __cdecl CGPValue__IsList(gpValue_t *this_)
{
  gpValue_t *self = this_;

  if ( self->list && self->list->next )
    return 1;
  return 0;
}

/* ---- CGPValue__GetTopValue  0x004A1F00 ---- HIGH */
const char *__cdecl CGPValue__GetTopValue(gpValue_t *this_)
{
  gpValue_t *self = this_;

  if ( self->list )
    return self->list->name;
  else
    return NULL;
}

/* ---- CGPValue__AddValue  0x004A1F10 ---- HIGH */
__declspec(noinline) void __cdecl CGPValue__AddValue(textPool_t **textPool, const char *newValue, gpValue_t *this_)
{
 gpValue_t *self=this_;
 gpObject_t *mem;
 gpObject_t *node;
 if(textPool) newValue=GP2_AllocText( textPool, newValue, 1 );
 if(!self->list) {
  /* new CGPObject(newValue) */
  mem=(gpObject_t*)Z_MallocInternal(16);
  if(mem) {mem->name=newValue;mem->next=0;mem->inOrderNext=0;mem->inOrderPrevious=0;node=mem;}
  else node=0;
  self->list=node;
  node->inOrderNext=node;
  return;
 }
 mem=(gpObject_t*)Z_MallocInternal(16);
 if(mem) {mem->name=newValue;mem->next=0;mem->inOrderNext=0;mem->inOrderPrevious=0;node=mem;}
 else node=0;
 self->list->inOrderNext->next=node;
 self->list->inOrderNext=self->list->inOrderNext->next;
}

/* ---- CGPValue__Parse  0x004A1F90 ---- CONFIRMED */
char __stdcall CGPValue__Parse(textPool_t **textPool, gpValue_t *this_, char **dataPtr)
{
 char *token;
 const char *pooled;
 for (;;) {
  token=GP2_GetToken(1,1,dataPtr);
  if(!*token) return 0;
  if(!_stricmp(token,"]")) return 1;
  pooled=GP2_AllocText( textPool, token, 1 );
  CGPValue__AddValue(NULL,pooled,this_);
 }
}

/* ---- CGPValue__Write  0x004A2000 ---- CONFIRMED */
char __stdcall CGPValue__Write(gpValue_t *this_, textPool_t **textPool, int depth)
{
  gpValue_t  *self = this_;
  gpObject_t *next;
  int         i;

  if ( !self->list )
    return 1;

  for ( i = 0; i < depth; i++ )
    GP2_AllocText( textPool, "\t", 0 );

  CGPObject__WriteText( self->base.name, textPool, &this_->base );

  if ( !self->list->next )
  {
    GP2_AllocText( textPool, "\t\t", 0 );
    CGPObject__WriteText( self->list->name, textPool, self->list );
    GP2_AllocText( textPool, "\r\n", 0 );
  }
  else
  {
    GP2_AllocText( textPool, "\r\n", 0 );

    for ( i = 0; i < depth; i++ )
      GP2_AllocText( textPool, "\t", 0 );
    GP2_AllocText( textPool, "[\r\n", 0 );

    next = self->list;
    while ( next )
    {
      for ( i = 0; i < depth + 1; i++ )
        GP2_AllocText( textPool, "\t", 0 );
      CGPObject__WriteText( next->name, textPool, self->list );
      GP2_AllocText( textPool, "\r\n", 0 );

      next = next->next;
    }

    for ( i = 0; i < depth; i++ )
      GP2_AllocText( textPool, "\t", 0 );
    GP2_AllocText( textPool, "]\r\n", 0 );
  }

  return 1;
}

/* ---- CGPGroup__CGPGroup  0x004A2200 ---- HIGH */
gpGroup_t *__cdecl CGPGroup__CGPGroup(gpGroup_t *this_, gpGroup_t *initParent, const char *initName)
{
  gpGroup_t *self = this_;

  self->base.name = initName;
  self->base.next = NULL;
  self->base.inOrderNext = NULL;
  self->base.inOrderPrevious = NULL;
  self->pairs = NULL;
  self->pairsInOrder = NULL;
  self->currentPair = NULL;
  self->subGroups = NULL;
  self->subGroupsInOrder = NULL;
  self->currentSubGroup = NULL;
  self->parent = initParent;
  self->writeable = 0;
  return this_;
}

/* ---- CGPGroup::~CGPGroup  0x004A2230 ---- HIGH */
CGPGroup::~CGPGroup()
{
  CGPGroup__Clean( this );
}

/* ---- CGPGroup__GetNumSubGroups  0x004A2240 ---- HIGH */
int __cdecl CGPGroup__GetNumSubGroups(gpGroup_t *self_)
{
  int        count;
  gpGroup_t *node;

  count = 0;
  node = self_->subGroups;

  /* NOTE: pre-advances the walker -- reproduces the retail body verbatim,
   * which walks node->next before counting and dereferences an empty list. */
  do
  {
    node = (gpGroup_t *)node->base.next;
    ++count;
  }
  while ( node );
  return count;
}

/* ---- CGPGroup__GetNumPairs  0x004A2250 ---- HIGH */
int __cdecl CGPGroup__GetNumPairs(gpGroup_t *self_)
{
  int        count;
  gpValue_t *node;

  count = 0;
  node = self_->pairs;

  do
  {
    node = (gpValue_t *)node->base.next;
    ++count;
  }
  while ( node );
  return count;
}

/* ---- CGPGroup__Clean  0x004A2260 ---- HIGH */
void __fastcall CGPGroup__Clean(gpGroup_t *self_)
{
  gpGroup_t  *self = self_;
  gpValue_t  *pair;
  gpObject_t *nextValue;
  gpGroup_t  *sub;

  
  while ( self->pairs )
  {
    self->currentPair = (gpValue_t *)self->pairs->base.next;
    /* delete mPairs */
    pair = self->pairs;
    if ( pair )
    {
      CGPValue__Clean(pair);
      free( pair );
    }
    self->pairs = self->currentPair;
  }

  while ( self->subGroups )
  {
    self->currentSubGroup = (gpGroup_t *)self->subGroups->base.next;
    /* delete mSubGroups */
    sub = self->subGroups;
    if ( sub )
    {
      CGPGroup__Clean( sub );
      free( sub );
    }
    self->subGroups = self->currentSubGroup;
  }

  self->currentPair = NULL;
  self->pairsInOrder = NULL;
  self->pairs = NULL;
  self->currentSubGroup = NULL;
  self->subGroupsInOrder = NULL;
  self->subGroups = NULL;
  self->parent = NULL;
  self->writeable = 0;
  return;
}

/* ---- CGPValue__scalar_dtor  0x004A2300 ---- HIGH */
gpValue_t *__stdcall CGPValue__scalar_dtor(gpValue_t *this_, char deleteFlags)
{
  gpValue_t  *self = this_;
  gpObject_t *next;

  CGPValue__Clean(this_);
  if ( (deleteFlags & 1) != 0 )
    free( this_ );
  return this_;
}

/* ---- CGPGroup__scalar_dtor  0x004A2340 ---- HIGH */
gpGroup_t *__stdcall CGPGroup__scalar_dtor(gpGroup_t *this_, char deleteFlags)
{
  CGPGroup__Clean(this_);
  if ( (deleteFlags & 1) != 0 )
    free(this_);
  return this_;
}

/* ---- CGPGroup__Duplicate  0x004A2360 ---- CONFIRMED */
gpGroup_t *__fastcall CGPGroup__Duplicate(gpGroup_t *receiver, GpPointerArgument poolReference, GpPointerArgument parentReference)
{
  gpGroup_t *src = receiver;
  textPool_t **textPool = (textPool_t **)poolReference.value;
  const char *dupName;
  gpGroup_t  *mem;
  gpGroup_t  *dest;
  gpGroup_t  *srcSub;
  gpValue_t  *srcPair;

  (void)parentReference;   

  if ( textPool )
    dupName = GP2_AllocText( textPool, src->base.name, 1 );
  else
    dupName = src->base.name;

  /* new CGPGroup(dupName) */
  mem = (gpGroup_t *)Z_MallocInternal( 0x30u );
  if ( mem )
  {
    mem->base.name = dupName;
    mem->base.next = NULL;
    mem->base.inOrderNext = NULL;
    mem->base.inOrderPrevious = NULL;
    mem->pairs = NULL;
    mem->pairsInOrder = NULL;
    mem->currentPair = NULL;
    mem->subGroups = NULL;
    mem->subGroupsInOrder = NULL;
    mem->currentSubGroup = NULL;
    mem->parent = NULL;
    mem->writeable = 0;
    dest = mem;
  }
  else dest=0;

  for ( srcSub = src->subGroups; srcSub; srcSub = (gpGroup_t *)srcSub->base.next )
  {
    GpPointerArgument poolArg = { textPool };
    GpPointerArgument parentArg = { dest };
    gpGroup_t *copy = CGPGroup__Duplicate(srcSub, poolArg, parentArg);
    CGP_InsertInOrder( dest, &copy->base, (gpObject_t **)&dest->subGroups,
                       (gpObject_t **)&dest->subGroupsInOrder, (gpObject_t **)&dest->currentSubGroup );
  }
  for ( srcPair = src->pairs; srcPair; srcPair = (gpValue_t *)srcPair->base.next )
  {
    GpPointerArgument poolArg = { textPool };
    gpValue_t *copy = CGPValue__Duplicate( srcPair, poolArg );
    CGP_InsertInOrder( dest, &copy->base, (gpObject_t **)&dest->pairs,
                       (gpObject_t **)&dest->pairsInOrder, (gpObject_t **)&dest->currentPair );
  }
  return dest;
}


/* ---- CGP_InsertInOrder  0x004A2430 ---- HIGH */
/* GP2 CGPGroup::SortObject: `this` is forwarded but never read. */
void __stdcall CGP_InsertInOrder(gpGroup_t *this_, struct gpObject_s *object, struct gpObject_s **unsortedList,
                                 struct gpObject_s **sortedList, struct gpObject_s **lastObject) GP_NOTHROW
{
  gpObject_t *test;
  gpObject_t *last;

  (void)this_;

  if ( !*unsortedList )
  {
    *unsortedList = *sortedList = object;
  }
  else
  {
    ( *lastObject )->next = object;   /* link after the sibling tail */

    /* walk the alphabetically ordered list; insert before the first larger name */
    test = *sortedList;
    last = NULL;
    while ( test )
    {
      if ( _stricmp( CGPObject_GetName( object ), CGPObject_GetName( test ) ) < 0 )
        break;
      last = test;
      test = test->inOrderNext;
    }

    if ( test )
    {
      test->inOrderPrevious = object;
      object->inOrderNext = test;
    }
    if ( last )
    {
      last->inOrderNext = object;
      object->inOrderPrevious = last;
    }
    else
    {
      *sortedList = object;
    }
  }

  *lastObject = object;
}

/* ---- CGPGroup__AddPair  0x004A24B0 ---- HIGH */
gpValue_t *__stdcall CGPGroup__AddPair(const char *value, textPool_t **textPool, gpGroup_t *this_, const char *initName)
{
  gpGroup_t  *group = this_;
  const char *name = initName;
  gpValue_t  *pair;
  gpValue_t  *result;

  if ( textPool )
  {
    name = GP2_AllocText( textPool, initName, 1 );
    if ( value )
      value = GP2_AllocText( textPool, value, 1 );
  }

#ifdef __cplusplus
  /* retail `new CGPValue(name, value)`: the constructor's AddValue runs in EH state 0 */
  result = (gpValue_t *)new CGPValue( name, value );
  (void)pair;
#else
  pair = (gpValue_t *)Z_MallocInternal( 0x14u );
  if ( pair )
  {
    pair->base.name = name;
    pair->base.next = NULL;
    pair->base.inOrderNext = NULL;
    pair->base.inOrderPrevious = NULL;
    pair->list = NULL;
    if ( value )
      CGPValue__AddValue( NULL, value, pair );
    result = pair;
  }
  else
  {
    result = NULL;
  }
#endif

  CGP_InsertInOrder( this_, &result->base, (gpObject_t **)&group->pairs,
                     (gpObject_t **)&group->pairsInOrder, (gpObject_t **)&group->currentPair );
  return result;
}

/* ---- CGPGroup__AddPairObject  0x004A2560 ---- HIGH */
void __cdecl CGPGroup__AddPairObject(gpGroup_t *this_, gpValue_t *pair)
{
  gpGroup_t *group = this_;

  CGP_InsertInOrder( this_, &pair->base, (gpObject_t **)&group->pairs,
                     (gpObject_t **)&group->pairsInOrder, (gpObject_t **)&group->currentPair );
}

/* ---- CGPGroup__AddGroup  0x004A2580 ---- HIGH */
gpGroup_t *__stdcall CGPGroup__AddGroup(const char *initName, textPool_t **textPool, gpGroup_t *this_)
{
  gpGroup_t  *parent = this_;
  const char *name = initName;
  gpGroup_t  *group;
  gpGroup_t  *result;

  if ( textPool )
    name = GP2_AllocText( textPool, initName, 1 );

  group = (gpGroup_t *)Z_MallocInternal( 0x30u );
  if ( group )
  {
    group->base.name = name;
    group->base.next = NULL;
    group->base.inOrderNext = NULL;
    group->base.inOrderPrevious = NULL;
    group->pairs = NULL;
    group->pairsInOrder = NULL;
    group->currentPair = NULL;
    group->subGroups = NULL;
    group->subGroupsInOrder = NULL;
    group->currentSubGroup = NULL;
    group->parent = NULL;
    group->writeable = 0;
    result = group;
  }
  else
  {
    result = NULL;
  }

  CGP_InsertInOrder( this_, &result->base, (gpObject_t **)&parent->subGroups,
                     (gpObject_t **)&parent->subGroupsInOrder, (gpObject_t **)&parent->currentSubGroup );
  return result;
}

/* ---- CGPGroup__AddGroupObject  0x004A25F0 ---- HIGH */
void __cdecl CGPGroup__AddGroupObject(gpGroup_t *this_, gpGroup_t *subGroup)
{
  gpGroup_t *group = this_;

  CGP_InsertInOrder( this_, &subGroup->base, (gpObject_t **)&group->subGroups,
                     (gpObject_t **)&group->subGroupsInOrder, (gpObject_t **)&group->currentSubGroup );
}

/* ---- CGPGroup__FindSubGroup  0x004A2610 ---- HIGH */
gpGroup_t *__cdecl CGPGroup__FindSubGroup(gpGroup_t *this_, const char *name)
{
  gpGroup_t *sub = this_->subGroups;

  while ( sub )
  {
    if ( !_stricmp( name, CGPObject_GetName( &sub->base ) ) )
      return sub;
    sub = (gpGroup_t *)sub->base.next;
  }
  return NULL;
}

/* ---- CGPGroup__Parse  0x004A2640 ---- CONFIRMED */
char __fastcall CGPGroup__Parse(gpGroup_t *receiver, GpPointerArgument dataReference, GpPointerArgument poolReference)
{
  gpGroup_t *group = receiver;
  char **dataPtr = (char**)dataReference.value;
  textPool_t **textPool = (textPool_t **)poolReference.value;
  gpGroup_t *sub;
  gpValue_t *pair;
  char      *token;
  char      *next;
  char       key[1024];

  for (;;) {
    token = GP2_GetToken(1,0,dataPtr);
    if (!*token) {
      /* end of data: an error inside a nested group */
      if (group->parent) return 0;
      break;
    } else if (!_stricmp(token,"}")) {
      break;
    }
    strcpy(key,token);
    next = GP2_GetToken(1,1,dataPtr);
    if (!_stricmp(next,"{")) {
      sub=CGPGroup__AddGroup(key,textPool,group);
      sub->writeable=group->writeable;
      if (!CGPGroup__Parse(sub,dataReference,poolReference)) return 0;
    } else if (!_stricmp(next,"[")) {
      pair=CGPGroup__AddPair(NULL,textPool,group,key);
      if (!CGPValue__Parse(textPool,pair,dataPtr)) return 0;
    } else CGPGroup__AddPair(next,textPool,group,key);
  }
  return 1;
}

/* ---- CGPGroup::Write  0x004A2790 ---- CONFIRMED */
char CGPGroup::Write(textPool_t **poolReference, int depthReference)
{
  gpGroup_t *group = this;
  textPool_t **textPool = poolReference;
  int startDepth = depthReference;
  gpValue_t  *pair;
  gpGroup_t  *sub;
  const char *name;
  const char *writeStr;
  int         depth = startDepth;
  int         i;

  pair = group->pairs;
  sub  = group->subGroups;

  
  if ( startDepth >= 0 )
  {
    for ( i = startDepth; i > 0; i-- )
      GP2_AllocText( textPool, "\t", 0 );

    name = group->base.name;
    if (strchr(name,32) || !*name) {
      GP2_AllocText( textPool, "\"", 0 );
      GP2_AllocText( textPool, name, 0 );
      GP2_AllocText( textPool, "\"", 0 );
    } else GP2_AllocText( textPool, name, 0 );
    GP2_AllocText( textPool, "\r\n", 0 );

    for ( i = startDepth; i > 0; i-- )
      GP2_AllocText( textPool, "\t", 0 );
    GP2_AllocText( textPool, "{\r\n", 0 );
    depth = startDepth;
  }

  if ( pair )
  {
    do
    {
      CGPValue__Write( pair, textPool, depth + 1 );
      pair = (gpValue_t *)pair->base.next;
    }
    while ( pair );
    depth = startDepth;
  }

  if ( sub )
  {
    do
    {
      ((CGPGroup *)sub)->Write( poolReference, depth + 1 );
      sub = (gpGroup_t *)sub->base.next;
    }
    while ( sub );
    depth = startDepth;
  }

  if ( depth >= 0 )
  {
    for ( i = depth; i > 0; i-- )
      GP2_AllocText( textPool, "\t", 0 );
    GP2_AllocText( textPool, "}\r\n", 0 );
  }
  return 1;
}

/* ---- CGPGroup__FindPair  0x004A28C0 ---- HIGH */
gpValue_t *__cdecl CGPGroup__FindPair(gpGroup_t *this_, const char *key)
{
 gpValue_t *pair;
 for (pair=this_->pairs; pair; pair=(gpValue_t *)pair->base.next) {
  if (!_stricmp(CGPObject_GetName( &pair->base ),key)) { return pair; }
 }
 return NULL;
}

/* ---- CGPGroup__FindPairValue  0x004A28F0 ---- HIGH */
const char *__stdcall CGPGroup__FindPairValue(gpGroup_t *this_, const char *key, const char *defaultVal)
{
 gpValue_t *pair=this_->pairs;
 for (;;) {
  if (!pair) return defaultVal;
  if (!_stricmp(CGPObject_GetName( &pair->base ),key)) break;
  pair=(gpValue_t *)pair->base.next;
 }
 if (pair->list) return pair->list->name;
 return NULL;
}

/* ---- CGenericParser2__CGenericParser2  0x004A2930 ---- CONFIRMED */
genericParser2_t *__cdecl CGenericParser2__CGenericParser2(genericParser2_t *this_)
{
  genericParser2_t *self = this_;

  self->topLevel.base.name = "Top Level";
  self->topLevel.base.next = NULL;
  self->topLevel.base.inOrderNext = NULL;
  self->topLevel.base.inOrderPrevious = NULL;
  self->topLevel.pairs = NULL;
  self->topLevel.pairsInOrder = NULL;
  self->topLevel.currentPair = NULL;
  self->topLevel.subGroups = NULL;
  self->topLevel.subGroupsInOrder = NULL;
  self->topLevel.currentSubGroup = NULL;
  self->topLevel.parent = NULL;
  self->topLevel.writeable = 0;
  self->textPool = NULL;
  self->writeable = 0;
  return this_;
}

/* ---- CGenericParser2__Clean  0x004A2960 ---- HIGH */
#ifdef __cplusplus
/* ~CGenericParser2 (this on the stack under LTCG): the body cleans the text
 * pool, then the top-level CGPGroup member's destructor runs, so
 * CleanTextPool sits in EH state 0 of an fs:0 frame (/EHs). */
CGenericParser2::~CGenericParser2()
{
  CGenericParser2__CleanTextPool( (genericParser2_t *)this );
}
#else
void __stdcall CGenericParser2__Clean(GpPointerArgument receiverReference)
{
  genericParser2_t *self=(genericParser2_t *)receiverReference.value;
  CGenericParser2__CleanTextPool(self);
  CGPGroup__Clean(&self->topLevel);
}
#endif

/* ---- CGenericParser2__Parse  0x004A29B0 ---- HIGH */
char __stdcall CGenericParser2__Parse(genericParser2_t *this_, char **dataPtr, char cleanFirst, char writeable)
{
  genericParser2_t *self = this_;
  textPool_t       *poolArg;

  if ( (char)cleanFirst )
    CGenericParser2__CleanTextPool( self );

  if ( !self->textPool )
    self->textPool = new CTextPool;   /* `new CTextPool`: operator delete runs if the constructor throws */

  self->writeable = writeable;
  self->topLevel.writeable = writeable;
  poolArg = self->textPool;
  { GpPointerArgument dataArg = { dataPtr };
    GpPointerArgument poolArgRef = { &poolArg };
    return CGPGroup__Parse(&self->topLevel, dataArg, poolArgRef); }
}

/* ---- CGenericParser2__CleanTextPool  0x004A2A60 ---- HIGH */
void __cdecl CGenericParser2__CleanTextPool(genericParser2_t *this_)
{
  genericParser2_t *self = this_;
  textPool_t       *pool;
  textPool_t       *next;

  CGPGroup__Clean( &this_->topLevel );

  pool = self->textPool;
  if ( pool )
  {
    do
    {
      next = pool->next;
      free( pool->base );
      free( pool );
      pool = next;
    }
    while ( next );
  }
  self->textPool = NULL;
}

/* ---- CGenericParser2__Write  0x004A2AA0 ---- HIGH */
char __stdcall CGenericParser2__Write(genericParser2_t *this_, textPool_t *textPool)
{
  genericParser2_t *self = this_;

  return ((CGPGroup *)&self->topLevel)->Write( &textPool, -1 );   /* mTopLevel.Write(&textPool, -1) */
}

/* ---- adler32  0x004A2AF0 ---- VERIFIED */
int __cdecl adler32(unsigned int adler, unsigned char *buf, unsigned int len)
{
 unsigned int s1=adler&0xffffu,s2=adler>>16;int count;if(!buf)return 1;
while(len>0){count=(len<5552u)?(int)len:5552; len-=count; while(count>=16){s1 += buf[0]; s2 += s1;
s1 += buf[1]; s2 += s1;
s1 += buf[2]; s2 += s1;
s1 += buf[3]; s2 += s1;
s1 += buf[4]; s2 += s1;
s1 += buf[5]; s2 += s1;
s1 += buf[6]; s2 += s1;
s1 += buf[7]; s2 += s1;
s1 += buf[8]; s2 += s1;
s1 += buf[9]; s2 += s1;
s1 += buf[10]; s2 += s1;
s1 += buf[11]; s2 += s1;
s1 += buf[12]; s2 += s1;
s1 += buf[13]; s2 += s1;
s1 += buf[14]; s2 += s1;
s1 += buf[15]; s2 += s1;
 buf += 16; count-=16;} if(count!=0)do{s1+=*buf++;s2+=s1;}while(--count);s1%=65521u;s2%=65521u;}
return s1|(s2<<16);
}

/* ---- compress2  0x004A2C20 ---- VERIFIED */
unsigned int __cdecl compress2(int source, int dest, int sourceLen, int *destLen, unsigned int level)
{
  z_stream     stream;
  unsigned int result;
  int          ret;

  stream.next_in   = (Byte *)source;
  stream.avail_in  = (uInt)sourceLen;
  stream.next_out  = (Byte *)dest;
  stream.avail_out = (uInt)*destLen;
  stream.zalloc = NULL; stream.zfree = NULL; stream.opaque = NULL;

  result = deflateInit2_( &stream, level, Z_DEFLATED, 15, 8, 0, "1.1.4", sizeof( z_stream ) );   /* deflateInit(&stream, level) */
  if ( result ) return result;
  ret = deflate( &stream, Z_FINISH );
  if ( ret != Z_STREAM_END ) { deflateEnd( &stream ); return ret == Z_OK ? Z_BUF_ERROR : ret; }
  *destLen = stream.total_out;
  return deflateEnd( &stream );
}

/* ---- compress  0x004A2CC0 ---- CONFIRMED */
unsigned int __cdecl compress(int *destLen, int dest, int sourceLen, int source)
{
  return compress2(source, dest, sourceLen, destLen, 0xFFFFFFFF);
}

/* ---- get_crc_table  0x004A2CD0 ---- CONFIRMED */
int *get_crc_table()
{
  return (int *)crc_table;
}

/* ---- crc32  0x004A2CE0 ---- VERIFIED */
/* zlib 1.1.4 crc32.c (the caller-visible argument order is the tree's). */
#define CRC_DO1(buf) c = crc_table[((int)c ^ (*buf++)) & 0xff] ^ (c >> 8);
#define CRC_DO2(buf) CRC_DO1(buf); CRC_DO1(buf);
#define CRC_DO4(buf) CRC_DO2(buf); CRC_DO2(buf);
#define CRC_DO8(buf) CRC_DO4(buf); CRC_DO4(buf);
int __cdecl crc32(int crc, const unsigned char *buf, unsigned int len)
{
    unsigned int c;

    if (buf == NULL) return 0;
    c = (unsigned int)crc ^ 0xffffffffUL;
    while (len >= 8)
    {
      CRC_DO8(buf);
      len -= 8;
    }
    if (len) do {
      CRC_DO1(buf);
    } while (--len);
    return (int)(c ^ 0xffffffffUL);
}

/* ---- deflateInit_  0x004A2E00 ---- VERIFIED */
int __cdecl deflateInit_(int stream_size, z_streamp strm, unsigned int level, const char *version)
{
  return deflateInit2_(strm, level, Z_DEFLATED, 15, 8, 0, version, stream_size);
}

/* deflate.c: `static const char* my_version = ZLIB_VERSION;` (retail 0x00572D58) */
static const char *my_version = "1.1.4";

/* ---- deflateInit2_  0x004A2E20 ---- CONFIRMED */
int __cdecl deflateInit2_(
        z_streamp strm,
        int level,
        int method,
        int windowBits,
        int memLevel,
        int strategy,
        const char *version,
        int stream_size)
{
  deflate_state *s;
  int            noheader = 0;
  ush           *overlay;

  if ( version == NULL || version[0] != my_version[0] || stream_size != sizeof( z_stream ) )
    return -6;                     /* Z_VERSION_ERROR */
  if ( strm == NULL )
    return Z_STREAM_ERROR;

  strm->msg = NULL;
  if ( !strm->zalloc )
  {
    strm->zalloc = (alloc_func)zcalloc;
    strm->opaque = NULL;
  }
  if ( !strm->zfree )
    strm->zfree = (free_func)zcfree;

  if ( level == Z_DEFAULT_COMPRESSION )
    level = 6;

  if ( windowBits < 0 )            /* undocumented feature: suppress zlib header */
  {
    noheader = 1;
    windowBits = -windowBits;
  }
  if ( memLevel < 1 || memLevel > 9 || method != Z_DEFLATED ||
       windowBits < 9 || windowBits > 15 || level < 0 || level > 9 ||
       strategy < 0 || strategy > Z_HUFFMAN_ONLY )
    return Z_STREAM_ERROR;

  s = (deflate_state *)ZALLOC( strm, 1, sizeof( deflate_state ) );
  if ( s == NULL )
    return Z_MEM_ERROR;
  strm->state = s;
  s->strm = strm;

  s->noheader = noheader;
  s->w_bits = windowBits;
  s->w_size = 1 << s->w_bits;
  s->w_mask = s->w_size - 1;

  s->hash_bits = memLevel + 7;
  s->hash_size = 1 << s->hash_bits;
  s->hash_mask = s->hash_size - 1;
  s->hash_shift = ( s->hash_bits + MIN_MATCH - 1 ) / MIN_MATCH;

  s->window = (Byte *)ZALLOC( strm, s->w_size, 2 * sizeof( Byte ) );
  s->prev   = (Pos *)ZALLOC( strm, s->w_size, sizeof( Pos ) );
  s->head   = (Pos *)ZALLOC( strm, s->hash_size, sizeof( Pos ) );

  s->lit_bufsize = 1 << ( memLevel + 6 );   /* 16K elements by default */

  overlay = (ush *)ZALLOC( strm, s->lit_bufsize, sizeof( ush ) + 2 );
  s->pending_buf = (uch *)overlay;
  s->pending_buf_size = (ulg)s->lit_bufsize * ( sizeof( ush ) + 2L );

  if ( s->window == NULL || s->prev == NULL || s->head == NULL || s->pending_buf == NULL )
  {
    strm->msg = ERR_MSG( Z_MEM_ERROR );
    deflateEnd( strm );
    return Z_MEM_ERROR;
  }
  s->d_buf = overlay + s->lit_bufsize / sizeof( ush );
  s->l_buf = s->pending_buf + ( 1 + sizeof( ush ) ) * s->lit_bufsize;

  s->level = level;
  s->strategy = strategy;
  s->method = (Byte)method;

  return deflateReset( strm );
}

/* ---- deflateReset  0x004A3120 ---- */
/* Missing from the retail function list: its body is the tail of the
 * deflateInit2_ span (0x004A2E20..0x004A3188), after deflateSetDictionary. */
int __cdecl deflateReset(z_streamp strm)
{
  deflate_state *s;

  if ( strm == NULL || strm->state == NULL || !strm->zalloc || !strm->zfree )
    return Z_STREAM_ERROR;

  strm->total_in = strm->total_out = 0;
  strm->msg = NULL;
  strm->data_type = 2;             /* Z_UNKNOWN */

  s = strm->state;
  s->pending = 0;
  s->pending_out = s->pending_buf;

  if ( s->noheader < 0 )
    s->noheader = 0;               /* was set to -1 by deflate(..., Z_FINISH) */
  s->status = s->noheader ? BUSY_STATE : INIT_STATE;
  strm->adler = 1;
  s->last_flush = Z_NO_FLUSH;

  _tr_init( s );
  lm_init( s );
  return Z_OK;
}

/* ---- deflateParams  0x004A3190 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c (LTCG inlines deflate() into it) */
int __cdecl deflateParams( z_streamp strm, int level, int strategy )
{
	deflate_state *s;
	int ( __cdecl *func )( deflate_state *s, int flush );
	int err = Z_OK;

	if ( strm == NULL || strm->state == NULL ) return Z_STREAM_ERROR;
	s = strm->state;

	if ( level == Z_DEFAULT_COMPRESSION ) {
		level = 6;
	}
	if ( level < 0 || level > 9 || strategy < 0 || strategy > Z_HUFFMAN_ONLY ) {
		return Z_STREAM_ERROR;
	}
	func = deflate_configuration_table[s->level].func;

	if ( func != deflate_configuration_table[level].func && strm->total_in != 0 ) {
		/* Flush the last buffer: */
		err = deflate( strm, Z_PARTIAL_FLUSH );
	}
	if ( s->level != level ) {
		s->level = level;
		s->max_lazy_match   = deflate_configuration_table[level].max_lazy;
		s->good_match       = deflate_configuration_table[level].good_length;
		s->nice_match       = deflate_configuration_table[level].nice_length;
		s->max_chain_length = deflate_configuration_table[level].max_chain;
	}
	s->strategy = strategy;
	return err;
}

/* ---- putShortMSB  0x004A3410 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c: put_byte( s, c ) is s->pending_buf[s->pending++] = c */
void __cdecl putShortMSB( deflate_state *s, uInt b )
{
	s->pending_buf[s->pending++] = (Byte)( b >> 8 );
	s->pending_buf[s->pending++] = (Byte)( b & 0xff );
}

/* ---- flush_pending  0x004A3440 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
void __cdecl flush_pending( z_streamp strm )
{
	unsigned len = strm->state->pending;

	if ( len > strm->avail_out ) len = strm->avail_out;
	if ( len == 0 ) return;

	memcpy( strm->next_out, strm->state->pending_out, len );
	strm->next_out  += len;
	strm->state->pending_out  += len;
	strm->total_out += len;
	strm->avail_out  -= len;
	strm->state->pending -= len;
	if ( strm->state->pending == 0 ) {
		strm->state->pending_out = strm->state->pending_buf;
	}
}

/* ---- deflate  0x004A34B0 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
int __cdecl deflate( z_streamp strm, int flush )
{
	int old_flush; /* value of flush param for previous deflate call */
	deflate_state *s;

	if ( strm == NULL || strm->state == NULL ||
		flush > Z_FINISH || flush < 0 ) {
		return Z_STREAM_ERROR;
	}
	s = strm->state;

	if ( strm->next_out == NULL ||
		( strm->next_in == NULL && strm->avail_in != 0 ) ||
		( s->status == FINISH_STATE && flush != Z_FINISH ) ) {
		ERR_RETURN( strm, Z_STREAM_ERROR );
	}
	if ( strm->avail_out == 0 ) ERR_RETURN( strm, Z_BUF_ERROR );

	s->strm = strm; /* just in case */
	old_flush = s->last_flush;
	s->last_flush = flush;

	/* Write the zlib header */
	if ( s->status == INIT_STATE ) {

		uInt header = ( Z_DEFLATED + ( ( s->w_bits - 8 ) << 4 ) ) << 8;
		uInt level_flags = ( s->level - 1 ) >> 1;

		if ( level_flags > 3 ) level_flags = 3;
		header |= ( level_flags << 6 );
		if ( s->strstart != 0 ) header |= PRESET_DICT;
		header += 31 - ( header % 31 );

		s->status = BUSY_STATE;
		putShortMSB( s, header );

		/* Save the adler32 of the preset dictionary: */
		if ( s->strstart != 0 ) {
			putShortMSB( s, (uInt)( strm->adler >> 16 ) );
			putShortMSB( s, (uInt)( strm->adler & 0xffff ) );
		}
		strm->adler = 1L;
	}

	/* Flush as much pending output as possible */
	if ( s->pending != 0 ) {
		flush_pending( strm );
		if ( strm->avail_out == 0 ) {
			/* Since avail_out is 0, deflate will be called again with
			 * more output space, but possibly with both pending and
			 * avail_in equal to zero. There won't be anything to do,
			 * but this is not an error situation so make sure we
			 * return OK instead of BUF_ERROR at next call of deflate: */
			s->last_flush = -1;
			return Z_OK;
		}

	/* Make sure there is something to do and avoid duplicate consecutive
	 * flushes. For repeated and useless calls with Z_FINISH, we keep
	 * returning Z_STREAM_END instead of Z_BUFF_ERROR. */
	} else if ( strm->avail_in == 0 && flush <= old_flush &&
		flush != Z_FINISH ) {
		ERR_RETURN( strm, Z_BUF_ERROR );
	}

	/* User must not provide more input after the first FINISH: */
	if ( s->status == FINISH_STATE && strm->avail_in != 0 ) {
		ERR_RETURN( strm, Z_BUF_ERROR );
	}

	/* Start a new block or continue the current one. */
	if ( strm->avail_in != 0 || s->lookahead != 0 ||
		( flush != Z_NO_FLUSH && s->status != FINISH_STATE ) ) {
		int bstate;

		bstate = ( *( deflate_configuration_table[s->level].func ) )( s, flush );

		if ( bstate == finish_started || bstate == finish_done ) {
			s->status = FINISH_STATE;
		}
		if ( bstate == need_more || bstate == finish_started ) {
			if ( strm->avail_out == 0 ) {
				s->last_flush = -1; /* avoid BUF_ERROR next call, see above */
			}
			return Z_OK;
		}
		if ( bstate == block_done ) {
			if ( flush == Z_PARTIAL_FLUSH ) {
				_tr_align( s );
			} else { /* FULL_FLUSH or SYNC_FLUSH */
				_tr_stored_block( s, 0, (char *)0, 0 );
				/* For a full flush, this empty block will be recognized
				 * as a special marker by inflate_sync(). */
				if ( flush == Z_FULL_FLUSH ) {
					CLEAR_HASH( s );             /* forget history */
				}
			}
			flush_pending( strm );
			if ( strm->avail_out == 0 ) {
				s->last_flush = -1; /* avoid BUF_ERROR at next call, see above */
				return Z_OK;
			}
		}
	}

	if ( flush != Z_FINISH ) return Z_OK;
	if ( s->noheader ) return Z_STREAM_END;

	/* Write the zlib trailer (adler32) */
	putShortMSB( s, (uInt)( strm->adler >> 16 ) );
	putShortMSB( s, (uInt)( strm->adler & 0xffff ) );
	flush_pending( strm );
	/* If avail_out is zero, the application will call deflate again
	 * to flush the rest. */
	s->noheader = -1; /* write the trailer only once! */
	return s->pending != 0 ? Z_OK : Z_STREAM_END;
}

/* ---- deflateEnd  0x004A3730 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
int __cdecl deflateEnd( z_streamp strm )
{
	int status;

	if ( strm == NULL || strm->state == NULL ) return Z_STREAM_ERROR;

	status = strm->state->status;
	if ( status != INIT_STATE && status != BUSY_STATE &&
		status != FINISH_STATE ) {
		return Z_STREAM_ERROR;
	}

	/* Deallocate in reverse order of allocations: */
	TRY_FREE( strm, strm->state->pending_buf );
	TRY_FREE( strm, strm->state->head );
	TRY_FREE( strm, strm->state->prev );
	TRY_FREE( strm, strm->state->window );

	ZFREE( strm, strm->state );
	strm->state = NULL;

	return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
}

/* ---- deflateCopy  0x004A37E0 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
int __cdecl deflateCopy( z_streamp dest, z_streamp source )
{
	deflate_state *ds;
	deflate_state *ss;
	ush *overlay;

	if ( source == NULL || dest == NULL || source->state == NULL ) {
		return Z_STREAM_ERROR;
	}

	ss = source->state;

	*dest = *source;

	ds = (deflate_state *)ZALLOC( dest, 1, sizeof( deflate_state ) );
	if ( ds == NULL ) return Z_MEM_ERROR;
	dest->state = (struct internal_state *)ds;
	*ds = *ss;
	ds->strm = dest;

	ds->window = (Byte *)ZALLOC( dest, ds->w_size, 2 * sizeof( Byte ) );
	ds->prev   = (Pos *)ZALLOC( dest, ds->w_size, sizeof( Pos ) );
	ds->head   = (Pos *)ZALLOC( dest, ds->hash_size, sizeof( Pos ) );
	overlay = (ush *)ZALLOC( dest, ds->lit_bufsize, sizeof( ush ) + 2 );
	ds->pending_buf = (uch *)overlay;

	if ( ds->window == NULL || ds->prev == NULL || ds->head == NULL ||
		ds->pending_buf == NULL ) {
		deflateEnd( dest );
		return Z_MEM_ERROR;
	}
	/* following zmemcpy do not work for 16-bit MSDOS */
	memcpy( ds->window, ss->window, ds->w_size * 2 * sizeof( Byte ) );
	memcpy( ds->prev, ss->prev, ds->w_size * sizeof( Pos ) );
	memcpy( ds->head, ss->head, ds->hash_size * sizeof( Pos ) );
	memcpy( ds->pending_buf, ss->pending_buf, (uInt)ds->pending_buf_size );

	ds->pending_out = ds->pending_buf + ( ss->pending_out - ss->pending_buf );
	ds->d_buf = overlay + ds->lit_bufsize / sizeof( ush );
	ds->l_buf = ds->pending_buf + ( 1 + sizeof( ush ) ) * ds->lit_bufsize;

	ds->l_desc.dyn_tree = ds->dyn_ltree;
	ds->d_desc.dyn_tree = ds->dyn_dtree;
	ds->bl_desc.dyn_tree = ds->bl_tree;

	return Z_OK;
}

/* ---- deflate_read_buf  0x004A3990 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c read_buf */
unsigned int __cdecl deflate_read_buf( z_streamp strm, Byte *buf, unsigned int size )
{
  unsigned int len = strm->avail_in;

  if ( len > size )
    len = size;
  if ( !len )
    return 0;
  strm->avail_in -= len;
  if ( !strm->state->noheader )   /* only checksum when writing a header */
    strm->adler = adler32( strm->adler, strm->next_in, len );
  memcpy( buf, strm->next_in, len );
  strm->next_in  += len;
  strm->total_in += len;
  return len;
}

/* ---- lm_init  0x004A3A00 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
void __cdecl lm_init( deflate_state *s )
{
  int level;

  s->window_size = (ulg)2L * s->w_size;

  CLEAR_HASH( s );

  level = s->level;
  s->max_lazy_match   = deflate_configuration_table[level].max_lazy;
  s->good_match       = deflate_configuration_table[level].good_length;
  s->nice_match       = deflate_configuration_table[level].nice_length;
  s->max_chain_length = deflate_configuration_table[level].max_chain;

  s->strstart = 0;
  s->block_start = 0L;
  s->lookahead = 0;
  s->match_length = s->prev_length = MIN_MATCH - 1;
  s->match_available = 0;
  s->ins_h = 0;
}

/* ---- longest_match  0x004A3A90 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c (no ASM_OBJ, no UNALIGNED_OK) */
uInt __cdecl longest_match( deflate_state *s, IPos cur_match )
{
	unsigned chain_length = s->max_chain_length;/* max hash chain length */
	register Byte *scan = s->window + s->strstart; /* current string */
	register Byte *match;                       /* matched string */
	register int len;                           /* length of current match */
	int best_len = s->prev_length;              /* best match length so far */
	int nice_match = s->nice_match;             /* stop if match long enough */
	IPos limit = s->strstart > (IPos)MAX_DIST( s ) ?
		s->strstart - (IPos)MAX_DIST( s ) : NIL;
	/* Stop when cur_match becomes <= limit. To simplify the code,
	 * we prevent matches with the string of window index 0. */
	Pos *prev = s->prev;
	uInt wmask = s->w_mask;

	register Byte *strend = s->window + s->strstart + MAX_MATCH;
	register Byte scan_end1  = scan[best_len - 1];
	register Byte scan_end   = scan[best_len];

	/* Do not waste too much time if we already have a good match: */
	if ( s->prev_length >= s->good_match ) {
		chain_length >>= 2;
	}
	/* Do not look for matches beyond the end of the input. This is necessary
	 * to make deflate deterministic. */
	if ( (uInt)nice_match > s->lookahead ) nice_match = s->lookahead;

	do {
		match = s->window + cur_match;

		/* Skip to next match if the match length cannot increase
		 * or if the match length is less than 2: */
		if ( match[best_len]   != scan_end  ||
			match[best_len - 1] != scan_end1 ||
			*match            != *scan     ||
			*++match          != scan[1] )      continue;

		/* It is not necessary to compare scan[2] and match[2] since they
		 * are always equal when the other bytes match, given that
		 * the hash keys are equal and that HASH_BITS >= 8. */
		scan += 2, match++;

		/* We check for insufficient lookahead only every 8th comparison;
		 * the 256th check will be made at strstart+258. */
		do {
		} while ( *++scan == *++match && *++scan == *++match &&
			*++scan == *++match && *++scan == *++match &&
			*++scan == *++match && *++scan == *++match &&
			*++scan == *++match && *++scan == *++match &&
			scan < strend );

		len = MAX_MATCH - (int)( strend - scan );
		scan = strend - MAX_MATCH;

		if ( len > best_len ) {
			s->match_start = cur_match;
			best_len = len;
			if ( len >= nice_match ) break;
			scan_end1  = scan[best_len - 1];
			scan_end   = scan[best_len];
		}
	} while ( ( cur_match = prev[cur_match & wmask] ) > limit
		&& --chain_length != 0 );

	if ( (uInt)best_len <= s->lookahead ) return (uInt)best_len;
	return s->lookahead;
}

/* ---- deflate_fill_window  0x004A3C00 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c fill_window */
void __cdecl deflate_fill_window( deflate_state *s )
{
	register unsigned n, m;
	register Pos *p;
	unsigned more;    /* Amount of free space at the end of the window. */
	uInt wsize = s->w_size;

	do {
		more = (unsigned)( s->window_size - (ulg)s->lookahead - (ulg)s->strstart );

		/* Deal with !@#$% 64K limit: */
		if ( more == 0 && s->strstart == 0 && s->lookahead == 0 ) {
			more = wsize;

		} else if ( more == (unsigned)( -1 ) ) {
			/* Very unlikely, but possible on 16 bit machine if strstart == 0
			 * and lookahead == 1 (input done one byte at time) */
			more--;

		/* If the window is almost full and there is insufficient lookahead,
		 * move the upper half to the lower one to make room in the upper half. */
		} else if ( s->strstart >= wsize + MAX_DIST( s ) ) {

			memcpy( s->window, s->window + wsize, (unsigned)wsize );
			s->match_start -= wsize;
			s->strstart    -= wsize; /* we now have strstart >= MAX_DIST */
			s->block_start -= (long)wsize;

			/* Slide the hash table (could be avoided with 32 bit values
			 * at the expense of memory usage). */
			n = s->hash_size;
			p = &s->head[n];
			do {
				m = *--p;
				*p = (Pos)( m >= wsize ? m - wsize : NIL );
			} while ( --n );

			n = wsize;
			p = &s->prev[n];
			do {
				m = *--p;
				*p = (Pos)( m >= wsize ? m - wsize : NIL );
				/* If n is not on any hash chain, prev[n] is garbage but
				 * its value will never be used. */
			} while ( --n );
			more += wsize;
		}
		if ( s->strm->avail_in == 0 ) return;

		n = deflate_read_buf( s->strm, s->window + s->strstart + s->lookahead, more );
		s->lookahead += n;

		/* Initialize the hash value now that we have some input: */
		if ( s->lookahead >= MIN_MATCH ) {
			s->ins_h = s->window[s->strstart];
			UPDATE_HASH( s, s->ins_h, s->window[s->strstart + 1] );
		}
		/* If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
		 * but this is not important since only literal bytes will be emitted. */

	} while ( s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0 );
}

/* ---- deflate_stored  0x004A3D20 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
int __cdecl deflate_stored( deflate_state *s, int flush )
{
	/* Stored blocks are limited to 0xffff bytes, pending_buf is limited
	 * to pending_buf_size, and each stored block has a 5 byte header: */
	ulg max_block_size = 0xffff;
	ulg max_start;

	if ( max_block_size > s->pending_buf_size - 5 ) {
		max_block_size = s->pending_buf_size - 5;
	}

	/* Copy as much as possible from input to output: */
	for ( ;; ) {
		/* Fill the window as much as possible: */
		if ( s->lookahead <= 1 ) {

			deflate_fill_window( s );
			if ( s->lookahead == 0 && flush == Z_NO_FLUSH ) return need_more;

			if ( s->lookahead == 0 ) break; /* flush the current block */
		}

		s->strstart += s->lookahead;
		s->lookahead = 0;

		/* Emit a stored block if pending_buf will be full: */
		max_start = s->block_start + max_block_size;
		if ( s->strstart == 0 || (ulg)s->strstart >= max_start ) {
			/* strstart == 0 is possible when wraparound on 16-bit machine */
			s->lookahead = (uInt)( s->strstart - max_start );
			s->strstart = (uInt)max_start;
			FLUSH_BLOCK( s, 0 );
		}
		/* Flush if we may have to slide, otherwise block_start may become
		 * negative and the data will be gone: */
		if ( s->strstart - (uInt)s->block_start >= MAX_DIST( s ) ) {
			FLUSH_BLOCK( s, 0 );
		}
	}
	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}

/* ---- deflate_fast  0x004A3EE0 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
int __cdecl deflate_fast( deflate_state *s, int flush )
{
	IPos hash_head = NIL; /* head of the hash chain */
	int bflush;           /* set if current block must be flushed */

	for ( ;; ) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match. */
		if ( s->lookahead < MIN_LOOKAHEAD ) {
			deflate_fill_window( s );
			if ( s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH ) {
				return need_more;
			}
			if ( s->lookahead == 0 ) break; /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain: */
		if ( s->lookahead >= MIN_MATCH ) {
			INSERT_STRING( s, s->strstart, hash_head );
		}

		/* Find the longest match, discarding those <= prev_length.
		 * At this point we have always match_length < MIN_MATCH */
		if ( hash_head != NIL && s->strstart - hash_head <= MAX_DIST( s ) ) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file). */
			if ( s->strategy != Z_HUFFMAN_ONLY ) {
				s->match_length = longest_match( s, hash_head );
			}
			/* longest_match() sets match_start */
		}
		if ( s->match_length >= MIN_MATCH ) {
			_tr_tally_dist( s, s->strstart - s->match_start,
				s->match_length - MIN_MATCH, bflush );

			s->lookahead -= s->match_length;

			/* Insert new strings in the hash table only if the match length
			 * is not too large. This saves time but degrades compression. */
			if ( s->match_length <= s->max_lazy_match &&
				s->lookahead >= MIN_MATCH ) {
				s->match_length--; /* string at strstart already in hash table */
				do {
					s->strstart++;
					INSERT_STRING( s, s->strstart, hash_head );
					/* strstart never exceeds WSIZE-MAX_MATCH, so there are
					 * always MIN_MATCH bytes ahead. */
				} while ( --s->match_length != 0 );
				s->strstart++;
			} else {
				s->strstart += s->match_length;
				s->match_length = 0;
				s->ins_h = s->window[s->strstart];
				UPDATE_HASH( s, s->ins_h, s->window[s->strstart + 1] );
				/* If lookahead < MIN_MATCH, ins_h is garbage, but it does not
				 * matter since it will be recomputed at next deflate call. */
			}
		} else {
			/* No match, output a literal byte */
			_tr_tally_lit( s, s->window[s->strstart], bflush );
			s->lookahead--;
			s->strstart++;
		}
		if ( bflush ) FLUSH_BLOCK( s, 0 );
	}
	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}

/* ---- deflate_slow  0x004A41E0 ---- CONFIRMED */
/* zlib 1.1.4 deflate.c */
int __cdecl deflate_slow( deflate_state *s, int flush )
{
	IPos hash_head = NIL;    /* head of hash chain */
	int bflush;              /* set if current block must be flushed */

	/* Process the input block. */
	for ( ;; ) {
		/* Make sure that we always have enough lookahead, except
		 * at the end of the input file. We need MAX_MATCH bytes
		 * for the next match, plus MIN_MATCH bytes to insert the
		 * string following the next match. */
		if ( s->lookahead < MIN_LOOKAHEAD ) {
			deflate_fill_window( s );
			if ( s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH ) {
				return need_more;
			}
			if ( s->lookahead == 0 ) break; /* flush the current block */
		}

		/* Insert the string window[strstart .. strstart+2] in the
		 * dictionary, and set hash_head to the head of the hash chain: */
		if ( s->lookahead >= MIN_MATCH ) {
			INSERT_STRING( s, s->strstart, hash_head );
		}

		/* Find the longest match, discarding those <= prev_length. */
		s->prev_length = s->match_length, s->prev_match = s->match_start;
		s->match_length = MIN_MATCH - 1;

		if ( hash_head != NIL && s->prev_length < s->max_lazy_match &&
			s->strstart - hash_head <= MAX_DIST( s ) ) {
			/* To simplify the code, we prevent matches with the string
			 * of window index 0 (in particular we have to avoid a match
			 * of the string with itself at the start of the input file). */
			if ( s->strategy != Z_HUFFMAN_ONLY ) {
				s->match_length = longest_match( s, hash_head );
			}
			/* longest_match() sets match_start */

			if ( s->match_length <= 5 && ( s->strategy == Z_FILTERED ||
				( s->match_length == MIN_MATCH &&
				  s->strstart - s->match_start > TOO_FAR ) ) ) {

				/* If prev_match is also MIN_MATCH, match_start is garbage
				 * but we will ignore the current match anyway. */
				s->match_length = MIN_MATCH - 1;
			}
		}
		/* If there was a match at the previous step and the current
		 * match is not better, output the previous match: */
		if ( s->prev_length >= MIN_MATCH && s->match_length <= s->prev_length ) {
			uInt max_insert = s->strstart + s->lookahead - MIN_MATCH;
			/* Do not insert strings in hash table beyond this. */

			_tr_tally_dist( s, s->strstart - 1 - s->prev_match,
				s->prev_length - MIN_MATCH, bflush );

			/* Insert in hash table all strings up to the end of the match.
			 * strstart-1 and strstart are already inserted. If there is not
			 * enough lookahead, the last two strings are not inserted in
			 * the hash table. */
			s->lookahead -= s->prev_length - 1;
			s->prev_length -= 2;
			do {
				if ( ++s->strstart <= max_insert ) {
					INSERT_STRING( s, s->strstart, hash_head );
				}
			} while ( --s->prev_length != 0 );
			s->match_available = 0;
			s->match_length = MIN_MATCH - 1;
			s->strstart++;

			if ( bflush ) FLUSH_BLOCK( s, 0 );

		} else if ( s->match_available ) {
			/* If there was no match at the previous position, output a
			 * single literal. If there was a match but the current match
			 * is longer, truncate the previous match to a single literal. */
			_tr_tally_lit( s, s->window[s->strstart - 1], bflush );
			if ( bflush ) {
				FLUSH_BLOCK_ONLY( s, 0 );
			}
			s->strstart++;
			s->lookahead--;
			if ( s->strm->avail_out == 0 ) return need_more;
		} else {
			/* There is no previous match to compare with, wait for
			 * the next step to decide. */
			s->match_available = 1;
			s->strstart++;
			s->lookahead--;
		}
	}
	if ( s->match_available ) {
		_tr_tally_lit( s, s->window[s->strstart - 1], bflush );
		s->match_available = 0;
	}
	FLUSH_BLOCK( s, flush == Z_FINISH );
	return flush == Z_FINISH ? finish_done : block_done;
}


#ifdef __cplusplus
}
#endif
