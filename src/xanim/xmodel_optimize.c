/*
 * Reconstructed from Call of Duty 1.1 (Windows, CoDMP.exe).
 * Original translation unit:
 * /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/xanim/xmodel_optimize.c
 * Retail range 0x0048C580-0x0048CB2F, 7 functions.
 * @fidelity: likely
 *
 * These functions build the per-XSurface vertex/index buffers that a rigid
 * (non-skinned) surface is drawn from, using whichever GL path the driver
 * supports: ARB vertex_buffer_object, NV vertex_array_range, or ATI
 * vertex_array_object.  Every vertex is packed into a 32-byte interleaved
 * record (2 texcoord floats followed by 6 vertex floats).  Struct layouts and
 * field offsets below were cross-checked against the CoDMP.exe decompile.
 */

#include "../qcommon/qcommon.h"
#include "../renderer/qgl.h"
#include "../renderer/tr_local.h"   /* glConfig, tr's static vertex-memory pools */

void *Hunk_AllocAlignInternal( int size, int align );

extern int R_CreateBufferARB();

/* ------------------------------------------------------------------------- *
 *  File-local typed views (same CoD XModel layout tr_staticmodel.c uses).
 *  Only the fields these functions touch are named.
 * ------------------------------------------------------------------------- */
#undef ASSERT_OFF
#define ASSERT_OFF( type, field, off ) \
	COD1_STATIC_ASSERT( offsetof( type, field ) == (off), #type "." #field " offset" )

typedef struct {
	unsigned char   tileMode;          /* +0x00 */
	unsigned char   pad01;             /* +0x01 */
	short           vertexCount;       /* +0x02 */
	short           triangleCount;     /* +0x04 */
	short           boneMatrixOffset;  /* +0x06  -1 (0xFFFF) => skinned (skip rigid optimize) */
	unsigned int    boneUsage[4];      /* +0x08 */
	void           *weightedPoints;    /* +0x18 */
	unsigned short *triangles;         /* +0x1c  index triples */
	void           *vertexData;        /* +0x20  6 dwords/vertex (pos+normal) */
	float         (*texCoords)[2];     /* +0x24  2 floats/vertex */
	void           *optimizedDataARB;  /* +0x28 */
	void           *optimizedDataATI;  /* +0x2c */
	void           *optimizedDataNV;   /* +0x30 */
} XSurface;
COD1_ASSERT_SIZE( XSurface, 0x34 );
ASSERT_OFF( XSurface, vertexCount, 0x02 );
ASSERT_OFF( XSurface, triangleCount, 0x04 );
ASSERT_OFF( XSurface, boneMatrixOffset, 0x06 );
ASSERT_OFF( XSurface, triangles, 0x1c );
ASSERT_OFF( XSurface, vertexData, 0x20 );
ASSERT_OFF( XSurface, texCoords, 0x24 );
ASSERT_OFF( XSurface, optimizedDataARB, 0x28 );
ASSERT_OFF( XSurface, optimizedDataATI, 0x2c );
ASSERT_OFF( XSurface, optimizedDataNV, 0x30 );

typedef struct XModelSurfsData {
	struct XModelSurfsData *next;         /* +0x00 */
	short                   surfaceCount; /* +0x04 */
	short                   pad06;        /* +0x06 */
	XSurface              **surfaces;     /* +0x08 */
} XModelSurfsData;
COD1_ASSERT_SIZE( XModelSurfsData, 0x0c );
ASSERT_OFF( XModelSurfsData, surfaceCount, 0x04 );
ASSERT_OFF( XModelSurfsData, surfaces, 0x08 );

/* Per-vertex record written into the interleave scratch and uploaded to GL. */
typedef struct {
	unsigned int texCoord[2];  /* +0x00  from XSurface.texCoords */
	unsigned int vertex[6];    /* +0x08  from XSurface.vertexData (pos+normal) */
} xoptVertex_t;
COD1_ASSERT_SIZE( xoptVertex_t, 0x20 );

/* GL buffer descriptors the alloc callbacks hand back (one per rendering path). */
typedef struct {
	unsigned int vertexBuffer;  /* +0x00  GL_ARRAY_BUFFER name */
	unsigned int indexBuffer;   /* +0x04  GL_ELEMENT_ARRAY_BUFFER name */
} xoptArbBuffers_t;
COD1_ASSERT_SIZE( xoptArbBuffers_t, 0x08 );

typedef struct {
	int   poolType;  /* +0x00  1=primary 2=secondary 3=hunk */
	void *base;      /* +0x04  interleaved vertex data */
} xoptNvBuffer_t;
COD1_ASSERT_SIZE( xoptNvBuffer_t, 0x08 );

typedef struct {
	int          type;          /* +0x00  always 3 */
	unsigned int arrayObject;   /* +0x04  ATI array-object name */
	unsigned int vertexOffset;  /* +0x08  index bytes + base offset */
	unsigned int indexOffset;   /* +0x0c  base offset */
} xoptAtiBuffer_t;
COD1_ASSERT_SIZE( xoptAtiBuffer_t, 0x10 );

/* File-local GL enum names (kept out of qgl.h; sibling renderer files do the same). */
#define GL_ARRAY_BUFFER_ARB          34962   /* 0x8892 */
#define GL_ELEMENT_ARRAY_BUFFER_ARB  34963   /* 0x8893 */
#define GL_STATIC_DRAW_ARB           35044   /* 0x88E4 */
#define GL_PRESERVE_ATI              34658   /* 0x8762 */

/* Retail [ebp-40000h] frame scratch: 0x40000 bytes == 8192 interleaved verts. */
#define XOPT_SCRATCH_VERTS  8192

/*
 * NOTE: all seven functions return void in retail (the decompile's int results
 * were leftover EAX; every caller discards them).  Retail passes surf/modelSurfs
 * in registers (LTCG conventions), which the typed void signatures reproduce.
 */

/* ---- XSurfaceRefresh_ARB  0x0048C580 ----  VERIFIED */
void __cdecl XSurfaceRefresh_ARB( XSurface *surf, int refreshMask )
{
	xoptArbBuffers_t   *buffers;
	unsigned int        indexSize;
	int                 vertexSize;
	int                 vertexCount;
	const unsigned int *srcVertex;
	const unsigned int *srcTexCoord;
	int                 i;
	xoptVertex_t        vertexScratch[XOPT_SCRATCH_VERTS]; /* retail [ebp-40000h] */

	buffers = (xoptArbBuffers_t *)surf->optimizedDataARB;
	if ( !buffers )
		return;

	indexSize   = (6 * surf->triangleCount + 31) & 0xFFFFFFE0;
	srcVertex   = (const unsigned int *)surf->vertexData;
	srcTexCoord = (const unsigned int *)surf->texCoords;
	vertexCount = surf->vertexCount;
	vertexSize  = 32 * vertexCount;
	for ( i = 0; i < vertexCount; ++i )
	{
		vertexScratch[i].texCoord[0] = srcTexCoord[0];
		vertexScratch[i].texCoord[1] = srcTexCoord[1];
		vertexScratch[i].vertex[0]   = srcVertex[0];
		vertexScratch[i].vertex[1]   = srcVertex[1];
		vertexScratch[i].vertex[2]   = srcVertex[2];
		vertexScratch[i].vertex[3]   = srcVertex[3];
		vertexScratch[i].vertex[4]   = srcVertex[4];
		vertexScratch[i].vertex[5]   = srcVertex[5];
		srcVertex   += 6;
		srcTexCoord += 2;
	}

	if ( refreshMask & 2 )   /* refresh index buffer */
	{
		qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, buffers->indexBuffer );
		qglBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, indexSize, NULL, GL_STATIC_DRAW_ARB );
		qglBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, indexSize, surf->triangles, GL_STATIC_DRAW_ARB );
		qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	}
	if ( refreshMask & 1 )   /* refresh vertex buffer */
	{
		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, buffers->vertexBuffer );
		qglBufferDataARB( GL_ARRAY_BUFFER_ARB, vertexSize, NULL, GL_STATIC_DRAW_ARB );
		qglBufferDataARB( GL_ARRAY_BUFFER_ARB, vertexSize, vertexScratch, GL_STATIC_DRAW_ARB );
		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	}
}

