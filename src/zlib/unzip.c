/*
 * Machine-translated from Call of Duty 1.1 (Windows, CoDMP.exe).
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/zlib/unzip.c
 *
 * Retail range 0x004A87C0-0x004A99A0, 30 functions: 30 translated, 0 stubbed.
 *
 * @fidelity: verified
 *
 * TAKEN OUT OF THE GENERATOR DELIBERATELY, for the same reason zlib/inflate.c
 * was.  `verified` here means checked; the bodies are now minizip's (zlib
 * 1.1.4 contrib/minizip, the Quake3/RTCW variant with unzReOpen and the
 * file-info-position pair) text with minizip's names over unz_s /
 * file_in_zip_read_info_s / z_stream (naming pass 2026-10-14, byte-identical
 * under VC7).  Three things in this file cannot survive a
 * regeneration: the real prototypes added below (the generator emits
 * `extern int f();`, which switches argument checking off), the note that
 * unzReadCurrentFile and unzGetCurrentFileInfo are in REGISTER order rather
 * than minizip's, and the (crc, len, buf) order of the crc32 call, which
 * universal/memorytree.cpp's crc32 signature is deliberately matched to.
 * Regenerating this file silently reverts all three.
 *
 * ---------------------------------------------------------------------------
 * VERIFICATION PASS.  Every function below now carries a `VERIFIED <addr>`
 * line naming what was actually checked.  Two things back them up:
 *
 *  1. BEHAVIOURAL.  zlib/unz_compat.c + zlib/unzip.c + zlib/inflate.c were
 *     linked into a standalone harness and driven through exactly the sequence
 *     universal/com_files.c uses -- Unz_Open, GetGlobalNumFiles, GoToFirstFile/
 *     NextFile, GetCurrentFileInfo, GetCurrentFileInfoPosition, then per entry
 *     the FS_FOpenFileRead path ReOpen -> SetCurrentFileInfoPosition ->
 *     OpenCurrentFile -> ReadCurrentFile (in 7919-byte chunks) ->
 *     CloseCurrentFile.  Against the nine STOCK retail paks in
 *     "D:/Call of Duty Clean/main": 14,493 non-empty entries, every one
 *     matching its central-directory CRC under an independent crc32, and every
 *     unzCloseCurrentFile returning 0 -- so unzip.c's OWN running CRC (through
 *     crc32 at 0x004A2CE0) agreed too.  Zero failures.  Coverage includes all
 *     three deflate block types and sizes from 1 byte to 45 MB.
 *
 *  2. STATIC.  The functions the harness cannot reach (error paths, and the
 *     ten entry points with no xrefs in retail at all) were read against the
 *     disassembly and against upstream zlib 1.1.4 / minizip.
 *
 * WHAT IS UPSTREAM AND WHAT IS NOT.  This is stock minizip except for four
 * places, all noted at their sites: unzReOpen and unzGet/SetCurrentFileInfo-
 * Position are id/IW additions that do not exist upstream; unzlocal_getShort/
 * getLong were rewritten to a single little-endian fread that cannot fail;
 * and zcalloc routes through Z_MallocInternal instead of malloc.
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
 * Functions this unit calls that live elsewhere.  REAL prototypes, not the
 * generator's `extern int f();` -- an empty parameter list switches argument
 * checking off entirely, which is the single most expensive defect class in
 * this tree.  Each was checked against its definition:
 *
 *   inflate / inflateEnd / inflateInit2_   zlib/inflate.c, same file set
 *   crc32 (0x004A2CE0)                universal/memorytree.cpp
 *
 * This is zlib's crc32(crc, buf, len).  Retail is __usercall crc@<eax>,
 * buf@<ecx>, len@<edx>; the declaration order still matters: LTCG evaluates
 * the register arguments right to left, so callers load len before buf
 * (unzReadCurrentFile 0x004A95C0).  The two call sites below and
 * memorytree.cpp's definition move together or not at all.
 */
extern int __cdecl inflate( z_streamp strm, int flush );
extern int __cdecl inflateEnd( z_streamp strm );
extern int __cdecl inflateInit2_( z_streamp strm, int windowBits,
                                  const char *version, int stream_size );
extern int __cdecl crc32( int crc, unsigned char *buf, unsigned int len );

/* minizip unzip.h */
typedef void *unzFile;

#define UNZ_OK                  (0)
#define UNZ_END_OF_LIST_OF_FILE (-100)
#define UNZ_ERRNO               (Z_ERRNO)
#define UNZ_EOF                 (0)
#define UNZ_PARAMERROR          (-102)
#define UNZ_BADZIPFILE          (-103)
#define UNZ_INTERNALERROR       (-104)
#define UNZ_CRCERROR            (-105)

/* minizip unzip.c */
#define CASESENSITIVITYDEFAULTVALUE 2
#define UNZ_BUFSIZE (16384)
#define UNZ_MAXFILENAMEINZIP (256)
#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)
#define BUFREADCOMMENT (0x400)

/* Defined below, but called before their definitions. */
int __cdecl unzGoToFirstFile( unzFile file );
int __cdecl unzCloseCurrentFile( unzFile file );
int __cdecl unzGoToNextFile( unzFile file );

/* z_stream (56 bytes, the size inflateInit2_ checks) is zlib_types.h's. */

/* minizip's own types (zlib 1.1.4 contrib/minizip), sized as retail's
 * malloc(0x80) / malloc(0x6C) and the field offsets used below. */
typedef struct tm_unz_s {
  unsigned int tm_sec;
  unsigned int tm_min;
  unsigned int tm_hour;
  unsigned int tm_mday;
  unsigned int tm_mon;
  unsigned int tm_year;
} tm_unz;

typedef struct unz_global_info_s {
  unsigned long number_entry;
  unsigned long size_comment;
} unz_global_info;

typedef struct unz_file_info_s {
  unsigned long version;
  unsigned long version_needed;
  unsigned long flag;
  unsigned long compression_method;
  unsigned long dosDate;
  unsigned long crc;
  unsigned long compressed_size;
  unsigned long uncompressed_size;
  unsigned long size_filename;
  unsigned long size_file_extra;
  unsigned long size_file_comment;
  unsigned long disk_num_start;
  unsigned long internal_fa;
  unsigned long external_fa;
  tm_unz tmu_date;
} unz_file_info;

typedef struct unz_file_info_internal_s {
  unsigned long offset_curfile;
} unz_file_info_internal;

typedef struct {
  char *read_buffer;
  z_stream stream;
  unsigned long pos_in_zipfile;
  unsigned long stream_initialised;
  unsigned long offset_local_extrafield;
  unsigned int size_local_extrafield;
  unsigned long pos_local_extrafield;
  unsigned long crc32;
  unsigned long crc32_wait;
  unsigned long rest_read_compressed;
  unsigned long rest_read_uncompressed;
  FILE *file;
  unsigned long compression_method;
  unsigned long byte_before_the_zipfile;
} file_in_zip_read_info_s;

typedef struct {
  FILE *file;
  unz_global_info gi;
  unsigned long byte_before_the_zipfile;
  unsigned long num_file;
  unsigned long pos_in_central_dir;
  unsigned long current_file_ok;
  unsigned long central_pos;
  unsigned long size_central_dir;
  unsigned long offset_central_dir;
  unz_file_info cur_file_info;
  unz_file_info_internal cur_file_info_internal;
  file_in_zip_read_info_s *pfile_in_zip_read;
} unz_s;

/* ---- uncompress  0x004A87C0 ---- */
// uncompress -- CONFIRMED 90. uncompr.c: builds a 56-byte z_stream on the stack, inflateInit_ / inflate(Z_FINISH) / inflateEnd.
/*
 * VERIFIED 0x004A87C0 against upstream zlib 1.1.4 uncompr.c, now written as
 * upstream's text (dest, destLen, source, sourceLen) over a real z_stream.
 * Retail's registers: source@eax, sourceLen@ecx, dest@edx, destLen@edi.
 * inflateInit2_ is declared with upstream's (strm, windowBits, version,
 * stream_size) parameter order (inflate.c matches); that order is what sets
 * up the register arguments as retail does.
 * DEAD IN RETAIL -- no xrefs.  CoD1 never calls uncompress(); the pak path
 * goes through unzReadCurrentFile.  Verified but not exercised.
 */
int __cdecl uncompress(
        unsigned char *dest,
        unsigned long *destLen,
        const unsigned char *source,
        unsigned long sourceLen)
{
  z_stream stream;
  int err;

  stream.next_in = (unsigned char *)source;
  stream.avail_in = (unsigned int)sourceLen;
  stream.next_out = dest;
  stream.avail_out = (unsigned int)*destLen;
  stream.zalloc = 0;
  stream.zfree = 0;

  err = inflateInit2_(&stream, MAX_WBITS, ZLIB_VERSION, sizeof(stream));
  if ( err != Z_OK )
    return err;

  err = inflate(&stream, Z_FINISH);
  if ( err != Z_STREAM_END )
  {
    inflateEnd(&stream);
    return err == Z_OK ? Z_BUF_ERROR : err;
  }
  *destLen = stream.total_out;

  err = inflateEnd(&stream);
  return err;
}

