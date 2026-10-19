/*
 * @fidelity: verified
 *
 * tr_allocator.c -- back-end vertex memory allocators.
 *
 * The retail renderer keeps two static vertex-memory pools -- a "primary"
 * (video-memory) pool and a "secondary" (AGP) pool -- plus a hunk fallback, and
 * one dynamic ARB stream buffer for the tessellator.  R_InitAllocators sizes and
 * allocates those pools from the r_mem_* cvars (or auto-tuned defaults) using the
 * best path the GL driver exposes (NV_vertex_array_range, ATI_vertex_array_object
 * or ARB_vertex_buffer_object), and picks the matching RB_StageIteratorGeneric*
 * back end.  R_AllocMemoryNV / R_AllocMemoryATI are the per-request pool
 * sub-allocators (32-byte aligned); R_CreateBufferARB / R_DeleteBuffersARB manage
 * ARB buffer objects; R_ShutdownAllocators frees the pools and R_MemInfo_f
 * ("r_meminfo") prints their utilisation.
 *
 * Nothing here is passed by pointer except the two out-parameters of the pool
 * sub-allocators (a void** / int*), so this file defines no new record types --
 * all state lives in the global tess_, glConfig_ and glState_ symbols reached
 * through tr_local.h and qgl.h.  The readable tess_ pool names below are
 * aliases onto those globals; offsets are noted from the retail layout.
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"
/* ---- renderer-owned globals ---- */
float sys_bytesToMbScale;
/* tr_staticVertexMemory{Secondary,Primary}Pool: tr fields, views in tr_local.h */


extern void *( __stdcall * qwglAllocateMemoryNV )( int size, float readFreq,
												   float writeFreq, float priority );
extern void  ( __stdcall * qwglFreeMemoryNV )( void *pointer );   /* 0x016C4500 */

extern void RB_StageIteratorGeneric();
extern void RB_StageIteratorGenericARB();
extern void RB_StageIteratorGenericNV();
extern void RB_StageIteratorGenericATI();

extern int R_CreateBufferARB();

/* Back-end vertex allocator state, 0x016D8850-0x016D8890 and 0x016D93EC. */
#define tess_agpMemory      tr_staticVertexMemorySecondary
#define tess_agpSize        tr_staticVertexMemorySecondaryLimit
#define tess_agpUsed        tr_staticVertexMemorySecondaryUsed
#define tess_videoMemory    tr_staticVertexMemoryPrimary
#define tess_videoSize      tr_staticVertexMemoryPrimaryLimit
#define tess_videoUsed      tr_staticVertexMemoryPrimaryUsed
#define tess_vertexPath     storageClass
#define tess_vboStreamDraw  tr_vboStreamDraw
#define tess_stageIterate   tr_stageIteratorFunc
#define tess_vbo            backEnd_dynamicBuffer_storage
#define tess_vboSize        backEnd_dynamicBuffer_capacity
#define tess_vboOffset      backEnd_dynamicBuffer_currentOffset

#define GL_ARRAY_BUFFER_ARB           0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB   0x8893
#define GL_DYNAMIC_DRAW_ARB           0x88E8
#define GL_STATIC_ATI                 0x8760
#define GL_DYNAMIC_ATI                0x8761
#define GL_TEXTURE0_ARB               0x84C0

#define QALIGN32( mb )  ( ( 31 - (int)( (mb) * -1048576.0f ) ) & 0xFFFFFFE0 )

/* ---- R_InitAllocators  0x004B8420 ----  VERIFIED
 * Size and allocate the static vertex-memory pools and the dynamic ARB stream
 * buffer, then select the matching back-end stage iterator.
 *
 * Retail passes the *MB float directly to ri.Printf's "%i" specifier (the value
 * is promoted to double and the low dword is consumed), so the counts print as
 * 0 for whole-MB sizes.  The bug is reproduced faithfully by handing the float
 * to "%i" here as well.
 *
 * Retail clears the pools with two 12-byte memsets of the {memory, limit, used}
 * pool structs. */