/* ---- XModelSurfsRefresh_ARB  0x0048C6C0 ----  VERIFIED */
void __cdecl XModelSurfsRefresh_ARB( XModelSurfsData *modelSurfs, int refreshMask )
{
	int       surfaceCount;
	XSurface **surfaces;
	int       i;

	surfaceCount = modelSurfs->surfaceCount;
	surfaces     = modelSurfs->surfaces;
	for ( i = 0; i < surfaceCount; ++i )
		XSurfaceRefresh_ARB( surfaces[i], refreshMask );
}

/* ---- R_OptimizeRigidXSurfaceARB  0x0048C6F0 ----  VERIFIED */
void __cdecl R_OptimizeRigidXSurfaceARB( XSurface *surf, int (__cdecl *alloc)(int) )
{
	unsigned int        indexSize;
	int                 vertexSize;
	const unsigned int *srcVertex;
	const unsigned int *srcTexCoord;
	int                 i;
	unsigned int        indexBuffer;
	unsigned int        vertexBuffer;
	xoptArbBuffers_t   *buffers;
	xoptVertex_t        vertexScratch[XOPT_SCRATCH_VERTS]; /* retail [ebp-40000h] */

	if ( surf->optimizedDataARB )
		return;

	indexSize   = (6 * surf->triangleCount + 31) & 0xFFFFFFE0;
	srcVertex   = (const unsigned int *)surf->vertexData;
	srcTexCoord = (const unsigned int *)surf->texCoords;
	vertexSize  = 32 * surf->vertexCount;
	for ( i = 0; i < surf->vertexCount; ++i )
	{
		vertexScratch[i].texCoord[0] = srcTexCoord[0];
		vertexScratch[i].texCoord[1] = srcTexCoord[1];
		vertexScratch[i].vertex[0]   = srcVertex[0];
		vertexScratch[i].vertex[1]   = srcVertex[1];
		vertexScratch[i].vertex[2]   = srcVertex[2];
		vertexScratch[i].vertex[3]   = srcVertex[3];
		vertexScratch[i].vertex[4]   = srcVertex[4];
		vertexScratch[i].vertex[5]   = srcVertex[5];
		srcVertex   += 6;
		srcTexCoord += 2;
	}

	indexBuffer = R_CreateBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB,
	                                 indexSize, (int)surf->triangles, GL_STATIC_DRAW_ARB );
	if ( !indexBuffer )
		return;
	vertexBuffer = R_CreateBufferARB( GL_ARRAY_BUFFER_ARB,
	                                  vertexSize, (int)vertexScratch, GL_STATIC_DRAW_ARB );
	if ( !vertexBuffer )
	{
		qglDeleteBuffersARB( 1, &indexBuffer );
		return;
	}
	buffers = (xoptArbBuffers_t *)alloc( 8 );
	buffers->indexBuffer   = indexBuffer;
	buffers->vertexBuffer  = vertexBuffer;
	surf->optimizedDataARB = buffers;
}