/* ---- unzlocal_getShort  0x004A8840 ---- */
// unzlocal_getShort -- CONFIRMED 92. minizip unzip.c: fread of 2 bytes into a zeroed dword. (zlib 1.1.4, per zlibVersion at 0x4A9950.)
/*
 * VERIFIED 0x004A8840 instruction by instruction.  Thirteen instructions:
 *   push ecx                       <- allocates the 4-byte scratch, uninitialised
 *   fread(&scratch, 2, 1, eax)     <- FILE* is the EAX register argument
 *   movsx edx, word ptr [scratch]  <- SIGN extension, so `(__int16)` is right
 *   mov [arg_0], edx ; xor eax,eax <- unconditional UNZ_OK
 * The scratch is a plain uninitialised `short` (the `push ecx`).  Retail's
 * inlined copies in unzOpen and unzlocal_GetCurrentFileInfoInternal share
 * their caller's scratch slots, so a short read there keeps an earlier
 * field's bytes; the modern build calls getShort/getLong out of line and gets
 * this call's own uninitialised scratch instead (truncated archives only).
 *
 * DIVERGES FROM UPSTREAM MINIZIP, which builds the value a byte at a time
 * through unzlocal_getByte and returns UNZ_ERRNO/UNZ_EOF on a short read.
 * Retail does one little-endian fread and CANNOT fail (Q3's unzip.c form, minus
 * its LittleShort) -- so the callers' upstream `if (... != UNZ_OK) err =
 * UNZ_ERRNO;` tests are kept in the source and compile away.  Little-endian
 * host assumed.
 *
 * DEAD IN RETAIL -- no xrefs; every call site was inlined.
 */
int __cdecl unzlocal_getShort(FILE *fin, unsigned long *pX)
{
  short v;

  fread(&v, sizeof(v), 1, fin);

  *pX = v;
  return 0;
}

/* ---- unzlocal_getLong  0x004A8870 ---- */
// unzlocal_getLong -- CONFIRMED 92. fread of 4 bytes; the 2/4 size pair against an identical body fixes which is which.
/* VERIFIED 0x004A8870.  Identical to unzlocal_getShort above but with
 * ElementSize 4 and no sign extension (the whole dword is the value).  Same
 * divergence from upstream, same dead-in-retail status (no xrefs). */
int __cdecl unzlocal_getLong(FILE *fin, unsigned long *pX)
{
  int v;

  fread(&v, sizeof(v), 1, fin);

  *pX = v;
  return 0;
}

/* ---- strcmpcasenosensitive_internal  0x004A8890 ---- */
// strcmpcasenosensitive_internal -- CONFIRMED 92. minizip's own upper-casing comparison; verbatim.
/* VERIFIED 0x004A8890 against upstream minizip strcmpcasenosensitive_internal,
 * term for term including the a-z/A-Z fold by subtracting 32 and the three-way
 * result.  Reached only from unzLocateFile, which is itself dead. */
int __cdecl strcmpcasenosensitive_internal(const char *fileName1, const char *fileName2)
{
  for ( ;; )
  {
    char c1 = *(fileName1++);
    char c2 = *(fileName2++);
    if ( c1 >= 'a' && c1 <= 'z' )
      c1 -= 0x20;
    if ( c2 >= 'a' && c2 <= 'z' )
      c2 -= 0x20;
    if ( c1 == '\0' )
      return ( c2 == '\0' ) ? 0 : -1;
    if ( c2 == '\0' )
      return 1;
    if ( c1 < c2 )
      return -1;
    if ( c1 > c2 )
      return 1;
  }
}

/* ---- unzStringFileNameCompare  0x004A88D0 ---- */
// unzStringFileNameCompare -- CONFIRMED 95. iCaseSensitivity == 1 selects strcmp, otherwise the case-insensitive helper. minizip verbatim.
/*
 * VERIFIED 0x004A88D0 instruction by instruction.  Retail is __usercall
 * fileName1@<edx>, fileName2@<eax>, iCaseSensitivity@<ecx>.  Both of upstream's
 * non-1 cases (0 -> CASESENSITIVITYDEFAULTVALUE, and 2) fall into the same
 * `jmp strcmpcasenosensitive_internal` tail (retail keeps upstream's `== 0`
 * test as a separate `test ecx,ecx / je`); the ==1 case is an INLINED strcmp ending in
 * `sbb eax,eax / sbb eax,-1`, i.e. sign(*edx - *eax).
 *
 * Parameter order is upstream's (fileName1, fileName2) here and in
 * strcmpcasenosensitive_internal: the registers are the same either way, but
 * unzLocateFile's inlined call sets eax before edx only in this order (the
 * recovered (a1, a2) swap set them up the other way round).
 * DEAD IN RETAIL -- no xrefs.
 */
int __cdecl unzStringFileNameCompare(const char *fileName1, const char *fileName2, int iCaseSensitivity)
{
  if ( iCaseSensitivity == 0 )
    iCaseSensitivity = 2;   /* CASESENSITIVITYDEFAULTVALUE */
  if ( iCaseSensitivity == 1 )
    return strcmp(fileName1, fileName2);
  return strcmpcasenosensitive_internal(fileName1, fileName2);
}

/* ---- unzlocal_SearchCentralDir  0x004A8920 ---- */
// unzlocal_SearchCentralDir -- CONFIRMED 88. Backward scan for the end-of-central-directory signature. Sole caller is unzOpen.
/*
 * VERIFIED 0x004A8920 against upstream minizip unzlocal_SearchCentralDir, and
 * BEHAVIOURALLY on all nine stock retail paks -- every one located its
 * end-of-central-directory record and reported the right entry count (12,816
 * for pak0 down to 3 for pak6), which is this function's whole job.
 *
 * Upstream's BUFREADCOMMENT is 0x400, hence the malloc(0x404) and the
 * 1028-byte read clamp; the 0xFFFF cap is upstream's uMaxBack.  The 'P','K',5,6
 * literal at 0x50/0x4b/5/6 is the EOCD signature.  Upstream's text (uLong
 * file offset returned, TRYFREE) gives retail's bytes.
 *
 * NOTE the unsigned arithmetic is load-bearing: the offsets are all uLong, so
 * `uBackRead + 0x400 > uMaxBack` cannot go negative on a pak larger than 2 GB.
 */
unsigned long __cdecl unzlocal_SearchCentralDir(FILE *fin)
{
  unsigned char *buf;
  unsigned long uSizeFile;
  unsigned long uBackRead;
  unsigned long uMaxBack = 0xffff; /* maximum size of global comment */
  unsigned long uPosFound = 0;

  if ( fseek(fin, 0, SEEK_END) != 0 )
    return 0;

  uSizeFile = ftell(fin);

  if ( uMaxBack > uSizeFile )
    uMaxBack = uSizeFile;

  buf = (unsigned char *)malloc(BUFREADCOMMENT + 4);
  if ( buf == NULL )
    return 0;

  uBackRead = 4;
  while ( uBackRead < uMaxBack )
  {
    unsigned long uReadSize, uReadPos;
    int i;
    if ( uBackRead + BUFREADCOMMENT > uMaxBack )
      uBackRead = uMaxBack;
    else
      uBackRead += BUFREADCOMMENT;
    uReadPos = uSizeFile - uBackRead;

    uReadSize = ((BUFREADCOMMENT + 4) < (uSizeFile - uReadPos)) ?
                (BUFREADCOMMENT + 4) : (uSizeFile - uReadPos);
    if ( fseek(fin, uReadPos, SEEK_SET) != 0 )
      break;

    if ( fread(buf, (unsigned int)uReadSize, 1, fin) != 1 )
      break;

    for ( i = (int)uReadSize - 3; (i--) > 0; )
      if ( ((*(buf + i)) == 0x50) && ((*(buf + i + 1)) == 0x4b) &&
           ((*(buf + i + 2)) == 0x05) && ((*(buf + i + 3)) == 0x06) )
      {
        uPosFound = uReadPos + i;
        break;
      }

    if ( uPosFound != 0 )
      break;
  }
  if ( buf )
    free(buf);
  return uPosFound;
}

