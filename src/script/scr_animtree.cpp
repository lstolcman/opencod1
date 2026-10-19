/*
 * script/scr_animtree.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_animtree.cpp
 *
 * Retail range 0x00467180-0x00468817, 20 functions.
 *
 * @fidelity: verified
 * @fidelity-default: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"

#include <ctype.h>
#include <string.h>

/* retail's 16-bit object handles (never address-taken) */
unsigned short  scrAnimPub_treeRoot;        /* 0x008E60C0 */
unsigned short  scrAnimPub_usingTreeNames;  /* 0x008E60B0 */

#define SCR_ANIMTREE_POOL_COUNT     2       /* 0x00A9CC58 selects one */
#define SCR_ANIMTREE_SLOTS          128

int             scrAnimPub_treeCount[SCR_ANIMTREE_POOL_COUNT];
unsigned short  scrAnimPub_treeNames[SCR_ANIMTREE_POOL_COUNT * SCR_ANIMTREE_SLOTS];
int             scrAnimPub_trees[SCR_ANIMTREE_POOL_COUNT * SCR_ANIMTREE_SLOTS];

#define scrAnimPub_treeCount    scrAnimPub_treeCount
#define scrAnimPub_treeNames    scrAnimPub_treeNames
#define scrAnimPub_trees        scrAnimPub_trees
#define scrAnimPub_pool         xanim_activePoolSlot

#define SCR_ANIMTREE_AT( slot ) \
	scrAnimPub_trees[ SCR_ANIMTREE_SLOTS * scrAnimPub_pool + (slot) ]
#define SCR_ANIMTREE_NAME_AT( slot ) \
	scrAnimPub_treeNames[ SCR_ANIMTREE_SLOTS * scrAnimPub_pool + (slot) ]

extern int AllocVariable__Fv();
extern int FindVariableIndexInternal__FUsUi();
extern int FreeValue__FP21VariableValueInternal();
extern int GetObject__FUs();
extern int GetVariableIndexInternal__FUsUi();
extern void *Hunk_AllocAlignInternal( int size, int align );
extern int MakeVariableExternal__FP8VariableP21VariableValueInternal();
extern int RemoveRefToObject__FUs();
extern int RemoveVariable__FUsUi();
extern int SL_FreeString__FUsPCcUi();
extern int Scr_IsIdentifier__FPCc();
extern int SetVariableValue__FUsP13VariableValue();
extern void XAnimPrecacheAnimTree_m( const char *animName,
                                     void *( __cdecl *allocFn )( unsigned int ) );
#define XAnimLoadFile XAnimPrecacheAnimTree_m

extern void __cdecl XAnimCheckSyncNodes_r( void *tree, int nodeIndex );

extern void QDECL CompileError( unsigned int sourcePos, const char *format, ... );
extern void QDECL CompileError2( const char *codePos, const char *format, ... );
#ifdef COD1_VC7
extern unsigned char Scr_IsIdentifier( const char *name );
#else
extern qboolean   Scr_IsIdentifier( const char *name );
#endif
extern qboolean   Scr_IsInOpcodeMemory( const char *pos );
extern qboolean   Scr_IsInOpcodeBuffer_m( const char *pos );
extern char      *Com_GetLastTokenPos( void );
extern char      *Com_ParseOnLine( char **data_p );
extern char      *Scr_AddSourceBuffer( const char *filename, void *codeStart, void *developerCodeStart );
extern int        Hunk_ClearTempMemoryHigh( void );

extern void      *FS_GetDataForFile( const char *dir, const char *name, const char *ext );

typedef struct scrXAnimEntry_s {
	unsigned short  childCount;      /* +0x00  0 == leaf */
	unsigned short  parentIndex;     /* +0x02 */
	union {
		void       *leafAsset;       /* +0x04  when childCount == 0 */
		struct {
			unsigned short  flags;           /* +0x04 */
			unsigned short  firstChildIndex; /* +0x06 */
		} parent;
	} u;
} scrXAnimEntry_t;

/* the dword an emitted animation reference is patched with */
typedef struct scrAnimRef_s {
	unsigned short  index;           /* +0x00  node index */
	unsigned short  tree;            /* +0x02  tree slot */
} scrAnimRef_t;

typedef struct scrXAnim_s {
	const char      *name;           /* +0x00 */
	int              nodeCount;      /* +0x04 */
	scrXAnimEntry_t  entries[1];     /* +0x08  [nodeCount] */
} scrXAnim_t;

