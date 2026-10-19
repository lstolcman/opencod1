/*
 * vc7_compat.h -- one source, two compilers.
 *
 * Retail was built with Visual C++ .NET 2002 (cl 13.00.9466, _MSC_VER 1300), and tools/vc7/build.py
 * builds this tree with that same compiler so functions can be matched byte for byte. The default
 * premake build uses modern MSVC. The two disagree about x87 precision:
 *
 *   VC7 (without /Op) keeps a value at the FPU's 53-bit precision across a (float) cast and across an
 *   assignment to a float local. It rounds to float only where it spills the value to a 4-byte slot.
 *   Modern MSVC (/fp:precise) rounds at every cast and every assignment.
 *
 * Source written the way retail's was (float locals, (float) casts) therefore reproduces retail's
 * instructions under VC7 but rounds more often under modern MSVC. These two markers keep both
 * builds faithful where it matters:
 *
 *   FCAST(x)  a (float) cast that retail's source had. VC7: the cast (no rounding, same instructions).
 *             Modern: no cast, so no rounding either.
 *   rfloat    a float local that retail keeps in an FPU register (never stored to a 4-byte slot).
 *             VC7: float. Modern: double, so it is not rounded. A local that retail does store as a
 *             dword (fstp dword ptr [esp+..]) is rounded in retail too: declare it plain float.
 *
 * Neither marker belongs in a /Op unit (universal/com_math.c): under /Op VC7 rounds at casts and
 * assignments like modern MSVC, so plain float and (float) already agree there.
 */
#ifndef __VC7_COMPAT_H__
#define __VC7_COMPAT_H__

#if defined( _MSC_VER ) && _MSC_VER < 1310
#define COD1_VC7 1
#define FCAST( x ) ( ( float )( x ) )
typedef float rfloat;
#else
#define FCAST( x ) ( x )
typedef double rfloat;
#endif

#endif /* __VC7_COMPAT_H__ */
