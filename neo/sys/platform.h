/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __PLATFORM__
#define __PLATFORM__

// NOTE: By default Win32 uses a 1MB stack. Doom3 1.3.1 uses 4MB (probably set after compiling with EDITBIN /STACK
// the engine now uses a 8MB stack, set with a linker flag in CMakeLists.txt (/STACK:8388608 for MSVC, -Wl,--stack,8388608 for mingw)
// Linux has a 8MB stack by default, and so does macOS, at least for the main thread.
// Anyway, a 1MB limit for _alloca() should be safe even when using it multiple times
// in the same function or callstack.
// If there's a risk of bigger stack allocations, Mem_MallocA() should be used instead.
#define ID_MAX_ALLOCA_SIZE 1048576 // 1MB

/*
===============================================================================

	CPU Arch detection.

===============================================================================
*/

// Setting D3_ARCH for VisualC++ from CMake doesn't work when using VS integrated CMake
// so set it in code instead
#ifdef _MSC_VER
	#ifdef D3_ARCH
	  #undef D3_ARCH
	#endif // D3_ARCH

	#ifdef _M_X64
	  // this matches AMD64 and ARM64EC (but not regular ARM64), but they're supposed to be binary-compatible somehow, so whatever
	  #define D3_ARCH "x86_64"
	#elif defined( _M_ARM64 )
	  #define D3_ARCH "arm64"
	#elif defined( _M_ARM )
	  #define D3_ARCH "arm"
	#elif defined( _M_IX86 )
	  #define D3_ARCH "x86"
	#else
	  // if you're not targeting one of the aforementioned architectures,
	  #define D3_ARCH "UNKNOWN"
	#endif // _M_X64 etc
#endif // _MSC_VER

/*
===============================================================================

	Non-portable system services.

===============================================================================
*/

// Win32
#if defined( WIN32 ) || defined( _WIN32 )
	#define BUILD_OS_ID					0

	#ifdef __MINGW32__
	  #undef _alloca // in mingw _alloca is a #define
	  // NOTE: Do *not* use __builtin_alloca_with_align(), unlike regular alloca it frees at end of block instead of end of function !
	  #define _alloca16( x )			( (void *) ( (assert((x)<ID_MAX_ALLOCA_SIZE)), ((((uintptr_t)__builtin_alloca( (x)+15 )) + 15) & ~15) ) )
	  #define _alloca( x )				( (assert((x)<ID_MAX_ALLOCA_SIZE)), __builtin_alloca( (x) ) )
	#else
	  #define _alloca16( x )			( (void *) ( (assert((x)<ID_MAX_ALLOCA_SIZE)), ((((uintptr_t)_alloca( (x)+15 )) + 15) & ~15) ) )
	  #define _alloca( x )				( (void *) ( (assert((x)<ID_MAX_ALLOCA_SIZE)), _alloca( (x) ) ) )
	#endif

	#define PATHSEPERATOR_STR			"\\"
	#define PATHSEPERATOR_CHAR			'\\'

	#ifdef _MSC_VER
		#ifdef GAME_DLL
			#define ID_GAME_API					__declspec(dllexport)
		#else
			#define ID_GAME_API
		#endif
		#define ALIGN16( x )				__declspec(align(16)) x
		#define PACKED
		#define ID_INLINE					__forceinline
		// DG: alternative to forced inlining of ID_INLINE for functions that do alloca()
		//     and are called in a loop so inlining them might cause stack overflow
		#define ID_MAYBE_INLINE				__inline
		#define ID_STATIC_TEMPLATE			static
		#define assertmem( x, y )			assert( _CrtIsValidPointer( x, y, true ) )
	#else
		#ifdef GAME_DLL
			#define ID_GAME_API					__attribute__((visibility ("default")))
		#else
			#define ID_GAME_API
		#endif
		#define ALIGN16( x )				x __attribute__ ((aligned (16)))
		#define PACKED						__attribute__((packed))
		#define ID_INLINE					inline
		#define ID_STATIC_TEMPLATE
		#define assertmem( x, y )
		#define strtok_s	strtok_r
	#endif
