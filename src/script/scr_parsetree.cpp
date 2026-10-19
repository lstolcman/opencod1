/*
 * script/scr_parsetree.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_parsetree.cpp
 *
 * Retail range 0x0046F1B0-0x0046F7B9, 16 functions.
 *
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"

typedef unsigned int sval_t;

/* retail passes this union by value, which keeps arguments on the stack under LTCG */
typedef union sval_u
{
	int				type;
	sval_t			val;
	unsigned int	sourcePosValue;
	union sval_u	*node;
} sval_u;

/* called directly: retail inlines it into every node function (a wrapper blocks that) */
extern void *Hunk_AllocateTempMemoryHighInternal( int size );

/* ---- node1_  0x0046F1B0 ---- */
sval_u node1_( int type )
{
	sval_u result;

	result.type = type;
	return result;
}

/* ---- node_pos  0x0046F1C0 ---- */
sval_u node_pos( unsigned int sourcePos )
{
	sval_u result;

	result.sourcePosValue = sourcePos;
	return result;
}

/* ---- node0  0x0046F1D0 ---- */
sval_u node0( int type )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 4 );

	node[0].type = type;
	result.node = node;
	return result;
}

/* ---- node1  0x0046F230 ---- */
sval_u node1( int type, sval_u a )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 8 );

	node[0].type = type;
	node[1] = a;
	result.node = node;
	return result;
}

/* ---- node2  0x0046F290 ---- */
sval_u node2( int type, sval_u a, sval_u b )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 12 );

	node[0].type = type;
	node[1] = a;
	node[2] = b;
	result.node = node;
	return result;
}

/* ---- node2_  0x0046F300 ---- */
sval_u node2_( sval_u a, sval_u b )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 8 );

	node[0] = a;
	node[1] = b;
	result.node = node;
	return result;
}

/* ---- node3  0x0046F360 ---- */
sval_u node3( int type, sval_u a, sval_u b, sval_u c )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 16 );

	node[0].type = type;
	node[1] = a;
	node[2] = b;
	node[3] = c;
	result.node = node;
	return result;
}

/* ---- node3_  0x0046F3D0 ---- */
sval_u node3_( sval_u a, sval_u b, sval_u c )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 12 );

	node[0] = a;
	node[1] = b;
	node[2] = c;
	result.node = node;
	return result;
}

/* ---- node4  0x0046F440 ---- */
sval_u node4( int type, sval_u a, sval_u b, sval_u c, sval_u d )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 20 );

	node[0].type = type;
	node[1] = a;
	node[2] = b;
	node[3] = c;
	node[4] = d;
	result.node = node;
	return result;
}

/* ---- node4_  0x0046F4B0 ---- */
sval_u node4_( sval_u a, sval_u b, sval_u c, sval_u d )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 16 );

	node[0] = a;
	node[1] = b;
	node[2] = c;
	node[3] = d;
	result.node = node;
	return result;
}

/* ---- node5  0x0046F520 ---- */
sval_u node5( int type, sval_u a, sval_u b, sval_u c, sval_u d, sval_u e )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 24 );

	node[0].type = type;
	node[1] = a;
	node[2] = b;
	node[3] = c;
	node[4] = d;
	node[5] = e;
	result.node = node;
	return result;
}

/* ---- node6  0x0046F5A0 ---- */
sval_u node6( int type, sval_u a, sval_u b, sval_u c, sval_u d, sval_u e, sval_u f )
{
	sval_u  result;
	sval_u *node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 28 );

	node[0].type = type;
	node[1] = a;
	node[2] = b;
	node[3] = c;
	node[4] = d;
	node[5] = e;
	node[6] = f;
	result.node = node;
	return result;
}

/* ---- linked_list_end  0x0046F620 ---- */
sval_u linked_list_end( sval_u a )
{
	sval_u *cell;
	sval_u *list;
	sval_u  result;

	cell = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 8 );
	cell[0] = a;
	cell[1].node = NULL;

	list = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 8 );
	list[0].node = cell;
	list[1].node = cell;
	result.node = list;
	return result;
}

/* ---- prepend_node  0x0046F6D0 ---- */
sval_u prepend_node( sval_u val1, sval_u val2 )
{
	sval_u *node;

	node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 8 );
	node[0] = val1;
	node[1] = val2.node[0];
	val2.node[0].node = node;
	return val2;
}

/* ---- append_node  0x0046F730 ---- */
sval_u append_node( sval_u val1, sval_u val2 )
{
	sval_u *node;

	node = (sval_u *)Hunk_AllocateTempMemoryHighInternal( 8 );
	node[0] = val2;
	node[1].node = NULL;
	val1.node[1].node[1].node = node;
	val1.node[1].node = node;
	return val1;
}

/* ---- append_list  0x0046F7A0 ---- VERIFIED */
sval_u append_list( sval_u val1, sval_u val2 )
{
	val1.node[1].node[1] = val2.node[0];
	val1.node[1] = val2.node[1];
	return val1;
}

#ifdef __cplusplus
}
#endif
