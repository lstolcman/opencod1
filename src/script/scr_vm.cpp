/*
 * script/scr_vm.cpp
 *
 * Original translation unit:
 *   /Volumes/BigCheese/ Source/AspyrP4/CoD/Source/script/scr_vm.cpp
 *
 * Retail range 0x00475850-0x0047E360, 82 functions.
 *
 * @fidelity: likely
 */

#ifdef __cplusplus
/* retail built this unit as C++ with exceptions (/TP /GX); every name keeps C linkage */
extern "C" {
#endif

#include "../qcommon/qcommon.h"


#include "scr_local.h"

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#define SCR_VM_STACK_COUNT      2048    /* 0x00A76480, 8 bytes each */
#define SCR_VM_MAX_CALL_DEPTH   32

#define SCR_TIME_MASK           0x00FFFFFFu
#define SCR_MAX_WAIT_SECONDS    16777
#define SCR_WAIT_SCALE          1000.0f
#define SCR_SHIFT_MASK          0x1F
#define SCR_FLOAT_EQ_EPSILON    0.000001f

#define SCR_STRING_MT_TYPE      14
#define SCR_VECTOR_ALLOC_SIZE   ( 2 + 3 * 4 )
#define SCR_STRING_MT_TYPE_ADD  6

#define SCR_NAME_NOTIFY_LIST    0x20000u
#define SCR_NAME_STACK          0x20001u

#define SCR_STACK_ENTRY_SIZE    5
#define SCR_STACK_HEADER_SIZE   12
#define SCR_STACK_BUFFER_SIZE( n )  ( SCR_STACK_ENTRY_SIZE * (n) + SCR_STACK_HEADER_SIZE )
#define SCR_STACK_MT_TYPE           1

typedef struct VariableStackBuffer_s {
	unsigned int    time;           /* +0x00 resume key, or 0 for a notify wait */
	const char     *codePos;        /* +0x04 where to resume */
	unsigned short  valueCount;     /* +0x08 packed entries that follow */
	unsigned short  thread;         /* +0x0A innermost thread handle */
	unsigned char   values[1];      /* +0x0C count * (byte type, dword payload) */
} VariableStackBuffer;

/* retail 0x00A76478 / 0x00A7A5A0 / 0x00A76480 -- see VM_Init */
VariableValue           scrVmPub_stack[SCR_VM_STACK_COUNT];
VariableValue          *scrVmPub_top;
VariableValue          *scrVmPub_maxstack;

int                     scrVmPub_outparamcount;
int                     scrVmPub_inparamcount;

#define scrVmGlob_funcCount         scrVmGlob_funcCount        /* 0x00A7A5D8 */
#define scrVmPub_terminal_error     scrVmPub_terminal_error         /* 0x00A7A5CC */

/* 0x00A7A5A4 / the dialog-message slot beside it */
const char             *scrVarPub_error_message;
const char             *scrVmGlob_dialog_error_message;

int                     scrVmGlob_leftType;
int                     scrVmGlob_rightType;

int                     scrVmGlob_loopTick;
signed __int32 scrVmGlob_funcCount;   /* 0x00A7A5D8 */
unsigned char scrVmPub_terminal_error;   /* 0x00A7A5CC */

/* 0x00A7A510 / 0x00A7A488 / 0x00A7A5AC -- the three per-depth stacks. */
const char             *scrVmGlob_callStack[SCR_VM_MAX_CALL_DEPTH];
const char             *scrVmGlob_devCallStack[SCR_VM_MAX_CALL_DEPTH];
unsigned char           scrVmGlob_devOpcode[SCR_VM_MAX_CALL_DEPTH];

#define scrVmGlob_callStack     scrVmGlob_callStack
#define scrVmGlob_devCallStack  scrVmGlob_devCallStack
#define scrVmGlob_devOpcode     scrVmGlob_devOpcode

unsigned short          scrVarPub_pauseArrayId;
unsigned short scrVarPub_levelId;
ScriptArrayRootStorage scrVarPub_gameId;
unsigned short scrVarPub_animId;
/* plain word handles defined in this C++ unit (scr_vm_state.c's C common could not be
   widened): retail loads them as dwords in VM_SetTime/Scr_SetTime and reads them ahead
   of WriteId's store in Scr_SavePost */
unsigned short scr_timeArrayId;

unsigned int            scrVarPub_time;

#define SCR_PROGRAM_BASE        ( (const char *)scrVarPub_programBuffer )
#define SCR_DEV_PATCH_TABLE     ( (unsigned char **)scrCompileGlob_devPatchTable )
#define SCR_DEVELOPER           scrVarPub_developer

#define SCR_SET_ENTITY_FIELD    scrImport_SetObjectField
#define SCR_GET_ENTITY_FIELD    scrImport_GetObjectField
/* the import table declares these unprototyped; spell the call type out for C++ */
#define SCR_OBJECT_FIELD_FN( fn )  ( (int (*)( unsigned int, int, int ))(fn) )

extern int              scr_loading;
extern int              scrVarPub_error_index;
extern int              __rdtsc( void );

#ifdef COD1_VC7
/* retail inlines rdtsc at every use (its unreferenced out-of-line copy is at 0x00475840) */
static __inline int Scr_ReadTsc( void ) {
	__asm push  edx
	__asm _emit 0x0F
	__asm _emit 0x31
	__asm pop   edx
}
#else
#define Scr_ReadTsc()   __rdtsc()
#endif

extern int  Scr_GetAnims( int slot );
extern int  Scr_GetAnimTreeCount( void );

extern void RuntimeError( const char *dialogMessage, const char *codePos, int offset,
                          const char *message );
extern void Scr_PrintPrevCodePos( const char *codePos, int offset );
extern unsigned short   Scr_EvalArrayIndex_m( unsigned short id, VariableValue *index );
extern unsigned short   EvalArrayRef( unsigned short id, VariableValue *index );
extern void             ClearArray( unsigned short id, VariableValue *index );
extern void             GetEmptyArray( VariableValue *value );

static unsigned short   VM_Execute( VariableValue *stackTop, const char *codePos,
									unsigned short thread, unsigned short currentObject,
									VariableValue *stackBase );

#ifdef __cplusplus
/* Retail compiled this unit as C++ with exceptions (/TP /GX): a script error throws a
   copy of this empty object (retail 0x00A7A598, ThrowInfo .?AVScriptErrorClass@@) and
   VM_Execute catches it.  The class name is retail's RTTI name; g_script_error is the
   name in the Mac 1.3 symbol list (inferred: its address is not confirmed there). */
class ScriptErrorClass { };
ScriptErrorClass        g_script_error;

#define SCR_ERROR_CAUGHT()      ( scrVmGlob_funcCount )
#define SCR_THROW_ERROR()       throw g_script_error
#define VM_TRY                  try
#define VM_CATCH                catch ( ScriptErrorClass ) { }
#define VM_ERROR_FRAME_DECL
#define VM_ERROR_FRAME_PUSH()
#define VM_ERROR_FRAME_POP()
#else
/* plain C build: VM_Execute's setjmp frame stands in for the catch */
static jmp_buf         *scrVmErrorFrame;

#define SCR_ERROR_CAUGHT()      ( scrVmErrorFrame != NULL && scrVmGlob_funcCount != 0 )
#define SCR_THROW_ERROR()       longjmp( *scrVmErrorFrame, 1 )
#define VM_TRY                  if ( setjmp( frame ) == 0 )
#define VM_CATCH
#define VM_ERROR_FRAME_DECL     jmp_buf frame; jmp_buf * volatile savedFrame = scrVmErrorFrame;
#define VM_ERROR_FRAME_PUSH()   ( scrVmErrorFrame = &frame )
#define VM_ERROR_FRAME_POP()    ( scrVmErrorFrame = savedFrame )
#endif

/* ---- Scr_Error__Fv  0x00475980 ---- the Mac's argument-less Scr_Error overload */
void Scr_ErrorNoArg_m( void ) {
	if ( SCR_ERROR_CAUGHT() ) {
		SCR_THROW_ERROR();
	}
	Com_Error( ERR_DROP, "\x15%s", scrVarPub_error_message );
}

/* ---- Scr_Error  0x0047DF40 ---- */
void Scr_Error( const char *error ) {
	scrVarPub_error_message = error;
	Scr_ErrorNoArg_m();
}

/* ---- Scr_ErrorWithDialogMessage  0x0047DF80 ---- */
void Scr_ErrorWithDialogMessage( const char *error, const char *dialogError ) {
	scrVarPub_error_message = error;
	scrVmGlob_dialog_error_message = dialogError;
	Scr_ErrorNoArg_m();
}

/* ---- Scr_TerminalError  0x0047DFD0 ---- */
void Scr_TerminalError( const char *error ) {
	Scr_DumpScriptThreads();
	scrVmPub_terminal_error = 1;
	Scr_Error( error );
}

/* ---- Scr_ParamError  0x0047E020 ---- */
void Scr_ParamError( unsigned int index, const char *error ) {
	scrVarPub_error_index = index + 1;
	Scr_Error( error );
}

/* ---- Scr_ObjectError  0x0047E070 ---- */
void Scr_ObjectError( const char *error ) {
	scrVarPub_error_index = -1;
	Scr_Error( error );
}

#define VAR_NODE( id )          ( &scrVarNodes[ (id) ] )
#define VAR_TYPEOF( id )        ( (int)( scrVarNodes[ (id) ].status & VAR_STATUS_TYPE_MASK ) )
#define VAR_NAMEOF( id )        ( scrVarNodes[ (id) ].status >> VAR_STATUS_NAME_SHIFT )

/* GetRefString as retail inlines it: through the cached arena base at 0x008E6208 */
#define SCR_REFSTRING( h )      ( (scrStringEntry_t *)( (scrMemBlock_t *)GetRefString_var + (h) ) )

/* AddRefToValue__FP13VariableValue  0x00475AEE  VERIFIED */
void AddRefToValue__FP13VariableValue( VariableValue *value ) {
	AddRefToValue( value->type, value->u );
}

__declspec(noinline) void RemoveRefToValue__FP13VariableValue( VariableValue *value ) {
	RemoveRefToValue( value->type, value->u );
}

/* ---- IsZero  0x004759D0 ---- */
unsigned char __fastcall IsZero( const char *s ) {
 while (*s <= ' ') {
  if (!*s) return 0;
  ++s;
 }
 if (*s == '-' || *s == '+') ++s;
 if (*s == '0') return 1;
 if (*s != '.') return 0;
 if (s[1] == '0') return 1;
 return 0;
}

int IsZero__FPCc( const char *s ) { return IsZero( s ); }

/* ---- Scr_GetNumScriptThreads  0x004759C0 ---- VERIFIED */
int Scr_GetNumScriptThreads( void ) {
	return 0;
}

/* ---- CastBool  0x00475A10 ---- */
unsigned char CastBool( VariableValue *value ) {
	unsigned short  handle;

	if ( value->type == VAR_FLOAT ) {
		value->type = VAR_INTEGER;
		value->u.intValue = ( value->u.floatValue != 0.0f ) ? 1 : 0;
		return qtrue;
	}

	if ( value->type == VAR_STRING ) {
		handle = value->u.stringValue;
		value->u.intValue = atoi( SL_ConvertToString( handle ) ) != 0;

		if ( value->u.intValue || IsZero( SL_ConvertToString( handle ) ) ) {
			value->type = VAR_INTEGER;
			SL_RemoveRefToString( handle );
			return qtrue;
		}

		scrVarPub_error_message =
			va( "cannot cast \"%s\" to bool", SL_ConvertToString( handle ) );
		SL_RemoveRefToString( handle );
		value->type = VAR_UNDEFINED;
		return qfalse;
	}

	scrVarPub_error_message =
		va( "cannot cast %s to bool", var_typename[value->type] );
	RemoveRefToValue__FP13VariableValue( value );
	value->type = VAR_UNDEFINED;
	return qfalse;
}

/* ---- CastInt  0x00475B00 ---- */
unsigned char CastInt( VariableValue *value ) {
	unsigned short  handle;
	int             n;

	if (value->type == VAR_INTEGER) {
		return qtrue;

 }
 if (value->type == VAR_FLOAT) {
		value->type = VAR_INTEGER;
		value->u.intValue = (int)value->u.floatValue;
		return qtrue;

 }
 if (value->type == VAR_STRING) {
		handle = value->u.stringValue;
		n = atoi( SL_ConvertToString( handle ) );
		value->u.intValue = n;

		if ( n == 0 && !IsZero( SL_ConvertToString( handle ) ) ) {
			scrVarPub_error_message =
				va( "cannot cast \"%s\" to int", SL_ConvertToString( handle ) );
			SL_RemoveRefToString( handle );
			value->type = VAR_UNDEFINED;
			return qfalse;
		}
		value->type = VAR_INTEGER;
		SL_RemoveRefToString( handle );
		return qtrue;

	}

	scrVarPub_error_message =
		va( "cannot cast %s to int", var_typename[value->type] );
	RemoveRefToValue__FP13VariableValue(value);
 value->type = VAR_UNDEFINED;
	return qfalse;
}

/* ---- CastFloat  0x00475BD0 ---- */
unsigned char CastFloat( VariableValue *value ) {
	unsigned short  handle;

	if (value->type == VAR_FLOAT) {
		return qtrue;

 }
 if (value->type == VAR_INTEGER) {
		value->type = VAR_FLOAT;
		value->u.floatValue = (float)value->u.intValue;
		return qtrue;

 }
 if (value->type == VAR_STRING) {
		handle = value->u.stringValue;
		value->u.floatValue = (float)atof( SL_ConvertToString( handle ) );

		if ( value->u.floatValue == 0 && !IsZero( SL_ConvertToString( handle ) ) ) {
			scrVarPub_error_message =
				va( "cannot cast \"%s\" to float", SL_ConvertToString( handle ) );
			SL_RemoveRefToString( handle );
			value->type = VAR_UNDEFINED;
			return qfalse;
		}
		value->type = VAR_FLOAT;
		SL_RemoveRefToString( handle );
		return qtrue;

	}

	scrVarPub_error_message =
		va( "cannot cast %s to float", var_typename[value->type] );
	RemoveRefToValue__FP13VariableValue(value);
 value->type = VAR_UNDEFINED;
	return qfalse;
}

/* ---- CastString  0x00475CB0 ---- */
unsigned char CastString( VariableValue *value ) {
	const float *v;

	if (value->type == VAR_STRING) {
		return qtrue;

 }
 if (value->type == VAR_INTEGER) {
		value->type = VAR_STRING;
 value->u.stringValue = SL_GetStringForInt(value->u.intValue);
		return qtrue;

 }
 if (value->type == VAR_FLOAT) {
		value->type = VAR_STRING;
 value->u.stringValue = SL_GetStringForFloat(value->u.floatValue);
		return qtrue;

 }
 if (value->type == VAR_VECTOR) {
		v = value->u.vectorValue;
		value->type = VAR_STRING;
 value->u.stringValue = SL_GetStringForVector(v);
		RemoveRefToVector( v );
		return qtrue;

	}

	scrVarPub_error_message =
		va( "cannot cast %s to string", var_typename[value->type] );
	RemoveRefToValue__FP13VariableValue(value);
 value->type = VAR_UNDEFINED;
	return qfalse;
}

/* ---- CastIString  0x00475D50 ---- */
/* CastIString/CastVector/CastPointer return byte: retail's bool comes back in al */
byte CastIString( VariableValue *value ) {
	if ( value->type == VAR_ISTRING ) {
		return qtrue;
	}
	scrVarPub_error_message =
		va( "cannot cast %s to istring", var_typename[value->type] );
	RemoveRefToValue__FP13VariableValue( value );
	value->type = VAR_UNDEFINED;
	return qfalse;
}

/* ---- CastVector  0x00475D90 ---- */
byte CastVector( VariableValue *value ) {
	if ( value->type == VAR_VECTOR ) {
		return qtrue;
	}
	scrVarPub_error_message =
		va( "cannot cast %s to vector", var_typename[value->type] );
	RemoveRefToValue__FP13VariableValue( value );
	value->type = VAR_UNDEFINED;
	return qfalse;
}

/* ---- CastPointer  0x00475DD0 ---- */
byte CastPointer( VariableValue *value ) {
	if ( value->type == VAR_OBJECT ) {
		return qtrue;
	}
	scrVarPub_error_message =
		va( "cannot cast %s to object", var_typename[value->type] );
	RemoveRefToValue__FP13VariableValue( value );
	value->type = VAR_UNDEFINED;
	return qfalse;
}

/* ---- CastVector2  0x00475E10 ---- */
void CastVector2( VariableValue *values ) {
	float   v[3];
	int     i;

	for ( i = 2; i >= 0; i-- ) {
		if ( CastFloat( &values[i] ) ) {
			v[2 - i] = values[i].u.floatValue;
		} else {
			scrVarPub_error_index = i;
			Scr_ErrorNoArg_m();
		}
	}

	values[0].type = VAR_VECTOR;
	values[0].u = AllocVectorValue( v );
}

/* ---- ClearVector  0x00475ED0 ---- */
extern const char *scrMemTree_constBase;
extern int scrMemTree_constLen;
void ClearVector( VariableValue *values ) {
	int i;

	for ( i = 2; i >= 0; --i ) {
		RemoveRefToValue( values[i].type, values[i].u );
	}
	values[0].type = VAR_UNDEFINED;
}

/* ---- UnmatchingTypesError  0x00475FB0 ---- */
void UnmatchingTypesError( VariableValue *left, VariableValue *right ) {
	scrVarPub_error_message = va( "pair has unmatching types '%s' and '%s'",
								  var_typename[scrVmGlob_leftType],
								  var_typename[scrVmGlob_rightType] );
	RemoveRefToValue(left->type,left->u);
 left->type = VAR_UNDEFINED;
	RemoveRefToValue(right->type,right->u);
 right->type = VAR_UNDEFINED;
}

/* ---- CastWeakerPair  0x00476090 ---- */
unsigned char CastWeakerPair(VariableValue *left, VariableValue *right) {
 int lt = left->type;
 int rt = right->type;
 const float *v;
 scrVmGlob_leftType = lt;
 scrVmGlob_rightType = rt;
 if (lt == rt) return 1;
if (lt < rt) {
 switch(lt) {
 case VAR_STRING:
  switch(rt) {
  case VAR_INTEGER:
   right->type = VAR_STRING;
   right->u.stringValue = SL_GetStringForInt(right->u.intValue);
   return 1;
  case VAR_FLOAT:
   right->type = VAR_STRING;
   right->u.stringValue = SL_GetStringForFloat(right->u.floatValue);
   return 1;
  case VAR_VECTOR:
   v = right->u.vectorValue;
   right->type = VAR_STRING;
   right->u.stringValue = SL_GetStringForVector(v);
   RemoveRefToVector(v);
   return 1;
  }
 case VAR_FLOAT:
  if (rt == VAR_INTEGER) {
   right->type = VAR_FLOAT;
   right->u.floatValue = (float)right->u.intValue;
   return 1;
  }
  break;
 }
 UnmatchingTypesError(left,right); return 0;
} else {
 switch(rt) {
 case VAR_STRING:
  switch(lt) {
  case VAR_INTEGER:
   left->type = VAR_STRING;
   left->u.stringValue = SL_GetStringForInt(left->u.intValue);
   return 1;
  case VAR_FLOAT:
   left->type = VAR_STRING;
   left->u.stringValue = SL_GetStringForFloat(left->u.floatValue);
   return 1;
  case VAR_VECTOR:
   v = left->u.vectorValue;
   left->type = VAR_STRING;
   left->u.stringValue = SL_GetStringForVector(v);
   RemoveRefToVector(v);
   return 1;
  }
 case VAR_FLOAT:
  if (lt == VAR_INTEGER) {
   left->type = VAR_FLOAT;
   left->u.floatValue = (float)left->u.intValue;
   return 1;
  }
  break;
 }
 UnmatchingTypesError(left,right); return 0;
}
}

/* CastWeakerPairArray  0x004761E0 */
qboolean CastWeakerPairArray( VariableValue *values ) {
	return (qboolean)CastWeakerPair( &values[0], &values[1] );
}

extern float CM_Q_fabs( float v );

/* ---- CheckEquality  0x004761E0 ---- */
unsigned char CheckEquality( VariableValue *left, VariableValue *right ) {
	unsigned short  ls, rs;
	const float    *lv, *rv;
	float           d;
 int result;

	if ( !CastWeakerPair( left, right ) ) {
		return qfalse;
	}

	switch ( left->type ) {
	case VAR_UNDEFINED:
		left->type = VAR_INTEGER;
		left->u.intValue = 1;
		return qtrue;

	case VAR_INTEGER:
		left->u.intValue = left->u.uintValue == right->u.uintValue;
		return qtrue;

	case VAR_FLOAT:
		left->type = VAR_INTEGER;
		left->u.intValue = CM_Q_fabs( left->u.floatValue - right->u.floatValue ) < 0.000001f;
		return qtrue;

	case VAR_STRING:
	case VAR_ISTRING:
		left->type = VAR_INTEGER;
 result = (left->u.stringValue == right->u.stringValue);
 SL_RemoveRefToString(left->u.stringValue);
 SL_RemoveRefToString(right->u.stringValue);
 left->u.intValue = result;
		return qtrue;

	case VAR_VECTOR:
		left->type = VAR_INTEGER;
 lv = left->u.vectorValue;
 rv = right->u.vectorValue;
 result = (lv[0] == rv[0] && lv[1] == rv[1] && lv[2] == rv[2]);
 RemoveRefToVector(left->u.vectorValue);
 RemoveRefToVector(right->u.vectorValue);
 left->u.intValue = result;
		return qtrue;

	case VAR_OBJECT:
		left->type = VAR_INTEGER;
 result = (left->u.halfword.lo == right->u.halfword.lo);
 RemoveRefToObject(left->u.halfword.lo);
 RemoveRefToObject(right->u.halfword.lo);
 left->u.intValue = result;
		return qtrue;

	case VAR_ANIMATION:
		left->type = VAR_INTEGER;
		left->u.intValue = left->u.uintValue == right->u.uintValue;
		return qtrue;

	default:
		break;
	}

	UnmatchingTypesError( left, right );
	return qfalse;
}

/* ---- VM_ConcatenateStrings  0x0047E360 ---- */
/* Retail has no length cap and no NULL guard: it _alloca's exactly
   strlen(a)+strlen(b)+1 bytes and always interns the result. The only limit
   is downstream, in MT_GetSize (0x10000 and up is an ERR_DROP). */
unsigned short VM_ConcatenateStrings( VariableValue *values ) {
	const char     *a = SL_ConvertToString( values[0].u.stringValue );
	const char     *b = SL_ConvertToString( values[1].u.stringValue );
	unsigned int    la = (unsigned int)strlen( a );
	unsigned int    len = (unsigned int)strlen( b ) + la + 1;
	char           *buffer = (char *)_alloca( len );

	strcpy( buffer, a );
	strcpy( buffer + la, b );
	return SL_GetStringOfLen( buffer, 0, len, SCR_STRING_MT_TYPE );
}

/* ---- IncInParam  0x0047C470 ---- */
void IncInParam( void ) {
	while ( scrVmPub_outparamcount != 0 ) {
		RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u );
		scrVmPub_top--;
		scrVmPub_outparamcount--;
	}

	scrVmPub_top++;
	scrVmPub_inparamcount++;

	if ( scrVmPub_top > scrVmPub_maxstack ) {
		Com_Error( ERR_DROP, "\x15Internal script stack overflow" );
	}

}

