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

#include "precompiled.h"
#pragma hdrstop

#include "snd_local.h"

/*
===================================================================================

  Thread safe decoder memory allocator.

  Each OggVorbis decoder consumes about 150kB of memory.

===================================================================================
*/

static const char *my_stbv_strerror( int stbVorbisError ) {
	switch(stbVorbisError)
	{
		case VORBIS__no_error: return "No Error";
#define ERRCASE(X) \
		case VORBIS_ ## X : return #X;

		ERRCASE( need_more_data )    // not a real error

		ERRCASE( invalid_api_mixing )           // can't mix API modes
		ERRCASE( outofmem )                     // not enough memory
		ERRCASE( feature_not_supported )        // uses floor 0
		ERRCASE( too_many_channels )            // STB_VORBIS_MAX_CHANNELS is too small
		ERRCASE( file_open_failure )            // fopen() failed
		ERRCASE( seek_without_length )          // can't seek in unknown-length file

		ERRCASE( unexpected_eof )               // file is truncated?
		ERRCASE( seek_invalid )                 // seek past EOF

		// decoding errors (corrupt/invalid stream) -- you probably
		// don't care about the exact details of these

		// vorbis errors:
		ERRCASE( invalid_setup )
		ERRCASE( invalid_stream )

		// ogg errors:
		ERRCASE( missing_capture_pattern )
		ERRCASE( invalid_stream_structure_version )
		ERRCASE( continued_packet_flag_invalid )
		ERRCASE( incorrect_stream_serial_number )
		ERRCASE( invalid_first_page )
		ERRCASE( bad_packet_type )
		ERRCASE( cant_find_last_page )
		ERRCASE( seek_failed )
		ERRCASE( ogg_skeleton_not_supported )

#undef ERRCASE
	}
	assert(0 && "unknown stb_vorbis errorcode!");
	return "Unknown Error!";
}


/*
===================================================================================

  OggVorbis file loading/decoding.

===================================================================================
*/

/*
====================
idOggFile::~idOggFile
====================
*/
void idOggFile::Close() {
	if ( vorbisFile ) {
		stb_vorbis_close( vorbisFile );
		vorbisFile = NULL;
	}

	if ( mhmmio ) {
		fileSystem->CloseFile( mhmmio );
		mhmmio = NULL;
	}
}

/*
====================
idOggFile::GetFormat
====================
*/
void idOggFile::GetFormat( idWaveFile::waveFmt_t &format ) {
	format.basic.samplesPerSec = this->info.sample_rate;
	format.basic.numChannels = this->info.channels;
	format.basic.bitsPerSample = sizeof( short ) * 8;
	format.basic.formatTag = idWaveFile::FORMAT_PCM;
	format.basic.blockSize = format.basic.numChannels * format.basic.bitsPerSample / 8;
	format.basic.avgBytesPerSec = format.basic.samplesPerSec * format.basic.blockSize;
}

/*
====================
idOggFile::Seek
====================
*/
void idOggFile::Seek( int samplePos ) {
	stb_vorbis_seek( this->vorbisFile, samplePos );
}

/*
====================
idOggFile::IsEOS
====================
*/
bool idOggFile::IsEOS() {
	int64 size = stb_vorbis_stream_length_in_samples( this->vorbisFile );
	return stb_vorbis_get_sample_offset( this->vorbisFile ) >= size;
}

/*
====================
idOggFile::Size
====================
*/
int64 idOggFile::Size() {
	int64 mdwSize = stb_vorbis_stream_length_in_samples( this->vorbisFile ) * this->info.channels;
	return mdwSize * sizeof( short );
}

/*
====================
idOggFile::CompressedSize
====================
*/
int64 idOggFile::CompressedSize() {
	return stb_vorbis_stream_length_in_samples( this->vorbisFile );
}

/*
====================
idOggFile::Read
====================
*/
int idOggFile::Read( void *pBuffer, int dwSizeToRead ) {
	// DG: Note that stb_vorbis_get_samples_short_interleaved() operates on shorts,
	//     while VorbisFile's ov_read() operates on bytes, so some numbers are different
	int total = dwSizeToRead / sizeof( short );
	short *bufferPtr = (short *)pBuffer;
	stb_vorbis *ov = (stb_vorbis *) vorbisFile;

	do {
		int numShorts = total; // total >= 2048 ? 2048 : total; - I think stb_vorbis doesn't mind decoding all of it
		int ret = stb_vorbis_get_samples_short_interleaved( ov, this->info.channels, bufferPtr, numShorts );
		if ( ret == 0 ) {
			break;
		}
		if ( ret < 0 ) {
			int stbverr = stb_vorbis_get_error( ov );
			common->Warning( "idOggFile::Read() stb_vorbis_get_samples_short_interleaved() %d shorts failed: %s\n", numShorts, my_stbv_strerror( stbverr ) );
			return -1;
		}
		// for some reason, stb_vorbis_get_samples_short_interleaved() takes the absolute
		// number of shorts to read as a function argument, but returns the number of samples
		// that were read PER CHANNEL
		ret *= this->info.channels;
		bufferPtr += ret;
		total -= ret;
	} while ( total > 0 );

	return (char *)bufferPtr - (char *)pBuffer;
}

/*
====================
idOggFile::Open
====================
*/
bool idOggFile::Open( const char * fileName ) {
	if ( mhmmio ) {
		fileSystem->CloseFile( mhmmio );
		mhmmio = NULL;
	}

	if ( vorbisFile != NULL ) {
		stb_vorbis_close( vorbisFile );
		vorbisFile = NULL;
	}

	mhmmio = fileSystem->OpenFileRead( fileName );
	if ( !mhmmio ) {
		return false;
	}

	int fileSize = mhmmio->Length();
	byte* oggFileData = (byte *)Mem_Alloc( fileSize, TAG_CRAP );

	mhmmio->Read( oggFileData, fileSize );

	int stbverr = 0;
	stb_vorbis *ov = stb_vorbis_open_memory( oggFileData, fileSize, &stbverr, NULL );
	if( ov == NULL ) {
		Mem_Free( oggFileData );
		common->Warning( "Opening OGG file '%s' with stb_vorbis failed: %s\n", fileName, my_stbv_strerror( stbverr ) );
		fileSystem->CloseFile( mhmmio );
		mhmmio = NULL;
		return false;
	}

	stb_vorbis_info stbvi = stb_vorbis_get_info( ov );
	int numSamples = stb_vorbis_stream_length_in_samples( ov );
	if( numSamples == 0 ) {
		stbverr = stb_vorbis_get_error( ov );
		common->Warning( "Couldn't get sound length of '%s' with stb_vorbis: %s\n", fileName, my_stbv_strerror( stbverr ) );
		return false;
	}

	this->vorbisFile = ov;
	this->info = stbvi;

	return true;
}
