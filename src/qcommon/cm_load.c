/*
 * qcommon/cm_load.c
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/qcommon/cm_load.c
 *
 * Retail range 0x00418FF0-0x0041A4D0, 26 functions.
 *
 * @fidelity: verified
 */

#include "cm_local.h"

extern void Sys_OutOfMemoryPrep( void );
extern char *SEH_GetLocalizedString_m( const char *key );
extern qboolean FS_MakeReadOnly( const char *path, qboolean readOnly );
extern unsigned Sys_LoadingKeepAlive( void );

#define CM_MB_ICONHAND              0x00000010
__declspec( dllimport ) int __stdcall MessageBoxA( void *wnd, const char *text,
												   const char *caption,
												   unsigned int type );

clipMap_t cm;    /* 0x016174A0 */
byte *cmod_base;    /* 0x016215FC */

cvar_t *cm_noCurves;    /* 0x01617480 */
cvar_t *cm_playerCurveClip;    /* 0x01621600 */

static unsigned int cm_lastChecksum;

static void *cm_lumpData;

/* LittleLong is a no-op on x86; retail keeps the emptied header swap loops */
#define CM_LittleLong( x )  ( x )

/* ---- CMod_LoadShaders  0x00418FF0 ---- VERIFIED */
static void CMod_LoadShaders( lump_t *l ) {
	dmaterial_t *in;
	int count;

	in = (dmaterial_t *)( cmod_base + l->fileofs );
	if ( l->filelen % sizeof( dmaterial_t ) ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadShaders: funny lump size" );
	}
	count = l->filelen / sizeof( dmaterial_t );
	if ( count < 1 ) {
		Com_Error( ERR_DROP, "\x15" "Map with no shaders" );
	}

	cm_materials = (dmaterial_t *)Hunk_AllocAlignInternal(
		sizeof( dmaterial_t ) * count + sizeof( dmaterial_t ), 32 ) + 1;
	cm_numMaterials = count;

	Com_Memcpy( cm_materials, in, count * sizeof( dmaterial_t ) );
}

/* ---- CMod_LoadSubmodels  0x00419070 ---- VERIFIED */
static void CMod_LoadSubmodels( lump_t *l ) {
	const float *in;
	cmodel_t *out;
	int *indexes;
	int count, i, j, n;

	in = (const float *)( cmod_base + l->fileofs );
	if ( (unsigned int)l->filelen % 48 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadSubmodels: funny lump size" );
	}
	count = (unsigned int)l->filelen / 48;
	if ( count < 1 ) {
		Com_Error( ERR_DROP, "\x15" "Map with no models" );
	}

	cm_cmodels = (cmodel_t *)Hunk_AllocAlignInternal( count * sizeof( cmodel_t ), 32 );
	cm_numSubModels = count;

	if ( count > MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "\x15" "MAX_SUBMODELS exceeded" );
	}

	for ( i = 0; i < count; i++, in += 12 ) {
		const int *ini = (const int *)in;

		out = &cm_cmodels[i];

		out->mins[0] = in[0] - 1.0f;
		out->maxs[0] = in[3] + 1.0f;
		out->mins[1] = in[1] - 1.0f;
		out->maxs[1] = in[4] + 1.0f;
		out->mins[2] = in[2] - 1.0f;
		out->maxs[2] = in[5] + 1.0f;

		if ( i == 0 ) {
			continue;
		}

		n = ini[11];
		out->leaf.numLeafBrushes = (unsigned short)n;
		if ( (unsigned short)n != n ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadSubmodels: numLeafBrushes exceeded" );
		}

		indexes = (int *)Hunk_AllocAlignInternal( n * sizeof( int ), 32 );
		out->leaf.firstLeafBrush = indexes - cm_leafbrushes;
		for ( j = 0; j < n; j++ ) {
			indexes[j] = ini[10] + j;
		}

		n = ini[9];
		out->leaf.numLeafSurfaces = (unsigned short)n;
		if ( (unsigned short)n != n ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadSubmodels: numLeafSurfaces exceeded" );
		}
		n = ini[8];
		out->leaf.firstLeafSurface = (unsigned short)n;
		if ( (unsigned short)n != n ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadSubmodels: firstLeafSurface exceeded" );
		}
	}
}

