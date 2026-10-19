/*
 * script/scr_memorytree.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_memorytree.cpp
 *
 * Retail range 0x0046DB80-0x0046E580, 16 functions.
 *
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "scr_local.h"

/* retail 0x008E6318: mt_bitWidth (0x00966318) directly follows the arena, so MT_Init's
   arena-end address (scrMemTree_constBase/constEnd) is also the table's */
#pragma bss_seg(push, ".bss$mt0")
scrMemBlock_t   scrMemTree_blocks[SCR_MT_BLOCK_COUNT];
#pragma bss_seg(pop)

unsigned short  scrMemTree_freeRoots[SCR_MT_BUCKET_COUNT];

int             scrMemTree_allocBuckets;    /* retail 0x008E6310 */
int             scrMemTree_allocInstances;  /* retail 0x008E60D8 */

/* Retail's vector ref-counting (AddRefToVector, RemoveRefToVector, VM_Execute, ...) skips a
   vector inside [scrMemTree_constBase, +scrMemTree_constLen).  Only MT_Init writes these (an
   empty range at the arena end), so the tree's unconditional ref-counting is equivalent.
   scrMemTree_constEnd is never read. */
const char     *scrMemTree_constBase;       /* retail 0x008E6204 */
const char     *scrMemTree_constEnd;        /* retail 0x008E60D4 */
int             scrMemTree_constLen;        /* retail 0x00966418 */

/* retail 0x008E6208: the arena base MT_Init publishes for SL_ConvertToString and the
   renderer/xanim string readers (scr_local.h) */
int             GetRefString_var;

/* MT_InitBits fills these once.  retail 0x008E60E0 / 0x008E6210 / 0x00966318 */
static unsigned char    mt_popcount[SCR_MT_LOOKUP_COUNT];
static unsigned char    mt_trailingZeroBits[SCR_MT_LOOKUP_COUNT];
#pragma bss_seg(push, ".bss$mt1")
static unsigned char    mt_bitWidth[SCR_MT_LOOKUP_COUNT];
#pragma bss_seg(pop)

/* ---- MT_InitBits  0x0046DC90 ---- VERIFIED */
static void MT_InitBits( void ) {
	int             value;
	int             bits;
	int             count;

	for ( value = 0; value < SCR_MT_LOOKUP_COUNT; value++ ) {
		count = 0;
		for ( bits = value; bits; bits >>= 1 ) {
			if ( bits & 1 ) {
				count++;
			}
		}
		mt_popcount[value] = count;

		count = 8;
		while ( ( ( ( 1 << count ) - 1 ) & value ) != 0 ) {
			count--;
		}
		mt_trailingZeroBits[value] = count;

		count = 0;
		for ( bits = value; bits; bits >>= 1 ) {
			count++;
		}
		mt_bitWidth[value] = count;
	}
}

/* ---- MT_GetScore  0x0046DD00 ---- VERIFIED */
static int MT_GetScore( int blockIndex ) {
	int distance;
	int lowByte;
	int highByte;
	int score;
	int trailingZeros;

	distance = SCR_MT_BLOCK_COUNT - blockIndex;
	lowByte  = (unsigned char)distance;
	highByte = ( distance >> 8 ) & 0xFF;

	score = distance - mt_popcount[highByte] - mt_popcount[lowByte];

	trailingZeros = mt_trailingZeroBits[lowByte];
	if ( lowByte == 0 ) {
		trailingZeros += mt_trailingZeroBits[highByte];
	}

	return score + ( 1 << trailingZeros );
}

/* ---- MT_GetSubTreeSize  0x0046DB80 ---- VERIFIED */
static int MT_GetSubTreeSize( int blockIndex ) {
	if ( blockIndex == 0 ) {
		return 0;
	}
	return MT_GetSubTreeSize( scrMemTree_blocks[blockIndex].node.right )
		 + MT_GetSubTreeSize( scrMemTree_blocks[blockIndex].node.left )
		 + 1;
}

/* ---- MT_DumpTree  0x0046DBC0 ---- VERIFIED */
void MT_DumpTree( void ) {
	int bucket;
	int nodes;

	Com_Printf( "********************************\n" );
	for ( bucket = 0; bucket <= SCR_MT_MAX_BUCKET; bucket++ ) {
		nodes = MT_GetSubTreeSize( scrMemTree_freeRoots[bucket] );
		Com_Printf( "%d subtree has %d * %d = %d free buckets\n",
					bucket, nodes, 1 << bucket, nodes * ( 1 << bucket ) );
	}
	Com_Printf( "********************************\n" );
	Com_Printf( "********************************\n" );
	Com_Printf( "total memory alloc buckets: %d (%d instances)\n",
				scrMemTree_allocBuckets, scrMemTree_allocInstances );
	Com_Printf( "total memory free buckets: %d\n",
				0xFFFF - scrMemTree_allocBuckets );
	Com_Printf( "********************************\n" );
}

/* ---- Scr_GetStringUsage  0x0046DC80 ---- VERIFIED */
int Scr_GetStringUsage( void ) {
	return scrMemTree_allocBuckets;
}

