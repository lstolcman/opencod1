/*
 * crt_cpp_runtime.h -- C names for C++ runtime functions retail's C++ units
 * called, for the C reconstructions of those units (effects and StringEd STL
 * code).  The VC7 build resolves each name to the library function itself
 * (LIBCMT new.obj, LIBCPMT string.obj) through a weak-external alias
 * (tools/vc7/crt_aliases.asm); the modern build compiles the fallbacks in
 * qcommon/crt_thunks.c.
 */

#ifndef __CRT_CPP_RUNTIME_H__
#define __CRT_CPP_RUNTIME_H__

#ifdef __cplusplus
extern "C" {
#endif

/* void *operator new( size_t ), retail 0x00528637: _nh_malloc( size, 1 ). */
void *__cdecl crt_operator_new( unsigned int size );

/* std::_String_base::_Xran() const, retail 0x00536DB0: throws
 * std::out_of_range( "invalid string position" ).  It ignores this, so a
 * plain call is retail's call sequence.  Not noreturn: retail's callers
 * continue after the call. */
void __cdecl std_String_base_Xran( void );

/* std::_String_base::_Xlen() const, retail 0x00536DF0: throws
 * std::length_error( "string too long" ). */
void __cdecl std_String_base_Xlen( void );

#ifdef __cplusplus
}
#endif

#endif /* __CRT_CPP_RUNTIME_H__ */
