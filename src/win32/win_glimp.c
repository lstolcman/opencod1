/*
 * @fidelity: verified
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../qcommon/qcommon.h"
#include "../qcommon/win_vars.h"

/* GLW_InitExtensions fills the driver slots alongside the qgl pointers. */
#define QGL_DRIVER_SLOTS
#include "../renderer/qgl.h"
#include "win_wgl.h"
#include "../renderer/tr_public.h"
#include "../renderer/tr_local.h"   /* renderer cvars, glState, backEndStorage (RTCW includes it here too) */

/* Globals owned by this unit. */
int glState_currentTextures_1;   /* 0x016C38CC */
FILE *Stream;                    /* 0x019C0004, the QGL log file (RTCW glw_state.log_fp) */

/* ---- sub_50AF10  0x0050AF10 ----
 * GLimp_LogComment: RTCW win_glimp.c:1438; Mac sky calls its no-op counterpart.
 * Retain the recorded retail entry name. Native VC7 produces the same nine
 * instructions, including the optimized ECX comment argument, without asm.
 */
void sub_50AF10(char *comment)
{
    if (Stream)
        fprintf(Stream, "%s", comment);
}


#define RCVAR( n )  ( (cvar_t *)( n ) )

#define GLW_PRINT_ALL       0
#define GLW_PRINT_WARNING   2

/* 0x00545AFC: the va() format of every driver-feature fatal error; the "\x15\n\n\x14"
 * tail appends the localized EXE_ERR_GET_NEWEST_DRIVERS reference.  The literal stays
 * split because 'E' is a hex digit. */
#define GLW_DRIVER_ERR_FMT  "%s\x15\n\n\x14" "EXE_ERR_GET_NEWEST_DRIVERS"

#define GL_VENDOR               0x1F00      /* push 1F00h @ 0x0050AC96 */
#define GL_RENDERER             0x1F01
#define GL_VERSION              0x1F02
#define GL_EXTENSIONS           0x1F03
#define GL_MAX_TEXTURE_SIZE     0x0D33      /* push 3379 @ 0x005080E1 */

extern char    *va( const char *format, ... );
extern char    *SEH_GetLocalizedString_m( const char *reference );

/* GLW_ApplyGLCSV inputs: 0x016C37E0 (registered in R_Register) and the q_parse.c
 * helpers retail inlines there; qcommon.h does not declare these two. */
extern cvar_t  *r_skip_auto_config;
char           *Com_ParseOnLine( char **data_p );
void            Com_SkipRestOfLine( char **data_p );

#define QGL_GetString( e )      qglGetString( e )
#define QGL_GetIntegerv( e, p ) qglGetIntegerv( ( e ), ( p ) )

/* GLW_InitExtensions reads glConfig.extensions_string afresh for every test (0x00509381, ...). */
#define GLW_EXTENSIONS          ( (const char *)glConfig_extensions_string )

/* GLW_SetMode return codes, from the caller's `dec eax` chain at 0x0050A59F. */
#define RSERR_OK            0
#define RSERR_INVALID_FULLSCREEN 1
#define RSERR_INVALID_MODE  2

#define WINDOW_CLASS_NAME   "Call of Duty Multiplayer"

extern void WG_CheckHardwareGamma( void );
extern void WG_RestoreGamma( void );

int      glw_allowSoftwareGL;       /* 0x019BFFC8 r_allowSoftwareGL cvar_t* */
glwstate_t glw_state;              /* 0x019BFFE0 */

static PIXELFORMATDESCRIPTOR    glw_pfd;    /* 0x011EECB8, 40 bytes */
static int                      s_classRegistered;  /* 0x014072E8 */

extern qboolean QCL_Init( const char *dllname );
extern void     QGL_Shutdown( void );
extern void     QGL_EnableDrawProfiling( qboolean enable );  /* 0x004CAE60, inlined in GLimp_EndFrame */
extern void     QGL_EnableErrorChecking( qboolean enable );  /* 0x004CAEB0 */
extern void     QGL_EnableLogging( qboolean enable );        /* 0x004CEB10 */

/* GLimp_EndFrame's per-frame GL debug toggles; defined in renderer/tr_init.c. */
extern cvar_t  *r_debugGLErrors;        /* 0x016C3728 */
extern cvar_t  *r_profileDrawElements;  /* 0x016C3700 */

#define GL_MAX_TEXTURE_UNITS_ARB        0x84E2
#define GL_TEXTURE0_ARB                 0x84C0
#define GL_MAX_GENERAL_COMBINERS_NV     0x854D
#define GL_FOG_DISTANCE_MODE_NV         0x855A
#define GL_EYE_PLANE_ABSOLUTE_NV        0x855C

/* Retail stores each wglGetProcAddress result in the driver slot and its qgl
   pointer together (qglX = dllX = qwglGetProcAddress( name )). */
