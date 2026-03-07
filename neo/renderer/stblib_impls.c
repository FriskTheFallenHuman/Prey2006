/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
	this source file includes the implementations of stb_image and stb_image_write
	having it in a separate source file allows optimizing it in debug builds (for faster load times)
	without hurting the debugability of the source files stb_image(_write) is used in
*/

// include this first, otherwise build breaks because of  use_idStr_* #defines in Str.h
#if defined(__APPLE__) && !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 5
  // Extra-Hack for ancient GCC 4.2-based Apple compilers that don't support __thread
  #define STBI_NO_THREAD_LOCALS
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_STDIO  // images are passed as buffers
#include "stb/stb_image.h"

#include "miniz/miniz.h"

static unsigned char *compress_for_stbiw( unsigned char  *data, int data_len, int *out_len, int quality ) {
	uLongf bufSize = mz_compressBound( data_len );
	// note that buf will be free'd by stb_image_write.h
	// with STBIW_FREE() (plain free() by default)
	unsigned char* buf = ( unsigned char* )malloc( bufSize );
	if ( buf == NULL ) {
		return NULL;
	}
	if ( mz_compress2( buf, &bufSize, data, data_len, quality ) != MZ_OK ) {
		free( buf );
		return NULL;
	}
	*out_len = bufSize;

	return buf;
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ZLIB_COMPRESS compress_for_stbiw
#include "stb/stb_image_write.h"