/* ---- R_OptimizeRigidXSurfaceNV  0x0048C7F0 ----  VERIFIED
 * Retail Hunk_AllocAlignInternal takes size in ECX (the tree's in EBX), which is
 * what still shifts this function's register allocation. */
void __cdecl R_OptimizeRigidXSurfaceNV( XSurface *surf, int (__cdecl *alloc)(int) )
{
	xoptNvBuffer_t     *nvBuffer;
	int                 allocSize;
	int                 poolType;   /* 1=primary 2=secondary 3=hunk */
	const unsigned int *srcVertex;
	const unsigned int *srcTexCoord;
	int                 i;

	if ( surf->optimizedDataNV )
		return;

	nvBuffer  = (xoptNvBuffer_t *)alloc( 8 );
	allocSize = (32 * surf->vertexCount + 31) & 0xFFFFFFE0;
	if ( tr_staticVertexMemoryPrimaryUsed + allocSize < tr_staticVertexMemoryPrimaryLimit )
	{
		nvBuffer->base = (void *)(tr_staticVertexMemoryPrimary + tr_staticVertexMemoryPrimaryUsed);
		tr_staticVertexMemoryPrimaryUsed += allocSize;
		poolType = 1;
	}
	else if ( tr_staticVertexMemorySecondaryUsed + allocSize < tr_staticVertexMemorySecondaryLimit )
	{
		nvBuffer->base = (void *)(tr_staticVertexMemorySecondary + tr_staticVertexMemorySecondaryUsed);
		tr_staticVertexMemorySecondaryUsed += allocSize;
		poolType = 2;
	}
	else
	{
		nvBuffer->base = Hunk_AllocAlignInternal( allocSize, 32 );
		poolType = 3;
	}
	nvBuffer->poolType = poolType;

	srcVertex   = (const unsigned int *)surf->vertexData;
	srcTexCoord = (const unsigned int *)surf->texCoords;
	for ( i = 0; i < surf->vertexCount; ++i )
	{
		((xoptVertex_t *)nvBuffer->base)[i].texCoord[0] = srcTexCoord[0];
		((xoptVertex_t *)nvBuffer->base)[i].texCoord[1] = srcTexCoord[1];
		((xoptVertex_t *)nvBuffer->base)[i].vertex[0]   = srcVertex[0];
		((xoptVertex_t *)nvBuffer->base)[i].vertex[1]   = srcVertex[1];
		((xoptVertex_t *)nvBuffer->base)[i].vertex[2]   = srcVertex[2];
		((xoptVertex_t *)nvBuffer->base)[i].vertex[3]   = srcVertex[3];
		((xoptVertex_t *)nvBuffer->base)[i].vertex[4]   = srcVertex[4];
		((xoptVertex_t *)nvBuffer->base)[i].vertex[5]   = srcVertex[5];
		srcVertex   += 6;
		srcTexCoord += 2;
	}
	surf->optimizedDataNV = nvBuffer;
}