int IncInParam__Fv( void ) { IncInParam(); return 0; }

/* ---- Scr_GetNumParam  0x0047DB30 ---- */
int Scr_GetNumParam( void ) {
	return scrVmPub_outparamcount;
}

/* ---- Scr_GetType  0x0047DA20 ---- */
int Scr_GetType(unsigned int index) {
 if(index < (unsigned int)scrVmPub_outparamcount) return (scrVmPub_top-index)->type;
 Scr_Error(va("parameter %d does not exist",index+1));
 return 0;
}


/* ---- Scr_GetPointerType  0x0047DA90 ---- */
int Scr_GetPointerType( unsigned int index ) {
	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		if ( ( scrVmPub_top - index )->type == VAR_OBJECT ) {
			return GetVarType( ( scrVmPub_top - index )->u.halfword.lo );
		}
		Scr_Error( va( "parameter %d is not a pointer", index + 1 ) );
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return 0;
}


/* ---- Scr_GetBool  0x0047D050 ---- */
int Scr_GetBool( unsigned int index ) {
	VariableValue *v;

	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		v = scrVmPub_top - index;
		if ( v->type == VAR_INTEGER ) {
			return v->u.intValue != 0;
		}
		if ( CastBool( v ) ) {
			return ( scrVmPub_top - index )->u.intValue;
		}
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return 0;
}

/* ---- Scr_GetInt  0x0047D100 ---- */
int Scr_GetInt( unsigned int index ) {
	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		if ( CastInt( scrVmPub_top - index ) ) {
			return ( scrVmPub_top - index )->u.intValue;
		}
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return 0;
}

/* ---- Scr_GetFloat  0x0047D3F0 ---- */
float Scr_GetFloat( unsigned int index ) {
	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		if ( CastFloat( scrVmPub_top - index ) ) {
			return ( scrVmPub_top - index )->u.floatValue;
		}
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return 0.0f;
}

/* ---- Scr_GetConstString  0x0047D490 ---- */
unsigned short Scr_GetConstString( unsigned int index ) {
	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		if ( CastString( scrVmPub_top - index ) ) {
			return ( scrVmPub_top - index )->u.stringValue;
		}
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return 0;
}

/* ---- Scr_GetString  0x0047D530 ---- */
const char *Scr_GetString( unsigned int index ) {
	unsigned short handle = Scr_GetConstString( index );

	return handle != 0 ? SCR_REFSTRING( handle )->text : NULL;
}

/* ---- Scr_GetConstIString  0x0047D6B0 ---- */
unsigned short Scr_GetConstIString( unsigned int index ) {
	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		if ( CastIString( scrVmPub_top - index ) ) {
			return ( scrVmPub_top - index )->u.stringValue;
		}
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return 0;
}


/* ---- Scr_GetIString  0x0047D760 ---- */
const char *Scr_GetIString( unsigned int index ) {
	unsigned short handle = Scr_GetConstIString( index );

	return handle != 0 ? SCR_REFSTRING( handle )->text : NULL;
}

/* xanim.cpp (an XAnim begins with its debug name; XAnimTree with its XAnim) */
extern const char *XAnimGetAnimName( const void *anims, int nodeIndex );
extern const char *XAnimGetAnimTreeDebugName_m( const void *anims );

typedef struct scr_anim_s {
	unsigned short  index;
	unsigned short  tree;
} scr_anim_t;

/* ---- Scr_GetDebugString  0x0047D560 ---- */
const char *Scr_GetDebugString( unsigned int index ) {
	VariableValue  *value;
	int             type;
	const char     *typeName;
	scr_anim_t      anim;

	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		value = scrVmPub_top - index;
		type  = value->type;
		typeName = var_typename[ type != VAR_OBJECT ? type : GetVarType( value->u.halfword.lo ) ];

		if ( CastString( scrVmPub_top - index ) ) {
			return SL_ConvertToString( ( scrVmPub_top - index )->u.stringValue );
		}

		scrVarPub_error_message = NULL;
		scrVmGlob_dialog_error_message = NULL;
		scrVarPub_error_index = 0;

		if ( type == VAR_ANIMATION ) {
			memcpy( &anim, &( scrVmPub_top - index )->u, sizeof( anim ) );
			return XAnimGetAnimName( (void *)Scr_GetAnims( anim.tree ), anim.index );
		}
		if ( type == VAR_ISTRING ) {
			return SL_ConvertToString( ( scrVmPub_top - index )->u.stringValue );
		}
		return typeName;
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return NULL;
}

/* ---- Scr_GetAnim  0x0047D190 ---- */
/* returns the 4-byte scr_anim_t by value (Mac 1.3 Scr_GetAnim) */
scr_anim_t Scr_GetAnim( unsigned int index, void **tree ) {
	VariableValue  *value;
	scr_anim_t      anim;

	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		value = scrVmPub_top - index;
		if ( value->type == VAR_ANIMATION ) {
			memcpy( &anim, &value->u, sizeof( anim ) );
			if ( !tree || (void *)Scr_GetAnims( anim.tree ) == *tree ) {
				return anim;
			}
			scrVarPub_error_message = va(
				"anim '%s' in animtree '%s' does not belong to the entity's animtree '%s'",
				XAnimGetAnimName( (void *)Scr_GetAnims( anim.tree ), anim.index ),
				XAnimGetAnimTreeDebugName_m( (void *)Scr_GetAnims( anim.tree ) ),
				XAnimGetAnimTreeDebugName_m( *tree ) );
		} else {
			scrVarPub_error_message = va( "cannot cast %s to anim", var_typename[value->type] );
		}
		RemoveRefToValue__FP13VariableValue( scrVmPub_top - index );
		( scrVmPub_top - index )->type = VAR_UNDEFINED;
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	anim.index = 0;
	anim.tree = 0;
	return anim;
}

/* ---- Scr_GetAnimTree  0x0047D2C0 ---- */
/* returns the 4-byte scr_animtree_t by value (Mac 1.3 Scr_GetAnimTree), reading the pool
   tables directly */
typedef struct scr_animtree_s {
	void           *anims;
} scr_animtree_t;

typedef scr_animtree_t scr_animtree_pool_t[128];

scr_animtree_t Scr_GetAnimTree( unsigned int index ) {
	VariableValue  *value;
	scr_animtree_t *tree;
	scr_animtree_t  result;

	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		value = scrVmPub_top - index;
		if ( value->type == VAR_INTEGER ) {
			if ( value->u.uintValue <= (unsigned int)scrAnimPub_treeCount[ xanim_activePoolSlot ] ) {
				tree = (scr_animtree_t *)&scrAnimPub_trees[ 128 * xanim_activePoolSlot + value->u.intValue ];
				if ( tree->anims ) {
					result = *tree;
					return result;
				}
			}
			scrVarPub_error_message = "bad anim tree";
		} else {
			scrVarPub_error_message = va( "cannot cast %s to animtree", var_typename[value->type] );
		}
		RemoveRefToValue__FP13VariableValue( scrVmPub_top - index );
		( scrVmPub_top - index )->type = VAR_UNDEFINED;
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return ( (scr_animtree_pool_t *)scrAnimPub_trees )[ xanim_activePoolSlot ][0];
}

/* ---- Scr_GetVector  0x0047D790 ---- */
void Scr_GetVector( unsigned int index, float *vectorValue ) {
	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		if ( CastVector( scrVmPub_top - index ) ) {
			VectorCopy( ( scrVmPub_top - index )->u.vectorValue, vectorValue );
			return;
		}
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
}

/* ---- Scr_GetFunc  0x0047D860 ---- */
int Scr_GetFunc(unsigned int index) {
 if(index < (unsigned int)scrVmPub_outparamcount) {
  if((scrVmPub_top-index)->type==VAR_FUNCTION) return (int)((scrVmPub_top-index)->u.codePosValue-SCR_PROGRAM_BASE);
  scrVarPub_error_message="not a function";
  scrVarPub_error_index=index+1;
  Scr_ErrorNoArg_m();
 }
 Scr_Error(va("parameter %d does not exist",index+1));
 return 0;
}


/* ---- Scr_GetEntityNum  0x0047D8F0 ---- */
int Scr_GetEntityNum( unsigned int index, unsigned int *classnum ) {
	unsigned short id;

	if ( index < (unsigned int)scrVmPub_outparamcount ) {
		if ( CastPointer( scrVmPub_top - index ) ) {
			id = ( scrVmPub_top - index )->u.halfword.lo;
			if ( GetVarType( id ) == VAR_ENTITY ) {
				*classnum = GetEntityType( id );
				return GetEntnum( id );
			}
			scrVarPub_error_message = va( "cannot cast %s to entity", var_typename[ GetVarType( id ) ] );
		}
		scrVarPub_error_index = index + 1;
		Scr_ErrorNoArg_m();
	}
	Scr_Error( va( "parameter %d does not exist", index + 1 ) );
	return 0;
}


/* ---- Scr_AddBool  0x0047DB40 ---- */
void Scr_AddBool( qboolean value ) {
	IncInParam();
	scrVmPub_top->type = VAR_INTEGER;
	scrVmPub_top->u.intValue = value;
}

/* ---- Scr_AddInt  0x0047DB60 ---- */
void Scr_AddInt( int value ) {
	IncInParam();
	scrVmPub_top->type = VAR_INTEGER;
	scrVmPub_top->u.intValue = value;
}

