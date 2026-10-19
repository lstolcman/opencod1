/*
 * Machine-translated from Call of Duty 1.1 (Windows, CoDMP.exe).
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/zlib/inflate.c
 *
 * Retail range 0x004A45C0-0x004A8730, 44 functions: 44 translated, 0 stubbed.
 *
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * `@fidelity-default: unreviewed` ADDED: this unit's per-function confidence
 * is not uniform (see SECOND PASS below) and the file-level `likely` was
 * being inherited by every un-banded function, including the trees.c/deflate
 * half that is explicitly UNVERIFIED and dead.  `_tr_flush_block` (0x004A7EF0,
 * deflate-side, unreachable -- see the dead-code note below) was flagged by
 * uninit_scan.py as "claims likely, does not compile clean" purely because
 * of this inherited default; the C4700 there was real (an un-banded raw body;
 * it is zlib's text now and compiles clean) but the confidence claim
 * was spurious.  The 22 inflate-side functions are unaffected: each carries
 * its own inline `VERIFIED` band, which overrides this default.
 *
 * TAKEN OUT OF THE GENERATOR DELIBERATELY.  Most of this unit is still raw
 * machine output, but the inflate side of it has been reconstructed by hand and
 * regenerating the file would silently put the server back to reading zeroes
 * out of every pak.  What was done, and must survive:
 *
 *   - inflate_blocks (0x004A46C0), inflate_codes (0x004A5300) and inflate_fast
 *     (0x004A5A10) were AUTO-STUBBED `return 0;`.  That is the entire deflate
 *     decoder, so inflate() returned Z_OK having produced nothing and every
 *     FS_ReadFile out of a pak came back zero-filled.  Their recovered bodies
 *     are now live.
 *   - the six zlib constant tables they need (inflate_mask, inflate_border,
 *     cplens, cplext, cpdist, cpdext) are defined below with real types.  The
 *     global generator saw them as `int inflate_border;` / `unsigned char
 *     zlib_cplens;` -- one scalar each -- which is both why the three bodies
 *     would not compile and why huft_build would have been fed all-zero
 *     length/distance bases even if they had.
 *   - huft_build's two split stack arrays were restored (since superseded:
 *     huft_build is now zlib 1.1.4's own text, see the note in it).
 *   - inflate_trees_fixed was rewritten: the recovered form stores through an
 *     uninitialised pointer (a lost ECX argument) and hands back the zeroed
 *     placeholders for the two 4KB fixed-Huffman tables.
 *
 * Verified by decompressing 1,040 entries across pak0 and pak5 and comparing
 * byte-for-byte against Python's zipfile -- all three deflate block types
 * (stored, fixed, dynamic), sizes from 10 bytes to 6.1 MB.
 *
 * ---------------------------------------------------------------------------
 * SECOND PASS -- VERIFICATION.  Every inflate-side function now carries a
 * `VERIFIED <addr>` note saying what was actually checked.  Three kinds of
 * evidence, and it is worth knowing which is which:
 *
 *  1. BEHAVIOURAL, and now much wider than the run above: all NINE stock
 *     retail paks, 14,493 non-empty entries, read through this file plus
 *     zlib/unzip.c and zlib/unz_compat.c in the exact call sequence
 *     universal/com_files.c uses, every one matching its central-directory
 *     CRC.  Sizes 1 byte to 45 MB.  This is the first pass in which the CRC
 *     comparison inside unzCloseCurrentFile has actually run -- crc32
 *     (0x004A2CE0) was an AUTO-STUB returning 0 until today -- and it now
 *     passes on every entry.
 *
 *  2. MECHANICAL TABLE DIFFS, because this file is mostly constant tables and
 *     eyeballing them is not verification:
 *       - the six tables below (inflate_mask, inflate_border, cplens, cplext,
 *         cpdist, cpdext) were read back out of retail .data and compared
 *         element by element against upstream zlib 1.1.4.  All six are
 *         identical in all three places: retail == upstream == this file.
 *       - the FIXED-HUFFMAN TREES that huft_build constructs were dumped
 *         and compared against retail's precompiled fixed_tl (0x00571C58,
 *         4096 bytes) and fixed_td (0x00572C58, 256 bytes).  SHA-1 identical
 *         over all 4,352 bytes, bl/bd = 9/5 matching 0x00571C4C/0x00571C50.
 *         That verifies huft_build itself, not just the tables.
 *
 *  3. LINE-BY-LINE against upstream zlib 1.1.4 for the small functions and
 *     for inflate_flush.
 *
 * ONE REAL DEFECT FIXED: huft_build read an uninitialised local (`v33 = v65`).
 * It was a faithful rendering of upstream's uninitialised `inflate_huft r`;
 * huft_build is now upstream's text, with r.base pinned to 0 in the modern
 * build only (the VC7 build keeps retail's code), no behaviour change (the
 * fixed trees are byte-identical before and after).
 *
 * VC7 MATCHING PASS: every function in this unit except _tr_flush_block
 * compiles to retail's bytes; the ones that did not are now written as the
 * zlib 1.1.4 text (inflate.c, infblock.c, infcodes.c, inffast.c, inftrees.c,
 * infutil.c, trees.c).
 *
 * NAMING PASS (2026-10-14): the structures are now zlib's own types
 * (z_stream, internal_state, inflate_blocks_statef, inflate_codes_statef,
 * inflate_huft, deflate_state, ct_data, tree_desc -- zlib/zlib_types.h), with
 * zlib's parameter/local names and mode enums in place of Hex-Rays' a1/v12
 * and `*(_DWORD *)(a1 + 0x1C)`.  Byte-identical to the previous source under
 * VC7.  Argument ORDERS that follow retail's registers rather than zlib's are
 * kept and noted at each definition.
 *
 * WHAT IS *NOT* VERIFIED: everything from 0x004A6D40 on.  That block is not
 * inflate at all -- it is zlib's trees.c (_tr_init, _tr_stored_block,
 * _tr_align, _tr_flush_block, _tr_tally and their statics), the COMPRESSION
 * side, emitted into this unit by the generator.  It is dead in the retail
 * binary: its only entry is compress() at 0x004A2CC0, which nothing calls.
 * Left alone deliberately -- see the note on _tr_init below.
 *
 * WHICH IS WHY THE MARKER ABOVE STILL SAYS `likely` AND NOT `verified`.  It is
 * per-unit, and this unit is two units: the inflate half (0x004A45C0 through
 * 0x004A6C00, 22 functions, all carrying a VERIFIED note) and the trees.c half
 * (0x004A6D40 through 0x004A8730, unchecked).  The five trees.c functions that
 * were AUTO-STUBBED (build_bl_tree, send_all_trees, _tr_tally, compress_block,
 * copy_block) and inflateSync now have hand-written bodies, read against the
 * retail disassembly and zlib 1.1.4 -- see `IMPLEMENTED (retail)` at each.
 * They are still dead and still unexercised.
 * Read the marker as the weaker of the two, not as a verdict on the first.
 * ---------------------------------------------------------------------------
 *
 * THIS FILE WAS NOT WRITTEN BY HAND.  It is Hex-Rays output with the
 * dialect mechanically rewritten.  It exists so the tree links
 * and so each function has a starting point -- not because it is right.
 *
 * Before trusting any function in here:
 *   - __usercall/__userpurge were rewritten to __cdecl.  Register argument
 *     ORDER in the recovered prototype is frequently wrong.
 *   - accesses through untyped pointers kept their offsets but the widths
 *     are IDA guesses.
 *   - a function replaced by a stub is marked as such at its site.
 *
 * Replace functions here with hand reconstructions one at a time; drop the
 * @fidelity marker to "likely" or "verified" as you go.
 */

#include "../qcommon/qcommon.h"
#include "zlib_types.h"

/*
 * ==========================================================================
 * EVERY FORMER STUB IN THIS UNIT IS DEAD CODE, 2026-08-19.  Verified, not
 * assumed.
 * ==========================================================================
 *
 * UPDATE, retail-fidelity pass: the owner asked for dead retail functions to
 * carry their real bodies too, so the six stubs this note covers have now been
 * written by hand from the retail disassembly (cross-checked against zlib
 * 1.1.4 trees.c / inflate.c) and are marked `IMPLEMENTED (retail)`.  Nothing
 * below changes: they are still unreachable, and still unexercised.
 *
 * CoDMP.exe links all of zlib but only ever INFLATES -- it reads .iwd/.pk3
 * archives and never writes one.  The whole deflate/trees half is therefore
 * unreachable, and the five `sub_4A*` functions plus inflateSync are all in it.
 *
 * The call graph terminates, which is the proof:
 *
 *     build_bl_tree, send_all_trees, compress_block  <- _tr_flush_block
 *     copy_block                          <- _tr_stored_block
 *     _tr_flush_block   <- deflate_stored, deflate_slow, deflate_fast
 *     _tr_stored_block  <- deflate, _tr_flush_block
 *     deflate_stored    <- deflateParams, deflate
 *     deflate           <- compress2  <- compress  <- NOTHING
 *
 * and deflate_slow, deflate_fast and deflateParams have no callers at all.
 *
 * CHECKED FOR THE INLINING TRAP TOO, because "zero xrefs" does NOT mean dead
 * in this tree -- six EffectsCore functions with no xrefs turned out to be
 * live and inlined.  A scan of every loaded segment for little-endian pointer
 * literals finds none to compress, compress2, deflate, deflateEnd,
 * deflateInit_, deflateInit2_, deflateParams, _tr_tally or inflateSync.
 * deflate_fast and deflate_slow DO appear, at 0x0054136C..0x005413A8 -- but
 * that is zlib's own `configuration_table`, one entry per compression level,
 * and the only thing that walks it is deflate(), which nothing calls.  A
 * pointer in a table a dead function reads does not make its target live.
 *
 * _tr_tally has neither xrefs nor pointer literals: its uses are inlined into
 * deflate_fast and deflate_slow (universal/memorytree.cpp), which are
 * themselves dead.  The out-of-line copy at 0x004A80D0 is never called.
 *
 * inflateSync is a public zlib entry point that this image never calls.
 *
 * zlib is third-party under CLAUDE.md 4 in any case; if a future build ever
 * needs the deflate half, PORT IT FROM THE ZLIB RELEASE rather than
 * decompiling it -- the source is public and permissive, and that is the same
 * rule that applies to the RTCW/Q3 code elsewhere in this tree.
 */


/* Functions this unit calls that live elsewhere.  Declared
 * unprototyped and int-returning: C89 makes that compatible with any
 * definition, and each unit is compiled on its own, so this cannot
 * conflict with the real signature in the defining unit.  Anything a
 * real header already declares is excluded. */
/* REAL prototypes, not `extern int f();`.  An empty parameter list switches
 * argument checking off entirely and has produced nine faults in this tree.
 * All three are only ever address-taken here (stored into z_stream.zalloc /
 * .zfree and into inflate_blocks_state.checkfn), but the declarations are
 * still written out so a future call site is checked.
 *
 *   adler32 (0x004A2AF0, universal/memorytree.cpp), called
 *                through checkfn as (check, buf, len) -- see inflate_flush.
 *   zcalloc/zcfree are defined in zlib/unzip.c; the signatures below are
 *                that file's, exactly. */
extern int  __cdecl adler32( unsigned int adler, unsigned char *buf, unsigned int len );
extern int *__cdecl zcalloc( int opaque, int items, int size );
extern void __cdecl zcfree( int opaque, void *block );

/* Defined below at its address-order place, declared here because its call
 * site in _tr_init comes first in the file.  Without this it would be an
 * implicit declaration, which decorates as plain `_sub_4A6DB0` and would have
 * kept binding to the link stub -- the silent shadow. */
short *__cdecl init_block( deflate_state *s );

/* trees.c statics that were AUTO-STUBBED until the retail-fidelity pass, and
 * _tr_tally / inflateSync.  Retail passes the deflate_state in EAX (inflateSync:
 * the z_stream in ESI); these __cdecl spellings put it first, in the order the
 * call sites in _tr_flush_block and _tr_stored_block already pass it.
 * compress_block and copy_block are called before their definitions. */
int    __cdecl build_bl_tree( deflate_state *s );
deflate_state *__cdecl send_all_trees( deflate_state *s, int lcodes, int dcodes, int blcodes );
void   __cdecl compress_block( deflate_state *s, const ct_data *ltree, const ct_data *dtree );
deflate_state *__cdecl copy_block( deflate_state *s, char *buf, unsigned len, int header );
int    __cdecl _tr_tally( deflate_state *s, unsigned dist, unsigned lc );
int    __cdecl inflateSync( z_streamp z );
unsigned int __fastcall bi_reverse( unsigned code, int len );
void   __cdecl set_data_type( deflate_state *s );
deflate_state *__cdecl bi_flush( deflate_state *s );
deflate_state *__cdecl bi_windup( deflate_state *s );

/*
 * ---------------------------------------------------------------------------
 * zlib 1.1.4 constant tables.
 *
 * These live in retail .data and the recovered code referenced them by
 * address: inflate_mask at 0x00571C08, border at 0x005412D0, cplens/cplext at
 * 0x005410D8/0x00541158, cpdist/cpdext at 0x005411D8/0x00541250.  The global
 * generator has no way to know a `inflate_border` is nineteen elements wide, so
 * it emitted each as a single zeroed scalar -- which is why the three block
 * decoders below could not even compile (`subscript requires array or pointer
 * type`) and why huft_build, had they compiled, would have been handed all-zero
 * length/distance bases.
 *
 * Every value here was read back out of the loaded image (idc.get_wide_dword
 * over each range) and matches zlib 1.1.4's inftrees.c / infutil.c exactly.
 * ---------------------------------------------------------------------------
 */

/* infutil.c: inflate_mask[n] = low n bits set.  Retail 0x00571C08. */
static const unsigned int inflate_mask[17] = {
	0x00000000u, 0x00000001u, 0x00000003u, 0x00000007u,
	0x0000000fu, 0x0000001fu, 0x0000003fu, 0x0000007fu,
	0x000000ffu, 0x000001ffu, 0x000003ffu, 0x000007ffu,
	0x00000fffu, 0x00001fffu, 0x00003fffu, 0x00007fffu,
	0x0000ffffu
};

/* infblock.c: order of the bit length code lengths.  Retail 0x005412D0. */
static const unsigned int inflate_border[19] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* inftrees.c: copy lengths and their extra bits.  Retail 0x005410D8/0x00541158.
 * The trailing 0,112,112 in cplext are zlib's "invalid code" markers. */
static const unsigned int cplens[31] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
	35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};
static const unsigned int cplext[31] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112
};

/* inftrees.c: copy offsets and their extra bits.  Retail 0x005411D8/0x00541250. */
static const unsigned int cpdist[30] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49,
	65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073,
	4097, 6145, 8193, 12289, 16385, 24577
};
static const unsigned int cpdext[30] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4,
	5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
	11, 11, 12, 12, 13, 13
};

/* inflate.c: inflateSync's `static const Byte mark[4]`, the 00 00 FF FF of an
 * empty stored block.  Retail 0x005412C8, between cpdext and inflate_border;
 * read out of the retail PE's .rdata (00 00 FF FF, then 4 bytes of padding).
 * Its only reader
 * is inflateSync (0x004A63D0, `cmp dl, zlib_mark[eax]`), a public entry point
 * retail never calls -- see above. */
static const unsigned char zlib_mark[4] = { 0x00, 0x00, 0xff, 0xff };