/* ---- CMod_LoadNodes  0x00419210 ---- VERIFIED */
static void CMod_LoadNodes( lump_t *l ) {
	const int *in;
	cNode_t *out;
	int count, i, j, child;

	in = (const int *)( cmod_base + l->fileofs );
	if ( (unsigned int)l->filelen % 36 ) {
		Com_Error( ERR_DROP, "\x15" "MOD_LoadBmodel: funny lump size" );
	}
	count = (unsigned int)l->filelen / 36;
	if ( count < 1 ) {
		Com_Error( ERR_DROP, "\x15" "Map has no nodes" );
	}

	cm_nodes = (cNode_t *)Hunk_AllocAlignInternal( count * sizeof( cNode_t ), 32 );
	cm_numNodes = count;

	out = cm_nodes;
	for ( i = 0; i < count; i++, out++, in += 9 ) {
		out->plane = cm_planes + in[0];
		for ( j = 0; j < 2; j++ ) {
			child = in[1 + j];
			out->children[j] = (short)child;
			if ( (short)child != child ) {
				Com_Error( ERR_DROP, "\x15" "CMod_LoadNodes: children exceeded" );
			}
		}
	}
}

/* ---- CMod_LoadBrushes  0x00419300 ---- VERIFIED */
static void CMod_LoadBrushes( lump_t *brushLump, lump_t *sidesLump ) {
	const short *in;
	const int *sideIn;
	cbrush_t *out;
	cbrushside_t *side;
	int numBrushes, numSides;
	int i, axis, end, materialNum;

	in = (const short *)( cmod_base + brushLump->fileofs );
	if ( brushLump->filelen & 3 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: funny lump size" );
	}
	numBrushes = (unsigned int)brushLump->filelen >> 2;

	sideIn = (const int *)( cmod_base + sidesLump->fileofs );
	if ( sidesLump->filelen & 7 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: funny lump size" );
	}

	numSides = ( (unsigned int)sidesLump->filelen >> 3 ) - 6 * numBrushes;
	if ( numSides < 0 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: bad side count" );
	}

	cm_brushsides = numSides ? (cbrushside_t *)Hunk_AllocAlignInternal(
		numSides * sizeof( cbrushside_t ), 32 ) : NULL;
	side = cm_brushsides;
	cm_numBrushSides = numSides;

	cm_brushes = (cbrush_t *)Hunk_AllocAlignInternal(
		( numBrushes + 1 ) * sizeof( cbrush_t ), 32 );
	cm_numBrushes = numBrushes;

	out = cm_brushes;

	for ( i = 0; i < numBrushes; i++, out++, in += 2 ) {
		out->numSides = in[0] - 6;
		if ( out->numSides < 0 ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: brush has less than 6 sides" );
		}
		out->sides = out->numSides ? side : NULL;

		for ( axis = 0; axis < 3; axis++ ) {
			for ( end = 0; end < 2; end++, sideIn += 2 ) {
				( &out->mins[0] )[end * 3 + axis] = *(const float *)&sideIn[0];

				materialNum = sideIn[1];
				if ( materialNum < 0 || materialNum >= cm_numMaterials ) {
					Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: bad shaderNum: %i",
							   materialNum );
				}
				out->axialMaterialNum[end * 3 + axis] = (short)materialNum;
				if ( (short)materialNum != materialNum ) {
					Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: axialShaderNum exceeded" );
				}
			}
		}

		{
			int n;
			for ( n = 0; n < out->numSides; n++, side++, sideIn += 2 ) {
				side->plane = cm_planes + sideIn[0];
				side->materialNum = sideIn[1];
				if ( side->materialNum < 0 || side->materialNum >= cm_numMaterials ) {
					Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: bad shaderNum: %i",
							   side->materialNum );
				}
			}
		}

		materialNum = in[1];
		if ( materialNum < 0 || materialNum >= cm_numMaterials ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadBrushes: bad shaderNum: %i", materialNum );
		}
		out->contents = cm_materials[materialNum].contentFlags & 0xDFFF7FFB;
	}
}

