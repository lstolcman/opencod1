/*
 * script/scr_compiler.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_compiler.cpp
 *
 * Retail range 0x00468820-0x0046CD20, 127 functions.
 *
 * @fidelity: verified
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "scr_local.h"


#include <malloc.h>

typedef unsigned int sval_t;

/* retail passes parse-tree cells by value as this union (symbols: 6sval_u),
 * which LTCG never moves into registers */
typedef union sval_u {
	sval_t                  value;
	int                     type;
	unsigned int            stringValue;
	int                     intValue;
	unsigned int            sourcePosValue;
} sval_u;

#define SVAL_U( v )         ( *(sval_u *)&(v) )

typedef struct sval_cell_s {
	sval_t                  value;
	struct sval_cell_s     *next;
} sval_cell_t;

typedef struct sval_list_s {
	sval_cell_t            *first;
	sval_cell_t            *last;
} sval_list_t;

typedef struct sval_entry_s {
	sval_t                  node;
	unsigned int            sourcePos;
} sval_entry_t;

typedef struct sval_stmtlist_s {
	sval_t                  kind;
	sval_cell_t            *head;
} sval_stmtlist_t;

#define SVAL_NODE( v )      ( (sval_t *)(v) )
#define SVAL_LIST( v )      ( (sval_list_t *)(v) )
#define SVAL_ENTRY( v )     ( (sval_entry_t *)(v) )
#define SVAL_BLOCKLIST( v ) ( *(sval_stmtlist_t **)(v) )

enum {
	SVAL_ASSIGNMENT_STATEMENT       = 0x02,
	SVAL_STRING_REF                 = 0x03,
	SVAL_PRIMITIVE_EXPRESSION       = 0x04,
	SVAL_INTEGER_LITERAL            = 0x05,
	SVAL_FLOAT_LITERAL              = 0x06,
	SVAL_NEG_INTEGER_LITERAL        = 0x07,
	SVAL_NEG_FLOAT_LITERAL          = 0x08,
	SVAL_STRING                     = 0x09,
	SVAL_ISTRING                    = 0x0A,
	SVAL_ARRAY_REF                  = 0x0B,
	SVAL_FIELD_REF                  = 0x0C,
	SVAL_REFERENCE_EXPRESSION       = 0x0D,
	SVAL_SCRIPT_FUNCTION_NAME       = 0x0E,
	SVAL_CALL_VALUE                 = 0x0F,
	SVAL_FUNCTION_REF               = 0x10,
	SVAL_SCRIPT_FUNCTION_REF        = 0x11,
	SVAL_FUNCTION_POINTER_CALL      = 0x12,
	SVAL_FUNCTION_CALL_VALUE        = 0x13,
	SVAL_METHOD_CALL_VALUE          = 0x14,
	SVAL_CALL_STATEMENT             = 0x15,
	SVAL_FUNCTION_CALL              = 0x16,
	SVAL_RETURN_VALUE_STATEMENT     = 0x17,
	SVAL_RETURN_STATEMENT           = 0x18,
	SVAL_WAIT_STATEMENT             = 0x19,
	SVAL_METHOD_CALL                = 0x1A,
	SVAL_UNDEFINED                  = 0x1B,
	SVAL_SELF                       = 0x1C,
	SVAL_LEVEL                      = 0x1D,
	SVAL_GAME                       = 0x1E,
	SVAL_ANIM                       = 0x1F,
	SVAL_IF_STATEMENT               = 0x20,
	SVAL_IF_ELSE_STATEMENT          = 0x21,
	SVAL_WHILE_STATEMENT            = 0x22,
	SVAL_DO_WHILE_STATEMENT         = 0x23,
	SVAL_FOR_STATEMENT              = 0x24,
	SVAL_INC_STATEMENT              = 0x25,
	SVAL_DEC_STATEMENT              = 0x26,
	SVAL_REF_ASSIGNMENT_STATEMENT   = 0x27,
	SVAL_STATEMENT_BLOCK            = 0x28,
	SVAL_DEVELOPER_BLOCK            = 0x29,
	SVAL_EXPRESSION_LIST            = 0x2A,
	SVAL_BOOL_OR                    = 0x2B,
	SVAL_BOOL_AND                   = 0x2C,
	SVAL_BINARY_OPERATOR            = 0x2D,
	SVAL_CAST_BOOL                  = 0x2E,
	SVAL_CAST_INT                   = 0x2F,
	SVAL_CAST_FLOAT                 = 0x30,
	SVAL_CAST_STRING                = 0x31,
	SVAL_BOOL_NOT                   = 0x32,
	SVAL_BOOL_COMPLEMENT            = 0x33,
	SVAL_SIZE                       = 0x34,
	SVAL_WAITTILL_STATEMENT         = 0x36,
	SVAL_WAITTILLMATCH_STATEMENT    = 0x37,
	SVAL_NOTIFY_STATEMENT           = 0x38,
	SVAL_ENDON_STATEMENT            = 0x39,
	SVAL_SWITCH_STATEMENT           = 0x3A,
	SVAL_CASE_STATEMENT             = 0x3B,
	SVAL_DEFAULT_STATEMENT          = 0x3C,
	SVAL_BREAK_STATEMENT            = 0x3D,
	SVAL_CONTINUE_STATEMENT         = 0x3E,
	SVAL_FOR_CONDITION              = 0x3F,
	SVAL_EMPTY_ARRAY                = 0x40,
	SVAL_ANIM_STRING                = 0x41,
	SVAL_FUNCTION_DEFINITION        = 0x42,
	SVAL_DEVELOPER_FUNCTION_DEF     = 0x43,
	SVAL_USING_ANIMTREE             = 0x44,
	SVAL_LITERAL_ZERO               = 0x45,
	SVAL_LITERAL_ONE                = 0x46,
	SVAL_ANIMTREE                   = 0x47
};

enum {
	OP_End                      = 0x00,
	OP_Return                   = 0x01,
	OP_GetUndefined             = 0x02,
	OP_GetInteger               = 0x03,
	OP_GetFloat                 = 0x04,
	OP_GetString                = 0x05,
	OP_GetIString               = 0x06,
	OP_GetSelfObject            = 0x07,
	OP_GetLevelObject           = 0x08,
	OP_GetAnimObject            = 0x09,
	OP_GetSelf                  = 0x0A,
	OP_GetLevel                 = 0x0B,
	OP_GetGame                  = 0x0C,
	OP_GetAnim                  = 0x0D,
	OP_GetAnimation             = 0x0E,
	OP_GetGameRef               = 0x0F,
	OP_GetFunction              = 0x10,
	OP_EvalLocalVariable        = 0x11,
	OP_EvalLocalVariableRef     = 0x12,
	OP_ClearLocalVariable       = 0x13,
	OP_EvalArray                = 0x14,
	OP_EvalArrayRef             = 0x15,
	OP_EvalArrayRefNoCreate     = 0x16,
	OP_ClearArray               = 0x17,
	OP_EmptyArray               = 0x18,
	OP_EvalFieldVariable        = 0x19,
	OP_EvalFieldVariableRef     = 0x1A,
	OP_ClearFieldVariable       = 0x1B,
	OP_DiscardRef               = 0x1C,
	OP_SafeSetVariableField     = 0x1D,
	OP_SafeSetWaittillField     = 0x1E,
	OP_EndWaittill              = 0x1F,
	OP_CheckClearParams         = 0x20,
	OP_SetVariableField         = 0x21,
	OP_CallBuiltin              = 0x22,
	OP_CallBuiltinMethod        = 0x23,
	OP_Wait                     = 0x24,
	OP_PreScriptCall            = 0x25,
	OP_ScriptFunctionCall       = 0x26,
	OP_ScriptFunctionCallPtr    = 0x27,
	OP_ScriptMethodCall         = 0x28,
	OP_ScriptMethodCallPtr      = 0x29,
	OP_ScriptThreadCall         = 0x2A,
	OP_ScriptThreadCallPtr      = 0x2B,
	OP_ScriptMethodThreadCall   = 0x2C,
	OP_ScriptMethodThreadCallPtr= 0x2D,
	OP_DecTop                   = 0x2E,
	OP_CastFieldObject          = 0x2F,
	OP_CastBool                 = 0x30,
	OP_CastInt                  = 0x31,
	OP_CastFloat                = 0x32,
	OP_CastString               = 0x33,
	OP_BoolNot                  = 0x34,
	OP_BoolComplement           = 0x35,
	OP_JumpOnFalse              = 0x36,
	OP_JumpOnTrueBack           = 0x37,
	OP_JumpOnFalseExpr          = 0x38,
	OP_JumpOnTrueExpr           = 0x39,
	OP_Jump                     = 0x3A,
	OP_JumpBack                 = 0x3B,
	OP_Inc                      = 0x3C,
	OP_Dec                      = 0x3D,
	OP_Size                     = 0x4E,
	OP_WaitTillMatch            = 0x4F,
	OP_WaitTill                 = 0x50,
	OP_Notify                   = 0x51,
	OP_EndOn                    = 0x52,
	OP_VoidCodepos              = 0x53,
	OP_Switch                   = 0x54,
	OP_EndSwitch                = 0x55,
	OP_Vector                   = 0x56,
	OP_NOP                      = 0x57,
	OP_DevBlockBegin            = 0x58,
	OP_DevBlockDeferred         = 0x59
};

#define LOCALDEPTH_NONE         0
#define LOCALDEPTH_BUILTIN      1
#define LOCALDEPTH_THREAD       2
#define LOCALDEPTH_CALL         3

#define SCR_CODEGEN_INTERN      0
#define SCR_CODEGEN_RELOCATE    1
#define SCR_CODEGEN_DISCARD     2

#define SCR_STRING_USAGE_CASE       1
#define SCR_STRING_USAGE_FUNCTION   2
#define SCR_STRING_USAGE_CANONICAL  2

#define SCR_FUNC_REFCOUNT       0
#define SCR_FUNC_DEFINITION     1
#define SCR_FUNC_FIRST_REF      2

#define SCR_DEV_OP_BUF_SIZE     0x100000
#define SCR_DEV_PATCH_CAPACITY  0x10000
#define SCR_BUILTIN_PARAM_LIMIT 0x100
#define SCR_LOCAL_SLOT_WIDTH    0x20
#define SCR_OPERAND_STACK_LIMIT 0x800

#define SCR_CASE_TEST_BIAS      0x007E0000u
#define SCR_CASE_TEST_MAX       0x00FDFFFFu
#define SCR_CASE_ENCODE_BIAS    0x00800000u
#define SCR_CASE_ENCODE_MASK    0x00FFFFFFu

#define scrCompileGlob_animTreeValue    scrCompileGlob_animTreeValue    /* 0x008E5ED8 */
#define scrCompilePub_developerBufPos   developerCodeStart    /* 0x008E5EE4 */
#define scrCompileGlob_codegenMode      developer_script_int /* 0x008E5EE8 */
#define scrCompileGlob_currentFuncRoot  scrCompileGlob_currentFuncRoot    /* 0x008E5EEC (low word) */
#define scrCompileGlob_loadCursor       scrCompileGlob_loadCursor    /* 0x008E5EF4 */
#define scrCompileGlob_codePos          scrCompileGlob_codePos    /* 0x008E5EFC */
#define scrCompileGlob_codeRelocStart   scrCompileGlob_codeRelocStart    /* 0x008E5F00 */
#define scrCompileGlob_lastOpcodePos    scrCompileGlob_lastOpcodePos    /* 0x00966430 */
#define scrCompilePub_checksum          scrCompilePub_checksum   /* 0x01407364 */
#define scrVarPub_programBuffer         scrVarPub_programBuffer   /* 0x01407380 */
#define scrCompilePub_programLen        scrCompilePub_programLen   /* 0x01407388 */
#define scrCompileGlob_devOpBuffer      scrCompileGlob_devOpBuffer   /* 0x0140738C */
#define scrCompileGlob_devPatchMax      scrCompileGlob_devPatchMax   /* 0x01407390 */
#define scrCompileGlob_devPatchTable    scrCompileGlob_devPatchTable   /* 0x01407394 */
#define scrCompileGlob_stringFixups     scrCompileGlob_stringFixups   /* 0x01407398 */
#define scrCompileGlob_breakAllowed     scrCompileGlob_breakAllowed    /* 0x0140739C */
#define scrCompileGlob_breakAllowedDev  scrCompileGlob_breakAllowedDev    /* 0x0140739D */
#define scrCompileGlob_breakList        scrCompileGlob_breakList   /* 0x014073A0 */
#define scrCompileGlob_contAllowed      scrCompileGlob_contAllowed    /* 0x014073A4 */
#define scrCompileGlob_contAllowedDev   scrCompileGlob_contAllowedDev    /* 0x014073A5 */
#define scrCompileGlob_contList         scrCompileGlob_contList   /* 0x014073A8 */
#define scrCompileGlob_caseAllowed      scrCompileGlob_caseAllowed    /* 0x014073AC */
#define scrCompileGlob_caseAllowedDev   scrCompileGlob_caseAllowedDev    /* 0x014073AD */
#define scrCompileGlob_caseList         scrCompileGlob_caseList   /* 0x014073B0 */
#define scrAnimPub_currentAnimTree      scrAnimPub_usingTreeNames    /* 0x008E60B0 (low word) */
#define scrCompilePub_loadedScripts     scrCompilePub_loadedScripts    /* 0x008E60B4 (low word) */
#define scrCompilePub_scriptCount       scrCompilePub_scriptCount    /* 0x008E60B8 */
#define scrCompilePub_developerScript   scrVarPub_developerScript    /* 0x008E60C4 */
#define scrCompilePub_scriptsAvailable  scrVarPub_developer    /* 0x008E60D0 */

#define SCR_HANDLE( g )     ( *(unsigned short *)&(g) )

#define scrCompileGlob_bDeferredCheck   ( *( (unsigned char *)&scrAnimPub_currentTreeRoot + 2 ) )
#define scrCompileGlob_bOwnsStrings     ( *( (unsigned char *)&scrAnimPub_currentTreeRoot + 3 ) )

