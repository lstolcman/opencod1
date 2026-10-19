/*
 * Retail range 0x0043D7B0-0x00442290, 80 functions.
 *
 * @fidelity: likely
 * @fidelity-default: unreviewed
 *
 * Hand-written parts: the SPLIT-RECORD sites (see the note below);
 * PC_ExpandBuiltinDefine 0x0043E4B0; and the two calls in PC_ReadToken to
 * PC_ReadDirective 0x00440FE0 and PC_ReadDollarDirective 0x00441350, which
 * live in botlib/l_precomp_pc.c along with the two .data tables they walk.
 * PC_EvaluateTokens 0x0043FC10 is RTCW's text (a standalone VC7 build of it
 * reproduces retail's frame and code).
 *
 * PC_ExpectTokenType (0x00441740) is unreachable in retail (no xrefs):
 * CoD MP ships no BotLibSetup and no bots.  Its body is reconstructed anyway
 * for retail fidelity.
 */

#include "../qcommon/qcommon.h"
#include "l_script.h"
#include "l_memory.h"
/* ---- module-owned globals ---- */
int numtokens;
void *globaldefines;
char pc_baseFolder[64];   /* 0x01646560, PC_SetBaseFolder's base folder */

/* ---- file-local records (RTCW l_precomp.h; not carried in l_script.h) ----
 *
 * token_t, source_t, script_t and punctuation_t all live in l_script.h with
 * their offsets pinned there.  Only define_t and indent_t are private to this
 * unit, so they are defined here.  source_t leaves defines/definehash/
 * indentstack as void* (they predate these types), so those fields are cast at
 * their use sites below. */

/* define_t -- one preprocessor macro.  Offsets fixed by the retail field
 * accesses: name +0, flags +4 (DEFINE_FIXED = 1), builtin +8, numparms +12,
 * parms +16, tokens +20, next +24, hashnext +28.  Z_Malloc(sizeof(define_t) +
 * strlen(name) + 1) reads in the decompile as malloc(strlen + 37) = 32 payload
 * + a 4-byte heap magic + the trailing NUL, and memset clears 0x20 = 32 bytes.
 * RTCW layout, no CoD divergence. */
typedef struct define_s {
	char *name;                     /* +0  */
	int flags;                      /* +4  */
	int builtin;                    /* +8  */
	int numparms;                   /* +12 */
	token_t *parms;                 /* +16 */
	token_t *tokens;                /* +20 */
	struct define_s *next;          /* +24 */
	struct define_s *hashnext;      /* +28 */
} define_t;                         /* 32 bytes */

COD1_ASSERT_SIZE( define_t, 32 );
COD1_STATIC_ASSERT( offsetof(define_t, tokens) == 20, "define_t.tokens must be +20" );
COD1_STATIC_ASSERT( offsetof(define_t, hashnext) == 28, "define_t.hashnext must be +28" );

/* define->flags */
#define DEFINE_FIXED    1

/* define->builtin values -- PC_ExpandBuiltinDefine's switch. */
#define BUILTIN_LINE    1
#define BUILTIN_FILE    2
#define BUILTIN_DATE    3
#define BUILTIN_TIME    4

/* indent_t -- one #if/#ifdef/#else nesting level on source->indentstack.
 * Z_Malloc(0x14) = a 4-byte heap magic + this 16-byte record: type +0,
 * skip +4, script +8, next +12.  RTCW layout. */
typedef struct indent_s {
	int type;                       /* +0  */
	int skip;                       /* +4  */
	script_t *script;               /* +8  */
	struct indent_s *next;          /* +12 */
} indent_t;                         /* 16 bytes */

COD1_ASSERT_SIZE( indent_t, 16 );

/* indent types -- PC_PushIndent's `type` argument.  RTCW INDENT_*. */
#define INDENT_IF       1
#define INDENT_ELSE     2
#define INDENT_ELIF     4
#define INDENT_IFDEF    8
#define INDENT_IFNDEF   16

/* Two DEFINEHASHSIZE-driven bounds this unit walks the hash table with. */
#define DEFINEHASHSIZE  1024

/* The 0x12345678 heap magic Z_MallocInternal stamps ahead of every block and
 * free() checks before releasing it. */
#define ZONE_MAGIC      0x12345678

/* Allocation goes through botlib's GetMemory / GetClearedMemory / FreeMemory
 * (l_memory_mp.c 0x0043D6B0 / 0x0043D6D0 / 0x0043D770), which LTCG inlines at
 * every site here, as in retail.  The former tree-only static copies
 * (PC_MP_GetMemory & co.) were emitted as three dead out-of-line functions
 * retail does not have; their LTCG symbols shifted VC7's scheduling ties in
 * yylex and Netchan_Process (tools/vc7/tie_probe.py). */

/*
 * ===========================================================================
 * THE STACK TOKEN RECORD
 * ===========================================================================
 *
 * Retail builds a `token_t token;` on the stack in eleven functions here.  It
 * is 1072 bytes and always sits at [ebp-434h], running down to the security
 * cookie at [ebp-4].  Callees read and write the full record: PC_ReadLine and
 * PC_ReadSourceToken fill 1072 bytes; PC_CopyToken memcpys 0x430 bytes out.
 *
 * The Hex-Rays decompile split each such record into a char* name (pointed at
 * token.string) plus loose scalars rooted on the fields (+1024 type, +1028
 * subtype, +1048 whitespace_p and so on).  This tidy folds every site back
 * into a plain `token_t token;` and passes &token where the whole record is
 * wanted -- `token.string` and `&token` are the same address.
 *
 * Layout is in l_script.h.
 */

/* Functions this unit calls that live elsewhere.  Declared unprototyped and
 * int-returning: C89 makes that compatible with any definition.  Anything a
 * real header already declares is excluded -- StripDoubleQuotes
 * (string@<edx>), LoadScriptFile (one stack argument),
 * LoadScriptMemory (retail lifts length into EAX; source order is
 * ( ptr, length, name )) and PS_ReadToken (retail ( token@<eax>,
 * script@<ecx> ); source order is ( script, token )) all come from
 * l_script.h.  An unprototyped `extern int f();` binds a __fastcall body to
 * the wrong decorated name and switches argument checking off. */
/* Log_Write: 0x0043D650 is RTCW's Log_Write, not Log_WriteTimeStamped (which
 * emits a botlibglobals.time stamp and is absent from this binary);
 * PC_PrintDefineHashTable is RTCW l_precomp.c:522 verbatim and uses Log_Write
 * for all three prints. */
extern void QDECL Log_Write( char *fmt, ... );

/* PC_CheckTokenString (0x00441A20) is defined later in this file but called
 * earlier, from PC_ReadDefineParms.  The spelling matches the definition. */
int __cdecl PC_CheckTokenString( source_t *source, const char *string );

/* PC_ExpectTokenType (0x00441740) has no callers in retail or the tree.
 * Retail passes subtype in EBX and type in ESI; this unit uses RTCW source
 * order, so any future caller must pass ( source, type, subtype, token ). */
int __cdecl PC_ExpectTokenType( source_t *source, int type, int subtype, token_t *token );

/* ---- SourceError  0x0043D7B0 ---- */
// SourceError | CONFIRMED | RTCW l_precomp.c SourceError.  DIVERGENCE: CoD prefixes the message with the "^1" red colour code; RTCW emits it plain via botimport.Print(PRT_ERROR,...)
void SourceError(source_t *source, char *str, ...)
{
  char text[1024];
  va_list ap;

  va_start(ap, str);
  vsprintf(text, str, ap);
  Com_Printf(
    "^1Error: file %s, line %d: %s\n",
    source->scriptstack->filename,
    source->scriptstack->line,
    text);
}