/* ---- Scr_AddFloat  0x0047DB80 ---- */
void Scr_AddFloat( float value ) {
	IncInParam();
	scrVmPub_top->type = VAR_FLOAT;
	scrVmPub_top->u.floatValue = value;
}

/* ---- Scr_AddAnim  0x0047DBA0 ---- */
void Scr_AddAnim( unsigned int value ) {
	IncInParam();
	scrVmPub_top->type = VAR_ANIMATION;
	scrVmPub_top->u.uintValue = value;
}

/* ---- Scr_AddUndefined  0x0047DBC0 ---- */
void Scr_AddUndefined( void ) {
	IncInParam();
	scrVmPub_top->type = VAR_UNDEFINED;
}

/* ---- Scr_AddObject  0x0047DBE0 ---- */
void Scr_AddObject( unsigned short id ) {
	IncInParam();
	scrVmPub_top->type = VAR_OBJECT;
	scrVmPub_top->u.halfword.lo = id;
	AddRefToObject( id );
}

/* ---- Scr_AddEntityNum  0x0047DC10 ---- */
void Scr_AddEntityNum( int entnum, int classnum ) {
	unsigned short id = Scr_GetEntityId( entnum, classnum );

	IncInParam();
	scrVmPub_top->type = VAR_OBJECT;
	scrVmPub_top->u.halfword.lo = id;
	AddRefToObject( id );
}

/* ---- Scr_AddStruct  0x0047DC50 ---- */
void Scr_AddStruct( void ) {
	VariableValueInternal  *node = AllocVariable();
	unsigned short          id;

	node->status = ( VAR_STAT_EXTERNAL | VAR_STRUCT );
	node->u.halfword.lo = 0;
	id = (unsigned short)( node - scrVarNodes );

	IncInParam();
	scrVmPub_top->type = VAR_OBJECT;
	scrVmPub_top->u.halfword.lo = id;
	AddRefToObject( id );
	RemoveRefToObject( id );
}

/* ---- Scr_AddString  0x0047DCE0 ---- */
void Scr_AddString( const char *text ) {
	IncInParam();
	scrVmPub_top->type = VAR_STRING;
	scrVmPub_top->u.stringValue =
		SL_GetStringOfLen( text, 0, strlen( text ) + 1, SCR_STRING_MT_TYPE_ADD );
}

/* ---- Scr_AddIString  0x0047DD30 ---- */
void Scr_AddIString( const char *text ) {
	IncInParam();
	scrVmPub_top->type = VAR_ISTRING;
	scrVmPub_top->u.stringValue =
		SL_GetStringOfLen( text, 0, strlen( text ) + 1, SCR_STRING_MT_TYPE_ADD );
}

/* ---- Scr_AddConstString  0x0047DD80 ---- */
void Scr_AddConstString( unsigned short handle ) {
	IncInParam();
	scrVmPub_top->type = VAR_STRING;
	scrVmPub_top->u.stringValue = handle;
	SCR_REFSTRING( handle )->refCount++;
}

/* ---- Scr_AddVector  0x0047DDB0 ---- */
void Scr_AddVector( const float *v ) {
	IncInParam();
	scrVmPub_top->type = VAR_VECTOR;
	scrVmPub_top->u = AllocVectorValue( v );
}

/* ---- Scr_MakeArray  0x0047DE00 ---- */
void Scr_MakeArray( void ) {
	IncInParam();
	scrVmPub_top->type = VAR_OBJECT;
	scrVmPub_top->u.halfword.lo = Scr_AllocArray();
}

/* ---- Scr_AddArray  0x0047DE50 ---- */
void Scr_AddArray( void ) {
	unsigned short  arrayId;

	scrVmPub_top--;
	scrVmPub_inparamcount--;

	arrayId = scrVmPub_top->u.halfword.lo;
	SetNewVariableValue( GetArrayVariable( arrayId, GetArraySize( arrayId ) ), scrVmPub_top + 1 );
}

/* ---- Scr_AddArrayStringIndexed  0x0047DED0 ---- */
void Scr_AddArrayStringIndexed( unsigned short name ) {
	scrVmPub_top--;
	scrVmPub_inparamcount--;
	SetNewVariableValue( GetVariable( scrVmPub_top->u.halfword.lo, name ), scrVmPub_top + 1 );
}

/* ---- SetEntityFieldValue  0x0047E0C0 ---- VERIFIED */
/* ---- GetEntityFieldValue  0x0047E1E0 ---- */
__declspec(noinline) void SetEntityFieldValue__FUiiiP13VariableValue( unsigned int classnum, int entnum,
												 int offset, VariableValue *value ) {
	scrVmPub_top = value;
	scrVmPub_outparamcount = 1;

	SCR_OBJECT_FIELD_FN( scrImport.SetObjectField )( classnum, entnum, offset );

	/* Scr_ClearOutParams, inlined */
	while ( scrVmPub_outparamcount ) {
		RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u );
		--scrVmPub_top;
		--scrVmPub_outparamcount;
	}
}

void GetEntityFieldValue__FUiiiP13VariableValue( unsigned int classnum, int entnum,
												 int offset, VariableValue *value ) {
	scrVmPub_top = value - 1;
	value->type = VAR_UNDEFINED;

	SCR_OBJECT_FIELD_FN( SCR_GET_ENTITY_FIELD )( classnum, entnum, offset );

	scrVmPub_inparamcount = 0;
}

/* ---- Scr_SetDynamicEntityField  0x0047E210 ---- VERIFIED */
void Scr_SetDynamicEntityField( int entnum, int classnum, unsigned short name ) {
	unsigned short entityId = Scr_GetEntityId( entnum, classnum );

	SetVariableFieldValue( GetVariableField( entityId, name ), scrVmPub_top );
	scrVmPub_top--;
	scrVmPub_inparamcount = 0;
}

/* ---- VM_ArchiveStack  0x0047AEC0 ---- */
void VM_ArchiveStack( int count, const char *codePos, unsigned short thread,
					  unsigned short currentObject, VariableValue *stackBase,
					  unsigned int time ) {
	VariableStackBuffer    *buf;
	unsigned char          *out;
	VariableValue           value;

	/* Mac 1.3 shape */
	buf = (VariableStackBuffer *)MT_Alloc( SCR_STACK_BUFFER_SIZE( count ), SCR_STACK_MT_TYPE );
	buf->thread = thread;
	AddRefToObject( currentObject );
	buf->valueCount = (unsigned short)count;
	buf->time = time;
	buf->codePos = codePos;
	out = buf->values;
	scrVmGlob_funcCount--;
	while ( count ) {
		count--;
		*out = (unsigned char)stackBase[1].type;
		stackBase++;
		if ( stackBase->type == VAR_CODEPOS ) {
			scrVmGlob_funcCount--;
		}
		out++;
		*(VariableUnion *)out = stackBase->u;
		out += 4;
	}
	value.type = VAR_STACK;
	value.u.pointerValue = buf;
	SetNewVariableValue( GetVariable( currentObject, SCR_NAME_STACK ), &value );
}

void VM_ArchiveStack__FiPCcUsUsP13VariableValueUi( int count, const char *codePos,
		unsigned short thread, unsigned short currentObject,
		VariableValue *stackBase, unsigned int time ) {
	VM_ArchiveStack( count, codePos, thread, currentObject, stackBase, time );
}

/* ---- VM_TerminateStackInternal  0x0047AF80 ---- */
void VM_TerminateStackInternal( unsigned short id, const VariableStackBuffer *buf ) {
	const unsigned char    *p;
	int                     count;
	unsigned short          thread;
	VariableUnion           u;

	RemoveVariable( id, SCR_NAME_STACK );
	RemoveRefToObject( id );

	count  = buf->valueCount;
	thread = buf->thread;
	p      = buf->values + SCR_STACK_ENTRY_SIZE * count;

	KillThread( thread );
	RemoveRefToObject( thread );

	/* Mac 1.3 shape: step back over the payload dword, then the type byte; the
	   frame marker is the rarely taken else */
	while ( count ) {
		p -= 4;
		memcpy( &u, p, 4 );
		p--;
		count--;
		if ( *p != VAR_CODEPOS ) {
			RemoveRefToValue( *p, u );
		} else {
			p -= 4;
			memcpy( &u, p, 4 );
			p--;
			count--;
			KillThread( u.halfword.lo );
			RemoveRefToObject( u.halfword.lo );
		}
	}

	MT_Free( (void *)buf, SCR_STACK_BUFFER_SIZE( buf->valueCount ) );
}

void VM_TerminateStackInternal__FUsPC19VariableStackBuffer( unsigned short id,
		const VariableStackBuffer *buf ) {
	VM_TerminateStackInternal( id, buf );
}

/* ---- VM_TerminateStack  0x0047B1A0 ---- */
void VM_TerminateStack( unsigned short id ) {
	unsigned short child = FindVariable( id, SCR_NAME_STACK );

	if ( child != 0 ) {
		VM_TerminateStackInternal( id, (const VariableStackBuffer *)
								   scrVarNodes[child].u.pointerValue );
		return;
	}
	KillThread( id );
	RemoveRefToObject( id );
}

void VM_TerminateStack__FUs( unsigned short id ) { VM_TerminateStack( id ); }

/* ---- VM_CancelNotify  0x0047B1F0 ---- */
void VM_CancelNotify( unsigned short startLocalId, unsigned short notifyListId ) {
	unsigned short  name;
	unsigned short  waitRoot;
	unsigned short  bucket;

	name = GetThreadNotifyName( startLocalId );
	ClearThreadNotifyName( startLocalId );

	waitRoot = scrVarNodes[ FindVariable( notifyListId, SCR_NAME_NOTIFY_LIST ) ].u.halfword.lo;
	bucket   = scrVarNodes[ FindVariable( waitRoot, name ) ].u.halfword.lo;

	RemoveObjectVariable( bucket, startLocalId );

	if ( GetArraySize( bucket ) == 0 ) {
		RemoveVariable( waitRoot, name );
		if ( GetArraySize( waitRoot ) == 0 ) {
			RemoveVariable( notifyListId, SCR_NAME_NOTIFY_LIST );
		}
	}
}

void VM_CancelNotify__FUsUs( unsigned short startLocalId, unsigned short notifyListId ) {
	VM_CancelNotify( startLocalId, notifyListId );
}

/* ---- VM_Terminate  0x0047B2A0 ---- */
void VM_Terminate( unsigned short id ) {
	unsigned short          stackChild;
	VariableStackBuffer    *buf;
	unsigned short          self;
	unsigned short          pauseSlot;
	unsigned short          pauseBucket;
	unsigned short          waitEntry;
	unsigned short          timeBucket;
	unsigned int            time;

	stackChild = FindVariable( id, SCR_NAME_STACK );
	if ( stackChild == 0 ) {
		return;
	}
	buf  = (VariableStackBuffer *)scrVarNodes[stackChild].u.pointerValue;
	self = GetSelf( id );

	pauseSlot = FindVariable( scrVarPub_pauseArrayId,
							  (unsigned int)self + VAR_NAME_OBJECT_BASE );
	if ( pauseSlot != 0 ) {
		pauseBucket = scrVarNodes[pauseSlot].u.halfword.lo;
		waitEntry = FindVariable( pauseBucket, (unsigned int)id + VAR_NAME_OBJECT_BASE );
	} else {
		pauseBucket = 0;
		waitEntry = 0;
	}

	if ( waitEntry != 0 ) {
		VM_CancelNotify( id, scrVarNodes[waitEntry].u.halfword.lo );
		AddRefToObject( id );
		RemoveObjectVariable( pauseBucket, id );
		if ( GetArraySize( pauseBucket ) == 0 ) {
			RemoveObjectVariable( scrVarPub_pauseArrayId, self );
		}
		VM_TerminateStackInternal( id, buf );
		return;
	} else {
		time = buf->time;
		timeBucket = scrVarNodes[ FindVariable( scr_timeArrayId, time ) ].u.halfword.lo;
		AddRefToObject( id );
		RemoveObjectVariable( timeBucket, id );
		if ( GetArraySize( timeBucket ) == 0 && time != scrVarPub_time ) {
			RemoveVariable( scr_timeArrayId, time );
		}
	}

	VM_TerminateStackInternal( id, buf );
}

void VM_Terminate__FUs( unsigned short id ) { VM_Terminate( id ); }

/* ---- VM_TerminateNotifyList  0x0047BDC0 ---- */
void VM_TerminateNotifyList( unsigned short id ) {
	unsigned short  notifyListId;
	unsigned short  startLocalId;

	for ( ;; ) {
		notifyListId = FindVariable( id, SCR_NAME_NOTIFY_LIST );
		if ( !notifyListId ) {
			break;
		}
		notifyListId = FindNextSibling( FindObject( notifyListId ) );
		if ( !notifyListId ) {
			break;
		}
		notifyListId = FindNextSibling( FindObject( notifyListId ) );
		if ( !notifyListId ) {
			break;
		}
		startLocalId = GetVariableKeyObject( notifyListId );
		AddRefToObject( startLocalId );
		VM_TerminateStack( startLocalId );
	}
}

void VM_TerminateNotifyList__FUs( unsigned short id ) { VM_TerminateNotifyList( id ); }

/* ---- VM_TerminateTime  0x0047BF10 ---- */
void VM_TerminateTime( unsigned short bucketId ) {
	unsigned short          entry;
	unsigned short          thread;
	unsigned short          stackChild;

	AddRefToObject( bucketId );

	for ( ;; ) {
		entry = FindNextSibling( bucketId );
		if ( entry == 0 ) {
			break;
		}
		thread = VAR_NAMEOF( entry ) - VAR_NAME_OBJECT_BASE;
		AddRefToObject( thread );

		RemoveObjectVariable( bucketId, thread );

		stackChild = FindVariable( thread, SCR_NAME_STACK );
		if ( stackChild != 0 ) {
			VM_TerminateStackInternal( thread, (const VariableStackBuffer *)
									   scrVarNodes[stackChild].u.pointerValue );
		} else {
			KillThread( thread );
			RemoveRefToObject( thread );
		}
	}

	RemoveRefToObject( bucketId );
}

void VM_TerminateTime__FUs( unsigned short id ) { VM_TerminateTime( id ); }

/* ---- VM_Resume  0x0047C090 ---- */
void VM_Resume( unsigned short timeId ) {
	unsigned short              id;
	unsigned short              startLocalId;
	unsigned short              stackId;
	unsigned short              localId;
	const VariableStackBuffer  *stackValue;
	VariableValue              *top;
	const unsigned char        *buf;
	const char                 *pos;
	int                         size;

	scrVmGlob_loopTick = Scr_ReadTsc();
	AddRefToObject( timeId );

	for ( ;; ) {
		id = FindNextSibling( timeId );
		if ( !id ) {
			break;
		}
		startLocalId = GetVariableKeyObject( id );
		RemoveObjectVariable( timeId, startLocalId );

		stackId = FindVariable( startLocalId, SCR_NAME_STACK );
		stackValue = (const VariableStackBuffer *)GetVariableValueAddress( stackId )->u.pointerValue;
		RemoveVariable( startLocalId, SCR_NAME_STACK );

		localId = stackValue->thread;
		size = stackValue->valueCount;
		pos = stackValue->codePos;
		buf = stackValue->values;
		top = scrVmPub_stack;

		while ( size ) {
			size--;
			top++;
			top->type = *buf++;
			top->u.codePosValue = *(const char * const *)buf;
			buf += 4;
			if ( top->type == VAR_CODEPOS ) {
				scrVmGlob_callStack[scrVmGlob_funcCount++] = top->u.codePosValue;
			}
		}
		scrVmGlob_callStack[scrVmGlob_funcCount++] = pos;

		MT_Free( (void *)stackValue, SCR_STACK_BUFFER_SIZE( stackValue->valueCount ) );

		if ( IsFieldObject( GetSelf( localId ) ) ) {
			RemoveRefToObject( VM_Execute( top, pos, localId, startLocalId, scrVmPub_stack ) );
			RemoveRefToValue( scrVmPub_stack[1].type, scrVmPub_stack[1].u );
		} else {
			for ( ;; ) {
				KillThread( localId );
				RemoveRefToObject( localId );

				while ( top->type != VAR_CODEPOS ) {
					RemoveRefToValue__FP13VariableValue( top );
					top--;
				}
				scrVmGlob_funcCount--;
				if ( top == scrVmPub_stack ) {
					break;
				}
				top--;
				localId = top->u.halfword.lo;
				top--;
			}
		}
	}

	RemoveRefToObject( timeId );
	ClearVariableValue( scrVarPub_tempVariable );
}