/* ---- CMod_LoadLeafs  0x00419560 ---- VERIFIED */
static void CMod_LoadLeafs( lump_t *l ) {
	const int *in;
	cLeaf_t *out;
	int count, i;
	int cellValue, clusterValue, areaValue, brushCountValue, firstSurfaceValue, surfaceCountValue;

	in = (const int *)( cmod_base + l->fileofs );
	if ( (unsigned int)l->filelen % 36 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafs: funny lump size" );
	}
	count = (unsigned int)l->filelen / 36;
	if ( count < 1 ) {
		Com_Error( ERR_DROP, "\x15" "Map with no leafs" );
	}

	cm_leafs = (cLeaf_t *)Hunk_AllocAlignInternal( count * sizeof( cLeaf_t ), 32 );
	cm_numLeafs = count;

	out = cm_leafs;
	for ( i = 0; i < count; i++, in += 9, out++ ) {
		cellValue = in[6];
		out->cellnum = (short)cellValue;
		if ( (short)cellValue != cellValue ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafs: cellnum exceeded" );
		}
		clusterValue = in[0];
		out->cluster = (short)clusterValue;
		if ( (short)clusterValue != clusterValue ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafs: cluster exceeded" );
		}
		areaValue = in[1];
		out->area = (short)areaValue;
		if ( (short)areaValue != areaValue ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafs: area exceeded" );
		}
		out->firstLeafBrush = in[4];
		brushCountValue = in[5];
		out->numLeafBrushes = (unsigned short)brushCountValue;
		if ( (unsigned short)brushCountValue != brushCountValue ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafs: numLeafBrushes exceeded" );
		}
		firstSurfaceValue = in[2];
		out->firstLeafSurface = (unsigned short)firstSurfaceValue;
		if ( (unsigned short)firstSurfaceValue != firstSurfaceValue ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafs: firstLeafSurface exceeded" );
		}
		surfaceCountValue = in[3];
		out->numLeafSurfaces = (unsigned short)surfaceCountValue;
		if ( (unsigned short)surfaceCountValue != surfaceCountValue ) {
			Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafs: numLeafSurfaces exceeded" );
		}

		if ( clusterValue >= cm_numClusters ) {
			cm_numClusters = clusterValue + 1;
		}
		if ( areaValue >= cm_numAreas ) {
			cm_numAreas = areaValue + 1;
		}
	}

	cm_areas = (cArea_t *)Hunk_AllocAlignInternal(
		cm_numAreas * sizeof( cArea_t ), 32 );
	cm_areaPortals = (int *)Hunk_AllocAlignInternal(
		cm_numAreas * cm_numAreas * sizeof( int ), 32 );
}

/* ---- CMod_LoadPlanes  0x00419700 ---- VERIFIED */
static void CMod_LoadPlanes( lump_t *l ) {
	const float *in;
	cplane_t *out;
	int count;
	int i;
	byte bits;
	int j;

	in = (const float *)( cmod_base + l->fileofs );
	if ( l->filelen & 15 ) {
		Com_Error( ERR_DROP, "\x15" "MOD_LoadBmodel: funny lump size" );
	}
	count = (unsigned int)l->filelen >> 4;
	if ( count < 1 ) {
		Com_Error( ERR_DROP, "\x15" "Map with no planes" );
	}

	cm_planes = (cplane_t *)Hunk_AllocAlignInternal( count * sizeof( cplane_t ), 32 );
	cm_numPlanes = count;

	out = cm_planes;
	for ( i = 0; i < count; i++, in += 4, out++ ) {
		bits = 0;
		for ( j = 0; j < 3; j++ ) {
			/* ( in[j] ): retail's LittleFloat is an identity here; its parentheses steer VC7's x87 schedule */
			out->normal[j] = ( in[j] );
			if ( out->normal[j] < 0 ) {
				bits |= 1 << j;
			}
		}

		out->dist = ( in[3] );
		out->type = ( out->normal[0] == 1.0 ? 0 : ( out->normal[1] == 1.0 ? 1 : ( out->normal[2] == 1.0 ? 2 : 3 ) ) );
		out->signbits = bits;
	}
}