extern void __cdecl XAnimSetLeafNode( const char *animName, scrXAnim_t *tree,
									  unsigned short index );
/* xanim/dobj.cpp */
unsigned short *__cdecl XAnimSetParentNode( unsigned short childCount, unsigned short flags,
									  scrXAnim_t *tree, unsigned short index, int unused,
									  unsigned short firstChildIndex );

#define SCR_HANDLE( g )     ( *(unsigned short *)&(g) )

/* retail 0x008E5EDC: the tree being loaded, a real 16-bit handle (Scr_EndLoadAnimTrees
   loads it with xor eax,eax / mov ax) */
unsigned short scrAnimPub_currentTreeRoot;
#define scrAnimPub_currentUsingTree scrAnimPub_usingTreeNames  /* 0x008E60B0 */
/* the compiler's running checksum; a plain int (an address-taken view stops VC7 from
   scheduling other loads past pointer stores) */
#define scrAnimPub_checksum         scrCompilePub_checksum
#define scrCompilePub_sourceBuf     scrCompilePub_sourceBuf               /* 0x01407370 */
#define scrCompilePub_scriptfilename scrCompilePub_scriptfilename              /* 0x01407374 */

static const char  *scrAnimPub_parseStart;
static char        *scrAnimPub_parseState;

#define SCR_ANIM_PROP_LOOPSYNC      0x01
#define SCR_ANIM_PROP_NONLOOPSYNC   0x02
#define SCR_ANIM_PROP_COMPLETE      0x08

static const char * const scrAnimPub_propertyNames[3] = {
	"loopsync", "nonloopsync", "complete"
};

#define SCR_ANIMTREE_CHILD_ANIMS    0
#define SCR_ANIMTREE_CHILD_TREE     1

#define SCR_ANIMTREE_NAME_TYPE      4

#include "scr_local.h"
/* ---- module-owned globals ---- */
unsigned char firstFreeVariable[786424];


/* ---- SetAnimCheck  0x00467180 ---- VERIFIED */
/* Com_Frame's (com_animCheck latch), LTCG-inlined there (0x0043800C). */
void SetAnimCheck(int result)
{
  com_animCheckCached = (qboolean)result;
}

/* ---- AnimTreeCompileError  0x00467190 ---- VERIFIED */
void AnimTreeCompileError( const char *message )
{
	const char *errorPos = Com_GetLastTokenPos();

	Com_EndParseSession();
	CompileError( (unsigned int)( errorPos - scrAnimPub_parseStart ), "%s", message );
}

int AnimTreeCompileError__FPc( const char *message ) {
	AnimTreeCompileError( message );
	return 0;
}

/* ---- GetAnimTreeParseProperties  0x004671F0 ---- VERIFIED */
unsigned int GetAnimTreeParseProperties( void )
{
	unsigned int flags = 0;

	for ( ;; ) {
		const char     *token = Com_ParseOnLine( &scrAnimPub_parseState );
		unsigned int    i;

		if ( !token[0] ) {
			return flags;
		}

		for ( i = 0; i < 3; i++ ) {
			/* retail 0x00467260: CRT _stricmp, not Q_stricmp */
			if ( !_stricmp( token, scrAnimPub_propertyNames[i] ) ) {
				break;
			}
		}

		switch ( i ) {
		case 0:  flags |= SCR_ANIM_PROP_LOOPSYNC;    break;
		case 1:  flags |= SCR_ANIM_PROP_NONLOOPSYNC; break;
		case 2:  flags |= SCR_ANIM_PROP_COMPLETE;    break;
		default: AnimTreeCompileError( "unknown anim property" ); break;
		}
	}
}

int GetAnimTreeParseProperties__Fv( void ) { return (int)GetAnimTreeParseProperties(); }

/* ---- Scr_EmitAnimationInternal  0x00467300 ---- VERIFIED */
void Scr_EmitAnimationInternal( char *animRef, unsigned short animName,
										   unsigned short tree, unsigned int sourcePos )
{
	unsigned short  handle;
	VariableValue  *stored;
	VariableValue   value;

	if ( developer_script_int ) {
		CompileError( sourcePos, "cannot reference animation from /# ... #/ comment" );
		return;
	}

	handle = FindVariable( tree, animName );
	if ( !handle ) {
		handle = GetVariable( tree, animName );
		*(unsigned int *)animRef = 0;
		value.type = VAR_CODEPOS;
		value.u.pointerValue = animRef;
		SetVariableValue( handle, &value );
		return;
	}
	stored = GetVariableValueAddress( handle );
	*(unsigned int *)animRef = (unsigned int)stored->u.pointerValue;
	stored->u.pointerValue = animRef;
}

