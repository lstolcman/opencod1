/*
 * qcommon/crt_thunks.c
 *
 * C names for CRT and C++ runtime functions the tree calls under names of its
 * own (retail's IDB names: j__atol, j__free, qsort_m, putenv_m), and for C++
 * runtime functions that C cannot name (crt_operator_new, std_String_base_Xran,
 * std_String_base_Xlen; declared in qcommon/crt_cpp_runtime.h).
 *
 * The VC7 build links the libraries retail linked (LIBCMT, LIBCPMT), so there
 * every one of these names is a weak-external alias of the library function
 * itself (tools/vc7/crt_aliases.asm, assembled into vc7_crt_aliases.lib by
 * tools/vc7/build.py): no thunk and no recompiled copy.  LINK 7.0 has no
 * /ALTERNATENAME; the COFF weak externals are what OLDNAMES.LIB uses.  The
 * modern build has a different CRT, so it compiles the bodies below, which
 * reproduce the VC7 behaviour.
 *
 * @fidelity: verified
 */

#include <ctype.h>
#include <stdlib.h>
#include "vc7_compat.h"
#include "crt_cpp_runtime.h"

/* Linker/CRT-level names the tree defines itself: the /GS cookie (retail
 * 0x00570930) and the image base (0x00400000).  Nothing in the tree names
 * them; the /GS prologues and the CRT reference __security_cookie. */
unsigned short __ImageBase;
int __security_cookie;

#ifdef COD1_VC7
#pragma comment( lib, "vc7_crt_aliases" )
#pragma comment( lib, "libcpmt" )	/* string.obj: std::_String_base::_Xran / _Xlen */
#else

/* j__atol is libcmt's atoi (atox.obj, 0x00525445): a plain jmp to its _atol
 * (0x005253BD).  The modern build reproduces that _atol body instead of
 * forwarding to the host CRT: the UCRT atol saturates to LONG_MAX/LONG_MIN
 * (and sets ERANGE) on overflow, whereas the VC7 one has no overflow check and
 * wraps modulo 2^32. */
/* ---- j__atol  0x00525445 ---- */
long j__atol( const char *string ) {
	const unsigned char *p = (const unsigned char *)string;
	unsigned long total = 0;	/* retail: lea eax,[eax+eax*4]; lea eax,[ecx+eax*2], no overflow check -> wraps mod 2^32 */
	int c, sign;

	while ( isspace( *p ) )	/* retail: movzx byte + _pctype mask 8 (_SPACE); _setlocale has no callers, so "C" locale */
		p++;

	c = *p++;
	sign = c;
	if ( c == '-' || c == '+' )
		c = *p++;

	while ( c >= '0' && c <= '9' ) {
		total = 10 * total + (unsigned long)( c - '0' );
		c = *p++;
	}

	return (long)( sign == '-' ? 0ul - total : total );	/* retail: neg eax */
}

/* j__free is libcmt's operator delete (delete.obj, 0x00528435): jmp _free. */
/* ---- j__free  0x00528435 ---- */
void j__free( void *block ) {
	free( block );
}

/* qsort_m is retail's CRT qsort (libcmt qsort.obj, 0x00526580; VC7 qsort.c:
 * median-of-three quicksort, selection sort for runs of <= 8), reproduced
 * here so the order of equal elements is retail's in every build, not the
 * host CRT's. */
#define CUTOFF 8
#define STKSIZ ( 8 * sizeof( void * ) - 2 )

/* ---- swap_m  0x005264F0 ---- */
static void __cdecl swap_m( char *a, char *b, unsigned int width ) {
	char tmp;

	if ( a != b ) {
		while ( width-- ) {
			tmp = *a;
			*a++ = *b;
			*b++ = tmp;
		}
	}
}

/* ---- shortsort_m  0x00526510 ---- */
static void __cdecl shortsort_m( char *lo, char *hi, unsigned int width,
								 int ( __cdecl *comp )( const void *, const void * ) ) {
	char *p, *max;

	while ( hi > lo ) {
		max = lo;
		for ( p = lo + width; p <= hi; p += width ) {
			if ( comp( p, max ) > 0 ) {
				max = p;
			}
		}
		swap_m( max, hi, width );
		hi -= width;
	}
}