/* ---- MT_Error  0x0046E210 ---- VERIFIED
 * Retail inlines Scr_TerminalError (0x0047DFD0) after the va: it dumps the
 * script threads, sets scrVmPub_terminal_error and scrVarPub_error_message,
 * then throws to the VM error frame when scrVmGlob_funcCount != 0, else
 * Com_Error( ERR_DROP, "\x15%s" ).  MT_AllocIndex (0x0046E335) inlines the
 * same body; the tree routes it through here. */
static void MT_Error( const char *caller, int size ) {
	MT_DumpTree();
	Scr_TerminalError( va( "%s: failed allocation of %d bytes for script usage", caller, size ) );
}

/* ---- MT_GetSize  0x0046E280 ---- VERIFIED */
static int MT_GetSize( int size ) {
	int blocks;

	if ( size < SCR_MT_MAX_ALLOC_SIZE ) {
		blocks = ( size + SCR_MT_BLOCK_SIZE - 1 ) / SCR_MT_BLOCK_SIZE - 1;
		if ( blocks <= 255 ) {
			return mt_bitWidth[blocks];
		}
		return mt_bitWidth[blocks >> 8] + 8;
	}
	MT_Error( "MT_GetSize: max allocation exceeded", size );
	return 0;
}

/* ---- MT_AddMemoryNode  0x0046DD50 ---- VERIFIED */
static void MT_AddMemoryNode( int blockIndex, int bucket ) {
	unsigned short *link;
	int             current;
	int             blockKey;
	int             centre;
	int             span;

	link = &scrMemTree_freeRoots[bucket];
	current = *link;

	if ( current != 0 ) {
		blockKey = MT_GetScore( blockIndex );
		centre = 0;
		span = SCR_MT_BLOCK_COUNT;

		do {
			if ( MT_GetScore( current ) < blockKey ) {
				for ( ;; ) {
					*link = (unsigned short)blockIndex;
					scrMemTree_blocks[blockIndex] = scrMemTree_blocks[current];

					if ( current == 0 ) {
						return;
					}

					span >>= 1;
					if ( current < centre ) {
						link = &scrMemTree_blocks[blockIndex].node.left;
						centre -= span;
					} else {
						link = &scrMemTree_blocks[blockIndex].node.right;
						centre += span;
					}
					blockIndex = current;
					current = *link;
				}
			}

			span >>= 1;
			if ( blockIndex < centre ) {
				link = &scrMemTree_blocks[current].node.left;
				centre -= span;
			} else {
				link = &scrMemTree_blocks[current].node.right;
				centre += span;
			}
			current = *link;
		} while ( current != 0 );
	}

	*link = (unsigned short)blockIndex;
	scrMemTree_blocks[blockIndex].node.left = 0;
	scrMemTree_blocks[blockIndex].node.right = 0;
}

/* ---- MT_RemoveMemoryNode  0x0046DE90 ---- VERIFIED */
static unsigned char MT_RemoveMemoryNode( int blockIndex, int bucket ) {
	unsigned short *link;
	int             current;
	int             next;
	int             centre;
	int             span;
	int             leftScore;
	int             rightScore;
	scrMemNode_t    replacement;
	scrMemNode_t    moved;

	centre = 0;
	span = SCR_MT_BLOCK_COUNT;
	link = &scrMemTree_freeRoots[bucket];

	for ( ;; ) {
		current = *link;
		if ( current == 0 ) {
			return qfalse;
		}
		if ( blockIndex == current ) {
			break;
		}
		if ( blockIndex == centre ) {
			return qfalse;
		}

		span >>= 1;
		if ( blockIndex < centre ) {
			link = &scrMemTree_blocks[current].node.left;
			centre -= span;
		} else {
			link = &scrMemTree_blocks[current].node.right;
			centre += span;
		}
	}

	replacement = scrMemTree_blocks[blockIndex].node;
	for ( ;; ) {
		if ( replacement.left == 0 ) {
			next = replacement.right;
			*link = (unsigned short)next;
			if ( next == 0 ) {
				return qtrue;
			}
			link = &scrMemTree_blocks[next].node.right;
		} else if ( replacement.right == 0 ) {
			next = replacement.left;
			*link = (unsigned short)next;
			link = &scrMemTree_blocks[next].node.left;
		} else {
			leftScore = MT_GetScore( replacement.left );
			rightScore = MT_GetScore( replacement.right );
			if ( leftScore < rightScore ) {
				next = replacement.right;
				*link = (unsigned short)next;
				link = &scrMemTree_blocks[next].node.right;
			} else {
				next = replacement.left;
				*link = (unsigned short)next;
				link = &scrMemTree_blocks[next].node.left;
			}
		}

		moved = replacement;
		replacement = scrMemTree_blocks[next].node;
		scrMemTree_blocks[next].node = moved;
	}
}