/* ---- Scr_EmitAnimation  0x004673A0 ---- VERIFIED */
void Scr_EmitAnimation( char *animRef, unsigned short animName, unsigned int sourcePos )
{
	if ( !scrAnimPub_currentUsingTree ) {
		CompileError( sourcePos, "#using_animtree was not specified" );
		return;
	}
	Scr_EmitAnimationInternal( animRef, animName, scrAnimPub_currentUsingTree, sourcePos );
}

int Scr_EmitAnimation__FPcUsUi( char *animRef, unsigned short animName,
								unsigned int sourcePos ) {
	Scr_EmitAnimation( animRef, animName, sourcePos );
	return 0;
}

/* Retail hands each token byte to the VC7.1 _isalnum (0x00525CCD) through
 * movsx (0x00467443, 0x004675F4), so a byte >= 0x80 becomes a negative index
 * into the "C" locale _pctype at 0x0053DF42 (_setlocale has no callers) and
 * reads the CRT runtime-error strings in .rdata just below the table.  Bit
 * (byte - 0x80) is set where that word & 0x107 is nonzero: 104 of the 128
 * high bytes pass as name characters in retail. */
static const unsigned int animTreeHighByteAlnum[4] = {
	0xFEF6BFFF, 0x6BFFBDBF, 0xDDFF9FFF, 0x0FDE77F9
};

static int AnimTree_IsAlnum( char c )
{
	return isalnum( (int)c );
}

/* AnimTree_TokenIsName  0x004674AC */
static qboolean AnimTree_TokenIsName( const char *token )
{
	const char *p = token;

	if ( !*p ) {
		return qtrue;
	}
	while ( AnimTree_IsAlnum( *p ) || *p == '_' ) {
		if ( !*++p ) {
			return qtrue;
		}
	}
	return qfalse;
}

/* ---- AnimTreeParseInternal  0x004673D0 ---- VERIFIED */
unsigned char AnimTreeParseInternal( unsigned short parentHandle,
									 unsigned short treeHandle,
									 unsigned char addEmptyVoid,
									 unsigned char addLoopingVoid,
									 unsigned char forceLoadedChildren )
{
	unsigned short  animName;
	unsigned short  animNode;
	unsigned int    properties;
	unsigned char   missingFromTree;
	unsigned char   hitEndOfFile;
	const char     *token;
	VariableValue   value;

	/* retail 0x004673D9 stores value.type before zeroing the others */
	value.type = VAR_INTEGER;
	animName = 0;
	animNode = 0;
	properties = 0;
	missingFromTree = qfalse;

	for ( ;; ) {
		/* retail 0x004673F2: Com_Parse inlined (unget restore, then Com_ParseExt qtrue) */
		token = Com_Parse( &scrAnimPub_parseState );
		if ( scrAnimPub_parseState == NULL ) {
			hitEndOfFile = qtrue;
			break;
		}

		if ( AnimTree_TokenIsName( token ) ) {
			if ( missingFromTree ) {
				RemoveVariable( parentHandle, animName );
			}

			animName = SL_GetLowercaseStringOfLen( token, SCR_STRING_USER_CANONICAL,
												   strlen( token ) + 1,
												   SCR_ANIMTREE_NAME_TYPE );
			if ( FindVariable( parentHandle, animName ) ) {
				AnimTreeCompileError( "duplicate animation" );
			}
			animNode = GetVariable( parentHandle, animName );

			missingFromTree = !forceLoadedChildren
						   && !FindVariable( treeHandle, animName )
						   && !com_animCheckCached;

			properties = 0;
			token = Com_ParseOnLine( &scrAnimPub_parseState );
			if ( !token[0] ) {
				continue;
			}
			if ( AnimTree_TokenIsName( token ) ) {
				AnimTreeCompileError( "FIXME: aliases not yet implemented" );
			}
			if ( token[0] != ':' || token[1] ) {
				AnimTreeCompileError( "bad token" );
			}

			properties = GetAnimTreeParseProperties();
			token = Com_Parse( &scrAnimPub_parseState );	/* retail 0x00467686 */
			if ( token[0] != '{' || token[1] ) {
				AnimTreeCompileError(
					"properties cannot be applied to primitive animations" );
			}
		}

		if ( token[0] == '{' ) {
			unsigned short  childArray;

			if ( token[1] ) {
				AnimTreeCompileError( "bad token" );
			}
			token = Com_ParseOnLine( &scrAnimPub_parseState );
			if ( token[0] ) {
				/* retail's literal at 0x0055BB60. */
				AnimTreeCompileError( "token not allowed after '{'" );
			}
			if ( !animNode ) {
				AnimTreeCompileError( "no animation specified for this block" );
			}

			childArray = GetArray( animNode );


			if ( AnimTreeParseInternal( childArray, treeHandle,
										missingFromTree ? qfalse : qtrue,
										( properties & SCR_ANIM_PROP_LOOPSYNC ) ? qtrue : qfalse,
										forceLoadedChildren || ( ( properties & 0x08 ) && !missingFromTree ) ) ) {
				AnimTreeCompileError( "unexpected end of file" );
			}

			if ( GetArraySize( childArray ) ) {
				value.u.intValue = (int)properties;
				SetVariableValue( GetArrayVariable( childArray, 0 ), &value );
			} else {
				RemoveVariable( parentHandle, animName );
			}

			animNode        = 0;
			missingFromTree = qfalse;
			continue;
		}

		if ( token[0] == '}' ) {
			if ( token[1] ) {
				AnimTreeCompileError( "bad token" );
			}
			token = Com_ParseOnLine( &scrAnimPub_parseState );
			if ( token[0] ) {
				/* retail's literal at 0x0055BB04. */
				AnimTreeCompileError( "token not allowed after '}'" );
			}
			hitEndOfFile = qfalse;
			break;
		}

		AnimTreeCompileError( "bad token" );
	}

	if ( missingFromTree ) {
		RemoveVariable( parentHandle, animName );
	}

	if ( addEmptyVoid && GetArraySize( parentHandle ) == 0 ) {
		const char     *voidName = addLoopingVoid ? "void_loop" : "void";
		unsigned short  handle;

		handle = SL_GetStringOfLen( voidName, 0, strlen( voidName ) + 1,
									SCR_ANIMTREE_NAME_TYPE );
		GetVariable( parentHandle, handle );
		SL_RemoveRefToString( handle );
	}

	return hitEndOfFile;
}

