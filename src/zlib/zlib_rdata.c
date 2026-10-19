/*
 * zlib_rdata.c -- zlib 1.1.4 data objects (zutil.c, trees.c) with their retail contents.
 *
 * @fidelity: verified   (storage and initialisers only -- no code here)
 *
 * This unit deliberately does NOT include zlib_types.h.  The definitions
 * below and that header's declarations of the same names (the raw-byte and
 * int views inflate.c and unzip.c address them by) have different types and
 * must never meet inside one translation unit; the linker matches them by
 * NAME alone.  Same arrangement, and same reason, as EffectsCore/fx_rdata.c
 * and client_mp/cl_refstorage.c.
 *
 * zlibVersion (0x004A9950) pins the bundled library at 1.1.4, so every layout
 * below is read off the public 1.1.4 sources -- zutil.c and trees.c -- and then
 * checked against the image.  No middleware internals are decompiled here.
 *
 * ==========================================================================
 * static_l_desc / static_d_desc / static_bl_desc  @ 0x00571BC8, BDC, BF0
 * ==========================================================================
 *
 * trees.c:
 *
 *     typedef struct static_tree_desc_s {
 *         const ct_data *static_tree;
 *         const intf    *extra_bits;
 *         int            extra_base;
 *         int            elems;
 *         int            max_length;
 *     } static_tree_desc;
 *
 * -- five fields, 20 bytes, and retail has three of them back to back:
 *
 *     0x00571BC8  005407C0 00540670 00000101 0000011E 0000000F   static_l_desc
 *     0x00571BDC  00540C40 005406E8 00000000 0000001E 0000000F   static_d_desc
 *     0x00571BF0  00000000 00540760 00000000 00000013 00000007   static_bl_desc
 *
 * which is trees.c verbatim: {static_ltree, extra_lbits, LITERALS+1 (257),
 * L_CODES (286), MAX_BITS (15)}, {static_dtree, extra_dbits, 0, D_CODES (30),
 * MAX_BITS}, {NULL, extra_blbits, 0, BL_CODES (19), MAX_BL_BITS (7)}.  The
 * three run 0x00571BC8..0x00571C03 and inflate_mask follows at 0x00571C08.
 *
 * _tr_init (zlib/inflate.c) stores their addresses into the deflate state at
 * +2840 and +2852 and reads .extra_bits, .extra_base, .elems and .max_length
 * through them, so all three need their real 20-byte initialised contents.
 * The stride is 20; a size measured to the end of the region (0x00571C08)
 * would say 64 and 44 bytes.
 *
 * ==========================================================================
 * trees.h tables  @ 0x00540670 .. 0x005410A7
 * ==========================================================================
 *
 * The tables those descriptors point at are defined in this unit too.  Retail
 * holds trees.h's precomputed data, all of it in .rdata:
 *
 *     0x00540670  extra_lbits[29]     116 bytes   (+4 pad)
 *     0x005406E8  extra_dbits[30]     120 bytes
 *     0x00540760  extra_blbits[19]     76 bytes   (unnamed)
 *     0x005407AC  bl_order[19]         19 bytes   (+1 pad)
 *     0x005407C0  static_ltree[288]  1152 bytes
 *     0x00540C40  static_dtree[30]    120 bytes
 *     0x00540CB8  _dist_code[512]     512 bytes
 *     0x00540EB8  _length_code[256]   256 bytes
 *     0x00540FB8  base_length[29]     116 bytes   (+4 pad)
 *     0x00541030  base_dist[30]       120 bytes
 *
 * ending exactly at the "inflate 1.1.4 Copyright" literal (0x005410A8), which
 * is what pins the chain.  Every initialiser below was read out of the retail
 * PE's .rdata and compared element for element against zlib 1.1.4's trees.h
 * (regenerated the way tr_static_init / gen_codes build it): all ten tables
 * match, including _length_code[255] == 28, the trees.c overwrite that makes
 * match length 258 use code 285 rather than 284 + 5 extra bits.
 *
 * Element types are zlib's -- int for the extra-bits and base tables, uch
 * (unsigned char) for bl_order, _dist_code and _length_code, ct_data for the
 * two static trees.  zlib_types.h declares each one const at its real size; it
 * has no ct_data, so it spells those two as const unsigned char arrays of the
 * same byte size; the only user outside this
 * unit (_tr_flush_block, zlib/inflate.c) takes their address and nothing else.
 *
 * There is no separate `dist_code_hi256` object.  IDA's byte_540DB8 is
 * &_dist_code[256] -- the upper half that trees.c's d_code() indexes as
 * _dist_code[256 + (dist >> 7)], with the 256 folded into the base address --
 * and the old zeroed 752-byte placeholder for it is gone; the readers in
 * zlib/inflate.c now spell it the way d_code() does.
 *
 * Only deflate reads these, and `compress` (0x004A2CC0) has no xrefs in
 * retail, so nothing in the game reaches them.  deflate_fast and deflate_slow
 * (universal/memorytree.cpp) read _length_code and _dist_code directly, as
 * retail does, instead of the private copies they used to build at run time.
 *
 * extra_blbits has no symbol in our tree because retail's 0x00540760 has no
 * xref for IDA to name; it is `local` in trees.c, so it is file-static here,
 * with the contents the image holds.
 *
 * ==========================================================================
 * z_errmsg  @ 0x00571BA0
 * ==========================================================================
 *
 * zutil.c's `const char *z_errmsg[10]`, 40 bytes, ending exactly where
 * static_l_desc begins:
 *
 *     0x00571BA0 -> 0x00559218 "need dictionary"       Z_NEED_DICT      2
 *     0x00571BA4 -> 0x0055920C "stream end"            Z_STREAM_END     1
 *     0x00571BA8 -> 0x00559228 ""                      Z_OK             0
 *     0x00571BAC -> 0x00559200 "file error"            Z_ERRNO         -1
 *     0x00571BB0 -> 0x005591F0 "stream error"          Z_STREAM_ERROR  -2
 *     0x00571BB4 -> 0x005591E4 "data error"            Z_DATA_ERROR    -3
 *     0x00571BB8 -> 0x005591D0 "insufficient memory"   Z_MEM_ERROR     -4
 *     0x00571BBC -> 0x005591C0 "buffer error"          Z_BUF_ERROR     -5
 *     0x00571BC0 -> 0x005591A8 "incompatible version"  Z_VERSION_ERROR -6
 *     0x00571BC4 -> 0x00559228 ""
 *
 * THE NAME IS NOT AT THE BASE.  IDA's `z_errmsg` label sits at 0x00571BA8,
 * which is &z_errmsg[2]: ERR_MSG(err) is z_errmsg[Z_NEED_DICT - err] and the
 * compiler folded the constant 2 into the base address, so the only xref in the
 * image points eight bytes in.  Defining the object here under the name the source uses puts the base back where zlib
 * expects it, and zlib/unzip.c's zError is spelled as zutil.c spells it.
 * off_571BA0, off_571BB0, off_571BB8 and off_571BBC are IDA labels on interior
 * elements of this same array, not separate objects; nothing references them.
 */