/* ---- unzReOpen  0x004A8A50 ---- */
// unzReOpen -- CONFIRMED 90. The id-added entry point: fopen(path,"rb"), malloc(0x80), Com_Memcpy the existing unz_s over it and swap in the new FILE*. Pins sizeof(unz_s) = 128.
/*
 * VERIFIED 0x004A8A50 instruction by instruction, 26 instructions.  Retail is
 * __usercall path@<eax> with the unz_s pointer as the single stack argument,
 * so (path, file) IS the source order:
 *     4a8a56  push eax          ; FileName  -> path
 *     4a8a72  mov ecx,[arg_0]   ; the unz_s -> file
 *     4a8a76  push 80h / push ecx / push esi / call Com_Memcpy
 * i.e. Com_Memcpy(dest = the new 128-byte block, src = file, 128).  ARGUMENT
 * ORDER OF THE Com_Memcpy CALL IS THEREFORE (dest, src, count) and matches
 * qcommon.h.  `mov [esi+7Ch],0` is pfile_in_zip_read = NULL on the copy.
 *
 * NOT UPSTREAM.  minizip has no unzReOpen; it is an id/IW addition so the
 * filesystem can hold several concurrently-open files inside one pak, each
 * with its own FILE* but sharing the parsed central directory.  This is the
 * function that makes sizeof(unz_s) == 128 a hard fact rather than a count.
 *
 * DIVERGENCE FROM ANY SANE VERSION: retail does not check malloc's return.
 * Kept, because the binary wins.
 *
 * BEHAVIOURALLY EXERCISED 14,493 times -- it is the first call on the
 * FS_FOpenFileRead pak path (universal/com_files.c:1274).
 */
unzFile __cdecl unzReOpen(const char *path, unzFile file)
{
  unz_s *s;
  FILE *fin;

  fin = fopen(path, "rb");
  if ( fin == NULL )
    return NULL;

  s = (unz_s *)malloc(sizeof(unz_s));
  Com_Memcpy(s, (unz_s *)file, sizeof(unz_s));

  s->file = fin;
  s->pfile_in_zip_read = NULL;

  return (unzFile)s;
}

/* ---- unzOpen  0x004A8AA0 ---- */
// unzOpen -- CONFIRMED 92. minizip verbatim; reads the end-of-central-directory record via unzlocal_getShort/getLong.
/*
 * VERIFIED 0x004A8AA0 against upstream minizip unzOpen, field by field, and
 * BEHAVIOURALLY on all nine stock paks.
 *
 * The unz_s layout it builds is now pinned end to end (dword indices):
 *      0 file          1 gi.number_entry     2 gi.size_comment
 *      3 byte_before_the_zipfile             4 num_file
 *      5 pos_in_central_dir                  6 current_file_ok
 *      7 central_pos   8 size_central_dir    9 offset_central_dir
 *     10..29 cur_file_info (20 dwords, the 0x50 copy in the Internal)
 *     30 cur_file_info_internal.offset_curfile
 *     31 pfile_in_zip_read                   -- 32 dwords, 0x80 bytes
 * which is exactly the malloc(0x80) here and in unzReOpen, and makes the
 * `+124` in unzClose/unzReadCurrentFile/unzCloseCurrentFile dword 31.  The
 * unz_s typedef at the top of this file spells that layout out.
 *
 * byte_before_the_zipfile = central_pos - (offset_central_dir + size_central_dir)
 * is the self-extracting-stub allowance.  Checked as nonzero-safe: it is 0 for
 * all nine stock paks and is added to every fseek offset downstream.
 *
 * Upstream's text, per-field `!= UNZ_OK` checks included: retail's
 * unzlocal_getShort/getLong cannot return anything but UNZ_OK (see their note
 * above), so those tests compile away and only the single-disk test, the
 * `central_pos < offset+size` sanity test and SearchCentralDir's 0 survive.
 *
 * SIGN NOTE: number_entry and size_comment come in through getShort's `short`,
 * i.e. movsx, so a pak with more than 32,767 entries would
 * store a negative count.  That is retail's behaviour, matched here; the
 * largest stock pak is 12,816 entries.  Do not widen it to movzx -- the
 * divergence would be ours, not the binary's.
 */
