/*
 * @fidelity: likely
 * @fidelity-default: unreviewed
 */

#include "../qcommon/qcommon.h"
#include "tr_records.h"
#include "tr_gl_types.h"
#include "tr_tess.h"
#include "tr_local.h"

extern int GL_Cull();
extern int GL_State();
int __cdecl RB_BuildDlightArrays( float *texCoords, unsigned char *colors, float *dlight, int hitIndexes );
extern int RB_ComputeColors();
extern int RB_ComputeTexCoords();
extern int RB_DeformTessGeometry();
extern int RB_SetIteratorFog();
extern int RB_SetupMultitextureATI();

#define tess_indexes_base           tess_indexes    /* 0x017A7F60 = tess base */

#define rb_clientStateBits          glState_clientStateBits   /* 0x016C3954 */

#define rb_atiObjectBuffer          backEnd_dynamicBuffer_storage   /* 0x016D93EC */
#define rb_atiBufferSize            backEnd_dynamicBuffer_capacity   /* 0x016D93F0 */
#define rb_atiBufferCursor          backEnd_dynamicBuffer_currentOffset   /* 0x016D93F4 */

#define backEnd_currentLight        backEnd_currentDlight   /* 0x016D8E60 */
#define backEnd_num_dlights         backEnd_refdef_num_dlights   /* 0x016D8B1C */
#define backEnd_dlights             backEnd_refdef_dlights   /* 0x016D8B24 */

#define R_MAX_SHADER_STAGES         8
#define R_MAX_TEXTURE_UNITS         8

/* shaderStage_t.flags bits (SHADER_STAGE_*). One texcoord-array bit per unit,
 * SHADER_STAGE_TEXCOORD_ARRAY0 << n, plus the color/normal-array requests. */
#define SHADER_STAGE_PER_LIGHT        0x80
#define SHADER_STAGE_TEXCOORD_ARRAY0  0x100
#define SHADER_STAGE_COLOR_ARRAY      0x10000
#define SHADER_STAGE_NORMAL_ARRAY     0x20000

/* shader_t.lightingFlags: the per-entity dynamic-light pass gate (at +0x54). */
#define SHADER_LIGHTING_PER_ENTITY    0x80
/* shader_t.surfaceParmFlags (+0x5c): either bit blocks dlight projection. */
#define SHADER_DLIGHT_PROJECTION_BLOCK_MASK   0x20004

#define CGEN_LIGHTING_PRECALC         11   /* shaderStage_t.rgbGen: use entity RGBA */
#define R_LIGHT_TYPE_DIFFUSE_SUN      8    /* dlight_t.type value skipped by entity lights */


/* ------------------------------------------------------------------------- *
 *  File-local typed views over the shader stage handed to the ATI backend.
 *
 *  RB_SingleStageGenericATI and the iterators reach every stage field through
 *  a shaderStage_t pointer; the record types below replace the raw
 *  `*(int *)(stage + off)` reads with named fields (bundle layout as documented
 *  in tr_shade.c; cross-checked against CoDUO backend.h shaderStage_s /
 *  textureBundle_s).  Retyping a pointer never moves storage, so the offsets
 *  the ATI path reads are pinned with asserts that fail the build if they drift.
 * ------------------------------------------------------------------------- */

/* textureBundle_t (0xc8) -- one texture stage's image/tcgen state.  The ATI
 * path reads only the component count of the generated texcoord array and the
 * lightmap flag used by r_lightmap early-out. */
typedef struct {
	void  *image[32];               /* +0x00 */
	int   numImageAnimations;       /* +0x80 */
	float imageAnimationSpeed;      /* +0x84 */
	void  *waterMap;                /* +0x88  waterMap_t (tr_shader.c) */
	int   texCoordComponentCount;   /* +0x8c  texcoord components uploaded for this unit */
	int   textureEnvMode;           /* +0x90 */
	void  *texEnvCombine;           /* +0x94  ShaderTexEnvCombine (tr_shader.c) */
	void  *nvTextureShader;         /* +0x98  ShaderNVTextureOperation (tr_shader.c) */
	int   tcGen;                    /* +0x9c */
	float tcGenVectors[2][3];       /* +0xa0  TCGEN_VECTOR (RB_ComputeTexCoords) */
	int   numTexMods;               /* +0xb8 */
	void  *texMods;                 /* +0xbc  texModInfo_t[numTexMods] */
	int   videoMapHandle;           /* +0xc0 */
	byte  isLightmap;               /* +0xc4  r_lightmap forces this unit to REPLACE */
	byte  isVideoMap;               /* +0xc5 */
	byte  clampAnimation;           /* +0xc6 */
	byte  reserved_0xc7;            /* +0xc7  alignment */
} textureBundle_t;
COD1_ASSERT_SIZE(textureBundle_t, 0xc8);
ASSERT_OFF(textureBundle_t, texCoordComponentCount, 0x8c);
ASSERT_OFF(textureBundle_t, isLightmap, 0xc4);