void R_InitAllocators( void ) {
	float	agpMB, backendMB, videoMB;
	int		size;

	tess_vertexPath   = 3;
	tess_stageIterate = (int (*)())RB_StageIteratorGeneric;

	if ( !r_optimize->integer ) {
		return;
	}

	memset( &tr_staticVertexMemorySecondaryPool, 0, sizeof( tr_staticVertexMemorySecondaryPool ) );
	memset( &tr_staticVertexMemoryPrimaryPool, 0, sizeof( tr_staticVertexMemoryPrimaryPool ) );

	agpMB   = 0.0f;
	videoMB = 0.0f;

	if ( r_mem_manual->integer ) {
		agpMB     = r_mem_agp->value;
		videoMB   = r_mem_video->value;
		backendMB = r_mem_backend->value;

		if ( backendMB < 1.0f ) {
			backendMB = 1.0f;
		} else if ( backendMB > 32.0f ) {
			backendMB = 32.0f;
		}
	} else {
		/* backendMB set in each arm: VC7 hoists it, and the extra reference gives
		 * retail's frame-slot order */
		if ( glConfig_NVVertexArrayRange ) {
			backendMB      = 1.0f;
			agpMB          = 13.0f;
			tess_agpSize   = 13 * 1048576;
			tess_agpMemory = (int)qwglAllocateMemoryNV( 13 * 1048576, 0.0f, 0.0f, 0.5f );
			if ( !tess_agpMemory ) {
				ri.Printf( 0,
					"^3No AGP memory available for video card to use with NV_vertex_array_range.\n"
					"^3This is usually because the motherboard has no AGP drivers installed,\n"
					"^3forcing the video card to use PCI mode.\n" );
				agpMB        = 0.0f;
				videoMB      = 13.0f;
				backendMB    = 1.0f;
				tess_agpSize = 0;
			}
		} else {
			backendMB = 1.0f;
			agpMB     = 1.0f;
			videoMB   = 12.0f;
		}
	}

	if ( videoMB > 0.0f ) {
		size = QALIGN32( videoMB );
		tess_videoSize = size;

		if ( glConfig_NVVertexArrayRange ) {
			tess_videoMemory = (int)qwglAllocateMemoryNV( size, 0.0f, 0.0f, 1.0f );
			if ( tess_videoMemory ) {
				ri.Printf( 0, "Allocated %i MB of video memory for vertex data\n", videoMB );
				tess_videoUsed = 0;
			} else {
				ri.Printf( 2, "Failed to allocate %i MB of video memory for vertex data\n", videoMB );
				tess_videoSize = 0;
			}
		} else if ( glConfig_ATIVertexArrayObject ) {
			tess_videoMemory = qglNewObjectBufferATI( size, 0, GL_STATIC_ATI );
			if ( tess_videoMemory ) {
				ri.Printf( 0, "Allocated %i MB of static buffers\n", videoMB );
				tess_videoUsed = 0;
			} else {
				ri.Printf( 2, "Failed to allocate %i MB of static buffers\n", videoMB );
				tess_videoSize = 0;
			}
		} else {
			tess_videoSize = 0;
		}
	}

	if ( agpMB > 0.0f ) {
		size = QALIGN32( agpMB );
		tess_agpSize = size;

		if ( glConfig_NVVertexArrayRange ) {
			if ( !tess_agpMemory ) {
				tess_agpMemory = (int)qwglAllocateMemoryNV( size, 0.0f, 0.0f, 0.5f );
			}
			if ( tess_agpMemory ) {
				ri.Printf( 0, "Allocated %i MB of AGP memory for vertex data\n", agpMB );
				tess_agpUsed = 0;
			} else {
				ri.Printf( 2, "Failed to allocate %i MB of AGP memory for vertex data\n", agpMB );
				tess_agpSize = 0;
			}
		} else if ( glConfig_ATIVertexArrayObject ) {
			tess_agpMemory = qglNewObjectBufferATI( size, 0, GL_DYNAMIC_ATI );
			if ( tess_agpMemory ) {
				ri.Printf( 0, "Allocated %i MB of dynamic buffers\n", agpMB );
				tess_agpUsed = 0;
			} else {
				ri.Printf( 2, "Failed to allocate %i MB of dynamic buffers\n", agpMB );
				tess_agpSize = 0;
			}
		} else {
			tess_agpSize = 0;
		}
	}

	if ( glConfig_ARBVertexBufferObject ) {
		tess_vbo       = 0;
		tess_vboOffset = 0;
		tess_vboSize   = 0;

		if ( !r_optimizeBackend->integer ) {
			return;
		}
		tess_stageIterate = (int (*)())RB_StageIteratorGenericARB;
		if ( tess_vboStreamDraw ) {
			return;
		}
		size = QALIGN32( backendMB );
		tess_vboSize = size;
		if ( size < 0 ) {
			tess_vboSize = 0;
			return;
		}
		if ( size > 0 ) {
			tess_vbo = R_CreateBufferARB( GL_ARRAY_BUFFER_ARB, size, 0,
										  GL_DYNAMIC_DRAW_ARB );
		}
		return;
	}

	size = QALIGN32( backendMB );
	tess_vboSize   = size;
	tess_vboOffset = 0;
	if ( size < 0 ) {
		tess_vboSize = 0;
		return;
	}
	if ( !r_optimizeBackend->integer ) {
		tess_vboSize = 0;
		return;
	}
	if ( size <= 0 ) {
		return;
	}

	if ( tess_agpSize ) {
		if ( size > tess_agpSize ) {
			size = tess_agpSize;
			tess_vboSize = size;
		}
		tess_agpUsed = size;
		tess_vbo     = tess_agpMemory;
		if ( glConfig_NVVertexArrayRange && glConfig_NVFenceAvailable ) {
			tess_vertexPath   = 2;
			tess_stageIterate = (int (*)())RB_StageIteratorGenericNV;
			return;
		}
	} else {
		if ( !tess_videoSize ) {
			return;
		}
		if ( size > tess_videoSize ) {
			size = tess_videoSize;
			tess_vboSize = size;
		}
		tess_vbo       = tess_videoMemory;
		tess_videoUsed = size;
		if ( glConfig_NVVertexArrayRange && glConfig_NVFenceAvailable ) {
			tess_vertexPath   = 1;
			tess_stageIterate = (int (*)())RB_StageIteratorGenericNV;
			return;
		}
	}

	if ( glConfig_ATIVertexArrayObject ) {
		tess_stageIterate = (int (*)())RB_StageIteratorGenericATI;
	}
}