int AnimTreeParseInternal__FUsUsbbb( unsigned short parentHandle, unsigned short treeHandle,
									 int addEmptyVoid, int addLoopingVoid,
									 int forceLoadedChildren ) {
	return AnimTreeParseInternal( parentHandle, treeHandle,
								  addEmptyVoid ? qtrue : qfalse,
								  addLoopingVoid ? qtrue : qfalse,
								  forceLoadedChildren ? qtrue : qfalse );
}

/* ---- Scr_AnimTreeParse  0x00467B40 ---- VERIFIED */
void Scr_AnimTreeParse( const char *source, unsigned short parentHandle,
						unsigned short treeHandle )
{
	Com_BeginParseSession( "Scr_AnimTreeParse" );
	scrAnimPub_parseState = (char *)source;
	scrAnimPub_parseStart = source;

	if ( !AnimTreeParseInternal( parentHandle, treeHandle, qtrue, qfalse, qfalse ) ) {
		AnimTreeCompileError( "bad token" );
	}
	Com_EndParseSession();
}

int Scr_AnimTreeParse__FPCcUsUs( const char *source, unsigned short parentHandle,
								 unsigned short treeHandle ) {
	Scr_AnimTreeParse( source, parentHandle, treeHandle );
	return 0;
}

/* ---- Hunk_AllocXAnimPrecache  0x00467BC0 ---- VERIFIED */
void * __cdecl Hunk_AllocXAnimPrecache( unsigned int size )
{
  return (void *)Hunk_AllocAlignInternal( size, 4 );
}

/* ---- Scr_GetAnimTreeSize  0x00467BD0 ---- VERIFIED */
int Scr_GetAnimTreeSize( unsigned short handle )
{
	unsigned short  child;
	int             nodeCount = 0;

	for ( child = FindNextSibling( handle ); child != 0; child = FindNextSibling( child ) ) {
		if ( GetVariableName( child ) >= VAR_NAME_OBJECT_BASE ) {
			continue;
		}
		if ( GetVarType( child ) == VAR_OBJECT ) {
			nodeCount += Scr_GetAnimTreeSize( FindObject( child ) );
		} else {
			nodeCount++;
		}
	}

	if ( nodeCount ) {
		nodeCount++;
	}
	return nodeCount;
}

