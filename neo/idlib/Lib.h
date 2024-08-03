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

#ifndef __LIB_H__
#define __LIB_H__

/*
===============================================================================

	idLib contains stateless support classes and concrete types. Some classes
	do have static variables, but such variables are initialized once and
	read-only after initialization (they do not maintain a modifiable state).

	The interface pointers idSys, idCommon, idCVarSystem and idFileSystem
	should be set before using idLib. The pointers stored here should not
	be used by any part of the engine except for idLib.

	The frameNumber should be continuously set to the number of the current
	frame if frame base memory logging is required.

===============================================================================
*/

#define assert_8_byte_aligned( pointer )		assert( (((UINT_PTR)(pointer))&7) == 0 );
#define assert_16_byte_aligned( pointer )		assert( (((UINT_PTR)(pointer))&15) == 0 );
#define assert_32_byte_aligned( pointer )		assert( (((UINT_PTR)(pointer))&31) == 0 );

// HUMANHEAD mdl
#ifdef ID_DEBUG_MEMORY //rww - this is the appropriate define to use
# error "compile_time_asserts are not currently compatible with inline debug with memory builds.  Comment them out and disable this check to continue."
#endif
// HUMANHEAD END

#define compile_time_assert( x )				{ typedef int compile_time_assert_failed[(x) ? 1 : -1]; }
#define file_scoped_compile_time_assert( x )	extern int compile_time_assert_failed[(x) ? 1 : -1]
#define assert_sizeof( type, size )				file_scoped_compile_time_assert( sizeof( type ) == size )
#define assert_offsetof( type, field, offset )	file_scoped_compile_time_assert( offsetof( type, field ) == offset )
#define assert_sizeof_8_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 8 ) == 0 )
#define assert_sizeof_16_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 15 ) == 0 )
#define assert_sizeof_32_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 31 ) == 0 )

class idLib {
public:
	static class idSys *		sys;
	static class idCommon *		common;
	static class idCVarSystem *	cvarSystem;
	static class idFileSystem *	fileSystem;
	static int					frameNumber;

	static void					Init( void );
	static void					ShutDown( void );

	// wrapper to idCommon functions
	static void					Error( const char *fmt, ... );
	static void					Warning( const char *fmt, ... );
	
#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
	static void					NetworkEntStats(const char *typeName, int type);
#endif //HUMANHEAD END
};


/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

class idFile;
class idVec3;
class idVec4;

// basic colors
extern	idVec4 colorBlack;
extern	idVec4 colorWhite;
extern	idVec4 colorRed;
extern	idVec4 colorGreen;
extern	idVec4 colorBlue;
extern	idVec4 colorYellow;
extern	idVec4 colorMagenta;
extern	idVec4 colorCyan;
extern	idVec4 colorOrange;
extern	idVec4 colorPurple;
extern	idVec4 colorPink;
extern	idVec4 colorBrown;
extern	idVec4 colorLtGrey;
extern	idVec4 colorMdGrey;
extern	idVec4 colorDkGrey;

// packs color floats in the range [0,1] into an integer
dword	PackColor( const idVec3 &color );
void	UnpackColor( const dword color, idVec3 &unpackedColor );
dword	PackColor( const idVec4 &color );
void	UnpackColor( const dword color, idVec4 &unpackedColor );

// little/big endian conversion
short	BigShort( short l );
short	LittleShort( short l );
int		BigInt( int l );
int		LittleInt( int l );
float	BigFloat( float l );
float	LittleFloat( float l );
void	BigRevBytes( void *bp, int elsize, int elcount );
void	LittleRevBytes( void *bp, int elsize, int elcount );
void	LittleBitField( void *bp, int elsize );
void	Swap_Init( void );

bool	Swap_IsBigEndian( void );

// for base64
void	SixtetsForInt( byte *out, int src);
int		IntForSixtets( byte *in );


#ifdef _DEBUG
void AssertFailed( const char *file, int line, const char *expression );
#undef assert
#ifdef HUMANHEAD_TESTSAVEGAME // HUMANHEAD mdl:  Disable asserts for automated savegame tests
#  define assert( X )
#else
// DG: change assert to use ?: so I can use it in _alloca()/_alloca16() (MSVC didn't like if() in there)
#	define assert( X )			(X) ? 1 : (AssertFailed( __FILE__, __LINE__, #X ), 0)
#endif
// HUMANHEAD END
#endif

class idException {
public:
	char error[MAX_STRING_CHARS];

	idException( const char *text = "" ) { strcpy( error, text ); }
};

// move from Math.h to keep gcc happy
template<class T> ID_INLINE T	Max( T x, T y ) { return ( x > y ) ? x : y; }
template<class T> ID_INLINE T	Min( T x, T y ) { return ( x < y ) ? x : y; }

/*
===============================================================================

	idLib headers.

===============================================================================
*/

// memory management and arrays
#include "Heap.h"
#include "containers/List.h"

// math
#include "math/Simd.h"
#include "math/Math.h"
#include "math/Random.h"
#include "math/Complex.h"
#include "math/Vector.h"
#include "math/Matrix.h"
#include "math/Angles.h"
#include "math/Quat.h"
#include "math/Rotation.h"
#include "math/Plane.h"
#include "math/Pluecker.h"
#include "math/Polynomial.h"
#include "math/Extrapolate.h"
#include "math/Interpolate.h"
#include "math/prey_interpolate.h"		// HUMANHEAD pdm
#include "math/prey_math.h"				// HUMANHEAD pdm
#include "math/Curve.h"
#include "math/Ode.h"
#include "math/Lcp.h"

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
#include "bv/Box.h"
#include "bv/Frustum.h"

// geometry
#include "geometry/DrawVert.h"
#include "geometry/JointTransform.h"
#include "geometry/Winding.h"
#include "geometry/Winding2D.h"
#include "geometry/Surface.h"
#include "geometry/Surface_Patch.h"
#include "geometry/Surface_Polytope.h"
#include "geometry/Surface_SweptSpline.h"
#include "geometry/TraceModel.h"

// text manipulation
#include "Str.h"
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include "Base64.h"
#include "CmdArgs.h"

// containers
#include "containers/BTree.h"
#include "containers/BinSearch.h"
#include "containers/HashIndex.h"
#include "containers/HashTable.h"
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"

// HUMANHEAD pdm: idlib additions
#include "containers/PreyStack.h"
// HUMANHEAD END

// hashing
#include "hashing/CRC8.h"
#include "hashing/CRC16.h"
#include "hashing/CRC32.h"
#include "hashing/Honeyman.h"
#include "hashing/MD4.h"
#include "hashing/MD5.h"

// misc
#include "Dict.h"
#include "LangDict.h"
#include "BitMsg.h"
#include "MapFile.h"
#include "Timer.h"

#endif	/* !__LIB_H__ */