/*
 * The fixed-Huffman trees: zlib 1.1.4's inffixed.h, compiled into retail as
 * initialised .data -- fixed_bl/fixed_bd at 0x00571C4C/0x00571C50 (9 and 5),
 * fixed_tl at 0x00571C58 (512 inflate_hufts, 0x1000 bytes) and fixed_td at
 * 0x00572C58 (32, 0x100 bytes).  inflate_trees_fixed (0x004A6BD0) hands back
 * exactly these four, and inflate_blocks' fixed-block case (0x004A47AB) pushes
 * them straight to inflate_codes_new; retail never builds them at run time.
 *
 * The global generator flattened the two tables to one zero byte each, so an
 * earlier pass rebuilt them on first use with huft_build (zlib's BUILDFIXED
 * path).  That output was SHA-1 identical, but it was an extra run-time step
 * retail does not have; the entries below are now read back out of the retail
 * PE's .data (VA 0x00571C58..0x00572D58) instead, all 544 of them, including
 * zlib's {192,5,24577} invalid-code fillers in fixed_td.
 *
 * inflate_huft (zlib_types.h) is { Byte exop; Byte bits; (2 bytes pad); uInt
 * base; } -- 8 bytes, padding zero in the image.  Non-const, as in inffixed.h
 * (`local`).
 */
static unsigned int   fixed_bl = 9;
static unsigned int   fixed_bd = 5;
static inflate_huft fixed_tl[512] = {
	{96,7,256}, {0,8,80}, {0,8,16}, {84,8,115}, {82,7,31}, {0,8,112},
	{0,8,48}, {0,9,192}, {80,7,10}, {0,8,96}, {0,8,32}, {0,9,160},
	{0,8,0}, {0,8,128}, {0,8,64}, {0,9,224}, {80,7,6}, {0,8,88},
	{0,8,24}, {0,9,144}, {83,7,59}, {0,8,120}, {0,8,56}, {0,9,208},
	{81,7,17}, {0,8,104}, {0,8,40}, {0,9,176}, {0,8,8}, {0,8,136},
	{0,8,72}, {0,9,240}, {80,7,4}, {0,8,84}, {0,8,20}, {85,8,227},
	{83,7,43}, {0,8,116}, {0,8,52}, {0,9,200}, {81,7,13}, {0,8,100},
	{0,8,36}, {0,9,168}, {0,8,4}, {0,8,132}, {0,8,68}, {0,9,232},
	{80,7,8}, {0,8,92}, {0,8,28}, {0,9,152}, {84,7,83}, {0,8,124},
	{0,8,60}, {0,9,216}, {82,7,23}, {0,8,108}, {0,8,44}, {0,9,184},
	{0,8,12}, {0,8,140}, {0,8,76}, {0,9,248}, {80,7,3}, {0,8,82},
	{0,8,18}, {85,8,163}, {83,7,35}, {0,8,114}, {0,8,50}, {0,9,196},
	{81,7,11}, {0,8,98}, {0,8,34}, {0,9,164}, {0,8,2}, {0,8,130},
	{0,8,66}, {0,9,228}, {80,7,7}, {0,8,90}, {0,8,26}, {0,9,148},
	{84,7,67}, {0,8,122}, {0,8,58}, {0,9,212}, {82,7,19}, {0,8,106},
	{0,8,42}, {0,9,180}, {0,8,10}, {0,8,138}, {0,8,74}, {0,9,244},
	{80,7,5}, {0,8,86}, {0,8,22}, {192,8,0}, {83,7,51}, {0,8,118},
	{0,8,54}, {0,9,204}, {81,7,15}, {0,8,102}, {0,8,38}, {0,9,172},
	{0,8,6}, {0,8,134}, {0,8,70}, {0,9,236}, {80,7,9}, {0,8,94},
	{0,8,30}, {0,9,156}, {84,7,99}, {0,8,126}, {0,8,62}, {0,9,220},
	{82,7,27}, {0,8,110}, {0,8,46}, {0,9,188}, {0,8,14}, {0,8,142},
	{0,8,78}, {0,9,252}, {96,7,256}, {0,8,81}, {0,8,17}, {85,8,131},
	{82,7,31}, {0,8,113}, {0,8,49}, {0,9,194}, {80,7,10}, {0,8,97},
	{0,8,33}, {0,9,162}, {0,8,1}, {0,8,129}, {0,8,65}, {0,9,226},
	{80,7,6}, {0,8,89}, {0,8,25}, {0,9,146}, {83,7,59}, {0,8,121},
	{0,8,57}, {0,9,210}, {81,7,17}, {0,8,105}, {0,8,41}, {0,9,178},
	{0,8,9}, {0,8,137}, {0,8,73}, {0,9,242}, {80,7,4}, {0,8,85},
	{0,8,21}, {80,8,258}, {83,7,43}, {0,8,117}, {0,8,53}, {0,9,202},
	{81,7,13}, {0,8,101}, {0,8,37}, {0,9,170}, {0,8,5}, {0,8,133},
	{0,8,69}, {0,9,234}, {80,7,8}, {0,8,93}, {0,8,29}, {0,9,154},
	{84,7,83}, {0,8,125}, {0,8,61}, {0,9,218}, {82,7,23}, {0,8,109},
	{0,8,45}, {0,9,186}, {0,8,13}, {0,8,141}, {0,8,77}, {0,9,250},
	{80,7,3}, {0,8,83}, {0,8,19}, {85,8,195}, {83,7,35}, {0,8,115},
	{0,8,51}, {0,9,198}, {81,7,11}, {0,8,99}, {0,8,35}, {0,9,166},
	{0,8,3}, {0,8,131}, {0,8,67}, {0,9,230}, {80,7,7}, {0,8,91},
	{0,8,27}, {0,9,150}, {84,7,67}, {0,8,123}, {0,8,59}, {0,9,214},
	{82,7,19}, {0,8,107}, {0,8,43}, {0,9,182}, {0,8,11}, {0,8,139},
	{0,8,75}, {0,9,246}, {80,7,5}, {0,8,87}, {0,8,23}, {192,8,0},
	{83,7,51}, {0,8,119}, {0,8,55}, {0,9,206}, {81,7,15}, {0,8,103},
	{0,8,39}, {0,9,174}, {0,8,7}, {0,8,135}, {0,8,71}, {0,9,238},
	{80,7,9}, {0,8,95}, {0,8,31}, {0,9,158}, {84,7,99}, {0,8,127},
	{0,8,63}, {0,9,222}, {82,7,27}, {0,8,111}, {0,8,47}, {0,9,190},
	{0,8,15}, {0,8,143}, {0,8,79}, {0,9,254}, {96,7,256}, {0,8,80},
	{0,8,16}, {84,8,115}, {82,7,31}, {0,8,112}, {0,8,48}, {0,9,193},
	{80,7,10}, {0,8,96}, {0,8,32}, {0,9,161}, {0,8,0}, {0,8,128},
	{0,8,64}, {0,9,225}, {80,7,6}, {0,8,88}, {0,8,24}, {0,9,145},
	{83,7,59}, {0,8,120}, {0,8,56}, {0,9,209}, {81,7,17}, {0,8,104},
	{0,8,40}, {0,9,177}, {0,8,8}, {0,8,136}, {0,8,72}, {0,9,241},
	{80,7,4}, {0,8,84}, {0,8,20}, {85,8,227}, {83,7,43}, {0,8,116},
	{0,8,52}, {0,9,201}, {81,7,13}, {0,8,100}, {0,8,36}, {0,9,169},
	{0,8,4}, {0,8,132}, {0,8,68}, {0,9,233}, {80,7,8}, {0,8,92},
	{0,8,28}, {0,9,153}, {84,7,83}, {0,8,124}, {0,8,60}, {0,9,217},
	{82,7,23}, {0,8,108}, {0,8,44}, {0,9,185}, {0,8,12}, {0,8,140},
	{0,8,76}, {0,9,249}, {80,7,3}, {0,8,82}, {0,8,18}, {85,8,163},
	{83,7,35}, {0,8,114}, {0,8,50}, {0,9,197}, {81,7,11}, {0,8,98},
	{0,8,34}, {0,9,165}, {0,8,2}, {0,8,130}, {0,8,66}, {0,9,229},
	{80,7,7}, {0,8,90}, {0,8,26}, {0,9,149}, {84,7,67}, {0,8,122},
	{0,8,58}, {0,9,213}, {82,7,19}, {0,8,106}, {0,8,42}, {0,9,181},
	{0,8,10}, {0,8,138}, {0,8,74}, {0,9,245}, {80,7,5}, {0,8,86},
	{0,8,22}, {192,8,0}, {83,7,51}, {0,8,118}, {0,8,54}, {0,9,205},
	{81,7,15}, {0,8,102}, {0,8,38}, {0,9,173}, {0,8,6}, {0,8,134},
	{0,8,70}, {0,9,237}, {80,7,9}, {0,8,94}, {0,8,30}, {0,9,157},
	{84,7,99}, {0,8,126}, {0,8,62}, {0,9,221}, {82,7,27}, {0,8,110},
	{0,8,46}, {0,9,189}, {0,8,14}, {0,8,142}, {0,8,78}, {0,9,253},
	{96,7,256}, {0,8,81}, {0,8,17}, {85,8,131}, {82,7,31}, {0,8,113},
	{0,8,49}, {0,9,195}, {80,7,10}, {0,8,97}, {0,8,33}, {0,9,163},
	{0,8,1}, {0,8,129}, {0,8,65}, {0,9,227}, {80,7,6}, {0,8,89},
	{0,8,25}, {0,9,147}, {83,7,59}, {0,8,121}, {0,8,57}, {0,9,211},
	{81,7,17}, {0,8,105}, {0,8,41}, {0,9,179}, {0,8,9}, {0,8,137},
	{0,8,73}, {0,9,243}, {80,7,4}, {0,8,85}, {0,8,21}, {80,8,258},
	{83,7,43}, {0,8,117}, {0,8,53}, {0,9,203}, {81,7,13}, {0,8,101},
	{0,8,37}, {0,9,171}, {0,8,5}, {0,8,133}, {0,8,69}, {0,9,235},
	{80,7,8}, {0,8,93}, {0,8,29}, {0,9,155}, {84,7,83}, {0,8,125},
	{0,8,61}, {0,9,219}, {82,7,23}, {0,8,109}, {0,8,45}, {0,9,187},
	{0,8,13}, {0,8,141}, {0,8,77}, {0,9,251}, {80,7,3}, {0,8,83},
	{0,8,19}, {85,8,195}, {83,7,35}, {0,8,115}, {0,8,51}, {0,9,199},
	{81,7,11}, {0,8,99}, {0,8,35}, {0,9,167}, {0,8,3}, {0,8,131},
	{0,8,67}, {0,9,231}, {80,7,7}, {0,8,91}, {0,8,27}, {0,9,151},
	{84,7,67}, {0,8,123}, {0,8,59}, {0,9,215}, {82,7,19}, {0,8,107},
	{0,8,43}, {0,9,183}, {0,8,11}, {0,8,139}, {0,8,75}, {0,9,247},
	{80,7,5}, {0,8,87}, {0,8,23}, {192,8,0}, {83,7,51}, {0,8,119},
	{0,8,55}, {0,9,207}, {81,7,15}, {0,8,103}, {0,8,39}, {0,9,175},
	{0,8,7}, {0,8,135}, {0,8,71}, {0,9,239}, {80,7,9}, {0,8,95},
	{0,8,31}, {0,9,159}, {84,7,99}, {0,8,127}, {0,8,63}, {0,9,223},
	{82,7,27}, {0,8,111}, {0,8,47}, {0,9,191}, {0,8,15}, {0,8,143},
	{0,8,79}, {0,9,255}
};
static inflate_huft fixed_td[32] = {
	{80,5,1}, {87,5,257}, {83,5,17}, {91,5,4097}, {81,5,5}, {89,5,1025},
	{85,5,65}, {93,5,16385}, {80,5,3}, {88,5,513}, {84,5,33}, {92,5,8193},
	{82,5,9}, {90,5,2049}, {86,5,129}, {192,5,24577}, {80,5,2}, {87,5,385},
	{83,5,25}, {91,5,6145}, {81,5,7}, {89,5,1537}, {85,5,97}, {93,5,24577},
	{80,5,4}, {88,5,769}, {84,5,49}, {92,5,12289}, {82,5,13}, {90,5,3073},
	{86,5,193}, {192,5,24577}
};

/*
 * ---------------------------------------------------------------------------
 * The zlib structures are in zlib_types.h; below are infutil.h's local-state
 * macros, written as upstream uses them.  Retail offsets, for reading the
 * disassembly:
 *
 * z_stream: next_in 0, avail_in 4, total_in 8, next_out 12, avail_out 16,
 * total_out 20, msg 24, state 28, zalloc 32, zfree 36, opaque 40, adler 48.
 * inflate_blocks_state: mode 0, sub 4..23, last 24, bitk 28, bitb 32,
 * hufts 36, window 40, end 44, read 48, write 52, checkfn 56, check 60.
 * ---------------------------------------------------------------------------
 */
/*
 * Functions defined further down (in retail's address order) but called
 * before their definitions.  The argument orders are this tree's, not
 * zlib's, where noted at each definition: they follow retail's register
 * assignment.
 */
int __cdecl inflate_flush(inflate_blocks_statef *s, z_streamp z, int r);
int __cdecl inflate_fast(uInt bl, uInt bd, inflate_huft *tl, inflate_huft *td,
                         inflate_blocks_statef *s, z_streamp z);
int __cdecl inflate_trees_fixed(uInt *bl, uInt *bd, inflate_huft **tl, inflate_huft **td);
int __cdecl inflate_trees_bits(z_streamp z, uInt *c, uInt *bb, inflate_huft **tb,
                               inflate_huft *hp);
int __cdecl inflate_trees_dynamic(z_streamp z, uInt nl, uInt nd, uInt *c, uInt *bl,
                                  uInt *bd, inflate_huft **tl, inflate_huft **td,
                                  inflate_huft *hp);
int __cdecl inflate_codes(inflate_blocks_statef *s, z_streamp z, int r);
inflate_codes_statef *__cdecl inflate_codes_new(z_streamp z, uInt bl, uInt bd,
                                                inflate_huft *tl, inflate_huft *td);
void __cdecl inflate_codes_free(z_streamp z, inflate_codes_statef *c);

/* infutil.h */
/*   update pointers and return */
#define UPDBITS {s->bitb=b;s->bitk=k;}
#define UPDIN {z->avail_in=n;z->total_in+=p-z->next_in;z->next_in=p;}
#define UPDOUT {s->write=q;}
#define UPDATE {UPDBITS UPDIN UPDOUT}
#define LEAVE {UPDATE return inflate_flush(s,z,r);}
/*   get bytes and bits */
#define LOADIN {p=z->next_in;n=z->avail_in;b=s->bitb;k=s->bitk;}
#define NEEDBYTE {if(n)r=Z_OK;else LEAVE}
#define NEXTBYTE (n--,*p++)
#define NEEDBITS(j) {while(k<(j)){NEEDBYTE;b|=((uLong)NEXTBYTE)<<k;k+=8;}}
#define DUMPBITS(j) {b>>=(j);k-=(j);}
/*   output bytes */
#define WAVAIL (uInt)(q<s->read?s->read-q-1:s->end-q)
#define LOADOUT {q=s->write;m=(uInt)WAVAIL;}
#define WRAP {if(q==s->end&&s->read!=s->window){q=s->window;m=(uInt)WAVAIL;}}
#define FLUSH {UPDOUT r=inflate_flush(s,z,r); LOADOUT}
#define NEEDOUT {if(m==0){WRAP if(m==0){FLUSH WRAP if(m==0) LEAVE}}r=Z_OK;}
#define OUTBYTE(a) {*q++=(Byte)(a);m--;}
/*   load local pointers */
#define LOAD {LOADIN LOADOUT}