/* ---- SourceWarning  0x0043D820 ---- */
// SourceWarning | HIGH | RTCW SourceWarning (same format, no colour prefix).  Distinguished from SourceError purely by the absence of the ^1 colour prefix.
void SourceWarning(source_t *source, char *str, ...)
{
  char text[1024];
  va_list ap;

  va_start(ap, str);
  vsprintf(text, str, ap);
  Com_Printf("file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

/* ---- PC_PushIndent  0x0043D890 ---- */
// PC_PushIndent | CONFIRMED | RTCW l_precomp.c:189 -- mallocs 0x14 node w/ 0x12345678 heap magic, links onto source[135] indent stack, tracks skip in [136]
indent_t *__cdecl PC_PushIndent(source_t *source, int type, int skip)
{
  int *magic;
  indent_t *indent;

  magic = Z_MallocInternal(4 + sizeof(indent_t));   /* 0x14 = heap magic + indent_t */
  if ( magic )
    *magic++ = ZONE_MAGIC;
  indent = (indent_t *)magic;
  indent->type = type;
  indent->script = source->scriptstack;
  indent->skip = (skip != 0);
  source->skip += indent->skip;
  indent->next = (indent_t *)source->indentstack;
  source->indentstack = indent;
  return indent;
}

/* ---- PC_PopIndent  0x0043D8F0 ---- */
// PC_PopIndent | CONFIRMED | RTCW l_precomp.c:206 -- pops source[135], writes back type/skip, frees node if heap magic present
void __cdecl PC_PopIndent(source_t *source, int *type, int *skip)
{
  indent_t *indent;

  *type = 0;
  *skip = 0;
  indent = (indent_t *)source->indentstack;
  if ( !indent )
    return;
  /* must be an indent from the current script */
  if ( indent->script != source->scriptstack )
    return;
  *type = indent->type;
  *skip = indent->skip;
  source->indentstack = ((indent_t *)source->indentstack)->next;
  source->skip -= indent->skip;
  if ( ((int *)indent)[-1] == ZONE_MAGIC )
    free((int *)indent - 1);
}

/* ---- PC_PushScript  0x0043D960 ---- */
// PC_PushScript | CONFIRMED | RTCW l_precomp.c
/* Q_stricmpn at 0x0043D977 is __usercall and its three arguments all arrive
   in registers:

       mov eax, 1869Fh      ; n  = 99999
       mov ecx, edi         ; s2 = the script being pushed
       mov edx, esi         ; s1 = the script already on the stack
       call Q_stricmpn

   Both scripts are script_t*, whose filename[] is at +0, so passing the struct
   pointers is passing the filenames.  99999 is retail's "compare it all"
   length; it is not a buffer bound and is reproduced as-is. */
void __cdecl PC_PushScript(script_t *script, source_t *source)
{
  script_t *s;

  for ( s = source->scriptstack; s; s = s->next )
  {
    if ( script && !Q_stricmpn(s->filename, script->filename, 99999) )
    {
      SourceError(source, "%s recursively included", script->filename);
      return;
    }
  }
  /* push the script on the script stack */
  script->next = source->scriptstack;
  source->scriptstack = script;
}

/* ---- PC_InitTokenHeap  0x0043D9C0 ----   VERBATIM
 *
 * RTCW's PC_InitTokenHeap (l_precomp.c:254), whose entire body is commented
 * out in the reference -- the USE_TOKEN_HEAP free-list initialiser -- so the
 * compiler emitted a bare `retn`.  It sits where RTCW puts it, between
 * PC_PushScript (0x0043D960) and PC_CopyToken (0x0043D9D0), and the Mac 1.3
 * xSYM listing names `PC_InitTokenHeap` in botlib/l_precomp_mp.c
 * (docs/mac13_symbols.tsv:3309).  Zero xrefs in the Windows binary -- retail
 * never calls it, which is also true of RTCW once the heap is disabled.
 *
 * VERIFIED 0x0043D9C0.  The whole function is one byte, `C3`. */
void PC_InitTokenHeap( void )
{
}

/* ---- PC_CopyToken  0x0043D9D0 ---- */
// PC_CopyToken | CONFIRMED | RTCW l_precomp.c PC_CopyToken -- token alloc 0x434 (magic 0x12345678 + 0x430 payload), memcpy 0x430, bumps numtokens.
token_t *__cdecl PC_CopyToken(const token_t *token)
{
  token_t *t;

  t = (token_t *)GetMemory(sizeof(token_t));   /* 0x434 = heap magic + token_t */
  if ( !t )
  {
    Com_Error(ERR_FATAL, "EXE_ERR_OUT_OF_MEMORY");
    return NULL;
  }
  memcpy(t, token, sizeof(token_t));   /* 0x430 */
  t->next = NULL;
  numtokens++;
  return t;
}

/* ---- PC_FreeToken  0x0043DA30 ---- */
// PC_FreeToken | CONFIRMED | FreeMemory(token) then --numtokens.
void __cdecl PC_FreeToken(token_t *token)
{
  FreeMemory( token );
  numtokens--;
}

/* ---- PC_ReadSourceToken  0x0043DA50 ---- */
// PC_ReadSourceToken | CONFIRMED | RTCW l_precomp.c
int __cdecl PC_ReadSourceToken(source_t *source,token_t *token)
{
  token_t *t;
  script_t *script;
  int type, skip;

  /* if there's no token already available */
  while ( !source->tokens )
  {
    /* PS_ReadToken( script, token ) -- source order; retail's register order
       is ( token@<eax>, script@<ecx> ). */
    if ( PS_ReadToken( source->scriptstack, token ) )
      return 1;
    /* if at the end of the script, remove all indents of the script */
    if ( source->scriptstack->script_p >= source->scriptstack->end_p )
    {
      while ( source->indentstack &&
              ((indent_t *)source->indentstack)->script == source->scriptstack )
      {
        SourceWarning( source, "missing #endif" );
        PC_PopIndent( source, &type, &skip );
      }
    }
    /* if this was the initial script, there are no more tokens */
    if ( !source->scriptstack->next )
      return 0;
    /* remove the script and return to the previous one (inlined FreeScript) */
    script = source->scriptstack;
    source->scriptstack = source->scriptstack->next;
    if ( script->punctuationtable && ((int *)script->punctuationtable)[-1] == ZONE_MAGIC )
      free((int *)script->punctuationtable - 1);
    if ( ((int *)script)[-1] == ZONE_MAGIC )
      free((int *)script - 1);
  }
  /* copy the already available token */
  memcpy( token, source->tokens, sizeof(token_t) );
  /* free the read token */
  t = source->tokens;
  source->tokens = source->tokens->next;
  PC_FreeToken( t );
  return 1;
}

/* ---- PC_UnreadSourceToken  0x0043DBC0 ---- */
// PC_UnreadSourceToken | CONFIRMED | RTCW l_precomp.c PC_UnreadSourceToken.  Wraps PC_CopyToken then prepends to source->tokens at +528.  Retail: source in ESI, token in EAX.
int __cdecl PC_UnreadSourceToken(source_t *source,const token_t *token)
{
  token_t *t;

  t = PC_CopyToken(token);
  t->next = source->tokens;
  source->tokens = t;
  return 1;
}

/* ---- PC_ReadDefineParms  0x0043DBF0 ---- */
// PC_ReadDefineParms | CONFIRMED | RTCW l_precomp.c:377
int __cdecl PC_ReadDefineParms(source_t *source, define_t *define, token_t **parms, int maxparms)
{
  token_t token;
  token_t *t, *last;
  int i, done, lastcomma, numparms, indent;

  if ( !PC_ReadSourceToken(source,&token) )
  {
    SourceError( source, "define %s missing parms", define->name );
    return 0;
  }
  if ( define->numparms > maxparms )
  {
    SourceError( source, "define with more than %d parameters", maxparms );
    return 0;
  }
  for ( i = 0; i < define->numparms; i++ )
    parms[i] = NULL;
  /* if no leading "(" */
  if ( strcmp( token.string, "(" ) )
  {
    PC_UnreadSourceToken(source,&token);
    SourceError( source, "define %s missing parms", define->name );
    return 0;
  }
  /* read the define parameters */
  for ( done = 0, numparms = 0, indent = 0; !done; )
  {
    if ( numparms >= maxparms )
    {
      SourceError( source, "define %s with too many parms", define->name );
      return 0;
    }
    if ( numparms >= define->numparms )
    {
      SourceWarning( source, "define %s has too many parms", define->name );
      return 0;
    }
    parms[numparms] = NULL;
    lastcomma = 1;
    last = NULL;
    while ( !done )
    {
      if ( !PC_ReadSourceToken(source,&token) )
      {
        SourceError( source, "define %s incomplete", define->name );
        return 0;
      }
      if ( !strcmp( token.string, "," ) )
      {
        if ( indent <= 0 )
        {
          if ( lastcomma )
            SourceWarning( source, "too many comma's" );
          lastcomma = 1;
          break;
        }
      }
      lastcomma = 0;
      if ( !strcmp( token.string, "(" ) )
      {
        indent++;
        continue;
      }
      else if ( !strcmp( token.string, ")" ) )
      {
        if ( --indent <= 0 )
        {
          if ( !parms[define->numparms - 1] )
            SourceWarning( source, "too few define parms" );
          done = 1;
          break;
        }
      }
      if ( numparms < define->numparms )
      {
        t = PC_CopyToken( &token );
        t->next = NULL;
        if ( last )
          last->next = t;
        else
          parms[numparms] = t;
        last = t;
      }
    }
    numparms++;
  }
  return 1;
}

/* ---- PC_StringizeTokens  0x0043DF70 ---- */
// PC_StringizeTokens | CONFIRMED | Builds a quoted string by walking the token list via +1064 (token->next) and strncat-ing each token->string, capped at 1024 (MAX_TOKEN).
int __cdecl PC_StringizeTokens(token_t *tokens, token_t *token)
{
  token_t *t;

  token->type = TT_STRING;
  token->whitespace_p = NULL;
  token->endwhitespace_p = NULL;
  token->string[0] = '\0';
  strcat( token->string, "\"" );
  for ( t = tokens; t; t = t->next )
    strncat( token->string, t->string, MAX_TOKEN - strlen(token->string) );
  /* Closing quote: the two-byte literal at 0x005682F8, a double quote and a NUL. */
  strncat( token->string, "\"", MAX_TOKEN - strlen(token->string) );
  return 1;
}

/* ---- PC_MergeTokens  0x0043E020 ---- */
// PC_MergeTokens | CONFIRMED | Concatenates two tokens when the type pair permits (4 = TT_NAME / 3 = TT_NUMBER in this build).
/* RTCW's ( t1, t2 ) order: t2 is appended onto t1, so t1 is the token that
   grows.  LTCG passes t2 in EAX and t1 in ECX; the declared order also fixes
   the order PC_ExpandDefine sets them up in. */
int __cdecl PC_MergeTokens(token_t *t1, token_t *t2)
{
  /* merging of a name with a name or number */
  if ( t1->type == TT_NAME && ( t2->type == TT_NAME || t2->type == TT_NUMBER ) )
  {
    strcat( t1->string, t2->string );
    return 1;
  }
  /* merging of two strings */
  if ( t1->type == TT_STRING && t2->type == TT_STRING )
  {
    /* remove trailing double quote, then concat without leading double quote */
    t1->string[strlen(t1->string) - 1] = '\0';
    strcat( t1->string, &t2->string[1] );
    return 1;
  }
  return 0;
}

/* ---- PC_PrintDefineHashTable  0x0043E0C0 ---- */
// PC_PrintDefineHashTable | CONFIRMED | Iterates 1024 buckets (DEFINEHASHSIZE) printing "%4d:" then each define name; hash chain link is define->hashnext at +28.
void __cdecl PC_PrintDefineHashTable(define_t **definehash)
{
  int i;
  define_t *d;

  for ( i = 0; i < DEFINEHASHSIZE; i++ )
  {
    Log_Write("%4d:", i);
    for ( d = definehash[i]; d; d = d->hashnext )
      Log_Write(" %s", d->name);
    Log_Write("\n");
  }
}

/* ---- PC_NameHash  0x0043E110 ---- */
// PC_NameHash | HIGH | RTCW l_precomp.c define-name hash -- (h ^ ((h^(h>>10))>>10)) & 0x3FF, base 119 multiplier
int __cdecl PC_NameHash(char *name)
{
  int hash, i;

  hash = 0;
  for ( i = 0; name[i] != '\0'; i++ )
    hash += name[i] * ( 119 + i );
  hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( DEFINEHASHSIZE - 1 );
  return hash;
}

/* ---- PC_AddDefineToHash  0x0043E150 ---- */
// PC_AddDefineToHash | CONFIRMED | define->hashnext (+28) = bucket; bucket = define.
int __cdecl PC_AddDefineToHash(define_t **definehash, define_t *define)
{
  int hash;

  hash = PC_NameHash(define->name);
  define->hashnext = definehash[hash];
  definehash[hash] = define;
  return hash;
}

/* ---- PC_FindHashedDefine  0x0043E170 ---- */
// PC_FindHashedDefine | CONFIRMED | RTCW l_precomp.c:576 -- hashes name, walks bucket via define->hashnext(+28), strcmp on define->name
define_t *__cdecl PC_FindHashedDefine(define_t **definehash, char *name)
{
  define_t *d;

  for ( d = definehash[PC_NameHash(name)]; d; d = d->hashnext )
  {
    if ( !strcmp(d->name, name) )
      return d;
  }
  return NULL;
}

/* ---- PC_FindDefine  0x0043E1E0 ---- */
// PC_FindDefine | CONFIRMED | Linear strcmp walk over the non-hashed define list, link at +24 (define->next).
define_t *__cdecl PC_FindDefine(define_t *defines, const char *name)
{
  define_t *d;

  for ( d = defines; d; d = d->next )
  {
    if ( !strcmp(d->name, name) )
      return d;
  }
  return NULL;
}

/* ---- PC_FindDefineParm  0x0043E240 ---- */
// PC_FindDefineParm | CONFIRMED | RTCW l_precomp.c:614 -- walks define->parms(+16) via +1064 stride, returns ordinal or -1
int __cdecl PC_FindDefineParm(define_t *define, const char *name)
{
  token_t *p;
  int i;

  i = 0;
  for ( p = define->parms; p; p = p->next )
  {
    if ( !strcmp(p->string, name) )
      return i;
    i++;
  }
  return -1;
}

/* ---- PC_FreeDefine  0x0043E2A0 ---- */
// PC_FreeDefine | CONFIRMED | RTCW l_precomp.c:634 -- frees parm token chain (+1064 stride) then define, decrementing numtokens
void __cdecl PC_FreeDefine(define_t *define)
{
  token_t *t, *next;

  /* free the define parameters (inlined PC_FreeToken) */
  for ( t = define->parms; t; t = next )
  {
    next = t->next;
    if ( ((int *)t)[-1] == ZONE_MAGIC )
      free((int *)t - 1);
    numtokens--;
  }
  /* free the define tokens */
  for ( t = define->tokens; t; t = next )
  {
    next = t->next;
    if ( ((int *)t)[-1] == ZONE_MAGIC )
      free((int *)t - 1);
    numtokens--;
  }
  /* free the define itself */
  if ( ((int *)define)[-1] == ZONE_MAGIC )
    free((int *)define - 1);
}

/* ---- PC_AddBuiltinDefines  0x0043E340 ---- */
// PC_AddBuiltinDefines | CONFIRMED | RTCW l_precomp.c:668
void __cdecl PC_AddBuiltinDefines(source_t *source)
{
  int i;
  define_t *define;
  struct builtin
  {
    char *string;
    int builtin;
  } builtin[] = {
    { "__LINE__", BUILTIN_LINE },
    { "__FILE__", BUILTIN_FILE },
    { "__DATE__", BUILTIN_DATE },
    { "__TIME__", BUILTIN_TIME },
    { NULL, 0 }
  };

  for ( i = 0; builtin[i].string; i++ )
  {
    define = (define_t *)GetMemory( sizeof(define_t) + strlen(builtin[i].string) + 1 );
    memset( define, 0, sizeof(define_t) );
    define->name = (char *)define + sizeof(define_t);
    strcpy( define->name, builtin[i].string );
    define->flags |= DEFINE_FIXED;
    define->builtin = builtin[i].builtin;
    /* add the define to the source */
    PC_AddDefineToHash( (define_t **)source->definehash, define );
  }
}

/* ---- PC_ExpandBuiltinDefine  0x0043E4B0 ---- */
// PC_ExpandBuiltinDefine | CONFIRMED | RTCW l_precomp.c PC_ExpandBuiltinDefine -- switch on define->builtin (+8) producing __LINE__/__FILE__/__DATE__/__TIME__ token text; the __DATE__ and __TIME__ arms call ctime
/*
 * Q3 source order ( source, deftoken, define, firsttoken, lasttoken ):
 * `PC_CopyToken(deftoken)` (RTCW copies deftoken here) and
 * `strcpy(token->string, source->scriptstack->filename)` -- +524 is
 * source->scriptstack and script_t::filename is at offset 0.
 *
 * REACHABILITY.  Dead in retail: the switch only runs when define->builtin is
 * non-zero, and the only thing that sets it is PC_AddBuiltinDefines
 * (0x0043E340), which has NO XREFS in the binary.  CoD never registers
 * __LINE__, __FILE__, __DATE__ or __TIME__, so every define reaching
 * PC_ExpandDefine has builtin == 0 and this function is never entered.  The
 * default arm returns 1 with both out-parameters NULL, which is success.
 *
 * RETAIL DEFECT PRESERVED, and it is a live one if anybody ever calls
 * PC_AddBuiltinDefines: the __DATE__ and __TIME__ arms `free()` the pointer
 * returned by ctime, which is a static CRT buffer.  Q3 and RTCW have the
 * identical line.  Retail's own ctime wrapper is 0x00527746 (localtime then
 * asctime), so the pointer really is the CRT's, and freeing it is heap
 * corruption.  Do not "fix" it without recording the divergence, and do not
 * make PC_AddBuiltinDefines live without dealing with it first.
 *
 * The __LINE__ arm sets subtype 4104 = TT_DECIMAL|TT_INTEGER, exactly as
 * RTCW-MP src/botlib/l_precomp.c:714 (0x1008); no divergence.
 *
 * This is the 5-argument, PC_CopyToken(deftoken) form that RTCW-MP's botlib
 * carries, NOT the older 4-argument form in RTCW-MP's src/extractfuncs/ copy,
 * which builds the token on the stack from source->token.  CoD1 forked the
 * same late-Q3 snapshot RTCW did.
 */
/* <time.h> is not in this unit's include set.  These two declarations are
 * MSVC 6's exactly: `time_t` is `long` there, which is also what retail's
 * `__time32_t` is. */
extern long   time( long *timer );
extern char * ctime( const long *timer );

int __cdecl PC_ExpandBuiltinDefine(source_t *source, token_t *deftoken, define_t *define,
                                   token_t **firsttoken, token_t **lasttoken)
{
  token_t *token;
  char    *curtime;
  long     t;

  token = PC_CopyToken(deftoken);
  switch ( define->builtin )
  {
    case BUILTIN_LINE:
      sprintf(token->string, "%d", deftoken->line);
      token->intvalue   = deftoken->line;
      token->floatvalue = (double)deftoken->line;
      token->type       = TT_NUMBER;
      token->subtype    = TT_DECIMAL | TT_INTEGER;   /* 4104, as RTCW */
      *firsttoken = token;
      *lasttoken  = token;
      return 1;

    case BUILTIN_FILE:
      strcpy(token->string, source->scriptstack->filename);
      token->type    = TT_NAME;
      token->subtype = strlen(token->string);
      *firsttoken = token;
      *lasttoken  = token;
      return 1;

    case BUILTIN_DATE:
      t = time(0);
      curtime = ctime(&t);
      strcpy(token->string, "\"");
      strncat(token->string, curtime + 4, 7);
      strncat(token->string + 7, curtime + 20, 4);
      strcat(token->string, "\"");
      free(curtime);                     /* retail defect -- see above */
      token->type    = TT_NAME;
      token->subtype = strlen(token->string);
      *firsttoken = token;
      *lasttoken  = token;
      return 1;

    case BUILTIN_TIME:
      t = time(0);
      curtime = ctime(&t);
      strcpy(token->string, "\"");
      strncat(token->string, curtime + 11, 8);
      strcat(token->string, "\"");
      free(curtime);                     /* retail defect -- see above */
      token->type    = TT_NAME;
      token->subtype = strlen(token->string);
      *firsttoken = token;
      *lasttoken  = token;
      return 1;

    default:                             /* BUILTIN_STDC and anything else */
      *firsttoken = 0;
      *lasttoken  = 0;
      return 1;
  }
}

/* ---- PC_ExpandDefine  0x0043E6A0 ---- */
// PC_ExpandDefine | CONFIRMED | RTCW l_precomp.c:844
int __cdecl PC_ExpandDefine(source_t *source, token_t *deftoken, define_t *define,
                            token_t **firsttoken, token_t **lasttoken)
{
  token_t *parms[128], *dt, *pt, *t;   /* MAX_DEFINEPARMS */
  token_t *t1, *t2, *first, *last, *nextpt, token;
  int parmnum, i;

  /* if it is a builtin define */
  if ( define->builtin )
    return PC_ExpandBuiltinDefine( source, deftoken, define, firsttoken, lasttoken );
  /* if the define has parameters */
  if ( define->numparms )
  {
    if ( !PC_ReadDefineParms( source, define, parms, 128 ) )
      return 0;
  }
  /* empty list at first */
  first = NULL;
  last = NULL;
  /* create a list with tokens of the expanded define */
  for ( dt = define->tokens; dt; dt = dt->next )
  {
    parmnum = -1;
    /* if the token is a name, it could be a define parameter */
    if ( dt->type == TT_NAME )
      parmnum = PC_FindDefineParm( define, dt->string );
    /* if it is a define parameter */
    if ( parmnum >= 0 )
    {
      for ( pt = parms[parmnum]; pt; pt = pt->next )
      {
        t = PC_CopyToken( pt );
        /* add the token to the list */
        t->next = NULL;
        if ( last )
          last->next = t;
        else
          first = t;
        last = t;
      }
    }
    else
    {
      /* if stringizing operator */
      if ( dt->string[0] == '#' && dt->string[1] == '\0' )
      {
        /* the stringizing operator must be followed by a define parameter */
        if ( dt->next )
          parmnum = PC_FindDefineParm( define, dt->next->string );
        else
          parmnum = -1;
        if ( parmnum >= 0 )
        {
          /* step over the stringizing operator */
          dt = dt->next;
          /* stringize the define parameter tokens */
          if ( !PC_StringizeTokens( parms[parmnum], &token ) )
          {
            SourceError( source, "can't stringize tokens" );
            return 0;
          }
          t = PC_CopyToken( &token );
        }
        else
        {
          SourceWarning( source, "stringizing operator without define parameter" );
          continue;
        }
      }
      else
      {
        t = PC_CopyToken( dt );
      }
      /* add the token to the list */
      t->next = NULL;
      if ( last )
        last->next = t;
      else
        first = t;
      last = t;
    }
  }
  /* check for the merging operator */
  for ( t = first; t; )
  {
    if ( t->next )
    {
      /* if the merging operator */
      if ( t->next->string[0] == '#' && t->next->string[1] == '#' )
      {
        t1 = t;
        t2 = t->next->next;
        if ( t2 )
        {
          if ( !PC_MergeTokens( t1, t2 ) )
          {
            SourceError( source, "can't merge %s with %s", t1->string, t2->string );
            return 0;
          }
          PC_FreeToken( t1->next );
          t1->next = t2->next;
          if ( t2 == last )
            last = t1;
          PC_FreeToken( t2 );
          continue;
        }
      }
    }
    t = t->next;
  }
  /* store the first and last token of the list */
  *firsttoken = first;
  *lasttoken = last;
  /* free all the parameter tokens */
  for ( i = 0; i < define->numparms; i++ )
  {
    for ( pt = parms[i]; pt; pt = nextpt )
    {
      nextpt = pt->next;
      PC_FreeToken( pt );
    }
  }
  return 1;
}

/* ---- PC_ExpandDefineIntoSource  0x0043EA90 ---- */
// PC_ExpandDefineIntoSource | HIGH | RTCW l_precomp.c:906 -- calls PC_ExpandDefine then splices firsttoken/lasttoken onto source[528] token stack
/* RTCW parameter order: retail callers set define (EDX) before source (ESI). */
int __cdecl PC_ExpandDefineIntoSource(source_t *source, token_t *deftoken, define_t *define)
{
  token_t *firsttoken, *lasttoken;

  if ( !PC_ExpandDefine( source, deftoken, define, &firsttoken, &lasttoken ) )
    return 0;
  if ( !firsttoken || !lasttoken )
    return 0;
  lasttoken->next = source->tokens;
  source->tokens = firsttoken;
  return 1;
}

/* ---- PC_ConvertPath  0x0043EAF0 ---- */
// PC_ConvertPath | CONFIRMED | Collapses doubled '\\' and '//' separators in place.  l_precomp.c verbatim.
char *__cdecl PC_ConvertPath(char *path)
{
  char *ptr;

  /* remove double path separators */
  for ( ptr = path; *ptr; )
  {
    if ( ( *ptr == '\\' || *ptr == '/' ) && ( ptr[1] == '\\' || ptr[1] == '/' ) )
      strcpy( ptr, ptr + 1 );
    else
      ptr++;
  }
  /* set OS-dependent path separators */
  for ( ptr = path; *ptr; ptr++ )
  {
    if ( *ptr == '/' || *ptr == '\\' )
      *ptr = '\\';
  }
  return ptr;
}

/* ---- PC_Directive_include  0x0043EB50 ----
 *
 * RETAIL STACK OVERFLOW, REPRODUCED DELIBERATELY.  Read before "fixing" it,
 * and before deciding this parser is safe on untrusted input.
 *
 * The `#include <a><b><c>` form accumulates the bracketed tokens into a
 * 260-byte stack buffer.  Retail's loop is
 *
 *     0043ECCE  push 104h        ; Count = 260
 *     0043ECD3  lea  edx, [Source]
 *     0043ECDF  lea  eax, [Destination]   ; char[260], [ebp-538h]
 *     0043ECE0  call _strncat
 *
 * and strncat's count bounds the SOURCE, not the room left in the
 * destination.  Each iteration appends up to 260 more bytes onto a buffer
 * that is 260 bytes total, and the loop runs once per token until a '>' or a
 * line break.  Destination sits below the token record and the cookie, so a
 * long enough `#include <...>` walks straight up the frame.  Q3 and RTCW have
 * the identical line; CoD inherited it unchanged.
 *
 * Left as retail because the binary is the spec here and because nothing on a
 * dedicated server can reach it -- the only callers of the whole precompiler
 * are the two client VM syscall dispatchers.  It is reachable in a CLIENT that
 * loads a .menu file from a downloaded IWD.  If this unit is ever linked
 * somewhere that untrusted input can reach it, this is the first thing to fix:
 * the bound wants to be the remaining space in Destination.
 */
// PC_Directive_include | CONFIRMED | RTCW l_precomp.c:969
int __cdecl PC_Directive_include(source_t *source)
{
  script_t *script;
  token_t token;
  char path[260];   /* _MAX_PATH */

  if ( source->skip > 0 )
    return 1;
  if ( !PC_ReadSourceToken(source,&token) )
  {
    SourceError( source, "#include without file name" );
    return 0;
  }
  if ( token.linescrossed > 0 )
  {
    SourceError( source, "#include without file name" );
    return 0;
  }
  if ( token.type == TT_STRING )
  {
    StripDoubleQuotes( token.string );
    PC_ConvertPath( token.string );
    script = LoadScriptFile( token.string );
    if ( !script )
    {
      strcpy( path, source->includepath );
      strcat( path, token.string );
      script = LoadScriptFile( path );
    }
  }
  else if ( token.type == TT_PUNCTUATION && token.string[0] == '<' )
  {
    strcpy( path, source->includepath );
    while ( PC_ReadSourceToken(source,&token) )
    {
      if ( token.linescrossed > 0 )
      {
        PC_UnreadSourceToken(source,&token);
        break;
      }
      if ( token.type == TT_PUNCTUATION && token.string[0] == '>' )
        break;
      /* RETAIL OVERFLOW -- see the banner above this function.  path is
         char[260]; the bound is the SOURCE length, not the remaining room,
         so this loop appends without limit. */
      strncat( path, token.string, 260 );
    }
    if ( token.string[0] != '>' )
      SourceWarning( source, "#include missing trailing >" );
    if ( !strlen( path ) )
    {
      SourceError( source, "#include without file name between < >" );
      return 0;
    }
    PC_ConvertPath( path );
    script = LoadScriptFile( path );
  }
  else
  {
    SourceError( source, "#include without file name" );
    return 0;
  }
  if ( !script )
  {
    SourceError( source, "file %s not found", path );
    return 0;
  }
  PC_PushScript( script, source );
  return 1;
}

/* ---- PC_ReadLine  0x0043EDB0 ---- */
// PC_ReadLine | HIGH | RTCW l_precomp.c:1043 -- reads tokens until line changes; backslash continues; unreads via PC_CopyToken onto source[528]
int __cdecl PC_ReadLine(source_t *source, token_t *token)
{
  int crossline;

  crossline = 0;
  do
  {
    if ( !PC_ReadSourceToken(source,token) )
      return 0;
    if ( token->linescrossed > crossline )
    {
      PC_UnreadSourceToken(source,token);
      return 0;
    }
    crossline = 1;
  }
  while ( !strcmp( token->string, "\\" ) );
  return 1;
}

/* ---- PC_WhiteSpaceBeforeToken  0x0043EE20 ---- */
// PC_WhiteSpaceBeforeToken | CONFIRMED | token->endwhitespace_p (+1052) - token->whitespace_p (+1048) > 0.
int __cdecl PC_WhiteSpaceBeforeToken(token_t *token)
{
  return token->endwhitespace_p - token->whitespace_p > 0;
}

/* ---- PC_ClearTokenWhiteSpace  0x0043EE40 ---- */
// PC_ClearTokenWhiteSpace | CONFIRMED | Zeroes whitespace_p (+1048), endwhitespace_p (+1052) and linescrossed (+1060).  token_t: string[1024], type 1024, subtype 1028, intvalue 1032, floatvalue 1040 (8-aligned), whitespace_p 1048, endwhitespace_p 1052, line 1056, linescrossed 1060, next 1064 (see l_script.h).
token_t *__cdecl PC_ClearTokenWhiteSpace(token_t *token)
{
  token->whitespace_p = NULL;
  token->endwhitespace_p = NULL;
  token->linescrossed = 0;
  return token;
}

/* ---- PC_Directive_undef  0x0043EE60 ---- */
// PC_Directive_undef | CONFIRMED | RTCW
int __cdecl PC_Directive_undef(source_t *source)
{
  token_t token;
  define_t *define, *lastdefine;
  int hash;

  if ( source->skip > 0 )
    return 1;
  if ( !PC_ReadLine( source, &token ) )
  {
    SourceError( source, "undef without name" );
    return 0;
  }
  if ( token.type != TT_NAME )
  {
    PC_UnreadSourceToken(source,&token);
    SourceError( source, "expected name, found %s", token.string );
    return 0;
  }
  hash = PC_NameHash( token.string );
  for ( lastdefine = NULL, define = ((define_t **)source->definehash)[hash]; define; define = define->hashnext )
  {
    if ( !strcmp( define->name, token.string ) )
    {
      if ( define->flags & DEFINE_FIXED )
      {
        SourceWarning( source, "can't undef %s", token.string );
      }
      else
      {
        if ( lastdefine )
          lastdefine->hashnext = define->hashnext;
        else
          ((define_t **)source->definehash)[hash] = define->hashnext;
        PC_FreeDefine( define );
      }
      break;
    }
    lastdefine = define;
  }
  return 1;
}

/* ---- PC_Directive_define  0x0043EFE0 ---- */
// PC_Directive_define | CONFIRMED | RTCW l_precomp.c:1161
int __cdecl PC_Directive_define(source_t *source)
{
  token_t token;
  token_t *t, *last;
  define_t *define;

  if ( source->skip > 0 )
    return 1;
  if ( !PC_ReadLine( source, &token ) )
  {
    SourceError( source, "#define without name" );
    return 0;
  }
  if ( token.type != TT_NAME )
  {
    PC_UnreadSourceToken(source,&token);
    SourceError( source, "expected name after #define, found %s", token.string );
    return 0;
  }
  /* check if the define already exists */
  define = PC_FindHashedDefine( (define_t **)source->definehash, token.string );
  if ( define )
  {
    if ( define->flags & DEFINE_FIXED )
    {
      SourceError( source, "can't redefine %s", token.string );
      return 0;
    }
    SourceWarning( source, "redefinition of %s", token.string );
    /* unread the define name before executing the #undef directive */
    PC_UnreadSourceToken(source,&token);
    if ( !PC_Directive_undef( source ) )
      return 0;
  }
  /* allocate define */
  define = (define_t *)GetMemory( sizeof(define_t) + strlen(token.string) + 1 );
  memset( define, 0, sizeof(define_t) );
  define->name = (char *)define + sizeof(define_t);
  strcpy( define->name, token.string );
  /* add the define to the source */
  PC_AddDefineToHash( (define_t **)source->definehash, define );
  /* if nothing is defined, just return */
  if ( !PC_ReadLine( source, &token ) )
    return 1;
  /* if it is a define with parameters -- no whitespace between the name and '(' */
  if ( !PC_WhiteSpaceBeforeToken( &token ) && !strcmp( token.string, "(" ) )
  {
    /* read the define parameters */
    last = NULL;
    if ( !PC_CheckTokenString( source, ")" ) )
    {
      while ( 1 )
      {
        if ( !PC_ReadLine( source, &token ) )
        {
          SourceError( source, "expected define parameter" );
          return 0;
        }
        if ( token.type != TT_NAME )
        {
          SourceError( source, "invalid define parameter" );
          return 0;
        }
        if ( PC_FindDefineParm( define, token.string ) >= 0 )
        {
          SourceError( source, "two the same define parameters" );
          return 0;
        }
        t = PC_CopyToken( &token );
        PC_ClearTokenWhiteSpace( t );
        t->next = NULL;
        if ( last )
          last->next = t;
        else
          define->parms = t;
        last = t;
        define->numparms++;
        if ( !PC_ReadLine( source, &token ) )
        {
          SourceError( source, "define parameters not terminated" );
          return 0;
        }
        if ( !strcmp( token.string, ")" ) )
          break;
        /* then it must be a comma */
        if ( strcmp( token.string, "," ) )
        {
          SourceError( source, "define not terminated" );
          return 0;
        }
      }
    }
    if ( !PC_ReadLine( source, &token ) )
      return 1;
  }
  /* read the defined stuff */
  last = NULL;
  do
  {
    t = PC_CopyToken( &token );
    if ( t->type == TT_NAME && !strcmp( t->string, define->name ) )
    {
      SourceError( source, "recursive define (removed recursion)" );
      continue;
    }
    PC_ClearTokenWhiteSpace( t );
    t->next = NULL;
    if ( last )
      last->next = t;
    else
      define->tokens = t;
    last = t;
  }
  while ( PC_ReadLine( source, &token ) );
  /* check for merge operators at the beginning or end */
  if ( last && ( !strcmp( define->tokens->string, "##" ) || !strcmp( last->string, "##" ) ) )
  {
    SourceError( source, "define with misplaced ##" );
    return 0;
  }
  return 1;
}

/* ---- PC_DefineFromString  0x0043F480 ---- */
// PC_DefineFromString | CONFIRMED | RTCW l_precomp.c PC_DefineFromString
/* RTCW's one-parameter form; LTCG passes the string in EDX (no stack argument,
   plain `ret`), so PC_AddGlobalDefine's EDX flows straight through. */
define_t *__cdecl PC_DefineFromString(const char *string)
{
  script_t *script;
  source_t src;
  token_t *t;
  int *magic;
  define_t *def;
  int res, i;

  /* source order ( ptr, length, name ) -- retail lifts length into EAX */
  script = LoadScriptMemory( (char *)string, strlen(string), "*extern" );
  /* create a new source */
  memset( &src, 0, sizeof(source_t) );
  strncpy( src.filename, "*extern", 260 );   /* _MAX_PATH */
  src.scriptstack = script;
  src.definehash = GetClearedMemory( DEFINEHASHSIZE * sizeof(define_t *) );   /* 0x1000 */
  /* create a define from the source */
  res = PC_Directive_define( &src );
  /* free any tokens if left */
  for ( t = src.tokens; t; t = src.tokens )
  {
    src.tokens = src.tokens->next;
    PC_FreeToken( t );
  }
  /* the define is the first non-empty hash bucket */
  def = NULL;
  for ( i = 0; i < DEFINEHASHSIZE; i++ )
  {
    if ( src.definehash[i] )
    {
      def = (define_t *)src.definehash[i];
      break;
    }
  }
  /* free the hash table (inlined FreeMemory) */
  magic = (int *)src.definehash - 1;
  if ( *magic == ZONE_MAGIC )
    free( magic );
  /* free the script (inlined FreeScript) */
  if ( script->punctuationtable && ((int *)script->punctuationtable)[-1] == ZONE_MAGIC )
    free((int *)script->punctuationtable - 1);
  if ( ((int *)script)[-1] == ZONE_MAGIC )
    free((int *)script - 1);
  /* if the define was created successfully */
  if ( res > 0 )
    return def;
  /* free the define if created */
  if ( src.defines )
    PC_FreeDefine( def );
  return NULL;
}

/* ---- PC_AddDefineToSourceFromString  0x0043F620 ---- */
// PC_AddDefineToSourceFromString | CONFIRMED | PC_DefineFromString then inlined PC_AddDefineToHash against source->definehash at +536.
int __cdecl PC_AddDefineToSourceFromString(source_t *source, const char *string)
{
  define_t *define;

  define = PC_DefineFromString( string );
  if ( !define )
    return 0;
  PC_AddDefineToHash( (define_t **)source->definehash, define );
  return 1;
}

/* ---- PC_AddGlobalDefine  0x0043F660 ---- */
// PC_AddGlobalDefine | CONFIRMED | Pushes onto the globaldefines list (link at +24).
int PC_AddGlobalDefine(const char *string)
{
  define_t *define;

  define = PC_DefineFromString( string );
  if ( !define )
    return 0;
  define->next = (define_t *)globaldefines;
  globaldefines = define;
  return 1;
}

/* ---- PC_RemoveGlobalDefine  0x0043F680 ---- */
// PC_RemoveGlobalDefine | CONFIRMED | PC_FindDefine(globaldefines, name) then PC_FreeDefine.
int __cdecl PC_RemoveGlobalDefine(const char *name)
{
  define_t *define;

  define = PC_FindDefine( (define_t *)globaldefines, name );
  if ( define )
  {
    PC_FreeDefine( define );
    return 1;
  }
  return 0;
}

/* ---- PC_RemoveAllGlobalDefines  0x0043F6B0 ---- */
// PC_RemoveAllGlobalDefines | CONFIRMED | Drains globaldefines through PC_FreeDefine.
void PC_RemoveAllGlobalDefines()
{
  define_t *define;

  for ( define = (define_t *)globaldefines; globaldefines; define = (define_t *)globaldefines )
  {
    globaldefines = define->next;
    PC_FreeDefine( define );
  }
}

/* ---- PC_CopyDefine  0x0043F6E0 ---- */
// PC_CopyDefine | HIGH | RTCW l_precomp.c PC_CopyDefine -- allocates via Z_Malloc(strlen(name)+37), copies flags/builtin/numparms/name, then walks parms and tokens lists into new define.  Divergence: CoD1 copies tokens list first (source[+20]) then parms list (source[+16]); RTCW does parms then tokens.  Field layout unchanged.
define_t *__cdecl PC_CopyDefine(source_t *source, const define_t *define)
{
  define_t *newdefine;
  token_t *token, *newtoken, *lasttoken;

  newdefine = (define_t *)GetMemory( sizeof(define_t) + strlen(define->name) + 1 );
  /* copy the define name */
  newdefine->name = (char *)newdefine + sizeof(define_t);
  strcpy( newdefine->name, define->name );
  newdefine->flags = define->flags;
  newdefine->builtin = define->builtin;
  newdefine->numparms = define->numparms;
  /* the define is not linked */
  newdefine->next = NULL;
  newdefine->hashnext = NULL;
  /* copy the define tokens */
  newdefine->tokens = NULL;
  for ( lasttoken = NULL, token = define->tokens; token; token = token->next )
  {
    newtoken = PC_CopyToken( token );
    newtoken->next = NULL;
    if ( lasttoken )
      lasttoken->next = newtoken;
    else
      newdefine->tokens = newtoken;
    lasttoken = newtoken;
  }
  /* copy the define parameters */
  newdefine->parms = NULL;
  for ( lasttoken = NULL, token = define->parms; token; token = token->next )
  {
    newtoken = PC_CopyToken( token );
    newtoken->next = NULL;
    if ( lasttoken )
      lasttoken->next = newtoken;
    else
      newdefine->parms = newtoken;
    lasttoken = newtoken;
  }
  return newdefine;
}

/* ---- PC_AddGlobalDefinesToSource  0x0043F8E0 ---- */
// PC_AddGlobalDefinesToSource | CONFIRMED | RTCW l_precomp.c PC_AddGlobalDefinesToSource -- iterates globaldefines list (via ->next at +24), copies each via PC_CopyDefine, computes name hash (poly 119, mask 0x3FF), inserts into source->definehash at +536.
void __cdecl PC_AddGlobalDefinesToSource(source_t *source)
{
  define_t *define, *newdefine;

  for ( define = (define_t *)globaldefines; define; define = define->next )
  {
    newdefine = PC_CopyDefine( source, define );
    /* inlined PC_AddDefineToHash */
    PC_AddDefineToHash( (define_t **)source->definehash, newdefine );
  }
}

/* ---- PC_Directive_if_def  0x0043F960 ---- */
// PC_Directive_if_def | CONFIRMED | RTCW
int __cdecl PC_Directive_if_def(source_t *source, int type)
{
  token_t token;
  define_t *d;
  int skip;

  if ( !PC_ReadLine( source, &token ) )
  {
    SourceError( source, "#ifdef without name" );
    return 0;
  }
  if ( token.type != TT_NAME )
  {
    PC_UnreadSourceToken(source,&token);
    SourceError( source, "expected name after #ifdef, found %s", token.string );
    return 0;
  }
  d = PC_FindHashedDefine( (define_t **)source->definehash, token.string );
  skip = ( type == INDENT_IFDEF ) == ( d == NULL );
  PC_PushIndent( source, type, skip );
  return 1;
}

/* ---- PC_Directive_ifdef  0x0043FA60 ---- */
// PC_Directive_ifdef | CONFIRMED | PC_Directive_if_def(source, 8).  8 = INDENT_IFDEF.
int __cdecl PC_Directive_ifdef(source_t *source)
{
  return PC_Directive_if_def(source, INDENT_IFDEF);
}

/* ---- PC_Directive_ifndef  0x0043FA80 ---- */
// PC_Directive_ifndef | CONFIRMED | PC_Directive_if_def(source, 16).  16 = INDENT_IFNDEF.
int __cdecl PC_Directive_ifndef(source_t *source)
{
  return PC_Directive_if_def(source, INDENT_IFNDEF);
}

/* ---- PC_Directive_else  0x0043FAA0 ---- */
// PC_Directive_else | CONFIRMED | RTCW
int __cdecl PC_Directive_else(source_t *source)
{
  int type, skip;

  PC_PopIndent( source, &type, &skip );
  if ( !type )
  {
    SourceError( source, "misplaced #else" );
    return 0;
  }
  if ( type == INDENT_ELSE )
  {
    SourceError( source, "#else after #else" );
    return 0;
  }
  PC_PushIndent( source, INDENT_ELSE, skip == 0 );
  return 1;
}

/* ---- PC_Directive_endif  0x0043FB10 ---- */
// PC_Directive_endif | CONFIRMED | RTCW
int __cdecl PC_Directive_endif(source_t *source)
{
  int type, skip;

  PC_PopIndent( source, &type, &skip );
  if ( !type )
  {
    SourceError( source, "misplaced #endif" );
    return 0;
  }
  return 1;
}

/* ---- PC_OperatorPriority  0x0043FB50 ---- */
// PC_OperatorPriority | CONFIRMED | Dense switch mapping punctuation subtype to C operator precedence; feeds PC_EvaluateTokens.
int __cdecl PC_OperatorPriority(int op)
{
  /* RTCW case order: VC7 places each shared return at its last case */
  switch ( op )
  {
    case 26: return 15;   /* P_MUL */
    case 27: return 15;   /* P_DIV */
    case 28: return 15;   /* P_MOD */
    case 29: return 14;   /* P_ADD */
    case 30: return 14;   /* P_SUB */

    case 5: return 7;     /* P_LOGIC_AND */
    case 6: return 6;     /* P_LOGIC_OR */
    case 7: return 12;    /* P_LOGIC_GEQ */
    case 8: return 12;    /* P_LOGIC_LEQ */
    case 9: return 11;    /* P_LOGIC_EQ */
    case 10: return 11;   /* P_LOGIC_UNEQ */

    case 36: return 16;   /* P_LOGIC_NOT */
    case 37: return 12;   /* P_LOGIC_GREATER */
    case 38: return 12;   /* P_LOGIC_LESS */

    case 21: return 13;   /* P_RSHIFT */
    case 22: return 13;   /* P_LSHIFT */

    case 32: return 10;   /* P_BIN_AND */
    case 33: return 8;    /* P_BIN_OR */
    case 34: return 9;    /* P_BIN_XOR */
    case 35: return 16;   /* P_BIN_NOT */

    case 42: return 5;    /* P_COLON */
    case 43: return 5;    /* P_QUESTIONMARK */
  }
  return 0;
}

/* ---- PC_EvaluateTokens  0x0043FC10 ---- */
// PC_EvaluateTokens | CONFIRMED | RTCW l_precomp.c:1649
/* RTCW l_precomp.c text.  Retail is __usercall(tokens@ecx, source, intvalue,
 * floatvalue, integer): LTCG lifts tokens into ECX and leaves the other four
 * on the stack.  Both callers (PC_Evaluate, PC_DollarEvaluate) pass the
 * firsttoken list head they free immediately afterwards.
 *
 * Retail's frame is `sub esp, 0D3Ch` under `and esp, -8` (0x0043FC13): the
 * two heaps tile it, operator_heap (64 x 20) at [esp+48h] and value_heap
 * (64 x 32, 8-aligned for floatvalue) at [esp+548h].  Live: the engine's PC_*
 * traps parse the UI .menu files, so any `#if` in one runs this. */

/* RTCW l_precomp.c operator_t / value_t, private to the evaluator there too. */
typedef struct operator_s
{
  int op;                           /* +0  -- RTCW `operator` */
  int priority;                     /* +4  */
  int parentheses;                  /* +8  */
  struct operator_s *prev, *next;   /* +12, +16 */
} operator_t;                       /* 20 bytes */

typedef struct value_s
{
  signed long int intvalue;         /* +0  */
  long double floatvalue;                /* +8  */
  int parentheses;                  /* +16 */
  struct value_s *prev, *next;      /* +20, +24 */
} value_t;                          /* 32 bytes */

/* RTCW l_script.h punctuation subtypes the evaluator switches on. */
#define P_LOGIC_AND         5
#define P_LOGIC_OR          6
#define P_LOGIC_GEQ         7
#define P_LOGIC_LEQ         8
#define P_LOGIC_EQ          9
#define P_LOGIC_UNEQ        10
#define P_INC               16
#define P_DEC               17
#define P_RSHIFT            21
#define P_LSHIFT            22
#define P_MUL               26
#define P_DIV               27
#define P_MOD               28
#define P_ADD               29
#define P_SUB               30
#define P_BIN_AND           32
#define P_BIN_OR            33
#define P_BIN_XOR           34
#define P_BIN_NOT           35
#define P_LOGIC_NOT         36
#define P_LOGIC_GREATER     37
#define P_LOGIC_LESS        38
#define P_COLON             42
#define P_QUESTIONMARK      43
#define P_PARENTHESESOPEN   44
#define P_PARENTHESESCLOSE  45

#define MAX_VALUES      64
#define MAX_OPERATORS   64
#define AllocValue( val )                                   \
  if ( numvalues >= MAX_VALUES ) {                          \
    SourceError( source, "out of value space\n" );          \
    error = 1;                                              \
    break;                                                  \
  }                                                         \
  else                                                      \
    val = &value_heap[numvalues++];
#define FreeValue( val )
#define AllocOperator( op )                                 \
  if ( numoperators >= MAX_OPERATORS ) {                    \
    SourceError( source, "out of operator space\n" );       \
    error = 1;                                              \
    break;                                                  \
  }                                                         \
  else                                                      \
    op = &operator_heap[numoperators++];
#define FreeOperator( op )

int __cdecl PC_EvaluateTokens(source_t *source, token_t *tokens, int *intvalue,
                              long double *floatvalue, int integer)
{
  operator_t *o, *firstoperator, *lastoperator;
  value_t *v, *firstvalue, *lastvalue, *v1, *v2;
  token_t *t;
  int brace = 0;
  int parentheses = 0;
  int error = 0;
  int lastwasvalue = 0;
  int negativevalue = 0;
  int questmarkintvalue = 0;
  double questmarkfloatvalue = 0;
  int gotquestmarkvalue = 0;
  int lastoperatortype = 0;
  operator_t operator_heap[MAX_OPERATORS];
  int numoperators = 0;
  value_t value_heap[MAX_VALUES];
  int numvalues = 0;

  firstoperator = lastoperator = NULL;
  firstvalue = lastvalue = NULL;
  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
    *floatvalue = 0;
  for ( t = tokens; t; t = t->next )
  {
    switch ( t->type )
    {
      case TT_NAME:
      {
        if ( lastwasvalue || negativevalue )
        {
          SourceError( source, "syntax error in #if/#elif" );
          error = 1;
          break;
        }
        if ( strcmp( t->string, "defined" ) )
        {
          SourceError( source, "undefined name %s in #if/#elif", t->string );
          error = 1;
          break;
        }
        t = t->next;
        if ( !strcmp( t->string, "(" ) )
        {
          brace = 1;
          t = t->next;
        }
        if ( !t || t->type != TT_NAME )
        {
          SourceError( source, "defined without name in #if/#elif" );
          error = 1;
          break;
        }
        AllocValue( v );
        if ( PC_FindHashedDefine( (define_t **)source->definehash, t->string ) )
        {
          v->intvalue = 1;
          v->floatvalue = 1;
        }
        else
        {
          v->intvalue = 0;
          v->floatvalue = 0;
        }
        v->parentheses = parentheses;
        v->next = NULL;
        v->prev = lastvalue;
        if ( lastvalue )
          lastvalue->next = v;
        else
          firstvalue = v;
        lastvalue = v;
        if ( brace )
        {
          t = t->next;
          if ( !t || strcmp( t->string, ")" ) )
          {
            SourceError( source, "defined without ) in #if/#elif" );
            error = 1;
            break;
          }
        }
        brace = 0;
        /* defined() creates a value */
        lastwasvalue = 1;
        break;
      }
      case TT_NUMBER:
      {
        if ( lastwasvalue )
        {
          SourceError( source, "syntax error in #if/#elif" );
          error = 1;
          break;
        }
        AllocValue( v );
        if ( negativevalue )
        {
          v->intvalue = -(signed int)t->intvalue;
          v->floatvalue = -t->floatvalue;
        }
        else
        {
          v->intvalue = t->intvalue;
          v->floatvalue = t->floatvalue;
        }
        v->parentheses = parentheses;
        v->next = NULL;
        v->prev = lastvalue;
        if ( lastvalue )
          lastvalue->next = v;
        else
          firstvalue = v;
        lastvalue = v;
        /* last token was a value */
        lastwasvalue = 1;
        negativevalue = 0;
        break;
      }
      case TT_PUNCTUATION:
      {
        if ( negativevalue )
        {
          SourceError( source, "misplaced minus sign in #if/#elif" );
          error = 1;
          break;
        }
        if ( t->subtype == P_PARENTHESESOPEN )
        {
          parentheses++;
          break;
        }
        else if ( t->subtype == P_PARENTHESESCLOSE )
        {
          parentheses--;
          if ( parentheses < 0 )
          {
            SourceError( source, "too many ) in #if/#elsif" );
            error = 1;
          }
          break;
        }
        /* check for invalid operators on floating point values */
        if ( !integer )
        {
          if ( t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
               t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
               t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
               t->subtype == P_BIN_XOR )
          {
            SourceError( source, "illigal operator %s on floating point operands\n", t->string );
            error = 1;
            break;
          }
        }
        switch ( t->subtype )
        {
          case P_LOGIC_NOT:
          case P_BIN_NOT:
          {
            if ( lastwasvalue )
            {
              SourceError( source, "! or ~ after value in #if/#elif" );
              error = 1;
              break;
            }
            break;
          }
          case P_INC:
          case P_DEC:
          {
            SourceError( source, "++ or -- used in #if/#elif" );
            break;
          }
          case P_SUB:
          {
            if ( !lastwasvalue )
            {
              negativevalue = 1;
              break;
            }
          }
          /* fall through */
          case P_MUL:
          case P_DIV:
          case P_MOD:
          case P_ADD:

          case P_LOGIC_AND:
          case P_LOGIC_OR:
          case P_LOGIC_GEQ:
          case P_LOGIC_LEQ:
          case P_LOGIC_EQ:
          case P_LOGIC_UNEQ:

          case P_LOGIC_GREATER:
          case P_LOGIC_LESS:

          case P_RSHIFT:
          case P_LSHIFT:

          case P_BIN_AND:
          case P_BIN_OR:
          case P_BIN_XOR:

          case P_COLON:
          case P_QUESTIONMARK:
          {
            if ( !lastwasvalue )
            {
              SourceError( source, "operator %s after operator in #if/#elif", t->string );
              error = 1;
              break;
            }
            break;
          }
          default:
          {
            SourceError( source, "invalid operator %s in #if/#elif", t->string );
            error = 1;
            break;
          }
        }
        if ( !error && !negativevalue )
        {
          AllocOperator( o );
          o->op = t->subtype;
          o->priority = PC_OperatorPriority( t->subtype );
          o->parentheses = parentheses;
          o->next = NULL;
          o->prev = lastoperator;
          if ( lastoperator )
            lastoperator->next = o;
          else
            firstoperator = o;
          lastoperator = o;
          lastwasvalue = 0;
        }
        break;
      }
      default:
      {
        SourceError( source, "unknown %s in #if/#elif", t->string );
        error = 1;
        break;
      }
    }
    if ( error )
      break;
  }
  if ( !error )
  {
    if ( !lastwasvalue )
    {
      SourceError( source, "trailing operator in #if/#elif" );
      error = 1;
    }
    else if ( parentheses )
    {
      SourceError( source, "too many ( in #if/#elif" );
      error = 1;
    }
  }
  gotquestmarkvalue = 0;
  questmarkintvalue = 0;
  questmarkfloatvalue = 0;
  /* while there are operators */
  while ( !error && firstoperator )
  {
    v = firstvalue;
    for ( o = firstoperator; o->next; o = o->next )
    {
      /* if the current operator is nested deeper in parentheses than the
         next operator */
      if ( o->parentheses > o->next->parentheses )
        break;
      /* if the current and next operator are nested equally deep */
      if ( o->parentheses == o->next->parentheses )
      {
        /* if the priority of the current operator is equal or higher than
           the priority of the next operator */
        if ( o->priority >= o->next->priority )
          break;
      }
      /* if the arity of the operator isn't equal to 1 */
      if ( o->op != P_LOGIC_NOT && o->op != P_BIN_NOT )
        v = v->next;
      /* if there's no value or no next value */
      if ( !v )
      {
        SourceError( source, "mising values in #if/#elif" );
        error = 1;
        break;
      }
    }
    if ( error )
      break;
    v1 = v;
    v2 = v->next;
    switch ( o->op )
    {
      case P_LOGIC_NOT:       v1->intvalue = !v1->intvalue;
                              v1->floatvalue = !v1->floatvalue; break;
      case P_BIN_NOT:         v1->intvalue = ~v1->intvalue;
                              break;
      case P_MUL:             v1->intvalue *= v2->intvalue;
                              v1->floatvalue *= v2->floatvalue; break;
      case P_DIV:             if ( !v2->intvalue || !v2->floatvalue )
                              {
                                SourceError( source, "divide by zero in #if/#elif\n" );
                                error = 1;
                                break;
                              }
                              v1->intvalue /= v2->intvalue;
                              v1->floatvalue /= v2->floatvalue; break;
      case P_MOD:             if ( !v2->intvalue )
                              {
                                SourceError( source, "divide by zero in #if/#elif\n" );
                                error = 1;
                                break;
                              }
                              v1->intvalue %= v2->intvalue; break;
      case P_ADD:             v1->intvalue += v2->intvalue;
                              v1->floatvalue += v2->floatvalue; break;
      case P_SUB:             v1->intvalue -= v2->intvalue;
                              v1->floatvalue -= v2->floatvalue; break;
      case P_LOGIC_AND:       v1->intvalue = v1->intvalue && v2->intvalue;
                              v1->floatvalue = v1->floatvalue && v2->floatvalue; break;
      case P_LOGIC_OR:        v1->intvalue = v1->intvalue || v2->intvalue;
                              v1->floatvalue = v1->floatvalue || v2->floatvalue; break;
      case P_LOGIC_GEQ:       v1->intvalue = v1->intvalue >= v2->intvalue;
                              v1->floatvalue = v1->floatvalue >= v2->floatvalue; break;
      case P_LOGIC_LEQ:       v1->intvalue = v1->intvalue <= v2->intvalue;
                              v1->floatvalue = v1->floatvalue <= v2->floatvalue; break;
      case P_LOGIC_EQ:        v1->intvalue = v1->intvalue == v2->intvalue;
                              v1->floatvalue = v1->floatvalue == v2->floatvalue; break;
      case P_LOGIC_UNEQ:      v1->intvalue = v1->intvalue != v2->intvalue;
                              v1->floatvalue = v1->floatvalue != v2->floatvalue; break;
      case P_LOGIC_GREATER:   v1->intvalue = v1->intvalue > v2->intvalue;
                              v1->floatvalue = v1->floatvalue > v2->floatvalue; break;
      case P_LOGIC_LESS:      v1->intvalue = v1->intvalue < v2->intvalue;
                              v1->floatvalue = v1->floatvalue < v2->floatvalue; break;
      case P_RSHIFT:          v1->intvalue >>= v2->intvalue;
                              break;
      case P_LSHIFT:          v1->intvalue <<= v2->intvalue;
                              break;
      case P_BIN_AND:         v1->intvalue &= v2->intvalue;
                              break;
      case P_BIN_OR:          v1->intvalue |= v2->intvalue;
                              break;
      case P_BIN_XOR:         v1->intvalue ^= v2->intvalue;
                              break;
      case P_COLON:
      {
        if ( !gotquestmarkvalue )
        {
          SourceError( source, ": without ? in #if/#elif" );
          error = 1;
          break;
        }
        if ( integer )
        {
          if ( !questmarkintvalue )
            v1->intvalue = v2->intvalue;
        }
        else
        {
          if ( !questmarkfloatvalue )
            v1->floatvalue = v2->floatvalue;
        }
        gotquestmarkvalue = 0;
        break;
      }
      case P_QUESTIONMARK:
      {
        if ( gotquestmarkvalue )
        {
          SourceError( source, "? after ? in #if/#elif" );
          error = 1;
          break;
        }
        questmarkintvalue = v1->intvalue;
        questmarkfloatvalue = v1->floatvalue;
        gotquestmarkvalue = 1;
        break;
      }
    }
    if ( error )
      break;
    lastoperatortype = o->op;
    /* if not an operator with arity 1 */
    if ( o->op != P_LOGIC_NOT && o->op != P_BIN_NOT )
    {
      /* remove the second value if not question mark operator */
      if ( o->op != P_QUESTIONMARK )
        v = v->next;
      if ( v->prev )
        v->prev->next = v->next;
      else
        firstvalue = v->next;
      if ( v->next )
        v->next->prev = v->prev;
      else
        lastvalue = v->prev;
      FreeValue( v );
    }
    /* remove the operator */
    if ( o->prev )
      o->prev->next = o->next;
    else
      firstoperator = o->next;
    if ( o->next )
      o->next->prev = o->prev;
    else
      lastoperator = o->prev;
    FreeOperator( o );
  }
  if ( firstvalue )
  {
    if ( intvalue )
      *intvalue = firstvalue->intvalue;
    if ( floatvalue )
      *floatvalue = firstvalue->floatvalue;
  }
  for ( o = firstoperator; o; o = lastoperator )
  {
    lastoperator = o->next;
    FreeOperator( o );
  }
  for ( v = firstvalue; v; v = lastvalue )
  {
    lastvalue = v->next;
    FreeValue( v );
  }
  if ( !error )
    return 1;
  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
    *floatvalue = 0;
  return 0;
}

/* 0x00440628-0x0044064F is NOT A FUNCTION: it is the JUMP TABLE for the
 * operator switch in PC_EvaluateTokens.  Its only cross-reference is a DATA
 * reference from 0x004400F5, inside that switch.  (0x00442784 in
 * l_script_mp.c is PS_ReadEscapeCharacter's jump table, likewise.) */

/* ---- PC_Evaluate  0x00440650 ---- */
// PC_Evaluate | CONFIRMED | RTCW l_precomp.c:2128
int __cdecl PC_Evaluate(source_t *source, int *intvalue, long double *floatvalue, int integer)
{
  token_t token;
  token_t *firsttoken, *lasttoken;
  token_t *t, *nexttoken;
  define_t *define;
  int defined = 0;

  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
    *floatvalue = 0;
  if ( !PC_ReadLine( source, &token ) )
  {
    SourceError( source, "no value after #if/#elif" );
    return 0;
  }
  firsttoken = NULL;
  lasttoken = NULL;
  do
  {
    /* if the token is a name */
    if ( token.type == TT_NAME )
    {
      if ( defined )
      {
        defined = 0;
        t = PC_CopyToken( &token );
        t->next = NULL;
        if ( lasttoken )
          lasttoken->next = t;
        else
          firsttoken = t;
        lasttoken = t;
      }
      else if ( !strcmp( token.string, "defined" ) )
      {
        defined = 1;
        t = PC_CopyToken( &token );
        t->next = NULL;
        if ( lasttoken )
          lasttoken->next = t;
        else
          firsttoken = t;
        lasttoken = t;
      }
      else
      {
        /* then it must be a define */
        define = PC_FindHashedDefine( (define_t **)source->definehash, token.string );
        if ( !define )
        {
          SourceError( source, "can't evaluate %s, not defined", token.string );
          return 0;
        }
        if ( !PC_ExpandDefineIntoSource( source, &token, define ) )
          return 0;
      }
    }
    /* if the token is a number or a punctuation */
    else if ( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
    {
      t = PC_CopyToken( &token );
      t->next = NULL;
      if ( lasttoken )
        lasttoken->next = t;
      else
        firsttoken = t;
      lasttoken = t;
    }
    else
    {
      SourceError( source, "can't evaluate %s", token.string );
      return 0;
    }
  }
  while ( PC_ReadLine( source, &token ) );
  /* tokens is the ECX argument in retail; it is the list this function frees below. */
  if ( !PC_EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) )
    return 0;
  for ( t = firsttoken; t; t = nexttoken )
  {
    nexttoken = t->next;
    if ( ((int *)t)[-1] == ZONE_MAGIC )
      free((int *)t - 1);
    numtokens--;
  }
  return 1;
}

/* ---- PC_DollarEvaluate  0x00440900 ---- */
// PC_DollarEvaluate | CONFIRMED | RTCW l_precomp.c:2232
/* Retail: source@<ecx> (`mov ebx, ecx` at 0x00440926) and the three stack
 * arguments at arg_0/arg_4/arg_8.  Spelled as an explicit leading parameter. */
int __cdecl PC_DollarEvaluate(source_t *source, int *intvalue, long double *floatvalue, int integer)
{
  token_t token;
  token_t *firsttoken, *lasttoken;
  token_t *t, *nexttoken;
  define_t *define;
  int indent, defined = 0;

  if ( intvalue )
    *intvalue = 0;
  if ( floatvalue )
    *floatvalue = 0;
  if ( !PC_ReadSourceToken(source,&token) )
  {
    SourceError( source, "no leading ( after $evalint/$evalfloat" );
    return 0;
  }
  if ( !PC_ReadSourceToken(source,&token) )
  {
    SourceError( source, "nothing to evaluate" );
    return 0;
  }
  indent = 1;
  firsttoken = NULL;
  lasttoken = NULL;
  do
  {
    /* if the token is a name */
    if ( token.type == TT_NAME )
    {
      if ( defined )
      {
        defined = 0;
        t = PC_CopyToken( &token );
        t->next = NULL;
        if ( lasttoken )
          lasttoken->next = t;
        else
          firsttoken = t;
        lasttoken = t;
      }
      else if ( !strcmp( token.string, "defined" ) )
      {
        defined = 1;
        t = PC_CopyToken( &token );
        t->next = NULL;
        if ( lasttoken )
          lasttoken->next = t;
        else
          firsttoken = t;
        lasttoken = t;
      }
      else
      {
        define = PC_FindHashedDefine( (define_t **)source->definehash, token.string );
        if ( !define )
        {
          SourceError( source, "can't evaluate %s, not defined", token.string );
          return 0;
        }
        if ( !PC_ExpandDefineIntoSource( source, &token, define ) )
          return 0;
      }
    }
    /* if the token is a number or a punctuation */
    else if ( token.type == TT_NUMBER || token.type == TT_PUNCTUATION )
    {
      if ( token.string[0] == '(' )
        indent++;
      else if ( token.string[0] == ')' )
        indent--;
      if ( indent <= 0 )
        break;
      t = PC_CopyToken( &token );
      t->next = NULL;
      if ( lasttoken )
        lasttoken->next = t;
      else
        firsttoken = t;
      lasttoken = t;
    }
    else
    {
      SourceError( source, "can't evaluate %s", token.string );
      return 0;
    }
  }
  while ( PC_ReadSourceToken(source,&token) );
  /* tokens is the ECX argument in retail; it is the list this function frees below. */
  if ( !PC_EvaluateTokens( source, firsttoken, intvalue, floatvalue, integer ) )
    return 0;
  for ( t = firsttoken; t; t = nexttoken )
  {
    nexttoken = t->next;
    if ( ((int *)t)[-1] == ZONE_MAGIC )
      free((int *)t - 1);
    numtokens--;
  }
  return 1;
}

/* ---- PC_Directive_elif  0x00440B70 ---- */
// PC_Directive_elif | CONFIRMED | RTCW
int __cdecl PC_Directive_elif(source_t *source)
{
  int value;
  int type, skip;

  PC_PopIndent( source, &type, &skip );
  if ( !type || type == INDENT_ELSE )
  {
    SourceError( source, "misplaced #elif" );
    return 0;
  }
  if ( !PC_Evaluate( source, &value, NULL, 1 ) )
    return 0;
  skip = (value == 0);
  PC_PushIndent( source, INDENT_ELIF, skip );
  return 1;
}

/* ---- PC_Directive_if  0x00440BE0 ---- */
// PC_Directive_if | CONFIRMED | PC_Evaluate(source, &value, NULL, qtrue) then PC_PushIndent(source, INDENT_IF=1, value == 0).
int __cdecl PC_Directive_if(source_t *source)
{
  int value;

  if ( !PC_Evaluate( source, &value, NULL, 1 ) )
    return 0;
  PC_PushIndent( source, INDENT_IF, value == 0 );
  return 1;
}

/* ---- PC_Directive_line  0x00440C20 ---- */
// PC_Directive_line | CONFIRMED | RTCW
int __cdecl PC_Directive_line(source_t *source)
{
  SourceError( source, "#line directive not supported" );
  return 0;
}

/* ---- PC_Directive_error  0x00440C40 ---- */
// PC_Directive_error | CONFIRMED | RTCW
int __cdecl PC_Directive_error(source_t *source)
{
  token_t token;

  token.string[0] = '\0';
  PC_ReadSourceToken(source,&token);
  SourceError( source, "#error directive: %s", token.string );
  return 0;
}

/* ---- PC_Directive_pragma  0x00440CA0 ---- */
// PC_Directive_pragma | CONFIRMED | RTCW
int __cdecl PC_Directive_pragma(source_t *source)
{
  token_t token;

  SourceWarning( source, "#pragma directive not supported" );
  while ( PC_ReadLine( source, &token ) )
    ;
  return 1;
}

/* ---- PC_UnreadSignToken  0x00440D10 ---- */
// PC_UnreadSignToken | CONFIRMED | Builds a '-' punctuation token on a stack token_t and unreads it.
token_t *__cdecl PC_UnreadSignToken(source_t *source)
{
  token_t token;
  token_t *t;

  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  strcpy( token.string, "-" );
  token.type = TT_PUNCTUATION;
  token.subtype = 30;                   /* P_SUB */
  /* inlined PC_UnreadSourceToken */
  t = PC_CopyToken( &token );
  t->next = source->tokens;
  source->tokens = t;
  return t;
}

/* ---- PC_Directive_eval  0x00440DC0 ---- */
// PC_Directive_eval | CONFIRMED | Evaluates and pushes the integer result back as a "%d" token.
int __cdecl PC_Directive_eval(source_t *source)
{
  int value;
  token_t token;

  if ( !PC_Evaluate( source, &value, NULL, 1 ) )
    return 0;
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf( token.string, "%d", abs(value) );
  token.type = TT_NUMBER;
  token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;   /* 12296 */
  PC_UnreadSourceToken(source,&token);
  if ( value < 0 )
    PC_UnreadSignToken( source );
  return 1;
}

/* ---- PC_Directive_evalfloat  0x00440ED0 ---- */
// PC_Directive_evalfloat | CONFIRMED | RTCW l_precomp.c:2466; reached from PC_ReadDirective 0x00440FE0
int __cdecl PC_Directive_evalfloat(source_t *source)
{
  double value;
  token_t token;

  if ( !PC_Evaluate( source, NULL, &value, 0 ) )
    return 0;
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf( token.string, "%1.2f", (double)fabs(value) );
  token.type = TT_NUMBER;
  token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;   /* 10248 */
  PC_UnreadSourceToken(source,&token);
  /* retail: fcomp, test ah,5, jp -- a NaN value skips the sign, as `<` does */
  if ( value < 0.0 )
    PC_UnreadSignToken( source );
  return 1;
}

/* PC_ReadDirective (0x00440FE0) is in botlib/l_precomp_pc.c, together with
 * the 14-entry `directives[]` table at 0x0057AAA0.  Declared in l_script.h. */

/* ---- PC_DollarDirective_evalint  0x00441100 ---- */
// PC_DollarDirective_evalint | CONFIRMED | $evalint twin of PC_Directive_eval; pairs with PC_DollarDirective_evalfloat at 0x441220.
int __cdecl PC_DollarDirective_evalint(source_t *source)
{
  int value;
  token_t token;

  if ( !PC_DollarEvaluate( source, &value, NULL, 1 ) )
    return 0;
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf( token.string, "%d", abs(value) );
  token.type = TT_NUMBER;
  token.floatvalue = (double)value;
  token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;   /* 12296 */
  token.intvalue = (unsigned long)value;
  PC_UnreadSourceToken(source,&token);
  if ( value < 0 )
    PC_UnreadSignToken( source );
  return 1;
}

/* ---- PC_DollarDirective_evalfloat  0x00441220 ---- */
// PC_DollarDirective_evalfloat | CONFIRMED | RTCW l_precomp.c:2576; reached from PC_ReadDollarDirective 0x00441350
int __cdecl PC_DollarDirective_evalfloat(source_t *source)
{
  double value;
  token_t token;

  if ( !PC_DollarEvaluate( source, NULL, &value, 0 ) )
    return 0;
  token.line = source->scriptstack->line;
  token.whitespace_p = source->scriptstack->script_p;
  token.endwhitespace_p = source->scriptstack->script_p;
  token.linescrossed = 0;
  sprintf( token.string, "%1.2f", (double)fabs(value) );
  token.type = TT_NUMBER;
  token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;   /* 10248 */
  token.intvalue = (unsigned long)value;
  token.floatvalue = value;
  PC_UnreadSourceToken(source,&token);
  /* retail: fcomp, test ah,5, jp -- a NaN value skips the sign, as `<` does */
  if ( value < 0.0 )
    PC_UnreadSignToken( source );
  return 1;
}

/* PC_ReadDollarDirective (0x00441350) is in botlib/l_precomp_pc.c, with the
 * 2-entry `dollardirectives[]` table at 0x0057AB40.  Declared in l_script.h. */

/* ---- PC_ReadToken  0x00441490 ---- */
// PC_ReadToken | CONFIRMED | RTCW
int __cdecl PC_ReadToken(source_t *source, token_t *token)
{
  define_t *define;

  while ( 1 )
  {
    if ( !PC_ReadSourceToken(source,token) )
      return 0;
    /* check for precompiler directives */
    if ( token->type == TT_PUNCTUATION && token->string[0] == '#' )
    {
      if ( !PC_ReadDirective( source ) )
        return 0;
      continue;
    }
    if ( token->type == TT_PUNCTUATION && token->string[0] == '$' )
    {
      if ( !PC_ReadDollarDirective( source ) )
        return 0;
      continue;
    }
    /* recursively concatenate strings that are behind each other still
       resolving defines */
    if ( token->type == TT_STRING )
    {
      token_t newtoken;
      if ( PC_ReadToken( source, &newtoken ) )
      {
        if ( newtoken.type == TT_STRING )
        {
          token->string[strlen(token->string) - 1] = '\0';
          if ( strlen(token->string) + strlen(newtoken.string + 1) + 1 >= MAX_TOKEN )
          {
            SourceError( source, "string longer than MAX_TOKEN %d\n", MAX_TOKEN );
            return 0;
          }
          strcat( token->string, newtoken.string + 1 );
        }
        else
        {
          PC_UnreadSourceToken(source,&newtoken);
        }
      }
    }
    /* if skipping source because of conditional compilation */
    if ( source->skip )
      continue;
    /* if the token is a name, check if it is a define macro */
    if ( token->type == TT_NAME )
    {
      define = PC_FindHashedDefine( (define_t **)source->definehash, token->string );
      if ( define )
      {
        /* expand the defined macro */
        if ( !PC_ExpandDefineIntoSource( source, token, define ) )
          return 0;
        continue;
      }
    }
    /* copy the token for unreading, and return it */
    memcpy( &source->token, token, sizeof(token_t) );
    return 1;
  }
}

/* ---- PC_ExpectTokenString  0x00441650 ---- */
// PC_ExpectTokenString | CONFIRMED | RTCW
int __cdecl PC_ExpectTokenString(const char *string, source_t *source)
{
  token_t token;

  if ( !PC_ReadToken( source, &token ) )
  {
    SourceError( source, "couldn't find expected %s", string );
    return 0;
  }
  if ( strcmp( token.string, string ) )
  {
    SourceError( source, "expected %s, found %s", string, token.string );
    return 0;
  }
  return 1;
}

/* ---- PC_ExpectTokenType  0x00441740 ----  IMPLEMENTED (retail) */
/* Unreachable in the RETAIL binary, not merely on a dedicated server: no
 * xrefs at all (0x00441740).  Kept for retail fidelity.
 *
 * RTCW l_precomp.c PC_ExpectTokenType, same control flow and strings (all
 * confirmed in .rdata 0x00561274-0x00561318).
 *
 * RETAIL: __usercall( subtype@<ebx>, type@<esi>, source, token on stack ),
 * cdecl cleanup.  Written here as __cdecl in RTCW source order
 * ( source, type, subtype, token ), matching PS_ExpectTokenType in
 * l_script_mp.c.  `str` is the MAX_TOKEN (0x400) buffer under the cookie.
 */
int __cdecl PC_ExpectTokenType( source_t *source, int type, int subtype, token_t *token )
{
  char str[MAX_TOKEN];

  if ( !PC_ReadToken( source, token ) )
  {
    SourceError( source, "couldn't read expected token" );
    return 0;
  }

  if ( token->type != type )
  {
    /* retail stores str[0] = 0, then an if/else chain on `type`; an unknown
       type leaves str empty. */
    strcpy( str, "" );
    if ( type == TT_STRING )
      strcpy( str, "string" );
    if ( type == TT_LITERAL )
      strcpy( str, "literal" );
    if ( type == TT_NUMBER )
      strcpy( str, "number" );
    if ( type == TT_NAME )
      strcpy( str, "name" );
    if ( type == TT_PUNCTUATION )
      strcpy( str, "punctuation" );
    SourceError( source, "expected a %s, found %s", str, token->string );
    return 0;
  }

  if ( token->type == TT_NUMBER )
  {
    if ( ( token->subtype & subtype ) != subtype )
    {
      /* str is not cleared on this path: a subtype with none of the four
         radix bits leaves it uninitialised before the strcats.  RTCW has
         the same hole. */
      if ( subtype & TT_DECIMAL )
        strcpy( str, "decimal" );
      if ( subtype & TT_HEX )
        strcpy( str, "hex" );   /* retail: one dword store of 0x00786568 */
      if ( subtype & TT_OCTAL )
        strcpy( str, "octal" );
      if ( subtype & TT_BINARY )
        strcpy( str, "binary" );
      if ( subtype & TT_LONG )
        strcat( str, " long" );
      if ( subtype & TT_UNSIGNED )
        strcat( str, " unsigned" );
      if ( subtype & TT_FLOAT )
        strcat( str, " float" );
      if ( subtype & TT_INTEGER )
        strcat( str, " integer" );
      SourceError( source, "expected %s, found %s", str, token->string );
      return 0;
    }
  }
  else if ( token->type == TT_PUNCTUATION )
  {
    /* unlike PS_ExpectTokenType there is no "wrong punctuation subtype"
       guard and no punctuation-table lookup; just the found string. */
    if ( token->subtype != subtype )
    {
      SourceError( source, "found %s", token->string );
      return 0;
    }
  }
  return 1;
}

/* ---- PC_ExpectAnyToken  0x004419F0 ---- */
// PC_ExpectAnyToken | HIGH | RTCW PC_ExpectAnyToken; 37-byte thin wrapper sharing its error string with PC_ExpectTokenType.
int __cdecl PC_ExpectAnyToken(token_t *token, source_t *source)
{
  if ( !PC_ReadToken( source, token ) )
  {
    SourceError( source, "couldn't read expected token" );
    return 0;
  }
  return 1;
}

/* ---- PC_CheckTokenString  0x00441A20 ---- */
// PC_CheckTokenString | CONFIRMED | Reads a token, returns 1 on match, otherwise pushes a copy back onto source->tokens (+528).
int __cdecl PC_CheckTokenString(source_t *source, const char *string)
{
  token_t token;

  if ( !PC_ReadToken( source, &token ) )
    return 0;
  /* if the token is available */
  if ( !strcmp( token.string, string ) )
    return 1;
  /* else push it back */
  PC_UnreadSourceToken(source,&token);
  return 0;
}

/* ---- PC_CheckTokenType  0x00441AF0 ---- */
// PC_CheckTokenType | CONFIRMED | Same shape as PC_CheckTokenString but matches token->type (+1024) and masks token->subtype (+1028); copies the whole 0x430-byte token out on success.
/* RTCW order; LTCG gives source EDX and subtype ECX, type and token stay on the stack. */
int __cdecl PC_CheckTokenType(source_t *source, int type, int subtype, token_t *out_token)
{
  token_t token;

  if ( !PC_ReadToken( source, &token ) )
    return 0;
  /* if the type matches */
  if ( token.type == type && (token.subtype & subtype) == subtype )
  {
    memcpy( out_token, &token, sizeof(token_t) );
    return 1;
  }
  /* else push it back */
  PC_UnreadSourceToken(source,&token);
  return 0;
}

/* ---- PC_SkipUntilString  0x00441BA0 ---- */
// PC_SkipUntilString | CONFIRMED | Reads tokens until strcmp matches or the source runs dry.
int __cdecl PC_SkipUntilString(const char *string, source_t *source)
{
  token_t token;

  while ( PC_ReadToken( source, &token ) )
  {
    if ( !strcmp( token.string, string ) )
      return 1;
  }
  return 0;
}

/* ---- PC_UnreadLastToken  0x00441C50 ---- */
// PC_UnreadLastToken | CONFIRMED | PC_CopyToken(&source->token at +552) pushed onto source->tokens (+528).
token_t *__cdecl PC_UnreadLastToken(source_t *source)
{
  token_t *t;

  /* inlined PC_UnreadSourceToken( &source->token, source ) */
  t = PC_CopyToken( &source->token );
  t->next = source->tokens;
  source->tokens = t;
  return t;
}

/* ---- PC_UnreadToken  0x00441C80 ---- */
// PC_UnreadToken | CONFIRMED | Same as above for an arbitrary token.
token_t *__cdecl PC_UnreadToken(const token_t *token, source_t *source)
{
  token_t *t;

  t = PC_CopyToken( token );
  t->next = source->tokens;
  source->tokens = t;
  return t;
}

/* ---- PC_SetIncludePath  0x00441CA0 ---- */
// PC_SetIncludePath | CONFIRMED | strncpy into source->includepath (+260, MAX_PATH 0x104) and appends a separator when one is missing.
void __cdecl PC_SetIncludePath(const char *path, source_t *source)
{
  strncpy( source->includepath, path, 260 );   /* _MAX_PATH */
  /* add trailing path separator */
  if ( source->includepath[strlen(source->includepath) - 1] != '\\' &&
       source->includepath[strlen(source->includepath) - 1] != '/' )
  {
    strcat( source->includepath, "\\" );       /* PATHSEPERATOR_STR */
  }
}

/* ---- PC_SetPunctuations  0x00441D10 ---- */
// PC_SetPunctuations | CONFIRMED | Single store into source->punctuations (+520).
punctuation_t *__cdecl PC_SetPunctuations(punctuation_t *p, source_t *source)
{
  source->punctuations = p;
  return p;
}

/* LoadSourceFile (0x00441D20) is in botlib/l_precomp_pc.c.  One argument. */

/* ---- LoadSourceMemory  0x00441DE0 ---- */
// LoadSourceMemory | CONFIRMED | LoadScriptMemory twin of LoadSourceFile.
/* RTCW l_precomp.c:2952, VERBATIM.  DEAD in retail -- zero xrefs.
 *
 * Retail is __usercall( ptr@<ecx>, length on stack arg_0, name on stack
 * arg_4 ) -- RTCW's ( ptr, length, name ) with only the first argument lifted
 * into a register.  Source order here. */
source_t *__cdecl LoadSourceMemory(char *ptr, int length, char *name)
{
  script_t *script;
  source_t *source;

  script = LoadScriptMemory( ptr, length, name );
  if ( !script )
    return NULL;
  script->next = NULL;
  source = (source_t *)GetMemory( sizeof(source_t) );   /* 0x65C with the magic */
  memset( source, 0, sizeof(source_t) );
  strncpy( source->filename, name, 260 );   /* _MAX_PATH */
  source->scriptstack = script;
  source->tokens = NULL;
  source->defines = NULL;
  source->indentstack = NULL;
  source->skip = 0;
  source->definehash = GetClearedMemory( DEFINEHASHSIZE * sizeof(define_t *) );
  PC_AddGlobalDefinesToSource( source );
  return source;
}

/* ---- FreeSource  0x00441EA0 ---- */
// FreeSource | CONFIRMED | Frees scripts, tokens, defines and indents; called by PC_FreeSourceHandle.
void __cdecl FreeSource(source_t *source)
{
  script_t *script;
  token_t *token;
  define_t *define;
  indent_t *indent;
  int i;

  /* free all the scripts (inlined FreeScript) */
  while ( source->scriptstack )
  {
    script = source->scriptstack;
    source->scriptstack = script->next;
    if ( script->punctuationtable && ((int *)script->punctuationtable)[-1] == ZONE_MAGIC )
      free((int *)script->punctuationtable - 1);
    if ( ((int *)script)[-1] == ZONE_MAGIC )
      free((int *)script - 1);
  }
  /* free all the tokens (inlined PC_FreeToken) */
  while ( source->tokens )
  {
    token = source->tokens;
    source->tokens = token->next;
    if ( ((int *)token)[-1] == ZONE_MAGIC )
      free((int *)token - 1);
    numtokens--;
  }
  /* free all the defines */
  for ( i = 0; i < DEFINEHASHSIZE; i++ )
  {
    while ( ((define_t **)source->definehash)[i] )
    {
      define = ((define_t **)source->definehash)[i];
      ((define_t **)source->definehash)[i] = ((define_t **)source->definehash)[i]->hashnext;
      PC_FreeDefine( define );
    }
  }
  /* free all the indents (inlined FreeMemory) */
  while ( source->indentstack )
  {
    indent = (indent_t *)source->indentstack;
    source->indentstack = indent->next;
    if ( ((int *)indent)[-1] == ZONE_MAGIC )
      free((int *)indent - 1);
  }
  /* free the hash table and the source itself */
  if ( source->definehash && ((int *)source->definehash)[-1] == ZONE_MAGIC )
    free((int *)source->definehash - 1);
  if ( ((int *)source)[-1] == ZONE_MAGIC )
    free((int *)source - 1);
}

/* PC_LoadSourceHandle (0x00442010), PC_FreeSourceHandle (0x00442100),
 * PC_ReadTokenHandle (0x00442140) and PC_SourceFileAndLine (0x00442220)
 * are in botlib/l_precomp_pc.c, together with the 64-entry sourceFiles table
 * they share and PC_CheckOpenSourceHandles.  All four are __usercall; see
 * the header comment in that file for the register bindings. */

/* ---- PC_SetBaseFolder  0x00442270 ---- */
// PC_SetBaseFolder | CONFIRMED | Com_sprintf into the module-static base folder buffer.
/* Com_sprintf is __usercall: destination in EDI, size in ESI, so only the
   format string is pushed.  Retail at 0x00442270 is

       mov  esi, 40h                     ; size  = 64
       mov  edi, offset pc_baseFolder     ; dest  = the base folder buffer
       push eax                          ; fmt   = the caller's path

   RtCW passes the path as the format string here too, which is sloppy but is
   what the binary does; a path containing a '%' would be misread by both. */
void __cdecl PC_SetBaseFolder(char *path)
{
  Com_sprintf((char *)pc_baseFolder, 64, path);
}

/* PC_CheckOpenSourceHandles (0x00442290) is in botlib/l_precomp_pc.c; it
 * walks the same private sourceFiles table the four handle functions do. */