#define GLW_BIND( n, name ) \
	( *(PROC *)&dll##n = qwglGetProcAddress( name ), qgl##n = dll##n )
#define GLW_BIND_WGL( n, name ) \
	( *(PROC *)&dllwgl##n = qwglGetProcAddress( name ), qwgl##n = dllwgl##n )
/* 0x00509416: CompressedTexSubImage1D alone stores its qgl pointer before the driver slot. */
#define GLW_BIND_QGL_FIRST( n, name ) \
	( *(PROC *)&qgl##n = qwglGetProcAddress( name ), dll##n = qgl##n )

/* ---- GLW_HaveGLVersion  0x00508B80 ---- */
static qboolean GLW_HaveGLVersion( int major, int minor ) {
	char	version[1024];
	int		haveMajor, haveMinor;
	char	*dot;

	strncpy( version, (const char *)glConfig_version_string, sizeof( version ) );
	version[sizeof( version ) - 1] = '\0';

	haveMajor = atoi( version );
	dot = strchr( version, '.' );
	haveMinor = dot ? atoi( dot + 1 ) : 0;

	if ( haveMajor < major || ( haveMajor == major && haveMinor < minor ) ) {
		return qfalse;
	}
	return qtrue;
}

/* ---- GLW_HaveExtension  0x00508C30 ---- */
static qboolean GLW_HaveExtension( const char *ext, const char *extensions ) {
	size_t	len;
	char	*p;

	/* Retail has no NULL test on extensions: strstr runs on it directly (0x00508C4D). */
	len = strlen( ext );
	p = strstr( (char *)extensions, ext );
	while ( p ) {
		if ( ( p == extensions || isspace( *( p - 1 ) ) ) &&
			 ( isspace( p[len] ) || p[len] == '\0' ) ) {
			return qtrue;
		}
		p = strstr( p + 1, ext );
	}
	return qfalse;
}

void GLW_DriverError( const char *reason ) {
 ri.Error(0,va(GLW_DRIVER_ERR_FMT,reason));
}

/* ---- GLW_MissingFeature  0x00508CD0 ---- */
static void GLW_MissingFeature( void ) {
	ri.Printf( GLW_PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig_vendor_string );
	ri.Printf( GLW_PRINT_ALL, "GL_RENDERER: %s\n", glConfig_renderer_string );
	ri.Printf( GLW_PRINT_ALL, "GL_VERSION: %s\n", glConfig_version_string );
	ri.Printf( GLW_PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig_extensions_string );
	ri.Error( 0, va( GLW_DRIVER_ERR_FMT, "EXE_ERR_VIDEOCARD_MISSING_FEATURE" ) );
}

/* ---- GLW_InitVertexProgramExtension  0x00508D40 ---- */
static void GLW_InitVertexProgramExtension( void ) {
	GLW_BIND( VertexAttrib1sARB,   "glVertexAttrib1sARB" );
	GLW_BIND( VertexAttrib1fARB,   "glVertexAttrib1fARB" );
	GLW_BIND( VertexAttrib1dARB,   "glVertexAttrib1dARB" );
	GLW_BIND( VertexAttrib2sARB,   "glVertexAttrib2sARB" );
	GLW_BIND( VertexAttrib2fARB,   "glVertexAttrib2fARB" );
	GLW_BIND( VertexAttrib2dARB,   "glVertexAttrib2dARB" );
	GLW_BIND( VertexAttrib3sARB,   "glVertexAttrib3sARB" );
	GLW_BIND( VertexAttrib3fARB,   "glVertexAttrib3fARB" );
	GLW_BIND( VertexAttrib3dARB,   "glVertexAttrib3dARB" );
	GLW_BIND( VertexAttrib4sARB,   "glVertexAttrib4sARB" );
	GLW_BIND( VertexAttrib4fARB,   "glVertexAttrib4fARB" );
	GLW_BIND( VertexAttrib4dARB,   "glVertexAttrib4dARB" );
	GLW_BIND( VertexAttrib4NubARB, "glVertexAttrib4NubARB" );
	GLW_BIND( VertexAttrib1svARB,  "glVertexAttrib1svARB" );
	GLW_BIND( VertexAttrib1fvARB,  "glVertexAttrib1fvARB" );
	GLW_BIND( VertexAttrib1dvARB,  "glVertexAttrib1dvARB" );
	GLW_BIND( VertexAttrib2svARB,  "glVertexAttrib2svARB" );
	GLW_BIND( VertexAttrib2fvARB,  "glVertexAttrib2fvARB" );
	GLW_BIND( VertexAttrib2dvARB,  "glVertexAttrib2dvARB" );
	GLW_BIND( VertexAttrib3svARB,  "glVertexAttrib3svARB" );
	GLW_BIND( VertexAttrib3fvARB,  "glVertexAttrib3fvARB" );
	GLW_BIND( VertexAttrib3dvARB,  "glVertexAttrib3dvARB" );
	GLW_BIND( VertexAttrib4bvARB,  "glVertexAttrib4bvARB" );
	GLW_BIND( VertexAttrib4svARB,  "glVertexAttrib4svARB" );
	GLW_BIND( VertexAttrib4ivARB,  "glVertexAttrib4ivARB" );
	GLW_BIND( VertexAttrib4ubvARB, "glVertexAttrib4ubvARB" );
	GLW_BIND( VertexAttrib4usvARB, "glVertexAttrib4usvARB" );
	GLW_BIND( VertexAttrib4uivARB, "glVertexAttrib4uivARB" );
	GLW_BIND( VertexAttrib4fvARB,  "glVertexAttrib4fvARB" );
	GLW_BIND( VertexAttrib4dvARB,  "glVertexAttrib4dvARB" );
	GLW_BIND( VertexAttrib4NbvARB,  "glVertexAttrib4NbvARB" );
	GLW_BIND( VertexAttrib4NsvARB,  "glVertexAttrib4NsvARB" );
	GLW_BIND( VertexAttrib4NivARB,  "glVertexAttrib4NivARB" );
	GLW_BIND( VertexAttrib4NubvARB, "glVertexAttrib4NubvARB" );
	GLW_BIND( VertexAttrib4NusvARB, "glVertexAttrib4NusvARB" );
	GLW_BIND( VertexAttrib4NuivARB, "glVertexAttrib4NuivARB" );
	GLW_BIND( VertexAttribPointerARB, "glVertexAttribPointerARB" );
	GLW_BIND( EnableVertexAttribArrayARB,  "glEnableVertexAttribArrayARB" );
	GLW_BIND( DisableVertexAttribArrayARB, "glDisableVertexAttribArrayARB" );
	GLW_BIND( ProgramStringARB,    "glProgramStringARB" );
	GLW_BIND( BindProgramARB,      "glBindProgramARB" );
	GLW_BIND( DeleteProgramsARB,   "glDeleteProgramsARB" );
	GLW_BIND( GenProgramsARB,      "glGenProgramsARB" );
	GLW_BIND( ProgramEnvParameter4fARB,   "glProgramEnvParameter4fARB" );
	GLW_BIND( ProgramEnvParameter4dARB,   "glProgramEnvParameter4dARB" );
	GLW_BIND( ProgramEnvParameter4fvARB,  "glProgramEnvParameter4fvARB" );
	GLW_BIND( ProgramEnvParameter4dvARB,  "glProgramEnvParameter4dvARB" );
	GLW_BIND( ProgramLocalParameter4fARB,  "glProgramLocalParameter4fARB" );
	GLW_BIND( ProgramLocalParameter4dARB,  "glProgramLocalParameter4dARB" );
	GLW_BIND( ProgramLocalParameter4fvARB, "glProgramLocalParameter4fvARB" );
	GLW_BIND( ProgramLocalParameter4dvARB, "glProgramLocalParameter4dvARB" );
	GLW_BIND( GetProgramEnvParameterfvARB, "glGetProgramEnvParameterfvARB" );
	GLW_BIND( GetProgramEnvParameterdvARB, "glGetProgramEnvParameterdvARB" );
	GLW_BIND( GetProgramLocalParameterfvARB, "glGetProgramLocalParameterfvARB" );
	GLW_BIND( GetProgramLocalParameterdvARB, "glGetProgramLocalParameterdvARB" );
	GLW_BIND( GetProgramivARB,     "glGetProgramivARB" );
	GLW_BIND( GetProgramStringARB, "glGetProgramStringARB" );
	GLW_BIND( GetVertexAttribdvARB, "glGetVertexAttribdvARB" );
	GLW_BIND( GetVertexAttribfvARB, "glGetVertexAttribfvARB" );
	GLW_BIND( GetVertexAttribivARB, "glGetVertexAttribivARB" );
	GLW_BIND( GetVertexAttribPointervARB, "glGetVertexAttribPointervARB" );
	GLW_BIND( IsProgramARB,        "glIsProgramARB" );
}

/* ---- GLW_InitExtensions  0x00509260 ---- */
static void GLW_InitExtensions( void ) {
	int			combiners;

	/* 0x0050926C-0x005092E4: retail clears these qgl pointers (not their driver
	   slots) and three glConfig fields on entry; the other feature flags are reset
	   per block below or left to GLimp_Shutdown's glConfig clear. */
	qglActiveTextureARB          = NULL;
	qglClientActiveTextureARB    = NULL;
	qglLockArraysEXT             = NULL;
	qglUnlockArraysEXT           = NULL;
	qglPNTrianglesiATI           = NULL;
	qglPNTrianglesfATI           = NULL;
	qglNewObjectBufferATI        = NULL;
	qglIsObjectBufferATI         = NULL;
	qglUpdateObjectBufferATI     = NULL;
	qglGetObjectBufferfvATI      = NULL;
	qglGetObjectBufferivATI      = NULL;
	qglFreeObjectBufferATI       = NULL;
	qglArrayObjectATI            = NULL;
	qglGetArrayObjectfvATI       = NULL;
	qglGetArrayObjectivATI       = NULL;
	qglVariantArrayObjectATI     = NULL;
	qglGetVariantArrayObjectfvATI = NULL;
	qglGetVariantArrayObjectivATI = NULL;
	glConfig_maxAnisotropy       = 0;
	glConfig_NVFogAvailable      = 0;
	glConfig_NVFogMode           = 0;

	ri.Printf( GLW_PRINT_ALL, "Initializing OpenGL extensions\n" );

	if ( GLW_HaveGLVersion( 1, 3 ) ) {
		GLW_BIND( CompressedTexImage3DARB,    "glCompressedTexImage3D" );
		GLW_BIND( CompressedTexImage2DARB,    "glCompressedTexImage2D" );
		GLW_BIND( CompressedTexImage1DARB,    "glCompressedTexImage1D" );
		GLW_BIND( CompressedTexSubImage3DARB, "glCompressedTexSubImage3D" );
		GLW_BIND( CompressedTexSubImage2DARB, "glCompressedTexSubImage2D" );
		GLW_BIND_QGL_FIRST( CompressedTexSubImage1D, "glCompressedTexSubImage1D" );
		GLW_BIND( GetCompressedTexImage,      "glGetCompressedTexImage" );
	} else {
		if ( !GLW_HaveExtension( "GL_ARB_texture_compression", GLW_EXTENSIONS ) ) {
			GLW_MissingFeature();
		}
		GLW_BIND( CompressedTexImage3DARB,    "glCompressedTexImage3DARB" );
		GLW_BIND( CompressedTexImage2DARB,    "glCompressedTexImage2DARB" );
		GLW_BIND( CompressedTexImage1DARB,    "glCompressedTexImage1DARB" );
		GLW_BIND( CompressedTexSubImage3DARB, "glCompressedTexSubImage3DARB" );
		GLW_BIND( CompressedTexSubImage2DARB, "glCompressedTexSubImage2DARB" );
		GLW_BIND_QGL_FIRST( CompressedTexSubImage1D, "glCompressedTexSubImage1DARB" );
		GLW_BIND( GetCompressedTexImage,      "glGetCompressedTexImageARB" );
	}
	if ( !GLW_HaveExtension( "GL_EXT_texture_compression_s3tc", GLW_EXTENSIONS ) ) {
		GLW_MissingFeature();
	}

	qglDrawRangeElementsEXT = NULL;
	if ( GLW_HaveGLVersion( 1, 2 ) ) {
		if ( r_ext_draw_range_elements->integer ) {
			GLW_BIND( DrawRangeElementsEXT, "glDrawRangeElements" );
			ri.Printf( GLW_PRINT_ALL, "...using OpenGL 1.2 draw element range\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring OpenGL 1.2 draw element range\n" );
		}
	} else if ( GLW_HaveExtension( "GL_EXT_draw_range_elements", GLW_EXTENSIONS ) ) {
		if ( r_ext_draw_range_elements->integer ) {
			GLW_BIND( DrawRangeElementsEXT, "glDrawRangeElementsEXT" );
			ri.Printf( GLW_PRINT_ALL, "...using GL_EXT_draw_range_elements\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_EXT_draw_range_elements\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_EXT_draw_range_elements not found\n" );
	}

	glConfig_textureEnvAddAvailable = 0;
	if ( GLW_HaveGLVersion( 1, 3 ) ) {
		if ( r_arb_texture_env_add->integer ) {
			glConfig_textureEnvAddAvailable = 1;
			ri.Printf( GLW_PRINT_ALL, "...using OpenGL 1.3 texture add environment mode\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring OpenGL 1.3 texture add environment mode\n" );
		}
	} else if ( GLW_HaveExtension( "GL_EXT_texture_env_add", GLW_EXTENSIONS ) ) {
		if ( r_arb_texture_env_add->integer ) {
			glConfig_textureEnvAddAvailable = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	}

	glConfig_textureEnvCombine = 0;
	if ( GLW_HaveGLVersion( 1, 3 ) ) {
		if ( r_arb_texture_env_combine->integer ) {
			glConfig_textureEnvCombine = 1;
			ri.Printf( GLW_PRINT_ALL, "...using OpenGL 1.3 texture combine environment mode\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring OpenGL 1.3 texture combine environment mode\n" );
		}
	} else if ( GLW_HaveExtension( "GL_ARB_texture_env_combine", GLW_EXTENSIONS ) ) {
		if ( r_arb_texture_env_combine->integer ) {
			glConfig_textureEnvCombine = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_ARB_texture_env_combine\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ARB_texture_env_combine\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ARB_texture_env_combine not found\n" );
	}

	glConfig_textureEnvDot3 = 0;
	if ( GLW_HaveGLVersion( 1, 3 ) ) {
		if ( r_arb_texture_env_dot3->integer ) {
			glConfig_textureEnvDot3 = 1;
			ri.Printf( GLW_PRINT_ALL, "...using OpenGL 1.3 texture dot3 environment mode\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring OpenGL 1.3 texture dot3 environment mode\n" );
		}
	} else if ( GLW_HaveExtension( "GL_ARB_texture_env_dot3", GLW_EXTENSIONS ) ) {
		if ( r_arb_texture_env_dot3->integer && glConfig_textureEnvCombine ) {
			glConfig_textureEnvDot3 = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_ARB_texture_env_dot3\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ARB_texture_env_dot3\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ARB_texture_env_dot3 not found\n" );
	}

	glConfig_textureCubeMap = 0;
	if ( GLW_HaveGLVersion( 1, 3 ) ) {
		if ( r_arb_texture_cube_map->integer ) {
			glConfig_textureCubeMap = 1;
			ri.Printf( GLW_PRINT_ALL, "...using OpenGL 1.3 cube map textures\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring OpenGL 1.3 cube map textures\n" );
		}
	} else if ( GLW_HaveExtension( "GL_ARB_texture_cube_map", GLW_EXTENSIONS ) ) {
		if ( r_arb_texture_cube_map->integer ) {
			glConfig_textureCubeMap = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_ARB_texture_cube_map\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ARB_texture_cube_map\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ARB_texture_cube_map not found\n" );
	}

	glConfig_ARBVertexProgram = 0;
	if ( GLW_HaveExtension( "GL_ARB_vertex_program", GLW_EXTENSIONS ) ) {
		if ( r_arb_vertex_program->integer ) {
			glConfig_ARBVertexProgram = 1;
			GLW_InitVertexProgramExtension();
			ri.Printf( GLW_PRINT_ALL, "...using GL_ARB_vertex_program\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ARB_vertex_program\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ARB_vertex_program not found\n" );
	}

	glConfig_NVTextureShader = 0;
	if ( GLW_HaveExtension( "GL_NV_texture_shader", GLW_EXTENSIONS ) ) {
		if ( r_nv_texture_shader->integer ) {
			glConfig_NVTextureShader = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_NV_texture_shader\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_NV_texture_shader\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_NV_texture_shader not found\n" );
	}

	glConfig_NVRegisterCombiners = 0;
	if ( GLW_HaveExtension( "GL_NV_register_combiners", GLW_EXTENSIONS ) ) {
		if ( r_nv_register_combiners->integer ) {
			glConfig_NVRegisterCombiners = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_NV_register_combiners\n" );
			GLW_BIND( CombinerParameterfvNV, "glCombinerParameterfvNV" );
			GLW_BIND( CombinerParameterfNV,  "glCombinerParameterfNV" );
			GLW_BIND( CombinerParameterivNV, "glCombinerParameterivNV" );
			GLW_BIND( CombinerParameteriNV,  "glCombinerParameteriNV" );
			GLW_BIND( CombinerInputNV,       "glCombinerInputNV" );
			GLW_BIND( CombinerOutputNV,      "glCombinerOutputNV" );
			GLW_BIND( FinalCombinerInputNV,  "glFinalCombinerInputNV" );
			GLW_BIND( GetCombinerInputParameterfvNV,  "glGetCombinerInputParameterfvNV" );
			GLW_BIND( GetCombinerInputParameterivNV,  "glGetCombinerInputParameterivNV" );
			GLW_BIND( GetCombinerOutputParameterfvNV, "glGetCombinerOutputParameterfvNV" );
			GLW_BIND( GetCombinerOutputParameterivNV, "glGetCombinerOutputParameterivNV" );
			GLW_BIND( GetFinalCombinerInputParameterfvNV, "glGetFinalCombinerInputParameterfvNV" );
			GLW_BIND( GetFinalCombinerInputParameterivNV, "glGetFinalCombinerInputParameterivNV" );

			if ( GLW_HaveExtension( "GL_NV_register_combiners2", GLW_EXTENSIONS ) ) {
				qglGetIntegerv( GL_MAX_GENERAL_COMBINERS_NV, &combiners );
				if ( combiners < 8 ) {
					ri.Printf( GLW_PRINT_ALL,
						"...ignoring GL_NV_register_combiners2 because "
						"GL_MAX_GENERAL_COMBINERS_NV is %i < 8\n", combiners );
				} else if ( r_nv_register_combiners->integer >= 2 ) {
					glConfig_NVRegisterCombiners = 2;
					ri.Printf( GLW_PRINT_ALL, "...using GL_NV_register_combiners2\n" );
					GLW_BIND( CombinerStageParameterfvNV, "glCombinerStageParameterfvNV" );
					GLW_BIND( GetCombinerStageParameterfvNV, "glGetCombinerStageParameterfvNV" );
				} else {
					ri.Printf( GLW_PRINT_ALL, "...ignoring GL_NV_register_combiners2\n" );
				}
			} else {
				ri.Printf( GLW_PRINT_ALL, "...GL_NV_register_combiners2 not found\n" );
			}
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_NV_register_combiners\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_NV_register_combiners not found\n" );
	}

	GLW_BIND_WGL( SwapIntervalEXT, "wglSwapIntervalEXT" );
	if ( qwglSwapIntervalEXT ) {
		ri.Printf( GLW_PRINT_ALL, "...using WGL_EXT_swap_control\n" );
		r_swapInterval->modified = qtrue;
	} else {
		ri.Printf( GLW_PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	}

	if ( GLW_HaveGLVersion( 1, 3 ) ) {
		GLW_BIND( ActiveTextureARB,       "glActiveTexture" );
		GLW_BIND( ClientActiveTextureARB, "glClientActiveTexture" );
		if ( !qglActiveTextureARB || !qglClientActiveTextureARB ) {
			ri.Error( 0, va( GLW_DRIVER_ERR_FMT, "EXE_ERR_MULTITEX_INIT_FAIL" ) );
		}
		qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig_maxActiveTextures );
		if ( glConfig_maxActiveTextures <= 1 ) {
			ri.Error( 0, va( GLW_DRIVER_ERR_FMT, "EXE_ERR_MULTITEX_BAD_MAX" ) );
		}
		if ( glConfig_maxActiveTextures > 8 ) {
			glConfig_maxActiveTextures = 8;
		}
		if ( r_maxActiveTextures->integer >= 2 &&
			 glConfig_maxActiveTextures > r_maxActiveTextures->integer ) {
			glConfig_maxActiveTextures = r_maxActiveTextures->integer;
		}
		/* Retail bug reproduced: the format has no conversion, yet 0x00509A75 still pushes the count. */
		ri.Printf( GLW_PRINT_ALL, "...using OpenGL 1.3 multitexture\n",
				   glConfig_maxActiveTextures );
	} else if ( GLW_HaveExtension( "GL_ARB_multitexture", GLW_EXTENSIONS ) ) {
		GLW_BIND( ActiveTextureARB,       "glActiveTextureARB" );
		GLW_BIND( ClientActiveTextureARB, "glClientActiveTextureARB" );
		if ( !qglActiveTextureARB || !qglClientActiveTextureARB ) {
			ri.Error( 0, va( GLW_DRIVER_ERR_FMT, "EXE_ERR_ARB_MULTITEX_INIT_FAILED" ) );
		}
		qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &glConfig_maxActiveTextures );
		if ( glConfig_maxActiveTextures <= 1 ) {
			ri.Error( 0, va( GLW_DRIVER_ERR_FMT, "EXE_ERR_ARB_MULTITEX_BAD_MAX" ) );
		}
		if ( glConfig_maxActiveTextures > 8 ) {
			glConfig_maxActiveTextures = 8;
		}
		if ( r_maxActiveTextures->integer >= 2 &&
			 glConfig_maxActiveTextures > r_maxActiveTextures->integer ) {
			glConfig_maxActiveTextures = r_maxActiveTextures->integer;
		}
		ri.Printf( GLW_PRINT_ALL,
			"...using GL_ARB_multitexture with %i max textures\n", glConfig_maxActiveTextures );
	} else {
		GLW_MissingFeature();
	}

	if ( GLW_HaveExtension( "GL_EXT_compiled_vertex_array", GLW_EXTENSIONS ) ) {
		if ( r_ext_compiled_vertex_array->integer ) {
			ri.Printf( GLW_PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
			GLW_BIND( LockArraysEXT,   "glLockArraysEXT" );
			GLW_BIND( UnlockArraysEXT, "glUnlockArraysEXT" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
	}

	glConfig_rescaleNormal = 0;
	if ( GLW_HaveGLVersion( 1, 2 ) ) {
		if ( r_ext_rescale_normal->integer ) {
			glConfig_rescaleNormal = 1;
			ri.Printf( GLW_PRINT_ALL, "...using OpenGL 1.2 normal rescaling\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring OpenGL 1.2 normal rescaling\n" );
		}
	} else if ( GLW_HaveExtension( "GL_EXT_rescale_normal", GLW_EXTENSIONS ) ) {
		if ( r_ext_rescale_normal->integer ) {
			ri.Printf( GLW_PRINT_ALL, "...using GL_EXT_rescale_normal\n" );
			glConfig_rescaleNormal = 1;
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_EXT_rescale_normal\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_EXT_rescale_normal not found\n" );
	}

	if ( GLW_HaveExtension( "GL_ATI_pn_triangles", GLW_EXTENSIONS ) ) {
		if ( r_ati_pntriangles->integer ) {
			ri.Printf( GLW_PRINT_ALL, "...using GL_ATI_pn_triangles\n" );
			GLW_BIND( PNTrianglesiATI, "glPNTrianglesiATI" );
			GLW_BIND( PNTrianglesfATI, "glPNTrianglesfATI" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ATI_pn_triangles\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ATI_pn_triangles not found\n" );
		ri.Cvar_Set( "r_ati_pntriangles", "0" );
	}

	if ( GLW_HaveExtension( "GL_ARB_vertex_buffer_object", GLW_EXTENSIONS ) ) {
		if ( r_arb_vertex_buffer_object->integer ) {
			GLW_BIND( BindBufferARB,     "glBindBufferARB" );
			GLW_BIND( DeleteBuffersARB,  "glDeleteBuffersARB" );
			GLW_BIND( GenBuffersARB,     "glGenBuffersARB" );
			GLW_BIND( IsBufferARB,       "glIsBufferARB" );
			GLW_BIND( BufferDataARB,     "glBufferDataARB" );
			GLW_BIND( BufferSubDataARB,  "glBufferSubDataARB" );
			GLW_BIND( GetBufferSubDataARB, "glGetBufferSubDataARB" );
			GLW_BIND( MapBufferARB,      "glMapBufferARB" );
			GLW_BIND( UnmapBufferARB,    "glUnmapBufferARB" );
			GLW_BIND( GetBufferParameterivARB, "glGetBufferParameterivARB" );
			GLW_BIND( GetBufferPointervARB,    "glGetBufferPointervARB" );
			glConfig_ARBVertexBufferObject = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_ARB_vertex_buffer_object\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ARB_vertex_buffer_object\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ARB_vertex_buffer_object not found\n" );
	}

	if ( GLW_HaveExtension( "GL_NV_fog_distance", GLW_EXTENSIONS ) ) {
		if ( r_nv_fog_dist->integer ) {
			glConfig_NVFogAvailable = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_NV_fog_distance\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_NV_fog_distance\n" );
			qglFogi( GL_FOG_DISTANCE_MODE_NV, GL_EYE_PLANE_ABSOLUTE_NV );
		}
		ri.Cvar_Set( "r_nv_fog_available", "1" );
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_NV_fog_distance not found\n" );
		ri.Cvar_Set( "r_nv_fog_dist", "0" );
		ri.Cvar_Set( "r_nv_fog_available", "0" );
	}

	if ( GLW_HaveExtension( "GL_NV_vertex_array_range", GLW_EXTENSIONS ) ||
		 GLW_HaveExtension( "GL_NV_vertex_array_range2", GLW_EXTENSIONS ) ) {
		if ( r_nv_vertex_array_range->integer && !glConfig_ARBVertexBufferObject ) {
			GLW_BIND( FlushVertexArrayRangeNV, "glFlushVertexArrayRangeNV" );
			GLW_BIND( VertexArrayRangeNV,      "glVertexArrayRangeNV" );
			GLW_BIND_WGL( AllocateMemoryNV,       "wglAllocateMemoryNV" );
			GLW_BIND_WGL( FreeMemoryNV,           "wglFreeMemoryNV" );
			glConfig_NVVertexArrayRange = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_NV_vertex_array_range\n" );

			if ( GLW_HaveExtension( "GL_NV_vertex_array_range2", GLW_EXTENSIONS ) ) {
				if ( r_nv_vertex_array_range->integer == 2 ) {
					glConfig_NVVertexArrayRange = 2;
					ri.Printf( GLW_PRINT_ALL, "...using GL_NV_vertex_array_range2\n" );
				} else {
					ri.Printf( GLW_PRINT_ALL, "...ignoring GL_NV_vertex_array_range2\n" );
				}
			} else {
				ri.Printf( GLW_PRINT_ALL, "...GL_NV_vertex_array_range2 not found\n" );
			}
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_NV_vertex_array_range" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_NV_vertex_array_range not found\n" );
	}

	if ( GLW_HaveExtension( "GL_NV_fence", GLW_EXTENSIONS ) ) {
		if ( r_nv_fence->integer && !glConfig_ARBVertexBufferObject ) {
			GLW_BIND( DeleteFencesNV, "glDeleteFencesNV" );
			GLW_BIND( GenFencesNV,    "glGenFencesNV" );
			GLW_BIND( IsFenceNV,      "glIsFenceNV" );
			GLW_BIND( TestFenceNV,    "glTestFenceNV" );
			GLW_BIND( GetFenceivNV,   "glGetFenceivNV" );
			GLW_BIND( FinishFenceNV,  "glFinishFenceNV" );
			GLW_BIND( SetFenceNV,     "glSetFenceNV" );
			glConfig_NVFenceAvailable = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_NV_fence\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_NV_fence\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_NV_fence not found\n" );
	}

	if ( GLW_HaveExtension( "GL_ATI_vertex_array_object", GLW_EXTENSIONS ) ) {
		if ( r_ati_vertex_array_object->integer &&
			 !glConfig_ARBVertexBufferObject && !glConfig_NVVertexArrayRange ) {
			GLW_BIND( NewObjectBufferATI,    "glNewObjectBufferATI" );
			GLW_BIND( IsObjectBufferATI,     "glIsObjectBufferATI" );
			GLW_BIND( UpdateObjectBufferATI, "glUpdateObjectBufferATI" );
			GLW_BIND( GetObjectBufferfvATI,  "glGetObjectBufferfvATI" );
			GLW_BIND( GetObjectBufferivATI,  "glGetObjectBufferivATI" );
			GLW_BIND( FreeObjectBufferATI,   "glFreeObjectBufferATI" );
			GLW_BIND( ArrayObjectATI,        "glArrayObjectATI" );
			GLW_BIND( GetArrayObjectfvATI,   "glGetArrayObjectfvATI" );
			GLW_BIND( GetArrayObjectivATI,   "glGetArrayObjectivATI" );
			GLW_BIND( VariantArrayObjectATI, "glVariantArrayObjectATI" );
			GLW_BIND( GetVariantArrayObjectfvATI, "glGetVariantArrayObjectfvATI" );
			GLW_BIND( GetVariantArrayObjectivATI, "glGetVariantArrayObjectivATI" );
			glConfig_ATIVertexArrayObject = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_ATI_vertex_array_object\n" );

			if ( GLW_HaveExtension( "GL_ATI_element_array", GLW_EXTENSIONS ) ) {
				if ( r_ati_element_array->integer ) {
					GLW_BIND( ElementPointerATI,       "glElementPointerATI" );
					GLW_BIND( DrawElementArrayATI,     "glDrawElementArrayATI" );
					GLW_BIND( DrawRangeElementArrayATI, "glDrawRangeElementArrayATI" );
					glConfig_ATIElementArray = 1;
					ri.Printf( GLW_PRINT_ALL, "...using GL_ATI_element_array\n" );
				} else {
					ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ATI_element_array\n" );
				}
			} else {
				ri.Printf( GLW_PRINT_ALL, "...GL_ATI_element_array not found\n" );
			}
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ATI_vertex_array_object\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ATI_vertex_array_object not found\n" );
	}

	glConfig_ATIFragmentShader = 0;
	if ( GLW_HaveExtension( "GL_ATI_fragment_shader", GLW_EXTENSIONS ) ) {
		if ( r_ati_fragment_shader->integer ) {
			GLW_BIND( GenFragmentShadersATI,   "glGenFragmentShadersATI" );
			GLW_BIND( BindFragmentShaderATI,   "glBindFragmentShaderATI" );
			GLW_BIND( DeleteFragmentShaderATI, "glDeleteFragmentShaderATI" );
			GLW_BIND( BeginFragmentShaderATI,  "glBeginFragmentShaderATI" );
			GLW_BIND( EndFragmentShaderATI,    "glEndFragmentShaderATI" );
			GLW_BIND( PassTexCoordATI,         "glPassTexCoordATI" );
			GLW_BIND( SampleMapATI,            "glSampleMapATI" );
			GLW_BIND( ColorFragmentOp1ATI,     "glColorFragmentOp1ATI" );
			GLW_BIND( ColorFragmentOp2ATI,     "glColorFragmentOp2ATI" );
			GLW_BIND( ColorFragmentOp3ATI,     "glColorFragmentOp3ATI" );
			GLW_BIND( AlphaFragmentOp1ATI,     "glAlphaFragmentOp1ATI" );
			GLW_BIND( AlphaFragmentOp2ATI,     "glAlphaFragmentOp2ATI" );
			GLW_BIND( AlphaFragmentOp3ATI,     "glAlphaFragmentOp3ATI" );
			GLW_BIND( SetFragmentShaderConstantATI, "glSetFragmentShaderConstantATI" );
			glConfig_ATIFragmentShader = 1;
			ri.Printf( GLW_PRINT_ALL, "...using GL_ATI_fragment_shader\n" );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...ignoring GL_ATI_fragment_shader\n" );
		}
	} else {
		ri.Printf( GLW_PRINT_ALL, "...GL_ATI_fragment_shader not found\n" );
	}

	/* Both branches disable anisotropy: retail 0x0050A447 and 0x0050A45F each Cvar_Set it to "0";
	   0x0050A423 still reads r_ext_texture_filter_anisotropic->integer. */
	if ( GLW_HaveExtension( "GL_EXT_texture_filter_anisotropic", GLW_EXTENSIONS ) ) {
		if ( r_ext_texture_filter_anisotropic->integer ) {
			glConfig_maxAnisotropy = 0;
			ri.Printf( GLW_PRINT_ALL,
				"...ignoring GL_EXT_texture_filter_anisotropic\n" );
			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
		} else {
			glConfig_maxAnisotropy = 0;
			ri.Printf( GLW_PRINT_ALL,
				"...ignoring GL_EXT_texture_filter_anisotropic\n" );
			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
		}
	} else {
		ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
	}
}

/* ---- GLW_CheckOSVersion  0x0050A470 ---- */
/* Accepts NT5+ (major >= 5) or Win98/ME (4.x, VER_PLATFORM_WIN32_WINDOWS, minor >= 10);
 * rejects Win95 and NT4.  No memset of vinfo: retail only sets dwOSVersionInfoSize. */
static qboolean GLW_CheckOSVersion( void ) {
	OSVERSIONINFOA	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof( vinfo );	/* 0x94 */
	if ( !GetVersionExA( &vinfo ) ) {
		ri.Printf( GLW_PRINT_ALL, "GLW_CheckOSVersion() - GetVersionEx failed\n" );
		return qfalse;
	}
	if ( vinfo.dwMajorVersion >= 5 ) {
		return qtrue;
	}
	if ( vinfo.dwMajorVersion != 4 ) {
		return qfalse;
	}
	if ( vinfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS || vinfo.dwMinorVersion < 10 ) {
		return qfalse;
	}
	return qtrue;
}

/* ---- PrintCDSError  0x00508580 ---- */
static void PrintCDSError( int value ) {
	/* Retail prints each message as its own format literal, newline included
	   (0x0050858F..0x005085E4), not through "%s\n". */
	switch ( value ) {
	case DISP_CHANGE_RESTART:
		ri.Printf( GLW_PRINT_ALL, "restart required\n" );
		break;
	case DISP_CHANGE_BADPARAM:
		ri.Printf( GLW_PRINT_ALL, "bad param\n" );
		break;
	case DISP_CHANGE_BADFLAGS:
		ri.Printf( GLW_PRINT_ALL, "bad flags\n" );
		break;
	case DISP_CHANGE_FAILED:
		ri.Printf( GLW_PRINT_ALL, "DISP_CHANGE_FAILED\n" );
		break;
	case DISP_CHANGE_BADMODE:
		ri.Printf( GLW_PRINT_ALL, "bad mode\n" );
		break;
	case DISP_CHANGE_NOTUPDATED:
		ri.Printf( GLW_PRINT_ALL, "not updated\n" );
		break;
	default:
		/* 0x005085F5: unknown codes (0 included) print the value. */
		ri.Printf( GLW_PRINT_ALL, "unknown error %d\n", value );
		break;
	}
}

extern int R_GetModeInfo( int *width, int *height, float *windowAspect, int mode );

/* ---- GLW_CreatePFD  0x00507DD0 ----  VERIFIED */
static void GLW_CreatePFD( PIXELFORMATDESCRIPTOR *pPFD, int colorbits,
						   int depthbits, int stencilbits, qboolean stereo ) {
	PIXELFORMATDESCRIPTOR	src = {
		sizeof( PIXELFORMATDESCRIPTOR ),	/* size of this pfd */
		1,									/* version number */
		PFD_DRAW_TO_WINDOW |				/* support window */
		PFD_SUPPORT_OPENGL |				/* support OpenGL */
		PFD_DOUBLEBUFFER,					/* double buffered */
		PFD_TYPE_RGBA,						/* RGBA type */
		24,									/* 24-bit color depth */
		0, 0, 0, 0, 0, 0,					/* color bits ignored */
		0,									/* no alpha buffer */
		0,									/* shift bit ignored */
		0,									/* no accumulation buffer */
		0, 0, 0, 0,							/* accum bits ignored */
		24,									/* 24-bit z-buffer */
		8,									/* 8-bit stencil buffer */
		0,									/* no auxiliary buffer */
		PFD_MAIN_PLANE,						/* main layer */
		0,									/* reserved */
		0, 0, 0								/* layer masks ignored */
	};

	src.cColorBits	= (BYTE)colorbits;
	src.cDepthBits	= (BYTE)depthbits;
	src.cStencilBits = (BYTE)stencilbits;

	if ( stereo ) {
		ri.Printf( GLW_PRINT_ALL, "...attempting to use stereo\n" );
		src.dwFlags |= PFD_STEREO;
		glConfig_stereoEnabled = qtrue;
	} else {
		glConfig_stereoEnabled = qfalse;
	}

	*pPFD = src;
}

#define MAX_PFDS 1024

/* ---- GLW_ChoosePFD  0x00507AA0 ----  VERIFIED */
static int GLW_ChoosePFD( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD ) {
	PIXELFORMATDESCRIPTOR	pfds[MAX_PFDS + 1];
	int		maxPFD = 0;
	int		i;
	int		bestMatch = 0;

	ri.Printf( GLW_PRINT_ALL, "...GLW_ChoosePFD( %d, %d, %d )\n",
		(int)pPFD->cColorBits, (int)pPFD->cDepthBits, (int)pPFD->cStencilBits );

	maxPFD = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );
	if ( maxPFD > MAX_PFDS ) {
		ri.Printf( GLW_PRINT_WARNING, "...numPFDs > MAX_PFDS (%d > %d)\n",
			maxPFD, MAX_PFDS );
		maxPFD = MAX_PFDS;
	}

	ri.Printf( GLW_PRINT_ALL, "...%d PFDs found\n", maxPFD - 1 );

	for ( i = 1; i <= maxPFD; i++ ) {
		DescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[i] );
	}

	for ( i = 1; i <= maxPFD; i++ ) {
		if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) {
			if ( !RCVAR( glw_allowSoftwareGL )->integer ) {
				if ( RCVAR( r_verbose )->integer ) {
					ri.Printf( GLW_PRINT_ALL,
						"...PFD %d rejected, software acceleration\n", i );
				}
				continue;
			}
		}

		if ( RCVAR( r_verbose )->integer ) {
			ri.Printf( GLW_PRINT_ALL,
				"ChoosePFD: format %i: %i color %i depth %i stencil\n",
				i, (int)pfds[i].cColorBits, (int)pfds[i].cDepthBits,
				(int)pfds[i].cStencilBits );
		}

		if ( pfds[i].iPixelType != PFD_TYPE_RGBA ) {
			if ( RCVAR( r_verbose )->integer ) {
				ri.Printf( GLW_PRINT_ALL, "...PFD %d rejected, not RGBA\n", i );
			}
			continue;
		}

		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) {
			if ( RCVAR( r_verbose )->integer ) {
				ri.Printf( GLW_PRINT_ALL,
					"...PFD %d rejected, improper flags (%x instead of %x)\n",
					i, pfds[i].dwFlags, pPFD->dwFlags );
			}
			continue;
		}

		if ( pfds[i].cDepthBits < 15 ) {
			if ( RCVAR( r_verbose )->integer ) {
				ri.Printf( GLW_PRINT_ALL,
					"...PFD %d rejected, insufficient depth bits (%d instead of %d)\n",
					i, (int)pfds[i].cDepthBits, (int)pPFD->cDepthBits );
			}
			continue;
		}

		if ( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) ) {
			if ( RCVAR( r_verbose )->integer ) {
				ri.Printf( GLW_PRINT_ALL,
					"...PFD %d rejected, insufficient stencil bits (%d instead of %d)\n",
					i, (int)pfds[i].cStencilBits, (int)pPFD->cStencilBits );
			}
			continue;
		}

		if ( bestMatch ) {
			/* 0x00507CB8: retail tests PFD_STEREO (0x2) here, as Q3's two checks */
			if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) &&
				 ( pPFD->dwFlags & PFD_STEREO ) ) {
				bestMatch = i;
				continue;
			}

			if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) &&
				 ( pPFD->dwFlags & PFD_STEREO ) ) {
				bestMatch = i;
				continue;
			}

			if ( pfds[bestMatch].cColorBits != pPFD->cColorBits ) {
				if ( pfds[i].cColorBits == pPFD->cColorBits ) {
					bestMatch = i;
					continue;
				} else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits ) {
					bestMatch = i;
					continue;
				}
			}

			if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits ) {
				if ( pfds[i].cDepthBits == pPFD->cDepthBits ) {
					bestMatch = i;
					continue;
				} else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits ) {
					bestMatch = i;
					continue;
				}
			}

			if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits ) {
				if ( pfds[i].cStencilBits == pPFD->cStencilBits ) {
					bestMatch = i;
					continue;
				} else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) &&
							( pPFD->cStencilBits > 0 ) ) {
					bestMatch = i;
					continue;
				}
			}
		} else {
			bestMatch = i;
		}
	}

	if ( !bestMatch ) {
		return 0;
	}

	if ( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) {
		if ( !RCVAR( glw_allowSoftwareGL )->integer ) {
			ri.Printf( GLW_PRINT_ALL, "...no hardware acceleration found\n" );
			return 0;
		} else {
			ri.Printf( GLW_PRINT_ALL, "...using software emulation\n" );
		}
	} else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED ) {
		ri.Printf( GLW_PRINT_ALL, "...MCD acceleration found\n" );
	} else {
		ri.Printf( GLW_PRINT_ALL, "...hardware acceleration found\n" );
	}

	*pPFD = pfds[bestMatch];

	return bestMatch;
}

/* ---- GLW_GetString  0x00507EA0 ----  VERIFIED */
static qboolean GLW_GetString( void ) {
	if ( !QGL_GetString( GL_VENDOR ) ) {
		ri.Printf( GLW_PRINT_ALL, "glGetString(GL_VENDOR) returned NULL\n" );
		return qfalse;
	}
	if ( !QGL_GetString( GL_RENDERER ) ) {
		ri.Printf( GLW_PRINT_ALL, "glGetString(GL_RENDERER) returned NULL\n" );
		return qfalse;
	}
	if ( !QGL_GetString( GL_VERSION ) ) {
		ri.Printf( GLW_PRINT_ALL, "glGetString(GL_VERSION) returned NULL\n" );
		return qfalse;
	}
	if ( !QGL_GetString( GL_EXTENSIONS ) ) {
		ri.Printf( GLW_PRINT_ALL, "glGetString(GL_EXTENSIONS) returned NULL\n" );
		return qfalse;
	}
	return qtrue;
}

/* ---- GLW_MakeContext  0x00507F30 ----  VERIFIED */
static int GLW_MakeContext( PIXELFORMATDESCRIPTOR *pPFD ) {
	int		pixelformat;

	if ( !glw_state.pixelFormatSet ) {
		pixelformat = GLW_ChoosePFD( glw_state.hDC, pPFD );
		if ( !pixelformat ) {
			ri.Printf( GLW_PRINT_ALL, "...GLW_ChoosePFD failed\n" );
			return 1;
		}
		ri.Printf( GLW_PRINT_ALL, "...PIXELFORMAT %d selected\n", pixelformat );

		DescribePixelFormat( glw_state.hDC, pixelformat,
			sizeof( PIXELFORMATDESCRIPTOR ), pPFD );

		if ( !SetPixelFormat( glw_state.hDC, pixelformat, pPFD ) ) {
			ri.Printf( GLW_PRINT_ALL, "...SetPixelFormat failed\n", glw_state.hDC );
			return 1;
		}
		glw_state.pixelFormatSet = qtrue;
	}

	if ( !glw_state.hGLRC ) {
		ri.Printf( GLW_PRINT_ALL, "...creating GL context: " );
		glw_state.hGLRC = qwglCreateContext( glw_state.hDC );
		if ( !glw_state.hGLRC ) {
			ri.Printf( GLW_PRINT_ALL, "failed\n" );
			return 2;
		}
		ri.Printf( GLW_PRINT_ALL, "succeeded\n" );

		ri.Printf( GLW_PRINT_ALL, "...making context current: " );
		if ( !qwglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
			qwglDeleteContext( glw_state.hGLRC );
			glw_state.hGLRC = NULL;
			ri.Printf( GLW_PRINT_ALL, "failed\n" );
			return 2;
		}
		ri.Printf( GLW_PRINT_ALL, "succeeded\n" );

		if ( !GLW_GetString() ) {
			qwglMakeCurrent( NULL, NULL );
			qwglDeleteContext( glw_state.hGLRC );
			glw_state.hGLRC = NULL;
			return 2;
		}
	}
	return 0;
}

/* ---- GLW_InitDriver  0x005080A0 ----  VERIFIED */
static qboolean GLW_InitDriver( const char *drivername, int colorbits ) {
	int		tpfd;
	int		depthbits, stencilbits;
	int		maxTextureSize;

	ri.Printf( GLW_PRINT_ALL, "Initializing OpenGL driver\n" );

	if ( !glw_state.hDC ) {
		ri.Printf( GLW_PRINT_ALL, "...getting DC: " );
		glw_state.hDC = GetDC( (HWND)g_wv_hWnd );
		if ( !glw_state.hDC ) {
			ri.Printf( GLW_PRINT_ALL, "failed\n" );
			return qfalse;
		}
		ri.Printf( GLW_PRINT_ALL, "succeeded\n" );
	}

	if ( !colorbits ) {
		colorbits = glw_state.desktopBitsPixel;
	}

	if ( !RCVAR( r_depthbits )->integer ) {
		if ( colorbits > 16 ) {
			depthbits = 24;
		} else {
			depthbits = 16;
		}
	} else {
		depthbits = RCVAR( r_depthbits )->integer;
	}

	stencilbits = RCVAR( r_stencilbits )->integer;
	if ( depthbits < 24 ) {
		stencilbits = 0;
	}

	if ( !glw_state.pixelFormatSet ) {
		GLW_CreatePFD( &glw_pfd, colorbits, depthbits, stencilbits, qfalse );
		if ( ( tpfd = GLW_MakeContext( &glw_pfd ) ) != 0 ) {
			if ( tpfd == 2 ) {
				ri.Printf( GLW_PRINT_WARNING, "...failed hard\n" );
				return qfalse;
			}
			if ( RCVAR( r_colorbits )->integer == glw_state.desktopBitsPixel &&
				 stencilbits == 0 ) {
				ReleaseDC( (HWND)g_wv_hWnd, glw_state.hDC );
				glw_state.hDC = NULL;
				ri.Printf( GLW_PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );
				return qfalse;
			}
			if ( colorbits > glw_state.desktopBitsPixel ) {
				colorbits = glw_state.desktopBitsPixel;
			}
			GLW_CreatePFD( &glw_pfd, colorbits, depthbits, 0, qfalse );
			if ( GLW_MakeContext( &glw_pfd ) != 0 ) {
				if ( glw_state.hDC ) {
					ReleaseDC( (HWND)g_wv_hWnd, glw_state.hDC );
					glw_state.hDC = NULL;
				}
				ri.Printf( GLW_PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );
				return qfalse;
			}
		}
	}

	QGL_GetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTextureSize );
	if ( glConfig_vidWidth > maxTextureSize || glConfig_vidHeight > maxTextureSize ) {
		if ( glw_state.hGLRC ) {
			qwglMakeCurrent( NULL, NULL );
			qwglDeleteContext( glw_state.hGLRC );
			glw_state.hGLRC = NULL;
		}
		if ( glw_state.hDC ) {
			ReleaseDC( (HWND)g_wv_hWnd, glw_state.hDC );
			glw_state.hDC = NULL;
		}
		glw_state.pixelFormatSet = qfalse;
		ri.Printf( GLW_PRINT_ALL,
			"video card cannot set mode %ix%i because GL_MAX_TEXTURE_SIZE is only %i\n",
			glConfig_vidWidth, glConfig_vidHeight, maxTextureSize );
		return qfalse;
	}

	glConfig_colorBits   = glw_pfd.cColorBits;
	glConfig_depthBits   = glw_pfd.cDepthBits;
	glConfig_stencilBits = glw_pfd.cStencilBits;
	return qtrue;
}

/* ---- GLW_CreateWindow  0x005082E0 ----  VERIFIED */
static qboolean GLW_CreateWindow( const char *drivername, int width, int height,
								  int colorbits, qboolean cdsFullscreen ) {
	RECT		r;
	WNDCLASS	wc;
	DWORD		stylebits, exstyle;
	int			x, y, w, h;
	cvar_t		*vid_xpos, *vid_ypos;

	if ( !s_classRegistered ) {
		memset( &wc, 0, sizeof( wc ) );

		wc.style			= 0;
		wc.lpfnWndProc		= glw_state.wndproc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= (HINSTANCE)g_wv_hInstance;
		wc.hIcon			= LoadIcon( (HINSTANCE)g_wv_hInstance,
										MAKEINTRESOURCE( 1 ) );
		wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground	= CreateSolidBrush( RGB( 0, 0, 0 ) );
		wc.lpszMenuName		= NULL;
		wc.lpszClassName	= WINDOW_CLASS_NAME;

		if ( !RegisterClass( &wc ) ) {
			ri.Error( 0, "EXE_ERR_COULDNT_REGISTER_WINDOW" );
		}
		s_classRegistered = qtrue;
		ri.Printf( GLW_PRINT_ALL, "...registered window class\n" );
	}

	r.left	 = 0;
	r.top	 = 0;
	r.right	 = width;
	r.bottom = height;

	if ( cdsFullscreen ) {
		exstyle		= WS_EX_TOPMOST;
		stylebits	= WS_POPUP | WS_VISIBLE | WS_SYSMENU;	/* 0x90080000 */
	} else {
		exstyle		= 0;
		stylebits	= WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE |
					  WS_SYSMENU;							/* 0x10C80000 */
		AdjustWindowRect( &r, 0x10C80000, FALSE );
	}

	w = r.right - r.left;
	h = r.bottom - r.top;

	if ( cdsFullscreen ) {
		x = 0;
		y = 0;
	} else {
		vid_xpos = RCVAR( ri.Cvar_Get( "vid_xpos", "", 0 ) );
		vid_ypos = RCVAR( ri.Cvar_Get( "vid_ypos", "", 0 ) );
		x = vid_xpos->integer;
		y = vid_ypos->integer;

		if ( x < 0 ) { x = 0; }
		if ( y < 0 ) { y = 0; }

		if ( w < glw_state.desktopWidth && h < glw_state.desktopHeight ) {
			if ( x + w > glw_state.desktopWidth )  { x = glw_state.desktopWidth  - w; }
			if ( y + h > glw_state.desktopHeight ) { y = glw_state.desktopHeight - h; }
		}
	}

	if ( !g_wv_hWnd ) {
		g_wv_hWnd = (int)CreateWindowEx( exstyle, WINDOW_CLASS_NAME,
			WINDOW_CLASS_NAME, stylebits, x, y, w, h,
			NULL, NULL, (HINSTANCE)g_wv_hInstance, NULL );

		if ( !g_wv_hWnd ) {
			ri.Error( 0, "EXE_ERR_COULDNT_CREATE_WINDOW" );
		}
		ShowWindow( (HWND)g_wv_hWnd, SW_SHOW );
		UpdateWindow( (HWND)g_wv_hWnd );
		ri.Printf( GLW_PRINT_ALL, "...created window@%d,%d (%dx%d)\n",
			x, y, w, h );
	} else {
		ri.Printf( GLW_PRINT_ALL,
			"...window already present, CreateWindowEx skipped\n" );
		MoveWindow( (HWND)g_wv_hWnd, x, y, w, h, FALSE );
		ri.Printf( GLW_PRINT_ALL, "...moved window to %d,%d (%dx%d)\n",
			x, y, w, h );
	}

	if ( !GLW_InitDriver( drivername, colorbits ) ) {
		ShowWindow( (HWND)g_wv_hWnd, SW_HIDE );
		DestroyWindow( (HWND)g_wv_hWnd );
		g_wv_hWnd = 0;
		return qfalse;
	}

	SetForegroundWindow( (HWND)g_wv_hWnd );
	SetFocus( (HWND)g_wv_hWnd );
	if ( g_splashWnd ) {
		ShowWindow( (HWND)g_splashWnd, SW_HIDE );
	}
	return qtrue;
}

/* ---- GLW_SetMode  0x00508630 ----  VERIFIED */
static int GLW_SetMode( const char *drivername, int mode, int colorbits, qboolean cdsFullscreen ) {
	HDC			hDC;
	int			cdsRet;
	DEVMODE		dm;

	ri.Printf( GLW_PRINT_ALL, "...setting mode %d:", mode );
	if ( !R_GetModeInfo( &glConfig_vidWidth, &glConfig_vidHeight, &glConfig_windowAspect, mode ) ) {
		ri.Printf( GLW_PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( GLW_PRINT_ALL, " %d %d %s\n", glConfig_vidWidth, glConfig_vidHeight,
		cdsFullscreen
			? ( RCVAR( r_displayRefresh )->integer
					? va( "FS (%i Hz)", RCVAR( r_displayRefresh )->integer )
					: "FS" )
			: "W" );

	hDC = GetDC( GetDesktopWindow() );
	glw_state.desktopBitsPixel = GetDeviceCaps( hDC, BITSPIXEL );
	glw_state.desktopWidth     = GetDeviceCaps( hDC, HORZRES );
	glw_state.desktopHeight    = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	if ( glw_state.desktopBitsPixel < 15 || glw_state.desktopBitsPixel == 24 ) {
		/* 0x00508742: warn on colorbits 0, or windowed with colorbits >= 15;
		 * MB_OKCANCEL | MB_ICONEXCLAMATION (0x31), only IDOK continues. */
		if ( colorbits == 0 || ( !cdsFullscreen && colorbits >= 15 ) ) {
			if ( MessageBox( NULL,
					(const char *)SEH_GetLocalizedString_m( "WIN_COLORDEPTH_WARN_BODY" ),
					(const char *)SEH_GetLocalizedString_m( "WIN_COLORDEPTH_WARN_TITLE" ),
					MB_OKCANCEL | MB_ICONEXCLAMATION ) != IDOK ) {
				return RSERR_INVALID_MODE;
			}
		}
	}

	if ( cdsFullscreen ) {
		memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );

		dm.dmPelsWidth  = glConfig_vidWidth;
		dm.dmPelsHeight = glConfig_vidHeight;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( RCVAR( r_displayRefresh )->integer != 0 ) {
			dm.dmDisplayFrequency = RCVAR( r_displayRefresh )->integer;
			dm.dmFields |= DM_DISPLAYFREQUENCY;
		}

		if ( colorbits != 0 ) {
			dm.dmBitsPerPel = colorbits;
			dm.dmFields |= DM_BITSPERPEL;
			ri.Printf( GLW_PRINT_ALL, "...using colorbits of %d\n", colorbits );
		} else {
			ri.Printf( GLW_PRINT_ALL, "...using desktop display depth of %d\n",
				glw_state.desktopBitsPixel );
		}

		if ( glw_state.cdsFullscreen ) {
			ri.Printf( GLW_PRINT_ALL, "...already fullscreen, avoiding redundant CDS\n" );

			if ( !GLW_CreateWindow( drivername, glConfig_vidWidth, glConfig_vidHeight, colorbits, qtrue ) ) {
				ri.Printf( GLW_PRINT_ALL, "...restoring display settings\n" );
				ChangeDisplaySettings( 0, 0 );
				return RSERR_INVALID_MODE;
			}
		} else {
			ri.Printf( GLW_PRINT_ALL, "...calling CDS: " );

			if ( ( cdsRet = ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
				ri.Printf( GLW_PRINT_ALL, "ok\n" );

				if ( !GLW_CreateWindow( drivername, glConfig_vidWidth, glConfig_vidHeight, colorbits, qtrue ) ) {
					ri.Printf( GLW_PRINT_ALL, "...restoring display settings\n" );
					ChangeDisplaySettings( 0, 0 );
					return RSERR_INVALID_MODE;
				}

				glw_state.cdsFullscreen = qtrue;
			} else {
				DEVMODE		devmode;
				int			modeNum;



				ri.Printf( GLW_PRINT_ALL, "failed, " );

				PrintCDSError( cdsRet );

				ri.Printf( GLW_PRINT_ALL, "...trying next higher resolution:" );

				for ( modeNum = 0; ; modeNum++ ) {
					if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
						modeNum = -1;
						break;
					}
					if ( devmode.dmPelsWidth >= (DWORD)glConfig_vidWidth
						&& devmode.dmPelsHeight >= (DWORD)glConfig_vidHeight
						&& devmode.dmBitsPerPel >= 15 ) {
						break;
					}
				}

				if ( modeNum != -1 &&
					 ( cdsRet = ChangeDisplaySettings( &devmode, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
					ri.Printf( GLW_PRINT_ALL, " ok\n" );
					if ( !GLW_CreateWindow( drivername, glConfig_vidWidth, glConfig_vidHeight, colorbits, qtrue ) ) {
						ri.Printf( GLW_PRINT_ALL, "...restoring display settings\n" );
						ChangeDisplaySettings( 0, 0 );
						return RSERR_INVALID_MODE;
					}
					glw_state.cdsFullscreen = qtrue;
				} else {
					ri.Printf( GLW_PRINT_ALL, " failed, " );

					PrintCDSError( cdsRet );

					ri.Printf( GLW_PRINT_ALL, "...restoring display settings\n" );
					ChangeDisplaySettings( 0, 0 );

					glw_state.cdsFullscreen = qfalse;
					glConfig_isFullscreen = qfalse;
					if ( !GLW_CreateWindow( drivername, glConfig_vidWidth, glConfig_vidHeight, colorbits, qfalse ) ) {
						return RSERR_INVALID_MODE;
					}
					if ( g_wv_hWnd ) {
						if ( glw_state.hGLRC ) {
							qwglMakeCurrent( NULL, NULL );
							qwglDeleteContext( glw_state.hGLRC );
							glw_state.hGLRC = NULL;
						}
						ShowWindow( (HWND)g_wv_hWnd, SW_HIDE );
						DestroyWindow( (HWND)g_wv_hWnd );
						g_wv_hWnd = 0;
						glw_state.pixelFormatSet = qfalse;
					}
					return RSERR_INVALID_FULLSCREEN;
				}
			}
		}
	} else {
		if ( glw_state.cdsFullscreen ) {
			ChangeDisplaySettings( 0, 0 );
		}

		glw_state.cdsFullscreen = qfalse;
		if ( !GLW_CreateWindow( drivername, glConfig_vidWidth, glConfig_vidHeight, colorbits, qfalse ) ) {
			return RSERR_INVALID_MODE;
		}
	}

	memset( &dm, 0, sizeof( dm ) );
	dm.dmSize = sizeof( dm );
	if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ) ) {
		glConfig_displayFrequency = dm.dmDisplayFrequency;
	}

	glConfig_isFullscreen = cdsFullscreen;

	return RSERR_OK;
}

/* ---- GLW_SetModeAndReport  0x00507A60 ----
 * Q3's GLW_StartDriverAndSetMode (retail sub_507A60); retail also inlines it into the function below. */
static qboolean GLW_SetModeAndReport( const char *drivername, int mode, int colorbits,
									  qboolean cdsFullscreen ) {
	int		err;

	err = GLW_SetMode( drivername, mode, colorbits, cdsFullscreen );
	switch ( err ) {
	case RSERR_INVALID_FULLSCREEN:
		ri.Printf( GLW_PRINT_ALL,
			"...WARNING: fullscreen unavailable in this mode\n" );
		return qfalse;
	case RSERR_INVALID_MODE:
		ri.Printf( GLW_PRINT_ALL,
			"...WARNING: could not set the given mode (%d)\n", mode );
		return qfalse;
	default:
		break;
	}
	return qtrue;
}

/* ---- GLW_StartDriverAndSetMode  0x0050A510 ----  VERIFIED */
static qboolean GLW_StartDriverAndSetMode( const char *drivername ) {
	char	buffer[1024];
	char	*p;

	strncpy( buffer, drivername, sizeof( buffer ) - 1 );
	buffer[sizeof( buffer ) - 1] = '\0';
	for ( p = buffer; *p; p++ ) {
		*p = (char)tolower( *p );
	}

	if ( QCL_Init( buffer ) ) {
		if ( GLW_SetModeAndReport( drivername, RCVAR( r_mode )->integer,
								   RCVAR( r_colorbits )->integer,
								   RCVAR( r_fullscreen )->integer ) ) {
			return qtrue;
		}
	}
	QGL_Shutdown();
	return qfalse;
}

/* ---- GLW_TryFallbackMode  0x0050A730 ----
 * Retail passes mode in eax; only modes below the current r_mode are tried. */
static qboolean GLW_TryFallbackMode( int mode, const char *name ) {
	char	buf[4];

	if ( RCVAR( r_mode )->integer > mode ) {
		_itoa( mode, buf, 10 );
		ri.Printf( GLW_PRINT_ALL,
			"Forcing %s resolution to allow OpenGL to run in fullscreen\n", name );
		ri.Cvar_Set( "r_mode", buf );
		ri.Cvar_Get( "r_mode", "3", 0x20 );
		if ( GLW_StartDriverAndSetMode( "opengl32" ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

/* ---- GLW_LoadOpenGL  0x0050A7A0 ----
 * Retries with r_displayRefresh forced to 0, then r_mode 9/6/4/3, and raises
 * EXE_ERR_COULDNT_LOAD_OPENGL only when every attempt has failed. */
static void GLW_LoadOpenGL( void ) {
	if ( GLW_StartDriverAndSetMode( "opengl32" ) ) {
		return;
	}
	if ( RCVAR( r_displayRefresh )->integer ) {
		ri.Printf( GLW_PRINT_ALL,
			"Forcing default value for r_displayRefresh to allow OpenGL to run in fullscreen\n" );
		ri.Cvar_Set( "r_displayRefresh", "0" );
		ri.Cvar_Get( "r_displayRefresh", "0", 0x20 );
		if ( GLW_StartDriverAndSetMode( "opengl32" ) ) {
			return;
		}
	}
	if ( GLW_TryFallbackMode( 9, "1600x1200" ) ) {
		return;
	}
	if ( GLW_TryFallbackMode( 6, "1024x768" ) ) {
		return;
	}
	if ( GLW_TryFallbackMode( 4, "800x600" ) ) {
		return;
	}
	if ( GLW_TryFallbackMode( 3, "640x480" ) ) {
		return;
	}
	ri.Error( 0, "EXE_ERR_COULDNT_LOAD_OPENGL" );
}

#define GLW_CSV_MAX_CVARS   1024
#define GLW_CSV_MAX_VALUE   64

/* ---- GLW_ApplyGLCSV  0x0050A870 ----
 * gl.csv hardware auto-config.  Row 1 is "renderer" followed by cvar names; every
 * later row starts with a GL_RENDERER prefix.  The row with the longest prefix that
 * matches (case-insensitive) sets each column's cvar.  Retail inlines Com_ParseOnLine,
 * Com_Parse, Com_SkipRestOfLine and Com_EndParseSession here.  The renderer byte is
 * compared unsigned and the token byte signed (movzx / movsx at 0x0050AA06 / 0x0050AA10). */
static void GLW_ApplyGLCSV( void ) {
	cvar_t	*cvars[GLW_CSV_MAX_CVARS];
	char	vals[GLW_CSV_MAX_CVARS][GLW_CSV_MAX_VALUE];
	char	*buf;
	char	*data_p;
	char	*tok;
	int		numCvars;
	int		bestLen;
	int		len;
	int		c;
	size_t	valLen;

	if ( r_skip_auto_config->integer ) {
		return;
	}
	if ( ri.FS_ReadFile( "gl.csv", (void **)&buf ) < 0 ) {
		return;
	}

	Com_BeginParseSession( "gl.csv" );
	parseInfo->csv = qtrue;
	data_p = buf;

	tok = Com_Parse( &data_p );
	if ( !*tok || _stricmp( tok, "renderer" ) ) {
		ri.Error( 0, "first column of gl.csv must be called 'renderer'" );
	}

	numCvars = 0;
	for ( ;; ) {
		tok = Com_ParseOnLine( &data_p );
		if ( !*tok ) {
			break;
		}
		if ( (unsigned)numCvars >= GLW_CSV_MAX_CVARS ) {
			ri.Error( 0, "more than %i cvars in gl.csv", GLW_CSV_MAX_CVARS );
		}
		cvars[numCvars] = (cvar_t *)ri.Cvar_FindVar( tok );
		if ( !cvars[numCvars] ) {
			ri.Error( 0, "cvar %s mentioned in gl.csv does not exist", tok );
		}
		numCvars++;
	}

	bestLen = 0;
	for ( ;; ) {
		tok = Com_Parse( &data_p );
		if ( !*tok ) {
			break;
		}
		for ( len = 0; tok[len]; len++ ) {
			if ( tolower( tok[len] ) !=
				 tolower( ( (const unsigned char *)glConfig_renderer_string )[len] ) ) {
				goto nextRow;	/* a mismatch skips the row without re-testing tok[len] */
			}
		}
		if ( bestLen < len ) {
			bestLen = len;
			for ( c = 0; c < numCvars; c++ ) {
				tok = Com_ParseOnLine( &data_p );
				valLen = strlen( tok );
				if ( valLen >= GLW_CSV_MAX_VALUE ) {
					ri.Error( 0, "cvar '%s' value '%s' len %i > %i",
						cvars[c]->name, tok, (int)valLen, GLW_CSV_MAX_VALUE - 1 );
				}
				memcpy( vals[c], tok, valLen + 1 );
			}
		}
nextRow:
		Com_SkipRestOfLine( &data_p );
	}

	Com_EndParseSession();
	ri.FS_FreeFile( buf );

	if ( bestLen > 0 ) {
		for ( c = 0; c < numCvars; c++ ) {
			ri.Cvar_Set( cvars[c]->name, vals[c] );
			ri.Cvar_Get( cvars[c]->name, cvars[c]->resetString, cvars[c]->flags );
		}
	}
}

/* ---- GLimp_Init  0x0050ABF0 ----  VERIFIED */
void GLimp_Init( void ) {
	cvar_t	*lastValidRenderer;
	cvar_t	*cv;

	lastValidRenderer = (cvar_t *)ri.Cvar_Get( "r_lastValidRenderer",
											   "(uninitialized)", 1 );

	ri.Printf( GLW_PRINT_ALL, "Initializing OpenGL subsystem\n" );

	if ( !GLW_CheckOSVersion() ) {
		ri.Error( 0, "EXE_ERR_BAD_WINDOWS_VER" );
	}

	cv = (cvar_t *)ri.Cvar_Get( "win_hinstance", "", 0 );
	sscanf( cv->string, "%i", (int *)&g_wv_hInstance );

	cv = (cvar_t *)ri.Cvar_Get( "win_wndproc", "", 0 );
	sscanf( cv->string, "%i", (int *)&glw_state.wndproc );

	glw_allowSoftwareGL = (int)ri.Cvar_Get( "r_allowSoftwareGL", "0", 0x20 );

	GLW_LoadOpenGL();

	glConfig_vendor_string     = (char *)QGL_GetString( GL_VENDOR );
	glConfig_renderer_string   = (char *)QGL_GetString( GL_RENDERER );
	glConfig_version_string    = (char *)QGL_GetString( GL_VERSION );
	glConfig_extensions_string = (char *)QGL_GetString( GL_EXTENSIONS );

	/* 0x0050ACE3 / 0x0050ACE8: the result goes to the driver slot and the pointer. */
	dllwglGetExtensionsStringEXT = (const char * (WINAPI *)( void ))
		qwglGetProcAddress( "wglGetExtensionsStringEXT" );
	qwglGetExtensionsStringEXT = dllwglGetExtensionsStringEXT;
	if ( qwglGetExtensionsStringEXT ) {
		glConfig_wextensions_string = (char *)qwglGetExtensionsStringEXT();
	}

	GLW_ApplyGLCSV();

	/* 0x0050ACFE / 0x0050AD08: a NULL on either side counts as a renderer change
	   and jumps straight to the r_textureMode reset. */
	if ( !lastValidRenderer->string || !glConfig_renderer_string ||
		 Q_stricmpn( lastValidRenderer->string,
					 (const char *)glConfig_renderer_string, 99999 ) ) {
		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
	}
	ri.Cvar_Set( "r_lastValidRenderer", (const char *)glConfig_renderer_string );

	GLW_InitExtensions();
	WG_CheckHardwareGamma();
}

/* ---- GLimp_EndFrame  0x0050A610 ----  VERIFIED */
void GLimp_EndFrame( void ) {
	if ( RCVAR( r_swapInterval )->modified ) {
		RCVAR( r_swapInterval )->modified = qfalse;
		if ( !glConfig_stereoEnabled && qwglSwapIntervalEXT ) {
			qwglSwapIntervalEXT( RCVAR( r_swapInterval )->integer );
		}
	}

	if ( !RCVAR( r_drawBuffer )->string ||
		 Q_stricmpn( RCVAR( r_drawBuffer )->string, "GL_FRONT", 99999 ) ) {
		if ( glConfig_NVVertexArrayRange ) {
			qglFlushVertexArrayRangeNV();
		}
		SwapBuffers( glw_state.hDC );
	}

	/* 0x0050A679: runs every frame, GL_FRONT included (je 0x0050A679 at 0x0050A65B).
	   r_logFile->integer is read once (esi at 0x0050A685) and feeds all three calls. */
	{
		int logFile = r_logFile->integer;

		QGL_EnableDrawProfiling( ( !logFile && !r_debugGLErrors->integer &&
								   r_profileDrawElements->integer ) ? qtrue : qfalse );
		QGL_EnableErrorChecking( ( !logFile && r_debugGLErrors->integer ) ? qtrue : qfalse );
		QGL_EnableLogging( logFile );
	}
}

/* ---- GLimp_Shutdown  0x0050AD50 ---- */
void GLimp_Shutdown( void ) {
	const char			*success[] = { "failed", "success" };
	int					retVal;

	if ( !qwglMakeCurrent ) {
		return;
	}

	ri.Printf( GLW_PRINT_ALL, "Shutting down OpenGL subsystem\n" );

	WG_RestoreGamma();

	if ( qwglMakeCurrent ) {
		retVal = qwglMakeCurrent( NULL, NULL ) != 0;
		ri.Printf( GLW_PRINT_ALL, "...wglMakeCurrent( NULL, NULL ): %s\n",
			success[retVal] );
	}

	if ( glw_state.hGLRC ) {
		retVal = qwglDeleteContext( glw_state.hGLRC ) != 0;
		ri.Printf( GLW_PRINT_ALL, "...deleting GL context: %s\n",
			success[retVal] );
		glw_state.hGLRC = NULL;
	}

	if ( glw_state.hDC ) {
		retVal = ReleaseDC( (HWND)g_wv_hWnd, glw_state.hDC ) != 0;
		ri.Printf( GLW_PRINT_ALL, "...releasing DC: %s\n", success[retVal] );
		glw_state.hDC = NULL;
	}

	if ( g_wv_hWnd ) {
		ri.Printf( GLW_PRINT_ALL, "...destroying window\n" );
		ShowWindow( (HWND)g_wv_hWnd, SW_HIDE );
		DestroyWindow( (HWND)g_wv_hWnd );
		g_wv_hWnd = 0;
		glw_state.pixelFormatSet = qfalse;

		if ( !com_dedicated->integer && g_splashWnd ) {
			ShowWindow( (HWND)g_splashWnd, SW_SHOW );
			UpdateWindow( (HWND)g_splashWnd );
		}
	}

	/* 0x019C0004, the QGL log file, spelled `Stream` as at the renderer's logging sites. */
	if ( Stream ) {
		fclose( Stream );
		Stream = NULL;
	}

	if ( glw_state.cdsFullscreen ) {
		ri.Printf( GLW_PRINT_ALL, "...resetting display\n" );
		ChangeDisplaySettings( NULL, 0 );
		glw_state.cdsFullscreen = qfalse;
	}

	QGL_Shutdown();

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );
}
