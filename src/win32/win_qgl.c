/*
 * @fidelity: likely
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "../qcommon/qcommon.h"

#define QGL_OWNER
#include "../renderer/qgl.h"
#include "win_wgl.h"
#include "../renderer/tr_public.h"
#include "../renderer/tr_local.h"   /* renderer cvars, glState, backEndStorage (RTCW includes it here too) */

#define QGL_PRINT_ALL       0

extern cvar_t *r_debugGLErrors;     /* 0x016C3728, defined in renderer/tr_init.c */

/* Read by the draw-profiling wrappers.  tr.frameCount and backEnd.pc.c_drawnIndexes /
   backEnd.projection2D are fields of the renderer's tr (tr_registered, 0x016C4D40) and
   backEnd (backEndStorage, 0x016D89C0) objects. */
extern cvar_t *r_profileDrawElements;       /* 0x016C3700, defined in renderer/tr_init.c */
#define tr_frameCount               ( *(int *)( (char *)(void *)&tr_registered + 0x0004 ) )   /* 0x016C4D44 */
#define backEnd_pc_drawnIndexCount  ( *(int *)( (char *)(void *)backEndStorage + 0x0474 ) )   /* 0x016D8E34 */
#define backEnd_projection2D        ( *(int *)( (char *)(void *)backEndStorage + 0x04B0 ) )   /* 0x016D8E70 */

void QGL_EnableErrorChecking( qboolean enable );
void QGL_EnableLogging( qboolean enable );

/* Retail stores the GetProcAddress result in the driver slot, then copies the
   driver slot into the qgl pointer (qglX = dllX = GetProcAddress( ... )). */
#define QGL_BIND_GL( n ) \
	( *(PROC *)&dll##n = GetProcAddress( glw_state.hinstOpenGL, "gl" #n ), qgl##n = dll##n )
#define QGL_BIND_WGL( n ) \
	( *(PROC *)&dllwgl##n = GetProcAddress( glw_state.hinstOpenGL, "wgl" #n ), qwgl##n = dllwgl##n )

/* Entry points outside qgl_entries.h that retail still gives a qgl pointer, a
   driver slot, a gl.log wrapper and a glGetError / GetLastError wrapper:
   wglGetExtensionsStringEXT (GLimp_Init fetches it, see win_wgl.h),
   glMultiTexCoord2fARB and the WGL_ARB_pixel_format / WGL_ARB_pbuffer /
   WGL_ARB_render_texture set.  Nothing ever fetches the last twelve: QGL_Shutdown
   and QCL_Init NULL them and the arming code wraps only non-NULL pointers, so
   they stay NULL, as in retail. */
DECLARE_HANDLE( HPBUFFERARB );

/* Driver slot for qwglGetExtensionsStringEXT (win_wgl.h); GLimp_Init stores the
   fetched address in both (0x0050ACE3). */
const char  *( WINAPI * dllwglGetExtensionsStringEXT )( void );                                        /* 0x016C43F0 */

static void  ( QGLAPI * qglMultiTexCoord2fARB )( GLenum target, GLfloat s, GLfloat t );                /* 0x016C4254 */
static BOOL  ( WINAPI * qwglGetPixelFormatAttribivARB )( HDC, int, int, UINT, const int *, int * );     /* 0x016C3D6C */
static BOOL  ( WINAPI * qwglGetPixelFormatAttribfvARB )( HDC, int, int, UINT, const int *, FLOAT * );   /* 0x016C428C */
static BOOL  ( WINAPI * qwglChoosePixelFormatARB )( HDC, const int *, const FLOAT *, UINT, int *, UINT * ); /* 0x016C4A48 */
static HPBUFFERARB ( WINAPI * qwglCreatePbufferARB )( HDC, int, int, int, const int * );               /* 0x016C401C */
static HDC   ( WINAPI * qwglGetPbufferDCARB )( HPBUFFERARB );                                          /* 0x016C3ED0 */
static int   ( WINAPI * qwglReleasePbufferDCARB )( HPBUFFERARB, HDC );                                 /* 0x016C4834 */
static BOOL  ( WINAPI * qwglDestroyPbufferARB )( HPBUFFERARB );                                        /* 0x016C44E4 */
static BOOL  ( WINAPI * qwglQueryPbufferARB )( HPBUFFERARB, int, int * );                              /* 0x016C3EC4 */
static BOOL  ( WINAPI * qwglBindTexImageARB )( HPBUFFERARB, int );                                     /* 0x016C4744 */
static BOOL  ( WINAPI * qwglReleaseTexImageARB )( HPBUFFERARB, int );                                  /* 0x016C4030 */
static BOOL  ( WINAPI * qwglSetPbufferAttribARB )( HPBUFFERARB, const int * );                         /* 0x016C3C54 */

static void  ( QGLAPI * dllMultiTexCoord2fARB )( GLenum target, GLfloat s, GLfloat t );                /* 0x016C4984 */
static BOOL  ( WINAPI * dllwglGetPixelFormatAttribivARB )( HDC, int, int, UINT, const int *, int * );   /* 0x016C434C */
static BOOL  ( WINAPI * dllwglGetPixelFormatAttribfvARB )( HDC, int, int, UINT, const int *, FLOAT * ); /* 0x016C4090 */
static BOOL  ( WINAPI * dllwglChoosePixelFormatARB )( HDC, const int *, const FLOAT *, UINT, int *, UINT * ); /* 0x016C41A0 */
static HPBUFFERARB ( WINAPI * dllwglCreatePbufferARB )( HDC, int, int, int, const int * );             /* 0x016C4198 */
static HDC   ( WINAPI * dllwglGetPbufferDCARB )( HPBUFFERARB );                                        /* 0x016C4000 */
static int   ( WINAPI * dllwglReleasePbufferDCARB )( HPBUFFERARB, HDC );                               /* 0x016C3F8C */
static BOOL  ( WINAPI * dllwglDestroyPbufferARB )( HPBUFFERARB );                                      /* 0x016C43DC */
static BOOL  ( WINAPI * dllwglQueryPbufferARB )( HPBUFFERARB, int, int * );                            /* 0x016C3E68 */
static BOOL  ( WINAPI * dllwglBindTexImageARB )( HPBUFFERARB, int );                                   /* 0x016C3F0C */
static BOOL  ( WINAPI * dllwglReleaseTexImageARB )( HPBUFFERARB, int );                                /* 0x016C4368 */
static BOOL  ( WINAPI * dllwglSetPbufferAttribARB )( HPBUFFERARB, const int * );                       /* 0x016C498C */

/* Retail's order for every walk over the qgl pointers (QGL_Shutdown, QCL_Init,
   QGL_EnableErrorChecking, QGL_EnableLogging): these extension entries, then the
   core ones in qgl_entries.h order, then the tail below.  Together the two lists
   hold qgl_entries.h's extension entries and the extra ones declared above. */
#define QGL_LOG_EXT_ENTRIES( GL, WGL ) \
	GL( MultiTexCoord2fARB ) \
	GL( ActiveTextureARB ) \
	GL( ClientActiveTextureARB ) \
	GL( LockArraysEXT ) \
	GL( UnlockArraysEXT ) \
	GL( PNTrianglesiATI ) \
	GL( PNTrianglesfATI ) \
	GL( DrawRangeElementsEXT ) \
	GL( CompressedTexImage3DARB ) \
	GL( CompressedTexImage2DARB ) \
	GL( CompressedTexImage1DARB ) \
	GL( CompressedTexSubImage3DARB ) \
	GL( CompressedTexSubImage2DARB ) \
	GL( CompressedTexSubImage1D ) \
	GL( GetCompressedTexImage ) \
	GL( BindBufferARB ) \
	GL( DeleteBuffersARB ) \
	GL( GenBuffersARB ) \
	GL( IsBufferARB ) \
	GL( BufferDataARB ) \
	GL( BufferSubDataARB ) \
	GL( GetBufferSubDataARB ) \
	GL( MapBufferARB ) \
	GL( UnmapBufferARB ) \
	GL( GetBufferParameterivARB ) \
	GL( GetBufferPointervARB ) \
	GL( NewObjectBufferATI ) \
	GL( IsObjectBufferATI ) \
	GL( UpdateObjectBufferATI ) \
	GL( GetObjectBufferfvATI ) \
	GL( GetObjectBufferivATI ) \
	GL( FreeObjectBufferATI ) \
	GL( ArrayObjectATI ) \
	GL( GetArrayObjectfvATI ) \
	GL( GetArrayObjectivATI ) \
	GL( VariantArrayObjectATI ) \
	GL( GetVariantArrayObjectfvATI ) \
	GL( GetVariantArrayObjectivATI ) \
	GL( ElementPointerATI ) \
	GL( DrawElementArrayATI ) \
	GL( DrawRangeElementArrayATI ) \
	GL( FlushVertexArrayRangeNV ) \
	GL( VertexArrayRangeNV ) \
	WGL( AllocateMemoryNV ) \
	WGL( FreeMemoryNV ) \
	GL( DeleteFencesNV ) \
	GL( GenFencesNV ) \
	GL( IsFenceNV ) \
	GL( TestFenceNV ) \
	GL( GetFenceivNV ) \
	GL( FinishFenceNV ) \
	GL( SetFenceNV ) \
	GL( CombinerParameterfvNV ) \
	GL( CombinerParameterfNV ) \
	GL( CombinerParameterivNV ) \
	GL( CombinerParameteriNV ) \
	GL( CombinerInputNV ) \
	GL( CombinerOutputNV ) \
	GL( FinalCombinerInputNV ) \
	GL( GetCombinerInputParameterfvNV ) \
	GL( GetCombinerInputParameterivNV ) \
	GL( GetCombinerOutputParameterfvNV ) \
	GL( GetCombinerOutputParameterivNV ) \
	GL( GetFinalCombinerInputParameterfvNV ) \
	GL( GetFinalCombinerInputParameterivNV ) \
	GL( CombinerStageParameterfvNV ) \
	GL( GetCombinerStageParameterfvNV ) \
	GL( GenFragmentShadersATI ) \
	GL( BindFragmentShaderATI ) \
	GL( DeleteFragmentShaderATI ) \
	GL( BeginFragmentShaderATI ) \
	GL( EndFragmentShaderATI ) \
	GL( PassTexCoordATI ) \
	GL( SampleMapATI ) \
	GL( ColorFragmentOp1ATI ) \
	GL( ColorFragmentOp2ATI ) \
	GL( ColorFragmentOp3ATI ) \
	GL( AlphaFragmentOp1ATI ) \
	GL( AlphaFragmentOp2ATI ) \
	GL( AlphaFragmentOp3ATI ) \
	GL( SetFragmentShaderConstantATI ) \
	GL( VertexAttrib1sARB ) \
	GL( VertexAttrib1fARB ) \
	GL( VertexAttrib1dARB ) \
	GL( VertexAttrib2sARB ) \
	GL( VertexAttrib2fARB ) \
	GL( VertexAttrib2dARB ) \
	GL( VertexAttrib3sARB ) \
	GL( VertexAttrib3fARB ) \
	GL( VertexAttrib3dARB ) \
	GL( VertexAttrib4sARB ) \
	GL( VertexAttrib4fARB ) \
	GL( VertexAttrib4dARB ) \
	GL( VertexAttrib4NubARB ) \
	GL( VertexAttrib1svARB ) \
	GL( VertexAttrib1fvARB ) \
	GL( VertexAttrib1dvARB ) \
	GL( VertexAttrib2svARB ) \
	GL( VertexAttrib2fvARB ) \
	GL( VertexAttrib2dvARB ) \
	GL( VertexAttrib3svARB ) \
	GL( VertexAttrib3fvARB ) \
	GL( VertexAttrib3dvARB ) \
	GL( VertexAttrib4bvARB ) \
	GL( VertexAttrib4svARB ) \
	GL( VertexAttrib4ivARB ) \
	GL( VertexAttrib4ubvARB ) \
	GL( VertexAttrib4usvARB ) \
	GL( VertexAttrib4uivARB ) \
	GL( VertexAttrib4fvARB ) \
	GL( VertexAttrib4dvARB ) \
	GL( VertexAttrib4NbvARB ) \
	GL( VertexAttrib4NsvARB ) \
	GL( VertexAttrib4NivARB ) \
	GL( VertexAttrib4NubvARB ) \
	GL( VertexAttrib4NusvARB ) \
	GL( VertexAttrib4NuivARB ) \
	GL( VertexAttribPointerARB ) \
	GL( EnableVertexAttribArrayARB ) \
	GL( DisableVertexAttribArrayARB ) \
	GL( ProgramStringARB ) \
	GL( BindProgramARB ) \
	GL( DeleteProgramsARB ) \
	GL( GenProgramsARB ) \
	GL( ProgramEnvParameter4fARB ) \
	GL( ProgramEnvParameter4dARB ) \
	GL( ProgramEnvParameter4fvARB ) \
	GL( ProgramEnvParameter4dvARB ) \
	GL( ProgramLocalParameter4fARB ) \
	GL( ProgramLocalParameter4dARB ) \
	GL( ProgramLocalParameter4fvARB ) \
	GL( ProgramLocalParameter4dvARB ) \
	GL( GetProgramEnvParameterfvARB ) \
	GL( GetProgramEnvParameterdvARB ) \
	GL( GetProgramLocalParameterfvARB ) \
	GL( GetProgramLocalParameterdvARB ) \
	GL( GetProgramivARB ) \
	GL( GetProgramStringARB ) \
	GL( GetVertexAttribdvARB ) \
	GL( GetVertexAttribfvARB ) \
	GL( GetVertexAttribivARB ) \
	GL( GetVertexAttribPointervARB ) \
	GL( IsProgramARB )

#define QGL_LOG_TAIL_ENTRIES( GL, WGL ) \
	WGL( GetExtensionsStringEXT ) \
	WGL( SwapIntervalEXT ) \
	WGL( GetPixelFormatAttribivARB ) \
	WGL( GetPixelFormatAttribfvARB ) \
	WGL( ChoosePixelFormatARB ) \
	WGL( CreatePbufferARB ) \
	WGL( GetPbufferDCARB ) \
	WGL( ReleasePbufferDCARB ) \
	WGL( DestroyPbufferARB ) \
	WGL( QueryPbufferARB ) \
	WGL( BindTexImageARB ) \
	WGL( ReleaseTexImageARB ) \
	WGL( SetPbufferAttribARB )

#define QGL_EXTRA_NULL_GL( n )      dll##n    = NULL; qgl##n  = NULL;
#define QGL_EXTRA_NULL_WGL( n )     dllwgl##n = NULL; qwgl##n = NULL;

/* ---- QGL_Shutdown  0x004C6AF0 ---- */
void QGL_Shutdown( void ) {
	ri.Printf( QGL_PRINT_ALL, "...shutting down QGL\n" );

	if ( glw_state.hinstOpenGL ) {
		ri.Printf( QGL_PRINT_ALL, "...unloading OpenGL DLL\n" );
		FreeLibrary( glw_state.hinstOpenGL );
	}
	glw_state.hinstOpenGL = NULL;

	/* driver slot and qgl pointer, as retail clears both */
	QGL_LOG_EXT_ENTRIES( QGL_EXTRA_NULL_GL, QGL_EXTRA_NULL_WGL )
#define QGL_CORE_GL( n )    dll##n    = NULL; qgl##n  = NULL;
#define QGL_CORE_WGL( n )   dllwgl##n = NULL; qwgl##n = NULL;
#include "../renderer/qgl_entries.h"
	QGL_LOG_TAIL_ENTRIES( QGL_EXTRA_NULL_GL, QGL_EXTRA_NULL_WGL )
}

/* ---- QCL_Init  0x004C8300 ----  VERIFIED */
qboolean QCL_Init( const char *dllname ) {
	char	systemDir[1024];
	char	displayPath[1024];

	GetSystemDirectory( systemDir, sizeof( systemDir ) );

	ri.Printf( QGL_PRINT_ALL, "...initializing QGL\n" );

	if ( dllname[0] != '!' && strstr( "dllname", ".dll" ) == NULL ) {
		Com_sprintf( displayPath, sizeof( displayPath ), "%s\\%s",
					 systemDir, dllname );
	} else {
		strncpy( displayPath, dllname, sizeof( displayPath ) - 1 );
		displayPath[sizeof( displayPath ) - 1] = '\0';
	}

	ri.Printf( QGL_PRINT_ALL, "...calling LoadLibrary( '%s.dll' ): ",
			   displayPath );

	glw_state.hinstOpenGL = LoadLibrary( dllname );
	if ( !glw_state.hinstOpenGL ) {
		ri.Printf( QGL_PRINT_ALL, "failed\n" );
		return qfalse;
	}
	ri.Printf( QGL_PRINT_ALL, "succeeded\n" );

	QGL_LOG_EXT_ENTRIES( QGL_EXTRA_NULL_GL, QGL_EXTRA_NULL_WGL )

#define QGL_CORE_GL( n )    QGL_BIND_GL( n );
#define QGL_CORE_WGL( n )   QGL_BIND_WGL( n );
#include "../renderer/qgl_entries.h"

	QGL_LOG_TAIL_ENTRIES( QGL_EXTRA_NULL_GL, QGL_EXTRA_NULL_WGL )

	/* 0x004CABD1: arm the GL debug hooks from the cvars.  r_logFile->integer is
	   read once (esi) and feeds both calls; no draw-profiling call here. */
	{
		int logFile = r_logFile->integer;

		QGL_EnableErrorChecking( ( !logFile && r_debugGLErrors->integer ) ? qtrue : qfalse );
		QGL_EnableLogging( logFile );
	}

	/* Retail never tests the GetProcAddress results: once LoadLibrary has
	   succeeded it always returns 1. */
	return qtrue;
}

/*
===========================================================================

glGetError / GetLastError wrappers.  QGL_EnableErrorChecking points every
non-NULL qgl pointer at the matching wrapper below (retail's block runs
0x004C0090-0x004C6AB0).
Each one calls the driver slot, then reads the error state and prints it:
GL entry points through dllGetError, WGL ones through GetLastError.  The
result of a returning entry point is kept across the check and returned.
glGetError's own wrapper therefore calls the driver twice: it returns the
first result and reports the second.

===========================================================================
*/

#define QGL_CHECK_GL( name ) \
	do { \
		GLenum err_ = dllGetError(); \
		if ( err_ ) { \
			Com_Printf( "^3" name ": glGetError() = 0x%04x\n", err_ ); \
		} \
	} while ( 0 )

#define QGL_CHECK_WGL( name ) \
	do { \
		DWORD err_ = GetLastError(); \
		if ( err_ ) { \
			Com_Printf( "^3" name ": GetLastError() = 0x%04x\n", err_ ); \
		} \
	} while ( 0 )

/* ---- qglCheckedAccum  0x004C1F60 ---- */
static void QGLAPI qglCheckedAccum( GLenum op, GLfloat value ) {
	dllAccum( op, value );
	QGL_CHECK_GL( "glAccum" );
}

/* ---- qglCheckedAlphaFunc  0x004C1F90 ---- */
static void QGLAPI qglCheckedAlphaFunc( GLenum func, GLclampf ref ) {
	dllAlphaFunc( func, ref );
	QGL_CHECK_GL( "glAlphaFunc" );
}

/* ---- qglCheckedAreTexturesResident  0x004C1FC0 ---- */
static GLboolean QGLAPI qglCheckedAreTexturesResident( GLsizei n, const GLuint *textures, GLboolean *residences ) {
	GLboolean result = dllAreTexturesResident( n, textures, residences );
	QGL_CHECK_GL( "glAreTexturesResident" );
	return result;
}

/* ---- qglCheckedArrayElement  0x004C2000 ---- */
static void QGLAPI qglCheckedArrayElement( GLint i ) {
	dllArrayElement( i );
	QGL_CHECK_GL( "glArrayElement" );
}

/* ---- qglCheckedBegin  0x004C2030 ---- */
static void QGLAPI qglCheckedBegin( GLenum mode ) {
	dllBegin( mode );
	QGL_CHECK_GL( "glBegin" );
}

/* ---- qglCheckedBindTexture  0x004C2060 ---- */
static void QGLAPI qglCheckedBindTexture( GLenum target, GLuint texture ) {
	dllBindTexture( target, texture );
	QGL_CHECK_GL( "glBindTexture" );
}

/* ---- qglCheckedBitmap  0x004C2090 ---- */
static void QGLAPI qglCheckedBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ) {
	dllBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
	QGL_CHECK_GL( "glBitmap" );
}

/* ---- qglCheckedBlendFunc  0x004C20E0 ---- */
static void QGLAPI qglCheckedBlendFunc( GLenum sfactor, GLenum dfactor ) {
	dllBlendFunc( sfactor, dfactor );
	QGL_CHECK_GL( "glBlendFunc" );
}

/* ---- qglCheckedCallList  0x004C2110 ---- */
static void QGLAPI qglCheckedCallList( GLuint list ) {
	dllCallList( list );
	QGL_CHECK_GL( "glCallList" );
}

/* ---- qglCheckedCallLists  0x004C2140 ---- */
static void QGLAPI qglCheckedCallLists( GLsizei n, GLenum type, const GLvoid *lists ) {
	dllCallLists( n, type, lists );
	QGL_CHECK_GL( "glCallLists" );
}

/* ---- qglCheckedClear  0x004C2170 ---- */
static void QGLAPI qglCheckedClear( GLbitfield mask ) {
	dllClear( mask );
	QGL_CHECK_GL( "glClear" );
}

/* ---- qglCheckedClearAccum  0x004C21A0 ---- */
static void QGLAPI qglCheckedClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
	dllClearAccum( red, green, blue, alpha );
	QGL_CHECK_GL( "glClearAccum" );
}

/* ---- qglCheckedClearColor  0x004C21E0 ---- */
static void QGLAPI qglCheckedClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
	dllClearColor( red, green, blue, alpha );
	QGL_CHECK_GL( "glClearColor" );
}

/* ---- qglCheckedClearDepth  0x004C2220 ---- */
static void QGLAPI qglCheckedClearDepth( GLclampd depth ) {
	dllClearDepth( depth );
	QGL_CHECK_GL( "glClearDepth" );
}

/* ---- qglCheckedClearIndex  0x004C2250 ---- */
static void QGLAPI qglCheckedClearIndex( GLfloat c ) {
	dllClearIndex( c );
	QGL_CHECK_GL( "glClearIndex" );
}

/* ---- qglCheckedClearStencil  0x004C2280 ---- */
static void QGLAPI qglCheckedClearStencil( GLint s ) {
	dllClearStencil( s );
	QGL_CHECK_GL( "glClearStencil" );
}

/* ---- qglCheckedClipPlane  0x004C22B0 ---- */
static void QGLAPI qglCheckedClipPlane( GLenum plane, const GLdouble *equation ) {
	dllClipPlane( plane, equation );
	QGL_CHECK_GL( "glClipPlane" );
}

/* ---- qglCheckedColor3b  0x004C22E0 ---- */
static void QGLAPI qglCheckedColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
	dllColor3b( red, green, blue );
	QGL_CHECK_GL( "glColor3b" );
}

/* ---- qglCheckedColor3bv  0x004C2310 ---- */
static void QGLAPI qglCheckedColor3bv( const GLbyte *v ) {
	dllColor3bv( v );
	QGL_CHECK_GL( "glColor3bv" );
}

/* ---- qglCheckedColor3d  0x004C2340 ---- */
static void QGLAPI qglCheckedColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
	dllColor3d( red, green, blue );
	QGL_CHECK_GL( "glColor3d" );
}

/* ---- qglCheckedColor3dv  0x004C2380 ---- */
static void QGLAPI qglCheckedColor3dv( const GLdouble *v ) {
	dllColor3dv( v );
	QGL_CHECK_GL( "glColor3dv" );
}

/* ---- qglCheckedColor3f  0x004C23B0 ---- */
static void QGLAPI qglCheckedColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
	dllColor3f( red, green, blue );
	QGL_CHECK_GL( "glColor3f" );
}

/* ---- qglCheckedColor3fv  0x004C23E0 ---- */
static void QGLAPI qglCheckedColor3fv( const GLfloat *v ) {
	dllColor3fv( v );
	QGL_CHECK_GL( "glColor3fv" );
}

/* ---- qglCheckedColor3i  0x004C2410 ---- */
static void QGLAPI qglCheckedColor3i( GLint red, GLint green, GLint blue ) {
	dllColor3i( red, green, blue );
	QGL_CHECK_GL( "glColor3i" );
}

/* ---- qglCheckedColor3iv  0x004C2440 ---- */
static void QGLAPI qglCheckedColor3iv( const GLint *v ) {
	dllColor3iv( v );
	QGL_CHECK_GL( "glColor3iv" );
}

/* ---- qglCheckedColor3s  0x004C2470 ---- */
static void QGLAPI qglCheckedColor3s( GLshort red, GLshort green, GLshort blue ) {
	dllColor3s( red, green, blue );
	QGL_CHECK_GL( "glColor3s" );
}

/* ---- qglCheckedColor3sv  0x004C24A0 ---- */
static void QGLAPI qglCheckedColor3sv( const GLshort *v ) {
	dllColor3sv( v );
	QGL_CHECK_GL( "glColor3sv" );
}

/* ---- qglCheckedColor3ub  0x004C24D0 ---- */
static void QGLAPI qglCheckedColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
	dllColor3ub( red, green, blue );
	QGL_CHECK_GL( "glColor3ub" );
}

/* ---- qglCheckedColor3ubv  0x004C2500 ---- */
static void QGLAPI qglCheckedColor3ubv( const GLubyte *v ) {
	dllColor3ubv( v );
	QGL_CHECK_GL( "glColor3ubv" );
}

/* ---- qglCheckedColor3ui  0x004C2530 ---- */
static void QGLAPI qglCheckedColor3ui( GLuint red, GLuint green, GLuint blue ) {
	dllColor3ui( red, green, blue );
	QGL_CHECK_GL( "glColor3ui" );
}

/* ---- qglCheckedColor3uiv  0x004C2560 ---- */
static void QGLAPI qglCheckedColor3uiv( const GLuint *v ) {
	dllColor3uiv( v );
	QGL_CHECK_GL( "glColor3uiv" );
}

/* ---- qglCheckedColor3us  0x004C2590 ---- */
static void QGLAPI qglCheckedColor3us( GLushort red, GLushort green, GLushort blue ) {
	dllColor3us( red, green, blue );
	QGL_CHECK_GL( "glColor3us" );
}

/* ---- qglCheckedColor3usv  0x004C25C0 ---- */
static void QGLAPI qglCheckedColor3usv( const GLushort *v ) {
	dllColor3usv( v );
	QGL_CHECK_GL( "glColor3usv" );
}

/* ---- qglCheckedColor4b  0x004C25F0 ---- */
static void QGLAPI qglCheckedColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
	dllColor4b( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4b" );
}

/* ---- qglCheckedColor4bv  0x004C2630 ---- */
static void QGLAPI qglCheckedColor4bv( const GLbyte *v ) {
	dllColor4bv( v );
	QGL_CHECK_GL( "glColor4bv" );
}

/* ---- qglCheckedColor4d  0x004C2660 ---- */
static void QGLAPI qglCheckedColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
	dllColor4d( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4d" );
}

/* ---- qglCheckedColor4dv  0x004C26B0 ---- */
static void QGLAPI qglCheckedColor4dv( const GLdouble *v ) {
	dllColor4dv( v );
	QGL_CHECK_GL( "glColor4dv" );
}

/* ---- qglCheckedColor4f  0x004C26E0 ---- */
static void QGLAPI qglCheckedColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
	dllColor4f( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4f" );
}

/* ---- qglCheckedColor4fv  0x004C2720 ---- */
static void QGLAPI qglCheckedColor4fv( const GLfloat *v ) {
	dllColor4fv( v );
	QGL_CHECK_GL( "glColor4fv" );
}

/* ---- qglCheckedColor4i  0x004C2750 ---- */
static void QGLAPI qglCheckedColor4i( GLint red, GLint green, GLint blue, GLint alpha ) {
	dllColor4i( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4i" );
}

/* ---- qglCheckedColor4iv  0x004C2790 ---- */
static void QGLAPI qglCheckedColor4iv( const GLint *v ) {
	dllColor4iv( v );
	QGL_CHECK_GL( "glColor4iv" );
}

/* ---- qglCheckedColor4s  0x004C27C0 ---- */
static void QGLAPI qglCheckedColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
	dllColor4s( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4s" );
}

/* ---- qglCheckedColor4sv  0x004C2800 ---- */
static void QGLAPI qglCheckedColor4sv( const GLshort *v ) {
	dllColor4sv( v );
	QGL_CHECK_GL( "glColor4sv" );
}

/* ---- qglCheckedColor4ub  0x004C2830 ---- */
static void QGLAPI qglCheckedColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
	dllColor4ub( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4ub" );
}

/* ---- qglCheckedColor4ubv  0x004C2870 ---- */
static void QGLAPI qglCheckedColor4ubv( const GLubyte *v ) {
	dllColor4ubv( v );
	QGL_CHECK_GL( "glColor4ubv" );
}

/* ---- qglCheckedColor4ui  0x004C28A0 ---- */
static void QGLAPI qglCheckedColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
	dllColor4ui( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4ui" );
}

/* ---- qglCheckedColor4uiv  0x004C28E0 ---- */
static void QGLAPI qglCheckedColor4uiv( const GLuint *v ) {
	dllColor4uiv( v );
	QGL_CHECK_GL( "glColor4uiv" );
}

/* ---- qglCheckedColor4us  0x004C2910 ---- */
static void QGLAPI qglCheckedColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
	dllColor4us( red, green, blue, alpha );
	QGL_CHECK_GL( "glColor4us" );
}

/* ---- qglCheckedColor4usv  0x004C2950 ---- */
static void QGLAPI qglCheckedColor4usv( const GLushort *v ) {
	dllColor4usv( v );
	QGL_CHECK_GL( "glColor4usv" );
}

/* ---- qglCheckedColorMask  0x004C2980 ---- */
static void QGLAPI qglCheckedColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
	dllColorMask( red, green, blue, alpha );
	QGL_CHECK_GL( "glColorMask" );
}

/* ---- qglCheckedColorMaterial  0x004C29C0 ---- */
static void QGLAPI qglCheckedColorMaterial( GLenum face, GLenum mode ) {
	dllColorMaterial( face, mode );
	QGL_CHECK_GL( "glColorMaterial" );
}

/* ---- qglCheckedColorPointer  0x004C29F0 ---- */
static void QGLAPI qglCheckedColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	dllColorPointer( size, type, stride, pointer );
	QGL_CHECK_GL( "glColorPointer" );
}

/* ---- qglCheckedCopyPixels  0x004C2A30 ---- */
static void QGLAPI qglCheckedCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
	dllCopyPixels( x, y, width, height, type );
	QGL_CHECK_GL( "glCopyPixels" );
}

/* ---- qglCheckedCopyTexImage1D  0x004C2A70 ---- */
static void QGLAPI qglCheckedCopyTexImage1D( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border ) {
	dllCopyTexImage1D( target, level, internalFormat, x, y, width, border );
	QGL_CHECK_GL( "glCopyTexImage1D" );
}

/* ---- qglCheckedCopyTexImage2D  0x004C2AC0 ---- */
static void QGLAPI qglCheckedCopyTexImage2D( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
	dllCopyTexImage2D( target, level, internalFormat, x, y, width, height, border );
	QGL_CHECK_GL( "glCopyTexImage2D" );
}

/* ---- qglCheckedCopyTexSubImage1D  0x004C2B10 ---- */
static void QGLAPI qglCheckedCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
	dllCopyTexSubImage1D( target, level, xoffset, x, y, width );
	QGL_CHECK_GL( "glCopyTexSubImage1D" );
}

/* ---- qglCheckedCopyTexSubImage2D  0x004C2B50 ---- */
static void QGLAPI qglCheckedCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
	dllCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
	QGL_CHECK_GL( "glCopyTexSubImage2D" );
}

/* ---- qglCheckedCullFace  0x004C2BA0 ---- */
static void QGLAPI qglCheckedCullFace( GLenum mode ) {
	dllCullFace( mode );
	QGL_CHECK_GL( "glCullFace" );
}

/* ---- qglCheckedDeleteLists  0x004C2BD0 ---- */
static void QGLAPI qglCheckedDeleteLists( GLuint list, GLsizei range ) {
	dllDeleteLists( list, range );
	QGL_CHECK_GL( "glDeleteLists" );
}

/* ---- qglCheckedDeleteTextures  0x004C2C00 ---- */
static void QGLAPI qglCheckedDeleteTextures( GLsizei n, const GLuint *textures ) {
	dllDeleteTextures( n, textures );
	QGL_CHECK_GL( "glDeleteTextures" );
}

/* ---- qglCheckedDepthFunc  0x004C2C30 ---- */
static void QGLAPI qglCheckedDepthFunc( GLenum func ) {
	dllDepthFunc( func );
	QGL_CHECK_GL( "glDepthFunc" );
}

/* ---- qglCheckedDepthMask  0x004C2C60 ---- */
static void QGLAPI qglCheckedDepthMask( GLboolean flag ) {
	dllDepthMask( flag );
	QGL_CHECK_GL( "glDepthMask" );
}

/* ---- qglCheckedDepthRange  0x004C2C90 ---- */
static void QGLAPI qglCheckedDepthRange( GLclampd zNear, GLclampd zFar ) {
	dllDepthRange( zNear, zFar );
	QGL_CHECK_GL( "glDepthRange" );
}

/* ---- qglCheckedDisable  0x004C2CD0 ---- */
static void QGLAPI qglCheckedDisable( GLenum cap ) {
	dllDisable( cap );
	QGL_CHECK_GL( "glDisable" );
}

/* ---- qglCheckedDisableClientState  0x004C2D00 ---- */
static void QGLAPI qglCheckedDisableClientState( GLenum array ) {
	dllDisableClientState( array );
	QGL_CHECK_GL( "glDisableClientState" );
}

/* ---- qglCheckedDrawArrays  0x004C2D30 ---- */
static void QGLAPI qglCheckedDrawArrays( GLenum mode, GLint first, GLsizei count ) {
	dllDrawArrays( mode, first, count );
	QGL_CHECK_GL( "glDrawArrays" );
}

/* ---- qglCheckedDrawBuffer  0x004C2D60 ---- */
static void QGLAPI qglCheckedDrawBuffer( GLenum mode ) {
	dllDrawBuffer( mode );
	QGL_CHECK_GL( "glDrawBuffer" );
}

/* ---- qglCheckedDrawElements  0x004C2D90 ---- */
static void QGLAPI qglCheckedDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
	dllDrawElements( mode, count, type, indices );
	QGL_CHECK_GL( "glDrawElements" );
}

/* ---- qglCheckedDrawPixels  0x004C2DD0 ---- */
static void QGLAPI qglCheckedDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
	dllDrawPixels( width, height, format, type, pixels );
	QGL_CHECK_GL( "glDrawPixels" );
}

/* ---- qglCheckedEdgeFlag  0x004C2E10 ---- */
static void QGLAPI qglCheckedEdgeFlag( GLboolean flag ) {
	dllEdgeFlag( flag );
	QGL_CHECK_GL( "glEdgeFlag" );
}

/* ---- qglCheckedEdgeFlagPointer  0x004C2E40 ---- */
static void QGLAPI qglCheckedEdgeFlagPointer( GLsizei stride, const GLvoid *pointer ) {
	dllEdgeFlagPointer( stride, pointer );
	QGL_CHECK_GL( "glEdgeFlagPointer" );
}

/* ---- qglCheckedEdgeFlagv  0x004C2E70 ---- */
static void QGLAPI qglCheckedEdgeFlagv( const GLboolean *flag ) {
	dllEdgeFlagv( flag );
	QGL_CHECK_GL( "glEdgeFlagv" );
}

/* ---- qglCheckedEnable  0x004C2EA0 ---- */
static void QGLAPI qglCheckedEnable( GLenum cap ) {
	dllEnable( cap );
	QGL_CHECK_GL( "glEnable" );
}

/* ---- qglCheckedEnableClientState  0x004C2ED0 ---- */
static void QGLAPI qglCheckedEnableClientState( GLenum array ) {
	dllEnableClientState( array );
	QGL_CHECK_GL( "glEnableClientState" );
}

/* ---- qglCheckedEnd  0x004C2F00 ---- */
static void QGLAPI qglCheckedEnd( void ) {
	dllEnd();
	QGL_CHECK_GL( "glEnd" );
}

/* ---- qglCheckedEndList  0x004C2F20 ---- */
static void QGLAPI qglCheckedEndList( void ) {
	dllEndList();
	QGL_CHECK_GL( "glEndList" );
}

/* ---- qglCheckedEvalCoord1d  0x004C2F40 ---- */
static void QGLAPI qglCheckedEvalCoord1d( GLdouble u ) {
	dllEvalCoord1d( u );
	QGL_CHECK_GL( "glEvalCoord1d" );
}

/* ---- qglCheckedEvalCoord1dv  0x004C2F70 ---- */
static void QGLAPI qglCheckedEvalCoord1dv( const GLdouble *u ) {
	dllEvalCoord1dv( u );
	QGL_CHECK_GL( "glEvalCoord1dv" );
}

/* ---- qglCheckedEvalCoord1f  0x004C2FA0 ---- */
static void QGLAPI qglCheckedEvalCoord1f( GLfloat u ) {
	dllEvalCoord1f( u );
	QGL_CHECK_GL( "glEvalCoord1f" );
}

/* ---- qglCheckedEvalCoord1fv  0x004C2FD0 ---- */
static void QGLAPI qglCheckedEvalCoord1fv( const GLfloat *u ) {
	dllEvalCoord1fv( u );
	QGL_CHECK_GL( "glEvalCoord1fv" );
}

/* ---- qglCheckedEvalCoord2d  0x004C3000 ---- */
static void QGLAPI qglCheckedEvalCoord2d( GLdouble u, GLdouble v ) {
	dllEvalCoord2d( u, v );
	QGL_CHECK_GL( "glEvalCoord2d" );
}

/* ---- qglCheckedEvalCoord2dv  0x004C3040 ---- */
static void QGLAPI qglCheckedEvalCoord2dv( const GLdouble *u ) {
	dllEvalCoord2dv( u );
	QGL_CHECK_GL( "glEvalCoord2dv" );
}

/* ---- qglCheckedEvalCoord2f  0x004C3070 ---- */
static void QGLAPI qglCheckedEvalCoord2f( GLfloat u, GLfloat v ) {
	dllEvalCoord2f( u, v );
	QGL_CHECK_GL( "glEvalCoord2f" );
}

/* ---- qglCheckedEvalCoord2fv  0x004C30A0 ---- */
static void QGLAPI qglCheckedEvalCoord2fv( const GLfloat *u ) {
	dllEvalCoord2fv( u );
	QGL_CHECK_GL( "glEvalCoord2fv" );
}

/* ---- qglCheckedEvalMesh1  0x004C30D0 ---- */
static void QGLAPI qglCheckedEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
	dllEvalMesh1( mode, i1, i2 );
	QGL_CHECK_GL( "glEvalMesh1" );
}

/* ---- qglCheckedEvalMesh2  0x004C3100 ---- */
static void QGLAPI qglCheckedEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
	dllEvalMesh2( mode, i1, i2, j1, j2 );
	QGL_CHECK_GL( "glEvalMesh2" );
}

/* ---- qglCheckedEvalPoint1  0x004C3140 ---- */
static void QGLAPI qglCheckedEvalPoint1( GLint i ) {
	dllEvalPoint1( i );
	QGL_CHECK_GL( "glEvalPoint1" );
}

/* ---- qglCheckedEvalPoint2  0x004C3170 ---- */
static void QGLAPI qglCheckedEvalPoint2( GLint i, GLint j ) {
	dllEvalPoint2( i, j );
	QGL_CHECK_GL( "glEvalPoint2" );
}

/* ---- qglCheckedFeedbackBuffer  0x004C31A0 ---- */
static void QGLAPI qglCheckedFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer ) {
	dllFeedbackBuffer( size, type, buffer );
	QGL_CHECK_GL( "glFeedbackBuffer" );
}

/* ---- qglCheckedFinish  0x004C31D0 ---- */
static void QGLAPI qglCheckedFinish( void ) {
	dllFinish();
	QGL_CHECK_GL( "glFinish" );
}

/* ---- qglCheckedFlush  0x004C31F0 ---- */
static void QGLAPI qglCheckedFlush( void ) {
	dllFlush();
	QGL_CHECK_GL( "glFlush" );
}

/* ---- qglCheckedFogf  0x004C3210 ---- */
static void QGLAPI qglCheckedFogf( GLenum pname, GLfloat param ) {
	dllFogf( pname, param );
	QGL_CHECK_GL( "glFogf" );
}

/* ---- qglCheckedFogfv  0x004C3240 ---- */
static void QGLAPI qglCheckedFogfv( GLenum pname, const GLfloat *params ) {
	dllFogfv( pname, params );
	QGL_CHECK_GL( "glFogfv" );
}

/* ---- qglCheckedFogi  0x004C3270 ---- */
static void QGLAPI qglCheckedFogi( GLenum pname, GLint param ) {
	dllFogi( pname, param );
	QGL_CHECK_GL( "glFogi" );
}

/* ---- qglCheckedFogiv  0x004C32A0 ---- */
static void QGLAPI qglCheckedFogiv( GLenum pname, const GLint *params ) {
	dllFogiv( pname, params );
	QGL_CHECK_GL( "glFogiv" );
}

/* ---- qglCheckedFrontFace  0x004C32D0 ---- */
static void QGLAPI qglCheckedFrontFace( GLenum mode ) {
	dllFrontFace( mode );
	QGL_CHECK_GL( "glFrontFace" );
}

/* ---- qglCheckedFrustum  0x004C3300 ---- */
static void QGLAPI qglCheckedFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	dllFrustum( left, right, bottom, top, zNear, zFar );
	QGL_CHECK_GL( "glFrustum" );
}

/* ---- qglCheckedGenLists  0x004C3360 ---- */
static GLuint QGLAPI qglCheckedGenLists( GLsizei range ) {
	GLuint result = dllGenLists( range );
	QGL_CHECK_GL( "glGenLists" );
	return result;
}

/* ---- qglCheckedGenTextures  0x004C3390 ---- */
static void QGLAPI qglCheckedGenTextures( GLsizei n, GLuint *textures ) {
	dllGenTextures( n, textures );
	QGL_CHECK_GL( "glGenTextures" );
}

/* ---- qglCheckedGetBooleanv  0x004C33C0 ---- */
static void QGLAPI qglCheckedGetBooleanv( GLenum pname, GLboolean *params ) {
	dllGetBooleanv( pname, params );
	QGL_CHECK_GL( "glGetBooleanv" );
}

/* ---- qglCheckedGetClipPlane  0x004C33F0 ---- */
static void QGLAPI qglCheckedGetClipPlane( GLenum plane, GLdouble *equation ) {
	dllGetClipPlane( plane, equation );
	QGL_CHECK_GL( "glGetClipPlane" );
}

/* ---- qglCheckedGetDoublev  0x004C3420 ---- */
static void QGLAPI qglCheckedGetDoublev( GLenum pname, GLdouble *params ) {
	dllGetDoublev( pname, params );
	QGL_CHECK_GL( "glGetDoublev" );
}

/* ---- qglCheckedGetError  0x004C3450 ---- */
static GLenum QGLAPI qglCheckedGetError( void ) {
	GLenum result = dllGetError();
	QGL_CHECK_GL( "glGetError" );
	return result;
}

/* ---- qglCheckedGetFloatv  0x004C3480 ---- */
static void QGLAPI qglCheckedGetFloatv( GLenum pname, GLfloat *params ) {
	dllGetFloatv( pname, params );
	QGL_CHECK_GL( "glGetFloatv" );
}

/* ---- qglCheckedGetIntegerv  0x004C34B0 ---- */
static void QGLAPI qglCheckedGetIntegerv( GLenum pname, GLint *params ) {
	dllGetIntegerv( pname, params );
	QGL_CHECK_GL( "glGetIntegerv" );
}

/* ---- qglCheckedGetLightfv  0x004C34E0 ---- */
static void QGLAPI qglCheckedGetLightfv( GLenum light, GLenum pname, GLfloat *params ) {
	dllGetLightfv( light, pname, params );
	QGL_CHECK_GL( "glGetLightfv" );
}

/* ---- qglCheckedGetLightiv  0x004C3510 ---- */
static void QGLAPI qglCheckedGetLightiv( GLenum light, GLenum pname, GLint *params ) {
	dllGetLightiv( light, pname, params );
	QGL_CHECK_GL( "glGetLightiv" );
}

/* ---- qglCheckedGetMapdv  0x004C3540 ---- */
static void QGLAPI qglCheckedGetMapdv( GLenum target, GLenum query, GLdouble *v ) {
	dllGetMapdv( target, query, v );
	QGL_CHECK_GL( "glGetMapdv" );
}

/* ---- qglCheckedGetMapfv  0x004C3570 ---- */
static void QGLAPI qglCheckedGetMapfv( GLenum target, GLenum query, GLfloat *v ) {
	dllGetMapfv( target, query, v );
	QGL_CHECK_GL( "glGetMapfv" );
}

/* ---- qglCheckedGetMapiv  0x004C35A0 ---- */
static void QGLAPI qglCheckedGetMapiv( GLenum target, GLenum query, GLint *v ) {
	dllGetMapiv( target, query, v );
	QGL_CHECK_GL( "glGetMapiv" );
}

/* ---- qglCheckedGetMaterialfv  0x004C35D0 ---- */
static void QGLAPI qglCheckedGetMaterialfv( GLenum face, GLenum pname, GLfloat *params ) {
	dllGetMaterialfv( face, pname, params );
	QGL_CHECK_GL( "glGetMaterialfv" );
}

/* ---- qglCheckedGetMaterialiv  0x004C3600 ---- */
static void QGLAPI qglCheckedGetMaterialiv( GLenum face, GLenum pname, GLint *params ) {
	dllGetMaterialiv( face, pname, params );
	QGL_CHECK_GL( "glGetMaterialiv" );
}

/* ---- qglCheckedGetPixelMapfv  0x004C3630 ---- */
static void QGLAPI qglCheckedGetPixelMapfv( GLenum map, GLfloat *values ) {
	dllGetPixelMapfv( map, values );
	QGL_CHECK_GL( "glGetPixelMapfv" );
}

/* ---- qglCheckedGetPixelMapuiv  0x004C3660 ---- */
static void QGLAPI qglCheckedGetPixelMapuiv( GLenum map, GLuint *values ) {
	dllGetPixelMapuiv( map, values );
	QGL_CHECK_GL( "glGetPixelMapuiv" );
}

/* ---- qglCheckedGetPixelMapusv  0x004C3690 ---- */
static void QGLAPI qglCheckedGetPixelMapusv( GLenum map, GLushort *values ) {
	dllGetPixelMapusv( map, values );
	QGL_CHECK_GL( "glGetPixelMapusv" );
}

/* ---- qglCheckedGetPointerv  0x004C36C0 ---- */
static void QGLAPI qglCheckedGetPointerv( GLenum pname, GLvoid* *params ) {
	dllGetPointerv( pname, params );
	QGL_CHECK_GL( "glGetPointerv" );
}

/* ---- qglCheckedGetPolygonStipple  0x004C36F0 ---- */
static void QGLAPI qglCheckedGetPolygonStipple( GLubyte *mask ) {
	dllGetPolygonStipple( mask );
	QGL_CHECK_GL( "glGetPolygonStipple" );
}

/* ---- qglCheckedGetString  0x004C3720 ---- */
static const GLubyte * QGLAPI qglCheckedGetString( GLenum name ) {
	const GLubyte *result = dllGetString( name );
	QGL_CHECK_GL( "glGetString" );
	return result;
}

/* ---- qglCheckedGetTexEnvfv  0x004C3750 ---- */
static void QGLAPI qglCheckedGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params ) {
	dllGetTexEnvfv( target, pname, params );
	QGL_CHECK_GL( "glGetTexEnvfv" );
}

/* ---- qglCheckedGetTexEnviv  0x004C3780 ---- */
static void QGLAPI qglCheckedGetTexEnviv( GLenum target, GLenum pname, GLint *params ) {
	dllGetTexEnviv( target, pname, params );
	QGL_CHECK_GL( "glGetTexEnviv" );
}

/* ---- qglCheckedGetTexGendv  0x004C37B0 ---- */
static void QGLAPI qglCheckedGetTexGendv( GLenum coord, GLenum pname, GLdouble *params ) {
	dllGetTexGendv( coord, pname, params );
	QGL_CHECK_GL( "glGetTexGendv" );
}

/* ---- qglCheckedGetTexGenfv  0x004C37E0 ---- */
static void QGLAPI qglCheckedGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params ) {
	dllGetTexGenfv( coord, pname, params );
	QGL_CHECK_GL( "glGetTexGenfv" );
}

/* ---- qglCheckedGetTexGeniv  0x004C3810 ---- */
static void QGLAPI qglCheckedGetTexGeniv( GLenum coord, GLenum pname, GLint *params ) {
	dllGetTexGeniv( coord, pname, params );
	QGL_CHECK_GL( "glGetTexGeniv" );
}

/* ---- qglCheckedGetTexImage  0x004C3840 ---- */
static void QGLAPI qglCheckedGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels ) {
	dllGetTexImage( target, level, format, type, pixels );
	QGL_CHECK_GL( "glGetTexImage" );
}

/* ---- qglCheckedGetTexLevelParameterfv  0x004C3880 ---- */
static void QGLAPI qglCheckedGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat *params ) {
	dllGetTexLevelParameterfv( target, level, pname, params );
	QGL_CHECK_GL( "glGetTexLevelParameterfv" );
}

/* ---- qglCheckedGetTexLevelParameteriv  0x004C38C0 ---- */
static void QGLAPI qglCheckedGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint *params ) {
	dllGetTexLevelParameteriv( target, level, pname, params );
	QGL_CHECK_GL( "glGetTexLevelParameteriv" );
}

/* ---- qglCheckedGetTexParameterfv  0x004C3900 ---- */
static void QGLAPI qglCheckedGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params ) {
	dllGetTexParameterfv( target, pname, params );
	QGL_CHECK_GL( "glGetTexParameterfv" );
}

/* ---- qglCheckedGetTexParameteriv  0x004C3930 ---- */
static void QGLAPI qglCheckedGetTexParameteriv( GLenum target, GLenum pname, GLint *params ) {
	dllGetTexParameteriv( target, pname, params );
	QGL_CHECK_GL( "glGetTexParameteriv" );
}

/* ---- qglCheckedHint  0x004C3960 ---- */
static void QGLAPI qglCheckedHint( GLenum target, GLenum mode ) {
	dllHint( target, mode );
	QGL_CHECK_GL( "glHint" );
}

/* ---- qglCheckedIndexMask  0x004C3990 ---- */
static void QGLAPI qglCheckedIndexMask( GLuint mask ) {
	dllIndexMask( mask );
	QGL_CHECK_GL( "glIndexMask" );
}

/* ---- qglCheckedIndexPointer  0x004C39C0 ---- */
static void QGLAPI qglCheckedIndexPointer( GLenum type, GLsizei stride, const GLvoid *pointer ) {
	dllIndexPointer( type, stride, pointer );
	QGL_CHECK_GL( "glIndexPointer" );
}

/* ---- qglCheckedIndexd  0x004C39F0 ---- */
static void QGLAPI qglCheckedIndexd( GLdouble c ) {
	dllIndexd( c );
	QGL_CHECK_GL( "glIndexd" );
}

/* ---- qglCheckedIndexdv  0x004C3A20 ---- */
static void QGLAPI qglCheckedIndexdv( const GLdouble *c ) {
	dllIndexdv( c );
	QGL_CHECK_GL( "glIndexdv" );
}

/* ---- qglCheckedIndexf  0x004C3A50 ---- */
static void QGLAPI qglCheckedIndexf( GLfloat c ) {
	dllIndexf( c );
	QGL_CHECK_GL( "glIndexf" );
}

/* ---- qglCheckedIndexfv  0x004C3A80 ---- */
static void QGLAPI qglCheckedIndexfv( const GLfloat *c ) {
	dllIndexfv( c );
	QGL_CHECK_GL( "glIndexfv" );
}

/* ---- qglCheckedIndexi  0x004C3AB0 ---- */
static void QGLAPI qglCheckedIndexi( GLint c ) {
	dllIndexi( c );
	QGL_CHECK_GL( "glIndexi" );
}

/* ---- qglCheckedIndexiv  0x004C3AE0 ---- */
static void QGLAPI qglCheckedIndexiv( const GLint *c ) {
	dllIndexiv( c );
	QGL_CHECK_GL( "glIndexiv" );
}

/* ---- qglCheckedIndexs  0x004C3B10 ---- */
static void QGLAPI qglCheckedIndexs( GLshort c ) {
	dllIndexs( c );
	QGL_CHECK_GL( "glIndexs" );
}

/* ---- qglCheckedIndexsv  0x004C3B40 ---- */
static void QGLAPI qglCheckedIndexsv( const GLshort *c ) {
	dllIndexsv( c );
	QGL_CHECK_GL( "glIndexsv" );
}

/* ---- qglCheckedIndexub  0x004C3B70 ---- */
static void QGLAPI qglCheckedIndexub( GLubyte c ) {
	dllIndexub( c );
	QGL_CHECK_GL( "glIndexub" );
}

/* ---- qglCheckedIndexubv  0x004C3BA0 ---- */
static void QGLAPI qglCheckedIndexubv( const GLubyte *c ) {
	dllIndexubv( c );
	QGL_CHECK_GL( "glIndexubv" );
}

/* ---- qglCheckedInitNames  0x004C3BD0 ---- */
static void QGLAPI qglCheckedInitNames( void ) {
	dllInitNames();
	QGL_CHECK_GL( "glInitNames" );
}

/* ---- qglCheckedInterleavedArrays  0x004C3BF0 ---- */
static void QGLAPI qglCheckedInterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer ) {
	dllInterleavedArrays( format, stride, pointer );
	QGL_CHECK_GL( "glInterleavedArrays" );
}

/* ---- qglCheckedIsEnabled  0x004C3C20 ---- */
static GLboolean QGLAPI qglCheckedIsEnabled( GLenum cap ) {
	GLboolean result = dllIsEnabled( cap );
	QGL_CHECK_GL( "glIsEnabled" );
	return result;
}

/* ---- qglCheckedIsList  0x004C3C50 ---- */
static GLboolean QGLAPI qglCheckedIsList( GLuint list ) {
	GLboolean result = dllIsList( list );
	QGL_CHECK_GL( "glIsList" );
	return result;
}

/* ---- qglCheckedIsTexture  0x004C3C80 ---- */
static GLboolean QGLAPI qglCheckedIsTexture( GLuint texture ) {
	GLboolean result = dllIsTexture( texture );
	QGL_CHECK_GL( "glIsTexture" );
	return result;
}

/* ---- qglCheckedLightModelf  0x004C3CB0 ---- */
static void QGLAPI qglCheckedLightModelf( GLenum pname, GLfloat param ) {
	dllLightModelf( pname, param );
	QGL_CHECK_GL( "glLightModelf" );
}

/* ---- qglCheckedLightModelfv  0x004C3CE0 ---- */
static void QGLAPI qglCheckedLightModelfv( GLenum pname, const GLfloat *params ) {
	dllLightModelfv( pname, params );
	QGL_CHECK_GL( "glLightModelfv" );
}

/* ---- qglCheckedLightModeli  0x004C3D10 ---- */
static void QGLAPI qglCheckedLightModeli( GLenum pname, GLint param ) {
	dllLightModeli( pname, param );
	QGL_CHECK_GL( "glLightModeli" );
}

/* ---- qglCheckedLightModeliv  0x004C3D40 ---- */
static void QGLAPI qglCheckedLightModeliv( GLenum pname, const GLint *params ) {
	dllLightModeliv( pname, params );
	QGL_CHECK_GL( "glLightModeliv" );
}

/* ---- qglCheckedLightf  0x004C3D70 ---- */
static void QGLAPI qglCheckedLightf( GLenum light, GLenum pname, GLfloat param ) {
	dllLightf( light, pname, param );
	QGL_CHECK_GL( "glLightf" );
}

/* ---- qglCheckedLightfv  0x004C3DA0 ---- */
static void QGLAPI qglCheckedLightfv( GLenum light, GLenum pname, const GLfloat *params ) {
	dllLightfv( light, pname, params );
	QGL_CHECK_GL( "glLightfv" );
}

/* ---- qglCheckedLighti  0x004C3DD0 ---- */
static void QGLAPI qglCheckedLighti( GLenum light, GLenum pname, GLint param ) {
	dllLighti( light, pname, param );
	QGL_CHECK_GL( "glLighti" );
}

/* ---- qglCheckedLightiv  0x004C3E00 ---- */
static void QGLAPI qglCheckedLightiv( GLenum light, GLenum pname, const GLint *params ) {
	dllLightiv( light, pname, params );
	QGL_CHECK_GL( "glLightiv" );
}

/* ---- qglCheckedLineStipple  0x004C3E30 ---- */
static void QGLAPI qglCheckedLineStipple( GLint factor, GLushort pattern ) {
	dllLineStipple( factor, pattern );
	QGL_CHECK_GL( "glLineStipple" );
}

/* ---- qglCheckedLineWidth  0x004C3E60 ---- */
static void QGLAPI qglCheckedLineWidth( GLfloat width ) {
	dllLineWidth( width );
	QGL_CHECK_GL( "glLineWidth" );
}

/* ---- qglCheckedListBase  0x004C3E90 ---- */
static void QGLAPI qglCheckedListBase( GLuint base ) {
	dllListBase( base );
	QGL_CHECK_GL( "glListBase" );
}

/* ---- qglCheckedLoadIdentity  0x004C3EC0 ---- */
static void QGLAPI qglCheckedLoadIdentity( void ) {
	dllLoadIdentity();
	QGL_CHECK_GL( "glLoadIdentity" );
}

/* ---- qglCheckedLoadMatrixd  0x004C3EE0 ---- */
static void QGLAPI qglCheckedLoadMatrixd( const GLdouble *m ) {
	dllLoadMatrixd( m );
	QGL_CHECK_GL( "glLoadMatrixd" );
}

/* ---- qglCheckedLoadMatrixf  0x004C3F10 ---- */
static void QGLAPI qglCheckedLoadMatrixf( const GLfloat *m ) {
	dllLoadMatrixf( m );
	QGL_CHECK_GL( "glLoadMatrixf" );
}

/* ---- qglCheckedLoadName  0x004C3F40 ---- */
static void QGLAPI qglCheckedLoadName( GLuint name ) {
	dllLoadName( name );
	QGL_CHECK_GL( "glLoadName" );
}

/* ---- qglCheckedLogicOp  0x004C3F70 ---- */
static void QGLAPI qglCheckedLogicOp( GLenum opcode ) {
	dllLogicOp( opcode );
	QGL_CHECK_GL( "glLogicOp" );
}

/* ---- qglCheckedMap1d  0x004C3FA0 ---- */
static void QGLAPI qglCheckedMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) {
	dllMap1d( target, u1, u2, stride, order, points );
	QGL_CHECK_GL( "glMap1d" );
}

/* ---- qglCheckedMap1f  0x004C3FF0 ---- */
static void QGLAPI qglCheckedMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) {
	dllMap1f( target, u1, u2, stride, order, points );
	QGL_CHECK_GL( "glMap1f" );
}

/* ---- qglCheckedMap2d  0x004C4030 ---- */
static void QGLAPI qglCheckedMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ) {
	dllMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
	QGL_CHECK_GL( "glMap2d" );
}

/* ---- qglCheckedMap2f  0x004C40A0 ---- */
static void QGLAPI qglCheckedMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) {
	dllMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
	QGL_CHECK_GL( "glMap2f" );
}

/* ---- qglCheckedMapGrid1d  0x004C4100 ---- */
static void QGLAPI qglCheckedMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) {
	dllMapGrid1d( un, u1, u2 );
	QGL_CHECK_GL( "glMapGrid1d" );
}

/* ---- qglCheckedMapGrid1f  0x004C4140 ---- */
static void QGLAPI qglCheckedMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
	dllMapGrid1f( un, u1, u2 );
	QGL_CHECK_GL( "glMapGrid1f" );
}

/* ---- qglCheckedMapGrid2d  0x004C4170 ---- */
static void QGLAPI qglCheckedMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
	dllMapGrid2d( un, u1, u2, vn, v1, v2 );
	QGL_CHECK_GL( "glMapGrid2d" );
}

/* ---- qglCheckedMapGrid2f  0x004C41D0 ---- */
static void QGLAPI qglCheckedMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
	dllMapGrid2f( un, u1, u2, vn, v1, v2 );
	QGL_CHECK_GL( "glMapGrid2f" );
}

/* ---- qglCheckedMaterialf  0x004C4210 ---- */
static void QGLAPI qglCheckedMaterialf( GLenum face, GLenum pname, GLfloat param ) {
	dllMaterialf( face, pname, param );
	QGL_CHECK_GL( "glMaterialf" );
}

/* ---- qglCheckedMaterialfv  0x004C4240 ---- */
static void QGLAPI qglCheckedMaterialfv( GLenum face, GLenum pname, const GLfloat *params ) {
	dllMaterialfv( face, pname, params );
	QGL_CHECK_GL( "glMaterialfv" );
}

/* ---- qglCheckedMateriali  0x004C4270 ---- */
static void QGLAPI qglCheckedMateriali( GLenum face, GLenum pname, GLint param ) {
	dllMateriali( face, pname, param );
	QGL_CHECK_GL( "glMateriali" );
}

/* ---- qglCheckedMaterialiv  0x004C42A0 ---- */
static void QGLAPI qglCheckedMaterialiv( GLenum face, GLenum pname, const GLint *params ) {
	dllMaterialiv( face, pname, params );
	QGL_CHECK_GL( "glMaterialiv" );
}

/* ---- qglCheckedMatrixMode  0x004C42D0 ---- */
static void QGLAPI qglCheckedMatrixMode( GLenum mode ) {
	dllMatrixMode( mode );
	QGL_CHECK_GL( "glMatrixMode" );
}

/* ---- qglCheckedMultMatrixd  0x004C4300 ---- */
static void QGLAPI qglCheckedMultMatrixd( const GLdouble *m ) {
	dllMultMatrixd( m );
	QGL_CHECK_GL( "glMultMatrixd" );
}

/* ---- qglCheckedMultMatrixf  0x004C4330 ---- */
static void QGLAPI qglCheckedMultMatrixf( const GLfloat *m ) {
	dllMultMatrixf( m );
	QGL_CHECK_GL( "glMultMatrixf" );
}

/* ---- qglCheckedNewList  0x004C4360 ---- */
static void QGLAPI qglCheckedNewList( GLuint list, GLenum mode ) {
	dllNewList( list, mode );
	QGL_CHECK_GL( "glNewList" );
}

/* ---- qglCheckedNormal3b  0x004C4390 ---- */
static void QGLAPI qglCheckedNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) {
	dllNormal3b( nx, ny, nz );
	QGL_CHECK_GL( "glNormal3b" );
}

/* ---- qglCheckedNormal3bv  0x004C43C0 ---- */
static void QGLAPI qglCheckedNormal3bv( const GLbyte *v ) {
	dllNormal3bv( v );
	QGL_CHECK_GL( "glNormal3bv" );
}

/* ---- qglCheckedNormal3d  0x004C43F0 ---- */
static void QGLAPI qglCheckedNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) {
	dllNormal3d( nx, ny, nz );
	QGL_CHECK_GL( "glNormal3d" );
}

/* ---- qglCheckedNormal3dv  0x004C4430 ---- */
static void QGLAPI qglCheckedNormal3dv( const GLdouble *v ) {
	dllNormal3dv( v );
	QGL_CHECK_GL( "glNormal3dv" );
}

/* ---- qglCheckedNormal3f  0x004C4460 ---- */
static void QGLAPI qglCheckedNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
	dllNormal3f( nx, ny, nz );
	QGL_CHECK_GL( "glNormal3f" );
}

/* ---- qglCheckedNormal3fv  0x004C4490 ---- */
static void QGLAPI qglCheckedNormal3fv( const GLfloat *v ) {
	dllNormal3fv( v );
	QGL_CHECK_GL( "glNormal3fv" );
}

/* ---- qglCheckedNormal3i  0x004C44C0 ---- */
static void QGLAPI qglCheckedNormal3i( GLint nx, GLint ny, GLint nz ) {
	dllNormal3i( nx, ny, nz );
	QGL_CHECK_GL( "glNormal3i" );
}

/* ---- qglCheckedNormal3iv  0x004C44F0 ---- */
static void QGLAPI qglCheckedNormal3iv( const GLint *v ) {
	dllNormal3iv( v );
	QGL_CHECK_GL( "glNormal3iv" );
}

/* ---- qglCheckedNormal3s  0x004C4520 ---- */
static void QGLAPI qglCheckedNormal3s( GLshort nx, GLshort ny, GLshort nz ) {
	dllNormal3s( nx, ny, nz );
	QGL_CHECK_GL( "glNormal3s" );
}

/* ---- qglCheckedNormal3sv  0x004C4550 ---- */
static void QGLAPI qglCheckedNormal3sv( const GLshort *v ) {
	dllNormal3sv( v );
	QGL_CHECK_GL( "glNormal3sv" );
}

/* ---- qglCheckedNormalPointer  0x004C4580 ---- */
static void QGLAPI qglCheckedNormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer ) {
	dllNormalPointer( type, stride, pointer );
	QGL_CHECK_GL( "glNormalPointer" );
}

/* ---- qglCheckedOrtho  0x004C45B0 ---- */
static void QGLAPI qglCheckedOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	dllOrtho( left, right, bottom, top, zNear, zFar );
	QGL_CHECK_GL( "glOrtho" );
}

/* ---- qglCheckedPassThrough  0x004C4610 ---- */
static void QGLAPI qglCheckedPassThrough( GLfloat token ) {
	dllPassThrough( token );
	QGL_CHECK_GL( "glPassThrough" );
}

/* ---- qglCheckedPixelMapfv  0x004C4640 ---- */
static void QGLAPI qglCheckedPixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values ) {
	dllPixelMapfv( map, mapsize, values );
	QGL_CHECK_GL( "glPixelMapfv" );
}

/* ---- qglCheckedPixelMapuiv  0x004C4670 ---- */
static void QGLAPI qglCheckedPixelMapuiv( GLenum map, GLsizei mapsize, const GLuint *values ) {
	dllPixelMapuiv( map, mapsize, values );
	QGL_CHECK_GL( "glPixelMapuiv" );
}

/* ---- qglCheckedPixelMapusv  0x004C46A0 ---- */
static void QGLAPI qglCheckedPixelMapusv( GLenum map, GLsizei mapsize, const GLushort *values ) {
	dllPixelMapusv( map, mapsize, values );
	QGL_CHECK_GL( "glPixelMapusv" );
}

/* ---- qglCheckedPixelStoref  0x004C46D0 ---- */
static void QGLAPI qglCheckedPixelStoref( GLenum pname, GLfloat param ) {
	dllPixelStoref( pname, param );
	QGL_CHECK_GL( "glPixelStoref" );
}

/* ---- qglCheckedPixelStorei  0x004C4700 ---- */
static void QGLAPI qglCheckedPixelStorei( GLenum pname, GLint param ) {
	dllPixelStorei( pname, param );
	QGL_CHECK_GL( "glPixelStorei" );
}

/* ---- qglCheckedPixelTransferf  0x004C4730 ---- */
static void QGLAPI qglCheckedPixelTransferf( GLenum pname, GLfloat param ) {
	dllPixelTransferf( pname, param );
	QGL_CHECK_GL( "glPixelTransferf" );
}

/* ---- qglCheckedPixelTransferi  0x004C4760 ---- */
static void QGLAPI qglCheckedPixelTransferi( GLenum pname, GLint param ) {
	dllPixelTransferi( pname, param );
	QGL_CHECK_GL( "glPixelTransferi" );
}

/* ---- qglCheckedPixelZoom  0x004C4790 ---- */
static void QGLAPI qglCheckedPixelZoom( GLfloat xfactor, GLfloat yfactor ) {
	dllPixelZoom( xfactor, yfactor );
	QGL_CHECK_GL( "glPixelZoom" );
}

/* ---- qglCheckedPointSize  0x004C47C0 ---- */
static void QGLAPI qglCheckedPointSize( GLfloat size ) {
	dllPointSize( size );
	QGL_CHECK_GL( "glPointSize" );
}

/* ---- qglCheckedPolygonMode  0x004C47F0 ---- */
static void QGLAPI qglCheckedPolygonMode( GLenum face, GLenum mode ) {
	dllPolygonMode( face, mode );
	QGL_CHECK_GL( "glPolygonMode" );
}

/* ---- qglCheckedPolygonOffset  0x004C4820 ---- */
static void QGLAPI qglCheckedPolygonOffset( GLfloat factor, GLfloat units ) {
	dllPolygonOffset( factor, units );
	QGL_CHECK_GL( "glPolygonOffset" );
}

/* ---- qglCheckedPolygonStipple  0x004C4850 ---- */
static void QGLAPI qglCheckedPolygonStipple( const GLubyte *mask ) {
	dllPolygonStipple( mask );
	QGL_CHECK_GL( "glPolygonStipple" );
}

/* ---- qglCheckedPopAttrib  0x004C4880 ---- */
static void QGLAPI qglCheckedPopAttrib( void ) {
	dllPopAttrib();
	QGL_CHECK_GL( "glPopAttrib" );
}

/* ---- qglCheckedPopClientAttrib  0x004C48A0 ---- */
static void QGLAPI qglCheckedPopClientAttrib( void ) {
	dllPopClientAttrib();
	QGL_CHECK_GL( "glPopClientAttrib" );
}

/* ---- qglCheckedPopMatrix  0x004C48C0 ---- */
static void QGLAPI qglCheckedPopMatrix( void ) {
	dllPopMatrix();
	QGL_CHECK_GL( "glPopMatrix" );
}

/* ---- qglCheckedPopName  0x004C48E0 ---- */
static void QGLAPI qglCheckedPopName( void ) {
	dllPopName();
	QGL_CHECK_GL( "glPopName" );
}

/* ---- qglCheckedPrioritizeTextures  0x004C4900 ---- */
static void QGLAPI qglCheckedPrioritizeTextures( GLsizei n, const GLuint *textures, const GLclampf *priorities ) {
	dllPrioritizeTextures( n, textures, priorities );
	QGL_CHECK_GL( "glPrioritizeTextures" );
}

/* ---- qglCheckedPushAttrib  0x004C4930 ---- */
static void QGLAPI qglCheckedPushAttrib( GLbitfield mask ) {
	dllPushAttrib( mask );
	QGL_CHECK_GL( "glPushAttrib" );
}

/* ---- qglCheckedPushClientAttrib  0x004C4960 ---- */
static void QGLAPI qglCheckedPushClientAttrib( GLbitfield mask ) {
	dllPushClientAttrib( mask );
	QGL_CHECK_GL( "glPushClientAttrib" );
}

/* ---- qglCheckedPushMatrix  0x004C4990 ---- */
static void QGLAPI qglCheckedPushMatrix( void ) {
	dllPushMatrix();
	QGL_CHECK_GL( "glPushMatrix" );
}

/* ---- qglCheckedPushName  0x004C49B0 ---- */
static void QGLAPI qglCheckedPushName( GLuint name ) {
	dllPushName( name );
	QGL_CHECK_GL( "glPushName" );
}

/* ---- qglCheckedRasterPos2d  0x004C49E0 ---- */
static void QGLAPI qglCheckedRasterPos2d( GLdouble x, GLdouble y ) {
	dllRasterPos2d( x, y );
	QGL_CHECK_GL( "glRasterPos2d" );
}

/* ---- qglCheckedRasterPos2dv  0x004C4A20 ---- */
static void QGLAPI qglCheckedRasterPos2dv( const GLdouble *v ) {
	dllRasterPos2dv( v );
	QGL_CHECK_GL( "glRasterPos2dv" );
}

/* ---- qglCheckedRasterPos2f  0x004C4A50 ---- */
static void QGLAPI qglCheckedRasterPos2f( GLfloat x, GLfloat y ) {
	dllRasterPos2f( x, y );
	QGL_CHECK_GL( "glRasterPos2f" );
}

/* ---- qglCheckedRasterPos2fv  0x004C4A80 ---- */
static void QGLAPI qglCheckedRasterPos2fv( const GLfloat *v ) {
	dllRasterPos2fv( v );
	QGL_CHECK_GL( "glRasterPos2fv" );
}

/* ---- qglCheckedRasterPos2i  0x004C4AB0 ---- */
static void QGLAPI qglCheckedRasterPos2i( GLint x, GLint y ) {
	dllRasterPos2i( x, y );
	QGL_CHECK_GL( "glRasterPos2i" );
}

/* ---- qglCheckedRasterPos2iv  0x004C4AE0 ---- */
static void QGLAPI qglCheckedRasterPos2iv( const GLint *v ) {
	dllRasterPos2iv( v );
	QGL_CHECK_GL( "glRasterPos2iv" );
}

/* ---- qglCheckedRasterPos2s  0x004C4B10 ---- */
static void QGLAPI qglCheckedRasterPos2s( GLshort x, GLshort y ) {
	dllRasterPos2s( x, y );
	QGL_CHECK_GL( "glRasterPos2s" );
}

/* ---- qglCheckedRasterPos2sv  0x004C4B40 ---- */
static void QGLAPI qglCheckedRasterPos2sv( const GLshort *v ) {
	dllRasterPos2sv( v );
	QGL_CHECK_GL( "glRasterPos2sv" );
}

/* ---- qglCheckedRasterPos3d  0x004C4B70 ---- */
static void QGLAPI qglCheckedRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) {
	dllRasterPos3d( x, y, z );
	QGL_CHECK_GL( "glRasterPos3d" );
}

/* ---- qglCheckedRasterPos3dv  0x004C4BB0 ---- */
static void QGLAPI qglCheckedRasterPos3dv( const GLdouble *v ) {
	dllRasterPos3dv( v );
	QGL_CHECK_GL( "glRasterPos3dv" );
}

/* ---- qglCheckedRasterPos3f  0x004C4BE0 ---- */
static void QGLAPI qglCheckedRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) {
	dllRasterPos3f( x, y, z );
	QGL_CHECK_GL( "glRasterPos3f" );
}

/* ---- qglCheckedRasterPos3fv  0x004C4C10 ---- */
static void QGLAPI qglCheckedRasterPos3fv( const GLfloat *v ) {
	dllRasterPos3fv( v );
	QGL_CHECK_GL( "glRasterPos3fv" );
}

/* ---- qglCheckedRasterPos3i  0x004C4C40 ---- */
static void QGLAPI qglCheckedRasterPos3i( GLint x, GLint y, GLint z ) {
	dllRasterPos3i( x, y, z );
	QGL_CHECK_GL( "glRasterPos3i" );
}

/* ---- qglCheckedRasterPos3iv  0x004C4C70 ---- */
static void QGLAPI qglCheckedRasterPos3iv( const GLint *v ) {
	dllRasterPos3iv( v );
	QGL_CHECK_GL( "glRasterPos3iv" );
}

/* ---- qglCheckedRasterPos3s  0x004C4CA0 ---- */
static void QGLAPI qglCheckedRasterPos3s( GLshort x, GLshort y, GLshort z ) {
	dllRasterPos3s( x, y, z );
	QGL_CHECK_GL( "glRasterPos3s" );
}

/* ---- qglCheckedRasterPos3sv  0x004C4CD0 ---- */
static void QGLAPI qglCheckedRasterPos3sv( const GLshort *v ) {
	dllRasterPos3sv( v );
	QGL_CHECK_GL( "glRasterPos3sv" );
}

/* ---- qglCheckedRasterPos4d  0x004C4D00 ---- */
static void QGLAPI qglCheckedRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	dllRasterPos4d( x, y, z, w );
	QGL_CHECK_GL( "glRasterPos4d" );
}

/* ---- qglCheckedRasterPos4dv  0x004C4D50 ---- */
static void QGLAPI qglCheckedRasterPos4dv( const GLdouble *v ) {
	dllRasterPos4dv( v );
	QGL_CHECK_GL( "glRasterPos4dv" );
}

/* ---- qglCheckedRasterPos4f  0x004C4D80 ---- */
static void QGLAPI qglCheckedRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	dllRasterPos4f( x, y, z, w );
	QGL_CHECK_GL( "glRasterPos4f" );
}

/* ---- qglCheckedRasterPos4fv  0x004C4DC0 ---- */
static void QGLAPI qglCheckedRasterPos4fv( const GLfloat *v ) {
	dllRasterPos4fv( v );
	QGL_CHECK_GL( "glRasterPos4fv" );
}

/* ---- qglCheckedRasterPos4i  0x004C4DF0 ---- */
static void QGLAPI qglCheckedRasterPos4i( GLint x, GLint y, GLint z, GLint w ) {
	dllRasterPos4i( x, y, z, w );
	QGL_CHECK_GL( "glRasterPos4i" );
}

/* ---- qglCheckedRasterPos4iv  0x004C4E30 ---- */
static void QGLAPI qglCheckedRasterPos4iv( const GLint *v ) {
	dllRasterPos4iv( v );
	QGL_CHECK_GL( "glRasterPos4iv" );
}

/* ---- qglCheckedRasterPos4s  0x004C4E60 ---- */
static void QGLAPI qglCheckedRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
	dllRasterPos4s( x, y, z, w );
	QGL_CHECK_GL( "glRasterPos4s" );
}

/* ---- qglCheckedRasterPos4sv  0x004C4EA0 ---- */
static void QGLAPI qglCheckedRasterPos4sv( const GLshort *v ) {
	dllRasterPos4sv( v );
	QGL_CHECK_GL( "glRasterPos4sv" );
}

/* ---- qglCheckedReadBuffer  0x004C4ED0 ---- */
static void QGLAPI qglCheckedReadBuffer( GLenum mode ) {
	dllReadBuffer( mode );
	QGL_CHECK_GL( "glReadBuffer" );
}

/* ---- qglCheckedReadPixels  0x004C4F00 ---- */
static void QGLAPI qglCheckedReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) {
	dllReadPixels( x, y, width, height, format, type, pixels );
	QGL_CHECK_GL( "glReadPixels" );
}

/* ---- qglCheckedRectd  0x004C4F50 ---- */
static void QGLAPI qglCheckedRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
	dllRectd( x1, y1, x2, y2 );
	QGL_CHECK_GL( "glRectd" );
}

/* ---- qglCheckedRectdv  0x004C4FA0 ---- */
static void QGLAPI qglCheckedRectdv( const GLdouble *v1, const GLdouble *v2 ) {
	dllRectdv( v1, v2 );
	QGL_CHECK_GL( "glRectdv" );
}

/* ---- qglCheckedRectf  0x004C4FD0 ---- */
static void QGLAPI qglCheckedRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
	dllRectf( x1, y1, x2, y2 );
	QGL_CHECK_GL( "glRectf" );
}

/* ---- qglCheckedRectfv  0x004C5010 ---- */
static void QGLAPI qglCheckedRectfv( const GLfloat *v1, const GLfloat *v2 ) {
	dllRectfv( v1, v2 );
	QGL_CHECK_GL( "glRectfv" );
}

/* ---- qglCheckedRecti  0x004C5040 ---- */
static void QGLAPI qglCheckedRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) {
	dllRecti( x1, y1, x2, y2 );
	QGL_CHECK_GL( "glRecti" );
}

/* ---- qglCheckedRectiv  0x004C5080 ---- */
static void QGLAPI qglCheckedRectiv( const GLint *v1, const GLint *v2 ) {
	dllRectiv( v1, v2 );
	QGL_CHECK_GL( "glRectiv" );
}

/* ---- qglCheckedRects  0x004C50B0 ---- */
static void QGLAPI qglCheckedRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
	dllRects( x1, y1, x2, y2 );
	QGL_CHECK_GL( "glRects" );
}

/* ---- qglCheckedRectsv  0x004C50F0 ---- */
static void QGLAPI qglCheckedRectsv( const GLshort *v1, const GLshort *v2 ) {
	dllRectsv( v1, v2 );
	QGL_CHECK_GL( "glRectsv" );
}

/* ---- qglCheckedRenderMode  0x004C5120 ---- */
static GLint QGLAPI qglCheckedRenderMode( GLenum mode ) {
	GLint result = dllRenderMode( mode );
	QGL_CHECK_GL( "glRenderMode" );
	return result;
}

/* ---- qglCheckedRotated  0x004C5150 ---- */
static void QGLAPI qglCheckedRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
	dllRotated( angle, x, y, z );
	QGL_CHECK_GL( "glRotated" );
}

/* ---- qglCheckedRotatef  0x004C51A0 ---- */
static void QGLAPI qglCheckedRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
	dllRotatef( angle, x, y, z );
	QGL_CHECK_GL( "glRotatef" );
}

/* ---- qglCheckedScaled  0x004C51E0 ---- */
static void QGLAPI qglCheckedScaled( GLdouble x, GLdouble y, GLdouble z ) {
	dllScaled( x, y, z );
	QGL_CHECK_GL( "glScaled" );
}

/* ---- qglCheckedScalef  0x004C5220 ---- */
static void QGLAPI qglCheckedScalef( GLfloat x, GLfloat y, GLfloat z ) {
	dllScalef( x, y, z );
	QGL_CHECK_GL( "glScalef" );
}

/* ---- qglCheckedScissor  0x004C5250 ---- */
static void QGLAPI qglCheckedScissor( GLint x, GLint y, GLsizei width, GLsizei height ) {
	dllScissor( x, y, width, height );
	QGL_CHECK_GL( "glScissor" );
}

/* ---- qglCheckedSelectBuffer  0x004C5290 ---- */
static void QGLAPI qglCheckedSelectBuffer( GLsizei size, GLuint *buffer ) {
	dllSelectBuffer( size, buffer );
	QGL_CHECK_GL( "glSelectBuffer" );
}

/* ---- qglCheckedShadeModel  0x004C52C0 ---- */
static void QGLAPI qglCheckedShadeModel( GLenum mode ) {
	dllShadeModel( mode );
	QGL_CHECK_GL( "glShadeModel" );
}

/* ---- qglCheckedStencilFunc  0x004C52F0 ---- */
static void QGLAPI qglCheckedStencilFunc( GLenum func, GLint ref, GLuint mask ) {
	dllStencilFunc( func, ref, mask );
	QGL_CHECK_GL( "glStencilFunc" );
}

/* ---- qglCheckedStencilMask  0x004C5320 ---- */
static void QGLAPI qglCheckedStencilMask( GLuint mask ) {
	dllStencilMask( mask );
	QGL_CHECK_GL( "glStencilMask" );
}

/* ---- qglCheckedStencilOp  0x004C5350 ---- */
static void QGLAPI qglCheckedStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
	dllStencilOp( fail, zfail, zpass );
	QGL_CHECK_GL( "glStencilOp" );
}

/* ---- qglCheckedTexCoord1d  0x004C5380 ---- */
static void QGLAPI qglCheckedTexCoord1d( GLdouble s ) {
	dllTexCoord1d( s );
	QGL_CHECK_GL( "glTexCoord1d" );
}

/* ---- qglCheckedTexCoord1dv  0x004C53B0 ---- */
static void QGLAPI qglCheckedTexCoord1dv( const GLdouble *v ) {
	dllTexCoord1dv( v );
	QGL_CHECK_GL( "glTexCoord1dv" );
}

/* ---- qglCheckedTexCoord1f  0x004C53E0 ---- */
static void QGLAPI qglCheckedTexCoord1f( GLfloat s ) {
	dllTexCoord1f( s );
	QGL_CHECK_GL( "glTexCoord1f" );
}

/* ---- qglCheckedTexCoord1fv  0x004C5410 ---- */
static void QGLAPI qglCheckedTexCoord1fv( const GLfloat *v ) {
	dllTexCoord1fv( v );
	QGL_CHECK_GL( "glTexCoord1fv" );
}

/* ---- qglCheckedTexCoord1i  0x004C5440 ---- */
static void QGLAPI qglCheckedTexCoord1i( GLint s ) {
	dllTexCoord1i( s );
	QGL_CHECK_GL( "glTexCoord1i" );
}

/* ---- qglCheckedTexCoord1iv  0x004C5470 ---- */
static void QGLAPI qglCheckedTexCoord1iv( const GLint *v ) {
	dllTexCoord1iv( v );
	QGL_CHECK_GL( "glTexCoord1iv" );
}

/* ---- qglCheckedTexCoord1s  0x004C54A0 ---- */
static void QGLAPI qglCheckedTexCoord1s( GLshort s ) {
	dllTexCoord1s( s );
	QGL_CHECK_GL( "glTexCoord1s" );
}

/* ---- qglCheckedTexCoord1sv  0x004C54D0 ---- */
static void QGLAPI qglCheckedTexCoord1sv( const GLshort *v ) {
	dllTexCoord1sv( v );
	QGL_CHECK_GL( "glTexCoord1sv" );
}

/* ---- qglCheckedTexCoord2d  0x004C5500 ---- */
static void QGLAPI qglCheckedTexCoord2d( GLdouble s, GLdouble t ) {
	dllTexCoord2d( s, t );
	QGL_CHECK_GL( "glTexCoord2d" );
}

/* ---- qglCheckedTexCoord2dv  0x004C5540 ---- */
static void QGLAPI qglCheckedTexCoord2dv( const GLdouble *v ) {
	dllTexCoord2dv( v );
	QGL_CHECK_GL( "glTexCoord2dv" );
}

/* ---- qglCheckedTexCoord2f  0x004C5570 ---- */
static void QGLAPI qglCheckedTexCoord2f( GLfloat s, GLfloat t ) {
	dllTexCoord2f( s, t );
	QGL_CHECK_GL( "glTexCoord2f" );
}

/* ---- qglCheckedTexCoord2fv  0x004C55A0 ---- */
static void QGLAPI qglCheckedTexCoord2fv( const GLfloat *v ) {
	dllTexCoord2fv( v );
	QGL_CHECK_GL( "glTexCoord2fv" );
}

/* ---- qglCheckedTexCoord2i  0x004C55D0 ---- */
static void QGLAPI qglCheckedTexCoord2i( GLint s, GLint t ) {
	dllTexCoord2i( s, t );
	QGL_CHECK_GL( "glTexCoord2i" );
}

/* ---- qglCheckedTexCoord2iv  0x004C5600 ---- */
static void QGLAPI qglCheckedTexCoord2iv( const GLint *v ) {
	dllTexCoord2iv( v );
	QGL_CHECK_GL( "glTexCoord2iv" );
}

/* ---- qglCheckedTexCoord2s  0x004C5630 ---- */
static void QGLAPI qglCheckedTexCoord2s( GLshort s, GLshort t ) {
	dllTexCoord2s( s, t );
	QGL_CHECK_GL( "glTexCoord2s" );
}

/* ---- qglCheckedTexCoord2sv  0x004C5660 ---- */
static void QGLAPI qglCheckedTexCoord2sv( const GLshort *v ) {
	dllTexCoord2sv( v );
	QGL_CHECK_GL( "glTexCoord2sv" );
}

/* ---- qglCheckedTexCoord3d  0x004C5690 ---- */
static void QGLAPI qglCheckedTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) {
	dllTexCoord3d( s, t, r );
	QGL_CHECK_GL( "glTexCoord3d" );
}

/* ---- qglCheckedTexCoord3dv  0x004C56D0 ---- */
static void QGLAPI qglCheckedTexCoord3dv( const GLdouble *v ) {
	dllTexCoord3dv( v );
	QGL_CHECK_GL( "glTexCoord3dv" );
}

/* ---- qglCheckedTexCoord3f  0x004C5700 ---- */
static void QGLAPI qglCheckedTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
	dllTexCoord3f( s, t, r );
	QGL_CHECK_GL( "glTexCoord3f" );
}

/* ---- qglCheckedTexCoord3fv  0x004C5730 ---- */
static void QGLAPI qglCheckedTexCoord3fv( const GLfloat *v ) {
	dllTexCoord3fv( v );
	QGL_CHECK_GL( "glTexCoord3fv" );
}

/* ---- qglCheckedTexCoord3i  0x004C5760 ---- */
static void QGLAPI qglCheckedTexCoord3i( GLint s, GLint t, GLint r ) {
	dllTexCoord3i( s, t, r );
	QGL_CHECK_GL( "glTexCoord3i" );
}

/* ---- qglCheckedTexCoord3iv  0x004C5790 ---- */
static void QGLAPI qglCheckedTexCoord3iv( const GLint *v ) {
	dllTexCoord3iv( v );
	QGL_CHECK_GL( "glTexCoord3iv" );
}

/* ---- qglCheckedTexCoord3s  0x004C57C0 ---- */
static void QGLAPI qglCheckedTexCoord3s( GLshort s, GLshort t, GLshort r ) {
	dllTexCoord3s( s, t, r );
	QGL_CHECK_GL( "glTexCoord3s" );
}

/* ---- qglCheckedTexCoord3sv  0x004C57F0 ---- */
static void QGLAPI qglCheckedTexCoord3sv( const GLshort *v ) {
	dllTexCoord3sv( v );
	QGL_CHECK_GL( "glTexCoord3sv" );
}

/* ---- qglCheckedTexCoord4d  0x004C5820 ---- */
static void QGLAPI qglCheckedTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
	dllTexCoord4d( s, t, r, q );
	QGL_CHECK_GL( "glTexCoord4d" );
}

/* ---- qglCheckedTexCoord4dv  0x004C5870 ---- */
static void QGLAPI qglCheckedTexCoord4dv( const GLdouble *v ) {
	dllTexCoord4dv( v );
	QGL_CHECK_GL( "glTexCoord4dv" );
}

/* ---- qglCheckedTexCoord4f  0x004C58A0 ---- */
static void QGLAPI qglCheckedTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
	dllTexCoord4f( s, t, r, q );
	QGL_CHECK_GL( "glTexCoord4f" );
}

/* ---- qglCheckedTexCoord4fv  0x004C58E0 ---- */
static void QGLAPI qglCheckedTexCoord4fv( const GLfloat *v ) {
	dllTexCoord4fv( v );
	QGL_CHECK_GL( "glTexCoord4fv" );
}

/* ---- qglCheckedTexCoord4i  0x004C5910 ---- */
static void QGLAPI qglCheckedTexCoord4i( GLint s, GLint t, GLint r, GLint q ) {
	dllTexCoord4i( s, t, r, q );
	QGL_CHECK_GL( "glTexCoord4i" );
}

/* ---- qglCheckedTexCoord4iv  0x004C5950 ---- */
static void QGLAPI qglCheckedTexCoord4iv( const GLint *v ) {
	dllTexCoord4iv( v );
	QGL_CHECK_GL( "glTexCoord4iv" );
}

/* ---- qglCheckedTexCoord4s  0x004C5980 ---- */
static void QGLAPI qglCheckedTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) {
	dllTexCoord4s( s, t, r, q );
	QGL_CHECK_GL( "glTexCoord4s" );
}

/* ---- qglCheckedTexCoord4sv  0x004C59C0 ---- */
static void QGLAPI qglCheckedTexCoord4sv( const GLshort *v ) {
	dllTexCoord4sv( v );
	QGL_CHECK_GL( "glTexCoord4sv" );
}

/* ---- qglCheckedTexCoordPointer  0x004C59F0 ---- */
static void QGLAPI qglCheckedTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	dllTexCoordPointer( size, type, stride, pointer );
	QGL_CHECK_GL( "glTexCoordPointer" );
}

/* ---- qglCheckedTexEnvf  0x004C5A30 ---- */
static void QGLAPI qglCheckedTexEnvf( GLenum target, GLenum pname, GLfloat param ) {
	dllTexEnvf( target, pname, param );
	QGL_CHECK_GL( "glTexEnvf" );
}

/* ---- qglCheckedTexEnvfv  0x004C5A60 ---- */
static void QGLAPI qglCheckedTexEnvfv( GLenum target, GLenum pname, const GLfloat *params ) {
	dllTexEnvfv( target, pname, params );
	QGL_CHECK_GL( "glTexEnvfv" );
}

/* ---- qglCheckedTexEnvi  0x004C5A90 ---- */
static void QGLAPI qglCheckedTexEnvi( GLenum target, GLenum pname, GLint param ) {
	dllTexEnvi( target, pname, param );
	QGL_CHECK_GL( "glTexEnvi" );
}

/* ---- qglCheckedTexEnviv  0x004C5AC0 ---- */
static void QGLAPI qglCheckedTexEnviv( GLenum target, GLenum pname, const GLint *params ) {
	dllTexEnviv( target, pname, params );
	QGL_CHECK_GL( "glTexEnviv" );
}

/* ---- qglCheckedTexGend  0x004C5AF0 ---- */
static void QGLAPI qglCheckedTexGend( GLenum coord, GLenum pname, GLdouble param ) {
	dllTexGend( coord, pname, param );
	QGL_CHECK_GL( "glTexGend" );
}

/* ---- qglCheckedTexGendv  0x004C5B30 ---- */
static void QGLAPI qglCheckedTexGendv( GLenum coord, GLenum pname, const GLdouble *params ) {
	dllTexGendv( coord, pname, params );
	QGL_CHECK_GL( "glTexGendv" );
}

/* ---- qglCheckedTexGenf  0x004C5B60 ---- */
static void QGLAPI qglCheckedTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
	dllTexGenf( coord, pname, param );
	QGL_CHECK_GL( "glTexGenf" );
}

/* ---- qglCheckedTexGenfv  0x004C5B90 ---- */
static void QGLAPI qglCheckedTexGenfv( GLenum coord, GLenum pname, const GLfloat *params ) {
	dllTexGenfv( coord, pname, params );
	QGL_CHECK_GL( "glTexGenfv" );
}

/* ---- qglCheckedTexGeni  0x004C5BC0 ---- */
static void QGLAPI qglCheckedTexGeni( GLenum coord, GLenum pname, GLint param ) {
	dllTexGeni( coord, pname, param );
	QGL_CHECK_GL( "glTexGeni" );
}

/* ---- qglCheckedTexGeniv  0x004C5BF0 ---- */
static void QGLAPI qglCheckedTexGeniv( GLenum coord, GLenum pname, const GLint *params ) {
	dllTexGeniv( coord, pname, params );
	QGL_CHECK_GL( "glTexGeniv" );
}

/* ---- qglCheckedTexImage1D  0x004C5C20 ---- */
static void QGLAPI qglCheckedTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
	dllTexImage1D( target, level, internalformat, width, border, format, type, pixels );
	QGL_CHECK_GL( "glTexImage1D" );
}

/* ---- qglCheckedTexImage2D  0x004C5C70 ---- */
static void QGLAPI qglCheckedTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
	dllTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
	QGL_CHECK_GL( "glTexImage2D" );
}

/* ---- qglCheckedTexParameterf  0x004C5CC0 ---- */
static void QGLAPI qglCheckedTexParameterf( GLenum target, GLenum pname, GLfloat param ) {
	dllTexParameterf( target, pname, param );
	QGL_CHECK_GL( "glTexParameterf" );
}

/* ---- qglCheckedTexParameterfv  0x004C5CF0 ---- */
static void QGLAPI qglCheckedTexParameterfv( GLenum target, GLenum pname, const GLfloat *params ) {
	dllTexParameterfv( target, pname, params );
	QGL_CHECK_GL( "glTexParameterfv" );
}

/* ---- qglCheckedTexParameteri  0x004C5D20 ---- */
static void QGLAPI qglCheckedTexParameteri( GLenum target, GLenum pname, GLint param ) {
	dllTexParameteri( target, pname, param );
	QGL_CHECK_GL( "glTexParameteri" );
}

/* ---- qglCheckedTexParameteriv  0x004C5D50 ---- */
static void QGLAPI qglCheckedTexParameteriv( GLenum target, GLenum pname, const GLint *params ) {
	dllTexParameteriv( target, pname, params );
	QGL_CHECK_GL( "glTexParameteriv" );
}

/* ---- qglCheckedTexSubImage1D  0x004C5D80 ---- */
static void QGLAPI qglCheckedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels ) {
	dllTexSubImage1D( target, level, xoffset, width, format, type, pixels );
	QGL_CHECK_GL( "glTexSubImage1D" );
}

/* ---- qglCheckedTexSubImage2D  0x004C5DD0 ---- */
static void QGLAPI qglCheckedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
	dllTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
	QGL_CHECK_GL( "glTexSubImage2D" );
}

/* ---- qglCheckedTranslated  0x004C5E20 ---- */
static void QGLAPI qglCheckedTranslated( GLdouble x, GLdouble y, GLdouble z ) {
	dllTranslated( x, y, z );
	QGL_CHECK_GL( "glTranslated" );
}

/* ---- qglCheckedTranslatef  0x004C5E60 ---- */
static void QGLAPI qglCheckedTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
	dllTranslatef( x, y, z );
	QGL_CHECK_GL( "glTranslatef" );
}

/* ---- qglCheckedVertex2d  0x004C5E90 ---- */
static void QGLAPI qglCheckedVertex2d( GLdouble x, GLdouble y ) {
	dllVertex2d( x, y );
	QGL_CHECK_GL( "glVertex2d" );
}

/* ---- qglCheckedVertex2dv  0x004C5ED0 ---- */
static void QGLAPI qglCheckedVertex2dv( const GLdouble *v ) {
	dllVertex2dv( v );
	QGL_CHECK_GL( "glVertex2dv" );
}

/* ---- qglCheckedVertex2f  0x004C5F00 ---- */
static void QGLAPI qglCheckedVertex2f( GLfloat x, GLfloat y ) {
	dllVertex2f( x, y );
	QGL_CHECK_GL( "glVertex2f" );
}

/* ---- qglCheckedVertex2fv  0x004C5F30 ---- */
static void QGLAPI qglCheckedVertex2fv( const GLfloat *v ) {
	dllVertex2fv( v );
	QGL_CHECK_GL( "glVertex2fv" );
}

/* ---- qglCheckedVertex2i  0x004C5F60 ---- */
static void QGLAPI qglCheckedVertex2i( GLint x, GLint y ) {
	dllVertex2i( x, y );
	QGL_CHECK_GL( "glVertex2i" );
}

/* ---- qglCheckedVertex2iv  0x004C5F90 ---- */
static void QGLAPI qglCheckedVertex2iv( const GLint *v ) {
	dllVertex2iv( v );
	QGL_CHECK_GL( "glVertex2iv" );
}

/* ---- qglCheckedVertex2s  0x004C5FC0 ---- */
static void QGLAPI qglCheckedVertex2s( GLshort x, GLshort y ) {
	dllVertex2s( x, y );
	QGL_CHECK_GL( "glVertex2s" );
}

/* ---- qglCheckedVertex2sv  0x004C5FF0 ---- */
static void QGLAPI qglCheckedVertex2sv( const GLshort *v ) {
	dllVertex2sv( v );
	QGL_CHECK_GL( "glVertex2sv" );
}

/* ---- qglCheckedVertex3d  0x004C6020 ---- */
static void QGLAPI qglCheckedVertex3d( GLdouble x, GLdouble y, GLdouble z ) {
	dllVertex3d( x, y, z );
	QGL_CHECK_GL( "glVertex3d" );
}

/* ---- qglCheckedVertex3dv  0x004C6060 ---- */
static void QGLAPI qglCheckedVertex3dv( const GLdouble *v ) {
	dllVertex3dv( v );
	QGL_CHECK_GL( "glVertex3dv" );
}

/* ---- qglCheckedVertex3f  0x004C6090 ---- */
static void QGLAPI qglCheckedVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
	dllVertex3f( x, y, z );
	QGL_CHECK_GL( "glVertex3f" );
}

/* ---- qglCheckedVertex3fv  0x004C60C0 ---- */
static void QGLAPI qglCheckedVertex3fv( const GLfloat *v ) {
	dllVertex3fv( v );
	QGL_CHECK_GL( "glVertex3fv" );
}

/* ---- qglCheckedVertex3i  0x004C60F0 ---- */
static void QGLAPI qglCheckedVertex3i( GLint x, GLint y, GLint z ) {
	dllVertex3i( x, y, z );
	QGL_CHECK_GL( "glVertex3i" );
}

/* ---- qglCheckedVertex3iv  0x004C6120 ---- */
static void QGLAPI qglCheckedVertex3iv( const GLint *v ) {
	dllVertex3iv( v );
	QGL_CHECK_GL( "glVertex3iv" );
}

/* ---- qglCheckedVertex3s  0x004C6150 ---- */
static void QGLAPI qglCheckedVertex3s( GLshort x, GLshort y, GLshort z ) {
	dllVertex3s( x, y, z );
	QGL_CHECK_GL( "glVertex3s" );
}

/* ---- qglCheckedVertex3sv  0x004C6180 ---- */
static void QGLAPI qglCheckedVertex3sv( const GLshort *v ) {
	dllVertex3sv( v );
	QGL_CHECK_GL( "glVertex3sv" );
}

/* ---- qglCheckedVertex4d  0x004C61B0 ---- */
static void QGLAPI qglCheckedVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	dllVertex4d( x, y, z, w );
	QGL_CHECK_GL( "glVertex4d" );
}

/* ---- qglCheckedVertex4dv  0x004C6200 ---- */
static void QGLAPI qglCheckedVertex4dv( const GLdouble *v ) {
	dllVertex4dv( v );
	QGL_CHECK_GL( "glVertex4dv" );
}

/* ---- qglCheckedVertex4f  0x004C6230 ---- */
static void QGLAPI qglCheckedVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	dllVertex4f( x, y, z, w );
	QGL_CHECK_GL( "glVertex4f" );
}

/* ---- qglCheckedVertex4fv  0x004C6270 ---- */
static void QGLAPI qglCheckedVertex4fv( const GLfloat *v ) {
	dllVertex4fv( v );
	QGL_CHECK_GL( "glVertex4fv" );
}

/* ---- qglCheckedVertex4i  0x004C62A0 ---- */
static void QGLAPI qglCheckedVertex4i( GLint x, GLint y, GLint z, GLint w ) {
	dllVertex4i( x, y, z, w );
	QGL_CHECK_GL( "glVertex4i" );
}

/* ---- qglCheckedVertex4iv  0x004C62E0 ---- */
static void QGLAPI qglCheckedVertex4iv( const GLint *v ) {
	dllVertex4iv( v );
	QGL_CHECK_GL( "glVertex4iv" );
}

/* ---- qglCheckedVertex4s  0x004C6310 ---- */
static void QGLAPI qglCheckedVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
	dllVertex4s( x, y, z, w );
	QGL_CHECK_GL( "glVertex4s" );
}

/* ---- qglCheckedVertex4sv  0x004C6350 ---- */
static void QGLAPI qglCheckedVertex4sv( const GLshort *v ) {
	dllVertex4sv( v );
	QGL_CHECK_GL( "glVertex4sv" );
}

/* ---- qglCheckedVertexPointer  0x004C6380 ---- */
static void QGLAPI qglCheckedVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	dllVertexPointer( size, type, stride, pointer );
	QGL_CHECK_GL( "glVertexPointer" );
}

/* ---- qglCheckedViewport  0x004C63C0 ---- */
static void QGLAPI qglCheckedViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
	dllViewport( x, y, width, height );
	QGL_CHECK_GL( "glViewport" );
}

/* ---- qwglCheckedCopyContext  0x004C6400 ---- */
static BOOL WINAPI qwglCheckedCopyContext( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask ) {
	BOOL result = dllwglCopyContext( hglrcSrc, hglrcDst, mask );
	QGL_CHECK_WGL( "wglCopyContext" );
	return result;
}

/* ---- qwglCheckedCreateContext  0x004C6440 ---- */
static HGLRC WINAPI qwglCheckedCreateContext( HDC hdc ) {
	HGLRC result = dllwglCreateContext( hdc );
	QGL_CHECK_WGL( "wglCreateContext" );
	return result;
}

/* ---- qwglCheckedCreateLayerContext  0x004C6470 ---- */
static HGLRC WINAPI qwglCheckedCreateLayerContext( HDC hdc, int iLayerPlane ) {
	HGLRC result = dllwglCreateLayerContext( hdc, iLayerPlane );
	QGL_CHECK_WGL( "wglCreateLayerContext" );
	return result;
}

/* ---- qwglCheckedDeleteContext  0x004C64B0 ---- */
static BOOL WINAPI qwglCheckedDeleteContext( HGLRC hglrc ) {
	BOOL result = dllwglDeleteContext( hglrc );
	QGL_CHECK_WGL( "wglDeleteContext" );
	return result;
}

/* ---- qwglCheckedGetCurrentContext  0x004C64E0 ---- */
static HGLRC WINAPI qwglCheckedGetCurrentContext( VOID ) {
	HGLRC result = dllwglGetCurrentContext();
	QGL_CHECK_WGL( "wglGetCurrentContext" );
	return result;
}

/* ---- qwglCheckedGetCurrentDC  0x004C6510 ---- */
static HDC WINAPI qwglCheckedGetCurrentDC( VOID ) {
	HDC result = dllwglGetCurrentDC();
	QGL_CHECK_WGL( "wglGetCurrentDC" );
	return result;
}

/* ---- qwglCheckedGetProcAddress  0x004C6540 ---- */
static PROC WINAPI qwglCheckedGetProcAddress( LPCSTR lpszProc ) {
	PROC result = dllwglGetProcAddress( lpszProc );
	QGL_CHECK_WGL( "wglGetProcAddress" );
	return result;
}

/* ---- qwglCheckedMakeCurrent  0x004C6570 ---- */
static BOOL WINAPI qwglCheckedMakeCurrent( HDC hdc, HGLRC hglrc ) {
	BOOL result = dllwglMakeCurrent( hdc, hglrc );
	QGL_CHECK_WGL( "wglMakeCurrent" );
	return result;
}

/* ---- qwglCheckedShareLists  0x004C65B0 ---- */
static BOOL WINAPI qwglCheckedShareLists( HGLRC hglrc1, HGLRC hglrc2 ) {
	BOOL result = dllwglShareLists( hglrc1, hglrc2 );
	QGL_CHECK_WGL( "wglShareLists" );
	return result;
}

/* ---- qwglCheckedUseFontBitmaps  0x004C65F0 ---- */
static BOOL WINAPI qwglCheckedUseFontBitmaps( HDC hdc, DWORD first, DWORD count, DWORD listBase ) {
	BOOL result = dllwglUseFontBitmaps( hdc, first, count, listBase );
	QGL_CHECK_WGL( "wglUseFontBitmaps" );
	return result;
}

/* ---- qwglCheckedUseFontOutlines  0x004C6630 ---- */
static BOOL WINAPI qwglCheckedUseFontOutlines( HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf ) {
	BOOL result = dllwglUseFontOutlines( hdc, first, count, listBase, deviation, extrusion, format, lpgmf );
	QGL_CHECK_WGL( "wglUseFontOutlines" );
	return result;
}

/* ---- qwglCheckedDescribeLayerPlane  0x004C6680 ---- */
static BOOL WINAPI qwglCheckedDescribeLayerPlane( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR plpd ) {
	BOOL result = dllwglDescribeLayerPlane( hdc, iPixelFormat, iLayerPlane, nBytes, plpd );
	QGL_CHECK_WGL( "wglDescribeLayerPlane" );
	return result;
}

/* ---- qwglCheckedSetLayerPaletteEntries  0x004C66C0 ---- */
static int WINAPI qwglCheckedSetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart, int cEntries, CONST COLORREF *pcr ) {
	int result = dllwglSetLayerPaletteEntries( hdc, iLayerPlane, iStart, cEntries, pcr );
	QGL_CHECK_WGL( "wglSetLayerPaletteEntries" );
	return result;
}

/* ---- qwglCheckedGetLayerPaletteEntries  0x004C6700 ---- */
static int WINAPI qwglCheckedGetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF *pcr ) {
	int result = dllwglGetLayerPaletteEntries( hdc, iLayerPlane, iStart, cEntries, pcr );
	QGL_CHECK_WGL( "wglGetLayerPaletteEntries" );
	return result;
}

/* ---- qwglCheckedRealizeLayerPalette  0x004C6740 ---- */
static BOOL WINAPI qwglCheckedRealizeLayerPalette( HDC hdc, int iLayerPlane, BOOL bRealize ) {
	BOOL result = dllwglRealizeLayerPalette( hdc, iLayerPlane, bRealize );
	QGL_CHECK_WGL( "wglRealizeLayerPalette" );
	return result;
}

/* ---- qwglCheckedSwapLayerBuffers  0x004C6780 ---- */
static BOOL WINAPI qwglCheckedSwapLayerBuffers( HDC hdc, UINT fuPlanes ) {
	BOOL result = dllwglSwapLayerBuffers( hdc, fuPlanes );
	QGL_CHECK_WGL( "wglSwapLayerBuffers" );
	return result;
}

/* ---- qglCheckedActiveTextureARB  0x004C00C0 ---- */
static void QGLAPI qglCheckedActiveTextureARB( GLenum texture ) {
	dllActiveTextureARB( texture );
	QGL_CHECK_GL( "glActiveTextureARB" );
}

/* ---- qglCheckedAlphaFragmentOp1ATI  0x004C1160 ---- */
static void QGLAPI qglCheckedAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
	dllAlphaFragmentOp1ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod );
	QGL_CHECK_GL( "glAlphaFragmentOp1ATI" );
}

/* ---- qglCheckedAlphaFragmentOp2ATI  0x004C11A0 ---- */
static void QGLAPI qglCheckedAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
	dllAlphaFragmentOp2ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
	QGL_CHECK_GL( "glAlphaFragmentOp2ATI" );
}

/* ---- qglCheckedAlphaFragmentOp3ATI  0x004C11F0 ---- */
static void QGLAPI qglCheckedAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
	dllAlphaFragmentOp3ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
	QGL_CHECK_GL( "glAlphaFragmentOp3ATI" );
}

/* ---- qglCheckedArrayObjectATI  0x004C07C0 ---- */
static void QGLAPI qglCheckedArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
	dllArrayObjectATI( array, size, type, stride, buffer, offset );
	QGL_CHECK_GL( "glArrayObjectATI" );
}

/* ---- qglCheckedBeginFragmentShaderATI  0x004C0FA0 ---- */
static void QGLAPI qglCheckedBeginFragmentShaderATI( void ) {
	dllBeginFragmentShaderATI();
	QGL_CHECK_GL( "glBeginFragmentShaderATI" );
}

/* ---- qglCheckedBindBufferARB  0x004C0430 ---- */
static void QGLAPI qglCheckedBindBufferARB( GLenum target, GLuint buffer ) {
	dllBindBufferARB( target, buffer );
	QGL_CHECK_GL( "glBindBufferARB" );
}

/* ---- qglCheckedBindFragmentShaderATI  0x004C0F40 ---- */
static void QGLAPI qglCheckedBindFragmentShaderATI( GLuint id ) {
	dllBindFragmentShaderATI( id );
	QGL_CHECK_GL( "glBindFragmentShaderATI" );
}

/* ---- qglCheckedBindProgramARB  0x004C1AC0 ---- */
static void QGLAPI qglCheckedBindProgramARB( GLenum target, GLuint program ) {
	dllBindProgramARB( target, program );
	QGL_CHECK_GL( "glBindProgramARB" );
}

/* ---- qglCheckedBufferDataARB  0x004C04F0 ---- */
static void QGLAPI qglCheckedBufferDataARB( GLenum target, GLsizei size, const GLvoid *data, GLenum usage ) {
	dllBufferDataARB( target, size, data, usage );
	QGL_CHECK_GL( "glBufferDataARB" );
}

/* ---- qglCheckedBufferSubDataARB  0x004C0530 ---- */
static void QGLAPI qglCheckedBufferSubDataARB( GLenum target, GLint offset, GLsizei size, const GLvoid *data ) {
	dllBufferSubDataARB( target, offset, size, data );
	QGL_CHECK_GL( "glBufferSubDataARB" );
}

/* ---- qglCheckedClientActiveTextureARB  0x004C00F0 ---- */
static void QGLAPI qglCheckedClientActiveTextureARB( GLenum texture ) {
	dllClientActiveTextureARB( texture );
	QGL_CHECK_GL( "glClientActiveTextureARB" );
}

/* ---- qglCheckedColorFragmentOp1ATI  0x004C1040 ---- */
static void QGLAPI qglCheckedColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
	dllColorFragmentOp1ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
	QGL_CHECK_GL( "glColorFragmentOp1ATI" );
}

/* ---- qglCheckedColorFragmentOp2ATI  0x004C1090 ---- */
static void QGLAPI qglCheckedColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
	dllColorFragmentOp2ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
	QGL_CHECK_GL( "glColorFragmentOp2ATI" );
}

/* ---- qglCheckedColorFragmentOp3ATI  0x004C10F0 ---- */
static void QGLAPI qglCheckedColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
	dllColorFragmentOp3ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
	QGL_CHECK_GL( "glColorFragmentOp3ATI" );
}

/* ---- qglCheckedCombinerInputNV  0x004C0C70 ---- */
static void QGLAPI qglCheckedCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
	dllCombinerInputNV( stage, portion, variable, input, mapping, componentUsage );
	QGL_CHECK_GL( "glCombinerInputNV" );
}

/* ---- qglCheckedCombinerOutputNV  0x004C0CB0 ---- */
static void QGLAPI qglCheckedCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) {
	dllCombinerOutputNV( stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
	QGL_CHECK_GL( "glCombinerOutputNV" );
}

/* ---- qglCheckedCombinerParameterfNV  0x004C0BE0 ---- */
static void QGLAPI qglCheckedCombinerParameterfNV( GLenum pname, GLfloat param ) {
	dllCombinerParameterfNV( pname, param );
	QGL_CHECK_GL( "glCombinerParameterfNV" );
}

/* ---- qglCheckedCombinerParameterfvNV  0x004C0BB0 ---- */
static void QGLAPI qglCheckedCombinerParameterfvNV( GLenum pname, const GLfloat *params ) {
	dllCombinerParameterfvNV( pname, params );
	QGL_CHECK_GL( "glCombinerParameterfvNV" );
}

/* ---- qglCheckedCombinerParameteriNV  0x004C0C40 ---- */
static void QGLAPI qglCheckedCombinerParameteriNV( GLenum pname, GLint param ) {
	dllCombinerParameteriNV( pname, param );
	QGL_CHECK_GL( "glCombinerParameteriNV" );
}

/* ---- qglCheckedCombinerParameterivNV  0x004C0C10 ---- */
static void QGLAPI qglCheckedCombinerParameterivNV( GLenum pname, const GLint *params ) {
	dllCombinerParameterivNV( pname, params );
	QGL_CHECK_GL( "glCombinerParameterivNV" );
}

/* ---- qglCheckedCombinerStageParameterfvNV  0x004C0EB0 ---- */
static void QGLAPI qglCheckedCombinerStageParameterfvNV( GLenum stage, GLenum pname, const GLfloat *params ) {
	dllCombinerStageParameterfvNV( stage, pname, params );
	QGL_CHECK_GL( "glCombinerStageParameterfvNV" );
}

/* ---- qglCheckedCompressedTexImage1DARB  0x004C02B0 ---- */
static void QGLAPI qglCheckedCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data ) {
	dllCompressedTexImage1DARB( target, level, internalformat, width, border, imageSize, data );
	QGL_CHECK_GL( "glCompressedTexImage1DARB" );
}

/* ---- qglCheckedCompressedTexImage2DARB  0x004C0260 ---- */
static void QGLAPI qglCheckedCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data ) {
	dllCompressedTexImage2DARB( target, level, internalformat, width, height, border, imageSize, data );
	QGL_CHECK_GL( "glCompressedTexImage2DARB" );
}

/* ---- qglCheckedCompressedTexImage3DARB  0x004C0210 ---- */
static void QGLAPI qglCheckedCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data ) {
	dllCompressedTexImage3DARB( target, level, internalformat, width, height, depth, border, imageSize, data );
	QGL_CHECK_GL( "glCompressedTexImage3DARB" );
}

/* ---- qglCheckedCompressedTexSubImage1D  0x004C03B0 ---- */
static void QGLAPI qglCheckedCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data ) {
	dllCompressedTexSubImage1D( target, level, xoffset, width, format, imageSize, data );
	QGL_CHECK_GL( "glCompressedTexSubImage1DARB" );
}

/* ---- qglCheckedCompressedTexSubImage2DARB  0x004C0360 ---- */
static void QGLAPI qglCheckedCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data ) {
	dllCompressedTexSubImage2DARB( target, level, xoffset, yoffset, width, height, format, imageSize, data );
	QGL_CHECK_GL( "glCompressedTexSubImage2DARB" );
}

/* ---- qglCheckedCompressedTexSubImage3DARB  0x004C0300 ---- */
static void QGLAPI qglCheckedCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data ) {
	dllCompressedTexSubImage3DARB( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
	QGL_CHECK_GL( "glCompressedTexSubImage3DARB" );
}

/* ---- qglCheckedDeleteBuffersARB  0x004C0460 ---- */
static void QGLAPI qglCheckedDeleteBuffersARB( GLsizei n, const GLuint *buffers ) {
	dllDeleteBuffersARB( n, buffers );
	QGL_CHECK_GL( "glDeleteBuffersARB" );
}

/* ---- qglCheckedDeleteFencesNV  0x004C0A60 ---- */
static void QGLAPI qglCheckedDeleteFencesNV( GLsizei n, const GLuint *fences ) {
	dllDeleteFencesNV( n, fences );
	QGL_CHECK_GL( "glDeleteFencesNV" );
}

/* ---- qglCheckedDeleteFragmentShaderATI  0x004C0F70 ---- */
static void QGLAPI qglCheckedDeleteFragmentShaderATI( GLuint id ) {
	dllDeleteFragmentShaderATI( id );
	QGL_CHECK_GL( "glDeleteFragmentShaderATI" );
}

/* ---- qglCheckedDeleteProgramsARB  0x004C1AF0 ---- */
static void QGLAPI qglCheckedDeleteProgramsARB( GLsizei n, const GLuint *programs ) {
	dllDeleteProgramsARB( n, programs );
	QGL_CHECK_GL( "glDeleteProgramsARB" );
}

/* ---- qglCheckedDisableVertexAttribArrayARB  0x004C1A50 ---- */
static void QGLAPI qglCheckedDisableVertexAttribArrayARB( GLuint index ) {
	dllDisableVertexAttribArrayARB( index );
	QGL_CHECK_GL( "glDisableVertexAttribArrayARB" );
}

/* ---- qglCheckedDrawElementArrayATI  0x004C0930 ---- */
static void QGLAPI qglCheckedDrawElementArrayATI( GLenum mode, GLsizei count ) {
	dllDrawElementArrayATI( mode, count );
	QGL_CHECK_GL( "glDrawElementArrayATI" );
}

/* ---- qglCheckedDrawRangeElementArrayATI  0x004C0960 ---- */
static void QGLAPI qglCheckedDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count ) {
	dllDrawRangeElementArrayATI( mode, start, end, count );
	QGL_CHECK_GL( "glDrawRangeElementArrayATI" );
}

/* ---- qglCheckedDrawRangeElementsEXT  0x004C01D0 ---- */
static void QGLAPI qglCheckedDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices ) {
	dllDrawRangeElementsEXT( mode, start, end, count, type, indices );
	QGL_CHECK_GL( "glDrawRangeElementsEXT" );
}

/* ---- qglCheckedElementPointerATI  0x004C0900 ---- */
static void QGLAPI qglCheckedElementPointerATI( GLenum type, const GLvoid *pointer ) {
	dllElementPointerATI( type, pointer );
	QGL_CHECK_GL( "glElementPointerATI" );
}

/* ---- qglCheckedEnableVertexAttribArrayARB  0x004C1A20 ---- */
static void QGLAPI qglCheckedEnableVertexAttribArrayARB( GLuint index ) {
	dllEnableVertexAttribArrayARB( index );
	QGL_CHECK_GL( "glEnableVertexAttribArrayARB" );
}

/* ---- qglCheckedEndFragmentShaderATI  0x004C0FC0 ---- */
static void QGLAPI qglCheckedEndFragmentShaderATI( void ) {
	dllEndFragmentShaderATI();
	QGL_CHECK_GL( "glEndFragmentShaderATI" );
}

/* ---- qglCheckedFinalCombinerInputNV  0x004C0D10 ---- */
static void QGLAPI qglCheckedFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
	dllFinalCombinerInputNV( variable, input, mapping, componentUsage );
	QGL_CHECK_GL( "glFinalCombinerInputNV" );
}

/* ---- qglCheckedFinishFenceNV  0x004C0B50 ---- */
static void QGLAPI qglCheckedFinishFenceNV( GLuint fence ) {
	dllFinishFenceNV( fence );
	QGL_CHECK_GL( "glFinishFenceNV" );
}

/* ---- qglCheckedFlushVertexArrayRangeNV  0x004C09A0 ---- */
static void QGLAPI qglCheckedFlushVertexArrayRangeNV( void ) {
	dllFlushVertexArrayRangeNV();
	QGL_CHECK_GL( "glFlushVertexArrayRangeNV" );
}

/* ---- qglCheckedFreeObjectBufferATI  0x004C0790 ---- */
static void QGLAPI qglCheckedFreeObjectBufferATI( GLuint buffer ) {
	dllFreeObjectBufferATI( buffer );
	QGL_CHECK_GL( "glFreeObjectBufferATI" );
}

/* ---- qglCheckedGenBuffersARB  0x004C0490 ---- */
static void QGLAPI qglCheckedGenBuffersARB( GLsizei n, GLuint *buffers ) {
	dllGenBuffersARB( n, buffers );
	QGL_CHECK_GL( "glGenBuffersARB" );
}

/* ---- qglCheckedGenFencesNV  0x004C0A90 ---- */
static void QGLAPI qglCheckedGenFencesNV( GLsizei n, GLuint *fences ) {
	dllGenFencesNV( n, fences );
	QGL_CHECK_GL( "glGenFencesNV" );
}

/* ---- qglCheckedGenFragmentShadersATI  0x004C0F10 ---- */
static GLuint QGLAPI qglCheckedGenFragmentShadersATI( GLuint range ) {
	GLuint result = dllGenFragmentShadersATI( range );
	QGL_CHECK_GL( "glGenFragmentShadersATI" );
	return result;
}

/* ---- qglCheckedGenProgramsARB  0x004C1B20 ---- */
static void QGLAPI qglCheckedGenProgramsARB( GLsizei n, GLuint *programs ) {
	dllGenProgramsARB( n, programs );
	QGL_CHECK_GL( "glGenProgramsARB" );
}

/* ---- qglCheckedGetArrayObjectfvATI  0x004C0800 ---- */
static void QGLAPI qglCheckedGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat *params ) {
	dllGetArrayObjectfvATI( array, pname, params );
	QGL_CHECK_GL( "glGetArrayObjectfvATI" );
}

/* ---- qglCheckedGetArrayObjectivATI  0x004C0830 ---- */
static void QGLAPI qglCheckedGetArrayObjectivATI( GLenum array, GLenum pname, GLint *params ) {
	dllGetArrayObjectivATI( array, pname, params );
	QGL_CHECK_GL( "glGetArrayObjectivATI" );
}

/* ---- qglCheckedGetBufferParameterivARB  0x004C0620 ---- */
static void QGLAPI qglCheckedGetBufferParameterivARB( GLenum target, GLenum pname, GLint *params ) {
	dllGetBufferParameterivARB( target, pname, params );
	QGL_CHECK_GL( "glGetBufferParameterivARB" );
}

/* ---- qglCheckedGetBufferPointervARB  0x004C0650 ---- */
static void QGLAPI qglCheckedGetBufferPointervARB( GLenum target, GLenum pname, GLvoid **params ) {
	dllGetBufferPointervARB( target, pname, params );
	QGL_CHECK_GL( "glGetBufferPointervARB" );
}

/* ---- qglCheckedGetBufferSubDataARB  0x004C0570 ---- */
static void QGLAPI qglCheckedGetBufferSubDataARB( GLenum target, GLint offset, GLsizei size, GLvoid *data ) {
	dllGetBufferSubDataARB( target, offset, size, data );
	QGL_CHECK_GL( "glGetBufferSubDataARB" );
}

/* ---- qglCheckedGetCombinerInputParameterfvNV  0x004C0D50 ---- */
static void QGLAPI qglCheckedGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params ) {
	dllGetCombinerInputParameterfvNV( stage, portion, variable, pname, params );
	QGL_CHECK_GL( "glGetCombinerInputParameterfvNV" );
}

/* ---- qglCheckedGetCombinerInputParameterivNV  0x004C0D90 ---- */
static void QGLAPI qglCheckedGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params ) {
	dllGetCombinerInputParameterivNV( stage, portion, variable, pname, params );
	QGL_CHECK_GL( "glGetCombinerInputParameterivNV" );
}

/* ---- qglCheckedGetCombinerOutputParameterfvNV  0x004C0DD0 ---- */
static void QGLAPI qglCheckedGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat *params ) {
	dllGetCombinerOutputParameterfvNV( stage, portion, pname, params );
	QGL_CHECK_GL( "glGetCombinerOutputParameterfvNV" );
}

/* ---- qglCheckedGetCombinerOutputParameterivNV  0x004C0E10 ---- */
static void QGLAPI qglCheckedGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint *params ) {
	dllGetCombinerOutputParameterivNV( stage, portion, pname, params );
	QGL_CHECK_GL( "glGetCombinerOutputParameterivNV" );
}

/* ---- qglCheckedGetCombinerStageParameterfvNV  0x004C0EE0 ---- */
static void QGLAPI qglCheckedGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat *params ) {
	dllGetCombinerStageParameterfvNV( stage, pname, params );
	QGL_CHECK_GL( "glGetCombinerStageParameterfvNV" );
}

/* ---- qglCheckedGetCompressedTexImage  0x004C0400 ---- */
static void QGLAPI qglCheckedGetCompressedTexImage( GLenum target, GLint level, GLvoid *img ) {
	dllGetCompressedTexImage( target, level, img );
	QGL_CHECK_GL( "glGetCompressedTexImageARB" );
}

/* ---- qglCheckedGetFenceivNV  0x004C0B20 ---- */
static void QGLAPI qglCheckedGetFenceivNV( GLuint fence, GLenum pname, GLint *params ) {
	dllGetFenceivNV( fence, pname, params );
	QGL_CHECK_GL( "glGetFenceivNV" );
}

/* ---- qglCheckedGetFinalCombinerInputParameterfvNV  0x004C0E50 ---- */
static void QGLAPI qglCheckedGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat *params ) {
	dllGetFinalCombinerInputParameterfvNV( variable, pname, params );
	QGL_CHECK_GL( "glGetFinalCombinerInputParameterfvNV" );
}

/* ---- qglCheckedGetFinalCombinerInputParameterivNV  0x004C0E80 ---- */
static void QGLAPI qglCheckedGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint *params ) {
	dllGetFinalCombinerInputParameterivNV( variable, pname, params );
	QGL_CHECK_GL( "glGetFinalCombinerInputParameterivNV" );
}

/* ---- qglCheckedGetObjectBufferfvATI  0x004C0730 ---- */
static void QGLAPI qglCheckedGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat *params ) {
	dllGetObjectBufferfvATI( buffer, pname, params );
	QGL_CHECK_GL( "glGetObjectBufferfvATI" );
}

/* ---- qglCheckedGetObjectBufferivATI  0x004C0760 ---- */
static void QGLAPI qglCheckedGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint *params ) {
	dllGetObjectBufferivATI( buffer, pname, params );
	QGL_CHECK_GL( "glGetObjectBufferivATI" );
}

/* ---- qglCheckedGetProgramEnvParameterdvARB  0x004C1D80 ---- */
static void QGLAPI qglCheckedGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble *params ) {
	dllGetProgramEnvParameterdvARB( target, index, params );
	QGL_CHECK_GL( "glGetProgramEnvParameterdvARB" );
}

/* ---- qglCheckedGetProgramEnvParameterfvARB  0x004C1D50 ---- */
static void QGLAPI qglCheckedGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat *params ) {
	dllGetProgramEnvParameterfvARB( target, index, params );
	QGL_CHECK_GL( "glGetProgramEnvParameterfvARB" );
}

/* ---- qglCheckedGetProgramLocalParameterdvARB  0x004C1DE0 ---- */
static void QGLAPI qglCheckedGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble *params ) {
	dllGetProgramLocalParameterdvARB( target, index, params );
	QGL_CHECK_GL( "glGetProgramLocalParameterdvARB" );
}

/* ---- qglCheckedGetProgramLocalParameterfvARB  0x004C1DB0 ---- */
static void QGLAPI qglCheckedGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat *params ) {
	dllGetProgramLocalParameterfvARB( target, index, params );
	QGL_CHECK_GL( "glGetProgramLocalParameterfvARB" );
}

/* ---- qglCheckedGetProgramStringARB  0x004C1E40 ---- */
static void QGLAPI qglCheckedGetProgramStringARB( GLenum target, GLenum pname, GLvoid *string ) {
	dllGetProgramStringARB( target, pname, string );
	QGL_CHECK_GL( "glGetProgramStringARB" );
}

/* ---- qglCheckedGetProgramivARB  0x004C1E10 ---- */
static void QGLAPI qglCheckedGetProgramivARB( GLenum target, GLenum pname, GLint *params ) {
	dllGetProgramivARB( target, pname, params );
	QGL_CHECK_GL( "glGetProgramivARB" );
}

/* ---- qglCheckedGetVariantArrayObjectfvATI  0x004C08A0 ---- */
static void QGLAPI qglCheckedGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat *params ) {
	dllGetVariantArrayObjectfvATI( id, pname, params );
	QGL_CHECK_GL( "glGetVariantArrayObjectfvATI" );
}

/* ---- qglCheckedGetVariantArrayObjectivATI  0x004C08D0 ---- */
static void QGLAPI qglCheckedGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint *params ) {
	dllGetVariantArrayObjectivATI( id, pname, params );
	QGL_CHECK_GL( "glGetVariantArrayObjectivATI" );
}

/* ---- qglCheckedGetVertexAttribPointervARB  0x004C1F00 ---- */
static void QGLAPI qglCheckedGetVertexAttribPointervARB( GLuint index, GLenum pname, GLvoid **pointer ) {
	dllGetVertexAttribPointervARB( index, pname, pointer );
	QGL_CHECK_GL( "glGetVertexAttribPointervARB" );
}

/* ---- qglCheckedGetVertexAttribdvARB  0x004C1E70 ---- */
static void QGLAPI qglCheckedGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble *params ) {
	dllGetVertexAttribdvARB( index, pname, params );
	QGL_CHECK_GL( "glGetVertexAttribdvARB" );
}

/* ---- qglCheckedGetVertexAttribfvARB  0x004C1EA0 ---- */
static void QGLAPI qglCheckedGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat *params ) {
	dllGetVertexAttribfvARB( index, pname, params );
	QGL_CHECK_GL( "glGetVertexAttribfvARB" );
}

/* ---- qglCheckedGetVertexAttribivARB  0x004C1ED0 ---- */
static void QGLAPI qglCheckedGetVertexAttribivARB( GLuint index, GLenum pname, GLint *params ) {
	dllGetVertexAttribivARB( index, pname, params );
	QGL_CHECK_GL( "glGetVertexAttribivARB" );
}

/* ---- qglCheckedIsBufferARB  0x004C04C0 ---- */
static GLboolean QGLAPI qglCheckedIsBufferARB( GLuint buffer ) {
	GLboolean result = dllIsBufferARB( buffer );
	QGL_CHECK_GL( "glIsBufferARB" );
	return result;
}

/* ---- qglCheckedIsFenceNV  0x004C0AC0 ---- */
static GLboolean QGLAPI qglCheckedIsFenceNV( GLuint fence ) {
	GLboolean result = dllIsFenceNV( fence );
	QGL_CHECK_GL( "glIsFenceNV" );
	return result;
}

/* ---- qglCheckedIsObjectBufferATI  0x004C06C0 ---- */
static GLboolean QGLAPI qglCheckedIsObjectBufferATI( GLuint buffer ) {
	GLboolean result = dllIsObjectBufferATI( buffer );
	QGL_CHECK_GL( "glIsObjectBufferATI" );
	return result;
}

/* ---- qglCheckedIsProgramARB  0x004C1F30 ---- */
static GLboolean QGLAPI qglCheckedIsProgramARB( GLuint program ) {
	GLboolean result = dllIsProgramARB( program );
	QGL_CHECK_GL( "glIsProgramARB" );
	return result;
}

/* ---- qglCheckedLockArraysEXT  0x004C0120 ---- */
static void QGLAPI qglCheckedLockArraysEXT( GLint first, GLsizei count ) {
	dllLockArraysEXT( first, count );
	QGL_CHECK_GL( "glLockArraysEXT" );
}

/* ---- qglCheckedMapBufferARB  0x004C05B0 ---- */
static GLvoid * QGLAPI qglCheckedMapBufferARB( GLenum target, GLenum access ) {
	GLvoid *result = dllMapBufferARB( target, access );
	QGL_CHECK_GL( "glMapBufferARB" );
	return result;
}

/* ---- qglCheckedNewObjectBufferATI  0x004C0680 ---- */
static GLuint QGLAPI qglCheckedNewObjectBufferATI( GLsizei size, const GLvoid *pointer, GLenum usage ) {
	GLuint result = dllNewObjectBufferATI( size, pointer, usage );
	QGL_CHECK_GL( "glNewObjectBufferATI" );
	return result;
}

/* ---- qglCheckedPNTrianglesfATI  0x004C01A0 ---- */
static void QGLAPI qglCheckedPNTrianglesfATI( GLenum pname, GLfloat param ) {
	dllPNTrianglesfATI( pname, param );
	QGL_CHECK_GL( "glPNTrianglesfATI" );
}

/* ---- qglCheckedPNTrianglesiATI  0x004C0170 ---- */
static void QGLAPI qglCheckedPNTrianglesiATI( GLenum pname, GLint param ) {
	dllPNTrianglesiATI( pname, param );
	QGL_CHECK_GL( "glPNTrianglesiATI" );
}

/* ---- qglCheckedPassTexCoordATI  0x004C0FE0 ---- */
static void QGLAPI qglCheckedPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle ) {
	dllPassTexCoordATI( dst, coord, swizzle );
	QGL_CHECK_GL( "glPassTexCoordATI" );
}

/* ---- qglCheckedProgramEnvParameter4dARB  0x004C1B90 ---- */
static void QGLAPI qglCheckedProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	dllProgramEnvParameter4dARB( target, index, x, y, z, w );
	QGL_CHECK_GL( "glProgramEnvParameter4dARB" );
}

/* ---- qglCheckedProgramEnvParameter4dvARB  0x004C1C20 ---- */
static void QGLAPI qglCheckedProgramEnvParameter4dvARB( GLenum target, GLuint index, const GLdouble *params ) {
	dllProgramEnvParameter4dvARB( target, index, params );
	QGL_CHECK_GL( "glProgramEnvParameter4dvARB" );
}

/* ---- qglCheckedProgramEnvParameter4fARB  0x004C1B50 ---- */
static void QGLAPI qglCheckedProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	dllProgramEnvParameter4fARB( target, index, x, y, z, w );
	QGL_CHECK_GL( "glProgramEnvParameter4fARB" );
}

/* ---- qglCheckedProgramEnvParameter4fvARB  0x004C1BF0 ---- */
static void QGLAPI qglCheckedProgramEnvParameter4fvARB( GLenum target, GLuint index, const GLfloat *params ) {
	dllProgramEnvParameter4fvARB( target, index, params );
	QGL_CHECK_GL( "glProgramEnvParameter4fvARB" );
}

/* ---- qglCheckedProgramLocalParameter4dARB  0x004C1C90 ---- */
static void QGLAPI qglCheckedProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	dllProgramLocalParameter4dARB( target, index, x, y, z, w );
	QGL_CHECK_GL( "glProgramLocalParameter4dARB" );
}

/* ---- qglCheckedProgramLocalParameter4dvARB  0x004C1D20 ---- */
static void QGLAPI qglCheckedProgramLocalParameter4dvARB( GLenum target, GLuint index, const GLdouble *params ) {
	dllProgramLocalParameter4dvARB( target, index, params );
	QGL_CHECK_GL( "glProgramLocalParameter4dvARB" );
}

/* ---- qglCheckedProgramLocalParameter4fARB  0x004C1C50 ---- */
static void QGLAPI qglCheckedProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	dllProgramLocalParameter4fARB( target, index, x, y, z, w );
	QGL_CHECK_GL( "glProgramLocalParameter4fARB" );
}

/* ---- qglCheckedProgramLocalParameter4fvARB  0x004C1CF0 ---- */
static void QGLAPI qglCheckedProgramLocalParameter4fvARB( GLenum target, GLuint index, const GLfloat *params ) {
	dllProgramLocalParameter4fvARB( target, index, params );
	QGL_CHECK_GL( "glProgramLocalParameter4fvARB" );
}

/* ---- qglCheckedProgramStringARB  0x004C1A80 ---- */
static void QGLAPI qglCheckedProgramStringARB( GLenum target, GLenum format, GLsizei len, const GLvoid *string ) {
	dllProgramStringARB( target, format, len, string );
	QGL_CHECK_GL( "glProgramStringARB" );
}

/* ---- qglCheckedSampleMapATI  0x004C1010 ---- */
static void QGLAPI qglCheckedSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle ) {
	dllSampleMapATI( dst, interp, swizzle );
	QGL_CHECK_GL( "glSampleMapATI" );
}

/* ---- qglCheckedSetFenceNV  0x004C0B80 ---- */
static void QGLAPI qglCheckedSetFenceNV( GLuint fence, GLenum condition ) {
	dllSetFenceNV( fence, condition );
	QGL_CHECK_GL( "glSetFenceNV" );
}

/* ---- qglCheckedSetFragmentShaderConstantATI  0x004C1250 ---- */
static void QGLAPI qglCheckedSetFragmentShaderConstantATI( GLuint dst, const GLfloat *value ) {
	dllSetFragmentShaderConstantATI( dst, value );
	QGL_CHECK_GL( "glSetFragmentShaderConstantATI" );
}

/* ---- qglCheckedTestFenceNV  0x004C0AF0 ---- */
static GLboolean QGLAPI qglCheckedTestFenceNV( GLuint fence ) {
	GLboolean result = dllTestFenceNV( fence );
	QGL_CHECK_GL( "glTestFenceNV" );
	return result;
}

/* ---- qglCheckedUnlockArraysEXT  0x004C0150 ---- */
static void QGLAPI qglCheckedUnlockArraysEXT( void ) {
	dllUnlockArraysEXT();
	QGL_CHECK_GL( "glUnlockArraysEXT" );
}

/* ---- qglCheckedUnmapBufferARB  0x004C05F0 ---- */
static GLboolean QGLAPI qglCheckedUnmapBufferARB( GLenum target ) {
	GLboolean result = dllUnmapBufferARB( target );
	QGL_CHECK_GL( "glUnmapBufferARB" );
	return result;
}

/* ---- qglCheckedUpdateObjectBufferATI  0x004C06F0 ---- */
static void QGLAPI qglCheckedUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, const GLvoid *pointer, GLenum preserve ) {
	dllUpdateObjectBufferATI( buffer, offset, size, pointer, preserve );
	QGL_CHECK_GL( "glUpdateObjectBufferATI" );
}

/* ---- qglCheckedVariantArrayObjectATI  0x004C0860 ---- */
static void QGLAPI qglCheckedVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
	dllVariantArrayObjectATI( id, type, stride, buffer, offset );
	QGL_CHECK_GL( "glVariantArrayObjectATI" );
}

/* ---- qglCheckedVertexArrayRangeNV  0x004C09C0 ---- */
static void QGLAPI qglCheckedVertexArrayRangeNV( GLsizei length, const GLvoid *pointer ) {
	dllVertexArrayRangeNV( length, pointer );
	QGL_CHECK_GL( "glVertexArrayRangeNV" );
}

/* ---- qglCheckedVertexAttrib1dARB  0x004C12E0 ---- */
static void QGLAPI qglCheckedVertexAttrib1dARB( GLuint index, GLdouble x ) {
	dllVertexAttrib1dARB( index, x );
	QGL_CHECK_GL( "glVertexAttrib1dARB" );
}

/* ---- qglCheckedVertexAttrib1dvARB  0x004C15F0 ---- */
static void QGLAPI qglCheckedVertexAttrib1dvARB( GLuint index, const GLdouble *v ) {
	dllVertexAttrib1dvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib1dvARB" );
}

/* ---- qglCheckedVertexAttrib1fARB  0x004C12B0 ---- */
static void QGLAPI qglCheckedVertexAttrib1fARB( GLuint index, GLfloat x ) {
	dllVertexAttrib1fARB( index, x );
	QGL_CHECK_GL( "glVertexAttrib1fARB" );
}

/* ---- qglCheckedVertexAttrib1fvARB  0x004C15C0 ---- */
static void QGLAPI qglCheckedVertexAttrib1fvARB( GLuint index, const GLfloat *v ) {
	dllVertexAttrib1fvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib1fvARB" );
}

/* ---- qglCheckedVertexAttrib1sARB  0x004C1280 ---- */
static void QGLAPI qglCheckedVertexAttrib1sARB( GLuint index, GLshort x ) {
	dllVertexAttrib1sARB( index, x );
	QGL_CHECK_GL( "glVertexAttrib1sARB" );
}

/* ---- qglCheckedVertexAttrib1svARB  0x004C1590 ---- */
static void QGLAPI qglCheckedVertexAttrib1svARB( GLuint index, const GLshort *v ) {
	dllVertexAttrib1svARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib1svARB" );
}

/* ---- qglCheckedVertexAttrib2dARB  0x004C1370 ---- */
static void QGLAPI qglCheckedVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y ) {
	dllVertexAttrib2dARB( index, x, y );
	QGL_CHECK_GL( "glVertexAttrib2dARB" );
}

/* ---- qglCheckedVertexAttrib2dvARB  0x004C1680 ---- */
static void QGLAPI qglCheckedVertexAttrib2dvARB( GLuint index, const GLdouble *v ) {
	dllVertexAttrib2dvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib2dvARB" );
}

/* ---- qglCheckedVertexAttrib2fARB  0x004C1340 ---- */
static void QGLAPI qglCheckedVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y ) {
	dllVertexAttrib2fARB( index, x, y );
	QGL_CHECK_GL( "glVertexAttrib2fARB" );
}

/* ---- qglCheckedVertexAttrib2fvARB  0x004C1650 ---- */
static void QGLAPI qglCheckedVertexAttrib2fvARB( GLuint index, const GLfloat *v ) {
	dllVertexAttrib2fvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib2fvARB" );
}

/* ---- qglCheckedVertexAttrib2sARB  0x004C1310 ---- */
static void QGLAPI qglCheckedVertexAttrib2sARB( GLuint index, GLshort x, GLshort y ) {
	dllVertexAttrib2sARB( index, x, y );
	QGL_CHECK_GL( "glVertexAttrib2sARB" );
}

/* ---- qglCheckedVertexAttrib2svARB  0x004C1620 ---- */
static void QGLAPI qglCheckedVertexAttrib2svARB( GLuint index, const GLshort *v ) {
	dllVertexAttrib2svARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib2svARB" );
}

/* ---- qglCheckedVertexAttrib3dARB  0x004C1430 ---- */
static void QGLAPI qglCheckedVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
	dllVertexAttrib3dARB( index, x, y, z );
	QGL_CHECK_GL( "glVertexAttrib3dARB" );
}

/* ---- qglCheckedVertexAttrib3dvARB  0x004C1710 ---- */
static void QGLAPI qglCheckedVertexAttrib3dvARB( GLuint index, const GLdouble *v ) {
	dllVertexAttrib3dvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib3dvARB" );
}

/* ---- qglCheckedVertexAttrib3fARB  0x004C13F0 ---- */
static void QGLAPI qglCheckedVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
	dllVertexAttrib3fARB( index, x, y, z );
	QGL_CHECK_GL( "glVertexAttrib3fARB" );
}

/* ---- qglCheckedVertexAttrib3fvARB  0x004C16E0 ---- */
static void QGLAPI qglCheckedVertexAttrib3fvARB( GLuint index, const GLfloat *v ) {
	dllVertexAttrib3fvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib3fvARB" );
}

/* ---- qglCheckedVertexAttrib3sARB  0x004C13B0 ---- */
static void QGLAPI qglCheckedVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z ) {
	dllVertexAttrib3sARB( index, x, y, z );
	QGL_CHECK_GL( "glVertexAttrib3sARB" );
}

/* ---- qglCheckedVertexAttrib3svARB  0x004C16B0 ---- */
static void QGLAPI qglCheckedVertexAttrib3svARB( GLuint index, const GLshort *v ) {
	dllVertexAttrib3svARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib3svARB" );
}

/* ---- qglCheckedVertexAttrib4NbvARB  0x004C18C0 ---- */
static void QGLAPI qglCheckedVertexAttrib4NbvARB( GLuint index, const GLbyte *v ) {
	dllVertexAttrib4NbvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4NbvARB" );
}

/* ---- qglCheckedVertexAttrib4NivARB  0x004C1920 ---- */
static void QGLAPI qglCheckedVertexAttrib4NivARB( GLuint index, const GLint *v ) {
	dllVertexAttrib4NivARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4NivARB" );
}

/* ---- qglCheckedVertexAttrib4NsvARB  0x004C18F0 ---- */
static void QGLAPI qglCheckedVertexAttrib4NsvARB( GLuint index, const GLshort *v ) {
	dllVertexAttrib4NsvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4NsvARB" );
}

/* ---- qglCheckedVertexAttrib4NubARB  0x004C1550 ---- */
static void QGLAPI qglCheckedVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
	dllVertexAttrib4NubARB( index, x, y, z, w );
	QGL_CHECK_GL( "glVertexAttrib4NubARB" );
}

/* ---- qglCheckedVertexAttrib4NubvARB  0x004C1950 ---- */
static void QGLAPI qglCheckedVertexAttrib4NubvARB( GLuint index, const GLubyte *v ) {
	dllVertexAttrib4NubvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4NubvARB" );
}

/* ---- qglCheckedVertexAttrib4NuivARB  0x004C19B0 ---- */
static void QGLAPI qglCheckedVertexAttrib4NuivARB( GLuint index, const GLuint *v ) {
	dllVertexAttrib4NuivARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4NuivARB" );
}

/* ---- qglCheckedVertexAttrib4NusvARB  0x004C1980 ---- */
static void QGLAPI qglCheckedVertexAttrib4NusvARB( GLuint index, const GLushort *v ) {
	dllVertexAttrib4NusvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4NusvARB" );
}

/* ---- qglCheckedVertexAttrib4bvARB  0x004C1740 ---- */
static void QGLAPI qglCheckedVertexAttrib4bvARB( GLuint index, const GLbyte *v ) {
	dllVertexAttrib4bvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4bvARB" );
}

/* ---- qglCheckedVertexAttrib4dARB  0x004C1500 ---- */
static void QGLAPI qglCheckedVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	dllVertexAttrib4dARB( index, x, y, z, w );
	QGL_CHECK_GL( "glVertexAttrib4dARB" );
}

/* ---- qglCheckedVertexAttrib4dvARB  0x004C1890 ---- */
static void QGLAPI qglCheckedVertexAttrib4dvARB( GLuint index, const GLdouble *v ) {
	dllVertexAttrib4dvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4dvARB" );
}

/* ---- qglCheckedVertexAttrib4fARB  0x004C14C0 ---- */
static void QGLAPI qglCheckedVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	dllVertexAttrib4fARB( index, x, y, z, w );
	QGL_CHECK_GL( "glVertexAttrib4fARB" );
}

/* ---- qglCheckedVertexAttrib4fvARB  0x004C1860 ---- */
static void QGLAPI qglCheckedVertexAttrib4fvARB( GLuint index, const GLfloat *v ) {
	dllVertexAttrib4fvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4fvARB" );
}

/* ---- qglCheckedVertexAttrib4ivARB  0x004C17A0 ---- */
static void QGLAPI qglCheckedVertexAttrib4ivARB( GLuint index, const GLint *v ) {
	dllVertexAttrib4ivARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4ivARB" );
}

/* ---- qglCheckedVertexAttrib4sARB  0x004C1480 ---- */
static void QGLAPI qglCheckedVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
	dllVertexAttrib4sARB( index, x, y, z, w );
	QGL_CHECK_GL( "glVertexAttrib4sARB" );
}

/* ---- qglCheckedVertexAttrib4svARB  0x004C1770 ---- */
static void QGLAPI qglCheckedVertexAttrib4svARB( GLuint index, const GLshort *v ) {
	dllVertexAttrib4svARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4svARB" );
}

/* ---- qglCheckedVertexAttrib4ubvARB  0x004C17D0 ---- */
static void QGLAPI qglCheckedVertexAttrib4ubvARB( GLuint index, const GLubyte *v ) {
	dllVertexAttrib4ubvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4ubvARB" );
}

/* ---- qglCheckedVertexAttrib4uivARB  0x004C1830 ---- */
static void QGLAPI qglCheckedVertexAttrib4uivARB( GLuint index, const GLuint *v ) {
	dllVertexAttrib4uivARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4uivARB" );
}

/* ---- qglCheckedVertexAttrib4usvARB  0x004C1800 ---- */
static void QGLAPI qglCheckedVertexAttrib4usvARB( GLuint index, const GLushort *v ) {
	dllVertexAttrib4usvARB( index, v );
	QGL_CHECK_GL( "glVertexAttrib4usvARB" );
}

/* ---- qglCheckedVertexAttribPointerARB  0x004C19E0 ---- */
static void QGLAPI qglCheckedVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) {
	dllVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
	QGL_CHECK_GL( "glVertexAttribPointerARB" );
}

/* ---- qwglCheckedAllocateMemoryNV  0x004C09F0 ---- */
static void * WINAPI qwglCheckedAllocateMemoryNV( int size, float readfreq, float writefreq, float priority ) {
	void *result = dllwglAllocateMemoryNV( size, readfreq, writefreq, priority );
	QGL_CHECK_WGL( "wglAllocateMemoryNV" );
	return result;
}

/* ---- qwglCheckedFreeMemoryNV  0x004C0A30 ---- */
static void WINAPI qwglCheckedFreeMemoryNV( void *pointer ) {
	dllwglFreeMemoryNV( pointer );
	QGL_CHECK_WGL( "wglFreeMemoryNV" );
}

/* ---- qwglCheckedSwapIntervalEXT  0x004C67F0 ---- */
static BOOL WINAPI qwglCheckedSwapIntervalEXT( int interval ) {
	BOOL result = dllwglSwapIntervalEXT( interval );
	QGL_CHECK_WGL( "wglSwapIntervalEXT" );
	return result;
}

/* ---- qglCheckedMultiTexCoord2fARB  0x004C0090 ---- */
static void QGLAPI qglCheckedMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) {
	dllMultiTexCoord2fARB( target, s, t );
	QGL_CHECK_GL( "glMultiTexCoord2fARB" );
}

/* ---- qwglCheckedGetExtensionsStringEXT  0x004C67C0 ---- */
static const char * WINAPI qwglCheckedGetExtensionsStringEXT( void ) {
	const char *result = dllwglGetExtensionsStringEXT();
	QGL_CHECK_WGL( "wglGetExtensionsStringEXT" );
	return result;
}

/* ---- qwglCheckedGetPixelFormatAttribivARB  0x004C6820 ---- */
static BOOL WINAPI qwglCheckedGetPixelFormatAttribivARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues ) {
	BOOL result = dllwglGetPixelFormatAttribivARB( hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues );
	QGL_CHECK_WGL( "wglGetPixelFormatAttribivARB" );
	return result;
}

/* ---- qwglCheckedGetPixelFormatAttribfvARB  0x004C6870 ---- */
static BOOL WINAPI qwglCheckedGetPixelFormatAttribfvARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues ) {
	BOOL result = dllwglGetPixelFormatAttribfvARB( hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues );
	QGL_CHECK_WGL( "wglGetPixelFormatAttribfvARB" );
	return result;
}

/* ---- qwglCheckedChoosePixelFormatARB  0x004C68C0 ---- */
static BOOL WINAPI qwglCheckedChoosePixelFormatARB( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats ) {
	BOOL result = dllwglChoosePixelFormatARB( hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats );
	QGL_CHECK_WGL( "wglChoosePixelFormatARB" );
	return result;
}

/* ---- qwglCheckedCreatePbufferARB  0x004C6910 ---- */
static HPBUFFERARB WINAPI qwglCheckedCreatePbufferARB( HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList ) {
	HPBUFFERARB result = dllwglCreatePbufferARB( hdc, iPixelFormat, iWidth, iHeight, piAttribList );
	QGL_CHECK_WGL( "wglCreatePbufferARB" );
	return result;
}

/* ---- qwglCheckedGetPbufferDCARB  0x004C6950 ---- */
static HDC WINAPI qwglCheckedGetPbufferDCARB( HPBUFFERARB hPbuffer ) {
	HDC result = dllwglGetPbufferDCARB( hPbuffer );
	QGL_CHECK_WGL( "wglGetPbufferDCARB" );
	return result;
}

/* ---- qwglCheckedReleasePbufferDCARB  0x004C6980 ---- */
static int WINAPI qwglCheckedReleasePbufferDCARB( HPBUFFERARB hPbuffer, HDC hDC ) {
	int result = dllwglReleasePbufferDCARB( hPbuffer, hDC );
	QGL_CHECK_WGL( "wglReleasePbufferDCARB" );
	return result;
}

/* ---- qwglCheckedDestroyPbufferARB  0x004C69C0 ---- */
static BOOL WINAPI qwglCheckedDestroyPbufferARB( HPBUFFERARB hPbuffer ) {
	BOOL result = dllwglDestroyPbufferARB( hPbuffer );
	QGL_CHECK_WGL( "wglDestroyPbufferARB" );
	return result;
}

/* ---- qwglCheckedQueryPbufferARB  0x004C69F0 ---- */
static BOOL WINAPI qwglCheckedQueryPbufferARB( HPBUFFERARB hPbuffer, int iAttribute, int *piValue ) {
	BOOL result = dllwglQueryPbufferARB( hPbuffer, iAttribute, piValue );
	QGL_CHECK_WGL( "wglQueryPbufferARB" );
	return result;
}

/* ---- qwglCheckedBindTexImageARB  0x004C6A30 ---- */
static BOOL WINAPI qwglCheckedBindTexImageARB( HPBUFFERARB hPbuffer, int iBuffer ) {
	BOOL result = dllwglBindTexImageARB( hPbuffer, iBuffer );
	QGL_CHECK_WGL( "wglBindTexImageARB" );
	return result;
}

/* ---- qwglCheckedReleaseTexImageARB  0x004C6A70 ---- */
static BOOL WINAPI qwglCheckedReleaseTexImageARB( HPBUFFERARB hPbuffer, int iBuffer ) {
	BOOL result = dllwglReleaseTexImageARB( hPbuffer, iBuffer );
	QGL_CHECK_WGL( "wglReleaseTexImageARB" );
	return result;
}

/* ---- qwglCheckedSetPbufferAttribARB  0x004C6AB0 ---- */
static BOOL WINAPI qwglCheckedSetPbufferAttribARB( HPBUFFERARB hPbuffer, const int *piAttribList ) {
	BOOL result = dllwglSetPbufferAttribARB( hPbuffer, piAttribList );
	QGL_CHECK_WGL( "wglSetPbufferAttribARB" );
	return result;
}

/* 0x00EA2CD8 and 0x00EA2CDC. */
static qboolean qgl_errorCheckingEnabled;
static qboolean qgl_drawProfilingEnabled;

/* ---- QGL_EnableErrorChecking  0x004CAEB0 ---- */
void QGL_EnableErrorChecking( qboolean enable ) {
	if ( qgl_errorCheckingEnabled ) {
		if ( enable ) {
			return;
		}
	} else if ( !enable ) {
		return;
	}
	qgl_errorCheckingEnabled = enable;

	/* Retail walks 508 slots: the 495 of qgl_entries.h and the 13 extra ones
	   (wrappers 0x004C0090, 0x004C67C0 and 0x004C6820-0x004C6AB0). */
	if ( enable ) {
		/* 0x004CAED9: every non-NULL qgl pointer -> its checked wrapper. */
#define QGL_EXTRA_ARM_GL( n )   if ( qgl##n )  { qgl##n  = qglChecked##n;  }
#define QGL_EXTRA_ARM_WGL( n )  if ( qwgl##n ) { qwgl##n = qwglChecked##n; }
		QGL_LOG_EXT_ENTRIES( QGL_EXTRA_ARM_GL, QGL_EXTRA_ARM_WGL )
#define QGL_CORE_GL( n )    if ( qgl##n )  { qgl##n  = qglChecked##n;  }
#define QGL_CORE_WGL( n )   if ( qwgl##n ) { qwgl##n = qwglChecked##n; }
#include "../renderer/qgl_entries.h"
		QGL_LOG_TAIL_ENTRIES( QGL_EXTRA_ARM_GL, QGL_EXTRA_ARM_WGL )
#undef QGL_EXTRA_ARM_GL
#undef QGL_EXTRA_ARM_WGL
	} else {
		/* 0x004CD492: restore every qgl pointer from its driver slot. */
#define QGL_EXTRA_RESTORE_GL( n )   qgl##n  = dll##n;
#define QGL_EXTRA_RESTORE_WGL( n )  qwgl##n = dllwgl##n;
		QGL_LOG_EXT_ENTRIES( QGL_EXTRA_RESTORE_GL, QGL_EXTRA_RESTORE_WGL )
#define QGL_CORE_GL( n )    qgl##n  = dll##n;
#define QGL_CORE_WGL( n )   qwgl##n = dllwgl##n;
#include "../renderer/qgl_entries.h"
		QGL_LOG_TAIL_ENTRIES( QGL_EXTRA_RESTORE_GL, QGL_EXTRA_RESTORE_WGL )
	}
}

/* 0x00EA2CE0: QGL_ProfileDrawElements' index scratch, 0x300000 bytes (it ends
   at 0x011A2CE0, CapToString's buffer); 0x180000 is also the wrapper's upper
   bound on r_profileDrawElements. */
static unsigned short qgl_profileIndexes[0x180000];
/* 0x01407304 and 0x01407308: the tr_frameCount of the last profiled draw and
   the number of profiled draws issued in that frame. */
static int qgl_profileFrame;
static int qgl_profileDrawCount;

/* ---- QGL_ProfileDrawElements  0x004CAC20 ----
   r_profileDrawElements 2..32 repeats each index list that many times in one
   draw.  33..0x180000 draws exactly 3 * r_profileDrawElements indexes, tiling
   the list and cutting the last copy short.  Any other value draws only the
   first 3 indexes.  After 350 profiled draws in a frame, nothing more is drawn.
   backEnd_pc_drawnIndexCount is corrected by the difference.  Indexes are
   always copied as 16-bit, whatever `type` is.  Retail does not bound the
   tiled copy: above 0x80000, 3 * r_profileDrawElements runs past the scratch. */
static void QGLAPI QGL_ProfileDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
	int profile;
	int drawCount;
	int i;

	if ( backEnd_projection2D ) {
		dllDrawElements( mode, count, type, indices );
		return;
	}

	profile = r_profileDrawElements->integer;
	if ( profile < 2 || profile > 0x180000 ) {
		dllDrawElements( mode, 3, type, indices );
		backEnd_pc_drawnIndexCount += 3 - count;
		return;
	}

	if ( qgl_profileFrame != tr_frameCount ) {
		drawCount = 0;
		qgl_profileFrame = tr_frameCount;
	} else {
		drawCount = qgl_profileDrawCount;
	}
	qgl_profileDrawCount = ++drawCount;
	if ( drawCount > 350 ) {
		backEnd_pc_drawnIndexCount -= count;
		return;
	}

	if ( profile > 32 ) {
		int remaining = 3 * profile;
		int copied = 0;

		while ( count <= remaining ) {
			memcpy( &qgl_profileIndexes[copied], indices, count * 2 );
			remaining -= count;
			copied += count;
		}
		if ( remaining ) {
			memcpy( &qgl_profileIndexes[copied], indices, remaining * 2 );
		}
		dllDrawElements( mode, 3 * r_profileDrawElements->integer, type, qgl_profileIndexes );
		backEnd_pc_drawnIndexCount += 3 * r_profileDrawElements->integer - count;
	} else {
		/* 0x004CAD50: the loop test re-reads the cvar after every copy. */
		for ( i = 0; i < r_profileDrawElements->integer; i++ ) {
			memcpy( &qgl_profileIndexes[i * count], indices, count * 2 );
		}
		dllDrawElements( mode, count * r_profileDrawElements->integer, type, qgl_profileIndexes );
		backEnd_pc_drawnIndexCount += count * ( r_profileDrawElements->integer - 1 );
	}
}

/* ---- QGL_ProfileDrawElementArrayATI  0x004CAE00 ----
   Unlike QGL_ProfileDrawElements this has no upper bound and no per-frame cap.
   Any r_profileDrawElements of 2 or more skips the draw. */
static void QGLAPI QGL_ProfileDrawElementArrayATI( GLenum mode, GLsizei count ) {
	if ( backEnd_projection2D ) {
		dllDrawElementArrayATI( mode, count );
		return;
	}

	if ( r_profileDrawElements->integer >= 2 ) {
		backEnd_pc_drawnIndexCount -= count;
	} else {
		dllDrawElementArrayATI( mode, 3 );
		backEnd_pc_drawnIndexCount += 3 - count;
	}
}

/* ---- QGL_EnableDrawProfiling  0x004CAE60 ----
   Retail inlines this into GLimp_EndFrame (0x0050A6A9). */
void QGL_EnableDrawProfiling( qboolean enable ) {
	if ( ( qgl_drawProfilingEnabled && enable ) || ( !qgl_drawProfilingEnabled && !enable ) ) {
		return;
	}
	qgl_drawProfilingEnabled = enable;

	if ( enable ) {
		/* 0x004CAE7D: unconditional, with no NULL test on the driver slots. */
		qglDrawElements         = QGL_ProfileDrawElements;          /* 0x004CAC20 */
		qglDrawElementArrayATI  = QGL_ProfileDrawElementArrayATI;   /* 0x004CAE00 */
	} else {
		/* 0x004CAE92: restores from the driver slots whatever the pointers hold,
		   so a gl.log wrapper installed while the flag was set is dropped too. */
		qglDrawElements         = dllDrawElements;
		qglDrawElementArrayATI  = dllDrawElementArrayATI;
	}
}

/*
===========================================================================

gl.log wrappers.  QGL_EnableLogging points every non-NULL qgl pointer at the
matching log function below (0x004B94D0-0x004C0060); each one writes its line
to the gl.log `Stream` and forwards to the driver slot the pointer is later
restored from.  Most print only the entry point's name; the ones the renderer
leans on decode their arguments with the helpers that follow.

===========================================================================
*/

#define GL_TEXTURE_2D               0x0DE1
#define GL_TEXTURE_CUBE_MAP_ARB     0x8513
#define GL_TEXTURE0_ARB             0x84C0
#define GL_FRONT                    0x0404
#define GL_FOG_COLOR                0x0B66
#define GL_DEPTH_BUFFER_BIT         0x00000100
#define GL_ACCUM_BUFFER_BIT         0x00000200
#define GL_STENCIL_BUFFER_BIT       0x00000400
#define GL_COLOR_BUFFER_BIT         0x00004000

/* PointerToString's input: retail tests one range, &tess .. &tess + 0x218068
   (0x017A7F60-0x019BFFC8). */
#include "../renderer/tr_tess.h"

/* tr.images (0x016C9840, tr_registered + 0x4B00); logBindTexture indexes it by texnum - 1024. */
#define tr_imageSlots ( (int *)( (char *)(void *)&tr_registered + 0x4B00 ) )

/* 0x00EA28D8: PrimToString's result, up to qgl_errorCheckingEnabled at 0x00EA2CD8. */
static char s_primString[1024];

/* 0x011A2CE0: CapToString's "0x%x" fallback. */
static char s_capString[1024];

/* ---- BooleanToString  0x004B8AD0 ----
 * logDepthMask's only helper; LTCG inlines it there and keeps this dead
 * copy (GLboolean in AL). */
static const char *BooleanToString( GLboolean b ) {
	if ( b == 0 ) {				/* GL_FALSE */
		return "GL_FALSE";
	} else if ( b == 1 ) {		/* GL_TRUE */
		return "GL_TRUE";
	}
	return "OUT OF RANGE FOR BOOLEAN";
}

/* ---- FuncToString  0x004B8AF0 ---- */
static const char *FuncToString( GLenum f ) {
	switch ( f ) {
	case 0x0207:	return "GL_ALWAYS";
	case 0x0200:	return "GL_NEVER";
	case 0x0203:	return "GL_LEQUAL";
	case 0x0201:	return "GL_LESS";
	case 0x0202:	return "GL_EQUAL";
	case 0x0204:	return "GL_GREATER";
	case 0x0206:	return "GL_GEQUAL";
	case 0x0205:	return "GL_NOTEQUAL";
	default:		return "!!! UNKNOWN !!!";
	}
}

/* ---- PrimToString  0x004B8B60 ---- */
static const char *PrimToString( GLenum mode ) {
	/* an if chain in retail's test order, not a switch (no jump table) */
	if ( mode == 0x0004 ) {
		strcpy( s_primString, "GL_TRIANGLES" );
	} else if ( mode == 0x0005 ) {
		strcpy( s_primString, "GL_TRIANGLE_STRIP" );
	} else if ( mode == 0x0006 ) {
		strcpy( s_primString, "GL_TRIANGLE_FAN" );
	} else if ( mode == 0x0007 ) {
		strcpy( s_primString, "GL_QUADS" );
	} else if ( mode == 0x0008 ) {
		strcpy( s_primString, "GL_QUAD_STRIP" );
	} else if ( mode == 0x0009 ) {
		strcpy( s_primString, "GL_POLYGON" );
	} else if ( mode == 0x0000 ) {
		strcpy( s_primString, "GL_POINTS" );
	} else if ( mode == 0x0001 ) {
		strcpy( s_primString, "GL_LINES" );
	} else if ( mode == 0x0003 ) {
		strcpy( s_primString, "GL_LINE_STRIP" );
	} else if ( mode == 0x0002 ) {
		strcpy( s_primString, "GL_LINE_LOOP" );
	} else {
		sprintf( s_primString, "0x%x", mode );
	}
	return s_primString;
}

/* ---- CapToString  0x004B8DA0 ---- */
static const char *CapToString( GLenum cap ) {
	switch ( cap ) {
	case 0x0DE1:	return "GL_TEXTURE_2D";
	case 0x8513:	return "GL_TEXTURE_CUBE_MAP_ARB";
	case 0x0BE2:	return "GL_BLEND";
	case 0x0B71:	return "GL_DEPTH_TEST";
	case 0x0B44:	return "GL_CULL_FACE";
	case 0x3000:	return "GL_CLIP_PLANE0";
	case 0x8076:	return "GL_COLOR_ARRAY";
	case 0x8078:	return "GL_TEXTURE_COORD_ARRAY";
	case 0x8074:	return "GL_VERTEX_ARRAY";
	case 0x8075:	return "GL_NORMAL_ARRAY";
	case 0x0BC0:	return "GL_ALPHA_TEST";
	case 0x0B90:	return "GL_STENCIL_TEST";
	case 0x851D:	return "GL_VERTEX_ARRAY_RANGE_NV";
	case 0x8533:	return "GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV";
	case 0x0B50:	return "GL_LIGHTING";
	case 0x4000:	return "GL_LIGHT0";
	case 0x4001:	return "GL_LIGHT1";
	case 0x4002:	return "GL_LIGHT2";
	case 0x4003:	return "GL_LIGHT3";
	case 0x4004:	return "GL_LIGHT4";
	case 0x4005:	return "GL_LIGHT5";
	case 0x4006:	return "GL_LIGHT6";
	case 0x4007:	return "GL_LIGHT7";
	case 0x8037:	return "GL_POLYGON_OFFSET_FILL";
	case 0x0B60:	return "GL_FOG";
	case 0x2300:	return "GL_TEXTURE_ENV";
	case 0x2200:	return "GL_TEXTURE_ENV_MODE";
	case 0x8570:	return "GL_COMBINE_ARB";
	case 0x8571:	return "GL_COMBINE_RGB_ARB";
	case 0x8573:	return "GL_RGB_SCALE_ARB";
	case 0x8580:	return "GL_SOURCE0_RGB_ARB";
	case 0x8581:	return "GL_SOURCE1_RGB_ARB";
	case 0x8582:	return "GL_SOURCE2_RGB_ARB";
	case 0x8590:	return "GL_OPERAND0_RGB_ARB";
	case 0x8591:	return "GL_OPERAND1_RGB_ARB";
	case 0x8592:	return "GL_OPERAND2_RGB_ARB";
	case 0x8572:	return "GL_COMBINE_ALPHA_ARB";
	case 0x0D1C:	return "GL_ALPHA_SCALE";
	case 0x8588:	return "GL_SOURCE0_ALPHA_ARB";
	case 0x8589:	return "GL_SOURCE1_ALPHA_ARB";
	case 0x858A:	return "GL_SOURCE2_ALPHA_ARB";
	case 0x8598:	return "GL_OPERAND0_ALPHA_ARB";
	case 0x8599:	return "GL_OPERAND1_ALPHA_ARB";
	case 0x859A:	return "GL_OPERAND2_ALPHA_ARB";
	case 0x1702:	return "GL_TEXTURE";
	case 0x8576:	return "GL_CONSTANT_EXT";
	case 0x8577:	return "GL_PRIMARY_COLOR_EXT";
	case 0x8578:	return "GL_PREVIOUS_EXT";
	case 0x0300:	return "GL_SRC_COLOR";
	case 0x0301:	return "GL_ONE_MINUS_SRC_COLOR";
	case 0x0302:	return "GL_SRC_ALPHA";
	case 0x0303:	return "GL_ONE_MINUS_SRC_ALPHA";
	case 0x1E01:	return "GL_REPLACE";
	case 0x2100:	return "GL_MODULATE";
	case 0x0104:	return "GL_ADD";
	case 0x8574:	return "GL_ADD_SIGNED_EXT";
	case 0x8575:	return "GL_INTERPOLATE_EXT";
	case 0x86AE:	return "GL_DOT3_RGB_ARB";
	case 0x86AF:	return "GL_DOT3_RGBA_ARB";
	case 0x0408:	return "GL_FRONT_AND_BACK";
	case 0x0404:	return "GL_FRONT";
	case 0x0405:	return "GL_BACK";
	case 0x1B00:	return "GL_POINT";
	case 0x1B01:	return "GL_LINE";
	case 0x1B02:	return "GL_FILL";
	case 0x0BA1:	return "GL_NORMALIZE";
	case 0x803A:	return "GL_RESCALE_NORMAL_EXT";
	case 0x8620:	return "GL_VERTEX_PROGRAM_ARB";
	case 0x8920:	return "GL_FRAGMENT_SHADER_ATI";
	case 0x8892:	return "GL_ARRAY_BUFFER_ARB";
	case 0x8893:	return "GL_ELEMENT_ARRAY_BUFFER_ARB";
	case 0x88E0:	return "GL_STREAM_DRAW_ARB";
	case 0x88E1:	return "GL_STREAM_READ_ARB";
	case 0x88E2:	return "GL_STREAM_COPY_ARB";
	case 0x88E4:	return "GL_STATIC_DRAW_ARB";
	case 0x88E5:	return "GL_STATIC_READ_ARB";
	case 0x88E6:	return "GL_STATIC_COPY_ARB";
	case 0x88E8:	return "GL_DYNAMIC_DRAW_ARB";
	case 0x88E9:	return "GL_DYNAMIC_READ_ARB";
	case 0x88EA:	return "GL_DYNAMIC_COPY_ARB";
	case 0x88B8:	return "GL_READ_ONLY_ARB";
	case 0x88B9:	return "GL_WRITE_ONLY_ARB";
	case 0x88BA:	return "GL_READ_WRITE_ARB";
	default:
		sprintf( s_capString, "0x%x", cap );
		return s_capString;
	}
}

/* ---- TypeToString  0x004B93D0 ---- */
static const char *TypeToString( GLenum t ) {
	switch ( t ) {
	case 0x1400:	return "GL_BYTE";
	case 0x1401:	return "GL_UNSIGNED_BYTE";
	case 0x1402:	return "GL_SHORT";
	case 0x1403:	return "GL_UNSIGNED_SHORT";
	case 0x1404:	return "GL_INT";
	case 0x1405:	return "GL_UNSIGNED_INT";
	case 0x1406:	return "GL_FLOAT";
	case 0x140A:	return "GL_DOUBLE";
	default:		return "!!! UNKNOWN !!!";
	}
}

/* ---- PointerToString  0x004B9450 ---- */
static const char *PointerToString( const GLvoid *pointer ) {
	unsigned int p = (unsigned int)pointer;

	if ( (const char *)pointer >= (const char *)&tess &&
		 (const char *)pointer <  (const char *)&tess + sizeof( tess ) ) {
		return "TESS";
	}

	if ( glConfig_ARBVertexBufferObject ) {
		if ( p < 0x100000 ) {
			return va( "VBO_OFFSET(%i)", p );
		}
		return "MEM";
	}

	if ( !glConfig_NVVertexArrayRange ) {
		return "MEM";
	}
	if ( p >= (unsigned int)tr_staticVertexMemorySecondary &&
		 p <  (unsigned int)tr_staticVertexMemorySecondary + (unsigned int)tr_staticVertexMemorySecondaryLimit ) {
		return "AGP_MEM";
	}
	if ( p >= (unsigned int)tr_staticVertexMemoryPrimary &&
		 p <  (unsigned int)tr_staticVertexMemoryPrimary + (unsigned int)tr_staticVertexMemoryPrimaryLimit ) {
		return "VID_MEM";
	}
	return "MEM";
}

/* ---- BlendToName  0x004B97A0 ---- */
static void BlendToName( char *n, GLenum f ) {
	switch ( f ) {
	case 0x0000:	strcpy( n, "GL_ZERO" );					break;
	case 0x0001:	strcpy( n, "GL_ONE" );					break;
	case 0x0302:	strcpy( n, "GL_SRC_ALPHA" );			break;
	case 0x0303:	strcpy( n, "GL_ONE_MINUS_SRC_ALPHA" );	break;
	case 0x0304:	strcpy( n, "GL_DST_ALPHA" );			break;
	case 0x0306:	strcpy( n, "GL_DST_COLOR" );			break;
	case 0x0307:	strcpy( n, "GL_ONE_MINUS_DST_COLOR" );	break;
	default:		sprintf( n, "0x%x", f );				break;
	}
}

/* ---- FogParamToString  0x004B9EF0 ---- */
static const char *FogParamToString( GLenum pname ) {
	switch ( pname ) {
	case 0x0B65:	return "GL_FOG_MODE";
	case 0x0B62:	return "GL_FOG_DENSITY";
	case 0x0B63:	return "GL_FOG_START";
	case 0x0B64:	return "GL_FOG_END";
	case 0x0B66:	return "GL_FOG_COLOR";
	default:		return va( "0x%4x", pname );
	}
}

/* ---- logAccum  0x004BBF30 ---- */
static void QGLAPI logAccum( GLenum op, GLfloat value ) {
	fprintf( Stream, "glAccum\n" );
	fflush( Stream );
	dllAccum( op, value );
}

/* ---- logAlphaFunc  0x004B94D0 ---- */
static void QGLAPI logAlphaFunc( GLenum func, GLclampf ref ) {
	fprintf( Stream, "glAlphaFunc( %s, %f )\n", FuncToString( func ), ref );
	fflush( Stream );
	dllAlphaFunc( func, ref );
}

/* ---- logAreTexturesResident  0x004BBF60 ---- */
static GLboolean QGLAPI logAreTexturesResident( GLsizei n, const GLuint *textures, GLboolean *residences ) {
	fprintf( Stream, "glAreTexturesResident\n" );
	fflush( Stream );
	return dllAreTexturesResident( n, textures, residences );
}

/* ---- logArrayElement  0x004BBF90 ---- */
static void QGLAPI logArrayElement( GLint i ) {
	fprintf( Stream, "glArrayElement\n" );
	fflush( Stream );
	dllArrayElement( i );
}

/* ---- logBegin  0x004B9520 ---- */
static void QGLAPI logBegin( GLenum mode ) {
	fprintf( Stream, "glBegin( %s )\n", PrimToString( mode ) );
	fflush( Stream );
	dllBegin( mode );
}

/* ---- logBindTexture  0x004B9560 ---- */
static void QGLAPI logBindTexture( GLenum target, GLuint texture ) {
	const char *image;
	const char *name;

	/* 0x016C8840 is tr.images - 1024 entries (texnum = index + 1024); image_t
	   starts with its name.  Texture 0 and empty slots print the number. */
	image = texture ? (const char *)tr_imageSlots[(int)texture - 1024] : NULL;
	name = image ? image : NULL;    /* image->imgName: two tests, as retail */
	if ( !name ) {
		name = va( "%u", texture );
	}

	if ( target == GL_TEXTURE_2D ) {
		fprintf( Stream, "glBindTexture( GL_TEXTURE_2D, %s )\n", name );
	} else if ( target == GL_TEXTURE_CUBE_MAP_ARB ) {
		fprintf( Stream, "glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, %s )\n", name );
	} else {
		fprintf( Stream, "glBindTexture( 0x%x, %s )\n", target, name );
	}
	fflush( Stream );
	dllBindTexture( target, texture );
}

/* ---- logBitmap  0x004BBFC0 ---- */
static void QGLAPI logBitmap( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ) {
	fprintf( Stream, "glBitmap\n" );
	fflush( Stream );
	dllBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
}

/* ---- logBlendFunc  0x004B98B0 ---- */
static void QGLAPI logBlendFunc( GLenum sfactor, GLenum dfactor ) {
	char sf[128], df[128];

	BlendToName( sf, sfactor );
	BlendToName( df, dfactor );
	fprintf( Stream, "glBlendFunc( %s, %s )\n", sf, df );
	fflush( Stream );
	dllBlendFunc( sfactor, dfactor );
}

/* ---- logCallList  0x004B9950 ---- */
static void QGLAPI logCallList( GLuint list ) {
	fprintf( Stream, "glCallList( %u )\n", list );
	fflush( Stream );
	dllCallList( list );
}

/* ---- logCallLists  0x004BBFF0 ---- */
static void QGLAPI logCallLists( GLsizei n, GLenum type, const GLvoid *lists ) {
	fprintf( Stream, "glCallLists\n" );
	fflush( Stream );
	dllCallLists( n, type, lists );
}

/* ---- logClear  0x004B9980 ---- */
static void QGLAPI logClear( GLbitfield mask ) {
	fprintf( Stream, "glClear( 0x%x = ", mask );

	if ( mask & GL_COLOR_BUFFER_BIT ) {
		fprintf( Stream, "GL_COLOR_BUFFER_BIT " );
	}
	if ( mask & GL_DEPTH_BUFFER_BIT ) {
		fprintf( Stream, "GL_DEPTH_BUFFER_BIT " );
	}
	if ( mask & GL_STENCIL_BUFFER_BIT ) {
		fprintf( Stream, "GL_STENCIL_BUFFER_BIT " );
	}
	if ( mask & GL_ACCUM_BUFFER_BIT ) {
		fprintf( Stream, "GL_ACCUM_BUFFER_BIT " );
	}

	fprintf( Stream, ")\n" );
	fflush( Stream );
	dllClear( mask );
}

/* ---- logClearAccum  0x004BC020 ---- */
static void QGLAPI logClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
	fprintf( Stream, "glClearAccum\n" );
	fflush( Stream );
	dllClearAccum( red, green, blue, alpha );
}

/* ---- logClearColor  0x004BC050 ---- */
static void QGLAPI logClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) {
	fprintf( Stream, "glClearColor\n" );
	fflush( Stream );
	dllClearColor( red, green, blue, alpha );
}

/* ---- logClearDepth  0x004B9A40 ---- */
static void QGLAPI logClearDepth( GLclampd depth ) {
	fprintf( Stream, "glClearDepth( %f )\n", FCAST( depth ) );
	fflush( Stream );
	dllClearDepth( depth );
}

/* ---- logClearIndex  0x004BC080 ---- */
static void QGLAPI logClearIndex( GLfloat c ) {
	fprintf( Stream, "glClearIndex\n" );
	fflush( Stream );
	dllClearIndex( c );
}

/* ---- logClearStencil  0x004B9A80 ---- */
static void QGLAPI logClearStencil( GLint s ) {
	fprintf( Stream, "glClearStencil( %d )\n", s );
	fflush( Stream );
	dllClearStencil( s );
}

/* ---- logClipPlane  0x004BC0B0 ---- */
static void QGLAPI logClipPlane( GLenum plane, const GLdouble *equation ) {
	fprintf( Stream, "glClipPlane\n" );
	fflush( Stream );
	dllClipPlane( plane, equation );
}

/* ---- logColor3b  0x004BC0E0 ---- */
static void QGLAPI logColor3b( GLbyte red, GLbyte green, GLbyte blue ) {
	fprintf( Stream, "glColor3b\n" );
	fflush( Stream );
	dllColor3b( red, green, blue );
}

/* ---- logColor3bv  0x004BC110 ---- */
static void QGLAPI logColor3bv( const GLbyte *v ) {
	fprintf( Stream, "glColor3bv\n" );
	fflush( Stream );
	dllColor3bv( v );
}

/* ---- logColor3d  0x004BC140 ---- */
static void QGLAPI logColor3d( GLdouble red, GLdouble green, GLdouble blue ) {
	fprintf( Stream, "glColor3d\n" );
	fflush( Stream );
	dllColor3d( red, green, blue );
}

/* ---- logColor3dv  0x004BC190 ---- */
static void QGLAPI logColor3dv( const GLdouble *v ) {
	fprintf( Stream, "glColor3dv\n" );
	fflush( Stream );
	dllColor3dv( v );
}

/* ---- logColor3f  0x004BC1C0 ---- */
static void QGLAPI logColor3f( GLfloat red, GLfloat green, GLfloat blue ) {
	fprintf( Stream, "glColor3f\n" );
	fflush( Stream );
	dllColor3f( red, green, blue );
}

/* ---- logColor3fv  0x004BC1F0 ---- */
static void QGLAPI logColor3fv( const GLfloat *v ) {
	fprintf( Stream, "glColor3fv\n" );
	fflush( Stream );
	dllColor3fv( v );
}

/* ---- logColor3i  0x004BC220 ---- */
static void QGLAPI logColor3i( GLint red, GLint green, GLint blue ) {
	fprintf( Stream, "glColor3i\n" );
	fflush( Stream );
	dllColor3i( red, green, blue );
}

/* ---- logColor3iv  0x004BC250 ---- */
static void QGLAPI logColor3iv( const GLint *v ) {
	fprintf( Stream, "glColor3iv\n" );
	fflush( Stream );
	dllColor3iv( v );
}

/* ---- logColor3s  0x004BC280 ---- */
static void QGLAPI logColor3s( GLshort red, GLshort green, GLshort blue ) {
	fprintf( Stream, "glColor3s\n" );
	fflush( Stream );
	dllColor3s( red, green, blue );
}

/* ---- logColor3sv  0x004BC2B0 ---- */
static void QGLAPI logColor3sv( const GLshort *v ) {
	fprintf( Stream, "glColor3sv\n" );
	fflush( Stream );
	dllColor3sv( v );
}

/* ---- logColor3ub  0x004BC2E0 ---- */
static void QGLAPI logColor3ub( GLubyte red, GLubyte green, GLubyte blue ) {
	fprintf( Stream, "glColor3ub\n" );
	fflush( Stream );
	dllColor3ub( red, green, blue );
}

/* ---- logColor3ubv  0x004BC310 ---- */
static void QGLAPI logColor3ubv( const GLubyte *v ) {
	fprintf( Stream, "glColor3ubv\n" );
	fflush( Stream );
	dllColor3ubv( v );
}

/* ---- logColor3ui  0x004BC340 ---- */
static void QGLAPI logColor3ui( GLuint red, GLuint green, GLuint blue ) {
	fprintf( Stream, "glColor3ui\n" );
	fflush( Stream );
	dllColor3ui( red, green, blue );
}

/* ---- logColor3uiv  0x004BC370 ---- */
static void QGLAPI logColor3uiv( const GLuint *v ) {
	fprintf( Stream, "glColor3uiv\n" );
	fflush( Stream );
	dllColor3uiv( v );
}

/* ---- logColor3us  0x004BC3A0 ---- */
static void QGLAPI logColor3us( GLushort red, GLushort green, GLushort blue ) {
	fprintf( Stream, "glColor3us\n" );
	fflush( Stream );
	dllColor3us( red, green, blue );
}

/* ---- logColor3usv  0x004BC3D0 ---- */
static void QGLAPI logColor3usv( const GLushort *v ) {
	fprintf( Stream, "glColor3usv\n" );
	fflush( Stream );
	dllColor3usv( v );
}

/* ---- logColor4b  0x004BC400 ---- */
static void QGLAPI logColor4b( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) {
	fprintf( Stream, "glColor4b\n" );
	fflush( Stream );
	dllColor4b( red, green, blue, alpha );
}

/* ---- logColor4bv  0x004BC430 ---- */
static void QGLAPI logColor4bv( const GLbyte *v ) {
	fprintf( Stream, "glColor4bv\n" );
	fflush( Stream );
	dllColor4bv( v );
}

/* ---- logColor4d  0x004BC460 ---- */
static void QGLAPI logColor4d( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) {
	fprintf( Stream, "glColor4d\n" );
	fflush( Stream );
	dllColor4d( red, green, blue, alpha );
}

/* ---- logColor4dv  0x004BC4B0 ---- */
static void QGLAPI logColor4dv( const GLdouble *v ) {
	fprintf( Stream, "glColor4dv\n" );
	fflush( Stream );
	dllColor4dv( v );
}

/* ---- logColor4f  0x004B9AB0 ---- */
static void QGLAPI logColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) {
	fprintf( Stream, "glColor4f( %f,%f,%f,%f )\n", red, green, blue, alpha );
	fflush( Stream );
	dllColor4f( red, green, blue, alpha );
}

/* ---- logColor4fv  0x004B9B00 ---- */
static void QGLAPI logColor4fv( const GLfloat *v ) {
	fprintf( Stream, "glColor4fv( %f,%f,%f,%f )\n", v[0], v[1], v[2], v[3] );
	fflush( Stream );
	dllColor4fv( v );
}

/* ---- logColor4i  0x004BC4E0 ---- */
static void QGLAPI logColor4i( GLint red, GLint green, GLint blue, GLint alpha ) {
	fprintf( Stream, "glColor4i\n" );
	fflush( Stream );
	dllColor4i( red, green, blue, alpha );
}

/* ---- logColor4iv  0x004BC510 ---- */
static void QGLAPI logColor4iv( const GLint *v ) {
	fprintf( Stream, "glColor4iv\n" );
	fflush( Stream );
	dllColor4iv( v );
}

/* ---- logColor4s  0x004BC540 ---- */
static void QGLAPI logColor4s( GLshort red, GLshort green, GLshort blue, GLshort alpha ) {
	fprintf( Stream, "glColor4s\n" );
	fflush( Stream );
	dllColor4s( red, green, blue, alpha );
}

/* ---- logColor4sv  0x004BC570 ---- */
static void QGLAPI logColor4sv( const GLshort *v ) {
	fprintf( Stream, "glColor4sv\n" );
	fflush( Stream );
	dllColor4sv( v );
}

/* ---- logColor4ub  0x004BC5A0 ---- */
static void QGLAPI logColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) {
	fprintf( Stream, "glColor4ub\n" );
	fflush( Stream );
	dllColor4ub( red, green, blue, alpha );
}

/* ---- logColor4ubv  0x004BC5D0 ---- */
static void QGLAPI logColor4ubv( const GLubyte *v ) {
	fprintf( Stream, "glColor4ubv\n" );
	fflush( Stream );
	dllColor4ubv( v );
}

/* ---- logColor4ui  0x004BC600 ---- */
static void QGLAPI logColor4ui( GLuint red, GLuint green, GLuint blue, GLuint alpha ) {
	fprintf( Stream, "glColor4ui\n" );
	fflush( Stream );
	dllColor4ui( red, green, blue, alpha );
}

/* ---- logColor4uiv  0x004BC630 ---- */
static void QGLAPI logColor4uiv( const GLuint *v ) {
	fprintf( Stream, "glColor4uiv\n" );
	fflush( Stream );
	dllColor4uiv( v );
}

/* ---- logColor4us  0x004BC660 ---- */
static void QGLAPI logColor4us( GLushort red, GLushort green, GLushort blue, GLushort alpha ) {
	fprintf( Stream, "glColor4us\n" );
	fflush( Stream );
	dllColor4us( red, green, blue, alpha );
}

/* ---- logColor4usv  0x004BC690 ---- */
static void QGLAPI logColor4usv( const GLushort *v ) {
	fprintf( Stream, "glColor4usv\n" );
	fflush( Stream );
	dllColor4usv( v );
}

/* ---- logColorMask  0x004BC6C0 ---- */
static void QGLAPI logColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) {
	fprintf( Stream, "glColorMask\n" );
	fflush( Stream );
	dllColorMask( red, green, blue, alpha );
}

/* ---- logColorMaterial  0x004BC6F0 ---- */
static void QGLAPI logColorMaterial( GLenum face, GLenum mode ) {
	fprintf( Stream, "glColorMaterial\n" );
	fflush( Stream );
	dllColorMaterial( face, mode );
}

/* ---- logColorPointer  0x004B9B50 ---- */
static void QGLAPI logColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	const char *pointerName = PointerToString( pointer );

	fprintf( Stream, "glColorPointer( %d, %s, %d, %s )\n", size, TypeToString( type ), stride, pointerName );
	fflush( Stream );
	dllColorPointer( size, type, stride, pointer );
}

/* ---- logCopyPixels  0x004BC720 ---- */
static void QGLAPI logCopyPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) {
	fprintf( Stream, "glCopyPixels\n" );
	fflush( Stream );
	dllCopyPixels( x, y, width, height, type );
}

/* ---- logCopyTexImage1D  0x004BC750 ---- */
static void QGLAPI logCopyTexImage1D( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border ) {
	fprintf( Stream, "glCopyTexImage1D\n" );
	fflush( Stream );
	dllCopyTexImage1D( target, level, internalFormat, x, y, width, border );
}

/* ---- logCopyTexImage2D  0x004BC780 ---- */
static void QGLAPI logCopyTexImage2D( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ) {
	fprintf( Stream, "glCopyTexImage2D\n" );
	fflush( Stream );
	dllCopyTexImage2D( target, level, internalFormat, x, y, width, height, border );
}

/* ---- logCopyTexSubImage1D  0x004BC7B0 ---- */
static void QGLAPI logCopyTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ) {
	fprintf( Stream, "glCopyTexSubImage1D\n" );
	fflush( Stream );
	dllCopyTexSubImage1D( target, level, xoffset, x, y, width );
}

/* ---- logCopyTexSubImage2D  0x004BC7E0 ---- */
static void QGLAPI logCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ) {
	fprintf( Stream, "glCopyTexSubImage2D\n" );
	fflush( Stream );
	dllCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

/* ---- logCullFace  0x004B9BB0 ---- */
static void QGLAPI logCullFace( GLenum mode ) {
	/* every value but GL_FRONT is labelled GL_BACK */
	fprintf( Stream, "glCullFace( %s )\n", ( mode == GL_FRONT ) ? "GL_FRONT" : "GL_BACK" );
	fflush( Stream );
	dllCullFace( mode );
}

/* ---- logDeleteLists  0x004BC810 ---- */
static void QGLAPI logDeleteLists( GLuint list, GLsizei range ) {
	fprintf( Stream, "glDeleteLists\n" );
	fflush( Stream );
	dllDeleteLists( list, range );
}

/* ---- logDeleteTextures  0x004BC840 ---- */
static void QGLAPI logDeleteTextures( GLsizei n, const GLuint *textures ) {
	fprintf( Stream, "glDeleteTextures\n" );
	fflush( Stream );
	dllDeleteTextures( n, textures );
}

/* ---- logDepthFunc  0x004B9C00 ---- */
static void QGLAPI logDepthFunc( GLenum func ) {
	fprintf( Stream, "glDepthFunc( %s )\n", FuncToString( func ) );
	fflush( Stream );
	dllDepthFunc( func );
}

/* ---- logDepthMask  0x004B9C40 ---- */
static void QGLAPI logDepthMask( GLboolean flag ) {
	fprintf( Stream, "glDepthMask( %s )\n", BooleanToString( flag ) );
	fflush( Stream );
	dllDepthMask( flag );
}

/* ---- logDepthRange  0x004B9C90 ---- */
static void QGLAPI logDepthRange( GLclampd zNear, GLclampd zFar ) {
	fprintf( Stream, "glDepthRange( %f, %f )\n", FCAST( zNear ), FCAST( zFar ) );
	fflush( Stream );
	dllDepthRange( zNear, zFar );
}

/* ---- logDisable  0x004B9CE0 ---- */
static void QGLAPI logDisable( GLenum cap ) {
	fprintf( Stream, "glDisable( %s )\n", CapToString( cap ) );
	fflush( Stream );
	dllDisable( cap );
}

/* ---- logDisableClientState  0x004B9D20 ---- */
static void QGLAPI logDisableClientState( GLenum array ) {
	fprintf( Stream, "glDisableClientState( %s )\n", CapToString( array ) );
	fflush( Stream );
	dllDisableClientState( array );
}

/* ---- logDrawArrays  0x004BC870 ---- */
static void QGLAPI logDrawArrays( GLenum mode, GLint first, GLsizei count ) {
	fprintf( Stream, "glDrawArrays\n" );
	fflush( Stream );
	dllDrawArrays( mode, first, count );
}

/* ---- logDrawBuffer  0x004BC8A0 ---- */
static void QGLAPI logDrawBuffer( GLenum mode ) {
	fprintf( Stream, "glDrawBuffer\n" );
	fflush( Stream );
	dllDrawBuffer( mode );
}

/* ---- logDrawElements  0x004B9D60 ---- */
static void QGLAPI logDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices ) {
	const char *indicesName = PointerToString( indices );
	const char *typeName    = TypeToString( type );

	fprintf( Stream, "glDrawElements( %s, %d, %s, %s )\n", PrimToString( mode ), count, typeName, indicesName );
	fflush( Stream );
	dllDrawElements( mode, count, type, indices );
}

/* ---- logDrawPixels  0x004BC8D0 ---- */
static void QGLAPI logDrawPixels( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
	fprintf( Stream, "glDrawPixels\n" );
	fflush( Stream );
	dllDrawPixels( width, height, format, type, pixels );
}

/* ---- logEdgeFlag  0x004BC900 ---- */
static void QGLAPI logEdgeFlag( GLboolean flag ) {
	fprintf( Stream, "glEdgeFlag\n" );
	fflush( Stream );
	dllEdgeFlag( flag );
}

/* ---- logEdgeFlagPointer  0x004BC930 ---- */
static void QGLAPI logEdgeFlagPointer( GLsizei stride, const GLvoid *pointer ) {
	fprintf( Stream, "glEdgeFlagPointer\n" );
	fflush( Stream );
	dllEdgeFlagPointer( stride, pointer );
}

/* ---- logEdgeFlagv  0x004BC960 ---- */
static void QGLAPI logEdgeFlagv( const GLboolean *flag ) {
	fprintf( Stream, "glEdgeFlagv\n" );
	fflush( Stream );
	dllEdgeFlagv( flag );
}

/* ---- logEnable  0x004B9E30 ---- */
static void QGLAPI logEnable( GLenum cap ) {
	fprintf( Stream, "glEnable( %s )\n", CapToString( cap ) );
	fflush( Stream );
	dllEnable( cap );
}

/* ---- logEnableClientState  0x004B9E70 ---- */
static void QGLAPI logEnableClientState( GLenum array ) {
	fprintf( Stream, "glEnableClientState( %s )\n", CapToString( array ) );
	fflush( Stream );
	dllEnableClientState( array );
}

/* ---- logEnd  0x004BC990 ---- */
static void QGLAPI logEnd( void ) {
	fprintf( Stream, "glEnd\n" );
	fflush( Stream );
	dllEnd();
}

/* ---- logEndList  0x004BC9C0 ---- */
static void QGLAPI logEndList( void ) {
	fprintf( Stream, "glEndList\n" );
	fflush( Stream );
	dllEndList();
}

/* ---- logEvalCoord1d  0x004BC9F0 ---- */
static void QGLAPI logEvalCoord1d( GLdouble u ) {
	fprintf( Stream, "glEvalCoord1d\n" );
	fflush( Stream );
	dllEvalCoord1d( u );
}

/* ---- logEvalCoord1dv  0x004BCA30 ---- */
static void QGLAPI logEvalCoord1dv( const GLdouble *u ) {
	fprintf( Stream, "glEvalCoord1dv\n" );
	fflush( Stream );
	dllEvalCoord1dv( u );
}

/* ---- logEvalCoord1f  0x004BCA60 ---- */
static void QGLAPI logEvalCoord1f( GLfloat u ) {
	fprintf( Stream, "glEvalCoord1f\n" );
	fflush( Stream );
	dllEvalCoord1f( u );
}

/* ---- logEvalCoord1fv  0x004BCA90 ---- */
static void QGLAPI logEvalCoord1fv( const GLfloat *u ) {
	fprintf( Stream, "glEvalCoord1fv\n" );
	fflush( Stream );
	dllEvalCoord1fv( u );
}

/* ---- logEvalCoord2d  0x004BCAC0 ---- */
static void QGLAPI logEvalCoord2d( GLdouble u, GLdouble v ) {
	fprintf( Stream, "glEvalCoord2d\n" );
	fflush( Stream );
	dllEvalCoord2d( u, v );
}

/* ---- logEvalCoord2dv  0x004BCB00 ---- */
static void QGLAPI logEvalCoord2dv( const GLdouble *u ) {
	fprintf( Stream, "glEvalCoord2dv\n" );
	fflush( Stream );
	dllEvalCoord2dv( u );
}

/* ---- logEvalCoord2f  0x004BCB30 ---- */
static void QGLAPI logEvalCoord2f( GLfloat u, GLfloat v ) {
	fprintf( Stream, "glEvalCoord2f\n" );
	fflush( Stream );
	dllEvalCoord2f( u, v );
}

/* ---- logEvalCoord2fv  0x004BCB60 ---- */
static void QGLAPI logEvalCoord2fv( const GLfloat *u ) {
	fprintf( Stream, "glEvalCoord2fv\n" );
	fflush( Stream );
	dllEvalCoord2fv( u );
}

/* ---- logEvalMesh1  0x004BCB90 ---- */
static void QGLAPI logEvalMesh1( GLenum mode, GLint i1, GLint i2 ) {
	fprintf( Stream, "glEvalMesh1\n" );
	fflush( Stream );
	dllEvalMesh1( mode, i1, i2 );
}

/* ---- logEvalMesh2  0x004BCBC0 ---- */
static void QGLAPI logEvalMesh2( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) {
	fprintf( Stream, "glEvalMesh2\n" );
	fflush( Stream );
	dllEvalMesh2( mode, i1, i2, j1, j2 );
}

/* ---- logEvalPoint1  0x004BCBF0 ---- */
static void QGLAPI logEvalPoint1( GLint i ) {
	fprintf( Stream, "glEvalPoint1\n" );
	fflush( Stream );
	dllEvalPoint1( i );
}

/* ---- logEvalPoint2  0x004BCC20 ---- */
static void QGLAPI logEvalPoint2( GLint i, GLint j ) {
	fprintf( Stream, "glEvalPoint2\n" );
	fflush( Stream );
	dllEvalPoint2( i, j );
}

/* ---- logFeedbackBuffer  0x004BCC50 ---- */
static void QGLAPI logFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer ) {
	fprintf( Stream, "glFeedbackBuffer\n" );
	fflush( Stream );
	dllFeedbackBuffer( size, type, buffer );
}

/* ---- logFinish  0x004BCC80 ---- */
static void QGLAPI logFinish( void ) {
	fprintf( Stream, "glFinish\n" );
	fflush( Stream );
	dllFinish();
}

/* ---- logFlush  0x004BCCB0 ---- */
static void QGLAPI logFlush( void ) {
	fprintf( Stream, "glFlush\n" );
	fflush( Stream );
	dllFlush();
}

/* ---- logFogf  0x004B9F50 ---- */
static void QGLAPI logFogf( GLenum pname, GLfloat param ) {
	/* the three fog wrappers do not flush */
	fprintf( Stream, "glFogf( %s, %g )\n", FogParamToString( pname ), param );
	dllFogf( pname, param );
}

/* ---- logFogfv  0x004B9F90 ---- */
static void QGLAPI logFogfv( GLenum pname, const GLfloat *params ) {
	if ( pname == GL_FOG_COLOR ) {
		fprintf( Stream, "glFogfv( GL_FOG_COLOR, { %g, %g, %g, %g } )\n",
				 params[0], params[1], params[2], params[3] );
	} else {
		fprintf( Stream, "glFogfv( %s, %g )\n", FogParamToString( pname ), params[0] );
	}
	dllFogfv( pname, params );
}

/* ---- logFogi  0x004BA010 ---- */
static void QGLAPI logFogi( GLenum pname, GLint param ) {
	fprintf( Stream, "glFogi( %s, %i )\n", FogParamToString( pname ), param );
	dllFogi( pname, param );
}

/* ---- logFogiv  0x004BCCE0 ---- */
static void QGLAPI logFogiv( GLenum pname, const GLint *params ) {
	fprintf( Stream, "glFogiv\n" );
	fflush( Stream );
	dllFogiv( pname, params );
}

/* ---- logFrontFace  0x004BCD10 ---- */
static void QGLAPI logFrontFace( GLenum mode ) {
	fprintf( Stream, "glFrontFace\n" );
	fflush( Stream );
	dllFrontFace( mode );
}

/* ---- logFrustum  0x004BCD40 ---- */
static void QGLAPI logFrustum( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	fprintf( Stream, "glFrustum\n" );
	fflush( Stream );
	dllFrustum( left, right, bottom, top, zNear, zFar );
}

/* ---- logGenLists  0x004BCDB0 ---- */
static GLuint QGLAPI logGenLists( GLsizei range ) {
	fprintf( Stream, "glGenLists\n" );
	fflush( Stream );
	return dllGenLists( range );
}

/* ---- logGenTextures  0x004BCDE0 ---- */
static void QGLAPI logGenTextures( GLsizei n, GLuint *textures ) {
	fprintf( Stream, "glGenTextures\n" );
	fflush( Stream );
	dllGenTextures( n, textures );
}

/* ---- logGetBooleanv  0x004BCE10 ---- */
static void QGLAPI logGetBooleanv( GLenum pname, GLboolean *params ) {
	fprintf( Stream, "glGetBooleanv\n" );
	fflush( Stream );
	dllGetBooleanv( pname, params );
}

/* ---- logGetClipPlane  0x004BCE40 ---- */
static void QGLAPI logGetClipPlane( GLenum plane, GLdouble *equation ) {
	fprintf( Stream, "glGetClipPlane\n" );
	fflush( Stream );
	dllGetClipPlane( plane, equation );
}

/* ---- logGetDoublev  0x004BCE70 ---- */
static void QGLAPI logGetDoublev( GLenum pname, GLdouble *params ) {
	fprintf( Stream, "glGetDoublev\n" );
	fflush( Stream );
	dllGetDoublev( pname, params );
}

/* ---- logGetError  0x004BCEA0 ---- */
static GLenum QGLAPI logGetError( void ) {
	fprintf( Stream, "glGetError\n" );
	fflush( Stream );
	return dllGetError();
}

/* ---- logGetFloatv  0x004BCED0 ---- */
static void QGLAPI logGetFloatv( GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetFloatv\n" );
	fflush( Stream );
	dllGetFloatv( pname, params );
}

/* ---- logGetIntegerv  0x004BCF00 ---- */
static void QGLAPI logGetIntegerv( GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetIntegerv\n" );
	fflush( Stream );
	dllGetIntegerv( pname, params );
}

/* ---- logGetLightfv  0x004BCF30 ---- */
static void QGLAPI logGetLightfv( GLenum light, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetLightfv\n" );
	fflush( Stream );
	dllGetLightfv( light, pname, params );
}

/* ---- logGetLightiv  0x004BCF60 ---- */
static void QGLAPI logGetLightiv( GLenum light, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetLightiv\n" );
	fflush( Stream );
	dllGetLightiv( light, pname, params );
}

/* ---- logGetMapdv  0x004BCF90 ---- */
static void QGLAPI logGetMapdv( GLenum target, GLenum query, GLdouble *v ) {
	fprintf( Stream, "glGetMapdv\n" );
	fflush( Stream );
	dllGetMapdv( target, query, v );
}

/* ---- logGetMapfv  0x004BCFC0 ---- */
static void QGLAPI logGetMapfv( GLenum target, GLenum query, GLfloat *v ) {
	fprintf( Stream, "glGetMapfv\n" );
	fflush( Stream );
	dllGetMapfv( target, query, v );
}

/* ---- logGetMapiv  0x004BCFF0 ---- */
static void QGLAPI logGetMapiv( GLenum target, GLenum query, GLint *v ) {
	fprintf( Stream, "glGetMapiv\n" );
	fflush( Stream );
	dllGetMapiv( target, query, v );
}

/* ---- logGetMaterialfv  0x004BD020 ---- */
static void QGLAPI logGetMaterialfv( GLenum face, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetMaterialfv\n" );
	fflush( Stream );
	dllGetMaterialfv( face, pname, params );
}

/* ---- logGetMaterialiv  0x004BD050 ---- */
static void QGLAPI logGetMaterialiv( GLenum face, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetMaterialiv\n" );
	fflush( Stream );
	dllGetMaterialiv( face, pname, params );
}

/* ---- logGetPixelMapfv  0x004BD080 ---- */
static void QGLAPI logGetPixelMapfv( GLenum map, GLfloat *values ) {
	fprintf( Stream, "glGetPixelMapfv\n" );
	fflush( Stream );
	dllGetPixelMapfv( map, values );
}

/* ---- logGetPixelMapuiv  0x004BD0B0 ---- */
static void QGLAPI logGetPixelMapuiv( GLenum map, GLuint *values ) {
	fprintf( Stream, "glGetPixelMapuiv\n" );
	fflush( Stream );
	dllGetPixelMapuiv( map, values );
}

/* ---- logGetPixelMapusv  0x004BD0E0 ---- */
static void QGLAPI logGetPixelMapusv( GLenum map, GLushort *values ) {
	fprintf( Stream, "glGetPixelMapusv\n" );
	fflush( Stream );
	dllGetPixelMapusv( map, values );
}

/* ---- logGetPointerv  0x004BD110 ---- */
static void QGLAPI logGetPointerv( GLenum pname, GLvoid* *params ) {
	fprintf( Stream, "glGetPointerv\n" );
	fflush( Stream );
	dllGetPointerv( pname, params );
}

/* ---- logGetPolygonStipple  0x004BD140 ---- */
static void QGLAPI logGetPolygonStipple( GLubyte *mask ) {
	fprintf( Stream, "glGetPolygonStipple\n" );
	fflush( Stream );
	dllGetPolygonStipple( mask );
}

/* ---- logGetString  0x004BD170 ---- */
static const GLubyte * QGLAPI logGetString( GLenum name ) {
	fprintf( Stream, "glGetString\n" );
	fflush( Stream );
	return dllGetString( name );
}

/* ---- logGetTexEnvfv  0x004BD1A0 ---- */
static void QGLAPI logGetTexEnvfv( GLenum target, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetTexEnvfv\n" );
	fflush( Stream );
	dllGetTexEnvfv( target, pname, params );
}

/* ---- logGetTexEnviv  0x004BD1D0 ---- */
static void QGLAPI logGetTexEnviv( GLenum target, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetTexEnviv\n" );
	fflush( Stream );
	dllGetTexEnviv( target, pname, params );
}

/* ---- logGetTexGendv  0x004BD200 ---- */
static void QGLAPI logGetTexGendv( GLenum coord, GLenum pname, GLdouble *params ) {
	fprintf( Stream, "glGetTexGendv\n" );
	fflush( Stream );
	dllGetTexGendv( coord, pname, params );
}

/* ---- logGetTexGenfv  0x004BD230 ---- */
static void QGLAPI logGetTexGenfv( GLenum coord, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetTexGenfv\n" );
	fflush( Stream );
	dllGetTexGenfv( coord, pname, params );
}

/* ---- logGetTexGeniv  0x004BD260 ---- */
static void QGLAPI logGetTexGeniv( GLenum coord, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetTexGeniv\n" );
	fflush( Stream );
	dllGetTexGeniv( coord, pname, params );
}

/* ---- logGetTexImage  0x004BD290 ---- */
static void QGLAPI logGetTexImage( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels ) {
	fprintf( Stream, "glGetTexImage\n" );
	fflush( Stream );
	dllGetTexImage( target, level, format, type, pixels );
}

/* ---- logGetTexLevelParameterfv  0x004BD2C0 ---- */
static void QGLAPI logGetTexLevelParameterfv( GLenum target, GLint level, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetTexLevelParameterfv\n" );
	fflush( Stream );
	dllGetTexLevelParameterfv( target, level, pname, params );
}

/* ---- logGetTexLevelParameteriv  0x004BD2F0 ---- */
static void QGLAPI logGetTexLevelParameteriv( GLenum target, GLint level, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetTexLevelParameteriv\n" );
	fflush( Stream );
	dllGetTexLevelParameteriv( target, level, pname, params );
}

/* ---- logGetTexParameterfv  0x004BD320 ---- */
static void QGLAPI logGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetTexParameterfv\n" );
	fflush( Stream );
	dllGetTexParameterfv( target, pname, params );
}

/* ---- logGetTexParameteriv  0x004BD350 ---- */
static void QGLAPI logGetTexParameteriv( GLenum target, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetTexParameteriv\n" );
	fflush( Stream );
	dllGetTexParameteriv( target, pname, params );
}

/* ---- logHint  0x004B9EB0 ---- */
static void QGLAPI logHint( GLenum target, GLenum mode ) {
	fprintf( Stream, "glHint( 0x%x, 0x%x )\n", target, mode );
	fflush( Stream );
	dllHint( target, mode );
}

/* ---- logIndexMask  0x004BD380 ---- */
static void QGLAPI logIndexMask( GLuint mask ) {
	fprintf( Stream, "glIndexMask\n" );
	fflush( Stream );
	dllIndexMask( mask );
}

/* ---- logIndexPointer  0x004BD3B0 ---- */
static void QGLAPI logIndexPointer( GLenum type, GLsizei stride, const GLvoid *pointer ) {
	fprintf( Stream, "glIndexPointer\n" );
	fflush( Stream );
	dllIndexPointer( type, stride, pointer );
}

/* ---- logIndexd  0x004BD3E0 ---- */
static void QGLAPI logIndexd( GLdouble c ) {
	fprintf( Stream, "glIndexd\n" );
	fflush( Stream );
	dllIndexd( c );
}

/* ---- logIndexdv  0x004BD420 ---- */
static void QGLAPI logIndexdv( const GLdouble *c ) {
	fprintf( Stream, "glIndexdv\n" );
	fflush( Stream );
	dllIndexdv( c );
}

/* ---- logIndexf  0x004BD450 ---- */
static void QGLAPI logIndexf( GLfloat c ) {
	fprintf( Stream, "glIndexf\n" );
	fflush( Stream );
	dllIndexf( c );
}

/* ---- logIndexfv  0x004BD480 ---- */
static void QGLAPI logIndexfv( const GLfloat *c ) {
	fprintf( Stream, "glIndexfv\n" );
	fflush( Stream );
	dllIndexfv( c );
}

/* ---- logIndexi  0x004BD4B0 ---- */
static void QGLAPI logIndexi( GLint c ) {
	fprintf( Stream, "glIndexi\n" );
	fflush( Stream );
	dllIndexi( c );
}

/* ---- logIndexiv  0x004BD4E0 ---- */
static void QGLAPI logIndexiv( const GLint *c ) {
	fprintf( Stream, "glIndexiv\n" );
	fflush( Stream );
	dllIndexiv( c );
}

/* ---- logIndexs  0x004BD510 ---- */
static void QGLAPI logIndexs( GLshort c ) {
	fprintf( Stream, "glIndexs\n" );
	fflush( Stream );
	dllIndexs( c );
}

/* ---- logIndexsv  0x004BD540 ---- */
static void QGLAPI logIndexsv( const GLshort *c ) {
	fprintf( Stream, "glIndexsv\n" );
	fflush( Stream );
	dllIndexsv( c );
}

/* ---- logIndexub  0x004BD570 ---- */
static void QGLAPI logIndexub( GLubyte c ) {
	fprintf( Stream, "glIndexub\n" );
	fflush( Stream );
	dllIndexub( c );
}

/* ---- logIndexubv  0x004BD5A0 ---- */
static void QGLAPI logIndexubv( const GLubyte *c ) {
	fprintf( Stream, "glIndexubv\n" );
	fflush( Stream );
	dllIndexubv( c );
}

/* ---- logInitNames  0x004BD5D0 ---- */
static void QGLAPI logInitNames( void ) {
	fprintf( Stream, "glInitNames\n" );
	fflush( Stream );
	dllInitNames();
}

/* ---- logInterleavedArrays  0x004BD600 ---- */
static void QGLAPI logInterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer ) {
	fprintf( Stream, "glInterleavedArrays\n" );
	fflush( Stream );
	dllInterleavedArrays( format, stride, pointer );
}

/* ---- logIsEnabled  0x004BD630 ---- */
static GLboolean QGLAPI logIsEnabled( GLenum cap ) {
	fprintf( Stream, "glIsEnabled\n" );
	fflush( Stream );
	return dllIsEnabled( cap );
}

/* ---- logIsList  0x004BD660 ---- */
static GLboolean QGLAPI logIsList( GLuint list ) {
	fprintf( Stream, "glIsList\n" );
	fflush( Stream );
	return dllIsList( list );
}

/* ---- logIsTexture  0x004BD690 ---- */
static GLboolean QGLAPI logIsTexture( GLuint texture ) {
	fprintf( Stream, "glIsTexture\n" );
	fflush( Stream );
	return dllIsTexture( texture );
}

/* ---- logLightModelf  0x004BD6C0 ---- */
static void QGLAPI logLightModelf( GLenum pname, GLfloat param ) {
	fprintf( Stream, "glLightModelf\n" );
	fflush( Stream );
	dllLightModelf( pname, param );
}

/* ---- logLightModelfv  0x004BD6F0 ---- */
static void QGLAPI logLightModelfv( GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glLightModelfv\n" );
	fflush( Stream );
	dllLightModelfv( pname, params );
}

/* ---- logLightModeli  0x004BD720 ---- */
static void QGLAPI logLightModeli( GLenum pname, GLint param ) {
	fprintf( Stream, "glLightModeli\n" );
	fflush( Stream );
	dllLightModeli( pname, param );
}

/* ---- logLightModeliv  0x004BD750 ---- */
static void QGLAPI logLightModeliv( GLenum pname, const GLint *params ) {
	fprintf( Stream, "glLightModeliv\n" );
	fflush( Stream );
	dllLightModeliv( pname, params );
}

/* ---- logLightf  0x004BD780 ---- */
static void QGLAPI logLightf( GLenum light, GLenum pname, GLfloat param ) {
	fprintf( Stream, "glLightf\n" );
	fflush( Stream );
	dllLightf( light, pname, param );
}

/* ---- logLightfv  0x004BD7B0 ---- */
static void QGLAPI logLightfv( GLenum light, GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glLightfv\n" );
	fflush( Stream );
	dllLightfv( light, pname, params );
}

/* ---- logLighti  0x004BD7E0 ---- */
static void QGLAPI logLighti( GLenum light, GLenum pname, GLint param ) {
	fprintf( Stream, "glLighti\n" );
	fflush( Stream );
	dllLighti( light, pname, param );
}

/* ---- logLightiv  0x004BD810 ---- */
static void QGLAPI logLightiv( GLenum light, GLenum pname, const GLint *params ) {
	fprintf( Stream, "glLightiv\n" );
	fflush( Stream );
	dllLightiv( light, pname, params );
}

/* ---- logLineStipple  0x004BD840 ---- */
static void QGLAPI logLineStipple( GLint factor, GLushort pattern ) {
	fprintf( Stream, "glLineStipple\n" );
	fflush( Stream );
	dllLineStipple( factor, pattern );
}

/* ---- logLineWidth  0x004BD870 ---- */
static void QGLAPI logLineWidth( GLfloat width ) {
	fprintf( Stream, "glLineWidth\n" );
	fflush( Stream );
	dllLineWidth( width );
}

/* ---- logListBase  0x004BD8A0 ---- */
static void QGLAPI logListBase( GLuint base ) {
	fprintf( Stream, "glListBase\n" );
	fflush( Stream );
	dllListBase( base );
}

/* ---- logLoadIdentity  0x004BD8D0 ---- */
static void QGLAPI logLoadIdentity( void ) {
	fprintf( Stream, "glLoadIdentity\n" );
	fflush( Stream );
	dllLoadIdentity();
}

/* ---- logLoadMatrixd  0x004BD900 ---- */
static void QGLAPI logLoadMatrixd( const GLdouble *m ) {
	fprintf( Stream, "glLoadMatrixd\n" );
	fflush( Stream );
	dllLoadMatrixd( m );
}

/* ---- logLoadMatrixf  0x004BD930 ---- */
static void QGLAPI logLoadMatrixf( const GLfloat *m ) {
	fprintf( Stream, "glLoadMatrixf\n" );
	fflush( Stream );
	dllLoadMatrixf( m );
}

/* ---- logLoadName  0x004BD960 ---- */
static void QGLAPI logLoadName( GLuint name ) {
	fprintf( Stream, "glLoadName\n" );
	fflush( Stream );
	dllLoadName( name );
}

/* ---- logLogicOp  0x004BD990 ---- */
static void QGLAPI logLogicOp( GLenum opcode ) {
	fprintf( Stream, "glLogicOp\n" );
	fflush( Stream );
	dllLogicOp( opcode );
}

/* ---- logMap1d  0x004BD9C0 ---- */
static void QGLAPI logMap1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ) {
	fprintf( Stream, "glMap1d\n" );
	fflush( Stream );
	dllMap1d( target, u1, u2, stride, order, points );
}

/* ---- logMap1f  0x004BDA10 ---- */
static void QGLAPI logMap1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ) {
	fprintf( Stream, "glMap1f\n" );
	fflush( Stream );
	dllMap1f( target, u1, u2, stride, order, points );
}

/* ---- logMap2d  0x004BDA40 ---- */
static void QGLAPI logMap2d( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ) {
	fprintf( Stream, "glMap2d\n" );
	fflush( Stream );
	dllMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

/* ---- logMap2f  0x004BDAB0 ---- */
static void QGLAPI logMap2f( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ) {
	fprintf( Stream, "glMap2f\n" );
	fflush( Stream );
	dllMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

/* ---- logMapGrid1d  0x004BDAE0 ---- */
static void QGLAPI logMapGrid1d( GLint un, GLdouble u1, GLdouble u2 ) {
	fprintf( Stream, "glMapGrid1d\n" );
	fflush( Stream );
	dllMapGrid1d( un, u1, u2 );
}

/* ---- logMapGrid1f  0x004BDB30 ---- */
static void QGLAPI logMapGrid1f( GLint un, GLfloat u1, GLfloat u2 ) {
	fprintf( Stream, "glMapGrid1f\n" );
	fflush( Stream );
	dllMapGrid1f( un, u1, u2 );
}

/* ---- logMapGrid2d  0x004BDB60 ---- */
static void QGLAPI logMapGrid2d( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) {
	fprintf( Stream, "glMapGrid2d\n" );
	fflush( Stream );
	dllMapGrid2d( un, u1, u2, vn, v1, v2 );
}

/* ---- logMapGrid2f  0x004BDBC0 ---- */
static void QGLAPI logMapGrid2f( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) {
	fprintf( Stream, "glMapGrid2f\n" );
	fflush( Stream );
	dllMapGrid2f( un, u1, u2, vn, v1, v2 );
}

/* ---- logMaterialf  0x004BDBF0 ---- */
static void QGLAPI logMaterialf( GLenum face, GLenum pname, GLfloat param ) {
	fprintf( Stream, "glMaterialf\n" );
	fflush( Stream );
	dllMaterialf( face, pname, param );
}

/* ---- logMaterialfv  0x004BDC20 ---- */
static void QGLAPI logMaterialfv( GLenum face, GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glMaterialfv\n" );
	fflush( Stream );
	dllMaterialfv( face, pname, params );
}

/* ---- logMateriali  0x004BDC50 ---- */
static void QGLAPI logMateriali( GLenum face, GLenum pname, GLint param ) {
	fprintf( Stream, "glMateriali\n" );
	fflush( Stream );
	dllMateriali( face, pname, param );
}

/* ---- logMaterialiv  0x004BDC80 ---- */
static void QGLAPI logMaterialiv( GLenum face, GLenum pname, const GLint *params ) {
	fprintf( Stream, "glMaterialiv\n" );
	fflush( Stream );
	dllMaterialiv( face, pname, params );
}

/* ---- logMatrixMode  0x004BDCB0 ---- */
static void QGLAPI logMatrixMode( GLenum mode ) {
	fprintf( Stream, "glMatrixMode\n" );
	fflush( Stream );
	dllMatrixMode( mode );
}

/* ---- logMultMatrixd  0x004BDCE0 ---- */
static void QGLAPI logMultMatrixd( const GLdouble *m ) {
	fprintf( Stream, "glMultMatrixd\n" );
	fflush( Stream );
	dllMultMatrixd( m );
}

/* ---- logMultMatrixf  0x004BDD10 ---- */
static void QGLAPI logMultMatrixf( const GLfloat *m ) {
	fprintf( Stream, "glMultMatrixf\n" );
	fflush( Stream );
	dllMultMatrixf( m );
}

/* ---- logNewList  0x004BDD40 ---- */
static void QGLAPI logNewList( GLuint list, GLenum mode ) {
	fprintf( Stream, "glNewList\n" );
	fflush( Stream );
	dllNewList( list, mode );
}

/* ---- logNormal3b  0x004BDD70 ---- */
static void QGLAPI logNormal3b( GLbyte nx, GLbyte ny, GLbyte nz ) {
	fprintf( Stream, "glNormal3b\n" );
	fflush( Stream );
	dllNormal3b( nx, ny, nz );
}

/* ---- logNormal3bv  0x004BDDA0 ---- */
static void QGLAPI logNormal3bv( const GLbyte *v ) {
	fprintf( Stream, "glNormal3bv\n" );
	fflush( Stream );
	dllNormal3bv( v );
}

/* ---- logNormal3d  0x004BDDD0 ---- */
static void QGLAPI logNormal3d( GLdouble nx, GLdouble ny, GLdouble nz ) {
	fprintf( Stream, "glNormal3d\n" );
	fflush( Stream );
	dllNormal3d( nx, ny, nz );
}

/* ---- logNormal3dv  0x004BDE20 ---- */
static void QGLAPI logNormal3dv( const GLdouble *v ) {
	fprintf( Stream, "glNormal3dv\n" );
	fflush( Stream );
	dllNormal3dv( v );
}

/* ---- logNormal3f  0x004BDE50 ---- */
static void QGLAPI logNormal3f( GLfloat nx, GLfloat ny, GLfloat nz ) {
	fprintf( Stream, "glNormal3f\n" );
	fflush( Stream );
	dllNormal3f( nx, ny, nz );
}

/* ---- logNormal3fv  0x004BDE80 ---- */
static void QGLAPI logNormal3fv( const GLfloat *v ) {
	fprintf( Stream, "glNormal3fv\n" );
	fflush( Stream );
	dllNormal3fv( v );
}

/* ---- logNormal3i  0x004BDEB0 ---- */
static void QGLAPI logNormal3i( GLint nx, GLint ny, GLint nz ) {
	fprintf( Stream, "glNormal3i\n" );
	fflush( Stream );
	dllNormal3i( nx, ny, nz );
}

/* ---- logNormal3iv  0x004BDEE0 ---- */
static void QGLAPI logNormal3iv( const GLint *v ) {
	fprintf( Stream, "glNormal3iv\n" );
	fflush( Stream );
	dllNormal3iv( v );
}

/* ---- logNormal3s  0x004BDF10 ---- */
static void QGLAPI logNormal3s( GLshort nx, GLshort ny, GLshort nz ) {
	fprintf( Stream, "glNormal3s\n" );
	fflush( Stream );
	dllNormal3s( nx, ny, nz );
}

/* ---- logNormal3sv  0x004BDF40 ---- */
static void QGLAPI logNormal3sv( const GLshort *v ) {
	fprintf( Stream, "glNormal3sv\n" );
	fflush( Stream );
	dllNormal3sv( v );
}

/* ---- logNormalPointer  0x004BA3A0 ---- */
static void QGLAPI logNormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer ) {
	const char *pointerName = PointerToString( pointer );

	fprintf( Stream, "glNormalPointer( %s, %d, %s )\n", TypeToString( type ), stride, pointerName );
	fflush( Stream );
	dllNormalPointer( type, stride, pointer );
}

/* ---- logOrtho  0x004BDF70 ---- */
static void QGLAPI logOrtho( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) {
	fprintf( Stream, "glOrtho\n" );
	fflush( Stream );
	dllOrtho( left, right, bottom, top, zNear, zFar );
}

/* ---- logPassThrough  0x004BDFE0 ---- */
static void QGLAPI logPassThrough( GLfloat token ) {
	fprintf( Stream, "glPassThrough\n" );
	fflush( Stream );
	dllPassThrough( token );
}

/* ---- logPixelMapfv  0x004BE010 ---- */
static void QGLAPI logPixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values ) {
	fprintf( Stream, "glPixelMapfv\n" );
	fflush( Stream );
	dllPixelMapfv( map, mapsize, values );
}

/* ---- logPixelMapuiv  0x004BE040 ---- */
static void QGLAPI logPixelMapuiv( GLenum map, GLsizei mapsize, const GLuint *values ) {
	fprintf( Stream, "glPixelMapuiv\n" );
	fflush( Stream );
	dllPixelMapuiv( map, mapsize, values );
}

/* ---- logPixelMapusv  0x004BE070 ---- */
static void QGLAPI logPixelMapusv( GLenum map, GLsizei mapsize, const GLushort *values ) {
	fprintf( Stream, "glPixelMapusv\n" );
	fflush( Stream );
	dllPixelMapusv( map, mapsize, values );
}

/* ---- logPixelStoref  0x004BE0A0 ---- */
static void QGLAPI logPixelStoref( GLenum pname, GLfloat param ) {
	fprintf( Stream, "glPixelStoref\n" );
	fflush( Stream );
	dllPixelStoref( pname, param );
}

/* ---- logPixelStorei  0x004BE0D0 ---- */
static void QGLAPI logPixelStorei( GLenum pname, GLint param ) {
	fprintf( Stream, "glPixelStorei\n" );
	fflush( Stream );
	dllPixelStorei( pname, param );
}

/* ---- logPixelTransferf  0x004BE100 ---- */
static void QGLAPI logPixelTransferf( GLenum pname, GLfloat param ) {
	fprintf( Stream, "glPixelTransferf\n" );
	fflush( Stream );
	dllPixelTransferf( pname, param );
}

/* ---- logPixelTransferi  0x004BE130 ---- */
static void QGLAPI logPixelTransferi( GLenum pname, GLint param ) {
	fprintf( Stream, "glPixelTransferi\n" );
	fflush( Stream );
	dllPixelTransferi( pname, param );
}

/* ---- logPixelZoom  0x004BE160 ---- */
static void QGLAPI logPixelZoom( GLfloat xfactor, GLfloat yfactor ) {
	fprintf( Stream, "glPixelZoom\n" );
	fflush( Stream );
	dllPixelZoom( xfactor, yfactor );
}

/* ---- logPointSize  0x004BE190 ---- */
static void QGLAPI logPointSize( GLfloat size ) {
	fprintf( Stream, "glPointSize\n" );
	fflush( Stream );
	dllPointSize( size );
}

/* ---- logPolygonMode  0x004BA050 ---- */
static void QGLAPI logPolygonMode( GLenum face, GLenum mode ) {
	char faceName[1024];

	/* face is copied out before mode can reuse CapToString's buffer */
	strcpy( faceName, CapToString( face ) );
	fprintf( Stream, "glPolygonMode( %s, %s )\n", faceName, CapToString( mode ) );
	fflush( Stream );
	dllPolygonMode( face, mode );
}

/* ---- logPolygonOffset  0x004BE1C0 ---- */
static void QGLAPI logPolygonOffset( GLfloat factor, GLfloat units ) {
	fprintf( Stream, "glPolygonOffset\n" );
	fflush( Stream );
	dllPolygonOffset( factor, units );
}

/* ---- logPolygonStipple  0x004BE1F0 ---- */
static void QGLAPI logPolygonStipple( const GLubyte *mask ) {
	fprintf( Stream, "glPolygonStipple\n" );
	fflush( Stream );
	dllPolygonStipple( mask );
}

/* ---- logPopAttrib  0x004BE220 ---- */
static void QGLAPI logPopAttrib( void ) {
	fprintf( Stream, "glPopAttrib\n" );
	fflush( Stream );
	dllPopAttrib();
}

/* ---- logPopClientAttrib  0x004BE250 ---- */
static void QGLAPI logPopClientAttrib( void ) {
	fprintf( Stream, "glPopClientAttrib\n" );
	fflush( Stream );
	dllPopClientAttrib();
}

/* ---- logPopMatrix  0x004BE280 ---- */
static void QGLAPI logPopMatrix( void ) {
	fprintf( Stream, "glPopMatrix\n" );
	fflush( Stream );
	dllPopMatrix();
}

/* ---- logPopName  0x004BE2B0 ---- */
static void QGLAPI logPopName( void ) {
	fprintf( Stream, "glPopName\n" );
	fflush( Stream );
	dllPopName();
}

/* ---- logPrioritizeTextures  0x004BE2E0 ---- */
static void QGLAPI logPrioritizeTextures( GLsizei n, const GLuint *textures, const GLclampf *priorities ) {
	fprintf( Stream, "glPrioritizeTextures\n" );
	fflush( Stream );
	dllPrioritizeTextures( n, textures, priorities );
}

/* ---- logPushAttrib  0x004BE310 ---- */
static void QGLAPI logPushAttrib( GLbitfield mask ) {
	fprintf( Stream, "glPushAttrib\n" );
	fflush( Stream );
	dllPushAttrib( mask );
}

/* ---- logPushClientAttrib  0x004BE340 ---- */
static void QGLAPI logPushClientAttrib( GLbitfield mask ) {
	fprintf( Stream, "glPushClientAttrib\n" );
	fflush( Stream );
	dllPushClientAttrib( mask );
}

/* ---- logPushMatrix  0x004BE370 ---- */
static void QGLAPI logPushMatrix( void ) {
	fprintf( Stream, "glPushMatrix\n" );
	fflush( Stream );
	dllPushMatrix();
}

/* ---- logPushName  0x004BE3A0 ---- */
static void QGLAPI logPushName( GLuint name ) {
	fprintf( Stream, "glPushName\n" );
	fflush( Stream );
	dllPushName( name );
}

/* ---- logRasterPos2d  0x004BE3D0 ---- */
static void QGLAPI logRasterPos2d( GLdouble x, GLdouble y ) {
	fprintf( Stream, "glRasterPos2d\n" );
	fflush( Stream );
	dllRasterPos2d( x, y );
}

/* ---- logRasterPos2dv  0x004BE410 ---- */
static void QGLAPI logRasterPos2dv( const GLdouble *v ) {
	fprintf( Stream, "glRasterPos2dv\n" );
	fflush( Stream );
	dllRasterPos2dv( v );
}

/* ---- logRasterPos2f  0x004BE440 ---- */
static void QGLAPI logRasterPos2f( GLfloat x, GLfloat y ) {
	fprintf( Stream, "glRasterPos2f\n" );
	fflush( Stream );
	dllRasterPos2f( x, y );
}

/* ---- logRasterPos2fv  0x004BE470 ---- */
static void QGLAPI logRasterPos2fv( const GLfloat *v ) {
	fprintf( Stream, "glRasterPos2fv\n" );
	fflush( Stream );
	dllRasterPos2fv( v );
}

/* ---- logRasterPos2i  0x004BE4A0 ---- */
static void QGLAPI logRasterPos2i( GLint x, GLint y ) {
	fprintf( Stream, "glRasterPos2i\n" );
	fflush( Stream );
	dllRasterPos2i( x, y );
}

/* ---- logRasterPos2iv  0x004BE4D0 ---- */
static void QGLAPI logRasterPos2iv( const GLint *v ) {
	fprintf( Stream, "glRasterPos2iv\n" );
	fflush( Stream );
	dllRasterPos2iv( v );
}

/* ---- logRasterPos2s  0x004BE500 ---- */
static void QGLAPI logRasterPos2s( GLshort x, GLshort y ) {
	fprintf( Stream, "glRasterPos2s\n" );
	fflush( Stream );
	dllRasterPos2s( x, y );
}

/* ---- logRasterPos2sv  0x004BE530 ---- */
static void QGLAPI logRasterPos2sv( const GLshort *v ) {
	fprintf( Stream, "glRasterPos2sv\n" );
	fflush( Stream );
	dllRasterPos2sv( v );
}

/* ---- logRasterPos3d  0x004BE560 ---- */
static void QGLAPI logRasterPos3d( GLdouble x, GLdouble y, GLdouble z ) {
	fprintf( Stream, "glRasterPos3d\n" );
	fflush( Stream );
	dllRasterPos3d( x, y, z );
}

/* ---- logRasterPos3dv  0x004BE5B0 ---- */
static void QGLAPI logRasterPos3dv( const GLdouble *v ) {
	fprintf( Stream, "glRasterPos3dv\n" );
	fflush( Stream );
	dllRasterPos3dv( v );
}

/* ---- logRasterPos3f  0x004BE5E0 ---- */
static void QGLAPI logRasterPos3f( GLfloat x, GLfloat y, GLfloat z ) {
	fprintf( Stream, "glRasterPos3f\n" );
	fflush( Stream );
	dllRasterPos3f( x, y, z );
}

/* ---- logRasterPos3fv  0x004BE610 ---- */
static void QGLAPI logRasterPos3fv( const GLfloat *v ) {
	fprintf( Stream, "glRasterPos3fv\n" );
	fflush( Stream );
	dllRasterPos3fv( v );
}

/* ---- logRasterPos3i  0x004BE640 ---- */
static void QGLAPI logRasterPos3i( GLint x, GLint y, GLint z ) {
	fprintf( Stream, "glRasterPos3i\n" );
	fflush( Stream );
	dllRasterPos3i( x, y, z );
}

/* ---- logRasterPos3iv  0x004BE670 ---- */
static void QGLAPI logRasterPos3iv( const GLint *v ) {
	fprintf( Stream, "glRasterPos3iv\n" );
	fflush( Stream );
	dllRasterPos3iv( v );
}

/* ---- logRasterPos3s  0x004BE6A0 ---- */
static void QGLAPI logRasterPos3s( GLshort x, GLshort y, GLshort z ) {
	fprintf( Stream, "glRasterPos3s\n" );
	fflush( Stream );
	dllRasterPos3s( x, y, z );
}

/* ---- logRasterPos3sv  0x004BE6D0 ---- */
static void QGLAPI logRasterPos3sv( const GLshort *v ) {
	fprintf( Stream, "glRasterPos3sv\n" );
	fflush( Stream );
	dllRasterPos3sv( v );
}

/* ---- logRasterPos4d  0x004BE700 ---- */
static void QGLAPI logRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	fprintf( Stream, "glRasterPos4d\n" );
	fflush( Stream );
	dllRasterPos4d( x, y, z, w );
}

/* ---- logRasterPos4dv  0x004BE750 ---- */
static void QGLAPI logRasterPos4dv( const GLdouble *v ) {
	fprintf( Stream, "glRasterPos4dv\n" );
	fflush( Stream );
	dllRasterPos4dv( v );
}

/* ---- logRasterPos4f  0x004BE780 ---- */
static void QGLAPI logRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	fprintf( Stream, "glRasterPos4f\n" );
	fflush( Stream );
	dllRasterPos4f( x, y, z, w );
}

/* ---- logRasterPos4fv  0x004BE7B0 ---- */
static void QGLAPI logRasterPos4fv( const GLfloat *v ) {
	fprintf( Stream, "glRasterPos4fv\n" );
	fflush( Stream );
	dllRasterPos4fv( v );
}

/* ---- logRasterPos4i  0x004BE7E0 ---- */
static void QGLAPI logRasterPos4i( GLint x, GLint y, GLint z, GLint w ) {
	fprintf( Stream, "glRasterPos4i\n" );
	fflush( Stream );
	dllRasterPos4i( x, y, z, w );
}

/* ---- logRasterPos4iv  0x004BE810 ---- */
static void QGLAPI logRasterPos4iv( const GLint *v ) {
	fprintf( Stream, "glRasterPos4iv\n" );
	fflush( Stream );
	dllRasterPos4iv( v );
}

/* ---- logRasterPos4s  0x004BE840 ---- */
static void QGLAPI logRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
	fprintf( Stream, "glRasterPos4s\n" );
	fflush( Stream );
	dllRasterPos4s( x, y, z, w );
}

/* ---- logRasterPos4sv  0x004BE870 ---- */
static void QGLAPI logRasterPos4sv( const GLshort *v ) {
	fprintf( Stream, "glRasterPos4sv\n" );
	fflush( Stream );
	dllRasterPos4sv( v );
}

/* ---- logReadBuffer  0x004BE8A0 ---- */
static void QGLAPI logReadBuffer( GLenum mode ) {
	fprintf( Stream, "glReadBuffer\n" );
	fflush( Stream );
	dllReadBuffer( mode );
}

/* ---- logReadPixels  0x004BE8D0 ---- */
static void QGLAPI logReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) {
	fprintf( Stream, "glReadPixels\n" );
	fflush( Stream );
	dllReadPixels( x, y, width, height, format, type, pixels );
}

/* ---- logRectd  0x004BE900 ---- */
static void QGLAPI logRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) {
	fprintf( Stream, "glRectd\n" );
	fflush( Stream );
	dllRectd( x1, y1, x2, y2 );
}

/* ---- logRectdv  0x004BE950 ---- */
static void QGLAPI logRectdv( const GLdouble *v1, const GLdouble *v2 ) {
	fprintf( Stream, "glRectdv\n" );
	fflush( Stream );
	dllRectdv( v1, v2 );
}

/* ---- logRectf  0x004BE980 ---- */
static void QGLAPI logRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) {
	fprintf( Stream, "glRectf\n" );
	fflush( Stream );
	dllRectf( x1, y1, x2, y2 );
}

/* ---- logRectfv  0x004BE9B0 ---- */
static void QGLAPI logRectfv( const GLfloat *v1, const GLfloat *v2 ) {
	fprintf( Stream, "glRectfv\n" );
	fflush( Stream );
	dllRectfv( v1, v2 );
}

/* ---- logRecti  0x004BE9E0 ---- */
static void QGLAPI logRecti( GLint x1, GLint y1, GLint x2, GLint y2 ) {
	fprintf( Stream, "glRecti\n" );
	fflush( Stream );
	dllRecti( x1, y1, x2, y2 );
}

/* ---- logRectiv  0x004BEA10 ---- */
static void QGLAPI logRectiv( const GLint *v1, const GLint *v2 ) {
	fprintf( Stream, "glRectiv\n" );
	fflush( Stream );
	dllRectiv( v1, v2 );
}

/* ---- logRects  0x004BEA40 ---- */
static void QGLAPI logRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) {
	fprintf( Stream, "glRects\n" );
	fflush( Stream );
	dllRects( x1, y1, x2, y2 );
}

/* ---- logRectsv  0x004BEA70 ---- */
static void QGLAPI logRectsv( const GLshort *v1, const GLshort *v2 ) {
	fprintf( Stream, "glRectsv\n" );
	fflush( Stream );
	dllRectsv( v1, v2 );
}

/* ---- logRenderMode  0x004BEAA0 ---- */
static GLint QGLAPI logRenderMode( GLenum mode ) {
	fprintf( Stream, "glRenderMode\n" );
	fflush( Stream );
	return dllRenderMode( mode );
}

/* ---- logRotated  0x004BEAD0 ---- */
static void QGLAPI logRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) {
	fprintf( Stream, "glRotated\n" );
	fflush( Stream );
	dllRotated( angle, x, y, z );
}

/* ---- logRotatef  0x004BEB20 ---- */
static void QGLAPI logRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) {
	fprintf( Stream, "glRotatef\n" );
	fflush( Stream );
	dllRotatef( angle, x, y, z );
}

/* ---- logScaled  0x004BEB50 ---- */
static void QGLAPI logScaled( GLdouble x, GLdouble y, GLdouble z ) {
	fprintf( Stream, "glScaled\n" );
	fflush( Stream );
	dllScaled( x, y, z );
}

/* ---- logScalef  0x004BEBA0 ---- */
static void QGLAPI logScalef( GLfloat x, GLfloat y, GLfloat z ) {
	fprintf( Stream, "glScalef\n" );
	fflush( Stream );
	dllScalef( x, y, z );
}

/* ---- logScissor  0x004BA0F0 ---- */
static void QGLAPI logScissor( GLint x, GLint y, GLsizei width, GLsizei height ) {
	fprintf( Stream, "glScissor( %d, %d, %d, %d )\n", x, y, width, height );
	fflush( Stream );
	dllScissor( x, y, width, height );
}

/* ---- logSelectBuffer  0x004BEBD0 ---- */
static void QGLAPI logSelectBuffer( GLsizei size, GLuint *buffer ) {
	fprintf( Stream, "glSelectBuffer\n" );
	fflush( Stream );
	dllSelectBuffer( size, buffer );
}

/* ---- logShadeModel  0x004BEC00 ---- */
static void QGLAPI logShadeModel( GLenum mode ) {
	fprintf( Stream, "glShadeModel\n" );
	fflush( Stream );
	dllShadeModel( mode );
}

/* ---- logStencilFunc  0x004BEC30 ---- */
static void QGLAPI logStencilFunc( GLenum func, GLint ref, GLuint mask ) {
	fprintf( Stream, "glStencilFunc\n" );
	fflush( Stream );
	dllStencilFunc( func, ref, mask );
}

/* ---- logStencilMask  0x004BEC60 ---- */
static void QGLAPI logStencilMask( GLuint mask ) {
	fprintf( Stream, "glStencilMask\n" );
	fflush( Stream );
	dllStencilMask( mask );
}

/* ---- logStencilOp  0x004BEC90 ---- */
static void QGLAPI logStencilOp( GLenum fail, GLenum zfail, GLenum zpass ) {
	fprintf( Stream, "glStencilOp\n" );
	fflush( Stream );
	dllStencilOp( fail, zfail, zpass );
}

/* ---- logTexCoord1d  0x004BECC0 ---- */
static void QGLAPI logTexCoord1d( GLdouble s ) {
	fprintf( Stream, "glTexCoord1d\n" );
	fflush( Stream );
	dllTexCoord1d( s );
}

/* ---- logTexCoord1dv  0x004BED00 ---- */
static void QGLAPI logTexCoord1dv( const GLdouble *v ) {
	fprintf( Stream, "glTexCoord1dv\n" );
	fflush( Stream );
	dllTexCoord1dv( v );
}

/* ---- logTexCoord1f  0x004BED30 ---- */
static void QGLAPI logTexCoord1f( GLfloat s ) {
	fprintf( Stream, "glTexCoord1f\n" );
	fflush( Stream );
	dllTexCoord1f( s );
}

/* ---- logTexCoord1fv  0x004BED60 ---- */
static void QGLAPI logTexCoord1fv( const GLfloat *v ) {
	fprintf( Stream, "glTexCoord1fv\n" );
	fflush( Stream );
	dllTexCoord1fv( v );
}

/* ---- logTexCoord1i  0x004BED90 ---- */
static void QGLAPI logTexCoord1i( GLint s ) {
	fprintf( Stream, "glTexCoord1i\n" );
	fflush( Stream );
	dllTexCoord1i( s );
}

/* ---- logTexCoord1iv  0x004BEDC0 ---- */
static void QGLAPI logTexCoord1iv( const GLint *v ) {
	fprintf( Stream, "glTexCoord1iv\n" );
	fflush( Stream );
	dllTexCoord1iv( v );
}

/* ---- logTexCoord1s  0x004BEDF0 ---- */
static void QGLAPI logTexCoord1s( GLshort s ) {
	fprintf( Stream, "glTexCoord1s\n" );
	fflush( Stream );
	dllTexCoord1s( s );
}

/* ---- logTexCoord1sv  0x004BEE20 ---- */
static void QGLAPI logTexCoord1sv( const GLshort *v ) {
	fprintf( Stream, "glTexCoord1sv\n" );
	fflush( Stream );
	dllTexCoord1sv( v );
}

/* ---- logTexCoord2d  0x004BEE50 ---- */
static void QGLAPI logTexCoord2d( GLdouble s, GLdouble t ) {
	fprintf( Stream, "glTexCoord2d\n" );
	fflush( Stream );
	dllTexCoord2d( s, t );
}

/* ---- logTexCoord2dv  0x004BEE90 ---- */
static void QGLAPI logTexCoord2dv( const GLdouble *v ) {
	fprintf( Stream, "glTexCoord2dv\n" );
	fflush( Stream );
	dllTexCoord2dv( v );
}

/* ---- logTexCoord2f  0x004BEEC0 ---- */
static void QGLAPI logTexCoord2f( GLfloat s, GLfloat t ) {
	fprintf( Stream, "glTexCoord2f\n" );
	fflush( Stream );
	dllTexCoord2f( s, t );
}

/* ---- logTexCoord2fv  0x004BEEF0 ---- */
static void QGLAPI logTexCoord2fv( const GLfloat *v ) {
	fprintf( Stream, "glTexCoord2fv\n" );
	fflush( Stream );
	dllTexCoord2fv( v );
}

/* ---- logTexCoord2i  0x004BEF20 ---- */
static void QGLAPI logTexCoord2i( GLint s, GLint t ) {
	fprintf( Stream, "glTexCoord2i\n" );
	fflush( Stream );
	dllTexCoord2i( s, t );
}

/* ---- logTexCoord2iv  0x004BEF50 ---- */
static void QGLAPI logTexCoord2iv( const GLint *v ) {
	fprintf( Stream, "glTexCoord2iv\n" );
	fflush( Stream );
	dllTexCoord2iv( v );
}

/* ---- logTexCoord2s  0x004BEF80 ---- */
static void QGLAPI logTexCoord2s( GLshort s, GLshort t ) {
	fprintf( Stream, "glTexCoord2s\n" );
	fflush( Stream );
	dllTexCoord2s( s, t );
}

/* ---- logTexCoord2sv  0x004BEFB0 ---- */
static void QGLAPI logTexCoord2sv( const GLshort *v ) {
	fprintf( Stream, "glTexCoord2sv\n" );
	fflush( Stream );
	dllTexCoord2sv( v );
}

/* ---- logTexCoord3d  0x004BEFE0 ---- */
static void QGLAPI logTexCoord3d( GLdouble s, GLdouble t, GLdouble r ) {
	fprintf( Stream, "glTexCoord3d\n" );
	fflush( Stream );
	dllTexCoord3d( s, t, r );
}

/* ---- logTexCoord3dv  0x004BF030 ---- */
static void QGLAPI logTexCoord3dv( const GLdouble *v ) {
	fprintf( Stream, "glTexCoord3dv\n" );
	fflush( Stream );
	dllTexCoord3dv( v );
}

/* ---- logTexCoord3f  0x004BF060 ---- */
static void QGLAPI logTexCoord3f( GLfloat s, GLfloat t, GLfloat r ) {
	fprintf( Stream, "glTexCoord3f\n" );
	fflush( Stream );
	dllTexCoord3f( s, t, r );
}

/* ---- logTexCoord3fv  0x004BF090 ---- */
static void QGLAPI logTexCoord3fv( const GLfloat *v ) {
	fprintf( Stream, "glTexCoord3fv\n" );
	fflush( Stream );
	dllTexCoord3fv( v );
}

/* ---- logTexCoord3i  0x004BF0C0 ---- */
static void QGLAPI logTexCoord3i( GLint s, GLint t, GLint r ) {
	fprintf( Stream, "glTexCoord3i\n" );
	fflush( Stream );
	dllTexCoord3i( s, t, r );
}

/* ---- logTexCoord3iv  0x004BF0F0 ---- */
static void QGLAPI logTexCoord3iv( const GLint *v ) {
	fprintf( Stream, "glTexCoord3iv\n" );
	fflush( Stream );
	dllTexCoord3iv( v );
}

/* ---- logTexCoord3s  0x004BF120 ---- */
static void QGLAPI logTexCoord3s( GLshort s, GLshort t, GLshort r ) {
	fprintf( Stream, "glTexCoord3s\n" );
	fflush( Stream );
	dllTexCoord3s( s, t, r );
}

/* ---- logTexCoord3sv  0x004BF150 ---- */
static void QGLAPI logTexCoord3sv( const GLshort *v ) {
	fprintf( Stream, "glTexCoord3sv\n" );
	fflush( Stream );
	dllTexCoord3sv( v );
}

/* ---- logTexCoord4d  0x004BF180 ---- */
static void QGLAPI logTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) {
	fprintf( Stream, "glTexCoord4d\n" );
	fflush( Stream );
	dllTexCoord4d( s, t, r, q );
}

/* ---- logTexCoord4dv  0x004BF1D0 ---- */
static void QGLAPI logTexCoord4dv( const GLdouble *v ) {
	fprintf( Stream, "glTexCoord4dv\n" );
	fflush( Stream );
	dllTexCoord4dv( v );
}

/* ---- logTexCoord4f  0x004BF200 ---- */
static void QGLAPI logTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) {
	fprintf( Stream, "glTexCoord4f\n" );
	fflush( Stream );
	dllTexCoord4f( s, t, r, q );
}

/* ---- logTexCoord4fv  0x004BF230 ---- */
static void QGLAPI logTexCoord4fv( const GLfloat *v ) {
	fprintf( Stream, "glTexCoord4fv\n" );
	fflush( Stream );
	dllTexCoord4fv( v );
}

/* ---- logTexCoord4i  0x004BF260 ---- */
static void QGLAPI logTexCoord4i( GLint s, GLint t, GLint r, GLint q ) {
	fprintf( Stream, "glTexCoord4i\n" );
	fflush( Stream );
	dllTexCoord4i( s, t, r, q );
}

/* ---- logTexCoord4iv  0x004BF290 ---- */
static void QGLAPI logTexCoord4iv( const GLint *v ) {
	fprintf( Stream, "glTexCoord4iv\n" );
	fflush( Stream );
	dllTexCoord4iv( v );
}

/* ---- logTexCoord4s  0x004BF2C0 ---- */
static void QGLAPI logTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q ) {
	fprintf( Stream, "glTexCoord4s\n" );
	fflush( Stream );
	dllTexCoord4s( s, t, r, q );
}

/* ---- logTexCoord4sv  0x004BF2F0 ---- */
static void QGLAPI logTexCoord4sv( const GLshort *v ) {
	fprintf( Stream, "glTexCoord4sv\n" );
	fflush( Stream );
	dllTexCoord4sv( v );
}

/* ---- logTexCoordPointer  0x004BA140 ---- */
static void QGLAPI logTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	const char *pointerName = PointerToString( pointer );

	fprintf( Stream, "glTexCoordPointer( %d, %s, %d, %s )\n", size, TypeToString( type ), stride, pointerName );
	fflush( Stream );
	dllTexCoordPointer( size, type, stride, pointer );
}

/* ---- logTexEnvf  0x004BA1A0 ---- */
static void QGLAPI logTexEnvf( GLenum target, GLenum pname, GLfloat param ) {
	char targetName[1024];

	strcpy( targetName, CapToString( target ) );
	fprintf( Stream, "glTexEnvf( %s, %s, %g )\n", targetName, CapToString( pname ), param );
	fflush( Stream );
	dllTexEnvf( target, pname, param );
}

/* ---- logTexEnvfv  0x004BF320 ---- */
static void QGLAPI logTexEnvfv( GLenum target, GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glTexEnvfv\n" );
	fflush( Stream );
	dllTexEnvfv( target, pname, params );
}

/* ---- logTexEnvi  0x004BA250 ---- */
static void QGLAPI logTexEnvi( GLenum target, GLenum pname, GLint param ) {
	char targetName[1024];
	char paramName[1024];

	/* target and param are copied out; pname is converted last */
	strcpy( targetName, CapToString( target ) );
	strcpy( paramName, CapToString( param ) );
	fprintf( Stream, "glTexEnvi( %s, %s, %s )\n", targetName, CapToString( pname ), paramName );
	fflush( Stream );
	dllTexEnvi( target, pname, param );
}

/* ---- logTexEnviv  0x004BF350 ---- */
static void QGLAPI logTexEnviv( GLenum target, GLenum pname, const GLint *params ) {
	fprintf( Stream, "glTexEnviv\n" );
	fflush( Stream );
	dllTexEnviv( target, pname, params );
}

/* ---- logTexGend  0x004BF380 ---- */
static void QGLAPI logTexGend( GLenum coord, GLenum pname, GLdouble param ) {
	fprintf( Stream, "glTexGend\n" );
	fflush( Stream );
	dllTexGend( coord, pname, param );
}

/* ---- logTexGendv  0x004BF3C0 ---- */
static void QGLAPI logTexGendv( GLenum coord, GLenum pname, const GLdouble *params ) {
	fprintf( Stream, "glTexGendv\n" );
	fflush( Stream );
	dllTexGendv( coord, pname, params );
}

/* ---- logTexGenf  0x004BF3F0 ---- */
static void QGLAPI logTexGenf( GLenum coord, GLenum pname, GLfloat param ) {
	fprintf( Stream, "glTexGenf\n" );
	fflush( Stream );
	dllTexGenf( coord, pname, param );
}

/* ---- logTexGenfv  0x004BF420 ---- */
static void QGLAPI logTexGenfv( GLenum coord, GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glTexGenfv\n" );
	fflush( Stream );
	dllTexGenfv( coord, pname, params );
}

/* ---- logTexGeni  0x004BF450 ---- */
static void QGLAPI logTexGeni( GLenum coord, GLenum pname, GLint param ) {
	fprintf( Stream, "glTexGeni\n" );
	fflush( Stream );
	dllTexGeni( coord, pname, param );
}

/* ---- logTexGeniv  0x004BF480 ---- */
static void QGLAPI logTexGeniv( GLenum coord, GLenum pname, const GLint *params ) {
	fprintf( Stream, "glTexGeniv\n" );
	fflush( Stream );
	dllTexGeniv( coord, pname, params );
}

/* ---- logTexImage1D  0x004BF4B0 ---- */
static void QGLAPI logTexImage1D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
	fprintf( Stream, "glTexImage1D\n" );
	fflush( Stream );
	dllTexImage1D( target, level, internalformat, width, border, format, type, pixels );
}

/* ---- logTexImage2D  0x004BF4E0 ---- */
static void QGLAPI logTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels ) {
	fprintf( Stream, "glTexImage2D\n" );
	fflush( Stream );
	dllTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

/* ---- logTexParameterf  0x004BA310 ---- */
static void QGLAPI logTexParameterf( GLenum target, GLenum pname, GLfloat param ) {
	fprintf( Stream, "glTexParameterf( 0x%x, 0x%x, %f )\n", target, pname, param );
	fflush( Stream );
	dllTexParameterf( target, pname, param );
}

/* ---- logTexParameterfv  0x004BF510 ---- */
static void QGLAPI logTexParameterfv( GLenum target, GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glTexParameterfv\n" );
	fflush( Stream );
	dllTexParameterfv( target, pname, params );
}

/* ---- logTexParameteri  0x004BA360 ---- */
static void QGLAPI logTexParameteri( GLenum target, GLenum pname, GLint param ) {
	fprintf( Stream, "glTexParameteri( 0x%x, 0x%x, 0x%x )\n", target, pname, param );
	fflush( Stream );
	dllTexParameteri( target, pname, param );
}

/* ---- logTexParameteriv  0x004BF540 ---- */
static void QGLAPI logTexParameteriv( GLenum target, GLenum pname, const GLint *params ) {
	fprintf( Stream, "glTexParameteriv\n" );
	fflush( Stream );
	dllTexParameteriv( target, pname, params );
}

/* ---- logTexSubImage1D  0x004BF570 ---- */
static void QGLAPI logTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels ) {
	fprintf( Stream, "glTexSubImage1D\n" );
	fflush( Stream );
	dllTexSubImage1D( target, level, xoffset, width, format, type, pixels );
}

/* ---- logTexSubImage2D  0x004BF5A0 ---- */
static void QGLAPI logTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ) {
	fprintf( Stream, "glTexSubImage2D\n" );
	fflush( Stream );
	dllTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}

/* ---- logTranslated  0x004BF5D0 ---- */
static void QGLAPI logTranslated( GLdouble x, GLdouble y, GLdouble z ) {
	fprintf( Stream, "glTranslated\n" );
	fflush( Stream );
	dllTranslated( x, y, z );
}

/* ---- logTranslatef  0x004BF620 ---- */
static void QGLAPI logTranslatef( GLfloat x, GLfloat y, GLfloat z ) {
	fprintf( Stream, "glTranslatef\n" );
	fflush( Stream );
	dllTranslatef( x, y, z );
}

/* ---- logVertex2d  0x004BF650 ---- */
static void QGLAPI logVertex2d( GLdouble x, GLdouble y ) {
	fprintf( Stream, "glVertex2d\n" );
	fflush( Stream );
	dllVertex2d( x, y );
}

/* ---- logVertex2dv  0x004BF690 ---- */
static void QGLAPI logVertex2dv( const GLdouble *v ) {
	fprintf( Stream, "glVertex2dv\n" );
	fflush( Stream );
	dllVertex2dv( v );
}

/* ---- logVertex2f  0x004BF6C0 ---- */
static void QGLAPI logVertex2f( GLfloat x, GLfloat y ) {
	fprintf( Stream, "glVertex2f\n" );
	fflush( Stream );
	dllVertex2f( x, y );
}

/* ---- logVertex2fv  0x004BF6F0 ---- */
static void QGLAPI logVertex2fv( const GLfloat *v ) {
	fprintf( Stream, "glVertex2fv\n" );
	fflush( Stream );
	dllVertex2fv( v );
}

/* ---- logVertex2i  0x004BF720 ---- */
static void QGLAPI logVertex2i( GLint x, GLint y ) {
	fprintf( Stream, "glVertex2i\n" );
	fflush( Stream );
	dllVertex2i( x, y );
}

/* ---- logVertex2iv  0x004BF750 ---- */
static void QGLAPI logVertex2iv( const GLint *v ) {
	fprintf( Stream, "glVertex2iv\n" );
	fflush( Stream );
	dllVertex2iv( v );
}

/* ---- logVertex2s  0x004BF780 ---- */
static void QGLAPI logVertex2s( GLshort x, GLshort y ) {
	fprintf( Stream, "glVertex2s\n" );
	fflush( Stream );
	dllVertex2s( x, y );
}

/* ---- logVertex2sv  0x004BF7B0 ---- */
static void QGLAPI logVertex2sv( const GLshort *v ) {
	fprintf( Stream, "glVertex2sv\n" );
	fflush( Stream );
	dllVertex2sv( v );
}

/* ---- logVertex3d  0x004BF7E0 ---- */
static void QGLAPI logVertex3d( GLdouble x, GLdouble y, GLdouble z ) {
	fprintf( Stream, "glVertex3d\n" );
	fflush( Stream );
	dllVertex3d( x, y, z );
}

/* ---- logVertex3dv  0x004BF830 ---- */
static void QGLAPI logVertex3dv( const GLdouble *v ) {
	fprintf( Stream, "glVertex3dv\n" );
	fflush( Stream );
	dllVertex3dv( v );
}

/* ---- logVertex3f  0x004BF860 ---- */
static void QGLAPI logVertex3f( GLfloat x, GLfloat y, GLfloat z ) {
	fprintf( Stream, "glVertex3f\n" );
	fflush( Stream );
	dllVertex3f( x, y, z );
}

/* ---- logVertex3fv  0x004BF890 ---- */
static void QGLAPI logVertex3fv( const GLfloat *v ) {
	fprintf( Stream, "glVertex3fv\n" );
	fflush( Stream );
	dllVertex3fv( v );
}

/* ---- logVertex3i  0x004BF8C0 ---- */
static void QGLAPI logVertex3i( GLint x, GLint y, GLint z ) {
	fprintf( Stream, "glVertex3i\n" );
	fflush( Stream );
	dllVertex3i( x, y, z );
}

/* ---- logVertex3iv  0x004BF8F0 ---- */
static void QGLAPI logVertex3iv( const GLint *v ) {
	fprintf( Stream, "glVertex3iv\n" );
	fflush( Stream );
	dllVertex3iv( v );
}

/* ---- logVertex3s  0x004BF920 ---- */
static void QGLAPI logVertex3s( GLshort x, GLshort y, GLshort z ) {
	fprintf( Stream, "glVertex3s\n" );
	fflush( Stream );
	dllVertex3s( x, y, z );
}

/* ---- logVertex3sv  0x004BF950 ---- */
static void QGLAPI logVertex3sv( const GLshort *v ) {
	fprintf( Stream, "glVertex3sv\n" );
	fflush( Stream );
	dllVertex3sv( v );
}

/* ---- logVertex4d  0x004BF980 ---- */
static void QGLAPI logVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	fprintf( Stream, "glVertex4d\n" );
	fflush( Stream );
	dllVertex4d( x, y, z, w );
}

/* ---- logVertex4dv  0x004BF9D0 ---- */
static void QGLAPI logVertex4dv( const GLdouble *v ) {
	fprintf( Stream, "glVertex4dv\n" );
	fflush( Stream );
	dllVertex4dv( v );
}

/* ---- logVertex4f  0x004BFA00 ---- */
static void QGLAPI logVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	fprintf( Stream, "glVertex4f\n" );
	fflush( Stream );
	dllVertex4f( x, y, z, w );
}

/* ---- logVertex4fv  0x004BFA30 ---- */
static void QGLAPI logVertex4fv( const GLfloat *v ) {
	fprintf( Stream, "glVertex4fv\n" );
	fflush( Stream );
	dllVertex4fv( v );
}

/* ---- logVertex4i  0x004BFA60 ---- */
static void QGLAPI logVertex4i( GLint x, GLint y, GLint z, GLint w ) {
	fprintf( Stream, "glVertex4i\n" );
	fflush( Stream );
	dllVertex4i( x, y, z, w );
}

/* ---- logVertex4iv  0x004BFA90 ---- */
static void QGLAPI logVertex4iv( const GLint *v ) {
	fprintf( Stream, "glVertex4iv\n" );
	fflush( Stream );
	dllVertex4iv( v );
}

/* ---- logVertex4s  0x004BFAC0 ---- */
static void QGLAPI logVertex4s( GLshort x, GLshort y, GLshort z, GLshort w ) {
	fprintf( Stream, "glVertex4s\n" );
	fflush( Stream );
	dllVertex4s( x, y, z, w );
}

/* ---- logVertex4sv  0x004BFAF0 ---- */
static void QGLAPI logVertex4sv( const GLshort *v ) {
	fprintf( Stream, "glVertex4sv\n" );
	fflush( Stream );
	dllVertex4sv( v );
}

/* ---- logVertexPointer  0x004BA3F0 ---- */
static void QGLAPI logVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) {
	const char *pointerName = PointerToString( pointer );

	fprintf( Stream, "glVertexPointer( %d, %s, %d, %s )\n", size, TypeToString( type ), stride, pointerName );
	fflush( Stream );
	dllVertexPointer( size, type, stride, pointer );
}

/* ---- logViewport  0x004BA450 ---- */
static void QGLAPI logViewport( GLint x, GLint y, GLsizei width, GLsizei height ) {
	fprintf( Stream, "glViewport( %d, %d, %d, %d )\n", x, y, width, height );
	fflush( Stream );
	dllViewport( x, y, width, height );
}

/* ---- logwglCopyContext  0x004BFB20 ---- */
static BOOL WINAPI logwglCopyContext( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask ) {
	fprintf( Stream, "wglCopyContext\n" );
	fflush( Stream );
	return dllwglCopyContext( hglrcSrc, hglrcDst, mask );
}

/* ---- logwglCreateContext  0x004BFB50 ---- */
static HGLRC WINAPI logwglCreateContext( HDC hdc ) {
	fprintf( Stream, "wglCreateContext\n" );
	fflush( Stream );
	return dllwglCreateContext( hdc );
}

/* ---- logwglCreateLayerContext  0x004BFB80 ---- */
static HGLRC WINAPI logwglCreateLayerContext( HDC hdc, int iLayerPlane ) {
	fprintf( Stream, "wglCreateLayerContext\n" );
	fflush( Stream );
	return dllwglCreateLayerContext( hdc, iLayerPlane );
}

/* ---- logwglDeleteContext  0x004BFBB0 ---- */
static BOOL WINAPI logwglDeleteContext( HGLRC hglrc ) {
	fprintf( Stream, "wglDeleteContext\n" );
	fflush( Stream );
	return dllwglDeleteContext( hglrc );
}

/* ---- logwglGetCurrentContext  0x004BFBE0 ---- */
static HGLRC WINAPI logwglGetCurrentContext( VOID ) {
	fprintf( Stream, "wglGetCurrentContext\n" );
	fflush( Stream );
	return dllwglGetCurrentContext();
}

/* ---- logwglGetCurrentDC  0x004BFC10 ---- */
static HDC WINAPI logwglGetCurrentDC( VOID ) {
	fprintf( Stream, "wglGetCurrentDC\n" );
	fflush( Stream );
	return dllwglGetCurrentDC();
}

/* ---- logwglGetProcAddress  0x004BFC40 ---- */
static PROC WINAPI logwglGetProcAddress( LPCSTR lpszProc ) {
	fprintf( Stream, "wglGetProcAddress\n" );
	fflush( Stream );
	return dllwglGetProcAddress( lpszProc );
}

/* ---- logwglMakeCurrent  0x004BFC70 ---- */
static BOOL WINAPI logwglMakeCurrent( HDC hdc, HGLRC hglrc ) {
	fprintf( Stream, "wglMakeCurrent\n" );
	fflush( Stream );
	return dllwglMakeCurrent( hdc, hglrc );
}

/* ---- logwglShareLists  0x004BFCA0 ---- */
static BOOL WINAPI logwglShareLists( HGLRC hglrc1, HGLRC hglrc2 ) {
	fprintf( Stream, "wglShareLists\n" );
	fflush( Stream );
	return dllwglShareLists( hglrc1, hglrc2 );
}

/* ---- logwglUseFontBitmaps  0x004BFCD0 ---- */
static BOOL WINAPI logwglUseFontBitmaps( HDC hdc, DWORD first, DWORD count, DWORD listBase ) {
	fprintf( Stream, "wglUseFontBitmaps\n" );
	fflush( Stream );
	return dllwglUseFontBitmaps( hdc, first, count, listBase );
}

/* ---- logwglUseFontOutlines  0x004BFD00 ---- */
static BOOL WINAPI logwglUseFontOutlines( HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf ) {
	fprintf( Stream, "wglUseFontOutlines\n" );
	fflush( Stream );
	return dllwglUseFontOutlines( hdc, first, count, listBase, deviation, extrusion, format, lpgmf );
}

/* ---- logwglDescribeLayerPlane  0x004BFD30 ---- */
static BOOL WINAPI logwglDescribeLayerPlane( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR plpd ) {
	fprintf( Stream, "wglDescribeLayerPlane\n" );
	fflush( Stream );
	return dllwglDescribeLayerPlane( hdc, iPixelFormat, iLayerPlane, nBytes, plpd );
}

/* ---- logwglSetLayerPaletteEntries  0x004BFD60 ---- */
static int WINAPI logwglSetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart, int cEntries, CONST COLORREF *pcr ) {
	fprintf( Stream, "wglSetLayerPaletteEntries\n" );
	fflush( Stream );
	return dllwglSetLayerPaletteEntries( hdc, iLayerPlane, iStart, cEntries, pcr );
}

/* ---- logwglGetLayerPaletteEntries  0x004BFD90 ---- */
static int WINAPI logwglGetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF *pcr ) {
	fprintf( Stream, "wglGetLayerPaletteEntries\n" );
	fflush( Stream );
	return dllwglGetLayerPaletteEntries( hdc, iLayerPlane, iStart, cEntries, pcr );
}

/* ---- logwglRealizeLayerPalette  0x004BFDC0 ---- */
static BOOL WINAPI logwglRealizeLayerPalette( HDC hdc, int iLayerPlane, BOOL bRealize ) {
	fprintf( Stream, "wglRealizeLayerPalette\n" );
	fflush( Stream );
	return dllwglRealizeLayerPalette( hdc, iLayerPlane, bRealize );
}

/* ---- logwglSwapLayerBuffers  0x004BFDF0 ---- */
static BOOL WINAPI logwglSwapLayerBuffers( HDC hdc, UINT fuPlanes ) {
	fprintf( Stream, "wglSwapLayerBuffers\n" );
	fflush( Stream );
	return dllwglSwapLayerBuffers( hdc, fuPlanes );
}

/* ---- logActiveTextureARB  0x004BA510 ---- */
static void QGLAPI logActiveTextureARB( GLenum texture ) {
	fprintf( Stream, "glActiveTextureARB( GL_TEXTURE%i_ARB )\n", texture - GL_TEXTURE0_ARB );
	fflush( Stream );
	dllActiveTextureARB( texture );
}

/* ---- logAlphaFragmentOp1ATI  0x004BB220 ---- */
static void QGLAPI logAlphaFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
	fprintf( Stream, "glAlphaFragmentOp1ATI\n" );
	fflush( Stream );
	dllAlphaFragmentOp1ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod );
}

/* ---- logAlphaFragmentOp2ATI  0x004BB250 ---- */
static void QGLAPI logAlphaFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
	fprintf( Stream, "glAlphaFragmentOp2ATI\n" );
	fflush( Stream );
	dllAlphaFragmentOp2ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
}

/* ---- logAlphaFragmentOp3ATI  0x004BB280 ---- */
static void QGLAPI logAlphaFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
	fprintf( Stream, "glAlphaFragmentOp3ATI\n" );
	fflush( Stream );
	dllAlphaFragmentOp3ATI( op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
}

/* ---- logArrayObjectATI  0x004BAA40 ---- */
static void QGLAPI logArrayObjectATI( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
	fprintf( Stream, "glArrayObjectATI\n" );
	fflush( Stream );
	dllArrayObjectATI( array, size, type, stride, buffer, offset );
}

/* ---- logBeginFragmentShaderATI  0x004BB0D0 ---- */
static void QGLAPI logBeginFragmentShaderATI( void ) {
	fprintf( Stream, "glBeginFragmentShaderATI\n" );
	fflush( Stream );
	dllBeginFragmentShaderATI();
}

/* ---- logBindBufferARB  0x004B9670 ---- */
static void QGLAPI logBindBufferARB( GLenum target, GLuint buffer ) {
	fprintf( Stream, "glBindBufferARB( %s, %u )\n", CapToString( target ), buffer );
	fflush( Stream );
	dllBindBufferARB( target, buffer );
}

/* ---- logBindFragmentShaderATI  0x004B9640 ---- */
static void QGLAPI logBindFragmentShaderATI( GLuint id ) {
	fprintf( Stream, "glBindFragmentShaderATI( %u )\n", id );
	fflush( Stream );
	dllBindFragmentShaderATI( id );
}

/* ---- logBindProgramARB  0x004B9600 ---- */
static void QGLAPI logBindProgramARB( GLenum target, GLuint program ) {
	fprintf( Stream, "glBindProgramARB( %s, %u )\n", CapToString( target ), program );
	fflush( Stream );
	dllBindProgramARB( target, program );
}

/* ---- logBufferDataARB  0x004B96B0 ---- */
static void QGLAPI logBufferDataARB( GLenum target, GLsizei size, const GLvoid *data, GLenum usage ) {
	/* args evaluate right to left: usage is converted first (unknown enums share CapToString's buffer) */
	fprintf( Stream, "glBufferDataARB( %s, %u, %p, %s )\n", CapToString( target ), size, data, CapToString( usage ) );
	fflush( Stream );
	dllBufferDataARB( target, size, data, usage );
}

/* ---- logBufferSubDataARB  0x004BA860 ---- */
static void QGLAPI logBufferSubDataARB( GLenum target, GLint offset, GLsizei size, const GLvoid *data ) {
	fprintf( Stream, "glBufferSubDataARB\n" );
	fflush( Stream );
	dllBufferSubDataARB( target, offset, size, data );
}

/* ---- logClientActiveTextureARB  0x004BA550 ---- */
static void QGLAPI logClientActiveTextureARB( GLenum texture ) {
	fprintf( Stream, "glClientActiveTextureARB( GL_TEXTURE%i_ARB )\n", texture - GL_TEXTURE0_ARB );
	fflush( Stream );
	dllClientActiveTextureARB( texture );
}

/* ---- logColorFragmentOp1ATI  0x004BB190 ---- */
static void QGLAPI logColorFragmentOp1ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ) {
	fprintf( Stream, "glColorFragmentOp1ATI\n" );
	fflush( Stream );
	dllColorFragmentOp1ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod );
}

/* ---- logColorFragmentOp2ATI  0x004BB1C0 ---- */
static void QGLAPI logColorFragmentOp2ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ) {
	fprintf( Stream, "glColorFragmentOp2ATI\n" );
	fflush( Stream );
	dllColorFragmentOp2ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod );
}

/* ---- logColorFragmentOp3ATI  0x004BB1F0 ---- */
static void QGLAPI logColorFragmentOp3ATI( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ) {
	fprintf( Stream, "glColorFragmentOp3ATI\n" );
	fflush( Stream );
	dllColorFragmentOp3ATI( op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod );
}

/* ---- logCombinerInputNV  0x004BAE60 ---- */
static void QGLAPI logCombinerInputNV( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
	fprintf( Stream, "glCombinerInputNV\n" );
	fflush( Stream );
	dllCombinerInputNV( stage, portion, variable, input, mapping, componentUsage );
}

/* ---- logCombinerOutputNV  0x004BAE90 ---- */
static void QGLAPI logCombinerOutputNV( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) {
	fprintf( Stream, "glCombinerOutputNV\n" );
	fflush( Stream );
	dllCombinerOutputNV( stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum );
}

/* ---- logCombinerParameterfNV  0x004BADD0 ---- */
static void QGLAPI logCombinerParameterfNV( GLenum pname, GLfloat param ) {
	fprintf( Stream, "glCombinerParameterfNV\n" );
	fflush( Stream );
	dllCombinerParameterfNV( pname, param );
}

/* ---- logCombinerParameterfvNV  0x004BADA0 ---- */
static void QGLAPI logCombinerParameterfvNV( GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glCombinerParameterfvNV\n" );
	fflush( Stream );
	dllCombinerParameterfvNV( pname, params );
}

/* ---- logCombinerParameteriNV  0x004BAE30 ---- */
static void QGLAPI logCombinerParameteriNV( GLenum pname, GLint param ) {
	fprintf( Stream, "glCombinerParameteriNV\n" );
	fflush( Stream );
	dllCombinerParameteriNV( pname, param );
}

/* ---- logCombinerParameterivNV  0x004BAE00 ---- */
static void QGLAPI logCombinerParameterivNV( GLenum pname, const GLint *params ) {
	fprintf( Stream, "glCombinerParameterivNV\n" );
	fflush( Stream );
	dllCombinerParameterivNV( pname, params );
}

/* ---- logCombinerStageParameterfvNV  0x004BB010 ---- */
static void QGLAPI logCombinerStageParameterfvNV( GLenum stage, GLenum pname, const GLfloat *params ) {
	fprintf( Stream, "glCombinerStageParameterfvNV\n" );
	fflush( Stream );
	dllCombinerStageParameterfvNV( stage, pname, params );
}

/* ---- logCompressedTexImage1DARB  0x004BA6E0 ---- */
static void QGLAPI logCompressedTexImage1DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data ) {
	fprintf( Stream, "glCompressedTexImage1DARB\n" );
	fflush( Stream );
	dllCompressedTexImage1DARB( target, level, internalformat, width, border, imageSize, data );
}

/* ---- logCompressedTexImage2DARB  0x004BA6B0 ---- */
static void QGLAPI logCompressedTexImage2DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data ) {
	fprintf( Stream, "glCompressedTexImage2DARB\n" );
	fflush( Stream );
	dllCompressedTexImage2DARB( target, level, internalformat, width, height, border, imageSize, data );
}

/* ---- logCompressedTexImage3DARB  0x004BA680 ---- */
static void QGLAPI logCompressedTexImage3DARB( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data ) {
	fprintf( Stream, "glCompressedTexImage3DARB\n" );
	fflush( Stream );
	dllCompressedTexImage3DARB( target, level, internalformat, width, height, depth, border, imageSize, data );
}

/* ---- logCompressedTexSubImage1D  0x004BA770 ---- */
static void QGLAPI logCompressedTexSubImage1D( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data ) {
	fprintf( Stream, "glCompressedTexSubImage1DARB\n" );
	fflush( Stream );
	dllCompressedTexSubImage1D( target, level, xoffset, width, format, imageSize, data );
}

/* ---- logCompressedTexSubImage2DARB  0x004BA740 ---- */
static void QGLAPI logCompressedTexSubImage2DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data ) {
	fprintf( Stream, "glCompressedTexSubImage2DARB\n" );
	fflush( Stream );
	dllCompressedTexSubImage2DARB( target, level, xoffset, yoffset, width, height, format, imageSize, data );
}

/* ---- logCompressedTexSubImage3DARB  0x004BA710 ---- */
static void QGLAPI logCompressedTexSubImage3DARB( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data ) {
	fprintf( Stream, "glCompressedTexSubImage3DARB\n" );
	fflush( Stream );
	dllCompressedTexSubImage3DARB( target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data );
}

/* ---- logDeleteBuffersARB  0x004BA7D0 ---- */
static void QGLAPI logDeleteBuffersARB( GLsizei n, const GLuint *buffers ) {
	fprintf( Stream, "glDeleteBuffersARB\n" );
	fflush( Stream );
	dllDeleteBuffersARB( n, buffers );
}

/* ---- logDeleteFencesNV  0x004BACB0 ---- */
static void QGLAPI logDeleteFencesNV( GLsizei n, const GLuint *fences ) {
	fprintf( Stream, "glDeleteFencesNV\n" );
	fflush( Stream );
	dllDeleteFencesNV( n, fences );
}

/* ---- logDeleteFragmentShaderATI  0x004BB0A0 ---- */
static void QGLAPI logDeleteFragmentShaderATI( GLuint id ) {
	fprintf( Stream, "glDeleteFragmentShaderATI\n" );
	fflush( Stream );
	dllDeleteFragmentShaderATI( id );
}

/* ---- logDeleteProgramsARB  0x004BBAE0 ---- */
static void QGLAPI logDeleteProgramsARB( GLsizei n, const GLuint *programs ) {
	fprintf( Stream, "glDeleteProgramsARB\n" );
	fflush( Stream );
	dllDeleteProgramsARB( n, programs );
}

/* ---- logDisableVertexAttribArrayARB  0x004BBA80 ---- */
static void QGLAPI logDisableVertexAttribArrayARB( GLuint index ) {
	fprintf( Stream, "glDisableVertexAttribArrayARB\n" );
	fflush( Stream );
	dllDisableVertexAttribArrayARB( index );
}

/* ---- logDrawElementArrayATI  0x004BAB90 ---- */
static void QGLAPI logDrawElementArrayATI( GLenum mode, GLsizei count ) {
	fprintf( Stream, "glDrawElementArrayATI\n" );
	fflush( Stream );
	dllDrawElementArrayATI( mode, count );
}

/* ---- logDrawRangeElementArrayATI  0x004BABC0 ---- */
static void QGLAPI logDrawRangeElementArrayATI( GLenum mode, GLuint start, GLuint end, GLsizei count ) {
	fprintf( Stream, "glDrawRangeElementArrayATI\n" );
	fflush( Stream );
	dllDrawRangeElementArrayATI( mode, start, end, count );
}

/* ---- logDrawRangeElementsEXT  0x004B9DC0 ---- */
static void QGLAPI logDrawRangeElementsEXT( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices ) {
	const char *indicesName = PointerToString( indices );
	const char *typeName    = TypeToString( type );

	fprintf( Stream, "glDrawRangeElementsEXT( %s, %u, %u, %d, %s, %s )\n",
				 PrimToString( mode ), start, end, count, typeName, indicesName );
	fflush( Stream );
	dllDrawRangeElementsEXT( mode, start, end, count, type, indices );
}

/* ---- logElementPointerATI  0x004BAB60 ---- */
static void QGLAPI logElementPointerATI( GLenum type, const GLvoid *pointer ) {
	fprintf( Stream, "glElementPointerATI\n" );
	fflush( Stream );
	dllElementPointerATI( type, pointer );
}

/* ---- logEnableVertexAttribArrayARB  0x004BBA50 ---- */
static void QGLAPI logEnableVertexAttribArrayARB( GLuint index ) {
	fprintf( Stream, "glEnableVertexAttribArrayARB\n" );
	fflush( Stream );
	dllEnableVertexAttribArrayARB( index );
}

/* ---- logEndFragmentShaderATI  0x004BB100 ---- */
static void QGLAPI logEndFragmentShaderATI( void ) {
	fprintf( Stream, "glEndFragmentShaderATI\n" );
	fflush( Stream );
	dllEndFragmentShaderATI();
}

/* ---- logFinalCombinerInputNV  0x004BAEC0 ---- */
static void QGLAPI logFinalCombinerInputNV( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) {
	fprintf( Stream, "glFinalCombinerInputNV\n" );
	fflush( Stream );
	dllFinalCombinerInputNV( variable, input, mapping, componentUsage );
}

/* ---- logFinishFenceNV  0x004BA4E0 ---- */
static void QGLAPI logFinishFenceNV( GLuint fence ) {
	fprintf( Stream, "glFinishFenceNV( %d )\n", fence );
	fflush( Stream );
	dllFinishFenceNV( fence );
}

/* ---- logFlushVertexArrayRangeNV  0x004BABF0 ---- */
static void QGLAPI logFlushVertexArrayRangeNV( void ) {
	fprintf( Stream, "glFlushVertexArrayRangeNV\n" );
	fflush( Stream );
	dllFlushVertexArrayRangeNV();
}

/* ---- logFreeObjectBufferATI  0x004BAA10 ---- */
static void QGLAPI logFreeObjectBufferATI( GLuint buffer ) {
	fprintf( Stream, "glFreeObjectBufferATI\n" );
	fflush( Stream );
	dllFreeObjectBufferATI( buffer );
}

/* ---- logGenBuffersARB  0x004BA800 ---- */
static void QGLAPI logGenBuffersARB( GLsizei n, GLuint *buffers ) {
	fprintf( Stream, "glGenBuffersARB\n" );
	fflush( Stream );
	dllGenBuffersARB( n, buffers );
}

/* ---- logGenFencesNV  0x004BACE0 ---- */
static void QGLAPI logGenFencesNV( GLsizei n, GLuint *fences ) {
	fprintf( Stream, "glGenFencesNV\n" );
	fflush( Stream );
	dllGenFencesNV( n, fences );
}

/* ---- logGenFragmentShadersATI  0x004BB070 ---- */
static GLuint QGLAPI logGenFragmentShadersATI( GLuint range ) {
	fprintf( Stream, "glGenFragmentShadersATI\n" );
	fflush( Stream );
	return dllGenFragmentShadersATI( range );
}

/* ---- logGenProgramsARB  0x004BBB10 ---- */
static void QGLAPI logGenProgramsARB( GLsizei n, GLuint *programs ) {
	fprintf( Stream, "glGenProgramsARB\n" );
	fflush( Stream );
	dllGenProgramsARB( n, programs );
}

/* ---- logGetArrayObjectfvATI  0x004BAA70 ---- */
static void QGLAPI logGetArrayObjectfvATI( GLenum array, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetArrayObjectfvATI\n" );
	fflush( Stream );
	dllGetArrayObjectfvATI( array, pname, params );
}

/* ---- logGetArrayObjectivATI  0x004BAAA0 ---- */
static void QGLAPI logGetArrayObjectivATI( GLenum array, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetArrayObjectivATI\n" );
	fflush( Stream );
	dllGetArrayObjectivATI( array, pname, params );
}

/* ---- logGetBufferParameterivARB  0x004BA8C0 ---- */
static void QGLAPI logGetBufferParameterivARB( GLenum target, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetBufferParameterivARB\n" );
	fflush( Stream );
	dllGetBufferParameterivARB( target, pname, params );
}

/* ---- logGetBufferPointervARB  0x004BA8F0 ---- */
static void QGLAPI logGetBufferPointervARB( GLenum target, GLenum pname, GLvoid **params ) {
	fprintf( Stream, "glGetBufferPointervARB\n" );
	fflush( Stream );
	dllGetBufferPointervARB( target, pname, params );
}

/* ---- logGetBufferSubDataARB  0x004BA890 ---- */
static void QGLAPI logGetBufferSubDataARB( GLenum target, GLint offset, GLsizei size, GLvoid *data ) {
	fprintf( Stream, "glGetBufferSubDataARB\n" );
	fflush( Stream );
	dllGetBufferSubDataARB( target, offset, size, data );
}

/* ---- logGetCombinerInputParameterfvNV  0x004BAEF0 ---- */
static void QGLAPI logGetCombinerInputParameterfvNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetCombinerInputParameterfvNV\n" );
	fflush( Stream );
	dllGetCombinerInputParameterfvNV( stage, portion, variable, pname, params );
}

/* ---- logGetCombinerInputParameterivNV  0x004BAF20 ---- */
static void QGLAPI logGetCombinerInputParameterivNV( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetCombinerInputParameterivNV\n" );
	fflush( Stream );
	dllGetCombinerInputParameterivNV( stage, portion, variable, pname, params );
}

/* ---- logGetCombinerOutputParameterfvNV  0x004BAF50 ---- */
static void QGLAPI logGetCombinerOutputParameterfvNV( GLenum stage, GLenum portion, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetCombinerOutputParameterfvNV\n" );
	fflush( Stream );
	dllGetCombinerOutputParameterfvNV( stage, portion, pname, params );
}

/* ---- logGetCombinerOutputParameterivNV  0x004BAF80 ---- */
static void QGLAPI logGetCombinerOutputParameterivNV( GLenum stage, GLenum portion, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetCombinerOutputParameterivNV\n" );
	fflush( Stream );
	dllGetCombinerOutputParameterivNV( stage, portion, pname, params );
}

/* ---- logGetCombinerStageParameterfvNV  0x004BB040 ---- */
static void QGLAPI logGetCombinerStageParameterfvNV( GLenum stage, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetCombinerStageParameterfvNV\n" );
	fflush( Stream );
	dllGetCombinerStageParameterfvNV( stage, pname, params );
}

/* ---- logGetCompressedTexImage  0x004BA7A0 ---- */
static void QGLAPI logGetCompressedTexImage( GLenum target, GLint level, GLvoid *img ) {
	fprintf( Stream, "glGetCompressedTexImageARB\n" );
	fflush( Stream );
	dllGetCompressedTexImage( target, level, img );
}

/* ---- logGetFenceivNV  0x004BAD70 ---- */
static void QGLAPI logGetFenceivNV( GLuint fence, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetFenceivNV\n" );
	fflush( Stream );
	dllGetFenceivNV( fence, pname, params );
}

/* ---- logGetFinalCombinerInputParameterfvNV  0x004BAFB0 ---- */
static void QGLAPI logGetFinalCombinerInputParameterfvNV( GLenum variable, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetFinalCombinerInputParameterfvNV\n" );
	fflush( Stream );
	dllGetFinalCombinerInputParameterfvNV( variable, pname, params );
}

/* ---- logGetFinalCombinerInputParameterivNV  0x004BAFE0 ---- */
static void QGLAPI logGetFinalCombinerInputParameterivNV( GLenum variable, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetFinalCombinerInputParameterivNV\n" );
	fflush( Stream );
	dllGetFinalCombinerInputParameterivNV( variable, pname, params );
}

/* ---- logGetObjectBufferfvATI  0x004BA9B0 ---- */
static void QGLAPI logGetObjectBufferfvATI( GLuint buffer, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetObjectBufferfvATI\n" );
	fflush( Stream );
	dllGetObjectBufferfvATI( buffer, pname, params );
}

/* ---- logGetObjectBufferivATI  0x004BA9E0 ---- */
static void QGLAPI logGetObjectBufferivATI( GLuint buffer, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetObjectBufferivATI\n" );
	fflush( Stream );
	dllGetObjectBufferivATI( buffer, pname, params );
}

/* ---- logGetProgramEnvParameterdvARB  0x004BBD50 ---- */
static void QGLAPI logGetProgramEnvParameterdvARB( GLenum target, GLuint index, GLdouble *params ) {
	fprintf( Stream, "glGetProgramEnvParameterdvARB\n" );
	fflush( Stream );
	dllGetProgramEnvParameterdvARB( target, index, params );
}

/* ---- logGetProgramEnvParameterfvARB  0x004BBD20 ---- */
static void QGLAPI logGetProgramEnvParameterfvARB( GLenum target, GLuint index, GLfloat *params ) {
	fprintf( Stream, "glGetProgramEnvParameterfvARB\n" );
	fflush( Stream );
	dllGetProgramEnvParameterfvARB( target, index, params );
}

/* ---- logGetProgramLocalParameterdvARB  0x004BBDB0 ---- */
static void QGLAPI logGetProgramLocalParameterdvARB( GLenum target, GLuint index, GLdouble *params ) {
	fprintf( Stream, "glGetProgramLocalParameterdvARB\n" );
	fflush( Stream );
	dllGetProgramLocalParameterdvARB( target, index, params );
}

/* ---- logGetProgramLocalParameterfvARB  0x004BBD80 ---- */
static void QGLAPI logGetProgramLocalParameterfvARB( GLenum target, GLuint index, GLfloat *params ) {
	fprintf( Stream, "glGetProgramLocalParameterfvARB\n" );
	fflush( Stream );
	dllGetProgramLocalParameterfvARB( target, index, params );
}

/* ---- logGetProgramStringARB  0x004BBE10 ---- */
static void QGLAPI logGetProgramStringARB( GLenum target, GLenum pname, GLvoid *string ) {
	fprintf( Stream, "glGetProgramStringARB\n" );
	fflush( Stream );
	dllGetProgramStringARB( target, pname, string );
}

/* ---- logGetProgramivARB  0x004BBDE0 ---- */
static void QGLAPI logGetProgramivARB( GLenum target, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetProgramivARB\n" );
	fflush( Stream );
	dllGetProgramivARB( target, pname, params );
}

/* ---- logGetVariantArrayObjectfvATI  0x004BAB00 ---- */
static void QGLAPI logGetVariantArrayObjectfvATI( GLuint id, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetVariantArrayObjectfvATI\n" );
	fflush( Stream );
	dllGetVariantArrayObjectfvATI( id, pname, params );
}

/* ---- logGetVariantArrayObjectivATI  0x004BAB30 ---- */
static void QGLAPI logGetVariantArrayObjectivATI( GLuint id, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetVariantArrayObjectivATI\n" );
	fflush( Stream );
	dllGetVariantArrayObjectivATI( id, pname, params );
}

/* ---- logGetVertexAttribPointervARB  0x004BBED0 ---- */
static void QGLAPI logGetVertexAttribPointervARB( GLuint index, GLenum pname, GLvoid **pointer ) {
	fprintf( Stream, "glGetVertexAttribPointervARB\n" );
	fflush( Stream );
	dllGetVertexAttribPointervARB( index, pname, pointer );
}

/* ---- logGetVertexAttribdvARB  0x004BBE40 ---- */
static void QGLAPI logGetVertexAttribdvARB( GLuint index, GLenum pname, GLdouble *params ) {
	fprintf( Stream, "glGetVertexAttribdvARB\n" );
	fflush( Stream );
	dllGetVertexAttribdvARB( index, pname, params );
}

/* ---- logGetVertexAttribfvARB  0x004BBE70 ---- */
static void QGLAPI logGetVertexAttribfvARB( GLuint index, GLenum pname, GLfloat *params ) {
	fprintf( Stream, "glGetVertexAttribfvARB\n" );
	fflush( Stream );
	dllGetVertexAttribfvARB( index, pname, params );
}

/* ---- logGetVertexAttribivARB  0x004BBEA0 ---- */
static void QGLAPI logGetVertexAttribivARB( GLuint index, GLenum pname, GLint *params ) {
	fprintf( Stream, "glGetVertexAttribivARB\n" );
	fflush( Stream );
	dllGetVertexAttribivARB( index, pname, params );
}

/* ---- logIsBufferARB  0x004BA830 ---- */
static GLboolean QGLAPI logIsBufferARB( GLuint buffer ) {
	fprintf( Stream, "glIsBufferARB\n" );
	fflush( Stream );
	return dllIsBufferARB( buffer );
}

/* ---- logIsFenceNV  0x004BAD10 ---- */
static GLboolean QGLAPI logIsFenceNV( GLuint fence ) {
	fprintf( Stream, "glIsFenceNV\n" );
	fflush( Stream );
	return dllIsFenceNV( fence );
}

/* ---- logIsObjectBufferATI  0x004BA950 ---- */
static GLboolean QGLAPI logIsObjectBufferATI( GLuint buffer ) {
	fprintf( Stream, "glIsObjectBufferATI\n" );
	fflush( Stream );
	return dllIsObjectBufferATI( buffer );
}

/* ---- logIsProgramARB  0x004BBF00 ---- */
static GLboolean QGLAPI logIsProgramARB( GLuint program ) {
	fprintf( Stream, "glIsProgramARB\n" );
	fflush( Stream );
	return dllIsProgramARB( program );
}

/* ---- logLockArraysEXT  0x004BA5C0 ---- */
static void QGLAPI logLockArraysEXT( GLint first, GLsizei count ) {
	fprintf( Stream, "glLockArraysEXT\n" );
	fflush( Stream );
	dllLockArraysEXT( first, count );
}

/* ---- logMapBufferARB  0x004B9710 ---- */
static GLvoid * QGLAPI logMapBufferARB( GLenum target, GLenum access ) {
	/* args evaluate right to left: access is converted first (unknown enums share CapToString's buffer) */
	fprintf( Stream, "glMapBufferARB( %s, %s )\n", CapToString( target ), CapToString( access ) );
	fflush( Stream );
	return dllMapBufferARB( target, access );
}

/* ---- logNewObjectBufferATI  0x004BA920 ---- */
static GLuint QGLAPI logNewObjectBufferATI( GLsizei size, const GLvoid *pointer, GLenum usage ) {
	fprintf( Stream, "glNewObjectBufferATI\n" );
	fflush( Stream );
	return dllNewObjectBufferATI( size, pointer, usage );
}

/* ---- logPNTrianglesfATI  0x004BA650 ---- */
static void QGLAPI logPNTrianglesfATI( GLenum pname, GLfloat param ) {
	fprintf( Stream, "glPNTrianglesfATI\n" );
	fflush( Stream );
	dllPNTrianglesfATI( pname, param );
}

/* ---- logPNTrianglesiATI  0x004BA620 ---- */
static void QGLAPI logPNTrianglesiATI( GLenum pname, GLint param ) {
	fprintf( Stream, "glPNTrianglesiATI\n" );
	fflush( Stream );
	dllPNTrianglesiATI( pname, param );
}

/* ---- logPassTexCoordATI  0x004BB130 ---- */
static void QGLAPI logPassTexCoordATI( GLuint dst, GLuint coord, GLenum swizzle ) {
	fprintf( Stream, "glPassTexCoordATI\n" );
	fflush( Stream );
	dllPassTexCoordATI( dst, coord, swizzle );
}

/* ---- logProgramEnvParameter4dARB  0x004BBB70 ---- */
static void QGLAPI logProgramEnvParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	fprintf( Stream, "glProgramEnvParameter4dARB\n" );
	fflush( Stream );
	dllProgramEnvParameter4dARB( target, index, x, y, z, w );
}

/* ---- logProgramEnvParameter4dvARB  0x004BBC00 ---- */
static void QGLAPI logProgramEnvParameter4dvARB( GLenum target, GLuint index, const GLdouble *params ) {
	fprintf( Stream, "glProgramEnvParameter4dvARB\n" );
	fflush( Stream );
	dllProgramEnvParameter4dvARB( target, index, params );
}

/* ---- logProgramEnvParameter4fARB  0x004BBB40 ---- */
static void QGLAPI logProgramEnvParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	fprintf( Stream, "glProgramEnvParameter4fARB\n" );
	fflush( Stream );
	dllProgramEnvParameter4fARB( target, index, x, y, z, w );
}

/* ---- logProgramEnvParameter4fvARB  0x004BBBD0 ---- */
static void QGLAPI logProgramEnvParameter4fvARB( GLenum target, GLuint index, const GLfloat *params ) {
	fprintf( Stream, "glProgramEnvParameter4fvARB\n" );
	fflush( Stream );
	dllProgramEnvParameter4fvARB( target, index, params );
}

/* ---- logProgramLocalParameter4dARB  0x004BBC60 ---- */
static void QGLAPI logProgramLocalParameter4dARB( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	fprintf( Stream, "glProgramLocalParameter4dARB\n" );
	fflush( Stream );
	dllProgramLocalParameter4dARB( target, index, x, y, z, w );
}

/* ---- logProgramLocalParameter4dvARB  0x004BBCF0 ---- */
static void QGLAPI logProgramLocalParameter4dvARB( GLenum target, GLuint index, const GLdouble *params ) {
	fprintf( Stream, "glProgramLocalParameter4dvARB\n" );
	fflush( Stream );
	dllProgramLocalParameter4dvARB( target, index, params );
}

/* ---- logProgramLocalParameter4fARB  0x004BBC30 ---- */
static void QGLAPI logProgramLocalParameter4fARB( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	fprintf( Stream, "glProgramLocalParameter4fARB\n" );
	fflush( Stream );
	dllProgramLocalParameter4fARB( target, index, x, y, z, w );
}

/* ---- logProgramLocalParameter4fvARB  0x004BBCC0 ---- */
static void QGLAPI logProgramLocalParameter4fvARB( GLenum target, GLuint index, const GLfloat *params ) {
	fprintf( Stream, "glProgramLocalParameter4fvARB\n" );
	fflush( Stream );
	dllProgramLocalParameter4fvARB( target, index, params );
}

/* ---- logProgramStringARB  0x004BBAB0 ---- */
static void QGLAPI logProgramStringARB( GLenum target, GLenum format, GLsizei len, const GLvoid *string ) {
	fprintf( Stream, "glProgramStringARB\n" );
	fflush( Stream );
	dllProgramStringARB( target, format, len, string );
}

/* ---- logSampleMapATI  0x004BB160 ---- */
static void QGLAPI logSampleMapATI( GLuint dst, GLuint interp, GLenum swizzle ) {
	fprintf( Stream, "glSampleMapATI\n" );
	fflush( Stream );
	dllSampleMapATI( dst, interp, swizzle );
}

/* ---- logSetFenceNV  0x004BA4A0 ---- */
static void QGLAPI logSetFenceNV( GLuint fence, GLenum condition ) {
	/* the condition is forwarded but not printed */
	fprintf( Stream, "glSetFenceNV( %d )\n", fence );
	fflush( Stream );
	dllSetFenceNV( fence, condition );
}

/* ---- logSetFragmentShaderConstantATI  0x004BB2B0 ---- */
static void QGLAPI logSetFragmentShaderConstantATI( GLuint dst, const GLfloat *value ) {
	fprintf( Stream, "glSetFragmentShaderConstantATI\n" );
	fflush( Stream );
	dllSetFragmentShaderConstantATI( dst, value );
}

/* ---- logTestFenceNV  0x004BAD40 ---- */
static GLboolean QGLAPI logTestFenceNV( GLuint fence ) {
	fprintf( Stream, "glTestFenceNV\n" );
	fflush( Stream );
	return dllTestFenceNV( fence );
}

/* ---- logUnlockArraysEXT  0x004BA5F0 ---- */
static void QGLAPI logUnlockArraysEXT( void ) {
	fprintf( Stream, "glUnlockArraysEXT\n" );
	fflush( Stream );
	dllUnlockArraysEXT();
}

/* ---- logUnmapBufferARB  0x004B9760 ---- */
static GLboolean QGLAPI logUnmapBufferARB( GLenum target ) {
	fprintf( Stream, "glUnmapBufferARB( %s )\n", CapToString( target ) );
	fflush( Stream );
	return dllUnmapBufferARB( target );
}

/* ---- logUpdateObjectBufferATI  0x004BA980 ---- */
static void QGLAPI logUpdateObjectBufferATI( GLuint buffer, GLuint offset, GLsizei size, const GLvoid *pointer, GLenum preserve ) {
	fprintf( Stream, "glUpdateObjectBufferATI\n" );
	fflush( Stream );
	dllUpdateObjectBufferATI( buffer, offset, size, pointer, preserve );
}

/* ---- logVariantArrayObjectATI  0x004BAAD0 ---- */
static void QGLAPI logVariantArrayObjectATI( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ) {
	fprintf( Stream, "glVariantArrayObjectATI\n" );
	fflush( Stream );
	dllVariantArrayObjectATI( id, type, stride, buffer, offset );
}

/* ---- logVertexArrayRangeNV  0x004BAC20 ---- */
static void QGLAPI logVertexArrayRangeNV( GLsizei length, const GLvoid *pointer ) {
	fprintf( Stream, "glVertexArrayRangeNV\n" );
	fflush( Stream );
	dllVertexArrayRangeNV( length, pointer );
}

/* ---- logVertexAttrib1dARB  0x004BB340 ---- */
static void QGLAPI logVertexAttrib1dARB( GLuint index, GLdouble x ) {
	fprintf( Stream, "glVertexAttrib1dARB\n" );
	fflush( Stream );
	dllVertexAttrib1dARB( index, x );
}

/* ---- logVertexAttrib1dvARB  0x004BB630 ---- */
static void QGLAPI logVertexAttrib1dvARB( GLuint index, const GLdouble *v ) {
	fprintf( Stream, "glVertexAttrib1dvARB\n" );
	fflush( Stream );
	dllVertexAttrib1dvARB( index, v );
}

/* ---- logVertexAttrib1fARB  0x004BB310 ---- */
static void QGLAPI logVertexAttrib1fARB( GLuint index, GLfloat x ) {
	fprintf( Stream, "glVertexAttrib1fARB\n" );
	fflush( Stream );
	dllVertexAttrib1fARB( index, x );
}

/* ---- logVertexAttrib1fvARB  0x004BB600 ---- */
static void QGLAPI logVertexAttrib1fvARB( GLuint index, const GLfloat *v ) {
	fprintf( Stream, "glVertexAttrib1fvARB\n" );
	fflush( Stream );
	dllVertexAttrib1fvARB( index, v );
}

/* ---- logVertexAttrib1sARB  0x004BB2E0 ---- */
static void QGLAPI logVertexAttrib1sARB( GLuint index, GLshort x ) {
	fprintf( Stream, "glVertexAttrib1sARB\n" );
	fflush( Stream );
	dllVertexAttrib1sARB( index, x );
}

/* ---- logVertexAttrib1svARB  0x004BB5D0 ---- */
static void QGLAPI logVertexAttrib1svARB( GLuint index, const GLshort *v ) {
	fprintf( Stream, "glVertexAttrib1svARB\n" );
	fflush( Stream );
	dllVertexAttrib1svARB( index, v );
}

/* ---- logVertexAttrib2dARB  0x004BB3E0 ---- */
static void QGLAPI logVertexAttrib2dARB( GLuint index, GLdouble x, GLdouble y ) {
	fprintf( Stream, "glVertexAttrib2dARB\n" );
	fflush( Stream );
	dllVertexAttrib2dARB( index, x, y );
}

/* ---- logVertexAttrib2dvARB  0x004BB6C0 ---- */
static void QGLAPI logVertexAttrib2dvARB( GLuint index, const GLdouble *v ) {
	fprintf( Stream, "glVertexAttrib2dvARB\n" );
	fflush( Stream );
	dllVertexAttrib2dvARB( index, v );
}

/* ---- logVertexAttrib2fARB  0x004BB3B0 ---- */
static void QGLAPI logVertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y ) {
	fprintf( Stream, "glVertexAttrib2fARB\n" );
	fflush( Stream );
	dllVertexAttrib2fARB( index, x, y );
}

/* ---- logVertexAttrib2fvARB  0x004BB690 ---- */
static void QGLAPI logVertexAttrib2fvARB( GLuint index, const GLfloat *v ) {
	fprintf( Stream, "glVertexAttrib2fvARB\n" );
	fflush( Stream );
	dllVertexAttrib2fvARB( index, v );
}

/* ---- logVertexAttrib2sARB  0x004BB380 ---- */
static void QGLAPI logVertexAttrib2sARB( GLuint index, GLshort x, GLshort y ) {
	fprintf( Stream, "glVertexAttrib2sARB\n" );
	fflush( Stream );
	dllVertexAttrib2sARB( index, x, y );
}

/* ---- logVertexAttrib2svARB  0x004BB660 ---- */
static void QGLAPI logVertexAttrib2svARB( GLuint index, const GLshort *v ) {
	fprintf( Stream, "glVertexAttrib2svARB\n" );
	fflush( Stream );
	dllVertexAttrib2svARB( index, v );
}

/* ---- logVertexAttrib3dARB  0x004BB490 ---- */
static void QGLAPI logVertexAttrib3dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z ) {
	fprintf( Stream, "glVertexAttrib3dARB\n" );
	fflush( Stream );
	dllVertexAttrib3dARB( index, x, y, z );
}

/* ---- logVertexAttrib3dvARB  0x004BB750 ---- */
static void QGLAPI logVertexAttrib3dvARB( GLuint index, const GLdouble *v ) {
	fprintf( Stream, "glVertexAttrib3dvARB\n" );
	fflush( Stream );
	dllVertexAttrib3dvARB( index, v );
}

/* ---- logVertexAttrib3fARB  0x004BB460 ---- */
static void QGLAPI logVertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z ) {
	fprintf( Stream, "glVertexAttrib3fARB\n" );
	fflush( Stream );
	dllVertexAttrib3fARB( index, x, y, z );
}

/* ---- logVertexAttrib3fvARB  0x004BB720 ---- */
static void QGLAPI logVertexAttrib3fvARB( GLuint index, const GLfloat *v ) {
	fprintf( Stream, "glVertexAttrib3fvARB\n" );
	fflush( Stream );
	dllVertexAttrib3fvARB( index, v );
}

/* ---- logVertexAttrib3sARB  0x004BB430 ---- */
static void QGLAPI logVertexAttrib3sARB( GLuint index, GLshort x, GLshort y, GLshort z ) {
	fprintf( Stream, "glVertexAttrib3sARB\n" );
	fflush( Stream );
	dllVertexAttrib3sARB( index, x, y, z );
}

/* ---- logVertexAttrib3svARB  0x004BB6F0 ---- */
static void QGLAPI logVertexAttrib3svARB( GLuint index, const GLshort *v ) {
	fprintf( Stream, "glVertexAttrib3svARB\n" );
	fflush( Stream );
	dllVertexAttrib3svARB( index, v );
}

/* ---- logVertexAttrib4NbvARB  0x004BB900 ---- */
static void QGLAPI logVertexAttrib4NbvARB( GLuint index, const GLbyte *v ) {
	fprintf( Stream, "glVertexAttrib4NbvARB\n" );
	fflush( Stream );
	dllVertexAttrib4NbvARB( index, v );
}

/* ---- logVertexAttrib4NivARB  0x004BB960 ---- */
static void QGLAPI logVertexAttrib4NivARB( GLuint index, const GLint *v ) {
	fprintf( Stream, "glVertexAttrib4NivARB\n" );
	fflush( Stream );
	dllVertexAttrib4NivARB( index, v );
}

/* ---- logVertexAttrib4NsvARB  0x004BB930 ---- */
static void QGLAPI logVertexAttrib4NsvARB( GLuint index, const GLshort *v ) {
	fprintf( Stream, "glVertexAttrib4NsvARB\n" );
	fflush( Stream );
	dllVertexAttrib4NsvARB( index, v );
}

/* ---- logVertexAttrib4NubARB  0x004BB5A0 ---- */
static void QGLAPI logVertexAttrib4NubARB( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w ) {
	fprintf( Stream, "glVertexAttrib4NubARB\n" );
	fflush( Stream );
	dllVertexAttrib4NubARB( index, x, y, z, w );
}

/* ---- logVertexAttrib4NubvARB  0x004BB990 ---- */
static void QGLAPI logVertexAttrib4NubvARB( GLuint index, const GLubyte *v ) {
	fprintf( Stream, "glVertexAttrib4NubvARB\n" );
	fflush( Stream );
	dllVertexAttrib4NubvARB( index, v );
}

/* ---- logVertexAttrib4NuivARB  0x004BB9F0 ---- */
static void QGLAPI logVertexAttrib4NuivARB( GLuint index, const GLuint *v ) {
	fprintf( Stream, "glVertexAttrib4NuivARB\n" );
	fflush( Stream );
	dllVertexAttrib4NuivARB( index, v );
}

/* ---- logVertexAttrib4NusvARB  0x004BB9C0 ---- */
static void QGLAPI logVertexAttrib4NusvARB( GLuint index, const GLushort *v ) {
	fprintf( Stream, "glVertexAttrib4NusvARB\n" );
	fflush( Stream );
	dllVertexAttrib4NusvARB( index, v );
}

/* ---- logVertexAttrib4bvARB  0x004BB780 ---- */
static void QGLAPI logVertexAttrib4bvARB( GLuint index, const GLbyte *v ) {
	fprintf( Stream, "glVertexAttrib4bvARB\n" );
	fflush( Stream );
	dllVertexAttrib4bvARB( index, v );
}

/* ---- logVertexAttrib4dARB  0x004BB540 ---- */
static void QGLAPI logVertexAttrib4dARB( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) {
	fprintf( Stream, "glVertexAttrib4dARB\n" );
	fflush( Stream );
	dllVertexAttrib4dARB( index, x, y, z, w );
}

/* ---- logVertexAttrib4dvARB  0x004BB8D0 ---- */
static void QGLAPI logVertexAttrib4dvARB( GLuint index, const GLdouble *v ) {
	fprintf( Stream, "glVertexAttrib4dvARB\n" );
	fflush( Stream );
	dllVertexAttrib4dvARB( index, v );
}

/* ---- logVertexAttrib4fARB  0x004BB510 ---- */
static void QGLAPI logVertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) {
	fprintf( Stream, "glVertexAttrib4fARB\n" );
	fflush( Stream );
	dllVertexAttrib4fARB( index, x, y, z, w );
}

/* ---- logVertexAttrib4fvARB  0x004BB8A0 ---- */
static void QGLAPI logVertexAttrib4fvARB( GLuint index, const GLfloat *v ) {
	fprintf( Stream, "glVertexAttrib4fvARB\n" );
	fflush( Stream );
	dllVertexAttrib4fvARB( index, v );
}

/* ---- logVertexAttrib4ivARB  0x004BB7E0 ---- */
static void QGLAPI logVertexAttrib4ivARB( GLuint index, const GLint *v ) {
	fprintf( Stream, "glVertexAttrib4ivARB\n" );
	fflush( Stream );
	dllVertexAttrib4ivARB( index, v );
}

/* ---- logVertexAttrib4sARB  0x004BB4E0 ---- */
static void QGLAPI logVertexAttrib4sARB( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w ) {
	fprintf( Stream, "glVertexAttrib4sARB\n" );
	fflush( Stream );
	dllVertexAttrib4sARB( index, x, y, z, w );
}

/* ---- logVertexAttrib4svARB  0x004BB7B0 ---- */
static void QGLAPI logVertexAttrib4svARB( GLuint index, const GLshort *v ) {
	fprintf( Stream, "glVertexAttrib4svARB\n" );
	fflush( Stream );
	dllVertexAttrib4svARB( index, v );
}

/* ---- logVertexAttrib4ubvARB  0x004BB810 ---- */
static void QGLAPI logVertexAttrib4ubvARB( GLuint index, const GLubyte *v ) {
	fprintf( Stream, "glVertexAttrib4ubvARB\n" );
	fflush( Stream );
	dllVertexAttrib4ubvARB( index, v );
}

/* ---- logVertexAttrib4uivARB  0x004BB870 ---- */
static void QGLAPI logVertexAttrib4uivARB( GLuint index, const GLuint *v ) {
	fprintf( Stream, "glVertexAttrib4uivARB\n" );
	fflush( Stream );
	dllVertexAttrib4uivARB( index, v );
}

/* ---- logVertexAttrib4usvARB  0x004BB840 ---- */
static void QGLAPI logVertexAttrib4usvARB( GLuint index, const GLushort *v ) {
	fprintf( Stream, "glVertexAttrib4usvARB\n" );
	fflush( Stream );
	dllVertexAttrib4usvARB( index, v );
}

/* ---- logVertexAttribPointerARB  0x004BBA20 ---- */
static void QGLAPI logVertexAttribPointerARB( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) {
	fprintf( Stream, "glVertexAttribPointerARB\n" );
	fflush( Stream );
	dllVertexAttribPointerARB( index, size, type, normalized, stride, pointer );
}

/* ---- logwglAllocateMemoryNV  0x004BAC50 ---- */
static void * WINAPI logwglAllocateMemoryNV( int size, float readfreq, float writefreq, float priority ) {
	fprintf( Stream, "wglAllocateMemoryNV\n" );
	fflush( Stream );
	return dllwglAllocateMemoryNV( size, readfreq, writefreq, priority );
}

/* ---- logwglFreeMemoryNV  0x004BAC80 ---- */
static void WINAPI logwglFreeMemoryNV( void *pointer ) {
	fprintf( Stream, "wglFreeMemoryNV\n" );
	fflush( Stream );
	dllwglFreeMemoryNV( pointer );
}

/* ---- logwglSwapIntervalEXT  0x004BFE50 ---- */
static BOOL WINAPI logwglSwapIntervalEXT( int interval ) {
	fprintf( Stream, "wglSwapIntervalEXT\n" );
	fflush( Stream );
	return dllwglSwapIntervalEXT( interval );
}

/* ---- logMultiTexCoord2fARB  0x004BA590 ---- */
static void QGLAPI logMultiTexCoord2fARB( GLenum target, GLfloat s, GLfloat t ) {
	fprintf( Stream, "glMultiTexCoord2fARB\n" );
	fflush( Stream );
	dllMultiTexCoord2fARB( target, s, t );
}

/* ---- logwglGetExtensionsStringEXT  0x004BFE20 ---- */
static const char * WINAPI logwglGetExtensionsStringEXT( void ) {
	fprintf( Stream, "wglGetExtensionsStringEXT\n" );
	fflush( Stream );
	return dllwglGetExtensionsStringEXT();
}

/* ---- logwglGetPixelFormatAttribivARB  0x004BFE80 ---- */
static BOOL WINAPI logwglGetPixelFormatAttribivARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues ) {
	fprintf( Stream, "wglGetPixelFormatAttribivARB\n" );
	fflush( Stream );
	return dllwglGetPixelFormatAttribivARB( hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues );
}

/* ---- logwglGetPixelFormatAttribfvARB  0x004BFEB0 ---- */
static BOOL WINAPI logwglGetPixelFormatAttribfvARB( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues ) {
	fprintf( Stream, "wglGetPixelFormatAttribfvARB\n" );
	fflush( Stream );
	return dllwglGetPixelFormatAttribfvARB( hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues );
}

/* ---- logwglChoosePixelFormatARB  0x004BFEE0 ---- */
static BOOL WINAPI logwglChoosePixelFormatARB( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats ) {
	fprintf( Stream, "wglChoosePixelFormatARB\n" );
	fflush( Stream );
	return dllwglChoosePixelFormatARB( hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats );
}

/* ---- logwglCreatePbufferARB  0x004BFF10 ---- */
static HPBUFFERARB WINAPI logwglCreatePbufferARB( HDC hdc, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList ) {
	fprintf( Stream, "wglCreatePbufferARB\n" );
	fflush( Stream );
	return dllwglCreatePbufferARB( hdc, iPixelFormat, iWidth, iHeight, piAttribList );
}

/* ---- logwglGetPbufferDCARB  0x004BFF40 ---- */
static HDC WINAPI logwglGetPbufferDCARB( HPBUFFERARB hPbuffer ) {
	fprintf( Stream, "wglGetPbufferDCARB\n" );
	fflush( Stream );
	return dllwglGetPbufferDCARB( hPbuffer );
}

/* ---- logwglReleasePbufferDCARB  0x004BFF70 ---- */
static int WINAPI logwglReleasePbufferDCARB( HPBUFFERARB hPbuffer, HDC hDC ) {
	fprintf( Stream, "wglReleasePbufferDCARB\n" );
	fflush( Stream );
	return dllwglReleasePbufferDCARB( hPbuffer, hDC );
}

/* ---- logwglDestroyPbufferARB  0x004BFFA0 ---- */
static BOOL WINAPI logwglDestroyPbufferARB( HPBUFFERARB hPbuffer ) {
	fprintf( Stream, "wglDestroyPbufferARB\n" );
	fflush( Stream );
	return dllwglDestroyPbufferARB( hPbuffer );
}

/* ---- logwglQueryPbufferARB  0x004BFFD0 ---- */
static BOOL WINAPI logwglQueryPbufferARB( HPBUFFERARB hPbuffer, int iAttribute, int *piValue ) {
	fprintf( Stream, "wglQueryPbufferARB\n" );
	fflush( Stream );
	return dllwglQueryPbufferARB( hPbuffer, iAttribute, piValue );
}

/* ---- logwglBindTexImageARB  0x004C0000 ---- */
static BOOL WINAPI logwglBindTexImageARB( HPBUFFERARB hPbuffer, int iBuffer ) {
	fprintf( Stream, "wglBindTexImageARB\n" );
	fflush( Stream );
	return dllwglBindTexImageARB( hPbuffer, iBuffer );
}

/* ---- logwglReleaseTexImageARB  0x004C0030 ---- */
static BOOL WINAPI logwglReleaseTexImageARB( HPBUFFERARB hPbuffer, int iBuffer ) {
	fprintf( Stream, "wglReleaseTexImageARB\n" );
	fflush( Stream );
	return dllwglReleaseTexImageARB( hPbuffer, iBuffer );
}

/* ---- logwglSetPbufferAttribARB  0x004C0060 ---- */
static BOOL WINAPI logwglSetPbufferAttribARB( HPBUFFERARB hPbuffer, const int *piAttribList ) {
	fprintf( Stream, "wglSetPbufferAttribARB\n" );
	fflush( Stream );
	return dllwglSetPbufferAttribARB( hPbuffer, piAttribList );
}

/* ---- QGL_EnableLogging  0x004CEB10 ---- */
void QGL_EnableLogging( qboolean enable ) {
	static qboolean isEnabled;      /* 0x00EA28D0 */

	/* Already logging: count r_logFile down one per call, stop at 0. */
	if ( isEnabled && enable ) {
		ri.Cvar_Set( "r_logFile", va( "%d", r_logFile->integer - 1 ) );
		if ( r_logFile->integer ) {
			return;
		}
		enable = qfalse;
	}

	if ( !enable && !isEnabled ) {
		return;
	}

	isEnabled = enable;             /* raw value, not normalised to 1 */

	if ( enable ) {
		if ( !Stream ) {            /* 0x019C0004 */
			struct tm  *newtime;
			time_t      aclock;
			char        buffer[1024];
			cvar_t     *basedir;

			time( &aclock );
			newtime = localtime( &aclock );

			asctime( newtime );     /* result unused, as retail */

			basedir = (cvar_t *)ri.Cvar_Get( "fs_basepath", "", 0 );
			Com_sprintf( buffer, sizeof( buffer ), "%s/gl.log", basedir->string );
			Stream = fopen( buffer, "wt" );

			fprintf( Stream, "%s\n", asctime( newtime ) );
			fflush( Stream );
		}

		/* 0x004CEC26: every non-NULL qgl pointer -> its log wrapper (0x004B94D0-0x004C0060). */
#define QGL_EXTRA_LOG_GL( n )   if ( qgl##n )  { qgl##n  = log##n;  }
#define QGL_EXTRA_LOG_WGL( n )  if ( qwgl##n ) { qwgl##n = logwgl##n; }
		QGL_LOG_EXT_ENTRIES( QGL_EXTRA_LOG_GL, QGL_EXTRA_LOG_WGL )
#define QGL_CORE_GL( n )    if ( qgl##n )  { qgl##n  = log##n;  }
#define QGL_CORE_WGL( n )   if ( qwgl##n ) { qwgl##n = logwgl##n; }
#include "../renderer/qgl_entries.h"
		QGL_LOG_TAIL_ENTRIES( QGL_EXTRA_LOG_GL, QGL_EXTRA_LOG_WGL )
	} else {
		if ( Stream ) {
			fprintf( Stream, "*** CLOSING LOG ***\n" );
			fclose( Stream );
			Stream = NULL;
		}

		/* 0x004D1244: restore every qgl pointer from its driver slot. */
		QGL_LOG_EXT_ENTRIES( QGL_EXTRA_RESTORE_GL, QGL_EXTRA_RESTORE_WGL )
#define QGL_CORE_GL( n )    qgl##n  = dll##n;
#define QGL_CORE_WGL( n )   qwgl##n = dllwgl##n;
#include "../renderer/qgl_entries.h"
		QGL_LOG_TAIL_ENTRIES( QGL_EXTRA_RESTORE_GL, QGL_EXTRA_RESTORE_WGL )
	}
}