void VM_Resume__FUs( unsigned short id ) { VM_Resume( id ); }

/* ---- VM_Notify  0x0047B430 ---- VERIFIED */
void VM_Notify( unsigned short objectId, unsigned short name, VariableValue *params ) {
	unsigned short          notifyListId;
	unsigned short          bucket;
	unsigned short          entry;
	unsigned short          thread;
	unsigned short          self;
	unsigned short          pauseBucket;
	unsigned short          stackChild;
	VariableValueInternal  *stackEntry;
	VariableStackBuffer    *buf;
	VariableStackBuffer    *grown;
	VariableValue          *p;
	VariableValue           archived;
	VariableValue           supplied;
	const unsigned char    *pat;
	unsigned char          *out;
	bool                    noParams;
	int                     size;
	int                     newCount;
	int                     len;

	notifyListId = FindVariable( objectId, SCR_NAME_NOTIFY_LIST );
	if ( !notifyListId ) {
		return;
	}
	notifyListId = FindObject( notifyListId );

	bucket = FindVariable( notifyListId, name );
	if ( !bucket ) {
		return;
	}
	bucket = FindObject( bucket );
	AddRefToObject( bucket );

	for ( ;; ) {
		entry = FindNextSibling( bucket );
next:
		if ( !entry ) {
			break;
		}
		thread = GetVariableKeyObject( entry );
		self = GetSelf( thread );
		pauseBucket = FindObject( FindObjectVariable( scrVarPub_pauseArrayId, self ) );

		stackChild = FindVariable( thread, SCR_NAME_STACK );
		if ( !stackChild ) {
			ClearThreadNotifyName( thread );
			AddRefToObject( thread );
			RemoveObjectVariable( bucket, thread );
			if ( !GetArraySize( bucket ) ) {
				RemoveVariable( notifyListId, name );
				if ( !GetArraySize( notifyListId ) ) {
					RemoveVariable( objectId, SCR_NAME_NOTIFY_LIST );
				}
			}
			RemoveObjectVariable( pauseBucket, thread );
			if ( !GetArraySize( pauseBucket ) ) {
				RemoveObjectVariable( scrVarPub_pauseArrayId, self );
			}
			AddRefToObject( self );
			KillThread( thread );
			RemoveRefToObject( thread );
			VM_Terminate( self );
			RemoveRefToObject( self );
			continue;
		}

		stackEntry = &scrVarNodes[stackChild];
		buf = (VariableStackBuffer *)stackEntry->u.pointerValue;

		if ( buf->codePos[-1] == 79 ) {
			size = (signed char)buf->codePos[0];
			pat = buf->values + SCR_STACK_ENTRY_SIZE * ( buf->valueCount - size );

			for ( p = params; size; p-- ) {
				if ( p->type == VAR_CODEPOS ) {
					goto skip;
				}
				size--;
				archived.type = *pat;
				if ( archived.type == VAR_CODEPOS ) {
					break;
				}
				pat++;
				archived.u.intValue = *(const int *)pat;
				pat += 4;
				AddRefToValue( archived.type, archived.u );

				supplied = *p;
				AddRefToValue( supplied.type, supplied.u );

				/* retail 0047B5E8: CheckEquality( left = archived, right = supplied );
				   the equality result lands in archived */
				if ( !CheckEquality( &archived, &supplied ) ) {
					RuntimeError( (char *)scrVmGlob_dialog_error_message,
								  buf->codePos,
								  (signed char)buf->codePos[0] - size + 2,
								  scrVarPub_error_message );
					scrVarPub_error_message = NULL;
					scrVmGlob_dialog_error_message = NULL;
					goto skip;
				}
				if ( !archived.u.intValue ) {
					goto skip;
				}
			}
			buf->codePos++;
			noParams = true;
		} else {
			noParams = params->type == VAR_CODEPOS;
		}

		ClearThreadNotifyName( thread );
		RemoveObjectVariable( bucket, thread );
		if ( !GetArraySize( bucket ) ) {
			RemoveVariable( notifyListId, name );
			if ( !GetArraySize( notifyListId ) ) {
				RemoveVariable( objectId, SCR_NAME_NOTIFY_LIST );
			}
		}

		buf->time = scrVarPub_time;
		GetObjectVariable( GetArray( GetVariable( scr_timeArrayId, scrVarPub_time ) ),
						   thread );

		RemoveObjectVariable( pauseBucket, thread );
		if ( !GetArraySize( pauseBucket ) ) {
			RemoveObjectVariable( scrVarPub_pauseArrayId, self );
		}

		if ( !noParams ) {
			size = buf->valueCount;
			p = params;
			newCount = size;
			do {
				newCount++;
				p--;
			} while ( p->type != VAR_CODEPOS );

			len = SCR_STACK_ENTRY_SIZE * size;
			if ( !MT_Realloc( len + SCR_STACK_HEADER_SIZE,
							  SCR_STACK_BUFFER_SIZE( newCount ) ) ) {
				grown = (VariableStackBuffer *)MT_Alloc( SCR_STACK_BUFFER_SIZE( newCount ),
														 SCR_STACK_MT_TYPE );
				grown->time    = buf->time;
				grown->codePos = buf->codePos;
				grown->thread  = buf->thread;
				Com_Memcpy( grown->values, buf->values, len );
				MT_Free( buf, len + SCR_STACK_HEADER_SIZE );
				buf = grown;
				stackEntry->u.pointerValue = buf;
			}
			buf->valueCount = (unsigned short)newCount;

			out = buf->values + len;
			newCount -= size;
			do {
				p++;
				AddRefToValue( p->type, p->u );
				*out = (unsigned char)p->type;
				out++;
				*(VariableUnion *)out = p->u;
				out += 4;
			} while ( --newCount );
		}
		continue;
skip:
		entry = FindNextSibling( entry );
		goto next;
	}

	RemoveRefToObject( bucket );
}

void VM_Notify__FUsUsP13VariableValue( unsigned short objectId, unsigned short name,
									   VariableValue *params ) {
	VM_Notify( objectId, name, params );
}

/* ---- Scr_NotifyId  0x0047B890 ---- */
void Scr_NotifyId( unsigned short id, unsigned short name, unsigned int paramCount ) {
	VariableValue  *base;
	int             savedType;
	int             savedIn;

	while (scrVmPub_outparamcount) { RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u ); --scrVmPub_top; --scrVmPub_outparamcount; }

	base    = scrVmPub_top - paramCount;
	savedIn = scrVmPub_inparamcount - (int)paramCount;

	savedType  = base->type;
	base->type = VAR_CODEPOS;
	scrVmPub_inparamcount = 0;

	VM_Notify( id, name, scrVmPub_top );

	base->type = savedType;
	while ( scrVmPub_top != base ) {
		RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u );
		scrVmPub_top--;
	}
	scrVmPub_inparamcount = savedIn;
}

/* ---- Scr_NotifyNum  0x0047BB20 ---- */
void Scr_NotifyNum( int entnum, int classnum, unsigned short name,
					unsigned int paramCount ) {
	VariableValue  *base;
	unsigned short  id;
	int             savedType;
	int             savedIn;

	while (scrVmPub_outparamcount) { RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u ); --scrVmPub_top; --scrVmPub_outparamcount; }

	base    = scrVmPub_top - paramCount;
	savedIn = scrVmPub_inparamcount - (int)paramCount;

	id = FindEntityId( entnum, classnum );
	if ( id != 0 ) {
		savedType  = base->type;
		base->type = VAR_CODEPOS;
		scrVmPub_inparamcount = 0;
		VM_Notify( id, name, scrVmPub_top );
		base->type = savedType;
	}

	while ( scrVmPub_top != base ) {
		RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u );
		scrVmPub_top--;
	}
	scrVmPub_inparamcount = savedIn;
}

/* ---- VM_SetTime  0x0047CB90 ---- */
void VM_SetTime( void ) {
	unsigned short child;

	if ( scr_timeArrayId == 0 ) {
		return;
	}
	child = FindVariable( scr_timeArrayId, scrVarPub_time );
	if ( child == 0 ) {
		return;
	}
	VM_Resume( scrVarNodes[child].u.halfword.lo );
	SafeRemoveVariable( scr_timeArrayId, scrVarPub_time );
}

/* ---- Scr_RunCurrentThreads  0x0047E340 ---- */
void Scr_RunCurrentThreads( void ) {
	VM_SetTime();
}

/* ---- Scr_SetTime  0x0047E250 ---- */
void Scr_SetTime(int time) {
 unsigned short child;
 time &= 0xFFFFFF;
 if((int)((unsigned int)time-scrVarPub_time)>0) {
  do {
   if(scr_timeArrayId != 0) {
    child=FindVariable(scr_timeArrayId,scrVarPub_time);
    if(child != 0) {
     VM_Resume(scrVarNodes[child].u.halfword.lo);
     SafeRemoveVariable(scr_timeArrayId,scrVarPub_time);
    }
   }
   ++scrVarPub_time;
   scrVarPub_time &= 0xFFFFFF;
  } while(scrVarPub_time != (unsigned int)time);
 } else scrVarPub_time=(unsigned int)time;
}

/* ---- Scr_ResetTimeout  0x0047E350 ---- */
void Scr_ResetTimeout( void ) {
	scrVmGlob_loopTick = Scr_ReadTsc();
}

typedef enum {
	OP_End                          = 0,
	OP_Return                       = 1,
	OP_GetUndefined                 = 2,
	OP_GetInteger                   = 3,
	OP_GetFloat                     = 4,
	OP_GetString                    = 5,
	OP_GetIString                   = 6,
	OP_GetSelfObject                = 7,
	OP_GetLevelObject               = 8,
	OP_GetAnimObject                = 9,
	OP_GetSelf                      = 10,
	OP_GetLevel                     = 11,
	OP_GetGame                      = 12,
	OP_GetAnim                      = 13,
	OP_GetAnimation                 = 14,
	OP_GetGameRef                   = 15,
	OP_GetFunction                  = 16,
	OP_GetLocal                     = 17,
	OP_SetLocalRef                  = 18,
	OP_ClearLocal                   = 19,
	OP_EvalArray                    = 20,
	OP_EvalArrayRef                 = 21,
	OP_EvalArrayRefNoCreate         = 22,
	OP_ClearArray                   = 23,
	OP_EmptyArray                   = 24,
	OP_GetField                     = 25,
	OP_SetFieldRef                  = 26,
	OP_ClearField                   = 27,
	OP_StoreTemp                    = 28,
	OP_SetLocal                     = 29,
	OP_SetLocalAndClear             = 30,
	OP_ClearParams                  = 31,
	OP_CheckClearParams             = 32,
	OP_StoreRef                     = 33,
	OP_CallBuiltin                  = 34,
	OP_CallBuiltinMethod            = 35,
	OP_Wait                         = 36,
	OP_ThreadMarker                 = 37,
	OP_CallFunction                 = 38,
	OP_CallPointer                  = 39,
	OP_MethodCallFunction           = 40,
	OP_MethodCallPointer            = 41,
	OP_ThreadCallFunction           = 42,
	OP_ThreadCallPointer            = 43,
	OP_MethodThreadCallFunction     = 44,
	OP_MethodThreadCallPointer      = 45,
	OP_DecTop                       = 46,
	OP_CastFieldObject              = 47,
	OP_CastBool                     = 48,
	OP_CastInt                      = 49,
	OP_CastFloat                    = 50,
	OP_CastString                   = 51,
	OP_BoolNot                      = 52,
	OP_BitNot                       = 53,
	OP_JumpOnFalse                  = 54,
	OP_JumpOnTrueBack               = 55,
	OP_JumpOnFalseExpr              = 56,
	OP_JumpOnTrueExpr               = 57,
	OP_Jump                         = 58,
	OP_JumpBack                     = 59,
	OP_Inc                          = 60,
	OP_Dec                          = 61,
	OP_BitOr                        = 62,
	OP_BitXor                       = 63,
	OP_BitAnd                       = 64,
	OP_Equality                     = 65,
	OP_Inequality                   = 66,
	OP_Less                         = 67,
	OP_Greater                      = 68,
	OP_LessEqual                    = 69,
	OP_GreaterEqual                 = 70,
	OP_ShiftLeft                    = 71,
	OP_ShiftRight                   = 72,
	OP_Plus                         = 73,
	OP_Minus                        = 74,
	OP_Multiply                     = 75,
	OP_Divide                       = 76,
	OP_Modulus                      = 77,
	OP_SizeOf                       = 78,
	OP_WaitTillMatch                = 79,
	OP_WaitTill                     = 80,
	OP_Notify                       = 81,
	OP_EndOn                        = 82,
	OP_VoidCodePos                  = 83,
	OP_Switch                       = 84,
	OP_EndSwitch                    = 85,
	OP_Vector                       = 86,
	OP_NOP                          = 87,
	OP_DevBlockBegin                = 88,
	OP_DevBlockEnd                  = 89
} scrOpcode_t;

typedef void (*scrBuiltinFunc_t)( void );
typedef void (*scrBuiltinMethod_t)( int entnum );

#define SCR_OPERAND_CODEPOS     4
#define SCR_OPERAND_CALL        8
#define SCR_OPERAND_POINTERCALL 4
#define SCR_SWITCH_ENTRY        8

/* ---- VM_Execute's inline expansions ----
   Retail's VM_Execute carries these helpers expanded in place, dozens of times over,
   while every other caller keeps the out-of-line function.  VC7 inlines nothing that
   makes a call into a C++ try block, and a called static __forceinline function leaves
   a dead out-of-line copy behind, so the expansions are spelled as macros and used only
   here.  Each one is the named function's body. */

extern const char      *scrMemTree_constBase;
extern int              scrMemTree_constLen;

/* FastRound 0x00408370 (client_mp/cl_console_mp.c): fld / fadd qword 2^-30 / fistp */
#define VM_FAST_ROUND( out, value ) \
	{ float vmF_ = ( value ); double vmBias_ = 9.313225746154785e-10; int vmI_; \
	  { __asm fld vmF_ __asm fadd vmBias_ __asm fistp vmI_ } \
	  ( out ) = vmI_; }

/* Scr_ErrorNoArg_m 0x00475980 / Scr_Error 0x0047DF40.  The no-argument half is a real
   inline function, not a macro: retail's 57 exception temporaries come from inline
   expansion, so they sit below VM_Execute's own locals in the frame (opcode at
   [ebp-11h], the temporaries from [ebp-12h] down in source order).  VC7 inlines it
   into the try block only when forced; it leaves a dead out-of-line copy. */
static __forceinline void VM_ErrorNoArg( void ) {
	if ( SCR_ERROR_CAUGHT() ) {
		SCR_THROW_ERROR();
	}
	Com_Error( ERR_DROP, "\x15%s", scrVarPub_error_message );
}
#define VM_ERROR_NOARG()    VM_ErrorNoArg()
#define VM_ERROR( msg ) \
	{ scrVarPub_error_message = ( msg ); VM_ERROR_NOARG(); }

/* RemoveRefToObject 0x004716B0, with ClearObject 0x00471010 */
#define VM_REMOVE_REF_TO_OBJECT( objectId ) \
	{ unsigned short vmObj_ = ( objectId ); \
	  VariableValueInternal *vmEntry_ = &scrVarNodes[vmObj_]; \
	  if ( vmEntry_->u.halfword.lo ) { vmEntry_->u.halfword.lo--; } \
	  else { scrVarNodes[vmObj_].u.halfword.lo++; ClearObjectInternal( vmObj_ ); \
	         RemoveRefToObject( vmObj_ ); FreeVariable( vmObj_ ); } }

/* RemoveRefToValue reading the value directly (no type/union copies) */
#define VM_REMOVE_REF_TO_VALUE_D( value ) \
	{ switch ( ( value )->type ) { \
	  case VAR_OBJECT: VM_REMOVE_REF_TO_OBJECT( ( value )->u.halfword.lo ); break; \
	  case VAR_STRING: case VAR_ISTRING: VM_SL_REMOVE_REF_TO_STRING( ( value )->u.stringValue ); break; \
	  case VAR_VECTOR: VM_REMOVE_REF_TO_VECTOR( ( value )->u.vectorValue ); break; \
	  default: break; } }