/* ---- CMod_LoadLeafBrushes  0x00419810 ---- VERIFIED */
static void CMod_LoadLeafBrushes( lump_t *l ) {
	const int *in;
	int *out;
	int count, i;

	in = (const int *)( cmod_base + l->fileofs );
	if ( l->filelen & 3 ) {
		Com_Error( ERR_DROP, "\x15" "MOD_LoadBmodel: funny lump size" );
	}
	count = (unsigned int)l->filelen >> 2;

	cm_leafbrushes = (int *)Hunk_AllocAlignInternal( count * sizeof( int ) + sizeof( int ), 32 );
	cm_numLeafBrushes = count;

	out = cm_leafbrushes;
	for ( i = 0; i < count; i++, in++, out++ ) {
		*out = *in;
	}
}

/* ---- CMod_LoadLeafSurfaces  0x00419870 ---- VERIFIED */
static void CMod_LoadLeafSurfaces( lump_t *l ) {
	const int *in;
	int *out;
	int count, i;

	in = (const int *)( cmod_base + l->fileofs );
	if ( l->filelen & 3 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafSurfaces: funny lump size" );
	}
	count = (unsigned int)l->filelen >> 2;

	cm_leafsurfaces = (int *)Hunk_AllocAlignInternal( count * sizeof( int ), 32 );
	cm_numLeafSurfaces = count;

	out = cm_leafsurfaces;
	for ( i = 0; i < count; i++, in++, out++ ) {
		*out = *in;
	}
}

#define MAX_TERRAIN_VERTS       0x2000
#define MAX_TERRAIN_INDEXES     0xC000

/* ---- CMod_LoadLeafCurvesAndTerrain  0x004198D0 ---- VERIFIED */
static void CMod_LoadLeafCurvesAndTerrain( lump_t *surfLump, lump_t *vertLump,
										   lump_t *indexLump ) {
	const dcollisionsurface_t *in;
	const float *verts;
	const short *indexes;
	cPatch_t *out;
	int count, i, j, n;
	/* retail keeps both scratch arrays on the stack (0x30044-byte frame,
	   points at esp+48h, heightIndexes at esp+18048h), not in .bss */
	vec3_t points[MAX_TERRAIN_VERTS];
	short heightIndexes[MAX_TERRAIN_INDEXES];

	in = (const dcollisionsurface_t *)( cmod_base + surfLump->fileofs );
	if ( surfLump->filelen & 15 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafpatchesAndTerrain: funny lump size" );
	}
	/* retail 0x00419932 shr / 0x0041992E div: lump sizes are treated as unsigned */
	count = (unsigned int)surfLump->filelen >> 4;

	verts = (const float *)( cmod_base + vertLump->fileofs );
	if ( (unsigned int)vertLump->filelen % 12 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafpatchesAndTerrain: funny lump size" );
	}

	indexes = (const short *)( cmod_base + indexLump->fileofs );
	if ( indexLump->filelen & 1 ) {
		Com_Error( ERR_DROP, "\x15" "CMod_LoadLeafpatchesAndTerrain: funny lump size" );
	}

	cm_numPatches = count;
	cm_patches = (cPatch_t *)Hunk_AllocAlignInternal( count * sizeof( cPatch_t ), 32 );

	out = cm_patches;
	for ( i = 0; i < count; i++, in++, out++ ) {
		n = ( in->materialIndex );
		out->checkcount = 0;
		out->materialNum = n;
		out->contents = cm_materials[n].contentFlags;

		if ( in->isTerrain ) {
			const float *src;
			int numVerts;

			numVerts = in->a;
			if ( (unsigned int)numVerts > MAX_TERRAIN_VERTS ) {
				Com_Error( ERR_DROP,
						   "\x15" "CMod_LoadLeafpatchesAndTerrain: more than %i verts in terrain\n",
						   MAX_TERRAIN_VERTS );
			}
			src = verts + 3 * in->firstIndex;
			for ( j = 0; j < numVerts; j++, src += 3 ) {
				points[j][0] = ( src[0] );
				points[j][1] = ( src[1] );
				points[j][2] = ( src[2] );
			}

			/* Retail sign-extends the 16-bit index count (movsx) before the
			   unsigned compare, so a raw count of 0x8000 or more goes negative
			   and fails the check with Com_Error, as a count above 0xC000 does. */
			n = in->b;
			if ( (unsigned int)n > MAX_TERRAIN_INDEXES ) {
				Com_Error( ERR_DROP,
						   "\x15" "CMod_LoadLeafpatchesAndTerrain: more than %i indexes in terrain\n",
						   MAX_TERRAIN_INDEXES );
			}
			{
				const short *isrc = indexes + in->secondIndex;
				for ( j = 0; j < n; j++ ) {
					heightIndexes[j] = ( isrc[j] );
				}
			}

			out->pc = NULL;
			out->tc = CM_GenerateTerrainCollide( n, heightIndexes,
												 numVerts, points,
												 &out->mins );
		} else {
			const float *src;
			int width, height;
			int maxError;

			width = in->a;
			height = in->b;
			n = width * height;
			if ( (unsigned int)n > MAX_TERRAIN_VERTS ) {
				Com_Error( ERR_DROP,
						   "\x15" "CMod_LoadLeafpatchesAndTerrain: more than %i verts in curve\n",
						   MAX_TERRAIN_VERTS );
			}
			src = verts + 3 * in->secondIndex;
			for ( j = 0; j < n; j++, src += 3 ) {
				points[j][0] = ( src[0] );
				points[j][1] = ( src[1] );
				points[j][2] = ( src[2] );
			}

			/* maxError is read into its own local only at the call: that is
			   what makes retail load height after pushing it (0x00419B07). */
			maxError = in->firstIndex;
			out->pc = CM_GeneratePatchCollide( width, height, maxError,
												   points, &out->mins );
			out->tc = NULL;
		}

		/* Retail inlines Sys_LoadingKeepAlive here. The gate tests the total
		   patch count, not i, so it is loop-invariant: it runs every
		   iteration or never. */
		if ( ( count & 3 ) == 3 ) {
			Sys_LoadingKeepAlive();
		}
	}
}