/* ---- R_OptimizeRigidXSurfaceATI  0x0048C900 ----  VERIFIED
 * Retail is __usercall: surf in EDI, alloc as the only stack argument
 * (LTCG convention; callers: push alloc; call; add esp,4).  alloc is the same
 * one-argument __cdecl allocator the ARB/NV paths take (0x0048C98B push 10h;
 * call [alloc]; 0x0048C99A add esp,4 pops the size; the push esi before
 * it is a callee-save, popped at 0x0048CA5D).  Frame: 0x40008 bytes = poolSelect (frame-0x40008,
 * later reused as the copy-loop counter), vertexUploadSize (frame-0x40004),
 * then the 0x40000-byte scratch. */
void __cdecl R_OptimizeRigidXSurfaceATI( XSurface *surf, int (__cdecl *alloc)(int) )
{
	unsigned int        indexSize;
	int                 vertexUploadSize;
	int                 allocSize;
	int                 poolBase;
	int                 poolSelect;   /* 1 = primary pool, 2 = secondary pool */
	xoptAtiBuffer_t    *atiBuffer;
	const unsigned int *srcVertex;
	const unsigned int *srcTexCoord;
	int                 i;
	xoptVertex_t        vertexScratch[XOPT_SCRATCH_VERTS]; /* retail frame-0x40000 */

	if ( surf->optimizedDataATI )
		return;

	if ( glConfig_ATIElementArray )
		indexSize = (6 * surf->triangleCount + 31) & 0xFFFFFFE0;
	else
		indexSize = 0;

	vertexUploadSize = 32 * surf->vertexCount;
	allocSize        = (vertexUploadSize + indexSize + 31) & 0xFFFFFFE0;
	if ( tr_staticVertexMemoryPrimaryUsed + allocSize <= tr_staticVertexMemoryPrimaryLimit )
	{
		poolBase = tr_staticVertexMemoryPrimaryUsed;
		tr_staticVertexMemoryPrimaryUsed += allocSize;
		poolSelect = 1;
	}
	else
	{
		if ( tr_staticVertexMemorySecondaryUsed + allocSize > tr_staticVertexMemorySecondaryLimit )
			return;
		poolBase = tr_staticVertexMemorySecondaryUsed;
		tr_staticVertexMemorySecondaryUsed += allocSize;
		poolSelect = 2;
	}

	atiBuffer = (xoptAtiBuffer_t *)alloc( 16 );
	atiBuffer->type = 3;
	if ( poolSelect == 1 )
		atiBuffer->arrayObject = tr_staticVertexMemoryPrimary;
	else
		atiBuffer->arrayObject = tr_staticVertexMemorySecondary;
	atiBuffer->indexOffset  = poolBase;
	atiBuffer->vertexOffset = poolBase + indexSize;

	srcVertex   = (const unsigned int *)surf->vertexData;
	srcTexCoord = (const unsigned int *)surf->texCoords;
	for ( i = 0; i < surf->vertexCount; ++i )
	{
		vertexScratch[i].texCoord[0] = srcTexCoord[0];
		vertexScratch[i].texCoord[1] = srcTexCoord[1];
		vertexScratch[i].vertex[0]   = srcVertex[0];
		vertexScratch[i].vertex[1]   = srcVertex[1];
		vertexScratch[i].vertex[2]   = srcVertex[2];
		vertexScratch[i].vertex[3]   = srcVertex[3];
		vertexScratch[i].vertex[4]   = srcVertex[4];
		vertexScratch[i].vertex[5]   = srcVertex[5];
		srcVertex   += 6;
		srcTexCoord += 2;
	}

	if ( indexSize )
		qglUpdateObjectBufferATI( atiBuffer->arrayObject, atiBuffer->indexOffset,
		                          indexSize, surf->triangles, GL_PRESERVE_ATI );
	qglUpdateObjectBufferATI( atiBuffer->arrayObject, atiBuffer->vertexOffset,
	                          vertexUploadSize, vertexScratch, GL_PRESERVE_ATI );
	surf->optimizedDataATI = atiBuffer;
}

