
#ifndef TR_QGL_H
#define TR_QGL_H

#include "tr_gl_types.h"

#ifndef QGLAPI
#define QGLAPI  __stdcall
#endif

#ifdef QGL_OWNER
#define QGLDECL
#else
#define QGLDECL extern
#endif

#define qglActiveTextureARB   qgls_ActiveTextureARB   /* 0x016C3F34 */
#define qglGetString          qgls_GetString          /* 0x016C3ED4 */
#define qglLockArraysEXT      qgls_LockArraysEXT      /* 0x016C40FC */

QGLDECL void ( QGLAPI * qglAccum )( GLenum op, GLfloat value );
QGLDECL void ( QGLAPI * qglAlphaFunc )( GLenum func, GLclampf ref );
QGLDECL GLboolean ( QGLAPI * qglAreTexturesResident )( GLsizei n, const GLuint *textures, GLboolean *residences );
QGLDECL void ( QGLAPI * qglArrayElement )( GLint i );
QGLDECL void ( QGLAPI * qglBegin )( GLenum mode );
QGLDECL void ( QGLAPI * qglBindTexture )( GLenum target, GLuint texture );
QGLDECL void ( QGLAPI * qglBitmap )( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
QGLDECL void ( QGLAPI * qglBlendFunc )( GLenum sfactor, GLenum dfactor );
QGLDECL void ( QGLAPI * qglCallList )( GLuint list );
QGLDECL void ( QGLAPI * qglCallLists )( GLsizei n, GLenum type, const GLvoid *lists );
QGLDECL void ( QGLAPI * qglClear )( GLbitfield mask );
QGLDECL void ( QGLAPI * qglClearAccum )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
QGLDECL void ( QGLAPI * qglClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
QGLDECL void ( QGLAPI * qglClearDepth )( GLclampd depth );
QGLDECL void ( QGLAPI * qglClearIndex )( GLfloat c );
QGLDECL void ( QGLAPI * qglClearStencil )( GLint s );
QGLDECL void ( QGLAPI * qglClipPlane )( GLenum plane, const GLdouble *equation );
QGLDECL void ( QGLAPI * qglColor3b )( GLbyte red, GLbyte green, GLbyte blue );
QGLDECL void ( QGLAPI * qglColor3bv )( const GLbyte *v );
QGLDECL void ( QGLAPI * qglColor3d )( GLdouble red, GLdouble green, GLdouble blue );
QGLDECL void ( QGLAPI * qglColor3dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglColor3f )( GLfloat red, GLfloat green, GLfloat blue );
QGLDECL void ( QGLAPI * qglColor3fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglColor3i )( GLint red, GLint green, GLint blue );
QGLDECL void ( QGLAPI * qglColor3iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglColor3s )( GLshort red, GLshort green, GLshort blue );
QGLDECL void ( QGLAPI * qglColor3sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglColor3ub )( GLubyte red, GLubyte green, GLubyte blue );
QGLDECL void ( QGLAPI * qglColor3ubv )( const GLubyte *v );
QGLDECL void ( QGLAPI * qglColor3ui )( GLuint red, GLuint green, GLuint blue );
QGLDECL void ( QGLAPI * qglColor3uiv )( const GLuint *v );
QGLDECL void ( QGLAPI * qglColor3us )( GLushort red, GLushort green, GLushort blue );
QGLDECL void ( QGLAPI * qglColor3usv )( const GLushort *v );
QGLDECL void ( QGLAPI * qglColor4b )( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );
QGLDECL void ( QGLAPI * qglColor4bv )( const GLbyte *v );
QGLDECL void ( QGLAPI * qglColor4d )( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );
QGLDECL void ( QGLAPI * qglColor4dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglColor4f )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
QGLDECL void ( QGLAPI * qglColor4fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglColor4i )( GLint red, GLint green, GLint blue, GLint alpha );
QGLDECL void ( QGLAPI * qglColor4iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglColor4s )( GLshort red, GLshort green, GLshort blue, GLshort alpha );
QGLDECL void ( QGLAPI * qglColor4sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglColor4ub )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
QGLDECL void ( QGLAPI * qglColor4ubv )( const GLubyte *v );
QGLDECL void ( QGLAPI * qglColor4ui )( GLuint red, GLuint green, GLuint blue, GLuint alpha );
QGLDECL void ( QGLAPI * qglColor4uiv )( const GLuint *v );
QGLDECL void ( QGLAPI * qglColor4us )( GLushort red, GLushort green, GLushort blue, GLushort alpha );
QGLDECL void ( QGLAPI * qglColor4usv )( const GLushort *v );
QGLDECL void ( QGLAPI * qglColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
QGLDECL void ( QGLAPI * qglColorMaterial )( GLenum face, GLenum mode );
QGLDECL void ( QGLAPI * qglColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglCopyPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type );
QGLDECL void ( QGLAPI * qglCopyTexImage1D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border );
QGLDECL void ( QGLAPI * qglCopyTexImage2D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
QGLDECL void ( QGLAPI * qglCopyTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
QGLDECL void ( QGLAPI * qglCopyTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
QGLDECL void ( QGLAPI * qglCullFace )( GLenum mode );
QGLDECL void ( QGLAPI * qglDeleteLists )( GLuint list, GLsizei range );
QGLDECL void ( QGLAPI * qglDeleteTextures )( GLsizei n, const GLuint *textures );
QGLDECL void ( QGLAPI * qglDepthFunc )( GLenum func );
QGLDECL void ( QGLAPI * qglDepthMask )( GLboolean flag );
QGLDECL void ( QGLAPI * qglDepthRange )( GLclampd zNear, GLclampd zFar );
QGLDECL void ( QGLAPI * qglDisable )( GLenum cap );
QGLDECL void ( QGLAPI * qglDisableClientState )( GLenum array );
QGLDECL void ( QGLAPI * qglDrawArrays )( GLenum mode, GLint first, GLsizei count );
QGLDECL void ( QGLAPI * qglDrawBuffer )( GLenum mode );
QGLDECL void ( QGLAPI * qglDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );
QGLDECL void ( QGLAPI * qglDrawPixels )( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
QGLDECL void ( QGLAPI * qglEdgeFlag )( GLboolean flag );
QGLDECL void ( QGLAPI * qglEdgeFlagPointer )( GLsizei stride, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglEdgeFlagv )( const GLboolean *flag );
QGLDECL void ( QGLAPI * qglEnable )( GLenum cap );
QGLDECL void ( QGLAPI * qglEnableClientState )( GLenum array );
QGLDECL void ( QGLAPI * qglEnd )( void );
QGLDECL void ( QGLAPI * qglEndList )( void );
QGLDECL void ( QGLAPI * qglEvalCoord1d )( GLdouble u );
QGLDECL void ( QGLAPI * qglEvalCoord1dv )( const GLdouble *u );
QGLDECL void ( QGLAPI * qglEvalCoord1f )( GLfloat u );
QGLDECL void ( QGLAPI * qglEvalCoord1fv )( const GLfloat *u );
QGLDECL void ( QGLAPI * qglEvalCoord2d )( GLdouble u, GLdouble v );
QGLDECL void ( QGLAPI * qglEvalCoord2dv )( const GLdouble *u );
QGLDECL void ( QGLAPI * qglEvalCoord2f )( GLfloat u, GLfloat v );
QGLDECL void ( QGLAPI * qglEvalCoord2fv )( const GLfloat *u );
QGLDECL void ( QGLAPI * qglEvalMesh1 )( GLenum mode, GLint i1, GLint i2 );
QGLDECL void ( QGLAPI * qglEvalMesh2 )( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
QGLDECL void ( QGLAPI * qglEvalPoint1 )( GLint i );
QGLDECL void ( QGLAPI * qglEvalPoint2 )( GLint i, GLint j );
QGLDECL void ( QGLAPI * qglFeedbackBuffer )( GLsizei size, GLenum type, GLfloat *buffer );
QGLDECL void ( QGLAPI * qglFinish )( void );
QGLDECL void ( QGLAPI * qglFlush )( void );
QGLDECL void ( QGLAPI * qglFogf )( GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglFogfv )( GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglFogi )( GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglFogiv )( GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglFrontFace )( GLenum mode );
QGLDECL void ( QGLAPI * qglFrustum )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
QGLDECL GLuint ( QGLAPI * qglGenLists )( GLsizei range );
QGLDECL void ( QGLAPI * qglGenTextures )( GLsizei n, GLuint *textures );
QGLDECL void ( QGLAPI * qglGetBooleanv )( GLenum pname, GLboolean *params );
QGLDECL void ( QGLAPI * qglGetClipPlane )( GLenum plane, GLdouble *equation );
QGLDECL void ( QGLAPI * qglGetDoublev )( GLenum pname, GLdouble *params );
QGLDECL GLenum ( QGLAPI * qglGetError )( void );
QGLDECL void ( QGLAPI * qglGetFloatv )( GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetIntegerv )( GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetLightfv )( GLenum light, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetLightiv )( GLenum light, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetMapdv )( GLenum target, GLenum query, GLdouble *v );
QGLDECL void ( QGLAPI * qglGetMapfv )( GLenum target, GLenum query, GLfloat *v );
QGLDECL void ( QGLAPI * qglGetMapiv )( GLenum target, GLenum query, GLint *v );
QGLDECL void ( QGLAPI * qglGetMaterialfv )( GLenum face, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetMaterialiv )( GLenum face, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetPixelMapfv )( GLenum map, GLfloat *values );
QGLDECL void ( QGLAPI * qglGetPixelMapuiv )( GLenum map, GLuint *values );
QGLDECL void ( QGLAPI * qglGetPixelMapusv )( GLenum map, GLushort *values );
QGLDECL void ( QGLAPI * qglGetPointerv )( GLenum pname, GLvoid* *params );
QGLDECL void ( QGLAPI * qglGetPolygonStipple )( GLubyte *mask );
QGLDECL const GLubyte *( QGLAPI * qglGetString )( GLenum name );
QGLDECL void ( QGLAPI * qglGetTexEnvfv )( GLenum target, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetTexEnviv )( GLenum target, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetTexGendv )( GLenum coord, GLenum pname, GLdouble *params );
QGLDECL void ( QGLAPI * qglGetTexGenfv )( GLenum coord, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetTexGeniv )( GLenum coord, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetTexImage )( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels );
QGLDECL void ( QGLAPI * qglGetTexLevelParameterfv )( GLenum target, GLint level, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetTexLevelParameteriv )( GLenum target, GLint level, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetTexParameterfv )( GLenum target, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetTexParameteriv )( GLenum target, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglHint )( GLenum target, GLenum mode );
QGLDECL void ( QGLAPI * qglIndexMask )( GLuint mask );
QGLDECL void ( QGLAPI * qglIndexPointer )( GLenum type, GLsizei stride, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglIndexd )( GLdouble c );
QGLDECL void ( QGLAPI * qglIndexdv )( const GLdouble *c );
QGLDECL void ( QGLAPI * qglIndexf )( GLfloat c );
QGLDECL void ( QGLAPI * qglIndexfv )( const GLfloat *c );
QGLDECL void ( QGLAPI * qglIndexi )( GLint c );
QGLDECL void ( QGLAPI * qglIndexiv )( const GLint *c );
QGLDECL void ( QGLAPI * qglIndexs )( GLshort c );
QGLDECL void ( QGLAPI * qglIndexsv )( const GLshort *c );
QGLDECL void ( QGLAPI * qglIndexub )( GLubyte c );
QGLDECL void ( QGLAPI * qglIndexubv )( const GLubyte *c );
QGLDECL void ( QGLAPI * qglInitNames )( void );
QGLDECL void ( QGLAPI * qglInterleavedArrays )( GLenum format, GLsizei stride, const GLvoid *pointer );
QGLDECL GLboolean ( QGLAPI * qglIsEnabled )( GLenum cap );
QGLDECL GLboolean ( QGLAPI * qglIsList )( GLuint list );
QGLDECL GLboolean ( QGLAPI * qglIsTexture )( GLuint texture );
QGLDECL void ( QGLAPI * qglLightModelf )( GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglLightModelfv )( GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglLightModeli )( GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglLightModeliv )( GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglLightf )( GLenum light, GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglLightfv )( GLenum light, GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglLighti )( GLenum light, GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglLightiv )( GLenum light, GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglLineStipple )( GLint factor, GLushort pattern );
QGLDECL void ( QGLAPI * qglLineWidth )( GLfloat width );
QGLDECL void ( QGLAPI * qglListBase )( GLuint base );
QGLDECL void ( QGLAPI * qglLoadIdentity )( void );
QGLDECL void ( QGLAPI * qglLoadMatrixd )( const GLdouble *m );
QGLDECL void ( QGLAPI * qglLoadMatrixf )( const GLfloat *m );
QGLDECL void ( QGLAPI * qglLoadName )( GLuint name );
QGLDECL void ( QGLAPI * qglLogicOp )( GLenum opcode );
QGLDECL void ( QGLAPI * qglMap1d )( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
QGLDECL void ( QGLAPI * qglMap1f )( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
QGLDECL void ( QGLAPI * qglMap2d )( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
QGLDECL void ( QGLAPI * qglMap2f )( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
QGLDECL void ( QGLAPI * qglMapGrid1d )( GLint un, GLdouble u1, GLdouble u2 );
QGLDECL void ( QGLAPI * qglMapGrid1f )( GLint un, GLfloat u1, GLfloat u2 );
QGLDECL void ( QGLAPI * qglMapGrid2d )( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 );
QGLDECL void ( QGLAPI * qglMapGrid2f )( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 );
QGLDECL void ( QGLAPI * qglMaterialf )( GLenum face, GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglMaterialfv )( GLenum face, GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglMateriali )( GLenum face, GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglMaterialiv )( GLenum face, GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglMatrixMode )( GLenum mode );
QGLDECL void ( QGLAPI * qglMultMatrixd )( const GLdouble *m );
QGLDECL void ( QGLAPI * qglMultMatrixf )( const GLfloat *m );
QGLDECL void ( QGLAPI * qglNewList )( GLuint list, GLenum mode );
QGLDECL void ( QGLAPI * qglNormal3b )( GLbyte nx, GLbyte ny, GLbyte nz );
QGLDECL void ( QGLAPI * qglNormal3bv )( const GLbyte *v );
QGLDECL void ( QGLAPI * qglNormal3d )( GLdouble nx, GLdouble ny, GLdouble nz );
QGLDECL void ( QGLAPI * qglNormal3dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglNormal3f )( GLfloat nx, GLfloat ny, GLfloat nz );
QGLDECL void ( QGLAPI * qglNormal3fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglNormal3i )( GLint nx, GLint ny, GLint nz );
QGLDECL void ( QGLAPI * qglNormal3iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglNormal3s )( GLshort nx, GLshort ny, GLshort nz );
QGLDECL void ( QGLAPI * qglNormal3sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglNormalPointer )( GLenum type, GLsizei stride, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
QGLDECL void ( QGLAPI * qglPassThrough )( GLfloat token );
QGLDECL void ( QGLAPI * qglPixelMapfv )( GLenum map, GLsizei mapsize, const GLfloat *values );
QGLDECL void ( QGLAPI * qglPixelMapuiv )( GLenum map, GLsizei mapsize, const GLuint *values );
QGLDECL void ( QGLAPI * qglPixelMapusv )( GLenum map, GLsizei mapsize, const GLushort *values );
QGLDECL void ( QGLAPI * qglPixelStoref )( GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglPixelStorei )( GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglPixelTransferf )( GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglPixelTransferi )( GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglPixelZoom )( GLfloat xfactor, GLfloat yfactor );
QGLDECL void ( QGLAPI * qglPointSize )( GLfloat size );
QGLDECL void ( QGLAPI * qglPolygonMode )( GLenum face, GLenum mode );
QGLDECL void ( QGLAPI * qglPolygonOffset )( GLfloat factor, GLfloat units );
QGLDECL void ( QGLAPI * qglPolygonStipple )( const GLubyte *mask );
QGLDECL void ( QGLAPI * qglPopAttrib )( void );
QGLDECL void ( QGLAPI * qglPopClientAttrib )( void );
QGLDECL void ( QGLAPI * qglPopMatrix )( void );
QGLDECL void ( QGLAPI * qglPopName )( void );
QGLDECL void ( QGLAPI * qglPrioritizeTextures )( GLsizei n, const GLuint *textures, const GLclampf *priorities );
QGLDECL void ( QGLAPI * qglPushAttrib )( GLbitfield mask );
QGLDECL void ( QGLAPI * qglPushClientAttrib )( GLbitfield mask );
QGLDECL void ( QGLAPI * qglPushMatrix )( void );
QGLDECL void ( QGLAPI * qglPushName )( GLuint name );
QGLDECL void ( QGLAPI * qglRasterPos2d )( GLdouble x, GLdouble y );
QGLDECL void ( QGLAPI * qglRasterPos2dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglRasterPos2f )( GLfloat x, GLfloat y );
QGLDECL void ( QGLAPI * qglRasterPos2fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglRasterPos2i )( GLint x, GLint y );
QGLDECL void ( QGLAPI * qglRasterPos2iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglRasterPos2s )( GLshort x, GLshort y );
QGLDECL void ( QGLAPI * qglRasterPos2sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglRasterPos3d )( GLdouble x, GLdouble y, GLdouble z );
QGLDECL void ( QGLAPI * qglRasterPos3dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglRasterPos3f )( GLfloat x, GLfloat y, GLfloat z );
QGLDECL void ( QGLAPI * qglRasterPos3fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglRasterPos3i )( GLint x, GLint y, GLint z );
QGLDECL void ( QGLAPI * qglRasterPos3iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglRasterPos3s )( GLshort x, GLshort y, GLshort z );
QGLDECL void ( QGLAPI * qglRasterPos3sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglRasterPos4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
QGLDECL void ( QGLAPI * qglRasterPos4dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglRasterPos4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
QGLDECL void ( QGLAPI * qglRasterPos4fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglRasterPos4i )( GLint x, GLint y, GLint z, GLint w );
QGLDECL void ( QGLAPI * qglRasterPos4iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglRasterPos4s )( GLshort x, GLshort y, GLshort z, GLshort w );
QGLDECL void ( QGLAPI * qglRasterPos4sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglReadBuffer )( GLenum mode );
QGLDECL void ( QGLAPI * qglReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels );
QGLDECL void ( QGLAPI * qglRectd )( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );
QGLDECL void ( QGLAPI * qglRectdv )( const GLdouble *v1, const GLdouble *v2 );
QGLDECL void ( QGLAPI * qglRectf )( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );
QGLDECL void ( QGLAPI * qglRectfv )( const GLfloat *v1, const GLfloat *v2 );
QGLDECL void ( QGLAPI * qglRecti )( GLint x1, GLint y1, GLint x2, GLint y2 );
QGLDECL void ( QGLAPI * qglRectiv )( const GLint *v1, const GLint *v2 );
QGLDECL void ( QGLAPI * qglRects )( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );
QGLDECL void ( QGLAPI * qglRectsv )( const GLshort *v1, const GLshort *v2 );
QGLDECL GLint ( QGLAPI * qglRenderMode )( GLenum mode );
QGLDECL void ( QGLAPI * qglRotated )( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
QGLDECL void ( QGLAPI * qglRotatef )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
QGLDECL void ( QGLAPI * qglScaled )( GLdouble x, GLdouble y, GLdouble z );
QGLDECL void ( QGLAPI * qglScalef )( GLfloat x, GLfloat y, GLfloat z );
QGLDECL void ( QGLAPI * qglScissor )( GLint x, GLint y, GLsizei width, GLsizei height );
QGLDECL void ( QGLAPI * qglSelectBuffer )( GLsizei size, GLuint *buffer );
QGLDECL void ( QGLAPI * qglShadeModel )( GLenum mode );
QGLDECL void ( QGLAPI * qglStencilFunc )( GLenum func, GLint ref, GLuint mask );
QGLDECL void ( QGLAPI * qglStencilMask )( GLuint mask );
QGLDECL void ( QGLAPI * qglStencilOp )( GLenum fail, GLenum zfail, GLenum zpass );
QGLDECL void ( QGLAPI * qglTexCoord1d )( GLdouble s );
QGLDECL void ( QGLAPI * qglTexCoord1dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglTexCoord1f )( GLfloat s );
QGLDECL void ( QGLAPI * qglTexCoord1fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglTexCoord1i )( GLint s );
QGLDECL void ( QGLAPI * qglTexCoord1iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglTexCoord1s )( GLshort s );
QGLDECL void ( QGLAPI * qglTexCoord1sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglTexCoord2d )( GLdouble s, GLdouble t );
QGLDECL void ( QGLAPI * qglTexCoord2dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglTexCoord2f )( GLfloat s, GLfloat t );
QGLDECL void ( QGLAPI * qglTexCoord2fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglTexCoord2i )( GLint s, GLint t );
QGLDECL void ( QGLAPI * qglTexCoord2iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglTexCoord2s )( GLshort s, GLshort t );
QGLDECL void ( QGLAPI * qglTexCoord2sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglTexCoord3d )( GLdouble s, GLdouble t, GLdouble r );
QGLDECL void ( QGLAPI * qglTexCoord3dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglTexCoord3f )( GLfloat s, GLfloat t, GLfloat r );
QGLDECL void ( QGLAPI * qglTexCoord3fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglTexCoord3i )( GLint s, GLint t, GLint r );
QGLDECL void ( QGLAPI * qglTexCoord3iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglTexCoord3s )( GLshort s, GLshort t, GLshort r );
QGLDECL void ( QGLAPI * qglTexCoord3sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglTexCoord4d )( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
QGLDECL void ( QGLAPI * qglTexCoord4dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglTexCoord4f )( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
QGLDECL void ( QGLAPI * qglTexCoord4fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglTexCoord4i )( GLint s, GLint t, GLint r, GLint q );
QGLDECL void ( QGLAPI * qglTexCoord4iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglTexCoord4s )( GLshort s, GLshort t, GLshort r, GLshort q );
QGLDECL void ( QGLAPI * qglTexCoord4sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglTexEnvf )( GLenum target, GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglTexEnvfv )( GLenum target, GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglTexEnvi )( GLenum target, GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglTexEnviv )( GLenum target, GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglTexGend )( GLenum coord, GLenum pname, GLdouble param );
QGLDECL void ( QGLAPI * qglTexGendv )( GLenum coord, GLenum pname, const GLdouble *params );
QGLDECL void ( QGLAPI * qglTexGenf )( GLenum coord, GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglTexGenfv )( GLenum coord, GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglTexGeni )( GLenum coord, GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglTexGeniv )( GLenum coord, GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglTexImage1D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
QGLDECL void ( QGLAPI * qglTexImage2D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels );
QGLDECL void ( QGLAPI * qglTexParameterf )( GLenum target, GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglTexParameteri )( GLenum target, GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglTexParameteriv )( GLenum target, GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels );
QGLDECL void ( QGLAPI * qglTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels );
QGLDECL void ( QGLAPI * qglTranslated )( GLdouble x, GLdouble y, GLdouble z );
QGLDECL void ( QGLAPI * qglTranslatef )( GLfloat x, GLfloat y, GLfloat z );
QGLDECL void ( QGLAPI * qglVertex2d )( GLdouble x, GLdouble y );
QGLDECL void ( QGLAPI * qglVertex2dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglVertex2f )( GLfloat x, GLfloat y );
QGLDECL void ( QGLAPI * qglVertex2fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglVertex2i )( GLint x, GLint y );
QGLDECL void ( QGLAPI * qglVertex2iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglVertex2s )( GLshort x, GLshort y );
QGLDECL void ( QGLAPI * qglVertex2sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglVertex3d )( GLdouble x, GLdouble y, GLdouble z );
QGLDECL void ( QGLAPI * qglVertex3dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglVertex3f )( GLfloat x, GLfloat y, GLfloat z );
QGLDECL void ( QGLAPI * qglVertex3fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglVertex3i )( GLint x, GLint y, GLint z );
QGLDECL void ( QGLAPI * qglVertex3iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglVertex3s )( GLshort x, GLshort y, GLshort z );
QGLDECL void ( QGLAPI * qglVertex3sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglVertex4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
QGLDECL void ( QGLAPI * qglVertex4dv )( const GLdouble *v );
QGLDECL void ( QGLAPI * qglVertex4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
QGLDECL void ( QGLAPI * qglVertex4fv )( const GLfloat *v );
QGLDECL void ( QGLAPI * qglVertex4i )( GLint x, GLint y, GLint z, GLint w );
QGLDECL void ( QGLAPI * qglVertex4iv )( const GLint *v );
QGLDECL void ( QGLAPI * qglVertex4s )( GLshort x, GLshort y, GLshort z, GLshort w );
QGLDECL void ( QGLAPI * qglVertex4sv )( const GLshort *v );
QGLDECL void ( QGLAPI * qglVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglViewport )( GLint x, GLint y, GLsizei width, GLsizei height );


QGLDECL void ( QGLAPI * qglActiveTextureARB )( GLenum texture );
QGLDECL void ( QGLAPI * qglAlphaFragmentOp1ATI )( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
QGLDECL void ( QGLAPI * qglAlphaFragmentOp2ATI )( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
QGLDECL void ( QGLAPI * qglAlphaFragmentOp3ATI )( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
QGLDECL void ( QGLAPI * qglArrayObjectATI )( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
QGLDECL void ( QGLAPI * qglBeginFragmentShaderATI )( void );
QGLDECL void ( QGLAPI * qglBindBufferARB )( GLenum target, GLuint buffer );
QGLDECL void ( QGLAPI * qglBindFragmentShaderATI )( GLuint id );
QGLDECL void ( QGLAPI * qglBindProgramARB )( GLenum target, GLuint program );
QGLDECL void ( QGLAPI * qglBufferDataARB )( GLenum target, GLsizei size, const GLvoid *data, GLenum usage );
QGLDECL void ( QGLAPI * qglBufferSubDataARB )( GLenum target, GLint offset, GLsizei size, const GLvoid *data );
QGLDECL void ( QGLAPI * qglClientActiveTextureARB )( GLenum texture );
QGLDECL void ( QGLAPI * qglColorFragmentOp1ATI )( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
QGLDECL void ( QGLAPI * qglColorFragmentOp2ATI )( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
QGLDECL void ( QGLAPI * qglColorFragmentOp3ATI )( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
QGLDECL void ( QGLAPI * qglCombinerInputNV )( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
QGLDECL void ( QGLAPI * qglCombinerOutputNV )( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum );
QGLDECL void ( QGLAPI * qglCombinerParameterfNV )( GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglCombinerParameterfvNV )( GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglCombinerParameteriNV )( GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglCombinerParameterivNV )( GLenum pname, const GLint *params );
QGLDECL void ( QGLAPI * qglCombinerStageParameterfvNV )( GLenum stage, GLenum pname, const GLfloat *params );
QGLDECL void ( QGLAPI * qglCompressedTexImage1DARB )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data );
QGLDECL void ( QGLAPI * qglCompressedTexImage2DARB )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data );
QGLDECL void ( QGLAPI * qglCompressedTexImage3DARB )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data );
QGLDECL void ( QGLAPI * qglCompressedTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data );
QGLDECL void ( QGLAPI * qglCompressedTexSubImage2DARB )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data );
QGLDECL void ( QGLAPI * qglCompressedTexSubImage3DARB )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data );
QGLDECL void ( QGLAPI * qglDeleteBuffersARB )( GLsizei n, const GLuint *buffers );
QGLDECL void ( QGLAPI * qglDeleteFencesNV )( GLsizei n, const GLuint *fences );
QGLDECL void ( QGLAPI * qglDeleteFragmentShaderATI )( GLuint id );
QGLDECL void ( QGLAPI * qglDeleteProgramsARB )( GLsizei n, const GLuint *programs );
QGLDECL void ( QGLAPI * qglDisableVertexAttribArrayARB )( GLuint index );
QGLDECL void ( QGLAPI * qglDrawElementArrayATI )( GLenum mode, GLsizei count );
QGLDECL void ( QGLAPI * qglDrawRangeElementArrayATI )( GLenum mode, GLuint start, GLuint end, GLsizei count );
QGLDECL void ( QGLAPI * qglDrawRangeElementsEXT )( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
QGLDECL void ( QGLAPI * qglElementPointerATI )( GLenum type, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglEnableVertexAttribArrayARB )( GLuint index );
QGLDECL void ( QGLAPI * qglEndFragmentShaderATI )( void );
QGLDECL void ( QGLAPI * qglFinalCombinerInputNV )( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
QGLDECL void ( QGLAPI * qglFinishFenceNV )( GLuint fence );
QGLDECL void ( QGLAPI * qglFlushVertexArrayRangeNV )( void );
QGLDECL void ( QGLAPI * qglFreeObjectBufferATI )( GLuint buffer );
QGLDECL void ( QGLAPI * qglGenBuffersARB )( GLsizei n, GLuint *buffers );
QGLDECL void ( QGLAPI * qglGenFencesNV )( GLsizei n, GLuint *fences );
QGLDECL GLuint ( QGLAPI * qglGenFragmentShadersATI )( GLuint range );
QGLDECL void ( QGLAPI * qglGenProgramsARB )( GLsizei n, GLuint *programs );
QGLDECL void ( QGLAPI * qglGetArrayObjectfvATI )( GLenum array, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetArrayObjectivATI )( GLenum array, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetBufferParameterivARB )( GLenum target, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetBufferPointervARB )( GLenum target, GLenum pname, GLvoid **params );
QGLDECL void ( QGLAPI * qglGetBufferSubDataARB )( GLenum target, GLint offset, GLsizei size, GLvoid *data );
QGLDECL void ( QGLAPI * qglGetCombinerInputParameterfvNV )( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetCombinerInputParameterivNV )( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetCombinerOutputParameterfvNV )( GLenum stage, GLenum portion, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetCombinerOutputParameterivNV )( GLenum stage, GLenum portion, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetCombinerStageParameterfvNV )( GLenum stage, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetCompressedTexImage )( GLenum target, GLint level, GLvoid *img );
QGLDECL void ( QGLAPI * qglGetFenceivNV )( GLuint fence, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetFinalCombinerInputParameterfvNV )( GLenum variable, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetFinalCombinerInputParameterivNV )( GLenum variable, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetObjectBufferfvATI )( GLuint buffer, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetObjectBufferivATI )( GLuint buffer, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetProgramEnvParameterdvARB )( GLenum target, GLuint index, GLdouble *params );
QGLDECL void ( QGLAPI * qglGetProgramEnvParameterfvARB )( GLenum target, GLuint index, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetProgramLocalParameterdvARB )( GLenum target, GLuint index, GLdouble *params );
QGLDECL void ( QGLAPI * qglGetProgramLocalParameterfvARB )( GLenum target, GLuint index, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetProgramStringARB )( GLenum target, GLenum pname, GLvoid *string );
QGLDECL void ( QGLAPI * qglGetProgramivARB )( GLenum target, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetVariantArrayObjectfvATI )( GLuint id, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetVariantArrayObjectivATI )( GLuint id, GLenum pname, GLint *params );
QGLDECL void ( QGLAPI * qglGetVertexAttribPointervARB )( GLuint index, GLenum pname, GLvoid **pointer );
QGLDECL void ( QGLAPI * qglGetVertexAttribdvARB )( GLuint index, GLenum pname, GLdouble *params );
QGLDECL void ( QGLAPI * qglGetVertexAttribfvARB )( GLuint index, GLenum pname, GLfloat *params );
QGLDECL void ( QGLAPI * qglGetVertexAttribivARB )( GLuint index, GLenum pname, GLint *params );
QGLDECL GLboolean ( QGLAPI * qglIsBufferARB )( GLuint buffer );
QGLDECL GLboolean ( QGLAPI * qglIsFenceNV )( GLuint fence );
QGLDECL GLboolean ( QGLAPI * qglIsObjectBufferATI )( GLuint buffer );
QGLDECL GLboolean ( QGLAPI * qglIsProgramARB )( GLuint program );
QGLDECL void ( QGLAPI * qglLockArraysEXT )( GLint first, GLsizei count );
QGLDECL GLvoid *( QGLAPI * qglMapBufferARB )( GLenum target, GLenum access );
QGLDECL GLuint ( QGLAPI * qglNewObjectBufferATI )( GLsizei size, const GLvoid *pointer, GLenum usage );
QGLDECL void ( QGLAPI * qglPNTrianglesfATI )( GLenum pname, GLfloat param );
QGLDECL void ( QGLAPI * qglPNTrianglesiATI )( GLenum pname, GLint param );
QGLDECL void ( QGLAPI * qglPassTexCoordATI )( GLuint dst, GLuint coord, GLenum swizzle );
QGLDECL void ( QGLAPI * qglProgramEnvParameter4dARB )( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
QGLDECL void ( QGLAPI * qglProgramEnvParameter4dvARB )( GLenum target, GLuint index, const GLdouble *params );
QGLDECL void ( QGLAPI * qglProgramEnvParameter4fARB )( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
QGLDECL void ( QGLAPI * qglProgramEnvParameter4fvARB )( GLenum target, GLuint index, const GLfloat *params );
QGLDECL void ( QGLAPI * qglProgramLocalParameter4dARB )( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
QGLDECL void ( QGLAPI * qglProgramLocalParameter4dvARB )( GLenum target, GLuint index, const GLdouble *params );
QGLDECL void ( QGLAPI * qglProgramLocalParameter4fARB )( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
QGLDECL void ( QGLAPI * qglProgramLocalParameter4fvARB )( GLenum target, GLuint index, const GLfloat *params );
QGLDECL void ( QGLAPI * qglProgramStringARB )( GLenum target, GLenum format, GLsizei len, const GLvoid *string );
QGLDECL void ( QGLAPI * qglSampleMapATI )( GLuint dst, GLuint interp, GLenum swizzle );
QGLDECL void ( QGLAPI * qglSetFenceNV )( GLuint fence, GLenum condition );
QGLDECL void ( QGLAPI * qglSetFragmentShaderConstantATI )( GLuint dst, const GLfloat *value );
QGLDECL GLboolean ( QGLAPI * qglTestFenceNV )( GLuint fence );
QGLDECL void ( QGLAPI * qglUnlockArraysEXT )( void );
QGLDECL GLboolean ( QGLAPI * qglUnmapBufferARB )( GLenum target );
QGLDECL void ( QGLAPI * qglUpdateObjectBufferATI )( GLuint buffer, GLuint offset, GLsizei size, const GLvoid *pointer, GLenum preserve );
QGLDECL void ( QGLAPI * qglVariantArrayObjectATI )( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
QGLDECL void ( QGLAPI * qglVertexArrayRangeNV )( GLsizei length, const GLvoid *pointer );
QGLDECL void ( QGLAPI * qglVertexAttrib1dARB )( GLuint index, GLdouble x );
QGLDECL void ( QGLAPI * qglVertexAttrib1dvARB )( GLuint index, const GLdouble *v );
QGLDECL void ( QGLAPI * qglVertexAttrib1fARB )( GLuint index, GLfloat x );
QGLDECL void ( QGLAPI * qglVertexAttrib1fvARB )( GLuint index, const GLfloat *v );
QGLDECL void ( QGLAPI * qglVertexAttrib1sARB )( GLuint index, GLshort x );
QGLDECL void ( QGLAPI * qglVertexAttrib1svARB )( GLuint index, const GLshort *v );
QGLDECL void ( QGLAPI * qglVertexAttrib2dARB )( GLuint index, GLdouble x, GLdouble y );
QGLDECL void ( QGLAPI * qglVertexAttrib2dvARB )( GLuint index, const GLdouble *v );
QGLDECL void ( QGLAPI * qglVertexAttrib2fARB )( GLuint index, GLfloat x, GLfloat y );
QGLDECL void ( QGLAPI * qglVertexAttrib2fvARB )( GLuint index, const GLfloat *v );
QGLDECL void ( QGLAPI * qglVertexAttrib2sARB )( GLuint index, GLshort x, GLshort y );
QGLDECL void ( QGLAPI * qglVertexAttrib2svARB )( GLuint index, const GLshort *v );
QGLDECL void ( QGLAPI * qglVertexAttrib3dARB )( GLuint index, GLdouble x, GLdouble y, GLdouble z );
QGLDECL void ( QGLAPI * qglVertexAttrib3dvARB )( GLuint index, const GLdouble *v );
QGLDECL void ( QGLAPI * qglVertexAttrib3fARB )( GLuint index, GLfloat x, GLfloat y, GLfloat z );
QGLDECL void ( QGLAPI * qglVertexAttrib3fvARB )( GLuint index, const GLfloat *v );
QGLDECL void ( QGLAPI * qglVertexAttrib3sARB )( GLuint index, GLshort x, GLshort y, GLshort z );
QGLDECL void ( QGLAPI * qglVertexAttrib3svARB )( GLuint index, const GLshort *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4NbvARB )( GLuint index, const GLbyte *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4NivARB )( GLuint index, const GLint *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4NsvARB )( GLuint index, const GLshort *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4NubARB )( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
QGLDECL void ( QGLAPI * qglVertexAttrib4NubvARB )( GLuint index, const GLubyte *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4NuivARB )( GLuint index, const GLuint *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4NusvARB )( GLuint index, const GLushort *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4bvARB )( GLuint index, const GLbyte *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4dARB )( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
QGLDECL void ( QGLAPI * qglVertexAttrib4dvARB )( GLuint index, const GLdouble *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4fARB )( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
QGLDECL void ( QGLAPI * qglVertexAttrib4fvARB )( GLuint index, const GLfloat *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4ivARB )( GLuint index, const GLint *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4sARB )( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
QGLDECL void ( QGLAPI * qglVertexAttrib4svARB )( GLuint index, const GLshort *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4ubvARB )( GLuint index, const GLubyte *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4uivARB )( GLuint index, const GLuint *v );
QGLDECL void ( QGLAPI * qglVertexAttrib4usvARB )( GLuint index, const GLushort *v );
QGLDECL void ( QGLAPI * qglVertexAttribPointerARB )( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer );

/* Driver slots.  QCL_Init and GLW_InitExtensions store every GetProcAddress /
 * wglGetProcAddress result in both the dll slot and its qgl pointer; the gl.log
 * and error-check wrappers forward through the dll slot, and QGL_EnableLogging /
 * QGL_EnableErrorChecking restore the qgl pointers from it.  Only the win32 QGL
 * layer sees them (QGL_OWNER defines them, QGL_DRIVER_SLOTS declares them). */
#if defined( QGL_OWNER ) || defined( QGL_DRIVER_SLOTS )
QGLDECL void ( QGLAPI * dllAccum )( GLenum op, GLfloat value );                                                      /* 0x016C3C44 */
QGLDECL void ( QGLAPI * dllAlphaFunc )( GLenum func, GLclampf ref );                                                 /* 0x016C458C */
QGLDECL GLboolean ( QGLAPI * dllAreTexturesResident )( GLsizei n, const GLuint *textures, GLboolean *residences );   /* 0x016C4A84 */
QGLDECL void ( QGLAPI * dllArrayElement )( GLint i );                                                                /* 0x016C4874 */
QGLDECL void ( QGLAPI * dllBegin )( GLenum mode );                                                                   /* 0x016C48D8 */
QGLDECL void ( QGLAPI * dllBindTexture )( GLenum target, GLuint texture );                                           /* 0x016C3B58 */
QGLDECL void ( QGLAPI * dllBitmap )( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap ); /* 0x016C3B34 */
QGLDECL void ( QGLAPI * dllBlendFunc )( GLenum sfactor, GLenum dfactor );                                            /* 0x016C4704 */
QGLDECL void ( QGLAPI * dllCallList )( GLuint list );                                                                /* 0x016C44DC */
QGLDECL void ( QGLAPI * dllCallLists )( GLsizei n, GLenum type, const GLvoid *lists );                               /* 0x016C415C */
QGLDECL void ( QGLAPI * dllClear )( GLbitfield mask );                                                               /* 0x016C4348 */
QGLDECL void ( QGLAPI * dllClearAccum )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );                  /* 0x016C48E4 */
QGLDECL void ( QGLAPI * dllClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );              /* 0x016C41D8 */
QGLDECL void ( QGLAPI * dllClearDepth )( GLclampd depth );                                                           /* 0x016C4A1C */
QGLDECL void ( QGLAPI * dllClearIndex )( GLfloat c );                                                                /* 0x016C4628 */
QGLDECL void ( QGLAPI * dllClearStencil )( GLint s );                                                                /* 0x016C417C */
QGLDECL void ( QGLAPI * dllClipPlane )( GLenum plane, const GLdouble *equation );                                    /* 0x016C3BF4 */
QGLDECL void ( QGLAPI * dllColor3b )( GLbyte red, GLbyte green, GLbyte blue );                                       /* 0x016C3B2C */
QGLDECL void ( QGLAPI * dllColor3bv )( const GLbyte *v );                                                            /* 0x016C45A8 */
QGLDECL void ( QGLAPI * dllColor3d )( GLdouble red, GLdouble green, GLdouble blue );                                 /* 0x016C4474 */
QGLDECL void ( QGLAPI * dllColor3dv )( const GLdouble *v );                                                          /* 0x016C499C */
QGLDECL void ( QGLAPI * dllColor3f )( GLfloat red, GLfloat green, GLfloat blue );                                    /* 0x016C3BB0 */
QGLDECL void ( QGLAPI * dllColor3fv )( const GLfloat *v );                                                           /* 0x016C4A88 */
QGLDECL void ( QGLAPI * dllColor3i )( GLint red, GLint green, GLint blue );                                          /* 0x016C4900 */
QGLDECL void ( QGLAPI * dllColor3iv )( const GLint *v );                                                             /* 0x016C423C */
QGLDECL void ( QGLAPI * dllColor3s )( GLshort red, GLshort green, GLshort blue );                                    /* 0x016C4414 */
QGLDECL void ( QGLAPI * dllColor3sv )( const GLshort *v );                                                           /* 0x016C3B4C */
QGLDECL void ( QGLAPI * dllColor3ub )( GLubyte red, GLubyte green, GLubyte blue );                                   /* 0x016C4298 */
QGLDECL void ( QGLAPI * dllColor3ubv )( const GLubyte *v );                                                          /* 0x016C4840 */
QGLDECL void ( QGLAPI * dllColor3ui )( GLuint red, GLuint green, GLuint blue );                                      /* 0x016C3C8C */
QGLDECL void ( QGLAPI * dllColor3uiv )( const GLuint *v );                                                           /* 0x016C3DD0 */
QGLDECL void ( QGLAPI * dllColor3us )( GLushort red, GLushort green, GLushort blue );                                /* 0x016C40C4 */
QGLDECL void ( QGLAPI * dllColor3usv )( const GLushort *v );                                                         /* 0x016C46D0 */
QGLDECL void ( QGLAPI * dllColor4b )( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );                         /* 0x016C3BBC */
QGLDECL void ( QGLAPI * dllColor4bv )( const GLbyte *v );                                                            /* 0x016C4ABC */
QGLDECL void ( QGLAPI * dllColor4d )( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );                 /* 0x016C47F8 */
QGLDECL void ( QGLAPI * dllColor4dv )( const GLdouble *v );                                                          /* 0x016C400C */
QGLDECL void ( QGLAPI * dllColor4f )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );                     /* 0x016C4948 */
QGLDECL void ( QGLAPI * dllColor4fv )( const GLfloat *v );                                                           /* 0x016C3E90 */
QGLDECL void ( QGLAPI * dllColor4i )( GLint red, GLint green, GLint blue, GLint alpha );                             /* 0x016C3CC8 */
QGLDECL void ( QGLAPI * dllColor4iv )( const GLint *v );                                                             /* 0x016C4A0C */
QGLDECL void ( QGLAPI * dllColor4s )( GLshort red, GLshort green, GLshort blue, GLshort alpha );                     /* 0x016C44B0 */
QGLDECL void ( QGLAPI * dllColor4sv )( const GLshort *v );                                                           /* 0x016C4290 */
QGLDECL void ( QGLAPI * dllColor4ub )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );                    /* 0x016C4A54 */
QGLDECL void ( QGLAPI * dllColor4ubv )( const GLubyte *v );                                                          /* 0x016C42CC */
QGLDECL void ( QGLAPI * dllColor4ui )( GLuint red, GLuint green, GLuint blue, GLuint alpha );                        /* 0x016C4644 */
QGLDECL void ( QGLAPI * dllColor4uiv )( const GLuint *v );                                                           /* 0x016C3E80 */
QGLDECL void ( QGLAPI * dllColor4us )( GLushort red, GLushort green, GLushort blue, GLushort alpha );                /* 0x016C4A34 */
QGLDECL void ( QGLAPI * dllColor4usv )( const GLushort *v );                                                         /* 0x016C44F8 */
QGLDECL void ( QGLAPI * dllColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );           /* 0x016C4194 */
QGLDECL void ( QGLAPI * dllColorMaterial )( GLenum face, GLenum mode );                                              /* 0x016C4224 */
QGLDECL void ( QGLAPI * dllColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );         /* 0x016C48F4 */
QGLDECL void ( QGLAPI * dllCopyPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type );             /* 0x016C4314 */
QGLDECL void ( QGLAPI * dllCopyTexImage1D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border ); /* 0x016C4068 */
QGLDECL void ( QGLAPI * dllCopyTexImage2D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border ); /* 0x016C3DF4 */
QGLDECL void ( QGLAPI * dllCopyTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width ); /* 0x016C46B8 */
QGLDECL void ( QGLAPI * dllCopyTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height ); /* 0x016C48A8 */
QGLDECL void ( QGLAPI * dllCullFace )( GLenum mode );                                                                /* 0x016C4450 */
QGLDECL void ( QGLAPI * dllDeleteLists )( GLuint list, GLsizei range );                                              /* 0x016C4A00 */
QGLDECL void ( QGLAPI * dllDeleteTextures )( GLsizei n, const GLuint *textures );                                    /* 0x016C48A4 */
QGLDECL void ( QGLAPI * dllDepthFunc )( GLenum func );                                                               /* 0x016C42D8 */
QGLDECL void ( QGLAPI * dllDepthMask )( GLboolean flag );                                                            /* 0x016C3C30 */
QGLDECL void ( QGLAPI * dllDepthRange )( GLclampd zNear, GLclampd zFar );                                            /* 0x016C42FC */
QGLDECL void ( QGLAPI * dllDisable )( GLenum cap );                                                                  /* 0x016C43A0 */
QGLDECL void ( QGLAPI * dllDisableClientState )( GLenum array );                                                     /* 0x016C3E24 */
QGLDECL void ( QGLAPI * dllDrawArrays )( GLenum mode, GLint first, GLsizei count );                                  /* 0x016C43D4 */
QGLDECL void ( QGLAPI * dllDrawBuffer )( GLenum mode );                                                              /* 0x016C4448 */
QGLDECL void ( QGLAPI * dllDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );         /* 0x016C3FBC */
QGLDECL void ( QGLAPI * dllDrawPixels )( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ); /* 0x016C470C */
QGLDECL void ( QGLAPI * dllEdgeFlag )( GLboolean flag );                                                             /* 0x016C491C */
QGLDECL void ( QGLAPI * dllEdgeFlagPointer )( GLsizei stride, const GLvoid *pointer );                               /* 0x016C3CF8 */
QGLDECL void ( QGLAPI * dllEdgeFlagv )( const GLboolean *flag );                                                     /* 0x016C460C */
QGLDECL void ( QGLAPI * dllEnable )( GLenum cap );                                                                   /* 0x016C4494 */
QGLDECL void ( QGLAPI * dllEnableClientState )( GLenum array );                                                      /* 0x016C4718 */
QGLDECL void ( QGLAPI * dllEnd )( void );                                                                            /* 0x016C3BEC */
QGLDECL void ( QGLAPI * dllEndList )( void );                                                                        /* 0x016C3E08 */
QGLDECL void ( QGLAPI * dllEvalCoord1d )( GLdouble u );                                                              /* 0x016C40B0 */
QGLDECL void ( QGLAPI * dllEvalCoord1dv )( const GLdouble *u );                                                      /* 0x016C41C4 */
QGLDECL void ( QGLAPI * dllEvalCoord1f )( GLfloat u );                                                               /* 0x016C4AB4 */
QGLDECL void ( QGLAPI * dllEvalCoord1fv )( const GLfloat *u );                                                       /* 0x016C43E8 */
QGLDECL void ( QGLAPI * dllEvalCoord2d )( GLdouble u, GLdouble v );                                                  /* 0x016C45B8 */
QGLDECL void ( QGLAPI * dllEvalCoord2dv )( const GLdouble *u );                                                      /* 0x016C4060 */
QGLDECL void ( QGLAPI * dllEvalCoord2f )( GLfloat u, GLfloat v );                                                    /* 0x016C3FF8 */
QGLDECL void ( QGLAPI * dllEvalCoord2fv )( const GLfloat *u );                                                       /* 0x016C3E20 */
QGLDECL void ( QGLAPI * dllEvalMesh1 )( GLenum mode, GLint i1, GLint i2 );                                           /* 0x016C3E50 */
QGLDECL void ( QGLAPI * dllEvalMesh2 )( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );                       /* 0x016C47A4 */
QGLDECL void ( QGLAPI * dllEvalPoint1 )( GLint i );                                                                  /* 0x016C3F10 */
QGLDECL void ( QGLAPI * dllEvalPoint2 )( GLint i, GLint j );                                                         /* 0x016C3DA0 */
QGLDECL void ( QGLAPI * dllFeedbackBuffer )( GLsizei size, GLenum type, GLfloat *buffer );                           /* 0x016C3C28 */
QGLDECL void ( QGLAPI * dllFinish )( void );                                                                         /* 0x016C3F48 */
QGLDECL void ( QGLAPI * dllFlush )( void );                                                                          /* 0x016C3DE4 */
QGLDECL void ( QGLAPI * dllFogf )( GLenum pname, GLfloat param );                                                    /* 0x016C49C8 */
QGLDECL void ( QGLAPI * dllFogfv )( GLenum pname, const GLfloat *params );                                           /* 0x016C4AA4 */
QGLDECL void ( QGLAPI * dllFogi )( GLenum pname, GLint param );                                                      /* 0x016C3E18 */
QGLDECL void ( QGLAPI * dllFogiv )( GLenum pname, const GLint *params );                                             /* 0x016C4908 */
QGLDECL void ( QGLAPI * dllFrontFace )( GLenum mode );                                                               /* 0x016C4018 */
QGLDECL void ( QGLAPI * dllFrustum )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ); /* 0x016C4638 */
QGLDECL GLuint ( QGLAPI * dllGenLists )( GLsizei range );                                                            /* 0x016C4764 */
QGLDECL void ( QGLAPI * dllGenTextures )( GLsizei n, GLuint *textures );                                             /* 0x016C3D50 */
QGLDECL void ( QGLAPI * dllGetBooleanv )( GLenum pname, GLboolean *params );                                         /* 0x016C4988 */
QGLDECL void ( QGLAPI * dllGetClipPlane )( GLenum plane, GLdouble *equation );                                       /* 0x016C48B4 */
QGLDECL void ( QGLAPI * dllGetDoublev )( GLenum pname, GLdouble *params );                                           /* 0x016C3EB8 */
QGLDECL GLenum ( QGLAPI * dllGetError )( void );                                                                     /* 0x016C44C8 */
QGLDECL void ( QGLAPI * dllGetFloatv )( GLenum pname, GLfloat *params );                                             /* 0x016C4860 */
QGLDECL void ( QGLAPI * dllGetIntegerv )( GLenum pname, GLint *params );                                             /* 0x016C3B88 */
QGLDECL void ( QGLAPI * dllGetLightfv )( GLenum light, GLenum pname, GLfloat *params );                              /* 0x016C4A8C */
QGLDECL void ( QGLAPI * dllGetLightiv )( GLenum light, GLenum pname, GLint *params );                                /* 0x016C4538 */
QGLDECL void ( QGLAPI * dllGetMapdv )( GLenum target, GLenum query, GLdouble *v );                                   /* 0x016C3C9C */
QGLDECL void ( QGLAPI * dllGetMapfv )( GLenum target, GLenum query, GLfloat *v );                                    /* 0x016C4388 */
QGLDECL void ( QGLAPI * dllGetMapiv )( GLenum target, GLenum query, GLint *v );                                      /* 0x016C3CA8 */
QGLDECL void ( QGLAPI * dllGetMaterialfv )( GLenum face, GLenum pname, GLfloat *params );                            /* 0x016C42A4 */
QGLDECL void ( QGLAPI * dllGetMaterialiv )( GLenum face, GLenum pname, GLint *params );                              /* 0x016C47F4 */
QGLDECL void ( QGLAPI * dllGetPixelMapfv )( GLenum map, GLfloat *values );                                           /* 0x016C3E78 */
QGLDECL void ( QGLAPI * dllGetPixelMapuiv )( GLenum map, GLuint *values );                                           /* 0x016C45F8 */
QGLDECL void ( QGLAPI * dllGetPixelMapusv )( GLenum map, GLushort *values );                                         /* 0x016C3B44 */
QGLDECL void ( QGLAPI * dllGetPointerv )( GLenum pname, GLvoid* *params );                                           /* 0x016C3B94 */
QGLDECL void ( QGLAPI * dllGetPolygonStipple )( GLubyte *mask );                                                     /* 0x016C3FD0 */
QGLDECL const GLubyte * ( QGLAPI * dllGetString )( GLenum name );                                                    /* 0x016C3D88 */
QGLDECL void ( QGLAPI * dllGetTexEnvfv )( GLenum target, GLenum pname, GLfloat *params );                            /* 0x016C4514 */
QGLDECL void ( QGLAPI * dllGetTexEnviv )( GLenum target, GLenum pname, GLint *params );                              /* 0x016C487C */
QGLDECL void ( QGLAPI * dllGetTexGendv )( GLenum coord, GLenum pname, GLdouble *params );                            /* 0x016C3E84 */
QGLDECL void ( QGLAPI * dllGetTexGenfv )( GLenum coord, GLenum pname, GLfloat *params );                             /* 0x016C449C */
QGLDECL void ( QGLAPI * dllGetTexGeniv )( GLenum coord, GLenum pname, GLint *params );                               /* 0x016C493C */
QGLDECL void ( QGLAPI * dllGetTexImage )( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels );  /* 0x016C4584 */
QGLDECL void ( QGLAPI * dllGetTexLevelParameterfv )( GLenum target, GLint level, GLenum pname, GLfloat *params );    /* 0x016C4990 */
QGLDECL void ( QGLAPI * dllGetTexLevelParameteriv )( GLenum target, GLint level, GLenum pname, GLint *params );      /* 0x016C4154 */
QGLDECL void ( QGLAPI * dllGetTexParameterfv )( GLenum target, GLenum pname, GLfloat *params );                      /* 0x016C42B4 */
QGLDECL void ( QGLAPI * dllGetTexParameteriv )( GLenum target, GLenum pname, GLint *params );                        /* 0x016C4960 */
QGLDECL void ( QGLAPI * dllHint )( GLenum target, GLenum mode );                                                     /* 0x016C4A24 */
QGLDECL void ( QGLAPI * dllIndexMask )( GLuint mask );                                                               /* 0x016C49FC */
QGLDECL void ( QGLAPI * dllIndexPointer )( GLenum type, GLsizei stride, const GLvoid *pointer );                     /* 0x016C47F0 */
QGLDECL void ( QGLAPI * dllIndexd )( GLdouble c );                                                                   /* 0x016C3BD8 */
QGLDECL void ( QGLAPI * dllIndexdv )( const GLdouble *c );                                                           /* 0x016C4268 */
QGLDECL void ( QGLAPI * dllIndexf )( GLfloat c );                                                                    /* 0x016C4310 */
QGLDECL void ( QGLAPI * dllIndexfv )( const GLfloat *c );                                                            /* 0x016C3BC0 */
QGLDECL void ( QGLAPI * dllIndexi )( GLint c );                                                                      /* 0x016C3FB0 */
QGLDECL void ( QGLAPI * dllIndexiv )( const GLint *c );                                                              /* 0x016C42B0 */
QGLDECL void ( QGLAPI * dllIndexs )( GLshort c );                                                                    /* 0x016C3D68 */
QGLDECL void ( QGLAPI * dllIndexsv )( const GLshort *c );                                                            /* 0x016C3EB4 */
QGLDECL void ( QGLAPI * dllIndexub )( GLubyte c );                                                                   /* 0x016C4120 */
QGLDECL void ( QGLAPI * dllIndexubv )( const GLubyte *c );                                                           /* 0x016C40AC */
QGLDECL void ( QGLAPI * dllInitNames )( void );                                                                      /* 0x016C3E54 */
QGLDECL void ( QGLAPI * dllInterleavedArrays )( GLenum format, GLsizei stride, const GLvoid *pointer );              /* 0x016C49AC */
QGLDECL GLboolean ( QGLAPI * dllIsEnabled )( GLenum cap );                                                           /* 0x016C49F8 */
QGLDECL GLboolean ( QGLAPI * dllIsList )( GLuint list );                                                             /* 0x016C41D4 */
QGLDECL GLboolean ( QGLAPI * dllIsTexture )( GLuint texture );                                                       /* 0x016C4850 */
QGLDECL void ( QGLAPI * dllLightModelf )( GLenum pname, GLfloat param );                                             /* 0x016C3DA4 */
QGLDECL void ( QGLAPI * dllLightModelfv )( GLenum pname, const GLfloat *params );                                    /* 0x016C3CEC */
QGLDECL void ( QGLAPI * dllLightModeli )( GLenum pname, GLint param );                                               /* 0x016C3EE0 */
QGLDECL void ( QGLAPI * dllLightModeliv )( GLenum pname, const GLint *params );                                      /* 0x016C3CF0 */
QGLDECL void ( QGLAPI * dllLightf )( GLenum light, GLenum pname, GLfloat param );                                    /* 0x016C4AC8 */
QGLDECL void ( QGLAPI * dllLightfv )( GLenum light, GLenum pname, const GLfloat *params );                           /* 0x016C40BC */
QGLDECL void ( QGLAPI * dllLighti )( GLenum light, GLenum pname, GLint param );                                      /* 0x016C437C */
QGLDECL void ( QGLAPI * dllLightiv )( GLenum light, GLenum pname, const GLint *params );                             /* 0x016C3CD4 */
QGLDECL void ( QGLAPI * dllLineStipple )( GLint factor, GLushort pattern );                                          /* 0x016C4098 */
QGLDECL void ( QGLAPI * dllLineWidth )( GLfloat width );                                                             /* 0x016C3C50 */
QGLDECL void ( QGLAPI * dllListBase )( GLuint base );                                                                /* 0x016C3E2C */
QGLDECL void ( QGLAPI * dllLoadIdentity )( void );                                                                   /* 0x016C48D4 */
QGLDECL void ( QGLAPI * dllLoadMatrixd )( const GLdouble *m );                                                       /* 0x016C45DC */
QGLDECL void ( QGLAPI * dllLoadMatrixf )( const GLfloat *m );                                                        /* 0x016C3C24 */
QGLDECL void ( QGLAPI * dllLoadName )( GLuint name );                                                                /* 0x016C4404 */
QGLDECL void ( QGLAPI * dllLogicOp )( GLenum opcode );                                                               /* 0x016C3C0C */
QGLDECL void ( QGLAPI * dllMap1d )( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points ); /* 0x016C4944 */
QGLDECL void ( QGLAPI * dllMap1f )( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points ); /* 0x016C42C4 */
QGLDECL void ( QGLAPI * dllMap2d )( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points ); /* 0x016C4A80 */
QGLDECL void ( QGLAPI * dllMap2f )( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points ); /* 0x016C44D8 */
QGLDECL void ( QGLAPI * dllMapGrid1d )( GLint un, GLdouble u1, GLdouble u2 );                                        /* 0x016C48E0 */
QGLDECL void ( QGLAPI * dllMapGrid1f )( GLint un, GLfloat u1, GLfloat u2 );                                          /* 0x016C4180 */
QGLDECL void ( QGLAPI * dllMapGrid2d )( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 );    /* 0x016C3F24 */
QGLDECL void ( QGLAPI * dllMapGrid2f )( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 );        /* 0x016C4110 */
QGLDECL void ( QGLAPI * dllMaterialf )( GLenum face, GLenum pname, GLfloat param );                                  /* 0x016C3DE0 */
QGLDECL void ( QGLAPI * dllMaterialfv )( GLenum face, GLenum pname, const GLfloat *params );                         /* 0x016C42A0 */
QGLDECL void ( QGLAPI * dllMateriali )( GLenum face, GLenum pname, GLint param );                                    /* 0x016C3EEC */
QGLDECL void ( QGLAPI * dllMaterialiv )( GLenum face, GLenum pname, const GLint *params );                           /* 0x016C3B28 */
QGLDECL void ( QGLAPI * dllMatrixMode )( GLenum mode );                                                              /* 0x016C483C */
QGLDECL void ( QGLAPI * dllMultMatrixd )( const GLdouble *m );                                                       /* 0x016C4534 */
QGLDECL void ( QGLAPI * dllMultMatrixf )( const GLfloat *m );                                                        /* 0x016C3F44 */
QGLDECL void ( QGLAPI * dllNewList )( GLuint list, GLenum mode );                                                    /* 0x016C3D08 */
QGLDECL void ( QGLAPI * dllNormal3b )( GLbyte nx, GLbyte ny, GLbyte nz );                                            /* 0x016C4714 */
QGLDECL void ( QGLAPI * dllNormal3bv )( const GLbyte *v );                                                           /* 0x016C3B20 */
QGLDECL void ( QGLAPI * dllNormal3d )( GLdouble nx, GLdouble ny, GLdouble nz );                                      /* 0x016C454C */
QGLDECL void ( QGLAPI * dllNormal3dv )( const GLdouble *v );                                                         /* 0x016C44B8 */
QGLDECL void ( QGLAPI * dllNormal3f )( GLfloat nx, GLfloat ny, GLfloat nz );                                         /* 0x016C3D7C */
QGLDECL void ( QGLAPI * dllNormal3fv )( const GLfloat *v );                                                          /* 0x016C4278 */
QGLDECL void ( QGLAPI * dllNormal3i )( GLint nx, GLint ny, GLint nz );                                               /* 0x016C41E8 */
QGLDECL void ( QGLAPI * dllNormal3iv )( const GLint *v );                                                            /* 0x016C4940 */
QGLDECL void ( QGLAPI * dllNormal3s )( GLshort nx, GLshort ny, GLshort nz );                                         /* 0x016C414C */
QGLDECL void ( QGLAPI * dllNormal3sv )( const GLshort *v );                                                          /* 0x016C461C */
QGLDECL void ( QGLAPI * dllNormalPointer )( GLenum type, GLsizei stride, const GLvoid *pointer );                    /* 0x016C3D84 */
QGLDECL void ( QGLAPI * dllOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ); /* 0x016C49E0 */
QGLDECL void ( QGLAPI * dllPassThrough )( GLfloat token );                                                           /* 0x016C407C */
QGLDECL void ( QGLAPI * dllPixelMapfv )( GLenum map, GLsizei mapsize, const GLfloat *values );                       /* 0x016C3BB8 */
QGLDECL void ( QGLAPI * dllPixelMapuiv )( GLenum map, GLsizei mapsize, const GLuint *values );                       /* 0x016C4880 */
QGLDECL void ( QGLAPI * dllPixelMapusv )( GLenum map, GLsizei mapsize, const GLushort *values );                     /* 0x016C41F8 */
QGLDECL void ( QGLAPI * dllPixelStoref )( GLenum pname, GLfloat param );                                             /* 0x016C47D4 */
QGLDECL void ( QGLAPI * dllPixelStorei )( GLenum pname, GLint param );                                               /* 0x016C4464 */
QGLDECL void ( QGLAPI * dllPixelTransferf )( GLenum pname, GLfloat param );                                          /* 0x016C43EC */
QGLDECL void ( QGLAPI * dllPixelTransferi )( GLenum pname, GLint param );                                            /* 0x016C4818 */
QGLDECL void ( QGLAPI * dllPixelZoom )( GLfloat xfactor, GLfloat yfactor );                                          /* 0x016C3ECC */
QGLDECL void ( QGLAPI * dllPointSize )( GLfloat size );                                                              /* 0x016C3FD8 */
QGLDECL void ( QGLAPI * dllPolygonMode )( GLenum face, GLenum mode );                                                /* 0x016C3B98 */
QGLDECL void ( QGLAPI * dllPolygonOffset )( GLfloat factor, GLfloat units );                                         /* 0x016C3E74 */
QGLDECL void ( QGLAPI * dllPolygonStipple )( const GLubyte *mask );                                                  /* 0x016C49D0 */
QGLDECL void ( QGLAPI * dllPopAttrib )( void );                                                                      /* 0x016C4690 */
QGLDECL void ( QGLAPI * dllPopClientAttrib )( void );                                                                /* 0x016C3C20 */
QGLDECL void ( QGLAPI * dllPopMatrix )( void );                                                                      /* 0x016C472C */
QGLDECL void ( QGLAPI * dllPopName )( void );                                                                        /* 0x016C4894 */
QGLDECL void ( QGLAPI * dllPrioritizeTextures )( GLsizei n, const GLuint *textures, const GLclampf *priorities );    /* 0x016C480C */
QGLDECL void ( QGLAPI * dllPushAttrib )( GLbitfield mask );                                                          /* 0x016C41E0 */
QGLDECL void ( QGLAPI * dllPushClientAttrib )( GLbitfield mask );                                                    /* 0x016C4608 */
QGLDECL void ( QGLAPI * dllPushMatrix )( void );                                                                     /* 0x016C4AF0 */
QGLDECL void ( QGLAPI * dllPushName )( GLuint name );                                                                /* 0x016C4914 */
QGLDECL void ( QGLAPI * dllRasterPos2d )( GLdouble x, GLdouble y );                                                  /* 0x016C431C */
QGLDECL void ( QGLAPI * dllRasterPos2dv )( const GLdouble *v );                                                      /* 0x016C4800 */
QGLDECL void ( QGLAPI * dllRasterPos2f )( GLfloat x, GLfloat y );                                                    /* 0x016C3BE8 */
QGLDECL void ( QGLAPI * dllRasterPos2fv )( const GLfloat *v );                                                       /* 0x016C3EA8 */
QGLDECL void ( QGLAPI * dllRasterPos2i )( GLint x, GLint y );                                                        /* 0x016C47BC */
QGLDECL void ( QGLAPI * dllRasterPos2iv )( const GLint *v );                                                         /* 0x016C413C */
QGLDECL void ( QGLAPI * dllRasterPos2s )( GLshort x, GLshort y );                                                    /* 0x016C3E5C */
QGLDECL void ( QGLAPI * dllRasterPos2sv )( const GLshort *v );                                                       /* 0x016C3F50 */
QGLDECL void ( QGLAPI * dllRasterPos3d )( GLdouble x, GLdouble y, GLdouble z );                                      /* 0x016C3B48 */
QGLDECL void ( QGLAPI * dllRasterPos3dv )( const GLdouble *v );                                                      /* 0x016C3C80 */
QGLDECL void ( QGLAPI * dllRasterPos3f )( GLfloat x, GLfloat y, GLfloat z );                                         /* 0x016C4A60 */
QGLDECL void ( QGLAPI * dllRasterPos3fv )( const GLfloat *v );                                                       /* 0x016C46CC */
QGLDECL void ( QGLAPI * dllRasterPos3i )( GLint x, GLint y, GLint z );                                               /* 0x016C4258 */
QGLDECL void ( QGLAPI * dllRasterPos3iv )( const GLint *v );                                                         /* 0x016C46B4 */
QGLDECL void ( QGLAPI * dllRasterPos3s )( GLshort x, GLshort y, GLshort z );                                         /* 0x016C46C4 */
QGLDECL void ( QGLAPI * dllRasterPos3sv )( const GLshort *v );                                                       /* 0x016C4A20 */
QGLDECL void ( QGLAPI * dllRasterPos4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w );                          /* 0x016C3EF0 */
QGLDECL void ( QGLAPI * dllRasterPos4dv )( const GLdouble *v );                                                      /* 0x016C4484 */
QGLDECL void ( QGLAPI * dllRasterPos4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w );                              /* 0x016C49BC */
QGLDECL void ( QGLAPI * dllRasterPos4fv )( const GLfloat *v );                                                       /* 0x016C3F64 */
QGLDECL void ( QGLAPI * dllRasterPos4i )( GLint x, GLint y, GLint z, GLint w );                                      /* 0x016C3F18 */
QGLDECL void ( QGLAPI * dllRasterPos4iv )( const GLint *v );                                                         /* 0x016C49A0 */
QGLDECL void ( QGLAPI * dllRasterPos4s )( GLshort x, GLshort y, GLshort z, GLshort w );                              /* 0x016C3B80 */
QGLDECL void ( QGLAPI * dllRasterPos4sv )( const GLshort *v );                                                       /* 0x016C4378 */
QGLDECL void ( QGLAPI * dllReadBuffer )( GLenum mode );                                                              /* 0x016C4550 */
QGLDECL void ( QGLAPI * dllReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ); /* 0x016C3BCC */
QGLDECL void ( QGLAPI * dllRectd )( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );                            /* 0x016C4A30 */
QGLDECL void ( QGLAPI * dllRectdv )( const GLdouble *v1, const GLdouble *v2 );                                       /* 0x016C3C60 */
QGLDECL void ( QGLAPI * dllRectf )( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );                                /* 0x016C49B4 */
QGLDECL void ( QGLAPI * dllRectfv )( const GLfloat *v1, const GLfloat *v2 );                                         /* 0x016C3E64 */
QGLDECL void ( QGLAPI * dllRecti )( GLint x1, GLint y1, GLint x2, GLint y2 );                                        /* 0x016C4814 */
QGLDECL void ( QGLAPI * dllRectiv )( const GLint *v1, const GLint *v2 );                                             /* 0x016C4A3C */
QGLDECL void ( QGLAPI * dllRects )( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );                                /* 0x016C4284 */
QGLDECL void ( QGLAPI * dllRectsv )( const GLshort *v1, const GLshort *v2 );                                         /* 0x016C4AEC */
QGLDECL GLint ( QGLAPI * dllRenderMode )( GLenum mode );                                                             /* 0x016C43B8 */
QGLDECL void ( QGLAPI * dllRotated )( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );                          /* 0x016C442C */
QGLDECL void ( QGLAPI * dllRotatef )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );                              /* 0x016C4784 */
QGLDECL void ( QGLAPI * dllScaled )( GLdouble x, GLdouble y, GLdouble z );                                           /* 0x016C4830 */
QGLDECL void ( QGLAPI * dllScalef )( GLfloat x, GLfloat y, GLfloat z );                                              /* 0x016C4148 */
QGLDECL void ( QGLAPI * dllScissor )( GLint x, GLint y, GLsizei width, GLsizei height );                             /* 0x016C45E0 */
QGLDECL void ( QGLAPI * dllSelectBuffer )( GLsizei size, GLuint *buffer );                                           /* 0x016C4458 */
QGLDECL void ( QGLAPI * dllShadeModel )( GLenum mode );                                                              /* 0x016C43B4 */
QGLDECL void ( QGLAPI * dllStencilFunc )( GLenum func, GLint ref, GLuint mask );                                     /* 0x016C46A8 */
QGLDECL void ( QGLAPI * dllStencilMask )( GLuint mask );                                                             /* 0x016C3D18 */
QGLDECL void ( QGLAPI * dllStencilOp )( GLenum fail, GLenum zfail, GLenum zpass );                                   /* 0x016C4328 */
QGLDECL void ( QGLAPI * dllTexCoord1d )( GLdouble s );                                                               /* 0x016C4190 */
QGLDECL void ( QGLAPI * dllTexCoord1dv )( const GLdouble *v );                                                       /* 0x016C3D20 */
QGLDECL void ( QGLAPI * dllTexCoord1f )( GLfloat s );                                                                /* 0x016C4720 */
QGLDECL void ( QGLAPI * dllTexCoord1fv )( const GLfloat *v );                                                        /* 0x016C3BA8 */
QGLDECL void ( QGLAPI * dllTexCoord1i )( GLint s );                                                                  /* 0x016C3C10 */
QGLDECL void ( QGLAPI * dllTexCoord1iv )( const GLint *v );                                                          /* 0x016C4AB8 */
QGLDECL void ( QGLAPI * dllTexCoord1s )( GLshort s );                                                                /* 0x016C3F98 */
QGLDECL void ( QGLAPI * dllTexCoord1sv )( const GLshort *v );                                                        /* 0x016C46F8 */
QGLDECL void ( QGLAPI * dllTexCoord2d )( GLdouble s, GLdouble t );                                                   /* 0x016C4970 */
QGLDECL void ( QGLAPI * dllTexCoord2dv )( const GLdouble *v );                                                       /* 0x016C49D8 */
QGLDECL void ( QGLAPI * dllTexCoord2f )( GLfloat s, GLfloat t );                                                     /* 0x016C3F5C */
QGLDECL void ( QGLAPI * dllTexCoord2fv )( const GLfloat *v );                                                        /* 0x016C4460 */
QGLDECL void ( QGLAPI * dllTexCoord2i )( GLint s, GLint t );                                                         /* 0x016C4504 */
QGLDECL void ( QGLAPI * dllTexCoord2iv )( const GLint *v );                                                          /* 0x016C4684 */
QGLDECL void ( QGLAPI * dllTexCoord2s )( GLshort s, GLshort t );                                                     /* 0x016C4768 */
QGLDECL void ( QGLAPI * dllTexCoord2sv )( const GLshort *v );                                                        /* 0x016C3D8C */
QGLDECL void ( QGLAPI * dllTexCoord3d )( GLdouble s, GLdouble t, GLdouble r );                                       /* 0x016C40A0 */
QGLDECL void ( QGLAPI * dllTexCoord3dv )( const GLdouble *v );                                                       /* 0x016C3E14 */
QGLDECL void ( QGLAPI * dllTexCoord3f )( GLfloat s, GLfloat t, GLfloat r );                                          /* 0x016C4420 */
QGLDECL void ( QGLAPI * dllTexCoord3fv )( const GLfloat *v );                                                        /* 0x016C40B8 */
QGLDECL void ( QGLAPI * dllTexCoord3i )( GLint s, GLint t, GLint r );                                                /* 0x016C44EC */
QGLDECL void ( QGLAPI * dllTexCoord3iv )( const GLint *v );                                                          /* 0x016C4590 */
QGLDECL void ( QGLAPI * dllTexCoord3s )( GLshort s, GLshort t, GLshort r );                                          /* 0x016C4650 */
QGLDECL void ( QGLAPI * dllTexCoord3sv )( const GLshort *v );                                                        /* 0x016C3B68 */
QGLDECL void ( QGLAPI * dllTexCoord4d )( GLdouble s, GLdouble t, GLdouble r, GLdouble q );                           /* 0x016C3CDC */
QGLDECL void ( QGLAPI * dllTexCoord4dv )( const GLdouble *v );                                                       /* 0x016C3FC8 */
QGLDECL void ( QGLAPI * dllTexCoord4f )( GLfloat s, GLfloat t, GLfloat r, GLfloat q );                               /* 0x016C453C */
QGLDECL void ( QGLAPI * dllTexCoord4fv )( const GLfloat *v );                                                        /* 0x016C42E4 */
QGLDECL void ( QGLAPI * dllTexCoord4i )( GLint s, GLint t, GLint r, GLint q );                                       /* 0x016C4A9C */
QGLDECL void ( QGLAPI * dllTexCoord4iv )( const GLint *v );                                                          /* 0x016C47EC */
QGLDECL void ( QGLAPI * dllTexCoord4s )( GLshort s, GLshort t, GLshort r, GLshort q );                               /* 0x016C4950 */
QGLDECL void ( QGLAPI * dllTexCoord4sv )( const GLshort *v );                                                        /* 0x016C4084 */
QGLDECL void ( QGLAPI * dllTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );      /* 0x016C42C0 */
QGLDECL void ( QGLAPI * dllTexEnvf )( GLenum target, GLenum pname, GLfloat param );                                  /* 0x016C4624 */
QGLDECL void ( QGLAPI * dllTexEnvfv )( GLenum target, GLenum pname, const GLfloat *params );                         /* 0x016C3EE8 */
QGLDECL void ( QGLAPI * dllTexEnvi )( GLenum target, GLenum pname, GLint param );                                    /* 0x016C430C */
QGLDECL void ( QGLAPI * dllTexEnviv )( GLenum target, GLenum pname, const GLint *params );                           /* 0x016C40DC */
QGLDECL void ( QGLAPI * dllTexGend )( GLenum coord, GLenum pname, GLdouble param );                                  /* 0x016C43C8 */
QGLDECL void ( QGLAPI * dllTexGendv )( GLenum coord, GLenum pname, const GLdouble *params );                         /* 0x016C48FC */
QGLDECL void ( QGLAPI * dllTexGenf )( GLenum coord, GLenum pname, GLfloat param );                                   /* 0x016C41B4 */
QGLDECL void ( QGLAPI * dllTexGenfv )( GLenum coord, GLenum pname, const GLfloat *params );                          /* 0x016C3E38 */
QGLDECL void ( QGLAPI * dllTexGeni )( GLenum coord, GLenum pname, GLint param );                                     /* 0x016C3E48 */
QGLDECL void ( QGLAPI * dllTexGeniv )( GLenum coord, GLenum pname, const GLint *params );                            /* 0x016C4518 */
QGLDECL void ( QGLAPI * dllTexImage1D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels ); /* 0x016C43C0 */
QGLDECL void ( QGLAPI * dllTexImage2D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels ); /* 0x016C464C */
QGLDECL void ( QGLAPI * dllTexParameterf )( GLenum target, GLenum pname, GLfloat param );                            /* 0x016C4678 */
QGLDECL void ( QGLAPI * dllTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params );                   /* 0x016C418C */
QGLDECL void ( QGLAPI * dllTexParameteri )( GLenum target, GLenum pname, GLint param );                              /* 0x016C45D0 */
QGLDECL void ( QGLAPI * dllTexParameteriv )( GLenum target, GLenum pname, const GLint *params );                     /* 0x016C3C14 */
QGLDECL void ( QGLAPI * dllTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels ); /* 0x016C4870 */
QGLDECL void ( QGLAPI * dllTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels ); /* 0x016C4318 */
QGLDECL void ( QGLAPI * dllTranslated )( GLdouble x, GLdouble y, GLdouble z );                                       /* 0x016C4668 */
QGLDECL void ( QGLAPI * dllTranslatef )( GLfloat x, GLfloat y, GLfloat z );                                          /* 0x016C41B0 */
QGLDECL void ( QGLAPI * dllVertex2d )( GLdouble x, GLdouble y );                                                     /* 0x016C47FC */
QGLDECL void ( QGLAPI * dllVertex2dv )( const GLdouble *v );                                                         /* 0x016C48D0 */
QGLDECL void ( QGLAPI * dllVertex2f )( GLfloat x, GLfloat y );                                                       /* 0x016C4AA0 */
QGLDECL void ( QGLAPI * dllVertex2fv )( const GLfloat *v );                                                          /* 0x016C4244 */
QGLDECL void ( QGLAPI * dllVertex2i )( GLint x, GLint y );                                                           /* 0x016C4524 */
QGLDECL void ( QGLAPI * dllVertex2iv )( const GLint *v );                                                            /* 0x016C496C */
QGLDECL void ( QGLAPI * dllVertex2s )( GLshort x, GLshort y );                                                       /* 0x016C44B4 */
QGLDECL void ( QGLAPI * dllVertex2sv )( const GLshort *v );                                                          /* 0x016C4454 */
QGLDECL void ( QGLAPI * dllVertex3d )( GLdouble x, GLdouble y, GLdouble z );                                         /* 0x016C3FEC */
QGLDECL void ( QGLAPI * dllVertex3dv )( const GLdouble *v );                                                         /* 0x016C3D44 */
QGLDECL void ( QGLAPI * dllVertex3f )( GLfloat x, GLfloat y, GLfloat z );                                            /* 0x016C4964 */
QGLDECL void ( QGLAPI * dllVertex3fv )( const GLfloat *v );                                                          /* 0x016C3DAC */
QGLDECL void ( QGLAPI * dllVertex3i )( GLint x, GLint y, GLint z );                                                  /* 0x016C3F60 */
QGLDECL void ( QGLAPI * dllVertex3iv )( const GLint *v );                                                            /* 0x016C4604 */
QGLDECL void ( QGLAPI * dllVertex3s )( GLshort x, GLshort y, GLshort z );                                            /* 0x016C3D2C */
QGLDECL void ( QGLAPI * dllVertex3sv )( const GLshort *v );                                                          /* 0x016C4574 */
QGLDECL void ( QGLAPI * dllVertex4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w );                             /* 0x016C41F4 */
QGLDECL void ( QGLAPI * dllVertex4dv )( const GLdouble *v );                                                         /* 0x016C4938 */
QGLDECL void ( QGLAPI * dllVertex4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w );                                 /* 0x016C3B7C */
QGLDECL void ( QGLAPI * dllVertex4fv )( const GLfloat *v );                                                          /* 0x016C4968 */
QGLDECL void ( QGLAPI * dllVertex4i )( GLint x, GLint y, GLint z, GLint w );                                         /* 0x016C4400 */
QGLDECL void ( QGLAPI * dllVertex4iv )( const GLint *v );                                                            /* 0x016C3BFC */
QGLDECL void ( QGLAPI * dllVertex4s )( GLshort x, GLshort y, GLshort z, GLshort w );                                 /* 0x016C4040 */
QGLDECL void ( QGLAPI * dllVertex4sv )( const GLshort *v );                                                          /* 0x016C41AC */
QGLDECL void ( QGLAPI * dllVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer );        /* 0x016C4168 */
QGLDECL void ( QGLAPI * dllViewport )( GLint x, GLint y, GLsizei width, GLsizei height );                            /* 0x016C3B54 */
QGLDECL void ( QGLAPI * dllActiveTextureARB )( GLenum texture );                                                     /* 0x016C49A8 */
QGLDECL void ( QGLAPI * dllAlphaFragmentOp1ATI )( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ); /* 0x016C4360 */
QGLDECL void ( QGLAPI * dllAlphaFragmentOp2ATI )( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ); /* 0x016C44E8 */
QGLDECL void ( QGLAPI * dllAlphaFragmentOp3ATI )( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ); /* 0x016C42F8 */
QGLDECL void ( QGLAPI * dllArrayObjectATI )( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ); /* 0x016C443C */
QGLDECL void ( QGLAPI * dllBeginFragmentShaderATI )( void );                                                         /* 0x016C3F84 */
QGLDECL void ( QGLAPI * dllBindBufferARB )( GLenum target, GLuint buffer );                                          /* 0x016C4910 */
QGLDECL void ( QGLAPI * dllBindFragmentShaderATI )( GLuint id );                                                     /* 0x016C40A8 */
QGLDECL void ( QGLAPI * dllBindProgramARB )( GLenum target, GLuint program );                                        /* 0x016C3E28 */
QGLDECL void ( QGLAPI * dllBufferDataARB )( GLenum target, GLsizei size, const GLvoid *data, GLenum usage );         /* 0x016C3EBC */
QGLDECL void ( QGLAPI * dllBufferSubDataARB )( GLenum target, GLint offset, GLsizei size, const GLvoid *data );      /* 0x016C3FA0 */
QGLDECL void ( QGLAPI * dllClientActiveTextureARB )( GLenum texture );                                               /* 0x016C432C */
QGLDECL void ( QGLAPI * dllColorFragmentOp1ATI )( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod ); /* 0x016C420C */
QGLDECL void ( QGLAPI * dllColorFragmentOp2ATI )( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod ); /* 0x016C49EC */
QGLDECL void ( QGLAPI * dllColorFragmentOp3ATI )( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod ); /* 0x016C403C */
QGLDECL void ( QGLAPI * dllCombinerInputNV )( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ); /* 0x016C3F74 */
QGLDECL void ( QGLAPI * dllCombinerOutputNV )( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ); /* 0x016C3D54 */
QGLDECL void ( QGLAPI * dllCombinerParameterfNV )( GLenum pname, GLfloat param );                                    /* 0x016C4188 */
QGLDECL void ( QGLAPI * dllCombinerParameterfvNV )( GLenum pname, const GLfloat *params );                           /* 0x016C3F88 */
QGLDECL void ( QGLAPI * dllCombinerParameteriNV )( GLenum pname, GLint param );                                      /* 0x016C3E3C */
QGLDECL void ( QGLAPI * dllCombinerParameterivNV )( GLenum pname, const GLint *params );                             /* 0x016C4558 */
QGLDECL void ( QGLAPI * dllCombinerStageParameterfvNV )( GLenum stage, GLenum pname, const GLfloat *params );        /* 0x016C4150 */
QGLDECL void ( QGLAPI * dllCompressedTexImage1DARB )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data ); /* 0x016C3CD0 */
QGLDECL void ( QGLAPI * dllCompressedTexImage2DARB )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data ); /* 0x016C495C */
QGLDECL void ( QGLAPI * dllCompressedTexImage3DARB )( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data ); /* 0x016C4304 */
QGLDECL void ( QGLAPI * dllCompressedTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data ); /* 0x016C4A5C */
QGLDECL void ( QGLAPI * dllCompressedTexSubImage2DARB )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data ); /* 0x016C4AE8 */
QGLDECL void ( QGLAPI * dllCompressedTexSubImage3DARB )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data ); /* 0x016C3F7C */
QGLDECL void ( QGLAPI * dllDeleteBuffersARB )( GLsizei n, const GLuint *buffers );                                   /* 0x016C4498 */
QGLDECL void ( QGLAPI * dllDeleteFencesNV )( GLsizei n, const GLuint *fences );                                      /* 0x016C43BC */
QGLDECL void ( QGLAPI * dllDeleteFragmentShaderATI )( GLuint id );                                                   /* 0x016C3E1C */
QGLDECL void ( QGLAPI * dllDeleteProgramsARB )( GLsizei n, const GLuint *programs );                                 /* 0x016C43A4 */
QGLDECL void ( QGLAPI * dllDisableVertexAttribArrayARB )( GLuint index );                                            /* 0x016C4048 */
QGLDECL void ( QGLAPI * dllDrawElementArrayATI )( GLenum mode, GLsizei count );                                      /* 0x016C44A0 */
QGLDECL void ( QGLAPI * dllDrawRangeElementArrayATI )( GLenum mode, GLuint start, GLuint end, GLsizei count );       /* 0x016C468C */
QGLDECL void ( QGLAPI * dllDrawRangeElementsEXT )( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices ); /* 0x016C47C8 */
QGLDECL void ( QGLAPI * dllElementPointerATI )( GLenum type, const GLvoid *pointer );                                /* 0x016C3C70 */
QGLDECL void ( QGLAPI * dllEnableVertexAttribArrayARB )( GLuint index );                                             /* 0x016C467C */
QGLDECL void ( QGLAPI * dllEndFragmentShaderATI )( void );                                                           /* 0x016C4478 */
QGLDECL void ( QGLAPI * dllFinalCombinerInputNV )( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ); /* 0x016C4470 */
QGLDECL void ( QGLAPI * dllFinishFenceNV )( GLuint fence );                                                          /* 0x016C3CCC */
QGLDECL void ( QGLAPI * dllFlushVertexArrayRangeNV )( void );                                                        /* 0x016C4A04 */
QGLDECL void ( QGLAPI * dllFreeObjectBufferATI )( GLuint buffer );                                                   /* 0x016C3FF4 */
QGLDECL void ( QGLAPI * dllGenBuffersARB )( GLsizei n, GLuint *buffers );                                            /* 0x016C43D8 */
QGLDECL void ( QGLAPI * dllGenFencesNV )( GLsizei n, GLuint *fences );                                               /* 0x016C45B4 */
QGLDECL GLuint ( QGLAPI * dllGenFragmentShadersATI )( GLuint range );                                                /* 0x016C40C8 */
QGLDECL void ( QGLAPI * dllGenProgramsARB )( GLsizei n, GLuint *programs );                                          /* 0x016C3BD4 */
QGLDECL void ( QGLAPI * dllGetArrayObjectfvATI )( GLenum array, GLenum pname, GLfloat *params );                     /* 0x016C4014 */
QGLDECL void ( QGLAPI * dllGetArrayObjectivATI )( GLenum array, GLenum pname, GLint *params );                       /* 0x016C3EDC */
QGLDECL void ( QGLAPI * dllGetBufferParameterivARB )( GLenum target, GLenum pname, GLint *params );                  /* 0x016C3F54 */
QGLDECL void ( QGLAPI * dllGetBufferPointervARB )( GLenum target, GLenum pname, GLvoid **params );                   /* 0x016C4610 */
QGLDECL void ( QGLAPI * dllGetBufferSubDataARB )( GLenum target, GLint offset, GLsizei size, GLvoid *data );         /* 0x016C4618 */
QGLDECL void ( QGLAPI * dllGetCombinerInputParameterfvNV )( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params ); /* 0x016C4364 */
QGLDECL void ( QGLAPI * dllGetCombinerInputParameterivNV )( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params ); /* 0x016C4640 */
QGLDECL void ( QGLAPI * dllGetCombinerOutputParameterfvNV )( GLenum stage, GLenum portion, GLenum pname, GLfloat *params ); /* 0x016C3F68 */
QGLDECL void ( QGLAPI * dllGetCombinerOutputParameterivNV )( GLenum stage, GLenum portion, GLenum pname, GLint *params ); /* 0x016C4A14 */
QGLDECL void ( QGLAPI * dllGetCombinerStageParameterfvNV )( GLenum stage, GLenum pname, GLfloat *params );           /* 0x016C49F0 */
QGLDECL void ( QGLAPI * dllGetCompressedTexImage )( GLenum target, GLint level, GLvoid *img );                       /* 0x016C4928 */
QGLDECL void ( QGLAPI * dllGetFenceivNV )( GLuint fence, GLenum pname, GLint *params );                              /* 0x016C3CB4 */
QGLDECL void ( QGLAPI * dllGetFinalCombinerInputParameterfvNV )( GLenum variable, GLenum pname, GLfloat *params );   /* 0x016C3F40 */
QGLDECL void ( QGLAPI * dllGetFinalCombinerInputParameterivNV )( GLenum variable, GLenum pname, GLint *params );     /* 0x016C41EC */
QGLDECL void ( QGLAPI * dllGetObjectBufferfvATI )( GLuint buffer, GLenum pname, GLfloat *params );                   /* 0x016C4600 */
QGLDECL void ( QGLAPI * dllGetObjectBufferivATI )( GLuint buffer, GLenum pname, GLint *params );                     /* 0x016C41E4 */
QGLDECL void ( QGLAPI * dllGetProgramEnvParameterdvARB )( GLenum target, GLuint index, GLdouble *params );           /* 0x016C406C */
QGLDECL void ( QGLAPI * dllGetProgramEnvParameterfvARB )( GLenum target, GLuint index, GLfloat *params );            /* 0x016C4A94 */
QGLDECL void ( QGLAPI * dllGetProgramLocalParameterdvARB )( GLenum target, GLuint index, GLdouble *params );         /* 0x016C3C08 */
QGLDECL void ( QGLAPI * dllGetProgramLocalParameterfvARB )( GLenum target, GLuint index, GLfloat *params );          /* 0x016C4664 */
QGLDECL void ( QGLAPI * dllGetProgramStringARB )( GLenum target, GLenum pname, GLvoid *string );                     /* 0x016C49B0 */
QGLDECL void ( QGLAPI * dllGetProgramivARB )( GLenum target, GLenum pname, GLint *params );                          /* 0x016C4050 */
QGLDECL void ( QGLAPI * dllGetVariantArrayObjectfvATI )( GLuint id, GLenum pname, GLfloat *params );                 /* 0x016C3B8C */
QGLDECL void ( QGLAPI * dllGetVariantArrayObjectivATI )( GLuint id, GLenum pname, GLint *params );                   /* 0x016C4998 */
QGLDECL void ( QGLAPI * dllGetVertexAttribPointervARB )( GLuint index, GLenum pname, GLvoid **pointer );             /* 0x016C47DC */
QGLDECL void ( QGLAPI * dllGetVertexAttribdvARB )( GLuint index, GLenum pname, GLdouble *params );                   /* 0x016C4390 */
QGLDECL void ( QGLAPI * dllGetVertexAttribfvARB )( GLuint index, GLenum pname, GLfloat *params );                    /* 0x016C482C */
QGLDECL void ( QGLAPI * dllGetVertexAttribivARB )( GLuint index, GLenum pname, GLint *params );                      /* 0x016C49E8 */
QGLDECL GLboolean ( QGLAPI * dllIsBufferARB )( GLuint buffer );                                                      /* 0x016C4264 */
QGLDECL GLboolean ( QGLAPI * dllIsFenceNV )( GLuint fence );                                                         /* 0x016C3F30 */
QGLDECL GLboolean ( QGLAPI * dllIsObjectBufferATI )( GLuint buffer );                                                /* 0x016C4888 */
QGLDECL GLboolean ( QGLAPI * dllIsProgramARB )( GLuint program );                                                    /* 0x016C4808 */
QGLDECL void ( QGLAPI * dllLockArraysEXT )( GLint first, GLsizei count );                                            /* 0x016C4124 */
QGLDECL GLvoid * ( QGLAPI * dllMapBufferARB )( GLenum target, GLenum access );                                       /* 0x016C3C4C */
QGLDECL GLuint ( QGLAPI * dllNewObjectBufferATI )( GLsizei size, const GLvoid *pointer, GLenum usage );              /* 0x016C440C */
QGLDECL void ( QGLAPI * dllPNTrianglesfATI )( GLenum pname, GLfloat param );                                         /* 0x016C3F08 */
QGLDECL void ( QGLAPI * dllPNTrianglesiATI )( GLenum pname, GLint param );                                           /* 0x016C409C */
QGLDECL void ( QGLAPI * dllPassTexCoordATI )( GLuint dst, GLuint coord, GLenum swizzle );                            /* 0x016C3F3C */
QGLDECL void ( QGLAPI * dllProgramEnvParameter4dARB )( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ); /* 0x016C4708 */
QGLDECL void ( QGLAPI * dllProgramEnvParameter4dvARB )( GLenum target, GLuint index, const GLdouble *params );       /* 0x016C4A90 */
QGLDECL void ( QGLAPI * dllProgramEnvParameter4fARB )( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ); /* 0x016C444C */
QGLDECL void ( QGLAPI * dllProgramEnvParameter4fvARB )( GLenum target, GLuint index, const GLfloat *params );        /* 0x016C46FC */
QGLDECL void ( QGLAPI * dllProgramLocalParameter4dARB )( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w ); /* 0x016C4614 */
QGLDECL void ( QGLAPI * dllProgramLocalParameter4dvARB )( GLenum target, GLuint index, const GLdouble *params );     /* 0x016C4078 */
QGLDECL void ( QGLAPI * dllProgramLocalParameter4fARB )( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w ); /* 0x016C4350 */
QGLDECL void ( QGLAPI * dllProgramLocalParameter4fvARB )( GLenum target, GLuint index, const GLfloat *params );      /* 0x016C4878 */
QGLDECL void ( QGLAPI * dllProgramStringARB )( GLenum target, GLenum format, GLsizei len, const GLvoid *string );    /* 0x016C477C */
QGLDECL void ( QGLAPI * dllSampleMapATI )( GLuint dst, GLuint interp, GLenum swizzle );                              /* 0x016C3D1C */
QGLDECL void ( QGLAPI * dllSetFenceNV )( GLuint fence, GLenum condition );                                           /* 0x016C4028 */
QGLDECL void ( QGLAPI * dllSetFragmentShaderConstantATI )( GLuint dst, const GLfloat *value );                       /* 0x016C4588 */
QGLDECL GLboolean ( QGLAPI * dllTestFenceNV )( GLuint fence );                                                       /* 0x016C46E4 */
QGLDECL void ( QGLAPI * dllUnlockArraysEXT )( void );                                                                /* 0x016C4398 */
QGLDECL GLboolean ( QGLAPI * dllUnmapBufferARB )( GLenum target );                                                   /* 0x016C42E0 */
QGLDECL void ( QGLAPI * dllUpdateObjectBufferATI )( GLuint buffer, GLuint offset, GLsizei size, const GLvoid *pointer, GLenum preserve ); /* 0x016C45F4 */
QGLDECL void ( QGLAPI * dllVariantArrayObjectATI )( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset ); /* 0x016C4660 */
QGLDECL void ( QGLAPI * dllVertexArrayRangeNV )( GLsizei length, const GLvoid *pointer );                            /* 0x016C3C98 */
QGLDECL void ( QGLAPI * dllVertexAttrib1dARB )( GLuint index, GLdouble x );                                          /* 0x016C4774 */
QGLDECL void ( QGLAPI * dllVertexAttrib1dvARB )( GLuint index, const GLdouble *v );                                  /* 0x016C463C */
QGLDECL void ( QGLAPI * dllVertexAttrib1fARB )( GLuint index, GLfloat x );                                           /* 0x016C4438 */
QGLDECL void ( QGLAPI * dllVertexAttrib1fvARB )( GLuint index, const GLfloat *v );                                   /* 0x016C4A6C */
QGLDECL void ( QGLAPI * dllVertexAttrib1sARB )( GLuint index, GLshort x );                                           /* 0x016C43F8 */
QGLDECL void ( QGLAPI * dllVertexAttrib1svARB )( GLuint index, const GLshort *v );                                   /* 0x016C4884 */
QGLDECL void ( QGLAPI * dllVertexAttrib2dARB )( GLuint index, GLdouble x, GLdouble y );                              /* 0x016C3ED8 */
QGLDECL void ( QGLAPI * dllVertexAttrib2dvARB )( GLuint index, const GLdouble *v );                                  /* 0x016C42F4 */
QGLDECL void ( QGLAPI * dllVertexAttrib2fARB )( GLuint index, GLfloat x, GLfloat y );                                /* 0x016C40D4 */
QGLDECL void ( QGLAPI * dllVertexAttrib2fvARB )( GLuint index, const GLfloat *v );                                   /* 0x016C41A8 */
QGLDECL void ( QGLAPI * dllVertexAttrib2sARB )( GLuint index, GLshort x, GLshort y );                                /* 0x016C3D10 */
QGLDECL void ( QGLAPI * dllVertexAttrib2svARB )( GLuint index, const GLshort *v );                                   /* 0x016C4A7C */
QGLDECL void ( QGLAPI * dllVertexAttrib3dARB )( GLuint index, GLdouble x, GLdouble y, GLdouble z );                  /* 0x016C3E6C */
QGLDECL void ( QGLAPI * dllVertexAttrib3dvARB )( GLuint index, const GLdouble *v );                                  /* 0x016C4920 */
QGLDECL void ( QGLAPI * dllVertexAttrib3fARB )( GLuint index, GLfloat x, GLfloat y, GLfloat z );                     /* 0x016C44F4 */
QGLDECL void ( QGLAPI * dllVertexAttrib3fvARB )( GLuint index, const GLfloat *v );                                   /* 0x016C481C */
QGLDECL void ( QGLAPI * dllVertexAttrib3sARB )( GLuint index, GLshort x, GLshort y, GLshort z );                     /* 0x016C40F4 */
QGLDECL void ( QGLAPI * dllVertexAttrib3svARB )( GLuint index, const GLshort *v );                                   /* 0x016C3E30 */
QGLDECL void ( QGLAPI * dllVertexAttrib4NbvARB )( GLuint index, const GLbyte *v );                                   /* 0x016C4AFC */
QGLDECL void ( QGLAPI * dllVertexAttrib4NivARB )( GLuint index, const GLint *v );                                    /* 0x016C410C */
QGLDECL void ( QGLAPI * dllVertexAttrib4NsvARB )( GLuint index, const GLshort *v );                                  /* 0x016C4904 */
QGLDECL void ( QGLAPI * dllVertexAttrib4NubARB )( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );        /* 0x016C4794 */
QGLDECL void ( QGLAPI * dllVertexAttrib4NubvARB )( GLuint index, const GLubyte *v );                                 /* 0x016C41FC */
QGLDECL void ( QGLAPI * dllVertexAttrib4NuivARB )( GLuint index, const GLuint *v );                                  /* 0x016C45BC */
QGLDECL void ( QGLAPI * dllVertexAttrib4NusvARB )( GLuint index, const GLushort *v );                                /* 0x016C47D0 */
QGLDECL void ( QGLAPI * dllVertexAttrib4bvARB )( GLuint index, const GLbyte *v );                                    /* 0x016C3BC4 */
QGLDECL void ( QGLAPI * dllVertexAttrib4dARB )( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );      /* 0x016C3B40 */
QGLDECL void ( QGLAPI * dllVertexAttrib4dvARB )( GLuint index, const GLdouble *v );                                  /* 0x016C4ADC */
QGLDECL void ( QGLAPI * dllVertexAttrib4fARB )( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );          /* 0x016C3F38 */
QGLDECL void ( QGLAPI * dllVertexAttrib4fvARB )( GLuint index, const GLfloat *v );                                   /* 0x016C43E4 */
QGLDECL void ( QGLAPI * dllVertexAttrib4ivARB )( GLuint index, const GLint *v );                                     /* 0x016C3D60 */
QGLDECL void ( QGLAPI * dllVertexAttrib4sARB )( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );          /* 0x016C43AC */
QGLDECL void ( QGLAPI * dllVertexAttrib4svARB )( GLuint index, const GLshort *v );                                   /* 0x016C4654 */
QGLDECL void ( QGLAPI * dllVertexAttrib4ubvARB )( GLuint index, const GLubyte *v );                                  /* 0x016C49C0 */
QGLDECL void ( QGLAPI * dllVertexAttrib4uivARB )( GLuint index, const GLuint *v );                                   /* 0x016C46EC */
QGLDECL void ( QGLAPI * dllVertexAttrib4usvARB )( GLuint index, const GLushort *v );                                 /* 0x016C459C */
QGLDECL void ( QGLAPI * dllVertexAttribPointerARB )( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ); /* 0x016C45E4 */
#endif /* QGL_OWNER || QGL_DRIVER_SLOTS */


#define glAlphaFunc                    qglAlphaFunc                       /* 0x016C4528 */
#define glBindFragmentShaderATI        qglBindFragmentShaderATI           /* 0x016C4334 */
#define glBindProgramARB               qglBindProgramARB                  /* 0x016C3FC4 */
#define glBindTexture                  qglBindTexture                     /* 0x016C439C */
#define glClear                        qglClear                           /* 0x016C46A4 */
#define glClearColor                   qglClearColor                      /* 0x016C4838 */
#define glColor4ubv                    qglColor4ubv                       /* 0x016C41D0 */
#define glColorPointer                 qglColorPointer                    /* 0x016C474C */
#define glCombinerInputNV              qglCombinerInputNV                 /* 0x016C48A0 */
#define glCombinerOutputNV             qglCombinerOutputNV                /* 0x016C3B30 */
#define glCombinerParameterfvNV        qglCombinerParameterfvNV           /* 0x016C3D78 */
#define glCombinerParameteriNV         qglCombinerParameteriNV            /* 0x016C45C0 */
#define glCombinerStageParameterfvNV   qglCombinerStageParameterfvNV      /* 0x016C3C74 */
#define glCopyTexSubImage2D            qglCopyTexSubImage2D               /* 0x016C4AF8 */
#define glDepthFunc                    qglDepthFunc                       /* 0x016C3B64 */
#define glDepthRange                   qglDepthRange                      /* 0x016C46BC */
#define glDisable                      qglDisable                         /* 0x016C3D00 */
#define glDisableClientState           qglDisableClientState              /* 0x016C3C7C */
#define glDrawArrays                   qglDrawArrays                      /* 0x016C433C */
#define glDrawBuffer                   qglDrawBuffer                      /* 0x016C4598 */
#define glDrawElementArrayATI          qglDrawElementArrayATI             /* 0x016C3E00 */
#define glDrawElements                 qglDrawElements                    /* 0x016C445C */
#define glEnable                       qglEnable                          /* 0x016C4288 */
#define glEnableClientState            qglEnableClientState               /* 0x016C3C58 */
#define glFinalCombinerInputNV         qglFinalCombinerInputNV            /* 0x016C3DF0 */
#define qglGetError_0                  qglGetError                        /* 0x016C3B5C */
#define glLightModelfv                 qglLightModelfv                    /* 0x016C3F00 */
#define glLoadMatrixf                  qglLoadMatrixf                     /* 0x016C4020 */
#define glMatrixMode                   qglMatrixMode                      /* 0x016C408C */
#define glNormalPointer                qglNormalPointer                   /* 0x016C435C */
#define lConfig_textureCompression     qglPNTrianglesiATI                 /* 0x016C3BE0 */
#define glPolygonMode                  qglPolygonMode                     /* 0x016C4094 */
#define glProgramLocalParameter4fARB   qglProgramLocalParameter4fARB      /* 0x016C4044 */
#define glProgramLocalParameter4fvARB  qglProgramLocalParameter4fvARB     /* 0x016C3DC4 */
#define glReadPixels                   qglReadPixels                      /* 0x016C4234 */
#define glScissor                      qglScissor                         /* 0x016C4540 */
#define glStencilFunc                  qglStencilFunc                     /* 0x016C419C */
#define glStencilOp                    qglStencilOp                       /* 0x016C4748 */
#define glTexCoordPointer              qglTexCoordPointer                 /* 0x016C3CC0 */
#define glTexEnvf                      qglTexEnvf                         /* 0x016C3CD8 */
#define glTexEnvfv                     qglTexEnvfv                        /* 0x016C3D30 */
#define glTexEnvi                      qglTexEnvi                         /* 0x016C447C */
#define glTexImage2D                   qglTexImage2D                      /* 0x016C3FE4 */
#define glTexParameteri                qglTexParameteri                   /* 0x016C49DC */
#define glTexSubImage2D                qglTexSubImage2D                   /* 0x016C416C */
#define glVertexPointer                qglVertexPointer                   /* 0x016C4810 */
#define glViewport                     qglViewport                        /* 0x016C47A0 */
#endif