/* ---- CMod_LoadEntityString  0x00419BC0 ---- VERIFIED */
static void CMod_LoadEntityString( lump_t *l ) {
	const byte *entitySource; int entityOffset; cm_entityString = (char *)Hunk_AllocAlignInternal( l->filelen, 32 );
	cm_entityStringLen = l->filelen;
	entitySource=cmod_base; entityOffset=l->fileofs; Com_Memcpy( cm_entityString, entitySource + entityOffset, l->filelen );
}

/* ---- CMod_LoadVisibility  0x00419C00 ---- VERIFIED */
static void CMod_LoadVisibility( lump_t *l ) {
	const int *in;
	unsigned int len;

	len = l->filelen;
	if ( !len ) {
		cm_clusterBytes = ( cm_numClusters + 31 ) & ~31;
		cm_visibility = (byte *)Hunk_AllocAlignInternal( cm_clusterBytes, 32 );
		Com_Memset( cm_visibility, 255, cm_clusterBytes );
		return;
	}

	in = (const int *)( cmod_base + l->fileofs );
	cm_vised = qtrue;
	cm_visibility = (byte *)Hunk_AllocAlignInternal( len, 32 );
	cm_numClusters = in[0];
	cm_clusterBytes = in[1];
	Com_Memcpy( cm_visibility, in + 2, len - 8 );
}

static void CM_ClearMap( void ) {
	Com_Memset( cm_mapName, 0, sizeof( cm_mapName ) );

	cm_numStaticModels = 0;     cm_staticModels = NULL;
	cm_numMaterials = 0;        cm_materials = NULL;
	cm_numBrushSides = 0;       cm_brushsides = NULL;
	cm_numPlanes = 0;           cm_planes = NULL;
	cm_numNodes = 0;            cm_nodes = NULL;
	cm_numLeafs = 0;            cm_leafs = NULL;
	cm_numLeafBrushes = 0;      cm_leafbrushes = NULL;
	cm_numLeafSurfaces = 0;     cm_leafsurfaces = NULL;
	cm_numSubModels = 0;        cm_cmodels = NULL;
	cm_numBrushes = 0;          cm_brushes = NULL;
	cm_numClusters = 0;         cm_clusterBytes = 0;
	cm_visibility = NULL;       cm_vised = qfalse;
	cm_entityStringLen = 0;     cm_entityString = NULL;
	cm_numAreas = 0;            cm_areas = NULL;
	cm_areaPortals = NULL;
	cm_numPatches = 0;          cm_patches = NULL;
	cm_floodvalid = 0;          cm_checkcount = 0;

	box_brush = NULL;
	Com_Memset( &cm_boxModel, 0, sizeof( cm_boxModel ) );

	CM_ClearWorldSectors();
}