#endif

// Mac OSX
#if defined(MACOS_X) || defined(__APPLE__)
	#define BUILD_OS_ID					1
	
	#ifdef GAME_DLL
		#define ID_GAME_API					__attribute__((visibility ("default")))
	#else
		#define ID_GAME_API
	#endif

	#define ALIGN16( x )				x __attribute__ ((aligned (16)))

	#define PACKED						__attribute__((packed))

	#define _alloca						alloca
	#define _alloca16( x )				((void *)((((uintptr_t)alloca( (x)+15 )) + 15) & ~15))

	#define PATHSEPERATOR_STR			"/"
	#define PATHSEPERATOR_CHAR			'/'

	#define __cdecl
	#define ASSERT						assert

	#define ID_INLINE					inline
	#define ID_STATIC_TEMPLATE

	#define assertmem( x, y )

	#define strtok_s	strtok_r
#endif


// Unix
#ifdef __unix__
	#define BUILD_OS_ID					3
	
	#ifdef	__GNUC__
	  // NOTE: Do *not* use __builtin_alloca_with_align(), unlike regular alloca it frees at end of block instead of end of function !
	  #define _alloca16( x )			(({assert( (x)<ID_MAX_ALLOCA_SIZE );}),((void *)((((uintptr_t)__builtin_alloca( (x)+15 )) + 15) & ~15)))
	  #define _alloca( x )				( ({assert((x)<ID_MAX_ALLOCA_SIZE);}), __builtin_alloca( (x) ) )
	#else
	  #define _alloca( x )				(({assert( (x)<ID_MAX_ALLOCA_SIZE );}), alloca( (x) ))
	  #define _alloca16( x )			(({assert( (x)<ID_MAX_ALLOCA_SIZE );}),((void *)((((uintptr_t)alloca( (x)+15 )) + 15) & ~15)))
	#endif

	#ifdef GAME_DLL
		#define ID_GAME_API					__attribute__((visibility ("default")))
	#else
		#define ID_GAME_API
	#endif

	#define ALIGN16( x )				x
	#define PACKED						__attribute__((packed))

	#define PATHSEPERATOR_STR			"/"
	#define PATHSEPERATOR_CHAR			'/'

	#define __cdecl
	#define ASSERT						assert

	#define ID_INLINE					inline
	#define ID_STATIC_TEMPLATE

	#define assertmem( x, y )

	#define strtok_s	strtok_r
#endif

#ifndef ID_MAYBE_INLINE
	// for MSVC it's __inline, otherwise just inline should work
	#define ID_MAYBE_INLINE inline
#endif // ID_MAYBE_INLINE

/*
================================================================================================
Setup for /analyze code analysis, which we currently only have on the 360, but
we may get later for win32 if we buy the higher end vc++ licenses.

Even with VS2010 ultmate, /analyze only works for x86, not x64

Also note the __analysis_assume macro in sys_assert.h relates to code analysis.

This header should be included even by job code that doesn't reference the
bulk of the codebase, so it is the best place for analyze pragmas.
================================================================================================
*/