int     scrCompileGlob_cumulOffset;     /* retail 0x008E5EF0 */
int     scrCompileGlob_maxOffset;       /* retail 0x008E5EF8 */
int     scrCompileGlob_maxCallOffset;   /* retail 0x008E5EE0 */
/* ---- module-owned globals ---- */
int scrCompileGlob_devPatchMax;   /* 0x01407390 */
void *scrCompileGlob_stringFixups;   /* 0x01407398 */
unsigned char scrCompileGlob_breakAllowed;   /* 0x0140739C */
unsigned char scrCompileGlob_breakAllowedDev;   /* 0x0140739D */
int scrCompileGlob_breakList;   /* 0x014073A0 */
unsigned char scrCompileGlob_contAllowed;   /* 0x014073A4 */
unsigned char scrCompileGlob_contAllowedDev;   /* 0x014073A5 */
int scrCompileGlob_contList;   /* 0x014073A8 */
unsigned char scrCompileGlob_caseAllowed;   /* 0x014073AC */
unsigned char scrCompileGlob_caseAllowedDev;   /* 0x014073AD */
int scrCompileGlob_caseList;   /* 0x014073B0 */
int scrCompileGlob_loadCursor;   /* 0x008E5EF4 */
int scrCompileGlob_codePos;   /* 0x008E5EFC */
int scrCompileGlob_codeRelocStart;   /* 0x008E5F00 */
int scrCompileGlob_lastOpcodePos;   /* 0x00966430 */
void *scrCompileGlob_devOpBuffer;   /* 0x0140738C */
void *scrCompileGlob_devPatchTable;   /* 0x01407394 */
int scrCompileGlob_animTreeValue;   /* 0x008E5ED8 */
int developer_script_int;   /* 0x008E5EE8 */
char *developerCodeStart;   /* 0x008E5EE4 */
int scrCompilePub_checksum;   /* 0x01407364 */
int scrCompilePub_programLen;   /* 0x01407388 */
int scrCompilePub_scriptCount;   /* 0x008E60B8 */
int scrCompilePub_scriptfilename;   /* 0x01407374 */
int scrCompilePub_sourceBuf;   /* 0x01407370 */
/* retail's 16-bit object handles (never address-taken) */
unsigned short scrCompileGlob_currentFuncRoot;   /* 0x008E5EEC */
unsigned short scrCompilePub_loadedScripts;   /* 0x008E60B4 */
unsigned short scrCompilePub_scriptsPos;   /* 0x008E60CC, the loaded-scripts object */

typedef struct scrCodeOffsetPatch_s {
	char                           *codePos;
	struct scrCodeOffsetPatch_s    *next;
} scrCodeOffsetPatch_t;

typedef struct scrCaseRecord_s {
	unsigned int                    value;
	unsigned int                    codePos;
	unsigned int                    sourcePos;
	struct scrCaseRecord_s         *next;
} scrCaseRecord_t;

typedef struct scrCaseTableEntry_s {
	unsigned int                    value;
	unsigned int                    codePos;
} scrCaseTableEntry_t;

typedef struct scrStringFixup_s {
	unsigned short                 *codePos;
	struct scrStringFixup_s        *next;
} scrStringFixup_t;

typedef struct scrScriptLoad_s {
	unsigned short                  filename;
	unsigned short                  pad;
	unsigned int                    sourcePos;
} scrScriptLoad_t;

extern void         *Hunk_AllocateTempMemoryHighInternal( int size );
extern int           Hunk_CommitTempMemory( void );
extern int           Hunk_ClearTempMemoryHigh( void );

extern void          AddOpcodePos( unsigned int sourcePos );
extern void QDECL    CompileError( unsigned int sourcePos, const char *format, ... );
extern void QDECL    CompileError2( const char *codePos, const char *format, ... );

extern unsigned short SL_TransferToCanonicalString( unsigned short handle );
extern qboolean      Scr_LoadScript( const char *filename );

#ifdef COD1_VC7
extern unsigned char Scr_IsIdentifier( const char *name );
#else
extern qboolean      Scr_IsIdentifier( const char *name );
#endif

extern void         *Scr_GetFunction( const char **name, int *developerOnly );
extern void         *Scr_GetMethod( const char **name, int *developerOnly );

extern void          Scr_EmitAnimationInternal( char *animRef, unsigned short animName,
												unsigned short tree, unsigned int sourcePos );
extern unsigned short Scr_UsingTreeInternal( const char *name, int *animTreeValue );
extern void           Scr_UsingTree( const char *name, unsigned int sourcePos );

static void  EmitExpression( sval_t node );
static void  EmitPrimitiveExpression( sval_t node );
static void  EmitPrimitiveExpressionFieldObject( sval_t node, unsigned int sourcePos );
static void  EmitExpressionFieldObject( sval_u node, sval_u sourcePos );
static void  EmitVariableExpression( sval_t node );
static void  EmitVariableExpressionRef( sval_t node, unsigned char bCreate );
static void  EmitArrayPrimitiveExpressionRef( sval_u node, unsigned char bCreate,
											  sval_u sourcePos );
static void  EmitArrayExpressionRef( sval_u node, unsigned char bCreate, sval_u sourcePos );
static void  EmitStatement( sval_t node );
static void  EmitStatementList( sval_u block );
static void  EmitCall( sval_u callNode, sval_u args, unsigned char bDropTop );
static void  EmitMethod( sval_u objectNode, sval_u callNode, sval_u args,
						 sval_u methodSourcePos, unsigned char bDropTop );
static void  EmitFunction( sval_u func, sval_u sourcePos );
static int   EmitExpressionList( sval_u list );
static void  AddExpressionListOpcodePos( sval_list_t *list );
static void  Scr_TransferToDeveloperBuffer( void );

static void  Scr_TransferStatementListToDeveloperBuffer( void );
static void  EmitDeveloperStatementList( sval_u block, sval_u sourcePos );

void         Scr_InitDeveloperOpcodes__Fv( void );
void         Scr_InsertDeveloperOpcodes__Fv( void );
void         Scr_ShutdownDeveloperOpcodes__Fv( void );

/* ---- CompileRemoveRefToString  0x00468820 ---- */
static void CompileRemoveRefToString( unsigned short string ) {
	if ( !scrCompileGlob_bOwnsStrings ) {
		SL_RemoveRefToString( string );
	}
}

/* ---- EmitCanonicalString  0x00468870 ---- */
static void EmitCanonicalString( unsigned short string ) {
	unsigned short     *out;
	scrStringFixup_t   *fixup;

	out = (unsigned short *)TempMalloc( 2 );
	scrCompileGlob_codePos = (int)out;

	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_DISCARD ) {
		CompileRemoveRefToString( string );
		return;
	}

	if ( scrCompileGlob_bOwnsStrings ) {
		SL_AddRefToString( string );
	}

	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_INTERN ) {
		*out = SL_TransferToCanonicalString( string );
		return;
	}

	*out = string;
	fixup = (scrStringFixup_t *)Z_MallocInternal( (int)sizeof( scrStringFixup_t ) );
	fixup->codePos = (unsigned short *)( (char *)scrCompileGlob_codePos
					+ ( (int)scrCompilePub_developerBufPos - scrCompileGlob_codeRelocStart ) );
	*fixup->codePos = 0;
	fixup->next = (scrStringFixup_t *)scrCompileGlob_stringFixups;
	scrCompileGlob_stringFixups = (void *)fixup;
}

/* ---- CompileTransferRefToString  0x00468910 ---- */
static void CompileTransferRefToString( unsigned short string, unsigned char usage ) {
	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_DISCARD ) {
		CompileRemoveRefToString( string );
		return;
	}

	if ( scrCompileGlob_bOwnsStrings ) {
		SL_AddRefToString( string );
	}
	SL_TransferRefToString( string, usage );
}

/* ---- EmitOpcode  0x00468950 ---- */
static void EmitOpcode( unsigned char opcode, int stackDelta, int localDepthMode ) {
	if ( scrCompileGlob_bDeferredCheck && scrCompileGlob_codegenMode == SCR_CODEGEN_INTERN ) {
		scrCompileGlob_bDeferredCheck = 0;
		Scr_TransferStatementListToDeveloperBuffer();
	}

	scrCompileGlob_cumulOffset += stackDelta;
	if ( scrCompileGlob_maxOffset < scrCompileGlob_cumulOffset ) {
		scrCompileGlob_maxOffset = scrCompileGlob_cumulOffset;
	}

	if ( localDepthMode && scrCompileGlob_maxCallOffset < scrCompileGlob_cumulOffset ) {
		scrCompileGlob_maxCallOffset = scrCompileGlob_cumulOffset;
		if ( localDepthMode == LOCALDEPTH_CALL ) {
			scrCompileGlob_maxCallOffset = scrCompileGlob_cumulOffset + 1;
		}
	}

	scrCompileGlob_codePos       = (int)TempMalloc( 1 );
	scrCompileGlob_lastOpcodePos = scrCompileGlob_codePos;
	*(unsigned char *)scrCompileGlob_codePos = opcode;
	scrCompilePub_checksum = scrCompilePub_checksum * 31 + opcode;
}

/* ---- EmitNOP  0x004689F0 ---- */
static void EmitNOP( void ) {
	EmitOpcode( OP_NOP, 0, LOCALDEPTH_NONE );
}

/* ---- EmitInteger  0x00468A00 ---- */
static void EmitInteger( int value ) {
	int    *out = (int *)TempMalloc( 4 );

	scrCompileGlob_codePos = (int)out;
	*out = value;
}

/* ---- EmitShort  0x00468A30 ---- */
static void EmitShort( short value ) {
	short  *out = (short *)TempMalloc( 2 );

	scrCompileGlob_codePos = (int)out;
	*out = value;
}

/* ---- EmitByte  0x00468A60 ---- */
static void EmitByte( unsigned char value ) {
	unsigned char  *out = (unsigned char *)TempMalloc( 1 );

	scrCompileGlob_codePos = (int)out;
	*out = value;
}

/* ---- EmitFloat  0x00468A90 ---- */
static void EmitFloat( float value ) {
	float  *out = (float *)TempMalloc( 4 );

	scrCompileGlob_codePos = (int)out;
	*out = value;
}

/* ---- EmitString  0x00468AC0 ---- */
static void EmitString( unsigned short value ) {
	unsigned short *out = (unsigned short *)TempMalloc( 2 );

	scrCompileGlob_codePos = (int)out;
	*out = value;
}

/* ---- EmitBlankPos  0x00468AF0 ---- */
static void EmitBlankPos( void ) {
	int    *out = (int *)TempMalloc( 4 );

	scrCompileGlob_codePos = (int)out;
	*out = 0;
}

/* ---- EmitCodepos  0x00468B20 ---- */
static void EmitCodepos( const char *codePos ) {
	const char **out = (const char **)TempMalloc( 4 );

	scrCompileGlob_codePos = (int)out;
	*out = codePos;
}

/* ---- EmitBuiltinFunction  0x00468B50 ---- */
static void EmitBuiltinFunction( void *function ) {
	void  **out = (void **)TempMalloc( 4 );

	scrCompileGlob_codePos = (int)out;
	*out = function;
}

/* ---- EmitBuiltinMethod  0x00468B80 ---- */
static void EmitBuiltinMethod( void *method ) {
	void  **out = (void **)TempMalloc( 4 );

	scrCompileGlob_codePos = (int)out;
	*out = method;
}

/* ---- EmitGetFloat  0x00468C00 ---- */
static void EmitGetFloat( float value ) {
	EmitOpcode( OP_GetFloat, 1, LOCALDEPTH_NONE );
	EmitFloat( value );
}

static void EmitGetInteger( int value ) {
	EmitOpcode( OP_GetInteger, 1, LOCALDEPTH_NONE );
	EmitInteger( value );
}

static void EmitFalse( void ) {
	EmitGetInteger( 0 );
}

static void EmitTrue( void ) {
	EmitGetInteger( 1 );
}

static void EmitIncTop( void ) {
	EmitOpcode( OP_GetUndefined, 1, LOCALDEPTH_NONE );
}

static void EmitSelf( void )  { EmitOpcode( OP_GetSelf,  1, LOCALDEPTH_NONE ); }
static void EmitLevel( void ) { EmitOpcode( OP_GetLevel, 1, LOCALDEPTH_NONE ); }
static void EmitGame( void )  { EmitOpcode( OP_GetGame,  1, LOCALDEPTH_NONE ); }
static void EmitAnim( void )  { EmitOpcode( OP_GetAnim,  1, LOCALDEPTH_NONE ); }

static void EmitEmptyArray( void ) {
	EmitOpcode( OP_EmptyArray, 1, LOCALDEPTH_NONE );
}

static void EmitAnimTree( unsigned int sourcePos ) {
	if ( !scrCompileGlob_animTreeValue ) {
		CompileError( sourcePos, "#using_animtree was not specified" );
		return;
	}
	EmitOpcode( OP_GetInteger, 1, LOCALDEPTH_NONE );
	EmitInteger( scrCompileGlob_animTreeValue );
}