/* ---- CM_LoadMap  0x00419C90 ---- VERIFIED */
void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) {
	dheader_t header;
	char mapName[MAX_QPATH];
	void *buf;
	int length;
	int i;

	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "\x15" "CM_LoadMap: NULL name" );
	}

	strncpy( mapName, name, 63 );
	mapName[63] = 0;

	cm_noCurves = Cvar_Get( "cm_noCurves", "0", CVAR_CHEAT );
	cm_playerCurveClip = Cvar_Get( "cm_playerCurveClip", "1", CVAR_CHEAT | CVAR_ARCHIVE );

	Com_DPrintf( "CM_LoadMap( %s, %i )\n", mapName, clientload );

	if ( clientload && com_sv_running->integer ) {
		*checksum = cm_lastChecksum;
		return;
	}

	Com_Memset( &cm, 0, sizeof(cm) );

	length = FS_ReadFile( mapName, &buf );
	if ( !buf ) {
		Com_Error( ERR_DROP, va( "EXE_ERR_COULDNT_LOAD\x15%s", mapName ) );
	}

	cm_lastChecksum = Com_BlockChecksum( buf, length );
	*checksum = cm_lastChecksum;

	header = *(dheader_t *)buf;
	for ( i = 0; i < sizeof( dheader_t ) / 4; i++ ) {
		( (int *)&header )[i] = CM_LittleLong( ( (int *)&header )[i] );
	}

	if ( header.version != CM_BSP_VERSION ) {
		Com_Error( ERR_DROP, va(
			"EXE_ERR_WRONG_MAP_VERSION_NUM\x15%s\x15(%i \x14" "EXE_ERR_SHOULD_BE\x15 %i)",
			mapName, header.version, CM_BSP_VERSION ) );
	}

	cmod_base = (byte *)buf;

	CMod_LoadShaders( &header.lumps[LUMP_MATERIALS] );
	CMod_LoadPlanes( &header.lumps[LUMP_PLANES] );
	CMod_LoadBrushes( &header.lumps[LUMP_BRUSHES], &header.lumps[LUMP_BRUSHSIDES] );
	CMod_LoadNodes( &header.lumps[LUMP_NODES] );
	CMod_LoadLeafs( &header.lumps[LUMP_LEAFS] );
	CMod_LoadLeafBrushes( &header.lumps[LUMP_LEAFBRUSHES] );
	CMod_LoadLeafSurfaces( &header.lumps[LUMP_LEAFSURFACES] );
	CMod_LoadLeafCurvesAndTerrain( &header.lumps[LUMP_LEAFSURFACES_D],
								   &header.lumps[LUMP_COLLISIONVERTS],
								   &header.lumps[LUMP_COLLISIONINDEX] );
	CMod_LoadSubmodels( &header.lumps[LUMP_MODELS] );
	CMod_LoadVisibility( &header.lumps[LUMP_VISIBILITY] );
	CMod_LoadEntityString( &header.lumps[LUMP_ENTITIES] );

	FS_FreeFile( buf );

	CM_InitBoxHull();
	CM_FloodAreaConnections();
	CM_ClearWorld();
	CM_LoadStaticModels();

	if ( !clientload ) {
		strncpy( cm_mapName, mapName, 63 );
		cm_mapName[63] = 0;
	}
}