int Scr_GetAnimTreeSize__FUs( unsigned short handle ) { return Scr_GetAnimTreeSize( handle ); }

/* ---- ConnectScriptToAnim  0x00467C80 ---- VERIFIED */
void ConnectScriptToAnim( unsigned short unresolvedRoot, int nodeIndex,
						  unsigned short treeName, unsigned short animName,
						  int treeIndex )
{
	unsigned short  handle;
	VariableValue  *stored;
	scrAnimRef_t    anim;
	char           *ref;
	char           *next;

	handle = FindVariable( unresolvedRoot, animName );
	if ( !handle ) {
		return;
	}

	stored = GetVariableValueAddress( handle );
	if ( stored->u.pointerValue == NULL ) {
		Com_Error( ERR_DROP, "\x15" "duplicate animation '%s' in 'animtrees/%s.atr'",
				   SL_ConvertToString( animName ), SL_ConvertToString( treeName ) );
	}

	anim.index = (unsigned short)nodeIndex;
	anim.tree  = (unsigned short)treeIndex;
	for ( ref = (char *)stored->u.pointerValue; ref != NULL; ref = next ) {
		next = *(char **)ref;
		*(scrAnimRef_t *)ref = anim;
	}
	stored->u.pointerValue = NULL;
}

int ConnectScriptToAnim__FUsiUsUsi( unsigned short unresolvedRoot, int nodeIndex,
									unsigned short treeName, unsigned short animName,
									int treeIndex ) {
	ConnectScriptToAnim( unresolvedRoot, nodeIndex, treeName, animName, treeIndex );
	return 0;
}

/* ---- Scr_GetAnimsIndex  0x00467D30 ---- VERIFIED */
int __cdecl Scr_GetAnimsIndex( void *anims )
{
	int  slot;
	int *trees = &scrAnimPub_trees[ SCR_ANIMTREE_SLOTS * scrAnimPub_pool ];

	/* retail 0x00467D50: no range check on slot */
	for ( slot = scrAnimPub_treeCount[ scrAnimPub_pool ]; slot != 0; slot-- ) {
		if ( trees[slot] == (int)anims ) {
			break;
		}
	}
	return slot;
}

/* Scr_GetAnimTreeCount  HIGH */
int __cdecl Scr_GetAnimTreeCount( void )
{
	return scrAnimPub_treeCount[ scrAnimPub_pool ];
}

/* ---- Scr_GetAnims  0x00467D60 ---- VERIFIED */
int __cdecl Scr_GetAnims( int slot )
{
	/* retail 0x00467D6E: no range check on slot */
	return SCR_ANIMTREE_AT( slot );
}

/* ---- Scr_CreateAnimationTree  0x00467D80 ---- VERIFIED */
unsigned int Scr_CreateAnimationTree( unsigned short sourceHandle,
									  unsigned short unresolvedRoot,
									  scrXAnim_t *tree,
									  unsigned int firstChildIndex,
									  const char *nodeName,
									  unsigned int nodeIndex,
									  unsigned short treeName,
									  int treeIndex )
{
	unsigned short   child;
	unsigned int     childCount = 0;
	unsigned short   properties;
	unsigned short   propertyHandle;

	for ( child = FindNextSibling( sourceHandle ); child != 0; child = FindNextSibling( child ) ) {
		if ( GetVariableName( child ) < VAR_NAME_OBJECT_BASE ) {
			childCount++;
		}
	}

	propertyHandle = FindArrayVariable( sourceHandle, 0 );
	if ( propertyHandle ) {
		properties = (unsigned short)GetVariableValueAddress( propertyHandle )->u.intValue;
	} else {
		properties = 0;
	}

	scrAnimPub_checksum = properties + 31u * ( childCount + 31u *
						  ( firstChildIndex + 31u * ( nodeIndex + 31u * scrAnimPub_checksum ) ) );

	/* retail inlines XAnimSetParentNode (0x00483FC0) here; nodeIndex then becomes the
	   running child index and firstChildIndex the next free node (retail keeps them in
	   nodeIndex's home and ebp) */
	XAnimSetParentNode( (unsigned short)childCount, properties, tree, (unsigned short)nodeIndex,
						0, (unsigned short)firstChildIndex );

	nodeIndex = firstChildIndex;
	firstChildIndex += childCount;

	for ( child = FindNextSibling( sourceHandle ); child != 0; child = FindNextSibling( child ) ) {
		unsigned int    name = GetVariableName( child );
		unsigned short  animName;

		if ( name >= VAR_NAME_OBJECT_BASE ) {
			continue;
		}
		animName = (unsigned short)name;

		ConnectScriptToAnim( unresolvedRoot, (int)nodeIndex, treeName, animName,
							 treeIndex );

		if ( GetVarType( child ) == VAR_OBJECT ) {
			firstChildIndex = Scr_CreateAnimationTree(
									FindObject( child ), unresolvedRoot, tree,
									firstChildIndex, SL_ConvertToString( animName ),
									nodeIndex, treeName, treeIndex );
		} else {
			scrAnimPub_checksum = nodeIndex + 31u * scrAnimPub_checksum;
			XAnimSetLeafNode( SL_ConvertToString( animName ), tree,
							  (unsigned short)nodeIndex );
		}
		nodeIndex++;
	}

	return firstChildIndex;
}