#define R_VERTEX_POOL_VIDEO	1
#define R_VERTEX_POOL_AGP	2
#define R_VERTEX_POOL_HUNK	3

#define R_VERTEX_ALIGN		32

extern void *Hunk_AllocAlignInternal( int size, int align );

/* ---- R_AllocMemoryNV  0x004B8330 ----  VERIFIED
 * Sub-allocate `size` bytes (32-byte aligned) from the requested pool, writing
 * the block ADDRESS to *memory.  firstPool 1 tries the video pool then falls
 * through to the AGP pool then the hunk; firstPool 2 tries AGP then the hunk;
 * anything else goes straight to the hunk.  The pool test is STRICT (used+size <
 * limit).  Returns the pool that satisfied the request. */
int R_AllocMemoryNV( int firstPool, unsigned int size, void **memory ) {
	unsigned int	aligned = ( size + ( R_VERTEX_ALIGN - 1 ) ) & ~( R_VERTEX_ALIGN - 1 );

	if ( firstPool == R_VERTEX_POOL_VIDEO ) {
		if ( (int)( tess_videoUsed + aligned ) < tess_videoSize ) {
			*memory = (void *)( tess_videoMemory + tess_videoUsed );
			tess_videoUsed += aligned;
			return R_VERTEX_POOL_VIDEO;
		}
	} else if ( firstPool != R_VERTEX_POOL_AGP ) {
		*memory = Hunk_AllocAlignInternal( aligned, R_VERTEX_ALIGN );
		return R_VERTEX_POOL_HUNK;
	}

	if ( (int)( tess_agpUsed + aligned ) < tess_agpSize ) {
		*memory = (void *)( tess_agpMemory + tess_agpUsed );
		tess_agpUsed += aligned;
		return R_VERTEX_POOL_AGP;
	}

	*memory = Hunk_AllocAlignInternal( aligned, R_VERTEX_ALIGN );
	return R_VERTEX_POOL_HUNK;
}

/* ---- R_AllocMemoryATI  0x004B83B0 ----  VERIFIED
 * As R_AllocMemoryNV, but for ATI_vertex_array_object: it writes the block
 * OFFSET into the named buffer (not an address), the pool test is NON-STRICT
 * (used+size <= limit), and there is no hunk fallback -- it returns 0 on
 * failure.  Extern: R_OptimizeSModelSurfATI (tr_staticmodel.c) is its
 * caller, LTCG-inlined there; this out-of-line copy stays (register args). */
