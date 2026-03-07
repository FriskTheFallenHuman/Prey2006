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
#ifndef __OGGFILE_H
#define __OGGFILE_H

/*
================================================================================================
Contains the OggFile declaration.
================================================================================================
*/

//#include "SDL_endian.h"
//#if SDL_BYTEORDER == SDL_BIG_ENDIAN
//  #define STB_VORBIS_BIG_ENDIAN
//#endif
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API // we're using the pulldata API
#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.h"

/*
================================================
idOggFile is used for reading generic Vorbis Ogg files.
================================================
*/
class idOggFile {
public:
	ID_INLINE 	idOggFile();
	ID_INLINE 	~idOggFile();

	bool	Open( const char *fileName );
	void	Close();
	bool	IsEOS();
	void	Seek( int samplePos );
	int		Read( void *buffer, int bufferSize );
	int64	Size();
	int64	CompressedSize();
	void	GetFormat( idWaveFile::waveFmt_t &format );

private:
	stb_vorbis * vorbisFile;
	stb_vorbis_info info;
	idFile * mhmmio;
};

/*
========================
idOggFile::idOggFile
========================
*/
ID_INLINE idOggFile::idOggFile() :
	vorbisFile( NULL ),
	mhmmio( NULL ) {
}

/*
========================
idOggFile::~idOggFile
========================
*/
ID_INLINE idOggFile::~idOggFile() {
	Close();
}

#endif // !__WAVEFILE_H__