/* ---- inflate_blocks_reset  0x004A45C0 ---- */
// inflate_blocks_reset -- CONFIRMED 92. zlib 1.1.4 infblock.c. Frees the codes/trees for modes 4/5/6 then resets bitb/bitk/read/write. Callers: inflate_blocks_new, inflateReset, inflate.
/*
 * VERIFIED 0x004A45C0 against zlib 1.1.4 infblock.c inflate_blocks_reset.
 * Every offset checked against inflate_blocks_state: mode 0, sub.trees.blens
 * 12, sub.decode.codes 4, last 24, bitk 28, bitb 32, hufts 36, window 40, end
 * 44, read 48, write 52, checkfn 56, check 60 -- 64 bytes, which is exactly
 * the ZALLOC(1,64) in inflate_blocks_new. Modes 4/5/6 are BTREE/DTREE/CODES
 * in zlib's enum, so the two frees are upstream's. ARGUMENT ORDER IS REGISTER
 * ORDER: (c, z, s), not zlib's (s, z, c). All three call sites
 * (inflate_blocks_new, inflateReset, inflate) pass it that way; do not
 * restore zlib's order without fixing them in the same change.
 */
void __cdecl inflate_blocks_reset(uLong *c, z_streamp z, inflate_blocks_statef *s)
{
  if (c != 0)
    *c = s->check;
  if (s->mode == BTREE || s->mode == DTREE)
    ZFREE(z, s->sub.trees.blens);
  if (s->mode == CODES)
    ZFREE(z, s->sub.decode.codes);      /* inflate_codes_free, inlined */
  s->read = s->write = s->window;
  s->mode = TYPE;
  s->bitk = 0;
  s->bitb = 0;
  if (s->checkfn != 0)
    z->adler = s->check = (*s->checkfn)(0L, (const Bytef *)0, 0);
}

/* ---- inflate_blocks_new  0x004A4630 ---- */
// inflate_blocks_new -- CONFIRMED 95. ZALLOC(1, sizeof(inflate_blocks_state)=64) then ZALLOC(sizeof(inflate_huft)=8, MANY=1440) for the huft pool. The 8x1440 pair is a unique-constant anchor for zlib 1.1.x.
/*
 * VERIFIED 0x004A4630 against upstream. The three allocations are ZALLOC(1,
 * 64) for the state, ZALLOC(8, 1440) for the huft pool --
 * sizeof(inflate_huft) x MANY, a unique-constant anchor for zlib 1.1.x -- and
 * ZALLOC(1, 1<<wbits) for the window. Exercised on all 14,493 stock-pak
 * reads. Arguments are zlib's (z, checkfn, 1 << wbits).
 */
inflate_blocks_statef *__cdecl inflate_blocks_new(z_streamp z, check_func c, uInt w)
{
  inflate_blocks_statef *s;

  if ((s = (inflate_blocks_statef *)ZALLOC
       (z,1,sizeof(struct inflate_blocks_state))) == 0)
    return s;
  if ((s->hufts =
       (inflate_huft *)ZALLOC(z, sizeof(inflate_huft), MANY)) == 0)
  {
    ZFREE(z, s);
    return 0;
  }
  if ((s->window = (Bytef *)ZALLOC(z, 1, w)) == 0)
  {
    ZFREE(z, s->hufts);
    ZFREE(z, s);
    return 0;
  }
  s->end = s->window + w;
  s->checkfn = c;
  s->mode = TYPE;
  inflate_blocks_reset(0, z, s);
  return s;
}

/* ---- inflate_blocks  0x004A46C0 ---- */
/*
 * VERIFIED 0x004A46C0 BEHAVIOURALLY, which for the block-type dispatcher is
 * the strongest test available: 14,493 stock-pak entries decompressed and
 * CRC-checked, covering all three deflate block types (stored, fixed,
 * dynamic) at sizes from 1 byte to 45 MB. A wrong constant in any state would
 * corrupt one of them.
 *
 * Written as the zlib 1.1.4 infblock.c text.  Argument order (s, z, r)
 * matches zlib and is pinned by the call in inflate():
 * inflate_blocks(state->blocks, z, r).  retail inlines inflate_trees_fixed
 * into the fixed-block case.  The callees keep this file's argument orders:
 * inflate_trees_bits/_dynamic, inflate_codes_new and inflate_codes_free
 * take z first.
 *
 * inflate_blocks_state.sub: left 4 (STORED); trees.table 4, index 8,
 * blens 12, bb 16, tb 20 (TABLE..DTREE); decode.codes 4 (CODES).
 * last 24, hufts 36.
 */
int __cdecl inflate_blocks(inflate_blocks_statef *s, z_streamp z, int r)
{
  uInt t;       /* temporary storage */
  uLong b;      /* bit buffer */
  uInt k;       /* bits in bit buffer */
  Bytef *p;     /* input data pointer */
  uInt n;       /* bytes available there */
  Bytef *q;     /* output window write pointer */
  uInt m;       /* bytes to end of window or read pointer */

  /* copy input/output information to locals (UPDATE macro restores) */
  LOAD

  /* process input based on current state */
  while (1) switch (s->mode)
  {
    case TYPE:
      NEEDBITS(3)
      t = (uInt)b & 7;
      s->last = t & 1;
      switch (t >> 1)
      {
        case 0:                         /* stored */
          DUMPBITS(3)
          t = k & 7;                    /* go to byte boundary */
          DUMPBITS(t)
          s->mode = LENS;  /* get length of stored block */
          break;
        case 1:                         /* fixed */
          {
            uInt bl, bd;
            inflate_huft *tl, *td;

            inflate_trees_fixed(&bl, &bd, &tl, &td);
            s->sub.decode.codes = inflate_codes_new(z, bl, bd, tl, td);
            if (s->sub.decode.codes == 0)
            {
              r = Z_MEM_ERROR;
              LEAVE
            }
          }
          DUMPBITS(3)
          s->mode = CODES;
          break;
        case 2:                         /* dynamic */
          DUMPBITS(3)
          s->mode = TABLE;
          break;
        case 3:                         /* illegal */
          DUMPBITS(3)
          s->mode = BAD;
          z->msg = (char*)"invalid block type";
          r = Z_DATA_ERROR;
          LEAVE
      }
      break;
    case LENS:
      NEEDBITS(32)
      if ((((~b) >> 16) & 0xffff) != (b & 0xffff))
      {
        s->mode = BAD;
        z->msg = (char*)"invalid stored block lengths";
        r = Z_DATA_ERROR;
        LEAVE
      }
      s->sub.left = (uInt)b & 0xffff;
      b = k = 0;                      /* dump bits */
      s->mode = s->sub.left ? STORED : (s->last ? DRY : TYPE);
      break;
    case STORED:
      if (n == 0)
        LEAVE
      NEEDOUT
      t = s->sub.left;
      if (t > n) t = n;
      if (t > m) t = m;
      memcpy(q, p, t);
      p += t;  n -= t;
      q += t;  m -= t;
      if ((s->sub.left -= t) != 0)
        break;
      s->mode = s->last ? DRY : TYPE;
      break;
    case TABLE:
      NEEDBITS(14)
      s->sub.trees.table = t = (uInt)b & 0x3fff;
      if ((t & 0x1f) > 29 || ((t >> 5) & 0x1f) > 29)
      {
        s->mode = BAD;
        z->msg = (char*)"too many length or distance symbols";
        r = Z_DATA_ERROR;
        LEAVE
      }
      t = 258 + (t & 0x1f) + ((t >> 5) & 0x1f);
      if ((s->sub.trees.blens = (uInt*)ZALLOC(z, t, sizeof(uInt))) == 0)
      {
        r = Z_MEM_ERROR;
        LEAVE
      }
      DUMPBITS(14)
      s->sub.trees.index = 0;
      s->mode = BTREE;
    case BTREE:
      while (s->sub.trees.index < 4 + (s->sub.trees.table >> 10))
      {
        NEEDBITS(3)
        s->sub.trees.blens[inflate_border[s->sub.trees.index++]] = (uInt)b & 7;
        DUMPBITS(3)
      }
      while (s->sub.trees.index < 19)
        s->sub.trees.blens[inflate_border[s->sub.trees.index++]] = 0;
      s->sub.trees.bb = 7;
      t = inflate_trees_bits(z, s->sub.trees.blens, &s->sub.trees.bb,
                             &s->sub.trees.tb, s->hufts);
      if (t != 0)
      {
        r = t;
        if (r == Z_DATA_ERROR)
        {
          ZFREE(z, s->sub.trees.blens);
          s->mode = BAD;
        }
        LEAVE
      }
      s->sub.trees.index = 0;
      s->mode = DTREE;
    case DTREE:
      while (t = s->sub.trees.table,
             s->sub.trees.index < 258 + (t & 0x1f) + ((t >> 5) & 0x1f))
      {
        inflate_huft *h;
        uInt i, j, c;

        t = s->sub.trees.bb;
        NEEDBITS(t)
        h = s->sub.trees.tb + ((uInt)b & inflate_mask[t]);
        t = h->bits;
        c = h->base;
        if (c < 16)
        {
          DUMPBITS(t)
          s->sub.trees.blens[s->sub.trees.index++] = c;
        }
        else /* c == 16..18 */
        {
          i = c == 18 ? 7 : c - 14;
          j = c == 18 ? 11 : 3;
          NEEDBITS(t + i)
          DUMPBITS(t)
          j += (uInt)b & inflate_mask[i];
          DUMPBITS(i)
          i = s->sub.trees.index;
          t = s->sub.trees.table;
          if (i + j > 258 + (t & 0x1f) + ((t >> 5) & 0x1f) ||
              (c == 16 && i < 1))
          {
            ZFREE(z, s->sub.trees.blens);
            s->mode = BAD;
            z->msg = (char*)"invalid bit length repeat";
            r = Z_DATA_ERROR;
            LEAVE
          }
          c = c == 16 ? s->sub.trees.blens[i - 1] : 0;
          do {
            s->sub.trees.blens[i++] = c;
          } while (--j);
          s->sub.trees.index = i;
        }
      }
      s->sub.trees.tb = 0;
      {
        uInt bl, bd;
        inflate_huft *tl, *td;
        inflate_codes_statef *c;

        bl = 9;         /* must be <= 9 for lookahead assumptions */
        bd = 6;         /* must be <= 9 for lookahead assumptions */
        t = s->sub.trees.table;
        t = inflate_trees_dynamic(z, 257 + (t & 0x1f), 1 + ((t >> 5) & 0x1f),
                                  s->sub.trees.blens, &bl, &bd, &tl, &td,
                                  s->hufts);
        if (t != 0)
        {
          if (t == (uInt)Z_DATA_ERROR)
          {
            ZFREE(z, s->sub.trees.blens);
            s->mode = BAD;
          }
          r = t;
          LEAVE
        }
        if ((c = inflate_codes_new(z, bl, bd, tl, td)) == Z_NULL)
        {
          r = Z_MEM_ERROR;
          LEAVE
        }
        s->sub.decode.codes = c;
      }
      ZFREE(z, s->sub.trees.blens);
      s->mode = CODES;
    case CODES:
      UPDATE
      if ((r = inflate_codes(s, z, r)) != Z_STREAM_END)
        return inflate_flush(s, z, r);
      r = Z_OK;
      inflate_codes_free(z, s->sub.decode.codes);
      LOAD
      if (!s->last)
      {
        s->mode = TYPE;
        break;
      }
      s->mode = DRY;
    case DRY:
      FLUSH
      if (s->read != s->write)
        LEAVE
      s->mode = DONE;
    case DONE:
      r = Z_STREAM_END;
      LEAVE
    case BAD:
      r = Z_DATA_ERROR;
      LEAVE
    default:
      r = Z_STREAM_ERROR;
      LEAVE
  }
}

/* ---- inflate_blocks_free  0x004A5200 ---- */
// inflate_blocks_free -- CONFIRMED 92. Same mode-keyed free as inflate_blocks_reset, without the reset. Sole caller is inflateEnd.
/*
 * VERIFIED 0x004A5200 against upstream inflate_blocks_free -- the same
 * mode-keyed frees as inflate_blocks_reset plus ZFREE of window, hufts and
 * the state itself. Runs once per pak file close, 14,493 times in the test
 * run with no double-free.
 */
int __cdecl inflate_blocks_free(z_streamp z, inflate_blocks_statef *s)
{
  inflate_blocks_reset(0, z, s);
  ZFREE(z, s->window);
  ZFREE(z, s->hufts);
  ZFREE(z, s);
  return Z_OK;
}

/* ---- inflate_set_dictionary  0x004A5280 ---- */
// inflate_set_dictionary -- CONFIRMED 90. memcpy into the window, then read = write = window + n.
/*
 * VERIFIED 0x004A5280 against upstream: zmemcpy into the window then read =
 * write = window + n. Reachable only from inflateSetDictionary, which has no
 * xrefs in retail -- it never runs on a pak, since raw-deflate streams carry
 * no dictionary.
 */
void __cdecl inflate_set_dictionary(inflate_blocks_statef *s, uInt n, const Bytef *d)
{
  memcpy(s->window, d, n);
  s->read = s->write = s->window + n;
}

/* ---- inflate_blocks_sync_point  0x004A52B0 ---- */
// inflate_blocks_sync_point -- CONFIRMED 88. s->mode == LENS (1).
/*
 * VERIFIED 0x004A52B0. `s->mode == LENS` and LENS is 1 in zlib's enum.
 * Reachable only from inflateSyncPoint, which is dead in retail.
 */
int __cdecl inflate_blocks_sync_point(inflate_blocks_statef *s)
{
  return s->mode == LENS;
}

/* ---- inflate_codes_new  0x004A52C0 ---- */
// inflate_codes_new -- CONFIRMED 92. ZALLOC(1, 28) then stores lbits/dbits (bytes at +16/+17) and the two trees. Pins sizeof(inflate_codes_state) = 28.
/*
 * VERIFIED 0x004A52C0 against upstream. ZALLOC(1, 28) pins
 * sizeof(inflate_codes_state) = 28; lbits and dbits are single BYTES at
 * +16/+17, which is what upstream's `Byte lbits; Byte dbits;` compiles to,
 * and ltree/dtree are dwords at +20/+24.
 */
inflate_codes_statef *__cdecl inflate_codes_new(z_streamp z, uInt bl, uInt bd, inflate_huft *tl, inflate_huft *td)
{
  inflate_codes_statef *c;

  if ((c = (inflate_codes_statef *)
       ZALLOC(z,1,sizeof(struct inflate_codes_state))) != Z_NULL)
  {
    c->lbits = (Byte)bl;
    c->dbits = (Byte)bd;
    c->mode = START;
    c->ltree = tl;
    c->dtree = td;
  }
  return c;
}

/* ---- inflate_codes  0x004A5300 ---- */
// inflate_codes -- CONFIRMED 92. The literal/length/distance state machine; owns "invalid literal/length code" and "invalid distance code".
/*
 * VERIFIED 0x004A5300 BEHAVIOURALLY over 14,493 stock-pak entries. This is
 * the slow-path literal/length/distance state machine; it runs whenever
 * inflate_fast declines, i.e. near the end of every output buffer, so every
 * one of those reads covers it.
 *
 * Written as the zlib 1.1.4 infcodes.c text (s@eax, z@esi, r on the stack).
 * Argument order (s, z, r) matches zlib; pinned by the call in
 * inflate_blocks' CODES case.  inflate_codes_state is 28 bytes, see
 * inflate_codes_new.
 */