int R_AllocMemoryATI( int firstPool, unsigned int size, int *offset ) {
	unsigned int	aligned = ( size + ( R_VERTEX_ALIGN - 1 ) ) & ~( R_VERTEX_ALIGN - 1 );

	if ( firstPool == R_VERTEX_POOL_VIDEO ) {
		if ( (int)( tess_videoUsed + aligned ) <= tess_videoSize ) {
			*offset = tess_videoUsed;
			tess_videoUsed += aligned;
			return R_VERTEX_POOL_VIDEO;
		}
	} else if ( firstPool != R_VERTEX_POOL_AGP ) {
		return 0;
	}

	if ( (int)( tess_agpUsed + aligned ) <= tess_agpSize ) {
		*offset = tess_agpUsed;
		tess_agpUsed += aligned;
		return R_VERTEX_POOL_AGP;
	}

	return 0;
}

/* ---- R_CreateBufferARB  0x004B81B0 ----  VERIFIED
 * Create and fill one ARB buffer object, returning its name (a frame-serial
 * counter, not a qglGenBuffersARB name) or 0 on GL error.
 *
 * Retail takes four arguments (target, size, data, usage; LTCG passes target
 * in ESI); the push-ecx at entry only reserves the 4-byte name slot. */
int R_CreateBufferARB( int target, int size, int data, int usage )
{
	int  name;
	int  serial;
	int  err;

	serial = tr_dynamicBufferFrameSerial;                  /* 0x004B81B1 */
	if ( tr_dynamicBufferFrameSerial < tr_dynamicBufferMaxFrameSerial )
		serial = tr_dynamicBufferMaxFrameSerial;
	tr_dynamicBufferFrameSerial = serial + 1;
	name = serial + 1;

	qglBindBufferARB( target, name );
	qglGetError();
	qglBufferDataARB( target, size, (const GLvoid *)data, usage );
	err = qglGetError();
	qglBindBufferARB( target, 0 );

	if ( err ) {
		qglDeleteBuffersARB( 1, (const GLuint *)&name );
		return 0;
	}
	return name;
}

/* ---- R_DeleteBuffersARB  0x004B8220 ----  VERIFIED
 * Unbind the ARB array/element buffers, detach every texture unit's array
 * pointers, then delete all buffer objects created so far and reset the
 * frame-serial counters.  The int argument is copied to `i` and immediately
 * overwritten by the delete loop, so it is dead; the return value is the last
 * value of `result` (dead EAX). */
int R_DeleteBuffersARB( int unused )
{
	int result;
	int tmu;
	int enumTmu;
	int last;
	int i;

	result = glConfig_ARBVertexBufferObject;
	if ( glConfig_ARBVertexBufferObject )
	{
		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
		qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );

		tmu = glConfig_maxActiveTextures - 1;
		if ( tmu >= 0 )
		{
			enumTmu = tmu + GL_TEXTURE0_ARB;
			do
			{
				if ( glState_currentTmu != tmu )
				{
					qglActiveTextureARB( enumTmu );
					glState_currentTmu = tmu;
				}
				if ( glState_currentClientTmu != tmu )
				{
					qglClientActiveTextureARB( enumTmu );
					glState_currentClientTmu = tmu;
				}
				qglTexCoordPointer( 2, 0x1406u, 0, 0 );
				--tmu;
				--enumTmu;
			}
			while ( tmu >= 0 );
		}
		qglNormalPointer( 0x1406u, 0, 0 );
		qglColorPointer( 4, 0x1401u, 0, 0 );
		qglVertexPointer( 3, 0x1406u, 0, 0 );

		result = tr_dynamicBufferFrameSerial;
		last   = tr_dynamicBufferMaxFrameSerial;
		if ( tr_dynamicBufferFrameSerial > tr_dynamicBufferMaxFrameSerial )
			last = tr_dynamicBufferFrameSerial;
		for ( i = 1; i <= last; ++i )
		{
			qglDeleteBuffersARB( 1, (const GLuint *)&i );
			result = i + 1;
		}
		tr_dynamicBufferFrameSerial    = 0;
		tr_dynamicBufferMaxFrameSerial = 0;
	}
	return result;
}

/* ---- R_ShutdownAllocators  0x004B88B0 ----  VERIFIED
 * Restore the default (system-memory) array pointers if a NV/ATI storage mode
 * was active, then free both static vertex-memory pools.  The texcoord
 * pointer is the VALUE of tess_activeTexCoords[0] (retail `mov reg, [0x01877F60]`,
 * the TMU0 texcoord buffer), not the address of the pointer table. */