int Scr_CreateAnimationTree__FUsUsP7XAnim_sUiPCcUiUsi(
		unsigned short sourceHandle, unsigned short unresolvedRoot, void *tree,
		unsigned int firstChildIndex, const char *nodeName, unsigned int nodeIndex,
		unsigned short treeName, int treeIndex ) {
	return (int)Scr_CreateAnimationTree( sourceHandle, unresolvedRoot,
										 (scrXAnim_t *)tree, firstChildIndex,
										 nodeName, nodeIndex, treeName, treeIndex );
}

/* ---- Scr_CheckAnimsDefined  0x00468000 ---- VERIFIED */
void Scr_CheckAnimsDefined( unsigned short unresolvedRoot, unsigned short treeName )
{
	unsigned short child;

	for ( child = FindNextSibling( unresolvedRoot ); child != 0;
		  child = FindNextSibling( child ) ) {
		unsigned short  animName;
		const char     *codePos;
		char           *message;

		animName = (unsigned short)GetVariableName( child );
		if ( GetVariableValueAddress( child )->u.pointerValue == NULL ) {
			continue;
		}

		message  = va( "animation '%s' not defined in anim tree '%s'",
					   SL_ConvertToString( animName ),
					   SL_ConvertToString( treeName ) );

		codePos = GetVariableValueAddress( child )->u.codePosValue;
		if ( Scr_IsInOpcodeMemory( codePos ) || Scr_IsInOpcodeBuffer_m( codePos ) ) {
			CompileError2( codePos, "%s", message );
		} else {
			Com_Error( ERR_DROP, "\x15%s", message );
		}
	}
}

int Scr_CheckAnimsDefined__FUsUs( unsigned short unresolvedRoot, unsigned short treeName ) {
	Scr_CheckAnimsDefined( unresolvedRoot, treeName );
	return 0;
}

/* ---- Scr_PrecacheAnimationTree  0x00468110 ---- VERIFIED */
void __cdecl Scr_PrecacheAnimationTree( unsigned short animsId )
{
	unsigned short  id;
	unsigned int    name;

	for ( id = FindNextSibling( animsId ); id != 0; id = FindNextSibling( id ) ) {
		name = scrVarNodes[id].status >> VAR_STATUS_NAME_SHIFT;
		if ( name >= VAR_NAME_OBJECT_BASE ) {
			continue;
		}

		if ( ( scrVarNodes[id].status & VAR_STATUS_TYPE_MASK ) == VAR_OBJECT ) {
			Scr_PrecacheAnimationTree( scrVarNodes[id].u.halfword.lo );
		} else {
			XAnimLoadFile( SL_ConvertToString( (unsigned short)name ),
						   Hunk_AllocXAnimPrecache );
		}
	}
}

int Scr_PrecacheAnimationTree__FUs( unsigned short animsId ) {
	Scr_PrecacheAnimationTree( animsId );
	return 0;
}