/* SL_RemoveRefToString 0x00470050 */
#define VM_SL_REMOVE_REF_TO_STRING( stringValue ) \
	{ unsigned short vmStr_ = ( stringValue ); \
	  scrStringEntry_t *vmRef_ = SCR_REFSTRING( vmStr_ ); \
	  if ( vmRef_->refCount ) { vmRef_->refCount--; } \
	  else { SL_FreeString( vmStr_, vmRef_->text, (unsigned int)strlen( vmRef_->text ) + 1 ); } }

/* RemoveRefToVector 0x00471770, with MT_Free 0x0046E4C0 */
#define VM_REMOVE_REF_TO_VECTOR( vectorValue ) \
	{ const float *vmVec_ = ( vectorValue ); \
	  if ( (unsigned int)( (const char *)vmVec_ - scrMemTree_constBase ) >= (unsigned int)scrMemTree_constLen ) { \
		unsigned short vmRefs_ = ( (unsigned short *)vmVec_ )[-1]; \
		if ( vmRefs_ ) { ( (unsigned short *)vmVec_ )[-1] = vmRefs_ - 1; } \
		else { MT_FreeIndex( (unsigned short)( (scrMemBlock_t *)( (unsigned short *)vmVec_ - 1 ) - scrMemTree_blocks ), \
		                     SCR_VECTOR_ALLOC_SIZE ); } } }

/* RemoveRefToValue 0x00471820 */
#define VM_REMOVE_REF_TO_VALUE( value ) \
	{ int vmType_ = ( value )->type; VariableUnion vmU_ = ( value )->u; \
	  switch ( vmType_ ) { \
	  case VAR_OBJECT: VM_REMOVE_REF_TO_OBJECT( vmU_.halfword.lo ); break; \
	  case VAR_STRING: case VAR_ISTRING: VM_SL_REMOVE_REF_TO_STRING( vmU_.stringValue ); break; \
	  case VAR_VECTOR: VM_REMOVE_REF_TO_VECTOR( vmU_.vectorValue ); break; \
	  default: break; } }

/* the same, with only the switch expanded */
#define VM_REMOVE_REF_TO_VALUE_CALLS( value ) \
	{ int vmType_ = ( value )->type; VariableUnion vmU_ = ( value )->u; \
	  switch ( vmType_ ) { \
	  case VAR_OBJECT: RemoveRefToObject( vmU_.halfword.lo ); break; \
	  case VAR_STRING: case VAR_ISTRING: SL_RemoveRefToString( vmU_.stringValue ); break; \
	  case VAR_VECTOR: RemoveRefToVector( vmU_.vectorValue ); break; \
	  default: break; } }

/* the same, with RemoveRefToObject left out of line */
#define VM_REMOVE_REF_TO_VALUE_OBJCALL( value ) \
	{ int vmType_ = ( value )->type; VariableUnion vmU_ = ( value )->u; \
	  switch ( vmType_ ) { \
	  case VAR_OBJECT: RemoveRefToObject( vmU_.halfword.lo ); break; \
	  case VAR_STRING: case VAR_ISTRING: VM_SL_REMOVE_REF_TO_STRING( vmU_.stringValue ); break; \
	  case VAR_VECTOR: VM_REMOVE_REF_TO_VECTOR( vmU_.vectorValue ); break; \
	  default: break; } }

/* GetVariable 0x00471B40 / GetObjectVariable 0x00471B60 */
#define VM_GET_VARIABLE( parentId, name ) \
	( scrVarIndirections[ GetVariableIndexInternal( ( parentId ), ( name ) ) ].id )
#define VM_GET_OBJECT_VARIABLE( parentId, objectId ) \
	VM_GET_VARIABLE( parentId, (unsigned int)( objectId ) + VAR_NAME_OBJECT_BASE )

/* SafeRemoveVariable 0x00471CD0 */
#define VM_SAFE_REMOVE_VARIABLE( parentId, name ) \
	{ unsigned short vmParent_ = ( parentId ); unsigned int vmName_ = ( name ); \
	  unsigned short vmHandle_ = FindVariableIndexInternal( vmParent_, vmName_ ); \
	  if ( vmHandle_ ) { Variable *vmSlot_ = &scrVarIndirections[vmHandle_]; \
		VariableValueInternal *vmChild_ = &scrVarNodes[vmSlot_->id]; \
		MakeVariableExternal( vmSlot_, &scrVarNodes[ vmParent_ ] ); \
		FreeValueInternal( vmChild_ ); } }

/* GetVariableValue 0x00472020 */
#define VM_GET_VARIABLE_VALUE( variableId, value ) \
	{ VariableValueInternal *vmVar_ = &scrVarNodes[ ( variableId ) ]; \
	  ( value )->type = (int)( vmVar_->status & VAR_STATUS_TYPE_MASK ); \
	  ( value )->u = vmVar_->u; }

/* SetThreadNotifyName 0x00471060 */
#define VM_SET_THREAD_NOTIFY_NAME( threadId, name ) \
	{ SL_AddRefToString( ( name ) ); \
	  scrVarNodes[ ( threadId ) ].status |= (unsigned int)( name ) << VAR_STATUS_NAME_SHIFT; }

/* IsFieldObject 0x00472310, through a node pointer as retail inlines it */
#define VM_IS_FIELD_OBJECT( objectId ) \
	( entryValue = &scrVarNodes[ ( objectId ) ], ( entryValue->status & VAR_STATUS_TYPE_MASK ) < VAR_ARRAY )

#define VM_NODE_INDEX( node )   ( (unsigned short)( ( node ) - scrVarNodes ) )

/* AllocThread 0x004715C0 / Scr_AllocArray 0x00471580 / AllocObject 0x00471510 */
#define VM_ALLOC_THREAD( out, self ) \
	{ VariableValueInternal *vmNode_ = AllocVariable(); \
	  vmNode_->status = VAR_STAT_EXTERNAL | VAR_THREAD; \
	  vmNode_->u.halfword.lo = 0; vmNode_->u.halfword.hi = ( self ); \
	  ( out ) = VM_NODE_INDEX( vmNode_ ); }
/* the same, with the self store written first: retail's OP_ThreadCallPointer copy
   stores hi before status/lo (0x00477C6D), the other copies status first */
#define VM_ALLOC_THREAD_SELF_FIRST( out, self ) \
	{ VariableValueInternal *vmNode_ = AllocVariable(); \
	  vmNode_->u.halfword.hi = ( self ); \
	  vmNode_->status = VAR_STAT_EXTERNAL | VAR_THREAD; \
	  vmNode_->u.halfword.lo = 0; \
	  ( out ) = VM_NODE_INDEX( vmNode_ ); }
#define VM_ALLOC_ARRAY( out ) \
	{ VariableValueInternal *vmNode_ = AllocVariable(); \
	  vmNode_->status = VAR_STAT_EXTERNAL | VAR_ARRAY; \
	  vmNode_->u.halfword.lo = 0; vmNode_->u.halfword.hi = 0; \
	  ( out ) = VM_NODE_INDEX( vmNode_ ); }
#define VM_ALLOC_OBJECT( out ) \
	{ VariableValueInternal *vmNode_ = AllocVariable(); \
	  vmNode_->status = VAR_STAT_EXTERNAL | VAR_STRUCT; \
	  vmNode_->u.halfword.lo = 0; \
	  ( out ) = VM_NODE_INDEX( vmNode_ ); }

/* GetArray 0x004722A0 / GetObject 0x00472250 */
#define VM_GET_ARRAY( out, id ) \
	{ VariableValueInternal *vmArray_ = &scrVarNodes[ ( id ) ]; \
	  if ( ( vmArray_->status & VAR_STATUS_TYPE_MASK ) == VAR_UNDEFINED ) { \
		vmArray_->status |= VAR_OBJECT; VM_ALLOC_ARRAY( vmArray_->u.halfword.lo ); } \
	  ( out ) = vmArray_->u.halfword.lo; }
#define VM_GET_OBJECT( out, id ) \
	{ VariableValueInternal *vmObject_ = &scrVarNodes[ ( id ) ]; \
	  if ( ( vmObject_->status & VAR_STATUS_TYPE_MASK ) == VAR_UNDEFINED ) { \
		vmObject_->status |= VAR_OBJECT; VM_ALLOC_OBJECT( vmObject_->u.halfword.lo ); } \
	  ( out ) = vmObject_->u.halfword.lo; }

/* VM_ReleaseInParams: the builtin's parameters, through scrVmPub_top */
#define VM_RELEASE_IN_PARAMS() \
	while ( scrVmPub_outparamcount ) { \
		VM_REMOVE_REF_TO_VALUE( scrVmPub_top ); \
		scrVmPub_top--; \
		scrVmPub_outparamcount--; }

#define VM_STACK_OVERFLOW_MESSAGE   "script stack overflow (too many embedded function calls)"

#ifdef __cplusplus
#define VM_VOLATILE
#else
#define VM_VOLATILE             volatile
#endif

/* ---- VM_Execute  0x00476370 ----
   Retail's shape: a try block around the dispatch loop and an empty catch, with the
   error recovery after it.  An error check is written with the success path first
   and the Scr_Error at the end of the case, so it runs on into the next case label
   (it never returns: Com_Error longjmps); likewise a failed opcode's handler below
   the catch.
   A few cases end in `goto next_opcode` instead of `continue`: the same target, but
   VC7 then lets the cases that share an identical tail keep retail's copy of it
   (cross-jumping), and register allocation follows retail more closely.  OP_End
   releases its values through VM_REMOVE_REF_TO_VALUE_D (no type/union copies), and
   OP_SetLocal / OP_SetLocalAndClear call GetVariable (inlined) instead of expanding
   VM_GET_VARIABLE: both measured closer to retail's register assignment. */