typedef struct ct_data_s {
	unsigned short fc;      /* freq / code  */
	unsigned short dl;      /* dad  / len   */
} ct_data;

typedef struct static_tree_desc_s {
	const ct_data *static_tree;
	const int     *extra_bits;
	int            extra_base;
	int            elems;
	int            max_length;
} static_tree_desc;

/* trees.h: extra bits for each length code.  Retail 0x00540670. */
const int extra_lbits[29] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

/* trees.h: extra bits for each distance code.  Retail 0x005406E8. */
const int extra_dbits[30] = {
	 0,  0,  0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,
	 7,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13
};

/* 0x00540760 -- unnamed in the IDB, `local` in trees.c. */
static const int extra_blbits[19] =
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7 };

/* trees.c: order of the bit length code lengths.  Retail 0x005407AC. */
const unsigned char bl_order[19] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* trees.h: the fixed literal/length tree, {code, len}.  Retail 0x005407C0. */
const ct_data static_ltree[288] = {
	{ 12, 8}, {140, 8}, { 76, 8}, {204, 8}, { 44, 8}, {172, 8}, {108, 8}, {236, 8},
	{ 28, 8}, {156, 8}, { 92, 8}, {220, 8}, { 60, 8}, {188, 8}, {124, 8}, {252, 8},
	{  2, 8}, {130, 8}, { 66, 8}, {194, 8}, { 34, 8}, {162, 8}, { 98, 8}, {226, 8},
	{ 18, 8}, {146, 8}, { 82, 8}, {210, 8}, { 50, 8}, {178, 8}, {114, 8}, {242, 8},
	{ 10, 8}, {138, 8}, { 74, 8}, {202, 8}, { 42, 8}, {170, 8}, {106, 8}, {234, 8},
	{ 26, 8}, {154, 8}, { 90, 8}, {218, 8}, { 58, 8}, {186, 8}, {122, 8}, {250, 8},
	{  6, 8}, {134, 8}, { 70, 8}, {198, 8}, { 38, 8}, {166, 8}, {102, 8}, {230, 8},
	{ 22, 8}, {150, 8}, { 86, 8}, {214, 8}, { 54, 8}, {182, 8}, {118, 8}, {246, 8},
	{ 14, 8}, {142, 8}, { 78, 8}, {206, 8}, { 46, 8}, {174, 8}, {110, 8}, {238, 8},
	{ 30, 8}, {158, 8}, { 94, 8}, {222, 8}, { 62, 8}, {190, 8}, {126, 8}, {254, 8},
	{  1, 8}, {129, 8}, { 65, 8}, {193, 8}, { 33, 8}, {161, 8}, { 97, 8}, {225, 8},
	{ 17, 8}, {145, 8}, { 81, 8}, {209, 8}, { 49, 8}, {177, 8}, {113, 8}, {241, 8},
	{  9, 8}, {137, 8}, { 73, 8}, {201, 8}, { 41, 8}, {169, 8}, {105, 8}, {233, 8},
	{ 25, 8}, {153, 8}, { 89, 8}, {217, 8}, { 57, 8}, {185, 8}, {121, 8}, {249, 8},
	{  5, 8}, {133, 8}, { 69, 8}, {197, 8}, { 37, 8}, {165, 8}, {101, 8}, {229, 8},
	{ 21, 8}, {149, 8}, { 85, 8}, {213, 8}, { 53, 8}, {181, 8}, {117, 8}, {245, 8},
	{ 13, 8}, {141, 8}, { 77, 8}, {205, 8}, { 45, 8}, {173, 8}, {109, 8}, {237, 8},
	{ 29, 8}, {157, 8}, { 93, 8}, {221, 8}, { 61, 8}, {189, 8}, {125, 8}, {253, 8},
	{ 19, 9}, {275, 9}, {147, 9}, {403, 9}, { 83, 9}, {339, 9}, {211, 9}, {467, 9},
	{ 51, 9}, {307, 9}, {179, 9}, {435, 9}, {115, 9}, {371, 9}, {243, 9}, {499, 9},
	{ 11, 9}, {267, 9}, {139, 9}, {395, 9}, { 75, 9}, {331, 9}, {203, 9}, {459, 9},
	{ 43, 9}, {299, 9}, {171, 9}, {427, 9}, {107, 9}, {363, 9}, {235, 9}, {491, 9},
	{ 27, 9}, {283, 9}, {155, 9}, {411, 9}, { 91, 9}, {347, 9}, {219, 9}, {475, 9},
	{ 59, 9}, {315, 9}, {187, 9}, {443, 9}, {123, 9}, {379, 9}, {251, 9}, {507, 9},
	{  7, 9}, {263, 9}, {135, 9}, {391, 9}, { 71, 9}, {327, 9}, {199, 9}, {455, 9},
	{ 39, 9}, {295, 9}, {167, 9}, {423, 9}, {103, 9}, {359, 9}, {231, 9}, {487, 9},
	{ 23, 9}, {279, 9}, {151, 9}, {407, 9}, { 87, 9}, {343, 9}, {215, 9}, {471, 9},
	{ 55, 9}, {311, 9}, {183, 9}, {439, 9}, {119, 9}, {375, 9}, {247, 9}, {503, 9},
	{ 15, 9}, {271, 9}, {143, 9}, {399, 9}, { 79, 9}, {335, 9}, {207, 9}, {463, 9},
	{ 47, 9}, {303, 9}, {175, 9}, {431, 9}, {111, 9}, {367, 9}, {239, 9}, {495, 9},
	{ 31, 9}, {287, 9}, {159, 9}, {415, 9}, { 95, 9}, {351, 9}, {223, 9}, {479, 9},
	{ 63, 9}, {319, 9}, {191, 9}, {447, 9}, {127, 9}, {383, 9}, {255, 9}, {511, 9},
	{  0, 7}, { 64, 7}, { 32, 7}, { 96, 7}, { 16, 7}, { 80, 7}, { 48, 7}, {112, 7},
	{  8, 7}, { 72, 7}, { 40, 7}, {104, 7}, { 24, 7}, { 88, 7}, { 56, 7}, {120, 7},
	{  4, 7}, { 68, 7}, { 36, 7}, {100, 7}, { 20, 7}, { 84, 7}, { 52, 7}, {116, 7},
	{  3, 8}, {131, 8}, { 67, 8}, {195, 8}, { 35, 8}, {163, 8}, { 99, 8}, {227, 8}
};