/* ---- MT_RemoveHeadMemoryNode  0x0046E020 ---- VERIFIED */
static void MT_RemoveHeadMemoryNode( int bucket ) {
	unsigned short *link;
	int             next;
	int             leftScore;
	int             rightScore;
	scrMemNode_t    replacement;
	scrMemNode_t    moved;

	link = &scrMemTree_freeRoots[bucket];
	replacement = scrMemTree_blocks[*link].node;

	for ( ;; ) {
		if ( replacement.left == 0 ) {
			next = replacement.right;
			*link = (unsigned short)next;
			if ( next == 0 ) {
				return;
			}
			link = &scrMemTree_blocks[next].node.right;
		} else if ( replacement.right == 0 ) {
			next = replacement.left;
			*link = (unsigned short)next;
			link = &scrMemTree_blocks[next].node.left;
		} else {
			leftScore = MT_GetScore( replacement.left );
			rightScore = MT_GetScore( replacement.right );
			if ( leftScore < rightScore ) {
				next = replacement.right;
				*link = (unsigned short)next;
				link = &scrMemTree_blocks[next].node.right;
			} else {
				next = replacement.left;
				*link = (unsigned short)next;
				link = &scrMemTree_blocks[next].node.left;
			}
		}

		moved = replacement;
		replacement = scrMemTree_blocks[next].node;
		scrMemTree_blocks[next].node = moved;
	}
}

/* ---- MT_Init  0x0046E190 ---- VERIFIED */
void MT_Init( void ) {
	int bucket;

	GetRefString_var = (int)scrMemTree_blocks;

	MT_InitBits();

	/* retail 0x0046E1A1 clears all 17 roots (rep stosd + stosw); pointer store because the
	   indexed form makes modern /GS add a range-check fast-fail retail never had */
	for ( bucket = 0; bucket < SCR_MT_BUCKET_COUNT; bucket++ ) {
		*( scrMemTree_freeRoots + bucket ) = 0;
	}

	scrMemTree_blocks[0].node.left = 0;
	scrMemTree_blocks[0].node.right = 0;

	scrMemTree_constBase = (const char *)&scrMemTree_blocks[SCR_MT_BLOCK_COUNT];
	scrMemTree_constEnd = (const char *)&scrMemTree_blocks[SCR_MT_BLOCK_COUNT];
	scrMemTree_constLen = 0;

	for ( bucket = 0; bucket < SCR_MT_MAX_BUCKET; bucket++ ) {
		MT_AddMemoryNode( 1 << bucket, bucket );
	}

	scrMemTree_allocInstances = 0;
	scrMemTree_allocBuckets = 0;
}

/* ---- MT_AllocIndex  0x0046E2D0 ---- VERIFIED */
unsigned short MT_AllocIndex( int size, int type ) {
	int             wanted;
	int             bucket;
	int             blockIndex;

	(void)type;

	wanted = MT_GetSize( size );

	for ( bucket = wanted; bucket <= SCR_MT_MAX_BUCKET; bucket++ ) {
		blockIndex = scrMemTree_freeRoots[bucket];
		if ( blockIndex ) {
			MT_RemoveHeadMemoryNode( bucket );

			while ( bucket != wanted ) {
				bucket--;
				MT_AddMemoryNode( blockIndex + ( 1 << bucket ), bucket );
			}

			scrMemTree_allocInstances++;
			scrMemTree_allocBuckets += 1 << wanted;

			return (unsigned short)blockIndex;
		}
	}

	MT_Error( "MT_AllocIndex", size );
	return 0;
}

/* ---- MT_FreeIndex  0x0046E3F0 ---- VERIFIED */
void MT_FreeIndex( unsigned short blockIndex, unsigned int size ) {
	int bucket;
	int bucketSize;

	bucket = MT_GetSize( (int)size );

	scrMemTree_allocInstances--;
	scrMemTree_allocBuckets -= 1 << bucket;

	bucketSize = 1 << bucket;
	while ( bucket != SCR_MT_MAX_BUCKET ) {
		if ( !MT_RemoveMemoryNode( blockIndex ^ bucketSize, bucket ) ) {
			break;
		}
		blockIndex = (unsigned short)( blockIndex & ~bucketSize );
		bucket++;
		bucketSize = 1 << bucket;
	}

	MT_AddMemoryNode( blockIndex, bucket );
}

/* ---- MT_Alloc  0x0046E4A0 ---- VERIFIED */
void *MT_Alloc( int size, int type ) {
	return &scrMemTree_blocks[MT_AllocIndex( size, type )];
}

/* ---- MT_Free  0x0046E4C0 ---- VERIFIED */
void MT_Free( void *p, unsigned int size ) {
	MT_FreeIndex( (unsigned short)( (scrMemBlock_t *)p - scrMemTree_blocks ), size );
}

/* ---- MT_Realloc  0x0046E4E0 ---- VERIFIED */
qboolean MT_Realloc( int oldSize, int newSize ) {
	return MT_GetSize( oldSize ) == MT_GetSize( newSize ) ? qtrue : qfalse;
}

#ifdef __cplusplus
}
#endif