/* shaderStage_t (0x688) -- one shader stage. */
typedef struct {
	unsigned        flags;                        /* +0x000  SHADER_STAGE_* */
	textureBundle_t bundle[R_MAX_TEXTURE_UNITS];  /* +0x004  8 * 0xc8 */
	int             atiFragmentShader;            /* +0x644 */
	void           *registerCombiners;            /* +0x648 */
	char           *vertexProgram;                /* +0x64c */
	byte            rgbWave[0x14];                /* +0x650  waveForm_t */
	int             rgbGen;                       /* +0x664  CGEN_* */
	byte            alphaWave[0x14];              /* +0x668  waveForm_t */
	int             alphaGen;                     /* +0x67c */
	byte            constantColor[4];             /* +0x680  RGBA */
	unsigned        stateBits;                    /* +0x684  GL_State bits */
} shaderStage_t;
COD1_ASSERT_SIZE(shaderStage_t, 0x688);
ASSERT_OFF(shaderStage_t, bundle, 0x004);
ASSERT_OFF(shaderStage_t, rgbGen, 0x664);
ASSERT_OFF(shaderStage_t, constantColor, 0x680);
ASSERT_OFF(shaderStage_t, stateBits, 0x684);

void __cdecl RB_SingleStageGenericATI( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indexes );

/* ---- RB_PickBufferOffsetATI  0x0050B540 ----  VERIFIED
 * Allocate `size` bytes from the ATI object buffer, wrapping the cursor back to
 * offset 0 when the allocation would run past `capacity`.  Inlined into every
 * upload class of RB_SingleStageGenericATI in the retail binary; kept here as
 * the out-of-line copy the compiler retained.  The parameter order gives
 * retail's LTCG registers (cursor esi, capacity edi, size ecx). */
int __cdecl RB_PickBufferOffsetATI( int *cursor, int capacity, int size )
{
	int offset;

	offset  = ( *cursor + size > capacity ) ? 0 : *cursor;
	*cursor = size + offset;
	return offset;
}

/* ---- RB_SingleStageGenericATI  0x0050B560 ----  VERIFIED
 * Upload one shader stage's vertex arrays into the ATI object buffer and draw
 * it.  Enabled texcoord/color/normal/vertex arrays are each packed at a wrapped
 * cursor offset (the offset picker is inlined per class), bound with
 * qglArrayObjectATI, then the indexed primitive is issued. */