/* ---- CM_SaveLump  0x00419F00 ---- VERIFIED */
void CM_SaveLump( int lump, const void *data, int length, unsigned int *checksum ) {
	dheader_t header;
	void *buf;
	byte *out;
	qboolean wasReadOnly;
	fileHandle_t f;
	size_t total;
	int ofs, i;

	FS_ReadFile( cm_mapName, &buf );
	if ( !buf ) {
		Com_Error( ERR_DROP, va( "EXE_ERR_COULDNT_LOAD\x15%s", cm_mapName ) );
	}

	header = *(dheader_t *)buf;
	for(i=0;i<sizeof(dheader_t)/4;i++){ ((int *)&header)[i]=CM_LittleLong(((int *)&header)[i]); }

	total = CM_HEADER_SIZE;
	for ( i = 0; i < CM_LUMPS; i++ ) {
		if ( i == lump ) {
			header.lumps[i].filelen = length;
		}
		total += ( header.lumps[i].filelen + 3 ) & ~3;
	}

	out = (byte *)malloc( total );
	if ( !out ) {
		Sys_OutOfMemoryPrep();
		MessageBoxA( 0, SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_BODY" ),
					 SEH_GetLocalizedString_m( "WIN_OUT_OF_MEM_TITLE" ), CM_MB_ICONHAND );
		exit( -1 );
	}
	Com_Memset( out, 0, total );

	ofs = CM_HEADER_SIZE;
	for ( i = 0; i < CM_LUMPS; i++ ) {
		if ( header.lumps[i].filelen ) {
			const void *src;

			if ( i != lump ) {
				src = (const byte *)buf + header.lumps[i].fileofs;
			} else {
				src = data;
			}
			Com_Memcpy( out + ofs, src, header.lumps[i].filelen );
		}
		header.lumps[i].fileofs = ofs;
		ofs += ( header.lumps[i].filelen + 3 ) & ~3;
	}

	for(i=0;i<sizeof(dheader_t)/4;i++){ ((int *)&header)[i]=CM_LittleLong(((int *)&header)[i]); }
	Com_Memcpy( out, &header, CM_HEADER_SIZE );

	wasReadOnly = FS_MakeReadOnly( cm_mapName, qfalse );
	f = FS_FOpenFileWrite( cm_mapName );
	if ( !f ) {
		Com_Printf( "Failed to open %s\n", cm_mapName );
	} else {
		FS_Write( out, ofs, f );
		FS_FCloseFile( f );
	}

	if ( wasReadOnly ) {
		/* inlined at retail 0x0041A09A; the first FS_MakeReadOnly stays a call */
		FS_MakeReadOnly( cm_mapName, qtrue );
	}

	if ( checksum ) {
		*checksum = Com_BlockChecksum( out, ofs );
	}

	free( out );
	FS_FreeFile( buf );
}

/* ---- CM_LoadLump_m  0x0041A180 ---- VERIFIED */
int CM_LoadLump_m( int lump, void **data ) {
	dheader_t header;
	fileHandle_t f;
	int len;
	int i;

	fs_loadingMode = 1;
	FS_FOpenFileRead( cm_mapName, &f, qfalse );
	if ( !f ) {
		Com_Error( ERR_DROP, va( "EXE_ERR_COULDNT_LOAD\x15%s", cm_mapName ) );
	}

	FS_Read( &header, CM_HEADER_SIZE, f );
	for ( i = 0; i < sizeof( dheader_t ) / 4; i++ ) {
		( (int *)&header )[i] = CM_LittleLong( ( (int *)&header )[i] );
	}

	len = header.lumps[lump].filelen;
	if ( !len ) {
		FS_FCloseFile( f );
		return 0;
	}

	FS_Seek( f, header.lumps[lump].fileofs - CM_HEADER_SIZE, FS_SEEK_CUR );
	cm_lumpData = Hunk_AllocateTempMemoryInternal( len );
	FS_Read( cm_lumpData, len, f );
	FS_FCloseFile( f );

	*data = cm_lumpData;
	return len;
}

/* ---- CM_FreeLump_m  0x0041A250 ---- VERIFIED */
void CM_FreeLump_m( void ) { extern int hunk_low_temp; int *header; int delta; if (!s_hunkData) { free(cm_lumpData); return; } header=(int *)cm_lumpData-4; if(header[0]!=(int)0x89537892) Com_Error(ERR_FATAL,"\x15Hunk_FreeTempMemory: bad magic"); delta=header[1]; header[0]=(int)0x89537893; hunk_low_temp-=delta; }