/* ---- EmitSetVariableField  0x00468D20 ---- */
static void EmitSetVariableField( unsigned int sourcePos ) {
	EmitOpcode( OP_SetVariableField, -1, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitSafeSetVariableField  0x00468D40 ---- */
static void EmitSafeSetVariableField( sval_u expr, sval_u sourcePos ) {
	EmitOpcode( OP_SafeSetVariableField, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
	EmitCanonicalString( (unsigned short)expr.stringValue );
}

/* ---- EmitSafeSetWaittillVariableField  0x00468D70 ---- VERIFIED */
static void EmitSafeSetWaittillVariableField( sval_u expr, sval_u sourcePos ) {
	EmitOpcode( OP_SafeSetWaittillField, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
	EmitCanonicalString( (unsigned short)expr.stringValue );
}

/* ---- EmitGetString  0x00468DA0 ---- */
static void EmitGetString( unsigned short string ) {
	EmitOpcode( OP_GetString, 1, LOCALDEPTH_NONE );
	EmitString( string );
	CompileTransferRefToString( string, SCR_STRING_USAGE_CASE );
}

/* ---- EmitGetIString  0x00468E20 ---- */
static void EmitGetIString( unsigned short string ) {
	EmitOpcode( OP_GetIString, 1, LOCALDEPTH_NONE );
	EmitString( string );
	CompileTransferRefToString( string, SCR_STRING_USAGE_CASE );
}

/* ---- EmitCastBool  0x00468EA0 ---- VERIFIED */
static void EmitCastBool( unsigned int sourcePos ) {
	EmitOpcode( OP_CastBool, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitCastInt  0x00468EC0 ---- VERIFIED */
static void EmitCastInt( unsigned int sourcePos ) {
	EmitOpcode( OP_CastInt, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitCastFloat  0x00468EE0 ---- VERIFIED */
static void EmitCastFloat( unsigned int sourcePos ) {
	EmitOpcode( OP_CastFloat, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitCastString  0x00468F00 ---- VERIFIED */
static void EmitCastString( unsigned int sourcePos ) {
	EmitOpcode( OP_CastString, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitBoolNot  0x00468F20 ---- VERIFIED */
static void EmitBoolNot( unsigned int sourcePos ) {
	EmitOpcode( OP_BoolNot, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitBoolComplement  0x00468F40 ---- VERIFIED */
static void EmitBoolComplement( unsigned int sourcePos ) {
	EmitOpcode( OP_BoolComplement, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitSize  0x00468F60 ---- */
static void EmitSize( sval_u node, sval_u sourcePos ) {
	EmitPrimitiveExpression( node.value );
	EmitOpcode( OP_Size, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
}

/* ---- EmitSelfObject  0x00468FD0 ---- VERIFIED */
static void EmitSelfObject( void ) {
	EmitOpcode( OP_GetSelfObject, 0, LOCALDEPTH_NONE );
}

/* ---- EmitLevelObject  0x00468FE0 ---- VERIFIED */
static void EmitLevelObject( void ) {
	EmitOpcode( OP_GetLevelObject, 0, LOCALDEPTH_NONE );
}

/* ---- EmitAnimObject  0x00468FF0 ---- VERIFIED */
static void EmitAnimObject( void ) {
	EmitOpcode( OP_GetAnimObject, 0, LOCALDEPTH_NONE );
}

/* ---- EmitLocalVariable  0x00469000 ---- */
static void EmitLocalVariable( sval_u string ) {
	EmitOpcode( OP_EvalLocalVariable, 1, LOCALDEPTH_NONE );
	EmitCanonicalString( (unsigned short)string.stringValue );
}

/* ---- EmitLocalVariableRef  0x00469020 ---- */
static void EmitLocalVariableRef( sval_u string ) {
	EmitOpcode( OP_EvalLocalVariableRef, 0, LOCALDEPTH_NONE );
	EmitCanonicalString( (unsigned short)string.stringValue );
}

/* ---- EmitGameRef  0x00469040 ---- */
static void EmitGameRef( void ) {
	EmitOpcode( OP_GetGameRef, 0, LOCALDEPTH_NONE );
}

/* ---- EmitClearLocalVariable  0x00469050 ---- */
static void EmitClearLocalVariable( sval_u string ) {
	EmitOpcode( OP_ClearLocalVariable, 0, LOCALDEPTH_NONE );
	EmitCanonicalString( (unsigned short)string.stringValue );
}

/* ---- EmitEvalArray  0x00469070 ---- */
static void EmitEvalArray( unsigned int arraySourcePos, unsigned int indexSourcePos ) {
	EmitOpcode( OP_EvalArray, -1, LOCALDEPTH_NONE );
	AddOpcodePos( indexSourcePos );
	AddOpcodePos( arraySourcePos );
}

/* ---- EmitEvalArrayRef  0x004690A0 ---- VERIFIED */
static void EmitEvalArrayRef( unsigned char bCreate, unsigned int arraySourcePos,
							  unsigned int indexSourcePos ) {
	if ( bCreate ) {
		EmitOpcode( OP_EvalArrayRef, -1, LOCALDEPTH_NONE );
	} else {
		EmitOpcode( OP_EvalArrayRefNoCreate, -1, LOCALDEPTH_NONE );
	}
	AddOpcodePos( indexSourcePos );
	AddOpcodePos( arraySourcePos );
}

/* ---- EmitClearArray  0x004690D0 ---- */
static void EmitClearArray( unsigned int arraySourcePos, unsigned int indexSourcePos ) {
	EmitOpcode( OP_ClearArray, -1, LOCALDEPTH_NONE );
	AddOpcodePos( indexSourcePos );
	AddOpcodePos( arraySourcePos );
}

/* ---- EmitAnimation  0x00469110 ---- */
static void EmitAnimation( sval_u string, sval_u sourcePos ) {
	EmitOpcode( OP_GetAnimation, 1, LOCALDEPTH_NONE );
	EmitInteger( -1 );
	AddOpcodePos( sourcePos.sourcePosValue );

	if ( !scrAnimPub_currentAnimTree ) {
		CompileError( sourcePos.sourcePosValue, "#using_animtree was not specified" );
	} else {
		Scr_EmitAnimationInternal( (char *)scrCompileGlob_codePos,
								   (unsigned short)string.stringValue,
								   scrAnimPub_currentAnimTree,
								   sourcePos.sourcePosValue );
	}
	CompileRemoveRefToString( (unsigned short)string.stringValue );
}

/* ---- EmitFieldVariable  0x004691D0 ---- */
static void EmitFieldVariable( sval_u objectNode, sval_u string, sval_u sourcePos ) {
	EmitPrimitiveExpressionFieldObject( objectNode.value, sourcePos.sourcePosValue );
	EmitOpcode( OP_EvalFieldVariable, 1, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
	EmitCanonicalString( (unsigned short)string.stringValue );
}

/* ---- EmitFieldVariableRef  0x00469210 ---- */
static void EmitFieldVariableRef( sval_u objectNode, sval_u string, sval_u sourcePos ) {
	EmitPrimitiveExpressionFieldObject( objectNode.value, sourcePos.sourcePosValue );
	EmitOpcode( OP_EvalFieldVariableRef, 0, LOCALDEPTH_NONE );
	EmitCanonicalString( (unsigned short)string.stringValue );
}

/* ---- EmitClearFieldVariable  0x00469240 ---- */
static void EmitClearFieldVariable( sval_u objectNode, sval_u string,
									sval_u objectSourcePos, sval_u opcodeSourcePos ) {
	EmitPrimitiveExpressionFieldObject( objectNode.value, objectSourcePos.sourcePosValue );
	EmitOpcode( OP_ClearFieldVariable, 0, LOCALDEPTH_NONE );
	AddOpcodePos( opcodeSourcePos.sourcePosValue );
	EmitCanonicalString( (unsigned short)string.stringValue );
}

/* ---- EmitCallRef  0x00469280 ---- */
static void EmitCallRef( sval_u nameNode, sval_u args, sval_u sourcePos ) {
	EmitCall( nameNode, args, qfalse );
	EmitOpcode( OP_DiscardRef, -1, LOCALDEPTH_NONE );
}

/* ---- EmitMethodRef  0x004692C0 ---- */
static void EmitMethodRef( sval_u objectNode, sval_u nameNode, sval_u args,
						   sval_u methodSourcePos ) {
	EmitMethod( objectNode, nameNode, args, methodSourcePos, qfalse );
	EmitOpcode( OP_DiscardRef, -1, LOCALDEPTH_NONE );
}

/* ---- EmitDecTop  0x004692F0 ---- */
static void EmitDecTop( void ) {
	EmitOpcode( OP_DecTop, -1, LOCALDEPTH_NONE );
}

/* ---- EmitCastFieldObject  0x00469300 ---- */
static void EmitCastFieldObject( unsigned int sourcePos ) {
	EmitOpcode( OP_CastFieldObject, -1, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
}

/* ---- EmitArrayVariable  0x00469320 ---- */
static void EmitArrayVariable( sval_u objectNode, sval_u indexNode,
							   sval_u objectSourcePos, sval_u indexSourcePos ) {
	EmitExpression( indexNode.value );
	EmitPrimitiveExpression( objectNode.value );
	EmitEvalArray( objectSourcePos.sourcePosValue, indexSourcePos.sourcePosValue );
}

/* ---- EmitArrayVariableRef  0x00469360 ---- */
static void EmitArrayVariableRef( sval_u arrayNode, sval_u indexNode,
								  sval_u arraySourcePos, sval_u indexSourcePos,
								  unsigned char bCreate ) {
	EmitExpression( indexNode.value );
	EmitArrayPrimitiveExpressionRef( arrayNode, bCreate, arraySourcePos );
	EmitEvalArrayRef( bCreate, arraySourcePos.sourcePosValue,
					  indexSourcePos.sourcePosValue );
}

/* ---- EmitClearArrayVariable  0x004693B0 ---- */
static void EmitClearArrayVariable( sval_u arrayNode, sval_u indexNode,
									sval_u arraySourcePos, sval_u indexSourcePos ) {
	EmitExpression( indexNode.value );
	EmitArrayPrimitiveExpressionRef( arrayNode, qfalse, arraySourcePos );
	EmitClearArray( arraySourcePos.sourcePosValue, indexSourcePos.sourcePosValue );
}

/* ---- EmitVariableExpression  0x004693F0 ---- */
static void EmitVariableExpression( sval_t node ) {
	sval_t *n = SVAL_NODE( node );

	switch ( n[0] ) {
	case SVAL_STRING_REF:
		EmitLocalVariable( SVAL_U( n[1] ) );
		break;
	case SVAL_ARRAY_REF:
		EmitArrayVariable( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_FIELD_REF:
		EmitFieldVariable( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	default:
		break;
	}
}

/* ---- EmitExpressionList  0x00469490 ---- */
static int EmitExpressionList( sval_u list ) {
	sval_cell_t    *item;
	int             count = 0;

	for ( item = SVAL_LIST( list.value )->first; item; item = item->next ) {
		EmitExpression( SVAL_ENTRY( item->value )->node );
		count++;
	}
	return count;
}

/* ---- GetSingleParameter  0x004694C0 ---- */
sval_cell_t *GetSingleParameter( sval_u list ) {
	sval_cell_t *item = SVAL_LIST( list.value )->first;
	if ( !item ) return NULL;
	return item->next ? NULL : item;
}

/* ---- AddExpressionListOpcodePos  0x004694E0 ---- */
static void AddExpressionListOpcodePos( sval_list_t *list ) {
	sval_cell_t    *item;

	if ( !scrCompilePub_scriptsAvailable ) {
		return;
	}
	for ( item = list->first; item; item = item->next ) {
		AddOpcodePos( SVAL_ENTRY( item->value )->sourcePos );
	}
}

/* ---- AddFilePrecache  0x00469510 ---- */
static unsigned short AddFilePrecache( unsigned short filename, unsigned int sourcePos ) {
	scrScriptLoad_t    *cursor;

	SL_AddRefToString( filename );

	cursor = (scrScriptLoad_t *)scrCompileGlob_loadCursor;
	cursor->filename  = filename;
	cursor->sourcePos = sourcePos;
	scrCompileGlob_loadCursor = (int)( cursor + 1 );

	return GetObject( GetVariable( scrCompilePub_scriptsPos, filename ) );
}

/* ---- EmitFunction  0x00469560 ---- */
static void EmitFunction( sval_u func, sval_u sourcePos ) {
	sval_t             *n = SVAL_NODE( func.value );
	unsigned short      functionObject;
	unsigned short      functionName;
	unsigned short      functionHandle;
	unsigned short      scriptFilename;
	unsigned short      scriptFunctionName;
	unsigned short      canonical;
	unsigned short      loadedScript;
	unsigned short      scriptRoot;
	unsigned short      countSlot;
	unsigned short      referenceSlot;
	VariableValue       functionValue;
	VariableValue       countValue;
	VariableValue       referenceValue;

	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_DISCARD ) {
		CompileRemoveRefToString( (unsigned short)n[1] );
		if ( n[0] == SVAL_SCRIPT_FUNCTION_REF ) {
			CompileRemoveRefToString( (unsigned short)n[2] );
			scrCompilePub_scriptCount--;
		}
		return;
	}

	functionObject = 0;

	switch ( n[0] ) {
	case SVAL_FUNCTION_REF:
		functionName   = (unsigned short)n[1];
		functionHandle = FindVariable( scrCompileGlob_currentFuncRoot,
									   functionName );
		CompileTransferRefToString( functionName, SCR_STRING_USAGE_FUNCTION );
		if ( !functionHandle ) {
			CompileError( sourcePos.sourcePosValue, "unknown function" );
			return;
		}
		functionObject = FindObject( functionHandle );
		break;
	case SVAL_SCRIPT_FUNCTION_REF:
		scriptFilename = (unsigned short)n[1];

		canonical = Scr_CreateCanonicalFilename( SL_ConvertToString( scriptFilename ) );
		CompileRemoveRefToString( (unsigned short)n[1] );

		loadedScript = FindVariable( scrCompilePub_loadedScripts, canonical );
		scriptRoot   = AddFilePrecache( canonical, sourcePos.sourcePosValue );
		CompileRemoveRefToString( canonical );

		if ( loadedScript ) {
			scriptFunctionName = (unsigned short)n[2];
			functionHandle = FindVariable( scriptRoot, scriptFunctionName );
			CompileTransferRefToString( scriptFunctionName, SCR_STRING_USAGE_FUNCTION );
			if ( !functionHandle ) {
				CompileError( sourcePos.sourcePosValue, "unknown function" );
				return;
			}
		} else {
			functionHandle = GetVariable( scriptRoot, (unsigned short)n[2] );
			CompileTransferRefToString( (unsigned short)n[2], SCR_STRING_USAGE_FUNCTION );
		}

		GetVariableValue( functionHandle, &functionValue );
		if ( functionValue.type == VAR_CODEPOS ) {
			EmitCodepos( functionValue.u.codePosValue );
			return;
		}
		if ( functionValue.type == VAR_INTEGER ) {
			if ( scrCompileGlob_codegenMode != SCR_CODEGEN_RELOCATE ) {
				CompileError( sourcePos.sourcePosValue,
							  "normal script cannot reference /# ... #/ comment" );
			}
			EmitCodepos( (const char *)functionValue.u.intValue );
			return;
		}
		functionObject = GetObject( functionHandle );
		break;
	}

	EmitBlankPos();
	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_DISCARD ) {
		return;
	}

	countSlot = GetVariable( functionObject, SCR_FUNC_REFCOUNT );
	GetVariableValue( countSlot, &countValue );
	if ( countValue.type == VAR_UNDEFINED ) {
		countValue.type = VAR_INTEGER;
		countValue.u.intValue = 0;
	}

	referenceSlot = GetVariable( functionObject,
								 (unsigned int)countValue.u.intValue + SCR_FUNC_FIRST_REF );
	/* the code position is stored first and overwritten for /# #/ code (Mac 1.3 order) */
	referenceValue.u.intValue = scrCompileGlob_codePos;
	if ( scrCompileGlob_codegenMode != SCR_CODEGEN_RELOCATE ) {
		referenceValue.type = VAR_CODEPOS;
	} else {
		referenceValue.type = VAR_INTEGER;
		referenceValue.u.codePosValue = scrCompilePub_developerBufPos
			+ ( scrCompileGlob_codePos - scrCompileGlob_codeRelocStart );
	}
	SetNewVariableValue( referenceSlot, &referenceValue );

	countValue.u.intValue++;
	SetVariableValue( countSlot, &countValue );
	AddOpcodePos( sourcePos.sourcePosValue );
}

/* ---- EmitGetFunction  0x00469830 ---- */
static void EmitGetFunction( sval_u func, sval_u sourcePos ) {
	EmitOpcode( OP_GetFunction, 1, LOCALDEPTH_NONE );
	EmitFunction( func, sourcePos );
}

/* ---- EmitPostScriptFunction  0x00469850 ---- */
static void EmitPostScriptFunction( sval_u func, int argCount,
									unsigned char bMethod, sval_u sourcePos ) {
	if ( !bMethod ) {
		EmitOpcode( OP_ScriptFunctionCall, -1 - argCount, LOCALDEPTH_CALL );
	} else {
		EmitOpcode( OP_ScriptMethodCall, -2 - argCount, LOCALDEPTH_CALL );
	}
	EmitFunction( func, sourcePos );
	EmitInteger( argCount );
}

/* ---- EmitPostScriptFunctionPointer  0x004698B0 ---- */
static void EmitPostScriptFunctionPointer( sval_u expr, int argCount,
										   unsigned char bMethod,
										   sval_u callSourcePos,
										   sval_u objectSourcePos ) {
	EmitExpression( expr.value );
	if ( !bMethod ) {
		EmitOpcode( OP_ScriptFunctionCallPtr, -2 - argCount, LOCALDEPTH_CALL );
	} else {
		EmitOpcode( OP_ScriptMethodCallPtr, -3 - argCount, LOCALDEPTH_CALL );
	}
	AddOpcodePos( objectSourcePos.sourcePosValue );
	AddOpcodePos( callSourcePos.sourcePosValue );
	EmitInteger( argCount );
}

/* ---- EmitPostScriptThread  0x00469920 ---- */
static void EmitPostScriptThread( sval_u func, int argCount,
								  unsigned char bMethod, sval_u sourcePos ) {
	if ( !bMethod ) {
		EmitOpcode( OP_ScriptThreadCall, 1 - argCount, LOCALDEPTH_THREAD );
	} else {
		EmitOpcode( OP_ScriptMethodThreadCall, -argCount, LOCALDEPTH_THREAD );
	}
	EmitFunction( func, sourcePos );
	EmitInteger( argCount );
}

/* ---- EmitPostScriptThreadPointer  0x00469980 ---- */
static void EmitPostScriptThreadPointer( sval_u expr, int argCount,
										 unsigned char bMethod,
										 sval_u sourcePos ) {
	EmitExpression( expr.value );
	if ( !bMethod ) {
		EmitOpcode( OP_ScriptThreadCallPtr, -argCount, LOCALDEPTH_THREAD );
	} else {
		EmitOpcode( OP_ScriptMethodThreadCallPtr, -1 - argCount, LOCALDEPTH_THREAD );
	}
	AddOpcodePos( sourcePos.sourcePosValue );
	EmitInteger( argCount );
}

/* ---- EmitPostScriptFunctionCall  0x004699F0 ---- */
static void EmitPostScriptFunctionCall( sval_u func, int argCount,
										unsigned char bMethod,
										sval_u callSourcePos ) {
	sval_t *callee = SVAL_NODE( func.value );

	switch ( callee[0] ) {
	case SVAL_SCRIPT_FUNCTION_NAME:
		EmitPostScriptFunction( SVAL_U( callee[1] ), argCount, bMethod, callSourcePos );
		break;
	case SVAL_FUNCTION_POINTER_CALL:
		EmitPostScriptFunctionPointer( SVAL_U( callee[1] ), argCount, bMethod,
									   callSourcePos, SVAL_U( callee[2] ) );
		break;
	}
}

/* ---- EmitPostScriptThreadCall  0x00469A30 ---- */
static void EmitPostScriptThreadCall( sval_u func, int argCount,
									  unsigned char bMethod,
									  sval_u callSourcePos,
									  sval_u methodSourcePos ) {
	sval_t *callee = SVAL_NODE( func.value );

	switch ( callee[0] ) {
	case SVAL_SCRIPT_FUNCTION_NAME:
		EmitPostScriptThread( SVAL_U( callee[1] ), argCount, bMethod, methodSourcePos );
		break;
	case SVAL_FUNCTION_POINTER_CALL:
		EmitPostScriptThreadPointer( SVAL_U( callee[1] ), argCount, bMethod,
									 SVAL_U( callee[2] ) );
		break;
	}
	AddOpcodePos( callSourcePos.sourcePosValue );
}

/* ---- EmitPreFunctionCall  0x00469A90 ---- */
static void EmitPreFunctionCall( sval_u callNode ) {
	if ( SVAL_NODE( callNode.value )[0] == SVAL_FUNCTION_CALL ) {
		EmitOpcode( OP_PreScriptCall, 2, LOCALDEPTH_NONE );
	}
}

/* ---- EmitPostFunctionCall  0x00469AB0 ---- */
static void EmitPostFunctionCall( sval_u func, int argCount, unsigned char bMethod ) {
	sval_t *n = SVAL_NODE( func.value );

	switch ( n[0] ) {
	case SVAL_FUNCTION_CALL:
		EmitPostScriptFunctionCall( SVAL_U( n[1] ), argCount, bMethod, SVAL_U( n[2] ) );
		break;
	case SVAL_METHOD_CALL:
		EmitPostScriptThreadCall( SVAL_U( n[1] ), argCount, bMethod,
								  SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	}
}

/* ---- GetBuiltin  0x00469B20 ---- */
static unsigned short GetBuiltin( sval_u callNode ) {
	sval_t         *n = SVAL_NODE( callNode.value );
	sval_t         *callee;
	sval_t         *nameNode;

	if ( n[0] != SVAL_FUNCTION_CALL ) {
		return 0;
	}
	callee = SVAL_NODE( n[1] );
	if ( callee[0] != SVAL_SCRIPT_FUNCTION_NAME ) {
		return 0;
	}
	nameNode = SVAL_NODE( callee[1] );
	if ( nameNode[0] != SVAL_FUNCTION_REF ) {
		return 0;
	}

	/* the name is read twice, as Mac 1.3 GetBuiltin does: keeps it in a 16-bit register */
	if ( FindVariable( scrCompileGlob_currentFuncRoot, (unsigned short)nameNode[1] ) ) {
		return 0;
	}
	return (unsigned short)nameNode[1];
}

/* ---- EmitCall  0x00469B70 ---- */
/* the developer-command bracket is written out in place (Mac 1.3 EmitCall): developerOnly
   doubles as the "bracket opened" flag */
static void EmitCall( sval_u callNode, sval_u args, unsigned char bDropTop ) {
	unsigned short  builtinName;
	const char     *builtinText;
	void           *function;
	int             developerOnly;
	int             savedChecksum;
	unsigned int    callSourcePos;
	void           *tempMark;
	int             argCount;

	builtinName = GetBuiltin( callNode );
	if ( builtinName ) {
		builtinText   = SL_ConvertToString( builtinName );
		developerOnly = 0;
		function      = Scr_GetFunction( &builtinText, &developerOnly );

		callSourcePos = SVAL_NODE( callNode.value )[2];
		savedChecksum = scrCompilePub_checksum;
		tempMark      = TempMalloc( 0 );
		if ( developerOnly ) {
			if ( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
				developerOnly = 0;
			} else {
				if ( !bDropTop ) {
					CompileError( callSourcePos, "developer command can only be used as a statement "
												 "if not in a /# ... #/ comment" );
				}
				if ( !scrCompilePub_developerScript ) {
					scrCompileGlob_codegenMode = SCR_CODEGEN_DISCARD;
				} else {
					if ( !scrCompileGlob_bDeferredCheck ) {
						EmitOpcode( OP_DevBlockBegin, 0, LOCALDEPTH_NONE );
						scrCompileGlob_codeRelocStart = scrCompileGlob_codePos;
					}
					scrCompileGlob_codegenMode = SCR_CODEGEN_RELOCATE;
				}
			}
		}

		argCount = EmitExpressionList( args );
		if ( argCount >= SCR_BUILTIN_PARAM_LIMIT ) {
			CompileRemoveRefToString( builtinName );
			CompileError( callSourcePos, "parameter count exceeds 256" );
		}
		if ( !function ) {
			CompileError( callSourcePos, "unknown (builtin) function '%s'", builtinText );
			CompileRemoveRefToString( builtinName );
		} else {
			CompileRemoveRefToString( builtinName );
			EmitOpcode( OP_CallBuiltin, 1 - argCount, LOCALDEPTH_BUILTIN );
			EmitByte( (unsigned char)argCount );
			EmitBuiltinFunction( function );
			AddOpcodePos( callSourcePos );
		}

		AddExpressionListOpcodePos( SVAL_LIST( args.value ) );
		if ( bDropTop ) {
			EmitDecTop();
		}
		if ( developerOnly ) {
			scrCompileGlob_codegenMode = SCR_CODEGEN_INTERN;
			if ( scrCompilePub_developerScript ) {
				scrCompileGlob_bDeferredCheck = 1;
			} else {
				TempMemorySetPos( tempMark );
			}
			scrCompilePub_checksum = savedChecksum;
		}
	} else {
		EmitPreFunctionCall( callNode );
		argCount = EmitExpressionList( args );
		EmitPostFunctionCall( callNode, argCount, 0 );
		AddExpressionListOpcodePos( SVAL_LIST( args.value ) );
		if ( bDropTop ) {
			EmitDecTop();
		}
	}
}

/* ---- EmitMethod  0x00469E10 ---- */
static void EmitMethod( sval_u objectNode, sval_u callNode, sval_u args,
						sval_u methodSourcePos, unsigned char bDropTop ) {
	unsigned short  builtinName;
	const char     *builtinText;
	void           *method;
	int             developerOnly;
	int             savedChecksum;
	unsigned int    callSourcePos;
	void           *tempMark;
	int             argCount;

	builtinName = GetBuiltin( callNode );
	if ( builtinName ) {
		builtinText   = SL_ConvertToString( builtinName );
		developerOnly = 0;
		method        = Scr_GetMethod( &builtinText, &developerOnly );

		callSourcePos = SVAL_NODE( callNode.value )[2];
		savedChecksum = scrCompilePub_checksum;
		tempMark      = TempMalloc( 0 );
		if ( developerOnly ) {
			if ( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
				developerOnly = 0;
			} else {
				if ( !bDropTop ) {
					CompileError( callSourcePos, "developer command can only be used as a statement "
												 "if not in a /# ... #/ comment" );
				}
				if ( !scrCompilePub_developerScript ) {
					scrCompileGlob_codegenMode = SCR_CODEGEN_DISCARD;
				} else {
					if ( !scrCompileGlob_bDeferredCheck ) {
						EmitOpcode( OP_DevBlockBegin, 0, LOCALDEPTH_NONE );
						scrCompileGlob_codeRelocStart = scrCompileGlob_codePos;
					}
					scrCompileGlob_codegenMode = SCR_CODEGEN_RELOCATE;
				}
			}
		}

		argCount = EmitExpressionList( args );
		EmitPrimitiveExpression( objectNode.value );
		if ( argCount >= SCR_BUILTIN_PARAM_LIMIT ) {
			CompileRemoveRefToString( builtinName );
			CompileError( callSourcePos, "parameter count exceeds 256" );
		}
		if ( !method ) {
			CompileError( callSourcePos, "unknown (builtin) method '%s'", builtinText );
			CompileRemoveRefToString( builtinName );
		} else {
			CompileRemoveRefToString( builtinName );
			EmitOpcode( OP_CallBuiltinMethod, -argCount, LOCALDEPTH_BUILTIN );
			EmitByte( (unsigned char)argCount );
			EmitBuiltinMethod( method );
			AddOpcodePos( callSourcePos );
		}

		AddOpcodePos( methodSourcePos.sourcePosValue );
		AddExpressionListOpcodePos( SVAL_LIST( args.value ) );
		if ( bDropTop ) {
			EmitDecTop();
		}
		if ( developerOnly ) {
			scrCompileGlob_codegenMode = SCR_CODEGEN_INTERN;
			if ( scrCompilePub_developerScript ) {
				scrCompileGlob_bDeferredCheck = 1;
			} else {
				TempMemorySetPos( tempMark );
			}
			scrCompilePub_checksum = savedChecksum;
		}
	} else {
		EmitPreFunctionCall( callNode );
		argCount = EmitExpressionList( args );
		EmitPrimitiveExpression( objectNode.value );
		EmitPostFunctionCall( callNode, argCount, 1 );
		AddOpcodePos( methodSourcePos.sourcePosValue );
		AddExpressionListOpcodePos( SVAL_LIST( args.value ) );
		if ( bDropTop ) {
			EmitDecTop();
		}
	}
}

/* ---- AdjustFunctionAddresses  0x0046A0E0 ---- */
static void AdjustFunctionAddresses( void ) {
	unsigned short      functionSlot;
	unsigned short      functionObject;
	unsigned short      definitionSlot;
	unsigned short      referenceCountSlot;
	unsigned short      referenceSlot;
	VariableValue       definitionValue;
	VariableValue       referenceCountValue;
	VariableValue      *referenceValue;
	int                 index;

	/* Retail (0x46A10B-0x46A116) clears definitionValue ONCE, before the loop.
	   A function with no definition slot keeps the previous function's value/type,
	   so the /# #/ reference test below can see a stale VAR_INTEGER. Do not move
	   this back into the loop body. */
	definitionValue.type = VAR_UNDEFINED;
	definitionValue.u.intValue = 0;

	for ( functionSlot = FindNextSibling( scrCompileGlob_currentFuncRoot );
		  functionSlot;
		  functionSlot = FindNextSibling( functionSlot ) ) {

		functionObject = FindObject( functionSlot );

		definitionSlot = FindVariable( functionObject, SCR_FUNC_DEFINITION );
		if ( definitionSlot ) {
			GetVariableValue( definitionSlot, &definitionValue );
		}

		referenceCountSlot = FindVariable( functionObject, SCR_FUNC_REFCOUNT );
		if ( referenceCountSlot ) {
			GetVariableValue( referenceCountSlot, &referenceCountValue );

			for ( index = 0; index < referenceCountValue.u.intValue; index++ ) {
				referenceSlot  = FindVariable( functionObject,
											   (unsigned int)( index + SCR_FUNC_FIRST_REF ) );
				referenceValue = GetVariableValueAddress( referenceSlot );

				if ( definitionValue.type == VAR_INTEGER
				  && GetVarType( referenceSlot ) == VAR_CODEPOS ) {
					CompileError2( referenceValue->u.codePosValue,
								   "normal script cannot reference /# ... #/ comment" );
				} else if ( definitionSlot ) {
					*(unsigned int *)referenceValue->u.intValue =
						(unsigned int)definitionValue.u.intValue;
				} else {
					CompileError2( referenceValue->u.codePosValue, "unknown function" );
				}
			}
		}

		if ( definitionSlot ) {
			SetVariableValue( functionSlot, &definitionValue );
		}
	}
}

/* ---- SpecifyThreadPosition  0x0046A290 ---- */
static void SpecifyThreadPosition( unsigned short functionObject,
								   unsigned int sourcePos ) {
	unsigned short  definitionSlot;
	VariableValue   definitionValue;

	definitionSlot = GetVariable( functionObject, SCR_FUNC_DEFINITION );
	GetVariableValue( definitionSlot, &definitionValue );

	if ( definitionValue.type != VAR_UNDEFINED ) {
		CompileError( sourcePos, "function already defined" );
		return;
	}

	definitionValue.type       = VAR_CODEPOS;
	definitionValue.u.intValue = 0;
	SetNewVariableValue( definitionSlot, &definitionValue );
}

/* ---- SpecifyDeveloperThreadPosition  0x0046A2F0 ---- */
static void SpecifyDeveloperThreadPosition( unsigned short functionObject,
											unsigned int sourcePos ) {
	unsigned short  definitionSlot;
	VariableValue   definitionValue;

	if ( !scrCompilePub_developerScript ) {
		return;
	}

	definitionSlot = GetVariable( functionObject, SCR_FUNC_DEFINITION );
	GetVariableValue( definitionSlot, &definitionValue );

	if ( definitionValue.type != VAR_UNDEFINED ) {
		CompileError( sourcePos, "function already defined" );
		return;
	}

	definitionValue.type       = VAR_INTEGER;
	definitionValue.u.intValue = 0;
	SetNewVariableValue( definitionSlot, &definitionValue );
}

/* ---- SetThreadPosition  0x0046A360 ---- */
static void SetThreadPosition( unsigned short functionObject ) {
	void *position;
	unsigned short definitionSlot;
	position = TempMalloc( 0 );
	definitionSlot = FindVariable( functionObject, SCR_FUNC_DEFINITION );
	GetVariableValueAddress( definitionSlot )->u.pointerValue = position;
}

/* ---- SetDeveloperThreadPosition  0x0046A3B0 ---- */
static void SetDeveloperThreadPosition( unsigned short functionObject, sval_u sourcePos ) {
	int position;
	unsigned short definitionSlot;
	position = (int)TempMalloc( 0 ) + ( (int)scrCompilePub_developerBufPos - scrCompileGlob_codeRelocStart );
	definitionSlot = FindVariable( functionObject, SCR_FUNC_DEFINITION );
	GetVariableValueAddress( definitionSlot )->u.intValue = position;
}

/* ---- EmitCallExpression  0x0046A410 ---- */
static void EmitCallExpression( sval_u node, unsigned char bDropTop ) {
	sval_t *n = SVAL_NODE( node.value );

	switch ( n[0] ) {
	case SVAL_FUNCTION_CALL_VALUE:
		EmitCall( SVAL_U( n[1] ), SVAL_U( n[2] ), bDropTop );
		break;
	case SVAL_METHOD_CALL_VALUE:
		EmitMethod( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ), bDropTop );
		break;
	}
}

/* ---- EmitCallExpressionRef  0x0046A450 ---- */
static void EmitCallExpressionRef( sval_u node ) {
	sval_t *n = SVAL_NODE( node.value );

	switch ( n[0] ) {
	case SVAL_FUNCTION_CALL_VALUE:
		EmitCallRef( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	case SVAL_METHOD_CALL_VALUE:
		EmitMethodRef( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	}
}

/* ---- EmitCallExpressionFieldObject  0x0046A4B0 ---- */
static void EmitCallExpressionFieldObject( sval_u node ) {
	sval_t *n = SVAL_NODE( node.value );

	switch ( n[0] ) {
	case SVAL_FUNCTION_CALL_VALUE:
		EmitCall( SVAL_U( n[1] ), SVAL_U( n[2] ), qfalse );
		EmitCastFieldObject( n[3] );
		break;
	case SVAL_METHOD_CALL_VALUE:
		EmitMethod( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ), qfalse );
		EmitCastFieldObject( n[5] );
		break;
	}
}

/* ---- EmitPrimitiveExpressionList  0x0046A520 ---- */
static void EmitPrimitiveExpressionList( sval_u list, sval_u sourcePos ) {
	int     count;

	count = EmitExpressionList( list );
	switch ( count ) {
	case 1:
		return;
	case 3:
		EmitOpcode( OP_Vector, -2, LOCALDEPTH_NONE );
		AddExpressionListOpcodePos( SVAL_LIST( list.value ) );
		return;
	default:
		CompileError( sourcePos.sourcePosValue,
					  "expression list must have 1 or 3 parameters" );
		return;
	}
}

/* ---- EmitArrayExpressionListRef  0x0046A580 ---- */
static void EmitArrayExpressionListRef( sval_list_t *list, unsigned char bCreate,
										unsigned int sourcePos ) {
	sval_cell_t    *item;
	sval_entry_t   *entry;

	item = list->first;
	if ( item && item->next ) item = NULL;
	if ( item ) {
		entry = SVAL_ENTRY( item->value );
		EmitArrayExpressionRef( SVAL_U( entry->node ), bCreate, SVAL_U( entry->sourcePos ) );
		return;
	}

	if ( EmitExpressionList( SVAL_U( list ) ) == 3 ) {
		EmitOpcode( OP_Vector, -2, LOCALDEPTH_NONE );
		AddExpressionListOpcodePos( list );
		EmitOpcode( OP_DiscardRef, -1, LOCALDEPTH_NONE );
	} else {
		CompileError( sourcePos, "not an array, string, or vector" );
	}
}

/* ---- EmitExpressionListFieldObject  0x0046A620 ---- */
static void EmitExpressionListFieldObject( sval_u list, sval_u sourcePos ) {
	sval_cell_t    *item;
	sval_entry_t   *entry;

	item = SVAL_LIST( list.value )->first;
	if ( item && item->next ) item = NULL;
	if ( item ) {
		entry = SVAL_ENTRY( item->value );
		EmitExpressionFieldObject( SVAL_U( entry->node ), SVAL_U( entry->sourcePos ) );
		return;
	}

	CompileError( sourcePos.sourcePosValue, "not an object" );
	EmitExpressionList( list );
}

/* ---- EmitPrimitiveExpression  0x0046A680 ---- */
static void EmitPrimitiveExpression( sval_t node ) {
	sval_t *n = SVAL_NODE( node );
	float   f;

	switch ( n[0] ) {
	case SVAL_EXPRESSION_LIST:
		EmitPrimitiveExpressionList( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_INTEGER_LITERAL:
		EmitGetInteger( (int)n[1] );
		break;
	case SVAL_FLOAT_LITERAL:
		EmitGetFloat( *(float *)&n[1] );
		break;
	case SVAL_NEG_INTEGER_LITERAL:
		EmitGetInteger( -(int)n[1] );
		break;
	case SVAL_NEG_FLOAT_LITERAL:
		f = -*(float *)&n[1];
		EmitGetFloat( f );
		break;
	case SVAL_STRING:
		EmitGetString( (unsigned short)n[1] );
		break;
	case SVAL_ISTRING:
		EmitGetIString( (unsigned short)n[1] );
		break;
	case SVAL_CALL_VALUE:
		EmitCallExpression( SVAL_U( n[1] ), qfalse );
		break;
	case SVAL_REFERENCE_EXPRESSION:
		EmitVariableExpression( n[1] );
		break;
	case SVAL_UNDEFINED:
		EmitIncTop();
		break;
	case SVAL_SELF:
		EmitSelf();
		break;
	case SVAL_LEVEL:
		EmitLevel();
		break;
	case SVAL_GAME:
		EmitGame();
		break;
	case SVAL_ANIM:
		EmitAnim();
		break;
	case SVAL_SIZE:
		EmitSize( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_SCRIPT_FUNCTION_NAME:
		EmitGetFunction( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_EMPTY_ARRAY:
		EmitEmptyArray();
		break;
	case SVAL_ANIM_STRING:
		EmitAnimation( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_LITERAL_ZERO:
		EmitFalse();
		break;
	case SVAL_LITERAL_ONE:
		EmitTrue();
		break;
	case SVAL_ANIMTREE:
		EmitAnimTree( n[1] );
		break;
	default:
		break;
	}
}

/* ---- EmitBoolOrExpression  0x0046A830 ---- VERIFIED */
static void EmitBoolOrExpression( sval_u leftNode, sval_u leftSourcePos,
								  sval_u rightNode, sval_u rightSourcePos ) {
	char   *patch;
	char   *end;

	EmitExpression( leftNode.value );
	EmitOpcode( OP_JumpOnTrueExpr, -1, LOCALDEPTH_NONE );
	AddOpcodePos( leftSourcePos.sourcePosValue );
	EmitInteger( 0 );
	patch = (char *)scrCompileGlob_codePos;

	EmitExpression( rightNode.value );
	EmitCastBool( rightSourcePos.sourcePosValue );

	end = (char *)TempMalloc( 0 );
	*(int *)patch = (int)( end - patch ) - 4;
}

/* ---- EmitBoolAndExpression  0x0046A8D0 ---- VERIFIED */
static void EmitBoolAndExpression( sval_u leftNode, sval_u leftSourcePos,
								   sval_u rightNode, sval_u rightSourcePos ) {
	char   *patch;
	char   *end;

	EmitExpression( leftNode.value );
	EmitOpcode( OP_JumpOnFalseExpr, -1, LOCALDEPTH_NONE );
	AddOpcodePos( leftSourcePos.sourcePosValue );
	EmitInteger( 0 );
	patch = (char *)scrCompileGlob_codePos;

	EmitExpression( rightNode.value );
	EmitCastBool( rightSourcePos.sourcePosValue );

	end = (char *)TempMalloc( 0 );
	*(int *)patch = (int)( end - patch ) - 4;
}

/* ---- EmitBinaryOperatorExpression  0x0046A970 ---- */
static void EmitBinaryOperatorExpression( sval_u leftNode, sval_u rightNode,
										  sval_u opcode, sval_u sourcePos ) {
	EmitExpression( leftNode.value );
	EmitExpression( rightNode.value );
	EmitOpcode( (unsigned char)opcode.type, -1, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
}

/* ---- EmitBinaryEqualsOperatorExpression  0x0046A9A0 ---- */
static void EmitBinaryEqualsOperatorExpression( sval_u refNode, sval_u valueNode,
												sval_u opcode, sval_u sourcePos ) {
	scrCompileGlob_bOwnsStrings = 1;
	EmitVariableExpression( refNode.value );
	scrCompileGlob_bOwnsStrings = 0;

	EmitExpression( valueNode.value );
	EmitOpcode( (unsigned char)opcode.type, -1, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );

	EmitVariableExpressionRef( refNode.value, qtrue );
	EmitSetVariableField( sourcePos.sourcePosValue );
}

/* ---- EmitExpression  0x0046AA00 ---- */
static void EmitExpression( sval_t node ) {
	sval_t *n = SVAL_NODE( node );

	switch ( n[0] ) {
	case SVAL_PRIMITIVE_EXPRESSION:
		EmitPrimitiveExpression( n[1] );
		break;
	case SVAL_BOOL_OR:
		EmitBoolOrExpression( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_BOOL_AND:
		EmitBoolAndExpression( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_BINARY_OPERATOR:
		EmitBinaryOperatorExpression( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_CAST_BOOL:
		EmitExpression( n[1] );
		EmitCastBool( n[2] );
		break;
	case SVAL_CAST_INT:
		EmitExpression( n[1] );
		EmitCastInt( n[2] );
		break;
	case SVAL_CAST_FLOAT:
		EmitExpression( n[1] );
		EmitCastFloat( n[2] );
		break;
	case SVAL_CAST_STRING:
		EmitExpression( n[1] );
		EmitCastString( n[2] );
		break;
	case SVAL_BOOL_NOT:
		EmitExpression( n[1] );
		EmitBoolNot( n[2] );
		break;
	case SVAL_BOOL_COMPLEMENT:
		EmitExpression( n[1] );
		EmitBoolComplement( n[2] );
		break;
	default:
		break;
	}
}

/* ---- EmitVariableExpressionRef  0x0046ABB0 ---- */
static void EmitVariableExpressionRef( sval_t node, unsigned char bCreate ) {
	sval_t *n = SVAL_NODE( node );

	switch ( n[0] ) {
	case SVAL_STRING_REF:
		EmitLocalVariableRef( SVAL_U( n[1] ) );
		break;
	case SVAL_ARRAY_REF:
		EmitArrayVariableRef( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ),
							  bCreate );
		break;
	case SVAL_FIELD_REF:
		EmitFieldVariableRef( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	default:
		break;
	}
}

/* ---- EmitArrayPrimitiveExpressionRef  0x0046AC20 ---- */
static void EmitArrayPrimitiveExpressionRef( sval_u node, unsigned char bCreate,
											 sval_u sourcePos ) {
	sval_t *n = SVAL_NODE( node.value );

	switch ( n[0] ) {
	case SVAL_EXPRESSION_LIST:
		EmitArrayExpressionListRef( SVAL_LIST( n[1] ), bCreate, sourcePos.sourcePosValue );
		break;
	case SVAL_CALL_VALUE:
		EmitCallExpressionRef( SVAL_U( n[1] ) );
		break;
	case SVAL_REFERENCE_EXPRESSION:
		EmitVariableExpressionRef( n[1], bCreate );
		break;
	case SVAL_GAME:
		EmitGameRef();
		break;
	default:
		CompileError( sourcePos.sourcePosValue, "not an array, string, or vector" );
		EmitPrimitiveExpression( node.value );
		break;
	}
}

/* ---- EmitArrayExpressionRef  0x0046ACF0 ---- */
static void EmitArrayExpressionRef( sval_u node, unsigned char bCreate, sval_u sourcePos ) {
	sval_t *n = SVAL_NODE( node.value );

	if ( n[0] != SVAL_PRIMITIVE_EXPRESSION ) {
		CompileError( sourcePos.sourcePosValue, "not an array, string, or vector" );
		EmitExpression( node.value );
		return;
	}
	EmitArrayPrimitiveExpressionRef( SVAL_U( n[1] ), bCreate, SVAL_U( n[2] ) );
}

/* ---- EmitPrimitiveExpressionFieldObject  0x0046AD30 ---- */
static void EmitPrimitiveExpressionFieldObject( sval_t node, unsigned int sourcePos ) {
	sval_t *n = SVAL_NODE( node );

	switch ( n[0] ) {
	case SVAL_EXPRESSION_LIST:
		EmitExpressionListFieldObject( SVAL_U( n[1] ), SVAL_U( sourcePos ) );
		return;
	case SVAL_CALL_VALUE:
		EmitCallExpressionFieldObject( SVAL_U( n[1] ) );
		return;
	case SVAL_REFERENCE_EXPRESSION:
		EmitVariableExpression( n[1] );
		EmitCastFieldObject( n[2] );
		return;
	case SVAL_SELF:
		EmitSelfObject();
		return;
	case SVAL_LEVEL:
		EmitLevelObject();
		return;
	case SVAL_ANIM:
		EmitAnimObject();
		return;
	default:
		break;
	}

	CompileError( sourcePos, "not an object" );
	EmitPrimitiveExpression( node );
}

/* ---- EmitExpressionFieldObject  0x0046AE20 ---- */
static void EmitExpressionFieldObject( sval_u node, sval_u sourcePos ) {
	sval_t *n = SVAL_NODE( node.value );

	if ( n[0] != SVAL_PRIMITIVE_EXPRESSION ) {
		CompileError( sourcePos.sourcePosValue, "not an object" );
		EmitExpression( node.value );
		return;
	}

	EmitPrimitiveExpressionFieldObject( n[1], n[2] );
}

/* ---- ConnectBreakStatements  0x0046AE60 ---- */
__declspec(noinline) static void ConnectBreakStatements( void ) {
	char                   *here;
	scrCodeOffsetPatch_t   *patch;

	here = (char *)TempMalloc( 0 );
	for ( patch = (scrCodeOffsetPatch_t *)scrCompileGlob_breakList;
		  patch; patch = patch->next ) {
		*(int *)patch->codePos = (int)( here - patch->codePos );
	}
}

/* ---- ConnectContinueStatements  0x0046AEA0 ---- */
static void ConnectContinueStatements( void ) {
	char                   *here;
	scrCodeOffsetPatch_t   *patch;

	EmitNOP();
	here = (char *)TempMalloc( 0 );
	for ( patch = (scrCodeOffsetPatch_t *)scrCompileGlob_contList;
		  patch; patch = patch->next ) {
		*(int *)patch->codePos = (int)( here - patch->codePos );
	}
}

/* ---- IsUndefinedPrimitiveExpression  0x0046AEF0 ---- */
static unsigned char IsUndefinedPrimitiveExpression( sval_u node ) {
	return SVAL_NODE( node.value )[0] == SVAL_UNDEFINED ? qtrue : qfalse;
}

/* ---- IsUndefinedExpression  0x0046AF00 ---- */
static unsigned char IsUndefinedExpression( sval_u node ) {
	sval_t         *n = SVAL_NODE( node.value );
	unsigned char   result;

	if ( n[0] == SVAL_PRIMITIVE_EXPRESSION ) {
		/* a named result reproduces the original C++ bool return */
		result = IsUndefinedPrimitiveExpression( SVAL_U( n[1] ) );
		return result;
	}
	return 0;
}

/* ---- EmitClearVariableExpression  0x0046AF20 ---- */
static void EmitClearVariableExpression( sval_u node ) {
	sval_t *n = SVAL_NODE( node.value );

	switch ( n[0] ) {
	case SVAL_STRING_REF:
		EmitClearLocalVariable( SVAL_U( n[1] ) );
		break;
	case SVAL_ARRAY_REF:
		EmitClearArrayVariable( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_FIELD_REF:
		EmitClearFieldVariable( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	default:
		break;
	}
}

/* ---- EmitAssignmentStatement  0x0046AFA0 ---- */
static void EmitAssignmentStatement( sval_u refNode, sval_u valueNode,
									 sval_u sourcePos ) {
	if ( IsUndefinedExpression( valueNode ) ) {
		EmitClearVariableExpression( refNode );
		return;
	}

	EmitExpression( valueNode.value );
	EmitVariableExpressionRef( refNode.value, qtrue );
	EmitSetVariableField( sourcePos.sourcePosValue );
}

/* ---- EmitExpressionStatement  0x0046AFF0 ---- */
static void EmitExpressionStatement( sval_u node ) {
	EmitCallExpression( node, qtrue );
}

/* ---- EmitReturnStatement  0x0046B030 ---- */
static void EmitReturnStatement( sval_u node ) {
	EmitExpression( node.value );
	EmitOpcode( OP_Return, -1, LOCALDEPTH_NONE );
}

/* ---- EmitEndStatement  0x0046B050 ---- */
static void EmitEndStatement( void ) {
	EmitOpcode( OP_End, 0, LOCALDEPTH_NONE );
}

/* ---- EmitWaitStatement  0x0046B060 ---- */
static void EmitWaitStatement( sval_u timeNode, sval_u timeSourcePos,
							   sval_u opcodeSourcePos ) {
	if ( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( opcodeSourcePos.sourcePosValue,
					  "wait not allowed in /# ... #/ comment" );
	}

	EmitExpression( timeNode.value );
	EmitOpcode( OP_Wait, -1, LOCALDEPTH_NONE );
	AddOpcodePos( opcodeSourcePos.sourcePosValue );
	AddOpcodePos( timeSourcePos.sourcePosValue );
}

static char *EmitForwardJump( unsigned char opcode, int stackDelta,
							  unsigned int sourcePos ) {
	EmitOpcode( opcode, stackDelta, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos );
	EmitInteger( 0 );
	return (char *)scrCompileGlob_codePos;
}

static void PatchForwardJump( char *patch ) {
	char   *here = (char *)TempMalloc( 0 );

	*(int *)patch = (int)( here - patch ) - 4;
}

/* ---- EmitIfStatement  0x0046B0B0 ---- */
static void EmitIfStatement( sval_u conditionNode, sval_u bodyNode,
							 sval_u sourcePos ) {
	char   *falsePatch;

	EmitExpression( conditionNode.value );
	falsePatch = EmitForwardJump( OP_JumpOnFalse, -1, sourcePos.sourcePosValue );

	EmitStatement( bodyNode.value );
	EmitNOP();
	PatchForwardJump( falsePatch );
}

/* ---- EmitIfElseStatement  0x0046B140 ---- */
static void EmitIfElseStatement( sval_u conditionNode, sval_u thenNode,
								 sval_u elseNode, sval_u sourcePos ) {
	char   *falsePatch;
	char   *endPatch;
	char   *here;

	EmitExpression( conditionNode.value );
	falsePatch = EmitForwardJump( OP_JumpOnFalse, -1, sourcePos.sourcePosValue );

	EmitStatement( thenNode.value );
	EmitOpcode( OP_Jump, 0, LOCALDEPTH_NONE );
	EmitInteger( 0 );
	endPatch = (char *)scrCompileGlob_codePos;

	PatchForwardJump( falsePatch );

	EmitStatement( elseNode.value );
	EmitNOP();

	here = (char *)TempMalloc( 0 );
	*(int *)endPatch = (int)( here - endPatch );
}

static void Scr_ClearLoopState( void ) {
	scrCompileGlob_breakAllowed    = 0;
	scrCompileGlob_breakAllowedDev = 0;
	scrCompileGlob_contAllowed     = 0;
	scrCompileGlob_contAllowedDev  = 0;
}

static void Scr_EnableLoopState( void ) {
	scrCompileGlob_breakAllowed    = 1;
	scrCompileGlob_breakAllowedDev = scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN;
	scrCompileGlob_breakList       = 0;
	scrCompileGlob_contAllowed     = 1;
	scrCompileGlob_contAllowedDev  = scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN;
	scrCompileGlob_contList        = 0;
}

/* ---- EmitWhileStatement  0x0046B220 ---- */
static void EmitWhileStatement( sval_u conditionNode, sval_u bodyNode,
								sval_u conditionSourcePos,
								sval_u loopSourcePos ) {
	unsigned char           breakAllowed;
	unsigned char           breakAllowedDev;
	scrCodeOffsetPatch_t   *breakList;
	unsigned char           contAllowed;
	unsigned char           contAllowedDev;
	scrCodeOffsetPatch_t   *contList;
	char                   *loopStart;
	char                   *falsePatch;
	char                   *jumpPos;

	EmitNOP();
	breakAllowed    = scrCompileGlob_breakAllowed;
	breakAllowedDev = scrCompileGlob_breakAllowedDev;
	breakList       = (scrCodeOffsetPatch_t *)scrCompileGlob_breakList;
	scrCompileGlob_breakAllowed    = 0;
	scrCompileGlob_breakAllowedDev = 0;
	contAllowed     = scrCompileGlob_contAllowed;
	contAllowedDev  = scrCompileGlob_contAllowedDev;
	contList        = (scrCodeOffsetPatch_t *)scrCompileGlob_contList;
	scrCompileGlob_contAllowed     = 0;
	scrCompileGlob_contAllowedDev  = 0;

	loopStart = (char *)TempMalloc( 0 );
	EmitExpression( conditionNode.value );
	falsePatch = EmitForwardJump( OP_JumpOnFalse, -1, conditionSourcePos.sourcePosValue );

	Scr_EnableLoopState();
	EmitStatement( bodyNode.value );
	Scr_ClearLoopState();
	ConnectContinueStatements();

	EmitOpcode( OP_JumpBack, 0, LOCALDEPTH_NONE );
	AddOpcodePos( loopSourcePos.sourcePosValue );
	jumpPos = (char *)TempMalloc( 0 );
	EmitInteger( (int)( loopStart - jumpPos ) );

	PatchForwardJump( falsePatch );
	ConnectBreakStatements();
	scrCompileGlob_breakAllowed    = breakAllowed;
	scrCompileGlob_breakAllowedDev = breakAllowedDev;
	scrCompileGlob_breakList       = (int)breakList;
	scrCompileGlob_contAllowed     = contAllowed;
	scrCompileGlob_contAllowedDev  = contAllowedDev;
	scrCompileGlob_contList        = (int)contList;
}

/* ---- EmitDoWhileStatement  0x0046B3F0 ---- */
static void EmitDoWhileStatement( sval_u bodyNode, sval_u conditionNode,
								  sval_u conditionSourcePos,
								  sval_u loopSourcePos ) {
	unsigned char           breakAllowed;
	unsigned char           breakAllowedDev;
	scrCodeOffsetPatch_t   *breakList;
	unsigned char           contAllowed;
	unsigned char           contAllowedDev;
	scrCodeOffsetPatch_t   *contList;
	char                   *loopStart;
	char                   *jumpPos;

	EmitNOP();
	breakAllowed    = scrCompileGlob_breakAllowed;
	breakAllowedDev = scrCompileGlob_breakAllowedDev;
	breakList       = (scrCodeOffsetPatch_t *)scrCompileGlob_breakList;
	contAllowed     = scrCompileGlob_contAllowed;
	contAllowedDev  = scrCompileGlob_contAllowedDev;
	contList        = (scrCodeOffsetPatch_t *)scrCompileGlob_contList;
	Scr_EnableLoopState();

	loopStart = (char *)TempMalloc( 0 );
	EmitStatement( bodyNode.value );
	Scr_ClearLoopState();
	ConnectContinueStatements();

	EmitExpression( conditionNode.value );
	EmitOpcode( OP_JumpOnTrueBack, -1, LOCALDEPTH_NONE );
	AddOpcodePos( loopSourcePos.sourcePosValue );
	AddOpcodePos( conditionSourcePos.sourcePosValue );
	jumpPos = (char *)TempMalloc( 0 );
	EmitInteger( (int)( loopStart - jumpPos ) );

	ConnectBreakStatements();
	scrCompileGlob_breakAllowed    = breakAllowed;
	scrCompileGlob_breakAllowedDev = breakAllowedDev;
	scrCompileGlob_breakList       = (int)breakList;
	scrCompileGlob_contAllowed     = contAllowed;
	scrCompileGlob_contAllowedDev  = contAllowedDev;
	scrCompileGlob_contList        = (int)contList;
}

/* ---- EmitForStatement  0x0046B560 ---- */
static void EmitForStatement( sval_t initNode, sval_t conditionNode,
							  sval_t incrementNode, sval_t bodyNode,
							  unsigned int conditionSourcePos,
							  unsigned int loopSourcePos ) {
	unsigned char           breakAllowed;
	unsigned char           breakAllowedDev;
	scrCodeOffsetPatch_t   *breakList;
	unsigned char           contAllowed;
	unsigned char           contAllowedDev;
	scrCodeOffsetPatch_t   *contList;
	char                   *loopStart;
	char                   *falsePatch;
	char                   *jumpPos;

	breakAllowed    = scrCompileGlob_breakAllowed;
	breakAllowedDev = scrCompileGlob_breakAllowedDev;
	breakList       = (scrCodeOffsetPatch_t *)scrCompileGlob_breakList;
	scrCompileGlob_breakAllowed    = 0;
	scrCompileGlob_breakAllowedDev = 0;
	contAllowed     = scrCompileGlob_contAllowed;
	contAllowedDev  = scrCompileGlob_contAllowedDev;
	contList        = (scrCodeOffsetPatch_t *)scrCompileGlob_contList;
	scrCompileGlob_contAllowed     = 0;
	scrCompileGlob_contAllowedDev  = 0;

	EmitStatement( initNode );
	EmitNOP();
	loopStart = (char *)TempMalloc( 0 );

	if ( SVAL_NODE( conditionNode )[0] == SVAL_FOR_CONDITION ) {
		EmitExpression( SVAL_NODE( conditionNode )[1] );
		falsePatch = EmitForwardJump( OP_JumpOnFalse, -1, conditionSourcePos );
	} else {
		falsePatch = NULL;
	}

	Scr_EnableLoopState();
	EmitStatement( bodyNode );
	Scr_ClearLoopState();
	ConnectContinueStatements();
	EmitStatement( incrementNode );

	EmitOpcode( OP_JumpBack, 0, LOCALDEPTH_NONE );
	AddOpcodePos( loopSourcePos );
	jumpPos = (char *)TempMalloc( 0 );
	EmitInteger( (int)( loopStart - jumpPos ) );

	if ( falsePatch ) {
		PatchForwardJump( falsePatch );
	}

	ConnectBreakStatements();
	scrCompileGlob_breakAllowed    = breakAllowed;
	scrCompileGlob_breakAllowedDev = breakAllowedDev;
	scrCompileGlob_breakList       = (int)breakList;
	scrCompileGlob_contAllowed     = contAllowed;
	scrCompileGlob_contAllowedDev  = contAllowedDev;
	scrCompileGlob_contList        = (int)contList;
}

/* ---- EmitIncStatement  0x0046B760 ---- */
static void EmitIncStatement( sval_u refNode, sval_u sourcePos ) {
	EmitVariableExpressionRef( refNode.value, qfalse );
	EmitOpcode( OP_Inc, 1, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
	EmitSetVariableField( sourcePos.sourcePosValue );
}

/* ---- EmitDecStatement  0x0046B7A0 ---- */
static void EmitDecStatement( sval_u refNode, sval_u sourcePos ) {
	EmitVariableExpressionRef( refNode.value, qfalse );
	EmitOpcode( OP_Dec, 1, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
	EmitSetVariableField( sourcePos.sourcePosValue );
}

/* ---- EmitFormalParameterListRefInternal  0x0046B7E0 ---- */
static void EmitFormalParameterListRefInternal( sval_cell_t *item ) {
	for ( item = item->next; item; item = item->next ) {
		EmitSafeSetVariableField( SVAL_U( SVAL_ENTRY( item->value )->node ),
								  SVAL_U( SVAL_ENTRY( item->value )->sourcePos ) );
	}
}

/* ---- EmitFormalWaittillParameterListRefInternal  0x0046B8C0 ---- VERIFIED */
static void EmitFormalWaittillParameterListRefInternal( sval_cell_t *item ) {
	for ( item = item->next; item; item = item->next ) {
		EmitSafeSetWaittillVariableField(
			SVAL_U( SVAL_ENTRY( item->value )->node ),
			SVAL_U( SVAL_ENTRY( item->value )->sourcePos ) );
	}
}

/* ---- EmitWaittillStatement  0x0046B9A0 ---- VERIFIED */
static void EmitWaittillStatement( sval_u objectNode, sval_u listNode,
								   sval_u objectSourcePos,
								   sval_u opcodeSourcePos ) {
	sval_cell_t    *eventItem;

	if ( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( opcodeSourcePos.sourcePosValue,
					  "waittill not allowed in developer script" );
	}

	eventItem = SVAL_LIST( listNode.value )->first->next;
	EmitExpression( SVAL_ENTRY( eventItem->value )->node );
	EmitPrimitiveExpression( objectNode.value );
	EmitOpcode( OP_WaitTill, -2, LOCALDEPTH_NONE );
	AddOpcodePos( opcodeSourcePos.sourcePosValue );
	AddOpcodePos( SVAL_ENTRY( eventItem->value )->sourcePos );
	AddOpcodePos( objectSourcePos.sourcePosValue );
	EmitFormalWaittillParameterListRefInternal( eventItem );
	EmitOpcode( OP_EndWaittill, 0, LOCALDEPTH_NONE );
}

/* ---- EmitWaittillmatchStatement  0x0046BA20 ---- */
static void EmitWaittillmatchStatement( sval_u objectNode, sval_u listNode,
										sval_u objectSourcePos,
										sval_u opcodeSourcePos ) {
	sval_cell_t    *eventItem;
	sval_cell_t    *item;
	int             matchCount;

	if ( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( opcodeSourcePos.sourcePosValue, "waittillmatch not allowed in developer script" );
	}

	/* Mac 1.3 loop shapes: the cursors are re-read from the list and advanced before the test */
	matchCount = 0;
	item = SVAL_LIST( listNode.value )->first->next;
	for ( ;; ) {
		item = item->next;
		if ( !item ) {
			break;
		}
		EmitExpression( SVAL_ENTRY( item->value )->node );
		matchCount++;
	}

	eventItem = SVAL_LIST( listNode.value )->first->next;
	EmitExpression( SVAL_ENTRY( eventItem->value )->node );
	EmitPrimitiveExpression( objectNode.value );
	EmitOpcode( OP_WaitTillMatch, -2 - matchCount, LOCALDEPTH_NONE );
	AddOpcodePos( opcodeSourcePos.sourcePosValue );
	AddOpcodePos( SVAL_ENTRY( eventItem->value )->sourcePos );
	AddOpcodePos( objectSourcePos.sourcePosValue );

	for ( ;; ) {
		eventItem = eventItem->next;
		if ( !eventItem ) {
			break;
		}
		AddOpcodePos( SVAL_ENTRY( eventItem->value )->sourcePos );
	}

	EmitByte( (unsigned char)matchCount );
	EmitOpcode( OP_EndWaittill, 0, LOCALDEPTH_NONE );
}

/* ---- EmitNotifyStatement  0x0046BB00 ---- */
static void EmitNotifyStatement( sval_u objectNode, sval_u list,
								 sval_u objectSourcePos, sval_u opcodeSourcePos ) {
	sval_cell_t    *item;
	sval_cell_t    *lastItem;
	int             argCount;

	EmitOpcode( OP_VoidCodepos, 1, LOCALDEPTH_NONE );

	argCount = 0;
	lastItem = NULL;
	for ( item = SVAL_LIST( list.value )->first; item; item = item->next ) {
		lastItem = item;
		EmitExpression( SVAL_ENTRY( item->value )->node );
		argCount++;
	}

	EmitPrimitiveExpression( objectNode.value );
	EmitOpcode( OP_Notify, -2 - argCount, LOCALDEPTH_NONE );
	AddOpcodePos( opcodeSourcePos.sourcePosValue );
	AddOpcodePos( SVAL_ENTRY( lastItem->value )->sourcePos );
	AddOpcodePos( objectSourcePos.sourcePosValue );
}

/* ---- EmitEndOnStatement  0x0046BC20 ---- */
static void EmitEndOnStatement( sval_u objectNode, sval_u eventNode,
								sval_u objectSourcePos, sval_u opcodeSourcePos ) {
	EmitExpression( eventNode.value );
	EmitPrimitiveExpression( objectNode.value );
	EmitOpcode( OP_EndOn, -2, LOCALDEPTH_NONE );
	AddOpcodePos( opcodeSourcePos.sourcePosValue );
	AddOpcodePos( objectSourcePos.sourcePosValue );
}

/* ---- CompareCaseInfo  0x0046BC60 ---- */
static int __cdecl CompareCaseInfo( const void *a, const void *b ) {
	const scrCaseTableEntry_t  *left  = (const scrCaseTableEntry_t *)a;
	const scrCaseTableEntry_t  *right = (const scrCaseTableEntry_t *)b;

	if ( left->value > right->value ) {
		return -1;
	}
	return left->value < right->value;
}

/* ---- EmitCaseStatementInfo  0x0046BF50 ---- */
static void EmitCaseStatementInfo( unsigned int value, unsigned int sourcePos ) {
	scrCaseRecord_t    *record;

	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_DISCARD ) {
		return;
	}

	record = (scrCaseRecord_t *)
		Hunk_AllocateTempMemoryHighInternal( (int)sizeof( scrCaseRecord_t ) );
	record->value   = value;
	record->codePos = (unsigned int)TempMalloc( 0 );
	if ( scrCompileGlob_codegenMode == SCR_CODEGEN_RELOCATE ) {
		record->codePos += ( (int)scrCompilePub_developerBufPos - scrCompileGlob_codeRelocStart );
	}
	record->sourcePos = sourcePos;
	record->next      = (scrCaseRecord_t *)scrCompileGlob_caseList;
	scrCompileGlob_caseList = (int)record;
}

/* ---- EmitCaseStatement  0x0046BFD0 ---- */
static void EmitCaseStatement( sval_u valueNode, sval_u sourcePos ) {
	sval_t         *n = SVAL_NODE( valueNode.value );
	unsigned int    caseValue;
	int             integerValue;
	unsigned short stringValue;

	switch ( n[0] ) {
	case SVAL_INTEGER_LITERAL:
		caseValue = (unsigned int)n[1];
		integerValue = (int)caseValue;
		if ( (unsigned int)( integerValue + SCR_CASE_TEST_BIAS ) > SCR_CASE_TEST_MAX ) {
			CompileError( sourcePos.sourcePosValue,
						  va( "case index %d out of range", integerValue ) );
			return;
		}
		caseValue = ( caseValue + SCR_CASE_ENCODE_BIAS )
				  & SCR_CASE_ENCODE_MASK;
		break;
	case SVAL_STRING:
		stringValue = (unsigned short)n[1];
		caseValue = stringValue;
		CompileTransferRefToString( stringValue, SCR_STRING_USAGE_CASE );
		break;
	default:
		CompileError( sourcePos.sourcePosValue, "case expression must be an int or string" );
		EmitPrimitiveExpression( valueNode.value );   /* retail 0x0046BFF1 */
		return;
	}

	/* retail 0x0046C062 / 0x0046C089: each error path returns without
	 * recording the case */
	if ( !scrCompileGlob_caseAllowed ) {
		CompileError( sourcePos.sourcePosValue, "illegal case statement" );
		return;
	}
	if ( !scrCompileGlob_caseAllowedDev
		 && scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( sourcePos.sourcePosValue,
					  "cannot use /# ... #/ comments directly around a case statement" );
		return;
	}

	EmitCaseStatementInfo( caseValue, sourcePos.sourcePosValue );
}

/* ---- EmitDefaultStatement  0x0046C0A0 ---- */
static void EmitDefaultStatement( sval_u sourcePos ) {
	if ( !scrCompileGlob_caseAllowed ) {
		CompileError( sourcePos.sourcePosValue, "illegal default statement" );
		return;     /* retail 0x0046C0BB */
	}
	EmitCaseStatementInfo( 0, sourcePos.sourcePosValue );
}

extern void         qsort_m( void *base, unsigned int count, unsigned int width,
							 int ( __cdecl *compare )( const void *, const void * ) );

/* ---- EmitSwitchStatement  0x0046BC80 ---- */
static void EmitSwitchStatement( sval_u valueNode, sval_u bodyNode, sval_u sourcePos ) {
	unsigned char           caseAllowed;
	unsigned char           caseAllowedDev;
	scrCaseRecord_t        *caseList;
	unsigned char           breakAllowed;
	unsigned char           breakAllowedDev;
	scrCodeOffsetPatch_t   *breakList;
	char                   *tablePatch;
	scrCaseTableEntry_t    *table;
	scrCaseRecord_t        *record;
	int                     caseCount;
	int                     index;

	caseAllowed    = scrCompileGlob_caseAllowed;
	caseAllowedDev = scrCompileGlob_caseAllowedDev;
	caseList       = (scrCaseRecord_t *)scrCompileGlob_caseList;
	scrCompileGlob_caseAllowed     = 0;
	scrCompileGlob_caseAllowedDev  = 0;
	breakAllowed    = scrCompileGlob_breakAllowed;
	breakAllowedDev = scrCompileGlob_breakAllowedDev;
	breakList       = (scrCodeOffsetPatch_t *)scrCompileGlob_breakList;
	scrCompileGlob_breakAllowed    = 0;
	scrCompileGlob_breakAllowedDev = 0;

	EmitExpression( valueNode.value );
	EmitOpcode( OP_Switch, -1, LOCALDEPTH_NONE );
	EmitInteger( 0 );
	tablePatch = (char *)scrCompileGlob_codePos;

	scrCompileGlob_caseAllowed     = 1;
	scrCompileGlob_caseAllowedDev  =
		(unsigned char)( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN );
	scrCompileGlob_caseList        = 0;
	scrCompileGlob_breakAllowed    = 1;
	scrCompileGlob_breakAllowedDev = scrCompileGlob_caseAllowedDev;
	scrCompileGlob_breakList       = 0;

	EmitStatement( bodyNode.value );

	scrCompileGlob_caseAllowed     = 0;
	scrCompileGlob_caseAllowedDev  = 0;
	scrCompileGlob_breakAllowed    = 0;
	scrCompileGlob_breakAllowedDev = 0;

	EmitOpcode( OP_EndSwitch, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
	EmitShort( 0 );

	table = (scrCaseTableEntry_t *)TempMalloc( 0 );
	*(int *)tablePatch = (int)( (char *)table - tablePatch );

	caseCount = 0;
	for ( record = (scrCaseRecord_t *)scrCompileGlob_caseList;
		  record; record = record->next ) {
		EmitInteger( (int)record->value );
		EmitInteger( (int)record->codePos );
		caseCount++;
	}

	*(unsigned short *)( (char *)table - 2 ) = (unsigned short)caseCount;
	qsort_m( table, (unsigned int)caseCount, sizeof( scrCaseTableEntry_t ), CompareCaseInfo );

	for ( index = 1; index < caseCount; index++, table++ ) {
		if ( table[0].value != table[1].value ) {
			continue;
		}
		for ( record = (scrCaseRecord_t *)scrCompileGlob_caseList;
			  record; record = record->next ) {
			if ( record->value == table[0].value ) {
				CompileError( record->sourcePos, "duplicate case expression" );
			}
		}
	}

	ConnectBreakStatements();

	scrCompileGlob_caseAllowed     = caseAllowed;
	scrCompileGlob_caseAllowedDev  = caseAllowedDev;
	scrCompileGlob_caseList        = (int)caseList;
	scrCompileGlob_breakAllowed    = breakAllowed;
	scrCompileGlob_breakAllowedDev = breakAllowedDev;
	scrCompileGlob_breakList       = (int)breakList;
}

/* ---- EmitBreakStatement  0x0046C0D0 ---- */
static void EmitBreakStatement( sval_u sourcePos ) {
	scrCodeOffsetPatch_t   *patch;

	/* retail 0x0046C0EB / 0x0046C110: each error path returns without
	 * emitting the jump */
	if ( !scrCompileGlob_breakAllowed ) {
		CompileError( sourcePos.sourcePosValue, "illegal break statement" );
		return;
	}
	if ( !scrCompileGlob_breakAllowedDev
		 && scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( sourcePos.sourcePosValue,
					  "cannot use /# ... #/ comments directly around a break statement" );
		return;
	}

	EmitOpcode( OP_Jump, 0, LOCALDEPTH_NONE );
	EmitInteger( 0 );

	patch = (scrCodeOffsetPatch_t *)
		Hunk_AllocateTempMemoryHighInternal( (int)sizeof( scrCodeOffsetPatch_t ) );
	patch->codePos = (char *)scrCompileGlob_codePos;
	patch->next    = (scrCodeOffsetPatch_t *)scrCompileGlob_breakList;
	scrCompileGlob_breakList = (int)patch;
}

/* ---- EmitContinueStatement  0x0046C170 ---- */
static void EmitContinueStatement( sval_u sourcePos ) {
	scrCodeOffsetPatch_t   *patch;

	/* retail 0x0046C18B / 0x0046C1B0: each error path returns without
	 * emitting the jump */
	if ( !scrCompileGlob_contAllowed ) {
		CompileError( sourcePos.sourcePosValue, "illegal continue statement" );
		return;
	}
	if ( !scrCompileGlob_contAllowedDev
		 && scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( sourcePos.sourcePosValue,
					  "cannot use /# ... #/ comments directly around a continue statement" );
		return;
	}

	EmitOpcode( OP_Jump, 0, LOCALDEPTH_NONE );
	EmitInteger( 0 );

	patch = (scrCodeOffsetPatch_t *)
		Hunk_AllocateTempMemoryHighInternal( (int)sizeof( scrCodeOffsetPatch_t ) );
	patch->codePos = (char *)scrCompileGlob_codePos;
	patch->next    = (scrCodeOffsetPatch_t *)scrCompileGlob_contList;
	scrCompileGlob_contList = (int)patch;
}

/* ---- EmitStatement  0x0046C210 ---- */
static void EmitStatement( sval_t node ) {
	sval_t *n = SVAL_NODE( node );

	switch ( n[0] ) {
	case SVAL_ASSIGNMENT_STATEMENT:
		EmitAssignmentStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	case SVAL_CALL_STATEMENT:
		EmitExpressionStatement( SVAL_U( n[1] ) );
		break;
	case SVAL_RETURN_VALUE_STATEMENT:
		EmitReturnStatement( SVAL_U( n[1] ) );
		break;
	case SVAL_RETURN_STATEMENT:
		EmitEndStatement();
		break;
	case SVAL_WAIT_STATEMENT:
		EmitWaitStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	case SVAL_IF_STATEMENT:
		EmitIfStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	case SVAL_IF_ELSE_STATEMENT:
		EmitIfElseStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_WHILE_STATEMENT:
		EmitWhileStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_DO_WHILE_STATEMENT:
		EmitDoWhileStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_FOR_STATEMENT:
		EmitForStatement( n[1], n[2], n[3], n[4], n[5], n[6] );
		break;
	case SVAL_INC_STATEMENT:
		EmitIncStatement( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_DEC_STATEMENT:
		EmitDecStatement( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_REF_ASSIGNMENT_STATEMENT:
		EmitBinaryEqualsOperatorExpression( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ),
												SVAL_U( n[4] ) );
		break;
	case SVAL_STATEMENT_BLOCK:
		EmitStatementList( SVAL_U( n[1] ) );
		break;
	case SVAL_DEVELOPER_BLOCK:
		EmitDeveloperStatementList( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_WAITTILL_STATEMENT:
		EmitWaittillStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_WAITTILLMATCH_STATEMENT:
		EmitWaittillmatchStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_NOTIFY_STATEMENT:
		EmitNotifyStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_ENDON_STATEMENT:
		EmitEndOnStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ), SVAL_U( n[4] ) );
		break;
	case SVAL_SWITCH_STATEMENT:
		EmitSwitchStatement( SVAL_U( n[1] ), SVAL_U( n[2] ), SVAL_U( n[3] ) );
		break;
	case SVAL_CASE_STATEMENT:
		EmitCaseStatement( SVAL_U( n[1] ), SVAL_U( n[2] ) );
		break;
	case SVAL_DEFAULT_STATEMENT:
		EmitDefaultStatement( SVAL_U( n[1] ) );
		break;
	case SVAL_BREAK_STATEMENT:
		EmitBreakStatement( SVAL_U( n[1] ) );
		break;
	case SVAL_CONTINUE_STATEMENT:
		EmitContinueStatement( SVAL_U( n[1] ) );
		break;
	default:
		break;
	}
}

/* ---- EmitStatementList  0x0046C4E0 ---- */
static void EmitStatementList( sval_u block ) {
	sval_cell_t    *item;

	for ( item = SVAL_BLOCKLIST( block.value )->head; item; item = item->next ) {
		EmitStatement( item->value );
	}
}

/* ---- Scr_TransferToDeveloperBuffer  0x0046C510 ---- */
static void Scr_TransferToDeveloperBuffer( void ) {
	char   *tempEnd;
	int     byteCount;
	int     usedBytes;

	tempEnd   = (char *)TempMalloc( 0 );
	byteCount = (int)( tempEnd - (char *)scrCompileGlob_codeRelocStart );
	usedBytes = scrCompilePub_developerBufPos - scrCompileGlob_devOpBuffer;

	if ( usedBytes + byteCount > SCR_DEV_OP_BUF_SIZE ) {
		Com_Error( ERR_DROP,
				   "max developer script size exceeded - increase DEV_OP_BUF_SIZE" );
	}

	Com_Memcpy( (void *)scrCompilePub_developerBufPos,
				(const void *)scrCompileGlob_codeRelocStart, (size_t)byteCount );
	scrCompilePub_developerBufPos += byteCount;
}

/* ---- Scr_TransferStatementListToDeveloperBuffer  0x0046C580 ---- */
static void Scr_TransferStatementListToDeveloperBuffer( void ) {
	int         savedChecksum;
	int         codeOffset;
	int         oldCapacity;
	int         newCapacity;
	char      **newTable;

	savedChecksum = scrCompilePub_checksum;
	EmitOpcode( OP_DevBlockDeferred, 0, LOCALDEPTH_NONE );
	codeOffset = scrCompileGlob_codeRelocStart - scrVarPub_programBuffer;
	scrCompilePub_checksum = savedChecksum;

	if ( codeOffset >= scrCompileGlob_devPatchMax ) {
		newCapacity = scrCompileGlob_devPatchMax * 2;
		if ( newCapacity <= codeOffset ) {
			newCapacity = codeOffset * 2;
		}

		newTable = (char **)Z_MallocInternal( newCapacity * (int)sizeof( char * ) );
		oldCapacity = scrCompileGlob_devPatchMax;
		Com_Memcpy( newTable, (const void *)scrCompileGlob_devPatchTable,
					(size_t)oldCapacity * sizeof( char * ) );
		Com_Memset( &newTable[scrCompileGlob_devPatchMax], 0,
					(size_t)( newCapacity - scrCompileGlob_devPatchMax ) * sizeof( char * ) );

		scrCompileGlob_devPatchMax = newCapacity;
		free( (void *)scrCompileGlob_devPatchTable );
		scrCompileGlob_devPatchTable = (void *)newTable;
	}

	( (char **)scrCompileGlob_devPatchTable )[codeOffset] =
		(char *)scrCompilePub_developerBufPos;

	Scr_TransferToDeveloperBuffer();
	TempMemorySetPos( (void *)scrCompileGlob_codeRelocStart );
}

/* ---- EmitDeveloperStatementList  0x0046C660 ---- */
static void EmitDeveloperStatementList( sval_u block, sval_u sourcePos ) {
	int     savedChecksum;
	void   *tempMark;

	if ( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( sourcePos.sourcePosValue, "cannot recurse /#" );
	}

	savedChecksum = scrCompilePub_checksum;

	if ( !scrCompilePub_developerScript ) {
		tempMark = TempMalloc( 0 );
		scrCompileGlob_codegenMode = SCR_CODEGEN_DISCARD;
		EmitStatementList( block );
		TempMemorySetPos( tempMark );
	} else {
		if ( !scrCompileGlob_bDeferredCheck ) {
			EmitOpcode( OP_DevBlockBegin, 0, LOCALDEPTH_NONE );
			scrCompileGlob_codeRelocStart = scrCompileGlob_codePos;
		}
		scrCompileGlob_codegenMode = SCR_CODEGEN_RELOCATE;
		EmitStatementList( block );
		scrCompileGlob_bDeferredCheck = 1;
	}

	scrCompileGlob_codegenMode = SCR_CODEGEN_INTERN;
	scrCompilePub_checksum = savedChecksum;
}

/* ---- Scr_InitDeveloperOpcodes  0x0046C790 ---- */
void Scr_InitDeveloperOpcodes__Fv( void ) {
	if ( !scrCompilePub_developerScript ) {
		return;
	}

	scrCompileGlob_devOpBuffer   = Z_MallocInternal( SCR_DEV_OP_BUF_SIZE );
	scrCompileGlob_devPatchMax   = SCR_DEV_PATCH_CAPACITY;
	scrCompileGlob_devPatchTable =
		Z_MallocInternal( SCR_DEV_PATCH_CAPACITY * (int)sizeof( char * ) );
	Com_Memset( (void *)scrCompileGlob_devPatchTable, 0,
				SCR_DEV_PATCH_CAPACITY * sizeof( char * ) );

	scrCompilePub_developerBufPos = (char *)scrCompileGlob_devOpBuffer;
	Com_Memset( scrVmGlob_devCallStack, 0, sizeof( scrVmGlob_devCallStack ) );   /* 0x00A7A488, 0x80 bytes */
	scrCompileGlob_stringFixups   = 0;
}

/* ---- Scr_InsertDeveloperOpcodes  0x0046C800 ---- */
void Scr_InsertDeveloperOpcodes__Fv( void ) {
	int                 codeOffset;
	char               *patch;
	scrStringFixup_t   *fixup;
	scrStringFixup_t   *next;
	unsigned short      string;

	if ( !scrCompilePub_developerScript ) {
		return;
	}

	for ( codeOffset = 0; codeOffset < scrCompileGlob_devPatchMax; codeOffset++ ) {
		patch = ( (char **)scrCompileGlob_devPatchTable )[codeOffset];
		if ( patch ) {
			*patch = ( (char *)scrVarPub_programBuffer )[codeOffset];
			( (char *)scrVarPub_programBuffer )[codeOffset] = (char)OP_DevBlockBegin;
		}
	}

	while ( scrCompileGlob_stringFixups ) {
		fixup = (scrStringFixup_t *)scrCompileGlob_stringFixups;
		if ( *fixup->codePos ) {
			*fixup->codePos = SL_TransferToCanonicalString( *fixup->codePos );
		}
		next = fixup->next;
		free( fixup );
		scrCompileGlob_stringFixups = (void *)next;
	}
}

/* ---- Scr_ShutdownDeveloperOpcodes  0x0046C8C0 ---- */
void Scr_ShutdownDeveloperOpcodes__Fv( void ) {
	if ( !scrCompilePub_developerScript ) {
		return;
	}

	if ( scrCompileGlob_devOpBuffer ) {
		free( (void *)scrCompileGlob_devOpBuffer );
		scrCompileGlob_devOpBuffer = 0;
	}
	if ( scrCompileGlob_devPatchTable ) {
		free( (void *)scrCompileGlob_devPatchTable );
		scrCompileGlob_devPatchTable = 0;
	}
}

/* ---- EmitFormalParameterListRef  0x0046C910 ---- VERIFIED */
static void EmitFormalParameterListRef( sval_u exprlist, sval_u sourcePos ) {
	EmitFormalParameterListRefInternal( SVAL_LIST( exprlist.value )->first );
	EmitOpcode( OP_CheckClearParams, 0, LOCALDEPTH_NONE );
	AddOpcodePos( sourcePos.sourcePosValue );
}

/* ---- SpecifyThread  0x0046C940 ---- */
static void SpecifyThread( sval_u node ) {
	sval_t         *n = SVAL_NODE( node.value );

	switch ( n[0] ) {
	case SVAL_FUNCTION_DEFINITION:
		SpecifyThreadPosition( GetObject( GetVariable( scrCompileGlob_currentFuncRoot, (unsigned short)n[1] ) ), n[4] );
		break;
	case SVAL_DEVELOPER_FUNCTION_DEF:
		SpecifyDeveloperThreadPosition( GetObject( GetVariable( scrCompileGlob_currentFuncRoot, (unsigned short)n[1] ) ), n[4] );
		break;
	}
}

/* ---- EmitThreadInternal  0x0046C9C0 ---- */
static void EmitThreadInternal( sval_u node, sval_u sourcePos ) {
	sval_t *n = SVAL_NODE( node.value );

	AddOpcodePos( sourcePos.sourcePosValue );

	scrCompileGlob_cumulOffset   = 0;
	scrCompileGlob_maxOffset     = 0;
	scrCompileGlob_maxCallOffset = 0;

	CompileTransferRefToString( (unsigned short)n[1], SCR_STRING_USAGE_FUNCTION );
	EmitFormalParameterListRef( SVAL_U( n[2] ), sourcePos );
	EmitStatementList( SVAL_U( n[3] ) );
	EmitOpcode( OP_End, 0, LOCALDEPTH_NONE );

	if ( scrCompileGlob_maxOffset
	   + SCR_LOCAL_SLOT_WIDTH * scrCompileGlob_maxCallOffset
	   >= SCR_OPERAND_STACK_LIMIT ) {
		CompileError( sourcePos.sourcePosValue, "function exceeds operand stack size" );
	}
}

/* ---- EmitNormalThread  0x0046CAB0 ---- */
static void EmitNormalThread( sval_t node, unsigned int sourcePos ) {
	sval_t         *n = SVAL_NODE( node );
	unsigned short  functionObject;

	functionObject = FindObject( FindVariable(
		scrCompileGlob_currentFuncRoot, (unsigned short)n[1] ) );
	SetThreadPosition( functionObject );
	EmitThreadInternal( SVAL_U( node ), SVAL_U( sourcePos ) );
}

/* ---- EmitDeveloperThread  0x0046CB30 ---- */
static void EmitDeveloperThread( sval_u node, sval_u sourcePos ) {
	sval_t         *n = SVAL_NODE( node.value );
	int             savedChecksum;
	unsigned short  functionObject;

	if ( scrCompileGlob_codegenMode != SCR_CODEGEN_INTERN ) {
		CompileError( sourcePos.sourcePosValue, "cannot recurse /#" );
	}

	scrCompileGlob_codeRelocStart = (int)TempMalloc( 0 );
	savedChecksum = scrCompilePub_checksum;

	if ( !scrCompilePub_developerScript ) {
		scrCompileGlob_codegenMode = SCR_CODEGEN_DISCARD;
		EmitThreadInternal( node, sourcePos );
	} else {
		scrCompileGlob_codegenMode = SCR_CODEGEN_RELOCATE;
		functionObject = FindObject( FindVariable(
			scrCompileGlob_currentFuncRoot, (unsigned short)n[1] ) );
		SetDeveloperThreadPosition( functionObject, SVAL_U( n[4] ) );
		EmitThreadInternal( node, sourcePos );
		Scr_TransferToDeveloperBuffer();
	}

	scrCompileGlob_codegenMode = SCR_CODEGEN_INTERN;
	TempMemorySetPos( (void *)scrCompileGlob_codeRelocStart );
	scrCompilePub_checksum = savedChecksum;
}

/* ---- EmitThread  0x0046CC30 ---- */
static void EmitThread( sval_u node, sval_u sourcePos ) {
	sval_t         *n = SVAL_NODE( node.value );

	switch ( n[0] ) {
	case SVAL_FUNCTION_DEFINITION:
		EmitNormalThread( node.value, sourcePos.sourcePosValue );
		break;
	case SVAL_DEVELOPER_FUNCTION_DEF:
		EmitDeveloperThread( node, sourcePos );
		break;
	case SVAL_USING_ANIMTREE:
		Scr_UsingTree( SL_ConvertToString( (unsigned short)n[1] ), n[2] );
		CompileRemoveRefToString( (unsigned short)n[1] );
		break;
	}
}

/* ---- EmitThreadList  0x0046CCD0 ---- */
static void EmitThreadList( sval_u block ) {
	sval_cell_t    *item;

	for ( item = SVAL_BLOCKLIST( block.value )->head; item; item = item->next ) {
		SpecifyThread( SVAL_U( SVAL_ENTRY( item->value )->node ) );
	}
	for ( item = SVAL_BLOCKLIST( block.value )->head; item; item = item->next ) {
		EmitThread( SVAL_U( SVAL_ENTRY( item->value )->node ),
					SVAL_U( SVAL_ENTRY( item->value )->sourcePos ) );
	}
}

/* ---- ScriptCompile  0x0046CD20 ---- */
void ScriptCompile( sval_t parseTree, unsigned short currentFunctionRoot ) {
	scrScriptLoad_t    *pendingLoads;
	int                 pendingCount;
	int                 index;
	const char         *filename;

	scrCompileGlob_currentFuncRoot = currentFunctionRoot;

	scrCompileGlob_codePos        = (int)TempMalloc( 0 );
	scrCompileGlob_bOwnsStrings   = 0;
	scrCompileGlob_animTreeValue  = 0;
	scrCompileGlob_codegenMode    = SCR_CODEGEN_INTERN;
	scrCompileGlob_bDeferredCheck = 0;

	pendingCount = scrCompilePub_scriptCount;
	pendingLoads = pendingCount
		? (scrScriptLoad_t *)_alloca( pendingCount * sizeof( scrScriptLoad_t ) )
		: NULL;
	scrCompileGlob_loadCursor = (int)pendingLoads;

	EmitOpcode( OP_End, 0, LOCALDEPTH_NONE );
	EmitThreadList( SVAL_U( SVAL_NODE( parseTree )[1] ) );

	scrCompilePub_programLen =
		(int)( (char *)TempMalloc( 0 ) - (char *)scrVarPub_programBuffer );

	AdjustFunctionAddresses();

	Hunk_CommitTempMemory();
	Hunk_ClearTempMemoryHigh();

	pendingCount = scrCompilePub_scriptCount;
	for ( index = 0; index < pendingCount; index++ ) {
		filename = SL_ConvertToString( pendingLoads[index].filename );
		if ( !Scr_LoadScript( filename ) ) {
			CompileError( pendingLoads[index].sourcePos,
						  "Could not find script '%s'",
						  SL_ConvertToString( pendingLoads[index].filename ) );
		}
		SL_RemoveRefToString( pendingLoads[index].filename );
	}
}

#ifdef __cplusplus
}
#endif