#ifdef _MSC_VER
	// disable some /analyze warnings here
	#pragma warning( disable: 6255 )	// warning C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead. (Note: _malloca requires _freea.)
	#pragma warning( disable: 6262 )	// warning C6262: Function uses '36924' bytes of stack: exceeds /analyze:stacksize'32768'. Consider moving some data to heap
	#pragma warning( disable: 6326 )	// warning C6326: Potential comparison of a constant with another constant

	#pragma warning( disable: 6031 )	//  warning C6031: Return value ignored
	// this warning fires whenever you have two calls to new in a function, but we assume new never fails, so it is not relevant for us
	#pragma warning( disable: 6211 )	// warning C6211: Leaking memory 'staticModel' due to an exception. Consider using a local catch block to clean up memory

	// we want to fix all these at some point...
	#pragma warning( disable: 6246 )	// warning C6246: Local declaration of 'es' hides declaration of the same name in outer scope. For additional information, see previous declaration at line '969' of 'w:\tech5\rage\game\ai\fsm\fsm_combat.cpp': Lines: 969
	#pragma warning( disable: 6244 )	// warning C6244: Local declaration of 'viewList' hides previous declaration at line '67' of 'w:\tech5\engine\renderer\rendertools.cpp'

	// win32 needs this, but 360 doesn't
	#pragma warning( disable: 6540 )	// warning C6540: The use of attribute annotations on this function will invalidate all of its existing __declspec annotations [D:\tech5\engine\engine-10.vcxproj]

	#pragma warning( disable: 4467 )	// .. Include\CodeAnalysis\SourceAnnotations.h(68): warning C4467: usage of ATL attributes is deprecated

	#if !defined( VERIFY_FORMAT_STRING )
		// checking format strings catches a LOT of errors
		#include <CodeAnalysis\SourceAnnotations.h>
		#define	VERIFY_FORMAT_STRING	[SA_FormatString(Style="printf")]
		// DG: alternative for GCC with attribute (NOOP for MSVC)
		#define ID_STATIC_ATTRIBUTE_PRINTF(STRIDX, FIRSTARGIDX)
	#endif

#else
	#define	VERIFY_FORMAT_STRING
	// STRIDX: index of format string in function arguments (first arg == 1)
	// FIRSTARGIDX: index of first argument for the format string
	#define ID_STATIC_ATTRIBUTE_PRINTF(STRIDX, FIRSTARGIDX) __attribute__ ((format (printf, STRIDX, FIRSTARGIDX)))
	// DG end
#endif // _MSC_VER

// This needs to be handled so shift by 1
#define ID_INSTANCE_ATTRIBUTE_PRINTF(STRIDX, FIRSTARGIDX) ID_STATIC_ATTRIBUTE_PRINTF((STRIDX+1),(FIRSTARGIDX+1))


// We need to inform the compiler that Error() and FatalError() will
// never return, so any conditions that leeds to them being called are
// guaranteed to be false in the following code

#if defined( _MSC_VER )
	#define NO_RETURN __declspec(noreturn)
#elif defined( __GNUC__ )
	#define NO_RETURN __attribute__((noreturn))
#else
	#define NO_RETURN
#endif

#if !defined(_MSC_VER)
	// MSVC does not provide this C99 header
	#include <inttypes.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <cstddef>
#include <typeinfo>
#include <errno.h>
#include <math.h>
//#define FLT_EPSILON 1.19209290E-07F
#include <cfloat>
#include <limits>
#include <chrono>
#include <thread>
#include <algorithm>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>

	// fix stupid namespace poluting Microsoft monkeys
	#undef FindText
	#undef MessageBox
	#undef DrawText
	#undef CopyFile
	#undef LoadImage
#endif

// Apple legacy
#ifdef __APPLE__
	#include <Availability.h>
	#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
		#if __MAC_OS_X_VERSION_MIN_REQUIRED == 1040
			#define OSX_TIGER
		#elif __MAC_OS_X_VERSION_MIN_REQUIRED < 1060
			#define OSX_LEOPARD
		#endif
	#endif
#endif

#define ID_TIME_T time_t

typedef unsigned char			byte;		// 8 bits
typedef unsigned short			word;		// 16 bits
typedef unsigned int			dword;		// 32 bits
typedef unsigned int			uint;
typedef unsigned long			ulong;

typedef int						qhandle_t;

#ifndef _WIN32
typedef intptr_t      INT_PTR;
typedef unsigned int  DWORD;
typedef bool          BOOL;
#endif

#ifndef NULL
#define NULL					((void *)0)
#endif

#ifndef BIT
#define BIT( num )				( 1 << ( num ) )
#endif

#ifndef _WIN32

#ifndef TRUE
#define TRUE true
#endif

#ifndef FALSE
#define FALSE false
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#endif

#define	MAX_STRING_CHARS		1024		// max length of a string

#define round_up(x, y)	(((x) + ((y)-1)) & ~((y)-1))

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

#endif