unzFile __cdecl unzOpen(const char *path)
{
  unz_s us;
  unz_s *s;
  unsigned long central_pos, uL;
  FILE *fin;

  unsigned long number_disk;
  unsigned long number_disk_with_CD;
  unsigned long number_entry_CD;

  int err = 0;

  fin = fopen(path, "rb");
  if ( fin == NULL )
    return NULL;

  central_pos = unzlocal_SearchCentralDir(fin);
  if ( central_pos == 0 )
    err = UNZ_ERRNO;

  if ( fseek(fin, central_pos, SEEK_SET) != 0 )
    err = UNZ_ERRNO;

  /* the signature, already checked */
  if ( unzlocal_getLong(fin, &uL) != UNZ_OK )
    err = UNZ_ERRNO;

  /* number of this disk */
  if ( unzlocal_getShort(fin, &number_disk) != UNZ_OK )
    err = UNZ_ERRNO;

  /* number of the disk with the start of the central directory */
  if ( unzlocal_getShort(fin, &number_disk_with_CD) != UNZ_OK )
    err = UNZ_ERRNO;

  /* total number of entries in the central dir on this disk */
  if ( unzlocal_getShort(fin, &us.gi.number_entry) != UNZ_OK )
    err = UNZ_ERRNO;

  /* total number of entries in the central dir */
  if ( unzlocal_getShort(fin, &number_entry_CD) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( (number_entry_CD != us.gi.number_entry) ||
       (number_disk_with_CD != 0) ||
       (number_disk != 0) )
    err = UNZ_BADZIPFILE;

  /* size of the central directory */
  if ( unzlocal_getLong(fin, &us.size_central_dir) != UNZ_OK )
    err = UNZ_ERRNO;

  /* offset of start of central directory with respect to the starting disk number */
  if ( unzlocal_getLong(fin, &us.offset_central_dir) != UNZ_OK )
    err = UNZ_ERRNO;

  /* zipfile comment length */
  if ( unzlocal_getShort(fin, &us.gi.size_comment) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( (central_pos < us.offset_central_dir + us.size_central_dir) &&
       (err == UNZ_OK) )
    err = UNZ_BADZIPFILE;

  if ( err != UNZ_OK )
  {
    fclose(fin);
    return NULL;
  }

  us.file = fin;
  us.byte_before_the_zipfile = central_pos -
                               (us.offset_central_dir + us.size_central_dir);
  us.central_pos = central_pos;
  us.pfile_in_zip_read = NULL;

  s = (unz_s *)malloc(sizeof(unz_s));
  *s = us;
  unzGoToFirstFile((unzFile)s);
  return (unzFile)s;
}

/* ---- unzClose  0x004A8C30 ---- */
// unzClose -- CONFIRMED 95. Calls unzCloseCurrentFile when unz_s+124 (pfile_in_zip_read) is live, then fclose + free. Returns UNZ_PARAMERROR (-102).
/* VERIFIED 0x004A8C30 against upstream minizip unzClose; identical, +124 is
 * dword 31 = pfile_in_zip_read per the layout in unzOpen.  Exercised 14,493
 * times (once per unzReOpen handle) plus nine times on the directory handle.
 * universal/com_files.c:910 records that FS_FCloseFile inlines this same body
 * rather than calling it -- the two must not drift. */
int __cdecl unzClose(unzFile file)
{
  unz_s *s;
  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;

  if ( s->pfile_in_zip_read != NULL )
    unzCloseCurrentFile(file);

  fclose(s->file);
  free(s);
  return UNZ_OK;
}

/* ---- unzGetGlobalInfo  0x004A8C60 ---- */
// unzGetGlobalInfo -- CONFIRMED 92. Copies number_entry (+4) and size_comment (+8) out. minizip verbatim.
/* VERIFIED 0x004A8C60 against upstream, and behaviourally: the count it hands
 * back matched the true entry count of all nine stock paks (12,816 / 4,858 /
 * 3,736 / 2,642 / 1,992 / 1,668 / 1,204 / 694 / 3).  This is the single field
 * Unz_GetGlobalNumFiles in zlib/unz_compat.c reads. */
int __cdecl unzGetGlobalInfo(unzFile file, unz_global_info *pglobal_info)
{
  unz_s *s;

  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  *pglobal_info = s->gi;
  return 0;
}

/* ---- unzlocal_DosDateToTmuDate  0x004A8C80 ---- */
// unzlocal_DosDateToTmuDate -- CONFIRMED 95. The exact MS-DOS date/time bit unpack, including the +1980 year base and the -1 month adjustment.
/* VERIFIED 0x004A8C80 against upstream minizip unzlocal_DosDateToTmuDate; all
 * six fields and both bases match, written in upstream's mask-and-divide
 * form on an unsigned ulDosDate, so every shift is logical, as required.  DEAD
 * IN RETAIL -- no xrefs; the unpack was INLINED into
 * unzlocal_GetCurrentFileInfoInternal below (file_info.tmu_date there), which
 * is the copy that actually runs. */
void __cdecl unzlocal_DosDateToTmuDate(uLong ulDosDate, tm_unz *ptm)
{
  uLong uDate;

  uDate = (uLong)(ulDosDate >> 16);
  ptm->tm_mday = (uInt)(uDate & 0x1f);
  ptm->tm_mon = (uInt)((((uDate) & 0x1E0) / 0x20) - 1);
  ptm->tm_year = (uInt)(((uDate & 0x0FE00) / 0x0200) + 1980);

  ptm->tm_hour = (uInt)((ulDosDate & 0xF800) / 0x800);
  ptm->tm_min = (uInt)((ulDosDate & 0x7E0) / 0x20);
  ptm->tm_sec = (uInt)(2 * (ulDosDate & 0x1f));
}

/* ---- unzlocal_GetCurrentFileInfoInternal  0x004A8CD0 ---- */
// unzlocal_GetCurrentFileInfoInternal -- CONFIRMED 92. The nine-argument workhorse behind unzGetCurrentFileInfo, unzGoToFirstFile, unzGoToNextFile and unzSetCurrentFileInfoPosition -- ten xrefs, all consistent.
/*
 * VERIFIED 0x004A8CD0 against upstream minizip unzlocal_GetCurrentFileInfo-
 * Internal, and BEHAVIOURALLY: 14,493 stock-pak entries came back with
 * filenames, sizes and CRCs that all cross-checked -- the CRC it reports here
 * is what the decompressed bytes were tested against, and the size it reports
 * is what unzOpenCurrentFile caches as rest_read_uncompressed.
 *
 * file_info is unz_file_info, 20 dwords / 0x50 bytes -- the struct copy at the
 * end and cur_file_info's dwords 10..29 inside unz_s:
 *   0 version  1 version_needed  2 flag  3 compression_method  4 dosDate
 *   5 crc      6 compressed_size 7 uncompressed_size
 *   8 size_filename  9 size_file_extra  10 size_file_comment
 *  11 disk_num_start 12 internal_fa     13 external_fa
 *  14..19 tmu_date{sec,min,hour,mday,mon,year}
 * 0x02014B50 is the central-directory header signature.
 *
 * Upstream's text, with getShort/getLong and DosDateToTmuDate inlined, gives
 * retail's bytes.  A failed comment read sets err = UNZ_ERRNO, which skips the
 * copy-out, so a short read of the comment loses pfile_info entirely rather
 * than reporting a partly-filled one (Hex-Rays showed this as an early
 * `return -1`).  CoD1 never passes a comment buffer.
 *
 * SIGN NOTE: every 2-byte field arrives through getShort's `short`, i.e. movsx --
 * see the note on unzlocal_getShort.  Consistent with the binary; a filename
 * longer than 32,767 bytes would go negative, which the ZIP format forbids.
 */
int __cdecl unzlocal_GetCurrentFileInfoInternal(
        unzFile file,
        unz_file_info *pfile_info,
        unz_file_info_internal *pfile_info_internal,
        char *szFileName,
        uLong fileNameBufferSize,
        void *extraField,
        uLong extraFieldBufferSize,
        char *szComment,
        uLong commentBufferSize)
{
  unz_s *s;
  unz_file_info file_info;
  unz_file_info_internal file_info_internal;
  int err = 0;
  unsigned long uMagic;
  long lSeek = 0;

  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  if ( fseek(s->file, s->pos_in_central_dir + s->byte_before_the_zipfile, SEEK_SET) != 0 )
    err = UNZ_ERRNO;

  /* we check the magic */
  if ( err == UNZ_OK )
  {
    if ( unzlocal_getLong(s->file, &uMagic) != UNZ_OK )
      err = UNZ_ERRNO;
    else if ( uMagic != 0x02014b50 )
      err = UNZ_BADZIPFILE;
  }

  if ( unzlocal_getShort(s->file, &file_info.version) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.version_needed) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.flag) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.compression_method) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getLong(s->file, &file_info.dosDate) != UNZ_OK )
    err = UNZ_ERRNO;

  unzlocal_DosDateToTmuDate(file_info.dosDate, &file_info.tmu_date);

  if ( unzlocal_getLong(s->file, &file_info.crc) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getLong(s->file, &file_info.compressed_size) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getLong(s->file, &file_info.uncompressed_size) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.size_filename) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.size_file_extra) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.size_file_comment) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.disk_num_start) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getShort(s->file, &file_info.internal_fa) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getLong(s->file, &file_info.external_fa) != UNZ_OK )
    err = UNZ_ERRNO;

  if ( unzlocal_getLong(s->file, &file_info_internal.offset_curfile) != UNZ_OK )
    err = UNZ_ERRNO;

  lSeek += file_info.size_filename;
  if ( (err == UNZ_OK) && (szFileName != NULL) )
  {
    unsigned long uSizeRead;
    if ( file_info.size_filename < fileNameBufferSize )
    {
      *(szFileName + file_info.size_filename) = '\0';
      uSizeRead = file_info.size_filename;
    }
    else
      uSizeRead = fileNameBufferSize;

    if ( (file_info.size_filename > 0) && (fileNameBufferSize > 0) )
      if ( fread(szFileName, (unsigned int)uSizeRead, 1, s->file) != 1 )
        err = UNZ_ERRNO;
    lSeek -= uSizeRead;
  }

  if ( (err == UNZ_OK) && (extraField != NULL) )
  {
    unsigned long uSizeRead;
    if ( file_info.size_file_extra < extraFieldBufferSize )
      uSizeRead = file_info.size_file_extra;
    else
      uSizeRead = extraFieldBufferSize;

    if ( lSeek != 0 )
    {
      if ( fseek(s->file, lSeek, SEEK_CUR) == 0 )
        lSeek = 0;
      else
        err = UNZ_ERRNO;
    }
    if ( (file_info.size_file_extra > 0) && (extraFieldBufferSize > 0) )
      if ( fread(extraField, (unsigned int)uSizeRead, 1, s->file) != 1 )
        err = UNZ_ERRNO;
    lSeek += file_info.size_file_extra - uSizeRead;
  }
  else
    lSeek += file_info.size_file_extra;

  if ( (err == UNZ_OK) && (szComment != NULL) )
  {
    unsigned long uSizeRead;
    if ( file_info.size_file_comment < commentBufferSize )
    {
      *(szComment + file_info.size_file_comment) = '\0';
      uSizeRead = file_info.size_file_comment;
    }
    else
      uSizeRead = commentBufferSize;

    if ( lSeek != 0 )
    {
      if ( fseek(s->file, lSeek, SEEK_CUR) == 0 )
        lSeek = 0;
      else
        err = UNZ_ERRNO;
    }
    if ( (file_info.size_file_comment > 0) && (commentBufferSize > 0) )
      if ( fread(szComment, (unsigned int)uSizeRead, 1, s->file) != 1 )
        err = UNZ_ERRNO;
    lSeek += file_info.size_file_comment - uSizeRead;
  }
  else
    lSeek += file_info.size_file_comment;

  if ( (err == UNZ_OK) && (pfile_info != NULL) )
    *pfile_info = file_info;

  if ( (err == UNZ_OK) && (pfile_info_internal != NULL) )
    *pfile_info_internal = file_info_internal;

  return err;
}

/* ---- unzGetCurrentFileInfo  0x004A9090 ---- */
// unzGetCurrentFileInfo -- CONFIRMED 92. Thin forwarder to the internal form.
/*
 * VERIFIED 0x004A9090.  THE PARAMETER ORDER BELOW IS REGISTER ORDER, NOT
 * MINIZIP'S -- this is one of the two functions in this file whose recovered
 * signature is a permutation of the public API, and it has already cost this
 * project hours once.  The forwarding call onto the nine-parameter internal
 * -- whose order IS canonical -- pins every slot: the recovered (a1..a8) are
 * szComment, extraFieldBufferSize, commentBufferSize, file, pfile_info,
 * szFileName, fileNameBufferSize, extraField, which is the order below.
 * zlib/unz_compat.c's Unz_GetCurrentFileInfo is the adapter that converts
 * from the canonical eight-argument order; both were exercised 14,493 times.
 * If this file is ever regenerated, that adapter must be re-checked with it.
 */
int __cdecl unzGetCurrentFileInfo(
        char *szComment,
        uLong extraFieldBufferSize,
        uLong commentBufferSize,
        unzFile file,
        unz_file_info *pfile_info,
        char *szFileName,
        uLong fileNameBufferSize,
        void *extraField)
{
  return unzlocal_GetCurrentFileInfoInternal(file, pfile_info, NULL,
                                             szFileName, fileNameBufferSize,
                                             extraField, extraFieldBufferSize,
                                             szComment, commentBufferSize);
}

/* ---- unzGoToFirstFile  0x004A90C0 ---- */
// unzGoToFirstFile -- CONFIRMED 92. pos_in_central_dir = offset_central_dir; num_file = 0. minizip verbatim.
/* VERIFIED 0x004A90C0 against upstream, and behaviourally as the first step of
 * every one of the nine pak walks.  &s->cur_file_info and
 * &s->cur_file_info_internal are bytes 40 and 120 of unz_s, which is what the
 * disassembly's `lea` operands are.  The handle arrives in ESI (`mov esi, ebx`
 * in unzOpen at 0x004A8C1A). */
