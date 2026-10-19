
#ifndef TR_TESS_H
#define TR_TESS_H

/* Retail's tess (shaderCommands_t, 0x017A7F60, 0x218068 bytes) as one object
 * (defined in renderer/tr_shade.c, as in RTCW).  Retail addresses every member off &tess, so a
 * store through one member may alias the others; the per-member names the
 * renderer uses are macros onto the members. */
typedef struct shaderCommands_s {
	unsigned short  indexes[49152];                 /* +0x000000 0x017A7F60 */
	float           xyz[32768];                     /* +0x018000 0x017BFF60 */
	float           texCoords0[32768];              /* +0x038000 */
	float           texCoords1[32768];              /* +0x058000 */
	unsigned char   vertexColors[32768];            /* +0x078000 */
	float           stageNormals[24576];            /* +0x080000 */
	float           stageBitangents[24576];         /* +0x098000 */
	float           stageTangents[24576];           /* +0x0B0000 */
	unsigned char   stageVertexColors[32768];       /* +0x0C8000 */
	void           *activeTexCoords[8];             /* +0x0D0000 */
	unsigned char   generatedTexCoords[0x100000];   /* +0x0D0020, 8 units x 0x20000 */
	unsigned int    constantColor255[8192];         /* +0x1D0020 */
	int             vertexComponentCount;           /* +0x1D8020 */
	int             requiresVertexBasis;            /* +0x1D8024 */
	int             stageTangentsValid;             /* +0x1D8028 */
	int             stageBitangentsValid;           /* +0x1D802C */
	int             shader;                         /* +0x1D8030 0x0197FF90 */
	int             reserved_0x1D8034;              /* +0x1D8034 no reader in the tree or retail (no xrefs to 0x0197FF94);
	                                                 * RTCW has shaderTime right after shader */
	float           shaderTime;                     /* +0x1D8038 */
	int             entity;                         /* +0x1D803C */
	int             dlightBits;                     /* +0x1D8040 */
	int             numIndexes;                     /* +0x1D8044 0x0197FFA4 */
	int             numVertexes;                    /* +0x1D8048 0x0197FFA8 */
	int             optimizedFirstVertex;           /* +0x1D804C */
	int             optimizedVertexEnd;             /* +0x1D8050 */
	unsigned char   optimizedIndexes[262144];       /* +0x1D8054 */
	int             renderedIndexCount;             /* +0x218054 0x019BFFB4 */
	int             renderedVertexCount;            /* +0x218058 */
	int             activeStageCount;               /* +0x21805C 0x019BFFBC */
	int           (*stageIterator)();               /* +0x218060 */
	int             activeStages;                   /* +0x218064 0x019BFFC4 */
} shaderCommands_t;
COD1_STATIC_ASSERT( sizeof( shaderCommands_t ) == 0x218068, "shaderCommands_t must be 0x218068 bytes" );
COD1_STATIC_ASSERT( offsetof( shaderCommands_t, constantColor255 ) == 0x1D0020, "tess.constantColor255 offset" );
COD1_STATIC_ASSERT( offsetof( shaderCommands_t, numIndexes ) == 0x1D8044, "tess.numIndexes offset" );
COD1_STATIC_ASSERT( offsetof( shaderCommands_t, renderedIndexCount ) == 0x218054, "tess.renderedIndexCount offset" );

extern shaderCommands_t tess;

#define tess_indexes                tess.indexes
#define tess_xyz                    tess.xyz
#define tess_texCoords0             tess.texCoords0
#define tess_texCoords1             tess.texCoords1
#define tess_vertexColors           tess.vertexColors
#define tess_stageNormals           tess.stageNormals
#define tess_stageBitangents        tess.stageBitangents
#define tess_stageTangents          tess.stageTangents
#define tess_stageVertexColors      tess.stageVertexColors
#define tess_activeTexCoords        tess.activeTexCoords
#define tess_generatedTexCoords     tess.generatedTexCoords
#define tess_constantColor255       tess.constantColor255
#define tess_vertexComponentCount   tess.vertexComponentCount
#define tess_requiresVertexBasis    tess.requiresVertexBasis
#define tess_stageTangentsValid     tess.stageTangentsValid
#define tess_stageBitangentsValid   tess.stageBitangentsValid
#define tess_shader                 tess.shader
#define tess_shaderTime             tess.shaderTime
#define tess_entity                 tess.entity
#define tess_dlightBits             tess.dlightBits
#define tess_numIndexes             tess.numIndexes
#define tess_numVertexes            tess.numVertexes
#define tess_optimizedFirstVertex   tess.optimizedFirstVertex
#define tess_optimizedVertexEnd     tess.optimizedVertexEnd
#define tess_optimizedIndexes       tess.optimizedIndexes
#define tess_renderedIndexCount     tess.renderedIndexCount
#define tess_renderedVertexCount    tess.renderedVertexCount
#define tess_activeStageCount       tess.activeStageCount
#define tess_stageIterator          tess.stageIterator
#define tess_activeStages           tess.activeStages

#define tess_indexes_last    (tess.indexes[49151])   /* 0x017BFF5E */


#undef  tess_texCoords0AsInt
#define tess_texCoords0AsInt   ((int *)((char *)tess_texCoords0 + 0))   /* 0x017DFF60 = tess_texCoords0+0 */


#undef  tess_vertexColorsPacked
#define tess_vertexColorsPacked   ((int *)((char *)tess_vertexColors + 0))   /* 0x0181FF60 = tess_vertexColors+0 */




#undef  tess_stageVertexColorsPacked
#define tess_stageVertexColorsPacked   ((int *)((char *)tess_stageVertexColors + 0))   /* 0x0186FF60 = tess_stageVertexColors+0 */


#ifndef tr_sunLight
extern float tr_sunLight[3];       /* 0x016C57C4 */
extern float tr_sunDirection[3];   /* 0x016C57D0 */
#endif

#undef  tr_sunLight_0
#define tr_sunLight_0     (*(float *)((char *)tr_sunLight + 0))       /* 0x016C57C4 */
#undef  tr_sunLight_1
#define tr_sunLight_1     (*(float *)((char *)tr_sunLight + 4))       /* 0x016C57C8 */
#undef  tr_sunLight_2
#define tr_sunLight_2     (*(float *)((char *)tr_sunLight + 8))       /* 0x016C57CC */

#undef  tr_sunDirection_0
#define tr_sunDirection_0   (*(int *)((char *)tr_sunDirection + 0))     /* 0x016C57D0 */
#undef  tr_sunDirection_1
#define tr_sunDirection_1   (*(int *)((char *)tr_sunDirection + 4))     /* 0x016C57D4 */
#undef  tr_sunDirection_2
#define tr_sunDirection_2   (*(int *)((char *)tr_sunDirection + 8))     /* 0x016C57D8 */


#undef  sky_mins_1
#define sky_mins_1     ( sky_mins[1] )                              /* 0x011DAE20 */
#undef  sky_maxs_1
#define sky_maxs_1     ( sky_maxs[1] )                              /* 0x011DAE50 */




#endif