/* ---- XModelOptimize  0x0048CA70 ----  VERIFIED */
void __cdecl XModelOptimize( XModelSurfsData *modelSurfs, int (__cdecl *alloc)(int) )
{
	int        surfaceCount;
	XSurface **surfaces;
	XSurface  *surf;
	int        i;

	surfaceCount = modelSurfs->surfaceCount;
	surfaces     = modelSurfs->surfaces;
	for ( i = 0; i < surfaceCount; ++i )
	{
		surf = surfaces[i];
		if ( surf->boneMatrixOffset != -1 )   /* rigid surface only */
		{
			if ( glConfig_ARBVertexBufferObject )
				R_OptimizeRigidXSurfaceARB( surf, alloc );
			else if ( glConfig_NVVertexArrayRange )
				R_OptimizeRigidXSurfaceNV( surf, alloc );
			else if ( glConfig_ATIVertexArrayObject )
				R_OptimizeRigidXSurfaceATI( surf, alloc );
		}
	}
}

/* ---- XSurfaceOptimize  0x0048CAE0 ----  VERIFIED
 * Retail: alloc in EAX, surf in ECX (LTCG convention; no caller in the tree). */
void __cdecl XSurfaceOptimize( XSurface *surf, int (__cdecl *alloc)(int) )
{
	if ( surf->boneMatrixOffset != -1 )   /* rigid surface only */
	{
		if ( glConfig_ARBVertexBufferObject )
			R_OptimizeRigidXSurfaceARB( surf, alloc );
		else if ( glConfig_NVVertexArrayRange )
			R_OptimizeRigidXSurfaceNV( surf, alloc );
		else if ( glConfig_ATIVertexArrayObject )
			R_OptimizeRigidXSurfaceATI( surf, alloc );
	}
}