/* ---- Scr_UsingTreeInternal  0x004681E0 ---- VERIFIED */
unsigned short __cdecl Scr_UsingTreeInternal( const char *name, int *slot )
{
	unsigned short  canonical;
	unsigned short  entryNode;
	unsigned short  object;
	unsigned short *names;
	int             count;
	int             i;

	canonical = Scr_CreateCanonicalFilename( name );

	entryNode = FindVariable( scrAnimPub_treeRoot, canonical );
	if ( !entryNode ) {
		entryNode = GetVariable( scrAnimPub_treeRoot, canonical );
		object    = GetObject( entryNode );

		/* retail 0x00468232-0x00468254: no bound; pool 1 past slot 127 relies on
		   retail's contiguous treeCount/treeNames/trees layout (0x8E58D0/0x8E58D8/0x8E5AD8) */
		count = scrAnimPub_treeCount[ scrAnimPub_pool ] + 1;
		scrAnimPub_treeCount[ scrAnimPub_pool ] = count;
		SCR_ANIMTREE_NAME_AT( count ) = entryNode;
		*slot = count;
	} else {
		object = FindObject( entryNode );
		*slot  = 0;
		names  = &scrAnimPub_treeNames[ SCR_ANIMTREE_SLOTS * scrAnimPub_pool ];
		for ( i = 1; i <= scrAnimPub_treeCount[ scrAnimPub_pool ]; i++ ) {
			if ( names[i] == entryNode ) {
				*slot = i;
				break;
			}
		}
	}

	object = GetArray( GetVariable( object, 0 ) );
	SL_RemoveRefToString( canonical );
	return object;
}

/* ---- Scr_UsingTree__FPCcUi  0x00468350 ---- VERIFIED */
void Scr_UsingTree( const char *name, unsigned int sourcePos )
{
	if ( !Scr_IsIdentifier( name ) ) {
		CompileError( sourcePos, "bad anim tree name" );
		return;
	}

	scrAnimPub_currentUsingTree = Scr_UsingTreeInternal( name, &scrCompileGlob_animTreeValue );
}

/* ---- Scr_LoadAnimTreeInternal  0x00468390 ---- VERIFIED */
unsigned char Scr_LoadAnimTreeInternal( const char *animTreeName,
										unsigned short parentHandle,
										unsigned short treeHandle )
{
	char            filename[64];
	const char     *savedSourcePos;
	const char     *savedFilename;
	char           *source;

	/* retail 0x004683B0: unbounded CRT sprintf (0x00525208) into char[64] */
	sprintf( filename, "animtrees/%s.atr", animTreeName );

	savedSourcePos = (const char *)scrCompilePub_sourceBuf;

	source = (char *)Scr_AddSourceBuffer( filename, 0, 0 );
	if ( !source ) {
		return qfalse;
	}

	savedFilename = (const char *)scrCompilePub_scriptfilename;
	scrCompilePub_scriptfilename = (int)filename;

	Scr_AnimTreeParse( source, parentHandle, treeHandle );

	/* Mac 1.3 order: restore the compile state, then release the source buffer */
	scrCompilePub_scriptfilename = (int)savedFilename;
	scrCompilePub_sourceBuf      = (int)savedSourcePos;
	Hunk_ClearTempMemoryHigh();
	return GetArraySize( parentHandle ) != 0 ? qtrue : qfalse;
}

int Scr_LoadAnimTreeInternal__FPCcUsUs( const char *animTreeName,
										unsigned short parentHandle,
										unsigned short treeHandle ) {
	return Scr_LoadAnimTreeInternal( animTreeName, parentHandle, treeHandle );
}