/* ---- qsort_m  0x00526580 ---- */
void qsort_m( void *base, unsigned int num, unsigned int width,
			  int ( __cdecl *comp )( const void *, const void * ) ) {
	char *lo, *hi;
	char *mid;
	char *loguy, *higuy;
	unsigned int size;
	char *lostk[STKSIZ], *histk[STKSIZ];
	int stkptr;

	if ( num < 2 || width == 0 )
		return;

	stkptr = 0;

	lo = base;
	hi = (char *)base + width * ( num - 1 );

recurse:

	size = ( hi - lo ) / width + 1;

	if ( size <= CUTOFF ) {
		shortsort_m( lo, hi, width, comp );
	} else {
		mid = lo + ( size / 2 ) * width;

		if ( comp( lo, mid ) > 0 ) {
			swap_m( lo, mid, width );
		}
		if ( comp( lo, hi ) > 0 ) {
			swap_m( lo, hi, width );
		}
		if ( comp( mid, hi ) > 0 ) {
			swap_m( mid, hi, width );
		}

		loguy = lo;
		higuy = hi;

		for ( ;; ) {
			if ( mid > loguy ) {
				do {
					loguy += width;
				} while ( loguy < mid && comp( loguy, mid ) <= 0 );
			}
			if ( mid <= loguy ) {
				do {
					loguy += width;
				} while ( loguy <= hi && comp( loguy, mid ) <= 0 );
			}

			do {
				higuy -= width;
			} while ( higuy > mid && comp( higuy, mid ) > 0 );

			if ( higuy < loguy )
				break;

			swap_m( loguy, higuy, width );

			if ( mid == higuy )
				mid = loguy;
		}

		higuy += width;
		if ( mid < higuy ) {
			do {
				higuy -= width;
			} while ( higuy > mid && comp( higuy, mid ) == 0 );
		}
		if ( mid >= higuy ) {
			do {
				higuy -= width;
			} while ( higuy > lo && comp( higuy, mid ) == 0 );
		}

		if ( higuy - lo >= hi - loguy ) {
			if ( lo < higuy ) {
				lostk[stkptr] = lo;
				histk[stkptr] = higuy;
				++stkptr;
			}

			if ( loguy < hi ) {
				lo = loguy;
				goto recurse;
			}
		} else {
			if ( loguy < hi ) {
				lostk[stkptr] = loguy;
				histk[stkptr] = hi;
				++stkptr;
			}

			if ( lo < higuy ) {
				hi = higuy;
				goto recurse;
			}
		}
	}

	--stkptr;
	if ( stkptr >= 0 ) {
		lo = lostk[stkptr];
		hi = histk[stkptr];
		goto recurse;
	} else
		return;
}

/* putenv_m is the CRT _putenv (libcmt putenv.obj, 0x005370C6: _lock(_ENV_LOCK)
 * around _putenv_lk).  Named putenv_m, not _putenv, on the qsort_m precedent:
 * the undecorated CRT spelling would collide with the <stdlib.h> import.
 * CL_Setenv_f is the only caller, in retail (0x0040F55D) and here. */
/* ---- putenv_m  0x005370C6 ---- */
int putenv_m( const char *envstring ) {
	return _putenv( envstring );
}

/* operator new (libcmt new.obj, 0x00528637) is _nh_malloc( size, 1 ). */
void *crt_operator_new( unsigned int size ) {
	return malloc( size );
}

/* std::_String_base::_Xran / _Xlen (libcpmt string.obj, 0x00536DB0 /
 * 0x00536DF0) throw std::out_of_range / std::length_error, which nothing
 * catches: terminate(), which aborts. */
void std_String_base_Xran( void ) {
	abort();
}

void std_String_base_Xlen( void ) {
	abort();
}

#endif /* COD1_VC7 */

#if !defined( _MSC_VER ) || _MSC_VER < 1400 /* the __rdtsc intrinsic arrived with VC8; VC7 (1300) lacks it */
__declspec( naked ) int __rdtsc( void ) {
	__asm {
		push    edx
		_emit   0x0F
		_emit   0x31
		pop     edx
		ret
	}
}
#endif