static unsigned short VM_Execute( VariableValue * VM_VOLATILE top, const char * VM_VOLATILE codePos,
								  unsigned short VM_VOLATILE thread, unsigned short currentObject,
								  VariableValue *stackBase ) {
	VM_VOLATILE int             devDepth = 0;
	VM_VOLATILE char            opcode;
	VM_VOLATILE unsigned short  fieldValueId;
	VM_VOLATILE int             caseCount;
	VariableValue               tempValue;
	VariableValueInternal      *entryValue;
	VariableValue              *startTop;
	const char                 *target;
	unsigned short              id;
	unsigned int                caseValue;
	int                         type;
	int                         count;
	float                      *vec;
	float                       waitTime;

	VM_ERROR_FRAME_DECL

	VM_ERROR_FRAME_PUSH();

	for ( ;; ) {
		VM_TRY {
		for ( ;; ) {
	next_opcode:
		opcode = *codePos++;

	interrupt_return:
		switch ( opcode ) {

		case OP_End:
			KillThread( thread );
			while ( top->type != VAR_CODEPOS ) {
				VM_REMOVE_REF_TO_VALUE_D( top );
				top--;
			}
			scrVmGlob_funcCount--;
			if ( top == stackBase ) {
				stackBase[1].type = VAR_UNDEFINED;
				VM_ERROR_FRAME_POP();
				return thread;
			}
			VM_REMOVE_REF_TO_OBJECT( thread );
			codePos = top->u.codePosValue;
			top--;
			thread = top->u.halfword.lo;
			top->type = VAR_UNDEFINED;
			/* retail 0x00476593 / 0x00476754: a caller whose self is no longer a
			   field object is killed, and so is every frame below it down to
			   stackBase -- no further self check is made on the lower frames. */
			if ( VM_IS_FIELD_OBJECT( GetSelf( thread ) ) ) {
				continue;
			}
			goto thread_end;

		case OP_Return:
			KillThread( thread );
			tempValue = *top;
			while ( ( --top )->type != VAR_CODEPOS ) {
				VM_REMOVE_REF_TO_VALUE( top );
			}
			scrVmGlob_funcCount--;
			if ( top == stackBase ) {
				stackBase[1] = tempValue;
				VM_ERROR_FRAME_POP();
				return thread;
			}
			VM_REMOVE_REF_TO_OBJECT( thread );
			codePos = top->u.codePosValue;
			top--;
			thread = top->u.halfword.lo;
			*top = tempValue;
			if ( VM_IS_FIELD_OBJECT( GetSelf( thread ) ) ) {
				continue;
			}
			goto thread_end;

		case OP_GetUndefined:
			top++;
			top->type = VAR_UNDEFINED;
			continue;

		case OP_GetInteger:
			top++;
			top->type = VAR_INTEGER;
			top->u.intValue = *(const int *)codePos;
			codePos += 4;
			continue;

		case OP_GetFloat:
			top++;
			top->type = VAR_FLOAT;
			top->u.intValue = *(const int *)codePos;
			codePos += 4;
			continue;

		case OP_GetString:
			top++;
			top->type = VAR_STRING;
			top->u.stringValue = *(const unsigned short *)codePos;
			SL_AddRefToString( top->u.stringValue );
			codePos += 2;
			continue;

		case OP_GetIString:
			top++;
			top->type = VAR_ISTRING;
			top->u.stringValue = *(const unsigned short *)codePos;
			SL_AddRefToString( top->u.stringValue );
			codePos += 2;
			continue;

		case OP_GetSelfObject:
			fieldValueId = GetSelf( thread );
			continue;

		case OP_GetLevelObject:
			fieldValueId = scrVarPub_levelId;
			continue;

		case OP_GetAnimObject:
			fieldValueId = scrVarPub_animId;
			continue;

		case OP_GetSelf:
			top++;
			top->type = VAR_OBJECT;
			top->u.halfword.lo = GetSelf( thread );
			AddRefToObject( top->u.halfword.lo );
			goto next_opcode;

		case OP_GetLevel:
			top++;
			top->type = VAR_OBJECT;
			top->u.halfword.lo = scrVarPub_levelId;
			AddRefToObject( scrVarPub_levelId );
			goto next_opcode;

		case OP_GetGame:
			top++;
			VM_GET_VARIABLE_VALUE( scrVarPub_gameId.handle, top );
			AddRefToValue( top->type, top->u );
			continue;

		case OP_GetAnim:
			top++;
			top->type = VAR_OBJECT;
			top->u.halfword.lo = scrVarPub_animId;
			AddRefToObject( scrVarPub_animId );
			continue;

		case OP_GetAnimation:
			top++;
			top->type = VAR_ANIMATION;
			top->u.intValue = *(const int *)codePos;
			codePos += 4;
			continue;

		case OP_GetGameRef:
			fieldValueId = scrVarPub_gameId.handle;
			continue;

		case OP_GetFunction:
			top++;
			top->type = VAR_FUNCTION;
			top->u.intValue = *(const int *)codePos;
			codePos += 4;
			continue;

		case OP_GetLocal:
			top++;
			VM_GET_VARIABLE_VALUE( VM_GET_VARIABLE( thread, *(const unsigned short *)codePos ), top );
			AddRefToValue( top->type, top->u );
			codePos += 2;
			continue;

		case OP_SetLocalRef:
			fieldValueId = VM_GET_VARIABLE( thread, *(const unsigned short *)codePos );
			codePos += 2;
			continue;

		case OP_ClearLocal:
			VM_SAFE_REMOVE_VARIABLE( thread, *(const unsigned short *)codePos );
			codePos += 2;
			continue;

		case OP_EvalArray:
			/* retail 0x004769E0: the pop is committed before the call, so a throw
			   inside EvalArray reaches the catch with top on the lower (index) slot */
			top--;
			EvalArray( top, top + 1 );
			continue;

		case OP_EvalArrayRef:
			fieldValueId = EvalArrayRef( fieldValueId, top );
			top--;
			goto next_opcode;

		case OP_EvalArrayRefNoCreate:
			fieldValueId = Scr_EvalArrayIndex_m( fieldValueId, top );
			top--;
			goto next_opcode;

		case OP_ClearArray:
			ClearArray( fieldValueId, top );
			top--;
			continue;

		case OP_EmptyArray:
			top++;
			top->type = VAR_OBJECT;
			VM_ALLOC_ARRAY( top->u.halfword.lo );
			continue;

		case OP_GetField:
			/* retail 0x00476A81: the push is committed before both calls and codePos
			   steps over the name only after they return */
			top++;
			GetVariableFieldValue( GetVariableField( fieldValueId, *(const unsigned short *)codePos ), top );
			codePos += 2;
			continue;

		case OP_SetFieldRef:
			fieldValueId = GetVariableField( fieldValueId, *(const unsigned short *)codePos );
			codePos += 2;
			continue;

		case OP_ClearField:
			ClearVariableField( fieldValueId, *(const unsigned short *)codePos );
			codePos += 2;
			continue;

		case OP_StoreTemp:
			fieldValueId = scrVarPub_tempVariable;
			SetVariableValue( scrVarPub_tempVariable, top );
			top--;
			continue;

		case OP_SetLocal:
			if ( top->type != VAR_CODEPOS ) {
				SetVariableFieldValue( GetVariable( thread, *(const unsigned short *)codePos ), top );
				top--;
			}
			codePos += 2;
			continue;

		case OP_SetLocalAndClear:
			if ( top->type != VAR_CODEPOS ) {
				SetVariableFieldValue( GetVariable( thread, *(const unsigned short *)codePos ), top );
				top--;
			} else {
				VM_SAFE_REMOVE_VARIABLE( thread, *(const unsigned short *)codePos );
			}
			codePos += 2;
			continue;

		case OP_ClearParams:
			while ( top->type != VAR_CODEPOS ) {
				VM_REMOVE_REF_TO_VALUE( top );
				top--;
			}
			continue;

		case OP_CheckClearParams:
			if ( top->type == VAR_CODEPOS ) {
				continue;
			}
			VM_ERROR( "function called with too many parameters" );

		case OP_StoreRef:
			SetVariableFieldValue( fieldValueId, top );
			top--;
			continue;

		case OP_CallBuiltin:
			/* retail 0x00476D34: codePos steps over the count byte before the call
			   and over the pointer only after it returns */
			scrVmPub_outparamcount = *(const unsigned char *)codePos;
			codePos++;
			scrVmPub_top = top;
			( *(const scrBuiltinFunc_t *)codePos )();
			codePos += sizeof( scrBuiltinFunc_t );
			top = scrVmPub_top;
			if ( scrVmPub_outparamcount ) {
				count = scrVmPub_outparamcount;
				scrVmPub_outparamcount = 0;
				scrVmPub_top -= count;
				do {
					VM_REMOVE_REF_TO_VALUE( top );
					top--;
				} while ( --count );
			}
			goto builtin_return;

		case OP_CallBuiltinMethod:
			/* retail 0x00476E6A: codePos steps over the count byte before the entity
			   checks and over the pointer only after the call returns */
			scrVmPub_outparamcount = *(const unsigned char *)codePos;
			codePos++;
			scrVmPub_top = top - 1;
			if ( top->type == VAR_OBJECT ) {
				fieldValueId = top->u.halfword.lo;
				if ( VAR_TYPEOF( fieldValueId ) == VAR_ENTITY ) {
					caseCount = GetEntnum( fieldValueId );
					VM_REMOVE_REF_TO_OBJECT( fieldValueId );
					( *(const scrBuiltinMethod_t *)codePos )( caseCount );
					codePos += sizeof( scrBuiltinMethod_t );
					top = scrVmPub_top;
					if ( scrVmPub_outparamcount ) {
						count = scrVmPub_outparamcount;
						scrVmPub_outparamcount = 0;
						scrVmPub_top -= count;
						do {
							VM_REMOVE_REF_TO_VALUE_OBJCALL( top );
							top--;
						} while ( --count );
					}
builtin_return:
					if ( scrVmPub_inparamcount ) {
						scrVmPub_inparamcount = 0;
					} else {
						top++;
						top->type = VAR_UNDEFINED;
					}
					continue;
				}
				caseCount = VAR_TYPEOF( fieldValueId );
				VM_REMOVE_REF_TO_OBJECT( fieldValueId );
				scrVarPub_error_index = -1;
				VM_ERROR( va( "%s is not an entity", var_typename[caseCount] ) );
			}
			caseCount = top->type;
			VM_REMOVE_REF_TO_VALUE_OBJCALL( top );
			scrVarPub_error_index = -1;
			VM_ERROR( va( "%s is not an entity", var_typename[caseCount] ) );

		case OP_Wait:
			if ( devDepth ) {
				VM_ERROR( "wait not allowed in /# ... #/ comment (call as a thread to fix)" );
			}
			if ( CastFloat( top ) ) {
				waitTime = top->u.floatValue;
				/* retail 0x004771D4: fcomp 0.0f / test ah,1 -- NaN is reported as a
				   negative wait */
				if ( waitTime >= 0.0f ) {
					if ( waitTime < (float)SCR_MAX_WAIT_SECONDS ) {
						VM_FAST_ROUND( caseCount, waitTime * SCR_WAIT_SCALE );
						caseCount = ( caseCount + scrVarPub_time ) & SCR_TIME_MASK;
						VM_GET_OBJECT_VARIABLE( GetArray( VM_GET_VARIABLE( scr_timeArrayId, caseCount ) ),
												currentObject );
						top--;
						VM_ArchiveStack( top - stackBase, codePos, thread, currentObject, stackBase, caseCount );
						stackBase[1].type = VAR_UNDEFINED;
						VM_ERROR_FRAME_POP();
						return currentObject;
					}
					VM_ERROR( va( "wait of %.0f seconds is too long", waitTime ) );
				}
				VM_ERROR( va( "negative wait of %g is not allowed", waitTime ) );
			}
			VM_ERROR_NOARG();

		case OP_ThreadMarker:
			AddRefToObject( thread );
			top++;
			top->type = VAR_OBJECT;
			top->u.halfword.lo = thread;
			top++;
			top->type = VAR_UNDEFINED;
			continue;

		case OP_CallFunction:
			VM_REMOVE_REF_TO_OBJECT( thread );
			/* retail 0x0047747A: no error_index; the depth check runs before the self
			   reference is taken, so an overflow leaks no reference */
			if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
				scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;
				fieldValueId = GetSelf( thread );
				AddRefToObject( fieldValueId );
				thread = AllocThread( fieldValueId );
				( top - *(const int *)( codePos + 4 ) )->type = VAR_CODEPOS;
				( top - *(const int *)( codePos + 4 ) )->u.codePosValue = codePos + 8;
				codePos = *(const char * const *)codePos;
				continue;
			}
			VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );

		case OP_CallPointer:
			VM_REMOVE_REF_TO_OBJECT( thread );
			if ( top->type == VAR_FUNCTION ) {
				if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
					scrVmGlob_callStack[ scrVmGlob_funcCount ] = codePos;
					target = top->u.codePosValue;
					scrVmGlob_funcCount++;
					top--;
					fieldValueId = GetSelf( thread );
					AddRefToObject( fieldValueId );
					thread = AllocThread( fieldValueId );
					( top - *(const int *)codePos )->type = VAR_CODEPOS;
					( top - *(const int *)codePos )->u.codePosValue = codePos + 4;
					codePos = target;
					continue;
				}
				scrVarPub_error_index = 1;
				VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );
			}
			VM_ERROR( va( "%s is not a function pointer", var_typename[top->type] ) );

		case OP_MethodCallFunction:
			VM_REMOVE_REF_TO_OBJECT( thread );
			/* retail 0x00477664: the first "is not an object" check leaves
			   error_index alone; only the IsFieldObject failure sets it */
			if ( top->type == VAR_OBJECT ) {
				if ( VM_IS_FIELD_OBJECT( top->u.halfword.lo ) ) {
					if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
						scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;
						/* still the macro: AllocThread() here (as retail) flips VM_Execute's
						   whole register assignment (comparator 0.917 -> 0.834) */
						VM_ALLOC_THREAD( thread, top->u.halfword.lo );
						top--;
						( top - *(const int *)( codePos + 4 ) )->type = VAR_CODEPOS;
						( top - *(const int *)( codePos + 4 ) )->u.codePosValue = codePos + 8;
						codePos = *(const char * const *)codePos;
						continue;
					}
					VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );
				}
				scrVarPub_error_index = 1;
				VM_ERROR( va( "%s is not an object", var_typename[ VAR_TYPEOF( top->u.halfword.lo ) ] ) );
			}
			VM_ERROR( va( "%s is not an object", var_typename[top->type] ) );

		case OP_MethodCallPointer:
			VM_REMOVE_REF_TO_OBJECT( thread );
			if ( top->type == VAR_FUNCTION ) {
				/* retail 0x0047783F: the pop of the function pointer is committed
				   before the object and depth checks */
				target = top->u.codePosValue;
				top--;
				if ( top->type == VAR_OBJECT ) {
					if ( VM_IS_FIELD_OBJECT( top->u.halfword.lo ) ) {
						if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
							scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;
							thread = AllocThread( top->u.halfword.lo );
							top--;
							( top - *(const int *)codePos )->type = VAR_CODEPOS;
							( top - *(const int *)codePos )->u.codePosValue = codePos + 4;
							codePos = target;
							continue;
						}
						scrVarPub_error_index = 1;
						VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );
					}
					scrVarPub_error_index = 2;
					VM_ERROR( va( "%s is not an object", var_typename[ VAR_TYPEOF( top->u.halfword.lo ) ] ) );
				}
				scrVarPub_error_index = 2;
				VM_ERROR( va( "%s is not an object", var_typename[top->type] ) );
			}
			VM_REMOVE_REF_TO_VALUE( top );
			top--;
			VM_ERROR( va( "%s is not a function pointer", var_typename[top[1].type] ) );

		case OP_ThreadCallFunction:
			if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
				scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;
				fieldValueId = GetSelf( thread );
				AddRefToObject( fieldValueId );
				fieldValueId = AllocThread( fieldValueId );
				startTop = top - *(const int *)( codePos + 4 );
				caseCount = startTop->type;
				startTop->type = VAR_CODEPOS;
				id = VM_Execute( top, *(const char * const *)codePos, fieldValueId, fieldValueId, startTop );
				VM_REMOVE_REF_TO_OBJECT( id );
				startTop->type = caseCount;
				top = startTop + 1;
				codePos += 8;
				goto next_opcode;
			}
			scrVarPub_error_index = 1;
			VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );

		case OP_ThreadCallPointer:
			if ( top->type == VAR_FUNCTION ) {
				if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
					scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;
					target = top->u.codePosValue;
					top--;
					fieldValueId = GetSelf( thread );
					AddRefToObject( fieldValueId );
					fieldValueId = AllocThread( fieldValueId );
					startTop = top - *(const int *)codePos;
					caseCount = startTop->type;
					startTop->type = VAR_CODEPOS;
					id = VM_Execute( top, target, fieldValueId, fieldValueId, startTop );
					VM_REMOVE_REF_TO_OBJECT( id );
					startTop->type = caseCount;
					top = startTop + 1;
					codePos += 4;
					continue;
				}
				scrVarPub_error_index = 1;
				VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );
			}
			VM_ERROR( va( "%s is not a function pointer", var_typename[top->type] ) );

		case OP_MethodThreadCallFunction:
			if ( top->type == VAR_OBJECT ) {
				if ( VM_IS_FIELD_OBJECT( top->u.halfword.lo ) ) {
					if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
						scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;
						fieldValueId = AllocThread( top->u.halfword.lo );
						top--;
						startTop = top - *(const int *)( codePos + 4 );
						caseCount = startTop->type;
						startTop->type = VAR_CODEPOS;
						id = VM_Execute( top, *(const char * const *)codePos, fieldValueId, fieldValueId, startTop );
						VM_REMOVE_REF_TO_OBJECT( id );
						startTop->type = caseCount;
						top = startTop + 1;
						codePos += 8;
						continue;
					}
					scrVarPub_error_index = 1;
					VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );
				}
				scrVarPub_error_index = 2;
				VM_ERROR( va( "%s is not an object", var_typename[ VAR_TYPEOF( top->u.halfword.lo ) ] ) );
			}
			scrVarPub_error_index = 2;
			VM_ERROR( va( "%s is not an object", var_typename[top->type] ) );

		case OP_MethodThreadCallPointer:
			if ( top->type == VAR_FUNCTION ) {
				/* retail 0x00477F69: the pop of the function pointer is committed
				   before the object and depth checks */
				target = top->u.codePosValue;
				top--;
				if ( top->type == VAR_OBJECT ) {
					if ( VM_IS_FIELD_OBJECT( top->u.halfword.lo ) ) {
						if ( scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH ) {
							scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;
							fieldValueId = AllocThread( top->u.halfword.lo );
							top--;
							startTop = top - *(const int *)codePos;
							caseCount = startTop->type;
							startTop->type = VAR_CODEPOS;
							id = VM_Execute( top, target, fieldValueId, fieldValueId, startTop );
							VM_REMOVE_REF_TO_OBJECT( id );
							startTop->type = caseCount;
							top = startTop + 1;
							codePos += 4;
							continue;
						}
						scrVarPub_error_index = 1;
						VM_ERROR( VM_STACK_OVERFLOW_MESSAGE );
					}
					scrVarPub_error_index = 2;
					VM_ERROR( va( "%s is not an object", var_typename[ VAR_TYPEOF( top->u.halfword.lo ) ] ) );
				}
				scrVarPub_error_index = 2;
				VM_ERROR( va( "%s is not an object", var_typename[top->type] ) );
			}
			VM_REMOVE_REF_TO_VALUE( top );
			top--;
			VM_ERROR( va( "%s is not a function pointer", var_typename[top[1].type] ) );

		case OP_DecTop:
			VM_REMOVE_REF_TO_VALUE( top );
			top--;
			continue;

		case OP_CastFieldObject:
			fieldValueId = CastFieldObject( top );
			top--;
			continue;

		case OP_CastBool:
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue = top->u.intValue != 0;
				continue;
			}
			if ( CastBool( top ) ) {
				continue;
			}
			VM_ERROR_NOARG();

		case OP_CastInt:
			if ( CastInt( top ) ) {
				continue;
			}
			VM_ERROR_NOARG();

		case OP_CastFloat:
			if ( CastFloat( top ) ) {
				continue;
			}
			VM_ERROR_NOARG();

		case OP_CastString:
			if ( CastString( top ) ) {
				continue;
			}
			VM_ERROR_NOARG();

		case OP_BoolNot:
			if ( top->type == VAR_INTEGER || CastBool( top ) ) {
				top->u.intValue = top->u.intValue == 0;
				continue;
			}
			VM_ERROR_NOARG();

		case OP_BitNot:
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue = ~top->u.intValue;
				continue;
			}
			VM_ERROR( va( "~ cannot be applied to \"%s\"", var_typename[top->type] ) );

		case OP_JumpOnFalse:
			if ( top->type == VAR_INTEGER || CastBool( top ) ) {
				if ( !top->u.intValue ) {
					codePos += *(const int *)codePos;
				}
				codePos += 4;
				top--;
				continue;
			}
			VM_ERROR_NOARG();

		case OP_JumpOnTrueBack:
			if ( top->type == VAR_INTEGER || CastBool( top ) ) {
				if ( top->u.intValue ) {
					top--;
					goto jump_back;
				}
				codePos += 4;
				top--;
				continue;
			}
			scrVarPub_error_index = 1;
			VM_ERROR_NOARG();

		case OP_JumpOnFalseExpr:
			if ( top->type == VAR_INTEGER || CastBool( top ) ) {
				if ( top->u.intValue ) {
					top--;
				} else {
					codePos += *(const int *)codePos;
				}
				goto jump_expr_next;
			}
			VM_ERROR_NOARG();

		case OP_JumpOnTrueExpr:
			if ( top->type == VAR_INTEGER || CastBool( top ) ) {
				if ( top->u.intValue ) {
					codePos += *(const int *)codePos;
				} else {
					top--;
				}
			jump_expr_next:
				codePos += 4;
				continue;
			}
			VM_ERROR_NOARG();

		case OP_Jump:
			codePos += *(const int *)codePos;
			continue;

		case OP_JumpBack:
		jump_back:
			/* retail 0x00478688: the budget is the sign bit of a 32-bit rdtsc
			   difference -- nothing is added at any arming site */
			if ( (int)( Scr_ReadTsc() - scrVmGlob_loopTick ) >= 0 ) {
				codePos += *(const int *)codePos;
				continue;
			}
			if ( scr_loading ) {
				Com_Printf( "WARNING: potential infinite loop in script.\n" );
				Scr_PrintPrevCodePos( codePos, 0 );
				codePos += *(const int *)codePos;
				scrVmGlob_loopTick = Scr_ReadTsc();
				continue;
			}
			if ( !SCR_DEVELOPER ) {
				Com_Printf( "ERROR: potential infinite loop in script - killing thread.\n" );
				Scr_PrintPrevCodePos( codePos, 0 );
				scrVmGlob_loopTick = Scr_ReadTsc();

		thread_end:
				/* retail 0x004787CA..0x00478844: shared by this path and by OP_End /
				   OP_Return when the caller's self is dead.  Every frame down to
				   stackBase is killed, with no self check on the lower frames. */
				for ( ;; ) {
					KillThread( thread );
					while ( top->type != VAR_CODEPOS ) {
						RemoveRefToValue__FP13VariableValue( top );
						top--;
					}
					scrVmGlob_funcCount--;
					if ( top == stackBase ) {
						stackBase[1].type = VAR_UNDEFINED;
						VM_ERROR_FRAME_POP();
						return thread;
					}
					RemoveRefToObject( thread );
					codePos = top->u.codePosValue;
					top--;
					thread = top->u.halfword.lo;
					top->type = VAR_UNDEFINED;
				}
			}
			/* Scr_TerminalError 0x0047DFD0 */
			Scr_DumpScriptThreads();
			scrVmPub_terminal_error = 1;
			VM_ERROR( "potential infinite loop in script" );

		case OP_Inc:
			/* retail 0x00478890: the push is committed before GetVariableFieldValue */
			top++;
			GetVariableFieldValue( fieldValueId, top );
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue++;
				continue;
			}
			VM_ERROR( va( "++ must be applied to an int (applied to %s)", var_typename[top->type] ) );

		case OP_Dec:
			top++;
			GetVariableFieldValue( fieldValueId, top );
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue--;
				continue;
			}
			VM_ERROR( va( "-- must be applied to an int (applied to %s)", var_typename[top->type] ) );

		case OP_BitOr:
			top--;
			CastWeakerPair( top, top + 1 );
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue |= top[1].u.intValue;
				continue;
			}
			goto unmatching_types;

		case OP_BitXor:
			top--;
			CastWeakerPair( top, top + 1 );
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue ^= top[1].u.intValue;
				continue;
			}
			goto unmatching_types;

		case OP_BitAnd:
			top--;
			CastWeakerPair( top, top + 1 );
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue &= top[1].u.intValue;
				continue;
			}
			goto unmatching_types;

		case OP_Equality:
			top--;
			if ( CheckEquality( top, top + 1 ) ) {
				continue;
			}
			VM_ERROR_NOARG();

		case OP_Inequality:
			top--;
			if ( CastWeakerPair( top, top + 1 ) ) {
				switch ( top->type ) {
				case VAR_UNDEFINED:
					top->type = VAR_INTEGER;
					top->u.intValue = 0;
					break;
				case VAR_INTEGER:
					top->u.intValue = top->u.intValue != top[1].u.intValue;
					break;
				case VAR_FLOAT:
					top->type = VAR_INTEGER;
					top->u.intValue = CM_Q_fabs( top->u.floatValue - top[1].u.floatValue ) >= SCR_FLOAT_EQ_EPSILON;
					break;
				case VAR_STRING:
				case VAR_ISTRING:
					caseCount = top->u.stringValue != top[1].u.stringValue;
					top->type = VAR_INTEGER;
					SL_RemoveRefToString( top->u.stringValue );
					SL_RemoveRefToString( top[1].u.stringValue );
					top->u.intValue = caseCount;
					break;
				case VAR_VECTOR:
					top->type = VAR_INTEGER;
					caseCount = top->u.vectorValue[0] != top[1].u.vectorValue[0]
							 || top->u.vectorValue[1] != top[1].u.vectorValue[1]
							 || top->u.vectorValue[2] != top[1].u.vectorValue[2];
					RemoveRefToVector( top->u.vectorValue );
					RemoveRefToVector( top[1].u.vectorValue );
					top->u.intValue = caseCount;
					break;
				case VAR_OBJECT:
					caseCount = top->u.halfword.lo != top[1].u.halfword.lo;
					top->type = VAR_INTEGER;
					RemoveRefToObject( top->u.halfword.lo );
					RemoveRefToObject( top[1].u.halfword.lo );
					top->u.intValue = caseCount;
					break;
				case VAR_ANIMATION:
					top->type = VAR_INTEGER;
					top->u.intValue = top->u.intValue != top[1].u.intValue;
					break;
				default:
					goto unmatching_types;
				}
				continue;
			}
			goto unmatching_types;

		case OP_Less:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				top->u.intValue = top->u.intValue < top[1].u.intValue;
				goto next_opcode;
			case VAR_FLOAT:
				top->type = VAR_INTEGER;
				top->u.intValue = top->u.floatValue < top[1].u.floatValue;
				goto next_opcode;
			default:
				goto unmatching_types;
			}

		case OP_Greater:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				top->u.intValue = top->u.intValue > top[1].u.intValue;
				goto next_opcode;
			case VAR_FLOAT:
				top->type = VAR_INTEGER;
				top->u.intValue = top->u.floatValue > top[1].u.floatValue;
				goto next_opcode;
			default:
				goto unmatching_types;
			}

		case OP_LessEqual:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				top->u.intValue = top->u.intValue <= top[1].u.intValue;
				goto next_opcode;
			case VAR_FLOAT:
				top->type = VAR_INTEGER;
				top->u.intValue = top->u.floatValue <= top[1].u.floatValue;
				goto next_opcode;
			default:
				goto unmatching_types;
			}

		case OP_GreaterEqual:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				top->u.intValue = top->u.intValue >= top[1].u.intValue;
				continue;
			case VAR_FLOAT:
				top->type = VAR_INTEGER;
				top->u.intValue = top->u.floatValue >= top[1].u.floatValue;
				continue;
			default:
				goto unmatching_types;
			}

		case OP_ShiftLeft:
			top--;
			CastWeakerPair( top, top + 1 );
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue <<= top[1].u.intValue;
				continue;
			}
			goto unmatching_types;

		case OP_ShiftRight:
			top--;
			CastWeakerPair( top, top + 1 );
			if ( top->type == VAR_INTEGER ) {
				top->u.intValue >>= top[1].u.intValue;
				continue;
			}
			goto unmatching_types;

		case OP_Plus:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				top->u.intValue += top[1].u.intValue;
				goto next_opcode;
			case VAR_FLOAT:
				top->u.floatValue += top[1].u.floatValue;
				goto next_opcode;
			case VAR_STRING: {
				/* block-scoped here and in WaitTill/EndOn: retail packs these with count */
				unsigned short stringValue = VM_ConcatenateStrings( top );
				VM_SL_REMOVE_REF_TO_STRING( top->u.stringValue );
				VM_SL_REMOVE_REF_TO_STRING( top[1].u.stringValue );
				top->u.stringValue = stringValue;
				goto next_opcode;
			}
			case VAR_VECTOR:
				/* AllocVector 0x00471700, with MT_Alloc 0x0046E4A0 */
				vec = (float *)( (unsigned short *)&scrMemTree_blocks[ MT_AllocIndex( SCR_VECTOR_ALLOC_SIZE, 2 ) ] + 1 );
				( (unsigned short *)vec )[-1] = 0;
				vec[0] = top->u.vectorValue[0] + top[1].u.vectorValue[0];
				vec[1] = top->u.vectorValue[1] + top[1].u.vectorValue[1];
				vec[2] = top->u.vectorValue[2] + top[1].u.vectorValue[2];
				VM_REMOVE_REF_TO_VECTOR( top->u.vectorValue );
				VM_REMOVE_REF_TO_VECTOR( top[1].u.vectorValue );
				top->u.vectorValue = vec;
				goto next_opcode;
			default:
				goto unmatching_types;
			}

		case OP_Minus:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				top->u.intValue -= top[1].u.intValue;
				continue;
			case VAR_FLOAT:
				top->u.floatValue -= top[1].u.floatValue;
				continue;
			case VAR_VECTOR:
				vec = (float *)( (unsigned short *)&scrMemTree_blocks[ MT_AllocIndex( SCR_VECTOR_ALLOC_SIZE, 2 ) ] + 1 );
				( (unsigned short *)vec )[-1] = 0;
				vec[0] = top->u.vectorValue[0] - top[1].u.vectorValue[0];
				vec[1] = top->u.vectorValue[1] - top[1].u.vectorValue[1];
				vec[2] = top->u.vectorValue[2] - top[1].u.vectorValue[2];
				VM_REMOVE_REF_TO_VECTOR( top->u.vectorValue );
				VM_REMOVE_REF_TO_VECTOR( top[1].u.vectorValue );
				top->u.vectorValue = vec;
				continue;
			default:
				goto unmatching_types;
			}

		case OP_Multiply:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				top->u.intValue *= top[1].u.intValue;
				continue;
			case VAR_FLOAT:
				top->u.floatValue *= top[1].u.floatValue;
				continue;
			default:
				goto unmatching_types;
			}

		case OP_Divide:
			top--;
			CastWeakerPair( top, top + 1 );
			switch ( top->type ) {
			case VAR_INTEGER:
				if ( top[1].u.intValue ) {
					top->u.intValue /= top[1].u.intValue;
					continue;
				}
				top->u.intValue = 0;
				VM_ERROR( "divide by 0" );
			case VAR_FLOAT:
				if ( top[1].u.floatValue != 0.0f ) {
					top->u.floatValue /= top[1].u.floatValue;
					continue;
				}
				top->u.intValue = 0;
				VM_ERROR( "divide by 0" );
			default:
				goto unmatching_types;
			}

		case OP_Modulus:
			top--;
			CastWeakerPair( top, top + 1 );
			if ( top->type == VAR_INTEGER ) {
				if ( top[1].u.intValue ) {
					top->u.intValue %= top[1].u.intValue;
					continue;
				}
				top->u.intValue = 0;
				VM_ERROR( "divide by 0" );
			}
			goto unmatching_types;

		case OP_SizeOf:
			GetSizeValue( top );
			continue;

		case OP_WaitTillMatch:
		case OP_WaitTill:
			if ( devDepth ) {
				VM_ERROR( "waittill not allowed in /# ... #/ comment (call as a thread to fix)" );
			}
			if ( top->type == VAR_OBJECT ) {
				id = top->u.halfword.lo;
				if ( VM_IS_FIELD_OBJECT( id ) ) {
					if ( top[-1].type == VAR_STRING ) {
						unsigned short stringValue = top[-1].u.stringValue;
						id = GetArray( VM_GET_VARIABLE( GetArray( VM_GET_VARIABLE( id, SCR_NAME_NOTIFY_LIST ) ),
														stringValue ) );
						VM_GET_OBJECT_VARIABLE( id, currentObject );
						tempValue.type = VAR_OBJECT;
						tempValue.u.halfword.lo = top->u.halfword.lo;
						SetNewVariableValue( VM_GET_OBJECT_VARIABLE(
							GetArray( VM_GET_OBJECT_VARIABLE( scrVarPub_pauseArrayId, GetSelf( currentObject ) ) ),
							currentObject ), &tempValue );
						VM_SET_THREAD_NOTIFY_NAME( currentObject, stringValue );
						VM_SL_REMOVE_REF_TO_STRING( stringValue );
						top -= 2;
						VM_ArchiveStack( top - stackBase, codePos, thread, currentObject, stackBase, 0 );
						stackBase[1].type = VAR_UNDEFINED;
						VM_ERROR_FRAME_POP();
						return currentObject;
					}
					scrVarPub_error_index = 1;
					VM_ERROR( "first parameter of waittill must evaluate to a string" );
				}
				scrVarPub_error_index = 2;
				VM_ERROR( va( "%s is not an object", var_typename[ VAR_TYPEOF( top->u.halfword.lo ) ] ) );
			}
			scrVarPub_error_index = 2;
			VM_ERROR( va( "%s is not an object", var_typename[top->type] ) );

		case OP_Notify:
			if ( top->type == VAR_OBJECT ) {
				id = top->u.halfword.lo;
				if ( VM_IS_FIELD_OBJECT( id ) ) {
					if ( top[-1].type == VAR_STRING ) {
						VM_Notify( id, top[-1].u.stringValue, top - 2 );
						while ( top->type != VAR_CODEPOS ) {
							VM_REMOVE_REF_TO_VALUE( top );
							top--;
						}
						top--;
						continue;
					}
					scrVarPub_error_index = 1;
					VM_ERROR( "first parameter of notify must evaluate to a string" );
				}
				scrVarPub_error_index = 2;
				VM_ERROR( va( "%s is not an object", var_typename[ VAR_TYPEOF( top->u.halfword.lo ) ] ) );
			}
			scrVarPub_error_index = 2;
			VM_ERROR( va( "%s is not an object", var_typename[top->type] ) );

		case OP_EndOn:
			if ( top->type == VAR_OBJECT ) {
				/* IsFieldObject through the node pointer, like the other cases: retail keeps
				   the (dead) lea of the node here too */
				if ( VM_IS_FIELD_OBJECT( top->u.halfword.lo ) ) {
					if ( top[-1].type == VAR_STRING ) {
						fieldValueId = top[-1].u.stringValue;
						AddRefToObject( currentObject );
						id = AllocThread( currentObject );
						VM_GET_ARRAY( type, VM_GET_VARIABLE( top->u.halfword.lo, SCR_NAME_NOTIFY_LIST ) );
						VM_GET_ARRAY( type, VM_GET_VARIABLE( type, fieldValueId ) );
						VM_GET_OBJECT_VARIABLE( type, id );
						VM_REMOVE_REF_TO_OBJECT( id );
						tempValue.type = VAR_OBJECT;
						tempValue.u.halfword.lo = top->u.halfword.lo;
						VM_GET_ARRAY( type, VM_GET_OBJECT_VARIABLE( scrVarPub_pauseArrayId, currentObject ) );
						SetNewVariableValue( VM_GET_OBJECT_VARIABLE( type, id ), &tempValue );
						VM_SET_THREAD_NOTIFY_NAME( id, fieldValueId );
						VM_SL_REMOVE_REF_TO_STRING( fieldValueId );
						top -= 2;
						continue;
					}
					/* retail 0x00479978: string 0x0055ABA4 reads "endof" */
					VM_ERROR( "first parameter of endof must evaluate to a string" );
				}
				scrVarPub_error_index = 1;
				VM_ERROR( va( "%s is not an object", var_typename[ VAR_TYPEOF( top->u.halfword.lo ) ] ) );
			}
			scrVarPub_error_index = 1;
			VM_ERROR( va( "%s is not an object", var_typename[top->type] ) );

		case OP_VoidCodePos:
			top++;
			top->type = VAR_CODEPOS;
			top->u.intValue = 0;
			continue;

		case OP_Switch:
			codePos += *(const short *)codePos;
			caseCount = *(const short *)( codePos - 2 );
			switch ( top->type ) {
			default:
				VM_ERROR( va( "cannot switch on %s", var_typename[top->type] ) );
				break;
			case VAR_INTEGER:
				if ( IsValidArrayIndex( top->u.intValue ) ) {
					caseValue = GetInternalVariableIndex( top->u.intValue );
					break;
				}
				VM_ERROR( va( "switch index %d out of range", top->u.intValue ) );
			case VAR_STRING:
				caseValue = top->u.stringValue;
				VM_SL_REMOVE_REF_TO_STRING( top->u.stringValue );
				break;
			}
			top--;
			if ( caseCount ) {
				do {
					if ( *(const unsigned int *)codePos == caseValue ) {
						codePos = *(const char * const *)( codePos + 4 );
						goto next_opcode;
					}
					codePos += SCR_SWITCH_ENTRY;
				} while ( --caseCount );
				if ( !*(const unsigned int *)( codePos - SCR_SWITCH_ENTRY ) ) {
					codePos = *(const char * const *)( codePos - 4 );
				}
			}
			continue;

		case OP_EndSwitch:
			codePos += *(const short *)codePos * SCR_SWITCH_ENTRY + 2;
			continue;

		case OP_Vector:
			top -= 2;
			CastVector2( top );
			continue;

		case OP_NOP:
			continue;

		case OP_DevBlockBegin:
			/* retail 0x00479C0D: patch[0] holds the displaced opcode (written by
			   Scr_InsertDeveloperOpcodes); the dev code starts at patch + 1 */
			scrVmGlob_devCallStack[ scrVmGlob_funcCount ] = codePos;
			target = (const char *)SCR_DEV_PATCH_TABLE[ codePos - SCR_PROGRAM_BASE - 1 ];
			scrVmGlob_devOpcode[ scrVmGlob_funcCount ] = *target;
			codePos = target + 1;
			devDepth++;
			continue;

		case OP_DevBlockEnd:
			codePos = scrVmGlob_devCallStack[ scrVmGlob_funcCount ];
			opcode = scrVmGlob_devOpcode[ scrVmGlob_funcCount ];
			scrVmGlob_devCallStack[ scrVmGlob_funcCount ] = NULL;
			devDepth--;
			goto interrupt_return;

		/* retail 0x0047912E: there is no "bad opcode" error; an out-of-range
		   opcode takes the shared unmatching-types error */
		default:
		unmatching_types:
			if ( !scrVarPub_error_message ) {
				UnmatchingTypesError( top, top + 1 );
			}
			VM_ERROR_NOARG();
		}
		break;
		}
		}
		VM_CATCH

		/* retail 0x00479C97: a terminal error is reported before any per-opcode
		   cleanup, with the unadjusted codePos and error_index (RuntimeError ends
		   in Com_Error when terminal_error is set) */
		if ( scrVmPub_terminal_error ) {
			RuntimeError( scrVmGlob_dialog_error_message, codePos, scrVarPub_error_index,
						  scrVarPub_error_message );
		}

		switch ( opcode ) {
		case OP_EvalArrayRef:
		case OP_EvalArrayRefNoCreate:
			ClearVariableValue( scrVarPub_tempVariable );
			fieldValueId = scrVarPub_tempVariable;
		case OP_ClearArray:
			if ( scrVarPub_error_index < 0 ) {
				scrVarPub_error_index = 1;
			}
			VM_REMOVE_REF_TO_VALUE( top );
			top--;
			break;

		case OP_CastFieldObject:
			VM_REMOVE_REF_TO_VALUE( top );
			ClearVariableValue( scrVarPub_tempVariable );
			VM_GET_OBJECT( fieldValueId, scrVarPub_tempVariable );
			top--;
			break;

		case OP_SetLocal:
		case OP_SetLocalAndClear:
			VM_RELEASE_IN_PARAMS();
			top--;
		case OP_GetField:
		case OP_ClearField:
			scrVarPub_error_index = 0;
			codePos += 2;
			break;

		case OP_StoreRef:
			VM_RELEASE_IN_PARAMS();
			scrVarPub_error_index = 0;
			top--;
			break;

		/* retail 0x0047A157 / 0x0047A288: the main path already stepped over the
		   count byte before any throw point, so only the pointer is skipped here */
		case OP_CallBuiltin:
			if ( scrVarPub_error_index > 0 ) {
				scrVarPub_error_index = scrVmPub_outparamcount - scrVarPub_error_index + 1;
			}
			codePos += sizeof( scrBuiltinFunc_t );
			VM_RELEASE_IN_PARAMS();
			top = scrVmPub_top + 1;
			top->type = VAR_UNDEFINED;
			break;

		case OP_CallBuiltinMethod:
			if ( scrVarPub_error_index > 0 ) {
				scrVarPub_error_index = scrVmPub_outparamcount - scrVarPub_error_index + 2;
			} else if ( scrVarPub_error_index < 0 ) {
				scrVarPub_error_index = 1;
			}
			codePos += sizeof( scrBuiltinMethod_t );
			VM_RELEASE_IN_PARAMS();
			top = scrVmPub_top + 1;
			top->type = VAR_UNDEFINED;
			break;

		case OP_Inc:
		case OP_Dec:
			scrVarPub_error_index = 0;
			break;

		case OP_SizeOf:
			VM_REMOVE_REF_TO_VALUE_CALLS( top );
			top->type = VAR_UNDEFINED;
			break;

		case OP_EvalArray:
			VM_REMOVE_REF_TO_VALUE_CALLS( top );
			VM_REMOVE_REF_TO_VALUE_CALLS( top + 1 );
			top->type = VAR_UNDEFINED;
			break;

		case OP_CheckClearParams:
			do {
				VM_REMOVE_REF_TO_VALUE( top );
				top--;
			} while ( top->type != VAR_CODEPOS );
			break;

		/* retail 0x0047A546 / 0x0047A655 / 0x0047A658: release the args (plus the
		   object or function pointer), then pop one more slot so the stack ends on
		   the ThreadMarker thread slot and mark it undefined -- the slot a normal
		   Return would have overwritten with the result */
		case OP_CallFunction:
			codePos += 4;
			scrVmPub_outparamcount = *(const int *)codePos;
			codePos += 4;
			while ( scrVmPub_outparamcount ) {
				VM_REMOVE_REF_TO_VALUE( top );
				top--;
				scrVmPub_outparamcount--;
			}
			top--;
			top->type = VAR_UNDEFINED;
			break;

		case OP_MethodCallFunction:
			codePos += 4;
		case OP_CallPointer:
		case OP_MethodCallPointer:
			scrVmPub_outparamcount = *(const int *)codePos + 1;
			codePos += 4;
			while ( scrVmPub_outparamcount ) {
				VM_REMOVE_REF_TO_VALUE( top );
				top--;
				scrVmPub_outparamcount--;
			}
			top--;
			top->type = VAR_UNDEFINED;
			break;

		/* retail 0x0047A768: thread calls leave the result slot one above the
		   released values */
		case OP_MethodThreadCallFunction:
			codePos += 4;
		case OP_ThreadCallPointer:
		case OP_MethodThreadCallPointer:
			scrVmPub_outparamcount = *(const int *)codePos + 1;
			codePos += 4;
			while ( scrVmPub_outparamcount ) {
				VM_REMOVE_REF_TO_VALUE( top );
				top--;
				scrVmPub_outparamcount--;
			}
			top++;
			top->type = VAR_UNDEFINED;
			break;

		case OP_Wait:
			scrVarPub_error_index = 1;
			top--;
			break;

		case OP_BitNot:
			RemoveRefToValue__FP13VariableValue( top );
			top->type = VAR_UNDEFINED;
			break;

		case OP_JumpOnFalse:
		case OP_JumpOnTrueBack:
		case OP_JumpOnFalseExpr:
		case OP_JumpOnTrueExpr:
			codePos += 4;
			top--;
			break;

		/* retail 0x0047A89E: the main path already lowered top by two before
		   CastVector2, so the three components start at top */
		case OP_Vector:
			ClearVector( top );
			break;

		/* retail 0x0047A8AC: one handler for all three; codePos is left as is, so
		   a WaitTillMatch error resumes at its count byte */
		case OP_WaitTillMatch:
		case OP_WaitTill:
		case OP_EndOn:
			RemoveRefToValue__FP13VariableValue( top );
			top--;
			RemoveRefToValue__FP13VariableValue( top );
			top--;
			break;

		case OP_Notify:
			while ( top->type != VAR_CODEPOS ) {
				VM_REMOVE_REF_TO_VALUE_OBJCALL( top );
				top--;
			}
			RemoveRefToValue__FP13VariableValue( top );
			top--;
			break;

		/* retail 0x0047A97C: every throw point has codePos == the jump table; skip
		   the table so execution resumes after the switch */
		case OP_Switch:
			RemoveRefToValue__FP13VariableValue( top );
			codePos += caseCount * SCR_SWITCH_ENTRY;
			top--;
			break;

		default:
			break;
		}

		RuntimeError( scrVmGlob_dialog_error_message, codePos, scrVarPub_error_index,
					  scrVarPub_error_message );
		scrVarPub_error_message = NULL;
		scrVmGlob_dialog_error_message = NULL;
		scrVarPub_error_index = 0;
	}
}