/* ---- Scr_LoadAnimTreeAtIndex  0x00468450 ---- VERIFIED */
void Scr_LoadAnimTreeAtIndex( int treeIndex, void *(__cdecl *allocFn)( int ) )
{
	unsigned short   treeRecord;
	unsigned short   treeName;
	unsigned short   treeData;
	unsigned short   sourceHandle;
	unsigned short   unresolvedRoot;
	unsigned short   rootName;
	int              nodeCount;
	void            *asset;
	const char      *name;
	scrXAnim_t      *tree;
	VariableValue    value;

	/* retail 0x00468453-0x00468466 indexes the pool with no range check */
	treeRecord = SCR_ANIMTREE_NAME_AT( treeIndex );
	treeName   = (unsigned short)GetVariableName( treeRecord );
	treeData   = FindObject( treeRecord );

	if ( FindVariable( treeData, SCR_ANIMTREE_CHILD_TREE ) ) {
		return;
	}

	sourceHandle = FindVariable( treeData, SCR_ANIMTREE_CHILD_ANIMS );
	if ( !sourceHandle ) {
		SCR_ANIMTREE_AT( treeIndex ) = 0;
		return;
	}

	unresolvedRoot = FindObject( sourceHandle );

	scrAnimPub_currentTreeRoot = Scr_AllocArray();

	if ( !Scr_LoadAnimTreeInternal( SL_ConvertToString( treeName ),
									scrAnimPub_currentTreeRoot, unresolvedRoot ) ) {
		RemoveVariable( scrAnimPub_treeRoot, treeName );
		RemoveRefToObject( scrAnimPub_currentTreeRoot );
		scrAnimPub_currentTreeRoot = 0;
		SCR_ANIMTREE_AT( treeIndex ) = 0;
		return;
	}

	nodeCount = Scr_GetAnimTreeSize( scrAnimPub_currentTreeRoot );

	asset = FS_GetDataForFile( "animtrees", SL_ConvertToString( treeName ), ".atr" );
	name  = asset ? *(const char **)asset : "(savegame)";

	/* --- XAnimAllocTree (0x00484000), inlined as retail does --- */
	tree = (scrXAnim_t *)allocFn( 8 * nodeCount + 8 );
	tree->name      = name;
	tree->nodeCount = nodeCount;

	rootName = SL_GetString_( "root", 0, SCR_ANIMTREE_NAME_TYPE );
	ConnectScriptToAnim( unresolvedRoot, SCR_ANIMTREE_CHILD_ANIMS, treeName, rootName,
						 treeIndex );
	SL_RemoveRefToString( rootName );

	Scr_CreateAnimationTree( scrAnimPub_currentTreeRoot, unresolvedRoot, tree,
							 SCR_ANIMTREE_CHILD_TREE, "root", SCR_ANIMTREE_CHILD_ANIMS,
							 treeName, treeIndex );
	Scr_CheckAnimsDefined( unresolvedRoot, treeName );
	Scr_PrecacheAnimationTree( scrAnimPub_currentTreeRoot );

	RemoveVariable( treeData, SCR_ANIMTREE_CHILD_ANIMS );
	RemoveRefToObject( scrAnimPub_currentTreeRoot );
	scrAnimPub_currentTreeRoot = 0;

	value.type           = VAR_CODEPOS;
	value.u.pointerValue = tree;
	SetVariableValue( GetVariable( treeData, SCR_ANIMTREE_CHILD_TREE ), &value );

	XAnimCheckSyncNodes_r( tree, 0 );

	SCR_ANIMTREE_AT( treeIndex ) = (int)tree;
}

int Scr_LoadAnimTreeAtIndex__FiPFi_Pv( int treeIndex, void *(__cdecl *allocFn)( int ) ) {
	Scr_LoadAnimTreeAtIndex( treeIndex, allocFn );
	return 0;
}

/* ---- Scr_FindAnimTree  0x004686E0 ---- VERIFIED */
/* returns scr_animtree_t by value (Mac 1.3 Scr_FindAnimTree writes it through the
   hidden result pointer); a 4-byte struct comes back in eax like the import's void * */
typedef struct scr_animtree_s {
	void           *anims;
} scr_animtree_t;

scr_animtree_t __cdecl Scr_FindAnimTree( const char *name )
{
	unsigned short  canonical;
	unsigned short  entryNode;
	unsigned short  treeNode;
	scr_animtree_t  tree;

	canonical = Scr_CreateCanonicalFilename( name );
	entryNode = scrVarIndirections[
		FindVariableIndexInternal( (unsigned short)scrAnimPub_treeRoot, canonical ) ].id;
	SL_RemoveRefToString( canonical );

	tree.anims = NULL;
	if ( !entryNode ) {
		return tree;
	}
	treeNode = scrVarIndirections[
		FindVariableIndexInternal( scrVarNodes[entryNode].u.halfword.lo, 1 ) ].id;
	if ( !treeNode ) {
		return tree;
	}
	tree.anims = scrVarNodes[treeNode].u.pointerValue;
	return tree;
}

/* ---- Scr_FindAnim  0x00468790 ---- VERIFIED */
void Scr_FindAnim( const char *treeName, const char *animName, void *animRef )
{
	unsigned short  animHandle;
	int             treeIndex;

	animHandle = SL_GetLowercaseStringOfLen( animName, 0, strlen( animName ) + 1,
											 SCR_ANIMTREE_NAME_TYPE );
	Scr_EmitAnimationInternal( (char *)animRef, animHandle,
							   Scr_UsingTreeInternal( treeName, &treeIndex ), 0 );
	SL_RemoveRefToString( animHandle );
}

#ifdef __cplusplus
}
#endif