/* trees.h: the fixed distance tree, {code, len}.  Retail 0x00540C40. */
const ct_data static_dtree[30] = {
	{ 0, 5}, {16, 5}, { 8, 5}, {24, 5}, { 4, 5}, {20, 5}, {12, 5}, {28, 5},
	{ 2, 5}, {18, 5}, {10, 5}, {26, 5}, { 6, 5}, {22, 5}, {14, 5}, {30, 5},
	{ 1, 5}, {17, 5}, { 9, 5}, {25, 5}, { 5, 5}, {21, 5}, {13, 5}, {29, 5},
	{ 3, 5}, {19, 5}, {11, 5}, {27, 5}, { 7, 5}, {23, 5}
};

/* trees.h: distance -> dist code.  [0..255] for distances 0..255, [256..511]
 * for (dist >> 7), which is d_code()'s upper half.  Retail 0x00540CB8; IDA's
 * byte_540DB8 is &_dist_code[256]. */
const unsigned char _dist_code[512] = {
	 0,  1,  2,  3,  4,  4,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,
	 8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9, 10, 10, 10, 10, 10, 10, 10, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
	11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
	14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,  0,  0, 16, 17,
	18, 18, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22,
	23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
	28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
	29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29
};

/* trees.h: match length - MIN_MATCH -> length code.  Retail 0x00540EB8. */
const unsigned char _length_code[256] = {
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 12, 12,
	13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16,
	17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19,
	19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
	21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22,
	22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23,
	23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
	25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
	26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
	27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28
};