int __cdecl unzGoToFirstFile(unzFile file)
{
  int err = UNZ_OK;
  unz_s *s;
  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  s->pos_in_central_dir = s->offset_central_dir;
  s->num_file = 0;
  err = unzlocal_GetCurrentFileInfoInternal(file, &s->cur_file_info,
                                            &s->cur_file_info_internal,
                                            NULL, 0, NULL, 0, NULL, 0);
  s->current_file_ok = (err == UNZ_OK);
  return err;
}

/* ---- unzGoToNextFile  0x004A9100 ---- */
// unzGoToNextFile -- CONFIRMED 92. Advances pos_in_central_dir by 46 + filename + extra + comment lengths -- 46 is sizeof(central directory header).
/* VERIFIED 0x004A9100 against upstream, and behaviourally: walking it 27,613
 * times across the nine stock paks landed on a valid 0x02014B50 header EVERY
 * time -- which is the strongest possible test of the +46 stride and of
 * size_filename/size_file_extra/size_file_comment (unz_s dwords 18/19/20,
 * cur_file_info[8]/[9]/[10]), since one wrong term desynchronises the whole
 * remaining walk. */
int __cdecl unzGoToNextFile(unzFile file)
{
  unz_s *s;
  int err;

  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  if ( !s->current_file_ok )
    return UNZ_END_OF_LIST_OF_FILE;
  if ( s->num_file + 1 == s->gi.number_entry )
    return UNZ_END_OF_LIST_OF_FILE;

  s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename +
                           s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment;
  s->num_file++;
  err = unzlocal_GetCurrentFileInfoInternal(file, &s->cur_file_info,
                                            &s->cur_file_info_internal,
                                            NULL, 0, NULL, 0, NULL, 0);
  s->current_file_ok = (err == UNZ_OK);
  return err;
}

/* ---- unzGetCurrentFileInfoPosition  0x004A9170 ---- */
// unzGetCurrentFileInfoPosition -- CONFIRMED 90. Returns unz_s->pos_in_central_dir (+20). An id addition to minizip, used by the FS to cache pak entries.
/* VERIFIED 0x004A9170.  NOT UPSTREAM -- an id/IW addition, paired with the
 * setter below so FS_LoadZipFile can cache one dword per entry at build time
 * and seek straight back to it later instead of re-walking.  +20 is dword 5 =
 * pos_in_central_dir.  Behaviourally: 14,493 positions cached on the walk and
 * fed back through unzSetCurrentFileInfoPosition, every one of which then
 * decompressed to the right bytes -- so the getter/setter pair round-trips.
 * NOTE zlib/unz_compat.c wraps this into a one-argument value-returning form
 * because that is the shape universal/com_files.c calls. */
int __cdecl unzGetCurrentFileInfoPosition(unzFile file, unsigned long *pos)
{
  unz_s *s;

  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;

  *pos = s->pos_in_central_dir;
  return UNZ_OK;
}

/* ---- unzSetCurrentFileInfoPosition  0x004A9190 ---- */
// unzSetCurrentFileInfoPosition -- CONFIRMED 92. The setter half; called by FS_FOpenFileRead_Internal and FS_Seek.
/* VERIFIED 0x004A9190.  NOT UPSTREAM -- the other half of the id/IW pair.
 * Note it returns 0 unconditionally and records success in current_file_ok
 * instead, so a caller that seeks to a bad position learns nothing from the
 * return value; unzOpenCurrentFile's `!s->current_file_ok` check is what catches
 * it.  Exercised 14,493 times on the FS_FOpenFileRead path. */
int __cdecl unzSetCurrentFileInfoPosition(unzFile file, unsigned long pos)
{
  unz_s *s;
  int err;

  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;

  s->pos_in_central_dir = pos;
  err = unzlocal_GetCurrentFileInfoInternal(file, &s->cur_file_info,
                                            &s->cur_file_info_internal,
                                            NULL, 0, NULL, 0, NULL, 0);
  s->current_file_ok = (err == UNZ_OK);
  return UNZ_OK;
}

/* ---- unzLocateFile  0x004A91D0 ---- */
// unzLocateFile -- CONFIRMED 88. Linear walk with unzStringFileNameCompare.
/* ARGUMENT ORDER.  The unzFile handle arrives in ECX (`mov esi, ecx` at
   0x004A91EB); only szFileName and iCaseSensitivity are on the stack.  The
   generator kept the two stack arguments and invented `v2` for the register,
   so the whole function operated on an uninitialised pointer -- the very first
   thing it does is `v3 = v2` and every access after that is through garbage.

   Structurally the body is stock zlib 1.1 `unzLocateFile` term for term, so
   only the missing parameter needed adding.  Field offsets confirmed against
   `unz_s`: +24 current_file_ok, +16 num_file, +20 pos_in_central_dir.

   Dead in the retail binary -- no xrefs.  CoD1's filesystem resolves pak
   entries through its own hash table and never walks the central directory
   linearly.  Fixed anyway because it was scored CONFIRMED while unable to
   work, which is worse than being unfixed.

   VERIFIED 0x004A91D0.  The missing-ECX-argument fix above re-checked against
   the disassembly (`mov esi, ecx` at 0x004A91EB with no matching stack slot)
   and the restored body re-read against upstream minizip unzLocateFile term
   for term, including the save/restore of num_file and pos_in_central_dir on
   failure and the UNZ_MAXFILENAMEINZIP 0x100 cap.  Not exercised: still dead.
   Now upstream's text (unzGetCurrentFileInfo and unzStringFileNameCompare
   inlined), byte-identical under VC7. */
int __cdecl unzLocateFile(unzFile file, const char *szFileName, int iCaseSensitivity)
{
  unz_s *s;
  int err;

  uLong num_fileSaved;
  uLong pos_in_central_dirSaved;

  if ( file == NULL )
    return UNZ_PARAMERROR;

  if ( strlen(szFileName) >= UNZ_MAXFILENAMEINZIP )
    return UNZ_PARAMERROR;

  s = (unz_s *)file;
  if ( !s->current_file_ok )
    return UNZ_END_OF_LIST_OF_FILE;

  num_fileSaved = s->num_file;
  pos_in_central_dirSaved = s->pos_in_central_dir;

  err = unzGoToFirstFile(file);

  while ( err == UNZ_OK )
  {
    char szCurrentFileName[UNZ_MAXFILENAMEINZIP + 1];
    unzGetCurrentFileInfo(NULL, 0, 0, file, NULL,
                          szCurrentFileName, sizeof(szCurrentFileName) - 1, NULL);
    if ( unzStringFileNameCompare(szCurrentFileName, szFileName, iCaseSensitivity) == 0 )
      return UNZ_OK;
    err = unzGoToNextFile(file);
  }

  s->num_file = num_fileSaved;
  s->pos_in_central_dir = pos_in_central_dirSaved;
  return err;
}

/* ---- unzlocal_CheckCurrentFileCoherencyHeader  0x004A92F0 ---- */
// unzlocal_CheckCurrentFileCoherencyHeader -- CONFIRMED 85. Validates the local file header against the central directory entry before decompression begins.
/*
 * VERIFIED 0x004A92F0 INSTRUCTION BY INSTRUCTION, all 164 of them, because
 * this body looks wrong and is not.
 *
 * Every fread lands in the incoming piSizeVar parameter's stack slot: MSVC
 * keeps the pointer in EBP for the whole body and packs the 4-byte read
 * scratch into that home slot.  Writing over the parameter itself (Hex-Rays'
 * shape) cost an extra `mov ebp,eax`; the separate `uData` local below is
 * what VC7 turns into retail's frame byte for byte.
 *
 * Ten reads, in retail's order: signature (0x04034B50), version needed
 * (discarded), flags (only bit 3 used), method, dosDate (discarded), crc,
 * compressed_size, uncompressed_size, size_filename, size_extra_field.
 * Central-directory fields compared against are unz_s +52/+60/+64/+68/+72 =
 * cur_file_info.compression_method/crc/compressed_size/uncompressed_size/
 * size_filename.  `(uFlags & 8) == 0` is upstream's "sizes live in the data
 * descriptor, do not compare" flag.  cur_file_info_internal.offset_curfile is
 * unz_s +120; SIZEZIPLOCALHEADER (30) is sizeof(local file header).
 *
 * The `compression_method && compression_method != Z_DEFLATED` check is
 * upstream's own (1.1.4 minizip tests compression_method here AND, dead,
 * again in unzOpenCurrentFile).  Upstream's text for this function (getShort/
 * getLong calls into separate locals) was tried and is byte-identical under
 * VC7 too; this form stays, with minizip's names, because its single shared
 * read scratch is what the modern build needs to reproduce retail's value on
 * a short read.
 *
 * BEHAVIOURALLY exercised 14,493 times -- it runs before every single pak read
 * and returned UNZ_OK for every stock entry, which also proves the offsets.
 */