int __cdecl inflate_codes(inflate_blocks_statef *s, z_streamp z, int r)
{
  uInt j;       /* temporary storage */
  inflate_huft *t;    /* temporary pointer */
  uInt e;       /* extra bits or operation */
  uLong b;      /* bit buffer */
  uInt k;       /* bits in bit buffer */
  Bytef *p;     /* input data pointer */
  uInt n;       /* bytes available there */
  Bytef *q;     /* output window write pointer */
  uInt m;       /* bytes to end of window or read pointer */
  Bytef *f;     /* pointer to copy strings from */
  inflate_codes_statef *c = s->sub.decode.codes;  /* codes state */

  /* copy input/output information to locals (UPDATE macro restores) */
  LOAD

  /* process input and output based on current state */
  while (1) switch (c->mode)
  {             /* waiting for "i:"=input, "o:"=output, "x:"=nothing */
    case START:  /* x: set up for LEN */
      if (m >= 258 && n >= 10)
      {
        UPDATE
        r = inflate_fast(c->lbits, c->dbits, c->ltree, c->dtree, s, z);
        LOAD
        if (r != Z_OK)
        {
          c->mode = r == Z_STREAM_END ? WASH : BADCODE;
          break;
        }
      }
      c->sub.code.need = c->lbits;
      c->sub.code.tree = c->ltree;
      c->mode = LEN;
    case LEN:  /* i: get length/literal/eob next */
      j = c->sub.code.need;
      NEEDBITS(j)
      t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
      DUMPBITS(t->bits)
      e = (uInt)(t->exop);
      if (e == 0)               /* literal */
      {
        c->sub.lit = t->base;
        c->mode = LIT;
        break;
      }
      if (e & 16)               /* length */
      {
        c->sub.copy.get = e & 15;
        c->len = t->base;
        c->mode = LENEXT;
        break;
      }
      if ((e & 64) == 0)        /* next table */
      {
        c->sub.code.need = e;
        c->sub.code.tree = t + t->base;
        break;
      }
      if (e & 32)               /* end of block */
      {
        c->mode = WASH;
        break;
      }
      c->mode = BADCODE;  /* invalid code */
      z->msg = (char*)"invalid literal/length code";
      r = Z_DATA_ERROR;
      LEAVE
    case LENEXT:  /* i: getting length extra (have base) */
      j = c->sub.copy.get;
      NEEDBITS(j)
      c->len += (uInt)b & inflate_mask[j];
      DUMPBITS(j)
      c->sub.code.need = c->dbits;
      c->sub.code.tree = c->dtree;
      c->mode = DIST;
    case DIST:  /* i: get distance next */
      j = c->sub.code.need;
      NEEDBITS(j)
      t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
      DUMPBITS(t->bits)
      e = (uInt)(t->exop);
      if (e & 16)               /* distance */
      {
        c->sub.copy.get = e & 15;
        c->sub.copy.dist = t->base;
        c->mode = DISTEXT;
        break;
      }
      if ((e & 64) == 0)        /* next table */
      {
        c->sub.code.need = e;
        c->sub.code.tree = t + t->base;
        break;
      }
      c->mode = BADCODE;  /* invalid code */
      z->msg = (char*)"invalid distance code";
      r = Z_DATA_ERROR;
      LEAVE
    case DISTEXT:  /* i: getting distance extra */
      j = c->sub.copy.get;
      NEEDBITS(j)
      c->sub.copy.dist += (uInt)b & inflate_mask[j];
      DUMPBITS(j)
      c->mode = COPY;
    case COPY:  /* o: copying bytes in window, waiting for space */
      f = q - c->sub.copy.dist;
      while (f < s->window)          /* modulo window size-"while" instead */
        f += s->end - s->window;  /* of "if" handles invalid distances */
      while (c->len)
      {
        NEEDOUT
        OUTBYTE(*f++)
        if (f == s->end)
          f = s->window;
        c->len--;
      }
      c->mode = START;
      break;
    case LIT:  /* o: got literal, waiting for output space */
      NEEDOUT
      OUTBYTE(c->sub.lit)
      c->mode = START;
      break;
    case WASH:  /* o: got eob, possibly more output */
      if (k > 7)        /* return unused byte, if any */
      {
        k -= 8;
        n++;
        p--;            /* can always return one */
      }
      FLUSH
      if (s->read != s->write)
        LEAVE
      c->mode = END;
    case END:
      r = Z_STREAM_END;
      LEAVE
    case BADCODE:  /* x: got error */
      r = Z_DATA_ERROR;
      LEAVE
    default:
      r = Z_STREAM_ERROR;
      LEAVE
  }
}

/* ---- inflate_codes_free  0x004A5A00 ---- */
// inflate_codes_free -- CONFIRMED 90. Single ZFREE.
/*
 * VERIFIED 0x004A5A00. A single ZFREE, upstream verbatim.
 */
void __cdecl inflate_codes_free(z_streamp z, inflate_codes_statef *c)
{
  ZFREE(z, c);
}

/* ---- inflate_fast  0x004A5A10 ---- */
// inflate_fast -- CONFIRMED 90. inffast.c: the unrolled inner loop, sharing the same two error strings as inflate_codes.
/*
 * VERIFIED 0x004A5A10 BEHAVIOURALLY over 14,493 stock-pak entries -- it is
 * the path that handles the bulk of every file over a few hundred bytes.
 * Verified indirectly at the table level too: the fixed-Huffman trees it
 * walks are byte-identical to retail's, see inflate_trees_fixed below.
 *
 * Written as the zlib 1.1.4 inffast.c text.  Argument order (bl, bd, tl, td,
 * s, z) matches zlib and is pinned by the call in inflate_codes.
 */
/* macros for bit input with no checking and for returning unused bytes */
#define GRABBITS(j) {while(k<(j)){b|=((unsigned long)NEXTBYTE)<<k;k+=8;}}
#define UNGRAB {c=z->avail_in-n;c=(k>>3)<c?k>>3:c;n+=c;p-=c;k-=c<<3;}

int __cdecl inflate_fast(uInt bl, uInt bd, inflate_huft *tl,
                         inflate_huft *td, /* need separate declaration for Borland C++ */
                         inflate_blocks_statef *s, z_streamp z)
{
  inflate_huft *t;    /* temporary pointer */
  uInt e;       /* extra bits or operation */
  uLong b;      /* bit buffer */
  uInt k;       /* bits in bit buffer */
  Bytef *p;     /* input data pointer */
  uInt n;       /* bytes available there */
  Bytef *q;     /* output window write pointer */
  uInt m;       /* bytes to end of window or read pointer */
  uInt ml;      /* mask for literal/length tree */
  uInt md;      /* mask for distance tree */
  uInt c;       /* bytes to copy */
  uInt d;       /* distance back to copy from */
  Bytef *r;     /* copy source pointer */

  /* load input, output, bit values */
  LOAD

  /* initialize masks */
  ml = inflate_mask[bl];
  md = inflate_mask[bd];

  /* do until not enough input or output space for fast loop */
  do {                          /* assume called with m >= 258 && n >= 10 */
    /* get literal/length code */
    GRABBITS(20)                /* max bits for literal/length code */
    if ((e = (t = tl + ((uInt)b & ml))->exop) == 0)
    {
      DUMPBITS(t->bits)
      *q++ = (Byte)t->base;
      m--;
      continue;
    }
    do {
      DUMPBITS(t->bits)
      if (e & 16)
      {
        /* get extra bits for length */
        e &= 15;
        c = t->base + ((uInt)b & inflate_mask[e]);
        DUMPBITS(e)

        /* decode distance base of block to copy */
        GRABBITS(15);           /* max bits for distance code */
        e = (t = td + ((uInt)b & md))->exop;
        do {
          DUMPBITS(t->bits)
          if (e & 16)
          {
            /* get extra bits to add to distance base */
            e &= 15;
            GRABBITS(e)         /* get extra bits (up to 13) */
            d = t->base + ((uInt)b & inflate_mask[e]);
            DUMPBITS(e)

            /* do the copy */
            m -= c;
            r = q - d;
            if (r < s->window)               /* wrap if needed */
            {
              do {
                r += s->end - s->window;  /* force pointer in window */
              } while (r < s->window);       /* covers invalid distances */
              e = s->end - r;
              if (c > e)
              {
                c -= e;                         /* wrapped copy */
                do {
                    *q++ = *r++;
                } while (--e);
                r = s->window;
                do {
                    *q++ = *r++;
                } while (--c);
              }
              else                              /* normal copy */
              {
                *q++ = *r++;  c--;
                *q++ = *r++;  c--;
                do {
                    *q++ = *r++;
                } while (--c);
              }
            }
            else                                /* normal copy */
            {
              *q++ = *r++;  c--;
              *q++ = *r++;  c--;
              do {
                *q++ = *r++;
              } while (--c);
            }
            break;
          }
          else if ((e & 64) == 0)
          {
            t += t->base;
            e = (t += ((uInt)b & inflate_mask[e]))->exop;
          }
          else
          {
            z->msg = (char*)"invalid distance code";
            UNGRAB
            UPDATE
            return Z_DATA_ERROR;
          }
        } while (1);
        break;
      }
      if ((e & 64) == 0)
      {
        t += t->base;
        if ((e = (t += ((uInt)b & inflate_mask[e]))->exop) == 0)
        {
          DUMPBITS(t->bits)
          *q++ = (Byte)t->base;
          m--;
          break;
        }
      }
      else if (e & 32)
      {
        UNGRAB
        UPDATE
        return Z_STREAM_END;
      }
      else
      {
        z->msg = (char*)"invalid literal/length code";
        UNGRAB
        UPDATE
        return Z_DATA_ERROR;
      }
    } while (1);
  } while (m >= 258 && n >= 10);

  /* not enough input or output--restore pointers and return */
  UNGRAB
  UPDATE
  return Z_OK;
}
#undef GRABBITS
#undef UNGRAB

/* ---- inflateReset  0x004A5DB0 ---- */
// inflateReset -- CONFIRMED 92. Clears total_in/total_out/msg and calls inflate_blocks_reset.
/*
 * VERIFIED 0x004A5DB0 against upstream. internal_state is 24 bytes -- mode 0,
 * sub 4..11, nowrap 12, wbits 16, blocks 20 -- matching the ZALLOC(1,24) in
 * inflateInit2_; `nowrap ? 7 : 0` is BLOCKS vs METHOD in zlib's enum, and
 * raw-deflate pak streams always take the 7.
 */
int __cdecl inflateReset(z_streamp z)
{
  if (z == Z_NULL || z->state == Z_NULL)
    return Z_STREAM_ERROR;
  z->total_in = z->total_out = 0;
  z->msg = Z_NULL;
  z->state->mode = z->state->nowrap ? BLOCKS : METHOD;
  inflate_blocks_reset(Z_NULL, z, z->state->blocks);
  return Z_OK;
}

/* ---- inflateEnd  0x004A5DF0 ---- */
// inflateEnd -- CONFIRMED 95. inflate_blocks_free then ZFREE of the state. Called by unzCloseCurrentFile and uncompress.
/*
 * VERIFIED 0x004A5DF0 against upstream. Called from unzCloseCurrentFile once
 * per pak file, 14,493 times in the test run.
 */
int __cdecl inflateEnd(z_streamp z)
{
  if (z == Z_NULL || z->state == Z_NULL || z->zfree == Z_NULL)
    return Z_STREAM_ERROR;
  if (z->state->blocks != Z_NULL)
    inflate_blocks_free(z, z->state->blocks);
  ZFREE(z, z->state);
  z->state = Z_NULL;
  return Z_OK;
}

/* ---- inflateInit2_  0x004A5E40 ---- */
// inflateInit2_ -- CONFIRMED 95. Validates version[0] == '1' (49) and stream_size == 56. The 56 pins sizeof(z_stream) for this build.
/*
 * VERIFIED 0x004A5E40 against upstream. The two guards are version[0] == 49
 * ('1') and stream_size == 56, which pins sizeof(z_stream) for this build.
 * Negative windowBits sets nowrap and suppresses the adler32 checkfn, which
 * is how the pak path runs -- and is why the CRC in unzCloseCurrentFile is
 * the only integrity check a pak read gets.
 */
int __cdecl inflateInit2_(z_streamp z, int w, const char *version, int stream_size)
{
  if (version == Z_NULL || version[0] != '1' ||   /* ZLIB_VERSION[0], spelled as the character retail compares against */
      stream_size != sizeof(z_stream))
      return Z_VERSION_ERROR;

  /* initialize state */
  if (z == Z_NULL)
    return Z_STREAM_ERROR;
  z->msg = Z_NULL;
  if (z->zalloc == Z_NULL)
  {
    z->zalloc = (alloc_func)zcalloc;
    z->opaque = (voidpf)0;
  }
  if (z->zfree == Z_NULL) z->zfree = (free_func)zcfree;
  if ((z->state = (struct internal_state *)
       ZALLOC(z,1,sizeof(struct internal_state))) == Z_NULL)
    return Z_MEM_ERROR;
  z->state->blocks = Z_NULL;

  /* handle undocumented nowrap option (no zlib header or check) */
  z->state->nowrap = 0;
  if (w < 0)
  {
    w = - w;
    z->state->nowrap = 1;
  }

  /* set window size */
  if (w < 8 || w > 15)
  {
    inflateEnd(z);
    return Z_STREAM_ERROR;
  }
  z->state->wbits = (uInt)w;

  /* create inflate_blocks state */
  if ((z->state->blocks =
      inflate_blocks_new(z, z->state->nowrap ? Z_NULL : (check_func)adler32, (uInt)1 << w))
      == Z_NULL)
  {
    inflateEnd(z);
    return Z_MEM_ERROR;
  }

  /* reset state */
  inflateReset(z);
  return Z_OK;
}

/* ---- inflateInit_  0x004A5F30 ---- */
// inflateInit_ -- CONFIRMED 92. inflateInit2_(strm, DEF_WBITS=15, version, stream_size).
/*
 * VERIFIED 0x004A5F30. inflateInit2_(strm, DEF_WBITS = 15, version,
 * stream_size); the argument order is confirmed by matching the forwarded
 * slots against the callee above.
 */
int __cdecl inflateInit_(int stream_size, z_streamp z, const char *version)
{
  return inflateInit2_(z, 15, version, stream_size);   /* DEF_WBITS */
}

/* ---- inflate  0x004A5F50 ---- */
// inflate -- CONFIRMED 95. The inflate.c driver; owns "unknown compression method", "invalid window size", "incorrect header check".
/*
 * VERIFIED 0x004A5F50 against upstream inflate.c. Exercised in both
 * configurations: raw-deflate on 14,493 pak entries (Z_SYNC_FLUSH from
 * unzReadCurrentFile), and zlib-wrapped through uncompress, which is dead in
 * retail.  Written as the upstream text (z@eax, f@edx).  z_stream: next_in 0,
 * avail_in 4, total_in 8, msg 24, state 28, adler 48.  internal_state (uInt
 * words): mode [0], sub.method / sub.check.was / sub.marker [1],
 * sub.check.need [2], nowrap [3], wbits [4], blocks [5].
 */
#undef NEEDBYTE
#undef NEXTBYTE
#define PRESET_DICT 0x20 /* preset dictionary flag in zlib header (zutil.h) */
#define NEEDBYTE {if(z->avail_in==0)return r;r=f;}
#define NEXTBYTE (z->avail_in--,z->total_in++,*z->next_in++)