void __cdecl RB_SingleStageGenericATI( shaderStage_t *stage, GLsizei numIndexes, GLvoid *indexes )
{
	int capacity;               /* rb_atiBufferSize, cached at entry */
	int cursor;                 /* running object-buffer offset, published at the end */
	int i;
	int size;
	int offset;
	int colorOffset;
	int normalOffset;
	int vertexOffset;
	int texCoordOffsets[R_MAX_TEXTURE_UNITS];

	capacity = rb_atiBufferSize;
	cursor   = rb_atiBufferCursor;

	qglEnableClientState( 0x8768u );   /* GL_ELEMENT_ARRAY_ATI */

	RB_ComputeTexCoords( (int)stage );

	for ( i = 0; i < glConfig_maxActiveTextures; i++ )
	{
		texCoordOffsets[i] = 0;
		if ( ( stage->flags & ( SHADER_STAGE_TEXCOORD_ARRAY0 << i ) ) != 0 )
		{
			size   = 4 * tess_numVertexes * stage->bundle[i].texCoordComponentCount;
			offset = ( size + cursor > capacity ) ? 0 : cursor;
			cursor = offset + size;
			texCoordOffsets[i] = offset;
			qglUpdateObjectBufferATI( rb_atiObjectBuffer, offset, size,
			                          (const GLvoid *)tess_activeTexCoords[i],   /* stored pointer, 0x01877F60 + 4*i */
			                          0x8762 /* GL_PRESERVE_ATI */ );
		}
	}

	RB_SetupMultitextureATI( stage, rb_atiObjectBuffer, (int)texCoordOffsets, 0 );

	if ( ( stage->flags & SHADER_STAGE_COLOR_ARRAY ) != 0 )
	{
		size        = 4 * tess_numVertexes;
		colorOffset = ( size + cursor > capacity ) ? 0 : cursor;
		cursor      = colorOffset + size;

		RB_ComputeColors( (int)stage );

		if ( ( rb_clientStateBits & 0x100 ) == 0 )   /* GLS_CLIENT_COLOR_ARRAY */
		{
			qglEnableClientState( 0x8076u );   /* GL_COLOR_ARRAY */
			rb_clientStateBits |= 0x100u;
		}
		qglUpdateObjectBufferATI( rb_atiObjectBuffer, colorOffset, size,
		                          tess_stageVertexColors, 0x8762 );
		qglArrayObjectATI( 0x8076 /* GL_COLOR_ARRAY */, 4, 0x1401 /* GL_UNSIGNED_BYTE */,
		                   0, rb_atiObjectBuffer, colorOffset );
	}
	else
	{
		if ( ( rb_clientStateBits & 0x100 ) != 0 )
		{
			qglDisableClientState( 0x8076u );
			rb_clientStateBits &= ~0x100u;
		}
		if ( stage->rgbGen == CGEN_LIGHTING_PRECALC )
			qglColor4ubv( (const GLubyte *)( backEnd_currentEntity + 108 ) );   /* currentEntity->e.shaderRGBA, +0x6c */
		else
			qglColor4ubv( (const GLubyte *)stage->constantColor );
	}

	if ( ( stage->flags & SHADER_STAGE_NORMAL_ARRAY ) != 0 )
	{
		size         = 12 * tess_numVertexes;
		normalOffset = ( size + cursor > capacity ) ? 0 : cursor;
		cursor       = normalOffset + size;
		qglUpdateObjectBufferATI( rb_atiObjectBuffer, normalOffset, size,
		                          tess_stageNormals, 0x8762 );
		if ( ( rb_clientStateBits & 0x200 ) == 0 )   /* GLS_CLIENT_NORMAL_ARRAY */
		{
			qglEnableClientState( 0x8075u );   /* GL_NORMAL_ARRAY */
			rb_clientStateBits |= 0x200u;
		}
		qglArrayObjectATI( 0x8075 /* GL_NORMAL_ARRAY */, 3, 0x1406 /* GL_FLOAT */, 0,
		                   rb_atiObjectBuffer, normalOffset );
	}
	else if ( ( rb_clientStateBits & 0x200 ) != 0 )
	{
		qglDisableClientState( 0x8075u );
		rb_clientStateBits &= ~0x200u;
	}

	size         = 4 * tess_numVertexes * tess_vertexComponentCount;
	vertexOffset = ( size + cursor > capacity ) ? 0 : cursor;
	cursor       = vertexOffset + size;
	qglUpdateObjectBufferATI( rb_atiObjectBuffer, vertexOffset, size,
	                          tess_xyz, 0x8762 );
	if ( ( rb_clientStateBits & 0x400 ) == 0 )   /* GLS_CLIENT_VERTEX_ARRAY */
	{
		qglEnableClientState( 0x8074u );   /* GL_VERTEX_ARRAY */
		rb_clientStateBits |= 0x400u;
	}
	qglArrayObjectATI( 0x8074 /* GL_VERTEX_ARRAY */, tess_vertexComponentCount, 0x1406 /* GL_FLOAT */, 0,
	                   rb_atiObjectBuffer, vertexOffset );

	GL_State( stage->stateBits );

	rb_atiBufferCursor = cursor;

	qglDrawElements( 4u /* GL_TRIANGLES */, numIndexes,
	                 0x1403u /* GL_UNSIGNED_SHORT */, indexes );
	qglDisableClientState( 0x8768u );   /* GL_ELEMENT_ARRAY_ATI */
}

/* ---- RB_IterateStagesGenericATI  0x0050B840 ----  VERIFIED
 * Draw every non-per-light stage of the current shader.  Per-light stages are
 * deferred to the dynamic-light passes; in r_lightmap mode iteration stops
 * after the first stage that consumes a lightmap bundle.  `input` is &tess
 * (Q3's shaderCommands_t *input; retail passes it in ebx). */
void __cdecl RB_IterateStagesGenericATI( shaderCommands_t *input )
{
	shaderStage_t *stage;
	int            i;

	for ( i = 0; i < R_MAX_SHADER_STAGES; i++ )
	{
		stage = ( (shaderStage_t **)tess_activeStages )[i];
		if ( !stage )
			break;

		if ( ( stage->flags & SHADER_STAGE_PER_LIGHT ) == 0 )
		{
			RB_SingleStageGenericATI( stage, input->numIndexes, (GLvoid *)input->indexes );
			if ( r_lightmap->integer
			  && ( stage->bundle[0].isLightmap || stage->bundle[1].isLightmap ) )
				break;
		}
	}
}

/* ---- ProjectDlightTextureATI  0x0050B8A0 ----  VERIFIED
 * For each dynamic light touching this surface, build the light's index/colour
 * arrays and draw them: per-entity-lit shaders replay their per-light stages,
 * otherwise the shared tr.dlightShader stage is used. */
