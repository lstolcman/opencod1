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
char rendererVertexPrograms[9488];
int r_numVertexPrograms[196];



/* ------------------------------------------------------------------------- *
 *  ARB_vertex_program setup / loading.
 *
 *  rendererVertexPrograms (defined above) is a flat blob of up to
 *  R_MAX_VERTEX_PROGRAMS fixed-size records; r_numVertexPrograms[0] holds the
 *  live count.  Both are standalone globals kept on the blob+symbol storage
 *  model, so only the per-record pointer view is typed here (vertexProgram_t),
 *  turning the *(int *)(entry + 64) soup into entry->glName.  Rather than ask
 *  glGenProgramsARB for names, the loader uses (slot index + 1) as the ARB
 *  program object id, so the id is implicit in the record's slot.
 * ------------------------------------------------------------------------- */

#define R_MAX_VERTEX_PROGRAMS          128       /* rendererVertexPrograms capacity */

#define GL_VERTEX_PROGRAM_ARB          0x8620
#define GL_PROGRAM_FORMAT_ASCII_ARB    0x8875
#define GL_PROGRAM_ERROR_POSITION_ARB  0x864B
#define GL_PROGRAM_ERROR_STRING_ARB    0x8874

/* one entry of the rendererVertexPrograms table (record stride 68 = 0x44) */
typedef struct {
	char   name[64];      /* +0x00  scripts/<name>.vp10 lookup key */
	GLuint glName;        /* +0x40  ARB program object id (slot index + 1) */
} vertexProgram_t;
COD1_ASSERT_SIZE(vertexProgram_t, 68);
ASSERT_OFF(vertexProgram_t, glName, 64);

int R_DeleteVertexPrograms( void );

/* ---- R_LoadVertexProgram  0x004EA150 ----  VERIFIED
 * Read scripts/<name>.vp10, claim the next table slot (ARB id = index + 1) and,
 * when ARB_vertex_program is available, upload and validate the program.
 * Returns the new table entry, or NULL on any failure. */
char *__cdecl R_LoadVertexProgram( const char *name )
{
  vertexProgram_t *program;
  const char      *path;
  const char      *errorString;
  int              fileLength;
  int              errorPosition;   /* qglGetIntegerv out-parameter */
  void            *fileBuffer;      /* ri.FS_ReadFile out-parameter */

  if ( r_numVertexPrograms[0] == R_MAX_VERTEX_PROGRAMS )
  {
    ri.Printf( 2, "WARNING: tried to load more than %i unique vertex programs in a single map\n",
               R_MAX_VERTEX_PROGRAMS );
    return 0;
  }

  path = va( "scripts/%s.vp10", name );

  fileLength = ri.FS_ReadFile( path, &fileBuffer );
  if ( fileLength < 0 )
  {
    ri.Printf( 2, "WARNING: couldn't open vertex program '%s'\n", path );
    return 0;
  }

  program = &( (vertexProgram_t *)rendererVertexPrograms )[ r_numVertexPrograms[0] ];
  program->glName = r_numVertexPrograms[0] + 1;
  strcpy( program->name, name );

  if ( glConfig_ARBVertexProgram )
  {
    qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, program->glName );
    qglProgramStringARB( GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, fileLength, fileBuffer );
    qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, 0 );

    qglGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, &errorPosition );
    if ( errorPosition >= 0 )
    {
      /* NOTE: retail's call site (0x4EA240..0x4EA25D) pushes only three
       * varargs for four conversions: path, errorPosition, errorString (a
       * source-level missing argument, plain C, not a miscompile).  So the
       * 0x864B out-parameter lands on the second '%s' (retail prints "(null)"
       * when it is 0 and takes an access violation otherwise), the ARB
       * error-string pointer is printed as the '%i', and the trailing '%s'
       * reads the next stack dword: the caller's saved ESI, which is
       * R_FindVertexProgram's loop counter and always equals
       * r_numVertexPrograms[0] here.  The modern build passes that value
       * explicitly as a fourth argument so it reads the same value retail
       * does instead of undefined stack; the VC7 build keeps retail's
       * three-argument call (byte-identical).  (The earlier Hex-Rays reading, with
       * 'retaddr'/'v10' as the '%s' operands, was an artifact of a wrong
       * stack-pointer delta after the __stdcall qglGetString.)  The path only
       * runs on a vertex program that fails to compile, which never occurred
       * in shipped data. */
      errorString = (const char *)qglGetString( GL_PROGRAM_ERROR_STRING_ARB );
#ifdef COD1_VC7
      /* retail's three-vararg call; the trailing '%s' reads the saved ESI */
      ri.Printf( 2, "WARNING: shader '%s': error in vertex program '%s' at char %i: %s\n",
                 path, errorPosition, errorString );
#else
      ri.Printf( 2, "WARNING: shader '%s': error in vertex program '%s' at char %i: %s\n",
                 path, errorPosition, errorString, r_numVertexPrograms[0] );
#endif
      qglDeleteProgramsARB( 1, &program->glName );
      ri.FS_FreeFile( fileBuffer );
      return 0;
    }
  }

  ri.FS_FreeFile( fileBuffer );
  ++r_numVertexPrograms[0];
  return (char *)program;
}

/* ---- R_FindVertexProgram  0x004EA2B0 ----  VERIFIED
 * Return the cached entry whose name matches (case-insensitive), loading it on
 * demand when it is not present yet. */
char *__cdecl R_FindVertexProgram( const char *name )
{
  int i;

  for ( i = 0; i < r_numVertexPrograms[0]; ++i )
  {
    if ( !_stricmp( name, ( (vertexProgram_t *)rendererVertexPrograms )[i].name ) )
      return (char *)&( (vertexProgram_t *)rendererVertexPrograms )[i];
  }

  return R_LoadVertexProgram( name );
}

/* ---- R_InitVertexPrograms  0x004EA300 ----  VERIFIED
 * Reset the vertex-program table at renderer init (thunk to the deleter). */
int R_InitVertexPrograms( void )
{
  return R_DeleteVertexPrograms();
}

/* ---- R_DeleteVertexPrograms  0x004EA310 ----  VERIFIED
 * Release every registered ARB program and clear the table.  Returns the freed
 * count as dead EAX (glConfig_ARBVertexProgram when ARB is unavailable). */
int R_DeleteVertexPrograms( void )
{
  vertexProgram_t *program;
  int              result;
  int              i;

  result = glConfig_ARBVertexProgram;
  if ( glConfig_ARBVertexProgram )
  {
    qglBindProgramARB( GL_VERTEX_PROGRAM_ARB, 0 );
    result = r_numVertexPrograms[0];
    glState_boundVertexProgram = 0;

    program = (vertexProgram_t *)rendererVertexPrograms;
    for ( i = 0; i < r_numVertexPrograms[0]; ++i, ++program )
    {
      qglDeleteProgramsARB( 1, &program->glName );
      result = r_numVertexPrograms[0];   /* dead re-read, kept 1:1 with retail */
    }
  }

  r_numVertexPrograms[0] = 0;
  return result;
}