unsigned short VM_Execute__FP13VariableValuePCcUsUsP13VariableValue(
		VariableValue *stackTop, const char *codePos, unsigned short thread,
		unsigned short currentObject, VariableValue *stackBase ) {
	return VM_Execute( stackTop, codePos, thread, currentObject, stackBase );
}

/* ---- VM_execute  0x0047C5D0 ---- */
unsigned short VM_execute( unsigned short self, const char *codePos, int paramCount ) {
	VariableValue  *base;
	unsigned short  thread;
	int             savedIn;
	int             savedType;

	while (scrVmPub_outparamcount) { RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u ); --scrVmPub_top; --scrVmPub_outparamcount; }

	base    = scrVmPub_top - paramCount;
	savedIn = scrVmPub_inparamcount - paramCount;

	AddRefToObject( self );
	thread = AllocThread( self );

	if (scrVmGlob_funcCount < SCR_VM_MAX_CALL_DEPTH) {
	scrVmGlob_callStack[ scrVmGlob_funcCount++ ] = codePos;

	savedType  = base->type;
	base->type = VAR_CODEPOS;
	scrVmPub_inparamcount = 0;

	thread = VM_Execute( scrVmPub_top, codePos, thread, thread, base );

	base->type = savedType;
	scrVmPub_top = base + 1;
	scrVmPub_inparamcount = savedIn + 1;
	ClearVariableValue( scrVarPub_tempVariable );
	return thread;

	} else {
		KillThread( thread );
		scrVmPub_inparamcount = savedIn + 1;
		/* Mac 1.3 loop: the saved count itself is consumed */
		while ( savedIn ) {
			RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u );
			--savedIn;
			scrVmPub_top--;
		}
		scrVmPub_top++;
		scrVmPub_top->type = VAR_UNDEFINED;
		RuntimeError( NULL, codePos, 0,
					  "script stack overflow (too many embedded function calls)" );
		return thread;
	
	}
}