int __cdecl unzlocal_CheckCurrentFileCoherencyHeader(unz_s *s, uInt *piSizeVar,
                                                     uLong *poffset_local_extrafield,
                                                     uInt *psize_local_extrafield)
{
  int uData;            /* read scratch: retail's getShort/getLong, inlined as raw freads */
  int err = UNZ_OK;
  int uFlags;
  int compression_method;
  int size_filename;
  int size_extra_field;

  *piSizeVar = 0;
  *poffset_local_extrafield = 0;
  *psize_local_extrafield = 0;

  if ( fseek(s->file, s->cur_file_info_internal.offset_curfile +
                      s->byte_before_the_zipfile, SEEK_SET) )
    return UNZ_ERRNO;
#ifndef COD1_VC7
  uData = (int)piSizeVar;   /* VC7 puts uData in piSizeVar's home slot: same start value on a short read */
#endif
  fread(&uData, 4u, 1u, s->file);
  if ( uData != 0x04034b50 )
    err = UNZ_BADZIPFILE;
  fread(&uData, 2u, 1u, s->file);       /* version needed, not checked */
  fread(&uData, 2u, 1u, s->file);
  uFlags = (short)uData;
  fread(&uData, 2u, 1u, s->file);
  if ( err == UNZ_OK )
  {
    compression_method = s->cur_file_info.compression_method;
    if ( (short)uData != compression_method ||
         compression_method && compression_method != Z_DEFLATED )
      err = UNZ_BADZIPFILE;
  }
  fread(&uData, 4u, 1u, s->file);       /* date/time */
  fread(&uData, 4u, 1u, s->file);       /* crc */
  if ( err == UNZ_OK && uData != s->cur_file_info.crc && (uFlags & 8) == 0 )
    err = UNZ_BADZIPFILE;
  fread(&uData, 4u, 1u, s->file);       /* size compr */
  if ( err == UNZ_OK && uData != s->cur_file_info.compressed_size && (uFlags & 8) == 0 )
    err = UNZ_BADZIPFILE;
  fread(&uData, 4u, 1u, s->file);       /* size uncompr */
  if ( err == UNZ_OK && uData != s->cur_file_info.uncompressed_size && (uFlags & 8) == 0 )
    err = UNZ_BADZIPFILE;
  fread(&uData, 2u, 1u, s->file);
  size_filename = (short)uData;
  if ( err == UNZ_OK && (short)uData != s->cur_file_info.size_filename )
    err = UNZ_BADZIPFILE;
  *piSizeVar += (short)uData;
  fread(&uData, 2u, 1u, s->file);
  size_extra_field = (short)uData;
  *poffset_local_extrafield = s->cur_file_info_internal.offset_curfile +
                              size_filename + SIZEZIPLOCALHEADER;
  *psize_local_extrafield = size_extra_field;
  *piSizeVar += size_extra_field;
  return err;
}

/* ---- unzOpenCurrentFile  0x004A94A0 ---- */
// unzOpenCurrentFile -- CONFIRMED 90. Allocates the read buffer and calls inflateInit2 with a negative window size (raw deflate). Called by FS_FOpenFileRead_Internal and FS_Seek.
/*
 * VERIFIED 0x004A94A0 against upstream minizip unzOpenCurrentFile, and
 * behaviourally 14,493 times.
 *
 * malloc(0x6C) = 108 = sizeof(file_in_zip_read_info_s), and the field map that
 * the rest of this file indexes by byte offset falls straight out of it
 * (dword index -> byte):
 *    0/0   read_buffer        1..14/4..56  stream (z_stream, 56 bytes)
 *   15/60  pos_in_zipfile     16/64  stream_initialised
 *   17/68  offset_local_extrafield  18/72 size_local_extrafield
 *   19/76  pos_local_extrafield     20/80 crc32   21/84 crc32_wait
 *   22/88  rest_read_compressed     23/92 rest_read_uncompressed
 *   24/96  file (FILE*)             25/100 compression_method
 *   26/104 byte_before_the_zipfile
 * The +80/+84/+88/+92/+96/+100/+104 in unzReadCurrentFile and
 * unzCloseCurrentFile are all in this table.
 *
 * malloc(0x4000) is UNZ_BUFSIZE = 16 KB.  inflateInit2_ with windowBits -15
 * is upstream's raw-deflate request (nowrap), which is why no adler32 check
 * runs over pak data and the CRC in unzCloseCurrentFile is the only integrity
 * test there is -- the reason crc32 being stubbed to `return 0` mattered.
 *
 * crc32_wait <- cur_file_info.crc is the value the running CRC is finally
 * compared against; rest_read_uncompressed <- cur_file_info.uncompressed_size
 * is also what Unz_GetCurrentFileUncompressedSize in zlib/unz_compat.c reads
 * directly out of the handle.
 *
 * Retail does not free the read buffer if inflateInit2_ fails -- it only
 * leaves stream_initialised clear.  That is upstream's own behaviour.
 *
 * Upstream's text.  inflateInit2_ takes upstream's (strm, windowBits,
 * version, stream_size) order, which sets up its register arguments the
 * way retail does (see uncompress).
 */
int __cdecl unzOpenCurrentFile(unzFile file)
{
  int err = 0;
  int Store;
  unsigned int iSizeVar;
  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  unsigned long offset_local_extrafield;
  unsigned int size_local_extrafield;

  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  if ( !s->current_file_ok )
    return UNZ_PARAMERROR;

  if ( s->pfile_in_zip_read != NULL )
    unzCloseCurrentFile(file);

  if ( unzlocal_CheckCurrentFileCoherencyHeader(s, &iSizeVar,
         &offset_local_extrafield, &size_local_extrafield) != UNZ_OK )
    return UNZ_BADZIPFILE;

  pfile_in_zip_read_info = (file_in_zip_read_info_s *)malloc(sizeof(file_in_zip_read_info_s));
  if ( pfile_in_zip_read_info == NULL )
    return UNZ_INTERNALERROR;

  pfile_in_zip_read_info->read_buffer = (char *)malloc(UNZ_BUFSIZE);
  pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
  pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
  pfile_in_zip_read_info->pos_local_extrafield = 0;

  if ( pfile_in_zip_read_info->read_buffer == NULL )
  {
    if ( pfile_in_zip_read_info )
      free(pfile_in_zip_read_info);
    return UNZ_INTERNALERROR;
  }

  pfile_in_zip_read_info->stream_initialised = 0;

  if ( (s->cur_file_info.compression_method != 0) &&
       (s->cur_file_info.compression_method != Z_DEFLATED) )
    err = UNZ_BADZIPFILE;
  Store = s->cur_file_info.compression_method == 0;

  pfile_in_zip_read_info->crc32_wait = s->cur_file_info.crc;
  pfile_in_zip_read_info->crc32 = 0;
  pfile_in_zip_read_info->compression_method = s->cur_file_info.compression_method;
  pfile_in_zip_read_info->file = s->file;
  pfile_in_zip_read_info->byte_before_the_zipfile = s->byte_before_the_zipfile;

  pfile_in_zip_read_info->stream.total_out = 0;

  if ( !Store )
  {
    pfile_in_zip_read_info->stream.zalloc = 0;
    pfile_in_zip_read_info->stream.zfree = 0;
    pfile_in_zip_read_info->stream.opaque = 0;

    err = inflateInit2_(&pfile_in_zip_read_info->stream, -MAX_WBITS, ZLIB_VERSION, sizeof(z_stream));
    if ( err == Z_OK )
      pfile_in_zip_read_info->stream_initialised = 1;
  }
  pfile_in_zip_read_info->rest_read_compressed = s->cur_file_info.compressed_size;
  pfile_in_zip_read_info->rest_read_uncompressed = s->cur_file_info.uncompressed_size;

  pfile_in_zip_read_info->pos_in_zipfile =
      s->cur_file_info_internal.offset_curfile + 30 + iSizeVar;

  pfile_in_zip_read_info->stream.avail_in = 0;

  s->pfile_in_zip_read = pfile_in_zip_read_info;
  return 0;
}