void __cdecl ProjectDlightTextureATI( void )
{
	shaderStage_t *dlightStage;
	shaderStage_t *stage;
	GLsizei        hitIndexCount;
	int            i;
	int            stageIndex;
	unsigned short hitIndexes[49152];   /* projected-dlight index scratch (R_MAX_TESS_INDEXES) */

	if ( !backEnd_num_dlights )
		return;

	for ( i = 0; i < backEnd_num_dlights; i++ )
	{
		if ( ( ( 1 << i ) & tess_dlightBits ) == 0 )
			continue;

		hitIndexCount = RB_BuildDlightArrays( (float *)tess_texCoords1,
		                                      (unsigned char *)tess_vertexColors,
		                                      (float *)&( (dlight_t *)backEnd_dlights )[i],
		                                      (int)hitIndexes );
		if ( !hitIndexCount )
			continue;

		if ( ( ( (shader_t *)tess_shader )->lightingFlags & SHADER_LIGHTING_PER_ENTITY ) != 0 )
		{
			backEnd_currentLight = (int)&( (dlight_t *)backEnd_dlights )[i];
			for ( stageIndex = 0; stageIndex < R_MAX_SHADER_STAGES; stageIndex++ )
			{
				stage = ( (shaderStage_t **)tess_activeStages )[stageIndex];
				if ( !stage )
					break;
				if ( ( stage->flags & SHADER_STAGE_PER_LIGHT ) != 0 )
					RB_SingleStageGenericATI( stage, hitIndexCount, hitIndexes );
			}
		}
		else
		{
			/* shared projected-dlight material: tr.dlightShader->stages[0] */
			dlightStage = *(shaderStage_t **)( tr_dlightShader + 340 );   /* shader->stages[0], +0x154 */
			if ( dlightStage )
				RB_SingleStageGenericATI( dlightStage, hitIndexCount, hitIndexes );
		}
	}
}

/* ---- RB_StageIteratorGenericATI  0x0050B9C0 ----  VERIFIED
 * ATI back-end entry for one tessellated surface: run the ordinary stage pass,
 * then the projected dynamic-light pass, then the per-entity light pass. */
void __cdecl RB_StageIteratorGenericATI( int portalPass )
{
	shaderStage_t *stage;
	char          *logText;
	int            entity;
	int            lightCount;
	int            lightIndex;
	int            stageIndex;

	if ( tess_activeStageCount <= 0 )
		return;
	if ( portalPass && !tess_dlightBits )
		return;

	if ( r_logFile->integer )
	{
		logText = va( "--- RB_StageIteratorGenericATI( %s ) ---\n", ( (shader_t *)tess_shader )->name );
		if ( Stream )
			fprintf( Stream, "%s", logText );
	}

	RB_DeformTessGeometry();
	RB_SetIteratorFog();
	GL_Cull( ((shader_t *)tess_shader)->cullType );   /* shader->cullType */

	if ( !portalPass )
		RB_IterateStagesGenericATI( &tess );

	/* projected dynamic lights: shaders sorting early, or that are per-entity
	 * lit, unless the shader blocks dlight projection */
	if ( tess_dlightBits )
	{
		if ( ( (shader_t *)tess_shader )->sort <= 5.0f
		  || ( ( (shader_t *)tess_shader )->lightingFlags & SHADER_LIGHTING_PER_ENTITY ) != 0 )
		{
			if ( ( *(unsigned *)( tess_shader + 0x5c ) & SHADER_DLIGHT_PROJECTION_BLOCK_MASK ) == 0 )   /* shader->surfaceParmFlags */
			{
				ProjectDlightTextureATI();
				return;
			}
		}
	}

	if ( ( ( (shader_t *)tess_shader )->lightingFlags & SHADER_LIGHTING_PER_ENTITY ) == 0 )
		return;

	entity = backEnd_currentEntity;
	if ( !entity )
		return;
	lightCount = ((trRefEntity_t *)entity)->lightCount;
	if ( !lightCount )
		return;

	for ( lightIndex = 0; lightIndex < lightCount; lightIndex++ )
	{
		backEnd_currentLight = (int)((trRefEntity_t *)entity)->lights[lightIndex].light;
		if ( ( (dlight_t *)backEnd_currentLight )->type != R_LIGHT_TYPE_DIFFUSE_SUN )
		{
			backEnd_currentLightScale =
			    ((trRefEntity_t *)entity)->lights[lightIndex].scale;

			for ( stageIndex = 0; stageIndex < R_MAX_SHADER_STAGES; stageIndex++ )
			{
				stage = ( (shaderStage_t **)tess_activeStages )[stageIndex];
				if ( !stage )
					break;
				if ( ( stage->flags & SHADER_STAGE_PER_LIGHT ) != 0 )
				{
					RB_SingleStageGenericATI( stage, tess_numIndexes,
					                          (GLvoid *)tess_indexes_base );
					entity = backEnd_currentEntity;
				}
			}
		}
		lightCount = ((trRefEntity_t *)entity)->lightCount;
	}

	backEnd_currentLight = 0;
}