static void VM_ArmTimeout( void ) {
	if ( scrVmGlob_funcCount == 0 ) {
		scrVmGlob_loopTick = Scr_ReadTsc();
	}
}

static __forceinline void VM_DiscardResult( void ) {
	RemoveRefToValue( scrVmPub_top->type, scrVmPub_top->u );
	scrVmPub_top->type = VAR_UNDEFINED;
	scrVmPub_top--;
	scrVmPub_inparamcount--;
}

/* ---- Scr_ExecThread  0x0047C8E0 ---- */
unsigned short Scr_ExecThread( int posOffset, int paramCount ) {
	unsigned short thread;

	VM_ArmTimeout();
	thread = VM_execute( scrVarPub_levelId, SCR_PROGRAM_BASE + posOffset, paramCount );
	VM_DiscardResult();
	return thread;
}

/* ---- Scr_ExecEntThreadNum  0x0047C990 ---- */
unsigned short Scr_ExecEntThreadNum( int entnum, int classnum, int posOffset,
									 int paramCount ) {
	unsigned short thread;
	unsigned short entityId;

	VM_ArmTimeout();
	thread = VM_execute( Scr_GetEntityId(entnum,classnum), SCR_PROGRAM_BASE + posOffset, paramCount );
	VM_DiscardResult();
	return thread;
}

/* ---- Scr_AddExecThread  0x0047CA40 ---- */
void Scr_AddExecThread( int posOffset, int paramCount ) {
	unsigned short thread;

	VM_ArmTimeout();
	thread = VM_execute( scrVarPub_levelId, SCR_PROGRAM_BASE + posOffset, paramCount );
	RemoveRefToObject( thread );
}

/* ---- Scr_AddExecEntThreadNum  0x0047CAC0 ---- */
void Scr_AddExecEntThreadNum( int entnum, int classnum, int posOffset, int paramCount ) {
	unsigned short thread;
	unsigned short entityId;

	VM_ArmTimeout();
	thread = VM_execute( Scr_GetEntityId(entnum,classnum), SCR_PROGRAM_BASE + posOffset, paramCount );
	RemoveRefToObject( thread );
}

/* ---- Scr_FreeThread  0x0047CB40 ---- */
void Scr_FreeThread( unsigned short thread ) {
	RemoveRefToObject( thread );
}

/* ---- Scr_InitSystem  0x0047CBF0 ---- */
void Scr_InitSystem( int unused, int time ) {
	VariableValueInternal *node;

	node = AllocVariable();
	node->status = ( VAR_STAT_EXTERNAL | VAR_STRUCT );
	node->u.halfword.lo = 0;
	scr_timeArrayId = (unsigned short)( node - scrVarNodes );

	node = AllocVariable();
	node->status = ( VAR_STAT_EXTERNAL | VAR_ARRAY );
	node->u.halfword.lo = 0;
	node->u.halfword.hi = 0;
	scrVarPub_pauseArrayId = (unsigned short)( node - scrVarNodes );

	node = AllocVariable();
	node->status = ( VAR_STAT_EXTERNAL | VAR_STRUCT );
	node->u.halfword.lo = 0;
	scrVarPub_levelId = (unsigned short)( node - scrVarNodes );

	node = AllocVariable();
	node->status = ( VAR_STAT_EXTERNAL | VAR_STRUCT );
	node->u.halfword.lo = 0;
	scrVarPub_animId = (unsigned short)( node - scrVarNodes );

	scrVarPub_time = (unsigned int)time & SCR_TIME_MASK;
}

/* ---- Scr_ShutdownSystem  0x0047CCD0 ---- */
void Scr_ShutdownSystem( void ) {
	unsigned short  id;

	for ( id = FindNextSibling( scr_timeArrayId ); id; id = FindNextSibling( id ) ) {
		VM_TerminateTime( FindObject( id ) );
	}

	for ( ;; ) {
		id = FindNextSibling( scrVarPub_pauseArrayId );
		if ( !id ) {
			break;
		}
		id = GetVariableValueAddress( FindNextSibling( FindObject( id ) ) )->u.halfword.lo;
		AddRefToObject( id );
		VM_TerminateNotifyList( id );
		RemoveRefToObject( id );
	}

	ClearObject( scrVarPub_levelId );
	RemoveRefToObject( scrVarPub_levelId );
	scrVarPub_levelId = 0;

	ClearObject( scrVarPub_animId );
	RemoveRefToObject( scrVarPub_animId );
	scrVarPub_animId = 0;

	ClearObject( scr_timeArrayId );
	RemoveRefToObject( scr_timeArrayId );
	scr_timeArrayId = 0;

	RemoveRefToObject( scrVarPub_pauseArrayId );
	scrVarPub_pauseArrayId = 0;
}

/* ---- VM_Init  0x00475850 ---- */
void VM_Init__Fv( void ) {
	scrVmPub_maxstack = &scrVmPub_stack[SCR_VM_STACK_COUNT - 1];
	scrVmPub_top      = &scrVmPub_stack[0];

	scrVmGlob_funcCount = 0;
	scrVarPub_error_message = NULL;
	scrVmGlob_dialog_error_message = NULL;
	scrVarPub_error_index = 0;
	scrVmPub_terminal_error = 0;
	scrVmPub_outparamcount = 0;
	scrVmPub_inparamcount = 0;

	scrVarPub_tempVariable = AllocValue();

	scr_timeArrayId        = 0;
	scrVarPub_pauseArrayId = 0;
	scrVarPub_levelId      = 0;
	scrVarPub_gameId.handle       = 0;
	scrVarPub_animId       = 0;
	scr_loading            = 0;

	scrVmPub_stack[0].type = VAR_CODEPOS;
}

int CastBool__FP13VariableValue( VariableValue *v )   { return CastBool( v ); }
int CastInt__FP13VariableValue( VariableValue *v )    { return CastInt( v ); }
int CastFloat__FP13VariableValue( VariableValue *v )  { return CastFloat( v ); }
int CastString__FP13VariableValue( VariableValue *v ) { return CastString( v ); }
int CastIString__FP13VariableValue( VariableValue *v ){ return CastIString( v ); }
int CastVector__FP13VariableValue( VariableValue *v ) { return CastVector( v ); }
int CastPointer__FP13VariableValue( VariableValue *v ){ return CastPointer( v ); }
int CastVector2__FP13VariableValue( VariableValue *v ){ CastVector2( v ); return 0; }
int ClearVector__FP13VariableValue( VariableValue *v ){ ClearVector( v ); return 0; }

/* CastWeakerPair__FP13VariableValueP13VariableValue  0x004761E0 */
int CastWeakerPair__FP13VariableValueP13VariableValue(
		VariableValue *right, VariableValue *left ) {
	return CastWeakerPair( left, right );
}
unsigned char CastWeakerPair__FP13VariableValue(VariableValue *values) {
 return CastWeakerPair(values,values+1);
}
int CheckEquality__FP13VariableValue( VariableValue *values ) {
	return CheckEquality( &values[0], &values[1] );
}
int VM_ConcatenateStrings__FP13VariableValue( VariableValue *values ) {
	return VM_ConcatenateStrings( values );
}

#ifdef __cplusplus
}
#endif