/* ---- unzReadCurrentFile  0x004A95C0 ---- */
// unzReadCurrentFile -- CONFIRMED 92. Sole callee of FS_Read on the pak path; drives inflate() over the buffered stream.
/*
 * VERIFIED 0x004A95C0.  This is the function every byte the server loads comes
 * out of, and it is verified the hard way: 14,493 stock-pak entries read in
 * 7919-byte chunks, every one matching its central-directory CRC under an
 * independent crc32, covering stored / fixed-Huffman / dynamic-Huffman blocks
 * and 1 byte to 45 MB.
 *
 * ARGUMENT ORDER IS (file, len, buf) AND THAT IS NOT MINIZIP'S.  Retail is
 * __usercall file@<eax>, len@<ecx>, buf@<stack+0>:
 *     4a95e3  mov esi,[eax+7Ch]   ; file -> pfile_in_zip_read
 *     4a960a  test ecx,ecx        ; len; zero -> return 0
 *     4a9624  mov [esi+14h],ecx   ;   -> stream.avail_out
 *     4a9618  mov eax,[esp+arg_0] ; buf
 *     4a961c  mov [esi+10h],eax   ;   -> stream.next_out
 * Retail's own FS_Read agrees at 0x0042A490.  Passing minizip's (file, buf,
 * len) put the length in next_out and the pointer in avail_out, which is half
 * of the historic "FS_ReadFile returns a buffer of zeroes".
 * zlib/unz_compat.c's Unz_ReadCurrentFile is the adapter.
 *
 * `len` and the uLong/uInt fields are unsigned, so every clamp here -- len vs
 * rest_read_uncompressed, rest_read_compressed vs 0x4000, avail_in vs
 * avail_out -- is an unsigned comparison, as zlib requires.  Checked
 * deliberately: a signed compare here would truncate reads rather than crash.
 *
 * crc32 (universal/memorytree.cpp) takes zlib's (crc, buf, len).
 *
 * The body is upstream minizip's text.  UNZ_EOF is 0, so upstream's
 * `iRead==0 ? UNZ_EOF : iRead` on Z_STREAM_END is plain iRead (a read past the
 * end yields 0; universal/com_files.c's FS_Read tolerates a single zero read),
 * and any other inflate error breaks out and returns the raw zlib error
 * (FS_Read treats -1 as fatal).  The remaining VC7 differences are crc32's:
 * retail's is zlib's DO8-unrolled body (not inlined here) taking upstream's
 * (crc, buf, len) order.
 */
int __cdecl unzReadCurrentFile(unzFile file, unsigned len, void *buf)
{
  int err = 0;
  unsigned int iRead = 0;
  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  pfile_in_zip_read_info = s->pfile_in_zip_read;

  if ( pfile_in_zip_read_info == NULL )
    return UNZ_PARAMERROR;

  if ( (pfile_in_zip_read_info->read_buffer == NULL) )
    return UNZ_END_OF_LIST_OF_FILE;
  if ( len == 0 )
    return 0;

  pfile_in_zip_read_info->stream.next_out = (unsigned char *)buf;

  pfile_in_zip_read_info->stream.avail_out = (unsigned int)len;

  if ( len > pfile_in_zip_read_info->rest_read_uncompressed )
    pfile_in_zip_read_info->stream.avail_out =
      (unsigned int)pfile_in_zip_read_info->rest_read_uncompressed;

  while ( pfile_in_zip_read_info->stream.avail_out > 0 )
  {
    if ( (pfile_in_zip_read_info->stream.avail_in == 0) &&
         (pfile_in_zip_read_info->rest_read_compressed > 0) )
    {
      uInt uReadThis = UNZ_BUFSIZE;
      if ( pfile_in_zip_read_info->rest_read_compressed < uReadThis )
        uReadThis = (unsigned int)pfile_in_zip_read_info->rest_read_compressed;
      if ( uReadThis == 0 )
        return UNZ_EOF;
      if ( fseek(pfile_in_zip_read_info->file,
                 pfile_in_zip_read_info->pos_in_zipfile +
                 pfile_in_zip_read_info->byte_before_the_zipfile, SEEK_SET) != 0 )
        return UNZ_ERRNO;
      if ( fread(pfile_in_zip_read_info->read_buffer, uReadThis, 1,
                 pfile_in_zip_read_info->file) != 1 )
        return UNZ_ERRNO;
      pfile_in_zip_read_info->pos_in_zipfile += uReadThis;

      pfile_in_zip_read_info->rest_read_compressed -= uReadThis;

      pfile_in_zip_read_info->stream.next_in =
        (unsigned char *)pfile_in_zip_read_info->read_buffer;
      pfile_in_zip_read_info->stream.avail_in = (unsigned int)uReadThis;
    }

    if ( pfile_in_zip_read_info->compression_method == 0 )
    {
      unsigned int uDoCopy, i;
      if ( pfile_in_zip_read_info->stream.avail_out <
           pfile_in_zip_read_info->stream.avail_in )
        uDoCopy = pfile_in_zip_read_info->stream.avail_out;
      else
        uDoCopy = pfile_in_zip_read_info->stream.avail_in;

      for ( i = 0; i < uDoCopy; i++ )
        *(pfile_in_zip_read_info->stream.next_out + i) =
          *(pfile_in_zip_read_info->stream.next_in + i);

      pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,
                                            pfile_in_zip_read_info->stream.next_out,
                                            uDoCopy);
      pfile_in_zip_read_info->rest_read_uncompressed -= uDoCopy;
      pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
      pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
      pfile_in_zip_read_info->stream.next_out += uDoCopy;
      pfile_in_zip_read_info->stream.next_in += uDoCopy;
      pfile_in_zip_read_info->stream.total_out += uDoCopy;
      iRead += uDoCopy;
    }
    else
    {
      unsigned long uTotalOutBefore, uTotalOutAfter;
      const unsigned char *bufBefore;
      unsigned long uOutThis;
      int flush = Z_SYNC_FLUSH;

      uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
      bufBefore = pfile_in_zip_read_info->stream.next_out;

      err = inflate(&pfile_in_zip_read_info->stream, flush);

      uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
      uOutThis = uTotalOutAfter - uTotalOutBefore;

      pfile_in_zip_read_info->crc32 =
        crc32(pfile_in_zip_read_info->crc32, (unsigned char *)bufBefore,
              (unsigned int)(uOutThis));

      pfile_in_zip_read_info->rest_read_uncompressed -= uOutThis;

      iRead += (unsigned int)(uTotalOutAfter - uTotalOutBefore);

      if ( err == Z_STREAM_END )
        return (iRead == 0) ? UNZ_EOF : iRead;
      if ( err != Z_OK )
        break;
    }
  }

  if ( err == Z_OK )
    return iRead;
  return err;
}

/* ---- unztell  0x004A97A0 ---- */
// unztell -- CONFIRMED 85. Returns stream.total_out. Position in the run after unzReadCurrentFile matches minizip.
/* VERIFIED 0x004A97A0.  +124 is pfile_in_zip_read, +24 inside it is
 * stream.total_out (z_stream offset 20 plus the 4-byte read_buffer that
 * precedes the stream in file_in_zip_read_info_s).  Upstream verbatim.
 * No xrefs in retail: it is inlined into FS_FTell (byte-identical with the call). */
int __cdecl unztell(unzFile file)
{
  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  pfile_in_zip_read_info = s->pfile_in_zip_read;

  if ( pfile_in_zip_read_info == NULL )
    return UNZ_PARAMERROR;

  return (int)pfile_in_zip_read_info->stream.total_out;
}

/* ---- unzeof  0x004A97C0 ---- */
// unzeof -- CONFIRMED 85. rest_read_uncompressed == 0.
/* VERIFIED 0x004A97C0.  +92 is rest_read_uncompressed, matching the field map
 * on unzOpenCurrentFile.  Upstream verbatim.  DEAD IN RETAIL -- no xrefs. */
int __cdecl unzeof(unzFile file)
{
  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  pfile_in_zip_read_info = s->pfile_in_zip_read;

  if ( pfile_in_zip_read_info == NULL )
    return UNZ_PARAMERROR;

  if ( pfile_in_zip_read_info->rest_read_uncompressed == 0 )
    return 1;
  else
    return 0;
}

/* ---- unzGetLocalExtrafield  0x004A97E0 ---- */
// unzGetLocalExtrafield -- CONFIRMED 82. minizip verbatim.
/*
 * VERIFIED 0x004A97E0 instruction by instruction -- AND IT CONTAINS A RETAIL
 * BUFFER OVERFLOW, which is faithfully reproduced below.  DO NOT WIRE THIS UP.
 *
 * read_now is the caller's len clamped to the bytes remaining; size_to_read is
 * the UNCLAMPED remaining.  The fread reads size_to_read and the function
 * returns read_now.  Retail agrees exactly:
 *     4a980d  sub edi, eax        ; edi = size_local_extrafield - pos_...
 *     4a981a  cmp ebx, edi / jbe / mov ebx, edi   ; ebx = min(len, edi)
 *     4a9851  push edi ; ElementSize  <-- reads EDI, the UNCLAMPED count
 *     4a9863  mov eax, ebx            <-- returns EBX, the clamped count
 * so any caller passing a buffer smaller than the entry's local extra field
 * gets that field written past the end of it.  This is zlib 1.1.4 minizip's
 * own `fread(buf,(uInt)size_to_read,...)` (later minizip reads read_now), so
 * upstream's text with upstream's (file, buf, len) order is what is below;
 * retail's registers: file@eax, len@ecx, buf on the stack.
 *
 * It is harmless today only because it is DEAD IN RETAIL -- no xrefs, and
 * nothing in universal/com_files.c reaches it.  Left matching the binary
 * because the binary wins; flagged this loudly because the day someone calls
 * it, it is a stack smash and it will not look like a zlib bug.
 */