/* ---- CM_ClipHandleToModel  0x0041A2A0 ---- VERIFIED */
__declspec(noinline) cmodel_t *CM_ClipHandleToModel( clipHandle_t handle ) {
	if ( handle < 0 ) {
		Com_Error( ERR_DROP, "\x15" "CM_ClipHandleToModel: bad handle %i", handle );
	}
	if ( handle < cm_numSubModels ) {
		return &cm_cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE || handle == CAPSULE_MODEL_HANDLE ) {
		return &cm_boxModel;
	}
	if ( handle < MAX_SUBMODELS ) {
		Com_Error( ERR_DROP, "\x15" "CM_ClipHandleToModel: bad handle %i < %i < %i",
				   cm_numSubModels, handle, MAX_SUBMODELS );
	}
	Com_Error( ERR_DROP, "\x15" "CM_ClipHandleToModel: bad handle %i",
			   handle + MAX_SUBMODELS );

	return NULL;
}

/* ---- CM_InlineModel  0x0041A320 ---- VERIFIED */
clipHandle_t CM_InlineModel( int index ) {
	if ( index < 0 || index >= cm_numSubModels ) {
		Com_Error( ERR_DROP, "\x15" "CM_InlineModel: bad number" );
	}
	return index;
}

/* ---- CM_NumClusters  0x0041A340 ---- VERIFIED */
int CM_NumClusters( void ) {
	return cm_numClusters;
}

/* ---- CM_NumInlineModels  0x0041A350 ---- VERIFIED */
int CM_NumInlineModels( void ) {
	return cm_numSubModels;
}

/* ---- CM_EntityString  0x0041A360 ---- VERIFIED */
const char *CM_EntityString( void ) {
	return cm_entityString;
}

/* ---- CM_LeafCluster  0x0041A370 ---- VERIFIED */
int CM_LeafCluster( int leafnum ) {
	return cm_leafs[leafnum].cluster;
}

/* ---- CM_LeafArea  0x0041A380 ---- VERIFIED */
int CM_LeafArea( int leafnum ) {
	return cm_leafs[leafnum].area;
}

/* ---- CM_InitBoxHull  0x0041A390 ---- VERIFIED */
void CM_InitBoxHull( void ) {
	int i;

	box_brush = &cm_brushes[cm_numBrushes];
	box_brush->numSides = 0;
	box_brush->sides = NULL;
	box_brush->contents = -1;
	box_brush->axialMaterialNum[0] = -1;
box_brush->axialMaterialNum[1] = -1;
box_brush->axialMaterialNum[2] = -1;
box_brush->axialMaterialNum[3] = -1;
box_brush->axialMaterialNum[4] = -1;
box_brush->axialMaterialNum[5] = -1;

	cm_boxModel.leaf.numLeafBrushes = 1;
	cm_boxModel.leaf.firstLeafBrush = cm_numLeafBrushes;
	cm_leafbrushes[cm_numLeafBrushes] = cm_numBrushes;
}

/* ---- CM_TempBoxModel  0x0041A420 ---- VERIFIED */
clipHandle_t CM_TempBoxModel( const vec3_t mins, const vec3_t maxs,
							  int contents, qboolean capsule ) {
	VectorCopy( mins, cm_boxModel.mins );
	VectorCopy( maxs, cm_boxModel.maxs );

	VectorCopy( mins, box_brush->mins );
	VectorCopy( maxs, box_brush->maxs );
	box_brush->contents = contents;

	return capsule ? CAPSULE_MODEL_HANDLE : BOX_MODEL_HANDLE;
}

/* ---- CM_SetTempBoxModelContents  0x0041A4C0 ---- VERIFIED */
int CM_SetTempBoxModelContents( int contents ) {
	( void )contents;
	return box_brush->contents;
}

/* ---- CM_ModelBounds  0x0041A4D0 ---- VERIFIED */
void CM_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	cmodel_t *cmod;

	cmod = CM_ClipHandleToModel( model );
	VectorCopy( cmod->mins, mins );
	VectorCopy( cmod->maxs, maxs );
}