int __cdecl inflate(z_streamp z, int f)
{
  int r;
  uInt b;

  if ( z == 0 || z->state == 0 || z->next_in == 0 )
    return Z_STREAM_ERROR;
  f = f == Z_FINISH ? Z_BUF_ERROR : Z_OK;
  r = Z_BUF_ERROR;
  while ( 1 ) switch ( z->state->mode )
  {
    case METHOD:
      NEEDBYTE
      if ( ((z->state->sub.method = NEXTBYTE) & 0xf) != Z_DEFLATED )
      {
        z->state->mode = IBAD;
        z->msg = (char*)"unknown compression method";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      if ( (z->state->sub.method >> 4) + 8 > z->state->wbits )
      {
        z->state->mode = IBAD;
        z->msg = (char*)"invalid window size";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      z->state->mode = FLAG;
    case FLAG:
      NEEDBYTE
      b = NEXTBYTE;
      if ( ((z->state->sub.method << 8) + b) % 31 )
      {
        z->state->mode = IBAD;
        z->msg = (char*)"incorrect header check";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      if ( !(b & PRESET_DICT) )
      {
        z->state->mode = BLOCKS;
        break;
      }
      z->state->mode = DICT4;
    case DICT4:
      NEEDBYTE
      z->state->sub.check.need = (uInt)NEXTBYTE << 24;
      z->state->mode = DICT3;
    case DICT3:
      NEEDBYTE
      z->state->sub.check.need += (uInt)NEXTBYTE << 16;
      z->state->mode = DICT2;
    case DICT2:
      NEEDBYTE
      z->state->sub.check.need += (uInt)NEXTBYTE << 8;
      z->state->mode = DICT1;
    case DICT1:
      NEEDBYTE
      z->state->sub.check.need += (uInt)NEXTBYTE;
      z->adler = z->state->sub.check.need;
      z->state->mode = DICT0;
      return Z_NEED_DICT;
    case DICT0:
      z->state->mode = IBAD;
      z->msg = (char*)"need dictionary";
      z->state->sub.marker = 0;         /* can try inflateSync */
      return Z_STREAM_ERROR;
    case BLOCKS:
      r = inflate_blocks(z->state->blocks, z, r);
      if ( r == Z_DATA_ERROR )
      {
        z->state->mode = IBAD;
        z->state->sub.marker = 0;       /* can try inflateSync */
        break;
      }
      if ( r == Z_OK )
        r = f;
      if ( r != Z_STREAM_END )
        return r;
      r = f;
      inflate_blocks_reset(&z->state->sub.check.was, z, z->state->blocks);
      if ( z->state->nowrap )
      {
        z->state->mode = IDONE;
        break;
      }
      z->state->mode = CHECK4;
    case CHECK4:
      NEEDBYTE
      z->state->sub.check.need = (uInt)NEXTBYTE << 24;
      z->state->mode = CHECK3;
    case CHECK3:
      NEEDBYTE
      z->state->sub.check.need += (uInt)NEXTBYTE << 16;
      z->state->mode = CHECK2;
    case CHECK2:
      NEEDBYTE
      z->state->sub.check.need += (uInt)NEXTBYTE << 8;
      z->state->mode = CHECK1;
    case CHECK1:
      NEEDBYTE
      z->state->sub.check.need += (uInt)NEXTBYTE;

      if ( z->state->sub.check.was != z->state->sub.check.need )
      {
        z->state->mode = IBAD;
        z->msg = (char*)"incorrect data check";
        z->state->sub.marker = 5;       /* can't try inflateSync */
        break;
      }
      z->state->mode = IDONE;
    case IDONE:                                     /* DONE */
      return Z_STREAM_END;
    case IBAD:                                     /* BAD */
      return Z_DATA_ERROR;
    default:
      return Z_STREAM_ERROR;
  }
}

/* ---- inflateSetDictionary  0x004A6340 ---- */
// inflateSetDictionary -- CONFIRMED 90. Public wrapper over inflate_set_dictionary; checks state mode == DICT0 (6).
/*
 * VERIFIED 0x004A6340 against upstream. DEAD IN RETAIL -- no xrefs. Verified,
 * not exercised.  Written as the upstream text (z@edi, dictLength@eax,
 * dictionary on the stack); z->state 28, z->adler 48; state->mode 0,
 * wbits 16, blocks 20.
 */
int __cdecl inflateSetDictionary(z_streamp z, Bytef *dictionary, uInt dictLength)
{
  uInt length = dictLength;

  if (z == Z_NULL || z->state == Z_NULL || z->state->mode != DICT0)
    return Z_STREAM_ERROR;

  if (adler32(1L, dictionary, dictLength) != z->adler) return Z_DATA_ERROR;
  z->adler = 1L;

  if (length >= ((uInt)1<<z->state->wbits))
  {
    length = (1<<z->state->wbits)-1;
    dictionary += dictLength - length;
  }
  inflate_set_dictionary(z->state->blocks, length, dictionary);
  z->state->mode = BLOCKS;
  return Z_OK;
}

/* ---- inflateSync  0x004A63D0 ----  IMPLEMENTED (retail) */
/*
 * zlib 1.1.4 inflate.c `inflateSync`: skip input until the 00 00 FF FF of an
 * empty stored block (zlib_mark), then restart the decoder at BLOCKS with the
 * byte counters preserved across the reset.
 *
 * Retail is __usercall z@<esi>, one argument, returning in EAX; written here as
 * __cdecl.  DEAD IN RETAIL: no xrefs and no pointer literal (see the dead-code
 * note at the top).
 *
 * Retail offsets: z_stream next_in +0, avail_in +4, total_in +8, total_out
 * +20, state +28; internal_state mode +0, sub.marker +4.  Modes are zlib's
 * enum: 13 = BAD (IBAD here), 7 = BLOCKS.
 *
 * Retail compiles the search as a bottom-tested loop (`dec ebp / jnz`) with
 * the m < 4 test at the top; that is upstream's `while (n && m < 4)`, since
 * avail_in has already been checked non-zero on the way in.
 */
int __cdecl inflateSync( z_streamp z )
{
	uInt  n;      /* number of bytes to look at */
	Bytef *p;     /* pointer to bytes */
	uInt  m;      /* number of marker bytes found in a row */
	uLong r, w;   /* temporaries to save total_in and total_out */

	/* set up */
	if ( z == Z_NULL || z->state == Z_NULL ) {
		return Z_STREAM_ERROR;
	}
	if ( z->state->mode != IBAD ) {         /* enter BAD, restart the search */
		z->state->mode = IBAD;
		z->state->sub.marker = 0;
	}
	if ( ( n = z->avail_in ) == 0 ) {
		return Z_BUF_ERROR;
	}
	p = z->next_in;
	m = z->state->sub.marker;

	/* search: a 00 byte that breaks a partial match can itself start a new
	 * one, which is what the 4 - m does */
	while ( n && m < 4 ) {
		if ( *p == zlib_mark[m] ) {
			m++;
		} else if ( *p ) {
			m = 0;
		} else {
			m = 4 - m;
		}
		p++;
		n--;
	}

	/* restore */
	z->total_in += p - z->next_in;
	z->next_in = p;
	z->avail_in = n;
	z->state->sub.marker = m;

	/* return no joy or set up to restart on a new block */
	if ( m != 4 ) {
		return Z_DATA_ERROR;
	}
	r = z->total_in;  w = z->total_out;
	inflateReset( z );
	z->total_in = r;  z->total_out = w;
	z->state->mode = BLOCKS;
	return Z_OK;
}

/* ---- inflateSyncPoint  0x004A6490 ---- */
// inflateSyncPoint -- CONFIRMED 90. Forwards to inflate_blocks_sync_point.
/*
 * VERIFIED 0x004A6490. One-line forward to inflate_blocks_sync_point. DEAD IN
 * RETAIL -- no xrefs.
 */
int __cdecl inflateSyncPoint(z_streamp z)
{
  if (z == Z_NULL || z->state == Z_NULL || z->state->blocks == Z_NULL)
    return Z_STREAM_ERROR;
  return inflate_blocks_sync_point(z->state->blocks);
}

/* ---- huft_build  0x004A64C0 ---- */
// huft_build -- CONFIRMED 92. inftrees.c: the Huffman table builder shared by inflate_trees_bits and inflate_trees_dynamic.
/*
 * VERIFIED 0x004A64C0 MECHANICALLY, and this is the strongest single result
 * in the file. Its output for the fixed literal/length and distance trees was
 * dumped and compared against retail's own precompiled tables at 0x00571C58
 * (fixed_tl, 4096 bytes) and 0x00572C58 (fixed_td, 256 bytes): SHA-1
 * identical over all 4,352 bytes, with bl/bd coming back 9/5 to match
 * 0x00571C4C/0x00571C50. Those tables ARE huft_build output, so that verifies
 * the whole builder -- including the hard-coded 0x5A0 (1440, MANY) overflow
 * guard -- without reading a single instruction. The dynamic path is covered
 * behaviourally by 14,493 stock-pak entries.
 *
 * Now written as the zlib 1.1.4 inftrees.c text, in zlib's argument order
 * (b@ecx, m@eax, the rest on the stack).  `r` is uninitialised as upstream:
 * the out-of-values path (`r.exop = 128 + 64`) leaves r.base alone, and both
 * inflate_codes and inflate_fast test `e & 64` and abort without reading it.
 */
int __cdecl huft_build(
        uInt *b,          /* code lengths in bits (all assumed <= BMAX) */
        uInt n,           /* number of codes (assumed <= 288) */
        uInt s,           /* number of simple-valued codes (0..s-1) */
        const uInt *d,    /* list of base values for non-simple codes */
        const uInt *e,    /* list of extra bits for non-simple codes */
        inflate_huft **t,       /* result: starting table */
        uInt *m,          /* maximum lookup bits, returns actual */
        inflate_huft *hp,       /* space for trees */
        uInt *hn,         /* hufts used in space */
        uInt *v)          /* working area: values in order of bit length */
{
  uInt a;               /* counter for codes of length k */
  uInt c[15+1];         /* bit length count table */
  uInt f;               /* i repeats in table every f entries */
  int g;                        /* maximum code length */
  int h;                        /* table level */
  register uInt i;      /* counter, current code */
  register uInt j;      /* counter */
  register int k;               /* number of bits in current code */
  int l;                        /* bits per table (returned in m) */
  uInt mask;            /* (1 << w) - 1, to avoid cc -O bug on HP */
  register uInt *p;     /* pointer into c[], b[], or v[] */
  inflate_huft *q;            /* points to current table */
  inflate_huft r;             /* table entry for structure assignment */
  inflate_huft *u[15];        /* table stack */
  register int w;               /* bits before this table == (l * h) */
  uInt x[15+1];         /* bit offsets, then code stack */
  uInt *xp;             /* pointer into x */
  int y;                        /* number of dummy codes added */
  uInt z;               /* number of entries in current table */

#ifndef COD1_VC7
  r.base = 0;                   /* modern build: no uninitialised read (never used) */
#endif

  /* Generate counts for each bit length */
  p = c;
#define C0 *p++ = 0;
#define C2 C0 C0 C0 C0
#define C4 C2 C2 C2 C2
  C4                            /* clear c[]--assume BMAX+1 is 16 */
#undef C0
#undef C2
#undef C4
  p = b;  i = n;
  do {
    c[*p++]++;                  /* assume all entries <= BMAX */
  } while (--i);
  if (c[0] == n)                /* null input--all zero length codes */
  {
    *t = (inflate_huft *)0;
    *m = 0;
    return Z_OK;
  }


  /* Find minimum and maximum length, bound *m by those */
  l = *m;
  for (j = 1; j <= 15; j++)
    if (c[j])
      break;
  k = j;                        /* minimum code length */
  if ((uInt)l < j)
    l = j;
  for (i = 15; i; i--)
    if (c[i])
      break;
  g = i;                        /* maximum code length */
  if ((uInt)l > i)
    l = i;
  *m = l;


  /* Adjust last length count to fill out codes, if needed */
  for (y = 1 << j; j < i; j++, y <<= 1)
    if ((y -= c[j]) < 0)
      return Z_DATA_ERROR;
  if ((y -= c[i]) < 0)
    return Z_DATA_ERROR;
  c[i] += y;


  /* Generate starting offsets into the value table for each length */
  x[1] = j = 0;
  p = c + 1;  xp = x + 2;
  while (--i) {                 /* note that i == g from above */
    *xp++ = (j += *p++);
  }


  /* Make a table of values in order of bit lengths */
  p = b;  i = 0;
  do {
    if ((j = *p++) != 0)
      v[x[j]++] = i;
  } while (++i < n);
  n = x[g];                     /* set n to length of v */


  /* Generate the Huffman codes and for each, make the table entries */
  x[0] = i = 0;                 /* first Huffman code is zero */
  p = v;                        /* grab values in bit order */
  h = -1;                       /* no tables yet--level -1 */
  w = -l;                       /* bits decoded == (l * h) */
  u[0] = (inflate_huft *)0;   /* just to keep compilers happy */
  q = (inflate_huft *)0;      /* ditto */
  z = 0;                        /* ditto */

  /* go through the bit lengths (k already is bits in shortest code) */
  for (; k <= g; k++)
  {
    a = c[k];
    while (a--)
    {
      /* here i is the Huffman code of length k bits for value *p */
      /* make tables up to required level */
      while (k > w + l)
      {
        h++;
        w += l;                 /* previous table always l bits */

        /* compute minimum size table less than or equal to l bits */
        z = g - w;
        z = z > (uInt)l ? l : z;        /* table size upper limit */
        if ((f = 1 << (j = k - w)) > a + 1)     /* try a k-w bit table */
        {                       /* too few codes for k-w bit table */
          f -= a + 1;           /* deduct codes from patterns left */
          xp = c + k;
          if (j < z)
            while (++j < z)     /* try smaller tables up to z bits */
            {
              if ((f <<= 1) <= *++xp)
                break;          /* enough codes to use up j bits */
              f -= *xp;         /* else deduct codes from patterns */
            }
        }
        z = 1 << j;             /* table entries for j-bit table */

        /* allocate new table */
        if (*hn + z > MANY)     /* (note: doesn't matter for fixed) */
          return Z_DATA_ERROR;  /* overflow of MANY */
        u[h] = q = hp + *hn;
        *hn += z;

        /* connect to last table, if there is one */
        if (h)
        {
          x[h] = i;             /* save pattern for backing up */
          r.bits = (Byte)l;     /* bits to dump before this table */
          r.exop = (Byte)j;     /* bits in this table */
          j = i >> (w - l);
          r.base = (uInt)(q - u[h-1] - j);   /* offset to this table */
          u[h-1][j] = r;        /* connect to last table */
        }
        else
          *t = q;               /* first table is returned result */
      }

      /* set up table entry in r */
      r.bits = (Byte)(k - w);
      if (p >= v + n)
        r.exop = 128 + 64;      /* out of values--invalid code */
      else if (*p < s)
      {
        r.exop = (Byte)(*p < 256 ? 0 : 32 + 64);     /* 256 is end-of-block */
        r.base = *p++;          /* simple code is just the value */
      }
      else
      {
        r.exop = (Byte)(e[*p - s] + 16 + 64);/* non-simple--look up in lists */
        r.base = d[*p++ - s];
      }

      /* fill code-like entries with r */
      f = 1 << (k - w);
      for (j = i >> w; j < z; j += f)
        q[j] = r;

      /* backwards increment the k-bit code i */
      for (j = 1 << (k - 1); i & j; j >>= 1)
        i ^= j;
      i ^= j;

      /* backup over finished tables */
      mask = (1 << w) - 1;      /* needed on HP, cc -O bug */
      while ((i & mask) != x[h])
      {
        h--;                    /* don't need to update q */
        w -= l;
        mask = (1 << w) - 1;
      }
    }
  }


  /* Return Z_BUF_ERROR if we were given an incomplete table */
  return y != 0 && g != 1 ? Z_BUF_ERROR : Z_OK;
}

/* ---- inflate_trees_bits  0x004A69B0 ---- */
// inflate_trees_bits -- CONFIRMED 95. Owns "oversubscribed dynamic bit lengths tree" / "incomplete dynamic bit lengths tree".
/*
 * VERIFIED 0x004A69B0 against upstream inflate_trees_bits, including the
 * 19/19 code counts and both error strings; now written as the upstream text
 * (z@ebx, c/bb/tb/hp on the stack).  z->msg 24, zalloc 32, zfree 36, opaque 40.
 */
int __cdecl inflate_trees_bits(z_streamp z, uInt *c, uInt *bb, inflate_huft **tb, inflate_huft *hp)
{
  int r;
  uInt hn = 0;          /* hufts used in space */
  uInt *v;              /* work area for huft_build */

  if ( (v = (uInt*)ZALLOC(z, 19, sizeof(uInt))) == Z_NULL )
    return Z_MEM_ERROR;
  r = huft_build(c, 19, 19, (uInt*)Z_NULL, (uInt*)Z_NULL, tb, bb, hp, &hn, v);
  if ( r == Z_DATA_ERROR )
    z->msg = (char*)"oversubscribed dynamic bit lengths tree";
  else if ( r == Z_BUF_ERROR || *bb == 0 )
  {
    z->msg = (char*)"incomplete dynamic bit lengths tree";
    r = Z_DATA_ERROR;
  }
  ZFREE(z, v);
  return r;
}

/* ---- inflate_trees_dynamic  0x004A6A50 ---- */
/*
 * VERIFIED 0x004A6A50 against upstream inflate_trees_dynamic -- both
 * huft_build calls, the 0x101 (257) literal base, and all five error strings.
 * Exercised on every dynamic block in the stock paks.  Written as the
 * upstream text; z (messages, zalloc/zfree) comes first here, the order the
 * inflate_blocks call site passes it.
 */
int __cdecl inflate_trees_dynamic(
        z_streamp z,              /* for messages */
        uInt nl,                  /* number of literal/length codes */
        uInt nd,                  /* number of distance codes */
        uInt *c,                  /* that many (total) code lengths */
        uInt *bl,                 /* literal desired/actual bit depth */
        uInt *bd,                 /* distance desired/actual bit depth */
        inflate_huft **tl,        /* literal/length tree result */
        inflate_huft **td,        /* distance tree result */
        inflate_huft *hp)         /* space for trees */
{
  int r;
  uInt hn = 0;          /* hufts used in space */
  uInt *v;              /* work area for huft_build */

  /* allocate work area */
  if ( (v = (uInt*)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL )
    return Z_MEM_ERROR;

  /* build literal/length tree */
  r = huft_build(c, nl, 257, cplens, cplext, tl, bl, hp, &hn, v);
  if ( r != Z_OK || *bl == 0 )
  {
    if ( r == Z_DATA_ERROR )
      z->msg = (char*)"oversubscribed literal/length tree";
    else if ( r != Z_MEM_ERROR )
    {
      z->msg = (char*)"incomplete literal/length tree";
      r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
  }

  /* build distance tree */
  r = huft_build(c + nl, nd, 0, cpdist, cpdext, td, bd, hp, &hn, v);
  if ( r != Z_OK || (*bd == 0 && nl > 257) )
  {
    if ( r == Z_DATA_ERROR )
      z->msg = (char*)"oversubscribed distance tree";
    else if ( r == Z_BUF_ERROR )
    {
      z->msg = (char*)"incomplete distance tree";
      r = Z_DATA_ERROR;
    }
    else if ( r != Z_MEM_ERROR )
    {
      z->msg = (char*)"empty distance tree with lengths";
      r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
  }

  /* done */
  ZFREE(z, v);
  return Z_OK;
}

/* ---- inflate_trees_fixed  0x004A6BD0 ---- */
// inflate_trees_fixed -- CONFIRMED 88. Hands back the two static tables at 0x571C58 and 0x572C58 with fixed bit counts -- no huft_build call, which is what distinguishes it.
/*
 * VERIFIED 0x004A6BD0: loads fixed_bl/fixed_bd and hands back the addresses
 * of fixed_tl/fixed_td, returning 0 (Z_OK).  Those four now ARE retail's
 * precompiled .data (see the tables near the top of this file); an earlier
 * pass built them at first use with huft_build instead.
 */
/*
 * The recovered body was unusable twice over.  It is __usercall with `bl` in
 * ECX, which the rewrite to __cdecl dropped, so the emitted code stored through
 * an uninitialised `v3` (VC6 warns C4700 on it); and the two tables it hands
 * back are the zeroed placeholders.  Rewritten with zlib's own signature and
 * backed by the retail tables.
 *
 * Its only caller is inflate_blocks' fixed-block case, where retail inlines
 * it; zlib's argument order is used (without the unused z).
 */
int __cdecl inflate_trees_fixed( uInt *bl, uInt *bd,
                                 inflate_huft **tl, inflate_huft **td )
{
  *bl = fixed_bl;
  *bd = fixed_bd;
  *tl = fixed_tl;
  *td = fixed_td;
  return Z_OK;
}

/* ---- inflate_flush  0x004A6C00 ---- */
// inflate_flush -- CONFIRMED 92. infutil.c: copies as much of the window as will fit into next_out, updating check and total_out. Callers: inflate_blocks, inflate_codes.
/*
 * VERIFIED 0x004A6C00 line by line against upstream infutil.c inflate_flush,
 * including the window-wrap half; now written as the upstream text (s@ebx,
 * z and r on the stack).  Field map:
 * s->window 40, end 44, read 48, write 52, checkfn 56, check 60; z->next_out
 * 12, avail_out 16, total_out 20, adler 48.
 */
int __cdecl inflate_flush(inflate_blocks_statef *s, z_streamp z, int r)
{
  uInt n;
  Bytef *p;
  Bytef *q;

  /* local copies of source and destination pointers */
  p = z->next_out;
  q = s->read;

  /* compute number of bytes to copy as far as end of window */
  n = (uInt)((q <= s->write ? s->write : s->end) - q);
  if (n > z->avail_out) n = z->avail_out;
  if (n && r == Z_BUF_ERROR) r = Z_OK;

  /* update counters */
  z->avail_out -= n;
  z->total_out += n;

  /* update check information */
  if (s->checkfn != Z_NULL)
    z->adler = s->check = (*s->checkfn)(s->check, q, n);

  /* copy as far as end of window */
  memcpy(p, q, n);
  p += n;
  q += n;

  /* see if more to copy at beginning of window */
  if (q == s->end)
  {
    /* wrap pointers */
    q = s->window;
    if (s->write == s->end)
      s->write = s->window;

    /* compute bytes to copy */
    n = (uInt)(s->write - q);
    if (n > z->avail_out) n = z->avail_out;
    if (n && r == Z_BUF_ERROR) r = Z_OK;

    /* update counters */
    z->avail_out -= n;
    z->total_out += n;

    /* update check information */
    if (s->checkfn != Z_NULL)
      z->adler = s->check = (*s->checkfn)(s->check, q, n);

    /* copy */
    memcpy(p, q, n);
    p += n;
    q += n;
  }

  /* update pointers */
  z->next_out = p;
  s->read = q;

  /* done */
  return r;
}

/* ---- tr_static_init  0x004A6D40 ---- */
void tr_static_init()
{
  ;
}

/* ---- _tr_init  0x004A6D50  VERIFIED ---- */
/*
 * THIS IS zlib trees.c `_tr_init`.  It WAS shadowed by a link stub; FIXED
 * 2026-08-15 by the qcommon/universal owner, who took zlib on for this.
 *
 * The shadow, as it stood:
 *     body   inflate.obj      @_tr_init@8   (__fastcall)
 *     stub   link_stubs.obj   _sub_4A6D50     (__cdecl)   <-- won the link
 *     caller universal/memorytree.cpp  `extern int _tr_init();`
 *                                      _tr_init(0, (int)v23)
 * Different decoration, so no LNK2005 and no warning: the body below had never
 * run, and the Huffman tree state was never initialised.
 *
 * THE FIX IS THE ONE THIS FILE ALREADY USES FOR init_block, twenty lines down:
 * declare the definition __cdecl so it decorates as plain `_sub_4A6D50` and
 * therefore collides with -- rather than hiding behind -- the stub, and delete
 * the stub in the same change.  qcommon/link_stubs.c's (file since removed)
 * `int _tr_init() { return 0; }` is removed.  No call site changes: every
 * caller already spells it __cdecl, which is exactly why the shadow existed.
 *
 * Proved at the symbol level, not by reading the source, because the source
 * reads correctly either way -- `dumpbin /SYMBOLS` now shows `_sub_4A6D50` as
 * SECT4 External (a definition) in inflate.obj and UNDEF in memorytree.obj,
 * where before inflate.obj carried only `@_tr_init@8` and nothing resolved
 * `_sub_4A6D50` but the stub.
 *
 * 2026-10-13 (script lane): now zlib's one-argument `_tr_init(s)`.  The
 * phantom stack argument made deflateReset's call push and pop a dead 0;
 * without it deflateReset is identical and _tr_init keeps s@<edx>.
 * Historical note follows.
 *
 * THE RECOVERED SIGNATURE IS STILL WRONG, and is kept deliberately.  Retail
 * 0x004A6D50 is fourteen instructions and touches ONE register argument:
 * everything is `[edx + ...]`, and ECX is written as scratch at 0x004A6D6A
 * before it is ever read.  So retail is __usercall s@<edx>, one argument, and
 * Hex-Rays' `(int a1, int a2)` has a phantom a1 -- which is why the call site
 * passes a literal 0 for it.  The IDB now carries `__usercall(s@<edx>)`.  The
 * tree keeps the two-parameter spelling so the caller needs no edit and the
 * dummy is harmless: `unused` is never read.  Same disposition as
 * Com_SkipRestOfLine in universal/q_parse.c.
 *
 * SEVERITY IS LOW AND THAT IS NOT A GUESS: this is the deflate side.  Its only
 * caller is deflateInit2_ (0x004A2E20), reached only from compress2 (0x004A2C20),
 * reached only from compress (0x004A2CC0), which has no xrefs in the retail
 * binary at all.  Nothing in a dedicated server compresses anything.  Unshadowing
 * it removes a latent wrong-code path rather than fixing a live one.
 *
 * It also tail-calls init_block = trees.c `init_block`, which is real below and
 * no longer stubbed, so the deflate init path is now whole.
 */
short *__cdecl _tr_init(deflate_state *s)
{
  /* tr_static_init() is empty without GEN_TREES_H: retail has no call */
  s->l_desc.dyn_tree = s->dyn_ltree;
  s->l_desc.stat_desc = (static_tree_desc *)&static_l_desc;

  s->d_desc.dyn_tree = s->dyn_dtree;
  s->d_desc.stat_desc = (static_tree_desc *)&static_d_desc;

  s->bl_desc.dyn_tree = s->bl_tree;
  s->bl_desc.stat_desc = (static_tree_desc *)&static_bl_desc;

  s->bi_buf = 0;
  s->bi_valid = 0;
  s->last_eob_len = 8; /* enough lookahead for inflate */

  /* Initialize the first block of the first file: */
  return init_block(s);
}

/* ---- init_block  0x004A6DB0  VERIFIED ----
 *
 * WAS STUB-ONLY.  Written 2026-08-14 from the coverage-gap audit; the only
 * definition in the tree was `int init_block() { return 0; }` in
 * qcommon/link_stubs.c (since removed).  THAT LINE MUST NOW BE DELETED -- this definition is
 * __cdecl, deliberately, so the two decorate identically (`_sub_4A6DB0`) and
 * the collision is a loud LNK2005 rather than the silent shadow that has kept
 * _tr_init next door from ever running.
 *
 * THIS IS zlib 1.1.4 trees.c `init_block`, and the identification is exact
 * rather than by analogy -- the three loop counts are zlib's three code-count
 * constants and nothing else has that triple:
 *
 *     0x004A6DB7  mov ecx, 11Eh   286 = L_CODES    over s->dyn_ltree
 *     0x004A6DCF  mov ecx, 1Eh     30 = D_CODES    over s->dyn_dtree
 *     0x004A6DE3  mov ecx, 13h     19 = BL_CODES   over s->bl_tree
 *
 * and the strides confirm the ct_data layout: each loop stores a WORD (the
 * .Freq half) and advances by 4 (sizeof ct_data).  The base offsets fall out
 * of deflate_state exactly -- dyn_ltree is ct_data[HEAP_SIZE] = [573], so
 * 0x8C + 573*4 = 0x980 is dyn_dtree, and 0x980 + 61*4 = 0xA74 is bl_tree.
 *
 *     s + 0x008C  dyn_ltree[573]
 *     s + 0x0980  dyn_dtree[61]
 *     s + 0x0A74  bl_tree[39]
 *     s + 0x048C  dyn_ltree[END_BLOCK].Freq   0x8C + 256*4, set to 1
 *     s + 0x1698  last_lit    = 0
 *     s + 0x16A0  opt_len     = 0
 *     s + 0x16A4  static_len  = 0
 *     s + 0x16A8  matches     = 0
 *
 * all four of the last group written as DWORDS (`mov [edx+...], esi` with esi
 * zeroed at 0x004A6DBE), and only the END_BLOCK store is a word.
 *
 * ARGUMENT COUNT: ONE, in EDX.  Every access is `[edx + ...]`; ECX is loaded
 * with a literal at 0x004A6DB7 before it is ever read, so Hex-Rays' first
 * parameter was a phantom -- the same shape _tr_init above has.  It is gone
 * now: _tr_flush_block (0x004A80AA) calls with EDX = s and nothing pushed.
 *
 * RETURN VALUE: zlib's init_block is void, but retail leaves EAX at
 * s + 0xA74 + 19*4 = s + 0xAC0, the address one past the end of bl_tree,
 * because that is where the third loop's cursor stopped.  _tr_init returns
 * it, so it is reproduced rather than dropped -- it is junk in both, but it
 * is the SAME junk.
 *
 * SEVERITY: low, and for the reason the _tr_init note above already
 * establishes -- this is the deflate side, whose only route in is
 * compress() (0x004A2CC0), which has no xrefs in the retail binary at all.
 * Nothing a dedicated server does compresses anything.  Written because the
 * pair was the last thing keeping trees.c from being reconstructable, not
 * because anything was failing.
 */
short *__cdecl init_block( deflate_state *s )
{
	short  *p;
	int     n; /* iterates over tree elements */

	/* Initialize the trees: zlib's
	 *     for (n = 0; n < L_CODES;  n++) s->dyn_ltree[n].Freq = 0;
	 * and the same over dyn_dtree and bl_tree, walked by pointer so that the
	 * cursor left behind is what retail returns (see above). */
	p = (short *)&s->dyn_ltree[0].Freq;
	for ( n = L_CODES ; n ; n-- ) {
		*p = 0;
		p = (short *)( (char *)p + sizeof(ct_data) );
	}

	p = (short *)&s->dyn_dtree[0].Freq;
	for ( n = D_CODES ; n ; n-- ) {
		*p = 0;
		p = (short *)( (char *)p + sizeof(ct_data) );
	}

	p = (short *)&s->bl_tree[0].Freq;
	for ( n = BL_CODES ; n ; n-- ) {
		*p = 0;
		p = (short *)( (char *)p + sizeof(ct_data) );
	}

	s->static_len = 0;
	s->opt_len = 0;
	s->matches = 0;
	s->last_lit = 0;

	s->dyn_ltree[END_BLOCK].Freq = 1;

	return p;
}

/*
 * ---------------------------------------------------------------------------
 * zlib 1.1.4 trees.c / deflate.h local constants and macros (non-DEBUG
 * forms), used by the trees.c functions below.  deflate_state, ct_data and
 * tree_desc are in zlib_types.h.
 * ---------------------------------------------------------------------------
 */
#define MAX_BL_BITS 7   /* Bit length codes must not exceed MAX_BL_BITS bits */
#define REP_3_6      16 /* repeat previous bit length 3-6 times (2 bits of repeat count) */
#define REPZ_3_10    17 /* repeat a zero length 3-10 times  (3 bits of repeat count) */
#define REPZ_11_138  18 /* repeat a zero length 11-138 times  (7 bits of repeat count) */

#define STORED_BLOCK 0  /* zutil.h block types */
#define STATIC_TREES 1
#define DYN_TREES    2

#define Buf_size (8 * 2*sizeof(char))
/* Number of bits used within bi_buf. (bi_buf might be implemented on
 * more than 16 bits on some systems.) */

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */

#define put_byte(s, c) {s->pending_buf[s->pending++] = (c);}

/* Output a short LSB first on the stream.
 * IN assertion: there is enough room in pendingBuf. */
#define put_short(s, w) {     put_byte(s, (uch)((w) & 0xff));     put_byte(s, (uch)((ush)(w) >> 8)); }

#define send_bits(s, value, length) { int len = length;  if (s->bi_valid > (int)Buf_size - len) {    int val = value;    s->bi_buf |= (val << s->bi_valid);    put_short(s, s->bi_buf);    s->bi_buf = (ush)val >> (Buf_size - s->bi_valid);    s->bi_valid += len - Buf_size;  } else {    s->bi_buf |= (value) << s->bi_valid;    s->bi_valid += len;  }}

#define send_code(s, c, tree) send_bits(s, tree[c].Code, tree[c].Len)
/* Send a code of the given tree. c and tree must not have side effects */

#define d_code(dist)    ((dist) < 256 ? _dist_code[dist] : _dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. _dist_code[256] and _dist_code[257] are never
 * used. */

#define pqremove(s, tree, top) {    top = s->heap[SMALLEST];     s->heap[SMALLEST] = s->heap[s->heap_len--];     pqdownheap(s, tree, SMALLEST); }
/* Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len. */

#define smaller(tree, n, m, depth)    (tree[n].Freq < tree[m].Freq ||    (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))
/* Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length. */

/* The static trees and descriptors (zlib/zlib_rdata.c) are declared by
 * zlib_types.h as raw bytes; these views give them trees.c's types. */
#define static_ltree_ct ((const ct_data *)static_ltree)
#define static_dtree_ct ((const ct_data *)static_dtree)

/* ---- pqdownheap  0x004A6E20 ---- */
/* zlib 1.1.4 trees.c pqdownheap (s@eax, tree@edi, k on the stack).
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).  deflate_state: heap[] 0xB54, heap_len 0x1448, depth[] 0x1450;
 * ct_data is 4 bytes (Freq at +0, Len/Dad at +2). */
void __cdecl pqdownheap(deflate_state *s, ct_data *tree, int k)
{
  int v = s->heap[k];
  int j = k << 1;  /* left son of k */

  while ( j <= s->heap_len )
  {
    /* Set j to the smallest of the two sons: */
    if ( j < s->heap_len && smaller(tree, s->heap[j + 1], s->heap[j], s->depth) )
      j++;
    /* Exit if v is smaller than both sons */
    if ( smaller(tree, v, s->heap[j], s->depth) )
      break;
    /* Exchange v with the smallest son */
    s->heap[k] = s->heap[j];
    k = j;
    /* And continue down the tree, setting j to the left son of k */
    j <<= 1;
  }
  s->heap[k] = v;
}

/* ---- gen_bitlen  0x004A6EF0 ---- */
/* zlib 1.1.4 trees.c gen_bitlen (desc@eax, s@edx).  bl_count[] at s+0xB34. */
void __cdecl gen_bitlen(tree_desc *desc, deflate_state *s)
{
  ct_data *tree        = desc->dyn_tree;
  int max_code         = desc->max_code;
  const ct_data *stree = desc->stat_desc->static_tree;
  const int *extra     = desc->stat_desc->extra_bits;
  int base             = desc->stat_desc->extra_base;
  int max_length       = desc->stat_desc->max_length;
  int h;              /* heap index */
  int n, m;           /* iterate over the tree elements */
  int bits;           /* bit length */
  int xbits;          /* extra bits */
  ush f;              /* frequency */
  int overflow = 0;   /* number of elements with bit length too large */

  for ( bits = 0; bits <= MAX_BITS; bits++ ) s->bl_count[bits] = 0;

  /* In a first pass, compute the optimal bit lengths (which may
   * overflow in the case of the bit length tree).
   */
  tree[s->heap[s->heap_max]].Len = 0; /* root of the heap */

  for ( h = s->heap_max + 1; h < HEAP_SIZE; h++ )
  {
    n = s->heap[h];
    bits = tree[tree[n].Dad].Len + 1;
    if ( bits > max_length ) bits = max_length, overflow++;
    tree[n].Len = (ush)bits;
    /* We overwrite tree[n].Dad which is no longer needed */

    if ( n > max_code ) continue; /* not a leaf node */

    s->bl_count[bits]++;
    xbits = 0;
    if ( n >= base ) xbits = extra[n - base];
    f = tree[n].Freq;
    s->opt_len += (ulg)f * (bits + xbits);
    if ( stree ) s->static_len += (ulg)f * (stree[n].Len + xbits);
  }
  if ( overflow == 0 ) return;

  /* Find the first bit length which could increase: */
  do
  {
    bits = max_length - 1;
    while ( s->bl_count[bits] == 0 ) bits--;
    s->bl_count[bits]--;      /* move one leaf down the tree */
    s->bl_count[bits + 1] += 2; /* move one overflow item as its brother */
    s->bl_count[max_length]--;
    /* The brother of the overflow item also moves one step up,
     * but this does not affect bl_count[max_length]
     */
    overflow -= 2;
  }
  while ( overflow > 0 );

  /* Now recompute all bit lengths, scanning in increasing frequency.
   * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
   * lengths instead of fixing only the wrong ones. This idea is taken
   * from 'ar' written by Haruhiko Okumura.)
   */
  for ( bits = max_length; bits != 0; bits-- )
  {
    n = s->bl_count[bits];
    while ( n != 0 )
    {
      m = s->heap[--h];
      if ( m > max_code ) continue;
      if ( tree[m].Len != (unsigned)bits )
      {
        s->opt_len += ((long)bits - (long)tree[m].Len) * (long)tree[m].Freq;
        tree[m].Len = (ush)bits;
      }
      n--;
    }
  }
}

/* ---- gen_codes  0x004A7100 ---- */
/* zlib 1.1.4 trees.c gen_codes, in register order rather than zlib's
 * (tree, max_code, bl_count): bl_count (s+0xB34, retail EDX), max_code (EBX),
 * tree (EDI).
 * zlib's next_code[MAX_BITS+1] is a real 16-entry ushort array (retail
 * [esp+4+2*bits] after push esi, with the compiler's /GS cookie above it).
 * void, as in zlib; the bi_reverse call is inlined (code@ecx, len@edx). */
void __cdecl gen_codes(ush *bl_count, int max_code, ct_data *tree)
{
  ush next_code[MAX_BITS+1]; /* next code value for each bit length */
  ush code = 0;              /* running code value */
  int bits, n, len;          /* bit index, code index, code length */

  /* The distribution counts are first used to generate the code values
   * without bit reversal.
   */
  for ( bits = 1; bits <= MAX_BITS; bits++ )
  {
    next_code[bits] = code = (ush)((code + bl_count[bits - 1]) << 1);
  }
  for ( n = 0; n <= max_code; n++ )              /* signed: test ebx,ebx / jl ; cmp esi,ebx / jle */
  {
    len = tree[n].Len;
    if ( len == 0 )
      continue;
    /* Now reverse the bits */
    tree[n].Code = (ush)bi_reverse(next_code[len]++, len);
  }
}

/* ---- build_tree  0x004A7190 ---- */
/* zlib 1.1.4 trees.c build_tree (s@esi, desc on the stack).  desc is tree_desc
 * { dyn_tree, max_code, stat_desc }; stat_desc { static_tree, extra_bits,
 * extra_base, elems, max_length }.  opt_len 0x16A0, static_len 0x16A4. */
void __cdecl build_tree(deflate_state *s, tree_desc *desc)
{
  ct_data *tree        = desc->dyn_tree;
  const ct_data *stree = desc->stat_desc->static_tree;
  int elems            = desc->stat_desc->elems;
  int n, m;          /* iterate over heap elements */
  int max_code = -1; /* largest code with non zero frequency */
  int node;          /* new node being created */

  /* Construct the initial heap, with least frequent element in
   * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
   * heap[0] is not used.
   */
  s->heap_len = 0, s->heap_max = HEAP_SIZE;

  for ( n = 0; n < elems; n++ )
  {
    if ( tree[n].Freq != 0 )
    {
      s->heap[++(s->heap_len)] = max_code = n;
      s->depth[n] = 0;
    }
    else
    {
      tree[n].Len = 0;
    }
  }

  /* The pkzip format requires that at least one distance code exists,
   * and that at least one bit should be sent even if there is only one
   * possible code. So to avoid special checks later on we force at least
   * two codes of non zero frequency.
   */
  while ( s->heap_len < 2 )
  {
    node = s->heap[++(s->heap_len)] = (max_code < 2 ? ++max_code : 0);
    tree[node].Freq = 1;
    s->depth[node] = 0;
    s->opt_len--; if ( stree ) s->static_len -= stree[node].Len;
    /* node is 0 or 1 so it does not have extra bits */
  }
  desc->max_code = max_code;

  /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
   * establish sub-heaps of increasing lengths:
   */
  for ( n = s->heap_len / 2; n >= 1; n-- )
    pqdownheap(s, tree, n);

  /* Construct the Huffman tree by repeatedly combining the least two
   * frequent nodes.
   */
  node = elems;              /* next internal node of the tree */
  do
  {
    pqremove(s, tree, n);    /* n = node of least frequency */
    m = s->heap[SMALLEST];   /* m = node of next least frequency */

    s->heap[--(s->heap_max)] = n; /* keep the nodes sorted by frequency */
    s->heap[--(s->heap_max)] = m;

    /* Create a new node father of n and m */
    tree[node].Freq = tree[n].Freq + tree[m].Freq;
    s->depth[node] = (uch)((s->depth[n] >= s->depth[m] ? s->depth[n] : s->depth[m]) + 1);
    tree[n].Dad = tree[m].Dad = (ush)node;
    /* and insert the new node in the heap */
    s->heap[SMALLEST] = node++;
    pqdownheap(s, tree, SMALLEST);
  }
  while ( s->heap_len >= 2 );

  s->heap[--(s->heap_max)] = s->heap[SMALLEST];

  /* At this point, the fields freq and dad are set. We can now
   * generate the bit lengths.
   */
  gen_bitlen(desc, s);

  /* The field len is now set, we can generate the bit codes */
  gen_codes(s->bl_count, max_code, tree);
}

/* ---- scan_tree  0x004A7390 ---- */
void __cdecl scan_tree(deflate_state *s, ct_data *tree, int max_code)
{
  int n;                     /* iterates over all tree elements */
  int prevlen = -1;          /* last emitted length */
  int curlen;                /* length of current code */
  int nextlen = tree[0].Len; /* length of next code */
  int count = 0;             /* repeat count of the current code */
  int max_count = 7;         /* max repeat count */
  int min_count = 4;         /* min repeat count */

  if ( nextlen == 0 )
    max_count = 138, min_count = 3;
  tree[max_code + 1].Len = (ush)0xffff; /* guard */

  for ( n = 0; n <= max_code; n++ )
  {
    curlen = nextlen;
    nextlen = tree[n + 1].Len;
    if ( ++count < max_count && curlen == nextlen )
    {
      continue;
    }
    else if ( count < min_count )
    {
      s->bl_tree[curlen].Freq += count;
    }
    else if ( curlen != 0 )
    {
      if ( curlen != prevlen )
        s->bl_tree[curlen].Freq++;
      s->bl_tree[REP_3_6].Freq++;
    }
    else if ( count <= 10 )
    {
      s->bl_tree[REPZ_3_10].Freq++;
    }
    else
    {
      s->bl_tree[REPZ_11_138].Freq++;
    }
    count = 0;
    prevlen = curlen;
    if ( nextlen == 0 )
    {
      max_count = 138, min_count = 3;
    }
    else if ( curlen == nextlen )
    {
      max_count = 6, min_count = 3;
    }
    else
    {
      max_count = 7, min_count = 4;
    }
  }
}

/* ---- send_tree  0x004A7470 ---- */
/* zlib 1.1.4 trees.c send_tree (s@eax, tree@ecx, max_code on the stack), with
 * the non-DEBUG send_bits/put_short macros (above pqdownheap) expanded as in
 * trees.c.  deflate_state: pending_buf +0x08, pending +0x14, bl_tree[] +0xA74,
 * bi_buf +0x16B0 (ush), bi_valid +0x16B4.  zlib's send_tree is void; retail
 * leaves s in EAX and send_all_trees passes it on, so it is returned. */
deflate_state *__cdecl send_tree(deflate_state *s, ct_data *tree, int max_code)
{
  int n;                     /* iterates over all tree elements */
  int prevlen = -1;          /* last emitted length */
  int curlen;                /* length of current code */
  int nextlen = tree[0].Len; /* length of next code */
  int count = 0;             /* repeat count of the current code */
  int max_count = 7;         /* max repeat count */
  int min_count = 4;         /* min repeat count */

  /* tree[max_code+1].Len = -1; */  /* guard already set */
  if ( nextlen == 0 ) max_count = 138, min_count = 3;

  for ( n = 0; n <= max_code; n++ )
  {
    curlen = nextlen; nextlen = tree[n + 1].Len;
    if ( ++count < max_count && curlen == nextlen )
    {
      continue;
    }
    else if ( count < min_count )
    {
      do { send_code(s, curlen, s->bl_tree); } while ( --count != 0 );
    }
    else if ( curlen != 0 )
    {
      if ( curlen != prevlen )
      {
        send_code(s, curlen, s->bl_tree); count--;
      }
      send_code(s, REP_3_6, s->bl_tree); send_bits(s, count - 3, 2);
    }
    else if ( count <= 10 )
    {
      send_code(s, REPZ_3_10, s->bl_tree); send_bits(s, count - 3, 3);
    }
    else
    {
      send_code(s, REPZ_11_138, s->bl_tree); send_bits(s, count - 11, 7);
    }
    count = 0;
    prevlen = curlen;
    if ( nextlen == 0 )
    {
      max_count = 138, min_count = 3;
    }
    else if ( curlen == nextlen )
    {
      max_count = 6, min_count = 3;
    }
    else
    {
      max_count = 7, min_count = 4;
    }
  }
  return s;
}

/*
 * ---------------------------------------------------------------------------
 * trees.c put_byte as a function, used by copy_block below (the put_byte
 * macro above pqdownheap is the same store; send_bits is zlib's non-DEBUG
 * macro body, Buf_size 16, which retail expands inline at every site).
 *
 * deflate_state offsets: pending_buf +0x08, pending +0x14.
 * ---------------------------------------------------------------------------
 */
static void tr_put_byte( deflate_state *s, uch c )
{
	s->pending_buf[s->pending] = c;
	s->pending += 1;
}

/* ---- build_bl_tree  0x004A7980 ----  IMPLEMENTED (retail) */
/*
 * zlib 1.1.4 trees.c `build_bl_tree`: scan both literal and distance trees for
 * repeat runs (scan_tree = scan_tree), build the bit-length tree (build_tree =
 * build_tree), and find the last bit-length code with a non-zero length in
 * bl_order to decide how many to send.  Returns that index, max_blindex.
 *
 * Retail is __usercall s@<eax>.  Callee register mapping: scan_tree takes
 * (tree@eax, max_code@ecx, s on the stack), build_tree (s@esi, desc on the
 * stack); scan_tree is spelled in zlib's (s, tree, max_code) order, which
 * is what gives retail's argument evaluation order here.
 *
 * deflate_state offsets: dyn_ltree +0x8C, dyn_dtree +0x980, bl_tree +0xA74
 * (so +0xA76 is .Len), l_desc.max_code +0xB14, d_desc.max_code +0xB20,
 * bl_desc +0xB28, opt_len +0x16A0.  The opt_len charge is 3*(max_blindex+1)
 * + 5+5+4, which retail folds to `lea edx, [eax+eax*2+11h]`.
 */
int __cdecl build_bl_tree( deflate_state *s )
{
	int max_blindex;  /* index of last bit length code of non zero freq */

	/* Determine the bit length frequencies for literal and distance trees */
	scan_tree( s, s->dyn_ltree, s->l_desc.max_code );
	scan_tree( s, s->dyn_dtree, s->d_desc.max_code );

	/* Build the bit length tree: */
	build_tree( s, &s->bl_desc );
	/* opt_len now includes the length of the tree representations, except
	 * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
	 */

	/* Determine the number of bit length codes to send. The pkzip format
	 * requires that at least 4 bit length codes be sent. (appnote.txt says
	 * 3 but the actual value used is 4.)
	 */
	for ( max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex-- ) {
		if ( s->bl_tree[bl_order[max_blindex]].Len != 0 ) {
			break;
		}
	}

	/* Update opt_len to include the bit length tree and counts */
	s->opt_len += 3 * ( max_blindex + 1 ) + 5 + 5 + 4;
	return max_blindex;
}

/* ---- send_all_trees  0x004A79F0 ----  IMPLEMENTED (retail) */
/*
 * zlib 1.1.4 trees.c `send_all_trees`: send the header for a dynamic block --
 * HLIT (lcodes-257, 5 bits), HDIST (dcodes-1, 5), HCLEN (blcodes-4, 4), the
 * bit-length code lengths in bl_order order (3 bits each), then the literal
 * and distance trees in run-length form (send_tree = send_tree).
 *
 * Retail is __usercall s@<eax> with lcodes/dcodes/blcodes on the stack, which
 * is the order _tr_flush_block's call site passes them.  send_tree is
 * (s@eax, tree@ecx, max_code on the stack) and returns s in EAX; retail feeds
 * that EAX straight into the second call and returns the second call's EAX,
 * so the value returned here is s.
 */
deflate_state *__cdecl send_all_trees( deflate_state *s, int lcodes, int dcodes, int blcodes )
{
	int rank;                    /* index in bl_order */

	send_bits( s, lcodes - 257, 5 );   /* not +255 as stated in appnote.txt */
	send_bits( s, dcodes - 1, 5 );
	send_bits( s, blcodes - 4, 4 );    /* not -3 as stated in appnote.txt */
	for ( rank = 0; rank < blcodes; rank++ ) {
		send_bits( s, s->bl_tree[bl_order[rank]].Len, 3 );
	}

	send_tree( s, s->dyn_ltree, lcodes - 1 );          /* literal tree */
	return send_tree( s, s->dyn_dtree, dcodes - 1 );   /* distance tree */
}

/* ---- _tr_stored_block  0x004A7C60 ---- */
/* zlib 1.1.4 trees.c _tr_stored_block (s@eax, eof@ecx, buf and stored_len on
 * the stack).  The parameter order is the one the existing callers pass. */
deflate_state *__cdecl _tr_stored_block(deflate_state *s, int eof, char *buf, ulg stored_len)
{
  send_bits(s, (STORED_BLOCK << 1) + eof, 3);  /* send block type */
  return copy_block(s, buf, (unsigned)stored_len, 1); /* with header */
}

/* ---- _tr_align  0x004A7D00 ---- */
/* zlib 1.1.4 trees.c _tr_align (s@eax).  last_eob_len at +0x16AC. */
deflate_state *__cdecl _tr_align(deflate_state *s)
{
  send_bits(s, STATIC_TREES << 1, 3);
  send_code(s, END_BLOCK, static_ltree_ct);
  bi_flush(s);
  /* Of the 10 bits for the empty block, we have already sent
   * (10 - bi_valid) bits. The lookahead for the last real code (before
   * the EOB of the previous block) was thus at least one plus the length
   * of the EOB plus what we have just sent of the empty static block.
   */
  if ( 1 + s->last_eob_len + 10 - s->bi_valid < 9 )
  {
    send_bits(s, STATIC_TREES << 1, 3);
    send_code(s, END_BLOCK, static_ltree_ct);
    bi_flush(s);
  }
  s->last_eob_len = 7;
  return s;
}

/* ---- _tr_flush_block  0x004A7EF0 ---- */
/* zlib 1.1.4 trees.c _tr_flush_block.  Retail is stored_len@eax, s@ecx,
 * eof@edi, buf on the stack, which upstream's (s, buf, stored_len, eof) order
 * gives; the tree keeps memorytree.cpp's (stored_len, s, eof, buf) order, so
 * it gets s@eax/stored_len@ecx and that is its only difference.
 * deflate_state: level +0x7C, data_type +0x1C (Byte), l_desc +0xB10 (max_code
 * +0xB14), d_desc +0xB1C (max_code +0xB20), opt_len +0x16A0, static_len +0x16A4. */
void __cdecl _tr_flush_block(deflate_state *s, char *buf, ulg stored_len, int eof)
{
  ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
  int max_blindex = 0;  /* index of last bit length code of non zero freq */

  /* Build the Huffman trees unless a stored block is forced */
  if ( s->level > 0 )
  {
    /* Check if the file is ascii or binary */
    if ( s->data_type == Z_UNKNOWN ) set_data_type(s);

    /* Construct the literal and distance trees */
    build_tree(s, &s->l_desc);
    build_tree(s, &s->d_desc);
    /* At this point, opt_len and static_len are the total bit lengths of
     * the compressed block data, excluding the tree representations.
     */

    /* Build the bit length tree for the above two trees, and get the index
     * in bl_order of the last bit length code to send.
     */
    max_blindex = build_bl_tree(s);

    /* Determine the best encoding. Compute first the block length in bytes*/
    opt_lenb = (s->opt_len + 3 + 7) >> 3;
    static_lenb = (s->static_len + 3 + 7) >> 3;

    if ( static_lenb <= opt_lenb ) opt_lenb = static_lenb;
  }
  else
  {
    opt_lenb = static_lenb = stored_len + 5; /* force a stored block */
  }

  if ( stored_len + 4 <= opt_lenb && buf != (char *)0 )
  {
    /* 4: two words for the lengths
     * The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
     * Otherwise we can't have processed more than WSIZE input bytes since
     * the last block flush, because compression would have been
     * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
     * transform a block into a stored block.
     */
    _tr_stored_block(s, eof, buf, stored_len);
  }
  else if ( static_lenb == opt_lenb )
  {
    send_bits(s, (STATIC_TREES << 1) + eof, 3);
    compress_block(s, static_ltree_ct, static_dtree_ct);
  }
  else
  {
    send_bits(s, (DYN_TREES << 1) + eof, 3);
    send_all_trees(s, s->l_desc.max_code + 1, s->d_desc.max_code + 1, max_blindex + 1);
    compress_block(s, s->dyn_ltree, s->dyn_dtree);
  }
  init_block(s);

  if ( eof )
  {
    bi_windup(s);
  }
}

/* ---- _tr_tally  0x004A80D0 ----  IMPLEMENTED (retail) */
/*
 * zlib 1.1.4 trees.c `_tr_tally`: record a literal (dist == 0, lc = the byte)
 * or a match (dist = distance, lc = length - MIN_MATCH) in the l_buf/d_buf
 * pending-symbol buffers and bump its frequency.  Returns true when the buffer
 * is full and the caller must flush the block.
 *
 * Retail is __usercall s@<eax>, dist@<ecx>, lc@<edx>; the IDB's prototype lists
 * them in register order (s, lc, dist).  It has no callers anywhere in the tree
 * -- deflate_fast/deflate_slow (universal/memorytree.cpp) carry retail's inlined
 * copies -- so this takes zlib's own order (s, dist, lc).
 *
 * TRUNCATE_BLOCK is off in 1.1.4 and retail has no trace of it: the return is
 * the bare `last_lit == lit_bufsize-1`.  Retail also computes a dead
 * `lea edx, [eax+edx*4+490h]` after the length-code increment; it has no
 * effect and is not reproduced.
 *
 * deflate_state offsets: dyn_ltree +0x8C (Freq is the low word of each 4-byte
 * ct_data; +0x490 is dyn_ltree[LITERALS+1]), dyn_dtree +0x980, l_buf +0x1690,
 * lit_bufsize +0x1694, last_lit +0x1698, d_buf +0x169C, matches +0x16A8.
 */
int __cdecl _tr_tally( deflate_state *s, unsigned dist, unsigned lc )
{
	s->d_buf[s->last_lit] = (ush)dist;
	s->l_buf[s->last_lit] = (uch)lc;
	s->last_lit += 1;

	if ( dist == 0 ) {
		/* lc is the unmatched char */
		s->dyn_ltree[lc].Freq++;
	} else {
		s->matches++;
		/* Here, lc is the match length - MIN_MATCH */
		dist--;             /* dist = match distance - 1 */
		s->dyn_ltree[_length_code[lc] + LITERALS + 1].Freq++;
		s->dyn_dtree[d_code(dist)].Freq++;
	}

	return ( s->last_lit == s->lit_bufsize - 1 );
}

/* ---- compress_block  0x004A8170 ----  IMPLEMENTED (retail) */
/*
 * zlib 1.1.4 trees.c `compress_block`: emit every pending symbol from
 * l_buf/d_buf with the given literal and distance trees (the static pair or the
 * block's dynamic pair -- _tr_flush_block calls it both ways), then END_BLOCK,
 * and remember END_BLOCK's length in last_eob_len for _tr_align.
 *
 * Retail is __usercall s@<eax> with ltree/dtree on the stack; void as in zlib
 * (an int return of s merges the two END_BLOCK epilogues retail keeps apart).
 *
 * ct_data is {ush Freq/Code; ush Dad/Len}: tree[c].Code at +4c, .Len at +4c+2.
 * The length code is sent from ltree[code + LITERALS + 1], which retail
 * addresses as ltree + code*4 + 0x404; END_BLOCK is ltree[256], +0x400/+0x402.
 * d_code(dist) reads the high half of _dist_code for dist >= 256 (IDA's
 * byte_540DB8 is &_dist_code[256]).
 *
 * deflate_state offsets: l_buf +0x1690, last_lit +0x1698, d_buf +0x169C,
 * last_eob_len +0x16AC.  The loop is do-while under an outer last_lit != 0
 * test, unsigned compare (`jb`), exactly as upstream writes it.
 */
void __cdecl compress_block( deflate_state *s, const ct_data *ltree, const ct_data *dtree )
{
	unsigned        dist;       /* distance of matched string */
	int             lc;         /* match length or unmatched char (if dist == 0) */
	unsigned        lx = 0;     /* running index in l_buf */
	unsigned        code;       /* the code to send */
	int             extra;      /* number of extra bits to send */

	if ( s->last_lit != 0 ) do {
		dist = s->d_buf[lx];
		lc = s->l_buf[lx++];
		if ( dist == 0 ) {
			send_code( s, lc, ltree );   /* send a literal byte */
		} else {
			/* Here, lc is the match length - MIN_MATCH */
			code = _length_code[lc];
			send_code( s, code + LITERALS + 1, ltree );   /* send the length code */
			extra = extra_lbits[code];
			if ( extra != 0 ) {
				lc -= base_length[code];
				send_bits( s, lc, extra );       /* send the extra length bits */
			}
			dist--; /* dist is now the match distance - 1 */
			code = d_code( dist );

			send_code( s, code, dtree );       /* send the distance code */
			extra = extra_dbits[code];
			if ( extra != 0 ) {
				dist -= base_dist[code];
				send_bits( s, dist, extra );   /* send the extra distance bits */
			}
		} /* literal or match pair ? */
	} while ( lx < s->last_lit );

	send_code( s, END_BLOCK, ltree );
	s->last_eob_len = ltree[END_BLOCK].Len;
}

/* ---- set_data_type  0x004A8590 ---- */
void __cdecl set_data_type(deflate_state *s)
{
  int n = 0;
  unsigned ascii_freq = 0;
  unsigned bin_freq = 0;
  while ( n < 7 )        bin_freq += s->dyn_ltree[n++].Freq;
  while ( n < 128 )    ascii_freq += s->dyn_ltree[n++].Freq;
  while ( n < LITERALS ) bin_freq += s->dyn_ltree[n++].Freq;
  s->data_type = (Byte)(bin_freq > (ascii_freq >> 2) ? Z_BINARY : Z_ASCII);
}

/* ---- bi_reverse  0x004A8620 ---- */
unsigned int __fastcall bi_reverse(unsigned code, int len)
{
  register unsigned res = 0;
  do
  {
    res |= code & 1;
    code >>= 1, res <<= 1;
  }
  while ( --len > 0 );
  return res >> 1;
}

/* ---- bi_flush  0x004A8640 ---- */
deflate_state *__cdecl bi_flush(deflate_state *s)
{
  if ( s->bi_valid == 16 )
  {
    put_short(s, s->bi_buf);
    s->bi_buf = 0;
    s->bi_valid = 0;
  }
  else if ( s->bi_valid >= 8 )
  {
    put_byte(s, (Byte)s->bi_buf);
    s->bi_buf >>= 8;
    s->bi_valid -= 8;
  }
  return s;
}

/* ---- bi_windup  0x004A86C0 ---- */
/* zlib's shared `bi_buf = bi_valid = 0` tail is written into each branch: that gives retail's layout. */
deflate_state *__cdecl bi_windup(deflate_state *s)
{
  if ( s->bi_valid > 8 )
  {
    put_short(s, s->bi_buf);
    s->bi_buf = 0;
    s->bi_valid = 0;
  }
  else if ( s->bi_valid > 0 )
  {
    put_byte(s, (Byte)s->bi_buf);
    s->bi_buf = 0;
    s->bi_valid = 0;
  }
  else
  {
    s->bi_buf = 0;
    s->bi_valid = 0;
  }
  return s;
}

/* ---- copy_block  0x004A8730 ----  IMPLEMENTED (retail) */
/*
 * zlib 1.1.4 trees.c `copy_block`: copy a stored block into pending_buf,
 * optionally preceded by the LEN / NLEN header, after aligning to a byte
 * boundary (bi_windup = bi_windup).  Called only by _tr_stored_block, with
 * header = 1.
 *
 * Retail is __usercall s@<eax>, buf@<edx>, len@<ecx>, header on the stack --
 * the order _tr_stored_block's call site passes them.  bi_windup returns s in
 * EAX and nothing after it touches EAX, so retail returns s; so does this.
 * last_eob_len (+0x16AC) is set to 8 -- enough lookahead for inflate.
 * pending_buf +0x08, pending +0x14.
 */
deflate_state *__cdecl copy_block( deflate_state *s, char *buf, unsigned len, int header )
{
	bi_windup( s );                            /* align on byte boundary */
	s->last_eob_len = 8; /* enough lookahead for inflate */

	if ( header ) {
		/* put_short(s, (ush)len); put_short(s, (ush)~len); */
		tr_put_byte( s, (uch)( len & 0xff ) );
		tr_put_byte( s, (uch)( (ush)len >> 8 ) );
		tr_put_byte( s, (uch)( ~len & 0xff ) );
		tr_put_byte( s, (uch)( (ush)~len >> 8 ) );
	}
	while ( len-- ) {
		tr_put_byte( s, *buf++ );
	}
	return s;
}