/* trees.h: first normalized length for each code (0 = MIN_MATCH).
 * Retail 0x00540FB8. */
const int base_length[29] = {
	  0,   1,   2,   3,   4,   5,   6,   7,   8,  10,  12,  14,  16,  20,  24,
	 28,  32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224,   0
};

/* trees.h: first normalized distance for each code (0 = distance of 1).
 * Retail 0x00541030. */
const int base_dist[30] = {
	    0,     1,     2,     3,     4,     6,     8,    12,    16,    24,
	   32,    48,    64,    96,   128,   192,   256,   384,   512,   768,
	 1024,  1536,  2048,  3072,  4096,  6144,  8192, 12288, 16384, 24576
};

/* 0x00571BC8  static_l_desc */
static_tree_desc static_l_desc = {
	static_ltree, extra_lbits, 257, 286, 15
};

/* 0x00571BDC  static_d_desc */
static_tree_desc static_d_desc = {
	static_dtree, extra_dbits, 0, 30, 15
};

/* 0x00571BF0  static_bl_desc */
static_tree_desc static_bl_desc = {
	(const ct_data *)0, extra_blbits, 0, 19, 7
};

/* 0x00571BA0  z_errmsg[10] */
const char *z_errmsg[10] = {
	"need dictionary",
	"stream end",
	"",
	"file error",
	"stream error",
	"data error",
	"insufficient memory",
	"buffer error",
	"incompatible version",
	""
};