void R_ShutdownAllocators( void ) {
	if ( glState_currentStorageMode != 3 ) {
		if ( glConfig_NVVertexArrayRange ) {
			if ( glConfig_NVVertexArrayRange == 1 ) {
				qglDisableClientState( 0x851Du );
			} else {
				qglDisableClientState( 0x8533u );
			}
			qglVertexPointer( tess_vertexComponentCount, 0x1406u, 0, tess_xyz );
			qglNormalPointer( 0x1406u, 0, tess_stageNormals );
			qglTexCoordPointer( 2, 0x1406u, 0, tess_activeTexCoords[0] );
			qglColorPointer( 4, 0x1401u, 0, tess_stageVertexColors );
		} else if ( glConfig_ATIVertexArrayObject ) {
			qglVertexPointer( tess_vertexComponentCount, 0x1406u, 0, tess_xyz );
			qglNormalPointer( 0x1406u, 12, tess_stageNormals );
			qglTexCoordPointer( 2, 0x1406u, 0, tess_activeTexCoords[0] );
		}
		glState_currentStorageMode = 3;
	}

	if ( glConfig_NVVertexArrayRange ) {
		if ( tess_agpMemory ) {
			qwglFreeMemoryNV( (void *)tess_agpMemory );
			tess_agpMemory = 0;
			tess_agpSize   = 0;
		}
		if ( tess_videoMemory ) {
			qwglFreeMemoryNV( (void *)tess_videoMemory );
			tess_videoMemory = 0;
			tess_videoSize   = 0;
		}
	} else if ( glConfig_ATIVertexArrayObject ) {
		if ( tess_agpMemory ) {
			qglFreeObjectBufferATI( tess_agpMemory );
			tess_agpMemory = 0;
			tess_agpSize   = 0;
		}
		if ( tess_videoMemory ) {
			qglFreeObjectBufferATI( tess_videoMemory );
			tess_videoMemory = 0;
			tess_videoSize   = 0;
		}
	}
}

/* ---- R_MemInfo_f  0x004B8A10 ----  VERIFIED
 * "r_meminfo".  Retail multiplies by the pooled float 2^-20 at 0x00568EDC
 * (sys_bytesToMbScale) and by 100.0f at 0x00568EB0; the percentage is
 * 0.0f (0x00568E64) when the pool was never allocated.  The arithmetic is
 * float-typed but done on the x87 stack at double precision: the ints enter
 * via fild (no float rounding), `used * 100 / limit` is fmul + fidiv, and each
 * vararg is stored with fstp qword.  The modern build keeps the double-typed
 * form, which rounds the same way. */
void R_MemInfo_f( void ) {
#ifdef COD1_VC7
	ri.Printf( 0, "AGP:   %.2f / %.2f MB (%.1f%%)\n",
			   tess_agpUsed * ( 1.0f / ( 1024.0f * 1024.0f ) ),
			   tess_agpSize * ( 1.0f / ( 1024.0f * 1024.0f ) ),
			   tess_agpSize ? tess_agpUsed * 100.0f / tess_agpSize : 0.0f );
	ri.Printf( 0, "Video: %.2f / %.2f MB (%.1f%%)\n",
			   tess_videoUsed * ( 1.0f / ( 1024.0f * 1024.0f ) ),
			   tess_videoSize * ( 1.0f / ( 1024.0f * 1024.0f ) ),
			   tess_videoSize ? tess_videoUsed * 100.0f / tess_videoSize : 0.0f );
#else
	ri.Printf( 0, "AGP:   %.2f / %.2f MB (%.1f%%)\n",
			   tess_agpUsed * ( 1.0 / ( 1024.0 * 1024.0 ) ),
			   tess_agpSize * ( 1.0 / ( 1024.0 * 1024.0 ) ),
			   tess_agpSize ? tess_agpUsed * 100.0 / tess_agpSize : 0.0 );
	ri.Printf( 0, "Video: %.2f / %.2f MB (%.1f%%)\n",
			   tess_videoUsed * ( 1.0 / ( 1024.0 * 1024.0 ) ),
			   tess_videoSize * ( 1.0 / ( 1024.0 * 1024.0 ) ),
			   tess_videoSize ? tess_videoUsed * 100.0 / tess_videoSize : 0.0 );
#endif
}