int __cdecl unzGetLocalExtrafield(unzFile file, void *buf, unsigned len)
{
  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  uInt read_now;
  uLong size_to_read;

  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  pfile_in_zip_read_info = s->pfile_in_zip_read;

  if ( pfile_in_zip_read_info == NULL )
    return UNZ_PARAMERROR;

  size_to_read = (pfile_in_zip_read_info->size_local_extrafield -
                  pfile_in_zip_read_info->pos_local_extrafield);

  if ( buf == NULL )
    return (int)size_to_read;

  if ( len > size_to_read )
    read_now = (uInt)size_to_read;
  else
    read_now = (uInt)len;

  if ( read_now == 0 )
    return 0;

  if ( fseek(pfile_in_zip_read_info->file,
             pfile_in_zip_read_info->offset_local_extrafield +
             pfile_in_zip_read_info->pos_local_extrafield, SEEK_SET) != 0 )
    return UNZ_ERRNO;

  if ( fread(buf, (uInt)size_to_read, 1, pfile_in_zip_read_info->file) != 1 )
    return UNZ_ERRNO;

  return (int)read_now;
}

/* ---- unzCloseCurrentFile  0x004A9870 ---- */
// unzCloseCurrentFile -- CONFIRMED 92. inflateEnd + frees; called by unzClose, FS_FCloseFile and FS_ShutdownSearchPaths.
/*
 * VERIFIED 0x004A9870 against upstream, and behaviourally 14,493 times -- ALL
 * OF WHICH RETURNED 0.  That is the load-bearing result in this file: the test
 * `!rest_read_uncompressed && crc32 != crc32_wait -> UNZ_CRCERROR (-105)` on
 * the first line is the pak integrity check, it now actually runs (crc32 at
 * 0x004A2CE0 was an AUTO-STUB returning 0 until today, so this comparison was
 * 0 != expected for every completely-read file), and it passes on every entry
 * of every stock pak.
 *
 * +80 crc32, +84 crc32_wait, +92 rest_read_uncompressed, +64
 * stream_initialised -- the map on unzOpenCurrentFile; &stream is byte
 * offset 4 of file_in_zip_read_info_s.
 *
 * NOTE the return value is the error, but universal/com_files.c:914 discards
 * it, so a CRC failure will NOT report itself -- it will surface as a corrupt
 * asset somewhere else.  Worth wiring to a Com_Printf one day; not changed
 * here because it is not this file's decision.
 */
int __cdecl unzCloseCurrentFile(unzFile file)
{
  int err = UNZ_OK;

  unz_s *s;
  file_in_zip_read_info_s *pfile_in_zip_read_info;
  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;
  pfile_in_zip_read_info = s->pfile_in_zip_read;

  if ( pfile_in_zip_read_info == NULL )
    return UNZ_PARAMERROR;

  if ( pfile_in_zip_read_info->rest_read_uncompressed == 0 )
  {
    if ( pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait )
      err = UNZ_CRCERROR;
  }

  free(pfile_in_zip_read_info->read_buffer);
  pfile_in_zip_read_info->read_buffer = NULL;
  if ( pfile_in_zip_read_info->stream_initialised )
    inflateEnd(&pfile_in_zip_read_info->stream);

  pfile_in_zip_read_info->stream_initialised = 0;
  free(pfile_in_zip_read_info);

  s->pfile_in_zip_read = NULL;

  return err;
}

/* ---- unzGetGlobalComment  0x004A98E0 ---- */
// unzGetGlobalComment -- CONFIRMED 82. Final function of minizip's unzip.c.
/* VERIFIED 0x004A98E0 against upstream minizip unzGetGlobalComment.  The
 * `*szComment = '\0';` that happens before the `szComment != NULL` check further down looks
 * like a defect and is upstream's own shape -- upstream writes `*szComment =
 * '\0'` inside `if (uReadThis > 0)` and only null-checks szComment for the
 * trailing terminator.  Left alone.  +8 is gi.size_comment, +28 is
 * central_pos, and 22 is sizeof(end of central directory record).
 * DEAD IN RETAIL -- no xrefs. */
int __cdecl unzGetGlobalComment(unzFile file, char *szComment, uLong uSizeBuf)
{
  unz_s *s;
  uLong uReadThis;
  if ( file == NULL )
    return UNZ_PARAMERROR;
  s = (unz_s *)file;

  uReadThis = uSizeBuf;
  if ( uReadThis > s->gi.size_comment )
    uReadThis = s->gi.size_comment;

  if ( fseek(s->file, s->central_pos + 22, SEEK_SET) != 0 )
    return UNZ_ERRNO;

  if ( uReadThis > 0 )
  {
    *szComment = '\0';
    if ( fread(szComment, (uInt)uReadThis, 1, s->file) != 1 )
      return UNZ_ERRNO;
  }

  if ( (szComment != NULL) && (uSizeBuf > s->gi.size_comment) )
    *(szComment + s->gi.size_comment) = '\0';
  return (int)uReadThis;
}

/* ---- zlibVersion  0x004A9950 ---- */
// zlibVersion -- CONFIRMED 96. Returns "1.1.4". Pins the bundled zlib at 1.1.4, so the whole inflate/inftrees/infblock block can be matched against that exact release.
/* VERIFIED 0x004A9950.  Two instructions, returning the "1.1.4" literal.  This
 * is the anchor the whole zlib verification rests on: it dates the bundled
 * library exactly, so inflate.c/infblock.c/infcodes.c/inffast.c/inftrees.c/
 * infutil.c and this unzip.c can be diffed against a specific public release
 * rather than guessed at.  DEAD IN RETAIL -- no xrefs. */
const char *zlibVersion()
{
  return ZLIB_VERSION;
}

/* ---- zError  0x004A9960 ---- */
// zError -- CONFIRMED 92. Indexes the z_errmsg table backwards from z_errmsg, i.e. z_errmsg[1-err]. zutil.c verbatim.
/*
 * VERIFIED 0x004A9960 against zutil.c's `return ERR_MSG(err);` =
 * z_errmsg[Z_NEED_DICT - err], with the compiler having folded the constant
 * into the base address -- IDA's `z_errmsg` label is at 0x00571BA8, which is
 * &z_errmsg[2], so retail's `&z_errmsg - err` is z_errmsg[2 - err].  DEAD IN
 * RETAIL -- no xrefs, and none in this tree either.
 *
 * FIXED.  The table now exists at its real base (0x00571BA0, ten `const char *`
 * -- zlib/zlib_rdata.c), so the folded constant is spelled out again rather
 * than being carried in the symbol's address.  Before that it indexed off the
 * FRONT of `extern int z_errmsg;` and returned neighbouring globals as strings.
 * The cast stays because zlib_types.h declares the name an int; the storage
 * behind it is the 40-byte table.
 */
const char *__cdecl zError(int err)
{
  return ((const char **)&z_errmsg)[Z_NEED_DICT - err];   /* ERR_MSG(err) */
}

/* ---- zcalloc  0x004A9980 ---- */
// zcalloc -- CONFIRMED 92. DIVERGENCE: stock zutil.c calls malloc; CoD routes zlib allocation through Z_MallocInternal so pak decompression shows up in the zone.
/*
 * VERIFIED 0x004A9980.  NOT UPSTREAM -- zutil.c's zcalloc calls malloc; this
 * routes through Z_MallocInternal so pak decompression is visible in the zone.
 * opaque is unused; the product size*items is commutative, so the recovered
 * register order of items/size cannot be wrong in any way that matters.
 * Reached only as a function POINTER, installed by inflateInit2_ into
 * z_stream.zalloc and called as (opaque, items, size).  Exercised on every one
 * of the 14,493 reads -- it allocates the 32 KB inflate window and the 1,440-
 * entry huft pool each time a pak file is opened.
 */
int *__cdecl zcalloc(int opaque, int items, int size)
{
  return Z_MallocInternal(size * items);
}

/* ---- zcfree  0x004A99A0 ---- */
// zcfree -- CONFIRMED 92. Plain free().
/*
 * VERIFIED 0x004A99A0.  DIVERGENCE WORTH KNOWING, and it is retail's: zcalloc
 * above allocates through Z_MallocInternal but zcfree releases through the CRT
 * free().  If Z_MallocInternal is ever anything but a malloc wrapper, this
 * pair does not match.  It is what the binary does, so it stays; noted because
 * it is exactly the kind of asymmetry that reads as a typo later.
 * Reached only as the z_stream.zfree function pointer, (opaque, block).
 */
void __cdecl zcfree(int opaque, void *ptr)
{
  free(ptr);
}

